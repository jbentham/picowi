// PicoWi TCP client, see http://iosoft.blog/picowi for details
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
#include "picowi_tcp.h"
#include "picowi_web.h"

extern int display_mode;

extern BYTE txbuff[TXDATA_LEN]; // Transmit buffer
extern int display_mode;        // Display mode
extern MACADDR my_mac;          // My MAC address
extern IPADDR my_ip;            // My IP address, and DNS server

char *tcflags[] =               // Text for TCP option flags
    {"<FIN>", "<SYN>", "<RST>", "<PSH>", "<ACK>", "<URG>"};
char *tstate_strings[] = { TCP_STATE_STRS };

extern NET_SOCKET net_sockets[NUM_NET_SOCKETS];
int accept_socket = -1;

int web_page_rx(int sock, char *req, int len);

// Initialise TCP sockets
void tcp_init(void)
{
}

// Find unused TCP socket, return -ve if none
int tcp_sock_unused(void)
{
    for (int i = 0; i < NUM_NET_SOCKETS; i++)
    {
        if (net_sockets[i].loc_port == 0 && net_sockets[i].rem_port == 0)
            return (i);
    }
    return (-1);
}

// Set parameters in a TCP socket
void tcp_sock_set(int sock, net_handler_t handler, IPADDR remip, WORD remport, WORD locport)
{
    NET_SOCKET *tsp = &net_sockets[sock];
    
    tsp = &net_sockets[sock];
    IP_CPY(tsp->rem_ip, remip);
    tsp->loc_port = locport;
    tsp->rem_port = remport;
    tsp->sock_handler = handler;
}

// Handler for incoming TCP segment
int tcp_server_event_handler(EVENT_INFO *eip)
{
    ETHERHDR *ehp = (ETHERHDR *)eip->data;
    IPHDR *ip = (IPHDR *)&eip->data[sizeof(ETHERHDR)];
    TCPHDR *tcp = (TCPHDR *)&eip->data[sizeof(ETHERHDR) + sizeof(IPHDR)];
    int sock, iplen = htons(ip->len);

    if (eip->chan == SDPCM_CHAN_DATA &&
        ip->pcol == PTCP &&
        IP_CMP(ip->dip, my_ip) &&
        ip_check_frame(eip->data, eip->dlen) &&
        eip->dlen >= TCP_DATA_OFFSET &&
        htons(tcp->dport) == eip->server_port)
    {
        eip->dlen = MIN(eip->dlen, sizeof(ETHERHDR) + iplen);
        sock = tcp_sock_match(ip->sip, htons(tcp->sport), htons(tcp->dport), tcp->flags);
        if (display_mode & DISP_TCP)
        {
            if (sock >= 0)
                printf("Rx%d ", sock);
            else
                printf("Rx  ");
            tcp_print_hdr(sock, eip->data, eip->dlen);
        }
        if (sock >= 0)
        {
            eip->sock = sock;
            return (tcp_sock_rx(sock, eip->data, eip->dlen));
        }
        else if (!(tcp->flags & TCP_RST))
        {
            tcp_send_reset(sock, ehp->srce, ip->sip, htons(tcp->sport), htons(tcp->dport),
                htonl(tcp->ack), htonl(tcp->seq) + (tcp->flags&TCP_SYN ? 1 : 0));
        }
        return(1);
    }
    return(0);
}

// Find matching socket for incoming TCP segment, return -ve if none
int tcp_sock_match(IPADDR remip, WORD remport, WORD locport, BYTE flags)
{
    NET_SOCKET *ts = 0;
    int i;

    for (i=0; i<TCP_NUM_SOCKETS; i++)
    {
        ts = &net_sockets[i];
        if (flags & TCP_SYN)
        {
            if (locport == ts->loc_port && (
                (ts->rem_port == 0 && ts->state == T_LISTEN) ||
                (remport == ts->rem_port && IP_CMP(remip, ts->rem_ip))))
                return (i);
        }
        else if (locport == ts->loc_port &&
            remport == ts->rem_port &&
            IP_CMP(remip, ts->rem_ip))
            return(i);
    }
    return (-1);
}

