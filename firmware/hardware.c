#include "hardware.h"
#include "app_state.h"
#include "pico/stdlib.h"

void power_on()
{
    gpio_set_dir(GPIO_POWER, true);
    gpio_put(GPIO_POWER, false);
    sleep_ms(100);
}

void power_off()
{
    gpio_set_dir(GPIO_POWER, false);
}

void init_buttons_encoder()
{
    gpio_init(GPIO_QUAD_A);
    gpio_init(GPIO_QUAD_B);
    gpio_init(GPIO_QUAD_BTN);
    gpio_init(GPIO_BACK_BTN);
    gpio_set_dir(GPIO_QUAD_A, GPIO_IN);
    gpio_set_dir(GPIO_QUAD_B, GPIO_IN);
    gpio_set_dir(GPIO_QUAD_BTN, GPIO_IN);
    gpio_set_dir(GPIO_BACK_BTN, GPIO_IN);

}

#define ENC_DEBOUNCE_COUNT 1000
#define BUTTON_DEBOUNCE_COUNT 50000

// Debounces a pin
uint8_t do_debounce(pin_debounce_t *d)
{
    if (gpio_get(d->pin)) {
        d->hcount++;
        if (d->hcount > ENC_DEBOUNCE_COUNT) d->hcount = ENC_DEBOUNCE_COUNT;
    } else {
        d->hcount = 0;
    }
    return (d->hcount >= ENC_DEBOUNCE_COUNT) ? 1 : 0;
}

// Returns true only *once* when a button is pushed. No key repeat.
bool is_button_pushed(pin_debounce_t *pin_b)
{
    if (!gpio_get(pin_b->pin)) {
        if (pin_b->hcount == 0) {
            pin_b->hcount = BUTTON_DEBOUNCE_COUNT;
            return true;
        }
    } else {
        if (pin_b->hcount > 0) {
            pin_b->hcount--;
        }
    }
    return false;
}
