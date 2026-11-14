// sfx_gefest.cpp -- Hephaestus's effects (Will Rock 1.2, v12.exe; feb obj sfx_gefest.obj)
//
// "sfx_gefest_hit": where his hammer lands -- a lightning flash and a glow flattened on the ground, a burst of
// sparks from its "emitter"; gone in 1.5 s. "sfx_gefest_expl": where a thrown coal lands -- burning bits, smoke from
// its "_emitter*", a flattened glow; gone in 1.3 s. Both at the size the creator passes (its init data: a float).
// (1.2's code is feb's, at other addresses: the effects grew with entENTITY.)

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"

static txmTEXTURE *hitTexLgtn;                          // v12 wrsfxGEFEST_HIT::pTexLgtn   "sfx_gefest_hit_lgtn"
static txmTEXTURE *hitTexGlow;                          // v12 wrsfxGEFEST_HIT::pTexGlow   "sfx_gefest_hit_glow"
static txmTEXTURE *hitTexSparks;                        // v12 wrsfxGEFEST_HIT::pTexSparks "sfx_gefest_hit_spark"
static txmTEXTURE *explTexParts;                        // v12 wrsfxGEFEST_EXPL::pTexParts "sfx_gefest_carbons"
static txmTEXTURE *explTexSmoke;                        // v12 wrsfxGEFEST_EXPL::pTexSmoke "sfx_gefest_smoke"
static txmTEXTURE *explTexGlow;                         // v12 wrsfxGEFEST_EXPL::pTexGlow  "sfx_gefest_carbon_expl"

static m3dSPL_LINEAR1D *sfxSpl(int n)
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

// a burst's brightness: up to `peak` in 1/30 s, held to t1, gone at t2
static m3dSPL_LINEAR1D *sfxBurst(float peak, float t1, float t2)
{
    m3dSPL_LINEAR1D *s = sfxSpl(4);
    s->SetKp(0, 0.0f, 0.0f);
    s->SetKp(1, 0.033333335f, peak);
    s->SetKp(2, t1, peak);
    s->SetKp(3, t2, 0.0f);
    return s;
}

// fading from a to 0
static m3dSPL_LINEAR1D *sfxFade(float a)
{
    m3dSPL_LINEAR1D *s = sfxSpl(2);
    s->SetKp(0, 0.0f, a);
    s->SetKp(1, 1.0f, 0.0f);
    return s;
}

// ---- the hammer's hit ---------------------------------------------------------------------------------------------

class wrsfxGEFEST_HIT_LGTN : public partEMITTER_PHYS { public: wrsfxGEFEST_HIT_LGTN(float s); };
class wrsfxGEFEST_HIT_GLOW : public partEMITTER_PHYS { public: wrsfxGEFEST_HIT_GLOW(float s); };
class wrsfxGEFEST_HIT_SPARKS : public partEMITTER_PHYS { public: wrsfxGEFEST_HIT_SPARKS(float s); };

// v12 0x00475250  (feb 0x0046B570)
// The flash: 10 at once (in 0.2 s), 0.15 (+-0.05) s each, 22.5 x s growing to 28.5 x s (+-17.5 x s), fading,
// flattened (y 0.3), seen from 8 m.
wrsfxGEFEST_HIT_LGTN::wrsfxGEFEST_HIT_LGTN(float s) : partEMITTER_PHYS(10, 37.5f, 0.2f)
{
    SetIntensity(0.0f, sfxBurst(37.5f, 0.16666667f, 0.2f));
    SetTimeLifePart(0.15f, 0.05f);
    m3dSPL_LINEAR1D *sz = sfxSpl(2);
    float a = s * 22.5f;
    sz->SetKp(0, 0.0f, a);
    sz->SetKp(1, 1.0f, s * 28.5f);
    SetSize(a, s * 17.5f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, sfxFade(255.0f));
    SetTexture(hitTexLgtn);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedARand = s * 0.0f;
    speedA = s;
    speedASpl = 0;
    flags |= 0x409;
    flags |= 0x1000000;
    SetScaleCSVal(1.0f, 0.3f, 1.0f);
    SetDegNearParams(8.0f, 0.01f);
}

