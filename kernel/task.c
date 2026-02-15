#include "task.h"

extern void os_port_pendsv_trigger(void);

static os_tcb_t *g_current;
static os_tcb_t *g_next;

uint32_t volatile g_first_switch;

static os_tcb_t *g_task0;
static os_tcb_t *g_task1;

static void os_task_exit_trap(void) { // called when a task function returns (this is a backup, since it shouldn't ever return)
  for (;;) {}
}

int os_task_create(os_tcb_t *tcb, os_task_fn_t entry, void *arg, uint32_t *stack_mem, uint32_t stack_words) {
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
  return 0;
}

void os_set_two_tasks(os_tcb_t *t0, os_tcb_t *t1) { // for simple round-robin scheduling between two tasks. Hardcoded for now to 2 tasks
  g_task0 = t0;
  g_task1 = t1;
  g_current = t0;
  g_next = t0;
}

void os_schedule_round_robin(void) { // simple round-robin between two tasks
  g_next = (g_current == g_task0) ? g_task1 : g_task0;
}

void os_start(void) { // start the scheduler, picks first task to run
  g_current = g_task0;
  g_next = g_task0;
  os_port_set_pendsv_priority_lowest(); // ensure PendSV is lowest priority so it only runs when no other exceptions are active
  g_first_switch = 1;

  __asm volatile("msr psp, %0" :: "r"(g_current->sp) : "memory"); // set PSP to first task's stack pointer

  systick_init(25000000u, 1000u); // initialize tick timer to generate interrupt every 1ms
  __asm volatile(
    "movs r0, #2  \n"   // CONTROL = 2 → use PSP, privileged
    "msr CONTROL, r0 \n"
    "isb           \n"
  );

  os_port_pendsv_trigger(); // trigger PendSV to perform first context switch to start first task
  for(;;) {} // should never get here
}

void os_yield(void) { // cooperative yield: asks kernel to switch away from current task. Picks task then triggers PendSV
  os_schedule_round_robin(); // pick next task
  os_port_pendsv_trigger(); // trigger PendSV to perform context switch
}

os_tcb_t *os_current_tcb(void) { return g_current; }
os_tcb_t *os_next_tcb(void) { return g_next; }

void os_commit_switch(void) { g_current = g_next; }
