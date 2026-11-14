// engine.h -- Will Rock 1.2 (v12.exe) reconstructed source: private header of library "engine"
// (original folder C:\PROJECTS\WillRock\Sources\..\engine\; objs Animbvol, Cdtm_sys, Obj_alloc, Obj_anim, ...).
//
// Declarations the unofficial SDK (src_recon/include) does not carry yet. The SDK is included as-is;
// SDK corrections made while rebuilding this library are listed in re/work/recon_fixes/04_engine.md.
#pragma once
#include <string.h>
#include <stdlib.h>
#include <new>
#include <stddef.h>
#include "wr_sdk.h"
#include "../m3d/m3d_lib.h"   // m3dTransformPoint/Vector, inline m3dNormalize
#include "../m3d/m3d_vol.h"   // game vtables of the m3d volumes (m3dBOX_vftable ...)
#include "../m3d/m3d_vox.h"   // voxGRID_EXT (voxel grid methods)
#include "../wr_vcall.h"     // wrVCall: slot-exact virtual calls

#ifndef WR_TODO
#define WR_TODO(a) ((void)0)
#endif

// ---------------------------------------------------------------------------------------------
// MSVC 6 object model helpers
// ---------------------------------------------------------------------------------------------
// Classes the game deletes through their vtable get slot 0 = MSVC "vector deleting destructor"
// (flags: 1 = free the memory, 2 = it is an array made by new[], count cookie in the dword before it).
// GCC's own virtual destructors take two slots and would shift every later slot, so the reconstructed
// classes declare slot 0 explicitly as __vecDelDtor and implement the MSVC semantics.
template <class T> T *wrArrayNew(int n)
{
    int *blk = (int *)operator new(n * sizeof(T) + 4);
    if (!blk)
        return 0;
    *blk = n;
    T *a = (T *)(blk + 1);
    for (int i = 0; i < n; i++)
        new (&a[i]) T();
    return a;
}
template <class T> void *wrVecDelDtor(T *self, unsigned flags)
{
    if (flags & 2) {
        int *blk = (int *)self - 1;
        for (int i = *blk - 1; i >= 0; i--)
            self[i].Destroy();
        if (flags & 1)
            operator delete(blk);
        return blk;
    }
    self->Destroy();
    if (flags & 1)
        operator delete(self);
    return self;
}
// delete / delete[] of a game object through slot 0 (what "delete p" / "delete[] p" compile to)
inline void wrDelete(void *obj, unsigned flags)
{
    typedef void *(__thiscall *dd_t)(void *, unsigned);
    (*(dd_t *)*(void **)obj)(obj, flags);
}

// ---------------------------------------------------------------------------------------------
// m3d volumes as plain data. The SDK classes are abstract (pure virtuals), so a volume embedded by value
// (abvBVOL::sphere, abvOBB_UPD::local) is declared as its layout; vptr = the game's vtable.
// ---------------------------------------------------------------------------------------------
#ifndef WR_NATIVE
extern const void *const m3dVOL_vftable[];     // v12 0x005F9478 (feb 0x005CEE78) abstract base (members' destructors reset to it)
#endif

struct m3dSPHERE_D { const void *vtbl; int type; m3dV center; float radius; };   // size 0x18
struct m3dOBB_D    { const void *vtbl; int type; m3dV org; m3dV ax[3]; };         // size 0x38
static_assert(sizeof(m3dSPHERE_D) == 0x18, "m3dSPHERE");
static_assert(sizeof(m3dOBB_D) == 0x38, "m3dOBB");

// new m3dBOX / m3dPRISM / m3dSPHERE / m3dOBB as the inlined constructors do: operator new, type, vptr.
// (The out-of-line m3dVOL::Make v12 0x005698B0 does the same; Animbvol inlines it.)
inline m3dVOL *abvNewVol(int type)
{
    int size; const void *vt;
    switch (type) {
    case M3D_VOL_BOX:    size = 0x20; vt = m3dBOX_vftable;    break;
    case M3D_VOL_PRISM:  size = 0xB4; vt = m3dPRISM_vftable;  break;
    case M3D_VOL_SPHERE: size = 0x18; vt = m3dSPHERE_vftable; break;
    case M3D_VOL_OBB:    size = 0x38; vt = m3dOBB_vftable;    break;
    default: return 0;
    }
    int *v = (int *)operator new(size);
    if (!v)
        return 0;
    v[1] = type;
    *(const void **)v = vt;
    return (m3dVOL *)v;
}

// ---------------------------------------------------------------------------------------------
// Animbvol.obj -- bounding volumes ("hit boxes") of animated instances
// ---------------------------------------------------------------------------------------------
// An abvBVOL is one volume attached to one object of an instance (or to the instance itself, obj = 0).
// Its abvVOL_UPD keeps the volume's offsets relative to the object origin (or, for an OBB, the volume
// in the object's local frame) and rebuilds the world-space m3dVOL each frame (Update), plus an
// enclosing sphere used for the quick rejection test.
class abvBVOL;

