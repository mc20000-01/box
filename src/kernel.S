/* BoxedLANG OS - 32-bit Multiboot Kernel Entry
 * Booted by QEMU/GRUB via Multiboot 1: entered in 32-bit protected
 * mode with flat segments, paging disabled.  Runs entirely in 32-bit
 * (the whole OS demo only touches ports, VGA 0xA0000 and low memory). */
.global _start
.global main

/* Multiboot1 header (must be within the first 8KB of the image) */
.section .text
.align 4
    .long 0x1BADB002            /* magic */
    .long 0x00000003            /* flags: page-align + memory info */
    .long -(0x1BADB002 + 0x00000003)  /* checksum */

/* Stack lives in RAM right after .bss (across __bss_end), so clear_bss
 * never wipes it.  stack_top/stack_bottom are provided by kernel.ld. */
.section .text
.code32
_start:
    /* Enable SSE/MMX (BIOS leaves them disabled: CR0.EM set, CR4.OSFXSR clear) */
    movl %cr0, %eax
    andl $~0x4, %eax          /* clear CR0.EM */
    orl $0x2, %eax            /* set CR0.MP */
    movl %eax, %cr0
    movl %cr4, %eax
    orl $0x600, %eax          /* set CR4.OSFXSR | CR4.OSXMMEXCPT */
    movl %eax, %cr4

    /* Set up stack */
    movl $stack_top, %esp

    /* Clear BSS */
    movl $__bss_start, %edi
    movl $__bss_end, %esi
    call clear_bss

    /* Call kernel main */
    call main

    /* Halt if main returns */
1:  hlt
    jmp 1b

clear_bss:
    cmpl %esi, %edi
    jae 1f
    movb $0, (%edi)
    incl %edi
    jmp clear_bss
1:  ret

/* VGA I/O - defined in C */
.global outb, inb, io_wait
outb:
    outb %al, %dx
    ret
inb:
    inb %dx, %al
    ret
io_wait:
    jmp 1f
1:  jmp 2f
2:  ret