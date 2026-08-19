#include "scheduler.h"
#include "tick.h"

#define NUM_PRIORITIES 4

OS_KERNEL_BSS static os_task_queue_t g_priorities[NUM_PRIORITIES]; // array of ready queue heads for each priority level
OS_KERNEL_BSS static os_task_queue_t g_timeout_queue; // blocked tasks ordered by soonest timeout

OS_KERNEL_TEXT static int os_tick_reached(uint32_t now, uint32_t wake_tick) {
    return (int32_t)(now - wake_tick) >= 0;
}

OS_KERNEL_TEXT static void os_timeout_queue_insert_sorted(os_tcb_t *tcb) {
    os_tcb_t *iter;

    if (!tcb) {
        return;
    }

    tcb->timeout_next = NULL;
    tcb->timeout_prev = NULL;

    if (g_timeout_queue.head == NULL) {
        g_timeout_queue.head = tcb;
        g_timeout_queue.tail = tcb;
        return;
    }

    iter = g_timeout_queue.head;
    // after the loop, iter contains the first wake_tick greater than tcb's wake_tick, or NULL if tcb has the latest wake_tick and should be at the end of the queue
    while (iter != NULL && !os_tick_reached(iter->wake_tick, tcb->wake_tick)) {
        iter = iter->timeout_next;
    }

    if (iter == NULL) {
        tcb->timeout_prev = g_timeout_queue.tail;
        g_timeout_queue.tail->timeout_next = tcb;
        g_timeout_queue.tail = tcb;
        return;
    }

    // insert tcb before iter, since iter is the first task in the queue with a wake_tick greater than tcb's wake_tick
    tcb->timeout_next = iter;
    tcb->timeout_prev = iter->timeout_prev; 

    if (iter->timeout_prev != NULL) {
        iter->timeout_prev->timeout_next = tcb;
    } else {
        g_timeout_queue.head = tcb;
    }

    iter->timeout_prev = tcb;
}

OS_KERNEL_TEXT static void os_timeout_queue_remove(os_tcb_t *tcb) {
    if (!tcb) {
        return;
    }

    if (tcb->timeout_prev != NULL) {
        tcb->timeout_prev->timeout_next = tcb->timeout_next;
    } else if (g_timeout_queue.head == tcb) {
        g_timeout_queue.head = tcb->timeout_next;
    }

    if (tcb->timeout_next != NULL) {
        tcb->timeout_next->timeout_prev = tcb->timeout_prev;
    } else if (g_timeout_queue.tail == tcb) {
        g_timeout_queue.tail = tcb->timeout_prev;
    }

    tcb->timeout_next = NULL;
    tcb->timeout_prev = NULL;
}

OS_KERNEL_TEXT void os_task_queue_init(os_task_queue_t *queue) {
    if (!queue) return;
    queue->head = NULL;
    queue->tail = NULL;
}

OS_KERNEL_TEXT os_tcb_t *os_task_queue_pop_head(os_task_queue_t *queue) {
    // Pops the head of the queue and returns it, or returns NULL if the queue is empty
    if (queue->head) {
        os_tcb_t *prev_head = queue->head;
        os_task_queue_remove(queue, queue->head); // remove from ready queue
        return prev_head;
    }
    return NULL; // no ready tasks found
}

OS_KERNEL_TEXT os_tcb_t *os_task_queue_peek_head(os_task_queue_t *queue){
    if (queue->head) {
        return queue->head;
    }
    return NULL; // no ready tasks found
}

OS_KERNEL_TEXT void os_task_queue_add(os_task_queue_t *queue, os_tcb_t *tcb){
    if (!queue || !tcb) return;
    tcb->next = NULL;
    tcb->prev = queue->tail;
    if(queue->tail) {
        queue->tail->next = tcb;
    } else {
        queue->head = tcb; // first element in the queue
    }
    queue->tail = tcb;
}

OS_KERNEL_TEXT void os_task_queue_remove(os_task_queue_t *queue, os_tcb_t *tcb){
    if (!queue || !tcb) return;
    if(tcb->prev) {
        tcb->prev->next = tcb->next;
    } else {
        queue->head = tcb->next; // removing head of the queue
    }
    if(tcb->next) {
        tcb->next->prev = tcb->prev;
    } else {
        queue->tail = tcb->prev; // removing tail of the queue
    }
    tcb->next = NULL;
    tcb->prev = NULL;
}

OS_KERNEL_TEXT void os_task_queue_insert_by_priority(os_task_queue_t *queue, os_tcb_t *tcb) {
    if (!queue || !tcb) return;

    tcb->next = NULL;
    tcb->prev = NULL;

    if (queue->head == NULL) {
        queue->head = tcb;
        queue->tail = tcb;
        return;
    }

    os_tcb_t *iter = queue->head;
    while (iter != NULL && iter->priority <= tcb->priority) {
        iter = iter->next;
    }

    if (iter == NULL) {
        // Insert at the end
        tcb->prev = queue->tail;
        queue->tail->next = tcb;
        queue->tail = tcb;
    } else {
        // Insert before iter
        tcb->next = iter;
        tcb->prev = iter->prev;

        if (iter->prev != NULL) {
            iter->prev->next = tcb;
        } else {
            queue->head = tcb; // inserting at the head
        }
        iter->prev = tcb;
    }
}

// Ready queue functions just call the generic queue functions with the appropriate priority queue

