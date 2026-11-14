// m3d.h -- Will Rock (Saber Interactive, 2003) UNOFFICIAL modding SDK, subsystem "math_app" (part 1: m3d math)
//
// The m3d library (m3d.lib: m3d_vect/matr/quat/box/sph/obb/prsm/poly/clip/line/lerp/rand/noise/spl/ctrl/vox)
// is pure, platform-independent FPU math. Static analysis only. Target build 1.2 (v12.exe), reference Feb 2003
// build (feb.exe + WillRock.map + Ghidra decompile).
//
// Conventions (same as re/sdk/engine_core/core_types.h):
//   "// v12 +0xNN (feb +0xNN)"            field offset in 1.2 and in the Feb build
//   "// v12 0x00XXXXXX (feb 0x00XXXXXX)"  function / global address
//   "// ?"                                not verified by a v12 instruction
// Every m3d function marked [exact] in m3d_index.h is byte-identical (modulo relocations) between feb and v12,
// so the struct offsets used inside it are verified for v12 by that match. The m3d layouts did NOT change
// between feb and 1.2 (no field moved; only BOX/OBB gained a QualPlane override, see m3dVOL).
// Calling convention: free functions are __fastcall (MSVC "YI" mangling) unless noted "__cdecl" (YA);
// methods are __thiscall. The whole engine uses x87 floats; 1 unit = 1 metre ? ; Y is UP (m3xz* = ground plane).
//
// Assumed from re/sdk/core: nothing (this file defines m3dV/m3dMATR; core_types.h forward-declares them).

#pragma once
#include "../core/wr_cc.h"   // calling-convention shims (integrator)
typedef int BOOL32;

// =====================================================================================================
// Basic value types (no vtables)
// =====================================================================================================
struct m2dV    { float x, y; };                       // size 0x08
struct m3dV    { float x, y, z; };                    // size 0x0C   (m3dGetPolyNormal reads +0/+4/+8)
inline m3dV  operator+(const m3dV &a, const m3dV &b) { m3dV r = { a.x + b.x, a.y + b.y, a.z + b.z }; return r; }
inline m3dV  operator-(const m3dV &a, const m3dV &b) { m3dV r = { a.x - b.x, a.y - b.y, a.z - b.z }; return r; }
inline m3dV  operator*(const m3dV &a, float k)       { m3dV r = { a.x * k, a.y * k, a.z * k }; return r; }
inline float m3dDot(const m3dV &a, const m3dV &b)    { return a.x * b.x + a.y * b.y + a.z * b.z; }
// m3dSqrt is inlined in the game (x87 / SSE / 3DNow! paths chosen by m3dSimdType); these are portable stand-ins.
#if defined(_MSC_VER)
extern "C" float __cdecl sqrtf(float);
inline float m3dSqrt(float v)                         { return sqrtf(v); }
#else
inline float m3dSqrt(float v)                         { return __builtin_sqrtf(v); }
#endif
inline float m3dLength(const m3dV &a)                 { return m3dSqrt(m3dDot(a, a)); }
inline float dist2(const m3dV &a, const m3dV &b)      { m3dV d = a - b; return m3dDot(d, d); }
struct m3dVINT { int   x, y, z; };                    // size 0x0C
struct m3dVTX  { float x, y, z; };                    // ? used by _m3dCheckValid(const m3dVTX*) (finite check)
struct m3dCOLOR{ float r, g, b, a; };                 // size 0x10 (stack objects are 16 bytes; channel order ?)

