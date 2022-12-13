// PicoWi scan definitions, see https://iosoft.blog/picowi
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

#define SCAN_CHAN_TIME      40
#define SSID_MAXLEN         32
#define SCANTYPE_ACTIVE     0
#define SCANTYPE_PASSIVE    1

#pragma pack(1)
typedef struct {
    uint32_t version;
    uint16_t action,
             sync_id;
    uint32_t ssidlen;
    uint8_t  ssid[SSID_MAXLEN],
             bssid[6],
             bss_type,
             scan_type;
    uint32_t nprobes,
             active_time,
             passive_time,
             home_time;
    uint16_t nchans,
             nssids;
    uint8_t  chans[14][2],
             ssids[1][SSID_MAXLEN];
} SCAN_PARAMS;
#pragma pack()

int scan_start(void);
int scan_event_handler(EVENT_INFO *eip);
void disp_ssid(uint8_t *data);

// EOF

