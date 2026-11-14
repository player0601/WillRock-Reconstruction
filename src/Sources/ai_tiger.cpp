// ai_tiger.cpp -- the tiger ("ai_tiger") and the lion ("ai_lion") (Will Rock 1.2, v12.exe; feb obj ai_tiger.obj)
//
// One class: "_lion" in the class name makes a lion (60 hp, its own sounds, harder bites). It idles calm or angry and
// roars; it trots (3.5 m/s) or, in the arena ("_arena", AI_TIGER isArena, or no "_sniper"), gallops at 16. Close up and
// facing the enemy (within 40 degrees, 0.2..3 m, the enemy slower than 0.8 m/s) it bites left or right; galloping within
// 17 m of where it will meet the enemy it pounces (ATTACK_JUMP), its body striking what it lands on (Kick: thrown aside
// at 15 m/s), and turns about after (JUMP_END_180). A "_sniper" one keeps its place and turns on the spot. Hits: a
// light one (most guns, 30%) plays SHOT_STAY, a heavy one (rockets and the like, or near shots of the big guns)
// SHOT_BY_BLAST. Damage falls from 1.5x within 1.1 m to nothing at 1.7 m: a pounce 15 (lion 20), its landing 2x (lion
// +3), a bite 10 (lion 13).

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../../include/ai/ai_world.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list);  // v12 0x004B1E30
int __fastcall ClassifyCollisionTarget(cdtINFO *ci);                // v12 0x004537A0 (ai_npc_wr.cpp)

// v12 0x0064DAA8.. and 0x0064DF90.. 12 sounds each, 0x68 apart (static initialisers v12 0x0045CDA0 / 0x0045D0F0)
static sncSOUND_DESCR_3D tigerSnd[12];
static sncSOUND_DESCR_3D lionSnd[12];
enum { TSND_ROAR_1, TSND_ROAR_2, TSND_CLOSE_1, TSND_CLOSE_2, TSND_DEATH_1, TSND_DEATH_2, TSND_FAR_ATTACK, TSND_IDLE_1,
       TSND_IDLE_2, TSND_HIT_1, TSND_HIT_2, TSND_WALK };

static float tigerEps2;                 // v12 0x0064DF8C (1e-6 squared, v12 0x0045DA50; unused)
static float tigerTurnHeight;           // v12 0x0064DAA0 the enemy's side at the last turn (written only)
static int   tigerKicks;                // v12 0x0064DF88 a count of the enemies thrown (written only)
static void aiNPC_SelectAttackOrCrumbGoal(aiNPC *n);

static void tigerSndSet(sncSOUND_DESCR_3D *d, const char *name, int flags, float mn, float mx)
{
    d->name.SetName((char *)name);
    d->flags = flags;
    d->volume = 0.0f;
    d->delay = 0;
    d->count = 0;
    d->time = 0;
    d->data = 0;
    d->dataFlags = 0;
    d->minDist = mn;
    d->maxDist = mx;
    d->vtbl = 0;
}

static struct aiTIGER_SND_INIT {
    aiTIGER_SND_INIT()
    {
        static const char *tn[12] = { "tiger_roar_1", "tiger_roar_2", "tiger_close_attack", "tiger_close_attack",
                                      "tiger_death_1", "tiger_death_2", "tiger_far_attack", "tiger_idle", "tiger_idle",
                                      "tiger_being_hit_1", "tiger_being_hit_2", "tiger_walk" };
        static const char *ln[12] = { "lion_roar_1", "lion_roar_2", "lion_close_attack_1", "lion_close_attack_2",
                                      "lion_death_1", "lion_death_2", "lion_far_attack", "lion_idle_1", "lion_idle_2",
                                      "lion_being_hit_1", "lion_being_hit_2", "lion_walk" };
        static const float mn[12] = { 15, 15, 15, 15, 30, 30, 15, 15, 15, 25.5f, 25.5f, 10 };
        static const float mx[12] = { 50, 50, 25, 25, 80, 80, 50, 50, 50, 70, 70, 33 };
        for (int i = 0; i < 12; i++) {
            int f = i == TSND_WALK ? 0x10000 : 0;
            tigerSndSet(&tigerSnd[i], tn[i], f, mn[i], mx[i]);
            tigerSndSet(&lionSnd[i], ln[i], f, mn[i], mx[i]);
        }
        tigerEps2 = 1e-6f * 1e-6f;
    }
} aiTigerSndInit;