// Poll TCP sockets for timeout
void tcp_socks_poll(void)
{
    int i;
    
    for (i = 0; i < TCP_NUM_SOCKETS; i++)
    {
        if (net_sockets[i].state)
            tcp_sock_rx(i, 0, 0);
    }
}

// Receive incoming TCP segment; if no data, just check for socket timeout
int tcp_sock_rx(int sock, BYTE *data, int len)
{
    ETHERHDR *ehp = (ETHERHDR *)data;
    IPHDR *ip = 0;
    TCPHDR *tcp = 0;
    NET_SOCKET *ts = &net_sockets[sock];
    BYTE rflags = 0;
    int hlen = 0;

    if (data)
    {
        ip = (IPHDR *)&data[sizeof(ETHERHDR)];
        tcp = (TCPHDR *)&data[IP_DATA_OFFSET];
        hlen = (tcp->hlen & 0xf0) >> 2;
        rflags = tcp->flags & (TCP_FIN + TCP_SYN + TCP_RST + TCP_ACK);
        ts->rxdata = data;
        ts->rxlen = len;
        ts->rx_seq = htonl(tcp->seq);
        ts->rx_ack = htonl(tcp->ack);
        ts->rxdlen = len - IP_DATA_OFFSET - hlen;
    }
    accept_socket = -1;
    switch (ts->state)
    {
    // Listening socket, receiving connection request
    case T_LISTEN:
        if (rflags & TCP_SYN)
        {
            MAC_CPY(ts->rem_mac, ehp->srce);
            IP_CPY(ts->rem_ip, ip->sip);
            ts->loc_port = htons(tcp->dport);
            ts->rem_port = htons(tcp->sport);
            ts->seq = ustime();
            ts->start_seq = ts->seq + 1;
            ts->ack = ts->rx_seq + 1;
            tcp_sock_send(sock, TCP_SYN + TCP_ACK, 0, 0);
            tcp_new_state(sock, T_SYN_RCVD);
            ts->seq++;
            accept_socket = sock;
            ts->errors = 0;
        }
        break;
    // Sent SYN ACK, waiting for ACK
    case T_SYN_RCVD:
        if ((rflags & TCP_ACK) && ts->rx_seq == ts->ack && ts->rx_ack == ts->seq)
        {
            ts->last_rx_ack = ts->ack;
            tcp_new_state(sock, T_ESTABLISHED);
        }
        else if (ustimeout(&ts->ticks, TCP_RETRY_USEC) && !tcp_sock_fail(sock))
        {
            ts->seq--;
            tcp_sock_send(sock, TCP_SYN + TCP_ACK, 0, 0);
            ts->seq++;
        }
        break;
    // Connection established, waiting for data or closure
    case T_ESTABLISHED:
        ts->txdlen = 0;
        // Handle incoming TCP reset
        if ((rflags & TCP_RST) && ts->rx_seq == ts->ack)
        {
            tcp_new_state(sock, T_FAILED);
        }
        // Handle incoming TCP FIN
        else if ((rflags & TCP_FIN) && ts->rx_seq == ts->ack)
        {
            ts->ack++;
            tcp_sock_send(sock, TCP_ACK, 0, ts->txdlen);
            tcp_new_state(sock, T_CLOSE_WAIT);
        }
        else if (rflags && ts->rx_ack == ts->seq)
        {
            // Segment has been received
            if (ts->rxdlen != 1)
                ts->tries = 0;
            // Handle incoming data, put outgoing data in socket buffer
            if (ts->rxdlen > 0 && ts->rx_seq == ts->ack)
            {
                tcp_get_resp(sock, &data[IP_DATA_OFFSET + hlen], ts->rxdlen);
                ts->ack += ts->rxdlen;
            }
            // Remote closing of connection
            if (rflags & TCP_FIN)
            {
                ts->ack++;
                tcp_sock_send(sock, TCP_ACK, 0, ts->txdlen);
                tcp_new_state(sock, T_CLOSE_WAIT);
            }
            // Connection failure
            if (rflags & TCP_RST)
            {
                tcp_new_state(sock, T_FAILED);
            }
            // Send data in socket Tx buffer
            else if (ts->txdlen > 0)
            {
                tcp_sock_send(sock, TCP_ACK, 0, ts->txdlen);
                ts->seq += ts->txdlen;
            }
            // Local closing of connection
            else if (ts->close)
            {
                tcp_sock_send(sock, TCP_FIN + TCP_ACK, 0, 0);
                ts->seq++;
                tcp_new_state(sock, T_FIN_WAIT_1);
            }
        }
        // ACK does not match SEQ; rewind transmission
        else if (rflags & TCP_ACK)
        {
            if (ts->last_rx_ack != ts->rx_ack)
                ts->last_rx_ack = ts->rx_ack;
            else
            {
                ts->seq = ts->rx_ack;
                ts->errors++;
            }
        }
        // Check connection is OK: send ACK, should receive ACK
        else if (ustimeout(&ts->ticks, TCP_CHECK_USEC) && !tcp_sock_fail(sock))
        {
            ts->seq--;
            tcp_sock_send(sock, TCP_ACK, "\0", 1);
            ts->seq++;
        }
        // Get more data to send
        else if (!ts->close && ts->web_handler)
        { 
            ts->txdlen = 0;
            if (ts->web_handler(sock, 0, ts->seq - ts->start_seq) > 0)
            {
                tcp_sock_send(sock, TCP_ACK, 0, ts->txdlen);
                ts->seq += ts->txdlen;
            }
        }
        break;
    // Remote connection close: received FIN from client, send FIN ACK
    case T_CLOSE_WAIT:
        tcp_sock_send(sock, TCP_FIN + TCP_ACK, 0, 0);
        ts->seq++;
        tcp_new_state(sock, T_LAST_ACK);
        break;
    // Sent FIN ACK, waiting for final ACK
    case T_LAST_ACK:
        if ((rflags & TCP_ACK)  && ts->rx_seq == ts->ack)
            tcp_new_state(sock, T_FINISHED);
        else if (ustimeout(&ts->ticks, TCP_RETRY_USEC) && !tcp_sock_fail(sock))
        {
            ts->seq--;
            tcp_sock_send(sock, TCP_FIN + TCP_ACK, 0, 0);
            ts->seq++;
        }
        break;
    // Local connection close: FIN has been sent, wait for FIN or ACK
    case T_FIN_WAIT_1:
        if ((rflags & TCP_FIN) && (rflags & TCP_ACK) && ts->rx_seq == ts->ack)
        {
            ts->ack++;
            tcp_sock_send(sock, TCP_ACK, 0, 0);
            tcp_new_state(sock, T_TIME_WAIT);
        }
        else if ((rflags & TCP_FIN) && ts->rx_seq == ts->ack)
        {
            ts->ack++;
            tcp_sock_send(sock, TCP_ACK, 0, 0);
            tcp_new_state(sock, T_CLOSING);
        }
        else if ((rflags & TCP_ACK)  && ts->rx_seq == ts->ack)
            tcp_new_state(sock, T_FIN_WAIT_2);
        else if (ustimeout(&ts->ticks, TCP_RETRY_USEC) && !tcp_sock_fail(sock))
        {
            ts->seq--;
            tcp_sock_send(sock, TCP_FIN + TCP_ACK, 0, 0);
            ts->seq++;
        }
        break;
    // FIN sent & ACK received, awaiting FIN from remote
    case T_FIN_WAIT_2:
        if ((rflags & TCP_FIN) && ts->rx_seq == ts->ack)
        {
            ts->ack++;
            tcp_sock_send(sock, TCP_ACK, 0, 0);
            tcp_new_state(sock, T_TIME_WAIT);
        }
        else if (ustimeout(&ts->ticks, TCP_RETRY_USEC))
            tcp_sock_fail(sock);
        break;
    // Disconnection is complete
    case T_TIME_WAIT:
    case T_FINISHED:
    case T_FAILED:
        tcp_sock_clear(sock);
        tcp_new_state(sock, T_LISTEN);
        break;
    }
    return (1);
}

