#include <stdio.h>
#include <stdint.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "pico/util/queue.h"
#include "hardware/pio.h"
#include "pio_patcher.h"
#include "mem_chip.h"

PIO pio;
uint sm = 0;
uint offset; // Returns offset of starting instruction

// Defined RAM pio programs
#include "ram4116.pio.h"
#include "ram4164.pio.h"
#include "ram41256.pio.h"

#include "st7789.h"

// Icons
#include "chip_icon.h"
#include "warn_icon.h"
#include "error_icon.h"
#include "check_icon.h"
#include "drum_icon0.h"
#include "drum_icon1.h"
#include "drum_icon2.h"
#include "drum_icon3.h"

#include "gui.h"


#define GPIO_POWER 4
#define GPIO_QUAD_A 22
#define GPIO_QUAD_B 26
#define GPIO_QUAD_BTN 27
#define GPIO_BACK_BTN 28
#define GPIO_LED 25

volatile int cur_addr;

gui_listbox_t *cur_menu;

#define MAIN_MENU_ITEMS 8
char *main_menu_items[MAIN_MENU_ITEMS];
gui_listbox_t main_menu = {7, 40, 220, MAIN_MENU_ITEMS, 4, 0, 0, main_menu_items};

#define NUM_CHIPS 3
const mem_chip_t *chip_list[] = {&ram4116_chip, &ram4164_chip, &ram41256_chip};

gui_listbox_t speed_menu = {7, 40, 220, 0, 4, 0, 0, 0};

typedef enum {
    MAIN_MENU,
    SPEED_MENU,
    DO_SOCKET,
    DO_TEST,
    TEST_RESULTS
} gui_state_t;

gui_state_t gui_state = MAIN_MENU;

void setup_main_menu()
{
    uint i;
    for (i = 0; i < NUM_CHIPS; i++) {
        main_menu_items[i] = (char *)chip_list[i]->chip_name;
    }
    main_menu.tot_lines = NUM_CHIPS;
}

// Function queue entry for dispatching worker functions
typedef struct
{
    uint32_t (*func)(uint32_t);
    uint32_t data;
} queue_entry_t;

queue_t call_queue;
queue_t results_queue;

// Entry point for second core. This is just a generic
// function dispatcher lifted from the Raspberry Pi example code.
void core1_entry() {
    while (1) {
        // Function pointer is passed to us via the queue_entry_t which also
        // contains the function parameter.
        // We provide an int32_t return value by simply pushing it back on the
        // return queue which also indicates the result is ready.

        queue_entry_t entry;

        queue_remove_blocking(&call_queue, &entry);

        int32_t result = entry.func(entry.data);

        queue_add_blocking(&results_queue, &result);
    }
}

// Routines for turning on-board power on and off
static inline void power_on()
{
    gpio_set_dir(GPIO_POWER, true);
    gpio_put(GPIO_POWER, false);
    sleep_ms(100);
}

static inline void power_off()
{
    gpio_set_dir(GPIO_POWER, false);
}

// Wrapper that just calls the read routine for the selected chip
static inline int ram_read(int addr)
{
    return chip_list[main_menu.sel_line]->ram_read(addr);
}

// Wrapper that just calls the write routine for the selected chip
static inline void ram_write(int addr, int data)
{
    chip_list[main_menu.sel_line]->ram_write(addr, data);
}

int addr_map(int addr)
{
    return addr;
 //   return (addr >> 8) | ((addr & 0xFF) << 8);
}

// Fills the RAM with the provided data.
void ram_fill(int range, int data)
{
    int i;
    for (i = 0; i < range; i++) {
        ram_write(addr_map(i), data);
    }
}

static inline bool me_r0(int a)
{
    int bit = ram_read(a);
    return (bit == 0);
}

static inline bool me_r1(int a)
{
    int bit = ram_read(a);
    return (bit == 1);
}

static inline bool me_w0(int a)
{
    ram_write(a, 0);
    return true;
}

static inline bool me_w1(int a)
{
    ram_write(a, 1);
    return true;
}

static inline bool marchb_m0(int a)
{
    me_w0(a);
    return true;
}