// vtable layout (6 slots), verified on the v12 vtables below
class abvVOL_UPD {
public:
    virtual void  *__vecDelDtor(unsigned flags);                          // slot +0x00 v12 0x004F2F80 (feb 0x004CEA30) shared by BOX/PRISM/SPHERE
    virtual void   MakeUpdStruct(animINST *inst, objOBJ *obj) = 0;        // slot +0x04 capture offsets from the current pose
    virtual void   Update(animINST *inst, abvBVOL *bv) = 0;               // slot +0x08 rebuild the world volume
    virtual void   Expand(float d) = 0;                                   // slot +0x0C
    virtual BOOL32 CheckEnclSph(m3dSPHERE *s);                            // slot +0x10 v12 0x004F2F70 (feb 0x004CEA20) -> vol
    virtual void   GetFace(cdtINFO *info, m3dPOLY *out, m3dV *normal);    // slot +0x14 v12 0x004F15E0 (feb 0x004CD610) -> vol
    m3dVOL *vol;                                                          // +0x04 world-space volume (owned)

    void Destroy();
    static abvVOL_UPD *__fastcall Make(m3dVOL *vol);                                // v12 0x004F0E70 (feb 0x004CCE80)
};
// vtable abvVOL_UPD: feb only (abstract)

class abvSPHERE_UPD : public abvVOL_UPD {                                 // size 0x14, vtable v12 0x006077B8
public:
    m3dV off;                                                             // +0x08 center - object origin
    void   MakeUpdStruct(animINST *inst, objOBJ *obj);                    // v12 0x004F1600 (feb 0x004CD650)
    void   Update(animINST *inst, abvBVOL *bv);                           // v12 0x004F1680 (feb 0x004CD6B0)
    void   Expand(float d);                                               // v12 0x004F16F0 (feb 0x004CD720)
    void   GetFace(cdtINFO *info, m3dPOLY *out, m3dV *normal);            // v12 0x004F1700 (feb 0x004CD730)
};
class abvBOX_UPD : public abvVOL_UPD {                                    // size 0x20, vtable v12 0x006077E8
public:
    m3dV offMin;                                                          // +0x08 box min - object origin
    m3dV offMax;                                                          // +0x14 box max - object origin
    void   MakeUpdStruct(animINST *inst, objOBJ *obj);                    // v12 0x004F1740 (feb 0x004CD770)
    void   Update(animINST *inst, abvBVOL *bv);                           // v12 0x004F1810 (feb 0x004CD840)
    void   Expand(float d);                                               // v12 0x004F18A0 (feb 0x004CD8D0)
};
class abvPRISM_UPD : public abvVOL_UPD {                                  // size 0x30, vtable v12 0x006077D0
public:
    m2dV  off[4];                                                         // +0x08 first 4 base vertices (x,z) - origin (x,z)
    float offYMin;                                                        // +0x28 yMin - origin.y
    float offYMax;                                                        // +0x2C yMax - origin.y
    void   MakeUpdStruct(animINST *inst, objOBJ *obj);                    // v12 0x004F18D0 (feb 0x004CD900)
    void   Update(animINST *inst, abvBVOL *bv);                           // v12 0x004F1990 (feb 0x004CD9C0)
    void   Expand(float d);                                               // v12 0x0051AAF0 (feb 0x004CDA70) shared empty "ret 4"
};
class abvOBB_UPD : public abvVOL_UPD {                                    // size 0x40, vtable v12 0x0060776C
public:
    m3dOBB_D local;                                                       // +0x08 the box in the object's local frame
    void  *__vecDelDtor(unsigned flags);                                  // v12 0x004F2FB0 (feb 0x004CEA60)
    void   MakeUpdStruct(animINST *inst, objOBJ *obj);                    // v12 0x004F1A40 (feb 0x004CDA80)
    void   Update(animINST *inst, abvBVOL *bv);                           // v12 0x004F1FF0 (feb 0x004CE030)
    void   Expand(float d);                                               // v12 0x004F25D0 (feb 0x004CE5D0)
    void   Destroy();
};

class abvBVOL {                                                           // size 0x24, vtable v12 0x005F9474 (feb 0x005DBC88)
public:
    virtual void *__vecDelDtor(unsigned flags);                           // slot +0x00 v12 0x004037D0 (emitted in another obj)
    objOBJ      *obj;                                                     // +0x04 object it follows (0 = the instance itself)
    abvVOL_UPD  *upd;                                                     // +0x08
    m3dSPHERE_D  sphere;                                                  // +0x0C enclosing sphere (radius -1 = not computed)

