// m3d_box.cpp -- Will Rock 1.2 (v12.exe): m3dBOX, the axis-aligned bounding box volume
//
// Reconstructed from the Feb 2003 build (m3d:m3d_box.obj) and checked against v12.
// All feb functions are unchanged in 1.2; 1.2 adds m3dBOX::QualPlane (vtable slot +0x30).
// v12 range 0x0056C530 .. 0x0056E71F (feb 0x005544B0 .. 0x005565FF).

#include "m3d_box.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0056D020 (feb 0x00554FC0) -> v12 0x009AEAD4 (feb 0x00963788)

static const float BOX_EPS = M3D_EPSILON;   // v12 0x00609A88 (feb 0x005DE5A8)
static const float BOX_BIG = M3D_INFINITY;  // v12 0x00609A8C (feb 0x005DE5AC)

// v12 0x00637C68  (feb 0x005F9A18)  m3d_box.obj .data: outward normals of the 6 faces
m3dV m3dBoxNormData[6] __asm__("_m3dBoxNorm") = {
    { 0.0f, 0.0f, -1.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f },
    { -1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }
};

static inline void _boxSet(m3dV *v, float x, float y, float z) { v->x = x; v->y = y; v->z = z; }


// v12 0x0056C530  (feb 0x005544B0)
// Point strictly inside the box grown by 100*eps.
BOOL32 m3dBOX::IsBelongPoint(m3dV *p)
{
    float e = BOX_EPS * 100.0f;
    if (vMin.x - e < p->x && vMin.y - e < p->y && vMin.z - e < p->z &&
        p->x < e + vMax.x && p->y < e + vMax.y && p->z < e + vMax.z)
        return 1;
    return 0;
}


// v12 0x0056C5C0  (feb 0x00554540)
// Swept-sphere test: grows the box by r, casts the ray, shrinks it back.
// Arguments as mangled: (m3dV *org, m3dV *dir, float maxT, float r).
BOOL32 m3dBOX::IsIsectCyl(m3dV *org, m3dV *dir, float maxT, float r)
{
    vMin.x -= r; vMin.y -= r; vMin.z -= r;
    vMax.x = r + vMax.x; vMax.y = r + vMax.y; vMax.z = r + vMax.z;
    BOOL32 hit = IsectRay(org, dir, maxT, 0, 0, 0);
    float nr = -r;
    vMin.x -= nr; vMin.y -= nr; vMin.z -= nr;
    vMax.x = nr + vMax.x; vMax.y = nr + vMax.y; vMax.z = nr + vMax.z;
    return hit != 0;
}


// v12 0x0056C660  (feb 0x005545E0)
// Ray (org, dir) vs. box within maxT: 2 = org inside, 1 = hit (hit point, distance, face), 0 = miss.
BOOL32 m3dBOX::IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face)
{
    if (IsBelongPoint(org))                     // virtual call (slot +0x04)
        return 2;
    float maxT2 = maxT * maxT;
    m3dV *n = M3D_BOX_NORM;
    for (int f = 0; f < 6; f++, n++) {
        if (n->z * dir->z + dir->x * n->x + dir->y * n->y > -BOX_EPS)
            continue;                           // face not facing the ray
        m3dV planePt;
        switch (f) {                            // jump table v12 (feb 0x0055481C)
        case 0: case 3: case 4: planePt = vMin; break;
        case 1: case 2: case 5: planePt = vMax; break;
        }
        m3dV h;
        if (!m3dIsectRayVNPlane(org, dir, &planePt, n, &h, 0))
            continue;
        m3dV d = h - *org;
        if (maxT2 < d.x * d.x + d.y * d.y + d.z * d.z)
            return 0;
        if (!static_cast<m3dBOX_EXT *>(this)->IsBelongPointFace(f, &h))
            continue;
        if (hit)
            *hit = h;
        if (t) {
            m3dV e = h - *org;
            *t = m3dSqrt(e.x * e.x + e.y * e.y + e.z * e.z);
        }
        if (face)
            *face = f;
        return 1;
    }
    return 0;
}


