/* i686 bare metal header (32-bit) - minimal freestanding environment for BX */
#ifndef BX_I686_FREESTANDING_H
#define BX_I686_FREESTANDING_H

#include <stdint.h>
#include <stddef.h>

/* i686 specific: serial port COM1 */
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

/* Timer/PIT */
void bx_timer_init(uint32_t frequency_hz);

#endif /* BX_I686_FREESTANDING_H */