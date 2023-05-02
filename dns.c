// PicoWi DNS example, see https://iosoft.blog/picowi
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

#include "lib/picowi_defs.h"
#include "lib/picowi_pico.h"
#include "lib/picowi_wifi.h"
#include "lib/picowi_init.h"
#include "lib/picowi_ioctl.h"
#include "lib/picowi_event.h"
#include "lib/picowi_join.h"
#include "lib/picowi_ip.h"
#include "lib/picowi_net.h"
#include "lib/picowi_udp.h"
#include "lib/picowi_dhcp.h"
#include "lib/picowi_dns.h"

// The hard-coded password is for test purposes only!!!
#define SSID                "testnet"
#define PASSWD              "testpass"
#define EVENT_POLL_USEC     100000
#define PING_RESP_USEC      200000
#define PING_DATA_SIZE      32
//#define SERVER_NAME         "pool.ntp.org"
#define SERVER_NAME         "www.raspberrypi.org"
#define LOCAL_PORT          1234

extern IPADDR my_ip, router_ip, dns_ip, zero_ip;
extern int dhcp_complete;

int main()
{
    uint32_t led_ticks, poll_ticks, dns_ticks;
    bool ledon=false;
    MACADDR mac;
    
    add_event_handler(join_event_handler);
    add_event_handler(arp_event_handler);
    add_event_handler(dhcp_event_handler);
    add_event_handler(udp_event_handler);
    udp_sock_init(udp_dns_handler, zero_ip, DNS_SERVER_PORT, LOCAL_PORT);
    set_display_mode(DISP_INFO);
    io_init();
    printf("PicoWi DHCP client\n");
    if (!wifi_setup())
        printf("Error: SPI communication\n");
    else if (!wifi_init())
        printf("Error: can't initialise WiFi\n");
    else if (!join_start(SSID, PASSWD))
        printf("Error: can't start network join\n");
    else if (!ip_init(0))
        printf("Error: can't start IP stack\n");
    else
    {
        // Additional diagnostic display
        set_display_mode(DISP_INFO|DISP_JOIN|DISP_ARP|DISP_UDP|DISP_DHCP|DISP_DNS);
        ustimeout(&led_ticks, 0);
        ustimeout(&poll_ticks, 0);
        while (1)
        {
            // Toggle LED at 0.5 Hz if joined, 5 Hz if not
            if (ustimeout(&led_ticks, link_check() > 0 ? 1000000 : 100000))
                wifi_set_led(ledon = !ledon);
            // Get any events, poll the network-join state machine
            if (wifi_get_irq() || ustimeout(&poll_ticks, EVENT_POLL_USEC))
            {
                event_poll();
                join_state_poll(SSID, PASSWD);
                ustimeout(&poll_ticks, 0);
            }
            // If link is up, poll DHCP state machine
            if (link_check() > 0)
                dhcp_poll();
            // When DHCP is complete, print IP addresses
            if (dhcp_complete == 1)
            {
                printf("DHCP complete, IP address ");
                print_ip_addr(my_ip);
                printf(" router ");
                print_ip_addr(router_ip);
                printf("\n");
                dhcp_complete = 2;
                ustimeout(&dns_ticks, 0);
            }
            if (dhcp_complete && ustimeout(&dns_ticks, 1000000))
            {
                if (!ip_find_arp(router_ip, mac))
                    ip_tx_arp(mac, router_ip, ARPREQ);
                else
                {
                    dns_tx(mac, dns_ip, LOCAL_PORT, SERVER_NAME);
                }
            }
        }
    }
}

// EOF
