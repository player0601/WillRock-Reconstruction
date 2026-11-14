// wr_stat.cpp -- wrSTAT_SYSTEM, the per-player statistics of a game (Will Rock 1.2, v12.exe; feb obj wr_stat.obj)
//
// Every player has a record (+0x124, 0x204 bytes each, +0x120 of them) found by its index (plrPLAYER_COMMON +0x21F):
// +0x40 the shooting accuracy in percent, +0x4C shots, +0x50 hits, +0x64 / +0x68 damage taken (kind 4 / other),
// +0x7C per weapon { shots, hits, .. } (0x10 each) and +0x14C the same carried over.
//
// Rebuilt so far: Notify_WpnHit, Notify_WpnShot, Notify_PlrHit, Notify_CreateItem, Notify_GameBegin. Not rebuilt: the rest (wr_game.h lists them).

#include "game.h"
#include "../gs/gs_mp_misc.h"   // gsMP_SYSTEM::FindMPID
#include "../engine/engine.h"   // dsDYN_ARRAY

wrSTAT_SYSTEM *wrSysStat;                               // v12 0x006500D0

// The player's record (the index checked; a remote player not yet announced in a running level announces itself,
// 0x408, as plrPLAYER_MP::ProcessFRAME does); 0 when it has none. (Inline in each Notify_*.)
static char *statRecord(wrSTAT_SYSTEM *s, plrPLAYER_COMMON *p)
{
    int n = WR_FIELD(int, s, 0x120);
    if (p->unk_21F < 0 || p->unk_21F >= n) {
        if (!(gsAppState & 0x40000))
            return 0;
        if (!(p->flags2 & 0x10) && WR_FIELD(int, p, 0x98) == 1) {
            msgSystem.SendAll(0x408, p, 0);
            p->flags2 |= 0x10;
        }
        if (p->unk_21F < 0 || p->unk_21F >= n)
            return 0;
    }
    return WR_FIELD(char *, s, 0x124) + p->unk_21F * 0x204;
}

// v12 0x004AE0F0  (feb 0x0049F930)
// A kill for the player who did it (single player: the player, whoever did it).
void wrSTAT_SYSTEM::Notify_NPCDie(unsigned long id)
{
    plrPLAYER_COMMON *p = (plrPLAYER_COMMON *)gsSysMP->FindMPID(id, 0);
    if (!p || p->classId != 0x504C5952) {
        if (gsSysMP->mode != 1)
            return;
        p = plrPlayer;
    }
    char *r = statRecord(this, p);
    if (r)
        WR_FIELD(int, r, 0x5C)++;
}

// v12 0x004AE660  (name ours)
// Every frame: the time the player has held this weapon (its record's +0x88 + 16 * weapon index).
void wrSTAT_SYSTEM::Notify_WpnTime(plrPLAYER_COMMON *p, pwpWEAPON *w)
{
    if (char *r = statRecord(this, p))
        *(float *)(r + 0x88 + w->wpnIndex * 16) += gsElapsedTime;
}

// The accuracy: hits * 100 / shots over all weapons (this game's and the carried-over counts), rounded up.
static void statAccuracy(char *r)
{
    int shots = 0, hits = 0;
    for (int i = 0; i < 11; i++) {
        float *w = (float *)(r + 0x7C + i * 0x10);
        shots += (int)(w[0x34] + w[0]);
        hits += (int)(w[0x35] + w[1]);
    }
    WR_FIELD(int, r, 0x40) = shots ? (hits * 100 + shots - 1) / shots : 0;
}

// v12 0x004AE1B0  (feb ?)
// A weapon of player mpId hit something: its hits counted.
void wrSTAT_SYSTEM::Notify_WpnHit(unsigned long id, int wpn)
{
    plrPLAYER_COMMON *p = (plrPLAYER_COMMON *)gsSysMP->FindMPID(id, 0);
    if (!p || p->classId != 0x504C5952)
        return;
    char *r = statRecord(this, p);
    if (!r)
        return;
    WR_FIELD(float, r, 0x80 + wpn * 0x10) += 1.0f;
    WR_FIELD(int, r, 0x50)++;
    statAccuracy(r);
}

// v12 0x004AE710  (feb ?)
// A shot: the weapon's shots counted.
void wrSTAT_SYSTEM::Notify_WpnShot(plrPLAYER_COMMON *p, pwpWEAPON *w)
{
    char *r = statRecord(this, p);
    if (!r)
        return;
    WR_FIELD(float, r, 0x7C + w->wpnIndex * 0x10) += 1.0f;
    WR_FIELD(int, r, 0x4C)++;
    statAccuracy(r);
}

// v12 0x004AE830  (feb 0x0049FDC0)
// Damage the player took: kind 4 (+4 byte) counted apart.
void wrSTAT_SYSTEM::Notify_PlrHit(plrPLAYER_COMMON *p, dmgDAMAGE *d)
{
    char *r = statRecord(this, p);
    if (!r)
        return;
    float v = *(float *)((char *)d + 6);
    if (*((u8 *)d + 4) == 4)
        WR_FIELD(float, r, 0x64) += v;
    else
        WR_FIELD(float, r, 0x68) += v;
}

