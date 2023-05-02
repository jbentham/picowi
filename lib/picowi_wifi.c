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

#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <hardware/structs/pio.h>
#include <hardware/dma.h>
#include "picowi_defs.h"
#include "picowi_pico.h"
#include "picowi_init.h"
#include "picowi_regs.h"
#include "picowi_wifi.h"
#include "picowi_pio.pio.h"

PIO wifi_pio = pio0;
uint wifi_sm;
io_rw_8 *wifi_rxfifo, *wifi_txfifo;
uint wifi_rx_dma_chan, wifi_tx_dma_chan;
uint wifi_tx_dma_dreq, wifi_rx_dma_dreq;

extern int display_mode;

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
    usdelay(100000);
    io_out(SD_ON_PIN, 1);
    usdelay(50000);
    io_set(SD_CMD_PIN, IO_IN, IO_PULLUP);
    io_set(SD_IRQ_PIN, IO_IN, IO_NOPULL);
    return (wifi_start());
}

// Start up the WiFi interface
int wifi_start(void)
{
    int ok = 0;
    uint32_t val = 0;
    //uint8_t data[20];

#if USE_PIO
    wifi_pio_init();
#endif
    for (int i = 0; i < 4 && !ok; i++)
    {
        usdelay(2000);
        val = wifi_reg_read(SD_FUNC_BUS_SWAP, 0x14, 4);
        ok = (val == SPI_TEST_VALUE);
    }
    if (!ok)
        printf("Error: SPI test pattern %08lX\n", val);
    else
    {
        printf("Detected WiFi chip\n");
        // Rx data changes on rising clock edge (default)
        wifi_reg_write(SD_FUNC_BUS_SWAP, SPI_BUS_CONTROL_REG, 0x200b3, 4);
        // Rx data changes on falling clock edge
        //wifi_reg_write(SD_FUNC_BUS_SWAP, SPI_BUS_CONTROL_REG, 0x200a3, 4);
        val = wifi_reg_read(SD_FUNC_BUS, 0x14, 4);
        ok = (val == SPI_TEST_VALUE);
        if (!ok)
            printf("Error: SPI swap pattern %08lX\n", val);
#if 1
        // Set WiFi chip SPI delay and interrupts
        wifi_reg_read(SD_FUNC_BUS, SPI_BUS_CONTROL_REG, 4);
        wifi_reg_read(SD_FUNC_BUS, SPI_BUS_CONTROL_REG, 4);
        wifi_reg_write(SD_FUNC_BUS, SPI_RESP_DELAY_F1_REG, 0x04, 1);
        wifi_reg_write(SD_FUNC_BUS, SPI_INTERRUPT_REG, 0x0099, 2);
        wifi_reg_write(SD_FUNC_BUS, SPI_INTERRUPT_ENABLE_REG, 0x00be, 2);
#else        
        n = wifi_data_read(SD_FUNC_BUS, 0x14, data, 4);
        disp_bytes(0, data, n);
        printf("\n");
#endif
    }
    return (ok);
}

// Initialse PIO
void wifi_pio_init(void) 
{
    wifi_sm = pio_claim_unused_sm(wifi_pio, true);
    wifi_txfifo = (io_rw_8 *) &wifi_pio->txf[0];
    uint offset = pio_add_program(wifi_pio, &picowi_pio_program);
    pio_sm_config c = picowi_pio_program_get_default_config(offset);
    // Set I/O pins to be controlled
    pio_gpio_init(wifi_pio, SD_CLK_PIN);
    pio_gpio_init(wifi_pio, SD_CMD_PIN);
    pio_sm_set_consecutive_pindirs(wifi_pio, wifi_sm, SD_CLK_PIN, 1, true);
    pio_sm_set_consecutive_pindirs(wifi_pio, wifi_sm, SD_CMD_PIN, 1, true);
    // Configure data pin as I/O, clock pin as O/P (sideset)
    sm_config_set_out_pins(&c, SD_CMD_PIN, 1);
    sm_config_set_in_pins(&c, SD_CMD_PIN);
    sm_config_set_sideset_pins(&c, SD_CLK_PIN);
    // Get 8 bits from FIFOs, disable auto-pull & auto-push
    sm_config_set_out_shift(&c, false, false, 8);
    sm_config_set_in_shift(&c, false, false, 8);
    // Set data rate
#if PIO_SPI_FREQ >= 40000000
    sm_config_set_clkdiv_int_frac(&c, 1, 0);
#else   
    float div = (float)clock_get_hz(clk_sys) / (PIO_SPI_FREQ * 3.1);
    sm_config_set_clkdiv(&c, div);
#endif    
    pio_sm_init(wifi_pio, wifi_sm, offset, &c);
    pio_sm_clear_fifos(wifi_pio, wifi_sm);
    pio_sm_set_enabled(wifi_pio, wifi_sm, true);
#if USE_PIO_DMA
    wifi_pio_dma_init();
#endif
}

