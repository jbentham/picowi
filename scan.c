// PicoWi network scan example, see https://iosoft.blog/picowi
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

#include "lib/picowi_defs.h"
#include "lib/picowi_pico.h"
#include "lib/picowi_wifi.h"
#include "lib/picowi_init.h"
#include "lib/picowi_ioctl.h"
#include "lib/picowi_event.h"
#include "lib/picowi_scan.h"

int main() 
{
    uint32_t led_ticks, poll_ticks;
    bool ledon=false;
    
    add_event_handler(scan_event_handler);
    set_display_mode(DISP_INFO);
    // Additional diagnostic display
    //set_display_mode(DISP_INFO|DISP_REG|DISP_IOCTL|DISP_SDPCM);
    io_init();
    printf("PicoWi network scan\n");
    if (!wifi_setup())
        printf("Error: SPI communication\n");
    else if (!wifi_init())
        printf("Error: can't initialise WiFi\n");
    else if (!scan_start())
        printf("Error: can't start scan\n");
    else
    {
        // Additional diagnostic display
        //set_display_mode(DISP_INFO|DISP_EVENT);
        ustimeout(&led_ticks, 0);
        ustimeout(&poll_ticks, 0);
        while (1)
        {
            // Toggle LED at 1 Hz
            if (ustimeout(&led_ticks, 500000))
                wifi_set_led(ledon = !ledon);
                
            // Get any events
            if (wifi_get_irq() || ustimeout(&poll_ticks, 10000))
            {
                if (event_poll() < 0)
                    printf("Total time %lu msec\n", ustime()/1000);
                ustimeout(&poll_ticks, 0);
            }
        }
    }
}

// EOF