// v12 0x0045D440  (feb 0x00454630)
int __fastcall aiTigerInit()
{
    if (!entRegisterClass((char *)"ai_lion", 0x244E5043, 0x32, aiTIGER::Create, (char *)"ai_lion", 0))
        return 0;
    if (!entRegisterClass((char *)"ai_tiger", 0x244E5043, 0x32, aiTIGER::Create, (char *)"ai_tiger", 0))
        return 0;
    if (!sncLinkSndListData(12, tigerSnd))
        return 0;
    return sncLinkSndListData(12, lionSnd) != 0;
}

// v12 0x0045D4B0  (feb 0x004546A0)
// "_arena" -> runs (0x100), "_sniper" -> keeps its place (0x800); neither -> runs. "_lion": 60 hp, fallback 130 (else
// 120).
void aiTIGER::ParseNameClass()
{
    int plain = 1;
    const char *c = inst->className ? inst->className : "";
    aiNPC_WR::ParseNameClass();
    if (strstr(c, "_arena")) {
        tigerFlags = (tigerFlags & ~0x800u) | 0x100;
        plain = 0;
    }
    if (strstr(c, "_sniper")) {
        tigerFlags = (tigerFlags & ~0x100u) | 0x800;
        plain = 0;
    }
    if (strstr(c, "_lion")) {
        unk98 = 8;
        tigerFlags |= 0x8000;
        fallbackAngle = 130.0f;
        maxHealth = 60.0f;
        health = 60.0f;
    } else
        fallbackAngle = 120.0f;
    if (plain)
        tigerFlags = (tigerFlags & ~0x800u) | 0x100;
}

// v12 0x0045D590  (feb 0x00454780)
entENTITY *__fastcall aiTIGER::Create(animINST *)
{
    aiTIGER *t = new aiTIGER;                           // operator new(0x597), aiNPC ctor
    if (!t)
        return 0;
    t->wrFlags = 0;
    t->bloodObj = 0;
    t->unk98 = 7;
    return t;
}

// v12 0x0045D5E0  (feb 0x004547D0)
// The idles change among themselves; the pounce (GALL_TO_JUMP, ATTACK_JUMP, JUMP_END_180, back to GALLOP); the hits from
// anything; bites from the idles and the gallop; one-way: deaths, turns, the turn after a pounce.
BOOL32 aiTIGER::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 16;
    u32 *blk = (u32 *)operator new(0x884);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 16;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 16; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    static const char *names[16] = { "TIGER_SEQ_IDLE_CALM", "TIGER_SEQ_IDLE_ROAR", "TIGER_SEQ_IDLE_ANGRY",
                                     "TIGER_SEQ_ATTACK_LCLOSE", "TIGER_SEQ_TROT", "TIGER_SEQ_GALLOP",
                                     "TIGER_SEQ_ATTACK_JUMP", "TIGER_SEQ_SHOT_BY_BLAST", "TIGER_SEQ_DEATH_BY_BLAST",
                                     "TIGER_SEQ_ATTACK_RCLOSE", "TIGER_SEQ_GALL_TO_JUMP", "TIGER_SEQ_JUMP_END_180",
                                     "TIGER_SEQ_TURN_LEFT", "TIGER_SEQ_TURN_RIGHT", "TIGER_SEQ_DEATH_STAY",
                                     "TIGER_SEQ_SHOT_STAY" };
    for (int i = 0; i < 16; i++)
        _entAnimSetSeqName(t, i, (char *)names[i]);
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(16, 0);
    if (!s) {
        if (t->seq)
            operator delete((u32 *)t->seq - 1);
        return 0;
    }
    t->aniSystem = s;
    s->SetTranParam(0, 1, 1, 1, 0);
    s->SetTranParam(1, 0, 1, 0, 0);
    s->SetTranParam(0, 2, 1, 2, 0);
    s->SetTranParam(2, 0, 1, 0, 0);
    s->SetTranParam(1, 2, 1, 2, 0);
    s->SetTranParam(2, 1, 1, 1, 0);
    s->SetTranParam(10, 6, 1, 6, 0);
    s->SetTranParam(6, 11, 1, 11, 0);
    s->SetTranParam(11, 5, 1, 5, 0);
    s->SetTranParam(7, 5, 1, 5, 0);
    s->SetTranParamFromAny(15, 0, 1, 15);
    s->SetTranCycleAnim(15);
    s->SetTranParamFromAny(7, 0, 1, 7);
    s->SetTranCycleAnim(7);
    s->SetTranParamFromAny(14, 0, 1, 14);
    s->SetTranParamFromAny(8, 0, 1, 8);
    s->SetTranParam(0, 3, 0, 3, 1);
    s->SetTranParam(0, 9, 0, 9, 1);
    s->SetTranParam(2, 3, 0, 3, 1);
    s->SetTranParam(2, 9, 0, 9, 1);
    s->SetTranParam(2, 5, 0, 5, 1);
    s->SetTranParam(5, 6, 0, 10, 1);
    s->SetTranParam(5, 3, 0, 3, 1);                     // (1.2: bites from the gallop)
    s->SetTranParam(5, 9, 0, 9, 1);
    s->SetTranTypeToAny(8, 2, 1, 8);
    s->SetTranTypeToAny(14, 2, 1, 14);
    s->SetTranParam(8, 8, 2, 8, 1);
    s->SetTranParam(12, 12, 2, 12, 1);
    s->SetTranParam(13, 13, 2, 13, 1);
    s->SetTranParam(14, 14, 2, 14, 1);
    s->SetTranParam(11, 11, 2, 11, 1);
    if (old)
        operator delete((u32 *)old - 1);
    static const int once[8] = { 6, 11, 7, 8, 14, 12, 13, 11 };   // (seq 11 also 4)
    for (int i = 0; i < 8; i++)
        t->seq[once[i]].unk84 |= 1;
    t->seq[11].unk84 |= 4;
    return 1;
}

