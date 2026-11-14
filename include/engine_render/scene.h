// Will Rock 1.2 unofficial SDK - engine render: scnSCENE (level container), scene file reader (sio*), domains
// objs: engine:Scene.obj, scn_vis.obj, scn_coll.obj, scn_domain.obj, sceneio*.obj
// Assumes from re/sdk/core: objOBJ, animINST (size 0x14C), animTPL (size 0xCC), cdtCOLL_SCN, camCAMERA, psSHEET,
//                           selSEL, fioFILE, m3dV, cdtREFINE, cdtINFO, lgtLIGHT_GROUP
// All scnSCENE methods marked exact are identical in v12 -> offsets verified.
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include <stdint.h>

// ---------------------------------------------------------------- limits (TWEAK)
#define SCN_MAX_TPL    300    // template pool; v12 push 0x12C at 0x004E5E23 (ctor), cmp 0x12C at 0x004E5EAD, 0x004E6426 (AllocTpl), 0x004E6E73/0x004E6E96 (FindNextTpl)
#define SCN_MAX_INST   1000   // instance pool; v12 push 0x3E8 at 0x004E5E4D (ctor), cmp at 0x004E64A2 (_FindInstEmpty), 0x004E6526 (AllocInst)
                              // loops use end offset 1000*0x14C = 0x510E0: v12 0x004E5E92, 0x004E6213 (DestroyLevel), 0x004E5F86 (dtor)
#define SCN_TPL_SIZE   0xCC   // sizeof(animTPL) (pool stride) both builds
#define SCN_INST_SIZE  0x14C  // sizeof(animINST) (pool stride) both builds
#define SCN_TPL_MAGIC  0x4154504C // 'LPTA' written to tpl[0] when a template slot is in use

// ---------------------------------------------------------------- scene file chunks (sioReadScene)
// The .scn/level file is a fioFILE chunk stream (fioCHUNK: uint16 id + size). sioReadScene v12 0x0059E680 (feb 0x00582350)
enum SIO_CHUNK {
    SIO_CHUNK_OBJ          = 0x0F0,  // root objOBJ hierarchy (static level geometry)      -> scene->root
    SIO_CHUNK_TEX_A        = 0x154,  // texture list (0x154..0x155)                        -> sioReadTex
    SIO_CHUNK_TEX_B        = 0x155,
    SIO_CHUNK_INST_LIST    = 0x1B8,  // animINST list                                      -> sioReadInstList
    SIO_CHUNK_TPL_LIST     = 0x1EA,  // animTPL list                                       -> sioReadTplList
    SIO_CHUNK_COLL_OLD     = 0x21C,  // collision (old format)                             -> scene->coll
    SIO_CHUNK_NAMED_SEL    = 0x21D,  // named selection sets                               -> scene->nSel/sel
    SIO_CHUNK_PSHEET       = 0x21E,  // global scene script (psSHEET)                      -> scene->sheet
    SIO_CHUNK_COLL         = 0x21F,  // collision                                          -> scene->coll
    SIO_CHUNK_LIGHT_LIST   = 0x280,  // light list                                         -> sioReadLightList
    SIO_CHUNK_CAMERA       = 0x3AC,  // camera                                             -> scene->camera
};
// v12 compares: 0x0059E6EF (0x21D), 0x0059E6FC (0x1B8), 0x0059E705 (0xF0), 0x0059E786 (0x280), 0x0059E7E1 (0x3AC)

extern class fioFILE *_sioFilePtr;    // v12 0x009AF8A0 (feb 0x0096442C) current scene file (245 xrefs v12)
extern int           *_sioTexListNmb; // v12 0x009AF898 (feb 0x00964424) texture index remap while reading

