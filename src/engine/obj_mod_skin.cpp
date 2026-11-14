// obj_mod_skin.cpp -- engine.lib skinned mesh modifier objMOD_SKIN (Will Rock 1.2, v12.exe; feb obj engine:obj_mod_geom.obj)
//
// v12 0x004ECB80..0x004ED0FF and 0x004ED840 / 0x004EDB20..0x004EEBFF / 0x004EF870. Every vertex of the skinned
// object is rebuilt each time the instance's vertices are validated: the sum over its references of the reference's
// weight times its offset carried by the reference's bone object (the object's ltm). A skin with several offset
// sets blends them by the sets' weights first (the weights come from the sets' controllers, Animate).
// The vertex math is the m3d point transform (the game has x87 / 3DNow! / SSE variants of it, picked by
// m3dSimdType; the plain one is used here). Results are in world space (the object's state bit 1), unless the skin
// has flag 0x2000.
// 1.2 changes: the offset sets are 0x54 bytes (feb 0x50: the weight moved to +0x4C, the controller to +0x50); a
// blended set's y and z (not x) are scaled by +0x40, as in the beta.
// Not rebuilt here: the 1.2 template loader's skin passes Reduce (v12 0x004ED550), Reorder (0x004ED890) and the
// skin-LOD builder (0x004ED120); the reader (sceneio_tpl.cpp) keeps the skins as the file has them (one group per
// vertex, Apply_NoReorder), which gives the same vertices.

#include "engine.h"

static const float objSkinEps = 1e-6f;              // v12 0x006076FC

static inline void _skinXform(objOBJ *o, const float *p, float *out)
{
    const float *m = o->ltm;                            // objOBJ +0x3C
    for (int i = 0; i < 3; i++) {
        out[i] = 0;
        for (int k = 0; k < 3; k++)
            out[i] = p[k] * m[k * 4 + i] + out[i];
        out[i] = m[12 + i] + out[i];
    }
}

// v12 0x004ECB80  (feb 0x004E2FC0)
objMOD_SKIN::objMOD_SKIN() : objMOD_GEOM(0x534B494E, 10)   // 'SKIN'
{
    nGroup = 0;
    objTbl = 0;
    nRef = 0;
    nGroupRef = 0;
    ref = 0;
    nBone = 0;
    bone = 0;
    blend = 1.0f;
    reduce = 0;
    inBits = 4;
}

// v12 0x004ECBD0  (feb 0x004E3010)
BOOL32 objMOD_SKIN::Init(int nG, int nR, int nB, int f)
{
    nRef = nR;
    nGroup = nG;
    nBone = nB;
    if (f & 1) {
        objTbl = (u32 *)apCalloc(nGroup, 4);
        if (!objTbl)
            return 0;
    }
    nGroupRef = (int *)apCalloc(nGroup, 4);
    if (!nGroupRef)
        return 0;
    ref = (REF *)apCalloc(nRef, 8);
    if (!ref)
        return 0;
    bone = (objSKIN_BONE *)apCalloc(nBone, sizeof(objSKIN_BONE));
    if (!bone)
        return 0;
    for (int i = 0; i < nBone; i++) {
        bone[i].offs = (float *)apMalloc(nRef * 12);
        if (!bone[i].offs)
            return 0;
        if (f & 2) {
            bone[i].offs2 = (float *)apMalloc(nRef * 12);
            if (!bone[i].offs2)
                return 0;
        }
    }
    return 1;
}