// v12 0x004AE2F0  (feb 0x0049FA80)
// An item made in the level: its amount added to the level's totals (what the end screen measures against) -- the
// weapons' (types 0..10, +0xB0), the 20..28 ones' (+0x60), and three groups (40..43 +0xA0, 44..47 +0xA4, 48 and
// 52..56 +0xA8); the other types are not counted.
void wrSTAT_SYSTEM::Notify_CreateItem(int type, float val)
{
    if ((unsigned)type > 0x38)
        return;
    if (type <= 10)
        WR_FIELD(float, this, 0xB0 + type * 4) += val;
    else if (type >= 20 && type <= 28)
        WR_FIELD(float, this, 0x60 + type * 4) += val;
    else if (type >= 40 && type <= 43)
        WR_FIELD(float, this, 0xA0) += val;
    else if (type >= 44 && type <= 47)
        WR_FIELD(float, this, 0xA4) += val;
    else if (type == 48 || type >= 52)
        WR_FIELD(float, this, 0xA8) += val;
}

// v12 0x004AE8E0  (feb 0x004A0020)
// A new game: no player records; their growing array (0x204 each, 0x10 at a time) set up empty.
void wrSTAT_SYSTEM::Notify_GameBegin()
{
    WR_FIELD(int, this, 0x120) = 0;
    WR_FIELD(void *, this, 0x124) = 0;
    ((dsDYN_ARRAY *)((char *)this + 0x128))->Init(0x204, 0x10, (void **)((char *)this + 0x124), 0, 0);
}

// v12 0x004AE910  (feb 0x004A0050)
void wrSTAT_SYSTEM::Notify_GameEnd()
{
    if (WR_FIELD(int, this, 0x120)) {
        ((dsDYN_ARRAY *)((char *)this + 0x128))->ReleaseMem();
        WR_FIELD(int, this, 0x120) = 0;
    }
}

// v12 0x004AE940  (feb 0x004A0080)
// A player's record: its old one when one has the player's name (a player back in the game), else a new one.
void wrSTAT_SYSTEM::Notify_AddPlayer(plrPLAYER_COMMON *p)
{
    if (p->unk_21F != -1)
        return;
    char *pname = p->inst->name ? p->inst->name : (char *)"";
    char *recs = WR_FIELD(char *, this, 0x124);
    for (int i = 0; i < WR_FIELD(int, this, 0x120); i++)
        if (((apNAME *)(recs + i * 0x204))->IsName(pname)) {
            p->unk_21F = i;
            WR_FIELD(plrPLAYER_COMMON *, recs + i * 0x204, 0x200) = p;
            WR_FIELD(int, recs + i * 0x204, 0x1FC) = 1;
            return;
        }
    if (!((dsDYN_ARRAY *)((char *)this + 0x128))->AddEntry(1))
        return;
    p->unk_21F = WR_FIELD(int, this, 0x120);
    WR_FIELD(int, this, 0x120)++;
    char *r = WR_FIELD(char *, this, 0x124) + p->unk_21F * 0x204;
    memset(r, 0, 0x204);
    ((apNAME *)r)->SetName(pname);
    WR_FIELD(int, r, 0x1FC) = 1;
    WR_FIELD(plrPLAYER_COMMON *, r, 0x200) = p;
}

// v12 0x004AEAB0  (feb 0x004A0190)
// A player gone: its record stays, without it.
void wrSTAT_SYSTEM::Notify_StopPlayer(plrPLAYER_COMMON *p)
{
    char *r = statRecord(this, p);
    if (r) {
        p->unk_21F = -1;
        WR_FIELD(int, r, 0x1FC) = 0;
        WR_FIELD(plrPLAYER_COMMON *, r, 0x200) = 0;
    }
}

// v12 0x004AE3C0  (name ours)
// An item taken: its amount into the player's record -- weapons 0x00..0x0A (+0x84 + 16 * kind), ammo 0x14..0x1C
// (16 * kind - 0x9C), health 0x28..0x2B (+0x6C), armor 0x2C..0x2F (+0x70), money and treasures 0x30, 0x34..0x38
// (+0x74).
void wrSTAT_SYSTEM::Notify_Pick(plrPLAYER_COMMON *p, int kind, float n)
{
    char *r = statRecord(this, p);
    if (!r || (unsigned)kind > 0x38)
        return;
    if (kind <= 0x0A)
        *(float *)(r + 0x84 + kind * 16) += n;
    else if (kind >= 0x14 && kind <= 0x1C)
        *(float *)(r + kind * 16 - 0x9C) += n;
    else if (kind >= 0x28 && kind <= 0x2B)
        *(float *)(r + 0x6C) += n;
    else if (kind >= 0x2C && kind <= 0x2F)
        *(float *)(r + 0x70) += n;
    else if (kind == 0x30 || (kind >= 0x34 && kind <= 0x38))
        *(float *)(r + 0x74) += n;
}

