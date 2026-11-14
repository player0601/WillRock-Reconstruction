// cam_trans.cpp -- Will Rock 1.2 (v12.exe): camCAMERA perspective / orthographic polygon projection
//
// Reconstructed from the Feb 2003 build (m3d:cam_trans.obj); all functions byte-identical in 1.2.
// v12 range 0x00579E10 .. 0x0057A09F (feb 0x005614F0 .. 0x0056177F).

#include "cam.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0057A080 (feb 0x00561760) -> v12 0x009AEB14 (feb 0x009637C4)


// v12 0x00579E10  (feb 0x005614F0)
// Perspective projection of a camera-space polygon onto the near plane (x*near/z, y*near/z).
void camCAMERA::ProjPoly(m3dPOLY *in, m2dPOLY *out)
{
    for (int i = 0; i < in->nVert; i++) {
        float k = zNear / in->vert[i].z;
        out->vert[i].x = in->vert[i].x * k;
        out->vert[i].y = in->vert[i].y * k;
    }
    out->nVert = in->nVert;
}


// v12 0x00579E70  (feb 0x00561550)
// Perspective projection of one camera-space point onto the near plane.
void camCAMERA::ProjPoint(m3dV *in, m2dV *out)
{
    out->x = (in->x / in->z) * zNear;
    out->y = (in->y / in->z) * zNear;
}


// v12 0x00579EA0  (feb 0x00561580)
// Inverse of ProjPoly: casts a ray from the eye (camera origin) through each near-plane vertex
// onto the plane of 'plane' (normal n, negated when flipN). 0 if a ray misses (parallel / behind).
BOOL32 camCAMERA::BackProjPoly(m2dPOLY *in, m3dPOLY *plane, m3dPOLY *out, m3dV *n, int flipN)
{
    m3dV org = { 0.0f, 0.0f, 0.0f };
    m3dV nn;
    if (flipN) {
        nn.x = n->x * -1.0f;
        nn.y = n->y * -1.0f;
        nn.z = n->z * -1.0f;
        n = &nn;
    }
    m3dV dir;
    dir.z = zNear;
    for (int i = 0; i < in->nVert; i++) {
        dir.x = in->vert[i].x;
        dir.y = in->vert[i].y;
        // m3dIsectRayPlane(org, dir, poly, hit OUT, normal, t)
        if (!m3dIsectRayPlane(&org, &dir, plane, &out->vert[i], n, 0))
            return 0;
    }
    out->nVert = in->nVert;
    return 1;
}


// v12 0x00579F80  (feb 0x00561660)
// Orthographic projection: drops z.
void __fastcall camCAMERA::ProjPolyOrtho(m3dPOLY *in, m2dPOLY *out)
{
    for (int i = 0; i < in->nVert; i++) {
        out->vert[i].x = in->vert[i].x;
        out->vert[i].y = in->vert[i].y;
    }
    out->nVert = in->nVert;
}


// v12 0x00579FC0  (feb 0x005616A0)
// Inverse of ProjPolyOrtho: rays along +Z from (x, y, 0) onto the plane of 'plane'.
BOOL32 __fastcall camCAMERA::BackProjPolyOrtho(m2dPOLY *in, m3dPOLY *plane, m3dPOLY *out, m3dV *n, int flipN)
{
    m3dV nn;
    if (flipN) {
        nn.x = n->x * -1.0f;
        nn.y = n->y * -1.0f;
        nn.z = n->z * -1.0f;
        n = &nn;
    }
    for (int i = 0; i < in->nVert; i++) {
        m3dV org = { in->vert[i].x, in->vert[i].y, 0.0f };
        if (!m3dIsectRayPlane(&org, &m3dVUnitZ, plane, &out->vert[i], n, 0))
            return 0;
    }
    out->nVert = in->nVert;
    return 1;
}
