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
*/


ENTRY(Reset_Handler)

MEMORY {
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 256K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector :
  {
    KEEP(*(.isr_vector)) /* interrupt vector table  */
  } > FLASH

  .text :
  {
    *(.text*) /* all compiled machine instructions  */
    *(.rodata*) /* read only data, ex: string literals, const variables, jump tables  */
    *(.ARM.exidx*) /* ARM exception unwinding tables  */
    *(.ARM.extab*)
  } > FLASH

  .data : AT (LOADADDR(.text) + SIZEOF(.text))
  {
    _sdata = .;            /* VMA: start address of .data in RAM */
    *(.data*)              /* all input .data* sections from object files */
    _edata = .;            /* VMA: end address of .data in RAM */
  } > RAM
  _sidata = LOADADDR(.data); /* LMA: start address of .data initial values in FLASH */
  
  .bss : /* Zero-initialized global/static variables */
  {
    _sbss = .;
    *(.bss*)
    *(COMMON) /* account for older toolchains   */ 
    _ebss = .;
  } > RAM
}
