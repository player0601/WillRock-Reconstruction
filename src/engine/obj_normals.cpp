// obj_normals.cpp -- engine.lib face / vertex normals of objects (Will Rock 1.2, v12.exe; feb obj engine:obj_normals.obj)
//
// v12 0x004EB9B0..0x004ECB7F. 1.2 reorganised this obj: the face functions below were matched by their call
// sites in animINST::Validate (the automatic twin table pairs them wrongly). Per object ("_" functions):
// with object-space normals (faceNormalLCS) the world normals are the LCS ones rotated by the world matrix
// (scale removed); otherwise they are computed from the world vertices. "Perp vectors" are the unnormalised
// cross products; NormalizeFacePerpVectors normalises them later. The recursive versions skip hidden objects
// (process state 0x1C / 0x3F, unless 0xF000).
// 1.2 changes: the LCS rotation no longer checks the instance's invalid bits; NormalizeFacePerpVectors
// skips objects with LCS normals; CalcFaceNormals takes an object filter.
// Not rebuilt yet: the rest of the vertex-normal half (v12 0x004EC790..0x004ECB7F), the skin's own vertex normals
// (objMOD_SKIN v12 0x004EEE60: a skinned object takes the general way in _CalcVertNormals).

#include "engine.h"


static inline BOOL32 objNormVisible(objOBJ *o) { u32 ps = o->procState; return !(ps & 0x1C) || (ps & 0xF000); }
static inline BOOL32 objNormOwn(objOBJ *o)     { u32 ps = o->procState; return !(ps & 0x3F) || (ps & 0xF000); }

// World normals = LCS normals rotated by ltm with its (uniform) scale divided out.
static inline void objRotateLCSNormals(objOBJ *o)
{
    objGEOM *g = o->geom;
    m3dMATR *l = (m3dMATR *)o->ltm;
    float k = 1.0f / m3dSqrt(l->m[0][2] * l->m[0][2] + l->m[0][1] * l->m[0][1] + l->m[0][0] * l->m[0][0]);
    m3dMATR m = *l;
    m.Scale(k, k, k, 2);
    m.TransformVectorList(o->nFace, g->faceNormalLCS, sizeof(m3dV), g->faceNormal, sizeof(m3dV));
}

static inline m3dV *objWorldVerts(objOBJ *o)
{
    if (o->state & 1)
        return o->geom->vert;
    o->TransformPoints(0, 0);
    return objOBJ::GetTransformedVertList();
}

#define FACE_V(o, v, f, k) (&(v)[((u16 *)((u8 *)(o)->geom->faceList + (o)->geom->faceSize * (f) + 2))[k]])

// v12 0x004EB9B0  (feb 0x004DB3C0)
void objOBJ::_CalcFacePerpVectors()
{
    if (!geom->faceNormal && !_AllocNormalList(nFace))
        return;
    if (geom->faceNormalLCS) {
        objRotateLCSNormals(this);
        return;
    }
    m3dV *v = objWorldVerts(this);
    for (int f = 0; f < nFace; f++)
        m3dGetPlanePerpVector(FACE_V(this, v, f, 0), FACE_V(this, v, f, 1), FACE_V(this, v, f, 2), &geom->faceNormal[f]);
}

// v12 0x004EBD10  (feb 0x004DB470)
void objOBJ::CalcFacePerpVectors()
{
    if (!this || !objNormVisible(this))
        return;
    if (geom && objNormOwn(this))
        _CalcFacePerpVectors();
    for (objOBJ *f = child, *c = f; c; c = c->next == f ? 0 : c->next)
        c->CalcFacePerpVectors();
}

// v12 0x004EBD70  (feb 0x004DB4D0)
void objOBJ::NormalizeFacePerpVectors()
{
    if (!this || !objNormVisible(this))
        return;
    if (geom && objNormOwn(this)) {
        if (geom->faceNormalLCS)
            return;                                     // (1.2: the rotated LCS normals are already unit length)
        m3dNormalizeList(nFace, geom->faceNormal);
    }
    for (objOBJ *f = child, *c = f; c; c = c->next == f ? 0 : c->next)
        c->NormalizeFacePerpVectors();
}