// v12 0x0056C8B0  (feb 0x00554840)
// Distance from p to the box if <= maxD: 2 = inside (closest = p, d = 0), 1 = within maxD
// (closest point, distance, face index), 0 = farther.
BOOL32 m3dBOX::DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face)
{
    if (IsBelongPoint(p)) {                     // virtual call
        if (closest) *closest = *p;
        if (d) *d = 0.0f;
        return 2;
    }
    float cx, cz, dist;
    int   f = -1;                               // (stale register in the original; always overwritten below)
    // ground plane (XZ) first
    if (p->x <= vMax.x) {
        if (vMin.x <= p->x) {
            if (p->z <= vMax.z) {
                if (vMin.z <= p->z) { cx = p->x; cz = p->z; dist = 0.0f; }
                else { cz = vMin.z; cx = p->x; f = 0; dist = vMin.z - p->z; }
            } else   { cx = p->x; cz = vMax.z; f = 2; dist = p->z - vMax.z; }
        } else if (vMin.z <= p->z) {
            if (p->z <= vMax.z) { cx = vMin.x; cz = p->z; f = 3; dist = vMin.x - p->x; }
            else {
                cx = vMin.x; cz = vMax.z;
                dist = m3dSqrt((p->x - cx) * (p->x - cx) + (p->z - cz) * (p->z - cz));
                f = 3;
            }
        } else {
            dist = m3dSqrt((p->x - vMin.x) * (p->x - vMin.x) + (p->z - vMin.z) * (p->z - vMin.z));
            cx = vMin.x; cz = vMin.z; f = 3;
        }
    } else if (p->z <= vMax.z) {
        if (vMin.z <= p->z) { cx = vMax.x; cz = p->z; f = 1; dist = p->x - vMax.x; }
        else {
            cx = vMax.x; cz = vMin.z;
            float dx = p->x - cx;
            dist = m3dSqrt(dx * dx + (p->z - cz) * (p->z - cz));
            f = 1;
        }
    } else {
        dist = m3dSqrt((p->x - vMax.x) * (p->x - vMax.x) + (p->z - vMax.z) * (p->z - vMax.z));
        cz = vMax.z; cx = vMax.x; f = 1;
    }
    // then height
    float cy;
    if (vMin.y <= p->y) {
        if (p->y <= vMax.y) {
            cy = p->y;
        } else {
            if (maxD < p->y - vMax.y)
                return 0;
            cy = vMax.y;
            f = 5;
            dist = m3dSqrt(dist * dist + (vMax.y - p->y) * (vMax.y - p->y));
        }
    } else {
        float dy = vMin.y - p->y;
        if (maxD < dy)
            return 0;
        cy = vMin.y;
        f = 4;
        dist = m3dSqrt(dist * dist + dy * dy);
    }
    if (dist <= maxD) {
        if (closest) _boxSet(closest, cx, cy, cz);
        if (d) *d = dist;
        if (face) *face = f;
        return 1;
    }
    return 0;
}


// v12 0x00408190  (inline in m3d_box.h of the original, emitted in game code; vtable slot +0x14)
// Fit to a point list, no minimum size.
void m3dBOX::Calc(m3dV *pts, int n)
{
    Calc(pts, n, 0.0f);
}


// v12 0x0056CF60  (feb 0x00554F00)
// Empty box ready for CalcUpdate: min = +BIG, max = -BIG.
void m3dBOX::CalcStart()
{
    float b = BOX_BIG;
    vMin.x = b; vMin.y = b; vMin.z = b;
    b = -BOX_BIG;
    vMax.x = b; vMax.y = b; vMax.z = b;
}


// v12 0x0056CFA0  (feb 0x00554F40)
// Grows the box to contain p.
void m3dBOX::CalcUpdate(m3dV *p)
{
    if (p->x < vMin.x) vMin.x = p->x;
    if (p->y < vMin.y) vMin.y = p->y;
    if (p->z < vMin.z) vMin.z = p->z;
    if (vMax.x < p->x) vMax.x = p->x;
    if (vMax.y < p->y) vMax.y = p->y;
    if (vMax.z < p->z) vMax.z = p->z;
}


