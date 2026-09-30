/* aarch64 bare metal header - minimal freestanding environment for BX */
#ifndef BX_AARCH64_FREESTANDING_H
#define BX_AARCH64_FREESTANDING_H

#include <stdint.h>
#include <stddef.h>

/* UART for early boot (PL011 at 0x09000000 - common for QEMU virt) */
#define UART0_BASE 0x09000000
#define UART_DR    0x00
#define UART_FR    0x18
#define UART_FR_TXFF (1 << 5)

static inline void serial_write(const char *s) {
    volatile uint32_t *uart = (volatile uint32_t*)UART0_BASE;
    while (*s) {
        while (uart[UART_FR/4] & UART_FR_TXFF) {}
        uart[UART_DR/4] = *s++;
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
    /* Disable interrupts */
    __asm__ volatile("msr daifset, #0xf");
}

static inline void bx_stack_init(void) {
    /* Stack set by assembly stub */
}

/* Memory management */
void *bx_mem_alloc(size_t size);
void bx_mem_free(void *ptr);

/* Timer (Generic Timer) */
void bx_timer_init(uint32_t frequency_hz);

/* GIC (Generic Interrupt Controller) */
void bx_gic_init(void);

#endif /* BX_AARCH64_FREESTANDING_H */