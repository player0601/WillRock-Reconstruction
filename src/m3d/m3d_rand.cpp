// m3d_rand.cpp -- Will Rock 1.2 (v12.exe): random numbers of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_rand.obj) and checked against v12.
// v12 range 0x0056E720 .. 0x0056F0C3 (feb 0x00556600 .. 0x00556FA3). No logic change in 1.2.
//
// Two generators:
//   m3dRandom* / m3dMakeV*Random / m3dRandSelect use the CRT rand() (shared seed, srand()).
//   m3dRND_GEN is a private LCG (MS rand constants) for reproducible streams.
// Mod hook point: everything that "sprays" (weapon spread = m3dMakeVUnitConusRandom, particle
// emitters = m3dMakeVCylinderRandom / m3dMakeVUnitRandom) goes through this file.

#include "m3d_geom.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0056F0B0 (feb 0x00556F90) -> v12 0x009AEAD8

#define M3D_LCG(s)  ((s) = (s) * 0x343FD + 0x269EC3)
#define M3D_2POW_M32 2.3283064365386963e-10   // __real@3df0000000100000 (1/2^32)


// v12 0x0056E720  (feb 0x00556600)
// Uniform float in [0,1].
float __fastcall m3dRandom()
{
    return (float)rand() * M3D_RAND_K;
}


// v12 0x0056E740  (feb 0x00556620)
// Uniform float in [0,max].
float __fastcall m3dRandMax(float max)
{
    return (float)rand() * max * M3D_RAND_K;
}


// v12 0x0056E760  (feb 0x00556640)
// Uniform float in [lo,hi].
float __fastcall m3dRandRange(float lo, float hi)
{
    return (hi - lo) * (float)rand() * M3D_RAND_K + lo;
}


// v12 0x0056E790  (feb 0x00556670)
// Uniform int in [0,max].
int __fastcall m3dRandIntMax(int max)
{
    int r = (int)((float)rand() * (float)(max + 1) * M3D_RAND_K);
    if (max < r)
        r = max;
    return r;
}


// v12 0x0056E7C0  (feb 0x005566A0)
// Uniform int in [lo,hi].
int __fastcall m3dRandIntRange(int lo, int hi)
{
    int range = hi - lo;
    int r = (int)((float)rand() * (float)(range + 1) * M3D_RAND_K);
    if (range < r)
        return range + lo;
    return r + lo;
}


// v12 0x0056E800  (feb 0x005566E0)
// LCG: int in [0,max] from the high 15 bits.
int m3dRND_GEN::RndIntMax(int max)
{
    M3D_LCG(seed);
    return (int)((seed >> 16) & 0x7FFF) * max / 0x7FFF;
}


// v12 0x0056E840  (feb 0x00556720)
// LCG: float in [0,1) from the full 32-bit state.
float m3dRND_GEN::RndFloat()
{
    M3D_LCG(seed);
    return (float)((double)seed * M3D_2POW_M32);
}


// v12 0x0056E880  (feb 0x00556760)
// LCG: float in [0,max).
float m3dRND_GEN::RndFloatMax(float max)
{
    M3D_LCG(seed);
    return (float)seed * max * (float)M3D_2POW_M32;
}


// v12 0x0056E8C0  (feb 0x005567A0)
// LCG: float in [lo,hi).
float m3dRND_GEN::RndFloatRange(float lo, float hi)
{
    M3D_LCG(seed);
    return (float)seed * (hi - lo) * (float)M3D_2POW_M32 + lo;
}


// v12 0x0056E910  (feb 0x005567F0)
// Random barycentric pair (u,v) with u+v <= 1 (uniform over a triangle: the outer half is folded back).
void __fastcall m3dRandBasicCoord(float *u, float *v)
{
    float a = (float)rand() * M3D_RAND_K;
    float b = (float)rand() * M3D_RAND_K;
    if (a + b <= 1.0f) {
        *u = a;
        *v = b;
        return;
    }
    *u = 1.0f - b;
    *v = 1.0f - a;
}


// v12 0x0056E990  (feb 0x00556870)
// Random unit vector (cube sample, normalised -- not exactly uniform on the sphere).
void __fastcall m3dMakeVUnitRandom(m3dV *out)
{
    out->x = (float)rand() * M3D_RAND_K - 0.5f;
    out->y = (float)rand() * M3D_RAND_K - 0.5f;
    out->z = (float)rand() * M3D_RAND_K - 0.5f;
    m3dNormalize(out);
}


