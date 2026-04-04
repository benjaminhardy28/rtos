#include "lfqueue.h"


OS_USER_TEXT static uint32_t lfqueue_next_index(const os_spsc_queue_t *queue, uint32_t index) {
    index++;
    if (index == queue->capacity) {
        index = 0u;
    }
    return index;
}

OS_USER_TEXT int lfqueue_init(os_spsc_queue_t *queue, void *storage, uint32_t msg_size, uint32_t capacity){
    if(queue == NULL || storage == NULL || msg_size == 0u || capacity < 2u){
        return -1;
    }

    queue->storage = (uint8_t *)storage;
    queue->msg_size = msg_size;
    queue->capacity = capacity;
    atomic_store(&queue->read_index, 0u);
    atomic_store(&queue->write_index, 0u);

    return 0;
}

OS_USER_TEXT void* get_write_location(os_spsc_queue_t *queue){
    uint32_t next_write;
    uint32_t write_index;
    uint32_t read_index;

    if(queue == NULL) return NULL;

    write_index =  atomic_load_explicit(&queue->write_index, memory_order_relaxed);
    next_write = lfqueue_next_index(queue, write_index);
    read_index = atomic_load_explicit(&queue->read_index, memory_order_acquire);

    if(next_write == read_index) return NULL; // buffer is full when the next write index would catch the reader

    return (void*)(queue->storage + (write_index * queue->msg_size));
}

OS_USER_TEXT int update_write_index(os_spsc_queue_t *queue){
    uint32_t write_index;

    if (queue == NULL) {
        return -1;
    }

    write_index = atomic_load_explicit(&queue->write_index, memory_order_relaxed);
    write_index = lfqueue_next_index(queue, write_index);
    atomic_store_explicit(&queue->write_index, write_index, memory_order_release);

    return 0;
}

OS_USER_TEXT void* get_read_location(os_spsc_queue_t *queue){
    uint32_t read_index;
    uint32_t write_index;

    if (queue == NULL) {
        return NULL;
    }

    read_index = atomic_load_explicit(&queue->read_index, memory_order_relaxed);
    write_index = atomic_load_explicit(&queue->write_index, memory_order_acquire);

    if (read_index == write_index) return NULL; // queue is empty, the current read index is where the write index is

    return (void *)(queue->storage + (read_index * queue->msg_size));
}

OS_USER_TEXT int update_read_index(os_spsc_queue_t *queue){
    uint32_t read_index;

    if (queue == NULL) {
        return -1;
    }

    read_index = atomic_load_explicit(&queue->read_index, memory_order_relaxed);
    read_index = lfqueue_next_index(queue, read_index);
    atomic_store_explicit(&queue->read_index, read_index, memory_order_release);

    return 0;
}
