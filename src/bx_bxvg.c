/* BX VG - implementation. See bx_bxvg.h for what this is.
 *
 * Rasterization goes through the GFX primitives rather than a second scanline
 * engine. A path is flattened to polygons and filled by bx_gfx_poly, which
 * means paths get the framebuffer's clipping, transform stack and blending for
 * free, and there is only one place in the tree that has to be right about
 * edges. The cost is that curves are flattened at a fixed subdivision, which
 * is fine at icon sizes and is the same trade every small rasterizer makes.
 */
#include "bx_bxvg.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static bx_vg_doc_t g_vg;

bx_vg_doc_t *bx_vg_get(void) { return &g_vg; }

void bx_vg_reset(bx_vg_doc_t *d) {
    if (!d) return;
    memset(d, 0, sizeof *d);
    /* An icon's natural size, and Lucide's too, so a document that never sets
     * one still scales sanely. */
    d->w = 24.0f;
    d->h = 24.0f;
}

/* --------------------------------------------------------------- shapes */

int32_t bx_vg_shape_add(bx_vg_doc_t *d, const char *name) {
    if (!d) return -1;
    if (d->nshape >= BX_VG_MAX_SHAPES) return -1;
    bx_vg_shape_t *s = &d->shape[d->nshape];
    memset(s, 0, sizeof *s);
    s->id = d->nshape + 1;
    snprintf(s->name, sizeof s->name, "%s", name ? name : "");
    s->stroke_w = 2.0f;
    s->opacity = 1.0f;
    s->fill = 0;
    s->has_fill = 0;
    s->has_stroke = 1;
    s->stroke = 0xffffffffu;
    d->nshape++;
    return d->nshape - 1;
}

/* Remove the shape at `i`, closing the gap. Only the tail is shuffled: a
 * shape that fails to be added should not leave a hole behind it. Ids are
 * positional, so the last shape's id changes when anything is removed. */
int32_t bx_vg_shape_del(bx_vg_doc_t *d, int32_t i) {
    if (!d || i < 0 || i >= d->nshape) return -1;
    for (int32_t k = i; k + 1 < d->nshape; k++) d->shape[k] = d->shape[k + 1];
    d->nshape--;
    memset(&d->shape[d->nshape], 0, sizeof d->shape[0]);
    return i;
}

bx_vg_shape_t *bx_vg_shape_find(bx_vg_doc_t *d, const char *name) {
    if (!d || !name) return NULL;
    for (int32_t i = 0; i < d->nshape; i++)
        if (!strcmp(d->shape[i].name, name)) return &d->shape[i];
    return NULL;
}

int32_t bx_vg_shape_at(const bx_vg_doc_t *d, int32_t i) {
    if (!d || i < 0 || i >= d->nshape) return -1;
    return i;
}

/* ---------------------------------------------------------------- frames */

int32_t bx_vg_frame_add(bx_vg_doc_t *d, const char *name) {
    if (!d) return -1;
    if (d->nframe >= BX_VG_MAX_FRAMES) return -1;
    bx_vg_frame_t *f = &d->frame[d->nframe];
    memset(f, 0, sizeof *f);
    snprintf(f->name, sizeof f->name, "%s", name ? name : "");
    f->first = 0;
    f->count = d->nshape;      /* a new frame starts showing everything */
    d->nframe++;
    if (d->nframe == 1) d->cur_frame = 0;
    return d->nframe - 1;
}

int32_t bx_vg_frame_find(const bx_vg_doc_t *d, const char *name) {
    if (!d || !name) return -1;
    for (int32_t i = 0; i < d->nframe; i++)
        if (!strcmp(d->frame[i].name, name)) return i;
    return -1;
}

/* Drop the frame at `i`, closing the gap. Used when a frame add is asked for
 * a shape range the document does not have, so a bad request does not leave a
 * half-built frame behind. */
int32_t bx_vg_frame_del(bx_vg_doc_t *d, int32_t i) {
    if (!d || i < 0 || i >= d->nframe) return -1;
    for (int32_t k = i; k + 1 < d->nframe; k++) d->frame[k] = d->frame[k + 1];
    d->nframe--;
    memset(&d->frame[d->nframe], 0, sizeof d->frame[0]);
    if (d->cur_frame >= d->nframe) d->cur_frame = d->nframe > 0 ? d->nframe - 1 : 0;
    return i;
}

int bx_vg_frame_set(bx_vg_doc_t *d, int32_t frame) {
    if (!d || frame < 0 || frame >= d->nframe) return -1;
    d->cur_frame = frame;
    d->from_frame = frame;
    d->to_frame = frame;
    d->anim_t = 0.0f;
    return 0;
}

int bx_vg_frame_to(bx_vg_doc_t *d, int32_t frame, float dt, float speed) {
    if (!d || frame < 0 || frame >= d->nframe) return 0;
    if (speed <= 0.0f) speed = 1.0f;
    if (d->to_frame != frame || d->from_frame == d->to_frame) {
        /* Starting a new move: freeze where we are, then head for the new
         * one. Starting from cur_frame instead would snap on every change. */
        d->from_frame = d->nframe ? d->cur_frame : frame;
        d->to_frame = frame;
        d->anim_t = 0.0f;
        if (d->from_frame == frame) { d->cur_frame = frame; return 0; }
    }
    d->anim_t += dt * speed;
    if (d->anim_t >= 1.0f) {
        d->anim_t = 1.0f;
        d->cur_frame = frame;
        d->from_frame = frame;
        return 0;
    }
    /* The interpolated frame: a blend index, stored in cur_frame as a fixed
     * point value so the renderer can use it without another field. */
    d->cur_frame = d->from_frame;
    return 1;
}

void bx_vg_anim_fps(bx_vg_doc_t *d, float fps) {
    if (!d) return;
    d->fps = fps;
    d->anim_acc = 0.0f;
}

float bx_vg_anim_get_fps(const bx_vg_doc_t *d) { return d ? d->fps : 0.0f; }

/* The banked time lives in the document, not in a static, so two documents
 * cannot step each other and a reset really resets. */
int32_t bx_vg_step(bx_vg_doc_t *d, float dt) {
    if (!d) return 0;
    return bx_vg_frame_anim(d, dt, d->fps > 0 ? d->fps : 12.0f, 1);
}

int32_t bx_vg_frame_anim(bx_vg_doc_t *d, float dt, float fps, int loop) {
    if (!d || d->nframe <= 1) return d ? d->cur_frame : 0;
    if (fps <= 0.0f) fps = 12.0f;
    float *accp = &d->anim_acc;
    *accp += dt;
    float step = 1.0f / fps;
    while (*accp >= step) {
        *accp -= step;
        int32_t next = d->cur_frame + 1;
        if (next >= d->nframe) next = loop ? 0 : d->nframe - 1;
        d->cur_frame = next;
        d->from_frame = d->to_frame = next;
        d->anim_t = 0.0f;
    }
    return d->cur_frame;
}

/* ------------------------------------------------------------ flattening */

/* A flattened path: a pile of polygons, one per subpath. A path with two
 * disjoint pieces is two polygons, which bx_gfx_poly fills independently. */
#define BX_VG_MAX_POLY 512
#define BX_VG_MAX_PTS 256

typedef struct {
    float pts[BX_VG_MAX_PTS * 2];
    int   n;
} vg_poly_t;

typedef struct {
    vg_poly_t poly[BX_VG_MAX_POLY];
    int n;
} vg_path_t;

