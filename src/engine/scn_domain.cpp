// scn_domain.cpp -- engine.lib scnDOMAIN: vertical volumes outlined by a flat object
// (Will Rock 1.2, v12.exe; feb obj engine:scn_domain.obj)
//
// v12 0x004EFB10..0x004F0E7F, all "exact" except Init (only call targets moved). A domain is the XZ outline
// of an object (each face becomes a 2D triangle; the object should be planar, else "Domain %s is not
// planar." is logged) between yMin and yMax. yMin == yMax means "no top": IsBelongPoint then only rejects
// points more than eps below it, and the bounding box spans +-200000 in Y. Water and fog volumes
// (pteFOG_GROUND: yMin = surface - depth), AI areas and trigger domains are domains.

#include "engine.h"

#define DOM_EPS2D   1e-6f              // v12 0x0060775C 2D point-in-triangle tolerance
#define DOM_EPSH    1e-4f              // v12 0x00607764 "yMin == yMax"
#define DOM_BIG     2000000.0f         // v12 0x00607760
#define DOM_PLANAR  0.001f             // v12 0x005FB874

static inline m3dBOX *domBox(scnDOMAIN *d)
{
    return (m3dBOX *)&d->bbox;
}

// A vertex of o in world space (objects with state bit 0 are already in world space).
static inline void domVert(objOBJ *o, int i, m3dV *out)
{
    m3dV *v = &o->geom->vert[i];
    if (!(o->state & 1))
        m3dTransformPoint(v, (m3dMATR *)o->ltm, out);
    else
        *out = *v;
}

// v12 0x00548C40  (feb 0x00535300, a copy of the inline constructor in ai_ctrl.obj)
scnDOMAIN::scnDOMAIN()
{
    nTri = 0;
    yMin = 0;
    yMax = 0;
    bbox.type = 1;
    bbox.vtbl = (void *)m3dBOX_vftable;
    tri = 0;
    obj = 0;
    inside = 0;
}

// v12 0x0043A2D0  (feb ?) the deleting destructor (folded with aiAREA's): resets the vtables
void *scnDOMAIN::__vecDelDtor(unsigned flags)
{
    bbox.vtbl = (void *)m3dVOL_vftable;
    if (flags & 1)
        operator delete(this);
    return this;
}

// v12 0x004EFB10  (feb 0x004DC1B0)
// The outline from obj's faces in world space; both heights = the plane's height (the first vertex).
BOOL32 scnDOMAIN::Init()
{
    if (obj->nFace <= 0)
        return 1;
    tri = (m2dTRI *)apCalloc(obj->nFace, sizeof(m2dTRI));
    if (!tri)
        return 0;
    m3dV w;
    domVert(obj, 0, &w);
    float y = w.y;
    for (int i = 1; i < obj->nVert; i++) {
        domVert(obj, i, &w);
        if (!(fabsf(w.y - y) < DOM_PLANAR)) {
            apLog((char *)"Domain %s is not planar.", obj->name);
            break;
        }
    }
    for (int f = 0; f < obj->nFace; f++) {
        m2dPOLY p;
        p.nVert = 3;
        objFACE *fc = obj->GetFace(f);
        for (int k = 0; k < 3; k++) {
            domVert(obj, fc->vert[k], &w);
            p.vert[k].x = w.x;
            p.vert[k].y = w.z;
        }
        m2dReOrientPoly(&p);
        for (int k = 0; k < 3; k++)
            tri[f].v[k] = p.vert[k];
    }
    yMin = y;
    CalcBBox();
    yMax = y;
    CalcBBox();
    nTri = obj->nFace;
    CalcBBox();
    return 1;
}

// v12 0x004F01D0  (feb 0x004DC870)
void scnDOMAIN::Term()
{
    if (tri)
        apFree(tri);
    tri = 0;
    nTri = 0;
    obj = 0;
}

