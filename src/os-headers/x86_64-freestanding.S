/* x86_64 Bare Metal Assembly Stub (non-multiboot, raw binary) */
.section .text.boot
.global _start
.type _start, @function
_start:
    /* Raw binary entry - no bootloader parameters */
    cli
    
    /* Set up stack */
    mov $stack_top, %rsp
    
    /* Clear BSS */
    mov $_bss_start, %rdi
    mov $_bss_end, %rsi
    sub %rdi, %rsi
    xor %rax, %rax
    rep stosb
    
    /* Initialize CPU */
    call bx_cpu_init
    
    /* Call C main */
    call bx_main
    
    /* Should never return */
    hlt
    jmp 1b

.section .bss
.align 16
_bss_start:
    .space 16384
stack_bottom:
    .space 16384
stack_top:
_bss_end: