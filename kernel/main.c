#include "tick.h"
#include "task.h"
#include "scheduler.h"
#include "../arch/arm_cm/port.h"
#include "../bsp/qemu_mps2/systick.h"
#include "../include/os/section.h"
#include <stdint.h>

OS_USER_DATA volatile uint32_t g_data = 0x11223344;
OS_USER_BSS volatile uint32_t g_bss;

OS_USER_BSS volatile uint32_t g_reached_main;

OS_KERNEL_BSS static os_tcb_t tcb0;
OS_KERNEL_BSS static os_tcb_t tcb1;

OS_USER_STACK static uint32_t stack0[256];
OS_USER_STACK static uint32_t stack1[256];

// idle task for when no other tasks are ready to run (just loops forever)
OS_KERNEL_BSS static os_tcb_t idle_tcb;
OS_USER_STACK static uint32_t idle_stack[128];

OS_USER_TEXT static void os_idle_task(void *arg) {
    (void)arg;
    for (;;) {
    }
}
// temporary tasks
OS_USER_BSS volatile uint32_t g_last_task_id;

OS_USER_TEXT static void task0(void *arg) {
  (void)arg;
  systick_enable(); // enable SysTick interrupt after first context switch to avoid
  for (;;) {
    // do stuff
    g_last_task_id = 0;
    //os_yield();
  }
}

OS_USER_TEXT static void task1(void *arg) {
  (void)arg;
  for (;;) {
    // do stuff
    g_last_task_id = 1;
    //os_yield();
  }
}


OS_USER_TEXT int main(void)
{
  g_reached_main = 0xA5A5A5A5;
  
  // initialies idle task (priority 3, lowest priority)
  os_task_create(&idle_tcb, OS_TASK_IDLE, os_idle_task, NULL, idle_stack, sizeof(idle_stack) / sizeof(idle_stack[0]));
  os_ready_queue_add(&idle_tcb);

  // int os_task_create(os_tcb_t *tcb, os_task_fn_t entry, void *arg, uint32_t *stack_mem, uint32_t stack_words);
  os_task_create(&tcb0, 1, task0, (void*)0, stack0, sizeof(stack0) / sizeof(stack0[0]));
  os_task_create(&tcb1, 1, task1, (void*)1, stack1, sizeof(stack1) / sizeof(stack1[1]));
  os_ready_queue_add(&tcb0);
  os_ready_queue_add(&tcb1);

  os_start(); // starts scheduler ONCE (enables tick + triggers first PendSV)

  while (1) {
    // should never get here
  }
}