// v12 0x0045DA70  (feb 0x00454C30)
// 50 hp; arriving at 2.85 m; 300 m activation; trotting 3.5 m/s, turning 70 deg/s (20 per step); the corpse lasts 20 s
// (fading 17); a sphere voice; its middle and blood from "ROOT".
int aiTIGER::ProcessINIT(void *init)
{
    int arena;
    if (!aiNPC_WR::ProcessINIT(init))
        return 0;
    maxHealth = 50.0f;
    navArriveDist = 2.85f;
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    alertAngle = 180.0f;
    moveSpeed = 3.5f;
    turnSpeed = 70.0f;
    slowAttackAngle = 40.0f;
    navUnk1A7 = 10.0f;
    health = 50.0f;
    tigerFlags = 0;
    hitSeq = 0xE;
    attackSeq = 3;
    turnSeq = 2;
    enemyHeight = 888.888f;
    aiFlags2 |= 0x30002;
    attackPause = 0;
    timer57B = 0;
    fleeTimer = 0;
    hitTimer = 0;
    turnTimer = 0;
    maxTurnStep = 20.0f;
    corpseTime = 20.0f;
    corpseFadeTime = 17.0f;
    hitCanEnd = 1;
    tigerFlags = 0x800;
    if (GetBool((char *)"AI_TIGER", (char *)"isArena", &arena) && arena)
        tigerFlags = (tigerFlags & ~0x800u) | 0x100;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;          // (a sncCHANNEL_SPH there)
    if (c) {
        c->minDist = 0.0f;
        c->maxDist = 1.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    if (!(rootObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    return 1;
}

// v12 0x0045DCE0 (name ours)
// Dying: DEATH_STAY; a light hit or a heavy one from behind the enemy's run: SHOT_STAY; a heavy one: SHOT_BY_BLAST
// (within 60 degrees of its facing), else SHOT_STAY. The hit sequence starts (4); an attack ends.
int aiTIGER::SelectHitSeq()
{
    int s = hitSeq;                                     // (never kept: only called with a hit flag set)
    if (aiFlags & 0x200000)
        s = 14;
    else if (tigerFlags & 1)
        s = 15;
    else if (tigerFlags & 2) {
        if (aiFlags & 0x40000)
            s = 15;
        else if (enemyAngle < 60.0f)
            s = 7;
        else
            s = 15;
    }
    u32 f = aiFlags;
    aiFlags = f & ~0x1000u;
    hitCanEnd = (~f >> 21) & 1;
    tigerFlags = (tigerFlags & ~0x33u) | 4;
    return s;
}

// v12 0x0045DD70 (1.2 NEW)
float aiTIGER::GetAnimPlayRate()
{
    return inst->seq == 15 ? 2.0f : 1.0f;
}

// v12 0x0045DD90  (feb 0x00454EA0)
int aiTIGER::EvalAnimGoalSeq()
{
    int idle = m3dRandom() < 0.8f ? 2 : 1;              // angry idle, now and then a roar
    int move;
    if (aiFlags & 1) {
        if ((aiFlags & 0x10000000) && 0.2f < enemyDist && enemyDist <= 3.0f && enemyAngle <= slowAttackAngle &&
            enemy.GetSpeedScalar() < 0.8f)
            move = m3dRandSelect(9, 3);
        else
            move = enemy.GetSpeedScalar() > 1.0f ? 5 : 4;
    } else if (!(tigerFlags & 0x100))
        move = 4;
    else if (enemyDist > 4.0f)
        move = 5;
    else
        move = enemy.GetSpeedScalar() > 1.0f ? 5 : 4;
    if (aiFlags & 0x200000)
        return hitSeq != 8 ? 14 : 8;
    if (enemy.IsDead() || (aiFlags & 4) || (aiFlags & 0x10000))
        return idle;
    u32 tf = tigerFlags;
    if (tf & 3)
        return hitSeq = SelectHitSeq();
    if (tf & 4)
        return hitSeq;
    if (!(idleTimer <= 1e-6f))
        return m3dRandSelect(1, 2, 1);
    if (aiFlags & 0x1000) {
        if (attackSeq < 0) {
            int a;
            if (tf & 0x10) {
                int left = enemyAngle > 20.0f ? (enemyHeight <= 0.0f) : (m3dRandom() <= 0.5f);
                a = left ? 9 : 3;
            } else
                a = 6;
            tigerFlags &= ~0x30u;
            aiFlags |= 0x1000;
            attackSeq = a;
        }
        return attackSeq;
    }
    if (tf & 0x30) {
        int a;
        if (tf & 0x10) {
            int left = enemyAngle > 20.0f ? (enemyHeight <= 0.0f) : (m3dRandom() <= 0.5f);
            a = left ? 9 : 3;
        } else
            a = 6;
        tigerFlags &= ~0x30u;
        aiFlags |= 0x1000;
        attackSeq = a;
        return a;
    }
    if (tf & 0x1000)
        return turnSeq;
    if (tf & 0x2000)
        return turnSeq = enemyHeight > 0.0f ? 13 : 12;
    if (aiFlags & 0x40000) {
        if (!(enemyDist < 3.0f) || !(enemyAngle < 50.0f))
            return idle;
        return m3dRandSelect(9, 3);
    }
    if (tf & 0x80)
        return 5;
    return move;
}

// a reach factor: 1.5 within 1.1 m, falling to 0 at 1.7 m (the inlined interpolation v12 keeps, with its zero-range test)
static float tigerReach(float d)
{
    if (d < 1.1f)
        return 1.5f;
    if (d > 1.7f)
        return 0.0f;
    float range = 0.6f;
    if (fabsf(range) < 1e-6f)
        return 0.75f;
    return 1.5f - (d - 1.1f) * 2.5f;
}

// v12 0x0045E0C0  (feb 0x004551C0)
// The better of the reach from its mouth (0.5 m ahead in a pounce, else 3 m) and from its body; the hit at the mouth.
int aiTIGER::CalcDamageToTarget(dmgGENERAL *g)
{
    m3dV pos, dir, mouth, t;
    GetPosRooted(&pos);
    inst->GetDir(&dir);
    float k = attackSeq == 6 ? 0.5f : 3.0f;
    mouth.x = dir.x * k + pos.x;
    mouth.y = dir.y * k + pos.y;
    mouth.z = dir.z * k + pos.z;
    GetPlayerTargetPos(&t);
    float dx = t.x - mouth.x, dy = t.y - mouth.y, dz = t.z - mouth.z;
    float f1 = tigerReach(m3dSqrt(dz * dz + dy * dy + dx * dx));
    dx = t.x - pos.x;
    dy = t.y - pos.y;
    dz = t.z - pos.z;
    float f2 = tigerReach(m3dSqrt(dx * dx + dy * dy + dz * dz));
    g->pos = mouth;
    float f = f1 > f2 ? f1 : f2;
    g->amount = f;
    int lion = (tigerFlags & 0x8000) != 0;
    if (attackSeq == 6) {
        if (tigerFlags & 8)
            g->amount = (lion ? 20.0f : 15.0f) * f;
        else
            g->amount = lion ? 3.0f + f : f + f;
    } else if (attackSeq == 3 || attackSeq == 9)
        g->amount = (lion ? 13.0f : 10.0f) * f;
    else
        g->amount = 0;
    return g->amount > 0.1f;
}

// v12 0x0045E4B0  (feb 0x00455450): 3 s of running off
void aiTIGER::SelectGoalToRunAway(float dist)
{
    aiNPC_WR::SelectGoalToRunAway(dist);
    fleeTimer = 3.0f;
    tigerFlags |= 0x80;
}

// v12 0x0045E4E0  (feb 0x00455480)
// A hit sequence's first frame ends it (when it is not a death); the pounce: in the air (frame 0), its landing (2: the
// turn about follows), its end (3: a "_sniper" turns about if it struck, 60% of the time; one that missed far off turns
// about); the turn about's end: run off (or a "_sniper" pauses); the bites strike at frame 0; the turns end.
void aiTIGER::ProcessActionFrame()
{
    if ((inst->IsActionFrame(7, 0) || inst->IsActionFrame(8, 0) || inst->IsActionFrame(15, 0) ||
         inst->IsActionFrame(14, 0)) && hitCanEnd)
        tigerFlags &= ~4u;
    if (inst->IsActionFrame(6, 0))
        tigerFlags |= 0x408;
    inst->IsActionFrame(6, 1);
    if (inst->IsActionFrame(6, 2)) {
        attackSeq = 11;
        tigerFlags &= ~8u;
    }
    if (inst->IsActionFrame(6, 3)) {
        u32 f = tigerFlags & ~0x400u;
        tigerFlags = f;
        if (f & 0x800) {
            if (!(f & 0x200))
                attackSeq = 11;
            else {
                tigerFlags = f & ~0x200u;
                if (m3dRandom() < 0.6f)
                    attackSeq = 11;
                else {
                    aiFlags &= ~0x1000u;
                    attackPause = 2.0f;
                }
            }
        } else if (enemyDist > 0.5f && enemyAngle > 80.0f)
            attackSeq = 11;
        else {
            aiFlags &= ~0x1000u;
            tigerFlags = f & ~0x200u;
            attackPause = 2.0f;
        }
    }
    if (inst->IsActionFrame(11, 0)) {
        tigerFlags &= ~0x200u;
        aiFlags &= ~0x1000u;
        if (tigerFlags & 0x800)
            attackPause = 2.0f;
        else if (enemyAngle > 160.0f && enemyDist < 6.0f)
            aiNPC_SelectAttackOrCrumbGoal(this);
        else
            SelectGoalToRunAway(-m3dRandRange(1.0f, 2.0f));
    }
    if (inst->IsActionFrame(3, 0) || inst->IsActionFrame(9, 0)) {
        dmgGENERAL g;
        attackSeq = 3;
        if (CalcDamageToTarget(&g) && enemy.ent)
            msgSystem.SendMsg(enemy.ent, 0x4C6, &g, this);
    }
    if (inst->IsActionFrame(3, 1) || inst->IsActionFrame(9, 1)) {
        attackPause = 0.5f;
        aiFlags &= ~0x1000u;
    }
    if (inst->IsActionFrame(12, 0) || inst->IsActionFrame(13, 0)) {
        turnTimer = 0.5f;
        tigerFlags &= ~0x3000u;
    }
}

// v12 0x0045E7E0  (feb 0x00455740)
// The arrival distance: 2.85 galloping, 1 trotting, 3.6 biting, else easing back down to 2.85 (1 m/s); running off it
// stops within 2 m of its goal.
void aiTIGER::Think()
{
    aiNPC_WR::Think();
    FillAttackFlags();
    int s = inst->seq;
    if (s == 5)
        navArriveDist = 2.85f;
    else if (s == 4)
        navArriveDist = 1.0f;
    else if (s == 3 || s == 9)
        navArriveDist = 3.6f;
    else {
        float d = navArriveDist - 2.85f;
        if (d > 0.0f) {
            float lo = 0.0f, hi = gsElapsedTime;
            if (hi < 0.0f) {
                lo = hi;
                hi = 0.0f;
            }
            if (d < lo)
                d = lo;
            else if (d > hi)
                d = hi;
        }
        navArriveDist -= d;
    }
    if (tigerFlags & 0x80) {
        m3dV p;
        inst->GetPos(&p);
        float dx = p.x - goalPos.x, dz = p.z - goalPos.z;
        if (m3dSqrt(dx * dx + dz * dz) < 2.0f)
            tigerFlags &= ~0x80u;
    }
    UpdateInAreaFlags();
}

// v12 0x0045E9C0  (feb 0x00455880): an attack / turn wanted has started
void aiTIGER::PostThink()
{
    aiNPC_WR::PostThink();
    int s = inst->seq;
    if ((tigerFlags & 0x30) && (s == 6 || s == 10 || s == 3 || s == 9)) {
        tigerFlags &= ~0x30u;
        aiFlags |= 0x1000;
    }
    if ((tigerFlags & 0x2000) && (s == 12 || s == 13)) {
        tigerTurnHeight = enemyHeight;
        tigerFlags = (tigerFlags & ~0x2000u) | 0x1000;
    }
}

// v12 0x0045EA40  (feb 0x00455900)
// Not attacking or pausing: a bite when the enemy is close in front and slow; galloping within 40 m (not a "_sniper"),
// a pounce when it will meet the enemy within 17 m nearly ahead (10 degrees).
void aiTIGER::FillAttackFlags()
{
    int s = inst->seq;
    if (s == 3 || s == 9 || s == 6)
        return;
    if (aiFlags & 0x1000)
        return;
    if (attackPause > 0.01f)
        return;
    if (tigerFlags & 0x84)
        return;
    tigerFlags &= ~0x30u;
    if ((aiFlags & 0x10000000) && 0.2f < enemyDist && enemyDist <= 3.0f && enemyAngle <= slowAttackAngle &&
        enemy.GetSpeedScalar() < 0.8f)
        tigerFlags |= 0x10;
    if (tigerFlags & 0x800)
        return;
    if (enemyDist > 40.0f)
        return;
    if (inst->seq != 5)
        return;
    m3dV mp, p, dir, v;
    if (!aiAttackPlanner->GetEnemyMeetPoint(this, 16.0f, &mp))
        return;
    inst->GetPos(&p);
    inst->GetDir(&dir);
    v.x = mp.x - p.x;
    v.y = mp.y - p.y;
    v.z = mp.z - p.z;
    float d = m3dSqrt(v.z * v.z + v.y * v.y + v.x * v.x);
    m3xzAngleVector(&v, &dir);                          // (the result is not used)
    if (fabsf(enemyHeight) <= 10.0f && d <= 17.0f)
        tigerFlags |= 0x20;
}

// v12 0x0045ECB0  (feb 0x00455B70)
// A "_sniper" wants a turn when the enemy is 55 degrees off; it stands through the hits, deaths, idles, bites, turns
// and pounce; 3.5 m/s trotting, 16 running. Turning: none in the hits, deaths, idles and turns, 70 in a pounce (none in
// the air), 20 biting, else 70 (270 not yet inited or the goal 60 degrees off).
void aiTIGER::SetMovementSpeed()
{
    if ((tigerFlags & 0x800) && turnTimer <= 0.01f) {
        if (enemyAngle < 55.0f)
            tigerFlags &= ~0x2000u;
        else
            tigerFlags |= 0x2000;
    }
    int s = inst->seq;
    if (s == 6 || s == 11 || s == 7 || s == 8 || s == 14 || s == 15 || s == 3 || s == 9 || s == 12 || s == 13 ||
        s == 2 || s == 1)
        aiFlags |= 0x200;
    int full = 0;
    if (tigerFlags & 0x100) {
        if (s == 4) {
            moveSpeed = 3.5f;
            full = 1;
        } else
            moveSpeed = 16.0f;
    } else
        moveSpeed = 3.5f;
    if (!full) {
        if (s == 11 || s == 7 || s == 8 || s == 15 || s == 14 || s == 12 || s == 13 || s == 0 || s == 2 || s == 1) {
            turnSpeed = 0;
            goto done;
        }
        if (s == 6) {
            turnSpeed = (tigerFlags & 0x400) ? 0.0f : 70.0f;
            goto done;
        }
        if (s == 3 || s == 9) {
            turnSpeed = 20.0f;
            goto done;
        }
    }
    turnSpeed = 70.0f;
    if (!(wrFlags & 0x10000))
        turnSpeed = 270.0f;
    if (!(aiFlags2 & 8) && goalAngle > 60.0f)
        turnSpeed = 270.0f;
done:
    aiFlags &= ~0x20u;
    if (s == 6 || s == 10) {
        aiFlags |= 0x20;
        unk37F = 16.0f;
    }
}

// v12 0x0054FCB0: aiNPC::SelectGoal's second half (attack point or the crumbs), called on its own here
static void aiNPC_SelectAttackOrCrumbGoal(aiNPC *n)
{
    if (n->aiFlags & 0x10000000) {
        aiAttackPlanner->GetAttackGoalPoint(n, &n->goalPos);
        n->GetPlayerTargetPos(&n->wayPointPos);
        return;
    }
    aiAttackPlanner->GetCrumbGoalPoint(n, &n->goalPos);
    n->wayPointPos = n->goalPos;
    if (n->aiFlags2 & 0x40)
        aiAttackPlanner->ClipPointByNavArea(n, &n->goalPos);
    n->aiFlags |= 8;
}

// v12 0x0045EEA0  (feb 0x00455D20)
// Not escaping or running off: close (10 m) with the enemy behind it (80 degrees), it runs off (not a "_sniper"); the
// goal at its own height.
int aiTIGER::SelectGoal()
{
    if (!SelectEscapeWhenOut() && !(tigerFlags & 0x80)) {
        if (!(tigerFlags & 0x800) && enemyDist <= 10.0f && enemyAngle > 80.0f)
            SelectGoalToRunAway(m3dRandRange(1.0f, 2.0f));
        else
            aiNPC::SelectGoal();
    }
    m3dV p;
    inst->GetPos(&p);
    goalPos.y = p.y;
    RestrictGoalByBanCross();
    return 1;
}

// v12 0x0045EF40  (feb 0x00455DC0): the turn about after a pounce turns it from the enemy (90 deg/s, about its ROOT)
void aiTIGER::MoveToGoal()
{
    if (inst->seq != 11) {
        aiNPC_WR::MoveToGoal();
        return;
    }
    m3dV e, p, v, out;
    enemy.GetPos(&e);
    GetPosRooted(&p);
    v.x = p.x - e.x;
    v.y = p.y - e.y;
    v.z = p.z - e.z;
    m3dNormalize(&v);
    m3dClampVector_A(&dir, &v, gsElapsedTime * 90.0f, &out, 0, 0);
    inst->SetDir_XZ_UsePivot(&out, &p);
}

// v12 0x0045F000  (feb 0x00455E80)
void aiTIGER::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&attackPause, gsElapsedTime);
    m3dUpdateTimeField(&timer57B, gsElapsedTime);
    m3dUpdateTimeField(&turnTimer, gsElapsedTime);
    if (m3dUpdateTimeField(&fleeTimer, gsElapsedTime) || (aiFlags & 1))
        tigerFlags &= ~0x80u;
    if (m3dUpdateTimeField(&hitTimer, gsElapsedTime))
        tigerFlags &= ~0x200u;
}

// v12 0x0045F090  (feb 0x00455EF0)
// Weapons 0, 1, 3, 4, 7: a light hit 30% of the time (not in a hit already); 2, 8: a heavy hit within 40 m, else light;
// 5, 6, 9, 10: heavy. Dying from a heavy hit close in front: DEATH_BY_BLAST.
int aiTIGER::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (!(entFlags & 4))
        return 1;
    if (d->type == 1) {
        u32 w = (u32)((dmgWEAPON *)d)->weaponType;
        static const u8 kind[11] = { 0, 0, 1, 0, 0, 2, 2, 0, 1, 2, 2 };
        if (w <= 10) {
            switch (kind[w]) {
            case 0:
                if (!(tigerFlags & 4) && m3dRandom() < 0.3f)
                    tigerFlags |= 1;
                break;
            case 1:
                tigerFlags |= enemyDist < 40.0f ? 2 : 1;
                break;
            case 2:
                tigerFlags |= 2;
                break;
            }
        }
    }
    aiNPC_WR::ProcessCDT_FIREABLE(d);
    if ((aiFlags & 0x200000) && (tigerFlags & 2) && enemyAngle < 60.0f)
        hitSeq = 8;
    return 1;
}

