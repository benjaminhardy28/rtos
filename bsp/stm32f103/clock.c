// RCC (Reset and Clock Control) bring-up for real STM32F103 silicon: HSE 8MHz
// crystal -> PLL x9 -> 72MHz SYSCLK, the chip's rated max.
//
// Only meaningful on real hardware. Renode's platform files
// (scripts/renode/*.repl) don't model RCC at all -- an unmapped register read
// there returns 0 forever, so the HSERDY/PLLRDY/SWS polls below would spin
// forever and hang the boot. This file is only compiled/called when building
// with HW=1 (see Makefile); the default Renode build never touches RCC and
// stays on the fixed frequency Renode's SysTick/DWT models assume.
#include "clock.h"

#define RCC_CR    (*(volatile uint32_t *)0x40021000u)
#define RCC_CFGR  (*(volatile uint32_t *)0x40021004u)
#define FLASH_ACR (*(volatile uint32_t *)0x40022000u)

#define RCC_CR_HSEON   (1u << 16)
#define RCC_CR_HSERDY  (1u << 17)
#define RCC_CR_PLLON   (1u << 24)
#define RCC_CR_PLLRDY  (1u << 25)

#define RCC_CFGR_SW_PLL      (2u << 0)  // select PLL as SYSCLK source
#define RCC_CFGR_SWS_MASK    (3u << 2)
#define RCC_CFGR_SWS_PLL     (2u << 2)  // status: PLL actually selected
#define RCC_CFGR_PPRE1_DIV2  (4u << 8)  // APB1 max is 36MHz, so /2 from 72MHz
#define RCC_CFGR_PLLSRC_HSE  (1u << 16) // PLL fed from HSE (not HSI/2)
#define RCC_CFGR_PLLMUL_X9   (7u << 18) // 0b0111 encodes x9 (encoded = mult - 2)

#define FLASH_ACR_LATENCY_2WS (2u << 0) // required for 48-72MHz flash reads
#define FLASH_ACR_PRFTBE      (1u << 4) // prefetch buffer, recommended with wait states

void clock_init(void)
{
  // Flash must be able to keep up with 72MHz fetches before we switch SYSCLK
  // to the PLL -- doing this after would fetch instructions faster than
  // flash can deliver them.
  FLASH_ACR = FLASH_ACR_PRFTBE | FLASH_ACR_LATENCY_2WS;

  RCC_CR |= RCC_CR_HSEON;
  while (!(RCC_CR & RCC_CR_HSERDY)) {}

  RCC_CFGR = (RCC_CFGR & ~0x003F0000u)
           | RCC_CFGR_PPRE1_DIV2
           | RCC_CFGR_PLLSRC_HSE
           | RCC_CFGR_PLLMUL_X9;

  RCC_CR |= RCC_CR_PLLON;
  while (!(RCC_CR & RCC_CR_PLLRDY)) {}

  RCC_CFGR = (RCC_CFGR & ~0x3u) | RCC_CFGR_SW_PLL;
  while ((RCC_CFGR & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {}
}
