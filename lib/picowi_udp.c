// PicoWi UDP socket interface, see http://iosoft.blog/picowi for details
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
#include "picowi_event.h"
#include "picowi_ip.h"
#include "picowi_net.h"
#include "picowi_udp.h"

extern int display_mode;

extern NET_SOCKET net_sockets[NUM_NET_SOCKETS];

extern BYTE txbuff[TXDATA_LEN]; // Transmit buffer
extern int display_mode;        // Display mode
extern MACADDR my_mac;          // My MAC address
extern IPADDR my_ip;            // My IP address, and DNS server

// Handler for incoming UDP datagram (not DHCP)
int udp_event_handler(EVENT_INFO *eip)
{
    IPHDR *ip = (IPHDR *)&eip->data[sizeof(ETHERHDR)];
    UDPHDR *udp = (UDPHDR *)&eip->data[sizeof(ETHERHDR) + sizeof(IPHDR)];
    NET_SOCKET *usp;
    int sock;

    if (eip->chan == SDPCM_CHAN_DATA &&
        ip->pcol == PUDP &&
        ip_check_frame(eip->data, eip->dlen) &&
        eip->dlen >= sizeof(ETHERHDR) + sizeof(IPHDR) + sizeof(UDPHDR))
    {
        if (display_mode & DISP_UDP)
        {
            printf("Rx ");
            udp_print_hdr(eip->data, eip->dlen);
        }
        if ((sock = udp_sock_match(ip->sip, htons(udp->sport), htons(udp->dport))) >= 0)
        {
            printf("Rx SOCK %d\n", sock);
            usp = &net_sockets[sock];
            return (udp_sock_rx(usp, eip->data, eip->dlen));
        }
    }
    return (0);
}

// Find next unused UDP socket, return index number, -ve if none
int udp_sock_unused(void)
{
    for (int i=0; i<NUM_NET_SOCKETS; i++)
    {
        if (net_sockets[i].loc_port == 0 && net_sockets[i].rem_port == 0)
            return (i);
    }
    return (-1);
}

// Set parameters in a UDP socket
void udp_sock_set(int sock, net_handler_t handler, IPADDR remip, WORD remport, WORD locport)
{
    NET_SOCKET *usp = &net_sockets[sock];

    usp = &net_sockets[sock];
    IP_CPY(usp->rem_ip, remip);
    usp->loc_port = locport;
    usp->rem_port = remport;
    usp->sock_handler = handler;
}

// Initialise a UDP socket
NET_SOCKET *udp_sock_init(net_handler_t handler, IPADDR remip, WORD remport, WORD locport)
{
    NET_SOCKET *usp = 0;
    int sock = udp_sock_unused();

    if (sock >= 0)
    {
        usp = &net_sockets[sock];
        udp_sock_set(sock, handler, remip, remport, locport);
    }
    return (usp);
}

// Find matching socket for incoming UDP datagram, return -ve if none
int udp_sock_match(IPADDR remip, WORD remport, WORD locport)
{
    NET_SOCKET *usp = 0;
    int i;

    for (i=0; i<NUM_NET_SOCKETS; i++)
    {
        usp = &net_sockets[i];
        if (locport == usp->loc_port)
            return (i);
    }
    return (-1);
}

// Receive incoming UDP datagram
int udp_sock_rx(NET_SOCKET *usp, BYTE *data, int len)
{
    ETHERHDR *ehp = (ETHERHDR *)data;
    IPHDR *ip = (IPHDR *)&data[sizeof(ETHERHDR)];
    UDPHDR *udp = (UDPHDR *)&data[sizeof(ETHERHDR) + sizeof(IPHDR)];

    MAC_CPY(usp->rem_mac, ehp->srce);
    IP_CPY(usp->rem_ip, ip->sip);
    usp->rem_port = htons(udp->sport);
    usp->rxdata = data;
    usp->rxlen = len;
    if (usp->sock_handler)
        return (usp->sock_handler(usp));
    return (0);
}

// Send a UDP datagram
int udp_tx(MACADDR mac, IPADDR dip, WORD remport, WORD locport, void *data, int dlen)
{
    int len = ip_add_eth(txbuff, mac, my_mac, PCOL_IP);

    len += ip_add_hdr(&txbuff[len], dip, PUDP, sizeof(UDPHDR) + dlen);
    len += udp_add_hdr_data(&txbuff[len], locport, remport, data, dlen);
    if (display_mode & DISP_UDP)
    {
        printf("Tx ");
        udp_print_hdr(txbuff, len+sizeof(UDPHDR));
    }
    return (ip_tx_eth(txbuff, len));
}

// Add UDP header to buffer, plus optional data, return byte count
int udp_add_hdr_data(BYTE *buff, WORD sport, WORD dport, void *data, int dlen)
{
    UDPHDR *udp = (UDPHDR *)buff;
    IPHDR *ip = (IPHDR *)(buff - sizeof(IPHDR));
    WORD len = sizeof(UDPHDR), check;
    PHDR ph;

    udp->sport = htons(sport);
    udp->dport = htons(dport);
    udp->len = htons(sizeof(UDPHDR) + dlen);
    udp->check = 0;
    len += ip_add_data(&buff[sizeof(UDPHDR)], data, dlen);
    check = add_csum(0, udp, len);
    IP_CPY(ph.sip, ip->sip);
    IP_CPY(ph.dip, ip->dip);
    ph.z = 0;
    ph.pcol = PUDP;
    ph.len = udp->len;
    udp->check = 0xffff ^ add_csum(check, &ph, sizeof(PHDR));
    return (len);
}

// Display UDP
void udp_print_hdr(BYTE *data, int dlen)
{
    IPHDR *ip = (IPHDR *)&data[sizeof(ETHERHDR)];
    UDPHDR *udp = (UDPHDR *)&data[sizeof(ETHERHDR) + sizeof(IPHDR)];

    printf("UDP ");
    print_ip_addr(ip->sip);
    printf(":%u->", htons(udp->sport));
    print_ip_addr(ip->dip);
    printf(":%u len %d\n", htons(udp->dport), dlen - sizeof(ETHERHDR) - sizeof(IPHDR) - sizeof(UDPHDR));
}

// EOF
