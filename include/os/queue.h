#pragma once

#include <stdint.h>
#include "mutex.h"
#include "semaphore.h"

typedef struct os_queue_t {
    uint8_t *storage;
    uint32_t msg_size;
    uint32_t capacity;
    uint32_t head; // stored as an index value
    uint32_t tail; // stored as an index value
    os_mutex_t lock;
    os_sem_t spaces;
    os_sem_t items;
} os_queue_t;

int os_queue_init(os_queue_t *queue, void *storage, uint32_t msg_size, uint32_t capacity);
int os_queue_send(os_queue_t *queue, const void *msg);
int os_queue_recv(os_queue_t *queue, void *msg);
