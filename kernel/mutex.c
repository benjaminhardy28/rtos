#include "mutex.h"

int os_mutex_init(os_mutex_t *mutex) {
    if (!mutex) {
        return -1;
    }

    mutex->owner = NULL;
    os_task_queue_init(&mutex->waiters);
    return 0;
}

int os_mutex_lock(os_mutex_t *mutex) {
    uint32_t key;

    if (!mutex) {
        return -1;
    }

    for (;;) {
        key = os_port_irq_save();

        if (mutex->owner == NULL) { // mutex is available, take ownership
            mutex->owner = g_current; // set current task as owner
            os_port_irq_restore(key);
            return 0;
        }

        if (mutex->owner == g_current) { // already own the mutex, cannot lock again (non-recursive)
            os_port_irq_restore(key);
            return -1; // non-recursive mutex for now
        }

        os_task_block_locked(&mutex->waiters, OS_WAIT_FOREVER); // caller is already inside a critical section

        os_port_irq_restore(key);
    }
}

int os_mutex_unlock(os_mutex_t *mutex) {
    uint32_t key;

    if (!mutex) {
        return -1;
    }

    key = os_port_irq_save();

    if (mutex->owner != g_current) { // only the owner can unlock the mutex
        os_port_irq_restore(key);
        return -1;
    }

    mutex->owner = NULL; // release ownership
 
    if (mutex->waiters.head != NULL) { // wake up the next waiting task, if any
        os_task_wake_one_locked(&mutex->waiters); // caller is already inside a critical section
        os_schedule(); // pick next task to run (may be the one we just woke up if it has higher priority than current)
        if (g_next != g_current) {
            os_port_pendsv_trigger(); // trigger PendSV to perform context switch to the newly woken task if it's different from current
        }
    }

    os_port_irq_restore(key);
    return 0;
}
