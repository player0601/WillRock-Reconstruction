// ai_glad_knife.cpp -- the knife gladiator ("ai_glad_knife") and its knife ("pjl_knife_glad") (Will Rock 1.2, v12.exe;
// feb obj ai_glad_knife.obj)
//
// Throws knives (THROW's action frames 0 and 1: two a throw) in an arc at where the planner says the enemy will be
// (within its aim, else straight ahead; beyond its shooting range: ahead too), then slashes up close: 10 within 1 m
// of its "AttackFocus" (flat), none past 1.3 m. Out of throws: 7 s till it takes them back.
// PLACEHOLDER (quick rebuild): pjlKNIFE flies as a plain projectile (the template's flight, hit and damage); its own
// ProcessINIT / ProcessFRAME / ProcessEV_CDT / ShowCollisSFX (v12 0x004497F0..0x00449E90: the spinning "rot_center",
// the fly sound, the trail) are not rebuilt yet.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../../include/game_shared/projectile.h"

void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int skip);  // v12 0x005130D0

static const float knifeRadius[2] = { 0.8f, 0.6f };    // v12 0x0062E0D0 fat (XL), thin (M)

// ---------------------------------------------------------------------------------------------- the knife

struct pjlKNIFE_SFX {                                   // PLACEHOLDER: no trail (8 bytes, at pjlPJL +0x16F)
    void *a, *b;
    pjlKNIFE_SFX() : a(0), b(0) {}
    int  StartSFX(physPHYS *, pjlPJL_INIT *) { return 1; }
    void UpdateSFX(physPHYS *) {}
    void StopSFX(physPHYS *) {}
};

#pragma pack(push, 1)
class pjlKNIFE : public pjlNPC_PJL<pjlKNIFE_SFX> {    // v12 size 0x1DC (feb 0x1C0), vtable v12 0x005FE320
public:
    u8     _1bc[8];
    int    size;                                        // +0x1C4 0 fat, 1 thin (LaunchOneProjectile)
    float  spin;                                        // +0x1C8 the knife's radius + 0.5
    u8     _1cc[0x1DC - 0x1CC];
    pjlKNIFE() { size = 0; spin = 0; }
    static entENTITY *__fastcall Create(animINST *);    // v12 0x00449710 (feb 0x004418E0)
};
#pragma pack(pop)

entENTITY *__fastcall pjlKNIFE::Create(animINST *)
{
    return new pjlKNIFE;
}

// v12 0x004496E0  (feb 0x004418B0) exact
int __fastcall aiGladKnifePjlInit()
{
    return entRegisterClass((char *)"pjl_knife_glad", 0x4B4E4946, 0x32, pjlKNIFE::Create, 0, 1) != 0;
}

// ---------------------------------------------------------------------------------------------- the gladiator

// v12 0x00449510  (feb 0x004416E0)
entENTITY *__fastcall aiGLAD_KNIFE::Create(animINST *)
{
    aiGLAD_KNIFE *g = new aiGLAD_KNIFE;                 // operator new(0x5BB), aiNPC ctor
    if (!g)
        return 0;
    g->wrFlags = 0;
    g->bloodObj = 0;
    g->unk98 = 0xB;
    return g;
}

// v12 0x00449560  (feb 0x00441730)
int aiGLAD_KNIFE::ProcessINIT(void *init)
{
    if (!aiGLAD_GENERIC::ProcessINIT(init))
        return 0;
    fallbackAngle = 90.0f;
    hitSeq = 5;
    return 1;
}

// v12 0x004495A0  (feb 0x00441770): the knife body's own skin
void aiGLAD_KNIFE::ProcessINIT_LEVEL()
{
    if (inst && inst->root)
        inst->root->ReplaceTex((char *)"gladiator_01a", (char *)"gladiator_01b");
    aiNPC_WR::ProcessINIT_LEVEL();
}

// v12 0x004495D0  (feb 0x004417A0)
// Dead: DEATH; no enemy / idling after spawning / held: IDLE; appearing: APPEAR; hit: SHOT (light) or DEATH-like
// heavy (held till done); throwing: THROW; slashing: SLASH; else RUN (IDLE when stopped). The fat body's 7 later.
int aiGLAD_KNIFE::EvalAnimGoalSeq()
{
    u32 f = aiFlags;
    int seq = (!(f & 1) && !(f & 0x10000)) ? 1 : 0;
    if (f & 0x200000)
        seq = 6;
    else if (enemy.IsDead())
        seq = 0;
    else {
        u32 g = gladFlags;
        if (g & 0x200)
            seq = 2;
        else if (aiFlags & 0x10000)
            seq = 0;
        else if (g & 0x30) {
            u32 a = aiFlags;
            seq = ((a & 0x200000) ? 1 : 0) + 5;
            gladFlags = (g & ~0x30u) | 8;
            hitLight = (~a >> 21) & 1;
            hitSeq = seq;
        } else if (g & 8)
            seq = hitSeq;
        else if (!(idleTimer <= 1e-6f))
            seq = 0;
        else if (aiFlags & 0x1000)
            seq = 3;
        else if (g & 4)
            seq = 4;
        else
            seq = (aiFlags & 4) ? 0 : seq;
    }
    if (gladFlags & 0x40)
        seq += 7;
    return seq;
}

