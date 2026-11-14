// ai_amur.cpp -- aiAMUR_GENERIC, the amur (cupid, "ai_amur"), and its arrow "pjl_amur_arrow" (Will Rock 1.2, v12.exe;
// feb objs ai_amur.obj, ai_amur_arrow_pjl.obj)
//
// A flyer (aiNPC_WR_FLY) that keeps 6..12 m over its enemy. It wanders (a new point, 5 s) and every so often attacks:
// it flies over the enemy (shotsLeft arrows: 3, or 8 with 40 % luck when idle) and shoots while the enemy is 5..50 m
// away and within 20 degrees of its facing; then it rests 2 s. Out of range it hurries (15 m/s for 4 s). Hit, it
// flinches (WOUNDED); shot down it falls (START_DEATH), bleeds and lies (DEATH_LAY). 7 hp.
// The arrow is the satyr's (pjlARROW_GENERIC, include/ai/pjl_arrow.h) with the amur's two hit sounds.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../../include/ai/pjl_arrow.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list);  // v12 0x004B1E30
void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int skip);  // v12 0x005130D0

// v12 0x00649230.. 11 sounds, 0x68 apart (static initialisers v12 0x00436DC0..0x004370FF); the distances are the
// base ones scaled (the nearer by 1.2, the farther by 1.1; the arrow's 1.3 / 1.5)
static sncSOUND_DESCR_3D amurSnd[11];
enum { ASND_HIT_1, ASND_HIT_2, ASND_HIT_3, ASND_DEATH_1, ASND_DEATH_2, ASND_DEATH_3, ASND_FLY, ASND_SHOT, ASND_FALL_GROUND,
       ASND_ARROW_HIT_GROUND, ASND_ARROW_HIT_BODY };

static struct aiAMUR_SND_INIT {
    aiAMUR_SND_INIT()
    {
        static const struct { const char *name; int flags, delay; float mn, mx; } s[11] = {
            { "amour_being_hit_1", 0x40000, 0, 25.5f * 1.2f, 70.0f * 1.1f },
            { "amour_being_hit_2", 0x40000, 0, 25.5f * 1.2f, 70.0f * 1.1f },
            { "amour_being_hit_3", 0x40000, 0, 25.5f * 1.2f, 70.0f * 1.1f },
            { "amour_death_1", 0, 20000, 25.5f * 1.2f, 70.0f * 1.1f },
            { "amour_death_2", 0, 20000, 25.5f * 1.2f, 70.0f * 1.1f },
            { "amour_death_3", 0, 20000, 25.5f * 1.2f, 70.0f * 1.1f },
            { "amour_fly", 0x10002, 0, 5.0f * 1.2f, 15.0f * 1.1f },
            { "amour_shot", 0, 0, 15.0f * 1.2f, 50.0f * 1.1f },
            { "amour_fall_ground", 0, 0, 8.0f * 1.2f, 25.0f * 1.1f },
            { "amour_arrow_hit_ground", 0x60000, 0, 3.0f * 1.3f, 18.0f * 1.5f },
            { "amour_arrow_hit_body", 0x60000, 0, 3.0f * 1.3f, 18.0f * 1.5f },
        };
        for (int i = 0; i < 11; i++) {
            sncSOUND_DESCR_3D *d = &amurSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = 0.0f;
            d->delay = s[i].delay;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].mn;
            d->maxDist = s[i].mx;
            d->vtbl = 0;
        }
    }
} aiAmurSndInit;

// ---------------------------------------------------------------------------------------------- the arrow

// pjlAMUR_ARROW: the satyr's arrow (its code, its size 0x208) with the amur's sounds. vtable v12 0x005FCB90
#pragma pack(push, 1)
class pjlAMUR_ARROW : public pjlARROW_GENERIC {
public:
    static entENTITY *__fastcall Create(animINST *i);
    sncSOUND_DESCR_3D *GetSound(int i);
};
#pragma pack(pop)

