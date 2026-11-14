// obj_mod.cpp -- engine.lib object modifiers, the object side: the modifier list of an object's geometry, the
// invalidate / validate masks of the hierarchy, applying, timing and animating the modifiers
// (Will Rock 1.2, v12.exe; feb obj engine:obj_mod.obj)
//
// v12 0x004E0150..0x004E05FF, every objOBJ function here an exact twin of the feb code; the objMODIFIER base
// (v12 0x004DFE10..0x004E00A2). Not rebuilt yet: AssignModifiers, FindNextModifier.
//
// A modifier (objMODIFIER, vtable slots: 0 deleting dtor, +0x04 AddObj(obj), +0x08 DelObj, +0x0C Duplicate(int),
// +0x10 Animate(n, frames, weights), +0x14 AdvanceTime, +0x18 Apply) has: +0x04 flags (1 off, 2 shared with the
// template), +0x10 the bits that invalidate it, +0x14 the bits it produces. objOBJ +0xE0 / +0xE4 hold the OR of
// those over the subtree (ValidateModifiersHier).

#include <math.h>
#include "engine.h"
#include "../wr_vcall.h"

struct objMOD_HDR {                         // the fields of objMODIFIER read here
    void   *vtbl;
    u32     flags;                          // +0x04
    u32     type;                           // +0x08 FourCC
    int     prio;                           // +0x0C
    u32     inBits;                         // +0x10
    u32     outBits;                        // +0x14
    objOBJ *obj;                            // +0x18
};

static inline objMOD_HDR *omMod(objOBJ *o, int i)
{
    return (objMOD_HDR *)o->geom->modifier[i];
}

// v12 0x004E0150  (feb 0x004D1ED0)
// Deletes the object's own modifiers (not those shared with the template) and the list.
void objOBJ::RemoveModifiers()
{
    if (!geom)
        return;
    for (int i = 0; i < geom->nModifier; i++) {
        objMOD_HDR *m = omMod(this, i);
        if (!(m->flags & 2)) {
            wrVCall<void>(m, 8);
            i--;
            wrVCall<void *>(m, 0, 1u);
        }
    }
    _FreeModifierList();
    geom->nModifier = 0;
}

// v12 0x004E00B0  (feb 0x004D1E30)
// The template's skins onto the objects that name one (+0x22 = index into the list), shared with the template
// (flag 2); then the subtree.
BOOL32 objOBJ::AssignModifiers(int n, objMODIFIER **list)
{
    objOBJ *first = child;
    if (unk22 != -1) {
        objMODIFIER *m = list[unk22];
        if (!m)
            return 0;
        m->flags |= 2;
        if (!m->AddObj(this))
            return 0;
    }
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        if (!c->AssignModifiers(n, list))
            return 0;
    return 1;
}

// v12 0x004E01B0  (feb 0x004D1F30)
BOOL32 objOBJ::DuplicateModifiers(objOBJ *to)
{
    if (geom)
        for (int i = 0; i < geom->nModifier; i++) {
            objMOD_HDR *c = wrVCall<objMOD_HDR *>(omMod(this, i), 0xC, 1);
            if (c) {
                c->flags &= ~2u;
                if (!wrVCall<BOOL32>(c, 4, to))
                    return 0;
            }
        }
    to->invalidMask = 0;
    to->invalidAdd = 0;
    return 1;
}

// v12 0x004E0230  (feb 0x004D1FB0)
// +0xE0 / +0xE4 of every object = the OR of its modifiers' and its subtree's in / out bits; added to *in / *out.
void objOBJ::ValidateModifiersHier(apSTATE *in, apSTATE *out)
{
    invalidMask = 0;
    invalidAdd = 0;
    if (geom)
        for (int i = 0; i < geom->nModifier; i++) {
            invalidMask |= omMod(this, i)->inBits;
            invalidAdd |= omMod(this, i)->outBits;
        }
    objOBJ *first = child;
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        c->ValidateModifiersHier((apSTATE *)&invalidMask, (apSTATE *)&invalidAdd);
    if (in && out) {
        in->bits |= invalidMask;
        out->bits |= invalidAdd;
    }
}

