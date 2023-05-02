// PicoWi IP definitions, see http://iosoft.blog/picowi for details
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

/* MAC address */
#define MACLEN      6           /* Ethernet (MAC) address length */
#define MAXFRAME    1500        /* Maximum frame size (excl CRC) */
typedef BYTE MACADDR[MACLEN];

/* Ethernet (DIX) header */
typedef struct {
    MACADDR dest;               /* Destination MAC address */
    MACADDR srce;               /* Source MAC address */
    WORD    ptype;              /* Protocol type or length */
} ETHERHDR;
#define PCOL_ARP    0x0806      /* Protocol type: ARP */
#define PCOL_IP     0x0800      /*                IP */

// Compare two MAC addresses
#define MAC_CMP(a, b) (a[0]==b[0]&&a[1]==b[1]&&a[2]==b[2]&&a[3]==b[3]&&a[4]==b[4]&&a[5]==b[5])
// Compare MAC address to broadcast
#define MAC_IS_BCAST(a) ((a[0]&a[1]&a[2]&a[3]&a[4]&a[5])==0xff)
// Set broadcast MAC address
#define MAC_BCAST(a) {a[0]=a[1]=a[2]=a[3]=a[4]=a[5]=0xff;}
// Check if MAC address is non-zero
#define MAC_IS_NONZERO(a) (a[0] || a[1] || a[2] || a[3] || a[4] || a[5])
// Copy a MAC address
#define MAC_CPY(a, b) memcpy(a, b, MACLEN)

/* IP address is an array of bytes, to avoid misalignment problems */
#define IPLEN           4
typedef BYTE            IPADDR[IPLEN];
// Initialiser for address variable
#define IPADDR_VAL(a, b, c, d) {a, b, c, d}
// Compare two IP addresses
#define IP_CMP(a, b)    (a[0]==b[0] && a[1]==b[1] && a[2]==b[2] && a[3]==b[3])
// Compare IP address to broadcast
#define IP_IS_BCAST(a)  ((a[0] & a[1] & a[2] & a[3]) == 0xff)
// Copy an IP address
#define IP_CPY(a, b)    ip_cpy(a, b) // memcpy((a), (b), IPLEN)
// Set an IP address to zero
#define IP_ZERO(a)      (a[0] = a[1] = a[2] = a[3] = 0)
// Check if IP address is zero
#define IP_IS_ZERO(a)   ((a[0] || a[1] || a[2] || a[3]) == 0)

/* ***** ARP (Address Resolution Protocol) packet ***** */
typedef struct
{
    WORD hrd,           /* Hardware type */
         pro;           /* Protocol type */
    BYTE  hln,          /* Len of h/ware addr (6) */
          pln;          /* Len of IP addr (4) */
    WORD op;            /* ARP opcode */
    MACADDR  smac;      /* Source MAC (Ethernet) addr */
    IPADDR   sip;       /* Source IP addr */
    MACADDR  dmac;      /* Destination Enet addr */
    IPADDR   dip;       /* Destination IP addr */
} ARPKT;

#define HTYPE       0x0001  /* Hardware type: ethernet */
#define ARPPRO      0x0800  /* Protocol type: IP */
#define ARPXXX      0x0000  /* ARP opcodes: unknown opcode */
#define ARPREQ      0x0001  /*              ARP request */
#define ARPRESP     0x0002  /*              ARP response */
#define RARPREQ     0x0003  /*              RARP request */
#define RARPRESP    0x0004  /*              RARP response */

typedef struct {
    MACADDR mac;
    IPADDR  ipaddr;
} ARP_ENTRY;

/* ***** IP (Internet Protocol) header ***** */
typedef struct
{
    BYTE   vhl,         /* Version and header len */
           service;     /* Quality of IP service */
    WORD   len,         /* Total len of IP datagram */
           ident,       /* Identification value */
           frags;       /* Flags & fragment offset */
    BYTE   ttl,         /* Time to live */
           pcol;        /* Protocol used in data area */
    WORD   check;       /* Header checksum */
    IPADDR sip,         /* IP source addr */
           dip;         /* IP dest addr */
} IPHDR;
#define PICMP   1           /* Protocol type: ICMP */
#define PTCP    6           /*                TCP */
#define PUDP   17           /*                UDP */

