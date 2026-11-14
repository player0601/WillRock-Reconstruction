// m3d_geom.h -- Will Rock 1.2 (v12.exe) reconstructed source: header of the m3d geometry objs
// (batch 01: m3d_line, m3d_math, m3d_matr, m3d_misc, m3d_noise, m3d_obb, m3d_poly, m3d_prsm,
//  m3d_quat, m3d_rand, m3d_sph -- original folder C:\PROJECTS\WillRock\Sources\..\m3d\).
//
// Why a separate full header and not math_app/m3d.h: the SDK declares m3dMATR, m3dQUAT, the
// bounding volumes and m3dNOISE with only part of their members, so their out-of-line member
// functions (m3dSPHERE::IsBelongPoint, m3dMATR::MakeProjTri, ...) cannot be defined against it.
// This header repeats the SDK value types with IDENTICAL layouts (sizes asserted below) and gives
// the complete class declarations as they must have been in the original m3d.h. It is NOT meant to
// be included together with math_app/m3d.h. See re/work/recon_fixes/01_m3d.md.
//
// Conventions: "// v12 0x00XXXXXX (feb 0x00XXXXXX)". Free functions are __fastcall (MSVC "YI"),
// members __thiscall. NB MSVC __fastcall passes only the first two INTEGER/POINTER arguments in
// ECX/EDX; float arguments always go on the stack (so m3dSolveQuadraticEq(a,b,c,&x0,&x1) has
// x0 in ECX, x1 in EDX).
#pragma once
#include "core/wr_cc.h"
#include <stddef.h>

#ifndef WR_TODO
#define WR_TODO(a) ((void)0)
#endif

typedef int BOOL32;

// ---------------------------------------------------------------------------------------------
// Library constants (each m3d .obj carries its own .rdata copies)
// ---------------------------------------------------------------------------------------------
#define M3D_EPSILON   1e-6f              // 0x358637BD
#define M3D_INFINITY  2000000.0f         // 0x49F42400 start value of min searches
#define M3D_PI        3.14159265f        // 0x40490FDB
#define M3D_DEG2RAD   (1.0f / 180.0f)    // __real@3bb60b61, always used as  deg * PI * (1/180)
#define M3D_RAND_K    (1.0f / 32767.0f)  // __real@38000100 = 3.0518e-5 (rand() -> [0,1])

// Every m3d .cpp includes the library header that defines
//     static float m3dEpsSq = M3D_EPSILON * M3D_EPSILON;
// MSVC emits it as a per-object dynamic initializer "_$E2" (fld eps; fmul eps; fstp m3dEpsSq).
#define M3D_DECLARE_EPS_SQ()  static float m3dEpsSq = M3D_EPSILON * M3D_EPSILON

// ---------------------------------------------------------------------------------------------
// Value types (same layouts as math_app/m3d.h)
// ---------------------------------------------------------------------------------------------
struct m2dV    { float x, y; };                          // 0x08
struct m3dV    { float x, y, z; };                       // 0x0C
struct m3dVINT { int x, y, z; };                         // 0x0C
struct m2dTRI  { m2dV v[3]; };                           // 0x18

#define M3D_POLY_MAX_VERT 20
class m3dPOLY { public: int nVert; m3dV vert[M3D_POLY_MAX_VERT]; };   // 0xF4
class m2dPOLY { public: int nVert; m2dV vert[M3D_POLY_MAX_VERT]; };   // 0xA4

// x87 helpers (the game uses fsqrt / fsin / fcos / fpatan / fabs directly)
inline float m3dFAbs(float v)           { return __builtin_fabsf(v); }
inline float m3dSin(float v)            { return __builtin_sinf(v); }
inline float m3dCos(float v)            { return __builtin_cosf(v); }
inline float m3dACos(float v)           { return __builtin_acosf(v); }
inline float m3dATan2(float y, float x) { return __builtin_atan2f(y, x); }

// m3dSqrt -- INLINE in the game: SSE (rsqrtss + one Newton step), 3DNow! (pfrsqrt) or fsqrt,
// selected by m3dSimdType. 1.2 recompiled the SSE path with a different register allocation, which
// is the only difference in most "callgraph" matches of this batch. Plain version here.
inline float m3dSqrt(float v)           { return __builtin_sqrtf(v); }
// m3dIntFloor -- inline: fistp(x - 0.49999899) (qqq = 0x3EFFFFDE) == floor for the game's ranges
inline int   m3dIntFloor(float v)       { return (int)__builtin_floorf(v); }