// v12 0x0056EA00  (feb 0x005568E0)
// Random direction inside the cone around 'dir' with half angle up to 'deg' degrees (weapon spread,
// particle cones). 'dir' should be unit length; a zero 'dir' gives a zero result.
// 1.2: same logic (inlined m3dSqrt rescheduled).
// NB: argument order is (dir, deg, out) -- the SDK lists (out, deg, axis); see recon_fixes/01_m3d.md.
void __fastcall m3dMakeVUnitConusRandom(m3dV *dir, float deg, m3dV *out)
{
    if (m3dLength(dir) < M3D_EPSILON) {
        out->x = out->y = out->z = 0.0f;
        return;
    }
    m3dV r;
    r.x = (float)rand() * M3D_RAND_K - 0.5f;
    r.y = (float)rand() * M3D_RAND_K - 0.5f;
    r.z = (float)rand() * M3D_RAND_K - 0.5f;
    m3dNormalize(&r);
    float k = m3dDot(&r, dir);
    m3dV perp = { r.x - k * dir->x, r.y - k * dir->y, r.z - k * dir->z };   // random vector _|_ dir
    if (M3D_EPSILON < m3dLength(&perp)) {
        m3dNormalize(&perp);
        float a = (float)rand() * deg * M3D_RAND_K * M3D_PI * M3D_DEG2RAD;
        float c = m3dCos(a), s = m3dSin(a);
        out->x = perp.x * s + c * dir->x;
        out->y = perp.y * s + c * dir->y;
        out->z = perp.z * s + c * dir->z;
        m3dNormalize(out);
        return;
    }
    *out = *dir;
}


// v12 0x0056ED00  (feb 0x00556BE0)
// Random unit vector perpendicular to n (tangent plane). A zero n is treated as +Y.
void __fastcall m3dMakeVTangRandom(m3dV *n, m3dV *out)
{
    m3dV nn = *n;
    if (!m3dNormalize(&nn))
        nn = m3dVUnitY;
    out->x = (float)rand() * M3D_RAND_K - 0.5f;
    out->y = (float)rand() * M3D_RAND_K - 0.5f;
    out->z = (float)rand() * M3D_RAND_K - 0.5f;
    m3dNormalize(out);
    float k = m3dDot(&nn, out);
    out->x -= nn.x * k;
    out->y -= nn.y * k;
    out->z -= nn.z * k;
    m3dNormalize(out);
}


// v12 0x0056EE00  (feb 0x00556CE0)
// Random point in a cylinder with axis 'axis' (unit) from the origin: radial distance in [0,r],
// height in [0,h] along the axis. A zero axis gives a zero result.
// 1.2: same logic (inlined m3dSqrt rescheduled).
void __fastcall m3dMakeVCylinderRandom(m3dV *axis, float r, float h, m3dV *out)
{
    if (m3dLength(axis) < M3D_EPSILON) {
        out->x = out->y = out->z = 0.0f;
        return;
    }
    m3dV p;
    p.x = (float)rand() * M3D_RAND_K - 0.5f;
    p.y = (float)rand() * M3D_RAND_K - 0.5f;
    p.z = (float)rand() * M3D_RAND_K - 0.5f;
    m3dNormalize(&p);
    float k = m3dDot(&p, axis);
    p.x -= k * axis->x;
    p.y -= k * axis->y;
    p.z -= k * axis->z;
    m3dNormalize(&p);
    float rr = (float)rand() * r * M3D_RAND_K;
    p.x *= rr; p.y *= rr; p.z *= rr;
    float hh = (float)rand() * h * M3D_RAND_K;
    out->x = hh * axis->x + p.x;
    out->y = hh * axis->y + p.y;
    out->z = hh * axis->z + p.z;
}


// v12 0x0056F020  (feb 0x00556F00)
// Picks a or b with equal probability.
int __fastcall m3dRandSelect(int a, int b)
{
    if (rand() > 0x3FFE)
        a = b;
    return a;
}


// v12 0x0056F040  (feb 0x00556F20)
// Picks one of a, b, c with equal probability.
int __fastcall m3dRandSelect(int a, int b, int c)
{
    int r = rand();
    if (r < 0x2AAA)
        return a;
    if (r > 0x5553)
        b = c;
    return b;
}


// v12 0x0056F070  (feb 0x00556F50)
// Picks one of a, b, c, d with equal probability.
int __fastcall m3dRandSelect(int a, int b, int c, int d)
{
    int r = rand();
    if (r < 0x1FFF)
        return a;
    if (r < 0x3FFF)
        return b;
    if (r > 0x5FFE)
        c = d;
    return c;
}
