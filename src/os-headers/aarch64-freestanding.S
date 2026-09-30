/* AArch64 Bare Metal Assembly Stub */
.section .text.boot
.global _start
.type _start, %function
_start:
    /* Entry from firmware (e.g., U-Boot, QEMU -kernel)
     * x0 = device tree pointer (FDT)
     * x1 = 0 (reserved)
     * x2 = 0 (reserved)
     * x3 = 0 (reserved)
     */
    
    /* Save FDT pointer */
    str x0, [bx_fdt_ptr]
    
    /* Disable interrupts */
    msr daifset, #0xf
    
    /* Set up stack */
    ldr sp, =stack_top
    
    /* Clear BSS */
    ldr x0, =_bss_start
    ldr x1, =_bss_end
    sub x2, x1, x0
    mov x3, #0
    bl memset
    
    /* Initialize CPU */
    bl bx_cpu_init
    
    /* Call C main */
    bl bx_main
    
    /* Should never return */
1:  wfe
    b 1b

.section .bss
.align 16
_bss_start:
    .space 16384
stack_bottom:
    .space 16384
stack_top:
_bss_end:

.global bx_fdt_ptr
.data
.align 8
bx_fdt_ptr:
    .quad 0