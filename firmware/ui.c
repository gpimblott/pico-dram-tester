#include "ui.h"
#include "app_state.h"
#include "dram_tests.h"
#include "hardware.h"
#include "hardware.h"
#include "st7789.h"
#include <stdio.h>
#include "sserif16.h"
#include "sserif20.h"

// Icons
#include "icons.h"

// Forward declarations
void start_the_ram_test();
void stop_the_ram_test();

void show_splash_screen() {
    st7789_fill(0, 0, 240, 135, COLOR_BLACK);
    font_string(20, 40, "pico-dram-tester", 255, COLOR_WHITE, COLOR_BLACK, &sserif20, true);
    font_string(20, 60, "v0.1", 255, COLOR_WHITE, COLOR_BLACK, &sserif20, false);
    font_string(20, 100, "Click to continue...", 255, COLOR_WHITE, COLOR_BLACK, &sserif16, false);

    static pin_debounce_t action_btn = {GPIO_QUAD_BTN, 0};
    while (true) {
        if (is_button_pushed(&action_btn)) {
            break;
        }
    }
}

void setup_main_menu()
{
    uint i;
    for (i = 0; i < NUM_CHIPS; i++) {
        main_menu_items[i] = (char *)chip_list[i]->chip_name;
    }
    main_menu.tot_lines = NUM_CHIPS;
}

// Setup and display the main menu
void show_main_menu()
{
    cur_menu = &main_menu;
    paint_dialog("Select Device");
    gui_listbox(cur_menu, LIST_ACTION_NONE);
}

void show_variant_menu()
{
    uint chip = main_menu.sel_line;
    cur_menu = &variants_menu;
    paint_dialog("Select Variant");
    variants_menu.items = (char **)chip_list[chip]->variants->variant_names;
    variants_menu.tot_lines = chip_list[chip]->variants->num_variants;
    gui_listbox(cur_menu, LIST_ACTION_NONE);
}

// With the selected chip, populate the speed grade menu and show it
void show_speed_menu()
{
    uint chip = main_menu.sel_line;
    cur_menu = &speed_menu;
    paint_dialog("Select Speed Grade");
    speed_menu.items = (char **)chip_list[chip]->speed_names;
    speed_menu.tot_lines = chip_list[chip]->speed_grades;
    gui_listbox(cur_menu, LIST_ACTION_NONE);
}


#define CELL_STAT_X 9
#define CELL_STAT_Y 33

// Used to update the RAM test GUI left pane
static inline void update_vis_dot(uint16_t cx, uint16_t cy, uint16_t col)
{
    st7789_fill(CELL_STAT_X + cx * 3, CELL_STAT_Y + cy * 3, 2, 2, col);
}

#define STATUS_ICON_X 155
#define STATUS_ICON_Y 65

// Play the drums
bool drum_animation_cb(struct repeating_timer *t)
{
    static uint8_t drum_st = 0;
    drum_st++;
    if (drum_st > 3) drum_st = 0;
    st7789_fill(STATUS_ICON_X, STATUS_ICON_Y, 32, 32, COLOR_LTGRAY);
    switch (drum_st) {
        case 0:
            draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon0);
            break;
        case 1:
            draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon1);
            break;
        case 2:
            draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon2);
            break;
        case 3:
            draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon3);
            break;
    }
    return true;
}

// Show the RAM test console GUI
void show_test_gui()
{
    uint16_t cx, cy;
    paint_dialog("Testing...");

    // Cell status area. 32x32 elements.
    fancy_rect(7, 31, 100, 100, B_SUNKEN_OUTER); // Usable size is 220x80.
    fancy_rect(8, 32, 98, 98, B_SUNKEN_INNER);
    st7789_fill(9, 33, 96, 96, COLOR_BLACK);
    for (cy = 0; cy < 32; cy++) {
        for (cx = 0; cx < 32; cx++) {
            update_vis_dot(cx, cy, COLOR_DKGRAY);
        }
    }
    stat_old_addr = 0;
    stat_cur_bit = 0;
    stat_cur_subtest = 0;

    // Current test indicator
    paint_status(120, 35, 110, "      ");
    draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon0);
    add_repeating_timer_ms(-100, drum_animation_cb, NULL, &drum_timer);
}