// v12 0x0045F170  (feb 0x00455FD0)
// The enemy in the cone ahead (1.5 m, 0.2 .. 60): thrown aside (to the side it is on) and a little up, at 15 m/s.
void aiTIGER::Kick()
{
    m3dV e, p, d, v;
    enemy.GetPos(&e);
    GetPosRooted(&p);
    inst->GetDir(&d);
    if (!m3dIsBelongPointCone(&e, &p, &d, 1.5f, 0.2f, 60.0f))
        return;
    d.y = 0;
    if (0.0f < enemyHeight) {
        v.x = d.x + d.z;
        v.z = d.z - d.x;
    } else {
        v.x = d.x - d.z;
        v.z = d.z + d.x;
    }
    v.y = 0;
    m3dNormalize(&v);
    v.y = 0.3f;
    m3dNormalize(&v);
    v.x *= 15.0f;
    v.y *= 15.0f;
    v.z *= 15.0f;
    enemy.ApplySpeed(&v);
    tigerKicks++;
}

// v12 0x0045F280  (feb 0x004560E0)
// Its body struck the enemy (once an attack): thrown aside and the damage; 0.2 s before another; arriving at 3.6 m.
void aiTIGER::ProcessCDT(cdtINFO *ci)
{
    dmgGENERAL g;
    if (ClassifyCollisionTarget(ci) != 1 || (tigerFlags & 0x200))
        return;
    Kick();
    if (CalcDamageToTarget(&g) && enemy.ent)
        msgSystem.SendMsg(enemy.ent, 0x4C6, &g, this);
    hitTimer = 0.2f;
    tigerFlags |= 0x200;
    navArriveDist = 3.6f;
}