// v12 0x004E0300  (feb 0x004D2080)
// The bits that invalidating `bits` invalidates through the modifiers of the subtree.
int objOBJ::InvalidateModifiers(int bits)
{
    if (!(bits & invalidMask))
        return bits;
    objOBJ *first = child;
    if (geom)
        for (int i = 0; i < geom->nModifier; i++)
            if (omMod(this, i)->inBits & bits)
                bits |= omMod(this, i)->outBits;
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        bits = c->InvalidateModifiers(bits);
    return bits;
}

// v12 0x004E0370  (feb 0x004D20F0)
// The bits that validating `bits` needs first.
int objOBJ::ValidateModifiers(int bits)
{
    if (!(bits & invalidAdd))
        return bits;
    objOBJ *first = child;
    if (geom)
        for (int i = 0; i < geom->nModifier; i++)
            if (omMod(this, i)->outBits & bits)
                bits |= omMod(this, i)->inBits;
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        bits = c->ValidateModifiers(bits);
    return bits;
}

// v12 0x004E03E0  (feb 0x004D2160)
// Applies the modifiers producing `bits` over the subtree (not for hidden / culled objects: procState & 0x3F,
// unless forced by 0xF000, for 0x205; never for 0x6400 then). 0 when one fails.
BOOL32 objOBJ::ApplyModifiers(int bits)
{
    if (!(bits & invalidAdd))
        return 1;
    objOBJ *first = child;
    if (geom && geom->nModifier) {
        u32 h = procState & 0x3F;
        if ((!h || (procState & 0xF000) || !(bits & 0x205)) && (!h || !(bits & 0x6400)))
            for (int i = 0; i < geom->nModifier; i++) {
                objMOD_HDR *m = omMod(this, i);
                if ((m->outBits & bits) && !(m->flags & 1) && !wrVCall<BOOL32>(m, 0x18))
                    return 0;
            }
    }
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        if (!c->ApplyModifiers(bits))
            return 0;
    return 1;
}

// v12 0x004E04D0  (feb 0x004D2250)
BOOL32 objOBJ::IsApplyModifiers(int bits)
{
    u32 h = procState & 0x3F;
    if (h && !(procState & 0xF000) && (bits & 0x205))
        return 0;
    if (h && (bits & 0x6400))
        return 0;
    return 1;
}

// v12 0x004E0530  (feb 0x004D22B0)
// Moves on the time of the timed modifiers (subtrees with in-bit 0x8000).
void objOBJ::AdvanceModifiersTime()
{
    if (!(invalidMask & 0x8000))
        return;
    objOBJ *first = child;
    if (geom)
        for (int i = 0; i < geom->nModifier; i++)
            if (!(omMod(this, i)->flags & 1))
                wrVCall<void>(omMod(this, i), 0x14);
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        c->AdvanceModifiersTime();
}

// v12 0x004E05B0  (feb 0x004D2330 AnimateModifiers)
// The animation frames (and weights) of the channels to each modifier of the object.
void objOBJ::_AnimateModifiers(int n, float *frames, float *weights)
{
    for (int i = 0; i < geom->nModifier; i++)
        if (!(omMod(this, i)->flags & 1))
            wrVCall<void>(omMod(this, i), 0x10, n, frames, weights);
}

// ---------------------------------------------------------------------------------------------- objMODIFIER

m3dV  objMODIFIER::viewPos;                 // v12 0x009786A0
float objMODIFIER::elapsedTime;             // v12 0x009786AC

// v12 0x004DFE10  (feb 0x004D1B90)
objMODIFIER::objMODIFIER(u32 t, int p)
{
    flags = 0;
    type = t;
    prio = p;
    inBits = 0;
    outBits = 0;
    obj = 0;
}

