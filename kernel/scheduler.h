#pragma once

#include "task.h"
#include <stddef.h>

typedef struct os_task_queue_t {
    os_tcb_t *head;
    os_tcb_t *tail;
} os_task_queue_t;

// Generic queue functions
void os_task_queue_init(os_task_queue_t *queue);
os_tcb_t *os_task_queue_pop_head(os_task_queue_t *queue); // pop head of queue and return it, or return NULL if queue is empty
os_tcb_t *os_task_queue_peek_head(os_task_queue_t *queue); // peek at head of queue without removing (for scheduler to check if higher priority task is ready)
void os_task_queue_add(os_task_queue_t *queue, os_tcb_t *tcb); // add TCB to queue
void os_task_queue_remove(os_task_queue_t *queue, os_tcb_t *tcb); // remove TCB from ready queue

// Accessors
os_tcb_t *os_current_tcb(void); // get current running task's TCB
os_tcb_t *os_next_tcb(void); // get next scheduled task's TCB

// Ready queue
void os_ready_queue_add(os_tcb_t *tcb); // add TCB to queue
void os_ready_queue_remove(os_tcb_t *tcb); // remove TCB from ready queue
void os_schedule(void); // pick next task to run and set g_next

// wait queue
void os_task_block_locked(os_task_queue_t *wait_queue, uint32_t timeout_ticks); // caller must have interrupts disabled
os_tcb_t *os_task_wake_one_locked(os_task_queue_t *wait_queue); // caller must have interrupts disabled
void os_process_timeouts(void);

// Rotate queue
void os_ready_queue_rotate(os_tcb_t *tcb); // move the given TCB to the end of its priority's ready queue (for round-robin scheduling among tasks of same priority)

// Context switch
void os_commit_switch(void);
