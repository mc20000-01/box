/* BX UI - see bx_ui.h for the design notes. */

#define _POSIX_C_SOURCE 200809L

/* M_PI is not in C99; define it here rather than relying on the build
 * system's feature macros. */
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#include "bx_ui.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

bx_ui_ctx_t g_bx_ui;

/* ------------------------------------------------------------- utilities */

static void copy_id(char *dst, const char *src) {
    if (!src) { dst[0] = 0; return; }
    size_t n = strlen(src);
    if (n >= BX_UI_ID_MAX) n = BX_UI_ID_MAX - 1;
    memcpy(dst, src, n);
    dst[n] = 0;
}

int bx_ui_id_valid(const char *id) {
    if (!id || !*id) return 0;
    size_t n = strlen(id);
    if (n >= BX_UI_ID_MAX) return 0;
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)id[i];
        int ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                 (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.';
        if (!ok) return 0;
    }
    return 1;
}

/* --------------------------------------------------------------- kinds */

/* One table drives both directions, so a new element type is one row rather
 * than three separate switches that can drift apart. */
typedef struct { const char *name; bx_ui_kind_t kind; } bx_ui_kind_row_t;

static const bx_ui_kind_row_t g_kinds[] = {
    { "frame", BX_UI_KIND_FRAME }, { "pane", BX_UI_KIND_PANE },
    { "panel", BX_UI_KIND_PANEL }, { "window", BX_UI_KIND_PANE },
    { "button", BX_UI_KIND_BUTTON }, { "btn", BX_UI_KIND_BUTTON },
    { "label", BX_UI_KIND_LABEL }, { "text", BX_UI_KIND_TEXT },
    { "textbox", BX_UI_KIND_TEXTBOX }, { "input", BX_UI_KIND_TEXTBOX },
    { "slider", BX_UI_KIND_SLIDER }, { "checkbox", BX_UI_KIND_CHECKBOX },
    { "check", BX_UI_KIND_CHECKBOX }, { "radio", BX_UI_KIND_RADIO },
    { "dropdown", BX_UI_KIND_DROPDOWN }, { "select", BX_UI_KIND_DROPDOWN },
    { "image", BX_UI_KIND_IMAGE }, { "icon", BX_UI_KIND_ICON },
    { "shape", BX_UI_KIND_SHAPE }, { "list", BX_UI_KIND_LIST },
    { "listitem", BX_UI_KIND_LISTITEM }, { "item", BX_UI_KIND_LISTITEM },
    { "tabs", BX_UI_KIND_TABS }, { "tab", BX_UI_KIND_TAB },
    { "split", BX_UI_KIND_SPLIT }, { "splitter", BX_UI_KIND_SPLIT },
    { "scroll", BX_UI_KIND_SCROLL }, { "scrollbar", BX_UI_KIND_SCROLL },
    { "grid", BX_UI_KIND_GRID }, { "stack", BX_UI_KIND_STACK },
    { "progress", BX_UI_KIND_PROGRESS }, { "bar", BX_UI_KIND_PROGRESS },
    { "spinner", BX_UI_KIND_SPINNER }, { "canvas", BX_UI_KIND_CANVAS },
    { "tree", BX_UI_KIND_TREE }, { "menu", BX_UI_KIND_MENU },
    { "menuitem", BX_UI_KIND_MENUITEM }, { "toolbar", BX_UI_KIND_TOOLBAR },
    { "statusbar", BX_UI_KIND_STATUSBAR }, { "sidebar", BX_UI_KIND_SIDEBAR },
    { "modal", BX_UI_KIND_MODAL }, { "dialog", BX_UI_KIND_MODAL },
    { "tooltip", BX_UI_KIND_TOOLTIP }, { "tween", BX_UI_KIND_TWEEN },
    { NULL, 0 }
};

