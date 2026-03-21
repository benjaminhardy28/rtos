#pragma once
#include "../../kernel/task.h"
#include <stdint.h>

void os_port_set_pendsv_priority_lowest(void); // set PendSV to lowest priority
void os_port_pendsv_trigger(void); // trigger PendSV exception to perform context switch

uint32_t os_port_irq_save(void); // disable interrupts and return previous PRIMASK value
void os_port_irq_restore(uint32_t primask); // restore PRIMASK to re-enable interrupts if they were previously enabled (if primask == 0)
