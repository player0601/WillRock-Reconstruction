// Object.cpp -- engine.lib objOBJ basics: construction, hierarchy links, duplication, world matrices and
// bounding volumes (Will Rock 1.2, v12.exe; feb obj engine:Object.obj)
//
// v12 0x004DBEC0..0x004DCEEF. All "exact" except Attach (1.2 adds a null check of this), the three
// matrix accessors (tail calls into m3dMATR) and objCalcBSphereCenter (same code, other register use).
// One shared growable vertex buffer (dsDYN_ARRAY of m3dV) serves TransformPoints and the box/prism
// calculations; GetTransformedVertList returns it.

#include "engine.h"

// v12 0x004DBEC0  (feb 0x004BF160) _$E15: static construction of the shared buffer (inline ctor) + atexit dtor
static dsDYN_ARRAY _objTransVert;      // v12 0x00978670 (feb 0x008B6F68)
static int         _objTransVertNmb;   // v12 0x0097866C (feb 0x008B6F64) TransformPoints(.., 0) calls

// v12 0x004DBF10  (feb 0x004BF1B0)
BOOL32 __fastcall objInit()
{
    return _objTransVert.Init(sizeof(m3dV), 100, 0, 0, 0) != 0;
}

// v12 0x004DBF30  (feb 0x004BF1D0)
void __fastcall objTerm()
{
    _objTransVert.ReleaseMem();
}

// v12 0x004DBF40  (feb 0x004BF1E0)
objOBJ::objOBJ()
{
    allocFlags = 0;
    bbox.type = 1;
    bbox.vtbl = (void *)m3dBOX_vftable;
    invalidMask = 0;
    invalidAdd = 0;
    state = 0;
    allocFlags = 0;
    procState = 0;
    rendState = 0;
    unk0C = 0;
    idHier = 0;
    name = 0;
    nFace = 0;
    nVert = 0;
    parent = 0;
    child = 0;
    inst = 0;
    unk1E = -1;
    unk20 = -1;
    unk22 = -1;
    geom = 0;
    invalidMask = 0;
    invalidAdd = 0;
    usrData = 0;
    unkEC = -1;
    unkEE = 0;
    next = this;
    prev = this;
    ((m3dMATR *)local)->Identity();
    ((m3dMATR *)ltm)->Identity();
    props = 0;
    lod = 0;
}

// v12 0x004DBFF0  (feb 0x004BF290)
// Deletes the children first, then unlinks and frees this object's data.
objOBJ::~objOBJ()
{
    while (child) {
        objOBJ *c = child;
        c->~objOBJ();
        operator delete(c);
    }
    Detach();
    _DestroyData();
    bbox.vtbl = (void *)m3dVOL_vftable;
}

// v12 0x004DC030  (feb 0x004BF2D0)  [callgraph: 1.2 returns at once when this == 0]
// Link as the last child of p (the children are a circular ring; p->child is the first).
void objOBJ::Attach(objOBJ *p)
{
    if (!this)
        return;
    parent = p;
    if (p->child) {
        next = p->child;
        prev = p->child->prev;
        p->child->prev->next = this;
        p->child->prev = this;
    }
    p->child = this;
}

// v12 0x004DC070  (feb 0x004BF300)
void objOBJ::Detach()
{
    if (prev == this) {
        if (parent)
            parent->child = 0;
    } else {
        next->prev = prev;
        prev->next = next;
        if (parent && parent->child == this)
            parent->child = next;
    }
    if (inst) {
        if (idHier >= 0 && idHier < inst->nObj)
            inst->objTable[idHier] = 0;
        if (!parent)
            inst->root = 0;
    }
    prev = this;
    next = this;
    parent = 0;
    inst = 0;
    unk1E = -1;
    unk20 = -1;
    unk22 = -1;
}