/* pp->n counts the polygons, so the one being filled is the last one. */
static void vg_poly_push(vg_path_t *pp, float x, float y) {
    if (pp->n <= 0 || pp->n > BX_VG_MAX_POLY) return;
    vg_poly_t *p = &pp->poly[pp->n - 1];
    if (p->n >= BX_VG_MAX_PTS) return;
    p->pts[p->n * 2] = x;
    p->pts[p->n * 2 + 1] = y;
    p->n++;
}

static void vg_poly_new(vg_path_t *pp) {
    if (pp->n >= BX_VG_MAX_POLY) return;
    pp->poly[pp->n].n = 0;
    pp->n++;
}

/* Curves become segments. 16 is enough that an icon's curve does not show
 * facets at 4x, and cheap enough to redo every frame. */
#define BX_VG_CURVE_STEPS 16

static void vg_cubic(vg_path_t *pp, float x0, float y0, float x1, float y1,
                     float x2, float y2, float x3, float y3) {
    for (int i = 1; i <= BX_VG_CURVE_STEPS; i++) {
        float t = (float)i / (float)BX_VG_CURVE_STEPS;
        float u = 1.0f - t;
        float a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, dd = t * t * t;
        vg_poly_push(pp, a * x0 + b * x1 + c * x2 + dd * x3,
                        a * y0 + b * y1 + c * y2 + dd * y3);
    }
}

/* A shape becomes a path. Rect, circle, ellipse, poly and line all go through
 * here so there is one fill path in the file rather than five. */
static void vg_flatten(const bx_vg_shape_t *shape, vg_path_t *pp) {
    /* closed is discovered while flattening (an arc is closed by being an
     * arc), and the shape belongs to the caller, so this works on a copy
     * rather than marking the document. */
    bx_vg_shape_t s = *shape;
    const bx_vg_shape_t *keep = shape;
    (void)keep;
    pp->n = 0;
    if (s.type == BX_VG_PATH || s.type == BX_VG_TEXT) {
        if (s.ncmd == 0) return;
        vg_poly_new(pp);
        float cx = 0, cy = 0;
        for (int32_t i = 0; i < s.ncmd; i++) {
            const bx_vg_cmd_t *c = &s.cmd[i];
            switch (c->op) {
            case BX_VG_OP_MOVE:
                /* A move with no polygon yet starts one; a move in the middle
                 * of a shape starts the next subpath, which is how a path
                 * with two holes or two pieces behaves. */
                if (pp->poly[pp->n - 1].n > 0) vg_poly_new(pp);
                cx = c->v[0]; cy = c->v[1];
                vg_poly_push(pp, cx, cy);
                break;
            case BX_VG_OP_LINE:
                cx = c->v[0]; cy = c->v[1];
                vg_poly_push(pp, cx, cy);
                break;
            case BX_VG_OP_CURVE: {
                float nx = c->v[4], ny = c->v[5];
                vg_cubic(pp, cx, cy, c->v[0], c->v[1], c->v[2], c->v[3], nx, ny);
                cx = nx; cy = ny;
                break;
            }
            case BX_VG_OP_ARC: {
                /* An arc as an elliptical sweep, which is all the icon work
                 * needs and is a lot less code than the SVG endpoint form. */
                float ccx = c->v[0], ccy = c->v[1], rx = c->v[2], ry = c->v[3];
                float a0 = c->v[4] * (float)(M_PI / 180.0);
                float a1 = c->v[5] * (float)(M_PI / 180.0);
                int steps = BX_VG_CURVE_STEPS;
                for (int i = 0; i <= steps; i++) {
                    float t = a0 + (a1 - a0) * (float)i / (float)steps;
                    vg_poly_push(pp, ccx + rx * (float)cos(t), ccy + ry * (float)sin(t));
                }
                s.closed = 1;
                break;
            }
            case BX_VG_OP_CLOSE:
                s.closed = 1;
                break;
            }
        }
        return;
    }
    if (s.type == BX_VG_RECT) {
        float x = s.x, y = s.y, w = s.w, h = s.h;
        if (s.stroke_w > 1.0f) {
            /* A stroked rect is drawn at half the stroke inside its box, the
             * way every SVG renderer does it. */
            float o = s.stroke_w / 2.0f;
            x += o; y += o; w -= s.stroke_w; h -= s.stroke_w;
        }
        if (w <= 0 || h <= 0) return;
        vg_poly_new(pp);
        vg_poly_push(pp, x, y);
        vg_poly_push(pp, x + w, y);
        vg_poly_push(pp, x + w, y + h);
        vg_poly_push(pp, x, y + h);
        return;
    }
    if (s.type == BX_VG_CIRCLE || s.type == BX_VG_ELLIPSE) {
        float cx = s.x + s.w / 2.0f, cy = s.y + s.h / 2.0f;
        float rx = s.w / 2.0f, ry = s.h / 2.0f;
        if (rx <= 0 || ry <= 0) return;
        if (s.type == BX_VG_CIRCLE && rx != ry) { ry = rx; }
        if (s.has_stroke && s.stroke_w > 1.0f) { rx -= s.stroke_w / 2.0f; ry -= s.stroke_w / 2.0f; }
        if (rx <= 0 || ry <= 0) return;
        int steps = BX_VG_CURVE_STEPS * 2;
            vg_poly_new(pp);
        for (int i = 0; i < steps; i++) {
            float t = 2.0f * (float)M_PI * (float)i / (float)steps;
            vg_poly_push(pp, cx + rx * (float)cos(t), cy + ry * (float)sin(t));
        }
        return;
    }
    if (s.type == BX_VG_POLY) {
        int n = s.ncmd;
        if (n < 2) return;
        vg_poly_new(pp);
        for (int i = 0; i < n; i++) vg_poly_push(pp, s.cmd[i].v[0], s.cmd[i].v[1]);
        return;
    }
    if (s.type == BX_VG_LINE) {
        /* A line has no area, so it is stroked as a thick polygon. That keeps
         * one fill path: no second stroke renderer to keep in sync. */
        float hw = s.stroke_w > 0 ? s.stroke_w / 2.0f : 0.5f;
        float dx = s.x1 - s.x, dy = s.y1 - s.y;
        float len = (float)sqrt(dx * dx + dy * dy);
        if (len < 0.0001f) return;
        float nx = -dy / len * hw, ny = dx / len * hw;
        vg_poly_new(pp);
        vg_poly_push(pp, s.x + nx, s.y + ny);
        vg_poly_push(pp, s.x1 + nx, s.y1 + ny);
        vg_poly_push(pp, s.x1 - nx, s.y1 - ny);
        vg_poly_push(pp, s.x - nx, s.y - ny);
        return;
    }
}

/* Triangulate a polygon and draw it.
 *
 * A fan from the first vertex, which is what bx_gfx_poly does, is only correct
 * for a convex polygon, and the first thing anyone draws with this is a circle,
 * which is not convex once it is more than a few points. Ear clipping is the
 * small fix: O(n^2) on the ~32 points a flattened curve produces, which is
 * nothing, and it handles the concave shapes a path makes too.
 */
static int vg_tri_area2(const float *p, int a, int b, int c) {
    return (int)((p[b*2]-p[a*2]) * (p[c*2+1]-p[a*2+1]) -
                 (p[c*2]-p[a*2]) * (p[b*2+1]-p[a*2+1]));
}

static int vg_point_in_tri(const float *p, int a, int b, int c, float x, float y) {
    float d1 = (p[b*2]-p[a*2])*(y-p[a*2+1]) - (p[b*2+1]-p[a*2+1])*(x-p[a*2]);
    float d2 = (p[c*2]-p[b*2])*(y-p[b*2+1]) - (p[c*2+1]-p[b*2+1])*(x-p[b*2]);
    float d3 = (p[a*2]-p[c*2])*(y-p[c*2+1]) - (p[a*2+1]-p[c*2+1])*(x-p[c*2]);
    int neg = (d1<0)||(d2<0)||(d3<0);
    int pos = (d1>0)||(d2>0)||(d3>0);
    return !(neg && pos);
}

