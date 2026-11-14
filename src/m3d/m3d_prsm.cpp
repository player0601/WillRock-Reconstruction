// m3d_prsm.cpp -- Will Rock 1.2 (v12.exe): vertical prism volume (m3dPRISM) of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_prsm.obj) and checked against v12.
// v12 range 0x00575D00 .. 0x00577AF0 (feb 0x0055D750 .. 0x0055F7A0); vtable v12 0x00607110
// (feb 0x005DBC24). No logic change in 1.2 (several functions differ only by the rescheduled inline
// m3dSqrt); QualPlane stays the m3dVOL default (enclosing sphere).
//
// In practice a prism is a RECTANGLE in the ground plane (base.nVert == 4, see Check) extruded from
// yMin to yMax, i.e. a box rotated about Y only. Calc fits the smallest such box to a point cloud by
// trying every whole degree 0..89. Vertex order of GetVertices: 0..3 = base at yMin, 4..7 = at yMax.

#include "m3d_geom.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x005770A0 (feb 0x0055EB20) -> v12 0x009AEB0C

#define PRISM_Y_TOL   0.001f       // __real@3a83126f
#define PRISM_XZ_TOL  0.2f         // 0x3E4CCCCD, tolerance of the 2D point-in-polygon test
#define PRISM_LEN_TOL 0.005f       // __real@3ba3d70a


// v12 0x00575D00  (feb 0x0055D750)  vtable +0x04
// Inside the y range (+-0.001) and inside the XZ outline (0.2 tolerance).
BOOL32 m3dPRISM::IsBelongPoint(m3dV *p)
{
    if (yMin - PRISM_Y_TOL <= p->y && p->y <= yMax + PRISM_Y_TOL) {
        m2dV q = { p->x, p->z };
        return m2dIsBelongPointPoly_A(&base, &q, PRISM_XZ_TOL);
    }
    return 0;
}


// v12 0x00575D70  (feb 0x0055D7C0)  vtable +0x08
// Swept sphere (org -> org + dir*len, radius r): start inside, or the path within r of any face.
// NB the original passes a NULL normal and NULL outputs to m3dDistLSegPoly, which dereferences the
// normal: this override cannot work for a point outside the prism (unused by the game ?).
BOOL32 m3dPRISM::IsIsectCyl(m3dV *org, m3dV *dir, float len, float r)
{
    if (IsBelongPoint(org))
        return 1;
    m3dPOLY face[6];
    for (int i = 0; i < 6; i++)
        GetFace(i, &face[i], 0);
    m3dV end;
    m3dAddScaled(org, dir, len, &end);
    for (int i = 0; i < 6; i++)
        if (m3dDistLSegPoly(org, &end, &face[i], 0, 0, 0, &r, 0, r))
            return 1;
    return 0;
}


// v12 0x00575E40  (feb 0x0055D890)  vtable +0x0C
// Ray cast against the 6 faces (front faces only, see m3dIsectRayPlane). 2 when org is inside
// (hit = org, *t = 0). On a hit: hit, *t = distance, *face = face index (t and face are written only
// when 'hit' is given). maxT is NOT checked.
BOOL32 m3dPRISM::IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face)
{
    if (IsBelongPoint(org)) {
        if (hit)
            *hit = *org;
        if (t)
            *t = 0.0f;
        return M3D_ISECT_INSIDE;
    }
    m3dPOLY f[6];
    for (int i = 0; i < 6; i++)
        GetFace(i, &f[i], 0);
    for (int i = 0; i < 6; i++) {
        m3dV h;
        if (m3dIsectRayPlane(org, dir, &f[i], &h, 0, 0) && m3dIsBelongPointPoly_A(&f[i], &h, 0, M3D_EPSILON)) {
            if (hit) {
                *hit  = h;
                *t    = m3dDist(&h, org);
                *face = i;
            }
            return M3D_ISECT_HIT;
        }
    }
    return 0;
}


