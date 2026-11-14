// cgm.h -- computational-geometry helpers of m3d.lib: sorted search lists (cgm_search.obj) and the
// winged-edge mesh graph cgmWED (cgm_wed.obj, cgm_wed_algo.obj). Will Rock 1.2 reconstruction.
// Not in the SDK. Users in the game: objMOD_NOISE_DEFORM (vertex welding via cgmSEARCH_VLIST),
// AI statue logic and shatter effects (cgmWED_3D / cgmQUERY_BOUNDARY), projected shadows (cgmWED).
//
// Vtables are written with an explicit MSVC "scalar deleting destructor" slot (__vecDelDtor) so the
// slot numbers match the game under GCC as well.
#pragma once
#include "m3d_lib.h"

class objOBJ;

// ---------------------------------------------------------------------------------------------
// cgmSEARCH -- index array sorted by a virtual compare; binary search on it
// vtables (feb): cgmSEARCH 0x005DC39C, cgmSEARCH_VLIST 0x005DC3B0, cgmSEARCH_INT_LIST 0x005DC3C4
// ---------------------------------------------------------------------------------------------
enum cgmSEARCH_FLAGS { CGM_SEARCH_OWN_INDEX = 1 };     // index array allocated by Init (freed by Term)
enum cgmSEARCH_MODE  { CGM_SEARCH_LOWER = -1, CGM_SEARCH_UPPER = 1 };

class cgmSEARCH {
public:
    virtual void *__vecDelDtor(unsigned f);                                      // slot +0x00 (feb 0x004E5090)
    virtual BOOL32 Init(int n, int stride, void *data, int *index);              // slot +0x04 v12 0x0057E340
    virtual void   Term();                                                       // slot +0x08 v12 0x0057E450
    virtual int    Search(void *key, int mode);                                  // slot +0x0C v12 0x0057E3D0
    virtual int    CmpItem(void *a, void *b) = 0;                                // slot +0x10
    static int __cdecl QSortCB(const void *a, const void *b);                    // v12 0x0057E700

    unsigned flags;       // +0x04 cgmSEARCH_FLAGS
    int      n;           // +0x08 number of items
    int     *index;       // +0x0C sorted item indices
    int      stride;      // +0x10 bytes per item (-1 after Term)
    int      keyOffset;   // +0x14 offset of the key inside an item
    char    *data;        // +0x18 items
};

class cgmSEARCH_VLIST : public cgmSEARCH {     // float vectors, sorted by projection on 'axis'
public:
    BOOL32 Init(int n, int stride, void *data, int *index);                      // v12 0x0057E480
    void   Term();                                                               // v12 0x0057E5D0
    int    CmpItem(void *a, void *b);                                            // v12 0x0057E620
    int    dim;           // +0x1C number of floats per key (-1 = stride/4)
    m3dV   axis;          // +0x20 sort axis = diagonal of the point cloud's box
    float  tol;           // +0x2C equality tolerance along the axis
};

class cgmSEARCH_INT_LIST : public cgmSEARCH {  // signed 1/2/4-byte integer keys
public:
    int    CmpItem(void *a, void *b);                                            // v12 0x0057E6A0
    int    size;          // +0x1C key size in bytes
};

// ---------------------------------------------------------------------------------------------
// cgmWED -- winged-edge graph of a polygon mesh (nodes = vertices, edges shared by up to 2 faces
// (more are kept in a list), faces). Used for connectivity, boundary detection and shortest paths.
// ---------------------------------------------------------------------------------------------
class cgmWED_EDGE;
class cgmWED_FACE;

enum cgmWED_FLAGS {
    CGM_WED_VISITED  = 0x01,   // node/edge/face: algorithm mark (ClearState)
    CGM_WED_START    = 0x02,   // node: Dijkstra start
    CGM_WED_IN_SET   = 0x04,   // node: part of the Dijkstra node set
    CGM_WED_SELECTED = 0x08,   // face: user selection for CalcBoundary(1)
    CGM_WED_BOUNDARY = 0x10,   // node/edge/face: CalcBoundary result
    CGM_WED_FLIPPED  = 0x20,   // edge: second face uses the edge in the same direction (inconsistent winding)
    CGM_WED_MULTI    = 0x40    // edge: more than two faces (extra faces in extraFace[])
};

class cgmWED_NODE {            // size 0x24
public:
    unsigned      flags;       // +0x00
    int           index;       // +0x04 vertex index
    int           nEdge;       // +0x08
    cgmWED_EDGE **edge;        // +0x0C
    int           _10, _14;    // +0x10 ?
    float         dist;        // +0x18 Dijkstra distance
    cgmWED_NODE  *prev;        // +0x1C Dijkstra predecessor
    int           component;   // +0x20 DepthFirstSearch component id
    cgmWED_NODE  *IncidentNode(cgmWED_EDGE *e);                                  // v12 0x00574000
    cgmWED_EDGE  *IsIncidentNode(cgmWED_NODE *n);                                // v12 0x00574020
};

class cgmWED_EDGE {            // size 0x24
public:
    unsigned      flags;       // +0x00
    int           index;       // +0x04
    cgmWED_NODE  *node[2];     // +0x08 / +0x0C
    cgmWED_FACE  *face[2];     // +0x10 / +0x14 (face[1] = 0 on a boundary)
    int           _18;         // +0x18 ?
    int           nExtraFace;  // +0x1C
    cgmWED_FACE **extraFace;   // +0x20 apRealloc'ed list (CGM_WED_MULTI)
};

