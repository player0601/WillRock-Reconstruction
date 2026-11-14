// anim.h -- Will Rock 1.2 unofficial SDK, engine_core: animation (animTPL / animINST / animSEQ /
//           animSYSTEM state machines) and bounding volumes (Animbvol / abv_encl)
//
// objs covered: engine:anim.obj, anim_anim.obj, anim_ctrl.obj, anim_chn.obj, anim_clean.obj, animkinem.obj,
//               Obj_anim.obj, Animbvol.obj, abv_encl.obj, sceneio_tpl.obj (.tpl reader, see fileio.h)
// Assumed: m3dV, m3dMATR, m3dBOX, m3dSPHERE, m3dQUAT, psSHEET, scnSCENE, cdtCOLL_ANIM (collision.h).
//
// Model:  animTPL  = a template = one ".tpl" file: object hierarchy + animation sequences + bounding
//                    volumes + property sheet. Shared by all instances. Pool of 300 in scnSCENE.
//         animINST = one placed copy in the scene (pool of 1000 in scnSCENE, NOT heap allocated): its own
//                    duplicated objOBJ tree, world matrix, current sequence/time, props, owning entity.
//         animSEQ  = one animation clip of a template (stride 0x88).
//         animSYSTEM(_GROUP/_INST) = transition table between sequences ("ani system") used by NPCs/player.
// Layouts of animINST / animTPL are IDENTICAL in feb and v12 (every accessor compared: SetAnimSeq,
// AdvanceTime, GetPos, GetStr ... "exact" matches; scnSCENE::AllocInst uses 0x14C/0x53 dwords in both).

#pragma once
#include "core_types.h"
#include "../math_app/m3d.h"   // m3dV, m3dMATR (animCREATE_DATA embeds them)
#include "object.h"

struct m3dBOX; struct m3dQUAT;
class scnSCENE; class entENTITY; class animTPL; class animSEQ; class abvBVOL; class cdtCOLL_ANIM; class selSEL; class camCAMERA;

// ------------------------------------------------------------------------------------------------
enum animINST_FLAGS {               // animINST::flags (+0x04); also OR'ed from animTPL::flags at creation
    ANIM_ST_FROZEN_A        = 0x00000001, // (flags & 3) != 0 -> AdvanceTime does nothing (unless 4 / "Actor_cap_1") // ?
    ANIM_ST_FROZEN_B        = 0x00000002,
    ANIM_ST_ANIMATE_ALWAYS  = 0x00000004, // ?
    ANIM_ST_IGNORE_CDT      = 0x00000010, // ANIM_ST_IGNORE_CDT_ALWAYS (entENTITY::ParsePS) / physMATH INIT
    ANIM_ST_LOADED_FROM_SCN = 0x00000800, // set by _sioReadInstList
    ANIM_ST_EXCLUDE_HH      = 0x00001000, // ANIM_ST_EXCLUDE_HH prop
    ANIM_ST_SEQ_CHANGED     = 0x0000E000, // SetAnimSeq sets these 3 bits
    ANIM_ST_MODIFIERS       = 0x20000000, // objects have modifiers (Invalidate path)
};
enum animINVALID {                  // animINST::invalid (+0x0C) bits for Invalidate()/Validate()
    ANIM_INV_POS     = 0x00000004,  // world matrices (Validate(4) before reading bone matrices)
    ANIM_INV_SEQ     = 0x00000008,
    ANIM_INV_FRAME   = 0x00000010,
    ANIM_INV_BBOX    = 0x00000080,  // ?
    ANIM_INV_BLEND   = 0x00000100,
    ANIM_INV_CDT     = 0x00040000,  // collision grid must be rebuilt (cdtCOLL_ANIM::Invalidate)
    ANIM_INV_MODS    = 0x00020000,
};

