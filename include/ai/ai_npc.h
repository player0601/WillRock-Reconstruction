// ai_npc.h -- Will Rock 1.2 unofficial SDK, subsystem "ai": the NPC framework (aiNPC / aiNPC_WR / aiNPC_WR_FLY)
//
// objs covered: gs_shared:ai_ctrl*.obj (ai_ctrl, _LOD, _MP, _anim, _die, _geo, _nav_phys, _nav_plan, _script),
//               gs_shared:ai_enemy.obj, ai_npc_wr.obj, ai_npc_wr_fly.obj
// Assumed from re/sdk/core (engine_core): msgSYSTEM, msgADDR, msgDATA, apNAME, objOBJ, animINST, entIACTIVE_OBJ, entENTITY
//   (entity.h), m3dV, m3dMATR, cdtINFO, scnDOMAIN, psSHEET, dmgDAMAGE (game_shared/damage.h), physMATH (physics.h).
//
// Conventions: "v12 +0xNN (feb +0xNN)". All AI classes are #pragma pack(1) (fields at odd offsets, like entENTITY).
// Every offset tagged [v12: ...] was read from a v12 instruction; others follow the proven feb->v12 shift rule below
// and are tagged "// ?" when the *meaning* is a guess.
//
// ------------------------------------------------------------------------------------------------
// FEB -> 1.2 FIELD SHIFT RULE for aiNPC (proven by aligning the two constructors:
//   feb aiNPC::aiNPC 0x00534210  vs  v12 0x00547A60, and every enemy's ProcessINIT, see FEB_VS_12.md)
//   entENTITY grew 0x14F -> 0x16B (+0x1C, see entity.h). aiNPC then inserts three more dwords:
//     feb 0x14F..0x19A  -> +0x1C
//     (1.2 inserts float @ v12 0x1B7, ctor 0.3f)
//     feb 0x19B..0x3C2  -> +0x20
//     (1.2 inserts 4 bytes @ v12 0x3E3)
//     feb 0x3C3..0x4CA  -> +0x24
//     (1.2 inserts int @ v12 0x4EF, ctor 0)
//     feb 0x4CB..0x51A  -> +0x28
//   => sizeof(aiNPC): feb 0x51B, v12 0x543.  aiNPC_WR / aiNPC_WR_FLY own fields: +0x28.
//   Enemy classes: own fields start at +0x28; tiger/lion, minotaur, satyr add one more dword (+0x2C after it),
//   medusa two (+0x30). Allocation sizes per class: see enemies.h.
// ------------------------------------------------------------------------------------------------
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include "../engine_core/entity.h"

class sncCHANNEL; class aiNPC; class aiPLANNER; class aiDOMAIN_SPAWN; class plrPLAYER; class physMATH; struct dmgDAMAGE; struct gsEVENT;

#pragma pack(push, 1)

// ------------------------------------------------------------------------------------------------
// Class signatures (entIACTIVE_OBJ +0x14 'type') used by AI code to tell NPCs from players
// ------------------------------------------------------------------------------------------------
enum aiSIGNATURE {
    AI_SIG_NPC    = 0x244E5043,   // '$NPC' every aiNPC (entRegisterClass sig of ai_* classes). ShouldChangeEnemy tests it
    AI_SIG_PLAYER = 0x504C5952,   // 'PLYR'
    AI_SIG_STATUE = 0x24445354,   // '$DST' ai_perseus / ai_sphinx register with this (class type 0x28)
};

// ------------------------------------------------------------------------------------------------
// aiNPC::aiFlags  (v12 +0x46F, feb +0x44B)   -- names from usage in feb, all bit tests re-seen in v12 code
// ------------------------------------------------------------------------------------------------
enum aiNPC_FLAGS {
    AIF_FOLLOW_CRUMBS    = 0x00000001, // chasing along the enemy breadcrumb trail (PostThink advances crumbID)
    AIF_04               = 0x00000004, // ?  (1.2 ProcessFRAME also tests it: 0x810004)
    AIF_CRUMB_REACHED    = 0x00000008, // ?  PostThink: clears FOLLOW_CRUMBS, sets 0x40
    AIF_HAS_WAYPOINT     = 0x00000010, // AI_PROP actorWayPoint found and unlocked
    AIF_FALLING          = 0x00000100, // UpdateTimers integrates fallSpeed with fallAccel
    AIF_ROOT_MOTION      = 0x00000200, // current anim moves the actor (SetMovementSpeed of tiger etc.)  // ?
    AIF_INACTIVE         = 0x00010000, // not activated yet / deactivated (Activate clears, Deactivate sets). Set at INIT.
    AIF_PLR_NO_VISIBILITY= 0x00020000, // AI_PROP isPlrNoVisibility: never self-activates on sight
    AIF_SNIPER           = 0x00040000, // AI_PROP isSniper or "_sniper" in the class name (shootRange 800)
    AIF_WAS_ACTIVATED    = 0x00080000, // set by Activate (inactivity death only counts after this)
    AIF_SPAWNED          = 0x00100000, // created by an aiDOMAIN_SPAWN (aiINIT_DATA given to ProcessINIT)
    AIF_DEAD             = 0x00200000, // StartDeath
    AIF_400000           = 0x00400000, // set at INIT; with DEAD it skips Think/plan in UpdateOnServerOnly  // ?
    AIF_AI_OFF           = 0x00800000, // ProcessFRAME does nothing (frozen / scripted)
    AIF_1000000          = 0x01000000, // set at INIT; controls inst flag 0x2000000 in ProcessFRAME  // ? (render/visibility)
    AIF_ENEMY_VISIBLE    = 0x10000000, // DecideEnemy: IsPlrVisible() succeeded
};
// aiNPC::aiFlags2 (v12 +0x473, feb +0x44F)
enum aiNPC_FLAGS2 {
    AIF2_NAV_AREA        = 0x00000008, // navArea domain valid
    AIF2_NAV_BAN_CROSS   = 0x00000010, // navBanCross domain valid
    AIF2_NICHE           = 0x00000020, // "_niche" actor with a waypoint (aiNPC_WR::ParseNameClass)
    AIF2_BOSS            = 0x00040000, // registered with aiPLANNER::SetBoss; 1.2: keeps thinking during cut-scenes
};
// aiNPC_WR::wrFlags (v12 +0x543, feb +0x51B)
enum aiNPC_WR_FLAGS {
    AIWR_DAMAGED_THIS_FRAME = 0x00000001, // ProcessCDT_FIREABLE: blood/fallback only once per frame  // ?
    AIWR_NO_BLOOD_04     = 0x00000004, // ?
    AIWR_ALTAR_HIDDEN    = 0x00000008, // "isAltarOpp" opponent waiting behind an altar (msg 0x4D2 unhides). 1.2: also blocks movement (vslot 58)
    AIWR_AIM_PLAIN       = 0x00000010, // AI_PROP Aiming = "Plain" (no predictive aiming)
    AIWR_FALLBACK        = 0x00000100, // AI_PROP isFallback (knock-back on heavy hits)
    AIWR_WAYP_TURN       = 0x00000400, // AI_PROP isWayPTurn
    AIWR_INITED          = 0x00010000, // always set by aiNPC_WR::ProcessINIT
    AIWR_EXPLODED        = 0x01000000, // already gibbed
    AIWR_NO_APPEAR_SFX   = 0x02000000, // 1.2 only: "_noappearsfx" in the class name (aiNPC_WR::ParseNameClass v12 0x00452D5C)
};

