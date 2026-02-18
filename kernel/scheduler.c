#include "scheduler.h"

#define NUM_PRIORITIES 3

static os_task_queue g_priorities[NUM_PRIORITIES]; // array of ready queue heads for each priority level

void os_schedule(void) {
    // picks the next task to run based on priority
    for (uint8_t p = 0; p < NUM_PRIORITIES; p++) {
        os_task_queue *queue = &g_priorities[p];
        if (queue->head) {
            g_next = queue->head; // pick the first task in the ready queue of this priority
            return;
        }
    }
    g_next = NULL; // no ready tasks found
}

void os_ready_queue_add(os_tcb_t *tcb){
    os_task_queue *queue = &g_priorities[tcb->priority];
    tcb->next = NULL;
    tcb->prev = queue->tail;
    if(queue->tail) {
        queue->tail->next = tcb;
    } else {
        queue->head = tcb; // first element in the queue
    }
    queue->tail = tcb;
}

void os_ready_queue_remove(os_tcb_t *tcb){
    os_task_queue *queue = &g_priorities[tcb->priority];
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
}

void os_ready_queue_rotate(os_tcb_t *tcb){
    if (!tcb) return;
    os_task_queue *queue = &g_priorities[tcb->priority];
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