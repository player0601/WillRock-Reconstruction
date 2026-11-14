// m3d_vect.h -- Will Rock 1.2 reconstruction: private declarations of m3d_vect.obj (batch 02_m3d)
#pragma once
#include "m3d_lib.h"

// globals of m3d_vect.obj that the SDK does not declare
extern m2dV m2dVUnitX;     // v12 ? (feb 0x005F99B4) {1,0}
extern m2dV m2dVUnitY;     // v12 ? (feb 0x005F99BC) {0,1}
extern m2dV m2dVOneXY;     // v12 ? (feb 0x005F99C4) {1,1}
extern m2dV m2dVZero;      // v12 0x009AEA9C ? (feb 0x00963754)
extern m2dV m2dVUndef;     // v12 0x009AEAA4 (feb 0x0096375C) {888.888, 888.888}

#define M3D_UNDEF_COORD 888.888f    // 0x445E38D5 "no position" marker (v12 0x00609920, feb 0x005DE450)

float __fastcall m2dAngleVector(m2dV *a, m2dV *b);
