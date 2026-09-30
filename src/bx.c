// BoxedLANG C runner, transpiler, and compiler driver.
// Build: make
// Usage:
//   ./bx run file.bx
//   ./bx transpile file.bx -o out.c
//   ./bx compile file.bx -o program
//   ./bx emit-c file.bx

#define _POSIX_C_SOURCE 200809L
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#if defined(BX_EMBEDDED_SOURCE)
#include <stdint.h>
enum { BX_WIFI_SEC_OPEN = 0, BX_WIFI_ENV_BAREMETAL = 0, BX_WIFI_ENV_NATIVE = 1, BX_WIFI_ENV_WEB = 2 };
typedef int bx_wifi_env_t;
typedef int bx_wifi_sec_t;
static int bx_wifi_init(void){ return 0; }
static int bx_wifi_detect_env(void){ return 1; }  /* NATIVE */
static int bx_wifi_creq(const char *ssid, bx_wifi_sec_t sec, const char *pass){ (void)ssid; (void)sec; (void)pass; return 0; }
static void bx_wifi_status(char *out_box, size_t box_size){ snprintf(out_box, box_size, "state=DISCONNECTED env=NATIVE"); }
static int bx_wifi_disconnect(void){ return 0; }
static uint32_t bx_gfx_parse_color(const char *str){ (void)str; return 0; }
/* Minimal GFX surface for transpiled output: the element tree and the
 * framebuffer are stubs, so high.gfx.* parses and stores ids but cannot draw.
 * The signatures mirror bx_gfx.h so the interpreter command layer compiles
 * unchanged when shipped to a target without the native rasterizer. */
typedef enum {
    BX_GFX_TYPE_BUTTON=0, BX_GFX_TYPE_TEXT=1, BX_GFX_TYPE_SLIDER=2,
    BX_GFX_TYPE_BOX=3, BX_GFX_TYPE_TEXTBOX=4, BX_GFX_TYPE_LABEL=5,
    BX_GFX_TYPE_IMAGE=6, BX_GFX_TYPE_PANEL=7, BX_GFX_TYPE_SHAPE=8
} bx_gfx_type_t;
typedef struct { uint32_t primary_color, highlight_color, ternary_color, quaternary_color; int32_t rounding; uint8_t is_percentage, alpha, valid; } bx_gfx_theme_t;
typedef struct { uint32_t id, parent_id; bx_gfx_type_t type; int32_t x, y; uint32_t width, height; bx_gfx_theme_t theme; char *text, *value_box; void *user_data; char shape_kind[16]; int32_t shape[6]; uint32_t shape_color, shape_color2; } bx_gfx_element_t;
typedef struct { bx_gfx_element_t *elements; uint32_t element_count, element_capacity, next_auto_id; } bx_gfx_ctx_t;
typedef struct { uint32_t *pixels; int32_t width, height; int32_t clip_x, clip_y, clip_w, clip_h; float m[9]; } bx_gfx_fb_t;
#define BX_GFX_RGBA(r,g,b,a) ((uint32_t)(((uint32_t)(a)<<24)|((uint32_t)(r)<<16)|((uint32_t)(g)<<8)|(uint32_t)(b)))
#define BX_GFX_R(c) ((uint8_t)(((c)>>24)&0xFF))
#define BX_GFX_G(c) ((uint8_t)(((c)>>16)&0xFF))
#define BX_GFX_B(c) ((uint8_t)(((c)>>8)&0xFF))
#define BX_GFX_A(c) ((uint8_t)((c)&0xFF))
static uint32_t bx_gfx_color_named(const char *name, int *found){ if(found)*found=0; (void)name; return BX_GFX_RGBA(0,0,0,255); }
static uint32_t bx_gfx_color_lerp(uint32_t c0, uint32_t c1, int32_t t){ (void)t; return c1; }
static uint32_t bx_gfx_color_scale_alpha(uint32_t c, int32_t f){ (void)f; return c; }
static bx_gfx_ctx_t g_bx_gfx;
static int bx_gfx_parse_theme(const char *s, bx_gfx_theme_t *t){ (void)s; if(t) memset(t,0,sizeof *t); return -1; }
static void bx_gfx_theme_history_push(const bx_gfx_theme_t *t){ (void)t; }
static int bx_gfx_draw(uint32_t parent, bx_gfx_type_t type, int32_t x, int32_t y, const char *theme, uint32_t id, uint32_t w, uint32_t h, const char *box, uint32_t *out){ (void)type;(void)x;(void)y;(void)theme;(void)w;(void)h;(void)box;(void)parent; if(!id) id=g_bx_gfx.next_auto_id++; g_bx_gfx.element_count++; if(out)*out=id; return 0; }
static bx_gfx_element_t *bx_gfx_find_id(uint32_t id){ (void)id; return NULL; }
static bx_gfx_element_t *bx_gfx_find_element(uint32_t p, uint32_t i){ (void)p;(void)i; return NULL; }
static bx_gfx_fb_t g_embedded_fb;
static bx_gfx_fb_t *bx_gfx_fb_get(void){ static int ready=0; if(!ready){ memset(&g_embedded_fb,0,sizeof g_embedded_fb); ready=1; } return &g_embedded_fb; }
static void bx_gfx_fb_set_size(int32_t w, int32_t h){ (void)w;(void)h; }
static void bx_gfx_fb_clear(bx_gfx_fb_t *fb, uint32_t c){ (void)fb;(void)c; }
static void bx_gfx_clip_set(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h){ (void)fb;(void)x;(void)y;(void)w;(void)h; }
static void bx_gfx_clip_reset(bx_gfx_fb_t *fb){ (void)fb; }
static void bx_gfx_push(bx_gfx_fb_t *fb){ (void)fb; }
static void bx_gfx_pop(bx_gfx_fb_t *fb){ (void)fb; }
static void bx_gfx_identity(bx_gfx_fb_t *fb){ (void)fb; }
static void bx_gfx_translate(bx_gfx_fb_t *fb, float dx, float dy){ (void)fb;(void)dx;(void)dy; }
static void bx_gfx_scale(bx_gfx_fb_t *fb, float sx, float sy){ (void)fb;(void)sx;(void)sy; }
static void bx_gfx_rotate(bx_gfx_fb_t *fb, float d){ (void)fb;(void)d; }
static void bx_gfx_plot(bx_gfx_fb_t *fb, int32_t x, int32_t y, uint32_t c){ (void)fb;(void)x;(void)y;(void)c; }
static uint32_t bx_gfx_get(bx_gfx_fb_t *fb, int32_t x, int32_t y){ (void)fb;(void)x;(void)y; return 0; }
static void bx_gfx_line(bx_gfx_fb_t *fb, int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t c){ (void)fb;(void)x0;(void)y0;(void)x1;(void)y1;(void)c; }
static void bx_gfx_rect(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c){ (void)fb;(void)x;(void)y;(void)w;(void)h;(void)c; }
static void bx_gfx_rect_outline(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t c){ (void)fb;(void)x;(void)y;(void)w;(void)h;(void)c; }
static void bx_gfx_circle(bx_gfx_fb_t *fb, int32_t cx, int32_t cy, int32_t r, uint32_t c){ (void)fb;(void)cx;(void)cy;(void)r;(void)c; }
static void bx_gfx_circle_outline(bx_gfx_fb_t *fb, int32_t cx, int32_t cy, int32_t r, uint32_t c){ (void)fb;(void)cx;(void)cy;(void)r;(void)c; }
static void bx_gfx_tri(bx_gfx_fb_t *fb, float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c){ (void)fb;(void)x0;(void)y0;(void)x1;(void)y1;(void)x2;(void)y2;(void)c; }
static void bx_gfx_tri_outline(bx_gfx_fb_t *fb, float x0, float y0, float x1, float y1, float x2, float y2, uint32_t c){ (void)fb;(void)x0;(void)y0;(void)x1;(void)y1;(void)x2;(void)y2;(void)c; }
static void bx_gfx_poly(bx_gfx_fb_t *fb, const float *pts, int32_t count, uint32_t c){ (void)fb;(void)pts;(void)count;(void)c; }
static void bx_gfx_gradient_v(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t a, uint32_t b){ (void)fb;(void)x;(void)y;(void)w;(void)h;(void)a;(void)b; }
static void bx_gfx_gradient_h(bx_gfx_fb_t *fb, int32_t x, int32_t y, int32_t w, int32_t h, uint32_t a, uint32_t b){ (void)fb;(void)x;(void)y;(void)w;(void)h;(void)a;(void)b; }
static const char *bx_gfx_style(const char *name){ (void)name; return NULL; }
static const char * const *bx_gfx_style_names(int *count){ static const char *const none[1]={NULL}; if(count)*count=0; return none; }
static int bx_gfx_shape_set(bx_gfx_element_t *el, const char *kind, const int32_t *pts, uint32_t c1, uint32_t c2){ (void)el;(void)kind;(void)pts;(void)c1;(void)c2; return -1; }
static int bx_gfx_render(void){ return 0; }
static int bx_gfx_render_one(uint32_t id){ (void)id; return -1; }
static int bx_gfx_ppm(const char *path){ (void)path; return -1; }
static char *bx_gfx_ascii(int32_t x, int32_t y, int32_t w, int32_t h, const char *ramp){ (void)x;(void)y;(void)w;(void)h;(void)ramp; return NULL; }
/* Minimal SND surface for transpiled output: voices never render, so
 * high.snd.* parses but produces silence. Signatures mirror bx_snd.h. */
typedef enum { BX_SND_WAVE_SINE=0, BX_SND_WAVE_SQUARE=1, BX_SND_WAVE_SAW=2,
               BX_SND_WAVE_TRI=3, BX_SND_WAVE_NOISE=4 } bx_snd_wave_t;
typedef struct { uint32_t rate; int32_t voice_count; } bx_snd_ctx_t;
static bx_snd_ctx_t g_bx_snd;
static void bx_snd_init(uint32_t rate){ g_bx_snd.rate=rate?rate:22050; g_bx_snd.voice_count=0; }
/* No libm dependency in the embedded surface, so step 440 Hz up/down. */
static double bx_snd_note_freq(int midi){ double f=440.0; int s=midi-69;
    while(s>0){ f*=1.0594630943592953; s--; } while(s<0){ f/=1.0594630943592953; s++; } return f; }
static int bx_snd_note_from_name(const char *name){ (void)name; return -1; }
static int bx_snd_tone(bx_snd_wave_t w,double f,double amp,double dur,double A,double D,double S,double R){ (void)w;(void)f;(void)amp;(void)dur;(void)A;(void)D;(void)S;(void)R; return -1; }
static int bx_snd_note(bx_snd_wave_t w,int m,double amp,double dur,double A,double D,double S,double R){ (void)w;(void)m;(void)amp;(void)dur;(void)A;(void)D;(void)S;(void)R; return -1; }
static int bx_snd_melody(const char*p,double d,bx_snd_wave_t w,double amp,double o,double A,double D,double S,double R){ (void)p;(void)d;(void)w;(void)amp;(void)o;(void)A;(void)D;(void)S;(void)R; return -1; }
static int bx_snd_clear(void){ g_bx_snd.voice_count=0; return 0; }
static int bx_snd_voice_count(void){ return g_bx_snd.voice_count; }
static uint32_t bx_snd_render(void){ return 0; }
static uint32_t bx_snd_samples(void){ return 0; }
static uint32_t bx_snd_rate(void){ return g_bx_snd.rate?g_bx_snd.rate:22050; }
static int bx_snd_wav(const char *p){ (void)p; return -1; }
#else
#include "bx_wifi.h"
#include "bx_gfx.h"
#include "bx_ui.h"
#include "bx_snd.h"
#include "bx_math.h"
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define BX_ARCH "x86_64"
#elif defined(__aarch64__)
#define BX_ARCH "aarch64"
#elif defined(__arm__)
#define BX_ARCH "arm"
#elif defined(__riscv)
#define BX_ARCH "riscv"
#else
#define BX_ARCH "unknown"
#endif


#ifndef BX_RUNTIME_PATH
#define BX_RUNTIME_PATH "src/bx.c"
#endif
#define BX_VERSION "0.2.0"

typedef struct { char *name; char *value; int inl; char ibuf[16]; long ival; int has_ival; } Box;
typedef struct { Box *items; size_t len, cap; } Boxes;
typedef struct { char *name; int line; } Mark;
typedef struct { Mark *items; size_t len, cap; } Marks;
typedef enum { OP_BOX, OP_SAY, OP_MATH, OP_TEST, OP_IF, OP_JUMP, OP_JUMPIF, OP_DEL, OP_END, OP_PREMARK } OpKind;
typedef struct { OpKind kind; int n; char **parts; } Op;
typedef struct {
    char **lines;
    char **comments;
    int count;
    Boxes boxes;
    Marks marks;
    int halted;
    Op **ops;
} Program;

static char *xstrdup(const char *s) { size_t n = strlen(s) + 1; char *p = malloc(n); if (!p) { perror("malloc"); exit(1); } memcpy(p, s, n); return p; }
static char *xstrndup(const char *s, size_t n) { char *p = malloc(n + 1); if (!p) { perror("malloc"); exit(1); } memcpy(p, s, n); p[n] = 0; return p; }
static void *xrealloc(void *p, size_t n) { void *q = realloc(p, n); if (!q) { perror("realloc"); exit(1); } return q; }

static char *trim(char *s) { while (isspace((unsigned char)*s)) s++; char *e = s + strlen(s); while (e > s && isspace((unsigned char)e[-1])) *--e = 0; return s; }
static int streqi(const char *a, const char *b) { while (*a && *b) { if (tolower((unsigned char)*a++) != tolower((unsigned char)*b++)) return 0; } return *a == 0 && *b == 0; }
static int is_number(const char *s) { if (*s == '-' || *s == '+') s++; if (!*s) return 0; while (*s) if (!isdigit((unsigned char)*s++)) return 0; return 1; }

static char *strip_comment(const char *line) {
    for (size_t i = 0; line[i]; i++) if (line[i] == '/' && line[i+1] == '/') return xstrndup(line, i);
    return xstrdup(line);
}

static char **split_bars(const char *s, int *out_n) {
    int cap = 8, n = 0; char **parts = malloc(sizeof(char*) * cap); if (!parts) exit(1);
    const char *start = s;
    for (const char *p = s;; p++) {
        if (*p == '|' || *p == 0) {
            if (n == cap) { cap *= 2; parts = xrealloc(parts, sizeof(char*) * cap); }
            parts[n++] = xstrndup(start, (size_t)(p - start));
            if (*p == 0) break;
            start = p + 1;
        }
    }
    *out_n = n; return parts;
}
static void free_parts(char **p, int n) { for (int i = 0; i < n; i++) free(p[i]); free(p); }

/* Split on a single delimiter character, like split_bars but for commas.
 * Empty fields are dropped so "a, b ,, c" yields a, b, c. */
static char **split_char(const char *s, char delim, int *out_n) {
    int n = 0, cap = 8;
    char **out = malloc(sizeof(char *) * (size_t)cap);
    const char *start = s;
    for (const char *p = s;; p++) {
        if (*p == delim || *p == 0) {
            size_t len = (size_t)(p - start);
            while (len && (start[0] == ' ' || start[0] == '\t')) { start++; len--; }
            while (len && (start[len-1] == ' ' || start[len-1] == '\t' || start[len-1] == '\r')) len--;
            if (len) {
                if (n == cap) { cap *= 2; out = xrealloc(out, sizeof(char *) * (size_t)cap); }
                out[n++] = xstrndup(start, len);
            }
            if (*p == 0) break;
            start = p + 1;
        }
    }
    *out_n = n;
    return out;
}

static void box_patch(Boxes *b){ for(size_t i=0;i<b->len;i++) if(b->items[i].inl) b->items[i].value=b->items[i].ibuf; }
/* Pure lookup. This deliberately does NOT reorder the array.
 *
 * A move-to-front heuristic is tempting, but box values are returned as raw
 * pointers into the Box structs (Box.ibuf is an inline buffer), and the
 * interpreter routinely holds such a pointer across another box operation --
 * e.g. `str|$out|take|$text|$n` resolves $text, then calls bx_int for $n,
 * then reads $text again. Any reordering in between silently changes which
 * box that pointer refers to, which produced wrong results and out-of-bounds
 * reads. Lookup is a short linear scan over a small table, so keep it stable. */
static int box_index(Boxes *b, const char *name) {
    for (size_t i = 0; i < b->len; i++)
        if (!strcmp(b->items[i].name, name)) return (int)i;
    return -1;
}
static void box_set_value(Box *bx, const char *value){
    if(bx->inl){ /* inline: nothing to free */ }
    else if(bx->value) free(bx->value);
    size_t l=strlen(value);
    bx->has_ival = is_number(value);
    bx->ival = bx->has_ival ? atol(value) : 0;
    if(l < sizeof bx->ibuf){ memcpy(bx->ibuf, value, l+1); bx->inl=1; bx->value=bx->ibuf; }
    else { bx->value=xstrdup(value); bx->inl=0; }
}
/* Read-only lookup (see box_index for why the array must stay stable). */
#define box_find(b, name) box_index((b), (name))
static const char *box_get(Boxes *b, const char *name) { int i = box_index(b, name); return i >= 0 ? b->items[i].value : ""; }
static void box_set(Boxes *b, const char *name, const char *value) {
    /* Callers often pass a name or value that came straight out of another box
     * (via rfast/box_get). Resolving the other argument can reallocate or
     * relocate the array, so copy first when the pointer points into it. */
    char nbuf[64], vbuf[64];
    const char *safe_name = name, *safe = value;
    if (b->items) {
        const char *lo = (const char *)b->items;
        const char *hi = lo + b->cap * sizeof(Box);
        if (name && name >= lo && name < hi) { snprintf(nbuf, sizeof nbuf, "%s", name); safe_name = nbuf; }
        if (value && value >= lo && value < hi) { snprintf(vbuf, sizeof vbuf, "%s", value); safe = vbuf; }
    }
    int i = box_index(b, safe_name);
    if (i >= 0) {
        if (b->items[i].value == safe) return;
        box_set_value(&b->items[i], safe); return; }
    if (b->len == b->cap) { b->cap = b->cap ? b->cap * 2 : 16; b->items = xrealloc(b->items, b->cap * sizeof(Box)); box_patch(b); }
    b->items[b->len].name = xstrdup(safe_name); b->items[b->len].ibuf[0]=0; b->items[b->len].value=NULL; b->items[b->len].inl=0; box_set_value(&b->items[b->len], safe); b->len++;
}
static void box_del(Boxes *b, const char *name) {
    int i = box_index(b, name); if (i < 0) return;
    free(b->items[i].name); if(!b->items[i].inl && b->items[i].value) free(b->items[i].value);
    memmove(&b->items[i], &b->items[i+1], sizeof(Box) * (b->len - (size_t)i - 1)); b->len--; box_patch(b);
}
static void boxes_free(Boxes *b) { for (size_t i = 0; i < b->len; i++) { free(b->items[i].name); if(!b->items[i].inl && b->items[i].value) free(b->items[i].value); } free(b->items); }

static void mark_add(Marks *m, const char *name, int line) {
    /* Duplicate mark names are almost always a mistake: mark_find returns the
     * first match, so a second `premark foo` makes jumps to `foo` silently
     * resume at the earlier block instead. Warn instead of failing so existing
     * programs that rely on the first-match behaviour keep working. */
    for (size_t i = 0; i < m->len; i++)
        if (!strcmp(m->items[i].name, name)) {
            fprintf(stderr, "warning: duplicate premark '%s' at line %d (first defined at line %d); jumps will use the first\n", name, line + 1, m->items[i].line + 1);
            return;
        }
    if (m->len == m->cap) { m->cap = m->cap ? m->cap * 2 : 16; m->items = xrealloc(m->items, sizeof(Mark) * m->cap); }
    m->items[m->len].name = xstrdup(name); m->items[m->len].line = line; m->len++;
}
static int mark_find(Marks *m, const char *name) { for (size_t i = 0; i < m->len; i++) if (!strcmp(m->items[i].name, name)) return m->items[i].line; return -1; }
static void marks_free(Marks *m) { for (size_t i = 0; i < m->len; i++) free(m->items[i].name); free(m->items); }

static int bx_var_char(unsigned char c) { return isalnum(c) || c == '_' || c == '-' || c == '?' || c == '#'; }
static int stdout_is_tty(void){ static int v=-1; if(v<0) v=isatty(1); return v; }

static char *resolve(Boxes *boxes, const char *s) {
    size_t cap = strlen(s) + 64, len = 0; char *out = malloc(cap); if (!out) exit(1); out[0] = 0;
    for (size_t i = 0; s[i];) {
        const char *rep = NULL; char *owned = NULL; char chbuf[2] = {0, 0};
        if (s[i] == '\\' && s[i+1] == '/' && s[i+2] == ':') { rep = ":"; i += 3; }
        else if (s[i] == ':' && s[i+1] == '$') { i++; continue; }
        else if (s[i] == '$' && s[i+1] == '$') {
            i += 2; size_t st = i; while (bx_var_char((unsigned char)s[i])) i++;
            owned = xstrndup(s + st, i - st); rep = box_get(boxes, box_get(boxes, owned));
        } else if (s[i] == '$') {
            i++; size_t st = i; while (bx_var_char((unsigned char)s[i])) i++;
            owned = xstrndup(s + st, i - st); rep = box_get(boxes, owned);
        } else if (s[i] == '\\' && s[i+1] == '(') {
            /* Explicit interpolation: \(name\). Unlike a bare $name, the name
             * stops at the closing paren, so trailing literal text is safe --
             * this is what makes names like d_\(i\)_0 build correctly. */
            i += 2; size_t st = i; while (bx_var_char((unsigned char)s[i])) i++;
            owned = xstrndup(s + st, i - st);
            if (s[i] == '\\' && s[i+1] == ')') i += 2;
            rep = box_get(boxes, owned);
        } else if (s[i] == '\\' && s[i+1] == ')') { chbuf[0] = ')'; i += 2; rep = chbuf; }
        else { chbuf[0] = s[i++]; rep = chbuf; }
        size_t rn = strlen(rep); if (len + rn + 1 > cap) { while (len + rn + 1 > cap) cap *= 2; out = xrealloc(out, cap); }
        memcpy(out + len, rep, rn); len += rn; out[len] = 0; free(owned);
    }
    return out;
}

typedef struct { const char *ptr; int owned; } Res;
static Res rfast(Boxes *b, const char *s) {
    const unsigned char *p=(const unsigned char*)s;
    while(*p){ unsigned char c=*p; if(c=='$'||c=='\\'||c==':') break; p++; }
    if(!*p){ Res r={(const char*)s,0}; return r; }
    if(s[0]=='$' && s[1] && s[1]!='$'){
        const char *q=s+1; while(bx_var_char((unsigned char)*q)) q++;
        if(!*q){ Res r={box_get(b,s+1),0}; return r; }
    }
    Res r={resolve(b,s),1}; return r;
}
static long bx_int(Boxes *b, const char *s) {
    int idx = -1;
    if(s[0]=='$' && s[1] && s[1]!='$'){ const char*q=s+1; while(bx_var_char((unsigned char)*q)) q++; if(!*q){ idx=box_index(b,s+1); if(idx>=0 && b->items[idx].has_ival) return b->items[idx].ival; } }
    Res r=rfast(b,s); long v=strtol(r.ptr,NULL,10); if(r.owned) free((char*)r.ptr); return v;
}
/* Normalize a comparison operator: === and !== are accepted as spellings of
 * == and !=, so code written either way behaves the same. */
