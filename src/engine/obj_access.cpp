// obj_access.cpp -- engine.lib material lookups: a face's / split entry's material for a render pass,
// texture coordinates, and the material control table
// (Will Rock 1.2, v12.exe; feb obj engine:obj_access.obj)
//
// v12 0x004EB0A0..0x004EB9BF. The "exact" functions are the beta code; the "callgraph" ones are the beta
// code read through the 1.2 clamping accessors (objSPLIT::GetEntry, objMTL_TBL::Get,
// objMTL_CTRL_TBL::Get: index clamped to [0, n]), rebuilt from the 1.2 code.

#include "engine.h"

static inline objSPLIT *objSplit(objOBJ *o)
{
    return o->geom->split;
}

// v12 0x004EB0A0  (feb 0x004ED720)
// Layer 'mtl' of the face's material (no check that it exists).
txmTEXTURE *objOBJ::GetFaceTexMtlNmb(int face, int mtl)
{
    objSPLIT_ENTRY *e = objSplit(this)->GetEntry(GetFace(face)->split);
    return e->Get(mtl)->tex;
}

// v12 0x004EB110  (feb 0x004ED760)
// The texture of the face's layer drawn in render pass 'rendMtl', or 0.
txmTEXTURE *objOBJ::GetFaceTexRendMtl(int face, int rendMtl)
{
    objSPLIT *s = objSplit(this);
    objSPLIT_ENTRY *e = s->GetEntry(GetFace(face)->split);
    for (int i = 0; i < e->nMtl; i++) {
        objMTL *m = e->Get(i);
        if (s->ctrl.Get(m->ctrl)->rendMtl == rendMtl)
            return m->tex;
    }
    return 0;
}

// v12 0x004EB1F0  (feb 0x004ED7E0)
// Texture coordinate of corner 'vert' of the face for layer 'mtl', scaled by the layer's scale. An index past
// the coordinate set reads coordinate 0.
void objOBJ::GetFaceTexCoord(int face, int vert, int mtl, m2dV *out)
{
    objSPLIT *s = objSplit(this);
    objFACE *f = GetFace(face);
    objSPLIT_ENTRY *e = s->GetEntry(f->split);
    objMTL *m = e->Get(mtl);
    objMTL_CTRL *c = s->ctrl.Get(m->ctrl);
    u32 idx = f->tc[c->tcInd * 3 + vert];
    int set = c->tcCoord;
    if (idx > geom->nTexCoord[set])
        idx = 0;
    m2dV *tc = (m2dV *)geom->texCoord[set];
    out->x = tc[idx].x;
    out->y = tc[idx].y;
    out->x = m->uScale * out->x;
    out->y = m->vScale * out->y;
}

// v12 0x004EB2D0  (feb 0x004ED880)
int objOBJ::GetTCIndIDRendMtl(int rendMtl)
{
    objMTL_CTRL_TBL *t = &objSplit(this)->ctrl;
    for (int i = 0; i < t->n; i++)
        if (t->c[i].rendMtl == rendMtl)
            return t->c[i].tcInd;
    return -1;
}

// v12 0x004EB330  (feb 0x004ED8E0)
int objOBJ::GetTCCoordIDRendMtl(int rendMtl)
{
    objMTL_CTRL_TBL *t = &objSplit(this)->ctrl;
    for (int i = 0; i < t->n; i++)
        if (t->c[i].rendMtl == rendMtl)
            return t->c[i].tcCoord;
    return -1;
}

// v12 0x004EB390  (feb 0x004ED940)
txmTEXTURE *objOBJ::GetSplitTexRendMtl(int split, int rendMtl)
{
    objSPLIT *s = objSplit(this);
    objSPLIT_ENTRY *e = s->GetEntry(split);
    for (int i = 0; i < e->nMtl; i++) {
        objMTL *m = e->Get(i);
        if (s->ctrl.Get(m->ctrl)->rendMtl == rendMtl)
            return m->tex;
    }
    return 0;
}

// v12 0x004EB460  (feb 0x004ED9B0)
objMTL *objOBJ::GetSplitMtlRendMtl(int split, int rendMtl)
{
    objSPLIT *s = objSplit(this);
    objSPLIT_ENTRY *e = s->GetEntry(split);
    for (int i = 0; i < e->nMtl; i++) {
        objMTL *m = e->Get(i);
        if (s->ctrl.Get(m->ctrl)->rendMtl == rendMtl)
            return m;
    }
    return 0;
}

// v12 0x004EB530  (feb 0x004EDA20)
objMTL *objOBJ::GetSplitMtlRendMtl(objSPLIT_ENTRY *e, int rendMtl)
{
    objMTL_CTRL_TBL *t = &objSplit(this)->ctrl;
    for (int i = 0; i < e->nMtl; i++) {
        objMTL *m = e->Get(i);
        if (t->Get(m->ctrl)->rendMtl == rendMtl)
            return m;
    }
    return 0;
}

