// Obj_misc.cpp -- engine.lib objOBJ hierarchy walks: counting, ids, state bits, searching, pointer lists,
// face list sorting / compression, textures, colours and render states
// (Will Rock 1.2, v12.exe; feb obj engine:Obj_misc.obj)
//
// v12 0x004CB4A0..0x004CD2F7, the whole obj. The "callgraph" functions are the beta code with the 1.2
// clamping material accessors; 1.2 addresses with no listed twin were found by their callers
// (re/tools/vfuncs.py): _CompressFaceList 0x004CB5E0, _SortMtlList 0x004CB760, ReplaceTexList 0x004CBD60,
// objFind(fn) 0x004CC930, CompMtlTbl 0x004CCA70, _objCmpFace 0x004CCFA0, _objCmpSplitEntry 0x004CD050.
// objIsNFace / objIsNVert / PropagateStateProc have no 1.2 copy (unused).
//
// Every walk visits the children through the circular sibling ring starting at child.

#include "engine.h"

#define FOR_CHILDREN(o, c)                                                         \
    for (objOBJ *_first = (o) ? (o)->child : 0, *c = _first; c; c = c->next == _first ? 0 : c->next)

// v12 0x004CB4A0  (feb 0x004BB4F0)
int objOBJ::GetNObj(BOOL32 (__fastcall *filter)(objOBJ *, void *))
{
    int n = (!filter || filter(this, 0)) ? 1 : 0;
    FOR_CHILDREN(this, c)
        n += c->GetNObj(filter);
    return n;
}

// v12 0x004CB510  (feb 0x004BB560)
int objOBJ::GetMaxNVert()
{
    int n = nVert;
    FOR_CHILDREN(this, c) {
        int m = c->GetMaxNVert();
        if (n <= m)
            n = m;
    }
    return n;
}

// v12 0x004CB560
// The SetStateProc* filter "has faces".
BOOL32 __fastcall objIsWithFaces(objOBJ *o, void *)
{
    return o->nFace > 0;
}

// v12 0x004CB580  (feb 0x004BB5D0)
// Depth-first numbering from 'first'; returns the next free number.
int objOBJ::SetIDHier(int first)
{
    idHier = (short)first;
    int n = first + 1;
    FOR_CHILDREN(this, c)
        n = c->SetIDHier(n);
    return n;
}

// v12 0x004CB5D0  (feb 0x004BB620)
// Face record: u16 flags, 3 u16 vertex indices, then 3 u16 texture-coordinate indices per set.
int objOBJ::_CalcFaceSize(int nTexSets)
{
    return nTexSets * 6 + 8;
}

// v12 0x004CC140  (feb 0x004BC080)
void objOBJ::SetStateYes(int bits)
{
    state |= bits;
    FOR_CHILDREN(this, c)
        c->SetStateYes(bits);
}

// The render-state rule of SetStateRendYes / _SetStateRendYes: 0x80 and 0x40 exclude each other (the new
// one wins), any of the low 14 bits clears 0x20000000, 0x800 implies 2.
static inline void objRendYes(objOBJ *o, int bits)
{
    u32 r = o->rendState | bits;
    o->rendState = r;
    if ((r & 0x80) && (r & 0x40)) {
        if (bits & 0x80)
            o->rendState = r & ~0x40u;
        if (bits & 0x40)
            o->rendState &= ~0x80u;
    }
    if (bits & 0x3FFF)
        o->rendState &= 0xDFFFFFFF;
    if (o->rendState & 0x800)
        o->rendState |= 2;
}
static inline void objRendNo(objOBJ *o, int bits)
{
    u32 r = o->rendState;
    o->rendState = r & ~bits;
    if (bits & 0x3FFF)
        o->rendState = r & ~bits & 0xDFFFFFFF;
}

// v12 0x004CC190  (feb 0x004BC0D0)
void objOBJ::SetStateRendYes(int bits)
{
    objRendYes(this, bits);
    FOR_CHILDREN(this, c)
        c->SetStateRendYes(bits);
}

