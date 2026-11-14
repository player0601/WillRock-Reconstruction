// sfx_wfall.cpp -- a waterfall's foam ("sfx_waterfall") (Will Rock 1.2, v12.exe; feb obj sfx_wfall.obj)
//
// Foam rising from each "&h_&nc_area*" object of the instance (up to 9), attached to it.

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"

static txmTEXTURE *wfallTexFoam;                        // v12 0x0064F224 wrsfxWATERFALL::pFoamTex "sfx_wfall_part"

// (objFILTER_NAME_LEN, v12 0x00485B50 in this obj: inline in engine_core/object.h)

class wrsfxWATERFALL_FOAM : public partEMITTER_PHYS { public: wrsfxWATERFALL_FOAM(float s); };

// v12 0x00485830  (feb 0x0047B520)
// 10 a second, 1.6 s window, growing to 11.25 x s by half its life (7.5 x s), fading in by 0.2 (31) and out,
// thrown 10 x s (+-0.5 x s) up and 3 x s (+-0.15 x s) across, falling at 40 x s.
wrsfxWATERFALL_FOAM::wrsfxWATERFALL_FOAM(float s) : partEMITTER_PHYS(10.0f, 1.6f)
{
    m3dSPL_LINEAR1D *sz = new m3dSPL_LINEAR1D;
    if (sz) {
        sz->flags = 0;
        sz->type = 0;
        sz->nKp = 0;
        sz->kpValueSize = 8;
        sz->kpStride = 8;
        sz->dim = 1;
        sz->kpBytes = 0;
        sz->kp = 0;
        sz->pad[0] = sz->pad[1] = sz->pad[2] = 0;
        sz->AllocKpList(2);
    }
    sz->SetKp(0, 0.0f, s * 0.0f);
    sz->SetKp(1, 0.5f, s * 11.25f);
    SetSize(s * 7.5f, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = new m3dSPL_LINEAR1D;
    if (a) {
        a->flags = 0;
        a->type = 0;
        a->nKp = 0;
        a->kpValueSize = 8;
        a->kpStride = 8;
        a->dim = 1;
        a->kpBytes = 0;
        a->kp = 0;
        a->pad[0] = a->pad[1] = a->pad[2] = 0;
        a->AllocKpList(3);
    }
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.215f, 31.212f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wfallTexFoam);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    flags |= 4;
    speedASpl = 0;
    speedBSpl = 0;
    gravity.x = 0;
    speedA = s * 10.0f;
    gravity.z = 0;
    speedARand = s * 0.5f;
    speedB = s * 3.0f;
    speedBRand = s * 0.15f;
    gravity.y = s * -40.0f;
    flags |= 0x800A409;
}

// v12 0x00485A00  (feb 0x0047B6F0)
// Lit by its place; a foam emitter over each "&h_&nc_area*" (at most 9), started, attached to the instance.
BOOL32 wrsfxWATERFALL::ProcessINIT(float s)
{
    objFILTER_NAME_LEN f((char *)"&h_&nc_area", (int)strlen("&h_&nc_area"));
    objOBJ *list[9];
    inst->flags |= 0x10;
    int n = objMakePtrList(&inst->root, 1, &f, list, 9);
    for (int i = 0; i < n; i++) {
        if (!(foam[i] = new wrsfxWATERFALL_FOAM(s)))
            return 0;
        if (!partSetAreaEmissionObj(foam[i], list[i], -1, 0))
            return 0;
        foam[i]->StartEmission();
        partAttachInst(foam[i], inst);
    }
    return 1;
}

// v12 0x00485AE0  (feb 0x0047B7D0)
BOOL32 wrsfxWATERFALL::ProcessMsg(int id, void *, msgADDR *, int *)
{
    if (id == 1) {
        ProcessINIT(1.0f);
    } else if (id == 2) {
        for (int i = 0; i < 9; i++) {
            if (foam[i])
                foam[i]->__vecDelDtor(1);
            foam[i] = 0;
        }
    }
    return 0;
}

// v12 0x00485B80  (feb 0x0047B870)
entENTITY *__fastcall wrsfxWATERFALL::Create(animINST *)
{
    wrsfxWATERFALL *e = new wrsfxWATERFALL;
    if (!e)
        return 0;
    for (int i = 0; i < 9; i++)
        e->foam[i] = 0;
    return e;
}

// v12 0x004857E0  (feb 0x0047B4D0)
int __fastcall wrsfxWFallInit()
{
    if (!(wfallTexFoam = txmManager->Add((char *)"sfx_wfall_part", 0x40003, 1)))
        return 0;
    return entRegisterClass((char *)"sfx_waterfall", 0x24534658, 0x50, wrsfxWATERFALL::Create, 0, 1) != 0;
}
