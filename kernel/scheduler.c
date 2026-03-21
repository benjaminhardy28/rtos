#include "scheduler.h"

#define NUM_PRIORITIES 3

static os_task_queue_t g_priorities[NUM_PRIORITIES]; // array of ready queue heads for each priority level

void os_task_queue_init(os_task_queue_t *queue) {
    if (!queue) return;
    queue->head = NULL;
    queue->tail = NULL;
}

os_tcb_t *os_task_queue_pop_head(os_task_queue_t *queue) {
    // picks the next task to run based on priority
    if (queue->head) {
        os_task_queue_remove(queue, queue->head); // remove from ready queue
        return queue->head;
    }
    return NULL; // no ready tasks found
}

void os_task_queue_add(os_task_queue_t *queue, os_tcb_t *tcb){
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

void os_task_queue_remove(os_task_queue_t *queue, os_tcb_t *tcb){
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

// Ready queue functions just call the generic queue functions with the appropriate priority queue

void os_ready_queue_rotate(os_tcb_t *tcb){
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

void os_ready_queue_add(os_tcb_t *tcb){
    os_task_queue_add(&g_priorities[tcb->priority], tcb);
}

void os_ready_queue_remove(os_tcb_t *tcb){
    os_task_queue_remove(&g_priorities[tcb->priority], tcb);
}

void os_schedule(void){
    for (uint8_t p = 0; p < NUM_PRIORITIES; p++) {
        g_next = os_task_queue_pop_head(&g_priorities[p]);
        if (g_next != NULL) {
            return;
        }
    }
    g_next = NULL; // no ready tasks found
}

// wait queue functions

void os_task_block(os_task_queue_t *wait_queue){
    os_tcb_t *current = os_current_tcb();
    if (!current || !wait_queue) return;
    current->state = OS_TASK_BLOCKED;
    os_task_queue_add(wait_queue, current);
}

os_tcb_t *os_task_wake_one(os_task_queue_t *wait_queue){
    if (!wait_queue) return NULL;
    os_tcb_t *tcb = os_task_queue_pop_head(wait_queue);
    if (tcb) {
        tcb->state = OS_TASK_READY;
        os_ready_queue_add(tcb);
    }
    return tcb;
}

void os_commit_switch(void){
    if (g_next != NULL) {
        g_current = g_next;
    }
}

os_tcb_t *os_current_tcb(void){
    return g_current;
}

os_tcb_t *os_next_tcb(void) {
    return g_next ? g_next : g_current;
}