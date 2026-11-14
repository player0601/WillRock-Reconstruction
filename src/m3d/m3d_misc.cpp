// m3d_misc.cpp -- Will Rock 1.2 (v12.exe): assorted XZ-plane polygon helpers of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_misc.obj) and checked against v12.
// v12 range 0x0056F0D0 .. 0x0056F670 (feb 0x00556FB0 .. 0x00557553). No logic change in 1.2.

#include "m3d_geom.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0056F650 (feb 0x00557540) -> v12 0x009AEADC


// v12 0x0056F0D0  (feb 0x00556FB0)   (no automatic match; located by identical prologue after _$E2 of m3d_rand)
// Index of the vertex nearest to p (-1 when n == 0 or every vertex is farther than M3D_INFINITY).
int __fastcall m3dGetClosestVert(int n, m3dV *verts, m3dV *p)
{
    int   best  = -1;
    float bestD = M3D_INFINITY;
    for (int i = 0; i < n; i++) {
        float d = m3dDist(p, &verts[i]);
        if (d < bestD) {
            bestD = d;
            best  = i;
        }
    }
    return best;
}


// v12 0x0056F1D0  (feb 0x005570C0)
// Exact test: does the circle (c, r) in the XZ plane touch the polygon outline?
// (any vertex inside the circle, or the perpendicular foot on any edge -- closing edge included -- within r)
int __fastcall _m3xzIsIsectPolyCircle(m3dPOLY *poly, m3dV *c, float r)
{
    float r2 = r * r;
    int   n  = poly->nVert;
    for (int i = 0; i < n; i++) {
        float dx = poly->vert[i].x - c->x, dz = poly->vert[i].z - c->z;
        if (dx * dx + dz * dz < r2)
            return 1;
    }
    int i = 0;
    for (; i < n - 1; i++) {
        m3dV *a = &poly->vert[i], *b = &poly->vert[i + 1];
        float ex = b->x - a->x, ez = b->z - a->z;
        float t  = ((c->z - a->z) * ez + (c->x - a->x) * ex) / (ex * ex + ez * ez);
        if (0.0f < t && t < 1.0f) {
            float dx = (ex * t + a->x) - c->x, dz = (ez * t + a->z) - c->z;
            if (dx * dx + dz * dz < r2)
                return 1;
        }
    }
    // closing edge vert[n-1] -> vert[0]
    m3dV *a = &poly->vert[i];
    float ex = poly->vert[0].x - a->x, ez = poly->vert[0].z - a->z;
    float t  = ((c->z - a->z) * ez + (c->x - a->x) * ex) / (ex * ex + ez * ez);
    if (0.0f < t && t < 1.0f) {
        float dx = (ex * t + a->x) - c->x, dz = (ez * t + a->z) - c->z;
        if (dx * dx + dz * dz < r2)
            return 1;
    }
    return 0;
}


// v12 0x0056F3A0  (feb 0x00557290)
// Circle (c, r) vs polygon in XZ: centre inside -> 1; otherwise a quick reject with the XZ bounding
// box's circumcircle, then the exact edge test above.
// 1.2: same logic (inlined m3dSqrt rescheduled).
int __fastcall m3xzIsIsectPolyCircle(m3dPOLY *poly, m3dV *c, float r)
{
    m3dBOX box;                                        // stack object (vtable m3dBOX, type 1)
    box.type = M3D_VOL_BOX;
    if (m3xzIsBelongPointPoly_A(poly, c, M3D_EPSILON))
        return 1;
    box.CalcXZ(poly->vert, poly->nVert, 0.0f);
    float cx = (box.vMax.x + box.vMin.x) * 0.5f;
    float cz = (box.vMax.z + box.vMin.z) * 0.5f;
    float dx = cx - c->x, dz = cz - c->z;
    float distC = m3dSqrt(dx * dx + dz * dz);          // box centre -> circle centre
    dx = cx - box.vMin.x; dz = cz - box.vMin.z;
    float boxR = m3dSqrt(dx * dx + dz * dz);           // box half diagonal
    if (boxR + r < distC)
        return 0;
    if (!_m3xzIsIsectPolyCircle(poly, c, r))
        return 0;
    return 1;
}


// v12 0x0056F5A0  (feb 0x00557490)
// Squared XZ distance from p to the polygon outline (all edges, closing edge first) and the closest point.
void __fastcall _m3xzDist2PointPoly(m3dV *p, m3dPOLY *poly, m3dV *closest, float *d)
{
    m3dV  best, cur;
    float bestD = m3xzDistPointLSeg_2(p, &poly->vert[poly->nVert - 1], &poly->vert[0], &best);
    for (int i = 0; i < poly->nVert - 1; i++) {
        float dd = m3xzDistPointLSeg_2(p, &poly->vert[i], &poly->vert[i + 1], &cur);
        if (dd < bestD) {
            best  = cur;
            bestD = dd;
        }
    }
    if (d)
        *d = bestD;
    if (closest)
        *closest = best;
}
