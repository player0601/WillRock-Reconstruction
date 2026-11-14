// ai_cent.cpp -- the centaur ("ai_cent"), the skeleton centaur ("ai_sketavr") and their trident ("pjl_trident") (Will
// Rock 1.2, v12.exe; feb objs ai_cent.obj, ai_sketavr.obj)
//
// The centaur gallops (20 m/s, slowing to 12 when the goal is far off its heading), in long straight runs at the enemy or
// round it (the planner's flee points); close up it charges: ATTACK's hook catches the enemy (18 damage, flung back and up
// at 28) and tramples what is in front (10 damage, knocked aside at 10). Between 10 (50 in the open) and 150 m, facing it
// within 20 degrees, it throws a trident (15 damage) every 4..8 s. It neighs now and then (9 s). Its "plr_hook" is
// armoured; a rocket or a gibbing weapon blows it apart. The skeleton centaur: 30 hp, its own sounds, and on death it
// shatters (the skeletons' rigid pieces).

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../../include/ai/pjl_arrow.h"
#include "../../include/ai/ai_world.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list);  // v12 0x004B1E30
void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int skip);  // v12 0x005130D0

// v12 0x00649A60.. 11 sounds, 0x68 apart (static initialisers v12 0x0043A350..0x0043A6AF)
static sncSOUND_DESCR_3D centSnd[11];
enum { CSND_APPEAR, CSND_NEIGH_1, CSND_NEIGH_2, CSND_NEIGH_3, CSND_DEATH, CSND_GALLOP, CSND_EXPLOS, CSND_TRIDENT_THROW,
       CSND_TRIDENT_FLY, CSND_TRIDENT_HIT_GROUND, CSND_TRIDENT_HIT_BODY };
// v12 0x0064D620.. the skeleton centaur's 4 (v12 0x0045C5E0..0x0045C82F)
static sncSOUND_DESCR_3D sketSnd[4];

static void centSndSet(sncSOUND_DESCR_3D *d, const char *name, int flags, int delay, float mn, float mx)
{
    d->name.SetName((char *)name);
    d->flags = flags;
    d->volume = 0.0f;
    d->delay = delay;
    d->count = 0;
    d->time = 0;
    d->data = 0;
    d->dataFlags = 0;
    d->minDist = mn;
    d->maxDist = mx;
    d->vtbl = 0;
}

static struct aiCENT_SND_INIT {
    aiCENT_SND_INIT()
    {
        centSndSet(&centSnd[0], "cent_appear", 0x40000, 0, 25.5f, 70.0f);
        centSndSet(&centSnd[1], "cent_neigh_01", 0x40000, 0, 25.5f, 70.0f);
        centSndSet(&centSnd[2], "cent_neigh_02", 0x40000, 0, 25.5f, 70.0f);
        centSndSet(&centSnd[3], "cent_neigh_03", 0x40000, 0, 25.5f, 70.0f);
        centSndSet(&centSnd[4], "cent_death", 0x20000, 2000, 30.0f, 80.0f);
        centSndSet(&centSnd[5], "cent_gallop", 0x10002, 0, 20.0f, 60.0f);
        centSndSet(&centSnd[6], "char_explos", 0x40000, 0, 25.5f, 70.0f);
        centSndSet(&centSnd[7], "cent_trident_throw", 0x440000, 250, 30.0f, 80.0f);
        centSndSet(&centSnd[8], "cent_trident_fly", 0x10002, 0, 4.0f, 20.0f);
        centSndSet(&centSnd[9], "cent_trident_hit_ground", 0x20000, 0, 4.0f * 1.3f, 20.0f * 1.5f);
        centSndSet(&centSnd[10], "cent_trident_hit_body", 0x20000, 0, 4.0f * 1.3f, 20.0f * 1.5f);
        centSndSet(&sketSnd[0], "char_appear", 0x40000, 0, 25.5f, 70.0f);
        centSndSet(&sketSnd[1], "opp_sfx_crossbow_crash", 0x20000, 2000, 30.0f, 80.0f);
        centSndSet(&sketSnd[2], "cent_gallop", 0x10002, 0, 20.0f, 60.0f);
        centSndSet(&sketSnd[3], "cent_trident_throw", 0x440000, 250, 30.0f, 80.0f);
    }
} aiCentSndInit;

// ---------------------------------------------------------------------------------------------- the trident

// pjlTRIDENT: the satyr's arrow (its code, size 0x208) with the centaur's sounds (0 flying, 1 the level, 2 a body)
#pragma pack(push, 1)
class pjlTRIDENT : public pjlARROW_GENERIC {
public:
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x0045CCD0 (feb 0x00454160)
    sncSOUND_DESCR_3D *GetSound(int i);                 // v12 0x0043C450 (feb 0x00435FC0)
};
#pragma pack(pop)

