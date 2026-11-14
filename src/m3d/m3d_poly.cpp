// m3d_poly.cpp -- Will Rock 1.2 (v12.exe): polygons and planes of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_poly.obj) and checked against v12.
// v12 range 0x00570010 .. 0x00572240 (feb 0x00557F00 .. 0x0055A140). No logic change in 1.2
// (m3dCalcPolyArea, m3dDistPointPoly, m3dIsectLSegPlane, m3dIsectLSegPoly differ only by the
// rescheduled inline m3dSqrt / inline point transform).
// Users: collision against level faces (cdt*), decals, AI navigation, editor picking.
//
// Polygon conventions: m3dPOLY holds up to 20 vertices; the face normal is (v1-v0) x (v2-v1),
// normalised; "front" is the side the normal points to.

#include "m3d_geom.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00570380 (feb 0x00558270) -> v12 0x009AEAE4


// v12 0x00570010  (feb 0x00557F00)
// Edge i of the polygon: a = v[i], b = v[(i+1) % n].
void __fastcall m3dGetPolyEdge(m3dPOLY *p, int i, m3dV *a, m3dV *b)
{
    *a = p->vert[i];
    *b = p->vert[(i + 1) % p->nVert];
}


// v12 0x00570060  (feb 0x00557F50)
// Projects the polygon onto the ground plane: (x, z) -> (x, y).
void __fastcall m3xzPoly3DToPoly2D(m3dPOLY *p, m2dPOLY *out)
{
    for (int i = 0; i < p->nVert; i++) {
        out->vert[i].x = p->vert[i].x;
        out->vert[i].y = p->vert[i].z;
    }
    out->nVert = p->nVert;
}


// v12 0x005700A0  (feb 0x00557F90)
// Unit face normal from the first three vertices.
void __fastcall m3dGetPolyNormal(m3dPOLY *p, m3dV *n)
{
    m3dV e1, e2;
    m3dSub(&p->vert[1], &p->vert[0], &e1);
    m3dSub(&p->vert[2], &p->vert[1], &e2);
    m3dCross(&e1, &e2, n);
    m3dNormalize(n);
}


// v12 0x00570110  (feb 0x00558000)
// Same as m3dGetPolyNormal; returns 0 for a degenerate polygon (out-of-line _m3dNormalize).
BOOL32 __fastcall _m3dGetPolyNormal(m3dPOLY *p, m3dV *n)
{
    m3dV e1, e2;
    m3dSub(&p->vert[1], &p->vert[0], &e1);
    m3dSub(&p->vert[2], &p->vert[1], &e2);
    m3dCross(&e1, &e2, n);
    return _m3dNormalize(n) != 0;
}


// v12 0x00570180  (feb 0x00558070)
// Unit normal of the plane through a, b, c: (b-a) x (c-b).
void __fastcall m3dGetPlaneNormal(m3dV *a, m3dV *b, m3dV *c, m3dV *n)
{
    m3dV e1, e2;
    m3dSub(b, a, &e1);
    m3dSub(c, b, &e2);
    m3dCross(&e1, &e2, n);
    m3dNormalize(n);
}


// v12 0x00570200  (feb 0x005580F0)
// Unnormalised plane normal (b-a) x (c-a). SSE / 3DNow! / x87 variants in the original (same result).
void __cdecl m3dGetPlanePerpVector(m3dV *a, m3dV *b, m3dV *c, m3dV *out)
{
    m3dV e1, e2;
    m3dSub(b, a, &e1);
    m3dSub(c, a, &e2);
    m3dCross(&e1, &e2, out);
}


// v12 0x005703A0  (feb 0x00558290)
// Squared area of the triangle a, b, c: |(b-a) x (c-a)|^2 / 4.
float __fastcall m3dCalcTriArea2(m3dV *a, m3dV *b, m3dV *c)
{
    m3dV e1, e2, x;
    m3dSub(b, a, &e1);
    m3dSub(c, a, &e2);
    m3dCross(&e1, &e2, &x);
    return m3dDot(&x, &x) * 0.25f;
}


