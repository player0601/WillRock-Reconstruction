// player_comm.cpp -- shared part of the local and network players (Will Rock 1.2, v12.exe; feb obj player_comm.obj)
//
// Rebuilt so far: ProcessMsg, ProcessSHOOT / _BODY (the shot relays), SetCurWpn, IsCanBuyPowerup, IsActivePowerup.

#include "game.h"
#include "../gs/gs_mp_server.h"   // damage: SendMsgClientAll, FindMPID
#include "../wr_vcall.h"
#include "mp_messages.h"      // gsMSG_MP_SHOW_MSG (the kill message)
#include "../../include/sfx_sob/sfx_api.h"   // wrsfxStartBlood, wrsfxStartChunkMeat

static void *_plrCfgDecompose4;                         // v12 0x00641C30 the cfg cache of [Debug] Decompose4

// v12 0x00426E60  (feb 0x00423400)
// The messages to the vtable slots: INIT, TERM, FRAME (not past frame 10 with [Debug] Decompose4), the game / map /
// level messages, the MP ones, damage (0x4C6), the shot relays (0x4C7 / 0x4C8), pick-ups (0x4C9); 0x415 asks for
// the position (the remembered one with flags2 0x100).
BOOL32 plrPLAYER_COMMON::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 1:     wrEVCall<void>(this, 0x7C); return 0;                // ProcessINIT
    case 2:     wrEVCall<void>(this, 0xAC); return 0;                // ProcessTERM
    case 1000: {
        int on;
        if (_apCfgReadBool(&_plrCfgDecompose4, (char *)"Debug", (char *)"Decompose4", &on) && on &&
            WR_FIELD(int, &msgSystem, 0x368) > 10)                   // the frame count
            return 0;
        wrEVCall<void>(this, 0x84);                                  // ProcessFRAME
        return 0;
    }
    case 0x3EF: wrEVCall<void>(this, 0xBC); return 0;                // ProcessINIT_GAME
    case 0x3F0: wrEVCall<void>(this, 0xC0); return 0;
    case 0x3F1: wrEVCall<void>(this, 0xB4); return 0;                // ProcessINIT_MAP
    case 0x3F3: wrEVCall<void>(this, 0xB8); return 0;
    case 0x3F4: wrEVCall<void>(this, 0x80); return 0;                // ProcessINIT_LEVEL
    case 0x3F5: wrEVCall<void>(this, 0xB0); return 0;
    case 0x406: wrEVCall<void>(this, 0xD4, data); return 0;
    case 0x408: wrEVCall<void>(this, 0xD8, data); return 0;
    case 0x40E: wrEVCall<void>(this, 0x8C, data); return 0;          // ProcessMP_NOTIFY
    case 0x415:
        if (flags2 & 0x100)
            *(m3dV *)data = *(m3dV *)((char *)this + 0x1E7);
        else
            inst->GetPos((m3dV *)data);
        return 0;
    case 0x4C6: wrEVCall<void>(this, 0xC8, data); return 0;          // ProcessCDT_FIREABLE
    case 0x4C7: wrEVCall<void>(this, 0xCC, data); return 0;          // ProcessSHOOT
    case 0x4C8: wrEVCall<void>(this, 0xD0, data); return 0;          // ProcessSHOOT_BODY
    case 0x4C9: wrEVCall<void>(this, 0xC4, data); return 0;          // ProcessPICK
    }
    return 0;
}

// v12 0x00428150
// A shot message (the weapon's index at +4) goes on to that weapon; the server counts the shot.
void plrPLAYER_COMMON::ProcessSHOOT(void *msg)
{
    u8 i = ((u8 *)msg)[4];
    if (i < PLR_WPN_COUNT && weapons[i])
        msgSystem.SendMsg(weapons[i], 0x4C7, msg, 0);
    if ((entFlags & 4) && wrSysStat)
        wrSysStat->Notify_WpnShot(this, weapons[((u8 *)msg)[4]]);
}

// v12 0x004281C0
void plrPLAYER_COMMON::ProcessSHOOT_BODY(void *msg)
{
    u8 i = ((u8 *)msg)[4];
    if (i < PLR_WPN_COUNT && weapons[i])
        msgSystem.SendMsg(weapons[i], 0x4C8, msg, 0);
}

// v12 0x00429F00  (feb 0x00425C20)
// The body's animation group of weapon w (-1: the current one): the shovel 3, the colt 2, the machine gun 1,
// the bomb 4, the rest 0 (its sequences sit at the group's base: run 8, back 0x13, jump 0x24...).
int plrPLAYER_COMMON::GetMountType(int w)
{
    if (w < 0)
        w = curWeapon;
    switch (w) {
    case PLR_WPN_SVL: return 3;
    case PLR_WPN_CLT: return 2;
    case PLR_WPN_MCG: return 1;
    case PLR_WPN_BMB: return 4;
    case PLR_WPN_STG: case PLR_WPN_CRB: case PLR_WPN_ACD: case PLR_WPN_BSK: case PLR_WPN_MDG: case PLR_WPN_MNG:
    case PLR_WPN_AMG: return 0;
    }
    return 1;
}

#pragma pack(push, 1)
struct plrSCORE_MSG : msgDATA {                         // GS_MSG_MP_NOTIFY 0x40E type 9: the score (0x12 bytes)
    short type;                                         // +0x04 9
    int   frags;                                        // +0x06
    int   deaths;                                       // +0x0A
    int   treasures;                                    // +0x0E
};
#pragma pack(pop)

