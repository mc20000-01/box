/* BoxedLANG OS Kernel Linker Script (32-bit, Multiboot 1) */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    . = 0x100000;  /* Load at 1MB (standard kernel load address) */
    
    .text :
    {
        *(.text)
        *(.text.*)
    }
    
    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }
    
    .data :
    {
        *(.data)
        *(.data.*)
    }
    
    .bss :
    {
        __bss_start = .;
        *(.bss)
        *(.bss.*)
        *(COMMON)
        __bss_end = .;
    }
    
    /* kernel stack lives right after .bss so clear_bss never wipes it */
    PROVIDE (stack_bottom = __bss_end);
    PROVIDE (stack_top = __bss_end + 16384);
    
    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
    }
}
