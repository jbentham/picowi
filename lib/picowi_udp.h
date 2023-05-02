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

int udp_event_handler(EVENT_INFO *eip);
int udp_sock_unused(void);
void udp_sock_set(int sock, net_handler_t handler, IPADDR remip, WORD remport, WORD locport);
NET_SOCKET *udp_sock_init(net_handler_t handler, IPADDR remip, WORD remport, WORD locport);
int dns_add_hdr_data(BYTE *buff, char *s);
int udp_sock_match(IPADDR remip, WORD locport, WORD remport);
int udp_sock_rx(NET_SOCKET *usp, BYTE *data, int len);
int udp_tx(MACADDR mac, IPADDR dip, WORD remport, WORD locport, void *data, int dlen);
int udp_add_hdr_data(BYTE *buff, WORD sport, WORD dport, void *data, int dlen);
void udp_print_hdr(BYTE *data, int dlen);

// EOF