int bx_ui_kind_by_name(const char *name) {
    if (!name) return -1;
    for (int i = 0; g_kinds[i].name; i++)
        if (!strcmp(g_kinds[i].name, name)) return (int)g_kinds[i].kind;
    /* A custom element: accept any name that is not taken, and give it a
     * kind of its own so custom and built-in elements can be listed apart. */
    if (bx_ui_id_valid(name)) return BX_UI_KIND_CUSTOM;
    return -1;
}

const char *bx_ui_kind_name(bx_ui_kind_t kind) {
    for (int i = 0; g_kinds[i].name; i++)
        if (g_kinds[i].kind == kind) return g_kinds[i].name;
    return "custom";
}

int bx_ui_kind_count(void) {
    int n = 0;
    while (g_kinds[n].name) n++;
    return n;
}

const char *bx_ui_kind_at(int i) {
    if (i < 0 || i >= bx_ui_kind_count()) return NULL;
    return g_kinds[i].name;
}

/* ------------------------------------------------------------- elements */

void bx_ui_init(void) {
    bx_ui_reset();
}

void bx_ui_reset(void) {
    memset(&g_bx_ui, 0, sizeof g_bx_ui);
    for (int i = 0; i < BX_UI_MAX_ELEMENTS; i++) {
        bx_ui_element_t *e = &g_bx_ui.els[i];
        e->visible = 1;
        e->min_w = e->min_h = 0;
    }
    bx_ui_clock_reset(BX_UI_FPS_DEFAULT);
    bx_ui_fn_default("linear");
    bx_ui_fn_default("ease");
    bx_ui_fn_default("ease-in");
    bx_ui_fn_default("ease-out");
    bx_ui_fn_default("ease-in-out");
    bx_ui_fn_default("spring");
    bx_ui_fn_default("bounce");
    bx_ui_fn_default("elastic");
}

bx_ui_element_t *bx_ui_find(const char *id) {
    if (!id || !*id) return NULL;
    for (int i = 0; i < g_bx_ui.count; i++)
        if (!strcmp(g_bx_ui.els[i].id, id)) return &g_bx_ui.els[i];
    return NULL;
}

bx_ui_element_t *bx_ui_add(const char *id, bx_ui_kind_t kind, const char *parent) {
    if (!bx_ui_id_valid(id)) return NULL;
    if (bx_ui_find(id)) return NULL;              /* same id means same object */
    if (g_bx_ui.count >= BX_UI_MAX_ELEMENTS) return NULL;
    bx_ui_element_t *e = &g_bx_ui.els[g_bx_ui.count++];
    memset(e, 0, sizeof *e);
    copy_id(e->id, id);
    copy_id(e->parent, parent ? parent : "");
    e->kind = kind;
    e->visible = 1;
    e->align_h = BX_UI_ALIGN_START;
    e->align_v = BX_UI_ALIGN_START;
    e->layout = (kind == BX_UI_KIND_FRAME) ? BX_UI_LAYOUT_NONE : BX_UI_LAYOUT_NONE;
    e->columns = 1;
    if (parent && *parent) {
        bx_ui_element_t *p = bx_ui_find(parent);
        if (p) bx_ui_child_add(p, id);
    }
    return e;
}

int bx_ui_child_add(bx_ui_element_t *parent, const char *child) {
    if (!parent || !child) return -1;
    if (parent->child_count >= (int)(sizeof parent->children / sizeof parent->children[0])) return -1;
    if (bx_ui_child_index(parent, child) >= 0) return 0;   /* idempotent */
    copy_id(parent->children[parent->child_count++], child);
    return 0;
}

int bx_ui_child_index(const bx_ui_element_t *parent, const char *child) {
    if (!parent || !child) return -1;
    for (int i = 0; i < parent->child_count; i++)
        if (!strcmp(parent->children[i], child)) return i;
    return -1;
}

int bx_ui_child_remove(bx_ui_element_t *parent, const char *child) {
    int idx = bx_ui_child_index(parent, child);
    if (idx < 0) return -1;
    for (int i = idx; i < parent->child_count - 1; i++)
        copy_id(parent->children[i], parent->children[i + 1]);
    parent->child_count--;
    parent->children[parent->child_count][0] = 0;
    return 0;
}

