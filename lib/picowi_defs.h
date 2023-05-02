// PicoWi definitions, see http://iosoft.blog/picowi for details
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

#include <stdint.h>
#include <stdbool.h>

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif
typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned int    DWORD;

#define RXDATA_LEN      1600
#define TXDATA_LEN      1600

// Display mask values
#define DISP_NOTHING    0       // No display
#define DISP_INFO       0x01    // General information
#define DISP_SPI        0x02    // SPI transfers
#define DISP_REG        0x04    // Register read/write
#define DISP_SDPCM      0x08    // SDPCM transfers
#define DISP_IOCTL      0x10    // IOCTL read/write
#define DISP_EVENT      0x20    // Event reception
#define DISP_DATA       0x40    // Data transfers
#define DISP_JOIN       0x80    // Network joining

#define DISP_ETH        0x1000  // Ethernet header
#define DISP_ARP        0x2000  // ARP header
#define DISP_ICMP       0x4000  // ICMP header
#define DISP_UDP        0x8000  // UDP header
#define DISP_DHCP       0x10000 // DHCP header
#define DISP_DNS        0x20000 // DNS header
#define DISP_SOCK       0x40000 // Socket
#define DISP_TCP        0x80000 // TCP
#define DISP_TCP_STATE  0x100000 // TCP state

#pragma pack(1)

// Async event parameters, used internally
typedef struct {
    uint32_t chan;                      // From SDPCM header
    uint32_t event_type, status, reason;// From async event (null if not event)
    uint16_t flags;
    uint16_t link;                      // Link state
    uint32_t join;                      // Joining state
    uint8_t *data;                      // Data block
    int     dlen;
    int     server_port;                // Port number if server
    int     sock;                       // Socket number if TCP
} EVENT_INFO;
#pragma pack()

void set_display_mode(int mask);
void display(int mask, const char* fmt, ...);
void wifi_set_led(bool on);
int link_check(void);

// EOF