// ------------------------------------------------------------------------------------------------
// aiENEMY: who the NPC is fighting (embedded at aiNPC v12 +0x4DF, feb +0x4BB)
// ------------------------------------------------------------------------------------------------
struct aiENEMY {                     // size 0x0C
    plrPLAYER *plr;                  // +0x00 player if the target is a player, else 0
    float      timer;                // +0x04 time with this target (UpdateTimers)  // ?
    entENTITY *ent;                  // +0x08 target entity (player or NPC)
    aiENEMY() { plr = 0; timer = 0; ent = 0; }
    void GetPos(m3dV *out);
    void GetDir(m3dV *out);
    void Assign(entENTITY *e);
    int  IsDead();
    int  IsActive();
    void ApplySpeed(m3dV *v);
    void ProcessCDT_FIREABLE(dmgDAMAGE *d);
    void *GetBreadCrumb(int back);
    int  GetMostRecentCrumbID();
    void UpdateTimers();
    float GetSpeedScalar();          // v12 0x0054C340 (feb 0x00531090) how fast the enemy moves
    // ctor v12 0x00548D00 (feb 0x005353C0); Assign v12 0x0054C480 (feb 0x00531230); IsDead v12 0x0054C4C0 (feb 0x00531270)
    // GetPos v12 0x0054C250 (feb 0x00530FE0); GetDir v12 0x0054C2A0 (feb 0x00531010); GetBreadCrumb v12 0x0054C450 (feb 0x00530FC0)
    // ApplySpeed v12 0x0054C520 (feb 0x005312D0); ProcessCDT_FIREABLE v12 0x0054C5A0 (feb 0x00531310)
    // GetSpeed feb 0x00531040, GetSpeedScalar feb 0x00531090, GetMostRecentCrumbID feb 0x00530FA0, UpdateTimers feb 0x00531340 (v12 ?)
};

// aiINIT_DATA: msgDATA passed with INIT when a spawn domain creates the NPC (ctor v12 0x005489A0, feb 0x00535080)
struct aiINIT_DATA /* : msgDATA (8 bytes) */ {
    u8               hdr[8];         // +0x00 msgDATA, packId aiINIT_DATA_PACK, size 0x10; +0x04 'ISIA'
    aiDOMAIN_SPAWN  *spawnDomain;    // +0x08 -> aiNPC::spawnDomain
    int              spawnParam;     // +0x0C
    aiINIT_DATA();                   // v12 0x005489A0 (src/gs_shared/ai_ctrl.cpp)
};

