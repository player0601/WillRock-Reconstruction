// sceneio_misc.cpp -- engine.lib scene readers shared by levels and templates: end chunk, texture list,
// script (property sheet), named selections, camera
// (Will Rock 1.2, v12.exe; feb obj engine:sceneio_misc.obj)
//
// v12 0x0059EA80..0x0059F1BF, all "exact" except _sioReadCamera (only call targets moved).
// Texture list (chunk 0x154 old / 0x155 new): u32 count, then per texture its name (0x154), or
// 0x156 { name } and optionally 0x157 { u32 nLayers, nLayers x 0x158 { layer name } } (a layered texture).
// sioTexListNmb maps the file's texture index to the texture's listNmb (txmTEXTURE +0x6C); every texture
// read here gets flag 0x200. Kept: _sioReadCamera leaks the camera when a spline fails to read.

#include "engine.h"

// v12 0x0059EA80  (feb 0x00582750)
BOOL32 __fastcall _sioReadLastChunk()
{
    if (sioFilePtr->ReadChunk(0) != 1) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    return 1;
}

// v12 0x0059EAB0  (feb 0x00582780)
BOOL32 __fastcall _sioReadTex()
{
    fioCHUNK c;
    char name[0x40];
    int n;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    c.id = 0;
    sioFilePtr->ReadChunk(&c);
    if (c.id != 0x154 && c.id != 0x155) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    sioFilePtr->ReadData(&n, 4, 4);
    sioTexListNmb = (int *)apRealloc(sioTexListNmb, n * 4);
    if (!sioTexListNmb)
        return 0;
    memset(sioTexListNmb, 0, n * 4);
    if (c.id == 0x155) {
        for (int i = 0; i < n; i++) {
            if (sioFilePtr->ReadChunk(0) != 0x156)
                return 0;
            sioFilePtr->Read(name, 0x40);
            txmTEXTURE *t;
            if (sioFilePtr->ReadChunk(0) == 0x157) {
                t = txmManager->Add(name, 0x100, (txmADD_MODE)0);    // a layered texture (mode 0)
                if (!t)
                    return 0;
                t->flags |= 0x200;
                int nLay;
                sioFilePtr->ReadData(&nLay, 4, 4);
                for (int k = 0; k < nLay; k++) {
                    if (sioFilePtr->ReadChunk(0) != 0x158)
                        return 0;
                    sioFilePtr->Read(name, 0x40);
                    if (((txmTEXTURE_LAY *)t)->Add(name) == -1)
                        return 0;
                }
            } else {
                t = txmManager->Add(name, 0x100, (txmADD_MODE)-1);
                if (!t)
                    return 0;
                t->flags |= 0x200;
                sioFilePtr->RevertChunk(0);
            }
            sioTexListNmb[i] = t->listNmb;
        }
    } else {
        for (int i = 0; i < n; i++) {
            sioFilePtr->Read(name, 0x40);
            txmTEXTURE *t = txmManager->Add(name, 0x100, (txmADD_MODE)-1);
            if (!t)
                return 0;
            sioTexListNmb[i] = t->listNmb;
            t->flags |= 0x200;
        }
    }
    if (sioFilePtr->ReadChunk(0) != 1) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    return 1;
}

// v12 0x0059ED10  (feb 0x005829E0)
// Optional chunk 0x1BA: the script text (max 0x1000 characters, read into apBuffer), parsed into a new
// psSHEET replacing *out. Returns 0 only when the script does not parse (or memory runs out).
BOOL32 __fastcall _sioReadPSheet(psSHEET **out)
{
    if (sioFilePtr->ReadChunk(0) != 0x1BA) {
        sioFilePtr->RevertChunk(0);
        return 1;
    }
    if (*out) {
        (*out)->Empty();
        operator delete(*out);
    }
    sioFilePtr->Read(apBuffer, 0x1000);
    psSHEET *ps = (psSHEET *)operator new(sizeof(psSHEET));
    if (ps)
        memset(ps, 0, sizeof(psSHEET));
    *out = ps;
    if (!ps)
        return 0;
    if (!ps->SetString(apBuffer))
        return 0;
    if (ps->ParseString()) {
        apLogErr((char *)"Cannot parse script");
        return 0;
    }
    return 1;
}

