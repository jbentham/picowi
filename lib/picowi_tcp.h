// PicoWi TCP definitions, see https://iosoft.blog/picowi
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

#pragma pack(1)

#define TCP_NUM_SOCKETS 5

#define TCP_MSS         1460
#define TCP_WINDOW      (1 * TCP_MSS)
#define TCP_CHECK_USEC  10000000
#define TCP_RETRY_USEC  2000000
#define TCP_TRIES       5

/* Well-known TCP port numbers */
#define ECHOPORT    7       /* Echo */
#define DAYPORT     13      /* Daytime */
#define CHARPORT    19      /* Character generator */
#define TELPORT     23      /* Telnet remote login */
#define HTTPORT     80      /* HTTP */

/* Dummy values for the segment data length */
#define DLEN_NODATA -1      /* Segment received, but no data */

typedef enum {
    T_CLOSED,
    T_LISTEN,
    T_SYN_SENT,
    T_SYN_RCVD,
    T_ESTABLISHED,
    T_FIN_WAIT_1,
    T_FIN_WAIT_2,
    T_CLOSE_WAIT,
    T_CLOSING,
    T_LAST_ACK,
    T_TIME_WAIT,
    T_FINISHED,
    T_FAILED,
    T_NUM_STATES
} TCP_STATES;

#define TCP_STATE_STRS "CLOSED", "LISTEN", "SYN_SENT", "SYN_RECEIVED", \
    "ESTABLISHED", "FIN_WAIT_1", "FIN_WAIT_2", "CLOSE_WAIT", "CLOSING", \
    "LAST_ACK", "TIME_WAIT", "FINISHED", "FAILED", ""

/* ***** TCP (Transmission Control Protocol) header ***** */
typedef struct tcph
{
    WORD  sport,            /* Source port */
          dport;            /* Destination port */
    DWORD seq,              /* Sequence number */
          ack;              /* Ack number */
    BYTE  hlen,             /* TCP header len (num of bytes << 2) */
          flags;            /* Option flags */
    WORD  window,           /* Flow control credit (num of bytes) */
          check,            /* Checksum */
          urgent;           /* Urgent data pointer */
} TCPHDR;

#define TCP_DATA_OFFSET (sizeof(ETHERHDR) + sizeof(IPHDR) + sizeof(TCPHDR))

#define TCP_FIN     0x01    /* Option flags: no more data */
#define TCP_SYN     0x02    /*           sync sequence nums */
#define TCP_RST     0x04    /*           reset connection */
#define TCP_PUSH    0x08    /*           push buffered data */
#define TCP_ACK     0x10    /*           acknowledgement */
#define TCP_URGE    0x20    /*           urgent */

#pragma pack()

void tcp_init(void);
int tcp_sock_unused(void);
void tcp_sock_set(int sock, net_handler_t handler, IPADDR remip, WORD remport, WORD locport);
int tcp_server_event_handler(EVENT_INFO *eip);
int tcp_sock_match(IPADDR remip, WORD remport, WORD locport, BYTE flags);
void tcp_socks_poll(void);
int tcp_sock_rx(int sock, BYTE *data, int len);
void tcp_sock_clear(int sock);
int tcp_get_resp(int sock, BYTE *data, int dlen);
int tcp_sock_add_tx_data(int sock, BYTE *data, int dlen);
void tcp_new_state(int sock, BYTE news);
int tcp_sock_fail(int sock);
void tcp_sock_close(int sock);
int tcp_sock_send(int sock, BYTE flags, void *data, int dlen);
int tcp_send_reset(int sock, MACADDR mac, IPADDR dip, WORD remport, WORD locport, DWORD seq, DWORD ack);
int tcp_tx(int sock, BYTE *buff, MACADDR mac, IPADDR dip, WORD remport, WORD locport, DWORD seq, DWORD ack, BYTE flags, void *data, int dlen);
int tcp_add_hdr_data(BYTE *buff, IPADDR dip, WORD remport, WORD locport, DWORD seq, DWORD ack, BYTE flags, void *data, int dlen);
WORD tcp_checksum(TCPHDR *tcp, IPADDR sip, IPADDR dip, int tlen);
WORD checksum(void *data, int len);
void tcp_print_hdr(int sock, BYTE *data, int dlen);

// EOF