/* Draw one quad as two triangles. A stroke segment is always convex, so this
 * does not need the ear clipper. */
static void vg_draw_quad(bx_gfx_fb_t *fb, const float *q, uint32_t c) {
    bx_gfx_tri(fb, q[0], q[1], q[2], q[3], q[4], q[5], c);
    bx_gfx_tri(fb, q[0], q[1], q[4], q[5], q[6], q[7], c);
}

static void vg_fill_poly(bx_gfx_fb_t *fb, const float *p, int n, uint32_t c) {
    if (n < 3) return;
    if (n == 3) { bx_gfx_tri(fb, p[0], p[1], p[2], p[3], p[4], p[5], c); return; }

    /* Ear clipping. Keep the index list so the points themselves are not
     * rewritten, and stop when what is left cannot be clipped further. */
    int idx[BX_VG_MAX_PTS];
    for (int i = 0; i < n; i++) idx[i] = i;
    int left = n;
    int guard = n * n + 8;      /* a clip that finds no ear must still end */
    while (left > 3 && guard-- > 0) {
        int clipped = 0;
        for (int i = 0; i < left; i++) {
            int ia = idx[(i + left - 1) % left];
            int ib = idx[i];
            int ic = idx[(i + 1) % left];
            /* Convex only: a reflex vertex is not an ear. */
            if (vg_tri_area2(p, ia, ib, ic) <= 0) continue;
            int ok = 1;
            for (int j = 0; j < left; j++) {
                int q = idx[j];
                if (q == ia || q == ib || q == ic) continue;
                if (vg_point_in_tri(p, ia, ib, ic, p[q*2], p[q*2+1])) { ok = 0; break; }
            }
            if (!ok) continue;
            bx_gfx_tri(fb, p[ia*2], p[ia*2+1], p[ib*2], p[ib*2+1],
                       p[ic*2], p[ic*2+1], c);
            for (int j = i; j < left - 1; j++) idx[j] = idx[j + 1];
            left--;
            clipped = 1;
            break;
        }
        /* Nothing left to clip: a self-intersecting polygon, or one that is
         * wound so that no vertex is convex. Fan the rest, which at least
         * covers most of it rather than nothing. */
        if (!clipped) break;
    }
    if (left >= 3) {
        for (int i = 1; i + 1 < left; i++) {
            int a = idx[0], b = idx[i], c = idx[i + 1];
            bx_gfx_tri(fb, p[a*2], p[a*2+1], p[b*2], p[b*2+1], p[c*2], p[c*2+1], c);
        }
    }
}

/* -------------------------------------------------------------- rendering */

static uint32_t vg_apply_opacity(uint32_t c, float op) {
    if (op >= 0.999f) return c;
    if (op <= 0.0f) return 0;
    int a = (int)((float)BX_GFX_A(c) * op);
    if (a < 0) a = 0;
    if (a > 255) a = 255;
    return (c & 0xffffff00u) | (uint32_t)a;
}

static void vg_draw_shape(bx_gfx_fb_t *fb, const bx_vg_shape_t *s,
                          float ox, float oy, float sx, float sy, int want_fill) {
    vg_path_t path;
    vg_flatten(s, &path);
    if (path.n == 0) return;

    /* Transform into device space once, then draw. Doing it here rather than
     * inside the polygon keeps the transform stack out of the hot path. */
    for (int p = 0; p < path.n; p++) {
        vg_poly_t *poly = &path.poly[p];
        if (poly->n < 2) continue;
        float dev[BX_VG_MAX_PTS * 2];
        for (int i = 0; i < poly->n; i++) {
            dev[i * 2]     = ox + poly->pts[i * 2] * sx;
            dev[i * 2 + 1] = oy + poly->pts[i * 2 + 1] * sy;
        }
        if (want_fill && s->has_fill)
            vg_fill_poly(fb, dev, poly->n, vg_apply_opacity(s->fill, s->opacity));
        if (s->has_stroke) {
            /* Stroke: draw each segment as its own quad. Not a stroker, but
             * it handles joins by overlapping, which at these sizes looks the
             * same and costs nothing to keep. */
            float w = s->stroke_w * ((sx + sy) / 2.0f);
            if (w < 0.6f) w = 0.6f;
            uint32_t sc = vg_apply_opacity(s->stroke, s->opacity);
            int closed = s->closed || s->type == BX_VG_RECT ||
                         s->type == BX_VG_CIRCLE || s->type == BX_VG_ELLIPSE;
            int last = closed ? poly->n : poly->n - 1;
            for (int i = 0; i < last; i++) {
                int j = (i + 1) % poly->n;
                float x0 = dev[i * 2], y0 = dev[i * 2 + 1];
                float x1 = dev[j * 2], y1 = dev[j * 2 + 1];
                float dx = x1 - x0, dy = y1 - y0;
                float len = (float)sqrt(dx * dx + dy * dy);
                if (len < 0.0001f) continue;
                float hw = w / 2.0f;
                float nx = -dy / len * hw, ny = dx / len * hw;
                float q[8];
                q[0] = x0 + nx; q[1] = y0 + ny;
                q[2] = x1 + nx; q[3] = y1 + ny;
                q[4] = x1 - nx; q[5] = y1 - ny;
                q[6] = x0 - nx; q[7] = y0 - ny;
                vg_draw_quad(fb, q, sc);
            }
        }
    }
}

int bx_vg_render(bx_gfx_fb_t *fb, const bx_vg_doc_t *d,
                 int32_t x, int32_t y, int32_t w, int32_t h, bx_vg_fit_t fit) {
    if (!fb || !d || w <= 0 || h <= 0) return 0;
    float dw = d->w > 0 ? d->w : 24.0f;
    float dh = d->h > 0 ? d->h : 24.0f;
    float sx = (float)w / dw, sy = (float)h / dh;
    float ox = (float)x, oy = (float)y;
    if (fit != BX_VG_FIT_NONE) {
        /* One scale for both axes, or a circle comes out an ellipse. */
        float s = fit == BX_VG_FIT_COVER
                    ? (sx > sy ? sx : sy)
                    : (sx < sy ? sx : sy);
        sx = sy = s;
        if (fit == BX_VG_FIT_CONTAIN) {
            ox = (float)x + ((float)w - dw * s) / 2.0f;
            oy = (float)y + ((float)h - dh * s) / 2.0f;
        }
    }

    if (d->has_bg) {
        /* The background covers the fitted document box, not the whole rect,
         * so a contain-fit icon keeps its padding. */
        bx_gfx_rect(fb, (int)ox, (int)oy, (int)(dw * sx), (int)(dh * sy), d->bg);
    }

    const bx_vg_frame_t *f = NULL;
    if (d->nframe > 0 && d->cur_frame >= 0 && d->cur_frame < d->nframe)
        f = &d->frame[d->cur_frame];

    int drawn = 0;
    int first = f ? f->first : 0;
    int count = f ? f->count : d->nshape;
    for (int i = first; i < first + count && i < d->nshape; i++) {
        const bx_vg_shape_t *s = &d->shape[i];
        int want_fill = s->has_fill || s->type == BX_VG_PATH ||
                        s->type == BX_VG_RECT || s->type == BX_VG_POLY;
        vg_draw_shape(fb, s, ox, oy, sx, sy, want_fill);
        drawn++;
    }
    return drawn;
}

/* --------------------------------------------------------------- parsing */

