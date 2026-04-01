#include "mempool.h"
#include "../arch/arm_cm/port.h"
#include <stddef.h>
#include <stdint.h>

typedef struct os_mempool_node_t {
  struct os_mempool_node_t *next;
} os_mempool_node_t;

OS_KERNEL_TEXT uint32_t os_mempool_align_up(uint32_t value, uint32_t align) {
  return (value + (align - 1u)) & ~(align - 1u);
}

OS_KERNEL_TEXT void os_mempool_init(os_mempool_t *pool,  void *buffer, uint32_t block_size, uint32_t block_count){
    uint32_t i;
    uint8_t *base;
    os_mempool_node_t *node;

    if(pool == NULL || buffer == NULL || block_size == 0u || block_count == 0u){
        return;
    }

    base = (uint8_t *)buffer;

    pool->free_list = NULL;
    pool->pool_start = base;
    pool->pool_end = base+(block_size * block_count);
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->free_count = block_count;

    for(i = 0; i < block_count; i++){
        node = (os_mempool_node_t *)(base + (i * block_size));
        node->next = (os_mempool_node_t *)pool->free_list;
        pool->free_list = node;
    }
}

OS_KERNEL_TEXT void *os_mempool_alloc(os_mempool_t *pool){
    uint32_t primask;
    os_mempool_node_t *node;

    if(pool == NULL){
        return NULL;
    }

    primask = os_port_irq_save();

    node = (os_mempool_node_t*)pool->free_list;
    if(node != NULL){
        pool->free_list = node->next;
        pool->free_count--;
    }

    os_port_irq_restore(primask);
    return (void*)node;
}

OS_KERNEL_TEXT int os_mempool_free(os_mempool_t *pool, void *ptr){
    uint32_t primask;
    uintptr_t offset;
    os_mempool_node_t *node;

    if(pool == NULL || ptr == NULL){
        return -1;
    }

    // check pointer is within pool memory range
    if((uint8_t *)ptr < pool->pool_start || (uint8_t *)ptr >= pool->pool_end){
        return -1;
    }

    // check that the pointer is looking at a valid block within the pool
    offset = (uintptr_t)((uint8_t *)ptr - pool->pool_start); 
    if(offset % pool->block_size != 0u){
        return -1;
    }

    primask = os_port_irq_save();

    node = (os_mempool_node_t *)ptr;
    node->next = (os_mempool_node_t *)pool->free_list;
    pool->free_list = node;
    pool->free_count++;

    os_port_irq_restore(primask);
    return 0;
}
