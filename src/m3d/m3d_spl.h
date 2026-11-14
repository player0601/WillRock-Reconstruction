// m3d_spl.h -- Will Rock 1.2 reconstruction: the spline classes of m3d_spl.obj (batch 02_m3d)
//
// The SDK (math_app/m3d.h) declares only the abstract m3dSPL base. This header adds the concrete classes
// (as they must have been in the original m3d.h), the members the SDK lacks, and the meaning of the three
// fields the SDK calls pad[3]. See re/work/recon_fixes/02_m3d.md.
#pragma once
#include "m3d_lib.h"

// ---------------------------------------------------------------------------------------------
// m3dSPL key-point storage
// ---------------------------------------------------------------------------------------------
// A key point is { float t; float value[dim]; <type-specific extras> } of kpStride bytes. Optimize() can
// compress the list in place; the flags then say how it is stored:
enum m3dSPL_FLAGS_EXT {
    M3D_SPLF_INTEGER_ARG = 0x01,   // key i has t == i (FindSeg uses floor(t))
    M3D_SPLF_CONST       = 0x02,   // all keys equal (FindSeg always returns -2 -> first key)
    M3D_SPLF_NO_TIME     = 0x04,   // t is not stored (implies INTEGER_ARG); GetKp writes out[0] = i
    M3D_SPLF_DIM_MAP     = 0x08,   // constant dims stripped: defKp holds the template key, dimMap the
                                   //   output index (1-based) of each stored value, 0 = unused
    M3D_SPLF_PACKED      = 0x10    // groups of kpPackNmb keys: 1 full key + (N-1) keys as 16-bit deltas
};
// SDK pad[0..2] at +0x28/+0x2C/+0x30:
#define SPL_DEFKP(s)    (*(float **)        ((char *)(s) + 0x28))   // template key (kpValueSize bytes), DIM_MAP
#define SPL_DIMMAP(s)   (*(unsigned char **)((char *)(s) + 0x2C))   // dim bytes, DIM_MAP
#define SPL_PACKSCALE(s)(*(float *)         ((char *)(s) + 0x30))   // delta unit = maxDelta/32000, PACKED

// static int m3dSPL::kpPackNmb  (".data", value 6)  v12 0x00637C20 (feb 0x005F99CC)
// (SDK gap: not declared on the class; reconstructed as a plain global with the same role)
extern int m3dSPL_kpPackNmb;

// Non-virtual members missing from the SDK class (view, no extra data)
struct m3dSPL_EXT : public m3dSPL {
    void  *GetKpPtr(int i);        // private inline: kp + stride*min(i, nKpMax)
    BOOL32 IsIntegerArg();         // v12 0x00566B70
    BOOL32 IsConst();              // v12 0x00566C10
};

// m3dSPL::m3dSPL(int type, int kpSize, int dim)  -- inline ctor, out-of-line copy v12 0x00403670 (plw_acd.obj)
inline void m3dSPL_Init(m3dSPL *s, int type, int kpSize, int dim)
{
    s->flags = 0;  s->nKp = 0;  s->kpBytes = 0;  s->kp = 0;
    s->pad[0] = s->pad[1] = s->pad[2] = 0;
    s->type = type;  s->kpValueSize = kpSize;  s->kpStride = kpSize;  s->dim = dim;
}

// ---------------------------------------------------------------------------------------------
// Concrete splines. Vtable slots: see math_app/m3d.h (slot 0 CompareKP ... slot 10 Convert).
// Slots not overridden (GetValueClosest, DistPoint, Convert defaults, the deleting dtor) are inline
// in the header, emitted in plw_acd.obj (v12 0x004036B0/0x004036C0/0x004036D0/0x004036E0).
// ---------------------------------------------------------------------------------------------
class m3dSPL_LINEAR1D : public m3dSPL {          // vtable v12 0x005F91C0 (feb 0x005CE508); kp {t, v}
public:
    int   CompareKP(void *a, void *b);
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);
    void  GetValue(float t, void *out, void *deriv);
    void  SetKp(int i, float t, float v);
};
class m3dSPL_LINEAR2D : public m3dSPL {          // vtable v12 0x00609A1C (feb 0x005DE540); kp {t, x, y}
public:
    int   CompareKP(void *a, void *b);
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);
    void  GetValue(float t, void *out, void *deriv);
};
class m3dSPL_LINEAR3D : public m3dSPL {          // vtable v12 0x006099F0 (feb 0x005DE514); kp {t, m3dV}
public:
    int   CompareKP(void *a, void *b);
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);
    void  GetValue(float t, void *out, void *deriv);
    void  GetValueClosest(float t, void *out, void *deriv);
    float DistPoint(void *p, int *seg, float *t, void *closest);
};
class m3dSPL_HERMIT : public m3dSPL {            // vtable v12 0x006099C4 (feb 0x005DE4E8); kp {t, v, tanIn, tanOut}
public:
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);
    void  GetValue(float t, void *out, void *deriv);
};
class m3dSPL_LAGRANGE : public m3dSPL {          // vtable v12 0x0060996C (feb 0x005DE490); kp {t, v}
public:
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);
    void  GetValue(float t, void *out, void *deriv);
};
class m3dSPL_BEZIER : public m3dSPL {            // abstract helper base of the two Bezier splines
protected:
    void  GetValueDim(float t, int dim, void *p0, void *c0, void *p1, void *c1, void *out, void *deriv);
};
class m3dSPL_BEZIER2D : public m3dSPL_BEZIER {   // vtable v12 0x005FB8A0 (feb 0x005D0B2C); kp {t, p, in, out} (m2dV)
public:
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);            // v12 0x0042DEB0 (inline, emitted outside this obj)
    void  GetValue(float t, void *out, void *deriv);
    void  SetKp(int i, float t, m2dV *p, m2dV *in, m2dV *out);
};
class m3dSPL_BEZIER3D : public m3dSPL_BEZIER {   // vtable v12 0x00609998 (feb 0x005DE4BC); kp {t, p, in, out} (m3dV)
public:
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);
    void  GetValue(float t, void *out, void *deriv);
    m3dSPL *Convert(int type, int n);
};
class m3dSPL_QUAT : public m3dSPL {              // vtable v12 0x00609940 (feb 0x005DE464); kp {t, m3dQUAT}
public:
    int   CompareKP(void *a, void *b);
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);
    void  GetValue(float t, void *out, void *deriv);
    void  GetValueClosest(float t, void *out, void *deriv);
};
class m3dSPL_COLOR : public m3dSPL {             // vtable v12 0x005FA93C (feb 0x005CFB54); kp {t, r, g, b, a}
public:
    int   CompareKP(void *a, void *b);
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);
    void  GetValue(float t, void *out, void *deriv);
    void  SetKp(int i, float t, float r, float g, float b, float a);
};

static_assert(sizeof(m3dSPL) == 0x34, "m3dSPL size");
