#pragma once

#include <stdint.h>
#include "task.h"
#include "scheduler.h"
#include "../arch/arm_cm/port.h"
#include "../include/os/tick.h"

void os_kernel_delay_ticks(uint32_t ticks);
