// collision.h -- Will Rock 1.2 unofficial SDK, engine_core: collision detection ("cdt")
//
// objs covered: engine:cdt_sys.obj, cdt_scn.obj, cdt_obj.obj, cdt_sys_anim.obj, Cdtm_sys.obj, cdto_math.obj,
//               cdt2_sgen.obj, scn_coll.obj (scene queries: ShootRay / FloorPlace / GetClosestPoint)
// Assumed: m3dV, m3dBOX, m3dSPHERE, scnSCENE, animINST (anim.h), objOBJ (object.h).
//
// Design: static level geometry = one cdtCOLL_SCN (voxel grid of face ids, read from the .scn chunk 0x21F,
// old format 0x21C) hung at scnSCENE+0x50. Every animINST that collides has a cdtCOLL_ANIM grid (built by
// animINST::CDTInit, invalidated on movement by animINST::Invalidate(0x40000)). Queries go through
// scnSCENE (ShootRay, FloorPlace, GetClosestPoint, ConstrSphere) with a cdtREFINE filter object and return
// a cdtINFO. All pure math on the CPU: nothing platform specific here.
// Layouts: unchanged feb -> v12 (all scnSCENE query functions and cdtCOLL_* methods match "exact").

#pragma once
#include "core_types.h"
#include "../math_app/m3d.h"
class animINST; class objOBJ; class scnSCENE;

enum cdtINFO_FLAGS {
    CDTI_HIT        = 0x002,       // something was hit (CheckCDTInfo)
    CDTI_NO_RESULT  = 0x004,       // caller passed no cdtINFO (ShootRay sets it on its local)  // ?
    CDTI_ID_IN_INST = 0x100,       // objId is an objOBJ::idHier inside 'inst' (else a scene object index)
};
struct cdtINFO {                   // size 0x20 (zeroed with 8 dwords in ShootRay)
    u32       flags;               // +0x00 cdtINFO_FLAGS
    animINST *inst;                // +0x04 hit instance, 0 = static level geometry
    int       objId;               // +0x08 object (scene: index into scene object table +0x4C; inst: idHier)
    int       faceId;              // +0x0C face index inside that object (< objOBJ::nFace)
    union { float point[3]; float normal[3]; }; // +0x10 hit / closest point (verified: cdtCOLL_OBJ::ShootRay /
                                  //   GetClosestPoint write it, FloorPlace reads .y as the floor height). 'normal' is
                                  //   the old SDK name, kept as an alias; the face normal comes from scnSCENE::GetCDTFace.
    float     dist;                // +0x1C distance along the ray (ShootRay shortens the ray to it)
};

enum cdtREFINE_MODE {              // cdtREFINE::mode (+0x04), test in cdtREFINE::IsInclInst v12 0x004DDD00 (feb 0x004D04B0)
    CDTR_NO_STATIC     = 0x01,     // skip the static level grid
    CDTR_ONLY_DYNAMIC  = 0x02,     // only instances with flags 0x48  (and with 8: skip instances entirely)
    CDTR_NO_FLAG8      = 0x04,     // skip instances with flags & 8
    CDTR_NO_DYNAMIC    = 0x08,     // skip instances with flags & 0x48
};
// Instances with animINST::flags & 0x11 never collide (0x10 = ANIM_ST_IGNORE_CDT).
// vtable cdtREFINE v12 0x005FA0E0 (feb 0x005CF370) (verified: v12 ctor 0x0040C520 stores it)
class cdtREFINE {                  // size 0x10 in 1.0/1.2 (feb 0x0C): v12 ctor 0x0040C520 stores excl at +0x0C
public:
    virtual BOOL32 IsInclInst(animINST *i);   // slot +0x00 v12 0x004DDD00 (feb 0x004D04B0) flags 0x11 never; mode bits; != exclude
    virtual BOOL32 IsInclObj(objOBJ *o);      // slot +0x04 v12 0x0040C550 (feb 0x0040B650) objOBJ::procState: (&0x1F)==0 or &0x10000, and !(&0x40000)
    virtual BOOL32 IsUseBVolEncl(animINST *i);// slot +0x08 v12 0x004DDD60 NEW in 1.0/1.2 (name ours): 0 = test the per-bone hit boxes
                                              //   (queryFlags & 0x20, instance has hit boxes, and not a physics object)
    u32       mode;                // +0x04 cdtREFINE_MODE (FloorPlace callers use 2)
    u32       queryFlags;          // +0x08 NEW in 1.0/1.2: the flags of the running query (written by the scene queries)
    animINST *exclude;             // +0x0C never hit this instance (usually the caller itself)  [feb: +0x08]
    cdtREFINE(int mode, animINST *excl);      // v12 0x0040C520 (feb 0x0040B620) mode = 0 | mode (inline in most callers)
    cdtREFINE() : mode(0), queryFlags(0), exclude(0) {}  // inline zero-initialised form (re/mpsrc)
};
// cdtINFO_SPHERE: the query object gsMP_SYSTEM::SelectStartPosDir builds (vtable v12 0x005FA0D4, 12 bytes before
// cdtREFINE's, so probably a cdtREFINE-family class). Fields as re/mpsrc uses them; offsets unverified. // ?
class cdtINFO_SPHERE {
public:
    virtual ~cdtINFO_SPHERE();
    animINST *ignoreInst;          // ? excluded instance (the spawning entity)
    m3dV     *pos_;                // ? (placeholder so the layout is not claimed)
    m3dV      pos;                 // ?
    float     hitFraction;         // re/mpsrc: +0x30 ?
};
// Subclasses: entCDT_REFINE_INCL / entCDT_REFINE_EXCL (gs:entity / physics), physCDT_REFINE,
//   plrCDT_REFINE, aiCYCLOP_REFINE, rgdRIGID_START (IsInclObj) ... a mod can derive its own filter.