// v12 0x0042A130  (feb 0x00425DF0)
// Treasure hunt (game mode 3), on the server, not for a remote player's copy (+0x98 2): n of the carried treasures
// (-1 all) dropped -- where the player is, or scattered round it -- and the score sent to all. (Nothing in
// single player.)
void plrPLAYER_COMMON::ReleaseTreasures(int scatter, int n)
{
    if (!(entFlags & 4) || unk98 == 2 || wrSysGO->mode != 3)
        return;
    int cnt;
    if (n == -1) {
        cnt = treasures;
    } else {
        if (n < 0)
            return;
        cnt = n > treasures ? treasures : n;
    }
    m3dV pos;
    inst->GetPos(&pos);
    for (int i = 0; i < cnt; i++) {
        if (scatter == 1) {
            m3dV off;
            m3dMakeVTangRandom(&m3dVUnitY, &off);
            pos.x = off.x + pos.x;
            pos.y = off.y + pos.y;
            pos.z = off.z + pos.z;
            wrSysGO->GenerateMP_Treasure(2, &pos);
        } else {
            wrSysGO->GenerateMP_Treasure(0, 0);
        }
    }
    treasures -= cnt;
    plrSCORE_MSG m;
    m.packId = 0;
    m.tag = 0x40;
    m.size = 0x12;
    m.type = 9;
    m.frags = frags;
    m.deaths = unk_217;
    m.treasures = treasures;
    gsSysServer->SendMsgClientAll(this, 0x40E, &m, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
}

// v12 0x00428DC0  (feb 0x00424C00)
// The body's animation group for weapon w (-1: the current one): the shovel, the colt, the machine gun, the
// bomb have their own; the rest share one.
void plrPLAYER_COMMON::SetCurWpn(int w, int animate)
{
    if (w < 0)
        w = curWeapon;
    int g;
    switch (w) {
    case PLR_WPN_SVL: g = 3; break;
    case PLR_WPN_CLT: g = 2; break;
    case PLR_WPN_MCG: g = 1; break;
    case PLR_WPN_BMB: g = 4; break;
    case PLR_WPN_STG: case PLR_WPN_CRB: case PLR_WPN_ACD: case PLR_WPN_BSK: case PLR_WPN_MDG: case PLR_WPN_MNG:
    case PLR_WPN_AMG: g = 0; break;
    default: g = 1; break;
    }
    ((animSYSTEM_INST *)inst->link)->SetGroupCur(g, 0.2f);
    unk108 |= 0x400;
}

// v12 0x00427760  (feb 0x00423AA0)
// A fresh start (not for a remote copy, entity type 2): health 100, nothing else. Every weapon put away with an empty
// clip -- in COOP (wrGO mode 2, unless flags2 4 or single-player MP) refilled to its maximum, else taken away (no
// ammo). Then the colt (14 rounds, a full clip, in the hands) and the shovel. In single player the rest by the level
// ("lev_<a>_<b>", n = 10a + b): over 13 the shotgun (200), 21 the machine gun (1000), 22 the crossbow (50), 31 the
// rocket launcher (100), 42 the minigun (1000), 43 the acid gun (100), 52 the bomb (20), 62 the medusa gun (50),
// 75 the atomic gun (20).
void plrPLAYER_COMMON::SetDefaultValues()
{
    if (unk98 == 2)
        return;
    int coop = 0;
    if (!(flags2 & 4) && gsSysMP->mode != 1) {
        coop = 1;
        if (wrSysGO->mode != 2)
            coop = 0;
    }
    aliveTime = 0;
    moveFlags &= ~2u;
    health = PLR_HEALTH_DEFAULT;
    armor = 0;
    money = 0;
    for (int i = 0; i < 3; i++) {
        powerup[i].state = 0;
        powerup[i].timeLeft = 0;
    }
    for (int i = 0; i < PLR_WPN_COUNT; i++) {
        if (coop) {
            weapons[i]->SetAmmo(weapons[i]->maxAmmo);
        } else {
            weapons[i]->Enable(0);
            weapons[i]->SetAmmo(0);
        }
        weapons[i]->clip = -1;
        weapons[i]->inst->flags |= 1;
    }
    weapons[PLR_WPN_CLT]->Enable(1);
    weapons[PLR_WPN_CLT]->SetAmmo(14.0f);
    weapons[PLR_WPN_CLT]->Recharge();
    wrEVCall<void>(this, 0x108, PLR_WPN_CLT, 0);         // SetCurWpn
    weapons[PLR_WPN_SVL]->Enable(1);
    weapons[PLR_WPN_SVL]->SetAmmo(1.0f);
    if (coop)
        return;
    const char *l = strstr(gsSceneFullName, "lev_");
    int a, b;
    if (!l || sscanf(l, "lev_%d_%d", &a, &b) != 2)
        return;
    int n = a * 10 + b;
    static const struct { int over, slot; float ammo; } give[9] = {
        { 13, 2, 200.0f }, { 21, 3, 1000.0f }, { 22, 4, 50.0f }, { 31, 6, 100.0f }, { 42, 8, 1000.0f },
        { 43, 5, 100.0f }, { 52, 9, 20.0f }, { 62, 7, 50.0f }, { 75, 10, 20.0f } };
    for (int i = 0; i < 9; i++)
        if (n > give[i].over) {
            weapons[give[i].slot]->Enable(1);
            weapons[give[i].slot]->SetAmmo(give[i].ammo);
        }
}

// v12 0x00428E50  (feb 0x00424C80)
// The shared part of a respawn: the fresh start's values (slot +0x104), the move flags but 0x1001 cleared, the
// weapon in the hands shown. Full (the debug key 4 with Shift): every weapon with ammo, 200 gold, the three
// power-ups.
void plrPLAYER_COMMON::Revitalize(int full)
{
    wrEVCall<void>(this, 0x104);                         // SetDefaultValues
    moveFlags &= 0x1001;
    aliveTime = 0;
    weapons[curWeapon]->inst->flags &= ~1u;
    moveFlags &= ~0x10000u;
    if (!full)
        return;
    static const struct { int slot; float ammo; } give[9] = {
        { 2, 200.0f }, { 3, 1000.0f }, { 4, 50.0f }, { 6, 100.0f }, { 5, 100.0f }, { 8, 1000.0f }, { 7, 50.0f },
        { 9, 20.0f }, { 10, 20.0f } };
    for (int i = 0; i < 9; i++) {
        weapons[give[i].slot]->Enable(1);
        weapons[give[i].slot]->SetAmmo(give[i].ammo);
    }
    money = 200.0f;
    powerup[0].state = 1;
    powerup[1].state = 1;
    powerup[2].state = 1;
}

// v12 0x004282E0  (feb 0x00424270)
// Damage the player does not take: god mode (moveFlags 4), dead; on the server, except falls and the world's
// harm (type 0 subtypes 1 / 3): the unholy-armor powerup; COOP / DM (wrGO modes 2 / 0) with moveFlags 0x400000;
// without friendly fire, another player's weapon in COOP (mode 2) or a team mate's in TH (mode 3); in DM / TH
// (modes 1 / 3) the spawn protection ([Multiplayer] ServerPlrGODAfterDeath against aliveTime).
int plrPLAYER_COMMON::IsIgnoreDmg(dmgDAMAGE *d)
{
    if (moveFlags & PLR_MF_GOD)
        return 1;
    if (wrEVCall<BOOL32>(this, 0x74))                    // IsDead
        return 1;
    if (!(entFlags & 4))
        return 0;
    int t = d->type;
    if (t == 5)
        return 0;
    if (t == 0 && (d->subtype == 3 || d->subtype == 1))
        return 0;
    if (powerup[PLR_PWR_UNHOLY_ARMOR].state == PLR_PWR_ACTIVE)
        return 1;
    int mode = wrSysGO->mode;
    if ((mode == 2 || mode == 0) && (moveFlags & 0x400000))
        return 1;
    if (!wrSysGO->friendlyFire && t == 1) {
        if (mode == 2 && mpId != (int)d->sourceId)
            return 1;
        if (mode == 3) {
            entIACTIVE_OBJ *e = gsSysMP->FindMPID(d->sourceId, 0);
            if (e && e->classId == 0x504C5952 && team == ((plrPLAYER_COMMON *)e)->team && mpId != (int)d->sourceId)
                return 1;
        }
    }
    mode = wrSysGO->mode;
    if ((mode == 1 || mode == 3) && !(wrSysGO->godAfterDeath <= aliveTime))
        return 1;
    return 0;
}

// v12 0x00428200  (feb 0x00424190)
// On the server: the hit scaled by the difficulty (wrGO_SYSTEM::GetPlrDmgScale); armor takes it first and lets a
// third of what it took through (1.2: not for falls (type 5) or the world (type 0)); then NotifyHealthChange
// (slot +0x110) and the hit's amount back to its unscaled value.
void plrPLAYER_COMMON::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (moveFlags & PLR_MF_GOD)
        return;
    if (!(entFlags & 4))
        return;
    if (IsIgnoreDmg(d))
        return;
    float dmg = wrSysGO->GetPlrDmgScale() * d->amount;
    float taken = 0, through = 0;
    d->amount = dmg;
    if (d->type != 5 && d->type != 0) {
        taken = armor < dmg ? armor : dmg;
        through = 0.33333334f * taken;
        dmg = dmg - taken;
    }
    armor = armor - taken;
    health = health - (dmg + through);
    wrEVCall<void>(this, 0x110, d);                     // NotifyHealthChange
    d->amount = d->amount / wrSysGO->GetPlrDmgScale();
}

#pragma pack(push, 1)
struct plrHEALTH_MSG : msgDATA { short type; int value; u8 sub; };              // 0x0B bytes (0x40E type 0x10)
struct plrARMOR_MSG  : msgDATA { short type; int value; };                      // 0x0A bytes (0x40E type 6)
#pragma pack(pop)

