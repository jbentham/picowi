// PicoWi camera interface, see https://iosoft.blog/picowi
//
// Copyright (c) 2023, Jeremy P Bentham
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
#include <string.h>
#include "hardware/i2c.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include <hardware/dma.h>

#include "../lib/picowi_defs.h"
#include "../lib/picowi_pico.h"
#include "../lib/picowi_init.h"
#include "cam_2640.h"

#define CAM_SPI         spi0
#define CAM_SPI_FREQ    8000000
#define CAM_PIN_SCK     2
#define CAM_PIN_MOSI    3
#define CAM_PIN_MISO    4
#define CAM_PIN_CS      5

#define CAM_I2C         i2c0
#define CAM_I2C_ADDR    0x30
#define CAM_I2C_FREQ    100000
#define CAM_PIN_SDA     8
#define CAM_PIN_SCL     9

const BYTE ov2640_jpeg_init[][2]    = { OV2640_JPEG_INIT };
const BYTE ov2640_yuv422[][2]       = { OV2640_YUV422 };
const BYTE ov2640_jpeg[][2]         = { OV2640_JPEG };
const BYTE setres_jpeg[][2]         = { SETRES_JPEG };

BYTE cam_data[1024 * 160];

// Initialise camera interface
int cam_init(void)
{
    WORD w;
    int ret = 0;
    
    i2c_init(CAM_I2C, CAM_I2C_FREQ);
    gpio_set_function(CAM_PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(CAM_PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(CAM_PIN_SDA);
    gpio_pull_up(CAM_PIN_SCL);

    spi_init(CAM_SPI, CAM_SPI_FREQ);
    gpio_set_function(CAM_PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(CAM_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(CAM_PIN_MOSI, GPIO_FUNC_SPI);
    gpio_init(CAM_PIN_CS);
    gpio_set_dir(CAM_PIN_CS, GPIO_OUT);
    gpio_put(CAM_PIN_CS, 1);
    
    cam_sensor_write_reg(0xff, 0x01);
    w = ((WORD)cam_sensor_read_reg(0x0a) << 8) | cam_sensor_read_reg(0x0b);
    if (w != 0x2640 && w != 0x2641 && w != 0x2642)
        printf("Camera i2c error: ID %04X\n", w);
    else if ((cam_write_reg(0, 0x55), cam_read_reg(0) != 0x55) || (cam_write_reg(0, 0xaa), cam_read_reg(0) != 0xaa))
        printf("Camera SPI error\n");
    else
    {
        cam_sensor_init();
        ret = 1;
    }
    return (ret);
}

// Initialise camera sensor
void cam_sensor_init(void)
{
    cam_sensor_write_reg(0xff, 0x01);
    cam_sensor_write_reg(0x12, 0x80);
    cam_sensor_write_cfg((BYTE *)ov2640_jpeg_init);
    cam_sensor_write_cfg((BYTE *)ov2640_yuv422);
    cam_sensor_write_cfg((BYTE *)ov2640_jpeg);
    cam_sensor_write_reg(0xff, 0x01);
    cam_sensor_write_reg(0x15, 0x00);
    cam_sensor_write_cfg((BYTE *)setres_jpeg);
}

// Read single camera frame
int cam_capture_single(void)
{
    int tries = 1000, ret=0, n=0;
    
    cam_write_reg(4, 0x01);
    cam_write_reg(4, 0x02);
    while ((cam_read_reg(0x41) & 0x08) == 0 && tries)
    {
        usdelay(100);
        tries--;
    }
    if (tries)
        n = cam_read_fifo_len();
    //printf("Image size %d\n", n);
    if (n > 0 && n <= sizeof(cam_data))
    {
        cam_select();
        spi_read_blocking(CAM_SPI, 0x3c, cam_data, 1);
        spi_read_blocking(CAM_SPI, 0x3c, cam_data, n);
        cam_deselect();
        ret = n;
    }
    else if (n > 0)
        printf("Camera image too large (%d bytes)\n", n);
    else
        printf("No image from camera\n");
    return (ret);
}

// Read FIFO length
int cam_read_fifo_len(void)
{
    return (cam_read_reg(0x42) + (((int)cam_read_reg(0x43)) << 8) + 
        ((int)(cam_read_reg(0x44)&0x7f) << 16));
}

// Read camera sensor i2c register
BYTE cam_sensor_read_reg(BYTE reg)
{
    BYTE b;
    
    i2c_write_blocking(CAM_I2C, CAM_I2C_ADDR, &reg, 1, true);
    i2c_read_blocking(CAM_I2C, CAM_I2C_ADDR, &b, 1, false);
    return (b);
}

// Write camera sensor i2c register
void cam_sensor_write_reg(BYTE reg, BYTE b)
{
    BYTE d[2] = {reg, b};
    
    i2c_write_blocking(CAM_I2C, CAM_I2C_ADDR, d, 2, true);
}

// Write camera sensor configuration to i2c registers
void cam_sensor_write_cfg(BYTE *cfg)
{
    BYTE reg, val;
    while (((reg = cfg[0]) & (val = cfg[1])) != 0xff)
    {
        cam_sensor_write_reg(reg, val);
        cfg += 2;
        sleep_us(10000);
    }
}

// Read camera SPI register
BYTE cam_read_reg(BYTE addr)
{
    BYTE value = 0;
    
    addr &= 0x7f;
    cam_select();
    spi_write_blocking(CAM_SPI, &addr, 1);
    sleep_ms(10);
    spi_read_blocking(CAM_SPI, 0, &value, 1);
    cam_deselect();
    sleep_ms(10);
    return (value);
}

// Write camera SPI register
void cam_write_reg(BYTE addr, BYTE value)
{
    BYTE buf[2] = { addr | 0x80, value };
    
    cam_select();
    spi_write_blocking(CAM_SPI, buf, 2);
    cam_deselect();
    sleep_ms(10); 
}

// Select SPI interface
void cam_select(void) 
{
    asm volatile("nop \n nop \n nop");
    gpio_put(CAM_PIN_CS, 0); // Active low
    asm volatile("nop \n nop \n nop");
}

// Deselect SPI interface
void cam_deselect(void)
{
    asm volatile("nop \n nop \n nop");
    gpio_put(CAM_PIN_CS, 1);
    asm volatile("nop \n nop \n nop");
}


// EOF