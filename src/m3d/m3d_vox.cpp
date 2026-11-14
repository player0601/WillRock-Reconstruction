// m3d_vox.cpp -- Will Rock 1.2 (v12.exe): voxGRID, the uniform voxel grid (collision / visibility acceleration)
//
// Reconstructed from the Feb 2003 build (m3d:m3d_vox.obj) and checked against v12.
// v12 range 0x0057EBC0 .. 0x0057F6FF (feb 0x005662A0 .. 0x00566DDF). All functions unchanged in 1.2.
// Voxel (ix,iy,iz) has linear index (ix*ny + iy)*nz + iz.

#include "m3d_vox.h"

M3D_DECLARE_EPS_SQ();   // _$E2: feb 0x00566DE0 (map labels it mino_ax.obj's, it follows this obj) // ?

static const float VOX_EPS = M3D_EPSILON;     // v12 0x00609B48 (feb 0x005DE65C)
static const float VOX_BIG = M3D_INFINITY;    // v12 0x00609B4C (feb 0x005DE660)


// v12 0x0057EBC0  (feb 0x005662A0)
// Total number of voxels nx*ny*nz.
int voxGRID_EXT::GetNVoxels()
{
    return nz * ny * nx;
}


// v12 0x0057EBD0  (feb 0x005662B0)
// Voxel indices of a world point (any out pointer may be 0); with `clamp` they are clamped to the grid.
void voxGRID_EXT::GetPosInd(m3dV *p, int *ix, int *iy, int *iz, BOOL32 clamp)
{
    if (ix) *ix = (int)((p->x - bbMin.x - VOX_EPS) / voxSize.x);
    if (iy) *iy = (int)((p->y - bbMin.y - VOX_EPS) / voxSize.y);
    if (iz) *iz = (int)((p->z - bbMin.z - VOX_EPS) / voxSize.z);
    if (clamp)
        ClampIndices(ix, iy, iz);           // inlined in the original
}


// v12 0x0057ECB0  (feb 0x00566390)
// Clamps the given indices (pointers may be 0) to [0, n-1].
void voxGRID_EXT::ClampIndices(int *ix, int *iy, int *iz)
{
    if (ix) { int v = *ix; if (v < 0) v = 0; else if (v > nx - 1) v = nx - 1; *ix = v; }
    if (iy) { int v = *iy; if (v < 0) v = 0; else if (v > ny - 1) v = ny - 1; *iy = v; }
    if (iz) { int v = *iz; if (v < 0) v = 0; else if (v > nz - 1) v = nz - 1; *iz = v; }
}


// v12 0x0057ED20  (feb 0x00566400)
// voxSize = (bbMax-bbMin)/n. Returns 0 if a count is <= 0 (size untouched) or the box is flat on an axis
// (size is still computed in that case).
BOOL32 voxGRID_EXT::CalcVoxSize()
{
    if (nx <= 0 || ny <= 0 || nz <= 0)
        return 0;
    BOOL32 ok = 1;
    if (m3dFAbs(bbMax.x - bbMin.x) < VOX_EPS ||
        m3dFAbs(bbMax.y - bbMin.y) < VOX_EPS ||
        m3dFAbs(bbMax.z - bbMin.z) < VOX_EPS)
        ok = 0;
    voxSize.x = (bbMax.x - bbMin.x) / (float)nx;
    voxSize.y = (bbMax.y - bbMin.y) / (float)ny;
    voxSize.z = (bbMax.z - bbMin.z) / (float)nz;
    return ok;
}


// v12 0x0057EE60  (feb 0x00566540)
// Number of voxels needed to cover `size` with cells of `voxSize`: (int)(size/voxSize) + 1.
int __fastcall voxCalcNVoxel(float size, float voxSize)
{
    return (int)(size / voxSize) + 1;
}


// v12 0x0057EE80  (feb 0x00566560)
// World-space box of voxel (ix,iy,iz).
void voxGRID_EXT::CalcVoxBBox(int ix, int iy, int iz, m3dBOX *out)
{
    out->vMin.x = (float)ix * voxSize.x + bbMin.x;
    out->vMin.y = (float)iy * voxSize.y + bbMin.y;
    out->vMin.z = (float)iz * voxSize.z + bbMin.z;
    out->vMax.x = voxSize.x + out->vMin.x;
    out->vMax.y = voxSize.y + out->vMin.y;
    out->vMax.z = voxSize.z + out->vMin.z;
}


