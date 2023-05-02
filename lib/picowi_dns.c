// PicoWi DNS client, see http://iosoft.blog/picowi for details
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
#include "picowi_ip.h"
#include "picowi_net.h"
#include "picowi_udp.h"
#include "picowi_dns.h"

extern BYTE txbuff[TXDATA_LEN]; // Transmit buffer
extern int display_mode;        // Display mode
extern MACADDR my_mac;          // My MAC address
extern IPADDR my_ip, dns_ip;    // My IP address, and DNS server
extern IPADDR zero_ip;

// Format DNS request data, given name string
int dns_add_hdr_data(BYTE *buff, char *s)
{
    BYTE *p, *q;
    DNS_HDR *dhp = (DNS_HDR *)buff;
    int len = sizeof(DNS_HDR);
    static int ident = 1;

    memset(dhp, 0, sizeof(DNS_HDR));
    dhp->ident = htons(ident++);
    dhp->flags = htons(0x100);  // Recursion desired
    dhp->n_query = htons(1);
    p = q = &buff[len];
    while (*s)                  // Prefix each part with length byte
    {
        p++;
        while (*s && *s != '.')
            *p++ = (BYTE)*s++;
        *q = (BYTE)(p - q - 1);
        q = p;
        if (*s)
            s++;
    }
    *p++ = 0;   // Null terminator
    *p++ = 0;   // Type A (host address)
    *p++ = 1;
    *p++ = 0;   // Class IN
    *p++ = 1;
    return (p - buff);
}

// Transmit DNS request
int dns_tx(MACADDR mac, IPADDR dip, WORD sport, char *s)
{
    char temps[300];
    int oset = 0;
    int len = ip_add_eth(txbuff, mac, my_mac, PCOL_IP);
    int dlen = dns_add_hdr_data(&txbuff[len + sizeof(IPHDR) + sizeof(UDPHDR)], s);

    len += ip_add_hdr(&txbuff[len], dip, PUDP, sizeof(UDPHDR) + dlen);
    len += udp_add_hdr_data(&txbuff[len], sport, DNS_SERVER_PORT, 0, dlen);
    if (display_mode & DISP_DNS)
    {
        printf("Tx %s:", dns_hdr_str(temps, txbuff, len));
        printf(" %s\n", dns_name_str(temps, txbuff, len, &oset, 0, 0));
    }
    if (display_mode & DISP_UDP)
    {
        printf("Tx ");
        udp_print_hdr(txbuff, len);
    }
    return (ip_tx_eth(txbuff, len));
}

// Handler for UDP DNS response
int udp_dns_handler(NET_SOCKET *usp)
{
    char temps[300];
    IPADDR addr;
    int oset = 0;

    if (display_mode & DISP_DNS)
    {
        printf("Rx %s: ", dns_hdr_str(temps, usp->rxdata, usp->rxlen));
        printf("%s\n", dns_name_str(temps, usp->rxdata, usp->rxlen, &oset, 0, 0));
        for (int n = 0; n < dns_num_resps(usp->rxdata, usp->rxlen); n++)
            printf("%s\n", dns_name_str(temps, usp->rxdata, usp->rxlen, &oset, 0, addr));
    }
    return (1);
}

// Convert header values to a string
char *dns_hdr_str(char *s, BYTE *buff, int len)
{
    DNS_HDR *dhp = (DNS_HDR *)&buff[sizeof(ETHERHDR) + sizeof(IPHDR) + sizeof(UDPHDR)];

    if (len > sizeof(ETHERHDR)+sizeof(IPHDR)+sizeof(UDPHDR)+sizeof(DNS_HDR))
    {
        if (dhp->n_ans)
            sprintf(s, "DNS %u query, %u resp", htons(dhp->n_query), htons(dhp->n_ans));
        else
            sprintf(s, "DNS %u query", htons(dhp->n_query));
    }
    else
        sprintf(s, "DNS length error");
    return (s);
}

// Return number of DNS responses
int dns_num_resps(BYTE *buff, int len)
{
    DNS_HDR *dhp = (DNS_HDR *)&buff[sizeof(ETHERHDR) + sizeof(IPHDR) + sizeof(UDPHDR)];

    return (len > sizeof(ETHERHDR)+sizeof(IPHDR)+sizeof(UDPHDR)+sizeof(DNS_HDR) ?
        htons(dhp->n_ans) : 0);
}

// Convert DNS name to string, set oset to next entry, optionally get type & IP address
char *dns_name_str(char *tmps, BYTE *buff, int len, int *osetp, int *typ, IPADDR addr)
{
    int n, i, j, ptr, type;
    char *s = tmps;

    // DNS header is after IP and UDP
    buff += sizeof(ETHERHDR) + sizeof(IPHDR) + sizeof(UDPHDR);
    len -= sizeof(ETHERHDR) + sizeof(IPHDR) + sizeof(UDPHDR);
    // Data begins after DNS header
    j = i = *osetp == 0 ? sizeof(DNS_HDR) : *osetp;
    ptr = htonsp(&buff[i]);
    // If compressed name, follow pointer to name string
    if ((ptr & 0xc000) == 0xc000)
        i = ptr & ~0xc000;
    if (i + 10 > len)
        return ("DNS response error");
    // Copy name string, using '.' separators
    while (buff[i] && i < len)
    {
        n = buff[i++];
        if (i + n > len)
            return ("DNS name error");
        while (n-- && buff[i])
            *s++ = (char)buff[i++];
        if (buff[i])
            *s++ = '.';
    }
    // Skip zero-byte end-of-string
    i++;
    // If compressed name, go back to original record
    if ((ptr & 0xc000) == 0xc000)
        i = j + 2;
    // Get entry type
    type = htonsp(&buff[i]);
    if (typ)
        *typ = type;
    s += sprintf(s, " type %s ", type==1?"A": type==2?"NS": type==5?"CNAME":
                 type==12?"PTR": type==13?"HINFO": type==15?"MX": "?");
    // Skip type & class
    i += 4;
    // If a response, get address
    if (*osetp)
    {
        // Skip time to live, get address length
        i += 4;
        n = htonsp(&buff[i]);
        i += 2;
        // If length is 4, copy address
        if (n == IPLEN)
        {
            if (addr)
                IP_CPY(addr, &buff[i]);
            ip_addr_str(s, &buff[i]);
        }
        i += n;
    }
    *osetp = i;
    return (tmps);
}

// EOF
