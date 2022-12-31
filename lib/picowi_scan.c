// PicoWi network scan functions, see https://iosoft.blog/picowi
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
#include "picowi_ioctl.h"
#include "picowi_event.h"
#include "picowi_scan.h"
#include "picowi_evtnum.h"

extern IOCTL_MSG ioctl_txmsg, ioctl_rxmsg;

SCAN_PARAMS scan_params = {
    .version=1, .action=1, .sync_id=0x1, .ssidlen=0, .ssid={0},
    .bssid={0xff,0xff,0xff,0xff,0xff,0xff}, .bss_type=2,
    .scan_type=SCANTYPE_PASSIVE, .nprobes=~0, .active_time=~0,
    .passive_time=~0, .home_time=~0, .nchans=0
};

extern EVENT_INFO event_info;

// Event handling
const EVT_STR escan_evts[] = {EVT(WLC_E_ESCAN_RESULT), 
                              EVT(WLC_E_SET_SSID), 
                              EVT(-1)};

// Start a network scan
int scan_start(void)
{
    int ret;
    
    events_enable(escan_evts);
    ret = ioctl_wr_int32(WLC_SET_SCAN_CHANNEL_TIME, 10, SCAN_CHAN_TIME) > 0 &&
        ioctl_set_uint32("pm2_sleep_ret", IOCTL_WAIT, 0xc8) > 0 &&
        ioctl_set_uint32("bcn_li_bcn", IOCTL_WAIT, 1) > 0 &&
        ioctl_set_uint32("bcn_li_dtim", IOCTL_WAIT, 1) > 0 &&
        ioctl_set_uint32("assoc_listen", IOCTL_WAIT, 0x0a) > 0 &&
        ioctl_wr_int32(WLC_SET_BAND, IOCTL_WAIT, WIFI_BAND_ANY) > 0 &&
        ioctl_wr_int32(WLC_UP, IOCTL_WAIT, 0) > 0 &&
        ioctl_set_data("escan", IOCTL_WAIT, &scan_params, sizeof(scan_params)) > 0;
    ioctl_err_display(ret);
    return(ret);
}

// Handler for scan events
int scan_event_handler(EVENT_INFO *eip)
{
    ESCAN_RESULT *erp=(ESCAN_RESULT *)eip->data;
    int ret = eip->chan==SDPCM_CHAN_EVT && eip->event_type==WLC_E_ESCAN_RESULT;
    char temps[30];
    
    if (ret)
    {
        if (erp->eventh.status == 0)
        {
            printf("Scan complete\n");
            return(-1);
        }
        else
        {
            mac_addr_str(temps, erp->info.bssid);
            printf("%s '", temps);
            disp_ssid(&erp->info.ssid_len);
            printf("' chan %d\n", SWAP16(erp->info.channel));
        }
    }
    return(ret);
}

// Display SSID string that is prefixed with length byte
void disp_ssid(uint8_t *data)
{
    int i=*data++;

    if (i == 0 || *data == 0)
        printf("[hidden]");
    else if (i <= SSID_MAXLEN)
    {
        while (i-- > 0)
            putchar(*data++);
    }
    else
        printf("[invalid length %u]", i);
}

// EOF