// v12 0x00449EB0  (feb 0x00442170)
// The throw: at 35 m/s along a direction -- to the planner's meeting point (1.5 up) when that is within its aim,
// else straight ahead (also beyond its shooting range) -- lobbed to arrive at the point's height under -9 gravity.
void aiGLAD_KNIFE::AimKnifeToPlayer(m3dV *org, pjlPJL_INIT *in)
{
    m3dV d, v, m;
    inst->GetDir(&d);
    d.y = 0;
    if (!(wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(this, 35.0f, &m)) {
        m.y += 1.5f;
        v.x = m.x - org->x;
        v.y = 0;
        v.z = m.z - org->z;
        m3dNormalize(&v);
        float a = fabsf(m3xzAngleVector(&v, &d));
        if (!(a <= aimAngle))
            v = d;
    } else {
        enemy.GetPos(&m);
        v = d;
    }
    float dx = org->x - m.x, dz = org->z - m.z;
    float dist = m3dSqrt(dx * dx + dz * dz);
    if (!(dist <= shootRange))
        v = d;
    float t = dist * 0.028571429f;                      // (dist / 35)
    in->phys.vel[0] = v.x * 35.0f;
    in->phys.vel[1] = v.y * 35.0f;
    in->phys.vel[2] = v.z * 35.0f;
    in->phys.vel[1] = ((m.y - org->y) - t * t * -4.5f) / t;
    in->phys.accel[0] = m3dVUnitY.x * -9.0f;
    in->phys.accel[1] = m3dVUnitY.y * -9.0f;
    in->phys.accel[2] = m3dVUnitY.z * -9.0f;
}

// v12 0x0044A110  (feb 0x004423D0)
// A knife from the "AttackFocus" (its matrix): the body's model ("pjl_gladXL_knife" fat, "pjl_gladM_knife" thin), 1000 s,
// two hits, bouncing 0.5 / 0.5, 10 damage (fat) or 9 (thin) within 1.5; lit like the gladiator.
void aiGLAD_KNIFE::LaunchOneProjectile()
{
    struct { pjlPJL_INIT i; u32 extra; } blk;           // (0x55 bytes sent)
    pjlPJL_INIT &in = blk.i;
    memset(&blk, 0, sizeof(blk));
    in.phys.packId = 0;
    in.phys.tag = 0x40;
    in.phys.size = 0x55;
    in.phys.radius = 0.1f;
    in.phys.lifetime = 2000000.0f;
    in.phys.maxHits = 1;
    in.weaponType = -1;
    in.ownerId = (u32)-1;
    in.termDelay = -1.0f;
    in.explosive = 1;
    animCREATE_DATA cd;
    inst->Validate(4);
    m3dV org;
    attackFocus->GetOrigin(&org);
    memcpy(&cd.matr, attackFocus->ltm, sizeof(cd.matr));
    int thin;
    const char *tpl;
    if (gladFlags & 0x40) {
        tpl = "pjl_gladXL_knife";
        thin = 0;
    } else {
        tpl = "pjl_gladM_knife";
        thin = 1;
    }
    in.phys.radius = knifeRadius[thin];
    in.phys.lifetime = 1000.0f;
    in.phys.maxHits = 2;
    in.phys.friction = 0.5f;
    in.phys.bounce = 0.5f;
    in.explosive = 0;
    in.ownerId = (u32)mpId;
    in.isGeneral = 4;
    in.radius = 1.5f;
    in.damage = 10.0f;
    if (thin)
        in.damage = 9.0f;
    AimKnifeToPlayer(&org, &in);
    pjlKNIFE *k = (pjlKNIFE *)entCreate(gsScenePtr, (char *)"pjl_knife_glad", (char *)tpl, &cd, &blk, 0);
    if (!k)
        return;
    k->size = thin;
    k->spin = knifeRadius[thin] + 0.5f;
    gsEffApplyLMChar(k, &floor, 0);
}

// v12 0x0044A2D0  (feb 0x00442580)
// THROW: knives on frames 0 and 1 (attacking stops till frame 2), frame 2 a throw used up (none left: 7 s till
// more); SLASH: the cut (10 within 1 m flat of the focus, none past 1.3 m) and 2 s; SHOT / DEATH frame 1 end a light
// reaction; APPEAR frame 0 ends the appearing.
void aiGLAD_KNIFE::ProcessActionFrame()
{
    dmgGENERAL g;
    int b = (gladFlags & 0x40) ? 7 : 0;
    inst->IsActionFrame(b + 1, 0);
    inst->IsActionFrame(b + 1, 1);
    if (inst->IsActionFrame(b + 3, 0))
        LaunchOneProjectile();
    if (inst->IsActionFrame(b + 3, 1)) {
        LaunchOneProjectile();
        gladFlags &= ~2u;
    }
    if (inst->IsActionFrame(b + 3, 2)) {
        gladFlags |= 2;
        aiFlags &= ~0x1000u;
        if (throwsLeft > 0 && --throwsLeft == 0)
            throwTimer = 7.0f;
    }
    if (inst->IsActionFrame(b + 4, 0) || inst->IsActionFrame(b + 4, 1)) {
        m3dV f, t;
        ((m3dMATR *)attackFocus->ltm)->GetOrigin(&f);
        GetPlayerTargetPos(&t);
        float dx = t.x - f.x, dz = t.z - f.z;
        float d = m3dSqrt(dx * dx + dz * dz);
        g.pos = f;
        if (!(d > 1.0f))
            g.amount = 10.0f;
        else if (!(d <= 1.3f))
            g.amount = 0;
        else
            g.amount = 10.0f - (d - 1.0f) * 33.33334f;
        if (g.amount > 0.1f && enemy.ent)
            msgSystem.SendMsg(enemy.ent, 0x4C6, &g, this);
        throwTimer = 2.0f;
    }
    inst->IsActionFrame(b + 5, 0);
    if (inst->IsActionFrame(b + 5, 1) && hitLight)
        gladFlags &= ~8u;
    inst->IsActionFrame(b + 6, 0);
    if (inst->IsActionFrame(b + 6, 1) && hitLight)
        gladFlags &= ~8u;
    if (inst->IsActionFrame(b + 2, 0))
        gladFlags &= ~0x200u;
}
