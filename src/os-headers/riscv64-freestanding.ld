/* RISC-V 64 bare metal linker script */
ENTRY(_start)
SECTIONS
{
    . = 0x80200000;
    .text : {
        *(.text.boot)
        *(.text*)
    }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
    /DISCARD/ : { *(.comment) *(.note*) }
}