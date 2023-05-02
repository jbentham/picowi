// PicoWi ICMP ping example, see https://iosoft.blog/picowi
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

// The hard-coded password is for test purposes only!!!
#define SSID                "testnet"
#define PASSWD              "testpass"
#define EVENT_POLL_USEC     100000
#define PING_RESP_USEC      200000
#define PING_DATA_SIZE      32

// IP address of this unit (must be unique on network)
IPADDR myip   = IPADDR_VAL(192, 168, 1, 123);
// IP address of Access Point
IPADDR hostip = IPADDR_VAL(192, 168, 1, 1);

BYTE ping_data[PING_DATA_SIZE];

extern uint32_t ping_tx_time, ping_rx_time;

int main()
{
    uint32_t led_ticks, poll_ticks, ping_ticks;
    bool ledon=false;
    MACADDR mac;
    int i, ping_state=0, t;
    
    for (i=0; i<sizeof(ping_data); i++)
        ping_data[i] = (BYTE)i;
    add_event_handler(icmp_event_handler);
    add_event_handler(arp_event_handler);
    add_event_handler(join_event_handler);
    set_display_mode(DISP_INFO);
    io_init();
    printf("PicoWi ping client & server\n");
    if (!wifi_setup())
        printf("Error: SPI communication\n");
    else if (!wifi_init())
        printf("Error: can't initialise WiFi\n");
    else if (!join_start(SSID, PASSWD))
        printf("Error: can't start network join\n");
    else if (!ip_init(myip))
        printf("Error: can't start IP stack\n");
    else
    {
        // Additional diagnostic display
        set_display_mode(DISP_INFO|DISP_JOIN|DISP_ARP|DISP_ICMP);
        ustimeout(&led_ticks, 0);
        ustimeout(&poll_ticks, 0);
        while (1)
        {
            // Toggle LED at 0.5 Hz if joined, 5 Hz if not
            if (ustimeout(&led_ticks, link_check() > 0 ? 1000000 : 100000))
            {
                wifi_set_led(ledon = !ledon);
                ustimeout(&ping_ticks, 0);
                // If LED is on, and we have joined a network..
                if (ledon && link_check()>0)
                {
                    // If not ARPed, send ARP request
                    if (!ip_find_arp(hostip, mac))
                    {
                        ip_tx_arp(mac, hostip, ARPREQ);
                        ping_state = 1;
                    }
                    // If ARPed, send ICMP request
                    else
                    {
                        ip_tx_icmp(mac, hostip, ICREQ, 0, ping_data, sizeof(ping_data));
                        ping_rx_time = 0;
                        ping_state = 2;
                    }
                }
            }
            // If awaiting ARP response..
            if (ping_state == 1)
            {
                // If we have an ARP response, send ICMP request
                if (ip_find_arp(hostip, mac))
                {
                    ustimeout(&ping_ticks, 0);
                    ip_tx_icmp(mac, hostip, ICREQ, 0, ping_data, sizeof(ping_data));
                    ping_rx_time = 0;
                    ping_state = 2;
                }
            }
            // Check for timeout on ARP or ICMP request
            if ((ping_state==1 || ping_state==2) && 
                ustimeout(&ping_ticks, PING_RESP_USEC))
            {
                printf("%s timeout\n", ping_state==1 ? "ARP" : "ICMP");
                ping_state = 0;
            }
            // If ICMP response received, LED off, print time
            else if (ping_state == 2 && ping_rx_time)
            {
                t = (ping_rx_time - ping_tx_time + 50) / 100;
                printf("Round-trip time %d.%d ms\n", t/10, t%10);
                wifi_set_led(0);
                ping_state = 0;
            }
            // Get any events, poll the network-join state machine
            if (wifi_get_irq() || ustimeout(&poll_ticks, EVENT_POLL_USEC))
            {
                event_poll();
                join_state_poll(SSID, PASSWD);
                ustimeout(&poll_ticks, 0);
            }
        }
    }
}

// EOF
