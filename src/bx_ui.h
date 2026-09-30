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
#define BX_UI_MAX_CHILDREN 64
/* A custom element is a bag of values, so it needs somewhere to keep them.
 * Fixed rather than allocated: the element array is static, and an element
 * that costs a pointer chase to configure is a worse trade than 8 slots. */
#define BX_UI_MAX_CONFIG  8
#define BX_UI_CONFIG_KEY  20
#define BX_UI_CONFIG_VAL  48
/* Drivers: one property of one element, recomputed every frame. */
#define BX_UI_MAX_DRIVERS 256
/* Procedural textures, computed per pixel from math. */
#define BX_UI_MAX_TEXTURES 64
#define BX_UI_MAX_FNS2    64
/* How fast hover and press chase their targets. A rate, not a duration: one
 * number covers every speed, and a zero dt costs nothing. */
#define BX_UI_INPUT_RATE   12.0
#define BX_UI_FPS_DEFAULT 60

/* Element kinds. Custom kinds start at BX_UI_KIND_CUSTOM so a user element
 * never collides with a built-in one. */
/* What a surface is made of. Matte is opaque and flat: a solid panel that
 * hides whatever is behind it. Glass is translucent with a lit edge and a
 * vertical sheen, so it reads as a surface with light on it rather than a
 * rectangle of slightly see-through paint. */
typedef enum {
    BX_UI_MAT_MATTE = 0,
    BX_UI_MAT_GLASS,
    BX_UI_MAT_FLAT      /* no fill at all, just the outline */
} bx_ui_material_t;

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

    /* Controls and containers added with the input layer. Each one exists
     * because something needed it to be a real target rather than a custom
     * element: a switch is a checkbox that looks like one, a stepper is a
     * slider that snaps, and so on. */
    BX_UI_KIND_SWITCH,
    BX_UI_KIND_STEPPER,
    BX_UI_KIND_COMBOBOX,
    BX_UI_KIND_GROUPBOX,
    BX_UI_KIND_FIELD,
    BX_UI_KIND_DIVIDER,
    BX_UI_KIND_BADGE,
    BX_UI_KIND_CHIP,
    BX_UI_KIND_ALERT,
    BX_UI_KIND_CARD,
    BX_UI_KIND_HEADER,
    BX_UI_KIND_FOOTER,
    BX_UI_KIND_BREADCRUMB,
    BX_UI_KIND_PAGINATION,
    BX_UI_KIND_DRAWER,

    /* The remaining names a UI is normally asked for. Some are containers and
     * some draw something of their own, but every one of them is a real kind
     * with a name, because "a panel with a different style" is not the same
     * thing as a toolbelt, and a spec should be able to say which it means. */
    BX_UI_KIND_NAV,
    BX_UI_KIND_TOOLBELT,
    BX_UI_KIND_TABLE,
    BX_UI_KIND_ROW,
    BX_UI_KIND_CELL,
    BX_UI_KIND_DIAL,
    BX_UI_KIND_GAUGE,
    BX_UI_KIND_METER,
    BX_UI_KIND_KNOB,
    BX_UI_KIND_BREADCRUMB_SEP,
    BX_UI_KIND_ACCORDION,
    BX_UI_KIND_AVATAR,
    BX_UI_KIND_LINK,
    BX_UI_KIND_KBD,
    BX_UI_KIND_CODEBLOCK,
    BX_UI_KIND_BLOCKQUOTE,
    BX_UI_KIND_WELL,
    BX_UI_KIND_SPACER,
    BX_UI_KIND_OVERLAY,
    BX_UI_KIND_RIPPLE,
    BX_UI_KIND_SKELETON,
    BX_UI_KIND_BANNER,
    BX_UI_KIND_TOAST,
    BX_UI_KIND_EMPTY,
    BX_UI_KIND_LOADING,
    BX_UI_KIND_STEPPER_DOT,
    BX_UI_KIND_RATING,
    BX_UI_KIND_TOGGLE_GROUP,
    BX_UI_KIND_COLOR,
    BX_UI_KIND_THUMBNAIL,
    BX_UI_KIND_TILE,
    BX_UI_KIND_BAR_GROUP,
    BX_UI_KIND_LEGEND,
    BX_UI_KIND_HINT,
    BX_UI_KIND_LABEL_GROUP,
    BX_UI_KIND_TOOLTIP_ARROW,
    BX_UI_KIND_MENU_SEP,
    BX_UI_KIND_SPLIT_PANE,
    BX_UI_KIND_SIDEBAR_ITEM,
    BX_UI_KIND_STATUS,
    BX_UI_KIND_ICON_BUTTON,

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
    bx_ui_material_t material;
    int      radius;           /* corner radius in pixels, 0 for square */
    char     text[128];
    char     value[64];       /* slider/textbox/check value, as text */
    char     style[32];       /* named style preset */
    /* Free-form settings, for custom kinds and for fields the layer does not
     * know yet. Read back with ui get like any other field. */
    char     config_key[BX_UI_MAX_CONFIG][BX_UI_CONFIG_KEY];
    char     config_val[BX_UI_MAX_CONFIG][BX_UI_CONFIG_VAL];
    int      config_count;
    char     bxvg[64];        /* for icons: the bxvg document id */
    uint32_t color, color2;

    /* Children, in order. */
    char     children[64][BX_UI_ID_MAX];
    int      child_count;

    /* Input state, kept so a hit test and a redraw agree. The three amounts
     * are continuous 0..1 rather than flags: input moves them toward 1 or 0
     * and the frame step eases them there, so a hover can fade and a press can
     * sink. Because they are ordinary fields, ui drive and ui tween can take
     * them over, which is how a scripted animation gets the same look as a
     * real pointer without a pointer. */
    int      hovered, pressed, focused, disabled;
    float    hover, press, focusv;
    float    hover_target, press_target, focus_target;
    int      selected;         /* tabs, radio, checkbox: the "on" visual */
    float    scroll;           /* 0..1, for scroll, list, and text */
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
    /* Total simulated time, in seconds, since the clock started. Everything
     * that is a function of time -- drivers, and texture animation -- reads
     * this rather than the wall clock, so a fixed-step frame advances it the
     * same way a real one does and a test is reproducible. */
    double   elapsed;
    double   pending;       /* fixed delta a single-shot frame will commit */
    double   now;
    int      running;
    int      single_shot;    /* frame runs one step then stops */
} bx_ui_clock_t;

