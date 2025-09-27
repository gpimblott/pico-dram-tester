/*
 * ui.c
 *
 * This file handles the User Interface (UI) logic for the pico-dram-tester.
 * It manages different GUI states, menu navigation, display updates,
 * and user input from buttons and rotary encoder.
 */
#include <stdio.h>

#include "ui.h"
#include "app_state.h"
#include "dram_tests.h"
#include "hardware.h"
#include "st7789.h"
#include "sserif16.h"
#include "sserif20.h"

// Icons for display
#include "icons.h"

// X-coordinate for the status icon display
#define STATUS_ICON_X 155
// Y-coordinate for the status icon display
#define STATUS_ICON_Y 65

// X-coordinate for the cell status display
#define CELL_STAT_X 9
// Y-coordinate for the cell status display
#define CELL_STAT_Y 33

// Forward declarations for functions used in this file
void start_the_ram_test();
void stop_the_ram_test();

/**
 * @brief Initializes the main menu with available DRAM chip names.
 *
 * Populates the `main_menu_items` array with the names of the DRAM chips
 * defined in `chip_list` and sets the total number of lines for the menu.
 */
void setup_main_menu()
{
    uint i;
    for (i = 0; i < NUM_CHIPS; i++) {
        main_menu_items[i] = (char *)chip_list[i]->chip_name;
    }
    main_menu.tot_lines = NUM_CHIPS;
}

/**
 * @brief Displays the initial splash screen of the application.
 *
 * Shows a welcome message, application version, and an instruction to continue.
 */
void show_splash_screen() {
    paint_dialog("Welcome");

    // Cell status area. 32x32 elements.
    fancy_rect(7, 31, 224, 100, B_SUNKEN_OUTER); // Usable size is 220x80.
    st7789_fill(9, 33, 220, 96, COLOR_LTGRAY);
    font_string(20, 40, "Pico DRAM Tester", 255, COLOR_BLACK, COLOR_LTGRAY, &sserif20, true);
    font_string(20, 60, APP_VERSION, 255, COLOR_BLACK, COLOR_LTGRAY, &sserif20, false);
    draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &check_icon);
    font_string(20, 100, "Click to continue...", 255, COLOR_BLACK, COLOR_LTGRAY, &sserif16, false);
}

/**
 * @brief Displays the main menu, allowing the user to select a DRAM device.
 *
 * Sets the current menu to `main_menu`, paints a dialog title, and
 * renders the listbox for device selection.
 */
void show_main_menu()
{
    cur_menu = &main_menu;
    paint_dialog("Select Device");
    gui_listbox(cur_menu, LIST_ACTION_NONE);
}

/**
 * @brief Displays the variant selection menu for the currently selected DRAM chip.
 *
 * Populates the `variants_menu` with the available variants for the chosen chip
 * and renders the listbox for variant selection.
 */
void show_variant_menu()
{
    uint chip = main_menu.sel_line;
    cur_menu = &variants_menu;
    paint_dialog("Select Variant");
    variants_menu.items = (char **)chip_list[chip]->variants->variant_names;
    variants_menu.tot_lines = chip_list[chip]->variants->num_variants;
    gui_listbox(cur_menu, LIST_ACTION_NONE);
}

/**
 * @brief Displays the speed grade selection menu for the currently selected DRAM chip.
 *
 * Populates the `speed_menu` with the available speed grades for the chosen chip
 * and renders the listbox for speed grade selection.
 */
void show_speed_menu()
{
    uint chip = main_menu.sel_line;
    cur_menu = &speed_menu;
    paint_dialog("Select DRAM Speed");
    speed_menu.items = (char **)chip_list[chip]->speed_names;
    speed_menu.tot_lines = chip_list[chip]->speed_grades;
    gui_listbox(cur_menu, LIST_ACTION_NONE);
}


/**
 * @brief Updates a single "dot" in the RAM test visualization area.
 *
 * Fills a 2x2 pixel area at the given coordinates (`cx`, `cy`) relative to
 * `CELL_STAT_X` and `CELL_STAT_Y` with the specified color.
 *
 * @param cx The X-coordinate of the dot (relative to the visualization area).
 * @param cy The Y-coordinate of the dot (relative to the visualization area).
 * @param col The color to fill the dot with.
 */
static inline void update_vis_dot(uint16_t cx, uint16_t cy, uint16_t col)
{
    st7789_fill(CELL_STAT_X + cx * 3, CELL_STAT_Y + cy * 3, 2, 2, col);
}


