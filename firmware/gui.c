// Routines for displaying a simple GUI

#include <stdint.h>
#include <stdbool.h>
#include "st7789.h"
#include "gui.h"

#include "sserif20.h"
#include "widgets16.h"


// Colors for fancy rectangles
uint16_t color_fill = COLOR_LTGRAY;
uint16_t color_outline = COLOR_BLACK;
uint16_t color_shadow = COLOR_DKGRAY;
uint16_t color_highlight = COLOR_WHITE;
uint16_t color_field = COLOR_WHITE;

// Two-color rectangle
void shadow_rect(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t tl, uint16_t br)
{
    st7789_fill(sx,             sy,              width - 1, 1,          tl);
    st7789_fill(sx,             sy,              1,         height - 1, tl);
    st7789_fill(sx + width - 1, sy,              1,         height,     br);
    st7789_fill(sx            , sy + height - 1, width,     1,          br);
}

// Draw a fancy shaded rectangle
void fancy_rect(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, rstyle_t style)
{
    switch (style) {
    case W_RAISED_OUTER:
        shadow_rect(sx, sy, width, height, color_fill, color_outline);
        break;
    case W_RAISED_INNER:
        shadow_rect(sx, sy, width, height, color_highlight, color_shadow);
        break;
    case W_SUNKEN_OUTER:
        shadow_rect(sx, sy, width, height, color_shadow, color_highlight);
        break;
    case W_SUNKEN_INNER:
        shadow_rect(sx, sy, width, height, color_outline, color_fill);
        break;
    case B_RAISED_OUTER:
        shadow_rect(sx, sy, width, height, color_highlight, color_outline);
        break;
    case B_RAISED_INNER:
        shadow_rect(sx, sy, width, height, color_fill, color_shadow);
        break;
    case B_SUNKEN_OUTER:
        shadow_rect(sx, sy, width, height, color_outline, color_highlight);
        break;
    case B_SUNKEN_INNER:
        shadow_rect(sx, sy, width, height, color_shadow, color_fill);
        break;
    case WINDOW:
        fancy_rect(sx, sy, width, height, W_RAISED_OUTER);
        fancy_rect(sx + 1, sy + 1, width - 2, height - 2, W_RAISED_INNER);
        st7789_fill(sx + 2, sy + 2, width - 4, height - 4, color_fill);
    case BUTTON:
        fancy_rect(sx, sy, width, height, B_RAISED_OUTER);
        fancy_rect(sx + 1, sy + 1, width - 2, height - 2, B_RAISED_INNER);
        st7789_fill(sx + 2, sy + 2, width - 4, height - 4, color_fill);
        break;
    case FIELD:
        fancy_rect(sx, sy, width, height, B_SUNKEN_OUTER);
        fancy_rect(sx + 1, sy + 1, width - 2, height - 2, B_SUNKEN_INNER);
        st7789_fill(sx + 2, sy + 2, width - 4, height - 4, color_field);
        break;
    case STATUS:
        fancy_rect(sx, sy, width, height, B_SUNKEN_OUTER);
        st7789_fill(sx + 1, sy + 1, width - 1, height - 1, color_fill);
    case GROUPING:
        fancy_rect(sx, sy, width, height, W_SUNKEN_OUTER);
        fancy_rect(sx + 1, sy + 1, width - 2, height - 2, W_RAISED_INNER);
        st7789_fill(sx + 2, sy + 2, width - 4, height - 4, color_fill);
        break;

    default:
        break;
    }
}

// Draws a button
void paint_button(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,
                  char *text, const font_def_t *font, bool bold)
{
    uint16_t twidth = font_string_width(text, 255, font, bold);
    uint16_t theight = font->height;
    uint16_t tsx = sx + (width / 2) - (twidth / 2);
    uint16_t tsy = sy + (height / 2) - (theight / 2);

    fancy_rect(sx, sy, width, height, BUTTON);
    font_string(tsx, tsy, text, 255, COLOR_BLACK, color_fill, font, bold);
}

void paint_status(uint16_t sx, uint16_t sy, uint16_t width, char *text)
{
    fancy_rect(sx, sy, width, sserif20.height + 4, STATUS);
    font_string(sx+2, sy+2, text, 255, COLOR_BLACK, color_fill, &sserif20, false);
}

