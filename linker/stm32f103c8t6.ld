/*
FLASH (image / storage)
  ├── vector table
  ├── code
  ├── constants
  └── initial .data values

RAM (runtime)
  ├── .data (copied at boot)
  ├── .bss (zeroed)
  └── stacks / heaps

Sized for the "Blue Pill" STM32F103C8T6: 64K flash / 20K RAM. 20K
doesn't split into two equal power-of-two halves (needed by the MPU
region setup in arch/arm_cm/port.c), so the split is uneven: 16K user /
4K kernel is the only power-of-two pair that sums to 20K. Measured
actual usage (see build/rtos.elf with the default app) is ~8.5K user
(mostly OS_TASK_STACK_COUNT * OS_TASK_STACK_WORDS task stacks, see
kernel/kalloc.h) and ~2.1K kernel (kernel object pool + TCBs), so both
halves have headroom.

Flash is mapped at its real hardware address 0x08000000 here (unlike
linker/stm32f103.ld's high-density variant, which links at 0x0 to match
Renode's boot-alias simplification). scripts/renode/stm32f103c8t6.repl
mirrors this same 0x08000000 base, so the emulator and real silicon see
identical addresses -- no separate hardware-only linker script needed,
and flashing tools (OpenOCD/st-flash) get the address they expect
without relying on the BOOT0-driven 0x0 alias.
*/


ENTRY(Reset_Handler)

MEMORY {
  FLASH_USER   (rx)  : ORIGIN = 0x08000000, LENGTH = 32K
  FLASH_KERNEL (rx)  : ORIGIN = 0x08008000, LENGTH = 32K
  RAM_USER     (rwx) : ORIGIN = 0x20000000, LENGTH = 16K
  RAM_KERNEL   (rwx) : ORIGIN = 0x20004000, LENGTH = 4K
}

__user_flash_region_start__ = ORIGIN(FLASH_USER);
__user_flash_region_end__ = ORIGIN(FLASH_USER) + LENGTH(FLASH_USER);
__kernel_flash_region_start__ = ORIGIN(FLASH_KERNEL);
__kernel_flash_region_end__ = ORIGIN(FLASH_KERNEL) + LENGTH(FLASH_KERNEL);
__user_ram_region_start__ = ORIGIN(RAM_USER);
__user_ram_region_end__ = ORIGIN(RAM_USER) + LENGTH(RAM_USER);
__kernel_ram_region_start__ = ORIGIN(RAM_KERNEL);
__kernel_ram_region_end__ = ORIGIN(RAM_KERNEL) + LENGTH(RAM_KERNEL);

_estack = ORIGIN(RAM_KERNEL) + LENGTH(RAM_KERNEL);

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(256);
    KEEP(*(.isr_vector))
    . = ALIGN(256);
  } > FLASH_USER

  .text.user :
  {
    . = ALIGN(128);
    __user_text_start__ = .;
    *(.text.user*)
    *(.rodata.user*)
    . = ALIGN(128);
    __user_text_end__ = .;
  } > FLASH_USER

  .text.kernel :
  {
    . = ALIGN(128);
    __kernel_text_start__ = .;
    *(.text.kernel*)
    *(.rodata.kernel*)
    *(.text*)
    *(.rodata*)
    *(.ARM.exidx*)
    *(.ARM.extab*)
    . = ALIGN(128);
    __kernel_text_end__ = .;
  } > FLASH_KERNEL

  .data.user :
  {
    . = ALIGN(128);
    __user_data_start__ = .;
    *(.data.user*)
    . = ALIGN(128);
    __user_data_end__ = .;
  } > RAM_USER AT > FLASH_USER
  __user_data_load__ = LOADADDR(.data.user);

  .bss.user (NOLOAD) :
  {
    . = ALIGN(128);
    __user_bss_start__ = .;
    *(.bss.user*)
    . = ALIGN(128);
    __user_bss_end__ = .;
  } > RAM_USER

  .stack.user (NOLOAD) :
  {
    . = ALIGN(128);
    __user_stacks_start__ = .;
    *(.stack.user*)
    . = ALIGN(128);
    __user_stacks_end__ = .;
  } > RAM_USER

  .data.kernel :
  {
    . = ALIGN(128);
    __kernel_data_start__ = .;
    *(.data.kernel*)
    *(.data*)
    . = ALIGN(128);
    __kernel_data_end__ = .;
  } > RAM_KERNEL AT > FLASH_KERNEL
  __kernel_data_load__ = LOADADDR(.data.kernel);

  .bss.kernel (NOLOAD) :
  {
    . = ALIGN(128);
    __kernel_bss_start__ = .;
    *(.bss.kernel*)
    *(.bss*)
    *(COMMON)
    . = ALIGN(128);
    __kernel_bss_end__ = .;
  } > RAM_KERNEL
}