/**
 * @brief Callback function for animating the "drum" icon during a RAM test.
 *
 * Cycles through different drum icon frames to create an animation effect.
 * This function is typically called repeatedly by a timer.
 *
 * @param t Pointer to the repeating timer structure.
 * @return Always returns true to continue the repeating timer.
 */
bool drum_animation_cb(struct repeating_timer *t)
{
    static uint8_t drum_st = 0; // Static variable to keep track of the current drum animation state
    drum_st++;
    if (drum_st > 3) drum_st = 0; // Cycle through 4 drum states (0-3)
    st7789_fill(STATUS_ICON_X, STATUS_ICON_Y, 32, 32, COLOR_LTGRAY); // Clear previous icon area
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
    return true; // Continue the timer
}

/**
 * @brief Sets up and displays the GUI for the RAM test in progress.
 *
 * Paints a "Testing..." dialog, initializes the cell status visualization area
 * with dark gray dots, and starts the drum animation.
 */
void show_test_gui()
{
    uint16_t cx, cy;
    paint_dialog("Testing...");

    // Cell status area. 32x32 elements.
    fancy_rect(7, 31, 100, 100, B_SUNKEN_OUTER); // Usable size is 220x80.
    fancy_rect(8, 32, 98, 98, B_SUNKEN_INNER);
    st7789_fill(9, 33, 96, 96, COLOR_BLACK);
    // Initialize all visualization dots to dark gray
    for (cy = 0; cy < 32; cy++) {
        for (cx = 0; cx < 32; cx++) {
            update_vis_dot(cx, cy, COLOR_DKGRAY);
        }
    }
    // Reset visualization statistics
    stat_old_addr = 0;
    stat_cur_bit = 0;
    stat_cur_subtest = 0;

    // Current test indicator
    paint_status(120, 35, 110, "      "); // Clear previous status text
    draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &drum_icon0); // Draw initial drum icon
    // Start the repeating timer for drum animation
    add_repeating_timer_ms(-100, drum_animation_cb, NULL, &drum_timer);
}

/**
 * @brief Initiates the RAM test with the currently selected chip, speed, and variant.
 *
 * Powers on the hardware, sets up the PIO (Programmable I/O) for the selected
 * DRAM chip, and dispatches the RAM test function to the second core.
 */
void start_the_ram_test()
{
    // Turn on power to the DRAM chip
    power_on();

    // Configure the PIO for the selected chip, speed grade, and variant
    chip_list[main_menu.sel_line]->setup_pio(speed_menu.sel_line, variants_menu.sel_line);

    // Prepare and add the RAM test entry to the call queue for the second core
    queue_entry_t entry = {all_ram_tests,
                           chip_list[main_menu.sel_line]->mem_size,
                           chip_list[main_menu.sel_line]->bits};
    queue_add_blocking(&call_queue, &entry);
}

/**
 * @brief Stops the currently running RAM test.
 *
 * Tears down the PIO configuration for the selected chip and powers off the hardware.
 */
void stop_the_ram_test()
{
    chip_list[main_menu.sel_line]->teardown_pio();
    power_off();
}

/**
 * @brief Maps a given memory address to a visualization dot coordinate.
 *
 * Calculates the `cx` and `cy` coordinates for a visualization dot based on
 * the memory address, bit size of the chip, and an offset. Then updates the dot.
 *
 * @param addr The memory address to visualize.
 * @param ox The X-offset for the visualization area.
 * @param oy The Y-offset for the visualization area.
 * @param bitsize The bit size of the DRAM chip (e.g., 4-bit, 16-bit).
 * @param col The color to draw the dot.
 */
static inline void map_vis_dot(int addr, int ox, int oy, int bitsize, uint16_t col)
{
    int cx, cy;
    if (bitsize == 4) {
        cx = addr & 0xf; // Lower 4 bits for X
        cy = (addr >> 4) & 0xf; // Next 4 bits for Y
    } else {
        cx = addr & 0x1f; // Lower 5 bits for X
        cy = (addr >> 5) & 0x1f; // Next 5 bits for Y
    }
    update_vis_dot(cx + ox, cy + oy, col);
}

/**
 * @brief Updates the RAM test visualization based on the current test state.
 *
 * Draws colored dots on the display to represent memory addresses being tested,
 * providing a visual feedback of the test progress. The color of the dot
 * indicates the current subtest.
 */