// Read a register
uint32_t wifi_reg_read(int func, uint32_t addr, int nbytes)
{
    uint32_t val, ret;
    
    wifi_data_read(func, addr, (uint8_t *)&val, nbytes);
    ret = func&SD_FUNC_SWAP && nbytes > 1 ? SWAP16_2(val) : val;
    display(DISP_REG,
        "Rd_reg   len %u %s 0x%04lx -> 0x%02lX\n", 
        nbytes,
        wifi_func_str(func),
        addr&SB_ADDR_MASK,
        ret);
    return (ret);
}

// Write a register
int wifi_reg_write(int func, uint32_t addr, uint32_t val, int nbytes)
{
    int ret;
    
    if (func & SD_FUNC_SWAP && nbytes > 1)
        val = SWAP16_2(val);
    ret = wifi_data_write(func, addr, (uint8_t *)&val, nbytes);
    display(DISP_REG,
        "Wr_reg   len %u %s 0x%04lx <- 0x%02lX\n",
        nbytes,
        wifi_func_str(func),
        addr&SB_ADDR_MASK,
        val);
    return (ret);
}

// Read data block using SPI
int wifi_data_read(int func, int addr, uint8_t *dp, int nbytes)
{
    SPI_MSG msg = {
        .hdr = { 
        .wr = SD_RD,
        .incr = 1,
        .func = func&SD_FUNC_MASK,
        .addr = addr,
        .len = nbytes 
    } 
    };
    U32DATA dat = { .uint32 = 0 };

#if !USE_PIO
    io_mode(SD_CMD_PIN, IO_OUT);
#endif    
    if (func & SD_FUNC_SWAP)
        msg.vals[0] = SWAP16_2(msg.vals[0]);
    else if (func == SD_FUNC_BAK)
        msg.hdr.len += 4;
    else if (func == SD_FUNC_RAD)
        nbytes = (nbytes + 3) & ~3;
    io_out(SD_CS_PIN, 0);
    wifi_spi_write((uint8_t *)&msg, 32);
#if !USE_PIO
    io_mode(SD_CMD_PIN, IO_IN);
#endif    
    if (func == SD_FUNC_BAK)
        wifi_spi_read(dat.bytes, 32);
    dat.uint32 = 0;
    wifi_spi_read(dp, nbytes * 8);
    io_out(SD_CS_PIN, 1);
    return (nbytes);
}

// Write a data block using SPI
int wifi_data_write(int func, int addr, uint8_t *dp, int nbytes)
{
    SPI_MSG msg = {
        .hdr = {
         .wr = SD_WR,
        .incr = 1,
        .func = func&SD_FUNC_MASK,
        .addr = addr,
        .len = nbytes
    }
    };

    if (func & SD_FUNC_SWAP)
        msg.vals[0] = SWAP16_2(msg.vals[0]);
#if !USE_PIO
    io_mode(SD_CMD_PIN, IO_OUT);
#endif    
    io_out(SD_CS_PIN, 0);
    if (nbytes <= 4)
    {
        memcpy(&msg.bytes[4], dp, nbytes);
        wifi_spi_write((uint8_t *)&msg, 64);
    }
    else
    {
        wifi_spi_write((uint8_t *)&msg, 32);
        wifi_spi_write(dp, nbytes * 8);
    }
    io_out(SD_CS_PIN, 1);
#if !USE_PIO
    io_mode(SD_CMD_PIN, IO_IN);
#endif    
    return (nbytes);
}

// Read data block from SPI interface
void wifi_spi_read(uint8_t *dp, int nbits)
{
#if !USE_PIO
    wifi_bb_spi_read(dp, nbits);
#else
    int rxlen = nbits / 8;
    int reader = PIO_SPI_FREQ >= 30000000 ? picowi_pio_offset_reader : picowi_pio_offset_slow_reader;
    pio_sm_exec(wifi_pio, wifi_sm, pio_encode_jmp(reader));
#if USE_PIO_DMA
    dma_channel_transfer_to_buffer_now(wifi_rx_dma_chan, dp, nbits / 8);
    pio_sm_put(wifi_pio, wifi_sm, rxlen - 1);
    dma_channel_wait_for_finish_blocking(wifi_rx_dma_chan);
#else    
    pio_sm_put(wifi_pio, wifi_sm, rxlen - 1);
    while (rxlen > 0)
    {
        if (!pio_sm_is_rx_fifo_empty(wifi_pio, wifi_sm))
        {
            *dp++ = pio_sm_get(wifi_pio, wifi_sm);
            rxlen--;
        }
    }
#endif    
#endif    
}

