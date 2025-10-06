#ifndef DRAM_TESTS_H
#define DRAM_TESTS_H

#include <stdint.h>
#include <stdbool.h>

// Function prototypes
int ram_read(int addr);
void ram_write(int addr, int data);
uint32_t all_ram_tests(uint32_t addr_size, uint32_t bits);
void psrand_init_seeds();

// Function queue entry for dispatching worker functions
typedef struct
{
    uint32_t (*func)(uint32_t, uint32_t);
    uint32_t data;
    uint32_t data2;
} queue_entry_t;


/**
 * @brief Configuration structure for refresh stress testing.
 */
typedef struct {
    uint32_t min_delay_ms;          // Minimum refresh delay (e.g., 64ms - normal)
    uint32_t max_delay_ms;          // Maximum refresh delay (e.g., 2000ms - extreme)
    uint32_t delay_steps;           // Number of delay steps to test
    uint32_t pattern_iterations;    // How many patterns to test per delay
    bool test_data_retention;       // Test with data patterns
    bool test_charge_pump;          // Test charge pump recovery
    bool progressive_stress;        // Increase stress progressively
} refresh_stress_config_t;

/**
 * @brief Results structure for refresh stress testing.
 */
typedef struct {
    uint32_t max_working_delay_ms;  // Maximum delay that still works
    uint32_t first_failure_delay_ms; // First delay that causes failure
    uint32_t failed_addresses;      // Number of addresses that failed
    uint32_t weak_bit_mask;         // Bitmask of weak bits
    uint32_t pattern_failures[8];   // Failures per test pattern
} refresh_stress_results_t;

#endif //DRAM_TESTS_H