void do_visualization()
{
    // Color map for different subtests
    const uint16_t cmap[] = {COLOR_DKBLUE, COLOR_DKGREEN, COLOR_DKMAGENTA, COLOR_DKYELLOW, COLOR_GREEN};
    int bitsize = chip_list[main_menu.sel_line]->bits;
    // Calculate new address for visualization, scaled by memory size and bitsize
    int new_addr = stat_cur_addr * 1024 / chip_list[main_menu.sel_line]->mem_size / bitsize;
    int bit = stat_cur_bit;
    uint16_t col = cmap[stat_cur_subtest]; // Get color based on current subtest
    int delta, i;
    int ox, oy = 0; // Offsets for visualization area

    // Adjust offsets based on bitsize for 4-bit chips to visualize different bits
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
            default: // Case 0
                ox = oy = 0;
        }
    } else {
        ox = oy = 0; // No offset for other bitsizes
    }

    // Update visualization dots based on address changes
    if (new_addr > stat_old_addr) {
        delta = new_addr - stat_old_addr;
        for (i = 0; i < delta; i++) {
            map_vis_dot(stat_old_addr + i, ox, oy, bitsize, col);
        }
    } else { // If address decreased (e.g., wrapping around or specific test patterns)
        delta = stat_old_addr - new_addr;
        for (i = delta - 1; i >= 0; i--) {
            map_vis_dot(stat_old_addr + i, ox, oy, bitsize, col);
        }
    }
    stat_old_addr = new_addr; // Update old address for next visualization step
}

/**
 * @brief Manages the status display during a RAM test and checks for test completion.
 *
 * Calls `do_visualization` to update the visual feedback. It also updates the
 * status text with the current test name and checks the `results_queue` for
 * test completion. If the test is complete, it stops the test, cancels the
 * drum animation, and displays the test results (Passed/Failed).
 */
void do_status()
{
    uint32_t retval;
    char retstring[30];
    uint16_t v;
    static uint16_t v_prev = 0; // This variable is declared but not used.
    int test;

    if (gui_state == DO_TEST) {
        do_visualization(); // Update the visual representation of the test progress

        // Update the status text with the current test being run
        if (queue_try_remove(&stat_cur_test, &test)) {
            paint_status(120, 35, 110, "      "); // Clear previous status
            paint_status(120, 35, 110, (char *)ram_test_names[test]); // Display current test name
        }

        // Check if the RAM test has completed by looking at the results queue
        if (!queue_is_empty(&results_queue)) {
            stop_the_ram_test(); // Stop the hardware and PIO
            sleep_ms(10); // Small delay
            cancel_repeating_timer(&drum_timer); // Stop the drum animation
            queue_remove_blocking(&results_queue, &retval); // Get the test result

            // Transition to test results state and display outcome
            gui_state = TEST_RESULTS;
            st7789_fill(STATUS_ICON_X, STATUS_ICON_Y, 32, 32, COLOR_LTGRAY); // Erase drum icon
            if (retval == 0) { // Test passed
                paint_status(120, 35, 110, "Passed!");
                draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &check_icon);
            } else { // Test failed
                draw_icon(STATUS_ICON_X, STATUS_ICON_Y, &error_icon);
                if (chip_list[main_menu.sel_line]->bits == 4) {
                    // For 4-bit chips, display individual bit failure status
                    sprintf(retstring, "Failed %d%d%d%d", (retval >> 3) & 1,
                                                           (retval >> 2) & 1,
                                                            (retval >> 1) & 1,
                                                            (retval & 1));
                    paint_status(120, 105, 110, retstring);
                } else {
                    paint_status(120, 105, 110, "Failed"); // Generic failure for other bitsizes
                }
            }
        }
    }
}

/**
 * @brief Handles actions triggered by the "action" button press.
 *
 * This function implements the state machine for GUI navigation and test initiation.
 * The behavior depends on the current `gui_state`.
 */
