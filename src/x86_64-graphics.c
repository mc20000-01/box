/* x86_64 VGA Graphics and Keyboard Implementation */
static inline int my_abs(int x) { return x < 0 ? -x : x; }
#include "os-headers/x86_64-graphics.h"

/* Framebuffer */
volatile uint8_t *vga_fb = (volatile uint8_t*)VGA_MEMORY;

/* Keyboard buffer */
uint8_t keyboard_buffer[256];
volatile uint8_t keyboard_head = 0, keyboard_tail = 0;

#include "bx_font8x8.h"

/* Wait for VGA retrace */
static void vga_wait_retrace(void) {
    while (inb(0x3DA) & 0x08);
    while (!(inb(0x3DA) & 0x08));
}

/* Initialize VGA Mode 13h (320x200x256) by programming the VGA chipset
 * directly.  The BIOS `int $0x10` call that set this mode historically is
 * unavailable in protected mode, so the registers are written by hand. */
void vga_init_mode13(void) {
    unsigned char crtc[18] = {0x5D,0x4F,0x50,0x82,0x54,0x80,0x0B,0x3E,0x00,
                              0x40,0x00,0x00,0x00,0x00,0x00,0x00,0xEA,0x8C};
    unsigned char grc[9] = {0x00,0x00,0x00,0x00,0x00,0x40,0x05,0x0F,0xFF};
    int i;

    /* Misc output register: color, high speed, VS+HS */
    outb(0x3C2, 0x63);

    /* Sequencer: reset, clocking, memory map */
    outb(0x3C4, 0x00); outb(0x3C5, 0x01);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x0F);
    outb(0x3C4, 0x03); outb(0x3C5, 0x00);
    outb(0x3C4, 0x04); outb(0x3C5, 0x0E);
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);

    /* CRTC */
    for (i = 0; i < 18; i++) {
        outb(0x3D4, i);
        outb(0x3D5, crtc[i]);
    }

    /* Graphics controller */
    for (i = 0; i < 9; i++) {
        outb(0x3CE, i);
        outb(0x3CF, grc[i]);
    }

    /* Attribute controller: reset/roll lights + enable */
    for (i = 0; i <= 0x13; i++) {
        outb(0x3C0, i);
        outb(0x3C0, 0x00);
    }
    outb(0x3C0, 0x20);

    /* Wait for mode set */
    for (volatile int j = 0; j < 100000; j++);
}

/* Clear screen */
void vga_clear(uint8_t color) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_fb[i] = color;
    }
}

/* Put pixel */
void vga_putpixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        vga_fb[y * VGA_WIDTH + x] = color;
    }
}

/* Get pixel */
uint8_t vga_getpixel(int x, int y) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        return vga_fb[y * VGA_WIDTH + x];
    }
    return 0;
}

