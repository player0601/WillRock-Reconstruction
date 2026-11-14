// m3d_vox.h -- Will Rock 1.2 reconstruction: private declarations of m3d_vox.obj (batch 02_m3d)
#pragma once
#include "m3d_lib.h"

// SDK gap/fix: voxGRID (math_app/m3d.h) has no methods, and its "_0C, _10, bbMin, bbMax" are in fact an
// embedded m3dBOX at +0x0C (vtable +0x0C, type +0x10, vMin +0x14, vMax +0x20): MakeIndListRay calls
// m3dBOX::ClipLSeg with this+0x0C (v12 0x0057EEF2 "lea esi,[edi+0xc]"). Size >= 0x38.
// The methods are reconstructed on this view class (no extra data). See re/work/recon_fixes/02_m3d.md.
struct voxGRID_EXT : public voxGRID {
    m3dBOX *Box() { return (m3dBOX *)((char *)this + 0x0C); }
    int    GetNVoxels();
    void   GetPosInd(m3dV *p, int *ix, int *iy, int *iz, BOOL32 clamp);
    void   ClampIndices(int *ix, int *iy, int *iz);
    BOOL32 CalcVoxSize();
    void   CalcVoxBBox(int ix, int iy, int iz, m3dBOX *out);
    void   MakeIndListRay(m3dV *a, m3dV *b, int *list, int *count, int maxCount);
};

int __fastcall voxCalcNVoxel(float size, float voxSize);
