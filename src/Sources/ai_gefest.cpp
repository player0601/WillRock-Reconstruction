// ai_gefest.cpp -- Hephaestus ("ai_gefest"), the boss of 4_3, and his thrown coals ("pjl_gefest") (Will Rock 1.2,
// v12.exe; feb obj ai_gefest.obj)
//
// He walks at the player and smashes the ground with his hammer (200 damage over 20 m: "sfx_gefest_hit"), or grabs
// coals from his bowl and throws them (5..7 at once, each 30 damage over 4 m, on an arc, homing a little as they
// fall: "sfx_gefest_expl" where they land). Big guns make him flinch (WOUNDED, at most once in 10 s). A "_mini" one
// is half size (x AI_PROP size), 400 hp, quicker, throws one coal (20 over 2 m), smashes for 100 over 10 m, and is
// not the level's boss.
//
// gefFlags (v12 +0x567): 8 a reaction / attack playing, 0x10 / 0x20 hit by a big gun / dead (react), 0x100 throw
// wanted, 0x200 smash wanted, 0x400 a "_mini".

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"
#include "../../include/game_shared/projectile.h"

int __fastcall wrsfxGefestInit();                                    // v12 0x00475140 (sfx_gefest.cpp)
void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int skip);  // v12 0x005130D0 (gs:gs_effect.obj)
float __fastcall entNearestPlayerDist(entENTITY *self, float yOff, m3dV *out, entENTITY **who);  // v12 0x00547450
struct wrsfxSCORCHMARK {                                // (not rebuilt: the test prints "scorchmark: not made")
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
                                                        // v12 0x00467DE0 (feb 0x0045DED0)
};

static txmTEXTURE *gefTexScorch;                        // v12 0x0064AF80 "sfx_ax_scorch_mark"
static txmTEXTURE *gefTexCarbons;                       // v12 0x0064B750 "sfx_gefest_carbons"
txmTEXTURE *gefTexCarbonExpl;                           // v12 0x0064B400 "sfx_gefest_carbon_expl" (sfx_gefest.cpp)
static txmTEXTURE *gefTexGlow;                          // v12 0x0064B74C "sfx_gefest_hit_glow"
static float gefEpsilon2, gefPjlEpsilon2;               // v12 0x0064B748 / 0x0064B758 1e-6 squared (static inits
                                                        // v12 0x004456C0 / 0x00447F80) // ? (never read)
static void *gefCfg;                                    // v12 0x0064B754 the "Debug / Decomp_OFF_AI" read's cache

// ---- the sounds ----------------------------------------------------------------------------------------------------

// v12 0x0064AF88 the boss's 11 (0x68 apart), v12 0x0064B408 a mini's 8 (static init v12 0x00444A90 / 0x00444DD0).
// 8..10 are the coals' (fly, into the level, into a body).
static sncSOUND_DESCR_3D gefSnd[11], gefMiniSnd[8];
enum { GSND_BEING_HIT, GSND_STEP_WOODEN, GSND_STEP_GOOD, GSND_SMASH, GSND_LAUGH, GSND_GRAB, GSND_THROW, GSND_DEATH,
       GSND_PJL_FLY, GSND_PJL_HIT_GROUND, GSND_PJL_HIT_BODY };

static void gefSndSet(sncSOUND_DESCR_3D *d, const char *name, int flags, float mn, float mx)
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
    d->vtbl = 0;                                        // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
}

static struct aiGEFEST_SND_INIT {
    aiGEFEST_SND_INIT()
    {
        gefSndSet(&gefSnd[0], "gefest_being_hit", 0x40000, 90.0f, 220.0f);
        gefSndSet(&gefSnd[1], "gefest_step_wooden", 0, 100.0f, 180.0f);
        gefSndSet(&gefSnd[2], "gefest_step_good", 0, 100.0f, 180.0f);
        gefSndSet(&gefSnd[3], "gefest_smash_hammer", 0, 90.0f, 250.0f);
        gefSndSet(&gefSnd[4], "gefest_laugh", 0x40000, 90.0f, 220.0f);
        gefSndSet(&gefSnd[5], "gefest_grab_carbons", 0x40000, 90.0f, 160.0f);
        gefSndSet(&gefSnd[6], "gefest_throw_carbons", 0x40000, 90.0f, 180.0f);
        gefSndSet(&gefSnd[7], "gefest_death", 0x40000, 90.0f, 250.0f);
        gefSndSet(&gefSnd[8], "gefest_pjl_fly", 0x10002, 5.0f, 30.0f);
        gefSndSet(&gefSnd[9], "gefest_pjl_hit_ground", 0x20000, 6.5f, 45.0f);
        gefSndSet(&gefSnd[10], "gefest_pjl_hit_body", 0x20000, 6.5f, 45.0f);
        gefSndSet(&gefMiniSnd[0], "gefest_being_hit", 0x40000, 25.5f, 70.0f);
        gefSndSet(&gefMiniSnd[1], "gefest_step_wooden", 0, 25.5f, 70.0f);
        gefSndSet(&gefMiniSnd[2], "gefest_step_good", 0, 25.5f, 70.0f);
        gefSndSet(&gefMiniSnd[3], "gefest_smash_hammer", 0, 30.0f, 80.0f);
        gefSndSet(&gefMiniSnd[4], "gefest_laugh", 0x40000, 25.5f, 70.0f);
        gefSndSet(&gefMiniSnd[5], "gefest_grab_carbons", 0x40000, 15.0f, 50.0f);
        gefSndSet(&gefMiniSnd[6], "gefest_throw_carbons", 0x40000, 25.5f, 70.0f);
        gefSndSet(&gefMiniSnd[7], "gefest_death", 0x40000, 30.0f, 80.0f);
        gefEpsilon2 = 0.000001f * 0.000001f;
        gefPjlEpsilon2 = 0.000001f * 0.000001f;
    }
} aiGefestSndInit;

// v12 0x004460C0
static sncSOUND_DESCR_3D *__fastcall aiGefestGetSound(int i)
{
    return &gefSnd[i];
}

static m3dSPL_LINEAR1D *gefSpl(int n)
{
    m3dSPL_LINEAR1D *s = new m3dSPL_LINEAR1D;
    if (!s)
        return 0;
    s->flags = 0;
    s->type = 0;
    s->nKp = 0;
    s->kpValueSize = 8;
    s->kpStride = 8;
    s->dim = 1;
    s->kpBytes = 0;
    s->kp = 0;
    s->pad[0] = s->pad[1] = s->pad[2] = 0;
    s->AllocKpList(n);
    return s;
}

