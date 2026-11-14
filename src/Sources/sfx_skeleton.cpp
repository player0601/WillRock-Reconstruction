// sfx_skeleton.cpp -- the skeletons' effects (Will Rock 1.2, v12.exe; feb obj sfx_skeleton.obj)
//
// "sfx_skel_spear_appear": a weapon growing into the hand. "sfx_skel_death": the dust and shards on its flying pieces.
// "sfx_skel_spear_land": grit and dust where a thrown spear sticks (5.1 s).
// Not rebuilt: wrsfxSKELETON_DEATH::AddPiece and its emitters (with the
// rigid pieces, rgdRIGID).

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../m3d/m3d_vol.h"
#include <stdio.h>

txmTEXTURE *wrsfxSKELETON_SPEAR_APPEAR::pCylTex;        // "sfx_skel_spear_appear_part"
txmTEXTURE *wrsfxSKELETON_SPEAR_LAND::pGritTex;          // "sfx_mcg_part"
txmTEXTURE *wrsfxSKELETON_SPEAR_LAND::pCloudTex;         // "sfx_mcg_cloud"

static m3dSPL_LINEAR1D *skelSpl(int n)
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

// v12 0x00481A40  (feb 0x004776C0)
int __fastcall wrsfxSkeletonInit()
{
    if (!(wrsfxSKELETON_SPEAR_APPEAR::pCylTex = txmManager->Add((char *)"sfx_skel_spear_appear_part", 0x40003, 1)))
        return 0;
    if (!entRegisterClass((char *)"sfx_skel_spear_appear", 0x24534658, 0x50, wrsfxSKELETON_SPEAR_APPEAR::Create, 0, 1))
        return 0;
    if (!(wrsfxSKELETON_SPEAR_LAND::pGritTex = txmManager->Add((char *)"sfx_mcg_part", 0x40003, 1)))
        return 0;
    if (!(wrsfxSKELETON_SPEAR_LAND::pCloudTex = txmManager->Add((char *)"sfx_mcg_cloud", 0x40003, 1)))
        return 0;
    if (!entRegisterClass((char *)"sfx_skel_spear_land", 0x24534658, 0x50, wrsfxSKELETON_SPEAR_LAND::Create, 0, 1))
        return 0;
    return entRegisterClass((char *)"sfx_skel_death", 0x24534658, 0x50, wrsfxSKELETON_DEATH::Create, 0, 1) != 0;
}

// v12 0x00481B50  (feb 0x004777D0)
wrsfxSKELETON_SPEAR_APPEAR_CYL::wrsfxSKELETON_SPEAR_APPEAR_CYL(float size) : partEMITTER_PHYS(100.0f, 0.2f)
{
    m3dSPL_LINEAR1D *in = skelSpl(4);
    in->SetKp(0, 0.5f, 100.0f);
    in->SetKp(1, 0.7f, 78.4f);
    in->SetKp(2, 0.93333334f, 28.175f);
    in->SetKp(3, 1.1666666f, 0.0f);
    SetIntensity(0.0f, in);
    m3dSPL_LINEAR1D *sp = skelSpl(2);
    sp->SetKp(0, 0.033333335f, size);
    float s0 = size * 0.0f;
    sp->SetKp(1, 1.1666666f, s0);
    spread = size;
    spreadSpl = sp;
    spreadMin = 0;
    SetTimeLifePart(0.2f, 0.0f);
    m3dSPL_LINEAR1D *sz = skelSpl(3);
    sz->SetKp(0, 0.0f, size * 6.75f);
    sz->SetKp(1, 0.5f, size * 3.375f);
    sz->SetKp(2, 1.0f, s0);
    float half = size * 0.5f;
    SetSize(0.0f, half, sz, 1.0f);
    SetColor(0xFFFFFFFF, 0, 0);
    SetTexture(wrsfxSKELETON_SPEAR_APPEAR::pCylTex);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 10.0f);
    flags |= 4;
    speedA = size;
    speedARand = half;
    speedASpl = 0;
    flags |= 0x100A409;
    SetScaleCSVal(0.33f, 0.33f, 1.0f);
    SetDegNearParams(3.0f, 0.3f);
}

// Over the weapon's object, scaled to it, turned a quarter.
void wrsfxSKELETON_SPEAR_APPEAR::Follow()
{
    m3dMATR m = *(m3dMATR *)srcObj->ltm;
    m.Scale(sx, sy, sz, 1);
    inst->Transform(&m, 0);
    inst->RotateY(90.0f, 1);
    if (partSetAreaEmissionObj(emitter, cylinder, -1, 0)) {
        m3dV o;
        m.GetOrigin(&o);
        emitter->SetPosDirSpeed(&o, 0, 0);
    }
}

