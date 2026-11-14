// cdt_obj.cpp -- engine.lib collision accessors of one object (cdtCOLL_OBJ): ids are faces
// (Will Rock 1.2, v12.exe; feb obj engine:cdt_obj.obj)
//
// v12 0x00501B70..0x0050274F. data = the objOBJ. A face record is faceSize bytes, the three u16 vertex
// indices at +2/+4/+6; vertices are taken through the object's world matrix (ltm) unless frozen.
// In 1.0/1.2 the vtable gained slot 15 (GetFacePolyNormal, poly + normal in one call) and the bounding
// sphere tests read objGEOM::bsphere directly.

#include "engine.h"

static inline objOBJ *OBJ(void *d) { return (objOBJ *)d; }

// v12 0x00501B70  (feb 0x004EF9A0)
int cdtCOLL_OBJ::GetNID()
{
    return OBJ(data)->nFace;
}

// v12 0x004B9CF0  identical-code-folded "return 0" (one-argument)
void *cdtCOLL_OBJ::GetID2Ptr(int id)
{
    return 0;
}

// v12 0x00501B80  (feb 0x004EF9B0)
int cdtCOLL_OBJ::GetNID2(int id)
{
    return -1;
}

// v12 0x00502060  (feb 0x004EFE30)  the object's local box, as is (not transformed)
void cdtCOLL_OBJ::GetBBoxAll(m3dBOX *out)
{
    m3dBOX_DATA *b = &OBJ(data)->bbox;
    ((m3dBOX_DATA *)out)->type = b->type;
    memcpy(&out->vMin, b->min, 12);
    memcpy(&out->vMax, b->max, 12);
}

// v12 0x005020B0  (feb 0x004EFE80)  world box of face id
BOOL32 cdtCOLL_OBJ::GetIDBBox(int id, m3dBOX *out)
{
    objOBJ *o = OBJ(data);
    if (!o->nFace)
        return 0;
    out->CalcStart();
    for (int k = 0; k < 3; k++) {
        m3dV p;
        objFaceVertW(o, id, k, &p);
        out->CalcUpdate(&p);
    }
    return 1;
}

// v12 0x005022F0  (feb 0x004F00C0)  does face id touch box b // ? body inferred from the SCN_INST twin
BOOL32 cdtCOLL_OBJ::IsectIDBBox(int id, m3dBOX *b)
{
    objOBJ *o = OBJ(data);
    m3dPOLY poly;
    poly.nVert = 3;
    for (int k = 0; k < 3; k++)
        objFaceVertW(o, id, k, &poly.vert[k]);
    return b->IsIsectPoly(&poly, &o->geom->faceNormal[id]) != 1;
}

// v12 0x00502560  identical-code-folded "return 0" (three arguments; also IsectID2BBox)
BOOL32 cdtCOLL_OBJ::GetID2BBox(void *d2, int id2, m3dBOX *out)
{
    return 0;
}

// v12 0x00502560
BOOL32 cdtCOLL_OBJ::IsectID2BBox(void *d2, int id2, m3dBOX *b)
{
    return 0;
}

// v12 0x00502570
BOOL32 cdtCOLL_OBJ::Is2LevelData()
{
    return 0;
}

// v12 0x00501B90  (feb 0x004EFC00)
BOOL32 cdtCOLL_OBJ::IsDoubleSided(void *d2)
{
    return (OBJ(d2)->state >> 2) & 1;
}

// v12 0x00501BA0  (no feb twin; name ours)
// Triangle f in world space plus its normal (the stored normal, object space, not transformed).
void cdtCOLL_OBJ::GetFacePolyNormal(void *d2, int f, m3dPOLY *poly, m3dV *n)
{
    objOBJ *o = OBJ(d2);
    poly->nVert = 3;
    for (int k = 0; k < poly->nVert; k++)
        objFaceVertW(o, f, k, &poly->vert[k]);
    *n = o->geom->faceNormal[f];
}

// v12 0x00502030  (feb 0x004EFC10)
void cdtCOLL_OBJ::GetFaceNormal(void *d2, int f, m3dV *n)
{
    *n = OBJ(d2)->geom->faceNormal[f];
}

// v12 0x00501DF0  (feb 0x004EF9C0)
void cdtCOLL_OBJ::GetFacePoly(void *d2, int f, m3dPOLY *poly)
{
    objOBJ *o = OBJ(d2);
    poly->nVert = 3;
    for (int k = 0; k < poly->nVert; k++)
        objFaceVertW(o, f, k, &poly->vert[k]);
}

// v12 0x00502620  "return 0" (five arguments)
BOOL32 cdtCOLL_OBJ::IsIsectRayBVol(void *d, int id, m3dV *org, m3dV *dir, float len)
{
    return 0;
}

// v12 0x00502580  (feb 0x004F0330)
// Coarse ray test of a whole object: small objects (< 8 faces) always pass; otherwise the bounding sphere
// must be hit within len, then (free objects only) the local box.
BOOL32 cdtCOLL_OBJ::IsIsectRayBVolAll(void *d, m3dV *org, m3dV *dir, float len)
{
    objOBJ *o = OBJ(d);
    if (o->nFace < 8)
        return 1;
    float t;
    if (!((m3dVOL *)&o->geom->bsphere)->IsectRay(org, dir, len, 0, &t, 0))
        return 0;
    if (!(t <= len))
        return 0;
    if (o->inst)
        return 1;
    return ((m3dVOL *)&o->bbox)->IsectRay(org, dir, len, 0, 0, 0) != 0;
}

// v12 0x00502750  "return 0" (four arguments)
BOOL32 cdtCOLL_OBJ::IsNearBVol(void *d, int id, m3dV *p, float r)
{
    return 0;
}

// v12 0x00502630  (feb 0x004F03E0)
// Is p within r of the object: bounding sphere first, then (free objects only) the local box.
BOOL32 cdtCOLL_OBJ::IsNearBVolAll(void *d, m3dV *p, float r)
{
    objOBJ *o = OBJ(d);
    m3dV c = { o->geom->bsphere.center[0], o->geom->bsphere.center[1], o->geom->bsphere.center[2] };
    float dist = m3dSqrt(dist2(c, *p));
    if (r + o->geom->bsphere.radius < dist)
        return 0;
    if (!o->inst && !((m3dVOL *)&o->bbox)->DistPoint(p, r, 0, 0, 0))
        return 0;
    return 1;
}

// feb 0x004F0500 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
