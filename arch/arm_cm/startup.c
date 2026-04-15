#include <stdint.h>

typedef void (*isr_t)(void);

extern uint32_t _estack;

extern uint32_t __user_data_load__;
extern uint32_t __user_data_start__;
extern uint32_t __user_data_end__;
extern uint32_t __kernel_data_load__;
extern uint32_t __kernel_data_start__;
extern uint32_t __kernel_data_end__;
extern uint32_t __user_bss_start__;
extern uint32_t __user_bss_end__;
extern uint32_t __kernel_bss_start__;
extern uint32_t __kernel_bss_end__;

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
{   // qemu execution pauses here when using -S since this is start of reset handler
  os_copy_words(&__user_data_start__, &__user_data_load__, &__user_data_end__);
  os_copy_words(&__kernel_data_start__, &__kernel_data_load__, &__kernel_data_end__);

  os_zero_words(&__user_bss_start__, &__user_bss_end__);
  os_zero_words(&__kernel_bss_start__, &__kernel_bss_end__);

  os_boot_main();

  while (1) {}
}

void Default_Handler(void)
{
  while (1) {}
}
