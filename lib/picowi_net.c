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

#include <stdio.h>
#include <string.h>

#include "picowi_defs.h"
#include "picowi_pico.h"
#include "picowi_spi.h"
#include "picowi_init.h"
#include "picowi_ioctl.h"
#include "picowi_event.h"
#include "picowi_join.h"
#include "picowi_ip.h"
#include "picowi_udp.h"
#include "picowi_dhcp.h"
#include "picowi_dns.h"
#include "picowi_net.h"

#define EVENT_POLL_USEC     100000

extern int display_mode;

// Initialise the network stack
int net_init(void)
{
    int ok = 0;
    
    add_event_handler(join_event_handler);
    add_event_handler(arp_event_handler);
    add_event_handler(dhcp_event_handler);
    add_event_handler(udp_event_handler);
    udp_sock_init(udp_dns_handler, zero_ip, 0, DNS_SERVER_PORT);
    printf("PicoWi DHCP client\n");
    if (!wifi_setup())
        printf("Error: SPI communication\n");
    else if (!wifi_init())
        printf("Error: can't initialise WiFi\n");
    else
        ok = 1;
    return (ok);
}

// Join a network
int net_join(char *ssid, char *passwd)
{
    int ok = 0;
    
    if (!join_start(ssid, passwd))
        printf("Error: can't start network join\n");
    else if (!ip_init(0))
        printf("Error: can't start IP stack\n");
    else
        ok = 1;
    return (ok);
}

// Poll the network interface
void net_poll(void)
{
    static uint32_t poll_ticks;

    // Get any events, poll the network-join state machine
    if (get_irq() || ustimeout(&poll_ticks, EVENT_POLL_USEC))
    {
        event_poll();
        join_state_poll(0, 0);
        ustimeout(&poll_ticks, 0);
    }
    // If link is up, poll DHCP state machine
    if (link_check() > 0)
        dhcp_poll();
    // When DHCP is complete, print IP addresses
    if (dhcp_complete == 1 && (display_mode & DISP_INFO))
    {
        printf("DHCP complete, IP address ");
        print_ip_addr(my_ip);
        printf(" router ");
        print_ip_addr(router_ip);
        printf("\n");
        dhcp_complete = 2;
    }
}

// Set socket option
int setsockopt(int sock, int level, int optname, void *optval, socklen_t optlen)
{
    UDP_SOCKET *usp = &udp_sockets[sock];
    struct timeval *tvp = optval;
    int ret = -1;
    
    if (sock >= 0 && sock <= NUM_UDP_SOCKETS)
    {
        if (optname == SO_RCVTIMEO && optlen == sizeof(struct timeval))
        {
            usp->timeout = tvp->tv_sec * 1000000 + tvp->tv_usec;
            ustimeout((uint32_t *)&usp->ticks, 0);
            ret = 0;
        }
    }
    return (ret);
}

// Convert IP address to string
char *inet_ntoa(struct in_addr  addr)
{
    static char temps[20];
    unsigned char *a = (unsigned char *)&addr;
    sprintf(temps, "%u.%u.%u.%u", a[0], a[1], a[2], a[3]);
    return (temps);
}

// Open a new socket
int socket(int domain, int type, int protocol)
{
    return (udp_sock_unused());
}
        
// Bind a UDP socket to a port (to act as server)
int bind(int sock, struct sockaddr *addr, socklen_t addrlen)
{
    struct sockaddr_in *sinp = (struct sockaddr_in *)addr;
    int ok = -1;
    
    if (sock >= 0 && sock < NUM_UDP_SOCKETS)
    {
        udp_sock_set(sock, 0, zero_ip, 0, htons(sinp->sin_port));
        ok = 1;
    }
    return (ok);
}

// Recieve from a UDP datagram, return the data and IP address
int recvfrom(int sock, void *mem, size_t len, int flags, struct sockaddr *from, socklen_t *fromlen)
{
    int dlen = 0;
    struct sockaddr_in *sinp = (struct sockaddr_in *)from;
    UDP_SOCKET *usp = &udp_sockets[sock];
    
    if (sock < 0 || sock >= NUM_UDP_SOCKETS)
        return (0);
    while (usp->dlen < UDP_DATA_OFFSET && 
        (usp->timeout == 0 || !ustimeout((uint32_t *)&usp->ticks, usp->timeout)))
    {
        net_poll();
    }
    if (usp->dlen >= UDP_DATA_OFFSET)
    {
        IP_CPY((uint8_t *)&sinp->sin_addr, usp->rem_ip);
        dlen = usp->dlen - UDP_DATA_OFFSET;
        dlen = dlen < len ? dlen : len;
        memcpy(mem, &usp->data[UDP_DATA_OFFSET], dlen);
        usp->dlen = 0;
    }
    return (dlen);
}

int sendto(int sock, void *data, size_t size, int flags, struct sockaddr *to, socklen_t tolen)
{
    UDP_SOCKET *usp = &udp_sockets[sock];
    
    if (sock >= 0 && sock < NUM_UDP_SOCKETS)
        return (udp_tx(usp->rem_mac, usp->rem_ip, usp->rem_port, usp->loc_port, data, size));
    return (0);
}

// EOF
