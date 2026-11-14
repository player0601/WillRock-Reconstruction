// player.h -- Will Rock 1.2 unofficial SDK, subsystem "player"
//
// objs covered (feb map): player.obj (plrPLAYER, _plrMOVE_CTRL, _plrFPS_FILTER), player_comm.obj
//   (plrPLAYER_COMMON, plrSYNC_DATA), player_mp.obj / player_mp_skin.obj (plrPLAYER_MP), plr_height.obj
//   (plrHEIGHT_CTRL, plrCAM_CTRL), plr_sfx.obj (plrWPN_CHROME, plrBLOOD_HANDS), plr_anim.obj.
//
// Assumed from re/sdk/core (and re/sdk/engine_core/entity.h): msgADDR, msgDATA, msgSYSTEM, apNAME,
//   objOBJ, animINST, entIACTIVE_OBJ, entENTITY (v12 size 0x16B, feb 0x14F), m3dV {float x,y,z},
//   m3dMATR, cdtINFO, camCAMERA, sncCHANNEL, dmgDAMAGE, gsEVENT, gsMSG_MP_* (re/mpsrc/mp_messages.h),
//   pwpWEAPON (game_shared/weapon_base.h).
//
// Offsets: "// v12 +0xNN (feb +0xNN)". Every v12 offset marked without "?" was matched by aligning the feb
// function with its v12 twin instruction by instruction (re/work/sdk_player/al.py, votes in votes.txt)
// or read directly from a v12 instruction (address given). Classes are #pragma pack(1): odd offsets.
//
// !!! LAYOUT WARNING !!!  The player block is NOT "feb + constant" in 1.2:
//   - plgsACTION shrank from 0xEC own bytes (feb 0x14F..0x23B) to 0x40 (v12 0x16B..0x1AB).
//   - plrPLAYER_COMMON first part: feb 0x23B..0x25B -> v12 0x1AB..0x1CB  (-0x90)
//                                  feb 0x287..0x297 -> v12 0x1D3..0x1E3  (-0xB4)
//                                  feb 0x29B..0x2A3 -> v12 0x1F3..0x1FB  (-0xA8)
//                                  feb 0x2A7..0x2DF -> v12 0x203..0x23B  (-0xA4)
//                                  feb 0x2E3..0x307 -> v12 0x263..0x287  (-0x80)
//   - feb plrPLAYER own 0x30B..0x327 (flags2, floor point) moved UP into the common block: v12 0x23F..0x25B
//     (the 1.2 constructors of BOTH plrPLAYER 0x0041D4D0 and plrPLAYER_MP 0x0042A950 init them).
//   - feb plrPLAYER_COMMON 0x25F..0x277 moved DOWN into plrPLAYER: v12 0x2E3..0x2FB (+0x84).
//   - feb plrPLAYER 0x32B..0x3CB -> v12 0x28B..0x353 (-0xA0 then -0x9C from 0x353, -0x78 from 0x37F).
//   sizeof(plrPLAYER) v12 0x357 (feb 0x3CF), sizeof(plrPLAYER_MP) v12 0x293 (feb 0x313).

#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include "../engine_core/entity.h"
#pragma pack(push, 1)

class plrPLAYER; class plrPLAYER_MP; class plrCAM_CTRL; class plrHEIGHT_CTRL; struct plrSYNC_DATA;
struct plrEV_START_DEATH; struct gssMSG_PICK; struct plrPLAYER_TRALTAR_INFO;

// ------------------------------------------------------------------------------------------------ enums
// Input action ids (gsINP_SYSTEM::IsOn/IsToggled index). Same numbers in feb and 1.2 up to 43.
enum plrACTION {
    PLR_ACT_FORWARD = 1, PLR_ACT_BACK = 2,          // results/player.json C_ACT_*
    PLR_ACT_LEFT = 9, PLR_ACT_RIGHT = 10,
    PLR_ACT_SWITCH_MODE = 12,                        // fly-through/noclip, gated by gsAppState&0x8000 (build_differences.md 5)
    PLR_ACT_JUMP = 14, PLR_ACT_CROUCH = 15, PLR_ACT_USE = 16, PLR_ACT_ZOOM = 18,
    PLR_ACT_POWERUP0 = 32, PLR_ACT_POWERUP1 = 33, PLR_ACT_POWERUP2 = 34,
    PLR_ACT_RELOAD = 35, PLR_ACT_FIRE = 36,
    PLR_ACT_TOGGLE_TEAM = 42, PLR_ACT_SCREENSHOT = 43,
    PLR_ACT_CHAT_ALL = 44, PLR_ACT_CHAT_TEAM = 45,   // retail only; in feb 44..52 are DBG_1..DBG_9
    PLR_ACT_DBG_1 = 46, PLR_ACT_DBG_9 = 54,          // v12 only has handlers (ProcessInput block 0x00420277..0x0042089C)
};

// plrPLAYER_COMMON::moveFlags (v12 +0x1F3, feb +0x29B)
enum plrMOVE_FLAGS {
    PLR_MF_FLYTHROUGH   = 0x00000001,   // ControlFlyThrough instead of ControlFirstPerson (SWITCH_MODE)
    PLR_MF_DEAD_OR_2    = 0x00000002,   // ? cleared in SetDefaultValues / INIT_LEVEL
    PLR_MF_GOD          = 0x00000004,   // IsIgnoreDmg returns 1 (v12 DBG_3: xor [esi+0x1F3],4 at 0x004202B8)
    PLR_MF_NO_GRAVITY   = 0x00000400,   // ProcessPhysics: gravity 0.0
    PLR_MF_WATER        = 0x00000800,   // ProcessPhysics: gravity 2.0 (PHYS_GRAVITY_WATER)
    PLR_MF_KEY          = 0x00001000,   // holds a key: HasKey (v12 0x00425680), UseKey clears it
    PLR_MF_JUMP_UP      = 0x00040000,   // 1.0/1.2 only: ApplySpeed(set) ORs it when vy > 5 (player.json OFS_PLR_VEL)
    PLR_MF_LEVEL_START  = 0x08000000,   // set by ProcessINIT_LEVEL
};

enum plrPOWERUP_ID { PLR_PWR_QUAD_DAMAGE = 0, PLR_PWR_UNHOLY_ARMOR = 1, PLR_PWR_TIMESCALE = 2, PLR_PWR_COUNT = 3 };
enum plrPOWERUP_STATE { PLR_PWR_NONE = 0, PLR_PWR_HELD = 1, PLR_PWR_ACTIVE = 2 };

// weapon slot index into plrPLAYER_COMMON::weapons[] (11 entries; entity class names from
// plrPLAYER_COMMON::ProcessINIT feb 0x00423550 / v12 0x00427160)
enum plrWEAPON_SLOT {
    PLR_WPN_SVL = 0, /* "pwp_svl" */  PLR_WPN_CLT = 1, /* colt, start 14 ammo */ PLR_WPN_STG = 2,
    PLR_WPN_MCG = 3, PLR_WPN_CRB = 4, PLR_WPN_ACD = 5, PLR_WPN_BSK = 6, PLR_WPN_MDG = 7,
    PLR_WPN_MNG = 8, PLR_WPN_BMB = 9, PLR_WPN_AMG = 10, PLR_WPN_COUNT = 11
};

