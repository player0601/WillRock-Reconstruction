// cdt_sys.cpp -- engine.lib voxel collision grid: construction, storage, the face-list scratch
// (Will Rock 1.2, v12.exe; feb obj engine:cdt_sys.obj)
//
// v12 0x00503580..0x005039CF. cdtCOLL uses the 1.0/1.2 layout (bitset marks, see engine.h); the
// constructor, destructor, AllocLists and CalcTotalMemory changed accordingly ("callgraph" matches).

#include "engine.h"

// v12 0x00503580  (feb 0x004F11E0)
cdtCOLL::cdtCOLL()
{
    flags = 0;
    grid.nx = m3dVIntZero.x;  grid.ny = m3dVIntZero.y;  grid.nz = m3dVIntZero.z;
    m3dBOX_DATA *box = (m3dBOX_DATA *)((char *)&grid + 0x0C);   // the voxGRID's embedded m3dBOX
    box->type = M3D_VOL_BOX;
    box->vtbl = (void *)m3dBOX_vftable;
    grid.voxSize = m3dVZero;
    nObjMark = 0;    objMark = 0;
    nFaceMark = 0;   faceMark = 0;
    cellStart = 0;   cellObj = 0;   nObj = 0;
    entFaceStart = 0; entFace = 0;  objFaceBase = 0;
    nFaceTotal = 0;  data = 0;
}

// v12 0x00503680  (feb 0x004F12D0)
void cdtCOLL::Destroy()
{
    apFree(cellStart);
    apFree(cellObj);
    apFree(objMark);
    objMark = 0;
    nObjMark = 0;
    apFree(entFaceStart);
    apFree(entFace);
    apFree(objFaceBase);
    apFree(faceMark);
    faceMark = 0;
    nFaceMark = 0;
    ((m3dBOX_DATA *)((char *)&grid + 0x0C))->vtbl = (void *)m3dVOL_vftable;   // ~m3dBOX of the grid box
}

// v12 0x00503600  (feb 0x004F1260)  scalar deleting destructor (cdtCOLL_OBJ/SCN/INST use v12 0x004CECF0)
void *cdtCOLL::__vecDelDtor(unsigned flags)
{
    Destroy();
    if (flags & 1)
        operator delete(this);
    return this;
}

// v12 0x0048D590  identical-code-folded "return 0" (a dozen unrelated methods share it)
BOOL32 cdtCOLL::RefineInd(int id)
{
    return 0;
}

// v12 0x00403860  identical-code-folded empty two-argument method
void cdtCOLL::RefineIndList(cdtOFL *ofl, int mode)
{
}

// v12 0x005036F0  (feb 0x004F1330)
// Size every list of the grid. nCell cells, nObj ids, nEntry (cell, id) entries; with per-face lists
// (flags & 1) also nFace face bits in total and nEntFace (entry, face) pairs.
BOOL32 cdtCOLL::AllocLists(int nCell, int nObjIds, int nEntry, int nFace, int nEntFace)
{
    nObj = nObjIds;
    cellStart = (int *)apRealloc(cellStart, nCell * 4 + 4);
    if (!cellStart)
        return 0;
    nObjMark = nObj;
    objMark = (u32 *)apRealloc(objMark, (nObjMark / 32) * 4 + 4);
    if (!objMark)
        return 0;
    cellObj = (u16 *)apRealloc(cellObj, nEntry * 2);
    if (!cellObj)
        return 0;
    if (flags & 1) {
        nFaceTotal = nFace;
        entFaceStart = (int *)apRealloc(entFaceStart, nEntry * 4 + 4);
        if (!entFaceStart)
            return 0;
        entFace = (u16 *)apRealloc(entFace, nEntFace * 2);
        if (!entFace && nEntFace > 0)
            return 0;
        objFaceBase = (int *)apRealloc(objFaceBase, nObjIds * 4 + 4);
        if (!objFaceBase)
            return 0;
        nFaceMark = nFaceTotal;
        faceMark = (u32 *)apRealloc(faceMark, (nFaceMark / 32) * 4 + 4);
        if (!faceMark)
            return 0;
    }
    return 1;
}

// v12 0x00503830  (feb 0x004F1440)
// Bytes the grid takes (used by the level loader's memory report). 1.2 counts the bitsets as n/8 bytes.
int cdtCOLL::CalcTotalMemory()
{
    voxGRID_EXT *g = (voxGRID_EXT *)&grid;
    int nVox = g->GetNVoxels();
    int nEnt = 0;
    if (nObj) {
        int n = g->GetNVoxels();
        for (int i = 0; i < n; i++)
            nEnt += cellStart[i + 1] - cellStart[i];
    }
    int total = (nObj / 8) + (nEnt + nVox * 2 + 0x38) * 2;
    if (!entFace)
        return total;
    int nEntFace = 0;
    if (nFaceTotal) {
        int ne = 0;
        if (nObj) {
            int n = g->GetNVoxels();
            for (int i = 0; i < n; i++)
                ne += cellStart[i + 1] - cellStart[i];
        }
        for (int i = 0; i < ne; i++)
            nEntFace += entFaceStart[i + 1] - entFaceStart[i];
    }
    return total + (nFaceTotal / 8) + (nEntFace + (nObj + nEnt) * 2) * 2;
}

// v12 0x00503910  (feb 0x004F1510)
// Face lists are only allocated when maxFace != 0. On an allocation failure maxObj/maxFace stay 0.
cdtOFL *cdtOFL_Construct(cdtOFL *o, int maxObj, int maxFace)
{
    memset(o, 0, sizeof(*o));
    o->obj = (u16 *)apCalloc(2, maxObj);
    if (!o->obj)
        return o;
    if (maxFace) {
        if (!(o->nObjFace = (u16 *)apCalloc(2, maxObj)))
            return o;
        if (!(o->objFaceStart = (int *)apCalloc(4, maxObj)))
            return o;
        if (!(o->face = (u16 *)apCalloc(2, maxFace)))
            return o;
    }
    o->maxObj  = maxObj;
    o->maxFace = maxFace;
    return o;
}

// v12 0x00503990  (feb 0x004F1590)
void cdtOFL_Destroy(cdtOFL *o)
{
    if (o->obj)          apFree(o->obj);
    if (o->nObjFace)     apFree(o->nObjFace);
    if (o->objFaceStart) apFree(o->objFaceStart);
    if (o->face)         apFree(o->face);
}

// feb 0x004F15E0 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