// ---- his coals: in the hand, in the bowl, the bowl's glow ---------------------------------------------------------

class aiGEFEST_CARBONS : public partEMITTER_PHYS { public: aiGEFEST_CARBONS(float s); };
class aiGEFEST_BOWL_CARBONS : public partEMITTER_PHYS { public: aiGEFEST_BOWL_CARBONS(float s); };
class aiGEFEST_BOWL_GLOW : public partEMITTER_PHYS { public: aiGEFEST_BOWL_GLOW(float s); };

// v12 0x00446300  (feb 0x0043E920)
// The coals he grabs: 50 at once (100 a second in a 0.7 s window: bright for 0.03 .. 0.87 of it), 0.7 s each, s big
// shrinking to nothing, thrown up at s, falling at 70 x s, half the hand's speed, within 3 x s of it.
aiGEFEST_CARBONS::aiGEFEST_CARBONS(float s) : partEMITTER_PHYS(50, 100.0f, 0.7f)
{
    m3dSPL_LINEAR1D *in = gefSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 100.0f);
    in->SetKp(2, 0.8666667f, 0.0f);
    SetIntensity(0.0f, in);
    SetTimeLifePart(0.7f, 0.0f);
    m3dSPL_LINEAR1D *sz = gefSpl(2);
    sz->SetKp(0, 0.0f, s);
    sz->SetKp(1, 1.0f, s * 0.0f);
    SetSize(s, 0.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, 0);
    SetTexture(gefTexCarbons);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 150.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spreadSpl = 0;
    spread = s * 3.0f;
    spreadMin = 0;
    speedA = s;
    speedARand = 0;
    speedASpl = 0;
    speedK = 0.5f;
    speedKRand = 0;
    gravity.x = 0;
    gravity.y = s * -70.0f;
    gravity.z = 0;
    flags |= 0x409;
    flags |= 0x200000;
    flags |= 0x50;
}

// v12 0x004464D0  (feb 0x0043EAF0)
// The coals in his bowl: 30 a second, 0.3 s each, s big shrinking, leaping 10 x s (+-5 x s) up and across, falling
// at 20 x s, from all over the bowl.
aiGEFEST_BOWL_CARBONS::aiGEFEST_BOWL_CARBONS(float s) : partEMITTER_PHYS(30.0f, 0.3f)
{
    m3dSPL_LINEAR1D *sz = gefSpl(2);
    sz->SetKp(0, 0.0f, s);
    sz->SetKp(1, 1.0f, s * 0.0f);
    SetSize(s, 0.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, 0);
    SetTexture(gefTexCarbons);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    spread = s;
    spreadSpl = 0;
    spreadMin = 0;
    speedA = s * 10.0f;
    speedASpl = 0;
    speedBSpl = 0;
    gravity.x = 0;
    gravity.z = 0;
    speedARand = s * 5.0f;
    speedB = s * 10.0f;
    speedBRand = s * 5.0f;
    gravity.y = s * -20.0f;
    flags |= 0x409;
    flags |= 0xA000;
}

// v12 0x00446630  (feb 0x0043EC50)
// The bowl's glow: 15 a second, 1 s each, growing to 15 x s, fading in by 0.2 and out, seen 3..40 m.
aiGEFEST_BOWL_GLOW::aiGEFEST_BOWL_GLOW(float s) : partEMITTER_PHYS(15.0f, 1.0f)
{
    m3dSPL_LINEAR1D *sz = gefSpl(2);
    sz->SetKp(0, 0.0f, s * 0.0f);
    sz->SetKp(1, 1.0f, s * 15.0f);
    SetSize(s * 15.0f, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = gefSpl(3);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.2f, 255.0f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(gefTexGlow);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    flags |= 0x409;
    flags |= 0x800000;
    SetDegNearParams(3.0f, 0.3f);
    SetDegFarParams(20.0f, 40.0f, 1.0f, 0.3f);
}

// ---- the boss ------------------------------------------------------------------------------------------------------

// v12 0x00445180  (feb 0x0043D880)
entENTITY *__fastcall aiGEFEST_GENERIC::Create(animINST *)
{
    aiGEFEST_GENERIC *g = new aiGEFEST_GENERIC;
    if (!g)
        return 0;
    g->wrFlags = 0;
    g->bloodObj = 0;
    g->carbons = 0;
    g->carbonsObj = 0;
    g->bowlCarbons = 0;
    g->bowlGlow = 0;
    g->bowlObj = 0;
    g->unk98 = 0x12;
    return g;
}

// v12 0x004451F0  (feb 0x0043D8F0)
// His 8 sequences; WOUNDED and DEATH from anything, DEATH held, WALK into THROW / SMASH / SHORTSMASH.
BOOL32 aiGEFEST_GENERIC::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 8;
    u32 *blk = (u32 *)operator new(0x444);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 8;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 8; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    static const char *names[8] = { "GEFEST_SEQ_IDLE", "GEFEST_SEQ_WALK", "GEFEST_SEQ_SMASH", "GEFEST_SEQ_THROW",
                                    "GEFEST_SEQ_ROAR", "GEFEST_SEQ_WOUNDED", "GEFEST_SEQ_DEATH",
                                    "GEFEST_SEQ_SHORTSMASH" };
    for (int i = 0; i < 8; i++)
        _entAnimSetSeqName(t, i, (char *)names[i]);
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(8, 0);
    if (!s) {
        if (t->seq)
            operator delete((u32 *)t->seq - 1);
        return 0;
    }
    t->aniSystem = s;
    s->SetTranParamFromAny(5, 0, 1, 5);
    s->SetTranCycleAnim(5);
    s->SetTranParamFromAny(6, 0, 1, 6);
    s->SetTranParam(6, 6, 2, 6, 1);
    s->SetTranParam(1, 3, 1, 3, 0);
    s->SetTranParam(1, 2, 1, 2, 0);
    s->SetTranParam(1, 7, 1, 7, 0);
    if (old)
        operator delete((u32 *)old - 1);
    t->seq[1].unk84 |= 2;
    t->seq[6].unk84 |= 1;
    return 1;
}