// v12 0x004ECE80  (feb 0x004E3250)
// share: the copy uses this skin's arrays (flag 4 on it and on its sets); else a deep copy, the controllers
// duplicated (their slot +0x1C).
objMODIFIER *objMOD_SKIN::Duplicate(int share)
{
    objMOD_SKIN *s = new objMOD_SKIN;
    if (!s)
        return 0;
    s->flags = flags;
    s->blend = blend;
    if (share) {
        s->nBone = nBone;
        s->nGroup = nGroup;
        s->nRef = nRef;
        s->nGroupRef = nGroupRef;
        s->ref = ref;
        s->objTbl = objTbl;
        s->flags |= 4;
        s->bone = (objSKIN_BONE *)apMalloc(nBone * sizeof(objSKIN_BONE));
        if (!s->bone)
            return 0;
        for (int i = 0; i < nBone; i++) {
            memcpy(&s->bone[i], &bone[i], sizeof(objSKIN_BONE));
            s->bone[i].flags |= 4;
        }
        return s;
    }
    int f = objTbl ? 1 : 0;
    if (bone->offs2)
        f |= 2;
    if (!s->Init(nGroup, nRef, nBone, f))
        return 0;
    if (objTbl)
        memcpy(s->objTbl, objTbl, nGroup * 4);
    memcpy(s->nGroupRef, nGroupRef, nGroup * 4);
    memcpy(s->ref, ref, nRef * 8);
    for (int i = 0; i < nBone; i++) {
        memcpy(s->bone[i].offs, bone[i].offs, nRef * 12);
        if (bone[i].offs2)
            memcpy(s->bone[i].offs2, bone[i].offs2, nRef * 12);
        s->bone[i].weight = bone[i].weight;
        if (bone[i].spl) {
            s->bone[i].spl = wrVCall<m3dSPL *>(bone[i].spl, 0x1C);
            if (!s->bone[i].spl)
                return 0;
        } else {
            s->bone[i].spl = 0;
        }
        s->bone[i].flags &= ~4u;
    }
    s->flags &= ~4u;
    return s;
}

// v12 0x004ED840  (feb 0x004E3B90)
// Flag 0x10000 when every vertex has a single reference (not for grouped skins).
BOOL32 objMOD_SKIN::Check1Bone()
{
    if (flags & 0x4000)
        return 0;
    flags |= 0x10000;
    for (int i = 0; i < nGroup; i++)
        if (nGroupRef[i] > 1)
            flags &= ~0x10000u;
    return flags & 0x10000;
}

objOBJ **objMOD_SKIN::ppObjList;            // v12 0x0097873C

// v12 0x004EDB20  (feb 0x004E3E40)
// Before the vertex modifiers of an instance run: the instance's object table (the skins find their bones in it).
void __fastcall objMOD_SKIN::StartApply(objOBJ **objTable)
{
    ppObjList = objTable;
}

// v12 0x004EDB30  (feb 0x004E3E50)
// The animated sets' weights: the sum over the blended sequences of the controller's value at the frame times the
// sequence's weight, in percent.
void objMOD_SKIN::Animate(int n, float *frames, float *weights)
{
    for (int i = 0; i < nBone; i++) {
        if (!bone[i].spl)
            continue;
        bone[i].weight = 0;
        for (int k = 0; k < n; k++) {
            float v;
            wrVCall<void>(bone[i].spl, 0x10, frames[k], &v, 0);
            bone[i].weight = v * weights[k] * 0.01f + bone[i].weight;
        }
    }
}

// v12 0x004EDBD0  (feb 0x004E3EF0)
BOOL32 objMOD_SKIN::Modify()
{
    if (flags & 0x2000)
        obj->state &= ~1u;
    else
        obj->state |= 1;
    if (flags & 0x4000)
        Apply_Reorder();
    else
        Apply_NoReorder();
    *(u32 *)((char *)obj->geom + 0x66) |= 0x2000;   // the vertices changed
    return 1;
}