// v12 0x004DC120  (feb 0x004BF3B0)
void objOBJ::_DestroyData()
{
    _FreeName();
    _FreeVertList();
    _FreeFaceList();
    for (int i = 0; i < 3; i++)
        _FreeTexCoord(i);
    _FreeColorList();
    _FreeFaceSplit();
    if (!(allocFlags & 0x40)) {                         // props are the template's when shared
        if (props) {
            props->Empty();
            operator delete(props);
        }
        props = 0;
    }
    _FreeNormalListLCS();
    _FreeVertNormalListLCS();
    RemoveModifiers();
    DestroyUsrData();
    _FreeNormalList();
    _FreeVertNormalList();
    _FreeStateVertList();
    _FreeStateFaceList();
    _FreeSplitUnshared();
    operator delete(lod);
    lod = 0;
    _FreeGeom();
}

// v12 0x004DC1E0  (feb 0x004BF470)
// Copy of this one node. The mesh lists are shared with the source (allocFlags bits set, so the copy never
// frees them); per-object state lists, the modifier list and the split block are not copied.
// withData != 0: also duplicate the modifiers and user data; else the copy is detached and unshared.
objOBJ *objOBJ::_Duplicate(int withData)
{
    objOBJ *o = Make();
    if (!o)
        return 0;
    o->state = state;
    o->allocFlags = allocFlags;
    o->procState = procState;
    o->unk0C = unk0C;
    o->nFace = nFace;
    o->nVert = nVert;
    o->name = name;
    o->idHier = idHier;
    o->unk1E = unk1E;
    o->unk20 = unk20;
    o->unk22 = unk22;
    o->rendState = rendState;
    o->geom = geom;
    o->parent = parent;
    o->next = next;
    o->prev = prev;
    o->child = child;
    memcpy(o->ltm, ltm, sizeof(ltm));
    memcpy(o->local, local, sizeof(local));
    o->inst = inst;
    o->bbox.type = bbox.type;
    memcpy(o->bbox.min, bbox.min, 12);
    memcpy(o->bbox.max, bbox.max, 12);
    o->invalidMask = invalidMask;
    o->invalidAdd = invalidAdd;
    o->usrData = usrData;
    o->unkEC = unkEC;
    o->unkEE = unkEE;
    o->props = props;
    o->lod = lod;
    o->child = 0;
    o->parent = 0;
    o->next = o;
    o->prev = o;
    o->usrData = 0;
    o->geom = 0;
    o->rendState &= 0xDFFFFFFF;
    o->procState &= 0xFFFFFFC0;
    o->allocFlags |= 0xC0;                              // name and props shared
    if (geom) {
        o->_AllocGeom();
        objGEOM *s = geom, *d = o->geom;
        d->faceList = s->faceList;
        d->faceSize = s->faceSize;
        d->faceFormat = s->faceFormat;
        for (int i = 0; i < 3; i++)
            d->nTexCoord[i] = s->nTexCoord[i];
        for (int i = 0; i < 3; i++)
            d->texCoord[i] = s->texCoord[i];
        d->split = s->split;
        d->splitUnshared = s->splitUnshared;
        d->stateVert = s->stateVert;
        d->stateFace = s->stateFace;
        ((m3dSPHERE *)&d->bsphere)->operator=(*(m3dSPHERE *)&s->bsphere);
        d->vert = s->vert;
        d->faceNormal = s->faceNormal;
        d->vertNormal = s->vertNormal;
        d->color = s->color;
        d->faceNormalLCS = s->faceNormalLCS;
        d->vertNormalLCS = s->vertNormalLCS;
        d->nModifier = s->nModifier;
        d->modifier = s->modifier;
        d->constColor = s->constColor;
        d->unk62 = s->unk62;
        memcpy(d->unk66, s->unk66, 0x1C);               // +0x66..+0x82
        o->allocFlags |= 0x3F;                          // faces, verts, colours, split, LCS normals shared
        *(u32 *)(o->geom->unk66) |= 2;
        for (int i = 0; i < 3; i++)
            if (geom->texCoord[i])
                o->allocFlags |= OBJ_SHARE_TEX0 << i;
        o->geom->stateVert = 0;
        o->geom->stateFace = 0;
        o->geom->vertNormal = 0;
        o->geom->color = 0;
        o->geom->splitUnshared = 0;
        o->geom->nModifier = 0;
        o->geom->modifier = 0;
        if (withData) {
            if (!DuplicateModifiers(o))
                return 0;
            goto usr;
        }
    } else if (withData) {
        goto usr;
    }
    o->inst = 0;
    o->unk1E = -1;
    o->unk20 = -1;
    o->unk22 = -1;
    o->_UnshareAll();
    if (!withData)
        return o;
usr:
    if (DuplicateUsrData(o, withData))
        return o;
    return 0;
}

