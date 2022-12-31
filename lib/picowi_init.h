// PicoWi initialisation definitions, see http://iosoft.blog/picowi for details
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

uint32_t wifi_chip_id(void);
bool wifi_init(void);
bool wifi_core_reset(bool ram);
void init_powersave(void);
int wifi_data_load(int func, uint32_t dest, const unsigned char *data, int len);
int wifi_clm_load(const unsigned char *data, int len);
bool wifi_reg_val_wait(int ms, int func, int addr, uint32_t mask, uint32_t val, int nbytes);
bool wifi_reg_read_check(int func, int addr, uint32_t mask, uint32_t val, int nbytes);
void wifi_bak_window(uint32_t addr);
int wifi_bak_reg_write(uint32_t addr, uint32_t val, int nbytes);
int wifi_bak_reg_read(uint32_t addr, uint32_t *valp, int nbytes);
void wifi_set_led(bool on);
char *mac_addr_str(char *s, uint8_t *mac);
void disp_bytes(int mask, uint8_t *data, int len);
void display(int mode, const char* fmt, ...);
void set_display_mode(int mask);

// EOF