// v12 0x00576010  (feb 0x0055DA70)  vtable +0x10
// Distance from an outside point to the nearest face (front side only), if below maxD.
// 2 when p is inside.
BOOL32 m3dPRISM::DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face)
{
    if (IsBelongPoint(p))
        return M3D_ISECT_INSIDE;
    m3dPOLY f[6];
    for (int i = 0; i < 6; i++)
        GetFace(i, &f[i], 0);
    BOOL32 found = 0;
    float  best  = maxD;
    int    bestFace = 0;
    m3dV   bestC = { 0, 0, 0 };
    for (int i = 0; i < 6; i++) {
        m3dV n, c;
        float dd;
        m3dGetPolyNormal(&f[i], &n);
        if (m3dDistPointPoly(p, &f[i], &n, 0, &c, &dd, M3D_INFINITY) && dd < best) {
            best     = dd;
            bestC    = c;
            bestFace = i;
            found    = 1;
        }
    }
    if (!found)
        return 0;
    if (d)
        *d = best;
    if (closest)
        *closest = bestC;
    if (face)
        *face = bestFace;
    return 1;
}


// v12 0x00576150  (feb 0x0055DBB0)  vtable +0x18
// Valid prism: convex CCW outline, yMin < yMax, and the outline is a rectangle (opposite edges of
// equal length within 0.005, every edge >= 10 eps, right angles within 1000 eps).
// 1.2: same logic (inlined m3dSqrt rescheduled).
BOOL32 m3dPRISM::Check()
{
    if (!m2dIsPolyConvexCCW(&base))
        return 0;
    if (yMin > yMax - M3D_EPSILON)
        return 0;
    m2dV e[4];
    for (int i = 0; i < 4; i++) {
        e[i].x = base.vert[(i + 1) & 3].x - base.vert[i].x;
        e[i].y = base.vert[(i + 1) & 3].y - base.vert[i].y;
    }
    float l[4];
    for (int i = 0; i < 4; i++)
        l[i] = m3dSqrt(e[i].x * e[i].x + e[i].y * e[i].y);
    if (!(m3dFAbs(l[0] - l[2]) < PRISM_LEN_TOL))
        return 0;
    if (!(m3dFAbs(l[1] - l[3]) < PRISM_LEN_TOL))
        return 0;
    for (int i = 0; i < 4; i++) {
        if (l[i] < M3D_EPSILON * 10.0f)
            return 0;
        if (M3D_EPSILON <= m3dFAbs(l[i])) {
            e[i].x *= 1.0f / l[i];
            e[i].y *= 1.0f / l[i];
        }
    }
    float tol = M3D_EPSILON * 1000.0f;
    if (!(m3dFAbs(e[1].x * e[0].x + e[1].y * e[0].y) < tol)) return 0;
    if (!(m3dFAbs(e[2].x * e[1].x + e[2].y * e[1].y) < tol)) return 0;
    if (!(m3dFAbs(e[3].x * e[2].x + e[3].y * e[2].y) < tol)) return 0;
    if (!(m3dFAbs(e[3].x * e[0].x + e[3].y * e[0].y) < tol)) return 0;
    return 1;
}


// v12 0x00576C30  (feb 0x0055E6B0)  vtable +0x1C
// Enclosing sphere: centre = GetCenter, radius = HALF the distance from the centre to the corner
// (v0.x, yMin, v0.z). Together with the GetCenter quirk below the sphere does not really enclose the
// prism (original behaviour; prisms rarely use it). Possible fix point for mods.
void m3dPRISM::CalcEnclSph(m3dSPHERE *out)
{
    m3dV corner = { base.vert[0].x, yMin, base.vert[0].y };
    GetCenter(&out->center);
    out->radius = m3dDist(&corner, &out->center) * 0.5f;
}


// v12 0x00576D30  (feb 0x0055E7B0)  vtable +0x20
// All 8 corners inside the sphere grown by 100 eps.
BOOL32 m3dPRISM::CheckEnclSph(m3dSPHERE *s)
{
    m3dSPHERE grown;                                   // stack sphere with the m3dSPHERE vtable
    grown.type   = s->type;
    grown.center = s->center;
    grown.radius = M3D_EPSILON * 100.0f + s->radius;
    m3dV v[8];
    GetVertices(v);
    for (int i = 0; i < 8; i++)
        if (!grown.m3dSPHERE::IsBelongPoint(&v[i]))
            return 0;
    return 1;
}