int bx_ui_remove(const char *id) {
    bx_ui_element_t *e = bx_ui_find(id);
    if (!e) return -1;
    /* Detach from the parent first so no frame points at a freed slot. */
    if (e->parent[0]) {
        bx_ui_element_t *p = bx_ui_find(e->parent);
        if (p) bx_ui_child_remove(p, id);
    }
    int idx = (int)(e - g_bx_ui.els);
    for (int i = 0; i < g_bx_ui.count - 1; i++)
        if (i >= idx) g_bx_ui.els[i] = g_bx_ui.els[i + 1];
    g_bx_ui.count--;
    memset(&g_bx_ui.els[g_bx_ui.count], 0, sizeof g_bx_ui.els[0]);
    return 0;
}

int bx_ui_set_rect(const char *id, float x, float y, float w, float h) {
    bx_ui_element_t *e = bx_ui_find(id);
    if (!e) return -1;
    e->x = x; e->y = y; e->w = w; e->h = h;
    return 0;
}

/* ---------------------------------------------------------------- easing */

static bx_ui_fn_t *fn_find(const char *name) {
    if (!name || !*name) return NULL;
    for (int i = 0; i < BX_UI_MAX_FNS; i++)
        if (g_bx_ui.fns[i].name[0] && !strcmp(g_bx_ui.fns[i].name, name))
            return &g_bx_ui.fns[i];
    return NULL;
}

int bx_ui_fn_define(const char *name, int kind, const double *p) {
    if (!bx_ui_id_valid(name)) return -1;
    bx_ui_fn_t *f = fn_find(name);
    if (!f) {
        for (int i = 0; i < BX_UI_MAX_FNS; i++) {
            if (!g_bx_ui.fns[i].name[0]) { f = &g_bx_ui.fns[i]; break; }
        }
    }
    if (!f) return -1;
    memset(f, 0, sizeof *f);
    copy_id(f->name, name);
    f->kind = kind;
    if (p) for (int i = 0; i < 4; i++) f->p[i] = p[i];
    return 0;
}

int bx_ui_fn_define_bezier(const char *name, double x1, double y1, double x2, double y2) {
    double p[4] = { x1, y1, x2, y2 };
    return bx_ui_fn_define(name, BX_UI_FN_BEZIER, p);
}

int bx_ui_fn_define_mxb(const char *name, double m, double b) {
    double p[4] = { m, b, 0, 0 };
    return bx_ui_fn_define(name, BX_UI_FN_MXB, p);
}

int bx_ui_fn_default(const char *name) {
    static const double linear[4] = { 0, 0, 0, 0 };
    static const double in[4]    = { 0.42, 0, 1, 1 };
    static const double out[4]   = { 0, 0, 0.58, 1 };
    static const double inout[4] = { 0.42, 0, 0.58, 1 };
    static const double spring[4]= { 0.34, 1.56, 0.64, 1 };   /* overshoot */
    static const double bounce[4]= { 0, 0, 0, 0 };
    static const double elastic[4]= { 0.5, 0, 0.5, 1 };

    if (!strcmp(name, "linear"))      return bx_ui_fn_define(name, BX_UI_FN_LINEAR, linear);
    if (!strcmp(name, "ease"))        return bx_ui_fn_define(name, BX_UI_FN_INOUT, inout);
    if (!strcmp(name, "ease-in"))     return bx_ui_fn_define(name, BX_UI_FN_IN, in);
    if (!strcmp(name, "ease-out"))    return bx_ui_fn_define(name, BX_UI_FN_OUT, out);
    if (!strcmp(name, "ease-in-out")) return bx_ui_fn_define(name, BX_UI_FN_INOUT, inout);
    if (!strcmp(name, "spring"))      return bx_ui_fn_define(name, BX_UI_FN_SPRING, spring);
    if (!strcmp(name, "bounce"))      return bx_ui_fn_define(name, BX_UI_FN_BOUNCE, bounce);
    if (!strcmp(name, "elastic"))     return bx_ui_fn_define(name, BX_UI_FN_ELASTIC, elastic);
    return -1;
}