static const char *cmp_op_norm(const char *op) {
    if (!strcmp(op, "===")) return "==";
    if (!strcmp(op, "!==")) return "!=";
    return op;
}

static int cmp_values(const char *left, const char *op0, const char *right) {
    const char *op = cmp_op_norm(op0);
    if (!strcmp(op, "==")) return strcmp(left, right) == 0;
    if (!strcmp(op, "!=")) return strcmp(left, right) != 0;
    long a = strtol(left, NULL, 10), c = strtol(right, NULL, 10);
    if (!strcmp(op, ">")) return a > c;
    if (!strcmp(op, "<")) return a < c;
    if (!strcmp(op, ">=")) return a >= c;
    if (!strcmp(op, "<=")) return a <= c;
    return 0;
}
static int cond_num(Boxes *b, const char *s, long *out){
    if(is_number(s)){ if(out)*out=atol(s); return 1; }
    if(s[0]=='$' && s[1] && s[1]!='$'){ const char*q=s+1; while(bx_var_char((unsigned char)*q)) q++; if(!*q){ int ix=box_index(b,s+1); if(ix>=0 && b->items[ix].has_ival){ if(out)*out=b->items[ix].ival; return 1; } } }
    return 0;
}
static int eval_cond(Boxes *b, char **p, int n) {
    if (n < 3) return 0;
    const char *op = cmp_op_norm(p[1]);
    long lv=0, rv=0; int lc=cond_num(b,p[0],&lv), rc=cond_num(b,p[2],&rv);
    if(lc && rc){
        if(!strcmp(op,"==")) return lv==rv;
        if(!strcmp(op,"!=")) return lv!=rv;
        if(!strcmp(op,">")) return lv>rv;
        if(!strcmp(op,"<")) return lv<rv;
        if(!strcmp(op,">=")) return lv>=rv;
        if(!strcmp(op,"<=")) return lv<=rv;
    }
    Res l=rfast(b,p[0]), r=rfast(b,p[2]);
    int ok = cmp_values(l.ptr, op, r.ptr);
    if(l.owned) free((char*)l.ptr); if(r.owned) free((char*)r.ptr);
    return ok;
}

static const char *canonical(const char *cmd) {
    if (streqi(cmd,"b")) return "box";
    if (streqi(cmd,"s")) return "say";
    if (streqi(cmd,"a")) return "ask";
    if (streqi(cmd,"m")) return "math";
    if (streqi(cmd,"t")) return "test";
    if (streqi(cmd,"i")) return "if";
    if (streqi(cmd,"j")) return "jump";
    if (streqi(cmd,"ji")) return "jumpif";
    if (streqi(cmd,"d")) return "del";
    if (streqi(cmd,"mark") || streqi(cmd,"mk")) return "premark";
    if (streqi(cmd,"e")) return "end";
    if (streqi(cmd,"cls")) return "clear";
    return cmd;
}

static int exec_line(Program *pr, const char *raw, int pc);
static char *read_file(const char *path);
static void program_load(Program *pr, const char *src);
static int program_run_source(const char *src);
static int umload_run_module(Program *pr, const char *path, const char *return_mark);
static int umload_resolve_path(const char *name, char *out, size_t cap);
static int umload_cache_dir(char *out, size_t cap);
static int umload_registry_path(char *out, size_t cap);
static int umload_pkg_meta(const char *path, const char *key, char *out, size_t cap);
static int umload_pkg_install(const char *name, const char *url, int quiet);
static int umload_require_deps(Program *pr, const char *name, char *stack, int depth);
static int umload_loaded_has(Program *pr, const char *name);
static void umload_cache_pkg(char *out, size_t cap, const char *name);
static void umload_meta_trim(char *s);
static void umload_shell_sanitize(char *s);
static int bx_casestr(const char *hay, const char *needle);
static int bx_caseeq(const char *a, const char *b);
static int umload_show_deps(Program *pr, const char *name, int depth);
static int umload_path_is_cached(const char *path);

/* ---------------------------------------------------------------- gfx utils */

/* Colors accept either "#rgb" hex, a named color such as "crimson", or a box
 * holding either of those, so computed colors can be piped in as "$box". */
static int gfx_color_arg(Program *pr, const char *s, uint32_t *out) {
    if (!s) return 0;
    Res r = rfast(&pr->boxes, s);
    const char *v = r.ptr;
    uint32_t c = 0;
    /* Accept the "0xRRGGBBAA" form that color, lerp, alpha, named and pixel
     * all emit, so a computed color can be fed straight back in. */
    if (v[0] == '0' && (v[1] == 'x' || v[1] == 'X')) c = (uint32_t)strtoul(v + 2, NULL, 16);
    else c = bx_gfx_parse_color(v);
    if (r.owned) free((char *)r.ptr);
    if (!c) {
        r = rfast(&pr->boxes, s); v = r.ptr;
        int found = 0;
        c = bx_gfx_color_named(v, &found);
        if (r.owned) free((char *)r.ptr);
        if (found) { if (out) *out = c; return 1; }
        return 0;
    }
    if (out) *out = c;
    return 1;
}

static int gfx_color_arg_raw(const char *s, uint32_t *out) {
    if (!s) return 0;
    uint32_t c = bx_gfx_parse_color(s);
    if (c) { if (out) *out = c; return 1; }
    int found = 0;
    c = bx_gfx_color_named(s, &found);
    if (found) { if (out) *out = c; return 1; }
    return 0;
}

/* Gradients take two stops in one field, as "c0,c1". A single color is
 * accepted and used for both stops. */
static int gfx_color_pair(Program *pr, const char *s, uint32_t *a, uint32_t *b) {
    if (!s) return 0;
    const char *comma = strchr(s, ',');
    if (!comma) { if (!gfx_color_arg(pr, s, a)) return 0; *b = *a; return 1; }
    char tmp[64];
    size_t n = (size_t)(comma - s);
    if (n >= sizeof tmp) return 0;
    memcpy(tmp, s, n); tmp[n] = 0;
    if (!gfx_color_arg(pr, tmp, a)) return 0;
    return gfx_color_arg(pr, comma + 1, b);
}

/* Write a result back to a (possibly interpolated) box name. */
static void gfx_out(Program *pr, const char *spec, const char *val) {
    char *name = resolve(&pr->boxes, spec);
    box_set(&pr->boxes, name, val);
    free(name);
}

static bx_snd_wave_t snd_wave(const char *s, bx_snd_wave_t dflt){
    if(streqi(s,"sine")||streqi(s,"sin")) return BX_SND_WAVE_SINE;
    if(streqi(s,"square")||streqi(s,"sq")) return BX_SND_WAVE_SQUARE;
    if(streqi(s,"saw")) return BX_SND_WAVE_SAW;
    if(streqi(s,"tri")||streqi(s,"triangle")) return BX_SND_WAVE_TRI;
    if(streqi(s,"noise")) return BX_SND_WAVE_NOISE;
    return dflt;
}


static char *fhandle_new(FILE *fp);
static FILE *fhandle_get(const char *name);
static void fhandle_del(const char *name);


typedef struct { char *name; Program pr; long cmds; } BxeEnv;
static BxeEnv bxe_envs[16];
static int bxe_count=0;
static BxeEnv *bxe_find(const char *name){ for(int i=0;i<bxe_count;i++) if(!strcmp(bxe_envs[i].name,name)) return &bxe_envs[i]; return NULL; }
static void bxe_seed(BxeEnv *e, Program *parent){
    for(size_t i=0;i<parent->boxes.len;i++)
        if(box_index(&e->pr.boxes, parent->boxes.items[i].name)<0)
            box_set(&e->pr.boxes, parent->boxes.items[i].name, parent->boxes.items[i].value);
}
static BxeEnv *bxe_get(Program *parent, const char *name){
    BxeEnv *e=bxe_find(name);
    if(!e){
        if(bxe_count>=16){
            int worst=0; for(int i=1;i<bxe_count;i++) if(bxe_envs[i].cmds>bxe_envs[worst].cmds) worst=i;
            boxes_free(&bxe_envs[worst].pr.boxes); marks_free(&bxe_envs[worst].pr.marks); free(bxe_envs[worst].name);
            bxe_envs[worst]=bxe_envs[--bxe_count];
        }
        e=&bxe_envs[bxe_count++]; e->name=xstrdup(name); memset(&e->pr,0,sizeof e->pr); e->cmds=0;
        bxe_seed(e,parent);
    }
    if(e->cmds>=2000){
        boxes_free(&e->pr.boxes); marks_free(&e->pr.marks); memset(&e->pr,0,sizeof e->pr); e->cmds=0;
        printf("bxe: environment %s reset after 2000 commands\n", name); fflush(stdout);
        bxe_seed(e,parent);
    }
    return e;
}
static int bxe_run_cmd(BxeEnv *e, const char *cmd){ int pc=exec_line(&e->pr,cmd,0); e->cmds++; return pc; }
static void bxe_list(void){ printf("environments: %d\n",bxe_count); for(int i=0;i<bxe_count;i++) printf("  %-16s cmds=%ld\n",bxe_envs[i].name,bxe_envs[i].cmds); }
static void bxe_reset(BxeEnv *e){ boxes_free(&e->pr.boxes); marks_free(&e->pr.marks); memset(&e->pr,0,sizeof e->pr); e->cmds=0; }
static char *bxe_join(char **p, int start, int n){ size_t cap=0; for(int k=start;k<n;k++) cap += strlen(p[k])+2; char *cmd=calloc(1,cap+1); if(!cmd) return xstrdup(""); for(int k=start;k<n;k++){ if(k>start) strcat(cmd,"|"); strcat(cmd,p[k]); } return cmd; }

/* Marshalling: move values between a caller and a boxedenv. Values are
 * resolved against the caller's boxes so `$name` and the \(...) escape work
 * on both sides, which lets one env be called repeatedly like a function. */
