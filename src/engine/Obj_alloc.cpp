// Obj_alloc.cpp -- engine.lib mesh storage of objOBJ (Will Rock 1.2, v12.exe; feb obj engine:Obj_alloc.obj)
//
// v12 0x004CED20..0x004CFBAF, every function an "exact" match of the beta (objOBJ / objGEOM layouts are
// unchanged). Each list of an object's mesh lives in its objGEOM and is grown with apRealloc; new
// elements are zeroed (colours: 0xFFFFFFFF). An object duplicated from a template SHARES the
// template's lists (objOBJ::allocFlags, OBJ_SHARE_*): the _Free* functions skip shared lists and the
// _Unshare* functions replace a shared list with a private copy (needed before editing a mesh, e.g.
// the vertex animation and destructible code).
//
// Allocation failure convention (all _Alloc*): returns 0 only if apRealloc failed for a positive size;
// asking for 0 elements frees nothing and succeeds.

#include "engine.h"

// v12 0x004CED20  (feb 0x004BD270)  [callgraph match; v12 tail-jumps into the constructor]
objOBJ *__fastcall objOBJ::Make()
{
    void *p = operator new(sizeof(objOBJ));
    return p ? new (p) objOBJ : 0;
}

// v12 0x004CED40  (feb 0x004BD290)
BOOL32 objOBJ::_AllocName(int len)
{
    char *s = (char *)apRealloc(name, len + 1);
    if (!s) {
        if (len > 0)
            return 0;
    } else {
        s[0] = 0;
    }
    name = s;
    return 1;
}

// v12 0x004CED80  (feb 0x004BD2D0)
void objOBJ::_FreeName()
{
    if (!(allocFlags & OBJ_SHARE_NAME))
        apFree(name);
    name = 0;
    allocFlags &= ~OBJ_SHARE_NAME;
}

// v12 0x004CEDB0  (feb 0x004BD300)
BOOL32 objOBJ::_AllocGeom()
{
    if (geom)
        return 1;
    objGEOM *g = (objGEOM *)operator new(sizeof(objGEOM));
    if (g) {
        // inline constructor
        g->bsphere.type   = M3D_VOL_SPHERE;
        g->bsphere.vtbl   = m3dSPHERE_vftable;
        g->unk78 = 0;
        g->unk7A = 0;
        g->unk7E = -1.0f;
        memset(g->unk66, 0, sizeof(g->unk66));
        g->unk76 = 0xFFFF;
    }
    geom = g;
    if (!g)
        return 0;
    g->color         = 0;
    g->faceList      = 0;
    g->faceSize      = 0;
    g->faceFormat    = 0;
    g->faceNormal    = 0;
    g->faceNormalLCS = 0;
    g->vertNormal    = 0;
    g->vertNormalLCS = 0;
    g->split         = 0;
    g->splitUnshared = 0;
    g->stateFace     = 0;
    g->stateVert     = 0;
    g->vert          = 0;
    g->nModifier     = 0;
    g->modifier      = 0;
    g->constColor         = -1;
    g->unk62         = 0;
    g->bsphere.center[0] = m3dVZero.x;
    g->bsphere.center[1] = m3dVZero.y;
    g->bsphere.center[2] = m3dVZero.z;
    g->bsphere.radius    = 0;
    for (int i = 0; i < 3; i++) {
        g->nTexCoord[i] = 0;
        g->texCoord[i]  = 0;
    }
    return 1;
}

// v12 0x004CEED0  (feb 0x004BD420)  [callgraph]  only the block itself; the lists are freed by their _Free*
void objOBJ::_FreeGeom()
{
    if (geom) {
        geom->bsphere.vtbl = m3dVOL_vftable;   // ~m3dSPHERE of the embedded bound
        operator delete(geom);
    }
    geom = 0;
}

