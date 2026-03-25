# Custom Cortex-M RTOS

This repository contains a small RTOS for ARM Cortex-M systems. The codebase is split into portable kernel logic, Cortex-M-specific port code, board support code, public API headers, and linker scripts that define the firmware memory layout.

## Directory Structure

### `kernel/`
This directory contains the core RTOS logic. It is the main implementation of scheduling, blocking, wakeups, and synchronization behavior, and it is intended to stay as CPU-agnostic as possible.

Main responsibilities:
- task and TCB management
- ready queues and wait queues
- timeout and tick-based blocking logic
- synchronization primitives such as semaphores and mutexes
- privileged kernel implementations behind the public RTOS APIs

Important idea:
- `kernel/` is where the RTOS decides what should happen
- `arch/` is where the CPU-specific mechanism for making it happen lives

### `arch/arm_cm/`
This directory is the Cortex-M port layer. It contains code that depends on ARM exception behavior, stack layout, special registers, and low-level assembly.

Main responsibilities:
- reset entry and vector table
- `PendSV_Handler` for context switching
- `SVC_Handler` for syscall entry from thread mode into privileged handler mode
- interrupt masking helpers such as `PRIMASK` save/restore
- MPU setup and memory protection policy
- stack pointer and exception frame handling (`MSP`, `PSP`, `EXC_RETURN`)

Important idea:
- `kernel/` asks for a context switch or blocking operation
- `arch/arm_cm/` implements the Cortex-M-specific path that performs it

### `bsp/`
The Board Support Package contains target-specific hardware support. This is where board- or emulator-specific peripherals and timing setup live.

Main responsibilities:
- target peripheral definitions
- timer or SysTick setup helpers
- simple bring-up support for the selected board/platform

In this repository, the BSP is currently lightweight and mainly provides SysTick support for the QEMU MPS2 target.

### `include/`
This directory contains shared headers. It is the beginning of the public interface boundary between application code and internal kernel code.

Current layout:
- `include/os/` contains the public RTOS-facing API surface
- public headers declare what application code is allowed to call
- internal kernel details remain in `kernel/*.h`

Important idea:
- `include/os/` is the API contract
- `kernel/*.h` contains internal implementation details the kernel needs for itself

This split matters for the privilege model:
- public API stubs can be placed in user-accessible flash
- privileged kernel implementations can remain in protected kernel regions

### `linker/`
This directory contains linker scripts that define how the firmware image is placed into memory.

Main responsibilities:
- define FLASH and RAM regions
- place code and data sections at concrete addresses
- provide symbols used by startup code and MPU setup
- separate user-accessible memory from privileged kernel memory

With the current layout, the linker script is also part of the privilege architecture because it separates:
- user flash
- kernel flash
- user RAM
- kernel RAM

### `scripts/`
Helper scripts for running or debugging the firmware in QEMU.

These are development conveniences rather than part of the RTOS itself.

## How The Pieces Fit Together

The system is organized as a layered RTOS:

1. Application code calls the public RTOS API declared in `include/os/`.
2. User-facing API stubs can issue `SVC` to cross from thread mode into privileged handler mode.
3. `SVC_Handler` dispatches to privileged kernel implementations in `kernel/`.
4. The scheduler and synchronization code decide which task should run next.
5. `PendSV_Handler` performs the actual context switch by saving and restoring thread context.
6. `SysTick_Handler` drives timekeeping and timeout processing.
7. The MPU and linker layout work together to keep kernel code/data privileged-only while leaving user code/data accessible to unprivileged threads.

## Memory And Privilege Model

The current design is moving toward a split between:
- user-facing code that runs in thread mode on PSP
- privileged kernel work that runs in handler mode on MSP

The intended model is:
- application threads call small public API stubs
- those stubs trap into `SVC`
- privileged kernel code performs scheduling and synchronization work
- `PendSV` applies deferred context switches
- the MPU prevents unprivileged threads from directly touching kernel-only code and data

That means this repository is not just split by source directory. It is also split by role:
- public interface
- privileged kernel implementation
- architecture-specific trap/switch machinery
- board-specific hardware support
- linker-defined memory protection boundaries

## Quick File Guide

Some useful anchor points in the repo:
- `kernel/task.c`: task creation, scheduler start, yield path
- `kernel/scheduler.c`: ready queues, blocking, wakeups, timeout handling
- `kernel/semaphore.c`: semaphore implementation
- `kernel/mutex.c`: mutex implementation
- `kernel/tick.c`: tick count, delays, timeout processing
- `arch/arm_cm/port.c`: PendSV, SVC, IRQ helpers, MPU setup
- `arch/arm_cm/startup.c`: reset handler and vector table
- `linker/mps2-an385.ld`: memory layout and section placement

## Build Notes

The project is currently set up for:
- ARM Cortex-M3
- QEMU `mps2-an385`

Build with:

```sh
make build
```

Run under QEMU with:

```sh
make qemu
```