// v12 0x00429040  (feb 0x00424DD0)
// The statistics hear of the hit; the server sends the new health and armor to the other clients, then either the
// death (-98, to every client, the local one too: it starts the death) or the hit event for the sounds
// (unk108 0x400000 for a fall, else 0x800).
void plrPLAYER_COMMON::NotifyHealthChange(dmgDAMAGE *d)
{
    plrHEALTH_MSG h;
    h.packId = 0;
    h.tag = 0x40;
    h.value = 0;
    h.sub = 0;
    h.type = 0x10;
    h.size = 0x0B;
    plrARMOR_MSG a;
    a.packId = 0;
    a.tag = 0x40;
    a.value = 0;
    a.type = 6;
    a.size = 0x0A;
    if (wrSysStat)
        wrSysStat->Notify_PlrHit(this, d);
    if (!(entFlags & 4))
        return;
    h.value = *(int *)&health;
    h.sub = d->subtype;
    gsSysServer->SendMsgClientAll(this, 0x40E, &h, 0xA0000, -1, -3);
    a.value = *(int *)&armor;
    gsSysServer->SendMsgClientAll(this, 0x40E, &a, 0xA0000, -1, -3);
    if (health < 1e-6f) {
        plrEV_START_DEATH m;
        m.packId = 0;
        m.tag = 0x40;
        m.world3 = 0;
        m.type = -98;
        m.size = 0x13;
        m.source = d->sourceId;
        m.sub = d->subtype;
        m.body = d->type == 1 ? *(int *)((u8 *)d + 0x1A) : -1;
        if (d->type == 0 && d->subtype == 3)
            m.world3 = 1;
        gsSysServer->SendMsgClientAll(this, 0x40E, &m, 0x20000, -3, -3);
        return;
    }
    if (d->subtype == 5)
        unk108 |= 0x400000;
    else
        unk108 |= 0x800;
}

// v12 0x00429F70
// The trading altar may sell powerup id: the gold is there and it is not held or active already.
int plrPLAYER_COMMON::IsCanBuyPowerup(int id)
{
    if (money < _plrPwrCostTbl[id])
        return 0;
    return powerup[id].state == PLR_PWR_NONE;
}

// v12 0x00429FA0  (feb 0x00425CC0)
int plrPLAYER_COMMON::IsActivePowerup(int id)
{
    return powerup[id].state == PLR_PWR_ACTIVE;
}

#pragma pack(push, 1)
struct plrPICK_DATA {                   // ProcessPICK's data (msgDATA, size 0xC)
    u8    packId, tag;
    u16   size;
    int   type;                         // +0x04 the item's kind (itemType)
    float amount;                       // +0x08
};
#pragma pack(pop)

// v12 0x00428100  (feb 0x00424060)  slot +0x9C
// An item ('$ITM') touched: its kind and amount to ProcessPICK (slot +0xC4), which gives the bonus; its answer.
BOOL32 plrPLAYER_COMMON::Pick(entENTITY *item)
{
    plrPICK_DATA d;
    d.packId = 0;
    d.tag = 0x40;
    d.size = 0xC;
    if (item->classId != 0x2449544D)
        return 0;
    d.type = item->itemType;
    d.amount = ((itemITEM *)item)->amount;
    return wrEVCall<BOOL32>(this, 0xC4, &d);
}

