/* x86_64 UEFI linker script */
ENTRY(efi_main)
SECTIONS
{
    . = 0x100000;
    .text : {
        *(.text*)
    }
    .reloc : { *(.reloc*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
    /DISCARD/ : { *(.comment) *(.note*) }
}