// ------------------------------------------------------------------------------------------------
// aiNPC  -- base of every enemy.   vtable v12 0x00608878 (feb 0x005DD448)   ctor v12 0x00547A60 (feb 0x00534210)
//   size v12 0x543 (feb 0x51B). Abstract: only enemies are registered (entRegisterClass, sig AI_SIG_NPC, type 0x32)
// ------------------------------------------------------------------------------------------------
class aiNPC : public entENTITY {
public:
    // --- vtable in slot order. "v12" = the aiNPC vtable entry (base implementation), slot 98+ = aiNPC_WR entries
    //     (aiNPC_WR has no vtable of its own; value = the one shared by most enemies). The slot map feb->v12 is:
    //     0..7 same | 1.2 inserts 8 | feb 8..19 +1 | feb 20..22 removed (entENTITY MP_UPD_POS) | 1.2 inserts 23 |
    //     feb 23..43 -1 | 1.2 inserts 43 | feb 44..55 same | feb 56 (SetDirection) removed | feb 57,58 -1 |
    //     1.2 inserts 58 | feb 59..65 same | 1.2 inserts 66,67 | feb 66..87 +2 | 1.2 inserts 90 | feb 88.. +3.
    //     Verified on all 24 enemy vtables (votes 24/24 for every slot >= 59).
    //  slot +0x000 [  0]  v12 0x00547E40   feb +0x000 [  0] 0x00456470  scalar deleting dtor
    //  slot +0x004 [  1]  v12 0x00547EC0   feb +0x004 [  1] 0x00534640  aiNPC::ProcessMsg
    //  slot +0x008 [  2]  v12 0x00506740   feb +0x008 [  2] 0x004F40E0  entENTITY::PreProcessMsg
    //  slot +0x00C [  3]  v12 0x005068A0   feb +0x00C [  3] 0x004F4250  entENTITY::PostProcessMsg
    //  slot +0x010 [  4]  v12 0x0050A580   feb +0x010 [  4] 0x004F7D30  entENTITY::IsMPSynchableExisting
    //  slot +0x014 [  5]  v12 0x0050A5F0   feb +0x014 [  5] 0x004F7D60  entENTITY::GetMPSysSyncData
    //  slot +0x018 [  6]  v12 0x0050A6F0   feb +0x018 [  6] 0x004F7E60  entIACTIVE_OBJ::IdentifyMPSysSyncData
    //  slot +0x01C [  7]  v12 0x0054D270   feb +0x01C [  7] 0x00531F70  aiNPC::GetMP_UPD_STATE
    //  slot +0x020 [  8]  v12 0x0054D6A0   (no feb slot)                1.2 NEW ProcessMP_UPD_STATE (see entity.h)
    //  slot +0x024 [  9]  v12 0x00506DC0   feb +0x020 [  8] 0x004F4780  entENTITY::ParseIActions
    //  slot +0x028 [ 10]  v12 0x0050BB40   feb +0x024 [  9] 0x004F9080  entIACTIVE_OBJ::AddIAction
    //  slot +0x02C [ 11]  v12 0x00403A20   feb +0x028 [ 10] 0x00403690  ProcessLOCK (empty)
    //  slot +0x030 [ 12]  v12 0x00403A20   feb +0x02C [ 11] 0x00403690  ProcessUNLOCK (empty)
    //  slot +0x034 [ 13]  v12 0x0054CB90   feb +0x030 [ 12] 0x00531890  aiNPC::ProcessMP_GET_SYNC_DATA
    //  slot +0x038 [ 14]  v12 0x0054CDA0   feb +0x034 [ 13] 0x00531AA0  aiNPC::ProcessMP_WRITE_SYNC_DATA
    //  slot +0x03C [ 15]  v12 0x0054D050   feb +0x038 [ 14] 0x00531D50  aiNPC::ProcessMP_READ_SYNC_DATA
    //  slot +0x040 [ 16]  v12 0x004031F0   feb +0x03C [ 15] 0x004036A0  SetupAniSystem (base return 1; every enemy overrides: names its anim sequences)
    //  slot +0x044 [ 17]  v12 0x0054E880   feb +0x040 [ 16] 0x00533440  aiNPC::GetFloorInfo
    //  slot +0x048 [ 18]  v12 0x00507200   feb +0x044 [ 17] 0x004F4B90  entENTITY::FloorPlace
    //  slot +0x04C [ 19]  v12 0x00403A30   feb +0x048 [ 18] 0x004036C0  entENTITY::GetAccel
    //  slot +0x050 [ 20]  v12 0x0050AA30   feb +0x04C [ 19] 0x004F80E0  entENTITY::GetDeadReckonData
    //  slot +0x054 [ 21]  v12 0x00430AC0   feb +0x05C [ 23] 0x0042B490  aiNPC::GetCenter
    //  slot +0x058 [ 22]  v12 0x00507D30   feb +0x060 [ 24] 0x004F56C0  entENTITY::AssignANIM_ST_ANIMATE_HIDDEN
    //  slot +0x05C [ 23]  v12 0x0054EDD0   (no feb slot)                1.2 NEW entENTITY slot (see entity.h)
    //  slot +0x060 [ 24]  v12 0x00506C00   feb +0x064 [ 25] 0x004F45C0  entENTITY::ParsePS
    //  slot +0x064 [ 25]  v12 0x005484F0   feb +0x068 [ 26] 0x0056770B  ParseNameClass (feb: pure; 1.2 base parses "_sniper")
    //  slot +0x068 [ 26]  v12 0x00548540   feb +0x06C [ 27] 0x00534C90  aiNPC::SetupHeightCDTParams
    //  slot +0x06C [ 27]  v12 0x0054EB40   feb +0x070 [ 28] 0x005336A0  aiNPC::SettleSurface
    //  slot +0x070 [ 28]  v12 0x0054EEE0   feb +0x074 [ 29] 0x00533A00  aiNPC::Colorize
    //  slot +0x074 [ 29]  v12 0x0054EED0   feb +0x078 [ 30] 0x005339F0  aiNPC::GetStateEntExcludeCDT
    //  slot +0x078 [ 30]  v12 0x0054E160   feb +0x07C [ 31] 0x00532DA0  aiNPC::GetFloorPlaceRefiner
    //  slot +0x07C [ 31]  v12 0x00403A20   feb +0x080 [ 32] 0x00403690  FillSoundFlags (base empty)
    //  slot +0x080 [ 32]  v12 0x00548990   feb +0x084 [ 33] 0x00535070  aiNPC::PlaySounds
    //  slot +0x084 [ 33]  v12 0x00401380   feb +0x088 [ 34] 0x00403BE0  vslot 33 (base empty) // ?
    //  slot +0x088 [ 34]  v12 0x0054BE00   feb +0x08C [ 35] 0x00530BE0  aiNPC::StartDeath
    //  slot +0x08C [ 35]  v12 0x0054BEB0   feb +0x090 [ 36] 0x00530C50  aiNPC::UpdateDead
    //  slot +0x090 [ 36]  v12 0x00401380   feb +0x094 [ 37] 0x00403BE0  vslot 36 (base empty) // ?
    //  slot +0x094 [ 37]  v12 0x00550590   feb +0x098 [ 38] 0x00535F00  aiNPC::PlanMove
    //  slot +0x098 [ 38]  v12 0x0054FB90   feb +0x09C [ 39] 0x00535790  aiNPC::SelectGoal
    //  slot +0x09C [ 39]  v12 0x00550C10   feb +0x0A0 [ 40] 0x00536580  aiNPC::FillGoalDirection
    //  slot +0x0A0 [ 40]  v12 0x00550700   feb +0x0A4 [ 41] 0x00536070  aiNPC::AnalyzeMove
    //  slot +0x0A4 [ 41]  v12 0x00403A20   feb +0x0A8 [ 42] 0x00403690  SetMovementSpeed (base empty)
    //  slot +0x0A8 [ 42]  v12 0x00550280   feb +0x0AC [ 43] 0x00535DF0  aiNPC::StartResolveCycle
    //  slot +0x0AC [ 43]  v12 0x004031F0   (no feb slot)                1.2 NEW: bool vslot (base return 1)  // ? name
    //  slot +0x0B0 [ 44]  v12 0x0054D7F0   feb +0x0B0 [ 44] 0x00532420  aiNPC::MoveToGoal
    //  slot +0x0B4 [ 45]  v12 0x0054DBC0   feb +0x0B4 [ 45] 0x00532800  aiNPC::ClampGoalFrame
    //  slot +0x0B8 [ 46]  v12 0x0054E200   feb +0x0B8 [ 46] 0x00532E20  aiNPC::ProcessVoluntaryPhysics
    //  slot +0x0BC [ 47]  v12 0x0054E6B0   feb +0x0BC [ 47] 0x00533290  aiNPC::ProcessPassivePhysics
    //  slot +0x0C0 [ 48]  v12 0x0054FFA0   feb +0x0C0 [ 48] 0x00535B00  aiNPC::StartJump
    //  slot +0x0C4 [ 49]  v12 0x0054F990   feb +0x0C4 [ 49] 0x005355C0  aiNPC::Animate
    //  slot +0x0C8 [ 50]  v12 0x00502570   feb +0x0C8 [ 50] 0x00579E90  EvalAnimGoalSeq (base: return 0) -> picks next anim sequence
    //  slot +0x0CC [ 51]  v12 0x00431E30   feb +0x0CC [ 51] 0x0042C490  aiNPC::GetAnimPlayRate
    //  slot +0x0D0 [ 52]  v12 0x00401380   feb +0x0D0 [ 52] 0x00403BE0  ProcessActionFrame (base empty) -> anim event: hit/throw/spawn
    //  slot +0x0D4 [ 53]  v12 0x00401380   feb +0x0D4 [ 53] 0x00403BE0  AnimateProcedural (base empty)
    //  slot +0x0D8 [ 54]  v12 0x0054F770   feb +0x0D8 [ 54] 0x00535540  aiNPC::MoveRootedPivoted
    //  slot +0x0DC [ 55]  v12 0x0054ED60   feb +0x0DC [ 55] 0x005338E0  aiNPC::GetWpnTip
    //  slot +0x0E0 [ 56]  v12 0x0054ED80   feb +0x0E4 [ 57] 0x00533900  aiNPC::GetPlayerTargetPos
    //  slot +0x0E4 [ 57]  v12 0x0054F260   feb +0x0E8 [ 58] 0x00533CD0  aiNPC::IsReadyActivate
    //  slot +0x0E8 [ 58]  v12 0x0057FFFB   (no feb slot)                1.2 NEW (pure in aiNPC): BOOL CanPlanMove(); aiNPC_WR impl 0x00453F00 = !(wrFlags & 8) (hidden altar opponent)
    //  slot +0x0EC [ 59]  v12 0x0054F330   feb +0x0EC [ 59] 0x00533DA0  aiNPC::Think
    //  slot +0x0F0 [ 60]  v12 0x0054F350   feb +0x0F0 [ 60] 0x00533DC0  aiNPC::PostThink
    //  slot +0x0F4 [ 61]  v12 0x00548880   feb +0x0F4 [ 61] 0x00534F90  aiNPC::UpdateTimers
    //  slot +0x0F8 [ 62]  v12 0x0054F1F0   feb +0x0F8 [ 62] 0x00533C90  aiNPC::Deactivate
    //  slot +0x0FC [ 63]  v12 0x0054C720   feb +0x0FC [ 63] 0x00531480  aiNPC::ShouldChangeEnemy
    //  slot +0x100 [ 64]  v12 0x0054C970   feb +0x100 [ 64] 0x005316A0  aiNPC::SelectNewEnemy
    //  slot +0x104 [ 65]  v12 0x0054CA20   feb +0x104 [ 65] 0x00531720  aiNPC::SelectFromSortedPlayers
    //  slot +0x108 [ 66]  v12 0x0048D590   (no feb slot)                1.2 NEW (base return 1?) // ? (0x0048D590 shared stub)
    //  slot +0x10C [ 67]  v12 0x0048DF30   (no feb slot)                1.2 NEW (base return 0) // ? ; overridden by diskbl/glad/mino/medusa
    //  slot +0x110 [ 68]  v12 0x0054F440   feb +0x108 [ 66] 0x00533EE0  aiNPC::UpdateOnServerOnly
    //  slot +0x114 [ 69]  v12 0x0054D340   feb +0x10C [ 67] 0x00532010  aiNPC::UpdateOnClientOnly
    //  slot +0x118 [ 70]  v12 0x0054F5A0   feb +0x110 [ 68] 0x00534010  aiNPC::UpdateOnServerAndClient
    //  slot +0x11C [ 71]  v12 0x0048DF30   feb +0x114 [ 69] 0x00403680  GetLayer1State (base 0)
    //  slot +0x120 [ 72]  v12 0x0048DF30   feb +0x118 [ 70] 0x00403680  GetLayer2State // ?
    //  slot +0x124 [ 73]  v12 0x0048DF30   feb +0x11C [ 71] 0x00403680  GetLayer3State
    //  slot +0x128 [ 74]  v12 0x00403950   feb +0x120 [ 72] 0x00403870  SetLayer1State (base empty)
    //  slot +0x12C [ 75]  v12 0x00403950   feb +0x124 [ 73] 0x00403870  SetLayer2State // ?
    //  slot +0x130 [ 76]  v12 0x00403950   feb +0x128 [ 74] 0x00403870  SetLayer3State
    //  slot +0x134 [ 77]  v12 0x0054D370   feb +0x12C [ 75] 0x00532040  aiNPC::NotifyHealthChange
    //  slot +0x138 [ 78]  v12 0x0054D3F0   feb +0x130 [ 76] 0x005320C0  aiNPC::NotifyDeath
    //  slot +0x13C [ 79]  v12 0x00403A20   feb +0x134 [ 77] 0x00403690  vslot (base empty) called by Animate  // ?
    //  slot +0x140 [ 80]  v12 0x00548120   feb +0x138 [ 78] 0x005348A0  aiNPC::ProcessINIT
    //  slot +0x144 [ 81]  v12 0x00548620   feb +0x13C [ 79] 0x00534D70  aiNPC::ProcessTERM
    //  slot +0x148 [ 82]  v12 0x005485A0   feb +0x140 [ 80] 0x00534CF0  aiNPC::ProcessINIT_LEVEL
    //  slot +0x14C [ 83]  v12 0x00548760   feb +0x144 [ 81] 0x00534EA0  aiNPC::ProcessFRAME
    //  slot +0x150 [ 84]  v12 0x0054BD10   feb +0x148 [ 82] 0x00530AF0  aiNPC::ProcessCDT_FIREABLE
    //  slot +0x154 [ 85]  v12 0x00403200   feb +0x14C [ 83] 0x004036B0  ProcessCDT(cdt) (base return 0)
    //  slot +0x158 [ 86]  v12 0x00548680   feb +0x150 [ 84] 0x0049A5B0  ProcessASK_KEEP_LEVEL_RELOAD
    //  slot +0x15C [ 87]  v12 0x0054D450   feb +0x154 [ 85] 0x00532130  aiNPC::ProcessMP_NOTIFY
    //  slot +0x160 [ 88]  v12 0x005486C0   feb +0x158 [ 86] 0x00534E00  aiNPC::ProcessIACTION
    //  slot +0x164 [ 89]  v12 0x00548690   feb +0x15C [ 87] 0x00534DD0  aiNPC::ProcessDESTROY_ENT
    //  slot +0x168 [ 90]  v12 0x00430AD0   (no feb slot)                1.2 NEW (base 0x00430AD0)  // ? ; diskobolus overrides
    //  slot +0x16C [ 91]  v12 0x00430AE0   feb +0x160 [ 88] 0x0042B4A0  aiNPC::Freeze
    //  slot +0x170 [ 92]  v12 0x00430AF0   feb +0x164 [ 89] 0x0042B4B0  aiNPC::GetGravity
    //  slot +0x174 [ 93]  v12 0x0054E850   feb +0x168 [ 90] 0x00533410  aiNPC::ApplySpeed
    //  slot +0x178 [ 94]  v12 0x0054F190   feb +0x16C [ 91] 0x00533C40  aiNPC::Activate
    //  slot +0x17C [ 95]  v12 0x0054F250   feb +0x170 [ 92] 0x00533CC0  aiNPC::ActivatePlrVisibility
    //  slot +0x180 [ 96]  v12 0x0054F120   feb +0x174 [ 93] 0x00533BD0  aiNPC::IsStayOn
    //  slot +0x184 [ 97]  v12 0x0054E870   feb +0x178 [ 94] 0x00533430  aiNPC::ApplySpeedTrampoline
    //  slot +0x188 [ 98]  v12 0x00453B80   feb +0x17C [ 95] 0x0044B590  aiNPC_WR::StartPassivePhysics
    //  slot +0x18C [ 99]  v12 0x004539B0   feb +0x180 [ 96] 0x0044B3C0  aiNPC_WR::SetInitDeadSpeed
    //  slot +0x190 [100]  v12 0x004537F0   feb +0x184 [ 97] 0x0044B220  aiNPC_WR::SetInitShootFallBack
    //  slot +0x194 [101]  v12 0x0048DF30   feb +0x188 [ 98] 0x00403680  IsAnimSeqGoodForFallback (base 0)
    //  slot +0x198 [102]  v12 0x00403940   feb +0x18C [ 99] 0x0057BCE0  AdjustSpeedsOnFallbackStart (base empty)
    //  slot +0x19C [103]  v12 0x00453140   feb +0x190 [100] 0x0044AB60  aiNPC_WR::DecideExplosion
    //  slot +0x1A0 [104]  v12 0x004530F0   feb +0x194 [101] 0x0044AB10  aiNPC_WR::Explode
    //  slot +0x1A4 [105]  v12 0x00453DC0   feb +0x198 [102] 0x0044B7D0  aiNPC_WR::InitSFX
    //  slot +0x1A8 [106]  v12 0x00453EB0   feb +0x19C [103] 0x0044B8D0  aiNPC_WR::SyncSFX
    //  slot +0x1AC [107]  v12 0x00401380   feb +0x1A0 [104] 0x00403BE0  TermSFX (base empty)
    //  slot +0x1B0 [108]  v12 0x004031F0   feb +0x1A4 [105] 0x004036A0  DoesNPCAllowSFXAppear (base 1)
    //  slot +0x1B4 [109]  v12 0x004031F0   feb +0x1A8 [106] 0x004036A0  IsGibbable (base 1): DecideExplosion gibs the NPC (vslot Explode) when an explosive hit (weapon type 6,9,10) lands < 1.0 m away; 1.2 aiRAT returns 0
    //  slot +0x1B8 [110]  v12 0x0048DF30   feb +0x1AC [107] 0x00403680  IsObstAvoidForced (base 0; checked before AI_PROP ObstAvoid)  // ? name
    //  slot +0x1BC [111]  v12 0x004543F0   feb +0x1B0 [108] 0x0044BD60  aiNPC_WR::RestrictGoalByBanCross
    //  slot +0x1C0 [112]  v12 0x004544C0   feb +0x1B4 [109] 0x0044BE30  aiNPC_WR::SelectGoalToRunAway
    //  slot +0x1C4 [113]  v12 0x00403200   feb +0x1B8 [110] 0x004036B0  IsArmoredObject(objName) (base 0) -- 1.2 keeps overrides only in cyclop/gefest/zevs
    //  slot +0x1C8 [114]  v12 0x0048DF30   feb +0x1BC [111] 0x00403680  vslot (base 0)  // ?
    //  slot +0x1CC [115]  v12 0x004546E0   feb +0x1C0 [112] 0x0044C050  aiNPC_WR::ProcessAltarOppUnhide