void button_action()
{
    // Perform action based on the current GUI state
    switch (gui_state) {
        case SPLASH_SCREEN:
            gui_state = MAIN_MENU;
            show_main_menu();
            break;
        case MAIN_MENU:
            // If the selected chip has variants, show the variant menu, otherwise go to speed menu
            if (chip_list[main_menu.sel_line]->variants == NULL) {
                gui_state = SPEED_MENU;
                show_speed_menu();
            } else {
                gui_state = VARIANT_MENU;
                show_variant_menu();
            }
            break;
        case VARIANT_MENU:
            // After selecting a variant, proceed to the speed menu
            gui_state = SPEED_MENU;
            show_speed_menu();
            break;
        case SPEED_MENU:
            // Prompt user to place chip and turn on external supply
            gui_messagebox("Place Chip in Socket",
                           "Turn on external supply afterwards, if used.", &chip_icon);
            gui_state = DO_SOCKET;
            break;
        case DO_SOCKET:
            // Start the RAM test after chip is placed
            gui_state = DO_TEST;
            show_test_gui();
            start_the_ram_test();
            break;
        case DO_TEST:
            // No action during active test, user must wait for completion or press back
            break;
        case TEST_RESULTS:
            // Allow quick retest from results screen
            gui_state = DO_TEST;
            show_test_gui();
            start_the_ram_test();
            break;
        default:
            // Fallback to main menu for unknown states
            gui_state = MAIN_MENU;
            break;
    }
}

/**
 * @brief Handles actions triggered by the "back" button press.
 *
 * This function manages navigation back through the GUI states.
 * The behavior depends on the current `gui_state`.
 */
void button_back()
{
    switch (gui_state) {
        case SPLASH_SCREEN:
            gui_state = MAIN_MENU;
            show_main_menu();
            break;
        case MAIN_MENU:
            // No action for back button on main menu (could exit or do nothing)
            break;
        case VARIANT_MENU:
            gui_state = MAIN_MENU;
            show_main_menu();
            break;
        case SPEED_MENU:
            // Go back to main menu if no variants, otherwise back to variant menu
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
            // No action for back button during active test
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

/**
 * @brief Checks the state of the action and back buttons and calls their respective handlers.
 *
 * Uses debounced button states to prevent multiple triggers from a single press.
 */
void do_buttons()
{
    // Static debounce structures for action and back buttons
    static pin_debounce_t action_btn = {GPIO_QUAD_BTN, 0};
    static pin_debounce_t back_btn = {GPIO_BACK_BTN, 0};
    if (is_button_pushed(&action_btn)) button_action(); // Call action handler if action button is pushed
    if (is_button_pushed(&back_btn)) button_back();     // Call back handler if back button is pushed
}

/**
 * @brief Increments the selected item in the current menu (moves down the list).
 *
 * This function is called when the rotary encoder is turned in a direction
 * that corresponds to incrementing the selection.
 */
void wheel_increment()
{
    // Only allow incrementing if a menu is currently active
    if (gui_state == MAIN_MENU || gui_state == SPEED_MENU || gui_state == VARIANT_MENU) {
        gui_listbox(cur_menu, LIST_ACTION_DOWN);
    }
}

/**
 * @brief Decrements the selected item in the current menu (moves up the list).
 *
 * This function is called when the rotary encoder is turned in a direction
 * that corresponds to decrementing the selection.
 */
void wheel_decrement()
{
    // Only allow decrementing if a menu is currently active
    if (gui_state == MAIN_MENU || gui_state == SPEED_MENU || gui_state == VARIANT_MENU) {
        gui_listbox(cur_menu, LIST_ACTION_UP);
    }
}

/**
 * @brief Handles input from the rotary encoder for menu navigation.
 *
 * Debounces the rotary encoder pins and determines if the wheel has been
 * incremented or decremented, then calls the appropriate handler.
 */
void do_encoder()
{
    // Static debounce structures for rotary encoder pins A and B
    static pin_debounce_t pin_a = {GPIO_QUAD_A, 0};
    static pin_debounce_t pin_b = {GPIO_QUAD_B, 0};
    static uint8_t wheel_state_old = 0; // Stores the previous state of the encoder
    uint8_t st;
    uint8_t wheel_state;

    // Read current debounced state of encoder pins
    wheel_state = do_debounce(&pin_a) | (do_debounce(&pin_b) << 1);
    // Combine current and old states to detect transition
    st = wheel_state | (wheel_state_old << 4);
    if (wheel_state != wheel_state_old) {
        // Logic to determine direction of rotation based on state transitions
        // 00 -> 01 clockwise
        // 10 -> 11 counterclockwise
        // 11 -> 10 clockwise
        // 01 -> 00 counterclockwise
        if ((st == 0x01) || (st == 0x32)) { // Clockwise transitions
            wheel_increment();
        }
        if ((st == 0x23) || (st == 0x10)) { // Counter-clockwise transitions
            wheel_decrement();
        }
        wheel_state_old = wheel_state; // Update old state for next iteration
    }
}