// v12 0x004F0200  (feb 0x004DC8A0)
// Heights from the object's plane: [plane, plane + h] for h > 0, [plane + h, plane] otherwise. Without
// an outline: Y unlimited (+-200000).
void scnDOMAIN::SetHeight(float h)
{
    float top;
    if (!nTri) {
        yMin = DOM_BIG * -0.1f;
        CalcBBox();
        top = DOM_BIG * 0.1f;
    } else {
        m3dV w;
        domVert(obj, 0, &w);
        if (h <= 0) {
            yMin = w.y + h;
            CalcBBox();
            top = w.y;
        } else {
            yMin = w.y;
            CalcBBox();
            top = w.y + h;
        }
    }
    yMax = top;
    CalcBBox();
    CalcBBox();
}

// v12 0x004F0430  (feb 0x004DCAD0)
BOOL32 scnDOMAIN::IsBelongPoint(m3dV *p)
{
    if (!nTri)
        return 1;
    if (!(fabsf(yMin - yMax) < DOM_EPSH)) {
        if (yMax < p->y || p->y < yMin)
            return 0;
    } else if (p->y < yMin - DOM_EPS2D) {
        return 0;
    }
    if (bbox.min[0] <= p->x && p->x <= bbox.max[0] && bbox.min[2] <= p->z && p->z <= bbox.max[2]) {
        m2dV q = { p->x, p->z };
        for (int i = 0; i < obj->nFace; i++)
            if (m2dIsBelongPointTri_A(&tri[i], &q, DOM_EPS2D))
                return 1;
    }
    return 0;
}

// v12 0x004F0560  (feb 0x004DCC00)
BOOL32 scnDOMAIN::IsBelongPointXZ(m3dV *p)
{
    if (!nTri)
        return 1;
    if (bbox.min[0] <= p->x && p->x <= bbox.max[0] && bbox.min[2] <= p->z && p->z <= bbox.max[2]) {
        m2dV q = { p->x, p->z };
        for (int i = 0; i < obj->nFace; i++)
            if (m2dIsBelongPointTri_A(&tri[i], &q, DOM_EPS2D))
                return 1;
    }
    return 0;
}

// v12 0x004F0610  (feb 0x004DCCB0)
// Does the XZ segment a-b cross the edge of one of the outline's triangles.
BOOL32 scnDOMAIN::IsLSegCrossBoundaryXZ(m3dV *a, m3dV *b)
{
    if (!obj)
        return 0;
    m2dV a2 = { a->x, a->z }, b2 = { b->x, b->z };
    for (int i = 0; i < obj->nFace; i++) {
        m2dPOLY p;
        for (int k = 0; k < 3; k++)
            p.vert[k] = tri[i].v[k];
        p.nVert = 3;
        if (m2dIsCrossLSegPolyBoundary_A(&p, &a2, &b2, DOM_EPS2D))
            return 1;
    }
    return 0;
}

// v12 0x00502560  (feb 0x004DCD70) identical-code-folded "return 0" (shared with cdtCOLL_OBJ::GetID2BBox ...)
BOOL32 scnDOMAIN::ISectLSegBoundaryXZ(m3dV *a, m3dV *b, m3dV *out)
{
    return 0;
}

// v12 0x004F06D0  (feb 0x004DCD80)
// Touching domain: a corner of b inside the outline (XZ); 'inside' domain: all corners inside (with heights).
BOOL32 scnDOMAIN::IsIsectBox(m3dBOX *b)
{
    if (domBox(this)->IsIsectBox(b) == 1)
        return 0;
    m3dV c[8];
    b->GetVertices(c);
    if (!inside) {
        for (int i = 0; i < 8; i++)
            if (IsBelongPointXZ(&c[i]))
                return 1;
        return 0;
    }
    for (int i = 0; i < 8; i++)
        if (!IsBelongPoint(&c[i]))
            return 0;
    return 1;
}