// v12 0x00570420  (feb 0x00558310)
// Area of a convex polygon (triangle fan from v0).
// 1.2: same logic (inlined m3dSqrt rescheduled).
float __fastcall m3dCalcPolyArea(m3dPOLY *p)
{
    float area = 0.0f;
    for (int i = 1; i <= p->nVert - 2; i++) {
        m3dV e1, e2, x;
        m3dSub(&p->vert[i], &p->vert[0], &e1);
        m3dSub(&p->vert[i + 1], &p->vert[0], &e2);
        m3dCross(&e1, &e2, &x);
        area += m3dSqrt(m3dDot(&x, &x) * 0.25f);
    }
    return area;
}


// v12 0x00570570  (feb 0x00558460)
// Reverses the winding of a 2D polygon, keeping vertex 1 in place: new[i] = old[(n + 2 - i) % n]
// (order 2,1,0,n-1,...,3).
void __fastcall m2dReOrientPoly(m2dPOLY *p)
{
    m2dPOLY tmp = *p;
    int n = tmp.nVert;
    for (int i = 0; i < n; i++) {
        if (i == 0)
            tmp.vert[0] = p->vert[2];
        else if (i == 1)
            tmp.vert[1] = p->vert[1];
        else if (i == 2)
            tmp.vert[2] = p->vert[0];
        else
            tmp.vert[i] = p->vert[n - i + 2];
    }
    *p = tmp;
}


// v12 0x00570610  (feb 0x00558500)
// 3D version of m2dReOrientPoly.
void __fastcall m3dReOrientPoly(m3dPOLY *p)
{
    m3dPOLY tmp = *p;
    int n = p->nVert;
    for (int i = 0; i < n; i++) {
        if (i == 0)
            tmp.vert[0] = p->vert[2];
        else if (i == 1)
            tmp.vert[1] = p->vert[1];
        else if (i == 2)
            tmp.vert[2] = p->vert[0];
        else
            tmp.vert[i] = p->vert[n - i + 2];
    }
    *p = tmp;
}


// v12 0x005706E0  (feb 0x005585D0)
// 1 when every vertex turn is counter-clockwise (no vertex right of the previous edge, 10 eps).
BOOL32 __fastcall m2dIsPolyConvexCCW(m2dPOLY *p)
{
    for (int i = 0; i < p->nVert; i++) {
        int n = p->nVert;
        if (m2dQualPointLine_A(&p->vert[(i + 2) % n], &p->vert[i % n], &p->vert[(i + 1) % n],
                               M3D_EPSILON * 10.0f) == 1)
            return 0;
    }
    return 1;
}


// v12 0x00570750  (feb 0x00558640)
// One-sided ray vs the polygon's plane (only rays hitting the FRONT face, dir.n < -eps, and only
// forward: t >= 0). 'n' may be NULL (computed from the polygon); hit and t may be NULL.
BOOL32 __fastcall m3dIsectRayPlane(m3dV *org, m3dV *dir, m3dPOLY *p, m3dV *hit, m3dV *n, float *t)
{
    m3dV nn;
    if (!n) {
        n = &nn;
        m3dV e1, e2;
        m3dSub(&p->vert[1], &p->vert[0], &e1);
        m3dSub(&p->vert[2], &p->vert[1], &e2);
        m3dCross(&e1, &e2, &nn);
        m3dNormalize(&nn);
    }
    float den = m3dDot(dir, n);
    if (-M3D_EPSILON < den)
        return 0;
    float s = (m3dDot(org, n) + -m3dDot(&p->vert[0], n)) / den;      // = -t
    if (0.0f < s)
        return 0;
    if (hit) {
        hit->x = org->x - s * dir->x;
        hit->y = org->y - s * dir->y;
        hit->z = org->z - s * dir->z;
    }
    if (t)
        *t = -s;
    return 1;
}


// v12 0x005708C0  (feb 0x005587B0)
// One-sided ray vs plane (point pp, unit normal pn): front face only, forward only.
BOOL32 __fastcall m3dIsectRayVNPlane(m3dV *org, m3dV *dir, m3dV *pp, m3dV *pn, m3dV *hit, float *t)
{
    float den = m3dDot(pn, dir);
    if (-M3D_EPSILON < den)
        return 0;
    float s = (m3dDot(org, pn) + -m3dDot(pp, pn)) / den;
    if (0.0f < s)
        return 0;
    if (hit) {
        hit->x = org->x - s * dir->x;
        hit->y = org->y - s * dir->y;
        hit->z = org->z - s * dir->z;
    }
    if (t)
        *t = -s;
    return 1;
}


