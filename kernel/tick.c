#include "tick.h"
#include "../include/os/syscall.h"
#include <stdint.h>

uint32_t volatile g_tick;
static os_task_queue_t g_sleep_queue;

uint32_t os_tick_get(void)
{
  return g_tick;
}

uint32_t os_ticks_from_ms(uint32_t ms)
{
  return ms;
}

void os_delay_ticks(uint32_t ticks)
{
  (void)os_port_svc_call1(OS_SYSCALL_DELAY_TICKS, ticks);
}

void os_kernel_delay_ticks(uint32_t ticks)
{
  uint32_t key;

  if (ticks == 0u) {
    os_yield();
    return;
  }

  key = os_port_irq_save();
  os_task_block_locked(&g_sleep_queue, ticks);
  os_port_irq_restore(key);
}

void SysTick_Handler(void)
{
  g_tick++;
  os_process_timeouts();
  if(g_tick % 1000 == 0) {
    // do something every 1000 ticks (1 second)
    os_ready_queue_rotate(g_current); // rotate current task to end of its priority's ready queue for round-robin scheduling among same priority tasks
    os_schedule(); // pick next task
    os_port_pendsv_trigger(); // trigger PendSV to perform context switch, will eventually only do if task is changing
  }
}
