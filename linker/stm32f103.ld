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

Sized for the STM32F103 "high-density" line (e.g. STM32F103VE/ZE:
512K flash / 64K RAM), not the smaller "Blue Pill" C8T6 (64K/20K) --
20K RAM doesn't split into two power-of-two halves, which the MPU
region setup in arch/arm_cm/port.c requires. If you're targeting a
smaller variant, give RAM_USER/RAM_KERNEL unequal power-of-two sizes
that sum to your chip's real RAM instead of an even split.

Flash is mapped at 0x0 (not the real hardware address 0x08000000) to
match scripts/renode/stm32f103.repl and Renode's own bundled platform
file for this chip -- both rely on the STM32 boot-time alias that maps
flash to address 0 when BOOT0=0. Flashing real silicon would need
FLASH_USER/FLASH_KERNEL origins shifted to 0x08000000.
*/


ENTRY(Reset_Handler)

MEMORY {
  FLASH_USER   (rx)  : ORIGIN = 0x00000000, LENGTH = 256K
  FLASH_KERNEL (rx)  : ORIGIN = 0x00040000, LENGTH = 256K
  RAM_USER     (rwx) : ORIGIN = 0x20000000, LENGTH = 32K
  RAM_KERNEL   (rwx) : ORIGIN = 0x20008000, LENGTH = 32K
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