#define PLR_MSG_NOTIFY        0x40E      // v12 (feb 0x40C, jun 0x40E): HUD/state notify sent by Pick*/ActivatePowerup
#define PLR_POWERUP_TIME      30.0f      // ActivatePowerup
#define PLR_POWERUP_SLOWMO    0.25f      // gsMsgSetElapsedTimeScale for powerup 2
#define PLR_HEALTH_DEFAULT    100.0f     // SetDefaultValues v12 0x004277B9: mov [esi+0xD0],100.0
#define PLR_HEALTH_ARMOR_CAP  100.0f     // small pickup cap   (float const v12 0x005FB220, shared! patch the mov imm)
#define PLR_HEALTH_ARMOR_MEGA 200.0f     // big pickup (>=150) cap (float const v12 0x005FB64C, shared with friction 200)

// ------------------------------------------------------------------------------------------------ small types
struct plrPOWERUP {                       // 8 bytes, x3
    int   state;                          // +0 plrPOWERUP_STATE
    float timeLeft;                       // +4 seconds, counts down in real time (dt / timeScale)
};

struct _plrMOVE_CTRL {                    // movement axis smoother; globals _plrCtrlForw / _plrCtrlSide (8 bytes)
    float value;                          // +0 -1..1 (beyond 1 while airborne: kept by the in-air rules)
    float rate;                           // +4 6.0 per second (static init v12 0x0041D020)
    void  Update(int dir);                // v12 0x0041D1A0 (feb 0x0041A720) dir -1/0/1
    float GetValue();                     // v12 0x0041D320 (feb 0x0041A8A0) smoothstep-shaped value (-1.5..1.5)
};

struct _plrFPS_FILTER {                   // 0x24: 4-tap moving average (weights 0.25), used for the mouse look
    int   reset;                          // +0x00 1 = fill the history with the next sample
    float w[4];                           // +0x04 weights (static init v12 0x0041D050.. : 0.25 each)
    float h[4];                           // +0x14 last four samples, oldest first
    float Update(float x);                // v12 0x0041D130 (feb 0x0041A680)
};

struct plrPLAYER_COMMON_CFG {             // one dword bitfield, ctor v12 0x004265C0 (feb 0x00422A90): &= ~6; |= 1
    unsigned bits;
};

// The death (GS_MSG_MP_NOTIFY 0x40E type -98, 0x13 bytes): sent by the server's NotifyHealthChange to every client,
// the local one too, whose ProcessMP_NOTIFY starts it (slot +0x114 StartDeath)
struct plrEV_START_DEATH : msgDATA {
    short type;                           // +0x04 -98
    int   source;                         // +0x06 the MP id of the damage's source (the killer)
    u8    sub;                            // +0x0A the damage's subtype: 4 / 5 / other pick the death cry
    int   body;                           // +0x0B a weapon hit (type 1): the weapon (the kill message's), else -1
    int   world3;                         // +0x0F the world's damage of subtype 3: torn to pieces
};

// ------------------------------------------------------------------------------------------------ plgsACTION
// Game-side base of anything the AI treats as "the player" (bread crumbs for AI path following).
// v12 own bytes 0x16B..0x1AB (0x40), feb 0x14F..0x23B (0xEC): heavily shrunk in retail. Ctor v12 0x00546DE0
// (feb 0x0052F1C0). Members not mapped: treat as opaque.
class plgsACTION : public entENTITY {
public:
    char _plgs[0x1AB - 0x16B];            // v12 +0x16B..+0x1AB (feb +0x14F..+0x23B) ?
    float GetGravity();                   // slot +0xA8 v12 0x00425E20 (feb 0x00422710) 30.0
    BOOL32 IsActive();                    // slot +0x78 v12 0x00546E40 not in a cut-scene that holds the camera
    void ProcessFRAME();                  // v12 0x00546E70 (the twin table names it ProcessINIT_LEVEL) the attack planner told
    void ProcessMP_NOTIFY(void *d);       // v12 0x00547430 the restored player (-100) to the attack planner
};
// The player's sounds (v12 0x00641C38, 41 of 0x68; sncLinkSndListData by plrCommInit): see player_comm.cpp
extern sncSOUND_DESCR_3D plrSnd[41];
extern sncSOUND_DESCR_3D *plrSndList;     // v12 0x0062B864 = plrSnd (the code reaches the table through it)
int __fastcall plrCommInit();             // v12 0x004265A0 (feb 0x00422A70)
extern plgsACTION* plgsPlayer;            // v12 0x009A84E0 (feb 0x0095D630) = the local plrPLAYER

// ------------------------------------------------------------------------------------------------ plrPLAYER_COMMON
// Shared by the local player (plrPLAYER, "player") and the network/3rd-person body (plrPLAYER_MP, "player_mp").
class plrPLAYER_COMMON : public plgsACTION {
public:
    int          skinNo;                  // v12 +0x1AB (feb +0x23B) init -1; the outfit (plrPLAYER_MP::arraySkin, 0..5)
    pwpWEAPON**  weapons;                 // v12 +0x1AF (feb +0x23F) apCalloc(4,11): one pwp_* entity per plrWEAPON_SLOT
    int          curWeapon;               // v12 +0x1B3 (feb +0x243)
    int          nextWeapon;              // v12 +0x1B7 (feb +0x247)
    float        pitch;                   // v12 +0x1BB (feb +0x24B) view pitch in degrees, 90 = level; clamp 5..175
    float        unk_1BF[4];              // v12 +0x1BF..+0x1CB (feb +0x24F..+0x25B) zeroed in ProcessINIT ?
    char         _1CF[4];                 // v12 +0x1CF ?
    float        aliveTime;               // v12 +0x1D3 (feb +0x287) seconds since the respawn (ProcessFRAME; the MP spawn protection)
    sncCHANNEL*  sndChannel[3];           // v12 +0x1D7 (feb +0x28B) 3 sncCHANNEL_CONE voices (ProcessINIT)
    int          unk_1E3;                 // v12 +0x1E3 (feb +0x297) ?
    m3dV         spawnPos;                // v12 +0x1E7 where the respawn put the player (ProcessMP_NOTIFY type 3; 0x415 answers it)
    unsigned     moveFlags;               // v12 +0x1F3 (feb +0x29B) plrMOVE_FLAGS   (139 aligned uses)
    unsigned     unk_1F7;                 // v12 +0x1F7 (feb +0x29F) ?
    unsigned     flags2;                  // v12 +0x1FB (feb +0x2A3) bit 4: keep weapons on SetDefaultValues (v12 0x00427779, new) ?
    char         _1FF[4];                 // v12 +0x1FF (no feb twin: 4 bytes inserted in retail) ?
    float        armor;                   // v12 +0x203 (feb +0x2A7)
    float        money;                   // v12 +0x207 (feb +0x2AB) gold; buys powerups at the trading altar
    char         _20B[4];
    int          team;                    // v12 +0x20F (feb +0x2B3) init -1; TH team
    int          frags;                   // v12 +0x213 (feb +0x2B7) DM frags (re/mpsrc/wr_go_mp.cpp)
    int          unk_217;                 // v12 +0x217 (feb +0x2BB) the deaths (StartDeath counts them; sent with frags and treasures)
    int          treasures;               // v12 +0x21B (feb +0x2BF) TH treasures carried (re/mpsrc)
    int          unk_21F;                 // v12 +0x21F (feb +0x2C3) init -1 ?
    plrPOWERUP   powerup[3];              // v12 +0x223 (feb +0x2C7) plrPOWERUP_ID
    plrPLAYER_COMMON_CFG cfg;             // v12 +0x23B (feb +0x2DF)
    // --- 1.2: block moved here from feb plrPLAYER (+0x30B..+0x327) ---
    union {
    cdtINFO      floorInfo;               // v12 +0x23F (feb plrPLAYER +0x30B) FloorPlace result of ProcessPhysics (verified v12 0x00420C34)
    struct {
    unsigned     flags3;                  // v12 +0x23F = floorInfo.flags (bit 2: standing on something)
    animINST    *floorInst;               // v12 +0x243 (feb +0x30F) = floorInfo.inst (moving platforms)
    int          unk_247;                 // v12 +0x247 (feb +0x313)
    int          unk_24B;                 // v12 +0x24B (feb +0x317)
    m3dV         floorPoint;              // v12 +0x24F (feb +0x31B) .y = floor Y at v12 +0x253 (feb +0x31F)
    int          unk_25B;                 // v12 +0x25B (feb +0x327) = floorInfo.dist
    };
    };
    float        bodyTwist;               // v12 +0x25F the body: how far the view is turned from the legs
                                          //   (plrPLAYER::Animate sets it, AnimateProcedural turns the back)
    // sizeof(plrPLAYER_COMMON) = 0x263 in v12: plrPLAYER_MP's own fields start there (its ctor v12
    // 0x0042A950, ProcessINIT 0x0042AA70); the movement block that follows in plrPLAYER is the local player's.

