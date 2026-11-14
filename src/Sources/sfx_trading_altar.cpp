// sfx_trading_altar.cpp -- the effect of a sale at the trading altar (Will Rock 1.2, v12.exe; feb obj
// sfx_trading_altar.obj)
//
// v12 0x00483E90..0x004847C0. A "dynamic" that plays once for 5 s at the altar: a glow rising from "source_glow" and a
// trail at each of "plane01".."plane03". Its look is by the powerup (3 sets of textures).

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../m3d/m3d_spl.h"

wrsfxTRADING_ALTAR::TEX wrsfxTRADING_ALTAR::tex[3];     // v12 0x0064F16C

// the m3dSPL_LINEAR1D constructor inline (v12 0x00484005)
static m3dSPL_LINEAR1D *traSpl(int n)
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

// v12 0x00483E90  (feb 0x00479B00)
int __fastcall wrsfxTradingAltarInit()
{
    if (!entRegisterClass((char *)"sfx_trading_altar", 0x24534658, 0x50, wrsfxTRADING_ALTAR::Create, 0, 1))
        return 0;
    for (int i = 0; i < 3; i++) {
        char name[0x40];
        wrsfxTRADING_ALTAR::TEX *t = &wrsfxTRADING_ALTAR::tex[i];
        sprintf(name, "sfx_trading_altar_part_%d", i + 1);
        t->part = txmManager->Add(name, 0x40003, 1);
        if (!t->part)
            return 0;
        sprintf(name, "sfx_trading_altar_trail_%d", i + 1);
        t->trail = txmManager->Add(name, 0x40003, 1);
        if (!t->trail)
            return 0;
        if (i == 0)
            strcpy(name, "GLOW_SPRITE");
        else
            sprintf(name, "GLOW_SPRITE%d", i);
        t->glow = txmManager->Add(name, 0x40000, 1);
        if (!t->glow)
            return 0;
        sprintf(name, "T_A%d", i + 1);
        t->body = txmManager->Add(name, 0x40000, 1);
        if (!t->body)
            return 0;
    }
    return 1;
}

// v12 0x00483FD0  (feb 0x00479C40)
wrsfxTRALTAR_PLAYER_GLOW::wrsfxTRALTAR_PLAYER_GLOW(float scale, int kind) : partEMITTER_PHYS(50.0f, 1.0f)
{
    m3dSPL_LINEAR1D *in = traSpl(4);
    in->SetKp(0, 1.8666667f, 0.0f);
    in->SetKp(1, 1.9f, 50.0f);
    in->SetKp(2, 4.0333333f, 50.0f);
    in->SetKp(3, 4.3666667f, 0.0f);
    SetIntensity(50.0f, in);
    SetTimeLifePart(0.75f, 0.25f);
    m3dSPL_LINEAR1D *sz = traSpl(2);
    float s0 = scale * 0.0f;
    sz->SetKp(0, 0.0f, s0);
    float s1 = scale * 3.0f;
    sz->SetKp(1, 1.0f, s1);
    SetSize(s1, 0, sz, 1.0f);
    m3dSPL_LINEAR1D *a = traSpl(2);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wrsfxTRADING_ALTAR::tex[kind].part);
    speedA = scale;
    speedARand = s0;
    speedASpl = 0;
    flags |= 0x1000000;
    SetScaleCSVal(0.16f, 1.0f, 0.16f);
    flags |= 0x409;
    flags |= 0xA000;
}

// v12 0x00484200  (feb 0x00479E70)
wrsfxTRALTAR_TRAIL_EMITTER::wrsfxTRALTAR_TRAIL_EMITTER(float scale, int kind) : partEMITTER_PHYS(90.0f, 0.4f)
{
    m3dSPL_LINEAR1D *in = traSpl(2);
    in->SetKp(0, 1.8333334f, 90.0f);
    in->SetKp(1, 2.0f, 0.0f);
    SetIntensity(90.0f, in);
    SetTimeLifePart(0.35f, 0.05f);
    m3dSPL_LINEAR1D *sz = traSpl(2);
    float s0 = scale * 0.25f;
    sz->SetKp(0, 0.0f, s0);
    sz->SetKp(1, 1.0f, scale * 0.0f);
    SetSize(s0, 0, sz, 1.0f);
    m3dSPL_LINEAR1D *a = traSpl(2);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wrsfxTRADING_ALTAR::tex[kind].trail);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 150.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spreadSpl = 0;
    spread = scale * 0.1f;
    spreadMin = 0;
    speedK = 0.1f;
    speedKRand = 0;
    flags |= 0x409;
    flags |= 0x200000;
    flags |= 0x50;
}

