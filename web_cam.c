// PicoWi Web server, see https://iosoft.blog/picowi
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

// v0.01 JPB 2/5/23  Adapted from web_server.c v0.38

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lib/picowi_defs.h"
#include "lib/picowi_pico.h"
#include "lib/picowi_wifi.h"
#include "lib/picowi_init.h"
#include "lib/picowi_ioctl.h"
#include "lib/picowi_event.h"
#include "lib/picowi_join.h"
#include "lib/picowi_ip.h"
#include "lib/picowi_dhcp.h"
#include "lib/picowi_net.h"
#include "lib/picowi_tcp.h"
#include "lib/picowi_web.h"
#include "camera/cam_2640.h"

#define TEST_BLOCK_COUNT 100
#define TCP_MAXDATA   1400

// The hard-coded password is for test purposes only!!!
#define SSID                "testnet"
#define PASSWD              "testpass"

const char hexchar[] = "0123456789ABCDEF";

extern IPADDR my_ip, router_ip;
extern int dhcp_complete;
extern NET_SOCKET net_sockets[NUM_NET_SOCKETS];
extern BYTE cam_data[1024 * 30];

char testblock[256*5 + 3];

int web_root_handler(int sock, char *req, int oset);
int web_cam_handler(int sock, char *req, int oset);
int web_video_handler(int sock, char *req, int oset);
int web_favicon_handler(int sock, char *req, int oset);

int main()
{
    uint32_t led_ticks;
    bool ledon=false;
    int server_sock;
    struct sockaddr_in server_addr;
    
    //set_display_mode(DISP_INFO | DISP_JOIN | DISP_TCP | DISP_TCP_STATE);
    set_display_mode(DISP_INFO | DISP_JOIN | DISP_TCP_STATE);
    io_init();
    usdelay(1000);
    cam_init();
    if (net_init() && net_join(SSID, PASSWD))
    {
        server_sock = socket(AF_INET, SOCK_STREAM, 0);
        memset(&server_addr, 0, sizeof(server_addr)); 
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; 
        server_addr.sin_port = htons(HTTPORT);         
        if (server_sock < 0 ||
            bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ||
            listen(server_sock, 3) < 0)
        { 
            perror("socket/bind/listen failed"); 
            return (1); 
        }
        printf("TCP server on port %u\n", HTTPORT);
        web_page_handler("GET /favicon.ico", web_favicon_handler);
        web_page_handler("GET /camera.jpg", web_cam_handler);
        web_page_handler("GET /video", web_video_handler);
        web_page_handler("GET /", web_root_handler);
        ustimeout(&led_ticks, 0);
        while (1)
        {
            // Get any events, poll the network-join state machine
            net_event_poll();
            net_state_poll();
            tcp_socks_poll();
            // Toggle LED at 0.5 Hz if joined, 5 Hz if not
            if (ustimeout(&led_ticks, link_check() > 0 ? 1000000 : 100000))
                wifi_set_led(ledon = !ledon);
        }
    }
}

// Handler for root Web page
int web_root_handler(int sock, char *req, int oset)
{
    static int count = 1;
    char temps[100];
    int n=0;
    
    if (req)
    {
        printf("TCP socket %d Rx %s\n", sock, req);
        sprintf(temps, "<html><pre>Test %u\n</pre></html>", count++);
        n = web_resp_add_str(sock,
            HTTP_200_OK HTTP_SERVER HTTP_NOCACHE HTTP_CONNECTION_CLOSE
            HTTP_CONTENT_HTML HTTP_HEADER_END) + 
            web_resp_add_str(sock, temps);
        tcp_sock_close(sock);
    }
    return (n);
}

// Handler for single camera image
int web_cam_handler(int sock, char *req, int oset)
{
    int n = 0, diff;
    static int captime = 0, startime = 0, hlen = 0, dlen = 0;
    NET_SOCKET *ts = &net_sockets[sock];
    
    if (req)
    {
        printf("\nTCP socket %d Rx %s\n", sock, strtok(req, "\n"));
        hlen = n = web_resp_add_str(sock,
            HTTP_200_OK HTTP_SERVER HTTP_NOCACHE HTTP_CONNECTION_CLOSE
            HTTP_CONTENT_JPEG HTTP_HEADER_END);
        captime = ustime();
        dlen = cam_capture_single();
        startime = ustime();
        n += web_resp_add_data(sock, cam_data, TCP_MAXDATA - n);
    }
    else
    {
        n = MIN(TCP_MAXDATA, dlen + hlen - oset);
        if (n > 0)
            web_resp_add_data(sock, &cam_data[oset - hlen], n);
        else
        {
            tcp_sock_close(sock);
            diff = ustime() - startime;
            printf(RESOLUTION " capture %u usec, transfer %u bytes in %u usec, %u kbyte/s, %u errors\n",
                startime-captime, dlen, diff, (dlen * 1000)/diff, ts->errors);
        }
    }
    return (n);
}

// Handler for MJPEG video
int web_video_handler(int sock, char *req, int oset)
{
    int n = 0;
    static int hlen = 0, dlen = -1;
    //NET_SOCKET *ts = &net_sockets[sock];
    
    if (req)
    {
        printf("\nTCP socket %d Rx %s\n", sock, strtok(req, "\n"));
        hlen = n = web_resp_add_str(sock,
            HTTP_200_OK HTTP_SERVER HTTP_NOCACHE
            HTTP_MULTIPART HTTP_HEADER_END);
        dlen = -1;
    }
    else if (dlen == -1)
    {
        dlen = cam_capture_single();
        n = web_resp_add_str(sock, HTTP_BOUNDARY);
        n += web_resp_add_str(sock, HTTP_CONTENT_JPEG);
        n += web_resp_add_content_len(sock, dlen);
        n += web_resp_add_str(sock, HTTP_HEADER_END);
        hlen = oset + n;
    }
    else
    {
        n = MIN(TCP_MAXDATA, dlen + hlen - oset);
        if (n > 0)
            web_resp_add_data(sock, &cam_data[oset - hlen], n);
        else
            dlen = -1;
    }
    return (n);
}

// Handler for favicon
int web_favicon_handler(int sock, char *req, int oset)
{
    int n = 0;
    if (req)
    {
        printf("\nTCP socket %d Rx %s\n", sock, strtok(req, "\n"));
        n = web_resp_add_str(sock, HTTP_404_FAIL HTTP_SERVER HTTP_CONTENT_HTML HTTP_HEADER_END);
        tcp_sock_close(sock);
    }
    return (n);
}

// EOF
