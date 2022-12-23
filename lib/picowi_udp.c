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

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "picowi_pico.h"
#include "picowi_ioctl.h"
#include "picowi_event.h"
#include "picowi_ip.h"
#include "picowi_udp.h"

extern int display_mode;

#define NUM_UDP_SOCKETS 5
UDP_SOCKET udp_sockets[NUM_UDP_SOCKETS];

extern BYTE txbuff[TXDATA_LEN];	// Transmit buffer
extern int display_mode;		// Display mode
extern MACADDR my_mac;			// My MAC address
extern IPADDR my_ip;			// My IP address, and DNS server

// Handler for incoming UDP datagram (not DHCP)
int udp_event_handler(EVENT_INFO *eip)
{
	IPHDR *ip = (IPHDR *)&eip->data[sizeof(ETHERHDR)];
	UDPHDR *udp = (UDPHDR *)&eip->data[sizeof(ETHERHDR) + sizeof(IPHDR)];
	UDP_SOCKET *usp;
    
	if (eip->chan == SDPCM_CHAN_DATA &&
	    ip->pcol == PUDP &&
	    ip_check_frame(eip->data, eip->dlen) &&
	    eip->dlen > sizeof(ETHERHDR) + sizeof(IPHDR) + sizeof(UDPHDR))
	{
		if (display_mode & DISP_UDP)
		{
			printf("Rx ");
			udp_print_hdr(eip->data, eip->dlen);
		}
		if ((usp = udp_sock_match(ip->sip, htons(udp->dport), htons(udp->sport))) != 0)
			return (udp_sock_rx(usp, eip->data, eip->dlen));
	}
	return (0);
}

// Initialise a UDP socket
UDP_SOCKET *udp_sock_init(udp_handler_t handler, IPADDR remip, WORD remport, WORD locport)
{
	UDP_SOCKET *usp = 0;
	int i;
	
	for (i = 0; i<NUM_UDP_SOCKETS; i++)
	{
		usp = &udp_sockets[i];
		if (usp->locport == 0 && usp->remport == 0)
		{
			IP_CPY(usp->remip, remip);
			usp->locport = locport;
			usp->remport = remport;
    		usp->handler = handler;
		}
	}
	return (0);
}

// Find matching socket for incoming UDP datagram
UDP_SOCKET *udp_sock_match(IPADDR remip, WORD remport, WORD locport)
{
	UDP_SOCKET *usp = 0;
	int i;
	
	for (i=0; i<NUM_UDP_SOCKETS; i++)
	{
		usp = &udp_sockets[i];
		if (locport == usp->locport && 
			(IP_IS_ZERO(usp->remip) || 
			 (IP_CMP(remip, usp->remip) && remport == usp->remport)))
			return (usp);
	} 
	return (0);
}

// Receive incoming UDP datagram
int udp_sock_rx(UDP_SOCKET *usp, BYTE *data, int len)
{
	UDPHDR *udp = (UDPHDR *)&data[sizeof(ETHERHDR) + sizeof(IPHDR)];
	
	usp->remport = udp->sport;
	usp->data = data;
	usp->dlen = len;
	if (usp->handler)
		return(usp->handler(usp));
	return (0);
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
