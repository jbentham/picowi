// PicoWi Pico low-level definitions, see https://iosoft.blog/picowi
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

#define USE_GPIO_REGS   1           // Set non-zero for direct register access
                                    // (boosts SPI from 2 to 5.4 MHz read, 7.8 MHz write)
#define SD_CLK_DELAY    0           // Clock on/off delay time in usec
#define USE_PIO         1           // Set non-zero to use Pico PIO for SPI
#define USE_PIO_DMA     1           // Set non-zero to use PIO DMA
#define PIO_SPI_FREQ    40000000    // SPI frequency if using PIO
#define SD_IRQ_ASSERT   1           // State of IRQ pin when asserted

// Use definition in CMakeLists to switch between CY4343W and CYW43439

#if CHIP_4343W == 1
// Pico Murata 1DX CYW4343W add-on module pinout
#define SD_D0_PIN       16
#define SD_D1_PIN       17
#define SD_D2_PIN       18
#define SD_D3_PIN       19
#define SD_CMD_PIN      20
#define SD_CLK_PIN      21
#define SD_ON_PIN       22
#define SD_DIN_PIN      SD_D0_PIN
#define SD_CS_PIN       SD_D3_PIN
#define SD_IRQ_PIN      SD_D1_PIN
#define SD_LED_GPIO     4
#else
// Pi Pico CYW43439 pinout
#define SD_ON_PIN       23
#define SD_CMD_PIN      24
#define SD_DIN_PIN      24
#define SD_D0_PIN       24
#define SD_CS_PIN       25
#define SD_CLK_PIN      29
#define SD_IRQ_PIN      24
#define SD_LED_GPIO     0
#endif

// I/O pin configuration
#define IO_IN         0
#define IO_OUT        1
#define IO_NOPULL     0
#define IO_PULLDN     1
#define IO_PULLUP     2

// SD function numbers
#define SD_FUNC_BUS         0
#define SD_FUNC_BAK         1
#define SD_FUNC_RAD         2
// Fake function number, used on startup when bus data is swapped
#define SD_FUNC_SWAP        4
#define SD_FUNC_MASK        (SD_FUNC_SWAP - 1)
#define SD_FUNC_BUS_SWAP    (SD_FUNC_BUS | SD_FUNC_SWAP)

// Read/write flags
#define SD_RD           0
#define SD_WR           1

// Swap bytes in 16 or 32-bit value
#define SWAP16(x) ((x&0xff)<<8 | (x&0xff00)>>8)
#define SWAP32(x) ((x&0xff)<<24 | (x&0xff00)<<8 | (x&0xff0000)>>8 | (x&0xff000000)>>24)

// Swap bytes in two 16-bit values
#define SWAP16_2(x) ((((x) & 0xff000000) >> 8) | (((x) & 0xff0000) << 8) | \
                    (((x) & 0xff00) >> 8)      | (((x) & 0xff) << 8))

#if USE_GPIO_REGS
#define GPIO_IN_REG         (volatile uint32_t *)0xd0000004
#define GPIO_SET_REG        (volatile uint32_t *)0xd0000014
#define GPIO_CLR_REG        (volatile uint32_t *)0xd0000018
#define IO_WR(pin, val)     if (val) *GPIO_SET_REG = (1 << pin); else *GPIO_CLR_REG = (1 << pin)
#define IO_WR_HI(pin)       *GPIO_SET_REG = (1 << pin)
#define IO_WR_LO(pin)       *GPIO_CLR_REG = (1 << pin)
#define IO_RD(pin)          ((*GPIO_IN_REG >> pin) & 1)
#else
#define IO_WR(pin, val)     io_out(pin, val)
#define IO_WR_HI(pin)       io_out(pin, 1)
#define IO_WR_LO(pin)       io_out(pin, 0)
#define IO_RD(pin)          io_in(pin)
#endif

void io_init(void);
void io_set(int pin, int mode, int pull);
void io_mode(int pin, int mode);
void io_pull(int pin, int pull);
void io_strength(int pin, uint32_t strength);
void io_slew(int pin, uint32_t slew);
void io_out(int pin, int val);
extern uint8_t io_in(int pin);
uint32_t ustime(void);
void usdelay(uint32_t usec);
int ustimeout(uint32_t *tickp, int usec);

// EOF