// v12 0x00425EA0  static initialiser (the sncSOUND_DESCR / _3D constructors v12 0x004038F0 / 0x0042A7A0 inline)
// The player's 41 sounds: steps (plain, gravel, wood, lava; 1st / 2nd foot; water), landings, the jump, the weapon
// change, the pick-ups, being hit, death, drowning, swimming, the power-ups, the secret area, crouching, coming up
// from water. Steps 10 dB down; being hit waits 150 ms between plays.
sncSOUND_DESCR_3D plrSnd[41];                           // v12 0x00641C38
sncSOUND_DESCR_3D *plrSndList = plrSnd;                 // v12 0x0062B864
static struct plrSND_INIT {
    plrSND_INIT()
    {
        static const struct { const char *name; u32 flags; float volume; int delay; } s[41] = {
            { "player_step_1", 0, -10.0f, 0 }, { "player_step_gravel_1", 0, -10.0f, 0 },
            { "player_step_wood_1", 0, -10.0f, 0 }, { "player_step_lava_1", 0, -10.0f, 0 },
            { "player_step_2", 0, -10.0f, 0 }, { "player_step_gravel_2", 0, -10.0f, 0 },
            { "player_step_wood_2", 0, -10.0f, 0 }, { "player_step_lava_2", 0, -10.0f, 0 },
            { "player_step_water_1", 0, -10.0f, 0 }, { "player_step_water_2", 0, -10.0f, 0 },
            { "player_land", 0, 0.0f, 0 }, { "player_land_gravel", 0, 0.0f, 0 }, { "player_land_wood", 0, 0.0f, 0 },
            { "player_land_lava", 0, 0.0f, 0 }, { "player_land_water", 0x80000, 0.0f, 0 },
            { "player_jump_start", 0x10000, 0.0f, 0 }, { "player_jump_end", 0x10000, 0.0f, 0 },
            { "player_change_wpn", 0, 0.0f, 0 }, { "player_get_ammo", 0, 0.0f, 0 },
            { "player_get_health", 0, 0.0f, 0 }, { "player_get_key", 0, 0.0f, 0 },
            { "player_get_weapon", 0, 0.0f, 0 }, { "player_get_treasure", 0, 0.0f, 0 },
            { "player_get_armor", 0, 0.0f, 0 }, { "player_being_hit_1", 0x4C0000, 0.0f, 150 },
            { "player_being_hit_2", 0x4C0000, 0.0f, 150 }, { "player_death", 0x180000, 0.0f, 0 },
            { "player_death_lava", 0x180000, 0.0f, 0 }, { "player_death_water", 0x180000, 0.0f, 0 },
            { "player_being_drown_1", 0xC0000, 0.0f, 0 }, { "player_being_drown_2", 0xC0000, 0.0f, 0 },
            { "player_swim", 0x40000, 0.0f, 0 }, { "player_get_titan_damage", 0, 0.0f, 0 },
            { "player_get_unholy_armor", 0, 0.0f, 0 }, { "player_find_secret_area", 0, 0.0f, 0 },
            { "player_activate_pwp", 0, 0.0f, 0 }, { "player_deactivate_pwp", 0, 0.0f, 0 },
            { "player_crouch", 0x40000, 0.0f, 0 }, { "player_up_from_water_1", 0, 0.0f, 0 },
            { "player_up_from_water_2", 0, 0.0f, 0 }, { "player_up_from_water_3", 0, 0.0f, 0 },
        };
        for (int i = 0; i < 41; i++) {
            sncSOUND_DESCR_3D *d = &plrSnd[i];
            d->name.SetName((char *)s[i].name);
            d->dataFlags = 0;
            d->flags = s[i].flags;
            d->volume = s[i].volume;
            d->delay = s[i].delay;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->minDist = 0.0f;
            d->maxDist = 0.0f;
            d->vtbl = 0;                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} plrSndInit;

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30

// v12 0x004265A0  (feb 0x00422A70)
int __fastcall plrCommInit()
{
    return sncLinkSndListData(41, plrSnd) != 0;
}

// v12 0x00426710  (feb 0x00422BB0)
// The body's animation system: 0x76 sequences in 5 groups (one per mount type: shoulder, hip, hand, shovel, bomb),
// 2 overlay channels; the transitions (idle, run / back cycles merged, death, jump start / fly / end, turns, power-up,
// swim, crouch, drowning, falling, landing, wounds); 0x79 named sequence slots taking the template's (the attack
// overlays ATTACK_SHOVEL 0x77 and ATTACK_BOMB 0x78 from the lower back down, 0.1 s blend), slot 0 a 0.5 s still.
BOOL32 plrPLAYER_COMMON::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSYSTEM_GROUP *s = new animSYSTEM_GROUP(0x76, 5, 2);
    if (!s)
        return 0;
    t->aniSystem = s;
    s->DeclareGroupSeq(0x1, 1);
    s->DeclareGroupSeq(0x7, 2);
    s->DeclareGroupSeq(0x12, 2);
    s->DeclareGroupSeq(0x1D, 1);
    s->DeclareGroupSeq(0x23, 2);
    s->DeclareGroupSeq(0x2E, 1);
    s->DeclareGroupSeq(0x34, 1);
    s->DeclareGroupSeq(0x3A, 1);
    s->DeclareGroupSeq(0x40, 1);
    s->DeclareGroupSeq(0x46, 1);
    s->DeclareGroupSeq(0x58, 1);
    s->DeclareGroupSeq(0x4C, 1);
    s->DeclareGroupSeq(0x52, 1);
    s->DeclareGroupSeq(0x5E, 1);
    s->DeclareGroupSeq(0x64, 1);
    s->DeclareGroupSeq(0x6A, 1);
    s->DeclareGroupSeq(0x70, 1);
    s->SetGroupSuffix(0, (char *)"_SHOULDER");
    s->SetGroupSuffix(1, (char *)"_HIP");
    s->SetGroupSuffix(2, (char *)"_HAND");
    s->SetGroupSuffix(3, (char *)"_SHOVEL");
    s->SetGroupSuffix(4, (char *)"_BOMB");
    s->SetTranTypeToAny(0, 0, 1, -1);
    s->SetTranTypeToAny(1, 0, 1, -1);
    s->SetTranCycleAnim(2);
    s->SetTranCycle2Anim(8, 0xD);
    s->SetTranCycle2Anim(0x13, 0x18);
    s->SetTranMergeCycles(8, 0xD, 0x13, 0x18, 0);
    s->SetTranParam(7, 1, 0, 2, 1);
    s->SetTranTypeToAny(0x1D, 0, 0, -1);
    s->SetTranParam(0x1D, 0x1D, 2, 0x1D, 1);
    s->SetTranTypeToAny(0x58, 0, 0, -1);
    s->SetTranParam(0x58, 0x58, 2, 0x58, 1);
    s->SetTranParam(7, 0x23, 0, 0x29, 1);
    s->SetTranParam(0x12, 0x23, 0, 0x29, 1);
    s->SetTranParam(1, 0x23, 0, 0x24, 1);
    s->SetTranParam(0x24, 0x23, 1, 0x29, 0);
    s->SetTranParam(0x29, 0x23, 2, 0x29, 0);
    s->SetTranParam(0x2E, 0x23, 1, 0x24, 1);
    s->SetTranParam(0x23, 0x2E, 1, 0x2F, 0);
    s->SetTranParam(0x2E, 0x2E, 2, 0x2F, 0);
    s->SetTranParam(0x24, 1, 1, 0x29, 1);
    s->SetTranParam(0x24, 7, 1, 0x29, 1);
    s->SetTranParam(0x24, 0x12, 1, 0x29, 1);
    s->SetTranParam(0x29, 1, 0, 0x2F, 1);
    s->SetTranParam(0x29, 7, 0, 0xD, 1);
    s->SetTranParam(0x29, 0x12, 0, 0x18, 1);
    s->SetTranParam(0x2E, 1, 1, 2, 1);
    s->SetTranParam(0x2E, 7, 1, 8, 1);
    s->SetTranParam(0x2E, 0x12, 1, 8, 1);
    s->SetTranTypeToAny(0x34, 0, 1, -1);
    s->SetTranTypeToAny(0x3A, 0, 1, -1);
    s->SetTranParam(0x35, 1, 1, 2, 0);
    s->SetTranParam(0x3B, 1, 1, 2, 0);
    s->SetTranCycleAnim(0x35);
    s->SetTranCycleAnim(0x3B);
    s->SetTranTypeToAny(0x40, 0, 0, -1);
    s->SetTranTypeToAny(0x46, 0, 1, -1);
    s->SetTranCycleAnim(0x47);
    s->SetTranParam(0x46, 0x58, 1, 0x59, 1);
    s->SetTranTypeToAny(0x52, 0, 1, -1);
    s->SetTranTypeToAny(0x4C, 0, 1, -1);
    s->SetTranCycleAnim(0x4D);
    s->SetTranCycleAnim(0x53);
    s->SetTranTypeToAny(0x6A, 0, 1, -1);
    s->SetTranTypeToAny(0x70, 0, 1, -1);
    s->SetTranParam(0x6A, 1, 1, 2, 1);
    s->SetTranParam(0x70, 1, 1, 2, 1);
    s->SetTranCycleAnim(0x4D);
    s->SetTranCycleAnim(0x53);
    animSEQ *old = 0;
    int nOld = 0;
    if (t->seq) {
        nOld = t->nSeq;
        old = t->seq;
    }
    t->nSeq = 0x79;
    int *blk = (int *)operator new(0x404C);
    animSEQ *seq = 0;
    if (blk) {
        blk[0] = 0x79;
        seq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 0x79; i++) {
            memset(&seq[i], 0, sizeof(animSEQ));
            seq[i].dir = 2;
        }
    }
    t->seq = seq;
    static const struct { int i; const char *n; } names[] = {
        { 0, "PLR_SEQ_UNUSED" }, { 2, "PLR_SEQ_IDLE1" }, { 8, "PLR_SEQ_RUN_1" }, { 0xD, "PLR_SEQ_RUN_2" },
        { 0x13, "PLR_SEQ_RUN_BACK_1" }, { 0x18, "PLR_SEQ_RUN_BACK_2" }, { 0x1E, "PLR_SEQ_DEATH" },
        { 0x24, "PLR_SEQ_JUMP_START" }, { 0x29, "PLR_SEQ_JUMP_FLY" }, { 0x2F, "PLR_SEQ_JUMP_END" },
        { 0x35, "PLR_SEQ_TURN_LEFT" }, { 0x3B, "PLR_SEQ_TURN_RIGHT" }, { 0x41, "PLR_SEQ_GET_POWER" },
        { 0x47, "PLR_SEQ_SWIMMING" }, { 0x4D, "PLR_SEQ_CROUCH" }, { 0x53, "PLR_SEQ_CROUCH_IDLE" },
        { 0x59, "PLR_SEQ_DROWNING" }, { 0x5F, "PLR_SEQ_FALLING" }, { 0x65, "PLR_SEQ_LANDING" },
        { 0x6B, "PLR_SEQ_SLIGHTLY_WOUNDED" }, { 0x71, "PLR_SEQ_BADLY_WOUNDED" },
    };
    for (unsigned k = 0; k < sizeof(names) / sizeof(names[0]); k++)
        _entAnimSetSeqGroupName(t, names[k].i, (char *)names[k].n);
    strcpy(t->seq[0x77].name, "ATTACK_SHOVEL");
    strcpy(t->seq[0x78].name, "ATTACK_BOMB");
    entAnimAssignTplSeq(t, old, nOld);
    objOBJ *lb = objFindName(t->root, (char *)"LowBACK");
    animSEQ *sq = &t->seq[t->FindSeqName((char *)"ATTACK_SHOVEL")];
    animSEQ_PARAM *pa = s->param;
    pa[0].seq = 0x77;
    pa[0].flags = lb ? lb->idHier : 0;
    pa[0].blendTime = 0.1f;
    pa[0].unk0C = (int)sq;
    pa[0].unk10 = 4;
    lb = objFindName(t->root, (char *)"LowBACK");
    sq = &t->seq[t->FindSeqName((char *)"ATTACK_BOMB")];
    pa[1].seq = 0x78;
    pa[1].flags = lb ? lb->idHier : 0;
    pa[1].blendTime = 0.1f;
    pa[1].unk0C = (int)sq;
    pa[1].unk10 = 4;
    t->seq[0].frameStart = 0.0f;
    t->seq[0].frameEnd = 1.0f;
    t->seq[0].lengthMs = 500.0f;
    t->seq[0].dir = 0;
    if (old)
        operator delete((int *)old - 1);
    return 1;
}

extern u8 gsMsgPackId;                                  // v12 0x0098EC2C the id written at the head of a message (name ours)

