#ifndef MEMCHIP_H
#define MEMCHIP_H

typedef struct {
    uint8_t num_variants;
    const char *variant_names[];
} mem_chip_variants_t;

typedef struct {
    void (*setup_pio)(uint speed_grade, uint variant);
    void (*teardown_pio)();
    int (*ram_read)(int addr);
    void (*ram_write)(int addr, int data);
    uint32_t mem_size;
    uint32_t bits;
    const mem_chip_variants_t *variants;
    uint8_t speed_grades;
    const char *chip_name;
    const char *speed_names[];
} mem_chip_t;



#endif
