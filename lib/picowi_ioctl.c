// PicoWi IOCTL functions, see http://iosoft.blog/picowi for details
//
// Copyright (c) 2022, Jeremy P Bentham
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdio.h>
#include <string.h>

#include "picowi_defs.h"
#include "picowi_pico.h"
#include "picowi_wifi.h"
#include "picowi_init.h"
#include "picowi_regs.h"
#include "picowi_ioctl.h"
#include "picowi_event.h"

IOCTL_MSG ioctl_txmsg, ioctl_rxmsg;
uint8_t sd_tx_seq = 1; //event_mask[EVENT_MAX / 8];
uint16_t ioctl_reqid=1;
extern int display_mode;

// Set an unsigned integer IOCTL variable
int ioctl_set_uint32(char *name, int wait_msec, uint32_t val)
{
    U32DATA u32 = {.uint32=val};

    return(ioctl_cmd(WLC_SET_VAR, name, strlen(name)+1, wait_msec, 1, u32.bytes, 4));
}

// Set data block in IOCTL variable, using variable name that has data
int ioctl_set_data2(char *name, int namelen, int wait_msec, void *data, int len)
{
    return(ioctl_cmd(WLC_SET_VAR, name, namelen, wait_msec, 1, data, len));
}

// Set 2 integers in IOCTL variable
int ioctl_set_intx2(char *name, int wait_msec, int val1, int val2)
{
    int data[2] = {val1, val2};

    return(ioctl_cmd(WLC_SET_VAR, name, strlen(name)+1, wait_msec, 1, data, 8));
}

// Get data block from IOCTL variable
int ioctl_get_data(char *name, int wait_msec, uint8_t *data, int dlen)
{
    return(ioctl_cmd(WLC_GET_VAR, name, strlen(name)+1, wait_msec, 0, data, dlen));
}

// Set data block in IOCTL variable
int ioctl_set_data(char *name, int wait_msec, void *data, int len)
{
    return(ioctl_cmd(WLC_SET_VAR, name, strlen(name)+1, wait_msec, 1, data, len));
}

// IOCTL write with integer parameter
int ioctl_wr_int32(int cmd, int wait_msec, int val)
{
    U32DATA u32 = {.uint32=(uint32_t)val};

    return(ioctl_cmd(cmd, 0, 0, wait_msec, 1, u32.bytes, 4));
}

// IOCTL write data
int ioctl_wr_data(int cmd, int wait_msec, void *data, int len)
{
    return(ioctl_cmd(cmd, 0, 0, wait_msec, 1, data, len));
}

// IOCTL read data
int ioctl_rd_data(int cmd, int wait_msec, void *data, int len)
{
    return(ioctl_cmd(cmd, 0, 0, wait_msec, 0, data, len));
}

// Do an IOCTL transaction, get response
// Return 0 if timeout, -1 if error response
int ioctl_cmd(int cmd, char *name, int namelen, int wait_msec, int wr, void *data, int dlen)
{
    IOCTL_CMD *cmdp = &ioctl_txmsg.cmd;
    int txdlen = ((namelen + dlen + 3) / 4) * 4, ret = 0;
    int hdrlen = sizeof(SDPCM_HDR) + sizeof(IOCTL_HDR);
    int txlen = hdrlen + txdlen;

    display(DISP_IOCTL, "Tx_IOCTL len %u cmd %u %s ", dlen, cmd,
            cmd==WLC_GET_VAR ? "get": cmd==WLC_SET_VAR ? "set": "");
    display(DISP_IOCTL, "%s\n", name ? name : "");
    memset(cmdp, 0, sizeof(ioctl_txmsg));
    cmdp->sdpcm.notlen = ~(cmdp->sdpcm.len = txlen);
    cmdp->sdpcm.seq = sd_tx_seq++;
    cmdp->sdpcm.chan = SDPCM_CHAN_CTRL;
    cmdp->sdpcm.hdrlen = sizeof(SDPCM_HDR);
    cmdp->ioctl.cmd = cmd;
    cmdp->ioctl.outlen = txdlen;
    cmdp->ioctl.flags = ((uint32_t)ioctl_reqid++ << 16) | (wr ? 2 : 0);
    if (namelen)
        memcpy(cmdp->data, name, namelen);
    if (wr && dlen>0)
        memcpy(&cmdp->data[namelen], data, dlen);
    display(DISP_SDPCM, "Tx_SDPCM len %u chan %u seq %u\n",
            cmdp->sdpcm.len, cmdp->sdpcm.chan, cmdp->sdpcm.seq);
    wifi_data_write(SD_FUNC_RAD, 0, (void *)cmdp, txlen);
    while (wait_msec>=0 && !(ret=ioctl_resp_match(cmd, data, dlen)))
    {
        wait_msec -= IOCTL_POLL_MSEC;        
        usdelay(IOCTL_POLL_MSEC * 1000);
    }
    return(ret);
}

// Read an ioctl response, match the given command, any command if 0
// Return 0 if no response, -ve if error response
int ioctl_resp_match(int cmd, void *data, int dlen)
{
    int rxlen=0, n=0, hdrlen;
    IOCTL_MSG *rsp = &ioctl_rxmsg;
    IOCTL_HDR *iohp;
    
    if ((rxlen = event_read(rsp, 0, 0)) > 0)
    {
        iohp = (IOCTL_HDR *)&rsp->data[rsp->cmd.sdpcm.hdrlen]; 
        hdrlen = rsp->cmd.sdpcm.hdrlen + sizeof(IOCTL_HDR);
        if (rsp->rsp.sdpcm.chan==SDPCM_CHAN_CTRL && 
            (cmd==0 || cmd==iohp->cmd))
        {
            n = MIN(dlen, rxlen-hdrlen);
            if (data && n>0)
                memcpy(data, &rsp->data[hdrlen], n);
            if (cmd)
            {
                display(DISP_IOCTL, "Rx_IOCTL len %u cmd %u flags 0x%X status 0x%X\n", 
                    n, cmd, iohp->flags, iohp->status);
                if (iohp->status)
                    n = -1;
            }
        }
    }
    return(cmd==0 ? rxlen : n);
}

// Display last IOCTL if error
void ioctl_err_display(int retval)
{
    IOCTL_MSG *msgp = &ioctl_txmsg;
    IOCTL_HDR *iohp = (IOCTL_HDR *)&msgp->data[msgp->cmd.sdpcm.hdrlen];
    char *cmds = iohp->cmd==WLC_GET_VAR ? "GET" : 
                 iohp->cmd==WLC_SET_VAR ? "SET" : "";
    char *data, *name;
    
    if (retval <= 0)
    {
        data = (char *)&msgp->data[msgp->cmd.sdpcm.hdrlen+sizeof(IOCTL_HDR)];
        name = iohp->cmd==WLC_GET_VAR || iohp->cmd==WLC_SET_VAR ? data : "";
        printf("IOCTL error: cmd %lu %s %s\n", iohp->cmd, cmds, name);
    }
}

// EOF
