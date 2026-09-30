/* x86_64 Multiboot2 header for BX - GRUB-compatible boot */
#ifndef BX_X86_64_MULTIBOOT_H
#define BX_X86_64_MULTIBOOT_H

#include <stdint.h>
#include <stddef.h>

/* x86_64 specific: serial port COM1 */
static inline void serial_write(const char *s) {
    while (*s) {
        while ((*(volatile uint8_t*)(0x3F8 + 5)) & 0x20) {}
        *(volatile uint8_t*)0x3F8 = *s++;
    }
}

static inline void bx_panic(const char *msg) {
    serial_write("PANIC: ");
    serial_write(msg);
    serial_write("\n");
    for (;;) __builtin_unreachable();
}

#define BX_MULTIBOOT 1

/* Multiboot2 magic and architecture */
#define MULTIBOOT2_MAGIC 0xE85250D6
#define MULTIBOOT_ARCHITECTURE_X86_64 0

/* Multiboot2 header tags */
#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_CMDLINE 1
#define MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME 2
#define MULTIBOOT_TAG_TYPE_MODULE 3
#define MULTIBOOT_TAG_TYPE_BASIC_MEMINFO 4
#define MULTIBOOT_TAG_TYPE_BOOTDEV 5
#define MULTIBOOT_TAG_TYPE_MMAP 6
#define MULTIBOOT_TAG_TYPE_VBE 7
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8
#define MULTIBOOT_TAG_TYPE_ELF_SECTIONS 9
#define MULTIBOOT_TAG_TYPE_APM 10
#define MULTIBOOT_TAG_TYPE_EFI32 11
#define MULTIBOOT_TAG_TYPE_EFI64 12
#define MULTIBOOT_TAG_TYPE_SMBIOS 13
#define MULTIBOOT_TAG_TYPE_ACPI_OLD 14
#define MULTIBOOT_TAG_TYPE_ACPI_NEW 15
#define MULTIBOOT_TAG_TYPE_NETWORK 16
#define MULTIBOOT_TAG_TYPE_EFI_MMAP 17
#define MULTIBOOT_TAG_TYPE_EFI_BS 18
#define MULTIBOOT_TAG_TYPE_EFI32_IH 19
#define MULTIBOOT_TAG_TYPE_EFI64_IH 20
#define MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR 21

#define MULTIBOOT_TAG_ALIGN 8

/* Multiboot2 header structure */
typedef struct {
    uint32_t type;
    uint32_t size;
} multiboot_tag_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t flags;
    uint32_t architecture;
    uint64_t header_addr;
    uint64_t load_addr;
    uint64_t load_end_addr;
    uint64_t bss_end_addr;
    uint64_t entry_addr;
} multiboot_header_tag_t;

/* Framebuffer info from bootloader */
typedef struct {
    uint64_t addr;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint8_t bpp;
    uint8_t type;
    uint8_t reserved;
} multiboot_framebuffer_t;

/* Memory map entry */
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t reserved;
} multiboot_mmap_entry_t;

typedef struct {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    multiboot_mmap_entry_t entries[];
} multiboot_mmap_tag_t;

/* Global multiboot info pointer (set by asm stub) */
extern multiboot_tag_t *bx_multiboot_info;

/* GFX: Extract framebuffer from multiboot info */
static inline int bx_gfx_init_from_multiboot(bx_framebuffer_t *fb) {
    if (!bx_multiboot_info) return -1;
    
    multiboot_tag_t *tag = bx_multiboot_info;
    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
            multiboot_framebuffer_t *fb_tag = (multiboot_framebuffer_t*)tag;
            fb->framebuffer = (uint32_t*)(uintptr_t)fb_tag->addr;
            fb->width = fb_tag->width;
            fb->height = fb_tag->height;
            fb->pitch = fb_tag->pitch;
            fb->bpp = fb_tag->bpp;
            return 0;
        }
        tag = (multiboot_tag_t*)((uintptr_t)tag + ((tag->size + 7) & ~7));
    }
    return -1;
}

/* CPU initialization */
static inline void bx_cpu_init(void) {
    __asm__ volatile("cli");
}

static inline void bx_stack_init(void) {
    /* Stack set by assembly stub */
}

/* Memory management */
void *bx_mem_alloc(size_t size);
void bx_mem_free(void *ptr);

/* Timer */
void bx_timer_init(uint32_t frequency_hz);

/* PCI enumeration for device drivers */
int bx_pci_scan(void);
uint32_t bx_pci_read_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset);
void bx_pci_write_config(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value);

#endif /* BX_X86_64_MULTIBOOT_H */