/* Solve x(u) = t for a cubic bezier whose x control points are fixed at 0 and
 * 1, which is how CSS and iOS timing curves are specified. Newton first, then
 * bisection if the derivative is too flat to trust. */
static double bezier_ease(const double *p, double t) {
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;
    double u = t;
    for (int i = 0; i < 8; i++) {
        double x = 3 * u * (1 - u) * (1 - u) * p[0] +
                   3 * u * u * (1 - u) * p[2] + u * u * u;
        double dx = 3 * (1 - u) * (1 - u) * p[0] +
                    6 * u * (1 - u) * (p[2] - p[0]) +
                    3 * u * u * (1 - p[2]);
        if (fabs(dx) < 1e-6) break;
        double err = x - t;
        if (fabs(err) < 1e-7) break;
        u -= err / dx;
        if (u < 0) u = 0;
        if (u > 1) u = 1;
    }
    double y = 3 * u * (1 - u) * (1 - u) * p[1] +
               3 * u * u * (1 - u) * p[3] + u * u * u;
    return y;
}

static double bounce_out(double t) {
    const double n = 7.5625, d = 2.75;
    if (t < 1 / d)      return n * t * t;
    if (t < 2 / d)      { t -= 1.5 / d; return n * t * t + 0.75; }
    if (t < 2.5 / d)    { t -= 2.25 / d; return n * t * t + 0.9375; }
    t -= 2.625 / d;     return n * t * t + 0.984375;
}

double bx_ui_ease(const char *name, double t) {
    if (t <= 0.0) return 0.0;
    if (t >= 1.0) return 1.0;
    bx_ui_fn_t *f = fn_find(name);
    if (!f) f = fn_find("ease-in-out");
    if (!f) return t;
    switch (f->kind) {
    case BX_UI_FN_LINEAR: return t;
    case BX_UI_FN_IN:     return t * t;
    case BX_UI_FN_OUT:    return 1 - (1 - t) * (1 - t);
    case BX_UI_FN_INOUT:  return t < 0.5 ? 2 * t * t : 1 - pow(-2 * t + 2, 2) / 2;
    case BX_UI_FN_BEZIER: return bezier_ease(f->p, t);
    case BX_UI_FN_MXB:    return f->p[0] * t + f->p[1];
    case BX_UI_FN_SPRING: {
        /* Damped spring that lands exactly on 1, so a layout does not creep. */
        double c = f->p[0] * 10.0;
        return 1 - exp(-c * t) * cos(6.28318 * (1 - f->p[2]) * t);
    }
    case BX_UI_FN_BOUNCE: return bounce_out(t);
    case BX_UI_FN_ELASTIC: {
        if (t == 0 || t == 1) return t;
        double c = (2 * M_PI) / 3;
        return pow(2, -10 * t) * sin((t * 10 - 0.75) * c) + 1;
    }
    default: return t;
    }
}

int bx_ui_fn_count(void) {
    int n = 0;
    while (n < BX_UI_MAX_FNS && g_bx_ui.fns[n].name[0]) n++;
    return n;
}

const bx_ui_fn_t *bx_ui_fn_at(int i) {
    if (i < 0 || i >= bx_ui_fn_count()) return NULL;
    return &g_bx_ui.fns[i];
}

/* ---------------------------------------------------------------- clock */

double bx_ui_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

void bx_ui_clock_reset(int fps) {
    bx_ui_clock_t *c = &g_bx_ui.clock;
    c->fps = fps > 0 ? fps : BX_UI_FPS_DEFAULT;
    c->last = bx_ui_now();
    c->dt = 0;
    c->acc = 0;
    c->frame = 0;
    c->running = 0;
    c->single_shot = 0;
}

