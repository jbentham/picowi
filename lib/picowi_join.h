// PicoWi WiFi join definitions, see http://iosoft.blog/picowi for details
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

// Security settings: 0 for none, 1 for WPA_TKIP, 2 for WPA2
#define SECURITY            2

// Enable power-saving (increases network response time)
#define POWERSAVE           0

// Flags for EVENT_INFO link state
#define LINK_UP_OK          0x01
#define LINK_AUTH_OK        0x02
#define LINK_OK            (LINK_UP_OK+LINK_AUTH_OK)
#define LINK_FAIL           0x04

#define JOIN_IDLE           0
#define JOIN_JOINING        1
#define JOIN_OK             2
#define JOIN_FAIL           3

#define JOIN_TRY_USEC       10000000
#define JOIN_RETRY_USEC     10000000

bool join_start(char *ssid, char *passwd);
bool join_stop(void);
bool join_restart(char *ssid, char *passwd);
int join_event_handler(EVENT_INFO *eip);
void join_state_poll(char *ssid, char *passwd);
int link_check(void);
int ip_event_handler(EVENT_INFO *eip);

// EOF