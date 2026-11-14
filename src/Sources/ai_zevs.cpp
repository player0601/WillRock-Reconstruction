// ai_zevs.cpp -- Zeus, the last boss (Will Rock 1.2, v12.exe; feb obj ai_zevs.obj)
//
// He sits on his throne (lev_10_4_zeus "actor_Zeus", locked until the throne turns). Woken, he fades in (single
// player) with the lightning round him, laughs, and attacks from where he sits: BASH (1) throws a series of bolts
// from his trident ("Garp_lod_8") at the "&dom_ai_zevs_bolts" area, BLOW (2) a projectile from "ATTACKfocus"; LOOK
// (3) and BREATHE (0) between. As his health goes down by fifths the five "Actor_roof_break_0n" niches fall in.
// 30000 health (10000 in feb). A "_mini" Zeus is not the boss and has no bolts.
// Not rebuilt here (the calls are left out): the bolts and the projectile (LaunchOneBolt / LaunchOneProjectile,
// ai_zevs_pjl.obj "pjl_zevs"), the stars and lightning emitters (aiZEVS_DEATH_SFX_*), the roofs breaking
// (wrsobNICHE::StartExplos, sob_niche.obj).

#include "game.h"
#include "../../include/sfx_sob/sob_entities.h"

static_assert(sizeof(aiZEVS) == 0x737, "aiZEVS size");

txmTEXTURE *aiZevsTexStars;                             // "sfx_zeus_death_stars"
txmTEXTURE *aiZevsTexLgtn;                              // "sfx_zeus_death_lgtn"
static float zevsLaughTimer;                            // v12 0x0064E484 after waking, BREATHE first

// v12 0x0064E500 the sounds (stride 0x68), linked in aiZevsInit
enum { ZS_LAUGH, ZS_DEATH, ZS_ATTACK_LAUGH, ZS_ATTACK_STUFF, ZS_ATTACK_BLOW, ZS_IDLE_1, ZS_IDLE_2, ZS_IDLE_3,
       ZS_PJL_FLY, ZS_PJL_HIT_GROUND, ZS_LIGHTNING_MAIN, ZS_APPEAR, ZS_COUNT };
static sncSOUND_DESCR_3D zevsSnd[ZS_COUNT];
static struct aiZEVS_SND_INIT {
    aiZEVS_SND_INIT()                                   // v12 0x0045F590 (the static initialiser)
    {
        static const struct { const char *name; u32 flags; float minD, maxD; } s[ZS_COUNT] = {
            { "zeus_laugh", 0x40000, 300, 800 },        { "zeus_death", 0x60000, 300, 800 },
            { "zeus_attack_laugh", 0x40000, 300, 800 }, { "zeus_attack_stuff", 0x40000, 300, 800 },
            { "zeus_attack_blow", 0x40000, 300, 800 },  { "zeus_idle_1", 0x10000, 300, 800 },
            { "zeus_idle_2", 0x10000, 300, 800 },       { "zeus_idle_3", 0x10000, 300, 800 },
            { "zeus_pjl_fly", 0x10002, 50, 200 },       { "zeus_pjl_hit_ground", 0x60000, 65, 300 },
            { "zeus_lightning_main", 0, 1000, 1000 },   { "zeus_appear", 0, 300, 800 },
        };
        for (int i = 0; i < ZS_COUNT; i++) {
            sncSOUND_DESCR_3D *d = &zevsSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].minD;
            d->maxDist = s[i].maxD;
            d->vtbl = 0;
        }
    }
} aiZevsSndInit;

// v12 0x0045F910  (feb 0x00456610)
int __fastcall aiZevsInit()
{
    if (!entRegisterClass((char *)"ai_zevs", 0x244E5043, 0x32, aiZEVS::Create, (char *)"ai_zevs", 0))
        return 0;
    // (not rebuilt: aiPjlZevsMoteInit v12 0x004617B0 "pjl_zevs", wrsfxZeusInit v12 0x00485D50 "sfx_zeus_lgtn")
    if (!(aiZevsTexStars = txmManager->Add((char *)"sfx_zeus_death_stars", 0x40003, 1)))
        return 0;
    if (!(aiZevsTexLgtn = txmManager->Add((char *)"sfx_zeus_death_lgtn", 0x40003, 1)))
        return 0;
    return sncLinkSndListData(ZS_COUNT, zevsSnd) != 0;
}

