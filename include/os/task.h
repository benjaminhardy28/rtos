#pragma once

#include <stdint.h>

typedef enum {
  OS_TASK_HIGH = 0,
  OS_TASK_MEDIUM = 1,
  OS_TASK_LOW = 2,
  OS_TASK_IDLE = 3,
} os_task_priority_t;

void os_yield(void);
