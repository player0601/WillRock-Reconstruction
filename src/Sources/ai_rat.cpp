// ai_rat.cpp -- the mechanical rat (Will Rock 1.2, v12.exe; feb obj ai_rat.obj)
//
// Runs at the player, sparks while it jumps, blows up within 1.8 m (and when killed): a 2 m blast of 6 damage, scaled by its own
// health to the other rats (they go in a chain). 1.2: the blast is 5000 on itself, not 10000; it cannot be gibbed.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../../include/sfx_sob/sfx_api.h"

// v12 0x0064C940 the sounds: "rat_die" 0, then the scream / run sounds (5 of them, linked in aiRatInit)
static sncSOUND_DESCR_3D ratSnd[5];

static struct aiRAT_SND_INIT {
    aiRAT_SND_INIT()
    {
        static const char *names[5] = { "rat_exploson", "rat_scream_01", "rat_scream_02", "rat_scream_03", "rat_scream_04" };
        for (int i = 0; i < 5; i++) {
            sncSOUND_DESCR_3D *d = &ratSnd[i];
            d->name.SetName((char *)names[i]);
            d->flags = i ? 0x10000 : 0x60000;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = i ? 15.0f : 25.5f;
            d->maxDist = i ? 50.0f : 70.0f;
            d->vtbl = 0;                                // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} aiRatSndInit;

// v12 0x00455350  (feb 0x0044CC00)
int __fastcall aiRatInit()
{
    if (!entRegisterClass((char *)"ai_rat", 0x244E5043, 0x32, aiRAT::Create, (char *)"ai_rat", 0))
        return 0;
    if (!wrsfxRatInit())
        return 0;
    return sncLinkSndListData(5, ratSnd) != 0;
}

// v12 0x004553A0  (feb 0x0044CC50)
entENTITY *__fastcall aiRAT::Create(animINST *i)
{
    void *mem = operator new(0x583);
    if (!mem)
        return 0;
    aiRAT *r = new (mem) aiRAT;
    r->wrFlags = 0;
    r->bloodObj = 0;
    r->unk98 = 0xC;
    return r;
}

// v12 0x004553F0  (feb 0x0044CCA0)
BOOL32 aiRAT::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 3;
    u32 *blk = (u32 *)operator new(0x19C);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 3;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 3; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    _entAnimSetSeqName(t, 0, (char *)"RAT_SEQ_IDLE");
    _entAnimSetSeqName(t, 1, (char *)"RAT_SEQ_JUMP");
    _entAnimSetSeqName(t, 2, (char *)"RAT_SEQ_TROT");
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(3, 0);
    if (!s) {
        if (t->seq)
            operator delete((u32 *)t->seq - 1);
        return 0;
    }
    t->aniSystem = s;
    if (old)
        operator delete((u32 *)old - 1);
    return 1;
}

// v12 0x00455510  (feb 0x0044CDC0)
int aiRAT::ProcessINIT(void *init)
{
    aiNPC_WR::ProcessINIT(init);
    maxHealth = 2.0f;
    health = 2.0f;
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    aiFlags2 |= 0x10002;
    wrFlags |= 2;
    navArriveDist = 0.5f;
    alertAngle = 180.0f;
    moveSpeed = 6.0f;
    turnSpeed = 70.0f;
    radCDT = 0.3f;
    heightLowTester = 0.3f;
    heightCDT = 0.6f;
    corpseTime = 1.0f;
    corpseFadeTime = 1.0f;
    shootRange = 50.0f;
    aimAngle = 20.0f;
    maxTurnStep = 5.0f;
    fallbackAngle = 10.0f;
    ratFlags = 0;
    timerA = 0;
    timerB = 0;
    sndTimer = 0;
    focusObj = objFindName(inst->root, (char *)"AttackFocus");
    if (!focusObj)
        return 0;
    rootObj = objFindName(inst->root, (char *)"ROOT");
    if (!rootObj)
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
    if (c) {
        c->minDist = 0.0f;
        c->maxDist = 1.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    struct { float size; entENTITY *owner; } data = { 0.1f, this };
    entENTITY *s = entCreate(gsScenePtr, (char *)"sfx_rat_sparks", (char *)"sfx_rat_sparks", 0, &data, 0);
    sparks = (wrsfxRAT_SPARKS *)s;
    if (!sparks)
        return 0;
    s->SetFollow(this, 0, 0);
    if (aiFlags & 0x100000)
        ratFlags |= 0x40;
    return 1;
}

// v12 0x004557D0  (feb 0x0044D060)
// 0 idle, 1 jump, 2 trot.
int aiRAT::EvalAnimGoalSeq()
{
    if (aiFlags & 0x200000) {
        ratFlags |= 1;
        return 0;
    }
    if (enemy.IsDead() || (aiFlags & 0x40000))
        return 0;
    if (aiFlags & 0x10000)
        return 2;
    if (ratFlags & 0x30)
        return 1;
    if (ratFlags & 8)
        return 1;
    if (aiFlags & 0x1000)
        return 1;
    if (aiFlags & 4)
        return 0;
    return (ratFlags & 0x40) ? 1 : 2;
}

// v12 0x00455860  (feb 0x0044D0F0)
void aiRAT::ProcessActionFrame()
{
    if (inst->IsActionFrame(1, 0))
        ratFlags |= 0x80;
    if (inst->IsActionFrame(1, 1))
        ratFlags &= ~0x80u;
}

// v12 0x004558B0  (feb 0x0044D140)
float aiRAT::GetAnimPlayRate()
{
    return (ratFlags & 1) ? 0.0f : 1.0f;
}

// v12 0x004558D0  (feb 0x0044D160)
// Once: the sound, no sparks, "sfx_rat_expl" at its back, the instance hidden; on the server the blast.
void aiRAT::ExplodeDead()
{
    if (ratFlags & 0x100)
        return;
    unk108 |= 1;
    ratFlags |= 0x120;
    PlaySounds();
    ratFlags |= 1;
    if (sparks)
        sparks->Stop();
    objOBJ *b = objFindName(inst->root, (char *)"BACK1");
    m3dV o;
    b->GetOrigin(&o);
    animCREATE_DATA cd;
    ((m3dMATR *)&cd)->_MakeLCS2WCS_VY(&o, &m3dVUnitY);
    float size = 0.3f;
    entCreate(gsScenePtr, (char *)"sfx_rat_expl", (char *)"sfx_rat_expl", &cd, &size, 0);
    inst->flags |= 1;
    inst->flags |= 0x10;
    if (!(entFlags & 4))
        return;
    m3dV p;
    if (focusObj) {
        objValidateLTM(focusObj);
        ((m3dMATR *)focusObj->ltm)->GetOrigin(&p);
    } else {
        GetPosRooted(&p);
    }
    aiDMG_DISPATCHER_TO_SIBLING disp;
    disp.amount = maxHealth * 10.0f;
    dmgDispatchExplosDamage(this, &p, 6.0f, 2.0f, 4, -1, 0, -1, &disp);
}

// v12 0x00455A50  (feb 0x0044D2D0)
void aiRAT::StartDeath(dmgDAMAGE *d)
{
    aiNPC_WR::StartDeath(d);
    ExplodeDead();
    if (entFlags & 4)
        msgSystem.PostMsg(this, 2, this);
}

// v12 0x00455A80  (feb 0x0044D300)
void aiRAT::Deactivate()
{
    ratFlags &= ~0x40u;
    aiNPC::Deactivate();
}

// v12 0x00455AA0  (feb 0x0044D320)
void aiRAT::SetMovementSpeed()
{
    int s = inst->seq;
    if (s == 0) {
        turnSpeed = 0;
        aiFlags |= 0x200;
    } else if (s == 2) {
        aiFlags &= ~0x200u;
        moveSpeed = 2.2285714f;
        turnSpeed = 70.0f;
    } else {
        aiFlags &= ~0x200u;
        moveSpeed = 9.4893618f;
        turnSpeed = (ratFlags & 0x80) ? 1.0f : 270.0f;
        if (aiFlags & 8)
            turnSpeed *= 5.0f;
    }
    if (aiFlags & 4) {
        moveSpeed = 1.0f;
        turnSpeed = 120.0f;
    }
}

// v12 0x00455B60  (feb 0x0044D3E0)
void aiRAT::Think()
{
    aiNPC_WR::Think();
    aiFlags &= ~0x1000u;
    if (enemyDist < 100.0f)
        ratFlags |= 0x40;
    if (enemyDist < 1.8f && !(aiFlags & 0x200000)) {
        static dmgDAMAGE d(0);
        d.type = 0;
        d.subtype = 0;
        d.amount = 5000.0f;
        StartDeath(&d);
    }
}

// v12 0x00455C20  (feb 0x0044D4A0)
void aiRAT::ProcessFRAME()
{
    aiNPC_WR::ProcessFRAME();
    if (!sparks || (aiFlags & 0x800000))
        return;
    if (inst->seq == 1)
        sparks->Start();
    else
        sparks->Stop();
}

// v12 0x00455C60  (feb 0x0044D4E0)
void aiRAT::MoveToGoal()
{
    if (aiFlags & 0x40000)
        aiFlags |= 0x200;
    if (inst->seq == 0)
        aiFlags |= 0x200;
    aiNPC_WR::MoveToGoal();
}

// v12 0x00455CA0  (feb 0x0044D520)
void aiRAT::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&timerA, gsElapsedTime);
    m3dUpdateTimeField(&sndTimer, gsElapsedTime);
    m3dUpdateTimeField(&timerB, gsElapsedTime);
}

// v12 0x00455CE0  (feb 0x0044D560)
void aiRAT::PlaySounds()
{
    if (unk108 & 1) {
        m3dV p;
        inst->GetPos(&p);
        gsSysSound->Play(&ratSnd[0], &p);
    } else if (unk108 & 4) {
        voice->Play(&ratSnd[2], 0);
    } else if (unk108 & 2) {
        voice->Play(&ratSnd[1], 0);
    } else if (unk108 & 8) {
        voice->Play(&ratSnd[3], 0);
    } else if (unk108 & 0x10) {
        voice->Play(&ratSnd[4], 0);
    }
    aiNPC::PlaySounds();
}

// v12 0x00455DA0  (feb 0x0044D610)
int aiRAT::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (entFlags & 4) {
        aiNPC_WR::ProcessCDT_FIREABLE(d);
        if (!(aiFlags & 0x200000)) {
            ratFlags |= 0x10;
            unk108 |= 4;
        }
    }
    return 1;
}

// v12 0x00455DF0  (feb 0x0044D660)
void aiRAT::Freeze()
{
    aiFlags |= 0x800000;
    if (sparks)
        sparks->Stop();
}

// v12 0x00455E20  (feb 0x0044D690)
void aiRAT::FillSoundFlags()
{
    if (sndTimer > 0.0f) {
        unk108 = 0;
        return;
    }
    switch (inst->seq) {
    case 0:
        unk108 |= m3dRandSelect(0x10, 2);
        sndTimer = m3dRandRange(3.0f, 7.0f);
        break;
    case 1:
        if (m3dRandom() > 0.4f) {
            unk108 |= 4;
            sndTimer = 3.0f;
        } else {
            unk108 |= 8;
            sndTimer = 1.0f;
        }
        break;
    case 2:
        unk108 |= 2;
        sndTimer = m3dRandRange(2.0f, 4.0f);
        break;
    }
}

// v12 0x00455F00  (feb 0x0044D760)
void aiRAT::AdjustSpeedsOnFallbackStart(dmgWEAPON *w, float *back, float *up)
{
    if (inst->seq == 1)
        *back *= 0.6f;
}

int aiRAT::IsGibbable() { return 0; }                    // 1.2