static int bxe_in(Program *parent, BxeEnv *e, char **p, int start, int n){
    if(((n-start)&1)!=0){ fprintf(stderr,"bxe in: usage bxe in|%s|BOX|VALUE|BOX|VALUE...\n",e->name); return -1; }
    for(int k=start;k+1<n;k+=2){
        Res rn=rfast(&parent->boxes,p[k]), rv=rfast(&parent->boxes,p[k+1]);
        box_set(&e->pr.boxes,rn.ptr,rv.ptr);
        if(rn.owned)free((char*)rn.ptr);
        if(rv.owned)free((char*)rv.ptr);
    }
    return 0;
}
static int bxe_copy_box(Program *parent, BxeEnv *e, const char *name){
    int i=box_index(&e->pr.boxes,name);
    if(i<0){ fprintf(stderr,"bxe out: no box %s in %s\n",name,e->name); return 0; }
    Res rn=rfast(&parent->boxes,name);
    box_set(&parent->boxes,rn.ptr,e->pr.boxes.items[i].value);
    if(rn.owned)free((char*)rn.ptr);
    return 1;
}
static int bxe_out(Program *parent, BxeEnv *e, char **p, int start, int n){
    int copied=0;
    for(int k=start;k<n;k++){
        if(streqi(p[k],"*")){
            for(size_t i=0;i<e->pr.boxes.len;i++) copied += bxe_copy_box(parent,e,e->pr.boxes.items[i].name);
        } else copied += bxe_copy_box(parent,e,p[k]);
    }
    return copied;
}
static void bxe_show(BxeEnv *e, const char *name){
    int i=box_index(&e->pr.boxes,name);
    if(i<0) printf("bxe: %s has no box %s\n",e->name,name);
    else printf("%s\n",e->pr.boxes.items[i].value);
    fflush(stdout);
}
static void bxe_boxes(BxeEnv *e){
    printf("bxe: %s has %zu boxes\n",e->name,e->pr.boxes.len);
    for(size_t i=0;i<e->pr.boxes.len;i++){
        const char *nm=e->pr.boxes.items[i].name, *v=e->pr.boxes.items[i].value;
        if(strlen(v)>40) printf("  %-16s %.40s...\n",nm,v);
        else printf("  %-16s %s\n",nm,v);
    }
    fflush(stdout);
}
static int exec_command(Program *pr, const char *cmdline, int pc) {
    char *line = strip_comment(cmdline); char *s = trim(line); if (!*s) { free(line); return pc + 1; }
    char *space = s; while (*space && !isspace((unsigned char)*space) && *space != '|') space++;
    char saved = *space; *space = 0; const char *cmd = canonical(s); char cmd_buf[64]; snprintf(cmd_buf, sizeof cmd_buf, "%s", cmd); cmd = cmd_buf; char *args = saved ? trim(space + 1) : space; *space = saved;

    if (streqi(cmd,"box")) { int n; char **p = split_bars(args, &n); if (n >= 2) { Res rn=rfast(&pr->boxes,p[0]); Res rv=rfast(&pr->boxes,p[1]); box_set(&pr->boxes,rn.ptr,rv.ptr); if(rn.owned) free((char*)rn.ptr); if(rv.owned) free((char*)rv.ptr); } free_parts(p,n); }
    else if (streqi(cmd,"say")) { int n; char **p = split_bars(args,&n); Res rt={0,0}; char *text; if(n){ rt=rfast(&pr->boxes,p[0]); text=(char*)rt.ptr; } else { text=(char*)"\n"; puts(text); free_parts(p,n); return pc+1; } puts(text); if(stdout_is_tty()) fflush(stdout); if (n >= 2) { long sec = bx_int(&pr->boxes,p[1]); if (sec > 0) { struct timespec ts = { sec, 0 }; nanosleep(&ts, NULL); } } if(rt.owned) free((char*)rt.ptr); free_parts(p,n); }
    else if (streqi(cmd,"ask")) { char *r = resolve(&pr->boxes,args); char *last = strrchr(r, ' '); char *target = r; if (last) { *last = 0; printf("%s ", r); target = last + 1; } fflush(stdout); char buf[4096]; if (!fgets(buf,sizeof buf,stdin)) buf[0]=0; buf[strcspn(buf,"\r\n")]=0; box_set(&pr->boxes,target,buf); free(r); }
    else if (streqi(cmd,"math")) { int n; char **p=split_bars(args,&n); if(n>=4){ long a=bx_int(&pr->boxes,p[1]), b=bx_int(&pr->boxes,p[2]), v=0; Res ro=rfast(&pr->boxes,p[3]); const char *op=ro.ptr; if(!strcmp(op,"+"))v=a+b; else if(!strcmp(op,"-"))v=a-b; else if(!strcmp(op,"*")||streqi(op,"x"))v=a*b; else if(!strcmp(op,"/"))v=b? a/b:0; else if(!strcmp(op,"%"))v=b? a%b:0; if(ro.owned) free((char*)ro.ptr); char buf[64]; snprintf(buf,sizeof buf,"%ld",v); Res rn=rfast(&pr->boxes,p[0]); box_set(&pr->boxes,rn.ptr,buf); if(rn.owned) free((char*)rn.ptr);} free_parts(p,n); }
    else if (streqi(cmd,"test")) { int n; char **p=split_bars(args,&n); if(n>=4){ char *cond[3] = { p[1], p[3], p[2] }; int ok=eval_cond(&pr->boxes,cond,3); Res rv=rfast(&pr->boxes, ok ? (n>=5?p[4]:"1") : (n>=6?p[5]:"0")); Res rn=rfast(&pr->boxes,p[0]); box_set(&pr->boxes,rn.ptr,rv.ptr); if(rn.owned) free((char*)rn.ptr); if(rv.owned) free((char*)rv.ptr);} free_parts(p,n); }
    else if (streqi(cmd,"if")) { int n; char **p=split_bars(args,&n); if(n>=4 && eval_cond(&pr->boxes,p,3)){ size_t total=0; for(int i=3;i<n;i++) total += strlen(p[i])+2; char *nested=calloc(1,total+1); for(int i=3;i<n;i++){ if(i>3) strcat(nested, i==4 ? " " : "|"); strcat(nested,p[i]); } int npc=exec_line(pr,nested,pc); free(nested); free_parts(p,n); free(line); return npc==pc+1?pc+1:npc; } free_parts(p,n); }
    else if (streqi(cmd,"jump")) { int n; char **p=split_bars(args,&n); if(n>=1){ Res rt=rfast(&pr->boxes,p[0]); const char *target=rt.ptr; if(n>=2 && streqi(p[1],"m")){ int m=mark_find(&pr->marks,target); if(m>=0){ if(rt.owned) free((char*)rt.ptr); free_parts(p,n); free(line); return m; }} else if(is_number(target)) { int t=atoi(target)-1; if(t>=0 && t<pr->count){ if(rt.owned) free((char*)rt.ptr); free_parts(p,n); free(line); return t; }} if(rt.owned) free((char*)rt.ptr); } free_parts(p,n); }
    else if (streqi(cmd,"jumpif")) { int n; char **p=split_bars(args,&n); if(n>=4 && eval_cond(&pr->boxes,p,3)){ Res rt=rfast(&pr->boxes,p[3]); const char *target=rt.ptr; if(n>=5 && streqi(p[4],"m")){ int m=mark_find(&pr->marks,target); if(m>=0){ if(rt.owned) free((char*)rt.ptr); free_parts(p,n); free(line); return m; }} else if(is_number(target)) { int t=atoi(target)-1; if(t>=0&&t<pr->count){ if(rt.owned) free((char*)rt.ptr); free_parts(p,n); free(line); return t; }} if(rt.owned) free((char*)rt.ptr); } free_parts(p,n); }
    else if (streqi(cmd,"del")) { Res rn=rfast(&pr->boxes,args); box_del(&pr->boxes,rn.ptr); if(rn.owned) free((char*)rn.ptr); }
    else if (streqi(cmd,"str")) { int n; char **p=split_bars(args,&n);
        if(n<3) fprintf(stderr,"str: usage str|$out|$op|$args...\n");
        else {
            Res ro=rfast(&pr->boxes,p[1]);
            const char *op=ro.ptr;
            char *out=NULL;
            if(bx_caseeq(op,"len")){
                Res r=rfast(&pr->boxes,p[2]);
                char buf[32]; snprintf(buf,sizeof buf,"%zu",strlen(r.ptr));
                out=xstrdup(buf); if(r.owned)free((char*)r.ptr);
            } else if(bx_caseeq(op,"upper")||bx_caseeq(op,"lower")){
                int up = bx_caseeq(op,"upper");
                Res r=rfast(&pr->boxes,p[2]); out=xstrdup(r.ptr); if(r.owned)free((char*)r.ptr);
                for(char *q=out;*q;q++) *q = up ? (char)toupper((unsigned char)*q) : (char)tolower((unsigned char)*q);
            } else if(bx_caseeq(op,"trim")){
                Res r=rfast(&pr->boxes,p[2]); const char *s=r.ptr;
                while(*s && isspace((unsigned char)*s)) s++;
                const char *e=s+strlen(s);
                while(e>s && isspace((unsigned char)e[-1])) e--;
                out=xstrndup(s,(size_t)(e-s)); if(r.owned)free((char*)r.ptr);
            } else if(bx_caseeq(op,"reverse")){
                Res r=rfast(&pr->boxes,p[2]); size_t l=strlen(r.ptr);
                out=malloc(l+1);
                for(size_t i=0;i<l;i++) out[i]=r.ptr[l-1-i];
                out[l]=0; if(r.owned)free((char*)r.ptr);
            } else if(bx_caseeq(op,"find")){
                if(n<4){ fprintf(stderr,"str find: usage str|$out|find|$hay|$needle\n"); }
                else { Res h=rfast(&pr->boxes,p[2]); Res nd=rfast(&pr->boxes,p[3]);
                    const char *f=strstr(h.ptr,nd.ptr);
                    char buf[32]; snprintf(buf,sizeof buf,"%ld", f?(long)(f-h.ptr):-1L);
                    out=xstrdup(buf);
                    if(h.owned)free((char*)h.ptr); if(nd.owned)free((char*)nd.ptr); }
            } else if(bx_caseeq(op,"contains")){
                if(n<4){ fprintf(stderr,"str contains: usage str|$out|contains|$hay|$needle\n"); }
                else { Res h=rfast(&pr->boxes,p[2]); Res nd=rfast(&pr->boxes,p[3]);
                    out=xstrdup(strstr(h.ptr,nd.ptr)?"1":"0");
                    if(h.owned)free((char*)h.ptr); if(nd.owned)free((char*)nd.ptr); }
            } else if(bx_caseeq(op,"take")||bx_caseeq(op,"left")||bx_caseeq(op,"right")){
                if(n<4){ fprintf(stderr,"str: usage str|$out|%s|$text|$n\n",op); }
                else {
                    Res r=rfast(&pr->boxes,p[2]);
                    size_t l=strlen(r.ptr);
                    long i = bx_int(&pr->boxes,p[3]);
                    if(i<0) i=0; if((size_t)i>l) i=(long)l;
                    if(bx_caseeq(op,"right"))       out=xstrndup(r.ptr+(l-(size_t)i),(size_t)i);
                    else if(bx_caseeq(op,"left"))  out=xstrndup(r.ptr,(size_t)i);
                    else { long len = n>=5 ? bx_int(&pr->boxes,p[4]) : (long)(l-(size_t)i);
                           if(len<0) len=0; if((size_t)(i+len)>l) len=(long)(l-(size_t)i);
                           out=xstrndup(r.ptr+i,(size_t)len); }
                    if(r.owned)free((char*)r.ptr);
                }
            } else if(bx_caseeq(op,"repeat")){
                if(n<4){ fprintf(stderr,"str repeat: usage str|$out|repeat|$text|$n\n"); }
                else { Res r=rfast(&pr->boxes,p[2]); long c=bx_int(&pr->boxes,p[3]);
                    if(c<0)c=0; if(c>10000)c=10000;
                    size_t l=strlen(r.ptr); out=malloc(l*(size_t)c+1); out[0]=0;
                    for(long k=0;k<c;k++) memcpy(out+(size_t)k*l,r.ptr,l);
                    out[l*(size_t)c]=0; if(r.owned)free((char*)r.ptr); }
            } else if(bx_caseeq(op,"pad")){
                if(n<4){ fprintf(stderr,"str pad: usage str|$out|pad|$text|$width\n"); }
                else { Res r=rfast(&pr->boxes,p[2]); long w=bx_int(&pr->boxes,p[3]);
                    size_t l=strlen(r.ptr);
                    if(w<0)w=0; if(w>100000)w=100000;
                    size_t want=(size_t)w; if(want<l)want=l;
                    out=malloc(want+1); memset(out,' ',want); memcpy(out,r.ptr,l); out[want]=0;
                    if(r.owned)free((char*)r.ptr); }
            } else if(bx_caseeq(op,"replace")){
                if(n<5){ fprintf(stderr,"str replace: usage str|$out|replace|$text|$from|$to\n"); }
                else { Res r=rfast(&pr->boxes,p[2]); Res f=rfast(&pr->boxes,p[3]); Res t=rfast(&pr->boxes,p[4]);
                    size_t fl=strlen(f.ptr);
                    if(!fl){ out=xstrdup(r.ptr); }
                    else { size_t cap=strlen(r.ptr)+1, len=0; out=malloc(cap+1); out[0]=0;
                        const char *s=r.ptr;
                        while(*s){ const char *hit=strstr(s,f.ptr);
                            if(!hit){ size_t add=strlen(s); memcpy(out+len,s,add); len+=add; break; }
                            size_t pre=(size_t)(hit-s); memcpy(out+len,s,pre); len+=pre;
                            size_t tl=strlen(t.ptr); if(len+tl+1>cap){ cap=(len+tl+1)*2; out=realloc(out,cap); }
                            memcpy(out+len,t.ptr,tl); len+=tl; s=hit+fl; }
                        out[len]=0; }
                    if(r.owned)free((char*)r.ptr); if(f.owned)free((char*)f.ptr); if(t.owned)free((char*)t.ptr); }
            } else if(bx_caseeq(op,"wordcount")){
                Res r=rfast(&pr->boxes,p[2]); long c=0; int inw=0;
                for(const char *s=r.ptr;*s;s++){ if(isspace((unsigned char)*s)) inw=0; else if(!inw){inw=1;c++;} }
                char buf[32]; snprintf(buf,sizeof buf,"%ld",c); out=xstrdup(buf);
                if(r.owned)free((char*)r.ptr);
            } else if(bx_caseeq(op,"startswith")||bx_caseeq(op,"endswith")){
                if(n<4){ fprintf(stderr,"str %s: usage str|$out|%s|$text|$needle\n",op,op); }
                else { Res r=rfast(&pr->boxes,p[2]); Res nd=rfast(&pr->boxes,p[3]);
                    size_t rl=strlen(r.ptr), nl=strlen(nd.ptr);
                    int hit = nl<=rl && (bx_caseeq(op,"startswith")
                        ? !strncmp(r.ptr,nd.ptr,nl)
                        : !strcmp(r.ptr+rl-nl,nd.ptr));
                    out=xstrdup(hit?"1":"0");
                    if(r.owned)free((char*)r.ptr); if(nd.owned)free((char*)nd.ptr); }
            } else if(bx_caseeq(op,"count")){
                if(n<4){ fprintf(stderr,"str count: usage str|$out|count|$text|$needle\n"); }
                else { Res r=rfast(&pr->boxes,p[2]); Res nd=rfast(&pr->boxes,p[3]);
                    size_t nl=strlen(nd.ptr); long c=0;
                    if(nl){ const char *s=r.ptr; for(;;){ const char *h=strstr(s,nd.ptr); if(!h)break; c++; s=h+nl; } }
                    char buf[32]; snprintf(buf,sizeof buf,"%ld",c); out=xstrdup(buf);
                    if(r.owned)free((char*)r.ptr); if(nd.owned)free((char*)nd.ptr); }
            } else if(bx_caseeq(op,"squeeze")){
                Res r=rfast(&pr->boxes,p[2]); size_t l=strlen(r.ptr);
                out=malloc(l+1); size_t w=0; int sp=0;
                for(size_t i=0;i<l;i++){ int c=isspace((unsigned char)r.ptr[i]);
                    if(c){ if(!sp) out[w++]=' '; sp=1; } else { out[w++]=r.ptr[i]; sp=0; } }
                while(w && out[w-1]==' ') w--;
                out[w]=0; if(r.owned)free((char*)r.ptr);
            } else if(bx_caseeq(op,"capitalize")){
                Res r=rfast(&pr->boxes,p[2]);
                out=xstrdup(r.ptr);
                for(size_t i=0;out[i];i++) out[i]=(char)(i?tolower((unsigned char)out[i]):toupper((unsigned char)out[i]));
                if(r.owned)free((char*)r.ptr);
            } else if(bx_caseeq(op,"titlecase")){
                Res r=rfast(&pr->boxes,p[2]);
                out=xstrdup(r.ptr); int ws=1;
                for(size_t i=0;out[i];i++){ unsigned char c=(unsigned char)out[i];
                    if(isspace(c)){ ws=1; }
                    else { out[i]=(char)(ws?toupper(c):tolower(c)); ws=0; } }
                if(r.owned)free((char*)r.ptr);
            } else if(bx_caseeq(op,"isdigit")||bx_caseeq(op,"isalpha")||bx_caseeq(op,"isspace")){
                Res r=rfast(&pr->boxes,p[2]); int ok = r.ptr[0]!=0, i=0;
                for(;r.ptr[i];i++){
                    unsigned char c=(unsigned char)r.ptr[i];
                    if(bx_caseeq(op,"isdigit") ? !isdigit(c) : bx_caseeq(op,"isalpha") ? !isalpha(c) : !isspace(c)) { ok=0; break; }
                }
                out=xstrdup(ok?"1":"0"); if(r.owned)free((char*)r.ptr);
            } else if(bx_caseeq(op,"center")){
                if(n<4){ fprintf(stderr,"str center: usage str|$out|center|$text|$width\n"); }
                else { Res r=rfast(&pr->boxes,p[2]); long w=bx_int(&pr->boxes,p[3]);
                    size_t l=strlen(r.ptr);
                    if(w<0)w=0; if(w>100000)w=100000;
                    if((size_t)w<l) w=(long)l;
                    size_t pad=(size_t)w-l;
                    out=malloc((size_t)w+1);
                    memset(out,' ',pad/2); memcpy(out+pad/2,r.ptr,l); memset(out+pad/2+l,' ',pad-pad/2);
                    out[(size_t)w]=0; if(r.owned)free((char*)r.ptr); }
            } else if(bx_caseeq(op,"quote")){
                Res r=rfast(&pr->boxes,p[2]);
                out=malloc(strlen(r.ptr)+3); out[0]='"'; strcpy(out+1,r.ptr); out[strlen(r.ptr)+1]='"'; out[strlen(r.ptr)+2]=0;
                if(r.owned)free((char*)r.ptr);
            } else if(bx_caseeq(op,"chunks")){
                if(n<4){ fprintf(stderr,"str chunks: usage str|$out|chunks|$text|$size|$sep\n"); }
                else { Res r=rfast(&pr->boxes,p[2]);
                    long step = bx_int(&pr->boxes,p[3]); if(step<1) step=1; if(step>100000) step=100000;
                    Res sep = n>=5 ? rfast(&pr->boxes,p[4]) : (Res){(char*)" ",0};
                    size_t l=strlen(r.ptr), sl=strlen(sep.ptr);
                    size_t total=l + ((l+(size_t)step-1)/(size_t)step ? ((l+(size_t)step-1)/(size_t)step-1)*sl : 0) + 1;
                    out=malloc(total+1); out[0]=0; size_t at=0;
                    for(size_t i=0;i<l;i+=(size_t)step){
                        if(i){ memcpy(out+at,sep.ptr,sl); at+=sl; }
                        size_t k=((size_t)step<l-i)?(size_t)step:(l-i);
                        memcpy(out+at,r.ptr+i,k); at+=k; out[at]=0;
                    }
                    if(r.owned)free((char*)r.ptr); if(sep.owned)free((char*)sep.ptr); }
            } else if(bx_caseeq(op,"wrap")){
                if(n<4){ fprintf(stderr,"str wrap: usage str|$out|wrap|$text|$width|$sep\n"); }
                else { Res r=rfast(&pr->boxes,p[2]);
                    long width = bx_int(&pr->boxes,p[3]);
                    if(width<1) width=1; if(width>1000) width=1000;
                    Res sep = n>=5 ? rfast(&pr->boxes,p[4]) : (Res){(char*)"\n",0};
                    size_t sl=strlen(sep.ptr);
                    size_t worst = strlen(r.ptr)*2 + 16;
                    out=malloc(worst); out[0]=0; size_t at=0;
                    const char *s=r.ptr; size_t linelen=0; int first=1;
                    while(*s){
                        while(*s==' ') s++;
                        if(!*s) break;
                        const char *w=s;
                        while(*s && *s!=' ') s++;
                        size_t wl=(size_t)(s-w);
                        if(!first){
                            if(linelen && linelen+1+wl > (size_t)width){ memcpy(out+at,sep.ptr,sl); at+=sl; linelen=0; }
                            else { out[at++]=' '; linelen++; }
                        }
                        memcpy(out+at,w,wl); at+=wl; linelen+=wl; out[at]=0;
                        first=0;
                    }
                    if(r.owned)free((char*)r.ptr); if(sep.owned)free((char*)sep.ptr); }
            } else if(bx_caseeq(op,"split")){
                if(n<4){ fprintf(stderr,"str split: usage str|$out|split|$text|$sep\n"); }
                else { Res r=rfast(&pr->boxes,p[2]); Res sep=rfast(&pr->boxes,p[3]);
                    size_t sl=strlen(sep.ptr);
                    out=malloc(strlen(r.ptr)+1); out[0]=0; size_t at=0;
                    if(!sl) strcpy(out,r.ptr);
                    else { const char *s=r.ptr;
                        for(;;){ const char *h=strstr(s,sep.ptr);
                            if(!h){ size_t add=strlen(s); memcpy(out+at,s,add); at+=add; break; }
                            size_t pre=(size_t)(h-s); memcpy(out+at,s,pre); at+=pre;
                            out[at++]='|'; s=h+sl; }
                        out[at]=0; }
                    if(r.owned)free((char*)r.ptr); if(sep.owned)free((char*)sep.ptr); }
            } else if(bx_caseeq(op,"join")){
                if(n<3){ fprintf(stderr,"str join: usage str|$out|join|$sep|$a|$b|...\n"); }
                else {
                    Res sep=rfast(&pr->boxes,p[2]);
                    size_t total=1; for(int i=3;i<n;i++){ Res t=rfast(&pr->boxes,p[i]); total+=strlen(t.ptr); if(t.owned)free((char*)t.ptr); }
                    total += (size_t)(n>3?n-4:0)*strlen(sep.ptr);
                    out=malloc(total+1); out[0]=0; size_t at=0;
                    for(int i=3;i<n;i++){ Res t=rfast(&pr->boxes,p[i]);
                        if(i>3){ size_t sl=strlen(sep.ptr); memcpy(out+at,sep.ptr,sl); at+=sl; }
                        size_t l=strlen(t.ptr); memcpy(out+at,t.ptr,l); at+=l; out[at]=0;
                        if(t.owned)free((char*)t.ptr); }
                    if(sep.owned)free((char*)sep.ptr);
                }
            } else fprintf(stderr,"str: unknown op %s\n",op);
            if(ro.owned)free((char*)ro.ptr);
            if(out){ Res rn=rfast(&pr->boxes,p[0]); box_set(&pr->boxes,rn.ptr,out); if(rn.owned)free((char*)rn.ptr); free(out); }
        }
        free_parts(p,n);
    }
    else if (streqi(cmd,"file")) { int n; char **p=split_bars(args,&n);
        if(n<2) fprintf(stderr,"file: usage file|$out|$op|...\n");
        else {
            Res ao = rfast(&pr->boxes,p[1]);
            const char *act = ao.ptr;
            if(bx_caseeq(act,"open")||bx_caseeq(act,"append")){
                if(n<3){ fprintf(stderr,"file %s: usage file|$h|%s|$path|$mode\n",act,act); }
                else {
                    Res pa=rfast(&pr->boxes,p[2]);
                    Res md = n>=4 ? rfast(&pr->boxes,p[3]) : (Res){(char*)"r",0};
                    FILE *f = fopen(pa.ptr,md.ptr);
                    if(!f) fprintf(stderr,"file: cannot open %s\n",pa.ptr);
                    else { char *h = fhandle_new(f);   /* the table owns this name */
                           if(h) box_set(&pr->boxes,p[0],h);
                           else { fprintf(stderr,"file: too many open handles\n"); fclose(f); } }
                    if(pa.owned)free((char*)pa.ptr); if(md.owned)free((char*)md.ptr);
                }
            }
            else if(bx_caseeq(act,"close")){
                /* the handle may be given as $h in the first slot or as a plain arg */
                Res hh = n>=3 ? rfast(&pr->boxes,p[2]) : rfast(&pr->boxes,p[0]);
                FILE *f=fhandle_get(hh.ptr);
                if(f){ fclose(f); fhandle_del(hh.ptr); }
                else fprintf(stderr,"file: %s is not open\n",hh.ptr);
                if(hh.owned)free((char*)hh.ptr);
            }
            else if(bx_caseeq(act,"write")){
                if(n<4){ fprintf(stderr,"file write: usage file|$h|write|$text\n"); }
                else { Res hh=rfast(&pr->boxes,p[2]); Res t=rfast(&pr->boxes,p[3]);
                    FILE *f=fhandle_get(hh.ptr);
                     if(!f) fprintf(stderr,"file: %s is not open\n",hh.ptr);
                     else { size_t w=fwrite(t.ptr,1,strlen(t.ptr),f); fflush(f);
                            char buf[32]; snprintf(buf,sizeof buf,"%zu",w);
                            box_set(&pr->boxes,p[0],buf); }
                    if(hh.owned)free((char*)hh.ptr); if(t.owned)free((char*)t.ptr); }
            }
            else if(bx_caseeq(act,"read")){
                if(n<4){ fprintf(stderr,"file read: usage file|$out|read|$h|$bytes\n"); }
                else { Res hh=rfast(&pr->boxes,p[2]); FILE *f=fhandle_get(hh.ptr);
                    if(!f) fprintf(stderr,"file: %s is not open\n",hh.ptr);
                    else { long want = n>=4 ? bx_int(&pr->boxes,p[3]) : 4096;
                        if(want<0)want=0; if(want>1048576)want=1048576;
                        char *buf=malloc((size_t)want+1);
                        size_t got=fread(buf,1,(size_t)want,f);
                        if(got==0&&want>0){ int c=fgetc(f); if(c!=EOF){ buf[0]=(char)c; got=1; } }
                        buf[got]=0; box_set(&pr->boxes,p[0],buf); free(buf); }
                    if(hh.owned)free((char*)hh.ptr); }
            }
            else if(bx_caseeq(act,"line")){
                if(n<3){ fprintf(stderr,"file line: usage file|$out|line|$h\n"); }
                else { Res hh=rfast(&pr->boxes,p[2]); FILE *f=fhandle_get(hh.ptr);
                    if(!f) fprintf(stderr,"file: %s is not open\n",hh.ptr);
                    else { char *buf=malloc(65536);
                        if(fgets(buf,65536,f)){ size_t l=strlen(buf);
                            while(l&&(buf[l-1]=='\n'||buf[l-1]=='\r'))buf[--l]=0;
                            box_set(&pr->boxes,p[0],buf); }
                        else box_set(&pr->boxes,p[0],"");
                        free(buf); }
                    if(hh.owned)free((char*)hh.ptr); }
            }
            else if(bx_caseeq(act,"size")){
                if(n<3){ fprintf(stderr,"file size: usage file|$out|size|$path\n"); }
                else { Res pa=rfast(&pr->boxes,p[2]);
                    FILE *f=fopen(pa.ptr,"rb"); long sz=-1;
                    if(f){ if(!fseek(f,0,SEEK_END)) sz=ftell(f); fclose(f); }
                    char buf[32]; snprintf(buf,sizeof buf,"%ld",sz);
                    box_set(&pr->boxes,p[0],buf);
                    if(pa.owned)free((char*)pa.ptr); }
            }
            else if(bx_caseeq(act,"exists")){
                if(n<3){ fprintf(stderr,"file exists: usage file|$out|exists|$path\n"); }
                else { Res pa=rfast(&pr->boxes,p[2]);
                    box_set(&pr->boxes,p[0],access(pa.ptr,F_OK)==0?"1":"0");
                    if(pa.owned)free((char*)pa.ptr); }
            }
            else if(bx_caseeq(act,"remove")){
                if(n<3){ fprintf(stderr,"file remove: usage file|$out|remove|$path\n"); }
                else { Res pa=rfast(&pr->boxes,p[2]);
                    box_set(&pr->boxes,p[0],remove(pa.ptr)==0?"1":"0");
                    if(pa.owned)free((char*)pa.ptr); }
            }
            else fprintf(stderr,"file: unknown op %s\n",act);
            if(ao.owned)free((char*)ao.ptr);
        }
        free_parts(p,n);
    }
    else if (streqi(cmd,"bxe")) { int n; char **p=split_bars(args,&n);
        if(n>=1 && streqi(p[0],"list")) { bxe_list(); }
        else if(n>=2 && streqi(p[0],"create")) { bxe_get(pr,p[1]); printf("bxe: environment %s ready\n",p[1]); fflush(stdout); }
        else if(n>=3 && streqi(p[0],"run")) { BxeEnv *e=bxe_get(pr,p[1]); if(e){ char *cmd=bxe_join(p,2,n); bxe_run_cmd(e,cmd); free(cmd); } }
        else if(n>=2 && streqi(p[0],"all")) { char *cmd=bxe_join(p,1,n); for(int i=0;i<bxe_count;i++) bxe_run_cmd(&bxe_envs[i],cmd); free(cmd); }
        else if(n>=2 && streqi(p[0],"cmds")) { BxeEnv *e=bxe_find(p[1]); printf("bxe: %s commands=%ld\n",p[1], e?(long)e->cmds:0L); fflush(stdout); }
        else if(n>=2 && streqi(p[0],"reset")) { BxeEnv *e=bxe_find(p[1]); if(e){ bxe_reset(e); printf("bxe: reset %s\n",p[1]); } else printf("bxe: no env %s\n",p[1]); fflush(stdout); }
        else if(n>=2 && streqi(p[0],"in")) { BxeEnv *e=bxe_get(pr,p[1]); if(e && bxe_in(pr,e,p,2,n)==0) printf("bxe: set %d boxes in %s\n",(n-2)/2,p[1]); fflush(stdout); }
        else if(n>=2 && streqi(p[0],"out")) { BxeEnv *e=bxe_get(pr,p[1]); if(e) printf("bxe: copied %d boxes from %s\n",bxe_out(pr,e,p,2,n),p[1]); fflush(stdout); }
        else if(n>=3 && streqi(p[0],"get")) { BxeEnv *e=bxe_find(p[1]); if(e) bxe_show(e,p[2]); else printf("bxe: no env %s\n",p[1]); fflush(stdout); }
        else if(n>=2 && streqi(p[0],"boxes")) { BxeEnv *e=bxe_find(p[1]); if(e) bxe_boxes(e); else printf("bxe: no env %s\n",p[1]); fflush(stdout); }
        else { printf("bxe commands:\n  list | create|name | run|name|cmd | all|cmd | cmds|name | reset|name\n");
               printf("  in|name|BOX|VALUE... | out|name|BOX|* | get|name|BOX | boxes|name\n"); }
        free_parts(p,n);
    }
    else if (streqi(cmd,"lib")) { int n; char **p=split_bars(args,&n);
        if(n>=1 && streqi(p[0],"list")){
            printf("compiled-in libraries: wifi, gfx, snd, math\n");
            printf("active:\n");
            int any=0;
            if(streqi(box_get(&pr->boxes,"lib_active_wifi"),"1")){ printf("  wifi\n"); any=1; }
            if(streqi(box_get(&pr->boxes,"lib_active_gfx"),"1")){ printf("  gfx\n"); any=1; }
            if(streqi(box_get(&pr->boxes,"lib_active_snd"),"1")){ printf("  snd\n"); any=1; }
            if(streqi(box_get(&pr->boxes,"lib_active_math"),"1")){ printf("  math\n"); any=1; }
            if(!any) printf("  (none) - use 'lib load|name'\n");
            fflush(stdout);
        }
        else if(n>=2 && streqi(p[0],"load")){
            if(streqi(p[1],"wifi")){ bx_wifi_init(); box_set(&pr->boxes,"lib_active_wifi","1"); printf("lib: wifi loaded\n"); fflush(stdout); }
            else if(streqi(p[1],"gfx")){ box_set(&pr->boxes,"lib_active_gfx","1"); printf("lib: gfx loaded\n"); fflush(stdout); }
            else if(streqi(p[1],"snd")){ if(!streqi(box_get(&pr->boxes,"lib_active_snd"),"1")) bx_snd_init(0);
                                        box_set(&pr->boxes,"lib_active_snd","1"); printf("lib: snd loaded\n"); fflush(stdout); }
            else if(streqi(p[1],"math")){ box_set(&pr->boxes,"lib_active_math","1"); printf("lib: math loaded\n"); fflush(stdout); }
            else { fprintf(stderr,"lib: '%s' is not compiled into this build (available: wifi, gfx, snd, math)\n",p[1]); fflush(stderr); }
        }
        else if(n>=2 && streqi(p[0],"unload")){
            if(streqi(p[1],"wifi")){ bx_wifi_disconnect(); box_set(&pr->boxes,"lib_active_wifi","0"); printf("lib: wifi unloaded\n"); fflush(stdout); }
            else if(streqi(p[1],"gfx")){ box_set(&pr->boxes,"lib_active_gfx","0"); printf("lib: gfx unloaded\n"); fflush(stdout); }
            else if(streqi(p[1],"snd")){ bx_snd_clear(); box_set(&pr->boxes,"lib_active_snd","0"); printf("lib: snd unloaded\n"); fflush(stdout); }
            else if(streqi(p[1],"math")){ box_set(&pr->boxes,"lib_active_math","0"); printf("lib: math unloaded\n"); fflush(stdout); }
            else { fprintf(stderr,"lib: unknown library '%s'\n",p[1]); fflush(stderr); }
        }
        else { printf("lib commands:\n  list | load|name | unload|name\n"); }
        free_parts(p,n);
    }
    else if (!strncmp(cmd,"high.wifi.",10)) {
        if(streqi(box_get(&pr->boxes,"lib_active_wifi"),"1")){
            const char *sub=cmd+10; int n; char **p=split_bars(args,&n);
            if(streqi(sub,"init")){ bx_wifi_init(); printf("high.wifi: init ok\n"); fflush(stdout); }
            else if(streqi(sub,"status") && n>=1){ char buf[256]; bx_wifi_status(buf,sizeof buf); char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,buf); free(name); }
            else if(streqi(sub,"info")){ char buf[256]; bx_wifi_status(buf,sizeof buf); puts(buf); }
            else if(streqi(sub,"connect") && n>=2){ char *ssid=resolve(&pr->boxes,p[0]); char *pass=xstrdup(""); if(n>=2){ free(pass); pass=resolve(&pr->boxes,p[1]); } bx_wifi_sec_t sec=BX_WIFI_SEC_OPEN; if(n>=3){ char *sr=resolve(&pr->boxes,p[2]); sec=(bx_wifi_sec_t)atoi(sr); free(sr); } int rc=bx_wifi_creq(ssid,sec,pass); if(rc==0) printf("high.wifi: connected to %s\n",ssid); else fprintf(stderr,"high.wifi: connect failed\n"); free(ssid); free(pass); fflush(stdout); }
            else if(streqi(sub,"disconnect")){ bx_wifi_disconnect(); printf("high.wifi: disconnected\n"); fflush(stdout); }
            else if(streqi(sub,"env") && n>=1){ bx_wifi_env_t e=bx_wifi_detect_env(); const char *s=e==BX_WIFI_ENV_BAREMETAL?"BAREMETAL":(e==BX_WIFI_ENV_NATIVE?"NATIVE":"WEB"); char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,s); free(name); }
            else { printf("high.wifi commands: init | connect|ssid|pass|sec | status|box | info | env|box | disconnect\n"); }
            free_parts(p,n);
        } else { fprintf(stderr,"wifi library not loaded: use 'lib load|wifi'\n"); fflush(stderr); }
    }
