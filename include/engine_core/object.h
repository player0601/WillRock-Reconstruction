// object.h -- Will Rock 1.2 unofficial SDK, engine_core: objOBJ (mesh/hierarchy node)
//
// objs covered: engine:Object.obj, Obj_alloc.obj, Obj_misc.obj, obj_access.obj, obj_iter.obj,
//               obj_usr_data.obj (render-side objs obj_mod*, obj_normals, obj_split, obj_cgm belong to engine_render)
// Assumed from re/sdk/core: m3dV, m3dMATR (0x40), m3dBOX (vtable + 0x1C data = 0x20 bytes), m3dSPHERE,
//   txmTEXTURE, psSHEET, objMODIFIER. Conventions: see core_types.h.
//
// objOBJ has NO vtable and its layout is IDENTICAL in feb, jun and v12 (ctor compared instruction by
// instruction: feb 0x004BF1E0 / v12 0x004DBF40; Make() allocates 0xF8 in both). Every offset below is
// therefore "v12 = feb". Objects form a tree (parent / circular sibling ring / first child); the root
// object of a model belongs to one animINST (anim.h). Geometry lives in a separately allocated objGEOM
// that instances share (state4 bit 0x40 = shared, do not free).

#pragma once
#include "core_types.h"
#include <string.h>

struct m3dV;
class animINST;
class txmTEXTURE;
class objMODIFIER;
class objUSR_DATA;
typedef objUSR_DATA *objUSR_DATA_P;
struct m3dBOX_DATA { void *vtbl; int type; float min[3]; float max[3]; }; // m3dBOX as embedded (0x20) // ? member names

enum objSTATE {                       // objOBJ::state (+0x00), SetStateYes/No (recursive)
    OBJ_ST_NO_PARENT_XFORM = 0x1000,  // objCalcLTM: ltm = parent ltm (ignore own local matrix)  (verified objCalcLTM)
    // other bits: hidden / no-collision / etc. are set by game code by name ("objFILTER_HIDDEN_NAME"). // ?
};

// objOBJ::allocFlags (+0x04): which parts of the object are SHARED with another object (the template's)
// and must not be freed. Set by objOBJ::Duplicate, cleared by the _Unshare* functions (engine:Obj_alloc.obj).
enum objALLOC_FLAGS {
    OBJ_SHARE_FACES     = 0x001,      // objGEOM::faceList
    OBJ_SHARE_VERTS     = 0x002,      // objGEOM::vert
    OBJ_SHARE_COLORS    = 0x004,      // objGEOM::color
    OBJ_SHARE_SPLIT     = 0x008,      // objGEOM::split
    OBJ_SHARE_NORM_LCS  = 0x010,      // objGEOM::faceNormalLCS
    OBJ_SHARE_VNORM_LCS = 0x020,      // objGEOM::vertNormalLCS
    OBJ_SHARE_DATA      = 0x040,      // props / user block (DestroyData keeps them)
    OBJ_SHARE_NAME      = 0x080,      // objOBJ::name
    OBJ_SHARE_TEX0      = 0x400,      // texCoord[i]: 0x400 << i
};