// v12 0x004CC220  (feb 0x004BC160)
void objOBJ::SetStateRendNo(int bits)
{
    objRendNo(this, bits);
    FOR_CHILDREN(this, c)
        c->SetStateRendNo(bits);
}

// v12 0x004CC290  (feb 0x004BC1D0)
void objOBJ::_SetStateRendYes(int bits)
{
    objRendYes(this, bits);
}

// v12 0x004CC2E0  (feb 0x004BC220)
void objOBJ::_SetStateRendNo(int bits)
{
    objRendNo(this, bits);
}

// v12 0x004CC320  (feb 0x004BC260)
void objOBJ::SetStateVBNo(int bits)
{
    if (geom)
        *(u32 *)geom->unk66 &= ~bits;
    FOR_CHILDREN(this, c)
        c->SetStateVBNo(bits);
}

// v12 0x004CC380  (feb 0x004BC2C0)
// Process-state bits on every object the filter accepts (0x10000 implies 0x1000).
void objOBJ::SetStateProcYes(int bits, BOOL32 (__fastcall *f)(objOBJ *, void *))
{
    if (bits & 0x10000)
        bits |= 0x1000;
    if (!f || f(this, 0))
        procState |= bits;
    FOR_CHILDREN(this, c)
        c->SetStateProcYes(bits, f);
}

// v12 0x004CC410  (feb 0x004BC350)
// Clears the bits; the filter applies to this object only, the subtree is cleared unconditionally.
void objOBJ::SetStateProcNo(int bits, BOOL32 (__fastcall *f)(objOBJ *, void *))
{
    if (!f || f(this, 0))
        procState &= ~bits;
    FOR_CHILDREN(this, c)
        c->SetStateProcNo(bits, 0);
}

// v12 0x004CC8D0  (feb 0x004BC870)
// First object of the subtree (depth-first, root first) the filter accepts.
objOBJ *__fastcall objFind(objOBJ *root, objFILTER *f)
{
    if (!root)
        return 0;
    if (f->IsUse(root))
        return root;
    FOR_CHILDREN(root, c) {
        objOBJ *r = objFind(c, f);
        if (r)
            return r;
    }
    return 0;
}

// v12 0x004CC9B0  (feb 0x004BC950)
// The object with the most vertices (process state bits 0x3F clear), or 0.
objOBJ *objOBJ::FindObjMaxNVert()
{
    if (!this)
        return 0;
    int n = 0;
    objOBJ *best = 0;
    if (!(procState & 0x3F) && nVert > 0) {
        best = this;
        n = nVert;
    }
    FOR_CHILDREN(this, c) {
        objOBJ *r = c->FindObjMaxNVert(&n);
        if (r)
            best = r;
    }
    return best;
}

// v12 0x004CCA10  (feb 0x004BC9B0)
objOBJ *objOBJ::FindObjMaxNVert(int *n)
{
    if (!this)
        return 0;
    objOBJ *best = 0;
    if (!(procState & 0x3F) && *n < nVert) {
        *n = nVert;
        best = this;
    }
    FOR_CHILDREN(this, c) {
        objOBJ *r = c->FindObjMaxNVert(n);
        if (r)
            best = r;
    }
    return best;
}

// v12 0x004CCB00  (feb 0x004BCAA0)
objOBJ *__fastcall objFindName(objOBJ *root, char *name)
{
    if (!root || !name)
        return 0;
    objFILTER_NAME f(name);
    if (f.IsUse(root))
        return root;
    FOR_CHILDREN(root, c) {
        objOBJ *r = objFind(c, &f);
        if (r)
            return r;
    }
    return 0;
}

// v12 0x004CCC00  (feb 0x004BCBA0)
objOBJ *__fastcall objFindID(objOBJ *root, int id)
{
    objFILTER_ID f(id);                                 // vtable v12 0x00606D64
    if (!root)
        return 0;
    if (f.IsUse(root))
        return root;
    FOR_CHILDREN(root, c) {
        objOBJ *r = objFind(c, &f);
        if (r)
            return r;
    }
    return 0;
}

