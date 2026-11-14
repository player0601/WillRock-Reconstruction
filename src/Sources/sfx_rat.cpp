// sfx_rat.cpp -- the rat's sparks and its explosion (Will Rock 1.2, v12.exe; feb obj sfx_rat.obj)

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../m3d/m3d_vol.h"
#include "../../include/sfx_sob/sfx_api.h"

txmTEXTURE *wrsfxRAT_SPARKS::pTexSparks;                // "sfx_rat_sparks"
txmTEXTURE *wrsfxRAT_EXPL::pTexBurn;                    // "sfx_rat_expl_burn"
txmTEXTURE *wrsfxRAT_EXPL::pTexSmoke;                   // "sfx_discus_land_cloud"
txmTEXTURE *wrsfxRAT_EXPL::pTexGlow;                    // "sfx_rat_expl_glow"

static m3dSPL_LINEAR1D *ratSpl(int n)
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

// v12 0x004802B0  (feb 0x00475F30)
int __fastcall wrsfxRatInit()
{
    if (!entRegisterClass((char *)"sfx_rat_sparks", 0x24534658, 0x50, wrsfxRAT_SPARKS::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"sfx_rat_expl", 0x24534658, 0x50, wrsfxRAT_EXPL::Create, 0, 1))
        return 0;
    if (!(wrsfxRAT_SPARKS::pTexSparks = txmManager->Add((char *)"sfx_rat_sparks", 0x40003, 1)))
        return 0;
    if (!(wrsfxRAT_EXPL::pTexBurn = txmManager->Add((char *)"sfx_rat_expl_burn", 0x40003, 1)))
        return 0;
    if (!(wrsfxRAT_EXPL::pTexSmoke = txmManager->Add((char *)"sfx_discus_land_cloud", 0x40003, 1)))
        return 0;
    return (wrsfxRAT_EXPL::pTexGlow = txmManager->Add((char *)"sfx_rat_expl_glow", 0x40003, 1)) != 0;
}

