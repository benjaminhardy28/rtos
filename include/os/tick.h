#pragma once

#include <stdint.h>

uint32_t os_tick_get(void);
uint32_t os_ticks_from_ms(uint32_t ms);
void os_delay_ticks(uint32_t ticks);
