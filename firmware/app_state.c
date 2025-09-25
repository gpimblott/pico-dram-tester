#include "app_state.h"
#include "mem_chip.h"

#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/pio.h"
#include "hardware/vreg.h"
#include "pio_patcher.h"
#include "mem_chip.h"
#include "xoroshiro64starstar.h"

// Defined RAM pio programs
#include "ram4116.pio.h"
#include "ram4132.pio.h"
#include "ram4164.pio.h"
#include "ram41128.pio.h"
#include "ram41256.pio.h"
#include "ram_4bit.pio.h"

// Global variable definitions

PIO pio;
uint sm = 0;
uint offset;

volatile int stat_cur_addr;
volatile int stat_old_addr;
volatile int stat_cur_bit;
queue_t stat_cur_test;
volatile int stat_cur_subtest;

uint ram_bit_mask;

uint64_t random_seeds[PSEUDO_VALUES];
const char *ram_test_names[] = {"March-B", "Pseudo", "Refresh"};

gui_listbox_t *cur_menu;

char *main_menu_items[MAIN_MENU_ITEMS];
gui_listbox_t main_menu = {7, 40, 220, MAIN_MENU_ITEMS, 4, 0, 0, main_menu_items};

const mem_chip_t *chip_list[NUM_CHIPS] = {&ram4027_chip, &ram4116_half_chip, &ram4116_chip,
                                 &ram4132_stk_chip, &ram4164_half_chip, &ram4164_chip,
                                 &ram41128_chip, &ram41256_chip, &ram4416_half_chip,
                                 &ram4416_chip, &ram4464_chip, &ram44256_chip};

gui_listbox_t variants_menu = {7, 40, 220, 0, 4, 0, 0, 0};
gui_listbox_t speed_menu = {7, 40, 220, 0, 4, 0, 0, 0};

gui_state_t gui_state = MAIN_MENU;

struct repeating_timer drum_timer;

queue_t call_queue;
queue_t results_queue;