    // ---- non-virtual members (v12 / feb) ----
    int  IsIgnoreDmg(dmgDAMAGE* d);       // feb 0x00424270, v12 ?  1 when god flag / team rules
    int  IsActivePowerup(int id);         // v12 0x00429FA0 (feb 0x00425CC0)
    BOOL32 IsDead();                      // vtable +0x74: v12 0x00425E50 (feb 0x00422720) moveFlags & 2
    int  GetMountType(int);               // v12 0x00429F00 (feb 0x00425C20) the body's animation group of a weapon
    BOOL32 ProcessINIT();                 // slot +0x7C v12 0x00427160 (feb 0x00423550) voices, weapons, sync block
    BOOL32 SetupAniSystem();              // slot +0x40 v12 0x00426710 (feb 0x00422BB0) the body's sequences
    void   ProcessTERM();                 // slot +0xAC v12 0x004274D0 (feb 0x004238B0)
    BOOL32 ProcessINIT_LEVEL();           // slot +0x80 v12 0x00427540 (feb 0x00423920)
    BOOL32 ProcessINIT_MAP();             // slot +0xB4 v12 0x004275D0 (feb 0x00423940)
    BOOL32 ProcessINIT_GAME();            // slot +0xBC v12 0x00427690 (feb 0x004239E0)
    void   ProcessTERM_LEVEL();           // slot +0xB0 v12 0x00427590 the voices stopped
    void ReleaseTreasures(int scatter, int n); // v12 0x0042A130 (feb 0x00425DF0) TH: drop n carried treasures (-1 all)
    void SetCurWpn(int w, int animate);   // slot +0x108 v12 0x00428DC0 (feb 0x00424C00) the body's animation group
    void ProcessFRAME();                  // slot +0x84 v12 0x00427AA0 (feb 0x00423CB0)
    void UpdateSound();                   // slot +0x120 v12 0x0042A270 the frame's sounds (unk108 bits) on the 3 voices
    void PlayStepSound(int base);         // v12 0x0042D380 a step / landing sound by what is underfoot (name ours)
    BOOL32 ProcessPICK(void *d);          // slot +0xC4 v12 0x00428450 (feb 0x004243B0) an item {kind +4, amount +8} taken
    BOOL32 PickWeapon(int w, float n);    // v12 0x00428770 (feb ?) the shared part of the Pick* slots (names by the slot)
    BOOL32 PickAmmo(int w, float n);      // v12 0x00428840 (feb 0x00424760)
    BOOL32 PickHealth(float n);           // v12 0x00428900 (feb 0x00424810)
    BOOL32 PickArmor(float n);            // v12 0x00428A30 (feb 0x00424920)
    BOOL32 PickKey();                     // v12 0x00428B40
    BOOL32 PickTreasure(float n);         // v12 0x00428B70 (feb 0x00424A20)
    BOOL32 PickMoney(float n);            // v12 0x00428C70 (feb 0x00424AF0)
    BOOL32 PickPowerup(int i);            // v12 0x00428CF0 (feb 0x00424B60)
    void Revitalize(int full);            // v12 0x00428E50 (feb 0x00424C80) the shared part of a respawn
    void SetTeam(int t);                  // v12 0x0042A050 the TH team: kept, sent to the clients, the HUD told -- not rebuilt
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // slot +0x04 v12 0x00426E60 (feb 0x00423400)
    void ProcessSHOOT(void *msg);         // slot +0xCC v12 0x00428150: msg 0x4C7 to the weapon (byte +4 its index)
    void ProcessSHOOT_BODY(void *msg);    // slot +0xD0 v12 0x004281C0: msg 0x4C8 to the weapon
    int  IsCanBuyPowerup(int id);         // v12 0x00429F70 enough gold and not held already
    void ProcessCDT_FIREABLE(dmgDAMAGE *d);  // slot +0xC8 v12 0x00428200 (feb 0x00424190) damage taken: armor, health
    void NotifyHealthChange(dmgDAMAGE *d);   // slot +0x110 v12 0x00429040 (feb 0x00424DD0) stats, MP notify, death
    void ProcessMP_NOTIFY(void *d);       // slot +0x08C v12 0x00427B80 (feb 0x00423D90) the shared notify types
    void StartDeath(plrEV_START_DEATH *m);   // v12 0x004291C0 (feb 0x00424F60) the shared part of a death
    void UpdateKillsCount(int n);         // slot +0x118 v12 0x00429FC0 (feb 0x00425CE0) frags += n, sent
    const wrWCHAR* WpnType2String(int); // v12 0x0042A770 (feb 0x00426250)
    static plrSYNC_DATA* GetPlrSyncDataStorage(); // v12 0x004265D0 (feb 0x00422AA0) static save/sync buffer
    // Non-overridden COMMON virtual bodies (called by the plrPLAYER / MP overrides):
    //   ProcessINIT          v12 0x00427160 (feb 0x00423550) creates sound voices + the 11 weapon entities
    //   ProcessTERM          v12 0x004274D0 (feb 0x004238B0)
    //   ProcessINIT_LEVEL    v12 0x00427540 (feb 0x00423920)
    //   ProcessINIT_MAP      v12 0x004275D0 (feb 0x00423940)
    //   ProcessINIT_GAME     v12 0x00427690 (feb 0x004239E0)
    //   SetDefaultValues     v12 0x00427760 (feb 0x00423AA0) health 100, armor 0, money 0, starting weapons
    void SetDefaultValues();              // v12 0x00427760 (feb 0x00423AA0)
    BOOL32 Pick(entENTITY *item);         // slot +0x09C v12 0x00428100 (feb 0x00424060) an item's {kind, amount} to ProcessPICK
    //   ProcessFRAME         v12 0x00427AA0 (feb 0x00423CB0)
    //   ProcessCDT_FIREABLE  v12 0x00428200 ? (feb 0x00424190) damage in (alignment 0.41)
    //   ProcessPICK          v12 0x00428450 (feb 0x004243B0) item pickup dispatcher -> Pick*
    //   PickAmmo             v12 0x00428840 (feb 0x00424760)
    //   PickHealth           v12 0x00428900 (feb 0x00424810) cap 100 (<11 or >=150 -> cap 200)
    //   PickArmor            v12 0x00428A30 (feb 0x00424920) cap 100 (>=150 -> 200)
    //   PickTreasure         v12 0x00428B70 (feb 0x00424A20)
    //   PickMoney            v12 0x00428C70 (feb 0x00424AF0) money += v; notify 0x40E
    //   PickPowerup          v12 0x00428CF0 (feb 0x00424B60) SP: costs _plrPwrCostTbl[i] gold; DM/TH: free
    //   SetCurWpn            v12 0x00428DC0 (feb 0x00424C00)
    //   Revitalize           v12 0x00428E50 ? (feb 0x00424C80) (call-site twin, low alignment)
    //   NotifyHealthChange   v12 0x00429040 (feb 0x00424DD0)
    //   GetMPSysSyncData     v12 0x00429480 (feb 0x004251B0)
    //   ProcessMP_GET_SYNC_DATA   v12 0x004295A0 (feb 0x004252D0)
    //   ProcessMP_WRITE_SYNC_DATA v12 0x004297D0 (feb 0x004254F0)  savegame/MP write
    //   ProcessMP_READ_SYNC_DATA  v12 0x00429B80 (feb 0x004258A0)  savegame/MP read
    //   SetupAniSystem       v12 0x00426710 (feb 0x00422BB0)
};