// ------------------------------------------------------------------------------------------------
class animINST {                    // size 0x14C, NO vtable
public:
    u32        magic;               // +0x000 'SNIA' (0x41494E53) ("ANIS"); checked by .scn loader
    u32        flags;               // +0x004 animINST_FLAGS
    u32        unk08;               // +0x008
    u32        invalid;             // +0x00C animINVALID bits (Invalidate v12 0x004CD770 (feb 0x004C3030))
    objOBJ    *root;                // +0x010 duplicated object tree (objOBJ::inst points back here)
    int        index;               // +0x014 slot in scnSCENE instance pool
    char      *name;                // +0x018 instance name (apStrdup; SetName)
    char      *className;           // +0x01C (SetNameClass) entity class to create, e.g. "phys"
    char      *tplName;             // +0x020 (SetNameTpl)
    float      seqTime;             // +0x024 seconds into current sequence
    u32        actionFrames;        // +0x028 bits of action frames crossed by the last AdvanceTime (animSEQ::IsActionFrame)
    u32       *seqActionFrames;     // +0x02C per sequence: action-frame bits kept by the ani system (IsActionFrame(seq, bit))
    int        blendSeq0;           // +0x030 SetSeqBlendParam: blend from this sequence ...
    int        blendSeq1;           // +0x034 ... into this one
    float      blendTime0;          // +0x038
    float      blendTime1;          // +0x03C
    float      blendFrame0;         // +0x040 StartApplyAnim: TimeToFrame of the two (-2.0 when blendSeq0 < 0)
    float      blendFrame1;         // +0x044
    float      blendWeight;         // +0x048
    u8         unk4C[0x58 - 0x4C];
    animINST  *listNext;            // +0x058 scene active ring
    animINST  *listPrev;            // +0x05C
    float      matr[16];            // +0x060 m3dMATR local-to-world of the whole instance (GetPos = origin of this)
    float      matrW2L[16];         // +0x0A0 m3dMATR world-to-local (inverse of matr), valid after Validate(0x40)
                                    //        (abvOBB_UPD::MakeUpdStruct v12 0x004F1A40 copies it when obj == 0)
    union { int seq; int linkValue; }; // +0x0E0 current sequence index (-1 none) [re/mpsrc: linkValue, written to link->value after MP sync]
    float      frame;               // +0x0E4 current frame (init -117.0f = 0xC2EA0000)
    struct animLINK { int _0, _4; int value; } *link; // +0x0E8 (re/mpsrc: ->value (+8) = linkValue after MP sync) // ?
    objOBJ    *centerObj;           // +0x0EC object whose origin GetCenter returns (0: enclosing volume / origin)
    float      renderOff;           // +0x0F0 "renderOff" prop (distance)  // ?
    // bounding volumes (hit boxes), engine:Animbvol.obj / abv_encl.obj. Earlier SDK drafts called +0xFC/+0x100
    // "seqData"/"seqDataCur" (stride 0x24): the stride is sizeof(abvBVOL). Verified v12 AbvInit 0x004F2740,
    // AbvTerm 0x004F2D70 (array-deletes +0xF8 and +0xFC, deletes +0x100 only when unk08 & 1).
    int        nBVol;               // +0x0F4 = tpl->nBVol
    abvBVOL   *bvol;                // +0x0F8 new abvBVOL[nBVol] (MSVC array, count cookie at -4)
    union { abvBVOL *bvSeq;  void *seqData; };    // +0x0FC enclosing volume per sequence, [tpl->nSeq] (AbvCalcBVolEnclos)
    union { abvBVOL *bvCur;  void *seqDataCur; }; // +0x100 current enclosing volume (= bvSeq + seq, or own copy if unk08 & 1)
    m3dBOX_DATA bbox;               // +0x104 world box of the instance (ValidateBBox; GetBBox copies type/min/max)
    u8         unk124[0x128 - 0x124];
    int        nObj;                // +0x128 objects in the tree
    objOBJ   **objTable;            // +0x12C by objOBJ::idHier
    int        tplObjId;            // +0x130 -1 or template sub-object id it was made from
    union { psSHEET *props; int seed; }; // +0x134 [re/mpsrc reads it as "animSeed" and copies it into MP create/sync messages; v12 0x00539F86] // ? meaning disputed
    //         props                // +0x134 instance property sheet (falls back to tpl->props, see GetStr)
    animTPL   *tpl;                 // +0x138
    union { entENTITY *ent; entENTITY *owner; }; // [alias owner: re/mpsrc]
    //         ent                  // +0x13C owning entity (0 = static scenery)
    struct animLODS *lods;          // +0x140 LOD set (lod.cpp)
    class cdtCOLL *coll;            // +0x144 own collision data (CDTInit; a cdtCOLL_INST), deleted via slot 0
    scnSCENE  *scene;               // +0x148 (scene +0x54 -> cdtCOLL_ANIM* used by Invalidate)  // ?

