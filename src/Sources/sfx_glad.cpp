// sfx_glad.cpp -- the flaming mace's explosion ("sfx_glad_mace_expl") (Will Rock 1.2, v12.exe; feb obj sfx_glad.obj)
//
// Made where the gladiator's mace lands (pjlMACE::ShowCollisSFX) at its size: a burst of fire, a flash, sparks from
// the model's "sparks_emitter" and a second, dimmer fire; the model's animation plays; gone in 1 s.

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"

static txmTEXTURE *gmeTexBurn;                          // wrsfxGLAD_MACE_EXPL::pTexBurn "sfx_glad_mace_expl_burn"
static txmTEXTURE *gmeTexGlow;                          // ::pTexGlow "pwp_amg_pjl_expl_sparks"
static txmTEXTURE *gmeTexSparks;                        // ::pTexSparks "pwp_crb_pjl_sparks"
static txmTEXTURE *gmeTexBurn1;                         // ::pTexBurn1 "sfx_glad_mace_expl_burn1"

static m3dSPL_LINEAR1D *gmeSpl(int n)
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

// four keys: (t0, 0) (t1, v) (t2, v) (t3, 0)
static m3dSPL_LINEAR1D *gmeBurst(float t0, float t1, float t2, float t3, float v)
{
    m3dSPL_LINEAR1D *s = gmeSpl(4);
    s->SetKp(0, t0, 0.0f);
    s->SetKp(1, t1, v);
    s->SetKp(2, t2, v);
    s->SetKp(3, t3, 0.0f);
    return s;
}

static m3dSPL_LINEAR1D *gmeFade(float a)
{
    m3dSPL_LINEAR1D *s = gmeSpl(2);
    s->SetKp(0, 0.0f, a);
    s->SetKp(1, 1.0f, 0.0f);
    return s;
}

class wrsfxGLAD_MACE_EXPL_BURN : public partEMITTER_PHYS { public: wrsfxGLAD_MACE_EXPL_BURN(float s); };
class wrsfxGLAD_MACE_EXPL_GLOW : public partEMITTER_PHYS { public: wrsfxGLAD_MACE_EXPL_GLOW(float s); };
class wrsfxGLAD_MACE_EXPL_SPARKS : public partEMITTER_PHYS { public: wrsfxGLAD_MACE_EXPL_SPARKS(float s); };
class wrsfxGLAD_MACE_EXPL_BURN1 : public partEMITTER_PHYS { public: wrsfxGLAD_MACE_EXPL_BURN1(float s); };

// (feb 0x0046C8A0)
// 20 flames at once (in 0.1 s), 0.45 (+-0.15) s each, 9.5 x s (+-1.5 x s), fading, turning 120 a second, any way at
// 2 x s (+-s) from within 3 x s.
wrsfxGLAD_MACE_EXPL_BURN::wrsfxGLAD_MACE_EXPL_BURN(float s) : partEMITTER_PHYS(20, 200.0f, 0.6f)
{
    SetIntensity(0.0f, gmeBurst(0.0f, 0.033333335f, 0.06666667f, 0.1f, 200.0f));
    SetTimeLifePart(0.45f, 0.15f);
    SetSize(s * 9.5f, s * 1.5f, 0, 1.0f);
    SetColor(0xFFFFFFFF, 0, gmeFade(255.0f));
    SetTexture(gmeTexBurn);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 120.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spread = s * 3.0f;
    spreadSpl = 0;
    spreadMin = 0;
    speedARand = s;
    speedASpl = 0;
    speedA = s + s;
    flags |= 0x408;
    SetDegNearParams(3.0f, 0.1f);
    SetDegFarParams(7.0f, 30.0f, 4.0f, 0.6f);
}

// (feb 0x0046CAC0)
// The flash: 7 (in 0.1 s), 0.35 (+-0.25) s each, growing 5 x s -> 20 x s, fading.
wrsfxGLAD_MACE_EXPL_GLOW::wrsfxGLAD_MACE_EXPL_GLOW(float s) : partEMITTER_PHYS(7, 50.0f, 0.6f)
{
    SetIntensity(0.0f, gmeBurst(0.0f, 0.033333335f, 0.06666667f, 0.1f, 50.0f));
    SetTimeLifePart(0.35f, 0.25f);
    m3dSPL_LINEAR1D *sz = gmeSpl(2);
    sz->SetKp(0, 0.0f, s * 5.0f);
    float b = s * 20.0f;
    sz->SetKp(1, 1.0f, b);
    SetSize(b, 0.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, gmeFade(255.0f));
    SetTexture(gmeTexGlow);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedA = 0;
    speedARand = 0;
    speedASpl = 0;
    flags |= 0x408;
    SetDegNearParams(3.0f, 0.3f);
    SetDegFarParams(7.0f, 30.0f, 4.0f, 0.6f);
}

