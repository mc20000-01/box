/* BX GFX - software rasterizer, color model, and UI element tree.
 * See bx_gfx.h for the layout and the pixel format contract. */
#include "bx_gfx.h"
#include "bx_font8x8.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

bx_gfx_ctx_t g_bx_gfx;
bx_gfx_theme_t g_gfx_theme_history[BX_GFX_THEME_HISTORY_MAX];
uint32_t g_gfx_theme_history_count = 0;

static void *xmalloc(size_t n) { void *p = malloc(n ? n : 1); if (!p) { fprintf(stderr, "bx_gfx: out of memory\n"); exit(1); } return p; }
static char *xstrdup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *d = (char *)xmalloc(n);
    memcpy(d, s, n);
    return d;
}
/* Local min/max so the rasterizer pulls in no math-library symbol beyond
 * what the compiler already emits for the trig used by transforms. */
static float fmin_(float a, float b) { return a < b ? a : b; }
static float fmax_(float a, float b) { return a > b ? a : b; }

static void *xrealloc(void *p, size_t n) { void *q = realloc(p, n ? n : 1); if (!q) { fprintf(stderr, "bx_gfx: out of memory\n"); exit(1); } return q; }

/* ------------------------------------------------------------------ colors */

uint32_t bx_gfx_color_lerp(uint32_t c0, uint32_t c1, int32_t t) {
    if (t < 0) t = 0;
    if (t > 256) t = 256;
    int32_t it = 256 - t;
    int32_t r = (int32_t)BX_GFX_R(c0) * it + (int32_t)BX_GFX_R(c1) * t;
    int32_t g = (int32_t)BX_GFX_G(c0) * it + (int32_t)BX_GFX_G(c1) * t;
    int32_t b = (int32_t)BX_GFX_B(c0) * it + (int32_t)BX_GFX_B(c1) * t;
    int32_t a = (int32_t)BX_GFX_A(c0) * it + (int32_t)BX_GFX_A(c1) * t;
    return BX_GFX_RGBA(r >> 8, g >> 8, b >> 8, a >> 8);
}

uint32_t bx_gfx_color_scale_alpha(uint32_t c, int32_t factor) {
    if (factor < 0) factor = 0;
    if (factor > 256) factor = 256;
    int32_t a = (int32_t)BX_GFX_A(c) * factor / 256;
    return (c & 0xFFFFFF00u) | (uint32_t)a;
}

/* CSS-ish names. Kept to the common set so the table stays small. */
static const struct { const char *name; uint32_t color; } named_colors[] = {
    {"black",       BX_GFX_RGBA(0,0,0,255)},       {"white",     BX_GFX_RGBA(255,255,255,255)},
    {"red",         BX_GFX_RGBA(255,0,0,255)},     {"green",     BX_GFX_RGBA(0,128,0,255)},
    {"lime",        BX_GFX_RGBA(0,255,0,255)},     {"blue",      BX_GFX_RGBA(0,0,255,255)},
    {"yellow",      BX_GFX_RGBA(255,255,0,255)},   {"cyan",      BX_GFX_RGBA(0,255,255,255)},
    {"aqua",        BX_GFX_RGBA(0,255,255,255)},   {"magenta",   BX_GFX_RGBA(255,0,255,255)},
    {"fuchsia",     BX_GFX_RGBA(255,0,255,255)},   {"silver",    BX_GFX_RGBA(192,192,192,255)},
    {"gray",        BX_GFX_RGBA(128,128,128,255)}, {"grey",      BX_GFX_RGBA(128,128,128,255)},
    {"maroon",      BX_GFX_RGBA(128,0,0,255)},     {"olive",     BX_GFX_RGBA(128,128,0,255)},
    {"navy",        BX_GFX_RGBA(0,0,128,255)},     {"purple",    BX_GFX_RGBA(128,0,128,255)},
    {"teal",        BX_GFX_RGBA(0,128,128,255)},   {"orange",    BX_GFX_RGBA(255,165,0,255)},
    {"pink",        BX_GFX_RGBA(255,192,203,255)}, {"brown",     BX_GFX_RGBA(165,42,42,255)},
    {"gold",        BX_GFX_RGBA(255,215,0,255)},   {"indigo",    BX_GFX_RGBA(75,0,130,255)},
    {"violet",      BX_GFX_RGBA(238,130,238,255)}, {"turquoise", BX_GFX_RGBA(64,224,208,255)},
    {"coral",       BX_GFX_RGBA(255,127,80,255)},  {"salmon",    BX_GFX_RGBA(250,128,114,255)},
    {"crimson",     BX_GFX_RGBA(220,20,60,255)},   {"khaki",     BX_GFX_RGBA(240,230,140,255)},
    {"plum",        BX_GFX_RGBA(221,160,221,255)}, {"orchid",    BX_GFX_RGBA(218,112,214,255)},
    {"beige",       BX_GFX_RGBA(245,245,220,255)}, {"ivory",     BX_GFX_RGBA(255,255,240,255)},
    {"navajo",      BX_GFX_RGBA(255,222,173,255)}, {"azure",     BX_GFX_RGBA(240,255,255,255)},
    {"lavender",    BX_GFX_RGBA(230,230,250,255)}, {"linen",     BX_GFX_RGBA(250,240,230,255)},
    {"snow",        BX_GFX_RGBA(255,250,250,255)}, {"skyblue",   BX_GFX_RGBA(135,206,235,255)},
    {"steelblue",   BX_GFX_RGBA(70,130,180,255)},  {"royalblue", BX_GFX_RGBA(65,105,225,255)},
    {"forestgreen", BX_GFX_RGBA(34,139,34,255)},   {"seagreen",  BX_GFX_RGBA(46,139,87,255)},
    {"darkred",     BX_GFX_RGBA(139,0,0,255)},     {"darkblue",  BX_GFX_RGBA(0,0,139,255)},
    {"darkgreen",   BX_GFX_RGBA(0,100,0,255)},     {"darkgray",  BX_GFX_RGBA(169,169,169,255)},
    {"lightgray",   BX_GFX_RGBA(211,211,211,255)}, {"lightblue", BX_GFX_RGBA(173,216,230,255)},
    {"lightgreen",  BX_GFX_RGBA(144,238,144,255)}, {"transparent", BX_GFX_RGBA(0,0,0,0)},
    {NULL, 0}
};