// v12 0x00570990  (feb 0x00558880)
// Two-sided ray vs plane: any |dir.n| >= eps, forward only.
BOOL32 __fastcall _m3dIsectRayVNPlane(m3dV *org, m3dV *dir, m3dV *pp, m3dV *pn, m3dV *hit, float *t)
{
    float den = m3dDot(pn, dir);
    if (m3dFAbs(den) < M3D_EPSILON)
        return 0;
    float s = (m3dDot(org, pn) + -m3dDot(pp, pn)) / den;
    if (0.0f < s)
        return 0;
    if (hit) {
        hit->x = -s * dir->x + org->x;
        hit->y = -s * dir->y + org->y;
        hit->z = -s * dir->z + org->z;
    }
    if (t)
        *t = -s;
    return 1;
}


// v12 0x00570A90  (feb 0x00558980)
// Distance from p to the polygon (unit normal n). Only points within maxD of the plane are
// considered, and only in front of it unless 'twoSided'. Works in the polygon's local frame
// (x = first edge, y = normal): a point above the polygon gets its plane distance, others the
// distance to the nearest edge (accepted up to maxD + 5 eps). Degenerate polygons fall back to a
// segment / point. Returns 1 with d and closest (each may be NULL).
// 1.2: same logic (inline sqrt / point transform rescheduled).
BOOL32 __fastcall m3dDistPointPoly(m3dV *p, m3dPOLY *poly, m3dV *n, int twoSided, m3dV *closest, float *d, float maxD)
{
    m3dV *v0 = &poly->vert[0];
    m3dV  rel;
    m3dSub(p, v0, &rel);
    float h = m3dDot(&rel, n);
    if (!(h <= maxD && -maxD <= h && (twoSided || -M3D_EPSILON <= h)))
        return 0;

    m3dV ex;
    m3dSub(&poly->vert[1], v0, &ex);
    if (!_m3dNormalize(&ex)) {
        m3dSub(&poly->vert[2], v0, &ex);
        if (_m3dNormalize(&ex))
            return m3dDistPointLSeg(p, v0, &poly->vert[2], maxD, d, closest, 0);
        float dist = m3dDist(v0, p);
        if (dist < maxD) {
            if (d)
                *d = dist;
            if (closest)
                *closest = *v0;
            return 1;
        }
        return 0;
    }

    m3dV ez;
    m3dCross(&ex, n, &ez);
    m3dMATR w2l, l2w;
    w2l.MakeWCS2LCS(v0, &ex, n, &ez);
    m3dV lp;
    m3dTransformPoint(p, &w2l, &lp);
    m3dPOLY lpoly;
    for (int i = 0; i < poly->nVert; i++)
        m3dTransformPoint(&poly->vert[i], &w2l, &lpoly.vert[i]);
    lpoly.nVert = poly->nVert;

    if (m3xzIsBelongPointPoly_A(&lpoly, &lp, M3D_EPSILON)) {        // straight above the polygon
        if (d)
            *d = m3dFAbs(lp.y);
        if (closest) {
            lp.y = 0.0f;
            l2w.MakeLCS2WCS(v0, &ex, n, &ez);
            m3dTransformPoint(&lp, &l2w, closest);
        }
        return 1;
    }
    m3dV c, cur;
    float best = m3xzDistPointLSeg_2(&lp, &lpoly.vert[lpoly.nVert - 1], &lpoly.vert[0], &c);
    for (int i = 0; i < poly->nVert - 1; i++) {
        float dd = m3xzDistPointLSeg_2(&lp, &lpoly.vert[i], &lpoly.vert[i + 1], &cur);
        if (dd < best) {
            c    = cur;
            best = dd;
        }
    }
    if (best <= maxD * maxD) {
        float dist = m3dDist(&lp, &c);
        if (dist <= M3D_EPSILON * 5.0f + maxD) {
            if (d)
                *d = dist;
            if (closest) {
                l2w.MakeLCS2WCS(v0, &ex, n, &ez);
                m3dTransformPoint(&c, &l2w, closest);
            }
            return 1;
        }
    }
    return 0;
}


