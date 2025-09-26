/*
 * pio_patcher.c
 *
 * This file provides utilities for patching PIO (Programmable I/O) programs
 * at runtime, specifically to modify delay fields within the PIO instructions.
 * This allows for dynamic adjustment of timing parameters for DRAM communication.
 */

#include <string.h>
#include "hardware/pio.h"

// Buffer to hold the currently loaded PIO program instructions.
// PIO programs typically have a maximum of 32 instructions.
uint16_t current_pio_instructions[32];
// Structure to represent the currently active PIO program.
struct pio_program current_pio_program;

/**
 * @brief Copies a constant PIO program into an internal mutable buffer.
 *
 * This allows the PIO program instructions to be modified at runtime,
 * for example, to patch delay values.
 *
 * @param prog A pointer to the constant PIO program definition.
 */
void set_current_pio_program(const struct pio_program *prog)
{
    // Copy instruction data
    memcpy(current_pio_instructions, prog->instructions, prog->length * sizeof(uint16_t));
    // Update the mutable PIO program structure to point to our buffer
    current_pio_program.instructions = current_pio_instructions;
    current_pio_program.length = prog->length;
    current_pio_program.origin = prog->origin;
    current_pio_program.pio_version = prog->pio_version;
    current_pio_program.used_gpio_ranges = prog->used_gpio_ranges;
}

/**
 * @brief Returns a pointer to the currently loaded and mutable PIO program.
 *
 * This allows other parts of the code to access and potentially modify
 * the PIO program before it's loaded onto the PIO state machine.
 *
 * @return A pointer to the `current_pio_program` structure.
 */
struct pio_program *get_current_pio_program()
{
    return &current_pio_program;
}

/**
 * @brief Patches the delay/sideset fields of the current PIO program instructions.
 *
 * This function iterates through the PIO instructions and, for instructions
 * that use the delay/sideset feature, it replaces the delay value with a new
 * value looked up from the provided `delays` array. The original delay value
 * in the instruction is used as an index into the `delays` array.
 *
 * @param delays A pointer to an array of `uint8_t` values representing the new delay values.
 *               The index into this array is derived from the original instruction's delay field.
 * @param length The number of elements in the `delays` array, used for bounds checking.
 */
void pio_patch_delays(const uint8_t *delays, uint8_t length)
{
    uint8_t i;     // Loop counter for instructions
    uint8_t field; // Extracted delay field from instruction
    uint16_t instr; // Temporary variable to hold instruction during modification

    // Iterate through each instruction in the current PIO program
    for (i = 0; i < 31; i++) { // Assuming max 32 instructions, loop up to 31 for safety
        // Extract the 5-bit delay/sideset field (bits 8-12) from the instruction
        field = (current_pio_instructions[i] >> 8) & 0x1f;

        // Check if the field is a valid index into the delays array (0 is reserved)
        if ((field > 0) && (field < length)) {
            // Mask out the existing delay/sideset field (bits 8-12)
            instr = current_pio_instructions[i] & 0xe0ff;
            // Insert the new delay value from the `delays` array into the instruction
            // The new delay value is masked to 5 bits (0x1f) and shifted into position
            instr |= ((delays[field] & 0x1f) << 8);
            // Update the instruction in the buffer
            current_pio_instructions[i] = instr;
        }
    }
}

