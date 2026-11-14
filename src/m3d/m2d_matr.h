// m2d_matr.h -- m2dMATR methods (m3d: m2d_matr.obj). Will Rock 1.2 reconstruction.
#pragma once
#include "m3d_lib.h"

// SDK gap: math_app/m3d.h has "struct m2dMATR { float m[3][3]; };" without methods. The original
// members are reconstructed on this method-only view (same size, no vtable):
//   ?Identity@m2dMATR@@QAEXXZ v12 0x0057C470, ?Scale@m2dMATR@@QAEXMMH@Z 0x0057C4A0,
//   ?Translate@m2dMATR@@QAEXMMH@Z 0x0057C510, ?Rotate@m2dMATR@@QAEXMH@Z 0x0057C580,
//   ?MakeRot@m2dMATR@@QAEXM@Z 0x0057C6C0, ?MakePolyPoly@m2dMATR@@QAEHPAVm2dPOLY@@0@Z 0x0057C700
struct m2dMATR_ : public m2dMATR {
    void   Identity();
    void   Scale(float sx, float sy, int op);
    void   Translate(float x, float y, int op);
    void   Rotate(float deg, int op);
    void   MakeRot(float deg);
    BOOL32 MakePolyPoly(m2dPOLY *from, m2dPOLY *to);
};
extern m2dMATR m2dMatrIdentity;     // v12 0x009AED14 (feb 0x009639C4)