// v12 0x0059EDD0  (feb 0x00582AA0)
// 0x348 { u32 flags, name } 0x349 { u32 n } then n x 0x34A { u32 id, name }; sorted for Find.
BOOL32 __fastcall _sioReadNamedSel(selSEL *sel)
{
    u32 v;
    int n;
    char name[0x40];
    if (sioFilePtr->ReadChunk(0) != 0x348) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    sioFilePtr->ReadData(&v, 4, 4);
    sioFilePtr->Read(name, 0x40);
    if (sioFilePtr->ReadChunk(0) != 0x349)
        return 0;
    sioFilePtr->ReadData(&n, 4, 4);
    if (!sel->Init(n))
        return 0;
    ((apNAME *)sel->name)->SetName(name);
    sel->flags = v;
    for (int i = 0; i < sel->n; i++) {
        if (sioFilePtr->ReadChunk(0) != 0x34A)
            return 0;
        sioFilePtr->ReadData(&v, 4, 4);
        sioFilePtr->Read(name, 0x40);
        ((apNAME *)sel->obj[i].name)->SetName(name);
        sel->obj[i].id = v;
    }
    sel->Prepare();
    return 1;
}

// v12 0x0059EF00  (feb 0x00582BD0)
// Optional chunk 0x21D: u32 count, then the selections (new selSEL[count], count cookie).
BOOL32 __fastcall _sioReadNamedSelList(int *n, selSEL **list)
{
    if (sioFilePtr->ReadChunk(0) != 0x21D) {
        sioFilePtr->RevertChunk(0);
        return 1;
    }
    sioFilePtr->ReadData(n, 4, 4);
    int cnt = *n;
    int *blk = (int *)operator new(cnt * sizeof(selSEL) + 4);
    selSEL *a = 0;
    if (blk) {
        *blk = cnt;
        a = (selSEL *)(blk + 1);
        for (int i = 0; i < cnt; i++) {
            a[i].flags = 0;
            a[i].name[0] = 0;
            a[i].n = 0;
            a[i].obj = 0;
        }
    }
    *list = a;
    if (!a)
        return 0;
    for (int i = 0; i < *n; i++)
        if (!_sioReadNamedSel(&(*list)[i]))
            return 0;
    return 1;
}

// v12 0x0059EFF0  (feb 0x00582CC0)
// Chunk 0x3AC: 0x3AD camera-to-world matrix, 0x3AE vertical angle, 0x3AF..0x3B2 animation splines
// (vertical angle, ?, position, interest), up to the end chunk.
camCAMERA *__fastcall _sioReadCamera()
{
    m3dSPL *sAngle = 0, *s200 = 0, *sPos = 0, *sInterest = 0;
    m3dMATR c2w;
    float angle;
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    if (sioFilePtr->ReadChunk(0) != 0x3AC) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    camCAMERA *cam = (camCAMERA *)operator new(sizeof(camCAMERA));
    if (!cam)
        return 0;
    new (cam) camCAMERA;
    int id = sioFilePtr->ReadChunk(&c);
    for (;;) {
        if (!id || c.id == 1) {
            cam->TransformMatrC2W(&c2w, 0);
            cam->SetAngleVert(angle);
            cam->splInterest = sInterest;
            cam->splUnused = s200;
            cam->splPos = sPos;
            cam->splAngleVert = sAngle;
            return cam;
        }
        m3dSPL *got = (m3dSPL *)1;
        switch (c.id) {
        case 0x3AD:
            sioFilePtr->ReadData(&c2w, 0x40, 4);
            break;
        case 0x3AE:
            sioFilePtr->ReadData(&angle, 4, 4);
            break;
        case 0x3AF:
            got = sAngle = sioFilePtr->ReadSpline();
            break;
        case 0x3B0:
            got = s200 = sioFilePtr->ReadSpline();
            break;
        case 0x3B1:
            got = sPos = sioFilePtr->ReadSpline();
            break;
        case 0x3B2:
            got = sInterest = sioFilePtr->ReadSpline();
            break;
        default:
            sioFilePtr->SkipChunk(&c);
            break;
        }
        if (!got)
            return 0;
        id = sioFilePtr->ReadChunk(&c);
    }
}
