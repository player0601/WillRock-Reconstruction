// Cdtm_sys.cpp -- engine.lib voxel-grid collision: candidate lists (Will Rock 1.2, v12.exe;
// feb obj engine:Cdtm_sys.obj)
//
// v12 0x00502780..0x00502D6F. Same algorithm as the beta, rebuilt for the 1.0/1.2 cdtCOLL layout: the
// "already listed" marks are bitsets instead of byte arrays (engine.h, class cdtCOLL). In 1.2 the face
// collector is still a separate fastcall function (v12 0x00502C50), no twin was found by the matcher
// because its body changed with the bitsets.
//
// A query (box or ray) walks the grid cells it touches and lists every object found in them once
// (cdtOFL::obj); with per-face lists (flags & 1) it also marks the faces of those objects that lie in
// the cells, then _cdtOFLCollectFaceList turns the marks into per-object face lists. The subclass then
// filters the result through RefineIndList (vtable +0x10).

#include "engine.h"

static inline BOOL32 cdtBitTest(const u32 *bits, int i) { return (bits[i >> 5] >> (i & 31)) & 1; }
static inline void   cdtBitSet(u32 *bits, int i)        { bits[i >> 5] |= 1u << (i & 31); }

static inline void cdtClearMarks(cdtCOLL *c)
{
    memset(c->objMark, 0, (c->nObjMark / 32) * 4 + 4);
    if (c->flags & 1)
        memset(c->faceMark, 0, (c->nFaceMark / 32) * 4 + 4);
}

// One grid cell: add its objects (once each), and mark their faces that lie in this cell.
// Returns 0 when the object list is full (the query stops there, as in the original's goto).
static inline BOOL32 cdtCollectCell(cdtCOLL *c, int cell, cdtOFL *ofl)
{
    int first = c->cellStart[cell];
    int n = c->cellStart[cell + 1] - first;
    for (int e = 0; e < n; e++) {
        int o = c->cellObj[first + e];
        if (!cdtBitTest(c->objMark, o)) {
            if (ofl->nObj >= ofl->maxObj)
                return 0;
            ofl->obj[ofl->nObj++] = (u16)o;
            cdtBitSet(c->objMark, o);
        }
        if (c->flags & 1) {
            int ent = first + e;
            int f0 = c->entFaceStart[ent];
            int nf = c->entFaceStart[ent + 1] - f0;
            for (int k = 0; k < nf; k++)
                cdtBitSet(c->faceMark, c->entFace[f0 + k] + c->objFaceBase[o]);
        }
    }
    return 1;
}

// v12 0x00502780  (feb 0x004F0520)
// Everything in the box spanned by a and b, grown by 'expand'.
void cdtCOLL::MakeIndList(m3dV *a, m3dV *b, float expand, cdtOFL *ofl, int mode)
{
    struct { const void *vtbl; int type; m3dV vMin, vMax; } box = { m3dBOX_vftable, M3D_VOL_BOX };
    cdtClearMarks(this);
    ((m3dBOX *)&box)->MakePointPoint(a, b);
    ((m3dBOX *)&box)->Expand(expand);
    voxGRID_EXT *g = (voxGRID_EXT *)&grid;
    int x0, y0, z0, x1, y1, z1;
    g->GetPosInd(&box.vMin, &x0, &y0, &z0, 1);
    g->GetPosInd(&box.vMax, &x1, &y1, &z1, 1);
    g->ClampIndices(&x0, &y0, &z0);
    g->ClampIndices(&x1, &y1, &z1);
    ofl->nObj = 0;
    for (int ix = x0; ix <= x1; ix++)
        for (int iy = y0; iy <= y1; iy++)
            for (int iz = z0; iz <= z1; iz++)
                if (!cdtCollectCell(this, (grid.ny * ix + iy) * grid.nz + iz, ofl))
                    goto done;
done:
    RefineIndList(ofl, mode);
    if (flags & 1)
        _cdtOFLCollectFaceList(this, ofl);
}

// v12 0x00502A40  (feb 0x004F0770)
// Everything in the cells a segment a->b crosses (at most 2000 cells, 8 KB of stack: __chkstk).
void cdtCOLL::MakeIndListRay(m3dV *a, m3dV *b, cdtOFL *ofl, int mode)
{
    int cells[2000];
    int nCells;
    cdtClearMarks(this);
    ((voxGRID_EXT *)&grid)->MakeIndListRay(a, b, cells, &nCells, 2000);
    ofl->nObj = 0;
    for (int i = 0; i < nCells; i++)
        if (!cdtCollectCell(this, cells[i], ofl))
            break;
    RefineIndList(ofl, mode);
    if (flags & 1)
        _cdtOFLCollectFaceList(this, ofl);
}

// v12 0x00502C50  (feb 0x004F08F0)
// Marked faces -> per-object face lists in ofl (objFaceStart / nObjFace / face).
void __fastcall _cdtOFLCollectFaceList(cdtCOLL *c, cdtOFL *ofl)
{
    int nOut = 0;
    ofl->nFace = 0;
    for (int i = 0; i < ofl->nObj; i++) {
        ofl->objFaceStart[i] = 0;
        ofl->nObjFace[i] = 0;
    }
    for (int i = 0; i < ofl->nObj; i++) {
        int o = ofl->obj[i];
        int n = c->objFaceBase[o + 1] - c->objFaceBase[o];
        ofl->objFaceStart[i] = nOut;
        ofl->nObjFace[i] = 0;
        for (int f = 0; f < n; f++) {
            if (cdtBitTest(c->faceMark, c->objFaceBase[o] + f)) {
                if (ofl->nFace >= ofl->maxFace)
                    return;
                ofl->nObjFace[i]++;
                ofl->nFace++;
                ofl->face[nOut++] = (u16)f;
            }
        }
    }
}

// feb 0x004F09B0 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
