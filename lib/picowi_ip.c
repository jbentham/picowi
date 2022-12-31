// PicoWi IP functions, see http://iosoft.blog/picowi for details
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

extern int display_mode;
IPADDR my_ip, bcast_ip=IPADDR_VAL(255,255,255,255);
IPADDR zero_ip=IPADDR_VAL(0, 0, 0, 0);
MACADDR bcast_mac={0xff,0xff,0xff,0xff,0xff,0xff};
extern MACADDR my_mac;
BYTE txbuff[TXDATA_LEN];

#define NUM_ARP_ENTRIES 10
ARP_ENTRY arp_entries[NUM_ARP_ENTRIES];
int arp_idx;
uint32_t ping_tx_time, ping_rx_time;


// Initialise the IP stack, using static address if provided
//int ip_init(BYTE *ip)
int ip_init(IPADDR addr)
{
    IP_CPY(my_ip, addr);
    return(1);
}

// Set my MAC address
void ip_set_mac(BYTE *mac)
{
    MAC_CPY(my_mac, mac);
}

// Send transmit data
int ip_tx_eth(BYTE *buff, int len)
{
    if (display_mode & DISP_ETH)
        ip_print_eth(buff);
    return(event_net_tx(buff, len));
}

// Add Ethernet header to buffer, return byte count
int ip_add_eth(BYTE *buff, MACADDR dmac, MACADDR smac, WORD pcol)
{
    ETHERHDR *ehp = (ETHERHDR *)buff;

    MAC_CPY(ehp->dest, dmac);
    MAC_CPY(ehp->srce, smac);
    ehp->ptype = htons(pcol);
    return(sizeof(ETHERHDR));
}

// Display MAC addresses in Ethernet frame
void ip_print_eth(BYTE *buff)
{
    ETHERHDR *ehp = (ETHERHDR *)buff;

    print_mac_addr(ehp->srce);
    printf("->");
    print_mac_addr(ehp->dest);
    printf("\n");
}

/*** ARP ***/

// Handler for incoming ARP frame
int arp_event_handler(EVENT_INFO *eip)
{
    ETHERHDR *ehp=(ETHERHDR *)eip->data;

    if (eip->chan == SDPCM_CHAN_DATA &&
        eip->dlen >= sizeof(ETHERHDR)+sizeof(ARPKT) &&
        htons(ehp->ptype) == PCOL_ARP &&
        (MAC_IS_BCAST(ehp->dest) ||
         MAC_CMP(ehp->dest, my_mac)))
    {
        return(ip_rx_arp(eip->data, eip->dlen));
    }
    return(0);
}

// Receive incoming ARP data
int ip_rx_arp(BYTE *data, int dlen)
{
    ETHERHDR *ehp=(ETHERHDR *)data;
    ARPKT *arp = (ARPKT *)&data[sizeof(ETHERHDR)];
    WORD op = htons(arp->op);

    if (display_mode & DISP_ETH)
        ip_print_eth(data);
    if IP_CMP(arp->dip, my_ip)
    {
        if (display_mode & DISP_ARP)
            ip_print_arp(arp);
        if (op == ARPREQ)
            ip_tx_arp(ehp->srce, arp->sip, ARPRESP);
        else if (op == ARPRESP)
            ip_save_arp(arp->smac, arp->sip);
        return(1);
    }
    return(0);
}

// Create an ARP frame
int ip_make_arp(BYTE *buff, MACADDR mac, IPADDR addr, WORD op)
{
    int n = ip_add_eth(buff, op==ARPREQ ? bcast_mac : mac, my_mac, PCOL_ARP);
    ARPKT *arp = (ARPKT *)&buff[n];

    MAC_CPY(arp->smac, my_mac);
    MAC_CPY(arp->dmac, op==ARPREQ ? bcast_mac : mac);
    arp->hrd = htons(HTYPE);
    arp->pro = htons(ARPPRO);
    arp->hln = MACLEN;
    arp->pln = sizeof(DWORD);
    arp->op  = htons(op);
    IP_CPY(arp->dip, addr);
    IP_CPY(arp->sip, my_ip);
    if (display_mode & DISP_ARP)
        ip_print_arp(arp);
    return(n + sizeof(ARPKT));
}

// Transmit an ARP frame
int ip_tx_arp(MACADDR mac, IPADDR addr, WORD op)
{
    int n = ip_make_arp(txbuff, mac, addr, op);
    
    return(ip_tx_eth(txbuff, n));
 }
 