/* One paced step. Returns 1 when a frame was actually stepped, so a caller
 * can loop until it returns 0 and never render a duplicate frame. */
int bx_ui_frame_step(void) {
    bx_ui_clock_t *c = &g_bx_ui.clock;
    double now = bx_ui_now();
    c->now = now;
    if (c->last == 0) c->last = now;

    if (c->single_shot) {
        c->dt = now - c->last;
        c->last = now;
        c->frame++;
        c->single_shot = 0;
        c->running = 0;
        bx_ui_tween_tick((float)c->dt);
        return 1;
    }

    if (!c->running) return 0;

    double elapsed = now - c->last;
    /* A stall should not turn into one enormous step that skips through the
     * whole animation, so cap the catch-up at half a second. */
    if (elapsed > 0.5) elapsed = 0.5;
    c->acc += elapsed;

    double step = 1.0 / (double)c->fps;
    if (c->acc < step) { c->dt = 0; return 0; }

    c->dt = c->acc;
    c->acc = 0;
    c->last = now;
    c->frame++;
    bx_ui_tween_tick((float)c->dt);
    return 1;
}

/* ---------------------------------------------------------------- tweens */

bx_ui_tween_t *bx_ui_tween_get(const char *id) {
    if (!id || !*id) return NULL;
    for (int i = 0; i < BX_UI_MAX_TWEENS; i++)
        if (g_bx_ui.tweens[i].used && !strcmp(g_bx_ui.tweens[i].id, id))
            return &g_bx_ui.tweens[i];
    return NULL;
}

static bx_ui_tween_t *tween_slot(const char *id) {
    bx_ui_tween_t *tw = bx_ui_tween_get(id);
    if (tw) return tw;
    for (int i = 0; i < BX_UI_MAX_TWEENS; i++) {
        if (!g_bx_ui.tweens[i].used) {
            memset(&g_bx_ui.tweens[i], 0, sizeof g_bx_ui.tweens[i]);
            copy_id(g_bx_ui.tweens[i].id, id);
            g_bx_ui.tweens[i].used = 1;
            return &g_bx_ui.tweens[i];
        }
    }
    return NULL;
}

int bx_ui_tween_start(const char *id, const char *target, const char *prop,
                      float from, float to, float duration, const char *fn) {
    bx_ui_tween_t *tw = tween_slot(id);
    if (!tw || !bx_ui_id_valid(id)) return -1;
    copy_id(tw->target, target ? target : "");
    copy_id(tw->prop, prop ? prop : "x");
    copy_id(tw->fn, fn && *fn ? fn : "ease-in-out");
    tw->from = from; tw->to = to; tw->value = from;
    tw->duration = duration > 0 ? duration : 0.25f;
    tw->elapsed = 0;
    tw->delay = 0;
    tw->state = BX_UI_TWEEN_RUNNING;
    tw->loop = 0;
    return 0;
}

/* Retargeting keeps the id, so an element that is already moving changes
 * course instead of jumping back to the old start value. */
int bx_ui_tween_retarget(const char *id, float to) {
    bx_ui_tween_t *tw = bx_ui_tween_get(id);
    if (!tw) return -1;
    tw->from = tw->value;
    tw->to = to;
    tw->elapsed = 0;
    tw->state = BX_UI_TWEEN_RUNNING;
    return 0;
}

int bx_ui_tween_cancel(const char *id) {
    bx_ui_tween_t *tw = bx_ui_tween_get(id);
    if (!tw) return -1;
    tw->state = BX_UI_TWEEN_IDLE;
    tw->used = 0;
    return 0;
}

int bx_ui_tween_running(void) {
    int n = 0;
    for (int i = 0; i < BX_UI_MAX_TWEENS; i++)
        if (g_bx_ui.tweens[i].used && g_bx_ui.tweens[i].state == BX_UI_TWEEN_RUNNING) n++;
    return n;
}

