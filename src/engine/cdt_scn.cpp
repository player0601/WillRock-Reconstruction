// cdt_scn.cpp -- engine.lib collision of the level and of instances: query filters (cdtREFINE),
// cdtCOLL_SCN_INST / cdtCOLL_SCN accessors (Will Rock 1.2, v12.exe; feb obj engine:cdt_scn.obj)
//
// v12 0x004DDD00..0x004DE90F. The level collision (scnSCENE::coll) is a cdtCOLL_SCN over the scene's flat
// object list: id = scene object, id2 = face of that object. 1.0/1.2 changes: cdtREFINE grew a field and a
// virtual (engine_core/collision.h); IsInclInst is otherwise the beta's.
// cdtCOLL_INST (instance collision, animINST::+0x144, data = the animINST).

#include "engine.h"

static inline objOBJ *OBJ(void *d) { return (objOBJ *)d; }
static inline BOOL32 cdtObjSelectable(objOBJ *o)
{
    u32 ps = o->procState;
    return ((ps & 0x1F) == 0 || (ps & 0x10000)) && !(ps & 0x40000);
}

// =============================================================================================
// cdtREFINE (the base filter; the constructor and IsInclObj are header inlines, emitted out of line in
// plw_bul_wpn.obj in both builds)
// =============================================================================================

// v12 0x0040C520  (feb 0x0040B620)
cdtREFINE::cdtREFINE(int m, animINST *excl)
{
    mode = 0;
    queryFlags = 0;
    exclude = excl;
    mode |= m;
}

// v12 0x004DDD00  (feb 0x004D04B0)  [callgraph match: offsets moved with the new +0x08 field]
// Instances with flags 0x11 never collide. mode 2: only physics/dynamic instances (flags & 0x48);
// mode 4: skip flags & 8; mode 8: skip flags & 0x48. The excluded instance never collides.
BOOL32 cdtREFINE::IsInclInst(animINST *i)
{
    u32 f = i->flags;
    if (f & 0x11)
        return 0;
    if ((mode & 2) && !(f & 0x48))
        return 0;
    if ((mode & 4) && (f & 8))
        return 0;
    if ((mode & 8) && (f & 0x48))
        return 0;
    return exclude != i;
}

// v12 0x0040C550  (feb 0x0040B650)
// Objects hidden from collision by their process state (bits 0x1F, unless 0x10000) or 0x40000 never collide.
BOOL32 cdtREFINE::IsInclObj(objOBJ *o)
{
    return cdtObjSelectable(o);
}

// v12 0x004DDD60  NEW in 1.0/1.2 (name ours)
// 0 = the caller asked for the per-bone hit boxes (query flags 0x20), the instance has some and it is not
// a physics object (flags 0x48, or 0x40 with query flag 0x80): test those instead of the enclosing volume.
BOOL32 cdtREFINE::IsUseBVolEncl(animINST *i)
{
    u32 qf = queryFlags;
    if ((qf & 0x20) && i->nBVol > 0) {
        u32 phys = (qf & 0x80) ? (i->flags & 0x40) : (i->flags & 0x48);
        if (!phys)
            return 0;
    }
    return 1;
}

// =============================================================================================
// cdtCOLL_SCN_INST
// =============================================================================================

// v12 0x004DDDA0  (feb 0x004D0510)
BOOL32 cdtCOLL_SCN_INST::RefineInd(int id)
{
    return cdtObjSelectable(OBJ(GetID2Ptr(id)));
}

// v12 0x004DDDD0  (feb 0x004D0540)
// Drop the objects that may not collide; mode 0x200 also drops objects with process state 0x20.
void cdtCOLL_SCN_INST::RefineIndList(cdtOFL *ofl, int mode)
{
    int n = 0;
    for (int i = 0; i < ofl->nObj; i++) {
        objOBJ *o = OBJ(GetID2Ptr(ofl->obj[i]));
        if (cdtObjSelectable(o) && (!(mode & 0x200) || !(o->procState & 0x20)))
            ofl->obj[n++] = ofl->obj[i];
    }
    ofl->nObj = n;
}

// v12 0x004DDE40  (feb 0x004D05B0)
// The object's stored box (local space, as the level stores its objects in world space).
BOOL32 cdtCOLL_SCN_INST::GetIDBBox(int id, m3dBOX *out)
{
    objOBJ *o = OBJ(GetID2Ptr(id));
    if (!o->nFace)
        return 0;
    ((m3dBOX_DATA *)out)->type = o->bbox.type;
    memcpy(&out->vMin, o->bbox.min, 12);
    memcpy(&out->vMax, o->bbox.max, 12);
    return 1;
}

