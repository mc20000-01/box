/* AVR (Arduino) freestanding headers for BoxedLANG bare-metal compilation */
#ifndef _AVR_FREESTANDING_H
#define _AVR_FREESTANDING_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* AVR specific types */
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;

/* AVR I/O */
#define DDRB  (*(volatile uint8_t*)0x24)
#define PORTB (*(volatile uint8_t*)0x25)
#define PINB  (*(volatile uint8_t*)0x23)

#define DDRC  (*(volatile uint8_t*)0x27)
#define PORTC (*(volatile uint8_t*)0x28)
#define PINC  (*(volatile uint8_t*)0x26)

#define DDRD  (*(volatile uint8_t*)0x2A)
#define PORTD (*(volatile uint8_t*)0x2B)
#define PIND  (*(volatile uint8_t*)0x29)

/* Timer registers (ATmega328P) */
#define TCCR0A  (*(volatile uint8_t*)0x44)
#define TCCR0B  (*(volatile uint8_t*)0x45)
#define TCNT0   (*(volatile uint8_t*)0x46)
#define OCR0A   (*(volatile uint8_t*)0x47)
#define OCR0B   (*(volatile uint8_t*)0x48)
#define TIMSK0  (*(volatile uint8_t*)0x6E)
#define TIFR0   (*(volatile uint8_t*)0x15)

#define TCCR1A  (*(volatile uint8_t*)0x80)
#define TCCR1B  (*(volatile uint8_t*)0x81)
#define TCCR1C  (*(volatile uint8_t*)0x82)
#define TCNT1   (*(volatile uint16_t*)0x84)
#define OCR1A   (*(volatile uint16_t*)0x88)
#define OCR1B   (*(volatile uint16_t*)0x8A)
#define ICR1    (*(volatile uint16_t*)0x86)
#define TIMSK1  (*(volatile uint8_t*)0x6F)
#define TIFR1   (*(volatile uint8_t*)0x16)

#define TCCR2A  (*(volatile uint8_t*)0xB0)
#define TCCR2B  (*(volatile uint8_t*)0xB1)
#define TCNT2   (*(volatile uint8_t*)0xB2)
#define OCR2A   (*(volatile uint8_t*)0xB3)
#define OCR2B   (*(volatile uint8_t*)0xB4)
#define TIMSK2  (*(volatile uint8_t*)0x70)
#define TIFR2   (*(volatile uint8_t*)0x17)

/* UART (USART0) */
#define UDR0    (*(volatile uint8_t*)0xC6)
#define UCSR0A  (*(volatile uint8_t*)0xC0)
#define UCSR0B  (*(volatile uint8_t*)0xC1)
#define UCSR0C  (*(volatile uint8_t*)0xC2)
#define UBRR0   (*(volatile uint16_t*)0xC4)

/* ADC */
#define ADMUX   (*(volatile uint8_t*)0x7C)
#define ADCSRA  (*(volatile uint8_t*)0x7A)
#define ADCSRB  (*(volatile uint8_t*)0x7B)
#define ADCL    (*(volatile uint8_t*)0x78)
#define ADCH    (*(volatile uint8_t*)0x79)

/* Interrupts */
#define sei()  __asm__ __volatile__ ("sei" ::: "memory")
#define cli()  __asm__ __volatile__ ("cli" ::: "memory")

/* Sleep */
#define sleep_mode() __asm__ __volatile__ ("sleep" ::: "memory")

/* Delay functions (using timer) */
static inline void _delay_us(uint16_t us) {
    for (uint16_t i = 0; i < us; i++) {
        __asm__ __volatile__ ("nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t");
    }
}

static inline void _delay_ms(uint16_t ms) {
    for (uint16_t i = 0; i < ms; i++) {
        _delay_us(1000);
    }
}

/* String functions */
static inline void *memcpy(void *dest, const void *src, size_t n) {
    uint8_t *d = (uint8_t*)dest;
    const uint8_t *s = (const uint8_t*)src;
    while (n--) *d++ = *s++;
    return dest;
}

static inline void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t*)s;
    while (n--) *p++ = (uint8_t)c;
    return s;
}

static inline int strcmp(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return (unsigned char)*a - (unsigned char)*b;
}

static inline size_t strlen(const char *s) {
    size_t len = 0;
    while (s[len]) len++;
    return len;
}

/* Print to UART */
static inline void uart_putc(char c) {
    while (!(UCSR0A & (1<<5)));
    UDR0 = c;
}

static inline void uart_puts(const char *s) {
    while (*s) uart_putc(*s++);
}

/* Setup UART at 9600 baud (16MHz clock) */
static inline void uart_init(uint32_t baud) {
    uint16_t ubrr = (F_CPU / 16 / baud) - 1;
    UBRR0 = ubrr;
    UCSR0B = (1<<3) | (1<<4);  // TXEN0 | RXEN0
    UCSR0C = (1<<1) | (1<<2);  // 8 data bits
}

/* F_CPU definition (16MHz for Arduino Uno) */
#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#endif /* _AVR_FREESTANDING_H */