    // --- transform ---
    void Translate(m3dV *v, int space);                    // v12 0x004CDFB0 (feb 0x004C3850) (results/weapon_npc.json)
    void SetPosDir_XZ(m3dV *pos, m3dV *dir);               // v12 0x004CE3C0 (feb 0x004C3C60)
    void GetDirUp(m3dV *out);                              // v12 0x004CE410 (feb 0x004C3CB0) Y axis
    int  UpdateInvalidateState(int bits);                  // v12 0x004CD5C0 (feb 0x004C2E80) bits -> every bit they invalidate
    int  UpdateValidateState(int bits);                    // v12 0x004CD6A0 (feb 0x004C2F60) bits -> every bit they need
    void ValidateBBox();                                   // v12 0x004CE5D0 (feb 0x004C3E00)
    BOOL32 CDTInit(m3dV *vox, struct m3dVINT *maxVox, char *seq, float time); // v12 0x004CE810 (feb 0x004C4040)
    BOOL32 GetHEX(char *sect, char *key, u32 *out);        // v12 0x004CEAF0 (feb 0x004C4350)
    void LODTerm();                                        // v12 0x004F3D90 (feb 0x004DFB50)
    void Translate(float x, float y, float z, int space);  // v12 0x004CDF80 (feb 0x004C3820) // ? overload order
    void RotateX(float a, int space);                      // v12 0x004CDF20 (feb 0x004C37C0)
    void RotateY(float a, int space);                      // v12 0x004CDF50 (feb 0x004C37F0)
    void Scale(float x, float y, float z, int space);       // v12 0x004CDFE0 (feb 0x004C3880)
    float GetScale();                                      // v12 0x004CE020 (feb 0x004C38C0) mean of the root's x/y/z scale
    void Transform(m3dMATR *m, int mode);                  // v12 0x004CE090 (feb 0x004C3930) mode 0 = set, 2 = multiply // ?
    void SetPos(m3dV *p);                                  // v12 0x004CE0C0 (feb 0x004C3960)
    void SetDir_XZ(m3dV *dir);                             // v12 0x004CE0F0 (feb 0x004C3990)
    void SetDir_XZ_UsePivot(m3dV *dir, m3dV *pivot);       // v12 0x004CE130 (feb 0x004C39D0) turn about a world pivot
    void GetPos(m3dV *out);                                // v12 0x004CE3F0 (feb 0x004C3C90) = matr origin
    void GetDir(m3dV *out);                                // v12 0x004CE400 (feb 0x004C3CA0) Z axis
    void GetDirRight(m3dV *out);                            // v12 0x004CE420 (feb 0x004C3CC0) X axis
    void GetMatrL2W(m3dMATR *out);                         // v12 0x004CE460 (feb 0x004C3D00)
    void GetCenter(m3dV *out);                             // v12 0x004CE4A0 (feb 0x004C3D40)
    void GetBBox(m3dBOX *out);                             // v12 0x004CE520 (feb 0x004C3DA0)
    float GetSeqLength(int seq);                           // v12 0x004CE580 (1.2 only, name ours) seconds, 0 when out of range
    void Invalidate(int bits);                             // v12 0x004CD770 (feb 0x004C3030)
    void Validate(int bits);                               // v12 0x004CD830 (feb 0x004C30F0)
    // --- animation ---
    void SetAnimSeq(int seq, float time);                  // v12 0x004D13A0 (feb 0x004C4E50)
    BOOL32 AdvanceTime(float dt, int mode, float *overflow);// v12 0x004D1450 (feb 0x004C4F00) mode 0 loop, 1 clamp (returns 1 at end)
    void SetSeqBlendParam(int s0, float t0, int s1, float t1, float w); // v12 0x004D1690 (feb 0x004C5140) sets flags 0x200
    void StopSeqBlend();                                   // v12 0x004D16E0 flags &= ~0x200
    void GetBlendParams(float *f0, float *f1, float *w);   // v12 0x004D16F0 (feb 0x004C51A0)
    BOOL32 CreateAnimSys();                                // v12 0x004CB3D0 (feb 0x004BB420)
    BOOL32 IsActionFrame(int seq, int bit);                // v12 0x004CB410 (feb 0x004BB460) seqActionFrames[seq] & 1 << bit
    void ClearActionFrame();                               // v12 0x004CB460 (feb 0x004BB4B0)
    void SnapshotPose();                                   // v12 0x004F2E20 refit the first hit box to the kept pose (a corpse)
    // animkinem.obj (v12 0x005011B0..0x00501ABF): the pose of the object tree at the current frame
    void ApplyAnim();                                      // v12 0x00501320 (feb 0x004EF1B0)
    void StartApplyAnim();                                 // v12 0x00501340 (feb 0x004EF1E0)
    void CalcObjModelMatrix(objOBJ *o);                    // v12 0x005013C0 (feb 0x004EF260) the subtree
    void _CalcObjModelMatrix(objOBJ *o);                   // v12 0x00501480 (feb 0x004EF340) one object; REWRITTEN in 1.2
    // --- properties (ANIM_PROP section of the psSHEET: instance first, then template) ---
    BOOL32 GetStr(char *sect, char *key, char *out, int n);// v12 0x004CE960 (feb 0x004C41C0)
    BOOL32 GetInt(char *sect, char *key, int *out);        // v12 0x004CE9B0 (feb 0x004C4210)
    BOOL32 GetBool(char *sect, char *key, int *out);       // v12 0x004CEA00 (feb 0x004C4260)
    BOOL32 GetFloat(char *sect, char *key, float *out);    // v12 0x004CEA50 (feb 0x004C42B0)
    BOOL32 GetColor(char *sect, char *key, void *out);     // v12 0x004CEAA0 (feb 0x004C4300)
    // --- names / data ---
    BOOL32 SetName(char *s);                               // v12 0x004CD500 (feb 0x004C2DC0)
    BOOL32 SetNameClass(char *s);                          // v12 0x004CD540 (feb 0x004C2E00)
    BOOL32 SetNameTpl(char *s);                            // v12 0x004CD580 (feb 0x004C2E40)
    void   MakeCreateData(void *cd);                       // v12 0x004CDB80 (feb 0x004C3420)
    void   ApplyCreateData(void *cd);                      // v12 0x004CDCA0 (feb 0x004C3540)
    void   DestroyData();                                  // v12 0x004CDE10 (feb 0x004C36B0)
    BOOL32 SetObjInstRef();                                // v12 0x004CDED0 (feb 0x004C3770) objOBJ::inst = this, subtree
    // --- collision / bounding volumes ---
    BOOL32 CDTInit();                                      // v12 0x004CE6C0 (feb 0x004C3EF0)
    BOOL32 AbvInit();                                      // v12 0x004F2740 (feb 0x004CE6E0) // callgraph; REWRITTEN in 1.2 (OBB hit boxes)
    void   AbvTerm();                                      // v12 0x004F2D70 (feb 0x004CE960)
    void   AbvTermBVolEnclos();                            // v12 0x004F2D40 (feb 0x004CE930)
    void   AbvUpdateBVList();                              // v12 0x004F2DD0 (feb 0x004CE9C0)
    BOOL32 AbvCalcBVolEnclos(int type, int mode, float step); // v12 0x004F7700 (feb 0x004E5210) (abv_encl.obj)
    // --- LOD ---
    void   LODSetTemp(int lod, int absolute);              // v12 0x004F4040 (feb 0x004DFE00) temporary LOD (collision tests)
    void   LODRestore();                                   // v12 0x004F4120 (feb 0x004DFEE0)
    void   LODInit();                                      // v12 0x004F3B50 (feb 0x004DF910)
    void   OBJ_LODInit(objOBJ *o);                         // v12 0x004F3E00 (feb 0x004DFBC0)
    void   LODApply(camCAMERA *cam);                       // v12 0x004F3EA0 (feb 0x004DFC60)
    void   LODEnable(int on);                              // v12 0x004F41A0 (feb 0x004DFF60)
    void   LODFreeze(int on);                              // v12 0x004F4210 (feb 0x004DFFD0)
};
// size 0x14C

