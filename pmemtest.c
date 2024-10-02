#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
// Our assembled program:
#include "pmemtest.pio.h"

PIO pio;
uint sm = 0;

// Routines for reading and writing memory.
int ram_read(int addr)
{
        pio_sm_put(pio, sm, 0 |                     // Fast page mode flag
                            0 << 1 |                // Write flag
                            (addr & 0xff) << 2 |    // Row address
                            (addr & 0xff00) << 2|   // Column address
                            ((0 & 1) << 18));       // Data bit
        while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for data to arrive
        return pio_sm_get(pio, sm);                 // Return the data
}

void ram_write(int addr, int data)
{
        pio_sm_put(pio, sm, 0 |                     // Fast page mode flag
                            1 << 1 |                // Write flag
                            (addr & 0xff) << 2 |    // Row address
                            (addr & 0xff00) << 2|   // Column address
                            ((data & 1) << 18));    // Data bit
        while (pio_sm_is_rx_fifo_empty(pio, sm)) {} // Wait for dummy data
        pio_sm_get(pio, sm);                        // Discard the dummy data bit
}

int addr_map(int addr)
{
    return addr;
 //   return (addr >> 8) | ((addr & 0xFF) << 8);
}

void ram_fill(int range, int data)
{
    int i;
    for (i = 0; i < range; i++) {
        ram_write(addr_map(i), data);
    }
}

int ram_toggle_check(int range, uint expected)
{
    int i, j;
    uint bit;
    int retval = -1;
    for (i = 0; i < range; i++) {
        bit = ram_read(addr_map(i));
        if (bit != expected) retval = i; //return i;
        //retval = (retval << 1) | (bit & 0x1);
        gpio_put(15, bit);
        gpio_put(15, bit);
//        for (j = 0; j < 100; j++) {
            gpio_put(15, 0);
//        }
        ram_write(addr_map(i), (~expected) & 1);
    }
    return retval;
}

int ramtest(int range)
{
    int retval1, retval2;
    ram_fill(range, 1);
    retval1 = ram_toggle_check(range, 1);
    retval2 = ram_toggle_check(range, 0);
    printf("1: %x. 2: %x\n", retval1, retval2);
    if (retval1 != -1) return retval1;
    if (retval2 != -1) return retval2;
    return -1;
}

// Also need some testing algorithms... :)

int main() {
    uint pin = 0;
    uint offset;
    uint16_t addr;
    uint8_t db = 0;
    uint din = 0;
    int i, retval;

    // PLL->prim = 0x51000.

    stdio_uart_init_full(uart0, 57600, 28, 29); // 28=tx, 29=rx actually runs at 115200 due to overclock
    gpio_init(15);
    gpio_set_dir(15, GPIO_OUT);

    printf("Test.\n");

    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&pmemtest_program, &pio, &sm, &offset, pin, 2, true);
    pmemtest_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    while(1) {
        retval = ramtest(65536);
        printf("Rv: %d\n", retval);
    }

    return 0;
}
