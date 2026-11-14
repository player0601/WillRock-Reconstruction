// sfx_fountain.cpp -- a fountain's jets ("sfx_fountain") (Will Rock 1.2, v12.exe; feb obj sfx_fountain.obj)
//
// Three jets (from "&h_&nc_AreaTop", "...AreaMed", "...AreaBot") and drops ("...AreaDrops"): each jet a base
// emitter whose particles each emit the visible water (tinted with the light on the floor under the fountain),
// all attached to the instance.

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"

u32 __fastcall gsEffEnvirColor(cdtINFO *ci, m3dCOLOR *col, m3dCOLOR *dyn);  // v12 0x005130E0 (gs:gs_effect.obj)

static txmTEXTURE *fntTexPart;                          // v12 wrsfxFOUNTAIN::pPartTex "sfx_fountain_part"
static txmTEXTURE *fntTexDrop;                          // v12 wrsfxFOUNTAIN::pDropTex "sfx_fountain_drop"

static m3dSPL_LINEAR1D *fntSpl(int n)
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

// ---- the jets' bases (inline in ProcessINIT there) ----------------------------------------------------------------

class wrsfxFOUNTAIN_TOP_EMITTER : public partEMITTER_PHYS { public: wrsfxFOUNTAIN_TOP_EMITTER(float s); };
class wrsfxFOUNTAIN_MED_EMITTER : public partEMITTER_PHYS { public: wrsfxFOUNTAIN_MED_EMITTER(float s, float n); };

// the top jet: 100 a second (0.5 s window), up at 2 x s (+-2 x s), 0.18 x s (+-0.18 x s) across
wrsfxFOUNTAIN_TOP_EMITTER::wrsfxFOUNTAIN_TOP_EMITTER(float s) : partEMITTER_PHYS(100.0f, 0.5f)
{
    speedASpl = 0;
    speedBSpl = 0;
    speedA = s + s;
    speedARand = s + s;
    float b = s * 0.18f;
    speedB = b;
    speedBRand = b;
    flags |= 0x840A000;
}

// the middle / bottom jets: n a second (0.3 s window), up at 0.5 x s, falling at 20 x s
wrsfxFOUNTAIN_MED_EMITTER::wrsfxFOUNTAIN_MED_EMITTER(float s, float n) : partEMITTER_PHYS(n, 0.3f)
{
    speedARand = 0;
    speedASpl = 0;
    gravity.x = 0;
    gravity.z = 0;
    speedA = s * 0.5f;
    gravity.y = s * -20.0f;
    flags |= 0x840A000;
}

// ---- the water they carry -----------------------------------------------------------------------------------------

class wrsfxFOUNTAIN_TOP_EMITTER_C : public partEMITTER_PHYS { public: wrsfxFOUNTAIN_TOP_EMITTER_C(float s, u32 c); };
class wrsfxFOUNTAIN_MED_EMITTER_C : public partEMITTER_PHYS { public: wrsfxFOUNTAIN_MED_EMITTER_C(float s, u32 c); };
class wrsfxFOUNTAIN_BOT_EMITTER_C : public partEMITTER_PHYS { public: wrsfxFOUNTAIN_BOT_EMITTER_C(float s, u32 c); };
class wrsfxFOUNTAIN_DROPS_EMITTER : public partEMITTER_PHYS { public: wrsfxFOUNTAIN_DROPS_EMITTER(float s); };

// v12 0x004743E0  (feb 0x0046A700)
// 400 a second (0.8 s window, 10 bright), 0.2 x s shrinking (0.1 -> 0.05 -> 0), fading from 51, twice their
// carrier's speed, 0.2 x s out, falling at 20 x s.
wrsfxFOUNTAIN_TOP_EMITTER_C::wrsfxFOUNTAIN_TOP_EMITTER_C(float s, u32 c) : partEMITTER_PHYS(400.0f, 0.8f)
{
    SetIntensity(10.0f, 0);
    m3dSPL_LINEAR1D *sz = fntSpl(3);
    sz->SetKp(0, 0.0f, s * 0.1f);
    sz->SetKp(1, 0.5f, s * 0.05f);
    sz->SetKp(2, 1.0f, s * 0.0f);
    float a = s * 0.2f;
    SetSize(a, 0.0f, sz, 10.0f);
    m3dSPL_LINEAR1D *al = fntSpl(2);
    al->SetKp(0, 0.0f, 51.0f);
    al->SetKp(1, 1.0f, 0.0f);
    SetColor(c, 0, al);
    SetTexture(fntTexPart);
    speedK = 2.0f;
    speedKRand = 0;
    cone = 180.0f;
    coneSpl = 0;
    speedA = a;
    gravity.y = s * -20.0f;
    speedARand = 0;
    speedASpl = 0;
    gravity.x = 0;
    gravity.z = 0;
    flags |= 0xC000408;
}

