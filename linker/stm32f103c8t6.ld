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

Sized for the "Blue Pill" STM32F103C8T6: 64K flash / 20K RAM.

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
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
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
