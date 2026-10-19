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
512K flash / 64K RAM), not the smaller "Blue Pill" C8T6 (64K/20K).

Flash is mapped at 0x0 (not the real hardware address 0x08000000) to
match scripts/renode/stm32f103.repl and Renode's own bundled platform
file for this chip -- both rely on the STM32 boot-time alias that maps
flash to address 0 when BOOT0=0. Flashing real silicon would need
FLASH's origin shifted to 0x08000000.
*/


ENTRY(Reset_Handler)

MEMORY {
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 512K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(256);
    KEEP(*(.isr_vector))
    . = ALIGN(256);
  } > FLASH

  .text :
  {
    . = ALIGN(128);
    __text_start__ = .;
    *(.text*)
    *(.rodata*)
    *(.ARM.exidx*)
    *(.ARM.extab*)
    . = ALIGN(128);
    __text_end__ = .;
  } > FLASH

  .data :
  {
    . = ALIGN(128);
    __data_start__ = .;
    *(.data*)
    . = ALIGN(128);
    __data_end__ = .;
  } > RAM AT > FLASH
  __data_load__ = LOADADDR(.data);

  .bss (NOLOAD) :
  {
    . = ALIGN(128);
    __bss_start__ = .;
    *(.bss*)
    *(.stack.user*)
    *(COMMON)
    . = ALIGN(128);
    __bss_end__ = .;
  } > RAM
}
