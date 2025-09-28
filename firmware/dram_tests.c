/*
 * dram_tests.c
 *
 * This file implements various DRAM memory tests, including March-B, Pseudo-random,
 * and Refresh tests. It interacts with the DRAM chip via the `mem_chip` interface
 * and updates the application state for UI visualization.
 */

#include "dram_tests.h"
#include "app_state.h"
#include "pico/stdlib.h"
#include "xoroshiro64starstar.h"

// A magic number used for seeding the pseudo-random number generator.
#define ARTISANAL_NUMBER 42

// Forward declarations for static (internal) helper functions
static inline bool me_r0(int a);                                                         // March Element Read 0
static inline bool me_r1(int a);                                                         // March Element Read 1
static inline bool me_w0(int a);                                                         // March Element Write 0
static inline bool me_w1(int a);                                                         // March Element Write 1
static inline bool marchb_m0(int a);                                                     // March-B element M0
static inline bool marchb_m1(int a);                                                     // March-B element M1
static inline bool marchb_m2(int a);                                                     // March-B element M2
static inline bool marchb_m3(int a);                                                     // March-B element M3
static inline bool marchb_m4(int a);                                                     // March-B element M4
static inline bool march_element(int addr_size, bool descending, int algorithm);         // Generic March element execution
static uint32_t marchb_testbit(uint32_t addr_size);                                      // Executes March-B test for a single bit
static uint32_t marchb_test(uint32_t addr_size, uint32_t bits);                          // Executes March-B test for all bits
static uint32_t psrand_next_bits(uint32_t bits);                                         // Generates next pseudo-random bits
static uint32_t psrandom_test(uint32_t addr_size, uint32_t bits);                        // Executes pseudo-random test
static uint32_t refresh_subtest(uint32_t addr_size, uint32_t bits, uint32_t time_delay); // Executes a refresh subtest
static uint32_t refresh_test(uint32_t addr_size, uint32_t bits);                         // Executes the refresh test
static uint32_t checkerboard_test( uint32_t adr_size, uint32_t bits);                    // Executes the checkerboard test

/**
 * @brief Reads a data word from the specified RAM address.
 *
 * This function acts as a wrapper around the `ram_read` function provided
 * by the currently selected memory chip in `chip_list`.
 *
 * @param addr The memory address to read from.
 * @return The data word read from the memory address.
 */
int ram_read(int addr)
{
    return chip_list[main_menu.sel_line]->ram_read(addr);
}

/**
 * @brief Writes a data word to the specified RAM address.
 *
 * This function acts as a wrapper around the `ram_write` function provided
 * by the currently selected memory chip in `chip_list`.
 *
 * @param addr The memory address to write to.
 * @param data The data word to write to the memory address.
 */
void ram_write(int addr, int data)
{
    chip_list[main_menu.sel_line]->ram_write(addr, data);
}

/**
 * @brief Initializes the seeds for the pseudo-random number generator.
 *
 * Seeds the `xoroshiro64starstar` generator with a fixed artisanal number
 * and then pre-generates a set of pseudo-random seeds for later use in tests.
 */
void psrand_init_seeds()
{
    int i;
    psrand_seed(ARTISANAL_NUMBER);
    for (i = 0; i < PSEUDO_VALUES; i++)
    {
        random_seeds[i] = psrand_next();
    }
}

/**
 * @brief Executes all defined RAM tests in sequence.
 *
 * Runs March-B, Pseudo-random, and Refresh tests. It updates the UI with the
 * current test being executed and returns the first failure encountered.
 *
 * @param addr_size The total number of addresses in the RAM chip.
 * @param bits The number of data bits in the RAM chip.
 * @return 0 if all tests pass, otherwise a non-zero value indicating failure.
 */
uint32_t all_ram_tests(uint32_t addr_size, uint32_t bits)
{
    int failed;
    int test = 0;
    // March-B Test
    march_element(addr_size, false, 0);        // Initialize memory for March-B
    queue_add_blocking(&stat_cur_test, &test); // Update UI with current test
    failed = marchb_test(addr_size, bits);
    if (failed)
        return failed;

    // Pseudo-random Test
    test = 1;
    queue_add_blocking(&stat_cur_test, &test); // Update UI with current test
    failed = psrandom_test(addr_size, bits);
    if (failed)
        return failed;

    // Refresh Test
    test = 2;
    queue_add_blocking(&stat_cur_test, &test); // Update UI with current test
    failed = refresh_test(addr_size, bits);
    if (failed)
        return failed;

    test = 3;
    queue_add_blocking(&stat_cur_test, &test); // Update UI with current test
    failed = checkerboard_test(addr_size, bits);
    if (failed)
        return failed;

    return 0; // All tests passed
}