// v12 0x00445400  (feb 0x0043DB00)
// 5000 hp, slow (5.1), turning 90 a second, sees 300 m all round, throws within 50 m; his voice a 360 m sphere;
// "AttackFocus", "RotationPivot", "ROOT" (blood from the root); the coals started.
int aiGEFEST_GENERIC::ProcessINIT(void *init)
{
    aiNPC_WR::ProcessINIT(init);
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    radCDT = 4.8f;
    heightLowTester = 4.8f;
    maxHealth = 5000.0f;
    navArriveDist = 1.8f;
    new1B7 = 0.6f;
    alertAngle = 180.0f;
    moveSpeed = 5.1072f;
    turnSpeed = 90.0f;
    f_593 = 5.0f;
    shootRange = 50.0f;
    aimAngle = 20.0f;
    timeDeactivate = 3600.0f;
    heightCDT = 8.5f;
    heightUpTester = 3.0f;
    heightUp1 = 9.0f;
    navAnalyzeDist = 20.0f;
    size = 1.0f;
    health = 5000.0f;
    gefFlags = 0;
    timer56F = 0;
    f_57B = 0;
    reactSeq = 5;
    attackSeq = 7;
    aiFlags2 |= 0x38042;
    navFlags |= 4;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;          // (a sncCHANNEL_SPH there: a sphere of 360)
    if (c) {
        c->minDist = 0.0f;
        c->maxDist = 1.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    if (!(attackFocus = objFindName(inst->root, (char *)"AttackFocus")))
        return 0;
    if (!(pivotObj = objFindName(inst->root, (char *)"RotationPivot")))
        return 0;
    if (!(rootObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    gsScenePtr->SetStateYesInst(inst, 8);
    inst->root->SetStateYes(0x20);
    StartSFX();
    return 1;
}

// v12 0x004456E0  (feb 0x0043DDC0)
// "_mini": half size (x AI_PROP size), 400 hp, quicker (x 1.8), its corpse gone in 10 s; else the boss: never
// decaying, the level's boss. (1.2: AI_PROP size; the boss is set here, not in Activate.)
void aiGEFEST_GENERIC::ParseNameClass()
{
    const char *cls = inst->className ? inst->className : "";
    float sz = 1.0f;
    aiNPC_WR::ParseNameClass();
    if (strstr(cls, "_mini")) {
        gefFlags |= 0x400;
        radCDT = 3.0f;
        heightLowTester = 3.0f;
        maxHealth = 400.0f;
        health = 400.0f;
        heightCDT = 5.0f;
        heightUpTester = 2.5f;
        heightUp1 = 5.0f;
        size = 0.5f;
        inst->Scale(0.5f, 0.5f, 0.5f, 2);
        if (GetFloat((char *)"AI_PROP", (char *)"size", &sz))
            size *= sz;
        corpseTime = 10.0f;
        corpseFadeTime = 9.0f;
        fallbackAngle = 400.0f;
        moveSpeed = moveSpeed * size * 1.8f;
        radCDT *= sz;
        heightLowTester *= sz;
        heightCDT *= sz;
        heightUpTester *= sz;
        heightUp1 *= sz;
    } else {
        aiFlags2 |= 0x40000;
        corpseTime = 1000.0f;
        corpseFadeTime = 999.0f;
        fallbackAngle = 10000.0f;
    }
    ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, EvalAnimGoalSeq(), 0.0f, 0);
    if (!(gefFlags & 0x400))
        aiAttackPlanner->SetBoss(this);
}

// v12 0x004458B0  (feb 0x0043DF10)
// The coals gone, the boss let go. (1.2: the base TERM not called)
void aiGEFEST_GENERIC::ProcessTERM()
{
    if (carbons) {
        carbons->__vecDelDtor(1);
        carbons = 0;
    }
    if (bowlCarbons) {
        bowlCarbons->__vecDelDtor(1);
        bowlCarbons = 0;
    }
    if (bowlGlow) {
        bowlGlow->__vecDelDtor(1);
        bowlGlow = 0;
    }
    if (!(gefFlags & 0x400))
        aiAttackPlanner->SetBoss(0);
}

// v12 0x00445920  (feb 0x0043DF70) (1.2: only the base's: the boss is set by ParseNameClass)
int aiGEFEST_GENERIC::Activate(int a)
{
    return aiNPC::Activate(a);
}

// v12 0x00445930  (feb 0x0043DFA0)
// Dead: DEATH; not active: IDLE; the enemy dead: IDLE (ROAR one time in 5); a big gun's hit: WOUNDED (WALK while
// the last WOUNDED is less than 10 s old); an attack wanted: THROW, or the smash (the boss's SHORTSMASH, a mini's
// SMASH); else WALK.
int aiGEFEST_GENERIC::EvalAnimGoalSeq()
{
    if (aiFlags & 0x200000)
        return 6;
    if (aiFlags & 0x10000)
        return 0;
    if (enemy.IsDead())
        return m3dRandom() < 0.8f ? 0 : 4;
    u32 f = gefFlags;
    if (f & 0x30) {
        int s;
        if (aiFlags & 0x200000)
            s = 6;
        else
            s = shootFallbackTimer > 0.000001f ? 1 : 5;
        f &= ~0x30u;
        gefFlags = f;
        if (s != 1)
            gefFlags = f | 8;
        reactSeq = s;
        reactAlive = (~aiFlags >> 21) & 1;
        return s;
    }
    if (f & 8)
        return reactSeq;
    if (aiFlags & 0x1000) {
        if (attackSeq < 0) {
            int s = (f & 0x200) ? ((f & 0x400) ? 2 : 7) : 3;
            gefFlags = f & ~0x300u;
            aiFlags |= 0x1000;
            attackSeq = s;
        }
        return attackSeq;
    }
    if (f & 0x300) {
        int s = (f & 0x200) ? ((f & 0x400) ? 2 : 7) : 3;
        gefFlags = f & ~0x300u;
        aiFlags |= 0x1000;
        attackSeq = s;
        return s;
    }
    return 1;
}

// v12 0x00445A90  (feb 0x0043E120)
// The chance of a throw: none unless the enemy is seen within 20 degrees; certain (from 40 m) when it stands in
// view within 10 degrees; else growing from 14 m to 35 m, 0.9 to 300 m, less beyond (all over his size).
float aiGEFEST_GENERIC::CalcBorderForThrowAttack()
{
    u32 f = aiFlags;
    if (!(f & 0x10000000))
        return -1.0f;
    if (!(enemyAngle <= 20.0f))
        return -1.0f;
    float d = enemyDist / size;
    if ((f & 1) && (f & 2) && enemyAngle < 10.0f && !(d <= 40.0f))
        return 1.0f;
    if (d < 35.0f)
        return (d - 14.0f) * 0.042857140f;
    if (!(d <= 300.0f))
        return 0.9f - (d - 300.0f) * 0.015f;
    return 0.9f;
}

// v12 0x00445B60  (feb 0x0043E1F0)
// Between attacks (not smashing or throwing), the enemy seen within 50 degrees: the chances of a smash (close; out
// of his nav area only from 0.1 to 40 m) and of a throw; each drawn. A mini whose ban area holds him does neither.
// (1.2: the enemy must be in sight.)
void aiGEFEST_GENERIC::FillAttackFlag()
{
    int seq = inst->seq;
    if (seq == 2 || seq == 7 || seq == 3)
        return;
    if (aiFlags & 0x1000)
        return;
    if (!(attackDelay <= 0.01f))
        return;
    gefFlags &= ~0x300u;
    if (aiFlags & 0x10)
        return;
    if (!(enemyAngle <= 50.0f))
        return;
    if (!(aiFlags & 0x10000000))
        return;
    if ((gefFlags & 0x400) && (wrFlags & 0x200))
        return;
    float d = enemyDist / size;
    float smash;
    if ((aiFlags2 & 8) && !((scnDOMAIN *)navArea)->IsBelongPointXZ(&wayPointPos)) {
        smash = (0.1f < d && d < 40.0f) ? 0.8f : -1.0f;
    } else if (d < 0.1f) {
        smash = (d - 0.04f) * 11.666667f;
    } else if (!(d <= 30.0f)) {
        smash = 0.7f - (d - 30.0f) * 0.11666667f;
    } else {
        smash = 0.7f;
    }
    smashChance = smash;
    throwChance = CalcBorderForThrowAttack();
    if (m3dRandom() < smashChance)
        gefFlags |= 0x200;
    if (m3dRandom() < throwChance)
        gefFlags |= 0x100;
}

// v12 0x00445D20  (feb 0x0043E3A0)
// Turning only while he walks (and can pivot): 90 a second, 108 with the enemy more than 20 degrees off.
void aiGEFEST_GENERIC::SetMovementSpeed()
{
    if (inst->seq != 1) {
        aiFlags |= 0x200;
        turnSpeed = 0;
        return;
    }
    if (!IsPivotUsable()) {
        turnSpeed = 0;
        return;
    }
    turnSpeed = 90.0f;
    if (!(enemyAngle <= 20.0f))
        turnSpeed = 108.00001f;
}

// v12 0x00445D90  (feb 0x0043E410)
// The attack chances; a boss's WOUNDED makes the next one wait 10 s.
void aiGEFEST_GENERIC::Think()
{
    aiNPC_WR::Think();
    FillAttackFlag();
    if (inst->seq == 5 && !(gefFlags & 0x400))
        shootFallbackTimer = 10.0f;
    UpdateInAreaFlags();
}

// v12 0x00445DD0  (feb 0x0043E440)
int aiGEFEST_GENERIC::SelectGoal()
{
    if (SelectEscapeWhenOut())
        return 1;
    return aiNPC::SelectGoal();
}

// v12 0x00445DF0  (feb 0x0043E460)
void aiGEFEST_GENERIC::ProcessFRAME()
{
    aiNPC_WR::ProcessFRAME();
    if (!(aiFlags & 0x800000))
        Slot33();
}

// v12 0x004440B0 (shared with aiCYCLOP: one function in 1.2)
void aiGEFEST_GENERIC::ClampGoalFrame(float speed, m3dV *dir, m3dV *goal)
{
    ((aiCYCLOP *)(aiNPC_WR *)this)->aiCYCLOP::ClampGoalFrame(speed, dir, goal);
}

// v12 0x00445E20  (feb 0x0043E4A0)
// A big gun (6, 9, 10) makes him react; and when it kills him.
int aiGEFEST_GENERIC::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (entFlags & 4) {
        if (d->type == 1) {
            int w = ((dmgWEAPON *)d)->weaponType;
            if (w == 6 || (w > 8 && w <= 10))
                gefFlags |= 0x20;
        }
        aiNPC_WR::ProcessCDT_FIREABLE(d);
        if (aiFlags & 0x200000)
            gefFlags |= 0x20;
    }
    return 1;
}

// v12 0x00445E90  (feb 0x0043E510)
// One voice a frame (death, smash, grab, hit, throw, laugh), the steps where he stands.
void aiGEFEST_GENERIC::PlaySounds()
{
    m3dV p;
    u32 f = unk108;
    int n = -1;
    if (f & 0x10)
        n = GSND_DEATH;
    else if (f & 2)
        n = GSND_SMASH;
    else if (f & 4)
        n = GSND_GRAB;
    else if (f & 1)
        n = GSND_BEING_HIT;
    else if (f & 8)
        n = GSND_THROW;
    else if (f & 0x80)
        n = GSND_LAUGH;
    if (n >= 0)
        voice->Play(GetSound(n), 0);
    if (unk108 & 0x20) {
        inst->GetPos(&p);
        gsSysSound->Play(GetSound(GSND_STEP_WOODEN), &p);
    }
    if (unk108 & 0x40) {
        inst->GetPos(&p);
        gsSysSound->Play(GetSound(GSND_STEP_GOOD), &p);
    }
    aiNPC::PlaySounds();
}

// v12 0x00445F70  (feb 0x0043E620)
// The steps (WALK's two action frames), the laugh (ROAR), the grab and the throw, the smash; a new WOUNDED: hit,
// a new DEATH: death.
void aiGEFEST_GENERIC::FillSoundFlags()
{
    if (inst->IsActionFrame(1, 0))
        unk108 |= 0x20;
    if (inst->IsActionFrame(1, 1))
        unk108 |= 0x40;
    if (inst->IsActionFrame(4, 0))
        unk108 |= 0x80;
    if (inst->IsActionFrame(3, 0))
        unk108 |= 4;
    if (inst->IsActionFrame(3, 1))
        unk108 |= 8;
    if (inst->IsActionFrame(2, 1) || inst->IsActionFrame(7, 1))
        unk108 |= 2;
    if (inst->flags & 0x4000) {
        if (inst->seq == 5)
            unk108 |= 1;
        else if (inst->seq == 6)
            unk108 |= 0x10;
    }
}

// v12 0x00446090
// His sound i: a mini's or the boss's.
sncSOUND_DESCR_3D *aiGEFEST_GENERIC::GetSound(int i)
{
    return (gefFlags & 0x400) ? &gefMiniSnd[i] : &gefSnd[i];
}

// v12 0x004460D0  (feb 0x0043E6C0)
// The smash lands (SMASH / SHORTSMASH's first action frame): 200 over 20 m round "AttackFocus" (a mini 100 over
// 10), a scorch mark, "sfx_gefest_hit" (0.4); THROW: the coals in his hand begin, then go (a mini one, the boss
// 5..7); ROAR / WOUNDED done: free again.
void aiGEFEST_GENERIC::ProcessActionFrame()
{
    if (inst->IsActionFrame(2, 0) || inst->IsActionFrame(7, 0)) {
        animCREATE_DATA cd;
        m3dV p;
        ((m3dMATR *)attackFocus->ltm)->GetOrigin(&p);
        float dmg, rad;
        if (gefFlags & 0x400) {
            dmg = 100.0f;
            rad = 10.0f;
        } else {
            dmg = 200.0f;
            rad = 20.0f;
        }
        dmgDispatchExplosDamage(this, &p, dmg, rad, 4, -1, 0, 0xFFFFFFFF, 0);
        wrsfxSCORCHMARK::Add(2.5f, 10.0f, gefTexScorch, &p, 0, 0x5343524D, 0, 0);     // 'SCRM'
        cd.matr._MakeLCS2WCS_VY(&p, &m3dVUnitY);
        float sc = 0.4f;
        entCreate(gsScenePtr, (char *)"sfx_gefest_hit", (char *)"sfx_gefest_hit", &cd, &sc, 0);
        aiFlags &= ~0x1000u;
    }
    if (inst->IsActionFrame(3, 0)) {
        carbons->StopEmission();
        carbons->StartEmission();
    }
    if (inst->IsActionFrame(3, 1)) {
        if (gefFlags & 0x400)
            LaunchOneProjectile(attackFocus);
        else
            LaunchSeveralProjectiles(attackFocus);
        aiFlags &= ~0x1000u;
    }
    if (inst->IsActionFrame(4, 1))
        gefFlags &= ~8u;
    inst->IsActionFrame(5, 1);                          // (1.2 asks and does nothing with it)
    if (inst->IsActionFrame(5, 0))
        gefFlags &= ~8u;
}

// v12 0x004462B0  (feb 0x0043E8F0)
// A mini walks, smashes 1.8 times as fast, throws and reacts 1.4 times.
float aiGEFEST_GENERIC::GetAnimPlayRate()
{
    if (!(gefFlags & 0x400))
        return 1.0f;
    switch (inst->seq) {
    case 1:
    case 2:
        return 1.8f;
    case 3:
    case 5:
        return 1.4f;
    }
    return 1.0f;
}

// v12 0x004467F0  (feb 0x0043EDE0) the hammer and the bowl take no hits
int aiGEFEST_GENERIC::IsArmoredObject(char *name)
{
    if (!_strnicmp(name, "molot", 5) || !_strnicmp(name, "obj2", 4) || !_strnicmp(name, "gorshok_3", 8))
        return 1;
    return 0;
}

// v12 0x00446840  (feb 0x0043EE30)
void aiGEFEST_GENERIC::Explode(dmgDAMAGE *d, int dead)
{
    if (bowlGlow)
        bowlGlow->StopEmission();
    if (bowlCarbons)
        bowlCarbons->StopEmission();
    if (carbons)
        carbons->StopEmission();
    aiNPC_WR::Explode(d, dead);
}

// v12 0x00446890  (feb 0x0043EE80)
void aiGEFEST_GENERIC::Freeze()
{
    aiFlags |= 0x800000;
    if (bowlGlow)
        bowlGlow->StopEmission();
    if (bowlCarbons)
        bowlCarbons->StopEmission();
    if (carbons)
        carbons->StopEmission();
}

// v12 0x004468E0  (feb 0x0043EED0)
// The coals for his hand ("carbons_source", started by THROW), the bowl's coals over "bowl_source" (hidden) and its
// glow, both burning.
int aiGEFEST_GENERIC::StartSFX()
{
    if (!(carbons = new aiGEFEST_CARBONS(0.35f)))
        return 0;
    if (!(carbonsObj = objFindName(inst->root, (char *)"carbons_source")))
        return 0;
    if (!(bowlCarbons = new aiGEFEST_BOWL_CARBONS(0.2f)))
        return 0;
    if (!(bowlObj = objFindName(inst->root, (char *)"bowl_source")))
        return 0;
    bowlObj->SetStateProcYes(8, 0);
    if (!partSetAreaEmissionObj(bowlCarbons, bowlObj, -1, 0))
        return 0;
    if (!(bowlGlow = new aiGEFEST_BOWL_GLOW(0.2f)))
        return 0;
    bowlGlow->SetCS((m3dMATR *)bowlObj->ltm);
    bowlCarbons->StartEmission();
    bowlGlow->StartEmission();
    return 1;
}

// v12 0x00446A00  (feb 0x0043EFF0) UpdateSFX
// The hand's coals follow "carbons_source" while they burn; the bowl's follow the bowl.
void aiGEFEST_GENERIC::Slot33()
{
    inst->Validate(4);
    if (carbons && ((carbons->state & 1) || carbons->nPart)) {
        m3dV p;
        carbonsObj->GetOrigin(&p);
        carbons->SetPosDirSpeed(&p, 0, 0);
    }
    partSetAreaEmissionObj(bowlCarbons, bowlObj, -1, 0);
    if (bowlGlow)
        bowlGlow->SetCS((m3dMATR *)bowlObj->ltm);
}

// v12 0x00446B30  (feb 0x0043F080)
int aiGEFEST_GENERIC::IsAnimSeqGoodForFallback()
{
    return EvalAnimGoalSeq() != 6;
}

// v12 0x00456F70  (feb 0x0044E610)
void aiGEFEST_GENERIC::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&timer56F, gsElapsedTime);
    m3dUpdateTimeField(&attackDelay, gsElapsedTime);
}

