/* x86_64 UEFI header for BX */
#ifndef BX_X86_64_UEFI_H
#define BX_X86_64_UEFI_H

#include <stdint.h>
#include <stddef.h>

#define BX_UEFI 1

/* UEFI types */
typedef uint64_t efi_status_t;
typedef uint64_t efi_handle_t;
typedef uint64_t efi_event_t;
typedef uint64_t efi_lba_t;
typedef uint64_t efi_tpl_t;
typedef uint64_t efi_physical_address_t;
typedef uint64_t efi_virtual_address_t;

#define EFI_SUCCESS 0
#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
    {0x5B1B31A1, 0x9562, 0x11D2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
    {0x9042A9DE, 0x23DC, 0x4A38, {0x96, 0xFB, 0x7A, 0xDE, 0xD0, 0x80, 0x51, 0x6A}}
#define EFI_SIMPLE_NETWORK_PROTOCOL_GUID \
    {0xA19832B9, 0xAC25, 0x11D3, {0x9A, 0x2D, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D}}

/* UEFI system table forward declarations */
typedef struct _EFI_SYSTEM_TABLE EFI_SYSTEM_TABLE;
typedef struct _EFI_BOOT_SERVICES EFI_BOOT_SERVICES;
typedef struct _EFI_RUNTIME_SERVICES EFI_RUNTIME_SERVICES;
typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;
typedef struct _EFI_SIMPLE_NETWORK_PROTOCOL EFI_SIMPLE_NETWORK_PROTOCOL;

extern EFI_SYSTEM_TABLE *gST;
extern EFI_BOOT_SERVICES *gBS;
extern EFI_RUNTIME_SERVICES *gRT;
extern efi_handle_t gImageHandle;

/* UEFI entry point */
efi_status_t efi_main(efi_handle_t image_handle, EFI_SYSTEM_TABLE *system_table);

/* Console output */
static inline void serial_write(const char *s) {
    if (gST && gST->ConOut) {
        uint16_t ucs2[256];
        size_t len = strlen(s);
        for (size_t i = 0; i < len && i < 255; i++) ucs2[i] = s[i];
        ucs2[len] = 0;
        gST->ConOut->OutputString(gST->ConOut, ucs2);
    }
}

static inline void bx_panic(const char *msg) {
    serial_write("PANIC: ");
    serial_write(msg);
    serial_write("\n");
    for (;;) __builtin_unreachable();
}

/* GFX from GOP */
static inline int bx_gfx_init_from_gop(bx_framebuffer_t *fb) {
    if (!gBS) return -1;
    
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    efi_status_t status = gBS->LocateProtocol(&EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID, NULL, (void**)&gop);
    if (status != EFI_SUCCESS || !gop) return -1;
    
    fb->framebuffer = (uint32_t*)(uintptr_t)gop->Mode->FrameBufferBase;
    fb->width = gop->Mode->Info->HorizontalResolution;
    fb->height = gop->Mode->Info->VerticalResolution;
    fb->pitch = gop->Mode->Info->PixelsPerScanLine * 4;
    fb->bpp = 32;
    return 0;
}

/* Simple Network Protocol for WiFi */
static inline int bx_wifi_uefi_init(void) {
    if (!gBS) return -1;
    EFI_SIMPLE_NETWORK_PROTOCOL *snp = NULL;
    efi_status_t status = gBS->LocateProtocol(&EFI_SIMPLE_NETWORK_PROTOCOL_GUID, NULL, (void**)&snp);
    if (status != EFI_SUCCESS || !snp) return -1;
    g_bx_wifi.hw_priv = snp;
    return 0;
}

static inline void bx_cpu_init(void) {
    /* UEFI handles CPU setup */
}

static inline void bx_stack_init(void) {
    /* UEFI provides stack */
}

void *bx_mem_alloc(size_t size);
void bx_mem_free(void *ptr);
void bx_timer_init(uint32_t frequency_hz);

#endif /* BX_X86_64_UEFI_H */