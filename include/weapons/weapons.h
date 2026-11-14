// weapons.h -- Will Rock 1.2 unofficial SDK, subsystem "weapons": the player weapons (pwp*, objs plw_*.obj and
//   pwp_comm.obj of the Feb 2003 map). Built by static analysis of v12.exe against feb.exe + WillRock.map.
//
// Assumed from re/sdk/core / engine_core / game_shared: entENTITY (v12 size 0x16B, feb 0x14F), wpnWEAPON
//   (game_shared/weapon_base.h: flags +0x16B, shootRate +0x16F, maxAmmo +0x173, damage +0x177, cooldown +0x17B,
//   shootPos/Dir, tip +0x197, ammo +0x19B), dmgWEAPON (game_shared/damage.h), physPHYS / physINIT
//   (engine_core/physics.h), animINST, objOBJ, m3dV, m3dMATR, cdtINFO, sncCHANNEL, sncCHANNEL_CONE, sncSOUND_DESCR_3D,
//   partEMITTER_PHYS, m3dSPL_LINEAR1D, gsEVENT, msgADDR/msgDATA, plrPLAYER (player/player.h).
//
// Conventions: "// v12 +0xNN (feb +0xNN)" for fields, "// v12 0x00XXXXXX (feb 0x00XXXXXX)" for code/data.
//   Fields marked without "?" were read from a v12 instruction (address given) or from a v12 ProcessINIT that
//   stores the same constants at feb offset + 0x1C (re/work/sdk_weapons/inits.py output, all 11 weapons line up).
//   "// ?" = not confirmed in v12. All classes are #pragma pack(1) (odd offsets are real).
// Field shift rule for every pwp* weapon: feb offset >= 0x14F -> v12 = feb + 0x1C (entENTITY grew 4 bytes and
//   wpnWEAPON's block grew 0x18). The anim pointer feb +0xBC -> v12 +0xC0. entENTITY +0xF8 (anim event id)
//   -> v12 +0x108 (v12 pwpWEAPON::UpdateSound 0x004180A0).
// Full function list with confidence: function_index.tsv (feb, v12, name, mnemonic similarity, how matched).

#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include "../game_shared/weapon_base.h"
#pragma pack(push, 1)

class pwpWEAPON; class pwpBULLET_WPN; struct pwpMCG_SHOOT; struct pjlPJL_INIT; class m3dSPL_LINEAR1D;

// =================================================================================================================
// Weapon index (entENTITY +0x98 v12 / +0x94 feb = OFS_WPN_TYPE). Written by every pwp*::Create, read by
// dmgWEAPON::weaponType, pwpWEAPON::AdjustTiming, the HUD and the player's inventory. Same numbers in feb and 1.2.
// Evidence: feb Create "mov [esi+0x94],N" (feb addrs below), v12 "mov [esi+0x98],N" (e.g. shovel 0x00416E9F).
// =================================================================================================================
enum pwpWEAPON_INDEX {
    PWP_SHOVEL     = 0,   // pwpSHOVEL     class "pwp_svl"  Create v12 0x00416E70 (feb 0x00415110)
    PWP_COLT       = 1,   // pwpCOLT       class "pwp_clt"  Create v12 0x0040CFB0 (feb 0x0040C110)  revolver
    PWP_STG        = 2,   // pwpSTG        class "pwp_stg"  Create v12 0x00415170 (feb 0x00413830)  shotgun
    PWP_MACHINEGUN = 3,   // pwpMACHINEGUN class "pwp_mcg"  Create v12 0x00410330 (feb 0x0040ECD0)  Tommy gun
    PWP_CROSSBOW   = 4,   // pwpCROSSBOW   class "pwp_crb"  Create ? (feb 0x0040E570)  explosive bolts
    PWP_ACIDGUN    = 5,   // pwpACIDGUN    class "pwp_acd"  Create v12 0x00403A50 (feb 0x00403760)
    PWP_BASOOKA    = 6,   // pwpBASOOKA    class "pwp_bsk"  Create v12 0x00409FA0 (feb 0x004095D0)  rocket launcher
    PWP_MEDUSA     = 7,   // pwpMEDUSA     class "pwp_mdg"  Create v12 0x00412170 (feb 0x004107C0)  petrifying gas balloons
    PWP_MINIGUN    = 8,   // pwpMINIGUN    class "pwp_mng"  Create v12 0x004142E0 (feb 0x00412980)
    PWP_BOMB       = 9,   // pwpBOMB       class "pwp_bmb"  Create v12 0x004081B0 (feb 0x00407C60)  bomb (grenade)
    PWP_ATOMICGUN  = 10,  // pwpATOMICGUN  class "pwp_amg"  Create ? (feb 0x00406230)  "atomic" rocket gun
};

// pwpWEAPON::flags2 (v12 +0x1AF, feb +0x193)
enum pwpFLAGS2 {
    PWPF2_SHOOT_ON_ACTFRAME = 0x02,  // IsShooting() waits for anim action frame 7/shootActFrame (v12 0x00418020 "test [ecx+0x1AF],2")
    PWPF2_ZOOM              = 0x08,  // SetZoom(1) (v12 0x00418070 "or al,8"); crossbow/colt zoom
    PWPF2_MNG_STOP_SOUND    = 0x20,  // minigun 1.2: play stop_shoot_1 once (build_differences.md 2)
};

// Shot messages (sent to the weapon itself). Ids >= 0x3F5 moved +1 in retail.
#define PWP_MSG_SHOOT        0x4C7   // v12 (feb 0x4C6): wpnWEAPON::ProcessMsg -> vslot ProcessSHOOT (+0x98). v12 0x00544025
#define PWP_MSG_SHOOT_BODY   0x4C8   // v12 only: pwpWEAPON::ProcessMsg 0x00417FF4 "cmp eax,0x4C8" -> vslot +0xB4 (ShootBody)

// =================================================================================================================
// pwpWEAPON : wpnWEAPON  -- base of every player weapon (pwp_comm.obj).
// size v12 0x1CB (feb 0x1AF). No own Create (abstract). vtable v12 0x005F9580 (feb 0x005CE820): verified as the
// vtable written right after pwpACIDGUN's in .rdata and by slot-by-slot comparison (all base slots match).
// =================================================================================================================
class pwpWEAPON : public wpnWEAPON {
public:
    // ---- virtuals (v12 vtable 0x005F9580; rebuilt in src/Sources/pwp_comm.cpp) ----------------------------------
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // +0x04 v12 0x00417FF0 (0x4C8 -> ShootBody)
    virtual BOOL32 SetupAniSystem();                       // +0x40 v12 0x00417920 the PWP_SEQ_* sequences + transitions
    virtual int    SubtractAmmo(float n);                  // +0x64 v12 0x00417680
    virtual int    ProcessINIT();                          // +0x80 v12 0x00417D50
    virtual void   ProcessFRAME();                         // +0x88 v12 0x00417F50
    virtual int    ProcessINIT_LEVEL();                    // +0x8C v12 0x00548680 (return 1)
    virtual void   ProcessTERM_LEVEL();                    // +0x90 v12 0x00417F40 stops the weapon sound
    virtual void   SetShootRate(float r);                  // +0xA0 v12 0x004176D0
    virtual float  GetDamage();                            // +0xA4 v12 0x004176F0
    virtual void   UpdateSound();                          // +0xAC v12 0x004180A0
    virtual void   Anim2Sound();                           // +0xB0 v12 0x00403A20 (empty)
    virtual void   ShootBody(void *msg);                   // +0xB4 v12 0x00403950 (empty) 1.2 NEW, msg 0x4C8
    virtual void   AdvanceAnimation(int seq, float dt);    // +0xB8 v12 0x00403B10
    virtual int    GetCurAnimSeq();                        // +0xBC v12 0x00403B40
    virtual BOOL32 IsWeaponMP();                           // +0xC0 v12 0x00403B50 a remote player's copy (flags2 & 4)
    virtual BOOL32 IsShooting();                           // +0xC4 v12 0x00418020
    virtual BOOL32 IsReadyRecharge();                      // +0xC8 v12 0x00418050
    virtual void   StopSound();                            // +0xCC v12 0x004180F0 1.2 NEW (name ours)
    // wpnWEAPON block ends at v12 +0x19F (feb +0x183)
    sncCHANNEL_CONE *sndChannel;   // v12 +0x19F (feb +0x183) weapon sound channel, created in ProcessINIT: sncCHANNEL_CONE
                                  //   min 10 / max 50 m (feb decompile 00415970). v12 0x00417F40 "mov ecx,[ecx+0x19F]"
    plrPLAYER *owner;             // v12 +0x1A3 (feb +0x187) owning player; GetDamage asks owner->IsActivePowerup(0)
                                  //   (v12 0x004176F7 "mov ecx,[ecx+0x1A3]"); owner->mpId sent in shot msgs
    int    clipSize;              // v12 +0x1A7 (feb +0x18B) -1 = no clip. v12 ProcessINIT 0x00417DAE; Recharge 0x00417630
    int    clip;                  // v12 +0x1AB (feb +0x18F) rounds in clip, -1 = no clip. SubtractAmmo decrements it
    u32    flags2;                // v12 +0x1AF (feb +0x193) pwpFLAGS2. v12 0x00417DCC
    int    reloadActFrame;        // v12 +0x1B3 (feb +0x197) action-frame id tested by IsReadyRecharge (feb decompile)
    int    shootActFrame;         // v12 +0x1B7 (feb +0x19B) action-frame id tested by IsShooting. v12 0x0041802F
    int    unk1BB;                // v12 +0x1BB (feb +0x19F) init 0  // ?
    float  baseShootRate;         // v12 +0x1BF (feb +0x1A3) copy written by SetShootRate (v12 0x004176D0)
    float  unk1C3;                // v12 +0x1C3 (feb +0x1A7) init 1.0: animation speed (AdvanceAnimation dt * it)
    int    lastAnimEvent;         // v12 +0x1C7 (feb +0x1AB) entENTITY event id latched by UpdateSound. v12 0x004180B0