// v12 0x004DFE40
void *objMODIFIER::__vecDelDtor(unsigned f)
{
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x00403200 / 0x00403940 / 0x00403A20 (shared empty functions)
objMODIFIER *objMODIFIER::Duplicate(int f)
{
    return 0;
}

void objMODIFIER::Animate(int n, float *frames, float *weights)
{
}

void objMODIFIER::AdvanceTime()
{
}

// v12 0x004DFE70
// The view position the modifiers measure from; 1 when it moved (by 1e-6 or more on an axis).
BOOL32 __fastcall objMODIFIER::SetViewPos(m3dV *p)
{
    if (!(fabsf(viewPos.x - p->x) < 1e-6f) || !(fabsf(viewPos.y - p->y) < 1e-6f) ||
        !(fabsf(viewPos.z - p->z) < 1e-6f)) {
        viewPos = *p;
        return 1;
    }
    return 0;
}

// v12 0x004E0670  the list order: by output bits (equal or not), then priority
static int __cdecl objModCompare(const void *a, const void *b)
{
    objMODIFIER *x = *(objMODIFIER **)a, *y = *(objMODIFIER **)b;
    if (x->outBits != y->outBits)
        return 0;
    if (x->prio < y->prio)
        return -1;
    return x->prio > y->prio;
}

// v12 0x004DFF70  (feb 0x004D1CF0)
// Into o's modifier list, kept sorted; the instance's modifiers are to be applied again.
BOOL32 objMODIFIER::AddObj(objOBJ *o)
{
    o->_AllocGeom();
    if (!o->_AllocModifierList(o->geom->nModifier + 1))
        return 0;
    obj = o;
    o->geom->modifier[o->geom->nModifier] = this;
    obj->geom->nModifier++;
    qsort(obj->geom->modifier, obj->geom->nModifier, 4, objModCompare);
    if (obj->inst)
        obj->inst->Invalidate(0x20000);
    return 1;
}

// v12 0x004E0000  (feb 0x004D1D80)
void objMODIFIER::DelObj()
{
    if (!obj)
        return;
    objGEOM *g = obj->geom;
    for (int i = 0; i < g->nModifier; i++) {
        if (g->modifier[i] != this)
            continue;
        if (i < g->nModifier - 1)
            memmove(&g->modifier[i], &g->modifier[i + 1], (g->nModifier - i - 1) * 4);
        obj->geom->modifier[obj->geom->nModifier - 1] = 0;
        obj->geom->nModifier--;
        i--;
        g = obj->geom;
    }
    if (obj->inst)
        obj->inst->Invalidate(0x20000);
    obj = 0;
}

// v12 0x004E06B0  (feb 0x004D2430)
objMOD_BILLBOARD::objMOD_BILLBOARD() : objMODIFIER(0x424C4244, 0x5A)
{
    inBits = 0x10004;
    outBits = 0x10000;
    parentLTM.Identity();
    parentInv.Identity();
}

// v12 0x004E06F0  (feb 0x004D2470)
void *objMOD_BILLBOARD::__vecDelDtor(unsigned f)
{
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004E0710  (feb 0x004D2490)
BOOL32 objMOD_BILLBOARD::AddObj(objOBJ *o)
{
    if (!objMODIFIER::AddObj(o))
        return 0;
    if (flags & 0x4000) {
        parentLTM = *(m3dMATR *)o->parent->ltm;
        parentLTM.Invert(&parentInv);
    }
    o->state &= ~0x1000u;
    return 1;
}

// v12 0x004E0770  (feb 0x004D24E0)
// The object's world matrix turned about its origin, Z to the viewer; its local matrix from that.
BOOL32 objMOD_BILLBOARD::Modify()
{
    static float limit = cosf(3.1415927f * 0.11111111f);    // v12 0x009786B4
    m3dMATR *w = (m3dMATR *)obj->ltm;
    m3dV org, z, to;
    w->GetOrigin(&org);
    w->GetAxisZ(&z);
    to.x = viewPos.x - org.x;
    to.y = viewPos.y - org.y;
    to.z = viewPos.z - org.z;
    if (m3dAngleVector_COS(&z, &to) > limit)
        return 1;
    if (flags & 0x2000) {
        to.y = 0;
        z.y = 0;
    }
    m3dMATR rot;
    rot.MakeRot(&org, &z, &to);
    w->Transform(&rot, 2);
    objOBJ *p = obj->parent;
    if (!p)
        return 1;
    if (!(flags & 0x4000) || !m3dIsEqualMatr_A((m3dMATR *)p->ltm, &parentLTM, 1e-6f, 1e-6f)) {
        parentLTM = *(m3dMATR *)p->ltm;
        parentLTM.Invert(&parentInv);
    }
    m3dMultiplyMatr(w, &parentInv, (m3dMATR *)obj->local);
    return 1;
}
