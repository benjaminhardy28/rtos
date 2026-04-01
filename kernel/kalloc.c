#include "kalloc.h"
#include <stdint.h>

OS_KERNEL_BSS static os_mempool_t g_kernel_object_pool;
OS_KERNEL_BSS static os_mempool_t g_user_stack_pool;

// Generic kernel-object pool in static region of kernel RAM
OS_KERNEL_BSS static uint8_t g_kernel_object_pool_storage[
  OS_KERNEL_OBJECT_BLOCK_SIZE * OS_KERNEL_OBJECT_COUNT
];

// Memory pool for task stacks, stored in user space in OS_USER_STACK which is designated for task stacks (not a stack itself)
OS_USER_STACK static os_task_stack_block_t g_user_stack_pool_storage[OS_TASK_STACK_COUNT];

OS_KERNEL_TEXT void os_kalloc_init(void){
    uint32_t kernel_block_size;
    uint32_t stack_block_size;

    kernel_block_size = os_mempool_align_up(OS_KERNEL_OBJECT_BLOCK_SIZE, (uint32_t)sizeof(void *));
    stack_block_size = os_mempool_align_up((uint32_t)sizeof(os_task_stack_block_t), (uint32_t)sizeof(void *));

    os_mempool_init(&g_kernel_object_pool,
                  g_kernel_object_pool_storage,
                  kernel_block_size,
                  OS_KERNEL_OBJECT_COUNT);

    os_mempool_init(&g_user_stack_pool,
                    g_user_stack_pool_storage,
                    stack_block_size,
                    OS_TASK_STACK_COUNT);
}


OS_KERNEL_TEXT void *os_kobj_alloc(uint32_t size) {
  if (size == 0u || size > OS_KERNEL_OBJECT_BLOCK_SIZE) {
    return NULL;
  }

  return os_mempool_alloc(&g_kernel_object_pool);
}


OS_KERNEL_TEXT int os_kobj_free(void *ptr) {
  return os_mempool_free(&g_kernel_object_pool, ptr);
}


OS_KERNEL_TEXT uint32_t *os_kstack_alloc(uint32_t stack_words) {
  os_task_stack_block_t *block;

  if (stack_words == 0u || stack_words > OS_TASK_STACK_WORDS) {
    return NULL;
  }

  block = (os_task_stack_block_t *)os_mempool_alloc(&g_user_stack_pool);
  if (block == NULL) {
    return NULL;
  }

  return block->words;
}


OS_KERNEL_TEXT int os_kstack_free(uint32_t *stack_mem) {
  return os_mempool_free(&g_user_stack_pool, (void *)stack_mem);
}
