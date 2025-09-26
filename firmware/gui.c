/*
 * gui.c
 *
 * This file contains routines for drawing a simple Graphical User Interface (GUI)
 * on the ST7789 display. It provides functions for drawing various UI elements
 * like rectangles, buttons, text, scrollbars, and listboxes.
 */

#include <stdint.h>
#include <stdbool.h>

#include "st7789.h"
#include "gui.h"

#include "sserif20.h" // Font definition for sserif20
#include "widgets16.h" // Widget icons/fonts


// Default colors used for drawing GUI elements
uint16_t color_fill = COLOR_LTGRAY;      // General fill color
uint16_t color_outline = COLOR_BLACK;   // Outline color for elements
uint16_t color_shadow = COLOR_DKGRAY;   // Shadow color for 3D effects
uint16_t color_highlight = COLOR_WHITE; // Highlight color for 3D effects
uint16_t color_field = COLOR_WHITE;     // Background color for input fields

/**
 * @brief Draws a two-color rectangle, typically used for shaded borders.
 *
 * Draws a rectangle with a top-left color and a bottom-right color to create
 * a simple 3D shading effect.
 *
 * @param sx Starting X-coordinate.
 * @param sy Starting Y-coordinate.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param tl Color for the top and left borders.
 * @param br Color for the bottom and right borders.
 */
void shadow_rect(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height, uint16_t tl, uint16_t br)
{
    // Top border
    st7789_fill(sx,             sy,              width - 1, 1,          tl);
    // Left border
    st7789_fill(sx,             sy,              1,         height - 1, tl);
    // Right border
    st7789_fill(sx + width - 1, sy,              1,         height,     br);
    // Bottom border
    st7789_fill(sx            , sy + height - 1, width,     1,          br);
}

/**
 * @brief Draws a fancy shaded rectangle with various styles.
 *
 * Uses `shadow_rect` and `st7789_fill` to create different 3D-like button
 * and window styles (raised, sunken, outer, inner).
 *
 * @param sx Starting X-coordinate.
 * @param sy Starting Y-coordinate.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param style The desired rendering style (e.g., W_RAISED_OUTER, B_SUNKEN_INNER).
 */
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
        // Draws a window with a raised outer and inner border, and a filled background
        fancy_rect(sx, sy, width, height, W_RAISED_OUTER);
        fancy_rect(sx + 1, sy + 1, width - 2, height - 2, W_RAISED_INNER);
        st7789_fill(sx + 2, sy + 2, width - 4, height - 4, color_fill);
        break; // Added break to prevent fall-through
    case BUTTON:
        // Draws a button with a raised outer and inner border, and a filled background
        fancy_rect(sx, sy, width, height, B_RAISED_OUTER);
        fancy_rect(sx + 1, sy + 1, width - 2, height - 2, B_RAISED_INNER);
        st7789_fill(sx + 2, sy + 2, width - 4, height - 4, color_fill);
        break;
    case FIELD:
        // Draws an input field with a sunken outer and inner border, and a filled background
        fancy_rect(sx, sy, width, height, B_SUNKEN_OUTER);
        fancy_rect(sx + 1, sy + 1, width - 2, height - 2, B_SUNKEN_INNER);
        st7789_fill(sx + 2, sy + 2, width - 4, height - 4, color_field);
        break;
    case STATUS:
        // Draws a status area with a sunken outer border and a filled background
        fancy_rect(sx, sy, width, height, B_SUNKEN_OUTER);
        st7789_fill(sx + 1, sy + 1, width - 1, height - 1, color_fill);
        break; // Added break to prevent fall-through
    case GROUPING:
        // Draws a grouping box with a sunken outer and raised inner border, and a filled background
        fancy_rect(sx, sy, width, height, W_SUNKEN_OUTER);
        fancy_rect(sx + 1, sy + 1, width - 2, height - 2, W_RAISED_INNER);
        st7789_fill(sx + 2, sy + 2, width - 4, height - 4, color_fill);
        break;

    default:
        break;
    }
}

/**
 * @brief Draws a button with text centered on it.
 *
 * @param sx Starting X-coordinate.
 * @param sy Starting Y-coordinate.
 * @param width Width of the button.
 * @param height Height of the button.
 * @param text The text to display on the button.
 * @param font The font definition to use for the text.
 * @param bold True if the text should be bold, false otherwise.
 */
