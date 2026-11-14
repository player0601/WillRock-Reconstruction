// ai_croc.cpp -- aiCROC, the crocodile ("ai_croc") (Will Rock 1.2, v12.exe; feb obj ai_croc.obj)
//
// A swimmer (aiNPC_WR_FLY's physics): it keeps under the surface of its water (0.6 below; out of water it heads 5 m
// down), swims at the enemy and bites when the enemy is close (4.5 m) and within 30 degrees of its jaws: the bite
// hurts 15 within 0.5 m of the "ATTACKfocus", falling to none at 1.9 m. 1.2: it bites only when that would hurt
// (else it swims off for 2 s: SelectGoalToRunAway), no sound on dying, no appearing effect.

#include "game.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list);  // v12 0x004B1E30
BOOL32 __fastcall waterIsUnderWater(m3dV *p, float *level, float r); // water.cpp

// v12 0x0064A370 its bite (static init v12 0x0043F1A0): heard 12 (8 x 1.5) to 25 m
static sncSOUND_DESCR_3D crocSnd;
static struct aiCROC_SND_INIT {
    aiCROC_SND_INIT()
    {
        crocSnd.name.SetName((char *)"croc_bite");
        crocSnd.flags = 0x40000;
        crocSnd.volume = 0;
        crocSnd.delay = 0;
        crocSnd.count = 0;
        crocSnd.time = 0;
        crocSnd.data = 0;
        crocSnd.dataFlags = 0;
        crocSnd.minDist = 8.0f * 1.5f;
        crocSnd.maxDist = 25.0f * 1.0f;
        crocSnd.vtbl = 0;
    }
} aiCrocSndInit;

// The bite's harm by the distance from the jaws: 15 within 0.5 m, none past 1.9 m, linear between (v12 inline).
static float crocBiteHarm(float d)
{
    if (!(d > 0.5f))
        return 15.0f;
    if (!(d <= 1.9f))
        return 0.0f;
    return 15.0f - (d - 0.5f) * 10.714286f;
}

// v12 0x0043F230  (feb 0x00438B60) exact
int __fastcall aiCrocInit()
{
    if (!entRegisterClass((char *)"ai_croc", 0x244E5043, 0x32, aiCROC::Create, (char *)"ai_croc", 0))
        return 0;
    return sncLinkSndListData(1, &crocSnd) != 0;
}

// v12 0x0043FED0  (feb 0x00439640)
entENTITY *__fastcall aiCROC::Create(animINST *)
{
    aiCROC *c = new aiCROC;                             // operator new(0x58B), aiNPC ctor
    if (!c)
        return 0;
    c->wrFlags = 0;
    c->bloodObj = 0;
    c->crocFlags = 0;
    c->voice = 0;
    c->unk98 = 3;
    return c;
}

// v12 0x0043F270  (feb 0x00438BA0)
// IDLE, SWIM (cycling), BITE, TURN, DEATH.
BOOL32 aiCROC::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 5;
    u32 *blk = (u32 *)operator new(0x2AC);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 5;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 5; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    _entAnimSetSeqName(t, 0, (char *)"CROC_SEQ_IDLE");
    _entAnimSetSeqName(t, 1, (char *)"CROC_SEQ_SWIM");
    _entAnimSetSeqName(t, 2, (char *)"CROC_SEQ_BITE");
    _entAnimSetSeqName(t, 3, (char *)"CROC_SEQ_TURN");
    _entAnimSetSeqName(t, 4, (char *)"CROC_SEQ_DEATH");
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(5, 0);
    if (!s)
        return 0;
    t->aniSystem = s;
    s->SetTranTypeToAny(0, 0, 1, -1);
    s->SetTranCycleAnim(0);
    s->SetTranCycleAnim(1);
    s->SetTranParamFromAny(3, 0, 1, 3);
    s->SetTranCycleAnim(3);
    s->SetTranParamFromAny(4, 0, 1, 4);
    s->SetTranParam(1, 2, 0, 2, 1);
    s->SetTranParam(4, 4, 2, 4, 1);
    if (old)
        operator delete((u32 *)old - 1);
    return 1;
}

