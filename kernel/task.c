#include "task.h"
#include "scheduler.h"

extern void os_port_pendsv_trigger(void);

uint32_t volatile g_first_switch;

os_tcb_t *g_current = NULL;
os_tcb_t *g_next = NULL;

static os_tcb_t *g_ready_head;

static void os_task_exit_trap(void) { // called when a task function returns (this is a backup, since it shouldn't ever return)
  for (;;) {}
}

int os_task_create(os_tcb_t *tcb, os_task_priority_t priority, os_task_fn_t entry, void *arg, uint32_t *stack_mem, uint32_t stack_words) {
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
  return 0;
}

void os_start(void) { // start the scheduler, picks first task to run
  os_port_set_pendsv_priority_lowest(); // ensure PendSV is lowest priority
  g_first_switch = 1;

  systick_init(25000000u, 1000u); // initialize tick timer to generate interrupt every 1ms

  __asm volatile(
    "movs r0, #2      \n"   // CONTROL = 2 -> use PSP, privileged
    "msr CONTROL, r0  \n"
    "isb              \n"
  );

  os_schedule();
  g_current = g_next; // first selected task

  if (g_current == NULL) {
    for (;;) {
    }
  }

  __asm volatile("msr psp, %0" :: "r"(g_current->sp) : "memory"); // now safe

  os_port_pendsv_trigger(); // trigger first context switch
  for (;;) {
  }
}


void os_yield(void) { // cooperative yield: asks kernel to switch away from current task. Picks task then triggers PendSV
  os_ready_queue_rotate(g_current); // move current task to end of its priority's ready queue for round-robin scheduling among same priority tasks
  os_schedule(); // pick next task
  os_port_pendsv_trigger(); // trigger PendSV to perform context switch
}

