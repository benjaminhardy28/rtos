#pragma once

#define OS_USER_TEXT __attribute__((section(".text.user")))
#define OS_USER_RODATA __attribute__((section(".rodata.user")))
#define OS_USER_DATA __attribute__((section(".data.user")))
#define OS_USER_BSS __attribute__((section(".bss.user")))
#define OS_USER_STACK __attribute__((section(".stack.user"), aligned(8)))

#define OS_KERNEL_TEXT __attribute__((section(".text.kernel")))
#define OS_KERNEL_RODATA __attribute__((section(".rodata.kernel")))
#define OS_KERNEL_DATA __attribute__((section(".data.kernel")))
#define OS_KERNEL_BSS __attribute__((section(".bss.kernel")))
