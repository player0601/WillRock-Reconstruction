// m3d_line.cpp -- Will Rock 1.2 (v12.exe): lines and segments of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_line.obj) and checked against v12.
// v12 range 0x00574B80 .. 0x00575CE0 (feb 0x0055C5C0 .. 0x0055D740). No logic change in 1.2
// (m3dDistPointLSeg / m3dDistLSegLSeg differ only by the rescheduled inline m3dSqrt).
// Users: collision (capsule tests of characters and projectiles), AI line-of-sight, editor picking.

#include "m3d_geom.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00574EA0 (feb 0x0055C8E0) -> v12 0x009AEB08


// v12 0x00574B80  (feb 0x0055C5C0)
// 2D segment a0-a1 vs segment b0-b1. The parameter on b must be in [-eps, 1+eps] and the hit point
// inside a's bounding rectangle (+100 eps). 'hit' may be NULL.
BOOL32 __fastcall m2dIsectLSegLSeg(m2dV *a0, m2dV *a1, m2dV *b0, m2dV *b1, m2dV *hit)
{
    m2dV tmp;
    if (!hit)
        hit = &tmp;
    float dbx = b1->x - b0->x, dby = b1->y - b0->y;
    float den = dbx * (a1->y - a0->y) - dby * (a1->x - a0->x);
    if (!(M3D_EPSILON <= den || den <= -M3D_EPSILON))
        return 0;                                      // parallel
    float t = ((b0->y - a0->y) * (a1->x - a0->x) - (b0->x - a0->x) * (a1->y - a0->y)) / den;
    if (t < -M3D_EPSILON || M3D_EPSILON + 1.0f < t)
        return 0;
    hit->x = t * dbx + b0->x;
    hit->y = t * dby + b0->y;
    float tol = M3D_EPSILON * 100.0f;
    float lo  = (a0->x < a1->x) ? a0->x : a1->x;
    float hi  = (a1->x < a0->x) ? a0->x : a1->x;
    if (hit->x < lo - tol || hi + tol < hit->x)
        return 0;
    lo = (a1->y <= a0->y) ? a1->y : a0->y;
    hi = (a0->y <= a1->y) ? a1->y : a0->y;
    if (hit->y < lo - tol || hi + tol < hit->y)
        return 0;
    return 1;
}


// v12 0x00574D50  (feb 0x0055C790)
// SQUARED distance in the XZ plane from p to segment a-b; 'closest' (may be NULL) gets the point on
// the segment (its y interpolated with weight 0, i.e. = a.y on the inner part).
float __fastcall m3xzDistPointLSeg_2(m3dV *p, m3dV *a, m3dV *b, m3dV *closest)
{
    m3dV tmp;
    if (!closest)
        closest = &tmp;
    float ex = b->x - a->x, ez = b->z - a->z;
    float l2 = ex * ex + ez * ez;
    m3dV *end = a;
    if (M3D_EPSILON * 10.0f <= l2) {
        float t = ((p->x - a->x) * ex + (p->z - a->z) * ez) / l2;
        if (0.0f <= t) {
            if (t <= 1.0f) {
                closest->x = t * ex + a->x;
                closest->y = t * 0.0f + a->y;
                closest->z = t * ez + a->z;
                float dx = closest->x - p->x, dz = closest->z - p->z;
                return dx * dx + dz * dz;
            }
            end = b;
        }
    }
    *closest = *end;
    float dx = end->x - p->x, dz = end->z - p->z;
    return dx * dx + dz * dz;
}


// v12 0x00574EC0  (feb 0x0055C900)   (no automatic match; identical bytes right after this obj's _$E2)
// Distance from p to the infinite line through a and b: |(p-a) x (b-a)| / |b-a|.
float __fastcall m3dDistPointLine(m3dV *p, m3dV *a, m3dV *b)
{
    m3dV d, ap, c;
    m3dSub(b, a, &d);
    m3dSub(p, a, &ap);
    c.z = ap.x * d.y - ap.y * d.x;
    c.x = ap.y * d.z - ap.z * d.y;
    c.y = ap.z * d.x - ap.x * d.z;
    return m3dLength(&c) / m3dLength(&d);
}


// v12 0x005750C0  (feb 0x0055CB00)
// Distance from p to segment a-b. Returns 1 when it is < maxD and fills d, closest and t (0..1 on the
// segment), any of which may be NULL. A degenerate segment measures the distance to b but reports a
// as the closest point (original quirk).
// 1.2: same logic (inlined m3dSqrt rescheduled).
BOOL32 __fastcall m3dDistPointLSeg(m3dV *p, m3dV *a, m3dV *b, float maxD, float *d, m3dV *closest, float *t)
{
    m3dV e;
    m3dSub(b, a, &e);
    if (m3dFAbs(m3dLength(&e)) < M3D_EPSILON) {
        float dist = m3dDist(p, b);
        if (maxD <= dist)
            return 0;
        if (d)
            *d = dist;
        if (closest)
            *closest = *a;
        if (t)
            *t = 0.0f;
        return 1;
    }
    float l2 = m3dDot(&e, &e);
    m3dV ap;
    m3dSub(p, a, &ap);
    float s = (-1.0f / (l2 + l2)) * m3dDot(&ap, &e) * -2.0f;    // projection parameter
    if (0.0f <= s) {
        if (1.0f < s)
            s = 1.0f;
    } else {
        s = 0.0f;
    }
    m3dV c;
    m3dAddScaled(a, &e, s, &c);
    float dist = m3dDist(&c, p);
    if (maxD <= dist)
        return 0;
    if (d)
        *d = dist;
    if (closest)
        *closest = c;
    if (t)
        *t = s;
    return 1;
}


