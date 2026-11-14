// sfx_barrel.cpp -- a barrel's explosion ("sfx_barrel_expl") (Will Rock 1.2, v12.exe; feb obj sfx_barrel.obj)
//
// Made by a breaking barrel (dstrDESTROY, sfx_destroy.cpp) at its size: fire thrown out from its "center" (each
// spark trailing flames), a flash, burning bits; the model's animation plays; gone in 1.2 s.

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"

static txmTEXTURE *barTexFire;                          // v12 wrsfxBARREL_EXPLODE::pTexFire "sfx_cerber_pjl_burn"
static txmTEXTURE *barTexGlow;                          // v12 wrsfxBARREL_EXPLODE::pTexGlow "sfx_barrel_expl_glow"
static txmTEXTURE *barTexPart;                          // v12 wrsfxBARREL_EXPLODE::pTexPart "sfx_barrel_expl_part"

static m3dSPL_LINEAR1D *barSpl(int n)
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
static m3dSPL_LINEAR1D *barBurst(float t0, float t1, float t2, float t3, float v)
{
    m3dSPL_LINEAR1D *s = barSpl(4);
    s->SetKp(0, t0, 0.0f);
    s->SetKp(1, t1, v);
    s->SetKp(2, t2, v);
    s->SetKp(3, t3, 0.0f);
    return s;
}

static m3dSPL_LINEAR1D *barFade()
{
    m3dSPL_LINEAR1D *s = barSpl(2);
    s->SetKp(0, 0.0f, 255.0f);
    s->SetKp(1, 1.0f, 0.0f);
    return s;
}

class wrsfxBARREL_EXPLODE_FIRE_SRC : public partEMITTER_PHYS { public: wrsfxBARREL_EXPLODE_FIRE_SRC(float s); };
class wrsfxBARREL_EXPLODE_FIRE : public partEMITTER_PHYS { public: wrsfxBARREL_EXPLODE_FIRE(float s); };
class wrsfxBARREL_EXPLODE_GLOW : public partEMITTER_PHYS { public: wrsfxBARREL_EXPLODE_GLOW(float s); };
class wrsfxBARREL_EXPLODE_PARTS : public partEMITTER_PHYS { public: wrsfxBARREL_EXPLODE_PARTS(float s); };

// v12 0x004698A0  (feb 0x0045FAD0)
// 10 carriers at once (in 0.1 s), 0.1 s each, any direction at 6 x s (+-3 x s), from within s.
wrsfxBARREL_EXPLODE_FIRE_SRC::wrsfxBARREL_EXPLODE_FIRE_SRC(float s) : partEMITTER_PHYS(10, 100.0f, 0.1f)
{
    SetIntensity(0.0f, barBurst(0.0f, 0.033333335f, 0.06666667f, 0.1f, 100.0f));
    SetTimeLifePart(0.1f, 0.0f);
    cone = 180.0f;
    coneSpl = 0;
    spread = s;
    spreadSpl = 0;
    spreadMin = 0;
    speedA = s * 6.0f;
    speedASpl = 0;
    speedARand = s * 3.0f;
    flags |= 0x400000;
}

// v12 0x004699C0  (feb 0x0045FBF0)
// Each carrier's flames: 66.7 a second (0.8 s window), 0.5 (+-0.3) s each, s (+-0.5 x s) growing 0.75 -> 1.5,
// fading, turning 120 a second, 2 x s (+-s) any way plus 0.4 of the carrier's speed, slowed 2 a second.
wrsfxBARREL_EXPLODE_FIRE::wrsfxBARREL_EXPLODE_FIRE(float s) : partEMITTER_PHYS(66.666664f, 0.8f)
{
    SetIntensity(6.6666665f, 0);
    SetTimeLifePart(0.5f, 0.3f);
    m3dSPL_LINEAR1D *sz = barSpl(2);
    sz->SetKp(0, 0.0f, s * 0.75f);
    sz->SetKp(1, 1.0f, s * 1.5f);
    SetSize(s, s * 0.5f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, barFade());
    SetTexture(barTexFire);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 120.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedARand = s;
    speedA = s + s;
    speedASpl = 0;
    speedK = 0.4f;
    speedKRand = 0;
    drag = 2.0f;
    flags |= 0x409;
    flags |= 0x4000409;
    SetDegNearParams(3.0f, 0.1f);
}

// v12 0x00469B90  (feb 0x0045FDC0)
// The flash: 30 (bright from 1/30 to 2/15 s), 0.15 (+-0.05) s each, 1.5 x s (+-s) growing to 4.5 x s, fading.
wrsfxBARREL_EXPLODE_GLOW::wrsfxBARREL_EXPLODE_GLOW(float s) : partEMITTER_PHYS(30, 200.0f, 0.2f)
{
    SetIntensity(200.0f, barBurst(0.033333335f, 0.06666667f, 0.1f, 0.13333334f, 200.0f));
    SetTimeLifePart(0.15f, 0.05f);
    m3dSPL_LINEAR1D *sz = barSpl(2);
    float a = s * 1.5f;
    sz->SetKp(0, 0.0f, a);
    sz->SetKp(1, 1.0f, s * 4.5f);
    SetSize(a, s, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, barFade());
    SetTexture(barTexGlow);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    flags |= 0x409;
    SetDegNearParams(3.0f, 0.1f);
}