// Grow an array with apRealloc and zero the new tail (the pattern shared by the _Alloc* functions).
static inline void *objGrow(void *p, int oldN, int newN, int elemSize, BOOL32 *ok)
{
    void *q = apRealloc(p, newN * elemSize);
    if (!q && newN > 0) {
        *ok = 0;
        return p;
    }
    if (oldN < newN)
        memset((char *)q + oldN * elemSize, 0, (newN - oldN) * elemSize);
    *ok = 1;
    return q;
}

// v12 0x004CEF00  (feb 0x004BD450)
BOOL32 objOBJ::_AllocVertList(int n)
{
    if (!geom)
        return 0;
    BOOL32 ok;
    void *v = objGrow(geom->vert, nVert, n, sizeof(m3dV), &ok);
    if (!ok)
        return 0;
    geom->vert = (m3dV *)v;
    nVert = n;
    return 1;
}

// v12 0x004CEF80  (feb 0x004BD4D0)
void objOBJ::_FreeVertList()
{
    if (!geom)
        return;
    if (!(allocFlags & OBJ_SHARE_VERTS))
        apFree(geom->vert);
    geom->vert = 0;
    nVert = 0;
    allocFlags &= ~OBJ_SHARE_VERTS;
}

// v12 0x004CEFC0  (feb 0x004BD510)
// (In the original _AllocVertList is inlined here, with the old count still in nVert.)
BOOL32 objOBJ::_UnshareVertList()
{
    if (!geom)
        return 0;
    if (!(allocFlags & OBJ_SHARE_VERTS))
        return 1;
    allocFlags &= ~OBJ_SHARE_VERTS;
    m3dV *shared = geom->vert;
    if (!shared)
        return 1;
    geom->vert = 0;
    int n = nVert;
    if (!_AllocVertList(n))
        return 0;
    memcpy(geom->vert, shared, n * sizeof(m3dV));
    return 1;
}

// v12 0x004CF090  (feb 0x004BD5E0)
BOOL32 objOBJ::_AllocFaceList(int n, int fmt)
{
    if (!geom)
        return 0;
    geom->faceSize   = (u8)_CalcFaceSize(fmt);
    geom->faceFormat = (u8)fmt;
    void *f = apRealloc(geom->faceList, geom->faceSize * n);
    if (!f && n > 0)
        return 0;
    geom->faceList = f;
    if (nFace < n)
        memset((char *)geom->faceList + nFace * geom->faceSize, 0, (n - nFace) * geom->faceSize);
    nFace = n;
    return 1;
}

// v12 0x004CF130  (feb 0x004BD680)
void objOBJ::_FreeFaceList()
{
    if (!geom)
        return;
    if (!(allocFlags & OBJ_SHARE_FACES))
        apFree(geom->faceList);
    geom->faceList   = 0;
    nFace            = 0;
    geom->faceSize   = 0;
    geom->faceFormat = 0;
    allocFlags &= ~OBJ_SHARE_FACES;
}

// v12 0x004CF180  (feb 0x004BD6D0)
// Texture coordinate set 0..2; n = 0 releases the set (honouring sharing).
BOOL32 objOBJ::_AllocTexCoord(int set, int n)
{
    if (!geom || set < 0 || set >= 3)
        return 0;
    if (n == 0) {
        u32 bit = OBJ_SHARE_TEX0 << set;
        if (!(allocFlags & bit))
            apFree(geom->texCoord[set]);
        geom->texCoord[set]  = 0;
        geom->nTexCoord[set] = 0;
        allocFlags &= ~bit;
        return 1;
    }
    BOOL32 ok;
    void *t = objGrow(geom->texCoord[set], geom->nTexCoord[set], n, 8, &ok);
    if (!ok)
        return 0;
    geom->texCoord[set]  = t;
    geom->nTexCoord[set] = (u16)n;
    return 1;
}

