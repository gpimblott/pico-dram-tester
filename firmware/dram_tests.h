#ifndef DRAM_TESTS_H
#define DRAM_TESTS_H

#include <stdint.h>

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


#endif //DRAM_TESTS_H
