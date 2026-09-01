#pragma once

#include <stdint.h>

// Default buffer sizes
#ifndef OS_RTT_UP_BUF_SIZE
#define OS_RTT_UP_BUF_SIZE   256u
#endif

#ifndef OS_RTT_DOWN_BUF_SIZE
#define OS_RTT_DOWN_BUF_SIZE 16u
#endif

// Target -> host
void os_rtt_write(const char *data, uint32_t len);
void os_rtt_puts(const char *str); // wrapper for write()

// Host -> target
uint32_t os_rtt_read(char *data, uint32_t len);
