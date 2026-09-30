/* Minimal Bare-Metal Common Header for BoxedLANG
 * Provides only core types and functions needed for bare metal
 * Does NOT include WiFi/GFX/HTTP stack - those are in bx_wifi.h/bx_gfx.h for hosted env
 */
#ifndef BX_COMMON_H
#define BX_COMMON_H

/* ============================================================================
 * Standard Integer Types (freestanding)
 * ============================================================================ */
/* Include system headers first to get standard types */
#include <stdint.h>
#include <stddef.h>

/* Only provide types that might be missing in truly bare metal environments.
 * When system headers are available (as they are in our build), they provide these. */
/* No typedefs here - rely on system headers */

/* ============================================================================
 * Core Interpreter Types
 * ============================================================================ */
#define BOX_HASH_SIZE 256

typedef struct { char *name; long value; int is_string; char *str_value; } Box;
typedef struct { Box *items; size_t len, cap; size_t hash_table[BOX_HASH_SIZE]; } Boxes;
typedef struct { char *name; int line; } Mark;
typedef struct { Mark *items; size_t len, cap; } Marks;
typedef struct {
    char **lines;
    char **comments;
    int count;
    Boxes boxes;
    Marks marks;
    int halted;
} Program;

/* ============================================================================
 * Common Macros
 * ============================================================================ */
#define NULL ((void*)0)
#define offsetof(type, member) __builtin_offsetof(type, member)

/* FILE type for minimal stdio */
#ifndef FILE
typedef struct __FILE FILE;
#endif

/* ============================================================================
 * Minimal String/Memory Functions
 * ============================================================================ */
static inline void *memcpy(void *dest, const void *src, size_t n) {
    char *d = dest; const char *s = src; while (n--) *d++ = *s++; return dest;
}
static inline void *memset(void *s, int c, size_t n) { char *d = s; while (n--) *d++ = c; return s; }
static inline void *memmove(void *dest, const void *src, size_t n) {
    if (dest <= src) return memcpy(dest, src, n);
    char *d = (char*)dest + n; const char *s = (const char*)src + n;
    while (n--) *--d = *--s; return dest;
}
static inline size_t strlen(const char *s) { size_t n = 0; while (*s++) n++; return n; }
static inline int strcmp(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *(unsigned char*)a - *(unsigned char*)b; }
static inline int strncmp(const char *a, const char *b, size_t n) { while (n && *a && *a == *b) { a++; b++; n--; } return n ? *(unsigned char*)a - *(unsigned char*)b : 0; }
static inline char *strcpy(char *d, const char *s) { char *r = d; while ((*d++ = *s++)); return r; }
static inline char *strncpy(char *d, const char *s, size_t n) { char *r = d; while (n && (*d++ = *s++)) n--; while (n--) *d++ = 0; return r; }
static inline int memcmp(const void *a, const void *b, size_t n) { const unsigned char *p = a, *q = b; while (n--) { if (*p != *q) return *p - *q; p++; q++; } return 0; }
static inline void *memchr(const void *s, int c, size_t n) { const unsigned char *p = s; while (n--) { if (*p == (unsigned char)c) return (void*)p; p++; } return NULL; }

/* ============================================================================
 * Standard I/O Stubs (to be implemented per-platform)
 * ============================================================================ */
/* Platforms MUST provide bx_panic implementation */
extern void bx_panic(const char *msg);

/* ============================================================================
 * Target Feature Flags
 * ============================================================================ */
#define BX_FREESTANDING 1
#define BX_VERSION "0.1.0"

/* ============================================================================
 * GFX / Linear Framebuffer Abstraction (minimal)
 * ============================================================================ */
typedef struct {
    uint32_t *framebuffer;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint8_t bpp;
} bx_framebuffer_t;

static inline void bx_gfx_init(bx_framebuffer_t *fb, void *addr, uint32_t w, uint32_t h, uint32_t pitch, uint8_t bpp) {
    fb->framebuffer = (uint32_t*)addr;
    fb->width = w;
    fb->height = h;
    fb->pitch = pitch;
    fb->bpp = bpp;
}

static inline void bx_gfx_put_pixel(bx_framebuffer_t *fb, uint32_t x, uint32_t y, uint32_t color) {
    if (x >= fb->width || y >= fb->height) return;
    size_t index = (y * (fb->pitch / 4)) + x;
    fb->framebuffer[index] = color;
}

static inline void bx_gfx_clear(bx_framebuffer_t *fb, uint32_t color) {
    size_t total_pixels = (fb->pitch / 4) * fb->height;
    for (size_t i = 0; i < total_pixels; i++) fb->framebuffer[i] = color;
}

static inline void bx_gfx_draw_rect(bx_framebuffer_t *fb, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t color) {
    for (uint32_t cy = y; cy < y + h; cy++) {
        for (uint32_t cx = x; cx < x + w; cx++) {
            bx_gfx_put_pixel(fb, cx, cy, color);
        }
    }
}

/* Color helpers */
static inline uint32_t bx_gfx_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return (r << 24) | (g << 16) | (b << 8) | 0xFF;
}
static inline uint32_t bx_gfx_rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return (r << 24) | (g << 16) | (b << 8) | a;
}

/* ============================================================================
 * System / Platform Abstraction (minimal)
 * ============================================================================ */
typedef enum {
    BX_SYS_NATIVE = 0,
    BX_SYS_BAREMETAL = 1,
    BX_SYS_UEFI = 2,
    BX_SYS_WEB = 3
} bx_sys_target_t;

bx_sys_target_t bx_sys_get_target(void);
void bx_sys_yield(void);
uint64_t bx_sys_get_ticks(void);
void bx_sys_sleep_ms(uint32_t ms);

/* Entry point - provided by user code */
extern void bx_main(void);

#endif /* BX_COMMON_H */