#pragma once

#include "task.h"
#include <stddef.h>

typedef struct {
    os_tcb_t *head;
    os_tcb_t *tail;
} os_task_queue;

// Task selection
void os_schedule(void); // picks the next task to run based on priority

// Accessors
os_tcb_t *os_current_tcb(void); // get current running task's TCB
os_tcb_t *os_next_tcb(void); // get next scheduled task's TCB

// Ready queue
void os_ready_queue_add(os_tcb_t *tcb); // add TCB to ready queue
void os_ready_queue_remove(os_tcb_t *tcb); // remove TCB from ready queue

// Rotate queue
void os_ready_queue_rotate(os_tcb_t *tcb); // move the given TCB to the end of its priority's ready queue (for round-robin scheduling among tasks of same priority)

// Context switch
void os_commit_switch(void);