// m3dMATR -- 4x4 float, ROW-major, ROW vectors (p' = p * M, Direct3D style).
// Rows: +0x00 axis X (right), +0x10 axis Y (up), +0x20 axis Z (forward), +0x30 origin. Column 3 is (0,0,0,1).
// Verified: GetOrigin reads +0x30/+0x34/+0x38 (v12 0x00561480 [callgraph+bytes 1.0]); Translate(…,1) does
//           origin += x*row0 + y*row1 + z*row2 (v12 0x005610A0 [exact]).
struct m3dMATR {
    float m[4][4];                                    // +0x00
    // "op" argument of Translate/Rotate*/Scale/Transform:
    //   0 = SET (replace the matrix / origin),  1 = LOCAL (apply in the matrix's own frame: M = T*M),
    //   2 = GLOBAL (apply in parent frame: M = M*T).   (Transform: 1 -> this = arg*this, 2 -> this = this*arg)
    void Identity();                                  // v12 0x00561050 (feb 0x00549BA0)
    void Translate(m3dV *v, int op);                  // v12 0x00561080 (feb 0x00549BD0)
    void Translate(float x, float y, float z, int op);// v12 0x005610A0 (feb 0x00549BF0)
    void RotateX(float deg, int op);                  // v12 0x00561170 (feb 0x00549CC0) angles are DEGREES
    void RotateY(float deg, int op);                  // v12 0x00561210 (feb 0x00549D60)
    void RotateZ(float deg, int op);                  // v12 0x005612B0 (feb 0x00549E00) (located by bytes)
    void Scale(float x, float y, float z, int op);    // v12 0x00561350 (feb 0x00549EA0)
    void MakeScale(float x, float y, float z);        // v12 0x00561F80 (name ours; 1.2 only, animkinem) diag(x, y, z, 1)
    void Transform(m3dMATR *m, int op);               // v12 0x00561420 (feb 0x00549F70)
    void GetOrigin(m3dV *out);                        // v12 0x00561480 (feb 0x00549FD0)
    void GetAxisX(m3dV *out);                         // v12 0x005614A0 (feb 0x00549FF0)
    void GetAxisY(m3dV *out);                         // v12 0x005614C0 (feb 0x0054A010)
    void GetAxisZ(m3dV *out);                         // v12 0x005614E0 (feb 0x0054A030)
    void GetTranslate(float *x, float *y, float *z);  // v12 0x00561500 (feb 0x0054A050)
    void GetScale(float *x, float *y, float *z);      // v12 0x00561520 (feb 0x0054A070)
    void MakeRotY(float deg);                         // v12 0x00562000 (feb 0x0054AAE0)
    void MakeRot(m3dV *from, m3dV *to);               // v12 0x00562050 (feb 0x0054AB30) rotation taking dir a to b
    void MakeRot(m3dV *a, m3dV *b, m3dV *c);          // v12 0x00562230 (feb 0x0054AD10)
    void MakeRotRay(m3dV *org, m3dV *dir, float deg); // v12 0x00562450 (feb 0x0054AF30) rotation about an arbitrary ray
    void MakeLCS2WCS(m3dV *org, m3dV *x, m3dV *y, m3dV *z); // v12 0x005624C0 (feb 0x0054AFA0) local->world from basis
    void MakeWCS2LCS(m3dV *org, m3dV *x, m3dV *y, m3dV *z); // v12 0x00562730 (feb 0x0054B210) world->local
    void MakeLCS2WCS_VZ(m3dV *org, m3dV *vz);         // v12 0x00562870 (feb 0x0054B350) basis from a forward vector (Y kept up)
    void _MakeLCS2WCS_VX(m3dV *org, m3dV *vx);       // v12 0x005628F0 (feb 0x0054B3D0) basis from a right vector
    void _MakeLCS2WCS_VY(m3dV *org, m3dV *vy);       // v12 0x00562B50 (feb 0x0054B630) basis from an up vector
    void _MakeLCS2WCS_VZ(m3dV *org, m3dV *vz);       // v12 0x00562A20 (feb 0x0054B500) variant used for MP spawn matrices // ?
    void Orthonormalize();                            // v12 0x00562C60 (feb 0x0054B740)
    void Transpose();                                 // v12 0x005634B0 (feb 0x0054BF90)
    BOOL32 Invert(m3dMATR *out);                      // v12 0x00563500 (feb 0x0054BFE0) general 4x4 inverse, 0 if singular
    void RemoveScale();                               // v12 0x00563700 (feb 0x0054C1E0) (located by bytes)
    void __cdecl TransformPointList (int n, void *src, int srcStride, void *dst, int dstStride); // v12 0x00564190 (feb 0x0054CC70) SSE/3DNow!/x87 paths
    void __cdecl TransformVectorList(int n, void *src, int srcStride, void *dst, int dstStride); // v12 0x00564600 (feb 0x0054D0E0)
};
// size 0x40
BOOL32 __fastcall m3dUpdateTimeField(float *timer, float dt); // feb 0x00549550 (m3d_math.obj): count down, 1 when it reaches 0 // ? v12 twin
void __cdecl m3dMultiplyMatr(m3dMATR *a, m3dMATR *b, m3dMATR *out); // v12 0x00560A50 (feb 0x00549770) out = a*b
void __fastcall   m3dAddMatr(m3dMATR *a, m3dMATR *b, m3dMATR *out);             // v12 0x00560A20 (feb 0x00549740)
BOOL32 __fastcall m3dIsEqualMatr_A(m3dMATR *a, m3dMATR *b, float epsRot, float epsPos); // v12 0x00560DF0 (feb 0x00549B10)

struct m2dMATR { float m[3][3]; };                    // size 0x24 ? (Identity v12 0x0057C470 (feb 0x00563B50) clears 9 dwords)

