// ai_glad_mace.cpp -- the mace gladiator ("ai_glad_mace") and its flaming mace ("pjl_mace_glad") (Will Rock 1.2,
// v12.exe; feb obj ai_glad_mace.obj)
//
// Glowing hands while he holds a mace (two emitters on "emitter_left/right", restarted when a new mace appears in
// them, THROW frame 3); throws the mace in an arc (30 m/s) at where the planner says the enemy will be, burning
// (12 thin / 15 fat, all round 3 m); slashes up close with either swing (15 within 1 m, none past 1.3 m). Frozen:
// the maces and the glow gone.
// PLACEHOLDER (quick rebuild): pjlMACE flies as the template projectile with its trail and its explosion; its own
// ProcessINIT / ProcessFRAME (v12 0x0044B240..0x0044B7F0: the spinning "rot_center", the fly sound, the hit by
// coming near the player: GenerateCDTUponHittingPlayer) are not rebuilt yet.

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../gs/gs_mp_server.h"
#include "../../include/game_shared/projectile.h"

void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int skip);  // v12 0x005130D0
struct wrsfxSCORCHMARK {
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
                                                        // v12 0x00467DE0 (feb 0x0045DED0)
};

static txmTEXTURE *maceTexTrail;                        // v12 0x0064C008 aiGLAD_MACE::pTexTrail "sfx_glad_mace_trail"
static txmTEXTURE *maceTexScorch;                       // v12 0x0064BFFC ::pTexScorchMark "sfx_ax_scorch_mark"
static txmTEXTURE *maceTexGlow;                         // v12 0x0064C004 ::pTexGlow "sfx_glad_mace_appear_glow"
static const float maceRadius[2] = { 0.8f, 0.6f };     // v12 0x0062E14C fat, thin
static const float maceDamage[2] = { 15.0f, 12.0f };   // v12 0x0062E154

static m3dSPL_LINEAR1D *maceSpl(int n)
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

// ---------------------------------------------------------------------------------------------- the glow, the trail

class aiGLAD_MACE_APPEAR_GLOW : public partEMITTER_PHYS { public: aiGLAD_MACE_APPEAR_GLOW(float s); };
class pjlMACE_TRAIL : public partEMITTER_PHYS { public: pjlMACE_TRAIL(float s); };