inline float m3dDot(const m3dV *a, const m3dV *b)    { return a->x * b->x + a->y * b->y + a->z * b->z; }
inline float m3dLength2(const m3dV *a)               { return m3dDot(a, a); }
inline float m3dLength(const m3dV *a)                { return m3dSqrt(m3dDot(a, a)); }
inline float m3dDist2(const m3dV *a, const m3dV *b)
{
    float dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;
    return dx * dx + dy * dy + dz * dz;
}
inline float m3dDist(const m3dV *a, const m3dV *b)   { return m3dSqrt(m3dDist2(a, b)); }
inline void  m3dSub(const m3dV *a, const m3dV *b, m3dV *o) { o->x = a->x - b->x; o->y = a->y - b->y; o->z = a->z - b->z; }
inline void  m3dAdd(const m3dV *a, const m3dV *b, m3dV *o) { o->x = a->x + b->x; o->y = a->y + b->y; o->z = a->z + b->z; }
inline void  m3dScale(const m3dV *a, float k, m3dV *o)     { o->x = a->x * k; o->y = a->y * k; o->z = a->z * k; }
inline void  m3dAddScaled(const m3dV *a, const m3dV *d, float k, m3dV *o) // o = a + d*k
{
    o->x = a->x + d->x * k; o->y = a->y + d->y * k; o->z = a->z + d->z * k;
}
inline void  m3dCross(const m3dV *a, const m3dV *b, m3dV *o)
{
    m3dV r = { a->y * b->z - a->z * b->y, a->z * b->x - a->x * b->z, a->x * b->y - a->y * b->x };
    *o = r;
}
// m3dNormalize -- inline, a static copy in every .obj (v12 e.g. 0x00402D80); returns 0 when |v| < eps
inline BOOL32 m3dNormalize(m3dV *v)
{
    float l = m3dSqrt(m3dDot(v, v));
    if (l < M3D_EPSILON) return 0;
    float k = 1.0f / l;
    v->x *= k; v->y *= k; v->z *= k;
    return 1;
}
// _m3dNormalize: the inline copy (feb 0x00412710 is one of its out-of-line instances); the same body as
// m3d_lib.h (this header is never included together with that one)
inline BOOL32 _m3dNormalize(m3dV *v)
{
    float l2 = m3dDot(v, v);
    if (l2 < M3D_EPSILON * M3D_EPSILON)
        return 0;
    float k = 1.0f / m3dSqrt(l2);
    v->x *= k; v->y *= k; v->z *= k;
    return 1;
}

// ---------------------------------------------------------------------------------------------
// m3dMATR -- 4x4, row vectors (p' = p*M); rows X, Y, Z axis, origin
// ---------------------------------------------------------------------------------------------
enum { M3D_OP_SET = 0, M3D_OP_LOCAL = 1, M3D_OP_GLOBAL = 2 };   // "op" of Translate/Rotate/Scale/Transform

class m3dMATR {
public:
    float m[4][4];
    void   Identity();
    void   Translate(m3dV *v, int op);
    void   Translate(float x, float y, float z, int op);
    void   RotateX(float deg, int op);
    void   RotateY(float deg, int op);
    void   RotateZ(float deg, int op);
    void   Scale(float x, float y, float z, int op);
    void   Transform(m3dMATR *t, int op);
    void   GetOrigin(m3dV *o);
    void   GetAxisX(m3dV *o);
    void   GetAxisY(m3dV *o);
    void   GetAxisZ(m3dV *o);
    void   GetTranslate(float *x, float *y, float *z);
    void   GetScale(float *x, float *y, float *z);
    void   MakeScale(float x, float y, float z);       // 1.2: new
    void   MakeRotX(float deg);                        // 1.2: new
    void   MakeRotY(float deg);
    void   MakeRot(m3dV *from, m3dV *to);
    void   MakeRot(m3dV *from, m3dV *to, m3dV *up);
    void   MakeRotRay(m3dV *org, m3dV *dir, float deg);
    void   MakeLCS2WCS(m3dV *org, m3dV *x, m3dV *y, m3dV *z);
    void   MakeWCS2LCS(m3dV *org, m3dV *x, m3dV *y, m3dV *z);
    void   _MakeWCS2LCS_VZ(m3dV *org, m3dV *vz);
    void   MakeLCS2WCS_VZ(m3dV *org, m3dV *vz);
    void   _MakeLCS2WCS_VX(m3dV *org, m3dV *vx);
    void   _MakeLCS2WCS_VZ(m3dV *org, m3dV *vz);
    void   _MakeLCS2WCS_VY(m3dV *org, m3dV *vy);
    void   Orthonormalize();
    void   MakeProjTri(m3dV *a, m3dV *b, m3dV *c);
    BOOL32 MakePolyPoly(m3dPOLY *src, m3dPOLY *dst);
    void   Transpose();
    BOOL32 Invert(m3dMATR *out);
    void   RemoveScale();
    void   __cdecl TransformPointList (int n, void *src, int srcStride, void *dst, int dstStride);
    void   __cdecl TransformVectorList(int n, void *src, int srcStride, void *dst, int dstStride);
};
BOOL32 __fastcall m3dInit3DMatr();
void   __fastcall m3dAddMatr(m3dMATR *a, m3dMATR *b, m3dMATR *out);
void   __cdecl    m3dMultiplyMatr(m3dMATR *a, m3dMATR *b, m3dMATR *out);
BOOL32 __fastcall m3dIsEqualMatr_A(m3dMATR *a, m3dMATR *b, float epsRot, float epsPos);
float  __fastcall _m2dCalcMatrDet(m3dMATR *m, short r0, short r1, short r2, short c0, short c1, short c2);
void   __fastcall m3dRotatePointAxis(m3dV *center, m3dV *axis, float deg, m3dV *p, m3dV *out); // 1.2: new, name ?

