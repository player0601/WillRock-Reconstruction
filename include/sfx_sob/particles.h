// Will Rock unofficial SDK - particle engine (engine:particle.obj, Partrend.obj, partobj.obj) and decals (pte_spot*.obj)
// The base every wrsfx*/pwp*/sob* particle effect is built on.
// v12 = 1.2 (re/bin/v12.exe), feb = Feb-2003 symbol build (db/WillRock.map).
// Assumes from re/sdk/core: m3dV, m3dMATR, m3dPLANE, m3dTRI, m3dBOX, m3dCOLOR, m3dSPL_COLOR, m3dSPL_LINEAR1D,
//   camCAMERA, txmTEXTURE, objOBJ, animINST, scnSCENE, entENTITY, msgADDR.
// Verification: every v12 offset below was seen in a v12 instruction (setters at 004D1C80..004D2810, ConstructorInit 004D1810)
// unless marked "// ?".
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include "../engine_render/particle.h" // partEMITTER_PHYS (authoritative)
#include <stdint.h>
#pragma pack(push,1)

// ---------------------------------------------------------------------------------------------
// partPART: one particle, 0x3C bytes in both builds (ConstructorInit is passed 0x3C: v12 push 0x3C at 004D17CF).
// Layout not reconstructed: treat as opaque; emitters receive arrays of partPART* in PartCreate/PartUpdate.
struct partPART;   // size 0x3C

// Flags in partEMITTER_PHYS::flags (+0x04) (meaning from partUpdate feb 004C51E0 / v12 004D41C0)
enum partEMITTER_FLAGS4 {
    PART_F4_ANIM_EMISSION  = 0x00000080, // SetAnimEmission active
    PART_F4_KILL_WHEN_DONE = 0x00000200, // auto-delete when no live particles and not emitting
    PART_F4_ATTACHED_CHILD = 0x00000800, // updated by its parent (AttachEmitter); partUpdate skips it
    PART_F4_OWNER_HIDDEN   = 0x08000000, // owner anim instance hidden: no update unless PART_F8_FORCE_UPDATE
};
// Flags in partEMITTER_PHYS::state (+0x08)
enum partEMITTER_FLAGS8 {
    PART_F8_EMITTING       = 0x00000001, // StartEmission sets, StopEmission clears
    PART_F8_RESTART        = 0x00000004,
    PART_F8_NO_RESTART     = 0x00000008,
    PART_F8_DELETE         = 0x00000020, // partUpdate deletes the emitter (virtual dtor slot 0) this frame
    PART_F8_FORCE_UPDATE   = 0x00000040, // one forced update (cleared after each partUpdate)
    PART_F8_POS_CHANGED    = 0x00000200, // SetPosDirSpeed moved the emitter
};

