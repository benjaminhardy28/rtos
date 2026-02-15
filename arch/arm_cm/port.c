// arch/arm_cm3/port.c
#include "port.h"


extern os_tcb_t *os_current_tcb(void);
extern void os_commit_switch(void);
extern volatile uint32_t g_first_switch;

#define SCB_ICSR   (*(volatile uint32_t *)0xE000ED04u) // Interrupt Control and State Register
#define SCB_SHPR3  (*(volatile uint32_t *)0xE000ED20u) // System Handler Priority Register 3 (for PendSV and SysTick)

#define ICSR_PENDSVSET (1u << 28) // Bit to set PendSV pending

void os_port_set_pendsv_priority_lowest(void) { // set PendSV to lowest priority by writing 0xFF to its priority field in SCB_SHPR3
  uint32_t v = SCB_SHPR3;
  v &= ~(0xFFu << 16); // Clear current PendSV priority bits
  v |=  (0xFFu << 16); // Set PendSV priority to lowest (0xFF)
  SCB_SHPR3 = v;
}

void os_port_pendsv_trigger(void) {
  SCB_ICSR |= ICSR_PENDSVSET; // Set PendSV pending bit to trigger PendSV exception for context switch
}

__attribute__((naked)) void PendSV_Handler(void) {
  __asm volatile(
    "push {lr}              \n" // save EXC_RETURN
    "mrs r0, psp            \n" // r0 = vcurrent PSP

    "ldr r2, =g_first_switch\n" // r2 = &g_first_switch
    "ldr r3, [r2]           \n" // r3 = g_first_switch
    "cbnz r3, 1f            \n" // if first_switch != 0, skip save

    "stmdb r0!, {r4-r11}    \n" // push r4-r11, r0 = saved_sp
    "mov r1, r0             \n" // r1 = saved_sp (preserve)
    "bl os_current_tcb      \n" // r0 = current_tcb
    "str r1, [r0]           \n" // current_tcb->sp = saved_sp
    "b 2f                   \n"

    "1:                     \n"
    "movs r3, #0            \n"
    "str r3, [r2]           \n" // clear first_switch flag

    "2:                     \n"
    "bl os_commit_switch    \n" // choose next task (updates current_tcb)
    "bl os_current_tcb      \n" // r0 = next_tcb
    "ldr r0, [r0]           \n" // r0 = next_tcb->sp
    "ldmia r0!, {r4-r11}    \n" // pop r4-r11, r0 = PSP to hw frame
    "msr psp, r0            \n" // PSP = next PSP

    "pop {lr}               \n" // restore EXC_RETURN
    "bx lr                  \n" // exception return (restores hw frame)
  );
}
