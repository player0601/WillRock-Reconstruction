// obj_split.cpp -- engine.lib face splits: grouping an object's faces by material (objSPLIT entries)
// (Will Rock 1.2, v12.exe; feb obj engine:obj_split.obj)
//
// v12 0x004FFD00..0x005010BF. A face's first u16 names its split entry. After loading, faces are sorted by
// material (_SortFaceList) and runs of faces with equal materials share one entry (firstFace, nFace), which
// is what the renderer draws in one batch. The "callgraph" functions are the beta code with the 1.2
// clamping accessors (objSPLIT::GetEntry etc., see object.h). RestoreFaceSplitNmb has no listed 1.2 twin;
// it is at v12 0x00500740 (found inside the Duplicate range).
// Not rebuilt yet: objSPLIT_ENTRY::CalcTexDensity (renderer mip selection).

#include "engine.h"

// The constructor as inlined into Make / Duplicate / _CreateFaceSplitPerFace / _CompressFaceSplit.
static objSPLIT *splitNew()
{
    objSPLIT *s = (objSPLIT *)operator new(sizeof(objSPLIT));
    if (!s)
        return 0;
    s->n = 0;
    s->entry = 0;
    new (&s->ctrl) objMTL_CTRL_TBL;
    return s;
}

// The entry-array growth as inlined (Alloc's body): new entries zeroed, splitNmb = their index.
static BOOL32 splitGrow(objSPLIT *s, int n)
{
    s->entry = (objSPLIT_ENTRY *)apRealloc(s->entry, n * sizeof(objSPLIT_ENTRY));
    if (!s->entry)
        return 0;
    for (int i = s->n; i < n; i++) {
        memset(&s->entry[i], 0, sizeof(objSPLIT_ENTRY));
        s->entry[i].splitNmb = (u16)i;
    }
    s->n = (u16)n;
    return 1;
}

// v12 0x004FFD00  (feb 0x004EDDC0)
objSPLIT *__fastcall objSPLIT::Make()
{
    objSPLIT *s = (objSPLIT *)operator new(sizeof(objSPLIT));
    if (!s)
        return 0;
    s->n = 0;
    s->entry = 0;
    for (int i = 0; i < 4; i++) {
        s->ctrl.c[i].tcInd = 0;
        s->ctrl.c[i].tcCoord = 0;
        s->ctrl.c[i].rendMtl = 0;
        s->ctrl.c[i].tcHWInd = 0;
    }
    s->ctrl.n = 0;
    return s;
}

// v12 0x004FFD40  (feb 0x004EDE00)
// One entry per face (copying the face's current materials), face i -> entry i; the controls are kept.
BOOL32 objOBJ::_CreateFaceSplitPerFace()
{
    if (!nFace)
        return 1;
    objSPLIT *s = splitNew();
    if (!s)
        return 0;
    if (!splitGrow(s, nFace))
        return 0;
    for (int i = 0; i < nFace; i++) {
        s->GetEntry(i)->firstFace = (u16)i;
        s->GetEntry(i)->nFace = 1;
        if (geom->split) {
            objMTL_TBL *src = geom->split->GetEntry(GetFace(i)->split);
            memcpy(s->GetEntry(i), src, sizeof(objMTL_TBL));
        }
        GetFace(i)->split = (u16)i;
    }
    if (geom->split)
        memcpy(&s->ctrl, &geom->split->ctrl, sizeof(objMTL_CTRL_TBL));
    objSPLIT *old = geom->split;
    if (old) {
        apFree(old->entry);
        operator delete(old);
    }
    geom->split = s;
    return 1;
}

// v12 0x004FFF20  (feb 0x004EDF80)
// Hardware texture stages for the controls: a control shares the stage of an earlier one with the same
// coordinate set unless one of its layers scales the coordinates; finally the lightmap pass (rendMtl 3) is
// moved before the detail pass (1) when it came later.
void objOBJ::_SetDefaultTCHWIndID()
{
    if (!nFace)
        return;
    int next = 0;
    objMTL_CTRL *c3 = 0, *c1 = 0;
    objSPLIT *s = geom->split;
    for (int i = 0; i < s->ctrl.n; i++) {
        objMTL_CTRL *c = s->ctrl.Get(i);
        int same = -1;
        for (int k = 0; k < i; k++)
            if (c->tcCoord == s->ctrl.Get(k)->tcCoord)
                same = k;
        BOOL32 scaled = 0;
        for (int e = 0; e < s->n; e++) {
            objSPLIT_ENTRY *en = s->GetEntry(e);
            for (int m = 0; m < en->nMtl; m++) {
                objMTL *mt = en->Get(m);
                if (i == mt->ctrl &&
                    (!(fabsf(mt->uScale - 1.0f) < 1e-6f) || !(fabsf(mt->vScale - 1.0f) < 1e-6f)))
                    scaled = 1;
            }
        }
        if (same == -1 || scaled)
            c->tcHWInd = (u8)next++;
        else
            c->tcHWInd = s->ctrl.Get(same)->tcHWInd;
        if (c->rendMtl == 3)
            c3 = c;
        if (c->rendMtl == 1)
            c1 = c;
        s = geom->split;
    }
    if (c3 && c1 && c3->tcHWInd < c1->tcHWInd) {
        u8 a = c3->tcHWInd, b = c1->tcHWInd;
        objMTL_CTRL_TBL *t = &geom->split->ctrl;
        for (int i = 0; i < t->n; i++) {
            u8 *h = &t->Get(i)->tcHWInd;
            if (*h == a)
                *h = b;
            else if (*h == b)
                *h = a;
        }
    }
}

