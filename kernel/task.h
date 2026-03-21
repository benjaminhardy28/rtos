#pragma once

#include <stdint.h>
#include "../arch/arm_cm/port.h"
#include "../bsp/qemu_mps2/systick.h"
#include <stddef.h>

typedef void (*os_task_fn_t)(void *);
struct os_task_queue_t;
typedef struct os_task_queue_t os_task_queue_t;

#define OS_WAIT_FOREVER UINT32_MAX

typedef enum {
  OS_TASK_READY = 0,
  OS_TASK_RUNNING = 1,
  OS_TASK_BLOCKED = 2,
} os_task_state_t;

typedef enum {
  OS_TASK_HIGH = 0,
  OS_TASK_MEDIUM = 1,
  OS_TASK_LOW = 2,
  OS_TASK_IDLE = 3,
} os_task_priority_t;

typedef struct os_tcb_t {
  uint32_t *sp; // stack pointer
  os_task_priority_t priority; // task priority
  os_task_state_t state; // task state (ready, blocked, etc.)
  struct os_tcb_t *next; // intrusive ready queue link
  struct os_tcb_t *prev; // previous link for doubly-linked ready queue
  struct os_tcb_t *timeout_next; // intrusive timeout queue link
  struct os_tcb_t *timeout_prev; // previous link for doubly-linked timeout queue
  os_task_queue_t *wait_queue;
  uint32_t wake_tick;
  int wait_result;
} os_tcb_t;

extern os_tcb_t *g_current;
extern os_tcb_t *g_next;

int os_task_create(os_tcb_t *tcb, os_task_priority_t priority, os_task_fn_t entry, void *arg, uint32_t *stack_mem, uint32_t stack_words);
void os_start(void); // start the scheduler, picks first task to run
void os_yield(void); // asks kernel to switch away from current task. Picks task then triggers PendSV
