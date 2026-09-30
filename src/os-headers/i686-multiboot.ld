/* i686 Multiboot2 linker script (32-bit) */
ENTRY(_start)
SECTIONS
{
    . = 1M;
    .text : {
        *(.multiboot)
        *(.text.boot)
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
    /DISCARD/ : { *(.comment) *(.note*) }
}