/* ***** IP packet ('datagram') ***** */
#define MAXIP (MAXFRAME-sizeof(IPHDR))
typedef struct
{
    IPHDR  i;               /* IP header */
    BYTE   data[MAXIP];     /* Data area */
} IPKT;
#define IP_DATA_OFFSET (sizeof(ETHERHDR) + sizeof(IPHDR))

/* ***** ICMP (Internet Control Message Protocol) header ***** */
typedef struct
{
    BYTE  type,         /* Message type */
          code;         /* Message code */
    WORD  check,        /* Checksum */
          ident,        /* Identifier */
          seq;          /* Sequence number */
} ICMPHDR;
#define ICREQ           8   /* Message type: echo request */
#define ICREP           0   /*               echo reply */
#define ICUNREACH       3   /*               destination unreachable */
#define ICQUENCH        4   /*               source quench */
#define UNREACH_NET     0   /* Destination Unreachable codes: network */
#define UNREACH_HOST    1   /*                                host */
#define UNREACH_PORT    3   /*                                port */
#define UNREACH_FRAG    4   /*     fragmentation needed, but disable flag set */

/* ***** UDP (User Datagram Protocol) header ***** */
typedef struct udph
{
    WORD  sport,            /* Source port */
          dport,            /* Destination port */
          len,              /* Length of datagram + this header */
          check;            /* Checksum of data, header + pseudoheader */
} UDPHDR;
#define UDP_DATA_OFFSET (sizeof(ETHERHDR) + sizeof(IPHDR) + sizeof(UDPHDR))

/* ***** Pseudo-header for UDP or TCP checksum calculation ***** */
/* The integers must be in hi-lo byte order for checksum */
typedef struct              /* Pseudo-header... */
{
    IPADDR sip,             /* Source IP address */
          dip;              /* Destination IP address */
    BYTE  z,                /* Zero */
          pcol;             /* Protocol byte */
    WORD  len;              /* UDP length field */
} PHDR;

#pragma pack()

int ip_init(IPADDR addr);
void ip_set_mac(BYTE *mac);
int ip_tx_eth(BYTE *buff, int len);
int ip_add_eth(BYTE *buff, MACADDR dmac, MACADDR smac, WORD pcol);
void ip_print_eth(BYTE *buff);
int arp_event_handler(EVENT_INFO *eip);
int ip_rx_arp(BYTE *data, int dlen);
int ip_make_arp(BYTE *buff, MACADDR mac, IPADDR addr, WORD op);
int ip_tx_arp(MACADDR mac, IPADDR addr, WORD op);
void ip_save_arp(MACADDR mac, IPADDR addr);
bool ip_find_arp(IPADDR addr, MACADDR mac);
void ip_print_arp(ARPKT *arp);
int ip_check_frame(BYTE *data, int dlen);
int ip_check_ip(BYTE *data, int dlen);
int ip_add_hdr(BYTE *buff, IPADDR dip, BYTE pcol, WORD dlen);
int icmp_event_handler(EVENT_INFO *eip);
int ip_rx_icmp(BYTE *data, int dlen);
int ip_add_icmp(BYTE *buff, BYTE type, BYTE code, void *data, WORD dlen);
int ip_add_data(BYTE *buff, void *data, int len);
int ip_make_icmp(BYTE *buff, MACADDR mac, IPADDR dip, BYTE type, BYTE code, BYTE *data, int dlen);
int ip_tx_icmp(MACADDR mac, IPADDR dip, BYTE type, BYTE code, BYTE *data, int dlen);
void ip_print_icmp(IPHDR *ip);

void print_mac_addr(MACADDR mac);
void print_ip_addr(IPADDR addr);
void print_ip_addrs(IPHDR *ip);
char *ip_addr_str(char *s, IPADDR addr);
void ip_cpy(BYTE *dest, BYTE *src);
WORD htons(WORD w);
WORD htonsp(BYTE *p);
DWORD htonl(DWORD d);
WORD add_csum(WORD sum, void *dp, int count);

// EOF