// v12 0x00475490  (feb 0x0046B7B0)
// The glow: 15 at once (in 0.1 s), 0.3 (+-0.1) s each, 45 x s growing to 51 x s (+-35 x s), fading, flattened.
wrsfxGEFEST_HIT_GLOW::wrsfxGEFEST_HIT_GLOW(float s) : partEMITTER_PHYS(15, 125.0f, 0.4f)
{
    SetIntensity(0.0f, sfxBurst(125.0f, 0.06666667f, 0.1f));
    SetTimeLifePart(0.3f, 0.1f);
    m3dSPL_LINEAR1D *sz = sfxSpl(2);
    float a = s * 45.0f;
    sz->SetKp(0, 0.0f, a);
    sz->SetKp(1, 1.0f, s * 51.0f);
    SetSize(a, s * 35.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, sfxFade(255.0f));
    SetTexture(hitTexGlow);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedA = s;
    speedARand = 0;
    speedASpl = 0;
    flags |= 0x409;
    flags |= 0x1000000;
    SetScaleCSVal(1.0f, 0.3f, 1.0f);
    SetDegNearParams(8.0f, 0.01f);
}

// v12 0x004756D0  (feb 0x0046B9F0)
// The sparks: 100 at once (in 0.13 s), 0.2 (+-0.1) s each, 5 x s shrinking, streaks (along their speed), thrown
// 70 x s (+-50 x s) out and 7 x s (+-5 x s) along the ground, from all over "emitter".
wrsfxGEFEST_HIT_SPARKS::wrsfxGEFEST_HIT_SPARKS(float s) : partEMITTER_PHYS(100, 750.0f, 0.3f)
{
    SetIntensity(0.0f, sfxBurst(750.0f, 0.1f, 0.13333334f));
    SetTimeLifePart(0.2f, 0.1f);
    m3dSPL_LINEAR1D *sz = sfxSpl(2);
    float a = s * 5.0f;
    sz->SetKp(0, 0.0f, a);
    sz->SetKp(1, 1.0f, s * 0.0f);
    SetSize(a, 0.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, sfxFade(255.0f));
    SetTexture(hitTexSparks);
    flags |= 0x100000;
    SetTextureScale(0.1f, 0.0f, 1.0f, 0.0f);
    speedASpl = 0;
    speedBRand = a;
    speedBSpl = 0;
    speedA = s * 70.0f;
    speedARand = s * 50.0f;
    speedB = s * 7.0f;
    flags |= 9;
    flags |= 0xA000;
    SetDegNearParams(8.0f, 0.01f);
}

// v12 0x00475930  (feb 0x0046BC50)
// 1 (init, the size): scaled, lit by its place; the flash and the glow where it stands (flattened as it is), the
// sparks over "emitter" (hidden); 1.5 s. 2: the emitters gone. 1000: its time.
BOOL32 wrsfxGEFEST_HIT::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id == 1000) {
        if (m3dUpdateTimeField(&timer, gsElapsedTime))
            msgSystem.PostMsg(this, 2, this);
    } else if (id == 2) {
        if (sparks) {
            sparks->__vecDelDtor(1);
            sparks = 0;
        }
        if (glow) {
            glow->__vecDelDtor(1);
            glow = 0;
        }
        if (lgtn) {
            lgtn->__vecDelDtor(1);
            lgtn = 0;
        }
    } else if (id == 1) {
        float s = *(float *)data;
        inst->Scale(s, s, s, 0);
        inst->flags |= 0x10;
        inst->root->SetStateYes(0x10);
        m3dV p;
        inst->GetPos(&p);
        m3dMATR *m = (m3dMATR *)inst->matr;
        objOBJ *em = objFindName(inst->root, (char *)"emitter");
        if (!em)
            return 0;
        em->SetStateProcYes(8, 0);
        if (!(lgtn = new wrsfxGEFEST_HIT_LGTN(s)))
            return 0;
        lgtn->SetPosDirSpeed(&p, 0, 0);
        lgtn->SetScaleCS(m);
        if (!(glow = new wrsfxGEFEST_HIT_GLOW(s)))
            return 0;
        glow->SetPosDirSpeed(&p, 0, 0);
        glow->SetScaleCS(m);
        if (!(sparks = new wrsfxGEFEST_HIT_SPARKS(s)))
            return 0;
        if (!partSetAreaEmissionObj(sparks, em, -1, 0))
            return 0;
        lgtn->StartEmission();
        glow->StartEmission();
        sparks->StartEmission();
        timer = 1.5f;
    }
    return 0;
}