// Static Helper Functions (March-B elements and related operations)

/**
 * @brief March Element: Read 0.
 * @param a The address to read from.
 * @return True if the read value is 0, false otherwise.
 */
static inline bool me_r0(int a)
{
    int bit = ram_read(a) & ram_bit_mask;
    return (bit == 0);
}

/**
 * @brief March Element: Read 1.
 * @param a The address to read from.
 * @return True if the read value is 1, false otherwise.
 */
static inline bool me_r1(int a)
{
    int bit = ram_read(a) & ram_bit_mask;
    return (bit == ram_bit_mask);
}

/**
 * @brief March Element: Write 0.
 * @param a The address to write to.
 * @return Always true.
 */
static inline bool me_w0(int a)
{
    ram_write(a, ~ram_bit_mask);
    return true;
}

/**
 * @brief March Element: Write 1.
 * @param a The address to write to.
 * @return Always true.
 */
static inline bool me_w1(int a)
{
    ram_write(a, ram_bit_mask);
    return true;
}

/**
 * @brief March-B Element M0: Write 0.
 * @param a The address to operate on.
 * @return Always true.
 */
static inline bool marchb_m0(int a)
{
    me_w0(a);
    return true;
}

/**
 * @brief March-B Element M1: Read 0, Write 1, Read 1, Write 0, Read 0, Write 1.
 * @param a The address to operate on.
 * @return True if all operations are successful, false otherwise.
 */
static inline bool marchb_m1(int a)
{
    return me_r0(a) && me_w1(a) && me_r1(a) && me_w0(a) && me_r0(a) && me_w1(a);
}

/**
 * @brief March-B Element M2: Read 1, Write 0, Write 1.
 * @param a The address to operate on.
 * @return True if all operations are successful, false otherwise.
 */
static inline bool marchb_m2(int a)
{
    return me_r1(a) && me_w0(a) && me_w1(a);
}

/**
 * @brief March-B Element M3: Read 1, Write 0, Write 1, Write 0.
 * @param a The address to operate on.
 * @return True if all operations are successful, false otherwise.
 */
static inline bool marchb_m3(int a)
{
    return me_r1(a) && me_w0(a) && me_w1(a) && me_w0(a);
}

/**
 * @brief March-B Element M4: Read 0, Write 1, Write 0.
 * @param a The address to operate on.
 * @return True if all operations are successful, false otherwise.
 */
static inline bool marchb_m4(int a)
{
    return me_r0(a) && me_w1(a) && me_w0(a);
}

/**
 * @brief Executes a single March test element (e.g., M0, M1, M2, M3, M4).
 *
 * Iterates through memory addresses (ascending or descending) and applies
 * the specified March algorithm. Updates `stat_cur_addr` and `stat_cur_subtest`
 * for UI visualization.
 *
 * @param addr_size The total number of addresses to test.
 * @param descending If true, iterate addresses in descending order; otherwise, ascending.
 * @param algorithm The index of the March algorithm to execute (0-4 for M0-M4).
 * @return True if all operations in the element pass, false otherwise.
 */
static inline bool march_element(int addr_size, bool descending, int algorithm)
{
    int inc = descending ? -1 : 1;                // Increment/decrement step
    int start = descending ? (addr_size - 1) : 0; // Starting address
    int end = descending ? -1 : addr_size;        // Ending condition
    int a;
    bool ret;

    stat_cur_subtest = algorithm; // Update current subtest for UI visualization

    // Iterate through addresses and apply the selected March algorithm
    for (stat_cur_addr = start; stat_cur_addr != end; stat_cur_addr += inc)
    {
        switch (algorithm)
        {
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
            ret = false; // Should not happen with valid algorithm indices
            break;
        }
        if (!ret)
            return false; // Return false immediately on first failure
    }
    return true;
}