// v12 0x0056D040  (feb 0x00554FE0)
// Grows the box to contain n points.
void m3dBOX::CalcUpdate(m3dV *p, int n)
{
    for (; n > 0; n--, p++)
        CalcUpdate(p);
}


// v12 0x0056D060  (feb 0x00555000)
// Grows the box to contain another box.
void m3dBOX::CalcUpdate(m3dBOX *b)
{
    CalcUpdate(&b->vMin);
    CalcUpdate(&b->vMax);
}


// v12 0x0056D080  (feb 0x00555020)
// Fits the box to n points; every side is at least minSize long.
void m3dBOX::Calc(m3dV *p, int n, float minSize)
{
    vMin = p[0];
    vMax = vMin;
    for (int i = 1; i < n; i++) {
        m3dV *q = &p[i];
        if (vMax.x < q->x) vMax.x = q->x;
        if (vMax.y < q->y) vMax.y = q->y;
        if (vMax.z < q->z) vMax.z = q->z;
        if (q->x < vMin.x) vMin.x = q->x;
        if (q->y < vMin.y) vMin.y = q->y;
        if (q->z < vMin.z) vMin.z = q->z;
    }
    if (m3dFAbs(vMax.x - vMin.x) < minSize) vMax.x = minSize + vMin.x;
    if (m3dFAbs(vMax.y - vMin.y) < minSize) vMax.y = minSize + vMin.y;
    if (m3dFAbs(vMax.z - vMin.z) < minSize) vMax.z = minSize + vMin.z;
}


// v12 0x0056D1C0  (feb 0x00555160)
// Like Calc but only X and Z are fitted (Y untouched).
void m3dBOX::CalcXZ(m3dV *p, int n, float minSize)
{
    vMin.x = vMax.x = p[0].x;
    vMin.z = vMax.z = p[0].z;
    for (int i = 1; i < n; i++) {
        m3dV *q = &p[i];
        if (vMax.x < q->x) vMax.x = q->x;
        if (vMax.z < q->z) vMax.z = q->z;
        if (q->x < vMin.x) vMin.x = q->x;
        if (q->z < vMin.z) vMin.z = q->z;
    }
    if (m3dFAbs(vMax.x - vMin.x) < minSize) vMax.x = minSize + vMin.x;
    if (m3dFAbs(vMax.z - vMin.z) < minSize) vMax.z = minSize + vMin.z;
}


// v12 0x0056D290  (feb 0x00555230)
// Box spanned by two corner points.
void m3dBOX::MakePointPoint(m3dV *a, m3dV *b)
{
    vMin.x = a->x < b->x ? a->x : b->x;
    vMin.y = a->y < b->y ? a->y : b->y;
    vMin.z = a->z < b->z ? a->z : b->z;
    vMax.x = b->x < a->x ? a->x : b->x;
    vMax.y = b->y < a->y ? a->y : b->y;
    vMax.z = b->z < a->z ? a->z : b->z;
}


// v12 0x0056D350  (feb 0x005552F0)
// The 8 corners: bottom (z = min) 0..3 counter-clockwise from min, top (z = max) 4..7.
void m3dBOX::GetVertices(m3dV out[8])
{
    _boxSet(&out[0], vMin.x, vMin.y, vMin.z);
    _boxSet(&out[1], vMax.x, vMin.y, vMin.z);
    _boxSet(&out[2], vMax.x, vMax.y, vMin.z);
    _boxSet(&out[3], vMin.x, vMax.y, vMin.z);
    _boxSet(&out[4], vMin.x, vMin.y, vMax.z);
    _boxSet(&out[5], vMax.x, vMin.y, vMax.z);
    _boxSet(&out[6], vMax.x, vMax.y, vMax.z);
    _boxSet(&out[7], vMin.x, vMax.y, vMax.z);
}


