/* BX GFX - software rasterizer, color model, and UI element tree.
 *
 * Two layers live here:
 *   1. A 32-bit RGBA software rasterizer (framebuffer, primitives, blending,
 *      gradients, clipping, transforms). This is the "advanced" surface and it
 *      is fully host-independent, so it works in the interpreter, in the
 *      transpiler, and on the bootloader.
 *   2. The UI element tree used by the high.gfx.* commands.
 *
 * Pixel format is 0xRRGGBBAA, matching bx_gfx_parse_color so colors parsed
 * from "#rgb" strings can be handed straight to the rasterizer.
 */
#ifndef BX_GFX_H
#define BX_GFX_H

#include <stdint.h>
#include <stddef.h>

/* ------------------------------------------------------------------ colors */

/* Pixel format: 0xRRGGBBAA, so a color parsed from "#rrggbb" can be passed
 * straight through to the command layer and printed as a signed int. Alpha
 * defaults to 0xFF (opaque) when a color string omits it. */
#define BX_GFX_RGBA(r, g, b, a) \
    ((uint32_t)(((uint32_t)(r) << 24) | ((uint32_t)(g) << 16) | \
                ((uint32_t)(b) << 8)  |  (uint32_t)(a)))

#define BX_GFX_R(c) ((uint8_t)(((c) >> 24) & 0xFF))
#define BX_GFX_G(c) ((uint8_t)(((c) >> 16) & 0xFF))
#define BX_GFX_B(c) ((uint8_t)(((c) >> 8)  & 0xFF))
#define BX_GFX_A(c) ((uint8_t)( (c)        & 0xFF))

/* Opaque shorthand, used constantly by the primitive helpers. */
#define BX_GFX_OPAQUE(r, g, b) BX_GFX_RGBA((r), (g), (b), 255)

/* Linear interpolation between two packed colors. t is 0..256 where 256 is
 * the far end, so callers can use integer math without floating point. */
uint32_t bx_gfx_color_lerp(uint32_t c0, uint32_t c1, int32_t t);

/* Multiply a color's alpha by a 0..256 factor, clamped. */
uint32_t bx_gfx_color_scale_alpha(uint32_t c, int32_t factor);

/* Named CSS-style colors. Returns opaque black when unknown and clears *found
 * so callers can report a useful error. */
uint32_t bx_gfx_color_named(const char *name, int *found);

/* Parse "#rgb", "#rgba", "#rrggbb", or "#rrggbbaa". Returns 0 if unparsable.
 * Short forms expand each nibble, so "#f80" is "#ff8800". */
uint32_t bx_gfx_parse_color(const char *str);

/* ------------------------------------------------------------------ styles */

/* Named style presets. Returns the theme string, or NULL when unknown. */
const char *bx_gfx_style(const char *name);
const char * const *bx_gfx_style_names(int *count);

/* ------------------------------------------------------------- framebuffer */

typedef struct {
    uint32_t *pixels;      /* w*h, one 0xRRGGBBAA word per pixel */
    int32_t  width, height;
    int32_t  clip_x, clip_y, clip_w, clip_h;   /* inclusive-exclusive rect */
    /* Inverse transform applied to every primitive, as a 3x3 matrix. Identity
     * unless push/pop/translate/scale/rotate are used. */
    float    m[9];
} bx_gfx_fb_t;

int  bx_gfx_fb_init(bx_gfx_fb_t *fb, int32_t w, int32_t h, uint32_t fill);
void bx_gfx_fb_free(bx_gfx_fb_t *fb);
void bx_gfx_fb_clear(bx_gfx_fb_t *fb, uint32_t color);

void bx_gfx_clip_set(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h);
void bx_gfx_clip_reset(bx_gfx_fb_t *fb);

/* Transforms. push saves the current matrix, pop restores it. All matrix math
 * is float so rotation and scale stay accurate; the rasterizer only needs
 * fixed-point at plot time. */
void bx_gfx_push(bx_gfx_fb_t *fb);
void bx_gfx_pop(bx_gfx_fb_t *fb);
void bx_gfx_identity(bx_gfx_fb_t *fb);
void bx_gfx_translate(bx_gfx_fb_t *fb, float dx, float dy);
void bx_gfx_scale(bx_gfx_fb_t *fb, float sx, float sy);
void bx_gfx_rotate(bx_gfx_fb_t *fb, float degrees);
void bx_gfx_transform_point(const bx_gfx_fb_t *fb, float x, float y, float *ox, float *oy);

/* ------------------------------------------------------------------ pixels */

void     bx_gfx_plot(bx_gfx_fb_t *fb, int32_t x, int32_t y, uint32_t c);
uint32_t bx_gfx_get(bx_gfx_fb_t *fb, int32_t x, int32_t y);

/* -------------------------------------------------------------- primitives */

void bx_gfx_line(bx_gfx_fb_t *fb, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t c);
void bx_gfx_rect(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c);
void bx_gfx_rect_outline(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c);
void bx_gfx_circle(bx_gfx_fb_t *fb, int32_t cx, int32_t cy, int32_t r, uint32_t c);
void bx_gfx_circle_outline(bx_gfx_fb_t *fb, int32_t cx, int32_t cy, int32_t r, uint32_t c);
void bx_gfx_tri(bx_gfx_fb_t *fb, float x0, float y0, float x1, float y1,
                float x2, float y2, uint32_t c);
void bx_gfx_tri_outline(bx_gfx_fb_t *fb, float x0, float y0, float x1, float y1,
                        float x2, float y2, uint32_t c);
void bx_gfx_poly(bx_gfx_fb_t *fb, const float *pts, int32_t count, uint32_t c);