// v12 0x00469DD0  (feb 0x00460000)
// Burning bits: 30 at once (in 0.1 s), 0.35 (+-0.15) s each, 0.3 x s (+-0.2 x s), fading, two frames, spinning,
// any way at 4 x s (+-2 x s) from within 3 x s.
wrsfxBARREL_EXPLODE_PARTS::wrsfxBARREL_EXPLODE_PARTS(float s) : partEMITTER_PHYS(30, 300.0f, 0.5f)
{
    SetIntensity(300.0f, barBurst(0.0f, 0.033333335f, 0.06666667f, 0.1f, 300.0f));
    SetTimeLifePart(0.35f, 0.15f);
    SetSize(s * 0.3f, s * 0.2f, 0, 1.0f);
    SetColor(0xFFFFFFFF, 0, barFade());
    SetTexture(barTexPart);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 300.0f);
    SetTextureFrameAnim(2, 1, 0.0f, 2.0f);
    flags |= 6;
    cone = 180.0f;
    coneSpl = 0;
    spreadSpl = 0;
    spread = s * 3.0f;
    spreadMin = 0;
    speedASpl = 0;
    speedA = s * 4.0f;
    speedARand = s + s;
    flags |= 0x408;
}

// v12 0x0046A000  (feb 0x00460230)
// 1 (init, the size): scaled, lit by its place; the fire (carriers with their flames), the flash and the bits from
// "center"; its animation from the start; 1.2 s. 2: the emitters gone. 1000: its time, its animation.
BOOL32 wrsfxBARREL_EXPLODE::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id == 1) {
        float s = *(float *)data;
        inst->Scale(s, s, s, 0);
        inst->flags |= 0x10;
        inst->root->SetStateYes(0x10);
        inst->Validate(4);
        objOBJ *c = objFindName(inst->root, (char *)"center");
        if (!c)
            return 0;
        m3dV p;
        c->GetOrigin(&p);
        if (!(fireSrc = new wrsfxBARREL_EXPLODE_FIRE_SRC(s)))
            return 0;
        fireSrc->SetPosDirSpeed(&p, 0, 0);
        if (!(fire = new wrsfxBARREL_EXPLODE_FIRE(s)))
            return 0;
        fireSrc->AttachEmitter(fire, 0, 0);
        fireSrc->flags |= 0x40000;
        if (!(glow = new wrsfxBARREL_EXPLODE_GLOW(s)))
            return 0;
        glow->SetPosDirSpeed(&p, 0, 0);
        if (!(parts = new wrsfxBARREL_EXPLODE_PARTS(s)))
            return 0;
        parts->SetPosDirSpeed(&p, 0, 0);
        fireSrc->StartEmission();
        glow->StartEmission();
        parts->StartEmission();
        inst->SetAnimSeq(0, 0.0f);
        timer = 1.2f;
    } else if (id == 2) {
        if (parts) {
            parts->__vecDelDtor(1);
            parts = 0;
        }
        if (glow) {
            glow->__vecDelDtor(1);
            glow = 0;
        }
        if (fire) {
            fire->__vecDelDtor(1);
            fire = 0;
        }
        if (fireSrc) {
            fireSrc->__vecDelDtor(1);
            fireSrc = 0;
        }
    } else if (id == 1000) {
        if (m3dUpdateTimeField(&timer, gsElapsedTime))
            msgSystem.PostMsg(this, 2, this);
        inst->AdvanceTime(gsElapsedTime, 1, 0);
    }
    return 0;
}

// v12 0x00469820  (feb 0x0045FA50)
entENTITY *__fastcall wrsfxBARREL_EXPLODE::Create(animINST *)
{
    wrsfxBARREL_EXPLODE *e = new wrsfxBARREL_EXPLODE;
    if (!e)
        return 0;
    e->fireSrc = 0;
    e->fire = 0;
    e->glow = 0;
    e->parts = 0;
    e->timer = 0;
    return e;
}

// v12 0x00469790  (feb 0x0045F9C0)
int __fastcall wrsfxBarrelInit()
{
    if (!entRegisterClass((char *)"sfx_barrel_expl", 0x24534658, 0x50, wrsfxBARREL_EXPLODE::Create, 0, 1))
        return 0;
    if (!(barTexFire = txmManager->Add((char *)"sfx_cerber_pjl_burn", 0x40003, 1)))
        return 0;
    if (!(barTexGlow = txmManager->Add((char *)"sfx_barrel_expl_glow", 0x40003, 1)))
        return 0;
    return (barTexPart = txmManager->Add((char *)"sfx_barrel_expl_part", 0x40003, 1)) != 0;
}