void __fastcall sioSetSourceFile(fioFILE *);                  // v12 0x0059E670 (feb 0x00582340)
// camCAMERA (cam*.obj) is not reconstructed here; only what other code calls. // ?
#include "../math_app/camera.h"   // camCAMERA (full class)
void __fastcall _scnDumpInstList(class scnSCENE *s);           // v12 0x004E7480 (feb 0x004DEB40)
void __fastcall _scnCalcRadRenderOff(animINST *inst);   // feb 0x004DEB30 (Scene.obj); v12 ? (called after MP create/sync, re/mpsrc)
int __fastcall  sioReadScene(class scnSCENE *);               // v12 0x0059E680 (feb 0x00582350) main reader (see SIO_CHUNK)
int __fastcall  sioReadSceneTex();                            // v12 0x0059E840 (feb 0x00582510) exact
int __fastcall  sioReadTemplate(animTPL *, int);              // v12 0x0059E900 (feb 0x005825D0) exact
objOBJ __fastcall *_sioReadObj();                             // v12 0x0059FBA0 (feb 0x00583870) callgraph

// ---------------------------------------------------------------- scnSCENE
// size 0x8C (ctor clears 0x23 dwords). Global instance pointer: gsScenePtr (game side, see re/sdk/core or gs headers).


// selSEL: a named selection = sorted list of object names (select.obj). Elements of scnSCENE::sel (0x4C each).
struct selOBJ { int id; char name[0x40]; };            // 0x44 per entry (the name compares as apNAME at +4)
class selSEL {
public:
    u32     flags;                 // +0x00 bit 1: sorted (Prepare), Find uses bsearch
    char    name[0x40];            // +0x04 ("_lm" in the name: animated lightmap selection, SetAnimLightFlags)
    int     n;                     // +0x44
    selOBJ *obj;                   // +0x48 operator new(n * 0x44)
    BOOL32 Init(int n);            // v12 0x004F4270 (feb 0x004E0030)
    int    Find(char *name);       // v12 0x004F42D0 (feb 0x004E0090) index or -1
    void   Prepare();              // v12 0x004F4370 (feb 0x004E0130) sort for Find
};
int __cdecl _selCompSelObj(const void *a, const void *b);   // v12 0x004F4390 (feb 0x004E0150)
int __cdecl _selSearchSelObj(const void *key, const void *b); // v12 0x004F43E0 (feb 0x004E01A0)
// scnLIGHT: one level light (chunk 0x281 in the 0x280 list; read by _sioReadLightList v12 0x0059F3E0).
// Sub-chunk ids in brackets; meaning of the numbers after the colour is ? (read as pairs / singles).
struct scnLIGHT {
    char     name[0x40];           // +0x00 [0x282]
    u32      flags;                // +0x40 [0x281] second dword; 0x1000 = animated lightmap ("_lm", SetAnimLightFlags)
    int      type;                 // +0x44 [0x281] first dword  // ?
    m3dMATR  matr;                 // +0x48 [0x283] position / orientation
    float    color[3];             // +0x88 [0x284]  // ? r g b
    float    _94;                  // +0x94 not read
    float    _98, _9c;             // +0x98 [0x285]  // ? attenuation / range
    float    _a0, _a4;             // +0xA0 [0x286]  // ? cone angles
    float    _a8;                  // +0xA8 [0x287]  // ? intensity
    selSEL   sel;                  // +0xAC [0x348] named selection of the objects it lights
    psSHEET *sheet;                // +0xF8 [0x288] the light's script
};                                 // size 0xFC

// The animated lights grouped by the named selection holding them (scnSCENE::GetAnimLights, size 0x1D4)
class lgtLIGHT_GROUP {
public:
    char      name[0x40];          // +0x00 (only [0] cleared)
    int       n;                   // +0x40
    scnLIGHT *light[100];          // +0x44
};

