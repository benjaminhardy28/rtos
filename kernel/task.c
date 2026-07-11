#include "task.h"
#include "scheduler.h"
#include "kalloc.h"
#include "../include/os/syscall.h"

extern void os_port_pendsv_trigger(void);

OS_KERNEL_BSS uint32_t volatile g_first_switch;

OS_KERNEL_BSS os_tcb_t *g_current = NULL;
OS_KERNEL_BSS os_tcb_t *g_next = NULL;

// temp

OS_USER_BSS volatile uint32_t g_debug_step;
OS_USER_BSS volatile uint32_t g_debug_value0;
OS_USER_BSS volatile uint32_t g_debug_value1;

OS_KERNEL_TEXT static void os_task_exit_trap(void) { // called when a task function returns (this is a backup, since it shouldn't ever return)
  for (;;) {}
}

OS_USER_TEXT int os_task_create(os_task_priority_t priority, os_task_fn_t entry, void *arg, uint32_t stack_words) {
  os_task_create_args_t syscall_args = {
    .priority = priority,
    .entry = entry,
    .arg = arg,
    .stack_words = stack_words,
  };

  return (int)os_port_svc_call1(OS_SYSCALL_TASK_CREATE, (uint32_t)(uintptr_t)&syscall_args);
}

OS_KERNEL_TEXT int os_task_create_internal(os_tcb_t *tcb, os_task_priority_t priority, os_task_fn_t entry, void *arg, uint32_t *stack_mem, uint32_t stack_words) {
  uint32_t *sp = stack_mem + stack_words;

  sp = (uint32_t *)((uintptr_t)sp & ~((uintptr_t)0x7)); // align to 8 bytes
  
  // Initialize the task's stack frame

  // 1. Hardware-saved registers (Stacked automatically on exception entry/exit)
  // These are popped by the CPU when the scheduler triggers an exception return
  *(--sp) = 0x01000000u;               // xPSR: Default value with Thumb (T) bit set
  *(--sp) = (uint32_t)(uintptr_t)entry; // PC:   The task's start address
  *(--sp) = (uint32_t)(uintptr_t)os_task_exit_trap; // LR: Return address if task function exits
  *(--sp) = 0u;                         // R12:
  *(--sp) = 0u;                         // R3:
  *(--sp) = 0u;                         // R2:
  *(--sp) = 0u;                         // R1:
  *(--sp) = (uint32_t)(uintptr_t)arg;   // R0:   Task function argument (per AAPCS)

  // 2. Manually-saved registers (Software-managed context)
  // These must be pushed/popped by the Assembly context switcher (PendSV)
  *(--sp) = 0u; // R11: General purpose (often used as Frame Pointer)
  *(--sp) = 0u; // R10: 
  *(--sp) = 0u; // R9:  
  *(--sp) = 0u; // R8:  
  *(--sp) = 0u; // R7:  
  *(--sp) = 0u; // R6:  
  *(--sp) = 0u; // R5:  
  *(--sp) = 0u; // R4:  

  tcb->sp = sp;
  tcb->priority = priority;
  tcb->state = OS_TASK_READY;
  tcb->wait_queue = NULL;
  tcb->wake_tick = 0;
  tcb->wait_result = 0;
  tcb->next = NULL;
  tcb->prev = NULL;
  tcb->timeout_next = NULL;
  tcb->timeout_prev = NULL;
  return 0;
}

OS_KERNEL_TEXT int os_kernel_task_create(const os_task_create_args_t *args) {
  os_tcb_t *tcb;
  uint32_t *stack_mem;
  uint32_t primask;
  int ret;

  if (args == NULL || args->entry == NULL) {
    return -1;
  }

  if ((uint32_t)args->priority > (uint32_t)OS_TASK_IDLE) {
    return -1;
  }

  g_debug_step = 1;
  tcb = (os_tcb_t *)os_kobj_alloc((uint32_t)sizeof(os_tcb_t));
  if (tcb == NULL) {
    return -1;
  }

  g_debug_step = 2;
  g_debug_value0 = (uint32_t)(uintptr_t)tcb;

  stack_mem = os_kstack_alloc(args->stack_words);
  if (stack_mem == NULL) {
    os_kobj_free(tcb);
    return -1;
  }

  g_debug_step = 3;
  g_debug_value1 = (uint32_t)(uintptr_t)stack_mem;


  ret = os_task_create_internal(tcb, args->priority, args->entry, args->arg, stack_mem, args->stack_words);
  if (ret != 0) {
    os_kstack_free(stack_mem);
    os_kobj_free(tcb);
    return ret;
  }

  primask = os_port_irq_save();
  os_ready_queue_add(tcb);
  os_schedule();
  if (g_current != NULL && g_next != g_current) {
    os_port_pendsv_trigger();
  }
  os_port_irq_restore(primask);

  return 0;
}

OS_KERNEL_TEXT void os_start(void) { // start the scheduler, picks first task to run
  os_port_set_pendsv_priority_lowest(); // ensure PendSV is lowest priority
  os_port_mpu_init(); // initialize MPU to set memory permissions for user/kernel regions
  g_first_switch = 1;

  systick_init(25000000u, 1000u); // initialize tick timer to generate interrupt every 1ms, pass CPU frequency and desired tick frequency

  os_schedule();
  g_current = g_next; // first selected task
  if (g_current == NULL) {
    for (;;) {
    }
  }

  __asm volatile("msr psp, %0" :: "r"(g_current->sp) : "memory"); // load PSP with the first task's stack pointer so that when we switch to thread mode, it will use the task's stack
  //systick_enable(); // enable SysTick interrupt
  __asm volatile(
    "movs r0, #2      \n"   // CONTROL = 2 -> use PSP, privileged since we are currently executing os_start which is a kernel function, but we will switch to unprivileged when we perform the first context switch in PendSV handler
    "msr CONTROL, r0  \n" 
    "isb              \n" // flush pipeline to ensure subsequent instructions execute with new CONTROL settings
  );

  os_port_pendsv_trigger(); // trigger first context switch
  
  for (;;) {
  }
}


OS_USER_TEXT void os_yield(void) {
  (void)os_port_svc_call0(OS_SYSCALL_YIELD);
}

OS_KERNEL_TEXT void os_kernel_yield(void) { // cooperative yield: asks kernel to switch away from current task. Picks task then triggers PendSV
  os_ready_queue_rotate(g_current); // move current task to end of its priority's ready queue for round-robin scheduling among same priority tasks
  os_schedule(); // pick next task
  os_port_pendsv_trigger(); // trigger PendSV to perform context switch
}
