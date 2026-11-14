// ai_glad_generic.cpp -- the gladiators' shared code (aiGLAD_GENERIC: "ai_glad_knife" / "ai_glad_mace") (Will Rock 1.2,
// v12.exe; feb obj ai_glad_generic.obj)
//
// Four bodies in one template set: knife / mace, thin / fat (a "_fat" name: 30 hp, slower, fewer throws). They run at
// the enemy and throw (knives, the flaming mace) from beyond twice their arrival distance -- a few throws (2 thin, 3
// fat; spawned ones more), then they close in and slash. Hit: a light (weapon 0, 1, 3, 4, 5, 7) or heavy reaction.
// 1.2: 21 sounds (feb 14) picked by priority, the sound flags set from the animation (FillSoundFlags took over the
// base ProcessActionFrame), a throw timer (Slot67: seen and thrown at within 2 s).

#include "game.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list);  // v12 0x004B1E30
int __fastcall aiGladKnifePjlInit();                                // ai_glad_knife.cpp
int __fastcall aiGladMacePjlInit();                                 // ai_glad_mace.cpp
int __fastcall wrsfxGladInit();                                     // sfx_glad.cpp (v12 0x004764D0)

// v12 0x0064B768.. 21 sounds, 0x68 apart (static init v12 0x00447FC0)
static sncSOUND_DESCR_3D gladSnd[21];
enum { GSND_RUN, GSND_FALL_GROUND, GSND_MACE_THIN_HIT, GSND_MACE_THIN_DEATH, GSND_MACE_THROW, GSND_MACE_ATTACK,
       GSND_MACE_APPEAR, GSND_MACE_FLY, GSND_MACE_HIT_GROUND, GSND_MACE_HIT_BODY, GSND_FAT_HIT, GSND_FAT_DEATH,
       GSND_KNIFE_THIN_HIT, GSND_KNIFE_THIN_DEATH, GSND_KNIFE_CATCH, GSND_KNIFE_THROW, GSND_KNIFE_ATTACK,
       GSND_KNIFE_RELOAD, GSND_KNIFE_FLY, GSND_KNIFE_HIT_GROUND, GSND_KNIFE_HIT_BODY };

static struct aiGLAD_SND_INIT {
    aiGLAD_SND_INIT()
    {
        static const struct { const char *name; int flags; float mn, mx; } s[21] = {
            { "gladiator_run", 0x10002, 5.0f, 15.0f },
            { "gladiator_fall_ground", 0x10000, 10.0f, 33.0f },
            { "gladiator_bulava_thin_being_hit", 0, 25.5f, 70.0f },
            { "gladiator_bulava_thin_death", 0, 30.0f, 80.0f },
            { "gladiator_bulava_throw", 0, 20.0f, 60.0f },
            { "gladiator_bulava_attack", 0, 15.0f, 50.0f },
            { "gladiator_bulava_appear", 0, 15.0f, 50.0f },
            { "gladiator_bulava_fly", 0x10002, 4.0f, 20.0f },
            { "gladiator_bulava_hit_ground", 0x20000, 4.0f * 1.3f, 20.0f * 1.5f },
            { "gladiator_bulava_hit_body", 0x20000, 4.0f * 1.3f, 20.0f * 1.5f },
            { "gladiator_fat_being_hit", 0, 30.0f, 80.0f },
            { "gladiator_fat_death", 0, 30.0f, 80.0f },
            { "gladiator_knife_thin_being_hit", 0, 25.5f, 70.0f },
            { "gladiator_knife_thin_death", 0, 30.0f, 80.0f },
            { "gladiator_knife_catch", 0, 15.0f, 50.0f },
            { "gladiator_knife_throw", 0, 20.0f, 60.0f },
            { "gladiator_knife_attack", 0, 15.0f, 50.0f },
            { "gladiator_knife_reload", 0, 15.0f, 50.0f },
            { "gladiator_knife_fly", 0x10002, 3.0f, 18.0f },
            { "gladiator_knife_hit_ground", 0x20000, 3.0f * 1.3f, 18.0f * 1.5f },
            { "gladiator_knife_hit_body", 0x20000, 3.0f * 1.3f, 18.0f * 1.5f },
        };
        for (int i = 0; i < 21; i++) {
            sncSOUND_DESCR_3D *d = &gladSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].mn;
            d->maxDist = s[i].mx;
            d->vtbl = 0;
        }
    }
} aiGladSndInit;

// v12 0x004494D0  (feb 0x004416A0)
sncSOUND_DESCR_3D *__fastcall aiGladGetSound(int i)
{
    return &gladSnd[i];
}

