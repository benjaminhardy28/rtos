#pragma once

#include <stdint.h>
#include "../arch/arm_cm/port.h"
#include "../bsp/qemu_mps2/systick.h"
#include "../include/os/section.h"
#include "../include/os/task.h"
#include "../include/os/mutex.h"
#include <stddef.h>

struct os_task_queue_t;
typedef struct os_task_queue_t os_task_queue_t;

#define OS_WAIT_FOREVER UINT32_MAX

typedef enum {
  OS_TASK_READY = 0,
  OS_TASK_RUNNING = 1,
  OS_TASK_BLOCKED = 2,
} os_task_state_t;

typedef struct os_tcb_t {
  uint32_t *sp; // stack pointer
  os_task_priority_t base_priority; // task base priority
  os_task_priority_t priority; // task priority
  os_task_state_t state; // task state (ready, blocked, etc.)
  struct os_tcb_t *next; // intrusive ready queue link
  struct os_tcb_t *prev; // previous link for doubly-linked ready queue
  struct os_tcb_t *timeout_next; // intrusive timeout queue link
  struct os_tcb_t *timeout_prev; // previous link for doubly-linked timeout queue
  os_task_queue_t *wait_queue; // pointer to the wait queue the task is blocked on, if any
  os_mutex_t *owned_mutexes; // pointer to the mutexes that this task owns
  os_mutex_t *blocked_on; // pointer to the mutex the task is waiting on, if any
  uint32_t wake_tick; // tick count at which the task should be woken up (for timeouts)
  int wait_result;
} os_tcb_t;

extern os_tcb_t *g_current;
extern os_tcb_t *g_next;

OS_KERNEL_TEXT int os_task_create_internal(os_tcb_t *tcb, os_task_priority_t priority, os_task_fn_t entry, void *arg, uint32_t *stack_mem, uint32_t stack_words);
OS_KERNEL_TEXT int os_kernel_task_create(const os_task_create_args_t *args);
void os_start(void); // start the scheduler, picks first task to run
void os_kernel_yield(void); // privileged implementation behind os_yield()
