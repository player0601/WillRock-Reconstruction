// cgm_wed.cpp -- Will Rock 1.2 (v12.exe): cgmWED winged-edge mesh graph (construction)
//
// Reconstructed from the Feb 2003 build (m3d:cgm_wed.obj); every function is identical in 1.2.
// v12 range 0x00573B30 .. 0x0057456F (feb 0x0055B560 .. 0x0055BF9F).

#include "cgm.h"
#include "engine_core/object.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00574060 (feb 0x0055BA90) -> v12 0x009AEAF4 (feb 0x009637A4)

// _$E8: v12 0x00573B30 (feb 0x0055B560) -- dynamic initializer of the default query object
// (writes the cgmQUERY vtable v12 0x00609AE0 into it).
static cgmQUERY _cgmQueryDefault;   // v12 0x009AEAF8 (feb 0x009637A8)

// grow / shrink policy of Realloc: step = max(n/100, 100); shrink to n + step/2 when the capacity
// exceeds n + step, grow to n + step when too small.
static inline int _cgmNewCap(int cap, int n)
{
    int step = n / 100;
    if (step < 101)
        step = 100;
    if (step + n < cap)
        return step / 2 + n;
    if (cap < n)
        return step + n;
    return cap;
}


// v12 0x00573B40  (feb 0x0055B570)
// Builds the graph of an object's triangle mesh.
BOOL32 cgmWED::Init(objOBJ *o)
{
    obj = o;
    return cgmWED_SYS::Init();
}


// v12 0x00573B50  (feb 0x0055B580)
// Face count of the object.
int cgmWED::MeshGetNFace()
{
    return obj->nFace;
}


// v12 0x00573B60  (feb 0x0055B590)
// Vertex count of the object.
int cgmWED::MeshGetNVert()
{
    return obj->nVert;
}


// v12 0x00573B70  (feb 0x0055B5A0)
// Object meshes are triangle lists.
int cgmWED::MeshGetFaceNVert(int face)
{
    return 3;
}


// v12 0x00573B80  (feb 0x0055B5B0)
// k-th vertex index of a face (u16 at +2 of the face record). Out-of-range indices are clamped to
// nVert-1 AND written back into the mesh. ? the test is '>' so index == nVert passes unchanged.
int cgmWED::MeshGetFaceVertInd(int face, int k)
{
    objGEOM *g = obj->geom;
    unsigned short *vi = (unsigned short *)((char *)g->faceList + g->faceSize * face + 2) + k;
    int i = *vi;
    if (i > obj->nVert) {
        i = obj->nVert - 1;
        *vi = (unsigned short)i;
    }
    return i;
}


// v12 0x00573BD0  (feb 0x0055B600)
// Object name.
char *cgmWED::MeshGetName()
{
    return obj->name;
}


// v12 0x00573BE0  (feb 0x0055B610)
// Empty graph; installs the default query on first use.
cgmWED_SYS::cgmWED_SYS()
{
    nNode = 0;
    node  = 0;
    nEdge = 0;
    edge  = 0;
    nFace = 0;
    face  = 0;
    nodeList = 0;
    faceList = 0;
    nodeCap  = 0;
    edgeCap  = 0;
    faceCap  = 0;
    if (!pQueryDefault)
        pQueryDefault = &_cgmQueryDefault;
    faceEdgePool = 0;
    nodeEdgePool = 0;
}