    // --- the slots, declared in v12 order ---------------------------------------------------------
    aiNPC();                                                        // v12 0x00547A60
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [ 1] v12 0x00547EC0
    int  GetMP_UPD_STATE(void **msg);                               // [ 7] v12 0x0054D270
    int  ProcessMP_UPD_STATE(void *msg);                            // [ 8] v12 0x0054D6A0
    void ProcessMP_GET_SYNC_DATA(void *d);                          // [13] v12 0x0054CB90
    void ProcessMP_WRITE_SYNC_DATA(gsMSG_MP_SAVELOAD *d);           // [14] v12 0x0054CDA0
    void ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d);            // [15] v12 0x0054D050
    BOOL32 GetFloorInfo(cdtINFO *ci);                               // [17] v12 0x0054E880
    void GetCenter(m3dV *out);                                      // [21] v12 0x00430AC0
    void Slot23(void *speed);                                       // [23] v12 0x0054EDD0 SetDirection (a slot again in 1.2)
    virtual void ParseNameClass();                                  // [25] +0x064 v12 0x005484F0
    virtual void SetupHeightCDTParams();                            // [26] +0x068 v12 0x00548540
    virtual void SettleSurface();                                   // [27] +0x06C v12 0x0054EB40
    virtual void Colorize(int skip);                                // [28] +0x070 v12 0x0054EEE0
    virtual int  GetStateEntExcludeCDT();                           // [29] +0x074 v12 0x0054EED0
    virtual class cdtREFINE *GetFloorPlaceRefiner();                // [30] +0x078 v12 0x0054E160
    virtual void FillSoundFlags();                                  // [31] +0x07C
    virtual void PlaySounds();                                      // [32] +0x080 v12 0x00548990
    virtual void Slot33();                                          // [33] +0x084 base empty
    virtual void StartDeath(dmgDAMAGE *d);                          // [34] +0x088 v12 0x0054BE00
    virtual void UpdateDead();                                      // [35] +0x08C v12 0x0054BEB0
    virtual void Slot36();                                          // [36] +0x090 base empty
    virtual void PlanMove();                                        // [37] +0x094 v12 0x00550590
    virtual int  SelectGoal();                                      // [38] +0x098 v12 0x0054FB90
    virtual void FillGoalDirection();                               // [39] +0x09C v12 0x00550C10
    virtual void AnalyzeMove();                                     // [40] +0x0A0 v12 0x00550700
    virtual void SetMovementSpeed();                                // [41] +0x0A4
    virtual void StartResolveCycle();                               // [42] +0x0A8 v12 0x00550280
    virtual int  Slot43();                                          // [43] +0x0AC base 1
    virtual void MoveToGoal();                                      // [44] +0x0B0 v12 0x0054D7F0
    virtual void ClampGoalFrame(float speed, m3dV *dir, m3dV *goal);// [45] +0x0B4 v12 0x0054DBC0
    virtual void ProcessVoluntaryPhysics();                         // [46] +0x0B8 v12 0x0054E200
    virtual void ProcessPassivePhysics();                           // [47] +0x0BC v12 0x0054E6B0
    virtual void StartJump(float speed, float up);                  // [48] +0x0C0 v12 0x0054FFA0
    virtual void Animate();                                         // [49] +0x0C4 v12 0x0054F990
    virtual int  EvalAnimGoalSeq();                                 // [50] +0x0C8 base 0
    virtual float GetAnimPlayRate();                                // [51] +0x0CC v12 0x00431E30
    virtual void ProcessActionFrame();                              // [52] +0x0D0 base empty
    virtual void AnimateProcedural();                               // [53] +0x0D4 base empty
    virtual void MoveRootedPivoted();                               // [54] +0x0D8 v12 0x0054F770
    virtual void GetWpnTip(m3dV *out);                              // [55] +0x0DC v12 0x0054ED60
    virtual void GetPlayerTargetPos(m3dV *out);                     // [56] +0x0E0 v12 0x0054ED80
    virtual int  IsReadyActivate();                                 // [57] +0x0E4 v12 0x0054F260
    virtual int  CanPlanMove() = 0;                                 // [58] +0x0E8 pure here; aiNPC_WR 0x00453F00
    virtual void Think();                                           // [59] +0x0EC v12 0x0054F330
    virtual void PostThink();                                       // [60] +0x0F0 v12 0x0054F350
    virtual void UpdateTimers();                                    // [61] +0x0F4 v12 0x00548880
    virtual void Deactivate();                                      // [62] +0x0F8 v12 0x0054F1F0
    virtual int  ShouldChangeEnemy();                               // [63] +0x0FC v12 0x0054C720
    virtual entENTITY *SelectNewEnemy(int *sawIt);                  // [64] +0x100 v12 0x0054C970
    virtual entENTITY *SelectFromSortedPlayers();                   // [65] +0x104 v12 0x0054CA20
    virtual int  Slot66(void *plr);                                 // [66] +0x108 base 1: may it pick this player
    virtual int  Slot67();                                          // [67] +0x10C base 0
    virtual void UpdateOnServerOnly();                              // [68] +0x110 v12 0x0054F440
    virtual void UpdateOnClientOnly();                              // [69] +0x114 v12 0x0054D340
    virtual void UpdateOnServerAndClient();                         // [70] +0x118 v12 0x0054F5A0
    virtual int  GetLayer1State();                                  // [71] +0x11C
    virtual int  GetLayer2State();                                  // [72] +0x120
    virtual int  GetLayer3State();                                  // [73] +0x124
    virtual void SetLayer1State(int s);                             // [74] +0x128
    virtual void SetLayer2State(int s);                             // [75] +0x12C
    virtual void SetLayer3State(int s);                             // [76] +0x130
    virtual void NotifyHealthChange(dmgDAMAGE *d);                  // [77] +0x134 v12 0x0054D370
    virtual void NotifyDeath(dmgDAMAGE *d);                         // [78] +0x138 v12 0x0054D3F0
    virtual void Slot79();                                          // [79] +0x13C base empty, on action frames
    virtual int  ProcessINIT(void *init);                           // [80] +0x140 v12 0x00548120
    virtual void ProcessTERM();                                     // [81] +0x144 v12 0x00548620
    virtual void ProcessINIT_LEVEL();                               // [82] +0x148 v12 0x005485A0
    virtual void ProcessFRAME();                                    // [83] +0x14C v12 0x00548760
    virtual int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                 // [84] +0x150 v12 0x0054BD10
    virtual void ProcessCDT(cdtINFO *ci);                           // [85] +0x154 base nothing
    virtual int  ProcessASK_KEEP_LEVEL_RELOAD();                    // [86] +0x158 v12 0x00548680
    virtual void ProcessMP_NOTIFY(gsEVENT *ev);                     // [87] +0x15C v12 0x0054D450
    virtual void ProcessIACTION(void *ia);                          // [88] +0x160 v12 0x005486C0
    virtual void ProcessDESTROY_ENT(entENTITY *e);                  // [89] +0x164 v12 0x00548690
    virtual float Slot90();                                         // [90] +0x168 v12 0x00430AD0 heightUp1
    virtual void Freeze();                                          // [91] +0x16C v12 0x00430AE0
    virtual float GetGravity();                                     // [92] +0x170 v12 0x00430AF0
    virtual void ApplySpeed(m3dV *v);                               // [93] +0x174 v12 0x0054E850
    virtual int  Activate(int force);                               // [94] +0x178 v12 0x0054F190
    virtual void ActivatePlrVisibility();                           // [95] +0x17C v12 0x0054F250
    virtual int  IsStayOn(animINST *i);                             // [96] +0x180 v12 0x0054F120
    virtual void ApplySpeedTrampoline(m3dV *v);                     // [97] +0x184 v12 0x0054E870

    // --- fields (aiNPC own part, v12 offsets) ---------------------------------------------------
    u32                  navFlags;                  // v12 +0x16B (feb +0x14F) bit 0x20000 = no obstacle avoidance (AI_PROP ObstAvoid=0); bit 0x100 strafe; AvoidObstacles/StartResolveCycle [v12: v12 ctor 0x00547A60 [esi+0x16B]=0]
    float                moveSpeed;                 // v12 +0x16F (feb +0x153) m/s; ctor 3.5; per-class SetMovementSpeed sets walk/run (tiger 3.5 / 16) [v12: v12 ctor [esi+0x16F]=3.5]
    float                minTurnRadius;             // v12 +0x173 (feb +0x157) ctor 5.0; ClampGoalFrame/GetClampingSpeeds  // ? [v12: v12 ctor [esi+0x173]=5.0]
    float                turnSpeed;                 // v12 +0x177 (feb +0x15B) deg/s; ctor 180; per-anim in SetMovementSpeed [v12: v12 ctor [esi+0x177]=180]
    float                maxTurnStep;               // v12 +0x17B (feb +0x15F) deg; ctor 90; ClampGoalFrame clamps heading change to this [v12: v12 ctor [esi+0x17B]=90]
    objOBJ*              rootObj;                   // v12 +0x17F (feb +0x163) "ROOT" object of the model (GetPosRooted/SetPosRooted)
    objOBJ*              pivotObj;                  // v12 +0x183 (feb +0x167) IsPivotUsable / SetDirection  // ?
    float                radCDT;                    // v12 +0x187 (feb +0x16B) collision radius, AI_PROP radCDT; ctor 0.7 [v12: v12 ctor [esi+0x187]=0.7]
    float                heightLowTester;           // v12 +0x18B (feb +0x16F) ctor 0.9  // ? (floor test) [v12: v12 ctor [esi+0x18B]=0.9]
    float                heightCDT;                 // v12 +0x18F (feb +0x173) ctor 1.1; raised to heightUpTester by SetupHeightCDTParams [v12: v12 ctor [esi+0x18F]=1.1]
    float                unk177;                    // v12 +0x193 (feb +0x177) feb ctor 30.0, 1.2 ctor 1.1 (CHANGED) ConstrainMotion  // ? [v12: v12 ctor [esi+0x193]=1.1]
    float                unk17B;                    // v12 +0x197 (feb +0x17B) ctor 30.0 ConstrainMotion/ProcessVoluntaryPhysics  // ? [v12: v12 ctor [esi+0x197]]
    float                alertAngle;                // v12 +0x19B (feb +0x17F) ctor 180; reset to 180 by ProcessCDT_FIREABLE when hit  // ? (FOV / attention) [v12: v12 0x0054BD72 [esi+0x19B]=180]
    float                gravity;                   // v12 +0x19F (feb +0x183) ctor 30; ProcessVoluntaryPhysics: vy -= dt*gravity [v12: v12 ctor [esi+0x19F]=30]
    float                aimAngle;                  // v12 +0x1A3 (feb +0x187) ctor 90; SelectGoal/GetShootPjlPhysParam/CheckShootCond  // ? [v12: v12 ctor [esi+0x1A3]=90]
    float                shootRange;                // v12 +0x1A7 (feb +0x18B) ctor 40; "_sniper" name -> 800 (AI shoot/see distance) [v12: v12 0x00548516 [esi+0x1A7]=800]
    float                meetDist;                  // v12 +0x1AB (feb +0x18F) ctor 10; PlanMove: with aiFlags2 0x10000 no closing in nearer than this
    float                navStuckTime;              // v12 +0x1AF (feb +0x193) ctor 2.0 (PlanMove)  // ? [v12: v12 ctor [esi+0x1AF]=2.0]
    float                navArriveDist;             // v12 +0x1B3 (feb +0x197) ctor 0.5; AnalyzeMove  // ? ; tiger 2.85 [v12: v12 ctor [esi+0x1B3]=0.5]
    float                new1B7;                    // v12 +0x1B7 1.2 NEW (no feb twin), ctor 0.3f [v12 ctor 0x00547A60 writes 0x3E99999A]  // ? meaning
    float                navAnalyzeDist;            // v12 +0x1BB (feb +0x19B) ctor 4.0; AnalyzeMove  // ? [v12: v12 ctor [esi+0x1BB]=4.0 (after the 1.2 insert)]
    u8   pad_1BF[0x4];
    float                navAnalyzeAngle;           // v12 +0x1C3 (feb +0x1A3) ctor 10.0; AnalyzeMove  // ? [v12: v12 ctor [esi+0x1C3]=10]
    float                navUnk1A7;                 // v12 +0x1C7 (feb +0x1A7) ctor 20.0; tiger 10  // ? [v12: v12 ctor [esi+0x1C7]=20]
    u8                   navArea[0x18C];            // v12 +0x1CB (feb +0x1AB) opaque, AI_PROP navArea polygon (vtable +4 = Init); flags2 bit 8 when valid // ? size
    u8                   navBanCross[0x3C];         // v12 +0x357 (feb +0x337) opaque, AI_PROP navBanCross polygon; flags2 bit 0x10 when valid // ? size
    float                passivePhys373;            // v12 +0x393 (feb +0x373) ctor 0.25 StartPassivePhysics  // ? [v12: v12 ctor [esi+0x393]=0.25]
    float                fallAccel;                 // v12 +0x397 (feb +0x377) ctor -3.0; UpdateTimers: fallSpeed += dt*fallAccel (flag 0x100) [v12: v12 ctor [esi+0x397]=-3]
    float                fallSpeedMin;              // v12 +0x39B (feb +0x37B) ctor 0; clamp for fallSpeed [v12: v12 ctor [esi+0x39B]=0]
    float                unk37F;                    // v12 +0x39F (feb +0x37F) ctor -888.0 (sentinel)  // ? [v12: v12 ctor [esi+0x39F]=-888]
    float                heightUp1;                 // v12 +0x3A3 (feb +0x383) ctor 1.7  // ? [v12: v12 ctor [esi+0x3A3]=1.7]
    float                heightUpTester;            // v12 +0x3A7 (feb +0x387) AI_PROP heightUpTester (also level psSHEET AI_PROP); ctor 1.1 [v12: SetupHeightCDTParams]
    m3dV                 obstPos;                   // v12 +0x3AB (feb +0x38B) AvoidObstacles: where it last looked ahead
    m3dV                 obstEnd;                   // v12 +0x3B7 (feb +0x397) obstPos + facing
    float                resolveTimer;              // v12 +0x3C3 (feb +0x3A3) m3dUpdateTimeField in UpdateTimers  // ?
    float                resolvePeriod;             // v12 +0x3C7 (feb +0x3A7) ctor 1: seconds of movement AvoidObstacles looks ahead, and its period
    float                strafeDuration;            // v12 +0x3CB (feb +0x3AB) ctor 2
    float                strafeTimeLeft;            // v12 +0x3CF (feb +0x3AF) UpdateTimers: counts down while strafing, StopStrafe at 0
    float                noStrafeTime;              // v12 +0x3D3 (feb +0x3B3) UpdateTimers: time since last strafe
    m3dV                 strafeDir;                 // v12 +0x3D7 (feb +0x3B7) ctor m3dVUndef
    float                cycleTime;                 // v12 +0x3E3 1.2 NEW: how long aiFlags 4 (a move cycle) has lasted (UpdateTimers)
    m3dV                 dir;                       // v12 +0x3E7 (feb +0x3C3) facing direction (GetDir copy)
    float                enemyDist;                 // v12 +0x3F3 (feb +0x3CF) distance to current enemy (FillToPlayerCoords); used by every SelectGoal
    float                enemyHeight;               // v12 +0x3F7 (feb +0x3D3) // ? (FillToPlayerCoords)
    float                enemyAngle;                // v12 +0x3FB (feb +0x3D7) deg between facing and enemy (FillToPlayerCoords)
    m3dV                 goalDir;                   // v12 +0x3FF (feb +0x3DB) FillGoalDirection
    float                goalAngleSigned;           // v12 +0x40B (feb +0x3E7) deg, goalDir from the facing
    float                goalAngle;                 // v12 +0x40F (feb +0x3EB) its absolute value
    int                  crumbID;                   // v12 +0x413 (feb +0x3EF) breadcrumb index into aiENEMY trail (PostThink); -1 on INIT_LEVEL
    m3dV                 goalPos;                   // v12 +0x417 (feb +0x3F3) current movement goal (SelectGoal writes)
    m3dV                 wayPointPos;               // v12 +0x423 (feb +0x3FF) AI_PROP actorWayPoint position
    float                arriveTime;                // v12 +0x42F (feb +0x40B) how long it has been at the goal (AnalyzeMove)
    m3dV                 prevPos[3];                // v12 +0x433 (feb +0x40F) position history (ProcessINIT_LEVEL fills, CheckCycle detects oscillation)
    m3dV                 speed;                     // v12 +0x457 (feb +0x433) velocity set by ApplySpeed (vslot 0x174) / StartJump
    physMATH*            passivePhys;               // v12 +0x463 (feb +0x43F) physDRIFT_MATH created by aiNPC_WR::ProcessINIT (ragdoll-less fall/knockback)
    float                fallSpeed;                 // v12 +0x467 (feb +0x443) 
    float                inactivityTimer;           // v12 +0x46B (feb +0x447) DieOfInactivity: >20 s (30 s when the planner has a boss) -> self damage 5000 (msg 0x4C5 feb / 0x4C6 v12)
    u32                  aiFlags;                   // v12 +0x46F (feb +0x44B) aiNPC_FLAGS (verified v12 [esi+0x46F] everywhere) [v12: v12 0x0054F4B4]
    u32                  aiFlags2;                  // v12 +0x473 (feb +0x44F) aiNPC_FLAGS2
    cdtINFO              floor;                     // v12 +0x477 (feb +0x453) last floor contact (8 dwords, GetFloorInfo copies it)
    float                floorGap;                  // v12 +0x497 (feb +0x473) height over the floor
    float                fallDist;                  // v12 +0x49B (feb +0x477) height fallen so far
    int                  enemyCheckPeriodActive;    // v12 +0x49F (feb +0x47B) frames between DecideEnemy runs when active (ctor 6) [v12: v12 ctor [esi+0x49F]=6]
    int                  enemyCheckPeriodIdle;      // v12 +0x4A3 (feb +0x47F) frames between DecideEnemy runs when not activated (ctor 10) [v12: v12 ctor [esi+0x4A3]=10]
    int                  enemyCheckCounter;         // v12 +0x4A7 (feb +0x483) 
    int                  colorizeFrames;            // v12 +0x4AB (feb +0x487) ctor 5 (Colorize)  // ? [v12: v12 ctor [esi+0x4AB]=5]
    int                  colorizePhase;             // v12 +0x4AF (feb +0x48B) ctor random (m3dRandRange)  // ?
    sncCHANNEL*          sndChannel;                // v12 +0x4B3 (feb +0x48F) main 3D voice channel (gsSND_SYSTEM::AddChannel); PlaySounds plays sncSOUND_DESCR lists on it. NOTE: the sound-event bits are entENTITY v12 +0x108 (feb +0xF8), set by FillSoundFlags
    float                deadTimer;                 // v12 +0x4B7 (feb +0x493) time since death (UpdateDead); 1.2 StartDeath presets it for "instant" deaths
    float                corpseTime;                // v12 +0x4BB (feb +0x497) ctor 5.0: corpse stays this long before fading  // ? [v12: v12 ctor [esi+0x4BB]=5.0]
    float                corpseFadeTime;            // v12 +0x4BF (feb +0x49B) ctor 4.0  // ? [v12: v12 ctor [esi+0x4BF]=4.0]
    aiDOMAIN_SPAWN*      spawnDomain;               // v12 +0x4C3 (feb +0x49F) set when spawned by an ai spawn domain (aiINIT_DATA +8)
    int                  spawnParam;                // v12 +0x4C7 (feb +0x4A3) aiINIT_DATA +0xC
    float                radActivate;               // v12 +0x4CB (feb +0x4A7) AI_PROP radActivate; ctor 30. Deactivates when enemyDist > 2*radActivate (1.7x when not visible) [v12: v12 ctor [esi+0x4CB]=30]
    float                radActivateBack;           // v12 +0x4CF (feb +0x4AB) AI_PROP radActivateBack; ctor 20 [v12: v12 ctor [esi+0x4CF]=20]
    float                timeNotSeen;               // v12 +0x4D3 (feb +0x4AF) UpdateTimers: grows while the enemy is not visible
    float                timeDeactivate;            // v12 +0x4D7 (feb +0x4B3) ctor 40; deactivate when timeNotSeen > this (0.9x)  // ? [v12: v12 ctor [esi+0x4D7]=40]
    float                enemyChangeTimer;          // v12 +0x4DB (feb +0x4B7) ctor 15  // ? (ShouldChangeEnemy, boss) [v12: v12 ctor [esi+0x4DB]=15]
    aiENEMY              enemy;                     // v12 +0x4DF (feb +0x4BB) 12 bytes: {plrPLAYER* plr; float timer; entENTITY* ent}
    int                  goalSeq;                   // v12 +0x4EB (feb +0x4C7) ctor -1: the sequence a client plays (MP)
    int                  goalSeq2;                  // v12 +0x4EF 1.2 NEW, ctor 0: a client's once-through sequence (aiFlags 0x2000000)
    float                animRate2;                 // v12 +0x4F3 (feb +0x4CB) ctor 1.0 (Animate)  // ?
    m3dMATR              rootedMatr;                // v12 +0x4F7 (feb +0x4CF) ROOT's matrix (or the instance's) when the sequence moves the actor
    int                  seqFlags;                  // v12 +0x537 (feb +0x50F) the sequence's own flags: animSEQ +0x84
    float                unk513;                    // v12 +0x53B (feb +0x513) ctor 1.0 (GetMP_UPD_STATE / Animate)  // ?
    float                idleTimer;                 // v12 +0x53F (feb +0x517) AI_PROP timeIdleAfterSpawn; counts down (m3dUpdateTimeField)
    // end v12 +0x543 (feb +0x51B)

    // --- non-virtual API (v12 / feb) ------------------------------------------------------------
    static aiNPC *GetNext(aiNPC *prev, int aliveOnly);           // v12 0x00547930 (feb 0x00534100) iterate all NPCs (debug "kill all" uses it)
    static int    GetNOpp(char *className, char *name);          // v12 0x00547990 (feb 0x00534160) count NPCs of a class
    BOOL   IsDead();                                             // v12 0x00548D20 (feb 0x005353E0) aiFlags & AIF_DEAD
    BOOL   IsInAnyStrafe();                                      // v12 0x00548D10 (feb 0x005353D0)
    void   DecideEnemy();                                        // v12 0x0054C7B0 (feb 0x00531510) every enemyCheckPeriod frames: ShouldChangeEnemy -> SelectNewEnemy, IsPlrVisible
    BOOL   IsPlrVisible(m3dV *pos);                              // v12 0x0054C5E0 (feb 0x00531350) ray test eye->target
    void   FillToPlayerCoords();                                 // v12 0x0054EC30 (feb 0x00533780) enemyDist / enemyAngle
    void   DieOfInactivity();                                    // v12 0x0054C110 (feb 0x00530E90) >20 s (30 s with boss) inactive after activation -> 5000 dmg to self
    BOOL   CheckDeactivateCondition(int strict);                 // v12 0x0054F290 (feb 0x00533D00)
    void   GetPosRooted(m3dV *out);                              // v12 0x0054F630 (feb 0x00535400)
    void   SetPosRooted(m3dV *p);                                // v12 0x0054F6A0 (feb 0x00535470)
    void   AdvanceClientAnim(int seq, float dt);                 // v12 0x0054F900 NEW in 1.2 (name ours)
    int    IsSniperOnFloor();                                    // v12 0x0054E1E0 NEW in 1.2 (name ours)
    int    SelectBasicStrafeDirection();                         // v12 0x0054FD40 (feb 0x005358A0)
    void   StopStrafe();                                         // v12 0x0054FF80 (feb 0x00535AE0)
    int    CheckCycle();                                         // v12 0x00550050 (feb 0x00535BA0)
    void   UpdateUnderwater();                                   // (feb 0x00536270)
    void   AvoidObstacles();                                     // v12 0x00550950 (feb 0x005362C0)
    char  *GetLogableName();                                     // v12 0x00553410 (feb 0x00538C10)
    void   GetPrevPositions();                                   // v12 0x0054F0E0 (feb 0x00533B90)
    int    SelectGoal_P();                                       // v12 0x0054FB60 (feb 0x00535760) planner part of goal selection
    void   ProcessPhysics();                                     // v12 0x0054E820 (feb 0x005333E0) voluntary or passive physics
    int    ConstrainMotion(m3dV *from, m3dV *to, cdtINFO *ci);   // v12 0x0054E8A0 (feb 0x00533470)
    void   GetClampingSpeeds(float speed, float *forward, float *side); // v12 0x0054D8B0 (feb 0x005324F0)
    BOOL   IsPivotUsable();                                      // v12 0x0054ED90 (feb 0x00533910)
    void   NotifyAIState();                                      // feb 0x00531830 (v12 ?)
    // script helpers: read keys from the instance property sheet (section, key) -- they fall back to the spawn domain sheet
    BOOL   GetStr  (char *sect, char *key, char *buf, int n);    // feb 0x00538970  v12 ?
    BOOL   GetInt  (char *sect, char *key, int *v);              // feb 0x00538A20  v12 ?
    BOOL   GetFloat(char *sect, char *key, float *v);            // feb 0x00538AD0  v12 ?
    BOOL   GetBool (char *sect, char *key, int *v);              // feb 0x00538B70  v12 0x00553370 (seen called by aiTIGER::ProcessINIT v12 0x0045DB89)
};

