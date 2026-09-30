/* BX MATH - see bx_math.h. */
#define _POSIX_C_SOURCE 200809L
#include "bx_math.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Integer extras ------------------------------------------------------------ */

long long bx_math_gcd(long long a, long long b) {
    if (a < 0) a = -a;
    if (b < 0) b = -b;
    while (b) { long long t = a % b; a = b; b = t; }
    return a;
}

long long bx_math_lcm(long long a, long long b) {
    if (!a || !b) return 0;
    return a / bx_math_gcd(a, b) * b;
}

long long bx_math_fact(int n) {
    long long r = 1;
    if (n < 0) return 1;
    for (int i = 2; i <= n; i++) r *= i;
    return r;
}

int bx_math_isprime(long long n) {
    if (n < 2) return 0;
    if (n % 2 == 0) return n == 2;
    for (long long i = 3; i * i <= n; i += 2)
        if (n % i == 0) return 0;
    return 1;
}

long long bx_math_nthprime(int n) {
    if (n < 1) return 0;
    if (n == 1) return 2;
    long long p = 2;
    int count = 1;
    for (long long c = 3; count < n; c += 2) {
        if (bx_math_isprime(c)) { p = c; count++; }
    }
    return p;
}

long long bx_math_ncr(long long n, long long r) {
    if (r < 0 || r > n) return 0;
    if (r > n - r) r = n - r;
    long long res = 1;
    for (long long i = 1; i <= r; i++)
        res = res * (n - r + i) / i;
    return res;
}

long long bx_math_npr(long long n, long long r) {
    if (r < 0 || r > n) return 0;
    long long res = 1;
    for (long long i = 0; i < r; i++) res *= n - i;
    return res;
}

long long bx_math_fib(int n) {
    if (n < 0) return 0;
    long long a = 0, b = 1;
    if (n == 0) return 0;
    for (int i = 2; i <= n; i++) { long long t = a + b; a = b; b = t; }
    return b;
}

long long bx_math_isqrt(long long x) {
    if (x < 0) return -1;
    long long r = (long long)sqrt((double)x);
    while (r * r > x) r--;
    while ((r + 1) * (r + 1) <= x) r++;
    return r;
}

long long bx_math_totient(long long n) {
    if (n < 2) return n < 1 ? 0 : 1;
    long long result = n, x = n;
    if (x % 2 == 0) { while (x % 2 == 0) x /= 2; result -= result / 2; }
    for (long long i = 3; i <= x && i * i <= n; i += 2) {
        if (x % i == 0) {
            while (x % i == 0) x /= i;
            result -= result / i;
        }
    }
    if (x > 1) result -= result / x;
    return result;
}

long long bx_math_divcount(long long n) {
    if (n < 1) return 0;
    long long count = 0;
    long long i;
    for (i = 1; i * i < n; i++) if (n % i == 0) count += 2;
    if (i * i == n) count++;
    return count;
}

long long bx_math_divsum(long long n) {
    if (n < 1) return 0;
    long long sum = n + 1;   /* 1 and n */
    if (n == 1) return 1;
    for (long long i = 2; i * i <= n; i++)
        if (n % i == 0) sum += i + (i == n / i ? 0 : n / i);
    return sum;
}

long long bx_math_digitsum(long long n) {
    if (n < 0) n = -n;
    long long s = 0;
    while (n) { s += n % 10; n /= 10; }
    return s;
}

long long bx_math_droot(long long n) {
    if (n < 0) n = -n;
    while (n > 9) n = bx_math_digitsum(n);
    return n;
}

long long bx_math_collatz(long long n) {
    if (n <= 0) return -1;
    long long steps = 0;
    while (n != 1) {
        n = (n % 2) ? 3 * n + 1 : n / 2;
        steps++;
    }
    return steps;
}

int bx_math_factor(long long n, char *out, size_t outsz) {
    size_t pos = 0;
    int count = 0;
    out[0] = 0;
    if (n < 2) return 0;
    while (n % 2 == 0) {
        pos += (size_t)snprintf(out + pos, outsz - pos, "%s2", pos ? " " : "");
        n /= 2; count++;
    }
    for (long long i = 3; i <= n; i += 2) {
        while (n % i == 0) {
            pos += (size_t)snprintf(out + pos, outsz - pos, "%s%lld", pos ? " " : "", i);
            n /= i; count++;
        }
    }
    return count;
}

