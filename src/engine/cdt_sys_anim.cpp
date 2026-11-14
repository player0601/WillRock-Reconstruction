// cdt_sys_anim.cpp -- engine.lib grid of a scene's animated instances (scnSCENE::collAnim)
// (Will Rock 1.2, v12.exe; feb obj engine:cdt_sys_anim.obj)
//
// v12 0x004FE320..0x004FEAEF, all "exact" matches. Unlike the static grids this one is dynamic: each
// cell is a linked list of nodes (taken from a node cache) naming the ids whose box overlaps the cell.
// Moving instances call Invalidate(id, 0); the next query re-places the pending ids (Validate).

#include "engine.h"

// ---------------------------------------------------------------------------------------------
// lstNODE_CACHE<cdtCOLL_ANIM_NODE> (template, emitted in Scene.obj: v12 0x004E7550..0x004E7631)
// ---------------------------------------------------------------------------------------------

// v12 0x004E7580
template <> BOOL32 lstNODE_CACHE<cdtCOLL_ANIM_NODE>::Init(int count)
{
    int *blk = (int *)operator new(count * sizeof(cdtCOLL_ANIM_NODE) + 4);
    cdtCOLL_ANIM_NODE *a = 0;
    if (blk) {
        *blk = count;
        a = (cdtCOLL_ANIM_NODE *)(blk + 1);
        for (int i = 0; i < count; i++) {
            a[i].prev = 0;
            a[i].next = 0;
        }
    }
    nodes = a;
    if (!a)
        return 0;
    n = count;
    for (int i = 0; i < n; i++)
        free.Add(&nodes[i]);
    return 1;
}

// v12 0x004E7610
template <> void lstNODE_CACHE<cdtCOLL_ANIM_NODE>::Term()
{
    if (nodes)
        operator delete((int *)nodes - 1);
    nodes = 0;
    n = 0;
}

// v12 0x004E7550
template <> void *lstNODE_CACHE<cdtCOLL_ANIM_NODE>::__vecDelDtor(unsigned flags)
{
    if (flags & 1)
        operator delete(this);
    return this;
}

// ---------------------------------------------------------------------------------------------
// cdtCOLL_ANIM
// ---------------------------------------------------------------------------------------------
static inline int animCell(cdtCOLL_ANIM *c, int ix, int iy, int iz) { return (c->grid.ny * ix + iy) * c->grid.nz + iz; }

// v12 0x004FE320  (feb 0x004EBD40)
void cdtCOLL_ANIM::Destroy()
{
    if (cells)
        wrDelete(cells, 3);
    cells = 0;
    cache.Term();
    operator delete(pending);
    pending = 0;
    operator delete(mark);
    mark = 0;
    operator delete(range);
    range = 0;
    ((cdtCOLL_ANIM_SCN *)this)->scene = 0;          // +0x70 (the subclass field, cleared by the base dtor)
    ((m3dBOX_DATA *)((char *)&grid + 0x0C))->vtbl = (void *)m3dVOL_vftable;
}

// v12 0x004E7640  (Scene.obj)  scalar deleting destructor
void *cdtCOLL_ANIM::__vecDelDtor(unsigned flags)
{
    Destroy();
    if (flags & 1)
        operator delete(this);
    return this;
}

// v12 0x004FE390  (feb 0x004EBDB0)
// Grid over box with voxel sizes vx/vy/vz (cell counts capped at maxX/maxY/maxZ, at least 1).
BOOL32 cdtCOLL_ANIM::InitData(float vx, float vy, float vz, int maxX, int maxY, int maxZ, m3dBOX *box)
{
    voxGRID_EXT *g = (voxGRID_EXT *)&grid;
    m3dBOX *gb = g->Box();
    ((m3dBOX_DATA *)gb)->type = ((m3dBOX_DATA *)box)->type;
    gb->vMin = box->vMin;
    gb->vMax = box->vMax;
    int n = voxCalcNVoxel(gb->vMax.x - gb->vMin.x, vx);
    if (maxX < n) n = maxX;
    if (n < 1) n = 1;
    grid.nx = n;
    n = voxCalcNVoxel(gb->vMax.y - gb->vMin.y, vy);
    if (maxY < n) n = maxY;
    if (n < 1) n = 1;
    grid.ny = n;
    n = voxCalcNVoxel(gb->vMax.z - gb->vMin.z, vz);
    if (maxZ < n) n = maxZ;
    if (n < 1) n = 1;
    grid.nz = n;
    g->CalcVoxSize();
    int nVox = g->GetNVoxels();
    int nId = GetNID();
    cells = wrArrayNew<lstLIST_SIMPLE>(nVox);
    if (!cells)
        return 0;
    if (!cache.Init(nId * 8 + grid.nz * grid.nx * 6))
        return 0;
    nPending = 0;
    pending = (int *)operator new(nId * 4);
    if (!pending)
        return 0;
    mark = (u8 *)operator new(nId);
    if (!mark)
        return 0;
    range = (int (*)[6])operator new(nId * 0x18);
    if (!range)
        return 0;
    memset(range, 0xFF, nId * 0x18);
    return 1;
}