    // ---- non-virtual functions -----------------------------------------------------------------------------
    pwpWEAPON() { sndChannel = 0; owner = 0; unk1C3 = 1.0f; } // inline in every pwp*::Create (v12 e.g. 0x00416E7B)
    void Recharge();              // v12 0x00417630 (feb 0x00415250) clip += min(ftol(x), clipSize-clip)
    void AdjustTiming();          // v12 0x00417720 (feb 0x00415340) per-weapon-index anim speed factors (switch on +0x98)
    void SetZoom(int on);         // v12 0x00418070 (feb 0x00415C30) flags2 bit 8
    int  IsEmpty();               // v12 0x004180C0 (feb 0x00415C80) !(wpnflags&4) && ammo <= 0
};
// Free functions (pwp_comm.obj)
int __fastcall  pwpInit();                   // v12 0x00416F00 ? (feb 0x00415170) calls every pwp*Init (class registration)
void __fastcall pwpTerm();                   // v12 0x00416FB0 (feb 0x00415200) exact
// Globals used by the weapons
extern wrGO_SYSTEM *wrSysGO;      // v12 0x0064FF5C (feb 0x0060EFDC). +0xA4 v12 (+0xA0 feb) != 0 -> SubtractAmmo does nothing
                                  //   (infinite ammo option). v12 pwpWEAPON::SubtractAmmo 0x00417680

// =================================================================================================================
// Per-weapon base values set in each ProcessINIT (all IDENTICAL in feb and 1.2; v12 sites from work/sdk_weapons/tweak.py).
// Patch the immediate: "mov dword ptr [esi+X], imm32" -> imm at site+6; "push imm32" -> imm at site+1.
// rate = shots/s given to SetShootRate (cooldown 1/rate); 25 = effectively "limited by the animation".
// damage = wpnWEAPON::damage (per ray / per projectile; GetDamage() x4 with the Titan powerup).
// -----------------------------------------------------------------------------------------------------------------
//  weapon      ProcessINIT v12 (feb)     rate         damage                  maxAmmo                 clip
//  SHOVEL      0x004154A0 (0x00413B10)   25 @004154C7 120 @004154EE           (infinite: wpnflags|=4)  -
//  COLT        0x0040C800 (0x0040B940)   0 (ebx)      10 @0040C888            ?                        14 (reg @0040C80E)
//  STG         0x004145A0 (0x00412C60)   1.4 @00414601 4 @0041461D (per pellet) 200 @00414627          5 (reg @004145AE)
//  MACHINEGUN  0x0040FC60 (0x0040E600)   10 @0040FC83 6 @0040FC99             1000 @0040FCA3           -
//  MINIGUN     0x00413DC0 (0x00412510)   10 @00413DD4 8 @00413E17             1000 @00413E21           100 (reg @00413DE3)
//  CROSSBOW    0x0040DC90 (0x0040CB80)   25 @0040DD05 60 @0040DD5C            50 @0040DD66             6 @0040DD3E
//  ACIDGUN     0x00401450 (0x00401410)   25 @004015FB 60 @00401634            100 @0040163E            5 (reg @004015F3)
//  ATOMICGUN   0x004048F0 (0x00404390)   25 @004049E2 200 @00404A26           20 @00404A30             4 @00404A12
//  BASOOKA     0x004084F0 (0x00407FA0)   0.714286 @00408519 90 @0040854A      100 @00408711            -1 (none)
//  MEDUSA      0x004105F0 (0x0040EF90)   1 @00410630  50 @0041065D            50 @0041068E             3 (reg @0041061B)
//  BOMB        0x00406C80 (0x00406740)   25 @00406CA2 110 @00406CE6           20 @00406CF0             1 @00406CC8
// =================================================================================================================

// =================================================================================================================
// pwpBULLET_WPN : pwpWEAPON -- hitscan base (colt, shotgun, machinegun, minigun). plw_bul_wpn.obj
// size v12 0x1DF (feb 0x1C3) // ? by rule. vtable v12 0x005F9FD0 (feb 0x005CF270) (written by the ctor, paired by
// work/sdk_weapons/vtpairs.json). ctor v12 ? (feb 0x00409960).
// Shot path 1.2: Shoot (slot +0x68) builds the ray list (FillCDTShootList, slot +0xF8) and sends msg 0x4C7 to itself
//   -> slot +0x98 = shared relay v12 0x0040AC20 re-sends msg 0x4C8 -> pwpWEAPON::ProcessMsg -> slot +0xB4 =
//   ShootBody v12 0x0040AC80 (feb ProcessSHOOT 0x0040A280): damage per ray (dmgWEAPON, msg GS_MSG_DAMAGE 0x4C6),
//   then ShowShotSFX (slot +0xD8) -> ShowSFX per 0x15-byte ray record (slot +0xDC).
// =================================================================================================================
class pwpBULLET_WPN : public pwpWEAPON {               // vtable v12 0x005F9FD0; src/Sources/plw_bul_wpn.cpp
public:
    // overrides
    virtual int   Shoot(entENTITY *target, m3dV *aim);                    // +0x68 v12 0x0040A8B0 (feb 0x00409EB0)
    virtual int   ProcessINIT();                                          // +0x80 v12 0x0040A3D0 (feb 0x004099C0)
    virtual void  ProcessTERM();                                          // +0x84 v12 0x0040A730 (feb 0x00409D30)
    virtual void  ProcessFRAME();                                         // +0x88 v12 0x0040A6C0 (feb 0x00409CC0)
    virtual void  ProcessSHOOT(void *msg);                                // +0x98 v12 0x0040AC20 the relay to ShootBody
    virtual void  ShootBody(void *msg);                                   // +0xB4 v12 0x0040AC80 (feb ProcessSHOOT 0x0040A280)
    // the bullet weapon's own slots
    virtual sncSOUND_DESCR_3D *GetShellSoundDesc();                       // +0xD0 v12 0x0040A530 (feb 0x00409B30)
    virtual float GetScorchmarkScale();                                   // +0xD4 v12 0x00431E30 (feb 0x0042C490) 1.0
    virtual void  ShowShotSFX(void *msg);                                 // +0xD8 v12 0x0040ADC0 1.2 NEW
    virtual void  ShowSFX(struct pwpMCG_RAY *r);                          // +0xDC v12 0x0040AE70 (feb 0x0040A380) one ray
    virtual int   CreateFlare(float scale);                               // +0xE0 v12 0x0040A400 (feb 0x004099F0)
    virtual int   CreateShell(float a, float b, m3dSPL_LINEAR1D *spl);    // +0xE4 v12 0x0040A540 (feb 0x00409B40)
    virtual int   CreateSmoke(float scale);                               // +0xE8 v12 0x0040A4E0 (feb 0x00409AE0)
    virtual void  StartShells(int n);                                     // +0xEC v12 0x0040B0A0 (feb 0x0040A5A0)
    virtual void  PlayShootSound();                                       // +0xF0 v12 0x0040B080 (feb 0x0040A580)
    virtual void  CorrectAim(cdtINFO *aim, m3dV *at);                     // +0xF4 v12 0x00403860 (empty; name ours)
    virtual int   FillCDTShootList(m3dV *org, cdtINFO *aim, cdtINFO *hits, int max); // +0xF8 v12 0x0040B340 (feb 0x0040A740)
    virtual int   FillCDTShootListSpread(m3dV *org, m3dV *dir, float a, float b, cdtINFO *hits, int max); // +0xFC v12 0x0040B360 -- not rebuilt yet
    virtual float GetRadiusNoSdrAround();                                 // +0x100 v12 0x0040C580 (feb 0x0040B680) 0.085