// v12 0x00481DD0  (feb 0x00477A50)
void wrsfxSKELETON_SPEAR_APPEAR::Start(int sword)
{
    if (!sword) {
        srcObj = objFindName(owner->inst->root, (char *)"SPEAR_FX");
        sy = 1.0f;
    } else {
        srcObj = objFindName(owner->inst->root, (char *)"SWORD_&lod");
        sy = 0.61f;
    }
    sx = 1.0f;
    sz = 1.0f;
    timer = 1.16f;
    emitter->StartEmission();
    inst->SetAnimSeq(0, 0.0f);
    if (!srcObj)
        return;
    Follow();
    started = 1;
    inst->flags &= ~1u;
}

// v12 0x00481F20  (feb 0x00477BA0)
BOOL32 wrsfxSKELETON_SPEAR_APPEAR::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 3) {
        if (owner == (entENTITY *)data) {
            owner = 0;
            srcObj = 0;
            msgSystem.PostMsg(this, 2, this);
        }
    } else if (id == 1) {
        float size = data ? *(float *)data : 0.07f;
        emitter = new wrsfxSKELETON_SPEAR_APPEAR_CYL(size);
        if (!emitter)
            return 0;
        cylinder = objFindName(inst->root, (char *)"Cylinder");
        if (!cylinder || !partSetAreaEmissionObj(emitter, cylinder, -1, 0))
            return 0;
        for (int i = 1;; i++) {
            char name[16];
            sprintf(name, "&ds_Plane%d", i);
            objOBJ *o = objFindName(inst->root, name);
            if (!o)
                return 0;
            o->SetStateRendYes(0x60);
            if (i > 3)
                break;
        }
        inst->root->SetStateYes(0x10);
        inst->flags |= 0x11;
        cylinder->SetStateProcYes(0x1008, 0);
        emitter->SetScaleCS((m3dMATR *)cylinder->ltm);
        flags |= 2;
    } else if (id == 2) {
        if (emitter)
            emitter->__vecDelDtor(1);
        emitter = 0;
    } else if (id == 1000 && started && !(owner->inst->flags & 1) && srcObj) {
        if (m3dUpdateTimeField(&timer, gsElapsedTime)) {
            inst->flags |= 1;
            emitter->StopEmission();
            started = 0;
            return 0;
        }
        inst->AdvanceTime(gsElapsedTime, 0, 0);
        Follow();
    }
    return 0;
}

// v12 0x004835A0  (feb 0x00479210)
entENTITY *__fastcall wrsfxSKELETON_SPEAR_APPEAR::Create(animINST *i)
{
    wrsfxSKELETON_SPEAR_APPEAR *a = new wrsfxSKELETON_SPEAR_APPEAR;
    if (!a)
        return 0;
    a->emitter = 0;
    a->cylinder = 0;
    a->timer = 0;
    a->started = 0;
    a->owner = 0;
    a->srcObj = 0;
    a->sx = a->sy = a->sz = 1.0f;
    return a;
}

// v12 0x004833C0  (feb 0x00479030)
void wrsfxSKELETON_DEATH::ProcessFRAME()
{
    if (m3dUpdateTimeField(&timer, gsElapsedTime))
        msgSystem.PostMsg(this, 2, this);
}

// v12 0x00483540  (feb 0x004791B0)
// (the pieces it follows come with rgdRIGID: AddPiece, not rebuilt)
BOOL32 wrsfxSKELETON_DEATH::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 1) {
        size = *(float *)data;
        timer = 4.0f;
        flags |= 2;
    } else if (id == 1000) {
        ProcessFRAME();
    }
    return 0;
}

// v12 0x004836D0  (feb 0x00479340)
entENTITY *__fastcall wrsfxSKELETON_DEATH::Create(animINST *i)
{
    return new wrsfxSKELETON_DEATH;
}

// ---------------------------------------------------------------------------------------------- the spear's landing

class wrsfxSKELETON_SPEAR_LAND_GRIT : public partEMITTER_PHYS { public: wrsfxSKELETON_SPEAR_LAND_GRIT(float s); };
class wrsfxSKELETON_SPEAR_LAND_CLOUDS : public partEMITTER_PHYS { public: wrsfxSKELETON_SPEAR_LAND_CLOUDS(float s); };

// (feb 0x00477E90)
// 15 bits of grit (in 1/6 s) from the circle (within s), 4 (+-1) s each, 0.14 x s (+-0.07 x s), two frames, fading
// after 0.55, thrown 12 x s (+-6 x s) up and 2 x s (+-s) across, falling at 30 x s, settling on the ground plane.
wrsfxSKELETON_SPEAR_LAND_GRIT::wrsfxSKELETON_SPEAR_LAND_GRIT(float s) : partEMITTER_PHYS(15, 87.5f, 5.0f)
{
    m3dSPL_LINEAR1D *in = skelSpl(5);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 75.0f);
    in->SetKp(2, 0.083333336f, 87.5f);
    in->SetKp(3, 0.13333334f, 75.0f);
    in->SetKp(4, 0.16666667f, 0.0f);
    SetIntensity(0.0f, in);
    spreadMin = s;
    spread = s;
    spreadSpl = 0;
    SetTimeLifePart(4.0f, 1.0f);
    SetSize(s * 0.14f, s * 0.07f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = skelSpl(3);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 0.555f, 255.0f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wrsfxSKELETON_SPEAR_LAND::pGritTex);
    SetTextureFrameAnim(2, 2, 0.0f, 3.0f);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 0.0f);
    flags |= 6;
    speedA = s * 12.0f;
    speedB = s;
    speedASpl = 0;
    speedBSpl = 0;
    gravity.x = 0;
    gravity.z = 0;
    speedARand = s * 6.0f;
    speedBRand = s + s;
    gravity.y = s * -30.0f;
    SetCDTParam(0.0f, s * 0.4f, s * 0.4f, 3);
    flags |= 0x1A008;
    SetDegFarParams(10.0f, 30.0f, 2.0f, 0.1f);
}

