// Will Rock unofficial SDK - effect spawning API (sfx_*.obj, sob_*.obj game code)
// v12 = 1.2, feb = Feb-2003 symbol build. All __fastcall (MSVC "YI"/"SI"): first two int/pointer args in ECX, EDX.
// Assumes from re/sdk/core: entENTITY, scnSCENE, animCREATE_DATA, m3dV, m3dMATR, m3dPLANE, m3dCOLOR, m3dBOX, cdtINFO,
//   txmTEXTURE, objOBJ, aiNPC_WR, psSHEET.
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include <stdint.h>

// ---- Generic: every effect is an entity class, spawned by name --------------------------------------------
// entCreate(scene, className, templateName, createData, initData, 0): the way game code spawns an effect.
// className is the string given to entRegisterClass (see README class table, e.g. "sfx_appear", "sfx_barrel_expl").
// templateName selects a template block in the level/entity database ("" = defaults).
entENTITY* __fastcall entCreate(scnSCENE* scene, char* cls, char* tmpl, animCREATE_DATA* cd, void* init, int flag);
                                                     // v12 0x005082B0 (feb 0x004F5C80)
extern scnSCENE* gsScenePtr;                         // v12 0x0098E81C (feb 0x0094821C)

// ---- Init / Term (called once at game start; register classes, load textures, link sound tables) ------------
int  __fastcall wrsfxInit();      // v12 0x00467830 (feb 0x0045DB60) loads sfx_cloud/sfx_appear_part/sfx_appear_hole, registers
                                  //   "sfx_appear", then calls wrsfxBloodInit, DustInit, WaterInit, PortalInit, ItemInit, PerseusInit,
                                  //   AltarInit, TreasureInit, ... (one *Init per sfx_*.obj)
void __fastcall wrsfxTerm();      // v12 0x004679A0 (feb 0x0045DCD0)
int  __fastcall wrsobInit();      // v12 0x00486560 (feb 0x0047C090) registers sob_* classes (geyser, katapult, niche, spikes, trading altar, tree)
void __fastcall wrsobTerm();      // v12 0x004865D0 (feb 0x0047C100)
int  __fastcall partInit();       // v12 0x004D1730 (feb 0x004C0180) see particles.h

// ---- Blood -----------------------------------------------------------------------------------------------
// type/hitKind are small ints chosen by the caller (player death, NPC hit via MP notify, weapon CDT).
void __fastcall wrsfxStartBlood(int type, int kind, m3dV* pos, m3dV* dir, m3dV* normal, m3dV* extra);
                                  // v12 0x0046AB50 (feb 0x00460D20) callers: plrPLAYER_COMMON::StartDeath, plrPLAYER_MP::ProcessCDT_FIREABLE,
                                  //   aiNPC_WR::ProcessMP_NOTIFY. Spawns 1..6 wrsfxBLOOD_AIR emitters + wrsfxBLOOD_SURF decals.
void __fastcall wrsfxCreateBloodAir(m3dV* pos, m3dV* dir, float size, m3dV* vel, m3dPLANE* plane, float surfSize, float surfTime, m3dCOLOR* col);
                                  // v12 0x0046AA30 (feb 0x00460C10)  1.2: returns at once if (wrAppState & 2) (blood disabled)
void __fastcall wrsfxStartChunkMeat(int, int, m3dV*, m3dV*, aiNPC_WR*);
                                  // v12 0x0046B1B0 (feb 0x00461360) gibs: spawns "sfx_chunk" (wrsfxCHUNK physics pieces)
int  __fastcall wrsfxBloodInit(); // v12 0x0046A950 (feb 0x00460B80) textures sfx_blood_floor_1.., sfx_blood_anim_8x4; reads GameOpt/DisplayBlood
// extern uint32_t wrAppState;       // v12 0x0064FFA8: bit 1 (value 2) = no blood (DisplayBlood=0, German or OEM flag), see build_differences.md   [integrator: declared once in core/wr_core.h / engine_core/core_types.h]

// ---- Decals ----------------------------------------------------------------------------------------------
int  __fastcall wrsfxSCORCHMARK_Add(float radius, float life, txmTEXTURE* tex, m3dV* pos, uint32_t color, int sig, float a, float b);
                                  // v12 0x00467DE0 (feb 0x0045DED0) wrsfxSCORCHMARK::Add (static). Scorch/blast decal; radius x2 when
                                  //   Video/VIDEO_LowScorchQuality is set; skipped if pteMANAGER::FindShaderAround finds one nearby.
int  __fastcall wrsfxFindFloor(m3dV* pos, m3dBOX* box);   // v12 0x00467D10 (feb 0x0045DE00) ray down to floor for decal placement

// ---- One-shot effects with explicit starters ----------------------------------------------------------------
int  __fastcall wrsfxAPPEAR_Start(m3dV* pos, int opp, cdtINFO* cdt);
                                  // v12 0x004686D0 (feb 0x0045E8B0) monster spawn-in: entCreate("sfx_appear") or ("sfx_opp_appear") if opp;
                                  //   skipped if an 'FEPA' decal is within 6.0 (push 0x40C00000 at v12 0x00468715)
int  __fastcall wrsfxALTAR_FIREBALL_StartFireBall(entENTITY* owner, psSHEET* sheet);  // v12 0x00467180 (feb 0x0045D6D0)
void __fastcall aisStatueSFXStartBreakPart(void* statue); // v12 0x00434E80 (feb 0x0042F100) statue break debris "sfx_statue_break_part"
// thiscall starters (object already created by entCreate):
//   wrsfxATLAS_GLOBE_APPEAR::Start()          v12 0x00468EE0 (feb 0x0045F110)
//   wrsfxATLAS_GLOBE_LAND::Start(m3dMATR*)    v12 0x00469480 (feb 0x0045F6B0)
//   wrsfxBIRD_DEATH::Start(m3dMATR*)          v12 0x0046A6C0 (feb 0x004608F0)
//   wrsfxBRIDGE_CRASH::Start(int)             v12 0x0046D160 (feb 0x00463300)
// Weapon impact effects: pwpOPP_SFX::Start(pwpOPP_SFX_START*) feb 0x00417B90 (v12 not located; called from weapon DispatchDamage,
//   see re/sdk/weapons) -> _Start feb 0x00417890.

// ---- "Melt" deformation (Medusa/acid) - chd* helpers in sfx_melt.obj -------------------------------------------
int  __fastcall _chdStartMelt(objOBJ*, m3dBOX*, float, float); // v12 0x0047B730 (feb 0x00471750)
int  __fastcall chdMeltStart(entENTITY*);                      // v12 0x0047B8E0 (feb 0x00471900)
int  __fastcall chdMeltIsComplete(objOBJ*);                    // v12 0x0047BB00 (feb 0x00471B10)
void __fastcall chdMeltStartSink(objOBJ*);                     // v12 0x0047BB80 (feb 0x00471B90)
void __fastcall chdRemoveDeform(objOBJ*);                      // v12 0x0047BC70 (feb 0x00471C00)
