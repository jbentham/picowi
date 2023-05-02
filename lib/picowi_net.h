// PicoWi net socket interface, see https://iosoft.blog/picowi
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

extern IPADDR my_ip, router_ip, dns_ip, zero_ip;
extern int dhcp_complete;

#define AF_INET         2
#define INADDR_ANY      0
#define SOL_SOCKET      0xFFF
#define SO_RCVTIMEO     0

#define SOCK_STREAM     1
#define SOCK_DGRAM      2

#define NUM_NET_SOCKETS 5

typedef int(*web_handler_t)(int sock, char *req, int oset);

#pragma pack(1)
struct net_socket_t
{
    WORD rem_port, loc_port;
    IPADDR rem_ip;
    MACADDR rem_mac;
    BYTE padding[2];
    BYTE *rxdata;
    int rxlen, rxdlen, txdlen, tries, close, errors;
    uint32_t ticks, timeout;
    int sock_type, state;
    DWORD seq, ack, rx_seq, rx_ack, start_seq, last_rx_ack;
    int(*sock_handler)(struct net_socket_t *usp);
    web_handler_t web_handler;
    BYTE txbuff[MAXFRAME];
};
typedef struct net_socket_t NET_SOCKET;
typedef int(*net_handler_t)(struct net_socket_t *usp);

typedef uint32_t in_addr_t;
typedef int socklen_t;

struct in_addr {
    in_addr_t s_addr;
};

struct sockaddr_in {
    uint8_t         sin_len;
    uint8_t         sin_family;
    u_short         sin_port;
    struct in_addr  sin_addr;
    char            sin_zero[8];
};

struct sockaddr {
    uint8_t        sa_len;
    uint8_t        sa_family;
    char           sa_data[14];
};
#pragma pack()

int net_init(void);
int net_join(char *ssid, char *passwd);
int net_event_poll(void);
void net_state_poll(void);
int setsockopt(int sock, int level, int optname, void *optval, socklen_t optlen);
char *inet_ntoa(struct in_addr  addr);
int socket(int domain, int type, int protocol);
int bind(int sock, struct sockaddr *addr, socklen_t addrlen);
int listen(int sock, int backlog);
int accept(int server_sock, struct sockaddr *addr, socklen_t *addrlen);
int recvfrom(int sock, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen);
int sendto(int sock, void *data, size_t size, int flags, struct sockaddr *to, socklen_t tolen);
NET_SOCKET *net_socket_ptr(int sock);

// EOF