// v12 0x00427160  (feb 0x00423550)
// Three cone voices (10 - 30 m, plain sound for the local player); the 11 weapons -- a body (+0x11C: the third-
// person ones) makes each class with its "_item" model and marks it (flags2 4) -- owned and switched off; the body
// class bit, the 0x8B net size, the frame and level messages, the back angles, pitch 90, the shovel; the instance
// flagged 0x10000000; the sync block (an MP update of 0x53 bytes: no sequence yet); the dead-reckoning smoothing.
BOOL32 plrPLAYER_COMMON::ProcessINIT()
{
    static const char *cls[11] = { "pwp_svl", "pwp_clt", "pwp_stg", "pwp_mcg", "pwp_crb", "pwp_acd", "pwp_bsk",
                                   "pwp_mdg", "pwp_mng", "pwp_bmb", "pwp_amg" };
    static const char *item[11] = { "pwp_svl_item", "pwp_clt_item", "pwp_stg_item", "pwp_mcg_item", "pwp_crb_item",
                                    "pwp_acd_item", "pwp_bsk_item", "pwp_mdg_item", "pwp_mng_item", "pwp_bmb_item",
                                    "pwp_amg_item" };
    entENTITY::flags2 |= 4;
    unk108 = 0;
    *(int *)_1FF = 0;
    for (int i = 0; i < 3; i++) {
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        if (c)
            c->maxDist = 30.0f;
        sndChannel[i] = c;
        if (!c)
            return 0;
        if (!wrEVCall<BOOL32>(this, 0x11C))
            c->flags |= 1;
        gsSysSound->AddChannel(c, this, 0);
    }
    weapons = (pwpWEAPON **)apCalloc(4, 0xB);
    if (!weapons)
        return 0;
    for (int i = 0; i < 11; i++) {
        if (wrEVCall<BOOL32>(this, 0x11C)) {
            weapons[i] = (pwpWEAPON *)entCreate(gsScenePtr, (char *)cls[i], (char *)item[i], 0, 0, 0);
            if (weapons[i])
                weapons[i]->pwpWEAPON::flags2 |= 4;
        } else
            weapons[i] = (pwpWEAPON *)entCreate(gsScenePtr, (char *)cls[i], (char *)cls[i], 0, 0, 0);
        if (!weapons[i])
            return 0;
        weapons[i]->owner = (plrPLAYER *)this;
        weapons[i]->Enable(0);
    }
    WR_FIELD(u32, this, 0xBC) |= 4;
    WR_FIELD(u8, this, 0x10C) = 0x8B;
    entFlags |= 0x30;
    unk_1BF[0] = unk_1BF[1] = unk_1BF[2] = unk_1BF[3] = 0;
    pitch = 90.0f;
    curWeapon = 0;
    inst->flags |= 0x10000000;
    u8 *s = (u8 *)operator new(0x53);
    if (s) {                                            // (the MP update message ctor v12 0x005151B0, inline)
        memset(s, 0, 0x53);
        s[0] = gsMsgPackId;
        s[1] = 0x40;
        s[0x4F] = 0xFF;
        s[0x50] = 0;
        *(u16 *)(s + 2) = 0x53;
    }
    unk_1E3 = (int)s;
    mpSmooth[0] = 0.1f;
    mpSmooth[1] = 2.0f;
    mpSmooth[2] = 0.2f;
    return 1;
}

// v12 0x00427540  (feb 0x00423920)
// Not on a floor yet, no remembered position, the 0x400000 mode off; 100 s since the respawn.
BOOL32 plrPLAYER_COMMON::ProcessINIT_LEVEL()
{
    flags3 &= ~2u;
    flags2 &= ~0x100u;
    moveFlags &= ~0x400000u;
    WR_FIELD(float, this, 0x1D3) = 100.0f;
    return 1;
}

// v12 0x004274D0  (feb 0x004238B0)
// A remote player drops what it carried (TH); the weapons end (TERM posted), their list and the sync block freed.
void plrPLAYER_COMMON::ProcessTERM()
{
    if (WR_FIELD(int, this, 0x98) == 1)
        ReleaseTreasures(0, -1);
    for (int i = 0; i < 11; i++)
        if (weapons[i])
            msgSystem.PostMsg(weapons[i], MSG_MSG_TERM, 0);
    apFree(weapons);
    weapons = 0;
    operator delete((void *)unk_1E3);
    unk_1E3 = 0;
}

// v12 0x004275D0  (feb 0x00423940)
// Placed: on the map's start (not for a client's copy, entFlags 1) -- in mode 3 where it is; the start values
// unless kept (flags2 4) in single player; the game counters cleared.
BOOL32 plrPLAYER_COMMON::ProcessINIT_MAP()
{
    m3dV pos, dir;
    if (WR_FIELD(int, gsSysMP, 0x28) == 3) {
        inst->GetPos(&pos);
        inst->GetDir(&dir);
        wrEVCall<void>(this, 0x70, &pos, &dir, 1);
    } else if (!(entFlags & 1)) {
        gsSysMP->GetStartPosDir(0, &pos, &dir);
        wrEVCall<void>(this, 0x70, &pos, &dir, 1);
    }
    if ((flags2 & 4) || WR_FIELD(int, gsSysMP, 0x28) != 1)
        wrEVCall<void>(this, 0x104);                    // SetDefaultValues
    flags2 &= ~4u;
    frags = 0;
    unk_217 = 0;
    treasures = 0;
    return 1;
}

// v12 0x00427690  (feb 0x004239E0)
// A new game: the move / state bits cleared; the weapons take the player's ownership bits (entFlags 7), drawn
// apart (0x1000) unless it is a client's; no gold, no power-ups, no counters; the start values to come (flags2 4).
BOOL32 plrPLAYER_COMMON::ProcessINIT_GAME()
{
    u32 own = entFlags & 7;
    moveFlags = 0;
    unk_1F7 = 0;
    flags2 = 0;
    unk108 = 0;
    *(int *)_1FF = 0;
    for (int i = 0; i < 11; i++) {
        weapons[i]->entFlags = (weapons[i]->entFlags & ~7u) | own;
        if (entFlags & 1)
            weapons[i]->inst->flags &= ~0x1000u;
        else
            weapons[i]->inst->flags |= 0x1000;
    }
    money = 0;
    for (int i = 0; i < 3; i++)
        powerup[i].state = 0;
    *(int *)_20B = 0;
    frags = 0;
    unk_217 = 0;
    treasures = 0;
    flags2 |= 4;
    return 1;
}

// v12 0x00427590
// The level ends: the voices stop; no events.
void plrPLAYER_COMMON::ProcessTERM_LEVEL()
{
    for (int i = 0; i < 3; i++)
        if (sndChannel[i])
            sndChannel[i]->Stop();
    unk108 = 0;
    *(int *)_1FF = 0;
}

static void *_plrCfgAutoWeapon;                         // v12 0x00641C28 the cfg cache of [GameOpt] AutoWeapon

// v12 0x00427AA0  (feb 0x00423CB0)
// The shared frame: [GameOpt] AutoWeapon (0..3) into the config bits; in TH a player without a team gets the smaller
// one; the statistics count the time with the weapon; the attack planner hears of the player; the time since the
// respawn; the frame's sounds (slot +0x120).
void plrPLAYER_COMMON::ProcessFRAME()
{
    int aw = 0;
    _apCfgReadInt(&_plrCfgAutoWeapon, (char *)"GameOpt", (char *)"AutoWeapon", &aw);
    if (aw < 0 || aw >= 4)
        aw = 0;
    cfg.bits = (cfg.bits & ~6u) | ((aw * 2) & 6);
    if (WR_FIELD(int, wrSysGO, 0x98) == 3 && gsSysMP->mode != 3 && WR_FIELD(int, this, 0x98) != 2 && team == -1)
        wrEVCall<void>(this, 0x128, wrSysGO->SelectDefaultTeam(this));   // SetTeam
    if (wrSysStat && WR_FIELD(int, this, 0x98) != 2)
        wrSysStat->Notify_WpnTime(this, weapons[curWeapon]);
    plgsACTION::ProcessFRAME();
    aliveTime += gsElapsedTime;
    wrEVCall<void>(this, 0x120);                         // UpdateSound
}

