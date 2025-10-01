# Copilot Instructions for pico-dram-tester Firmware

## Project Overview
This firmware project targets the Raspberry Pi Pico for testing various DRAM and SRAM chips. It is organized as a monolithic C codebase with clear separation between hardware abstraction, test logic, and user interface components.

## Architecture & Key Components
- **Test Logic**: `dram_tests.c`, `dram_tests.h` contain routines for DRAM/SRAM testing. Each chip type may have a dedicated test function.
- **Hardware Abstraction**: `hardware.c`, `hardware.h` manage low-level I/O, pin control, and chip-specific protocols.
- **User Interface**: `gui.c`, `gui.h`, `ui.c`, `ui.h` implement the display and user interaction logic, using custom fonts (`sserif*.c/h`) and icons (`icons.c/h`).
- **PIO Programs**: Files like `ram_4bit.pio`, `ram41128.pio`, etc., define custom PIO state machines for memory access. Their headers are auto-generated in `build/`.
- **Main Entry**: `pmemtest.c` is the main application entry point.

## Build & Flash Workflow
- **Build**: Use the VS Code task "Compile Project" (runs Ninja via CMake). Output is in `build/`.
- **Flash**: Use "Run Project" to load the firmware via `picotool`, or "Flash" to program via OpenOCD. Both tasks are preconfigured in `.vscode/tasks.json`.
- **Debug/Rescue**: "Rescue Reset" and "Risc-V Reset (RP2350)" tasks are available for recovery and advanced chip support.

## Conventions & Patterns
- **Chip Support**: Adding new memory chip support typically involves:
  - Creating a new `.pio` file for the protocol
  - Adding corresponding test logic in `dram_tests.c`
  - Updating hardware abstraction as needed
- **Generated Headers**: PIO headers are auto-generated in `build/` and should not be manually edited.
- **UI Assets**: Fonts and icons are stored as C arrays for direct framebuffer use.
- **No Standard Unit Tests**: Testing is performed on hardware; no conventional unit test framework is present.

## Integration Points
- **External Tools**: Relies on Pico SDK, Ninja, OpenOCD, and Picotool (paths are set in VS Code tasks).
- **Build System**: CMake is used for configuration; see `CMakeLists.txt` and `pico_sdk_import.cmake`.

## Examples
- To add a new DRAM chip:
  1. Create `ramXXXX.pio` for protocol
  2. Add test in `dram_tests.c`
  3. Update `hardware.c` if new pin logic is needed
  4. Rebuild and flash using tasks

## Key Files
- `dram_tests.c/h`, `hardware.c/h`, `gui.c/h`, `ui.c/h`, `pmemtest.c`, `CMakeLists.txt`, `build/`

---
For questions or unclear patterns, ask for clarification or examples from maintainers.