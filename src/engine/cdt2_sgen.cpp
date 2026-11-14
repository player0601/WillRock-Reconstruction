// cdt2_sgen.cpp -- engine.lib two-level collision grid generator (Will Rock 1.2, v12.exe;
// feb obj engine:cdt2_sgen.obj)
//
// v12 0x00502D70..0x0050355F. CalcData2 builds the voxel grid of any 2-level cdtCOLL (level geometry,
// instances) from its accessors: every face (id2) of every id is put in each cell its box overlaps, then
// the per-cell lists are packed into cellStart/cellObj/entFaceStart/entFace. Same algorithm as the beta,
// on the 1.0/1.2 cdtCOLL layout; 1.2 also defaults vz to 10 when negative (the beta only did vx).

#include "engine.h"

// Generator scratch (CalcData2's locals; _cdt2AddID receives a pointer to it).
struct _cdt2VOX_INFO { int head, tail; };                       // one per cell, entry indices, -1 = none
struct _cdt2ENTRY    { u32 key; int next; };                    // key = id * maxN2 + id2
struct _cdt2GEN_DATA {
    voxGRID      grid;                                          // +0x00 copy of the target grid (0x38)
    _cdt2VOX_INFO *vox;                                         // +0x38 [nVox]
    int          nEntry;                                        // +0x3C distinct (cell, id) pairs
    int          nEntFace;                                      // +0x40 (cell, id, id2) triples
    u32          maxN2;                                         // +0x44 max GetNID2 over all ids
    int          count;                                         // +0x48 entries used
    _cdt2ENTRY  *entries;                                       // +0x4C grown by 'dyn'
    dsDYN_ARRAY  dyn;                                           // +0x50
};
static_assert(offsetof(_cdt2GEN_DATA, vox) == 0x38 && offsetof(_cdt2GEN_DATA, dyn) == 0x50, "_cdt2GEN_DATA");

BOOL32 __fastcall _cdt2AddID(_cdt2GEN_DATA *g, _cdt2VOX_INFO *v, int id, int id2);   // v12 0x005034E0

static inline voxGRID_EXT *GX(voxGRID *g) { return (voxGRID_EXT *)g; }
static inline m3dBOX *GBOX(voxGRID *g)    { return GX(g)->Box(); }