// v12 0x00438710  (feb 0x004324E0)
entENTITY *__fastcall pjlAMUR_ARROW::Create(animINST *)
{
    return new pjlAMUR_ARROW;
}

// v12 0x00437150  (feb 0x00430FA0 GetArrowSound): no flying sound; 1 into the level, 2 into a body
sncSOUND_DESCR_3D *pjlAMUR_ARROW::GetSound(int i)
{
    if (i == 1)
        return &amurSnd[ASND_ARROW_HIT_GROUND];
    if (i == 2)
        return &amurSnd[ASND_ARROW_HIT_BODY];
    return 0;
}

// v12 0x00438300  (feb 0x004320D0)
int __fastcall pjlAmurArrowInit()
{
    return entRegisterClass((char *)"pjl_amur_arrow", 0x41525257, 0x32, pjlAMUR_ARROW::Create, 0, 1) != 0;
}

// ---------------------------------------------------------------------------------------------- the amur

// v12 0x00437100  (feb 0x00430F50)
int __fastcall aiAmurInit()
{
    if (!entRegisterClass((char *)"ai_amur", 0x244E5043, 0x32, aiAMUR_GENERIC::Create, (char *)"ai_amur", 0))
        return 0;
    if (!pjlAmurArrowInit())
        return 0;
    return sncLinkSndListData(11, amurSnd) != 0;
}

// v12 0x00437180  (feb 0x00430FC0)
entENTITY *__fastcall aiAMUR_GENERIC::Create(animINST *)
{
    aiAMUR_GENERIC *a = new aiAMUR_GENERIC;             // operator new(0x5A7), aiNPC ctor
    if (!a)
        return 0;
    a->wrFlags = 0;
    a->bloodObj = 0;
    a->unk98 = 10;
    return a;
}

// v12 0x004371D0  (feb 0x00431010)
// IDLE_FLY, the turn to shoot, SHOOT (again and again), the turn back; WOUNDED; START_DEATH (the fall) then DEATH_LAY.
BOOL32 aiAMUR_GENERIC::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 7;
    u32 *blk = (u32 *)operator new(0x3BC);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 7;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 7; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    _entAnimSetSeqName(t, 0, (char *)"AMUR_SEQ_IDLE_FLY");
    _entAnimSetSeqName(t, 1, (char *)"AMUR_SEQ_TRAN_FLY2SHOOT");
    _entAnimSetSeqName(t, 2, (char *)"AMUR_SEQ_SHOOT");
    _entAnimSetSeqName(t, 3, (char *)"AMUR_SEQ_TRAN_SHOOT2FLY");
    _entAnimSetSeqName(t, 4, (char *)"AMUR_SEQ_WOUNDED");
    _entAnimSetSeqName(t, 5, (char *)"AMUR_SEQ_START_DEATH");
    _entAnimSetSeqName(t, 6, (char *)"AMUR_SEQ_DEATH_LAY");
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(7, 0);
    if (!s) {
        if (t->seq)
            operator delete((u32 *)t->seq - 1);
        return 0;
    }
    t->aniSystem = s;
    s->SetTranParamFromAny(4, 0, 1, 4);
    s->SetTranParamFromAny(5, 0, 1, 5);
    s->SetTranCycleAnim(4);
    s->SetTranParam(5, 6, 0, 6, 1);
    s->SetTranParam(5, 5, 2, 5, 1);
    s->SetTranParam(6, 6, 2, 6, 1);
    s->SetTranParam(0, 1, 1, 1, 0);
    s->SetTranParam(1, 2, 1, 2, 0);
    s->SetTranParam(2, 3, 1, 3, 0);
    s->SetTranParam(3, 0, 1, 0, 0);
    s->SetTranParam(0, 2, 1, 1, 0);
    s->SetTranParam(2, 0, 1, 3, 0);
    s->SetTranParam(2, 2, 1, 0, 1);
    s->SetTranParam(1, 2, 1, 3, 0);
    if (old)
        operator delete((u32 *)old - 1);
    return 1;
}