// v12 0x0045CD90 / 0x0045A950 (shared)
int aiGEFEST_GENERIC::GetLayer3State()
{
    return (int)gefFlags;
}
void aiGEFEST_GENERIC::SetLayer3State(int s)
{
    gefFlags = (u32)s;
}

// ---- his coals, thrown --------------------------------------------------------------------------------------------

#pragma pack(push, 1)
struct pjlGEFEST_INIT : pjlPJL_INIT {                   // 0x71 bytes
    u32   extra;                                        // +0x51 0
    float scale;                                        // +0x55 the coal's size (1)
    m3dV  from;                                         // +0x59 where it leaves his hand
    m3dV  dir;                                          // +0x65 its first direction
};
#pragma pack(pop)

class pjlGEFEST_SFX_TRAIL : public partEMITTER_PHYS { public: pjlGEFEST_SFX_TRAIL(float s); };

// v12 0x00446F60  (feb 0x0043F310)
// A coal's sparks: 30 a second (bright for its first 0.03 s), 0.3 s each, half its size shrinking to nothing, from
// within 0.4 x s of it, 0.3 of its speed, seen from 3 m.
pjlGEFEST_SFX_TRAIL::pjlGEFEST_SFX_TRAIL(float s) : partEMITTER_PHYS(30.0f, 0.3f)
{
    m3dSPL_LINEAR1D *in = gefSpl(2);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 30.0f);
    SetIntensity(30.0f, in);
    m3dSPL_LINEAR1D *sz = gefSpl(2);
    sz->SetKp(0, 0.0f, s * 0.5f);
    sz->SetKp(1, 1.0f, s * 0.0f);
    SetSize(s * 0.5f, 0.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, 0);
    SetTexture(gefTexCarbons);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 150.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spreadSpl = 0;
    spread = s * 0.4f;
    spreadMin = 0;
    speedK = 0.3f;
    speedKRand = 0;
    flags |= 0x409;
    flags |= 0x10;
    flags |= 0x200;
    SetDegNearParams(3.0f, 0.3f);
}

