#include "../include/os/queue.h"
#include "memory.h"
#include <stddef.h>

OS_USER_TEXT int os_queue_init(os_queue_t *queue, void *storage, uint32_t msg_size, uint32_t capacity){
    if(queue == NULL || storage == NULL || msg_size == 0u || capacity == 0u){
        return -1;
    }

    queue->storage = (uint8_t *)storage;
    queue->msg_size = msg_size;
    queue->capacity = capacity;
    queue->head = 0u;
    queue->tail = 0u;
    if (os_mutex_init(&queue->lock) != 0) {
        return -1;
    }
    if (os_sem_init(&queue->spaces, (int32_t)capacity) != 0) {
        return -1;
    }
    if (os_sem_init(&queue->items, 0) != 0) {
        return -1;
    }

    return 0;
}

OS_USER_TEXT int os_queue_send(os_queue_t *queue, const void *msg){
    uint8_t *slot;
    int ret;

    if(queue == NULL || msg == NULL){
        return -1;
    }

    ret = os_sem_take(&queue->spaces);
    if (ret != 0) {
        return ret;
    }

    ret = os_mutex_lock(&queue->lock);
    if (ret != 0) {
        (void)os_sem_give(&queue->spaces);
        return ret;
    }

    slot = queue->storage + (queue->tail * queue->msg_size);
    os_memcpy(slot, msg, queue->msg_size);

    queue->tail++;
    if (queue->tail == queue->capacity) {
        queue->tail = 0u;
    }

    ret = os_mutex_unlock(&queue->lock);
    if (ret != 0) {
        return ret;
    }

    return os_sem_give(&queue->items);
}

OS_USER_TEXT int os_queue_recv(os_queue_t *queue, void *msg){
    uint8_t *slot;
    int ret;

    if(queue == NULL || msg == NULL){
        return -1;
    }

    ret = os_sem_take(&queue->items);
    if (ret != 0) {
        return ret;
    }

    ret = os_mutex_lock(&queue->lock);
    if (ret != 0) {
        (void)os_sem_give(&queue->items);
        return ret;
    }

    slot = queue->storage + (queue->head * queue->msg_size);
    os_memcpy(msg, slot, queue->msg_size);

    queue->head++;
    if (queue->head == queue->capacity) {
        queue->head = 0u;
    }

    ret = os_mutex_unlock(&queue->lock);
    if (ret != 0) {
        return ret;
    }

    return os_sem_give(&queue->spaces);
}
