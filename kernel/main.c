#include "task.h"
#include "kalloc.h"
#include "../include/os/app.h"
#include <stdint.h>

// Runs like any other task -- dispatched unprivileged through the same trampoline as user
// tasks (see os_task_start_trampoline in arch/arm_cm/port.c) -- so it must live in
// user-accessible flash; OS_KERNEL_TEXT faulted the scheduler's first (idle task) switch.
OS_USER_TEXT static void os_idle_task(void *arg) {
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
