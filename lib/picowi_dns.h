// PicoWi DNS client definitions, see http://iosoft.blog/picowi for details
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

#define DNS_SERVER_PORT 53

#pragma pack(1)
typedef struct
{
    WORD ident,
        flags,
        n_query,
        n_ans,
        n_auth,
        n_rr;
} DNS_HDR;
#pragma pack()

int dns_add_data(BYTE *buff, char *s);
int dns_tx(MACADDR mac, IPADDR dip, WORD sport, char *s);
int udp_dns_handler(NET_SOCKET *usp);
char *dns_hdr_str(char *s, BYTE *buff, int len);
char *dns_name_str(char *tmps, BYTE *buff, int len, int *osetp, int *typ, IPADDR addr);
int dns_num_resps(BYTE *buff, int len);

// EOF