// v12 0x004CF270  (feb 0x004BD7C0)
void objOBJ::_FreeTexCoord(int set)
{
    if (!geom || set < 0 || set >= 3)
        return;
    u32 bit = OBJ_SHARE_TEX0 << set;
    if (!(allocFlags & bit))
        apFree(geom->texCoord[set]);
    geom->texCoord[set]  = 0;
    geom->nTexCoord[set] = 0;
    allocFlags &= ~bit;
}

// feb 0x004BD820 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();

// v12 0x004CF2F0  (feb 0x004BD840)
BOOL32 objOBJ::_UnshareTexCoord(int set)
{
    if (!geom || set < 0 || set >= 3)
        return 0;
    u32 bit = OBJ_SHARE_TEX0 << set;
    if (!(allocFlags & bit))
        return 1;
    allocFlags &= ~bit;
    void *shared = geom->texCoord[set];
    if (shared) {
        geom->texCoord[set] = 0;
        if (!_AllocTexCoord(set, geom->nTexCoord[set]))
            return 0;
        memcpy(geom->texCoord[set], shared, geom->nTexCoord[set] * 8);
    }
    return 1;
}

// v12 0x004CF390  (feb 0x004BD8E0)
// The whole list is set to opaque white (0xFFFFFFFF), not only the new tail.
BOOL32 objOBJ::_AllocColorList(int n)
{
    if (!geom)
        return 0;
    u32 *c = (u32 *)apRealloc(geom->color, n * 4);
    if (!c && n > 0)
        return 0;
    for (int i = 0; i < n; i++)
        c[i] = 0xFFFFFFFF;
    geom->color = c;
    return 1;
}

// v12 0x004CF3F0  (feb 0x004BD940)
void objOBJ::_FreeColorList()
{
    if (!geom)
        return;
    if (!(allocFlags & OBJ_SHARE_COLORS))
        apFree(geom->color);
    geom->color = 0;
    allocFlags &= ~OBJ_SHARE_COLORS;
}

// v12 0x004CF420  (feb 0x004BD970)
BOOL32 objOBJ::_UnshareFaceSplit()
{
    if (!geom)
        return 0;
    if (allocFlags & OBJ_SHARE_SPLIT) {
        allocFlags &= ~OBJ_SHARE_SPLIT;
        if (geom->split) {
            objSPLIT *s = geom->split->Duplicate();
            if (!s)
                return 0;
            geom->split = s;
        }
    }
    return 1;
}

// v12 0x004CF460  (feb 0x004BD9B0)
// Recursive version (this object and its whole subtree). Unlike _UnshareFaceSplit a failed Duplicate
// of THIS object is ignored (the shared split stays); failures in children are reported.
BOOL32 objOBJ::UnshareFaceSplit()
{
    objOBJ *first = this ? child : 0;
    if (geom && (allocFlags & OBJ_SHARE_SPLIT)) {
        allocFlags &= ~OBJ_SHARE_SPLIT;
        if (geom->split) {
            objSPLIT *s = geom->split->Duplicate();
            if (s)
                geom->split = s;
        }
    }
    BOOL32 started = 0;
    for (objOBJ *c = first; c && (!started || c != first); c = c->next) {
        if (!c->UnshareFaceSplit())
            return 0;
        started = 1;
    }
    return 1;
}

// v12 0x004CF4F0  (feb 0x004BDA40)
void objOBJ::_FreeFaceSplit()
{
    if (!geom || (allocFlags & OBJ_SHARE_SPLIT))
        return;
    objSPLIT *s = geom->split;
    if (s) {
        apFree(s->entry);
        operator delete(s);
    }
    geom->split = 0;
}

// v12 0x004CF530  (feb 0x004BDA80)
void objOBJ::_FreeSplitUnshared()
{
    if (!geom)
        return;
    void **b = geom->splitUnshared;
    if (b) {
        operator delete(b[0]);
        operator delete(b);
    }
    geom->splitUnshared = 0;
}