class animTPL {                     // size 0xCC, NO vtable. Read by sioReadTemplate.
public:
    u32        magic;               // +0x00 'APTL' (0x4154504C) in the file
    char       tplName[0x40];       // +0x04
    char       className[0x40];     // +0x44 default entity class for instances
    u32        flags;               // +0x84 OR'ed into animINST::flags (0x100 special attach; 0x840 when loaded from .scn)
    objOBJ    *root;                // +0x88
    int        nSeq;                // +0x8C
    animSEQ   *seq;                 // +0x90 array, stride 0x88
    int        defSeq;              // +0x94 sequence set while building the hit boxes (AbvInit: SetAnimSeq(tpl->defSeq, 0))
    int        unk98;               // +0x98
    int        nObjAnim;            // +0x9C object animation tracks (_sioReadObjAnimList, chunk 0x2E8)
    class animOBJ_ANIM *objAnim;    // +0xA0 new animOBJ_ANIM[] (0x40 each, count cookie), DestroyData
    class animSYSTEM   *aniSystem;  // +0xA4 the transition table (entENTITY::SetupAniSystem), deleted through slot 0 (DestroyData)
    int        nSkin;               // +0xA8 skin modifiers (_sioReadObjSkinList v12 0x005A1440, chunk 0x2EE)
    class objMOD_SKIN **skinList;   // +0xAC apCalloc'ed list; each: slot +0x08 Term, slot 0 deleting dtor (DestroyData)
    int        nBVol;               // +0xB0 bounding volumes of the template (from the .tpl, or 1 default)
    abvBVOL   *bvol;                // +0xB4 abvBVOL[nBVol] (MSVC array new)
    abvBVOL   *bvSeq;               // +0xB8 enclosing volume per sequence (AbvCalcBVolEnclos), deleted only if nSeq > 0
    int        nSel;                // +0xBC named selections (sioReadTemplate -> _sioReadNamedSelList, chunk 0x21D)
    selSEL    *sel;                 // +0xC0 new selSEL[] (0x4C each, count cookie; DestroyData frees each ->obj)
    psSHEET   *props;               // +0xC4 template property sheet (the "script" of the .tpl)
    int        _c8;                 // +0xC8 ?

