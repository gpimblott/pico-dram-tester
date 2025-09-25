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
