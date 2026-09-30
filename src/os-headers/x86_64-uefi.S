/* x86_64 UEFI assembly stub */
.section .text.efi_main, "ax"
.global efi_main
.align 16

efi_main:
    /* UEFI calling convention: RCX = ImageHandle, RDX = SystemTable */
    /* We need to save these and set up our own stack */
    pushq %rbp
    movq %rsp, %rbp
    
    /* Save UEFI parameters */
    movq %rcx, gImageHandle(%rip)
    movq %rdx, gST(%rip)
    
    /* Get BootServices from SystemTable */
    movq 0x60(%rdx), %rax  /* BootServices offset in EFI_SYSTEM_TABLE */
    movq %rax, gBS(%rip)
    
    /* Call our C main */
    call bx_main
    
    /* Return EFI_SUCCESS */
    xorl %eax, %eax
    popq %rbp
    ret

.data
.global gImageHandle
.global gST
.global gBS
gImageHandle: .quad 0
gST: .quad 0
gBS: .quad 0