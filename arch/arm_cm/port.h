#pragma once
#include "../../include/os/section.h"
#include <stdint.h>

OS_KERNEL_TEXT void os_port_set_pendsv_priority_lowest(void); // set PendSV to lowest priority
OS_KERNEL_TEXT void os_port_pendsv_trigger(void); // trigger PendSV exception to perform context switch
OS_KERNEL_TEXT void os_port_mpu_init(void);
OS_KERNEL_TEXT void SVC_Handler(void);

OS_KERNEL_TEXT uint32_t os_port_irq_save(void); // disable interrupts and return previous PRIMASK value
OS_KERNEL_TEXT void os_port_irq_restore(uint32_t primask); // restore PRIMASK to re-enable interrupts if they were previously enabled (if primask == 0)

// __asm format: __asm volatile ("assembly text" : outputs : inputs : clobbers);


OS_USER_TEXT static inline uint32_t os_port_svc_call0(uint32_t syscall_id) {
  register uint32_t r0 __asm("r0") = syscall_id;

  __asm volatile(
    "svc 0\n"
    : "+r"(r0) // r0 is both input (syscall_id) and output (return value), so we use the "+r" constraint
    :
    : "r1", "r2", "r3", "memory"); // r1-r3 are caller-saved registers, so we need to mark them as clobbered since the SVC handler may use them

  return r0;
}

OS_USER_TEXT OS_USER_TEXT static inline uint32_t os_port_svc_call1(uint32_t syscall_id, uint32_t arg1) {
  register uint32_t r0 __asm("r0") = syscall_id;
  register uint32_t r1 __asm("r1") = arg1;

  __asm volatile(
    "svc 0\n"
    : "+r"(r0) // r0 is both input (syscall_id) and output (return value), so we use the "+r" constraint
    : "r"(r1) // r1 is input (arg0)
    : "r2", "r3", "memory"); // r2-r3 are caller-saved registers, so we need to mark them as clobbered since the SVC handler may use them

  return r0;
}

OS_USER_TEXT static inline uint32_t os_port_svc_call2(uint32_t syscall_id, uint32_t arg1, uint32_t arg2) {
  register uint32_t r0 __asm("r0") = syscall_id;
  register uint32_t r1 __asm("r1") = arg1;
  register uint32_t r2 __asm("r2") = arg2;

  __asm volatile(
    "svc 0"
    : "+r"(r0) // r0 is both input (syscall_id) and output (return value), so we use the "+r" constraint
    : "r"(r1), "r"(r2) // r1 and r2 are two arguments
    : "r3", "memory"); // r3 is caller-saved registers, so we need to mark it as clobbered since the SVC handler may use them

  return r0;
}