// objGEOM: the mesh of an object, allocated by objOBJ::_AllocGeom. Packed (modifier list at +0x5A).
// Every field verified on the Obj_alloc functions (identical feb / v12, all "exact" matches).
#pragma pack(push, 1)
struct objGEOM {                      // size 0x82 (objOBJ::_AllocGeom feb 0x004BD300 / v12 0x004CEDB0)
    void  *faceList;                  // +0x00 faces, stride = faceSize (_AllocFaceList)
    u8     faceSize;                  // +0x04 bytes per face (_CalcFaceSize(fmt))
    u8     faceFormat;                // +0x05
    u16    nTexCoord[3];              // +0x06 texture coordinate sets (_AllocTexCoord)
    void  *texCoord[3];               // +0x0C m2dV[nTexCoord[i]] (8 bytes each)
    struct objSPLIT *split;           // +0x18 face split data (_UnshareFaceSplit duplicates it)
    void **splitUnshared;             // +0x1C block whose first dword is another new'ed block (_FreeSplitUnshared)
    u8    *stateVert;                 // +0x20 per-vertex state bytes (_AllocStateVertList)
    u8    *stateFace;                 // +0x24 per-face state bytes (_AllocStateFaceList)
    struct { const void *vtbl; int type; float center[3]; float radius; } bsphere; // +0x28 m3dSPHERE (type 3)
    m3dV  *vert;                      // +0x40 [objOBJ::nVert] (_AllocVertList)
    m3dV  *faceNormal;                // +0x44 (_AllocNormalList)
    m3dV  *vertNormal;                // +0x48 [nVert] (_AllocVertNormalList)
    u32   *color;                     // +0x4C [n] ARGB, initialised to 0xFFFFFFFF (_AllocColorList)
    m3dV  *faceNormalLCS;             // +0x50 (_AllocNormalListLCS)
    m3dV  *vertNormalLCS;             // +0x54 (_AllocVertNormalListLCS)
    u16    nModifier;                 // +0x58
    objMODIFIER **modifier;           // +0x5A (_AllocModifierList)
    u32    constColor;                // +0x5E -1: constant ARGB colour (SetConstColor; alpha byte +0x61 = SetTransparency)
    int    unk62;                     // +0x62
    u8     unk66[0x76 - 0x66];        // +0x66 zeroed
    u16    unk76;                     // +0x76 0xFFFF
    u16    unk78;                     // +0x78
    int    unk7A;                     // +0x7A
    float  unk7E;                     // +0x7E -1.0f
};
#pragma pack(pop)
// ------------------------------------------------------------------------------------------------
// Materials (obj_split.obj, obj_access.obj). A face's first u16 is its split number: the objSPLIT_ENTRY
// that holds its materials (up to 4 layers). Each material names a material control (objMTL_CTRL) that
// says which texture-coordinate index set (in the face) and coordinate set (in objGEOM::texCoord) the
// layer uses and for which render pass ("rendMtl"). 1.2 reads every one of these lists through clamping
// accessors (index clamped to [0, n] -- note n, not n-1); the inline Get* helpers below are those.
// ------------------------------------------------------------------------------------------------
class txmTEXTURE;
#pragma pack(push, 1)
struct objMTL {                       // size 0x19
    txmTEXTURE *tex;                  // +0x00
    float  uScale, vScale;            // +0x04 texture coordinate scale (GetFaceTexCoord)
    float  _0c;                       // +0x0C 1.0 from AddMtl  // ?
    u32    _10, _14;                  // +0x10 not set by AddMtl  // ?
    signed char ctrl;                   // +0x18 index into objSPLIT::ctrl
};
struct objMTL_CTRL {                  // size 4
    u8     tcInd;                     // +0 which of the face's texture index triples (face +8 + tcInd*6)
    u8     tcCoord;                   // +1 which objGEOM::texCoord set
    u8     rendMtl;                   // +2 render pass / material kind (1, 3 compared in _SetDefaultTCHWIndID) // ?
    u8     tcHWInd;                   // +3 hardware texture stage (_SetDefaultTCHWIndID)
};
struct objMTL_CTRL_TBL {              // size 0x22
    objMTL_CTRL c[4];                 // +0x00
    short  n;                         // +0x10
    float  coeff[4];                  // +0x12 material coefficients (objOBJ::SetMtlCoeff, mtlCOEFF)
    objMTL_CTRL *Get(int i) { if (i > n) i = n; else if (i < 0) i = 0; return &c[i]; } // 1.2 clamp (inlined)
    objMTL_CTRL_TBL();                                   // v12 0x00501050 (feb 0x004EEF80)
    int    FindFreeTCCoordID();                          // v12 0x004EB640 (feb 0x004EDAB0)
    BOOL32 IsSharedTCCoordID(int tcCoord);               // v12 0x004EB680 (feb 0x004EDAF0)
    int    AddMtlCtrl(int rendMtl);                      // v12 0x004EB6C0 (feb 0x004EDB30) index, -1 when full
    int    FindMtlCtrlID(int rendMtl);                   // v12 0x004EB7C0 (feb 0x004EDC30)
    objMTL_CTRL *FindMtlCtrl(int rendMtl);               // v12 0x004EB7F0 (feb 0x004EDC60)
};
struct objMTL_TBL {                   // size 0x66: the start of an objSPLIT_ENTRY
    objMTL mtl[4];                    // +0x00
    short  nMtl;                      // +0x64
    objMTL *Get(int i) { if (i > nMtl) i = nMtl; else if (i < 0) i = 0; return &mtl[i]; } // 1.2 clamp (inlined)
    int    AddMtl(int ctrl, txmTEXTURE *t);              // v12 0x004EB840 (feb 0x004EDCB0) {t, 1, 1, 1, ?, ?, ctrl}
};
struct objSPLIT_ENTRY : objMTL_TBL {  // size 0x70
    u16    splitNmb;                  // +0x66 its own index
    u16    firstFace;                 // +0x68 faces [firstFace, firstFace + nFace) use it (after sorting)
    u16    nFace;                     // +0x6A
    u8     _6c[4];                    // +0x6C
    objMTL *FindMtlTCCoordID(class objOBJ *o, int tcCoord); // v12 0x004EB8C0 (feb 0x004EDD30)
    objMTL *FindMtlRendMtl(class objOBJ *o, int rendMtl);   // v12 0x004EB940 (feb 0x004EDD80)
    BOOL32 CalcTexDensity(class objOBJ *o, int rendMtl, float *du, float *dv); // v12 0x005007D0 (feb 0x004EE720)
    int    GetNFace() { return nFace; }                  // v12 0x004C9800 (feb 0x004B99C0)
    int    GetSplitNmb() { return splitNmb; }            // v12 0x004C9810 (feb 0x004B99D0)
};
struct objSPLIT {                     // size 0x28 (operator new 0x28)
    u16    n;                         // +0x00 entries
    objSPLIT_ENTRY *entry;            // +0x02 apRealloc'ed, 0x70 each
    objMTL_CTRL_TBL ctrl;             // +0x06
    objSPLIT_ENTRY *GetEntry(int i) { if (i > (int)n) i = n; else if (i < 0) i = 0; return &entry[i]; } // 1.2 clamp
                                      //   (inlined; an out-of-line copy at v12 0x004C9820)
    static objSPLIT *__fastcall Make();                   // v12 0x004FFD00 (feb 0x004EDDC0)
    BOOL32 Alloc(int n);                                 // v12 0x005005A0 (feb 0x004EE520) grow to n zeroed entries
    objSPLIT *Duplicate();                               // v12 0x00500640 (feb 0x004EE5C0)
    void   RestoreFaceSplitNmb(class objOBJ *o);         // v12 0x00500740 (feb 0x004EE6C0) faces' split numbers from the ranges
};
// A face record (stride objGEOM::faceSize = nTexSets * 6 + 8, _CalcFaceSize).
struct objFACE {
    u16    split;                     // +0x00 objSPLIT entry (material) of the face
    u16    vert[3];                   // +0x02
    u16    tc[9];                     // +0x08 per texture index set: 3 coordinate indices (only nTexSets used)
};
struct mtlCOEFF { float c[4]; };      // objOBJ::SetMtlCoeff -> objSPLIT::ctrl.coeff  // ? meaning
struct objSPLIT_UNSHARED {            // objGEOM::splitUnshared
    struct { void *p; int id; } *list; // +0x00 operator new(n * 8): {0, -1} each
    int    n;                         // +0x04
    BOOL32 Alloc(int n);                                 // v12 0x00500FF0 (feb 0x004EEF20)
};
#pragma pack(pop)

