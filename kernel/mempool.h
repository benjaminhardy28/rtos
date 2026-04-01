#pragma once

#include <stddef.h>
#include <stdint.h>
#include "../include/os/section.h"

typedef struct os_mempool_t {
  void *free_list;
  uint8_t *pool_start;
  uint8_t *pool_end;
  uint32_t block_size;
  uint32_t block_count;
  uint32_t free_count;
} os_mempool_t;

OS_KERNEL_TEXT uint32_t os_mempool_align_up(uint32_t value, uint32_t align);

OS_KERNEL_TEXT void os_mempool_init(os_mempool_t *pool,  void *buffer, uint32_t block_size, uint32_t block_count);

OS_KERNEL_TEXT void *os_mempool_alloc(os_mempool_t *pool);

OS_KERNEL_TEXT int os_mempool_free(os_mempool_t *pool, void *ptr);
