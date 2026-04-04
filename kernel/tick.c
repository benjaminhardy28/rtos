#include "tick.h"
#include "../include/os/syscall.h"
#include <stdint.h>

OS_USER_BSS uint32_t volatile g_tick;
OS_KERNEL_BSS static os_task_queue_t g_sleep_queue;

OS_USER_TEXT uint32_t os_tick_get(void)
{
  return g_tick;
}

OS_USER_TEXT uint32_t os_ticks_from_ms(uint32_t ms)
{
  return ms;
}

OS_USER_TEXT void os_delay_ticks(uint32_t ticks)
{
  (void)os_port_svc_call1(OS_SYSCALL_DELAY_TICKS, ticks);
}

OS_KERNEL_TEXT void os_kernel_delay_ticks(uint32_t ticks)
{
  uint32_t key;

  if (ticks == 0u) {
    os_kernel_yield();
    return;
  }

  key = os_port_irq_save();
  os_task_block_locked(&g_sleep_queue, ticks);
  os_port_irq_restore(key);
}

OS_KERNEL_TEXT void SysTick_Handler(void)
{
  g_tick++;

  os_process_timeouts();

  if (g_current != NULL) {
    os_ready_queue_rotate(g_current);
  }

  os_schedule();

  if (g_current != g_next) {
    os_port_pendsv_trigger();
  }
}