// ------------------------------------------------------------------------------------------------
// aiNPC_WR -- "Will Rock" layer: blood, gibs, fall-back (knock-back), altar opponents, ban-cross areas.
//   no own vtable (abstract); size v12 0x567 (feb 0x53F)
// ------------------------------------------------------------------------------------------------
class aiNPC_WR : public aiNPC {
public:
    static int  dbgShow;                     // v12 0x0064C928 (name ours) draw the NPCs' debug lines and crosses
    static void __fastcall DbgToggleShow();  // v12 0x00454060 (name ours) dbgShow = !dbgShow (DBG_7)
    u32      wrFlags;                // v12 +0x543 (feb +0x51B) aiNPC_WR_FLAGS [v12: aiNPC_WR::ProcessCDT_FIREABLE 0x004533D9]
    float    shootFallbackTimer;     // v12 +0x547 (feb +0x51F) cleared in ProcessINIT  // ?
    float    unk523;                 // v12 +0x54B (feb +0x523) UpdateInAreaFlags  // ?
    objOBJ  *bloodObj;               // v12 +0x54F (feb +0x527) where blood spurts from (usually "ROOT") [v12: 0x004533E7]
    float    fallbackAngle;          // v12 +0x553 (feb +0x52B) ProcessINIT 90.0  // ?
    float    waterDepth;             // v12 +0x557 (feb +0x52F) ProcessINIT 2.0; CorrectGoalByWaterLevel  // ?
    u32      areaFlags[3];           // v12 +0x55B (feb +0x533) UpdateInAreaFlags / RestrictGoalByBanCross  // ?
    // end v12 +0x567 (feb +0x53F)

