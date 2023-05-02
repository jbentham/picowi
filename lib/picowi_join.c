// PicoWi WiFi join functions, see http://iosoft.blog/picowi for details
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
#include "picowi_wifi.h"
#include "picowi_pico.h"
#include "picowi_init.h"
#include "picowi_ioctl.h"
#include "picowi_regs.h"
#include "picowi_event.h"
#include "picowi_evtnum.h"
#include "picowi_join.h"

const char country_data[20] = "XX\x00\x00\xFF\xFF\xFF\xFFXX";
const uint8_t mcast_addr[10*6] = {0x01,0x00,0x00,0x00,0x01,0x00,0x5E,0x00,0x00,0xFB};
const EVT_STR join_evts[]={EVT(WLC_E_JOIN), EVT(WLC_E_ASSOC), EVT(WLC_E_REASSOC), 
    EVT(WLC_E_ASSOC_REQ_IE), EVT(WLC_E_ASSOC_RESP_IE), EVT(WLC_E_SET_SSID),
    EVT(WLC_E_LINK), EVT(WLC_E_AUTH), EVT(WLC_E_PSK_SUP),  EVT(WLC_E_EAPOL_MSG),
    EVT(WLC_E_DISASSOC_IND), EVT(-1)};

extern EVENT_INFO event_info;

// Start to join a network
bool join_start(char *ssid, char *passwd)
{
    uint32_t val;
    
    // Clear pullups
    wifi_reg_write(SD_FUNC_BAK, BAK_PULLUP_REG, 0xf, 1);
    wifi_reg_write(SD_FUNC_BAK, BAK_PULLUP_REG, 0, 1);
    val = wifi_reg_read(SD_FUNC_BAK, BAK_PULLUP_REG, 1);
    // Clear data unavail error
    val = wifi_reg_read(SD_FUNC_BUS, BUS_INTEN_REG, 2);
    if (val & 1)
        wifi_reg_write(SD_FUNC_BUS, BUS_INTEN_REG, val, 2);
    // Set sleep KSO (should poll to check for success)
    wifi_reg_write(SD_FUNC_BAK, BAK_SLEEP_CSR_REG, 1, 1);
    wifi_reg_write(SD_FUNC_BAK, BAK_SLEEP_CSR_REG, 1, 1);
    val = wifi_reg_read(SD_FUNC_BAK, BAK_SLEEP_CSR_REG, 1);
    // Set country
    ioctl_set_data2("country", 8, IOCTL_WAIT, (void *)country_data, sizeof(country_data));
    // Select antenna 
    ioctl_wr_int32(WLC_SET_ANTDIV, IOCTL_WAIT, 0x00);
    // Data aggregation
    ioctl_set_uint32("bus:txglom", IOCTL_WAIT, 0x00);
    ioctl_set_uint32("apsta", IOCTL_WAIT, 0x01);
    ioctl_set_uint32("ampdu_ba_wsize", IOCTL_WAIT, 0x08);
    ioctl_set_uint32("ampdu_mpdu", IOCTL_WAIT, 0x04);
    ioctl_set_uint32("ampdu_rx_factor", IOCTL_WAIT, 0x00);
    usdelay(150000);
    // Enable events for reporting the join process
    events_enable(join_evts);
    usdelay(50000);
    // Enable multicast
    ioctl_set_data2("mcast_list", 11, IOCTL_WAIT, (void *)mcast_addr, sizeof(mcast_addr));
    usdelay(50000);
    // Register SSID and password with polling function
    join_state_poll(ssid, passwd);
    return(true);
}

// Stop trying to join network
// (Set WiFi interface 'down', ignore IOCTL response)
bool join_stop(void)
{
    ioctl_wr_data(WLC_DOWN, 50, 0, 0); 
    return(true);
}