entENTITY *__fastcall pjlTRIDENT::Create(animINST *)
{
    return new pjlTRIDENT;
}

sncSOUND_DESCR_3D *pjlTRIDENT::GetSound(int i)
{
    if (i < 0 || i > 2)
        return 0;
    return &centSnd[CSND_TRIDENT_FLY + i];
}

// v12 0x0045CC50  (feb 0x00454110)
int __fastcall pjlTridentInit()
{
    return entRegisterClass((char *)"pjl_trident", 0x41525257, 0x32, pjlTRIDENT::Create, 0, 1) != 0;
}

// ---------------------------------------------------------------------------------------------- helpers

// v12 0x0043A700  (feb 0x00434220)
// p within r of the segment a..b (a sphere of radius r round p met by the ray from a, as far as b; a point segment:
// p within r of a).
static int __fastcall m3dIsBelongPointCyl(m3dV *p, m3dV *a, m3dV *b, float r)
{
    float dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;
    float len = m3dSqrt(dx * dx + dy * dy + dz * dz);
    m3dV d;
    d.x = b->x - a->x;
    d.y = b->y - a->y;
    d.z = b->z - a->z;
    if (fabsf(len) < 0.001f) {
        float ex = a->x - p->x, ey = a->y - p->y, ez = a->z - p->z;
        return m3dSqrt(ex * ex + ey * ey + ez * ez) < r;
    }
    m3dNormalize(&d);
    // (m3dSPHERE{p, r}::IsectRay(a, d, len): the ray meets the sphere within len)
    float lx = a->x - p->x, ly = a->y - p->y, lz = a->z - p->z;
    float bb = lx * d.x + ly * d.y + lz * d.z;
    float c = lx * lx + ly * ly + lz * lz - r * r;
    if (c <= 0.0f)
        return 1;
    float disc = bb * bb - c;
    if (disc < 0.0f)
        return 0;
    float t = -bb - m3dSqrt(disc);
    return t >= 0.0f && t <= len;
}

// ---------------------------------------------------------------------------------------------- the centaur

// v12 0x0043A6B0  (feb 0x004341D0) exact
int __fastcall aiSketavrInit();
int __fastcall aiCentInit()
{
    if (!entRegisterClass((char *)"ai_cent", 0x244E5043, 0x32, aiCENT::Create, (char *)"ai_cent", 0))
        return 0;
    if (!aiSketavrInit())
        return 0;
    if (!pjlTridentInit())
        return 0;
    return sncLinkSndListData(11, centSnd) != 0;
}

// v12 0x0043AA00  (feb 0x00434530)
entENTITY *__fastcall aiCENT::Create(animINST *)
{
    aiCENT *c = new aiCENT;                             // operator new(0x5BB), aiNPC ctor
    if (!c)
        return 0;
    c->wrFlags = 0;
    c->bloodObj = 0;
    c->hookObj = 0;
    c->voice = 0;
    c->unk98 = 1;
    return c;
}

// v12 0x0043AA50  (feb 0x00434580)
// UNUSED (half a second), APPEAR (-> RUN), the RUN / ATTACK_RUN cycles, ATTACK, BEING_HIT (from anything), DEATH,
// NEIGH, the throw (RUN2THROW, THROW, THROW2RUN).
BOOL32 aiCENT::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 14;
    u32 *blk = (u32 *)operator new(0x774);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 14;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 14; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    static const char *names[14] = { "CENT_SEQ_UNUSED", "CENT_SEQ_APPEAR", "CENT_SEQ_APPEAR2RUN", "CENT_SEQ_RUN",
                                     "CENT_SEQ_ATTACK", "CENT_SEQ_BEING_HIT", "CENT_SEQ_DEATH", "CENT_SEQ_ATTACK_HOOK",
                                     "CENT_SEQ_ATTACK_THROW", "CENT_SEQ_ATTACK_RUN", "CENT_SEQ_NEIGH",
                                     "CENT_SEQ_RUN2THROW", "CENT_SEQ_THROW", "CENT_SEQ_THROW2RUN" };
    for (int i = 0; i < 14; i++)
        _entAnimSetSeqName(t, i, (char *)names[i]);
    entAnimAssignTplSeq(t, old, nOld);
    t->seq[0].frameStart = 0.0f;
    t->seq[0].frameEnd = 1.0f;
    t->seq[0].lengthMs = 500.0f;
    t->seq[0].dir = 0;
    animSYSTEM *s = new animSYSTEM(14, 0);
    if (!s)
        return 0;
    t->aniSystem = s;
    s->SetTranTypeToAny(0, 0, 1, -1);
    s->SetTranCycleAnim(3);
    s->SetTranCycleAnim(9);
    s->SetTranTypeToAny(1, 1, 1, 3);
    s->SetTranParam(1, 3, 1, 2, 1);
    s->SetTranParam(3, 4, 0, 4, 1);
    s->SetTranParam(4, 3, 1, 3, 0);
    s->SetTranParam(10, 3, 0, 13, 1);
    s->SetTranParam(10, 4, 0, 4, 1);
    s->SetTranParam(6, 6, 2, 6, 1);
    s->SetTranParamFromAny(5, 0, 1, 5);
    s->SetTranCycleAnim(5);
    s->SetTranParam(3, 12, 1, 11, 1);
    s->SetTranParam(9, 12, 1, 11, 1);
    s->SetTranParam(12, 3, 1, 13, 1);
    s->SetTranParam(12, 9, 1, 13, 1);
    s->SetTranParam(11, 10, 1, 12, 1);
    if (old)
        operator delete((u32 *)old - 1);
    return 1;
}

