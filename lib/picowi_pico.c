// PicoWi Pico low-level interface, see https://iosoft.blog/picowi
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
#include <stdio.h>
#include "pico/stdlib.h"

#include "picowi_pico.h"
#include "picowi_wifi.h"

// Initialise hardware
void io_init(void)
{
    stdio_init_all();
    gpio_set_input_hysteresis_enabled(SD_DIN_PIN, true);
}

// Set input or output with pullups
void io_set(int pin, int mode, int pull)
{
    gpio_init(pin);
    io_mode(pin, mode);
    io_pull(pin, pull);
}
// Set input or output
void io_mode(int pin, int mode)
{
    gpio_set_dir(pin, mode==IO_OUT);
}
// Set I/P pullup or pulldown
void io_pull(int pin, int pull)
{
    if (pull == IO_PULLUP)
        gpio_pull_up(pin);
    else if (pull == IO_PULLDN)
        gpio_pull_down(pin);
    else
        gpio_disable_pulls(pin);
}

// Set O/P strength
void io_strength(int pin, uint32_t strength)
{
    gpio_set_drive_strength(pin, strength);
}

// Set O/P slew rate
void io_slew(int pin, uint32_t slew)
{
    gpio_set_slew_rate(pin, slew);
}

// Set an O/P pin
inline void io_out(int pin, int val)
{
    gpio_put(pin, val != 0);
}
// Get an I/P pin value
inline uint8_t io_in(int pin)
{
    return(gpio_get(pin));
}

// Return timer tick value in microseconds
uint32_t ustime(void)
{
    return(time_us_32());
}

// Delay given number of microseconds
void usdelay(uint32_t usec)
{
    uint32_t ticks;
    if (usec)
    {
        ustimeout(&ticks, 0);
        while (!ustimeout(&ticks, usec)) ;
    }
}

// Return non-zero if timeout
int ustimeout(uint32_t *tickp, int usec)
{
    uint32_t t = time_us_32();
    uint32_t dt=t - *tickp;

    if (usec == 0 || dt >= usec)
    {
        *tickp = t;
        return (1);
    }
    return (0);
}

// EOF
