// cgm_wed_algo.cpp -- Will Rock 1.2 (v12.exe): graph algorithms on cgmWED (DFS components,
// Dijkstra shortest paths, boundary extraction)
//
// Reconstructed from the Feb 2003 build (m3d:cgm_wed_algo.obj); every function is byte-identical in 1.2.
// v12 range 0x0057E760 .. 0x0057EBBF (feb 0x00565E40 .. 0x0056629F).

#include "cgm.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0057E9B0 (feb 0x00566090) -> v12 0x009AED90 (feb 0x00963A40)

cgmQUERY *cgmWED_SYS::pQueryDefault;   // v12 0x009AED94 (feb 0x00963A44)

static const float WED_BIG = M3D_INFINITY;   // v12 0x00609B44 (feb 0x005DE658) initial Dijkstra distance


// v12 0x0057E760  (feb 0x00565E40)
// Labels connected components (node->component); q->ProcessNode(0, root) may veto a root.
// Returns the number of components found.
int cgmWED_SYS::DepthFirstSearch(cgmQUERY *q)
{
    ClearState();
    int comp = 0;
    for (int i = 0; i < nNode; i++) {
        cgmWED_NODE *n = &node[i];
        if (!(n->flags & CGM_WED_VISITED)) {
            n->flags |= CGM_WED_VISITED;
            if (q->ProcessNode(0, n)) {
                DepthSearch(n, comp, q);
                comp++;
            }
        }
    }
    return comp;
}


// v12 0x0057E7E0  (feb 0x00565EC0)
// Dijkstra over the node set (all nodes when nSet == 0 or set == 0) from the start nodes
// (initial distances startDist[] or 0). Results in node->dist / node->prev. Stops when
// q->ProcessNode returns 0 or the nearest open node is farther than 0.9 * WED_BIG.
// ? As compiled, the "in set" test of the relaxation reads the flags of the LAST START NODE
//   (the variable is reused), not of the neighbour; with nStart == 0 it reads *set.
void cgmWED_SYS::Dijkstra(int nSet, int *set, int nStart, int *start, float *startDist, cgmQUERY *q)
{
    ClearState();
    int *ids = set;
    if (nSet == 0 || set == 0) {
        nSet = nNode;
        ids = 0;
    }
    for (int i = 0; i < nSet; i++) {
        nodeList[i] = ids ? &node[ids[i]] : &node[i];
        nodeList[i]->dist = WED_BIG;
        nodeList[i]->flags |= CGM_WED_IN_SET;
    }
    unsigned *setCheck = (unsigned *)set;            // sic: see header comment
    for (int i = 0; i < nStart; i++) {
        cgmWED_NODE *s = &node[start[i]];
        setCheck = &s->flags;
        s->prev = 0;
        s->dist = startDist ? startDist[i] : 0.0f;
        s->flags |= CGM_WED_START;
        if (!q->ProcessNode(0, s))
            return;
    }
    for (;;) {
        cgmWED_NODE *best = 0;
        float bestD = WED_BIG;
        if (nNode < 1)
            return;
        for (int i = 0; i < nNode; i++) {            // ? walks nNode entries even when nSet < nNode
            cgmWED_NODE *n = nodeList[i];
            if (!(n->flags & CGM_WED_VISITED) && n->dist < bestD) {
                bestD = n->dist;
                best = n;
            }
        }
        if (!best)
            return;
        if (WED_BIG * 0.9f < bestD)                  // __real@3f666666
            return;
        best->flags |= CGM_WED_VISITED;
        if (!q->ProcessNode(0, best))
            return;
        for (int k = 0; k < best->nEdge; k++) {
            cgmWED_EDGE *e = best->edge[k];
            cgmWED_NODE *o = best->IncidentNode(e);
            if ((*setCheck & CGM_WED_IN_SET) && !(o->flags & CGM_WED_VISITED) && q->ProcessEdge(best, e)) {
                float d = CalcDist(e) + best->dist;    // virtual (slot +0x20)
                if (d < o->dist) {
                    o->dist = d;
                    o->prev = best;
                }
            }
        }
    }
}


// v12 0x0057E9D0  (feb 0x005660B0)
// Marks boundary edges (and their nodes, and faces touching them) with CGM_WED_BOUNDARY:
// useSelection == 0: edges with exactly one face; otherwise edges between a selected
// (CGM_WED_SELECTED) and an unselected / missing face.
void cgmWED_SYS::CalcBoundary(int useSelection)
{
    for (int i = 0; i < nEdge; i++) edge[i].flags &= ~(unsigned)CGM_WED_BOUNDARY;
    for (int i = 0; i < nFace; i++) face[i].flags &= ~(unsigned)CGM_WED_BOUNDARY;
    for (int i = 0; i < nNode; i++) node[i].flags &= ~(unsigned)CGM_WED_BOUNDARY;
    for (int i = 0; i < nEdge; i++) {
        cgmWED_EDGE *e = &edge[i];
        bool mark;
        if (!useSelection) {
            mark = e->face[0] != 0 && e->face[1] == 0;
        } else {
            bool s0 = e->face[0] && (e->face[0]->flags & CGM_WED_SELECTED);
            bool s1 = e->face[1] && (e->face[1]->flags & CGM_WED_SELECTED);
            mark = s0 != s1;
        }
        if (mark) {
            e->flags |= CGM_WED_BOUNDARY;
            e->node[0]->flags |= CGM_WED_BOUNDARY;
            e->node[1]->flags |= CGM_WED_BOUNDARY;
        }
    }
    for (int i = 0; i < nFace; i++) {
        cgmWED_FACE *f = &face[i];
        for (int k = 0; k < f->nEdge; k++)
            if (f->edge[k]->flags & CGM_WED_BOUNDARY)
                f->flags |= CGM_WED_BOUNDARY;
    }
}


// v12 0x0057EAF0  (feb 0x005661D0)
// Recursive flood fill of one component.
void cgmWED_SYS::DepthSearch(cgmWED_NODE *n, int component, cgmQUERY *q)
{
    n->component = component;
    for (int k = 0; k < n->nEdge; k++) {
        cgmWED_EDGE *e = n->edge[k];
        cgmWED_NODE *o = n->IncidentNode(e);
        if (!(o->flags & CGM_WED_VISITED) && q->ProcessEdge(n, e)) {
            o->flags |= CGM_WED_VISITED;
            if (q->ProcessNode(e, o))
                DepthSearch(o, component, q);
        }
    }
}


// v12 0x0057EB80  (feb 0x00566260)
// Searches may run along boundary edges and between interior nodes, but not step from the
// interior onto the boundary (or off it) through an interior edge.
BOOL32 cgmQUERY_BOUNDARY::ProcessEdge(cgmWED_NODE *n, cgmWED_EDGE *e)
{
    cgmWED_NODE *o = n->IncidentNode(e);
    if (e->flags & CGM_WED_BOUNDARY)
        return 1;
    if (!(n->flags & CGM_WED_BOUNDARY) && !(o->flags & CGM_WED_BOUNDARY))
        return 1;
    return 0;
}
