// m3d_math.cpp -- Will Rock 1.2 (v12.exe): small scalar solvers of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_math.obj) and checked against v12.
// v12 range 0x00565B30 .. 0x00565E70 (feb 0x00549350 .. 0x005496B3). No logic change in 1.2.

#include "m3d_geom.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00565E70 (feb 0x005496A0) -> v12 0x009AEAB8


// v12 0x00565B30  (feb 0x00549350)
// Solves a*x^2 + b*x + c = 0. Returns the number of roots (0, 1 or 2); x0 <= x1 when a > 0.
// A linear equation (|a| < eps) gives one root, a near-zero discriminant (< 10 eps) one double root.
// 1.2: same logic; only the inlined m3dSqrt SSE code was rescheduled.
int __fastcall m3dSolveQuadraticEq(float a, float b, float c, float *x0, float *x1)
{
    if (m3dFAbs(a) < M3D_EPSILON) {
        if (m3dFAbs(b) < M3D_EPSILON)
            return 0;
        *x1 = -(c / b);
        *x0 = -(c / b);
        return 1;
    }
    float d = b * b - a * c * 4.0f;
    if (d < M3D_EPSILON * -10.0f)
        return 0;
    if (d <= 0.0f)
        d = 0.0f;
    float inv2a = 1.0f / (a + a);
    if (d < M3D_EPSILON * 10.0f) {
        *x1 = -(inv2a * b);
        *x0 = -(inv2a * b);
        return 1;
    }
    float s = m3dSqrt(d);
    *x0 = (-b - s) * inv2a;
    *x1 = (s - b) * inv2a;
    return 2;
}


// v12 0x00565D20  (feb 0x00549550)
// Count-down timer: *timer -= dt. Returns 1 exactly once, on the frame the timer runs out
// (it is then clamped to 0); an already expired timer (<= eps) returns 0.
BOOL32 __fastcall m3dUpdateTimeField(float *timer, float dt)
{
    if (*timer < M3D_EPSILON + dt) {
        BOOL32 fired = M3D_EPSILON < *timer;
        *timer = 0.0f;
        return fired;
    }
    *timer -= dt;
    return 0;
}


// v12 0x00565D70  (feb 0x005495A0)
// Cramer's rule for  a*x + b*y = e,  c*x + d*y = f. Returns 0 when the determinant is ~0.
BOOL32 __fastcall m3dSolveLinSys2D(float a, float b, float c, float d, float e, float f, float *x, float *y)
{
    float det = a * d - b * c;
    if (m3dFAbs(det) < M3D_EPSILON)
        return 0;
    det = 1.0f / det;
    *x = (d * e - b * f) * det;
    *y = (a * f - c * e) * det;
    return 1;
}


// v12 0x00565E10  (feb 0x00549640)
// Moves 'cur' towards 'target' by at most 'step' and writes it to *out. Returns 1 when the target is reached.
BOOL32 __fastcall m3dLimitChange_Float(float cur, float target, float step, float *out)
{
    if (target <= cur) {
        *out = cur - step;
        if (*out <= target) {
            *out = target;
            return 1;
        }
    } else {
        *out = cur + step;
        if (target <= cur + step) {
            *out = target;
            return 1;
        }
    }
    return 0;
}