// m3dQUAT -- {w, x, y, z}: SetAngAxis stores cos(a/2) at +0 and sin(a/2)*axis at +4..+0xC (angle in DEGREES).
struct m3dQUAT {
    float w, x, y, z;                                 // +0x00 +0x04 +0x08 +0x0C
    void Identity();                                  // (feb 0x00567160) inlined in v12, no out-of-line copy: write w=1,x=y=z=0
    void Invert();                                    // v12 0x0057FA80 (feb 0x00567180)
    void SetAngAxis(float deg, m3dV *axis);           // v12 0x0057FAB0 (feb 0x005671B0)
    void GetMatr(m3dMATR *out);                       // v12 0x0057FAF0 (feb 0x005671F0) rotation part, origin = 0
    void SetRotMatr(m3dMATR *m);                      // v12 0x0057FC20 (feb 0x00567320) from a pure rotation
    void SetMatr(m3dMATR *m);                         // v12 0x0057FEA0 (feb 0x005675B0) handles scaled matrices
};
// size 0x10
BOOL32 __fastcall m3dIsEqualQuat_A(m3dQUAT *a, m3dQUAT *b, float eps);      // v12 0x0057F720 (feb 0x00566E00)
void __fastcall   m3dSlerpQuat(m3dQUAT *a, m3dQUAT *b, float t, m3dQUAT *out); // v12 0x0057F830 (feb 0x00566F10)

// =====================================================================================================
// Polygons (fixed capacity, no heap)
// =====================================================================================================
#define M3D_POLY_MAX_VERT 20                          // from sizes: m3dPOLY 0xF4 = 4+20*12, m2dPOLY 0xA4 = 4+20*8
struct m3dPOLY { int nVert; m3dV vert[M3D_POLY_MAX_VERT]; };   // +0x00 nVert, +0x04 vert[] ; size 0xF4
struct m2dPOLY { int nVert; m2dV vert[M3D_POLY_MAX_VERT]; };   // +0x00 nVert, +0x04 vert[] ; size 0xA4
struct m2dTRI  { m2dV v[3]; };                                 // ? size 0x18

// =====================================================================================================
// Bounding volumes: m3dVOL (abstract) -> m3dBOX / m3dPRISM / m3dSPHERE / m3dOBB
// =====================================================================================================
enum m3dVOL_TYPE {                                    // m3dVOL::type, value passed to m3dVOL::Make
    M3D_VOL_BOX = 1, M3D_VOL_PRISM = 2, M3D_VOL_SPHERE = 3, M3D_VOL_OBB = 4
};
// Return of QualPlane: which side of a plane the volume is on (front / back / crossing) -- exact codes ?
class m3dVOL {
public:
    // vtable (13 slots, same order for every volume type; feb and v12 identical slot order)
    virtual void  *__vecDelDtor(unsigned flags);                                   // slot +0x00 v12 0x004F2F50 (feb 0x0041F110) shared
    virtual BOOL32 IsBelongPoint(m3dV *p) = 0;                                      // slot +0x04 point inside?
    virtual BOOL32 IsIsectCyl(m3dV *base, m3dV *top, float r, float h) = 0;         // slot +0x08 vertical cylinder test (args ?)
    virtual BOOL32 IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face) = 0; // slot +0x0C ray cast
    virtual BOOL32 DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face) = 0;          // slot +0x10
    virtual void   Calc(m3dV *pts, int n) = 0;                                      // slot +0x14 fit to a point list (BOX: v12 0x00408190 inline copy)
    virtual BOOL32 Check() = 0;                                                     // slot +0x18 validity (min<=max ...)
    virtual void   CalcEnclSph(class m3dSPHERE *out) = 0;                           // slot +0x1C enclosing sphere
    virtual BOOL32 CheckEnclSph(class m3dSPHERE *s) = 0;                            // slot +0x20
    virtual void   GetCenter(m3dV *out) = 0;                                        // slot +0x24
    virtual void   GetFace(int i, m3dPOLY *out, m3dV *normal) = 0;                  // slot +0x28
    virtual void   Expand(float d) = 0;                                             // slot +0x2C grow by d on every side
    virtual int    QualPlane(m3dV *planePt, m3dV *planeN);                          // slot +0x30 m3dVOL default v12 0x00569950 (feb 0x00554300):
                                                                                    //   CalcEnclSph, radius *= 0.7 (const v12 0x005F9808), sphere test
    int type;                                         // v12 +0x04 (feb +0x04) m3dVOL_TYPE (written by m3dVOL::Make v12 0x005698B0)
    static m3dVOL *Make(int type);                    // v12 0x005698B0 (feb 0x00554260) operator new + vtable + type
};
// vtable m3dVOL v12 0x005F9478 (feb 0x005CEE78)

