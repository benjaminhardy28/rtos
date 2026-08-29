// SysTick is part of the ARMv7-M core (System Control Space), not a vendor peripheral, so
// this has no chip-specific logic yet -- it's its own BSP purely so STM32F103 has its own
// place to grow chip-specific peripheral code (CAN, GPIO, USART, ...).
#include "systick.h"

#define SYST_CSR (*(volatile uint32_t *)0xE000E010u) // Control and Status Register: Enables the counter, interrupt and selects the clock source
#define SYST_RVR (*(volatile uint32_t *)0xE000E014u) // Reload Value Register: Sets the start value to count down from
#define SYST_CVR (*(volatile uint32_t *)0xE000E018u) // Current Value Register: Current value of the counter

void systick_init(uint32_t cpu_hz, uint32_t tick_hz)
{
    uint32_t reload = (cpu_hz / tick_hz) - 1u;

    SYST_CSR = 0u; // Clear SysTick during setup
    SYST_RVR = reload; // Set reload register, setup tick period
    SYST_CVR = 0u; // Reset the current value register
    SYST_CSR = (1u << 2) | (1u << 0); // Enable counter + clock, but NOT interrupt yet
}

void systick_enable(){
    SYST_CSR |= (1u << 1); // Enable SysTick interrupt
}