    entENTITY *flare;             // v12 +0x1CB (feb +0x1AF) "dynamic"/"sfx_mcg_flare" muzzle flare (CreateFlare)
    partEMITTER_PHYS *muzzleSmoke;// v12 +0x1CF (feb +0x1B3) pwpMCG_MUZZLE_SMOKE (0x2C4 bytes, CreateSmoke)
    partEMITTER_PHYS *shells;     // v12 +0x1D3 (feb +0x1B7) pwpMCG_SHELL emitter (0x2CC bytes, CreateShell)
    sncCHANNEL *shellSnd;         // v12 +0x1D7 (feb +0x1BB) the shell-drop channel (a sphere 1..5)
    float  flareScale;            // v12 +0x1DB (feb +0x1BF) ctor 1.0; the hit effects' scale (sfx_mcg_hit init data)
    pwpBULLET_WPN();                                          // v12 0x0040A370 (feb 0x00409960)
    int  PrepareShootMsg(struct pwpMCG_RAY *r, cdtINFO *ci);   // v12 0x0040A790 (feb 0x00409D90)
};
int __fastcall pwpBulWpnInit();                               // v12 0x0040A1C0 (feb 0x004097D0)

// Spread (FillCDTShootList overrides): cone half-angle grows linearly with target distance d:
//   MACHINEGUN v12 0x0040FDF0 (feb 0x0040E790): 0.1 at d<=3 m .. 0.5 at d>=80 m  (slope 0.00519481)
//   STG        v12 0x004147A0 (feb 0x00412E60): 0.15 at d<=5 .. 4.0 at d>=80     (slope 0.0513333), pellets via slot +0xFC
//   MINIGUN    v12 0x00413700 (feb 0x00411E90): 0.45 at d<=5 .. 2.0 at d>=80     (slope 0.0206667)
//   Constants are the literals "mov [ebp-0x14],0.1f / 0.5f" (MCG v12 0x0040FEED / 0x0040FF08) and
//   "0.45f / 2f" (MNG v12 0x004137FD / 0x00413818); slopes and 3/5/80 are pooled __real constants (do not patch).