// v12 0x004CCCE0  (feb 0x004BCC80)
BOOL32 objFILTER_NAME::IsUse(objOBJ *o)
{
    if (!o->name)
        return 0;
    if (!name)
        return 0;
    return strcmp(o->name, name) == 0;
}

// v12 0x004CCD50  (feb 0x004BCCF0)
// Every object of the given subtrees the filter accepts, depth-first, at most max.
int __fastcall objMakePtrList(objOBJ **roots, int nRoots, objFILTER *f, objOBJ **out, int max)
{
    int n = 0;
    for (int i = 0; i < nRoots; i++) {
        objOBJ *o = roots[i];
        if (!o)
            continue;
        if (!f || f->IsUse(o)) {
            if (n >= max)
                continue;
            out[n++] = o;
        }
        FOR_CHILDREN(o, c)
            if (!_objBuildObjPtrList(c, f, out, max, &n))
                break;
    }
    return n;
}

// v12 0x004CCE10  (feb 0x004BCDB0)
int __fastcall objMakePtrList(objOBJ **roots, int nRoots, objOBJ **out, int max)
{
    if (nRoots <= 0)
        return 0;
    int n = 0;
    for (int i = 0; i < nRoots; i++) {
        objOBJ *o = roots[i];
        if (!o || n >= max)
            continue;
        out[n++] = o;
        FOR_CHILDREN(o, c)
            if (!_objBuildObjPtrList(c, 0, out, max, &n))
                break;
    }
    return n;
}

// v12 0x004CCEC0  (feb 0x004BCE60)
// out[idHier] = object for the subtree; returns the number stored.
int __fastcall objMakePtrListID(objOBJ *root, objOBJ **out, int max)
{
    int n = 0;
    if (root && root->idHier < max && root->idHier >= 0) {
        out[root->idHier] = root;
        n = 1;
        FOR_CHILDREN(root, c)
            if (!_objIncludePtrListID(c, out, max, &n))
                break;
    }
    return n;
}

// v12 0x004CCF70  (feb 0x004BCF10)
// Is this o or one of o's parents.
BOOL32 objOBJ::IsAncestor(objOBJ *o)
{
    while (this != o) {
        o = o->parent;
        if (!o)
            return 0;
    }
    return 1;
}

// v12 0x004CD060  (feb 0x004BCFD0)
int __fastcall _objBuildObjPtrList(objOBJ *o, objFILTER *f, objOBJ **out, int max, int *n)
{
    if (!o)
        return 1;
    if (!f || f->IsUse(o)) {
        if (max <= *n)
            return 0;
        out[(*n)++] = o;
    }
    FOR_CHILDREN(o, c)
        if (!_objBuildObjPtrList(c, f, out, max, n))
            return 0;
    return 1;
}

// v12 0x004CD110  (feb 0x004BD080)
int __fastcall _objIncludePtrListID(objOBJ *o, objOBJ **out, int max, int *n)
{
    if (!o)
        return 1;
    if (max <= o->idHier || o->idHier < 0)
        return 0;
    out[o->idHier] = o;
    (*n)++;
    FOR_CHILDREN(o, c)
        if (!_objIncludePtrListID(c, out, max, n))
            return 0;
    return 1;
}

// v12 0x004CD2B0  (feb 0x004BD220)
BOOL32 objFILTER_ID::IsUse(objOBJ *o)
{
    return o->idHier == id;
}

// v12 0x004CD2D0  (feb 0x004BD240)
BOOL32 objFILTER_FUNC::IsUse(objOBJ *o)
{
    return fn(o, data);
}

// v12 0x004CD2E0  (feb 0x004BD250)
BOOL32 objFILTER_ANIM_NMB::IsUse(objOBJ *o)
{
    return o->unk20 == n;
}

