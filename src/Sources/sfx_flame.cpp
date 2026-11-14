// sfx_flame.cpp -- flames and their sparks (Will Rock 1.2, v12.exe; feb obj sfx_flame.obj)
//
// The burning quad of a torch: a frame animated, additive "FLAME..." object. [FLAME] isDayNight makes it light and go out
// on a random cycle (or by the IActions START / STOP), with sparks as it lights and smoke as it goes out. wrsfxFLAME_SPARKS
// is also an item's "appear" effect (wrITEM::ShowUnhideSfx).
//
// Not rebuilt: the [FLAME] isLight spot light, a pteSPOT_ANIM with a noise colour (v12 0x004737BE..0x004739EF; engine
// pte_spot.obj is not rebuilt): 'light' stays 0, which every use below allows.

#include "game.h"
#include "../../include/game_shared/wr_item.h"
#include "../m3d/m3d_spl.h"
#include "../m3d/m3d_ctrl.h"

txmTEXTURE *wrsfxFLAME::pTexSpark;                      // v12 0x0064ED6C "sfx_flame_spark"
txmTEXTURE *wrsfxFLAME::pTexSmoke;                      // v12 0x0064ED70 "sfx_mcg_cloud"

// v12 0x00472AD0  (feb 0x00468E00)
int __fastcall wrsfxFlameInit()
{
    if (!(wrsfxFLAME::pTexSmoke = txmManager->Add((char *)"sfx_mcg_cloud", 0x40003, 1)))
        return 0;
    if (!(wrsfxFLAME::pTexSpark = txmManager->Add((char *)"sfx_flame_spark", 0x40003, 1)))
        return 0;
    return entRegisterClass((char *)"sfx_flame", 0x24444E54, 0x2D, wrsfxFLAME::Create, 0, 1) != 0;
}

// the m3dSPL_LINEAR1D constructor inline (v12 0x00472B72)
static m3dSPL_LINEAR1D *flameSpl(int n)
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

// v12 0x00472B40  (feb 0x00468E70)
// A burst of fast sparks (up to 1000 a second for 2 s, the first quarter second), falling, fading.
wrsfxFLAME_SPARKS::wrsfxFLAME_SPARKS(float size) : partEMITTER_PHYS(250, 2000.0f, 1.7f)
{
    m3dSPL_LINEAR1D *in = flameSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.041666668f, 1000.0f);
    in->SetKp(2, 0.25f, 0.0f);
    SetIntensity(0.0f, in);
    SetTimeLifePart(0.85f, 0.65f);
    float sz = size + size;
    spreadSpl = 0;
    spreadMin = 0;
    spread = size * 0.15f;
    SetSize(sz * 0.075f, sz * 0.025f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = flameSpl(2);
    a->SetKp(0, 0.0f, 202.215f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    flags |= 1;
    flags |= 0x100004;
    gravity.x = 0;
    gravity.y = size * -8.0f;
    gravity.z = 0;
    speedASpl = 0;
    speedBSpl = 0;
    speedA = sz * 3.0f;
    speedARand = sz * 1.5f;
    speedB = sz * 0.6f;
    speedBRand = sz * 0.3f;
    SetTexture(wrsfxFLAME::pTexSpark);
    SetAnimEmission(2.0f, 0.0f);
    SetDegFarParams(40.0f, 80.0f, 4.0f, 0.2f);
}

// v12 0x00472D60  (feb 0x00469090)
// Two seconds of slow rising puffs.
wrsfxFLAME_SMOKE::wrsfxFLAME_SMOKE(float size) : partEMITTER_PHYS(50, 50.0f, 2.0f)
{
    m3dSPL_LINEAR1D *in = flameSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.041666668f, 50.0f);
    in->SetKp(2, 1.9166666f, 0.0f);
    SetIntensity(50.0f, in);
    SetTimeLifePart(1.0f, 0.5f);
    SetSize(size * 0.675f, size * 0.225f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = flameSpl(3);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.18f, 80.58f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 80.0f);
    flags |= 4;
    m3dSPL_LINEAR1D *sp = flameSpl(2);
    sp->SetKp(0, size * 0.0f, size * 4.2f);
    sp->SetKp(1, size * 1.9166666f, size * 1.2f);
    speedA = 4.2f;
    speedARand = 0.25f;
    speedASpl = sp;
    spreadSpl = 0;
    spreadMin = 0;
    cone = 20.0f;
    spread = size * 0.2f;
    coneSpl = 0;
    SetTexture(wrsfxFLAME::pTexSmoke);
    flags |= 0x400;
    flags |= 8;
    SetAnimEmission(2.0f, 0.0f);
    SetDegFarParams(40.0f, 80.0f, 2.0f, 0.3f);
}

