#include "tick.h"
#include <stdint.h>

uint32_t volatile g_tick;

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
  uint32_t start = os_tick_get();
  while ((uint32_t)(os_tick_get() - start) < ticks) {}
}

void SysTick_Handler(void)
{
  g_tick++;
  if(g_tick % 1000 == 0) {
    // do something every 1000 ticks (1 second)
    os_ready_queue_rotate(g_current); // rotate current task to end of its priority's ready queue for round-robin scheduling among same priority tasks
    os_schedule(); // pick next task
    os_port_pendsv_trigger(); // trigger PendSV to perform context switch, will eventually only do if task is changing
  }
}
