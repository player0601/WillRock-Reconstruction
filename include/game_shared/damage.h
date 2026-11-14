// damage.h -- Will Rock 1.2 unofficial SDK, game_shared: damage messages and explosion damage (gs_shared:damage.obj)
//
// Assumes from re/sdk/core (engine_core): msgSYSTEM msgSystem, msgADDR, msgDATA (4-byte header), msgDATA_PACK,
//   fioFILE_MEM, m3dV, animINST, entENTITY, cdtINFO, cdtREFINE / entCDT_REFINE_EXCL, gsMP_SYSTEM *gsSysMP.
// Convention: "// v12 +0xNN (feb +0xNN)" fields, "// v12 0x00XXXXXX (feb 0x00XXXXXX)" code/data. "// ?" = not
// confirmed by a v12 instruction. All structs are #pragma pack(1) (the game was built packed).
//
// Damage is delivered as a MESSAGE: SendMsg(target, GS_MSG_DAMAGE, dmg, attacker). Every entity that can be hurt
// handles that id in ProcessMsg and usually forwards to its vslot ProcessCDT_FIREABLE(dmgDAMAGE*).
// 1.2 changed the payload layout (see dmgDAMAGE) -- old feb-based mods that read +8 / +0xC are wrong on 1.2.
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#pragma pack(push, 1)

// ---- message id -------------------------------------------------------------------------------------------
#define GS_MSG_DAMAGE   0x4C6   // v12 (feb 0x4C5). v12 dmgDISPATCHER::Dispatch 0x005463E0 "push 0x4C6"; verified.

// ---- damage types (dmgDAMAGE::type) -------------------------------------------------------------------------
// v12 values = feb value + 1 (a new type 0 was inserted). Evidence: re/results/weapon_npc.json (OFS_DMG_TYPE).
enum dmgTYPE_V12 {
    DMG_SPECIAL     = 0,  // new in retail; uses subtype (+5). subtype 2 = "die of inactivity" kill (aiNPC::DieOfInactivity)
    DMG_WEAPON      = 1,  // dmgWEAPON         (feb 0)  v12 ctor 0x00546D00 "mov byte [eax+4],1"  verified
    DMG_GENERAL     = 2,  // dmgGENERAL/explos (feb 1)  v12 ctor 0x005461F0 "mov byte [eax+4],2"  verified
    DMG_TYPE3       = 3,  // (feb 2) ?
    DMG_NPC_MELEE   = 4,  // (feb 3) aiNPC::ProcessCDT_FIREABLE special case // ? name
    DMG_NO_ARMOUR   = 5,  // (feb 4) player armour bypass in plrPLAYER_COMMON::ProcessCDT_FIREABLE // ? name
};

// ---- dmgDAMAGE: base payload ---------------------------------------------------------------------------------
// v12 size 0x0E (feb 0x10). ctor v12 0x00425E00 (feb 0x0052F190): byte[0]=0, byte[1]=0x40, word[2]=size, type, sub=0.
struct dmgDAMAGE /* : msgDATA */ {
    unsigned char  msgId;      // v12 +0x00 (feb +0x00) msgDATA registration id (dmgWEAPON: from global dmgMsgDataId)
    unsigned char  msgFlags;   // v12 +0x01 (feb +0x01) 0x40 = stack/local data
    unsigned short size;       // v12 +0x02 (feb +0x02) 0x0E dmgDAMAGE, 0x1A dmgGENERAL, 0x30 dmgWEAPON (feb 0x10/0x1C/0x32)
    unsigned char  type;       // v12 +0x04 (feb +0x04 as int) dmgTYPE_V12
    unsigned char  subtype;    // v12 +0x05 (feb: none) 0 normally
    float          amount;     // v12 +0x06 (feb +0x08) hit points. wpnWEAPON::MakeDamage "fstp [eax+6]" v12 0x00543EE2
    unsigned int   sourceId;   // v12 +0x0A (feb +0x0C) MP entity id of attacker (entENTITY id), -1 = none
    dmgDAMAGE() {}
    dmgDAMAGE(int t);          // v12 0x00425E00 (feb 0x0052F190) inline, emitted in player.obj
};

// ---- dmgGENERAL: generic / explosion damage -----------------------------------------------------------------
// v12 size 0x1A (feb 0x1C). ctor v12 0x005461F0 (feb 0x0052E690): type 2, sourceId -1.
struct dmgGENERAL : dmgDAMAGE {
    m3dV           pos;        // v12 +0x0E (feb +0x10) explosion centre (set by dmgDispatchExplosDamageInst) // ? v12 offset by -2 rule
    dmgGENERAL();              // v12 0x005461F0 (feb 0x0052E690) src/gs_shared/damage.cpp
};

