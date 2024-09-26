#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
// Our assembled program:
#include "pmemtest.pio.h"


int main() {
    PIO pio;
    uint pin = 0;
    uint offset;
    uint sm;
    uint16_t addr;
    uint8_t db = 0;

    stdio_init_all();
    printf("Test.\n");

    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&pmemtest_program, &pio, &sm, &offset, pin, 2, true);
    pmemtest_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    while(1) {
        addr++;
        db++;
        pio_sm_put(pio, sm, (addr & 0xff) | // Row address
                            (addr & 0xff00) | // Column address
                            ((db & 1) << 16)); // Data bit
//        pio_sm_put(pio, sm, (0xff << 9) | (1 << 17));
        while (pio_sm_is_tx_fifo_full(pio, sm)) {}
//        pio_sm_put(pio, sm, 0xfedcba98l);
//        while (pio_sm_is_tx_fifo_full(pio, sm)) {}
    }

    return 0;
}