// p*M helpers (inlined everywhere in the game)
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
// m3dQUAT {w,x,y,z}
// ---------------------------------------------------------------------------------------------
class m3dQUAT {
public:
    float w, x, y, z;
    void Identity();
    void Invert();
    void SetAngAxis(float deg, m3dV *axis);
    void GetMatr(m3dMATR *out);
    void SetRotMatr(m3dMATR *m);
    void SetMatr(m3dMATR *m);
};
BOOL32 __fastcall m3dIsEqualQuat_A(m3dQUAT *a, m3dQUAT *b, float eps);
void   __fastcall m3dSlerpQuat(m3dQUAT *a, m3dQUAT *b, float t, m3dQUAT *out);

// ---------------------------------------------------------------------------------------------
// Bounding volumes
// ---------------------------------------------------------------------------------------------
enum m3dVOL_TYPE { M3D_VOL_BOX = 1, M3D_VOL_PRISM = 2, M3D_VOL_SPHERE = 3, M3D_VOL_OBB = 4 };
// QualPlane result: 0 = crosses the plane, 1 = behind (negative side), -1 = in front
enum { M3D_QUAL_CROSS = 0, M3D_QUAL_BACK = 1, M3D_QUAL_FRONT = -1 };
// IsectRay / DistPoint result: 0 = miss/too far, 1 = hit, 2 = start point inside the volume
enum { M3D_ISECT_NONE = 0, M3D_ISECT_HIT = 1, M3D_ISECT_INSIDE = 2 };

class m3dSPHERE;
class m3dVOL {
public:
    virtual void  *__vecDelDtor(unsigned flags);                                      // +0x00
    virtual BOOL32 IsBelongPoint(m3dV *p) = 0;                                         // +0x04
    virtual BOOL32 IsIsectCyl(m3dV *org, m3dV *dir, float len, float r) = 0;           // +0x08 swept sphere
    virtual BOOL32 IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face) = 0; // +0x0C
    virtual BOOL32 DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face) = 0;          // +0x10
    virtual void   Calc(m3dV *pts, int n) = 0;                                         // +0x14
    virtual BOOL32 Check() = 0;                                                        // +0x18
    virtual void   CalcEnclSph(m3dSPHERE *out) = 0;                                    // +0x1C
    virtual BOOL32 CheckEnclSph(m3dSPHERE *s) = 0;                                     // +0x20
    virtual void   GetCenter(m3dV *out) = 0;                                           // +0x24
    virtual void   GetFace(int i, m3dPOLY *out, m3dV *normal) = 0;                     // +0x28
    virtual void   Expand(float d) = 0;                                                // +0x2C
    virtual int    QualPlane(m3dV *planePt, m3dV *planeN);                             // +0x30
    int type;                                                                          // +0x04
};