// v12 0x0045F310  (feb 0x00456160): one sound a frame, the first of: dying, death, the pounce, the hits, a roar, a
// bite, the idle, a step
void aiTIGER::PlaySounds()
{
    sncSOUND_DESCR_3D *l = (tigerFlags & 0x8000) ? lionSnd : tigerSnd;
    u32 f = unk108;
    sncSOUND_DESCR_3D *d = 0;
    if (f & 4)
        d = &l[TSND_DEATH_2];
    else if (f & 0x40000)
        d = &l[TSND_DEATH_1];
    else if (f & 8)
        d = &l[TSND_FAR_ATTACK];
    else if (f & 0x20)
        d = &l[TSND_HIT_2];
    else if (f & 0x200000)
        d = &l[TSND_HIT_1];
    else if (f & 1)
        d = &l[m3dRandSelect(0, 1)];
    else if (f & 2)
        d = &l[m3dRandSelect(2, 3)];
    else if (f & 0x10)
        d = &l[m3dRandSelect(7, 8)];
    else if (f & 0x40)
        d = &l[TSND_WALK];
    if (d)
        voice->Play((sncSOUND_DESCR *)d, 0);
    aiNPC::PlaySounds();
}

// v12 0x0045F470  (feb 0x004562F0): a gallop step; a sequence's start
void aiTIGER::FillSoundFlags()
{
    if (inst->IsActionFrame(5, 0))
        unk108 |= 0x40;
    if (!(inst->flags & 0x4000))
        return;
    switch (inst->seq) {
    case 1:
        unk108 |= 1;
        break;
    case 2:
        unk108 |= 0x10;
        break;
    case 3:
    case 9:
        unk108 |= 2;
        break;
    case 4:
    case 5:
        unk108 |= 0x40;
        break;
    case 6:
        unk108 |= 8;
        break;
    case 7:
        unk108 |= 0x200000;
        break;
    case 8:
        unk108 |= 0x40000;
        break;
    case 14:
        unk108 |= 4;
        break;
    case 15:
        unk108 |= 0x20;
        break;
    }
}

// v12 0x0045CD90 / 0x0045A950 (shared): its flags are the client's layer-2 state
int aiTIGER::GetLayer2State()
{
    return (int)tigerFlags;
}
void aiTIGER::SetLayer2State(int s)
{
    tigerFlags = (u32)s;
}

static_assert(sizeof(aiTIGER) == 0x597, "aiTIGER");