// v12 0x00573C30  (feb 0x0055B660)
// Builds nodes, edges and faces from the Mesh* virtuals. On failure calls Term() and returns 0.
BOOL32 cgmWED_SYS::Init()
{
    int nFaceEdges = 0;
    for (int f = 0; f < MeshGetNFace(); f++)
        nFaceEdges += MeshGetFaceNVert(f);
    int nf = MeshGetNFace();
    if (!Realloc(MeshGetNVert(), nFaceEdges, nf))
        goto fail;
    {
        nNode = MeshGetNVert();
        nFace = MeshGetNFace();
        int faceVertTotal = 0;
        unsigned nodeEdgeTotal = 0;
        for (int i = 0; i < nNode * 9; i++) ((int *)node)[i] = 0;          // nNode * 0x24
        for (int i = 0; i < nFaceEdges * 9; i++) ((int *)edge)[i] = 0;     // capacity for every face edge
        for (int i = 0; i < (nFace * 0x1C) / 4; i++) ((int *)face)[i] = 0;
        for (int f = 0; f < nFace; f++) {
            for (int k = 0; k < MeshGetFaceNVert(f); k++) {
                int v = MeshGetFaceVertInd(f, k);
                nodeEdgeTotal += 2;
                node[v].nEdge += 2;                  // upper bound of incident edges
            }
            faceVertTotal += MeshGetFaceNVert(f);
        }
        nodeEdgePool = (cgmWED_EDGE **)apCalloc(nodeEdgeTotal, 4);
        if (!nodeEdgePool)
            return 0;
        int acc = 0;
        for (int i = 0; i < nNode; i++) {
            node[i].index = i;
            node[i].edge  = nodeEdgePool + acc;
            acc += node[i].nEdge;
            node[i].nEdge = 0;
        }
        faceEdgePool = (cgmWED_EDGE **)apCalloc(faceVertTotal, 4);
        if (!faceEdgePool)
            return 0;
        int off = 0;
        for (int f = 0; f < nFace; f++) {
            cgmWED_FACE *pf = &face[f];
            pf->index = f;
            pf->edge  = faceEdgePool + off;
            int nv = MeshGetFaceNVert(f);
            if (!AddFaceEdge(pf, MeshGetFaceVertInd(f, nv - 1), MeshGetFaceVertInd(f, 0)))
                goto fail;
            for (int k = 1; k < nv; k++)
                if (!AddFaceEdge(pf, MeshGetFaceVertInd(f, k - 1), MeshGetFaceVertInd(f, k)))
                    goto fail;
            off += nv;
        }
        if (!Realloc(nNode, nEdge, nFace))           // shrink to the real edge count
            goto fail;
        nodeList = (cgmWED_NODE **)apRealloc(nodeList, nNode << 2);
        if (!nodeList)
            goto fail;
        faceList = (void **)apRealloc(faceList, nFace << 2);
        if (!faceList)
            goto fail;
        // consistency walk (debug leftover): every edge must be listed by both of its nodes;
        // the result is 1 whether or not it is.
        for (int e = 0; e < nEdge; e++) {
            cgmWED_EDGE *pe = &edge[e];
            for (int s = 0; s < 2; s++) {
                cgmWED_NODE *pn = pe->node[s];
                if (pn->nEdge < 1)
                    return 1;
                int k = 0;
                while (pn->edge[k] != pe)
                    if (++k >= pn->nEdge)
                        return 1;
            }
        }
        return 1;
    }
fail:
    Term();                                          // virtual (slot +0x08)
    return 0;
}


// v12 0x00573F60  (feb 0x0055B990)
// Frees everything.
void cgmWED_SYS::Term()
{
    for (int e = 0; e < nEdge; e++)
        apFree(edge[e].extraFace);
    apFree(edge);
    edge  = 0;
    nEdge = 0;
    apFree(node);
    node  = 0;
    nNode = 0;
    apFree(nodeList);
    apFree(faceList);
    nodeList = 0;
    faceList = 0;
    apFree(face);
    face  = 0;
    nFace = 0;
    apFree(faceEdgePool);
    apFree(nodeEdgePool);
    faceEdgePool = 0;
    nodeEdgePool = 0;
    nodeCap = 0;
    edgeCap = 0;
    faceCap = 0;
}


// v12 0x00574000  (feb 0x0055BA30)
// The other end of edge e.
cgmWED_NODE *cgmWED_NODE::IncidentNode(cgmWED_EDGE *e)
{
    cgmWED_NODE *o = e->node[0];
    if (o == this)
        o = e->node[1];
    return o;
}


// v12 0x00574020  (feb 0x0055BA50)
// The edge joining this node to n, or 0.
cgmWED_EDGE *cgmWED_NODE::IsIncidentNode(cgmWED_NODE *n)
{
    for (int i = 0; i < nEdge; i++) {
        cgmWED_EDGE *e = edge[i];
        cgmWED_NODE *o = e->node[0];
        if (o == this)
            o = e->node[1];
        if (o == n)
            return e;
    }
    return 0;
}


