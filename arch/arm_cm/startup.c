#include <stdint.h>

typedef void (*isr_t)(void);

extern uint32_t _estack;

extern uint32_t _sidata;
extern uint32_t _sdata;
extern uint32_t _edata;

extern uint32_t _sbss;
extern uint32_t _ebss;

int main(void);

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

// Initialize memory
void Reset_Handler(void)
{   // qemu execution pauses here when using -S since this is start of reset handler
  // store .data in RAM for initiliazed global and static variables
  uint32_t *src = &_sidata;
  uint32_t *dst = &_sdata;
  while (dst < &_edata) {
    *dst++ = *src++;
  }

  // zero values in .bss section, also storing in RAM
  dst = &_sbss;
  while (dst < &_ebss) {
    *dst++ = 0;
  }

  // call main script
  (void)main();

  while (1) {}
}

void Default_Handler(void)
{
  while (1) {}
}