// v12 0x004DDEA0  (feb 0x004D0610)
// Does any face of object id touch box b.
BOOL32 cdtCOLL_SCN_INST::IsectIDBBox(int id, m3dBOX *b)
{
    objOBJ *o = OBJ(GetID2Ptr(id));
    for (int f = 0; f < o->nFace; f++) {
        m3dPOLY poly;
        poly.nVert = 3;
        for (int k = 0; k < 3; k++)
            objFaceVertW(o, f, k, &poly.vert[k]);
        if (b->IsIsectPoly(&poly, &o->geom->faceNormal[f]) != 1)
            return 1;
    }
    return 0;
}

// v12 0x004DE140  (feb 0x004D08B0)
// World box of face id2 of object d2.
BOOL32 cdtCOLL_SCN_INST::GetID2BBox(void *d2, int id2, m3dBOX *out)
{
    objOBJ *o = OBJ(d2);
    m3dV v[3];
    for (int k = 0; k < 3; k++)
        objFaceVertW(o, id2, k, &v[k]);
    out->CalcStart();
    out->CalcUpdate(v, 3);
    return 1;
}

// v12 0x004DE390  (feb 0x004D0B00)
BOOL32 cdtCOLL_SCN_INST::IsectID2BBox(void *d2, int id2, m3dBOX *b)
{
    objOBJ *o = OBJ(d2);
    m3dPOLY poly;
    poly.nVert = 3;
    for (int k = 0; k < 3; k++)
        objFaceVertW(o, id2, k, &poly.vert[k]);
    return b->IsIsectPoly(&poly, &o->geom->faceNormal[id2]) != 1;
}

// =============================================================================================
// cdtCOLL_SCN -- data = scnSCENE
// =============================================================================================
static inline scnSCENE *SCN(void *d) { return (scnSCENE *)d; }

// v12 0x004DE5F0  (feb 0x004D0D60)
int cdtCOLL_SCN::GetNID()
{
    return SCN(data)->nObj;
}

// v12 0x004DE600  (feb 0x004D0D70)
void *cdtCOLL_SCN::GetID2Ptr(int id)
{
    return SCN(data)->objList[id];
}

// v12 0x004DE610  (feb 0x004D0D80)
int cdtCOLL_SCN::GetNID2(int id)
{
    return SCN(data)->objList[id]->nFace;
}

// v12 0x004DE630  (feb 0x004D0DA0)  the box of the level's root object
void cdtCOLL_SCN::GetBBoxAll(m3dBOX *out)
{
    objOBJ *r = SCN(data)->root;
    ((m3dBOX_DATA *)out)->type = r->bbox.type;
    memcpy(&out->vMin, r->bbox.min, 12);
    memcpy(&out->vMax, r->bbox.max, 12);
}

// v12 0x00548680  identical-code-folded "return 1" (no arguments)
BOOL32 cdtCOLL_SCN::Is2LevelData()
{
    return 1;
}

// v12 0x004DE680  (feb 0x004D0E00)
BOOL32 cdtCOLL_SCN::IsIsectRayBVol(void *d, int id, m3dV *org, m3dV *dir, float len)
{
    return cdtCOLL_OBJ::IsIsectRayBVolAll(GetID2Ptr(id), org, dir, len);
}

// v12 0x004DE810  "return 1" (four arguments)
BOOL32 cdtCOLL_SCN::IsIsectRayBVolAll(void *d, m3dV *org, m3dV *dir, float len)
{
    return 1;
}

// v12 0x004DE6B0  (feb 0x004D0E40)
BOOL32 cdtCOLL_SCN::IsNearBVol(void *d, int id, m3dV *p, float r)
{
    return cdtCOLL_OBJ::IsNearBVolAll(GetID2Ptr(id), p, r);
}

// v12 0x004C23D0  identical-code-folded "return 1" (three arguments)
BOOL32 cdtCOLL_SCN::IsNearBVolAll(void *d, m3dV *p, float r)
{
    return 1;
}

// =============================================================================================
// cdtCOLL_ANIM_SCN -- ids = the instance pool of a scene
// =============================================================================================

// v12 0x004DE8B0  (feb 0x004D1030)
int cdtCOLL_ANIM_SCN::GetNID()
{
    return 1000;
}