// v12 0x004AE520  (name ours)
// A death: the record's deaths (+0x60) and the current streak (+0x54) ended.
void wrSTAT_SYSTEM::Notify_Death(plrPLAYER_COMMON *p)
{
    char *r = statRecord(this, p);
    if (!r)
        return;
    *(int *)(r + 0x54) = 0;
    (*(int *)(r + 0x60))++;
}

// v12 0x004AE5C0  (name ours)
// A kill (n 1) or a suicide (n -1): the record's frags (+0x48); a suicide ends the streak (+0x54).
void wrSTAT_SYSTEM::Notify_Kill(plrPLAYER_COMMON *p, int n)
{
    char *r = statRecord(this, p);
    if (!r)
        return;
    *(int *)(r + 0x48) += n;
    if (n < 0)
        *(int *)(r + 0x54) = 0;
}

// v12 0x004AD0D0  (name ours)
char *wrSTAT_SYSTEM::GetPlrRecord(plrPLAYER_COMMON *p)
{
    return statRecord(this, p);
}

// v12 0x004AD2EC  (ProcessMsg's GS_MSG_FRAME case; name ours)
// The level's time; each player's time in the game and, alive, in this life (its longest life kept).
void wrSTAT_SYSTEM::ProcessFRAME()
{
    WR_FIELD(float, this, 0xAC) += gsElapsedTime;
    int n = WR_FIELD(int, this, 0x120);
    for (int i = 0; i < n; i++) {
        char *r = WR_FIELD(char *, this, 0x124) + i * 0x204;
        if (!WR_FIELD(int, r, 0x1FC))
            continue;
        WR_FIELD(float, r, 0x44) += gsElapsedTime;
        plrPLAYER_COMMON *p = WR_FIELD(plrPLAYER_COMMON *, r, 0x200);
        if (p->IsDead())
            continue;
        WR_FIELD(float, r, 0x54) += gsElapsedTime;
        if (WR_FIELD(float, r, 0x54) > WR_FIELD(float, r, 0x58))
            WR_FIELD(float, r, 0x58) = WR_FIELD(float, r, 0x54);
    }
}

// v12 0x004AEB60  (feb 0x004A0220)
// The level's time (this level's and the carried-over).
float wrSTAT_SYSTEM::GetMapTime()
{
    return WR_FIELD(float, this, 0xF0) + WR_FIELD(float, this, 0xAC);
}

// v12 0x004AEB70  (feb 0x004A0230)
// The player's kills, and (total) the opponents the level had.
int wrSTAT_SYSTEM::GetMapKills(plrPLAYER_COMMON *p, int *total)
{
    char *r = statRecord(this, p);
    if (!r)
        return 0;
    *total = WR_FIELD(int, this, 0xDC) + WR_FIELD(int, this, 0x98);
    return WR_FIELD(int, r, 0x12C) + WR_FIELD(int, r, 0x5C);
}

// v12 0x004AEC20  (feb 0x004A02D0)
int wrSTAT_SYSTEM::GetMapSecrets(plrPLAYER_COMMON *p, int *total)
{
    char *r = statRecord(this, p);
    if (!r)
        return 0;
    *total = WR_FIELD(int, this, 0xE0) + WR_FIELD(int, this, 0x9C);
    return WR_FIELD(int, r, 0x148) + WR_FIELD(int, r, 0x78);
}

// v12 0x004AECD0  (feb 0x004A0370)
int wrSTAT_SYSTEM::GetMapMoney(plrPLAYER_COMMON *p, int *total)
{
    char *r = statRecord(this, p);
    if (!r)
        return 0;
    *total = (int)(WR_FIELD(float, this, 0xEC) + WR_FIELD(float, this, 0xA8));
    return (int)(WR_FIELD(float, r, 0x144) + WR_FIELD(float, r, 0x74));
}

// v12 0x004AED80  (feb 0x004A0410)
// The weapon held longest (-1: none).
int wrSTAT_SYSTEM::GetFavouriteWpn(plrPLAYER_COMMON *p)
{
    char *r = statRecord(this, p);
    if (!r)
        return 0;
    int best = -1;
    float t = -1.0f;
    for (int i = 0; i < 11; i++) {
        float v = WR_FIELD(float, r, 0x88 + i * 16 + 0xD0) + WR_FIELD(float, r, 0x88 + i * 16);
        if (t < v) {
            t = v;
            best = i;
        }
    }
    return best;
}

// v12 0x004AEE50  (feb 0x004A04D0)
// Shots and hits over all weapons; hits / (shots + 0.01), at most 100.
float wrSTAT_SYSTEM::GetShootAccuracy(plrPLAYER_COMMON *p, int *shots, int *hits)
{
    char *r = statRecord(this, p);
    if (!r)
        return 0.0f;
    *shots = 0;
    *hits = 0;
    for (int i = 0; i < 11; i++) {
        *shots += (int)(WR_FIELD(float, r, 0x7C + i * 16 + 0xD0) + WR_FIELD(float, r, 0x7C + i * 16));
        *hits += (int)(WR_FIELD(float, r, 0x80 + i * 16 + 0xD0) + WR_FIELD(float, r, 0x80 + i * 16));
    }
    float a = (float)*hits / ((float)*shots + 0.01f);
    return a > 100.0f ? 100.0f : a;
}