// v12 0x0056D3F0  (feb 0x00555390)
// End points of edge 0..11 (0-3 ring at z min, 4-7 ring at z max, 8-11 the z-parallel edges).
void m3dBOX_EXT::GetEdgePoints(int edge, m3dV *a, m3dV *b)
{
    switch (edge) {
    case 0:  _boxSet(a, vMin.x, vMin.y, vMin.z); _boxSet(b, vMax.x, vMin.y, vMin.z); break;
    case 1:  _boxSet(a, vMax.x, vMin.y, vMin.z); _boxSet(b, vMax.x, vMax.y, vMin.z); break;
    case 2:  _boxSet(a, vMax.x, vMax.y, vMin.z); _boxSet(b, vMin.x, vMax.y, vMin.z); break;
    case 3:  _boxSet(a, vMin.x, vMax.y, vMin.z); _boxSet(b, vMin.x, vMin.y, vMin.z); break;
    case 4:  _boxSet(a, vMin.x, vMin.y, vMax.z); _boxSet(b, vMax.x, vMin.y, vMax.z); break;
    case 5:  _boxSet(a, vMax.x, vMin.y, vMax.z); _boxSet(b, vMax.x, vMax.y, vMax.z); break;
    case 6:  _boxSet(a, vMax.x, vMax.y, vMax.z); _boxSet(b, vMin.x, vMax.y, vMax.z); break;
    case 7:  _boxSet(a, vMin.x, vMax.y, vMax.z); _boxSet(b, vMin.x, vMin.y, vMax.z); break;
    case 8:  _boxSet(a, vMin.x, vMin.y, vMin.z); _boxSet(b, vMin.x, vMin.y, vMax.z); break;
    case 9:  _boxSet(a, vMax.x, vMin.y, vMin.z); _boxSet(b, vMax.x, vMin.y, vMax.z); break;
    case 10: _boxSet(a, vMax.x, vMax.y, vMin.z); _boxSet(b, vMax.x, vMax.y, vMax.z); break;
    case 11: _boxSet(a, vMin.x, vMax.y, vMin.z); _boxSet(b, vMin.x, vMax.y, vMax.z); break;
    default: break;
    }
}


// v12 0x0056D620  (feb 0x005555C0)
// Face polygon (4 verts) and/or outward normal; out-of-range faces are treated as face 0.
void m3dBOX::GetFace(int i, m3dPOLY *out, m3dV *normal)
{
    if (i < 0 || i > 5)
        i = 0;
    if (normal)
        *normal = M3D_BOX_NORM[i];
    if (!out)
        return;
    m3dV *v = out->vert;
    switch (i) {
    case 0:   // z = min
        _boxSet(&v[0], vMin.x, vMin.y, vMin.z); _boxSet(&v[1], vMin.x, vMax.y, vMin.z);
        _boxSet(&v[2], vMax.x, vMax.y, vMin.z); _boxSet(&v[3], vMax.x, vMin.y, vMin.z); break;
    case 1:   // x = max
        _boxSet(&v[0], vMax.x, vMin.y, vMin.z); _boxSet(&v[1], vMax.x, vMax.y, vMin.z);
        _boxSet(&v[2], vMax.x, vMax.y, vMax.z); _boxSet(&v[3], vMax.x, vMin.y, vMax.z); break;
    case 2:   // z = max
        _boxSet(&v[0], vMin.x, vMin.y, vMax.z); _boxSet(&v[1], vMax.x, vMin.y, vMax.z);
        _boxSet(&v[2], vMax.x, vMax.y, vMax.z); _boxSet(&v[3], vMin.x, vMax.y, vMax.z); break;
    case 3:   // x = min
        _boxSet(&v[0], vMin.x, vMin.y, vMin.z); _boxSet(&v[1], vMin.x, vMin.y, vMax.z);
        _boxSet(&v[2], vMin.x, vMax.y, vMax.z); _boxSet(&v[3], vMin.x, vMax.y, vMin.z); break;
    case 4:   // y = min
        _boxSet(&v[0], vMax.x, vMin.y, vMin.z); _boxSet(&v[1], vMax.x, vMin.y, vMax.z);
        _boxSet(&v[2], vMin.x, vMin.y, vMax.z); _boxSet(&v[3], vMin.x, vMin.y, vMin.z); break;
    case 5:   // y = max
        _boxSet(&v[0], vMin.x, vMax.y, vMin.z); _boxSet(&v[1], vMin.x, vMax.y, vMax.z);
        _boxSet(&v[2], vMax.x, vMax.y, vMax.z); _boxSet(&v[3], vMax.x, vMax.y, vMin.z); break;
    }
    out->nVert = 4;
}