int bx_math_frac(double v, long long *num, long long *den) {
    if (!(v > 0)) { *num = 0; *den = 1; return -1; }
    /* continued-fraction -> rational approximation on 1e-9 tolerance */
    long long h0 = 0, h1 = 1, k0 = 1, k1 = 0;
    double x = v;
    for (int i = 0; i < 60; i++) {
        long long a = (long long)floor(x);
        long long nh = a * h1 + h0, nk = a * k1 + k0;
        if (nk > 1000000000LL) break;
        h0 = h1; h1 = nh; k0 = k1; k1 = nk;
        double diff = h1 - v * k1;
        if (diff < -0.0) diff = -diff;
        if (diff < 1e-9 && k1 != 0) { *num = h1; *den = k1; return 0; }
        x = 1.0 / (x - a);
        if (!isfinite(x)) break;
    }
    *num = (long long)(v + 0.5); *den = 1;
    return 0;
}

/* Float cleanup -------------------------------------------------------------- */

double bx_math_fpclean(double v) {
    if (!isfinite(v)) return v;
    v = round(v * 1e9) / 1e9;
    return v == 0.0 ? 0.0 : v;
}

/* vec3 ------------------------------------------------------------------------ */

int bx_math_vec_parse(const char *s, double out[3]) {
    int n = 0;
    char buf[128];
    size_t len = strlen(s);
    if (len >= sizeof buf) len = sizeof buf - 1;
    memcpy(buf, s, len);
    buf[len] = 0;
    char *save = NULL, *tok;
    for (tok = strtok_r(buf, " ,", &save); tok && n < 3; tok = strtok_r(NULL, " ,", &save)) {
        out[n++] = atof(tok);
    }
    return n;
}

int bx_math_vec_fmt(char *out, size_t sz, const double v[3]) {
    return snprintf(out, sz, "%g %g %g", bx_math_fpclean(v[0]),
                    bx_math_fpclean(v[1]), bx_math_fpclean(v[2])) < 0 ? -1 : 0;
}

int bx_math_vec_len(const char *s, double *len) {
    double v[3] = {0, 0, 0};
    int n = bx_math_vec_parse(s, v);
    if (n < 1) return -1;
    *len = bx_math_fpclean(sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]));
    return 0;
}

int bx_math_vec_norm(const char *s, char *out, size_t sz) {
    double v[3] = {0, 0, 0};
    int n = bx_math_vec_parse(s, v);
    if (n < 1) return -1;
    double l = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (l == 0) { out[0] = 0; return -1; }
    double r[3] = {v[0] / l, v[1] / l, v[2] / l};
    return bx_math_vec_fmt(out, sz, r) < 0 ? -1 : 0;
}

#define BX_M_PI 3.14159265358979323846

int bx_math_vec_add(const char *a, const char *b, char *out, size_t sz) {
    double va[3] = {0, 0, 0}, vb[3] = {0, 0, 0};
    if (bx_math_vec_parse(a, va) < 1 || bx_math_vec_parse(b, vb) < 1) return -1;
    double r[3] = {va[0] + vb[0], va[1] + vb[1], va[2] + vb[2]};
    return bx_math_vec_fmt(out, sz, r) < 0 ? -1 : 0;
}

int bx_math_vec_sub(const char *a, const char *b, char *out, size_t sz) {
    double va[3] = {0, 0, 0}, vb[3] = {0, 0, 0};
    if (bx_math_vec_parse(a, va) < 1 || bx_math_vec_parse(b, vb) < 1) return -1;
    double r[3] = {va[0] - vb[0], va[1] - vb[1], va[2] - vb[2]};
    return bx_math_vec_fmt(out, sz, r) < 0 ? -1 : 0;
}

int bx_math_vec_scale(const char *a, double s, char *out, size_t sz) {
    double v[3] = {0, 0, 0};
    if (bx_math_vec_parse(a, v) < 1) return -1;
    double r[3] = {v[0] * s, v[1] * s, v[2] * s};
    return bx_math_vec_fmt(out, sz, r) < 0 ? -1 : 0;
}

int bx_math_vec_dot(const char *a, const char *b, double *d) {
    double va[3] = {0, 0, 0}, vb[3] = {0, 0, 0};
    if (bx_math_vec_parse(a, va) < 1 || bx_math_vec_parse(b, vb) < 1) return -1;
    *d = bx_math_fpclean(va[0] * vb[0] + va[1] * vb[1] + va[2] * vb[2]);
    return 0;
}

int bx_math_vec_cross(const char *a, const char *b, char *out, size_t sz) {
    double va[3] = {0, 0, 0}, vb[3] = {0, 0, 0};
    if (bx_math_vec_parse(a, va) < 1 || bx_math_vec_parse(b, vb) < 1) return -1;
    double r[3] = {va[1] * vb[2] - va[2] * vb[1],
                   va[2] * vb[0] - va[0] * vb[2],
                   va[0] * vb[1] - va[1] * vb[0]};
    return bx_math_vec_fmt(out, sz, r) < 0 ? -1 : 0;
}

/* matrices -------------------------------------------------------------------- */

