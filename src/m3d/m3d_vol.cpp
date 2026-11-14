// m3d_vol.cpp -- Will Rock 1.2 (v12.exe): m3dVOL, the abstract bounding volume (factory + default QualPlane)
//
// Reconstructed from the Feb 2003 build (m3d:m3d_vol.obj) and checked against v12.
// v12 range 0x005698B0 .. 0x00569AF2 (feb 0x00554260 .. 0x005544A0). All three functions are unchanged in 1.2.
// NB in 1.2 m3dBOX and m3dOBB got their own QualPlane, so the default below is now only reached by PRISM.

#include "m3d_vol.h"
#include <new>

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00569AE0 (feb 0x00554490) -> v12 0x009AEACC

static const float VOL_QUALPLANE_SPH_K = 0.7f;   // __real@3f333333  v12 0x005F9808


// v12 0x004F2F50 (??_Em3dVOL, slot 0 of every volume vtable; emitted in Animbvol.obj)
// The deleting destructor shared by all volume types: nothing to free.
void *m3dVOL::__vecDelDtor(unsigned flags)
{
    if (flags & 1)
        operator delete(this);
    return this;
}

// v12 0x005698B0  (feb 0x00554260)
// (static __fastcall in the original, "SI" mangling; the SDK declares it without the convention)
// Factory: allocates the volume of the given m3dVOL_TYPE (no constructor; only vtable + type are set).
m3dVOL *m3dVOL::Make(int type)
{
    // The original is `new m3dBOX` etc. with the inline default ctors; written out so the abstract SDK
    // classes do not need to be instantiable here.
    size_t       size;
    const void  *vtbl;
    switch (type) {
    case M3D_VOL_BOX:    size = 0x20; vtbl = m3dBOX_vftable;    break;
    case M3D_VOL_PRISM:  size = 0xB4; vtbl = m3dPRISM_vftable;  break;
    case M3D_VOL_SPHERE: size = 0x18; vtbl = m3dSPHERE_vftable; break;
    case M3D_VOL_OBB:    size = 0x38; vtbl = m3dOBB_vftable;    break;
    default:             return 0;
    }
    m3dVOL *v = (m3dVOL *)::operator new(size);
    if (!v)
        return 0;
    v->type = type;
    *(const void **)v = vtbl;
    return v;
}


// v12 0x00569950  (feb 0x00554300)
// Default plane classification: the enclosing sphere shrunk to 70% is classified against the plane.
// TWEAK: the 0.7 factor (v12 0x005F9808) makes PRISM volumes cull more aggressively than their real extent.
int m3dVOL::QualPlane(m3dV *planePt, m3dV *planeN)
{
    m3dSPHERE_QP sph;
    sph.vtbl = m3dSPHERE_vftable;
    sph.type = M3D_VOL_SPHERE;
    CalcEnclSph((m3dSPHERE *)&sph);                 // virtual slot +0x1C
    sph.radius *= VOL_QUALPLANE_SPH_K;
    return sph.QualPlane(planePt, planeN);
}


// v12 0x005699A0  (feb 0x00554350)
// Is p inside the cone with apex `apex`, axis `dir`, half-angle maxAng (degrees)?
// Points closer than minDist always belong, points farther than maxDist never.
BOOL32 __fastcall m3dIsBelongPointCone(m3dV *p, m3dV *apex, m3dV *dir, float maxDist, float minDist, float maxAng)
{
    m3dV d = { apex->x - p->x, apex->y - p->y, apex->z - p->z };
    float dist = m3dSqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (dist < minDist)
        return 1;
    if (dist > maxDist)
        return 0;
    m3dV v = { p->x - apex->x, p->y - apex->y, p->z - apex->z };
    float ang = m3dAngleVector(&v, dir);
    return m3dFAbs(ang) <= maxAng;
}