void paint_button(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,
                  char *text, const font_def_t *font, bool bold)
{
    uint16_t twidth = font_string_width(text, 255, font, bold); // Width of the text string
    uint16_t theight = font->height; // Height of the font
    uint16_t tsx = sx + (width / 2) - (twidth / 2); // X-coordinate to center text
    uint16_t tsy = sy + (height / 2) - (theight / 2); // Y-coordinate to center text

    fancy_rect(sx, sy, width, height, BUTTON); // Draw the button background
    font_string(tsx, tsy, text, 255, COLOR_BLACK, color_fill, font, bold); // Draw the text on the button
}

/**
 * @brief Paints a status message in a designated area.
 *
 * Draws a status rectangle and then displays the provided text within it.
 *
 * @param sx Starting X-coordinate of the status area.
 * @param sy Starting Y-coordinate of the status area.
 * @param width Width of the status area.
 * @param text The status message to display.
 */
void paint_status(uint16_t sx, uint16_t sy, uint16_t width, char *text)
{
    fancy_rect(sx, sy, width, sserif20.height + 4, STATUS); // Draw the status background
    font_string(sx+2, sy+2, text, 255, COLOR_BLACK, color_fill, &sserif20, false); // Draw the status text
}

/**
 * @brief Draws a standard dialog box with a title and a close button.
 *
 * @param title The title text for the dialog box.
 */
void paint_dialog(char *title)
{
    fancy_rect(0, 0, 240, 135, WINDOW); // Draw the main window frame
    st7789_fill(3, 3, 240 - 7, 26, COLOR_DKBLUE); // Fill the title bar area
    paint_button(240 - 6 - 21, 6, 21, 21,"\x01", &widgets16, false); // Draw a close/back button icon
    font_string(5, 5, title, 255, COLOR_WHITE, COLOR_DKBLUE, &sserif20, true); // Draw the dialog title
}

// Minimum height for the scrollbar thumb to be visible
#define MIN_SCROLL_HEIGHT 10

/**
 * @brief Draws a scrollbar for a list or content area.
 *
 * Renders scroll up/down buttons and a scroll thumb whose size and position
 * reflect the visible portion and current position within the total content.
 *
 * @param sx Starting X-coordinate of the scrollbar.
 * @param sy Starting Y-coordinate of the scrollbar.
 * @param width Width of the scrollbar.
 * @param height Height of the scrollbar.
 * @param vis Number of visible lines/items.
 * @param tot Total number of lines/items.
 * @param pos Current starting position (line/item) of the visible content.
 */
void paint_scrollbar(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,
                     uint32_t vis, uint32_t tot, uint32_t pos)
{
    // Calculate the usable height for the scroll thumb (excluding buttons)
    uint32_t avail = height - 21 * 2; // Pixels representing the "total" scrollbar area
    uint32_t sb_height; // Height of the scrollbar thumb
    uint32_t start_pos; // Starting Y-position of the scrollbar thumb

    // Adjust position and thumb height if total items are less than visible items
    if (vis >= tot) {
        pos = 0;
        sb_height = avail;
    } else {
        // Ensure position is not out of bounds
        if (pos > tot - vis) {
            pos = tot - vis;
        }
        // Compute scrollbar thumb height proportional to visible content
        sb_height = avail * vis / tot;
    }
    // Calculate the starting position of the scroll thumb
    start_pos = pos * avail / tot;

    // Draw scroll buttons
    paint_button(sx, sy, width, 21, "\x03", &widgets16, false); // Up arrow button
    paint_button(sx, sy + height - 21, width, 21, "\x02", &widgets16, false); // Down arrow button
    // Fill the track area of the scrollbar with a halftone pattern
    st7789_halftone_fill(sx, sy + 21, width, height - 21 - 21, COLOR_WHITE, color_fill);
    // Draw the scroll thumb if it's tall enough to be visible
    if (sb_height >= MIN_SCROLL_HEIGHT) {
        paint_button(sx, sy + 21 + start_pos, width, sb_height, "\x00", &widgets16, false); // Scroll thumb
    }
}

/**
 * @brief Calculates the width of a single word in pixels.
 *
 * A word is defined as a sequence of non-space characters followed by a space
 * or the end of the string.
 *
 * @param text Pointer to the start of the word.
 * @param font The font definition to use.
 * @param bold True if bold, false otherwise.
 * @return The width of the word in pixels.
 */
uint16_t word_width(char *text, const font_def_t *font, bool bold)
{
    uint16_t total_width = 0;
    while (*text != 0) {
        if (*text >= font->count) {
            text++;
            continue;
        }
        total_width += font->widths[*text] + (bold ? 1 : 0);
        if (*text == ' ') break; // Stop at the first space to get word width
        text++;
    }
    return total_width;
}