// ------------------------------------------------------------------------------------------------ plrPLAYER
// The local first-person player. Entity class "player" ('PLYR'), created by plrPLAYER::Create
// v12 0x0041D4D0 (feb 0x0041AA50): operator new(0x357), sets plrPlayer and plgsPlayer.
class plrPLAYER : public plrPLAYER_COMMON {
public:
    // --- movement parameters ---
    int          unk_263;                 // v12 +0x263 (feb +0x2E3) set to 11 by INIT_LEVEL (v12 0x0041D889) ?
    float        maxSpeed;                // v12 +0x267 (feb +0x2E7) ProcessInput stores 5.0 walk / 10.0 run every frame
    float        accel;                   // v12 +0x26B (feb +0x2EB) 30.0 walk / 90.0 run
    float        eyeHeight;               // v12 +0x26F (feb +0x2EF) 1.6 (v12 0x0041D62C) ?name
    float        stepHeight;              // v12 +0x273 (feb +0x2F3) 0.9 (v12 0x0041D638): the floor ray starts this far up (stairs)
    m3dV         vel;                     // v12 +0x277 (feb +0x2F7) player-controlled velocity (ApplySpeed)
    float        gravity;                 // v12 +0x283 (feb +0x303) 30.0 / 2.0 water / 0 no-grav
    float        radius;                  // v12 +0x287 (feb +0x307) 0.8 (v12 0x0041D64C): collision sphere radius (ProcessPhysics)
    float GetUnderwaterTimeLeft();        // v12 0x00422310 (feb 0x0041EEA0) air left 0..1 (25 s of breath)
    float        floorHeight;             // v12 +0x28B (feb +0x32B) height above floor (jump test < 0.2)
    float        fallDist;                // v12 +0x28F (feb +0x32F) height lost since the last landing (falling damage)
    float        unk_293;                 // v12 +0x293 (feb +0x333) 5.0 (INIT_MAP)
    float        timer_297;               // v12 +0x297 (feb +0x337) += dt (UpdateTimers)
    float        timer_29B;               // v12 +0x29B (feb +0x33B) += dt
    float        realTimer;               // v12 +0x29F (feb +0x33F) += dt/timeScale
    float        unk_2A3;                 // v12 +0x2A3 (feb +0x343) 0.2
    float        flashTimer;              // v12 +0x2A7 (feb +0x347) -1 = off; += dt until >= flashPeriod ?
    float        flashPeriod;             // v12 +0x2AB (feb +0x34B) 0.8 ?
    float        pushTime;                // v12 +0x2AF (feb +0x34F) time the push-out (ConstrSphere) keeps touching
    float        stillTime;               // v12 +0x2B3 (1.2 only) time with |vel.y| < 0.1 (landing test); zeroed in INIT_LEVEL 0x0041D8AF
    m3dV         ctrlVel;                 // v12 +0x2B7 (feb +0x353) smoothed control velocity (ControlFirstPerson)
    float        airTime;                 // v12 +0x2C3 (feb +0x35F) time above the floor (> 0.5: in-air move control)
    float        breathTime;              // v12 +0x2C7 (feb +0x363) seconds under water (26: drowning)
    int          bloodHands[2];           // v12 +0x2CB/+0x2CF (feb +0x367/+0x36B) plrBLOOD_HANDS / chrome ? (NotifySFXBloodHands)
    plrHEIGHT_CTRL* heightCtrl;           // v12 +0x2D3 (feb +0x36F) new(0x20) in ProcessINIT
    plrCAM_CTRL* camCtrl;                 // v12 +0x2D7 (feb +0x373) new(0x4C) in ProcessINIT
    plrPLAYER_MP* body;                   // v12 +0x2DB (feb +0x377) "player_mp" entity used as the visible body/shadow
    float        bodyScale;               // v12 +0x2DF 0.073: the body's scale (SetupGhostRendState)
    m3dV         bodyDir;                 // v12 +0x2E3 (feb COMMON +0x25F) lower body direction, lags the view (ControlFirstPerson)
    m3dV         placePos;                // v12 +0x2EF (feb COMMON +0x26B) where SetPosDir put the player (name ours)
    m3dV         placePos2;               // v12 +0x2FB (feb COMMON +0x277) the same (SetPosDir) // ?
    int          unk_307;                 // v12 +0x307 (feb +0x37F) -1 (INIT_MAP 0x0041DA17)
    animINST    *platInst;                // v12 +0x30B (feb +0x383) moving platform stood on last frame (ProcessPhysics)
    objOBJ      *platObj;                 // v12 +0x30F (feb +0x387) its object
    m3dV         platLocalPos;            // v12 +0x313 (feb +0x38B) position in the platform object's space
    m3dV         platLastPos;             // v12 +0x31F (feb +0x397) world position after last frame
    m3dV         platLocalDir;            // v12 +0x32B (feb +0x3A3) view direction in the platform's space
    m3dV         platLastDir;             // v12 +0x337 (feb +0x3AF)
    m3dV         platVel;                 // v12 +0x343 (feb +0x3BB) platform velocity, kept as momentum when stepping off; zeroed by INIT_LEVEL
    entENTITY*   tradeCamera;             // v12 +0x34F (feb +0x3C7) "sob_tra_camera" (trading altar camera)
    int          flyReenter;              // v12 +0x353 (feb +0x3CB) never set (dead code in SWITCH_MODE)
    // sizeof 0x357 (feb 0x3CF)