// v12 0x00574080  (feb 0x0055BAB0)
// Resizes the node / edge / face arrays (see _cgmNewCap) and rebases every pointer into them.
// ? on a failed apRealloc the already moved arrays are not stored back (as compiled).
BOOL32 cgmWED_SYS::Realloc(int wantNode, int wantEdge, int wantFace)
{
    int capN = _cgmNewCap(nodeCap, wantNode);
    int capE = _cgmNewCap(edgeCap, wantEdge);
    int capF = _cgmNewCap(faceCap, wantFace);

    cgmWED_NODE *nn;
    if (capN == nodeCap) {
        nn = node;
    } else {
        nn = (cgmWED_NODE *)apRealloc(node, capN * 0x24);
        if (!nn) return 0;
        nodeCap = capN;
    }
    cgmWED_FACE *nf;
    if (capF == faceCap) {
        nf = face;
    } else {
        nf = (cgmWED_FACE *)apRealloc(face, capF * 0x1C);
        if (!nf) return 0;
        faceCap = capF;
    }
    cgmWED_EDGE *ne;
    if (capE == edgeCap) {
        ne = edge;
    } else {
        ne = (cgmWED_EDGE *)apRealloc(edge, capE * 0x24);
        if (!ne) return 0;
        edgeCap = capE;
    }
    if (ne != edge) {                                // rebase edge pointers held by nodes and faces
        for (int i = 0; i < nNode; i++)
            for (int k = 0; k < nn[i].nEdge; k++)
                nn[i].edge[k] = (cgmWED_EDGE *)((char *)ne + ((char *)nn[i].edge[k] - (char *)edge));
        for (int i = 0; i < nFace; i++)
            for (int k = 0; k < nf[i].nEdge; k++)
                nf[i].edge[k] = (cgmWED_EDGE *)((char *)ne + ((char *)nf[i].edge[k] - (char *)edge));
        edge = ne;
    }
    if (nn != node) {                                // rebase node pointers held by edges
        for (int i = 0; i < nEdge; i++) {
            ne[i].node[0] = (cgmWED_NODE *)((char *)ne[i].node[0] + ((char *)nn - (char *)node));
            ne[i].node[1] = (cgmWED_NODE *)((char *)ne[i].node[1] + ((char *)nn - (char *)node));
        }
        node = nn;
    }
    if (nf != face) {                                // rebase face pointers held by edges
        for (int i = 0; i < nEdge; i++) {
            if (ne[i].face[0])
                ne[i].face[0] = (cgmWED_FACE *)(((char *)ne[i].face[0] - (char *)face) + (char *)nf);
            if (ne[i].face[1])
                ne[i].face[1] = (cgmWED_FACE *)(((char *)ne[i].face[1] - (char *)face) + (char *)nf);
        }
        face = nf;
    }
    return 1;
}


// v12 0x005742D0  (feb 0x0055BD00)
// Adds the edge v0->v1 of face f: reuses an existing edge between the two vertices (second face, or
// extra-face list beyond two) or creates a new one; appends it to the face's edge list.
BOOL32 cgmWED_SYS::AddFaceEdge(cgmWED_FACE *f, int v0, int v1)
{
    cgmWED_NODE *a = &node[v0];
    cgmWED_NODE *b = &node[v1];
    cgmWED_EDGE *e = 0;
    for (int i = 0; i < a->nEdge; i++) {
        cgmWED_EDGE *c = a->edge[i];
        cgmWED_NODE *o = c->node[0];
        if (o == a)
            o = c->node[1];
        if (o->index == b->index) {
            e = a->edge[i];
            if (e->face[1] == 0) {
                e->face[1] = f;
                if (a == e->node[0])
                    e->flags |= CGM_WED_FLIPPED;
            } else {
                e->flags |= CGM_WED_MULTI;
                e->extraFace = (cgmWED_FACE **)apRealloc(e->extraFace, e->nExtraFace * 4 + 4);
                if (!e->extraFace)
                    return 0;
                e->extraFace[e->nExtraFace] = f;
                e->nExtraFace++;
            }
            break;
        }
    }
    if (!e) {
        e = &edge[nEdge];
        e->index = nEdge;
        nEdge++;
        e->node[0] = a;
        e->node[1] = b;
        e->face[0] = f;
        e->face[1] = 0;
        e->flags   = 0;
        a->edge[a->nEdge++] = e;
        b->edge[b->nEdge++] = e;
    }
    f->edge[f->nEdge++] = e;
    return 1;
}


// v12 0x005743F0  (feb 0x0055BE20)
// Clears the algorithm marks (nodes: bits 0..2, faces and edges: bit 0).
void cgmWED_SYS::ClearState()
{
    for (int i = 0; i < nNode; i++) {
        node[i].flags &= ~1u;
        node[i].flags &= ~2u;
        node[i].flags &= ~4u;
    }
    for (int i = 0; i < nFace; i++)
        face[i].flags &= ~1u;
    for (int i = 0; i < nEdge; i++)
        edge[i].flags &= ~1u;
}


// v12 0x00574480  (feb 0x0055BEB0)
// Edge length from the vertex positions (M3D_INFINITY without positions).
float cgmWED_3D::CalcDist(cgmWED_EDGE *e)
{
    if (!vert)
        return M3D_INFINITY;                                       // v12 0x00609ADC
    m3dV *p0 = &vert[e->node[0]->index];
    m3dV *p1 = &vert[e->node[1]->index];
    float dx = p0->x - p1->x, dy = p0->y - p1->y, dz = p0->z - p1->z;
    return m3dSqrt(dx * dx + dy * dy + dz * dz);
}
