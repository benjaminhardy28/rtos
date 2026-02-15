#pragma once
#include "../../kernel/task.h"
#include <stdint.h>

void os_port_set_pendsv_priority_lowest(void); // set PendSV to lowest priority
void os_port_pendsv_trigger(void); // trigger PendSV exception to perform context switch