#ifndef BX_EMBEDDED_SOURCE
    /* The UI layer lives in bx_ui.c, which a transpiled program does not
     * link, so these commands are native-only like high.math.* and high.m3d.*. */
    else if (!strcmp(cmd,"ui") || !strncmp(cmd,"ui.",3)) {
        /* Two spellings, both supported:
         *   ui.new|ID|KIND      (dotted, like high.gfx.*)
         *   ui|new|ID|KIND      (bar form, like lib)
         * The bar form is convenient in a script, the dotted form reads like
         * the rest of the library commands. */
        const char *sub = NULL;
        char **p = NULL; int n = 0; char *substr = NULL;
        char family[48], act[48];
        if (cmd[2]=='.') {
            sub = cmd+3;
            p = split_bars(args,&n);
        } else {
            p = split_bars(args,&n);
            if (n>=1) {
                /* p[0] is the subcommand; move the rest down one slot so the
                 * handlers can keep indexing from p[0]. substr is freed at the
                 * end, because free_parts no longer sees it. */
                substr = p[0];
                sub = substr;
                for(int i=1;i<n;i++) p[i-1] = p[i];
                n--;
            }
        }
        /* ui.tweenfn.set is a two-level name; split it into family and action
         * and fold the action back in as the first field so the handlers only
         * ever look at one shape. */
        family[0] = act[0] = 0;
        if (sub) {
            const char *dot = strchr(sub,'.');
            if (dot) {
                size_t fn = (size_t)(dot - sub); if (fn >= sizeof family) fn = sizeof family - 1;
                memcpy(family, sub, fn); family[fn] = 0;
                snprintf(act, sizeof act, "%s", dot+1);
            } else snprintf(family, sizeof family, "%s", sub);
            if (act[0]) {
                static char joined[4096];
                snprintf(joined, sizeof joined, "%s|%s", act, args ? args : "");
                free_parts(p,n);
                p = split_bars(joined,&n);
            }
        }
        const char *fam = family;
        if(!sub){ printf("ui commands: init | kinds|list | new|ID|KIND [parent] | set|ID|field|val | get|ID|field | attach|FRAME|ID... | detach|FRAME|ID | list | layout|FRAME | dock|PANE | switch|PANE | close|ID | tween|ID|TARGET|PROP|FROM|TO|DUR|FN | tween|ID|to|V | tween|ID|cancel | tween|list | tweenfn|set|NAME|... | tweenfn|list | frame|once | frame|step|SECS | frame|fps|N | ease|FN|T...\n"); free_parts(p,n); free(substr); return pc+1; }
        if(streqi(sub,"init")){
            bx_ui_reset(); printf("ui: init ok\n"); fflush(stdout);
        }
        else if(streqi(fam,"kinds") && n>=1){
            if(!strcmp(p[0],"list")||streqi(p[0],"list")){
                printf("ui element types (%d):\n", bx_ui_kind_count());
                for(int i=0;i<bx_ui_kind_count();i++) printf("  %s\n", bx_ui_kind_at(i));
                printf("custom types accepted: any unused id (sets a config object)\n");
                fflush(stdout);
            }
        }
        else if(streqi(fam,"new") && n>=2){
            /* ui new|ID|KIND [parent] - the pane/element constructor */
            int kind = bx_ui_kind_by_name(p[1]);
            if(kind<0){ fprintf(stderr,"ui new: unknown kind '%s'\n",p[1]); }
            else {
                bx_ui_element_t *e = bx_ui_add(p[0],(bx_ui_kind_t)kind, n>=3?p[2]:"");
                if(!e) fprintf(stderr,"ui new: bad id '%s' (max %d chars, a-zA-Z0-9_.-) or id already exists\n",p[0],BX_UI_ID_MAX-1);
                else {
                    /* An unknown kind becomes a config object: a free-form
                     * container whose fields can be set and tuned later. */
                    if(kind>=BX_UI_KIND_CUSTOM){ e->layout=BX_UI_LAYOUT_COLUMN; e->pad_x=8; e->pad_y=8; e->gap=6; }
                    if(kind==BX_UI_KIND_FRAME || kind==BX_UI_KIND_PANE){ e->w = n>=4?atoi(p[3]):320; e->h = n>=5?atoi(p[4]):240; }
                }
            }
        }
        else if(streqi(fam,"set") && n>=3){
            bx_ui_element_t *e = bx_ui_find(p[0]);
            if(!e) fprintf(stderr,"ui set: no element %s\n",p[0]);
            else {
                char *f = resolve(&pr->boxes,p[1]);
                char *v = resolve(&pr->boxes,p[2]);
                const char *field = p[1];
                if(!strcmp(field,"x")) e->x=atof(v);
                else if(!strcmp(field,"y")) e->y=atof(v);
                else if(!strcmp(field,"w")) e->w=atof(v);
                else if(!strcmp(field,"h")) e->h=atof(v);
                else if(!strcmp(field,"dock")){
                    if(streqi(v,"left")) e->dock=BX_UI_DOCK_LEFT;
                    else if(streqi(v,"right")) e->dock=BX_UI_DOCK_RIGHT;
                    else if(streqi(v,"top")) e->dock=BX_UI_DOCK_TOP;
                    else if(streqi(v,"bottom")) e->dock=BX_UI_DOCK_BOTTOM;
                    else if(streqi(v,"center")) e->dock=BX_UI_DOCK_CENTER;
                    else if(streqi(v,"fill")) e->dock=BX_UI_DOCK_FILL;
                    else e->dock=BX_UI_DOCK_NONE;
                }
                else if(!strcmp(field,"visible")) e->visible=atoi(v)?1:0;
                else if(!strcmp(field,"text")) snprintf(e->text,sizeof e->text,"%s",v);
                else if(!strcmp(field,"value")) snprintf(e->value,sizeof e->value,"%s",v);
                else if(!strcmp(field,"style")) snprintf(e->style,sizeof e->style,"%s",v);
                else if(!strcmp(field,"theme")){ if(bx_gfx_parse_theme(v,&e->theme)!=0) fprintf(stderr,"ui set: bad theme\n"); }
                else if(!strcmp(field,"color")){ int ok=0; uint32_t c=bx_gfx_parse_color(v); ok=c!=0||streqi(v,"0"); e->color=c; (void)ok; }
                else if(!strcmp(field,"alpha")) e->theme.alpha=(uint8_t)atoi(v);
                else if(!strcmp(field,"gap")) e->gap=atof(v);
                else if(!strcmp(field,"padx")) e->pad_x=atof(v);
                else if(!strcmp(field,"pady")) e->pad_y=atof(v);
                else if(!strcmp(field,"columns")) e->columns=atoi(v);
                else if(!strcmp(field,"z")) e->z=atoi(v);
                else if(!strcmp(field,"minw")) e->min_w=atof(v);
                else if(!strcmp(field,"minh")) e->min_h=atof(v);
                else if(!strcmp(field,"align")){
                    if(streqi(v,"start")){ e->align_h=e->align_v=BX_UI_ALIGN_START; }
                    else if(streqi(v,"center")){ e->align_h=e->align_v=BX_UI_ALIGN_CENTER; }
                    else if(streqi(v,"end")){ e->align_h=e->align_v=BX_UI_ALIGN_END; }
                    else if(streqi(v,"stretch")){ e->align_h=e->align_v=BX_UI_ALIGN_STRETCH; }
                }
                else if(!strcmp(field,"layout")){
                    if(streqi(v,"none")) e->layout=BX_UI_LAYOUT_NONE;
                    else if(streqi(v,"row")) e->layout=BX_UI_LAYOUT_ROW;
                    else if(streqi(v,"column")||streqi(v,"col")) e->layout=BX_UI_LAYOUT_COLUMN;
                    else if(streqi(v,"grid")) e->layout=BX_UI_LAYOUT_GRID;
                    else if(streqi(v,"stack")) e->layout=BX_UI_LAYOUT_STACK;
                    else if(streqi(v,"wrap")) e->layout=BX_UI_LAYOUT_WRAP;
                    else fprintf(stderr,"ui set: unknown layout %s\n",v);
                }
                else if(!strcmp(field,"justify")){
                    if(streqi(v,"start")) e->justify=BX_UI_JUSTIFY_START;
                    else if(streqi(v,"center")) e->justify=BX_UI_JUSTIFY_CENTER;
                    else if(streqi(v,"end")) e->justify=BX_UI_JUSTIFY_END;
                    else if(streqi(v,"between")||streqi(v,"space-between")) e->justify=BX_UI_JUSTIFY_SPACE_BETWEEN;
                }
                else {
                    /* Unknown field on a config object: keep it. A custom
                     * element is a bag of tweakable values by design. */
                    char tmp[128];
                    snprintf(tmp,sizeof tmp,"%s=%s",field,v);
                    /* stored in style slot when it fits, else ignored loudly */
                    if(strlen(tmp)<sizeof e->style) snprintf(e->style,sizeof e->style,"%s",tmp);
                    else fprintf(stderr,"ui set: value too long for %s\n",field);
                }
                free(f); free(v);
            }
        }
        else if(streqi(fam,"get") && n>=2){
            bx_ui_element_t *e = bx_ui_find(p[0]);
            if(!e){ fprintf(stderr,"ui get: no element %s\n",p[0]); }
            else {
                /* ui get|ID|FIELD|BOX - the box is optional and defaults to
                 * the id, matching how the other library getters behave. */
                const char *field=p[1];
                /* ID|FIELD[|BOX] - after normalisation both spellings have
                 * the same shape, so the box is the third field when given. */
                char *name = n>=3 ? resolve(&pr->boxes,p[2]) : resolve(&pr->boxes,p[0]);
                char b[160];
                if(!strcmp(field,"kind")||!strcmp(field,"kindname")) snprintf(b,sizeof b,"%s",bx_ui_kind_name(e->kind));
                else if(!strcmp(field,"x")) snprintf(b,sizeof b,"%g",e->x);
                else if(!strcmp(field,"y")) snprintf(b,sizeof b,"%g",e->y);
                else if(!strcmp(field,"w")) snprintf(b,sizeof b,"%g",e->w);
                else if(!strcmp(field,"h")) snprintf(b,sizeof b,"%g",e->h);
                else if(!strcmp(field,"text")) snprintf(b,sizeof b,"%s",e->text);
                else if(!strcmp(field,"value")) snprintf(b,sizeof b,"%s",e->value);
                else if(!strcmp(field,"visible")) snprintf(b,sizeof b,"%d",e->visible?1:0);
                else if(!strcmp(field,"dock")){
                    static const char *dn[]={"none","left","right","top","bottom","center","fill"};
                    snprintf(b,sizeof b,"%s",(e->dock>=0&&e->dock<=6)?dn[e->dock]:"none");
                }
                else if(!strcmp(field,"z")) snprintf(b,sizeof b,"%d",e->z);
                else if(!strcmp(field,"children")) snprintf(b,sizeof b,"%d",e->child_count);
                else if(!strcmp(field,"layout")) snprintf(b,sizeof b,"%d",(int)e->layout);
                else { fprintf(stderr,"ui get: unknown field %s\n",field); free(name); goto ui_done; }
                box_set(&pr->boxes,name,b);
                free(name);
            }
        }
        else if(streqi(fam,"attach") && n>=2){
            bx_ui_element_t *par=bx_ui_find(p[0]);
            if(!par) fprintf(stderr,"ui attach: no frame %s\n",p[0]);
            else {
                for(int i=1;i<n;i++){
                    bx_ui_element_t *c=bx_ui_find(p[i]);
                    if(!c){ fprintf(stderr,"ui attach: no element %s\n",p[i]); continue; }
                    bx_ui_child_add(par,c->id);
                    snprintf(c->parent,sizeof c->parent,"%s",par->id);
                }
            }
        }
        else if(streqi(fam,"detach") && n>=2){
            bx_ui_element_t *par=bx_ui_find(p[0]);
            if(par) bx_ui_child_remove(par,p[1]);
        }
        else if(streqi(fam,"list")){
            printf("ui elements (%d):\n", g_bx_ui.count);
            for(int i=0;i<g_bx_ui.count;i++){
                bx_ui_element_t *e=&g_bx_ui.els[i];
                printf("  %-14s %-9s parent=%-12s x=%g y=%g w=%g h=%g %s\n",
                       e->id, bx_ui_kind_name(e->kind), e->parent[0]?e->parent:"-",
                       e->x,e->y,e->w,e->h, e->visible?"":"(hidden)");
            }
            fflush(stdout);
        }
        else if(streqi(fam,"layout") && n>=1){
            bx_ui_element_t *e=bx_ui_find(p[0]);
            if(!e) fprintf(stderr,"ui layout: no frame %s\n",p[0]);
            else { bx_ui_layout_apply(p[0]); printf("ui: layout applied to %s (%d children)\n",p[0],e->child_count); fflush(stdout); }
        }
        else if(streqi(fam,"dock") && n>=1){
            bx_ui_element_t *e=bx_ui_find(p[0]);
            if(!e) fprintf(stderr,"ui dock: no pane %s\n",p[0]);
            else { bx_ui_dock_apply(e,bx_gfx_fb_get()); printf("ui: docked %s -> x=%g y=%g w=%g h=%g\n",p[0],e->x,e->y,e->w,e->h); fflush(stdout); }
        }
        else if(streqi(fam,"switch") && n>=1){
            /* pane switch: bring a pane forward and make it the active frame */
            bx_ui_element_t *e=bx_ui_find(p[0]);
            if(!e) fprintf(stderr,"ui switch: no pane %s\n",p[0]);
            else {
                int top=-1;
                for(int i=0;i<g_bx_ui.count;i++) if(g_bx_ui.els[i].z>top) top=g_bx_ui.els[i].z;
                e->z=top+1;
                snprintf(g_bx_ui.active_frame,sizeof g_bx_ui.active_frame,"%s",p[0]);
                for(int i=0;i<g_bx_ui.count;i++) g_bx_ui.els[i].focused = !strcmp(g_bx_ui.els[i].id,p[0]);
                printf("ui: switched to %s (z=%d)\n",p[0],e->z); fflush(stdout);
            }
        }
        else if(streqi(fam,"close") && n>=1){
            if(bx_ui_remove(p[0])!=0) fprintf(stderr,"ui close: no element %s\n",p[0]);
            else printf("ui: closed %s\n",p[0]);
        }
        /* -------------------------------------------------------- tweens */
        else if(streqi(fam,"tweenfn")){
            /* ui tweenfn set|NAME|X1 Y1 X2 Y2   cubic bezier control points,
             * the same form CSS and iOS use for timing curves
             * ui tweenfn set|NAME|M B            y = mx + b
             * ui tweenfn set|NAME|BUILTIN
             * ui tweenfn list                     */
            if(n<1 || streqi(p[0],"list")){
                printf("tween functions (%d):\n", bx_ui_fn_count());
                for(int i=0;i<bx_ui_fn_count();i++){ const bx_ui_fn_t *f=bx_ui_fn_at(i);
                    printf("  %-14s kind=%d",f->name,f->kind);
                    if(f->kind==BX_UI_FN_MXB) printf("  y=%gx+%g",f->p[0],f->p[1]);
                    else if(f->kind==BX_UI_FN_BEZIER) printf("  bezier(%g %g %g %g)",f->p[0],f->p[1],f->p[2],f->p[3]);
                    printf("\n"); }
                fflush(stdout);
            }
            else if(streqi(p[0],"set") && n>=2){
                char *name=xstrndup(p[1],BX_UI_ID_MAX-1);
                double v[4]={0,0,0,0}; int nv=0;
                /* Values may arrive as one bar field or several, so accept
                 * both "0.4 0 0.2 1" and "0.4|0|0.2|1". */
                for(int i=2;i<n;i++){
                    const char *q=p[i];
                    /* Accept the formula spelling too: "y=mx+b 2 0" means the
                     * same as "2 0", so parsing starts after the formula. */
                    const char *mx=strstr(q,"mx+b");
                    if(mx) q = mx+4;
                    else if(strchr(q,'=')) continue;
                    while(*q && nv<4){
                        char *endp=NULL; double d=strtod(q,&endp);
                        if(endp==q) break;
                        v[nv++]=d; q=endp;
                        while(*q==' '||*q==','||*q=='*') q++;
                    }
                }
                if(nv==0){
                    /* No "=" seen, so the values may be plain: retry raw. */
                    for(int i=2;i<n && nv<4;i++){
                        const char *q=p[i];
                        while(*q && nv<4){
                            char *endp=NULL; double d=strtod(q,&endp);
                            if(endp==q) break;
                            v[nv++]=d; q=endp;
                            while(*q==' '||*q==',') q++;
                        }
                    }
                }
                int rc;
                if(nv==4)      rc=bx_ui_fn_define_bezier(name,v[0],v[1],v[2],v[3]);
                else if(nv==2) rc=bx_ui_fn_define_mxb(name,v[0],v[1]);
                else           rc=bx_ui_fn_default(name);
                if(rc==0) printf("tweenfn: %s defined (%d values)\n",name,nv);
                else fprintf(stderr,"tweenfn: could not define %s\n",name);
                free(name); fflush(stdout);
            }
            else { fprintf(stderr,"ui tweenfn: set|NAME|... or list\n"); }
        }
        else if(streqi(fam,"tween")){
            /* ui tween|ID|TARGET|PROP|FROM|TO|DURATION|FN
             * ui tween|ID|to|VALUE     retarget in place
             * ui tween|ID|cancel
             * ui tween|list */
            if(n<1 || streqi(p[0],"list")){
                printf("tweens (%d running):\n", bx_ui_tween_running());
                for(int i=0;i<BX_UI_MAX_TWEENS;i++) if(g_bx_ui.tweens[i].used){
                    bx_ui_tween_t *t=&g_bx_ui.tweens[i];
                    printf("  %-12s %-10s %-3s from=%g to=%g value=%g fn=%s state=%d\n",
                           t->id,t->target,t->prop,t->from,t->to,t->value,t->fn,t->state);
                }
                fflush(stdout);
            }
            else if(n>=2 && streqi(p[1],"cancel")){ bx_ui_tween_cancel(p[0]); }
            else if(n>=3 && streqi(p[1],"to")){ bx_ui_tween_retarget(p[0],(float)atof(p[2])); }
            else if(n>=6){
                char *fn=xstrndup(n>=7?p[6]:"ease-in-out",BX_UI_ID_MAX-1);
                bx_ui_tween_start(p[0],p[1],p[2],(float)atof(p[3]),(float)atof(p[4]),(float)atof(p[5]),fn);
                free(fn);
            }
            else fprintf(stderr,"ui tween: need ID|TARGET|PROP|FROM|TO|DUR[|FN]\n");
        }
        else if(streqi(fam,"frame") && n>=1){
            /* ui frame|once   - one paced step (uses the real clock)
             * ui frame|fps|N  - set the pacing rate
             * ui frame|now    - print the frame counter and dt */
            if(streqi(p[0],"once")){ g_bx_ui.clock.single_shot=1; bx_ui_frame_step();
                printf("frame %llu dt=%.4f tweens=%d\n",(unsigned long long)g_bx_ui.clock.frame,g_bx_ui.clock.dt,bx_ui_tween_running()); fflush(stdout); }
            else if(streqi(p[0],"step") && n>=2){
                /* Advance by a fixed delta. Real pacing comes from ui frame
                 * once, but a fixed step makes an animation reproducible in a
                 * test, which the wall clock cannot be. */
                float dt=(float)atof(p[1]);
                g_bx_ui.clock.frame++;
                g_bx_ui.clock.dt=dt;
                bx_ui_tween_tick(dt);
                printf("frame %llu dt=%.4f tweens=%d\n",(unsigned long long)g_bx_ui.clock.frame,dt,bx_ui_tween_running()); fflush(stdout);
            }
            else if(streqi(p[0],"fps")&&n>=2){ bx_ui_clock_reset(atoi(p[1])); printf("frame: %d fps\n",g_bx_ui.clock.fps); fflush(stdout); }
            else if(streqi(p[0],"now")){ printf("frame %llu dt=%.4f\n",(unsigned long long)g_bx_ui.clock.frame,g_bx_ui.clock.dt); fflush(stdout); }
        }
        else if(streqi(fam,"ease")){
            /* ui ease|FN|T|BOX [T|BOX ...] - sample an easing function, so the
             * curve can be inspected without running an animation. The box may
             * be its own field or trail the number in the same field. */
            const char *fn = n>=1 && *p[0] ? p[0] : "ease-in-out";
            printf("ease %s:",fn);
            for(int i=1;i<n;i++){
                const char *q=p[i];
                while(*q){
                    char *endp=NULL; double t=strtod(q,&endp);
                    if(endp==q) break;
                    double v=bx_ui_ease(fn,t);
                    printf(" %g->%.4f",t,v);
                    const char *r=endp;
                    while(*r==' ') r++;
                    /* box name trailing in this same field */
                    if(*r && !isdigit((unsigned char)*r) && *r!='-' && *r!='.' && *r!=','){
                        char *nm=xstrndup(r,BX_UI_ID_MAX-1);
                        char buf[64]; snprintf(buf,sizeof buf,"%.6g",v);
                        char *dn=resolve(&pr->boxes,nm);
                        box_set(&pr->boxes,dn,buf);
                        free(dn); free(nm);
                        break;
                    }
                    /* box name in the next field */
                    if(i+1<n){
                        const char *nx=p[i+1];
                        if(*nx && !isdigit((unsigned char)*nx) && *nx!='-' && *nx!='.' && *nx!=','){
                            char buf[64]; snprintf(buf,sizeof buf,"%.6g",v);
                            char *dn=resolve(&pr->boxes,nx);
                            box_set(&pr->boxes,dn,buf);
                            free(dn);
                            i++;
                        }
                    }
                    q=endp;
                    while(*q==' '||*q==',') q++;
                }
            }
            printf("\n"); fflush(stdout);
        }
        else { printf("ui commands: init | kinds|list | new|ID|KIND [parent] | set|ID|field|val | get|ID|field | attach|FRAME|ID... | detach|FRAME|ID | list | layout|FRAME | dock|PANE | switch|PANE | close|ID | tween|ID|TARGET|PROP|FROM|TO|DUR|FN | tween|ID|to|V | tween|ID|cancel | tween|list | tweenfn|set|NAME|... | tweenfn|list | frame|once | frame|step|SECS | frame|fps|N | ease|FN|T...\n"); }
ui_done:
        free_parts(p,n); free(substr);
    }
#else
    else if (!strcmp(cmd,"ui") || !strncmp(cmd,"ui.",3)) {
        fprintf(stderr,"ui: the UI layer is not available in an embedded build\n");
    }