OS_KERNEL_TEXT void os_ready_queue_rotate(os_tcb_t *tcb){
    if (!tcb) return;
    os_task_queue_t *queue = &g_priorities[tcb->priority];
    if (queue->head != tcb || tcb->next == NULL) {
        // not at head or only one task -> no rotation needed
        return;
    }

    // remove head
    queue->head = tcb->next;
    queue->head->prev = NULL;

    // append to tail
    tcb->next = NULL;
    tcb->prev = queue->tail;
    queue->tail->next = tcb;
    queue->tail = tcb;
}

OS_KERNEL_TEXT void os_ready_queue_add(os_tcb_t *tcb){
    os_task_queue_add(&g_priorities[tcb->priority], tcb);
}

OS_KERNEL_TEXT void os_ready_queue_remove(os_tcb_t *tcb){
    os_task_queue_remove(&g_priorities[tcb->priority], tcb);
}

OS_KERNEL_TEXT void os_schedule(void){
    for (uint8_t p = 0; p < NUM_PRIORITIES; p++) {
        g_next = os_task_queue_peek_head(&g_priorities[p]);
        if (g_next != NULL) {
            return;
        }
    }
    g_next = NULL; // no ready tasks found
}

OS_KERNEL_TEXT void os_task_set_priority(os_tcb_t *tcb, os_task_priority_t new_priority) {
    if (!tcb || new_priority > OS_TASK_IDLE) return;

    // Update the tasks prority 
    if (tcb->state == OS_TASK_READY || tcb->state == OS_TASK_RUNNING) {
        os_ready_queue_remove(tcb);
        tcb->priority = new_priority;
        os_ready_queue_add(tcb);
    } else {
        tcb->priority = new_priority;
    }

    // Re-insert the task into any priority based wait queues it may be in, this will only ever be called for a mutex 
    if(tcb->state == OS_TASK_BLOCKED && tcb->wait_queue != NULL) {
        os_task_queue_remove(tcb->wait_queue, tcb);
        os_task_queue_insert_by_priority(tcb->wait_queue, tcb);
    }
}

// wait queue functions

OS_KERNEL_TEXT void os_task_block_locked(os_task_queue_t *wait_queue, uint32_t timeout_ticks, bool priority_ordered)
{
    os_tcb_t *current;

    if (!wait_queue) return;

    current = g_current;
    if (!current) {
        return;
    }

    os_ready_queue_remove(current); // remove current task from ready queue
    current->state = OS_TASK_BLOCKED; // set task state to blocked
    current->wait_queue = wait_queue; // record which wait queue owns the blocked task
    current->wake_tick = OS_WAIT_FOREVER;
    current->wait_result = 0;

    // Add task to the specified wait queue, either in priority order or FIFO order based on the priority_ordered flag
    if(priority_ordered) {
        os_task_queue_insert_by_priority(wait_queue, current); // insert into wait queue sorted by priority
    } else {
        os_task_queue_add(wait_queue, current); // add current task to the specified wait queue
    }

    // If a timeout is specified, set the wake_tick and insert into the timeout queue
    if (timeout_ticks != OS_WAIT_FOREVER) {
        current->wake_tick = os_tick_get() + timeout_ticks;
        os_timeout_queue_insert_sorted(current);
    }

    os_schedule(); // pick next task to run
    if (g_next != g_current) {
        os_port_pendsv_trigger(); // trigger PendSV to perform context switch to the newly scheduled task if it's different from current
    }
}


OS_KERNEL_TEXT os_tcb_t *os_task_wake_one_locked(os_task_queue_t *wait_queue) {
    os_tcb_t *tcb;

    if (!wait_queue) {
        return NULL;
    }

    tcb = os_task_queue_pop_head(wait_queue); // wake up the next waiting task, if any
    if (tcb != NULL) {
        if (tcb->wake_tick != OS_WAIT_FOREVER) {
            os_timeout_queue_remove(tcb);
        }
        tcb->wait_queue = NULL;
        tcb->wake_tick = OS_WAIT_FOREVER;
        tcb->wait_result = 0;
        tcb->state = OS_TASK_READY; // set task state to ready
        os_ready_queue_add(tcb); // add the task to the ready queue so it can be scheduled
    }

    return tcb;
}

OS_KERNEL_TEXT void os_process_timeouts(void) {
    os_tcb_t *tcb;
    uint32_t now;

    now = os_tick_get();
    tcb = g_timeout_queue.head;

    while (tcb != NULL && os_tick_reached(now, tcb->wake_tick)) { // task's wake tick has been reached, unblock it with a timeout result
        os_tcb_t *next = tcb->timeout_next;

        os_timeout_queue_remove(tcb);
        if (tcb->wait_queue != NULL) {
            os_task_queue_remove(tcb->wait_queue, tcb);
            tcb->wait_queue = NULL;
        }
        tcb->wake_tick = OS_WAIT_FOREVER; // reset wake_tick since we're no longer using it for timeout
        tcb->wait_result = -1;
        tcb->state = OS_TASK_READY;
        os_ready_queue_add(tcb);

        tcb = next;
    }

    os_schedule();
    if (g_next != g_current) {
        os_port_pendsv_trigger();
    }
}

OS_KERNEL_TEXT void os_commit_switch(void) {
    os_tcb_t *prev = g_current;

    if (g_next == NULL) {
        return;
    }

    if (prev != NULL && prev != g_next && prev->state == OS_TASK_RUNNING) {
        prev->state = OS_TASK_READY;
    }

    g_current = g_next;
    g_current->state = OS_TASK_RUNNING;
}


OS_KERNEL_TEXT os_tcb_t *os_current_tcb(void){
    return g_current;
}

OS_KERNEL_TEXT os_tcb_t *os_next_tcb(void) {
    return g_next ? g_next : g_current;
}