// scnDOMAIN (scn_domain.obj): a vertical volume = the XZ outline of a flat object (its faces as 2D triangles)
// between two heights. Fog and water volumes (pteFOG_GROUND), AI areas (aiAREA), trigger domains.
// size 0x3C; vtable v12 0x005FCF28, slot order verified on v12.
class scnDOMAIN {
public:
    virtual void  *__vecDelDtor(unsigned flags);           // slot +0x00 (folded copy: v12 0x0043A2D0)
    virtual BOOL32 Init();                                 // slot +0x04 v12 0x004EFB10 (feb 0x004DC1B0) from obj's faces
    virtual void   Term();                                 // slot +0x08 v12 0x004F01D0 (feb 0x004DC870)
    virtual BOOL32 IsBelongPoint(m3dV *p);                 // slot +0x0C v12 0x004F0430 (feb 0x004DCAD0)
    virtual BOOL32 IsBelongPointXZ(m3dV *p);               // slot +0x10 v12 0x004F0560 (feb 0x004DCC00)
    virtual BOOL32 IsLSegCrossBoundaryXZ(m3dV *a, m3dV *b); // slot +0x14 v12 0x004F0610 (feb 0x004DCCB0)
    virtual BOOL32 ISectLSegBoundaryXZ(m3dV *a, m3dV *b, m3dV *out); // slot +0x18 v12 0x00502560 folded "return 0"
    virtual BOOL32 IsIsectBox(m3dBOX *b);                  // slot +0x1C v12 0x004F06D0 (feb 0x004DCD80)
    virtual BOOL32 _IsIsectObj(objOBJ *o);                 // slot +0x20 v12 0x004F0750 (feb 0x004DCE00)
    virtual BOOL32 IsIsectObj(objOBJ *o);                  // slot +0x24 v12 0x004F0BD0 (feb 0x004DD280) o or a child
    virtual BOOL32 _IsIsectInst(animINST *i);              // slot +0x28 v12 0x004F0C80 (feb 0x004DD330) by its position
    virtual BOOL32 IsIsectInst(animINST *i);               // slot +0x2C v12 0x004F0C60 (feb 0x004DD310)
    int         nTri;              // +0x04 0 = everywhere
    float       yMin, yMax;        // +0x08 +0x0C (equal: no height limit except "not below yMin")
    m3dBOX_DATA bbox;              // +0x10 (CalcBBox)
    struct m2dTRI *tri;            // +0x30 apCalloc(nFace, 0x18): the object's faces in XZ
    objOBJ     *obj;               // +0x34 the outline object
    int         inside;            // +0x38 1: boxes / objects must lie entirely inside (else: touch)
    scnDOMAIN();                                           // v12 0x00548C40 (feb 0x00535300; an inline's copy, ai_ctrl.obj)
    void   SetHeight(float h);                             // v12 0x004F0200 (feb 0x004DC8A0) from the plane, up (h > 0) or down
    void   CalcBBox();                                     // v12 0x004F0CC0 (feb 0x004DD370)
    void   GetRandomPoint(m3dV *out);                      // v12 0x004F0DA0 (feb 0x004DD450)
    void   GetPoly(int i, struct m2dPOLY *out);            // v12 0x004F0E20 (feb 0x004DD4D0)
};

class scnSCENE {
public:
    char       name[0x40];   // v12 +0x00 (feb +0x00)  // ? (cleared by ctor, not referenced by name in Scene.obj)
    union { int _40; int flags; }; // v12 +0x40 (feb +0x40) ctor = 0; bit 0 set by wrGO on MP servers (re/mpsrc) // ?
    objOBJ    *root;         // v12 +0x44 (feb +0x44) static level geometry root (chunk 0xF0); verified v12 0x0059E731
    int        nObj;         // v12 +0x48 (feb +0x48) number of objects in flat list (InitLevel)
    objOBJ   **objList;      // v12 +0x4C (feb +0x4C) flat object list -> rendDRIVER::RenderObjListStatic
    cdtCOLL_SCN *coll;       // v12 +0x50 (feb +0x50) collision scene (chunk 0x21C/0x21F); verified v12 0x0059E75C
    class cdtCOLL_ANIM *collAnim; // v12 +0x54 (feb +0x54) created in InitLevel
    animTPL   *tplPool;      // v12 +0x58 (feb +0x58) SCN_MAX_TPL * 0xCC
    animINST  *instPool;     // v12 +0x5C (feb +0x5C) SCN_MAX_INST * 0x14C
    animINST  *instFirst;    // v12 +0x60 (feb +0x60) circular list head (link at animINST +0x58)
    int        _64;          // v12 +0x64 (feb +0x64)
    int        nTmpV3D;      // v12 +0x68 (feb +0x68) capacity of tmp vertex buffer (max verts of any obj)
    m3dV      *tmpV3D;       // v12 +0x6C (feb +0x6C) 12 bytes per vertex
    int        nLights;      // v12 +0x70 (feb +0x70)
    scnLIGHT  *lights;       // v12 +0x74 (feb +0x74) 0xFC per light
    int        nSel;         // v12 +0x78 (feb +0x78) named selections (chunk 0x21D); verified v12 0x0059E77C
    selSEL    *sel;          // v12 +0x7C (feb +0x7C)
    psSHEET   *sheet;        // v12 +0x80 (feb +0x80) global scene script (chunk 0x21E)
    camCAMERA *camera;       // v12 +0x84 (feb +0x84) chunk 0x3AC; verified v12 0x0059E7FF
    animTPL   *dummyTpl;     // v12 +0x88 (feb +0x88) "__DUMMY__" template allocated by ctor

