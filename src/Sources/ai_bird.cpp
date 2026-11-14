// ai_bird.cpp -- aiBIRD, the Stymphalian bird ("ai_bird", the harpy sounds) (Will Rock 1.2, v12.exe; feb obj
// ai_bird.obj)
//
// It flies about an area (its "AI_PROP area" object, "ai_area_bird" by default; a spawned one: its domain's), a new
// point of it every 2.5..4 s, soaring (SOAR) when well above where it is going. Every 10 s or so (5 at first) it
// attacks: it flies to 10 m ahead of the enemy and 6 m up, dives once it is within 5 m of that or after 9 s, and drops
// an egg (THROW_EGG's action frame; "bird_egg") when the enemy is within 20 m flat, below it and roughly ahead; after
// the egg, or 15 s, it goes back to its area. Shot down it falls (aiNPC_WR_FLY) and on the first touch bleeds and
// starts "sfx_bird_death".

#include "game.h"
#include "../../include/game_shared/projectile.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list);  // v12 0x004B1E30
int __fastcall birdEggInit();                                       // v12 0x00462A90 (bird_egg.obj)
int __fastcall wrsfxBirdInit();                                     // v12 0x0046A290 (sfx_bird.obj)

// v12 0x006496B8.. 9 sounds, 0x68 apart (the static initialisers v12 0x00438A10..0x00438CEE); the distances are
// the base ones scaled, the nearer by 1.3 and the farther by 1.2 (the egg's 1.3 / 1.5)
static sncSOUND_DESCR_3D birdSnd[9];
enum { BSND_APPEAR, BSND_HIT, BSND_DEATH, BSND_FALL_GROUND, BSND_FLY, BSND_EGG_THROW, BSND_EGG_FLY, BSND_EGG_EXPLOSION,
       BSND_EGG_HIT_BODY };