// v12 0x00502D70  (feb 0x004F09D0)
// vx/vy/vz: voxel size per axis (<0: 10, 10 and "one layer" for y); maxX..maxZ: caps on the cell
// counts (0 = none); box: the grid bounds, 0 = GetBBoxAll.
BOOL32 cdtCOLL::CalcData2(float vx, float vy, float vz, int maxX, int maxY, int maxZ, m3dBOX *box)
{
    _cdt2GEN_DATA gen;
    memset(&gen, 0, sizeof(gen) - sizeof(gen.dyn));
    ((m3dBOX_DATA *)GBOX(&gen.grid))->type = M3D_VOL_BOX;
    ((m3dBOX_DATA *)GBOX(&gen.grid))->vtbl = (void *)m3dBOX_vftable;
    gen.grid.nx = m3dVIntZero.x;  gen.grid.ny = m3dVIntZero.y;  gen.grid.nz = m3dVIntZero.z;
    gen.grid.voxSize = m3dVZero;
    gen.dyn.flags = 0;
    gen.dyn.Init(sizeof(_cdt2ENTRY), 1000, (void **)&gen.entries, 0, 0);

    if (vx < 0) vx = 10.0f;
    if (vz < 0) vz = 10.0f;
    BOOL32 oneLayer = 0;
    if (vy < 0) { vy = 2000000.0f; oneLayer = 1; }

    m3dBOX *gb = GBOX(&grid);
    if (!box) {
        GetBBoxAll(gb);
    } else {
        ((m3dBOX_DATA *)gb)->type = ((m3dBOX_DATA *)box)->type;
        gb->vMin = box->vMin;
        gb->vMax = box->vMax;
    }
    int nx = voxCalcNVoxel(gb->vMax.x - gb->vMin.x, vx);
    int ny = oneLayer ? 1 : voxCalcNVoxel(gb->vMax.y - gb->vMin.y, vy);
    int nz = voxCalcNVoxel(gb->vMax.z - gb->vMin.z, vz);
    grid.nx = nx;  grid.ny = ny;  grid.nz = nz;
    if (maxX && maxX < nx) grid.nx = maxX;
    if (maxY && maxY < ny) grid.ny = maxY;
    if (maxZ && maxZ < nz) grid.nz = maxZ;
    GX(&grid)->CalcVoxSize();

    BOOL32 ok = 0;
    int nId = GetNID();
    if (nId >= 1 && Is2LevelData()) {
        u32 maxN2 = 0;
        for (int i = 0; i < nId; i++) {
            int n2 = GetNID2(i);
            if ((int)maxN2 < n2) maxN2 = n2;
        }
        gen.maxN2 = maxN2;
        if ((float)nId * (float)(int)maxN2 <= 4294967296.0f) {
            int nVox = GX(&grid)->GetNVoxels();
            gen.vox = (_cdt2VOX_INFO *)apCalloc(nVox, 8);
            if (gen.vox) {
                gen.grid = grid;
                for (int c = 0; c < nVox; c++)
                    gen.vox[c].head = gen.vox[c].tail = -1;
                struct { const void *vtbl; int type; m3dV vMin, vMax; } fbox = { m3dBOX_vftable, M3D_VOL_BOX },
                                                                       cbox = { m3dBOX_vftable, M3D_VOL_BOX };
                int nIds = GetNID();
                for (int id = 0; id < nIds; id++) {
                    if (!RefineInd(id))
                        continue;
                    int n2 = GetNID2(id);
                    void *d2 = GetID2Ptr(id);
                    for (int k = 0; k < n2; k++) {
                        GetID2BBox(d2, k, (m3dBOX *)&fbox);
                        ((m3dBOX *)&fbox)->Expand(0.001f);
                        int x0, y0, z0, x1, y1, z1;
                        GX(&gen.grid)->GetPosInd(&fbox.vMin, &x0, &y0, &z0, 1);
                        GX(&gen.grid)->GetPosInd(&fbox.vMax, &x1, &y1, &z1, 1);
                        GX(&gen.grid)->ClampIndices(&x0, &y0, &z0);
                        GX(&gen.grid)->ClampIndices(&x1, &y1, &z1);
                        for (int ix = x0; ix <= x1; ix++)
                            for (int iy = y0; iy <= y1; iy++)
                                for (int iz = z0; iz <= z1; iz++) {
                                    _cdt2VOX_INFO *v = &gen.vox[(gen.grid.ny * ix + iy) * gen.grid.nz + iz];
                                    GX(&gen.grid)->CalcVoxBBox(ix, iy, iz, (m3dBOX *)&cbox);
                                    ((m3dBOX *)&cbox)->Expand(0.001f);
                                    if (IsectID2BBox(d2, k, (m3dBOX *)&cbox) && !_cdt2AddID(&gen, v, id, k))
                                        goto fail;
                                }
                    }
                }
                flags |= 1;
                nFaceTotal = 0;
                for (int i = 0; i < nId; i++)
                    nFaceTotal += GetNID2(i);
                if (AllocLists(nVox, nId, gen.nEntry, nFaceTotal, gen.nEntFace)) {
                    int e = 0, f = 0;
                    for (int c = 0; c < nVox; c++) {
                        int k = 0;
                        int idx = gen.vox[c].head;
                        if (idx != -1) {
                            int cur = -1, cnt = 0, id = 0;
                            while (idx != -1) {
                                int next = gen.entries[idx].next;
                                u32 key  = gen.entries[idx].key;
                                id = key / maxN2;
                                int id2 = key % maxN2;
                                if (cur != id) {
                                    if (cur != -1) {
                                        cellObj[e + k] = (u16)cur;
                                        entFaceStart[e + k] = f;
                                        f += cnt;
                                        k++;
                                    }
                                    cnt = 0;
                                    cur = id;
                                }
                                entFace[f + cnt] = (u16)id2;
                                cnt++;
                                idx = next;
                            }
                            cellObj[e + k] = (u16)id;
                            entFaceStart[e + k] = f;
                            f += cnt;
                            k++;
                        }
                        cellStart[c] = e;
                        e += k;
                    }
                    cellStart[nVox] = e;
                    entFaceStart[e] = f;
                    int base = 0, i = 0;
                    for (; i < GetNID(); i++) {
                        objFaceBase[i] = base;
                        base += GetNID2(i);
                    }
                    objFaceBase[i] = base;
                    ok = 1;
                }
            }
        }
    }
fail:
    apFree(gen.vox);
    gen.dyn.ReleaseMem();
    gen.dyn.~dsDYN_ARRAY();
    return ok;
}

// v12 0x005034E0  (feb 0x004F1140)
// Append (id, id2) to a cell's list.
BOOL32 __fastcall _cdt2AddID(_cdt2GEN_DATA *g, _cdt2VOX_INFO *v, int id, int id2)
{
    if (!g->dyn.AddEntry(1))
        return 0;
    int idx = g->count++;
    _cdt2ENTRY *e = &g->entries[idx];
    e->key  = g->maxN2 * id + id2;
    e->next = -1;
    u32 prevId;
    if (v->head == -1) {
        prevId = 0xFFFFFFFF;
        v->head = idx;
    } else {
        _cdt2ENTRY *t = &g->entries[v->tail];
        prevId = t->key / g->maxN2;
        t->next = idx;
    }
    v->tail = idx;
    if (prevId != (u32)id)
        g->nEntry++;
    g->nEntFace++;
    return 1;
}

// feb 0x004F11C0 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