// Save ARP result
void ip_save_arp(MACADDR mac, IPADDR addr)
{
    MAC_CPY(arp_entries[arp_idx].mac, mac);
    IP_CPY(arp_entries[arp_idx].ipaddr, addr);
    arp_idx = (arp_idx+1) % NUM_ARP_ENTRIES;
}

// Find saved ARP response
bool ip_find_arp(IPADDR addr, MACADDR mac)
{
    int n=0, i=arp_idx;
    bool ok=0;
    
    do
    {
        i = i == 0 ? NUM_ARP_ENTRIES-1 : i-1;
        ok = (IP_CMP(addr, arp_entries[i].ipaddr));
    } while (!ok && ++n<NUM_ARP_ENTRIES);
    if (ok)
        MAC_CPY(mac, arp_entries[i].mac);
    return(ok);
}

// Display ARP
void ip_print_arp(ARPKT *arp)
{
    WORD op=htons(arp->op);

    print_ip_addr(arp->sip);
    printf("->");
    print_ip_addr(arp->dip);
    printf(" ARP %s\n", op==ARPREQ ? "request" : op==ARPRESP ? "response" : "");
}

/*** IP ***/

// Check if IP frame
int ip_check_frame(BYTE *data, int dlen)
{
    ETHERHDR *ehp=(ETHERHDR *)data;
    IPHDR *ip = (IPHDR *)&data[sizeof(ETHERHDR)];

    return (dlen >= sizeof(ETHERHDR)+sizeof(ARPKT) &&
        (MAC_IS_BCAST(ehp->dest) || MAC_CMP(ehp->dest, my_mac)) &&
        htons(ehp->ptype) == PCOL_IP &&
        (IP_IS_BCAST(ip->dip) || IP_CMP(ip->dip, my_ip) || IP_IS_ZERO(my_ip)) &&
        sizeof(ETHERHDR) + htons(ip->len) <= dlen);
}

// Add IP header to buffer, return length
int ip_add_hdr(BYTE *buff, IPADDR dip, BYTE pcol, WORD dlen)
{
    static WORD ident=1;
    IPHDR *ip=(IPHDR *)buff;

    ip->ident = htons(ident++);
    ip->frags = 0;
    ip->vhl = 0x40+(sizeof(IPHDR)>>2);
    ip->service = 0;
    ip->ttl = 100;
    ip->pcol = pcol;
    IP_CPY(ip->sip, my_ip);
    IP_CPY(ip->dip, dip);
    ip->len = htons(dlen + sizeof(IPHDR));
    ip->check = 0;
    ip->check = 0xffff ^ add_csum(0, ip, sizeof(IPHDR));
    return(sizeof(IPHDR));
}

/*** ICMP ***/

// Handler for incoming ICMP frame
int icmp_event_handler(EVENT_INFO *eip)
{
    IPHDR *ip = (IPHDR *)&eip->data[sizeof(ETHERHDR)];
    
    if (eip->chan == SDPCM_CHAN_DATA &&
        ip->pcol == PICMP &&
        ip_check_frame(eip->data, eip->dlen) &&
        IP_CMP(ip->dip, my_ip) &&
        eip->dlen > sizeof(ETHERHDR)+sizeof(IPHDR)+sizeof(ICMPHDR))
    {
        return(ip_rx_icmp(eip->data, eip->dlen));
    }
    return(0);
}

// Receive incoming ICMP data
int ip_rx_icmp(BYTE *data, int dlen)
{
    ETHERHDR *ehp=(ETHERHDR *)data;
    IPHDR *ip = (IPHDR *)&data[sizeof(ETHERHDR)];
    ICMPHDR *icmp = (ICMPHDR *)&data[sizeof(ETHERHDR)+sizeof(IPHDR)];
    int n;

    if (display_mode & DISP_ICMP)
        ip_print_icmp(ip);
    if (icmp->type == ICREQ)
    {
        ip_add_eth(data, ehp->srce, my_mac, PCOL_IP);
        IP_CPY(ip->dip, ip->sip);
        IP_CPY(ip->sip, my_ip);
        icmp->check = add_csum(icmp->check, &icmp->type, 1);
        icmp->type = ICREP;
        n = htons(ip->len);
        return(ip_tx_eth(data, sizeof(ETHERHDR)+n+sizeof(ICMPHDR)));
    }
    else if (icmp->type == ICREP)
    {
        ping_rx_time = ustime();
    }
    return(0);
}

