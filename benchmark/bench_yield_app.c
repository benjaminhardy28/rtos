#include "../include/os/app.h"
#include "../include/os/bench.h"
#include "../include/os/task.h"
#include <stdint.h>

// Debug variables to watch -- OS_USER_BSS since worker_task/pinger_task run unprivileged;
// without it these fall into the linker script's .bss.kernel catch-all (privileged-only
// RAM) and fault the instant an unprivileged task touches them.
OS_USER_BSS volatile uint32_t g_yield_start_cycle;
OS_USER_BSS volatile uint32_t g_yield_end_cycle;
OS_USER_BSS volatile uint32_t g_yield_latency_cycles;
OS_USER_BSS volatile uint32_t g_worker_ready;
OS_USER_BSS volatile uint32_t g_ping;

OS_USER_TEXT static void worker_task(void *arg)
{
    (void)arg;
    g_worker_ready = 1u;

    for (;;) {
        while (g_ping == 0u) {
            os_yield();
        }

        g_yield_end_cycle = os_bench_clock_now();
        g_yield_latency_cycles = g_yield_end_cycle - g_yield_start_cycle;
        g_ping = 0u;
    }
}


OS_USER_TEXT static void pinger_task(void *arg)
{
    (void)arg;

    os_task_create(OS_TASK_MEDIUM, worker_task, (void *)0, 256);

    while (g_worker_ready == 0u) {
        os_yield();
    }

    g_yield_start_cycle = os_bench_clock_now();
    g_ping = 1u;

    for (;;) {
        os_yield();
    }
}

OS_USER_TEXT void os_app_main(void)
{
    os_bench_clock_init();

    g_worker_ready = 0u;
    g_ping = 0u;

    os_task_create(OS_TASK_MEDIUM, pinger_task, (void *)0, 256);
}