/* The grammar, in one place, so the parser and the dumper cannot disagree:
 *
 *   doc W H [bg #hex] [name ID]
 *   shape NAME type geometry style...
 *     rect    x y w h
 *     circle  cx cy r
 *     ellipse cx cy rx ry
 *     line    x1 y1 x2 y2
 *     poly    x1 y1 x2 y2 ...
 *     path    M x y L x y C x1 y1 x2 y2 x y Z ...
 *   style    fill|#hex   stroke|#hex  w|N  opacity|N  closed|0|1
 *   frame NAME [from count]
 *
 * Everything is whitespace separated and '#' starts a comment to end of line.
 * Numbers are plain decimals; a command letter may be attached to its first
 * number, as SVG writes it, because that is how paths are written everywhere.
 */

typedef struct {
    const char *p;
    char       *err;
    size_t      errcap;
    const char *start;   /* the first character, so column 0 counts as a line start */
} vg_lexer_t;

static void vg_skip(vg_lexer_t *lx) {
    /* Only a '#' in the first column is a comment. A '#' further in is a
     * colour, and colours are the common case here, so treating every '#' as a
     * comment silently threw away every fill. */
    int fresh_line = (lx->p == lx->start);
    for (;;) {
        while (*lx->p == '\n') { lx->p++; fresh_line = 1; }
        while (*lx->p == ' ' || *lx->p == '\t' || *lx->p == '\r') lx->p++;
        if (fresh_line && *lx->p == '#') { while (*lx->p && *lx->p != '\n') lx->p++; continue; }
        break;
    }
}

static int vg_word(vg_lexer_t *lx, char *out, size_t cap) {
    vg_skip(lx);
    if (!*lx->p) return 0;
    size_t i = 0;
    while (*lx->p && *lx->p != ' ' && *lx->p != '\t' && *lx->p != '\r' && *lx->p != '\n') {
        if (i + 1 < cap) out[i++] = *lx->p;
        lx->p++;
    }
    out[i] = 0;
    return 1;
}

/* A number, with an optional command letter in front of it. SVG writes
 * "L10-20" and bxvg accepts it, because hand-editing a path should not require
 * remembering which letters take a space. */
static int vg_num(vg_lexer_t *lx, float *out, char *cmd) {
    vg_skip(lx);
    char *begin = (char *)lx->p;      /* before any letter, for a clean undo */
    char c = 0;
    if ((*lx->p >= 'A' && *lx->p <= 'Z') || (*lx->p >= 'a' && *lx->p <= 'z')) {
        c = *lx->p;
        lx->p++;
    }
    char *start = (char *)lx->p;
    char *end = NULL;
    double v = strtod(start, &end);
    /* A word where a number was expected is not an error, it is the end of
     * this run of numbers. Undo the letter too, or "fill" would come back as
     * "ill" and silently never match anything. */
    if (!end || end == start) { lx->p = begin; if (cmd) *cmd = 0; return 0; }
    lx->p = end;
    if (out) *out = (float)v;
    if (cmd) *cmd = c;
    return 1;
}

static int vg_keyword(vg_lexer_t *lx, const char *kw) {
    vg_skip(lx);
    size_t n = strlen(kw);
    if (strncmp(lx->p, kw, n) != 0) return 0;
    char after = lx->p[n];
    if (after && after != ' ' && after != '\t' && after != '\r' && after != '\n') return 0;
    lx->p += n;
    return 1;
}

void bx_vg_path_cmd(bx_vg_shape_t *s, uint8_t op, const float *v, int nv);

static void vg_err(char *err, size_t cap, const char *msg) {
    if (err && cap) snprintf(err, cap, "%s", msg);
}

/* Style keywords apply to the shape being read. */
static void vg_style(bx_vg_shape_t *s, const char *key, const char *val) {
    if (!strcmp(key, "fill")) {
        if (!strcmp(val, "none") || !strcmp(val, "0")) s->has_fill = 0;
        else { s->fill = bx_gfx_parse_color(val); s->has_fill = 1; }
    } else if (!strcmp(key, "stroke")) {
        if (!strcmp(val, "none") || !strcmp(val, "0")) s->has_stroke = 0;
        else { s->stroke = bx_gfx_parse_color(val); s->has_stroke = 1; }
    } else if (!strcmp(key, "w") || !strcmp(key, "width")) {
        s->stroke_w = (float)atof(val);
    } else if (!strcmp(key, "opacity")) {
        s->opacity = (float)atof(val);
    } else if (!strcmp(key, "closed")) {
        s->closed = (char)(atoi(val) != 0);
    }
}

void bx_vg_path_cmd(bx_vg_shape_t *s, uint8_t op, const float *v, int nv) {
    if (s->ncmd >= BX_VG_MAX_CMDS) return;
    bx_vg_cmd_t *c = &s->cmd[s->ncmd++];
    c->op = op;
    for (int i = 0; i < nv && i < 6; i++) c->v[i] = v[i];
}