    // the overrides, then its slots [98..115] in v12 order
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00454780
    void ParseNameClass();                                       // v12 0x00452CE0
    void StartDeath(dmgDAMAGE *d);                               // v12 0x00453270
    void UpdateDead();                                           // v12 0x004532F0
    int  IsReadyActivate();                                      // v12 0x00453EE0
    int  CanPlanMove();                                          // v12 0x00453F00
    void Think();                                                // v12 0x004538B0
    void PostThink();                                            // v12 0x00453920
    void UpdateTimers();                                         // v12 0x00454030
    int  ProcessINIT(void *init);                                // v12 0x00452D70
    void ProcessTERM();                                          // v12 0x004530C0
    void ProcessINIT_LEVEL();                                    // v12 0x00453D90
    void ProcessFRAME();                                         // v12 0x00453F20
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                      // v12 0x00453370
    void ProcessMP_NOTIFY(gsEVENT *ev);                          // v12 0x00453590
    virtual int  StartPassivePhysics(dmgDAMAGE *d);              // [ 98] +0x188 v12 0x00453B80
    virtual void SetInitDeadSpeed(dmgWEAPON *w, m3dV *v);        // [ 99] +0x18C v12 0x004539B0
    virtual void SetInitShootFallBack(dmgWEAPON *w);             // [100] +0x190 v12 0x004537F0
    virtual int  IsAnimSeqGoodForFallback();                     // [101] +0x194
    virtual void AdjustSpeedsOnFallbackStart(dmgWEAPON *w, float *back, float *up); // [102] +0x198
    virtual void DecideExplosion(dmgDAMAGE *d);                  // [103] +0x19C v12 0x00453140
    virtual void Explode(dmgDAMAGE *d, int dead);                // [104] +0x1A0 v12 0x004530F0
    virtual int  InitSFX();                                      // [105] +0x1A4 v12 0x00453DC0
    virtual void SyncSFX(gsEVENT *ev);                           // [106] +0x1A8 v12 0x00453EB0
    virtual void TermSFX();                                      // [107] +0x1AC
    virtual int  DoesNPCAllowSFXAppear();                        // [108] +0x1B0
    virtual int  IsGibbable();                                   // [109] +0x1B4
    virtual int  IsObstAvoidForced();                            // [110] +0x1B8
    virtual void RestrictGoalByBanCross();                       // [111] +0x1BC v12 0x004543F0
    virtual void SelectGoalToRunAway(float dist);                // [112] +0x1C0 v12 0x004544C0
    virtual int  IsArmoredObject(char *name);                    // [113] +0x1C4
    virtual int  Slot114();                                      // [114] +0x1C8
    virtual void ProcessAltarOppUnhide();                        // [115] +0x1CC v12 0x004546E0
    // non-virtual (v12 / feb)
    void StartBlood(int kind, int dead, m3dV *at, m3dV*, m3dV *vel, m3dV *dir); // v12 0x00453650 (feb 0x0044B070) honours wrAppState 2 (no blood)
    void StartChunkMeat(int a, int b, m3dV *p, m3dV *d);         // v12 0x00453780 (feb 0x0044B1B0)
    void OrientTowardsPlayer();                                  // feb 0x0044B960 (v12 ?)
    void CorrectGoalByWaterLevel();                              // v12 0x00453FB0 (feb 0x0044B9D0)
    int  SelectEscapeWhenOut();                                  // v12 0x00454370 (feb 0x0044BCE0)
    void UpdateInAreaFlags();                                    // v12 0x00454510 (feb 0x0044BE80)
    // overrides (v12 / feb): ProcessMsg 0x00454780/0x0044C0F0, ProcessINIT 0x00452D70/0x0044A7C0, ParseNameClass 0x00452CE0/0x0044A720,
    //   StartDeath 0x00453270/0x0044AC90, ProcessCDT_FIREABLE 0x00453370/0x0044AD00, ProcessMP_NOTIFY 0x00432380/0x0044AF20,
    //   Think 0x004538B0/0x0044B2C0, PostThink 0x00453920/0x0044B330, IsReadyActivate 0x00453EE0/0x0044B920,
    //   ProcessFRAME 0x00453F20/0x0044B940, UpdateTimers 0x00454030/0x0044BA50, MoveToGoal 0x00454080/0x0044BA80,
    //   ProcessINIT_LEVEL 0x00453D90/0x0044B7A0, CanPlanMove (1.2 only) 0x00453F00
};