class objOBJ {
public:
    u32        state;                 // +0x00 v12 +0x00 (feb +0x00) objSTATE bits, SetStateYes v12 0x004CC140 (feb 0x004BC080)
    u32        allocFlags;            // +0x04 v12 +0x04 (feb +0x04) 0x40 = data shared with the template (DestroyData keeps props/geometry)
    u32        procState;             // +0x08 v12 +0x08 (feb +0x08) SetStateProcYes/No v12 0x004CC380/0x004CC410
    u32        unk0C;                 // +0x0C
    int        nFace;                 // +0x10 v12 +0x10 (feb +0x10) AllocFaceList writes it
    int        nVert;                 // +0x14 v12 +0x14 (feb +0x14) AllocVertList writes it
    char      *name;                  // +0x18 v12 +0x18 (feb +0x18) heap string (SetName v12 0x004DC610)
    short      idHier;                // +0x1C v12 +0x1C (feb +0x1C) depth-first index inside its animINST (SetIDHier); slot in animINST::objTable
    short      unk1E, unk20, unk22;   // +0x1E/+0x20/+0x22 -1 when detached  // ?
    u32        rendState;             // +0x24 v12 +0x24 (feb +0x24) SetStateRendYes/No (0x80/0x40 transparency pair, 0x800 -> also 2) // ?
    objGEOM   *geom;                  // +0x28 v12 +0x28 (feb +0x28)
    objOBJ    *parent;                // +0x2C v12 +0x2C (feb +0x2C) verified Attach/Detach v12 0x004DC030/0x004DC070
    objOBJ    *next;                  // +0x30 v12 +0x30 (feb +0x30) circular sibling ring (self when alone)
    objOBJ    *prev;                  // +0x34 v12 +0x34 (feb +0x34)
    objOBJ    *child;                 // +0x38 v12 +0x38 (feb +0x38) first child
    float      ltm[16];               // +0x3C v12 +0x3C (feb +0x3C) m3dMATR local-to-world (objCalcLTM: local * parent.ltm)
    float      local[16];             // +0x7C v12 +0x7C (feb +0x7C) m3dMATR local-to-parent
    animINST  *inst;                  // +0xBC v12 +0xBC (feb +0xBC) owning instance (Detach clears inst->objTable[idHier])
    m3dBOX_DATA bbox;                 // +0xC0 v12 +0xC0 (feb +0xC0) local bounding box (m3dBOX vtable at +0xC0, type 1 at +0xC4)
    u32        invalidMask;           // +0xE0 v12 +0xE0 (feb +0xE0) animINST::Invalidate: if (mask & bits) bits |= +0xE4
    u32        invalidAdd;            // +0xE4
    objUSR_DATA_P *usrData;           // +0xE8 v12 +0xE8 (feb +0xE8) array[nUsrDataSlots] of objUSR_DATA* (AddUsrData)
    short      unkEC;                 // +0xEC -1
    short      unkEE;                 // +0xEE
    psSHEET   *props;                 // +0xF0 v12 +0xF0 (feb +0xF0) per-object property sheet (owned unless allocFlags&0x40)
    class objLOD *lod;                    // +0xF4 the "&lod" switch (objInitLOD), moved to the instance by LODInit; deleted in DestroyData
    // --- functions ---
    static objOBJ *__fastcall Make();                                // v12 0x004CED20 (feb 0x004BD270) new(0xF8)+ctor
    objOBJ();                                             // v12 0x004DBF40 (feb 0x004BF1E0)
    ~objOBJ();                                            // v12 0x004DBFF0 (feb 0x004BF290) deletes children, Detach, DestroyData
    void   Attach(objOBJ *parent);                        // v12 0x004DC030 (feb 0x004BF2D0)
    void   Detach();                                      // v12 0x004DC070 (feb 0x004BF300)
    void   _DestroyData();                                // v12 0x004DC120 (feb 0x004BF3B0) free every part of the mesh + props
    void   DestroyData() { _DestroyData(); }              // (earlier SDK spelling)
    objOBJ *_Duplicate(int withData);                     // v12 0x004DC1E0 (feb 0x004BF470) copy of this node only (geometry shared)
    static m3dV *__fastcall GetTransformedVertList();     // v12 0x004DC660 (feb 0x004BF8F0) the shared TransformPoints buffer
    static int  __fastcall GetTransformedVertNmb();       // v12 0x004DC670 (feb 0x004BF900) TransformPoints calls so far
    void   GetAxisY(m3dV *out);                           // v12 0x004DC8E0 (feb 0x004BFB70)
    void   GetAxisZ(m3dV *out);                           // v12 0x004DC8F0 (feb 0x004BFB80)
    void   TransformPoints(m3dMATR *m, m3dV *out);        // v12 0x004DC900 (feb 0x004BFB90) world (or m*world) vertices; out 0 = shared buffer
    void   GetTransformedVert(int i, m3dV *out);          // v12 0x004DC9B0 (feb 0x004BFC40) from the shared buffer
    // obj_mod.obj / obj_usr_data.obj / obj_normals.obj
    void   ValidateModifiersHier(apSTATE *a, apSTATE *b); // v12 0x004E0230 (feb 0x004D1FB0)
    int    InvalidateModifiers(int bits);                 // v12 0x004E0300 (feb 0x004D2080) // ? feb address
    int    ValidateModifiers(int bits);                   // v12 0x004E0370 (feb 0x004D20F0)
    BOOL32 ApplyModifiers(int bits);                      // v12 0x004E03E0 (feb 0x004D2160) 0 when a modifier failed
    BOOL32 IsApplyModifiers(int bits);                    // v12 0x004E04D0 (feb 0x004D2250)
    void   AdvanceModifiersTime();                        // v12 0x004E0530 (feb 0x004D22B0)
    void   CalcFaceNormals(BOOL32 (__fastcall *filter)(objOBJ *, void *)); // v12 0x004EBDD0 (feb 0x004DBD30) recursive, unit
    void   CalcFacePerpVectors();                         // v12 0x004EBD10 (feb 0x004DB470) recursive, unnormalised
    void   NormalizeFacePerpVectors();                    // v12 0x004EBD70 (feb 0x004DB4D0) recursive
    void   _CalcFaceNormals();                            // v12 0x004EBE40 (feb 0x004DB5D0)
    void   _CalcFacePerpVectors();                        // v12 0x004EB9B0 (feb 0x004DB3C0)
    void   _CalcFaceNormalsLCS(int lcs);                  // v12 0x004EC230 (feb 0x004DB880, 1.2 adds the argument)
    void   CalcVertNormals(BOOL32 (__fastcall *filter)(objOBJ *, void *)); // v12 0x004EC2C0 (feb 0x004DBDA0) // ? by call site
    void   CalcVertPerpVectors();                         // v12 0x004EC790 (feb 0x004DBE10) // ? by call site
    void   NormalizeVertPerpVectors();                    // v12 0x004ECB10 (feb 0x004DBF40) // ? by call site
    void   RemoveModifiers();                             // v12 0x004E0150 (feb 0x004D1ED0)
    BOOL32 AssignModifiers(int n, class objMODIFIER **list);  // v12 0x004E00B0 (feb 0x004D1E30) the template's skins (+0x22 index)
    BOOL32 DuplicateModifiers(objOBJ *to);                // v12 0x004E01B0 (feb 0x004D1F30)
    BOOL32 DuplicateUsrData(objOBJ *to, int mode);        // v12 0x004FFBF0 (feb 0x004ED610)
    void   DestroyUsrData();                              // v12 0x004FFC80 (feb 0x004ED6A0)
    objOBJ *Duplicate(objOBJ *parent);                    // v12 0x004DC560 (feb 0x004BF7F0) deep copy of the hierarchy (shares geometry)
    BOOL32 SetName(char *s);                              // v12 0x004DC610 (feb 0x004BF8A0)
    void   ScaleVertList(float x, float y, float z);      // v12 0x004DC700 (feb 0x004BF990)
    void   GetOrigin(m3dV *out);                          // v12 0x004DC8D0 (feb 0x004BFB60) // callgraph match, verify before hooking
    int    GetNObj(BOOL32 (__fastcall *filter)(objOBJ *, void *)); // v12 0x004CB4A0 (feb 0x004BB4F0) count nodes (filter(o, 0); may be 0)
    int    GetMaxNVert();                                 // v12 0x004CB510 (feb 0x004BB560)
    void   SetStateVBNo(int bits);                        // v12 0x004CC320 (feb 0x004BC260) geom +0x66 vertex-buffer state, recursive
    void   SetStateProcYes(int bits, BOOL32 (__fastcall *f)(objOBJ *, void *)); // v12 0x004CC380 (feb 0x004BC2C0) recursive
    void   SetStateProcNo(int bits, BOOL32 (__fastcall *f)(objOBJ *, void *));  // v12 0x004CC410 (feb 0x004BC350) filter only at the top
    void   _SetStateRendYes(int bits);                    // v12 0x004CC290 (feb 0x004BC1D0) this object only
    void   _SetStateRendNo(int bits);                     // v12 0x004CC2E0 (feb 0x004BC220)
    objOBJ *FindObjMaxNVert();                            // v12 0x004CC9B0 (feb 0x004BC950)
    objOBJ *FindObjMaxNVert(int *n);                      // v12 0x004CCA10 (feb 0x004BC9B0)
    int    SetIDHier(int first);                          // v12 0x004CB580 (feb 0x004BB5D0)
    void   SetStateYes(int bits);                         // v12 0x004CC140 (feb 0x004BC080) recursive
    void   SetStateRendYes(int bits);                     // v12 0x004CC190 (feb 0x004BC0D0) recursive
    void   SetStateRendNo(int bits);                      // v12 0x004CC220 (feb 0x004BC160)
    void   SetTransparency(int on, u8 alpha);             // v12 0x004CC040 (feb 0x004BBF90) // callgraph
    void   SetConstColor(u32 argb, int mode, int recurse);// v12 0x004CBDC0 (feb 0x004BBD10)
    void   ReplaceTex(char *from, char *to);              // v12 0x004CBCD0 (feb 0x004BBC20)
    BOOL32 IsAncestor(objOBJ *o);                         // v12 0x004CCF70 (feb 0x004BCF10)
    // --- Obj_alloc.obj: mesh storage (names as in the map; the leading '_' is part of the name) ---
    BOOL32 _AllocName(int len);                           // v12 0x004CED40 (feb 0x004BD290)
    void   _FreeName();                                   // v12 0x004CED80 (feb 0x004BD2D0)
    BOOL32 _AllocGeom();                                  // v12 0x004CEDB0 (feb 0x004BD300)
    void   _FreeGeom();                                   // v12 0x004CEED0 (feb 0x004BD420)
    BOOL32 _AllocVertList(int n);                         // v12 0x004CEF00 (feb 0x004BD450)
    void   _FreeVertList();                               // v12 0x004CEF80 (feb 0x004BD4D0)
    BOOL32 _UnshareVertList();                            // v12 0x004CEFC0 (feb 0x004BD510)
    BOOL32 _AllocFaceList(int n, int fmt);                // v12 0x004CF090 (feb 0x004BD5E0)
    void   _FreeFaceList();                               // v12 0x004CF130 (feb 0x004BD680)
    BOOL32 _AllocTexCoord(int set, int n);                // v12 0x004CF180 (feb 0x004BD6D0) n = 0 frees
    void   _FreeTexCoord(int set);                        // v12 0x004CF270 (feb 0x004BD7C0)
    BOOL32 _UnshareTexCoord(int set);                     // v12 0x004CF2F0 (feb 0x004BD840)
    BOOL32 _AllocColorList(int n);                        // v12 0x004CF390 (feb 0x004BD8E0)
    void   _FreeColorList();                              // v12 0x004CF3F0 (feb 0x004BD940)
    BOOL32 _UnshareFaceSplit();                           // v12 0x004CF420 (feb 0x004BD970)
    BOOL32 UnshareFaceSplit();                            // v12 0x004CF460 (feb 0x004BD9B0) recursive
    void   _FreeFaceSplit();                              // v12 0x004CF4F0 (feb 0x004BDA40)
    void   _FreeSplitUnshared();                          // v12 0x004CF530 (feb 0x004BDA80)
    BOOL32 _AllocModifierList(int n);                     // v12 0x004CF560 (feb 0x004BDAB0)
    void   _FreeModifierList();                           // v12 0x004CF5E0 (feb 0x004BDB30)
    BOOL32 _UnshareAll();                                 // v12 0x004CF610 (feb 0x004BDB60)
    BOOL32 _AllocNormalList(int n);                       // v12 0x004CF7F0 (feb 0x004BDD40)
    void   _FreeNormalList();                             // v12 0x004CF840 (feb 0x004BDD90)
    void   _CalcVertNormals();                            // v12 0x004EC330 (obj_normals, not rebuilt) // ? name
    BOOL32 AllocVertNormalList();                         // v12 0x004CF870 (feb 0x004BDDC0) recursive, nVert each
    BOOL32 _AllocVertNormalList(int n);                   // v12 0x004CF900 (feb 0x004BDE50)
    void   _FreeVertNormalList();                         // v12 0x004CF950 (feb 0x004BDEA0)
    BOOL32 _AllocNormalListLCS(int n);                    // v12 0x004CF980 (feb 0x004BDED0)
    void   _FreeNormalListLCS();                          // v12 0x004CF9D0 (feb 0x004BDF20)
    BOOL32 _AllocVertNormalListLCS(int n);                // v12 0x004CFA00 (feb 0x004BDF50)
    void   _FreeVertNormalListLCS();                      // v12 0x004CFA50 (feb 0x004BDFA0)
    BOOL32 _AllocStateFaceList(int bytes);                // v12 0x004CFA80 (feb 0x004BDFD0)
    void   _FreeStateFaceList();                          // v12 0x004CFAE0 (feb 0x004BE030)
    BOOL32 _AllocStateVertList(int bytes);                // v12 0x004CFB10 (feb 0x004BE060)
    void   _FreeStateVertList();                          // v12 0x004CFB70 (feb 0x004BE0C0)
    int    _CalcFaceSize(int fmt);                        // v12 0x004CB5D0 (feb 0x004BB620) // callgraph
    // --- Obj_anim.obj: object matrices ---
    void   FreezeLTM();                                   // v12 0x004DF420 (feb 0x004D11A0) bake ltm into the vertices (subtree)
    void   _UnfreezeLTM();                                // v12 0x004DF670 (feb 0x004D13F0)
    void   _FreezeModelMatr();                            // v12 0x004DF850 (feb 0x004D15D0) bake local matrix into vertices + children
    void   GetCenter(m3dV *out);                          // v12 0x004DFA80 (feb 0x004D1800)
    void   RotateGlbAxis(float deg, m3dV *axis);          // v12 0x004DFAB0 (feb 0x004D1830) rotate about a world axis through the origin
    void   SetMatrLT(m3dV *org, m3dV *vx, m3dV *vy, m3dV *vz, int keepChild); // v12 0x004DFB30 (feb 0x004D18B0) set world matrix
    void   DeclareProcAnim();                             // v12 0x004CCF90 (feb 0x004BCF30) state &= ~0x1000 (Obj_misc.obj)
    objFACE *GetFace(int i) { return (objFACE *)((u8 *)geom->faceList + geom->faceSize * i); } // inline (copy v12 0x00479AF0)
    // --- Obj_misc.obj: faces and materials ---
    BOOL32 _CompressFaceList(int nTexSets);               // v12 0x004CB5E0 (feb 0x004BB630) resize face records (-1: as the controls need)
    BOOL32 _SortFaceList();                               // v12 0x004CB700 (feb 0x004BB740) by material, then vertices
    BOOL32 _SortMtlList();                                // v12 0x004CB760 (feb 0x004BB7A0)
    void   MarkUsedTex();                                 // v12 0x004CB8F0 (feb 0x004BB900) recursive: texture flags |= 0x200
    void   ReplaceTex(txmTEXTURE *from, txmTEXTURE *to);  // v12 0x004CB9F0 (feb 0x004BB9E0) recursive
    void   SetTex(int rendMtl, txmTEXTURE *t);            // v12 0x004CBC00 (feb 0x004BBB60) recursive
    void   ReplaceTexList(char **names, int n, char *suffix); // v12 0x004CBD60 (feb 0x004BBCB0)
    void   _SetConstColor(u32 argb, int mode);            // v12 0x004CBE40 (feb 0x004BBD90)
    void   UpdRenderState();                              // v12 0x004CBEC0 (feb 0x004BBE10) recursive
    void   _UpdRenderState();                             // v12 0x004CBF10 (feb 0x004BBE60)
    void   _SetTransparency(int mode, u8 alpha);          // v12 0x004CC0B0 (feb 0x004BC030)
    void   _AnimateModifiers(int n, float *frames, float *weights); // v12 0x004E05B0 (obj_mod.obj) // callgraph
    void   InvalidateRendCFG();                           // v12 0x004CC310 (feb 0x004BC250)
    void   PropagateStateProc(int mask, int bits);        // (feb 0x004BC3C0) not in 1.2
    void   SetMtlCoeff(mtlCOEFF *c);                      // v12 0x004CC480 (feb 0x004BC420) recursive
    void   _NormalizeTextCoord(int set);                  // v12 0x004CC500 (feb 0x004BC4A0)
    static int __fastcall CompMtlTbl(objMTL_TBL *a, objMTL_TBL *b); // v12 0x004CCA70 (feb 0x004BCA10) 0 = same textures
    // --- obj_access.obj ---
    txmTEXTURE *GetFaceTexMtlNmb(int face, int mtl);      // v12 0x004EB0A0 (feb 0x004ED720)
    txmTEXTURE *GetFaceTexRendMtl(int face, int rendMtl); // v12 0x004EB110 (feb 0x004ED760)
    void   GetFaceTexCoord(int face, int vert, int mtl, m2dV *out); // v12 0x004EB1F0 (feb 0x004ED7E0)
    int    GetTCIndIDRendMtl(int rendMtl);                // v12 0x004EB2D0 (feb 0x004ED880)
    int    GetTCCoordIDRendMtl(int rendMtl);              // v12 0x004EB330 (feb 0x004ED8E0)
    txmTEXTURE *GetSplitTexRendMtl(int split, int rendMtl); // v12 0x004EB390 (feb 0x004ED940)
    objMTL *GetSplitMtlRendMtl(int split, int rendMtl);   // v12 0x004EB460 (feb 0x004ED9B0)
    objMTL *GetSplitMtlRendMtl(objSPLIT_ENTRY *e, int rendMtl); // v12 0x004EB530 (feb 0x004EDA20)
    int    CalcMaxTCHWIndID();                            // v12 0x004EB5E0 (feb 0x004EDA80)
    // --- obj_split.obj ---
    BOOL32 _CreateFaceSplitPerFace();                     // v12 0x004FFD40 (feb 0x004EDE00) one split entry per face
    void   _SetDefaultTCHWIndID();                        // v12 0x004FFF20 (feb 0x004EDF80)
    BOOL32 _CompressFaceSplit();                          // v12 0x005001F0 (feb 0x004EE1A0) merge runs of faces with equal materials
    m3dV  *GetFaceVertPtr(int face, int v);               // v12 0x00501080 (feb 0x004EEFB0)
    // earlier SDK spellings
    BOOL32 AllocVertList(int n)          { return _AllocVertList(n); }
    BOOL32 AllocFaceList(int n, int fmt) { return _AllocFaceList(n, fmt); }
    BOOL32 AddUsrData(int slot, objUSR_DATA *d);          // v12 0x004FFBA0 (feb 0x004ED5C0)
    static int  RegisterUsrData(u32 fourcc);              // v12 0x004FFB30 (feb 0x004ED550) max 5 user-data kinds
    static void UnregisterUsrData(u32 fourcc);            // v12 0x004FFB80 (feb 0x004ED5A0)
};
// size 0xF8 (verified: Make pushes 0xF8 in v12 0x004CED20)