static int ci_equal(const char *a, const char *b) {
    while (*a && *b) {
        int ca = *a, cb = *b;
        if (ca >= 'A' && ca <= 'Z') ca += 32;
        if (cb >= 'A' && cb <= 'Z') cb += 32;
        if (ca != cb) return 0;
        a++; b++;
    }
    return *a == 0 && *b == 0;
}

uint32_t bx_gfx_color_named(const char *name, int *found) {
    if (found) *found = 0;
    if (!name || !*name) return BX_GFX_RGBA(0, 0, 0, 255);
    for (int i = 0; named_colors[i].name; i++) {
        if (ci_equal(named_colors[i].name, name)) {
            if (found) *found = 1;
            return named_colors[i].color;
        }
    }
    return BX_GFX_RGBA(0, 0, 0, 255);
}

static int hexval(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

uint32_t bx_gfx_parse_color(const char *str) {
    if (!str) return 0;
    while (*str == ' ' || *str == '\t') str++;
    if (*str != '#') return 0;
    str++;
    int d[8], n = 0;
    while (n < 8 && hexval(str[n]) >= 0) { d[n] = hexval(str[n]); n++; }
    /* A trailing unit suffix (%, px) is tolerated and ignored. */
    if (n != 3 && n != 4 && n != 6 && n != 8) return 0;
    if (str[n] != 0 && str[n] != '%' && str[n] != ' ' && str[n] != '\t') return 0;

    if (n == 3 || n == 4) {
        /* Short form expands each nibble: #f80 -> #ff8800. */
        uint32_t r = (uint32_t)(d[0] * 17), g = (uint32_t)(d[1] * 17), b = (uint32_t)(d[2] * 17);
        uint32_t a = (n == 4) ? (uint32_t)(d[3] * 17) : 255u;
        return BX_GFX_RGBA(r, g, b, a);
    }
    uint32_t r = (uint32_t)((d[0] << 4) | d[1]);
    uint32_t g = (uint32_t)((d[2] << 4) | d[3]);
    uint32_t b = (uint32_t)((d[4] << 4) | d[5]);
    uint32_t a = (n == 8) ? (uint32_t)((d[6] << 4) | d[7]) : 255u;
    return BX_GFX_RGBA(r, g, b, a);
}

/* ------------------------------------------------------------- framebuffer */

int bx_gfx_fb_init(bx_gfx_fb_t *fb, int32_t w, int32_t h, uint32_t fill) {
    if (!fb || w <= 0 || h <= 0) return -1;
    if (w > 8192 || h > 8192) return -1;
    free(fb->pixels);
    fb->pixels = (uint32_t *)xmalloc((size_t)w * (size_t)h * sizeof(uint32_t));
    fb->width = w; fb->height = h;
    bx_gfx_identity(fb);
    bx_gfx_clip_reset(fb);
    if (fill) bx_gfx_fb_clear(fb, fill);
    return 0;
}

void bx_gfx_fb_free(bx_gfx_fb_t *fb) {
    if (!fb) return;
    free(fb->pixels); fb->pixels = NULL; fb->width = fb->height = 0;
}

void bx_gfx_fb_clear(bx_gfx_fb_t *fb, uint32_t color) {
    if (!fb || !fb->pixels) return;
    size_t total = (size_t)fb->width * (size_t)fb->height;
    for (size_t i = 0; i < total; i++) fb->pixels[i] = color;
}

void bx_gfx_clip_set(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h) {
    if (!fb) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > fb->width)  w = fb->width - x;
    if (y + h > fb->height) h = fb->height - y;
    if (w < 0) w = 0;
    if (h < 0) h = 0;
    fb->clip_x = x; fb->clip_y = y; fb->clip_w = w; fb->clip_h = h;
}

void bx_gfx_clip_reset(bx_gfx_fb_t *fb) {
    if (!fb) return;
    fb->clip_x = 0; fb->clip_y = 0; fb->clip_w = fb->width; fb->clip_h = fb->height;
}

/* --------------------------------------------------------------- matrices */

void bx_gfx_identity(bx_gfx_fb_t *fb) {
    if (!fb) return;
    fb->m[0] = 1; fb->m[1] = 0; fb->m[2] = 0;
    fb->m[3] = 0; fb->m[4] = 1; fb->m[5] = 0;
    fb->m[6] = 0; fb->m[7] = 0; fb->m[8] = 1;
}

/* out = fb->m * n, in row-major 3x3 order. */
static void mat_mul(const float *a, const float *n, float *out) {
    float t[9];
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 3; c++)
            t[r * 3 + c] = a[r * 3] * n[c] + a[r * 3 + 1] * n[3 + c] + a[r * 3 + 2] * n[6 + c];
    memcpy(out, t, sizeof t);
}

void bx_gfx_translate(bx_gfx_fb_t *fb, float dx, float dy) {
    if (!fb) return;
    float n[9] = {1, 0, dx, 0, 1, dy, 0, 0, 1};
    mat_mul(fb->m, n, fb->m);
}

void bx_gfx_scale(bx_gfx_fb_t *fb, float sx, float sy) {
    if (!fb) return;
    float n[9] = {sx, 0, 0, 0, sy, 0, 0, 0, 1};
    mat_mul(fb->m, n, fb->m);
}