// v12 0x004CF560  (feb 0x004BDAB0)
// Grows the modifier pointer list (zeroing new slots); nModifier itself is set by the caller.
BOOL32 objOBJ::_AllocModifierList(int n)
{
    if (!geom)
        return 0;
    BOOL32 ok;
    void *m = objGrow(geom->modifier, geom->nModifier, n, 4, &ok);
    if (!ok)
        return 0;
    geom->modifier = (objMODIFIER **)m;
    return 1;
}

// v12 0x004CF5E0  (feb 0x004BDB30)
void objOBJ::_FreeModifierList()
{
    if (!geom)
        return;
    apFree(geom->modifier);
    geom->modifier = 0;
}

// v12 0x004CF610  (feb 0x004BDB60)
// Make every part of this object private: name, vertices, faces, the 3 texture sets, colours, split.
// The props sheet pointer is dropped (not copied) and the two LCS normal lists are simply forgotten
// (recomputed on demand). In the original the _Alloc*/_Unshare* calls below are partly inlined.
BOOL32 objOBJ::_UnshareAll()
{
    props = 0;
    allocFlags &= ~OBJ_SHARE_DATA;
    if (allocFlags & OBJ_SHARE_NAME) {
        allocFlags &= ~(OBJ_SHARE_NAME | OBJ_SHARE_DATA);
        char *shared = name;
        if (shared) {
            name = 0;
            int len = (int)strlen(shared);
            char *s = (char *)apRealloc(0, len + 1);
            if (!s) {
                if (len > 0)
                    return 0;
            } else {
                s[0] = 0;
            }
            name = s;
            strcpy(s, shared);
        }
    }
    if (!geom)
        return 1;
    if (allocFlags & OBJ_SHARE_VERTS) {
        allocFlags &= ~OBJ_SHARE_VERTS;
        m3dV *shared = geom->vert;
        if (shared) {
            geom->vert = 0;
            if (!_AllocVertList(nVert))
                return 0;
            memcpy(geom->vert, shared, nVert * sizeof(m3dV));
        }
    }
    if (!geom)
        return 0;
    if (allocFlags & OBJ_SHARE_FACES) {
        allocFlags &= ~OBJ_SHARE_FACES;
        void *shared = geom->faceList;
        if (shared) {
            geom->faceList = 0;
            if (!_AllocFaceList(nFace, geom->faceFormat))
                return 0;
            memcpy(geom->faceList, shared, geom->faceSize * nFace);
        }
    }
    for (int i = 0; i < 3; i++)
        if (!_UnshareTexCoord(i))
            return 0;
    if (!geom)
        return 0;
    if (allocFlags & OBJ_SHARE_COLORS) {
        allocFlags &= ~OBJ_SHARE_COLORS;
        u32 *shared = geom->color;
        if (shared) {
            geom->color = 0;
            if (!_AllocColorList(nVert))
                return 0;
            memcpy(geom->color, shared, nVert * 4);
        }
    }
    if (!geom)
        return 0;
    if (allocFlags & OBJ_SHARE_SPLIT) {
        allocFlags &= ~OBJ_SHARE_SPLIT;
        if (geom->split) {
            objSPLIT *s = geom->split->Duplicate();
            if (!s)
                return 0;
            geom->split = s;
        }
    }
    geom->faceNormalLCS = 0;
    allocFlags &= ~OBJ_SHARE_NORM_LCS;
    geom->vertNormalLCS = 0;
    allocFlags &= ~OBJ_SHARE_VNORM_LCS;
    return 1;
}

// Plain resize of a list without zeroing (the normal lists).
#define OBJ_REALLOC_LIST(field, n, elem)                         \
    do {                                                         \
        if (!geom)                                               \
            return 0;                                            \
        void *p_ = apRealloc(geom->field, (n) * (elem));         \
        if (!p_ && (n) > 0)                                      \
            return 0;                                            \
        *(void **)&geom->field = p_;                             \
        return 1;                                                \
    } while (0)

// v12 0x004CF7F0  (feb 0x004BDD40)
BOOL32 objOBJ::_AllocNormalList(int n)       { OBJ_REALLOC_LIST(faceNormal, n, sizeof(m3dV)); }