// v12 0x00571520  (feb 0x00559420)
// Distance between segment a-b and the polygon. Rejects segments entirely beyond maxD in front of
// the plane or deeper than 0.05 behind it. A crossing gives distance 0 (where = -1). Otherwise the
// best of: every edge vs the segment (where = edge index), a above the polygon, b above the polygon
// (where = -1). ca = point on the segment, cp = point on the polygon. NB ca/cp/d/where must be valid.
BOOL32 __fastcall m3dDistLSegPoly(m3dV *a, m3dV *b, m3dPOLY *p, m3dV *n, m3dV *ca, m3dV *cp, float *d, int *where, float maxD)
{
    m3dV *v0 = &p->vert[0];
    float ha = (a->y - v0->y) * n->y + (a->z - v0->z) * n->z + (a->x - v0->x) * n->x;
    float hb = (b->y - v0->y) * n->y + (b->z - v0->z) * n->z + (b->x - v0->x) * n->x;
    if (!((ha <= maxD || hb <= maxD) && (-0.05f <= ha || -0.05f <= hb)))
        return 0;
    m3dV h;
    if (m3dIsectLSegPoly(a, b, p, n, &h)) {
        *cp = h;
        *ca = h;
        *d = 0.0f;
        *where = -1;
        return 1;
    }
    BOOL32 found = 0;
    m3dV  bestA, bestP, sa, sp;
    float dd;
    for (int i = 0; i < p->nVert; i++) {
        if (m3dDistLSegLSeg(a, b, &p->vert[i], &p->vert[(i + 1) % p->nVert], maxD, &dd, &sa, &sp)) {
            bestA  = sa;
            bestP  = sp;
            maxD   = dd;
            found  = 1;
            *where = i;
        }
    }
    if (m3dDistPointPoly(a, p, n, 1, &bestP, &dd, maxD)) {
        bestA  = *a;
        maxD   = dd;
        found  = 1;
        *where = -1;
    }
    if (m3dDistPointPoly(b, p, n, 1, &bestP, &dd, maxD)) {
        bestA  = *b;
        found  = 1;
        *where = -1;
    } else {
        dd = maxD;
        if (!found)
            return 0;
    }
    *ca = bestA;
    *cp = bestP;
    *d  = dd;
    return found;
}


// v12 0x005717D0  (feb 0x005596D0)
// Distance between segment a-b and the plane (pp, unit pn), front side only. Crossing -> 0.
// Otherwise the nearer end point (ca) and its projection on the plane (cp) when within maxD.
BOOL32 __fastcall m3dDistLSegPlane(m3dV *a, m3dV *b, m3dV *pp, m3dV *pn, m3dV *ca, m3dV *cp, float *d, float maxD)
{
    m3dV ra, rb;
    m3dSub(a, pp, &ra);
    m3dSub(b, pp, &rb);
    float ha = m3dDot(&ra, pn);
    float hb = m3dDot(&rb, pn);
    if ((maxD < ha && maxD < hb) || (ha < 0.0f && hb < 0.0f))
        return 0;
    m3dV h;
    if (m3dIsectLSegPlane(a, b, pp, pn, &h)) {
        *cp = h;
        *ca = h;
        *d = 0.0f;
        return 1;
    }
    float m = (ha < hb) ? ha : hb;
    if (m <= maxD) {
        if (hb <= ha) {
            *ca = *b;
            m3dAddScaled(b, pn, -m3dDot(&rb, pn), cp);
            *d = hb;
            return 1;
        }
        *ca = *a;
        m3dAddScaled(a, pn, -m3dDot(&ra, pn), cp);
        *d = ha;
        return 1;
    }
    return 0;
}


// v12 0x00571A20  (feb 0x00559920)
// Segment a-b vs plane (pp, unit pn), two-sided. A degenerate segment hits when a lies on the plane
// (100 eps). 'hit' may be NULL.
// 1.2: same logic (inlined m3dSqrt rescheduled).
BOOL32 __fastcall m3dIsectLSegPlane(m3dV *a, m3dV *b, m3dV *pp, m3dV *pn, m3dV *hit)
{
    m3dV e;
    m3dSub(b, a, &e);
    float len = m3dLength(&e);
    if (m3dFAbs(len) < M3D_EPSILON) {
        m3dV r;
        m3dSub(a, pp, &r);
        if (!(m3dFAbs(m3dDot(&r, pn)) < M3D_EPSILON * 100.0f))
            return 0;
        if (hit)
            *hit = *a;
        return 1;
    }
    float inv = 1.0f / len;
    float den = e.y * inv * pn->y + inv * e.z * pn->z + e.x * inv * pn->x;
    if (m3dFAbs(den) < M3D_EPSILON)
        return 0;
    float t = -((m3dDot(a, pn) + -m3dDot(pp, pn)) / den);        // distance along the segment
    if (t < 0.0f || len < t)
        return 0;
    if (hit) {
        hit->x = e.x * inv * t + a->x;
        hit->y = e.y * inv * t + a->y;
        hit->z = inv * e.z * t + a->z;
    }
    return 1;
}