// v12 0x0043AD60  (feb 0x00434890)
// 70 hp; 20 m/s, turning 120 deg/s round 14 m; a sphere voice (20..100); the hook ("plr_hook", else "ATTACKfocus") is
// also the trident's start; throws within 20 degrees at 50..150 m; spawned: APPEAR (Activate at once); bleeds from
// "ROOT_2" (else "CENTRE"); "cent_colis_center" kept; a cloud of dust following it.
int aiCENT::ProcessINIT(void *init)
{
    radActivate = 200.0f;
    radActivateBack = 200.0f;
    radCDT = 0.7f;
    heightCDT = 1.4f;                                   // (1.2)
    unk17B = 0.9f;                                      // (1.2)
    heightUp1 = 2.0f;                                   // (1.2)
    moveSpeed = 20.0f;
    minTurnRadius = 14.0f;
    navArriveDist = 1.0f;
    turnSpeed = 120.0f;
    navUnk1A7 = 40.0f;
    maxTurnStep = 30.0f;
    if (!aiNPC_WR::ProcessINIT(init))
        return 0;
    attackPause = 0;
    trampleTimer = 0;
    centFlags = 0;
    maxHealth = 70.0f;
    health = 70.0f;
    fallbackAngle = 150.0f;
    runTimer = 0;
    neighTimer = 0;
    throwTimer = 0;
    aiFlags2 |= 0x20002;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;          // (a sncCHANNEL_SPH there)
    if (c) {
        c->minDist = 20.0f;
        c->maxDist = 100.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    if (!(hookObj = objFindName(inst->root, (char *)"plr_hook")))
        hookObj = objFindName(inst->root, (char *)"ATTACKfocus");
    throwObj = hookObj;
    minThrowDist = 50.0f;
    aimAngle = 20.0f;
    maxThrowDist = 150.0f;
    if (flags2 & 1) {
        centFlags |= 4;
        Activate(0);
    }
    if (!(bloodObj = objFindName(inst->root, (char *)"ROOT_2")))
        bloodObj = objFindName(inst->root, (char *)"CENTRE");
    if (animINST *cc = gsScenePtr->FindInstName((char *)"cent_colis_center")) {
        cc->GetPos(&colisCenter);
        centFlags |= 0x10;
    }
    float dustInit[4] = { 1.0f, 5.0f, 2.3f, 1.0f };
    dust = entCreate(gsScenePtr, (char *)"sfx_dust", (char *)"sfx_dust", 0, dustInit, 0);
    if (dust)
        dust->SetFollow(this, 0, 0);
    return 1;
}

// v12 0x0043B0A0  (feb 0x00434BC0)
int aiCENT::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (entFlags & 4)
        aiNPC_WR::ProcessCDT_FIREABLE(d);
    return 1;
}

// v12 0x0043B0C0  (feb 0x00434BE0)
// Blown apart: meat and blood from its middle (along the hit, for a weapon / explosion), the explosion's sound; gone.
void aiCENT::Explode(dmgDAMAGE *d, int dead)
{
    if (!bloodObj)
        return;
    m3dV *at = 0;
    if (d->type == 1 || d->type == 3 || d->type == 4)
        at = (m3dV *)((u8 *)d + 0xE);
    m3dV p;
    bloodObj->GetOrigin(&p);
    StartChunkMeat(3, dead, &p, at);
    StartBlood(7, dead, &p, 0, 0, at);
    gsSysSound->Play(&centSnd[CSND_EXPLOS], &p);
    msgSystem.PostMsg(this, 2, this);
}