    abvBVOL();                                                            // v12 0x004F0FE0 (feb 0x004CCFF0)
    void Destroy();                                                       //          (feb 0x004CD020 ~abvBVOL)
    abvBVOL &operator=(abvBVOL &src);                                     // v12 0x004F1010 (feb 0x004CD040)
    BOOL32 Alloc(int volType);                                            // v12 0x004F1060 (feb 0x004CD090)
    void   DuplicateVol(abvBVOL *src);                                    // v12 0x004F1200 (feb 0x004CD230)
    void   Calc(animINST *inst);                                          // v12 0x004F1470 (feb 0x004CD4A0)
    void   GetOrg(animINST *inst, m3dV *out);                             // v12 0x004F14A0 (feb 0x004CD4D0)
    BOOL32 IsIsectCyl(m3dV *base, m3dV *top, float r, float h);           // v12 0x004F14E0 (feb 0x004CD510)
    BOOL32 IsectRay(m3dV *org, m3dV *dir, float maxT, cdtINFO *info);     // v12 0x004F1500 (feb 0x004CD530)
    BOOL32 DistPoint(m3dV *p, float maxD, cdtINFO *info);                 // v12 0x004F1570 (feb 0x004CD5A0)
};

static_assert(sizeof(abvSPHERE_UPD) == 0x14, "abvSPHERE_UPD");
static_assert(sizeof(abvBOX_UPD)    == 0x20, "abvBOX_UPD");
static_assert(sizeof(abvPRISM_UPD)  == 0x30, "abvPRISM_UPD");
static_assert(sizeof(abvOBB_UPD)    == 0x40, "abvOBB_UPD");
static_assert(sizeof(abvBVOL)       == 0x24, "abvBVOL");

// ---------------------------------------------------------------------------------------------
// Engine functions used by this library (defined in engine objs not rebuilt yet, or in the SDK)
// ---------------------------------------------------------------------------------------------
void __fastcall objValidateLTM(objOBJ *o);                                // v12 0x004DFC70 (feb 0x004D19F0) Obj_anim.obj
void __fastcall objCalcBPrism(objOBJ *o, m3dPRISM *out);                  // v12 0x004DCCE0 (feb 0x004BFF70) Object.obj

// layout checks of the SDK fields this library relies on
static_assert(offsetof(animINST, matrW2L) == 0x0A0 && offsetof(animINST, nBVol) == 0x0F4 &&
              offsetof(animINST, bvSeq) == 0x0FC && offsetof(animINST, bvCur) == 0x100 &&
              offsetof(animINST, tpl) == 0x138 && sizeof(animINST) == 0x14C, "animINST layout");
static_assert(offsetof(animTPL, defSeq) == 0x094 && offsetof(animTPL, nBVol) == 0x0B0 &&
              offsetof(animTPL, bvSeq) == 0x0B8 && offsetof(animTPL, props) == 0x0C4 &&
              sizeof(animTPL) == 0xCC, "animTPL layout");
static_assert(offsetof(objOBJ, ltm) == 0x3C && offsetof(objOBJ, bbox) == 0xC0 && sizeof(objOBJ) == 0xF8, "objOBJ layout");

// ---------------------------------------------------------------------------------------------
// Cdtm_sys.obj / cdt_sys.obj -- voxel-grid collision base
// ---------------------------------------------------------------------------------------------
// cdtOFL: scratch "object + face list" filled by the collision queries (one global, cdtOflPtr).
struct cdtOFL {                       // size 0x20; ctor(maxObj, maxFace) v12 0x00503910 (feb 0x004F1510)
    int  nObj;                        // +0x00 objects found
    u16 *obj;                         // +0x04 [maxObj] object ids
    u16 *nObjFace;                    // +0x08 [maxObj] faces kept per object (only with per-face lists)
    int *objFaceStart;                // +0x0C [maxObj] first entry of the object in face[]
    int  nFace;                       // +0x10
    u16 *face;                        // +0x14 [maxFace] face index inside its object
    int  maxObj;                      // +0x18
    int  maxFace;                     // +0x1C
};