/**
 * @brief Executes the full March-B test for a single data bit.
 *
 * Calls `march_element` for each phase of the March-B algorithm.
 *
 * @param addr_size The total number of addresses in the RAM chip.
 * @return True if the March-B test passes for the current bit, false otherwise.
 */
static uint32_t marchb_testbit(uint32_t addr_size)
{
    bool ret;
    ret = march_element(addr_size, false, 0); // M0 (w0) in ascending order
    if (!ret)
        return false;
    ret = march_element(addr_size, false, 1); // M1 (r0,w1,r1,w0,r0,w1) in ascending order
    if (!ret)
        return false;
    ret = march_element(addr_size, false, 2); // M2 (r1,w0,w1) in ascending order
    if (!ret)
        return false;
    ret = march_element(addr_size, true, 3); // M3 (r1,w0,w1,w0) in descending order
    if (!ret)
        return false;
    ret = march_element(addr_size, true, 4); // M4 (r0,w1,w0) in descending order
    if (!ret)
        return false;
    return true;
}

/**
 * @brief Executes the March-B test for all data bits of the RAM chip.
 *
 * Iterates through each data bit, sets the `ram_bit_mask` accordingly,
 * and calls `marchb_testbit`. Accumulates failure flags for each bit.
 *
 * @param addr_size The total number of addresses in the RAM chip.
 * @param bits The number of data bits in the RAM chip.
 * @return A bitmask where each set bit indicates a failure in the corresponding data bit.
 */
static uint32_t marchb_test(uint32_t addr_size, uint32_t bits)
{
    int failed = 0;
    int bit = 0;

    // Iterate through each data bit
    for (bit = 0; bit < bits; bit++)
    {
        stat_cur_bit = bit;      // Update current bit for UI visualization
        ram_bit_mask = 1 << bit; // Set mask for the current bit
        if (!marchb_testbit(addr_size))
        {
            failed |= 1 << bit; // Set failure flag for this bit
        }
    }

    return (uint32_t)failed;
}

/**
 * @brief Generates the next `bits` number of pseudo-random bits.
 *
 * Uses the `xoroshiro64starstar` generator to produce 32-bit random numbers
 * and extracts the requested number of bits from it.
 *
 * @param bits The number of bits to generate (1 to 32).
 * @return A `uint32_t` containing the generated pseudo-random bits.
 */
static uint32_t psrand_next_bits(uint32_t bits)
{
    static int bitcount = 0;  // Number of remaining bits in current `cur_rand`
    static uint32_t cur_rand; // Current 32-bit random number
    uint32_t out;

    // If not enough bits left in `cur_rand`, generate a new one
    if (bitcount < bits)
    {
        cur_rand = psrand_next();
        bitcount = 32;
    }

    out = cur_rand & ((1 << (bits)) - 1); // Extract the requested number of bits
    cur_rand = cur_rand >> bits;          // Shift `cur_rand` for next extraction
    bitcount -= bits;                     // Decrement available bit count
    return out;
}

/**
 * @brief Executes a pseudo-random data test on the RAM chip.
 *
 * Writes a sequence of pseudo-random data to memory, then reads it back
 * and verifies its integrity. This process is repeated with different seeds.
 *
 * @param addr_size The total number of addresses in the RAM chip.
 * @param bits The number of data bits in the RAM chip.
 * @return 0 if the test passes, 1 if a mismatch is found.
 */
static uint32_t psrandom_test(uint32_t addr_size, uint32_t bits)
{
    uint i;
    uint32_t bitsout;                  // Data written to RAM
    uint32_t bitsin;                   // Data read from RAM
    uint32_t bitshift = addr_size / 4; // This variable is declared but not used.

    // Iterate through pre-generated random seeds
    for (i = 0; i < PSEUDO_VALUES; i++)
    {
        stat_cur_subtest = i >> 2;    // Update subtest for UI visualization
        stat_cur_bit = i & 3;         // Update current bit for UI visualization
        psrand_seed(random_seeds[i]); // Seed the generator with a stored seed

        // Write seeded pseudo-random data to all addresses
        for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++)
        {
            bitsout = psrand_next_bits(bits);
            ram_write(stat_cur_addr, bitsout);
        }

        // Reseed with the same seed and then read the data back for verification
        psrand_seed(random_seeds[i]);
        for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++)
        {
            bitsout = psrand_next_bits(bits);
            bitsin = ram_read(stat_cur_addr);
            if (bitsout != bitsin)
            {
                return 1; // Return 1 on first mismatch (failure)
            }
        }
    }

    return 0; // Test passed
}