class pwpCOLT : public pwpBULLET_WPN {               // size v12 0x1DF (feb 0x1C3); no own fields. vtable v12 0x005FA1D0
public:                                                // src/Sources/plw_clt.cpp
    virtual BOOL32 SetupAniSystem();                   // +0x40 v12 0x0040C9A0 (feb 0x0040BAE0) 11 sequences
    virtual int    ProcessINIT();                      // +0x80 v12 0x0040C800 (feb 0x0040B940)
    virtual void   ProcessFRAME();                     // +0x88 v12 0x0040C940 (feb 0x0040BA80)
    virtual void   UpdateSound();                      // +0xAC v12 0x0040C7D0 (feb 0x0040B910)
    virtual void   Anim2Sound();                       // +0xB0 v12 0x0040C7A0 (feb 0x0040B8E0)
    virtual int    GetCurAnimSeq();                    // +0xBC v12 0x0040E070 (folded with the crossbow's)
    virtual sncSOUND_DESCR_3D *GetShellSoundDesc();    // +0xD0 v12 0x0040C740
    virtual void   PlayShootSound();                   // +0xF0 v12 0x0040C750 (feb 0x0040B890)
    void ThrowClip();                                  // v12 0x0040CDF0 (feb ?) the empty clip, a "rigid" for 5 s
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x0040CFB0
};
int __fastcall pwpCLTInit();                           // v12 0x0040C710 (feb 0x0040B850)
class pwpMEDUSA : public pwpWEAPON {                  // size v12 0x1CB (feb 0x1AF); no own fields. vtable v12 0x005FA850
public:                                                // src/Sources/plw_mdg.cpp (the shot not rebuilt)
    virtual int    ProcessINIT();                      // +0x80 v12 0x004105F0 (feb 0x0040EF90)
    virtual void   ProcessFRAME();                     // +0x88 v12 0x004106B0 (feb 0x0040F050)
    virtual void   UpdateSound();                      // +0xAC v12 0x004105A0 (feb 0x0040EF40)
    virtual void   Anim2Sound();                       // +0xB0 v12 0x00410540 (feb 0x0040EEE0)
    void ThrowBalloon();                               // v12 0x004109D0 the empty balloon, a "rigid" for 5 s
    static txmTEXTURE *pCloudTex;                      // v12 0x00640F18 "sfx_mdg_cloud"
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x00412170 (feb 0x004107C0)
};
int __fastcall pwpMDGInit();                           // v12 0x00410430 (feb 0x0040EDD0)
class pwpBOMB : public pwpWEAPON {                    // the bomb (grenade). size v12 0x1CB (feb 0x1AF); no own fields.
public:                                                // vtable v12 0x005F9BA0; src/Sources/plw_bmb.cpp
    virtual BOOL32 SetupAniSystem();                   // +0x40 v12 0x00406B80
    virtual int    Shoot(entENTITY *target, m3dV *aim); // +0x68 v12 0x00406F10 the throw asked for
    virtual void   ProcessSHOOT(void *msg);            // +0x98 v12 0x00406D20 the bomb thrown (server)
    virtual void   UpdateShootPosDir() {}              // +0x7C v12 0x00403A20 (empty: no "tip"; the throw has its own)
    virtual int    ProcessINIT();                      // +0x80 v12 0x00406C80
    virtual void   UpdateSound();                      // +0xAC v12 0x00406C50
    virtual void   Anim2Sound();                       // +0xB0 v12 0x00406C20
    static txmTEXTURE *pTexSmokeExpl, *pTexFlameExpl, *pTexGritExpl, *pTexScorchMark; // v12 0x00640348..0x00640354
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x004081B0 (feb 0x00407C60)
};
int __fastcall pwpBMBInit();                           // v12 0x00406930 (feb 0x00406410)
class pwpSTG_SPARKLE_1 : public partEMITTER_PHYS {    // vtable v12 0x005FACC0 (the base's slots)
public:
    pwpSTG_SPARKLE_1();                                // v12 0x00414DB0 (feb 0x00413470)
};
class pwpSTG_SPARKLE_2 : public partEMITTER_PHYS {    // vtable v12 0x005FACF0
public:
    pwpSTG_SPARKLE_2();                                // v12 0x00414F80 (feb 0x00413640)
};
class pwpSTG : public pwpBULLET_WPN {                 // the shotgun. size v12 0x1EB (feb 0x1CF). vtable v12 0x005FABB0
public:                                                // src/Sources/plw_stg.cpp (the shot's own parts not rebuilt)
    virtual int    ProcessINIT();                      // +0x80 v12 0x004145A0 (feb 0x00412C60)
    virtual void   ProcessTERM();                      // +0x84 v12 0x00414770
    virtual void   ProcessFRAME();                     // +0x88 v12 0x004146E0
    virtual void   UpdateSound();                      // +0xAC v12 0x00414550
    virtual void   Anim2Sound();                       // +0xB0 v12 0x00414500
    virtual sncSOUND_DESCR_3D *GetShellSoundDesc();    // +0xD0 v12 0x00414D90
    virtual void   PlayShootSound();                   // +0xF0 v12 0x00414D40
    virtual int    FillCDTShootList(m3dV *org, cdtINFO *aim, cdtINFO *hits, int max);   // +0xF8 v12 0x004147A0
    virtual float  GetRadiusNoSdrAround();             // +0x100 v12 0x00414DA0
    partEMITTER_PHYS *sparkle[2]; // v12 +0x1DF / +0x1E3 (feb +0x1C3 / +0x1C7) pwpSTG_SPARKLE_1 / _2 at the muzzle
    objOBJ *sparkSource;          // v12 +0x1E7 (feb +0x1CB) "spark_source": the sparkles' space
    static txmTEXTURE *pTexGlow;                       // v12 0x00641208 "sfx_egg_glow"
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x00415170 (feb 0x00413830)
};
int __fastcall pwpSTGInit();                           // v12 0x004144A0 (feb 0x00412B60)
class pwpMACHINEGUN : public pwpBULLET_WPN {          // the Tommy gun. size v12 0x1EF (feb 0x1D3). vtable v12 0x005FA5E0
public:                                                // src/Sources/plw_mcg.cpp (ShowSFX / FillCDTShootList not rebuilt)
    virtual int    ProcessINIT();                      // +0x80 v12 0x0040FC60
    virtual void   ProcessFRAME();                     // +0x88 v12 0x0040FD70
    virtual void   ProcessDESTROY_ADDR(void *addr);   // +0x94 v12 0x0040FDB0 (msg 3: forget the target)
    virtual void   ShowSFX(struct pwpMCG_RAY *r);      // +0xDC v12 0x0040FDD0
    virtual int    FillCDTShootList(m3dV *org, cdtINFO *aim, cdtINFO *hits, int max);   // +0xF8 v12 0x0040FDF0
    void  *target;                // v12 +0x1DF (feb +0x1C3) the entity aimed at // ? (cleared when it goes away)
    float  targetTimer;           // v12 +0x1E3 (feb +0x1C7) runs down while out; at 0 the target is forgotten
    int    targetSince;           // v12 +0x1E7 (feb +0x1CB) // ?
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x00410330 (feb 0x0040ECD0)
};
int __fastcall pwpMCGInit();                           // v12 0x0040FC30 (feb 0x0040E5D0)
class pwpMINIGUN : public pwpBULLET_WPN {             // size v12 0x1F3 (feb 0x1D3). vtable v12 0x005FAA70; src/Sources/plw_mng.cpp
public:                                                // spins up (START_SHOOT), fires 2 rays a shot, spins down (STOP_SHOOT)
    virtual BOOL32 SetupAniSystem();                   // +0x40 v12 0x00412FD0 13 sequences
    virtual int    SubtractAmmo(float n);              // +0x64 v12 0x004142C0 one round, whatever asked
    virtual int    IsReadyShoot();                     // +0x78 v12 0x00548680 (the shared "return 1")
    virtual int    ProcessINIT();                      // +0x80 v12 0x00413DC0
    virtual void   ProcessTERM();                      // +0x84 v12 0x00413ED0
    virtual void   ProcessFRAME();                     // +0x88 v12 0x00413F40
    virtual void   ProcessTERM_LEVEL();                // +0x90 v12 0x00413F20
    virtual void   ProcessMP_NOTIFY(void *data);       // +0x9C v12 0x00414000 a remote copy shooting or not
    virtual void   UpdateSound();                      // +0xAC v12 0x00413D00
    virtual void   Anim2Sound();                       // +0xB0 v12 0x00413C20
    virtual void   AdvanceAnimation(int seq, float dt);// +0xB8 v12 0x004135C0
    virtual int    GetCurAnimSeq();                    // +0xBC v12 0x00404DA0 (shared with the atomic gun)
    virtual BOOL32 IsShooting();                       // +0xC4 v12 0x00413600
    virtual BOOL32 IsReadyRecharge();                  // +0xC8 v12 0x00413630
    virtual void   StopSound();                        // +0xCC v12 0x00414050
    virtual float  GetScorchmarkScale();               // +0xD4 v12 0x00430B30 (shared) 1.5
    virtual void   ShowSFX(struct pwpMCG_RAY *r);      // +0xDC v12 0x00413570
    virtual void   PlayShootSound();                   // +0xF0 v12 0x00413660
    virtual int    FillCDTShootList(m3dV *org, cdtINFO *aim, cdtINFO *hits, int max);   // +0xF8 v12 0x00413700
    entENTITY *mngFlare;          // v12 +0x1DF (feb +0x1C3) "sfx_mng_flare" (CreateMngFlare)
    partEMITTER_PHYS *mngSmoke;   // v12 +0x1E3 (feb +0x1C7) pwpMNG_SMOKE, from the barrels ("smoke_source")
    objOBJ *smokeSource;          // v12 +0x1E7 (feb +0x1CB) "smoke_source"
    int    remoteShooting;        // v12 +0x1EB (feb +0x1CF) a remote copy: its owner shoots (ProcessMP_NOTIFY)
    objOBJ *shellSource;          // v12 +0x1EF (1.2) "shell_source" (pwpMNG_SHELL::PartUpdate)
    int  CreateMngFlare();                             // v12 0x00412CF0 (feb 0x00411350)
    int  CreateMngShells();                            // v12 0x00412E40 (feb 0x004114B0)
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x004142E0 (feb 0x00412980)
};
int __fastcall pwpMNGInit();                           // v12 0x00412330
class pwpSHOVEL : public pwpWEAPON {                   // vtable v12 0x005FADA0; src/Sources/plw_svl.cpp
public:
    virtual int  Shoot(entENTITY *target, m3dV *aim);  // +0x68 v12 0x004155A0 (feb 0x00413C10)
    virtual int  ProcessINIT();                        // +0x80 v12 0x004154A0 (feb 0x00413B10)
    virtual void ProcessFRAME();                       // +0x88 v12 0x00415560 (feb 0x00413BD0)
    virtual void ProcessSHOOT(void *msg);              // +0x98 v12 0x0040AC20 (folded with pwpBULLET_WPN's relay)
    virtual void ShootBody(void *msg);                 // +0xB4 v12 0x00415B00 (feb ProcessSHOOT 0x00414110)
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x00416E60 (feb 0x00415110)
};
int __fastcall pwpSVLInit();                           // v12 0x004152D0 (feb 0x00413970) "pwp_svl", "sfx_shovel"
/* pwpSHOVEL notes: */      // size v12 0x1CB (feb 0x1AF) (Create push 0x1CB at v12 0x00416E61). Melee:
                                                //   Shoot v12 0x004155A0 (feb 0x00413C10) does ScnShootRay + dmgCollectInst(1.5 m),
                                                //   damage applied in ShootBody v12 0x00415B00 (feb ProcessSHOOT 0x00414110)
class pwpCROSSBOW : public pwpWEAPON {                // size v12 0x1E7 (feb 0x1CB). vtable v12 0x005FA500
public:                                                // src/Sources/plw_crb.cpp (the shot and the bolt not rebuilt)
    virtual BOOL32 SetupAniSystem();                   // +0x40 v12 0x0040D370 11 sequences
    virtual int    ProcessINIT();                      // +0x80 v12 0x0040DC90
    virtual void   ProcessFRAME();                     // +0x88 v12 0x0040DEF0
    virtual void   UpdateSound();                      // +0xAC v12 0x0040DE60
    virtual void   Anim2Sound();                       // +0xB0 v12 0x0040DD90
    virtual int    GetCurAnimSeq();                    // +0xBC v12 0x0040E070 (folded with the colt's)
    int     loaded;               // v12 +0x1CB (feb +0x1AF) bolts gone from the bow (the shown ones from the end)
    objOBJ *arrow[6];             // v12 +0x1CF..+0x1E3 (feb +0x1B3..) "arrow01".."arrow06" on the bow
    void ThrowClip(int);                               // v12 0x0040E3B0 the empty clip, a "rigid" for 5 s
    static txmTEXTURE *pTexFlame, *pTexSparks, *pTexScorchMark; // v12 0x00640B78 / 0x00640B88 / 0x00640BC0
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x0040FBD0 (feb 0x0040E570)
};
int __fastcall pwpCRBInit();                           // v12 0x0040D190 (feb 0x0040C2D0)
class pwpACIDGUN    /* : pwpWEAPON */ {         // size v12 0x1E3 (feb 0x1C7) (Create push 0x1E3)
public:
    objOBJ *dropsSource;          // v12 +0x1CB (feb +0x1AF) "drops_source" // ?
    partEMITTER_PHYS *drops;      // v12 +0x1CF (feb +0x1B3) pwpACD_DROPS  // ?
    int    unk1D3, unk1D7;        // v12 +0x1D3/+0x1D7 (feb +0x1B7/+0x1BB) // ?
    float  clipDelay;             // v12 +0x1DB (feb +0x1BF) = 0.833333 (v12 ProcessFRAME 0x00401BCF "mov [edi+0x1DB],0x3F555555")
                                  //   feb set it in ProcessSHOOT // ? meaning (time until the empty canister is thrown)
    float  unk1DF;                // v12 +0x1DF (feb +0x1C3) init 0 // ?
};
class pwpATOMICGUN : public pwpWEAPON {           // size feb 0x1EB, v12 0x207 // ? by rule; src/Sources/plw_amg.cpp
public:
    int    chargeState;           // v12 +0x1CB (feb +0x1AF) 1 / 2 loading, 3 fire (ReleaseShot v12 0x00404E70; name ours)
    float  chargeA;               // v12 +0x1CF (feb +0x1B3) init 0.7 // ? (charge threshold: Shoot compares 0.7, scales 1/0.7)
    int    rocketIdx;             // v12 +0x1D3 (feb +0x1B7) "rocket%02d" // ?
    char   _1D7[0x1F7 - 0x1D7];
    float  unk1F7;                // v12 +0x1F7 (feb +0x1DB) init 0.5 // ?
    void ReleaseShot();           // v12 0x00404E70 (name ours) the fire key let go: a loading charge fires in 0.7 s
};
class pwpBASOOKA : public pwpWEAPON {                 // the rocket launcher. size v12 0x1D7 (feb 0x1BB). vtable v12 0x005F9DC0
public:                                                // src/Sources/plw_bsk.cpp
    virtual int    Shoot(entENTITY *target, m3dV *aim); // +0x68 v12 0x00408870 the shot asked for
    virtual void   ProcessSHOOT(void *msg);            // +0x98 v12 0x00408950 the rocket fired (server), the muzzle
    virtual int    ProcessINIT();                      // +0x80 v12 0x004084F0
    virtual void   ProcessTERM();                      // +0x84 v12 0x00408740
    virtual void   ProcessFRAME();                     // +0x88 v12 0x00408780
    class objMOD_TEX_FRAME_ANIM *trailFlame; // v12 +0x1CB (feb +0x1AF) the smoke's "&bb_&trl_flame" sheet
    entENTITY *smoke;             // v12 +0x1CF (feb +0x1B3) "sfx_bsk_smoke" at the muzzle
    objOBJ *rocketObj;            // v12 +0x1D3 (feb +0x1B7) "rocket" (hidden while not loaded)
    static txmTEXTURE *pTexExplos, *pTexScorchMark, *pTexSmoke, *pTexTrail, *pTexRipple; // v12 0x00640360..0x00640518
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x00409FA0 (feb 0x004095D0)
};
int __fastcall pwpBSKInit();                           // v12 0x00408370 (feb 0x00407E20)

