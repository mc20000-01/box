/* x86_64 VGA Graphics and Keyboard for BoxedLANG OS */
#ifndef _X86_64_GRAPHICS_H
#define _X86_64_GRAPHICS_H

#include <stdint.h>
#include <stddef.h>

/* VGA Mode 13h: 320x200x256 colors */
#define VGA_WIDTH  320
#define VGA_HEIGHT 200
#define VGA_MEMORY 0xA0000

/* VGA Ports */
#define VGA_SEQ_INDEX    0x3C4
#define VGA_SEQ_DATA     0x3C5
#define VGA_CRTC_INDEX   0x3D4
#define VGA_CRTC_DATA    0x3D5
#define VGA_GC_INDEX     0x3CE
#define VGA_GC_DATA      0x3CF
#define VGA_ATTR_INDEX   0x3C0
#define VGA_ATTR_DATA    0x3C0
#define VGA_ATTR_WRITE   0x3C0
#define VGA_ATTR_READ    0x3C1
#define VGA_MISC_WRITE   0x3C2
#define VGA_MISC_READ    0x3CC
#define VGA_PEL_WRITE    0x3C8
#define VGA_PEL_READ     0x3C7
#define VGA_DAC_MASK     0x3C6
#define VGA_DAC_READ     0x3C7
#define VGA_DAC_WRITE    0x3C8
#define VGA_DAC_DATA     0x3C9

/* PS/2 Keyboard Ports */
#define PS2_DATA_PORT    0x60
#define PS2_STATUS_PORT  0x64
#define PS2_CMD_PORT     0x64

/* PS/2 Commands */
#define PS2_CMD_RESET         0xFF
#define PS2_CMD_SELF_TEST     0xAA
#define PS2_CMD_ECHO          0xEE
#define PS2_CMD_ENABLE        0xF4
#define PS2_CMD_DISABLE       0xF5
#define PS2_CMD_SET_DEFAULTS  0xF6
#define PS2_CMD_SET_SCANCODE  0xF0

/* Keyboard Controller Commands */
#define KBC_CMD_READ_CONFIG    0x20
#define KBC_CMD_WRITE_CONFIG   0x60
#define KBC_CMD_DISABLE_PORT2  0xA7
#define KBC_CMD_ENABLE_PORT2   0xA8
#define KBC_CMD_TEST_PORT2     0xA9
#define KBC_CMD_TEST_CONTROLLER 0xAA
#define KBC_CMD_TEST_PORT1     0xAB
#define KBC_CMD_DISABLE_PORT1  0xAD
#define KBC_CMD_ENABLE_PORT1   0xAE

/* VGA Palette (256 colors) */
typedef struct { uint8_t r, g, b; } vga_color_t;

/* Framebuffer */
extern volatile uint8_t *vga_fb;

/* Keyboard */
extern uint8_t keyboard_buffer[256];
extern volatile uint8_t keyboard_head, keyboard_tail;

/* Function declarations */
void vga_init_mode13(void);
void vga_clear(uint8_t color);
void vga_putpixel(int x, int y, uint8_t color);
uint8_t vga_getpixel(int x, int y);
void vga_draw_line(int x1, int y1, int x2, int y2, uint8_t color);
void vga_draw_rect(int x, int y, int w, int h, uint8_t color);
void vga_fill_rect(int x, int y, int w, int h, uint8_t color);
void vga_draw_char(int x, int y, char c, uint8_t fg, uint8_t bg);
void vga_draw_string(int x, int y, const char *str, uint8_t fg, uint8_t bg);
void vga_set_palette(int index, uint8_t r, uint8_t g, uint8_t b);
void vga_load_default_palette(void);

/* Keyboard */
void keyboard_init(void);
uint8_t keyboard_read(void);
int keyboard_available(void);
char keyboard_getchar(void);

/* I/O */
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__ ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}
static inline void io_wait(void) { __asm__ __volatile__ ("jmp 1f\n1: jmp 2f\n2:" ::: "memory"); }

#endif
