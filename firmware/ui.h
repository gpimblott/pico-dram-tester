#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include "gui.h"
#include "pico/time.h"

// Function prototypes
void setup_main_menu();
void show_main_menu();
void show_variant_menu();
void show_speed_menu();
void show_test_gui();
void do_visualization();
void do_status();
void button_action();
void button_back();
void do_buttons();
void wheel_increment();
void wheel_decrement();
void do_encoder();
bool drum_animation_cb(struct repeating_timer *t);

#endif //UI_H
