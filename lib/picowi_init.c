// PicoWi device initialisation, see http://iosoft.blog/picowi for details
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
#include <stdarg.h>

#include "picowi_defs.h"
#include "picowi_pico.h"
#include "picowi_wifi.h"
#include "picowi_init.h"
#include "picowi_ioctl.h"
#include "picowi_event.h"
#include "picowi_regs.h"

// Build-time options; set non-zero to enable the option
#define USE_WIFI_REGS       1       // Use WiFi register accesses to set LED
#define DISPLAY_CLMVER      0       // Display CLM information

// Active Low Power (ALP) & High Throughput (HT) clock bits for CLOCK_CSR_REG
#define SD_ALP_REQ          0x08
#define SD_HT_REQ           0x10
#define SD_ALP_AVAIL        0x40
#define SD_HT_AVAIL         0x80

// Addresses to load firmware and NVRAM image
#define FW_BASE_ADDR        0
#define NVRAM_BASE_ADDR     0x7FCFC
#define MAX_LOAD_LEN        512

typedef struct {
    uint16_t flag;
    uint16_t type;
    uint32_t len;
    uint32_t crc;
} CLM_LOAD_HDR;

typedef struct {
    char req[8];
    CLM_LOAD_HDR hdr;
} CLM_LOAD_REQ;

uint8_t my_mac[6];
extern const unsigned int fw_nvram_len, fw_firmware_len, fw_clm_len;
extern const unsigned char fw_nvram_data[], fw_firmware_data[], fw_clm_data[];
int display_mode;
void set_my_mac(uint8_t *mac);

// Get the chip ID (should be 0xA9AF for CYW43439)
uint32_t wifi_chip_id(void)
{
    wifi_bak_window(BAK_BASE_ADDR);
    return(wifi_reg_read(SD_FUNC_BAK, SB_32BIT_WIN, 2));
}

// Initialise WiFi chip
bool wifi_init(void)
{
    uint32_t n;
    char temps[30];

    // Check Active Low Power (ALP) clock
    wifi_reg_write(SD_FUNC_BAK, BAK_CHIP_CLOCK_CSR_REG, SD_ALP_REQ, 1);
    if (!wifi_reg_val_wait(10, SD_FUNC_BAK, BAK_CHIP_CLOCK_CSR_REG,
                           SD_ALP_AVAIL, SD_ALP_AVAIL, 1))
        return(false);
    wifi_reg_write(SD_FUNC_BAK, BAK_CHIP_CLOCK_CSR_REG, 0x00, 1);
    wifi_core_reset(true);
    wifi_bak_reg_write(SRAM_BANKX_IDX_REG, 0x03, 4);
    wifi_bak_reg_write(SRAM_BANKX_PDA_REG, 0x00, 4);
    // Load firmware
    n = wifi_data_load(SD_FUNC_BAK, FW_BASE_ADDR, fw_firmware_data, fw_firmware_len);
    display(DISP_INFO, "Loaded firmware addr 0x%04x, len %lu bytes\n", FW_BASE_ADDR, n);
    usdelay(5000);
    // Load NVRAM
    n = wifi_data_load(SD_FUNC_BAK, NVRAM_BASE_ADDR, fw_nvram_data, fw_nvram_len);
    display(DISP_INFO, "Loaded NVRAM addr 0x%04X len %lu bytes\n", NVRAM_BASE_ADDR, n);
    n = ((~(fw_nvram_len / 4) & 0xffff) << 16) | (fw_nvram_len / 4);
    wifi_reg_write(SD_FUNC_BAK, SB_32BIT_WIN | (SB_32BIT_WIN-4), n, 4);
    // Reset, and wait for High Throughput (HT) clock ready
    wifi_core_reset(false);
    if (!wifi_reg_val_wait(50, SD_FUNC_BAK, BAK_CHIP_CLOCK_CSR_REG,
                              SD_HT_AVAIL, SD_HT_AVAIL, 1))
        return(false);
    // Wait for backplane ready
    if (!wifi_rx_event_wait(100, SPI_STATUS_F2_RX_READY))
        return(false);
    // Load CLM
    wifi_clm_load(fw_clm_data, fw_clm_len);
#if DISPLAY_CLMVER
    uint8_t data[300];
    ioctl_get_data("clmver", 30, data, sizeof(data)-1);
    printf("%s\n", data);
#endif
    n = ioctl_get_data("cur_etheraddr", 10, my_mac, 6);
    mac_addr_str(temps, my_mac);
    display(DISP_INFO, "MAC address %s\n", n ? temps : "unknown");
    return(true);
}

// Reset device core (ARM or RAM)
bool wifi_core_reset(bool ram)
{
    uint32_t val, base=ram ? RAM_CORE_ADDR : ARM_CORE_ADDR;

    wifi_bak_reg_read(base+AI_IOCTRL_OSET, &val, 1);
    wifi_bak_reg_write(base+AI_IOCTRL_OSET, 0x03, 1);
    wifi_bak_reg_read(base+AI_IOCTRL_OSET, &val, 1);
    wifi_bak_reg_write(base+AI_RESETCTRL_OSET, 0x00, 1);
    usdelay(1000);
    wifi_bak_reg_write(base+AI_IOCTRL_OSET, 0x01, 1);
    wifi_bak_reg_read(base+AI_IOCTRL_OSET, &val, 1);
    usdelay(1000);
    return(true);
}

