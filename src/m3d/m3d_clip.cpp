// m3d_clip.cpp -- Will Rock 1.2 (v12.exe): point-in-polygon tests and polygon / segment clipping
//
// Reconstructed from the Feb 2003 build (m3d:m3d_clip.obj); every function is byte-identical in 1.2.
// v12 range 0x0057A0A0 .. 0x0057C41F (feb 0x00561780 .. 0x00563AFF).
//
// Clip result convention (all clip functions): -1 = nothing was clipped (input completely inside),
// 0 = clipped, 1 = nothing left (completely outside / degenerate). Point tests: 1 = inside, 0 = outside.

#include "m3d_lib.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0057A640 (feb 0x00561D20) -> v12 0x009AEB18 (feb 0x009637C8)

static const float CLIP_EPS = M3D_EPSILON;   // v12 0x00609B28 (feb 0x005DE63C)

// scratch polygons of _m2dClipPolyRect (one per intermediate pass; not re-entrant)
static m2dPOLY _m2dClipTmpXMin;   // v12 0x009AEB20 (feb 0x009637D0)
static m2dPOLY _m2dClipTmpXMax;   // v12 0x009AEBC8 (feb 0x00963878)
static m2dPOLY _m2dClipTmpYMin;   // v12 0x009AEC70 (feb 0x00963920)

// Signed area term of (a, b, p) in the 2D plane (u, v): > 0 / < 0 depending on the side of p.
// Written in the exact operation order of the original (all six copies are the same expression).
static inline float _side(float au, float av, float bu, float bv, float pu, float pv)
{
    return (pv * bu + ((bv - pv) * au - (bu - pu) * av)) - bv * pu;
}

// Point-in-convex-polygon in the 2D projection (u = component U, v = component V) with tolerance eps.
// The winding is taken from the first three vertices.
template <typename VTX, int U, int V, typename PT>
static inline BOOL32 _isBelongPointPoly(int n, const VTX *vert, const PT *p, float eps)
{
    const float *v0 = (const float *)&vert[0], *v1 = (const float *)&vert[1], *v2 = (const float *)&vert[2];
    const float *pp = (const float *)p;
    float o = _side(v0[U], v0[V], v1[U], v1[V], v2[U], v2[V]);
    int i = 0;
    if (o <= eps && o < -eps) {               // clockwise: every edge term must be <= eps
        for (; i < n - 1; i++) {
            const float *a = (const float *)&vert[i], *b = (const float *)&vert[i + 1];
            if (eps < _side(a[U], a[V], b[U], b[V], pp[U], pp[V]))
                return 0;
        }
        const float *a = (const float *)&vert[i];
        if (eps < _side(a[U], a[V], v0[U], v0[V], pp[U], pp[V]))
            return 0;
        return 1;
    }
    for (; i < n - 1; i++) {                   // counter-clockwise (or degenerate): every term >= -eps
        const float *a = (const float *)&vert[i], *b = (const float *)&vert[i + 1];
        float f = _side(a[U], a[V], b[U], b[V], pp[U], pp[V]);
        if (f <= eps && f < -eps)
            return 0;
    }
    const float *a = (const float *)&vert[i];
    float f = _side(a[U], a[V], v0[U], v0[V], pp[U], pp[V]);
    if (f <= eps && f < -eps)
        return 0;
    return 1;
}


// v12 0x0057A0A0  (feb 0x00561780)
// Side of p relative to the directed line a->b: -1 left (> eps), 1 right (< -eps), 0 on the line.
int __fastcall m2dQualPointLine_A(m2dV *p, m2dV *a, m2dV *b, float eps)
{
    float f = _side(a->x, a->y, b->x, b->y, p->x, p->y);
    if (eps < f)
        return -1;
    if (f < -eps)
        return 1;
    return 0;
}


// v12 0x0057A100  (feb 0x005617E0)
// Point in triangle (copies it into a 3-vertex polygon).
BOOL32 __fastcall m2dIsBelongPointTri_A(m2dTRI *t, m2dV *p, float eps)
{
    m2dPOLY poly;
    poly.nVert = 3;
    poly.vert[0] = t->v[0];
    poly.vert[1] = t->v[1];
    poly.vert[2] = t->v[2];
    return m2dIsBelongPointPoly_A(&poly, p, eps);
}


