#pragma once

#include "task.h"
#include <stddef.h>

typedef struct {
    os_tcb_t *head;
    os_tcb_t *tail;
} os_task_queue_t;

// Generic queue functions
void os_task_queue_init(os_task_queue_t *queue);
os_tcb_t *os_task_queue_pop_head(os_task_queue_t *queue); // picks the next task to run based on priority
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
void os_task_block(os_task_queue_t *wait_queue);
os_tcb_t *os_task_wake_one(os_task_queue_t *wait_queue);

// Rotate queue
void os_ready_queue_rotate(os_tcb_t *tcb); // move the given TCB to the end of its priority's ready queue (for round-robin scheduling among tasks of same priority)

// Context switch
void os_commit_switch(void);