// ---------------------------------------------------------------------------------------------
// faces and materials
// ---------------------------------------------------------------------------------------------

static objOBJ *s_objCmp;               // v12 0x008FA950 (feb 0x008B6710) the object _objCmpFace sorts
static int     s_texN, s_texMax;        // v12 0x008FA944 / 0x008FA954 (feb 0x008B6704 / 0x008B6714) objMakeTexList

// v12 0x004CB5E0  (feb 0x004BB630)
// Resize every face record for nTexSets texture index sets (-1: as many as the material controls use),
// moving the records in place.
BOOL32 objOBJ::_CompressFaceList(int ns)
{
    if (!nFace)
        return 1;
    if (ns == -1) {
        objMTL_CTRL_TBL *t = &geom->split->ctrl;
        int mx = -1;
        for (int i = 0; i < t->n; i++)
            if (mx <= t->Get(i)->tcInd)
                mx = t->Get(i)->tcInd;
        ns = mx + 1;
    }
    int nsz = ns * 6 + 8, osz = geom->faceSize;
    if (nsz < osz) {
        for (int i = 0; i < nFace; i++)
            memmove((u8 *)geom->faceList + i * nsz, (u8 *)geom->faceList + geom->faceSize * i, nsz);
        if (!_AllocFaceList(nFace, ns))
            return 0;
    } else if (nsz > osz) {
        if (!_AllocFaceList(nFace, ns))
            return 0;
        for (int i = nFace - 1; i >= 0; i--)
            memmove((u8 *)geom->faceList + geom->faceSize * i, (u8 *)geom->faceList + osz * i, osz);
    }
    return 1;
}

// v12 0x004CB700  (feb 0x004BB740)
// Sort the faces by material, then rebuild the split entries (one per face when there are as many entries
// as faces, else runs of equal materials).
BOOL32 objOBJ::_SortFaceList()
{
    if (!nFace)
        return 1;
    s_objCmp = this;
    qsort(geom->faceList, nFace, geom->faceSize, _objCmpFace);
    if (geom->split->n == nFace)
        return _CreateFaceSplitPerFace() != 0;
    return _CompressFaceSplit() != 0;
}

// v12 0x004CB760  (feb 0x004BB7A0)
// Sort the split entries by material and lay the faces out in that order.
BOOL32 objOBJ::_SortMtlList()
{
    objSPLIT *s = geom->split;
    if (!s || !s->n)
        return 1;
    s_objCmp = this;
    qsort(s->GetEntry(0), s->n, sizeof(objSPLIT_ENTRY), _objCmpSplitEntry);
    u8 *tmp = (u8 *)apCalloc(geom->faceSize, nFace);
    if (!tmp)
        return 0;
    int cur = 0;
    for (int e = 0; e < geom->split->n; e++) {
        objSPLIT_ENTRY *en = geom->split->GetEntry(e);
        int fs = geom->faceSize;
        memcpy(tmp + fs * cur, (u8 *)geom->faceList + fs * en->firstFace, fs * en->nFace);
        for (int j = 0; j < en->nFace; j++)
            ((objFACE *)(tmp + geom->faceSize * (cur + j)))->split = (u16)e;
        en->firstFace = (u16)cur;
        cur += en->nFace;
    }
    memcpy(geom->faceList, tmp, geom->faceSize * nFace);
    apFree(tmp);
    _CompressFaceSplit();
    return 1;
}

// v12 0x004CB8F0  (feb 0x004BB900)
// Flag every texture the subtree uses (0x200: keep loaded).
void objOBJ::MarkUsedTex()
{
    if (geom && geom->split && geom->split->n) {
        for (int e = 0; e < geom->split->n; e++) {
            objSPLIT_ENTRY *en = geom->split->GetEntry(e);
            for (int m = 0; m < en->nMtl; m++) {
                txmTEXTURE *t = en->Get(m)->tex;
                if (t)
                    t->flags |= 0x200;
            }
        }
    }
    FOR_CHILDREN(this, c)
        c->MarkUsedTex();
}

