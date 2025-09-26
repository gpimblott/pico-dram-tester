/*
 * hardware.c
 *
 * This file provides functions for controlling hardware components
 * such as power to the DRAM chip and handling input from buttons
 * and a rotary encoder.
 */

#include "hardware.h"
#include "app_state.h"
#include "pico/stdlib.h"

/**
 * @brief Turns on power to the DRAM chip.
 *
 * Configures the GPIO pin connected to the power control as an output
 * and sets it to a low state (active-low power control).
 * Includes a small delay for power stabilization.
 */
void power_on()
{
    gpio_set_dir(GPIO_POWER, true);  // Set power control pin as output
    gpio_put(GPIO_POWER, false);     // Set pin low to turn on power
    sleep_ms(100);                   // Wait for power to stabilize
}

/**
 * @brief Turns off power to the DRAM chip.
 *
 * Configures the GPIO pin connected to the power control as an input
 * (effectively high-impedance) to turn off power.
 */
void power_off()
{
    gpio_set_dir(GPIO_POWER, false); // Set power control pin as input (power off)
}

/**
 * @brief Initializes GPIO pins for the rotary encoder and buttons.
 *
 * Sets up the quadrature encoder pins (A and B), the encoder's push button,
 * and the dedicated back button as inputs.
 */
void init_buttons_encoder()
{
    // Initialize GPIO pins
    gpio_init(GPIO_QUAD_A);
    gpio_init(GPIO_QUAD_B);
    gpio_init(GPIO_QUAD_BTN);
    gpio_init(GPIO_BACK_BTN);
    // Set GPIO pins as inputs
    gpio_set_dir(GPIO_QUAD_A, GPIO_IN);
    gpio_set_dir(GPIO_QUAD_B, GPIO_IN);
    gpio_set_dir(GPIO_QUAD_BTN, GPIO_IN);
    gpio_set_dir(GPIO_BACK_BTN, GPIO_IN);

}

// Debounce count for rotary encoder pins (shorter debounce)
#define ENC_DEBOUNCE_COUNT 1000
// Debounce count for push buttons (longer debounce)
#define BUTTON_DEBOUNCE_COUNT 50000

/**
 * @brief Performs debouncing for a given GPIO pin.
 *
 * Increments a counter if the pin is high, resets it if low. Returns 1 if
 * the pin has been high for `ENC_DEBOUNCE_COUNT` consecutive checks, 0 otherwise.
 *
 * @param d Pointer to a `pin_debounce_t` structure holding pin and counter state.
 * @return 1 if the pin is considered stable high, 0 otherwise.
 */
uint8_t do_debounce(pin_debounce_t *d)
{
    if (gpio_get(d->pin)) { // If pin is currently high
        d->hcount++;
        // Prevent overflow, cap at debounce count
        if (d->hcount > ENC_DEBOUNCE_COUNT) d->hcount = ENC_DEBOUNCE_COUNT;
    } else { // If pin is currently low
        d->hcount = 0; // Reset counter
    }
    // Return true if high count reaches debounce threshold
    return (d->hcount >= ENC_DEBOUNCE_COUNT) ? 1 : 0;
}

/**
 * @brief Checks if a button has been pushed, returning true only once per press.
 *
 * This function implements a stateful debouncing mechanism for buttons.
 * It returns true when a button is first detected as pressed after being released,
 * and then remains false until the button is released and pressed again.
 *
 * @param pin_b Pointer to a `pin_debounce_t` structure for the button.
 * @return True if the button was just pushed, false otherwise.
 */
bool is_button_pushed(pin_debounce_t *pin_b)
{
    if (!gpio_get(pin_b->pin)) { // If button is currently pressed (assuming active-low)
        if (pin_b->hcount == 0) { // If this is the first detection of a press
            pin_b->hcount = BUTTON_DEBOUNCE_COUNT; // Set counter to debounce threshold
            return true; // Indicate button was pushed
        }
    } else { // If button is currently released
        if (pin_b->hcount > 0) {
            pin_b->hcount--; // Decrement counter, allowing for next press detection
        }
    }
    return false; // Button not pushed or still debouncing
}