// v12 0x004EDC20  (feb 0x004E3F40)
// One group per vertex of the skinned object, its references in order.
void objMOD_SKIN::Apply_NoReorder()
{
    m3dV *vert = obj->geom->vert;
    if (nBone == 1 && fabsf(1.0f - blend) < objSkinEps) {
        if (flags & 0x10000) {                          // one reference each: the transformed offset
            for (int v = 0; v < obj->nVert; v++)
                _skinXform(ppObjList[ref[v].obj], &bone->offs[v * 3], &vert[v].x);
            return;
        }
        int r = 0;
        for (int v = 0; v < obj->nVert; v++) {
            vert[v] = m3dVZero;
            for (int k = 0; k < nGroupRef[v]; k++, r++) {
                float p[3];
                _skinXform(ppObjList[ref[r].obj], &bone->offs[r * 3], p);
                float w = ref[r].weight;
                vert[v].x = p[0] * w + vert[v].x;
                vert[v].y = p[1] * w + vert[v].y;
                vert[v].z = p[2] * w + vert[v].z;
            }
        }
        return;
    }
    int r = 0;
    for (int v = 0; v < obj->nVert; v++) {
        vert[v] = m3dVZero;
        for (int k = 0; k < nGroupRef[v]; k++, r++) {
            float o[3] = { bone->offs[r * 3], bone->offs[r * 3 + 1], bone->offs[r * 3 + 2] };
            for (int b = 1; b < nBone; b++) {
                float w = bone[b].weight;
                const float *q = &bone[b].offs[r * 3];
                o[0] = w * q[0] + o[0];
                o[1] = w * q[1] + o[1];
                o[2] = w * q[2] + o[2];
            }
            o[1] = o[1] * blend;
            o[2] = o[2] * blend;
            float p[3];
            _skinXform(ppObjList[ref[r].obj], o, p);
            float w = ref[r].weight;
            vert[v].x = p[0] * w + vert[v].x;
            vert[v].y = p[1] * w + vert[v].y;
            vert[v].z = w * p[2] + vert[v].z;
        }
    }
}

// v12 0x004EE3A0  (feb 0x004E46C0)
// One group per bone object (objTbl), each reference naming the vertex it adds to; the vertices start at 0.
void objMOD_SKIN::Apply_Reorder()
{
    m3dV *vert = obj->geom->vert;
    memset(vert, 0, obj->nVert * 12);
    int multi = !(nBone == 1 && fabsf(1.0f - blend) < objSkinEps);
    int r = 0;
    for (int g = 0; g < nGroup; g++) {
        objOBJ *b0 = ppObjList[objTbl[g]];
        for (int k = 0; k < nGroupRef[g]; k++, r++) {
            float o[3] = { bone->offs[r * 3], bone->offs[r * 3 + 1], bone->offs[r * 3 + 2] };
            if (multi) {
                for (int b = 1; b < nBone; b++) {
                    float w = bone[b].weight;
                    const float *q = &bone[b].offs[r * 3];
                    o[0] = w * q[0] + o[0];
                    o[1] = w * q[1] + o[1];
                    o[2] = w * q[2] + o[2];
                }
                o[1] = o[1] * blend;
                o[2] = o[2] * blend;
            }
            float p[3];
            _skinXform(b0, o, p);
            float w = ref[r].weight;
            m3dV *d = &vert[ref[r].obj];
            d->x = p[0] * w + d->x;
            d->y = p[1] * w + d->y;
            d->z = w * p[2] + d->z;
        }
    }
}

// v12 0x004EF870
// The arrays unless shared, each set's offsets and controller (slot +0x04, deleting), then objMOD_GEOM's part.
void *objMOD_SKIN::__vecDelDtor(unsigned f)
{
    if (!(flags & 4)) {
        apFree(objTbl);
        objTbl = 0;
        apFree(nGroupRef);
        nGroupRef = 0;
        apFree(ref);
        ref = 0;
        for (int i = 0; i < nBone; i++) {
            if (bone[i].flags & 4)
                continue;
            apFree(bone[i].offs);
            bone[i].offs = 0;
            apFree(bone[i].offs2);
            bone[i].offs2 = 0;
            if (bone[i].spl)
                wrVCall<void *>(bone[i].spl, 4, 1u);
            bone[i].spl = 0;
        }
    }
    apFree(bone);
    bone = 0;
    objMOD_GEOM::__vecDelDtor(0);
    if (f & 1)
        operator delete(this);
    return this;
}
