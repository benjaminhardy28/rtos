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

OS_USER_TEXT static void os_idle_task(void *arg) {
    (void)arg;
    for (;;) {
    }
}
// temporary tasks
OS_USER_BSS volatile uint32_t g_last_task_id;

OS_USER_TEXT static void task0(void *arg) {
  (void)arg;
  int i = 0;
  for (;;) {
    // do stuff
    g_last_task_id = 0;
    //os_yield();
    if(i%100 == 0) {
      os_delay_ticks(10); // delay for 10 ticks every 1000 iterations to test blocking and timeouts
    }
    i++;
  }
}

OS_USER_TEXT static void task1(void *arg) {
  (void)arg;
  int i = 0;
  for (;;) {
    // do stuff
    g_last_task_id = 1;
    //os_yield();
    if(i%100 == 0) {
      os_delay_ticks(10); // delay for 10 ticks every 1000 iterations to test blocking and timeouts
    }
    i++;
  }
}


OS_USER_TEXT int main(void)
{
  g_reached_main = 0xA5A5A5A5;
  
  os_task_create(OS_TASK_IDLE, os_idle_task, NULL, 128);
  os_task_create(OS_TASK_MEDIUM, task0, (void *)0, 256);
  os_task_create(OS_TASK_MEDIUM, task1, (void *)1, 256);

  os_start(); // starts scheduler ONCE (enables tick + triggers first PendSV)

  while (1) {
    // should never get here
  }
}
