// Main entry point

// TODO:
// Make the refresh test fancier
// Bug fix the 41128

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "app_state.h"
#include "hardware/pio.h"
#include "hardware/vreg.h"
#include "pio_patcher.h"
#include "mem_chip.h"
#include "xoroshiro64starstar.h"
#include "dram_tests.h"
#include "hardware.h"
#include "ui.h"

// Entry point for second core. This is just a generic
// function dispatcher lifted from the Raspberry Pi example code.
void core1_entry() {
    while (1) {
        queue_entry_t entry;
        queue_remove_blocking(&call_queue, &entry);
        int32_t result = entry.func(entry.data, entry.data2);
        queue_add_blocking(&results_queue, &result);
    }
}

// Global variables are defined in app_state.c

// UI functions are in ui.c


// init_buttons_encoder() moved to hardware.c

int main() {
    uint offset;
    uint16_t addr;
    uint8_t db = 0;
    uint din = 0;
    int i, retval;

    // Increase core voltage slightly (default is 1.1V) to better handle overclock
    vreg_set_voltage(VREG_VOLTAGE_1_20);

    // PLL->prim = 0x51000.

    //stdio_uart_init_full(uart0, 57600, 28, 29); // 28=tx, 29=rx actually runs at 115200 due to overclock
    //gpio_init(15);
    //gpio_set_dir(15, GPIO_OUT);

    //printf("Test.\n");
    psrand_init_seeds();

    gpio_init(GPIO_LED);
    gpio_set_dir(GPIO_LED, GPIO_OUT);
    gpio_put(GPIO_LED, 1);

    gpio_init(GPIO_POWER);
    power_off();

    // Set up second core
    queue_init(&call_queue, sizeof(queue_entry_t), 2);
    queue_init(&results_queue, sizeof(int32_t), 2);
    queue_init(&stat_cur_test, sizeof(int), 2);

    // Second core will wait for the call queue.
    multicore_launch_core1(core1_entry);

    // Init display
    st7789_init();

    setup_main_menu();
 //   gui_demo();
    show_main_menu();
    init_buttons_encoder();


// Testing
#if 0
    power_on();
    ram44256_setup_pio(5);
    sleep_ms(10);
    for (i=0; i < 100; i++) {
        ram44256_ram_read(i&7);
        ram44256_ram_write(i&7, 1);
        ram44256_ram_read(i&7);
        ram44256_ram_write(i&7, 0);
//gpio_put(GPIO_LED, marchb_test(8, 1));
    }
    while(1) {}
#endif

    while(1) {
        do_encoder();
        do_buttons();
        do_status();
    }



    return 0;
}