// v12 0x00437420  (feb 0x00431270)
// 7 hp; 6 m/s (15 hurrying), 60 deg/s; shoots 5..50 m within 20 degrees; hovers 6..12 m over the enemy (its arrival
// distance the slant to that height over 10 m); a sphere voice (30..150); "attack" (the arrows), "ROOT" (bleeding).
int aiAMUR_GENERIC::ProcessINIT(void *init)
{
    aiNPC_WR_FLY::ProcessINIT(init);
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    maxHealth = 7.0f;
    alertAngle = 180.0f;
    moveSpeed = 6.0f;
    turnSpeed = 60.0f;
    minShootDist = 5.0f;
    shootRange = 50.0f;
    aimAngle = 20.0f;
    corpseTime = 7.0f;
    corpseFadeTime = 6.0f;
    maxTurnStep = 50.0f;
    timeDeactivate = 120.0f;
    radCDT = 1.0f;                                      // (feb 3)
    heightLowTester = 3.0f;
    heightCDT = 0;
    fallbackAngle = 15.0f;
    health = 7.0f;
    amurFlags = 0;
    shotPause = 0;
    wanderTimer = 0;
    shotsLeft = 0;
    woundEnds = 1;
    woundSeq = 4;
    attackTimer = 0;
    fastTimer = 0;
    restTimer = 2.0f;
    hover = (m3dRandom() + 1.0f) * 6.0f;
    navArriveDist = m3dSqrt(hover * hover + 100.0f);
    heightUp1 = 0.3f;                                   // (1.2)
    fallDrag = -12.0f;
    aiFlags2 |= 0x8000;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;          // (a sncCHANNEL_SPH there)
    if (c) {
        c->minDist = 30.0f;
        c->maxDist = 150.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    if (!(attackObj = objFindName(inst->root, (char *)"attack")))
        return 0;
    if (!(rootObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    if (aiFlags & 0x400000)                             // spawned: the player at once
        amurFlags |= 0xC00;
    return 1;
}

// v12 0x00437790  (feb 0x004315C0)
// Spawned: it turns to the player first (an attack of 5). The enemy out of range: it hurries (4 s), the attack over.
void aiAMUR_GENERIC::Think()
{
    if ((flags2 & 1) && (amurFlags & 0x800)) {
        amurFlags &= ~0x800u;
        OrientTowardsPlayer();
        shotsLeft = 5;
    }
    if (!(enemyDist <= shootRange)) {
        fastTimer = 4.0f;
        amurFlags = (amurFlags & ~0x200u) | 0x400;
    }
    aiNPC_WR::Think();
}

// v12 0x00437800  (feb 0x00431660)
// Shot down: START_DEATH, once fallen DEATH_LAY. Alive: just hit -> the wound (held till its end), shooting -> SHOOT.
int aiAMUR_GENERIC::EvalAnimGoalSeq()
{
    if (aiFlags & 0x200000)
        return (amurFlags & 0x80) ? 6 : 5;
    if (enemy.IsDead() || (aiFlags & 0x80010000))
        return 0;
    if (amurFlags & 0x30) {
        u32 f = aiFlags;
        amurFlags = (amurFlags & ~0x30u) | 8;
        woundSeq = (int)(((f & 0x200000) | 0x800000) >> 21);
        woundEnds = (int)((~f >> 21) & 1);
        return woundSeq;
    }
    if (amurFlags & 8)
        return woundSeq;
    if (aiFlags & 0x1001)
        return 2;
    return 0;
}

// v12 0x004378A0  (feb 0x00431700)
// The flyer's death, falling at 7.64 to start with; the fall's drag (physINIT +0x14) held till START_DEATH's frame.
void aiAMUR_GENERIC::StartDeath(dmgDAMAGE *d)
{
    u8 *pi = (u8 *)passivePhys + 0x5C;
    aiNPC_WR_FLY::StartDeath(d);
    passivePhys->vel[1] = -7.64f;
    fallDrag = *(float *)(pi + 0x14);
    *(float *)(pi + 0x14) = 0;
}

// v12 0x00437900  (feb 0x00431760)
// Falling, the first touch: it bleeds (along its fall) and stops.
void aiAMUR_GENERIC::ProcessCDT(cdtINFO *)
{
    if (!(aiFlags & 0x200000) || (amurFlags & 0x80))
        return;
    amurFlags = (amurFlags & ~8u) | 0x80;
    m3dV p, v;
    inst->GetPos(&p);
    v = *(m3dV *)passivePhys->vel;
    StartBlood(4, 1, &p, 0, &v, &p);
    *(m3dV *)passivePhys->vel = m3dVZero;
}

// v12 0x004379C0  (feb 0x00431820)
// SHOOT's / the turn's second action frame: an arrow. The wound's end (alive): free. START_DEATH's first: the fall
// at 11.5 with its drag back.
void aiAMUR_GENERIC::ProcessActionFrame()
{
    inst->IsActionFrame(0, 0);
    if (!inst->IsActionFrame(2, 0))
        inst->IsActionFrame(1, 0);
    if (inst->IsActionFrame(2, 1) || inst->IsActionFrame(1, 1))
        LaunchOneProjectile();
    inst->IsActionFrame(4, 0);
    if (inst->IsActionFrame(4, 1) && woundEnds)
        amurFlags &= ~8u;
    if (inst->IsActionFrame(5, 0)) {
        passivePhys->vel[1] = -11.5f;
        *(float *)((u8 *)passivePhys + 0x70) = fallDrag;
    }
    if (inst->IsActionFrame(5, 1) && woundEnds)
        amurFlags &= ~8u;
}

// v12 0x00437AE0  (feb 0x00431940)
// Attacking (0x200): over the enemy, shooting while it may, till the attack's time or arrows run out (then a rest).
// Else (asked to attack, 0x40): after a rest a new point to wander to (5 s); hurt or the wander over, an attack begins
// (5..10 s; when it may not shoot now, 40 %: 8 arrows over 20..25 s); 1.2: arrived (0.2) and facing it, it shoots.
// Always: the goal 6..12 m over the enemy, kept out of water.
int aiAMUR_GENERIC::SelectGoal()
{
    m3dV e;
    enemy.GetPos(&e);
    int unset = 0;                                      // (both builds read an uninitialised local here; taken as 0)
    auto mayShoot = [&]() -> bool {
        return (aiFlags & 0x10000000) && !(aiFlags & 8) && !(wrFlags & 0x200) && wanderTimer <= 1e-6f &&
               !(enemyDist < minShootDist) && !(shootRange < enemyDist) && enemyAngle <= aimAngle;
    };
    if (amurFlags & 0x200) {
        if (attackTimer < 1e-6f || shotsLeft < 1) {
            amurFlags = (amurFlags & ~0x200u) | 0x400;
            shotsLeft = 0;
        }
        if (mayShoot())
            aiFlags |= 0x1000;
        else
            aiFlags &= ~0x1000u;
        aiFlags |= 0x40000;
        aiNPC::SelectGoal();
        aiFlags &= ~0x40000u;
    } else if ((aiFlags & 0x40000) || !(amurFlags & 0x40)) {
        if (mayShoot()) {
            shotsLeft = 1;
            aiFlags |= 0x1000;
        } else
            aiFlags &= ~0x1000u;
        aiNPC::SelectGoal();
        m3dV p;                                         // 1.2: at the goal and facing the enemy: shoot
        inst->GetPos(&p);
        float dx = p.x - goalPos.x, dz = p.z - goalPos.z;
        if (m3dSqrt(dx * dx + dz * dz) < 0.2f && enemyAngle < aimAngle)
            aiFlags |= 0x1000;
    } else {
        bool attack = false, wander = false;
        if (amurFlags & 0x400) {                        // the attack over: rest, then wander
            amurFlags &= ~0x400u;
            restTimer = 2.0f;
            wander = true;
        } else if ((aiFlags & 1) || wanderTimer < 1e-6f) {
            if (mayShoot()) {
                attackTimer = (m3dRandom() + 1.0f) * 5.0f;
            } else if (m3dRandom() < 0.4f) {
                shotsLeft = 8;
                attackTimer = m3dRandom() * 5.0f + 20.0f;
            }
            attack = true;
        } else if (restTimer >= 1e-6f) {
            wander = unset != 0;
        } else if (mayShoot()) {
            attackTimer = (m3dRandom() + 1.0f) * 5.0f;
            attack = true;
        }
        if (attack) {
            if (shotsLeft == 0)
                shotsLeft = 3;
            wayPointPos = e;
            amurFlags |= 0x200;
        } else if (wander) {
            aiFlags &= ~0x1000u;
            shotsLeft = 0;
            m3dV p;
            inst->GetPos(&p);
            aiNPC::SelectGoal();
            wanderTimer = 5.0f;
            wayPointPos = goalPos;
        }
    }
    goalPos.y = e.y + hover;
    CorrectGoalByWaterLevel();
    return 1;
}

// v12 0x00437FD0  (feb 0x00431D40): the flyer's frame (aiNPC_WR's), then the effect slot
void aiAMUR_GENERIC::ProcessFRAME()
{
    aiNPC_WR::ProcessFRAME();
    Slot33();
}

// v12 0x00437FF0  (feb 0x00431D60): 15 m/s hurrying, else 6; wounded or falling: no turning to the goal
void aiAMUR_GENERIC::SetMovementSpeed()
{
    moveSpeed = fastTimer > 1e-6f ? 15.0f : 6.0f;
    int s = inst->seq;
    if (s == 4 || s == 5 || s == 6)
        aiFlags |= 0x200;
}

// v12 0x00438050  (feb 0x00431DC0)
void aiAMUR_GENERIC::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&shotPause, gsElapsedTime);
    m3dUpdateTimeField(&wanderTimer, gsElapsedTime);
    m3dUpdateTimeField(&attackTimer, gsElapsedTime);
    m3dUpdateTimeField(&restTimer, gsElapsedTime);
    m3dUpdateTimeField(&fastTimer, gsElapsedTime);
}

// v12 0x004380C0  (feb 0x00431E30): on the server: the hurt (a wound to show) and one of the three hit / death cries
int aiAMUR_GENERIC::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (!(entFlags & 4))
        return 1;
    aiNPC_WR::ProcessCDT_FIREABLE(d);
    if (aiFlags & 0x200000) {
        amurFlags |= 0x20;
        unk108 |= m3dRandSelect(0x100, 0x200, 0x400);
    } else {
        amurFlags |= 0x10;
        unk108 |= m3dRandSelect(0x10, 0x20, 0x40);
    }
    return 1;
}

