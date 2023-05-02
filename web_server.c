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

// v0.38 JPB 1/5/23  Adapted for iosoft.blog post

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

// The hard-coded password is for test purposes only!!!
#define SSID                "testnet"
#define PASSWD              "testpass"
#define EVENT_POLL_USEC     100000
#define PING_RESP_USEC      200000
#define PING_DATA_SIZE      32

#define TEST_BLOCK_COUNT 100

const char hexchar[] = "0123456789ABCDEF";
const char b64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

extern IPADDR my_ip, router_ip;
extern int dhcp_complete;
extern NET_SOCKET net_sockets[NUM_NET_SOCKETS];

BYTE testdata[256 * 3];
char testblock[256 * 5 + 3];
char temps[2000];

typedef struct {
    char name[16];
    int val;
} SERVER_ARG;

#define NUM_STATES  6
typedef enum { 
    STATE_IDLE,
    STATE_READY,
    STATE_PRELOAD,
    STATE_PRETRIG,
    STATE_POSTTRIG,
    STATE_UPLOAD
} STATE_VALS;

SERVER_ARG server_args[] = {
    { "state", STATE_IDLE },
    { "nsamp", 0 },
    { "xsamp", 10000 },
    { "xrate", 100000 },
    { "" }
};

int web_status_handler(int sock, char *req, int oset);
int web_json_status(char *buff, int maxlen);
int web_root_handler(int sock, char *req, int oset);
int web_data_handler(int sock, char *req, int oset);
int web_bin_data_handler(int sock, char *req, int oset);
int web_favicon_handler(int sock, char *req, int oset);
void make_testdata(BYTE *data, int len);
void make_testblock(char *s, int len);
int base64_encode(char *out, unsigned char *in, int ilen);