static inline bool marchb_m1(int a)
{
    return me_r0(a) && me_w1(a) && me_r1(a) && me_w0(a) && me_r0(a) && me_w1(a);
}

static inline bool marchb_m2(int a)
{
    return me_r1(a) && me_w0(a) && me_w1(a);
}

static inline bool marchb_m3(int a)
{
    return me_r1(a) && me_w0(a) && me_w1(a) && me_w0(a);
}

static inline bool marchb_m4(int a)
{
    return me_r0(a) && me_w1(a) && me_w0(a);
}

static inline bool march_element(int addr_size, bool descending, int algorithm)
{
    int inc = descending ? -1 : 1;
    int start = descending ? (addr_size - 1) : 0;
    int end = descending ? -1 : addr_size;
    int a;
    bool ret;

    for (cur_addr = start; cur_addr != end; cur_addr += inc) {
        switch (algorithm) {
            case 0:
                ret = marchb_m0(cur_addr);
                break;
            case 1:
                ret = marchb_m1(cur_addr);
                break;
            case 2:
                ret = marchb_m2(cur_addr);
                break;
            case 3:
                ret = marchb_m3(cur_addr);
                break;
            case 4:
                ret = marchb_m4(cur_addr);
                break;
            default:
                break;
        }
        if (!ret) return false;
    }
    return true;
}