// v12 0x004EBDD0  (feb 0x004DBD30)
// Unit face normals of every visible object of the subtree the filter accepts (filter 0: all).
void objOBJ::CalcFaceNormals(BOOL32 (__fastcall *filter)(objOBJ *, void *))
{
    if (!this || !objNormVisible(this))
        return;
    if (geom && objNormOwn(this) && (!filter || filter(this, 0)))
        _CalcFaceNormals();
    for (objOBJ *f = child, *c = f; c; c = c->next == f ? 0 : c->next)
        c->CalcFaceNormals(filter);
}

// v12 0x004EBE40  (feb 0x004DB5D0)
// (The SSE path of the original writes (0,1,0) for a degenerate face; the x87 path, rebuilt here through
// m3dGetPlaneNormal, leaves the zero vector.)
void objOBJ::_CalcFaceNormals()
{
    if (!geom->faceNormal && !_AllocNormalList(nFace))
        return;
    if (geom->faceNormalLCS) {
        objRotateLCSNormals(this);
        return;
    }
    m3dV *v = objWorldVerts(this);
    for (int f = 0; f < nFace; f++)
        m3dGetPlaneNormal(FACE_V(this, v, f, 0), FACE_V(this, v, f, 1), FACE_V(this, v, f, 2), &geom->faceNormal[f]);
}

// v12 0x004EC230  (feb 0x004DB880, extended in 1.2; name ours)
// lcs != 0: object-space normals from the stored vertices into faceNormalLCS; else world normals.
void objOBJ::_CalcFaceNormalsLCS(int lcs)
{
    m3dV *v, *out;
    if (lcs) {
        v = geom->vert;
        out = geom->faceNormalLCS;
    } else {
        TransformPoints(0, 0);
        v = objOBJ::GetTransformedVertList();
        out = geom->faceNormal;
    }
    for (int f = 0; f < nFace; f++)
        m3dGetPlaneNormal(FACE_V(this, v, f, 0), FACE_V(this, v, f, 1), FACE_V(this, v, f, 2), &out[f]);
}

// v12 0x004EC210 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();

// v12 0x00564CA0
static void objNormalizeVectors(int n, m3dV *v)
{
    for (int i = 0; i < n; i++) {
        float l = m3dSqrt(v[i].x * v[i].x + v[i].y * v[i].y + v[i].z * v[i].z);
        if (l > 1e-6f) {
            v[i].x /= l;
            v[i].y /= l;
            v[i].z /= l;
        }
    }
}

// v12 0x004EC2C0  (feb 0x004DBDA0)
void objOBJ::CalcVertNormals(BOOL32 (__fastcall *filter)(objOBJ *, void *))
{
    if (!this || !objNormVisible(this))                 // (v12 0x004EC2C5: none is nothing to do)
        return;
    if (geom && objNormOwn(this) && (!filter || filter(this, 0)))
        _CalcVertNormals();
    objOBJ *first = child;
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        c->CalcVertNormals(filter);
}

// v12 0x004EC330
// With object-space normals: those rotated (normalised only for state 0x8000). Otherwise the sum of the faces' normals at
// each vertex, normalised.
void objOBJ::_CalcVertNormals()
{
    if (!geom->vertNormal && !_AllocVertNormalList(nVert))
        return;
    objGEOM *g = geom;
    if (g->vertNormalLCS) {
        m3dMATR *l = (m3dMATR *)ltm;
        float k = 1.0f / m3dSqrt(l->m[0][2] * l->m[0][2] + l->m[0][1] * l->m[0][1] + l->m[0][0] * l->m[0][0]);
        m3dMATR m = *l;
        m.Scale(k, k, k, 2);
        m.TransformVectorList(nVert, g->vertNormalLCS, sizeof(m3dV), g->vertNormal, sizeof(m3dV));
        if (state & 0x8000)
            objNormalizeVectors(nVert, g->vertNormal);
        return;
    }
    m3dV *v = objWorldVerts(this);
    memset(g->vertNormal, 0, nVert * sizeof(m3dV));
    for (int f = 0; f < nFace; f++) {
        m3dV n;
        m3dGetPlaneNormal(FACE_V(this, v, f, 0), FACE_V(this, v, f, 1), FACE_V(this, v, f, 2), &n);
        u16 *ind = (u16 *)((u8 *)g->faceList + g->faceSize * f + 2);
        for (int k = 0; k < 3; k++) {
            m3dV *o = &g->vertNormal[ind[k]];
            o->x += n.x;
            o->y += n.y;
            o->z += n.z;
        }
    }
    objNormalizeVectors(nVert, g->vertNormal);
}
