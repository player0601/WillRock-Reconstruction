// m3d_obb.cpp -- Will Rock 1.2 (v12.exe): oriented bounding box (m3dOBB) of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_obb.obj) and checked against v12.
// v12 range 0x00578190 .. 0x00579D80 (feb 0x0055FC20 .. 0x00561460); vtable v12 0x00607784
// (feb 0x005DBBD8). An OBB is a corner 'org' plus three full-length, mutually orthogonal edge
// vectors ax[0..2]. Most queries move the problem into the box's own frame (GetMatrW2L) and reuse
// the axis-aligned m3dBOX code (m3d_box.obj).
//
// 1.2 changes:
//   - NEW vtable slot +0x30 override m3dOBB::QualPlane (v12 0x00578E50): exact plane test through the
//     local box (feb used m3dVOL::QualPlane, a shrunken enclosing sphere).
//   - GetFace now also returns the outward face normal (un-normalised edge vector) and accepts a NULL
//     polygon pointer.
// Vertex order: 0 org, 1 org+a0, 2 org+a0+a1, 3 org+a1, 4..7 = 0..3 + a2.

#include "m3d_geom.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00579B90 (feb 0x00561350) -> v12 0x009AEB10

// faces as vertex indices (same table as m3dPRISM::GetFace)
static const unsigned char s_obbFace[6][4] = {
    { 3, 7, 4, 0 }, { 2, 6, 7, 3 }, { 5, 6, 2, 1 }, { 4, 5, 1, 0 }, { 0, 1, 2, 3 }, { 7, 6, 5, 4 }
};

static inline void m3dBoxInit(m3dBOX *b) { b->type = M3D_VOL_BOX; }   // stack m3dBOX: vtable + type 1


// v12 0x00578190  (feb 0x0055FC20)  vtable +0x04
// Point inside: transformed into the local frame and tested against the local box [0, |a_i|].
BOOL32 m3dOBB::IsBelongPoint(m3dV *p)
{
    m3dBOX  box;
    m3dMATR w2l;
    m3dBoxInit(&box);
    GetMatrW2L(&w2l, 0, &box);
    m3dV lp;
    m3dTransformPoint(p, &w2l, &lp);
    return box.m3dBOX::IsBelongPoint(&lp);
}


// v12 0x00578350  (feb 0x0055FDE0)  vtable +0x08
// Swept sphere (org -> along dir up to len, radius r): ray against the local box grown by r.
BOOL32 m3dOBB::IsIsectCyl(m3dV *org, m3dV *dir, float len, float r)
{
    m3dBOX  box;
    m3dMATR w2l;
    m3dBoxInit(&box);
    GetMatrW2L(&w2l, 0, &box);
    box.m3dBOX::Expand(r);
    m3dV lo, ld;
    m3dTransformPoint(org, &w2l, &lo);
    m3dTransformVector(dir, &w2l, &ld);
    return box.m3dBOX::IsectRay(&lo, &ld, len, 0, 0, 0) != 0;
}


// v12 0x00578660  (feb 0x005600F0)  vtable +0x0C
// Ray cast through the local box; the hit point is transformed back to world space
// (t / face come straight from m3dBOX::IsectRay; the frame is orthonormal so t is unchanged).
// 1.2: same logic (inline transforms rescheduled).
BOOL32 m3dOBB::IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face)
{
    m3dBOX  box;
    m3dMATR w2l, l2w;
    m3dBoxInit(&box);
    GetMatrW2L(&w2l, &l2w, &box);
    m3dV lo, ld;
    m3dTransformPoint(org, &w2l, &lo);
    m3dTransformVector(dir, &w2l, &ld);
    BOOL32 r = box.m3dBOX::IsectRay(&lo, &ld, maxT, hit, t, face);
    if (r && hit) {
        m3dV w;
        m3dTransformPoint(hit, &l2w, &w);
        *hit = w;
    }
    return r;
}


// v12 0x00578B00  (feb 0x00560590)  vtable +0x10
// Distance to the surface through the local box; closest point transformed back to world space.
BOOL32 m3dOBB::DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face)
{
    m3dBOX  box;
    m3dMATR w2l, l2w;
    m3dBoxInit(&box);
    GetMatrW2L(&w2l, &l2w, &box);
    m3dV lp;
    m3dTransformPoint(p, &w2l, &lp);
    BOOL32 r = box.m3dBOX::DistPoint(&lp, maxD, closest, d, face);
    if (r && closest) {
        m3dV w;
        m3dTransformPoint(closest, &l2w, &w);
        *closest = w;
    }
    return r;
}


// v12 0x00548680 (shared 'return 1' stub)  vtable +0x18
// Always valid (inline in the header; COMDAT-folded with other 'return 1' functions).
BOOL32 m3dOBB::Check()
{
    return 1;
}


// v12 0x00578E50  (feb -- )  vtable +0x30
// 1.2: NEW override. Side of the plane (pp, pn), exact: the plane is moved into the local frame and
// m3dBOX::QualPlane (v12 0x0056E660, also new in 1.2) tests the 8 corners.
// 0 = crossing, 1 = behind, -1 = in front.
int m3dOBB::QualPlane(m3dV *pp, m3dV *pn)
{
    m3dBOX  box;
    m3dMATR w2l;
    m3dBoxInit(&box);
    GetMatrW2L(&w2l, 0, &box);
    m3dV lp, ln;
    m3dTransformPoint(pp, &w2l, &lp);
    m3dTransformVector(pn, &w2l, &ln);
    return box.m3dBOX::QualPlane(&lp, &ln);
}