struct pjlGEFEST_SFX {                                  // 8 bytes, at pjlPJL +0x16F
    partEMITTER_PHYS *emitter;                          // pjlGEFEST_SFX_TRAIL
    objOBJ           *emitterObj;                       // "emitter*"
    pjlGEFEST_SFX() : emitter(0), emitterObj(0) {}

    // v12 0x00447100  (feb 0x0043F4B0)
    // The coal at its size, its sparks from "emitter*", its animation from a random point (0..1.5 s).
    int StartSFX(physPHYS *p, pjlPJL_INIT *init)
    {
        float s = ((pjlGEFEST_INIT *)init)->scale;
        animINST *a = p->inst;
        a->Scale(s, s, s, 0);
        emitter = new pjlGEFEST_SFX_TRAIL(s);
        if (!emitter)
            return 0;
        if (!(emitterObj = objFindNameN(a->root, (char *)"emitter", (int)strlen("emitter"))))
            return 0;
        a->SetAnimSeq(0, m3dRandom() * 1.5f);
        if (emitter) {
            animINST *pi = p->inst;
            pi->AdvanceTime(gsElapsedTime, 0, 0);
            pi->Validate(4);
            m3dV pos;
            emitterObj->GetOrigin(&pos);
            emitter->SetPosDirSpeed(&pos, 0, (m3dV *)p->velocity);
        }
        emitter->StartEmission();
        return 1;
    }
    // v12 0x00447210  (feb 0x0043F5C0)
    void UpdateSFX(physPHYS *p)
    {
        if (!emitter)
            return;
        animINST *a = p->inst;
        a->AdvanceTime(gsElapsedTime, 0, 0);
        a->Validate(4);
        m3dV pos;
        emitterObj->GetOrigin(&pos);
        emitter->SetPosDirSpeed(&pos, 0, (m3dV *)p->velocity);
    }
    // v12 0x00447270  (feb 0x0044F180)
    void StopSFX(physPHYS *)
    {
        if (emitter) {
            emitter->StopEmission();
            emitter = 0;
        }
    }
};