// v12 0x0043B150  (feb 0x00434C70): its dust stays with it over a level reload
int aiCENT::ProcessASK_KEEP_LEVEL_RELOAD()
{
    if (dust)
        dust->flags2 |= 8;
    return aiNPC::ProcessASK_KEEP_LEVEL_RELOAD();
}

// v12 0x0043B170  (feb 0x00434C90): a rocket (weapon 6) or a weapon without type on the server blows it apart
void aiCENT::StartDeath(dmgDAMAGE *d)
{
    aiNPC_WR::StartDeath(d);
    if ((entFlags & 4) && d && d->type == 1) {
        int w = *(int *)((u8 *)d + 0x1A);
        if (w == 6 || w == 0)
            Explode(d, 0);
    }
}

// v12 0x0043B1B0  (feb 0x00434CE0)
// Turning: none while throwing, fast while neighing (270), slow while charging (40), else 110; neighing / throwing it
// stands (5 m/s, no turning to the goal). The speed eases (5 m/s per second) to 20, or 12 when the goal is 45 degrees
// or more off its heading in the open.
void aiCENT::SetMovementSpeed()
{
    int s = inst->seq;
    if (s == 12 || s == 11)
        turnSpeed = 0;
    else if (s == 10)
        turnSpeed = 270.0f;
    else if (s == 4)
        turnSpeed = 40.0f;
    else
        turnSpeed = 110.0f;
    if (s == 12 || s == 10) {
        aiFlags |= 0x200;
        moveSpeed = 5.0f;                               // v12 0x0062D084
    }
    float want = ((wrFlags & 0x10000) || goalAngle < 45.0f) ? 20.0f : 12.0f;   // v12 0x0062D088
    float up = moveSpeed + gsElapsedTime * 5.0f, dn = moveSpeed - gsElapsedTime * 5.0f;
    if (up < dn) {
        float t = up;
        up = dn;
        dn = t;
    }
    if (want < dn)
        moveSpeed = dn;
    else if (up < want)
        moveSpeed = up;
    else
        moveSpeed = want;
}

// v12 0x0043B300  (feb 0x00434E30)
// Appearing or neighing: straight on. Outside the open field, after 9 s it neighs. Charging: the hook out and the enemy
// at its height (1.6 lower, within 1.5) in the cone before the hook or swept by it since the last frame -> Throw.
// Running (RUN) with the charge pause over: the enemy within 6 m (flat) and 3 m of height -> it charges. Not hooking
// and the trample pause over: the enemy in the cone in front (1.5 m, 60 degrees) is struck aside.
void aiCENT::MoveToGoal()
{
    u32 f = centFlags;
    if (!(f & 4)) {
        if (!(f & 8) && (wrFlags & 0x10000)) {
            if (!(neighTimer <= 9.0f))
                centFlags = f | 0x20;
        } else
            neighTimer = 0;
    }
    if ((f & 4) || inst->seq == 10) {
        aiFlags |= 0x200;
        aiNPC_WR::MoveToGoal();
        return;
    }
    centFlags &= ~8u;
    if (centFlags & 1) {
        inst->Validate(4);
        m3dV h0;
        hookObj->GetOrigin(&h0);
        if (centFlags & 2) {
            m3dV h, e;
            GetHookPos(&h);
            enemy.GetPos(&e);
            if (fabsf(e.y + 1.6f - h.y) <= 1.5f) {
                h.y = 0;
                e.y = 0;
                m3dV d;
                inst->GetDir(&d);
                if (m3dIsBelongPointCone(&e, &h, &d, 1.5f, 0.5f, 45.0f) || m3dIsBelongPointCyl(&e, &h, &hookPrev, 0.5f))
                    Throw();
            }
        }
        hookPrev = h0;
    } else if (inst->seq == 3 && attackPause <= 1e-6f && !enemy.IsDead()) {
        m3dV p, e;
        inst->GetPos(&p);
        enemy.GetPos(&e);
        hookPrev = hookPrev2;
        float dx = p.x - e.x, dz = p.z - e.z;
        if (m3dSqrt(dx * dx + dz * dz) < 6.0f && fabsf(p.y - e.y) < 3.0f) {
            runTimer = 0;
            centFlags |= 1;
        }
    }
    if (!(centFlags & 2) && trampleTimer <= 1e-6f) {
        m3dV e, p, d;
        enemy.GetPos(&e);
        inst->GetPos(&p);
        inst->GetDir(&d);
        if (m3dIsBelongPointCone(&e, &p, &d, 1.5f, 0.5f, 60.0f)) {
            dmgGENERAL g;
            g.amount = 10.0f;
            inst->Validate(4);
            hookObj->GetOrigin(&g.pos);
            enemy.ProcessCDT_FIREABLE(&g);
            m3dV fw, v;
            inst->GetDir(&fw);
            m3dMakeVUnitConusRandom(&fw, 75.0f, &v);
            v.y = 0;
            m3dNormalize(&v);
            v.y = 1.5f;
            m3dNormalize(&v);
            v.x *= 10.0f;
            v.y *= 10.0f;
            v.z *= 10.0f;
            enemy.ApplySpeed(&v);
            trampleTimer = 1.2f;
            centFlags &= ~1u;
        }
    }
    aiNPC_WR::MoveToGoal();
}