    int  FindSeqName(char *name);   // v12 0x004CD470 (feb 0x004C2D30) (results/engine.json F_TPL_FIND_SEQ)
    BOOL32 IsDummy();               // v12 0x004CD4E0 (feb 0x004C2DA0)
    void DestroyData();             // v12 0x004CD300 (feb 0x004C2BC0)
    BOOL32 AbvInit(abvBVOL *seqEncl); // v12 0x004F25F0 (feb 0x004CE5F0) (located by its call to AbvCalcBVolEnclos)
    void AbvMakeDefaultBVol();      // v12 0x004F2680 NEW in 1.2 (name ours): default hit box, OBB on "ROOT" for ai_* / player*
    void AbvTerm();                 // v12 0x004F2640 (feb 0x004CE6A0)
    BOOL32 AbvCalcBVolEnclos();     // v12 0x004F7F10 (feb 0x004E5A00) (abv_encl.obj)
};

// animOBJ_ANIM: per-object animation data of a template (0x40 bytes). The dtor deletes +0x14, +0x28,
// +0x38, +0x3C through vtable slot +0x04 (deleting dtor at slot 1: m3dSPL objects).
// Chunk 0x2E9 (_sioReadObjAnimList): flags bit 1 = constant pose present (else taken from the object's local
// matrix), 2 / 4 / 8 = position / rotation / scale animated or set.
class animOBJ_ANIM {
public:
    u32   flags;                    // +0x00
    u32   unk04;                    // +0x04 first dword of chunk 0x2E9 // ?
    float pos[3];                   // +0x08 constant position (0x2FA)
    class m3dSPL *spl14;            // +0x14 position spline (0x2EA)
    float rot[4];                   // +0x18 constant rotation, m3dQUAT w,x,y,z (0x2FB)
    class m3dSPL *spl28;            // +0x28 rotation spline (0x2EB)
    float scale[3];                 // +0x2C constant scale (0x2FC)
    class m3dSPL *spl38;            // +0x38 scale spline (0x2EC)
    class m3dSPL *spl3C;            // +0x3C visibility spline (0x2ED): GetVisibility
    ~animOBJ_ANIM();                // v12 0x004CECB0 (feb 0x004C4670)
    // animkinem.obj. GetPos/GetRot/GetScale are new in 1.2 (names ours; feb inlined them in GetObjTRSV, which
    // 1.2 dropped): the spline value at frame f (slot +0x14 GetValueClosest when `closest`), else the constant.
    void   GetPos(float f, int closest, m3dV *out);       // v12 0x005010B0
    void   GetRot(float f, int closest, m3dQUAT *out);    // v12 0x00501100 (inverted after)
    void   GetScale(float f, int closest, m3dV *out);     // v12 0x00501160
    BOOL32 GetVisibility(float f, int closest, float *out); // v12 0x005011B0 (feb 0x004EF050; 1.2 returns 0 without a spline)
};
// animCTRL_3D_FOLLOW: a 3D controller that returns the position of an instance or an object.
class animCTRL_3D_FOLLOW {
public:
    virtual void *__vecDelDtor(unsigned flags);
    u8 _04[0x24 - 0x04];
    animINST *inst;                 // +0x24
    objOBJ   *obj;                  // +0x28
    void   SetFollow(animINST *i, objOBJ *o);           // v12 0x004CEBB0 (feb 0x004C4410)
    BOOL32 IsFollow(animINST *i);                       // v12 0x004CEBD0 (feb 0x004C4430)
    BOOL32 IsFollow(objOBJ *o);                         // v12 0x004CEBF0 (feb 0x004C4450)
    void   GetValue(m3dV *out);                         // v12 0x004CEC10 (feb 0x004C4470) (vtable slot, as a member here)
};
int __fastcall _animSetObjInstRef(objOBJ *o, animINST *i);
// animkinem.obj: a matrix from the parts flagged (2 position, 4 rotation, 8 scale; -1 all). 1.2 added the flags.
void __fastcall _animCombineTRS(u32 flags, m3dV *t, m3dQUAT *r, m3dV *s, m3dMATR *out); // v12 0x00501AF0 (feb 0x004EF940)  // v12 0x004CEC70 (feb 0x004C4630)