class m3dBOX : public m3dVOL {                        // axis-aligned box; size 0x20
public:
    m3dV vMin;                                        // v12 +0x08 (feb +0x08)  verified: GetCenter v12 0x0056DB60 reads +0x08/+0x14
    m3dV vMax;                                        // v12 +0x14 (feb +0x14)
    // overrides (vtable m3dBOX v12 0x005F9B68, addresses below) -- declared so m3dBOX can be embedded by value
    BOOL32 IsBelongPoint(m3dV *p);                                                 // v12 0x0056C530
    BOOL32 IsIsectCyl(m3dV *base, m3dV *top, float r, float h);                    // v12 0x0056C5C0
    BOOL32 IsectRay(m3dV *org, m3dV *dir, float maxT, m3dV *hit, float *t, int *face); // v12 0x0056C660
    BOOL32 DistPoint(m3dV *p, float maxD, m3dV *closest, float *d, int *face);    // v12 0x0056C8B0
    void   Calc(m3dV *pts, int n);                                                 // v12 0x00408190
    BOOL32 Check();                                                                // v12 0x0056D870
    void   CalcEnclSph(class m3dSPHERE *out);                                      // v12 0x0056D8C0
    BOOL32 CheckEnclSph(class m3dSPHERE *s);                                       // v12 0x0056D9B0
    void   GetCenter(m3dV *out);                                                   // v12 0x0056DB60
    void   GetFace(int i, m3dPOLY *out, m3dV *normal);                             // v12 0x0056D620
    void   Expand(float d);                                                        // v12 0x0056E070
    int    QualPlane(m3dV *planePt, m3dV *planeN);                                 // v12 0x0056E660 NEW in 1.2
    void  CalcStart();                                // v12 0x0056CF60 (feb 0x00554F00) min=+BIG, max=-BIG
    void  CalcUpdate(m3dV *p);                        // v12 0x0056CFA0 (feb 0x00554F40) grow to include p
    void  CalcUpdate(m3dV *p, int n);                 // v12 0x0056D040 (feb 0x00554FE0)
    void  CalcUpdate(m3dBOX *b);                      // v12 0x0056D060 (feb 0x00555000)
    void  Calc(m3dV *p, int n, float expand);         // v12 0x0056D080 (feb 0x00555020)
    void  CalcXZ(m3dV *p, int n, float expand);       // v12 0x0056D1C0 (feb 0x00555160)
    void  MakePointPoint(m3dV *a, m3dV *b);           // v12 0x0056D290 (feb 0x00555230)
    void  GetVertices(m3dV out[8]);                   // v12 0x0056D350 (feb 0x005552F0)
    float GetArea();                                  // v12 0x0056DBA0 (feb 0x00555B40)
    BOOL32 IsIsectBox(m3dBOX *b);                     // v12 0x0056DBD0 (feb 0x00555B70)
    BOOL32 IsIsectPoly(m3dPOLY *p, m3dV *n);          // v12 0x0056DC20 (feb 0x00555BC0)
    BOOL32 ClipLSeg(m3dV *a, m3dV *b, m3dV *oa, m3dV *ob); // v12 0x0056E0B0 (feb 0x00556050)
};
// vtable m3dBOX v12 0x005F9B68 (feb 0x005CEE44). slots +0x04..+0x2C: 0x0056C530 IsBelongPoint, 0x0056C5C0 IsIsectCyl,
//   0x0056C660 IsectRay, 0x0056C8B0 DistPoint, 0x00408190 Calc, 0x0056D870 Check, 0x0056D8C0 CalcEnclSph,
//   0x0056D9B0 CheckEnclSph, 0x0056DB60 GetCenter, 0x0056D620 GetFace, 0x0056E070 Expand,
//   slot +0x30 QualPlane = v12 0x0056E660  ** NEW in 1.2 ** (feb used m3dVOL::QualPlane): tests the 8 corners.

class m3dPRISM : public m3dVOL {                      // vertical prism: 2D polygon in XZ extruded along Y; size 0xB4
public:
    m2dPOLY base;                                     // v12 +0x08 (feb +0x08) XZ outline (x -> m2dV.x, z -> m2dV.y)
    float   yMin;                                     // v12 +0xAC (feb +0xAC) verified: IsBelongPoint v12 0x00575D00
    float   yMax;                                     // v12 +0xB0 (feb +0xB0)
    void   GetVertices(m3dV *out);                    // v12 0x005770C0 (feb 0x0055EB40)
    BOOL32 Expand(m3dV *a, m3dV *b);                  // v12 0x00577110 (feb 0x0055EB90)
};
// vtable m3dPRISM v12 0x00607110 (feb 0x005DBC24); IsBelongPoint tolerance 0.001 (const v12 0x005FB874), 2D eps 0.2

class m3dSPHERE : public m3dVOL {                     // size 0x18
public:
    m3dV  center;                                     // v12 +0x08 (feb +0x08)
    float radius;                                     // v12 +0x14 (feb +0x14) verified: m3dVOL::QualPlane v12 0x0056996D
    m3dSPHERE &operator=(const m3dSPHERE &s);       // v12 0x004DC530 (feb 0x004BF7C0, Object.obj) type, centre, radius
};
// vtable m3dSPHERE v12 0x005FB68C (feb 0x005D0930); QualPlane own (v12 0x0056FF80)

