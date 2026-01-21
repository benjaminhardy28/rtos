#include <stdint.h>

volatile uint32_t g_data = 0x11223344;
volatile uint32_t g_bss;

volatile uint32_t g_reached_main;

int main(void)
{
  g_reached_main = 0xA5A5A5A5; // Value can be checked to indicate that we reached main from boot

  while (1) {}
}