#pragma pack(push, 1)
class pjlGEFEST : public pjlNPC_PJL<pjlGEFEST_SFX> {
public:
    u32 layerState;                                     // +0x1BC 0x100 hit the level, 0x200 something else
    static entENTITY *__fastcall Create(animINST *i);
    BOOL32 ProcessINIT(physINIT *p);
    void   ProcessFRAME();
    void   ShowCollisSFX(pjlEV_CDT *ev);
    sncSOUND_DESCR_3D *GetSound(int i);
    int    GetLayerAppState() { return (int)layerState; }        // v12 0x0043DA40
    void   SetLayerAppState(int s) { layerState = (u32)s; }       // v12 0x0043DA50
};
#pragma pack(pop)

// v12 0x00446B50  (feb 0x0043F0A0)
entENTITY *__fastcall pjlGEFEST::Create(animINST *)
{
    pjlGEFEST *g = new pjlGEFEST;
    if (!g)
        return 0;
    g->layerState = 0;
    return g;
}

// v12 0x00447290  (feb 0x0043F620) with pjlPJL<pjlGEFEST_SFX>::ProcessINIT v12 0x00446C50 (feb 0x0043F1A0)
// As the egg: the coal, its sound channel (a sphere), and one born inside a wall (the line from his chest or head
// blocked) hits at once; it keeps its speed's direction (physMATH flag 2).
BOOL32 pjlGEFEST::ProcessINIT(physINIT *p)
{
    if (!physPHYS::ProcessINIT(p))
        return 0;
    pjlPJL_INIT *in = (pjlPJL_INIT *)p;
    damage = in->damage;
    radius = in->radius;
    isGeneral = in->isGeneral;
    weaponType = in->weaponType;
    termDelay = in->termDelay;
    explosive = in->explosive;
    if (entFlags & 4) {
        if (in->ownerId != 0xFFFFFFFF) {
            entIACTIVE_OBJ *e = gsSysMP->FindMPID((int)in->ownerId, 4);
            if (e)
                AddEntCDTExclude((entENTITY *)e);
        }
        ownerId = in->ownerId;
    }
    sfx.StartSFX(this, in);
    if (HasSound()) {
        float mn, mx;
        GetSndDist(&mn, &mx);
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        if (c) {
            c->minDist = mn;
            c->maxDist = mx;
        }
        sndChannel = c;
        if (!sndChannel)
            return 0;
        gsSysSound->AddChannel(sndChannel, this, 0);
    }
    pjlFlags |= 0x12;
    if ((entFlags & 4) && in->ownerId != 0xFFFFFFFF) {
        entENTITY *owner = (entENTITY *)gsSysMP->FindMPID((int)in->ownerId, 4);
        if (owner && owner->unk98 != 0x14) {
            cdtREFINE ref(2, owner->inst);
            cdtINFO ci;
            memset(&ci, 0, sizeof(ci));
            m3dV op, ap;
            owner->inst->GetPos(&op);
            inst->GetPos(&ap);
            m3dV chest = op;
            chest.y = op.y + 0.3f;
            m3dV head = op;
            head.y = ((aiNPC *)owner)->Slot90() + op.y;
            if (!gsScenePtr->IsVisiblePoint(&chest, &ap, 1, &ref, &ci) ||
                !gsScenePtr->IsVisiblePoint(&head, &ap, 1, &ref, &ci)) {
                pjlHIT_MSG m;
                m.packId = 0;
                m.tag = 0x40;
                m.size = 0x24;
                entENTITY *he = ci.inst ? ci.inst->ent : 0;
                m.mpId = he ? (u32)he->mpId : 0xFFFFFFFF;
                m.ci0 = ci.flags;
                m.ci8 = (u32)ci.objId;
                m.ciC = (u32)ci.faceId;
                m.point = *(m3dV *)ci.point;
                memcpy(&m.ci1C, &ci.dist, 4);
                msgSystem.PostMsgData(this, 0x4D3, &m, this);
            }
        }
    }
    math->flags |= 2;
    return 1;
}