// v12 0x004CB9F0  (feb 0x004BB9E0)
// Replace texture 'from' by 'to' in the subtree (from 0: the base layer, render pass 0, of every entry).
// A replaced animated texture (0x10000) makes 'to' animated too (0x30000). The split is unshared first.
void objOBJ::ReplaceTex(txmTEXTURE *from, txmTEXTURE *to)
{
    if (nFace > 0) {
        if (!from) {
            _UnshareFaceSplit();
            for (int e = 0; e < geom->split->n; e++) {
                objMTL *m = geom->split->GetEntry(e)->FindMtlRendMtl(this, 0);   // (not checked for 0)
                if (m->tex) {
                    if (m->tex->flags & 0x10000)
                        to->flags |= 0x30000;
                    m->tex = to;
                }
            }
        } else {
            if (from->flags & 0x10000)
                to->flags |= 0x30000;
            BOOL32 unshared = 0;
            for (int e = 0; e < geom->split->n; e++)
                for (int m = 0; m < geom->split->GetEntry(e)->nMtl; m++)
                    if (geom->split->GetEntry(e)->Get(m)->tex == from) {
                        if (!unshared) {
                            _UnshareFaceSplit();
                            unshared = 1;
                        }
                        geom->split->GetEntry(e)->Get(m)->tex = to;
                    }
        }
        rendState &= ~0x20000000u;
    }
    FOR_CHILDREN(this, c)
        c->ReplaceTex(from, to);
}

// v12 0x004CBC00  (feb 0x004BBB60)
void objOBJ::SetTex(int rendMtl, txmTEXTURE *t)
{
    if (nFace > 0) {
        _UnshareFaceSplit();
        for (int e = 0; e < geom->split->n; e++) {
            objMTL *m = geom->split->GetEntry(e)->FindMtlRendMtl(this, rendMtl);
            if (m)
                m->tex = t;
        }
        rendState &= ~0x20000000u;
    }
    FOR_CHILDREN(this, c)
        c->SetTex(rendMtl, t);
}

// v12 0x004CBCD0  (feb 0x004BBC20)
// By name; the new texture inherits the old one's blending flags.
void objOBJ::ReplaceTex(char *from, char *to)
{
    txmTEXTURE *a = txmManager->Add(from, 0x100, (txmADD_MODE)1);
    if (!a)
        return;
    txmTEXTURE *b = txmManager->Add(to, 0x100, (txmADD_MODE)1);
    if (!b)
        return;
    if (a->flags & 0x10)
        b->flags |= 0x20010;
    if (a->flags & 0x20)
        b->flags |= 0x20020;
    if (a->flags & 8)
        b->flags |= 0x20008;
    if (a->flags & 0x10000)
        b->flags |= 0x30000;
    ReplaceTex(a, b);
}

// v12 0x004CBD60  (feb 0x004BBCB0)
// Every name -> name + suffix.
void objOBJ::ReplaceTexList(char **names, int n, char *suffix)
{
    char b[0x40];
    for (int i = 0; i < n; i++) {
        sprintf(b, "%s%s", names[i], suffix);
        ReplaceTex(names[i], b);
    }
}

// v12 0x004CBDC0  (feb 0x004BBD10)
// Objects whose state has none of 'stateMask' (top only; children always) get the colour.
void objOBJ::SetConstColor(u32 argb, int mode, int stateMask)
{
    if (nFace > 0 && !(stateMask & state))
        _SetConstColor(argb, mode);
    FOR_CHILDREN(this, c)
        c->SetConstColor(argb, mode, 0);
}

