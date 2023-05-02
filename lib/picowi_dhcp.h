// PicoWi DHCP client definitions, see http://iosoft.blog/picowi for details
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

#define SNAME_LEN   64      /* Server name len */
#define BOOTF_LEN   128     /* Boot filename len */
#define DHCP_CLIENT_PORT 68 /* Client & server port numbers */
#define DHCP_SERVER_PORT 67

#define DHCP_REQUEST        1
#define DHCP_REPLY          2

#define DHCP_OPT_PAD        0
#define DHCP_OPT_SUBNET     1
#define DHCP_OPT_ROUTER     3
#define DHCP_OPT_DNS        6
#define DHCP_OPT_REQIP      50
#define DHCP_OPT_LEASE      51
#define DHCP_OPT_MSGTYPE    53
#define DHCP_OPT_SERVERID   54
#define DHCP_OPT_LIST       55
#define DHCP_OPT_T1         58
#define DHCP_OPT_T2         59
#define DHCP_OPT_END        255

#define DHCP_COOKIE_LEN     4
#define DHCP_TIMEOUT        2000000

#pragma pack(1)

// DHCP message options
typedef struct {
    BYTE typ1, len1, opt;
    BYTE typ2, len2, data[4];
    BYTE end;
} DHCP_MSG_OPTS;

/* Option code 53 */
typedef enum {
    DHCPT_DISCOVER  =   1,
    DHCPT_OFFER     =   2,
    DHCPT_REQUEST   =   3,
    DHCPT_DECLINE   =   4,
    DHCPT_ACK       =   5,
    DHCPT_NAK       =   6,
    DHCPT_RELEASE   =   7,
    DHCPT_INFORM    =   8
} DHCP_TYPE;

#define DHCP_TYPESTRS "DISCOVER","OFFER","REQUEST","DECLINE","ACK","NAK","RELEASE","INFORM"

typedef struct {
    BYTE  opcode;               /* Message opcode/type. */
    BYTE  htype;                /* Hardware addr type (net/if_types.h). */
    BYTE  hlen;                 /* Hardware addr length. */
    BYTE  hops;                 /* Number of relay agent hops from client. */
    DWORD trans;                /* Transaction ID. */
    WORD secs;                  /* Seconds since client started looking. */
    WORD flags;                 /* Flag bits. */
    IPADDR ciaddr,              /* Client IP address (if already in use). */
           yiaddr,              /* Client IP address. */
           siaddr,              /* Server IP address */
           giaddr;              /* Relay agent IP address. */
    BYTE chaddr [16];           /* Client hardware address. */
    char sname[SNAME_LEN];      /* Server name. */
    char bname[BOOTF_LEN];      /* Boot filename. */
    BYTE cookie[DHCP_COOKIE_LEN];   /* Magic cookie */
} DHCPHDR;

#pragma pack()

int dhcp_event_handler(EVENT_INFO *eip);
void dhcp_poll(void);
int dhcp_rx(BYTE *buff, int len);
BYTE dhcp_msg_type(DHCPHDR *dhcp, int len);
int dhcp_get_opt_data(DHCPHDR *dhcp, int len, BYTE opt, BYTE *data, int maxlen);
BYTE *dhcp_get_opt(BYTE *buff, int len, BYTE opt);
char *dhcp_type_str(BYTE typ);

// EOF