// v12 0x004474A0  (feb 0x0043F8C0)
// ([Debug] Decomp_OFF_AI stops it.) Flies with its sparks and sounds; the layer state to the clients. On the
// server, falling: it turns towards the nearest player (its direction + 2.5 x the way to him), keeping its speed.
void pjlGEFEST::ProcessFRAME()
{
    int off;
    if (_apCfgReadBool(&gefCfg, (char *)"Debug", (char *)"Decomp_OFF_AI", &off) && off)
        return;
    pjlEV_LAYER ev;
    ev.type = 7;
    ev.size = 0xA;
    ev.state = 0;
    if (!((gsSysCinema->state & 1) && gsSysCinema->IsSuspendAction())) {
        physPHYS::ProcessFRAME();
        sfx.UpdateSFX(this);
        PlaySounds();
    }
    if (GetLayerAppState() != lastLayerState) {
        ev.packId = 0;
        ev.tag = 0x40;
        ev.state = 0;
        ev.type = 7;
        ev.size = 0xA;
        ev.state = GetLayerAppState();
        gsSysServer->SendMsgClientAll(this, 0x40E, &ev, 0x10000, 0xFFFFFFFD, 0xFFFFFFFD);
        lastLayerState = ev.state;
    }
    if (!(entFlags & 4))
        return;
    m3dV v = *(m3dV *)math->vel;
    if (!(v.y < 0.0f))
        return;
    m3dV p;
    ((m3dMATR *)math->matr)->GetOrigin(&p);             // (physMATH::GetPos)
    v = *(m3dV *)math->vel;
    float speed = m3dSqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    m3dV at;
    entNearestPlayerDist(this, 0.5f, &at, 0);
    m3dV to = { at.x - p.x, at.y - p.y, at.z - p.z };
    m3dNormalize(&to);
    v.x += to.x * 2.5f;
    v.y += to.y * 2.5f;
    v.z += to.z * 2.5f;
    m3dNormalize(&v);
    math->vel[0] = v.x * speed;
    math->vel[1] = v.y * speed;
    math->vel[2] = v.z * speed;
}

// v12 0x00447770  (feb 0x0043FC40)
// Where it hits: "sfx_gefest_expl" standing on the surface (on a floor-ish face, 0.7 .. 1 up, its normal; else
// upright); the layer state says what it hit.
void pjlGEFEST::ShowCollisSFX(pjlEV_CDT *ev)
{
    animCREATE_DATA cd;
    if (ev->kind == 1)
        layerState |= 0x100;
    else
        layerState |= 0x200;
    float c = fabsf(m3dAngleVector_COS(&ev->normal, &m3dVUnitY));
    m3dV *up = (0.7f < c && c <= 1.0f) ? &ev->normal : &m3dVUnitY;
    cd.matr._MakeLCS2WCS_VY(&ev->pos, up);
    float sc = 1.0f;
    entCreate(gsScenePtr, (char *)"sfx_gefest_expl", (char *)"sfx_gefest_expl", &cd, &sc, 0);
}

// v12 0x00447FA0
// 0 flying, 1 into the level, 2 into a body: the boss's 8..10.
sncSOUND_DESCR_3D *pjlGEFEST::GetSound(int i)
{
    if (i < 0 || i > 2)
        return 0;
    return aiGefestGetSound(i + 8);
}