    scnSCENE();                                         // v12 0x004E5E10 (feb 0x004DD520) exact
    ~scnSCENE();                                        // v12 0x004E5F60 (feb 0x004DD670) exact
    int  InitLevel();                                   // v12 0x004E6010 (feb 0x004DD720) builds objList, normals, bbox, collAnim
    void DestroyLevel(int);                             // v12 0x004E61E0 (feb 0x004DD8E0) exact
    animINST *InstPtr(int i);                           // v12 0x004E5FF0 (feb 0x004DD700) instPool + i*0x14C
    int  IsEmptyInst(animINST *);                       // v12 0x004E5FE0 (feb 0x004DD6F0)
    animTPL *FindTplName(char *);                       // v12 0x004E63A0 (feb 0x004DDAA0) exact
    animTPL *AllocTpl();                                // v12 0x004E6410 (feb 0x004DDB10) exact
    void DestroyTpl(animTPL *);                         // v12 0x004E6470 (feb 0x004DDB70)
    animINST *AllocInst();                              // v12 0x004E6510 (feb 0x004DDC10) exact
    int  _FindInstEmpty();                              // v12 0x004E6490 (feb 0x004DDB90) exact  first free pool slot, -1
    void _AttachInst(animINST *i);                      // v12 0x004E64C0 (feb 0x004DDBC0) exact  into the active ring
    animINST *CreateInst(animTPL *, class animCREATE_DATA *, int); // v12 0x004E6620 (feb 0x004DDD20) HOOK: every spawned instance
    animINST *CreateInst(objOBJ *, animCREATE_DATA *);  // v12 0x004E6950 (feb 0x004DE050)
    void DestroyInst(animINST *);                       // v12 0x004E6CD0 (feb 0x004DE390) exact  HOOK: instance removal
    animINST *FindInstName(char *);                     // v12 0x004E6D90 (feb 0x004DE450) exact
    void ForAllInst(int (__fastcall *)(animINST *, void *), void *); // v12 0x004E6DE0 (feb 0x004DE4A0) exact
    animINST *FindNextInst(animINST *);                 // v12 0x004E6E20 (feb 0x004DE4E0) exact  iterate: p = FindNextInst(0) ... until 0
    animTPL  *FindNextTpl(animTPL *);                   // v12 0x004E6E50 (feb 0x004DE510) exact
    int  GetNInst();                                    // v12 0x004E6EC0 (feb 0x004DE580) exact
    int  GetNTpl();                                     // v12 0x004E6EF0 (feb 0x004DE5B0) exact
    void SetAnimLightFlags();                           // v12 0x004E6F60 (feb 0x004DE620) exact  marks lights whose name contains "_lm"
    int  GetAnimLights(int *, lgtLIGHT_GROUP **, class txmTEXTURE_LAY ***); // v12 0x004E7030 (feb 0x004DE6F0) exact
    int  ExpandV3DTmpList(objOBJ *);                    // v12 0x004E7390 (feb 0x004DEA50) exact
    void SetStateYesInst(animINST *, int);              // v12 0x004E73E0 (feb 0x004DEAA0) exact

