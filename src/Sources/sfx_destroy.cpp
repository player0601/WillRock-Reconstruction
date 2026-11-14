// sfx_destroy.cpp -- the game's things that break (Will Rock 1.2, v12.exe; feb obj sfx_destroy.obj)
//
// dstrDESTROY (dyn_destroy.cpp) with a little more: a vase throws clay shards and dust, a mirror stands 1.7 higher
// (its pivot at the bottom), a barrel explodes and sets off the barrels near it one after another.
// A tripod ("tripod", wrsfxDESTROY_FLAME's code) burns with an "sfx_flame" on top; broken, the flame goes out and
// burning bits fall from it, bouncing on the floor.

#include "game.h"
#include "../m3d/m3d_spl.h"

txmTEXTURE *wrsfxDestrTexPart;                          // v12 0x0064ED4C "sfx_mcg_part"
txmTEXTURE *wrsfxDestrTexCloud;                         // v12 0x0064ED50 "sfx_mcg_cloud"
txmTEXTURE *wrsfxDestrTexGlow;                          // v12 0x0064ED44 "sfx_egg_glow"

BOOL32 appFloorPlane(m3dV *c, m3dV *plane, float r);     // v12 0x00467B80 (sfx_appear.cpp)

static m3dSPL_LINEAR1D *destrSpl(int n)
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

// v12 0x004707F0  (feb 0x00466AB0)
int __fastcall wrsfxDestrInit()
{
    if (!entRegisterClass((char *)"vase_destroy", 0x24445354, 0x28, wrsfxDESTROY_VASE::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"tripod", 0x24445354, 0x28, wrsfxDESTROY_TRIPOD::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"mirror", 0x24445354, 0x28, wrsfxDESTROY_MIRROR::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"barrel", 0x24445354, 0x28, wrsfxDESTROY_BARREL::Create, 0, 1))
        return 0;
    if (!(wrsfxDestrTexPart = txmManager->Add((char *)"sfx_mcg_part", 0x40003, 1)))
        return 0;
    if (!(wrsfxDestrTexCloud = txmManager->Add((char *)"sfx_mcg_cloud", 0x40003, 1)))
        return 0;
    return (wrsfxDestrTexGlow = txmManager->Add((char *)"sfx_egg_glow", 0x40003, 1)) != 0;
}

// v12 0x004708E0  (feb 0x00466BA0)
// Broken this frame: clay shards bouncing on the floor under it and a puff of dust, from where its debris is.
void wrsfxDESTROY_VASE::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    dstrDESTROY::ProcessCDT_FIREABLE(d);
    if (!(flags & DSTRF_THIS_FRAME))
        return;
    m3dV pos, plane[2];
    inst->GetPos(&pos);
    appFloorPlane(&pos, plane, 2.0f);
    wrsfxDESTROY_VASE_PARTS *parts = new wrsfxDESTROY_VASE_PARTS((m3dPLANE *)plane);
    if (!parts)
        return;
    if (objOBJ *o = objFindName(inst->root, (char *)"geom_debris"))
        o->GetOrigin(&pos);
    else
        inst->GetPos(&pos);
    parts->SetPosDirSpeed(&pos, &m3dVUnitY, 0);
    parts->StartEmission();
    wrsfxDESTROY_VASE_SMOKE *smoke = new wrsfxDESTROY_VASE_SMOKE;
    if (!smoke)
        return;
    smoke->SetPosDirSpeed(&pos, &m3dVUnitY, 0);
    smoke->StartEmission();
}