long bx_vg_parse(const char *text, char *err, size_t errcap) {
    if (!text) { vg_err(err, errcap, "no text"); return -1; }
    bx_vg_doc_t *d = &g_vg;
    bx_vg_reset(d);
    vg_lexer_t lx = { text, err, errcap, text };
    char w[64];

    for (;;) {
        vg_skip(&lx);
        if (!*lx.p) break;
        if (vg_word(&lx, w, sizeof w)) {
            if (!strcmp(w, "doc")) {
                float v;
                char cmd;
                vg_num(&lx, &v, &cmd);
                if (v > 0) d->w = v;
                vg_num(&lx, &v, &cmd);
                if (v > 0) d->h = v;
                continue;
            }
            if (!strcmp(w, "bg")) {
                if (vg_word(&lx, w, sizeof w)) {
                    d->bg = bx_gfx_parse_color(w);
                    d->has_bg = 1;
                }
                continue;
            }
            if (!strcmp(w, "name")) {
                if (vg_word(&lx, w, sizeof w)) snprintf(d->name, sizeof d->name, "%s", w);
                continue;
            }
            if (!strcmp(w, "frame")) {
                char nm[64];
                if (!vg_word(&lx, nm, sizeof nm)) continue;
                int32_t fi = bx_vg_frame_add(d, nm);
                float v; char cmd;
                if (vg_num(&lx, &v, &cmd)) d->frame[fi].first = (int32_t)v;
                if (vg_num(&lx, &v, &cmd)) d->frame[fi].count = (int32_t)v;
                continue;
            }
            if (!strcmp(w, "shape")) {
                char nm[64];
                if (!vg_word(&lx, nm, sizeof nm)) continue;
                char type[32];
                if (!vg_word(&lx, type, sizeof type)) continue;
                int32_t si = bx_vg_shape_add(d, nm);
                if (si < 0) continue;
                bx_vg_shape_t *s = &d->shape[si];

                if (!strcmp(type, "rect") || !strcmp(type, "circle") ||
                    !strcmp(type, "ellipse")) {
                    float a, b, c = 0, e = 0; char cmd;
                    vg_num(&lx, &a, &cmd);
                    vg_num(&lx, &b, &cmd);
                    if (!strcmp(type, "circle")) {
                        vg_num(&lx, &c, &cmd);
                        s->type = BX_VG_CIRCLE;
                        s->x = a - c; s->y = b - c; s->w = c * 2; s->h = c * 2;
                    } else if (!strcmp(type, "ellipse")) {
                        vg_num(&lx, &c, &cmd);
                        vg_num(&lx, &e, &cmd);
                        s->type = BX_VG_ELLIPSE;
                        s->x = a - c; s->y = b - c; s->w = c * 2; s->h = e * 2;
                    } else {
                        vg_num(&lx, &c, &cmd);
                        vg_num(&lx, &e, &cmd);
                        s->type = BX_VG_RECT;
                        s->x = a; s->y = b; s->w = c; s->h = e;
                    }
                } else if (!strcmp(type, "line")) {
                    float v[4]; char cmd;
                    for (int i = 0; i < 4; i++) vg_num(&lx, &v[i], &cmd);
                    s->type = BX_VG_LINE;
                    s->x = v[0]; s->y = v[1];
                    s->x1 = v[2]; s->y1 = v[3];
                } else if (!strcmp(type, "poly")) {
                    s->type = BX_VG_POLY;
                    float v[2]; char cmd;
                    while (vg_num(&lx, &v[0], &cmd)) {
                        if (!vg_num(&lx, &v[1], &cmd)) break;
                        float pair[2] = { v[0], v[1] };
                        bx_vg_path_cmd(s, BX_VG_OP_LINE, pair, 2);
                    }
                } else if (!strcmp(type, "path")) {
                    s->type = BX_VG_PATH;
                    float v[6]; char cmd;
                    for (;;) {
                        if (!vg_num(&lx, &v[0], &cmd)) break;
                        char op = cmd ? cmd : 'L';    /* a bare number continues a line */
                        if (op == 'M' || op == 'm') {
                            vg_num(&lx, &v[1], NULL);
                            float pair[2] = { v[0], v[1] };
                            bx_vg_path_cmd(s, BX_VG_OP_MOVE, pair, 2);
                        } else if (op == 'L' || op == 'l' || op == 'H' || op == 'V') {
                            vg_num(&lx, &v[1], NULL);
                            float pair[2] = { v[0], v[1] };
                            bx_vg_path_cmd(s, BX_VG_OP_LINE, pair, 2);
                        } else if (op == 'C' || op == 'c') {
                            for (int i = 2; i < 6; i++) vg_num(&lx, &v[i], NULL);
                            bx_vg_path_cmd(s, BX_VG_OP_CURVE, v, 6);
                        } else if (op == 'Z' || op == 'z') {
                            bx_vg_path_cmd(s, BX_VG_OP_CLOSE, NULL, 0);
                        } else {
                            /* A letter this grammar does not know: stop the
                             * path rather than the document. */
                            break;
                        }
                    }
                } else {
                    continue;    /* unknown shape type: skip its line */
                }
                /* Trailing style keywords, up to the next shape or frame. */
                for (;;) {
                    vg_skip(&lx);
                    if (!*lx.p) break;
                    if (*lx.p == 's' && !strncmp(lx.p, "shape", 5)) break;
                    if (*lx.p == 'f' && !strncmp(lx.p, "frame", 5)) break;
                    char key[32], val[64];
                    if (!vg_word(&lx, key, sizeof key)) break;
                    if (!vg_word(&lx, val, sizeof val)) break;
                    vg_style(s, key, val);
                }
                continue;
            }
            /* An unrecognised top-level word: skip it. A hand-written file
             * should not blank because of one stray line. */
        }
    }

    /* If the document declared frames, show the first. */
    if (d->nframe > 0) bx_vg_frame_set(d, 0);
    return d->nshape;
}

long bx_vg_parse_file(const char *path, char *err, size_t errcap) {
    if (!path) { vg_err(err, errcap, "no path"); return -1; }
    FILE *f = fopen(path, "rb");
    if (!f) {
        char m[256];
        snprintf(m, sizeof m, "cannot read %s", path);
        vg_err(err, errcap, m);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0 || sz > (1 << 22)) { fclose(f); vg_err(err, errcap, "file too large"); return -1; }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); vg_err(err, errcap, "out of memory"); return -1; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    buf[got] = 0;
    fclose(f);
    long n = bx_vg_parse(buf, err, errcap);
    free(buf);
    return n;
}

/* ------------------------------------------------------------------ dump */

static size_t vg_put(char *out, size_t cap, size_t n, const char *fmt, ...) {
    if (n >= cap) return n;
    va_list ap;
    va_start(ap, fmt);
    int w = vsnprintf(out + n, cap - n, fmt, ap);
    va_end(ap);
    if (w < 0) return n;
    return n + (size_t)w;
}

static void vg_hex(char *out, size_t cap, uint32_t c) {
    snprintf(out, cap, "#%02x%02x%02x", BX_GFX_R(c), BX_GFX_G(c), BX_GFX_B(c));
}

int bx_vg_dump(const bx_vg_doc_t *d, char *out, size_t cap) {
    if (!d || !out || cap == 0) return -1;
    size_t n = 0;
    char col[16];
    n = vg_put(out, cap, n, "doc %g %g", d->w, d->h);
    if (d->has_bg) { vg_hex(col, sizeof col, d->bg); n = vg_put(out, cap, n, " bg %s", col); }
    if (d->name[0]) n = vg_put(out, cap, n, " name %s", d->name);
    n = vg_put(out, cap, n, "\n");
    for (int32_t i = 0; i < d->nshape; i++) {
        const bx_vg_shape_t *s = &d->shape[i];
        n = vg_put(out, cap, n, "shape %s ", s->name[0] ? s->name : "s");
        switch (s->type) {
        case BX_VG_RECT:
            n = vg_put(out, cap, n, "rect %g %g %g %g\n", s->x, s->y, s->w, s->h);
            break;
        case BX_VG_CIRCLE:
            n = vg_put(out, cap, n, "circle %g %g %g\n",
                      s->x + s->w / 2, s->y + s->h / 2, s->w / 2);
            break;
        case BX_VG_ELLIPSE:
            n = vg_put(out, cap, n, "ellipse %g %g %g %g\n",
                      s->x + s->w / 2, s->y + s->h / 2, s->w / 2, s->h / 2);
            break;
        case BX_VG_LINE:
            n = vg_put(out, cap, n, "line %g %g %g %g\n", s->x, s->y, s->x1, s->y1);
            break;
        case BX_VG_POLY:
            n = vg_put(out, cap, n, "poly");
            for (int32_t k = 0; k < s->ncmd; k++)
                n = vg_put(out, cap, n, " %g %g", s->cmd[k].v[0], s->cmd[k].v[1]);
            n = vg_put(out, cap, n, "\n");
            break;
        case BX_VG_PATH:
            n = vg_put(out, cap, n, "path");
            for (int32_t k = 0; k < s->ncmd; k++) {
                const bx_vg_cmd_t *c = &s->cmd[k];
                switch (c->op) {
                case BX_VG_OP_MOVE:  n = vg_put(out, cap, n, " M %g %g", c->v[0], c->v[1]); break;
                case BX_VG_OP_LINE:  n = vg_put(out, cap, n, " L %g %g", c->v[0], c->v[1]); break;
                case BX_VG_OP_CURVE: n = vg_put(out, cap, n, " C %g %g %g %g %g %g",
                                c->v[0], c->v[1], c->v[2], c->v[3], c->v[4], c->v[5]); break;
                case BX_VG_OP_ARC:   n = vg_put(out, cap, n, " A %g %g %g %g %g %g",
                                c->v[0], c->v[1], c->v[2], c->v[3], c->v[4], c->v[5]); break;
                case BX_VG_OP_CLOSE: n = vg_put(out, cap, n, " Z"); break;
                }
            }
            n = vg_put(out, cap, n, "\n");
            break;
        default:
            n = vg_put(out, cap, n, "rect 0 0 0 0\n");
            break;
        }
        if (s->has_fill) { vg_hex(col, sizeof col, s->fill); n = vg_put(out, cap, n, " fill %s", col); }
        if (s->has_stroke) { vg_hex(col, sizeof col, s->stroke); n = vg_put(out, cap, n, " stroke %s", col); }
        n = vg_put(out, cap, n, " w %g", s->stroke_w);
        if (s->opacity < 0.999f) n = vg_put(out, cap, n, " opacity %g", s->opacity);
        if (s->closed) n = vg_put(out, cap, n, " closed 1");
        n = vg_put(out, cap, n, "\n");
    }
    for (int32_t i = 0; i < d->nframe; i++)
        n = vg_put(out, cap, n, "frame %s %d %d\n", d->frame[i].name, d->frame[i].first, d->frame[i].count);
    if (n < cap) out[n] = 0;
    else if (cap) out[cap - 1] = 0;
    return (int)(n < cap ? n : cap - 1);
}

