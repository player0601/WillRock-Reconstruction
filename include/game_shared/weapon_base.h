// weapon_base.h -- Will Rock 1.2 unofficial SDK, game_shared: wpnWEAPON (gs_shared:weapon.obj), plgsACTION note,
//   and the projectile template pjlPJL<SFX> (header-only, instantiated in weapon/AI objs).
// Assumes from re/sdk/core: entENTITY (v12 size 0x16B), objOBJ, m3dV, animINST, physPHYS. dmgWEAPON from damage.h.
// Concrete player weapons (pwp*) are the player/weapons subsystem; per-weapon tweak values live in
// re/results/weapon_npc.json (verified there; repeated here only where wpnWEAPON itself defines them).
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#pragma pack(push, 1)

// =================================================================================================================
// wpnWEAPON -- base of every player/NPC weapon. Ammo, fire rate, damage, shoot position/direction.
// ctor v12 0x00543CC0 (feb 0x0052E210). Not created by name (no entRegisterClass); subclasses allocate it.
// =================================================================================================================
enum wpnFLAGS {
    WPNF_ENABLED       = 0x01, // set in ctor
    WPNF_INFINITE_AMMO = 0x04, // Shoot() ignores ammo == 0 // name from use in Shoot
};
class wpnWEAPON : public entENTITY {
public:
    // entENTITY +0x98 (feb +0x94): weapon type index (OFS_WPN_TYPE) -> dmgWEAPON::weaponType. verified MakeDamage
    unsigned int flags;        // v12 +0x16B (feb +0x14F) wpnFLAGS, ctor 1
    float  shootRate;          // v12 +0x16F (feb +0x153) shots/s, ctor 1.0; cooldown = 1/rate
    float  maxAmmo;            // v12 +0x173 (feb +0x157) ctor 100.0. verified "mov [esi+0x173],0x42C80000" v12 0x00543CE6
    float  damage;             // v12 +0x177 (feb +0x15B) ctor 10.0. verified v12 0x00543CD3
    float  cooldown;           // v12 +0x17B (feb +0x15F) time to next shot
    m3dV   shootPos;           // v12 +0x17F (feb +0x163) // ? v12 by rule
    m3dV   shootDir;           // v12 +0x18B (feb +0x16F) normalised (Shoot computes target - shootPos) // ? v12 by rule
    objOBJ *tip;               // v12 +0x197 (feb +0x17B) "tip" object of the model. verified MakeDamage v12 0x00543F17
    float  ammo;               // v12 +0x19B (feb +0x17F) ctor 10.0. verified v12 0x00543CD9
    // entIACTIVE_OBJ +0x1C priority = 0x4B (ctor). size feb 0x183 (highest touched), v12 0x19F // ?