// v12 0x004F0750  (feb 0x004DCE00)
// Touching domain: the object's box overlaps and one of its world vertices is inside; 'inside' domain:
// every vertex inside (an object without vertices: no).
BOOL32 scnDOMAIN::_IsIsectObj(objOBJ *o)
{
    if (!o)
        return 0;
    m3dV w;
    if (!inside) {
        if (!nTri)
            return 1;
        m3dBOX_DATA b;
        b.vtbl = (void *)m3dBOX_vftable;
        b.type = 1;
        if (!o->inst) {
            b.type = o->bbox.type;
            for (int k = 0; k < 3; k++) {
                b.min[k] = o->bbox.min[k];
                b.max[k] = o->bbox.max[k];
            }
        } else {
            o->inst->GetBBox((m3dBOX *)&b);
        }
        if (domBox(this)->IsIsectBox((m3dBOX *)&b) == 1)
            return 0;
        for (int i = 0; i < o->nVert; i++) {
            domVert(o, i, &w);
            if (IsBelongPoint(&w))
                return 1;
        }
        return 0;
    }
    if (!o->nVert)
        return 0;
    for (int i = 0; i < o->nVert; i++) {
        domVert(o, i, &w);
        if (!IsBelongPoint(&w))
            return 0;
    }
    return 1;
}

// v12 0x004F0BD0  (feb 0x004DD280)
// o, else one of its children's subtrees.
BOOL32 scnDOMAIN::IsIsectObj(objOBJ *o)
{
    objOBJ *first = o ? o->child : 0;
    if (_IsIsectObj(o))
        return 1;
    for (objOBJ *c = first; c; c = c->next == first ? 0 : c->next)
        if (IsIsectObj(c))
            return 1;
    return 0;
}

// v12 0x004F0C60  (feb 0x004DD310)
BOOL32 scnDOMAIN::IsIsectInst(animINST *i)
{
    if (!i)
        return 0;
    return _IsIsectInst(i);                             // (a tail call through the vtable)
}

// v12 0x004F0C80  (feb 0x004DD330)
BOOL32 scnDOMAIN::_IsIsectInst(animINST *i)
{
    if (!i)
        return 0;
    m3dV p;
    i->GetPos(&p);
    return IsBelongPoint(&p);
}

// v12 0x004F0CC0  (feb 0x004DD370)
// XZ from the triangles (at yMin), Y = [yMin, yMax], or +-200000 when they are equal.
void scnDOMAIN::CalcBBox()
{
    m3dBOX *b = domBox(this);
    b->CalcStart();
    for (int i = 0; i < nTri; i++)
        for (int k = 0; k < 3; k++) {
            m3dV p = { tri[i].v[k].x, yMin, tri[i].v[k].y };
            b->CalcUpdate(&p);
        }
    if (fabsf(yMin - yMax) < DOM_EPSH) {
        bbox.min[1] = DOM_BIG * -0.1f;
        bbox.max[1] = DOM_BIG * 0.1f;
        return;
    }
    bbox.min[1] = yMin;
    bbox.max[1] = yMax;
}

// v12 0x004F0DA0  (feb 0x004DD450)
// A random point: a random triangle, a random point in it, a random height between the limits.
void scnDOMAIN::GetRandomPoint(m3dV *out)
{
    int i = (int)m3dRandMax((float)nTri);
    float u, v;
    m3dRandBasicCoord(&u, &v);
    m2dV q;
    m2dTriApplyBasisCoord(&tri[i], u, v, &q);
    out->x = q.x;
    out->y = m3dRandRange(yMax, yMin);
    out->z = q.y;
}

// v12 0x004F0E20  (feb 0x004DD4D0)
void scnDOMAIN::GetPoly(int i, m2dPOLY *out)
{
    for (int k = 0; k < 3; k++)
        out->vert[k] = tri[i].v[k];
    out->nVert = 3;
}