// Inline helpers of MakeIndListRay
static inline float voxClamp(float v, float hi)    // clamp(v, 0, hi) with ordered bounds
{
    float lo = 0.0f;
    if (hi < 0.0f) { lo = hi; hi = 0.0f; }
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
static inline float voxFloor(float v)               // inline m3dFloor(v, 1.0): floor unless step < eps
{
    if (1.0f < VOX_EPS) return v;
    return (float)__builtin_floor((double)v);
}

// DDA setup on one axis (grid units): step direction, tDelta, first tMax. Degenerate axis -> tMax = BIG.
static inline void voxDdaAxis(float s, float &e, int &step, float &tDelta, float &tMax)
{
    if (m3dFAbs(s - e) < VOX_EPS) {
        e = s;
        tMax = VOX_BIG;
        return;
    }
    if (e <= s) {
        step   = -1;
        tDelta = 1.0f / (s - e);
        tMax   = (s - voxFloor(s)) * tDelta;
    } else {
        step   = 1;
        tDelta = 1.0f / (e - s);
        tMax   = (1.0f - (s - voxFloor(s))) * tDelta;
    }
}


// v12 0x0057EED0  (feb 0x005665B0)
// Lists the voxels crossed by segment a-b (3D DDA, Amanatides-Woo) into list[], count in *count.
// The segment is first clipped to the grid box; nothing is listed if it misses.
// NB maxCount is NOT checked (as compiled): list must hold |dix|+|diy|+|diz|+1 entries.
void voxGRID_EXT::MakeIndListRay(m3dV *a, m3dV *b, int *list, int *count, int maxCount)
{
    *count = 0;
    m3dV s, e;
    if (!Box()->ClipLSeg(a, b, &s, &e))
        return;

    float kx = 1.0f / voxSize.x, ky = 1.0f / voxSize.y, kz = 1.0f / voxSize.z;
    s.x = (s.x - bbMin.x) * kx;  s.y = (s.y - bbMin.y) * ky;  s.z = (s.z - bbMin.z) * kz;
    e.x = (e.x - bbMin.x) * kx;  e.y = (e.y - bbMin.y) * ky;  e.z = (e.z - bbMin.z) * kz;

    float lim = 1.0f - VOX_EPS;
    float hx = (float)nx * lim, hy = (float)ny * lim, hz = (float)nz * lim;
    s.x = voxClamp(s.x, hx);  s.y = voxClamp(s.y, hy);  s.z = voxClamp(s.z, hz);
    e.x = voxClamp(e.x, hx);  e.y = voxClamp(e.y, hy);  e.z = voxClamp(e.z, hz);

    int   stepX = 0, stepY = 0, stepZ = 0;
    float dtX = 0, dtY = 0, dtZ = 0, tX, tY, tZ;
    voxDdaAxis(s.x, e.x, stepX, dtX, tX);
    voxDdaAxis(s.y, e.y, stepY, dtY, tY);
    voxDdaAxis(s.z, e.z, stepZ, dtZ, tZ);

    int ix = (int)s.x, iy = (int)s.y, iz = (int)s.z;
    int ex = (int)e.x, ey = (int)e.y, ez = (int)e.z;
    int n = __builtin_abs(ez - iz) + __builtin_abs(ey - iy) + 1 + __builtin_abs(ex - ix);
    if (ix == ex) tX = VOX_BIG;
    if (iy == ey) tY = VOX_BIG;
    if (iz == ez) tZ = VOX_BIG;

    for (int i = 0; i < n; i++) {
        list[i] = (ix * ny + iy) * nz + iz;
        if (tX < tY) {
            if (tX < tZ) {
                ix += stepX;
                tX = (ix == ex) ? VOX_BIG : tX + dtX;
                continue;
            }
        } else if (tY < tZ) {
            iy += stepY;
            tY = (iy == ey) ? VOX_BIG : tY + dtY;
            continue;
        }
        iz += stepZ;
        tZ = (iz == ez) ? VOX_BIG : tZ + dtZ;
    }
    *count = n;
}
