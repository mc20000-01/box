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
#include <ctype.h>

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

    /* Controls that arrived with the input layer, plus the containers the
     * common ones are built from. Aliases are deliberate: bx code reads
     * better as card or alert than as panel with a different style. */
    { "switch", BX_UI_KIND_SWITCH }, { "toggle", BX_UI_KIND_SWITCH },
    { "stepper", BX_UI_KIND_STEPPER }, { "spinnerbox", BX_UI_KIND_STEPPER },
    { "combobox", BX_UI_KIND_COMBOBOX }, { "combo", BX_UI_KIND_COMBOBOX },
    { "groupbox", BX_UI_KIND_GROUPBOX }, { "group", BX_UI_KIND_GROUPBOX },
    { "field", BX_UI_KIND_FIELD }, { "fieldbox", BX_UI_KIND_FIELD },
    { "divider", BX_UI_KIND_DIVIDER }, { "rule", BX_UI_KIND_DIVIDER },
    { "separator", BX_UI_KIND_DIVIDER }, { "hr", BX_UI_KIND_DIVIDER },
    { "badge", BX_UI_KIND_BADGE }, { "chip", BX_UI_KIND_CHIP },
    { "tag", BX_UI_KIND_CHIP }, { "pill", BX_UI_KIND_CHIP },
    { "alert", BX_UI_KIND_ALERT }, { "notice", BX_UI_KIND_ALERT },
    { "card", BX_UI_KIND_CARD }, { "panel2", BX_UI_KIND_CARD },
    { "header", BX_UI_KIND_HEADER }, { "footer", BX_UI_KIND_FOOTER },
    { "breadcrumb", BX_UI_KIND_BREADCRUMB }, { "crumbs", BX_UI_KIND_BREADCRUMB },
    { "pagination", BX_UI_KIND_PAGINATION }, { "pager", BX_UI_KIND_PAGINATION },
    { "drawer", BX_UI_KIND_DRAWER }, { "flyout", BX_UI_KIND_DRAWER },
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
    c->pending = 0;
    c->elapsed = 0;
}

/* One paced step. Returns 1 when a frame was actually stepped, so a caller
 * can loop until it returns 0 and never render a duplicate frame. */
/* The fixed step `ui frame|DT` runs. Both frame modes end here, so a driver
 * sees simulated time advancing the same amount whichever mode asked for it:
 * the difference between a looping animation and a test that means something
 * should not be a difference in what "now" is. */
static void ui_frame_commit(double dt) {
    bx_ui_clock_t *c = &g_bx_ui.clock;
    c->dt = dt;
    c->elapsed += dt;
    c->last = c->now;
    c->frame++;
    bx_ui_tween_tick((float)dt);
    /* Input first, then drivers: a driver is allowed to take a hover or press
     * amount over, and it should win, because it is the later writer. */
    bx_ui_input_tick(dt);
    bx_ui_apply_drivers(c->elapsed);
}

void bx_ui_frame_step_dt(double dt) {
    ui_frame_commit(dt);
}