// aiNPC_WR_FLY -- flying enemies (bird, amur, croc, medusa). size v12 0x56F (feb 0x547)
class aiNPC_WR_FLY : public aiNPC_WR {
public:
    float    flyHeight;              // v12 +0x567 (feb +0x53F) ProcessINIT -1.5: the upward speed of a shot-down flyer's fall
    float    floorTimer;             // v12 +0x56B (feb +0x543) the floor under it looked up every 0.5 s
    virtual int  ProcessINIT(void *init);                   // +0x140 v12 0x00454AF0 (feb 0x0044C460)
    virtual void ProcessVoluntaryPhysics();                 // +0x0B8 v12 0x00454B30 (feb 0x0044C4A0)
    virtual void SettleSurface();                           // +0x06C v12 0x00454D80 (feb 0x0044C6F0)
    virtual void StartDeath(dmgDAMAGE *d);                  // +0x088 v12 0x00454E10 (feb 0x0044C770)
    virtual int  StartPassivePhysics(dmgDAMAGE *d);         // +0x188 v12 0x00454E70 (feb 0x0044C7D0)
    // ProcessINIT v12 0x00454AF0 (feb 0x0044C460), ProcessVoluntaryPhysics v12 0x00454B30 (feb 0x0044C4A0),
    // SettleSurface v12 0x00454D80 (feb 0x0044C6F0), StartDeath v12 0x00454E10 (feb 0x0044C770),
    // StartPassivePhysics v12 0x00454E70 (feb 0x0044C7D0), ProcessFRAME v12 0x00448F00 (feb 0x00441110)
};

