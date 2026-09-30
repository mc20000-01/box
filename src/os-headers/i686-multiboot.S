/* i686 Multiboot2 Assembly Stub (32-bit) */
.section .multiboot
.align 8
.global multiboot_header
multiboot_header:
    /* Multiboot2 header */
    .long 0xE85250D6          /* magic */
    .long 0                   /* architecture: i386 (0) */
    .long header_end - multiboot_header  /* header length */
    .long 0x100000000 - (0xE85250D6 + 0 + (header_end - multiboot_header))  /* checksum */

    /* Framebuffer tag (request) */
    .long 8                   /* type: MULTIBOOT_TAG_TYPE_FRAMEBUFFER */
    .long 24                  /* size */
    .long 0                   /* flags */
    .long 0                   /* width */
    .long 0                   /* height */
    .long 32                  /* depth */

    /* End tag */
    .long 0                   /* type: END */
    .long 8                   /* size */

header_end:

.section .text
.global _start
.type _start, @function
_start:
    /* Multiboot entry: %eax = magic, %ebx = addr of multiboot info */
    cli
    
    /* Verify multiboot magic */
    cmpl $0x36d76289, %eax
    jne .panic
    
    /* Save multiboot info pointer */
    movl %ebx, bx_multiboot_info
    
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

.panic:
    /* Print 'P' to serial port */
    movw $0x3F8, %dx
    movb $'P', %al
    outb %al, %dx
    hlt
    jmp .panic

.section .bss
.align 16
_bss_start:
    .space 16384
stack_bottom:
    .space 16384
stack_top:
_bss_end:

.global bx_multiboot_info
.data
.align 4
bx_multiboot_info:
    .long 0