void __fastcall    objCalcLTM(objOBJ *o);                            // v12 0x004DC680 (feb 0x004BF910) recompute ltm of o and its subtree
void __fastcall    objValidateLTM(objOBJ *o);                        // v12 0x004DFC70 (feb 0x004D19F0) (Obj_anim.obj)
// ------------------------------------------------------------------------------------------------
// objMODIFIER (obj_mod.obj): something that changes an object's mesh every frame (objOBJ::ApplyModifiers from
// animINST::Validate). Kept in the object's geometry list sorted by output bits, then priority. Every modifier
// works on the static timer and view position below (objMODIFIER::SetViewPos, elapsedTime set per frame).
// vtable v12 0x0060715C, 7 slots.
// ------------------------------------------------------------------------------------------------
class objMODIFIER {
public:
    virtual void  *__vecDelDtor(unsigned flags);                     // [0] v12 0x004DFE40
    virtual BOOL32 AddObj(objOBJ *o);                                // [1] v12 0x004DFF70 into o's list (sorted)
    virtual void   DelObj();                                         // [2] v12 0x004E0000 out of its object's list
    virtual objMODIFIER *Duplicate(int flags);                       // [3] base: none (v12 0x00403200)
    virtual void   Animate(int n, float *frames, float *weights);    // [4] base: nothing (v12 0x00403940)
    virtual void   AdvanceTime();                                    // [5] base: nothing (v12 0x00403A20)
    virtual BOOL32 Modify() = 0;                                     // [6]
    u32      flags;                 // +0x04 1 off, 2 shared with the template
    u32      type;                  // +0x08 FourCC
    int      prio;                  // +0x0C
    u32      inBits;                // +0x10 invalidated by
    u32      outBits;               // +0x14 produces (the list's sort key)
    objOBJ  *obj;                   // +0x18
    objMODIFIER(u32 type, int prio);                                 // v12 0x004DFE10 (feb 0x004D1B90)
    static BOOL32 __fastcall SetViewPos(m3dV *p);                    // v12 0x004DFE70 1 when it moved
    static m3dV  viewPos;                                            // v12 0x009786A0
    static float elapsedTime;                                        // v12 0x009786AC
};
// objMOD_BILLBOARD (src/engine/obj_mod.cpp): turns its object to the viewer once it is over 20 degrees off. Size 0x9C,
// vtable v12 0x00607180. flags: 0x2000 about Y only, 0x4000 keep the parent's inverse until the parent moves.
class objMOD_BILLBOARD : public objMODIFIER {
public:
    m3dMATR parentLTM;              // +0x1C
    m3dMATR parentInv;              // +0x5C
    objMOD_BILLBOARD();                                              // v12 0x004E06B0 (feb 0x004D2430)
    void  *__vecDelDtor(unsigned flags);                             // v12 0x004E06F0
    BOOL32 AddObj(objOBJ *o);                                        // v12 0x004E0710
    BOOL32 Modify();                                                 // v12 0x004E0770
};
// a prism volume held by value (+0xB4, m3dPRISM vtable; the SDK's m3dPRISM is abstract here): wrVCall slots
struct objMOD_PRISM { const void *vtbl; int type; u8 data[0xAC]; };
// objMOD_GEOM (obj_mod_geom.obj): a modifier of vertex positions; keeps copies of the vertices. vtable v12 0x0060786C
class objMOD_GEOM : public objMODIFIER {
public:
    m3dV    *vertLCS;               // +0x1C the object's own vertices (StoreIniVert lcs)
    m3dV    *vertWCS;               // +0x20 in world space (StoreIniVert wcs)
    objMOD_GEOM(u32 type, int prio);                                 // v12 0x004F6E90 (feb 0x004E2C40)
    void  *__vecDelDtor(unsigned flags);                             // v12 0x004F6EC0
    BOOL32 AddObj(objOBJ *o);                                        // v12 0x004F6F30
    BOOL32 StoreIniVert(objOBJ *o, int lcs, int wcs);                // v12 0x004F6F80
};
// objMOD_SKIN (obj_mod_skin.obj; the feb map puts it in obj_mod_geom.obj): a skinned mesh. Every vertex of the
// skinned object is the weighted sum of its references, each an offset carried by one bone object (its ltm). A skin
// can hold several offset sets ("bones" here, morph targets in practice) blended by weights its controllers animate
// (Animate). Size 0x48, vtable v12 0x00607704: [0] dtor 0x004EF870, [1] objMOD_GEOM::AddObj, [2] DelObj,
// [3] Duplicate 0x004ECE80, [4] Animate 0x004EDB30, [5] AdvanceTime (base), [6] Modify 0x004EDBD0.
// flags (+0x04): 4 data shared with another skin (Duplicate(1)), 0x2000 vertices left in object space (else the
// object's state bit 1: world space), 0x4000 references grouped by bone object (the 1.2 loader's Reorder),
// 0x10000 one reference per vertex (Check1Bone).
struct objSKIN_BONE {                                                // stride 0x54 (1.2; feb 0x50)
    u32    flags;                                                    // +0x00 4: the arrays are another skin's
    char   name[0x40];                                               // +0x04
    float *offs;                                                     // +0x44 nRef x m3dV (chunk 0x2F2)
    float *offs2;                                                    // +0x48 second table when Init(flags & 2)
    float  weight;                                                   // +0x4C blend weight (chunk 0x2F5; Animate)
    class m3dSPL *spl;                                               // +0x50 weight controller (chunk 0x2F6)
};
class objMOD_SKIN : public objMOD_GEOM {
public:
    int    nGroup;                                                   // +0x24 skinned vertices (or bone groups, 0x4000)
    u32   *objTbl;                                                   // +0x28 the group's bone object (0x4000)
    int    nRef;                                                     // +0x2C references
    int   *nGroupRef;                                                // +0x30 per group: its number of references
    struct REF { int obj; float weight; } *ref;                      // +0x34 nRef: bone object (or vertex, 0x4000)
    int    nBone;                                                    // +0x38 offset sets
    objSKIN_BONE *bone;                                              // +0x3C
    float  blend;                                                    // +0x40 1.0 (scales y and z of a blended set)
    float  reduce;                                                   // +0x44 the loader's weight threshold (0.05)
    objMOD_SKIN();                                                   // v12 0x004ECB80 (feb 0x004E2FC0)
    void  *__vecDelDtor(unsigned flags);                             // v12 0x004EF870
    objMODIFIER *Duplicate(int share);                               // v12 0x004ECE80 (feb 0x004E3250)
    void   Animate(int n, float *frames, float *weights);            // v12 0x004EDB30 (feb 0x004E3E50)
    BOOL32 Modify();                                                 // v12 0x004EDBD0 (feb 0x004E3EF0)
    BOOL32 Init(int nGroup, int nRef, int nBone, int flags);         // v12 0x004ECBD0 (feb 0x004E3010)
    BOOL32 Check1Bone();                                             // v12 0x004ED840 (feb 0x004E3B90)
    void   Apply_NoReorder();                                        // v12 0x004EDC20 (feb 0x004E3F40)
    void   Apply_Reorder();                                          // v12 0x004EE3A0 (feb 0x004E46C0)
    static void __fastcall StartApply(objOBJ **objTable);            // v12 0x004EDB20 (feb 0x004E3E40) the instance's objects
    static objOBJ **ppObjList;                                       // v12 0x0097873C the table StartApply set
};
// objMOD_NOISE_DEFORM ("NDEF"): water waves -- every vertex lifted by maxAmp * (noise + 1) / 2 over its rest
// height (noise of x, z and the time), times its boundary weight. vtable v12 0x00607888, size 0xF0
class objMOD_NOISE_DEFORM : public objMOD_GEOM {
public:
    float    maxAmp;                // +0x24
    float    freqXZ;                // +0x28
    float    freqTime;              // +0x2C
    float    dist;                  // +0x30 the view's distance (prism DistPoint; 2000000 at first)
    float   *weight;                // +0x34 per vertex 0..1 (FixBoundary: 0 on the mesh boundary)
    objMOD_PRISM prism;             // +0x38 around the surface, grown by maxAmp
    float    time;                  // +0xEC
    objMOD_NOISE_DEFORM();                                           // v12 0x004F7210 (feb 0x004E4C40)
    void  *__vecDelDtor(unsigned flags);                             // v12 0x004F76A0
    BOOL32 AddObj(objOBJ *o);                                        // v12 0x004F7270
    void   AdvanceTime();                                            // v12 0x004F7680
    BOOL32 Modify();                                                 // v12 0x004F7530
    void   SetParams(float maxAmp, float freqXZ, float freqTime, float dist); // v12 0x004F72F0
    BOOL32 FixBoundary(float dist);                                  // v12 0x004F7310
};
// objMOD_TEX (obj_mod_tex.obj): a modifier of one texture layer's coordinates. vtable v12 0x00607068
class objMOD_TEX : public objMODIFIER {
public:
    int          layer;             // +0x1C the material control (render pass) it changes
    objMTL_CTRL *ctrl;              // +0x20
    objMOD_TEX(u32 type, int prio) : objMODIFIER(type, prio) {}
    BOOL32 AddObj(objOBJ *o);                                        // v12 0x004DCF30 own texture coordinates
};
// objMOD_TEX_SCROLL ("RCST"): the layer's coordinates move by speed * time. vtable v12 0x00607084, size 0x30
class objMOD_TEX_SCROLL : public objMOD_TEX {
public:
    u32      _24;                   // +0x24
    float    uSpeed, vSpeed;        // +0x28 +0x2C
    objMOD_TEX_SCROLL(int layer);                                    // v12 0x004DD040
    void  *__vecDelDtor(unsigned flags);                             // v12 0x004DCEF0
    BOOL32 Modify();                                                 // v12 0x004DD080
};
// objMOD_TEX_FRAME_ANIM ("NAFT"): the layer's coordinates step through the cells of a rows x cols texture sheet over
// period (looped with flag 0x4000). vtable v12 0x006070A0, size 0x4C
class objMOD_TEX_FRAME_ANIM : public objMOD_TEX {
public:
    float   *uvIni;                 // +0x24 the layer's coordinates at AddObj
    int      nRows, nCols;          // +0x28 +0x2C the sheet
    int      nFrames;               // +0x30 (SetParams: < 0 -> rows * cols)
    float    period;                // +0x34 seconds for all the frames
    u32      _38;                   // +0x38
    int      cur;                   // +0x3C the frame shown
    float    texScaleU, texScaleV;  // +0x40 +0x44 > 0: the cell's step scaled by sheet / texture size // ?
    float    time;                  // +0x48
    objMOD_TEX_FRAME_ANIM(int layer);                                // v12 0x004DD1F0 (feb 0x004CEDA0)
    void  *__vecDelDtor(unsigned flags);                             // v12 0x004DCEF0 (shared)
    BOOL32 AddObj(objOBJ *o);                                        // v12 0x004DD240 (feb 0x004CEDF0)
    void   AdvanceTime();                                            // v12 0x004DDC90 (feb 0x004CF820)
    BOOL32 Modify();                                                 // v12 0x004DD320 (feb 0x004CEED0)
    void   SetParams(int rows, int cols, float period, float time, int nFrames, float scaleU, float scaleV, int loop); // v12 0x004DD2C0
    void   ApplyFrameNmb(int n);                                     // v12 0x004DD3E0 (feb 0x004CEF90)
};
// objMOD_TEX_SEQ_ANIM ("NAFT"): the layer's texture cycles through a list over period. vtable v12 0x006070BC, size 0x34
class objMOD_TEX_SEQ_ANIM : public objMODIFIER {
public:
    int      layer;                 // +0x1C
    int      nTex;                  // +0x20
    txmTEXTURE **tex;               // +0x24
    float    period;                // +0x28
    float    time;                  // +0x2C
    int      cur;                   // +0x30 -1
    objMOD_TEX_SEQ_ANIM(int layer);                                  // v12 0x004DD4F0
    void  *__vecDelDtor(unsigned flags);                             // v12 0x004DD540
    BOOL32 AddObj(objOBJ *o);                                        // v12 0x004DD5A0
    void   AdvanceTime();                                            // v12 0x004DDCA0
    BOOL32 Modify();                                                 // v12 0x004DD630
    BOOL32 SetParams(int n, txmTEXTURE **tex, float period);         // v12 0x004DD5D0
};
// objMOD_NOISE_DEFORM_UV ("VUDN"): the layer's coordinates wobble by two noise samples. vtable v12 0x006070F4, size 0xF0
class objMOD_NOISE_DEFORM_UV : public objMOD_TEX {
public:
    float   *uvIni;                 // +0x24 the coordinates at AddObj
    float    maxAmp;                // +0x28
    float    freqXZ;                // +0x2C
    float    freqTime;              // +0x30
    float    dist;                  // +0x34
    objMOD_PRISM prism;             // +0x38
    float    time;                  // +0xEC
    objMOD_NOISE_DEFORM_UV(int layer);                               // v12 0x004DD8E0
    void  *__vecDelDtor(unsigned flags);                             // v12 0x004DDC50
    BOOL32 AddObj(objOBJ *o);                                        // v12 0x004DD950
    void   AdvanceTime();                                            // (the NOISE_DEFORM one, v12 0x004F7680)
    BOOL32 Modify();                                                 // v12 0x004DDA20
    void   SetParams(float maxAmp, float freqXZ, float freqTime, float dist); // v12 0x004DDA00
};
BOOL32 __fastcall  objInit();                                        // v12 0x004DBF10 (feb 0x004BF1B0) the shared vertex buffer
void __fastcall    objTerm();                                        // v12 0x004DBF30 (feb 0x004BF1D0)
void __fastcall    objCalcBSphereCenter(objOBJ *o, class m3dSPHERE *s); // v12 0x004DC9E0 (feb 0x004BFC70) grow s.radius over the subtree's world vertices
void __fastcall    objCalcBPrism(objOBJ *o, class m3dPRISM *p);      // v12 0x004DCCE0 (feb 0x004BFF70)
BOOL32 __fastcall  objCalcBBox(objOBJ *o);                           // v12 0x004DCD10 (feb 0x004BFFA0) world boxes of the subtree (objOBJ::bbox)