    // ---- non-virtual members ----
    void ProcessFRAME();                  // slot +0x84 v12 0x0041E0C0 (feb 0x0041B550) the player's frame
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // slot +0x04 v12 0x0041DC30 (feb 0x0041B0C0)
    void ProcessInput();                  // v12 0x0041F790 (feb 0x0041C960) actions -> speeds, weapons, powerups, use, zoom, DBG keys (1.2)
    void ProcessPhysics(m3dV* prev);      // v12 0x004208B0 (feb 0x0041D5D0) gravity, friction, floor, water; prev = position before the frame
    void ProcessInputMove();              // the movement part of ProcessInput (v12 0x0041F790..0x0041FA9E), name ours
    void GetPos(m3dV *out);               // vtable +0x64: v12 0x0041DE70 (feb: folded into plgsACTION::GetPos 0x0052F2B0) inst->GetPos
    void SetPos(m3dV *p);                 // vtable +0x68: v12 0x0041DE90 (feb 0x0041B1B0)
    void SetPosDir(m3dV *pos, m3dV *dir, int onFloor); // vtable +0x70: v12 0x0041DED0 (feb 0x0041B1F0)
    void GetDir(m3dV *out);               // vtable +0x6C: v12 0x0041DE80 (feb: plgsACTION::GetDir 0x0052F2C0) inst->GetDir
    void ApplySpeed(m3dV *v, int set);    // vtable +0xA4: v12 0x00423780 (feb 0x00420340) set: vel = v (vy > 5: moveFlags |= JUMP_UP); else vel += v
    void UpdateUnderwater();              // v12 0x00422020 (feb 0x0041ED00)
    void UpdateCamera();                  // v12 0x00422340 (feb 0x0041EED0) calls camCtrl->Update (site v12 0x00422416)
    void ControlFlyThrough();             // v12 0x00422570 (feb 0x0041F130) noclip
    void ControlFirstPerson();            // v12 0x004229B0 (feb 0x0041F570) walk/jump/crouch, pitch clamp 5..175
    void AnimateWeapon();                 // v12 0x00423890 (feb 0x00420430)
    void UpdateCursor();                  // v12 0x00423DE0 (feb 0x00420840) aim ray -> plrAimInfo / plrAimEnt / plrAimPoint
    animINST* GetTargetInst(m3dV*, m3dV*, float, cdtINFO*, float); // v12 0x00424310 (feb 0x00420D50)
    void NotifyMsgUse();                  // v12 0x00424DB0 (feb 0x00421760)
    int  IsCanUse(animINST*, float*);     // v12 0x00424F20 (feb 0x004218C0)
    BOOL32 HasKey();                      // slot +0x94 v12 0x00425680 (feb 0x00422030)
    BOOL32 IsStayOn(animINST *a, objOBJ *o, int notRising); // slot +0xA0 v12 0x00423800 (feb 0x004203A0)
    BOOL32 GetFloorInfo(cdtINFO *ci);                     // slot +0x44 v12 0x00425E30 (feb 0x00422780)
    void UseKey();                        // slot +0x98 v12 0x00425690 (feb 0x00422040)
    void ProcessUse();                    // v12 0x00424C10 (feb 0x004215C0) the use key
    void UpdateTimers();                  // v12 0x004251A0 (feb 0x00421B50) powerup countdown; 1.2: returns if gsAppState&0x200
    void ClearZoom();                     // v12 0x004256B0 (feb 0x00422060)
    int  IsReadyShoot();                  // v12 0x00425710 (feb 0x004220C0)
    void ActivatePowerup(int id);         // v12 0x004257B0 (feb 0x00422160) HELD -> ACTIVE, 30 s; id 2 -> timescale 0.25
    int  GetNextPriorWeapon();            // v12 0x00425A40 (feb 0x004223F0)
    void SetupGhostRendState();           // v12 0x0041F2E0 (feb 0x0041C4B0)
    void Revitalize(int full);            // slot +0x10C v12 0x0041F110 (feb 0x0041C350) back alive (debug key 4)
    void SelectSkin(int s);               // slot +0x124 v12 0x00425890 (feb 0x004222F0) the outfit, on the body too
    BOOL32 PickWeapon(int w, float n);    // slot +0xDC v12 0x00425310 the shared part, the reload, "GAME_MSG_GOT_<weapon>"
    BOOL32 PickAmmo(int w, float n);      // slot +0xE0 v12 0x004253C0 ... and the HUD message of each
    BOOL32 PickHealth(float n);           // slot +0xE4 v12 0x00425430
    BOOL32 PickArmor(float n);            // slot +0xE8 v12 0x00425490
    BOOL32 PickPowerup(int i);            // slot +0xEC v12 0x004255B0
    BOOL32 PickKey();                     // slot +0xF0 v12 0x00425630
    BOOL32 PickMoney(float n);            // slot +0xF4 v12 0x00425550
    BOOL32 PickTreasure(float n);         // slot +0xF8 v12 0x004254F0
    void SetCurWpn(int w, int animate);   // slot +0x108 v12 0x00423C90 (feb 0x00420790)
    void Animate();                       // v12 0x0042CA00 (feb 0x00427B30)
    void SyncFlags2();                    // v12 0x0041F080 (name ours) flags2 bits into the sync block
    int  InitSFX();                       // v12 0x0042DF30 (feb 0x00428E50)
    void NotifySFXBloodHands(m3dV*);      // v12 0x0042E120 (feb 0x00428FF0)
    void ProcessCDT_FIREABLE(dmgDAMAGE *d);  // slot +0xC8 v12 0x0041E730 (feb 0x0041BA90) push-back, pain flash
    void NotifyHealthChange(dmgDAMAGE *d);   // slot +0x110 v12 0x004259F0 (feb 0x00422360)
    void ProcessPowerup(int type, void *data); // slot +0x0FC v12 0x004249C0 (feb 0x004213A0) the altar's cut-scene: 0 in, 1 out
    void RevitalizeStart(int full);       // v12 0x0041F270 (feb 0x0041C450) a respawn asked of the server (notify 3)
    void SetDefaultValues();              // slot +0x104 v12 0x0041F0E0 (feb 0x0041C330) COMMON's, the view back, the body alive
    void ProcessMP_NOTIFY(void *d);       // slot +0x08C v12 0x0041EAF0 (feb 0x0041BE50) the local player's notify types
    void StartDeath(plrEV_START_DEATH *m);   // slot +0x114 v12 0x004247D0 (feb 0x004211E0) the death
    // feb-only names with no v12 twin found yet: SelectPosDirCOOP 0x0041B3C0,
    //   GetUnderwaterTimeLeft 0x0041EEA0, GetZoomStrength 0x00422050, IsCanSaveNow 0x00422320, IsCanLoadNow 0x004223E0.
};