// (feb 0x00478100)
// 15 puffs (in 1/6 s) from within 0.5 x s, 1.5 (+-0.5) s each, 1.5 x s, faint (35), turning 60 a second, 0.6 x s
// (+-0.3 x s) up and 0.8 x s (+-0.4 x s) across.
wrsfxSKELETON_SPEAR_LAND_CLOUDS::wrsfxSKELETON_SPEAR_LAND_CLOUDS(float s) : partEMITTER_PHYS(15, 79.166664f, 2.0f)
{
    m3dSPL_LINEAR1D *in = skelSpl(5);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 66.666664f);
    in->SetKp(2, 0.083333336f, 79.166664f);
    in->SetKp(3, 0.13333334f, 66.666664f);
    in->SetKp(4, 0.16666667f, 0.0f);
    SetIntensity(0.0f, in);
    spreadSpl = 0;
    spreadMin = 0;
    spread = s * 0.5f;
    SetTimeLifePart(1.5f, 0.5f);
    SetSize(s * 1.5f, 0.0f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = skelSpl(2);
    a->SetKp(0, 0.0f, 34.782f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0x1FFFFFFF, 0, a);
    SetTexture(wrsfxSKELETON_SPEAR_LAND::pCloudTex);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 60.0f);
    flags |= 4;
    speedASpl = 0;
    speedBSpl = 0;
    speedA = s * 0.6f;
    speedARand = s * 0.3f;
    speedB = s * 0.8f;
    speedBRand = s * 0.4f;
    flags |= 0xA008;
    SetDegFarParams(10.0f, 30.0f, 2.0f, 0.1f);
}

// v12 0x00482680  (feb 0x00478300)
// Both emitters for 5.1 s; the model shown (instance flag 1 off).
void wrsfxSKELETON_SPEAR_LAND::Start()
{
    timer = 5.1f;
    grit->StartEmission();
    clouds->StartEmission();
    running = 1;
    inst->flags &= ~1u;
}

// v12 0x004826C0  (feb 0x00478340)
// 1 (init, the size, 0.7 by default): half that scale; grit and dust from its "Circle" (hidden), the grit kept above
// the plane it stands on. 2: the emitters gone. 1000: when its time is up, stopped, hidden, gone.
BOOL32 wrsfxSKELETON_SPEAR_LAND::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id == 1) {
        float s = data ? *(float *)data : 0.7f;
        float h = s * 0.5f;
        inst->Scale(h, h, h, 2);
        if (!(grit = new wrsfxSKELETON_SPEAR_LAND_GRIT(s)))
            return 0;
        if (!(clouds = new wrsfxSKELETON_SPEAR_LAND_CLOUDS(s)))
            return 0;
        if (!(circle = objFindName(inst->root, (char *)"Circle")))
            return 0;
        if (!partSetAreaEmissionObj(grit, circle, -1, 0))
            return 0;
        m3dV plane[2];
        ((m3dMATR *)inst->matr)->GetOrigin(&plane[0]);
        ((m3dMATR *)inst->matr)->GetAxisY(&plane[1]);
        grit->SetCDTPlane(1, 0, (m3dPLANE *)plane);
        if (!partSetAreaEmissionObj(clouds, circle, -1, 0))
            return 0;
        circle->SetStateProcYes(0x1008, 0);
        inst->flags |= 0x10;
    } else if (id == 2) {
        if (clouds)
            clouds->__vecDelDtor(1);
        clouds = 0;
        if (grit)
            grit->__vecDelDtor(1);
        grit = 0;
    } else if (id == 1000 && running) {
        if (m3dUpdateTimeField(&timer, gsElapsedTime)) {
            inst->flags |= 1;
            running = 0;
            grit->StopEmission();
            clouds->StopEmission();
            msgSystem.PostMsg(this, 2, this);
        }
    }
    return 0;
}

// v12 0x00483650  (feb 0x004792C0)
entENTITY *__fastcall wrsfxSKELETON_SPEAR_LAND::Create(animINST *)
{
    wrsfxSKELETON_SPEAR_LAND *e = new wrsfxSKELETON_SPEAR_LAND;
    if (!e)
        return 0;
    e->grit = 0;
    e->clouds = 0;
    e->circle = 0;
    e->timer = 5.1f;
    e->running = 1;
    return e;
}
