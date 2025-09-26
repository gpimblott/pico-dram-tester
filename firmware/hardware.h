#ifndef HARDWARE_H
#define HARDWARE_H

#include "pico/stdlib.h"

void power_on();
void power_off();
void init_buttons_encoder();

typedef struct {
    uint32_t pin;
    uint32_t hcount;
} pin_debounce_t;

uint8_t do_debounce(pin_debounce_t *d);

bool is_button_pushed(pin_debounce_t *pin_b);

#endif //HARDWARE_H