// v12 0x004484B0  (feb 0x004406F0)
// Both kinds (class "ai_glad"), their projectiles, their effects, the sounds.
int __fastcall aiGladiatorsAllInit()
{
    if (!entRegisterClass((char *)"ai_glad_knife", 0x244E5043, 0x32, aiGLAD_KNIFE::Create, (char *)"ai_glad", 0))
        return 0;
    if (!entRegisterClass((char *)"ai_glad_mace", 0x244E5043, 0x32, aiGLAD_MACE::Create, (char *)"ai_glad", 0))
        return 0;
    if (!aiGladKnifePjlInit())
        return 0;
    if (!aiGladMacePjlInit())
        return 0;
    if (!wrsfxGladInit())
        return 0;
    return sncLinkSndListData(21, gladSnd) != 0;
}

// v12 0x00448530  (feb 0x00440770)
// 30 sequences, 7 for each body (idle, run, appear, throw, slash, shot, death; the mace's second slash at 21 / 29).
BOOL32 aiGLAD_GENERIC::SetupAniSystem()
{
    static const char *names[30] = {
        "GLAD_SEQ_IDLE_KNIFE", "GLAD_SEQ_RUN_KNIFE", "GLAD_SEQ_APPEAR_KNIFE", "GLAD_SEQ_THROW_KNIFE",
        "GLAD_SEQ_SLASH_KNIFE", "GLAD_SEQ_SHOT_KNIFE", "GLAD_SEQ_DEATH_KNIFE", "GLAD_SEQ_IDLE_FAT_KNIFE",
        "GLAD_SEQ_RUN_FAT_KNIFE", "GLAD_SEQ_APPEAR_FAT_KNIFE", "GLAD_SEQ_THROW_FAT_KNIFE", "GLAD_SEQ_SLASH_FAT_KNIFE",
        "GLAD_SEQ_SHOT_FAT_KNIFE", "GLAD_SEQ_DEATH_FAT_KNIFE", "GLAD_SEQ_IDLE_MACE", "GLAD_SEQ_RUN_MACE",
        "GLAD_SEQ_APPEAR_MACE", "GLAD_SEQ_THROW_MACE", "GLAD_SEQ_SLASH1_MACE", "GLAD_SEQ_SHOT_MACE",
        "GLAD_SEQ_DEATH_MACE", "GLAD_SEQ_SLASH2_MACE", "GLAD_SEQ_IDLE_FAT_MACE", "GLAD_SEQ_RUN_FAT_MACE",
        "GLAD_SEQ_APPEAR_FAT_MACE", "GLAD_SEQ_THROW_FAT_MACE", "GLAD_SEQ_SLASH1_FAT_MACE", "GLAD_SEQ_SHOT_FAT_MACE",
        "GLAD_SEQ_DEATH_FAT_MACE", "GLAD_SEQ_SLASH2_FAT_MACE" };
    static const int base[4] = { 0, 7, 14, 22 };
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 30;
    u32 *blk = (u32 *)operator new(0xFF4);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 30;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 30; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    for (int i = 0; i < 30; i++)
        _entAnimSetSeqName(t, i, (char *)names[i]);
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(30, 0);
    if (!s) {
        if (t->seq)
            operator delete((u32 *)t->seq - 1);
        return 0;
    }
    t->aniSystem = s;
    for (int k = 0; k < 4; k++) {
        int b = base[k];
        s->SetTranTypeToAny(b, 0, 1, -1);
        s->SetTranCycleAnim(b);
        s->SetTranParamFromAny(b + 5, 0, 1, b + 5);
        s->SetTranCycleAnim(b + 5);
        s->SetTranParamFromAny(b + 6, 0, 1, b + 6);
        s->SetTranParam(b + 1, b + 4, 0, b + 4, 1);
        s->SetTranParam(b + 6, b + 6, 2, b + 6, 1);
        s->SetTranParam(b + 2, b + 2, 2, b + 2, 1);
        s->SetTranCycleAnim(b + 3);
        s->SetTranParam(b + 3, b + 1, 1, b + 1, 0);
        s->SetTranParam(b + 1, b + 3, 1, b + 3, 0);
    }
    s->SetTranParam(15, 21, 0, 21, 1);
    s->SetTranParam(23, 29, 0, 29, 1);
    if (old)
        operator delete((u32 *)old - 1);
    return 1;
}