#endif
    else if (!strncmp(cmd,"high.gfx.",9)) {
        if(streqi(box_get(&pr->boxes,"lib_active_gfx"),"1")){
            const char *sub=cmd+9; int n; char **p=split_bars(args,&n);
            if(streqi(sub,"color") && n>=2){
                uint32_t c=bx_gfx_parse_color(p[1]); char buf[32]; snprintf(buf,sizeof buf,"%u",c);
                char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,buf); free(name);
            }
            else if(streqi(sub,"colorhex") && n>=2){
                Res cv=rfast(&pr->boxes,p[1]);
                uint32_t v = (uint32_t)strtoul(cv.ptr,NULL,10);
                if(cv.owned) free((char*)cv.ptr);
                char buf[16]; snprintf(buf,sizeof buf,"#%02x%02x%02x",(v>>24)&0xFF,(v>>16)&0xFF,(v>>8)&0xFF);                char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,buf); free(name);
            }
            else if(streqi(sub,"theme") && n>=2){
                /* Resolve a box reference first, so a theme can come from a
                 * box (including high.gfx.style output). Raw themes are left
                 * literal: they legitimately contain "$8" percentage tags and
                 * rfast would re-scan and corrupt them. Only resolve when the
                 * argument is a whole "$name". */
                const char *theme_arg = p[1];
                char thbuf[512] = {0};
                if (p[1][0] == '$' && p[1][1]) {
                    Res rt = rfast(&pr->boxes, p[1]);
                    snprintf(thbuf, sizeof thbuf, "%s", rt.ptr);
                    if (rt.owned) free((char *)rt.ptr);
                    theme_arg = thbuf;
                }
                bx_gfx_theme_t t; int rc=bx_gfx_parse_theme(theme_arg,&t);
                if(rc!=0) fprintf(stderr,"high.gfx theme: bad theme string\n");
                else { char buf[128];
                    snprintf(buf,sizeof buf,"c%d h%d r%d t%d x%d b%d",
                        t.primary_color,t.highlight_color,t.rounding,t.alpha,t.ternary_color,t.quaternary_color);
                    char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,buf); free(name); }
            }
            else if(streqi(sub,"new") && n>=5){
                /* new|id|parent|type|x|y [w|h|theme|box]  id=0 auto-assigns */
                Res ri=rfast(&pr->boxes,p[0]), rp=rfast(&pr->boxes,p[1]);
                uint32_t id=(uint32_t)strtoul(ri.ptr,NULL,10);
                uint32_t parent=(uint32_t)strtoul(rp.ptr,NULL,10);
                if(ri.owned)free((char*)ri.ptr); if(rp.owned)free((char*)rp.ptr);
                const char *tn = p[2];
                bx_gfx_type_t type;
                if(streqi(tn,"button"))type=BX_GFX_TYPE_BUTTON;
                else if(streqi(tn,"text"))type=BX_GFX_TYPE_TEXT;
                else if(streqi(tn,"slider"))type=BX_GFX_TYPE_SLIDER;
                else if(streqi(tn,"box"))type=BX_GFX_TYPE_BOX;
                else if(streqi(tn,"textbox"))type=BX_GFX_TYPE_TEXTBOX;
                else if(streqi(tn,"label"))type=BX_GFX_TYPE_LABEL;
                else if(streqi(tn,"image"))type=BX_GFX_TYPE_IMAGE;
                else if(streqi(tn,"panel"))type=BX_GFX_TYPE_PANEL;
                else { fprintf(stderr,"high.gfx new: unknown type %s\n",tn); free_parts(p,n); return pc+1; }
                Res rx=rfast(&pr->boxes,p[3]), ry=rfast(&pr->boxes,p[4]);
                int32_t x=(int32_t)strtol(rx.ptr,NULL,10), y=(int32_t)strtol(ry.ptr,NULL,10);
                if(rx.owned)free((char*)rx.ptr); if(ry.owned)free((char*)ry.ptr);
                uint32_t w = n>=6 ? (uint32_t)strtoul(p[5],NULL,10) : 0;
                uint32_t h = n>=7 ? (uint32_t)strtoul(p[6],NULL,10) : 0;
                const char *theme = n>=8 ? p[7] : "[c#fff.h#000.r$0.t%100.x#000.b#fff]";
                const char *boxname = n>=9 ? p[8] : NULL;
                const char *text = n>=10 ? p[9] : NULL;
                uint32_t real=0;
                int rc=bx_gfx_draw(parent,type,x,y,theme,id,w,h,boxname,&real);
                if(rc==0){
                    char buf[32]; snprintf(buf,sizeof buf,"%u",real);
                    char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,buf); free(name);
                    if(text && *text){ bx_gfx_element_t *el=bx_gfx_find_id(real);
                        if(el){ free(el->text); el->text=xstrdup(text); } }
                } else fprintf(stderr,"high.gfx new: draw failed (bad theme, or id %s already exists)\n",p[0]);
            }
            else if(streqi(sub,"set") && n>=3){
                /* set|id|field|value  fields: x y w h text theme */
                Res ri=rfast(&pr->boxes,p[0]);
                uint32_t id=(uint32_t)strtoul(ri.ptr,NULL,10);
                if(ri.owned)free((char*)ri.ptr);
                bx_gfx_element_t *el=bx_gfx_find_id(id);
                if(!el) fprintf(stderr,"high.gfx set: no element %s\n",p[0]);
                else {
                    Res rv=rfast(&pr->boxes,p[2]);
                    if(streqi(p[1],"x")) el->x=(int32_t)strtol(rv.ptr,NULL,10);
                    else if(streqi(p[1],"y")) el->y=(int32_t)strtol(rv.ptr,NULL,10);
                    else if(streqi(p[1],"w")) el->width=(uint32_t)strtoul(rv.ptr,NULL,10);
                    else if(streqi(p[1],"h")) el->height=(uint32_t)strtoul(rv.ptr,NULL,10);
                    else if(streqi(p[1],"text")){ free(el->text); el->text=xstrdup(rv.ptr); }
                    else if(streqi(p[1],"theme")){ bx_gfx_parse_theme(rv.ptr,&el->theme); }
                    else if(streqi(p[1],"box")){ free(el->value_box); el->value_box=xstrdup(rv.ptr); }
                    else fprintf(stderr,"high.gfx set: unknown field %s\n",p[1]);
                    if(rv.owned)free((char*)rv.ptr);
                }
            }
            else if(streqi(sub,"get") && n>=1 && p[0][0]){                Res ri=rfast(&pr->boxes,p[0]);
                uint32_t id=(uint32_t)strtoul(ri.ptr,NULL,10);
                if(ri.owned)free((char*)ri.ptr);
                bx_gfx_element_t *el=bx_gfx_find_id(id);
                if(!el){ char *name=resolve(&pr->boxes,p[1]?p[1]:p[0]); box_set(&pr->boxes,name,""); free(name); }
                else { char buf[256];
                    if(n>=3 && streqi(p[1],"text")) snprintf(buf,sizeof buf,"%s",el->text?el->text:"");
                    else if(n>=3 && streqi(p[1],"x")) snprintf(buf,sizeof buf,"%d",el->x);
                    else if(n>=3 && streqi(p[1],"y")) snprintf(buf,sizeof buf,"%d",el->y);
                    else if(n>=3 && streqi(p[1],"w")) snprintf(buf,sizeof buf,"%u",el->width);
                    else if(n>=3 && streqi(p[1],"h")) snprintf(buf,sizeof buf,"%u",el->height);
                    else if(n>=3 && streqi(p[1],"parent")) snprintf(buf,sizeof buf,"%u",el->parent_id);
                    else if(n>=3 && streqi(p[1],"type")) snprintf(buf,sizeof buf,"%s",
                            el->type==BX_GFX_TYPE_BUTTON?"button":el->type==BX_GFX_TYPE_TEXT?"text":
                            el->type==BX_GFX_TYPE_SLIDER?"slider":el->type==BX_GFX_TYPE_BOX?"box":
                            el->type==BX_GFX_TYPE_TEXTBOX?"textbox":el->type==BX_GFX_TYPE_LABEL?"label":
                            el->type==BX_GFX_TYPE_IMAGE?"image":"panel");
                    else if(n>=3 && streqi(p[1],"box")) snprintf(buf,sizeof buf,"%s",el->value_box?el->value_box:"");
                    else snprintf(buf,sizeof buf,"%s x%d y%d %ux%u",el->text?el->text:"",el->x,el->y,el->width,el->height);
                    if(n>=3){ char *name=resolve(&pr->boxes,p[2]); box_set(&pr->boxes,name,buf); free(name); }
                    else printf("%s\n",buf); }
            }
            else if(streqi(sub,"count")){
                char buf[32]; snprintf(buf,sizeof buf,"%u",g_bx_gfx.element_count);
                if(n>=1 && p[0][0]){ char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,buf); free(name); }
                else printf("%s\n",buf);
            }
            else if(streqi(sub,"list")){
                for(uint32_t i=0;i<g_bx_gfx.element_count;i++){
                    const char *tn="?";
                    switch(g_bx_gfx.elements[i].type){
                    case BX_GFX_TYPE_BUTTON:tn="button";break; case BX_GFX_TYPE_TEXT:tn="text";break;
                    case BX_GFX_TYPE_SLIDER:tn="slider";break; case BX_GFX_TYPE_BOX:tn="box";break;
                    case BX_GFX_TYPE_TEXTBOX:tn="textbox";break; case BX_GFX_TYPE_LABEL:tn="label";break;
                    case BX_GFX_TYPE_IMAGE:tn="image";break; case BX_GFX_TYPE_PANEL:tn="panel";break; }
                    printf("  %u %s %dx%d at %d,%d %s\n",g_bx_gfx.elements[i].id,tn,
                        g_bx_gfx.elements[i].width,g_bx_gfx.elements[i].height,
                        g_bx_gfx.elements[i].x,g_bx_gfx.elements[i].y,
                        g_bx_gfx.elements[i].text?g_bx_gfx.elements[i].text:"");
                }
            }
            else if(streqi(sub,"clear")){
                for(uint32_t i=0;i<g_bx_gfx.element_count;i++){
                    free(g_bx_gfx.elements[i].text); free(g_bx_gfx.elements[i].value_box);
                }
                free(g_bx_gfx.elements); g_bx_gfx.elements=NULL;
                g_bx_gfx.element_count=0; g_bx_gfx.element_capacity=0; g_bx_gfx.next_auto_id=1;
            }
            else if(streqi(sub,"types")){
                printf("button text slider box textbox label image panel\n");
            }
            else if(streqi(sub,"plot") && n>=3){
                uint32_t c;
                if(!gfx_color_arg(pr,p[2],&c)){ fprintf(stderr,"high.gfx plot: bad color %s\n",p[2]); }
                else bx_gfx_plot(bx_gfx_fb_get(),atoi(p[0]),atoi(p[1]),c);
            }
            else if(streqi(sub,"pixel") && n>=3){
                uint32_t c=bx_gfx_get(bx_gfx_fb_get(),atoi(p[1]),atoi(p[2]));
                char buf[16]; snprintf(buf,sizeof buf,"0x%08x",c); gfx_out(pr,p[0],buf);
            }
            else if(streqi(sub,"rgb") && n>=3){
                uint32_t c=bx_gfx_get(bx_gfx_fb_get(),atoi(p[1]),atoi(p[2]));
                char buf[32]; snprintf(buf,sizeof buf,"%u %u %u %u",BX_GFX_R(c),BX_GFX_G(c),BX_GFX_B(c),BX_GFX_A(c));
                gfx_out(pr,p[0],buf);
            }
            else if((streqi(sub,"line")||streqi(sub,"rect")||streqi(sub,"frame")) && n>=5){
                uint32_t c;
                if(!gfx_color_arg(pr,p[4],&c)) fprintf(stderr,"high.gfx %s: bad color %s\n",sub,p[4]);
                else { bx_gfx_fb_t *fb=bx_gfx_fb_get();
                    int a0=atoi(p[0]),a1=atoi(p[1]),a2=atoi(p[2]),a3=atoi(p[3]);
                    if(streqi(sub,"line"))       bx_gfx_line(fb,a0,a1,a2,a3,c);
                    else if(streqi(sub,"rect"))  bx_gfx_rect(fb,a0,a1,a2,a3,c);
                    else                         bx_gfx_rect_outline(fb,a0,a1,a2,a3,c);
                }
            }
            else if((streqi(sub,"circle")||streqi(sub,"ring")) && n>=4){
                /* circle and ring take a radius, not a width and height. */
                uint32_t c;
                if(!gfx_color_arg(pr,p[3],&c)) fprintf(stderr,"high.gfx %s: bad color %s\n",sub,p[3]);
                else { bx_gfx_fb_t *fb=bx_gfx_fb_get();
                    int cx=atoi(p[0]),cy=atoi(p[1]),r=atoi(p[2]);
                    if(streqi(sub,"circle")) bx_gfx_circle(fb,cx,cy,r,c);
                    else                    bx_gfx_circle_outline(fb,cx,cy,r,c);
                }
            }
            else if((streqi(sub,"tri")||streqi(sub,"triline")) && n>=7){
                uint32_t c;
                if(!gfx_color_arg(pr,p[6],&c)) fprintf(stderr,"high.gfx %s: bad color %s\n",sub,p[6]);
                else { bx_gfx_fb_t *fb=bx_gfx_fb_get();
                    float x0=(float)strtod(p[0],NULL),y0=(float)strtod(p[1],NULL);
                    float x1=(float)strtod(p[2],NULL),y1=(float)strtod(p[3],NULL);
                    float x2=(float)strtod(p[4],NULL),y2=(float)strtod(p[5],NULL);
                    if(streqi(sub,"tri")) bx_gfx_tri(fb,x0,y0,x1,y1,x2,y2,c);
                    else                  bx_gfx_tri_outline(fb,x0,y0,x1,y1,x2,y2,c);
                }
            }
            else if(streqi(sub,"poly") && n>=4){
                int np=atoi(p[0]);
                if(np<3||n<np*2+2){ fprintf(stderr,"high.gfx poly: need n>=3 points and n*2+2 fields\n"); }
                else { uint32_t c;
                    if(!gfx_color_arg(pr,p[np*2+1],&c)) fprintf(stderr,"high.gfx poly: bad color %s\n",p[np*2+1]);
                    else { float *pts=(float*)malloc(sizeof(float)*2*np);
                        for(int i=0;i<np;i++){ pts[i*2]=(float)atof(p[1+i*2]); pts[i*2+1]=(float)atof(p[2+i*2]); }
                        bx_gfx_poly(bx_gfx_fb_get(),pts,np,c); free(pts); }
                }
            }
            else if((streqi(sub,"gradv")||streqi(sub,"gradh")) && n>=5){
                uint32_t c0,c1;
                /* Two stop fields are the normal spelling; a single
                 * "c0,c1" field is also accepted. */
                int okc = (n>=6 && gfx_color_arg(pr,p[4],&c0) && gfx_color_arg(pr,p[5],&c1))
                       || gfx_color_pair(pr,p[4],&c0,&c1);
                if(!okc) fprintf(stderr,"high.gfx %s: bad colors %s\n",sub,p[4]);
                else { bx_gfx_fb_t *fb=bx_gfx_fb_get();
                    if(streqi(sub,"gradv")) bx_gfx_gradient_v(fb,atoi(p[0]),atoi(p[1]),atoi(p[2]),atoi(p[3]),c0,c1);
                    else                  bx_gfx_gradient_h(fb,atoi(p[0]),atoi(p[1]),atoi(p[2]),atoi(p[3]),c0,c1);
                }
            }
            else if(streqi(sub,"fbsize") && n>=2) bx_gfx_fb_set_size(atoi(p[0]),atoi(p[1]));
            else if(streqi(sub,"fbclear") && n>=1){
                uint32_t c; if(gfx_color_arg(pr,p[0],&c)) bx_gfx_fb_clear(bx_gfx_fb_get(),c);
                else bx_gfx_fb_clear(bx_gfx_fb_get(),0);
            }
            else if(streqi(sub,"fbinfo") && n>=1){
                bx_gfx_fb_t *fb=bx_gfx_fb_get(); char buf[64];
                snprintf(buf,sizeof buf,"%d %d",fb->width,fb->height); gfx_out(pr,p[0],buf);
            }
            else if(streqi(sub,"clip") && n>=4)
                bx_gfx_clip_set(bx_gfx_fb_get(),atoi(p[0]),atoi(p[1]),atoi(p[2]),atoi(p[3]));
            else if(streqi(sub,"clipreset")) bx_gfx_clip_reset(bx_gfx_fb_get());
            else if(streqi(sub,"push"))   bx_gfx_push(bx_gfx_fb_get());
            else if(streqi(sub,"pop"))    bx_gfx_pop(bx_gfx_fb_get());
            else if(streqi(sub,"identity")) bx_gfx_identity(bx_gfx_fb_get());
            else if(streqi(sub,"translate") && n>=2)
                bx_gfx_translate(bx_gfx_fb_get(),(float)strtod(p[0],NULL),(float)strtod(p[1],NULL));
            else if(streqi(sub,"scale") && n>=2)
                bx_gfx_scale(bx_gfx_fb_get(),(float)strtod(p[0],NULL),(float)strtod(p[1],NULL));
            else if(streqi(sub,"rotate") && n>=1)
                bx_gfx_rotate(bx_gfx_fb_get(),(float)strtod(p[0],NULL));
            else if(streqi(sub,"named") && n>=2){
                int found=0; uint32_t c=bx_gfx_color_named(p[1],&found);
                if(!found) gfx_out(pr,p[0],"");
                else { char buf[16]; snprintf(buf,sizeof buf,"0x%08x",c); gfx_out(pr,p[0],buf); }
            }
            else if(streqi(sub,"colors")){
                static const char *nm[]={"black","white","red","green","lime","blue","yellow",
                    "cyan","magenta","silver","gray","maroon","olive","navy","purple","teal",
                    "orange","pink","brown","gold","indigo","violet","turquoise","coral","salmon",
                    "crimson","khaki","plum","orchid","beige","ivory","azure","lavender",
                    "linen","snow","skyblue","steelblue","royalblue","forestgreen","seagreen",
                    "darkred","darkblue","darkgreen","darkgray","lightgray","lightblue","lightgreen",NULL};
                for(int i=0;nm[i];i++){ int f=0; uint32_t c=bx_gfx_color_named(nm[i],&f);
                    if(f) printf("%s=0x%08x ",nm[i],c); }
                printf("\n");
            }
            else if(streqi(sub,"lerp") && n>=4){
                uint32_t c0,c1;
                if(!gfx_color_arg(pr,p[1],&c0)||!gfx_color_arg(pr,p[2],&c1)) gfx_out(pr,p[0],"");
                else { char buf[16];
                    snprintf(buf,sizeof buf,"0x%08x",bx_gfx_color_lerp(c0,c1,atoi(p[3]))); gfx_out(pr,p[0],buf); }
            }
            else if(streqi(sub,"alpha") && n>=3){
                uint32_t c;
                if(!gfx_color_arg(pr,p[1],&c)) gfx_out(pr,p[0],"");
                else { char buf[16];
                    snprintf(buf,sizeof buf,"0x%08x",bx_gfx_color_scale_alpha(c,atoi(p[2]))); gfx_out(pr,p[0],buf); }
            }
            else if(streqi(sub,"style") && n>=2){
                const char *th=bx_gfx_style(p[1]);
                if(!th){ fprintf(stderr,"high.gfx style: unknown style %s\n",p[1]); gfx_out(pr,p[0],""); }
                else gfx_out(pr,p[0],th);
            }
            else if(streqi(sub,"styles")){
                int cnt=0; const char *const *names=bx_gfx_style_names(&cnt);
                for(int i=0;i<cnt;i++) printf("%s ",names[i]);
                printf("\n");
            }
            else if(streqi(sub,"shape") && n>=10){
                /* shape|BOX|PARENT|KIND|x0|y0|x1|y1|x2|y2|COLOR  (COLOR may be "c0,c1") */
                uint32_t c1=0,c2=0;
                if(!gfx_color_pair(pr,p[9],&c1,&c2)){ fprintf(stderr,"high.gfx shape: bad color %s\n",p[9]); }
                else { Res ri=rfast(&pr->boxes,p[0]), rp=rfast(&pr->boxes,p[1]);
                    uint32_t id=(uint32_t)strtoul(ri.ptr,NULL,10);
                    uint32_t par=(uint32_t)strtoul(rp.ptr,NULL,10);
                    if(ri.owned)free((char*)ri.ptr); if(rp.owned)free((char*)rp.ptr);
                    int32_t pts[6]; for(int i=0;i<6;i++) pts[i]=atoi(p[3+i]);
                    uint32_t real=0; int32_t w=pts[2],h=pts[3];
                    int rc=bx_gfx_draw(par,BX_GFX_TYPE_SHAPE,pts[0],pts[1],
                        "[c#fff.h#000.r$0.t%100.x#000.b#fff]",id,(uint32_t)(w>0?w:0),(uint32_t)(h>0?h:0),NULL,&real);
                    if(rc!=0) fprintf(stderr,"high.gfx shape: bad kind or duplicate id\n");
                    else { bx_gfx_element_t *el=bx_gfx_find_id(real);
                        if(!el||bx_gfx_shape_set(el,p[2],pts,c1,c2)!=0)
                            fprintf(stderr,"high.gfx shape: unknown kind %s\n",p[2]);
                        else { char buf[32]; snprintf(buf,sizeof buf,"%u",real); gfx_out(pr,p[0],buf); }
                    }
                }
            }
            else if(streqi(sub,"render")){
                int nsh=bx_gfx_render();
                if(nsh==0) fprintf(stderr,"high.gfx render: no shape elements\n");
            }
            else if(streqi(sub,"draw") && n>=1){
                /* draw|ID draws just one shape element. */
                Res ri=rfast(&pr->boxes,p[0]);
                uint32_t id=(uint32_t)strtoul(ri.ptr,NULL,10); if(ri.owned)free((char*)ri.ptr);
                if(bx_gfx_render_one(id)!=0) fprintf(stderr,"high.gfx draw: no shape element %s\n",p[0]);
            }
            else if(streqi(sub,"ppm") && n>=2){
                if(bx_gfx_ppm(p[1])!=0) fprintf(stderr,"high.gfx ppm: cannot write %s\n",p[1]);
                else gfx_out(pr,p[0],"ok");
            }
            else if(streqi(sub,"ascii") && n>=5){
                const char *ramp=(n>=6&&p[5][0])?p[5]:" .:-=+*#%@";
                char *art=bx_gfx_ascii(atoi(p[1]),atoi(p[2]),atoi(p[3]),atoi(p[4]),ramp);
                if(art){ gfx_out(pr,p[0],art); free(art); }
                else gfx_out(pr,p[0],"");
            }
            else { printf("high.gfx commands:\n");
                printf("  color|box|#rgb        parse a color to a number\n");
                printf("  colorhex|box|number    format a number back to #rrggbb\n");
                printf("  theme|box|[c..h..r..t..x..b..]\n");
                printf("  new|id|parent|type|x|y [w|h|theme|box|text]\n");
                printf("  set|id|field|value     field: x y w h text theme box\n");
                printf("  get|id [field]         read back an element\n");
                printf("  count [box]            number of elements\n");
                printf("  list                   print every element\n");
                printf("  clear                  drop every element\n");
                printf("  types                  element type names\n"); }
            free_parts(p,n);
        } else { fprintf(stderr,"gfx library not loaded: use 'lib load|gfx'\n"); fflush(stderr); }
    }
    else if (!strncmp(cmd,"high.snd.",9)) {
        if(streqi(box_get(&pr->boxes,"lib_active_snd"),"1")){
            const char *sub=cmd+9; int n; char **p=split_bars(args,&n);

            int *pown=(int*)calloc((size_t)(n?n:1),sizeof(int));
            const char **q=(const char**)calloc((size_t)(n?n:1),sizeof *q);
            for(int iq=0;iq<n;iq++){ Res gg=rfast(&pr->boxes,p[iq]); q[iq]=gg.ptr; pown[iq]=gg.owned; }
            double defA=atof(box_get(&pr->boxes,"snd_env_a")); if(defA<=0) defA=0.005;
            double defD=atof(box_get(&pr->boxes,"snd_env_d")); if(defD<0)  defD=0.05;
            double defS=atof(box_get(&pr->boxes,"snd_env_s")); if(defS<0)  defS=0.6;
            double defR=atof(box_get(&pr->boxes,"snd_env_r")); if(defR<0)  defR=0.05;
            if(streqi(sub,"init")){ if(n>=2) bx_snd_init((uint32_t)strtoul(q[1],NULL,10)); else bx_snd_init(0);
                printf("high.snd: rate=%u\n",bx_snd_rate()); fflush(stdout); }
            else if(streqi(sub,"rate") && n>=1){ char buf[32]; snprintf(buf,sizeof buf,"%u",bx_snd_rate()); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"samples") && n>=1){ char buf[32]; snprintf(buf,sizeof buf,"%u",bx_snd_samples()); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"voices") && n>=1){ char buf[32]; snprintf(buf,sizeof buf,"%d",bx_snd_voice_count()); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"freq") && n>=2){ int m=(int)strtol(q[1],NULL,10); char buf[32]; snprintf(buf,sizeof buf,"%.3f",bx_snd_note_freq(m)); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"midi") && n>=2){ int m=bx_snd_note_from_name(q[1]); char buf[32]; snprintf(buf,sizeof buf,"%d",m); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"env")){
                if(n>=5){ box_set(&pr->boxes,"snd_env_a",q[1]); box_set(&pr->boxes,"snd_env_d",q[2]);
                          box_set(&pr->boxes,"snd_env_s",q[3]); box_set(&pr->boxes,"snd_env_r",q[4]);
                          printf("high.snd env %s %s %s %s\n",q[1],q[2],q[3],q[4]); }
                else printf("high.snd env: %s %s %s %s\n",(defA>0?"0.005":"0"),"0.05","0.6","0.05");
                fflush(stdout);
            }
            else if(streqi(sub,"tone") && n>=3){
                double freq=atof(q[1]), dur=atof(q[2]);
                bx_snd_wave_t w=snd_wave(n>=4?q[3]:"",BX_SND_WAVE_SINE);
                double vol=n>=5?atof(q[4]):0.5;
                double a=n>=6?atof(q[5]):defA, d=n>=7?atof(q[6]):defD;
                double s=n>=8?atof(q[7]):defS, r=n>=9?atof(q[8]):defR;
                int rc=bx_snd_tone(w,freq,vol,dur,a,d,s,r);
                gfx_out(pr,q[0],rc==0?"ok":"err");
            }
            else if(streqi(sub,"note") && n>=3){
                int midi=(int)strtol(q[1],NULL,10); double dur=atof(q[2]);
                bx_snd_wave_t w=snd_wave(n>=4?q[3]:"",BX_SND_WAVE_SINE);
                double vol=n>=5?atof(q[4]):0.5;
                double a=n>=6?atof(q[5]):defA, d=n>=7?atof(q[6]):defD;
                double s=n>=8?atof(q[7]):defS, r=n>=9?atof(q[8]):defR;
                int rc=bx_snd_note(w,midi,vol,dur,a,d,s,r);
                gfx_out(pr,q[0],rc==0?"ok":"err");
            }
            else if(streqi(sub,"melody") && n>=3){
                double dur=atof(q[2]);
                bx_snd_wave_t w=snd_wave(n>=4?q[3]:"",BX_SND_WAVE_SINE);
                double vol=n>=5?atof(q[4]):0.5;
                double off=n>=6?atof(q[5]):0.0;
                double a=n>=7?atof(q[6]):defA, d=n>=8?atof(q[7]):defD;
                double s=n>=9?atof(q[8]):defS, r=n>=10?atof(q[9]):defR;
                int rc=bx_snd_melody(q[1],dur,w,vol,off,a,d,s,r);
                char buf[16]; snprintf(buf,sizeof buf,"%d",rc); gfx_out(pr,q[0],buf);
            }
            else if(streqi(sub,"render") && n>=1){ char buf[32]; snprintf(buf,sizeof buf,"%u",bx_snd_render()); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"clear")){ bx_snd_clear(); printf("high.snd: cleared\n"); fflush(stdout); }
            else if(streqi(sub,"wav") && n>=2){ int rc=bx_snd_wav(q[1]); gfx_out(pr,q[0],rc==0?"ok":"err"); }
            else if(streqi(sub,"info") && n>=1){
                char buf[96];
                snprintf(buf,sizeof buf,"rate=%u voices=%d samples=%u used=%u",
                         bx_snd_rate(),bx_snd_voice_count(),bx_snd_samples(),bx_snd_samples());
                gfx_out(pr,q[0],buf);
            }
            else { printf("high.snd commands:\n");
                printf("  init|rate | rate|BOX | samples|BOX | voices|BOX\n");
                printf("  freq|BOX|midi | midi|BOX|name | env|a|d|s|r\n");
                printf("  tone|BOX|hz|sec|wave|vol | note|BOX|midi|sec|wave|vol\n");
                printf("  melody|BOX|names|sec|wave|vol|off\n");
                printf("  render|BOX | wav|BOX|path | clear\n"); }
            for(int iq=0;iq<n;iq++) if(pown[iq]) free((char*)q[iq]);
            free(pown); free(q);
            free_parts(p,n);
        } else { fprintf(stderr,"snd library not loaded: use 'lib load|snd'\n"); fflush(stderr); }
    }
