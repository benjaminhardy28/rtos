#pragma once

#include <stdint.h>
#include <stdatomic.h>

typedef struct {
    uint8_t *storage;
    uint32_t msg_size;
    uint32_t capacity;
    _Atomic uint32_t read_index;
    _Atomic uint32_t write_index;
} os_spsc_queue_t;


int lfqueue_init(os_spsc_queue_t *queue, void *storage, uint32_t msg_size, uint32_t capacity);

void* get_write_location(os_spsc_queue_t *queue);

int update_write_index(os_spsc_queue_t *queue);

void* get_read_location(os_spsc_queue_t *queue);

int update_read_index(os_spsc_queue_t *queue);