// (feb 0x0046CD00)
// 300 sparks (in 0.1 s), 0.45 (+-0.15) s each, shrinking from 0.75 x s, streaks, out of the emitter object at
// 15 x s (+-7.5 x s) and 3 x s (+-1.5 x s) across, falling at 5 x s.
wrsfxGLAD_MACE_EXPL_SPARKS::wrsfxGLAD_MACE_EXPL_SPARKS(float s) : partEMITTER_PHYS(300, 3000.0f, 0.6f)
{
    SetIntensity(0.0f, gmeBurst(0.0f, 0.033333335f, 0.06666667f, 0.1f, 3000.0f));
    SetTimeLifePart(0.45f, 0.15f);
    m3dSPL_LINEAR1D *sz = gmeSpl(2);
    float a = s * 0.75f;
    sz->SetKp(0, 0.0f, a);
    sz->SetKp(1, 1.0f, s * 0.0f);
    SetSize(a, 0.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, 0);
    SetTexture(gmeTexSparks);
    flags |= 0x100000;
    spreadSpl = 0;
    spreadMin = 0;
    speedASpl = 0;
    spread = s * 5.0f;
    speedBSpl = 0;
    gravity.x = 0;
    gravity.z = 0;
    speedA = s * 15.0f;
    speedARand = s * 7.5f;
    speedB = s * 3.0f;
    speedBRand = s * 1.5f;
    gravity.y = s * -5.0f;
    flags |= 0xA409;
    SetDegNearParams(3.0f, 0.1f);
    SetDegFarParams(7.0f, 30.0f, 4.0f, 0.6f);
}

// (feb 0x0046CF20)
// The second fire: 15 (from 1/30 to 2/15 s), 0.3 (+-0.1) s each, growing 5 x s -> 20 x s, fading from 127.5, thrown at s.
wrsfxGLAD_MACE_EXPL_BURN1::wrsfxGLAD_MACE_EXPL_BURN1(float s) : partEMITTER_PHYS(15, 100.0f, 0.4f)
{
    SetIntensity(0.0f, gmeBurst(0.033333335f, 0.06666667f, 0.1f, 0.13333334f, 100.0f));
    SetTimeLifePart(0.3f, 0.1f);
    m3dSPL_LINEAR1D *sz = gmeSpl(2);
    sz->SetKp(0, 0.0f, s * 5.0f);
    float b = s * 20.0f;
    sz->SetKp(1, 1.0f, b);
    SetSize(b, 0.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, gmeFade(127.5f));
    SetTexture(gmeTexBurn1);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedA = s;
    speedARand = 0;
    speedASpl = 0;
    flags |= 0x408;
    SetDegNearParams(3.0f, 0.1f);
    SetDegFarParams(7.0f, 30.0f, 4.0f, 0.6f);
}

// v12 0x00476E60  (feb 0x0046D180)
// 1 (init, the size): scaled; the four emitters at its place (the sparks from "sparks_emitter", hidden), its
// animation, 1 s. 2: the emitters gone. 1000: its time, its animation.
BOOL32 wrsfxGLAD_MACE_EXPL::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id == 1) {
        float s = *(float *)data;
        inst->Scale(s, s, s, 0);
        m3dV p;
        inst->GetPos(&p);
        if (!(burn = new wrsfxGLAD_MACE_EXPL_BURN(s)))
            return 0;
        burn->SetPosDirSpeed(&p, 0, 0);
        if (!(glow = new wrsfxGLAD_MACE_EXPL_GLOW(s)))
            return 0;
        glow->SetPosDirSpeed(&p, 0, 0);
        if (!(sparks = new wrsfxGLAD_MACE_EXPL_SPARKS(s)))
            return 0;
        objOBJ *e = objFindName(inst->root, (char *)"sparks_emitter");
        if (!e)
            return 0;
        e->SetStateProcYes(8, 0);
        if (!partSetAreaEmissionObj(sparks, e, -1, 0))
            return 0;
        if (!(burn1 = new wrsfxGLAD_MACE_EXPL_BURN1(s)))
            return 0;
        burn1->SetPosDirSpeed(&p, 0, 0);
        burn->StartEmission();
        glow->StartEmission();
        sparks->StartEmission();
        burn1->StartEmission();
        timer = 1.0f;
        inst->SetAnimSeq(0, 0.0f);
        inst->flags |= 0x10;
    } else if (id == 2) {
        if (burn1) {
            burn1->__vecDelDtor(1);
            burn1 = 0;
        }
        if (sparks) {
            sparks->__vecDelDtor(1);
            sparks = 0;
        }
        if (glow) {
            glow->__vecDelDtor(1);
            glow = 0;
        }
        if (burn) {
            burn->__vecDelDtor(1);
            burn = 0;
        }
    } else if (id == 1000) {
        if (m3dUpdateTimeField(&timer, gsElapsedTime))
            msgSystem.PostMsg(this, 2, this);
        inst->AdvanceTime(gsElapsedTime, 1, 0);
    }
    return 0;
}

// v12 0x004770E0  (feb 0x0046D400)
entENTITY *__fastcall wrsfxGLAD_MACE_EXPL::Create(animINST *)
{
    wrsfxGLAD_MACE_EXPL *e = new wrsfxGLAD_MACE_EXPL;
    if (!e)
        return 0;
    e->burn = 0;
    e->glow = 0;
    e->sparks = 0;
    e->burn1 = 0;
    e->timer = 0;
    return e;
}

// v12 0x004764D0  (feb 0x0046C7F0) exact
int __fastcall wrsfxGladInit()
{
    if (!entRegisterClass((char *)"sfx_glad_mace_expl", 0x24534658, 0x50, wrsfxGLAD_MACE_EXPL::Create, 0, 1))
        return 0;
    if (!(gmeTexBurn = txmManager->Add((char *)"sfx_glad_mace_expl_burn", 0x40003, 1)))
        return 0;
    if (!(gmeTexGlow = txmManager->Add((char *)"pwp_amg_pjl_expl_sparks", 0x40003, 1)))
        return 0;
    if (!(gmeTexSparks = txmManager->Add((char *)"pwp_crb_pjl_sparks", 0x40000, 1)))
        return 0;
    return (gmeTexBurn1 = txmManager->Add((char *)"sfx_glad_mace_expl_burn1", 0x40003, 1)) != 0;
}
