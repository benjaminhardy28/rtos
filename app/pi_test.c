
#include "../include/os/app.h"
#include "../include/os/task.h"
#include "../include/os/mutex.h"
#include "../include/os/section.h"
#include <stddef.h>
#include <stdint.h>

/*
 * Classic priority-inversion scenario, built to run without tick support
 * (systick_enable() is currently disabled in kernel/task.c, so this can't
 * rely on os_delay_ticks). Sequencing instead relies on the fact that
 * os_task_create() preempts immediately when the new task outranks the
 * caller, and that priority inheritance boosts the mutex owner back above
 * the medium-priority hog before it can preempt.
 *
 * Trace, if priority inheritance is working:
 *   1. L (LOW) is the only real task at boot, so it runs first and locks
 *      the mutex uncontended.
 *   2. L creates H (HIGH). This immediately preempts L (unboosted, still
 *      LOW) and switches to H.
 *   3. H tries to lock the mutex, finds it owned by L, boosts L to HIGH,
 *      and blocks.
 *   4. L (now HIGH) resumes right where os_task_create(H, ...) returned.
 *   5. L creates M (MEDIUM). Because L is currently boosted to HIGH, this
 *      does NOT preempt L -- M just sits ready.
 *   6. L finishes its "critical section" and unlocks, restoring itself to
 *      LOW and handing the mutex straight to H.
 *   7. H acquires the mutex promptly and sets g_pi_high_acquired = 1.
 *
 * Without priority inheritance, step 5 would instead have M (MEDIUM,
 * outranks L's unboosted LOW) preempt L on creation. M never blocks or
 * yields, so it would spin forever, L would never get back to unlock the
 * mutex, and H would stay blocked on it permanently --
 * g_pi_high_acquired would never become 1.
 */

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