int main()
{
    uint32_t led_ticks;
    bool ledon = false;
    int server_sock;
    struct sockaddr_in server_addr;
    
    //set_display_mode(DISP_INFO | DISP_JOIN | DISP_TCP);
    set_display_mode(DISP_INFO | DISP_JOIN | DISP_TCP_STATE);
    io_init();
    usdelay(1000);
    make_testdata(testdata, sizeof(testdata));
    make_testblock(testblock, sizeof(testblock));
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
        printf("Web server on port %u\n", HTTPORT);
        web_page_handler("GET /favicon.ico", web_favicon_handler);
        web_page_handler("GET /status.txt", web_status_handler);
        web_page_handler("GET /data.txt", web_data_handler);
        web_page_handler("GET /data.bin", web_bin_data_handler);
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
    int n = 0;
    
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

// Handler Web status page
int web_status_handler(int sock, char *req, int oset)
{
    int n;
    
    web_json_status(temps, sizeof(temps) - 1);
    n = web_resp_add_str(sock,
        HTTP_200_OK HTTP_SERVER HTTP_NOCACHE HTTP_ORIGIN_ANY
        HTTP_CONTENT_HTML HTTP_CONNECTION_CLOSE HTTP_HEADER_END) + 
        web_resp_add_str(sock, temps);
    tcp_sock_close(sock);
    return (n);
}

// Return server status as json string
int web_json_status(char *buff, int maxlen) 
{
    SERVER_ARG *arg = server_args;
    int n = sprintf(buff, "{");
    while (arg->name[0] && n < maxlen - 20) 
    {
        n += sprintf(&buff[n], "%s\"%s\":%d", n > 2 ? "," : "", arg->name, arg->val);
        arg++;
    }
    return (n += sprintf(&buff[n], "}"));
}

// Handler for test data Web page
int web_data_handler(int sock, char *req, int oset)
{
    int n = 0, diff;
    static int dlen = 0, count = 0, last_oset = 0, startime;
    NET_SOCKET *ts = &net_sockets[sock];

    if (req)
    {
        printf("\nTCP socket %d Rx %s\n", sock, strtok(req, "\n"));
        dlen = base64_encode(temps, testdata, sizeof(testdata));
        n = web_resp_add_str(sock,
            HTTP_200_OK HTTP_SERVER HTTP_NOCACHE HTTP_ORIGIN_ANY
            HTTP_CONTENT_TEXT HTTP_CONNECTION_CLOSE HTTP_HEADER_END);
        n += web_resp_add_data(sock, (BYTE *)temps, dlen);
        count = last_oset = 0;
        startime = ustime();
    }
    else
    {
        n = web_resp_add_data(sock, (BYTE *)temps, dlen);
        if (oset - last_oset > 0)
        {
            last_oset = oset;
            count++;
        }
        if (count >= TEST_BLOCK_COUNT)
        {
            tcp_sock_close(sock);
            diff = ustime() - startime;
            printf("%u bytes in %u usec, %u kbyte/s, %u errors\n",
                count*sizeof(testblock),
                diff,
                (count*sizeof(testblock) * 1000)/diff,
                ts->errors);
        }
    }
    return (n);
}

// Handler for binary test data Web page
int web_bin_data_handler(int sock, char *req, int oset)
{
    int n = 0, diff;
    static int count = 0, last_oset = 0, startime;
    NET_SOCKET *ts = &net_sockets[sock];

    if (req)
    {
        printf("\nTCP socket %d Rx %s\n", sock, strtok(req, "\n"));
        n = web_resp_add_str(sock,
            HTTP_200_OK HTTP_SERVER HTTP_NOCACHE HTTP_ORIGIN_ANY
            HTTP_CONTENT_BINARY);
        n += web_resp_add_content_len(sock, TEST_BLOCK_COUNT * sizeof(testdata));
        n += web_resp_add_str(sock, HTTP_CONNECTION_CLOSE HTTP_HEADER_END);
        n += web_resp_add_data(sock, testdata, sizeof(testdata));
        count = last_oset = 0;
        startime = ustime();
    }
    else
    {
        n = web_resp_add_data(sock, testdata, sizeof(testdata));
        if (oset - last_oset > 0)
        {
            last_oset = oset;
            count++;
        }
        if (count >= TEST_BLOCK_COUNT)
        {
            tcp_sock_close(sock);
            diff = ustime() - startime;
            printf("%u bytes in %u usec, %u kbyte/s, %u errors\n",
                count*sizeof(testdata),
                diff,
                (count*sizeof(testdata) * 1000)/diff,
                ts->errors);
        }
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

// Make data block
void make_testdata(BYTE *data, int len)
{
    int n;
  
    for (n = 0; n < len; n++)
        data[n] = n & 1 ? (BYTE)(~n >> 1) : (BYTE)(n >> 1);
}

// Make test block
void make_testblock(char *s, int len)
{
    int n;
    
    for (n = 0; n < (len - 1) / 3; n++)
    {
        *s++ = hexchar[(n >> 4) & 15];
        *s++ = hexchar[n & 15];
        *s++ = ' ';
    }
    *s++ = '\n';
    *s++ = 0;
}

// Encode binry block as base64 string
int base64_encode(char *out, unsigned char *in, int ilen)
{
    int olen, i, j, v;

    olen = ilen % 3 ? ilen + 3 - ilen % 3 : ilen;
    olen = (olen / 3) * 4;
    out[olen] = '\0';
    for (i = 0, j = 0; i < ilen; i += 3, j += 4) 
    {
        v = in[i];
        v = i + 1 < ilen ? v << 8 | in[i + 1] : v << 8;
        v = i + 2 < ilen ? v << 8 | in[i + 2] : v << 8;
        out[j]   = b64chars[(v >> 18) & 0x3F];
        out[j + 1] = b64chars[(v >> 12) & 0x3F];
        if (i + 1 < ilen)
            out[j + 2] = b64chars[(v >> 6) & 0x3F];
        else
            out[j + 2] = '=';
        if (i + 2 < ilen)
            out[j + 3] = b64chars[v & 0x3F];
        else
            out[j + 3] = '=';
    }
    return (olen);
}
// EOF
