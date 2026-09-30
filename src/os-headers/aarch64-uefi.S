/* aarch64 UEFI assembly stub */
.section .text.efi_main, "ax"
.global efi_main
.align 4

efi_main:
    /* UEFI calling convention: X0 = ImageHandle, X1 = SystemTable */
    stp x29, x30, [sp, -16]!
    mov x29, sp
    
    /* Save UEFI parameters */
    str x0, gImageHandle
    str x1, gST
    
    /* Get BootServices from SystemTable (offset 0x60 in EFI_SYSTEM_TABLE) */
    ldr x0, [x1, #0x60]
    str x0, gBS
    
    /* Call our C main */
    bl bx_main
    
    /* Return EFI_SUCCESS */
    mov w0, #0
    ldp x29, x30, [sp], 16
    ret

.data
.global gImageHandle
.global gST
.global gBS
gImageHandle: .quad 0
gST: .quad 0
gBS: .quad 0