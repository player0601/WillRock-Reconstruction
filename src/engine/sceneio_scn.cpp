// sceneio_scn.cpp -- engine.lib level readers: instance list, template list, lights, collision grid
// (Will Rock 1.2, v12.exe; feb obj engine:sceneio_scn.obj)
//
// v12 0x0059F1C0..0x0059FB9F. The instance and template lists are "exact"; the light list and both
// collision readers were rebuilt from the 1.2 code (the collision grid has the 1.2 cdtCOLL layout: size
// 0x70, the per-face lists at +0x54/+0x58/+0x5C, data at +0x6C).
//   0x1B8 instances: u32 n, n x 0x1B9 { 'SNIA', name, template, class, (unused string), matrix,
//                    [0x1BA script], [0x1CC u32 (read and dropped)] }, end chunk
//   0x1EA templates: u32 n, n templates (sioReadTemplate without textures), end chunk
//   0x280 lights:    u32 n, n x 0x281 { u32 type, u32 flags, sub-chunks 0x282..0x288, 0x348 } (see scnLIGHT)
//   0x21F grid:      0x220 { n[3], bbMin, bbMax, voxSize } 0x221 { u32 nObj, nEntry, cellStart, cellObj }
//                    0x222 { u32 nFace, nEntFace, entFaceStart, entFace, objFaceBase } (per-face lists)
//   0x21C old grid:  flags (bit 0 set = unsupported), grid, counts, per-cell counts, cellStart, cellObj

#include "engine.h"

// v12 0x0059F1C0  (feb 0x00582E90)
BOOL32 __fastcall _sioReadInstList(scnSCENE *s)
{
    char buf[0x40];
    int n;
    if (sioFilePtr->ReadChunk(0) != 0x1B8) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    sioFilePtr->ReadData(&n, 4, 4);
    for (int i = 0; i < n; i++) {
        if (sioFilePtr->ReadChunk(0) != 0x1B9)
            return 0;
        animINST *a = s->AllocInst();
        if (!a)
            return 0;
        sioFilePtr->ReadData(a, 4, 4);                  // magic
        if (a->magic != 0x41494E53)                     // 'SNIA'
            return 0;
        sioFilePtr->Read(buf, 0x40);
        a->SetName(buf);
        sioFilePtr->Read(buf, 0x40);
        a->SetNameTpl(buf);
        sioFilePtr->Read(buf, 0x40);
        a->SetNameClass(buf);
        sioFilePtr->Read(buf, 0x40);                    // a fourth name, not kept
        sioFilePtr->ReadData(a->matr, 0x40, 4);
        if (!_sioReadPSheet(&a->props))
            apLogErr((char *)"Cannot parse script for instance %s", a->name ? a->name : (char *)"");
        a->flags |= 0x800;
        if (sioFilePtr->ReadChunk(0) == 0x1CC)
            sioFilePtr->ReadData(buf, 4, 4);            // read into the name buffer and dropped
        else
            sioFilePtr->RevertChunk(0);
    }
    return _sioReadLastChunk() != 0;
}

// v12 0x0059F350  (feb 0x00583020)
BOOL32 __fastcall _sioReadTplList(scnSCENE *s)
{
    int n;
    if (sioFilePtr->ReadChunk(0) != 0x1EA) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    sioFilePtr->ReadData(&n, 4, 4);
    for (int i = 0; i < n; i++) {
        animTPL *t = s->AllocTpl();
        if (!t)
            return 0;
        if (!sioReadTemplate(t, 0))
            return 0;
        t->flags |= 0x840;
    }
    return _sioReadLastChunk() != 0;
}

// v12 0x0059F3E0  (feb 0x005830B0)
BOOL32 __fastcall _sioReadLightList(scnSCENE *s)
{
    fioCHUNK c;
    int n;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    if (sioFilePtr->ReadChunk(0) != 0x280) {
        sioFilePtr->RevertChunk(0);
        return 1;
    }
    sioFilePtr->ReadData(&n, 4, 4);
    s->lights = (scnLIGHT *)apCalloc(sizeof(scnLIGHT), n);
    if (!s->lights)
        return 0;
    for (int i = 0; i < n; i++) {
        if (sioFilePtr->ReadChunk(0) != 0x281)
            return 0;
        scnLIGHT *l = &s->lights[s->nLights++];
        u32 fl;
        sioFilePtr->ReadData(&l->type, 4, 4);
        sioFilePtr->ReadData(&fl, 4, 4);
        l->flags = fl;
        int id = sioFilePtr->ReadChunk(&c);
        while (id && c.id != 1) {
            switch (c.id) {
            case 0x282:
                sioFilePtr->Read(l->name, 0x40);
                break;
            case 0x283:
                sioFilePtr->ReadData(&l->matr, 0x40, 4);
                break;
            case 0x284:
                sioFilePtr->ReadData(&l->color[0], 4, 4);
                sioFilePtr->ReadData(&l->color[1], 4, 4);
                sioFilePtr->ReadData(&l->color[2], 4, 4);
                break;
            case 0x285:
                sioFilePtr->ReadData(&l->_98, 4, 4);
                sioFilePtr->ReadData(&l->_9c, 4, 4);
                break;
            case 0x286:
                sioFilePtr->ReadData(&l->_a0, 4, 4);
                sioFilePtr->ReadData(&l->_a4, 4, 4);
                break;
            case 0x287:
                sioFilePtr->ReadData(&l->_a8, 4, 4);
                break;
            case 0x288:
                if (!_sioReadPSheet(&l->sheet))
                    apLogErr((char *)"Cannot parse script for light %s", l->name);
                break;
            case 0x348:
                sioFilePtr->RevertChunk(0);
                if (!_sioReadNamedSel(&l->sel))
                    return 0;
                break;
            default:
                sioFilePtr->SkipChunk(&c);
                break;
            }
            id = sioFilePtr->ReadChunk(&c);
        }
    }
    return _sioReadLastChunk() != 0;
}