/* Writes an interpolated value onto the channel the tween drives. */
static void tween_apply(bx_ui_tween_t *tw) {
    if (!tw->target[0]) return;
    bx_ui_element_t *e = bx_ui_find(tw->target);
    if (!e) return;
    float v = tw->value;
    if (!strcmp(tw->prop, "x")) e->x = v;
    else if (!strcmp(tw->prop, "y")) e->y = v;
    else if (!strcmp(tw->prop, "w")) e->w = v;
    else if (!strcmp(tw->prop, "h")) e->h = v;
    else if (!strcmp(tw->prop, "alpha")) e->theme.alpha = (uint8_t)(v < 0 ? 0 : (v > 100 ? 100 : v));
}

int bx_ui_tween_tick(float dt) {
    if (dt < 0) dt = 0;
    int active = 0;
    for (int i = 0; i < BX_UI_MAX_TWEENS; i++) {
        bx_ui_tween_t *tw = &g_bx_ui.tweens[i];
        if (!tw->used || tw->state != BX_UI_TWEEN_RUNNING) continue;

        if (tw->delay > 0) {
            tw->delay -= dt;
            continue;
        }
        tw->elapsed += dt;
        double t = tw->duration > 0 ? (double)tw->elapsed / (double)tw->duration : 1.0;
        if (t > 1.0) t = 1.0;
        double eased = bx_ui_ease(tw->fn, t);
        tw->value = (float)(tw->from + (tw->to - tw->from) * eased);
        tween_apply(tw);

        if (tw->elapsed >= tw->duration) {
            if (tw->loop) {
                tw->elapsed = 0;
                tw->from = tw->to;
                tw->to = (float)(tw->from + (tw->from - tw->to));
                if (--tw->loop == 0) { tw->loop = 1; }
                active++;
            } else {
                tw->value = tw->to;
                tween_apply(tw);
                tw->state = BX_UI_TWEEN_DONE;
            }
        } else {
            active++;
        }
    }
    return active;
}

/* ---------------------------------------------------------------- layout */

int bx_ui_layout_apply(const char *frame_id) {
    bx_ui_element_t *f = bx_ui_find(frame_id);
    if (!f) return -1;

    int n = f->child_count;
    if (n == 0) return 0;

    float inner_w = f->w - 2 * f->pad_x;
    float inner_h = f->h - 2 * f->pad_y;

    if (f->layout == BX_UI_LAYOUT_NONE) {
        /* Absolute: children keep the x/y they were given. */
        return 0;
    }

    if (f->layout == BX_UI_LAYOUT_STACK) {
        for (int i = 0; i < n; i++) {
            bx_ui_element_t *c = bx_ui_find(f->children[i]);
            if (!c || !c->visible) continue;
            c->x = f->pad_x;
            c->y = f->pad_y;
        }
        return 0;
    }

    if (f->layout == BX_UI_LAYOUT_GRID) {
        int cols = f->columns > 0 ? f->columns : 1;
        float cw = (inner_w - (float)(cols - 1) * f->gap) / (float)cols;
        float ch = (inner_h - (float)((n + cols - 1) / cols - 1) * f->gap) /
                   (float)((n + cols - 1) / cols > 0 ? (n + cols - 1) / cols : 1);
        for (int i = 0; i < n; i++) {
            bx_ui_element_t *c = bx_ui_find(f->children[i]);
            if (!c || !c->visible) continue;
            int col = i % cols, row = i / cols;
            c->x = f->pad_x + (float)col * (cw + f->gap);
            c->y = f->pad_y + (float)row * (ch + f->gap);
            if (c->w <= 0) c->w = cw;
            if (c->h <= 0) c->h = ch;
        }
        return 0;
    }

    /* Row and column share the same walk; only the axis swaps. */
    int horizontal = f->layout == BX_UI_LAYOUT_ROW;
    float total = 0;
    for (int i = 0; i < n; i++) {
        bx_ui_element_t *c = bx_ui_find(f->children[i]);
        if (!c || !c->visible) continue;
        total += horizontal ? c->w : c->h;
    }
    total += f->gap * (float)(n - 1 > 0 ? n - 1 : 0);

    float avail = horizontal ? inner_w : inner_h;
    float offset = horizontal ? f->pad_x : f->pad_y;
    float leftover = avail - total;
    float gap = f->gap;

    if (f->justify == BX_UI_JUSTIFY_CENTER) offset += leftover / 2.0f;
    else if (f->justify == BX_UI_JUSTIFY_END) offset += leftover;
    else if (f->justify == BX_UI_JUSTIFY_SPACE_BETWEEN && n > 1) gap = leftover / (float)(n - 1);

    for (int i = 0; i < n; i++) {
        bx_ui_element_t *c = bx_ui_find(f->children[i]);
        if (!c || !c->visible) continue;
        float extent = horizontal ? c->w : c->h;

        if (horizontal) {
            c->x = offset;
            if (c->align_v == BX_UI_ALIGN_STRETCH) c->h = inner_h;
            else if (c->align_v == BX_UI_ALIGN_CENTER) c->y = f->pad_y + (inner_h - c->h) / 2.0f;
            else if (c->align_v == BX_UI_ALIGN_END) c->y = f->pad_y + inner_h - c->h;
            else c->y = f->pad_y;
        } else {
            c->y = offset;
            if (c->align_h == BX_UI_ALIGN_STRETCH) c->w = inner_w;
            else if (c->align_h == BX_UI_ALIGN_CENTER) c->x = f->pad_x + (inner_w - c->w) / 2.0f;
            else if (c->align_h == BX_UI_ALIGN_END) c->x = f->pad_x + inner_w - c->w;
            else c->x = f->pad_x;
        }
        offset += extent + gap;
    }
    return 0;
}