// v12 0x0042A270  plrPLAYER slot +0x120 (plrPLAYER_MP's for a remote one)
// The frame's sound events (unk108, and +0x1FF for coming up from water) on the three voices: steps (0x1 / 0x2: by
// the floor; in water 0x10 / 0x20; swimming 0x800000), landing (0x40: by the floor; 0x80 in water), crouching
// (0x20000000) on the first; the jump (0x100 / 0x200) and a secret area (0x4000000) on the second; the weapon change
// (0x400), being hit (0x800) / drowning (0x400000) -- one of two at random --, death (0x1000 / 0x2000 lava / 0x4000
// water), the pick-ups (0x8000 weapon, 0x10000 ammo, 0x20000 health, 0x40000 armor, 0x80000 key, 0x100000 /
// 0x200000 treasure), the power-ups (0x1000000 titan damage, 0x2000000 unholy armor, 0x8000000 / 0x10000000 on /
// off) on the third. Then the events are cleared; the weapon's sound event goes into the sync block (byte 0x51
// low nibble), and if there is one the entity is synchronised (0x100).
void plrPLAYER_COMMON::UpdateSound()
{
    sncCHANNEL **ch = sndChannel;
    if (unk108 & 1)
        PlayStepSound(0);
    if (unk108 & 2)
        PlayStepSound(4);
    if (unk108 & 0x800000)
        ch[0]->Play(&plrSndList[31], 0);
    if (unk108 & 0x10)
        ch[0]->Play(&plrSndList[9], 0);
    if (unk108 & 0x20)
        ch[0]->Play(&plrSndList[9], 0);
    if (unk108 & 0x40)
        PlayStepSound(10);
    if (unk108 & 0x80)
        ch[0]->Play(&plrSndList[14], 0);
    if (unk108 & 0x20000000)
        ch[0]->Play(&plrSndList[37], 0);
    if (unk108 & 0x100)
        ch[1]->Play(&plrSndList[15], 0);
    if (unk108 & 0x200)
        ch[1]->Play(&plrSndList[16], 0);
    if (unk108 & 0x4000000)
        ch[1]->Play(&plrSndList[34], 0);
    if (unk108 & 0x400)
        ch[2]->Play(&plrSndList[17], 0);
    if (unk108 & 0x800)
        ch[2]->Play(&plrSndList[m3dRandIntRange(0x18, 0x19)], 0);
    if (unk108 & 0x400000)
        ch[2]->Play(&plrSndList[m3dRandIntRange(0x1D, 0x1E)], 0);
    static const struct { u32 bit; int snd; } third[] = {
        { 0x1000, 26 }, { 0x2000, 27 }, { 0x4000, 28 }, { 0x8000, 21 }, { 0x10000, 18 }, { 0x20000, 19 },
        { 0x40000, 23 }, { 0x80000, 20 }, { 0x100000, 22 }, { 0x200000, 22 }, { 0x1000000, 32 }, { 0x2000000, 33 },
        { 0x8000000, 35 }, { 0x10000000, 36 } };
    for (unsigned k = 0; k < sizeof(third) / sizeof(third[0]); k++)
        if (unk108 & third[k].bit)
            ch[2]->Play(&plrSndList[third[k].snd], 0);
    for (int k = 0; k < 3; k++)
        if (WR_FIELD(u8, this, 0x1FF) & (1 << k))
            ch[2]->Play(&plrSndList[38 + k], 0);
    u8 *sync = (u8 *)unk_1E3;
    sync[0x51] = (sync[0x51] & 0xF0) | (weapons[curWeapon]->lastAnimEvent & 0x0F);
    WR_FIELD(u32, this, 0x1FF) = 0;
    unk108 = 0;
    if (((u8 *)unk_1E3)[0x51] & 0x0F)
        entFlags |= 0x100;
}

// v12 0x0042D380  (name ours)
// A step / landing sound (base: the first foot 0, the second 4, a landing 10) when standing on something: on an
// entity's instance whose entity has bit 2 at +0xBC (lava) the lava one (+3); on the level, by the floor face's
// texture -- a gravel / grass / ground one (+1), a wood / barrel one (+2), else the plain one.
void plrPLAYER_COMMON::PlayStepSound(int base)
{
    static const char *gravel[] = { "grass_gr", "grass1", "grass", "grass2", "grass3", "grass4", "grass5",
        "grav_001", "grav_0011", "grav_002", "grav_022", "ground", "ground1", "ground2", "ground3", "ground4",
        "ground5", "ground7", "ground7a", "ground7b", "ground7c", "ground7d", "ground7e", "ground7f", "sand_001",
        "skull01", 0 };                                  // v12 0x0062C7C8
    static const char *wood[] = { "barrel", "barrel2", "lebedka1", "wood", "wood_093", "wood_093a", "wood_c",
        "wood_c1", "wood_TR", "wood1", "wood3_c", 0 };   // v12 0x0062C834
    if (!(flags3 & 2) || unk_247 == -1 || unk_24B == -1)
        return;
    if (floorInst && floorInst->ent && (WR_FIELD(u8, floorInst->ent, 0xBC) & 2)) {
        sndChannel[0]->Play(&plrSndList[base + 3], 0);
        return;
    }
    objOBJ *o = gsScenePtr->GetCDTObj(&floorInfo);
    if (!o)
        return;
    txmTEXTURE *t = o->GetFaceTexRendMtl(unk_24B, 0);
    if (!t)
        return;
    int n = 0;
    for (int i = 0; gravel[i]; i++)
        if (!stricmp(gravel[i], t->name)) {
            n = 1;
            break;
        }
    if (!n)
        for (int i = 0; wood[i]; i++)
            if (!stricmp(wood[i], t->name)) {
                n = 2;
                break;
            }
    sndChannel[0]->Play(&plrSndList[base + n], 0);
}

#pragma pack(push, 1)
struct plrPICK_NOTIFY {                                 // MP 0x40E to the clients: the new count
    msgDATA h;
    short   type;                                       // +0x04 0x10 health, 6 armor, 0xC money, 5 ammo, 9 TH treasures
    union {
        float value;                                    // +0x06
        struct { u8 slot; short ammo; short unk; } a;   // +0x06 ammo: the slot, the new ammo, -2
        struct { int frags, unk217, treasures; } th;    // +0x06 TH
    };
    u8      pad;
};
#pragma pack(pop)

static void plrSendPick(plrPLAYER_COMMON *p, plrPICK_NOTIFY *m, int size, int type, int flags)
{
    m->h.packId = 0;
    m->h.tag = 0x40;
    m->h.size = (short)size;
    m->type = (short)type;
    gsSysServer->SendMsgClientAll(p, 0x40E, &m->h, flags, 0xFFFFFFFF, 0xFFFFFFFD);
}