// v12 0x0044AD10  (feb 0x00442E40)
// The new mace's glow in the hand: 10 a second from 0.1 to 1.2 s, 0.15 (+-0.05) s each, 2.25 x s (+-0.25 x s),
// fading, turning 30 a second, standing still.
aiGLAD_MACE_APPEAR_GLOW::aiGLAD_MACE_APPEAR_GLOW(float s) : partEMITTER_PHYS(10.0f, 0.2f)
{
    m3dSPL_LINEAR1D *in = maceSpl(4);
    in->SetKp(0, 0.1f, 0.0f);
    in->SetKp(1, 0.13333334f, 10.0f);
    in->SetKp(2, 0.93333334f, 10.0f);
    in->SetKp(3, 1.2f, 0.0f);
    SetIntensity(0.0f, in);
    SetTimeLifePart(0.15f, 0.05f);
    SetSize(s * 2.25f, s * 0.25f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = maceSpl(2);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(maceTexGlow);
    SetTextureRotAnim(180.5f, 179.5f, 30.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedA = 0;
    speedARand = 0;
    speedASpl = 0;
    flags |= 0x409;
    flags |= 0x800000;
}

// (feb 0x00444550)
// The flying mace's fire: 150 a second, 0.325 (+-0.125) s each, 2 x s, fading, turning -450 a second, from within
// 2 x s at s, 0.1 of the mace's speed.
pjlMACE_TRAIL::pjlMACE_TRAIL(float s) : partEMITTER_PHYS(150.0f, 0.45f)
{
    SetIntensity(150.0f, 0);
    SetTimeLifePart(0.325f, 0.125f);
    float b = s + s;
    m3dSPL_LINEAR1D *sz = maceSpl(2);
    sz->SetKp(0, 0.0f, b);
    sz->SetKp(1, 1.0f, b);
    SetSize(b, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = maceSpl(2);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(maceTexTrail);
    SetTextureRotAnim(180.5f, 179.5f, -450.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spread = b;
    spreadSpl = 0;
    spreadMin = 0;
    speedA = s;
    speedARand = 0;
    speedASpl = 0;
    speedK = 0.1f;
    speedKRand = 0;
    flags |= 0x619;
    SetDegFarParams(7.0f, 30.0f, 4.0f, 0.6f);
    SetDegNearParams(4.0f, 0.1f);
}

// ---------------------------------------------------------------------------------------------- the mace

struct pjlMACE_SFX {                                    // 8 bytes, at pjlPJL +0x16F
    partEMITTER_PHYS *trail;
    objOBJ           *trailObj;                         // "trail_emitter"
    pjlMACE_SFX() : trail(0), trailObj(0) {}
    // (feb 0x00443510)
    int StartSFX(physPHYS *p, pjlPJL_INIT *)
    {
        if (!(trail = new pjlMACE_TRAIL(0.1f)))
            return 0;
        if (!(trailObj = objFindName(p->inst->root, (char *)"trail_emitter")))
            return 0;
        p->inst->Validate(4);
        m3dV o;
        trailObj->GetOrigin(&o);
        trail->SetPosDirSpeed(&o, 0, (m3dV *)p->math->vel);
        trail->StartEmission();
        return 1;
    }
    void UpdateSFX(physPHYS *p)
    {
        if (!trail)
            return;
        p->inst->Validate(4);
        m3dV o;
        trailObj->GetOrigin(&o);
        trail->SetPosDirSpeed(&o, 0, (m3dV *)p->math->vel);
    }
    void StopSFX(physPHYS *)
    {
        if (trail) {
            trail->StopEmission();
            trail = 0;
        }
    }
};

#pragma pack(push, 1)
class pjlMACE : public pjlNPC_PJL<pjlMACE_SFX> {      // v12 size 0x1E0 (feb 0x1C4), vtable v12 0x005FE680
public:
    u8     _1bc[4];
    u32    maceFlags;                                   // +0x1C0 1 landed
    int    size;                                        // +0x1C4 0 fat, 1 thin
    float  spin;                                        // +0x1C8
    u8     _1cc[0x1E0 - 0x1CC];
    pjlMACE() { maceFlags = 0; size = 0; spin = 0; }
    static entENTITY *__fastcall Create(animINST *);    // v12 0x0044AF70 (feb 0x004430A0)
    void ShowCollisSFX(pjlEV_CDT *ev);                  // v12 0x0044B7F0 (feb 0x00443A10)
};
#pragma pack(pop)

entENTITY *__fastcall pjlMACE::Create(animINST *)
{
    return new pjlMACE;
}

// v12 0x0044B7F0  (feb 0x00443A10)
// Landed: the explosion (0.1) standing on the surface hit, and a scorch mark (0.75) in the light there.
void pjlMACE::ShowCollisSFX(pjlEV_CDT *ev)
{
    animCREATE_DATA cd;
    maceFlags |= 1;
    cd.matr._MakeLCS2WCS_VY(&ev->pos, &ev->normal);
    float sc = 0.1f;
    entCreate(gsScenePtr, (char *)"sfx_glad_mace_expl", (char *)"sfx_glad_mace_expl", &cd, &sc, 0);
    wrsfxSCORCHMARK::Add(0.75f, 10.0f, maceTexScorch, &ev->pos, ev->color, 0x5343524D, 0, 0);   // 'SCRM'
}

// v12 0x0044AEE0  (feb 0x00443010)
int __fastcall aiGladMacePjlInit()
{
    if (!entRegisterClass((char *)"pjl_mace_glad", 0x4D414345, 0x32, pjlMACE::Create, 0, 1))
        return 0;
    if (!(maceTexTrail = txmManager->Add((char *)"sfx_glad_mace_trail", 0x40003, 1)))
        return 0;
    if (!(maceTexScorch = txmManager->Add((char *)"sfx_ax_scorch_mark", 0x40003, 1)))
        return 0;
    return (maceTexGlow = txmManager->Add((char *)"sfx_glad_mace_appear_glow", 0x40003, 1)) != 0;
}

// ---------------------------------------------------------------------------------------------- the gladiator

// v12 0x0044A9D0  (feb 0x00442B10)
entENTITY *__fastcall aiGLAD_MACE::Create(animINST *)
{
    aiGLAD_MACE *g = new aiGLAD_MACE;                   // operator new(0x5CB), aiNPC ctor
    if (!g)
        return 0;
    g->wrFlags = 0;
    g->bloodObj = 0;
    g->glowL = 0;
    g->emitterL = 0;
    g->glowR = 0;
    g->emitterR = 0;
    g->unk98 = 0xB;
    return g;
}

// v12 0x0044AA30  (feb 0x00442B70)
// The two hand glows, "emitter_left/right", the 12 fire planes ("pPlane1..12": additive, no shadow), 12 hp.
int aiGLAD_MACE::ProcessINIT(void *init)
{
    if (!aiGLAD_GENERIC::ProcessINIT(init))
        return 0;
    if (!(glowL = new aiGLAD_MACE_APPEAR_GLOW(0.2f)))
        return 0;
    if (!(glowR = new aiGLAD_MACE_APPEAR_GLOW(0.2f)))
        return 0;
    if (!(emitterL = objFindName(inst->root, (char *)"emitter_left")))
        return 0;
    if (!(emitterR = objFindName(inst->root, (char *)"emitter_right")))
        return 0;
    objFindName(inst->root, (char *)"pPlane21");
    for (int i = 1; i <= 12; i++) {
        char n[0x20];
        sprintf(n, "pPlane%d", i);
        objOBJ *o = objFindName(inst->root, n);
        if (!o)
            break;
        o->_SetStateRendNo(0x80);
        o->_SetStateRendYes(0x68);
        o->SetStateYes(0x30);
    }
    maxHealth = 12.0f;
    navArriveDist = 2.0f;
    fallbackAngle = 100.0f;
    hitSeq = 19;
    radCDT = 1.0f;
    heightCDT = 1.7f;
    return 1;
}

// v12 0x0044ABB0  (feb 0x00442CE0): the glows gone (the base's ProcessTERM not called, as in the game)
void aiGLAD_MACE::ProcessTERM()
{
    if (glowR) {
        glowR->__vecDelDtor(1);
        glowR = 0;
    }
    if (glowL) {
        glowL->__vecDelDtor(1);
        glowL = 0;
    }
}

// v12 0x0044ABF0  (feb 0x00442D20)
// As the knife's (14..21 mace sequences, the fat body's 8 later), slashing with either swing at random.
int aiGLAD_MACE::EvalAnimGoalSeq()
{
    u32 f = aiFlags;
    int idle = ((f & 1) || (f & 0x10000)) ? 14 : 15;
    int seq;
    if (f & 0x200000)
        seq = 20;
    else if (enemy.IsDead())
        seq = 14;
    else {
        u32 g = gladFlags;
        if (g & 0x200)
            seq = 16;
        else if (aiFlags & 0x10000)
            seq = 14;
        else if (g & 0x30) {
            u32 a = aiFlags;
            seq = ((a & 0x200000) ? 1 : 0) + 19;
            gladFlags = (g & ~0x30u) | 8;
            hitLight = (~a >> 21) & 1;
            hitSeq = seq;
        } else if (g & 8)
            seq = hitSeq;
        else if (!(idleTimer <= 1e-6f))
            seq = 14;
        else if (aiFlags & 0x1000)
            seq = 17;
        else if (g & 4)
            seq = m3dRandSelect(18, 21);
        else
            seq = (aiFlags & 4) ? 14 : idle;
    }
    if (gladFlags & 0x40)
        seq += 8;
    return seq;
}

// v12 0x0044B890  (feb 0x00443AC0)
// As the knife's throw at 30 m/s.
void aiGLAD_MACE::AimMaceToPlayer(m3dV *org, pjlPJL_INIT *in)
{
    m3dV d, v, m;
    inst->GetDir(&d);
    d.y = 0;
    if (!(wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(this, 30.0f, &m)) {
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
    float t = dist * 0.033333335f;                      // (dist / 30)
    in->phys.vel[0] = v.x * 30.0f;
    in->phys.vel[1] = v.y * 30.0f;
    in->phys.vel[2] = v.z * 30.0f;
    in->phys.vel[1] = ((m.y - org->y) - t * t * -4.5f) / t;
    in->phys.accel[0] = m3dVUnitY.x * -9.0f;
    in->phys.accel[1] = m3dVUnitY.y * -9.0f;
    in->phys.accel[2] = m3dVUnitY.z * -9.0f;
}

// v12 0x0044BAF0  (feb 0x00443D20)
// A mace from the "AttackFocus": the body's model ("pjl_gladXL_mace" / "pjl_gladM_mace"), 1000 s, two hits,
// bouncing 0.5 / 0.5, burning 15 / 12 all round 3 m.
void aiGLAD_MACE::LaunchOneProjectile()
{
    struct { pjlPJL_INIT i; u32 extra; } blk;           // (0x55 bytes sent)
    pjlPJL_INIT &in = blk.i;
    memset(&blk, 0, sizeof(blk));
    in.phys.packId = 0;
    in.phys.tag = 0x40;
    in.phys.size = 0x55;
    in.phys.lifetime = 2000000.0f;
    in.phys.maxHits = 1;
    in.weaponType = -1;
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
        tpl = "pjl_gladXL_mace";
        thin = 0;
    } else {
        tpl = "pjl_gladM_mace";
        thin = 1;
    }
    in.phys.radius = maceRadius[thin];
    in.damage = maceDamage[thin];
    in.radius = 3.0f;
    in.isGeneral = 4;
    in.phys.lifetime = 1000.0f;
    in.phys.maxHits = 2;
    in.phys.friction = 0.5f;
    in.phys.bounce = 0.5f;
    in.ownerId = (u32)mpId;
    AimMaceToPlayer(&org, &in);
    pjlMACE *k = (pjlMACE *)entCreate(gsScenePtr, (char *)"pjl_mace_glad", (char *)tpl, &cd, &blk, 0);
    if (!k)
        return;
    k->size = thin;
    k->spin = maceRadius[thin] + 0.5f;
    gsEffApplyLMChar(k, &floor, 0);
}

// v12 0x0044BCB0
// A new mace in the hands (0x100): both glows started again; the glows follow the hands.
void aiGLAD_MACE::Slot33()
{
    if (gladFlags & 0x100) {
        glowL->StopEmission();
        glowL->StartEmission();
        glowR->StopEmission();
        glowR->StartEmission();
        gladFlags &= ~0x100u;
    }
    glowL->SetCS((m3dMATR *)emitterL->ltm);
    glowR->SetCS((m3dMATR *)emitterR->ltm);
}

// v12 0x0044BD30
// Frozen (0x800000), once: the glows stopped, the maces and their fire planes hidden.
void aiGLAD_MACE::ProcessFRAME()
{
    aiNPC_WR::ProcessFRAME();
    if (!(aiFlags & 0x800000) || (gladFlags & 0x400))
        return;
    if (glowL)
        glowL->StopEmission();
    if (glowR)
        glowR->StopEmission();
    static const char *hide[14] = { "BULAVA_L_&lod", "BULAVA_R_&lod", "pPlane1", "pPlane2", "pPlane3", "pPlane4",
                                    "pPlane5", "pPlane6", "pPlane7", "pPlane8", "pPlane9", "pPlane10", "pPlane11",
                                    "pPlane12" };
    for (int i = 0; i < 14; i++)
        if (objOBJ *o = objFindName(inst->root, (char *)hide[i]))
            o->SetStateProcYes(8, 0);
    gladFlags |= 0x400;
}

// v12 0x0044BF70  (feb 0x00444190)
// THROW: maces on frames 0 and 1, frame 2 a throw used up (none left: 7 s), frame 3 a new mace in the hands;
// SLASH1 / SLASH2: the blow (15 within 1 m flat of the focus, none past 1.3 m) and 2 s; SHOT / DEATH frame 1 end a
// light reaction; APPEAR frame 0 ends the appearing.
void aiGLAD_MACE::ProcessActionFrame()
{
    dmgGENERAL g;
    int b = (gladFlags & 0x40) ? 8 : 0;
    inst->IsActionFrame(b + 15, 0);
    inst->IsActionFrame(b + 15, 1);
    if (inst->IsActionFrame(b + 17, 0))
        LaunchOneProjectile();
    if (inst->IsActionFrame(b + 17, 1)) {
        LaunchOneProjectile();
        gladFlags &= ~2u;
    }
    if (inst->IsActionFrame(b + 17, 2)) {
        gladFlags |= 2;
        aiFlags &= ~0x1000u;
        if (throwsLeft > 0 && --throwsLeft == 0)
            throwTimer = 7.0f;
    }
    if (inst->IsActionFrame(b + 17, 3))
        gladFlags |= 0x100;
    if (inst->IsActionFrame(b + 18, 0) || inst->IsActionFrame(b + 18, 1) || inst->IsActionFrame(b + 21, 0) ||
        inst->IsActionFrame(b + 21, 1)) {
        m3dV f, t;
        ((m3dMATR *)attackFocus->ltm)->GetOrigin(&f);
        GetPlayerTargetPos(&t);
        float dx = t.x - f.x, dz = t.z - f.z;
        float d = m3dSqrt(dx * dx + dz * dz);
        g.pos = f;
        if (!(d > 1.0f))
            g.amount = 15.0f;
        else if (!(d <= 1.3f))
            g.amount = 0;
        else
            g.amount = 15.0f - (d - 1.0f) * 50.0f;
        if (g.amount > 0.1f && enemy.ent)
            msgSystem.SendMsg(enemy.ent, 0x4C6, &g, this);
        throwTimer = 2.0f;
    }
    inst->IsActionFrame(b + 19, 0);
    if (inst->IsActionFrame(b + 19, 1) && hitLight)
        gladFlags &= ~8u;
    inst->IsActionFrame(b + 20, 0);
    if (inst->IsActionFrame(b + 20, 1) && hitLight)
        gladFlags &= ~8u;
    if (inst->IsActionFrame(b + 16, 0))
        gladFlags &= ~0x200u;
}