// Clear TCP socket
void tcp_sock_clear(int sock)
{
    NET_SOCKET *ts = &net_sockets[sock];
    WORD locport = ts->loc_port;
    int state = ts->state;
    
    memset(ts, 0, sizeof(NET_SOCKET));
    ts->loc_port = locport;
    ts->state = state;
}

// Read in TCP request, get response into socket Tx buffer, return length
int tcp_get_resp(int sock, BYTE *data, int dlen)
{
    return(web_page_rx(sock, (char *)data, dlen));
}

// Add Tx data to a TCP socket
int tcp_sock_add_tx_data(int sock, BYTE *data, int dlen)
{
    NET_SOCKET *ts = &net_sockets[sock];
    
    if (dlen<=0 || TCP_DATA_OFFSET+ts->txdlen+dlen > TCP_MSS)
        return (0);
    memcpy(&ts->txbuff[TCP_DATA_OFFSET + ts->txdlen], data, dlen);
    ts->txdlen += dlen;
    return (dlen);
}

// Change state of socket
void tcp_new_state(int sock, BYTE news)
{
    NET_SOCKET *ts = &net_sockets[sock];

    if ((display_mode & DISP_TCP_STATE) && ts->state < T_NUM_STATES && news < T_NUM_STATES)
        printf("TCP socket %u state %s -> %s\n", sock,
            tstate_strings[ts->state], tstate_strings[news]);
    ts->state = news;
    ts->ticks = ustime();
}

