
#include "../include/os/app.h"
#include "../include/os/task.h"
#include "../include/os/tick.h"
#include "../include/os/section.h"
#include <stddef.h>
#include <stdint.h>

// Blinks the Blue Pill's onboard LED (PC13). Unlike app_LED.c's external
// LEDs on GPIOA, PC13 is wired active-low on this board -- driving it low
// turns the LED on, so led_on()/led_off() below are inverted relative to
// a normal push-pull LED.

#define RCC_APB2ENR        (*(volatile uint32_t *)0x40021018u)
#define RCC_APB2ENR_IOPCEN (1u << 4) // GPIOC clock enable

#define GPIOC_CRH  (*(volatile uint32_t *)0x40011004u) // config for pins 8-15
#define GPIOC_BSRR (*(volatile uint32_t *)0x40011010u) // atomic set/reset

#define GPIOC_PIN13 (1u << 13)

// CRH is 4 config bits per pin (CNF:MODE); pin13 -> bits [23:20].
// MODE=10 (output, max 2MHz), CNF=00 (general-purpose push-pull) = 0b0010.
#define GPIOC_CRH_PIN13_MASK (0xFu << 20)
#define GPIOC_CRH_PIN13_OUT  (0x2u << 20)

OS_USER_BSS volatile uint32_t g_blink_count;

static void led_gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC_CRH = (GPIOC_CRH & ~GPIOC_CRH_PIN13_MASK) | GPIOC_CRH_PIN13_OUT;
}

static void led_on(void)
{
    GPIOC_BSRR = GPIOC_PIN13 << 16; // active-low: reset half of BSRR turns it on
}

static void led_off(void)
{
    GPIOC_BSRR = GPIOC_PIN13; // set half of BSRR turns it off
}

OS_USER_TEXT static void task0(void *arg) {
    (void)arg;

    for (;;) {
        led_on();
        os_delay_ticks(os_ticks_from_ms(500));
        led_off();
        os_delay_ticks(os_ticks_from_ms(500));

        g_blink_count++;
    }
}

OS_USER_TEXT void os_app_main(void)
{
    led_gpio_init();

    os_task_create(OS_TASK_MEDIUM, task0, NULL, 256);
}
