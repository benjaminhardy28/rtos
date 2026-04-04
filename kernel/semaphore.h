#pragma once

#include "task.h"
#include "scheduler.h"
#include "../arch/arm_cm/port.h"
#include "../include/os/section.h"
#include "../include/os/semaphore.h"

int os_sem_init(os_sem_t *sem, int32_t initial_count);
int os_kernel_sem_take(os_sem_t *sem);
int os_kernel_sem_give(os_sem_t *sem);
