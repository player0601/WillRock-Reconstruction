// ai_world.h -- Will Rock 1.2 unofficial SDK, subsystem "ai": the world-level AI objects
//   aiPLANNER (attack planner / director), spawn domains and spawn areas, arenas, spawn waves.
//
// objs covered: gs_shared:ai_att_planner.obj, ai_plan_filter.obj, ai_area.obj, ai_spawn.obj, aia_arena.obj, aiw_wave.obj
// Assumed from re/sdk/core: msgADDR, entIACTIVE_OBJ, gsDOMAIN, gsDOMAIN_LIST, scnDOMAIN, objOBJ, psSHEET, m3dV, m3dSPL.
// Offsets: "v12 +0xNN (feb +0xNN)". Unverified items carry "// ?".
#pragma once
#include "ai_npc.h"
void __fastcall aiSpawnSetOppCoeff(float k);   // feb 0x00539520 (gs_shared:ai_spawn.obj); wrGO calls it with 0.6 on easy (v12 0x004A6E5E)

#pragma pack(push, 1)

// ------------------------------------------------------------------------------------------------
// Level-object naming conventions (the AI lists scan scene object names with strstr):
//   "&dom_ai..."    -> aiDOMAIN_SPAWN   (aiDOMAIN_LIST_SPAWN::IsAddObj  feb 0x00539560)
//   "&dom_arena..." -> aiaDOMAIN_ARENA  (aiaDOMAIN_LIST_ARENA::IsAddObj feb 0x00538CD0; v12 string 0x00637594)
//   "&eio_aiw..."   -> aiwWAVE groups the "&dom_ai" domains below it (aiDOMAIN_LIST_SPAWN::CreateWaves
//                      v12 0x00559550, feb 0x00541000)
//   "&dom_ai_zevs_bolts" -> Zeus lightning targets (aiZEVS::ParseNameClass)
//   Arena stages are child objects "STAGE_%d" with an "objMarkup" key (aiaDOMAIN_ARENA::Init v12 0x00553900, feb 0x00539100)
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// aiPLANNER -- one global director (aiAttackPlanner, v12 0x009A8C78 / feb 0x0095D938). It keeps the list of
// activated NPCs, the boss, player information for target filtering, and hands out attack/meet/flee points
// so NPCs do not stack on the same spot.  vtable v12 0x00608CF0 (feb 0x005DD5D0), size 988 (feb, classdb)
//   slot +0x00 v12 0x00550F40 (feb 0x00536850) scalar deleting dtor
//   slot +0x04 v12 0x00551030 (feb 0x005368F0) ProcessMsg (FRAME, level messages, NPC death bookkeeping)
//   slot +0x08 v12 0x004802A0 (feb 0x00499C50) msgADDR::PostProcessMsg (base)
//   slot +0x0C v12 0x004802A0 (feb 0x00499C50) msgADDR::PostProcessMsg (base)
//   slot +0x10 v12 0x005511F0 (feb 0x00536A00) ProcessFRAME
// ------------------------------------------------------------------------------------------------
#pragma pack(push, 1)
struct plnCRUMB {                    // 0x18 bytes, a crumb of a player's trail (name ours)
    m3dV pos;
    int  id;                         // +0x0C ascending; 0 none
    int  next;                       // +0x10 the id of the next crumb, -1 none
    u32  flags;                      // +0x14 0x10000 changed, 0x1000 to look at
};
struct plnTRAIL {                    // 0x4C4 bytes (name ours); ctor v12 0x00552080
    int      owner;                  // +0x00 the player's MP id, -1 free
    plnCRUMB c[50];                  // +0x04
    float    timer;                  // +0x4B4 until the next crumb
    int      nextId;                 // +0x4B8
    int      firstId;                // +0x4BC
    int      n;                      // +0x4C0
};
struct plnSAVED {                    // what a player's restore carries
    int      owner;
    plnCRUMB c[49];
    int      nextId;
    int      n;
};
class aiaDOMAIN_LIST_ARENA {                           // (only what is called so far; not rebuilt)
public:
    int GetPlrStageNmb();                                       // v12 0x005537C0 (feb 0x00538FC0) the stage the player is on
    int GetNPCStageNmb(class aiNPC *n);                         // v12 0x00553840 (feb 0x00539040) the stage of n
};

#pragma pack(pop)