// ---------------------------------------------------------------------------------------------
// partEMITTER_PHYS: CPU particle emitter. NOT an entity; owned by an effect entity (wrsfx*) or a weapon/NPC.
// size v12 0x2C4 (feb 0x2C0). 1.2 inserted one dword at v12 +0x34: feb offsets >= 0x34 are +4 in v12.
// Every live emitter registers itself in _partEmitList (600 slots) in ConstructorInit and is driven by partUpdate.
#if 0 // [integrator] merged into engine_render/particle.h (class partEMITTER_PHYS, with its virtuals); kept as documentation
struct partEMITTER_PHYS {
    void**            vtbl;          // +0x00 v12 0x00606ED0 (feb 0x005DB950)
    uint32_t          flags;         // v12 +0x04 (feb +0x04) partEMITTER_FLAGS4
    uint32_t          state;         // v12 +0x08 (feb +0x08) partEMITTER_FLAGS8, ctor sets 0x2C0
    float             emitRate;      // v12 +0x0C (feb +0x0C) particles/sec (ctor arg 1)
    float             intensity;     // v12 +0x10 (feb +0x10) SetIntensity
    m3dSPL_LINEAR1D*  intensitySpl;  // v12 +0x14 (feb +0x14) SetIntensity, owned (released with vfunc+4(1))
    float             f_18;          // v12 +0x18 (feb +0x18) ctor = arg 2
    float             lifeMin;       // v12 +0x1C (feb +0x1C) SetTimeLifePart arg1 (ctor = arg 2)
    float             lifeRand;      // v12 +0x20 (feb +0x20) SetTimeLifePart arg2
    float             sizeStart;     // v12 +0x24 (feb +0x24) SetSize arg1, default 5.0
    float             sizeEnd;       // v12 +0x28 (feb +0x28) SetSize arg2
    float             sizeRand;      // v12 +0x2C (feb +0x2C) SetSize arg4, default 1.0
    m3dSPL_LINEAR1D*  sizeSpl;       // v12 +0x30 (feb +0x30) SetSize arg3
    float             sizeMax;       // v12 +0x34 (feb: none) NEW in 1.2: sizeStart+sizeEnd, cached by SetSize (004D1DFD), default 5.0
    uint32_t          color;         // v12 +0x38 (feb +0x34) SetColor ARGB
    m3dSPL_COLOR*     colorSpl;      // v12 +0x3C (feb +0x38)
    m3dSPL_LINEAR1D*  alphaSpl;      // v12 +0x40 (feb +0x3C)
    uint8_t           _44[0x14];     // v12 +0x44 (feb +0x40); feb dword index 0x12 (+0x48) = live particle count used by partUpdate // ?
    float             f_58;          // v12 +0x58 (feb +0x54) ctor default 0.5
    float             texScale[4];   // v12 +0x5C (feb +0x58) SetTextureScale
    txmTEXTURE*       texture;       // v12 +0x6C (feb +0x68) SetTexture (v12 004D1E40)
    float             texRot[4];     // v12 +0x70 (feb +0x6C) SetTextureRotAnim
    int32_t           frameFirst;    // v12 +0x80 (feb +0x7C) SetTextureFrameAnim
    int32_t           frameCount;    // v12 +0x84 (feb +0x80)
    float             frameFps;      // v12 +0x88 (feb +0x84)
    float             frameRand;     // v12 +0x8C (feb +0x88)
    float             animEmitTime;  // v12 +0x90 (feb +0x8C) SetAnimEmission
    float             animEmitRate;  // v12 +0x94 (feb +0x90)
    float             emitAccum;     // v12 +0x98 (feb +0x94) zeroed by StartEmission
    float             degNear[2];    // v12 +0x9C (feb +0x98) SetDegNearParams: near-camera fade
    float             degFar[4];     // v12 +0xA4 (feb +0xA0) SetDegFarParams: distance LOD fade
    uint8_t           _B4[0x50];     // v12 +0xB4
    m3dV              pos;           // v12 +0x104 (feb +0x100) SetPosDirSpeed
    m3dV              dir;           // v12 +0x110 (feb +0x10C) // ?
    m3dV              speed;         // v12 +0x11C (feb +0x118) // ?
    uint8_t           _128[0x24];
    m3dMATR           cs;            // v12 +0x14C (feb +0x148) SetCS (emission space, 0x40 bytes)
    m3dMATR           scaleCS;       // v12 +0x18C (feb +0x188) SetScaleCS
    float             scaleCSVal[3]; // v12 +0x1CC (feb +0x1C8) SetScaleCSVal
    uint8_t           _1D8[0x14];
    int32_t           cdtPlaneCount; // v12 +0x1EC (feb +0x1E8) SetCDTPlane (collision planes for particles)
    uint8_t           cdtPlanes[4][0x18]; // v12 +0x1F0 (feb +0x1EC) m3dPLANE[4]
    float             cdtBounce;     // v12 +0x250 (feb +0x24C) SetCDTParam arg1
    float             cdtFriction;   // v12 +0x254 (feb +0x250)
    float             cdtArg3;       // v12 +0x258 (feb +0x254)
    int32_t           cdtMode;       // v12 +0x25C (feb +0x258)
    partEMITTER_PHYS* parent;        // v12 +0x260 (feb +0x25C) AttachEmitter (dword index 0x97 in feb partUpdate)
    int32_t           parentArg;     // v12 +0x264 (feb +0x260)
    uint8_t           _268[0x5C];    // ..0x2C4
};
#endif

