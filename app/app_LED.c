
#include "../include/os/app.h"
#include "../include/os/mutex.h"
#include "../include/os/task.h"
#include "../include/os/tick.h"
#include "../include/os/section.h"
#include <stddef.h>
#include <stdint.h>

// Two LEDs, one mutex: task0 (PA9) and task1 (PA12) both have to take
// g_led_mutex before they're allowed to light their LED, so only one LED
// is ever lit at a time -- the mutex is what makes them "compete".

#define RCC_APB2ENR       (*(volatile uint32_t *)0x40021018u)
#define RCC_APB2ENR_IOPAEN (1u << 2) // GPIOA clock enable

#define GPIOA_CRH  (*(volatile uint32_t *)0x40010804u) // config for pins 8-15
#define GPIOA_BSRR (*(volatile uint32_t *)0x40010810u) // atomic set/reset

#define GPIOA_PIN9  (1u << 9)
#define GPIOA_PIN12 (1u << 12)

// CRH is 4 config bits per pin (CNF:MODE); pin9 -> bits [7:4], pin12 -> bits [19:16].
// MODE=10 (output, max 2MHz), CNF=00 (general-purpose push-pull) = 0b0010.
#define GPIOA_CRH_PIN9_MASK  (0xFu << 4)
#define GPIOA_CRH_PIN9_OUT   (0x2u << 4)
#define GPIOA_CRH_PIN12_MASK (0xFu << 16)
#define GPIOA_CRH_PIN12_OUT  (0x2u << 16)

OS_USER_BSS static os_mutex_t g_led_mutex;

OS_USER_BSS volatile uint32_t g_task0_blinks;
OS_USER_BSS volatile uint32_t g_task1_blinks;

static void led_gpio_init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;

    GPIOA_CRH = (GPIOA_CRH & ~GPIOA_CRH_PIN9_MASK) | GPIOA_CRH_PIN9_OUT;
    GPIOA_CRH = (GPIOA_CRH & ~GPIOA_CRH_PIN12_MASK) | GPIOA_CRH_PIN12_OUT;
}

static void led_on(uint32_t pin)
{
    GPIOA_BSRR = pin;
}

static void led_off(uint32_t pin)
{
    GPIOA_BSRR = pin << 16; // upper half of BSRR resets the pin
}

OS_USER_TEXT static void task0(void *arg) {
    (void)arg;

    for (;;) {
        os_mutex_lock(&g_led_mutex);
        led_on(GPIOA_PIN9);
        os_delay_ticks(200);
        led_off(GPIOA_PIN9);
        os_mutex_unlock(&g_led_mutex);

        g_task0_blinks++;
        os_delay_ticks(200);
    }
}

OS_USER_TEXT static void task1(void *arg) {
    (void)arg;

    for (;;) {
        os_mutex_lock(&g_led_mutex);
        led_on(GPIOA_PIN12);
        os_delay_ticks(200);
        led_off(GPIOA_PIN12);
        os_mutex_unlock(&g_led_mutex);

        g_task1_blinks++;
        os_delay_ticks(200);
    }
}

OS_USER_TEXT void os_app_main(void)
{
    led_gpio_init();
    os_mutex_init(&g_led_mutex);

    os_task_create(OS_TASK_MEDIUM, task0, (void *)0, 256);
    os_task_create(OS_TASK_MEDIUM, task1, (void *)1, 256);
}
