
#include "../include/os/app.h"
#include "../include/os/task.h"
#include "../include/os/mutex.h"
#include "../include/os/section.h"
#include <stddef.h>
#include <stdint.h>

/* Priority-inversion scenario: L locks the mutex, H preempts and blocks on it boosting L to
 * HIGH so M can't preempt; L unlocks, H acquires the mutex and sets g_pi_high_acquired = 1.
 * Without PI, M would preempt L on creation and spin forever, so H never acquires it. */

#define PI_BUSY_ITERS 50000u

OS_USER_BSS volatile uint32_t g_reached_main;

OS_USER_BSS static os_mutex_t g_pi_mutex;

/* Debug globals -- watch these in GDB/Renode */
OS_USER_BSS volatile uint32_t g_pi_low_locked;    /* L took the mutex uncontended */
OS_USER_BSS volatile uint32_t g_pi_low_done;      /* L finished its critical section and unlocked */
OS_USER_BSS volatile uint32_t g_pi_medium_spins;  /* heartbeat: how far M's hog loop got */
OS_USER_BSS volatile uint32_t g_pi_high_acquired; /* H got the mutex -- the thing we're checking */

OS_USER_TEXT static void pi_high_task(void *arg) {
    (void)arg;

    (void)os_mutex_lock(&g_pi_mutex); /* blocks until L unlocks; should be prompt if PI works */
    g_pi_high_acquired = 1;
    (void)os_mutex_unlock(&g_pi_mutex);

    for (;;) {
    }
}

OS_USER_TEXT static void pi_medium_task(void *arg) {
    (void)arg;

    /* Pure CPU hog: never blocks, never yields. This is the task that
     * would starve L (and transitively H) if L were never boosted. */
    for (;;) {
        g_pi_medium_spins++;
    }
}

OS_USER_TEXT static void pi_low_task(void *arg) {
    volatile uint32_t i;

    (void)arg;

    (void)os_mutex_lock(&g_pi_mutex); /* uncontended: mutex is free */
    g_pi_low_locked = 1;

    os_task_create(OS_TASK_HIGH, pi_high_task, NULL, 256);
    /* ^ preempts us immediately; H blocks on g_pi_mutex and boosts us back to HIGH */

    os_task_create(OS_TASK_MEDIUM, pi_medium_task, NULL, 256);
    /* ^ does NOT preempt us: we're boosted to HIGH right now, so M just waits ready */

    for (i = 0; i < PI_BUSY_ITERS; i++) {
        /* simulated critical-section work */
    }

    g_pi_low_done = 1;
    (void)os_mutex_unlock(&g_pi_mutex); /* restores us to LOW, hands mutex straight to H */

    for (;;) {
    }
}

OS_USER_TEXT void os_app_main(void) {
    g_reached_main = 0xA5A5A5A5;

    os_mutex_init(&g_pi_mutex);

    os_task_create(OS_TASK_LOW, pi_low_task, NULL, 256);
    /* H and M are created dynamically by pi_low_task, not here -- creating
     * them up front would let H (highest priority) run first and block
     * before L ever gets a chance to lock the mutex. */
}
