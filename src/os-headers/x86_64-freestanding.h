/* x86_64 bare metal header - minimal freestanding environment for BX */
#ifndef BX_X86_64_FREESTANDING_H
#define BX_X86_64_FREESTANDING_H

#include <stdint.h>
#include <stddef.h>

/* x86_64 specific: serial port COM1 for early boot output */
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

/* Multiboot2 info pointer (set by asm stub if multiboot) */
extern void *bx_multiboot_info;

/* GDT/IDT setup for bare metal */
static inline void bx_cpu_init(void) {
    /* Disable interrupts */
    __asm__ volatile("cli");
    
    /* Load GDT - placeholder for real implementation */
    /* Load IDT - placeholder for real implementation */
}

/* Stack setup - called from assembly stub */
static inline void bx_stack_init(void) {
    /* Stack is already set up by assembly stub */
}

/* Memory management stubs */
void *bx_mem_alloc(size_t size);
void bx_mem_free(void *ptr);

/* Timer/PIT initialization */
void bx_timer_init(uint32_t frequency_hz);

#endif /* BX_X86_64_FREESTANDING_H */