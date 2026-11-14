// m3d_lib.h -- Will Rock 1.2 (v12.exe) reconstructed source: private header of library "m3d"
// (original folder C:\PROJECTS\WillRock\Sources\..\m3d\, objs cam, cam_qual, cam_trans, cgm_*, m2d_matr,
//  m3d, m3d_box, m3d_clip, m3d_ctrl, m3d_lerp).
//
// Declarations the unofficial SDK (src_recon/include/math_app/m3d.h) does not carry. The SDK is
// included as-is; everything here is additive. Layout workarounds are listed in
// re/work/recon_fixes/00_m3d.md.
#pragma once
#include "math_app/m3d.h"
#include "engine_core/core_types.h"   // apLog, apMalloc/apFree/apRealloc, _apCfgReadBool

#ifndef WR_TODO
#define WR_TODO(a) ((void)0)
#endif

// ---------------------------------------------------------------------------------------------
// Library constants (every m3d .obj has its own copy in .rdata)
// ---------------------------------------------------------------------------------------------
#define M3D_EPSILON     1e-6f            // 0x358637BD   (v12 e.g. 0x00609A50)
#define M3D_INFINITY    2000000.0f       // 0x49F42400   "big" value used for min/max starts
#define M3D_PI          3.14159265f      // 0x40490FDB
#define M3D_DEG2RAD     (M3D_PI / 180.0f)
#define M3D_RAD2DEG     (180.0f / M3D_PI)

// Every m3d .cpp includes the library header that defines
//     static float m3dEpsSq = M3D_EPSILON * M3D_EPSILON;
// MSVC emits it as a per-object dynamic initializer "_$E2" (fld eps; fmul eps; fstp m3dEpsSq).
// We keep one per .cpp, exactly like the original.
#define M3D_DECLARE_EPS_SQ()  static float m3dEpsSq = M3D_EPSILON * M3D_EPSILON

// x87 helpers (the game uses fsqrt / fsin / fcos / fpatan / fabs directly)
inline float m3dFAbs(float v)          { return __builtin_fabsf(v); }
inline float m3dSin(float v)           { return __builtin_sinf(v); }
inline float m3dCos(float v)           { return __builtin_cosf(v); }
inline float m3dTan(float v)           { return __builtin_tanf(v); }
inline float m3dATan(float v)          { return __builtin_atanf(v); }
inline float m3dATan2(float y, float x){ return __builtin_atan2f(y, x); }
inline float m3dACos(float v)          { return __builtin_acosf(v); }
inline float m3dMin(float a, float b)  { return a < b ? a : b; }
inline float m3dMax(float a, float b)  { return a > b ? a : b; }

// m3dNormalize: inlined in every .obj (static copy, branches on m3dSimdType). Plain x87 version.
inline void m3dNormalize(m3dV *v)
{
    float l = m3dSqrt(v->x * v->x + v->y * v->y + v->z * v->z);
    if (l > M3D_EPSILON) { float k = 1.0f / l; v->x *= k; v->y *= k; v->z *= k; }
}
// _m3dNormalize: the other inline copy (e.g. feb 0x00412710 plw_mng.obj): returns 0, vector untouched, when
// the squared length is below eps^2.
inline int _m3dNormalize(m3dV *v)
{
    float l2 = v->x * v->x + v->y * v->y + v->z * v->z;
    if (l2 < M3D_EPSILON * M3D_EPSILON)
        return 0;
    float k = 1.0f / m3dSqrt(l2);
    v->x *= k; v->y *= k; v->z *= k;
    return 1;
}
inline void m3dCrossProduct(const m3dV *a, const m3dV *b, m3dV *out)
{
    m3dV r = { a->y * b->z - a->z * b->y, a->z * b->x - a->x * b->z, a->x * b->y - a->y * b->x };
    *out = r;
}
inline float m3dDotProduct(const m3dV *a, const m3dV *b) { return a->x * b->x + a->y * b->y + a->z * b->z; }
inline float m3dDist(const m3dV *a, const m3dV *b)       { return m3dSqrt(dist2(*a, *b)); }

// Transform helpers (row vectors, p' = p*M)
inline void m3dTransformPoint(const m3dV *p, const m3dMATR *m, m3dV *o)
{
    m3dV r = { p->x * m->m[0][0] + p->y * m->m[1][0] + p->z * m->m[2][0] + m->m[3][0],
               p->x * m->m[0][1] + p->y * m->m[1][1] + p->z * m->m[2][1] + m->m[3][1],
               p->x * m->m[0][2] + p->y * m->m[1][2] + p->z * m->m[2][2] + m->m[3][2] };
    *o = r;
}
inline void m3dTransformVector(const m3dV *p, const m3dMATR *m, m3dV *o)
{
    m3dV r = { p->x * m->m[0][0] + p->y * m->m[1][0] + p->z * m->m[2][0],
               p->x * m->m[0][1] + p->y * m->m[1][1] + p->z * m->m[2][1],
               p->x * m->m[0][2] + p->y * m->m[1][2] + p->z * m->m[2][2] };
    *o = r;
}