// v12 0x0043B760  (feb 0x00435290)
void aiCENT::Think()
{
    aiNPC_WR::Think();
    UpdateInAreaFlags();
    if (CheckShootCond())
        centFlags |= 0x1000;
    else
        centFlags &= ~0x1000u;
    if (inst->seq == 10)
        neighTimer = 0;
}

// v12 0x0043B7B0  (feb 0x004352E0)
// DEATH; APPEAR; outside the open field and not alerted: NEIGH; THROW; ATTACK; NEIGH when it wants to, else RUN.
int aiCENT::EvalAnimGoalSeq()
{
    if (aiFlags & 0x200000)
        return 6;
    u32 f = centFlags;
    if (f & 4)
        return 1;
    if (!(wrFlags & 0x10000) && !(aiFlags & 2))
        return 10;
    if (f & 0x1000)
        return 12;
    if (f & 1)
        return 4;
    return (f & 0x20) ? 10 : 3;
}

// v12 0x0043B820  (feb 0x00435350)
// APPEAR's end; NEIGH's end; ATTACK's hook out (0) and back (1: a 5 s pause); THROW's frame: the trident, 4..8 s more.
void aiCENT::ProcessActionFrame()
{
    if (inst->IsActionFrame(1, 0))
        centFlags &= ~4u;
    if (inst->IsActionFrame(10, 0))
        centFlags &= ~0x20u;
    if (inst->IsActionFrame(4, 0))
        centFlags |= 2;
    if (inst->IsActionFrame(4, 1)) {
        attackPause = 5.0f;
        centFlags &= ~3u;
    }
    if (inst->IsActionFrame(12, 0)) {
        LaunchOneProjectile();
        throwTimer = m3dRandRange(4.0f, 8.0f);
        centFlags &= ~0x1000u;
    }
}

// v12 0x00451B50 (shared)
void aiCENT::GetWpnTip(m3dV *out)
{
    inst->GetPos(out);
    out->y += 1.0f;
}

// v12 0x0043B900 (shared)
void aiCENT::GetPlayerTargetPos(m3dV *out)
{
    enemy.GetPos(out);
    out->y += 0.01f;
}

// v12 0x0043B930  (feb 0x00435460; 1.2: the throw's pause runs 5 times as fast outside the open field)
void aiCENT::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&attackPause, gsElapsedTime);
    m3dUpdateTimeField(&runTimer, gsElapsedTime);
    m3dUpdateTimeField(&trampleTimer, gsElapsedTime);
    m3dUpdateTimeField(&throwTimer, (wrFlags & 0x20000) ? gsElapsedTime : gsElapsedTime * 5.0f);
    if (!(centFlags & 0x20))
        neighTimer += gsElapsedTime;
}

// v12 0x0043B9D0  (feb 0x00435500)
// NEIGH's second frame: a neigh (2); ATTACK's first: a hit sound (0x10); a sequence's start: APPEAR (1), running (8),
// DEATH (4), THROW (0x20).
void aiCENT::FillSoundFlags()
{
    if (inst->IsActionFrame(10, 1))
        unk108 |= 2;
    if (inst->IsActionFrame(4, 0))
        unk108 |= 0x10;
    if (!(inst->flags & 0x4000))
        return;
    switch (inst->seq) {
    case 1:
    case 2:
        unk108 |= 1;
        break;
    case 3:
    case 4:
    case 5:
    case 7:
    case 8:
    case 9:
    case 11:
    case 13:
        unk108 |= 8;
        break;
    case 6:
        unk108 |= 4;
        break;
    case 12:
        unk108 |= 0x20;
        break;
    }
}