/* Text, from the VGA 8x8 set in bx_font8x8.h. The glyphs are 8x8 with no
 * inter-character spacing, which is what makes bx_gfx_text_w exact. */
void     bx_gfx_text(bx_gfx_fb_t *fb, int32_t x, int32_t y, const char *s, uint32_t c);
int32_t  bx_gfx_text_w(const char *s);
int32_t  bx_gfx_text_h(void);
/* Same, but a 1px outline in `edge` behind `c`, for text over a busy fill. */
void     bx_gfx_text_outlined(bx_gfx_fb_t *fb, int32_t x, int32_t y,
                              const char *s, uint32_t c, uint32_t edge);
void bx_gfx_gradient_v(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h,
                       uint32_t top, uint32_t bottom);
void bx_gfx_gradient_h(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h,
                       uint32_t left, uint32_t right);

/* --------------------------------------------------------- UI element tree */

typedef enum {
    BX_GFX_TYPE_BUTTON = 0,
    BX_GFX_TYPE_TEXT = 1,
    BX_GFX_TYPE_SLIDER = 2,
    BX_GFX_TYPE_BOX = 3,
    BX_GFX_TYPE_TEXTBOX = 4,
    BX_GFX_TYPE_LABEL = 5,
    BX_GFX_TYPE_IMAGE = 6,
    BX_GFX_TYPE_PANEL = 7,
    BX_GFX_TYPE_SHAPE = 8   /* friendly, rasterizable shape element */
} bx_gfx_type_t;

/* Parsed theme structure */
typedef struct {
    uint32_t primary_color;     /* c - primary color */
    uint32_t highlight_color;   /* h - highlight/secondary color */
    uint32_t ternary_color;     /* x - ternary color (text) */
    uint32_t quaternary_color;  /* b - quaternary color (border/bg) */
    int32_t rounding;           /* r - radius in pixels (negative = percentage) */
    uint8_t is_percentage;      /* rounding is percentage if 1 */
    uint8_t alpha;              /* t - transparency 0-100 */
    uint8_t valid;              /* theme was parsed successfully */
} bx_gfx_theme_t;

/* UI element */
typedef struct {
    uint32_t id;
    uint32_t parent_id;
    bx_gfx_type_t type;
    int32_t x, y;
    uint32_t width, height;
    bx_gfx_theme_t theme;
    char *text;           /* for text/button/label */
    char *value_box;      /* box to store result (for textbox/slider) */
    void *user_data;      /* platform-specific */

    /* Shape description, used by the friendly element-based layer. The
     * meaning of the six coordinates depends on shape_kind:
     *   rect/frame  x,y = top left,      p1 = width,  height
     *   gradv/gradh x,y = top left,      p1 = width,  height
     *   line        x,y = start,         p1 = end
     *   circle/ring x,y = center,        p1 = radius
     *   tri         x,y,p1,p2 = vertices
     * Colors are primary and, for gradients, an optional second stop. */
    char     shape_kind[16];
    int32_t  shape[6];
    uint32_t shape_color, shape_color2;
} bx_gfx_element_t;

/* Theme history for caret inheritance (max 4) */
#define BX_GFX_THEME_HISTORY_MAX 4
extern bx_gfx_theme_t g_gfx_theme_history[BX_GFX_THEME_HISTORY_MAX];
extern uint32_t g_gfx_theme_history_count;

typedef struct {
    bx_gfx_element_t *elements;
    uint32_t element_count;
    uint32_t element_capacity;
    uint32_t next_auto_id;
} bx_gfx_ctx_t;

extern bx_gfx_ctx_t g_bx_gfx;

/* The default drawing target. Created on demand by bx_gfx_fb_get. */
bx_gfx_fb_t *bx_gfx_fb_get(void);
void         bx_gfx_fb_set_size(int32_t w, int32_t h);

/* Theme parsing */
int bx_gfx_parse_theme(const char *theme_str, bx_gfx_theme_t *out_theme);
void bx_gfx_theme_history_push(const bx_gfx_theme_t *theme);
const bx_gfx_theme_t *bx_gfx_theme_history_get(uint32_t carets);

/* GFX commands. my_id may be 0 to auto-assign; w/h are the element size.
 * The id that was actually used is written to *out_id. */
int bx_gfx_draw(uint32_t parent_id, bx_gfx_type_t type, int32_t x, int32_t y,
                const char *theme_str, uint32_t my_id, uint32_t w, uint32_t h,
                const char *box_name, uint32_t *out_id);

/* Element management */
bx_gfx_element_t *bx_gfx_find_element(uint32_t parent_id, uint32_t my_id);
bx_gfx_element_t *bx_gfx_find_id(uint32_t my_id);
int bx_gfx_add_element(const bx_gfx_element_t *elem);

/* ------------------------------------------------- friendly shape elements */

int  bx_gfx_shape_set(bx_gfx_element_t *el, const char *kind,
                      const int32_t *pts, uint32_t c1, uint32_t c2);
int  bx_gfx_render(void);          /* rasterize every shape element, in id order */
int  bx_gfx_render_one(uint32_t id);

/* Write the framebuffer as a binary PPM (P6). Returns 0 on success. */
int  bx_gfx_ppm(const char *path);
/* Luminance ramp of a region, as a heap string the caller must free. Uses the
 * caller-supplied ramp characters, ordered dark to light. */
char *bx_gfx_ascii(int32_t x, int32_t y, int32_t w, int32_t h, const char *ramp);


/* Element type name lookup, shared by the command layer. */
const char *bx_gfx_type_name(bx_gfx_type_t t);
int bx_gfx_type_from_name(const char *name, bx_gfx_type_t *out);

#endif
