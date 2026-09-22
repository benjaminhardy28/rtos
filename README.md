# RTOS

This is a project that I have been working on during my free time to gain a deeper understanding of Real Time Operating Systems (RTOS). The RTOS is designed for an ARM Cortex-M3 processor and I'm targetting a STM32, using renode for emulation and a physical STM32f103 board. A lot of the design decisions and features of this RTOS were built for learning purposes, including the kernel vs user space seperation. I'm also working on benchmarking how design choices can impact latency, like by comparing the blocking vs lockfree queue.

## Getting Started

```sh
make build            # builds build/rtos.elf for stm32f103
make renode            # build + boot under Renode
make renode-debug      # build, boot under Renode, and attach GDB
```

To flash the RTOS onto the real stm32 chip, override `RENODE_BOARD=stm32f103c8t6` to target the Blue Pill variant and use `HW=1` to bring up real HSE/PLL clocks (72MHz) instead of the default 8MHz HSI reset clock

# Kernel 

## Scheduler

The kernel uses fixed priority preemptive scheduling. For same priority tasks, it implements time slicing with each systick. The kernel allows for 4 different priority levels, each with their own linked list for storing ready tasks.

## Timeout Queue

A global timeout queue is used to store tasks that are timed out, currently only used for storing tasks that voluntarily sleep or block with a timout. These tasks are sorted by their wake tick, and the head of the timeout queue is checked on each systick. 

## Memory Pool

This RTOS uses a static memory pool for deterministic runtime memory allocation. The memory pool is currently used for 2 implementations, one in kernel space to store kernel data structures (ex: Task TCBs which can be initalized at run time) and one for storing task stacks in user space.

## Primitives

Semaphores are implemented by tracking an arbitrary integer count, along with a task queue of waiters. Mutex's are implemented using an owner, a task queue of waiters, as well as a mutex pointer for the case of tracking multiple mutex's owned by a TCB.  

## MPMC Queue (Blocking)

A standard MPMC ring buffer queue was implemented. Internally, it makes use of a single mutex for ownership of the queue, a semaphore to represent the quantity of items to be consumed, and a semaphore for the quantity of free spaces that can be written to.

## SPSC Queue (lockfree)

A lockfree SPSC queue was implemented to learn about low latency lockfree datastructures. The queue is implemented using a head and tail pointer which atomically increment only after corresponding data has been written to or read from. In a single producer/consumer scenario, this ensures expected concurrent behavoir. The write function calls make use of release memory ordering while the read function calls use acquire memory ordering to ensure the CPU does not re-order instructions that could alter the expected queue values.

## Memory Layout

FLASH and RAM are each split in half between user and kernel regions, enforced by the MPU:

```
FLASH (512K, high-density STM32F103)      RAM (64K)
0x00000000 ┌─────────────────┐            0x20000000 ┌─────────────────┐
           │ FLASH_USER 256K │                        │ RAM_USER   32K  │
           │ vectors, user   │                        │ .data/.bss,     │
           │ text/rodata     │                        │ task stacks     │
0x00040000 ├─────────────────┤            0x20008000  ├─────────────────┤
           │ FLASH_KERNEL    │                        │ RAM_KERNEL 32K  │
           │ 256K            │                        │ .data/.bss,     │
           │ kernel text/    │                        │ kernel objects  │
           │ rodata          │                        │                 │
0x00080000 └─────────────────┘            0x20010000  └─────────────────┘
```

## Startup Script

On reset, the initial data values are copied into RAM and the zero-initialized memory is cleared, for both the user and kernel regions. The kernel then sets up its memory pools and creates the idle task, before handing control to the application so it can register its own tasks. Once the application is done registering tasks, the kernel configures the MPU regions, brings up the system clock, and starts the periodic tick timer. It then switches into unprivileged thread mode and triggers the first context switch into a task, handing off control to the scheduler.

## Benchmarking

I've currently implemented a framework for benchmarking cycle counts using the DWT (Data Watch Trace) register, which is also supported with renode. I am working on implementing an interface for using RTT (Real Time Transfer) to test with the physical STM32 board.