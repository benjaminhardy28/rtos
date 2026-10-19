#include "../include/os/app.h"
#include "../include/os/bench.h"
#include "../include/os/task.h"
#include <stdint.h>

// Debug variables to watch
volatile uint32_t g_yield_start_cycle;
volatile uint32_t g_yield_end_cycle;
volatile uint32_t g_yield_latency_cycles;
volatile uint32_t g_worker_ready;
volatile uint32_t g_ping;

static void worker_task(void *arg)
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


static void pinger_task(void *arg)
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

void os_app_main(void)
{
    os_bench_clock_init();

    g_worker_ready = 0u;
    g_ping = 0u;

    os_task_create(OS_TASK_MEDIUM, pinger_task, (void *)0, 256);
}