    // visibility / ray queries (scn_vis.obj)
    int  ShootRay(m3dV *from, m3dV *to, float, int, cdtREFINE *, cdtINFO *);          // v12 0x004DE9F0 (feb 0x004CF890) exact
    int  IsectRayInst(animINST *, m3dV *, m3dV *, float, cdtREFINE *, cdtINFO *);     // v12 0x004DED20 (feb 0x004CFBD0)
    int  IsVisiblePoint(m3dV *, m3dV *, int, cdtREFINE *, cdtINFO *);                 // v12 0x004DEFD0 (feb 0x004CFE80) line of sight
    int  IsVisiblePointCone(m3dV *from, m3dV *dir, float front, float back, float angle, m3dV *to, int flags, cdtREFINE *r, cdtINFO *ci); // v12 0x004DF130
    int  ConstrSphere(m3dV *pos, m3dV *out, float radius, int iter, cdtREFINE *r, cdtINFO *ci); // v12 0x004DB000 (feb 0x004CBFA0) push a sphere out of geometry
    int  FloorPlace(m3dV *, float, m3dV *, float, float, int, cdtREFINE *, cdtINFO *);// v12 0x004DF2B0 (feb 0x004D0370) exact
    // the rest of scn_vis.obj / scn_coll.obj (rebuilt in src_recon/src/engine)
    int  _ShootRayAnim(m3dV *org, m3dV *dir, float len, int flags, cdtREFINE *r, cdtINFO *ci);      // v12 0x004DEB20 (feb 0x004CF9C0)
    int  _IsectRayInst(animINST *, m3dV *org, m3dV *dir, float len, cdtREFINE *r, cdtINFO *ci);    // v12 0x004DEDC0 (feb 0x004CFC70)
    int  MakeAnimList(m3dV *a, m3dV *b, float r, cdtREFINE *ref, struct scnAOBJ *out, int max);     // v12 0x004DA6A0 (feb 0x004CB6D0)
    int  MakeAnimList(m3dBOX *box, cdtREFINE *ref, struct scnAOBJ *out, int max);    // v12 0x004DA3B0 (feb 0x004CB3F0)
    int  _ConstrSphere_1(m3dV *from, m3dV *pos, float r, int flags, cdtREFINE *ref, cdtINFO *ci);  // v12 0x004DAA10 (feb 0x004CBA20)
    int  _ConstrSphere(m3dV *from, m3dV *to, float r, int flags, cdtREFINE *ref, cdtINFO *ci);     // v12 0x004DB040 (feb 0x004CBFE0)
    int  GetClosestPoint(m3dV *p, float r, int flags, cdtREFINE *ref, cdtINFO *ci);                // v12 0x004DB2B0 (feb 0x004CC250) exact
    int  _GetClosestPointAnim(m3dV *p, float r, int flags, cdtREFINE *ref, cdtINFO *ci);           // v12 0x004DB370 (feb 0x004CC310)
    int  _DistPointInst(animINST *, m3dV *p, float r, cdtREFINE *ref, cdtINFO *ci);                // v12 0x004DB790 (feb 0x004CC750)
    int  DistPointObj(objOBJ *o, m3dV *p, float r, int flags, cdtINFO *ci);                       // v12 0x004DBA00 (feb 0x004CC9B0)
    int  CheckCDTInfo(cdtINFO *ci);                                                               // v12 0x004DBAF0 (feb 0x004CCA90) exact
    void GetCDTFace(cdtINFO *ci, m3dPOLY *poly, m3dV *normal);                                    // v12 0x004DBB70 (feb 0x004CCB10)
    objOBJ *GetCDTObj(cdtINFO *ci);                                                               // v12 0x004DBE70 (feb 0x004CCE30) exact
};
// One candidate of MakeAnimList: an instance and one of its hit boxes (-1 = the enclosing volume).
struct scnAOBJ { animINST *inst; int bvol; };
extern int scnVisPointActive;                                 // v12 0x00978698 (feb 0x0093445C) 1 while IsVisiblePoint runs (name ours)

// scnDOMAIN: declared above scnSCENE.