// Add ICMP header to buffer, return byte count
int ip_add_icmp(BYTE *buff, BYTE type, BYTE code, void *data, WORD dlen)
{
    ICMPHDR *icmp=(ICMPHDR *)buff;
    WORD len=sizeof(ICMPHDR);
    static WORD seq=1;

    icmp->type = type;
    icmp->code = code;
    icmp->seq = htons(seq++);
    icmp->ident = icmp->check = 0;
    len += ip_add_data(&buff[len], data, dlen);
    icmp->check = 0xffff ^ add_csum(0, icmp, len);
    return(len);
}

// Add data to buffer, return length
int ip_add_data(BYTE *buff, void *data, int len)
{
    if (len>0 && data)
        memcpy(buff, data, len);
    return(len);
}

// Create ICMP request
int ip_make_icmp(BYTE *buff, MACADDR mac, IPADDR dip, BYTE type, BYTE code, BYTE *data, int dlen)
{
    int n = ip_add_eth(buff, mac, my_mac, PCOL_IP);
    
    n += ip_add_hdr(&buff[n], dip, PICMP, sizeof(ICMPHDR)+dlen);
    n += ip_add_icmp(&buff[n], type, code, data, dlen);
    return(n);
}

// Transmit ICMP request
int ip_tx_icmp(MACADDR mac, IPADDR dip, BYTE type, BYTE code, BYTE *data, int dlen)
{
    int n=ip_make_icmp(txbuff, mac, dip, type, code, data, dlen);
    
    if (display_mode & DISP_ICMP)
        ip_print_icmp((IPHDR *)&txbuff[sizeof(ETHERHDR)]);
    ping_tx_time = ustime();
    return(ip_tx_eth(txbuff, n));
}

// Display ICMP
void ip_print_icmp(IPHDR *ip)
{
    ICMPHDR *icmp = (ICMPHDR *)((BYTE *)ip + sizeof(IPHDR));
    
    print_ip_addrs(ip);
    printf(" ICMP %s\n", icmp->type==ICREQ     ? "request" : 
                        icmp->type==ICREP     ? "response" : 
                        icmp->type==ICUNREACH ? "dest unreachable" : 
                        icmp->type==ICQUENCH  ? "srce quench" : "?");
}

/*** Utilities ***/

// Display MAC address
void print_mac_addr(MACADDR mac)
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
}

// Display IP address
void print_ip_addr(IPADDR a)
{
    printf("%u.%u.%u.%u", a[0],a[1],a[2],a[3]);
}

// Display IP addresses in IP header
void print_ip_addrs(IPHDR *ip)
{
    print_ip_addr(ip->sip);
    printf("->");
    print_ip_addr(ip->dip);
}

// Convert IP address to string
char *ip_addr_str(char *s, IPADDR a)
{
    sprintf(s, "%u.%u.%u.%u", a[0],a[1],a[2],a[3]);
    return(s);
}

// Copy IP address (byte-by-byte, in case misaligned)
void ip_cpy(BYTE *dest, BYTE *src)
{
    *dest++ = *src++;
    *dest++ = *src++;
    *dest++ = *src++;
    *dest = *src;
}

// Convert byte-order in a 'short' variable
WORD htons(WORD w)
{
    return(w<<8 | w>>8);
}

// Convert byte-order in a 'short' variable, given byte pointer
WORD htonsp(BYTE *p)
{
    WORD w = (WORD)(*p++) << 8;
    return(w | *p);
}

// Convert byte-order in a 'long' variable
DWORD htonl(DWORD x)
{
    return((((x) & 0xff000000u) >> 24) | (((x) & 0x00ff0000u) >>  8) |
           (((x) & 0x0000ff00u) <<  8) | (((x) & 0x000000ffu) << 24));
}

/* Calculate TCP-style checksum, add to old value */
WORD add_csum(WORD sum, void *dp, int count)
{
    WORD n=count>>1, *p=(WORD *)dp, last=sum;

    while (n--)
    {
        sum += *p++;
        if (sum < last)
            sum++;
        last = sum;
    }
    if (count & 1)
        sum += *p & 0x00ff;
    if (sum < last)
        sum++;
    return(sum);
}

// EOF