// cyw43_wifi_pm: cyw43_ll_wifi_pm: power saving
void init_powersave(void)
{
    ioctl_set_uint32("pm2_sleep_ret", 10, 0xc8);
    ioctl_set_uint32("bcn_li_bcn", 10, 0x01);
    ioctl_set_uint32("bcn_li_dtim", 10, 0x01);
    ioctl_set_uint32("assoc_listen", 10, 0x0a);
    ioctl_wr_int32(WLC_SET_PM, 10, 0x02);
    //ioctl_wr_int32(WLC_SET_GMODE, 10, 0x01);
    //ioctl_wr_int32(WLC_SET_BAND, 10, 0x00);
}

// Load data block into WiFi chip (CPU firmware or NVRAM file)
int wifi_data_load(int func, uint32_t dest, const unsigned char *data, int len)
{
    int nbytes=0, n;
    uint32_t oset=0;

    wifi_bak_window(dest);
    dest &= SB_ADDR_MASK;
    while (nbytes < len)
    {
        if (oset >= SB_32BIT_WIN)
        {
            wifi_bak_window(dest+nbytes);
            oset -= SB_32BIT_WIN;
        }
        n = MIN(MAX_BLOCKLEN, len-nbytes);
        wifi_data_write(func, dest+oset, (uint8_t *)&data[nbytes], n);
        nbytes += n;
        oset += n;
    }
    return(nbytes);
}

// Load CLM
int wifi_clm_load(const unsigned char *data, int len)
{
    int nbytes=0, oset=0, n;
    CLM_LOAD_REQ clr = {.req="clmload", .hdr={.type=2, .crc=0}};

    while (nbytes < len)
    {
        n = MIN(MAX_LOAD_LEN, len-nbytes);
        clr.hdr.flag = 1<<12 | (nbytes?0:2) | (nbytes+n>=len?4:0);
        clr.hdr.len = n;
        ioctl_set_data2((void *)&clr, sizeof(clr), 1000, (void *)&data[oset], n);
        nbytes += n;
        oset += n;
    }
    return(nbytes);
}

// Check register value every msec, until correct or timeout
bool wifi_reg_val_wait(int ms, int func, int addr, uint32_t mask, uint32_t val, int nbytes)
{
    bool ok;

    while (!(ok=wifi_reg_read_check(func, addr, mask, val, nbytes)) && ms--)
        usdelay(1000);
    return(ok);
}

// Read & check a masked value, return zero if incorrect
bool wifi_reg_read_check(int func, int addr, uint32_t mask, uint32_t val, int nbytes)
{
    return((wifi_reg_read(func, addr, nbytes) & mask) == val);
}

// Set backplane window if address has changed
void wifi_bak_window(uint32_t addr)
{
    static uint32_t lastaddr=0;

    addr &= SB_WIN_MASK;
    if (addr != lastaddr)
        wifi_reg_write(SD_FUNC_BAK, BAK_WIN_ADDR_REG, addr>>8, 3);
    lastaddr = addr;
}

// Write a 1 - 4 byte value via the backplane window
int wifi_bak_reg_write(uint32_t addr, uint32_t val, int nbytes)
{
    wifi_bak_window(addr);
    addr |= nbytes==4 ? SB_32BIT_WIN : 0;
    return(wifi_reg_write(SD_FUNC_BAK, addr, val, nbytes));
}

// Read a 1 - 4 byte value via the backplane window
int wifi_bak_reg_read(uint32_t addr, uint32_t *valp, int nbytes)
{
    wifi_bak_window(addr);
    addr |= nbytes==4 ? SB_32BIT_WIN : 0;
    *valp = wifi_reg_read(SD_FUNC_BAK, addr, nbytes);
    return(nbytes);
}

// Set WiFi LED on or off
void wifi_set_led(bool on)
{
#if USE_WIFI_REGS
    static bool init=false;
    if (!init)
        wifi_bak_reg_write(BAK_GPIOOUTEN_REG, 1<<SD_LED_GPIO, 4);
    init = true;
    wifi_bak_reg_write(BAK_GPIOOUT_REG, on ? 1<<SD_LED_GPIO : 0, 4);
#else
    ioctl_set_intx2("gpioout", 10, 1<<SD_LED_GPIO, on ? 1<<SD_LED_GPIO : 0);
#endif
}

// Convert MAC address to string
char *mac_addr_str(char *s, uint8_t *mac)
{
    sprintf(s, "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    return(s);
}

// Dump data as hex
void disp_bytes(int mask, uint8_t *data, int len)
{
    if (mask == 0 || (mask & display_mode))
    {
        while (len--)
            printf("%02X ", *data++);
    }
}

// Display diagnostic data
void display(int mask, const char* fmt, ...)
{
    va_list args;

    if (mask == 0 || (mask & display_mode))
    {
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
    }
}

// Set display mode
void set_display_mode(int mask)
{
    display_mode = mask;
}

// EOF
