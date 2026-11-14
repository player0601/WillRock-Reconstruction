// sfx_cerber.cpp -- the cerberus' projectile bursts (Will Rock 1.2, v12.exe; feb obj sfx_cerber.obj)
//
// "sfx_cerber_pjl_expl": where a fireball bursts -- a quick spray of sparks, flames over its "emitter" (hidden), a
// glow. "sfx_cerber_pjl_acid_expl" (no model): where the acid lands -- green smoke and a splash glow. Both at the size
// the creator passes (a float), and both gone at once: the emitters, started, are left to die out by themselves.

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"

static txmTEXTURE *frbTexSparks;                        // v12 0x0064ED2C wrsfxCERBER_FIREBALL_EXPL::pTexSparks
static txmTEXTURE *frbTexBurn;                          // v12 0x0064ED24 wrsfxCERBER_FIREBALL_EXPL::pTexBurn
static txmTEXTURE *frbTexGlow;                          // v12 0x0064ED34 wrsfxCERBER_FIREBALL_EXPL::pTexGlow
static txmTEXTURE *acdTexSmoke;                         // v12 0x0064ED30 wrsfxCERBER_ACID_EXPL::pTexSmoke
static txmTEXTURE *acdTexGlow;                          // v12 0x0064ED28 wrsfxCERBER_ACID_EXPL::pTexGlow
static float cerbSfxEps2;                               // v12 0x0064ED20 (1e-6 squared, v12 0x0046DD20; unused)
static struct cerbSfxStatic { cerbSfxStatic() { cerbSfxEps2 = 1e-6f * 1e-6f; } } cerbSfxStaticInit;

static m3dSPL_LINEAR1D *cbSpl(int n)
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
// a burst's intensity: up to `peak` in 1/30 s, held to 1/15, gone at 0.1
static m3dSPL_LINEAR1D *cbBurst(float peak)
{
    m3dSPL_LINEAR1D *s = cbSpl(4);
    s->SetKp(0, 0.0f, 0.0f);
    s->SetKp(1, 0.033333335f, peak);
    s->SetKp(2, 0.06666667f, peak);
    s->SetKp(3, 0.1f, 0.0f);
    return s;
}
static m3dSPL_LINEAR1D *cbLine(float a, float b)
{
    m3dSPL_LINEAR1D *s = cbSpl(2);
    s->SetKp(0, 0.0f, a);
    s->SetKp(1, 1.0f, b);
    return s;
}

class wrsfxCERBER_FIREBALL_EXPL_SPARKS : public partEMITTER_PHYS { public: wrsfxCERBER_FIREBALL_EXPL_SPARKS(float s); };
class wrsfxCERBER_FIREBALL_EXPL_BURN : public partEMITTER_PHYS { public: wrsfxCERBER_FIREBALL_EXPL_BURN(float s); };
class wrsfxCERBER_FIREBALL_EXPL_GLOW : public partEMITTER_PHYS { public: wrsfxCERBER_FIREBALL_EXPL_GLOW(float s); };
class wrsfxCERBER_ACID_EXPL_SMOKE : public partEMITTER_PHYS { public: wrsfxCERBER_ACID_EXPL_SMOKE(float s); };
class wrsfxCERBER_ACID_EXPL_GLOW : public partEMITTER_PHYS { public: wrsfxCERBER_ACID_EXPL_GLOW(float s); };

// v12 0x0046D680  (feb 0x00463810): 75 sparks in a 0.1 s burst, fast (10x size), fading
wrsfxCERBER_FIREBALL_EXPL_SPARKS::wrsfxCERBER_FIREBALL_EXPL_SPARKS(float s) : partEMITTER_PHYS(75, 750.0f, 0.7f)
{
    SetIntensity(0, cbBurst(750.0f));
    SetTimeLifePart(0.5f, 0.2f);
    SetSize(s * 6.0f, 0.0f, cbLine(s * 3.0f, s * 6.0f), 1.0f);
    SetColor(0xFFFFFFFF, 0, cbLine(200.175f, 0.0f));
    SetTexture(frbTexSparks);
    flags |= 0x100000;
    SetTextureScale(0.083333336f, 0.0f, 1.0f, 0.0f);
    cone = 180.0f;
    coneSpl = 0;
    speedASpl = 0;
    speedA = s * 10.0f;
    speedARand = s * 10.0f;
    flags |= 0x409;
    SetDegNearParams(3.0f, 0.1f);
    SetDegFarParams(10.0f, 30.0f, 2.0f, 0.1f);
    SetAnimEmission(1.2f, 0.0f);
    flags |= 0x200;
}