/* Read an SVG "d" string into a shape's command list.
 *
 * This is the same reader the bxvg parser uses for a path, which is why an
 * SVG loads without a second path implementation: M/L/C/Z plus Q, raised to a
 * cubic so nothing downstream has to know there are two kinds of curve. A bare
 * number continues a line, so the "M10 10 20 20" form SVG allows does what it
 * looks like.
 */
int bx_vg_parse_path_d(bx_vg_shape_t *s, const char *dstr) {
    if (!s || !dstr) return 0;
    s->type = BX_VG_PATH;
    vg_lexer_t lx = { dstr, NULL, 0, dstr };
    float val[6];
    char cmd;
    int start = s->ncmd;
    for (;;) {
        if (!vg_num(&lx, &val[0], &cmd)) break;
        char op = cmd ? cmd : 'L';
        if (op == 'M' || op == 'm') {
            vg_num(&lx, &val[1], NULL);
            float pair[2] = { val[0], val[1] };
            bx_vg_path_cmd(s, BX_VG_OP_MOVE, pair, 2);
        } else if (op == 'L' || op == 'l') {
            vg_num(&lx, &val[1], NULL);
            float pair[2] = { val[0], val[1] };
            bx_vg_path_cmd(s, BX_VG_OP_LINE, pair, 2);
        } else if (op == 'C' || op == 'c') {
            for (int k = 2; k < 6; k++) vg_num(&lx, &val[k], NULL);
            bx_vg_path_cmd(s, BX_VG_OP_CURVE, val, 6);
        } else if (op == 'Q' || op == 'q') {
            float qx, qy, ex, ey;
            vg_num(&lx, &qx, NULL);
            vg_num(&lx, &qy, NULL);
            vg_num(&lx, &ex, NULL);
            vg_num(&lx, &ey, NULL);
            float cub[6];
            cub[0] = val[0] + 2.0f / 3.0f * (qx - val[0]);
            cub[1] = val[1] + 2.0f / 3.0f * (qy - val[1]);
            cub[2] = ex + 2.0f / 3.0f * (qx - ex);
            cub[3] = ey + 2.0f / 3.0f * (qy - ey);
            cub[4] = ex; cub[5] = ey;
            bx_vg_path_cmd(s, BX_VG_OP_CURVE, cub, 6);
            val[0] = ex; val[1] = ey;
        } else if (op == 'Z' || op == 'z') {
            bx_vg_path_cmd(s, BX_VG_OP_CLOSE, NULL, 0);
        } else {
            break;
        }
    }
    return s->ncmd - start;
}

/* ------------------------------------------------------------------- SVG */

int bx_vg_render_svg(const bx_vg_doc_t *d, char *out, size_t cap) {
    return bx_vg_to_svg(d, out, cap);
}

int bx_vg_to_svg(const bx_vg_doc_t *d, char *out, size_t cap) {
    if (!d || !out || cap == 0) return -1;
    size_t n = 0;
    char col[16];
    /* A document is its own viewBox, so the SVG needs no width or height on
     * the root: it scales wherever it lands. */
    n = vg_put(out, cap, n,
        "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 %g %g\">\n", d->w, d->h);
    if (d->has_bg) {
        vg_hex(col, sizeof col, d->bg);
        n = vg_put(out, cap, n, "<rect width=\"%g\" height=\"%g\" fill=\"%s\"/>\n", d->w, d->h, col);
    }
    const bx_vg_frame_t *f = NULL;
    if (d->nframe > 0 && d->cur_frame >= 0 && d->cur_frame < d->nframe)
        f = &d->frame[d->cur_frame];
    int first = f ? f->first : 0;
    int count = f ? f->count : d->nshape;

    for (int i = first; i < first + count && i < d->nshape; i++) {
        const bx_vg_shape_t *s = &d->shape[i];
        char fill[16], stroke[16];
        snprintf(fill, sizeof fill, s->has_fill ? "%s" : "none", "");
        if (s->has_fill) vg_hex(fill, sizeof fill, s->fill);
        if (s->has_stroke) vg_hex(stroke, sizeof stroke, s->stroke);
        else snprintf(stroke, sizeof stroke, "none");

        const char *paint =
            "fill=\"%s\" stroke=\"%s\" stroke-width=\"%g\" stroke-linecap=\"round\" "
            "stroke-linejoin=\"round\"";
        if (s->opacity < 0.999f)
            n = vg_put(out, cap, n, "<g opacity=\"%g\">", s->opacity);

        switch (s->type) {
        case BX_VG_RECT:
            n = vg_put(out, cap, n, "<rect x=\"%g\" y=\"%g\" width=\"%g\" height=\"%g\" ",
                       s->x, s->y, s->w, s->h);
            n = vg_put(out, cap, n, paint, fill, stroke, s->stroke_w);
            n = vg_put(out, cap, n, "/>\n");
            break;
        case BX_VG_CIRCLE:
            n = vg_put(out, cap, n, "<circle cx=\"%g\" cy=\"%g\" r=\"%g\" ",
                       s->x + s->w / 2, s->y + s->h / 2, s->w / 2);
            n = vg_put(out, cap, n, paint, fill, stroke, s->stroke_w);
            n = vg_put(out, cap, n, "/>\n");
            break;
        case BX_VG_ELLIPSE:
            n = vg_put(out, cap, n, "<ellipse cx=\"%g\" cy=\"%g\" rx=\"%g\" ry=\"%g\" ",
                       s->x + s->w / 2, s->y + s->h / 2, s->w / 2, s->h / 2);
            n = vg_put(out, cap, n, paint, fill, stroke, s->stroke_w);
            n = vg_put(out, cap, n, "/>\n");
            break;
        case BX_VG_LINE:
            n = vg_put(out, cap, n, "<line x1=\"%g\" y1=\"%g\" x2=\"%g\" y2=\"%g\" ",
                       s->x, s->y, s->x1, s->y1);
            n = vg_put(out, cap, n, paint, "none", stroke, s->stroke_w);
            n = vg_put(out, cap, n, "/>\n");
            break;
        case BX_VG_POLY:
            n = vg_put(out, cap, n, "<polygon points=\"");
            for (int32_t k = 0; k < s->ncmd; k++)
                n = vg_put(out, cap, n, "%g,%g ", s->cmd[k].v[0], s->cmd[k].v[1]);
            n = vg_put(out, cap, n, "\" ");
            n = vg_put(out, cap, n, paint, fill, stroke, s->stroke_w);
            n = vg_put(out, cap, n, "/>\n");
            break;
        case BX_VG_PATH:
            n = vg_put(out, cap, n, "<path d=\"");
            for (int32_t k = 0; k < s->ncmd; k++) {
                const bx_vg_cmd_t *c = &s->cmd[k];
                switch (c->op) {
                case BX_VG_OP_MOVE:  n = vg_put(out, cap, n, "M%g %g ", c->v[0], c->v[1]); break;
                case BX_VG_OP_LINE:  n = vg_put(out, cap, n, "L%g %g ", c->v[0], c->v[1]); break;
                case BX_VG_OP_CURVE: n = vg_put(out, cap, n, "C%g %g %g %g %g %g ",
                                c->v[0], c->v[1], c->v[2], c->v[3], c->v[4], c->v[5]); break;
                case BX_VG_OP_ARC:   n = vg_put(out, cap, n, "A%g %g %g %g %g %g ",
                                c->v[0], c->v[1], c->v[2], c->v[3], c->v[4], c->v[5]); break;
                case BX_VG_OP_CLOSE: n = vg_put(out, cap, n, "Z"); break;
                }
            }
            n = vg_put(out, cap, n, "\" ");
            n = vg_put(out, cap, n, paint, fill, stroke, s->stroke_w);
            n = vg_put(out, cap, n, "/>\n");
            break;
        default:
            break;
        }
        if (s->opacity < 0.999f) n = vg_put(out, cap, n, "</g>\n");
    }
    n = vg_put(out, cap, n, "</svg>\n");
    if (n < cap) out[n] = 0;
    else if (cap) out[cap - 1] = 0;
    return (int)(n < cap ? n : cap - 1);
}