// v12 0x005001F0  (feb 0x004EE1A0)
// Runs of consecutive faces with equal materials (CompMtlTbl) become one entry each; faces renumbered.
// No faces: the split is removed.
BOOL32 objOBJ::_CompressFaceSplit()
{
    if (!geom)
        return 1;
    objSPLIT *ns = 0;
    int *heap0 = 0, *heap1 = 0;
    if (nFace > 0) {
        int first0[20], count0[20];
        int *first = first0, *count = count0;
        int cap = 20, n = 0;
        for (int i = 0; i < nFace; i++) {
            if (i == 0) {
                first[n] = 0;
                count[n] = 0;
                n++;
            } else {
                objMTL_TBL *a = geom->split->GetEntry(GetFace(i - 1)->split);
                objMTL_TBL *b = geom->split->GetEntry(GetFace(i)->split);
                if (CompMtlTbl(a, b)) {
                    if (n >= cap) {
                        heap0 = (int *)apRealloc(heap0, cap * 8);
                        if (!heap0)
                            return 0;
                        heap1 = (int *)apRealloc(heap1, cap * 8);
                        if (!heap1)
                            return 0;
                        if (cap == 20) {
                            memcpy(heap0, first0, sizeof(first0));
                            memcpy(heap1, count0, sizeof(count0));
                        }
                        first = heap0;
                        count = heap1;
                        cap *= 2;
                    }
                    first[n] = i;
                    count[n] = 0;
                    n++;
                }
            }
            count[n - 1]++;
        }
        ns = splitNew();
        if (!splitGrow(ns, n))
            return 0;
        for (int e = 0; e < n; e++) {
            ns->GetEntry(e)->firstFace = (u16)first[e];
            ns->GetEntry(e)->nFace = (u16)count[e];
            objMTL_TBL *src = geom->split->GetEntry(GetFace(first[e])->split);
            memcpy(ns->GetEntry(e), src, sizeof(objMTL_TBL));
        }
        for (int e = 0; e < ns->n; e++)
            for (int j = 0; j < ns->GetEntry(e)->nFace; j++)
                GetFace(ns->GetEntry(e)->firstFace + j)->split = (u16)e;
        if (geom->split)
            memcpy(&ns->ctrl, &geom->split->ctrl, sizeof(objMTL_CTRL_TBL));
    }
    objSPLIT *old = geom->split;
    if (old) {
        apFree(old->entry);
        operator delete(old);
    }
    geom->split = ns;
    _FreeSplitUnshared();
    apFree(heap0);
    apFree(heap1);
    return 1;
}

// v12 0x005005A0  (feb 0x004EE520)
BOOL32 objSPLIT::Alloc(int cnt)
{
    return splitGrow(this, cnt);
}

// v12 0x00500640  (feb 0x004EE5C0)
objSPLIT *objSPLIT::Duplicate()
{
    objSPLIT *d = Make();
    if (!d)
        return 0;
    if (!splitGrow(d, n))
        return 0;
    memcpy(d->entry, entry, n * sizeof(objSPLIT_ENTRY));
    memcpy(&d->ctrl, &ctrl, sizeof(objMTL_CTRL_TBL));
    return d;
}

// v12 0x00500740  (feb 0x004EE6C0)
// Faces' split numbers from the entries' face ranges.
void objSPLIT::RestoreFaceSplitNmb(objOBJ *o)
{
    for (int e = 0; e < n; e++)
        for (int j = 0; j < GetEntry(e)->nFace; j++)
            o->GetFace(GetEntry(e)->firstFace + j)->split = (u16)e;
}

// [not rebuilt yet] original v12 0x005007D0 (feb 0x004EE720): texture density (texels per world unit) of
// an entry's layer for render pass rendMtl
BOOL32 objSPLIT_ENTRY::CalcTexDensity(objOBJ *o, int rendMtl, float *du, float *dv)
{
    WR_NOT_REBUILT(0x005007D0);
    return 0;
}

// v12 0x00500FF0  (feb 0x004EEF20)
BOOL32 objSPLIT_UNSHARED::Alloc(int cnt)
{
    struct E { void *p; int id; };
    E *l = (E *)operator new(cnt * sizeof(E));
    if (l)
        for (int i = 0; i < cnt; i++) {
            l[i].p = 0;
            l[i].id = -1;
        }
    list = (decltype(list))l;
    if (!l)
        return 0;
    n = cnt;
    return 1;
}

// v12 0x00501050  (feb 0x004EEF80)
objMTL_CTRL_TBL::objMTL_CTRL_TBL()
{
    for (int i = 0; i < 4; i++) {
        c[i].tcInd = 0;
        c[i].tcCoord = 0;
        c[i].rendMtl = 0;
        c[i].tcHWInd = 0;
    }
    n = 0;
}

// v12 0x00501080  (feb 0x004EEFB0)
m3dV *objOBJ::GetFaceVertPtr(int face, int v)
{
    return &geom->vert[GetFace(face)->vert[v]];
}