// Draws a dialog box
void paint_dialog(char *title)
{
    fancy_rect(0, 0, 240, 135, WINDOW);
    st7789_fill(3, 3, 240 - 7, 26, COLOR_DKBLUE);
    paint_button(240 - 6 - 21, 6, 21, 21,"\x01", &widgets16, false);
    font_string(5, 5, title, 255, COLOR_WHITE, COLOR_DKBLUE, &sserif20, true);
}

// Lines visible
// Lines available
// Starting line
#define MIN_SCROLL_HEIGHT 10
void paint_scrollbar(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,
                     uint32_t vis, uint32_t tot, uint32_t pos)
{
    // Position of marker
    uint32_t avail = height - 21 * 2; // Pixels representing the "total" scrollbar area
    uint32_t sb_height = avail * vis / tot; // Compute scrollbar height
    uint32_t start_pos;

    if (vis >= tot) {
        pos = 0;
        sb_height = avail;
    } else {
        if (pos > tot - vis) { // Is position out of range?
            pos = tot - vis;
        }
    }
    start_pos = pos * avail / tot;

    // Buttons
    paint_button(sx, sy, width, 21, "\x03", &widgets16, false);
    paint_button(sx, sy + height - 21, width, 21, "\x02", &widgets16, false);
    st7789_halftone_fill(sx, sy + 21, width, height - 21 - 21, COLOR_WHITE, color_fill);
    // Position marker. Only show if there is room.
    if (sb_height >= MIN_SCROLL_HEIGHT) {
        paint_button(sx, sy + 21 + start_pos, width, sb_height, "\x00", &widgets16, false);
    }
}

// Get the width of a single word
uint16_t word_width(char *text, const font_def_t *font, bool bold)
{
    uint16_t total_width = 0;
    while (*text != 0) {
        if (*text >= font->count) {
            text++;
            continue;
        }
        total_width += font->widths[*text] + (bold ? 1 : 0);
        if (*text == ' ') break; // Ensure the space is counted
        text++;
    }
    return total_width;
}

uint16_t word_line_count(char *text, uint16_t width, const font_def_t *font, bool bold)
{
    uint16_t w_accu = 0;
    uint16_t line_count = 1;
    uint16_t cur_word_width;
    while (*text) {
        cur_word_width = word_width(text, font, bold);
        if (w_accu + cur_word_width > width) {
            line_count++;
            w_accu = cur_word_width;
        } else {
            w_accu += cur_word_width;
        }
        // Go to the next word
        while ((*text != ' ') && (*text != 0)) {
            text++;
        }
        if (*text == ' ') text++;
    }
    return line_count;
}

// Get number of characters in the given line that fits in width
uint16_t word_line(char *text, uint16_t width, const font_def_t *font, bool bold)
{
    uint16_t w_accu = 0;
    uint16_t cur_word_width;
    uint16_t char_count = 0;
    while (*text) {
        cur_word_width = word_width(text, font, bold);
        if (w_accu + cur_word_width > width) {
            break;
        } else {
            w_accu += cur_word_width;
        }
        // Get the next word
        while ((*text != ' ') && (*text != 0)) {
            text++;
            char_count++;
        }
        // Ignore the space
        if (*text == ' ') {
            text++;
            char_count++;
        }
    }
    return char_count;
}

// Paints text in the given box. TODO: justification options
void paint_text(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,
                char *text, uint16_t fg_color, uint16_t bg_color, const font_def_t *font, bool bold)
{
    uint16_t line;
    uint16_t line_count = word_line_count(text, width, font, bold);
    char *text_buf = text;
    uint16_t line_chars;
    // Can we fit it all or do we truncate the number of lines?
    if (font->height * line_count > height) {
        line_count = height / font->height;
    }

    // Vertically centered
    sy = sy + (height / 2) - (line_count * font->height / 2);

    for (line = 0; line < line_count; line++) {
        line_chars = word_line(text, width, font, bold);
        font_string(sx, sy + line * font->height, text, line_chars, fg_color, bg_color, font, bold);
        text += line_chars;
    }
}

