#pragma once

#include <stdint.h>
#include "../include/os/section.h"

OS_KERNEL_TEXT void *os_memcpy(void *dst, const void *src, uint32_t len);