class aiPLANNER : public msgADDR {   // size 0x501C, vtable v12 0x00608CF0; src/gs_shared/ai_att_planner.cpp
public:
    // fields: +0x28 nRegistered, +0x2C aiNPC *list[200], +0x34C mode (bit 2: predictive aiming), +0x350 boss,
    //   +0x354 nPlayers, +0x358 players[16], +0x398 nFiltered, +0x39C filtered[16], +0x3DC plnTRAIL trail[16]
    u8 _body[0x501C - sizeof(msgADDR)];
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x005510A0
    void ProcessFRAME();                                     // v12 0x005511B0 (feb 0x00536A00)
    void *GetPlrCrumbs(class plgsACTION *a);                 // v12 0x00558BE0 (name ours)
    void SaveTarget(void *saved, class plgsACTION *a);       // v12 0x00558C40 (name ours)
    void RestoreTarget(void *saved, class plgsACTION *a);    // v12 0x00558CA0 (name ours) a restored player's trail
    void NotifyTarget(class plgsACTION *a);                  // v12 0x00558D00 (name ours) a target's frame
    void RegisterNPC(aiNPC *n);                              // v12 0x00550F70 (feb 0x00536880) called by aiNPC::Activate
    void UnRegisterNPC(aiNPC *n);                            // v12 0x00550FE0 (feb 0x005368A0)
    BOOL GetBossHealth(float *cur, float *max, int *isDead); // v12 0x00551DB0 (feb 0x00537680) 0 = no boss
    void SetBoss(aiNPC *n);                                  // v12 0x00551E30 (feb 0x005376F0) boss gets AIF2_BOSS
    scnDOMAIN *GetBanCrossDomain(aiNPC *n);                  // v12 0x00551220 (feb 0x00536A60)
    void GetFleeGoalPoint(aiNPC*, float dist, m3dV *out);    // v12 0x00551240 (feb 0x00536A80)
    BOOL GetEnemyMeetPoint(aiNPC*, float dist, m3dV *out);   // v12 0x00551390 (feb 0x00536BD0) tiger pounce etc.
    void GetAttackGoalPoint(aiNPC*, m3dV *out);              // v12 0x005517E0 (feb 0x00536FE0)
    void GetCrumbGoalPoint(aiNPC*, m3dV *out);               // v12 0x00559270 (feb 0x00537350) follow the trail
    void DesyncAnimations();                                 // v12 0x00551A10 (feb 0x00537420)
    void ClipPointByNavArea(aiNPC*, m3dV *p);                // v12 0x00551E40 (feb 0x00537700)
    void GetNavAreaCenter(aiNPC*, m3dV *out);                // v12 0x00551FB0 (feb 0x00537870)
    void UsePredictiveAiming(int on);                        // v12 0x00552050 (feb 0x00537910)
    // player filtering (ai_plan_filter.obj): which player each NPC should pick in multiplayer/co-op
    void GatherPlrInformation();                             // v12 0x0055AD90 (feb 0x00540CE0)
    void StartFilteringPlayers();                            // v12 0x0055AEC0 (feb 0x00540DA0)
    void RemoveHiddenFromFiltered(aiNPC *n);                 // v12 0x0055AF10 (name ours)
    void FilterPlayersFor(aiNPC *n);                         // v12 0x0055AFB0 (name ours)
    void SortFilteredPlayersByDist(aiNPC *n);                // v12 0x0055B040 (feb 0x00540DF0)
    BOOL HaveAChoiceFromFiltered(entENTITY **only);          // v12 0x0055B1D0 (feb 0x00540F80)
    class plgsACTION *GetFilteredByIdx(int i);               // v12 0x0055B200 (feb 0x00540FB0)
};
int __fastcall  aiPlannerInit();   // feb 0x00536760 (v12 ?)
void __fastcall aiPlannerTerm();   // v12 0x00550F20 (feb 0x00536830)

