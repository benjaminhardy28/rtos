#pragma once

#include <stdint.h>

typedef void (*os_task_fn_t)(void *);

typedef enum {
  OS_TASK_HIGH = 0,
  OS_TASK_MEDIUM = 1,
  OS_TASK_LOW = 2,
  OS_TASK_IDLE = 3,
} os_task_priority_t;

typedef struct {
  os_task_priority_t priority;
  os_task_fn_t entry;
  void *arg;
  uint32_t stack_words;
} os_task_create_args_t;

void os_yield(void);
int os_task_create(os_task_priority_t priority, os_task_fn_t entry, void *arg, uint32_t stack_words);