// --- scene queries (scnSCENE methods, __thiscall on gsScenePtr; gsScenePtr v12 0x0098E81C (feb 0x0094821C)) ---
// int  scnSCENE::ShootRay(m3dV *from, m3dV *dir, float len, int flags, cdtREFINE *r, cdtINFO *out);
//                                  v12 0x004DE9F0 (feb 0x004CF890)  static grid via cdtCOLL_SCN vslot +0x58, then instances
// int  scnSCENE::IsectRayInst(...)       v12 0x004DED20 (feb 0x004CFBD0)
// int  scnSCENE::IsVisiblePoint(...)     v12 0x004DEFD0 (feb 0x004CFE80)
// int  scnSCENE::FloorPlace(m3dV *pos, float maxDown, m3dV *out, float h0, float h1, int, cdtREFINE *r, cdtINFO *out);
//                                  v12 0x004DF2B0 (feb 0x004D0370)  maxDown clamped to 10000.0
// int  scnSCENE::GetClosestPoint(...)    v12 0x004DB2B0 (feb 0x004CC250)
// int  scnSCENE::ConstrSphere(...)       v12 0x004DB000 (feb 0x004CBFA0)  push a sphere out of geometry (movement)
// int  scnSCENE::CheckCDTInfo(cdtINFO*)  v12 0x004DBAF0 (feb 0x004CCA90)
// objOBJ* scnSCENE::GetCDTObj(cdtINFO*)  v12 0x004DBE70 (feb 0x004CCE30)
// int  scnSCENE::GetCDTFace(cdtINFO*, ...) v12 0x004DBB70 (feb 0x004CCB10)  (results/weapon_npc.json)

// --- collision structures ---
// cdtCOLL (base, voxel grid): ctor v12 0x00503580 (feb 0x004F11E0), AllocLists 0x005036F0, CalcData2 0x00502D70,
//   MakeIndList 0x00502780, MakeIndListRay 0x00502A40.     vtable feb 0x005DC43C (v12 ? 0x006078EC data map)
// cdtCOLL_SCN (level):  IsIsectRayBVol v12 0x004DE680, IsNearBVol 0x004DE6B0.   vtable feb 0x005DCFA0
// cdtCOLL_INST:         SetCDTFrame v12 0x004DE820, IsCDTFrame 0x004DE840.      vtable feb 0x005DB9B8
// cdtCOLL_OBJ (per object): GetFacePoly v12 0x00501DF0, GetFaceNormal 0x00502030, ShootRay 0x004F5CB0,
//   ShootCyl 0x004F6170, GetClosestPoint 0x004F6730/0x004F6A10/0x004F6CD0.         vtable feb 0x005DBA28
// cdtCOLL_ANIM (per instance): InitData v12 0x004FE390, Invalidate 0x004FE5C0, Validate 0x004FE620,
//   RemoveFromGrid 0x004FE810, MakeIndList 0x004FE8D0, MakeIndListRay 0x004FEA30. vtable feb 0x005DC2F8
// cdtOFL (face list scratch): ctor v12 0x00503910, global cdtOflPtr v12 0x00978758 (feb 0x00934510).
// cdtoInit v12 0x004F5C50 / cdtoTerm 0x004F5C90 (feb 0x004E1A10 / 0x004E1A50).
// .scn reader: _sioReadColl v12 0x0059F790 (feb 0x00583460), _sioReadColl_OLD v12 0x0059F9D0.