/**
 * @brief Calculates the number of lines required to display a given text within a specified width.
 *
 * This function performs word wrapping to determine how many lines the text will occupy.
 *
 * @param text The text string to analyze.
 * @param width The maximum width available for the text.
 * @param font The font definition to use.
 * @param bold True if bold, false otherwise.
 * @return The number of lines the text will span.
 */
uint16_t word_line_count(char *text, uint16_t width, const font_def_t *font, bool bold)
{
    uint16_t w_accu = 0; // Accumulated width for the current line
    uint16_t line_count = 1; // Start with at least one line
    uint16_t cur_word_width;
    while (*text) {
        cur_word_width = word_width(text, font, bold);
        if (w_accu + cur_word_width > width) {
            line_count++; // Start a new line
            w_accu = cur_word_width; // Reset accumulated width with the current word's width
        } else {
            w_accu += cur_word_width; // Add word width to current line
        }
        // Advance `text` pointer to the beginning of the next word
        while ((*text != ' ') && (*text != 0)) {
            text++;
        }
        if (*text == ' ') text++; // Skip the space if present
    }
    return line_count;
}

/**
 * @brief Determines how many characters of a text string fit on a single line within a given width.
 *
 * This function respects word boundaries, meaning it will not break words across lines.
 *
 * @param text The text string to analyze.
 * @param width The maximum width available for the line.
 * @param font The font definition to use.
 * @param bold True if bold, false otherwise.
 * @return The number of characters that fit on the line.
 */
uint16_t word_line(char *text, uint16_t width, const font_def_t *font, bool bold)
{
    uint16_t w_accu = 0; // Accumulated width for the current line
    uint16_t cur_word_width;
    uint16_t char_count = 0; // Number of characters that fit
    while (*text) {
        cur_word_width = word_width(text, font, bold);
        if (w_accu + cur_word_width > width) {
            break; // Current word doesn't fit, so stop here
        } else {
            w_accu += cur_word_width; // Add word width to current line
        }
        // Advance `text` pointer and count characters until a space or end of string
        while ((*text != ' ') && (*text != 0)) {
            text++;
            char_count++;
        }
        // If a space is encountered, include it in the count and advance past it
        if (*text == ' ') {
            text++;
            char_count++;
        }
    }
    return char_count;
}

/**
 * @brief Paints a block of text within a specified rectangular area, with word wrapping.
 *
 * The text is vertically centered within the given height. It calculates line breaks
 * based on word boundaries and the available width.
 *
 * @param sx Starting X-coordinate of the text area.
 * @param sy Starting Y-coordinate of the text area.
 * @param width Width of the text area.
 * @param height Height of the text area.
 * @param text The text string to display.
 * @param fg_color Foreground color of the text.
 * @param bg_color Background color of the text.
 * @param font The font definition to use.
 * @param bold True if bold, false otherwise.
 */
void paint_text(uint16_t sx, uint16_t sy, uint16_t width, uint16_t height,
                char *text, uint16_t fg_color, uint16_t bg_color, const font_def_t *font, bool bold)
{
    uint16_t line;
    uint16_t line_count = word_line_count(text, width, font, bold); // Total lines needed for text
    char *text_buf = text; // Keep original text pointer
    uint16_t line_chars; // Number of characters for the current line

    // Adjust line_count if the text exceeds the available height
    if (font->height * line_count > height) {
        line_count = height / font->height;
    }

    // Vertically center the text block within the given height
    sy = sy + (height / 2) - (line_count * font->height / 2);

    // Draw each line of text
    for (line = 0; line < line_count; line++) {
        line_chars = word_line(text, width, font, bold); // Get characters for current line
        font_string(sx, sy + line * font->height, text, line_chars, fg_color, bg_color, font, bold);
        text += line_chars; // Advance text pointer to the start of the next line
    }
}

// Default width for scrollbars
#define DEFAULT_SCROLLBAR_WIDTH 23

/**
 * @brief Handles and displays a listbox GUI element.
 *
 * This function updates the listbox's selection based on `act` (up, down, or none),
 * calculates scrolling, and then draws the listbox with its items and a scrollbar.
 *
 * @param lb Pointer to the `gui_listbox_t` structure holding listbox state.
 * @param act The action to perform on the listbox (LIST_ACTION_UP, LIST_ACTION_DOWN, LIST_ACTION_NONE).
 * @return The index of the currently selected list item.
 */
