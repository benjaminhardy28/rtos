#pragma once

#include "../include/os/lfqueue.h"
#include <stddef.h>
#include "../include/os/section.h"

OS_USER_TEXT static uint32_t lfqueue_next_index(const os_spsc_queue_t *queue, uint32_t index);