// plrPLAYER vtable: v12 0x005FB4F8 (feb 0x005D06A8), 75 slots in v12 (feb 71).
// Slot mapping rule feb->v12: 0..7 same; 8 new; feb 8..18 -> +1; feb 19..24 (MP_UPD_POS group) -> v12 20..23;
//   feb 25..36 -> -1; 36 new; 37..43 same; 44 new; feb 44..51 -> +1; 53,54 new; feb 52..70 -> +3; 74 new.
// Pick*/Health/Armor/Powerup/Frame/INIT/SetDefaults/CDT slots verified in results/player.json; the rest by
// the rule and alignment ratio (re/work/sdk_player/ratios.json).
//   slot +0x004 [ 1] v12 0x0041DC30 (feb 0x0041B0C0) ProcessMsg
//   slot +0x018 [ 6] v12 0x0041DBB0 (feb 0x0041B080) IdentifyMPSysSyncData
//   slot +0x020 [ 8] v12 0x00428010 (new: entIACTIVE_OBJ ProcessMP_UPD_STATE)
//   slot +0x034 [13] v12 0x0041F050 (feb [12] 0x0041C300) ProcessMP_GET_SYNC_DATA
//   slot +0x038 [14] v12 0x004297D0 (feb [13] 0x004254F0) ProcessMP_WRITE_SYNC_DATA (COMMON)
//   slot +0x03C [15] v12 0x00429B80 (feb [14] 0x004258A0) ProcessMP_READ_SYNC_DATA (COMMON)
//   slot +0x040 [16] v12 0x00426710 (feb [15] 0x00422BB0) SetupAniSystem (COMMON)
//   slot +0x044 [17] v12 0x00425E30 (feb [16] 0x00422780) GetFloorInfo
//   slot +0x068 [26] v12 0x0041DE90 (feb [27] 0x0041B1B0) SetPos
//   slot +0x070 [28] v12 0x0041DED0 (feb [29] 0x0041B1F0) SetPosDir
//   slot +0x074 [29] v12 0x00425E50 (feb [30] 0x00422720) IsDead (COMMON)
//   slot +0x07C [31] v12 0x0041D600 (feb [32] 0x0041AB80) ProcessINIT
//   slot +0x080 [32] v12 0x0041D880 (feb [33] 0x0041ADE0) ProcessINIT_LEVEL
//   slot +0x084 [33] v12 0x0041E0C0 (feb [34] 0x0041B550) ProcessFRAME            <- per-frame hook
//   slot +0x08C [35] v12 0x0041EAF0 (feb [36] 0x0041BE50) ProcessMP_NOTIFY
//   slot +0x090 [36] v12 0x00425E60 (new in retail) ?
//   slot +0x094 [37] v12 0x00425680 (feb 0x00422030) HasKey
//   slot +0x098 [38] v12 0x00425690 (feb 0x00422040) UseKey
//   slot +0x09C [39] v12 0x00428100 (feb 0x00424060) Pick (COMMON)
//   slot +0x0A0 [40] v12 0x00423800 (feb 0x004203A0) IsStayOn
//   slot +0x0A4 [41] v12 0x00423780 (feb 0x00420340) ApplySpeed
//   slot +0x0A8 [42] v12 0x00425E20 (feb 0x00422710) GetGravity
//   slot +0x0AC [43] v12 0x0041DB20 (feb 0x0041AFF0) ProcessTERM
//   slot +0x0B0 [44] v12 0x00427590 (new in retail) ?
//   slot +0x0B4 [45] v12 0x0041D9E0 (feb [44] 0x0041AEE0) ProcessINIT_MAP
//   slot +0x0BC [47] v12 0x0041DA30 (feb [46] 0x0041AF70) ProcessINIT_GAME
//   slot +0x0C4 [49] v12 0x00428450 (feb [48] 0x004243B0) ProcessPICK (COMMON)
//   slot +0x0C8 [50] v12 0x0041E730 (feb [49] 0x0041BA90) ProcessCDT_FIREABLE    <- damage hook
//   slot +0x0CC [51] v12 0x00428150 (feb [50] 0x00426C10) ProcessSHOOT ?
//   slot +0x0D0 [52] v12 0x004281C0 (feb [51] 0x0041C2F0) ProcessMP_CLIENT_READY ?
//   slot +0x0D4 [53] v12 0x0041F040 (new in retail) ?
//   slot +0x0D8 [54] v12 0x0041DAB0 (new in retail) ?
//   slot +0x0DC [55] v12 0x00425310 (feb [52] 0x00421CA0) PickWeapon
//   slot +0x0E0 [56] v12 0x004253C0 (feb [53] 0x00421D10) PickAmmo
//   slot +0x0E4 [57] v12 0x00425430 (feb [54] 0x00421D90) PickHealth
//   slot +0x0E8 [58] v12 0x00425490 (feb [55] 0x00421E00) PickArmor
//   slot +0x0EC [59] v12 0x004255B0 (feb [56] 0x00421F50) PickPowerup
//   slot +0x0F0 [60] v12 0x00425630 (feb [57] 0x00421FD0) PickKey
//   slot +0x0F4 [61] v12 0x00425550 (feb [58] 0x00421EE0) PickMoney
//   slot +0x0F8 [62] v12 0x004254F0 (feb [59] 0x00421E70) PickTreasure
//   slot +0x0FC [63] v12 0x004249C0 (feb [60] 0x004213A0) ProcessPowerup
//   slot +0x100 [64] v12 0x00425E70 (feb [61] 0x004227A0) GetSpeed
//   slot +0x104 [65] v12 0x0041F0E0 (feb [62] 0x0041C330) SetDefaultValues
//   slot +0x108 [66] v12 0x00423C90 (feb [63] 0x00420790) SetCurWpn
//   slot +0x10C [67] v12 0x0041F110 (feb [64] 0x0041C350) Revitalize(int full)
//   slot +0x110 [68] v12 0x004259F0 (feb [65] 0x00422360) NotifyHealthChange
//   slot +0x114 [69] v12 0x004247D0 (feb [66] 0x004211E0) StartDeath
//   slot +0x118 [70] v12 0x00429FC0 (feb [67] 0x00425CE0) UpdateKillsCount (COMMON)
//   slot +0x120 [72] v12 0x0042A270 (feb [69] 0x00422230) SelectSkin
//   slot +0x124 [73] v12 0x00425890 (feb [70] 0x004222F0) SetTeam
//   slot +0x128 [74] v12 0x00425970 (new in retail) ?
//   Full raw listing (both classes): re/work/sdk_player/vt_player.txt

// The power-up glow emitted over the body's skin (plrPLAYER_MP::ProcessINIT). size 0x2C4
class plrPOWERUP_EMITTER : public partEMITTER_PHYS {
public:
    plrPOWERUP_EMITTER(float rate);       // v12 0x0042C3F0 (feb 0x00427630) -- not rebuilt
};