int bx_math_mat_parse(const char *s, double m[16]) {
    int n = 0;
    char buf[512];
    size_t len = strlen(s);
    if (len >= sizeof buf) len = sizeof buf - 1;
    memcpy(buf, s, len);
    buf[len] = 0;
    char *save = NULL, *tok;
    for (tok = strtok_r(buf, " ,", &save); tok && n < 16; tok = strtok_r(NULL, " ,", &save)) {
        m[n++] = atof(tok);
    }
    return n;
}

int bx_math_mat_fmt(char *out, size_t sz, const double m[16]) {
    size_t pos = 0;
    for (int i = 0; i < 16; i++) {
        pos += (size_t)snprintf(out + pos, sz - pos, "%s%g",
                                i ? " " : "", bx_math_fpclean(m[i]));
    }
    return pos >= sz ? -1 : 0;
}

static void mat_identity(double m[16]) {
    for (int i = 0; i < 16; i++) m[i] = 0;
    m[0] = m[5] = m[10] = m[15] = 1;
}

int bx_math_mat_identity(char *out, size_t sz) {
    double m[16];
    mat_identity(m);
    return bx_math_mat_fmt(out, sz, m);
}

int bx_math_mat_translate(const char *xyz, char *out, size_t sz) {
    double v[3] = {0, 0, 0};
    bx_math_vec_parse(xyz, v);
    double m[16];
    mat_identity(m);
    m[12] = v[0]; m[13] = v[1]; m[14] = v[2];
    return bx_math_mat_fmt(out, sz, m);
}

int bx_math_mat_scale(const char *xyz, char *out, size_t sz) {
    double v[3] = {0, 0, 0};
    bx_math_vec_parse(xyz, v);
    double m[16];
    mat_identity(m);
    m[0] = v[0]; m[5] = v[1]; m[10] = v[2];
    return bx_math_mat_fmt(out, sz, m);
}

static void rot_matrix(double m[16], double rx, double ry, double rz) {
    double cx = cos(rx), sx = sin(rx), cy = cos(ry), sy = sin(ry), cz = cos(rz), sz = sin(rz);
    m[0] = cy * cz;               m[1] = -cy * sz;              m[2] = sy;
    m[3] = 0;
    m[4] = sx * sy * cz + cx * sz; m[5] = -sx * sy * sz + cx * cz; m[6] = -sx * cy;
    m[7] = 0;
    m[8] = -cx * sy * cz + sx * sz; m[9] = cx * sy * sz + sx * cz; m[10] = cx * cy;
    m[11] = 0;
    m[12] = m[13] = m[14] = 0; m[15] = 1;
}

int bx_math_mat_rotx(double deg, char *out, size_t sz) {
    double m[16];
    rot_matrix(m, deg * BX_M_PI / 180.0, 0, 0);
    return bx_math_mat_fmt(out, sz, m);
}

int bx_math_mat_roty(double deg, char *out, size_t sz) {
    double m[16];
    rot_matrix(m, 0, deg * BX_M_PI / 180.0, 0);
    return bx_math_mat_fmt(out, sz, m);
}

int bx_math_mat_rotz(double deg, char *out, size_t sz) {
    double m[16];
    rot_matrix(m, 0, 0, deg * BX_M_PI / 180.0);
    return bx_math_mat_fmt(out, sz, m);
}

int bx_math_mat_rot(const char *axis, double deg, char *out, size_t sz) {
    /* Rodrigues rotation about a unit axis */
    double v[3] = {0, 0, 0};
    bx_math_vec_parse(axis, v);
    double l = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
    if (l == 0) return -1;
    l = sqrt(l); v[0] /= l; v[1] /= l; v[2] /= l;
    double a = deg * BX_M_PI / 180.0, c = cos(a), s = sin(a), t = 1 - c;
    double m[16];
    m[0] = t * v[0] * v[0] + c;
    m[1] = t * v[0] * v[1] - s * v[2];
    m[2] = t * v[0] * v[2] + s * v[1];
    m[3] = 0;
    m[4] = t * v[0] * v[1] + s * v[2];
    m[5] = t * v[1] * v[1] + c;
    m[6] = t * v[1] * v[2] - s * v[0];
    m[7] = 0;
    m[8] = t * v[0] * v[2] - s * v[1];
    m[9] = t * v[1] * v[2] + s * v[0];
    m[10] = t * v[2] * v[2] + c;
    m[11] = 0;
    m[12] = m[13] = m[14] = 0; m[15] = 1;
    return bx_math_mat_fmt(out, sz, m);
}

int bx_math_mat_mul(const char *a, const char *b, char *out, size_t sz) {
    double A[16], B[16], R[16];
    if (bx_math_mat_parse(a, A) != 16 || bx_math_mat_parse(b, B) != 16) return -1;
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) {
            double sum = 0;
            for (int k = 0; k < 4; k++) sum += A[r * 4 + k] * B[k * 4 + c];
            R[r * 4 + c] = sum;
        }
    return bx_math_mat_fmt(out, sz, R);
}

