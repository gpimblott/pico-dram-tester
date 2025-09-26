#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/util/queue.h"
#include "hardware/pio.h"
#include "gui.h"
#include "mem_chip.h"

#define APP_VERSION "0.4"

#define GPIO_POWER 4
#define GPIO_QUAD_A 22
#define GPIO_QUAD_B 26
#define GPIO_QUAD_BTN 27
#define GPIO_BACK_BTN 28
#define GPIO_LED 25


#define MAIN_MENU_ITEMS 16
#define NUM_CHIPS 12
#define PSEUDO_VALUES 64

// Enums
typedef enum {
    SPLASH_SCREEN,
    MAIN_MENU,
    VARIANT_MENU,
    SPEED_MENU,
    DO_SOCKET,
    DO_TEST,
    TEST_RESULTS
} gui_state_t;

// Extern declarations for global variables

// PIO
extern PIO pio;
extern uint sm;
extern uint offset;

// Test Status
extern volatile int stat_cur_addr;
extern volatile int stat_old_addr;
extern volatile int stat_cur_bit;
extern queue_t stat_cur_test;
extern volatile int stat_cur_subtest;
extern uint ram_bit_mask;

// GUI
extern gui_listbox_t *cur_menu;
extern char *main_menu_items[MAIN_MENU_ITEMS];
extern gui_listbox_t main_menu;
extern gui_listbox_t variants_menu;
extern gui_listbox_t speed_menu;
extern gui_state_t gui_state;
extern struct repeating_timer drum_timer;

// Chip Data
extern const mem_chip_t *chip_list[NUM_CHIPS];
extern const char *ram_test_names[];

// Multicore
extern queue_t call_queue;
extern queue_t results_queue;

// Pseudorandom Test
extern uint64_t random_seeds[PSEUDO_VALUES];

#endif //APP_STATE_H
