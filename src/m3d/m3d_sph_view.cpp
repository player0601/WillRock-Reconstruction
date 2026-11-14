// m3d_sph_view.cpp -- m3dSPHERE's point and plane tests on the stack views m3dSPHERE_TMP / m3dSPHERE_QP
// (m3d_box.h / m3d_vol.h): the same code as m3dSPHERE::IsBelongPoint / QualPlane (m3d_sph.cpp).

#include "m3d_box.h"
#include "m3d_vol.h"
#include <math.h>

// v12 0x0056F670  (feb 0x00557560): within the radius (on it counts)
BOOL32 m3dSPHERE_TMP::IsBelongPoint(m3dV *p)
{
    float dx = center.x - p->x, dy = center.y - p->y, dz = center.z - p->z;
    return sqrtf(dz * dz + dy * dy + dx * dx) <= radius;
}

// v12 0x0056FF80  (feb 0x00557E70) exact
// 0: the plane cuts it; -1 wholly on the normal's side, 1 wholly behind.
int m3dSPHERE_QP::QualPlane(m3dV *pt, m3dV *n)
{
    float d = (center.z - pt->z) * n->z + (center.y - pt->y) * n->y + (center.x - pt->x) * n->x;
    if (fabsf(d) <= radius)
        return 0;
    return d < 0.0f ? 1 : -1;
}
