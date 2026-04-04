#pragma once

#include "sync_types.h"

typedef struct os_mutex_t {
    struct os_tcb_t *owner;
    os_task_queue_t waiters;
} os_mutex_t;

int os_mutex_init(os_mutex_t *mutex);
int os_mutex_lock(os_mutex_t *mutex);
int os_mutex_unlock(os_mutex_t *mutex);
