#pragma once

#include "task.h"
#include "scheduler.h"
#include "../arch/arm_cm/port.h"
#include "../include/os/section.h"
#include "../include/os/mutex.h"
#include "stdbool.h"

int os_mutex_init(os_mutex_t *mutex);
int os_kernel_mutex_lock(os_mutex_t *mutex);
int os_kernel_mutex_unlock(os_mutex_t *mutex);

// helpers
static void os_mutex_add_to_owned(os_tcb_t *tcb, os_mutex_t *mutex);
static void os_mutex_remove_from_owned(os_tcb_t *tcb, os_mutex_t *mutex);
static os_task_priority_t os_mutex_recompute_priority(os_tcb_t *tcb);