/* ----------------------------------------------------------- math functions */

/* A named function of t. These are what make a property be math instead of a
 * number: the same shape as an easing curve, but unbounded and not tied to
 * the 0..1 that a tween spends. */
enum {
    BX_UI_MFN_MXB = 0,   /* y = mx + b, the null model everything else departs from */
    BX_UI_MFN_SIN,       /* amp*sin(2*pi*freq*t + phase) + offset */
    BX_UI_MFN_TRI,       /* triangle wave: 0..1..0, for motion that must not ease */
    BX_UI_MFN_DECAY,     /* exp(-k*t): for something that settles and stays */
    BX_UI_MFN_STEP,      /* quantised, for a value that should not be continuous */
    BX_UI_MFN_NOISE,     /* deterministic hash of t, for flicker and shimmer */
    BX_UI_MFN_SQRT       /* sqrt, which is slower and therefore reads as weight */
};

typedef struct {
    char   name[BX_UI_ID_MAX];
    int    kind;
    double p[4];
} bx_ui_mfn_t;

int  bx_ui_mathfn_define_mxb(const char *name, double m, double b);
int  bx_ui_mathfn_define(const char *name, int kind, const double *p);
double bx_ui_mathfn_eval(const char *name, double t);
int  bx_ui_mathfn_count(void);
const bx_ui_mfn_t *bx_ui_mathfn_at(int i);

/* --------------------------------------------------------------- drivers */

/* What computes a property each frame. A driver names an element and a
 * property, and a source: a constant, a math function of time, a function
 * remapped onto a range, or another element's property. */
