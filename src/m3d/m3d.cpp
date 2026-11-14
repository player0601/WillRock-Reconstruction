// m3d.cpp -- Will Rock 1.2 (v12.exe): m3d library init / term, SIMD detection, float validity checks
//
// Reconstructed from the Feb 2003 build (m3d:m3d.obj) and checked against v12 disassembly.
// v12 range 0x005695C0 .. 0x005698A2 (feb 0x00551530 .. 0x005517E2).

#include "m3d_lib.h"

int m3dSimdType = M3D_SIMD_NONE;   // v12 0x00637C24 (feb 0x005F99D0) 0 SSE, 1 3DNow!, 2 none (.data, initialised 2 ?)

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00569890 (feb 0x005517D0) -> v12 0x009AEAC8 (feb 0x0096377C)

static const float M3D_VALID_RANGE = M3D_INFINITY + 100.0f;  // v12 0x00609A60 + __real@42c80000


// v12 0x005695C0  (feb 0x00551530)
// Library start-up: SIMD detection, 2D and 3D matrix tables, global Perlin noise.
BOOL32 __fastcall m3dInit()
{
    _m3dDetectSIMD();
    if (!m3dInit2DMatr())
        return 0;
    if (!m3dInit3DMatr())
        return 0;
    return m3dInitNoise() != 0;
}


// v12 0x005695F0  (feb 0x00551560)
// Library shutdown: frees the noise table; Term2DMatr / Term3DMatr are empty (both folded to one 'ret').
void __fastcall m3dTerm()
{
    m3dTermNoise();
    // m3dTerm3DMatr(); m3dTerm2DMatr();  -- empty functions (v12 call/jmp 0x00401380, a shared 'ret')
}


// v12 0x00569600  (feb 0x00551570)
// Hardware-dependent init: re-detects SIMD.
// 1.2: reads [Debug] Decomp_OFF_SIMD (bool, default No) and forces the x87 paths (m3dSimdType = 2) when set
//      (already in 1.0, jun 0x00568950; Saber's own debug switch, same family as Decomp_OFF_AI).
// TWEAK: Decomp_OFF_SIMD=Yes in the [Debug] section of the game's ini disables SSE/3DNow! paths.
BOOL32 __fastcall m3dInitHW()
{
    _m3dDetectSIMD();
    int off = 0;
    _apCfgReadBool(0, (char *)"Debug", (char *)"Decomp_OFF_SIMD", &off);   // 1.2: new
    if (off)
        m3dSimdType = M3D_SIMD_NONE;                                        // 1.2: new
    return 1;
}


// v12 0x00569640  (feb 0x00551580)
// Debug check: not NaN and within +-(M3D_INFINITY + 100).
BOOL32 __fastcall _m3dCheckValid(float f)
{
    if (!__builtin_isnan(f) && -M3D_VALID_RANGE <= f && f <= M3D_VALID_RANGE)
        return 1;
    return 0;
}


// v12 0x005696A0  (feb 0x005515E0)
// Debug check of a vertex. As compiled it returns 0 as soon as x or y is VALID and 1 only when both
// are invalid (z is never looked at) -- the original test is inverted.  // ?
BOOL32 __fastcall _m3dCheckValid(const m3dVTX *v)
{
    if (_m3dCheckValid(v->x))
        return 0;
    if (_m3dCheckValid(v->y))
        return 0;
    return 1;
}


// v12 0x00569760  (feb 0x005516A0)
// CPUID: SSE (edx bit 25) -> 0, else AMD extended leaf 0x80000001 edx bit 31 (3DNow!) -> 1, else 2.
// The first CPUID runs inside __try so a CPU without CPUID leaves m3dSimdType = 2.
void __fastcall _m3dDetectSIMD()
{
    m3dSimdType = M3D_SIMD_NONE;
    unsigned a, b, c, d;
    // __try { cpuid(1); } __except(1) { return; }   -- SEH guard, MinGW has no __try
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(1));
    __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(1));
    if (d & 0x02000000) {
        m3dSimdType = M3D_SIMD_SSE;
    } else {
        __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0x80000000));
        if (a >= 0x80000001) {
            __asm__ volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(0x80000001));
            if (d & 0x80000000)
                m3dSimdType = M3D_SIMD_3DNOW;
        }
    }
    switch (m3dSimdType) {
    case M3D_SIMD_SSE:   apLog((char *)"SSE detected.");                 break;
    case M3D_SIMD_3DNOW: apLog((char *)"3DNOW detected.");               break;
    default:             apLog((char *)"No SIMD extensions detected.");  break;
    }
}


// v12 0x00569880  (feb 0x005517C0)
// Current SIMD path (bit 31 masked off: reserved as an "override" marker ?).
int __fastcall m3dGetSimdType()
{
    return m3dSimdType & 0x7FFFFFFF;
}
