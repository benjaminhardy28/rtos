#pragma once

#include "task.h"
#include "scheduler.h"
#include "../arch/arm_cm/port.h"
#include "../include/os/section.h"
#include "../include/os/mutex.h"

int os_mutex_init(os_mutex_t *mutex);
int os_kernel_mutex_lock(os_mutex_t *mutex);
int os_kernel_mutex_unlock(os_mutex_t *mutex);
