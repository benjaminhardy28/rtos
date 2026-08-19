#include "mutex.h"
#include <stdint.h>
#include "../include/os/syscall.h"

OS_USER_TEXT int os_mutex_init(os_mutex_t *mutex) {
    if (!mutex) {
        return -1;
    }

    mutex->owner = NULL;
    mutex->waiters.head = NULL;
    mutex->waiters.tail = NULL;
    return 0;
}

OS_USER_TEXT int os_mutex_lock(os_mutex_t *mutex) {
    return (int)os_port_svc_call1(OS_SYSCALL_MUTEX_LOCK, (uint32_t)(uintptr_t)mutex);
}

OS_KERNEL_TEXT int os_kernel_mutex_lock(os_mutex_t *mutex) {
    uint32_t key;

    if (!mutex) {
        return -1;
    }

    key = os_port_irq_save();

    if (mutex->owner == NULL) { // mutex is available, take ownership and return
        mutex->owner = g_current;
        os_mutex_add_to_owned(g_current, mutex);
        os_port_irq_restore(key);
        return 0;
    }

    if (mutex->owner == g_current) { // already the owner
        os_port_irq_restore(key);
        return -1;
    }
    
    g_current->blocked_on = mutex;
    os_tcb_t *owner = mutex->owner;

    // go through chain of blocked tasks to update each priority  
    while(owner != NULL && g_current->priority < owner->priority){
        os_task_set_priority(owner, g_current->priority);
        // the owner is blocked on another task
        if(owner->state == OS_TASK_BLOCKED && owner->blocked_on != NULL){
            owner = owner->blocked_on->owner; // blocked_on is a mutext, get the owner by dereferecning mutex owner
        } else {
            break; // owner is null
        }
    }

    os_task_block_locked(&mutex->waiters, OS_WAIT_FOREVER, true); // block current task on mutex wait queue until it becomes available. Caller is already inside critical section, so we can call the _locked version of block
    // task is no longer blocked
    g_current->blocked_on = NULL;
    os_mutex_add_to_owned(g_current, mutex);
    // Any tasks that wakeup will automatically be given ownership, so we don't need to do anything else here when blocking
    os_port_irq_restore(key); // restore interrupts after blocking (we don't want to restore before blocking since that would allow an interrupt to occur

    return 0;
}


OS_USER_TEXT int os_mutex_unlock(os_mutex_t *mutex) {
    return (int)os_port_svc_call1(OS_SYSCALL_MUTEX_UNLOCK, (uint32_t)(uintptr_t)mutex);
}

OS_KERNEL_TEXT int os_kernel_mutex_unlock(os_mutex_t *mutex) {
    uint32_t key;
    os_tcb_t *next_owner;

    if (!mutex) {
        return -1;
    }

    key = os_port_irq_save();

    if (mutex->owner != g_current) { // only the owner can unlock
        os_port_irq_restore(key);
        return -1;
    }

    os_mutex_remove_from_owned(g_current, mutex);
    os_task_set_priority(g_current, os_mutex_recompute_priority(g_current));

    next_owner = os_task_wake_one_locked(&mutex->waiters); // wake up the next waiting task, if any, to become the new owner of the mutex. If no waiting tasks, just set owner to NULL
    if (next_owner != NULL) {
        mutex->owner = next_owner; // transfer ownership to the next waiting task
        os_schedule(); // pick next task to run (may be the one we just woke up if it has higher priority than current)
        if (g_next != g_current) {
            os_port_pendsv_trigger(); // trigger PendSV to perform context switch to the newly scheduled task if it's different from current
        }
    } else {
        mutex->owner = NULL;
    }

    os_port_irq_restore(key);
    return 0;
}

// helper functions

static void os_mutex_add_to_owned(os_tcb_t *tcb, os_mutex_t *mutex) {
    mutex->next_mutex = tcb->owned_mutexes;
    tcb->owned_mutexes = mutex;
}

static void os_mutex_remove_from_owned(os_tcb_t *tcb, os_mutex_t *mutex) {
    os_mutex_t *prev = NULL;
    os_mutex_t *curr = tcb->owned_mutexes;

    while (curr != NULL) {
        if (curr == mutex) {
            if (prev == NULL) {
                tcb->owned_mutexes = curr->next_mutex; // removing the head
            } else {
                prev->next_mutex = curr->next_mutex; // removing a middle/tail node
            }
            curr->next_mutex = NULL;
            return;
        }
        prev = curr;
        curr = curr->next_mutex;
    }
}

static os_task_priority_t os_mutex_recompute_priority(os_tcb_t *tcb) {
    os_task_priority_t result = tcb->base_priority;
    for (os_mutex_t *m = tcb->owned_mutexes; m != NULL; m = m->next_mutex) {
        if (m->waiters.head != NULL && m->waiters.head->priority < result) {
            result = m->waiters.head->priority; // numerically smaller = more urgent
        }
    }
    return result;
}