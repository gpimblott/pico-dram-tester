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
void st7789_fill(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t col);
void font_string(uint16_t x, uint16_t y, char *text,
                 uint16_t fg_color, uint16_t bg_color,
                 const font_def_t *font, bool bold);
uint16_t font_string_width(char *text, const font_def_t *font, bool bold);
void st7789_halftone_fill(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t c1, uint16_t c2);



#endif