class m3dOBB : public m3dVOL {                        // oriented box = origin corner + 3 full-length edge vectors; size 0x38
public:
    m3dV org;                                         // v12 +0x08 (feb +0x08) corner
    m3dV ax[3];                                       // v12 +0x14,+0x20,+0x2C (feb same) edges; center = org + 0.5*(ax0+ax1+ax2)
    void GetMatrW2L(m3dMATR *w2l, m3dMATR *l2w, m3dBOX *localBox); // v12 0x00579140 (feb 0x005608E0)
    void GetVertices(m3dV out[8]);                    // v12 0x00579BB0 (feb 0x00561270)
};
// vtable m3dOBB v12 0x00607784 (feb 0x005DBBD8); slot +0x30 QualPlane = v12 0x00578E50 ** NEW in 1.2 **

// =====================================================================================================
// Random numbers (two generators!)
// =====================================================================================================
// m3dRandom*/m3dRandInt*/m3dMakeV*Random use the CRT rand() (v12 _rand 0x00580755, global seed: CRT srand).
// m3dRND_GEN is a private LCG: seed = seed*0x343FD + 0x269EC3 (same constants as MS rand) -> deterministic streams.
struct m3dRND_GEN {
    unsigned int seed;                                // +0x00
    int   RndIntMax(int max);                         // v12 0x0056E800 (feb 0x005566E0)
    float RndFloat();                                 // v12 0x0056E840 (feb 0x00556720) [0,1)
    float RndFloatMax(float max);                     // v12 0x0056E880 (feb 0x00556760)
    float RndFloatRange(float lo, float hi);          // v12 0x0056E8C0 (feb 0x005567A0)
};
float __fastcall m3dRandom();                                    // v12 0x0056E720 (feb 0x00556600) rand()*(1/32767)
float __fastcall m3dRandMax(float max);                          // v12 0x0056E740 (feb 0x00556620)
float __fastcall m3dRandRange(float lo, float hi);               // v12 0x0056E760 (feb 0x00556640)
int __fastcall   m3dRandIntMax(int max);                         // v12 0x0056E790 (feb 0x00556670)
int __fastcall   m3dRandIntRange(int lo, int hi);                // v12 0x0056E7C0 (feb 0x005566A0)
void __fastcall  m3dMakeVUnitRandom(m3dV *out);                  // v12 0x0056E990 (feb 0x00556870)
void __fastcall  m3dMakeVUnitConusRandom(m3dV *out, float deg, m3dV *axis); // v12 0x0056EA00 (feb 0x005568E0) random dir in cone (weapon spread)
void __fastcall  m3dMakeVTangRandom(m3dV *out, m3dV *n);         // v12 0x0056ED00 (feb 0x00556BE0)
void __fastcall  m3dMakeVCylinderRandom(m3dV *out, float r, float h, m3dV *c); // v12 0x0056EE00 (feb 0x00556CE0)
int __fastcall   m3dRandSelect(int a, int b);                    // v12 0x0056F020 (feb 0x00556F20) (+3/4-arg overloads 0x0056F020/0x0056F070)
int __fastcall   m3dRandSelect(int a, int b, int c);             // v12 0x0056F040 one of three
int __fastcall   m3dRandSelect(int a, int b, int c, int d);      // v12 0x0056F070 one of four