class animSYSTEM;
class animCHN;
class animSYSTEM_INST {             // the animation state machine of an instance (animINST::link, +0xE8), 0x34 bytes
public:                             //   src/engine/anim_ctrl.cpp
    animSYSTEM_INST(animINST *a);   // v12 0x004CAA90 (feb 0x004BABD0)
    ~animSYSTEM_INST();             // v12 0x004CAB50 (feb 0x004BAC90)
    int  AdvanceAnimation(animINST *a, int seq, float dt, int force); // v12 0x004CAB60 (feb 0x004BACA0, 1.2 changed)
    void SetGroupCur(int group, float blend); // v12 0x004CB390 (feb 0x004BB3E0)
    animSYSTEM *sys;                // +0x00 the template's (animTPL::aniSystem)
    animCHN    *chn;                // +0x04 one channel per parameter block (0x18 each)
    int         cur;                // +0x08 the sequence playing (-1 none)
    int         prev;               // +0x0C the one before it
    int         blendKind;          // +0x10 0 none, 1 cross-fade, 2 same phase
    float       blendT;             // +0x14
    float       blendLen;           // +0x18 1/7 of the new sequence, 0.3 s at most
    float       prevTime;           // +0x1C the time the previous sequence had reached
    int         groupBase;          // +0x20 GetBaseSeqNmb of cur
    int         group;              // +0x24 the sub-variant group in use (-1)
    int         groupNext;          // +0x28 the one to blend to (SetGroupCur)
    float       groupT;             // +0x2C
    float       groupLen;           // +0x30
};
// animSYSTEM (src/engine/anim_ctrl.cpp): the transition table of a template's sequences, 0x14 bytes, vtable v12
// 0x00606CE4 (feb 0x005DB844). Slot 0 is the deleting destructor (__vecDelDtor: the slots keep the original offsets).
#pragma pack(push, 1)
struct animTRAN {                   // one transition, 4 bytes: tran[n * to + from]
    u8    type;                     // 1 go (by 'next'), 3 wait for the end of 'from'
    u16   next;                     // the sequence to play on the way (0xFFFF none)
    u8    blend;                    // blend into it
};
#pragma pack(pop)
struct animSEQ_PARAM {              // 0x14 per parameter block
    int   seq;                      // +0x00 -1
    int   flags;                    // +0x04
    float blendTime;                // +0x08 0.1 s
    int   unk0C;                    // +0x0C (not set by the ctor) // ?
    int   unk10;                    // +0x10
};
class animSYSTEM {
public:
    animSYSTEM(int nSeq, int nParam);                                         // v12 0x004C9D90 (feb 0x004B9EE0)
    virtual void *__vecDelDtor(unsigned f);                                   // +0x00 v12 0x004C9E80 (feb 0x004B9FD0)
    virtual void SetTranParam(int from, int to, u8 type, u16 next, u8 blend); // +0x04 v12 0x004C9EC0
    virtual void SetTranParamSymm(int a, int b, u8 type);                     // +0x08 v12 0x004C9F00
    virtual void SetTranTypeToAny(int from, int type, u8 blend, int next);    // +0x0C v12 0x004C9F60
    virtual void SetTranParamFromAny(int to, int type, u8 blend, int next);   // +0x10 v12 0x004C9FC0
    virtual void SetTranCycleAnim(int seq);                                   // +0x14 v12 0x004CA030
    virtual void SetTranCycle2Anim(int a, int b);                             // +0x18 v12 0x004CA060
    virtual void SetTranMergeCycles(int a1, int a2, int b1, int b2, u8 blend);// +0x1C v12 0x004CA100
    virtual void DuplicateTranParam(int from, int to);                        // +0x20 v12 0x004CA1B0
    virtual int  GetTranType(int from, int to, int sub);                      // +0x24 v12 0x004CA240
    virtual int  GetNextAnimSeq(int from, int to, int sub);                   // +0x28 v12 0x004CA2A0
    virtual int  GetTranTypeBlend(int from, int to, int sub);                 // +0x2C v12 0x004CA270
    // animSYSTEM_INST::AdvanceAnimation calls slots +0x30 / +0x34 on every system; animSYSTEM's vtable ends at +0x2C
    // and v12 finds unrelated functions there (v12 0x005F12A0 returns its argument, 0x00403200 returns 0): the base
    // class gets those two here, with the same results
    virtual int  GetBaseSeqNmb(int seq);                                      // +0x30 (see above)
    virtual int  GetNSubSeq(int seq);                                         // +0x34 (see above)
    animTRAN      *tran;            // +0x04 n * n
    animSEQ_PARAM *param;           // +0x08
    int            nParam;          // +0x0C
    int            n;               // +0x10 sequences
};
// animSYSTEM_GROUP: sequences in groups of nGroup sub-variants (the player's body per weapon). 0x20 bytes, vtable
// v12 0x00606D1C (feb 0x005DB87C).
class animSYSTEM_GROUP : public animSYSTEM {
public:
    animSYSTEM_GROUP(int nSeq, int nGroup, int nParam);                       // v12 0x004CA2D0 (feb 0x004BA420)
    virtual void *__vecDelDtor(unsigned f);                                   // +0x00 v12 0x004CA340
    virtual void SetTranParam(int from, int to, u8 type, u16 next, u8 blend); // +0x04 v12 0x004CA3F0
    virtual void SetTranParamSymm(int a, int b, u8 type);                     // +0x08 v12 0x004CA510
    virtual void SetTranTypeToAny(int from, int type, u8 blend, int next);    // +0x0C v12 0x004CA5A0
    virtual void SetTranParamFromAny(int to, int type, u8 blend, int next);   // +0x10 v12 0x004CA6D0 (empty)
    virtual void SetTranCycleAnim(int seq);                                   // +0x14 v12 0x004CA6E0
    virtual void SetTranCycle2Anim(int a, int b);                             // +0x18 v12 0x004CA720
    virtual void SetTranMergeCycles(int a1, int a2, int b1, int b2, u8 blend);// +0x1C v12 0x004CA820
    virtual void DuplicateTranParam(int from, int to);                        // +0x20 v12 0x004CA940
    virtual int  GetTranType(int from, int to, int sub);                      // +0x24 v12 0x004CA9A0
    virtual int  GetNextAnimSeq(int from, int to, int sub);                   // +0x28 v12 0x004CAA60
    virtual int  GetTranTypeBlend(int from, int to, int sub);                 // +0x2C v12 0x004CA9D0
    virtual int  GetBaseSeqNmb(int seq);                                      // +0x30 v12 0x004CAA00
    virtual int  GetNSubSeq(int seq);                                         // +0x34 v12 0x004CAA40
    void  DeclareGroupSeq(int seq, int nSubSeq);                              // feb 0x004BA4E0 (inline in 1.2)
    void  SetGroupSuffix(int g, char *s);                                     // feb 0x004BA500 (inline in 1.2)
    char *GetGroupSuffix(int g);                                              // v12 0x004CA3D0 (feb 0x004BA520)
    int   nGroup;                   // +0x14
    u8   *nSub;                     // +0x18 per sequence: its sub-variants (0 = an ungrouped one)
    char *suffix;                   // +0x1C nGroup names of 0x40
};