// v12 0x004CBE40  (feb 0x004BBD90)
// mode 0: the whole colour, 1: only its alpha, 2: only its RGB, else none (0).
void objOBJ::_SetConstColor(u32 argb, int mode)
{
    objGEOM *g = geom;
    if (!g)
        return;
    if (mode == 0)
        g->constColor = argb;
    else if (mode == 1)
        g->constColor = (g->constColor & 0x00FFFFFF) | (argb & 0xFF000000);
    else if (mode == 2)
        g->constColor = (argb & 0x00FFFFFF) | (g->constColor & 0xFF000000);
    else
        g->constColor = 0;
    _UpdRenderState();
}

// v12 0x004CBEC0  (feb 0x004BBE10)
void objOBJ::UpdRenderState()
{
    _UpdRenderState();
    FOR_CHILDREN(this, c)
        c->UpdRenderState();
}

// v12 0x004CBF10  (feb 0x004BBE60)
// The render-state bits from the materials and colours: 1 textured, 4 the base texture has alpha, 8/2
// vertex alpha / colours, 0x10 vertex colours present, 0x20/0x80/0x40 blending. A change of the low 14 bits
// clears 0x20000000 (render configuration valid).
void objOBJ::_UpdRenderState()
{
    if (!geom)
        return;
    u32 old = rendState;
    rendState = old & ~0x3Fu;
    uintptr_t oldLow = old & 0x3FFF;
    BOOL32 blend = 0;
    uintptr_t t = oldLow;                               // (the texture variable starts as these bits)
    if (nFace > 0) {
        txmTEXTURE *tx = GetFaceTexRendMtl(0, 0);
        t = (uintptr_t)tx;
        if (tx) {
            u32 r = rendState;
            rendState = r | 1;
            if (tx->flags & 4) {
                blend = 1;
                rendState = r | 5;
            }
        }
    }
    objGEOM *g = geom;
    if ((u8)(g->constColor >> 24) != 0xFF) {
        rendState |= 10;
        blend = 1;
    }
    if ((g->constColor & 0xFFFFFF) != 0xFFFFFF || !t || (rendState & 0x800))
        rendState |= 10;
    if (g->color) {
        for (int i = 0; i < nVert; i++) {
            if ((u8)(g->color[i] >> 24) != 0xFF) {
                rendState |= 8;
                blend = 1;
            }
            if ((g->color[i] & 0xFFFFFF) != 0xFFFFFF)
                rendState |= 2;
        }
        rendState |= 0x10;
    }
    u32 u = rendState;
    if (!(u & 0x1C0)) {
        if (blend)
            rendState = u | 0xA0;
    } else {
        rendState = u | 0x20;
        if (u & 8)
            rendState = u | 0x22;
    }
    if (oldLow != (rendState & 0x3FFF))
        rendState &= ~0x20000000u;
}

// v12 0x004CC040  (feb 0x004BBF90)
void objOBJ::SetTransparency(int mode, u8 alpha)
{
    if (geom)
        _SetTransparency(mode, alpha);
    FOR_CHILDREN(this, c)
        c->SetTransparency(mode, alpha);
}

// v12 0x004CC0B0  (feb 0x004BC030)
// The constant colour's alpha; mode 0x40 additive / 0x80 alpha blending (1.2: mode 0 = keep additive if
// set, else blending when alpha < 255).
void objOBJ::_SetTransparency(int mode, u8 alpha)
{
    objGEOM *g = geom;
    if (!g)
        return;
    if (!mode) {
        if (rendState & 0x40)
            mode = 0x40;
        else if (alpha < 0xFF)
            mode = 0x80;
    }
    u32 r = rendState;
    u32 oldLow = r & 0x3FFF;
    r &= ~0xC0u;
    rendState = r;
    if (mode & 0x40)
        rendState = r | 0x60;
    else if ((signed char)mode < 0)
        rendState = r | 0xA0;
    g->constColor = ((u32)alpha << 24) | (g->constColor & 0xFFFFFF);
    _UpdRenderState();
    if (oldLow != (rendState & 0x3FFF))
        rendState &= ~0x20000000u;
}

// v12 0x004CC310  (feb 0x004BC250)
void objOBJ::InvalidateRendCFG()
{
    rendState &= ~0x20000000u;
}