// v12 0x004FE5C0  (feb 0x004EBFE0)
// remove = 0: queue id for re-placement (once); remove != 0: take it out of the grid now.
void cdtCOLL_ANIM::Invalidate(int id, int remove)
{
    for (int i = 0; i < nPending; i++) {
        if (pending[i] == id) {
            if (!remove)
                return;
            pending[i] = -1;
            RemoveFromGrid(id);
            return;
        }
    }
    if (!remove) {
        pending[nPending++] = id;
        return;
    }
    RemoveFromGrid(id);
}

// v12 0x004FE620  (feb 0x004EC040)
// Re-place every pending id whose cell range changed.
void cdtCOLL_ANIM::Validate()
{
    if (!nPending)
        return;
    struct { const void *vtbl; int type; m3dV vMin, vMax; } box = { m3dBOX_vftable, M3D_VOL_BOX };
    voxGRID_EXT *g = (voxGRID_EXT *)&grid;
    for (int p = 0; p < nPending; p++) {
        int id = pending[p];
        if (id == -1)
            continue;
        if (!GetIDBBox(id, (m3dBOX *)&box)) {
            RemoveFromGrid(pending[p]);
            continue;
        }
        int r[6];
        g->GetPosInd(&box.vMin, &r[0], &r[1], &r[2], 1);
        g->GetPosInd(&box.vMax, &r[3], &r[4], &r[5], 1);
        id = pending[p];
        int *old = range[id];
        if (r[0] == old[0] && r[1] == old[1] && r[2] == old[2] &&
            r[3] == old[3] && r[4] == old[4] && r[5] == old[5])
            continue;
        RemoveFromGrid(id);
        for (int ix = r[0]; ix <= r[3]; ix++)
            for (int iy = r[1]; iy <= r[4]; iy++)
                for (int iz = r[2]; iz <= r[5]; iz++) {
                    cdtCOLL_ANIM_NODE *n = (cdtCOLL_ANIM_NODE *)cache.free.head;
                    cache.free.Remove(n);
                    if (n) {
                        n->id = pending[p];
                        cells[animCell(this, ix, iy, iz)].Add(n);
                    }
                }
        memcpy(old, r, sizeof(r));
    }
    nPending = 0;
}

// v12 0x004FE810  (feb 0x004EC230)
void cdtCOLL_ANIM::RemoveFromGrid(int id)
{
    int *r = range[id];
    if (r[0] < 0)
        return;
    for (int ix = r[0]; ix <= r[3]; ix++)
        for (int iy = r[1]; iy <= r[4]; iy++)
            for (int iz = r[2]; iz <= r[5]; iz++) {
                lstLIST_SIMPLE *c = &cells[animCell(this, ix, iy, iz)];
                for (lstNODE *n = c->head; n; n = n->next) {
                    if (((cdtCOLL_ANIM_NODE *)n)->id == id) {
                        c->Remove(n);
                        cache.free.Add(n);
                        break;
                    }
                }
            }
    memset(r, 0xFF, sizeof(int) * 6);
}

// v12 0x004FE8D0  (feb 0x004EC2F0)
// Every id in the cells box b covers (each once). The 'mode' argument is unused.
void cdtCOLL_ANIM::MakeIndList(m3dBOX *b, cdtOFL *ofl, int mode)
{
    Validate();
    memset(mark, 0, GetNID());
    voxGRID_EXT *g = (voxGRID_EXT *)&grid;
    int x0, y0, z0, x1, y1, z1;
    g->GetPosInd(&b->vMin, &x0, &y0, &z0, 1);
    g->GetPosInd(&b->vMax, &x1, &y1, &z1, 1);
    ofl->nObj = 0;
    for (int ix = x0; ix <= x1; ix++)
        for (int iy = y0; iy <= y1; iy++)
            for (int iz = z0; iz <= z1; iz++)
                for (lstNODE *n = cells[animCell(this, ix, iy, iz)].head; n; n = n->next) {
                    int id = ((cdtCOLL_ANIM_NODE *)n)->id;
                    if (!mark[id]) {
                        ofl->obj[ofl->nObj++] = (u16)id;
                        mark[id] = 1;
                    }
                }
}

// v12 0x004FEA30  (feb 0x004EC450)
// Every id in the cells the segment a->b crosses (at most 2000 cells).
void cdtCOLL_ANIM::MakeIndListRay(m3dV *a, m3dV *b, cdtOFL *ofl, int mode)
{
    int list[2000];
    int nCells;
    Validate();
    memset(mark, 0, GetNID());
    ((voxGRID_EXT *)&grid)->MakeIndListRay(a, b, list, &nCells, 2000);
    ofl->nObj = 0;
    for (int i = 0; i < nCells; i++)
        for (lstNODE *n = cells[list[i]].head; n; n = n->next) {
            int id = ((cdtCOLL_ANIM_NODE *)n)->id;
            if (!mark[id]) {
                ofl->obj[ofl->nObj++] = (u16)id;
                mark[id] = 1;
            }
        }
}

// feb 0x004EC430 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