class m3dBOX : public m3dVOL {                          // 0x20 (m3d_box.obj, other batch)
public:
    m3dV vMin, vMax;                                    // +0x08, +0x14
    BOOL32 IsBelongPoint(m3dV *p);
    BOOL32 IsIsectCyl(m3dV *org, m3dV *dir, float len, float r);
    BOOL32 IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face);
    BOOL32 DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face);
    void   Calc(m3dV *pts, int n);
    BOOL32 Check();
    void   CalcEnclSph(m3dSPHERE *out);
    BOOL32 CheckEnclSph(m3dSPHERE *s);
    void   GetCenter(m3dV *out);
    void   GetFace(int i, m3dPOLY *out, m3dV *normal);
    void   Expand(float d);
    int    QualPlane(m3dV *planePt, m3dV *planeN);
    void   CalcStart();
    void   CalcUpdate(m3dV *p);
    void   Calc(m3dV *p, int n, float expand);
    void   CalcXZ(m3dV *p, int n, float expand);
    void   GetVertices(m3dV out[8]);
};

class m3dSPHERE : public m3dVOL {                       // 0x18
public:
    m3dV  center;                                       // +0x08
    float radius;                                       // +0x14
    BOOL32 IsBelongPoint(m3dV *p);
    BOOL32 IsIsectCyl(m3dV *org, m3dV *dir, float len, float r);
    BOOL32 IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face);
    BOOL32 DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face);
    void   Calc(m3dV *pts, int n);
    BOOL32 Check();
    void   CalcEnclSph(m3dSPHERE *out);
    BOOL32 CheckEnclSph(m3dSPHERE *s);
    void   GetCenter(m3dV *out) { *out = center; }     // inline in the header (v12 0x00422550 copy)
    void   GetFace(int, m3dPOLY *, m3dV *) {}          // folded empty stub (v12 0x0051C0D0)
    void   Expand(float d);
    int    QualPlane(m3dV *planePt, m3dV *planeN);
};

class m3dPRISM : public m3dVOL {                        // 0xB4 vertical prism: XZ outline, y range
public:
    m2dPOLY base;                                       // +0x08 (m2dV.x = world x, m2dV.y = world z)
    float   yMin;                                       // +0xAC
    float   yMax;                                       // +0xB0
    BOOL32 IsBelongPoint(m3dV *p);
    BOOL32 IsIsectCyl(m3dV *org, m3dV *dir, float len, float r);
    BOOL32 IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face);
    BOOL32 DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face);
    void   Calc(m3dV *pts, int n);
    BOOL32 Check();
    void   CalcEnclSph(m3dSPHERE *out);
    BOOL32 CheckEnclSph(m3dSPHERE *s);
    void   GetCenter(m3dV *out);
    void   GetFace(int i, m3dPOLY *out, m3dV *normal);
    void   Expand(float d);
    void   GetVertices(m3dV *out);
    BOOL32 Expand(m3dV *a, m3dV *b);
private:
    float  _MakeBPrismRect(m3dV *pts, int n, float ang, float *xMin, float *xMax, float *zMin, float *zMax);
    void   _MakeBPrismPolyBase(float ang, float xMin, float xMax, float zMin, float zMax);
};

class m3dOBB : public m3dVOL {                          // 0x38 corner + 3 edge vectors
public:
    m3dV org;                                           // +0x08
    m3dV ax[3];                                         // +0x14, +0x20, +0x2C
    BOOL32 IsBelongPoint(m3dV *p);
    BOOL32 IsIsectCyl(m3dV *org, m3dV *dir, float len, float r);
    BOOL32 IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face);
    BOOL32 DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face);
    void   Calc(m3dV *, int) {}                         // folded stub (v12 0x005AA550 'ret 8')
    BOOL32 Check();                                     // folded stub (v12 0x00548680)
    void   CalcEnclSph(m3dSPHERE *out);
    BOOL32 CheckEnclSph(m3dSPHERE *s);
    void   GetCenter(m3dV *out);
    void   GetFace(int i, m3dPOLY *out, m3dV *normal);
    void   Expand(float d);
    int    QualPlane(m3dV *planePt, m3dV *planeN);      // 1.2: new override
    void   GetMatrW2L(m3dMATR *w2l, m3dMATR *l2w, m3dBOX *localBox);
    void   GetVertices(m3dV out[8]);
};