// Begins the RAM test with the selected RAM chip
void start_the_ram_test()
{
    // Get the power turned on
    power_on();

    // Get the PIO going
    chip_list[main_menu.sel_line]->setup_pio(speed_menu.sel_line, variants_menu.sel_line);

    // Dispatch the second core
    // (The memory size is from our memory description data structure)
    queue_entry_t entry = {all_ram_tests,
                           chip_list[main_menu.sel_line]->mem_size,
                           chip_list[main_menu.sel_line]->bits};
    queue_add_blocking(&call_queue, &entry);
}

// Stops the RAM test
void stop_the_ram_test()
{
    chip_list[main_menu.sel_line]->teardown_pio();
    power_off();
}

// Figure out where visualization dot goes and map it
static inline void map_vis_dot(int addr, int ox, int oy, int bitsize, uint16_t col)
{
    int cx, cy;
    if (bitsize == 4) {
        cx = addr & 0xf;
        cy = (addr >> 4) & 0xf;
    } else {
        cx = addr & 0x1f;
        cy = (addr >> 5) & 0x1f;
    }
    update_vis_dot(cx + ox, cy + oy, col);
}

// Draw up visualization from current test state
void do_visualization()
{
    const uint16_t cmap[] = {COLOR_DKBLUE, COLOR_DKGREEN, COLOR_DKMAGENTA, COLOR_DKYELLOW, COLOR_GREEN};
    int bitsize = chip_list[main_menu.sel_line]->bits;
    int new_addr = stat_cur_addr * 1024 / chip_list[main_menu.sel_line]->mem_size / bitsize;
    int bit = stat_cur_bit;
    uint16_t col = cmap[stat_cur_subtest];
    int delta, i;
    int ox, oy = 0;

    if (bitsize == 4) {
        switch (bit) {
            case 1:
                oy = 0;
                ox = 16;
                break;
            case 2:
                oy = 16;
                ox = 0;
                break;
            case 3:
                ox = oy = 16;
                break;
            default:
                ox = oy = 0;
        }
    } else {
        ox = oy = 0;
    }

    if (new_addr > stat_old_addr) {
        delta = new_addr - stat_old_addr;
        for (i = 0; i < delta; i++) {
            map_vis_dot(stat_old_addr + i, ox, oy, bitsize, col);
        }
    } else {
        delta = stat_old_addr - new_addr;
        for (i = delta - 1; i >= 0; i--) {
            map_vis_dot(stat_old_addr + i, ox, oy, bitsize, col);
        }
    }
    stat_old_addr = new_addr;
}

// During a RAM test, updates the status window and checks for the end of the test
void do_status()
{
    uint32_t retval;
    char retstring[30];
    uint16_t v;
    static uint16_t v_prev = 0;
    int test;

    if (gui_state == DO_TEST) {
        do_visualization();

        // Update the status text
        if (queue_try_remove(&stat_cur_test, &test)) {
            paint_status(120, 35, 110, "      ");
            paint_status(120, 35, 110, (char *)ram_test_names[test]);
        }

        // Check official status
        if (!queue_is_empty(&results_queue)) {
            stop_the_ram_test();
            // The RAM test completed, so let's handle that
            sleep_ms(10);
            // No more drums
            cancel_repeating_timer(&drum_timer);
            queue_remove_blocking(&results_queue, &retval);
            // Show the completion status
            gui_state = TEST_RESULTS;
            st7789_fill(STATUS_ICON_X, STATUS_ICON_Y, 32, 32, COLOR_LTGRAY); // Erase icon
            if (retval == 0) {
                paint_status(120, 35, 110, "Passed!");
                draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &check_icon);
            } else {
                draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &error_icon);
                if (chip_list[main_menu.sel_line]->bits == 4) {
                    sprintf(retstring, "Failed %d%d%d%d", (retval >> 3) & 1,
                                                           (retval >> 2) & 1,
                                                            (retval >> 1) & 1,
                                                            (retval & 1));
                    paint_status(120, 105, 110, retstring);
                } else {
                    paint_status(120, 105, 110, "Failed");
                }
            }
        }
    }
}