void bx_gfx_rotate(bx_gfx_fb_t *fb, float degrees) {
    if (!fb) return;
    double rad = (double)degrees * 3.14159265358979323846 / 180.0;
    float c = (float)cos(rad), s = (float)sin(rad);
    float n[9] = {c, -s, 0, s, c, 0, 0, 0, 1};
    mat_mul(fb->m, n, fb->m);
}

void bx_gfx_transform_point(const bx_gfx_fb_t *fb, float x, float y, float *ox, float *oy) {
    if (!fb) { if (ox) *ox = x; if (oy) *oy = y; return; }
    if (ox) *ox = fb->m[0] * x + fb->m[1] * y + fb->m[2];
    if (oy) *oy = fb->m[3] * x + fb->m[4] * y + fb->m[5];
}

/* push/pop need somewhere to keep saved matrices. A small stack is enough for
 * the scripting use case and avoids exposing another allocation to callers. */
#define BX_GFX_MAT_STACK_MAX 32
static float g_mat_stack[BX_GFX_MAT_STACK_MAX][9];
static int   g_mat_sp = 0;

void bx_gfx_push(bx_gfx_fb_t *fb) {
    if (!fb || g_mat_sp >= BX_GFX_MAT_STACK_MAX) return;
    memcpy(g_mat_stack[g_mat_sp++], fb->m, sizeof fb->m);
}

void bx_gfx_pop(bx_gfx_fb_t *fb) {
    if (!fb || g_mat_sp <= 0) return;
    memcpy(fb->m, g_mat_stack[--g_mat_sp], sizeof fb->m);
}

/* ------------------------------------------------------------------ pixels */

/* Source-over blend of a single pixel. */
static void blend_px(bx_gfx_fb_t *fb, int32_t x, int32_t y, uint32_t c) {
    uint32_t sa = BX_GFX_A(c);
    if (sa == 0) return;
    uint32_t *px = &fb->pixels[(size_t)y * (size_t)fb->width + (size_t)x];
    if (sa == 255) { *px = c; return; }
    uint32_t da = BX_GFX_A(*px);
    if (da == 0) { *px = c | 0xFFu; return; }
    /* Integer source-over: out = src + dst*(1-srcA). */
    uint32_t inv = 255u - sa;
    uint32_t r = BX_GFX_R(c) + BX_GFX_R(*px) * inv / 255u;
    uint32_t g = BX_GFX_G(c) + BX_GFX_G(*px) * inv / 255u;
    uint32_t b = BX_GFX_B(c) + BX_GFX_B(*px) * inv / 255u;
    uint32_t a = sa + da * inv / 255u;
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    if (a > 255) a = 255;
    *px = BX_GFX_RGBA(r, g, b, a);
}

static int in_clip(const bx_gfx_fb_t *fb, int32_t x, int32_t y) {
    return x >= fb->clip_x && y >= fb->clip_y &&
           x < fb->clip_x + fb->clip_w && y < fb->clip_y + fb->clip_h;
}

void bx_gfx_plot(bx_gfx_fb_t *fb, int32_t x, int32_t y, uint32_t c) {
    if (!fb || !fb->pixels) return;
    if (x < 0 || y < 0 || x >= fb->width || y >= fb->height) return;
    if (!in_clip(fb, x, y)) return;
    blend_px(fb, x, y, c);
}

uint32_t bx_gfx_get(bx_gfx_fb_t *fb, int32_t x, int32_t y) {
    if (!fb || !fb->pixels) return 0;
    if (x < 0 || y < 0 || x >= fb->width || y >= fb->height) return 0;
    return fb->pixels[(size_t)y * (size_t)fb->width + (size_t)x];
}

/* -------------------------------------------------------------- primitives */