// v12 0x00448970  (feb 0x00440B70)
// 15 hp, quick (6.5 m/s, turning 110), a sphere voice, "AttackFocus", "ROOT" (bleeding); spawned: appearing.
int aiGLAD_GENERIC::ProcessINIT(void *init)
{
    if (!aiNPC_WR::ProcessINIT(init))
        return 0;
    maxHealth = 15.0f;
    navArriveDist = 1.7f;
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    alertAngle = 180.0f;
    moveSpeed = 6.525f;
    turnSpeed = 110.0f;
    aimAngle = 20.0f;
    shootRange = 40.0f;
    minThrowDist = 5.0f;
    heightCDT = 1.3f;
    corpseTime = 7.0f;
    corpseFadeTime = 6.0f;
    hitLight = 1;
    meetDist = 10.0f;
    navStuckTime = 2.0f;
    heightUp1 = 2.1f;
    fallbackAngle = 90.0f;
    health = 15.0f;
    throwTimer = 0;
    throwsLeft = 5;
    gladFlags = 2;
    aiFlags2 |= 0x18000;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;          // (a sncCHANNEL_SPH there)
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
    if (!(rootObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    if (flags2 & 1)
        gladFlags |= 0x200;
    return 1;
}

// v12 0x00448BF0  (feb 0x00440E00)
// "_fat": 30 hp, 7.25 m/s, 3 throws (4 spawned), turning back at 110; else 7.18 m/s, 2 throws (4 spawned). Then
// the starting animation.
void aiGLAD_GENERIC::ParseNameClass()
{
    const char *name = inst->className ? inst->className : "";
    aiNPC_WR::ParseNameClass();
    if (strstr(name, "_fat"))
        gladFlags |= 0x40;
    if (gladFlags & 0x40) {
        maxHealth = 30.0f;
        moveSpeed = 7.25f;
        fallbackAngle = 110.0f;
        throwsLeft = 3 + ((flags2 & 1) ? 1 : 0);
    } else {
        moveSpeed = 7.1775f;
        throwsLeft = (flags2 & 1) ? 4 : 2;
    }
    health = maxHealth;
    ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, EvalAnimGoalSeq(), 0.0f, 0);
}

// v12 0x00448CC0  (feb 0x00440ED0)
// Not while throwing. Close (within its aim): when hit-locked (0x40000) slash at once; with throws left and the
// enemy beyond twice the arrival distance: throw (seen, not blocked, timer out, beyond 5 m, within the aim);
// out of throws: slash when the enemy is in the aim, or take the throws back once 4 x arrival away.
void aiGLAD_GENERIC::FillAttackFlag()
{
    int s = inst->seq;
    if (s == 3 || s == 10 || s == 17 || s == 25)
        return;
    u32 f = aiFlags & ~0x1000u;
    aiFlags = f;
    u32 g = gladFlags;
    if (!(g & 2))
        return;
    if (f & 0x40000) {
        if ((f & 0x10000000) && !(f & 0x80000008) && !(wrFlags & 0x200) && enemyAngle <= aimAngle) {
            throwsLeft = 1;
            aiFlags = f | 0x1000;
        }
        return;
    }
    if (throwsLeft > 0 && navArriveDist + navArriveDist < enemyDist) {
        if ((f & 0x10000000) && !(f & 0x80000008) && !(wrFlags & 0x200) && throwTimer <= 1e-6f &&
            enemyDist >= minThrowDist && enemyAngle <= aimAngle)
            aiFlags = f | 0x1000;
        gladFlags = g & ~4u;
        return;
    }
    if (f & 1) {
        if (enemyAngle <= aimAngle)
            gladFlags = g | 4;
        return;
    }
    gladFlags = g & ~4u;
    if (navArriveDist * 4.0f < enemyDist && throwTimer < 1e-6f)
        throwsLeft = (int)((((g & 0x40) | 0x80) >> 6) & 0xFF);
}

// v12 0x00448E80  (feb 0x00441090)
// Not running: no turning on the spot (0x200); not throwing or appearing: it may attack (2).
void aiGLAD_GENERIC::Think()
{
    aiNPC_WR::Think();
    FillAttackFlag();
    int s = inst->seq;
    if (s != 1 && s != 8 && s != 15 && s != 23)
        aiFlags |= 0x200;
    if (s != 3 && s != 10 && s != 17 && s != 25 && s != 2 && s != 9 && s != 16 && s != 24)
        gladFlags |= 2;
}

// v12 0x00448F10 (1.2)
void aiGLAD_GENERIC::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&throwTimer, gsElapsedTime);
}

// v12 0x00448F30  (feb 0x00441120)
// Hit by a weapon (when the entity takes hits): a light reaction (weapons 0, 1, 3, 4, 5, 7; not over a heavy one)
// or a heavy one; killed: heavy.
int aiGLAD_GENERIC::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (!(entFlags & 4))
        return 1;
    if (d->type == 1) {
        u32 w = *(u32 *)((u8 *)d + 0x1A);                 // dmgWEAPON::weaponType
        if (w <= 10) {
            static const u8 kind[11] = { 0, 0, 1, 0, 0, 0, 1, 0, 1, 1, 1 };
            if (kind[w] == 0) {
                if (!(gladFlags & 8))
                    gladFlags |= 0x10;
            } else
                gladFlags |= 0x20;
        }
    }
    aiNPC_WR::ProcessCDT_FIREABLE(d);
    if (aiFlags & 0x200000)
        gladFlags |= 0x20;
    return 1;
}