// ---------------------------------------------------------------------------------------------
// Library functions used by this batch but defined in other m3d objs (declared in the SDK or here)
// ---------------------------------------------------------------------------------------------
BOOL32 __fastcall m3dInit2DMatr();     // m2d_matr.obj   v12 0x0057C420 (feb 0x00563B00)
BOOL32 __fastcall m3dInit3DMatr();     // m3d_matr.obj   v12 0x005609A0 (feb 0x005496C0)
BOOL32 __fastcall m3dInitNoise();      // m3d_noise.obj  v12 0x00574570
void   __fastcall m3dTermNoise();      // m3d_noise.obj  v12 0x00574660 (feb 0x0055C0A0)
BOOL32 __fastcall m3dInitHW();         // m3d.obj        v12 0x00569600 (feb 0x00551570)
BOOL32 __fastcall _m3dCheckValid(const m3dVTX *v);   // m3d.obj v12 0x005696A0

// m2d/m3xz/m3xy/m3yz point-in-polygon family (m3d_clip.obj)
int    __fastcall m2dQualPointLine_A(m2dV *p, m2dV *a, m2dV *b, float eps);
BOOL32 __fastcall m2dIsBelongPointTri_A(m2dTRI *t, m2dV *p, float eps);
BOOL32 __fastcall m2dIsBelongPointPoly_A(m2dPOLY *poly, m2dV *p, float eps);
// m3d_poly.obj / m3d_rand.obj (also in m3d_geom.h; repeated for the libraries that use the SDK m3d.h)
void   __fastcall m2dReOrientPoly(m2dPOLY *p);
void   __fastcall m2dTriApplyBasisCoord(m2dTRI *t, float u, float v, m2dV *out);
BOOL32 __fastcall m2dIsCrossLSegPolyBoundary_A(m2dPOLY *p, m2dV *a, m2dV *b, float eps);
void   __fastcall m3dRandBasicCoord(float *u, float *v);
BOOL32 __fastcall m3xzIsBelongPointPoly_A(m3dPOLY *poly, m3dV *p, float eps);
BOOL32 __fastcall m3xyIsBelongPointPoly_A(m3dPOLY *poly, m3dV *p, float eps);
BOOL32 __fastcall m3yzIsBelongPointPoly_A(m3dPOLY *poly, m3dV *p, float eps);
BOOL32 __fastcall _m2dClipPolyRect(m2dPOLY *p, m2dPOLY *rect, m2dPOLY *out, int axis, int side);
BOOL32 __fastcall m3xyClipLSegRect(m3dV *a, m3dV *b, m2dPOLY *rect, m3dV *oa, m3dV *ob);
BOOL32 __fastcall m3dClipLSegNearFar(m3dV *a, m3dV *b, float zNear, float zFar);

// m3d_lerp.obj
BOOL32 __fastcall m3dClampVector_L(m3dV *v, m3dV *ref, float maxLen, m3dV *out);
BOOL32 __fastcall m3dClampVector_L(m3dV *v, m3dV *ref, float maxLen);
BOOL32 __fastcall m3dClampVector_A(m3dV *v, const m3dV *ref, float maxDeg, m3dV *axis, m3dV *out, int flags);
float  __fastcall m3dLerpRectCorners(m2dV *a, m2dV *b, m2dV *c, float, float, float, float);

// cgm_phys.obj
float __fastcall cgmGetThrowSpeed_XZ(m3dV *from, m3dV *to, float g, float h, m3dV *outDir);
float __fastcall cgmGetThrowSpeed_H(m3dV *from, m3dV *to, float g, float h, m3dV *outDir);

// SDK gap: m3dNOISE vtable slot 2 is GetValue(m3dV*) (feb vtable 0x005DE5xx, m3dNOISE_GRAD::GetValue
// v12 0x005748C0). The SDK class stops at Fill(); we call slot 2 through this view.
struct m3dNOISE_VIEW {
    virtual void  *__vecDelDtor(unsigned);
    virtual BOOL32 Fill(int n);
    virtual float  GetValue(m3dV *p);                 // slot +0x08
};
inline float m3dNoiseGetValue(void *noise, m3dV *p) { return ((m3dNOISE_VIEW *)noise)->GetValue(p); }

// SDK gap: m3dSPL::GetValue(float t, void *out, void *deriv) is slot +0x10; SDK has it pure in the class,
// we call it through the SDK class directly.

// Global used by m3dGetSimdType / m3dInitHW
extern int m3dSimdType;                               // v12 0x00637C24 (feb 0x005F99D0)