// A new level collision grid: cdtCOLL_OBJ constructor then the cdtCOLL_SCN vtable (inlined in both readers).
static cdtCOLL_SCN *sioNewCollScn()
{
    void *m = operator new(sizeof(cdtCOLL_SCN));
    if (!m)
        return 0;
    return new (m) cdtCOLL_SCN;
}

// v12 0x0059F790  (feb 0x00583460)
cdtCOLL_SCN *__fastcall _sioReadColl()
{
    fioCHUNK c;
    int nObj, nEntry, nFace, nEntFace;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    if (sioFilePtr->ReadChunk(0) != 0x21F) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    cdtCOLL_SCN *g = sioNewCollScn();
    if (!g)
        return 0;
    voxGRID_EXT *grid = (voxGRID_EXT *)&g->grid;
    while (sioFilePtr->ReadChunk(&c) && c.id != 1) {
        switch (c.id) {
        case 0x220:
            sioFilePtr->ReadData(&g->grid.nx, 12, 4);
            sioFilePtr->ReadData(&g->grid.bbMin, 12, 4);
            sioFilePtr->ReadData(&g->grid.bbMax, 12, 4);
            sioFilePtr->ReadData(&g->grid.voxSize, 12, 4);
            break;
        case 0x221:
            sioFilePtr->ReadData(&nObj, 4, 4);
            sioFilePtr->ReadData(&nEntry, 4, 4);
            if (!g->AllocLists(grid->GetNVoxels(), nObj, nEntry, 0, 0))
                return 0;
            sioFilePtr->ReadData(g->cellStart, (grid->GetNVoxels() + 1) * 4, 4);
            sioFilePtr->ReadData(g->cellObj, nEntry * 2, 2);
            break;
        case 0x222:
            g->flags |= 1;
            sioFilePtr->ReadData(&nFace, 4, 4);
            sioFilePtr->ReadData(&nEntFace, 4, 4);
            if (!g->AllocLists(grid->GetNVoxels(), nObj, nEntry, nFace, nEntFace))
                return 0;
            sioFilePtr->ReadData(g->entFaceStart, nEntry * 4 + 4, 4);
            sioFilePtr->ReadData(g->entFace, nEntFace * 2, 2);
            sioFilePtr->ReadData(g->objFaceBase, g->nObj * 4 + 4, 4);
            break;
        default:
            sioFilePtr->SkipChunk(&c);
            break;
        }
    }
    return g;
}

// v12 0x0059F9D0  (feb 0x005836A0)
// The old grid format: only cell/object lists. The per-cell counts are read but only the last one is used
// (to close cellStart).
cdtCOLL_SCN *__fastcall _sioReadColl_OLD()
{
    u32 fl;
    int nEntry, nObj;
    u16 cnt = 0;
    if (sioFilePtr->ReadChunk(0) != 0x21C) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    sioFilePtr->ReadData(&fl, 4, 4);
    if (fl & 1)
        return 0;
    cdtCOLL_SCN *g = sioNewCollScn();
    if (!g)
        return 0;
    g->flags = fl;
    voxGRID_EXT *grid = (voxGRID_EXT *)&g->grid;
    sioFilePtr->ReadData(&g->grid.nx, 12, 1);
    sioFilePtr->ReadData(&g->grid.bbMin, 12, 4);
    sioFilePtr->ReadData(&g->grid.bbMax, 12, 4);
    sioFilePtr->ReadData(&g->grid.voxSize, 12, 1);
    sioFilePtr->ReadData(&nEntry, 4, 4);
    sioFilePtr->ReadData(&nObj, 4, 4);
    if (!g->AllocLists(grid->GetNVoxels(), nObj, nEntry, 0, 0))
        return 0;
    for (int i = 0; i < grid->GetNVoxels(); i++)
        sioFilePtr->ReadData(&cnt, 2, 2);
    sioFilePtr->ReadData(g->cellStart, grid->GetNVoxels() * 4, 4);
    int last = g->cellStart[grid->GetNVoxels() - 1];
    g->cellStart[grid->GetNVoxels()] = last + cnt;
    sioFilePtr->ReadData(g->cellObj, nEntry * 2, 2);
    return _sioReadLastChunk() ? g : 0;
}