// cdtCOLL: a voxel grid over a set of objects; each cell lists the objects (and optionally, per object,
// the faces) that touch it. 1.0/1.2 LAYOUT DIFFERS FROM THE BETA (size 0x70 vs 0x68): the "already
// collected" marks became bitsets (one bit per object / face) and the count/pointer pairs were reordered.
//   feb: +0x48 u8 *objMark, +0x4C nObj, +0x50 cellFaceStart, +0x54 cellFace, +0x58 objFaceBase,
//        +0x5C u8 *faceMark, +0x60 nFaceMark.
// Verified on v12 ctor 0x00503580, dtor 0x00503600, AllocLists 0x005036F0, MakeIndList 0x00502780.
// The 27-slot query interface. The generic algorithms (cdto_math.obj: ShootRay, GetClosestPoint) and the
// grid builder (cdt2_sgen.obj: CalcData2) only use the accessor slots, so the same code serves:
//   cdtCOLL_OBJ   one objOBJ, id = face                     (data = objOBJ*,   1 level)
//   cdtCOLL_SCN   the static level, id = scene object,      (data = scnSCENE*, 2 levels)
//                 id2 = face of that object
//   cdtCOLL_INST  one animated instance, id = its object    (data = animINST*, 2 levels)
// "id2 data" (GetID2Ptr) is the objOBJ* of the id for SCN/INST and the data pointer for OBJ.
struct cdtINFO_LSEG; struct cdtQUERY;
class cdtCOLL {
public:
    virtual void *__vecDelDtor(unsigned flags);                                       // [ 0] v12 0x00503600 (feb 0x004F1260)
    virtual void  MakeIndList(m3dV *a, m3dV *b, float expand, cdtOFL *out, int mode); // [ 1] v12 0x00502780 (feb 0x004F0520)
    virtual void  MakeIndListRay(m3dV *a, m3dV *b, cdtOFL *out, int mode);            // [ 2] v12 0x00502A40 (feb 0x004F0770)
    virtual BOOL32 RefineInd(int id);                                                 // [ 3] v12 0x0048D590 folded "return 0"
    virtual void  RefineIndList(cdtOFL *ofl, int mode);                               // [ 4] v12 0x00403860 folded empty; subclasses filter
    virtual int   GetNID() = 0;                                                       // [ 5]
    virtual void *GetID2Ptr(int id) = 0;                                              // [ 6]
    virtual int   GetNID2(int id) = 0;                                                // [ 7]
    virtual void  GetBBoxAll(m3dBOX *out) = 0;                                        // [ 8]
    virtual BOOL32 GetIDBBox(int id, m3dBOX *out) = 0;                                // [ 9]
    virtual BOOL32 IsectIDBBox(int id, m3dBOX *b) = 0;                                // [10]
    virtual BOOL32 GetID2BBox(void *d2, int id2, m3dBOX *out) = 0;                    // [11]
    virtual BOOL32 IsectID2BBox(void *d2, int id2, m3dBOX *b) = 0;                    // [12]
    virtual BOOL32 Is2LevelData() = 0;                                                // [13]
    virtual BOOL32 IsDoubleSided(void *d2) = 0;                                       // [14]
    virtual void  GetFacePolyNormal(void *d2, int f, m3dPOLY *poly, m3dV *n) = 0;     // [15] (name ours; 1.0/1.2)
    virtual void  GetFaceNormal(void *d2, int f, m3dV *n) = 0;                        // [16]
    virtual void  GetFacePoly(void *d2, int f, m3dPOLY *poly) = 0;                    // [17]
    virtual BOOL32 IsIsectRayBVol(void *d, int id, m3dV *org, m3dV *dir, float len) = 0; // [18]
    virtual BOOL32 IsIsectRayBVolAll(void *d, m3dV *org, m3dV *dir, float len) = 0;   // [19]
    virtual BOOL32 IsNearBVol(void *d, int id, m3dV *p, float r) = 0;                 // [20]
    virtual BOOL32 IsNearBVolAll(void *d, m3dV *p, float r) = 0;                      // [21]
    virtual BOOL32 ShootRay(m3dV *org, m3dV *dir, float len, int flags, cdtINFO *info) = 0;     // [22]
    virtual BOOL32 ShootCyl(m3dV *a, m3dV *b, float r, int flags, cdtINFO_LSEG *info) = 0;      // [23]
    virtual BOOL32 GetClosestPointQ(cdtQUERY *q, int flags) = 0;                                // [24]
    virtual BOOL32 GetClosestPointSeg(m3dV *a, m3dV *b, float r, int flags, cdtINFO_LSEG *i) = 0; // [25]
    virtual BOOL32 GetClosestPoint(m3dV *p, float r, int flags, cdtINFO *info) = 0;             // [26]
    u32       flags;                  // +0x04 bit 0: per-object face lists are kept
    voxGRID   grid;                   // +0x08 (size 0x38) cell (ix,iy,iz) = (grid.ny*ix + iy)*grid.nz + iz
    int      *cellStart;              // +0x40 [nCell+1] first entry of each cell in cellObj
    u16      *cellObj;                // +0x44 [nEntry] object id per entry
    int       nObjMark;               // +0x48 bits in objMark (= nObj)
    u32      *objMark;                // +0x4C bitset, (nObjMark/32+1) dwords
    int       nObj;                   // +0x50
    int      *entFaceStart;           // +0x54 [nEntry+1] (flags & 1) first face of an entry in entFace
    u16      *entFace;                // +0x58 face index inside the object
    int      *objFaceBase;            // +0x5C [nObj+1] first bit of each object in faceMark
    int       nFaceMark;              // +0x60 bits in faceMark
    u32      *faceMark;               // +0x64 bitset
    int       nFaceTotal;             // +0x68 sum of GetNID2 over all ids (CalcData2 / AllocLists)
    void     *data;                   // +0x6C what the ids refer to (objOBJ* / scnSCENE* / animINST*)

