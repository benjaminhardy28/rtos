#include <stdint.h>

// Brings SYSCLK up from the reset-default 8MHz HSI to 72MHz (HSE + PLL x9).
// Real hardware only -- see clock.c for why this must never run under Renode.
void clock_init(void);