// (feb 0x004BC3C0) not in 1.2
void objOBJ::PropagateStateProc(int mask, int bits)
{
    u32 s = procState;
    procState = s | bits;
    FOR_CHILDREN(this, c)
        c->PropagateStateProc(mask, (s | bits) & mask);
}

// v12 0x004CC480  (feb 0x004BC420)
void objOBJ::SetMtlCoeff(mtlCOEFF *c)
{
    if (nFace > 0)
        memcpy(geom->split->ctrl.coeff, c, sizeof(mtlCOEFF));
    FOR_CHILDREN(this, ch)
        ch->SetMtlCoeff(c);
}

// v12 0x004CC500  (feb 0x004BC4A0)
// Shift a texture coordinate set by whole units so that no coordinate is below -100 / above 100.
void objOBJ::_NormalizeTextCoord(int set)
{
    float minU = 2000000.0f, maxU = -2000000.0f, minV = 2000000.0f, maxV = -2000000.0f;
    int n = geom->nTexCoord[set];
    m2dV *tc = (m2dV *)geom->texCoord[set];
    for (int i = 0; i < n; i++) {
        if (tc[i].x <= minU) minU = tc[i].x;
        if (maxU <= tc[i].x) maxU = tc[i].x;
        if (tc[i].y <= minV) minV = tc[i].y;
        if (maxV <= tc[i].y) maxV = tc[i].y;
    }
    // (the original also tests "1.0 < 1e-6" before each floor: always false, so the floor is always taken)
    if (-100.0f <= minU) {
        if (100.0f < maxU) {
            float d = floorf(maxU);
            for (int i = 0; i < geom->nTexCoord[set]; i++)
                ((m2dV *)geom->texCoord[set])[i].x -= d;
        }
    } else {
        float d = floorf(minU);
        for (int i = 0; i < geom->nTexCoord[set]; i++)
            ((m2dV *)geom->texCoord[set])[i].x -= d;
    }
    if (-100.0f <= minV) {
        if (100.0f < maxV) {
            float d = floorf(maxV);
            for (int i = 0; i < geom->nTexCoord[set]; i++)
                ((m2dV *)geom->texCoord[set])[i].y -= d;
        }
    } else {
        float d = floorf(minV);
        for (int i = 0; i < geom->nTexCoord[set]; i++)
            ((m2dV *)geom->texCoord[set])[i].y -= d;
    }
}

// v12 0x004CC810  (feb 0x004BC7B0)
// An object whose vertices come in triples (in, point, out handles) as a Bezier 3D spline in world space.
m3dSPL *__fastcall objConvertSpl(objOBJ *o)
{
    int nv = o->nVert;
    if (!nv || nv % 3)
        return 0;
    m3dSPL *s = m3dSPL::Make(M3D_SPL_BEZIER3D, nv / 3);
    if (!s)
        return 0;
    o->TransformPoints(0, 0);
    for (int i = 0, v = 2; i < s->nKp; i++, v += 3) {
        int k = s->nKpMax < i ? s->nKpMax : i;
        u8 *kp = (u8 *)s->kp + s->kpStride * k;
        o->GetTransformedVert(v - 2, (m3dV *)(kp + 0x10));
        o->GetTransformedVert(v - 1, (m3dV *)(kp + 4));
        o->GetTransformedVert(v, (m3dV *)(kp + 0x1C));
    }
    return s;
}

// v12 0x004CC930  (feb 0x004BC8D0)
objOBJ *__fastcall objFind(objOBJ *root, BOOL32 (__fastcall *fn)(objOBJ *, void *), void *data)
{
    objFILTER_FUNC f(fn, data);                         // vtable v12 0x00606D5C
    if (!root)
        return 0;
    if (f.IsUse(root))
        return root;
    FOR_CHILDREN(root, c) {
        objOBJ *r = objFind(c, &f);
        if (r)
            return r;
    }
    return 0;
}