// v12 0x004616B0  (feb 0x00458170)
entENTITY *__fastcall aiZEVS::Create(animINST *)
{
    aiZEVS *z = new aiZEVS;
    if (!z)
        return 0;
    z->wrFlags = 0;
    z->bloodObj = 0;
    z->zevsFlags = 0;
    z->voice = 0;
    z->emitter = 0;
    z->unk98 = 0x14;
    return z;
}

// v12 0x0045F9A0  (feb 0x004566A0)
// Five sequences; any goes to any other directly, BLOW after BLOW through LOOK, BASH after BASH through BREATHE.
BOOL32 aiZEVS::SetupAniSystem()
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
    _entAnimSetSeqName(t, 0, (char *)"ZEVS_SEQ_BREATHE");
    _entAnimSetSeqName(t, 1, (char *)"ZEVS_SEQ_BASH");
    _entAnimSetSeqName(t, 3, (char *)"ZEVS_SEQ_LOOK");
    _entAnimSetSeqName(t, 2, (char *)"ZEVS_SEQ_BLOW");
    _entAnimSetSeqName(t, 4, (char *)"ZEVS_SEQ_TURN");
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(5, 0);
    if (!s)
        return 0;
    t->aniSystem = s;
    static const u8 tr[14][4] = {
        { 0, 1, 1, 1 }, { 0, 3, 1, 3 }, { 0, 2, 1, 2 }, { 1, 0, 1, 0 }, { 1, 3, 1, 3 }, { 1, 2, 1, 2 },
        { 3, 0, 1, 0 }, { 3, 1, 1, 1 }, { 3, 2, 1, 2 }, { 2, 0, 1, 0 }, { 2, 1, 1, 1 }, { 2, 3, 1, 3 },
        { 2, 2, 1, 3 }, { 1, 1, 1, 0 },
    };
    for (int i = 0; i < 14; i++)
        s->SetTranParam(tr[i][0], tr[i][1], tr[i][2], tr[i][3], 0);
    if (old)
        operator delete((u32 *)old - 1);
    return 1;
}

// v12 0x0045FBD0  (feb 0x004568D0)
void aiZEVS::SetMovementSpeed()
{
    aiFlags |= 0x600;                                   // he does not walk
}

// v12 0x0045FBE0  (feb 0x004568E0)
int aiZEVS::ProcessINIT(void *init)
{
    if (!aiNPC_WR::ProcessINIT(init))
        return 0;
    radCDT = 15.0f;
    heightLowTester = 15.0f;
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    aiFlags |= 0x40000;
    aiFlags2 |= 0x30000;
    maxHealth = 30000.0f;
    health = 30000.0f;
    heightCDT = 40.0f;
    fallbackAngle = 10000.0f;
    alertAngle = 180.0f;
    maxTurnStep = 70.0f;
    boltTimer = 5.0f;
    timeDeactivate = 3600.0f;
    corpseFadeTime = 0.33333334f;
    corpseTime = 1.6666666f;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
    if (c) {
        c->minDist = 0.0f;
        c->maxDist = 1.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    unk108 |= 0x100;                                    // "zeus_appear"
    rootObj = objFindName(inst->root, (char *)"ROOT");
    if (!rootObj)
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    if (!(attackFocus = objFindName(inst->root, (char *)"ATTACKfocus")))
        return 0;
    if (!(garp = objFindName(inst->root, (char *)"Garp_lod_8")))
        return 0;
    emitter = objFindNameN(inst->root, (char *)"emitter", 7);
    inst->flags |= 1;                                   // hidden until woken
    flags2 |= 0x80;
    fadeIn = 30000.0f;
    gsScenePtr->SetStateYesInst(inst, 8);
    inst->root->SetStateYes(0x20);
    return 1;
}

// v12 0x0045FEB0  (feb 0x00456B70)
void aiZEVS::ProcessINIT_LEVEL()
{
    cdtREFINE ref(2, 0);
    inst->Validate(2);
    m3dV pos, out;
    inst->GetPos(&pos);
    gsScenePtr->FloorPlace(&pos, 100.0f, &out, heightCDT, 0, 0, &ref, &floor);
    MakeRoofsIgnoreCDT();
    aiNPC_WR::ProcessINIT_LEVEL();
}

// v12 0x0045FF60  (feb 0x00456C00)
// Woken: in single player he fades in over a second; seen, the lightning, the laugh first.
int aiZEVS::Activate(int force)
{
    if (WR_FIELD(int, gsSysMP, 0x28) == 1) {
        fadeIn = 3.0f;
        inst->root->SetTransparency(0x80, 0);
    } else {
        fadeIn = -1.0f;
    }
    inst->flags &= ~1u;
    if (emitter)
        emitter->SetStateProcYes(8, 0);
    boltsLeft = 0;
    zevsLaughTimer = 3.5f;
    blowTimer = 8.0f;
    StartMainSFX();
    return aiNPC::Activate(force);
}

// v12 0x0045FFF0  (feb 0x00456C90)
void aiZEVS::ProcessTERM()
{
    if (zevsFlags & 0x100)
        boltArea.Term();
    if (!(zevsFlags & 0x10000))
        aiAttackPlanner->SetBoss(0);
    aiNPC_WR::ProcessTERM();
}

// v12 0x00460030  (feb 0x00456D00)
int aiZEVS::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    aiNPC_WR::ProcessCDT_FIREABLE(d);
    return 1;
}