// v12 0x0056D870  (feb 0x00555810)
// Valid unless all three extents are <= -eps (i.e. max < min on every axis, e.g. right after CalcStart).
BOOL32 m3dBOX::Check()
{
    if (BOX_EPS + vMax.x <= vMin.x && BOX_EPS + vMax.y <= vMin.y && BOX_EPS + vMax.z <= vMin.z)
        return 0;
    return 1;
}


// v12 0x0056D8C0  (feb 0x00555860)
// Enclosing sphere: centre of the box, radius to the min corner.
void m3dBOX::CalcEnclSph(m3dSPHERE *s)
{
    GetCenter(&s->center);                      // virtual call (slot +0x24)
    float dx = vMin.x - s->center.x;
    s->radius = m3dSqrt(dx * dx + (vMin.y - s->center.y) * (vMin.y - s->center.y) +
                        (vMin.z - s->center.z) * (vMin.z - s->center.z));
}


// v12 0x0056D9B0  (feb 0x00555950)
// True when all 8 corners are inside s (radius grown by 100*eps).
BOOL32 m3dBOX::CheckEnclSph(m3dSPHERE *s)
{
    m3dSPHERE_TMP tmp;
    tmp.vtbl   = *(const void **)s;             // the original stores the m3dSPHERE vtable constant  // ?
    tmp.type   = s->type;
    tmp.center = s->center;
    tmp.radius = BOX_EPS * 100.0f + s->radius;
    m3dV v[8];
    GetVertices(v);
    for (int i = 0; i < 8; i++)
        if (!tmp.IsBelongPoint(&v[i]))
            return 0;
    return 1;
}


// v12 0x0056DA40  (feb 0x005559E0)
// p lies within the rectangle of face f (the two in-plane coordinates, open interval grown by eps).
BOOL32 m3dBOX_EXT::IsBelongPointFace(int f, m3dV *p)
{
    switch (f) {
    case 0: case 2:      // z faces: x, y
        if (p->x <= vMin.x - BOX_EPS) return 0;
        if (p->y <= vMin.y - BOX_EPS) return 0;
        if (BOX_EPS + vMax.x <= p->x) return 0;
        return p->y < BOX_EPS + vMax.y;
    case 1: case 3:      // x faces: y, z
        if (p->y <= vMin.y - BOX_EPS) return 0;
        if (p->z <= vMin.z - BOX_EPS) return 0;
        if (!(p->y < BOX_EPS + vMax.y)) return 0;
        return p->z < BOX_EPS + vMax.z;
    case 4: case 5:      // y faces: x, z
        if (p->x <= vMin.x - BOX_EPS) return 0;
        if (p->z <= vMin.z - BOX_EPS) return 0;
        if (!(p->x < BOX_EPS + vMax.x)) return 0;
        return p->z < BOX_EPS + vMax.z;
    }
    return 0;
}


// v12 0x0056DB60  (feb 0x00555B00)
// Centre of the box.
void m3dBOX::GetCenter(m3dV *out)
{
    out->x = (vMax.x + vMin.x) * 0.5f;
    out->y = (vMax.y + vMin.y) * 0.5f;
    out->z = (vMax.z + vMin.z) * 0.5f;
}


// v12 0x0056DBA0  (feb 0x00555B40)
// Surface area.
float m3dBOX::GetArea()
{
    float dx = vMax.x - vMin.x, dy = vMax.y - vMin.y, dz = vMax.z - vMin.z;
    float a = dz * dx + (dz + dx) * dy;
    return a + a;
}


// v12 0x0056DBD0  (feb 0x00555B70)
// NOTE the inverted result as compiled: 0 when the boxes overlap on all three axes, 1 when separated.  // ?
BOOL32 m3dBOX::IsIsectBox(m3dBOX *b)
{
    const float *mn = &vMin.x, *mx = &vMax.x, *bmn = &b->vMin.x, *bmx = &b->vMax.x;
    for (int i = 0; i < 3; i++)
        if (!(bmn[i] <= mx[i] && mn[i] <= bmx[i]))
            return 1;
    return 0;
}


