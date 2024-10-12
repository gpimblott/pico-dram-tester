#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define PIN_SPI_CS 17
#define PIN_SPI_SCK 18
#define PIN_SPI_DO 19
#define PIN_SPI_DC 16

#define PIN_7789_RST 20

#define RESET_DELAY 140

#define CMD_NOP      0x00
#define CMD_SWRESET  0x01
#define CMD_SLPIN    0x10
#define CMD_SLPOUT   0x11
#define CMD_NORON    0x13
#define CMD_INVOFF   0x20
#define CMD_INVON    0x21
#define CMD_DISPOFF  0x28
#define CMD_DISPON   0x29
#define CMD_CASET    0x2A
#define CMD_RASET    0x2B
#define CMD_RAMWR    0x2C
#define CMD_MADCTL   0x36
#define CMD_COLMOD   0x3A

// Display corner isn't always 0,0
static uint16_t _x_offset = 0;
static uint16_t _y_offset = 0;

// Mode for transmitting a command
static inline void mode_cmd()
{
    gpio_put(PIN_SPI_DC, 0);
}

// Mode for transmitting data
static inline void mode_data()
{
    gpio_put(PIN_SPI_DC, 1);
}

// Select the device
static inline void cs_low()
{
    asm volatile("nop \n nop \n nop");
    gpio_put (PIN_SPI_CS, 0);
    asm volatile("nop \n nop \n nop");
}

// Deselect the device
static inline void cs_high()
{
    asm volatile("nop \n nop \n nop");
    gpio_put (PIN_SPI_CS, 1);
    asm volatile("nop \n nop \n nop");
}

// Write a command byte
static void write_command(uint8_t cmd, uint8_t num_bytes, uint8_t buf[], bool cs)
{
    if (cs) cs_low();
    mode_cmd();
    // Write command byte
    spi_write_blocking(spi0, &cmd, 1);
    mode_data();
    // Write data bytes
    if (num_bytes > 0) {
        spi_write_blocking(spi0, buf, num_bytes);
    }
    if (cs) cs_high();
}

// Write a 16-bit value
static void write_data16(int num_words, uint16_t *buf)
{
    hw_write_masked(&spi_get_hw(spi0)->cr0, 15 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    spi_write16_blocking(spi0, buf, num_words);
    hw_write_masked(&spi_get_hw(spi0)->cr0, 7 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
}

static void st7789_gpio_init()
{
    // Set up GPIO for SPI0
    spi_init(spi0, 30 * 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_DO, GPIO_FUNC_SPI);

    gpio_init(PIN_SPI_CS);
    gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
    gpio_put(PIN_SPI_CS, 1);

    gpio_init(PIN_SPI_DC);
    gpio_set_dir(PIN_SPI_DC, GPIO_OUT);
    gpio_put(PIN_SPI_DC, 0);

    gpio_init(PIN_7789_RST);
    gpio_set_dir(PIN_7789_RST, GPIO_OUT);
    gpio_put(PIN_7789_RST, 0);
    sleep_ms(RESET_DELAY);
    gpio_put(PIN_7789_RST, 1);

}

// Set the window size for data writes
static inline void st7789_window(uint16_t x, uint16_t y, uint16_t width, uint16_t height, bool cs)
{
    uint16_t sx = x + _x_offset;
    uint16_t sy = y + _y_offset;
    uint16_t ex = x + width - 1 + _x_offset;
    uint16_t ey = y + height - 1 + _y_offset;
    write_command(CMD_CASET, 4, (uint8_t []){sy >> 8, sy & 0xff, ey >> 8, ey & 0xff}, cs);
    write_command(CMD_RASET, 4, (uint8_t []){sx >> 8, sx & 0xff, ex >> 8, ex & 0xff}, cs);
}

// Low level display initialization.
void st7789_disp_init(uint16_t xoff, uint16_t yoff, uint16_t width, uint16_t height)
{
    _x_offset = xoff;
    _y_offset = yoff;
    write_command(CMD_SWRESET, 0, NULL, true);                   // Software reset
    sleep_ms(120);
    write_command(CMD_SLPOUT, 0, NULL, true);                    // Exit sleep mode
    sleep_ms(5);
    write_command(CMD_COLMOD, 1, (uint8_t []){ 0x55 }, true);    // Interface format: 16 bit color
    write_command(CMD_MADCTL, 1, (uint8_t []){ 0x48 }, true);    // Memory address control: BGR
    st7789_window(0, 0, width, height, true);                    // Set display size
    write_command(CMD_INVON, 0, NULL, true);                     // Display inversion on
    write_command(CMD_NORON, 0, NULL, true);                     // Partial off = normal
    write_command(CMD_DISPON, 0, NULL, true);                    // Display on
}

// Draw a filled rectangle.
void st7789_fill (uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t col)
{
    int count;
    cs_low();
    st7789_window(sx, sy, width, height, false);
    write_command(CMD_RAMWR, 0, NULL, false);
    for (count = 0; count < width * height; count++) {
        write_data16(1, &col);
    }
    cs_high();
}

// Plot a single pixel. Inefficient!
static void pset(uint16_t x, uint16_t y, uint16_t col)
{
    uint8_t db[] = {0, 0, 0, 0};
    cs_low();
    st7789_window(x, y, 1, 1, false);
    write_command(CMD_RAMWR, 0, NULL, false);
    write_data16(1, &col);
    cs_high();
}

// Initialize the display
void st7789_init()
{
    uint16_t count;
    uint16_t col;
    st7789_gpio_init();
    st7789_disp_init(40, 53, 240, 135);
    st7789_fill(0, 0, 240, 135, 0x0040); // Clear screen

    // Pixel format: BGR (15-0) 5:6:5.
    for (count = 0; count < 64; count++) {
        st7789_fill(0, count, 100, 1, count << 11); // horizontal line
    }

    for (int count = 0; count < 60; count++) {
        pset(count, count, 0xFFFF);
    }
}