// v12 0x004DC530  (feb 0x004BF7C0)  type, centre, radius (not the vtable)
m3dSPHERE &m3dSPHERE::operator=(const m3dSPHERE &s)
{
    type = s.type;
    center = s.center;
    radius = s.radius;
    return *this;
}

// v12 0x004DC560  (feb 0x004BF7F0)
// Deep copy of the hierarchy below this object, attached under p (if given).
objOBJ *objOBJ::Duplicate(objOBJ *p)
{
    objOBJ_ITER_SAFE it(this);
    objOBJ *o = _Duplicate(1);
    if (!o)
        return 0;
    if (p) {
        o->parent = p;
        if (p->child) {
            o->next = p->child;
            o->prev = p->child->prev;
            p->child->prev->next = o;
            p->child->prev = o;
        }
        p->child = o;
    }
    for (int i = 0; i < it.n; i++)
        if (!it.obj[i]->Duplicate(o))
            return 0;
    return o;
}

// v12 0x004DC610  (feb 0x004BF8A0)
BOOL32 objOBJ::SetName(char *s)
{
    if (!_AllocName((int)strlen(s)))
        return 0;
    strcpy(name, s);
    return 1;
}

// v12 0x004DC660  (feb 0x004BF8F0)
m3dV *__fastcall objOBJ::GetTransformedVertList()
{
    return *(m3dV **)_objTransVert.ptr;
}

// v12 0x004DC670  (feb 0x004BF900)
int __fastcall objOBJ::GetTransformedVertNmb()
{
    return _objTransVertNmb;
}

// v12 0x004DC680  (feb 0x004BF910)
// ltm = local * parent.ltm (root: local; OBJ_ST_NO_PARENT_XFORM: parent's ltm), then the children.
void __fastcall objCalcLTM(objOBJ *o)
{
    if (!o)
        return;
    objOBJ *p = o->parent, *first = o->child;
    if (!p)
        memcpy(o->ltm, o->local, sizeof(o->ltm));
    else if (!(o->state & OBJ_ST_NO_PARENT_XFORM))
        m3dMultiplyMatr((m3dMATR *)o->local, (m3dMATR *)p->ltm, (m3dMATR *)o->ltm);
    else
        memcpy(o->ltm, p->ltm, sizeof(o->ltm));
    int started = 0;
    for (objOBJ *c = first; c && (!started || c != first); c = c->next) {
        objCalcLTM(c);
        started = 1;
    }
}

// v12 0x004DC700  (feb 0x004BF990)
void objOBJ::ScaleVertList(float x, float y, float z)
{
    m3dMATR m;
    m.Identity();
    m.Scale(x, y, z, 2);
    for (int i = 0; i < nVert; i++) {
        m3dV *v = &geom->vert[i];
        m3dTransformPoint(v, &m, v);
    }
}

// v12 0x004DC8D0  (feb 0x004BFB60)
void objOBJ::GetOrigin(m3dV *out)
{
    ((m3dMATR *)ltm)->GetOrigin(out);
}

// v12 0x004DC8E0  (feb 0x004BFB70)
void objOBJ::GetAxisY(m3dV *out)
{
    ((m3dMATR *)ltm)->GetAxisY(out);
}

// v12 0x004DC8F0  (feb 0x004BFB80)
void objOBJ::GetAxisZ(m3dV *out)
{
    ((m3dMATR *)ltm)->GetAxisZ(out);
}

// v12 0x004DC900  (feb 0x004BFB90)
// World vertices (frozen objects: as stored) into out, or into the shared buffer when out == 0.
// m != 0: transformed by m after the object's matrix.
void objOBJ::TransformPoints(m3dMATR *m, m3dV *out)
{
    m3dMATR t;
    if (!out) {
        _objTransVert.Expand(nVert);
        _objTransVertNmb++;
        out = *(m3dV **)_objTransVert.ptr;
    }
    if (!(state & 1)) {
        if (m) {
            m3dMultiplyMatr((m3dMATR *)ltm, m, &t);
            m = &t;
        } else {
            m = (m3dMATR *)ltm;
        }
    } else if (!m) {
        memcpy(out, geom->vert, nVert * sizeof(m3dV));
        return;
    }
    m->TransformPointList(nVert, geom->vert, sizeof(m3dV), out, sizeof(m3dV));
}