// =====================================================================================================
// Vectors, planes, rays, segments (free functions; m3dV arithmetic itself is inline everywhere)
// =====================================================================================================
// m3dNormalize(m3dV*) is an INLINE function: every .obj has its own static copy (e.g. v12 0x00402D80, 0x00414080)
// that branches on m3dSimdType (SSE rsqrt / 3DNow! / x87). There is no single address to hook.
float __fastcall  _m3dAngleVector(m3dV *a, m3dV *b);             // v12 0x00564DA0 (feb 0x0054D600)
float __fastcall  m3dAngleVector(m3dV *a, m3dV *b);              // v12 0x00565010 (feb 0x0054D870) degrees
float __fastcall  m3dAngleVector_COS(m3dV *a, m3dV *b);          // v12 0x00564E10 (feb 0x0054D670)
float __fastcall  m3xzAngleVector(m3dV *a, m3dV *b);             // v12 0x00565220 (feb 0x0054DA90) signed angle in XZ
void __fastcall   m3dBasisCoord(m3dV *v, m3dV *a, m3dV *b, float *u, float *w); // v12 0x005654C0 (feb 0x0054DD20)
void __fastcall   m3dMirrorPointPlane(m3dV *p, m3dV *pp, m3dV *pn, m3dV *out);  // v12 0x005659B0 (feb 0x0054E180)
void __fastcall   m3dReflectVector(m3dV *v, m3dV *n, m3dV *out); // v12 0x00565A40 (feb 0x0054E210)
void __fastcall   m3dWCS2SphereCoord(m3dV *v, float *yaw, float *pitch); // v12 0x00565A80 (feb 0x0054E250)
void __fastcall   m3dGetPolyNormal(m3dPOLY *p, m3dV *n);         // v12 0x005700A0 (feb 0x00557F90) from verts 0,1,2, normalised
void __fastcall   m3dGetPlaneNormal(m3dV *a, m3dV *b, m3dV *c, m3dV *n); // v12 0x00570180 (feb 0x00558070)
void __cdecl      m3dGetPlanePerpVector(m3dV *a, m3dV *b, m3dV *c, m3dV *out); // v12 0x00570200 (feb 0x005580F0) unnormalised
void __fastcall   m3dNormalizeList(int n, m3dV *v);              // v12 0x00564B20 NEW in 1.2 (name ours): m3dNormalize each
float __fastcall  m3dCalcPolyArea(m3dPOLY *p);                   // v12 0x00570420 (feb 0x00558310)
BOOL32 __fastcall m3dIsectRayPlane(m3dV *org, m3dV *dir, m3dPOLY *p, m3dV *n, m3dV *hit, float *t); // v12 0x00570750 (feb 0x00558640)
BOOL32 __fastcall m3dIsectRayVNPlane(m3dV *org, m3dV *dir, m3dV *pp, m3dV *pn, m3dV *hit, float *t); // v12 0x005708C0 (feb 0x005587B0)
BOOL32 __fastcall m3dDistPointPoly(m3dV *p, m3dPOLY *poly, m3dV *n, int flags, m3dV *closest, float *d, float eps); // v12 0x00570A90 (feb 0x00558980)
BOOL32 __fastcall m3dDistLSegPoly(m3dV *a, m3dV *b, m3dPOLY *p, m3dV *n, m3dV *c0, m3dV *c1, float *d, int *f, float eps); // v12 0x00571520 (feb 0x00559420)
BOOL32 __fastcall m3dIsectLSegPlane(m3dV *a, m3dV *b, m3dV *pp, m3dV *pn, m3dV *hit); // v12 0x00571A20 (feb 0x00559920)
BOOL32 __fastcall m3dIsectLSegPoly(m3dV *a, m3dV *b, m3dPOLY *p, m3dV *n, m3dV *hit); // v12 0x00571D30 (feb 0x00559C30)
float __fastcall  m3dDistPointLine(m3dV *p, m3dV *a, m3dV *b);   // v12 0x00574EC0 ? (feb 0x0055C900) located by bytes
BOOL32 __fastcall m3dDistPointLSeg(m3dV *p, m3dV *a, m3dV *b, float maxD, float *d, m3dV *c, float *t); // v12 0x005750C0 (feb 0x0055CB00)
BOOL32 __fastcall m3dDistLSegLSeg(m3dV *a0, m3dV *a1, m3dV *b0, m3dV *b1, float maxD, float *d, m3dV *ca, m3dV *cb); // v12 0x005754A0 (feb 0x0055CEF0)
BOOL32 __fastcall m2dIsectLSegLSeg(m2dV *a0, m2dV *a1, m2dV *b0, m2dV *b1, m2dV *hit); // v12 0x00574B80 (feb 0x0055C5C0)
BOOL32 __fastcall m3dClipPolyPlane(m3dPOLY *p, m3dV *pp, m3dV *pn, m3dPOLY *out); // v12 0x0057AD60 (feb 0x00562440)
BOOL32 __fastcall m3dClipPolyNearPlane(m3dPOLY *p, float z, m3dPOLY *out);        // v12 0x0057A900 (feb 0x00561FE0)
BOOL32 __fastcall m2dClipPolyRect(m2dPOLY *p, m2dPOLY *rect, m2dPOLY *out);       // v12 0x0057B020 (feb 0x00562700)
BOOL32 __fastcall m3dIsBelongPointPoly_A(m3dPOLY *p, m3dV *n, m3dV *pt, float eps); // v12 0x0057A810 (feb 0x00561EF0)
BOOL32 __fastcall m3dIsBelongPointCone(m3dV *p, m3dV *apex, m3dV *dir, float ang, float r0, float r1); // v12 0x005699A0 ? (feb 0x00554350) by bytes
void __fastcall   m3dLerpVector_L(m3dV *a, m3dV *b, float t0, float t1, float t, m3dV *out); // v12 0x005731A0 (feb 0x0055ABD0)
int __fastcall    m3dSolveQuadraticEq(float a, float b, float c, float *x0, float *x1); // v12 0x00565B30 (feb 0x00549350)
BOOL32 __fastcall m3dLimitChange_Float(float cur, float target, float maxStep, float *out); // v12 0x00565E10 (feb 0x00549640)

