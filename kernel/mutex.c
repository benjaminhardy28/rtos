#include "mutex.h"
#include "../include/os/syscall.h"

OS_KERNEL_TEXT int os_mutex_init(os_mutex_t *mutex) {
    if (!mutex) {
        return -1;
    }

    mutex->owner = NULL;
    os_task_queue_init(&mutex->waiters);
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
        os_port_irq_restore(key);
        return 0;
    }

    if (mutex->owner == g_current) { // already the owner
        os_port_irq_restore(key);
        return -1;
    }

    os_task_block_locked(&mutex->waiters, OS_WAIT_FOREVER); // block current task on mutex wait queue until it becomes available. Caller is already inside critical section, so we can call the _locked version of block
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