/* Docking a pane resolves to a rectangle inside the frame it lives in. */
void bx_ui_dock_apply(bx_ui_element_t *pane, const bx_gfx_fb_t *fb) {
    if (!pane) return;
    bx_ui_element_t *parent = pane->parent[0] ? bx_ui_find(pane->parent) : NULL;

    /* A pane docks inside its parent frame when it has one, so nested frames
     * size correctly; only a pane with no parent falls back to the screen. */
    float aw, ah;
    if (parent && parent->w > 0) { aw = parent->w; ah = parent->h; }
    else if (fb) { aw = (float)fb->width; ah = (float)fb->height; }
    else { aw = 640.0f; ah = 480.0f; }
    float px = parent ? parent->x : 0.0f;
    float py = parent ? parent->y : 0.0f;

    /* Size comes from the pane when it has one, otherwise from the region. */
    float size = 0;
    switch (pane->dock) {
    case BX_UI_DOCK_LEFT: case BX_UI_DOCK_RIGHT:
        size = pane->w > 0 ? pane->w : aw * 0.25f; break;
    case BX_UI_DOCK_TOP: case BX_UI_DOCK_BOTTOM:
        size = pane->h > 0 ? pane->h : ah * 0.25f; break;
    case BX_UI_DOCK_FILL:  size = 0; break;
    default: return;   /* NONE and CENTER keep the explicit rect */
    }

    switch (pane->dock) {
    case BX_UI_DOCK_LEFT:
        pane->x = px; pane->y = py; pane->h = ah;
        if (pane->w <= 0) pane->w = size; break;
    case BX_UI_DOCK_RIGHT:
        pane->w = (pane->w > 0 ? pane->w : size);
        pane->x = px + aw - pane->w; pane->y = py; pane->h = ah; break;
    case BX_UI_DOCK_TOP:
        pane->x = px; pane->y = py; pane->w = aw;
        if (pane->h <= 0) pane->h = size; break;
    case BX_UI_DOCK_BOTTOM:
        pane->h = (pane->h > 0 ? pane->h : size);
        pane->x = px; pane->y = py + ah - pane->h; pane->w = aw; break;
    case BX_UI_DOCK_FILL:
        pane->x = px; pane->y = py; pane->w = aw; pane->h = ah; break;
    default: break;
    }
}
