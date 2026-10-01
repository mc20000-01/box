/* BX VG - vector graphics for bx.
 *
 * A document is a set of shapes in a coordinate space of its own, rendered
 * into whatever framebuffer it is given and scaled to fit a rect. Three things
 * make it worth having over calling the primitives directly:
 *
 *   - Shapes are named and stored, so a drawing is a thing a program can
 *     build, change and re-render, not a sequence of side effects.
 *   - Frames. A named frame is one arrangement of the shapes, and a document
 *     can be stepped through frames or moved between them, which is how an
 *     icon becomes an animation without a second format.
 *   - SVG in and out, so artwork made elsewhere can be used and artwork made
 *     here can go somewhere that is not here.
 *
 * The document format is a text grammar, because bx's own is: whitespace
 * separated, numbers and names, comments with #. It parses the same way in
 * every front end, so a .bxvg is readable and diffable.
 */
#ifndef BX_BXVG_H
#define BX_BXVG_H

#include <stdint.h>
#include <stddef.h>
#include "bx_gfx.h"

/* The rasterizer's framebuffer type. Declared under its own name so callers
 * that only want the document grammar do not have to think about pixels. */
typedef bx_gfx_fb_t bx_vg_fb_t;

/* ------------------------------------------------------------------ shapes */

typedef enum {
    BX_VG_PATH = 0,      /* move/line/curve/close, the only filled kind */
    BX_VG_RECT,
    BX_VG_CIRCLE,
    BX_VG_ELLIPSE,
    BX_VG_LINE,
    BX_VG_POLY,
    BX_VG_TEXT
} bx_vg_shape_type_t;

/* One command in a path. Kept as a flat array so a path is one allocation. */
typedef struct {
    uint8_t op;           /* BX_VG_OP_* */
    float   v[6];         /* x,y for most; x1,y1,x2,y2,x,y for curves */
} bx_vg_cmd_t;

#define BX_VG_OP_MOVE 0
#define BX_VG_OP_LINE 1
#define BX_VG_OP_CURVE 2   /* cubic: v[0..1] control, v[2..3] control, v[4..5] end */
#define BX_VG_OP_CLOSE 3
#define BX_VG_OP_ARC 4     /* v[0..1] centre, v[2] rx, v[3] ry, v[4..5] start/end deg */

#define BX_VG_MAX_CMDS 512

typedef struct {
    int32_t  id;             /* 1-based, stable within a document */
    char     name[32];
    bx_vg_shape_type_t type;
    float    x, y, w, h;     /* rect-ish geometry, also the path origin */
    float    x1, y1;         /* line endpoints */
    float    stroke_w;
    uint32_t fill;
    uint32_t stroke;
    uint8_t  has_fill;
    uint8_t  has_stroke;
    uint8_t  closed;
    float    opacity;
    bx_vg_cmd_t cmd[BX_VG_MAX_CMDS];
    int32_t  ncmd;
} bx_vg_shape_t;

/* ---------------------------------------------------------------- document */

#define BX_VG_MAX_SHAPES 512
#define BX_VG_MAX_FRAMES 64
#define BX_VG_NAME_MAX 48

typedef struct {
    char   name[BX_VG_NAME_MAX];
    int32_t first, count;    /* which shapes this frame shows */
} bx_vg_frame_t;

typedef struct {
    char     id[BX_VG_NAME_MAX];
    char     name[BX_VG_NAME_MAX];
    float    w, h;           /* the document's own coordinate space */
    uint32_t bg;
    uint8_t  has_bg;
    bx_vg_shape_t shape[BX_VG_MAX_SHAPES];
    int32_t  nshape;
    bx_vg_frame_t frame[BX_VG_MAX_FRAMES];
    int32_t  nframe;
    int32_t  cur_frame;
    /* Frames are interpolated when a render asks for a fractional frame, so a
     * drawn animation moves rather than jumping. */
    float    anim_t;         /* 0..1 within the current frame pair */
    int32_t  from_frame, to_frame;
    float    fps;            /* >0 animates on the frame step */
    float    anim_acc;       /* time banked toward the next frame */
} bx_vg_doc_t;