// If retry count has been exceeded, reset socket
int tcp_sock_fail(int sock)
{
    NET_SOCKET *ts = &net_sockets[sock];
    
    if (++ts->tries >= TCP_TRIES)
    {
        tcp_sock_send(sock, TCP_RST, 0, 0);
        tcp_new_state(sock, T_FAILED);
        return (1);
    }
    return (0);
}

// Close a socket
void tcp_sock_close(int sock)
{
    NET_SOCKET *ts = net_socket_ptr(sock);
    
    ts->close = 1;
}

// Send a TCP segment from a socket
int tcp_sock_send(int sock, BYTE flags, void *data, int dlen)
{
    NET_SOCKET *ts = &net_sockets[sock];

    ts->ticks = (DWORD)ustime();
    return(tcp_tx(sock, ts->txbuff, ts->rem_mac, ts->rem_ip, ts->rem_port, ts->loc_port,
        ts->seq, ts->ack, flags, data, dlen));
}

// Send a TCP 'reset' to client
int tcp_send_reset(int sock, MACADDR mac, IPADDR dip, WORD remport, WORD locport, DWORD seq, DWORD ack)
{
    tcp_tx(sock, txbuff, mac, dip, remport, locport, seq, ack, TCP_RST+TCP_ACK, 0, 0);
    return (1);
}

// Send a TCP segment
int tcp_tx(int sock, BYTE *buff, MACADDR mac, IPADDR dip, WORD remport, WORD locport, DWORD seq, DWORD ack,
    BYTE flags, void *data, int dlen)
{
    int tlen = tcp_add_hdr_data(&buff[IP_DATA_OFFSET], dip, remport, locport, seq, ack, flags, data, dlen);
    int len = ip_add_eth(buff, mac, my_mac, PCOL_IP);

    len += ip_add_hdr(&buff[len], dip, PTCP, tlen) + tlen;
    if (display_mode & DISP_TCP)
    {
        if (sock >= 0)
            printf("Tx%d ", sock);
        else
            printf("Tx  ");
        tcp_print_hdr(sock, buff, len);
    }
    return (ip_tx_eth(buff, len));
}

