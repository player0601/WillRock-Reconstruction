// m3d_lerp.cpp -- Will Rock 1.2 (v12.exe): vector clamping (length / angle) and linear interpolation
//
// Reconstructed from the Feb 2003 build (m3d:m3d_lerp.obj); identical in 1.2 (only the inlined SSE
// m3dSqrt differs in register allocation).
// v12 range 0x00572720 .. 0x0057378F (feb 0x0055A140 .. 0x0055B1BF).

#include "m3d_lib.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00573770 (feb 0x0055B1A0) -> v12 0x009AEAEC (feb 0x0096379C)

static const float LERP_EPS = M3D_EPSILON;   // v12 0x00609ACC (feb 0x005DE5E0)

// clamp x into [min(0,lim), max(0,lim)] (lim may be negative)
static inline float _clampSigned(float x, float lim)
{
    float hi = lim, lo = 0.0f;
    if (lim < 0.0f) { hi = 0.0f; lo = lim; }
    float r = lo;
    if (lo <= x) {
        r = x;
        if (hi < x) r = hi;
    }
    return r;
}


// v12 0x00572720  (feb 0x0055A140)
// out = from + (to - from) limited to length maxLen. Returns 0 when the length was cut.
// HOOK: used for capped per-frame movement (turn / move speed limits).
BOOL32 __fastcall m3dClampVector_L(m3dV *from, m3dV *to, float maxLen, m3dV *out)
{
    float dx = to->x - from->x, dy = to->y - from->y, dz = to->z - from->z;
    float len = m3dSqrt(dx * dx + dy * dy + dz * dz);
    if (m3dFAbs(len) < LERP_EPS * 100.0f) {
        *out = *from;
        return 1;
    }
    float l = _clampSigned(len, maxLen);
    float k = l / len;
    out->x = dx * k + from->x;
    out->y = dy * k + from->y;
    out->z = dz * k + from->z;
    if (l < len - LERP_EPS * 100.0f)
        return 0;
    return 1;
}


// v12 0x00572900  (feb 0x0055A320)
// In-place variant: 'to' is replaced by the clamped point.
BOOL32 __fastcall m3dClampVector_L(m3dV *from, m3dV *to, float maxLen)
{
    float dx = to->x - from->x, dy = to->y - from->y, dz = to->z - from->z;
    float len = m3dSqrt(dy * dy + dz * dz + dx * dx);
    m3dV r;
    BOOL32 res = 1;
    if (m3dFAbs(len) < LERP_EPS * 100.0f) {
        r = *from;
    } else {
        float l = _clampSigned(len, maxLen);
        float k = l / len;
        r.x = dx * k + from->x;
        r.y = dy * k + from->y;
        r.z = k * dz + from->z;
        if (l < len - LERP_EPS * 100.0f)
            res = 0;
    }
    *to = r;
    return res;
}


// v12 0x00572AF0  (feb 0x0055A510)
// Turns v towards target by at most maxDeg degrees (in the plane of both); the length is
// interpolated between |v| and |target| by the fraction of the angle covered. Returns 0 when the
// turn was limited. Parameters 5 and 6 are never read (as compiled).
BOOL32 __fastcall m3dClampVector_A(m3dV *v, const m3dV *target, float maxDeg, m3dV *out, m3dV *unused5, int unused6)
{
    m3dV a = *v, b = *target;
    m3dNormalize(&a);
    m3dNormalize(&b);
    m3dV axis;
    m3dCrossProduct(&a, &b, &axis);
    float ang = m3dAngleVector(&a, &b);
    if (ang < 0.1f) {                                       // __real@3dcccccd
        *out = *target;
        return 1;
    }
    (void)m3dSqrt(axis.x * axis.x + axis.y * axis.y + axis.z * axis.z);  // computed and dropped
    float rot = _clampSigned(ang, maxDeg);
    m3dNormalize(&axis);
    m3dV c;
    m3dCrossProduct(&a, &axis, &c);

    m3dMATR w2l, rotY, l2w;
    w2l.MakeWCS2LCS(&m3dVZero, &a, &axis, &c);              // local frame: x = a, y = rotation axis
    rotY.MakeRotY(rot);
    m3dV d;
    m3dTransformVector(&m3dVUnitX, &rotY, &d);
    w2l.Invert(&l2w);
    m3dTransformVector(&d, &l2w, out);

    float lenT = m3dSqrt(target->z * target->z + target->x * target->x + target->y * target->y);
    float lenV = m3dSqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    float t0 = 0.0f, t1 = ang, l0 = lenV, l1 = lenT;
    if (ang < 0.0f) { t1 = 0.0f; t0 = ang; l0 = lenT; l1 = lenV; }
    float k = ((rot - t0) * (l1 - l0)) / (t1 - t0) + l0;
    out->x = k * out->x;
    out->y = k * out->y;
    out->z = k * out->z;
    m3dAngleVector(v, out);                                 // result unused (debug leftover)
    if (rot < ang - LERP_EPS * 100.0f)
        return 0;
    return 1;
}


// v12 0x005731A0  (feb 0x0055ABD0)
// Per-component linear interpolation: value a at t0, b at t1, evaluated at t (extrapolates).
void __fastcall m3dLerpVector_L(m3dV *a, m3dV *b, float t0, float t1, float t, m3dV *out)
{
    const float *pa = &a->x, *pb = &b->x;
    float *po = &out->x;
    for (int i = 0; i < 3; i++) {
        float lo = t0, hi = t1, va = pa[i], vb = pb[i];
        if (t1 < t0) { lo = t1; hi = t0; va = pb[i]; vb = pa[i]; }
        po[i] = ((t - lo) * (vb - va)) / (hi - lo) + va;
    }
}


// 1D linear interpolation with the endpoint swap used throughout m3dLerpRectCorners
static inline float _lerpSorted(float x, float x0, float x1, float v0, float v1)
{
    if (x1 < x0) { float t = x0; x0 = x1; x1 = t; t = v0; v0 = v1; v1 = t; }
    return v0 + ((x - x0) * (v1 - v0)) / (x1 - x0);
}


// v12 0x005732F0  (feb 0x0055AD20)
// Bilinear interpolation inside the rectangle c0..c1 of the corner values
//   v00 at (c0.x, c0.y), v01 at (c0.x, c1.y), v10 at (c1.x, c0.y), v11 at (c1.x, c1.y),
// averaged over the two evaluation orders (x then y, y then x). Degenerate sides use the mean.
float __fastcall m3dLerpRectCorners(m2dV *c0, m2dV *c1, m2dV *p, float v00, float v01, float v10, float v11)
{
    bool degX = m3dFAbs(c1->x - c0->x) < LERP_EPS;
    bool degY = m3dFAbs(c1->y - c0->y) < LERP_EPS;
    // x first
    float a = degX ? (v00 + v10) * 0.5f : _lerpSorted(p->x, c0->x, c1->x, v00, v10);
    float b = degX ? (v01 + v11) * 0.5f : _lerpSorted(p->x, c0->x, c1->x, v01, v11);
    float xy = degY ? (b + a) * 0.5f : _lerpSorted(p->y, c0->y, c1->y, a, b);
    // y first
    float d = degY ? (v00 + v01) * 0.5f : _lerpSorted(p->y, c0->y, c1->y, v00, v01);
    float e = degY ? (v10 + v11) * 0.5f : _lerpSorted(p->y, c0->y, c1->y, v10, v11);
    if (degX)
        return ((e + d) * 0.5f + xy) * 0.5f;
    return (_lerpSorted(p->x, c0->x, c1->x, d, e) + xy) * 0.5f;
}
