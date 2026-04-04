#include "tick.h"
#include "task.h"
#include "scheduler.h"
#include "kalloc.h"
#include "mutex.h"
#include "semaphore.h"
#include "../include/os/mutex.h"
#include "../include/os/semaphore.h"
#include "../arch/arm_cm/port.h"
#include "../bsp/qemu_mps2/systick.h"
#include "../include/os/section.h"
#include "../include/os/queue.h"
#include <stdint.h>

OS_USER_DATA volatile uint32_t g_data = 0x11223344;
OS_USER_BSS volatile uint32_t g_bss;

OS_USER_BSS volatile uint32_t g_reached_main;

OS_USER_TEXT static void os_idle_task(void *arg) {
    (void)arg;
    for (;;) {
    }
}

static os_queue_t g_queue;
static uint32_t g_queue_storage[8];

/* Debug variables you can watch in GDB/QEMU */
volatile uint32_t g_task1_sent = 0;
volatile uint32_t g_task2_recv = 0;
volatile uint32_t g_last_sent = 0;
volatile uint32_t g_last_recv = 0;
volatile uint32_t g_error_count = 0;

static void task0(void *arg) {
    uint32_t value = 1;
    (void)arg;
    os_delay_ticks(50);
    for (;;) {
        g_last_sent = value;

        if (os_queue_send(&g_queue, &value) == 0) {
            g_task1_sent++;
            value++;
        } else {
            g_error_count++;
        }

        os_delay_ticks(10);
    }
}

static void task1(void *arg) {
    uint32_t value;
    uint32_t expected = 1;
    (void)arg;

    for (;;) {
        if (os_queue_recv(&g_queue, &value) == 0) {
            g_last_recv = value;
            g_task2_recv++;

            if (value != expected) {
                g_error_count++;
                expected = value + 1;
            } else {
                expected++;
            }
        } else {
            g_error_count++;
        }

        os_delay_ticks(15);
    }
}


OS_USER_TEXT int main(void)
{
  g_reached_main = 0xA5A5A5A5;
  
  os_kalloc_init(); // Allocate memory regions for kernel objects and task stacks

  // initialize static kernel objects
  os_queue_init(&g_queue, g_queue_storage, sizeof(uint32_t), 8);
  //os_sem_init(&g_test_sem, 0);

  // Create IDLE task so there is always a task for the scheduler to choose
  os_task_create(OS_TASK_IDLE, os_idle_task, NULL, 128);

  // Create tasks for application
  os_task_create(OS_TASK_MEDIUM, task0, (void *)0, 256);
  os_task_create(OS_TASK_MEDIUM, task1, (void *)1, 256);

  os_start(); // starts scheduler ONCE (enables tick + triggers first PendSV)

  while (1) {
    // should never get here
  }
}