// vtable (12 slots, identical order in both builds)
// slot +0x00 v12 0x00402FF0 (feb 0x0047F4C0) scalar deleting dtor (shared COMDAT)
// slot +0x04 v12 0x004D1D10 (feb 0x004C0760) SetColor(ulong argb, m3dSPL_COLOR*, m3dSPL_LINEAR1D*)
// slot +0x08 v12 0x004D1D60 (feb 0x004C0780) GetColor
// slot +0x0C v12 0x004D3730 (feb 0x004C2120) Update(camCAMERA*, float dt) -> int   (FLY variant: v12 0x004D40E0 feb 0x004C2AE0)
// slot +0x10 v12 0x004038E0 (feb 0x004035E0) Emit(int n, float t)
// slot +0x14 v12 0x00403860 (feb 0x0057BD10) empty stub: PartDeath or PartPostCDT // ?
// slot +0x18 v12 0x004D5980 (feb 0x004C6980) Register(camCAMERA*, partEMITTER_VIS*)  - hands visible particles to the renderer
// slot +0x1C v12 0x004D2860 (feb 0x004C1250) PartCreate(int n, partPART**, float, int, int)  - OVERRIDE to shape spawning
// slot +0x20 v12 0x004D35A0 (feb 0x004C1F90) PartUpdate(int n, partPART**, float dt)         - OVERRIDE to animate
// slot +0x24 v12 0x004D2290 (feb 0x004C0C80) PartUpdateVisible(int, partPART**)
// slot +0x28 v12 0x00403860 (feb 0x0057BD10) empty stub: PartPostCDT or PartDeath // ?
// slot +0x2C v12 0x004D3CA0 (feb 0x004C2690) PartApplyCDT(partPART*, m3dV*, float) -> int    - particle hit geometry (blood decals hook here)

// Methods                                                   v12          feb
// partEMITTER_PHYS(float rate, float life)                  0x004D1790   0x004C01E0  maxParts = ftol(rate*life*1.5+3), min 3
// partEMITTER_PHYS(int maxParts, float rate, float life)    0x004D17E0   0x004C0230
// ConstructorInit(int partSize=0x3C,int max,float,float)    0x004D1810   0x004C0260  allocs particle pool, registers in _partEmitList
// ~partEMITTER_PHYS                                         0x004D1B60   0x004C05B0  unregisters
// StartEmission / StopEmission                              0x004D1C80 / 0x004D1CB0   0x004C06D0 / 0x004C0700
// SetAnimEmission(float,float)                              0x004D1CC0   0x004C0710
// SetIntensity(float, spline*)                              0x004D1D90   0x004C07B0
// SetTimeLifePart(float,float)                              0x004D1DC0   0x004C07E0
// SetSize(float start,float end,spline*,float rand)         0x004D1DE0   0x004C0800  (1.2 also stores start+end at +0x34)
// SetTexture(txmTEXTURE*)                                   0x004D1E40   0x004C0830
// SetTextureScale / FrameAnim / RotAnim                     0x004D1E50 / 0x004D1E70 / 0x004D1ED0   0x004C0840 / 0x004C0860 / 0x004C0890
// SetDegNearParams / SetDegFarParams                        0x004D1EF0 / 0x004D27E0   0x004C08B0 / 0x004C11D0
// SetPosDirSpeed(m3dV* pos, m3dV* dir, m3dV* speed)         0x004D23D0   0x004C0DC0  any arg may be NULL
// GetPosDirSpeed                                            0x004D2530   0x004C0F20
// SetCS(m3dMATR*)                                           ?            0x004C0F90  (inlined/unmatched in v12)
// SetAreaEmission(int,int,m3dTRI*,int)                      0x004D25C0   0x004C0FB0
// SetCDTPlane(int count,int idx,m3dPLANE*)                  0x004D2750   0x004C1140
// SetCDTParam(float,float,float,int)                        0x004D2790   0x004C1180
// AttachEmitter(partEMITTER_PHYS* child,int,int)            ?            0x004C11B0
// SetScaleCSVal(float,float,float)                          0x004D2810   0x004C1200
// KillParticle(partPART*) / KillAll                         0x004D2270 / ?   0x004C0C60 / 0x004C0C70
// UpdateUpdate/UpdateErase/UpdateGetNmbEmit/UpdateEmit      0x004D1F10/0x004D2070/0x004D2120/0x004D21E0   0x004C0900/0x004C0A60/0x004C0B10/0x004C0BD0
// RegisterPart (renderer)                                   0x004D5450   0x004C6470
// CalcBBox                                                  0x004D5C00   0x004C6BF0

// partEMITTER_PHYS_FLY : partEMITTER_PHYS   size v12 0x2D4 (feb 0x2D0) ?  extra 4 dwords at feb +0x2C0..+0x2CC; own Update (slot +0x0C)