// v12 0x0057A160  (feb 0x00561840)
// Point in convex 2D polygon.
BOOL32 __fastcall m2dIsBelongPointPoly_A(m2dPOLY *poly, m2dV *p, float eps)
{
    return _isBelongPointPoly<m2dV, 0, 1>(poly->nVert, poly->vert, p, eps);
}


// v12 0x0057A300  (feb 0x005619E0)
// Point in convex 3D polygon projected on the ground plane (x, z).
BOOL32 __fastcall m3xzIsBelongPointPoly_A(m3dPOLY *poly, m3dV *p, float eps)
{
    return _isBelongPointPoly<m3dV, 0, 2>(poly->nVert, poly->vert, p, eps);
}


// v12 0x0057A4A0  (feb 0x00561B80)
// Point in convex 3D polygon projected on (x, y).
BOOL32 __fastcall m3xyIsBelongPointPoly_A(m3dPOLY *poly, m3dV *p, float eps)
{
    return _isBelongPointPoly<m3dV, 0, 1>(poly->nVert, poly->vert, p, eps);
}


// v12 0x0057A660  (feb 0x00561D40)
// Point in convex 3D polygon projected on (y, z).
BOOL32 __fastcall m3yzIsBelongPointPoly_A(m3dPOLY *poly, m3dV *p, float eps)
{
    return _isBelongPointPoly<m3dV, 1, 2>(poly->nVert, poly->vert, p, eps);
}


// v12 0x0057A810  (feb 0x00561EF0)
// Point in convex 3D polygon: projects along the dominant axis of the normal n (computed if 0).
BOOL32 __fastcall m3dIsBelongPointPoly_A(m3dPOLY *poly, m3dV *p, m3dV *n, float eps)
{
    m3dV tmp;
    if (!n) {
        m3dGetPolyNormal(poly, &tmp);
        n = &tmp;
    }
    if (m3dFAbs(n->x) <= m3dFAbs(n->y)) {
        if (m3dFAbs(n->z) < m3dFAbs(n->y))
            return m3xzIsBelongPointPoly_A(poly, p, eps);
    } else if (m3dFAbs(n->z) < m3dFAbs(n->x)) {
        return m3yzIsBelongPointPoly_A(poly, p, eps);
    }
    return m3xyIsBelongPointPoly_A(poly, p, eps);
}


// appends v to out unless it repeats the last emitted vertex (all components within CLIP_EPS)
static inline void _emit3(m3dPOLY *out, int &n, float x, float y, float z)
{
    if (n != 0) {
        m3dV *l = &out->vert[n - 1];
        if (m3dFAbs(l->x - x) < CLIP_EPS && m3dFAbs(l->y - y) < CLIP_EPS && m3dFAbs(l->z - z) < CLIP_EPS)
            return;
    }
    out->vert[n].x = x; out->vert[n].y = y; out->vert[n].z = z;
    n++;
}
static inline void _emit2(m2dPOLY *out, int &n, float x, float y)
{
    if (n != 0) {
        m2dV *l = &out->vert[n - 1];
        if (m3dFAbs(l->x - x) < CLIP_EPS && m3dFAbs(l->y - y) < CLIP_EPS)
            return;
    }
    out->vert[n].x = x; out->vert[n].y = y;
    n++;
}


// v12 0x0057A900  (feb 0x00561FE0)
// Sutherland-Hodgman against z >= zNear (camera space). Consecutive duplicates are dropped.
int __fastcall m3dClipPolyNearPlane(m3dPOLY *p, float zNear, m3dPOLY *out)
{
    int res = -1, n = 0;
    for (int i = 0; i < p->nVert; i++) {
        m3dV *a = &p->vert[i];
        m3dV *b = &p->vert[i == p->nVert - 1 ? 0 : i + 1];
        float ax = a->x, ay = a->y, az = a->z;
        if (az < zNear) {
            if (zNear <= b->z) {
                float x = ((b->x - ax) * (zNear - az)) / (b->z - az) + ax;
                float y = ((b->y - ay) * (zNear - az)) / (b->z - az) + ay;
                _emit3(out, n, x, y, zNear);
                res = 0;
            }
        } else {
            _emit3(out, n, ax, ay, az);
            if (b->z < zNear) {
                float x = ((b->x - ax) * (zNear - az)) / (b->z - az) + ax;
                float y = ((b->y - ay) * (zNear - az)) / (b->z - az) + ay;
                _emit3(out, n, x, y, zNear);
                res = 0;
            }
        }
    }
    if (n > 2) {
        out->nVert = n;
        return res;
    }
    return 1;
}


