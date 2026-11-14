// m3d_sph.cpp -- Will Rock 1.2 (v12.exe): bounding sphere (m3dSPHERE) of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_sph.obj) and checked against v12.
// v12 range 0x0056F670 .. 0x00570000 (feb 0x00557560 .. 0x00557EF3); vtable v12 0x005FB68C (feb 0x005D0930).
// No logic change in 1.2 (several functions differ only by the rescheduled inline m3dSqrt).
// GetCenter (inline, v12 0x00422550 copy) and an empty GetFace (folded stub v12 0x0051C0D0) live in the header.

#include "m3d_geom.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0056FFF0 (feb 0x00557EE0) -> v12 0x009AEAE0


// v12 0x0056F670  (feb 0x00557560)  vtable +0x04
// Point inside or on the sphere.
BOOL32 m3dSPHERE::IsBelongPoint(m3dV *p)
{
    if (radius < m3dDist(&center, p))
        return 0;
    return 1;
}


// v12 0x0056F750  (feb 0x00557640)  vtable +0x08
// Swept-sphere test: does a sphere of radius r moving from org along dir (up to len) touch this sphere?
// Implemented by growing the radius by r and casting a ray.
BOOL32 m3dSPHERE::IsIsectCyl(m3dV *org, m3dV *dir, float len, float r)
{
    radius += r;
    BOOL32 hit = IsectRay(org, dir, len, 0, 0, 0);
    radius -= r;
    return hit != 0;
}


// v12 0x0056F790  (feb 0x00557680)  vtable +0x0C
// Ray org + t*dir. Returns 2 when org is inside (hit = org, t = 0), 1 on a hit not farther than maxT
// (t = DISTANCE from org, face = -1), else 0.
BOOL32 m3dSPHERE::IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face)
{
    float ox = org->x - center.x, oy = org->y - center.y, oz = org->z - center.z;
    float c  = ox * ox + oy * oy + oz * oz;
    float r2 = radius * radius;
    if (c < r2) {
        if (t)
            *t = 0.0f;
        if (hit)
            *hit = *org;
        return M3D_ISECT_INSIDE;
    }
    float a    = m3dDot(dir, dir);
    float b    = oz * dir->z + oy * dir->y + ox * dir->x;
    float disc = b * b - (c - r2) * a;
    if (disc < 0.0f)
        return 0;
    float s    = m3dSqrt(disc);
    float inva = 1.0f / a;
    float t1   = (s - b) * inva;
    float t0   = (-b - s) * inva;
    if (0.0f <= t1) {
        if (t0 <= 0.0f || t1 < t0)
            t0 = t1;
    } else if (t0 < 0.0f) {
        return 0;
    }
    m3dV h;
    m3dAddScaled(org, dir, t0, &h);
    if (hit)
        *hit = h;
    float d = m3dDist(&h, org);
    if (d <= maxT) {
        if (t)
            *t = d;
        if (face)
            *face = -1;
        return M3D_ISECT_HIT;
    }
    return 0;
}


// v12 0x0056FAA0  (feb 0x00557990)  vtable +0x10
// Distance from p to the surface. 2 when p is inside, 1 when within maxD (closest point on the
// surface, face = -1), else 0.
BOOL32 m3dSPHERE::DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face)
{
    float l = m3dDist(p, &center);
    if (l <= radius)
        return M3D_ISECT_INSIDE;
    if (l - radius > maxD)
        return 0;
    if (d)
        *d = l - radius;
    if (closest) {
        float k = 1.0f / l;
        closest->x = (p->x - center.x) * radius * k + center.x;
        closest->y = (p->y - center.y) * radius * k + center.y;
        closest->z = (p->z - center.z) * radius * k + center.z;
    }
    if (face)
        *face = -1;
    return 1;
}


// v12 0x0056FC00  (feb 0x00557AF0)  vtable +0x18
// Valid when the radius is positive.
BOOL32 m3dSPHERE::Check()
{
    if (M3D_EPSILON < radius)
        return 1;
    return 0;
}


// v12 0x0056FC20  (feb 0x00557B10)  vtable +0x1C
// The enclosing sphere is the sphere itself (copies type, centre, radius).
void m3dSPHERE::CalcEnclSph(m3dSPHERE *out)
{
    out->type   = type;
    out->center = center;
    out->radius = radius;
}


// v12 0x0056FC50  (feb 0x00557B40)  vtable +0x20
// Same centre and radius within 10 eps.
BOOL32 m3dSPHERE::CheckEnclSph(m3dSPHERE *s)
{
    float eps = M3D_EPSILON * 10.0f;
    if (m3dFAbs(s->center.x - center.x) < eps &&
        m3dFAbs(s->center.y - center.y) < eps &&
        m3dFAbs(s->center.z - center.z) < eps)
        return m3dFAbs(s->radius - radius) <= eps;
    return 0;
}


// v12 0x0056FD70  (feb 0x00557C60)  vtable +0x14
// Fit to a point list: centre = midpoint of the two farthest points, radius = largest distance to it
// (+100 eps). O(n^2).
// 1.2: same logic (inlined m3dSqrt rescheduled).
void m3dSPHERE::Calc(m3dV *pts, int n)
{
    float best = -1.0f;
    int   i0 = 0, i1 = 0;
    if (n == 1) {
        i0 = 0;
        i1 = 0;
    } else {
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++) {
                float d = m3dDist2(&pts[i], &pts[j]);
                if (best < d) {
                    i0   = i;
                    best = d;
                    i1   = j;
                }
            }
    }
    center.x = (pts[i1].x + pts[i0].x) * 0.5f;
    center.y = (pts[i1].y + pts[i0].y) * 0.5f;
    center.z = (pts[i1].z + pts[i0].z) * 0.5f;
    float r2 = m3dDist2(&pts[i0], &center);
    for (int i = 0; i < n; i++) {
        float d = m3dDist2(&center, &pts[i]);
        if (r2 <= d)
            r2 = d;
    }
    radius = M3D_EPSILON * 100.0f + m3dSqrt(r2);
}


// v12 0x0056FF70  (feb 0x00557E60)  vtable +0x2C
// Grow the radius by d.
void m3dSPHERE::Expand(float d)
{
    radius += d;
}


// v12 0x0056FF80  (feb 0x00557E70)  vtable +0x30
// Side of the plane (pp, unit pn): 0 = crosses (|dist| < r), 1 = behind, -1 = in front.
int m3dSPHERE::QualPlane(m3dV *pp, m3dV *pn)
{
    float d = (center.x - pp->x) * pn->x + (center.y - pp->y) * pn->y + (center.z - pp->z) * pn->z;
    if (m3dFAbs(d) < radius)
        return M3D_QUAL_CROSS;
    if (d < 0.0f)
        return M3D_QUAL_BACK;
    return M3D_QUAL_FRONT;
}