#ifndef BX_EMBEDDED_SOURCE
    else if (!strncmp(cmd,"high.math.",10)) {
        if(streqi(box_get(&pr->boxes,"lib_active_math"),"1")){
            const char *sub=cmd+10; int n; char **p=split_bars(args,&n);
            int *pown=(int*)calloc((size_t)(n?n:1),sizeof(int));
            const char **q=(const char**)calloc((size_t)(n?n:1),sizeof *q);
            for(int iq=0;iq<n;iq++){ Res gg=rfast(&pr->boxes,p[iq]); q[iq]=gg.ptr; pown[iq]=gg.owned; }

            char buf[256];
#define LLO(s) ((long long)strtoll((s),NULL,10))
#define DBL(s) (atof(s))
            if(streqi(sub,"gcd")&&n>=3) { snprintf(buf,sizeof buf,"%lld",bx_math_gcd(LLO(q[1]),LLO(q[2]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"lcm")&&n>=3) { snprintf(buf,sizeof buf,"%lld",bx_math_lcm(LLO(q[1]),LLO(q[2]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"fact")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_fact((int)LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"fib")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_fib((int)LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"isprime")&&n>=2) { snprintf(buf,sizeof buf,"%d",bx_math_isprime(LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"nthprime")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_nthprime((int)LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"ncr")&&n>=3) { snprintf(buf,sizeof buf,"%lld",bx_math_ncr(LLO(q[1]),LLO(q[2]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"npr")&&n>=3) { snprintf(buf,sizeof buf,"%lld",bx_math_npr(LLO(q[1]),LLO(q[2]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"isqrt")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_isqrt(LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"abs")&&n>=2) { double v=DBL(q[1]); snprintf(buf,sizeof buf,"%g",fabs(v)); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"sign")&&n>=2) { double v=DBL(q[1]); snprintf(buf,sizeof buf,"%g",v<0?-1.0:(v>0?1.0:0.0)); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"floor")&&n>=2) { snprintf(buf,sizeof buf,"%g",floor(DBL(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"ceil")&&n>=2) { snprintf(buf,sizeof buf,"%g",ceil(DBL(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"round")&&n>=2) { snprintf(buf,sizeof buf,"%g",round(DBL(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"trunc")&&n>=2) { snprintf(buf,sizeof buf,"%g",trunc(DBL(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"clamp")&&n>=4) { double v=DBL(q[1]),lo=DBL(q[2]),hi=DBL(q[3]); v=v<lo?lo:(v>hi?hi:v); snprintf(buf,sizeof buf,"%g",v); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"lerp")&&n>=4) { double a=DBL(q[1]),b=DBL(q[2]),t=DBL(q[3]); snprintf(buf,sizeof buf,"%g",bx_math_fpclean(a+(b-a)*t)); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"min")&&n>=3) { double a=DBL(q[1]),b=DBL(q[2]); snprintf(buf,sizeof buf,"%g",a<b?a:b); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"max")&&n>=3) { double a=DBL(q[1]),b=DBL(q[2]); snprintf(buf,sizeof buf,"%g",a>b?a:b); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"sqrt")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(sqrt(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"root")&&n>=3) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(pow(DBL(q[1]),1.0/DBL(q[2])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"pow")&&n>=3) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(pow(DBL(q[1]),DBL(q[2])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"exp")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(exp(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"log")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(log(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"log2")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(log(DBL(q[1]))/log(2.0))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"log10")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(log10(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"hypot")&&n>=3) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(hypot(DBL(q[1]),DBL(q[2])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"sind")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(sin(DBL(q[1])*0.017453292519943295))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"cosd")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(cos(DBL(q[1])*0.017453292519943295))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"tand")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(tan(DBL(q[1])*0.017453292519943295))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"sin")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(sin(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"cos")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(cos(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"tan")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(tan(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"asin")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(asin(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"acos")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(acos(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"atan")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(atan(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"atan2")&&n>=3) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(atan2(DBL(q[1]),DBL(q[2])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"sinh")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(sinh(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"cosh")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(cosh(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"tanh")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(tanh(DBL(q[1])))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"deg2rad")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(DBL(q[1])*0.017453292519943295)); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"rad2deg")&&n>=2) { snprintf(buf,sizeof buf,"%g",bx_math_fpclean(DBL(q[1])*57.29577951308232)); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"totient")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_totient(LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"divcount")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_divcount(LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"divsum")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_divsum(LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"digitsum")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_digitsum(LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"droot")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_droot(LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"collatz")&&n>=2) { snprintf(buf,sizeof buf,"%lld",bx_math_collatz(LLO(q[1]))); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"factor")&&n>=2) { bx_math_factor(LLO(q[1]),buf,sizeof buf); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"frac")&&n>=2) { long long num,den; bx_math_frac(DBL(q[1]),&num,&den); snprintf(buf,sizeof buf,"%lld %lld",num,den); gfx_out(pr,q[0],buf); }
            else { printf("high.math commands:\n");
                printf("  gcd|lcm|fact|fib|isprime|nthprime|ncr|npr|isqrt\n");
                printf("  abs|sign|floor|ceil|round|trunc|clamp|lerp|min|max\n");
                printf("  sqrt|root|pow|exp|log|log2|log10|hypot\n");
                printf("  sin|cos|tan|asin|acos|atan|atan2|sinh|cosh|tanh\n");
                printf("  sind|cosd|tand|deg2rad|rad2deg\n");
                printf("  totient|divcount|divsum|digitsum|droot|collatz|factor|frac\n"); }
#undef LLO
#undef DBL
            for(int iq=0;iq<n;iq++) if(pown[iq]) free((char*)q[iq]); free(pown); free(q); free_parts(p,n);
        } else { fprintf(stderr,"math library not loaded: use 'lib load|math'\n"); fflush(stderr); }
    }
    else if (!strncmp(cmd,"high.m3d.",9)) {
        if(streqi(box_get(&pr->boxes,"lib_active_math"),"1")){
            const char *sub=cmd+9; int n; char **p=split_bars(args,&n);
            int *pown=(int*)calloc((size_t)(n?n:1),sizeof(int));
            const char **q=(const char**)calloc((size_t)(n?n:1),sizeof *q);
            for(int iq=0;iq<n;iq++){ Res gg=rfast(&pr->boxes,p[iq]); q[iq]=gg.ptr; pown[iq]=gg.owned; }

            char buf[512];
            if(streqi(sub,"vlen")&&n>=2) { double l; if(bx_math_vec_len(q[1],&l)==0){ snprintf(buf,sizeof buf,"%g",l); gfx_out(pr,q[0],buf); } else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"vnorm")&&n>=2) { if(bx_math_vec_norm(q[1],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"vadd")&&n>=3) { if(bx_math_vec_add(q[1],q[2],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"vsub")&&n>=3) { if(bx_math_vec_sub(q[1],q[2],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"vscale")&&n>=3) { if(bx_math_vec_scale(q[1],atof(q[2]),buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"vdot")&&n>=3) { double d; if(bx_math_vec_dot(q[1],q[2],&d)==0){ snprintf(buf,sizeof buf,"%g",d); gfx_out(pr,q[0],buf); } else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"vcross")&&n>=3) { if(bx_math_vec_cross(q[1],q[2],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"mident")&&n>=1) { bx_math_mat_identity(buf,sizeof buf); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"mtrans")&&n>=4) { char xyz[64]; snprintf(xyz,sizeof xyz,"%f %f %f",atof(q[1]),atof(q[2]),atof(q[3])); bx_math_mat_translate(xyz,buf,sizeof buf); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"mscale")&&n>=4) { char xyz[64]; snprintf(xyz,sizeof xyz,"%f %f %f",atof(q[1]),atof(q[2]),atof(q[3])); bx_math_mat_scale(xyz,buf,sizeof buf); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"mrotx")&&n>=2) { bx_math_mat_rotx(atof(q[1]),buf,sizeof buf); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"mroty")&&n>=2) { bx_math_mat_roty(atof(q[1]),buf,sizeof buf); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"mrotz")&&n>=2) { bx_math_mat_rotz(atof(q[1]),buf,sizeof buf); gfx_out(pr,q[0],buf); }
            else if(streqi(sub,"mrot")&&n>=3) { if(bx_math_mat_rot(q[1],atof(q[2]),buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"mmul")&&n>=3) { if(bx_math_mat_mul(q[1],q[2],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"mvec")&&n>=3) { if(bx_math_mat_vec(q[1],q[2],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"qaxis")&&n>=5) { if(bx_math_quat_axis(atof(q[1]),atof(q[2]),atof(q[3]),atof(q[4]),buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"qmul")&&n>=3) { if(bx_math_quat_mul(q[1],q[2],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"qconj")&&n>=2) { if(bx_math_quat_conj(q[1],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"qnorm")&&n>=2) { if(bx_math_quat_norm(q[1],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else if(streqi(sub,"qrot")&&n>=3) { if(bx_math_quat_rot(q[1],q[2],buf,sizeof buf)==0) gfx_out(pr,q[0],buf); else gfx_out(pr,q[0],"err"); }
            else { printf("high.m3d commands:\n");
                printf("  vlen|vnorm|vadd|vsub|vscale|vdot|vcross (vecs as \"x y z\")\n");
                printf("  mident|mtrans|mscale|mrotx|mroty|mrotz|mrot|mmul|mvec\n");
                printf("  qaxis|qmul|qconj|qnorm|qrot\n"); }
            for(int iq=0;iq<n;iq++) if(pown[iq]) free((char*)q[iq]); free(pown); free(q); free_parts(p,n);
        } else { fprintf(stderr,"math library not loaded: use 'lib load|math'\n"); fflush(stderr); }
    }
#endif
    else if (!strncmp(cmd,"low.",4)) {
        const char *sub=cmd+4; int n; char **p=split_bars(args,&n);
        if(streqi(sub,"arch")){ if(n>=1){ char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,BX_ARCH); free(name); } else puts(BX_ARCH); }
        else if(streqi(sub,"env")){ bx_wifi_env_t e=bx_wifi_detect_env(); const char *s=e==BX_WIFI_ENV_BAREMETAL?"BAREMETAL":(e==BX_WIFI_ENV_NATIVE?"NATIVE":"WEB"); if(n>=1){ char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,s); free(name); } else puts(s); }
        else if(streqi(sub,"tick")){ struct timespec ts; clock_gettime(CLOCK_MONOTONIC,&ts); long ns=(long)ts.tv_sec*1000000000L+ts.tv_nsec; char buf[64]; snprintf(buf,sizeof buf,"%ld",ns); if(n>=1){ char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,buf); free(name); } else puts(buf); }
        else if(streqi(sub,"pid")){ char buf[64]; snprintf(buf,sizeof buf,"%ld",(long)getpid()); if(n>=1){ char *name=resolve(&pr->boxes,p[0]); box_set(&pr->boxes,name,buf); free(name); } else puts(buf); }
        else { printf("low commands: arch | env | tick|box | pid|box\n"); }
        free_parts(p,n);
    }
    else if (streqi(cmd,"umload")) { int n; char **p=split_bars(args,&n); if(n>=1){
        const char *action=p[0];
        if(streqi(action,"load")){
            if(n<2){ fprintf(stderr,"umload load: usage load|path|mark\n"); }
            else {
                const char *mark=NULL;
                if(n>=3 && !streqi(p[2],"m")) mark=p[2];
                else if(n>=4) mark=p[3];
                int rc=umload_run_module(pr,p[1],mark);
                if(rc>0){ free_parts(p,n); free(line); return rc; }
            }
        }
        else if(streqi(action,"require")){
            if(n<2){ fprintf(stderr,"umload require: usage require|name|mark\n"); }
            else {
                const char *mark=NULL;
                if(n>=3 && !streqi(p[2],"m")) mark=p[2];
                else if(n>=4) mark=p[3];
                char path[512];
                if(umload_resolve_path(p[1],path,sizeof path)!=0){
                    if(umload_pkg_install(p[1], n>=4?p[3]:NULL, 0)!=0) fprintf(stderr,"umload require: package %s not found\n",p[1]);
                }
                if(umload_resolve_path(p[1],path,sizeof path)==0){
                    if(umload_require_deps(pr,p[1],"",0)!=0) fprintf(stderr,"umload: missing dependency for %s\n",p[1]);
                    int rc=umload_run_module(pr,path,mark);
                    if(rc>0){ free_parts(p,n); free(line); return rc; }
                    char key[160]; snprintf(key,sizeof key,"pkg_%s",p[1]);
                    box_set(&pr->boxes,key,"1");
                }
            }
        }
        else if(streqi(action,"install")){
            if(n<2){ fprintf(stderr,"umload install: usage install|name|url\n"); }
            else {
                const char *name=p[1];
                const char *url = n>=3?p[2]:NULL;
                if(umload_pkg_install(name,url,0)==0){
                    char key[160]; snprintf(key,sizeof key,"pkg_%s",name);
                    box_set(&pr->boxes,key,"1");
                }
            }
        }
        else if(streqi(action,"remove") || streqi(action,"uninstall")){
            if(n<2) fprintf(stderr,"umload remove: usage remove|name\n");
            else {
                char path[512];
                char key[192];
                umload_cache_pkg(path,sizeof path,p[1]);
                snprintf(key,sizeof key,"pkg_%s_installed",p[1]); box_set(&pr->boxes,key,"0");
                snprintf(key,sizeof key,"pkg_%s_cached",p[1]);    box_set(&pr->boxes,key,"0");
                if(access(path,F_OK)!=0) { printf("umload: %s not installed\n",p[1]); fflush(stdout); }
                else if(remove(path)==0){ printf("umload: removed %s\n",p[1]); fflush(stdout); }
                else fprintf(stderr,"umload: failed to remove %s\n",p[1]);
            }
        }
        else if(streqi(action,"list")){
            int found=0;
            printf("Loaded packages:\n");
            for(size_t i=0;i<pr->boxes.len;i++){
                const char *nm=pr->boxes.items[i].name;
                if(!strncmp(nm,"pkg_",4) && !strstr(nm,"_loaded")){
                    printf("  %s\n",nm+4); found=1;
                }
            }
            if(!found) printf("  (none)\n");
            fflush(stdout);

            char root[512]; umload_cache_dir(root,sizeof root);
            DIR *d = opendir(root);
            int cached=0;
            if(d){
                struct dirent *de;
                while((de=readdir(d))){
                    size_t l=strlen(de->d_name);
                    if(l>3 && !strcmp(de->d_name+l-3,".bx")){
                        char nm[256]; snprintf(nm,sizeof nm,"%.*s",(int)(l-3),de->d_name);
                        if(umload_loaded_has(pr,nm)) continue;
                        char p2[640]; snprintf(p2,sizeof p2,"%s/%s",root,de->d_name);
                        char ver[64]="?"; umload_pkg_meta(p2,"version",ver,sizeof ver);
                        if(!cached) printf("Cached packages:\n");
                        printf("  %s %s\n",nm,ver); cached=1;
                    }
                }
                closedir(d);
            }
            if(!cached && found) printf("Cached packages:\n  (none)\n");
        }
        else if(streqi(action,"info")){
            if(n<2){ fprintf(stderr,"umload info: usage info|name\n"); }
            else {
                char path[512];
                char key[192];
                if(umload_resolve_path(p[1],path,sizeof path)!=0){
                    /* Clear every published field so the result stays accurate
                     * after a package is removed, rather than leaving the value
                     * from a previous `umload info` in place. */
                    static const char *fields[] = {"_installed","_cached","_path","_version","_author","_description","_deps",0};
                    for (int fi=0; fields[fi]; fi++) {
                        snprintf(key,sizeof key,"pkg_%s%s",p[1],fields[fi]);
                        box_set(&pr->boxes, key, fields[fi][1]=='i' ? "0" : "");
                    }
                    printf("%s: not installed\n",p[1]); fflush(stdout);
                }
                else {
                    char val[512];
                    /* Also publish every field as a box so scripts and tests can
                     * assert on package metadata without scraping stdout. */
                    snprintf(key,sizeof key,"pkg_%s_installed",p[1]); box_set(&pr->boxes,key,"1");
                    snprintf(key,sizeof key,"pkg_%s_cached",p[1]);
                    box_set(&pr->boxes,key, umload_path_is_cached(path) ? "1" : "0");
                    snprintf(key,sizeof key,"pkg_%s_path",p[1]);      box_set(&pr->boxes,key,path);
                    printf("name: %s\n",p[1]);
                    if(umload_pkg_meta(path,"version",val,sizeof val)==0){
                        snprintf(key,sizeof key,"pkg_%s_version",p[1]); box_set(&pr->boxes,key,val);
                        printf("version: %s\n",val); }
                    else { snprintf(key,sizeof key,"pkg_%s_version",p[1]); box_set(&pr->boxes,key,""); }
                    if(umload_pkg_meta(path,"author",val,sizeof val)==0){
                        snprintf(key,sizeof key,"pkg_%s_author",p[1]); box_set(&pr->boxes,key,val);
                        printf("author: %s\n",val); }
                    else { snprintf(key,sizeof key,"pkg_%s_author",p[1]); box_set(&pr->boxes,key,""); }
                    if(umload_pkg_meta(path,"description",val,sizeof val)==0){
                        snprintf(key,sizeof key,"pkg_%s_description",p[1]); box_set(&pr->boxes,key,val);
                        printf("description: %s\n",val); }
                    else { snprintf(key,sizeof key,"pkg_%s_description",p[1]); box_set(&pr->boxes,key,""); }
                    if(umload_pkg_meta(path,"deps",val,sizeof val)==0){
                        snprintf(key,sizeof key,"pkg_%s_deps",p[1]); box_set(&pr->boxes,key,val);
                        printf("deps: %s\n",val); }
                    else { snprintf(key,sizeof key,"pkg_%s_deps",p[1]); box_set(&pr->boxes,key,"none"); }
                    printf("path: %s\n",path);
                    fflush(stdout);
                }
            }
        }
        else if(streqi(action,"deps") || streqi(action,"tree")){
            if(n<2){ fprintf(stderr,"umload deps: usage deps|name\n"); }
            else umload_show_deps(pr,p[1],0);
        }
        else if(streqi(action,"verify")){
            int n2; char **p2=split_bars(args,&n2);
            (void)p2;
            if(n<2){ fprintf(stderr,"umload verify: usage verify|name\n"); }
            else {
                if(umload_require_deps(pr,p[1],"",0)==0) printf("umload: %s and all dependencies satisfied\n",p[1]);
                else printf("umload: %s has missing dependencies\n",p[1]);
                fflush(stdout);
            }
            if(n2) free_parts(p2,n2);
        }
        else if(streqi(action,"search")){
            const char *q = n>=2?p[1]:"";
            char reg[512];
            umload_registry_path(reg,sizeof reg);
            FILE *rf = fopen(reg,"rb");
            if(!rf){ printf("umload search: no registry at %s\n",reg); fflush(stdout); }
            else {
                char line[1024];
                int hits=0;
                while(fgets(line,sizeof line,rf)){
                    umload_meta_trim(line);
                    if(!line[0]||line[0]=='#') continue;
                    int m; char **f=split_bars(line,&m);
                    char desc[256]="";
                    if(m>=4) snprintf(desc,sizeof desc,"%s",f[3]);
                    int match = !q[0];
                    if(!match){
                        for(int i=0;i<m;i++) if(bx_casestr(f[i],q)) { match=1; break; }
                    }
                    if(match){
                        printf("  %s %s\n", m>=1?f[0]:"?", m>=3?f[2]:"?");
                        if(desc[0]) printf("    %s\n",desc);
                        hits++;
                    }
                    free_parts(f,m);
                }
                fclose(rf);
                if(!hits) printf("  no packages found\n");
                fflush(stdout);
            }
        }
        else if(streqi(action,"publish")){
            if(n<2){ fprintf(stderr,"umload publish: usage publish|name|version|source\n"); }
            else {
                const char *name=p[1];
                const char *version=n>=3?p[2]:"1.0.0";
                char src[512];
                if(n>=4) snprintf(src,sizeof src,"%s",p[3]);
                else snprintf(src,sizeof src,"./packages/%s.bx",name);
                if(access(src,F_OK)!=0){ fprintf(stderr,"umload publish: %s not found\n",src); }
                else {
                    char val[256];
                    char *author = (umload_pkg_meta(src,"author",val,sizeof val)==0) ? strdup(val) : strdup("unknown");
                    char *desc   = (umload_pkg_meta(src,"description",val,sizeof val)==0) ? strdup(val) : strdup("");
                    /* Metadata is interpolated into a shell command, so drop
                     * quotes and shell metacharacters from it. */
                    umload_shell_sanitize(author);
                    umload_shell_sanitize(desc);
                    char baseurl[512];
                    snprintf(baseurl,sizeof baseurl,
                        "https://raw.githubusercontent.com/mc20000-01/BoxPkg/main/packages/%s.bx",name);
                    char cmd[2048];
                    snprintf(cmd,sizeof cmd,
                        "mkdir -p boxpkg/packages && cp '%s' 'boxpkg/packages/%s.bx' && "
                        "(grep -v '^%s|' boxpkg/registry.txt 2>/dev/null; printf '%%s|%%s|%%s|%%s|%%s\\n' '%s' '%s' '%s' '%s' '%s') > boxpkg/registry.txt.tmp && "
                        "mv boxpkg/registry.txt.tmp boxpkg/registry.txt && "
                        "git -C boxpkg add packages/%s.bx registry.txt && "
                        "git -C boxpkg commit -q -m 'publish %s@%s' && git -C boxpkg push",
                        src,name,name,
                        name,baseurl,version,desc,author,
                        name,name,version);
                    int rc=system(cmd);
                    if(rc==0) printf("umload: published %s@%s\n",name,version);
                    else fprintf(stderr,"umload: publish failed (is ./boxpkg set up and connected?)\n");
                    free(author); free(desc);
                }
                fflush(stdout);
            }
        }
        else if(streqi(action,"cache")){
            if(n>=2 && streqi(p[1],"dir")){ char root[512]; umload_cache_dir(root,sizeof root); printf("%s\n",root); fflush(stdout); }
            else if(n>=2 && streqi(p[1],"clear")){
                char root[512]; umload_cache_dir(root,sizeof root);
                char cmd[768]; snprintf(cmd,sizeof cmd,"rm -f '%s'/*.bx",root);
                int rc=system(cmd);
                printf("umload: cache cleared (%s)\n", rc==0?"ok":"partial");
                fflush(stdout);
            }
            else fprintf(stderr,"umload cache: usage cache|dir or cache|clear\n");
        }
        else {
            printf("umload commands:\n");
            printf("  load|path|mark      run a module file, return to mark\n");
            printf("  require|name|mark   install if needed, load deps, run package\n");
            printf("  install|name|url    install from url, registry, or ./packages\n");
            printf("  remove|name         delete cached package\n");
            printf("  list                list loaded and cached packages\n");
            printf("  info|name           show package metadata\n");
            printf("  deps|name           show dependency tree\n");
            printf("  verify|name         install deps without running\n");
            printf("  search|query        search the registry\n");
            printf("  publish|name|ver    publish to ./boxpkg and push\n");
            printf("  cache|dir           show cache directory\n");
            printf("  cache|clear         empty the cache\n");
            fflush(stdout);
        }
        free_parts(p,n);
    }}

    else if (streqi(cmd,"clear")) { fputs("\033[2J\033[H", stdout); }
    else if (streqi(cmd,"end")) { pr->halted = 1; free(line); return pr->count; }
    free(line); return pc + 1;
}
static int exec_line(Program *pr, const char *raw, int pc) { return exec_command(pr, raw, pc); }

static int exec_op(Program *pr, Op *op, int pc) {
    int n=op->n; char **p=op->parts;
    switch(op->kind){
    case OP_BOX: if(n>=2){ Res rn=rfast(&pr->boxes,p[0]); Res rv=rfast(&pr->boxes,p[1]); box_set(&pr->boxes,rn.ptr,rv.ptr); if(rn.owned) free((char*)rn.ptr); if(rv.owned) free((char*)rv.ptr); } return pc+1;
    case OP_SAY: { Res rt={0,0}; char *text; if(n){ rt=rfast(&pr->boxes,p[0]); text=(char*)rt.ptr; } else { puts("\n"); return pc+1; } puts(text); if(stdout_is_tty()) fflush(stdout); if(n>=2){ long sec=bx_int(&pr->boxes,p[1]); if(sec>0){ struct timespec ts={sec,0}; nanosleep(&ts,NULL); } } if(rt.owned) free((char*)rt.ptr); return pc+1; }
    case OP_MATH: if(n>=4){ long a=bx_int(&pr->boxes,p[1]), b=bx_int(&pr->boxes,p[2]), v=0; Res ro=rfast(&pr->boxes,p[3]); const char *op2=ro.ptr; if(!strcmp(op2,"+"))v=a+b; else if(!strcmp(op2,"-"))v=a-b; else if(!strcmp(op2,"*")||streqi(op2,"x"))v=a*b; else if(!strcmp(op2,"/"))v=b? a/b:0; else if(!strcmp(op2,"%"))v=b? a%b:0; if(ro.owned) free((char*)ro.ptr); char buf[64]; snprintf(buf,sizeof buf,"%ld",v); Res rn=rfast(&pr->boxes,p[0]); box_set(&pr->boxes,rn.ptr,buf); if(rn.owned) free((char*)rn.ptr);} return pc+1;
    case OP_TEST: if(n>=4){ char *cond[3]={p[1],p[3],p[2]}; int ok=eval_cond(&pr->boxes,cond,3); Res rv=rfast(&pr->boxes, ok?(n>=5?p[4]:"1"):(n>=6?p[5]:"0")); Res rn=rfast(&pr->boxes,p[0]); box_set(&pr->boxes,rn.ptr,rv.ptr); if(rn.owned) free((char*)rn.ptr); if(rv.owned) free((char*)rv.ptr);} return pc+1;
    case OP_IF: if(n>=4 && eval_cond(&pr->boxes,p,3)){ size_t total=0; for(int i=3;i<n;i++) total += strlen(p[i])+2; char *nested=calloc(1,total+1); for(int i=3;i<n;i++){ if(i>3) strcat(nested, i==4 ? " " : "|"); strcat(nested,p[i]); } int npc=exec_line(pr,nested,pc); free(nested); return npc==pc+1?pc+1:npc; } return pc+1;
    case OP_JUMP: if(n>=1){ Res rt=rfast(&pr->boxes,p[0]); const char *target=rt.ptr; if(n>=2 && streqi(p[1],"m")){ int m=mark_find(&pr->marks,target); if(m>=0){ if(rt.owned) free((char*)rt.ptr); return m; }} else if(is_number(target)) { int t=atoi(target)-1; if(t>=0 && t<pr->count){ if(rt.owned) free((char*)rt.ptr); return t; }} if(rt.owned) free((char*)rt.ptr); } return pc+1;
    case OP_JUMPIF: if(n>=4 && eval_cond(&pr->boxes,p,3)){ Res rt=rfast(&pr->boxes,p[3]); const char *target=rt.ptr; if(n>=5 && streqi(p[4],"m")){ int m=mark_find(&pr->marks,target); if(m>=0){ if(rt.owned) free((char*)rt.ptr); return m; }} else if(is_number(target)) { int t=atoi(target)-1; if(t>=0&&t<pr->count){ if(rt.owned) free((char*)rt.ptr); return t; }} if(rt.owned) free((char*)rt.ptr); } return pc+1;
    case OP_DEL: { Res rn=rfast(&pr->boxes,p[0]); box_del(&pr->boxes,rn.ptr); if(rn.owned) free((char*)rn.ptr); } return pc+1;
    case OP_END: pr->halted=1; return pr->count;
    case OP_PREMARK: return pc+1;
    }
    return pc+1;
}

static void program_free(Program *pr){
    for(int i=0;i<pr->count;i++) free(pr->lines[i]);
    free(pr->lines);
    if(pr->ops){ for(int i=0;i<pr->count;i++){ Op*o=pr->ops[i]; if(o){ free_parts(o->parts,o->n); free(o); } } free(pr->ops); }
    boxes_free(&pr->boxes); marks_free(&pr->marks);
}

static char *read_file(const char *path) {
    FILE *f=fopen(path,"rb"); if(!f){ perror(path); exit(1); } fseek(f,0,SEEK_END); long n=ftell(f); rewind(f); char *buf=malloc((size_t)n+1); if(!buf)exit(1); if(fread(buf,1,(size_t)n,f)!=(size_t)n && ferror(f)){ perror(path); exit(1);} buf[n]=0; fclose(f); return buf;
}
static void program_load(Program *pr, const char *src) {
    memset(pr,0,sizeof *pr); int cap=32; pr->lines=malloc(sizeof(char*)*cap); if(!pr->lines)exit(1);
    const char *st=src; for(const char *p=src;;p++){ if(*p=='\n'||*p==0){ if(pr->count==cap){cap*=2;pr->lines=xrealloc(pr->lines,sizeof(char*)*cap);} size_t n=(size_t)(p-st); if(n&&st[n-1]=='\r')n--; pr->lines[pr->count++]=xstrndup(st,n); if(*p==0)break; st=p+1; }}
    pr->ops=calloc((size_t)pr->count,sizeof(Op*));
    for(int i=0;i<pr->count;i++){ char *line=strip_comment(pr->lines[i]); char *s=trim(line); if(!*s){free(line);continue;} char *sp=s; while(*sp&&!isspace((unsigned char)*sp)&&*sp!='|')sp++; char save=*sp; *sp=0; const char *cmd=canonical(s); char *args=save?trim(sp+1):sp; OpKind k;
        if(streqi(cmd,"premark")){ mark_add(&pr->marks,args,i); k=OP_PREMARK; }
        else if(streqi(cmd,"box"))k=OP_BOX; else if(streqi(cmd,"say"))k=OP_SAY; else if(streqi(cmd,"math"))k=OP_MATH;
        else if(streqi(cmd,"test"))k=OP_TEST; else if(streqi(cmd,"if"))k=OP_IF; else if(streqi(cmd,"jump"))k=OP_JUMP;
        else if(streqi(cmd,"jumpif"))k=OP_JUMPIF; else if(streqi(cmd,"del"))k=OP_DEL; else if(streqi(cmd,"end"))k=OP_END;
        else { free(line); continue; }
        Op *o=calloc(1,sizeof(Op)); o->kind=k; o->parts=split_bars(args,&o->n); pr->ops[i]=o; free(line); }
}
static int program_run_source(const char *src) { Program pr; program_load(&pr,src); for(int pc=0; pc<pr.count && !pr.halted;) { Op *o=pr.ops?pr.ops[pc]:NULL; pc=o?exec_op(&pr,o,pc):exec_line(&pr,pr.lines[pc],pc); } program_free(&pr); return 0; }
/* Case-insensitive substring search used by the package registry search. */
static int bx_casestr(const char *hay, const char *needle) {
    if (!needle[0]) return 1;
    size_t nl = strlen(needle);
    for (const char *h = hay; *h; h++) {
        size_t i = 0;
        while (i < nl && h[i] && tolower((unsigned char)h[i]) == tolower((unsigned char)needle[i])) i++;
        if (i == nl) return 1;
    }
    return 0;
}

/* Case-insensitive exact match, used to dispatch on a resolved operand. */
static int bx_caseeq(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == *b;
}

/* Open-file handle registry. Handles are exposed to BX code as opaque names so
 * that modules can pass them around like any other box value. */
typedef struct { char *name; FILE *fp; } FHandle;
static FHandle fhandles[64];
static int fhandle_count = 0;
static char *fhandle_new(FILE *fp) {
    if (fhandle_count >= 64) return NULL;
    char *nm = malloc(32);
    snprintf(nm, 32, "#fh%d", fhandle_count);
    fhandles[fhandle_count].name = nm;
    fhandles[fhandle_count].fp = fp;
    fhandle_count++;
    return nm;
}
static FILE *fhandle_get(const char *name) {
    for (int i = 0; i < fhandle_count; i++) if (!strcmp(fhandles[i].name, name)) return fhandles[i].fp;
    return NULL;
}
static void fhandle_del(const char *name) {
    for (int i = 0; i < fhandle_count; i++) if (!strcmp(fhandles[i].name, name)) {
        free(fhandles[i].name);
        for (int j = i; j < fhandle_count - 1; j++) fhandles[j] = fhandles[j+1];
        fhandle_count--;
        return;
    }
}

/* Package manager section */

/* Cache root: $BOXEDLANG_CACHE, else ~/.cache/boxedlang, else ./.boxcache */
static int umload_cache_dir(char *out, size_t cap) {
    const char *env = getenv("BOXEDLANG_CACHE");
    if (env && env[0]) { snprintf(out, cap, "%s", env); return 0; }
    const char *home = getenv("HOME");
    if (home && home[0]) {
        snprintf(out, cap, "%s/.cache/boxedlang", home);
        if (mkdir(out, 0755) != 0 && errno != EEXIST) { /* fall through to local */ }
        else return 0;
    }
    snprintf(out, cap, ".boxcache");
    return 0;
}

static void umload_mkdir_p(const char *path) {
    char tmp[512];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    }
    mkdir(tmp, 0755);
}

static void umload_cache_pkg(char *out, size_t cap, const char *name) {
    char root[512];
    umload_cache_dir(root, sizeof root);
    snprintf(out, cap, "%s/%s.bx", root, name);
}

/* Registry index to search. Checked in order, so a checkout of the BoxPkg repo
 * under ./boxpkg wins over the local packages directory:
 *   $BOXEDLANG_REGISTRY, ./boxpkg/registry.txt, ./packages/registry.txt,
 *   ./registry.txt, and the registry inside the bx installation prefix. */
static const char *umload_registry_cands[] = {
    "./boxpkg/registry.txt", "./packages/registry.txt", "./registry.txt",
    "/usr/local/share/boxedlang/registry.txt", "/usr/share/boxedlang/registry.txt"
};
static int umload_registry_path(char *out, size_t cap) {
    const char *env = getenv("BOXEDLANG_REGISTRY");
    if (env && env[0]) {
        snprintf(out, cap, "%s", env);
        if (access(out, F_OK) == 0) return 0;
    }
    for (size_t i = 0; i < sizeof umload_registry_cands / sizeof umload_registry_cands[0]; i++) {
        snprintf(out, cap, "%s", umload_registry_cands[i]);
        if (access(out, F_OK) == 0) return 0;
    }
    /* fall back to a registry next to the running binary, so an installed bx
     * can find the packages that were installed alongside it */
    char self[1024];
    ssize_t sl = readlink("/proc/self/exe", self, sizeof self - 1);
    if (sl > 0) {
        self[sl] = 0;
        char *slash = strrchr(self, '/');
        if (slash) {
            *slash = 0;
            char cand[1024];
            snprintf(cand, sizeof cand, "%s/../share/boxedlang/registry.txt", self);
            if (access(cand, F_OK) == 0) { snprintf(out, cap, "%s", cand); return 0; }
            snprintf(cand, sizeof cand, "%s/registry.txt", self);
            if (access(cand, F_OK) == 0) { snprintf(out, cap, "%s", cand); return 0; }
        }
    }
    snprintf(out, cap, "%s", umload_registry_cands[0]);
    return -1;
}

/* Look a name up in the registry. On success sets out_url to a malloc'd url,
 * and copies version and description when out_version/out_desc are given. */
static int umload_registry_lookup(const char *name, char *out_url, size_t url_cap,
                                  char *out_version, size_t ver_cap, char *out_desc, size_t desc_cap) {
    char reg[512];
    umload_registry_path(reg, sizeof reg);
    FILE *rf = fopen(reg, "rb");
    if (!rf) return -1;
    int hit = -1;
    char line[1024];
    while (fgets(line, sizeof line, rf)) {
        umload_meta_trim(line);
        if (!line[0] || line[0] == '#') continue;
        int n; char **f = split_bars(line, &n);
        if (n >= 2 && !strcmp(f[0], name)) {
            snprintf(out_url, url_cap, "%s", f[1]);
            if (out_version) {
                snprintf(out_version, ver_cap, "%s", n >= 3 ? f[2] : "?");
                umload_meta_trim(out_version);
            }
            if (out_desc) {
                snprintf(out_desc, desc_cap, "%s", n >= 4 ? f[3] : "");
                umload_meta_trim(out_desc);
            }
            hit = 0;
        }
        free_parts(f, n);
        if (hit == 0) break;
    }
    fclose(rf);
    return hit;
}

static void umload_meta_trim(char *s) {
    size_t n = strlen(s);
    while (n && (s[n-1] == ' ' || s[n-1] == '\t' || s[n-1] == '\r' || s[n-1] == '\n')) s[--n] = 0;
}

/* Strip quotes and shell metacharacters from metadata before it goes into a
 * system() command line, so package metadata cannot inject shell syntax. */
static void umload_shell_sanitize(char *s) {
    if (!s) return;
    size_t w = 0;
    for (size_t r = 0; s[r]; r++) {
        unsigned char c = (unsigned char)s[r];
        /* Everything lands inside a single quoted shell word, so only quote
         * and metacharacters need dropping. Colons, commas and slashes are
         * common in descriptions and read fine once kept, but '|' has to go
         * because registry.txt is pipe separated. */
        int safe = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                   (c >= '0' && c <= '9') || c == ' ' || c == '.' || c == '-' ||
                   c == '_' || c == ':' || c == ',' || c == '/' || c == '(' || c == ')';
        s[w++] = safe ? (char)c : ' ';
    }
    s[w] = 0;
    umload_meta_trim(s);
}

/* Package metadata lives in leading `// key: value` comment lines of a .bx file.
 * Recognized keys: name, version, author, description, deps (comma separated). */
static int umload_pkg_meta(const char *path, const char *key, char *out, size_t cap) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    char line[512];
    size_t klen = strlen(key);
    int found = -1;
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (strncmp(p, "//", 2)) break;      /* metadata block must be at the top */
        p += 2;
        while (*p == ' ' || *p == '\t') p++;
        if (!strncmp(p, key, klen) && p[klen] == ':') {
            char *v = p + klen + 1;
            while (*v == ' ' || *v == '\t') v++;
            snprintf(out, cap, "%s", v);
            umload_meta_trim(out);
            found = 0;
        }
    }
    fclose(f);
    return found;
}

/* Install a package: URL via curl, else registry entry, else local ./packages/name.bx */
static int umload_pkg_install(const char *name, const char *url, int quiet) {
    char path[512];
    umload_cache_pkg(path, sizeof path, name);
    if (access(path, F_OK) == 0) {
        if (!quiet) { printf("umload: %s already installed\n", name); fflush(stdout); }
        return 0;
    }
    char root[512];
    umload_cache_dir(root, sizeof root);
    umload_mkdir_p(root);

    int ok = 0;
    if (url && url[0]) {
        char cmd[1024];
        snprintf(cmd, sizeof cmd, "curl -fsSL -o '%s' '%s' 2>/dev/null", path, url);
        int rc = system(cmd);
        ok = (rc == 0 && access(path, F_OK) == 0);
    }
    if (!ok) {
        char url2[1024];
        if (umload_registry_lookup(name, url2, sizeof url2, NULL, 0, NULL, 0) == 0) {
            char cmd[1200];
            snprintf(cmd, sizeof cmd, "curl -fsSL -o '%s' '%s' 2>/dev/null", path, url2);
            int rc = system(cmd);
            ok = (rc == 0 && access(path, F_OK) == 0);
        }
    }
    if (!ok) {
        char local[512];
        snprintf(local, sizeof local, "./packages/%s.bx", name);
        if (access(local, F_OK) == 0) {
            FILE *in = fopen(local, "rb"), *out = fopen(path, "wb");
            if (in && out) {
                char buf[4096]; size_t got;
                while ((got = fread(buf, 1, sizeof buf, in)) > 0) fwrite(buf, 1, got, out);
                ok = 1;
            }
            if (in) fclose(in);
            if (out) fclose(out);
        }
    }
    if (ok) {
        if (!quiet) { printf("umload: installed %s\n", name); fflush(stdout); }
        return 0;
    }
    if (url && url[0]) fprintf(stderr, "umload: failed to install %s (network unavailable)\n", name);
    else fprintf(stderr, "umload install: no URL, registry entry, or local package %s\n", name);
    return -1;
}

static int umload_loaded_has(Program *pr, const char *name) {
    char key[128];
    snprintf(key, sizeof key, "pkg_%s", name);
    return strcmp(box_get(&pr->boxes, key), "") != 0;
}

/* Recursively ensure a package and its dependencies are installed. Returns the
 * install of the package itself, or -1 on failure. */
static int umload_require_deps(Program *pr, const char *name, char *stack, int depth) {
    if (depth > 16) { fprintf(stderr, "umload: dependency chain too deep at %s\n", name); return -1; }
    /* stack holds the ancestor chain, each name followed by a space. A name
     * already on the chain means the package graph loops back on itself. */
    if (stack) {
        char tagged[200]; snprintf(tagged, sizeof tagged, "%s ", name);
        if (strstr(stack, tagged)) { fprintf(stderr, "umload: dependency cycle at %s\n", name); return -1; }
    }
    int rc = umload_pkg_install(name, NULL, 1);
    if (rc != 0) return -1;

    char path[512];
    umload_cache_pkg(path, sizeof path, name);
    char deps[512];
    if (umload_pkg_meta(path, "deps", deps, sizeof deps) == 0 && deps[0] &&
        strcmp(deps, "none") && strcmp(deps, "-")) {
        int n; char **d = split_char(deps, ',', &n);
        for (int i = 0; i < n; i++) {
            char dn[128];
            snprintf(dn, sizeof dn, "%s", d[i]);
            umload_meta_trim(dn);
            if (dn[0] && !umload_loaded_has(pr, dn)) {
                char deeper[2048];
                snprintf(deeper, sizeof deeper, "%s%s ", stack?stack:"", name);
                if (umload_require_deps(pr, dn, deeper, depth + 1) != 0) { free_parts(d, n); return -1; }
                char key[160];
                snprintf(key, sizeof key, "pkg_%s", dn);
                box_set(&pr->boxes, key, "1");
            }
        }
        free_parts(d, n);
    }
    char key[160];
    snprintf(key, sizeof key, "pkg_%s", name);
    box_set(&pr->boxes, key, "1");
    return 0;
}

/* True when `path` lives inside the package cache rather than being picked up
 * from a local ./packages directory or an install prefix. */
static int umload_path_is_cached(const char *path) {
    char dir[512];
    if (umload_cache_dir(dir, sizeof dir) != 0 || !dir[0]) return 0;
    size_t dl = strlen(dir);
    if (dl && path[0] == '/' && dl && dir[0] == '/')
        return !strncmp(path, dir, dl) && (path[dl] == '/' || path[dl] == 0);
    return !strncmp(path, dir, dl) && (path[dl] == '/' || path[dl] == 0);
}

/* List a package's dependency tree. Also accumulates the flattened, comma
 * separated dependency list into $pkg_<name>_deplist and its count into
 * $pkg_<name>_depcount so callers can assert on it without scraping stdout. */
static char umload_dep_list[1024];
static int  umload_dep_count = 0;
static int umload_show_deps(Program *pr, const char *name, int depth) {
    if (depth > 16) return 0;
    char path[512];
    if (umload_resolve_path(name, path, sizeof path) != 0) {
        umload_cache_pkg(path, sizeof path, name);
        if (access(path, F_OK) != 0) { printf("%*s%s (not installed)\n", depth * 2, "", name); return 0; }
    }
    char ver[64] = "?";
    umload_pkg_meta(path, "version", ver, sizeof ver);
    char state[16];
    snprintf(state, sizeof state, "%s", umload_loaded_has(pr, name) ? "loaded" : "cached");
    printf("%*s%s %s [%s]\n", depth * 2, "", name, ver, state);

    /* Record this package's own version for the top-level call. */
    if (depth == 0) {
        umload_dep_list[0] = 0; umload_dep_count = 0;
        size_t used = 0;
        int w = snprintf(umload_dep_list, sizeof umload_dep_list, "%s@%s", name, ver);
        used = (w > 0 && (size_t)w < sizeof umload_dep_list) ? (size_t)w : 0;
        umload_dep_count = 1;
        char key[192];
        snprintf(key, sizeof key, "pkg_%s_version", name); box_set(&pr->boxes, key, ver);
    }

    char deps[512];
    if (umload_pkg_meta(path, "deps", deps, sizeof deps) == 0 && deps[0] &&
        strcmp(deps, "none") && strcmp(deps, "-")) {
        int n; char **d = split_char(deps, ',', &n);
        for (int i = 0; i < n; i++) {
            char dn[128];
            snprintf(dn, sizeof dn, "%s", d[i]);
            umload_meta_trim(dn);
            if (!dn[0]) continue;
            char dver[64] = "?";
            char dpath[512];
            if (umload_resolve_path(dn, dpath, sizeof dpath) == 0)
                umload_pkg_meta(dpath, "version", dver, sizeof dver);
            size_t used = strlen(umload_dep_list);
            if (used + 1 < sizeof umload_dep_list) {
                int w = snprintf(umload_dep_list + used, sizeof umload_dep_list - used, ",%s@%s", dn, dver);
                if (w > 0) umload_dep_count++;
            }
            umload_show_deps(pr, dn, depth + 1);
        }
        free_parts(d, n);
    }
    if (depth == 0) {
        char key[192];
        snprintf(key, sizeof key, "pkg_%s_deplist", name);  box_set(&pr->boxes, key, umload_dep_list);
        char cnt[16];
        snprintf(cnt, sizeof cnt, "%d", umload_dep_count);
        snprintf(key, sizeof key, "pkg_%s_depcount", name); box_set(&pr->boxes, key, cnt);
    }
    return 0;
}

static int umload_resolve_path(const char *name, char *out, size_t cap) {
    if (strchr(name,'/') || (strlen(name)>=3 && !strcmp(name+strlen(name)-3,".bx"))) {
        snprintf(out,cap,"%s",name);
        return access(out,F_OK)==0 ? 0 : -1;
    }
    /* cache first, then the old locations for backward compat */
    char cache[512];
    umload_cache_pkg(cache, sizeof cache, name);
    static const char *dirs[] = { "%s", "./packages/%s.bx", "/tmp/bx_pkg_%s.bx", "./%s.bx" };
    snprintf(out, cap, dirs[0], cache);
    if (access(out, F_OK) == 0) return 0;
    for (size_t i=1;i<4;i++) {
        snprintf(out,cap,dirs[i],name);
        if (access(out,F_OK)==0) return 0;
    }
    return -1;
}
static int umload_run_module(Program *parent, const char *path, const char *return_mark) {
    char *src = read_file(path);
    Program mod;
    program_load(&mod, src);
    for (size_t i=0;i<parent->boxes.len;i++)
        if (box_index(&mod.boxes, parent->boxes.items[i].name) < 0)
            box_set(&mod.boxes, parent->boxes.items[i].name, parent->boxes.items[i].value);
    if (return_mark && return_mark[0]) box_set(&mod.boxes,"umload_return",return_mark);
    for (int pc=0; pc<mod.count && !mod.halted;) pc = exec_line(&mod, mod.lines[pc], pc);
    for (size_t i=0;i<mod.boxes.len;i++) {
        const char *nm = mod.boxes.items[i].name;
        if (strcmp(nm,"umload_return")) box_set(&parent->boxes,nm,mod.boxes.items[i].value);
    }
    int rc = 0;
    if (return_mark && return_mark[0]) {
        int m = mark_find(&parent->marks, return_mark);
        if (m >= 0) rc = m;
    }
    program_free(&mod);
    free(src);
    return rc;
}


static void c_string(FILE *out, const char *s) { fputc('"',out); for(;*s;s++){ unsigned char c=*s; if(c=='\\'||c=='"') fprintf(out,"\\%c",c); else if(c=='\n') fputs("\\n",out); else if(c=='\r') fputs("\\r",out); else if(c=='\t') fputs("\\t",out); else if(c<32||c>126) fprintf(out,"\\x%02x",c); else fputc(c,out);} fputc('"',out); }
static int emit_c(const char *src, const char *outpath) {
    char *runtime = read_file(BX_RUNTIME_PATH); FILE *out = outpath ? fopen(outpath,"wb") : stdout; if(!out){ perror(outpath); free(runtime); return 1; }
    fputs("#define BX_EMBEDDED_SOURCE 1\n", out); fputs(runtime, out); fputs("\n#ifdef main\n#undef main\n#endif\n", out);
    fputs("int main(void){ const char *src = ", out); c_string(out, src); fputs("; return program_run_source(src); }\n", out);
    if(outpath) fclose(out);
    free(runtime);
    return 0;
}

#ifndef BX_EMBEDDED_SOURCE
typedef struct { const char *name; const char *cc; const char *objcopy; const char *cpu; } Target;
static const Target targets[] = {
    {"native", "cc", "objcopy", "host default"},
    {"x86_64", "x86_64-linux-gnu-gcc", "x86_64-linux-gnu-objcopy", "AMD64 / x86-64"},
    {"i386", "i686-linux-gnu-gcc", "i686-linux-gnu-objcopy", "Intel 386+ 32-bit"},
    {"i686", "i686-linux-gnu-gcc", "i686-linux-gnu-objcopy", "Intel Pentium Pro+ 32-bit"},
    {"aarch64", "aarch64-linux-gnu-gcc", "aarch64-linux-gnu-objcopy", "ARM 64-bit"},
    {"armv7", "arm-linux-gnueabihf-gcc", "arm-linux-gnueabihf-objcopy", "ARMv7 hard-float"},
    {"arm", "arm-linux-gnueabi-gcc", "arm-linux-gnueabi-objcopy", "ARM 32-bit soft-float"},
    {"riscv64", "riscv64-linux-gnu-gcc", "riscv64-linux-gnu-objcopy", "RISC-V 64-bit"},
    {"riscv32", "riscv32-linux-gnu-gcc", "riscv32-linux-gnu-objcopy", "RISC-V 32-bit"},
    {"mips", "mips-linux-gnu-gcc", "mips-linux-gnu-objcopy", "MIPS 32-bit big-endian"},
    {"mipsel", "mipsel-linux-gnu-gcc", "mipsel-linux-gnu-objcopy", "MIPS 32-bit little-endian"},
    {"mips64", "mips64-linux-gnuabi64-gcc", "mips64-linux-gnuabi64-objcopy", "MIPS 64-bit big-endian"},
    {"powerpc", "powerpc-linux-gnu-gcc", "powerpc-linux-gnu-objcopy", "PowerPC 32-bit"},
    {"ppc64", "powerpc64-linux-gnu-gcc", "powerpc64-linux-gnu-objcopy", "PowerPC 64-bit big-endian"},
    {"ppc64le", "powerpc64le-linux-gnu-gcc", "powerpc64le-linux-gnu-objcopy", "PowerPC 64-bit little-endian"},
    {"s390x", "s390x-linux-gnu-gcc", "s390x-linux-gnu-objcopy", "IBM z/Architecture"},
    {"sparc64", "sparc64-linux-gnu-gcc", "sparc64-linux-gnu-objcopy", "SPARC 64-bit"},
    {"loongarch64", "loongarch64-linux-gnu-gcc", "loongarch64-linux-gnu-objcopy", "LoongArch 64-bit"},
    {NULL, NULL, NULL, NULL}
};

static const Target *find_target(const char *name) {
    for (int i = 0; targets[i].name; i++) if (!strcmp(targets[i].name, name)) return &targets[i];
    return NULL;
}

static void shell_quote(char *out, size_t cap, const char *s);

/* ---------------------------------------------------------------- rulesets

 * A ruleset is a ruleset.md file of `// key: value` lines, the same shape the
 * package metadata already uses. It lets someone describe a build the runner
 * has never heard of - a microcontroller, a retro console, a board with a
 * custom linker script - without patching the C source.
 *
 * Search order: $BOXEDLANG_RULESETS, ./rulesets, then ./.
 */
static int ruleset_path(const char *name, char *out, size_t cap) {
    if (!name || !*name || strchr(name, '/') || strstr(name, "..")) return -1;
    const char *dirs[3];
    char env[512];
    int nd = 0;
    const char *e = getenv("BOXEDLANG_RULESETS");
    if (e && *e) { snprintf(env, sizeof env, "%s", e); dirs[nd++] = env; }
    dirs[nd++] = "./rulesets";
    dirs[nd++] = ".";
    for (int i = 0; i < nd; i++) {
        snprintf(out, cap, "%s/%s.md", dirs[i], name);
        if (access(out, R_OK) == 0) return 0;
    }
    return -1;
}

/* Reads one field out of a ruleset. Callers own the returned string. */
static char *ruleset_field(const char *path, const char *key) {
    char buf[512];
    char *val = NULL;
    if (umload_pkg_meta(path, key, buf, sizeof buf) == 0) val = xstrdup(buf);
    return val;
}

static void list_rulesets(void) {
    printf("ruleset commands:\n");
    printf("  rulesets              list ruleset.md files found\n");
    printf("  ruleset|name          show the fields of one ruleset\n");
    printf("  compile ... --ruleset NAME   build with that ruleset\n");
    printf("search order: $BOXEDLANG_RULESETS, ./rulesets, ./\n");
    fflush(stdout);
}

static int ruleset_scan_dir(const char *dir, const char *label) {
    int found = 0;
    DIR *d = opendir(dir);
    if (!d) return 0;
    struct dirent *ent;
    while ((ent = readdir(d))) {
        size_t l = strlen(ent->d_name);
        if (l < 4 || strcmp(ent->d_name + l - 3, ".md")) continue;
        char path[768];
        snprintf(path, sizeof path, "%s/%s", dir, ent->d_name);
        char nm[256] = "", ds[256] = "", vr[64] = "";
        umload_pkg_meta(path, "name", nm, sizeof nm);
        umload_pkg_meta(path, "description", ds, sizeof ds);
        umload_pkg_meta(path, "version", vr, sizeof vr);
        if (!*nm) { size_t k = l - 3; if (k < sizeof nm) { memcpy(nm, ent->d_name, k); nm[k] = 0; } }
        printf("  %-18s v%-8s %s\n", *nm ? nm : ent->d_name, *vr ? vr : "?", *ds ? ds : "(no description)");
        found++;
    }
    closedir(d);
    (void)label;
    return found;
}

static void list_all_rulesets(void) {
    printf("rulesets:\n");
    int n = ruleset_scan_dir("./rulesets", "local");
    char env[512];
    const char *e = getenv("BOXEDLANG_RULESETS");
    if (e && *e) { snprintf(env, sizeof env, "%s", e); n += ruleset_scan_dir(env, "env"); }
    if (!n) printf("  (none found - drop a ruleset.md in ./rulesets)\n");
    fflush(stdout);
}

static void show_ruleset(const char *name) {
    char path[768];
    if (ruleset_path(name, path, sizeof path) != 0) {
        fprintf(stderr, "ruleset: no ruleset.md for '%s'\n", name);
        return;
    }
    char buf[512];
    printf("ruleset %s (%s)\n", name, path);
    static const char *keys[] = {"name","version","author","description","cc","cflags","ld","ldflags","objcopy","cpu","suffix",NULL};
    for (int i = 0; keys[i]; i++)
        if (umload_pkg_meta(path, keys[i], buf, sizeof buf) == 0) printf("  %-12s %s\n", keys[i], buf);
    fflush(stdout);
}

/* Builds one shell command line for a ruleset, honouring an explicit linker
 * step when the ruleset names `ld` instead of letting the compiler drive it. */
static int ruleset_command(const char *path, const char *mode, const char *out,
                           char *cmd, size_t cap) {
    char *cc = ruleset_field(path, "cc");
    char *cflags = ruleset_field(path, "cflags");
    char *ld = ruleset_field(path, "ld");
    char *ldflags = ruleset_field(path, "ldflags");
    char *objcopy = ruleset_field(path, "objcopy");
    if (!cc) { fprintf(stderr, "ruleset: no cc: field in %s\n", path); free(cflags); free(ld); free(ldflags); free(objcopy); return -1; }
    if (!cflags) cflags = xstrdup("");
    if (!strstr(cflags, "-std=")) {
        char *with = calloc(strlen(cflags) + 16, 1);
        if (with) { strcpy(with, "-std=c99 "); strcat(with, cflags); free(cflags); cflags = with; }
    }
    char qcc[256], qout[512], tmp_obj[256], qtmp_obj[512];
    shell_quote(qcc, sizeof qcc, cc);
    shell_quote(qout, sizeof qout, out);
    snprintf(tmp_obj, sizeof tmp_obj, "/tmp/bx_rs_%ld.o", (long)getpid());

    if (!strcmp(mode, "asm")) {
        snprintf(cmd, cap, "%s %s -S -o %s %s", qcc, cflags, qout, "$BXIN$");
    } else if (!strcmp(mode, "compile") && ld) {
        shell_quote(qtmp_obj, sizeof qtmp_obj, tmp_obj);
        char qld[256];
        shell_quote(qld, sizeof qld, ld);
        snprintf(cmd, cap,
                 "%s %s -c -o %s %s && %s %s -o %s %s",
                 qcc, cflags, qtmp_obj, "$BXIN$", qld, ldflags ? ldflags : "", qout, qtmp_obj);
    } else if (!strcmp(mode, "compile")) {
        snprintf(cmd, cap, "%s %s %s -o %s %s", qcc, cflags, ldflags ? ldflags : "", qout, "$BXIN$");
    } else if (!strcmp(mode, "raw")) {
        shell_quote(qtmp_obj, sizeof qtmp_obj, tmp_obj);
        char qoc[256];
        shell_quote(qoc, sizeof qoc, objcopy ? objcopy : "objcopy");
        snprintf(cmd, cap, "%s %s -c -o %s %s && %s -O binary %s %s",
                 qcc, cflags, qtmp_obj, "$BXIN$", qoc, qtmp_obj, qout);
    } else { fprintf(stderr, "ruleset: mode '%s' is not buildable\n", mode); return -1; }

    free(cc); free(cflags); free(ld); free(ldflags); free(objcopy);
    return 0;
}
static void list_targets(void) {
    puts("known targets:");
    for (int i = 0; targets[i].name; i++) printf("  %-12s %-34s %s\n", targets[i].name, targets[i].cc, targets[i].cpu);
}
static void shell_quote(char *out, size_t cap, const char *s) {
    size_t n = 0;
    if (n + 1 < cap) out[n++] = '\'';
    for (; *s && n + 5 < cap; s++) {
        if (*s == '\'') { memcpy(out + n, "'\\''", 4); n += 4; }
        else out[n++] = *s;
    }
    if (n + 1 < cap) out[n++] = '\'';
    out[n] = 0;
}
static int emit_temp_c(const char *src, char *tmp, size_t tmp_cap) {
    snprintf(tmp, tmp_cap, "/tmp/bx_%ld_%ld.c", (long)getpid(), (long)time(NULL));
    return emit_c(src, tmp);
}
static int run_backend(const char *src, const char *mode, const char *out, const char *target_name, int keep_c) {
    /* Try ruleset first. A ruleset is a file like foo.md that describes
     * build commands (cc, cflags, ld, ldflags, objcopy, cpu). */
    const Target *t = NULL;
    char rpath[768];
    int is_ruleset = 0;
    if (target_name && ruleset_path(target_name, rpath, sizeof rpath) == 0) {
        is_ruleset = 1;
    } else {
        t = find_target(target_name ? target_name : "native");
        if (!t) { fprintf(stderr, "unknown target: %s\n", target_name); list_targets(); return 2; }
    }
    if (!out) { fprintf(stderr, "%s needs -o OUT\n", mode); return 2; }

    char tmp_c[256], qin[512], qout[512], cmd[4096];
    int rc = emit_temp_c(src, tmp_c, sizeof tmp_c); if (rc) return rc;
    char qcc[512];
    if (!is_ruleset) shell_quote(qcc, sizeof qcc, t->cc);
    shell_quote(qin, sizeof qin, tmp_c); shell_quote(qout, sizeof qout, out);

    if (is_ruleset) {
        rc = ruleset_command(rpath, mode, out, cmd, sizeof cmd);
        if (rc == 0) {
            char *repl = cmd;
            for (size_t i = 0; i + 5 < strlen(repl); i++) {
                if (repl[i] == '$' && repl[i+1] == 'B' && repl[i+2] == 'X' && repl[i+3] == 'I' && repl[i+4] == 'N' && repl[i+5] == '$') {
                    repl[i] = 0;
                    char tmp[4096];
                    snprintf(tmp, sizeof tmp, "%s%s%s", repl, qin, repl + i + 6);
                    snprintf(cmd, sizeof cmd, "%s", tmp);
                    break;
                }
            }
            rc = system(cmd);
        }
        if (rc) fprintf(stderr, "%s ruleset '%s' failed\n", mode, target_name);
    } else if (!strcmp(mode, "compile")) {
        snprintf(cmd, sizeof cmd, "%s -x c -std=c99 -O2 -o %s %s", qcc, qout, qin);
        rc = system(cmd);
    } else if (!strcmp(mode, "asm")) {
        snprintf(cmd, sizeof cmd, "%s -x c -std=c99 -O2 -S -o %s %s", qcc, qout, qin);
        rc = system(cmd);
    } else if (!strcmp(mode, "raw")) {
        char tmp_obj[256], qtmp_obj[512], qobjcopy[512];
        snprintf(tmp_obj, sizeof tmp_obj, "/tmp/bx_%ld_%ld.o", (long)getpid(), (long)time(NULL));
        shell_quote(qtmp_obj, sizeof qtmp_obj, tmp_obj); shell_quote(qobjcopy, sizeof qobjcopy, t->objcopy);
        snprintf(cmd, sizeof cmd, "%s -x c -std=c99 -O2 -c -o %s %s", qcc, qtmp_obj, qin);
        rc = system(cmd);
        if (!rc) { snprintf(cmd, sizeof cmd, "%s -O binary %s %s", qobjcopy, qtmp_obj, qout); rc = system(cmd); }
        remove(tmp_obj);
    } else rc = 2;
    if (rc && !is_ruleset) fprintf(stderr, "%s backend failed for target '%s' using %s\n", mode, t->name, t->cc);
    if (!keep_c) remove(tmp_c);
    return rc;
}

static void usage(void){ fprintf(stderr,"usage: bx run FILE | bx transpile FILE -o OUT.c | bx emit-c FILE | bx compile FILE -o OUT [--target T] | bx asm FILE -o OUT.s [--target T] | bx raw FILE -o OUT.bin [--target T] | bx targets | bx version | add -t/--time for timing, options may appear anywhere\n"); }

static void print_version(void) {
    printf("BoxedLANG version %s\n", BX_VERSION);
    char *latest = read_file("VERSION");
    if (latest) {
        // trim newline
        size_t len = strlen(latest);
        if (len && latest[len-1] == '\n') latest[len-1] = '\0';
        if (strcmp(BX_VERSION, latest) != 0) {
            printf("Latest version: %s\n", latest);
            printf("Consider updating.\n");
        } else {
            printf("Up to date.\n");
        }
        free(latest);
    } else {
        printf("No VERSION file found for update check.\n");
    }
}
int main(int argc, char **argv) {
    if(argc >= 2 && !strcmp(argv[1], "targets")) { list_targets(); return 0; }
    if(argc >= 2 && !strcmp(argv[1], "version")) { print_version(); return 0; }
    if(argc >= 2 && !strcmp(argv[1], "rulesets")) { list_all_rulesets(); return 0; }
    if(argc >= 3 && !strcmp(argv[1], "ruleset")) { show_ruleset(argv[2]); return 0; }
    if(argc < 3){ usage(); return 2; }
    const char *mode=NULL, *in=NULL, *out=NULL, *target="native"; int keep_c=0, timeit=0;
    for(int i=1;i<argc;i++) {
        if(!strcmp(argv[i],"-o")&&i+1<argc) out=argv[++i];
        else if(!strcmp(argv[i],"--target")&&i+1<argc) target=argv[++i];
        else if(!strcmp(argv[i],"--ruleset")&&i+1<argc) target=argv[++i];
        else if(!strcmp(argv[i],"--keep-c")) keep_c=1;
        else if(!strcmp(argv[i],"-t") || !strcmp(argv[i],"--time")) timeit=1;
        else if(!mode) mode=argv[i];
        else if(!in) in=argv[i];
    }
    if(!mode || !in){ usage(); return 2; }
    struct timespec bt0, bt1;
    if(timeit) clock_gettime(CLOCK_MONOTONIC, &bt0);
    char *src=read_file(in); int rc=0;
    if(!strcmp(mode,"run")) rc=program_run_source(src);
    else if(!strcmp(mode,"emit-c")) rc=emit_c(src,NULL);
    else if(!strcmp(mode,"transpile")) { if(!out){ usage(); rc=2; } else rc=emit_c(src,out); }
    else if(!strcmp(mode,"compile") || !strcmp(mode,"asm") || !strcmp(mode,"raw")) rc=run_backend(src,mode,out,target,keep_c);
    else if(!strcmp(mode,"ruleset") && !in){ show_ruleset("help"); rc=0; }
    else if(!strcmp(mode,"ruleset")){ show_ruleset(in); rc=0; }
    else if(!strcmp(mode,"rulesets")){ list_all_rulesets(); rc=0; }
    else { usage(); rc=2; }    if(timeit){ clock_gettime(CLOCK_MONOTONIC, &bt1); double sec=(bt1.tv_sec-bt0.tv_sec)+(bt1.tv_nsec-bt0.tv_nsec)/1e9; fprintf(stderr,"bx: %s %s took %.6f s\n", mode, in, sec); }
    free(src); return rc;
}
#endif
