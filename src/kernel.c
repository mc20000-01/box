/* BoxedLANG OS Kernel - Graphics Demo */
#include "os-headers/x86_64-graphics.h"

int cursor_x = 0, cursor_y = 0;
uint8_t current_fg = 0x0F, current_bg = 0x00;

/* 16550 UART on the ISA serial port (COM1) for headless verification */
static void serial_init(void) {
    outb(0x3F8 + 1, 0x00);   /* disable interrupts */
    outb(0x3F8 + 3, 0x80);   /* enable DLAB */
    outb(0x3F8 + 0, 0x01);   /* divisor low  (115200) */
    outb(0x3F8 + 1, 0x00);   /* divisor high */
    outb(0x3F8 + 3, 0x03);   /* 8N1 */
    outb(0x3F8 + 2, 0xC7);   /* FIFO enable/clear */
    outb(0x3F8 + 4, 0x0B);   /* IRQs enabled, RTS/DSR set */
}
static void serial_putc(char c) {
    while (!(inb(0x3F8 + 5) & 0x20)) io_wait();
    outb(0x3F8, (unsigned char)c);
}
static void serial_puts(const char *s) { while (*s) serial_putc(*s++); }

void kernel_puts(const char *s) {
    while (*s) {
        if (*s == '\n') {
            cursor_x = 0;
            cursor_y += 8;
        } else if (*s == '\r') {
            cursor_x = 0;
        } else if (*s == '\b') {
            if (cursor_x >= 8) {
                cursor_x -= 8;
                vga_fill_rect(cursor_x, cursor_y, 8, 8, current_bg);
            }
        } else {
            vga_draw_char(cursor_x, cursor_y, *s, current_fg, current_bg);
            cursor_x += 8;
            if (cursor_x >= VGA_WIDTH) {
                cursor_x = 0;
                cursor_y += 8;
            }
        }
        s++;
    }
}

void kernel_puthex(uint64_t val) {
    char buf[17];
    for (int i = 15; i >= 0; i--) {
        uint8_t nibble = (val >> (i * 4)) & 0xF;
        buf[15 - i] = nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
    }
    buf[16] = 0;
    kernel_puts(buf);
}

void main(void) {
    /* Initialize VGA */
    vga_init_mode13();
    vga_load_default_palette();
    vga_clear(0x00);
    
    /* Initialize keyboard */
    keyboard_init();

    /* Headless verification marker on COM1 */
    serial_init();
    serial_puts("BOXEDLANG-OK\r\n");
    
    /* Draw boot screen */
    vga_fill_rect(0, 0, VGA_WIDTH, 24, 0x01);
    vga_draw_string(10, 4, "BoxedLANG OS v0.01", 0x0F, 0x01);
    vga_draw_string(10, 14, "Graphics: 320x200x256 | Keyboard: PS/2", 0x0A, 0x01);
    
    /* Draw border */
    vga_draw_rect(0, 24, VGA_WIDTH, VGA_HEIGHT - 24, 0x0E);
    
    /* Demo graphics */
    for (int i = 0; i < 16; i++) {
        vga_fill_rect(i * 20, 30, 18, 18, i);
    }
    
    vga_draw_string(10, 60, "Keyboard test - type something:", 0x0F, 0x00);
    
    cursor_x = 10;
    cursor_y = 70;
    
    /* Main loop */
    char input_buffer[256];
    int input_pos = 0;
    
    while (1) {
        if (keyboard_available()) {
            char c = keyboard_getchar();
            if (c) {
                if (c == '\n') {
                    input_buffer[input_pos] = 0;
                    kernel_puts("\n> ");
                    input_pos = 0;
                } else if (c == '\b') {
                    if (input_pos > 0) {
                        input_pos--;
                        kernel_puts("\b \b");
                    }
                } else {
                    if (input_pos < 255) {
                        input_buffer[input_pos++] = c;
                        char tmp[2] = {c, 0};
                        kernel_puts(tmp);
                    }
                }
            }
        }
        
        /* Draw some animated graphics */
        static int frame = 0;
        frame++;
        if (frame % 100000 == 0) {
            vga_fill_rect((frame / 100000) % 320, 200, 2, 2, (frame / 10000) % 16);
        }
    }
}