// v12 0x00480380  (feb 0x00476000)
wrsfxRAT_SPARKS_EMITTER::wrsfxRAT_SPARKS_EMITTER(float size) : partEMITTER_PHYS(40.0f, 1.5f)
{
    SetIntensity(40.0f, 0);
    SetTimeLifePart(1.0f, 0.5f);
    m3dSPL_LINEAR1D *sz = ratSpl(2);
    float s0 = size * 0.45f;
    sz->SetKp(0, 0.0f, s0);
    sz->SetKp(1, 1.0f, size * 0.0f);
    SetSize(s0, 0.0f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = ratSpl(2);
    a->SetKp(0, 0.0f, 191.25f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wrsfxRAT_SPARKS::pTexSparks);
    flags |= 0x100000;
    speedA = size * 10.0f;
    speedARand = size * 5.0f;
    cone = 180.0f;
    coneSpl = 0;
    speedASpl = 0;
    gravity.x = 0;
    gravity.y = size * -45.0f;
    gravity.z = 0;
    SetCDTParam(0.0f, 0.5f, 0.5f, 3);
    flags |= 0x10000;
    flags |= 1;
}

// v12 0x00480550  (feb 0x004761D0)
// The sparks fall to the floor under the source; on, and the plane shows.
void wrsfxRAT_SPARKS::Start()
{
    if (started)
        return;
    struct { const void *vtbl; int type; m3dV mn, mx; } box = { m3dBOX_vftable, 1 };
    m3dV p;
    source->GetOrigin(&p);
    struct { m3dV pos; m3dV n; } plane;
    plane.pos = wrsfxFindFloor(&p, (m3dBOX *)&box) ? m3dV { p.x, box.mx.y, p.z } : m3dVZero;
    plane.n = m3dVUnitY;
    emitter->SetCDTPlane(1, 0, (m3dPLANE *)&plane);
    emitter->StartEmission();
    float k = m3dRandRange(size * 0.5f, size * 4.0f);
    inst->Scale(k, k, k, 0);
    objValidateLTM(source);
    m3dV o;
    source->GetOrigin(&o);
    inst->Translate(&o, 0);
    emitter->SetPosDirSpeed(&o, 0, 0);
    inst->flags &= ~1u;
    started = 1;
}

// v12 0x004806B0  (feb 0x00476320)
void wrsfxRAT_SPARKS::Stop()
{
    if (started) {
        emitter->StopEmission();
        inst->flags |= 1;
        started = 0;
    }
}

// v12 0x004806F0  (feb 0x00476360)
BOOL32 wrsfxRAT_SPARKS::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 3) {
        if (owner == (entENTITY *)data) {
            started = 0;
            owner = 0;
            source = 0;
            msgSystem.PostMsg(this, 2, this);
        }
    } else if (id == 1) {
        struct { float size; entENTITY *owner; } *d = (decltype(d))data;
        size = d->size;
        owner = d->owner;
        objOBJ *o = objFindName(inst->root, (char *)"plane_&bb_&ds");
        if (o) {
            o->SetStateRendYes(0x60);
            emitter = new wrsfxRAT_SPARKS_EMITTER(d->size);
            if (emitter) {
                source = objFindName(owner->inst->root, (char *)"source_sparks");
                if (source) {
                    inst->flags |= 0x11;
                    inst->root->SetStateYes(0x10);
                    started = 0;
                    flags |= 2;
                }
            }
        }
    } else if (id == 2) {
        if (emitter) {
            emitter->__vecDelDtor(1);
            emitter = 0;
        }
    } else if (id == 1000 && started) {
        float k = m3dRandRange(size * 0.5f, size * 4.0f);
        inst->Scale(k, k, k, 0);
        objValidateLTM(source);
        m3dV o;
        source->GetOrigin(&o);
        inst->Translate(&o, 0);
        emitter->SetPosDirSpeed(&o, 0, 0);
    }
    return 0;
}

// v12 0x004808F0  (feb 0x00476570)
wrsfxRAT_EXPL_BURN::wrsfxRAT_EXPL_BURN(float size) : partEMITTER_PHYS(0x23, 100.0f, 0.6f)
{
    m3dSPL_LINEAR1D *in = ratSpl(4);
    in->SetKp(0, 0.033333335f, 0.0f);
    in->SetKp(1, 0.06666667f, 200.0f);
    in->SetKp(2, 0.13333334f, 200.0f);
    in->SetKp(3, 0.16666667f, 0.0f);
    SetIntensity(0.0f, in);
    SetTimeLifePart(0.4f, 0.2f);
    m3dSPL_LINEAR1D *sz = ratSpl(2);
    sz->SetKp(0, 0.0f, size * 1.5f);
    float s1 = size * 3.0f;
    sz->SetKp(1, 1.0f, s1);
    SetSize(s1, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = ratSpl(2);
    a->SetKp(0, 0.0f, 191.25f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wrsfxRAT_EXPL::pTexBurn);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 120.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spread = s1;
    speedARand = size * 0.5f;
    spreadSpl = 0;
    spreadMin = 0;
    speedA = size;
    speedASpl = 0;
    flags |= 0x409;
    SetDegNearParams(4.0f, 0.1f);
}

// v12 0x00480B50  (feb 0x004767D0)
wrsfxRAT_EXPL_SMOKE::wrsfxRAT_EXPL_SMOKE(float size) : partEMITTER_PHYS(0x14, 200.0f, 1.2f)
{
    m3dSPL_LINEAR1D *in = ratSpl(4);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 200.0f);
    in->SetKp(2, 0.06666667f, 200.0f);
    in->SetKp(3, 0.1f, 0.0f);
    SetIntensity(0.0f, in);
    SetTimeLifePart(0.9f, 0.3f);
    m3dSPL_LINEAR1D *sz = ratSpl(2);
    sz->SetKp(0, 0.0f, size * 1.75f);
    float s1 = size * 3.5f;
    sz->SetKp(1, 1.0f, s1);
    SetSize(s1, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = ratSpl(2);
    a->SetKp(0, 0.0f, 178.5f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wrsfxRAT_EXPL::pTexSmoke);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 90.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spreadSpl = 0;
    spread = size * 3.0f;
    spreadMin = 0;
    speedA = size;
    speedASpl = 0;
    gravity.x = 0;
    gravity.y = size * 4.0f;
    gravity.z = 0;
    speedARand = size * 0.5f;
    flags |= 0x408;
    SetDegNearParams(4.0f, 0.1f);
}

// v12 0x00480D80  (feb 0x00476A00)
wrsfxRAT_EXPL_GLOW::wrsfxRAT_EXPL_GLOW(float size) : partEMITTER_PHYS(0xC, 100.0f, 0.25f)
{
    m3dSPL_LINEAR1D *in = ratSpl(4);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 100.0f);
    in->SetKp(2, 0.06666667f, 100.0f);
    in->SetKp(3, 0.1f, 0.0f);
    SetIntensity(0.0f, in);
    SetTimeLifePart(0.175f, 0.075f);
    m3dSPL_LINEAR1D *sz = ratSpl(2);
    sz->SetKp(0, 0.0f, size * 3.45f);
    float s1 = size * 11.5f;
    sz->SetKp(1, 1.0f, s1);
    SetSize(s1, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = ratSpl(2);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wrsfxRAT_EXPL::pTexGlow);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spread = size;
    spreadSpl = 0;
    spreadMin = 0;
    speedA = 0;
    speedARand = 0;
    speedASpl = 0;
    flags |= 0x409;
    SetDegNearParams(4.0f, 0.1f);
}