// ------------------------------------------------------------------------------------------------ plrPLAYER_MP
// Remote players in MP and the local player's own body. Entity class "player_mp", Create v12 0x0042A950
// (feb 0x00426400): operator new(0x293), +0x98 (entity mpType, feb +0x94) = 1.
class plrPLAYER_MP : public plrPLAYER_COMMON {
public:
    int          seqTarget;               // v12 +0x263 the sequence to play (sync byte 0x4F; ctor -1... INIT 1)
    objOBJ     **holders;                 // v12 +0x267 apCalloc(4, 11): "wpn_holder_*" per weapon slot
    int          shadowExcl;              // v12 +0x26B pshSHADOW_SYSTEM::AddExcludeList of the instance
    objOBJ      *boneCentre;              // v12 +0x26F "CENTRE": leans with strafing / backing up
    objOBJ      *boneLowBack;             // v12 +0x273 "LowBACK": the lower half of the twist and the bend
    objOBJ      *boneUpBack;              // v12 +0x277 "UpBACK": the upper half
    partEMITTER_PHYS *pwrEmitter;         // v12 +0x27B plrPOWERUP_EMITTER (new 0x2C4, 0.1)
    int          nSkin;                   // v12 +0x27F objects of the "Skin" part source (the emitter's area)
    objOBJ      *skin[4];                 // v12 +0x283
    // size 0x293
    static plrPLAYER_MP *__fastcall Create(animINST *i);           // v12 0x0042A950 (feb 0x00426400)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // slot +0x04 v12 0x0042AA40 (feb 0x004264B0)
    void   AnimateBody();                 // v12 0x0042BE10 (feb ?) the frame's animation (name ours)
    void   UpdateWeapon();                // v12 0x0042C000 (feb 0x004272B0)
    void   AnimateProcedural();           // v12 0x0042D050 (feb 0x004280E0)
    void   SetupGhostWpnState();          // v12 0x0042C2D0 (feb 0x00427580) its weapons kept to the mirror pass
    // ---- its own virtual slots, v12 vtable 0x005FB708 from slot 25 (+0x64; entENTITY's end at +0x60). Those not
    //      rebuilt yet have no body here: the generated stubs stop the program with their name if reached.
    virtual void   GetPos(m3dV *out);                         // [25] +0x064 v12 0x0041DE70 the instance's
    virtual void   SetPos(m3dV *p);                           // [26] +0x068 v12 0x00403950 (nothing)
    virtual void   GetDir(m3dV *out);                         // [27] +0x06C v12 0x0041DE80
    virtual void   SetPosDir(m3dV *pos, m3dV *dir, int onFloor); // [28] +0x070 v12 0x0042BB30 a remote one remembers it
    virtual BOOL32 IsDead();                                  // [29] +0x074 v12 0x00425E50 moveFlags 2
    virtual BOOL32 IsActive();                                // [30] +0x078 v12 0x00546E40 (plgsACTION)
    virtual BOOL32 ProcessINIT();                             // [31] +0x07C v12 0x0042AA70 (feb 0x004264E0)
    virtual BOOL32 ProcessINIT_LEVEL();                       // [32] +0x080 v12 0x0042AD80 (feb 0x00426810)
    virtual void   ProcessFRAME();                            // [33] +0x084 v12 0x0042AF30 (feb 0x004268F0)
    virtual BOOL32 IsRemote();                                // [34] +0x088 v12 0x0042C670 mpType != 2 (name ours)
    virtual void   ProcessMP_NOTIFY(void *d);                 // [35] +0x08C v12 0x0042B4D0 -- not rebuilt
    virtual BOOL32 Slot90();                                  // [36] +0x090 v12 0x00425E60 flags2 8
    virtual BOOL32 HasKey();                                  // [37] +0x094 v12 0x0048DF30 0
    virtual void   UseKey();                                  // [38] +0x098 v12 0x00403A20 (nothing)
    virtual BOOL32 Pick(entENTITY *item);                     // [39] +0x09C v12 0x00428100 (COMMON)
    virtual BOOL32 IsStayOn(animINST *a, objOBJ *o, int notRising); // [40] +0x0A0 v12 0x00403970 0
    virtual void   ApplySpeed(m3dV *v, int set);              // [41] +0x0A4 v12 0x00403860 (nothing)
    virtual float  GetGravity();                              // [42] +0x0A8 v12 0x00425E20 (plgsACTION)
    virtual void   ProcessTERM();                             // [43] +0x0AC v12 0x0042AD00 (feb 0x00426790)
    virtual void   ProcessTERM_LEVEL();                       // [44] +0x0B0 v12 0x00427590 (COMMON)
    virtual BOOL32 ProcessINIT_MAP();                         // [45] +0x0B4 v12 0x0042AE80 (feb 0x00426840)
    virtual BOOL32 Slot0B8();                                 // [46] +0x0B8 v12 0x00434800 1 (msg 0x3F3)
    virtual BOOL32 ProcessINIT_GAME();                        // [47] +0x0BC v12 0x0042AE90 (feb 0x00426850)
    virtual void   ProcessTERM_GAME();                        // [48] +0x0C0 v12 0x0042AEE0 (feb 0x004268A0)
    virtual void   ProcessPICK(void *d);                      // [49] +0x0C4 v12 0x00428450 (COMMON) -- not rebuilt
    virtual void   ProcessCDT_FIREABLE(dmgDAMAGE *d);         // [50] +0x0C8 v12 0x0042B3E0 (feb 0x00426B20) -- not rebuilt
    virtual void   ProcessSHOOT(void *msg);                   // [51] +0x0CC v12 0x00428150 (COMMON)
    virtual void   ProcessSHOOT_BODY(void *msg);              // [52] +0x0D0 v12 0x004281C0 (COMMON)
    virtual void   Slot0D4(void *d);                          // [53] +0x0D4 v12 0x0042BA10 (msg 0x406) -- not rebuilt
    virtual void   Slot0D8(void *d);                          // [54] +0x0D8 v12 0x00403950 (msg 0x408: nothing)
    virtual BOOL32 PickWeapon(void *d);                       // [55] +0x0DC v12 0x0042BCC0 -- not rebuilt
    virtual BOOL32 PickAmmo(void *d);                         // [56] +0x0E0 v12 0x0042BCE0 -- not rebuilt
    virtual BOOL32 PickHealth(void *d);                       // [57] +0x0E4 v12 0x0042BD00 -- not rebuilt
    virtual BOOL32 PickArmor(void *d);                        // [58] +0x0E8 v12 0x00428A30 -- not rebuilt
    virtual BOOL32 PickPowerup(void *d);                      // [59] +0x0EC v12 0x00428CF0 -- not rebuilt
    virtual BOOL32 PickKey(void *d);                          // [60] +0x0F0 v12 0x0042BD20 -- not rebuilt
    virtual BOOL32 PickMoney(void *d);                        // [61] +0x0F4 v12 0x00428C70 -- not rebuilt
    virtual BOOL32 PickTreasure(void *d);                     // [62] +0x0F8 v12 0x00428B70 -- not rebuilt
    virtual void   ProcessPowerup(int id);                    // [63] +0x0FC v12 0x0042C300 -- not rebuilt
    virtual void   Slot100(m3dV *out);                        // [64] +0x100 v12 0x0042C640 m3dVZero
    virtual void   SetDefaultValues();                        // [65] +0x104 v12 0x0042BB70 (feb 0x00427010)
    virtual void   SetCurWpn(int w, int animate);             // [66] +0x108 v12 0x0042BB90 (feb 0x00427030)
    virtual void   Slot10C(int on);                           // [67] +0x10C v12 0x0042C5C0 -- not rebuilt
    virtual void   NotifyHealthChange(dmgDAMAGE *d);          // [68] +0x110 v12 0x00429040 (COMMON)
    virtual void   StartDeath();                              // [69] +0x114 v12 0x0042BC80 (feb 0x00427120) -- not rebuilt
    virtual void   UpdateKillsCount();                        // [70] +0x118 v12 0x00429FC0 -- not rebuilt
    virtual BOOL32 IsThirdPerson();                           // [71] +0x11C v12 0x004031F0 1 (name ours)
    virtual void   Slot120();                                 // [72] +0x120 v12 0x0042C5B0 a remote one's UpdateSound
    virtual void   SelectSkin(int s);                         // [73] +0x124 v12 0x0042C6D0 (feb 0x00427820)
    virtual void   SetTeam(int t);                            // [74] +0x128 v12 0x0042C900 (feb 0x00427A40)
    virtual void   ReadAnimSync(u8 *sync);                    // [75] +0x12C v12 0x0042BD40 (name ours) the player's anim state in
    // The outfits: "Skins" -> Skin_0N shown one at a time, some with their textures replaced and parts hidden.
    struct SKIN {
        char  name[0x80];                 // +0x00 the Skin_0N object
        int   nRepl;                      // +0x80
        char *from[4];                    // +0x84
        char *to[4];                      // +0x94
        int   nHide;                      // +0xA4
        char *hide[4];                    // +0xA8 objects hidden by name prefix
    };                                    // 0xB8
    // The TH team colours of each outfit: its textures and their _r / _b versions.
    struct TEAM {
        int   n;                          // +0x00
        char *tex[4];                     // +0x04
        char *teamTex[2][4];              // +0x14
    };                                    // 0x34
    static SKIN arraySkin[6];             // v12 0x0062C000 (feb 0x005EEBE0)
    static TEAM arrayTeam[6];             // v12 0x0062C450 (feb 0x005EF030)
    static txmTEXTURE* pTexPwrQd;         // v12 0x0064327C (feb 0x00603F68) "plr_pwr_qd" quad-damage overlay
    static txmTEXTURE* pTexPwrUa;         // v12 0x00643284 (feb 0x00603F70) "plr_pwr_ua" unholy-armor overlay
    // ProcessMsg v12 0x0042AA40 (feb 0x004264B0); ProcessINIT 0x0042AA70 (0x004264E0); ProcessTERM 0x0042AD00 (0x00426790)
    // ProcessINIT_GAME 0x0042AE90 (0x00426850); ProcessTERM_GAME 0x0042AEE0 (0x004268A0)
    // ProcessFRAME v12 0x0042AF30 (feb 0x004268F0)  vtable slot 33
    // ProcessCDT_FIREABLE v12 0x0042B3E0 (0x00426B20); ProcessMP_NOTIFY 0x0042B4D0 (0x00426C20)
    // SetDefaultValues 0x0042BB70 (0x00427010); SetCurWpn 0x0042BB90 (0x00427030); StartDeath 0x0042BC80 (0x00427120)
    // PickWeapon 0x0042BCC0; PickAmmo 0x0042BCE0; PickHealth 0x0042BD00; PickKey(COMMON) 0x0042BD20
    // ProcessPowerup 0x0042C300 (0x004275B0); SetupGhostWpnState 0x0042C2D0 (0x00427580)
    // plrPOWERUP_EMITTER ctor 0x0042C3F0 (0x00427630)
};
// plrPLAYER_MP vtable v12 0x005FB708 (feb 0x005D09A8): same slot rule as plrPLAYER.