// v12 0x0056DC20  (feb 0x00555BC0)
// Polygon vs. box. The scene is permuted so the polygon's dominant normal axis becomes Y, the
// polygon (XZ) is clipped against the box's bottom face, and the plane height at every clipped
// vertex is compared with the box Y range. 1 = no contact, -1 = polygon inside, 0 = crossing;
// 0 also when no axis dominates (|n| ties).
BOOL32 m3dBOX::IsIsectPoly(m3dPOLY *poly, m3dV *n)
{
    m3dBOX  box;                                // temporary with the m3dBOX vtable, type = 1
    box.type = M3D_VOL_BOX;
    m3dV    nn, tmpN;
    m3dPOLY p;
    if (!n) {
        m3dGetPolyNormal(poly, &tmpN);
        n = &tmpN;
    }
    int axis;
    float ax = m3dFAbs(n->x), ay = m3dFAbs(n->y), az = m3dFAbs(n->z);
    if (ax >= ay && ax >= az)       axis = 0;
    else if (ay >= ax && ay >= az)  axis = 1;
    else if (az >= ax && az >= ay)  axis = 2;
    else                            return 0;

    p.nVert = poly->nVert;
    if (axis == 0) {            // (x,y,z) -> (z,x,y)
        _boxSet(&box.vMin, vMin.z, vMin.x, vMin.y);
        _boxSet(&box.vMax, vMax.z, vMax.x, vMax.y);
        for (int i = 0; i < p.nVert; i++)
            _boxSet(&p.vert[i], poly->vert[i].z, poly->vert[i].x, poly->vert[i].y);
        _boxSet(&nn, n->z, n->x, n->y);
    } else if (axis == 1) {     // unchanged
        box.type = type;
        box.vMin = vMin;
        box.vMax = vMax;
        for (int i = 0; i < p.nVert; i++)
            p.vert[i] = poly->vert[i];
        nn = *n;
    } else {                    // (x,y,z) -> (y,z,x)
        _boxSet(&box.vMin, vMin.y, vMin.z, vMin.x);
        _boxSet(&box.vMax, vMax.y, vMax.z, vMax.x);
        for (int i = 0; i < p.nVert; i++)
            _boxSet(&p.vert[i], poly->vert[i].y, poly->vert[i].z, poly->vert[i].x);
        _boxSet(&nn, n->y, n->z, n->x);
    }

    m3dPOLY face;
    m2dPOLY face2, poly2, clip;
    box.GetFace(M3D_BOX_FACE_YMIN, &face, 0);  // direct (non-virtual) call
    m3xzPoly3DToPoly2D(&face, &face2);
    m3xzPoly3DToPoly2D(&p, &poly2);
    int res = m2dClipPolyRect(&poly2, &face2, &clip);
    if (res == 1)
        return 1;

    int inside = 0, above = 0, below = 0;
    if (clip.nVert <= 0)
        return 1;
    for (int i = 0; i < clip.nVert; i++) {
        m3dV q = { clip.vert[i].x, 0.0f, clip.vert[i].y };
        m3dV h;
        m3dIsIsectYLinePlane(&q, &p.vert[0], &nn, &h);
        if (box.vMin.y <= h.y && h.y <= box.vMax.y)
            inside++;
        if (box.vMax.y < h.y)
            above++;
        if (h.y < box.vMin.y)
            below++;
    }
    if (below == 0 && inside == 0)
        return 1;
    if (above == 0) {
        if (inside == 0)
            return 1;
        if (below == 0 && res == -1)
            return -1;
    }
    return 0;
}


// v12 0x0056E070  (feb 0x00556010)
// Grows the box by d on every side.
void m3dBOX::Expand(float d)
{
    vMin.x -= d; vMin.y -= d; vMin.z -= d;
    vMax.x = d + vMax.x; vMax.y = d + vMax.y; vMax.z = d + vMax.z;
}


