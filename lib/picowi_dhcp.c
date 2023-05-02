// PicoWi DHCP client, see http://iosoft.blog/picowi for details
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
#include "picowi_ioctl.h"
#include "picowi_ip.h"
#include "picowi_net.h"
#include "picowi_udp.h"
#include "picowi_dhcp.h"

int dhcp_state;                 // State variable to track DHCP progress
MACADDR host_mac;               // MAC address of host
IPADDR router_ip, dns_ip;       // Router & DNS server IP addresses
IPADDR subnet_mask, offered_ip; // Subnet mask & address being offered
int dhcp_complete;              // Flag to show DHCP complete
char *dhcp_typestrs[] = {DHCP_TYPESTRS};

extern BYTE txbuff[TXDATA_LEN]; // Transmit buffer
extern int display_mode;        // Display mode
extern MACADDR my_mac;          // My MAC address
extern IPADDR my_ip, bcast_ip;  // My IP address, and broadcast
extern MACADDR bcast_mac;       // Broadcast MAC address

// Cookie to identify DHCP message
const BYTE dhcp_cookie[DHCP_COOKIE_LEN] = {99, 130, 83, 99};

// DHCP discover options
DHCP_MSG_OPTS dhcp_disco_opts = 
   {53, 1, 1,               // Msg len 1 type 1: discover
    55, 4, {1, 3, 6, 15},   // Param len 4: mask, router, DNS, name
    255};                   // End

// DHCP request options
DHCP_MSG_OPTS dhcp_req_opts = 
   {53, 1, 3,               // Msg len 1 type 3: request
    50, 4, {0, 0, 0, 0},    // Address len 4 (copied from offer)
    255};                   // End

// Handler for incoming DHCP frame
int dhcp_event_handler(EVENT_INFO *eip)
{
    IPHDR *ip = (IPHDR *)&eip->data[sizeof(ETHERHDR)];
    UDPHDR *udp = (UDPHDR *)&eip->data[sizeof(ETHERHDR)+sizeof(IPHDR)];
    
    if (eip->chan == SDPCM_CHAN_DATA &&
        ip->pcol == PUDP &&
        ip_check_frame(eip->data, eip->dlen) &&
        eip->dlen > sizeof(ETHERHDR)+sizeof(IPHDR)+sizeof(UDPHDR) &&
        udp->dport == htons(DHCP_CLIENT_PORT))
    {
        if (display_mode & DISP_UDP)
        {
            printf("Rx ");
            udp_print_hdr(eip->data, eip->dlen);
        }
        return(dhcp_rx(eip->data, eip->dlen));
    }
    return(0);
}

// Add DHCP header & data to buffer
int dhcp_add_hdr_data(BYTE *buff, BYTE opcode, void *data, int dlen)
{
    DHCPHDR *dhcp=(DHCPHDR *)buff;
    int len=sizeof(DHCPHDR);
    static DWORD trans=1;

    memset(dhcp, 0, len);
    dhcp->opcode = opcode;
    dhcp->htype = 1;
    dhcp->hlen = MACLEN;
    dhcp->trans = htonl(trans++);
    MAC_CPY(dhcp->chaddr, my_mac);
    memcpy(dhcp->cookie, dhcp_cookie, sizeof(dhcp_cookie));
    len += ip_add_data(&buff[len], data, dlen);
    return(len);
}

// Send an DHCP datagram
int dhcp_tx(MACADDR mac, IPADDR dip, BYTE opcode, void *data, int dlen)
{
    int len;
    DHCPHDR *dhcp;

    len = ip_add_eth(txbuff, mac, my_mac, PCOL_IP);
    dlen = dhcp_add_hdr_data(&txbuff[len+sizeof(IPHDR)+sizeof(UDPHDR)], opcode, data, dlen);
    len += ip_add_hdr(&txbuff[len], dip, PUDP, sizeof(UDPHDR)+dlen);
    len += udp_add_hdr_data(&txbuff[len], DHCP_CLIENT_PORT, DHCP_SERVER_PORT, 0, dlen);
    len += dlen;
    if (display_mode & DISP_UDP)
    {
        printf("Tx ");
        udp_print_hdr(txbuff, len);
    }
    if (display_mode & DISP_DHCP)
    {
        dhcp = (DHCPHDR *)&txbuff[sizeof(ETHERHDR)+sizeof(IPHDR)+sizeof(UDPHDR)];
        printf("Tx DHCP %s\n", dhcp_type_str(dhcp_msg_type(dhcp, len)));
    }
    return(ip_tx_eth(txbuff, len));
}

