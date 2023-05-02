// PicoWi event handler, see http://iosoft.blog/picowi for details
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
#include "picowi_init.h"
#include "picowi_wifi.h"
#include "picowi_regs.h"
#include "picowi_ioctl.h"
#include "picowi_evtnum.h"
#include "picowi_event.h"

#define MAX_HANDLERS    20
int num_handlers;
event_handler_t event_handlers[MAX_HANDLERS];
WORD event_ports[MAX_HANDLERS];
EVT_STR *current_evts;

uint8_t event_mask[EVENT_MAX / 8];
uint8_t rxdata[RXDATA_LEN];
EVENT_INFO event_info;
TX_MSG tx_msg = {.sdpcm = {.chan=SDPCM_CHAN_DATA, .hdrlen=sizeof(SDPCM_HDR)+2},
                 .bdc =   {.flags=0x20}};

extern IOCTL_MSG ioctl_txmsg, ioctl_rxmsg;
extern uint8_t sd_tx_seq;

extern void rx_frame(void *buff, uint16_t len);

// Enable events
int events_enable(const EVT_STR *evtp)
{
    current_evts = (EVT_STR *)evtp;
    memset(event_mask, 0, sizeof(event_mask));
    while (evtp->num >= 0)
    {
        if (evtp->num / 8 < sizeof(event_mask))
            SET_EVENT(event_mask, evtp->num);
        evtp++;
    }
    return(ioctl_set_data("bsscfg:event_msgs", 10, event_mask, sizeof(event_mask)));
}

// Add an event handler to the chain
bool add_event_handler(event_handler_t fn)
{
    return(add_server_event_handler(fn , 0));
}

// Add a server event handler to the chain (with local port number)
bool add_server_event_handler(event_handler_t fn, WORD port)
{
    bool ok = num_handlers < MAX_HANDLERS;
    if (ok)
    {
        event_ports[num_handlers] = port;
        event_handlers[num_handlers++] = fn;
    }
    return (ok);
}

// Run event handlers, until one returns non-zero
int event_handle(EVENT_INFO *eip)
{
    int i, ret=0;
    
    for (i=0; i<num_handlers && !ret; i++)
    {
        eip->server_port = event_ports[i];
        ret = event_handlers[i](eip);
    }
    return(ret);
}

// Poll for async event, put results in info structure, data in rxdata
int event_poll(void)
{
    EVENT_INFO *eip = &event_info;
    IOCTL_MSG *iomp = &ioctl_rxmsg;
    ESCAN_RESULT *erp=(ESCAN_RESULT *)rxdata;
    EVENT_HDR *ehp = &erp->eventh;
    int ret = 0, n;
        
    n = event_read(iomp, rxdata, sizeof(rxdata));
    if (n > 0)
    {
        eip->chan = iomp->rsp.sdpcm.chan;
        eip->flags = SWAP16(ehp->flags);
        eip->event_type = SWAP32(ehp->event_type);
        eip->status = SWAP32(ehp->status);
        eip->reason = SWAP32(ehp->reason);
        eip->data = rxdata;
        eip->dlen = n;
        eip->sock = -1;
        display(DISP_EVENT, "Rx_%s ", sdpcm_chan_str(eip->chan));
        if (eip->chan == SDPCM_CHAN_CTRL)
            display(DISP_EVENT, "\n");
        else if (eip->chan==SDPCM_CHAN_EVT &&
            n >= sizeof(ETHER_HDR)+sizeof(BCMETH_HDR)+sizeof(EVENT_HDR))
        {
            display(DISP_EVENT, "%2lu %s, flags %u, status %lu, reason %lu\n",
                eip->event_type, event_str(eip->event_type),
                eip->flags, eip->status, eip->reason);
            ret = event_handle(eip);
        }
        else if (eip->chan == SDPCM_CHAN_DATA) 
        {
            display(DISP_EVENT, "len %d\n", n);
            disp_bytes(DISP_DATA, rxdata, n);
            display(DISP_DATA, "\n");
            ret = event_handle(eip);
        }
    }
    return(ret);
}

