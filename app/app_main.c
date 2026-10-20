
#include "../include/os/app.h"
#include "../include/os/bench.h"
#include "../include/os/lfqueue.h"
#include "../include/os/rtt.h"
#include "../include/os/task.h"
#include "../include/os/tick.h"
#include <stddef.h>
#include <stdint.h>

volatile uint32_t g_bss;

volatile uint32_t g_reached_main;

static os_spsc_queue_t g_queue;
static uint32_t g_queue_storage[8];

/* Debug variables you can watch in GDB */
volatile uint32_t g_task1_sent;
volatile uint32_t g_task2_recv;
volatile uint32_t g_last_sent;
volatile uint32_t g_last_recv;
volatile uint32_t g_error_count;

static void task0(void *arg) {
    uint32_t value = 1;
    uint32_t *slot;

    (void)arg;
    //os_delay_ticks(50);
    for (;;) {
        g_last_sent = value;

        slot = (uint32_t *)get_write_location(&g_queue);
        if (slot != NULL) {
            *slot = value;
            (void)update_write_index(&g_queue);
            uint32_t now = os_bench_clock_now();
            char clock_buf[10];
            uint32_t clock_len = u32_to_str(now, clock_buf);
            os_rtt_puts("Sent: ");
            os_rtt_write(clock_buf, clock_len);
            g_task1_sent++;
            value++;
           
        }
        os_yield();

    //    os_delay_ticks(10);
    }
}

static void task1(void *arg) {
    uint32_t value;
    uint32_t expected = 1;
    uint32_t *slot;

    (void)arg;

    for (;;) {
        slot = (uint32_t *)get_read_location(&g_queue);
        if (slot != NULL) {
            value = *slot;
            (void)update_read_index(&g_queue);
            uint32_t now = os_bench_clock_now();
            char clock_buf[10];
            uint32_t clock_len = u32_to_str(now, clock_buf);
            os_rtt_puts("Received: ");
            os_rtt_write(clock_buf, clock_len);
            g_last_recv = value;
            g_task2_recv++;

            if (value != expected) {
                g_error_count++;
                expected = value + 1;
            } else {
                expected++;
            }
        }
        os_yield();

        //os_delay_ticks(15);
    }
}

void os_app_main(void)
{
    g_reached_main = 0xA5A5A5A5;

    lfqueue_init(&g_queue, g_queue_storage, sizeof(uint32_t), 8);

    os_task_create(OS_TASK_MEDIUM, task0, (void *)0, 256);
    os_task_create(OS_TASK_MEDIUM, task1, (void *)1, 256);
}