    cdtCOLL();                                                          // v12 0x00503580 (feb 0x004F11E0)
    void   Destroy();                                                   // ~cdtCOLL v12 0x00503680 (feb 0x004F12D0)
    BOOL32 AllocLists(int nCell, int nObj, int nEntry, int nFace, int nEntFace); // v12 0x005036F0 (feb 0x004F1330)
    int    CalcTotalMemory();                                           // v12 0x00503830 (feb 0x004F1440)
    BOOL32 CalcData2(float vx, float vy, float vz, int maxX, int maxY, int maxZ, m3dBOX *box); // v12 0x00502D70 (feb 0x004F09D0)
};
static_assert(offsetof(cdtCOLL, cellStart) == 0x40 && offsetof(cdtCOLL, faceMark) == 0x64 && sizeof(cdtCOLL) == 0x70, "cdtCOLL");

void __fastcall _cdtOFLCollectFaceList(cdtCOLL *c, cdtOFL *ofl);                    // v12 0x00502C50 (feb 0x004F08F0)
extern cdtOFL *cdtOflPtr;                                                           // v12 0x00978758 (feb 0x00934510)
BOOL32 __fastcall cdtoInit();                                                       // v12 0x004F5C50 (feb 0x004E1A10)
void __fastcall   cdtoTerm();                                                       // v12 0x004F5C90 (feb 0x004E1A50)
cdtOFL *cdtOFL_Construct(cdtOFL *o, int maxObj, int maxFace);                       // ??0cdtOFL v12 0x00503910 (feb 0x004F1510)
void    cdtOFL_Destroy(cdtOFL *o);                                                  // ??1cdtOFL v12 0x00503990 (feb 0x004F1590)

// cdtCOLL_OBJ (cdt_obj.obj + cdto_math.obj): one object's faces. vtable v12 0x00606DE0 (feb 0x005DBA28).
class cdtCOLL_OBJ : public cdtCOLL {
public:
    void *__vecDelDtor(unsigned flags);                                 // v12 0x004CECF0 (feb 0x004C4190, anim.obj) shared by OBJ/SCN/INST
    int   GetNID();                                                     // v12 0x00501B70  data->nFace
    void *GetID2Ptr(int id);                                            // v12 0x004B9CF0  folded "return 0"
    int   GetNID2(int id);                                              // v12 0x00501B80  -1
    void  GetBBoxAll(m3dBOX *out);                                      // v12 0x00502060 (feb 0x004EFE30)
    BOOL32 GetIDBBox(int id, m3dBOX *out);                              // v12 0x005020B0 (feb 0x004EFE80)
    BOOL32 IsectIDBBox(int id, m3dBOX *b);                              // v12 0x005022F0 (feb 0x004F00C0)
    BOOL32 GetID2BBox(void *d2, int id2, m3dBOX *out);                  // v12 0x00502560  folded "return 0"
    BOOL32 IsectID2BBox(void *d2, int id2, m3dBOX *b);                  // v12 0x00502560
    BOOL32 Is2LevelData();                                              // v12 0x00502570  0
    BOOL32 IsDoubleSided(void *d2);                                     // v12 0x00501B90 (feb 0x004EFC00) objOBJ::state bit 2
    void  GetFacePolyNormal(void *d2, int f, m3dPOLY *poly, m3dV *n);   // v12 0x00501BA0 (1.0/1.2, name ours)
    void  GetFaceNormal(void *d2, int f, m3dV *n);                      // v12 0x00502030 (feb 0x004EFC10)
    void  GetFacePoly(void *d2, int f, m3dPOLY *poly);                  // v12 0x00501DF0 (feb 0x004EF9C0)
    BOOL32 IsIsectRayBVol(void *d, int id, m3dV *org, m3dV *dir, float len); // v12 0x00502620  0
    BOOL32 IsIsectRayBVolAll(void *d, m3dV *org, m3dV *dir, float len); // v12 0x00502580 (feb 0x004F0330)
    BOOL32 IsNearBVol(void *d, int id, m3dV *p, float r);               // v12 0x00502750  0
    BOOL32 IsNearBVolAll(void *d, m3dV *p, float r);                    // v12 0x00502630 (feb 0x004F03E0)
    BOOL32 ShootRay(m3dV *org, m3dV *dir, float len, int flags, cdtINFO *info);     // v12 0x004F5CB0 (feb 0x004E1A70)
    BOOL32 ShootCyl(m3dV *a, m3dV *b, float r, int flags, cdtINFO_LSEG *info);      // v12 0x004F6170 (feb 0x004E1F00)
    BOOL32 GetClosestPointQ(cdtQUERY *q, int flags);                                // v12 0x004F6CD0 (feb 0x004E2A80)
    BOOL32 GetClosestPointSeg(m3dV *a, m3dV *b, float r, int flags, cdtINFO_LSEG *i); // v12 0x004F6A10 (feb 0x004E27B0)
    BOOL32 GetClosestPoint(m3dV *p, float r, int flags, cdtINFO *info);             // v12 0x004F6730 (feb 0x004E24C0)
};

