/*
 * app_state.c
 *
 * This file defines and initializes global application state variables
 * for the pico-dram-tester. It includes hardware configurations, UI states,
 * and data structures used across different modules.
 */
#include "app_state.h"

#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/pio.h"
#include "hardware/vreg.h"
#include "pio_patcher.h"
#include "mem_chip.h"
#include "xoroshiro64starstar.h"

// Include PIO (Programmable I/O) programs for various RAM types
#include "ram2114.pio.h"
#include "ram4116.pio.h"
#include "ram4132.pio.h"
#include "ram4164.pio.h"
#include "ram41128.pio.h"
#include "ram41256.pio.h"
#include "ram_4bit.pio.h"

// Global variable definitions

// PIO instance and state machine number used for DRAM communication
PIO pio; // The PIO block instance (e.g., pio0, pio1)
uint sm = 0; // The state machine index within the PIO block
uint offset; // The instruction memory offset where the PIO program is loaded

// Volatile variables for tracking current test status and visualization
volatile int stat_cur_addr;    // Current memory address being tested
volatile int stat_old_addr;    // Previous memory address for visualization updates
volatile int stat_cur_bit;     // Current data bit being tested
queue_t stat_cur_test;         // Queue to communicate the current test being run to the UI
volatile int stat_cur_subtest; // Current sub-test phase within a larger test (e.g., March-B phases)

// Mask for RAM data bits, used to determine the width of the data bus
uint ram_bit_mask;

// Array to store seeds for pseudo-random number generation in tests
uint64_t random_seeds[PSEUDO_VALUES];

// Array of strings holding the names of the RAM tests for display purposes
const char *ram_test_names[] = {"March-B", "Pseudo", "Refresh", "Checkboard"};

// Pointer to the currently active menu in the GUI
gui_listbox_t *cur_menu;

// Array of strings for the main menu items (DRAM chip names)
char *main_menu_items[MAIN_MENU_ITEMS];
// Definition of the main menu listbox structure
gui_listbox_t main_menu = {7, 40, 220, MAIN_MENU_ITEMS, 4, 0, 0, main_menu_items};

// Array of pointers to `mem_chip_t` structures, defining supported DRAM chips
const mem_chip_t *chip_list[NUM_CHIPS] = {&ram2114_chip, &ram4027_chip, &ram4116_half_chip, &ram4116_chip,
                                 &ram4132_stk_chip, &ram4164_half_chip, &ram4164_chip,
                                 &ram41128_chip, &ram41256_chip, &ram4416_half_chip,
                                 &ram4416_chip, &ram4464_chip, &ram44256_chip};

// Definition of the variants menu listbox structure (initialized dynamically)
gui_listbox_t variants_menu = {7, 40, 220, 0, 4, 0, 0, 0};
// Definition of the speed grade menu listbox structure (initialized dynamically)
gui_listbox_t speed_menu = {7, 40, 220, 0, 4, 0, 0, 0};

// Current state of the Graphical User Interface (GUI) state machine
gui_state_t gui_state = SPLASH_SCREEN;

// Repeating timer structure for animations (e.g., drum animation during tests)
struct repeating_timer drum_timer;

// Queues for inter-core communication
queue_t call_queue;    // Queue for sending function calls/tasks to the second core
queue_t results_queue; // Queue for receiving results/status from the second core