static struct aiBIRD_SND_INIT {
    aiBIRD_SND_INIT()
    {
        static const struct { const char *name; int flags; float mn, mx; } s[9] = {
            { "harpy_appear", 0x40000, 30.0f * 1.3f, 80.0f * 1.2f },
            { "harpy_being_hit", 0x40000, 25.5f * 1.3f, 70.0f * 1.2f },
            { "harpy_death", 0, 30.0f * 1.3f, 80.0f * 1.2f },
            { "harpy_fall_ground", 0x40000, 8.0f * 1.3f, 25.0f * 1.2f },
            { "harpy_fly", 0x10000, 15.0f * 1.3f, 50.0f * 1.2f },
            { "harpy_egg_throw", 0, 25.5f * 1.3f, 70.0f * 1.2f },
            { "harpy_egg_fly", 0x10002, 4.0f * 1.0f, 20.0f * 1.0f },
            { "harpy_egg_explosion", 0x20000, 4.0f * 1.3f, 20.0f * 1.5f },
            { "harpy_egg_hit_body", 0x20000, 4.0f * 1.3f, 20.0f * 1.5f },
        };
        for (int i = 0; i < 9; i++) {
            sncSOUND_DESCR_3D *d = &birdSnd[i];
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
} aiBirdSndInit;

// v12 0x00438CF0 (the egg's sounds are the bird's)
sncSOUND_DESCR_3D *__fastcall birdGetSound(int i)
{
    return &birdSnd[i];
}

// PLACEHOLDER (test, not rebuilt): sfx_bird.obj -- no death effect is registered, so entCreate makes none.
// DELETE when wrsfxBirdInit is rebuilt. (birdEggInit: src/Sources/bird_egg.cpp)
int __fastcall wrsfxBirdInit() { return 1; }

// v12 0x00438D00  (feb 0x004328E0)
int __fastcall aiBirdInit()
{
    if (!entRegisterClass((char *)"ai_bird", 0x244E5043, 0x32, aiBIRD::Create, (char *)"ai_bird", 0))
        return 0;
    if (!birdEggInit())
        return 0;
    if (!sncLinkSndListData(9, birdSnd))
        return 0;
    return wrsfxBirdInit() != 0;
}

// v12 0x00438D50  (feb 0x00432930)
entENTITY *__fastcall aiBIRD::Create(animINST *)
{
    aiBIRD *b = new aiBIRD;                             // operator new(0x71B), aiNPC ctor; ownArea: aiAREA's own ctor
    if (!b)
        return 0;
    b->bloodObj = 0;
    b->birdFlags = 0;
    b->voice = 0;
    b->unk98 = 4;
    return b;
}

// v12 0x00438E00  (feb 0x004329E0)
BOOL32 aiBIRD::SetupAniSystem()
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
    _entAnimSetSeqName(t, 0, (char *)"BIRD_SEQ_UNUSED");
    _entAnimSetSeqName(t, 1, (char *)"BIRD_SEQ_FLY");
    _entAnimSetSeqName(t, 2, (char *)"BIRD_SEQ_THROW_EGG");
    _entAnimSetSeqName(t, 3, (char *)"BIRD_SEQ_DEATH");
    _entAnimSetSeqName(t, 4, (char *)"BIRD_SEQ_SOAR");
    entAnimAssignTplSeq(t, old, nOld);
    t->seq[0].frameStart = 0.0f;
    t->seq[0].frameEnd = 1.0f;
    t->seq[0].lengthMs = 500.0f;
    t->seq[0].dir = 0;
    animSYSTEM *s = new animSYSTEM(5, 0);
    if (!s)
        return 0;
    t->aniSystem = s;
    s->SetTranTypeToAny(0, 0, 1, -1);
    s->SetTranCycleAnim(1);
    s->SetTranTypeToAny(2, 1, 1, -1);
    s->SetTranParamFromAny(3, 0, 1, 3);
    s->SetTranParam(1, 2, 0, 2, 1);
    s->SetTranParam(3, 3, 2, 3, 1);
    if (old)
        operator delete((u32 *)old - 1);
    return 1;
}

// v12 0x00438FC0  (feb 0x00432BA0)
// 1.6 m round, 0.6 up, 8 m/s, 70 deg/s (20 a step), seen from 300 m, 7 health. Its area: the "AI_PROP area" object
// ("ai_area_bird") of the level, or a spawned one's domain's. Falls at -4 up, the corpse 8 s (6 fading).
int aiBIRD::ProcessINIT(void *init)
{
    radCDT = 1.6f;
    heightLowTester = 1.6f;
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    aiFlags2 |= 0x20000;
    heightCDT = 0.6f;
    moveSpeed = 8.0f;
    turnSpeed = 70.0f;
    maxTurnStep = 20.0f;
    if (!aiNPC_WR_FLY::ProcessINIT(init))
        return 0;
    attackDelay = 5.0f;
    attackTime = 0;
    timer57B = 10.0f;
    settleTimer = m3dRandRange(1.0f, 2.0f);
    wanderTimer = 0;
    maxHealth = 7.0f;
    health = 7.0f;
    fallbackAngle = 10.0f;
    if (!(bloodObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    m3dV pos;
    inst->GetPos(&pos);
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
    if (c) {
        c->minDist = 0.0f;
        c->maxDist = 1.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    area = 0;
    if (!(flags2 & 1)) {
        char name[0x1000];
        strcpy(name, "ai_area_bird");
        inst->GetStr((char *)"AI_PROP", (char *)"area", name, sizeof(name));
        if (objOBJ *o = objFindName(gsScenePtr->root, name)) {
            ownArea.obj = o;
            ownArea.Init();
            o->SetStateProcYes(8, 0);
            area = &ownArea;
        }
    } else if (entFlags & 4) {
        area = spawnDomain ? WR_FIELD(aiAREA *, spawnDomain, 0xDC) : 0;
    }
    unk108 |= 1;                                        // the appearing sound
    aiFlags |= 1;
    passivePhys373 = 0.25f;
    fallAccel = -3.0f;
    fallSpeedMin = -0.17f;
    flyHeight = -4.0f;
    corpseTime = 8.0f;
    corpseFadeTime = 6.0f;
    Activate(0);
    return 1;
}

// v12 0x004392F0  (feb 0x00432EE0)
void aiBIRD::ProcessTERM()
{
    area = 0;
    ownArea.Term();
    aiNPC_WR::ProcessTERM();
}

// v12 0x00439360  (feb 0x00432F20)
// Dead and falling, its first touch: blood (with the fall's speed) and the death effect at its root.
void aiBIRD::ProcessCDT(cdtINFO *)
{
    if (!(aiFlags & 0x200000) || (birdFlags & 2))
        return;
    birdFlags |= 2;
    m3dV pos, v;
    inst->GetPos(&pos);
    v.x = passivePhys->vel[0];
    v.y = passivePhys->vel[1];
    v.z = passivePhys->vel[2];
    StartBlood(4, 1, &pos, 0, &v, &pos);
    // then entCreate("sfx_bird_death", "") and wrsfxBIRD_DEATH::Start(it, the root's matrix) -- sfx_bird.obj is not
    // rebuilt (no class registered: none would be made), so left out here
}

// v12 0x00439420  (feb 0x00432FE0)
int aiBIRD::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    aiNPC_WR::ProcessCDT_FIREABLE(d);
    unk108 |= 1;
    return 1;
}

// v12 0x00439460  (feb 0x00433020; feb tested the amount < 5000)
void aiBIRD::StartDeath(dmgDAMAGE *d)
{
    aiNPC_WR_FLY::StartDeath(d);
    if (d && d->subtype != 2)
        unk108 |= 4;                                    // the death cry
}

// v12 0x00439490  (feb 0x00433060)
void aiBIRD::UpdateDead()
{
    aiNPC_WR::UpdateDead();
    if (deadTimer > corpseFadeTime)
        inst->root->SetStateYes(0x80);
}

// v12 0x004394D0  (feb 0x004330A0)
void aiBIRD::MoveToGoal()
{
    ProcessAttack();
    aiNPC_WR::MoveToGoal();
}

// v12 0x004394E0  (feb 0x004330B0)
// Towards the goal at the speed, flat and up / down shared by the distance (diving: the height 1.2 times as fast;
// aiFlags 4: straight up at the full speed); turned too far, straight ahead along the facing; plus the strafe.
void aiBIRD::ClampGoalFrame(float spd, m3dV *dirTo, m3dV *goal)
{
    m3dV pos, g = *goal;
    inst->GetPos(&pos);
    float dx = pos.x - goal->x, dy = pos.y - goal->y, dz = pos.z - goal->z;
    float dist = m3dSqrt(dx * dx + dy * dy + dz * dz);
    float fwd, side;
    GetClampingSpeeds(spd, &fwd, &side);
    m3dV d;
    inst->GetDir(&d);
    float a = m3xzAngleVector(&d, dirTo);
    m3dV to = g;
    if (fabsf(a) <= maxTurnStep) {
        float hx = to.x - pos.x, hz = to.z - pos.z;
        float flat = m3dSqrt(hx * hx + hz * hz);
        m3dClampVector_L(&pos, &to, flat / dist * gsElapsedTime * fwd);
    } else {
        float k = gsElapsedTime * fwd;
        to.x = d.x * k + pos.x;
        to.y = d.y * k + pos.y;
        to.z = d.z * k + pos.z;
    }
    m3dV y0 = { 0, pos.y, 0 }, y1 = { 0, g.y, 0 };
    float k = fabsf(g.y - pos.y) / dist;
    if ((aiFlags & 0x1000) && !(aiFlags & 0x2000))
        k *= 1.2f;
    if (aiFlags & 4) {
        y1.y = gsElapsedTime * fwd + pos.y;
        k = 1.0f;
    }
    m3dClampVector_L(&y0, &y1, gsElapsedTime * k * fwd);
    goal->x = to.x;
    goal->z = to.z;
    goal->y = y1.y;
    if (navFlags & 1) {
        float s = gsElapsedTime * side;
        goal->x = s * strafeDir.x + to.x;
        goal->y = s * strafeDir.y + y1.y;
        goal->z = s * strafeDir.z + to.z;
    }
}

// v12 0x00439850  (feb 0x00433430)
// Attacking: 10 m ahead of the enemy (by its movement) and 6 m up -- diving: right over it, 6 m up; else a new point
// of its area (0..3 m higher) when the last one's time is up.
int aiBIRD::SelectGoal()
{
    m3dV pos, e, ed;
    inst->GetPos(&pos);
    enemy.GetPos(&e);
    enemy.GetDir(&ed);
    ed.y = 0;
    m3dNormalize(&ed);
    if (aiFlags & 0x1000) {
        if (aiFlags & 0x2000) {
            goalPos = e;
            goalPos.y += 6.0f;
            wayPointPos = e;
        } else {
            goalPos.x = ed.x * 10.0f + e.x;
            goalPos.y = ed.y * 10.0f + e.y;
            goalPos.z = ed.z * 10.0f + e.z;
            goalPos.y += 6.0f;
            wayPointPos = goalPos;
        }
        CorrectGoalByWaterLevel();
        return 1;
    }
    if (wanderTimer < 1e-6f && area) {
        area->GetRandomPos(&goalPos, &pos);
        goalPos.y += m3dRandRange(0.0f, 3.0f);
        wanderTimer = m3dRandRange(2.5f, 4.0f);
    }
    wayPointPos = goalPos;
    CorrectGoalByWaterLevel();
    return 1;
}

// v12 0x00439A30  (feb 0x00433610)
// FLY, SOAR when 2 m above the goal, DEATH dead; attacking, THROW_EGG once (it sees the enemy, not over the egg's
// sequence) it is over the dive point (5 m flat, 3 m up or down) or the enemy is within 20 m flat and 3 m below.
int aiBIRD::EvalAnimGoalSeq()
{
    m3dV pos;
    inst->GetPos(&pos);
    int seq = goalPos.y + 2.0f > pos.y ? 1 : 4;
    if (aiFlags & 0x200000)
        return 3;
    if (!(aiFlags & 0x1000))
        return seq;
    if (((animSYSTEM_INST *)inst->link)->groupBase == 2)
        return seq;
    if (enemy.IsDead())
        return seq;
    if (!(aiFlags & 0x10000000) || (aiFlags & 0x80000008))
        return seq;
    m3dV p, e;
    inst->GetPos(&p);
    enemy.GetPos(&e);
    float dx = p.x - goalPos.x, dz = p.z - goalPos.z;
    if (m3dSqrt(dx * dx + dz * dz) < 5.0f && fabsf(p.y - goalPos.y) < 3.0f)
        return 2;
    dx = p.x - e.x;
    dz = p.z - e.z;
    if (m3dSqrt(dx * dx + dz * dz) < 20.0f && e.y + 3.0f < p.y)
        return 2;
    return seq;
}

// v12 0x00439CC0  (feb 0x004338A0)
void aiBIRD::ProcessActionFrame()
{
    if (inst->IsActionFrame(2, 0)) {
        ThrowEgg();
        attackDelay = 10.0f;
        aiFlags = (aiFlags & ~0x3000u) | 1;
    }
}

// v12 0x00439D00  (feb 0x00433910)
// The attack starts when its time has come and the enemy is there; the dive within 5 m (flat) of the point ahead
// of it, or after 9 s; it ends after 15 s (or aiFlags 4).
void aiBIRD::ProcessAttack()
{
    if (!(aiFlags & 0x1000) && attackDelay < 0.1f && enemy.IsActive()) {
        attackTime = 0;
        aiFlags |= 0x1000;
    }
    if (!(aiFlags & 0x1000))
        return;
    if (!(aiFlags & 0x2000)) {
        m3dV p;
        inst->GetPos(&p);
        int dive = 1;
        if (attackTime <= 9.0f) {
            float dx = p.x - goalPos.x, dz = p.z - goalPos.z;
            dive = m3dSqrt(dx * dx + dz * dz) < 5.0f;
        }
        if (dive)
            aiFlags |= 0x2000;
    }
    if ((aiFlags & 0x1000) && (attackTime > 15.0f || (aiFlags & 4))) {
        attackDelay = 10.0f;
        aiFlags = (aiFlags & ~0x3000u) | 1;
    }
}

// v12 0x00439EA0  (feb 0x00433AB0)
void aiBIRD::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&attackDelay, gsElapsedTime);
    m3dUpdateTimeField(&timer57B, gsElapsedTime);
    if (m3dUpdateTimeField(&settleTimer, gsElapsedTime)) {
        settleTimer = m3dRandRange(1.5f, 2.5f);
        SettleSurface();
    }
    if (aiFlags & 0x1000)
        attackTime += gsElapsedTime;
    if (m3dUpdateTimeField(&wanderTimer, gsElapsedTime))
        aiFlags |= 1;
}

// v12 0x00439F40  (feb 0x00433B50)
// An egg from "L3_2" when the enemy is within 90 degrees of its facing: thrown at it at 17 m/s flat, 10 down; 10
// damage over 4 m (general 4), a 0.3 sphere, 7 s, 10 bounces.
void aiBIRD::ThrowEgg()
{
    pjlPJL_INIT in;
    memset(&in, 0, sizeof(in));
    in.phys.packId = 0;
    in.phys.tag = 0x40;
    in.phys.size = 0x55;
    in.phys.vel[0] = m3dVZero.x;
    in.phys.vel[1] = m3dVZero.y;
    in.phys.vel[2] = m3dVZero.z;
    in.phys.accel[0] = m3dVZero.x;
    in.phys.accel[1] = m3dVZero.y;
    in.phys.accel[2] = m3dVZero.z;
    in.phys.radius = 0.1f;
    in.phys.lifetime = 2000000.0f;
    in.phys.maxHits = 1;
    in.damage = -1.0f;
    in.radius = 0;
    in.weaponType = -1;
    in.isGeneral = -1;
    in.ownerId = 0xFFFFFFFF;
    in.termDelay = -1.0f;
    in.explosive = 1;
    animCREATE_DATA cd;
    inst->Validate(4);
    objOBJ *o = objFindName(inst->root, (char *)"L3_2");
    if (!o)
        return;
    in.phys.radius = 0.3f;
    in.phys.lifetime = 7.0f;
    in.phys.cdtMode = 2;
    in.phys.friction = 0;
    in.phys.bounce = 0;
    in.phys.maxHits = 10;
    in.damage = 10.0f;
    in.radius = 4.0f;
    in.isGeneral = 4;
    in.ownerId = (u32)mpId;
    m3dV org, d, e;
    o->GetOrigin(&org);
    inst->GetDir(&d);
    enemy.GetPos(&e);
    m3dV to = { e.x - org.x, e.y - org.y, e.z - org.z };
    if (!(fabsf(m3xzAngleVector(&d, &to)) <= 90.0f))
        return;
    in.phys.accel[0] = m3dVUnitY.x * -10.0f;
    in.phys.accel[1] = m3dVUnitY.y * -10.0f;
    in.phys.accel[2] = m3dVUnitY.z * -10.0f;
    cgmGetThrowSpeed_XZ(&org, &e, 17.0f, in.phys.accel[1], (m3dV *)in.phys.vel);
    memcpy(&cd.matr, o->ltm, sizeof(cd.matr));
    struct { pjlPJL_INIT i; u32 extra; } blk;           // (0x55 bytes: a zero dword after the init)
    blk.i = in;
    blk.extra = 0;
    entCreate(gsScenePtr, (char *)"bird_egg", (char *)"bird_egg", &cd, &blk, 0);
}

// v12 0x0043A180  (feb 0x00433DA0)
// The first bounce of its fall (1.2); a sequence begun: FLY 0x10, THROW_EGG 0x20, DEATH 4.
void aiBIRD::FillSoundFlags()
{
    if ((aiFlags & 0x8000) && passivePhys->nHits == 1)
        unk108 |= 8;
    if (inst->flags & 0x4000) {
        switch (inst->seq) {
        case 1: unk108 |= 0x10; break;
        case 2: unk108 |= 0x20; break;
        case 3: unk108 |= 4; break;
        }
    }
}

// v12 0x0043A200  (feb 0x00433DF0)
void aiBIRD::PlaySounds()
{
    u32 f = unk108;
    if (f & 4)
        voice->Play(&birdSnd[BSND_DEATH], 0);
    else if (f & 1)
        voice->Play(&birdSnd[BSND_APPEAR], 0);
    else if (f & 0x20)
        voice->Play(&birdSnd[BSND_EGG_THROW], 0);
    else if (f & 2)
        voice->Play(&birdSnd[BSND_HIT], 0);
    else if (f & 0x10)
        voice->Play(&birdSnd[BSND_FLY], 0);
    else if (inst->seq != 1 && voice->IsPlay(&birdSnd[BSND_FLY]))
        voice->Play((sncSOUND_DESCR *)0, 0);            // the wings stop
    if (unk108 & 8) {
        m3dV p;
        inst->GetPos(&p);
        gsSysSound->Play(&birdSnd[BSND_FALL_GROUND], &p);
    }
    aiNPC::PlaySounds();
}

// v12 0x0043A300  (feb 0x00433EF0)
int aiBIRD::IsAnimSeqGoodForFallback()
{
    return inst->seq != 3;
}

// v12 0x0043B900 (shared)
void aiBIRD::GetPlayerTargetPos(m3dV *out)
{
    enemy.GetPos(out);
    out->y += 0.01f;
}

// v12 0x0045C600 / 0x0045C610 (shared)
int aiBIRD::GetLayer3State()
{
    return (int)birdFlags;
}

void aiBIRD::SetLayer3State(int s)
{
    birdFlags = (u32)s;
}
