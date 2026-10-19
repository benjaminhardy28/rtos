#pragma once

#include "kalloc.h"
#include <stdint.h>
#include "mempool.h"
#include "task.h"

#define OS_KERNEL_OBJECT_BLOCK_SIZE 64u
#define OS_KERNEL_OBJECT_COUNT      32u

#define OS_TASK_STACK_WORDS         256u
#define OS_TASK_STACK_COUNT         8u

typedef struct {
  uint32_t words[OS_TASK_STACK_WORDS];
} os_task_stack_block_t;

void os_kalloc_init(void);

void *os_kobj_alloc(uint32_t size);
int os_kobj_free(void *ptr);

// Stack operates off of words rather than direct bytes to match typical type representation of stacks
uint32_t *os_kstack_alloc(uint32_t stack_words);
int os_kstack_free(uint32_t *stack_mem);