// v12 0x00480FA0  (feb 0x00476C20)
BOOL32 wrsfxRAT_EXPL::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 1) {
        float size = *(float *)data;
        m3dV p;
        inst->GetPos(&p);
        inst->Scale(size, size, size, 0);
        inst->flags |= 0x10;
        inst->root->SetStateYes(0x10);
        if (!(burn = new wrsfxRAT_EXPL_BURN(size)))
            return 0;
        burn->SetPosDirSpeed(&p, 0, 0);
        if (!(smoke = new wrsfxRAT_EXPL_SMOKE(size)))
            return 0;
        smoke->SetPosDirSpeed(&p, 0, 0);
        if (!(glow = new wrsfxRAT_EXPL_GLOW(size)))
            return 0;
        glow->SetPosDirSpeed(&p, 0, 0);
        burn->StartEmission();
        glow->StartEmission();
        smoke->StartEmission();
        inst->SetAnimSeq(0, 0);
        timer = 2.0f;
    } else if (id == 2) {
        if (glow) {
            glow->__vecDelDtor(1);
            glow = 0;
        }
        if (smoke) {
            smoke->__vecDelDtor(1);
            smoke = 0;
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

// v12 0x004811A0  (feb 0x00476E20)
entENTITY *__fastcall wrsfxRAT_EXPL::Create(animINST *i)
{
    void *mem = operator new(0x17B);
    if (!mem)
        return 0;
    wrsfxRAT_EXPL *e = new (mem) wrsfxRAT_EXPL;
    e->burn = 0;
    e->smoke = 0;
    e->glow = 0;
    e->timer = 0;
    return e;
}

// v12 0x00481220  (feb 0x00476EA0)
entENTITY *__fastcall wrsfxRAT_SPARKS::Create(animINST *i)
{
    void *mem = operator new(0x17F);
    if (!mem)
        return 0;
    wrsfxRAT_SPARKS *s = new (mem) wrsfxRAT_SPARKS;
    s->emitter = 0;
    s->owner = 0;
    s->source = 0;
    s->started = 0;
    return s;
}