// =================================================================================================================
// Key virtual implementations (v12 (feb)). Use these as hook targets; slot offsets in the vtable dumps below.
// =================================================================================================================
//  pwpWEAPON     ProcessINIT 0x00417D50 (0x00415970)  creates sndChannel (sncCHANNEL_CONE 10..50 m), hides hands for MP copies
//                ProcessFRAME 0x00417F50 (0x00415BB0) 1.2: also toggles model render (flag 0x1000000) by IsWeaponMP
//                ProcessMsg 0x00417FF0 (0x00415BD0)   1.2: msg 0x4C8 -> slot +0xB4 ShootBody
//                SubtractAmmo 0x00417680 (0x004152A0) GetDamage 0x004176F0 (0x00415310) SetShootRate 0x004176D0 (0x004152F0)
//                SetupAniSystem 0x00417920 (0x00415520) maps PWP_SEQ_* names to sequences
//  wpnWEAPON     Shoot 0x00543E20 (0x0052E370) MakeDamage 0x00543ED0 (0x0052E420) GetOrgDirTip 0x00544050 (0x0052E5A0)
//  BULLET_WPN    Shoot 0x0040A8B0 (0x00409EB0)  relay 0x0040AC20 (-)  ShootBody 0x0040AC80 (ProcessSHOOT 0x0040A280)
//                StartShells 0x0040B0A0 (0x0040A5A0)  CreateShell 0x0040A540 (0x00409B40)  ShowSFX(rec) 0x0040AE70 (0x0040A380)
//  SHOVEL        Shoot 0x004155A0 (0x00413C10)  ShootBody 0x00415B00 (ProcessSHOOT 0x00414110)  INIT 0x004154A0 (0x00413B10)
//  COLT          INIT 0x0040C800 (0x0040B940) FRAME 0x0040C940 (0x0040BA80) PlayShootSound 0x0040C750 (0x0040B890)
//  STG           INIT 0x004145A0 (0x00412C60) FRAME 0x004146E0 (0x00412DA0) FillCDTShootList 0x004147A0 (0x00412E60)
//  MACHINEGUN    INIT 0x0040FC60 (0x0040E600) FillCDTShootList 0x0040FDF0 (0x0040E790) ShowSFX 0x0040FDD0 (0x0040E770)
//  MINIGUN       see re/results/build_differences.md section 2 (full table)
//  CROSSBOW      Shoot 0x0040D7E0 (0x0040C910) ProcessSHOOT 0x0040D9C0 (0x0040CF40) ShowCollisSFX 0x0040E090 (0x0040D1F0)
//  ACIDGUN       Shoot 0x004016D0 (0x004016A0) ProcessSHOOT 0x004017D0 (0x00401950) FRAME 0x004019B0 (0x004017B0, 1.2 inlines ThrowClip)
//  ATOMICGUN     Shoot 0x00404440 (0x00404110) ProcessSHOOT 0x00404550 (0x00404820) FRAME 0x00404A90 (0x00404530)
//  BASOOKA       Shoot 0x00408870 (0x00408330) ProcessSHOOT 0x00408950 (0x00408420) FRAME 0x00408780 (0x00408240)
//  MEDUSA        Shoot 0x004106F0 (0x0040F090) ProcessSHOOT 0x004107D0 (0x0040F180)
//  BOMB          Shoot 0x00406F10 (0x004069D0) ProcessSHOOT 0x00406D20 (0x004067E0)