// =====================================================================================================
// Splines (m3dSPL) -- keyframe curves used by animation, camera paths, particle/colour controllers
// =====================================================================================================
enum m3dSPL_TYPE {                                    // m3dSPL::Make(type) (v12 0x00565E90, feb 0x0054E300); kp size / dim
    M3D_SPL_LINEAR1D = 0,  // kp 0x08 {t,v}           dim 1
    M3D_SPL_LINEAR2D = 1,  // kp 0x0C {t,x,y}         dim 2
    M3D_SPL_LINEAR3D = 2,  // kp 0x10 {t,x,y,z}       dim 3
    M3D_SPL_HERMIT   = 3,  // kp 0x10                 dim 1
    M3D_SPL_BEZIER2D = 4,  // kp 0x1C                 dim 2
    M3D_SPL_BEZIER3D = 5,  // kp 0x28                 dim 3
    M3D_SPL_LAGRANGE = 6,  // kp 0x08                 dim 1
    M3D_SPL_QUAT     = 7,  // kp 0x14 {t,w,x,y,z}     dim 4
    M3D_SPL_COLOR    = 8   // kp 0x14 {t,r,g,b,a}     dim 4
};
enum m3dSPL_FLAGS { M3D_SPL_INTEGER_ARG = 1 /* t = key index */, M3D_SPL_CONST = 2 /* FindSeg returns -2 */ };
class m3dSPL {                                        // size 0x34 (all types)
public:
    // vtable: NOTE slot 0 is CompareKP, the deleting dtor is slot 1 (11 slots)
    virtual int   CompareKP(void *a, void *b);        // slot +0x00 base v12 0x00566C80 (feb 0x0054F0F0)
    virtual void *__vecDelDtor(unsigned flags);       // slot +0x04
    virtual void  GetKpValue(int i, void *out) = 0;   // slot +0x08
    virtual void  SetKpValue(int i, void *v) = 0;     // slot +0x0C
    virtual void  GetValue(float t, void *out, void *deriv) = 0; // slot +0x10 evaluate
    virtual void  GetValueClosest(float t, void *out, void *deriv); // slot +0x14
    virtual float DistPoint(void *p, int *seg, float *t, void *c);  // slot +0x18
    virtual m3dSPL *Duplicate();                      // slot +0x1C v12 0x00566140 (feb 0x0054E5B0)
    virtual m3dSPL *Sample(float t0, float t1, float step, int type); // slot +0x20 v12 0x00566CB0 (feb 0x0054F120)
    virtual void  GetMinMaxValue(float t0, float t1, float step, void *mn, void *mx); // slot +0x24 v12 0x00566DB0
    virtual m3dSPL *Convert(int type, int n);         // slot +0x28 (BEZIER3D v12 0x00568400)
    unsigned flags;                                   // v12 +0x04 (feb +0x04) m3dSPL_FLAGS
    int   type;                                       // v12 +0x08 (feb +0x08) m3dSPL_TYPE
    int   nKp;                                        // v12 +0x0C (feb +0x0C)
    int   nKpMax;                                     // v12 +0x10 (feb +0x10) ? clamp for index lookups (= nKp after AllocKpList)
    int   kpValueSize;                                // v12 +0x14 (feb +0x14) bytes copied by GetKp
    int   kpStride;                                   // v12 +0x18 (feb +0x18) sizeof(key point)
    int   dim;                                        // v12 +0x1C (feb +0x1C) number of value floats
    int   kpBytes;                                    // v12 +0x20 (feb +0x20) allocated bytes
    void *kp;                                         // v12 +0x24 (feb +0x24) key points (apRealloc), first float = t
    int   pad[3];                                     // v12 +0x28..+0x30 ? zeroed by Make
    static m3dSPL *Make(int type);                    // v12 0x00565E90 (feb 0x0054E300)
    static m3dSPL *Make(int type, int nKp);           // v12 0x00566090 (feb 0x0054E500)
    BOOL32 AllocKpList(int n);                        // v12 0x005660F0 (feb 0x0054E560)
    BOOL32 RemoveKp(int i, int n);                    // v12 0x00566230 (feb 0x0054E6A0)
    void   GetKp(int i, void *out);                   // v12 0x005662E0 (feb 0x0054E750)
    int    FindSeg(float t);                          // v12 0x005664E0 (feb 0x0054E950) binary search; -2 before first, -1 after last
    BOOL32 Optimize(int flags);                       // v12 0x00566600 (feb 0x0054EA70)
};
// vtables (feb): m3dSPL 0x005CE720, LINEAR1D 0x005CE508, LINEAR2D 0x005DE540, LINEAR3D 0x005DE514, HERMIT 0x005DE4E8,
// BEZIER2D 0x005D0B2C, BEZIER3D 0x005DE4BC, LAGRANGE 0x005DE490, QUAT 0x005DE464, COLOR 0x005CFB54. GetValue in v12:
// LINEAR1D 0x00566ED0 (recompiled), LINEAR2D 0x00567510, LINEAR3D 0x005677B0, HERMIT 0x00567D60, LAGRANGE 0x00567ED0,
// BEZIER2D 0x00568250, BEZIER3D 0x00568370, QUAT 0x005687E0, COLOR 0x005689C0.

