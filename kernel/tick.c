#include "tick.h"
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
  uint32_t key;

  if (ticks == 0u) {
    os_yield();
    return;
  }

  key = os_port_irq_save();
  os_task_block_locked(&g_sleep_queue, ticks, false);
  os_port_irq_restore(key);
}

void SysTick_Handler(void)
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