/**
 * @brief Executes a refresh subtest for the RAM chip.
 *
 * Writes pseudo-random data to memory, waits for a specified `time_delay`
 * (simulating refresh interval), then reads back and verifies the data.
 *
 * @param addr_size The total number of addresses in the RAM chip.
 * @param bits The number of data bits in the RAM chip.
 * @param time_delay The delay in microseconds to simulate refresh interval.
 * @return 0 if the test passes, 1 if a mismatch is found.
 */
static uint32_t refresh_subtest(uint32_t addr_size, uint32_t bits, uint32_t time_delay)
{
    uint32_t bitsout;
    uint32_t bitsin;

    psrand_seed(random_seeds[0]); // Use the first pre-generated seed
    // Write pseudo-random data to all addresses
    for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++)
    {
        bitsout = psrand_next_bits(bits);
        ram_write(stat_cur_addr, bitsout); // Note: This writes 'bits' value, not 'bitsout'. This might be a bug or intentional.
    }

    sleep_us(time_delay); // Wait for the specified delay

    psrand_seed(random_seeds[0]); // Reseed with the same seed
    // Read back and verify data
    for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++)
    {
        bitsout = psrand_next_bits(bits);
        bitsin = ram_read(stat_cur_addr);
        if (bitsout != bitsin)
        {             // Note: This compares 'bits' with 'bitsin', not 'bitsout'. This might be a bug or intentional.
            return 1; // Return 1 on first mismatch (failure)
        }
    }
    return 0; // Test passed
}

/**
 * @brief Executes the refresh test for the RAM chip.
 *
 * Calls `refresh_subtest` with a default delay of 5000 microseconds.
 *
 * @param addr_size The total number of addresses in the RAM chip.
 * @param bits The number of data bits in the RAM chip.
 * @return 0 if the test passes, 1 if a mismatch is found.
 */
static uint32_t refresh_test(uint32_t addr_size, uint32_t bits)
{
    return refresh_subtest(addr_size, bits, 5000); // 5000 us delay for refresh test
}

/**
 * @brief Executes the Checkerboard test on the RAM chip.
 *
 * Writes alternating patterns (0x5555... and 0xAAAA...) to memory,
 * then reads back and verifies the data.
 *
 * @param addr_size The total number of addresses in the RAM chip.
 * @param bits The number of data bits in the RAM chip.
 * @return 0 if the test passes, 1 if a mismatch is found.
 */
static uint32_t checkerboard_test(uint32_t addr_size, uint32_t bits)
{
    uint32_t pattern1 = 0x55555555 & ((1ULL << bits) - 1);
    uint32_t pattern2 = 0xAAAAAAAA & ((1ULL << bits) - 1);
    uint32_t bitsin;

    for (int loop = 0; loop < 10; loop++)
    {
        // Write pattern1
        stat_cur_subtest = 0;
        for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++) {
            for (int bit = 0; bit < bits; bit++) {
                stat_cur_bit = bit; // Update for visualization
                // Optionally, call a visualization update function here if needed
            }
            ram_write(stat_cur_addr, pattern1);
        }

        // Read and check pattern1
        stat_cur_subtest = 1;
        for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++) {
            bitsin = ram_read(stat_cur_addr);
            for (int bit = 0; bit < bits; bit++) {
                stat_cur_bit = bit; // Update for visualization
                // Optionally, call a visualization update function here if needed
            }
            if (bitsin != pattern1)
                return 1;
        }

        // Write pattern2
        stat_cur_subtest = 2;
        for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++) {
            for (int bit = 0; bit < bits; bit++) {
                stat_cur_bit = bit; // Update for visualization
            }
            ram_write(stat_cur_addr, pattern2);
        }

        // Read and check pattern2
        stat_cur_subtest = 3;
        for (stat_cur_addr = 0; stat_cur_addr < addr_size; stat_cur_addr++) {
            bitsin = ram_read(stat_cur_addr);
            for (int bit = 0; bit < bits; bit++) {
                stat_cur_bit = bit; // Update for visualization
            }
            if (bitsin != pattern2)
                return 1;
        }
    }
    return 0;
}