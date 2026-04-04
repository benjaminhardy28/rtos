#include "memory.h"

OS_KERNEL_TEXT void *os_memcpy(void *dst, const void *src, uint32_t len) {
    uint8_t *d;
    const uint8_t *s;

    d = (uint8_t *)dst;
    s = (const uint8_t *)src;

    while (len > 0u) {
        *d++ = *s++;
        len--;
    }

    return dst;
}