// v12 0x00571D30  (feb 0x00559C30)
// Segment a-b vs polygon (unit normal n), two-sided. A degenerate segment hits when a lies in the
// polygon's plane (100 eps, plane normal recomputed) and inside it. 'hit' may be NULL.
// 1.2: same logic (inlined m3dSqrt rescheduled).
BOOL32 __fastcall m3dIsectLSegPoly(m3dV *a, m3dV *b, m3dPOLY *p, m3dV *n, m3dV *hit)
{
    m3dV e;
    m3dSub(b, a, &e);
    float len = m3dLength(&e);
    if (m3dFAbs(len) < M3D_EPSILON) {
        m3dV pn, e1, e2, r;
        m3dSub(&p->vert[1], &p->vert[0], &e1);
        m3dSub(&p->vert[2], &p->vert[1], &e2);
        m3dCross(&e1, &e2, &pn);
        m3dNormalize(&pn);
        m3dSub(a, &p->vert[0], &r);
        if (!(m3dFAbs(m3dDot(&pn, &r)) < M3D_EPSILON * 100.0f))
            return 0;
        if (!m3dIsBelongPointPoly_A(p, a, n, M3D_EPSILON))
            return 0;
        if (hit)
            *hit = *a;
        return 1;
    }
    float inv = 1.0f / len;
    m3dV dir;
    m3dScale(&e, inv, &dir);
    float den = m3dDot(&dir, n);
    if (m3dFAbs(den) < M3D_EPSILON)
        return 0;
    float t = -((m3dDot(a, n) + -m3dDot(n, &p->vert[0])) / den);
    if (t < 0.0f || len < t)
        return 0;
    m3dV h;
    m3dAddScaled(a, &dir, t, &h);
    if (!m3dIsBelongPointPoly_A(p, &h, n, M3D_EPSILON))
        return 0;
    if (hit)
        *hit = h;
    return 1;
}


// v12 0x00572100  (feb 0x0055A000)
// Point of a 2D triangle from basis coordinates: v0 + (v1-v0)*u + (v2-v0)*v.
void __fastcall m2dTriApplyBasisCoord(m2dTRI *t, float u, float v, m2dV *out)
{
    out->x = (t->v[1].x - t->v[0].x) * u + (t->v[2].x - t->v[0].x) * v;
    out->y = (t->v[2].y - t->v[0].y) * v + (t->v[1].y - t->v[0].y) * u;
    out->x = out->x + t->v[0].x;
    out->y = out->y + t->v[0].y;
}


// v12 0x00572160  (feb 0x0055A060)
// Point of a polygon's first triangle from basis coordinates: v0 + (v1-v0)*u + (v2-v0)*v.
void __fastcall m3dPolyApplyBasisCoord(m3dPOLY *p, float u, float v, m3dV *out)
{
    m3dV *v0 = &p->vert[0], *v1 = &p->vert[1], *v2 = &p->vert[2];
    out->x = (v1->x - v0->x) * u + (v2->x - v0->x) * v;
    out->y = (v1->y - v0->y) * u + (v2->y - v0->y) * v;
    out->z = (v1->z - v0->z) * u + (v2->z - v0->z) * v;
    out->x = out->x + v0->x;
    out->y = out->y + v0->y;
    out->z = out->z + v0->z;
}


// v12 0x005721F0  (feb 0x0055A0F0)
// 1 when segment a-b crosses any edge of the 2D polygon (closing edge first). 'eps' is unused.
BOOL32 __fastcall m2dIsCrossLSegPolyBoundary_A(m2dPOLY *p, m2dV *a, m2dV *b, float eps)
{
    m2dV *prev = &p->vert[p->nVert - 1];
    for (int i = 0; i < p->nVert; i++) {
        if (m2dIsectLSegLSeg(prev, &p->vert[i], a, b, 0))
            return 1;
        prev = &p->vert[i];
    }
    return 0;
}