// ------------------------------------------------------------------------------------------------ camera / height
struct plrCAM_CTRL {                      // 0x4C, layout identical in feb and 1.2 (ctor byte-identical)
    int   zoomState;                      // +0x00 0 off, 1 zooming in, 2 zooming out
    float zoomFov;                        // +0x04 the view angle zooming starts at: normal - 55, then 45 / s down to 10.5
    float savedFov;                       // +0x08 camera horizontal angle before zoom
    float zoomT;                          // +0x0C
    m3dV  kick;                           // +0x10 weapon recoil kick (ShootNotify)
    float kickT;                          // +0x1C
    float kickTime;                       // +0x20 per weapon: 1 -> 0.07, 2 -> 0.35, 6 -> 0.5, 10 -> 0
    m3dV  deathOrigin;                    // +0x24 DeathNotify: camera origin
    m3dV  deathLookAt;                    // +0x30 DeathNotify: camera look-at
    float deathTime;                      // +0x3C time into the death view (0: the fall starts; -10: none, the game holds the camera)
    float deathEnd;                       // +0x40 1.0 while the death view runs, 0 off
    int   attached;                       // +0x44 PowerupNotify: the view follows an object
    objOBJ *attachObj;                    // +0x48 its origin and Z axis
    plrCAM_CTRL();                        // v12 0x0042D8C0 (feb 0x004287F0)
    float GetZoomStrength();              // v12 0x0042D900 (feb 0x00428830)  tan(savedFov)/tan(curFov)
    void ZoomNotify(int on);              // v12 0x0042D940 (feb 0x00428870)
    void ZoomClear();                     // v12 0x0042D9A0 (feb 0x004288D0)
    void ShootNotify(int wpn);            // v12 0x0042D9C0 (feb 0x004288F0)  camera recoil table
    void DeathNotify(int quiet);          // v12 0x0042DAB0 (feb 0x004289E0)
    void RevitalizeNotify();              // v12 0x0042DB00 (feb 0x00428A30)
    void PowerupNotify(int on, int a, objOBJ *o); // v12 0x0042DB10 (feb 0x00428A40)
    void Update();                        // v12 0x0042DB40 (feb 0x00428A70)  writes gsCameraPtr each frame
};

class m3dSPL_BEZIER2D;
struct plrHEIGHT_CTRL {                   // 0x20: eye height (crouch blend + landing dip spline)
    float value;                          // +0x00 current eye height without the dip (1.6 .. 1.0)
    float lowH;                           // +0x04 1.0 crouched eye height
    float highH;                          // +0x08 1.6 standing eye height
    float t;                              // +0x0C landing dip time
    float duration;                       // +0x10 landing dip length, < 0 = none
    m3dSPL_BEZIER2D *spline;              // +0x14 dip shape: y = height factor, 3 keys (ApplySpeed)
    float crouchT;                        // +0x18 0 .. crouchTime
    float crouchTime;                     // +0x1C 0.12 s to crouch fully
    plrHEIGHT_CTRL();                     // v12 0x0042D4E0 (feb 0x00428410)
    ~plrHEIGHT_CTRL();                    // v12 0x0042D570 (feb 0x004284A0)
    void  ApplySpeed(float vy);           // v12 0x0042D580 (feb 0x004284B0) landing with vertical speed vy (< 0)
    float Update();                       // v12 0x0042D770 (feb 0x004286A0) returns the eye height
    void  Reset();                        // v12 0x0042D8B0 (feb 0x004287E0)
};

// ------------------------------------------------------------------------------------------------ collision filters
// plrCDT_REFINE: what the player collides with. vtable v12 0x005FB664 (feb 0x005D0920). The class mask
// (entCDT_REFINE_EXCL::cls, +0x10) is 1: water volumes are walked through.
class plrCDT_REFINE : public entCDT_REFINE_EXCL {
public:
    BOOL32 IsInclInst(animINST *i);       // v12 0x00425AD0 (feb 0x00422480)
    BOOL32 IsInclObj(objOBJ *o);          // v12 0x00425BE0 (feb 0x00422570)
    plrCDT_REFINE(int m, animINST *excl, u32 cls) : entCDT_REFINE_EXCL(m, excl, cls) {}
};
// plrCDT_REFINE_NPC: the auto-aim's targets (GetTargetInst): living NPCs. vtable v12 0x005FA0EC.
class plrCDT_REFINE_NPC : public entCDT_REFINE_EXCL {
public:
    BOOL32 IsInclInst(animINST *i);       // v12 0x00425C60
    plrCDT_REFINE_NPC(animINST *excl) : entCDT_REFINE_EXCL(0, excl, 4) {}
};
// plrCDT_REFINE_BUTTON: what the use key and the use tips look at. vtable v12 0x005FB6E0 (1.2 only, name from
// the RTTI-less slot table): buttons and instances without an entity.
class plrCDT_REFINE_BUTTON : public cdtREFINE {
public:
    BOOL32 IsInclInst(animINST *i);       // v12 0x00425C30
    plrCDT_REFINE_BUTTON(animINST *excl) : cdtREFINE(0, excl) {}
};

// ------------------------------------------------------------------------------------------------ globals
extern plrPLAYER*    plrPlayer;           // v12 0x00641BF8 (feb 0x006035F8)
extern _plrMOVE_CTRL _plrCtrlForw;        // v12 0x00641B1C (feb 0x00603528)
extern _plrMOVE_CTRL _plrCtrlSide;        // v12 0x00641BF0 (feb 0x006035F0)
extern m3dV          _plrPosJumpStart;    // v12 0x00641B24 (feb 0x00603534) where the last jump started
extern cdtINFO       _plrLastCDTInfo;     // v12 0x00641BFC (1.2) last push-out contact of ProcessPhysics (name ours)
extern int           plrSlideOnSlopes;    // v12 0x0062B3D4 (feb 0x005EE370) 1: steep floors push the player down (name ours)
extern float         _plrPwrCostTbl[3];   // v12 0x0062B868 (feb 0x005EE740) {50,50,50} gold per powerup (SP altar)
// The aim ray's results are members of gscCursor (game_systems/render.h): aimInfo (v12 0x0098EAC8, feb 0x009483AC),
// aimEnt (v12 0x0098EAC4, feb 0x009483A8), aimPoint (v12 0x0098EAA4, feb 0x0094838C), x / y the screen point.
extern gsINP_SYSTEM* gsSysInput;          // v12 0x0098E838 (feb 0x00948238); key state at +0x188C, keys +0xC
// gsINP_SYSTEM::IsOn(int) v12 0x00425CF0 (feb 0x00422670); IsToggled v12 0x00425D20 (feb 0x004226A0);
//   Power v12 0x00425DC0 (feb 0x004226D0); GetInput detour point v12 0x004B67F0 (feb 0x004A6B10).
// plrInit v12 0x0041D420 (feb 0x0041A9A0): plrCommInit, pwpInit, entRegisterClass("player"/"player_mp"),
//   textures. plrCommInit v12 0x004265A0 (feb 0x00422A70).

#pragma pack(pop)
