/* BX UI - frames, layouts, docking, elements, tweens, and frame pacing.
 *
 * This layer sits on top of bx_gfx: it decides where things go and how they
 * move, while bx_gfx still owns pixels and colours.
 *
 * The design rules that matter:
 *
 *   IDs are strings. Boxes are string-based, so a UI object named "sidebar"
 *   should be reachable as "sidebar" and not as a number that drifts every
 *   run. An id is interned once and reused, so identity is cheap.
 *
 *   A frame is a container with a layout and an ordered list of child ids.
 *   Frames nest, so a frame can be docked inside a pane and the pane itself
 *   docked to a region.
 *
 *   Tweens are objects, not one-shot calls. A tween with an id keeps its
 *   start, target, clock and easing across frames. Re-setting the same id
 *   retargets it instead of starting a second one, which is what makes a
 *   layout feel continuous instead of restarting on every event.
 *
 *   Timing follows the same idea as the rest of the language: a fixed 60 Hz
 *   step with a measured delta, so motion is smooth on a 60 Hz display and
 *   still correct on a 120 Hz one.
 */

#ifndef BX_UI_H
#define BX_UI_H

#include <stdint.h>
#include "bx_gfx.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---------------------------------------------------------------- limits */
#define BX_UI_ID_MAX      32
#define BX_UI_MAX_ELEMENTS 1024
#define BX_UI_MAX_TWEENS  256
#define BX_UI_MAX_FNS     64
#define BX_UI_FPS_DEFAULT 60

/* Element kinds. Custom kinds start at BX_UI_KIND_CUSTOM so a user element
 * never collides with a built-in one. */
typedef enum {
    BX_UI_KIND_FRAME = 0,
    BX_UI_KIND_PANE,
    BX_UI_KIND_PANEL,
    BX_UI_KIND_BUTTON,
    BX_UI_KIND_LABEL,
    BX_UI_KIND_TEXT,
    BX_UI_KIND_TEXTBOX,
    BX_UI_KIND_SLIDER,
    BX_UI_KIND_CHECKBOX,
    BX_UI_KIND_RADIO,
    BX_UI_KIND_DROPDOWN,
    BX_UI_KIND_IMAGE,
    BX_UI_KIND_ICON,          /* renders a bxvg document */
    BX_UI_KIND_SHAPE,
    BX_UI_KIND_LIST,
    BX_UI_KIND_LISTITEM,
    BX_UI_KIND_TABS,
    BX_UI_KIND_TAB,
    BX_UI_KIND_SPLIT,
    BX_UI_KIND_SCROLL,
    BX_UI_KIND_GRID,
    BX_UI_KIND_STACK,
    BX_UI_KIND_PROGRESS,
    BX_UI_KIND_SPINNER,
    BX_UI_KIND_CANVAS,
    BX_UI_KIND_TREE,
    BX_UI_KIND_MENU,
    BX_UI_KIND_MENUITEM,
    BX_UI_KIND_TOOLBAR,
    BX_UI_KIND_STATUSBAR,
    BX_UI_KIND_SIDEBAR,
    BX_UI_KIND_MODAL,
    BX_UI_KIND_TOOLTIP,
    BX_UI_KIND_TWEEN,         /* a tween object, also an element */
    BX_UI_KIND_CUSTOM = 1000
} bx_ui_kind_t;

/* Layout modes for a frame. */
typedef enum {
    BX_UI_LAYOUT_NONE = 0,     /* absolute: x/y are honoured */
    BX_UI_LAYOUT_ROW,
    BX_UI_LAYOUT_COLUMN,
    BX_UI_LAYOUT_GRID,
    BX_UI_LAYOUT_STACK,        /* every child at the same origin, z-ordered */
    BX_UI_LAYOUT_WRAP
} bx_ui_layout_t;

/* Cross-axis alignment within a layout. */
typedef enum {
    BX_UI_ALIGN_START = 0,
    BX_UI_ALIGN_CENTER,
    BX_UI_ALIGN_END,
    BX_UI_ALIGN_STRETCH
} bx_ui_align_t;

/* Justification on the main axis. */
typedef enum {
    BX_UI_JUSTIFY_START = 0,
    BX_UI_JUSTIFY_CENTER,
    BX_UI_JUSTIFY_END,
    BX_UI_JUSTIFY_SPACE_BETWEEN
} bx_ui_justify_t;

/* Where a pane docks. */
typedef enum {
    BX_UI_DOCK_NONE = 0,
    BX_UI_DOCK_LEFT,
    BX_UI_DOCK_RIGHT,
    BX_UI_DOCK_TOP,
    BX_UI_DOCK_BOTTOM,
    BX_UI_DOCK_CENTER,
    BX_UI_DOCK_FILL
} bx_ui_dock_t;

typedef enum {
    BX_UI_TWEEN_IDLE = 0,
    BX_UI_TWEEN_RUNNING,
    BX_UI_TWEEN_DONE
} bx_ui_tween_state_t;