    // vtable v12 0x00608620 (feb 0x005DD148). One slot removed before 25 and one added after 35 vs feb.
    // slot +0x04 v12 0x00543FA0 (feb 0x0052E500) ProcessMsg
    // slot +0x64 v12 0x00543DD0 (feb 0x0052E320) SubtractAmmo(float) -> int
    // slot +0x68 v12 0x00543E20 (feb 0x0052E370) Shoot(entENTITY *target, m3dV *aim)  (VS_WPN_SHOOTFN 26)
    // slot +0x74 v12 0x00543ED0 (feb 0x0052E420) MakeDamage(dmgWEAPON*, m3dV *hit)  verified
    // slot +0x78 v12 0x00403AD0 (feb 0x004037E0) IsReadyShoot
    // slot +0x7C v12 0x00544090 (feb 0x0052E5E0) UpdateShootPosDir
    // slot +0x80 v12 0x00543F30 (feb 0x0052E480) ProcessINIT
    // slot +0x88 v12 0x00543F60 (feb 0x0052E4B0) ProcessFRAME (VS_WPN_FRAME 34)
    // slot +0x98 shared relay / ProcessSHOOT (VS_WPN_SHOOT 38) -- see re/results/weapon_npc.json notes
    // slot +0xA0 v12 0x005440E0 (feb 0x0052E630) SetShootRate(float)
    // slot +0xA4 v12 0x005440F0 (feb 0x0052E640) GetDamage() -> float  HOOK: global damage scale
    // slot +0xA8 v12 0x00403AE0 (feb 0x004037F0) Enable(int)
    // ---- virtuals, v12 slot order (rebuilt in src/gs_shared/weapon.cpp) -----------------------------------------
    wpnWEAPON();                                                       // v12 0x00543CC0 (feb 0x0052E210)
    virtual ~wpnWEAPON();                                              // slot +0x00 v12 0x00543D20 / 0x00543D50
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // slot +0x04 v12 0x00543FA0
    virtual int   SubtractAmmo(float a);                               // slot +0x64 v12 0x00543DD0
    virtual int   Shoot(entENTITY *target, m3dV *aim);                 // slot +0x68 v12 0x00543E20
    virtual void  _slot6C();                                           // slot +0x6C v12 0x00403A20 (empty)
    virtual void  _slot70();                                           // slot +0x70 v12 0x00403A20 (empty)
    virtual void  MakeDamage(dmgWEAPON *d, m3dV *hit);                 // slot +0x74 v12 0x00543ED0
    virtual int   IsReadyShoot();                                      // slot +0x78 v12 0x00403AD0
    virtual void  UpdateShootPosDir();                                 // slot +0x7C v12 0x00544090
    virtual int   ProcessINIT();                                       // slot +0x80 v12 0x00543F30 (msg 1)
    virtual void  ProcessTERM();                                       // slot +0x84 v12 0x00401380 (msg 2, empty)
    virtual void  ProcessFRAME();                                      // slot +0x88 v12 0x00543F60 (msg 1000)
    virtual int   ProcessINIT_LEVEL();                                 // slot +0x8C v12 0x004031F0 (msg 0x3F4, returns 1)
    virtual void  ProcessTERM_LEVEL();                                 // slot +0x90 v12 0x00403A20 (msg 0x3F5, empty)
    virtual void  ProcessDESTROY_ADDR(void *addr);                     // slot +0x94 v12 0x00403950 (msg 3, empty)
    virtual void  ProcessSHOOT(void *data);                            // slot +0x98 v12 0x00403950 (msg 0x4C7, empty here)
    virtual void  ProcessMP_NOTIFY(void *data);                        // slot +0x9C v12 0x00403950 (msg 0x40E, empty)
    virtual void  SetShootRate(float r);                               // slot +0xA0 v12 0x005440E0
    virtual float GetDamage();                                         // slot +0xA4 v12 0x005440F0
    virtual void  Enable(int on);                                      // slot +0xA8 v12 0x00403AE0
    void  SetAmmo(float a);                          // v12 0x00543D60 (feb 0x0052E2B0) verified: mov [ecx+0x19B],eax
    int   AddAmmo(float a);                          // v12 0x00543D70 (feb 0x0052E2C0) verified: test [ecx+0x16B],4 clamps to maxAmmo
    void  GetOrgDirTip(m3dV *org, m3dV *dir);        // v12 0x00544050 (feb 0x0052E5A0)
};

// =================================================================================================================
// plgsACTION -- "player in game-shared": the base class the shared objects talk to (items, doors, buttons, AI).
// Owned by the player subsystem (plrPLAYER derives from it). Only what game_shared calls:
//   vslot GetPos (feb +0x68 -> v12 +0x64, verified via itemITEM::IsCanPick v12 0x0055736C "call [edx+0x64]")
//   vslot GetDir (feb +0x70) // ?
//   accept-pickup vslot (feb +0x9C) called by itemITEM::ProcessFRAME // ? v12 slot
//   entIACTIVE_OBJ +0x90 player network id (door sends notify to it) // ?
extern plgsACTION *plgsPlayer;   // v12 ? (feb 0x0095D630) local player
// feb functions (v12 twins unverified): ctor 0x0052F1C0, ProcessINIT_LEVEL 0x0052F230 (v12 0x00546E70, table "vtable"),
// UpdateBreadCrumbs 0x0052F320 (path trail used by AI to follow the player), GetBreadCrumb 0x0052F610,
// entENTITY::GetDistToClosestPlayer 0x0052FAF0.

// =================================================================================================================
// Projectiles: template pjlPJL<SFX> : physPHYS (header-only, one instantiation per projectile type, e.g.
//   pjlPJL<pwpBSK_PJL_SFX> rocket, pjlPJL<pjlMEDUSA_SFX>, pjlPJL<pjlSPEAR_SFX>, ...; sizes 0x16B..0x184 in feb).
// Virtuals: ProcessEV_CDT(pjlEV_CDT*) (collision -> damage/explosion), ShowCollisSFX(pjlEV_CDT*) (default empty
//   v12 0x00403950 / feb 0x00403870), Terminate(). SFX::StartSFX(physPHYS*, pjlPJL_INIT*) attaches the trail/particles.
// pjlPJL_INIT grew to 0x55 bytes in 1.2 (re/results/weapon_npc.json). Explosive projectiles call
//   dmgDispatchExplosDamage (damage.h). Per-projectile layouts are documented by the weapons/AI subsystems. // ?
// =================================================================================================================

#pragma pack(pop)
