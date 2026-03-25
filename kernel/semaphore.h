#pragma once

#include "task.h"
#include "scheduler.h"

typedef struct os_sem_t {
    int32_t count;
    os_task_queue_t waiters;
} os_sem_t;

int os_sem_init(os_sem_t *sem, int32_t initial_count);
int os_kernel_sem_take(os_sem_t *sem);
int os_kernel_sem_give(os_sem_t *sem);
