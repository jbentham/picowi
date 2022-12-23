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

#pragma pack(1)
struct udp_socket_t
{
	WORD remport, locport;
	IPADDR remip;
	BYTE *data;
	int dlen;
    int(*handler)(struct udp_socket_t *usp);
    };
typedef struct udp_socket_t UDP_SOCKET;
typedef int(*udp_handler_t)(struct udp_socket_t *usp);

#pragma pack()

int udp_event_handler(EVENT_INFO *eip);
UDP_SOCKET *udp_sock_init(udp_handler_t handler, IPADDR remip, WORD remport, WORD locport);
int dns_add_hdr_data(BYTE *buff, char *s);
UDP_SOCKET *udp_sock_match(IPADDR remip, WORD remport, WORD locport);
int udp_sock_rx(UDP_SOCKET *usp, BYTE *data, int len);

int udp_add_hdr_data(BYTE *buff, WORD sport, WORD dport, void *data, int dlen);
void udp_print_hdr(BYTE *data, int dlen);

// EOF