enum {
    BX_UI_DRIVE_CONST = 0,
    BX_UI_DRIVE_MFN,      /* fn|t0|t1|lo|hi  - remapped into a range */
    BX_UI_DRIVE_MIRROR,   /* mirror|id|prop   - follows another element */
    BX_UI_DRIVE_WAVE      /* fn|lo|hi        - oscillates between lo and hi */
};

typedef struct {
    char   id[BX_UI_ID_MAX];
    char   prop[24];
    int    kind;
    double v;             /* CONST */
    char   fn[BX_UI_ID_MAX];
    double t0, t1;        /* MFN: the time window the function is sampled over */
    double lo, hi;        /* MFN/WAVE: the range the result lands in */
    char   src_id[BX_UI_ID_MAX];  /* MIRROR */
    char   src_prop[24];
} bx_ui_driver_t;

int  bx_ui_drive(const char *id, const char *prop, int kind, const double *p,
                 const char *fn, const char *src_id, const char *src_prop);
int  bx_ui_drive_count(void);
const bx_ui_driver_t *bx_ui_drive_at(int i);
void bx_ui_drive_remove(const char *id, const char *prop);
/* Evaluate every driver. Called once per frame, before layout and render. */
int  bx_ui_apply_drivers(double t);

/* --------------------------------------------------------------- textures */

/* A fill computed per pixel from math rather than stored as an image. Cheap
 * to animate, because animating a texture means evaluating the function
 * again with a new t, not pushing pixels. */
enum {
    BX_UI_TEX_SOLID = 0,
    BX_UI_TEX_GRADV,
    BX_UI_TEX_GRADH,
    BX_UI_TEX_CHECKER,
    BX_UI_TEX_STRIPES,
    BX_UI_TEX_DOTS,
    BX_UI_TEX_GRID,
    BX_UI_TEX_NOISE,
    BX_UI_TEX_RING,
    BX_UI_TEX_WAVE,      /* amplitude*sin(freq*x + phase) modulating c1..c2 */
    BX_UI_TEX_MFN        /* a math function of y drives the gradient */
};

typedef struct {
    char     id[BX_UI_ID_MAX];
    int      kind;
    double   p[4];        /* pattern parameters, meaning depends on kind */
    char     fn[BX_UI_ID_MAX];
    uint32_t c1, c2;
    int      active;
} bx_ui_texture_t;

int bx_ui_texture_set(const char *id, int kind, const double *p,
                      const char *fn, const char *c1, const char *c2);
int bx_ui_texture_count(void);
const bx_ui_texture_t *bx_ui_texture_at(int i);
/* Draw an element's texture, if it has one. Returns 1 if it drew. */
int bx_ui_texture_draw(bx_gfx_fb_t *fb, bx_ui_element_t *e);