// v12 0x00472FE0  (feb 0x00469310)
entENTITY *__fastcall wrsfxFLAME::Create(animINST *i)
{
    void *mem = operator new(0x183);
    if (!mem)
        return 0;
    wrsfxFLAME *f = new (mem) wrsfxFLAME;
    f->flameFlags = 0;
    f->cycle = 0;
    f->light = 0;
    f->smoke = 0;
    f->sparks = 0;
    f->f_17F = 0;
    return f;
}

// v12 0x00473040  (feb 0x00469370)
// Only a day and night flame: 4 sequences of the template, UNUSED empty, and the ways between STAY, START and STOP.
BOOL32 wrsfxFLAME::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    int v;
    if (!inst->GetInt((char *)"FLAME", (char *)"isDayNight", &v) || !v)
        return 1;
    animSEQ *old = t->seq;
    int nOld = 0;
    if (old)
        nOld = t->nSeq;
    t->nSeq = 4;
    u32 *blk = (u32 *)operator new(0x224);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 4;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 4; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    _entAnimSetSeqName(t, 0, (char *)"WRSFX_FLAME_SEQ_UNUSED");
    _entAnimSetSeqName(t, 1, (char *)"WRSFX_FLAME_SEQ_STAY");
    _entAnimSetSeqName(t, 2, (char *)"WRSFX_FLAME_SEQ_START");
    _entAnimSetSeqName(t, 3, (char *)"WRSFX_FLAME_SEQ_STOP");
    entAnimAssignTplSeq(t, old, nOld);
    t->seq[0].frameStart = 0;
    t->seq[0].frameEnd = 1.0f;
    t->seq[0].lengthMs = 500.0f;
    t->seq[0].dir = 0;
    animSYSTEM *s = new animSYSTEM(4, 0);
    if (!s)
        return 0;
    t->aniSystem = s;
    s->SetTranTypeToAny(0, 0, 1, -1);
    s->SetTranCycleAnim(1);
    s->SetTranParam(2, 2, 1, 1, 0);
    s->SetTranParam(2, 1, 1, 1, 0);
    s->SetTranParam(2, 3, 0, 3, 0);
    s->SetTranParam(1, 2, 1, 1, 0);
    s->SetTranParam(1, 3, 0, 3, 0);
    s->SetTranParam(3, 3, 2, 3, 0);
    s->SetTranParam(3, 2, 0, 2, 0);
    if (old)
        operator delete((u8 *)old - 4);
    return 1;
}

// (inline four times in ProcessMsg)
void wrsfxFLAME::SfxStart()
{
    if (sparks) {
        sparks->StopEmission();
        sparks->StartEmission();
    }
    if (light)
        light->Enable(1);
    flameFlags |= 8;
    pendingEvents |= 1;
}

void wrsfxFLAME::SfxStop()
{
    if (smoke) {
        smoke->StopEmission();
        smoke->StartEmission();
    }
    if (light)
        light->Enable(0);
    flameFlags &= ~8u;
    pendingEvents |= 2;
}