// v12 0x00471280  (feb 0x004675B0)
// 190 shards in a burst of 1/4 s, up at 3 (+-1.5) under gravity 9.8, living 4.5 s (+-1.5), fading; bouncing on the
// floor plane.
wrsfxDESTROY_VASE_PARTS::wrsfxDESTROY_VASE_PARTS(m3dPLANE *floor) : partEMITTER_PHYS(0xBE, 1500.0f, 6.0f)
{
    m3dSPL_LINEAR1D *in = destrSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.041666668f, 1500.0f);
    in->SetKp(2, 0.25f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(1.0f, 0.0f);
    flags |= 0x200;
    SetTimeLifePart(4.5f, 1.5f);
    SetSize(0.1f, 0.05f, 0, 1.0f);
    m3dSPL_LINEAR1D *al = destrSpl(3);
    al->SetKp(0, 0.0f, 255.0f);
    al->SetKp(1, 0.495f, 178.5f);
    al->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, al);
    SetTextureFrameAnim(2, 2, 0.0f, 3.0f);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 180.0f);
    flags |= 4;
    flags |= 2;
    gravity.x = 0;
    gravity.y = -9.8f;
    gravity.z = 0;
    speedA = 3.0f;
    speedARand = 1.5f;
    speedASpl = 0;
    cone = 180.0f;
    coneSpl = 0;
    spread = 0.5f;
    spreadSpl = 0;
    spreadMin = 0;
    SetTexture(wrsfxDestrTexPart);
    SetCDTParam(0.0f, 0.4f, 0.3f, 4);
    SetCDTPlane(1, 0, floor);
    flags |= 0x10000;
}

// v12 0x00471480  (feb 0x004677B0)
// 30 puffs of dust in 1/4 s, slowly rising, 1.5 m across, fading in 1.5 s.
wrsfxDESTROY_VASE_SMOKE::wrsfxDESTROY_VASE_SMOKE() : partEMITTER_PHYS(0x1E, 200.0f, 2.0f)
{
    m3dSPL_LINEAR1D *in = destrSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.041666668f, 200.0f);
    in->SetKp(2, 0.25f, 0.0f);
    SetIntensity(100.0f, in);
    SetAnimEmission(1.0f, 0.0f);
    flags |= 0x200;
    SetTimeLifePart(1.5f, 0.5f);
    SetSize(1.5f, 0.5f, 0, 1.0f);
    m3dSPL_LINEAR1D *al = destrSpl(3);
    al->SetKp(0, 0.0f, 0.0f);
    al->SetKp(1, 0.08f, 130.05f);
    al->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, al);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 0.0f);
    flags |= 4;
    gravity.x = 0;
    gravity.y = 1.8f;
    gravity.z = 0;
    speedA = 1.0f;
    speedARand = 0;
    speedASpl = 0;
    cone = 180.0f;
    coneSpl = 0;
    spread = 0.5f;
    spreadSpl = 0;
    spreadMin = 0;
    SetTexture(wrsfxDestrTexCloud);
    flags |= 0x400;
    flags |= 8;
    SetDegNearParams(4.0f, 0.1f);
}

// v12 0x00470CB0  (feb 0x00466FF0)
// The mirror's pivot is at its foot: the instance 1.7 up, its tree 1.7 down, the volumes worked out again.
BOOL32 wrsfxDESTROY_MIRROR::ProcessINIT()
{
    if (!dstrDESTROY::ProcessINIT())
        return 0;
    inst->Translate(0, 1.7f, 0, 2);
    ((m3dMATR *)inst->root->child->local)->Translate(0, -1.7f, 0, 2);
    inst->root->DeclareProcAnim();
    inst->Invalidate(4);
    inst->AbvTerm();
    inst->AbvInit();
    flags2 |= 0x80;
    return 1;
}

// v12 0x00470D40  (feb 0x00467080)
// A barrel's blast on another barrel comes later, the more so the lower that one stands: barrels go off one by one.
void wrsfxDESTROY_BARREL_DMG_DISPATCHER::Dispatch(entENTITY *target, dmgDAMAGE *d, entENTITY *attacker)
{
    char *a = target->inst->className ? target->inst->className : (char *)"";
    char *b = attacker->inst->className ? attacker->inst->className : (char *)"";
    if (strcmp(a, b)) {
        dmgDISPATCHER::Dispatch(target, d, attacker);
        return;
    }
    m3dV ct, ca;
    target->inst->GetCenter(&ct);
    attacker->inst->GetCenter(&ca);
    float dy = ct.y - ca.y, k;
    if (dy < 0)
        k = 1.0f;
    else if (dy <= 1.0f)
        k = 1.0f - dy * 0.8f;
    else
        k = 0.2f;
    msgSystem.PostTimeMsgData(target, 0x4C6, (msgDATA *)d, attacker, m3dRandRange(0.2f, 2.0f) * k);
}

