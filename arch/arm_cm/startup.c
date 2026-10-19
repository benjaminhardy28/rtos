#include <stdint.h>

typedef void (*isr_t)(void);

extern uint32_t _estack;

extern uint32_t __data_load__;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;

void os_boot_main(void);

void Reset_Handler(void);
void Default_Handler(void);

// weak alias tells linker to use Default_Handler if no other definition is found
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

__attribute__((section(".isr_vector"))) // place vector table in special section for vector table, defined at address 0x0
const isr_t isr_vector[] = {
  (isr_t)&_estack,
  Reset_Handler,
  NMI_Handler,
  HardFault_Handler,
  MemManage_Handler,
  BusFault_Handler,
  UsageFault_Handler,
  0, 0, 0, 0,
  SVC_Handler,
  DebugMon_Handler,
  0,
  PendSV_Handler,
  SysTick_Handler
};

static void os_copy_words(uint32_t *dst, const uint32_t *src, uint32_t *end)
{
  while (dst < end) {
    *dst++ = *src++;
  }
}

static void os_zero_words(uint32_t *dst, uint32_t *end)
{
  while (dst < end) {
    *dst++ = 0;
  }
}

// Initialize memory
void Reset_Handler(void)
{   // execution pauses here when a debugger halts at reset (e.g. Renode's `-S`-style start)
  os_copy_words(&__data_start__, &__data_load__, &__data_end__);

  os_zero_words(&__bss_start__, &__bss_end__);

  os_boot_main();

  while (1) {}
}

void Default_Handler(void)
{
  while (1) {}
}
