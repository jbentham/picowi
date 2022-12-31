// PicoWi half-duplex SPI interface, see https://iosoft.blog/picowi
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

#include "picowi_defs.h"
#include "picowi_pico.h"
#include "picowi_spi.h"
#include "picowi_init.h"
#include "picowi_regs.h"
#include "picowi_pio.h"

// Set up the SPI WiFi interface
int wifi_setup(void)
{
    io_set(SD_ON_PIN, IO_OUT, IO_NOPULL);
    io_out(SD_ON_PIN, 0);
    io_set(SD_CS_PIN, IO_OUT, IO_NOPULL);
    io_out(SD_CS_PIN, 1);
    io_set(SD_CLK_PIN, IO_OUT, IO_NOPULL);
    io_out(SD_CLK_PIN, 0);
    io_set(SD_CMD_PIN, IO_OUT, IO_NOPULL);
    io_out(SD_CMD_PIN, 0);
#if CHIP_4343W
    io_set(SD_D2_PIN, IO_OUT, IO_NOPULL);
    io_out(SD_D2_PIN, 0);
#endif    
    usdelay(100000);
    io_out(SD_ON_PIN, 1);
    usdelay(50000);
    io_set(SD_CMD_PIN, IO_IN, IO_PULLUP);
    io_set(SD_IRQ_PIN, IO_IN, IO_NOPULL);
    return(spi_init());
}

// Set up SPI interface
int spi_init(void)
{
    int ok=0;
    uint32_t val=0;
    
#if USE_PIO
    pio_init();
#endif    
    for (int i=0; i<4 && !ok; i++)
    {
        usdelay(2000);
        val = wifi_reg_read(SD_FUNC_BUS_SWAP, 0x14, 4);
        ok = (val == SPI_TEST_VALUE);
    }
    if (!ok)
        printf("Error: SPI test pattern %08lX\n", val);
    else
    {
        wifi_reg_read(SD_FUNC_BUS_SWAP, SPI_BUS_CONTROL_REG, 4);
        // Configure SPI interface: data changes on rising clock edge
        //wifi_reg_write(SD_FUNC_BUS_SWAP, SPI_BUS_CONTROL_REG, 0x204b3, 4);
        wifi_reg_write(SD_FUNC_BUS_SWAP, SPI_BUS_CONTROL_REG, 0x200b3, 4);
        // Set SPI delay and interrupts
        wifi_reg_read(SD_FUNC_BUS, SPI_BUS_CONTROL_REG, 4);
        wifi_reg_read(SD_FUNC_BUS, SPI_BUS_CONTROL_REG, 4);
        wifi_reg_write(SD_FUNC_BUS, SPI_RESP_DELAY_F1_REG, 0x04, 1);
        wifi_reg_write(SD_FUNC_BUS, SPI_INTERRUPT_REG, 0x0099, 2);
        wifi_reg_write(SD_FUNC_BUS, SPI_INTERRUPT_ENABLE_REG, 0x00be, 2);
    }
    return(ok);
}

// Poll for WiFi receive data event, with timeout
bool wifi_rx_event_wait(int msec, uint8_t evt)
{
    bool ok;
    
    while (!(ok = (wifi_reg_read(SD_FUNC_BUS, BUS_SPI_STATUS_REG, 1) & evt) == evt) && msec--)
        usdelay(1000);
    return(ok);
}

// Read a register
uint32_t wifi_reg_read(int func, uint32_t addr, int nbytes)
{
    uint32_t ret = spi_reg_read(func, addr, nbytes);
    display(DISP_REG, "Rd_reg   len %u %s 0x%04lx -> 0x%02X\n", 
            nbytes, func_str(func), addr&SB_ADDR_MASK, ret);
    return(ret);
}
// Read a register using SPI
uint32_t spi_reg_read(int func, uint32_t addr, int nbytes)
{
    uint32_t val=0;

    wifi_data_read(func, addr, (uint8_t *)&val, nbytes);
    return(func&SD_FUNC_SWAP && nbytes>1 ? SWAP16_2(val) : val);
}

// Write a register
int wifi_reg_write(int func, uint32_t addr, uint32_t val, int nbytes)
{
    int ret = spi_reg_write(func, addr, val, nbytes);
    display(DISP_REG, "Wr_reg   len %u %s 0x%04lx <- 0x%02X\n",
            nbytes, func_str(func), addr&SB_ADDR_MASK, val);
    return(ret);
}
// Write a register using SPI
int spi_reg_write(int func, uint32_t addr, uint32_t val, int nbytes)
{
    if (func&SD_FUNC_SWAP && nbytes>1)
        val = SWAP16_2(val);
    return(wifi_data_write(func, addr, (uint8_t *)&val, nbytes));
}