// v12 0x0043BAB0  (feb 0x004355E0)
void aiCENT::PlaySounds()
{
    u32 f = unk108;
    if (f & 1)
        voice->Play((sncSOUND_DESCR *)&centSnd[CSND_APPEAR], 0);
    else if (f & 4)
        voice->Play((sncSOUND_DESCR *)&centSnd[CSND_DEATH], 0);
    else if (f & 2)
        voice->Play((sncSOUND_DESCR *)&centSnd[m3dRandSelect(1, 2, 3)], 0);
    else if (f & 0x20)
        voice->Play((sncSOUND_DESCR *)&centSnd[CSND_TRIDENT_THROW], 0);
    else if (f & 8)
        voice->Play((sncSOUND_DESCR *)&centSnd[CSND_GALLOP], 0);
    if (unk108 & 0x10) {
        m3dV p;
        inst->GetPos(&p);
        gsSysSound->Play(&centSnd[m3dRandSelect(1, 2, 3)], &p);
    }
    aiNPC::PlaySounds();
}

// v12 0x0043BBA0  (feb 0x004356D0)
void aiCENT::SelectGoalToRunAway(float dist)
{
    aiNPC_WR::SelectGoalToRunAway(dist);
    runTimer = 1.0f;
    centFlags &= ~8u;
}

// v12 0x0043BBD0  (feb 0x00435700)
// Unless escaping its area: a run -- away when the enemy is within 3 m, straight at it when it is within 70 degrees
// of its heading, else round. The goal at its own height, kept within its bans.
int aiCENT::SelectGoal()
{
    if (!SelectEscapeWhenOut()) {
        m3dV t, p;
        GetPlayerTargetPos(&t);
        inst->GetPos(&p);
        StartRunStraight(enemyDist < 3.0f ? 0 : enemyAngle <= 70.0f ? 3 : 2);
    }
    m3dV p;
    inst->GetPos(&p);
    goalPos.y = p.y;
    RestrictGoalByBanCross();
    return 1;
}

// v12 0x0043BC70  (feb 0x004357A0)
// While a run lasts (and it is neither hurt nor outside the open field) nothing. Else: 0 / 1 a flee point (3 s), 2 a
// flee point when the enemy is within 20 m (4 s), else, and 3, straight at it -- when the charge pause is over and it
// may attack in the open (8: aiNPC's goal); otherwise, or when told to keep away, a near flee point (6 s).
void aiCENT::StartRunStraight(int how)
{
    if (runTimer > 1e-6f && !(aiFlags & 1) && (wrFlags & 0x10000))
        return;
    m3dV p, d;
    inst->GetPos(&p);
    inst->GetDir(&d);
    int straight = 0;
    switch (how) {
    case 0:
    case 1:
        runTimer = 3.0f;
        aiAttackPlanner->GetFleeGoalPoint(this, m3dRandRange(1.0f, 1.5f), &goalPos);
        break;
    case 2:
        if (enemyDist < 20.0f) {
            runTimer = 4.0f;
            aiAttackPlanner->GetFleeGoalPoint(this, m3dRandRange(1.0f, 1.5f), &goalPos);
            break;
        }
        // (fall through)
    case 3:
        straight = 1;
        runTimer = 0;
        break;
    }
    if (runTimer < 1e-6f && (aiFlags & 4)) {
        runTimer = 6.0f;
        aiAttackPlanner->GetFleeGoalPoint(this, m3dRandRange(0.2f, 0.5f), &goalPos);
    }
    if (runTimer < 1e-6f) {
        if (straight && attackPause <= 1e-6f && (aiFlags & 0x10000000) && (wrFlags & 0x30000)) {
            centFlags |= 8;
            aiNPC::SelectGoal();
        } else {
            runTimer = 6.0f;
            aiAttackPlanner->GetFleeGoalPoint(this, m3dRandRange(0.2f, 0.5f), &goalPos);
        }
    }
    wayPointPos = goalPos;
}

// v12 0x0043BE70  (feb 0x004359A0)
// The hook catches the enemy: 18 damage, flung back over its head (1.7 up) at 28; 1.5 s before it strikes again.
void aiCENT::Throw()
{
    dmgGENERAL g;
    g.amount = 18.0f;
    inst->Validate(4);
    hookObj->GetOrigin(&g.pos);
    enemy.ProcessCDT_FIREABLE(&g);
    m3dV v;
    inst->GetDir(&v);
    v.x *= -1.0f;
    v.y = 1.7f;
    v.z *= -1.0f;
    m3dNormalize(&v);
    v.x *= 28.0f;
    v.y *= 28.0f;
    v.z *= 28.0f;
    enemy.ApplySpeed(&v);
    trampleTimer = 1.5f;
    centFlags &= ~3u;
}

// v12 0x0043BF60  (feb 0x00435A90)
void aiCENT::GetHookPos(m3dV *out)
{
    inst->Validate(4);
    hookObj->GetOrigin(out);
}