// Called when user presses the action button
void button_action()
{
    // Do something based on the current menu
    switch (gui_state) {
        case MAIN_MENU:
            // Check for variant
            if (chip_list[main_menu.sel_line]->variants == NULL) {
                gui_state = SPEED_MENU;
                show_speed_menu();
            } else {
                gui_state = VARIANT_MENU;
                show_variant_menu();
            }
            break;
        case VARIANT_MENU:
            // Set up variant
            gui_state = SPEED_MENU;
            show_speed_menu();
            break;
        case SPEED_MENU:
            gui_messagebox("Place Chip in Socket",
                           "Turn on external supply afterwards, if used.", &chip_icon);
            gui_state = DO_SOCKET;
            break;
        case DO_SOCKET:
            gui_state = DO_TEST;
            show_test_gui();
            start_the_ram_test();
            break;
        case DO_TEST:
            break;
        case TEST_RESULTS:
            // Quick retest to save time
            gui_state = DO_TEST;
            show_test_gui();
            start_the_ram_test();
            break;
        default:
            gui_state = MAIN_MENU;
            break;
    }
}

// Called when the user presses the back button
void button_back()
{
    switch (gui_state) {
        case MAIN_MENU:
            break;
        case VARIANT_MENU:
            gui_state = MAIN_MENU;
            show_main_menu();
            break;
        case SPEED_MENU:
            // Check if our selection has a variant
            if (chip_list[main_menu.sel_line]->variants == NULL) {
                gui_state = MAIN_MENU;
                show_main_menu();
            } else {
                gui_state = VARIANT_MENU;
                show_variant_menu();
            }
            break;
        case DO_SOCKET:
            gui_state = SPEED_MENU;
            show_speed_menu();
            break;
        case DO_TEST:
            break;
        case TEST_RESULTS:
            gui_state = SPEED_MENU;
            show_speed_menu();
            break;
        default:
            gui_state = MAIN_MENU;
            break;
    }
}

void do_buttons()
{
    static pin_debounce_t action_btn = {GPIO_QUAD_BTN, 0};
    static pin_debounce_t back_btn = {GPIO_BACK_BTN, 0};
    if (is_button_pushed(&action_btn)) button_action();
    if (is_button_pushed(&back_btn)) button_back();
}

void wheel_increment()
{
    if (gui_state == MAIN_MENU || gui_state == SPEED_MENU || gui_state == VARIANT_MENU) {
        gui_listbox(cur_menu, LIST_ACTION_DOWN);
    }
}

void wheel_decrement()
{
    if (gui_state == MAIN_MENU || gui_state == SPEED_MENU || gui_state == VARIANT_MENU) {
        gui_listbox(cur_menu, LIST_ACTION_UP);
    }
}

void do_encoder()
{
    static pin_debounce_t pin_a = {GPIO_QUAD_A, 0};
    static pin_debounce_t pin_b = {GPIO_QUAD_B, 0};
    static uint8_t wheel_state_old = 0;
    uint8_t st;
    uint8_t wheel_state;

    wheel_state = do_debounce(&pin_a) | (do_debounce(&pin_b) << 1);
    st = wheel_state | (wheel_state_old << 4);
    if (wheel_state != wheel_state_old) {
        // Present state, next state
        // 00 -> 01 clockwise
        // 10 -> 11 counterclockwise
        // 11 -> 10 clockwise
        // 01 -> 00 counterclockwise
        if ((st == 0x01) || (st == 0x32)) {
            wheel_increment();
        }
        if ((st == 0x23) || (st == 0x10)) {
            wheel_decrement();
        }
        wheel_state_old = wheel_state;
    }
}