// v12 0x0057AD60  (feb 0x00562440)
// Clips p to the half space n.(v - planePt) >= 0 (planePt may be 0 = plane through the origin).
// Intersections within 100*eps of a vertex are not emitted (avoid slivers).
int __fastcall m3dClipPolyPlane(m3dPOLY *p, m3dV *planePt, m3dV *n, m3dPOLY *out)
{
    m3dPOLY local;
    m3dPOLY *src = p;
    if (planePt) {
        local.nVert = p->nVert;
        for (int i = 0; i < p->nVert; i++)
            local.vert[i] = p->vert[i] - *planePt;
        src = &local;
    }
    float d[M3D_POLY_MAX_VERT];
    int cnt = src->nVert;
    for (int i = 0; i < cnt; i++)
        d[i] = src->vert[i].y * n->y + src->vert[i].x * n->x + src->vert[i].z * n->z;

    int res = -1, k = 0;
    for (int i = 0; i < cnt; i++) {
        int j = (i == cnt - 1) ? 0 : i + 1;
        bool jOut = d[j] < 0.0f;
        m3dV *a = &src->vert[i], *b = &src->vert[j];
        if (d[i] < 0.0f) {
            if (!jOut) {
                float t = d[i] / (d[i] - d[j]);
                if (1.0f - CLIP_EPS * 100.0f <= t) {
                    res = 0;                                   // intersection == b, emitted next
                } else {
                    float s = 1.0f - t;
                    res = 0;
                    out->vert[k].x = t * b->x + s * a->x;
                    out->vert[k].y = t * b->y + s * a->y;
                    out->vert[k].z = s * a->z + t * b->z;
                    k++;
                }
            }
        } else {
            out->vert[k++] = *a;
            if (jOut) {
                float t = d[i] / (d[i] - d[j]);
                if (CLIP_EPS * 100.0f < t) {
                    float s = 1.0f - t;
                    out->vert[k].x = s * a->x + t * b->x;
                    out->vert[k].y = t * b->y + s * a->y;
                    out->vert[k].z = s * a->z + t * b->z;
                    k++;
                }
                res = 0;
            }
        }
    }
    if (k > 2) {
        if (planePt)
            for (int i = 0; i < k; i++) {
                out->vert[i].x = out->vert[i].x + planePt->x;
                out->vert[i].y = planePt->y + out->vert[i].y;
                out->vert[i].z = out->vert[i].z + planePt->z;
            }
        out->nVert = k;
        return res;
    }
    return 1;
}


