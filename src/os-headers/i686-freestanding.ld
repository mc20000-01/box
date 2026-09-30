/* i686 bare metal linker script (32-bit) */
ENTRY(_start)
SECTIONS
{
    . = 0x100000;
    .text : {
        *(.text.boot)
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
    /DISCARD/ : { *(.comment) *(.note*) }
}