/* Create/reset. Both leave an empty document with a default 24x24 viewbox,
 * which is what an icon is. */
void bx_vg_reset(bx_vg_doc_t *d);
bx_vg_doc_t *bx_vg_get(void);

/* Parse text into the document. Returns the number of shapes built, or -1
 * with a message in err. A shape that fails to parse is skipped rather than
 * losing the rest of the document: a bad line in a hand-written file should
 * not blank the file. */
long bx_vg_parse(const char *text, char *err, size_t errcap);
long bx_vg_parse_file(const char *path, char *err, size_t errcap);

/* Write the document back out. Same grammar as parse, so a round trip is a
 * fixed point. */
int  bx_vg_dump(const bx_vg_doc_t *d, char *out, size_t cap);
/* Write it as SVG. */
int  bx_vg_to_svg(const bx_vg_doc_t *d, char *out, size_t cap);
/* Write the SVG to a file. */
int  bx_vg_svg_file(const bx_vg_doc_t *d, const char *path);

/* Read SVG into the document. Covers the subset that is geometry: path, rect,
 * circle, ellipse, line, polyline, polygon, g, and fill/stroke attributes
 * including "#rgb", "none", and the common names. */
long bx_vg_from_svg(const char *text, char *err, size_t errcap);
long bx_vg_from_svg_readfile(const char *path, char *err, size_t errcap);

/* Shapes. Returns the shape's index, or -1. */
int32_t bx_vg_shape_add(bx_vg_doc_t *d, const char *name);
bx_vg_shape_t *bx_vg_shape_find(bx_vg_doc_t *d, const char *name);
int32_t bx_vg_shape_at(const bx_vg_doc_t *d, int32_t i);
int32_t bx_vg_shape_del(bx_vg_doc_t *d, int32_t i);

/* Frames. */
int32_t bx_vg_frame_add(bx_vg_doc_t *d, const char *name);
int32_t bx_vg_frame_find(const bx_vg_doc_t *d, const char *name);
int32_t bx_vg_frame_del(bx_vg_doc_t *d, int32_t i);
int     bx_vg_frame_set(bx_vg_doc_t *d, int32_t frame);
/* Move toward a frame over dt seconds, and land on it when close. Returns 1
 * while it is still moving. */
int     bx_vg_frame_to(bx_vg_doc_t *d, int32_t frame, float dt, float speed);
/* Set the frame rate the frame loop advances at. 0 stops it. The step is what
 * reads it, so a program that drives its own clock does not also need this. */
void    bx_vg_anim_fps(bx_vg_doc_t *d, float fps);
float   bx_vg_anim_get_fps(const bx_vg_doc_t *d);
/* Advance an animating document by dt. Returns the frame now showing. */
int32_t bx_vg_step(bx_vg_doc_t *d, float dt);
/* Step through frames on a timer. Returns the frame now showing. */
int32_t bx_vg_frame_anim(bx_vg_doc_t *d, float dt, float fps, int loop);

/* Append one command to a path shape. Public because ui.vg add|...|poly builds
 * a path the same way the parser does, rather than through a second code path. */
void bx_vg_path_cmd(bx_vg_shape_t *s, uint8_t op, const float *v, int nv);
/* Fill a shape's command list from an SVG "d" string. Returns the count. */
int  bx_vg_parse_path_d(bx_vg_shape_t *s, const char *d);

/* Render the current frame into fb, fitted into the rect with `fit`:
 *   BX_VG_FIT_CONTAIN  scale to fit inside, keep the aspect, centre it
 *   BX_VG_FIT_COVER   scale to cover, crop the overflow
 *   BX_VG_FIT_NONE    use the rect as-is, stretching
 */
typedef enum {
    BX_VG_FIT_CONTAIN = 0,
    BX_VG_FIT_COVER,
    BX_VG_FIT_NONE
} bx_vg_fit_t;

int bx_vg_render(bx_vg_fb_t *fb, const bx_vg_doc_t *d,
                 int32_t x, int32_t y, int32_t w, int32_t h, bx_vg_fit_t fit);
int bx_vg_render_svg(const bx_vg_doc_t *d, char *out, size_t cap);

#endif