// v12 0x0043BF90  (feb 0x00435AC0) exact: a trampoline throws it half as far
void aiCENT::ApplySpeedTrampoline(m3dV *v)
{
    m3dV h;
    h.x = v->x * 0.5f;
    h.y = v->y * 0.5f;
    h.z = v->z * 0.5f;
    aiNPC::ApplySpeedTrampoline(&h);
}

// v12 0x0043BFD0  (feb 0x00435B00): knocked back at a gallop, a quarter less
void aiCENT::AdjustSpeedsOnFallbackStart(dmgWEAPON *, float *back, float *)
{
    int s = inst->seq;
    if (s == 3 || s == 4 || s == 7 || s == 9)
        *back *= 0.75f;
}

// v12 0x0043C010  (feb 0x00435B80)
// At 25 m/s: towards where the enemy will be (the planner's meet point 25 m ahead, 1.5 m up) within the aim angle of
// its facing, else along its facing; up enough to arc there under a fall of 10.
void aiCENT::GetShootPjlPhysParam(m3dV *org, physINIT *p)
{
    m3dV fw, d, m;
    inst->GetDir(&fw);
    fw.y = 0;
    if (!(wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(this, 25.0f, &m)) {
        m.y += 1.5f;
        d.x = m.x - org->x;
        d.y = 0;
        d.z = m.z - org->z;
        m3dNormalize(&d);
        if (!(fabsf(m3xzAngleVector(&d, &fw)) <= aimAngle))
            d = fw;
    } else {
        enemy.GetPos(&m);
        d = fw;
    }
    d.x *= 25.0f;
    d.y *= 25.0f;
    d.z *= 25.0f;
    float dx = org->x - m.x, dz = org->z - m.z;
    float t = m3dSqrt(dx * dx + dz * dz) * 0.04f;
    p->vel[0] = d.x;
    p->vel[1] = ((m.y - org->y) - t * t * -5.0f) / t;
    p->vel[2] = d.z;
    p->accel[0] = m3dVUnitY.x * -10.0f;
    p->accel[1] = m3dVUnitY.y * -10.0f;
    p->accel[2] = m3dVUnitY.z * -10.0f;
}

// v12 0x0043C250  (feb 0x00435DC0)
// A trident from the hook (its matrix): 15 damage (1.5 m), a 0.3 sphere, 7 s at most, two hits; lit like its floor.
void aiCENT::LaunchOneProjectile()
{
    pjlPJL_INIT in;
    memset(&in, 0, sizeof(in));
    in.phys.packId = 0;
    in.phys.tag = 0x40;
    in.phys.size = 0x55;
    animCREATE_DATA cd;
    objOBJ *o = throwObj;
    inst->Validate(4);
    m3dV org;
    o->GetOrigin(&org);
    in.ownerId = (u32)mpId;
    memcpy(&cd.matr, (u8 *)o + 0x3C, sizeof(cd.matr));
    in.phys.maxHits = 2;
    in.phys.cdtMode = 2;
    in.phys.radius = 0.3f;
    in.phys.lifetime = 7.0f;
    in.phys.friction = 0;
    in.damage = 15.0f;
    in.radius = 1.5f;
    in.weaponType = -1;
    in.isGeneral = 4;                                   // (feb 3)
    in.termDelay = -1.0f;
    in.explosive = 0;
    GetShootPjlPhysParam(&org, &in.phys);
    struct { pjlPJL_INIT i; u32 extra; } blk;           // (1.2 sends 0x55 bytes: a zero dword after the init)
    blk.i = in;
    blk.extra = 0;
    entENTITY *t = entCreate(gsScenePtr, (char *)"pjl_trident", (char *)"pjl_trident", &cd, &blk, 0);
    if (t)
        gsEffApplyLMChar(t, &floor, 0);
}

// v12 0x0043C3C0  (feb 0x00435F30)
// Alerted, not stunned or hurt, may see it, the pause over, within the throw range (beyond 10 m; 50 m in the open)
// and within the aim angle.
int aiCENT::CheckShootCond()
{
    if (!(aiFlags & 0x10000000) || (aiFlags & 0x80000008) || (wrFlags & 0x200) || !(throwTimer <= 1e-6f))
        return 0;
    if (maxThrowDist < enemyDist)
        return 0;
    if (wrFlags & 0x20000) {
        if (enemyDist < minThrowDist)
            return 0;
    } else if (enemyDist < 10.0f)
        return 0;
    return enemyAngle <= aimAngle;
}

// v12 0x0043C480  (feb 0x00435FF0)
int aiCENT::IsAnimSeqGoodForFallback()
{
    return inst->seq != 6;
}

// v12 0x0045CD90 / 0x0045A950 (shared)
int aiCENT::GetLayer3State()
{
    return (int)centFlags;
}
void aiCENT::SetLayer3State(int s)
{
    centFlags = (u32)s;
}

// ---------------------------------------------------------------------------------------------- the skeleton centaur

// v12 0x0045C830  (feb 0x00453D10)
int __fastcall aiSketavrInit()
{
    if (!entRegisterClass((char *)"ai_sketavr", 0x244E5043, 0x32, aiSKETAVR::Create, (char *)"ai_sketavr", 0))
        return 0;
    return sncLinkSndListData(4, sketSnd) != 0;
}

// v12 0x0045C870  (feb 0x00453D50)
entENTITY *__fastcall aiSKETAVR::Create(animINST *)
{
    aiSKETAVR *c = new aiSKETAVR;
    if (!c)
        return 0;
    c->wrFlags = 0;
    c->bloodObj = 0;
    c->hookObj = 0;
    c->voice = 0;
    c->unk98 = 6;
    return c;
}

// v12 0x0045C8E0  (feb 0x00453DA0): 30 hp, a skeleton (0x800), bloodless (wrFlags 2), knocked back within 80 degrees
int aiSKETAVR::ProcessINIT(void *init)
{
    if (!aiCENT::ProcessINIT(init))
        return 0;
    maxHealth = 30.0f;
    health = 30.0f;
    centFlags |= 0x800;
    wrFlags |= 2;
    fallbackAngle = 80.0f;
    return 1;
}

// v12 0x0045C8C0
void aiSKETAVR::StartDeath(dmgDAMAGE *d)
{
    StartDeath(d, 0, 1);
}

// v12 0x0045C940  (feb 0x00453E00)
// Unless the damage asks otherwise (subtype 2): its voice muted, the death sound, the bones' cloud ("sfx_skel_death",
// 0.07); shattered: the pieces flung along the hit (3..7 by the damage, more near it) or along the face it hit / its
// heading (4), and it is gone.
void aiSKETAVR::StartDeath(dmgDAMAGE *d, cdtINFO *ci, int shatter)
{
    if (!d || d->subtype != 2) {
        voice->flags |= 0x400;
        unk108 |= 4;
        PlaySounds();
        float size = 0.07f;
        aiSKEL_RIGID_START r;
        r.sfx = (wrsfxSKELETON_DEATH *)entCreate(gsScenePtr, (char *)"sfx_skel_death", (char *)"", 0, &size, 0);
        if (!r.sfx)
            return;
        if (shatter) {
            m3dV v;
            float k;
            if (d) {
                m3dV pr, tmp;
                float dist;
                GetPosRooted(&pr);
                dmgGetOrgDir(d, &pr, &tmp, &v, &dist);
                float a = d->amount, sp;
                if (a < 5.0f)
                    sp = 3.0f;
                else if (a > 20.0f)
                    sp = 7.0f;
                else
                    sp = (a - 5.0f) * 0.26666668f + 3.0f;
                if (dist < 0.0f)
                    k = 2.0f * sp;
                else if (dist <= 15.0f)
                    k = (2.0f - dist * 0.06666667f) * sp;
                else
                    k = 1.0f * sp;
            } else {
                if (ci && (!ci->inst || (ci->inst->flags & 0x48)))
                    gsScenePtr->GetCDTFace(ci, 0, &v);
                else
                    inst->GetDir(&v);
                k = 4.0f;
            }
            v.y = 0;
            if (!_m3dNormalize(&v)) {
                v = m3dVUnitY;
                k = 1.0f;
            }
            r.impulse.x = v.x * k;
            r.impulse.y = v.y * k;
            r.impulse.z = v.z * k;
            r.StartExplos(this);
            bloodObj = 0;
            rootObj = 0;
            inst->root = 0;
            msgSystem.PostMsg(this, 2, 0);
        }
    }
    aiNPC_WR::StartDeath(d);
}

// v12 0x0045CBA0  (feb 0x00454050, rewritten in 1.2): one of its own four sounds
void aiSKETAVR::PlaySounds()
{
    u32 f = unk108;
    if (f & 4)
        voice->Play((sncSOUND_DESCR *)&sketSnd[1], 0);
    else if (f & 1)
        voice->Play((sncSOUND_DESCR *)&sketSnd[0], 0);
    else if (f & 0x20)
        voice->Play((sncSOUND_DESCR *)&sketSnd[3], 0);
    else if (f & 8)
        voice->Play((sncSOUND_DESCR *)&sketSnd[2], 0);
    aiNPC::PlaySounds();
}

static_assert(sizeof(aiCENT) == 0x5BB, "aiCENT");
static_assert(sizeof(aiSKETAVR) == 0x5BB, "aiSKETAVR");
static_assert(sizeof(pjlTRIDENT) == 0x208, "pjlTRIDENT");