// v12 0x00447840  (feb 0x0043FD10)
// The throw from `from`: at 15 m/s flat towards where he meets his enemy (1.5 up) when that is within aimAngle of
// his facing, else ahead (the enemy's own position for the height); straight ahead beyond shootRange; 0.4 higher
// still; up so as to come down there under gravity 15. Its first direction kept in the init.
void aiGEFEST_GENERIC::AimPjlToPlayer(m3dV *from, pjlPJL_INIT *init)
{
    pjlGEFEST_INIT *in = (pjlGEFEST_INIT *)init;
    m3dV dir, target, v;
    inst->GetDir(&dir);
    if (!(wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(this, 15.0f, &target)) {
        target.y += 1.5f;
        v.x = target.x - from->x;
        v.y = 0;
        v.z = target.z - from->z;
        m3dNormalize(&v);
        float a = fabsf(m3xzAngleVector(&v, &dir));
        if (!(a <= aimAngle))
            v = dir;
    } else {
        enemy.GetPos(&target);
        v = dir;
    }
    target.y += 0.4f;
    float dx = from->x - target.x, dz = from->z - target.z;
    float d = m3dSqrt(dx * dx + dz * dz);
    if (!(d <= shootRange))
        v = dir;
    float t = d * 0.06666667f;                          // the flight's time: 1/15 a metre
    in->phys.vel[0] = v.x * 15.0f;
    in->phys.vel[1] = v.y * 15.0f;
    in->phys.vel[2] = v.z * 15.0f;
    in->phys.vel[1] = ((target.y - from->y) - t * t * -7.5f) / t;
    in->phys.accel[0] = m3dVUnitY.x * -15.0f;
    in->phys.accel[1] = m3dVUnitY.y * -15.0f;
    in->phys.accel[2] = m3dVUnitY.z * -15.0f;
    in->dir = *(m3dV *)in->phys.vel;
    m3dNormalize(&in->dir);
}

// the init of a coal: no speed yet, 1 hit, explosive, size 1
static void gefPjlInit(pjlGEFEST_INIT *in)
{
    memset(in, 0, sizeof(*in));
    in->phys.packId = 0;
    in->phys.tag = 0x40;
    in->phys.size = 0x71;
    in->phys.vel[0] = m3dVZero.x;
    in->phys.vel[1] = m3dVZero.y;
    in->phys.vel[2] = m3dVZero.z;
    in->phys.accel[0] = m3dVZero.x;
    in->phys.accel[1] = m3dVZero.y;
    in->phys.accel[2] = m3dVZero.z;
    in->phys.radius = 0.1f;
    in->phys.lifetime = 2000000.0f;
    in->phys.maxHits = 1;
    in->damage = -1.0f;
    in->radius = 0;
    in->weaponType = -1;
    in->isGeneral = -1;
    in->ownerId = 0xFFFFFFFF;
    in->termDelay = -1.0f;
    in->explosive = 1;
    in->extra = 0;
    in->scale = 1.0f;
}

// the coal's settings for this thrower: 30 over 4 m (a mini 20 over 2), general 4, a 0.8 sphere, 1000 s, 5
// bounces at half the speed
static void gefPjlArm(aiGEFEST_GENERIC *g, pjlGEFEST_INIT *in)
{
    if (g->gefFlags & 0x400) {
        in->damage = 20.0f;
        in->radius = 2.0f;
    } else {
        in->damage = 30.0f;
        in->radius = 4.0f;
    }
    in->ownerId = (u32)g->mpId;
    in->isGeneral = 4;
    in->phys.radius = 0.8f;
    in->phys.lifetime = 1000.0f;
    in->phys.maxHits = 5;
    in->phys.friction = 0.5f;
    in->phys.bounce = 0.5f;
}

// v12 0x00447AD0  (feb 0x0043FFA0)
// One coal from `from` (a mini's), aimed; lit as the floor under him.
void aiGEFEST_GENERIC::LaunchOneProjectile(objOBJ *from)
{
    pjlGEFEST_INIT in;
    gefPjlInit(&in);
    animCREATE_DATA cd;
    inst->Validate(4);
    from->GetOrigin(&in.from);
    memcpy(&cd.matr, from->ltm, sizeof(cd.matr));
    gefPjlArm(this, &in);
    AimPjlToPlayer(&in.from, &in);
    entENTITY *e = entCreate(gsScenePtr, (char *)"pjl_gefest", (char *)"pjl_gefest", &cd, &in, 0);
    if (e)
        gsEffApplyLMChar(e, &floor, 0);
}

// v12 0x00447C70  (feb 0x00440170)
// The boss's handful: 5..7 coals, each aimed as one and turned up to 8 degrees off at random, at its speed.
void aiGEFEST_GENERIC::LaunchSeveralProjectiles(objOBJ *from)
{
    pjlGEFEST_INIT in, one;
    gefPjlInit(&in);
    gefPjlInit(&one);
    animCREATE_DATA cd;
    inst->Validate(4);
    from->GetOrigin(&in.from);
    memcpy(&cd.matr, from->ltm, sizeof(cd.matr));
    gefPjlArm(this, &in);
    AimPjlToPlayer(&in.from, &in);
    int n = m3dRandIntRange(5, 7);
    for (int i = 0; i < n; i++) {
        one = in;
        m3dV d = *(m3dV *)one.phys.vel;
        m3dNormalize(&d);
        m3dMakeVUnitConusRandom(&d, 8.0f, &d);
        m3dNormalize(&d);
        float speed = m3dSqrt(in.phys.vel[0] * in.phys.vel[0] + in.phys.vel[1] * in.phys.vel[1] +
                              in.phys.vel[2] * in.phys.vel[2]);
        one.phys.vel[0] = d.x * speed;
        one.phys.vel[1] = d.y * speed;
        one.phys.vel[2] = d.z * speed;
        entENTITY *e = entCreate(gsScenePtr, (char *)"pjl_gefest", (char *)"pjl_gefest", &cd, &one, 0);
        if (e)
            gsEffApplyLMChar(e, &floor, 0);
    }
}

// ---- the registration ---------------------------------------------------------------------------------------------

// v12 0x00445070  (feb 0x0043D790)
int __fastcall aiGefestInit()
{
    if (!entRegisterClass((char *)"ai_gefest", 0x244E5043, 0x32, aiGEFEST_GENERIC::Create, (char *)"ai_gefest", 0))
        return 0;
    if (!entRegisterClass((char *)"pjl_gefest", 0x46494247, 0x32, pjlGEFEST::Create, 0, 1))
        return 0;
    if (!wrsfxGefestInit())
        return 0;
    if (!sncLinkSndListData(11, gefSnd))
        return 0;
    if (!sncLinkSndListData(8, gefMiniSnd))
        return 0;
    if (!(gefTexScorch = txmManager->Add((char *)"sfx_ax_scorch_mark", 0x40003, 1)))
        return 0;
    if (!(gefTexCarbons = txmManager->Add((char *)"sfx_gefest_carbons", 0x40003, 1)))
        return 0;
    if (!(gefTexCarbonExpl = txmManager->Add((char *)"sfx_gefest_carbon_expl", 0x40003, 1)))
        return 0;
    return (gefTexGlow = txmManager->Add((char *)"sfx_gefest_hit_glow", 0x40003, 1)) != 0;
}