// v12 0x00470E70  (feb 0x004671B0)
// Broken this frame: the explosion ("sfx_barrel_expl", 1.5 times its scale) and its damage, 20 within 5 times that.
void wrsfxDESTROY_BARREL::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    animCREATE_DATA cd;
    dstrDESTROY::ProcessCDT_FIREABLE(d);
    if (!(flags & DSTRF_THIS_FRAME))
        return;
    m3dV pos;
    if (objOBJ *o = objFindName(inst->root, (char *)"geom_debris"))
        o->GetOrigin(&pos);
    else
        inst->GetPos(&pos);
    float size = inst->GetScale() * 1.5f;
    ((m3dMATR *)&cd.matr)->_MakeLCS2WCS_VY(&pos, &m3dVUnitY);
    entCreate(gsScenePtr, (char *)"sfx_barrel_expl", (char *)"sfx_barrel_expl", &cd, &size, 0);
    wrsfxDESTROY_BARREL_DMG_DISPATCHER disp;
    dmgDispatchExplosDamage(this, &pos, 20.0f, size * 5.0f, 2, -1, 0, 0xFFFFFFFF, &disp);
}

// v12 0x00471650  (feb 0x00467980)
entENTITY *__fastcall wrsfxDESTROY_VASE::Create(animINST *i)
{
    wrsfxDESTROY_VASE *d = new wrsfxDESTROY_VASE;
    if (!d)
        return 0;
    d->flags = 0;
    return d;
}

// v12 0x004716E0  (feb 0x00467A10)
entENTITY *__fastcall wrsfxDESTROY_MIRROR::Create(animINST *i)
{
    wrsfxDESTROY_MIRROR *d = new wrsfxDESTROY_MIRROR;
    if (!d)
        return 0;
    d->flags = 0;
    return d;
}

// v12 0x00471720  (feb 0x00467A50)
entENTITY *__fastcall wrsfxDESTROY_BARREL::Create(animINST *i)
{
    wrsfxDESTROY_BARREL *d = new wrsfxDESTROY_BARREL;
    if (!d)
        return 0;
    d->flags = 0;
    return d;
}

// ---- the burning tripod -------------------------------------------------------------------------------------------

// v12 0x00470F70  (feb 0x004672C0): 33 burning bits in 0.4 s, thrown out and falling (9.8), bouncing on the floor
class wrsfxDESTROY_FLAME_PART : public partEMITTER_PHYS { public: wrsfxDESTROY_FLAME_PART(); };
wrsfxDESTROY_FLAME_PART::wrsfxDESTROY_FLAME_PART() : partEMITTER_PHYS(0x21, 150.0f, 8.0f)
{
    m3dSPL_LINEAR1D *in = destrSpl(2);
    in->SetKp(0, 0.0f, 150.0f);
    in->SetKp(1, 0.41666666f, 0.0f);
    SetIntensity(0, in);
    SetAnimEmission(1.0f, 0.0f);
    flags |= 0x200;
    SetTimeLifePart(5.0f, 3.0f);
    SetSize(0.1f, 0.0f, 0, 1.0f);
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 0.0f);
    flags |= 4;
    gravity.x = 0;
    gravity.y = -9.8f;
    gravity.z = 0;
    speedA = 3.0f;
    speedARand = 5.0f;
    speedASpl = 0;
    speedB = 0.9f;
    speedBRand = 1.5f;
    speedBSpl = 0;
    flags |= 0x8000;
    SetTexture(wrsfxDestrTexGlow);
    SetCDTParam(0.1f, 0.0f, 0.0f, -1);
    flags |= 0x30000;
    flags |= 0x2000;
}

// v12 0x004710F0  (feb 0x00467420): each bit's glowing trail
class wrsfxDESTROY_FLAME_PART_TRAIL : public partEMITTER_PHYS { public: wrsfxDESTROY_FLAME_PART_TRAIL(); };
wrsfxDESTROY_FLAME_PART_TRAIL::wrsfxDESTROY_FLAME_PART_TRAIL() : partEMITTER_PHYS(660.0f, 0.9f)
{
    SetTimeLifePart(0.6f, 0.3f);
    SetIntensity(20.0f, 0);
    m3dSPL_LINEAR1D *sz = destrSpl(2);
    sz->SetKp(0, 0.0f, 0.3f);
    sz->SetKp(1, 1.0f, 0.001f);
    SetSize(0.0f, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = destrSpl(2);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 0.0f);
    flags |= 4;
    gravity.x = 0;
    gravity.y = -5.0f;
    gravity.z = 0;
    speedA = 0.1f;
    speedARand = 0;
    speedASpl = 0;
    speedK = 0.6f;
    speedKRand = 0;
    SetTexture(wrsfxDestrTexGlow);
}

