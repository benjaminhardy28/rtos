#include "task.h"
#include "kalloc.h"
#include "../include/os/app.h"
#include "../include/os/bench.h"
#include <stdint.h>

// Runs like any other task -- dispatched through the same trampoline as other
// tasks (see os_task_start_trampoline in arch/arm_cm/port.c).
static void os_idle_task(void *arg) {
    (void)arg;
    for (;;) {
    }
}

void os_boot_main(void)
{
  os_kalloc_init(); // Allocate memory regions for kernel objects and task stacks

  os_bench_clock_init(); // Gates DWT's bus interface (DEMCR.TRCENA)

  // Create IDLE task so there is always a task for the scheduler to choose
  os_task_create(OS_TASK_IDLE, os_idle_task, NULL, 128);

  // Let the application register user tasks before the scheduler starts.
  os_app_main();

  os_start(); // starts scheduler ONCE (enables tick + triggers first PendSV)

  while (1) {
    // should never get here
  }
}
