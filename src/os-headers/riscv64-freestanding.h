/* RISC-V 64-bit bare metal header for BX */
#ifndef BX_RISCV64_FREESTANDING_H
#define BX_RISCV64_FREESTANDING_H

#include <stdint.h>
#include <stddef.h>

/* UART (SiFive / QEMU virt at 0x10000000) */
#define UART0_BASE 0x10000000
#define UART_DR    0x00

static inline void serial_write(const char *s) {
    volatile uint32_t *uart = (volatile uint32_t*)UART0_BASE;
    while (*s) {
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
    __asm__ volatile("csrci mstatus, 0x8");  /* Clear MIE */
}

static inline void bx_stack_init(void) {
    /* Stack set by assembly stub */
}

/* Memory management */
void *bx_mem_alloc(size_t size);
void bx_mem_free(void *ptr);

/* Timer (CLINT/MTIME) */
void bx_timer_init(uint32_t frequency_hz);

/* PLIC (Platform Level Interrupt Controller) */
void bx_plic_init(void);

#endif /* BX_RISCV64_FREESTANDING_H */