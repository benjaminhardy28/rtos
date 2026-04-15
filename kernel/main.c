#include "task.h"
#include "kalloc.h"
#include "../include/os/app.h"
#include <stdint.h>

OS_KERNEL_TEXT static void os_idle_task(void *arg) {
    (void)arg;
    for (;;) {
    }
}

OS_KERNEL_TEXT void os_boot_main(void)
{
  os_kalloc_init(); // Allocate memory regions for kernel objects and task stacks

  // Create IDLE task so there is always a task for the scheduler to choose
  os_task_create(OS_TASK_IDLE, os_idle_task, NULL, 128);

  // Let the application register user tasks before the scheduler starts.
  os_app_main();

  os_start(); // starts scheduler ONCE (enables tick + triggers first PendSV)

  while (1) {
    // should never get here
  }
}
