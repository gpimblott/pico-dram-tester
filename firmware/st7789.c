#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"

#include "st7789.h"

#include "rtc_image.h"
//#include "chip_icon.h"
//#include "prop_font.h"
//#include "sserif13.h"
//#include "sserif16.h"
//#include "sserif20.h"
//#include "widgets16.h"

#define PIN_SPI_CS 1
#define PIN_SPI_SCK 2
#define PIN_SPI_DO 3
#define PIN_SPI_DC 0

//#define PIN_7789_RST 6 // FIXME no more reset

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

// A few colors
#define COLOR_BLACK 0x0000
#define COLOR_DKBLUE 0xa800
#define COLOR_DKGREEN 0x0540
#define COLOR_DKCYAN 0xad40
#define COLOR_DKRED 0x0015
#define COLOR_DKMAGENTA 0xa815
#define COLOR_DKYELLOW 0x0555
#define COLOR_LTGRAY 0xad55
#define COLOR_DKGRAY 0x52aa
#define COLOR_BLUE 0xfaaa
#define COLOR_GREEN 0x57ea
#define COLOR_CYAN 0xffea
#define COLOR_RED 0x52bf
#define COLOR_MAGENTA 0xfabf
#define COLOR_YELLOW 0x57ff
#define COLOR_WHITE 0xffff

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
static void write_data16(size_t num_words, uint16_t *buf)
{
    hw_write_masked(&spi_get_hw(spi0)->cr0, 15 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    spi_write16_blocking(spi0, buf, num_words);
    hw_write_masked(&spi_get_hw(spi0)->cr0, 7 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
}

static void st7789_gpio_init()
{
    // Set up GPIO for SPI0
    spi_init(spi0, 70 * 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_DO, GPIO_FUNC_SPI);

    gpio_init(PIN_SPI_CS);
    gpio_set_dir(PIN_SPI_CS, GPIO_OUT);
    gpio_put(PIN_SPI_CS, 1);

    gpio_init(PIN_SPI_DC);
    gpio_set_dir(PIN_SPI_DC, GPIO_OUT);
    gpio_put(PIN_SPI_DC, 0);

//    gpio_init(PIN_7789_RST);
//    gpio_set_dir(PIN_7789_RST, GPIO_OUT);
//    gpio_put(PIN_7789_RST, 0);
    sleep_ms(RESET_DELAY);
//    gpio_put(PIN_7789_RST, 1);

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
void st7789_fill(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t col)
{
    int count;
    cs_low();
    st7789_window(sx, sy, width, height, false);
    write_command(CMD_RAMWR, 0, NULL, false);
    hw_write_masked(&spi_get_hw(spi0)->cr0, 15 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    for (count = 0; count < width * height; count++) {
        spi_write16_blocking(spi0, &col, 1);
    }
    hw_write_masked(&spi_get_hw(spi0)->cr0, 7 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    cs_high();
}

// Fill with 50% halftone
void st7789_halftone_fill(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t c1, uint16_t c2)
{
    int row, col;
    uint8_t rowstate = 0;
    uint8_t colstate = 0;
    for (row = 0; row < height; row++) {
        rowstate = ~rowstate;
        colstate = rowstate;
        cs_low();
        st7789_window(sx, sy + row, width, 1, false);
        write_command(CMD_RAMWR, 0, NULL, false);
        hw_write_masked(&spi_get_hw(spi0)->cr0, 15 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
        for (col = 0; col < width; col++) {
            colstate = ~colstate;
            spi_write16_blocking(spi0, (colstate) ? &c1 : &c2, 1);
        }
        hw_write_masked(&spi_get_hw(spi0)->cr0, 7 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
        cs_high();
    }
}

// Plots a bitmap. Must be 16bpp and match the display type (BGR 565)
void st7789_bitblt(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t *buf)
{
    cs_low();
    st7789_window(sx, sy, width, height, false);
    write_command(CMD_RAMWR, 0, NULL, false);
    write_data16(width * height, buf);
    cs_high();
}

// Rotates the bitmap using a trick. A single-line bitblt assumes the orientation of the line.
void st7789_bitblt_rot(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t *buf)
{
    for (int count = 0; count < height; count++) {
        st7789_bitblt(sx, sy + count, width, 1, &buf[width * count]);
    }
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

// Get the width of a string using a particular font
uint16_t font_string_width(char *text, const font_def_t *font, bool bold)
{
    uint16_t total_width = 0;

    while (*text) {
        if (*text >= font->count) {
            text++;
            continue;
        }
        total_width += font->widths[*(text++)] + (bold ? 1 : 0);
    }
    return total_width;
}

// Draws a string at the specific coordinates using the default font
void font_string(uint16_t x, uint16_t y, char *text,
                 uint16_t fg_color, uint16_t bg_color,
                 const font_def_t *font, bool bold)
{
    uint8_t row, col;
    uint16_t total_width, width;
    uint16_t offset;
    uint8_t bytes_column;
    uint8_t byte;
    uint8_t db;
    uint8_t col_count;
    char *text_buf = text;
    uint8_t prev_bit;

    // First, compute total width
    total_width = font_string_width(text, font, bold);

    for (row = 0; row < font->height; row++) {
        // Set the window
        cs_low();
        st7789_window(x, y + row, total_width, 1, false);
        write_command(CMD_RAMWR, 0, NULL, false);
        hw_write_masked(&spi_get_hw(spi0)->cr0, 15 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);

        text_buf = text;
        while (*text_buf) {
            // Get information about this character
            if (*text_buf >= font->count) {
                text_buf++; // Skip if invalid
                continue;
            }
            width = font->widths[*text_buf];
            bytes_column = (width + 7) >> 3;
            offset = font->offsets[*text_buf];
            // Get exact byte offset into the character table
            offset += row * bytes_column;
            prev_bit = 0;
            for (byte = 0; byte < bytes_column; byte++) {
                db = font->data[offset + byte];
                col_count = (width > 8) ? 8 : width;
                for (col = 0; col < col_count; col++) {
                    if ((db & 0x1) || (bold && (prev_bit == 1))) {
                        // Emit foreground color
                        spi_write16_blocking(spi0, &fg_color, 1);
                    } else {
                        // Emit background color
                        spi_write16_blocking(spi0, &bg_color, 1);
                    }
                    prev_bit = db & 0x1;
                    db = db >> 1;
                }
               width -= col_count;
            }
            if (bold) {
                if (prev_bit) {
                    spi_write16_blocking(spi0, &fg_color, 1);
                } else {
                    spi_write16_blocking(spi0, &bg_color, 1);
                }
            }

            text_buf++;
        }
        hw_write_masked(&spi_get_hw(spi0)->cr0, 7 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
        cs_high();
    }
}

// Draws a icon at the given coordinates. Requires an icon structure.
// Draws it upside down as is tradition.
void draw_icon(uint16_t sx, uint16_t sy, const ico_def_t *ico)
{
    uint16_t x, y;
    int sp;
    const uint8_t *id = ico->image;
    const uint8_t *md = ico->mask;
    uint16_t c;

    for (y = sy + ico->height - 1; y >= sy; y--) {
        for (x = sx; x < sx + ico->width; x+=8) {
            for (sp = 0; sp < 8; sp++) {
                if (!((*md >> (7-sp)) & 1)) {
                    c = ico->pal[*id >> 4];
                    // No better way to draw with transparency, sadly
                    pset(x + sp, y, c);
                }
                sp++;
                if (!((*md >> (7-sp)) & 1)) {
                    c = ico->pal[*id & 0xf];
                    pset(x + sp, y, c);
                }
                id++;
            }
            md++;
        }
    }
}

// Initialize the display
void st7789_init()
{
    uint16_t count;
    uint16_t col;
    st7789_gpio_init();
    st7789_disp_init(40, 53, 240, 135);

    st7789_fill(0, 0, 240, 135, 0x001F); // Clear screen

//    draw_icon(20, 20, &chip_icon);
//    while(1){}
#if 0
    // Pixel format: BGR (15-0) 5:6:5.
    for (count = 0; count < 64; count++) {
        st7789_fill(0, count, 100, 1, count << 11); // horizontal line
    }
#endif

//    st7789_bitblt_rot(0, 0, 240, 135, (uint16_t *)image_data);

}