// v12 0x004CCA70  (feb 0x004BCA10)
// Compare two material tables layer by layer by texture name; then by the number of layers.
int __fastcall objOBJ::CompMtlTbl(objMTL_TBL *a, objMTL_TBL *b)
{
    for (int i = 0;; i++) {
        int n = a->nMtl < b->nMtl ? a->nMtl : b->nMtl;
        if (i >= n)
            return a->nMtl - b->nMtl;
        txmTEXTURE *ta = a->Get(i)->tex, *tb = b->Get(i)->tex;
        if (!ta) {
            if (tb)
                return -1;
        } else {
            if (!tb)
                return 1;
            int r = stricmp(ta->name, tb->name);
            if (r)
                return r;
        }
    }
}

// v12 0x004CCB80  (feb 0x004BCB20)
// The first object whose name starts with the n first characters of name.
objOBJ *__fastcall objFindNameN(objOBJ *root, char *name, int n)
{
    objFILTER_NAME_LEN f(name, n);
    if (!root)
        return 0;
    if (f.IsUse(root))
        return root;
    FOR_CHILDREN(root, c) {
        objOBJ *r = objFind(c, &f);
        if (r)
            return r;
    }
    return 0;
}

// v12 0x004CCC70  (feb 0x004BCC10)
objOBJ *__fastcall objFindAnimNmb(objOBJ *root, int n)
{
    objFILTER_ANIM_NMB f(n);                            // vtable v12 0x00606D68
    if (!root)
        return 0;
    if (f.IsUse(root))
        return root;
    FOR_CHILDREN(root, c) {
        objOBJ *r = objFind(c, &f);
        if (r)
            return r;
    }
    return 0;
}

// v12 0x004CCF40  (feb 0x004BCEE0)
// The distinct textures of render pass rendMtl in the subtree (at most max); returns how many.
int __fastcall objMakeTexList(objOBJ *root, int rendMtl, txmTEXTURE **out, int max)
{
    s_texN = 0;
    s_texMax = max;
    _objIncludeTexList(root, rendMtl, out);
    return s_texN;
}

// v12 0x004CCF90  (feb 0x004BCF30)
void objOBJ::DeclareProcAnim()
{
    state &= ~0x1000u;
}

// v12 0x004CCFA0  (feb 0x004BCF40)
int __cdecl _objCmpFace(const void *pa, const void *pb)
{
    const objFACE *a = (const objFACE *)pa, *b = (const objFACE *)pb;
    objSPLIT *s = s_objCmp->geom->split;
    int r = objOBJ::CompMtlTbl(s->GetEntry(a->split), s->GetEntry(b->split));
    if (r)
        return r;
    for (int i = 0; i < 3; i++) {
        if (a->vert[i] < b->vert[i])
            return -1;
        if (a->vert[i] > b->vert[i])
            return 1;
    }
    return 0;
}

// v12 0x004CD050  (feb 0x004BCFC0)
int __cdecl _objCmpSplitEntry(const void *a, const void *b)
{
    return objOBJ::CompMtlTbl((objMTL_TBL *)a, (objMTL_TBL *)b);
}

// v12 0x004CD190  (feb 0x004BD100)
int __fastcall _objIncludeTexList(objOBJ *o, int rendMtl, txmTEXTURE **out)
{
    if (!o)
        return 1;
    if (o->nFace > 0 && o->geom->split->n) {
        for (int e = 0; e < o->geom->split->n; e++) {
            txmTEXTURE *t = o->GetFaceTexRendMtl(o->geom->split->GetEntry(e)->firstFace, rendMtl);
            if (!t)
                continue;
            int k = 0;
            while (k < s_texN && out[k] != t)
                k++;
            if (k < s_texN)
                continue;
            if (s_texN >= s_texMax)
                break;
            out[s_texN++] = t;
        }
    }
    FOR_CHILDREN(o, c)
        if (!_objIncludeTexList(c, rendMtl, out))
            return 0;
    return 1;
}
