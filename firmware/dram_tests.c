#include "dram_tests.h"
#include "app_state.h"
#include "pico/stdlib.h"
#include "xoroshiro64starstar.h"

#define ARTISANAL_NUMBER 42

// Forward declarations for static functions
static inline bool me_r0(int a);
static inline bool me_r1(int a);
static inline bool me_w0(int a);
static inline bool me_w1(int a);
static inline bool marchb_m0(int a);
static inline bool marchb_m1(int a);
static inline bool marchb_m2(int a);
static inline bool marchb_m3(int a);
static inline bool marchb_m4(int a);
static inline bool march_element(int addr_size, bool descending, int algorithm);
static uint32_t marchb_testbit(uint32_t addr_size);
static uint32_t marchb_test(uint32_t addr_size, uint32_t bits);
static uint32_t psrand_next_bits(uint32_t bits);
static uint32_t psrandom_test(uint32_t addr_size, uint32_t bits);
static uint32_t refresh_subtest(uint32_t addr_size, uint32_t bits, uint32_t time_delay);
static uint32_t refresh_test(uint32_t addr_size, uint32_t bits);


// Public Functions

int ram_read(int addr)
{
    return chip_list[main_menu.sel_line]->ram_read(addr);
}

void ram_write(int addr, int data)
{
    chip_list[main_menu.sel_line]->ram_write(addr, data);
}

void psrand_init_seeds()
{
    int i;
    psrand_seed(ARTISANAL_NUMBER);
    for (i = 0; i < PSEUDO_VALUES; i++) {
        random_seeds[i] = psrand_next();
    }
}

uint32_t all_ram_tests(uint32_t addr_size, uint32_t bits)
{
    int failed;
    int test = 0;
    march_element(addr_size, false, 0);
    queue_add_blocking(&stat_cur_test, &test);
    failed = marchb_test(addr_size, bits);
    if (failed) return failed;
    test = 1;
    queue_add_blocking(&stat_cur_test, &test);
    failed = psrandom_test(addr_size, bits);
    if (failed) return failed;
    test = 2;
    queue_add_blocking(&stat_cur_test, &test);
    failed = refresh_test(addr_size, bits);
    if (failed) return failed;
    return 0;
}


// Static Helper Functions

static inline bool me_r0(int a)
{
    int bit = ram_read(a) & ram_bit_mask;
    return (bit == 0);
}

static inline bool me_r1(int a)
{
    int bit = ram_read(a) & ram_bit_mask;
    return (bit == ram_bit_mask);
}

static inline bool me_w0(int a)
{
    ram_write(a, ~ram_bit_mask);
    return true;
}

static inline bool me_w1(int a)
{
    ram_write(a, ram_bit_mask);
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

    stat_cur_subtest = algorithm;

    for (stat_cur_addr = start; stat_cur_addr != end; stat_cur_addr += inc) {
        switch (algorithm) {
            case 0:
                ret = marchb_m0(stat_cur_addr);
                break;
            case 1:
                ret = marchb_m1(stat_cur_addr);
                break;
            case 2:
                ret = marchb_m2(stat_cur_addr);
                break;
            case 3:
                ret = marchb_m3(stat_cur_addr);
                break;
            case 4:
                ret = marchb_m4(stat_cur_addr);
                break;
            default:
                break;
        }
        if (!ret) return false;
    }
    return true;
}

static uint32_t marchb_testbit(uint32_t addr_size)
{
    bool ret;
    ret = march_element(addr_size, false, 0);
    if (!ret) return false;
    ret = march_element(addr_size, false, 1);
    if (!ret) return false;
    ret = march_element(addr_size, false, 2);
    if (!ret) return false;
    ret = march_element(addr_size, true, 3);
    if (!ret) return false;
    ret = march_element(addr_size, true, 4);
    if (!ret) return false;
    return true;
}

static uint32_t marchb_test(uint32_t addr_size, uint32_t bits)
{
    int failed = 0;
    int bit = 0;

    for (bit = 0; bit < bits; bit++) {
        stat_cur_bit = bit;
        ram_bit_mask = 1 << bit;
        if (!marchb_testbit(addr_size)) {
            failed |= 1 << bit; // fail flag
        }
    }

    return (uint32_t)failed;
}

static uint32_t psrand_next_bits(uint32_t bits)
{
    static int bitcount = 0;
    static uint32_t cur_rand;
    uint32_t out;

    if (bitcount < bits) {
        cur_rand = psrand_next();
        bitcount = 32;
    }

    out = cur_rand & ((1 << (bits)) - 1);
    cur_rand = cur_rand >> bits;
    bitcount -= bits;
    return out;
}


static uint32_t psrandom_test(uint32_t addr_size, uint32_t bits)
{
    uint i;
    uint32_t bitsout;
    uint32_t bitsin;
    uint32_t bitshift = addr_size / 4;

    // Write seeded pseudorandom data
    for (i = 0; i < PSEUDO_VALUES; i++) {
        stat_cur_subtest = i >> 2;
        stat_cur_bit = i & 3;
        psrand_seed(random_seeds[i]);
        for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++) {
            bitsout = psrand_next_bits(bits);
            ram_write(stat_cur_addr, bitsout);
        }

        // Reseed and then read the data back
        psrand_seed(random_seeds[i]);
        for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++) {
            bitsout = psrand_next_bits(bits);
            bitsin = ram_read(stat_cur_addr);
            if (bitsout != bitsin) {
                return 1;
            }
        }
    }

    return 0;
}

static uint32_t refresh_subtest(uint32_t addr_size, uint32_t bits, uint32_t time_delay)
{
    uint32_t bitsout;
    uint32_t bitsin;

    psrand_seed(random_seeds[0]);
    for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++) {
        bitsout = psrand_next_bits(bits);
        ram_write(stat_cur_addr, bits);
    }

    sleep_us(time_delay);

    psrand_seed(random_seeds[0]);
    for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++) {
        bitsout = psrand_next_bits(bits);
        bitsin = ram_read(stat_cur_addr);
        if (bits != bitsin) {
            return 1;
        }
    }
    return 0;
}


static uint32_t refresh_test(uint32_t addr_size, uint32_t bits)
{
    return refresh_subtest(addr_size, bits, 5000);
}