// v12 0x00460050  (feb 0x00456D70)
// PLACEHOLDER (not rebuilt: aiZEVS_DEATH_SFX_STARS / _LGTN, v12 0x00460C70 / 0x00460E20): the stars round his torso
// and the lightning at his "emitter" are not made.
void aiZEVS::StartMainSFX()
{
}

// v12 0x00460120  (feb 0x00456E40)
void aiZEVS::StartDeath(dmgDAMAGE *d)
{
    StartMainSFX();
    unk108 |= 2;                                        // "zeus_death"
    aiNPC_WR::StartDeath(d);
}

// v12 0x00460150  (feb 0x00456E60)
// He stays where he is (the enemy's position read, nothing done with it).
int aiZEVS::SelectGoal()
{
    m3dV e, p;
    enemy.GetPos(&e);
    inst->GetPos(&p);
    return 1;
}

// v12 0x00460180  (feb 0x00456E90)
// The next sequence: in a bolt series LOOK or BREATHE; just woken BREATHE; BLOW while its timer runs; else at
// random. With few enemies about (under 12 times the spawn coefficient) BLOW is the third choice, else LOOK or
// BREATHE.
int aiZEVS::EvalAnimGoalSeq()
{
    int third = (float)WR_FIELD(int, aiAttackPlanner, 0x28) < aiSpawnGetOppCoeff() * 12.0f ? 2 : m3dRandSelect(0, 3);
    if (boltsLeft > 0)
        return m3dRandSelect(3, 0);
    if (zevsLaughTimer > 1e-6f)
        return 0;
    if (blowTimer > 1e-6f)
        return 2;
    if (boltTimer > 1e-6f)
        return m3dRandSelect(0, third, 3);
    return m3dRandSelect(0, third, 1, 1);
}

// v12 0x00460250  (feb 0x00456EF0)
// The timers; a series' next bolt; the fading in (alpha 255 - 84 * fadeIn, three times as fast as time).
void aiZEVS::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    if (m3dUpdateTimeField(&boltTimer, gsElapsedTime) && boltsLeft)
        LaunchOneBolt();
    m3dUpdateTimeField(&blowTimer, gsElapsedTime);
    m3dUpdateTimeField(&zevsLaughTimer, gsElapsedTime);
    if (fadeIn > 1e-6f) {
        inst->root->SetTransparency(0x80, (u8)(int)(255.0f - fadeIn * 84.0f));
        if (m3dUpdateTimeField(&fadeIn, gsElapsedTime * 3.0f))
            inst->root->SetTransparency(0, 0xFF);
    }
}

// v12 0x00460320
// The sounds' events from the animation: BASH's frames 1 (stuff) and 0 (the lightning), BLOW's 1, LOOK's 0; a
// sequence starting: BREATHE an idle, BASH the attack laugh.
void aiZEVS::FillSoundFlags()
{
    if (inst->IsActionFrame(1, 1))
        unk108 |= 8;
    if (inst->IsActionFrame(1, 0))
        unk108 |= 0x10;
    if (inst->IsActionFrame(2, 1))
        unk108 |= 0x20;
    if (inst->IsActionFrame(3, 0))
        unk108 |= 0x80;
    if (inst->flags & 0x4000) {
        if (inst->seq == 0)
            unk108 |= 0x40;
        else if (inst->seq == 1)
            unk108 |= 4;
    }
}