// v12 0x00484400  (feb 0x0047A070)
// Scaled, out of collision; the look of its powerup; the emitters; a 5 s life.
BOOL32 wrsfxTRADING_ALTAR::ProcessINIT(void *init)
{
    wrsfxTRADING_ALTAR_INIT *d = (wrsfxTRADING_ALTAR_INIT *)init;
    if (!dynDYNAMIC::ProcessINIT(init))
        return 0;
    float s = d->scale;
    inst->Scale(s, s, s, 0);
    inst->flags |= 0x10;
    inst->root->SetStateYes(0x10);
    inst->Validate(4);
    if (d->kind) {
        inst->root->ReplaceTex(tex[0].body, tex[d->kind].body);
        inst->root->ReplaceTex(tex[0].glow, tex[d->kind].glow);
    }
    glow = new wrsfxTRALTAR_PLAYER_GLOW(d->scale, d->kind);
    if (!glow)
        return 0;
    objOBJ *o = objFindName(inst->root, (char *)"source_glow");
    if (!o)
        return 0;
    o->SetStateProcYes(8, 0);
    if (!partSetAreaEmissionObj(glow, o, -1, 0))
        return 0;
    glow->SetScaleCS((m3dMATR *)((char *)o + 0x3C));
    glow->StartEmission();
    for (int i = 0; i < 3; i++) {
        trail[i].emitter = new wrsfxTRALTAR_TRAIL_EMITTER(d->scale, d->kind);
        if (!trail[i].emitter)
            return 0;
        char name[0x10];
        sprintf(name, "plane%02d", i + 1);
        trail[i].obj = objFindNameN(inst->root, name, (int)strlen(name));
        if (!trail[i].obj)
            return 0;
        m3dV p;
        trail[i].obj->GetOrigin(&p);
        trail[i].emitter->SetPosDirSpeed(&p, 0, 0);
        trail[i].emitter->StartEmission();
    }
    life = 5.0f;
    gsSysServer->DeclSyncable(this);
    return 1;
}

// v12 0x00484640  (feb 0x0047A2B0)
void wrsfxTRADING_ALTAR::ProcessTERM()
{
    for (int i = 0; i < 3; i++)
        if (trail[i].emitter) {
            trail[i].emitter->__vecDelDtor(1);
            trail[i].emitter = 0;
        }
    if (glow) {
        glow->__vecDelDtor(1);
        glow = 0;
    }
}

// v12 0x00484690  (feb 0x0047A300)
// Its life runs out; the trails follow their planes.
void wrsfxTRADING_ALTAR::ProcessFRAME()
{
    dynDYNAMIC::ProcessFRAME();
    if (m3dUpdateTimeField(&life, gsElapsedTime))
        msgSystem.PostMsg(this, 2, this);
    inst->Validate(4);
    for (int i = 0; i < 3; i++) {
        m3dV p;
        trail[i].obj->GetOrigin(&p);
        trail[i].emitter->SetPosDirSpeed(&p, 0, 0);
    }
}

// v12 0x00484700  (feb 0x0047A370)
entENTITY *__fastcall wrsfxTRADING_ALTAR::Create(animINST *)
{
    void *mem = operator new(0x1BB);
    if (!mem)
        return 0;
    wrsfxTRADING_ALTAR *e = new (mem) wrsfxTRADING_ALTAR;
    e->catchUpRate = 1.0f;                              // (the dynamic's Create, inline)
    e->timeDebt = 0;
    e->dynFlags = 0;
    e->unk177[0] = e->unk177[1] = e->unk177[2] = e->unk177[3] = 0;
    e->unk187 = m3dVZero;
    e->unk193 = 0;
    e->glow = 0;
    for (int i = 0; i < 3; i++) {
        e->trail[i].emitter = 0;
        e->trail[i].obj = 0;
    }
    e->life = 0;
    return e;
}