// v12 0x0046D8C0  (feb 0x00463A50): 20 flames over the emitter object, rising and spreading, slowing down
wrsfxCERBER_FIREBALL_EXPL_BURN::wrsfxCERBER_FIREBALL_EXPL_BURN(float s) : partEMITTER_PHYS(20, 200.0f, 0.9f)
{
    SetIntensity(0, cbBurst(200.0f));
    SetTimeLifePart(0.7f, 0.2f);
    SetSize(s * 4.5f, s * 1.5f, 0, 1.0f);
    SetColor(0xFFFFFFFF, 0, cbLine(255.0f, 0.0f));
    SetTexture(frbTexBurn);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 180.0f);
    flags |= 4;
    spreadSpl = 0;
    spreadMin = 0;
    speedARand = s * 4.5f;
    spread = s * 3.0f;
    speedASpl = 0;
    speedB = s * 1.5f;
    speedBSpl = 0;
    drag = s * 0.1f;
    speedA = s * 9.0f;
    speedBRand = s * 0.75f;
    flags |= 0x409;
    flags |= 0xA000;
    SetDegNearParams(4.0f, 0.1f);
    SetAnimEmission(1.2f, 0.0f);
    flags |= 0x200;
}

// v12 0x0046DAE0  (feb 0x00463C50): the glow, 35x size shrinking to 17.5
wrsfxCERBER_FIREBALL_EXPL_GLOW::wrsfxCERBER_FIREBALL_EXPL_GLOW(float s) : partEMITTER_PHYS(75, 75.0f, 0.8f)
{
    SetIntensity(0, cbBurst(75.0f));
    SetTimeLifePart(0.6f, 0.2f);
    SetSize(s * 35.0f, s * 17.5f, cbLine(s * 8.75f, s * 17.5f), 1.0f);
    SetColor(0xFFFFFFFF, 0, cbLine(255.0f, 0.0f));
    SetTexture(frbTexGlow);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedARand = s * 0.0f;
    speedA = s;
    speedASpl = 0;
    flags |= 0x409;
    SetDegNearParams(4.0f, 0.1f);
    SetAnimEmission(1.2f, 0.0f);
    flags |= 0x200;
}

// v12 0x0046DEE0  (feb 0x004640C0): the acid's smoke (colour from the texture: white, no alpha in the colour)
wrsfxCERBER_ACID_EXPL_SMOKE::wrsfxCERBER_ACID_EXPL_SMOKE(float s) : partEMITTER_PHYS(25, 225.0f, 0.8f)
{
    SetIntensity(0, cbBurst(225.0f));
    SetTimeLifePart(0.55f, 0.25f);
    SetSize(s * 1.25f, s * 0.25f, 0, 1.0f);
    SetColor(0x00FFFFFF, 0, cbLine(255.0f, 0.0f));
    SetTexture(acdTexSmoke);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 120.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spreadSpl = 0;
    spread = s * 1.5f;
    spreadMin = 0;
    speedA = s;
    speedASpl = 0;
    speedARand = s * 0.5f;
    flags |= 0x409;
    SetDegNearParams(3.0f, 0.1f);
    SetAnimEmission(1.2f, 0.0f);
    flags |= 0x200;
}

// v12 0x0046E0E0: the splash glow
wrsfxCERBER_ACID_EXPL_GLOW::wrsfxCERBER_ACID_EXPL_GLOW(float s) : partEMITTER_PHYS(12, 100.0f, 0.4f)
{
    SetIntensity(0, cbBurst(100.0f));
    SetTimeLifePart(0.3f, 0.1f);
    SetSize(s * 2.25f, s * 0.75f, cbLine(s * 2.25f, s * 3.6f), 1.0f);
    SetColor(0x00FFFFFF, 0, cbLine(255.0f, 0.0f));
    SetTexture(acdTexGlow);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    flags |= 0x409;
    SetDegNearParams(3.0f, 0.1f);
    SetAnimEmission(1.2f, 0.0f);
    flags |= 0x200;
}

