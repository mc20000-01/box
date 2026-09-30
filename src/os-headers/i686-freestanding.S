/* i686 Bare Metal Assembly Stub (32-bit, raw binary) */
.section .text.boot
.global _start
.type _start, @function
_start:
    /* Raw binary entry */
    cli
    
    /* Set up stack */
    movl $stack_top, %esp
    
    /* Clear BSS */
    movl $_bss_start, %edi
    movl $_bss_end, %esi
    subl %edi, %esi
    xorl %eax, %eax
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