class animSEQ {                     // stride 0x88 (chunk 0x2E7, _sioReadAnimSeq)
public:
    char  name[0x40];               // +0x00
    float frameStart;               // +0x40
    float frameEnd;                 // +0x44
    float unk48;                    // +0x48 optional chunk 0x2F3 // ?
    float frameRange;               // +0x4C chunk 0x2F4; PrepareDir sets |end - start| when below 0.1
    float lengthMs;                 // +0x50 duration in milliseconds (AdvanceTime: len*0.001)
    u32   unk54;                    // +0x54
    int   actionFrame[10];          // +0x58 action frames (IsActionFrame: actionFrame[bit]); 0x2F7 = all ten
    int   dir;                      // +0x80 0 forward, 1 backward, 2 single frame (PrepareDir; 2 before)
    u32   unk84;                    // +0x84
    void  PrepareDir();             // v12 0x004D10C0 (feb 0x004C4B90)
    float TimeToFrame(float t);     // v12 0x004D0BF0 (feb 0x004C46C0)
    float FrameToTime(float f);     // v12 0x004D0EF0 (feb 0x004C49C0)
    BOOL32 IsActionFrame(int bit, float f0, float f1); // v12 0x004D1130 (feb 0x004C4C00)
};

// animSYSTEM: per-template transition graph (which sequence follows which, blend times).
//   ctor v12 0x004C9D90 (feb 0x004B9EE0); SetTranParam 0x004C9EC0; SetTranCycleAnim 0x004CA030;
//   GetNextAnimSeq 0x004CA2A0. animSYSTEM_GROUP ctor 0x004CA2D0, GetNextAnimSeq 0x004CAA60.
//   animSYSTEM_INST ctor 0x004CAA90 (feb 0x004BABD0), SetGroupCur 0x004CB390;
//   AdvanceAnimation feb 0x004BACA0 (v12 not matched).  vtables animSYSTEM v12 0x00606CE4 (feb 0x005DB844),
//   animSYSTEM_GROUP v12 0x00606D1C (feb 0x005DB87C) // ? v12 vtables from data map only
// animCHN (anim channel, e.g. upper/lower body): AdvanceAnimation v12 0x004EADE0 (feb 0x004DB100).
// The channels of an instance: animINST::link (+0xE8) -> { [0] -> group (+0xC count), [1] -> animCHN[], 0x18 each }.
class animCHN {
public:
    u8    _00[0x0C];
    float weight;                   // +0x0C blend weight (CalcObjModelMatrix)
    u32   actionFrames;             // +0x10 action-frame bits of its sequence (copied by animSYSTEM_INST)
    struct animSEQ_PARAM *param;    // +0x14 its parameter block (seq at +0)
    void   Start();                         // v12 0x004EADC0 an idle channel (state +0 0) starts from 0 (name ours)
    void   AdvanceAnimation(float dt);      // v12 0x004EADE0 (feb 0x004DB100)
    void   BegObjHier(objOBJ *o);           // v12 0x004EB000 (feb 0x004DB320)
    void   EndObjHier(objOBJ *o);           // v12 0x004EB020 (feb 0x004DB340)
    BOOL32 IsActive(objOBJ *o, int what);   // v12 0x004EB040 (feb 0x004DB360) 4 = transform, 0x200 = modifiers
    float  GetFrameCur();                   // v12 0x004EB070 (feb 0x004DB390)
};
// animCREATE_DATA: how to create an instance (spawn transform). size 0x96, packed. Sent inside the MP create/sync
// messages. ctor v12 0x004D1360 (feb 0x004C4E10): rep stosd 0x25 + stosw (=0x96 zeroed), then the fields below.
#pragma pack(push, 1)
class animCREATE_DATA {
public:
    m3dV     scale;            // +0x00 (1,1,1)                v12 0x004D138C/89/86
    union { short seqId; short s16_0C; }; // +0x0C -1        v12 0x004D1379  // ? meaning (re/mpsrc: s16_0C)
    union { int unk0E; float f_0E; };     // +0x0E 0         v12 0x004D137F (re/mpsrc reads it as float)
    m3dMATR  matr;             // +0x12 identity (start matrix) v12 0x004D1376
    char     name[0x40];       // +0x52 instance name (MakeCreateData copies 0x3F chars + NUL at +0x90)
    int      animSeed;         // +0x92 (re/mpsrc: message +0x160 / +0x172 <- animINST +0x134) // ?
    animCREATE_DATA();         // v12 0x004D1360 (feb 0x004C4E10)
};
#pragma pack(pop)