// v12 0x00428450  (feb 0x004243B0)  slot +0xC4
// An item taken (not dead, only once the player is in the game: flags2 8): by its kind -- weapons 0x02..0x0A
// (PickWeapon, slot = kind; the bomb's box 0x1B a weapon until it is had), ammo 0x14..0x1C (PickAmmo, slot =
// kind - 0x12), health 0x28..0x2B, armor 0x2C..0x2F, money 0x30, a key 0x31, the power-ups 0x32 / 0x33, treasures
// 0x34..0x38 and 0x3C. Taken: the server passes it on to the clients (0x4C9), the statistics count it.
BOOL32 plrPLAYER_COMMON::ProcessPICK(void *d)
{
    if (wrEVCall<BOOL32>(this, 0x74))                   // IsDead
        return 0;
    if (!(flags2 & 8))
        return 0;
    int kind = *(int *)((u8 *)d + 4);
    float n = *(float *)((u8 *)d + 8);
    BOOL32 r = 0;
    if (kind >= 2 && kind <= 8)
        r = wrEVCall<BOOL32>(this, 0xDC, kind, n);
    else if (kind == 0x0A)
        r = wrEVCall<BOOL32>(this, 0xDC, 0x0A, n);
    else if (kind >= 0x14 && kind <= 0x1A)
        r = wrEVCall<BOOL32>(this, 0xE0, kind - 0x12, n);
    else if (kind == 0x1B)
        r = (weapons[9]->wpnWEAPON::flags & 1) ? wrEVCall<BOOL32>(this, 0xE0, 9, n) : wrEVCall<BOOL32>(this, 0xDC, 9, n);
    else if (kind == 0x1C)
        r = wrEVCall<BOOL32>(this, 0xE0, 0x0A, n);
    else if (kind >= 0x28 && kind <= 0x2B)
        r = wrEVCall<BOOL32>(this, 0xE4, n);
    else if (kind >= 0x2C && kind <= 0x2F)
        r = wrEVCall<BOOL32>(this, 0xE8, n);
    else if (kind == 0x30)
        r = wrEVCall<BOOL32>(this, 0xF4, n);
    else if (kind == 0x31)
        r = wrEVCall<BOOL32>(this, 0xF0);
    else if (kind == 0x32)
        r = wrEVCall<BOOL32>(this, 0xEC, 0);
    else if (kind == 0x33)
        r = wrEVCall<BOOL32>(this, 0xEC, 1);
    else if ((kind >= 0x34 && kind <= 0x38) || kind == 0x3C)
        r = wrEVCall<BOOL32>(this, 0xF8, n);
    else
        return 0;
    if (!r)
        return 0;
    if (entFlags & 4)
        gsSysServer->SendMsgClientAll(this, 0x4C9, (msgDATA *)d, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
    if (wrSysStat)
        wrSysStat->Notify_Pick(this, kind, n);
    return r;
}

// v12 0x00428770
// A weapon: had already, only what the item holds above the ammo (at least 1) is added; switched on; out of a
// replay, switched to by [GameOpt] AutoWeapon (1 a new one, 2 a higher slot, 3 always). The "got weapon" sound.
BOOL32 plrPLAYER_COMMON::PickWeapon(int w, float n)
{
    pwpWEAPON *p = weapons[w];
    if (!p)
        return 0;
    int had = p->wpnWEAPON::flags & 1;
    if (had) {
        n = n - p->ammo;
        if (!(n >= 1.0f))
            n = 1.0f;
    }
    p->AddAmmo(n);
    weapons[w]->Enable(1);
    if (!(entFlags & 1) && w != curWeapon) {
        switch ((cfg.bits >> 1) & 3) {
        case 1:
            if (!had)
                wrEVCall<void>(this, 0x108, w, 1);
            break;
        case 2:
            if (w > curWeapon)
                wrEVCall<void>(this, 0x108, w, 1);
            break;
        case 3:
            wrEVCall<void>(this, 0x108, w, 1);
            break;
        }
    }
    unk108 |= 0x8000;
    return 1;
}

// v12 0x00428840  (feb 0x00424760)
// Ammo (on the server; a full weapon takes none: not taken); the clients told the new count. The ammo sound.
BOOL32 plrPLAYER_COMMON::PickAmmo(int w, float n)
{
    if (entFlags & 4) {
        pwpWEAPON *p = weapons[w];
        if (!p)
            return 1;
        if (!p->AddAmmo(n))
            return 0;
        plrPICK_NOTIFY m;
        m.a.slot = (u8)w;
        m.a.ammo = (short)(int)weapons[w]->ammo;
        m.a.unk = -2;
        plrSendPick(this, &m, 0xB, 5, 0xA0000);
    }
    unk108 |= 0x10000;
    return 1;
}

// v12 0x00428900  (feb 0x00424810)
// Health (on the server): up to 100, a small (< 11) or a big (150 and more) one up to 200; at the cap not taken.
BOOL32 plrPLAYER_COMMON::PickHealth(float n)
{
    if (entFlags & 4) {
        float cap = (n < 11.0f || n >= 150.0f) ? 200.0f : 100.0f;
        if (!(health < cap))
            return 0;
        health += n;
        if (!(health <= cap))
            health = cap;
        plrPICK_NOTIFY m;
        m.value = health;
        m.pad = 0;
        plrSendPick(this, &m, 0xB, 0x10, 0xA0000);
    }
    unk108 |= 0x20000;
    return 1;
}

// v12 0x00428A30  (feb 0x00424920)
// Armor (on the server): up to 100, a big one (150 and more) up to 200; at the cap not taken.
BOOL32 plrPLAYER_COMMON::PickArmor(float n)
{
    if (entFlags & 4) {
        float cap = n < 150.0f ? 100.0f : 200.0f;
        if (!(armor < cap))
            return 0;
        armor += n;
        if (!(armor <= cap))
            armor = cap;
        plrPICK_NOTIFY m;
        m.value = armor;
        plrSendPick(this, &m, 0xA, 6, 0xA0000);
    }
    unk108 |= 0x40000;
    return 1;
}

// v12 0x00428B40
BOOL32 plrPLAYER_COMMON::PickKey()
{
    moveFlags |= 0x1000;                                // PLR_MF_KEY
    unk108 |= 0x80000;
    return 1;
}

// v12 0x00428B70  (feb 0x00424A20)
// A treasure (on the server): in TH one more carried (a player of a team only), else its gold.
BOOL32 plrPLAYER_COMMON::PickTreasure(float n)
{
    if (entFlags & 4) {
        plrPICK_NOTIFY m;
        if (WR_FIELD(int, wrSysGO, 0x98) == 3) {
            if (team != 0 && team != 1)
                return 0;
            treasures++;
            m.th.frags = frags;
            m.th.unk217 = unk_217;
            m.th.treasures = treasures;
            plrSendPick(this, &m, 0x12, 9, 0x20000);
        } else {
            money += n;
            m.value = money;
            plrSendPick(this, &m, 0xA, 0xC, 0x20000);
        }
    }
    unk108 |= 0x100000;
    return 1;
}

// v12 0x00428C70  (feb 0x00424AF0)
BOOL32 plrPLAYER_COMMON::PickMoney(float n)
{
    if (entFlags & 4) {
        money += n;
        plrPICK_NOTIFY m;
        m.value = money;
        plrSendPick(this, &m, 0xA, 0xC, 0x20000);
    }
    unk108 |= 0x200000;
    return 1;
}

extern float _plrPwrCostTbl[3];                         // v12 0x0062B868 (player.cpp)

// v12 0x00428CF0  (feb 0x00424B60)
// A power-up not had yet: free in DM / TH (wrGO modes 1 / 3; the quad damage / unholy armor sound), else bought
// with its gold.
BOOL32 plrPLAYER_COMMON::PickPowerup(int i)
{
    int mode = WR_FIELD(int, wrSysGO, 0x98);
    if (mode == 3 || mode == 1) {
        if (powerup[i].state)
            return 0;
        if (i == 0)
            unk108 |= 0x1000000;
        else if (i == 1)
            unk108 |= 0x2000000;
        powerup[i].state = 1;
        powerup[i].timeLeft = 0;
        return 1;
    }
    if (money < _plrPwrCostTbl[i] || powerup[i].state)
        return 0;
    powerup[i].state = 1;
    powerup[i].timeLeft = 0;
    money -= _plrPwrCostTbl[i];
    return 1;
}

// v12 0x0042A770  (feb 0x00426250)
const wrWCHAR *plrPLAYER_COMMON::WpnType2String(int w)
{
    return gsStrings->GetStringById(gsStrings->GetStringIdFmt("WPN_%i", w));
}

#pragma pack(push, 1)
struct plrSTATS_MSG : msgDATA { short type; int frags, deaths, treasures; };    // 0x12 bytes (0x40E type 9)
struct plrSPAWN_MSG : msgDATA { short type; u8 full, keepPos; m3dV pos, dir; };  // 0x20 bytes (0x40E type 4)
struct plrREFUSE_MSG : msgDATA { short type; };                                 // 6 bytes (0x40E type 0x13)
#pragma pack(pop)

// v12 0x00429FC0  (feb 0x00425CE0)  slot +0x118
// On the server while the game is not over: the frags changed by n, then frags, deaths and treasures to the clients.
void plrPLAYER_COMMON::UpdateKillsCount(int n)
{
    plrSTATS_MSG s;
    s.packId = 0;
    s.frags = 0;
    s.tag = 0x40;
    s.deaths = 0;
    s.type = 9;
    s.treasures = 0;
    s.size = 0x12;
    if (!(entFlags & 4) || (gsAppState & 0x80000))
        return;
    frags += n;
    s.deaths = unk_217;
    s.frags = frags;
    s.treasures = treasures;
    gsSysServer->SendMsgClientAll(this, 0x40E, &s, 0x20000, -1, -3);
}

// v12 0x004291C0  (feb 0x00424F60)
// Once. On the server of a multiplayer game: one more death, sent with the frags and treasures (type 9); a kill to the
// killer -- a player other than this one -- else a suicide (-1) to this one, both told to the statistics; the kill
// message (GS_MSG_MP_SHOW_MSG: the names and the weapon) to every ready client; the treasures dropped (3 scattered,
// then all that are left). Then the statistics hear of the death; a death by the world's subtype 3 tears the body to
// pieces (moveFlags 0x10000: gone; the gibs on the server, the blood everywhere), as does one while 0x40000000 is
// set; else the death cry by the damage's subtype (4: 0x2000, 5: 0x4000, other: 0x1000). Dead (moveFlags 2).
void plrPLAYER_COMMON::StartDeath(plrEV_START_DEATH *m)
{
    if (!(moveFlags & 2) && (entFlags & 4) && WR_FIELD(int, gsSysMP, 0x28) != 1) {
        plrSTATS_MSG s;
        s.packId = 0;
        s.tag = 0x40;
        s.type = 9;
        s.size = 0x12;
        unk_217++;
        s.frags = frags;
        s.deaths = unk_217;
        s.treasures = treasures;
        gsSysServer->SendMsgClientAll(this, 0x40E, &s, 0xA0000, -1, -3);
        entIACTIVE_OBJ *killer = 0;
        entIACTIVE_OBJ *k = gsSysMP->FindMPID(m->source, 0);
        if (k && k != this && ((entENTITY *)k)->classId == 0x504C5952) {     // 'PLYR'
            killer = k;
            wrEVCall<void>(k, 0x118, 1);                // UpdateKillsCount
            if (wrSysStat)
                wrSysStat->Notify_Kill((plrPLAYER_COMMON *)k, 1);
        } else {
            wrEVCall<void>(this, 0x118, -1);
            if (wrSysStat)
                wrSysStat->Notify_Kill(this, -1);
        }
        gsMSG_MP_SHOW_MSG km;
        km.packId = 0;
        km.tag = 0x40;
        km.killer[0] = 0;
        km.victim[0] = 0;
        km.size = 0x8C;
        km.type = 0;
        km.weapon = m->body;
        if (killer) {
            char *n = ((entENTITY *)killer)->inst->name;
            ((apNAME *)km.killer)->SetName(n ? n : (char *)"");
        }
        char *n = inst->name;
        ((apNAME *)km.victim)->SetName(n ? n : (char *)"");
        gsSysServer->SendMsgClientAll(0, 0x40A, &km, 0x90000, -3, -3);
        ReleaseTreasures(1, 3);
        ReleaseTreasures(0, treasures);
    }
    if (!(moveFlags & 2)) {
        if (wrSysStat)
            wrSysStat->Notify_Death(this);
        if (m->world3) {
            m3dV p;
            moveFlags |= 0x10000;
            inst->GetPos(&p);
            if (entFlags & 4)
                wrsfxStartChunkMeat(3, 0, &p, 0, 0);
            wrsfxStartBlood(7, 0, &p, 0, 0, 0);
        }
        if (moveFlags & 0x40000000) {
            moveFlags |= 0x10000;
        } else if (m->sub == 4) {
            unk108 |= 0x2000;
        } else if (m->sub == 5) {
            unk108 |= 0x4000;
        } else {
            unk108 |= 0x1000;
        }
    }
    moveFlags |= 2;
}

// v12 0x00427B80  (feb 0x00423D90)  slot +0x08C
// The notify types both kinds of player take (plgsACTION's first): -100 the kept state restored (the weapons, the
// powerups, the current weapon, health, armor, flags, gold, the outfit, the counts; TH: the team), 13 / 14 a powerup
// used / ended, 16 the health (lower: hurt), 11 a team asked for (the server may refuse it, auto-balance: 0x13 back to
// the asker), 19 refused (the HUD says so), 3 a respawn asked for (the server picks the start, or the place where the
// player is, and sends 4 to its client), 17 to a weapon. A multiplayer server passes the others on to every client
// (1 and 8 not back to the sender's).
void plrPLAYER_COMMON::ProcessMP_NOTIFY(void *d)
{
    u8 *m = (u8 *)d;
    unsigned long skip = (unsigned long)-3;
    plrSPAWN_MSG r;
    r.packId = 0;
    r.tag = 0x40;
    r.full = 0;
    r.keepPos = 0;
    r.type = 4;
    r.size = 0x20;
    plgsACTION::ProcessMP_NOTIFY(d);
    switch (*(short *)(m + 4)) {
    case -100: {
        u8 *s = *(u8 **)(m + 6);                        // the kept state
        for (int i = 0; i < PLR_WPN_COUNT; i++) {
            u8 *w = s + 0x4D8 + i * 0xC;
            wrEVCall<void>(weapons[i], 0xA8, *(int *)w);                 // Enable
            weapons[i]->SetAmmo(*(float *)(w + 4));
            weapons[i]->clip = *(int *)(w + 8);
        }
        for (int i = 0; i < PLR_PWR_COUNT; i++)
            powerup[i] = ((plrPOWERUP *)(s + 0x568))[i];
        wrEVCall<void>(this, 0x108, *(int *)(s + 0x4D4), 0);           // SetCurWpn
        health = *(float *)(s + 0x4C0);
        armor = *(float *)(s + 0x4C4);
        moveFlags = *(unsigned *)(s + 0x4C8);
        money = *(float *)(s + 0x4D0);
        wrEVCall<void>(this, 0x124, *(int *)(s + 0x4CC));              // SelectSkin
        frags = *(int *)(s + 0x580);
        unk_217 = *(int *)(s + 0x584);
        treasures = *(int *)(s + 0x588);
        if (wrSysGO->mode == 3)
            wrEVCall<void>(this, 0x128, *(int *)(s + 0x58C));          // SetTeam
        break;
    }
    case 13:
        wrEVCall<void>(this, 0xFC, 0, m + 6);                          // ProcessPowerup
        break;
    case 14:
        wrEVCall<void>(this, 0xFC, 1, m + 6);
        break;
    case 16:
        if (*(float *)(m + 6) < health) {
            unk108 |= 0x800;
            flags2 |= 0x20;
        }
        health = *(float *)(m + 6);
        break;
    case 11:
        if (moveFlags & 0x40000000)
            return;
        if ((entFlags & 4) && m[0xA] && wrSysGO->autoBalanceTeam && !wrSysGO->IsCanChangeTeam(this)) {
            plrREFUSE_MSG f;
            f.packId = 0;
            f.tag = 0x40;
            f.size = 6;
            f.type = 0x13;
            gsSysServer->SendMsgClient(mpOwner, this, 0x40E, &f, 0x10000);
            return;
        }
        wrEVCall<void>(this, 0x128, *(int *)(m + 6));                  // SetTeam
        return;
    case 19:
        uiSystem->ShowMsgW(gsStrings->Printf("GAME_PLAYER_CANNOT_CHANGE_TEAM"), 3.0f, 0);
        return;
    case 1:
    case 8:
        skip = mpOwner;
        break;
    case 3: {
        m3dV pos, dir;
        if (!m[7]) {
            gsSysMP->SelectStartPosDir(&pos, &dir, this);
        } else {
            inst->GetPos(&pos);
            inst->GetDir(&dir);
        }
        r.full = m[6];
        r.keepPos = m[7];
        r.pos = pos;
        r.dir = dir;
        gsSysServer->SendMsgClient(mpOwner, this, 0x40E, &r, 0x20000);
        flags2 |= 0x100;
        spawnPos = pos;
        return;
    }
    case 17:
        msgSystem.SendMsg(weapons[m[6] >> 1], 0x40E, m, 0);
        break;
    }
    if (WR_FIELD(int, gsSysMP, 0x28) != 1 && (entFlags & 4))
        gsSysServer->SendMsgClientAll(this, 0x40E, (msgDATA *)m, 0x20000, -1, skip);
}
