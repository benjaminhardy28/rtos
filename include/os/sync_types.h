#pragma once

struct os_tcb_t;

typedef struct os_task_queue_t {
    struct os_tcb_t *head;
    struct os_tcb_t *tail;
} os_task_queue_t;
