#include <stdint.h>

void systick_init(uint32_t cpu_hz, uint32_t tick_hz);
void systick_enable();