// m3dCTRL_* -- time controllers driving a spline/noise (particles, light flicker, texture anims)
class m3dCTRL_TIME {
public:
    virtual void *__vecDelDtor(unsigned);             // slot +0x00
    virtual void  Update(float dt);                   // slot +0x04 v12 0x005691C0 (feb 0x00551130)
    virtual void  OnEnd();                            // slot +0x08 ? called when t passes length and flag 2 set
    unsigned flags;                                   // v12 +0x04 (feb +0x04) 1 = running, 2 = notify at end, 4 = clamp (no loop)
    float    t;                                       // v12 +0x08 (feb +0x08)
    float    length;                                  // v12 +0x0C (feb +0x0C) (negative = reversed for CTRL_1D_SPL)
    float    speed;                                   // v12 +0x10 (feb +0x10)
};
// m3dCTRL_1D_SPL: +0x1C m3dSPL *spl (GetValue v12 0x00569220 (feb 0x00551190) evaluates spl->GetValue(t/len)).

// m3dNOISE / m3dNOISE_GRAD -- Perlin gradient noise, global instance m3dNoise
class m3dNOISE {
public:
    virtual void *__vecDelDtor(unsigned);
    virtual BOOL32 Fill(int n);                       // v12 0x00574690 (feb 0x0055C0D0) (GRAD: v12 0x00574770)
    int    _04;                                       // ?
    int    size;                                      // v12 +0x08 (feb +0x08) table size (power of 2, mask = size-1)
    int    gradStride;                                // v12 +0x0C (feb +0x0C)
    float *grad;                                      // v12 +0x10 (feb +0x10)
    int   *hash;                                      // v12 +0x14 (feb +0x14)
};
float m3dNOISE_GRAD_GetValue(m3dNOISE *n, m3dV *p);   // m3dNOISE_GRAD::GetValue v12 0x005748C0 (feb 0x0055C300)

// voxGRID -- uniform grid used by collision / visibility
struct voxGRID {
    int  nx, ny, nz;                                  // +0x00 +0x04 +0x08
    int  _0C, _10;                                    // ?
    m3dV bbMin;                                       // +0x14
    m3dV bbMax;                                       // +0x20
    m3dV voxSize;                                     // +0x2C  (bbMax-bbMin)/n, set by CalcVoxSize v12 0x0057ED20 (feb 0x00566400)
};

// =====================================================================================================
// Library init and globals
// =====================================================================================================
BOOL32 __fastcall m3dInit();                                     // v12 0x005695C0 (feb 0x00551530) DetectSIMD, Init2DMatr, Init3DMatr, InitNoise
void __fastcall   m3dTerm();                                     // v12 0x005695F0 (feb 0x00551560) m3dTermNoise + empty Term2D/3DMatr
void __fastcall   _m3dDetectSIMD();                              // v12 0x00569760 (feb 0x005516A0) CPUID -> m3dSimdType, logs "SSE detected." ...
int __fastcall    m3dGetSimdType();                              // v12 0x00569880 (feb 0x005517C0) returns m3dSimdType & 0x7FFFFFFF
BOOL32 __fastcall _m3dCheckValid(float f);                       // v12 0x00569640 (feb 0x00551580) NaN/inf check (debug)

enum m3dSIMD { M3D_SIMD_SSE = 0, M3D_SIMD_3DNOW = 1, M3D_SIMD_NONE = 2 };
extern int     m3dSimdType;                           // v12 0x00637C24 (feb 0x005F99D0) write 2 to force the x87 paths
extern m3dV    m3dVUnitX;                             // v12 0x00637BD0 (feb 0x005F9984)
extern m3dV    m3dVUnitY;                             // v12 0x00637BDC (feb 0x005F9990)
extern m3dV    m3dVUnitZ;                             // v12 0x00637BE8 (feb 0x005F999C)
extern m3dV    m3dVOneXYZ;                            // v12 0x00637BF4 (feb 0x005F99A8)
extern m3dV   *m3dBoxNorm;                            // v12 0x00637C68 (feb 0x005F9A18) 6 face normals of a box ?
extern m3dMATR m3dMatrIdentity;                       // v12 0x009AEA40 (feb 0x009636F8)
extern m3dV    m3dVZero;                              // v12 0x009AEA84 (feb 0x0096373C)
extern m2dV    m2dVZero;                              // v12 0x009AEA9C (feb 0x00963754) (verified: m3dSPL_BEZIER2D::SetKpValue v12 0x0042DEF0)
extern m2dV    m2dVOneXY;                             // v12 0x00637C10 (feb 0x005F99C4) {1, 1} (uiUI_SYSTEM ctor v12 0x005199BB)
extern m3dV    m3dVUndef;                             // v12 0x009AEA90 (feb 0x00963748) "no position" marker (verified: WinMain v12 0x004AA218)
extern m3dVINT m3dVIntZero;                           // v12 0x009AEAAC (feb 0x00963764)
extern m3dNOISE *m3dNoise;                            // v12 0x009AEB00 (feb 0x009637B0)