class cgmWED_FACE {            // size 0x1C
public:
    unsigned      flags;       // +0x00
    int           index;       // +0x04
    int           nEdge;       // +0x08
    cgmWED_EDGE **edge;        // +0x0C
    int           _10, _14, _18; // ?
};

// cgmQUERY -- callbacks steering the graph searches (default: accept everything).
// vtable feb 0x005DE5F4: all 5 slots are the same "return 1" (feb 0x0050DF70, 2 args).
class cgmQUERY {
public:
    virtual BOOL32 Query00(void *a, void *b)                    { return 1; }    // slot +0x00 ? never called here
    virtual BOOL32 ProcessEdge(cgmWED_NODE *n, cgmWED_EDGE *e)  { return 1; }    // slot +0x04 may the search cross e?
    virtual BOOL32 Query08(void *a, void *b)                    { return 1; }    // slot +0x08 ?
    virtual BOOL32 ProcessNode(cgmWED_EDGE *e, cgmWED_NODE *n)  { return 1; }    // slot +0x0C node reached (e = 0 for roots); 0 = stop  // ? name
    virtual BOOL32 Query10(void *a, void *b)                    { return 1; }    // slot +0x10 ?
};
class cgmQUERY_BOUNDARY : public cgmQUERY {   // vtable feb 0x005DCAE4: do not walk along/into the boundary
public:
    BOOL32 ProcessEdge(cgmWED_NODE *n, cgmWED_EDGE *e);                          // v12 0x0057EB80
};

class cgmWED_SYS {             // abstract mesh graph; size 0x38. vtable feb 0x005D1800
public:
    cgmWED_SYS();                                                                // v12 0x00573BE0
    virtual void *__vecDelDtor(unsigned f);                                      // slot +0x00 (feb 0x0042F020)
    virtual BOOL32 Init();                                                       // slot +0x04 v12 0x00573C30
    virtual void   Term();                                                       // slot +0x08 v12 0x00573F60
    virtual int    MeshGetNFace() = 0;                                           // slot +0x0C
    virtual int    MeshGetNVert() = 0;                                           // slot +0x10
    virtual int    MeshGetFaceNVert(int face) = 0;                               // slot +0x14
    virtual int    MeshGetFaceVertInd(int face, int k) = 0;                      // slot +0x18
    virtual char  *MeshGetName() { return (char *)"unnamed"; }                   // slot +0x1C inline (feb 0x0042F000)
    virtual float  CalcDist(cgmWED_EDGE *e) { return M3D_INFINITY; }             // slot +0x20 inline (feb 0x0042F010)

    int    DepthFirstSearch(cgmQUERY *q);                                        // v12 0x0057E760
    void   Dijkstra(int nSet, int *set, int nStart, int *start, float *startDist, cgmQUERY *q); // v12 0x0057E7E0
    void   CalcBoundary(int useSelection);                                       // v12 0x0057E9D0

    int           nNode;       // +0x04
    cgmWED_NODE  *node;        // +0x08
    int           nEdge;       // +0x0C
    cgmWED_EDGE  *edge;        // +0x10
    int           nFace;       // +0x14
    cgmWED_FACE  *face;        // +0x18
    cgmWED_EDGE **faceEdgePool;// +0x1C storage of all face edge lists
    cgmWED_EDGE **nodeEdgePool;// +0x20 storage of all node edge lists
    cgmWED_NODE **nodeList;    // +0x24 nNode entries (Dijkstra working set)
    void        **faceList;    // +0x28 nFace entries ?
    int           nodeCap;     // +0x2C
    int           edgeCap;     // +0x30
    int           faceCap;     // +0x34

    static cgmQUERY *pQueryDefault;                                              // v12 0x009AED94 (feb 0x00963A44)

private:
    BOOL32 Realloc(int nNode, int nEdge, int nFace);                             // v12 0x00574080
    BOOL32 AddFaceEdge(cgmWED_FACE *f, int v0, int v1);                          // v12 0x005742D0
    void   ClearState();                                                         // v12 0x005743F0
    void   DepthSearch(cgmWED_NODE *n, int component, cgmQUERY *q);              // v12 0x0057EAF0
};

class cgmWED : public cgmWED_SYS {    // graph over an objOBJ's triangle list; vtable feb 0x005DD0F0
public:
    int    MeshGetNFace();                                                       // v12 0x00573B50
    int    MeshGetNVert();                                                       // v12 0x00573B60
    int    MeshGetFaceNVert(int face);                                           // v12 0x00573B70
    int    MeshGetFaceVertInd(int face, int k);                                  // v12 0x00573B80
    char  *MeshGetName();                                                        // v12 0x00573BD0
    virtual BOOL32 Init(objOBJ *obj);                                            // slot +0x24 v12 0x00573B40
    using cgmWED_SYS::Init;
    objOBJ *obj;               // +0x38
};

class cgmWED_3D : public cgmWED {     // + vertex positions for edge lengths; vtable feb 0x005D17D8
public:
    float  CalcDist(cgmWED_EDGE *e);                                             // v12 0x00574480
    m3dV  *vert;               // +0x3C
};
