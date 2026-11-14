// sfx_treasure.cpp -- a treasure's glitter (Will Rock 1.2, v12.exe; feb obj sfx_treasure.obj)
//
// An unseen emitter runs over the object "emitter" of the template (scaled to the treasure's size) and each of its
// particles throws glowing stars from a second one. The glow texture has its own alpha: drawn blended, not added.

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../m3d/m3d_vol.h"

txmTEXTURE *wrsfxTREASURE::pTexSparks;                  // "sfx_treasure_glow"

// v12 0x004847F0  (feb 0x0047A450)
int __fastcall wrsfxTreasureInit()
{
    if (!(wrsfxTREASURE::pTexSparks = txmManager->Add((char *)"sfx_treasure_glow", 0x40003, 1)))
        return 0;
    return entRegisterClass((char *)"sfx_treasure", 0x24534658, 0x50, wrsfxTREASURE::Create, 0, 1) != 0;
}

// v12 0x00484840  (feb 0x0047A4A0)
entENTITY *__fastcall wrsfxTREASURE::Create(animINST *i)
{
    wrsfxTREASURE *t = new wrsfxTREASURE;
    if (t) {
        t->emitterObj = 0;
        t->sparks1 = 0;
        t->sparks2 = 0;
    }
    return t;
}

// v12 0x00484910  (feb 0x0047A560)
// Created with the treasure's instance as its data.
BOOL32 wrsfxTREASURE::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    dynDYNAMIC::ProcessMsg(id, data, from, ret);
    if (id == 1000) {
        partSetAreaEmissionObj(sparks1, emitterObj, -1, 0);
    } else if (id == 1) {
        float k = 1.0f;
        if (data) {
            struct { const void *vtbl; int type; m3dV c; float radius; } a, b;
            animINST *t = (animINST *)data;
            t->Validate(2);
            ((m3dBOX *)&t->root->bbox)->m3dBOX::CalcEnclSph((m3dSPHERE *)&a);
            inst->Validate(2);
            ((m3dBOX *)&inst->root->bbox)->m3dBOX::CalcEnclSph((m3dSPHERE *)&b);
            k = a.radius / b.radius;
        }
        inst->Scale(k, k, k, 0);
        emitterObj = objFindName(inst->root, (char *)"emitter");
        if (!emitterObj)
            return 0;
        inst->root->SetStateProcYes(0x1008, 0);
        inst->flags |= 0x10;
        if (!(sparks1 = new wrsfxTREASURE_SPARKS_1(k)))
            return 0;
        if (!(sparks2 = new wrsfxTREASURE_SPARKS_2(k)))
            return 0;
        sparks1->AttachEmitter(sparks2, 0, 0);
        sparks1->flags |= 0x40000;
        if (!partSetAreaEmissionObj(sparks1, emitterObj, -1, 0))
            return 0;
        sparks1->StartEmission();
        partAttachInst(sparks1, inst);
        partAttachInst(sparks2, inst);
        WR_FIELD(objOBJ *, inst, 0xEC) = inst->root;
    } else if (id == 2) {
        if (sparks1)
            sparks1->__vecDelDtor(1);
        sparks1 = 0;
        if (sparks2)
            sparks2->__vecDelDtor(1);
        sparks2 = 0;
    } else if (id == 0x3FB) {
        *ret = 1;
        return 1;
    }
    return 0;
}

// v12 0x00484B00  (feb 0x0047A7F0)
// Never drawn (0x400000): where the stars come from, over the emitter object.
wrsfxTREASURE_SPARKS_1::wrsfxTREASURE_SPARKS_1(float size) : partEMITTER_PHYS(20.0f, 0.15f)
{
    SetTimeLifePart(0.1f, 0.05f);
    SetSize(0.3f, 0.0f, 0, 1.0f);
    spreadSpl = 0;
    spreadMin = 0;
    cone = 180.0f;
    coneSpl = 0;
    speedA = size;
    spread = size * 3.0f;
    speedARand = 0;
    speedASpl = 0;
    speedK = 1.0f;
    speedKRand = 0;
    flags |= 0x200000;
    flags |= 0x400000;
    flags |= 0x2000;
    SetDegFarParams(10.0f, 35.0f, 2.5f, 0.3f);
    flags |= 0x8000000;
}

// v12 0x00484BE0  (feb 0x0047A8D0)
wrsfxTREASURE_SPARKS_2::wrsfxTREASURE_SPARKS_2(float size) : partEMITTER_PHYS(400.0f, 1.0f)
{
    SetTimeLifePart(0.75f, 0.25f);
    SetIntensity(50.0f, 0);
    m3dSPL_LINEAR1D *s = new m3dSPL_LINEAR1D;
    s->flags = 0;
    s->type = 0;
    s->nKp = 0;
    s->kpValueSize = 8;
    s->kpStride = 8;
    s->dim = 1;
    s->kpBytes = 0;
    s->kp = 0;
    s->pad[0] = s->pad[1] = s->pad[2] = 0;
    s->AllocKpList(3);
    s->SetKp(0, 0.0f, 0.0f);
    s->SetKp(1, 0.405f, size * 8.0f);
    s->SetKp(2, 1.0f, 0.0f);
    SetSize(0.0f, 0.0f, s, 1.0f);
    speedK = 1.0f;
    speedKRand = 0;
    cone = 180.0f;
    coneSpl = 0;
    SetTexture(wrsfxTREASURE::pTexSparks);
    flags |= 0x400;
    flags |= 8;
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 75.0f);
    flags |= 4;
    SetTextureScale(1.0f, 0.0f, 0.2f, 0.05f);
    flags |= 0x8000000;
}