// v12 0x004DE8C0  (feb 0x004D1040)
// Box of instance id for the instance grid: 0 for a free pool slot (box set to (0,0,0)-(1,1,1)) or an
// instance that ignores collision; a box larger than 1000 on any axis is replaced by the root's object box.
BOOL32 cdtCOLL_ANIM_SCN::GetIDBBox(int id, m3dBOX *out)
{
    animINST *i = scene->InstPtr(id);
    if (scene->IsEmptyInst(i)) {
        out->MakePointPoint(&m3dVZero, &m3dVOneXYZ);
        return 0;
    }
    if (i->flags & 0x10)
        return 0;
    i->GetBBox(out);
    if (!(fabsf(out->vMax.x - out->vMin.x) > 1000.0f) && !(fabsf(out->vMax.y - out->vMin.y) > 1000.0f) &&
        !(fabsf(out->vMax.z - out->vMin.z) > 1000.0f))
        return 1;
    i->Invalidate(2);
    i->Validate(2);
    ((m3dBOX_DATA *)out)->type = i->root->bbox.type;
    memcpy(&out->vMin, i->root->bbox.min, 12);
    memcpy(&out->vMax, i->root->bbox.max, 12);
    return 1;
}

// =============================================================================================
// cdtCOLL_INST -- data = animINST
// =============================================================================================

static inline animINST *INST(void *d) { return (animINST *)d; }

// v12 0x004DE6E0  (feb 0x004D0E70)
int cdtCOLL_INST::GetNID()
{
    return INST(data)->nObj;
}

// v12 0x004DE6F0  (feb 0x004D0E80)
void *cdtCOLL_INST::GetID2Ptr(int id)
{
    return INST(data)->objTable[id];
}

// v12 0x004DE710  (feb 0x004D0EA0)
int cdtCOLL_INST::GetNID2(int id)
{
    return INST(data)->objTable[id]->nFace;
}

// v12 0x00548680  (the shared "return 1", as cdtCOLL_SCN)
// An instance's collision is two-level too: its objects, then their faces.
BOOL32 cdtCOLL_INST::Is2LevelData()
{
    return 1;
}

// v12 0x004DE730  (feb 0x004D0EC0)
void cdtCOLL_INST::GetBBoxAll(m3dBOX *out)
{
    objOBJ *r = INST(data)->root;
    ((m3dBOX_DATA *)out)->type = r->bbox.type;
    memcpy(&out->vMin, r->bbox.min, 12);
    memcpy(&out->vMax, r->bbox.max, 12);
}

// v12 0x004DE780  (feb 0x004D0F10)  the ray must touch the instance's enclosing volume
BOOL32 cdtCOLL_INST::IsIsectRayBVolAll(void *d, m3dV *org, m3dV *dir, float len)
{
    INST(data)->Validate(0x100);
    return INST(data)->bvCur->IsectRay(org, dir, len, 0) != 0;
}

// v12 0x004DE7C0  (feb 0x004D0F50)  "return 1" (five arguments)
BOOL32 cdtCOLL_INST::IsIsectRayBVol(void *d, int id, m3dV *org, m3dV *dir, float len)
{
    return 1;
}

// v12 0x004DE7D0  (feb 0x004D0F60)
BOOL32 cdtCOLL_INST::IsNearBVolAll(void *d, m3dV *p, float r)
{
    INST(data)->Validate(0x100);
    return INST(data)->bvCur->DistPoint(p, r, 0) != 0;
}

// v12 0x004DE810  (feb 0x004D0DF0)  "return 1" (four arguments)
BOOL32 cdtCOLL_INST::IsNearBVol(void *d, int id, m3dV *p, float r)
{
    return 1;
}

// v12 0x004DE820  (feb 0x004D0FA0)
void cdtCOLL_INST::SetCDTFrame(float f)
{
    frame = f;
    flags |= 0x100;
}

// v12 0x004DE840  (feb 0x004D0FC0)
// Was the data built for the instance's current animation time (within 0.02)? No frame set: always.
BOOL32 cdtCOLL_INST::IsCDTFrame()
{
    if (!(flags & 0x100))
        return 1;
    float t = *(float *)((u8 *)data + 0xE4);
    int ok = fabsf(t - frame) < 0.02f;                  // v12 0x005F9ECC
    return ok != 0;
}

// feb 0x004D1150 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