// v12 0x00576DC0  (feb 0x0055E840)  vtable +0x24
// Centre of the base rectangle (v0 + v2)/2. NB y = (yMin + yMin)/2 = yMin in the original (the
// author surely meant yMax for the second term).
void m3dPRISM::GetCenter(m3dV *out)
{
    out->x = (base.vert[2].x + base.vert[0].x) * 0.5f;
    out->y = (yMin + yMin) * 0.5f;                     // original quirk
    out->z = (base.vert[2].y + base.vert[0].y) * 0.5f;
}


// v12 0x00576E20  (feb 0x0055E8A0)  vtable +0x28
// Face i (0..3 sides, 4 bottom, 5 top) as a 4-vertex polygon; 'normal' is not filled.
void m3dPRISM::GetFace(int i, m3dPOLY *out, m3dV *normal)
{
    static const unsigned char idx[6][4] = {
        { 3, 7, 4, 0 }, { 2, 6, 7, 3 }, { 5, 6, 2, 1 }, { 4, 5, 1, 0 }, { 0, 1, 2, 3 }, { 7, 6, 5, 4 }
    };
    m3dV v[8];
    GetVertices(v);
    if (0 <= i && i <= 5)
        for (int k = 0; k < 4; k++)
            out->vert[k] = v[idx[i][k]];
    out->nVert = 4;
}


// v12 0x00577740  (feb 0x0055F1D0)  vtable +0x2C
// Grow by d in every direction.
void m3dPRISM::Expand(float d)
{
    m3dV v = { d, d, d };
    Expand(&v, &v);
}


// v12 0x00577770  (feb 0x0055F200)  vtable +0x14
// Fits the smallest Y-rotated rectangle prism to n points (n > 1): y range = min/max y; the XZ
// rectangle is searched at every whole degree 0..89 (smallest half perimeter wins).
void m3dPRISM::Calc(m3dV *pts, int n)
{
    if (n <= 1)
        return;
    float lo = pts[0].y, hi = pts[0].y;
    for (int i = 1; i < n; i++) {
        if (pts[i].y < lo) lo = pts[i].y;
        if (hi < pts[i].y) hi = pts[i].y;
    }
    yMin = lo;
    yMax = hi;
    float xMin, xMax, zMin, zMax;
    float best = M3D_INFINITY;
    int   bestAng = 0;
    for (int a = 0; (float)a < 90.0f; a++) {
        float s = _MakeBPrismRect(pts, n, (float)a, &xMin, &xMax, &zMin, &zMax);
        if (s < best) {
            best    = s;
            bestAng = a;
        }
    }
    _MakeBPrismRect(pts, n, (float)bestAng, &xMin, &xMax, &zMin, &zMax);
    _MakeBPrismPolyBase((float)bestAng, xMin, xMax, zMin, zMax);
}


// v12 0x005778B0  (feb 0x0055F340)  private
// XZ bounds of the points rotated by 'ang' degrees about Y; returns the half perimeter.
float m3dPRISM::_MakeBPrismRect(m3dV *pts, int n, float ang, float *xMin, float *xMax, float *zMin, float *zMax)
{
    m3dMATR rot;
    rot.MakeRotY(ang);
    for (int i = 0; i < n; i++) {
        m3dV q;
        m3dTransformPoint(&pts[i], &rot, &q);
        if (i == 0) {
            *xMax = *xMin = q.x;
            *zMax = *zMin = q.z;
        } else {
            *xMin = (q.x <= *xMin) ? q.x : *xMin;
            *xMax = (*xMax <= q.x) ? q.x : *xMax;
            *zMin = (q.z <= *zMin) ? q.z : *zMin;
            *zMax = (*zMax <= q.z) ? q.z : *zMax;
        }
    }
    return (*zMax - *zMin) + (*xMax - *xMin);
}