// ---------------------------------------------------------------------------------------------
// Random numbers (CRT rand() based + private LCG)
// ---------------------------------------------------------------------------------------------
struct m3dRND_GEN {
    unsigned int seed;
    int   RndIntMax(int max);
    float RndFloat();
    float RndFloatMax(float max);
    float RndFloatRange(float lo, float hi);
};
float  __fastcall m3dRandom();
float  __fastcall m3dRandMax(float max);
float  __fastcall m3dRandRange(float lo, float hi);
int    __fastcall m3dRandIntMax(int max);
int    __fastcall m3dRandIntRange(int lo, int hi);
void   __fastcall m3dRandBasicCoord(float *u, float *v);
void   __fastcall m3dMakeVUnitRandom(m3dV *out);
void   __fastcall m3dMakeVUnitConusRandom(m3dV *dir, float deg, m3dV *out);
void   __fastcall m3dMakeVTangRandom(m3dV *n, m3dV *out);
void   __fastcall m3dMakeVCylinderRandom(m3dV *axis, float r, float h, m3dV *out);
int    __fastcall m3dRandSelect(int a, int b);
int    __fastcall m3dRandSelect(int a, int b, int c);
int    __fastcall m3dRandSelect(int a, int b, int c, int d);

// ---------------------------------------------------------------------------------------------
// Noise
// ---------------------------------------------------------------------------------------------
class m3dNOISE {
public:
    virtual void  *__vecDelDtor(unsigned f);            // +0x00 (shared scalar deleting dtor; slot-exact, no GCC dtor pair)
    virtual BOOL32 Fill(int seed);                      // +0x04
    virtual float  GetValue(m3dV *p) = 0;               // +0x08
    int    _04;                                         // +0x04 ?
    int    size;                                        // +0x08 table size, power of two
    int    gradStride;                                  // +0x0C bytes per gradient (12)
    float *grad;                                        // +0x10
    int   *hash;                                        // +0x14
    m3dNOISE() : _04(0), size(0x200), gradStride(0xC), grad(0), hash(0) {}
};
class m3dNOISE_GRAD : public m3dNOISE {
public:
    BOOL32 Fill(int seed);
    float  GetValue(m3dV *p);
};
BOOL32 __fastcall m3dInitNoise();
void   __fastcall m3dTermNoise();
BOOL32 __fastcall _noiseFillHash(int *hash, int n);
extern m3dNOISE *m3dNoise;

// ---------------------------------------------------------------------------------------------
// Scalar solvers (m3d_math)
// ---------------------------------------------------------------------------------------------
int    __fastcall m3dSolveQuadraticEq(float a, float b, float c, float *x0, float *x1);
BOOL32 __fastcall m3dUpdateTimeField(float *timer, float dt);
BOOL32 __fastcall m3dSolveLinSys2D(float a, float b, float c, float d, float e, float f, float *x, float *y);
BOOL32 __fastcall m3dLimitChange_Float(float cur, float target, float step, float *out);

// ---------------------------------------------------------------------------------------------
// Lines, polygons, misc
// ---------------------------------------------------------------------------------------------
float  __fastcall m3xzDistPointLSeg_2(m3dV *p, m3dV *a, m3dV *b, m3dV *closest);
float  __fastcall m3dDistPointLine(m3dV *p, m3dV *a, m3dV *b);
BOOL32 __fastcall m3dDistPointLSeg(m3dV *p, m3dV *a, m3dV *b, float maxD, float *d, m3dV *closest, float *t);
BOOL32 __fastcall m3dDistLSegLSeg(m3dV *a0, m3dV *a1, m3dV *b0, m3dV *b1, float maxD, float *d, m3dV *ca, m3dV *cb);
BOOL32 __fastcall m3dIsIsectYLinePlane(m3dV *p, m3dV *a, m3dV *b, m3dV *c);
BOOL32 __fastcall m2dIsectLSegLSeg(m2dV *a0, m2dV *a1, m2dV *b0, m2dV *b1, m2dV *hit);

int    __fastcall m3dGetClosestVert(int n, m3dV *verts, m3dV *p);
int    __fastcall _m3xzIsIsectPolyCircle(m3dPOLY *poly, m3dV *c, float r);
int    __fastcall m3xzIsIsectPolyCircle(m3dPOLY *poly, m3dV *c, float r);
void   __fastcall _m3xzDist2PointPoly(m3dV *p, m3dPOLY *poly, m3dV *closest, float *d);