// out-code of a point (Cohen-Sutherland in 3D)
static inline unsigned _boxOutCode(const m3dV *p, const m3dV &mn, const m3dV &mx)
{
    unsigned c = p->x < mn.x ? 1u : 0u;
    if (p->y < mn.y) c |= 2;
    if (p->z < mn.z) c |= 4;
    if (mx.x < p->x) c |= 8;
    if (mx.y < p->y) c |= 0x10;
    if (mx.z < p->z) c |= 0x20;
    return c;
}
// moves 'q' along the segment a-b onto the plane axis == v (interpolated from a)
static inline void _boxClipTo(m3dV *q, const m3dV *a, const m3dV *b, int axis, float v)
{
    const float *pa = &a->x, *pb = &b->x;
    float t = (v - pa[axis]) / (pb[axis] - pa[axis]);
    m3dV r = { (pb[0] - pa[0]) * t + pa[0], (pb[1] - pa[1]) * t + pa[1], (pb[2] - pa[2]) * t + pa[2] };
    (&r.x)[axis] = v;
    *q = r;
}
static inline void _boxClipCode(m3dV *q, const m3dV *a, const m3dV *b, unsigned c, const m3dV &mn, const m3dV &mx)
{
    if      (c & 1)    _boxClipTo(q, a, b, 0, mn.x);
    else if (c & 2)    _boxClipTo(q, a, b, 1, mn.y);
    else if (c & 4)    _boxClipTo(q, a, b, 2, mn.z);
    else if (c & 8)    _boxClipTo(q, a, b, 0, mx.x);
    else if (c & 0x10) _boxClipTo(q, a, b, 1, mx.y);
    else if (c & 0x20) _boxClipTo(q, a, b, 2, mx.z);
}


// v12 0x0056E0B0  (feb 0x00556050)
// Clips segment a-b to the box (Cohen-Sutherland). 0 if it lies completely outside.
BOOL32 m3dBOX::ClipLSeg(m3dV *a, m3dV *b, m3dV *oa, m3dV *ob)
{
    m3dV mn = vMin, mx = vMax;                  // sorted per axis (tolerates inverted boxes)
    if (mx.x < mn.x) { float t = mn.x; mn.x = mx.x; mx.x = t; }
    if (mx.y < mn.y) { float t = mn.y; mn.y = mx.y; mx.y = t; }
    if (mx.z < mn.z) { float t = mn.z; mn.z = mx.z; mx.z = t; }
    *oa = *a;
    *ob = *b;
    unsigned ca = _boxOutCode(oa, mn, mx);
    unsigned cb = _boxOutCode(ob, mn, mx);
    while (cb != 0 || ca != 0) {
        if (ca & cb)
            return 0;
        _boxClipCode(oa, oa, ob, ca, mn, mx);
        ca = _boxOutCode(oa, mn, mx);
        if (cb & ca)
            return 0;
        m3dV a0 = *oa;
        _boxClipCode(ob, &a0, ob, cb, mn, mx);  // interpolated from the (already clipped) start point
        cb = _boxOutCode(ob, mn, mx);
    }
    return 1;
}


// v12 0x0056E660  (feb none)
// 1.2: new override (vtable slot +0x30). Exact plane test on the 8 corners instead of the beta's
// m3dVOL::QualPlane (enclosing sphere scaled by 0.7):
//   1 = every corner behind the plane, -1 = every corner in front, 0 = crossing.
// HOOK: plane culling / splitting of every box volume.
int m3dBOX::QualPlane(m3dV *planePt, m3dV *planeN)
{
    m3dV v[8];
    GetVertices(v);
    float dMin = BOX_BIG;
    float dMax = -BOX_BIG;
    for (int i = 0; i < 8; i++) {
        float d = (v[i].z - planePt->z) * planeN->z + (v[i].y - planePt->y) * planeN->y +
                  (v[i].x - planePt->x) * planeN->x;
        if (d < dMin) dMin = d;
        if (d > dMax) dMax = d;
    }
    if (dMax < 0.0f)
        return 1;
    if (dMin > 0.0f)
        return -1;
    return 0;
}