// =================================================================================================================
// VTABLES in slot order (generated by work/sdk_weapons/vtrule.py + genvt.py; v12 slot = feb slot mapped by the entity/
// weapon slot rule; every own-class slot checked by mnemonic similarity, see function_index.tsv). Slots < 25
// (entIACTIVE_OBJ/entENTITY) are in engine_core/entity.h. Names in [] are the feb map name of that slot.
// Slot rule feb->v12: 26..35 -> -1; 36..44 -> same; 45..49 -> +1; bullet-class slots 50,51 -> +2, 52.. -> +3.
// New in 1.2: +0x8C (35), +0xB4 (45), +0xCC (51); bullet +0xD8 (54).
// =================================================================================================================
// ---- pwpWEAPON vtable v12 0x005F9580 (feb 0x005CE820), 52 slots (feb 50). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL  [pwpWEAPON::SubtractAmmo]
//   slot +0x068 v12 0x00543E20 (feb 0x0052E370) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x00417D50 (feb 0x00415970) ProcessINIT() -> BOOL  [pwpWEAPON::ProcessINIT]
//   slot +0x084 v12 0x00401380 (feb 0x00403BE0) ProcessTERM()
//   slot +0x088 v12 0x00417F50 (feb 0x00415BB0) ProcessFRAME()  [pwpWEAPON::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL  [pwpWEAPON::ProcessINIT_LEVEL]
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x00403950 (feb 0x00403870) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)  [pwpWEAPON::SetShootRate]
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)  [pwpWEAPON::GetDamage]
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x004180A0 (feb 0x00415C60) UpdateSound()  [pwpWEAPON::UpdateSound]
//   slot +0x0B0 v12 0x00403A20 (feb 0x00403690) Anim2Sound()
//   slot +0x0B4 v12 0x00403950 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)  [pwpWEAPON::AdvanceAnimation]
//   slot +0x0BC v12 0x00403B40 (feb 0x00403850) GetCurAnimSeq() -> int  [pwpWEAPON::GetCurAnimSeq]
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)  [pwpWEAPON::IsWeaponMP]
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL  [pwpWEAPON::IsShooting]
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL  [pwpWEAPON::IsReadyRecharge]
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
// ---- pwpBULLET_WPN vtable v12 0x005F9FD0 (feb 0x005CF270), 65 slots (feb 62). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x0040A8B0 (feb 0x00409EB0) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)  [pwpBULLET_WPN::Shoot]
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x0040A3D0 (feb 0x004099C0) ProcessINIT() -> BOOL  [pwpBULLET_WPN::ProcessINIT]
//   slot +0x084 v12 0x0040A730 (feb 0x00409D30) ProcessTERM()  [pwpBULLET_WPN::ProcessTERM]
//   slot +0x088 v12 0x0040A6C0 (feb 0x00409CC0) ProcessFRAME()  [pwpBULLET_WPN::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x0040AC20 (feb 0x0040A280) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)  [pwpBULLET_WPN::ProcessSHOOT]
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x004180A0 (feb 0x00415C60) UpdateSound()
//   slot +0x0B0 v12 0x00403A20 (feb 0x00403690) Anim2Sound()
//   slot +0x0B4 v12 0x0040AC80 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x00403B40 (feb 0x00403850) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
//   slot +0x0D0 v12 0x0040A530 (feb 0x00409B30) GetShellSoundDesc() -> sncSOUND_DESCR_3D*  [pwpBULLET_WPN::GetShellSoundDesc]
//   slot +0x0D4 v12 0x00431E30 (feb 0x0042C490) GetScorchmarkScale() -> float  [pwpBULLET_WPN::GetScorchmarkScale]
//   slot +0x0D8 v12 0x0040ADC0 (feb --) 1.2 NEW (bullet): ShowShotSFX(msg) -- loops the 0x15-byte ray records of a shot msg, calls slot 55 for each
//   slot +0x0DC v12 0x0040AE70 (feb 0x0040A380) ShowSFX(pwpMCG_SHOOT *rec)  (hit/flare effects of one ray)  [pwpBULLET_WPN::ShowSFX]
//   slot +0x0E0 v12 0x0040A400 (feb 0x004099F0) CreateFlare(float)  [pwpBULLET_WPN::CreateFlare]
//   slot +0x0E4 v12 0x0040A540 (feb 0x00409B40) CreateShell(float,float,m3dSPL_LINEAR1D*)  [pwpBULLET_WPN::CreateShell]
//   slot +0x0E8 v12 0x0040A4E0 (feb 0x00409AE0) CreateSmoke(float)  [pwpBULLET_WPN::CreateSmoke]
//   slot +0x0EC v12 0x0040B0A0 (feb 0x0040A5A0) StartShells(int n)  [pwpBULLET_WPN::StartShells]
//   slot +0x0F0 v12 0x0040B080 (feb 0x0040A580) PlayShootSound()  [pwpBULLET_WPN::PlayShootSound]
//   slot +0x0F4 v12 0x00403860 (feb 0x0057BD10) (stub)
//   slot +0x0F8 v12 0x0040B340 (feb 0x0040A740) FillCDTShootList(m3dV*,cdtINFO*,m3dV*,int) -> int nRays  [pwpBULLET_WPN::FillCDTShootList]
//   slot +0x0FC v12 0x0040B360 (feb 0x0040A760) FillCDTShootListSpread(m3dV*,m3dV*,float,float,cdtINFO*,int) -> int  [pwpBULLET_WPN::FillCDTShootListSpread]
//   slot +0x100 v12 0x0040C580 (feb 0x0040B680) GetRadiusNoSdrAround() -> float  [pwpBULLET_WPN::GetRadiusNoSdrAround]
// ---- pwpSHOVEL vtable v12 0x005FADA0 (feb 0x005CFF90), 52 slots (feb 50). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x004155A0 (feb 0x00413C10) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)  [pwpSHOVEL::Shoot]
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x004154A0 (feb 0x00413B10) ProcessINIT() -> BOOL  [pwpSHOVEL::ProcessINIT]
//   slot +0x084 v12 0x00401380 (feb 0x00403BE0) ProcessTERM()
//   slot +0x088 v12 0x00415560 (feb 0x00413BD0) ProcessFRAME()  [pwpSHOVEL::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x0040AC20 (feb 0x00414110) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)  [pwpSHOVEL::ProcessSHOOT]
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x004180A0 (feb 0x00415C60) UpdateSound()
//   slot +0x0B0 v12 0x00403A20 (feb 0x00403690) Anim2Sound()
//   slot +0x0B4 v12 0x00415B00 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x00403B40 (feb 0x00403850) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
// ---- pwpCOLT vtable v12 0x005FA1D0 (feb 0x005CF458), 65 slots (feb 62). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x0040A8B0 (feb 0x00409EB0) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x0040C800 (feb 0x0040B940) ProcessINIT() -> BOOL  [pwpCOLT::ProcessINIT]
//   slot +0x084 v12 0x0040A730 (feb 0x00409D30) ProcessTERM()
//   slot +0x088 v12 0x0040C940 (feb 0x0040BA80) ProcessFRAME()  [pwpCOLT::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x0040AC20 (feb 0x0040A280) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x0040C7D0 (feb 0x0040B910) UpdateSound()  [pwpCOLT::UpdateSound]
//   slot +0x0B0 v12 0x0040C7A0 (feb 0x0040B8E0) Anim2Sound()  [pwpCOLT::Anim2Sound]
//   slot +0x0B4 v12 0x0040AC80 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x0040E070 (feb 0x0040BF30) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
//   slot +0x0D0 v12 0x0040C740 (feb 0x0040B880) GetShellSoundDesc() -> sncSOUND_DESCR_3D*  [pwpCOLT::GetShellSoundDesc]
//   slot +0x0D4 v12 0x00431E30 (feb 0x0042C490) GetScorchmarkScale() -> float
//   slot +0x0D8 v12 0x0040ADC0 (feb --) 1.2 NEW (bullet): ShowShotSFX(msg) -- loops the 0x15-byte ray records of a shot msg, calls slot 55 for each
//   slot +0x0DC v12 0x0040AE70 (feb 0x0040A380) ShowSFX(pwpMCG_SHOOT *rec)  (hit/flare effects of one ray)
//   slot +0x0E0 v12 0x0040A400 (feb 0x004099F0) CreateFlare(float)
//   slot +0x0E4 v12 0x0040A540 (feb 0x00409B40) CreateShell(float,float,m3dSPL_LINEAR1D*)
//   slot +0x0E8 v12 0x0040A4E0 (feb 0x00409AE0) CreateSmoke(float)
//   slot +0x0EC v12 0x0040B0A0 (feb 0x0040A5A0) StartShells(int n)
//   slot +0x0F0 v12 0x0040C750 (feb 0x0040B890) PlayShootSound()  [pwpCOLT::PlayShootSound]
//   slot +0x0F4 v12 0x00403860 (feb 0x0057BD10) (stub)
//   slot +0x0F8 v12 0x0040B340 (feb 0x0040A740) FillCDTShootList(m3dV*,cdtINFO*,m3dV*,int) -> int nRays
//   slot +0x0FC v12 0x0040B360 (feb 0x0040A760) FillCDTShootListSpread(m3dV*,m3dV*,float,float,cdtINFO*,int) -> int
//   slot +0x100 v12 0x0040C580 (feb 0x0040B680) GetRadiusNoSdrAround() -> float
// ---- pwpSTG vtable v12 0x005FABB0 (feb 0x005CFDA8), 65 slots (feb 62). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x0040A8B0 (feb 0x00409EB0) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x004145A0 (feb 0x00412C60) ProcessINIT() -> BOOL  [pwpSTG::ProcessINIT]
//   slot +0x084 v12 0x00414770 (feb 0x00412E30) ProcessTERM()  [pwpSTG::ProcessTERM]
//   slot +0x088 v12 0x004146E0 (feb 0x00412DA0) ProcessFRAME()  [pwpSTG::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x0040AC20 (feb 0x0040A280) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x00414550 (feb 0x00412C10) UpdateSound()  [pwpSTG::UpdateSound]
//   slot +0x0B0 v12 0x00414500 (feb 0x00412BC0) Anim2Sound()  [pwpSTG::Anim2Sound]
//   slot +0x0B4 v12 0x0040AC80 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x00403B40 (feb 0x00403850) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
//   slot +0x0D0 v12 0x00414D90 (feb 0x00413450) GetShellSoundDesc() -> sncSOUND_DESCR_3D*  [pwpSTG::GetShellSoundDesc]
//   slot +0x0D4 v12 0x00430B30 (feb 0x0042B4F0) GetScorchmarkScale() -> float
//   slot +0x0D8 v12 0x0040ADC0 (feb --) 1.2 NEW (bullet): ShowShotSFX(msg) -- loops the 0x15-byte ray records of a shot msg, calls slot 55 for each
//   slot +0x0DC v12 0x0040AE70 (feb 0x0040A380) ShowSFX(pwpMCG_SHOOT *rec)  (hit/flare effects of one ray)
//   slot +0x0E0 v12 0x0040A400 (feb 0x004099F0) CreateFlare(float)
//   slot +0x0E4 v12 0x0040A540 (feb 0x00409B40) CreateShell(float,float,m3dSPL_LINEAR1D*)
//   slot +0x0E8 v12 0x0040A4E0 (feb 0x00409AE0) CreateSmoke(float)
//   slot +0x0EC v12 0x0040B0A0 (feb 0x0040A5A0) StartShells(int n)
//   slot +0x0F0 v12 0x00414D40 (feb 0x00413400) PlayShootSound()  [pwpSTG::PlayShootSound]
//   slot +0x0F4 v12 0x00403860 (feb 0x0057BD10) (stub)
//   slot +0x0F8 v12 0x004147A0 (feb 0x00412E60) FillCDTShootList(m3dV*,cdtINFO*,m3dV*,int) -> int nRays  [pwpSTG::FillCDTShootList]
//   slot +0x0FC v12 0x0040B360 (feb 0x0040A760) FillCDTShootListSpread(m3dV*,m3dV*,float,float,cdtINFO*,int) -> int
//   slot +0x100 v12 0x00414DA0 (feb 0x00413460) GetRadiusNoSdrAround() -> float  [pwpSTG::GetRadiusNoSdrAround]
// ---- pwpMACHINEGUN vtable v12 0x005FA5E0 (feb 0x005CF830), 65 slots (feb 62). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x0040A8B0 (feb 0x00409EB0) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x0040FC60 (feb 0x0040E600) ProcessINIT() -> BOOL  [pwpMACHINEGUN::ProcessINIT]
//   slot +0x084 v12 0x0040A730 (feb 0x00409D30) ProcessTERM()
//   slot +0x088 v12 0x0040FD70 (feb 0x0040E710) ProcessFRAME()  [pwpMACHINEGUN::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x0040FDB0 (feb 0x0040E750) (empty stub, 1 arg)  [pwpMACHINEGUN::ProcessDESTROY_ENT]
//   slot +0x098 v12 0x0040AC20 (feb 0x0040A280) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x004180A0 (feb 0x00415C60) UpdateSound()
//   slot +0x0B0 v12 0x00403A20 (feb 0x00403690) Anim2Sound()
//   slot +0x0B4 v12 0x0040AC80 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x00403B40 (feb 0x00403850) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
//   slot +0x0D0 v12 0x0040A530 (feb 0x00409B30) GetShellSoundDesc() -> sncSOUND_DESCR_3D*
//   slot +0x0D4 v12 0x00431E30 (feb 0x0042C490) GetScorchmarkScale() -> float
//   slot +0x0D8 v12 0x0040ADC0 (feb --) 1.2 NEW (bullet): ShowShotSFX(msg) -- loops the 0x15-byte ray records of a shot msg, calls slot 55 for each
//   slot +0x0DC v12 0x0040FDD0 (feb 0x0040E770) ShowSFX(pwpMCG_SHOOT *rec)  (hit/flare effects of one ray)  [pwpMACHINEGUN::ShowSFX]
//   slot +0x0E0 v12 0x0040A400 (feb 0x004099F0) CreateFlare(float)
//   slot +0x0E4 v12 0x0040A540 (feb 0x00409B40) CreateShell(float,float,m3dSPL_LINEAR1D*)
//   slot +0x0E8 v12 0x0040A4E0 (feb 0x00409AE0) CreateSmoke(float)
//   slot +0x0EC v12 0x0040B0A0 (feb 0x0040A5A0) StartShells(int n)
//   slot +0x0F0 v12 0x0040B080 (feb 0x0040A580) PlayShootSound()
//   slot +0x0F4 v12 0x00403860 (feb 0x0057BD10) (stub)
//   slot +0x0F8 v12 0x0040FDF0 (feb 0x0040E790) FillCDTShootList(m3dV*,cdtINFO*,m3dV*,int) -> int nRays  [pwpMACHINEGUN::FillCDTShootList]
//   slot +0x0FC v12 0x0040B360 (feb 0x0040A760) FillCDTShootListSpread(m3dV*,m3dV*,float,float,cdtINFO*,int) -> int
//   slot +0x100 v12 0x0040C580 (feb 0x0040B680) GetRadiusNoSdrAround() -> float
// ---- pwpMINIGUN vtable v12 0x005FAA70 (feb 0x005CFC78), 65 slots (feb 62). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x004142C0 (feb 0x00412960) SubtractAmmo(float n) -> BOOL  [pwpMINIGUN::SubtractAmmo]
//   slot +0x068 v12 0x0040A8B0 (feb 0x00409EB0) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00548680 (feb 0x0049A5B0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x00413DC0 (feb 0x00412510) ProcessINIT() -> BOOL  [pwpMINIGUN::ProcessINIT]
//   slot +0x084 v12 0x00413ED0 (feb 0x00412620) ProcessTERM()  [pwpMINIGUN::ProcessTERM]
//   slot +0x088 v12 0x00413F40 (feb 0x00412670) ProcessFRAME()  [pwpMINIGUN::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00413F20 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x0040AC20 (feb 0x0040A280) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)
//   slot +0x09C v12 0x00414000 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x00413D00 (feb 0x00412450) UpdateSound()  [pwpMINIGUN::UpdateSound]
//   slot +0x0B0 v12 0x00413C20 (feb 0x004123C0) Anim2Sound()  [pwpMINIGUN::Anim2Sound]
//   slot +0x0B4 v12 0x0040AC80 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x004135C0 (feb 0x00411D80) AdvanceAnimation(int seq, float dt)  [pwpMINIGUN::AdvanceAnimation]
//   slot +0x0BC v12 0x00404DA0 (feb 0x00404A20) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00413600 (feb 0x00411DC0) IsShooting() -> BOOL  [pwpMINIGUN::IsShooting]
//   slot +0x0C8 v12 0x00413630 (feb 0x00411DF0) IsReadyRecharge() -> BOOL  [pwpMINIGUN::IsReadyRecharge]
//   slot +0x0CC v12 0x00414050 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
//   slot +0x0D0 v12 0x0040A530 (feb 0x00409B30) GetShellSoundDesc() -> sncSOUND_DESCR_3D*
//   slot +0x0D4 v12 0x00430B30 (feb 0x0042B4F0) GetScorchmarkScale() -> float  [pwpMINIGUN::GetScorchmarkScale]
//   slot +0x0D8 v12 0x0040ADC0 (feb --) 1.2 NEW (bullet): ShowShotSFX(msg) -- loops the 0x15-byte ray records of a shot msg, calls slot 55 for each
//   slot +0x0DC v12 0x00413570 (feb 0x00411D40) ShowSFX(pwpMCG_SHOOT *rec)  (hit/flare effects of one ray)  [pwpMINIGUN::ShowSFX]
//   slot +0x0E0 v12 0x0040A400 (feb 0x004099F0) CreateFlare(float)
//   slot +0x0E4 v12 0x0040A540 (feb 0x00409B40) CreateShell(float,float,m3dSPL_LINEAR1D*)
//   slot +0x0E8 v12 0x0040A4E0 (feb 0x00409AE0) CreateSmoke(float)
//   slot +0x0EC v12 0x0040B0A0 (feb 0x0040A5A0) StartShells(int n)
//   slot +0x0F0 v12 0x00413660 (feb 0x00411E20) PlayShootSound()  [pwpMINIGUN::PlayShootSound]
//   slot +0x0F4 v12 0x00403860 (feb 0x0057BD10) (stub)
//   slot +0x0F8 v12 0x00413700 (feb 0x00411E90) FillCDTShootList(m3dV*,cdtINFO*,m3dV*,int) -> int nRays  [pwpMINIGUN::FillCDTShootList]
//   slot +0x0FC v12 0x0040B360 (feb 0x0040A760) FillCDTShootListSpread(m3dV*,m3dV*,float,float,cdtINFO*,int) -> int
//   slot +0x100 v12 0x0040C580 (feb 0x0040B680) GetRadiusNoSdrAround() -> float
// ---- pwpCROSSBOW vtable v12 0x005FA500 (feb 0x005CF758), 52 slots (feb 50). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x0040D7E0 (feb 0x0040C910) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)  [pwpCROSSBOW::Shoot]
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x0040DC90 (feb 0x0040CB80) ProcessINIT() -> BOOL  [pwpCROSSBOW::ProcessINIT]
//   slot +0x084 v12 0x00401380 (feb 0x00403BE0) ProcessTERM()
//   slot +0x088 v12 0x0040DEF0 (feb 0x0040CDE0) ProcessFRAME()  [pwpCROSSBOW::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x0040D9C0 (feb 0x0040CF40) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)  [pwpCROSSBOW::ProcessSHOOT]
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x0040DE60 (feb 0x0040CD50) UpdateSound()  [pwpCROSSBOW::UpdateSound]
//   slot +0x0B0 v12 0x0040DD90 (feb 0x0040CC80) Anim2Sound()  [pwpCROSSBOW::Anim2Sound]
//   slot +0x0B4 v12 0x00403950 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x0040E070 (feb 0x0040BF30) GetCurAnimSeq() -> int  [pwpCROSSBOW::GetCurAnimSeq]
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
// ---- pwpACIDGUN vtable v12 0x005F94B0 (feb 0x005CE758), 52 slots (feb 50). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x004016D0 (feb 0x004016A0) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)  [pwpACIDGUN::Shoot]
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x00401450 (feb 0x00401410) ProcessINIT() -> BOOL  [pwpACIDGUN::ProcessINIT]
//   slot +0x084 v12 0x00401660 (feb 0x00401630) ProcessTERM()  [pwpACIDGUN::ProcessTERM]
//   slot +0x088 v12 0x004019B0 (feb 0x004017B0) ProcessFRAME()  [pwpACIDGUN::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x004017D0 (feb 0x00401950) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)  [pwpACIDGUN::ProcessSHOOT]
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x004013E0 (feb 0x004013A0) UpdateSound()  [pwpACIDGUN::UpdateSound]
//   slot +0x0B0 v12 0x00401390 (feb 0x00401350) Anim2Sound()  [pwpACIDGUN::Anim2Sound]
//   slot +0x0B4 v12 0x00403950 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x00403B40 (feb 0x00403850) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
// ---- pwpATOMICGUN vtable v12 0x005F98E0 (feb 0x005CEBC0), 52 slots (feb 50). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x00404440 (feb 0x00404110) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)  [pwpATOMICGUN::Shoot]
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x004031F0 (feb 0x004036A0) IsReadyShoot() -> BOOL  [pwpATOMICGUN::IsReadyShoot]
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x004048F0 (feb 0x00404390) ProcessINIT() -> BOOL  [pwpATOMICGUN::ProcessINIT]
//   slot +0x084 v12 0x00404A60 (feb 0x00404500) ProcessTERM()  [pwpATOMICGUN::ProcessTERM]
//   slot +0x088 v12 0x00404A90 (feb 0x00404530) ProcessFRAME()  [pwpATOMICGUN::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x00404550 (feb 0x00404820) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)  [pwpATOMICGUN::ProcessSHOOT]
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00406440 (feb 0x00405F70) Enable(BOOL)  [pwpATOMICGUN::Enable]
//   slot +0x0AC v12 0x00404860 (feb 0x00404300) UpdateSound()  [pwpATOMICGUN::UpdateSound]
//   slot +0x0B0 v12 0x004047D0 (feb 0x00404270) Anim2Sound()  [pwpATOMICGUN::Anim2Sound]
//   slot +0x0B4 v12 0x00403950 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00404DD0 (feb 0x00404A50) AdvanceAnimation(int seq, float dt)  [pwpATOMICGUN::AdvanceAnimation]
//   slot +0x0BC v12 0x00404DA0 (feb 0x00404A20) GetCurAnimSeq() -> int  [pwpATOMICGUN::GetCurAnimSeq]
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x00406470 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
// ---- pwpBASOOKA vtable v12 0x005F9DC0 (feb 0x005CF0A0), 52 slots (feb 50). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x00408870 (feb 0x00408330) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)  [pwpBASOOKA::Shoot]
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x004084F0 (feb 0x00407FA0) ProcessINIT() -> BOOL  [pwpBASOOKA::ProcessINIT]
//   slot +0x084 v12 0x00408740 (feb 0x00408200) ProcessTERM()  [pwpBASOOKA::ProcessTERM]
//   slot +0x088 v12 0x00408780 (feb 0x00408240) ProcessFRAME()  [pwpBASOOKA::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x00408950 (feb 0x00408420) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)  [pwpBASOOKA::ProcessSHOOT]
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x004180A0 (feb 0x00415C60) UpdateSound()
//   slot +0x0B0 v12 0x00403A20 (feb 0x00403690) Anim2Sound()
//   slot +0x0B4 v12 0x00403950 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x00403B40 (feb 0x00403850) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
// ---- pwpMEDUSA vtable v12 0x005FA850 (feb 0x005CFA70), 52 slots (feb 50). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x004106F0 (feb 0x0040F090) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)  [pwpMEDUSA::Shoot]
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir()
//   slot +0x080 v12 0x004105F0 (feb 0x0040EF90) ProcessINIT() -> BOOL  [pwpMEDUSA::ProcessINIT]
//   slot +0x084 v12 0x00401380 (feb 0x00403BE0) ProcessTERM()
//   slot +0x088 v12 0x004106B0 (feb 0x0040F050) ProcessFRAME()  [pwpMEDUSA::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x004107D0 (feb 0x0040F180) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)  [pwpMEDUSA::ProcessSHOOT]
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x004105A0 (feb 0x0040EF40) UpdateSound()  [pwpMEDUSA::UpdateSound]
//   slot +0x0B0 v12 0x00410540 (feb 0x0040EEE0) Anim2Sound()  [pwpMEDUSA::Anim2Sound]
//   slot +0x0B4 v12 0x00403950 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x00403B40 (feb 0x00403850) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)
// ---- pwpBOMB vtable v12 0x005F9BA0 (feb 0x005CEEB0), 52 slots (feb 50). Slots < 25 = entENTITY (engine_core/entity.h).
//   slot +0x064 v12 0x00417680 (feb 0x004152A0) SubtractAmmo(float n) -> BOOL
//   slot +0x068 v12 0x00406F10 (feb 0x004069D0) Shoot(entENTITY *target, m3dV *aim) -> BOOL  (client side: start a shot)  [pwpBOMB::Shoot]
//   slot +0x06C v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x070 v12 0x00403A20 (feb 0x00403690) (empty stub)
//   slot +0x074 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)
//   slot +0x078 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot() -> BOOL
//   slot +0x07C v12 0x00403A20 (feb 0x00403690) UpdateShootPosDir()
//   slot +0x080 v12 0x00406C80 (feb 0x00406740) ProcessINIT() -> BOOL  [pwpBOMB::ProcessINIT]
//   slot +0x084 v12 0x00401380 (feb 0x00403BE0) ProcessTERM()
//   slot +0x088 v12 0x00406D10 (feb 0x004067D0) ProcessFRAME()  [pwpBOMB::ProcessFRAME]
//   slot +0x08C v12 0x00548680 (feb --) 1.2 NEW: returns 1 (base 0x00548680)  // ? meaning
//   slot +0x090 v12 0x00417F40 (feb 0x00415B60) ProcessINIT_LEVEL() -> BOOL
//   slot +0x094 v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg)
//   slot +0x098 v12 0x00406D20 (feb 0x004067E0) ProcessSHOOT(void *shotMsg)  (msg 0x4C7 handler)  [pwpBOMB::ProcessSHOOT]
//   slot +0x09C v12 0x00403950 (feb 0x00403870) (empty stub, 1 arg) / minigun: net shooting-state
//   slot +0x0A0 v12 0x004176D0 (feb 0x004152F0) SetShootRate(float shotsPerSec)
//   slot +0x0A4 v12 0x004176F0 (feb 0x00415310) GetDamage() -> float  (x4 with Titan powerup)
//   slot +0x0A8 v12 0x00403AE0 (feb 0x004037F0) Enable(BOOL)
//   slot +0x0AC v12 0x00406C50 (feb 0x00406710) UpdateSound()  [pwpBOMB::UpdateSound]
//   slot +0x0B0 v12 0x00406C20 (feb 0x004066E0) Anim2Sound()  [pwpBOMB::Anim2Sound]
//   slot +0x0B4 v12 0x00403950 (feb --) 1.2 NEW: ShootBody(void *msg) -- real shot handler reached through msg 0x4C8 (pwpWEAPON::ProcessMsg); bullet weapons/shovel
//   slot +0x0B8 v12 0x00403B10 (feb 0x00403820) AdvanceAnimation(int seq, float dt)
//   slot +0x0BC v12 0x00403B40 (feb 0x00403850) GetCurAnimSeq() -> int
//   slot +0x0C0 v12 0x00403B50 (feb 0x00403860) IsWeaponMP() -> BOOL (remote player weapon)
//   slot +0x0C4 v12 0x00418020 (feb 0x00415BE0) IsShooting() -> BOOL
//   slot +0x0C8 v12 0x00418050 (feb 0x00415C10) IsReadyRecharge() -> BOOL
//   slot +0x0CC v12 0x004180F0 (feb --) 1.2 NEW: sndChannel->vslot3()  // ? (stop weapon sound)

#pragma pack(pop)