// cdtCOLL_SCN_INST (cdt_scn.obj): shared part of the level and instance collision (ids = objects,
// id2 = faces). No vtable of its own in 1.2 (only the two subclasses are ever built).
class cdtCOLL_SCN_INST : public cdtCOLL_OBJ {
public:
    BOOL32 RefineInd(int id);                                           // v12 0x004DDDA0 (feb 0x004D0510)
    void   RefineIndList(cdtOFL *ofl, int mode);                        // v12 0x004DDDD0 (feb 0x004D0540)
    BOOL32 GetIDBBox(int id, m3dBOX *out);                              // v12 0x004DDE40 (feb 0x004D05B0)
    BOOL32 IsectIDBBox(int id, m3dBOX *b);                              // v12 0x004DDEA0 (feb 0x004D0610)
    BOOL32 GetID2BBox(void *d2, int id2, m3dBOX *out);                  // v12 0x004DE140 (feb 0x004D08B0)
    BOOL32 IsectID2BBox(void *d2, int id2, m3dBOX *b);                  // v12 0x004DE390 (feb 0x004D0B00)
};

// cdtCOLL_SCN: the static level geometry; data = scnSCENE*. vtable v12 0x00608478 (feb 0x005DCFA0).
class cdtCOLL_SCN : public cdtCOLL_SCN_INST {
public:
    int   GetNID();                                                     // v12 0x004DE5F0  scene->nObj
    void *GetID2Ptr(int id);                                            // v12 0x004DE600  scene->objList[id]
    int   GetNID2(int id);                                              // v12 0x004DE610  objList[id]->nFace
    void  GetBBoxAll(m3dBOX *out);                                      // v12 0x004DE630  root bbox
    BOOL32 Is2LevelData();                                              // v12 0x00548680  folded "return 1"
    BOOL32 IsIsectRayBVol(void *d, int id, m3dV *org, m3dV *dir, float len); // v12 0x004DE680 (feb 0x004D0E00)
    BOOL32 IsIsectRayBVolAll(void *d, m3dV *org, m3dV *dir, float len); // v12 0x004DE810  1
    BOOL32 IsNearBVol(void *d, int id, m3dV *p, float r);               // v12 0x004DE6B0 (feb 0x004D0E40)
    BOOL32 IsNearBVolAll(void *d, m3dV *p, float r);                    // v12 0x004C23D0  folded "return 1"
};

// cdtCOLL_INST: collision of one instance (animINST::+0x144, data = the animINST), built for one animation
// frame. Only the frame test is rebuilt so far.
class cdtCOLL_INST : public cdtCOLL_SCN_INST {
public:
    float frame;                                                        // +0x70 frame the data was built for
    // vtable v12 0x00606D70: overrides of the SCN_INST accessors (data = the animINST)
    int    GetNID();                                                    // v12 0x004DE6E0  inst->nObj
    void  *GetID2Ptr(int id);                                           // v12 0x004DE6F0  inst->objTable[id]
    int    GetNID2(int id);                                             // v12 0x004DE710  objTable[id]->nFace
    void   GetBBoxAll(m3dBOX *out);                                     // v12 0x004DE730  root box
    BOOL32 Is2LevelData();                                              // v12 0x00548680  folded "return 1" (objects, then faces)
    BOOL32 IsIsectRayBVol(void *d, int id, m3dV *org, m3dV *dir, float len); // v12 0x004DE7C0  1
    BOOL32 IsIsectRayBVolAll(void *d, m3dV *org, m3dV *dir, float len); // v12 0x004DE780  enclosing volume
    BOOL32 IsNearBVol(void *d, int id, m3dV *p, float r);               // v12 0x004DE810  1
    BOOL32 IsNearBVolAll(void *d, m3dV *p, float r);                    // v12 0x004DE7D0  enclosing volume
    void   SetCDTFrame(float f);                                        // v12 0x004DE820 (feb ?)  frame = f, flags |= 0x100
    BOOL32 IsCDTFrame();                                                // v12 0x004DE840 (feb 0x004D0FC0)
};
// lodLOD (lod.obj): one level-of-detail switch: up to 10 levels, each a distance and the level's data (an object
// of an "&lod" group, or the object set of an "&lod" named selection). No virtual dtor (slot 0 is Apply). Size 0x68.
class camCAMERA;
class lodLOD {
public:
    animINST *inst;                   // +0x04 the instance whose centre is measured (0: the first level's object)
    struct { float dist; void *data; } lev[10];  // +0x08 sorted by detail (level 0 = the most detailed)
    int       nLod;                   // +0x58
    float     maxDist;                // +0x5C [LOD] maxDist, default 100
    float     gamma;                  // +0x60 [LOD] gamma, default 1.5: dist[i] = maxDist * ((i+1)/n)^gamma
    int       cur;                    // +0x64 -1 until the first SetLodNmb
    lodLOD() : nLod(0), maxDist(100.0f), gamma(1.5f), cur(-1) {}       // inline (objInitLOD / LODInit)
    virtual void  Apply(camCAMERA *cam);                                // +0x00 v12 0x004F2FF0 (feb 0x004DED30)
    virtual void  GetLodCenter(m3dV *out) = 0;                          // +0x04
    virtual int   Init();                                               // +0x08 v12 0x004F31F0 (feb 0x004DEF40)
    virtual void  _SetLodNmb(int n) = 0;                                // +0x0C
    virtual void *GetDataPtr(int n);                                    // +0x10 v12 0x004F3250 (feb 0x004DEFA0)
    virtual void  Term();                                               // +0x14 v12 0x00401380 (the shared empty function)
    void SetLodNmb(int n);                                              // v12 0x004F3180 (feb 0x004DEED0)
};
class objLOD : public lodLOD {        // an "&lod" object: its children are the levels (vtable v12 0x00607820)
public:
    void GetLodCenter(m3dV *out);                                       // +0x04 v12 0x004F3440 (feb 0x004DF190)
    void _SetLodNmb(int n);                                             // +0x0C v12 0x004F33D0 (feb 0x004DF120)
    virtual int Init(objOBJ *o);                                        // +0x18 v12 0x004F3260 (feb 0x004DEFB0)
};
class animLOD_SEL : public lodLOD {   // the "&lod" named selections of a template (vtable v12 0x0060783C)
public:
    void GetLodCenter(m3dV *out);                                       // +0x04 v12 0x004F3460 (feb 0x004DF1B0)
    void _SetLodNmb(int n);                                             // +0x0C v12 0x004F3470 (feb 0x004DF1C0)
    void Term();                                                        // +0x14 v12 0x004F36B0 (feb 0x004DF400)
    virtual int Init(animINST *i);                                      // +0x18 v12 0x004F3500 (feb 0x004DF250)
};
// animINST::+0x140: the instance's LOD set (LODInit, operator new 0x200).
struct animLODS {
    u32     saved;                    // +0x000 instance state (+0x0C) saved by LODSetTemp
    u32     flags;                    // +0x004 1 temporary LOD active, 2 disabled (LODEnable), 4 frozen (LODFreeze)
    int     n;                        // +0x008
    lodLOD *lod[0x3C];                // +0x00C
    int     savedLod[0x3C];           // +0x0FC LODSetTemp
    int     scale;                    // +0x1EC the most levels any of its LODs has
    float   unk1F0;                   // +0x1F0 -100
    float   distAnim;                 // +0x1F4 [LOD] distLODAnim (beyond: inst flag 0x200000)
    float   distOffMatrModel;         // +0x1F8 [LOD] distLODOffMatrModel (0x400000)
    float   distOffAnimBlend;         // +0x1FC [LOD] distLODOffAnimBlend (0x800000)
};
int __fastcall objInitLOD(objOBJ *o);                                   // v12 0x004F3A00 (feb 0x004DF7C0)
class m3dSPHERE;
m3dSPHERE *__fastcall animGetObjBSphere(objOBJ *o);                     // v12 0x004CEB40 (feb 0x004C43A0)