int bx_vg_svg_file(const bx_vg_doc_t *d, const char *path) {
    if (!d || !path) return -1;
    size_t cap = 1 << 20;
    char *buf = (char *)malloc(cap);
    if (!buf) return -1;
    if (bx_vg_to_svg(d, buf, cap) < 0) { free(buf); return -1; }
    FILE *f = fopen(path, "wb");
    if (!f) { free(buf); return -1; }
    size_t len = strlen(buf);
    fwrite(buf, 1, len, f);
    fclose(f);
    free(buf);
    return 0;
}

/* -------------------------------------------------------------- SVG in */

/* SVG is XML and bx has no XML parser, but it does not need one to read an
 * icon: this walks the text looking for the shape elements it understands and
 * reads their attributes. Anything else is skipped, which is right for artwork
 * made by a drawing tool, where most of the file is <defs> and metadata nobody
 * drawing wants to see.
 *
 * Attributes come in two spellings: SVG writes stroke-width, bx writes w. Both
 * are accepted, and so are single quotes, which is what a hand-written file
 * tends to have.
 */

/* Read attr="value" or attr='value' from *p, advancing past it. */
static int vg_attr(const char **p, const char *name, char *out, size_t cap) {
    const char *s = *p;
    size_t nlen = strlen(name);
    while ((s = strchr(s, name[0])) != NULL) {
        /* Must be a whole attribute: preceded by a space or a tag start. */
        if (s > *p && (s[-1] != ' ' && s[-1] != '\t' && s[-1] != '\n' && s[-1] != '<')) {
            s++;
            continue;
        }
        if (strncmp(s, name, nlen) != 0) { s++; continue; }
        const char *q = s + nlen;
        while (*q == ' ') q++;
        if (*q != '=') { s++; continue; }
        q++;
        while (*q == ' ') q++;
        char qc = *q;
        if (qc != '"' && qc != '\'') { s++; continue; }
        q++;
        size_t i = 0;
        while (*q && *q != qc && i + 1 < cap) out[i++] = *q++;
        out[i] = 0;
        if (*q == qc) q++;
        *p = q;
        return 1;
    }
    *p = s ? s : *p;
    return 0;
}

/* A color that may be "#rgb", "none", or a name. Returns 0 for "none". */
static int vg_svg_color(const char *val, uint32_t *out) {
    if (!val || !*val) return 0;
    if (!strcmp(val, "none") || !strcmp(val, "transparent")) return 0;
    int found = 0;
    uint32_t c = bx_gfx_color_named(val, &found);
    if (!found) {
        c = bx_gfx_parse_color(val);
        if (BX_GFX_A(c) == 0) return 0;      /* unparsable */
    }
    *out = c;
    return 1;
}

static float vg_svg_num(const char *val, float dflt) {
    if (!val || !*val) return dflt;
    char *end = NULL;
    double v = strtod(val, &end);
    if (end == val) return dflt;
    return (float)v;
}

/* SVG lengths often carry a unit. Strip it: everything here is user units. */
static void vg_svg_len(const char *val, float *out, float dflt) {
    if (!val || !*val) { *out = dflt; return; }
    char *end = NULL;
    double v = strtod(val, &end);
    if (end == val) { *out = dflt; return; }
    *out = (float)v;
}

/* Read a file into the document as SVG. Same shape as bx_vg_parse_file, and
 * kept separate so a caller can offer both and let the caller decide. */
long bx_vg_from_svg_readfile(const char *path, char *err, size_t errcap) {
    if (!path) { vg_err(err, errcap, "no path"); return -1; }
    FILE *f = fopen(path, "rb");
    if (!f) {
        char m[256];
        snprintf(m, sizeof m, "cannot read %s", path);
        vg_err(err, errcap, m);
        return -1;
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0 || sz > (1 << 22)) { fclose(f); vg_err(err, errcap, "file too large"); return -1; }
    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); vg_err(err, errcap, "out of memory"); return -1; }
    size_t got = fread(buf, 1, (size_t)sz, f);
    buf[got] = 0;
    fclose(f);
    long n = bx_vg_from_svg(buf, err, errcap);
    free(buf);
    return n;
}