// Write data block to SPI interface
void wifi_spi_write(uint8_t *dp, int nbits)
{
#if !USE_PIO
    wifi_bb_spi_write(dp, nbits);
#else
    pio_sm_clear_fifos(wifi_pio, wifi_sm);
    pio_sm_exec(wifi_pio, wifi_sm, pio_encode_jmp(picowi_pio_offset_writer));
    pio_sm_set_consecutive_pindirs(wifi_pio, wifi_sm, SD_CMD_PIN, 1, true);
#if USE_PIO_DMA
    dma_channel_transfer_from_buffer_now(wifi_tx_dma_chan, dp, nbits / 8);
    dma_channel_wait_for_finish_blocking(wifi_tx_dma_chan);
#else    
    int n = 0;
    while (n < nbits)
    {
        if (!pio_sm_is_tx_fifo_full(wifi_pio, wifi_sm))
        {
            *wifi_txfifo = *dp++;
            n += 8;
        }
    }
#endif    
    while (!pio_sm_is_tx_fifo_empty(wifi_pio, wifi_sm)) ;
    while (wifi_pio->sm[wifi_sm].addr != picowi_pio_offset_writer) ;
    pio_sm_set_consecutive_pindirs(wifi_pio, wifi_sm, SD_CMD_PIN, 1, false);
    pio_sm_exec(wifi_pio, wifi_sm, pio_encode_jmp(picowi_pio_offset_stall));
#endif    
}

// Poll for WiFi receive data event, with timeout
bool wifi_rx_event_wait(int msec, uint8_t evt)
{
    bool ok;
    
    while (!(ok = (wifi_reg_read(SD_FUNC_BUS, BUS_SPI_STATUS_REG, 1) & evt) == evt) && msec--)
        usdelay(1000);
    return (ok);
}

// Initialise PIO DMA channels
void wifi_pio_dma_init(void)
{
    dma_channel_config cfg;
    
    wifi_tx_dma_dreq = pio_get_dreq(wifi_pio, wifi_sm, true);
    wifi_tx_dma_chan = dma_claim_unused_channel(true);
    cfg = dma_channel_get_default_config(wifi_tx_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    channel_config_set_dreq(&cfg, wifi_tx_dma_dreq);
    dma_channel_configure(wifi_tx_dma_chan, &cfg, &wifi_pio->txf[wifi_sm], NULL, 8, false);
    
    wifi_rx_dma_dreq = pio_get_dreq(wifi_pio, wifi_sm, false);
    wifi_rx_dma_chan = dma_claim_unused_channel(true);
    cfg = dma_channel_get_default_config(wifi_rx_dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, wifi_rx_dma_dreq);
    dma_channel_configure(wifi_rx_dma_chan, &cfg, NULL, &wifi_pio->rxf[wifi_sm], 8, false);
}

// Read data from SPI interface, using bit-bash
int wifi_bb_spi_read(uint8_t *data, int nbits)
{
    uint8_t b, *start = data;
    int n = 0;

    data--;
    while (n < nbits)
    {
        b = IO_RD(SD_DIN_PIN);
        IO_WR_HI(SD_CLK_PIN);
        //b = IO_RD(SD_DIN_PIN);
        if ((n++ & 7) == 0)
            * ++data = 0;        
        *data = (*data << 1) | b;
        IO_WR_LO(SD_CLK_PIN);
    }
    display(DISP_SPI, "Rd_SPI ");
    disp_bytes(DISP_SPI, start, nbits / 8);
    display(DISP_SPI, "\n");
    return (nbits);
}

// Write data to SPI interface, using bit-bash
void wifi_bb_spi_write(uint8_t *data, int nbits)
{
    uint8_t b = 0, *start = data;
    int n = 0;

    io_mode(SD_CMD_PIN, IO_OUT);
    b = *data++;
    while (n < nbits)
    {
        IO_WR(SD_CMD_PIN, b & 0x80);
        IO_WR_HI(SD_CLK_PIN);
        b <<= 1;
        if ((++n & 7) == 0)
            b = *data++;
        IO_WR_LO(SD_CLK_PIN);
    }
    io_mode(SD_CMD_PIN, IO_IN);
    display(DISP_SPI, "Wr_SPI ");
    disp_bytes(DISP_SPI, start, nbits / 8);
    display(DISP_SPI, "\n");
}

// Get state of IRQ pin
bool wifi_get_irq(void)
{
#if SD_IRQ_ASSERT
    return (io_in(SD_IRQ_PIN));
#else
    return (!io_in(SD_IRQ_PIN));
#endif    
}

// Return string with function name
char *wifi_func_str(int func)
{
    func &= SD_FUNC_MASK;
    return (func == SD_FUNC_BUS ? "Bus" :
           func == SD_FUNC_BAK ? "Bak" :
           func == SD_FUNC_RAD ? "Rad" : "???");
}

// EOF
