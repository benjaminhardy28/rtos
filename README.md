# RTOS

This is a project that I have been working on during my free time to gain a deeper understanding of Real Time Operating Systems (RTOS). The RTOS is designed for an ARM Cortex-M3 processor and I'm targetting a STM32, using renode for emulation and a physical STM32f103 board. A lot of the design decisions and features of this RTOS were built for learning purposes, including the kernel vs user space seperation. I'm also working on benchmarking how design choices can impact latency, like by comparing the blocking vs lockfree queue.

## Getting Started

```sh
make build            # builds build/rtos.elf for the real Blue Pill (stm32f103c8t6)
make renode            # build + boot under Renode
make renode-debug      # build, boot under Renode, and attach GDB
```

Use `HW=1` to bring up real HSE/PLL clocks (72MHz) instead of the default 8MHz HSI reset clock when flashing real hardware. To build/debug the high-density variant under Renode instead, override `BOARD=stm32f103`.

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
FLASH + RAM ADDRESS MAP (STM32F103 "high-density", flash aliased to 0x0)
════════════════════════════════════════════════════════════════════════

0x00000000 ┌──────────────────────────────┐
           │  FLASH_USER        256 KB    │
           │  ┌────────────────────────┐  │
           │  │ .isr_vector  (256B align)│ │  vector table (link-time, read by core on exception)
           │  ├────────────────────────┤  │
           │  │ .text.user              │  │  unprivileged code: syscall wrappers,
           │  │ .rodata.user             │  │  task trampoline, os_task_exit_trap
           │  └────────────────────────┘  │
0x00040000 ├──────────────────────────────┤
           │  FLASH_KERNEL       256 KB    │
           │  ┌────────────────────────┐  │
           │  │ .text.kernel             │  │  privileged-only kernel code
           │  │ .rodata.kernel           │  │  (+ catch-all .text*/.rodata* —
           │  │ (+ generic .text*/.rodata*)│ │   anything not tagged .user lands
           │  └────────────────────────┘  │   here by default)
0x00080000 └──────────────────────────────┘

0x20000000 ┌──────────────────────────────┐   ◄── RAM_USER starts
           │  RAM_USER            32 KB    │       (MPU: unprivileged full access)
           │  ┌────────────────────────┐  │
           │  │ .data.user               │  │  unprivileged initialized globals
           │  │ .bss.user                │  │  unprivileged zero-init globals:
           │  │                          │  │  g_tick (tagged user-space on
           │  │                          │  │  purpose, so os_tick_get() can
           │  │                          │  │  read it with no SVC trap)
           │  ├────────────────────────┤  │
           │  │ .stack.user  = 8 KB      │  │  g_user_stack_pool_storage[8]
           │  │  ┌──────────────────┐   │  │
           │  │  │ block 0  (1 KB)  │ ▼ │  │  ◄ each block = OS_TASK_STACK_WORDS(256)
           │  │  │ block 1  (1 KB)  │ ▼ │  │     × 4 bytes = 1024 B
           │  │  │ block 2  (1 KB)  │ ▼ │  │
           │  │  │   ...            │   │  │  PSP for whichever task owns a block
           │  │  │ block 7  (1 KB)  │ ▼ │  │  grows DOWN within that 1KB slot
           │  │  └──────────────────┘   │  │  (OS_TASK_STACK_COUNT = 8 tasks max)
           │  └────────────────────────┘  │
           │        (unused padding)       │
0x20008000 ├──────────────────────────────┤   ◄── RAM_USER ends / RAM_KERNEL starts
           │  RAM_KERNEL           32 KB   │       (MPU: privileged read/write only)
           │  ┌────────────────────────┐  │
           │  │ .data.kernel             │  │  privileged initialized globals
           │  │ .bss.kernel              │  │  g_current, g_next, g_first_switch,
           │  │                          │  │  g_kernel_object_pool_storage:
           │  │                          │  │   32 blocks × 64B = 2 KB
           │  │                          │  │   (currently used for TCBs only;
           │  │                          │  │    mutexes/semaphores/queues are
           │  │                          │  │    caller-supplied structs, not
           │  │                          │  │    pool-allocated)
           │  ├────────────────────────┤  │
           │  │  MSP STACK (grows DOWN)  │  │  ◄ single shared stack, used by
           │  │         ▼                │  │    SysTick/PendSV/SVC/faults/Reset
           │  │         ▼                │  │    (Handler mode ALWAYS uses MSP,
           │  │         ▼                │  │    regardless of which task was
           │  │         ▼                │  │    interrupted — no per-task MSP)
           │  │      free space          │  │
           │  └────────────────────────┘  │
0x20010000 └──────────────────────────────┘   ◄── _estack (top of RAM_KERNEL)
                                                   loaded into MSP by hardware at reset
                                                   (isr_vector[0]); starts here and
                                                   descends into the free space above;
                                                   NO reserved/bounded region/guard
```

**Key numbers**

| Pool | Block size | Count | Total | Lives in |
|---|---|---|---|---|
| Task stack pool | 1 KB (`OS_TASK_STACK_WORDS=256` words) | 8 (`OS_TASK_STACK_COUNT`) | 8 KB | `RAM_USER` / `.stack.user` |
| Kernel object pool | 64 B (`OS_KERNEL_OBJECT_BLOCK_SIZE`) | 32 (`OS_KERNEL_OBJECT_COUNT`) | 2 KB | `RAM_KERNEL` / `.bss.kernel` |
| MSP stack | — | 1 (shared) | unbounded, no guard | `RAM_KERNEL`, below `_estack` |

**Growth directions**: MSP grows downward from `_estack = 0x20010000` into `RAM_KERNEL`'s free space — one stack, shared by every exception handler, never task-specific. PSP also grows downward, but *within* whichever 1KB block in the `.stack.user` pool belongs to the currently running task — each task's stack is independent and bounded to its own block.

The privilege boundary sits exactly at `0x20008000` — the MPU makes `RAM_USER` fully accessible to unprivileged code and `RAM_KERNEL` privileged-RW-only, so no task can ever directly touch a TCB, the kernel object pool, or the MSP stack — only kernel code running in Handler/privileged mode can.

## Startup Script

On reset, the initial data values are copied into RAM and the zero-initialized memory is cleared, for both the user and kernel regions. The kernel then sets up its memory pools and creates the idle task, before handing control to the application so it can register its own tasks. Once the application is done registering tasks, the kernel configures the MPU regions, brings up the system clock, and starts the periodic tick timer. It then switches into unprivileged thread mode and triggers the first context switch into a task, handing off control to the scheduler.

## Benchmarking

I've currently implemented a framework for benchmarking cycle counts using the DWT (Data Watch Trace) register, which is also supported with renode. I am working on implementing an interface for using RTT (Real Time Transfer) to test with the physical STM32 board.