// v12 0x004763D0  (feb 0x0046C6F0)
entENTITY *__fastcall wrsfxGEFEST_HIT::Create(animINST *)
{
    wrsfxGEFEST_HIT *e = new wrsfxGEFEST_HIT;
    if (!e)
        return 0;
    e->lgtn = 0;
    e->glow = 0;
    e->sparks = 0;
    e->timer = 0;
    return e;
}

// ---- a coal's explosion -------------------------------------------------------------------------------------------

class wrsfxGEFEST_EXPL_PARTS : public partEMITTER_PHYS { public: wrsfxGEFEST_EXPL_PARTS(float s); };
class wrsfxGEFEST_EXPL_SMOKE : public partEMITTER_PHYS { public: wrsfxGEFEST_EXPL_SMOKE(float s); };
class wrsfxGEFEST_EXPL_GLOW : public partEMITTER_PHYS { public: wrsfxGEFEST_EXPL_GLOW(float s); };

// v12 0x00475B50  (feb 0x0046BE70)
// Burning bits: 90 at once (in 0.1 s), 0.55 (+-0.25) s each, 0.35 x s shrinking from 0.25 x s, thrown 10 x s
// (+-5 x s) up and 3 x s (+-1.5 x s) across, falling at 9.8 x s, slowed 2 a second.
wrsfxGEFEST_EXPL_PARTS::wrsfxGEFEST_EXPL_PARTS(float s) : partEMITTER_PHYS(90, 900.0f, 0.8f)
{
    SetIntensity(0.0f, sfxBurst(900.0f, 0.06666667f, 0.1f));
    SetTimeLifePart(0.55f, 0.25f);
    m3dSPL_LINEAR1D *sz = sfxSpl(2);
    sz->SetKp(0, 0.0f, s * 0.25f);
    sz->SetKp(1, 1.0f, s * 0.0f);
    SetSize(s * 0.35f, 0.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, 0);
    SetTexture(explTexParts);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    speedASpl = 0;
    speedBSpl = 0;
    gravity.x = 0;
    speedA = s * 10.0f;
    gravity.z = 0;
    drag = 2.0f;
    speedARand = s * 5.0f;
    speedB = s * 3.0f;
    speedBRand = s * 1.5f;
    gravity.y = s * -9.8f;
    flags |= 9;
    flags |= 0xA000;
    SetDegNearParams(3.0f, 0.3f);
}

// v12 0x00475D50  (feb 0x0046C070)
// Smoke: 20 at once (in 0.1 s), 0.5 (+-0.2) s each, s (+-0.5 x s) big, fading from 127, turning 90 a second,
// rising 10 x s (+-5 x s), 3 x s (+-1.5 x s) across, slowed 4 a second.
wrsfxGEFEST_EXPL_SMOKE::wrsfxGEFEST_EXPL_SMOKE(float s) : partEMITTER_PHYS(20, 200.0f, 0.7f)
{
    SetIntensity(0.0f, sfxBurst(200.0f, 0.06666667f, 0.1f));
    SetTimeLifePart(0.5f, 0.2f);
    SetSize(s, s * 0.5f, 0, 1.0f);
    SetColor(0xFFFFFFFF, 0, sfxFade(127.5f));
    SetTexture(explTexSmoke);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 90.0f);
    flags |= 4;
    speedASpl = 0;
    speedBSpl = 0;
    drag = 4.0f;
    speedA = s * 10.0f;
    speedARand = s * 5.0f;
    speedB = s * 3.0f;
    speedBRand = s * 1.5f;
    flags |= 9;
    flags |= 0xA000;
    SetDegNearParams(3.0f, 0.3f);
}

