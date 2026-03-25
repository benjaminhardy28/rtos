#include "semaphore.h"
#include "scheduler.h"
#include "../arch/arm_cm/port.h"
#include "../include/os/syscall.h"

int os_sem_init(os_sem_t *sem, int32_t initial_count) {
    if (!sem || initial_count < 0) {
        return -1;
    }

    sem->count = initial_count;
    os_task_queue_init(&sem->waiters);
    return 0;
}

int os_sem_take(os_sem_t *sem) {
    return (int)os_port_svc_call1(OS_SYSCALL_SEM_TAKE, (uint32_t)(uintptr_t)sem);
}

int os_kernel_sem_take(os_sem_t *sem) {
    uint32_t key;

    if (!sem) {
        return -1;
    }

    for (;;) {
        key = os_port_irq_save();

        if (sem->count > 0) {
            sem->count--; // take the semaphore
            os_port_irq_restore(key);
            return 0; // successfully took the semaphore
        }

        os_task_block_locked(&sem->waiters, OS_WAIT_FOREVER); // caller is already inside a critical section

        os_port_irq_restore(key);
    }
}

int os_sem_give(os_sem_t *sem) {
    return (int)os_port_svc_call1(OS_SYSCALL_SEM_GIVE, (uint32_t)(uintptr_t)sem);
}

int os_kernel_sem_give(os_sem_t *sem) {
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