// v12 0x005754A0  (feb 0x0055CEF0)
// Distance between segments a0-a1 and b0-b1. Minimises |a0 + s*da - b0 - t*db|^2: interior solution
// of the 2x2 system, otherwise the better of the two clamped edge solutions. Returns 1 when the
// distance is < maxD (d, ca, cb filled, each may be NULL).
// 1.2: same logic (inlined m3dSqrt rescheduled).
BOOL32 __fastcall m3dDistLSegLSeg(m3dV *a0, m3dV *a1, m3dV *b0, m3dV *b1, float maxD, float *d, m3dV *ca, m3dV *cb)
{
    m3dV da, db, r;
    m3dSub(a1, a0, &da);
    if (m3dFAbs(m3dLength(&da)) < M3D_EPSILON) {       // a is a point
        if (!m3dDistPointLSeg(a0, b0, b1, maxD, d, cb, 0))
            return 0;
        if (ca)
            *ca = *a0;
        return 1;
    }
    m3dSub(b1, b0, &db);
    if (m3dFAbs(m3dLength(&db)) < M3D_EPSILON) {       // b is a point
        if (!m3dDistPointLSeg(b0, a0, a1, maxD, d, ca, 0))
            return 0;
        if (cb)
            *cb = *b0;
        return 1;
    }
    m3dSub(a0, b0, &r);
    // f(s,t) = A s^2 + C t^2 + B s t + D s + E t + F
    float A  = m3dDot(&da, &da);
    float C  = m3dDot(&db, &db);
    float B  = m3dDot(&db, &da) * -2.0f;
    float D  = m3dDot(&r, &da); D = D + D;
    float E  = m3dDot(&r, &db) * -2.0f;
    float F  = m3dDot(&r, &r);
    float C2 = C + C;
    float A2 = A + A;
    float s, t;
    if (!m3dSolveLinSys2D(A2, B, B, C2, -D, -E, &s, &t)) {   // parallel
        s = D / A2;
        t = 0.0f;
    }
    // candidate 1: s clamped to an end, best t for it
    BOOL32 sOut = 1;
    float  sEdge = 0.0f, tForS = 0.0f, f1 = 0.0f;
    if (0.0f <= s) {
        if (s <= 1.0f) {
            sOut = 0;
        } else {
            sEdge = 1.0f;
            float v = -((E + B) / C2);
            tForS = (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
            f1 = (C * tForS + E + B) * tForS + F + D + A;
        }
    } else {
        sEdge = 0.0f;
        float v = -(E / C2);
        tForS = (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
        f1 = (C * tForS + E) * tForS + F;
    }
    // candidate 2: t clamped to an end, best s for it
    BOOL32 tOut = 1;
    float  tEdge = 0.0f, sForT = 0.0f, f2 = 0.0f;
    if (0.0f <= t) {
        if (t <= 1.0f) {
            tOut = 0;
        } else {
            tEdge = 1.0f;
            float v = -((D + B) / A2);
            sForT = (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
            f2 = (sForT * A + D + B) * sForT + F + E + C;
        }
    } else {
        tEdge = 0.0f;
        float v = -(D / A2);
        sForT = (v < 0.0f) ? 0.0f : (v > 1.0f ? 1.0f : v);
        f2 = (sForT * A + D) * sForT + F;
    }
    if (sOut) {
        if (!tOut || f1 < f2) {
            s = sEdge;
            t = tForS;
        } else {
            s = sForT;
            t = tEdge;
        }
    } else if (tOut) {
        s = sForT;
        t = tEdge;
    }
    m3dV pa, pb;
    m3dAddScaled(a0, &da, s, &pa);
    m3dAddScaled(b0, &db, t, &pb);
    float dist = m3dDist(&pa, &pb);
    if (dist < maxD) {
        if (d)
            *d = dist;
        if (ca)
            *ca = pa;
        if (cb)
            *cb = pb;
        return 1;
    }
    return 0;
}


// v12 0x00575C40  (feb 0x0055D690)
// Vertical line through p vs plane (pp, pn): 0 when the plane is (almost) vertical; otherwise 1 and
// 'hit' (may be NULL) = p + Y * ((pp-p).pn / pn.y).
BOOL32 __fastcall m3dIsIsectYLinePlane(m3dV *p, m3dV *pp, m3dV *pn, m3dV *hit)
{
    if (m3dFAbs(pn->y) < M3D_EPSILON * 100.0f)
        return 0;
    if (hit) {
        float t = ((pp->y - p->y) * pn->y + (pp->x - p->x) * pn->x + (pp->z - p->z) * pn->z) / pn->y;
        hit->x = m3dVUnitY.x * t + p->x;
        hit->y = m3dVUnitY.y * t + p->y;
        hit->z = m3dVUnitY.z * t + p->z;
    }
    return 1;
}
