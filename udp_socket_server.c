// PicoWi UDP socket server example, see https://iosoft.blog/picowi
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

#include "lib/picowi_defs.h"
#include "lib/picowi_pico.h"
#include "lib/picowi_ip.h"
#include "lib/picowi_net.h"

// The hard-coded password is for test purposes only!!!
#define SSID     "testnet"
#define PASSWD   "testpass"

#define PORTNUM  8080
#define MAXLEN   1024 

void sock_set_timeout(int sock, DWORD usec);

int main()
{
    int server_sock, n, testnum=1, msec=0;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addrlen = sizeof(client_addr);
    char buffer[MAXLEN]; 
    
    io_init();
    usdelay(1000);
    set_display_mode(DISP_INFO | DISP_JOIN | DISP_UDP | DISP_SOCK);
    if (net_init() && net_join(SSID, PASSWD))
    {
        server_sock = socket(AF_INET, SOCK_DGRAM, 0);
        memset(&server_addr, 0, sizeof(server_addr)); 
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; 
        server_addr.sin_port = htons(PORTNUM);         
        if (server_sock >= 0 &&
            bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
        { 
            perror("bind failed"); 
            return (1); 
        } 
        printf("UDP server on port %u\n", PORTNUM);
        sock_set_timeout(server_sock, 1000);
        while (1)
        {
            memset(&client_addr, 0, sizeof(client_addr)); 
            addrlen = sizeof(client_addr);
            n = recvfrom(server_sock, buffer, MAXLEN, 0, 
                (struct sockaddr *)&client_addr, &addrlen);
            if (n > 0)
            {
                buffer[n] = 0; 
                printf("Client %s: %s\n", inet_ntoa(client_addr.sin_addr), buffer); 
                n = sprintf(buffer, "Test %u\n", testnum++);
                sendto(server_sock, buffer, n, 0, 
                    (struct sockaddr *)&client_addr, addrlen);
                msec = 0;
            }
            else
            {
                msec++;
                if (msec == 1)
                    wifi_set_led(1);
                else if (msec == 100)
                    wifi_set_led(0);
                else if (msec == 200 && !link_check())
                    msec = 0;
                else if (msec == 2000)
                    msec = 0;
            }
        }
            
    }
}

// Set socket timeout
void sock_set_timeout(int sock, DWORD usec)
{
    struct timeval tv;
    
    tv.tv_sec = 0;
    tv.tv_usec = usec;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

// EOF