// Add TCP header and optional data to buffer
int tcp_add_hdr_data(BYTE *buff, IPADDR dip, WORD remport, WORD locport,
    DWORD seq, DWORD ack, BYTE flags, void *data, int dlen)
{
    TCPHDR *tcp = (TCPHDR *)buff;
    WORD hlen = sizeof(TCPHDR), *wp = (WORD *)&buff[sizeof(TCPHDR)], len;

    if (flags & TCP_SYN)
    {
        *wp++ = 0x0402;
        *wp = htons(TCP_MSS);
        hlen += 4;
    }
    tcp->sport = htons(locport);
    tcp->dport = htons(remport);
    tcp->hlen = (BYTE)(hlen << 2);
    tcp->flags = flags;
    tcp->check = tcp->urgent = 0;
    tcp->window = htons(TCP_WINDOW);
    tcp->seq = htonl(seq);
    tcp->ack = htonl(ack);
    len = hlen + ip_add_data(&buff[hlen], data, dlen);
    tcp->check = tcp_checksum(tcp, my_ip, dip, len);
    return (len);
}

// Return TCP checksum, given segment (TCP header + data) length.
WORD tcp_checksum(TCPHDR *tcp, IPADDR sip, IPADDR dip, int tlen)
{
    PHDR tph = {.len = htons(tlen), .z=0, .pcol=PTCP};
    DWORD sum = checksum(tcp, tlen);

    IP_CPY(tph.sip, sip);
    IP_CPY(tph.dip, dip);
    sum += checksum(&tph, sizeof(tph));
    return (WORD)(sum + (sum >> 16));
}

// Calculate checksum given data block
WORD checksum(void *data, int len)
{
    WORD *buff = data;
    DWORD cksum = 0;
    while (len > 1)
    {
        cksum += *buff++;
        len -= sizeof(WORD);
    }
    if (len)
        cksum += *(BYTE *)buff;
    cksum = (cksum >> 16) + (cksum & 0xffff);
    cksum += (cksum >> 16);
    return (WORD)(~cksum);
}

// Display TCP header
void tcp_print_hdr(int sock, BYTE *data, int dlen)
{
    IPHDR *ip = (IPHDR *)&data[sizeof(ETHERHDR)];
    TCPHDR *tcp = (TCPHDR *)&data[IP_DATA_OFFSET];
    int hlen = (tcp->hlen & 0xf0) >> 2, datalen;
    NET_SOCKET *ts = &net_sockets[sock];

    printf("TCP ");
    print_ip_addr(ip->sip);
    printf(":%u->", htons(tcp->sport));
    print_ip_addr(ip->dip);
    printf(":%u", htons(tcp->dport));
    if (IP_CMP(ip->sip, my_ip))
        printf(" seq %08x ack %08x ", htonl(tcp->seq), htonl(tcp->ack));
    else
        printf(" ack %08x seq %08x ", htonl(tcp->ack), htonl(tcp->seq));
    for (int i = 0; i < 6; i++)
        printf("%s", tcp->flags&(1 << i) ? tcflags[i] : "");
    if (hlen > sizeof(TCPHDR))
        printf(" optlen %d", hlen - sizeof(TCPHDR));
    datalen = dlen - sizeof(ETHERHDR) - sizeof(IPHDR) - hlen;
    if (datalen > 0)
        printf(" dlen %d", datalen);
    printf(" win %d", htons(tcp->window));
    printf(" [%d %03X %03X]\n", ts->state, ts->seq & 0xfff, ts->ack & 0xfff);
}

// EOF