// v12 0x004603F0  (feb 0x00456FC0)
void aiZEVS::PlaySounds()
{
    u32 f = unk108;
    if (f & 2)
        voice->Play((sncSOUND_DESCR *)&zevsSnd[ZS_DEATH], 0);
    else if (f & 4)
        voice->Play((sncSOUND_DESCR *)&zevsSnd[ZS_ATTACK_LAUGH], 0);
    else if (f & 8)
        voice->Play((sncSOUND_DESCR *)&zevsSnd[ZS_ATTACK_STUFF], 0);
    else if (f & 0x20)
        voice->Play((sncSOUND_DESCR *)&zevsSnd[ZS_ATTACK_BLOW], 0);
    else if (f & 1)
        voice->Play((sncSOUND_DESCR *)&zevsSnd[ZS_LAUGH], 0);
    else if (f & 0x80)
        voice->Play((sncSOUND_DESCR *)&zevsSnd[ZS_IDLE_3], 0);
    else if (f & 0x40)
        voice->Play((sncSOUND_DESCR *)&zevsSnd[m3dRandSelect(ZS_IDLE_1, ZS_IDLE_2)], 0);
    m3dV p;
    if (unk108 & 0x10) {
        inst->GetPos(&p);
        gsSysSound->Play(&zevsSnd[ZS_LIGHTNING_MAIN], &p);
    }
    if (unk108 & 0x100) {
        inst->GetPos(&p);
        gsSysSound->Play(&zevsSnd[ZS_APPEAR], &p);
    }
    aiNPC::PlaySounds();
}

// v12 0x00460550  (feb 0x00457040)
// A "_mini" Zeus is no boss; the real one is (the planner's boss) and gets the bolts' area if the level has one.
void aiZEVS::ParseNameClass()
{
    char *tpl = inst->tplName ? inst->tplName : (char *)"";
    aiNPC_WR::ParseNameClass();
    if (strstr(tpl, "_mini"))
        zevsFlags |= 0x10000;
    if (zevsFlags & 0x10000)
        return;
    aiFlags2 |= 0x40000;
    if (objOBJ *o = objFindName(gsScenePtr->root, (char *)"&dom_ai_zevs_bolts")) {
        boltArea.obj = o;
        zevsFlags |= 0x100;
        boltArea.Init();
    }
    if (!(zevsFlags & 0x10000))
        aiAttackPlanner->SetBoss(this);
}

// v12 0x00460600  (feb 0x004570E0)
// A roof falls in for each fifth of his health gone.
void aiZEVS::Think()
{
    aiNPC_WR::Think();
    float step = maxHealth * 0.2f, at = maxHealth - step;
    for (int i = 0; i < 5; i++, at -= step)
        if (roofs[i] && health < at) {
            if (roofs[i]->ent)                              // the roof falls in (wrsobNICHE::StartExplos)
                ((wrsobNICHE *)roofs[i]->ent)->StartExplos(0);
            roofs[i] = 0;
        }
}

// v12 0x00460680  (feb 0x00457160)
// The five roofs that fall: their niches do not collide (until they fall).
void aiZEVS::MakeRoofsIgnoreCDT()
{
    static const char *names[5] = { "Actor_roof_break_01", "Actor_roof_break_02", "Actor_roof_break_03",
                                    "Actor_roof_break_04", "Actor_roof_break_05" };
    for (int i = 0; i < 5; i++)
        roofs[i] = gsScenePtr->FindInstName((char *)names[i]);
    for (int i = 0; i < 5; i++) {
        animINST *r = roofs[i];
        if (!r)
            continue;
        if (r->ent)
            ((wrsobNICHE *)r->ent)->f_16F = 1;          // wrsobNICHE: damage does not break it
        else
            apLogErr((char *)"AI: zeus roof has no entity (%s)", r->name ? r->name : "");
    }
}

// v12 0x00460740  (feb 0x00457220)
// BASH's frame 0: a series of bolts (2 at most, fewer the farther the enemy) from the trident; BLOW's frame 0 the
// projectile.
void aiZEVS::ProcessActionFrame()
{
    if (inst->IsActionFrame(1, 0)) {
        boltsLeft = 2 - (int)(enemyDist * -0.033333335f);
        garp->GetOrigin(&boltFrom);
        LaunchOneBolt();
    }
    if (inst->IsActionFrame(2, 0))
        LaunchOneProjectile();
}

// PLACEHOLDER (not rebuilt: v12 0x00460A30, ai_zevs_pjl.obj): no bolt flies; the series still counts down
void aiZEVS::LaunchOneBolt()
{
    if (boltsLeft > 0)
        boltsLeft--;
    boltTimer = 0.5f;
}

// PLACEHOLDER (not rebuilt: v12 0x004607B0, "pjl_zevs"): no projectile
void aiZEVS::LaunchOneProjectile()
{
}