#define DEFAULT_SCROLLBAR_WIDTH 23
// Handles and displays the list box.
// Pass it a struct to hold info about this list box as well
// as the action to perform (move the selection up or down, or no action).
// Returns the current selected list item.
uint8_t gui_listbox(gui_listbox_t *lb, list_action_t act)
{
    uint16_t height, w;
    int count, st;
    uint16_t fg, bg;
    height = 20 * lb->vis_lines + 4;
    int vis_count;

    if (lb->sel_line >= lb->tot_lines) {
        lb->sel_line = lb->tot_lines - 1;
    }
    if (lb->tot_lines >= lb->vis_lines) {
        if (lb->start_line > lb->tot_lines - lb->vis_lines) {
            lb->start_line = lb->tot_lines - lb->vis_lines;
        }
    } else {
        lb->start_line = 0;
    }

    if (act == LIST_ACTION_UP) {
        if (lb->sel_line > 0) {
            lb->sel_line--;
            if (lb->start_line > lb->sel_line) {
                lb->start_line = lb->sel_line;
            }
        }
    } else if (act == LIST_ACTION_DOWN) {
        if (lb->sel_line < lb->tot_lines - 1) {
            lb->sel_line++;
            if (lb->start_line < lb->sel_line - lb->vis_lines + 1) {
                lb->start_line = lb->sel_line - lb->vis_lines + 1;
            }
        }
    }

    fancy_rect(lb->sx, lb->sy, lb->width, height, FIELD);
    paint_scrollbar(lb->sx + lb->width - 2 - DEFAULT_SCROLLBAR_WIDTH,
                    lb->sy + 2, DEFAULT_SCROLLBAR_WIDTH, height - 4,
                    lb->vis_lines, lb->tot_lines, lb->start_line);

    vis_count = (lb->tot_lines < lb->vis_lines) ? lb->tot_lines : lb->vis_lines;

    for (count = 0; count < vis_count; count++) {
        st = lb->start_line + count;
        if (st == lb->sel_line) {
            fg = COLOR_WHITE;
            bg = COLOR_DKBLUE;
        } else {
            fg = COLOR_BLACK;
            bg = COLOR_WHITE;
        }
        font_string(lb->sx + 2, lb->sy + 2 + count * 20, (char *)lb->items[st], 255,
                    fg, bg, &sserif20, false);
        w = font_string_width((char *)lb->items[st], 255, &sserif20, false);
        st7789_fill(lb->sx + 2 + w, lb->sy + 2 + count * 20,
                    lb->width - w - 4 - DEFAULT_SCROLLBAR_WIDTH, 20, bg);
    }
    return lb->sel_line;
}

void gui_messagebox(char *title, char *contents, const ico_def_t *icon)
{
    paint_dialog(title);
    paint_text(70, 30, 160, 80, contents, COLOR_BLACK, COLOR_LTGRAY, &sserif20, false);
    draw_icon(20, 60, icon);
    paint_button(90, 110, 60, 23, "OK", &sserif20, false);
}

void gui_demo()
{
//    const char *list1_items[] = {"Item one", "Item two", "Item three", "Item four",
//                                 "Item five", "Item six", "Item seven", "Item eight",
//                                 "Item nine", "Item ten" };
//    gui_listbox_t list1 = {7, 40, 220, 10, 4, 0, 0, list1_items};
     paint_dialog("Dialog Box");
    st7789_fill(70, 32, 160, 100, 0x0000);
 //   paint_text(70, 32, 160, 100, "This is a test, testing testing testing", 0x0000, 0xffff, &sserif20, false);
    paint_text(70, 32, 160, 100, "This is a test of some wrapping text to see if it wraps correctly. So much text!", 0x0000, 0xffff, &sserif20, false);
    while(1) {}


//     fancy_rect(3, 30, 210, 72, GROUPING);
//     fancy_rect(7, 40, 190, 52, FIELD);
//     font_string(9, 43, "Hello World Text Box", 255, 0x0000, 0xffff, &sserif20, false);
//     paint_button(45, 105, 90, 25, "OK", &sserif20, false);
//     paint_scrollbar(7+190-2-23, 40+2, 23, 52 - 4, 1, 2, 0);
//    gui_listbox(&list1, LIST_ACTION_NONE);

}
