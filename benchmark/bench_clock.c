#include "../include/os/bench.h"

#define DEMCR      (*(volatile uint32_t *)0xE000EDFCu)
#define DWT_CTRL   (*(volatile uint32_t *)0xE0001000u)
#define DWT_CYCCNT (*(volatile uint32_t *)0xE0001004u)

#define DEMCR_TRCENA  (1u << 24)
#define DWT_CYCCNTENA (1u << 0)

OS_USER_TEXT void os_bench_clock_init(void)
{
  DEMCR |= DEMCR_TRCENA;
  DWT_CYCCNT = 0u;
  DWT_CTRL |= DWT_CYCCNTENA;
}

OS_USER_TEXT uint32_t os_bench_clock_now(void)
{
  return DWT_CYCCNT;
}
