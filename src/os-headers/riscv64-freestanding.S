/* RISC-V 64 Bare Metal Assembly Stub */
.section .text.boot
.global _start
.type _start, @function
_start:
    /* Entry from OpenSBI/firmware
     * a0 = hartid
     * a1 = device tree pointer (FDT)
     */
    
    /* Save FDT pointer */
    sd a1, bx_fdt_ptr
    
    /* Disable interrupts */
    csrci mstatus, 0x8    /* Clear MIE */
    
    /* Set up stack */
    la sp, stack_top
    
    /* Clear BSS */
    la t0, _bss_start
    la t1, _bss_end
    sub t2, t1, t0
    li t3, 0
    call memset
    
    /* Initialize CPU */
    call bx_cpu_init
    
    /* Call C main */
    call bx_main
    
    /* Should never return */
1:  wfi
    j 1b

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
    .dword 0