// v12 0x00448FC0  (feb 0x004411B0): knocked about only by weapons
int aiGLAD_GENERIC::StartPassivePhysics(dmgDAMAGE *d)
{
    if (d && d->type == 1)
        return aiNPC_WR::StartPassivePhysics(d);
    return 0;
}

// v12 0x00448FE0  (feb 0x004411D0; 1.2: one sound a frame, by priority)
// The first of: fat death / knife death / mace death / fat hit / mace hit / knife hit / mace throw / mace slash /
// mace back in hand / knife catch / knife throw / knife slash / knife reload / the fall / running. The running
// sound stops once it is not running.
void aiGLAD_GENERIC::PlaySounds()
{
    u32 f = unk108;
    static const struct { u32 bit; int snd; } order[15] = {
        { 0x800, GSND_FAT_DEATH }, { 0x2000, GSND_KNIFE_THIN_DEATH }, { 8, GSND_MACE_THIN_DEATH },
        { 0x400, GSND_FAT_HIT }, { 4, GSND_MACE_THIN_HIT }, { 0x1000, GSND_KNIFE_THIN_HIT },
        { 0x10, GSND_MACE_THROW }, { 0x20, GSND_MACE_ATTACK }, { 0x40, GSND_MACE_APPEAR },
        { 0x4000, GSND_KNIFE_CATCH }, { 0x8000, GSND_KNIFE_THROW }, { 0x10000, GSND_KNIFE_ATTACK },
        { 0x20000, GSND_KNIFE_RELOAD }, { 2, GSND_FALL_GROUND }, { 1, GSND_RUN } };
    for (int i = 0; i < 15; i++)
        if (f & order[i].bit) {
            voice->Play(&gladSnd[order[i].snd], 0);
            break;
        }
    if (voice->IsPlay(&gladSnd[GSND_RUN])) {
        int s = inst->seq;
        if (s != 1 && s != 8 && s != 15 && s != 23)
            voice->Stop();
    }
    aiNPC::PlaySounds();
}

// v12 0x00449170  (feb 0x00441320 + the base ProcessActionFrame 0x004413F0)
// The animation's sound frames (knife throw / reload / catch / slash, mace back in hand / slash / throw), the fall
// (thrown down, its drift landing), and on a new sequence: running, hit, death (by body).
void aiGLAD_GENERIC::FillSoundFlags()
{
    animINST *a = inst;
    if (a->IsActionFrame(3, 0) || a->IsActionFrame(10, 0) || a->IsActionFrame(3, 1) || a->IsActionFrame(10, 1))
        unk108 |= 0x8000;
    if (a->IsActionFrame(3, 2) || a->IsActionFrame(10, 2))
        unk108 |= 0x20000;
    if (a->IsActionFrame(3, 3) || a->IsActionFrame(10, 3))
        unk108 |= 0x4000;
    if (a->IsActionFrame(4, 0) || a->IsActionFrame(11, 0) || a->IsActionFrame(4, 1) || a->IsActionFrame(11, 1))
        unk108 |= 0x10000;
    if (a->IsActionFrame(17, 3) || a->IsActionFrame(25, 3))
        unk108 |= 0x40;
    if (a->IsActionFrame(18, 0) || a->IsActionFrame(21, 0) || a->IsActionFrame(26, 0) || a->IsActionFrame(29, 0) ||
        a->IsActionFrame(18, 1) || a->IsActionFrame(21, 1) || a->IsActionFrame(26, 1) || a->IsActionFrame(29, 1))
        unk108 |= 0x20;
    if (a->IsActionFrame(17, 0) || a->IsActionFrame(25, 0) || a->IsActionFrame(17, 1) || a->IsActionFrame(25, 1))
        unk108 |= 0x10;
    if ((aiFlags & 0x8000) && WR_FIELD(int, passivePhys, 0xB0) == 1)
        unk108 |= 2;
    if (!(a->flags & 0x4000))
        return;
    switch (a->seq) {
    case 1: case 8: case 15: case 23:
        unk108 |= 1;
        break;
    case 5:
        unk108 |= 0x1000;
        break;
    case 6:
        unk108 |= 0x2000;
        break;
    case 12: case 27:
        unk108 |= 0x400;
        break;
    case 13: case 28:
        unk108 |= 0x800;
        break;
    case 19:
        unk108 |= 4;
        break;
    case 20:
        unk108 |= 8;
        break;
    }
}

// v12 0x004494E0 (1.2)
int aiGLAD_GENERIC::Slot67()
{
    return (aiFlags & 0x10000000) && throwTimer < 2.0f;
}