// v12 0x0046DD40  (feb 0x00463EB0)
// 1 (init, the size): scaled, lit by its place; the sparks and the glow where it stands, the flames over "emitter"
// (hidden); all started and let go; gone.
BOOL32 wrsfxCERBER_FIREBALL_EXPL::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id != 1)
        return 0;
    float s = *(float *)data;
    inst->Scale(s, s, s, 0);
    inst->flags |= 0x10;
    inst->root->SetStateYes(0x10);
    m3dV p;
    inst->GetPos(&p);
    objOBJ *em = objFindName(inst->root, (char *)"emitter");
    if (!em)
        return 0;
    em->SetStateProcYes(8, 0);
    if (!(sparks = new wrsfxCERBER_FIREBALL_EXPL_SPARKS(s)))
        return 0;
    sparks->SetPosDirSpeed(&p, 0, 0);
    if (!(burn = new wrsfxCERBER_FIREBALL_EXPL_BURN(s)))
        return 0;
    if (!partSetAreaEmissionObj(burn, em, -1, 0))
        return 0;
    if (!(glow = new wrsfxCERBER_FIREBALL_EXPL_GLOW(s)))
        return 0;
    glow->SetPosDirSpeed(&p, 0, 0);
    sparks->StartEmission();
    burn->StartEmission();
    glow->StartEmission();
    timer = 1.5f;
    glow = 0;
    burn = 0;
    sparks = 0;
    msgSystem.PostMsg(this, 2, this);
    return 0;
}

// v12 0x0046E430  (feb 0x00464620)
entENTITY *__fastcall wrsfxCERBER_FIREBALL_EXPL::Create(animINST *)
{
    wrsfxCERBER_FIREBALL_EXPL *e = new wrsfxCERBER_FIREBALL_EXPL;
    if (!e)
        return 0;
    e->sparks = 0;
    e->burn = 0;
    e->glow = 0;
    e->timer = 0;
    return e;
}

// v12 0x0046E320  (feb 0x00464500): 1 (init, the size): the smoke and the glow there, let go; gone
BOOL32 wrsfxCERBER_ACID_EXPL::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id != 1)
        return 0;
    float s = *(float *)data;
    inst->Scale(s, s, s, 0);
    inst->flags |= 0x10;
    m3dV p;
    inst->GetPos(&p);
    if (!(smoke = new wrsfxCERBER_ACID_EXPL_SMOKE(s)))
        return 0;
    smoke->SetPosDirSpeed(&p, 0, 0);
    if (!(glow = new wrsfxCERBER_ACID_EXPL_GLOW(s)))
        return 0;
    glow->SetPosDirSpeed(&p, 0, 0);
    smoke->StartEmission();
    glow->StartEmission();
    timer = 1.0f;
    smoke = 0;
    glow = 0;
    msgSystem.PostMsg(this, 2, this);
    return 0;
}

// v12 0x0046E4B0  (feb 0x004646A0)
entENTITY *__fastcall wrsfxCERBER_ACID_EXPL::Create(animINST *)
{
    wrsfxCERBER_ACID_EXPL *e = new wrsfxCERBER_ACID_EXPL;
    if (!e)
        return 0;
    e->smoke = 0;
    e->glow = 0;
    e->timer = 0;
    return e;
}

// v12 0x0046D590  (feb 0x00463720)
int __fastcall wrsfxCerberInit()
{
    if (!entRegisterClass((char *)"sfx_cerber_pjl_expl", 0x24534658, 0x50, wrsfxCERBER_FIREBALL_EXPL::Create, 0, 1))
        return 0;
    if (!(frbTexSparks = txmManager->Add((char *)"sfx_cerber_pjl_expl_sparks", 0x40003, 1)))
        return 0;
    if (!(frbTexBurn = txmManager->Add((char *)"sfx_cerber_pjl_burn", 0x40003, 1)))
        return 0;
    if (!(frbTexGlow = txmManager->Add((char *)"sfx_rat_expl_glow", 0x40003, 1)))
        return 0;
    if (!entRegisterClass((char *)"sfx_cerber_pjl_acid_expl", 0x24534658, 0x50, wrsfxCERBER_ACID_EXPL::Create, 0, 1))
        return 0;
    if (!(acdTexSmoke = txmManager->Add((char *)"sfx_cerber_pjl_acid_smoke", 0x40003, 1)))
        return 0;
    return (acdTexGlow = txmManager->Add((char *)"sfx_opp_acd_splash", 0x40003, 1)) != 0;
}

static_assert(sizeof(wrsfxCERBER_FIREBALL_EXPL) == 0x17B, "wrsfxCERBER_FIREBALL_EXPL");
static_assert(sizeof(wrsfxCERBER_ACID_EXPL) == 0x177, "wrsfxCERBER_ACID_EXPL");