// v12 0x004745A0  (feb 0x0046A8C0)
// 400 a second (0.3 s, 5 bright), growing 1.03 x s to 2.5 x s (0.5 x s), fading in by 0.14 (61) and out, 0.2 of
// the carrier's speed, s out, falling at 20; squashed thin (0.025, 1, 0.025).
wrsfxFOUNTAIN_MED_EMITTER_C::wrsfxFOUNTAIN_MED_EMITTER_C(float s, u32 c) : partEMITTER_PHYS(400.0f, 0.3f)
{
    SetIntensity(5.0f, 0);
    m3dSPL_LINEAR1D *sz = fntSpl(2);
    sz->SetKp(0, 0.0f, s * 1.0325f);
    sz->SetKp(1, 1.0f, s * 2.5f);
    SetSize(s * 0.5f, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *al = fntSpl(3);
    al->SetKp(0, 0.0f, 0.0f);
    al->SetKp(1, 0.14f, 61.199997f);
    al->SetKp(2, 1.0f, 0.0f);
    SetColor(c, 0, al);
    speedK = 0.2f;
    speedKRand = 0;
    SetTexture(fntTexPart);
    SetTextureRotAnim(180.0f, 0.0f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedA = s;
    speedARand = 0;
    speedASpl = 0;
    gravity.x = 0;
    gravity.y = -20.0f;
    gravity.z = 0;
    flags |= 0x5000408;
    SetScaleCSVal(0.025f, 1.0f, 0.025f);
    m3dMATR m;
    m.Identity();
    SetScaleCS(&m);
    flags |= 0x8000000;
}

// v12 0x004747B0  (feb 0x0046AAD0)
// As the middle one, 1200 a second (10 bright), 2.5 x s big, a little wider (0.04).
wrsfxFOUNTAIN_BOT_EMITTER_C::wrsfxFOUNTAIN_BOT_EMITTER_C(float s, u32 c) : partEMITTER_PHYS(1200.0f, 0.3f)
{
    SetIntensity(10.0f, 0);
    m3dSPL_LINEAR1D *sz = fntSpl(2);
    sz->SetKp(0, 0.0f, s * 1.0325f);
    float b = s * 2.5f;
    sz->SetKp(1, 1.0f, b);
    SetSize(b, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *al = fntSpl(3);
    al->SetKp(0, 0.0f, 0.0f);
    al->SetKp(1, 0.14f, 61.199997f);
    al->SetKp(2, 1.0f, 0.0f);
    SetColor(c, 0, al);
    speedK = 0.2f;
    speedKRand = 0;
    SetTexture(fntTexPart);
    SetTextureRotAnim(180.0f, 0.0f, 0.0f, 0.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    speedA = s;
    speedARand = 0;
    speedASpl = 0;
    gravity.x = 0;
    gravity.y = -20.0f;
    gravity.z = 0;
    flags |= 0x5000408;
    SetScaleCSVal(0.04f, 1.0f, 0.04f);
    m3dMATR m;
    m.Identity();
    SetScaleCS(&m);
    flags |= 0x8000000;
}

// v12 0x004749A0  (feb 0x0046ACC0)
// The drops: 30 a second (1 s window), growing to 1.5 x s, fading in by 0.12 (93) and out, 0.2 x s (+-0.1 x s)
// across; flattened (y 0.1).
wrsfxFOUNTAIN_DROPS_EMITTER::wrsfxFOUNTAIN_DROPS_EMITTER(float s) : partEMITTER_PHYS(30.0f, 1.0f)
{
    m3dSPL_LINEAR1D *sz = fntSpl(2);
    sz->SetKp(0, 0.0f, s * 0.0f);
    float b = s * 1.5f;
    sz->SetKp(1, 1.0f, b);
    SetSize(b, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *al = fntSpl(3);
    al->SetKp(0, 0.0f, 0.0f);
    al->SetKp(1, 0.12f, 92.82f);
    al->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, al);
    SetTexture(fntTexDrop);
    speedBSpl = 0;
    speedB = s * 0.2f;
    speedBRand = s * 0.1f;
    flags |= 0x100A408;
    SetScaleCSVal(1.0f, 0.1f, 1.0f);
    m3dMATR m;
    m.Identity();
    SetScaleCS(&m);
    flags |= 0x8000000;
}

// v12 0x00474B70  (feb 0x0046AE90)
// The light on the floor under it (within 30 m) tints the water; the three jets (each base carrying its water)
// and the drops over their areas, started and attached to the instance.
BOOL32 wrsfxFOUNTAIN::ProcessINIT(float s)
{
    cdtREFINE ref(2, 0);
    cdtINFO ci;
    memset(&ci, 0, sizeof(ci));
    m3dCOLOR col, dyn;
    memset(&col, 0, sizeof(col));
    memset(&dyn, 0, sizeof(dyn));
    m3dV p, fl;
    ((m3dMATR *)inst->matr)->GetOrigin(&p);
    gsScenePtr->FloorPlace(&p, 30.0f, &fl, 10.0f, 1.0f, 0, &ref, &ci);
    gsEffEnvirColor(&ci, &col, &dyn);
    u32 c = (u32)(int)col.a;
    c = (c << 8) | ((u32)(int)col.r & 0xFF);
    c = (c << 8) | ((u32)(int)col.g & 0xFF);
    c = (c << 8) | ((u32)(int)col.b & 0xFF);
    if (!(part[0].emitter = new wrsfxFOUNTAIN_TOP_EMITTER(s)))
        return 0;
    if (!(part[0].child = new wrsfxFOUNTAIN_TOP_EMITTER_C(s, c)))
        return 0;
    if (!(part[0].area = objFindName(inst->root, (char *)"&h_&nc_AreaTop")))
        return 0;
    if (!(part[1].emitter = new wrsfxFOUNTAIN_MED_EMITTER(s, 200.0f)))
        return 0;
    if (!(part[1].child = new wrsfxFOUNTAIN_MED_EMITTER_C(s, c)))
        return 0;
    if (!(part[1].area = objFindName(inst->root, (char *)"&h_&nc_AreaMed")))
        return 0;
    if (!(part[2].emitter = new wrsfxFOUNTAIN_MED_EMITTER(s, 400.0f)))    // (the BOT base, its own vtable there)
        return 0;
    if (!(part[2].child = new wrsfxFOUNTAIN_BOT_EMITTER_C(s, c)))
        return 0;
    if (!(part[2].area = objFindName(inst->root, (char *)"&h_&nc_AreaBot")))
        return 0;
    if (!(part[3].emitter = new wrsfxFOUNTAIN_DROPS_EMITTER(s)))
        return 0;
    if (!(part[3].area = objFindName(inst->root, (char *)"&h_&nc_AreaDrops")))
        return 0;
    part[3].child = 0;
    for (int i = 0; i < 4; i++) {
        PART *pt = &part[i];
        if (pt->child) {
            pt->emitter->AttachEmitter(pt->child, 0, 0);
            pt->emitter->flags |= 0x40000;
        }
        if (!partSetAreaEmissionObj(pt->emitter, pt->area, -1, 0))
            return 0;
        pt->emitter->StartEmission();
        partAttachInst(pt->emitter, inst);
        if (pt->child)
            partAttachInst(pt->child, inst);
    }
    return 1;
}

// v12 0x00475060  (feb 0x0046B380)
BOOL32 wrsfxFOUNTAIN::ProcessMsg(int id, void *, msgADDR *, int *)
{
    if (id == 1) {
        ProcessINIT(1.0f);
    } else if (id == 2) {
        for (int i = 0; i < 4; i++) {
            if (part[i].emitter)
                part[i].emitter->__vecDelDtor(1);
            part[i].emitter = 0;
            if (part[i].child) {
                part[i].child->__vecDelDtor(1);
                part[i].child = 0;
            }
        }
    }
    return 0;
}

// v12 0x004750C0  (feb 0x0046B3E0)
entENTITY *__fastcall wrsfxFOUNTAIN::Create(animINST *)
{
    wrsfxFOUNTAIN *e = new wrsfxFOUNTAIN;
    if (!e)
        return 0;
    for (int i = 0; i < 4; i++) {
        e->part[i].emitter = 0;
        e->part[i].child = 0;
        e->part[i].area = 0;
    }
    return e;
}

// v12 0x00474370  (feb 0x0046A690)
int __fastcall wrsfxFountainInit()
{
    if (!(fntTexPart = txmManager->Add((char *)"sfx_fountain_part", 0x40003, 1)))
        return 0;
    if (!(fntTexDrop = txmManager->Add((char *)"sfx_fountain_drop", 0x40003, 1)))
        return 0;
    return entRegisterClass((char *)"sfx_fountain", 0x24534658, 0x50, wrsfxFOUNTAIN::Create, 0, 1) != 0;
}