// ---------------------------------------------------------------------------------------------
// llist.obj / data_struct.obj -- the containers the collision code uses
// ---------------------------------------------------------------------------------------------
// lstLIST_SIMPLE: engine_core/core_types.h
class dsDYN_ARRAY {                                                     // size 0x1C
public:
    u32    flags;                     // +0x00 bit 0: owns the memory (dtor frees)
    int    count;                     // +0x04
    int    elemSize;                  // +0x08
    int    capacity;                  // +0x0C
    int    growBy;                    // +0x10
    void  *ownPtr;                    // +0x14 storage pointer when the caller gives none
    void **ptr;                       // +0x18 where the storage pointer lives
    dsDYN_ARRAY() : flags(0), count(0), elemSize(-1), capacity(0), growBy(-1), ownPtr(0), ptr(0) {}
                                      // inline default ctor (as in Object.obj's static init v12 0x004DBEC0)
    dsDYN_ARRAY(int elemSize, int growBy, void **ptr);                  // v12 0x004EF940 (feb 0x004DBFE0)
    ~dsDYN_ARRAY();                                                     // v12 0x004EF990 (feb 0x004DC030)
    BOOL32 Init(int elemSize, int growBy, void **ptr, int count, int capacity); // v12 0x004EF9C0 (feb 0x004DC060)
    void   ReleaseMem();                                                // v12 0x004EFA10 (feb 0x004DC0B0) = Term
    BOOL32 AddEntry(int n);                                             // v12 0x004EFA40 (feb 0x004DC0E0)
    BOOL32 Expand(int n);                                               // v12 0x004EFAB0 (feb 0x004DC150)
};

