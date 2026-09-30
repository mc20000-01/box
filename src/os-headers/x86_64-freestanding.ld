/* x86_64 bare metal linker script */
ENTRY(_start)
SECTIONS
{
    . = 0x100000;
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