void   __fastcall m3dGetPolyEdge(m3dPOLY *p, int i, m3dV *a, m3dV *b);
void   __fastcall m3xzPoly3DToPoly2D(m3dPOLY *p, m2dPOLY *out);
void   __fastcall m3dGetPolyNormal(m3dPOLY *p, m3dV *n);
BOOL32 __fastcall _m3dGetPolyNormal(m3dPOLY *p, m3dV *n);
void   __fastcall m3dGetPlaneNormal(m3dV *a, m3dV *b, m3dV *c, m3dV *n);
void   __cdecl    m3dGetPlanePerpVector(m3dV *a, m3dV *b, m3dV *n, m3dV *out);
float  __fastcall m3dCalcTriArea2(m3dV *a, m3dV *b, m3dV *c);
float  __fastcall m3dCalcPolyArea(m3dPOLY *p);
void   __fastcall m2dReOrientPoly(m2dPOLY *p);
void   __fastcall m3dReOrientPoly(m3dPOLY *p);
BOOL32 __fastcall m2dIsPolyConvexCCW(m2dPOLY *p);
BOOL32 __fastcall m3dIsectRayPlane(m3dV *org, m3dV *dir, m3dPOLY *p, m3dV *hit, m3dV *n, float *t);
BOOL32 __fastcall m3dIsectRayVNPlane(m3dV *org, m3dV *dir, m3dV *pp, m3dV *pn, m3dV *hit, float *t);
BOOL32 __fastcall _m3dIsectRayVNPlane(m3dV *org, m3dV *dir, m3dV *pp, m3dV *pn, m3dV *hit, float *t);
BOOL32 __fastcall m3dDistPointPoly(m3dV *p, m3dPOLY *poly, m3dV *n, int flags, m3dV *closest, float *d, float maxD);
BOOL32 __fastcall m3dDistLSegPoly(m3dV *a, m3dV *b, m3dPOLY *p, m3dV *n, m3dV *c0, m3dV *c1, float *d, int *where, float maxD);
BOOL32 __fastcall m3dDistLSegPlane(m3dV *a, m3dV *b, m3dV *pp, m3dV *pn, m3dV *c0, m3dV *c1, float *d, float maxD);
BOOL32 __fastcall m3dIsectLSegPlane(m3dV *a, m3dV *b, m3dV *pp, m3dV *pn, m3dV *hit);
BOOL32 __fastcall m3dIsectLSegPoly(m3dV *a, m3dV *b, m3dPOLY *p, m3dV *n, m3dV *hit);
void   __fastcall m2dTriApplyBasisCoord(m2dTRI *t, float u, float v, m2dV *out);
void   __fastcall m3dPolyApplyBasisCoord(m3dPOLY *p, float u, float v, m3dV *out);
BOOL32 __fastcall m2dIsCrossLSegPolyBoundary_A(m2dPOLY *p, m2dV *a, m2dV *b, float eps);

// ---- defined in other m3d objs (other batches) ----
float  __fastcall _m3dAngleVector(m3dV *a, m3dV *b);                                   // m3d_vect
BOOL32 __fastcall m3xzIsBelongPointPoly_A(m3dPOLY *p, m3dV *pt, float eps);            // m3d_clip
BOOL32 __fastcall m3dIsBelongPointPoly_A(m3dPOLY *p, m3dV *pt, m3dV *n, float eps);    // m3d_clip (pt before n, see the calls)
BOOL32 __fastcall m2dIsBelongPointPoly_A(m2dPOLY *p, m2dV *pt, float eps);             // m3d_clip
int    __fastcall m2dQualPointLine_A(m2dV *p, m2dV *a, m2dV *b, float eps);           // m3d_clip: 1 = right of a->b

// ---- engine ----
void *apCalloc(size_t n, size_t sz);
void  apFree(void *p);
void *apRealloc(void *p, size_t n);
extern "C" int __cdecl rand(void);

// ---- globals ----
extern int     m3dSimdType;
extern m3dV    m3dVUnitX, m3dVUnitY, m3dVUnitZ;
extern m3dV    m3dVZero;
extern m2dV    m2dVUnitX, m2dVUnitY;
extern m3dMATR m3dMatrIdentity;

static_assert(sizeof(m3dPOLY) == 0xF4, "m3dPOLY");
static_assert(sizeof(m2dPOLY) == 0xA4, "m2dPOLY");
static_assert(sizeof(m3dMATR) == 0x40, "m3dMATR");
#if defined(__i386__) || defined(_M_IX86)
static_assert(sizeof(m3dBOX) == 0x20 && sizeof(m3dSPHERE) == 0x18, "vol");
static_assert(sizeof(m3dPRISM) == 0xB4 && sizeof(m3dOBB) == 0x38, "vol");
static_assert(sizeof(m3dNOISE) == 0x18, "noise");
#endif