// v12 0x00577AF0  (feb 0x0055F580)  private
// Builds the 4-vertex base from a rectangle in the rotated frame (grown by eps), rotated back by -ang.
void m3dPRISM::_MakeBPrismPolyBase(float ang, float xMin, float xMax, float zMin, float zMax)
{
    m3dMATR rot;
    rot.MakeRotY(-ang);
    base.nVert = 4;
    const m3dV c[4] = {
        { xMin - M3D_EPSILON, 0.0f, zMin - M3D_EPSILON },
        { M3D_EPSILON + xMax, 0.0f, zMin - M3D_EPSILON },
        { M3D_EPSILON + xMax, 0.0f, M3D_EPSILON + zMax },
        { xMin - M3D_EPSILON, 0.0f, M3D_EPSILON + zMax },
    };
    for (int i = 0; i < 4; i++) {
        m3dV q;
        m3dTransformPoint(&c[i], &rot, &q);
        base.vert[i].x = q.x;
        base.vert[i].y = q.z;
    }
}


// v12 0x005770C0  (feb 0x0055EB40)
// The 8 corners: 0..3 = outline at yMin, 4..7 = outline at yMax.
void m3dPRISM::GetVertices(m3dV *out)
{
    for (int i = 0; i < 4; i++) {
        out[i].x     = base.vert[i].x;
        out[i].z     = base.vert[i].y;
        out[i].y     = yMin;
        out[i + 4].x = base.vert[i].x;
        out[i + 4].z = base.vert[i].y;
        out[i + 4].y = yMax;
    }
}


// v12 0x00577110  (feb 0x0055EB90)
// Grows the rectangle prism: 'hi' along +u (v0->v1), +y, +w (v0->v3); 'lo' along the negative
// directions. Degenerate edges are rebuilt perpendicular to the other one (or the world axes).
// Returns 0 when a negative growth collapses the rectangle (-hi-lo > edge length).
// 1.2: same logic (inlined m3dSqrt rescheduled).
BOOL32 m3dPRISM::Expand(m3dV *hi, m3dV *lo)
{
    m2dV *v = base.vert;
    m2dV u = { v[1].x - v[0].x, v[1].y - v[0].y };
    m2dV w = { v[3].x - v[0].x, v[3].y - v[0].y };
    float lu = m3dSqrt(u.x * u.x + u.y * u.y);
    float lw = m3dSqrt(w.y * w.y + w.x * w.x);
    float tol = M3D_EPSILON * 10.0f;
    if (tol <= lu || tol <= lw) {
        if (lu < tol) {
            u.y = -w.x;
            u.x = w.y;
        } else if (lw < tol) {
            w.x = -u.y;
            w.y = u.x;
        }
    } else {
        u = m2dVUnitX;
        w = m2dVUnitY;
    }
    lu = m3dSqrt(u.x * u.x + u.y * u.y);
    lw = m3dSqrt(w.y * w.y + w.x * w.x);
    if (M3D_EPSILON <= m3dFAbs(lu)) { u.x *= 1.0f / lu; u.y *= 1.0f / lu; }
    if (M3D_EPSILON <= m3dFAbs(lw)) { w.x *= 1.0f / lw; w.y *= 1.0f / lw; }

    v[0].x += u.x * -lo->x;  v[0].y += -lo->x * u.y;
    v[3].x += u.x * -lo->x;  v[3].y += -lo->x * u.y;
    v[1].x += u.x * hi->x;   v[1].y += hi->x * u.y;
    v[2].x += u.x * hi->x;   v[2].y += hi->x * u.y;
    v[0].x += w.x * -lo->z;  v[0].y += -lo->z * w.y;
    v[1].x += w.x * -lo->z;  v[1].y += -lo->z * w.y;
    v[2].x += w.x * hi->z;   v[2].y += hi->z * w.y;
    v[3].x += w.x * hi->z;   v[3].y += hi->z * w.y;
    yMin -= lo->y;
    yMax += hi->y;
    if (-hi->x - lo->x <= lu && -hi->z - lo->z <= lw)
        return 1;
    return 0;
}