// v12 0x00579140  (feb 0x005608E0)
// World->local matrix (origin org, unit edge directions), optional local->world matrix and the
// local box [0,0,0]..[|a0|,|a1|,|a2|]. l2w and localBox may be NULL.
// 1.2: same logic (inlined m3dSqrt rescheduled).
void m3dOBB::GetMatrW2L(m3dMATR *w2l, m3dMATR *l2w, m3dBOX *localBox)
{
    float l0 = m3dLength(&ax[0]);
    float l1 = m3dLength(&ax[1]);
    float l2 = m3dLength(&ax[2]);
    m3dV u0, u1, u2;
    m3dScale(&ax[0], 1.0f / l0, &u0);
    m3dScale(&ax[1], 1.0f / l1, &u1);
    m3dScale(&ax[2], 1.0f / l2, &u2);
    w2l->MakeWCS2LCS(&org, &u0, &u1, &u2);
    if (l2w)
        l2w->MakeLCS2WCS(&org, &u0, &u1, &u2);
    if (localBox) {
        localBox->vMin   = m3dVZero;
        localBox->vMax.x = l0;
        localBox->vMax.y = l1;
        localBox->vMax.z = l2;
    }
}


// v12 0x00579440  (feb 0x00560BF0)  vtable +0x1C
// Enclosing sphere: centre of the box, radius = distance centre -> corner.
// 1.2: same logic (inlined m3dSqrt rescheduled).
void m3dOBB::CalcEnclSph(m3dSPHERE *out)
{
    GetCenter(&out->center);
    out->radius = m3dDist(&out->center, &org);
}


// v12 0x00579530  (feb 0x00560CE0)  vtable +0x20
// All 8 corners inside the sphere grown by 100 eps.
BOOL32 m3dOBB::CheckEnclSph(m3dSPHERE *s)
{
    m3dSPHERE grown;
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


// v12 0x005796B0  (feb 0x00560E60)  vtable +0x24
// Centre = org + (a0 + a1 + a2) / 2.
void m3dOBB::GetCenter(m3dV *out)
{
    out->x = ax[0].x * 0.5f + org.x;
    out->y = ax[0].y * 0.5f + org.y;
    out->z = ax[0].z * 0.5f + org.z;
    out->x = ax[1].x * 0.5f + out->x;
    out->y = ax[1].y * 0.5f + out->y;
    out->z = ax[1].z * 0.5f + out->z;
    out->x = ax[2].x * 0.5f + out->x;
    out->y = ax[2].y * 0.5f + out->y;
    out->z = ax[2].z * 0.5f + out->z;
}


// v12 0x00579740  (feb 0x00560EF0)  vtable +0x28
// Face i (0..5) as a 4-vertex polygon.
// 1.2: 'normal' (when not NULL) receives the outward edge vector of the face (-a0, a1, a0, -a1,
//      -a2, a2 for faces 0..5; not normalised), and 'out' may now be NULL.
void m3dOBB::GetFace(int i, m3dPOLY *out, m3dV *normal)
{
    if (normal && 0 <= i && i <= 5) {                  // 1.2: new block
        switch (i) {
        case 0: m3dScale(&ax[0], -1.0f, normal); break;
        case 1: *normal = ax[1]; break;
        case 2: *normal = ax[0]; break;
        case 3: m3dScale(&ax[1], -1.0f, normal); break;
        case 4: m3dScale(&ax[2], -1.0f, normal); break;
        case 5: *normal = ax[2]; break;
        }
    }
    if (!out)                                          // 1.2: NULL polygon allowed
        return;
    m3dV v[8];
    GetVertices(v);
    if (0 <= i && i <= 5)
        for (int k = 0; k < 4; k++)
            out->vert[k] = v[s_obbFace[i][k]];
    out->nVert = 4;
}


// v12 0x00579BB0  (feb 0x00561270)
// The 8 corners (order in the file header).
void m3dOBB::GetVertices(m3dV out[8])
{
    out[0] = org;
    m3dAdd(&org, &ax[0], &out[1]);
    m3dAdd(&ax[1], &out[1], &out[2]);
    m3dAdd(&ax[1], &out[0], &out[3]);
    m3dAdd(&ax[2], &out[0], &out[4]);
    m3dAdd(&out[1], &ax[2], &out[5]);
    m3dAdd(&out[2], &ax[2], &out[6]);
    m3dAdd(&out[3], &ax[2], &out[7]);
}


// v12 0x00579C90  (feb 0x00561370)  vtable +0x2C
// Grow by d on every side: corner moves back by d along each axis, every edge gets 2d longer.
void m3dOBB::Expand(float d)
{
    m3dV u[3];
    for (int i = 0; i < 3; i++) {
        u[i] = ax[i];
        m3dNormalize(&u[i]);
    }
    for (int i = 0; i < 3; i++)
        m3dAddScaled(&org, &u[i], -d, &org);
    for (int i = 0; i < 3; i++)
        m3dAddScaled(&ax[i], &u[i], d + d, &ax[i]);
}