// Get ioctl response, async event, or network data
// Optionally copy data after SDPCM & BDC headers into a buffer, return its length
int event_read(IOCTL_MSG *rsp, void *data, int dlen)
{
    int rxlen=0, n=0, hdrlen;
    SDPCM_HDR *sdp=&rsp->rsp.sdpcm;
    BDC_HDR *bdcp;
    
    if ((rxlen = event_get_resp(rsp, sizeof(IOCTL_MSG))) >= sizeof(SDPCM_HDR)+sizeof(BDC_HDR))
    {
        if ((sdp->len ^ sdp->notlen) == 0xffff && sdp->chan <= SDPCM_CHAN_DATA)
        {
            if (sdp->chan==SDPCM_CHAN_CTRL || sdp->chan==SDPCM_CHAN_DATA)
                display(DISP_SDPCM, "Rx_SDPCM len %u chan %u seq %u flow %u credit %u hdrlen %u\n",
                sdp->len, sdp->chan, sdp->seq, sdp->flow, sdp->credit, sdp->hdrlen);
            else
                display(DISP_SDPCM, "Rx_SDPCM len %u chan %u\n", sdp->len, sdp->chan);
            hdrlen = sdp->hdrlen;
            bdcp = (BDC_HDR *)&rsp->data[hdrlen];
            hdrlen += sizeof(BDC_HDR) + bdcp->offset*4;
            n = MIN(dlen, rxlen-hdrlen);
            if (data && n>0 && n<=sizeof(rsp->data)-hdrlen)
                memcpy(data, &rsp->data[hdrlen], n);
        }
    }
    return(dlen>0 ? (n>0 ? n : 0) : rxlen);
}

// Get ioctl response, async event, or network data.
int event_get_resp(void *data, int maxlen)
{
    uint32_t val=0;
    int rxlen=0;
    val = wifi_reg_read(SD_FUNC_BUS, SPI_STATUS_REG, 4);
    if ((val != ~0) && (val & SPI_STATUS_PKT_AVAIL))
    {
        rxlen = (val >> SPI_STATUS_LEN_SHIFT) & SPI_STATUS_LEN_MASK;
        rxlen = MIN(rxlen, maxlen);
        // Read event data if present
        if (data && rxlen>0)
            wifi_data_read(SD_FUNC_RAD, 0, data, rxlen);
        // ..or clear interrupt, and discard data
        else
        {
            wifi_reg_write(SD_FUNC_BAK, SPI_FRAME_CONTROL, 0x01, 1);
            val = wifi_reg_read(SD_FUNC_BUS, SPI_INTERRUPT_REG, 2);
            wifi_reg_write(SD_FUNC_BUS, SPI_INTERRUPT_REG, val, 2);
            rxlen = 0;
        }
    }
    return(rxlen);
}

// Return string corresponding to SDPCM channel number
char *sdpcm_chan_str(int chan)
{
    return(chan==SDPCM_CHAN_CTRL ? "CTRL" : chan==SDPCM_CHAN_EVT ? "EVT ": 
           chan==SDPCM_CHAN_DATA ? "DATA" : "?");
}

// Return string corresponding to event number, without "WLC_E_" prefix
char *event_str(int event)
{
    EVT_STR *evtp=current_evts;

    while (evtp && evtp->num>=0 && evtp->num!=event)
        evtp++;
    return(evtp && evtp->num>=0 && strlen(evtp->str)>6 ? &evtp->str[6] : "?");
}

// Transmit network data
int event_net_tx(void *data, int len)
{
    TX_MSG *txp = &tx_msg;
    uint8_t *dp = (uint8_t *)txp;
    int txlen = sizeof(SDPCM_HDR)+2+sizeof(BDC_HDR)+len;
    
    display(DISP_DATA, "Tx_DATA len %d\n", len);
    disp_bytes(DISP_DATA, data, len);
    display(DISP_DATA, "\n");
    txp->sdpcm.len = txlen;
    txp->sdpcm.notlen = ~txp->sdpcm.len;
    txp->sdpcm.seq = sd_tx_seq++;
    memcpy(txp->data, data, len);
    if (!wifi_reg_val_wait(10, SD_FUNC_BUS, SPI_STATUS_REG, 
            SPI_STATUS_F2_RX_READY, SPI_STATUS_F2_RX_READY, 4))
        return(0);
    while (txlen & 3)
        dp[txlen++] = 0;
    return (wifi_data_write(SD_FUNC_RAD, 0, dp, txlen));
}

// EOF