// Retry joining a network
bool join_restart(char *ssid, char *passwd)
{
    uint32_t n;
    uint8_t data[100];
    bool ret=0;
    
    // Start up the interface
    ioctl_wr_data(WLC_UP, 50, 0, 0);
    ret = ioctl_wr_int32(WLC_SET_GMODE, IOCTL_WAIT, 0x01) > 0 &&
          ioctl_wr_int32(WLC_SET_BAND, IOCTL_WAIT, 0x00) > 0 &&
          ioctl_set_uint32("pm2_sleep_ret", IOCTL_WAIT, 0xc8) > 0 &&
          ioctl_set_uint32("bcn_li_bcn", IOCTL_WAIT, 0x01) > 0 &&
          ioctl_set_uint32("bcn_li_dtim", IOCTL_WAIT, 0x01) > 0 &&
          ioctl_set_uint32("assoc_listen", IOCTL_WAIT, 0x0a) > 0;
#if POWERSAVE    
    // Enable power saving
    init_powersave();
#endif    
    // Wireless security
    ret = ret && ioctl_wr_int32(WLC_SET_INFRA, 50, 1) > 0 &&
        ioctl_wr_int32(WLC_SET_AUTH, IOCTL_WAIT, 0) > 0;
#if SECURITY    
    ret = ret && ioctl_wr_int32(WLC_SET_WSEC, IOCTL_WAIT, SECURITY==2 ? 6 : 2) > 0 &&
        ioctl_set_data("bsscfg:sup_wpa", IOCTL_WAIT, (void *)"\x00\x00\x00\x00\x01\x00\x00\x00", 8) > 0 &&
        ioctl_set_data("bsscfg:sup_wpa2_eapver", IOCTL_WAIT, (void *)"\x00\x00\x00\x00\xFF\xFF\xFF\xFF", 8) > 0 &&
        ioctl_set_data("bsscfg:sup_wpa_tmo", IOCTL_WAIT, (void *)"\x00\x00\x00\x00\xC4\x09\x00\x00", 8) > 0;
    usdelay(2000);
    n = strlen(passwd);
    *(uint32_t *)data = n + 0x10000;
    strcpy((char *)&data[4], passwd);
    ret = ret && ioctl_wr_data(WLC_SET_WSEC_PMK, IOCTL_WAIT, data, n+4) >0 &&
        ioctl_wr_int32(WLC_SET_INFRA, IOCTL_WAIT, 0x01) > 0 &&
        ioctl_wr_int32(WLC_SET_AUTH, IOCTL_WAIT, 0x00) > 0 &&
        ioctl_wr_int32(WLC_SET_WPA_AUTH, IOCTL_WAIT, SECURITY==2 ? 0x80 : 4) > 0;
#else
    ret = ret && ioctl_wr_int32(WLC_SET_WSEC, IOCTL_WAIT, 0) > 0 &&
        ioctl_wr_int32(WLC_SET_WPA_AUTH, IOCTL_WAIT, 0) > 0;
#endif      
    n = strlen(ssid);
    *(uint32_t *)data = n;
    strcpy((char *)&data[4], ssid);
    ret = ret && ioctl_wr_data(WLC_SET_SSID, IOCTL_WAIT, data, n+4) > 0;
    ioctl_err_display(ret);
    return(ret);
}

// Handler for join events (link & auth changes)
int join_event_handler(EVENT_INFO *eip)
{
    int ret = 1;
    uint16_t news;
    
    if (eip->chan == SDPCM_CHAN_EVT)
    {
        news = eip->link;
        if (eip->event_type==WLC_E_LINK && eip->status==0)
            news = eip->flags&1 ? news|LINK_UP_OK : news&~LINK_UP_OK;
        else if (eip->event_type == WLC_E_PSK_SUP)
            news = eip->status==6 ? news|LINK_AUTH_OK : news&~LINK_AUTH_OK;
        else if (eip->event_type == WLC_E_DISASSOC_IND)
            news = LINK_FAIL;
        else
            ret = 0;
        eip->link = news;
    }
    else
        ret = 0;
    return(ret);
}

// Poll the network joining state machine
void join_state_poll(char *ssid, char *passwd)
{
    EVENT_INFO *eip = &event_info;
    static uint32_t join_ticks;
    static char *s = "", *p = "";

    if (ssid)
        s = ssid;
    if (passwd)
        p = passwd;
    if (eip->join == JOIN_IDLE)
    {
        display(DISP_JOIN, "Joining network %s\n", s);
        eip->link = 0;
        eip->join = JOIN_JOINING;
        ustimeout(&join_ticks, 0);
        join_restart(s, p);
    }
    else if (eip->join == JOIN_JOINING)
    {
        if (link_check() > 0)
        {
            display(DISP_JOIN, "Joined network\n");
            eip->join = JOIN_OK;
        }
        else if (link_check()<0 || ustimeout(&join_ticks, JOIN_TRY_USEC))
        {
            display(DISP_JOIN, "Failed to join network\n");
            ustimeout(&join_ticks, 0);
            join_stop();
            eip->join = JOIN_FAIL;
        }
    }
    else if (eip->join == JOIN_OK)
    {
        ustimeout(&join_ticks, 0);
        if (link_check() < 1)
        {
            display(DISP_JOIN, "Leaving network\n");
            join_stop();
            eip->join = JOIN_FAIL;
        }
    }
    else  // JOIN_FAIL
    {
        if (ustimeout(&join_ticks, JOIN_RETRY_USEC))
            eip->join = JOIN_IDLE;
    }
}

// Return 1 if joined to network, -1 if error joining
int link_check(void)
{
    EVENT_INFO *eip=&event_info;
    return((eip->link&LINK_OK) == LINK_OK ? 1 : 
            eip->link == LINK_FAIL ? -1 : 0);
}

#if 0
// Handler for incoming network data
int ip_event_handler(EVENT_INFO *eip)
{
    int ret=0;
    
    if (eip->chan==SDPCM_CHAN_DATA && eip->dlen>0)
    {
        rx_frame(eip->data, eip->dlen);
        ret = 1;
    }
    return(ret);
}
#endif
// EOF
