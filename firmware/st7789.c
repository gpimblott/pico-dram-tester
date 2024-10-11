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

static inline void mode_cmd()
{
    gpio_put(PIN_SPI_DC, 0);
}

static inline void mode_data()
{
    gpio_put(PIN_SPI_DC, 1);
}

static inline void cs_low()
{
    asm volatile("nop \n nop \n nop");
    gpio_put (PIN_SPI_CS, 0);
    asm volatile("nop \n nop \n nop");
}

static inline void cs_high()
{
    asm volatile("nop \n nop \n nop");
    gpio_put (PIN_SPI_CS, 1);
    asm volatile("nop \n nop \n nop");
}

static void write_command(uint8_t cmd, uint8_t num_bytes, uint8_t *buf)
{
    cs_low();
    mode_cmd();
    // Write command byte
    spi_write_blocking(spi0, &cmd, 1);
    mode_data();
    // Write data bytes
    if (num_bytes > 0) {
        spi_write_blocking(spi0, buf, num_bytes);
    }
    cs_high();
}

static void write_data16(int num_words, uint16_t *buf)
{
    hw_write_masked(&spi_get_hw(spi0)->cr0, 15 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
    spi_write16_blocking(spi0, buf, num_words);
    hw_write_masked(&spi_get_hw(spi0)->cr0, 7 << SPI_SSPCR0_DSS_LSB, SPI_SSPCR0_DSS_BITS);
}

static void pset(uint16_t x, uint16_t y, uint16_t col)
{
    uint8_t db[] = {0, 0, 0, 0};
    db[0] = x >> 8;
    db[1] = x & 0xFF;
    db[2] = (x + 1) >> 8;
    db[3] = (x + 1) & 0xFF;
    write_command(CMD_CASET, 4, db);
    db[0] = y >> 8;
    db[1] = y & 0xFF;
    db[2] = (y + 1) >> 8;
    db[3] = (y + 1) & 0xFF;
    write_command(CMD_RASET, 4, db);
    write_command(CMD_RAMWR, 0, NULL);
    write_data16(1, &col);
}

static void st7789_gpio_init()
{

#if !defined(spi0)
#warning something wrong with spi0 not being defined
#endif
    // Set up GPIO for SPI0
    spi_init(spi0, 1 * 1000 * 1000);
    spi_set_format(spi0, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(PIN_SPI_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SPI_DO, GPIO_FUNC_SPI);
 //   bi_decl(bi_2pins_with_func(PIN_SPI_DO, PIN_SPI_SCK));

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

void st7789_init()
{
    uint8_t db[] = {0, 0, 0, 0};
    st7789_gpio_init();
    write_command(CMD_SWRESET, 0, NULL); // Software reset
    sleep_ms(150);
    write_command(CMD_SLPOUT, 0, NULL);  // Exit sleep mode
    sleep_ms(10);
    db[0] = 0x55; // 16 bit color
    write_command(CMD_COLMOD, 1, db);    // Interface format
    sleep_ms(10);
    db[0] = 0x08; // RGB
    write_command(CMD_MADCTL, 1, db);    // Memory address control
    db[0] = 0x0; // X address start = 0
    db[1] = 0x0;
    db[2] = 0x0; // X address end = 240
    db[3] = 240;
    write_command(CMD_CASET, 4, db);
    db[0] = 0x0; // Y address start = 0
    db[1] = 0x0;
    db[2] = 320 >> 8; // Y address end = 320
    db[3] = 320 & 0xFF;
    write_command(CMD_RASET, 4, db);
    write_command(CMD_INVON, 0, NULL);   // Display inversion on
    sleep_ms(10);
    write_command(CMD_NORON, 0, NULL);   // Partial off = normal
    sleep_ms(10);
    write_command(CMD_DISPON, 0, NULL);  // Display on
    sleep_ms(10);

    for (int count = 10; count < 100; count++) {
        pset(count, count, 0x0FFF);
    }
}
