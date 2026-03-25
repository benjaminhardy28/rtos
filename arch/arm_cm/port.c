// arch/arm_cm3/port.c
#include "port.h"

extern uint32_t __user_flash_region_start__;
extern uint32_t __user_flash_region_end__;
extern uint32_t __kernel_flash_region_start__;
extern uint32_t __kernel_flash_region_end__;
extern uint32_t __user_ram_region_start__;
extern uint32_t __user_ram_region_end__;
extern uint32_t __kernel_ram_region_start__;
extern uint32_t __kernel_ram_region_end__;

extern os_tcb_t *os_current_tcb(void);
extern void os_commit_switch(void);
extern volatile uint32_t g_first_switch;

#define SCB_ICSR   (*(volatile uint32_t *)0xE000ED04u) // Interrupt Control and State Register
#define SCB_SHCSR  (*(volatile uint32_t *)0xE000ED24u)
#define SCB_SHPR3  (*(volatile uint32_t *)0xE000ED20u) // System Handler Priority Register 3 (for PendSV and SysTick)
#define MPU_TYPE   (*(volatile uint32_t *)0xE000ED90u)
#define MPU_CTRL   (*(volatile uint32_t *)0xE000ED94u)
#define MPU_RNR    (*(volatile uint32_t *)0xE000ED98u)
#define MPU_RBAR   (*(volatile uint32_t *)0xE000ED9Cu)
#define MPU_RASR   (*(volatile uint32_t *)0xE000EDA0u)

#define ICSR_PENDSVSET (1u << 28) // Bit to set PendSV pending
#define SHCSR_MEMFAULTENA (1u << 16)

#define MPU_CTRL_ENABLE      (1u << 0)
#define MPU_CTRL_PRIVDEFENA  (1u << 2)

#define MPU_RASR_XN          (1u << 28)
#define MPU_AP_PRIV_RW       (1u << 24)
#define MPU_AP_FULL_ACCESS   (3u << 24)
#define MPU_AP_PRIV_RO       (5u << 24)
#define MPU_AP_RO_ALL        (6u << 24)
#define MPU_ATTR_FLASH       (1u << 17)
#define MPU_ATTR_SRAM        ((1u << 17) | (1u << 16))
#define MPU_REGION_ENABLE    (1u << 0)

static uint32_t os_port_mpu_size_encode(uint32_t size_bytes) {
  uint32_t encode = 0;

  while (size_bytes > 1u) {
    size_bytes >>= 1;
    encode++;
  }

  return (encode - 1u) << 1;
}

static void os_port_mpu_region_configure(uint32_t region, uintptr_t base, uint32_t size_bytes, uint32_t attr) {
  MPU_RNR = region;
  MPU_RBAR = base;
  MPU_RASR = attr | os_port_mpu_size_encode(size_bytes) | MPU_REGION_ENABLE;
}

OS_KERNEL_TEXT void os_port_set_pendsv_priority_lowest(void) { // set PendSV to lowest priority by writing 0xFF to its priority field in SCB_SHPR3
  uint32_t v = SCB_SHPR3;
  v &= ~(0xFFu << 16); // Clear current PendSV priority bits
  v |=  (0xFFu << 16); // Set PendSV priority to lowest (0xFF)
  SCB_SHPR3 = v;
}

OS_KERNEL_TEXT void os_port_pendsv_trigger(void) {
  SCB_ICSR |= ICSR_PENDSVSET; // Set PendSV pending bit to trigger PendSV exception for context switch
}

OS_KERNEL_TEXT void os_port_mpu_init(void) {
  uint32_t user_flash_size = (uint32_t)((uintptr_t)&__user_flash_region_end__ - (uintptr_t)&__user_flash_region_start__);
  uint32_t kernel_flash_size = (uint32_t)((uintptr_t)&__kernel_flash_region_end__ - (uintptr_t)&__kernel_flash_region_start__);
  uint32_t user_ram_size = (uint32_t)((uintptr_t)&__user_ram_region_end__ - (uintptr_t)&__user_ram_region_start__);
  uint32_t kernel_ram_size = (uint32_t)((uintptr_t)&__kernel_ram_region_end__ - (uintptr_t)&__kernel_ram_region_start__);

  if ((MPU_TYPE & 0xFFu) == 0u) {
    return;
  }

  __asm volatile("dsb");
  __asm volatile("isb");

  MPU_CTRL = 0u;
  SCB_SHCSR |= SHCSR_MEMFAULTENA;

  os_port_mpu_region_configure(0u, (uintptr_t)&__user_flash_region_start__, user_flash_size, MPU_AP_RO_ALL | MPU_ATTR_FLASH);
  os_port_mpu_region_configure(1u, (uintptr_t)&__kernel_flash_region_start__, kernel_flash_size, MPU_AP_PRIV_RO | MPU_ATTR_FLASH);
  os_port_mpu_region_configure(2u, (uintptr_t)&__user_ram_region_start__, user_ram_size, MPU_AP_FULL_ACCESS | MPU_ATTR_SRAM | MPU_RASR_XN);
  os_port_mpu_region_configure(3u, (uintptr_t)&__kernel_ram_region_start__, kernel_ram_size, MPU_AP_PRIV_RW | MPU_ATTR_SRAM | MPU_RASR_XN);

  MPU_CTRL = MPU_CTRL_ENABLE | MPU_CTRL_PRIVDEFENA;

  __asm volatile("dsb");
  __asm volatile("isb");
}

OS_KERNEL_TEXT void SVC_Handler(void) {
}

OS_KERNEL_TEXT __attribute__((naked)) void PendSV_Handler(void) {
  __asm volatile(
    "push {lr}              \n" // save EXC_RETURN
    "mrs r0, psp            \n" // r0 = vcurrent PSP

    "ldr r2, =g_first_switch\n" // r2 = &g_first_switch
    "ldr r3, [r2]           \n" // r3 = g_first_switch
    "cbnz r3, 1f            \n" // if first_switch != 0, skip save because we haven't started the scheduler yet and don't have a current task to save context for

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

OS_KERNEL_TEXT uint32_t os_port_irq_save(void) {
  uint32_t primask;
  __asm volatile(
    "mrs %0, PRIMASK \n" // read current PRIMASK value
    "cpsid i         \n" // disable interrupts
    : "=r"(primask)
    :
    : "memory");
  return primask;
}

OS_KERNEL_TEXT void os_port_irq_restore(uint32_t primask) {
  __asm volatile(
    "msr PRIMASK, %0 \n" // restore PRIMASK to re-enable interrupts if they were previously enabled (if primask == 0)
    :
    : "r"(primask)
    : "memory");
}