// v12 0x004709E0  (feb 0x00466CE0): an "sfx_flame" on "flame_source"; "flame_emitter" hidden
BOOL32 wrsfxDESTROY_TRIPOD::ProcessINIT()
{
    animCREATE_DATA cd;
    if (!dstrDESTROY::ProcessINIT())
        return 0;
    inst->Validate(4);
    objOBJ *src = objFindName(inst->root, (char *)"flame_source");
    if (!src)
        return 1;
    m3dV p;
    src->GetOrigin(&p);
    cd.matr.Transform((m3dMATR *)inst->matr, 0);
    cd.matr.Translate(&p, 0);
    flame = entCreate(gsScenePtr, (char *)"sfx_flame", (char *)"sfx_flame", &cd, 0, 0);
    if (!flame)
        return 0;
    msgADDR::flags |= 2;
    flameEmitter = objFindName(inst->root, (char *)"flame_emitter");
    if (flameEmitter)
        flameEmitter->SetStateProcYes(0x1008, 0);
    return 1;
}

// v12 0x00470AD0  (feb 0x00466DD0): the flame back on its place
void wrsfxDESTROY_TRIPOD::ProcessINIT_LEVEL()
{
    dstrDESTROY::ProcessINIT_LEVEL();
    objOBJ *src = objFindName(inst->root, (char *)"flame_source");
    if (!src)
        return;
    inst->Validate(4);
    m3dV p;
    src->GetOrigin(&p);
    flame->inst->Translate(&p, 0);
}

// v12 0x00470B30  (feb 0x00466E30)
void wrsfxDESTROY_TRIPOD::ProcessTERM()
{
    if (flame) {
        msgSystem.PostMsg(flame, 2, 0);
        flame = 0;
    }
}

// v12 0x00470B60  (feb 0x00466E60)
// Broken: the flame out; not silently, the burning bits (with trails) from "flame_emitter", bouncing on the floor
// under it (3 m down at most); its model hidden.
void wrsfxDESTROY_TRIPOD::StartExplos(int silent)
{
    dstrDESTROY::StartExplos(silent);
    if (!(flags & 4))
        return;
    if (flame) {
        msgSystem.PostMsg(flame, 2, 0);
        flame = 0;
    }
    if (!silent && flameEmitter) {
        m3dV pos, plane[2];
        inst->GetPos(&pos);
        appFloorPlane(&pos, plane, 3.0f);
        wrsfxDESTROY_FLAME_PART *part = new wrsfxDESTROY_FLAME_PART;
        if (!part)
            return;
        wrsfxDESTROY_FLAME_PART_TRAIL *trail = new wrsfxDESTROY_FLAME_PART_TRAIL;
        if (!trail)
            return;
        part->AttachEmitter(trail, 0, 0);
        part->flags |= 0x40000;
        part->SetCDTPlane(1, 0, (m3dPLANE *)plane);
        if (!partSetAreaEmissionObj(part, flameEmitter, -1, 0))
            return;
        part->StartEmission();
    }
    inst->flags |= 0x10;
}

// v12 0x00470C70  (feb 0x00466FB0): its flame gone by itself
BOOL32 wrsfxDESTROY_TRIPOD::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 3 && (void *)flame == data)
        flame = 0;
    return dstrDESTROY::ProcessMsg(id, data, from, ret);
}

// v12 0x00471690  (feb 0x004679C0)
entENTITY *__fastcall wrsfxDESTROY_TRIPOD::Create(animINST *)
{
    wrsfxDESTROY_TRIPOD *d = new wrsfxDESTROY_TRIPOD;
    if (!d)
        return 0;
    d->flags = 0;
    d->flame = 0;
    d->flameEmitter = 0;
    return d;
}
static_assert(sizeof(wrsfxDESTROY_TRIPOD) == 0x18B, "wrsfxDESTROY_TRIPOD");