// v12 0x004CF840  (feb 0x004BDD90)
void objOBJ::_FreeNormalList()
{
    if (!geom)
        return;
    apFree(geom->faceNormal);
    geom->faceNormal = 0;
}

// v12 0x004CF870  (feb 0x004BDDC0)
// Vertex normal list sized to nVert, for this object and its whole subtree.
BOOL32 objOBJ::AllocVertNormalList()
{
    objOBJ *first = this ? child : 0;
    if (geom) {
        int n = nVert;
        void *p = apRealloc(geom->vertNormal, n * sizeof(m3dV));
        if (!p && n > 0)
            return 0;
        geom->vertNormal = (m3dV *)p;
    }
    BOOL32 started = 0;
    for (objOBJ *c = first; c && (!started || c != first); c = c->next) {
        if (!c->AllocVertNormalList())
            return 0;
        started = 1;
    }
    return 1;
}

// v12 0x004CF900  (feb 0x004BDE50)
BOOL32 objOBJ::_AllocVertNormalList(int n)   { OBJ_REALLOC_LIST(vertNormal, n, sizeof(m3dV)); }

// v12 0x004CF950  (feb 0x004BDEA0)
void objOBJ::_FreeVertNormalList()
{
    if (!geom)
        return;
    apFree(geom->vertNormal);
    geom->vertNormal = 0;
}

// v12 0x004CF980  (feb 0x004BDED0)
BOOL32 objOBJ::_AllocNormalListLCS(int n)    { OBJ_REALLOC_LIST(faceNormalLCS, n, sizeof(m3dV)); }

// v12 0x004CF9D0  (feb 0x004BDF20)
void objOBJ::_FreeNormalListLCS()
{
    if (!geom)
        return;
    if (!(allocFlags & OBJ_SHARE_NORM_LCS))
        apFree(geom->faceNormalLCS);
    geom->faceNormalLCS = 0;
    allocFlags &= ~OBJ_SHARE_NORM_LCS;
}

// v12 0x004CFA00  (feb 0x004BDF50)
BOOL32 objOBJ::_AllocVertNormalListLCS(int n){ OBJ_REALLOC_LIST(vertNormalLCS, n, sizeof(m3dV)); }

// v12 0x004CFA50  (feb 0x004BDFA0)
void objOBJ::_FreeVertNormalListLCS()
{
    if (!geom)
        return;
    if (!(allocFlags & OBJ_SHARE_VNORM_LCS))
        apFree(geom->vertNormalLCS);
    geom->vertNormalLCS = 0;
    allocFlags &= ~OBJ_SHARE_VNORM_LCS;
}

// v12 0x004CFA80  (feb 0x004BDFD0)  'bytes' bytes, all zeroed
BOOL32 objOBJ::_AllocStateFaceList(int bytes)
{
    if (!geom)
        return 0;
    u8 *p = (u8 *)apRealloc(geom->stateFace, bytes);
    if (!p && bytes > 0)
        return 0;
    memset(p, 0, bytes);
    geom->stateFace = p;
    return 1;
}

// v12 0x004CFAE0  (feb 0x004BE030)
void objOBJ::_FreeStateFaceList()
{
    if (!geom)
        return;
    apFree(geom->stateFace);
    geom->stateFace = 0;
}

// v12 0x004CFB10  (feb 0x004BE060)
BOOL32 objOBJ::_AllocStateVertList(int bytes)
{
    if (!geom)
        return 0;
    u8 *p = (u8 *)apRealloc(geom->stateVert, bytes);
    if (!p && bytes > 0)
        return 0;
    memset(p, 0, bytes);
    geom->stateVert = p;
    return 1;
}

// v12 0x004CFB70  (feb 0x004BE0C0)
void objOBJ::_FreeStateVertList()
{
    if (!geom)
        return;
    apFree(geom->stateVert);
    geom->stateVert = 0;
}