// v12 0x00475F40  (feb 0x0046C260)
// The flash: 8 at once (in 0.1 s), 0.3 (+-0.1) s each, 2.4 x s growing to 4 x s, fading, flattened (0.5, 1, 0.5).
wrsfxGEFEST_EXPL_GLOW::wrsfxGEFEST_EXPL_GLOW(float s) : partEMITTER_PHYS(8, 60.0f, 0.4f)
{
    SetIntensity(60.0f, sfxBurst(60.0f, 0.06666667f, 0.1f));
    SetTimeLifePart(0.3f, 0.1f);
    m3dSPL_LINEAR1D *sz = sfxSpl(2);
    sz->SetKp(0, 0.0f, s * 2.4f);
    float a = s * 4.0f;
    sz->SetKp(1, 1.0f, a);
    SetSize(a, 0.0f, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, sfxFade(255.0f));
    SetTexture(explTexGlow);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedA = s;
    speedARand = 0;
    speedASpl = 0;
    flags |= 9;
    flags |= 0x1000000;
    SetScaleCSVal(0.5f, 1.0f, 0.5f);
    SetDegNearParams(3.0f, 0.3f);
}

// v12 0x00476170  (feb 0x0046C490)
// 1 (init, the size): scaled, lit by its place; the bits and the smoke over "_emitter*", the glow where it stands
// (its matrix without the scale); 1.3 s. 2: the emitters gone. 1000: its time.
BOOL32 wrsfxGEFEST_EXPL::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id == 1000) {
        if (m3dUpdateTimeField(&timer, gsElapsedTime))
            msgSystem.PostMsg(this, 2, this);
    } else if (id == 2) {
        if (glow) {
            glow->__vecDelDtor(1);
            glow = 0;
        }
        if (smoke) {
            smoke->__vecDelDtor(1);
            smoke = 0;
        }
        if (parts) {
            parts->__vecDelDtor(1);
            parts = 0;
        }
    } else if (id == 1) {
        float s = *(float *)data;
        inst->Scale(s, s, s, 0);
        inst->flags |= 0x10;
        inst->root->SetStateYes(0x10);
        inst->Validate(4);
        m3dV p;
        inst->GetPos(&p);
        if (!(parts = new wrsfxGEFEST_EXPL_PARTS(s)))
            return 0;
        if (!(smoke = new wrsfxGEFEST_EXPL_SMOKE(s)))
            return 0;
        objOBJ *em = objFindNameN(inst->root, (char *)"_emitter", (int)strlen("_emitter"));
        if (!em)
            return 0;
        if (!partSetAreaEmissionObj(parts, em, -1, 0))
            return 0;
        if (!partSetAreaEmissionObj(smoke, em, -1, 0))
            return 0;
        if (!(glow = new wrsfxGEFEST_EXPL_GLOW(s)))
            return 0;
        glow->SetPosDirSpeed(&p, 0, 0);
        m3dMATR m = *(m3dMATR *)inst->matr;
        m.RemoveScale();
        glow->SetScaleCS(&m);
        parts->StartEmission();
        smoke->StartEmission();
        glow->StartEmission();
        timer = 1.3f;
    }
    return 0;
}

// v12 0x00476450  (feb 0x0046C770)
entENTITY *__fastcall wrsfxGEFEST_EXPL::Create(animINST *)
{
    wrsfxGEFEST_EXPL *e = new wrsfxGEFEST_EXPL;
    if (!e)
        return 0;
    e->parts = 0;
    e->smoke = 0;
    e->glow = 0;
    e->timer = 0;
    return e;
}

// v12 0x00475140  (feb 0x0046B460)
int __fastcall wrsfxGefestInit()
{
    if (!entRegisterClass((char *)"sfx_gefest_hit", 0x24534658, 0x50, wrsfxGEFEST_HIT::Create, 0, 1))
        return 0;
    if (!(hitTexLgtn = txmManager->Add((char *)"sfx_gefest_hit_lgtn", 0x40003, 1)))
        return 0;
    if (!(hitTexGlow = txmManager->Add((char *)"sfx_gefest_hit_glow", 0x40003, 1)))
        return 0;
    if (!(hitTexSparks = txmManager->Add((char *)"sfx_gefest_hit_spark", 0x40003, 1)))
        return 0;
    if (!entRegisterClass((char *)"sfx_gefest_expl", 0x24534658, 0x50, wrsfxGEFEST_EXPL::Create, 0, 1))
        return 0;
    if (!(explTexParts = txmManager->Add((char *)"sfx_gefest_carbons", 0x40003, 1)))
        return 0;
    if (!(explTexSmoke = txmManager->Add((char *)"sfx_gefest_smoke", 0x40003, 1)))
        return 0;
    return (explTexGlow = txmManager->Add((char *)"sfx_gefest_carbon_expl", 0x40003, 1)) != 0;
}