/* Draw line (Bresenham) */
void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color) {
    int dx = my_abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -my_abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy, e2;
    
    while (1) {
        vga_putpixel(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

/* Draw rectangle outline */
void vga_draw_rect(int x, int y, int w, int h, uint8_t color) {
    vga_draw_line(x, y, x + w - 1, y, color);
    vga_draw_line(x, y + h - 1, x + w - 1, y + h - 1, color);
    vga_draw_line(x, y, x, y + h - 1, color);
    vga_draw_line(x + w - 1, y, x + w - 1, y + h - 1, color);
}

/* Fill rectangle */
void vga_fill_rect(int x, int y, int w, int h, uint8_t color) {
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            vga_putpixel(xx, yy, color);
        }
    }
}

/* Draw 8x8 character */
void vga_draw_char(int x, int y, char c, uint8_t fg, uint8_t bg) {
    if ((unsigned char)c < 32 || (unsigned char)c >= 128) c = 32;
    const uint8_t *glyph = bx_font8x8[(uint8_t)c];
    
    for (int yy = 0; yy < 8; yy++) {
        uint8_t row = glyph[yy];
        for (int xx = 0; xx < 8; xx++) {
            vga_putpixel(x + xx, y + yy, (row & (0x80 >> xx)) ? fg : bg);
        }
    }
}

/* Draw string */
void vga_draw_string(int x, int y, const char *str, uint8_t fg, uint8_t bg) {
    while (*str) {
        vga_draw_char(x, y, *str++, fg, bg);
        x += 8;
    }
}

/* Set palette color */
void vga_set_palette(int index, uint8_t r, uint8_t g, uint8_t b) {
    outb(VGA_PEL_WRITE, index);
    outb(VGA_DAC_DATA, r >> 2);
    outb(VGA_DAC_DATA, g >> 2);
    outb(VGA_DAC_DATA, b >> 2);
}

/* Load default VGA palette */
void vga_load_default_palette(void) {
    for (int i = 0; i < 256; i++) {
        vga_set_palette(i, 
            ((i & 0x07) * 255) / 7,
            ((i & 0x38) * 255) / 56,
            ((i & 0xC0) * 255) / 192);
    }
}

/* PS/2 Keyboard */
void keyboard_wait_input(void) {
    while (inb(PS2_STATUS_PORT) & 0x02);
}

void keyboard_wait_output(void) {
    while (!(inb(PS2_STATUS_PORT) & 0x01));
}

void keyboard_send_cmd(uint8_t cmd) {
    keyboard_wait_input();
    outb(PS2_CMD_PORT, cmd);
}

void keyboard_send_data(uint8_t data) {
    keyboard_wait_input();
    outb(PS2_DATA_PORT, data);
}

void keyboard_init(void) {
    /* Disable keyboard */
    keyboard_send_cmd(0xAD);
    io_wait();
    
    /* Disable mouse */
    keyboard_send_cmd(0xA7);
    io_wait();
    
    /* Flush output buffer */
    while (inb(PS2_STATUS_PORT) & 0x01) {
        inb(PS2_DATA_PORT);
    }
    
    /* Set controller config */
    keyboard_send_cmd(KBC_CMD_READ_CONFIG);
    keyboard_wait_output();
    uint8_t config = inb(PS2_DATA_PORT);
    config |= 0x01;  /* Enable port 1 interrupt */
    config &= ~0x20; /* Disable port 2 */
    config &= ~0x10; /* Disable port 2 interrupt */
    keyboard_send_cmd(KBC_CMD_WRITE_CONFIG);
    keyboard_wait_input();
    outb(PS2_DATA_PORT, config);
    
    /* Reset keyboard */
    keyboard_send_data(PS2_CMD_RESET);
    keyboard_wait_output();
    uint8_t ack = inb(PS2_DATA_PORT);
    
    /* Set scancode set 2 */
    keyboard_send_data(0xF0);
    keyboard_wait_output();
    ack = inb(PS2_DATA_PORT);
    keyboard_send_data(0x02);
    keyboard_wait_output();
    ack = inb(PS2_DATA_PORT);
    
    /* Enable keyboard */
    keyboard_send_data(PS2_CMD_ENABLE);
    keyboard_wait_output();
    ack = inb(PS2_DATA_PORT);
    
    /* Enable keyboard interrupt */
    keyboard_send_cmd(KBC_CMD_READ_CONFIG);
    keyboard_wait_output();
    config = inb(PS2_DATA_PORT);
    config |= 0x01;  /* Enable port 1 interrupt */
    keyboard_send_cmd(KBC_CMD_WRITE_CONFIG);
    keyboard_wait_input();
    outb(PS2_DATA_PORT, config);
    
    keyboard_head = keyboard_tail = 0;
}

/* Keyboard interrupt handler (call from IRQ1 handler) */
void keyboard_interrupt(void) {
    uint8_t scancode = inb(PS2_DATA_PORT);
    uint8_t next_tail = (keyboard_tail + 1) % 256;
    if (next_tail != keyboard_head) {
        keyboard_buffer[keyboard_tail] = scancode;
        keyboard_tail = next_tail;
    }
}

/* Check if key available */
int keyboard_available(void) {
    return keyboard_head != keyboard_tail;
}

/* Get key from buffer */
uint8_t keyboard_read(void) {
    if (keyboard_head == keyboard_tail) return 0;
    uint8_t sc = keyboard_buffer[keyboard_head];
    keyboard_head = (keyboard_head + 1) % 256;
    return sc;
}

/* Simple scancode to ASCII (set 2) */
/* Simple scancode to ASCII (set 2) */
/* Simple scancode to ASCII (set 2) */
static const char scancode_to_ascii[128] = {
    [0x1C] = 'a', [0x32] = 'b', [0x21] = 'c', [0x23] = 'd', [0x24] = 'e',
    [0x2B] = 'f', [0x34] = 'g', [0x33] = 'h', [0x43] = 'h', [0x3A] = 'j',
    [0x42] = 'k', [0x4B] = 'l', [0x3B] = 'm', [0x31] = 'n', [0x44] = 'o',
    [0x4D] = 'p', [0x15] = 'q', [0x2D] = 'r', [0x1B] = 's', [0x2C] = 't',
    [0x3C] = 'u', [0x2A] = 'v', [0x1D] = 'w', [0x22] = 'x', [0x35] = 'y',
    [0x1A] = 'z', [0x45] = '0', [0x16] = '1', [0x1E] = '2', [0x26] = '3',
    [0x25] = '4', [0x2E] = '5', [0x36] = '6', [0x3D] = '7', [0x3E] = '8',
    [0x46] = '9', [0x5A] = '\n', [0x66] = '\b', [0x29] = ' ', [0x0D] = '\t',
    [0x12] = '-', [0x13] = '=', [0x5B] = '[', [0x5D] = ']', [0x5C] = '\\',
    [0x33] = ';', [0x34] = '\'', [0x32] = '`', [0x41] = ',', [0x49] = '.',
    [0x4A] = '/', [0x58] = '\n', [0x14] = ' ', [0x66] = '\b',
};char keyboard_getchar(void) {
    while (!keyboard_available());
    uint8_t sc = keyboard_read();
    
    /* Handle extended scancodes (E0 prefix) */
    if (sc == 0xE0) {
        while (!keyboard_available());
        sc = keyboard_read();
        /* Handle extended keys if needed */
        return 0;
    }
    
    /* Handle break codes (F0 prefix) */
    if (sc == 0xF0) {
        while (!keyboard_available());
        sc = keyboard_read();  /* Discard break code */
        return 0;
    }
    
    if (sc < 128 && scancode_to_ascii[sc]) {
        return scancode_to_ascii[sc];
    }
    return 0;
}
