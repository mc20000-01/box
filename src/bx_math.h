/* BX MATH - general purpose math: integer extras, real analysis, and a small
 * 3D toolkit (vec3, 4x4 matrices, quaternions).
 *
 * The command layer (high.math.* / high.m3d.*) renders small fixed buffers,
 * so vector/matrix operations share one text format: numbers separated by a
 * single space ("x y z" or the 16 values of a 4x4 matrix in row-major order).
 * All angles given to the 3D helpers are in degrees.
 */
#ifndef BX_MATH_H
#define BX_MATH_H

#include <stddef.h>

/* integer extras ------------------------------------------------------------ */

long long bx_math_gcd(long long a, long long b);
long long bx_math_lcm(long long a, long long b);
long long bx_math_fact(int n);
int       bx_math_isprime(long long n);
long long bx_math_nthprime(int n);
long long bx_math_ncr(long long n, long long r);
long long bx_math_npr(long long n, long long r);
long long bx_math_fib(int n);
long long bx_math_isqrt(long long x);
long long bx_math_totient(long long n);
long long bx_math_divcount(long long n);
long long bx_math_divsum(long long n);
long long bx_math_digitsum(long long n);
long long bx_math_droot(long long n);
long long bx_math_collatz(long long n);

/* prime factorization: fills out with space-separated factors, e.g. "2 2 3".
 * Returns the number of factors written. */
int bx_math_factor(long long n, char *out, size_t outsz);

/* decimal fraction: writes the reduced numerator/denominator of v (v>0). */
int bx_math_frac(double v, long long *num, long long *den);

/* 3D: vec3 ------------------------------------------------------------------ */

int bx_math_vec_parse(const char *s, double out[3]);      /* components read */
int bx_math_vec_fmt(char *out, size_t sz, const double v[3]);
int bx_math_vec_len(const char *s, double *len);
int bx_math_vec_norm(const char *s, char *out, size_t sz);
int bx_math_vec_add(const char *a, const char *b, char *out, size_t sz);
int bx_math_vec_sub(const char *a, const char *b, char *out, size_t sz);
int bx_math_vec_scale(const char *a, double s, char *out, size_t sz);
int bx_math_vec_dot(const char *a, const char *b, double *d);
int bx_math_vec_cross(const char *a, const char *b, char *out, size_t sz);

/* 3D: 4x4 matrices (row-major double[16]) ----------------------------------- */

int bx_math_mat_parse(const char *s, double m[16]);
int bx_math_mat_fmt(char *out, size_t sz, const double m[16]);
int bx_math_mat_mul(const char *a, const char *b, char *out, size_t sz);
int bx_math_mat_vec(const char *m, const char *v, char *out, size_t sz);
int bx_math_mat_identity(char *out, size_t sz);
int bx_math_mat_translate(const char *xyz, char *out, size_t sz);
int bx_math_mat_scale(const char *xyz, char *out, size_t sz);
int bx_math_mat_rotx(double deg, char *out, size_t sz);
int bx_math_mat_roty(double deg, char *out, size_t sz);
int bx_math_mat_rotz(double deg, char *out, size_t sz);
int bx_math_mat_rot(const char *axis, double deg, char *out, size_t sz);

/* 3D: quaternions "w x y z" -------------------------------------------------- */

int bx_math_quat_axis(double x, double y, double z, double deg, char *out, size_t sz);
int bx_math_quat_mul(const char *a, const char *b, char *out, size_t sz);
int bx_math_quat_conj(const char *q, char *out, size_t sz);
int bx_math_quat_norm(const char *q, char *out, size_t sz);
int bx_math_quat_rot(const char *q, const char *v, char *out, size_t sz);

/* Cleanup a float before printing: kills tiny noise and negative zero. */
double bx_math_fpclean(double v);

#endif