// v12 0x00473260  (feb 0x00469590)
// No base call. FRAME: the cycle lights and puts out a day and night flame, and it plays START or STOP.
BOOL32 wrsfxFLAME::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 1000) {
        if (inst->flags & 3)
            return 0;
        if (cycle)
            cycle->Update(gsElapsedTime);
        if (!(flameFlags & 1)) {
            if (light)
                light->Enable(1);
            return 0;
        }
        float v;
        cycle->GetValue(&v);
        if (v > 0.5f) {
            if (!(flameFlags & 8))
                SfxStart();
        } else if (flameFlags & 8) {
            SfxStop();
        }
        ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, ((~flameFlags & 8) | 0x10) >> 3, gsElapsedTime, 0);
    } else if (id == 2) {
        light = 0;                                      // (each deleted only if made: a flame without
        if (smoke)                                      //  [FLAME] isDayNight has none of them -- v12 0x004733F8)
            smoke->__vecDelDtor(1);
        smoke = 0;
        if (sparks)
            sparks->__vecDelDtor(1);
        sparks = 0;
        if (cycle)
            cycle->__vecDelDtor(1);
        cycle = 0;
    } else if (id == 1) {
        m3dV pos;
        inst->GetPos(&pos);
        objOBJ *o = objFindNameN(inst->root, (char *)"FLAME", 5);
        if (!o)
            return 0;
        inst->flags |= 0x10;
        objMOD_TEX_FRAME_ANIM *m = new objMOD_TEX_FRAME_ANIM(0);
        m->SetParams(4, 8, 1.5f, m3dRandMax(1.5f), 30, 60.0f, 120.0f, 1);
        m->AddObj(o);
        inst->root->SetTransparency(0x40, 0xFF);
        inst->root->SetConstColor(0xFFAFAFAF, 2, 0);
        int on;
        if (inst->GetInt((char *)"FLAME", (char *)"isDayNight", &on) && on) {
            float size = 1.0f;
            if (inst->GetFloat((char *)"FLAME", (char *)"size", &size)) {
                inst->Scale(size, size, size, 1);
                inst->Invalidate(4);
            }
            if (!(sparks = new wrsfxFLAME_SPARKS(size * 0.58823532f)))
                return 0;
            if (!(smoke = new wrsfxFLAME_SMOKE(size * 0.58823532f)))
                return 0;
            smoke->SetPosDirSpeed(&pos, &m3dVUnitY, 0);
            sparks->SetPosDirSpeed(&pos, &m3dVUnitY, 0);
            partAttachInst(smoke, inst);
            partAttachInst(sparks, inst);
            int start = 1;
            inst->GetBool((char *)"FLAME", (char *)"isStartSFX", &start);
            if (start) {
                if (light)
                    light->Enable(1);
                flameFlags |= 8;
                pendingEvents |= 1;
            }
            flameFlags |= 1;
            m3dSPL_LINEAR1D *s = flameSpl(3);
            s->SetKp(0, 0.0f, 0.0f);
            s->SetKp(1, m3dRandRange(0.6f, 0.8f), 0.5f);
            s->SetKp(2, 1.0f, 1.0f);
            m3dCTRL_1D_SPL *c = new m3dCTRL_1D_SPL;
            if (c) {
                c->flags = 0;
                c->t = 0;
                c->length = m3dRandRange(10.0f, 15.0f);
                c->speed = 1.0f;
                c->_14 = 0;
                c->flags |= M3D_CTRL_RUNNING;
                c->value = 0;
                c->spl = s;
            }
            cycle = c;
            float t = m3dRandMax(15.0f);
            if (cycle->flags & M3D_CTRL_RUNNING)
                cycle->t = t;
        } else {
            inst->SetAnimSeq(0, 0);
        }
        inst->centerObj = inst->root;
    } else if (id == 0x3F4) {
        if (light)
            light->Enable(0);                           // (after pteSHADER::CalcStaticData)
    } else if (id == 0x3F8) {
        apNAME *n = (apNAME *)((u8 *)data + 8);
        if (n->IsName((char *)"START"))
            SfxStart();
        else if (n->IsName((char *)"STOP"))
            SfxStop();
    } else if (id == 0x4D0) {
        if (flameFlags & 1)
            flameFlags = (flameFlags | 2) & ~4u;
    } else if (id == 0x4D1) {
        if (flameFlags & 1)
            flameFlags = (flameFlags & ~2u) | 4;
    }
    return 0;
}