int bx_ui_frame_step(void) {
    bx_ui_clock_t *c = &g_bx_ui.clock;
    double now = bx_ui_now();
    c->now = now;
    if (c->last == 0) c->last = now;

    if (c->single_shot) {
        double dt = c->pending > 0 ? c->pending : (now - c->last);
        c->pending = 0;
        c->single_shot = 0;
        c->running = 0;
        ui_frame_commit(dt);
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

    c->acc = 0;
    ui_frame_commit(elapsed);
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

/* Lay out a frame's docked children as one group.
 *
 * Docking a pane at a time gives every pane the whole edge, so two left-docked
 * panes land on top of each other and a center pane has no region to be
 * centered in. Working on the group instead is the only way an edge can be
 * shared: each side is measured, its panes split it in order, and whatever is
 * left over is the center. */
static void ui_dock_children(bx_ui_element_t *f) {
    if (!f || f->child_count == 0) return;

    float aw = f->w - 2 * f->pad_x;
    float ah = f->h - 2 * f->pad_y;
    if (aw < 0) aw = 0;
    if (ah < 0) ah = 0;

    /* Count the panes on each edge and how much thickness they already asked
     * for. An edge's total is what its panes asked for, plus a quarter of the
     * frame for each pane that asked for nothing: so two unsized sidebars
     * share one quarter, and a sized one keeps its size without stealing the
     * unsized one's share from it. */
    int nl = 0, nr = 0, nt = 0, nb = 0, nf = 0, nc = 0;
    int free_l = 0, free_r = 0, free_t = 0, free_b = 0;
    float wl = 0, wr = 0, ht = 0, hb = 0;
    for (int i = 0; i < f->child_count; i++) {
        bx_ui_element_t *c = bx_ui_find(f->children[i]);
        if (!c || !c->visible) continue;
        switch (c->dock) {
            case BX_UI_DOCK_LEFT:
                nl++;
                if (c->w > 0) wl += c->w; else free_l++;
                break;
            case BX_UI_DOCK_RIGHT:
                nr++;
                if (c->w > 0) wr += c->w; else free_r++;
                break;
            case BX_UI_DOCK_TOP:
                nt++;
                if (c->h > 0) ht += c->h; else free_t++;
                break;
            case BX_UI_DOCK_BOTTOM:
                nb++;
                if (c->h > 0) hb += c->h; else free_b++;
                break;
            case BX_UI_DOCK_FILL:   nf++; break;
            case BX_UI_DOCK_CENTER: nc++; break;
            default: break;                      /* NONE keeps its rect */
        }
    }
    /* An edge with free panes on it gets a quarter of the frame, shared
     * between those panes. So two unsized sidebars each take an eighth and
     * the edge takes a quarter in total. Dividing the edge total by the number
     * of panes on it instead would overpay: with a sized and an unsized pane
     * side by side, the unsized one would claim half of the sized one's width
     * as well as its own share. */
    float sh_l = free_l ? aw * 0.25f / (float)free_l : 0;
    float sh_r = free_r ? aw * 0.25f / (float)free_r : 0;
    float sh_t = free_t ? ah * 0.25f / (float)free_t : 0;
    float sh_b = free_b ? ah * 0.25f / (float)free_b : 0;
    if (free_l) wl += sh_l * (float)free_l;
    if (free_r) wr += sh_r * (float)free_r;
    if (free_t) ht += sh_t * (float)free_t;
    if (free_b) hb += sh_b * (float)free_b;
    /* More edge than frame: the centre gets nothing rather than a negative
     * width, and the edges keep what they asked for. */
    if (wl > aw) wl = aw;
    if (wr > aw) wr = aw;
    if (ht > ah) ht = ah;
    if (hb > ah) hb = ah;

    float x = f->pad_x;
    float y = f->pad_y;
    float cw = aw - wl - wr;      /* the centre region */
    float ch = ah - ht - hb;
    if (cw < 0) cw = 0;
    if (ch < 0) ch = 0;

    /* Right and bottom walk inwards from their edge, which is why they need
     * their own cursors rather than the shared x and y. */
    float rx = f->pad_x + aw;
    float by = f->pad_y + ah;

    for (int i = 0; i < f->child_count; i++) {
        bx_ui_element_t *c = bx_ui_find(f->children[i]);
        if (!c || !c->visible) continue;
        switch (c->dock) {
            case BX_UI_DOCK_LEFT:
                if (c->w <= 0) c->w = sh_l;
                if (c->w > wl) c->w = wl;
                c->x = x; c->y = f->pad_y; c->h = ah;
                x += c->w;
                break;
            case BX_UI_DOCK_RIGHT:
                if (c->w <= 0) c->w = sh_r;
                if (c->w > wr) c->w = wr;
                rx -= c->w;
                c->x = rx; c->y = f->pad_y; c->h = ah;
                break;
            case BX_UI_DOCK_TOP:
                if (c->h <= 0) c->h = sh_t;
                if (c->h > ht) c->h = ht;
                c->x = x; c->y = y; c->w = cw;
                y += c->h;
                break;
            case BX_UI_DOCK_BOTTOM:
                if (c->h <= 0) c->h = sh_b;
                if (c->h > hb) c->h = hb;
                by -= c->h;
                c->x = x; c->y = by; c->w = cw;
                break;
            case BX_UI_DOCK_FILL: {
                /* Fill panes share the centre region in order. */
                float share = cw / (float)(nf > 0 ? nf : 1);
                c->w = share;
                c->x = x;
                c->y = y; c->h = ch;
                x += share;
                break;
            }
            case BX_UI_DOCK_CENTER: {
                /* Centered in the region the edges left, not in the whole
                 * frame: a centred dialog must not sit under the toolbar. */
                float share = cw / (float)(nc > 0 ? nc : 1);
                float bw = c->w > 0 ? c->w : share;
                float bh = c->h > 0 ? c->h : ch;
                if (bw > cw) bw = cw;
                if (bh > ch) bh = ch;
                c->w = bw; c->h = bh;
                c->x = f->pad_x + wl + (cw - bw) / 2.0f;
                c->y = f->pad_y + ht + (ch - bh) / 2.0f;
                break;
            }
            default: break;                      /* NONE keeps its rect */
        }
    }
}

static int ui_layout_here(bx_ui_element_t *f) {
    int n = f->child_count;
    if (n == 0) return 0;

    float inner_w = f->w - 2 * f->pad_x;
    float inner_h = f->h - 2 * f->pad_y;

    /* Docking wins over a flow layout, and over absolute too. A pane that
     * says dock left has asked for the left edge; the layout mode it was
     * added under cannot take that away. */
    int any_docked = 0;
    for (int i = 0; i < n; i++) {
        bx_ui_element_t *c = bx_ui_find(f->children[i]);
        if (c && c->visible && c->dock != BX_UI_DOCK_NONE) { any_docked = 1; break; }
    }
    if (any_docked) {
        ui_dock_children(f);
        return 0;
    }

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
        case BX_UI_KIND_BADGE: return BX_UI_C_ACCENT;
        case BX_UI_KIND_CHIP: return BX_UI_C_INSET;
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

/* Draw a surface according to its material. Everything that looks like a
 * container goes through here, which is what makes glass and matte agree
 * with each other instead of drifting apart per widget. */
static void ui_surface(bx_gfx_fb_t *fb, const bx_ui_element_t *e, uint32_t base) {
    int32_t x = (int32_t)e->x, y = (int32_t)e->y, w = (int32_t)e->w, h = (int32_t)e->h;
    int32_t r = e->radius > 0 ? e->radius : 6;
    if (e->kind == BX_UI_KIND_FRAME || e->kind == BX_UI_KIND_PANE) r = e->radius;

    switch (e->material) {
        case BX_UI_MAT_GLASS: {
            /* A translucent body, a brighter top edge, and a sheen across the
             * top third. The sheen is the part that sells it: without a
             * highlight a transparent rectangle reads as a hole. */
            uint32_t body = bx_gfx_color_scale_alpha(base, 150);
            uint32_t top  = bx_gfx_color_scale_alpha(base, 190);
            uint32_t edge = bx_gfx_color_lerp(base, BX_UI_C_TEXT, 70);
            if (r > 0) bx_gfx_rect_round(fb, x, y, w, h, r, body);
            else bx_gfx_rect(fb, x, y, w, h, body);
            int32_t sheen = h / 3;
            if (sheen > 2) {
                uint32_t hi = bx_gfx_color_scale_alpha(top, 120);
                if (r > 0) {
                    for (int32_t yy = 0; yy < sheen; yy++) {
                        /* Fade the sheen out with height so there is no hard
                         * edge where the highlight stops. */
                        int32_t t = (yy * 256) / sheen;
                        uint32_t c = bx_gfx_color_scale_alpha(hi, 255 - t);
                        for (int32_t xx = 0; xx < w; xx++)
                            if (bx_gfx_round_inside(x + xx, y + yy, x, y, w, h, r))
                                bx_gfx_plot(fb, x + xx, y + yy, c);
                    }
                }
            }
            if (r > 0) bx_gfx_rect_round_outline(fb, x, y, w, h, r, edge);
            else bx_gfx_rect_outline(fb, x, y, w, h, edge);
            /* A brighter line along the very top, where light lands. */
            for (int32_t xx = r; xx < w - r; xx++) bx_gfx_plot(fb, x + xx, y, edge);
            break;
        }
        case BX_UI_MAT_FLAT:
            if (r > 0) bx_gfx_rect_round_outline(fb, x, y, w, h, r, bx_gfx_color_lerp(base, BX_UI_C_TEXT, 40));
            else bx_gfx_rect_outline(fb, x, y, w, h, bx_gfx_color_lerp(base, BX_UI_C_TEXT, 40));
            break;
        case BX_UI_MAT_MATTE:
        default:
            if (r > 0) {
                bx_gfx_rect_round(fb, x, y, w, h, r, base);
                bx_gfx_rect_round_outline(fb, x, y, w, h, r, BX_UI_C_BORDER);
            } else {
                bx_gfx_rect(fb, x, y, w, h, base);
                bx_gfx_rect_outline(fb, x, y, w, h, BX_UI_C_BORDER);
            }
            break;
    }
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
            /* A texture, if one is attached, replaces the material fill: the
             * pattern is the surface, so drawing the material first would only
             * show through the gaps. */
            if (!bx_ui_texture_draw(fb, e)) ui_surface(fb, e, ui_fill_for(e));
            break;
        case BX_UI_KIND_BUTTON: {
            uint32_t c = ui_fill_for(e);
            /* A pressed button reads as pressed without any animation, which
             * is the cheapest possible feedback. Lerp toward the background
             * rather than fading alpha: a translucent button over a panel
             * would show the panel through it and read as lighter, not
             * pressed. */
            /* Read the eased amounts, not the flags. The flags are for logic
             * and for scripts; the visual weight comes from how far the value
             * has actually got, which is what makes a hover fade. */
            int hv = (int)(e->hover * 60.0f);
            if (hv > 0) c = bx_gfx_color_lerp(c, BX_UI_C_ACCENT, hv);
            int pv = (int)(e->press * 96.0f);
            if (pv > 0) c = bx_gfx_color_lerp(c, BX_UI_C_BG, pv);
            bx_ui_element_t tmp = *e;
            tmp.material = BX_UI_MAT_MATTE;
            tmp.color = c;
            ui_surface(fb, &tmp, c);
            if (e->radius > 0) bx_gfx_rect_round_outline(fb, x, y, w, h, e->radius, BX_UI_C_ACCENT_DK);
            else bx_gfx_rect_outline(fb, x, y, w, h, BX_UI_C_ACCENT_DK);
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
            {
                int32_t r = e->radius > 0 ? e->radius : 4;
                bx_gfx_rect_round(fb, x, y, w, h, r, BX_UI_C_INSET);
                bx_gfx_rect_round_outline(fb, x, y, w, h, r,
                                          e->focused ? BX_UI_C_ACCENT : BX_UI_C_BORDER);
            }
            bx_gfx_text(fb, x + 5, y + (h - bx_gfx_text_h()) / 2, e->text, BX_UI_C_TEXT);
            break;
        case BX_UI_KIND_SLIDER:
        case BX_UI_KIND_SCROLL: {
            int32_t r = e->radius > 0 ? e->radius : 4;
            int32_t ty = y + h / 3;
            bx_gfx_rect_round(fb, x, ty, w, h / 3, r, BX_UI_C_INSET);
            int32_t knob = h - 6;
            if (knob < 4) knob = 4;
            int32_t kx = x + (int32_t)(ui_fraction(e) * (w - knob));
            bx_gfx_rect_round(fb, kx, ty - 2, knob, h / 3 + 4, r, BX_UI_C_ACCENT);
            break;
        }
        case BX_UI_KIND_PROGRESS:
            {
                int32_t r = e->radius > 0 ? e->radius : 4;
                int32_t ty = y + h / 3;
                bx_gfx_rect_round(fb, x, ty, w, h / 3, r, BX_UI_C_INSET);
                bx_gfx_rect_round(fb, x, ty, (int32_t)(w * ui_fraction(e)), h / 3, r, BX_UI_C_ACCENT);
            }
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
        case BX_UI_KIND_SWITCH: {
            /* A pill with a knob. The knob's travel is the eased press amount,
             * so the switch can be thrown the same way everything else moves:
             * by changing the amount. */
            int32_t th = h - 4; if (th < 6) th = 6;
            int32_t tr = th / 2;
            int32_t tx = x + (int32_t)((double)w * (e->selected ? 1.0 : 0.0)) - tr;
            uint32_t track = e->selected ? BX_UI_C_ACCENT : BX_UI_C_INSET;
            int32_t hv = (int)(e->hover * 120.0f);
            if (hv > 0) track = bx_gfx_color_lerp(track, BX_UI_C_ACCENT, hv);
            bx_gfx_rect_round(fb, x, y + 2, w, th, tr, track);
            bx_gfx_circle(fb, tx, y + 2 + tr, tr - 1,
                          e->selected ? BX_UI_C_BG : BX_UI_C_TEXT_DIM);
            if (e->text[0])
                bx_gfx_text(fb, x + w + 6, y + (h - bx_gfx_text_h()) / 2, e->text, BX_UI_C_TEXT);
            break;
        }
        case BX_UI_KIND_STEPPER: {
            /* Minus and plus either side of a number, which is a slider that
             * snaps to whole values. */
            int32_t bw = h - 2;
            bx_gfx_rect_round(fb, x, y, bw, h, 3, BX_UI_C_INSET);
            bx_gfx_rect_round(fb, x + w - bw, y, bw, h, 3, BX_UI_C_INSET);
            bx_gfx_line(fb, x + bw / 2 - 3, y + h / 2, x + bw / 2 + 3, y + h / 2, BX_UI_C_TEXT);
            bx_gfx_line(fb, x + w - bw / 2 - 3, y + h / 2, x + w - bw / 2 + 3, y + h / 2, BX_UI_C_TEXT);
            bx_gfx_line(fb, x + w - bw / 2, y + h / 2 - 3, x + w - bw / 2, y + h / 2 + 3, BX_UI_C_TEXT);
            double v = ui_fraction(e);
            int32_t cx = x + bw + (int32_t)((double)(w - bw * 2) * v);
            bx_gfx_text(fb, cx - 6, y + (h - bx_gfx_text_h()) / 2, e->text, BX_UI_C_TEXT);
            break;
        }
        case BX_UI_KIND_BADGE:
        case BX_UI_KIND_CHIP: {
            /* A rounded pill of text. The width follows the text so a chip
             * never needs its width set by hand. */
            int32_t tw = bx_gfx_text_w(e->text);
            int32_t cw = e->w > 0 ? e->w : tw + 12;
            uint32_t base = ui_fill_for(e);
            int32_t r = e->radius > 0 ? e->radius : (h > 2 ? h / 2 : 2);
            bx_gfx_rect_round(fb, x, y, cw, h, r, base);
            bx_gfx_rect_round_outline(fb, x, y, cw, h, r, BX_UI_C_BORDER);
            bx_gfx_text(fb, x + (cw - tw) / 2, y + (h - bx_gfx_text_h()) / 2,
                        e->text, e->kind == BX_UI_KIND_BADGE ? BX_UI_C_BG : BX_UI_C_TEXT);
            break;
        }
        case BX_UI_KIND_DIVIDER: {
            uint32_t c = BX_UI_C_BORDER;
            int hz = (e->w >= e->h);
            if (hz) bx_gfx_line(fb, x, y + h / 2, x + w, y + h / 2, c);
            else    bx_gfx_line(fb, x + w / 2, y, x + w / 2, y + h, c);
            break;
        }
        case BX_UI_KIND_CARD:
        case BX_UI_KIND_ALERT:
        case BX_UI_KIND_GROUPBOX:
        case BX_UI_KIND_FIELD:
        case BX_UI_KIND_HEADER:
        case BX_UI_KIND_FOOTER:
        case BX_UI_KIND_DRAWER:
        case BX_UI_KIND_COMBOBOX:
        case BX_UI_KIND_BREADCRUMB:
        case BX_UI_KIND_PAGINATION:
            ui_surface(fb, e, ui_fill_for(e));
            if (e->text[0] && e->kind != BX_UI_KIND_FIELD)
                bx_gfx_text(fb, x + 6, y + (h - bx_gfx_text_h()) / 2, e->text, BX_UI_C_TEXT);
            break;
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

/* ---------------------------------------------------------------- fields */

/* Field names are matched case-insensitively, like every other command word in
 * the language, so ui set|win|Color and ui set|win|color mean the same thing. */
static int ui_streqi(const char *a, const char *b) {
    for (; *a && *b; a++, b++) {
        int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);
        if (ca != cb) return 0;
    }
    return *a == *b;
}
#define streqi(a,b) ui_streqi((a),(b))

/* One setter for every property. ui.set, a driver and a tween all call this,
 * so there is exactly one answer to what a property means. */
int bx_ui_set_field(bx_ui_element_t *e, const char *field, const char *value) {
    if (!e || !field || !value) return -2;
    double d = atof(value);

    if (!strcmp(field, "x")) e->x = (float)d;
    else if (!strcmp(field, "y")) e->y = (float)d;
    else if (!strcmp(field, "w")) e->w = (float)d;
    else if (!strcmp(field, "h")) e->h = (float)d;
    else if (!strcmp(field, "minw")) e->min_w = (float)d;
    else if (!strcmp(field, "minh")) e->min_h = (float)d;
    else if (!strcmp(field, "gap")) e->gap = (float)d;
    else if (!strcmp(field, "padx")) e->pad_x = (float)d;
    else if (!strcmp(field, "pady")) e->pad_y = (float)d;
    else if (!strcmp(field, "columns")) e->columns = atoi(value);
    else if (!strcmp(field, "z")) e->z = atoi(value);
    else if (!strcmp(field, "radius")) e->radius = atoi(value);
    else if (!strcmp(field, "hover")) e->hover = (float)d;
    else if (!strcmp(field, "press")) e->press = (float)d;
    else if (!strcmp(field, "focusv")) e->focusv = (float)d;
    else if (!strcmp(field, "alpha")) e->theme.alpha = (uint8_t)d;
    else if (!strcmp(field, "scroll")) e->scroll = (float)d;
    else if (!strcmp(field, "value")) snprintf(e->value, sizeof e->value, "%s", value);
    else if (!strcmp(field, "text")) snprintf(e->text, sizeof e->text, "%s", value);
    else if (!strcmp(field, "style")) snprintf(e->style, sizeof e->style, "%s", value);
    else if (!strcmp(field, "bxvg")) snprintf(e->bxvg, sizeof e->bxvg, "%s", value);
    else if (!strcmp(field, "visible")) e->visible = atoi(value) ? 1 : 0;
    else if (!strcmp(field, "selected")) e->selected = atoi(value) ? 1 : 0;
    else if (!strcmp(field, "disabled")) e->disabled = atoi(value) ? 1 : 0;
    else if (!strcmp(field, "color")) {
        uint32_t c = bx_gfx_parse_color(value);
        if (c == 0 && !streqi(value, "0")) return -2;
        e->color = c;
    }
    else if (!strcmp(field, "color2")) {
        uint32_t c = bx_gfx_parse_color(value);
        if (c == 0 && !streqi(value, "0")) return -2;
        e->color2 = c;
    }
    else if (!strcmp(field, "dock")) {
        if (streqi(value,"left")) e->dock = BX_UI_DOCK_LEFT;
        else if (streqi(value,"right")) e->dock = BX_UI_DOCK_RIGHT;
        else if (streqi(value,"top")) e->dock = BX_UI_DOCK_TOP;
        else if (streqi(value,"bottom")) e->dock = BX_UI_DOCK_BOTTOM;
        else if (streqi(value,"center")) e->dock = BX_UI_DOCK_CENTER;
        else if (streqi(value,"fill")) e->dock = BX_UI_DOCK_FILL;
        else if (streqi(value,"none")) e->dock = BX_UI_DOCK_NONE;
        else return -2;
    }
    else if (!strcmp(field, "material")) {
        if (streqi(value,"matte")) e->material = BX_UI_MAT_MATTE;
        else if (streqi(value,"glass")) e->material = BX_UI_MAT_GLASS;
        else if (streqi(value,"flat")) e->material = BX_UI_MAT_FLAT;
        else return -2;
    }
    else if (!strcmp(field, "theme")) {
        if (bx_gfx_parse_theme(value, &e->theme) != 0) return -2;
    }
    else if (!strcmp(field, "align")) {
        if (streqi(value,"start")) { e->align_h = e->align_v = BX_UI_ALIGN_START; }
        else if (streqi(value,"center")) { e->align_h = e->align_v = BX_UI_ALIGN_CENTER; }
        else if (streqi(value,"end")) { e->align_h = e->align_v = BX_UI_ALIGN_END; }
        else if (streqi(value,"stretch")) { e->align_h = e->align_v = BX_UI_ALIGN_STRETCH; }
        else return -2;
    }
    else if (!strcmp(field, "alignx")) {
        if (streqi(value,"start")) e->align_h = BX_UI_ALIGN_START;
        else if (streqi(value,"center")) e->align_h = BX_UI_ALIGN_CENTER;
        else if (streqi(value,"end")) e->align_h = BX_UI_ALIGN_END;
        else if (streqi(value,"stretch")) e->align_h = BX_UI_ALIGN_STRETCH;
        else return -2;
    }
    else if (!strcmp(field, "aligny")) {
        if (streqi(value,"start")) e->align_v = BX_UI_ALIGN_START;
        else if (streqi(value,"center")) e->align_v = BX_UI_ALIGN_CENTER;
        else if (streqi(value,"end")) e->align_v = BX_UI_ALIGN_END;
        else if (streqi(value,"stretch")) e->align_v = BX_UI_ALIGN_STRETCH;
        else return -2;
    }
    else if (!strcmp(field, "layout")) {
        if (streqi(value,"none")) e->layout = BX_UI_LAYOUT_NONE;
        else if (streqi(value,"row")) e->layout = BX_UI_LAYOUT_ROW;
        else if (streqi(value,"column")||streqi(value,"col")) e->layout = BX_UI_LAYOUT_COLUMN;
        else if (streqi(value,"grid")) e->layout = BX_UI_LAYOUT_GRID;
        else if (streqi(value,"stack")) e->layout = BX_UI_LAYOUT_STACK;
        else if (streqi(value,"wrap")) e->layout = BX_UI_LAYOUT_WRAP;
        else return -2;
    }
    else if (!strcmp(field, "justify")) {
        if (streqi(value,"start")) e->justify = BX_UI_JUSTIFY_START;
        else if (streqi(value,"center")) e->justify = BX_UI_JUSTIFY_CENTER;
        else if (streqi(value,"end")) e->justify = BX_UI_JUSTIFY_END;
        else if (streqi(value,"between")||streqi(value,"space-between")) e->justify = BX_UI_JUSTIFY_SPACE_BETWEEN;
        else return -2;
    }
    else {
        /* An unknown field on a custom element is a config value, and it is
         * kept rather than squeezed into the style slot, so it reads back. */
        for (int i = 0; i < e->config_count; i++) {
            if (streqi(e->config_key[i], field)) {
                snprintf(e->config_val[i], BX_UI_CONFIG_VAL, "%s", value);
                return -1;
            }
        }
        if (e->config_count < BX_UI_MAX_CONFIG) {
            snprintf(e->config_key[e->config_count], BX_UI_CONFIG_KEY, "%s", field);
            snprintf(e->config_val[e->config_count], BX_UI_CONFIG_VAL, "%s", value);
            e->config_count++;
            return -1;
        }
        return -2;
    }
    return 0;
}

const char *bx_ui_config_get(const bx_ui_element_t *e, const char *key) {
    if (!e || !key) return NULL;
    for (int i = 0; i < e->config_count; i++)
        if (streqi(e->config_key[i], key)) return e->config_val[i];
    return NULL;
}

int bx_ui_get_field(const bx_ui_element_t *e, const char *field, char *buf, size_t cap) {
    if (!e || !field || !buf || cap == 0) return -1;
    const char *cfg;
    if (!strcmp(field,"kind")||!strcmp(field,"kindname")) snprintf(buf,cap,"%s",bx_ui_kind_name(e->kind));
    else if (!strcmp(field,"x")) snprintf(buf,cap,"%g",e->x);
    else if (!strcmp(field,"y")) snprintf(buf,cap,"%g",e->y);
    else if (!strcmp(field,"w")) snprintf(buf,cap,"%g",e->w);
    else if (!strcmp(field,"h")) snprintf(buf,cap,"%g",e->h);
    else if (!strcmp(field,"minw")) snprintf(buf,cap,"%g",e->min_w);
    else if (!strcmp(field,"minh")) snprintf(buf,cap,"%g",e->min_h);
    else if (!strcmp(field,"gap")) snprintf(buf,cap,"%g",e->gap);
    else if (!strcmp(field,"padx")) snprintf(buf,cap,"%g",e->pad_x);
    else if (!strcmp(field,"pady")) snprintf(buf,cap,"%g",e->pad_y);
    else if (!strcmp(field,"radius")) snprintf(buf,cap,"%d",e->radius);
    else if (!strcmp(field,"columns")) snprintf(buf,cap,"%d",e->columns);
    else if (!strcmp(field,"z")) snprintf(buf,cap,"%d",e->z);
    else if (!strcmp(field,"alpha")) snprintf(buf,cap,"%d",e->theme.alpha);
    else if (!strcmp(field,"scroll")) snprintf(buf,cap,"%g",e->scroll);
    else if (!strcmp(field,"text")) snprintf(buf,cap,"%s",e->text);
    else if (!strcmp(field,"value")) snprintf(buf,cap,"%s",e->value);
    else if (!strcmp(field,"style")) snprintf(buf,cap,"%s",e->style);
    else if (!strcmp(field,"bxvg")) snprintf(buf,cap,"%s",e->bxvg);
    else if (!strcmp(field,"visible")) snprintf(buf,cap,"%d",e->visible?1:0);
    else if (!strcmp(field,"selected")) snprintf(buf,cap,"%d",e->selected?1:0);
    else if (!strcmp(field,"disabled")) snprintf(buf,cap,"%d",e->disabled?1:0);
    else if (!strcmp(field,"hovered")) snprintf(buf,cap,"%d",e->hovered?1:0);
    else if (!strcmp(field,"pressed")) snprintf(buf,cap,"%d",e->pressed?1:0);
    else if (!strcmp(field,"focused")) snprintf(buf,cap,"%d",e->focused?1:0);
    /* The eased amounts, not the flags. A driver can read them and so can a
     * script, which is what makes hover something a program can animate. */
    else if (!strcmp(field,"hover")) snprintf(buf,cap,"%g",e->hover);
    else if (!strcmp(field,"press")) snprintf(buf,cap,"%g",e->press);
    else if (!strcmp(field,"focusv")) snprintf(buf,cap,"%g",e->focusv);
    /* The same three as whole percentages. bx compares integers, so a 0..1
     * amount is not something a script can test; 0..100 is. */
    else if (!strcmp(field,"hoverpct")) snprintf(buf,cap,"%d",(int)(e->hover*100.0f+0.5f));
    else if (!strcmp(field,"presspct")) snprintf(buf,cap,"%d",(int)(e->press*100.0f+0.5f));
    else if (!strcmp(field,"focuspct")) snprintf(buf,cap,"%d",(int)(e->focusv*100.0f+0.5f));
    else if (!strcmp(field,"children")) snprintf(buf,cap,"%d",e->child_count);
    else if (!strcmp(field,"parent")) snprintf(buf,cap,"%s",e->parent);
    else if (!strcmp(field,"color")) {
        uint32_t c = e->color;
        if (!BX_GFX_A(c)) c = 0;
        snprintf(buf,cap,"0x%02x%02x%02x%02x",BX_GFX_R(c),BX_GFX_G(c),BX_GFX_B(c),BX_GFX_A(c));
    }
    else if (!strcmp(field,"material")) {
        const char *m = e->material==BX_UI_MAT_GLASS?"glass":e->material==BX_UI_MAT_FLAT?"flat":"matte";
        snprintf(buf,cap,"%s",m);
    }
    else if (!strcmp(field,"align")) {
        /* One field back for both axes, because that is how they are set. When
         * they disagree, report the horizontal one, which is the one the
         * layout reads first. */
        const char *an[]={"start","center","end","stretch"};
        snprintf(buf,cap,"%s",(e->align_h>=0&&e->align_h<=3)?an[e->align_h]:"start");
    }
    else if (!strcmp(field,"alignh")) {
        const char *an[]={"start","center","end","stretch"};
        snprintf(buf,cap,"%s",(e->align_h>=0&&e->align_h<=3)?an[e->align_h]:"start");
    }
    else if (!strcmp(field,"alignv")) {
        const char *an[]={"start","center","end","stretch"};
        snprintf(buf,cap,"%s",(e->align_v>=0&&e->align_v<=3)?an[e->align_v]:"start");
    }
    else if (!strcmp(field,"justify")) {
        const char *jn[]={"start","center","end","stretch"};
        snprintf(buf,cap,"%s",(e->justify>=0&&e->justify<=3)?jn[e->justify]:"start");
    }
    else if (!strcmp(field,"layout")) {
        static const char *ln[]={"none","row","column","grid","stack"};
        snprintf(buf,cap,"%s",(e->layout>=0&&e->layout<=4)?ln[e->layout]:"none");
    }
    else if (!strcmp(field,"visible")) snprintf(buf,cap,"%d",e->visible?1:0);
    else if (!strcmp(field,"dock")) {
        static const char *dn[]={"none","left","right","top","bottom","center","fill"};
        snprintf(buf,cap,"%s",(e->dock>=0&&e->dock<=6)?dn[e->dock]:"none");
    }
    else if ((cfg = bx_ui_config_get(e, field)) != NULL) snprintf(buf,cap,"%s",cfg);
    else return -1;
    return 0;
}

/* ----------------------------------------------------------- math functions */

int bx_ui_mathfn_define(const char *name, int kind, const double *p) {
    if (!name || !*name) return -1;
    for (int i = 0; i < g_bx_ui.mathfn_count; i++) {
        if (streqi(g_bx_ui.mathfns[i].name, name)) {
            g_bx_ui.mathfns[i].kind = kind;
            for (int k = 0; k < 4; k++) g_bx_ui.mathfns[i].p[k] = p ? p[k] : 0.0;
            return 0;
        }
    }
    if (g_bx_ui.mathfn_count >= BX_UI_MAX_FNS2) return -1;
    bx_ui_mfn_t *m = &g_bx_ui.mathfns[g_bx_ui.mathfn_count++];
    memset(m, 0, sizeof *m);
    snprintf(m->name, sizeof m->name, "%s", name);
    m->kind = kind;
    for (int k = 0; k < 4; k++) m->p[k] = p ? p[k] : 0.0;
    return 0;
}

int bx_ui_mathfn_define_mxb(const char *name, double m, double b) {
    double p[4] = { m, b, 0, 0 };
    return bx_ui_mathfn_define(name, BX_UI_MFN_MXB, p);
}

/* A hash-based noise. Deterministic from t alone so a texture animated by it
 * replays identically, which a random number generator would not. */
static double ui_hash01(double t) {
    uint64_t x = (uint64_t)(int64_t)(t * 1000.0);
    x ^= x >> 33; x *= 0xff51afd7ed558ccdULL;
    x ^= x >> 33; x *= 0xc4ceb9fe1a85ec53ULL;
    x ^= x >> 33;
    return (double)(x & 0xFFFFFFu) / (double)0xFFFFFF;
}

double bx_ui_mathfn_eval(const char *name, double t) {
    if (!name) return 0.0;
    const bx_ui_mfn_t *m = NULL;
    for (int i = 0; i < g_bx_ui.mathfn_count; i++)
        if (streqi(g_bx_ui.mathfns[i].name, name)) { m = &g_bx_ui.mathfns[i]; break; }
    /* An undefined name is linear, not zero: a typo should make something
     * move, not make something vanish. */
    if (!m) return t;

    switch (m->kind) {
        case BX_UI_MFN_MXB:  return m->p[0] * t + m->p[1];
        case BX_UI_MFN_SIN: {
            double amp = m->p[0], freq = m->p[1], phase = m->p[2], off = m->p[3];
            return off + amp * sin(2.0 * M_PI * freq * t + phase);
        }
        case BX_UI_MFN_TRI: {
            double period = m->p[0] > 0 ? m->p[0] : 1.0;
            double u = fmod(t, period) / period;
            return u < 0.5 ? u * 2.0 : 2.0 - u * 2.0;
        }
        case BX_UI_MFN_DECAY: {
            double k = m->p[0];
            return exp(-(k > 0 ? k : 1.0) * t);
        }
        case BX_UI_MFN_STEP: {
            double step = m->p[0] > 0 ? m->p[0] : 1.0;
            return floor(t / step) * step;
        }
        case BX_UI_MFN_NOISE:
            return ui_hash01(t * (m->p[0] > 0 ? m->p[0] : 1.0) + (m->p[1] != 0 ? m->p[1] : 0.0));
        case BX_UI_MFN_SQRT:
            return sqrt(t > 0 ? t : 0.0) * (m->p[0] != 0 ? m->p[0] : 1.0);
        default:
            return t;
    }
}

int bx_ui_mathfn_count(void) { return g_bx_ui.mathfn_count; }
const bx_ui_mfn_t *bx_ui_mathfn_at(int i) {
    return (i >= 0 && i < g_bx_ui.mathfn_count) ? &g_bx_ui.mathfns[i] : NULL;
}

/* --------------------------------------------------------------- drivers */

int bx_ui_drive(const char *id, const char *prop, int kind, const double *p,
                const char *fn, const char *src_id, const char *src_prop) {
    if (!id || !prop) return -1;
    /* One driver per (element, property): a second one replaces the first,
     * so re-driving a property mid-animation does not stack. */
    for (int i = 0; i < g_bx_ui.driver_count; i++) {
        bx_ui_driver_t *d = &g_bx_ui.drivers[i];
        if (streqi(d->id, id) && streqi(d->prop, prop)) {
            d->kind = kind;
            snprintf(d->fn, sizeof d->fn, "%s", fn ? fn : "");
            snprintf(d->src_id, sizeof d->src_id, "%s", src_id ? src_id : "");
            snprintf(d->src_prop, sizeof d->src_prop, "%s", src_prop ? src_prop : "");
            d->v = p ? p[0] : 0.0;
            d->t0 = p ? p[1] : 0.0;
            d->t1 = p ? p[2] : 1.0;
            d->lo = p ? p[0] : 0.0;
            d->hi = p ? p[3] : 1.0;
            return 0;
        }
    }
    if (g_bx_ui.driver_count >= BX_UI_MAX_DRIVERS) return -1;
    bx_ui_driver_t *d = &g_bx_ui.drivers[g_bx_ui.driver_count++];
    memset(d, 0, sizeof *d);
    snprintf(d->id, sizeof d->id, "%s", id);
    snprintf(d->prop, sizeof d->prop, "%s", prop);
    d->kind = kind;
    snprintf(d->fn, sizeof d->fn, "%s", fn ? fn : "");
    snprintf(d->src_id, sizeof d->src_id, "%s", src_id ? src_id : "");
    snprintf(d->src_prop, sizeof d->src_prop, "%s", src_prop ? src_prop : "");
    d->v  = p ? p[0] : 0.0;
    d->t0 = p ? p[1] : 0.0;
    d->t1 = p ? p[2] : 1.0;
    d->lo = p ? p[0] : 0.0;
    d->hi = p ? p[3] : 1.0;
    return 0;
}

int bx_ui_drive_count(void) { return g_bx_ui.driver_count; }
const bx_ui_driver_t *bx_ui_drive_at(int i) {
    return (i >= 0 && i < g_bx_ui.driver_count) ? &g_bx_ui.drivers[i] : NULL;
}

int bx_ui_apply_drivers(double t) {
    (void)t;
    int applied = 0;
    for (int i = 0; i < g_bx_ui.driver_count; i++) {
        bx_ui_driver_t *d = &g_bx_ui.drivers[i];
        bx_ui_element_t *e = bx_ui_find(d->id);
        if (!e) continue;
        char buf[64];
        switch (d->kind) {
            case BX_UI_DRIVE_CONST:
                snprintf(buf, sizeof buf, "%g", d->v);
                break;
            case BX_UI_DRIVE_MFN: {
                /* Sample the function over [t0,t1] and put the result in
                 * [lo,hi]. That is what makes a driver contextual: the same
                 * function can drive a 0..1 opacity or a 0..400 pixel slide
                 * just by changing the range. */
                double span = d->t1 - d->t0;
                double u = span != 0.0 ? (t - d->t0) / span : (t >= d->t1 ? 1.0 : 0.0);
                if (u < 0.0) u = 0.0;
                if (u > 1.0) u = 1.0;
                double val = bx_ui_mathfn_eval(d->fn, t);
                double lo = d->lo, hi = d->hi;
                /* A function that already returns 0..1 is used directly as a
                 * fraction; anything else is assumed to be an absolute value
                 * and scaled into the range. */
                double out = (val >= 0.0 && val <= 1.0) ? (lo + val * (hi - lo))
                                                        : (lo + val);
                snprintf(buf, sizeof buf, "%g", out);
                break;
            }
            case BX_UI_DRIVE_WAVE: {
                double val = bx_ui_mathfn_eval(d->fn, t);
                /* Fold into [lo,hi] so an oscillating function stays in range
                 * however far it swings. */
                double k = val - floor(val);
                snprintf(buf, sizeof buf, "%g", d->lo + k * (d->hi - d->lo));
                break;
            }
            case BX_UI_DRIVE_MIRROR: {
                bx_ui_element_t *s = bx_ui_find(d->src_id);
                if (!s) continue;
                char tmp[64];
                if (bx_ui_get_field(s, d->src_prop, tmp, sizeof tmp) != 0) continue;
                snprintf(buf, sizeof buf, "%s", tmp);
                break;
            }
            default:
                continue;
        }
        bx_ui_set_field(e, d->prop, buf);
        applied++;
    }
    return applied;
}

/* --------------------------------------------------------------- textures */

int bx_ui_texture_set(const char *id, int kind, const double *p,
                      const char *fn, const char *c1, const char *c2) {
    if (!id) return -1;
    bx_ui_texture_t *t = NULL;
    for (int i = 0; i < g_bx_ui.texture_count; i++)
        if (streqi(g_bx_ui.textures[i].id, id)) { t = &g_bx_ui.textures[i]; break; }
    if (!t) {
        if (g_bx_ui.texture_count >= BX_UI_MAX_TEXTURES) return -1;
        t = &g_bx_ui.textures[g_bx_ui.texture_count++];
        memset(t, 0, sizeof *t);
        snprintf(t->id, sizeof t->id, "%s", id);
    }
    t->kind = kind;
    for (int k = 0; k < 4; k++) t->p[k] = p ? p[k] : 0.0;
    snprintf(t->fn, sizeof t->fn, "%s", fn ? fn : "");
    /* An unparseable colour used to fall back to the palette silently, which
     * turned a typo into a texture in the wrong colours that still rendered.
     * Say so instead. */
    t->c1 = c1 ? bx_gfx_parse_color(c1) : 0;
    t->c2 = c2 ? bx_gfx_parse_color(c2) : 0;
    if (c1 && *c1 && !t->c1) fprintf(stderr,"ui texture %s: bad colour '%s', want #rrggbb or #rrggbbaa\n",id,c1);
    if (c2 && *c2 && !t->c2) fprintf(stderr,"ui texture %s: bad colour '%s', want #rrggbb or #rrggbbaa\n",id,c2);
    if (!t->c1) t->c1 = BX_UI_C_PANEL;
    if (!t->c2) t->c2 = BX_UI_C_ACCENT;
    t->active = 1;
    return 0;
}

int bx_ui_texture_count(void) { return g_bx_ui.texture_count; }
const bx_ui_texture_t *bx_ui_texture_at(int i) {
    return (i >= 0 && i < g_bx_ui.texture_count) ? &g_bx_ui.textures[i] : NULL;
}

static const bx_ui_texture_t *ui_texture_of(const char *id) {
    for (int i = 0; i < g_bx_ui.texture_count; i++)
        if (streqi(g_bx_ui.textures[i].id, id) && g_bx_ui.textures[i].active)
            return &g_bx_ui.textures[i];
    return NULL;
}

/* Fill an element's rect from its texture. Every pattern is a function of the
 * pixel's position and, where it matters, of the clock: a stripe is
 * sin(k*(x+y)), a checker is (x/s + y/s) & 1, noise is a hash of position and
 * time. That is why a texture can be animated by tweening time rather than by
 * touching pixels. */
int bx_ui_texture_draw(bx_gfx_fb_t *fb, bx_ui_element_t *e) {
    if (!fb || !e) return 0;
    const bx_ui_texture_t *t = ui_texture_of(e->id);

    if (!t) return 0;

    int32_t x0 = (int32_t)e->x, y0 = (int32_t)e->y, w = (int32_t)e->w, h = (int32_t)e->h;
    if (w <= 0 || h <= 0) return 0;
    double tt = g_bx_ui.clock.elapsed;

    for (int32_t yy = 0; yy < h; yy++) {
        for (int32_t xx = 0; xx < w; xx++) {
            int32_t px = x0 + xx, py = y0 + yy;
            uint32_t c = t->c1;
            double fx = (double)xx, fy = (double)yy;

            switch (t->kind) {
                case BX_UI_TEX_GRADV:
                    c = bx_gfx_color_lerp(t->c1, t->c2, h > 1 ? (yy * 256) / (h - 1) : 0);
                    break;
                case BX_UI_TEX_GRADH:
                    c = bx_gfx_color_lerp(t->c1, t->c2, w > 1 ? (xx * 256) / (w - 1) : 0);
                    break;
                case BX_UI_TEX_CHECKER: {
                    double s = t->p[0] > 0 ? t->p[0] : 8.0;
                    int64_t cx = (int64_t)floor(fx / s), cy = (int64_t)floor(fy / s);
                    c = ((cx + cy) & 1) ? t->c2 : t->c1;
                    break;
                }
                case BX_UI_TEX_STRIPES: {
                    /* Diagonal stripes: the dot product of the position with
                     * the stripe direction, so one number decides the colour. */
                    double ang = t->p[0];
                    double period = t->p[1] > 0 ? t->p[1] : 8.0;
                    /* p2 slides the pattern, in periods per second. The
                     * offset goes in before the floor, not after it: adding a
                     * whole number of periods later would only ever move the
                     * stripes once per second. */
                    double v = (fx * cos(ang * M_PI / 180.0) + fy * sin(ang * M_PI / 180.0))
                             + t->p[2] * tt * period;
                    c = (((int64_t)floor(v / period) & 1) == 0) ? t->c1 : t->c2;
                    break;
                }
                case BX_UI_TEX_DOTS: {
                    double period = t->p[0] > 0 ? t->p[0] : 10.0;
                    double rad = t->p[1];
                    double dx = fmod(fx, period) - period / 2.0;
                    double dy = fmod(fy, period) - period / 2.0;
                    double d2 = dx * dx + dy * dy;
                    c = (rad > 0 && d2 <= rad * rad) ? t->c2 : t->c1;
                    break;
                }
                case BX_UI_TEX_GRID: {
                    double step = t->p[0] > 0 ? t->p[0] : 8.0;
                    int on_x = fmod(fx, step) < 1.0;
                    int on_y = fmod(fy, step) < 1.0;
                    c = (on_x || on_y) ? t->c2 : t->c1;
                    break;
                }
                case BX_UI_TEX_NOISE: {
                    double s = t->p[0] > 0 ? t->p[0] : 1.0;
                    c = bx_gfx_color_lerp(t->c1, t->c2, (int32_t)(ui_hash01(px * 7.13 + py * 3.71 + tt * s * 13.0) * 256.0));
                    break;
                }
                case BX_UI_TEX_RING: {
                    double cx = w / 2.0, cy = h / 2.0;
                    double d = sqrt((fx - cx) * (fx - cx) + (fy - cy) * (fy - cy));
                    double period = t->p[0] > 0 ? t->p[0] : 8.0;
                    double wdt = t->p[1] > 0 ? t->p[1] : 2.0;
                    c = (fmod(d + tt * t->p[2], period) < wdt) ? t->c2 : t->c1;
                    break;
                }
                case BX_UI_TEX_WAVE: {
                    double amp = t->p[0], freq = t->p[1], phase = t->p[2] * tt;
                    double v = amp * sin((fx / (freq > 0 ? freq : 20.0)) + phase);
                    c = bx_gfx_color_lerp(t->c1, t->c2, (int32_t)((v * 0.5 + 0.5) * 256.0));
                    break;
                }
                case BX_UI_TEX_MFN: {
                    double v = bx_ui_mathfn_eval(t->fn, fy + tt);
                    double u = (v < 0.0) ? 0.0 : (v > 1.0 ? 1.0 : v);
                    c = bx_gfx_color_lerp(t->c1, t->c2, (int32_t)(u * 256.0));
                    break;
                }
                case BX_UI_TEX_SOLID:
                default:
                    c = t->c1;
                    break;
            }
            /* An element colour set with ui set tints the texture rather than
             * replacing it, so a pattern can be recoloured without being
             * redefined. */
            if (e->color && BX_GFX_A(e->color)) c = bx_gfx_color_lerp(c, e->color, 128);
            if (e->theme.alpha) c = bx_gfx_color_scale_alpha(c, (int32_t)e->theme.alpha * 256 / 100);
            bx_gfx_plot(fb, px, py, c);
        }
    }
    return 1;
}


void bx_ui_drive_remove(const char *id, const char *prop) {
    if (!id) return;
    for (int i = 0; i < g_bx_ui.driver_count; i++) {
        if (streqi(g_bx_ui.drivers[i].id, id) &&
            (!prop || streqi(g_bx_ui.drivers[i].prop, prop))) {
            memmove(&g_bx_ui.drivers[i], &g_bx_ui.drivers[i+1],
                    (size_t)(g_bx_ui.driver_count - 1 - i) * sizeof g_bx_ui.drivers[i]);
            g_bx_ui.driver_count--;
            i--;
        }
    }
}

/* ---------------------------------------------------------------- input */

/* Anything that takes a pointer. A container is a hit target only when one
 * of its children is not covering the same point, which bx_ui_hit already
 * handles, so the only question here is what responds at all. */
static int ui_interactive(const bx_ui_element_t *e) {
    if (!e || !e->visible || e->disabled) return 0;
    switch (e->kind) {
        case BX_UI_KIND_BUTTON:
        case BX_UI_KIND_CHECKBOX:
        case BX_UI_KIND_RADIO:
        case BX_UI_KIND_SWITCH:
        case BX_UI_KIND_STEPPER:
        case BX_UI_KIND_COMBOBOX:
        case BX_UI_KIND_CHIP:
        case BX_UI_KIND_PAGINATION:
        case BX_UI_KIND_SLIDER:
        case BX_UI_KIND_TEXTBOX:
        case BX_UI_KIND_LIST:
        case BX_UI_KIND_TAB:
        case BX_UI_KIND_MENU:
        case BX_UI_KIND_TREE:
            return 1;
        default:
            return 0;
    }
}

static void ui_set_target(bx_ui_element_t *e, float hover, float press) {
    if (!e) return;
    e->hover_target = hover;
    e->press_target = press;
}

void bx_ui_input_release(void) {
    for (int i = 0; i < g_bx_ui.count; i++) {
        bx_ui_element_t *e = &g_bx_ui.els[i];
        e->hover_target = 0.0f;
        e->press_target = 0.0f;
    }
    g_bx_ui.capture[0] = 0;
    g_bx_ui.hover_id[0] = 0;
}

/* Chase the targets. This is the whole of the feel of the UI: one rate
 * constant, applied to every interactive element, and nothing else. */
void bx_ui_input_tick(double dt) {
    if (dt <= 0) return;
    /* Exponential approach, framed so a full-scale move settles in roughly
     * 4/BX_UI_INPUT_RATE seconds whatever the frame rate. Independent of dt,
     * so a slow machine and a fast one look the same. */
    double k = 1.0 - exp(-BX_UI_INPUT_RATE * dt);
    for (int i = 0; i < g_bx_ui.count; i++) {
        bx_ui_element_t *e = &g_bx_ui.els[i];
        e->hover  += (float)((double)e->hover_target  - e->hover)  * k;
        e->press  += (float)((double)e->press_target  - e->press)  * k;
        e->focusv += (float)((double)e->focus_target  - e->focusv) * k;
        e->hovered = e->hover_target > 0.5f;
        e->pressed = e->press_target > 0.5f;
        e->focused = e->focus_target > 0.5f;
    }
}

bx_ui_element_t *bx_ui_pointer(float x, float y, int button, bx_ui_ptr_action_t action) {
    g_bx_ui.pointer_x = x;
    g_bx_ui.pointer_y = y;
    bx_ui_element_t *hit = bx_ui_hit(x, y);
    if (hit && !ui_interactive(hit)) hit = NULL;

    /* While a drag is live the element that was pressed keeps it, even if the
     * pointer has left it: a slider dragged past its end must not snap back
     * to the pointer's position on the next move. */
    bx_ui_element_t *held = g_bx_ui.capture[0] ? bx_ui_find(g_bx_ui.capture) : NULL;
    if (held) hit = held;

    switch (action) {
        case BX_UI_PTR_MOVE:
            for (int i = 0; i < g_bx_ui.count; i++)
                ui_set_target(&g_bx_ui.els[i], 0.0f, g_bx_ui.els[i].press_target);
            if (hit) {
                hit->hover_target = 1.0f;
                /* Hovering a child means hovering its container, so a panel
                 * can light up under the pointer without eating the event. */
                for (const char *p = hit->parent; *p;) {
                    bx_ui_element_t *pe = bx_ui_find(p);
                    if (!pe) break;
                    pe->hover_target = 0.45f;
                    p = pe->parent;
                }
            }
            break;

        case BX_UI_PTR_DOWN:
            if (hit) {
                hit->press_target = 1.0f;
                hit->hover_target = 1.0f;
                snprintf(g_bx_ui.capture, sizeof g_bx_ui.capture, "%s", hit->id);
                bx_ui_focus(hit->id);
                /* A checkbox and a switch change on the press, not the
                 * release: the release may never come, if the pointer drags
                 * off. A button waits for the release so a drag away cancels. */
                if (hit->kind == BX_UI_KIND_CHECKBOX || hit->kind == BX_UI_KIND_SWITCH)
                    hit->selected = !hit->selected;
            }
            break;

        case BX_UI_PTR_UP: {
            bx_ui_element_t *up = bx_ui_hit(x, y);
            if (!ui_interactive(up)) up = NULL;
            if (held) held->press_target = 0.0f;
            g_bx_ui.capture[0] = 0;
            /* A click only counts if the release is on the element that took
             * the press. Anything else is a cancelled click. */
            if (held && up == held) {
                switch (held->kind) {
                    case BX_UI_KIND_CHECKBOX:
                    case BX_UI_KIND_SWITCH:
                        break;               /* already toggled on press */
                    case BX_UI_KIND_RADIO:
                    case BX_UI_KIND_TAB:
                        held->selected = 1;
                        break;
                    case BX_UI_KIND_SLIDER: {
                        /* Click-to-position: the click sets where the handle
                         * is, which a drag alone would not. */
                        double f = (x - held->x) / (held->w > 0 ? held->w : 1);
                        if (f < 0) f = 0;
                        if (f > 1) f = 1;
                        if (held->w > 0) held->scroll = (float)f;
                        break;
                    }
                    default:
                        held->selected = !held->selected;
                        break;
                }
            }
            return up;
        }

        case BX_UI_PTR_WHEEL: {
            bx_ui_element_t *w = hit;
            if (!w && g_bx_ui.focused_id[0]) w = bx_ui_find(g_bx_ui.focused_id);
            if (!w) break;
            /* Scroll is a property too, so a script can set it and a driver
             * can animate it; the wheel just nudges it. */
            float delta = (button > 0) ? 0.08f : -0.08f;
            double v = (double)w->scroll + delta;
            if (v < 0) v = 0;
            if (v > 1) v = 1;
            w->scroll = (float)v;
            break;
        }
    }
    return hit;
}

bx_ui_element_t *bx_ui_focus(const char *id) {
    for (int i = 0; i < g_bx_ui.count; i++) g_bx_ui.els[i].focus_target = 0.0f;
    if (!id || !*id) { g_bx_ui.focused_id[0] = 0; return NULL; }
    bx_ui_element_t *e = bx_ui_find(id);
    if (!e || !ui_interactive(e)) return NULL;
    e->focus_target = 1.0f;
    snprintf(g_bx_ui.focused_id, sizeof g_bx_ui.focused_id, "%s", id);
    return e;
}

bx_ui_element_t *bx_ui_focused(void) {
    return g_bx_ui.focused_id[0] ? bx_ui_find(g_bx_ui.focused_id) : NULL;
}

bx_ui_element_t *bx_ui_focus_next(const char *from, int back) {
    /* Tab walks the elements in creation order, which is creation order in
     * every layout, so focus follows what the eye sees without the caller
     * having to know the tree. */
    int start = 0;
    if (from && *from) {
        for (int i = 0; i < g_bx_ui.count; i++)
            if (streqi(g_bx_ui.els[i].id, from)) { start = i + (back ? -1 : 1); break; }
    }
    int n = g_bx_ui.count;
    for (int k = 0; k < n; k++) {
        int i = ((start + k * (back ? -1 : 1)) % n + n) % n;
        if (ui_interactive(&g_bx_ui.els[i])) return &g_bx_ui.els[i];
    }
    return NULL;
}

int bx_ui_key(int key, const char *action) {
    int shift = (key >= 'A' && key <= 'Z');
    int lower = shift ? key + 32 : key;
    /* The key code carries the meaning, not the action: "backspace" arrives
     * as the code 8 with whatever action the backend reported. */
    int back = (key == 8);

    if (key == 25) {                                  /* shift-tab */
        bx_ui_element_t *cur = bx_ui_focused();
        bx_ui_element_t *nx = bx_ui_focus_next(cur ? cur->id : NULL, 1);
        if (nx) { bx_ui_focus(nx->id); return 1; }
        return 0;
    }
    if (key == 9 || streqi(action, "tab")) {
        bx_ui_element_t *cur = bx_ui_focused();
        bx_ui_element_t *nx = bx_ui_focus_next(cur ? cur->id : NULL, 0);
        if (nx) { bx_ui_focus(nx->id); return 1; }
        return 0;
    }
    if (streqi(action, "shifttab")) {
        bx_ui_element_t *cur = bx_ui_focused();
        bx_ui_element_t *nx = bx_ui_focus_next(cur ? cur->id : NULL, 1);
        if (nx) { bx_ui_focus(nx->id); return 1; }
        return 0;
    }
    if (streqi(action, "escape")) {
        bx_ui_focus(NULL);
        return 1;
    }

    bx_ui_element_t *e = bx_ui_focused();
    if (!e) return 0;

    if (e->kind == BX_UI_KIND_TEXTBOX) {
        if (back) {
            size_t l = strlen(e->text);
            if (l) e->text[l - 1] = 0;
            return 1;
        }
        if (lower == 13 || lower == 10) return 1;    /* newline ends the edit */
        if (lower == 27) return 1;                   /* escape ends the edit */
        if (key == 9) return 1;
        if (lower < 32 || lower > 126) return 0;
        size_t l = strlen(e->text);
        if (l + 2 < sizeof e->text) {
            e->text[l] = (char)lower;
            e->text[l + 1] = 0;
            return 1;
        }
        return 0;
    }

    /* Arrow keys nudge the focused control, which is what a focused control
     * has to do or focus is decoration. */
    if (e->kind == BX_UI_KIND_SLIDER || e->kind == BX_UI_KIND_PROGRESS) {
        if (lower == 37) { e->scroll = (float)(e->scroll - 0.05 < 0 ? 0 : e->scroll - 0.05); return 1; }
        if (lower == 39) { e->scroll = (float)(e->scroll + 0.05 > 1 ? 1 : e->scroll + 0.05); return 1; }
    }
    if (lower == 13 || lower == 32) {
        if (e->kind == BX_UI_KIND_CHECKBOX || e->kind == BX_UI_KIND_SWITCH) e->selected = !e->selected;
        else e->selected = !e->selected;
        return 1;
    }
    return 0;
}

/* ---------------------------------------------------------- element builder */

/* One line of a spec. Each line creates an element and sets its fields, or
 * sets fields on one that already exists:
 *
 *   id=root kind=frame w=200 h=120
 *   id=side kind=pane  parent=root dock=left w=120
 *   set id=ok x=20 y=40 text=Save
 *
 * Every key=value goes through bx_ui_set_field, so a spec can set anything a
 * ui set can, including a field this layer has never heard of, which lands in
 * the config bag and reads back. A spec is not a second way to describe an
 * element; it is the same description with the words "id=" in front.
 *
 * Blank lines and lines starting with # are skipped, so a spec can be commented
 * the way a .bx file is.
 */
static char *ui_dup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

/* One spec line, broken into key=value pairs before anything is created.
 * Two passes over these is what lets a line read in any order: parent= may
 * come before the parent exists on the previous line, and fields are applied
 * only once every element in the spec has been created. */
#define UI_SPEC_MAX_FIELDS 32
typedef struct {
    char key[32];
    char val[BX_UI_VALUE_MAX];
} ui_spec_field_t;

typedef struct {
    char id[BX_UI_ID_MAX];
    char kind[40];
    char parent[BX_UI_ID_MAX];
    ui_spec_field_t f[UI_SPEC_MAX_FIELDS];
    int nf;
    int update;              /* "set" line: never creates an element */
} ui_spec_line_t;

static void ui_spec_value_trim(char *v) {
    size_t n = strlen(v);
    if (n >= 2 && ((v[0] == '"' && v[n - 1] == '"') ||
                   (v[0] == '\'' && v[n - 1] == '\''))) {
        memmove(v, v + 1, n - 2);
        v[n - 2] = 0;
    }
}

/* Split a line into fields. Whitespace separates; a quoted value may contain
 * spaces, which is how a label with a space in it survives a round trip. */
static void ui_spec_split(char *line, ui_spec_line_t *out) {
    memset(out, 0, sizeof *out);
    char *p = line;
    while (*p) {
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        if (!*p || *p == '#') break;
        char key[32] = {0};
        int ki = 0;
        while (*p && *p != '=' && *p != ' ' && *p != '\t' && *p != ',' &&
               ki < (int)sizeof key - 1) key[ki++] = *p++;
        key[ki] = 0;
        if (*p == '=') p++;
        char val[BX_UI_VALUE_MAX];
        size_t vi = 0;
        if (*p == '"' || *p == '\'') {
            char q = *p++;
            while (*p && *p != q && vi + 1 < sizeof val) val[vi++] = *p++;
            if (*p == q) p++;
        } else {
            while (*p && *p != ' ' && *p != '\t' && *p != ',' && *p != '#' &&
                   vi + 1 < sizeof val) val[vi++] = *p++;
        }
        val[vi] = 0;
        if (!key[0]) continue;
        ui_spec_value_trim(val);
        if (!strcmp(key, "id")) copy_id(out->id, val);
        else if (!strcmp(key, "kind")) snprintf(out->kind, sizeof out->kind, "%s", val);
        else if (!strcmp(key, "parent")) copy_id(out->parent, val);
        else if (out->nf < UI_SPEC_MAX_FIELDS) {
            snprintf(out->f[out->nf].key, sizeof out->f->key, "%s", key);
            snprintf(out->f[out->nf].val, sizeof out->f->val, "%s", val);
            out->nf++;
        }
    }
}

/* A value needs quoting when it would not survive a round trip: whitespace, a
 * comma, a quote, or a trailing # all end a bare field early. */
static int ui_spec_needs_quote(const char *v) {
    if (!*v) return 1;
    for (const char *p = v; *p; p++) {
        if (*p == ' ' || *p == '\t' || *p == ',' || *p == '"' || *p == '\'' ||
            *p == '#' || *p == '=')
            return 1;
    }
    return 0;
}

int bx_ui_build(const char *text, char *err, size_t errcap) {
    if (!text) return -1;
    if (err && errcap) err[0] = 0;

    /* Pass one splits every line; pass two creates; pass three applies. */
    int cap = 16, nlines = 0;
    ui_spec_line_t *lines = (ui_spec_line_t *)calloc((size_t)cap, sizeof *lines);
    char *copy = ui_dup(text);
    if (!lines || !copy) { free(lines); free(copy); return -1; }

    char *line = copy;
    while (line && *line) {
        char *nl = strchr(line, '\n');
        if (nl) *nl = 0;
        char *next = nl ? nl + 1 : NULL;
        char *t = line;
        while (*t == ' ' || *t == '\t') t++;
        if (*t && *t != '#') {
            /* "set" edits an element that already exists, without creating
             * one. A line that only says "set" is a no-op, not an error. */
            if (!strncmp(t, "set ", 4) || !strncmp(t, "set\t", 4)) {
                t += 3;
                while (*t == ' ' || *t == '\t') t++;
            }
            if (*t) {
                if (nlines >= cap) {
                    cap *= 2;
                    ui_spec_line_t *bigger =
                        (ui_spec_line_t *)realloc(lines, (size_t)cap * sizeof *lines);
                    if (!bigger) {
                        if (err) snprintf(err, errcap, "out of memory");
                        free(lines); free(copy);
                        return -1;
                    }
                    lines = bigger;
                }
                ui_spec_split(t, &lines[nlines]);
                lines[nlines].update = (lines[nlines].kind[0] == 0);
                nlines++;
            }
        }
        line = next;
    }

    /* Create every element first, so a parent may be declared after a child
     * references it. bx_ui_add attaches to the parent if it already exists, so
     * the second loop picks up the ones that did not. */
    int made = 0;
    for (int i = 0; i < nlines; i++) {
        ui_spec_line_t *L = &lines[i];
        if (L->update || !L->id[0] || !L->kind[0]) continue;
        int k = bx_ui_kind_by_name(L->kind);
        bx_ui_element_t *e = bx_ui_add(L->id,
                k >= 0 ? (bx_ui_kind_t)k : BX_UI_KIND_CUSTOM, L->parent);
        if (!e) {
            if (err) snprintf(err, errcap, "cannot create '%s'", L->id);
            free(lines); free(copy);
            return -1;
        }
        /* An unknown kind becomes a container that holds fields, so a spec can
         * describe an element this build has never heard of and still get it
         * onto the tree. */
        if (k < 0) { e->layout = BX_UI_LAYOUT_COLUMN; e->pad_x = 8; e->pad_y = 8; e->gap = 6; }
        made++;
    }
    for (int i = 0; i < nlines; i++) {
        ui_spec_line_t *L = &lines[i];
        if (L->parent[0] && L->id[0]) {
            bx_ui_element_t *par = bx_ui_find(L->parent);
            bx_ui_element_t *e = bx_ui_find(L->id);
            if (par && e && !e->parent[0]) {
                copy_id(e->parent, L->parent);
                bx_ui_child_add(par, e->id);
            }
        }
    }

    /* Apply fields last, in spec order. */
    for (int i = 0; i < nlines; i++) {
        ui_spec_line_t *L = &lines[i];
        if (!L->id[0]) continue;
        bx_ui_element_t *e = bx_ui_find(L->id);
        if (!e) {
            if (err) snprintf(err, errcap, "no element '%s'", L->id);
            free(lines); free(copy);
            return -1;
        }
        for (int j = 0; j < L->nf; j++)
            bx_ui_set_field(e, L->f[j].key, L->f[j].val);
    }

    free(lines);
    free(copy);
    return made;
}

/* Fields worth writing out. A value equal to the kind's default is left out,
 * so a dump is readable and rebuilding it does not depend on defaults holding. */
static const char *g_spec_keys[] = {
    "x", "y", "w", "h", "minw", "minh", "text", "value", "style", "color",
    "radius", "dock", "layout", "align", "justify", "gap", "padx", "pady",
    "columns", "z", "alpha", "material", "selected"
};

int bx_ui_spec_dump(char *out, size_t cap) {
    if (!out || cap == 0) return 0;
    size_t n = 0;
    int lines = 0;
    for (int i = 0; i < g_bx_ui.count; i++) {
        bx_ui_element_t *e = &g_bx_ui.els[i];
        if (!e->id[0]) continue;
        /* An element must exist before its parent does, or a rebuild cannot
         * attach it: dump creation order, which is the order they were made. */
        if (e->parent[0] && !bx_ui_find(e->parent)) continue;
        int w = snprintf(out + n, cap - n, "id=%s kind=%s", e->id, bx_ui_kind_name(e->kind));
        if (w < 0) break;
        n += (size_t)w;
        lines++;
        for (unsigned k = 0; k < sizeof g_spec_keys / sizeof *g_spec_keys; k++) {
            char v[BX_UI_VALUE_MAX];
            if (bx_ui_get_field(e, g_spec_keys[k], v, sizeof v) != 0) continue;
            if (!v[0]) continue;
            /* Leave out anything still at its default: a dump that repeats
             * every field is noise, and one that has to be read against a
             * default table is worse than one that is simply short. Layout
             * results are not defaults, though, so they stay. */
            if (!strcmp(v,"0") || !strcmp(v,"none") || !strcmp(v,"start") ||
                !strcmp(v,"matte") || !strcmp(v,"1") ||
                !strcmp(v,"0x00000000"))
                continue;
            if (ui_spec_needs_quote(v))
                w = snprintf(out + n, cap - n, " %s=\"%s\"", g_spec_keys[k], v);
            else
                w = snprintf(out + n, cap - n, " %s=%s", g_spec_keys[k], v);
            if (w < 0) break;
            n += (size_t)w;
            if (n + 64 >= cap) { out[n] = 0; return lines; }
        }
        if (e->parent[0]) {
            w = snprintf(out + n, cap - n, " parent=%s", e->parent);
            if (w > 0) n += (size_t)w;
        }
        for (int c = 0; c < e->config_count; c++) {
            if (n + 96 >= cap) { out[n] = 0; return lines; }
            if (ui_spec_needs_quote(e->config_val[c]))
                w = snprintf(out + n, cap - n, " %s=\"%s\"", e->config_key[c], e->config_val[c]);
            else
                w = snprintf(out + n, cap - n, " %s=%s", e->config_key[c], e->config_val[c]);
            if (w > 0) n += (size_t)w;
        }
        if (n + 2 >= cap) break;
        out[n++] = '\n';
    }
    out[n] = 0;
    return lines;
}