int bx_math_mat_vec(const char *m, const char *v, char *out, size_t sz) {
    double M[16], vv[3] = {0, 0, 0};
    if (bx_math_mat_parse(m, M) != 16 || bx_math_vec_parse(v, vv) < 1) return -1;
    double r[3] = {M[0] * vv[0] + M[1] * vv[1] + M[2] * vv[2] + M[3],
                   M[4] * vv[0] + M[5] * vv[1] + M[6] * vv[2] + M[7],
                   M[8] * vv[0] + M[9] * vv[1] + M[10] * vv[2] + M[11]};
    return bx_math_vec_fmt(out, sz, r);
}

/* quaternions ------------------------------------------------------------------ */

int bx_math_quat_axis(double x, double y, double z, double deg, char *out, size_t sz) {
    double l = sqrt(x * x + y * y + z * z);
    if (l == 0) return -1;
    x /= l; y /= l; z /= l;
    double a = deg * BX_M_PI / 360.0;
    double q[4] = {cos(a), x * sin(a), y * sin(a), z * sin(a)};
    return snprintf(out, sz, "%g %g %g %g", bx_math_fpclean(q[0]),
                    bx_math_fpclean(q[1]), bx_math_fpclean(q[2]), bx_math_fpclean(q[3])) < 0 ? -1 : 0;
}

static int qparse(const char *s, double q[4]) {
    char buf[128];
    size_t len = strlen(s);
    if (len >= sizeof buf) len = sizeof buf - 1;
    memcpy(buf, s, len);
    buf[len] = 0;
    char *save = NULL, *tok;
    int n = 0;
    for (tok = strtok_r(buf, " ,", &save); tok && n < 4; tok = strtok_r(NULL, " ,", &save)) {
        q[n++] = atof(tok);
    }
    return n;
}

int bx_math_quat_mul(const char *a, const char *b, char *out, size_t sz) {
    double qa[4], qb[4], r[4];
    if (qparse(a, qa) != 4 || qparse(b, qb) != 4) return -1;
    r[0] = qa[0] * qb[0] - qa[1] * qb[1] - qa[2] * qb[2] - qa[3] * qb[3];
    r[1] = qa[0] * qb[1] + qa[1] * qb[0] + qa[2] * qb[3] - qa[3] * qb[2];
    r[2] = qa[0] * qb[2] - qa[1] * qb[3] + qa[2] * qb[0] + qa[3] * qb[1];
    r[3] = qa[0] * qb[3] + qa[1] * qb[2] - qa[2] * qb[1] + qa[3] * qb[0];
    return snprintf(out, sz, "%g %g %g %g", bx_math_fpclean(r[0]), bx_math_fpclean(r[1]),
                    bx_math_fpclean(r[2]), bx_math_fpclean(r[3])) < 0 ? -1 : 0;
}

int bx_math_quat_conj(const char *q, char *out, size_t sz) {
    double qv[4];
    if (qparse(q, qv) != 4) return -1;
    return snprintf(out, sz, "%g %g %g %g", bx_math_fpclean(qv[0]), bx_math_fpclean(-qv[1]),
                    bx_math_fpclean(-qv[2]), bx_math_fpclean(-qv[3])) < 0 ? -1 : 0;
}

int bx_math_quat_norm(const char *q, char *out, size_t sz) {
    double qv[4];
    if (qparse(q, qv) != 4) return -1;
    double l = sqrt(qv[0] * qv[0] + qv[1] * qv[1] + qv[2] * qv[2] + qv[3] * qv[3]);
    if (l == 0) return -1;
    return snprintf(out, sz, "%g %g %g %g", bx_math_fpclean(qv[0] / l), bx_math_fpclean(qv[1] / l),
                    bx_math_fpclean(qv[2] / l), bx_math_fpclean(qv[3] / l)) < 0 ? -1 : 0;
}

int bx_math_quat_rot(const char *q, const char *v, char *out, size_t sz) {
    double qv[4], vv[3] = {0, 0, 0};
    if (qparse(q, qv) != 4 || bx_math_vec_parse(v, vv) < 1) return -1;
    /* v' = q v q^-1 using the (w,x,y,z) convention */
    double x = vv[0], y = vv[1], z = vv[2];
    double w = qv[0], qx = qv[1], qy = qv[2], qz = qv[3];
    double ix = w * x + qy * z - qz * y;
    double iy = w * y + qz * x - qx * z;
    double iz = w * z + qx * y - qy * x;
    double iw = -qx * x - qy * y - qz * z;
    double r[3] = {ix * w + iw * -qx + iy * -qz - iz * -qy,
                   iy * w + iw * -qy + iz * -qx - ix * -qz,
                   iz * w + iw * -qz + ix * -qy - iy * -qx};
    return bx_math_vec_fmt(out, sz, r);
}