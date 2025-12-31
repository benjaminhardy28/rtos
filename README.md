# This a custom RTOS built for an ARM Cortex-M processor.

## Project Layout

## `kernel/`
This represents the OS logic and is platform agnostic.
- scheduler and task management
- synchronization primitives (mutexes, semaphores, queues)
- time/tick logic
- generic utils

## `arch/` 
### `arm_cm/`
This contains ARM Cortex-M specific code.
- startup/reset code and vector table
- context switching (PendSV) and system calls (SVC)
- interrupt enable/disable + critical sections
- SysTick setup
- stack frame layout, ABI details, low-level assembly

## `bsp/` (Board Support Package)
Where specific targets are described and initialized.
- memory mapped peripheral addresses (UART, timers, GPIO)
- clock setup / PLL config
- basic device drivers used for bringup (UART logging, timers)
- pin mux / board init
- interrupt mapping and peripheral configuration

## `include/`
Folder contains headers used across the project
- RTOS public API headers
- internal kernel interfaces
- arch-facing interfaces
- BSP-facing interfaces

## `linker/`
Contains the `.ld` scripts that define:
- FLASH/RAM regions and sizes for each target
- sections placement (`.text`, `.data`, `.bss`, stack, heap)
- symbols used by startup code (`_estack`, `_sidata`, etc)

## How it fits together

- `kernel/` implements OS features without assuming a specific CPU or board.
- `arch/` provides the Cortex-M “port” (context switching, interrupts, startup).
- `bsp/` provides the board/chip layer (UART, timers, clocks, memory map).
- `linker/` defines where the firmware lives in memory for a given target.
- `include/` provides the common interfaces tying these layers together. 
 