// v12 0x00438130  (feb 0x00431EA0)
void aiAMUR_GENERIC::PlaySounds()
{
    if (unk108 & 2)
        voice->Play((sncSOUND_DESCR *)&amurSnd[ASND_SHOT], 0);
    if (unk108 & 4)
        voice->Play((sncSOUND_DESCR *)&amurSnd[ASND_FLY], 0);
    if (unk108 & 0x10)
        voice->Play((sncSOUND_DESCR *)&amurSnd[ASND_HIT_1], 0);
    if (unk108 & 0x20)
        voice->Play((sncSOUND_DESCR *)&amurSnd[ASND_HIT_2], 0);
    if (unk108 & 0x40)
        voice->Play((sncSOUND_DESCR *)&amurSnd[ASND_HIT_3], 0);
    if (unk108 & 0x100)
        voice->Play((sncSOUND_DESCR *)&amurSnd[ASND_DEATH_1], 0);
    if (unk108 & 0x200)
        voice->Play((sncSOUND_DESCR *)&amurSnd[ASND_DEATH_2], 0);
    if (unk108 & 0x400)
        voice->Play((sncSOUND_DESCR *)&amurSnd[ASND_DEATH_3], 0);
    if (unk108 & 8) {
        m3dV p;
        inst->GetPos(&p);
        gsSysSound->Play(&amurSnd[ASND_FALL_GROUND], &p);
    }
    aiNPC::PlaySounds();
}