long bx_vg_from_svg(const char *text, char *err, size_t errcap) {
    if (!text) { vg_err(err, errcap, "no text"); return -1; }
    bx_vg_doc_t *d = &g_vg;
    bx_vg_reset(d);

    /* The root's viewBox, or width/height, sets the document's size. An SVG
     * with neither is treated as a 24x24 icon rather than rejected. */
    const char *root = text;
    if (root && strstr(root, "<svg")) root = strstr(root, "<svg");
    if (root) {
        char v[64];
        const char *p = root;
        if (vg_attr(&p, "viewBox", v, sizeof v)) {
            float a[4] = { 0, 0, 0, 0 };
            if (sscanf(v, "%g %g %g %g", &a[0], &a[1], &a[2], &a[3]) >= 3) {
                if (a[2] > 0) d->w = a[2];
                if (a[3] > 0) d->h = a[3];
            }
        } else if (vg_attr(&p, "width", v, sizeof v) && vg_attr(&p, "height", v, sizeof v)) {
            float w = 0, h = 0;
            vg_svg_len(v, &w, 0);
            p = root;
            vg_attr(&p, "height", v, sizeof v);
            vg_svg_len(v, &h, 0);
            if (w > 0) d->w = w;
            if (h > 0) d->h = h;
        }
    }

    const char *p = text;
    int depth = 0;
    while ((p = strchr(p, '<')) != NULL) {
        p++;
        if (*p == '/' || *p == '?' || *p == '!') { while (*p && *p != '>') p++; continue; }
        char tag[32];
        size_t i = 0;
        while (*p && !(*p == ' ' || *p == '>' || *p == '/' || *p == '\n') &&
               i + 1 < sizeof tag) tag[i++] = *p++;
        tag[i] = 0;
        /* Find the end of the tag, quoting-aware so a '>' inside an attribute
         * value does not end it early. */
        const char *tagend = p;
        char qc = 0;
        while (*tagend) {
            if (qc) { if (*tagend == qc) qc = 0; }
            else if (*tagend == '"' || *tagend == '\'') qc = *tagend;
            else if (*tagend == '>') break;
            tagend++;
        }
        if (*tagend == '>') tagend++;

        /* stroke-width and friends live on <g> as often as on the shape, so
         * carry the group down. */
        static char inh_fill[16] = "", inh_stroke[16] = "", inh_w[16] = "";
        char save_fill[16], save_stroke[16], save_w[16];
        memcpy(save_fill, inh_fill, 16);
        memcpy(save_stroke, inh_stroke, 16);
        memcpy(save_w, inh_w, 16);
        {
            char v[64];
            const char *ap = p;
            if (vg_attr(&ap, "fill", v, sizeof v)) snprintf(inh_fill, 16, "%s", v);
            if (vg_attr(&ap, "stroke", v, sizeof v)) snprintf(inh_stroke, 16, "%s", v);
            if (vg_attr(&ap, "stroke-width", v, sizeof v) ||
                vg_attr(&ap, "width", v, sizeof v)) snprintf(inh_w, 16, "%s", v);
        }
        if (!strcmp(tag, "g") || !strcmp(tag, "svg")) depth++;
        if (!strcmp(tag, "/g") || !strcmp(tag, "/svg")) {
            depth--;
            memcpy(inh_fill, save_fill, 16);
            memcpy(inh_stroke, save_stroke, 16);
            memcpy(inh_w, save_w, 16);
            continue;
        }

        int is_shape = !strcmp(tag, "rect") || !strcmp(tag, "circle") ||
                       !strcmp(tag, "ellipse") || !strcmp(tag, "line") ||
                       !strcmp(tag, "polyline") || !strcmp(tag, "polygon") ||
                       !strcmp(tag, "path");
        if (!is_shape) continue;

        char name[48];
        snprintf(name, sizeof name, "%s%ld", tag, (long)d->nshape);
        int32_t si = bx_vg_shape_add(d, name);
        if (si < 0) continue;
        bx_vg_shape_t *s = &d->shape[si];

        const char *ap = p;
        char v[256];
        if (vg_attr(&ap, "id", v, sizeof v) && *v) snprintf(s->name, sizeof s->name, "%s", v);

        /* Style first, then geometry: geometry must not reset a fill that was
         * set on the element. */
        s->has_fill = 0;
        s->has_stroke = 0;
        s->stroke_w = 1.0f;
        if (inh_fill[0] && vg_svg_color(inh_fill, &s->fill)) s->has_fill = 1;
        if (inh_stroke[0] && vg_svg_color(inh_stroke, &s->stroke)) s->has_stroke = 1;
        if (inh_w[0]) s->stroke_w = vg_svg_num(inh_w, 1.0f);
        if (vg_attr(&ap, "fill", v, sizeof v)) s->has_fill = vg_svg_color(v, &s->fill);
        if (vg_attr(&ap, "stroke", v, sizeof v)) s->has_stroke = vg_svg_color(v, &s->stroke);
        if (vg_attr(&ap, "stroke-width", v, sizeof v)) s->stroke_w = vg_svg_num(v, 1.0f);
        if (vg_attr(&ap, "opacity", v, sizeof v)) s->opacity = vg_svg_num(v, 1.0f);

        float a = 0, b = 0, c = 0, e = 0;
        if (!strcmp(tag, "rect")) {
            s->type = BX_VG_RECT;
            const char *q = p;
            if (vg_attr(&q, "x", v, sizeof v)) vg_svg_len(v, &a, 0);
            if (vg_attr(&q, "y", v, sizeof v)) vg_svg_len(v, &b, 0);
            if (vg_attr(&q, "width", v, sizeof v)) vg_svg_len(v, &c, 0);
            if (vg_attr(&q, "height", v, sizeof v)) vg_svg_len(v, &e, 0);
            s->x = a; s->y = b; s->w = c; s->h = e;
            if (!s->has_fill) { s->fill = BX_GFX_OPAQUE(0, 0, 0); s->has_fill = 1; }
        } else if (!strcmp(tag, "circle")) {
            s->type = BX_VG_CIRCLE;
            const char *q = p;
            if (vg_attr(&q, "cx", v, sizeof v)) vg_svg_len(v, &a, 0);
            if (vg_attr(&q, "cy", v, sizeof v)) vg_svg_len(v, &b, 0);
            if (vg_attr(&q, "r", v, sizeof v)) vg_svg_len(v, &c, 0);
            s->x = a - c; s->y = b - c; s->w = c * 2; s->h = c * 2;
            if (!s->has_fill && !s->has_stroke) {
                s->fill = BX_GFX_OPAQUE(0, 0, 0); s->has_fill = 1;
            }
        } else if (!strcmp(tag, "ellipse")) {
            s->type = BX_VG_ELLIPSE;
            const char *q = p;
            if (vg_attr(&q, "cx", v, sizeof v)) vg_svg_len(v, &a, 0);
            if (vg_attr(&q, "cy", v, sizeof v)) vg_svg_len(v, &b, 0);
            if (vg_attr(&q, "rx", v, sizeof v)) vg_svg_len(v, &c, 0);
            if (vg_attr(&q, "ry", v, sizeof v)) vg_svg_len(v, &e, 0);
            s->x = a - c; s->y = b - c; s->w = c * 2; s->h = e * 2;
            if (!s->has_fill && !s->has_stroke) {
                s->fill = BX_GFX_OPAQUE(0, 0, 0); s->has_fill = 1;
            }
        } else if (!strcmp(tag, "line")) {
            s->type = BX_VG_LINE;
            const char *q = p;
            if (vg_attr(&q, "x1", v, sizeof v)) vg_svg_len(v, &a, 0);
            if (vg_attr(&q, "y1", v, sizeof v)) vg_svg_len(v, &b, 0);
            if (vg_attr(&q, "x2", v, sizeof v)) vg_svg_len(v, &c, 0);
            if (vg_attr(&q, "y2", v, sizeof v)) vg_svg_len(v, &e, 0);
            s->x = a; s->y = b; s->x1 = c; s->y1 = e;
            if (!s->has_stroke) { s->stroke = BX_GFX_OPAQUE(0, 0, 0); s->has_stroke = 1; }
        } else if (!strcmp(tag, "polyline") || !strcmp(tag, "polygon")) {
            s->type = BX_VG_POLY;
            const char *q = p;
            if (vg_attr(&q, "points", v, sizeof v)) {
                float px = 0, py = 0;
                const char *s2 = v;
                while (*s2) {
                    char *end = NULL;
                    double vx = strtod(s2, &end);
                    if (end == s2) break;
                    s2 = end;
                    double vy = strtod(s2, &end);
                    if (end == s2) break;
                    s2 = end;
                    px = (float)vx; py = (float)vy;
                    float pair[2] = { px, py };
                    bx_vg_path_cmd(s, BX_VG_OP_LINE, pair, 2);
                }
            }
            if (!strcmp(tag, "polygon")) s->closed = 1;
            if (!s->has_fill && !s->has_stroke) {
                s->fill = BX_GFX_OPAQUE(0, 0, 0); s->has_fill = 1;
            }
        } else if (!strcmp(tag, "path")) {
            s->type = BX_VG_PATH;
            const char *q = p;
            if (vg_attr(&q, "d", v, sizeof v)) bx_vg_parse_path_d(s, v);
            if (!s->has_fill && !s->has_stroke) {
                s->fill = BX_GFX_OPAQUE(0, 0, 0); s->has_fill = 1;
            }
        }
        /* A path with no fill and no stroke drew nothing; give it the default
         * paint so importing an artwork file does not produce blank icons. */
        if (!s->has_fill && !s->has_stroke && s->type != BX_VG_LINE) {
            s->fill = BX_GFX_OPAQUE(0, 0, 0);
            s->has_fill = 1;
        }
        p = tagend;
    }
    return d->nshape;
}
