/* x86_64 Multiboot2 Assembly Stub */
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
    .long 0                   /* flags: prefer graphics mode */
    .long 0                   /* width (0 = any) */
    .long 0                   /* height (0 = any) */
    .long 32                  /* depth: 32bpp */

    /* End tag */
    .long 0                   /* type: END */
    .long 8                   /* size */

header_end:

.section .text
.global _start
.type _start, @function
_start:
    /* Multiboot entry: %rdi = magic, %rsi = addr of multiboot info */
    cli
    
    /* Verify multiboot magic */
    cmp $0x36d76289, %edi     /* EAX = 0x36d76289 (Multiboot2 bootloader magic in EDI) */
    jne .panic
    
    /* Save multiboot info pointer */
    mov %rsi, bx_multiboot_info(%rip)
    
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

.panic:
    /* Print 'P' to serial port */
    mov $0x3F8, %dx
    mov $'P', %al
    out %al, %dx
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
.align 8
bx_multiboot_info:
    .quad 0