// ------------------------------------------------------------------------------------------------
// aiAREA -- polygon area (scnDOMAIN) with a random point list; used for spawn areas and the bird "area".
//   size 524 (feb). vtable feb 0x005D1FD4 (in ai_bird.obj). AI_SPAWN keys: nPoint, isShowPoints
// ------------------------------------------------------------------------------------------------
struct aiAREA_POINT {                    // size 0x1C
    m3dV  pos;                           // +0x00
    m3dV  dir;                           // +0x0C (the explicit merged list)
    float weight;                        // +0x18
};
struct aiAREA_GET_POINT_PARAM {
    m3dV       pos;                      // +0x00 the place to go by
    void      *domain;                   // +0x0C a formation's domain
    scnDOMAIN *cross;                    // +0x10 areaFlags 0x10: the border
    m3dV       crossFrom;                // +0x14 areaFlags 0x10: from here
};
class aiAREA : public scnDOMAIN {        // size 0x190; vtable v12 0x005FCEF0
public:
    aiAREA_POINT *points;                // +0x3C
    int   nPoint;                        // +0x40 [AI_SPAWN] nPoint (50)
    int   nBound;                        // +0x44
    m2dV  bound0[20];                    // +0x48  the outline's edges
    m2dV  bound1[20];                    // +0xE8
    u32   areaFlags;                     // +0x188 distribution 1 even, 2 distant (the ctor's), 4 formation, 8 by sight,
                                         //   0x10 behind a border; 0x20 show the points; 0x10000 outline as a polygon,
                                         //   0x20000 the outline made
    aiAREA();                                  // v12 0x00548CC0
    BOOL32 Init();                             // [ 1] v12 0x00552100 (feb 0x00537940)
    void   Term();                             // [ 2] v12 0x00552150 (feb 0x00537990)
    BOOL32 ISectLSegBoundaryXZ(m3dV *a, m3dV *b, m3dV *out); // [ 6] v12 0x00553090 (feb 0x00538890)
    virtual BOOL32 IsValidPoint(m3dV *p, float r);   // [12] v12 0x00552A10
    virtual void   ConstrainPoint(m3dV *p, float r); // [13] v12 0x00552A60
    void   CalcWeights(aiAREA_GET_POINT_PARAM *q);   // v12 0x00552180 (feb 0x005379C0)
    void   GetRandomPos(m3dV *out, aiAREA_GET_POINT_PARAM *q); // v12 0x005528D0 (feb 0x00538120)
    void   GetRandomPos(m3dV *out, m3dV *from);      // v12 0x00552960 (feb 0x005381B0)
    void   MakePointList();                          // v12 0x00552AB0 (feb 0x00538260)
    void   FillBoundaryPoly();                       // v12 0x00552E40 (feb 0x00538640)
};

// ------------------------------------------------------------------------------------------------
// aiDOMAIN_SPAWN -- a "&dom_ai..." object: spawns enemies of one class while a player is inside. See ai_spawn.cpp.
//   size v12 0x1F0 (feb 0x1E8). vtables v12 0x00608B28 (entIACTIVE_OBJ part) / 0x00608AF8 (scnDOMAIN part)
// ------------------------------------------------------------------------------------------------
class aiDOMAIN_SPAWN : public gsDOMAIN {
public:
    float timeOppSpawn;            // v12 +0x130 (feb +0x12C) 0.5
    char  oppName[0x40];           // v12 +0x134 (feb +0x130)
    char  oppClass[0x40];          // v12 +0x174 (feb +0x170)
    float timer;                   // v12 +0x1B4 (feb +0x1B0) since the last spawn
    float timeAfterKill;           // v12 +0x1B8 (feb +0x1B4) 0.5 (0.2 for skeletons)
    float killTimer;               // v12 +0x1BC (feb +0x1B8) since the last kill
    float doneTimer;               // v12 +0x1C0 (1.2 only) nothing left to spawn: the 10 s check
    int   nSpawn;                  // v12 +0x1C4 (feb +0x1BC) -1
    int   nSpawned;                // v12 +0x1C8 (feb +0x1C0)
    int   nMaxSimult;              // v12 +0x1CC (feb +0x1C4)
    int   nMaxSimultSame;          // v12 +0x1D0 (feb +0x1C8)
    int   nKillComplete;           // v12 +0x1D4 (feb +0x1CC)
    int   nKilled;                 // v12 +0x1D8 (feb +0x1D0)
    int   nSpawnFirst;             // v12 +0x1DC (feb +0x1D4) 1
    int   _1e0, _1e4;              // v12 +0x1E0 never used here // ?
    int   nMerged;                 // v12 +0x1E8 (feb +0x1E0)
    aiAREA_POINT *merged;          // v12 +0x1EC (feb +0x1E4)
    aiDOMAIN_SPAWN();
    ~aiDOMAIN_SPAWN();                                         // v12 0x0054B000 (feb 0x0053B290)
    BOOL32 Init();                                             // v12 0x00548F90 (feb 0x005396C0)
    void   Update(m3dV *pos);                                  // v12 0x005493E0 (feb 0x00539B10)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00549C00 (feb 0x0053A1A0)
    void   ProcessMP_GET_SYNC_DATA(void *d);                   // v12 0x0054AC60 (feb 0x0053AF80)
    void   ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d);     // v12 0x0054AD00 (feb 0x0053B140)
    virtual void ProcessMP_NOTIFY(gsEVENT *e);                 // [19] v12 0x0054AC20 (feb 0x0053AF40)
    BOOL32 IsReadySpawn();                                     // v12 0x00549520 (feb 0x00539B90)
    BOOL32 IsSpawning();                                       // v12 0x00549640 (name ours)
    void   StopSpawning();                                     // v12 0x00549680 (name ours)
    void   Spawn();                                            // v12 0x005496D0 (feb 0x00539CB0)
    void   SpawnSingle();                                      // v12 0x00549700 (feb 0x00539CE0)
    int    GetSpawnPosDir(m3dV *pos, m3dV *dir);               // v12 0x00549860 (feb 0x00539E40) also aiNPC::ProcessINIT
    void   FillMergedPointsList();                             // v12 0x0054A3A0 (feb 0x0053A6A0)
    void   FillExplicitMergedPointsList();                     // v12 0x0054A4D0 (feb 0x0053A7D0)
    void   GetFromMergedListSpawnPosDir(m3dV *pos, m3dV *dir); // v12 0x0054A6F0 (feb 0x0053A9F0)
    void   CalcWeightDistant();                                // v12 0x0054A8A0 (feb 0x0053AC60)
};