void bx_gfx_line(bx_gfx_fb_t *fb, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t c) {
    if (!fb || !fb->pixels) return;
    float tx0, ty0, tx1, ty1;
    bx_gfx_transform_point(fb, (float)x0, (float)y0, &tx0, &ty0);
    bx_gfx_transform_point(fb, (float)x1, (float)y1, &tx1, &ty1);
    x0 = (int32_t)floorf(tx0 + 0.5f); y0 = (int32_t)floorf(ty0 + 0.5f);
    x1 = (int32_t)floorf(tx1 + 0.5f); y1 = (int32_t)floorf(ty1 + 0.5f);

    /* Bresenham, so lines stay 1px wide with no gaps. */
    int32_t dx = x1 > x0 ? x1 - x0 : x0 - x1;
    int32_t dy = y1 > y0 ? y1 - y0 : y0 - y1;
    int32_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int32_t err = dx - dy;
    for (;;) {
        bx_gfx_plot(fb, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        int32_t e2 = err * 2;
        if (e2 > -dy) { err -= dy; x0 += sx; }
        if (e2 < dx)  { err += dx; y0 += sy; }
    }
}

void bx_gfx_rect(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) {
    if (!fb || !fb->pixels || w <= 0 || h <= 0) return;
    /* Apply the transform to the two opposite corners and take the bounding
     * box, which is exact for axis-aligned transforms and a close-enough
     * bound for rotation. */
    float ax, ay, bx, by;
    bx_gfx_transform_point(fb, (float)x, (float)y, &ax, &ay);
    bx_gfx_transform_point(fb, (float)(x + w), (float)(y + h), &bx, &by);
    int32_t x0 = (int32_t)floorf(fmin_(ax, bx) + 0.5f), y0 = (int32_t)floorf(fmin_(ay, by) + 0.5f);
    int32_t x1 = (int32_t)floorf(fmax_(ax, bx) + 0.5f), y1 = (int32_t)floorf(fmax_(ay, by) + 0.5f);
    for (int32_t yy = y0; yy < y1; yy++)
        for (int32_t xx = x0; xx < x1; xx++)
            bx_gfx_plot(fb, xx, yy, c);
}

void bx_gfx_rect_outline(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c) {
    if (!fb || w <= 0 || h <= 0) return;
    bx_gfx_line(fb, x, y, x + w - 1, y, c);
    bx_gfx_line(fb, x, y + h - 1, x + w - 1, y + h - 1, c);
    bx_gfx_line(fb, x, y, x, y + h - 1, c);
    bx_gfx_line(fb, x + w - 1, y, x + w - 1, y + h - 1, c);
}

/* Filled disk. Each scanline is one horizontal span whose half-width is
 * floor(sqrt(r^2 - dy^2)), so the interior is genuinely covered rather than
 * just the perimeter. */
/* Is (px,py) inside the rounded rect? Every corner is a circle of radius r
 * and the straight edges are handled by clamping to the corner centres, so
 * one test covers every case including r = 0. */
int bx_gfx_round_inside(int32_t px, int32_t py, int32_t x, int32_t y,
                        int32_t w, int32_t h, int32_t r) {
    if (w <= 0 || h <= 0) return 0;
    if (r < 0) r = 0;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (px < x || py < y || px >= x + w || py >= y + h) return 0;
    if (r == 0) return 1;
    int32_t x1 = x + r, x2 = x + w - 1 - r;
    int32_t y1 = y + r, y2 = y + h - 1 - r;
    int32_t cx = px < x1 ? x1 : (px > x2 ? x2 : px);
    int32_t cy = py < y1 ? y1 : (py > y2 ? y2 : py);
    int32_t dx = px - cx, dy = py - cy;
    return dx * dx + dy * dy <= r * r;
}

void bx_gfx_rect_round(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h,
                       int32_t r, uint32_t c) {
    for (int32_t yy = 0; yy < h; yy++)
        for (int32_t xx = 0; xx < w; xx++)
            if (bx_gfx_round_inside(x + xx, y + yy, x, y, w, h, r))
                bx_gfx_plot(fb, x + xx, y + yy, c);
}

void bx_gfx_rect_round_gradient_v(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h,
                                  int32_t r, uint32_t top, uint32_t bottom) {
    if (h <= 0) return;
    for (int32_t yy = 0; yy < h; yy++) {
        /* t is 0..256 across the height, matching bx_gfx_color_lerp. */
        int32_t t = h > 1 ? (yy * 256) / (h - 1) : 0;
        uint32_t c = bx_gfx_color_lerp(top, bottom, t);
        for (int32_t xx = 0; xx < w; xx++)
            if (bx_gfx_round_inside(x + xx, y + yy, x, y, w, h, r))
                bx_gfx_plot(fb, x + xx, y + yy, c);
    }
}

void bx_gfx_rect_round_outline(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h,
                               int32_t r, uint32_t c) {
    if (w <= 0 || h <= 0) return;
    for (int32_t xx = 0; xx < w; xx++) {
        if (bx_gfx_round_inside(x + xx, y, x, y, w, h, r)) bx_gfx_plot(fb, x + xx, y, c);
        if (bx_gfx_round_inside(x + xx, y + h - 1, x, y, w, h, r)) bx_gfx_plot(fb, x + xx, y + h - 1, c);
    }
    for (int32_t yy = 0; yy < h; yy++) {
        if (bx_gfx_round_inside(x, y + yy, x, y, w, h, r)) bx_gfx_plot(fb, x, y + yy, c);
        if (bx_gfx_round_inside(x + w - 1, y + yy, x, y, w, h, r)) bx_gfx_plot(fb, x + w - 1, y + yy, c);
    }
}

void bx_gfx_circle(bx_gfx_fb_t *fb, int32_t cx, int32_t cy, int32_t r, uint32_t c) {
    if (!fb || !fb->pixels || r < 0) return;
    float tcx, tcy;
    bx_gfx_transform_point(fb, (float)cx, (float)cy, &tcx, &tcy);
    cx = (int32_t)floorf(tcx + 0.5f);
    cy = (int32_t)floorf(tcy + 0.5f);
    int64_t rr = (int64_t)r * (int64_t)r;
    for (int32_t dy = -r; dy <= r; dy++) {
        int64_t rem = rr - (int64_t)dy * (int64_t)dy;
        if (rem < 0) continue;
        int32_t dx = (int32_t)sqrt((double)rem);
        /* Guard against the float rounding up into the circle. */
        while (dx > 0 && (int64_t)dx * dx + (int64_t)dy * dy > rr) dx--;
        for (int32_t x = -dx; x <= dx; x++) bx_gfx_plot(fb, cx + x, cy + dy, c);
    }
}

void bx_gfx_circle_outline(bx_gfx_fb_t *fb, int32_t cx, int32_t cy, int32_t r, uint32_t c) {
    if (!fb || r < 0) return;
    float tcx, tcy;
    bx_gfx_transform_point(fb, (float)cx, (float)cy, &tcx, &tcy);
    cx = (int32_t)floorf(tcx + 0.5f);
    cy = (int32_t)floorf(tcy + 0.5f);
    if (r == 0) { bx_gfx_plot(fb, cx, cy, c); return; }
    int32_t x = r, y = 0, err = 1 - r;
    while (x >= y) {
        bx_gfx_plot(fb, cx + x, cy + y, c); bx_gfx_plot(fb, cx + y, cy + x, c);
        bx_gfx_plot(fb, cx - y, cy + x, c); bx_gfx_plot(fb, cx - x, cy + y, c);
        bx_gfx_plot(fb, cx - x, cy - y, c); bx_gfx_plot(fb, cx - y, cy - x, c);
        bx_gfx_plot(fb, cx + y, cy - x, c); bx_gfx_plot(fb, cx + x, cy - y, c);
        y++;
        if (err < 0) err += 2 * y + 1;
        else { x--; err += 2 * (y - x) + 1; }
    }
}

/* Half-space test shared by the filled and outlined triangle. */
static float edge(float ax, float ay, float bx, float by, float px, float py) {
    return (bx - ax) * (py - ay) - (by - ay) * (px - ax);
}

void bx_gfx_tri(bx_gfx_fb_t *fb, float x0, float y0, float x1, float y1,
                float x2, float y2, uint32_t c) {
    if (!fb || !fb->pixels) return;
    float ax, ay, bx, by, cx, cy;
    bx_gfx_transform_point(fb, x0, y0, &ax, &ay);
    bx_gfx_transform_point(fb, x1, y1, &bx, &by);
    bx_gfx_transform_point(fb, x2, y2, &cx, &cy);

    float minx = fmin_(ax, fmin_(bx, cx)), maxx = fmax_(ax, fmax_(bx, cx));
    float miny = fmin_(ay, fmin_(by, cy)), maxy = fmax_(ay, fmax_(by, cy));
    int32_t x_start = (int32_t)floorf(minx), x_end = (int32_t)ceilf(maxx);
    int32_t y_start = (int32_t)floorf(miny), y_end = (int32_t)ceilf(maxy);
    if (x_start < 0) x_start = 0;
    if (y_start < 0) y_start = 0;
    if (x_end > fb->width)  x_end = fb->width;
    if (y_end > fb->height) y_end = fb->height;

    float area = edge(ax, ay, bx, by, cx, cy);
    if (area == 0.0f) return;
    /* Winding can be either way; normalize so a single sign test covers both. */
    float sign = area < 0.0f ? -1.0f : 1.0f;

    for (int32_t y = y_start; y < y_end; y++) {
        for (int32_t x = x_start; x < x_end; x++) {
            float px = (float)x + 0.5f, py = (float)y + 0.5f;
            float w0 = sign * edge(ax, ay, bx, by, px, py);
            float w1 = sign * edge(bx, by, cx, cy, px, py);
            float w2 = sign * edge(cx, cy, ax, ay, px, py);
            if (w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) bx_gfx_plot(fb, x, y, c);
        }
    }
}

void bx_gfx_tri_outline(bx_gfx_fb_t *fb, float x0, float y0, float x1, float y1,
                        float x2, float y2, uint32_t c) {
    if (!fb) return;
    float ax, ay, bx, by, cx, cy;
    bx_gfx_transform_point(fb, x0, y0, &ax, &ay);
    bx_gfx_transform_point(fb, x1, y1, &bx, &by);
    bx_gfx_transform_point(fb, x2, y2, &cx, &cy);
    bx_gfx_line(fb, (int32_t)ax, (int32_t)ay, (int32_t)bx, (int32_t)by, c);
    bx_gfx_line(fb, (int32_t)bx, (int32_t)by, (int32_t)cx, (int32_t)cy, c);
    bx_gfx_line(fb, (int32_t)cx, (int32_t)cy, (int32_t)ax, (int32_t)ay, c);
}

void bx_gfx_poly(bx_gfx_fb_t *fb, const float *pts, int32_t count, uint32_t c) {
    if (!fb || !pts || count < 3) return;
    /* Fan triangulation. Correct for convex polygons, which is the common case
     * for the shapes this exposes. */
    for (int32_t i = 1; i + 1 < count; i++)
        bx_gfx_tri(fb, pts[0], pts[1], pts[i * 2], pts[i * 2 + 1], pts[(i + 1) * 2], pts[(i + 1) * 2 + 1], c);
}

/* Draw one glyph. The transform and the clip are honoured because the caller
 * may have translated or scaled, and text that ignored both would land in the
 * wrong place as soon as anything else did. */
static void bx_gfx_glyph(bx_gfx_fb_t *fb, float x, float y, unsigned char ch, uint32_t c) {
    if (ch < 32 || ch >= 128) ch = ' ';
    const uint8_t *g = bx_font8x8[ch];
    for (int yy = 0; yy < BX_FONT_H; yy++) {
        uint8_t row = g[yy];
        if (!row) continue;
        for (int xx = 0; xx < BX_FONT_W; xx++) {
            if (!(row & (0x80 >> xx))) continue;
            float px = x + xx, py = y + yy, tx, ty;
            bx_gfx_transform_point(fb, px, py, &tx, &ty);
            bx_gfx_plot(fb, (int32_t)tx, (int32_t)ty, c);
        }
    }
}

void bx_gfx_text(bx_gfx_fb_t *fb, int32_t x, int32_t y, const char *s, uint32_t c) {
    if (!s) return;
    for (int i = 0; s[i]; i++)
        bx_gfx_glyph(fb, (float)(x + i * BX_FONT_W), (float)y, (unsigned char)s[i], c);
}

void bx_gfx_text_outlined(bx_gfx_fb_t *fb, int32_t x, int32_t y,
                          const char *s, uint32_t c, uint32_t edge) {
    if (!s) return;
    /* Four passes offset by a pixel, then the glyph on top: that is a border
     * and costs the same as drawing the string five times. */
    for (int i = 0; s[i]; i++)
        for (int d = 0; d < 4; d++) {
            float ox = (d == 0) ? -1 : (d == 1) ? 1 : 0;
            float oy = (d == 2) ? -1 : (d == 3) ? 1 : 0;
            bx_gfx_glyph(fb, (float)(x + i * BX_FONT_W) + ox, (float)y + oy,
                         (unsigned char)s[i], edge);
        }
    bx_gfx_text(fb, x, y, s, c);
}

int32_t bx_gfx_text_w(const char *s) { return s ? (int32_t)strlen(s) * BX_FONT_W : 0; }
int32_t bx_gfx_text_h(void) { return BX_FONT_H; }

void bx_gfx_gradient_v(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h,
                       uint32_t top, uint32_t bottom) {
    if (!fb || !fb->pixels || w <= 0 || h <= 0) return;
    for (int32_t yy = 0; yy < h; yy++) {
        int32_t t = h > 1 ? (yy * 256) / (h - 1) : 0;
        uint32_t c = bx_gfx_color_lerp(top, bottom, t);
        bx_gfx_rect(fb, x, y + yy, w, 1, c);
    }
}

void bx_gfx_gradient_h(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h,
                       uint32_t left, uint32_t right) {
    if (!fb || !fb->pixels || w <= 0 || h <= 0) return;
    for (int32_t xx = 0; xx < w; xx++) {
        int32_t t = w > 1 ? (xx * 256) / (w - 1) : 0;
        uint32_t c = bx_gfx_color_lerp(left, right, t);
        bx_gfx_rect(fb, x + xx, y, 1, h, c);
    }
}

/* --------------------------------------------------------- default target */

static bx_gfx_fb_t g_default_fb;
static int g_default_fb_ready = 0;

bx_gfx_fb_t *bx_gfx_fb_get(void) {
    if (!g_default_fb_ready) {
        bx_gfx_fb_init(&g_default_fb, 320, 200, 0);
        g_default_fb_ready = 1;
    }
    return &g_default_fb;
}

void bx_gfx_fb_set_size(int32_t w, int32_t h) {
    bx_gfx_fb_get();
    if (bx_gfx_fb_init(&g_default_fb, w, h, 0) != 0)
        fprintf(stderr, "high.gfx: bad framebuffer size %dx%d\n", w, h);
}

/* --------------------------------------------------------- element tree */

const char *bx_gfx_type_name(bx_gfx_type_t t) {
    switch (t) {
    case BX_GFX_TYPE_BUTTON:  return "button";
    case BX_GFX_TYPE_TEXT:    return "text";
    case BX_GFX_TYPE_SLIDER:  return "slider";
    case BX_GFX_TYPE_BOX:     return "box";
    case BX_GFX_TYPE_TEXTBOX: return "textbox";
    case BX_GFX_TYPE_LABEL:   return "label";
    case BX_GFX_TYPE_IMAGE:   return "image";
    case BX_GFX_TYPE_PANEL:   return "panel";
    case BX_GFX_TYPE_SHAPE:   return "shape";
    }
    return "panel";
}

int bx_gfx_type_from_name(const char *name, bx_gfx_type_t *out) {
    if (!name || !out) return -1;
    static const struct { const char *n; bx_gfx_type_t t; } tbl[] = {
        {"button", BX_GFX_TYPE_BUTTON}, {"text", BX_GFX_TYPE_TEXT},
        {"slider", BX_GFX_TYPE_SLIDER}, {"box", BX_GFX_TYPE_BOX},
        {"textbox", BX_GFX_TYPE_TEXTBOX}, {"label", BX_GFX_TYPE_LABEL},
        {"image", BX_GFX_TYPE_IMAGE}, {"panel", BX_GFX_TYPE_PANEL},
        {NULL, BX_GFX_TYPE_PANEL}
    };
    for (int i = 0; tbl[i].n; i++)
        if (ci_equal(tbl[i].n, name)) { *out = tbl[i].t; return 0; }
    return -1;
}

/* -------------------------------------------------------- theme parsing */

/* Parse a "[c..h..r..t..x..b..]" theme. Returns 0 on success.
 *
 * The six tags are positional and must appear in exactly this order. Colors
 * are #rgb/#rgba/#rrggbb/#rrggbbaa, rounding is "$N" for a percentage or a
 * bare number for pixels, transparency is 0-100 with an optional '%'. Any
 * deviation is a parse error, which callers surface by leaving the box empty. */
int bx_gfx_parse_theme(const char *s, bx_gfx_theme_t *out) {
    if (!s || !out) return -1;
    while (*s == ' ' || *s == '\t') s++;

    /* Caret form inherits an already-pushed theme. This never pushes, so
     * repeated inheritance walks further back rather than extending history. */
    if (*s == '^') {
        uint32_t carets = 0;
        while (s[carets] == '^') carets++;
        if (s[carets] != 0) return -1;
        const bx_gfx_theme_t *prev = bx_gfx_theme_history_get(carets);
        if (!prev) return -1;
        *out = *prev;
        return 0;
    }

    if (*s != '[') return -1;
    s++;
    char buf[256];
    size_t bl = 0;
    bx_gfx_theme_t t;
    memset(&t, 0, sizeof t);
    /* Defaults so a partially specified theme still yields sane colors. */
    t.primary_color    = BX_GFX_RGBA(255, 255, 255, 255);
    t.highlight_color  = BX_GFX_RGBA(0, 0, 0, 255);
    t.ternary_color    = BX_GFX_RGBA(0, 0, 0, 255);
    t.quaternary_color = BX_GFX_RGBA(255, 255, 255, 255);
    t.alpha = 100;

    static const char tags[6] = { 'c', 'h', 'r', 't', 'x', 'b' };
    for (int i = 0; i < 6; i++) {
        /* Each field runs to the next '.' or the closing ']'. */
        bl = 0;
        while (*s && *s != '.' && *s != ']') {
            if (bl + 1 >= sizeof buf) return -1;
            buf[bl++] = *s++;
        }
        buf[bl] = 0;
        if (bl < 2) return -1;
        if (buf[0] != tags[i]) return -1;   /* order is part of the format */
        const char *val = buf + 1;

        switch (tags[i]) {
        case 'c': case 'h': case 'x': case 'b': {
            uint32_t c = bx_gfx_parse_color(val);
            if (!c) return -1;
            if (tags[i] == 'c')      t.primary_color = c;
            else if (tags[i] == 'h') t.highlight_color = c;
            else if (tags[i] == 'x') t.ternary_color = c;
            else                     t.quaternary_color = c;
            break;
        }
        case 'r':
            if (*val == '$') { t.is_percentage = 1; val++; }
            else             { t.is_percentage = 0; }
            if (*val < '0' || *val > '9') return -1;
            t.rounding = (int32_t)strtol(val, NULL, 10);
            break;
        case 't':
            if (*val == '%') val++;
            if (*val < '0' || *val > '9') return -1;
            t.alpha = (uint8_t)strtoul(val, NULL, 10);
            if (t.alpha > 100) return -1;
            break;
        }
        if (i < 5) {
            if (*s != '.') return -1;
            s++;
        }
    }
    if (*s != ']') return -1;
    s++;
    while (*s == ' ' || *s == '\t') s++;
    if (*s != 0) return -1;   /* trailing junk is an error */

    t.valid = 1;
    *out = t;
    bx_gfx_theme_history_push(&t);
    return 0;
}

void bx_gfx_theme_history_push(const bx_gfx_theme_t *theme) {
    if (!theme) return;
    /* Keep the newest four. When full, drop the oldest by shifting down. */
    if (g_gfx_theme_history_count == BX_GFX_THEME_HISTORY_MAX) {
        for (int i = 1; i < BX_GFX_THEME_HISTORY_MAX; i++)
            g_gfx_theme_history[i - 1] = g_gfx_theme_history[i];
        g_gfx_theme_history_count--;
    }
    g_gfx_theme_history[g_gfx_theme_history_count++] = *theme;
}

/* carets is the caret count from the theme string: 2 means the newest theme,
 * 3 the one before it, and so on. Returns NULL when that would reach past the
 * oldest theme still held. */
const bx_gfx_theme_t *bx_gfx_theme_history_get(uint32_t carets) {
    if (carets < 2) return NULL;
    uint32_t back = carets - 2;
    if (back >= g_gfx_theme_history_count) return NULL;
    return &g_gfx_theme_history[g_gfx_theme_history_count - 1 - back];
}

/* ------------------------------------------------------- element storage */

bx_gfx_element_t *bx_gfx_find_id(uint32_t my_id) {
    if (my_id == 0) return NULL;
    for (uint32_t i = 0; i < g_bx_gfx.element_count; i++)
        if (g_bx_gfx.elements[i].id == my_id) return &g_bx_gfx.elements[i];
    return NULL;
}

bx_gfx_element_t *bx_gfx_find_element(uint32_t parent_id, uint32_t my_id) {
    bx_gfx_element_t *el = bx_gfx_find_id(my_id);
    if (el && el->parent_id == parent_id) return el;
    return NULL;
}

int bx_gfx_add_element(const bx_gfx_element_t *elem) {
    if (!elem) return -1;
    if (g_bx_gfx.element_count == g_bx_gfx.element_capacity) {
        uint32_t cap = g_bx_gfx.element_capacity ? g_bx_gfx.element_capacity * 2 : 16;
        g_bx_gfx.elements = (bx_gfx_element_t *)xrealloc(g_bx_gfx.elements,
                                        (size_t)cap * sizeof(bx_gfx_element_t));
        g_bx_gfx.element_capacity = cap;
    }
    g_bx_gfx.elements[g_bx_gfx.element_count++] = *elem;
    if (elem->id >= g_bx_gfx.next_auto_id) g_bx_gfx.next_auto_id = elem->id + 1;
    return 0;
}

/* Create an element. my_id of 0 auto-assigns from next_auto_id. Returns 0 on
 * success, -1 for a bad theme or a duplicate id; *out_id gets the real id. */
int bx_gfx_draw(uint32_t parent_id, bx_gfx_type_t type, int32_t x, int32_t y,
                const char *theme_str, uint32_t my_id, uint32_t w, uint32_t h,
                const char *box_name, uint32_t *out_id) {
    if (out_id) *out_id = 0;
    bx_gfx_theme_t theme;
    if (!theme_str) theme_str = "[c#fff.h#000.r$0.t%100.x#000.b#fff]";
    if (bx_gfx_parse_theme(theme_str, &theme) != 0) return -1;

    uint32_t id = my_id;
    if (id == 0) {
        id = g_bx_gfx.next_auto_id;
        if (id == 0) id = 1;
    }
    if (bx_gfx_find_id(id)) return -1;   /* ids must be unique */

    bx_gfx_element_t el;
    memset(&el, 0, sizeof el);
    el.id = id;
    el.parent_id = parent_id;
    el.type = type;
    el.x = x; el.y = y;
    el.width = w; el.height = h;
    el.theme = theme;
    if (box_name && *box_name) el.value_box = xstrdup(box_name);

    if (bx_gfx_add_element(&el) != 0) return -1;
    if (out_id) *out_id = id;
    return 0;
}

/* ------------------------------------------------------------------ styles */

/* Style presets are ordinary themes with friendlier names, so anything that
 * takes a theme string also takes a style name. */
static const char *const gfx_style_names[] = {
    "flat", "dark", "light", "glass", "neon", "warm", "cool", "mono", "solar", NULL
};
static const struct { const char *name; const char *theme; } gfx_styles[] = {
    {"flat",   "[c#ffffff.h#cccccc.r$0.t%0.x#000000.b#ffffff]"},
    {"dark",   "[c#3b82f6.h#1e40af.r$6.t%0.x#e5e7eb.b#111827]"},
    {"light",  "[c#ffffff.h#e5e7eb.r$6.t%20.x#1f2937.b#f3f4f6]"},
    {"glass",  "[c#ffffff7f.h#ffffff3f.r$12.t%40.x#ffffffdf.b#ffffff1f]"},
    {"neon",   "[c#39ff14.h#00e5ff.r$8.t%0.x#f0fff4.b#0a0a12]"},
    {"warm",   "[c#ff9f43.h#ff6b35.r$10.t%0.x#2b1b12.b#ffe8d6]"},
    {"cool",   "[c#48dbfb.h#0abdc3.r$10.t%0.x#0b2027.b#dff9fb]"},
    {"mono",   "[c#e0e0e0.h#a0a0a0.r$0.t%0.x#101010.b#202020]"},
    {"solar",  "[c#ffdd00.h#ff8800.r$14.t%0.x#241a00.b#3a2a00]"},
    {NULL, NULL}
};

const char *bx_gfx_style(const char *name) {
    if (!name) return NULL;
    for (int i = 0; gfx_styles[i].name; i++)
        if (ci_equal(gfx_styles[i].name, name)) return gfx_styles[i].theme;
    return NULL;
}

const char * const *bx_gfx_style_names(int *count) {
    if (count) *count = (int)(sizeof gfx_style_names / sizeof gfx_style_names[0]) - 1;
    return gfx_style_names;
}

/* ------------------------------------------------- friendly shape elements */

int bx_gfx_shape_set(bx_gfx_element_t *el, const char *kind,
                     const int32_t *pts, uint32_t c1, uint32_t c2) {
    if (!el || !kind) return -1;
    static const char *const kinds[] = { "rect", "frame", "line", "circle",
                                        "ring", "tri", "gradv", "gradh", NULL };
    int ok = 0;
    for (int i = 0; kinds[i]; i++) if (ci_equal(kinds[i], kind)) { ok = 1; break; }
    if (!ok) return -1;
    snprintf(el->shape_kind, sizeof el->shape_kind, "%s", kind);
    for (int i = 0; i < 6; i++) el->shape[i] = pts ? pts[i] : 0;
    el->shape_color = c1;
    el->shape_color2 = c2 ? c2 : c1;
    return 0;
}

static int shape_known(const char *k) {
    return strcmp(k, "rect") == 0 || strcmp(k, "frame") == 0 || strcmp(k, "line") == 0 ||
           strcmp(k, "circle") == 0 || strcmp(k, "ring") == 0 || strcmp(k, "tri") == 0 ||
           strcmp(k, "gradv") == 0 || strcmp(k, "gradh") == 0;
}

/* Draw one shape element into the framebuffer. */
int bx_gfx_render_one(uint32_t id) {
    bx_gfx_element_t *el = bx_gfx_find_id(id);
    if (!el) return -1;
    if (el->type != BX_GFX_TYPE_SHAPE) return -1;
    if (!shape_known(el->shape_kind)) return -1;

    bx_gfx_fb_t *fb = bx_gfx_fb_get();
    const int32_t *s = el->shape;
    const char *k = el->shape_kind;
    uint32_t c = el->shape_color;

    if (!strcmp(k, "rect"))
        bx_gfx_rect(fb, s[0], s[1], s[2], s[3], c);
    else if (!strcmp(k, "frame"))
        bx_gfx_rect_outline(fb, s[0], s[1], s[2], s[3], c);
    else if (!strcmp(k, "line"))
        bx_gfx_line(fb, s[0], s[1], s[2], s[3], c);
    else if (!strcmp(k, "circle"))
        bx_gfx_circle(fb, s[0], s[1], s[2], c);
    else if (!strcmp(k, "ring"))
        bx_gfx_circle_outline(fb, s[0], s[1], s[2], c);
    else if (!strcmp(k, "tri"))
        bx_gfx_tri(fb, (float)s[0], (float)s[1], (float)s[2], (float)s[3],
                   (float)s[4], (float)s[5], c);
    else if (!strcmp(k, "gradv"))
        bx_gfx_gradient_v(fb, s[0], s[1], s[2], s[3], c, el->shape_color2);
    else if (!strcmp(k, "gradh"))
        bx_gfx_gradient_h(fb, s[0], s[1], s[2], s[3], c, el->shape_color2);
    return 0;
}

/* Elements are stored in insertion order, which is also id order for the
 * common case, so rendering in array order gives painter's-algorithm output. */
int bx_gfx_render(void) {
    int n = 0;
    for (uint32_t i = 0; i < g_bx_gfx.element_count; i++) {
        if (g_bx_gfx.elements[i].type != BX_GFX_TYPE_SHAPE) continue;
        if (bx_gfx_render_one(g_bx_gfx.elements[i].id) == 0) n++;
    }
    return n;
}

int bx_gfx_ppm(const char *path) {
    bx_gfx_fb_t *fb = bx_gfx_fb_get();
    if (!path || !*path || !fb->pixels) return -1;
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    fprintf(f, "P6\n%d %d\n255\n", fb->width, fb->height);
    for (int32_t y = 0; y < fb->height; y++) {
        for (int32_t x = 0; x < fb->width; x++) {
            uint32_t c = fb->pixels[(size_t)y * (size_t)fb->width + (size_t)x];
            /* Bytes are 8-bit, so drop the alpha byte. */
            uint8_t rgb[3] = { BX_GFX_R(c), BX_GFX_G(c), BX_GFX_B(c) };
            fwrite(rgb, 1, 3, f);
        }
    }
    int rc = ferror(f) ? -1 : 0;
    fclose(f);
    return rc;
}

char *bx_gfx_ascii(int32_t x, int32_t y, int32_t w, int32_t h, const char *ramp) {
    bx_gfx_fb_t *fb = bx_gfx_fb_get();
    if (!ramp || !*ramp) return NULL;
    int32_t n = (int32_t)strlen(ramp);
    if (w <= 0 || h <= 0) return NULL;
    size_t need = (size_t)w * (size_t)h + (size_t)h;   /* rows + newlines */
    char *out = (char *)xmalloc(need + 1);
    size_t o = 0;
    for (int32_t yy = 0; yy < h; yy++) {
        for (int32_t xx = 0; xx < w; xx++) {
            uint32_t c = bx_gfx_get(fb, x + xx, y + yy);
            /* Rec. 601 luma, so the ramp tracks perceived brightness. */
            /* Rec. 601 luma weights sum to 256, so the dot product tops out at
             * 65280 and needs /65536 to land in 0..255. */
            int32_t luma = (int32_t)BX_GFX_R(c) * 77 + (int32_t)BX_GFX_G(c) * 150 +
                           (int32_t)BX_GFX_B(c) * 29;
            int32_t idx = (luma * n) / 65536;
            if (idx > n - 1) idx = n - 1;
            out[o++] = ramp[idx];
        }
        out[o++] = '\n';
    }
    out[o] = 0;
    return out;
}