// v12 0x004EB5E0  (feb 0x004EDA80)
// Number of hardware texture stages used: highest tcHWInd + 1.
int objOBJ::CalcMaxTCHWIndID()
{
    objMTL_CTRL_TBL *t = &objSplit(this)->ctrl;
    int mx = 0;
    for (int i = 0; i < t->n; i++)
        if (t->Get(i)->tcHWInd > mx)
            mx = t->Get(i)->tcHWInd;
    return mx + 1;
}

// v12 0x004EB640  (feb 0x004EDAB0)
// The lowest coordinate set (0..2) no control uses, -1 when all three are taken.
int objMTL_CTRL_TBL::FindFreeTCCoordID()
{
    for (int id = 0;; id++) {
        if (n < 1)
            return id;
        BOOL32 used = 0;
        for (int i = 0; i < n; i++)
            if (c[i].tcCoord == id)
                used = 1;
        if (!used)
            return id;
        if (id + 1 > 2)
            return -1;
    }
}

// v12 0x004EB680  (feb 0x004EDAF0)
BOOL32 objMTL_CTRL_TBL::IsSharedTCCoordID(int tcCoord)
{
    int k = 0;
    for (int i = 0; i < n; i++)
        if (c[i].tcCoord == tcCoord)
            k++;
    return k > 1;
}

// v12 0x004EB6C0  (feb 0x004EDB30)
// The control of render pass 'rendMtl'; a new one gets the lowest free index set and coordinate set
// (its hardware stage = its coordinate set). -1 when no set is free.
int objMTL_CTRL_TBL::AddMtlCtrl(int rendMtl)
{
    for (int i = 0; i < n; i++)
        if (c[i].rendMtl == rendMtl)
            return i;
    int ind = 0;
    for (;;) {
        BOOL32 used = 0;
        if (n < 1)
            break;
        for (int i = 0; i < n; i++)
            if (c[i].tcInd == ind)
                used = 1;
        if (!used)
            break;
        if (++ind > 2)
            return -1;
    }
    int crd = 0;
    for (;;) {
        BOOL32 used = 0;
        if (n < 1)
            break;
        for (int i = 0; i < n; i++)
            if (c[i].tcCoord == crd)
                used = 1;
        if (!used)
            break;
        if (++crd > 2)
            return -1;
    }
    c[n].rendMtl = (u8)rendMtl;
    c[n].tcInd = (u8)ind;
    c[n].tcCoord = (u8)crd;
    c[n].tcHWInd = (u8)crd;
    n++;
    return n - 1;
}

// v12 0x004EB7C0  (feb 0x004EDC30)
int objMTL_CTRL_TBL::FindMtlCtrlID(int rendMtl)
{
    for (int i = 0; i < n; i++)
        if (c[i].rendMtl == rendMtl)
            return i;
    return -1;
}

// v12 0x004EB7F0  (feb 0x004EDC60)
objMTL_CTRL *objMTL_CTRL_TBL::FindMtlCtrl(int rendMtl)
{
    for (int i = 0; i < n; i++)
        if (c[i].rendMtl == rendMtl)
            return &c[i];
    return 0;
}

// v12 0x004EB840  (feb 0x004EDCB0)
// Appends {t, scale 1, 1, 1, ctrl}. The original copies two uninitialised stack dwords into +0x10/+0x14;
// here they are zeroed.
int objMTL_TBL::AddMtl(int ctrl, txmTEXTURE *t)
{
    objMTL *m = &mtl[nMtl];
    m->tex = t;
    m->uScale = 1.0f;
    m->vScale = 1.0f;
    m->_0c = 1.0f;
    m->_10 = 0;
    m->_14 = 0;
    m->ctrl = (signed char)ctrl;
    nMtl++;
    return nMtl - 1;
}

// v12 0x004EB8C0  (feb 0x004EDD30)
// The first layer with a texture that uses coordinate set 'tcCoord'.
objMTL *objSPLIT_ENTRY::FindMtlTCCoordID(objOBJ *o, int tcCoord)
{
    objMTL_CTRL_TBL *t = &o->geom->split->ctrl;
    for (int i = 0; i < nMtl; i++) {
        objMTL *m = Get(i);
        if (m->tex && t->Get(m->ctrl)->tcCoord == tcCoord)
            return m;
    }
    return 0;
}

// v12 0x004EB940  (feb 0x004EDD80)
objMTL *objSPLIT_ENTRY::FindMtlRendMtl(objOBJ *o, int rendMtl)
{
    objMTL_CTRL_TBL *t = &o->geom->split->ctrl;
    for (int i = 0; i < nMtl; i++) {
        objMTL *m = Get(i);
        if (t->Get(m->ctrl)->rendMtl == rendMtl)
            return m;
    }
    return 0;
}