// Free functions                                            v12          feb
int  __fastcall partInit();                               // 0x004D1730   0x004C0180  loads texture "particle", clears _partEmitList
void __fastcall partTerm();                               // ?            0x004C01C0  deletes every emitter
void __fastcall partUpdate(camCAMERA* cam, float dt);     // 0x004D41C0   0x004C51E0  PER FRAME: Update() every registered emitter, delete flagged ones
void __fastcall partDestroyEmitters();                    // 0x004D42A0   0x004C52C0  level change
int  __fastcall partGetNEmit();                           // 0x004D42D0   0x004C52F0  count of live emitters (called when the list is full)
void __fastcall partRender(camCAMERA*, int, partEMITTER_VIS*); // 0x004D42F0 0x004C5310  sorts (qsort by Z) and builds quads via rendCFG
void __fastcall partAttachInst(partEMITTER_PHYS*, animINST*);  // 0x004D9B70 0x004CABB0  follow an animated instance
void __fastcall partDetachInst(animINST*);                // 0x004D9B80   0x004CABC0
int  __fastcall partIsOwnerInstHidden(partEMITTER_PHYS*); // 0x004D9BB0   0x004CABF0
int  __fastcall partIsOwnerInstVisible(partEMITTER_PHYS*);// 0x004D9BD0   0x004CAC10
int  __fastcall partSetAreaEmissionObj(partEMITTER_PHYS*, objOBJ*, int, m3dV*);           // 0x004D9BF0 0x004CAC30 emit from a mesh surface
int  __fastcall partSetAreaEmissionObjList(partEMITTER_PHYS*, int, objOBJ**, int, m3dV*); // 0x004D9C20 0x004CAC60

extern partEMITTER_PHYS* _partEmitList[600]; // v12 0x008FB1B0 (feb 0x008B6F90)  hard limit 600 (0x258) emitters
extern partPART*         _partTmpList[];     // v12 0x008FBB10 ? (feb 0x008B78F0) directly follows _partEmitList
extern txmTEXTURE*       partDefaultTex;     // v12 0x008FDA50 (feb 0x008B9830) texture "particle"

// ---------------------------------------------------------------------------------------------
// Ready-made engine emitters (engine:part_eff.obj) all : partEMITTER_PHYS, size 0x2C0 feb
//   peffSMOKE, peffSMOKE_ANIM, peffFLAME, peffGAS (+objOBJ* +0x2C0, int +0x2C4), peffSPARKLE : partEMITTER_PHYS_FLY
// Level-placeable particle entities peffPART_EFF (entENTITY, feb size 0x173) - created by name from level data:
//   CreateSteam/CreateSmoke/CreateGas/CreateFlame/CreateFollow feb 0x00545A00..0x00545D00 (v12 not matched)
//   peffPART_EFF_SPARK::Create feb 0x005462F0;  peffInit v12 0x0055CFD0 (feb 0x00545900) registers them.

// ---------------------------------------------------------------------------------------------
// Decals ("spots") - projected textures on level geometry: blood pools, scorch marks, shadows of effects
// pteSPOT : pteSHADER (pte_spot.obj).  Created per decal; pteMANAGER owns them.
// pteSPOT::ctor(int)             v12 0x004D6120 (feb 0x004C7140)
// pteSPOT::Init                  v12 0x004D62C0 (feb 0x004C72E0)
// pteSPOT::SetRadius(float)      v12 0x004D64A0 (feb 0x004C74C0)
// pteSPOT::SetOrg(m3dV*)         v12 0x004D64B0 (feb 0x004C74D0) virtual
// pteSPOT::SetColor(m3dCOLOR,m3dCOLOR) v12 0x004D64D0 (feb 0x004C74F0)
// pteSPOT::SetTex(txmTEXTURE*)   v12 0x004D6520 (feb 0x004C7540)
// pteSPOT::AddObjExclude(objOBJ*) v12 0x004D6610 (feb 0x004C7630)
// pteSPOT::PrepareFrame          v12 0x004D68C0 (feb 0x004C78F0)
// pteSPOT::ShadeFace             v12 0x004D69C0 (feb 0x004C79F0)  per-face projection (renderer path)
// pteSPOT_ANIM : pteSPOT  animated decal (radius/pos/colour controllers)
//   Create v12 0x004D7BD0 (feb 0x004C8C10), Update v12 0x004D7C20 (feb 0x004C8C60), Init v12 0x004D7DC0,
//   SetCtrlRad 0x004D80A0, SetCtrlPos 0x004D80D0, SetCtrlColorLM 0x004D8120, SetCtrlColorBase 0x004D8190, SetCurTime 0x004D8200
// pteMANAGER::FindShaderAround(int type,int* sig,m3dV* pos,float radius) v12 0x004D9360 (feb 0x004CA3A0)
//   used by every decal spawner to avoid stacking decals: returns nonzero if one of that sig is already within radius.
// extern pteMANAGER *pteManager; // v12 0x00974E10 -- declared in core/wr_core.h

#pragma pack(pop)