uint32_t marchb_test(uint32_t addr_size)
{
    bool ret = true;
    me_w0(0); // ES Hack
    printf("M0 ");
    ret = march_element(addr_size, false, 0);
    if (!ret) return false;
    printf("M1 ");
    ret = march_element(addr_size, false, 1);
    if (!ret) return false;
    printf("M2 ");
    ret = march_element(addr_size, false, 2);
    if (!ret) return false;
    printf("M3 ");
    ret = march_element(addr_size, true, 3);
    if (!ret) return false;
    printf("M4 ");
    ret = march_element(addr_size, true, 4);
    if (!ret) return false;
    return (uint32_t)ret;
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


typedef struct {
    uint32_t pin;
    uint32_t hcount;
} pin_debounce_t;

#define DEBOUNCE_COUNT 1000

// Debounces a pin
uint8_t do_debounce(pin_debounce_t *d)
{
    if (gpio_get(d->pin)) {
        d->hcount++;
        if (d->hcount > DEBOUNCE_COUNT) d->hcount = DEBOUNCE_COUNT;
    } else {
        d->hcount = 0;
    }
    return (d->hcount >= DEBOUNCE_COUNT) ? 1 : 0;
}

// Returns true only *once* when a button is pushed. No key repeat.
bool is_button_pushed(pin_debounce_t *pin_b)
{
    if (!gpio_get(pin_b->pin)) {
        if (pin_b->hcount < DEBOUNCE_COUNT) {
            pin_b->hcount++;
            if (pin_b->hcount == DEBOUNCE_COUNT) {
                return true;
            }
        }
    } else {
        pin_b->hcount = 0;
    }
    return false;
}

// Setup and display the main menu
void show_main_menu()
{
    cur_menu = &main_menu;
    paint_dialog("Select Device");
    gui_listbox(cur_menu, LIST_ACTION_NONE);
}

// With the selected chip, populate the speed grade menu and show it
void show_speed_menu()
{
    uint chip = main_menu.sel_line;
    cur_menu = &speed_menu;
    paint_dialog("Select Speed Grade");
    speed_menu.items = (char **)chip_list[chip]->speed_names;
    speed_menu.tot_lines = chip_list[chip]->speed_grades;
    gui_listbox(cur_menu, LIST_ACTION_NONE);
}

typedef enum {
    UNTESTED,
    TESTING,
    PASSED,
    FAILED
} test_status_t;

#define CELL_STAT_X 9
#define CELL_STAT_Y 33

// Used to update the RAM test GUI left pane
void update_test_gui(uint16_t addr, test_status_t status)
{
    uint16_t cx, cy, col;
    cx = addr & 0x1f;
    cy = (addr >> 5) & 0x1f; // 10 bits are used
    switch (status) {
        case UNTESTED:
            col = COLOR_DKGRAY; // Untested
            break;
        case TESTING:
            col = COLOR_RED;
            break;
        case PASSED:
            col = COLOR_GREEN;
            break;
        case FAILED:
            col = COLOR_RED;
            break;
    }
    st7789_fill(CELL_STAT_X + cx * 3, CELL_STAT_Y + cy * 3, 2, 2, col);
}

// Show the RAM test console GUI
void show_test_gui()
{
    uint16_t cx, cy;
    paint_dialog("Testing...");

    // Cell status area. 32x32 elements.
    fancy_rect(7, 31, 100, 100, B_SUNKEN_OUTER); // Usable size is 220x80.
    fancy_rect(8, 32, 98, 98, B_SUNKEN_INNER);
    st7789_fill(9, 33, 96, 96, COLOR_BLACK);
//    for (cx = 0; cx < 32; cx++) {
//        for (cy = 0; cy < 32; cy++) {
//            st7789_fill(CELL_STAT_X + cx * 3, CELL_STAT_Y + cy * 3, 2, 2, COLOR_GREEN);
//        }
//    }
    for (cx = 0; cx < 1024; cx++) {
        update_test_gui(cx, UNTESTED);
    }

    // Current test indicator
    paint_status(120, 45, 100, "March B");
    draw_icon(164, 80, &drum_icon0);
}

// Begins the RAM test with the selected RAM chip
void start_the_ram_test()
{
    // Get the power turned on
    power_on();

    // Get the PIO going
    chip_list[main_menu.sel_line]->setup_pio(speed_menu.sel_line);

    // Dispatch the second core
    // (The memory size is from our memory description data structure)
    queue_entry_t entry = {marchb_test, chip_list[main_menu.sel_line]->mem_size};
    queue_add_blocking(&call_queue, &entry);
}

// Stops the RAM test
void stop_the_ram_test()
{
    chip_list[main_menu.sel_line]->teardown_pio();
    power_off();
}

// During a RAM test, updates the status window and checks for the end of the test
void do_status()
{
    uint32_t retval;
    char retstring[30];
    uint16_t v;
    static uint16_t v_prev = 0;
    static uint16_t drum_anim = 0;
    static uint8_t drum_st = 0;

    if (gui_state == DO_TEST) {
        // TODO: Visualizations
        v = cur_addr >> 6; // FIXME: depends on our address space
        update_test_gui(v, TESTING);
        update_test_gui(v_prev, PASSED);
        v_prev = v;

        // Drum animation
        drum_anim++;
        if (drum_anim > 8191) {
            drum_anim = 0;
            drum_st++;
            if (drum_st > 3) drum_st = 0;
            st7789_fill(164, 80, 32, 32, COLOR_LTGRAY);
            switch (drum_st) {
                case 0:
                    draw_icon(164, 80, &drum_icon0);
                    break;
                case 1:
                    draw_icon(164, 80, &drum_icon1);
                    break;
                case 2:
                    draw_icon(164, 80, &drum_icon2);
                    break;
                case 3:
                    draw_icon(164, 80, &drum_icon3);
                    break;
            }
        }

    // Check official status
        if (!queue_is_empty(&results_queue)) {
            stop_the_ram_test();
            // The RAM test completed, so let's handle that
            sleep_ms(100);
            queue_remove_blocking(&results_queue, &retval);
            // Show the completion status
            gui_state = TEST_RESULTS;
            st7789_fill(164, 80, 32, 32, COLOR_LTGRAY);
            if (retval) {
                paint_status(120, 45, 100, "Passed!");
                draw_icon(164, 80, &check_icon);
            } else {
                paint_status(120, 45, 100, "Failed.");
                draw_icon(164, 80, &error_icon);
            }
            // TODO: Some sort of status text indicating the type of failure?
        }
    }
}

// Called when user presses the action button
void button_action()
{
    // Do something based on the current menu
    switch (gui_state) {
        case MAIN_MENU:
            gui_state = SPEED_MENU;
            show_speed_menu();
            break;
        case SPEED_MENU:
            gui_messagebox("Place Chip in Socket",
                           "Turn on external supply afterwards, if used.", &chip_icon);
            gui_state = DO_SOCKET;
            break;
        case DO_SOCKET:
            gui_state = DO_TEST;
            show_test_gui();
            start_the_ram_test();
            break;
        case DO_TEST:
            break;
        case TEST_RESULTS:
            // TODO: Maybe make it easy to run the test again?
            break;
        default:
            gui_state = MAIN_MENU;
            break;
    }
}

// Called when the user presses the back button
void button_back()
{
    switch (gui_state) {
        case MAIN_MENU:
            break;
        case SPEED_MENU:
            gui_state = MAIN_MENU;
            show_main_menu();
            break;
        case DO_SOCKET:
            gui_state = SPEED_MENU;
            show_speed_menu();
            break;
        case DO_TEST:
            gui_state = SPEED_MENU;
            show_speed_menu();
            break;
        case TEST_RESULTS:
            gui_state = MAIN_MENU;
            show_main_menu();
            break;
        default:
            gui_state = MAIN_MENU;
            break;
    }
}

void do_buttons()
{
    static pin_debounce_t action_btn = {GPIO_QUAD_BTN, 0};
    static pin_debounce_t back_btn = {GPIO_BACK_BTN, 0};
    if (is_button_pushed(&action_btn)) button_action();
    if (is_button_pushed(&back_btn)) button_back();
}

void wheel_increment()
{
    if (gui_state == MAIN_MENU || gui_state == SPEED_MENU) {
        gui_listbox(cur_menu, LIST_ACTION_DOWN);
    }
}

void wheel_decrement()
{
    if (gui_state == MAIN_MENU || gui_state == SPEED_MENU) {
        gui_listbox(cur_menu, LIST_ACTION_UP);
    }
}

void do_encoder()
{
    static pin_debounce_t pin_a = {GPIO_QUAD_A, 0};
    static pin_debounce_t pin_b = {GPIO_QUAD_B, 0};
    static uint8_t wheel_state_old = 0;
    uint8_t st;
    uint8_t wheel_state;

    wheel_state = do_debounce(&pin_a) | (do_debounce(&pin_b) << 1);
    st = wheel_state | (wheel_state_old << 4);
    if (wheel_state != wheel_state_old) {
        // Present state, next state
        // 00 -> 01 clockwise
        // 10 -> 11 counterclockwise
        // 11 -> 10 clockwise
        // 01 -> 00 counterclockwise
        if ((st == 0x01) || (st == 0x32)) {
            wheel_increment();
        }
        if ((st == 0x23) || (st == 0x10)) {
            wheel_decrement();
        }
        wheel_state_old = wheel_state;
    }
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

int main() {
    uint offset;
    uint16_t addr;
    uint8_t db = 0;
    uint din = 0;
    int i, retval;

    // PLL->prim = 0x51000.

    //stdio_uart_init_full(uart0, 57600, 28, 29); // 28=tx, 29=rx actually runs at 115200 due to overclock
    //gpio_init(15);
    //gpio_set_dir(15, GPIO_OUT);

    //printf("Test.\n");

    gpio_init(GPIO_LED);
    gpio_set_dir(GPIO_LED, GPIO_OUT);
    gpio_put(GPIO_LED, 1);

    gpio_init(GPIO_POWER);
    power_off();

    // Set up second core
    queue_init(&call_queue, sizeof(queue_entry_t), 2);
    queue_init(&results_queue, sizeof(int32_t), 2);

    // Second core will wait for the call queue.
    multicore_launch_core1(core1_entry);

    // Init display
    st7789_init();

    setup_main_menu();
 //   gui_demo();
    show_main_menu();
    init_buttons_encoder();


// Testing
#if 0
    power_on();
//    sleep_ms(500);
    prepare_ram_pio();
//    sleep_ms(10);
        for (i=0; i < 100; i++) {
//            ram_read(i&7);
//            ram_write(i&7, 1);
//            ram_read(i&7);
//            ram_write(i&7, 0);
              gpio_put(GPIO_LED, marchb_test(8));
        }
    while(1) {}
#endif

    while(1) {
        do_encoder();
        do_buttons();
        do_status();
    }

    while(1) {
        //retval = ramtest(65536);
//        printf("Begin march test.\n");
        retval = marchb_test(65536);
//        printf("Rv: %d\n", retval);
    }

    return 0;
}