uint8_t gui_listbox(gui_listbox_t *lb, list_action_t act)
{
    uint16_t height, w;
    int count, st;
    uint16_t fg, bg;
    height = 20 * lb->vis_lines + 4; // Calculate total height of the listbox
    int vis_count; // Number of visible items to draw

    // Adjust selected line if it's out of bounds
    if (lb->sel_line >= lb->tot_lines) {
        lb->sel_line = lb->tot_lines - 1;
    }
    // Adjust starting line for scrolling if total lines exceed visible lines
    if (lb->tot_lines >= lb->vis_lines) {
        if (lb->start_line > lb->tot_lines - lb->vis_lines) {
            lb->start_line = lb->tot_lines - lb->vis_lines;
        }
    } else {
        lb->start_line = 0;
    }

    // Handle list actions (up/down selection)
    if (act == LIST_ACTION_UP) {
        if (lb->sel_line > 0) {
            lb->sel_line--; // Move selection up
            if (lb->start_line > lb->sel_line) {
                lb->start_line = lb->sel_line; // Scroll up if selection goes above visible area
            }
        }
    } else if (act == LIST_ACTION_DOWN) {
        if (lb->sel_line < lb->tot_lines - 1) {
            lb->sel_line++; // Move selection down
            if (lb->start_line < lb->sel_line - lb->vis_lines + 1) {
                lb->start_line = lb->sel_line - lb->vis_lines + 1; // Scroll down if selection goes below visible area
            }
        }
    }

    fancy_rect(lb->sx, lb->sy, lb->width, height, FIELD); // Draw listbox background
    // Draw the scrollbar
    paint_scrollbar(lb->sx + lb->width - 2 - DEFAULT_SCROLLBAR_WIDTH,
                    lb->sy + 2, DEFAULT_SCROLLBAR_WIDTH, height - 4,
                    lb->vis_lines, lb->tot_lines, lb->start_line);

    // Determine how many items to actually draw
    vis_count = (lb->tot_lines < lb->vis_lines) ? lb->tot_lines : lb->vis_lines;

    // Draw each visible list item
    for (count = 0; count < vis_count; count++) {
        st = lb->start_line + count; // Index of the item to draw
        if (st == lb->sel_line) {
            fg = COLOR_WHITE; // Selected item foreground color
            bg = COLOR_DKBLUE; // Selected item background color
        } else {
            fg = COLOR_BLACK; // Unselected item foreground color
            bg = COLOR_WHITE; // Unselected item background color
        }
        // Draw the item text
        font_string(lb->sx + 2, lb->sy + 2 + count * 20, (char *)lb->items[st], 255,
                    fg, bg, &sserif20, false);
        // Fill the remaining space on the line with the background color
        w = font_string_width((char *)lb->items[st], 255, &sserif20, false);
        st7789_fill(lb->sx + 2 + w, lb->sy + 2 + count * 20,
                    lb->width - w - 4 - DEFAULT_SCROLLBAR_WIDTH, 20, bg);
    }
    return lb->sel_line; // Return the index of the selected item
}

/**
 * @brief Displays a message box with a title, content text, and an icon.
 *
 * Includes an "OK" button for user acknowledgment.
 *
 * @param title The title of the message box.
 * @param contents The main text content of the message box.
 * @param icon A pointer to the icon definition to display.
 */
void gui_messagebox(char *title, char *contents, const ico_def_t *icon)
{
    paint_dialog(title); // Draw the dialog box frame with title
    paint_text(70, 30, 160, 80, contents, COLOR_BLACK, COLOR_LTGRAY, &sserif20, false); // Draw the message content
    draw_icon(20, 60, icon); // Draw the icon
    paint_button(90, 110, 60, 23, "OK", &sserif20, false); // Draw the OK button
}

/**
 * @brief A demonstration function for GUI elements.
 *
 * This function is commented out but shows examples of how to use various
 * GUI drawing functions like `paint_dialog`, `paint_text`, etc.
 */
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
    while(1) {} // Infinite loop to keep the demo screen displayed


//     fancy_rect(3, 30, 210, 72, GROUPING);
//     fancy_rect(7, 40, 190, 52, FIELD);
//     font_string(9, 43, "Hello World Text Box", 255, 0x0000, 0xffff, &sserif20, false);
//     paint_button(45, 105, 90, 25, "OK", &sserif20, false);
//     paint_scrollbar(7+190-2-23, 40+2, 23, 52 - 4, 1, 2, 0);
//    gui_listbox(&list1, LIST_ACTION_NONE);

}

