// PicoWi Web interface, see http://iosoft.blog/picowi for details
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
#include "picowi_ip.h"
#include "picowi_net.h"
#include "picowi_tcp.h"
#include "picowi_web.h"

int num_web_handlers;
extern NET_SOCKET net_sockets[NUM_NET_SOCKETS];
WEB_HANDLER web_handlers[MAX_WEB_HANDLERS];

// Set up a Web page handler
int web_page_handler(char *req, web_handler_t handler)
{
    if (num_web_handlers < MAX_WEB_HANDLERS)
    {
        web_handlers[num_web_handlers].req = req;
        web_handlers[num_web_handlers++].handler = handler;
        return (1);
    }
    return (0);
}    

// Handle a Web page request, return length of response
int web_page_rx(int sock, char *req, int len)
{
    int i, n;
    WEB_HANDLER *whp=web_handlers;
    NET_SOCKET *ts = &net_sockets[sock];
    
    req[len] = 0;
    for (i = 0; i < MAX_WEB_HANDLERS; i++, whp++)
    {
        if (whp->req && whp->handler)
        {
            n = strlen(whp->req);
            if (!memcmp(whp->req, req, n))
            {
                ts->web_handler = whp->handler;
                return (whp->handler(sock, req, 0));
            }
        }
    }
    return (0);
}

// Add data to an HTTP response
int web_resp_add_data(int sock, BYTE *data, int dlen)
{
    return (tcp_sock_add_tx_data(sock, data, dlen));
}

// Add string to an HTTP response
int web_resp_add_str(int sock, char *str)
{
    return (tcp_sock_add_tx_data(sock, (BYTE *)str, strlen(str)));
}

// Add content length string to an HTTP response
int web_resp_add_content_len(int sock, int n)
{
    NET_SOCKET *ts = &net_sockets[sock];
    int i = sprintf((char *)&ts->txbuff[TCP_DATA_OFFSET + ts->txdlen], HTTP_CONTENT_LENGTH, n);
    ts->txdlen += i;
    return (i);
}

// Send a Web response
int web_resp_send(int sock)
{
    NET_SOCKET *ts = net_socket_ptr(sock);

    return(tcp_sock_send(sock, TCP_ACK, 0, ts->txdlen));
}

// EOF