// v12 0x0057B020  (feb 0x00562700)
// Clips a 2D polygon to an axis-aligned rectangle given as a polygon (corners 0 and 2 used).
// Finds which rectangle sides are crossed and hands over to _m2dClipPolyRect; -1 + copy if none.
int __fastcall m2dClipPolyRect(m2dPOLY *p, m2dPOLY *rect, m2dPOLY *out)
{
    float x0 = rect->vert[0].x, y0 = rect->vert[0].y, x1 = rect->vert[2].x, y1 = rect->vert[2].y;
    if (x1 < x0) { float t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { float t = y0; y0 = y1; y1 = t; }
    int n = p->nVert;
    int sides = 0, cnt = 0;
    for (int i = 0; i < n; i++) if (p->vert[i].x < x0) { sides = 1; cnt = 1; break; }
    for (int i = 0; i < n; i++) if (x1 < p->vert[i].x) { sides |= 4; cnt++; break; }
    for (int i = 0; i < n; i++) if (p->vert[i].y < y0) { sides |= 8; cnt++; break; }
    for (int i = 0; i < n; i++) if (y1 < p->vert[i].y) { sides |= 2; cnt++; break; }
    if (sides)
        return _m2dClipPolyRect(p, rect, out, sides, cnt);
    *out = *p;                                                 // 0x29 dwords
    return -1;
}


// one Sutherland-Hodgman pass of _m2dClipPolyRect against a vertical (axis 0) or horizontal (axis 1)
// line at 'c'; keepGreater selects the kept side. Returns the vertex count.
static inline int _m2dClipPass(m2dPOLY *src, m2dPOLY *dst, int axis, float c, bool keepGreater, int &res)
{
    int n = 0;
    for (int i = 0; i < src->nVert; i++) {
        m2dV a = src->vert[i];
        m2dV b = src->vert[i == src->nVert - 1 ? 0 : i + 1];
        float au = axis == 0 ? a.x : a.y, av = axis == 0 ? a.y : a.x;
        float bu = axis == 0 ? b.x : b.y, bv = axis == 0 ? b.y : b.x;
        bool aOut = keepGreater ? (au < c) : (au > c);
        bool bIn  = keepGreater ? (c <= bu) : (bu <= c);
        float iv;
        if (aOut) {
            if (bIn) {
                iv = ((c - au) * (bv - av)) / (bu - au) + av;
                if (axis == 0) _emit2(dst, n, c, iv); else _emit2(dst, n, iv, c);
                res = 0;
            }
        } else {
            _emit2(dst, n, a.x, a.y);
            if (!bIn) {
                iv = ((c - au) * (bv - av)) / (bu - au) + av;
                if (axis == 0) _emit2(dst, n, c, iv); else _emit2(dst, n, iv, c);
                res = 0;
            }
        }
    }
    return n;
}


// v12 0x0057B160  (feb 0x00562840)
// Clips against the rectangle sides selected in 'sides' (1 = x min, 8 = y min, 4 = x max, 2 = y max,
// in that order); nPass = number of selected sides; the last pass writes to out, earlier ones to static
// scratch polygons. A closing vertex equal to the first one is dropped at the end.
int __fastcall _m2dClipPolyRect(m2dPOLY *p, m2dPOLY *rect, m2dPOLY *out, int sides, int nPass)
{
    float x0 = rect->vert[0].x, y0 = rect->vert[0].y, x1 = rect->vert[2].x, y1 = rect->vert[2].y;
    if (x1 < x0) { float t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { float t = y0; y0 = y1; y1 = t; }
    int res = -1;
    int n = nPass;
    m2dPOLY *src = p;
    if (sides & 1) {
        m2dPOLY *dst = nPass != 1 ? &_m2dClipTmpXMin : out;
        if (src->nVert < 1) return 1;
        n = _m2dClipPass(src, dst, 0, x0, true, res);
        if (n < 3) return 1;
        nPass--;
        dst->nVert = n;
        src = dst;
    }
    if (sides & 8) {
        m2dPOLY *dst = nPass != 1 ? &_m2dClipTmpYMin : out;
        if (src->nVert < 1) return 1;
        n = _m2dClipPass(src, dst, 1, y0, true, res);
        if (n < 3) return 1;
        nPass--;
        dst->nVert = n;
        src = dst;
    }
    if (sides & 4) {
        m2dPOLY *dst = nPass != 1 ? &_m2dClipTmpXMax : out;
        if (src->nVert < 1) return 1;
        n = _m2dClipPass(src, dst, 0, x1, false, res);
        if (n < 3) return 1;
        dst->nVert = n;
        src = dst;
    }
    if (sides & 2) {
        if (src->nVert < 1) return 1;
        n = _m2dClipPass(src, out, 1, y1, false, res);
        if (n < 3) return 1;
        out->nVert = n;
    }
    // drop a duplicated closing vertex (n = vertex count of the last pass)
    if (m3dFAbs(out->vert[n - 1].x - out->vert[0].x) < CLIP_EPS &&
        m3dFAbs(out->vert[n - 1].y - out->vert[0].y) < CLIP_EPS) {
        out->nVert--;
        if (out->nVert < 3)
            return 1;
    }
    return res;
}


// Cohen-Sutherland out-code in (x, y) for m3xyClipLSegRect: 1 x<min, 8 y<min, 4 x>max, 2 y>max
static inline unsigned _xyCode(const m3dV *p, float x0, float y0, float x1, float y1)
{
    unsigned c = p->x < x0 ? 1u : 0u;
    if (p->y < y0) c |= 8;
    if (x1 < p->x) c |= 4;
    if (y1 < p->y) c |= 2;
    return c;
}
// moves q onto the line axis == c on segment a-b, z interpolated perspective-correctly (in 1/z)
static inline void _xyClip(m3dV *q, const m3dV *a, const m3dV *b, int axis, float c)
{
    float au = axis == 0 ? a->x : a->y, av = axis == 0 ? a->y : a->x;
    float bu = axis == 0 ? b->x : b->y, bv = axis == 0 ? b->y : b->x;
    float iz = 1.0f / ((((1.0f / b->z - 1.0f / a->z) * (c - au)) / (bu - au)) + 1.0f / a->z);
    float v  = ((bv - av) * (c - au)) / (bu - au) + av;
    q->z = iz;
    if (axis == 0) { q->x = c; q->y = v; } else { q->y = c; q->x = v; }
}
static inline void _xyClipCode(m3dV *q, const m3dV *a, const m3dV *b, unsigned code, float x0, float y0, float x1, float y1)
{
    if      (code & 1) _xyClip(q, a, b, 0, x0);
    else if (code & 8) _xyClip(q, a, b, 1, y0);
    else if (code & 4) _xyClip(q, a, b, 0, x1);
    else if (code & 2) _xyClip(q, a, b, 1, y1);
}


// v12 0x0057BEC0  (feb 0x005635A0)
// Clips a camera-space segment to a near-plane rectangle in (x, y); z is interpolated in 1/z.
int __fastcall m3xyClipLSegRect(m3dV *a, m3dV *b, m2dPOLY *rect, m3dV *oa, m3dV *ob)
{
    float x0 = rect->vert[0].x, y0 = rect->vert[0].y, x1 = rect->vert[2].x, y1 = rect->vert[2].y;
    if (x1 < x0) { float t = x0; x0 = x1; x1 = t; }
    if (y1 < y0) { float t = y0; y0 = y1; y1 = t; }
    int res = -1;
    *oa = *a;
    *ob = *b;
    unsigned ca = _xyCode(oa, x0, y0, x1, y1);
    unsigned cb = _xyCode(ob, x0, y0, x1, y1);
    if (cb != 0 || ca != 0) {
        res = 0;
        do {
            if (cb & ca)
                return 1;
            _xyClipCode(oa, oa, ob, ca, x0, y0, x1, y1);
            ca = _xyCode(oa, x0, y0, x1, y1);
            if (ca & cb)
                return 1;
            m3dV a0 = *oa;
            _xyClipCode(ob, &a0, ob, cb, x0, y0, x1, y1);
            cb = _xyCode(ob, x0, y0, x1, y1);
        } while (cb != 0 || ca != 0);
    }
    return res;
}


// v12 0x0057C330  (feb 0x00563A10)
// Clips segment a-b (in place) to zNear <= z <= zFar.
// ? As compiled the far test compares the far point against zFar twice, so the far clipping code is
//   unreachable: any segment reaching beyond zFar returns 1 (rejected).
int __fastcall m3dClipLSegNearFar(m3dV *a, m3dV *b, float zNear, float zFar)
{
    m3dV *n = a, *f = b;                         // n = nearer end, f = farther end
    if (b->z < a->z) { f = a; n = b; }
    int res = -1;
    if (n->z < zNear) {
        if (f->z < zNear)
            return 1;
        float nz = n->z;
        res = 0;
        n->x = ((f->x - n->x) * (zNear - nz)) / (f->z - n->z) + n->x;
        float fy = f->y, fz = f->z, oz = n->z;
        n->z = zNear;
        n->y = ((fy - n->y) * (zNear - nz)) / (fz - oz) + n->y;
    }
    if (zFar < f->z) {
        if (zFar < f->z)                         // ? (sic) -- always true here
            return 1;
        f->x = ((f->x - n->x) * (zFar - n->z)) / (f->z - n->z) + n->x;
        float nz = n->z, ny = n->y, fz = f->z;
        f->z = zFar;
        res = 0;
        f->y = ((f->y - ny) * (zFar - nz)) / (fz - nz) + ny;
    }
    return res;
}