// v12 0x00438250  (feb 0x00431FC0): on a sequence's start: flying, shooting, a cry when wounded / falling, the landing
void aiAMUR_GENERIC::FillSoundFlags()
{
    if (!(inst->flags & 0x4000))
        return;
    switch (inst->seq) {
    case 0:
    case 3:
        unk108 = 4;
        break;
    case 1:
    case 2:
        unk108 = 2;
        break;
    case 4:
        unk108 = m3dRandSelect(0x10, 0x20, 0x40);
        break;
    case 5:
        unk108 = m3dRandSelect(0x100, 0x200, 0x400);
        break;
    case 6:
        unk108 = 8;
        break;
    }
}

// v12 0x004382F0  (feb 0x004320C0): the state told (a remote copy), its attack over
void aiAMUR_GENERIC::SetLayer3State(int s)
{
    amurFlags = (u32)s | 0x400;
}

// v12 0x00438330  (feb 0x00432100)
// At 40 m/s: towards where the enemy will be (the planner's meet point 40 m ahead, 1 m up) if that is within the aim
// angle of its facing, else along its facing; up enough to arc there under a fall of 10 (the time: distance / 40).
void aiAMUR_GENERIC::GetShootArrowPhysParam(m3dV *org, physINIT *p)
{
    m3dV fw, d, m;
    inst->GetDir(&fw);
    fw.y = 0;
    if (!(wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(this, 40.0f, &m)) {
        m.y += 1.0f;
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
    d.x *= 40.0f;
    d.y *= 40.0f;
    d.z *= 40.0f;
    float dx = org->x - m.x, dz = org->z - m.z;
    float t = m3dSqrt(dx * dx + dz * dz) * 0.025f;
    p->vel[0] = d.x;
    p->vel[1] = d.y;
    p->vel[2] = d.z;
    p->vel[1] = ((m.y - org->y) - t * t * -5.0f) / t;
    p->accel[0] = m3dVUnitY.x * -10.0f;
    p->accel[1] = m3dVUnitY.y * -10.0f;
    p->accel[2] = m3dVUnitY.z * -10.0f;
}

// v12 0x00438570  (feb 0x00432340)
// One arrow from "attack" (its matrix): 8 damage, a 0.3 sphere, 7 s at most, two hits; lit like the amur's floor.
// The volley's last: a 0.1 s pause.
void aiAMUR_GENERIC::LaunchOneProjectile()
{
    pjlPJL_INIT in;
    memset(&in, 0, sizeof(in));
    in.phys.packId = 0;
    in.phys.tag = 0x40;
    in.phys.size = 0x55;
    in.phys.unk24 = 1;
    in.damage = -1.0f;
    in.radius = 0;
    in.weaponType = -1;
    in.isGeneral = -1;
    in.ownerId = 0xFFFFFFFF;
    in.termDelay = -1.0f;
    in.explosive = 1;
    animCREATE_DATA cd;
    objOBJ *o = attackObj;
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
    in.damage = 8.0f;
    in.radius = 0;
    in.isGeneral = 4;                                   // (feb 3)
    in.explosive = 0;
    GetShootArrowPhysParam(&org, &in.phys);
    struct { pjlPJL_INIT i; u32 extra; } blk;           // (1.2 sends 0x55 bytes: a zero dword after the init)
    blk.i = in;
    blk.extra = 0;
    entENTITY *a = entCreate(gsScenePtr, (char *)"pjl_amur_arrow", (char *)"pjl_amur_arrow", &cd, &blk, 0);
    if (!a)
        return;
    gsEffApplyLMChar(a, &floor, 0);
    if (--shotsLeft == 0)
        shotPause = 0.1f;
}

static_assert(sizeof(aiAMUR_GENERIC) == 0x5A7, "aiAMUR_GENERIC");
static_assert(sizeof(pjlAMUR_ARROW) == 0x208, "pjlAMUR_ARROW");