// aiSPAWN_AREA -- a spawn domain's sub-domain: an active object and an aiAREA. size v12 0x228 (feb 0x224)
//   vtables v12 0x00608AB8 / 0x00608A80. Its points fit the class that spawns there.
class aiSPAWN_AREA : public entIACTIVE_OBJ, public aiAREA {
public:
    aiDOMAIN_SPAWN *domain;        // v12 +0x224 (feb +0x220)
    aiSPAWN_AREA() : domain(0) {}
    BOOL32 Init();                                 // v12 0x00549FB0 (feb 0x0053A550)
    BOOL32 IsValidPoint(m3dV *p, float r);         // v12 0x0054A0E0 (feb 0x0053A590)
    void   ConstrainPoint(m3dV *p, float r);       // v12 0x0054A260
};

// aiDOMAIN_LIST_SPAWN (global aiSpawnDomList v12 0x009A89E8, feb 0x0095D96C)  size v12 0x3D0; vtable v12 0x00608A08 (feb 0x005DD6F8)
//   Update returns at once on an MP client: clients never spawn. Debug key DBG_5 toggles every domain (build_differences.md).
class aiDOMAIN_LIST_SPAWN : public gsDOMAIN_LIST {
public:
    int   _3c8;                    // v12 +0x3C8 0   // ?
    int   _3cc;                    // v12 +0x3CC -1  // ?
    int   IsAddObj(objOBJ *o);                     // v12 0x00548E30 (feb 0x00539560)
    gsDOMAIN *MakeDomain();                        // v12 0x0054B060 (feb 0x0053B2F0)
    scnDOMAIN *MakeSubDomain(gsDOMAIN *d);         // v12 0x00548E50 (feb 0x00539580)
    void  DeleteSubDomain(scnDOMAIN *s);           // v12 0x00548F00 (feb 0x00539630)
    void  ProcessLEVEL_LOADED();                   // v12 0x00548F30 (feb 0x00539660)
    int   ProcessINIT_LEVEL();                     // v12 0x00548F50 (feb 0x00539680)
    void  Update();                                // v12 0x00548F60 (feb 0x00539690)
    int   CreateWaves(objOBJ *o);                  // v12 0x00559550 (feb 0x00541000)
};
int   __fastcall aiSpawnInit();                    // v12 0x00548D50 (feb 0x00539490)
void  __fastcall aiSpawnTerm();                    // v12 0x00548DC0 (feb 0x00539500)
float __fastcall aiSpawnGetOppCoeff();             // v12 0x00548E20

// ------------------------------------------------------------------------------------------------
// aiwWAVE -- "&eio_aiw..." object: picks which child spawn domain spawns next, weighted by AI_SPAWN "ratio"
//   (float, default 10.0) of each domain; up to 10 domains (array +0x9C, count +0xC4), m3dSPL_LINEAR1D at +0xC8.
//   size 0xCC (feb). vtable v12 0x006090A0 (feb 0x005DDB08)
//   Init v12 0x00559610 (feb 0x005410C0); ProcessFRAME v12 0x00559830 (feb 0x005412E0); ProcessMsg v12 0x00559980 (feb 0x00541430)
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// aiaDOMAIN_ARENA -- "&dom_arena..." object (coliseum fights): stages "STAGE_%d" with markup objects.
//   size 19996 (feb). vtables feb 0x005DD674 / 0x005DD6A8; v12 0x00608D90 / 0x00608DC0 (dtor 0x00553C00 [exact]).
//   Init v12 0x00553900 (feb 0x00539100) [exact]. aiaDOMAIN_LIST_ARENA (global aiaArenaDomList v12 0x009A8C90,
//   feb 0x0095D94C) vtable v12 0x00608D18 (feb 0x005DD600):
//     GetPlrStageNmb v12 0x005537C0 (feb 0x00538FC0), GetNPCStageNmb v12 0x00553840 (feb 0x00539040),
//     AssignArena feb 0x00538D00, IsPlrInArena feb 0x00538FB0, Enter/Leave feb 0x005390D0/0x005390F0.
//   Used by aiTIGER ("AI_TIGER isArena") and aiMINO_ATRIUM to stay inside the arena stage.
// ------------------------------------------------------------------------------------------------

#pragma pack(pop)