// Read data block using SPI
int wifi_data_read(int func, int addr, uint8_t *dp, int nbytes)
{
    SPI_MSG msg={.hdr = {.wr=SD_RD, .incr=1, .func=func&SD_FUNC_MASK,
                         .addr=addr, .len=nbytes}};
    U32DATA dat={.uint32=0};

#if !USE_PIO
    io_mode(SD_CMD_PIN, IO_OUT);
#endif    
    if (func & SD_FUNC_SWAP)
        msg.vals[0] = SWAP16_2(msg.vals[0]);
    else if (func == SD_FUNC_BAK)
        msg.hdr.len += 4;
    else if (func == SD_FUNC_RAD)
        nbytes = (nbytes + 4) & ~3;
    io_out(SD_CS_PIN, 0);
    usdelay(SD_CLK_DELAY);
    spi_write((uint8_t *)&msg, 32);
#if !USE_PIO
    io_mode(SD_CMD_PIN, IO_IN);
#endif    
    usdelay(SD_CLK_DELAY);
    if (func == SD_FUNC_BAK)
        spi_read(dat.bytes, 32);
    usdelay(SD_CLK_DELAY);
    dat.uint32 = 0;
    spi_read(dp, nbytes*8);
    usdelay(SD_CLK_DELAY);
    io_out(SD_CS_PIN, 1);
    return(nbytes);
}

// Write a data block using SPI
int wifi_data_write(int func, int addr, uint8_t *dp, int nbytes)
{
    SPI_MSG msg={.hdr = {.wr=SD_WR, .incr=1, .func=func&SD_FUNC_MASK,
                         .addr=addr, .len=nbytes}};

    if (func & SD_FUNC_SWAP)
        msg.vals[0] = SWAP16_2(msg.vals[0]);
#if !USE_PIO
    io_mode(SD_CMD_PIN, IO_OUT);
#endif    
    io_out(SD_CS_PIN, 0);
    usdelay(SD_CLK_DELAY);
    if (nbytes <= 4)
    {
        memcpy(&msg.bytes[4], dp, nbytes);
        spi_write((uint8_t *)&msg, 64);
    }
    else
    {
        spi_write((uint8_t *)&msg, 32);
        usdelay(SD_CLK_DELAY);
        spi_write(dp, nbytes*8);
    }
    usdelay(SD_CLK_DELAY);
    io_out(SD_CS_PIN, 1);
#if !USE_PIO
    io_mode(SD_CMD_PIN, IO_IN);
#endif    
    usdelay(SD_CLK_DELAY);
    return(nbytes);
}

// Read data from SPI interface
int spi_read(uint8_t *data, int nbits)
{
#if USE_PIO
    uint8_t *start=data;
    pio_spi_read(data, nbits/8);
#else
    uint8_t b, *start=data;
    int n=0;

    data--;
    while (n < nbits)
    {
        b = IO_RD(SD_DIN_PIN);
        IO_WR_HI(SD_CLK_PIN);
        //b = IO_RD(SD_DIN_PIN);
#if SD_CLK_DELAY
        usdelay(SD_CLK_DELAY);
#endif        
        if ((n++ & 7) == 0)
            *++data = 0;        
        *data = (*data << 1) | b;
        IO_WR_LO(SD_CLK_PIN);
#if SD_CLK_DELAY
        usdelay(SD_CLK_DELAY);
#endif        
    }
#endif    
    display(DISP_SPI, "Rd_SPI ");
    disp_bytes(DISP_SPI, start, nbits/8);
    display(DISP_SPI, "\n");
    return(nbits);
}

// Write data to SPI interface
void spi_write(uint8_t *data, int nbits)
{
#if USE_PIO
    uint8_t *start=data;
    pio_spi_write(data, nbits/8);
#else
    uint8_t b=0, *start=data;
    int n=0;

    io_mode(SD_CMD_PIN, IO_OUT);
    usdelay(SD_CLK_DELAY);
    b = *data++;
    while (n < nbits)
    {
        IO_WR(SD_CMD_PIN, b & 0x80);
#if SD_CLK_DELAY
        usdelay(SD_CLK_DELAY);
#endif        
        IO_WR_HI(SD_CLK_PIN);
        b <<= 1;
        if ((++n & 7) == 0)
            b = *data++;
#if SD_CLK_DELAY
        usdelay(SD_CLK_DELAY);
#endif        
        IO_WR_LO(SD_CLK_PIN);
    }
    usdelay(SD_CLK_DELAY);
    io_mode(SD_CMD_PIN, IO_IN);
    usdelay(SD_CLK_DELAY);
#endif    
    display(DISP_SPI, "Wr_SPI ");
    disp_bytes(DISP_SPI, start, nbits/8);
    display(DISP_SPI, "\n");
}

// Get state of IRQ pin
bool get_irq(void)
{
#if SD_IRQ_ASSERT
    return(io_in(SD_IRQ_PIN));
#else
    return(!io_in(SD_IRQ_PIN));
#endif    
}

// Return string with function name
char *func_str(int func)
{
    func &= SD_FUNC_MASK;
    return(func==SD_FUNC_BUS ? "Bus" :
           func==SD_FUNC_BAK ? "Bak" :
           func==SD_FUNC_RAD ? "Rad" : "???");
}

// EOF
