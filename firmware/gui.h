#ifndef GUI_H
#define GUI_H

#include <stdint.h>
#include "st7789.h"

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

typedef enum {
    W_RAISED_OUTER,
    W_RAISED_INNER,
    W_SUNKEN_OUTER,
    W_SUNKEN_INNER,
    WINDOW,
    B_RAISED_OUTER,
    B_RAISED_INNER,
    B_SUNKEN_OUTER,
    B_SUNKEN_INNER,
    BUTTON,
    FIELD,
    STATUS,
    GROUPING
} rstyle_t;

typedef struct {
    uint16_t sx;
    uint16_t sy;
    uint16_t width;
    uint8_t tot_lines;
    uint8_t vis_lines;
    uint8_t sel_line;
    uint8_t start_line;
    char **items;
} gui_listbox_t;

typedef enum {
    LIST_ACTION_NONE,
    LIST_ACTION_DOWN,
    LIST_ACTION_UP
} list_action_t;

void fancy_rect(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, rstyle_t style);
void paint_button(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,
                  char *text, const font_def_t *font, bool bold);
void paint_status(uint16_t sx, uint16_t sy, uint16_t width, char *text);
void paint_dialog(char *title);
uint8_t gui_listbox(gui_listbox_t *lb, list_action_t act);
void gui_messagebox(char *title, char *contents, const ico_def_t *icon);

void gui_demo();


#endif
