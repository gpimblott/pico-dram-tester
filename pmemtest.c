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
        pio_sm_put(pio, sm, 0 | // Fast page mode flag
                            0 << 1 | // Write flag
                            (addr & 0xff) << 2 |  // Row address
                            (addr & 0xff00) << 2| // Column address
                            ((0 & 1) << 18));    // Data bit
        while (pio_sm_is_tx_fifo_full(pio, sm)) {}
        return pio_sm_get(pio, sm) & 0x1;
}

void ram_write(int addr, int data)
{
        pio_sm_put(pio, sm, 0 | // Fast page mode flag
                            1 << 1 | // Write flag
                            (addr & 0xff) << 2 |  // Row address
                            (addr & 0xff00) << 2| // Column address
                            ((data & 1) << 18));    // Data bit
        while (pio_sm_is_tx_fifo_full(pio, sm)) {}
        pio_sm_get(pio, sm); // FIXME: do something with the data bit
}

// Also need a ram address mapper, do i track ras and cas separately?

// Also need some testing algorithms... :)

int main() {
//    PIO pio;
    uint pin = 0;
    uint offset;
//    uint sm;
    uint16_t addr;
    uint8_t db = 0;
    int i;

    stdio_init_all();
    printf("Test.\n");

    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&pmemtest_program, &pio, &sm, &offset, pin, 2, true);
    pmemtest_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    while(1) {
#if 0
        addr++;
        db++;
        pio_sm_put(pio, sm, 0 | // Fast page mode flag
                            0 << 1 | // Write flag
                            (addr & 0xff) << 2 |  // Row address
                            (addr & 0xff00) << 2| // Column address
                            ((db & 1) << 18));    // Data bit
        while (pio_sm_is_tx_fifo_full(pio, sm)) {}
        pio_sm_get(pio, sm); // FIXME: do something with the data bit
#endif
        for (i = 0; i < 5; i++) {
            db++;
            ram_read(i);
            ram_write(i, db & 1);
            ram_read(i);
        }
    }

    return 0;
}