// ---------------------------------------------------------------------------------------------
// cdt_sys_anim.obj -- grid of the animated instances of a scene (scnSCENE::collAnim, +0x54)
// ---------------------------------------------------------------------------------------------
struct cdtCOLL_ANIM_NODE : lstNODE { int id; };                         // size 0x0C
template <class T> class lstNODE_CACHE {                                // vtable v12 0x006075E8
public:
    virtual void  *__vecDelDtor(unsigned flags);                        // [0] v12 0x004E7550
    virtual BOOL32 Init(int n);                                         // [1] v12 0x004E7580
    virtual void   Term();                                              // [2] v12 0x004E7610
    int            n;                                                   // +0x04
    T             *nodes;                                               // +0x08 MSVC array new
    lstLIST_SIMPLE free;                                                // +0x0C (vptr) / +0x10 head
};
class cdtCOLL_ANIM {                                                    // vtable v12 0x006075DC (abstract)
public:
    virtual void  *__vecDelDtor(unsigned flags);                        // [0] v12 0x004E7640
    virtual int    GetNID() = 0;                                        // [1]
    virtual BOOL32 GetIDBBox(int id, m3dBOX *out) = 0;                  // [2]
    u32            flags;                                               // +0x04
    voxGRID        grid;                                                // +0x08
    lstLIST_SIMPLE *cells;                                              // +0x40 [nVox] (MSVC array new, 8 bytes each)
    lstNODE_CACHE<cdtCOLL_ANIM_NODE> cache;                             // +0x44 (0x14 bytes)
    u8             _58[0x60 - 0x58];
    int            nPending;                                            // +0x60
    int           *pending;                                             // +0x64 [GetNID] ids to re-place
    u8            *mark;                                                // +0x68 [GetNID] scratch marks
    int          (*range)[6];                                           // +0x6C [GetNID] cell range per id, -1 = none
    cdtCOLL_ANIM();                                                     // v12 0x004E74C0 (feb 0x004DEB60) (Scene.obj)
    void Destroy();                                                     // ~cdtCOLL_ANIM v12 0x004FE320 (feb 0x004EBD40)
    BOOL32 InitData(float vx, float vy, float vz, int maxX, int maxY, int maxZ, m3dBOX *box); // v12 0x004FE390
    void Invalidate(int id, int remove);                                // v12 0x004FE5C0 (feb 0x004EBFE0)
    void Validate();                                                    // v12 0x004FE620 (feb 0x004EC040)
    void RemoveFromGrid(int id);                                        // v12 0x004FE810 (feb 0x004EC230)
    void MakeIndList(m3dBOX *b, cdtOFL *ofl, int mode);                 // v12 0x004FE8D0 (feb 0x004EC2F0)
    void MakeIndListRay(m3dV *a, m3dV *b, cdtOFL *ofl, int mode);       // v12 0x004FEA30 (feb 0x004EC450)
};
// cdtCOLL_ANIM_SCN: ids = the instance pool of a scene. vtable v12 0x006075D0.
class cdtCOLL_ANIM_SCN : public cdtCOLL_ANIM {
public:
    scnSCENE *scene;                                                    // +0x70
    void  *__vecDelDtor(unsigned flags);                                // v12 0x004E7660 (feb 0x004DED00) (Scene.obj)
    int    GetNID();                                                    // v12 0x004DE8B0  1000 (SCN_MAX_INST)
    BOOL32 GetIDBBox(int id, m3dBOX *out);                              // v12 0x004DE8C0 (feb 0x004D1040)
};
static_assert(offsetof(cdtCOLL_ANIM, cache) == 0x44 && offsetof(cdtCOLL_ANIM, nPending) == 0x60 &&
              sizeof(cdtCOLL_ANIM) == 0x70 && offsetof(cdtCOLL_ANIM_SCN, scene) == 0x70, "cdtCOLL_ANIM");

// Obj_anim.obj
objOBJ *__fastcall objGetTplObj(objOBJ *o);                                // v12 0x004DF3F0 (feb 0x004D1170)
void __fastcall    _objValidateLTM(objOBJ *o);                             // v12 0x004DFD30 (feb 0x004D1AB0)
void __fastcall    _objValidateAnim(objOBJ *o);                            // v12 0x004DFDA0 (feb 0x004D1B20)

// ---------------------------------------------------------------------------------------------
// Functions of the original that exist (vtable slots, callees) but are not rebuilt yet. Calling one
// stops the program with its name and 1.2 address (never silently does nothing). These bodies carry no
// "// v12" header line, so the wrrecon.dll swap table never patches the game with them.
// ---------------------------------------------------------------------------------------------
extern "C" void wrNotRebuilt(const char *fn, unsigned v12addr);
#define WR_NOT_REBUILT(addr) wrNotRebuilt(__PRETTY_FUNCTION__, addr)

// objOBJ face access shared by the collision code: vertex k (0..2) of face f, in world space unless the
// object's matrix is frozen into its vertices (objSTATE bit 1, see Obj_anim.cpp).
inline m3dV *objFaceVertPtr(objOBJ *o, int f, int k)
{
    objGEOM *g = o->geom;
    u16 *rec = (u16 *)((u8 *)g->faceList + g->faceSize * f + 2);
    return &g->vert[rec[k]];
}
inline void objFaceVertW(objOBJ *o, int f, int k, m3dV *out)
{
    m3dV *v = objFaceVertPtr(o, f, k);
    if (o->state & 1)
        *out = *v;
    else
        m3dTransformPoint(v, (m3dMATR *)o->ltm, out);
}
