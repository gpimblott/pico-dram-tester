#ifndef ST7789_H
#define ST7789_H


// Font definition table
typedef struct {
    const uint16_t count;
    const uint8_t height;
    const uint8_t *widths;
    const uint16_t *offsets;
    const uint8_t *data;
} font_def_t;


void st7789_init();




#endif