// objOBJ_ITER_SAFE (obj_iter.obj): snapshot of the direct children (max 128), safe against re-linking.
struct objOBJ_ITER_SAFE {                                            // size 0x208
    objOBJ *obj[128];                                                // +0x000
    int     n;                                                       // +0x200
    int     cur;                                                     // +0x204
    objOBJ_ITER_SAFE(objOBJ *parent);                                // v12 0x00503A00 (feb 0x004EEFE0)
};
// objSET: a fixed-size list of objects (LOD sets). vtable slot 0 = deleting dtor (v12 0x004F4240, lod.obj).
class objSET {
public:
    virtual void *__vecDelDtor(unsigned flags);                      // v12 0x004F4240 (feb 0x004E0000)
    int      max;                                                    // +0x04
    int      n;                                                      // +0x08
    objOBJ **obj;                                                    // +0x0C apCalloc(max, 4)
    BOOL32 Alloc(int max);                                           // v12 0x004DCE80 (feb 0x004C0110)
    BOOL32 AddObj(objOBJ *o);                                        // v12 0x004DCEC0 (feb 0x004C0150)
};
objOBJ __fastcall *objFindName(objOBJ *root, char *name);            // v12 0x004CCB00 (feb 0x004BCAA0) depth-first, exact name
objOBJ __fastcall *objFindNameN(objOBJ *root, char *name, int n);    // v12 0x004CCB80 (feb 0x004BCB20) // callgraph
objOBJ __fastcall *objFindID(objOBJ *root, int idHier);              // v12 0x004CCC00 (feb 0x004BCBA0) // callgraph
objOBJ __fastcall *objFind(objOBJ *root, class objFILTER *f);        // v12 0x004CC8D0 (feb 0x004BC870)
objOBJ __fastcall *objFind(objOBJ *root, BOOL32 (__fastcall *fn)(objOBJ *, void *), void *data); // v12 0x004CC930 (feb 0x004BC8D0)
objOBJ __fastcall *objFindAnimNmb(objOBJ *root, int n);              // v12 0x004CCC70 (feb 0x004BCC10) first with unk20 == n
int __fastcall     objMakeTexList(objOBJ *root, int rendMtl, class txmTEXTURE **out, int max); // v12 0x004CCF40 (feb 0x004BCEE0)
int __fastcall     _objIncludeTexList(objOBJ *o, int rendMtl, class txmTEXTURE **out);          // v12 0x004CD190 (feb 0x004BD100)
class m3dSPL __fastcall *objConvertSpl(objOBJ *o);                   // v12 0x004CC810 (feb 0x004BC7B0) vertex triples -> Bezier 3D spline
int __cdecl        _objCmpFace(const void *a, const void *b);        // v12 0x004CCFA0 (feb 0x004BCF40) qsort: material, then vertices
int __cdecl        _objCmpSplitEntry(const void *a, const void *b);  // v12 0x004CD050 (feb 0x004BCFC0) qsort: CompMtlTbl
int __fastcall     objMakePtrList(objOBJ **roots, int nRoots, objFILTER *f, objOBJ **out, int max); // v12 0x004CCD50 (feb 0x004BCCF0) depth-first, filtered
int __fastcall     objMakePtrList(objOBJ **roots, int nRoots, objOBJ **out, int max);              // v12 0x004CCE10 (feb 0x004BCDB0)
int __fastcall     objMakePtrListID(objOBJ *root, objOBJ **out, int max);                         // v12 0x004CCEC0 (feb 0x004BCE60) out[idHier] = obj
int __fastcall     _objBuildObjPtrList(objOBJ *o, objFILTER *f, objOBJ **out, int max, int *n);   // v12 0x004CD060 (feb 0x004BCFD0)
int __fastcall     _objIncludePtrListID(objOBJ *o, objOBJ **out, int max, int *n);                // v12 0x004CD110 (feb 0x004BD080)

