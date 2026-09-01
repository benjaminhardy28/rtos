#include "../include/os/rtt.h"
#include "../include/os/section.h"
#include "../arch/arm_cm/port.h"
#include <stdint.h>

// From-scratch reimplementation of SEGGER's RTT control block
// OpenOCD polls SRAM for the 16-byte "SEGGER RTT" id below, then parses the data below and reads from buffer

#define OS_RTT_MODE_NO_BLOCK_SKIP 1u

typedef struct {
    const char *name;
    char       *buffer;
    uint32_t    size;
    uint32_t    wr_off; // written by the target (us)
    uint32_t    rd_off; // written by the host (OpenOCD)
    uint32_t    flags;
} os_rtt_channel_t;

typedef struct {
    char             id[16];
    int32_t          max_up;
    int32_t          max_down;
    os_rtt_channel_t up[1];
    os_rtt_channel_t down[1];
} os_rtt_cb_t;

// Both channels live in user RAM/flash which is not kernel protected by the MPU
OS_USER_BSS static char g_rtt_up_buf[OS_RTT_UP_BUF_SIZE];
OS_USER_BSS static char g_rtt_down_buf[OS_RTT_DOWN_BUF_SIZE];

// "used" forces the compiler to include this struct, since it is not used
OS_USER_DATA volatile os_rtt_cb_t g_rtt_cb __attribute__((used)) = {
    .id       = "SEGGER RTT",
    .max_up   = 1,
    .max_down = 1,
    .up   = {{ .name = "Terminal", .buffer = g_rtt_up_buf,   .size = OS_RTT_UP_BUF_SIZE,
               .wr_off = 0u, .rd_off = 0u, .flags = OS_RTT_MODE_NO_BLOCK_SKIP }},
    .down = {{ .name = "Terminal", .buffer = g_rtt_down_buf, .size = OS_RTT_DOWN_BUF_SIZE,
               .wr_off = 0u, .rd_off = 0u, .flags = OS_RTT_MODE_NO_BLOCK_SKIP }},
};

OS_USER_TEXT void os_rtt_write(const char *data, uint32_t len) {
    volatile os_rtt_channel_t *ch = &g_rtt_cb.up[0];
    uint32_t primask;
    uint32_t i;
    uint32_t next;

    // primask guards the wr_off against concurrent os_rtt_write() calls from other tasks/ISRs
    primask = os_port_irq_save();
    for (i = 0; i < len; i++) {
        next = ch->wr_off + 1u;
        if (next == ch->size) {
            next = 0u;
        }
        if (next == ch->rd_off) {
            break;
        }
        ch->buffer[ch->wr_off] = data[i];
        ch->wr_off = next;
    }
    os_port_irq_restore(primask);
}

OS_USER_TEXT void os_rtt_puts(const char *str) {
    uint32_t len = 0u;

    while (str[len] != '\0') {
        len++;
    }
    os_rtt_write(str, len);
}

OS_USER_TEXT uint32_t os_rtt_read(char *data, uint32_t len) {
    volatile os_rtt_channel_t *ch = &g_rtt_cb.down[0];
    uint32_t primask;
    uint32_t count = 0u;

    primask = os_port_irq_save();
    while (count < len && ch->rd_off != ch->wr_off) {
        data[count] = ch->buffer[ch->rd_off];
        ch->rd_off = (ch->rd_off + 1u == ch->size) ? 0u : (ch->rd_off + 1u);
        count++;
    }
    os_port_irq_restore(primask);

    return count;
}