typedef struct {
    bx_ui_element_t els[BX_UI_MAX_ELEMENTS];
    int         count;
    bx_ui_tween_t tweens[BX_UI_MAX_TWEENS];
    bx_ui_fn_t   fns[BX_UI_MAX_FNS];
    bx_ui_clock_t clock;
    bx_ui_driver_t drivers[BX_UI_MAX_DRIVERS];
    int         driver_count;
    bx_ui_texture_t textures[BX_UI_MAX_TEXTURES];
    int         texture_count;
    bx_ui_mfn_t mathfns[BX_UI_MAX_FNS2];
    int         mathfn_count;
    char       focused[BX_UI_ID_MAX];
    char       capture[BX_UI_ID_MAX];   /* holds the pointer while a drag is live */
    char       hover_id[BX_UI_ID_MAX];
    char       focused_id[BX_UI_ID_MAX];
    float      pointer_x, pointer_y;
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

/* --------------------------------------------------------------- fields */

/* The one place a property is written. ui.set, a driver, and a tween all go
 * through here, which is what lets any property be driven by math without a
 * second code path that can disagree about what "x" means.
 * Returns 0 on success, -1 if the field is unknown and was stored as a
 * config value, -2 if the value did not make sense. */
int bx_ui_set_field(bx_ui_element_t *e, const char *field, const char *value);
/* Read a property back into buf. Returns 0 on success, -1 if unknown. */
int bx_ui_get_field(const bx_ui_element_t *e, const char *field, char *buf, size_t cap);
const char *bx_ui_config_get(const bx_ui_element_t *e, const char *key);

/* ---------------------------------------------------------- element builder */

/* Build elements from a spec: one line per element, id= and kind= to create,
 * every other key=value is a field set through bx_ui_set_field. Returns the
 * number created, or -1 with err set. */
int bx_ui_build(const char *text, char *err, size_t errcap);
/* Write the tree back as a spec. What this prints, bx_ui_build reads. */
int bx_ui_spec_dump(char *out, size_t cap);
#define BX_UI_SPEC_MAX 65536
/* A spec value may be any field a ui set accepts, and the longest of those is
 * an element's text, so the buffer is sized to text rather than to a config
 * slot: quoting and re-parsing must not truncate what was written. */
#define BX_UI_VALUE_MAX 128

/* --------------------------------------------------------------- render */
/* Draw the tree into a framebuffer. Returns how many elements were drawn.
 * The framebuffer must already be the right size; ui render clears nothing,
 * so a caller can draw a background first. */
int bx_ui_render(bx_gfx_fb_t *fb);
/* Hit test: the topmost visible element whose rect contains (x,y), or NULL.
 * Children are searched before parents, and higher z wins, so the answer is
 * the thing the pointer is actually over. */
bx_ui_element_t *bx_ui_hit(float x, float y);

/* --------------------------------------------------------------- input */

/* Backend-neutral input. Nothing here knows about a windowing library: a
 * platform feeds it coordinates and key names, and the same calls work for a
 * script, a test, or a real event loop. */
typedef enum {
    BX_UI_PTR_MOVE = 0,
    BX_UI_PTR_DOWN,
    BX_UI_PTR_UP,
    BX_UI_PTR_WHEEL
} bx_ui_ptr_action_t;

/* Feed a pointer event. Returns the element it landed on, or NULL. */
bx_ui_element_t *bx_ui_pointer(float x, float y, int button, bx_ui_ptr_action_t action);
/* Feed a key. Returns 1 if something handled it. Printable keys are the
 * character itself, so "a" types an a. */
int bx_ui_key(int key, const char *action);
/* The element that would take focus next, for Tab. */
bx_ui_element_t *bx_ui_focus_next(const char *from, int back);
/* Move focus. Returns the newly focused element, or NULL for none. */
bx_ui_element_t *bx_ui_focus(const char *id);
bx_ui_element_t *bx_ui_focused(void);
/* Eased input state. Called from the frame step. */
void bx_ui_input_tick(double dt);
/* Nothing has the pointer any more, so drop hover and press. */
void bx_ui_input_release(void);

/* The site palette, as 0xRRGGBBAA: the framebuffer packs alpha last, so a
 * color parsed from "#rrggbb" can be written here directly. */
static const uint32_t BX_UI_C_BG        = 0x0B0F14FFu;
static const uint32_t BX_UI_C_PANEL     = 0x131A22FFu;
static const uint32_t BX_UI_C_INSET     = 0x0F151CFFu;
static const uint32_t BX_UI_C_BORDER    = 0x233040FFu;
static const uint32_t BX_UI_C_TEXT      = 0xD7E2EEFFu;
static const uint32_t BX_UI_C_TEXT_DIM  = 0x8497ABFFu;
static const uint32_t BX_UI_C_ACCENT    = 0x4FD6C4FFu;
static const uint32_t BX_UI_C_ACCENT_DK = 0x2A8D81FFu;

/* ---------------------------------------------------------------- clock */
void bx_ui_clock_reset(int fps);
int  bx_ui_frame_step(void);   /* advance one paced step, returns 1 if stepped */
/* Advance simulated time by dt, ticking tweens and then drivers, in that
 * order. ui frame|step|DT is this, and so is every paced frame. */
void bx_ui_frame_step_dt(double dt);
double bx_ui_now(void);

#ifdef __cplusplus
}
#endif

#endif /* BX_UI_H */