#pragma pack(pop)

// aiCDT_REFINE: the floor and walls an NPC collides with (src/gs_shared/ai_ctrl_geo.cpp); vtable v12 0x005FF008
class aiCDT_REFINE : public entCDT_REFINE_EXCL {
public:
    aiCDT_REFINE() : entCDT_REFINE_EXCL(0, 0, 0) {}
    BOOL32 IsInclInst(animINST *i);                              // v12 0x0054EFA0 (feb 0x00533AB0)
    BOOL32 IsInclObj(objOBJ *o);                                 // v12 0x0054F070 (feb 0x00533B20)
};
// aiDMG_DISPATCHER_TO_SIBLING: a blast among NPCs of one class goes round late (src/gs_shared/ai_ctrl_die.cpp)
struct aiDMG_DISPATCHER_TO_SIBLING : dmgDISPATCHER {             // vtable v12 0x005FDC6C
    float amount;                                                // +0x04
    void Dispatch(entENTITY *target, dmgDAMAGE *d, entENTITY *attacker); // v12 0x0054C040 (feb 0x00530DD0)
};

// ------------------------------------------------------------------------------------------------
// Globals
// ------------------------------------------------------------------------------------------------
extern aiPLANNER *aiAttackPlanner;      // v12 0x009A8C78 (feb 0x0095D938) [v12: 0x0054F47A]
extern class aiDOMAIN_LIST_SPAWN  *aiSpawnDomList;  // v12 0x009A89E8 (feb 0x0095D96C)
extern class aiaDOMAIN_LIST_ARENA *aiaArenaDomList; // v12 0x009A8C90 (feb 0x0095D94C)
extern int   aiDbgThinking;             // v12 0x009A89A8 (feb 0x0095D910) 1 while an NPC runs its Update* (ProcessFRAME)
extern aiNPC *aiDbgSelectedNPC;         // v12 0x009A89BC (feb 0x0095D924) the only NPC that thinks when [Debug] Decomp_OFF_AI=1
extern int   aiDbgAllOff;               // v12 0x009A89C8 (feb 0x0095D928) OR'ed with [Debug] Decomp_OFF_AI
// [Debug] Decomp_OFF_AI is read with _apCfgReadBool every frame by aiNPC::ProcessFRAME (v12 0x00548802).

u8    __fastcall m3dFloatToByte(float v, float lo, float hi);   // v12 0x00572440 (src/gs_shared/ai_ctrl_anim.cpp)
float __fastcall m3dByteToFloat(u8 b, float lo, float hi);      // v12 0x00572540
// module init/term (called from wrAPP_SYSTEM::InitSubsys)
int __fastcall  aiInit();                          // feb 0x005340B0 (v12 ?)
void __fastcall aiTerm();                          // v12 0x00547920 (feb 0x005340F0)
BOOL __fastcall aiIsNPCDead(entENTITY *e, int *isNPC); // v12 0x0054BCD0 (feb 0x00530AB0)