// ---- dmgWEAPON: weapon hit ----------------------------------------------------------------------------------
// v12 size 0x30 (feb 0x32). ctor v12 0x00546D00 (feb 0x0052F0C0). Filled by wpnWEAPON::MakeDamage v12 0x00543ED0.
struct dmgWEAPON : dmgDAMAGE {
    m3dV           hitPos;     // v12 +0x0E (feb +0x10) MakeDamage "lea edx,[eax+0xE]" verified
    int            weaponType; // v12 +0x1A (feb +0x1C) = wpnWEAPON::type (+0x98); -1 = explosion. verified
    m3dV           origin;     // v12 +0x1E (feb +0x20) muzzle/tip origin (objOBJ::GetOrigin of weapon tip). verified
    int            unk2A;      // v12 +0x2A (feb +0x2C) 0 in ctor // ?
    short          unk2E;      // v12 +0x2E (feb +0x30) -1 in ctor // ? (hit bone / part index?)
    dmgWEAPON();               // v12 0x00546D00 (feb 0x0052F0C0) src/gs_shared/damage.cpp
};

// MP serialisation of dmgWEAPON (msgDATA_PACK). vtable feb 0x005DD1F8, v12 ? -- registered by dmgInit.
struct dmgWEAPON_PACK /* : msgDATA_PACK */ {
    int  Pack(fioFILE_MEM *f, msgDATA *d);     // v12 ? (feb 0x0052F0F0)
    msgDATA *UnPack(fioFILE_MEM *f);           // v12 ? (feb 0x0052F140)
};

// ---- dmgDISPATCHER: optional redirect of explosion damage -----------------------------------------------------
// One virtual: Dispatch(target, dmg, attacker). Default sends GS_MSG_DAMAGE. Subclasses: aiDMG_DISPATCHER_TO_SIBLING,
// wrsfxDESTROY_BARREL_DMG_DISPATCHER (barrels pass damage to neighbours).
struct dmgDISPATCHER {
    virtual void Dispatch(entENTITY *target, dmgDAMAGE *d, entENTITY *attacker); // slot +0x00 v12 0x005463E0 (feb 0x0052E870)
};

// ---- globals --------------------------------------------------------------------------------------------------
extern unsigned char dmgMsgDataId;  // v12 0x009A84CC (feb 0x0095D620) msgDATA id stamped into dmgWEAPON byte 0 (verified, ctor)
extern char          dmgMsgDataReg[]; // v12 0x009A84C8 (feb 0x0095D61C) descriptor passed to msgSYSTEM::RegisterMsgData by dmgInit

// ---- functions (all __fastcall unless noted) -----------------------------------------------------------------
int  __fastcall dmgInit(void);                                                    // v12 0x005461D0 (feb 0x0052E670) register dmgWEAPON msg data
// Collect animINSTs within radius of pos into out[], max entries (v12 caller passes 0x32). Uses scnSCENE::MakeAnimList.
void __fastcall dmgCollectInst(m3dV *pos, float radius, animINST **out, int *n, int max); // v12 0x00546210 (feb 0x0052E6B0)
// Radial damage. radius=outer radius, dmg=damage at centre. isGeneral!=0 -> dmgGENERAL else dmgWEAPON(type explosion).
// 1.2: returns immediately when gsSysMP->mode == 3 (MP client); only server/single compute explosion damage.
void __fastcall dmgDispatchExplosDamage(entENTITY *attacker, m3dV *pos, float dmg, float radius, int isGeneral,
                                        int weaponType, cdtINFO *hit, unsigned long srcId, dmgDISPATCHER *disp); // v12 0x00546410 (feb 0x0052E8A0)
// Per-instance part: falloff + line of sight + send. srcId 0xFFFFFFFF = take attacker id when IsRegularMPID.
void __fastcall dmgDispatchExplosDamageInst(animINST *inst, entENTITY *attacker, m3dV *pos, float dmg, float radius,
                                            int isGeneral, int weaponType, cdtINFO *hit, unsigned long srcId,
                                            dmgDISPATCHER *disp);                  // v12 0x005464C0 (feb 0x0052E930)
int  __fastcall dmgIsHiddenObstacles(animINST *inst, entENTITY *attacker, m3dV *pos); // v12 0x00546930 (feb 0x0052ED40) verified by prologue (entCDT_REFINE_EXCL 6/7 setup); also reads the 1.5 eye height at 0x00546991
int  __fastcall dmgGetOrgDir(dmgDAMAGE *d, m3dV *org, m3dV *dir, m3dV *hit, float *dist); // v12 0x00546A20 (feb 0x0052EDE0) verified: movsx byte type at +4, switch 0..5 (feb dword, 0..4) origin/dir of a hit for blood/impulse

// Explosion falloff (feb decompile, same constants used by v12 0x005466FE / 0x00546725):
//   inner = radius * (1/3);  dist <= inner -> dmg;  dist >= radius -> dmg * 0.25;  linear in between.
//   Hit by direct collision (hit->flags&2 && hit->inst==inst) -> full dmg.
//   LOS: unless target entity flags2 (v12 +0xB8) & 0x800, scnSCENE::IsVisiblePoint from pos+1.5Y to target centre
//   (1.2 tries a second ray at +1.0Y before giving up). Constants are pooled __real@ shared with many sites: patch
//   the fmul/fadd operand (site+2), never the constant itself.

#pragma pack(pop)
