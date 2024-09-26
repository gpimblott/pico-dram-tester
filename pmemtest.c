#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
// Our assembled program:
#include "pmemtest.pio.h"


int main() {
    PIO pio;
    uint pin = 0;
    uint offset;
    uint sm;

    stdio_init_all();
    printf("Test.\n");

    bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&pmemtest_program, &pio, &sm, &offset, pin, 2, true);
    pmemtest_program_init(pio, sm, offset, pin);
    pio_sm_set_enabled(pio, sm, true);

    while(1) {
        pio_sm_put(pio, sm, 0x76543210l);
        while (pio_sm_is_tx_fifo_full(pio, sm)) {}
        pio_sm_put(pio, sm, 0xfedcba98l);
        while (pio_sm_is_tx_fifo_full(pio, sm)) {}
    }

    return 0;
}
