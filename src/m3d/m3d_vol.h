// m3d_vol.h -- Will Rock 1.2 reconstruction: private declarations of m3d_vol.obj (batch 02_m3d)
#pragma once
#include "m3d_lib.h"

// SDK gap: m3dSPHERE does not declare its QualPlane override (v12 0x0056FF80, feb m3d_sph.obj) and
// is abstract in the SDK, so m3dVOL::QualPlane builds a stack sphere through this view.
struct m3dSPHERE_QP {
    const void *vtbl;           // +0x00 m3dSPHERE vtable  v12 0x005FB68C (feb 0x005D0930)
    int   type;                 // +0x04 M3D_VOL_SPHERE
    m3dV  center;               // +0x08
    float radius;               // +0x14
    int   QualPlane(m3dV *planePt, m3dV *planeN);   // = m3dSPHERE::QualPlane, direct (non-virtual) call
};

// vtables written by m3dVOL::Make (rdata of the other volume objs)
extern const void *const m3dBOX_vftable[];     // v12 0x005F9B68 (feb 0x005CEE44)
extern const void *const m3dPRISM_vftable[];   // v12 0x00607110 (feb 0x005DBC24)
extern const void *const m3dSPHERE_vftable[];  // v12 0x005FB68C (feb 0x005D0930)
extern const void *const m3dOBB_vftable[];     // v12 0x00607784 (feb 0x005DBBD8)
#ifdef WR_NATIVE
// Native builds (no game exe): GCC's own vtables of these classes, same slot order as the game's (slot 0 is the
// deleting destructor, math_app/m3d.h). m3dVOL is abstract (no vtable); destructors store the box's.
extern "C" const void *const _ZTV6m3dBOX[], *const _ZTV8m3dPRISM[], *const _ZTV9m3dSPHERE[], *const _ZTV6m3dOBB[];
#define m3dBOX_vftable    (_ZTV6m3dBOX + 2)
#define m3dPRISM_vftable  (_ZTV8m3dPRISM + 2)
#define m3dSPHERE_vftable (_ZTV9m3dSPHERE + 2)
#define m3dOBB_vftable    (_ZTV6m3dOBB + 2)
#define m3dVOL_vftable    (_ZTV6m3dBOX + 2)
#endif

BOOL32 __fastcall m3dIsBelongPointCone(m3dV *p, m3dV *apex, m3dV *dir, float maxDist, float minDist, float maxAng);