// v12 0x0043F410  (feb 0x00438D40)
// Idle 4.56 m/s turning 45; swimming 8.88 / 75; biting 10.27 / 45; else (turning) 180 and the turn flag.
void aiCROC::SetMovementSpeed()
{
    int s = inst->seq;
    if (s == 0) {
        moveSpeed = 4.56f;
        turnSpeed = 45.0f;
    } else if (s == 1) {
        turnSpeed = 75.0f;
        moveSpeed = 8.88f;
    } else if (s == 2) {
        moveSpeed = 10.272f;
        turnSpeed = 45.0f;
    } else {
        turnSpeed = 180.0f;
        aiFlags |= 0x200;
    }
}

// v12 0x0043F480  (feb 0x00438DB0)
// 50 hp, its sizes, a sphere voice, "ROOT" (bleeding, flagged 0x200) and the jaws ("ATTACKfocus").
int aiCROC::ProcessINIT(void *init)
{
    int noSfx = 0;
    if (!aiNPC_WR_FLY::ProcessINIT(init))
        return 0;
    maxHealth = 50.0f;
    health = 50.0f;
    radCDT = 1.0f;
    heightLowTester = 3.0f;
    heightCDT = -0.5f;
    unk17B = 2.5f;
    navArriveDist = 1.9f;
    timeDeactivate = 120.0f;
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    biteAngle = 30.0f;
    fallbackAngle = 140.0f;
    alertAngle = 180.0f;
    maxTurnStep = 70.0f;
    biteTimer = 0;
    fleeTimer = 0;
    corpseTime = 10.0f;
    corpseFadeTime = 9.0f;
    turnSeq = 3;
    passivePhys373 = 0.25f;
    fallAccel = -3.0f;
    fallSpeedMin = -1.5f;
    flyHeight = -0.5f;
    aiFlags2 |= 0x10003;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;          // (a sncCHANNEL_SPH there)
    if (c) {
        c->minDist = 0.0f;
        c->maxDist = 1.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    GetInt((char *)"AI_PROP", (char *)"isNoSFXAppear", &noSfx);  // (1.2: read, unused)
    if (flags2 & 1)
        crocFlags |= 0x1000000;
    if (!(rootObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    attackFocus = objFindName(inst->root, (char *)"ATTACKfocus");
    return attackFocus != 0;
}

// v12 0x0043F740  (feb 0x004390D0)
// Torn apart: meat and blood from its root (along the blow), gone. (1.2: no sound.)
void aiCROC::Explode(dmgDAMAGE *d, int dead)
{
    m3dV *dir = 0;
    if (d->type == 1 || d->type == 3 || d->type == 4)
        dir = (m3dV *)((u8 *)d + 0xE);
    m3dV p;
    bloodObj->GetOrigin(&p);
    StartChunkMeat(3, dead, &p, dir);
    StartBlood(7, dead, &p, 0, 0, dir);
    msgSystem.PostMsg(this, 2, this);
}

// v12 0x0043F7B0  (feb 0x00439160)
// Swimming off (1.2): away from the enemy. Else the usual goal 0.8 higher, kept 0.6 under its water's surface;
// out of the water: 5 m under where it is.
int aiCROC::SelectGoal()
{
    if (fleeTimer > 1e-6f) {
        SelectGoalToRunAway(1.0f);
        wayPointPos = goalPos;
        return 1;
    }
    aiNPC::SelectGoal();
    goalPos.y += 0.8f;
    m3dV e, p;
    float level;
    enemy.GetPos(&e);
    inst->GetPos(&p);
    if (!waterIsUnderWater(&p, &level, 0.1f)) {
        goalPos = p;
        goalPos.y -= 5.0f;
        return 1;
    }
    if (level - 0.5f < goalPos.y)
        goalPos.y = level - 0.6f;
    return 1;
}

// v12 0x0043F8F0  (feb 0x00439240)
// Dead: DEATH; no enemy: IDLE; a turn asked: TURN (kept till done); biting: BITE; else SWIM (IDLE when stopped).
int aiCROC::EvalAnimGoalSeq()
{
    if (aiFlags & 0x200000)
        return m3dRandSelect(4, 4);
    if (enemy.IsDead())
        return 0;
    u32 f = aiFlags;
    if (f & 0x10000)
        return 0;
    if (crocFlags & 0x100000) {
        crocFlags |= 0x10000;
        turnSeq = 3;
        return 3;
    }
    if (crocFlags & 0x10000)
        return turnSeq;
    if (f & 0x1000)
        return 2;
    return ((int)(char)~(u8)f & 4) >> 2;
}

// v12 0x0043F980  (feb 0x004392D0)
void aiCROC::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&biteTimer, gsElapsedTime);
    m3dUpdateTimeField(&fleeTimer, gsElapsedTime);
}

// v12 0x0043F9B0  (feb 0x00439300; 1.2: the bite's harm checked first)
// Not biting, ready, not turning: the enemy within 4.5 m and 30 degrees -> bite (when the jaws would hurt it, else
// swim off for 2 s).
void aiCROC::Think()
{
    aiNPC_WR::Think();
    if (inst->seq == 2)
        return;
    if (aiFlags & 0x1000)
        return;
    if (!(biteTimer <= 0.01f))
        return;
    if (crocFlags & 0x10000)
        return;
    aiFlags &= ~0x1000u;
    if (!(enemyDist <= 4.5f))
        return;
    if (!(enemyAngle <= biteAngle))
        return;
    dmgGENERAL g;                                       // (made, unused)
    if (aiFlags & 1) {
        inst->Validate(4);
        m3dV f, t;
        attackFocus->GetOrigin(&f);
        GetPlayerTargetPos(&t);
        float dx = t.x - f.x, dy = t.y - f.y, dz = t.z - f.z;
        if (!(crocBiteHarm(m3dSqrt(dx * dx + dy * dy + dz * dz)) > 0.1f)) {
            fleeTimer = 2.0f;
            return;
        }
    }
    aiFlags |= 0x1000;
}

// v12 0x0043FC30  (feb 0x00439380)
// The bite (BITE's action frame): done, 1.5..3 s till the next; the enemy hurt by how near the jaws are.
void aiCROC::ProcessActionFrame()
{
    dmgGENERAL g;
    if (!inst->IsActionFrame(2, 0))
        return;
    aiFlags &= ~0x1000u;
    biteTimer = m3dRandRange(1.5f, 3.0f);
    inst->Validate(4);
    m3dV f, t;
    attackFocus->GetOrigin(&f);
    GetPlayerTargetPos(&t);
    float dx = t.x - f.x, dy = t.y - f.y, dz = t.z - f.z;
    g.pos = f;
    g.amount = crocBiteHarm(m3dSqrt(dx * dx + dy * dy + dz * dz));
    if (g.amount > 0.1f && enemy.ent)
        msgSystem.SendMsg(enemy.ent, 0x4C6, &g, this);
}

// v12 0x0043FE70  (feb 0x004395B0; 1.2: only the bite)
void aiCROC::FillSoundFlags()
{
    if ((inst->flags & 0x4000) && inst->seq == 2)
        unk108 = 1;
}

// v12 0x0043FEA0  (feb 0x00439610)
void aiCROC::PlaySounds()
{
    if (unk108 & 1)
        voice->Play(&crocSnd, 0);
    aiNPC::PlaySounds();
}

// v12 0x0043FF20  (feb 0x00439690)
int aiCROC::IsAnimSeqGoodForFallback()
{
    return turnSeq != 4;
}