// Bounding volumes (hit boxes) -- Animbvol.obj: abvBVOL (Calc v12 0x004F1470, IsectRay 0x004F1500,
//   DistPoint 0x004F1570, GetOrg 0x004F14A0), updaters abvSPHERE_UPD / abvBOX_UPD / abvPRISM_UPD /
//   abvOBB_UPD (Update v12 0x004F1680 / 0x004F1810 / 0x004F1990 / 0x004F1FF0).

// Template post-processing after load (anim_clean.obj)
BOOL32 __fastcall animCleanConstSpl(animTPL *t);    // v12 0x0059DB30 (feb 0x00581800)
BOOL32 __fastcall animOptimizeSpl(animTPL *t);      // v12 0x0059DE60
BOOL32 __fastcall animCompressAnim(animTPL *t);     // v12 0x0059DF30
// Object-level animation helpers (Obj_anim.obj): objValidateLTM v12 0x004DFC70 (feb 0x004D19F0),
//   objOBJ::GetCenter v12 0x004DFA80 (feb 0x004D1800), objOBJ::SetMatrLT v12 0x004DFB30, objOBJ::FreezeLTM 0x004DF420.

// scnSCENE instance/template pools (engine:Scene.obj, owned by engine_render's scene.h):
//   AllocInst v12 0x004E6510 (feb 0x004DDC10), CreateInst(tpl,cd,id) v12 0x004E6620 (feb 0x004DDD20),
//   CreateInst(obj,cd) v12 0x004E6950, DestroyInst v12 0x004E6CD0, FindInstName v12 0x004E6D90,
//   ForAllInst v12 0x004E6DE0, AllocTpl v12 0x004E6410, FindTplName v12 0x004E63A0.
//   Limits (TWEAK): 1000 instances x 0x14C (ctor v12 0x004E5E10 "push 0x3E8 / push 0x14C"), 300 templates x 0xCC.
