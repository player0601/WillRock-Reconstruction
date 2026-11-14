// m3d_noise.h -- Will Rock 1.2 reconstructed source: what m3d_noise.cpp needs from m3d_spl.obj (other batch).
// Layout as math_app/m3d.h m3dSPL (size 0x34; slot 0 = CompareKP, slot 1 = deleting dtor).
#pragma once
#include "m3d_geom.h"

class m3dSPL {
public:
    virtual int    CompareKP(void *a, void *b);           // +0x00
    virtual void  *__vecDelDtor(unsigned flags);          // +0x04
    virtual void   GetKpValue(int i, void *out) = 0;      // +0x08
    virtual void   SetKpValue(int i, void *v) = 0;        // +0x0C
    virtual void   GetValue(float t, void *out, void *deriv) = 0; // +0x10
    unsigned flags;                                       // +0x04
    int   type;                                           // +0x08
    int   nKp;                                            // +0x0C
    int   nKpMax;                                         // +0x10
    int   kpValueSize;                                    // +0x14
    int   kpStride;                                       // +0x18
    int   dim;                                            // +0x1C
    int   kpBytes;                                        // +0x20
    void *kp;                                             // +0x24
    int   pad[3];                                         // +0x28..+0x30
    BOOL32 AllocKpList(int n);                            // v12 0x005660F0 (feb 0x0054E560)
};

// Lagrange spline (type 6). The constructor is inline in the original (seen in m3dInitNoise).
class m3dSPL_LAGRANGE : public m3dSPL {                   // vtable v12 ? (feb 0x005DE490)
public:
    m3dSPL_LAGRANGE()
    {
        flags = 0; type = 6; nKp = 0; kpValueSize = 8; kpStride = 8; dim = 1;
        kpBytes = 0; kp = 0; pad[0] = pad[1] = pad[2] = 0;
    }
    void *__vecDelDtor(unsigned flags);
    void  GetKpValue(int i, void *out);
    void  SetKpValue(int i, void *v);
    void  GetValue(float t, void *out, void *deriv);
};
extern m3dSPL *m3dNoiseSpl;