class objFILTER {                                                    // functor: slot +0x00 = IsUse
public:
    virtual BOOL32 IsUse(objOBJ *o) = 0;
};
class objFILTER_NAME : public objFILTER {                            // vtable v12 ? (feb 0x005DA1A4)
public:
    char *name;                                                      // +0x04
    objFILTER_NAME(char *n) : name(n) {}
    BOOL32 IsUse(objOBJ *o);                                         // v12 0x004CCCE0 (feb 0x004BCC80) strcmp
};
// objFILTER_HIDDEN_NAME: a shown object (not procState 8) whose name contains name. vtable v12 0x005FB630
class objFILTER_HIDDEN_NAME : public objFILTER {
public:
    char *name;                                                      // +0x04
    objFILTER_HIDDEN_NAME(char *n) : name(n) {}
    BOOL32 IsUse(objOBJ *o);                                         // v12 0x00425CC0 (player.cpp)
};
// objFILTER_PART_SOURCE: collects up to max (<= 2) objects to emit particles from (the biggest skin parts).
// vtable v12 0x00600A04
class objFILTER_PART_SOURCE : public objFILTER {
public:
    int     n;                                                       // +0x04 found
    int     max;                                                     // +0x08
    objOBJ *found[2];                                                // +0x0C
    objFILTER_PART_SOURCE(int m) : n(0), max(m > 2 ? 2 : m) { found[0] = found[1] = 0; } // v12 0x00467A00
    BOOL32 IsUse(objOBJ *o);                                         // v12 0x00467A40 -- not rebuilt
};
BOOL32 __fastcall objIsWithFaces(objOBJ *o, void *);                 // v12 0x004CB560 nFace > 0 (name ours)
class objFILTER_ID : public objFILTER {                              // vtable v12 0x00606D64
public:
    int id;                                                          // +0x04 objOBJ::idHier
    objFILTER_ID(int i) : id(i) {}
    BOOL32 IsUse(objOBJ *o);                                         // v12 0x004CD2B0 (feb 0x004BD220)
};
typedef BOOL32 (__fastcall *objFILTER_FN)(objOBJ *o, void *data);
class objFILTER_FUNC : public objFILTER {
public:
    objFILTER_FN fn;                                                 // +0x04
    void        *data;                                               // +0x08
    objFILTER_FUNC(objFILTER_FN f, void *d) : fn(f), data(d) {}
    BOOL32 IsUse(objOBJ *o);                                         // v12 0x004CD2D0 (feb 0x004BD240)
};
class objFILTER_NAME_LEN : public objFILTER {                        // vtable v12 0x0060377C (objFindNameN)
public:
    char *name;                                                      // +0x04
    int   len;                                                       // +0x08
    objFILTER_NAME_LEN(char *n, int l) : name(n), len(l) {}
    BOOL32 IsUse(objOBJ *o) { return o->name && !strncmp(o->name, name, len); } // inline (copy v12 0x00485B50)
};
class objFILTER_ANIM_NMB : public objFILTER {
public:
    int n;                                                           // +0x04 compared with objOBJ::unk20 (+0x20)
    objFILTER_ANIM_NMB(int i) : n(i) {}
    BOOL32 IsUse(objOBJ *o);                                         // v12 0x004CD2E0 (feb 0x004BD250)
};
// objFILTER: tiny functor class, vtable slot +0x00 = BOOL IsUse(objOBJ*). Subclasses in the game:
//   objFILTER_NAME (+0x04 char*), objFILTER_ID, objFILTER_ANIM_NMB, objFILTER_HIDDEN_NAME, objFILTER_NAME_LEN...
// objOBJ_ITER / objOBJ_ITER_SAFE (obj_iter): depth-first iterators; Rewind v12 0x0041C160 (feb 0x00419700).

extern int    objNUsrDataSlots;    // v12 0x00984DFC (feb 0x00940BBC) highest registered user-data index + 1
extern u32    objUsrDataIds[5];    // v12 0x00984E04 (feb 0x00940BC4)