// v12 0x004DC9B0  (feb 0x004BFC40)
void objOBJ::GetTransformedVert(int i, m3dV *out)
{
    *out = (*(m3dV **)_objTransVert.ptr)[i];
}

// v12 0x004DC9E0  (feb 0x004BFC70)
// Grow s->radius to reach every world vertex of the subtree (the centre is given).
void __fastcall objCalcBSphereCenter(objOBJ *o, m3dSPHERE *s)
{
    objOBJ *first = o ? o->child : 0, *c = first;
    int started = 0;
    for (int i = 0; i < o->nVert; i++) {
        m3dV w;
        if (!(o->state & 1))
            m3dTransformPoint(&o->geom->vert[i], (m3dMATR *)o->ltm, &w);
        else
            w = o->geom->vert[i];
        float dx = s->center.x - w.x, dy = s->center.y - w.y, dz = s->center.z - w.z;
        float d = m3dSqrt(dz * dz + dy * dy + dx * dx);
        float r = s->radius;
        s->radius = !(r <= d) ? r : d;
    }
    while (first && (!started || c != first)) {
        objCalcBSphereCenter(c, s);
        c = c->next;
        started = 1;
    }
}

// v12 0x004DCCE0  (feb 0x004BFF70)
void __fastcall objCalcBPrism(objOBJ *o, m3dPRISM *p)
{
    o->TransformPoints(0, 0);
    ((m3dVOL *)p)->Calc(objOBJ::GetTransformedVertList(), o->nVert);
}

// v12 0x004DCD10  (feb 0x004BFFA0)
// World box of each object (its vertices, or the union of its children's boxes; a point box at the origin
// when it has neither). Objects with process state 0x1C hidden (unless 0xF000) are skipped. 1 = has a box.
BOOL32 __fastcall objCalcBBox(objOBJ *o)
{
    if (!o)
        return 0;
    u32 ps = o->procState;
    if ((ps & 0x1C) && !(ps & 0xF000))
        return 0;
    objOBJ *first = o->child, *c = first;
    int have = 0, started = 0;
    if (o->nVert > 0 && (!(ps & 0x3F) || (ps & 0xF000))) {
        o->TransformPoints(0, 0);
        ((m3dBOX *)&o->bbox)->Calc(objOBJ::GetTransformedVertList(), o->nVert, 0.01f);
        have = 1;
    }
    for (;;) {
        if (!first || (started && c == first)) {
            if (!have) {
                m3dV org;
                ((m3dMATR *)o->ltm)->GetOrigin(&org);
                o->bbox.max[0] = org.x; o->bbox.max[1] = org.y; o->bbox.max[2] = org.z;
                o->bbox.min[0] = org.x; o->bbox.min[1] = org.y; o->bbox.min[2] = org.z;
                ((m3dVOL *)&o->bbox)->Expand(0.01f);
            }
            return have;
        }
        started = objCalcBBox(c);
        if (started) {
            if (!have) {
                o->bbox.type = c->bbox.type;
                memcpy(o->bbox.min, c->bbox.min, 12);
                memcpy(o->bbox.max, c->bbox.max, 12);
                have = 1;
            } else {
                ((m3dBOX *)&o->bbox)->CalcUpdate((m3dBOX *)&c->bbox);
            }
        }
        c = c->next;
        started = 1;
    }
}

// v12 0x004DCE80  (feb 0x004C0110)
BOOL32 objSET::Alloc(int m)
{
    obj = (objOBJ **)apCalloc(m, 4);
    if (!obj)
        return 0;
    max = m;
    n = 0;
    return 1;
}

// v12 0x004DCEC0  (feb 0x004C0150)
BOOL32 objSET::AddObj(objOBJ *o)
{
    if (n >= max)
        return 0;
    obj[n++] = o;
    return 1;
}
