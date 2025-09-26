/*
 * pmemtest.c
 *
 * This is the main entry point for the pico-dram-tester application.
 * It initializes the system, sets up the UI, handles user input,
 * and dispatches DRAM test functions to the second core.
 */

// TODO:
// Make the refresh test fancier
// Bug fix the 41128
#include <stdio.h>
#include <stdint.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/pio.h"
#include "hardware/vreg.h"

#include "app_state.h"
#include "hardware.h"
#include "pio_patcher.h"
#include "mem_chip.h"
#include "xoroshiro64starstar.h"
#include "dram_tests.h"
#include "ui.h"

/**
 * @brief Entry point for the second CPU core (Core 1).
 *
 * This function continuously waits for function calls to be added to `call_queue`.
 * When a call is received, it executes the specified function with its arguments
 * and then places the result into `results_queue`.
 */
void core1_entry() {
    while (1) {
        queue_entry_t entry; // Structure to hold function pointer and arguments
        // Block until an entry is available in the call queue
        queue_remove_blocking(&call_queue, &entry);
        // Execute the function with the provided data
        int32_t result = entry.func(entry.data, entry.data2);
        // Add the result to the results queue
        queue_add_blocking(&results_queue, &result);
    }
}

// init_buttons_encoder() moved to hardware.c (comment indicates refactoring)

/**
 * @brief Main entry point of the pico-dram-tester application.
 *
 * Initializes the Pico W microcontroller, sets up peripherals like the display,
 * buttons, and rotary encoder, initializes application state, launches Core 1,
 * and then enters the main event loop to handle UI updates and user input.
 *
 * @return 0 on successful execution (though typically an embedded system loops indefinitely).
 */
int main() {
    uint offset; // Variable for PIO program offset (not directly used here, but common in PIO setup)
    uint16_t addr; // Generic address variable (not directly used here)
    uint8_t db = 0; // Generic byte variable (not directly used here)
    uint din = 0; // Generic data input variable (not directly used here)
    int i, retval; // Loop counter and return value variable

    // Increase core voltage slightly (default is 1.1V) to better handle overclock
    vreg_set_voltage(VREG_VOLTAGE_1_20);

    // PLL->prim = 0x51000. (Commented out, possibly for future reference or debugging)

    // stdio_uart_init_full(uart0, 57600, 28, 29); // 28=tx, 29=rx actually runs at 115200 due to overclock
    // gpio_init(15);
    // gpio_set_dir(15, GPIO_OUT);

    psrand_init_seeds(); // Initialize pseudo-random number generator seeds

    // Initialize onboard LED
    gpio_init(GPIO_LED);
    gpio_set_dir(GPIO_LED, GPIO_OUT);
    gpio_put(GPIO_LED, 1); // Turn LED on to indicate power/initialization

    // Initialize power control GPIO and ensure power is off initially
    gpio_init(GPIO_POWER);
    power_off();

    init_buttons_encoder(); // Initialize buttons and rotary encoder GPIOs

    // Set up inter-core communication queues
    queue_init(&call_queue, sizeof(queue_entry_t), 2);    // Queue for Core 0 to send tasks to Core 1
    queue_init(&results_queue, sizeof(int32_t), 2); // Queue for Core 1 to send results to Core 0
    queue_init(&stat_cur_test, sizeof(int), 2);     // Queue for test status updates to UI

    // Launch Core 1, which will start executing `core1_entry`
    multicore_launch_core1(core1_entry);

    // Initialize the ST7789 LCD display
    st7789_init();
   
    // Setup the main menu items
    setup_main_menu();

    // Display the splash screen and wait for user interaction
    show_splash_screen();

    // Re-initialize the encoder (redundant if already called by init_buttons_encoder, but harmless)
    init_buttons_encoder();

    // Main application loop
    while(1) {
        do_encoder(); // Handle rotary encoder input
        do_buttons(); // Handle button presses
        do_status();  // Update UI status and check for test completion
    }

    return 0; // Should not be reached in a typical embedded application
}

