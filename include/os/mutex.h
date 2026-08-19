#pragma once

#include "sync_types.h"

typedef struct os_mutex_t {
    struct os_tcb_t *owner;
    struct os_mutex_t *next_mutex; // this is the link for the tcb owned_mutexes linked list
    os_task_queue_t waiters;
} os_mutex_t;

int os_mutex_init(os_mutex_t *mutex);
int os_mutex_lock(os_mutex_t *mutex);
int os_mutex_unlock(os_mutex_t *mutex);
