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

static int ui_layout_here(bx_ui_element_t *f);

/* Lay out one frame's children, then recurse. Recursion is what makes a
 * window system work: a toolbar inside a window has to know where the window
 * is, and the window has to know where the toolbar is. */
static int ui_layout_one(const char *frame_id, int depth) {
    bx_ui_element_t *f = bx_ui_find(frame_id);
    if (!f || depth > 32) return -1;

    ui_layout_here(f);

    for (int i = 0; i < f->child_count; i++) {
        bx_ui_element_t *c = bx_ui_find(f->children[i]);
        if (c && c->visible && c->child_count > 0) ui_layout_one(c->id, depth + 1);
    }
    return 0;
}

int bx_ui_layout_apply(const char *frame_id) {
    return ui_layout_one(frame_id, 0);
}

static int ui_layout_here(bx_ui_element_t *f) {
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

    /* Every layout writes children in the frame's own coordinates, so the
     * frame's origin is added last, once, in one place. Doing it here rather
     * than in each branch is what keeps a nested frame consistent with the
     * dock code, which also works in absolute coordinates. */
    for (int i = 0; i < n; i++) {
        bx_ui_element_t *c = bx_ui_find(f->children[i]);
        if (!c || !c->visible) continue;
        if (f->layout != BX_UI_LAYOUT_NONE) { c->x += f->x; c->y += f->y; }
        /* minw and minh are floors, so a layout cannot shrink an element
         * below the size it says it needs. */
        if (c->w < c->min_w) c->w = c->min_w;
        if (c->h < c->min_h) c->h = c->min_h;
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

/* ---------------------------------------------------------------- render */

/* An element's own fill. A colour set with ui.set wins over the kind's
 * default, which is what makes a themed window possible without teaching the
 * program about themes. */
static uint32_t ui_fill_for(const bx_ui_element_t *e) {
    if (BX_GFX_A(e->color)) return e->color;
    switch (e->kind) {
        case BX_UI_KIND_BUTTON: return BX_UI_C_ACCENT_DK;
        case BX_UI_KIND_PANEL:
        case BX_UI_KIND_MODAL:
        case BX_UI_KIND_TOOLBAR: return BX_UI_C_PANEL;
        case BX_UI_KIND_PANE:
        case BX_UI_KIND_FRAME: return BX_UI_C_PANEL;
        case BX_UI_KIND_SLIDER:
        case BX_UI_KIND_PROGRESS:
        case BX_UI_KIND_SCROLL: return BX_UI_C_INSET;
        case BX_UI_KIND_CHECKBOX:
        case BX_UI_KIND_RADIO: return BX_UI_C_BG;
        default: return BX_UI_C_BG;
    }
}

static uint32_t ui_text_for(const bx_ui_element_t *e) {
    if (e->disabled) return BX_UI_C_TEXT_DIM;
    if (e->kind == BX_UI_KIND_BUTTON) return BX_UI_C_TEXT;
    return BX_UI_C_TEXT;
}

static void ui_text_center(bx_gfx_fb_t *fb, const bx_ui_element_t *e, uint32_t c) {
    if (!e->text[0]) return;
    int32_t tw = bx_gfx_text_w(e->text), th = bx_gfx_text_h();
    int32_t tx = (int32_t)(e->x + (e->w - tw) / 2.0f);
    int32_t ty = (int32_t)(e->y + (e->h - th) / 2.0f);
    bx_gfx_text_outlined(fb, tx, ty, e->text, c, BX_UI_C_BG);
}

/* Fraction a slider or progress bar is at, from its text field. */
static float ui_fraction(const bx_ui_element_t *e) {
    double v = atof(e->value);
    if (v <= 0.0 && e->value[0] != '0') return e->kind == BX_UI_KIND_PROGRESS ? 0.0f : 0.5f;
    if (v > 1.0) return v > 100.0 ? 1.0f : (float)v;
    return (float)v;
}

static int ui_draw_one(bx_gfx_fb_t *fb, bx_ui_element_t *e) {
    int32_t x = (int32_t)e->x, y = (int32_t)e->y, w = (int32_t)e->w, h = (int32_t)e->h;
    if (w <= 0 || h <= 0) return 0;

    switch (e->kind) {
        case BX_UI_KIND_FRAME:
        case BX_UI_KIND_PANE:
        case BX_UI_KIND_PANEL:
        case BX_UI_KIND_MODAL:
        case BX_UI_KIND_SIDEBAR:
        case BX_UI_KIND_TOOLBAR:
        case BX_UI_KIND_STATUSBAR:
            bx_gfx_rect(fb, x, y, w, h, ui_fill_for(e));
            bx_gfx_rect_outline(fb, x, y, w, h, BX_UI_C_BORDER);
            break;
        case BX_UI_KIND_BUTTON: {
            uint32_t c = ui_fill_for(e);
            /* A pressed button reads as pressed without any animation, which
             * is the cheapest possible feedback. Lerp toward the background
             * rather than fading alpha: a translucent button over a panel
             * would show the panel through it and read as lighter, not
             * pressed. */
            if (e->pressed) c = bx_gfx_color_lerp(c, BX_UI_C_BG, 96);
            else if (e->hovered) c = bx_gfx_color_lerp(c, BX_UI_C_ACCENT, 60);
            bx_gfx_rect(fb, x, y, w, h, c);
            bx_gfx_rect_outline(fb, x, y, w, h, BX_UI_C_ACCENT_DK);
            ui_text_center(fb, e, ui_text_for(e));
            break;
        }
        case BX_UI_KIND_LABEL:
        case BX_UI_KIND_TEXT:
        case BX_UI_KIND_MENUITEM:
        case BX_UI_KIND_LISTITEM:
        case BX_UI_KIND_TOOLTIP:
            /* Text is outlined because it is drawn over a fill that may be
             * any colour the program asked for. */
            bx_gfx_text_outlined(fb, x, y + (h - bx_gfx_text_h()) / 2, e->text,
                                 e->disabled ? BX_UI_C_TEXT_DIM : BX_UI_C_TEXT, BX_UI_C_BG);
            break;
        case BX_UI_KIND_TEXTBOX:
        case BX_UI_KIND_LIST:
        case BX_UI_KIND_TREE:
        case BX_UI_KIND_MENU:
            bx_gfx_rect(fb, x, y, w, h, BX_UI_C_INSET);
            bx_gfx_rect_outline(fb, x, y, w, h, e->focused ? BX_UI_C_ACCENT : BX_UI_C_BORDER);
            bx_gfx_text(fb, x + 4, y + (h - bx_gfx_text_h()) / 2, e->text, BX_UI_C_TEXT);
            break;
        case BX_UI_KIND_SLIDER:
        case BX_UI_KIND_SCROLL: {
            bx_gfx_rect(fb, x, y + h / 3, w, h / 3, BX_UI_C_INSET);
            int32_t knob = w / 8;
            if (knob < 4) knob = 4;
            int32_t kx = x + (int32_t)(ui_fraction(e) * (w - knob));
            bx_gfx_rect(fb, kx, y + h / 3 - 2, knob, h / 3 + 4, BX_UI_C_ACCENT);
            break;
        }
        case BX_UI_KIND_PROGRESS:
            bx_gfx_rect(fb, x, y + h / 3, w, h / 3, BX_UI_C_INSET);
            bx_gfx_rect(fb, x, y + h / 3, (int32_t)(w * ui_fraction(e)), h / 3, BX_UI_C_ACCENT);
            break;
        case BX_UI_KIND_CHECKBOX: {
            int32_t bs = h - 4; if (bs < 6) bs = 6;
            bx_gfx_rect_outline(fb, x, y + 2, bs, bs, e->focused ? BX_UI_C_ACCENT : BX_UI_C_BORDER);
            if (ui_fraction(e) > 0.0f)
                bx_gfx_line(fb, x + 2, y + 2 + bs / 2, x + bs / 2, y + bs,
                            BX_UI_C_ACCENT);
            if (e->text[0])
                bx_gfx_text(fb, x + bs + 6, y + (h - bx_gfx_text_h()) / 2, e->text, BX_UI_C_TEXT);
            break;
        }
        case BX_UI_KIND_RADIO: {
            int32_t r = (h < w ? h : w) / 2;
            bx_gfx_circle_outline(fb, x + r, y + r, r, e->focused ? BX_UI_C_ACCENT : BX_UI_C_BORDER);
            if (ui_fraction(e) > 0.0f) bx_gfx_circle(fb, x + r, y + r, r - 2, BX_UI_C_ACCENT);
            if (e->text[0])
                bx_gfx_text(fb, x + r * 2 + 6, y + (h - bx_gfx_text_h()) / 2, e->text, BX_UI_C_TEXT);
            break;
        }
        case BX_UI_KIND_TABS:
            bx_gfx_rect(fb, x, y, w, h, BX_UI_C_BG);
            bx_gfx_line(fb, x, y + h - 1, x + w, y + h - 1, BX_UI_C_BORDER);
            break;
        case BX_UI_KIND_TAB:
            bx_gfx_rect(fb, x, y, w, h, e->selected ? BX_UI_C_PANEL : BX_UI_C_BG);
            bx_gfx_rect_outline(fb, x, y, w, h, e->selected ? BX_UI_C_ACCENT : BX_UI_C_BORDER);
            ui_text_center(fb, e, e->selected ? BX_UI_C_ACCENT : BX_UI_C_TEXT_DIM);
            break;
        case BX_UI_KIND_SPINNER: {
            /* A spinner is a spinner: an arc that grows. Frame-timing it is
             * what makes it spin rather than sit. */
            int32_t r = (h < w ? h : w) / 2 - 1;
            if (r < 3) break;
            int32_t segs = 8;
            float spin = (float)(g_bx_ui.clock.frame % (uint64_t)segs);
            for (int32_t i = 0; i < segs; i++) {
                float a = (float)(i - spin) * 0.7853981634f;
                int32_t px = x + w / 2 + (int32_t)(cosf(a) * r);
                int32_t py = y + h / 2 + (int32_t)(sinf(a) * r);
                bx_gfx_circle(fb, px, py, 1, i == 0 ? BX_UI_C_ACCENT : BX_UI_C_BORDER);
            }
            break;
        }
        case BX_UI_KIND_ICON:
        case BX_UI_KIND_IMAGE:
        case BX_UI_KIND_CANVAS:
            /* Filled with the frame colour and outlined, so a document that
             * fails to load is visibly empty rather than silently missing. */
            bx_gfx_rect(fb, x, y, w, h, BX_UI_C_INSET);
            bx_gfx_rect_outline(fb, x, y, w, h, BX_UI_C_BORDER);
            if (e->text[0]) ui_text_center(fb, e, BX_UI_C_TEXT_DIM);
            break;
        case BX_UI_KIND_SHAPE:
            bx_gfx_rect(fb, x, y, w, h, ui_fill_for(e));
            break;
        case BX_UI_KIND_TWEEN:
            break;                    /* a tween has no appearance of its own */
        default:
            bx_gfx_rect(fb, x, y, w, h, ui_fill_for(e));
            bx_gfx_rect_outline(fb, x, y, w, h, BX_UI_C_BORDER);
            if (e->text[0]) ui_text_center(fb, e, ui_text_for(e));
            break;
    }
    return 1;
}

/* Draw order: parent before children, and higher z first among siblings, so a
 * raised pane covers what it was raised above. */
static int ui_render_into(bx_gfx_fb_t *fb, const char *id, int depth) {
    bx_ui_element_t *e = bx_ui_find(id);
    if (!e || !e->visible || depth > 32) return 0;

    int n = 0;
    /* A stack, not a frame, because bx_ui.c is C99 with no VLAs in the
     * kernel build and a window tree should not need one. */
    int nc = e->child_count;
    if (nc > BX_UI_MAX_CHILDREN) nc = BX_UI_MAX_CHILDREN;
    /* Collect first, then sort a small pointer array by z: the element list
     * must not be reordered, or ids would stop matching indices. */
    bx_ui_element_t *kids[BX_UI_MAX_CHILDREN];
    for (int i = 0; i < nc; i++) kids[i] = bx_ui_find(e->children[i]);
    for (int i = 0; i < nc; i++)
        for (int j = i + 1; j < nc; j++)
            if (kids[j] && kids[i] && kids[j]->z > kids[i]->z) {
                bx_ui_element_t *t = kids[i]; kids[i] = kids[j]; kids[j] = t;
            }

    n += ui_draw_one(fb, e);
    for (int i = 0; i < nc; i++)
        if (kids[i]) n += ui_render_into(fb, kids[i]->id, depth + 1);
    return n;
}

int bx_ui_render(bx_gfx_fb_t *fb) {
    if (!fb || !fb->pixels) return 0;
    /* Roots only: an element whose parent is gone is a root too, otherwise a
     * closed frame would take its children out of the display. */
    int n = 0;
    for (int i = 0; i < g_bx_ui.count; i++) {
        bx_ui_element_t *e = &g_bx_ui.els[i];
        if (!e->id[0]) continue;
        if (e->parent[0] && bx_ui_find(e->parent)) continue;
        n += ui_render_into(fb, e->id, 0);
    }
    return n;
}

bx_ui_element_t *bx_ui_hit(float x, float y) {
    bx_ui_element_t *best = NULL;
    for (int i = 0; i < g_bx_ui.count; i++) {
        bx_ui_element_t *e = &g_bx_ui.els[i];
        if (!e->id[0] || !e->visible) continue;
        if (x < e->x || y < e->y || x >= e->x + e->w || y >= e->y + e->h) continue;
        /* Deeper wins, so a label inside a frame beats the frame. */
        if (!best) { best = e; continue; }
        int db = 0, dw = 0;
        for (const char *p = e->parent; *p && db < 32; db++) {
            bx_ui_element_t *pe = bx_ui_find(p);
            if (!pe) break;
            p = pe->parent;
        }
        for (const char *p = best->parent; *p && dw < 32; dw++) {
            bx_ui_element_t *pe = bx_ui_find(p);
            if (!pe) break;
            p = pe->parent;
        }
        if (db >= dw || (db == dw && e->z >= best->z)) best = e;
    }
    return best;
}