// Poll DHCP state machine
void dhcp_poll(void)
{
    static uint32_t dhcp_ticks=0;
    
    if (dhcp_state == 0 ||              // Send DHCP Discover
       (dhcp_state != DHCPT_ACK && ustimeout(&dhcp_ticks, DHCP_TIMEOUT)))
    {
        ustimeout(&dhcp_ticks, 0);
        IP_ZERO(my_ip);
        dhcp_tx(bcast_mac, bcast_ip, DHCP_REQUEST, 
                   &dhcp_disco_opts, sizeof(dhcp_disco_opts));
        dhcp_state = DHCPT_DISCOVER;
    }
    else if (dhcp_state == DHCPT_OFFER) // Received Offer, send Request
    {
        ustimeout(&dhcp_ticks, 0);
        IP_CPY(dhcp_req_opts.data, offered_ip);
        dhcp_tx(host_mac, bcast_ip, DHCP_REQUEST, 
                   &dhcp_req_opts, sizeof(dhcp_req_opts));
        dhcp_state = DHCPT_REQUEST;
    }
}

// Receive DHCP response
int dhcp_rx(BYTE *data, int len)
{
    ETHERHDR *ehp=(ETHERHDR *)data;
    DHCPHDR *dhcp=(DHCPHDR *)&data[sizeof(ETHERHDR)+sizeof(IPHDR)+sizeof(UDPHDR)];
    BYTE typ;
    char temps[30];

    if (len>=sizeof(ETHERHDR)+sizeof(IPHDR)+sizeof(UDPHDR)+sizeof(DHCPHDR) &&
        MAC_CMP(dhcp->chaddr, my_mac))
    {
        MAC_CPY(host_mac, ehp->srce);
        typ = dhcp_msg_type(dhcp, len);
        display(DISP_DHCP, "Rx DHCP %s %s", 
                dhcp_type_str(typ), ip_addr_str(temps, dhcp->yiaddr));
        if (dhcp_state==DHCPT_DISCOVER && typ==DHCPT_OFFER)
        {                           // Sent DHCP Discover, received Offer
            IP_CPY(offered_ip, dhcp->yiaddr);
            dhcp_state = DHCPT_OFFER;
        }
        else if (dhcp_state==DHCPT_REQUEST && typ==DHCPT_ACK)
        {                          // Sent Request, received Ack
            IP_CPY(my_ip, dhcp->yiaddr);
            if (dhcp_get_opt_data(dhcp, len, DHCP_OPT_SUBNET, subnet_mask, IPLEN) == IPLEN)
                display(DISP_DHCP, " mask %s", ip_addr_str(temps, subnet_mask));
            if (dhcp_get_opt_data(dhcp, len, DHCP_OPT_ROUTER, router_ip, IPLEN) == IPLEN)
                display(DISP_DHCP, " router %s", ip_addr_str(temps, router_ip));
            if (dhcp_get_opt_data(dhcp, len, DHCP_OPT_DNS, dns_ip, IPLEN) == IPLEN)
                    display(DISP_DHCP, " DNS %s", ip_addr_str(temps, dns_ip));
            dhcp_state = DHCPT_ACK;
            dhcp_complete = 1;
        }
        display(DISP_DHCP, "\n");
        return(1);
    }
    return(0);
}

// Get the type of DCHP response, 0 if unknown
BYTE dhcp_msg_type(DHCPHDR *dhcp, int len)
{
    BYTE b;

    return(dhcp_get_opt_data(dhcp, len, DHCP_OPT_MSGTYPE, &b, 1)==1 ? b : 0);
}

// Get data for DHCP option, return length
int dhcp_get_opt_data(DHCPHDR *dhcp, int len, BYTE opt, BYTE *data, int maxlen)
{
    BYTE n=0, *p=(BYTE *)dhcp;

    p = dhcp_get_opt(p+sizeof(DHCPHDR), len-sizeof(DHCPHDR), opt);
    if (p)
    {
        n = *++p;
        n = (n > maxlen) ? maxlen : n;
        if (n > 0)
            memcpy(data, p+1, n);
    }
    return(n);
}

// Get pointer to option in DHCP data, zero if not found
BYTE *dhcp_get_opt(BYTE *buff, int len, BYTE opt)
{
    BYTE *p=buff;

    while (p && *p!=opt)
    {
        if (*p==DHCP_OPT_END || p>=buff+len)
            p = 0;
        else
            p += *(p+1) + 2;
    }
    return(p);
}

// Display type of DHCP request
char *dhcp_type_str(BYTE typ)
{
    return(typ>0 && typ<=DHCPT_INFORM ? dhcp_typestrs[typ-1] : "?");
}

/* EOF */
