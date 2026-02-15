#pragma once
#include <stdint.h>
#include "../arch/arm_cm/port.h"
#include "../bsp/qemu_mps2/systick.h"

typedef void (*os_task_fn_t)(void *);

typedef struct { // inital task context structure
  uint32_t *sp; // stack pointer
} os_tcb_t; // task control block

int os_task_create(os_tcb_t *tcb, os_task_fn_t entry, void *arg, uint32_t *stack_mem, uint32_t stack_words);
void os_start(void); // start the scheduler, picks first task to run
void os_yield(void); // asks kernel to switch away from current task. Picks task then triggers PendSV
void os_schedule_round_robin(void); // simple round-robin between two tasks
void os_set_two_tasks(os_tcb_t *t0, os_tcb_t *t1) ; // for simple round-robin scheduling between two tasks. Hardcoded for now to 2 tasks.
os_tcb_t *os_current_tcb(void); // get current running task's TCB
os_tcb_t *os_next_tcb(void); // get next scheduled task's TCB

void os_commit_switch(void);