/* A UI element. Every one has an id; identity is the id, not the index. */
typedef struct {
    char     id[BX_UI_ID_MAX];
    char     parent[BX_UI_ID_MAX];
    bx_ui_kind_t kind;
    bx_ui_dock_t dock;

    /* Geometry is float so a tween can drive it per frame. Layout writes the
     * same fields, so layout and animation compose instead of fighting. */
    float    x, y, w, h;
    float    min_w, min_h;
    int      visible;
    int      z;

    /* Layout, only meaningful on a frame. */
    bx_ui_layout_t layout;
    bx_ui_align_t  align_h, align_v;
    bx_ui_justify_t justify;
    float    gap, pad_x, pad_y;
    int      columns;

    /* Appearance. */
    bx_gfx_theme_t theme;
    char     text[128];
    char     value[64];       /* slider/textbox/check value, as text */
    char     style[32];       /* named style preset */
    char     bxvg[64];        /* for icons: the bxvg document id */
    uint32_t color, color2;

    /* Children, in order. */
    char     children[64][BX_UI_ID_MAX];
    int      child_count;

    /* Input state, kept so a hit test and a redraw agree. */
    int      hovered, pressed, focused, disabled;
} bx_ui_element_t;

/* An easing function. Every tween points at one by name. */
typedef struct {
    char   name[BX_UI_ID_MAX];
    int    kind;
    /* Built-in polynomials use these; a custom cubic bezier uses the four
     * control points, which is how iOS and the web express timing. */
    double p[4];
} bx_ui_fn_t;

enum {
    BX_UI_FN_LINEAR = 0,
    BX_UI_FN_IN,
    BX_UI_FN_OUT,
    BX_UI_FN_INOUT,
    BX_UI_FN_SPRING,
    BX_UI_FN_BOUNCE,
    BX_UI_FN_ELASTIC,
    BX_UI_FN_BEZIER,      /* four control points */
    BX_UI_FN_MXB          /* y = mx + b */
};

/* A tween object. Same id = same tween, retargeted in place. */
typedef struct {
    char     id[BX_UI_ID_MAX];
    int      used;
    char     prop[16];      /* which channel moves: x, y, w, h, alpha, or a number */
    char     target[64];    /* element id this tween drives */
    float    from, to;
    float    value;         /* current interpolated value */
    float    duration;      /* seconds */
    float    elapsed;
    float    delay;
    char     fn[BX_UI_ID_MAX];
    bx_ui_tween_state_t state;
    int      loop;          /* 0 once, 1 ping-pong, >1 loop count */
    int      bounces;
} bx_ui_tween_t;

/* Frame pacing. */
typedef struct {
    int      fps;
    double   last;           /* clock at the previous step, seconds */
    double   dt;             /* seconds since the previous step */
    double   acc;            /* unsimulated time carried forward */
    uint64_t frame;          /* steps taken */
    double   now;
    int      running;
    int      single_shot;    /* frame runs one step then stops */
} bx_ui_clock_t;

typedef struct {
    bx_ui_element_t els[BX_UI_MAX_ELEMENTS];
    int         count;
    bx_ui_tween_t tweens[BX_UI_MAX_TWEENS];
    bx_ui_fn_t   fns[BX_UI_MAX_FNS];
    bx_ui_clock_t clock;
    char       focused[BX_UI_ID_MAX];
    char       active_frame[BX_UI_ID_MAX];
} bx_ui_ctx_t;

extern bx_ui_ctx_t g_bx_ui;

/* ---------------------------------------------------------------- setup */
void bx_ui_init(void);
void bx_ui_reset(void);

/* Ids. Interned so a second "sidebar" is the same object as the first. */
int  bx_ui_id_valid(const char *id);

/* ---------------------------------------------------------------- elements */
bx_ui_element_t *bx_ui_find(const char *id);
bx_ui_element_t *bx_ui_add(const char *id, bx_ui_kind_t kind, const char *parent);
int  bx_ui_remove(const char *id);
int  bx_ui_kind_by_name(const char *name);
const char *bx_ui_kind_name(bx_ui_kind_t kind);
int  bx_ui_kind_count(void);
const char *bx_ui_kind_at(int index);

/* Children */
int  bx_ui_child_add(bx_ui_element_t *parent, const char *child);
int  bx_ui_child_remove(bx_ui_element_t *parent, const char *child);
int  bx_ui_child_index(const bx_ui_element_t *parent, const char *child);

/* ---------------------------------------------------------------- layout */
int  bx_ui_set_rect(const char *id, float x, float y, float w, float h);
int  bx_ui_layout_apply(const char *frame_id);
void bx_ui_dock_apply(bx_ui_element_t *pane, const bx_gfx_fb_t *fb);

/* ---------------------------------------------------------------- tweens */
bx_ui_tween_t *bx_ui_tween_get(const char *id);
int  bx_ui_tween_start(const char *id, const char *target, const char *prop,
                       float from, float to, float duration, const char *fn);
int  bx_ui_tween_retarget(const char *id, float to);
int  bx_ui_tween_cancel(const char *id);
int  bx_ui_tween_tick(float dt);
int  bx_ui_tween_running(void);

/* ---------------------------------------------------------------- easing */
int  bx_ui_fn_define(const char *name, int kind, const double *p);
int  bx_ui_fn_define_bezier(const char *name, double x1, double y1, double x2, double y2);
int  bx_ui_fn_define_mxb(const char *name, double m, double b);
int  bx_ui_fn_default(const char *name);
double bx_ui_ease(const char *name, double t);
int  bx_ui_fn_count(void);
const bx_ui_fn_t *bx_ui_fn_at(int index);

/* ---------------------------------------------------------------- clock */
void bx_ui_clock_reset(int fps);
int  bx_ui_frame_step(void);   /* advance one paced step, returns 1 if stepped */
double bx_ui_now(void);

#ifdef __cplusplus
}
#endif

#endif /* BX_UI_H */
