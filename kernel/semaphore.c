#include "semaphore.h"
#include "scheduler.h"
#include "../arch/arm_cm/port.h"

int os_sem_init(os_sem_t *sem, int32_t initial_count) {
    if (!sem || initial_count < 0) {
        return -1;
    }

    sem->count = initial_count;
    sem->waiters.head = NULL;
    sem->waiters.tail = NULL;
    return 0;
}

int os_sem_take(os_sem_t *sem) {
    uint32_t key;

    if (!sem) {
        return -1;
    }

    key = os_port_irq_save();

    if (sem->count > 0) { // semaphore is available, take it and return
        sem->count--; // decrement count to take the semaphore
        os_port_irq_restore(key);
        return 0;
    }

    os_task_block_locked(&sem->waiters, OS_WAIT_FOREVER, false); // block current task on semaphore wait queue until it becomes available. Caller is already inside critical section, so we can call the _locked version of block
    // Any tasks that wakeup will automatically be given ownership, so the semaphore count doesn't need to be modified here since it's effectively being transferred to the waiting task that gets unblocked when the semaphore is given
    os_port_irq_restore(key); // restore interrupts after blocking (we don't want to restore before blocking since that would allow an interrupt to occur

    return 0;
}


int os_sem_give(os_sem_t *sem) {
    uint32_t key;
    os_tcb_t *task;

    if (!sem) {
        return -1;
    }

    key = os_port_irq_save();

    task = os_task_wake_one_locked(&sem->waiters); // caller is already inside a critical section
    if (task != NULL) {
        os_schedule(); // pick next task to run (may be the one we just woke up if it has higher priority than current)
        if (g_next != g_current) {
            os_port_pendsv_trigger();
        }
    } else {
        sem->count++; // no waiting tasks, just increment the semaphore count
    }

    os_port_irq_restore(key);
    return 0;
}
