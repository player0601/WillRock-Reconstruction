// sfx_appear.cpp -- an enemy appearing (Will Rock 1.2, v12.exe; feb obj sfx_appear.obj)
//
// A cloud and pieces thrown up out of the ground (each large piece leaves small ones), a hole in the floor, a sound.
// Not rebuilt: wrsfxSCORCHMARK (the pteSPOT shaders), objFILTER_PART_SOURCE; the rest of wrsfxInit's list.

#include "game.h"
#include "../engine/engine.h"   // cdtCOLL_SCN, cdtOflPtr
#include "../m3d/m3d_spl.h"
#include "../m3d/m3d_vol.h"
#include "../../include/engine_render/shader.h"
#include "../../include/sfx_sob/sfx_api.h"

void __fastcall m3dRotatePointAxis(m3dV *center, m3dV *axis, float deg, m3dV *p, m3dV *out);  // v12 0x00560E80
u32 __fastcall gsEffEnvirColor(cdtINFO *ci, m3dCOLOR *col, m3dCOLOR *dyn);                      // v12 0x005130E0
struct wrsfxSCORCHMARK {
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
                                                        // v12 0x00467DE0 (feb 0x0045DED0)
};

txmTEXTURE *wrsfxAPPEAR::pTexCloud;                     // v12 "sfx_cloud"
txmTEXTURE *wrsfxAPPEAR::pTexPart;                      // "sfx_appear_part"
txmTEXTURE *wrsfxAPPEAR::pTexHole;                      // v12 0x0064EC68 "sfx_appear_hole"

static sncSOUND_DESCR_3D wrsfxAppearSnd;                // v12 0x0064EBF0
static struct wrsfxAPPEAR_SND_INIT {
    wrsfxAPPEAR_SND_INIT()                              // v12 0x004677D0
    {
        wrsfxAppearSnd.name.SetName((char *)"char_appear");
        wrsfxAppearSnd.flags = 0;
        wrsfxAppearSnd.volume = 0;
        wrsfxAppearSnd.delay = 0;
        wrsfxAppearSnd.count = 0;
        wrsfxAppearSnd.time = 0;
        wrsfxAppearSnd.data = 0;
        wrsfxAppearSnd.dataFlags = 0;
        wrsfxAppearSnd.minDist = 80.0f;
        wrsfxAppearSnd.maxDist = 300.0f;
        wrsfxAppearSnd.vtbl = 0;                        // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
    }
} wrsfxAppearSndInit;

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30

static m3dSPL_LINEAR1D *appSpl(int n)
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

// v12 0x00467830  (feb 0x0045DB60)
// Its own part, then the blood; the list of the other effects' Inits after that is not rebuilt.
int __fastcall wrsfxInit()
{
    if (!(wrsfxAPPEAR::pTexCloud = txmManager->Add((char *)"sfx_cloud", 0x40003, 1)))
        return 0;
    if (!(wrsfxAPPEAR::pTexPart = txmManager->Add((char *)"sfx_appear_part", 0x40003, 1)))
        return 0;
    if (!(wrsfxAPPEAR::pTexHole = txmManager->Add((char *)"sfx_appear_hole", 0x40003, 1)))
        return 0;
    if (!entRegisterClass((char *)"sfx_appear", 0x24534658, 0x2D, wrsfxAPPEAR::Create, 0, 1))
        return 0;
    if (!sncLinkSndListData(1, &wrsfxAppearSnd)) {
        apMsg((char *)"Cannot link sound data");
        return 0;
    }
    return wrsfxBloodInit();
}

// v12 0x00468160  (feb 0x0045E340)
wrsfxAPPEAR_CLOUD::wrsfxAPPEAR_CLOUD(float) : partEMITTER_PHYS(0xF, 50.0f, 3.0f)
{
    m3dSPL_LINEAR1D *in = appSpl(2);
    in->SetKp(0, 0.125f, 50.0f);
    in->SetKp(1, 0.41666666f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(4.0f, 0.0f);
    flags |= 0x200;
    SetTimeLifePart(2.0f, 1.0f);
    SetSize(5.0f, 1.0f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = appSpl(3);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.095f, 51.0f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 72.0f);
    flags |= 4;
    speedA = 2.0f;
    speedARand = 0;
    speedASpl = 0;
    cone = 180.0f;
    coneSpl = 0;
    SetTexture(wrsfxAPPEAR::pTexCloud);
    flags |= 0x2408;
}

// v12 0x00468310  (feb 0x0045E4F0)
wrsfxAPPEAR_LARGE_PART::wrsfxAPPEAR_LARGE_PART(float) : partEMITTER_PHYS(0x14, 150.0f, 3.0f)
{
    m3dSPL_LINEAR1D *in = appSpl(2);
    in->SetKp(0, 0.0f, 150.0f);
    in->SetKp(1, 0.25f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(4.0f, 0.0f);
    flags |= 0x200;
    SetTimeLifePart(3.0f, 0.0f);
    SetSize(0.5f, 0.0f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = appSpl(2);
    a->SetKp(0, 0.98f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 240.0f);
    flags |= 4;
    gravity.x = 0;
    gravity.y = -10.0f;
    gravity.z = 0;
    speedA = 10.0f;
    speedARand = 5.0f;
    speedASpl = 0;
    speedB = 2.0f;
    speedBRand = 0;
    speedBSpl = 0;
    flags |= 0x8000;
    SetTexture(wrsfxAPPEAR::pTexPart);
    SetTextureScale(0.75f, 0.25f, 0.75f, 0.25f);
    SetCDTParam(0.0f, 0.3f, 0.4f, -1);
    flags |= 0x32008;
}

// v12 0x004684E0  (feb 0x0045E6C0)
wrsfxAPPEAR_SMALL_PART::wrsfxAPPEAR_SMALL_PART(float) : partEMITTER_PHYS(0x8C, 10.0f, 4.0f)
{
    m3dSPL_LINEAR1D *in = appSpl(2);
    in->SetKp(0, 0.0f, 10.0f);
    in->SetKp(1, 1.25f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(4.0f, 0.0f);
    SetTimeLifePart(3.0f, 1.0f);
    SetSize(0.2f, 0.0f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = appSpl(3);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 0.7f, 255.0f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 168.0f);
    flags |= 4;
    gravity.x = 0;
    gravity.y = -10.0f;
    gravity.z = 0;
    speedA = 2.0f;
    speedARand = 0;
    speedASpl = 0;
    SetTexture(wrsfxAPPEAR::pTexPart);
    SetTextureScale(0.715f, 0.285f, 0.715f, 0.285f);
    SetCDTParam(0.0f, 0.3f, 0.4f, 5);
    flags |= 0x10008;
}

// v12 0x00467B80 (name ours; 1.2)
// The plane the pieces land on: the highest floor under three points on a circle round the place.
BOOL32 appFloorPlane(m3dV *c, m3dV *plane, float r)
{
    cdtREFINE ref(10, 0);
    cdtINFO ci = {};
    plane[1] = m3dVUnitY;
    plane[0] = m3dVZero;
    plane[0].y = -2000000.0f;
    m3dV t = { -m3dVUnitY.y, m3dVUnitY.x, m3dVUnitY.z };     // (v12 0x00565900 a perpendicular)
    m3dNormalize(&t);
    m3dV pts[3];
    pts[0].x = t.x * r + c->x;
    pts[0].y = t.y * r + c->y;
    pts[0].z = t.z * r + c->z;
    for (int i = 1; i < 3; i++)
        m3dRotatePointAxis(c, &m3dVUnitY, (float)i * 120.0f, &pts[0], &pts[i]);
    static m3dV down = { 0, -1, 0 };
    for (int i = 0; i < 3; i++) {
        pts[i].y += 0.1f;
        if (!gsScenePtr->ShootRay(&pts[i], &down, 20.0f, 0, &ref, &ci))
            return 0;
        if (plane[0].y <= ci.point[1])
            plane[0].y = ci.point[1];
    }
    return 1;
}

// v12 0x004686D0  (feb 0x0045E8B0)
// An enemy that flies: the other effect (sfx_opp_appear). Else not if another hole is within 6 m.
int __fastcall wrsfxAPPEAR_Start(m3dV *pos, int opp, cdtINFO *cdt)
{
    int sig = 0x41504546;                               // 'FEPA'
    animCREATE_DATA cd;
    cd.matr._MakeLCS2WCS_VY(pos, &m3dVUnitY);
    if (opp)
        return entCreate(gsScenePtr, (char *)"sfx_opp_appear", (char *)"", &cd, 0, 0) != 0;
    if (pteManager->FindShaderAround(1, &sig, pos, 6.0f))
        return 1;
    struct { cdtINFO *cdt; int noPlane; } data = { cdt, 1 };
    return entCreate(gsScenePtr, (char *)"sfx_appear", (char *)"sfx_appear", &cd, &data, 0) != 0;
}

// v12 0x00468790  (feb 0x0045E970)
BOOL32 wrsfxAPPEAR::ProcessMsg(int id, void *d, msgADDR *from, int *ret)
{
    if (id != 1)
        return 0;
    m3dV p;
    inst->GetPos(&p);
    wrsfxAPPEAR_CLOUD *cloud = new wrsfxAPPEAR_CLOUD(1.0f);
    wrsfxAPPEAR_LARGE_PART *large = new wrsfxAPPEAR_LARGE_PART(1.0f);
    wrsfxAPPEAR_SMALL_PART *small_ = new wrsfxAPPEAR_SMALL_PART(1.0f);
    cdtINFO *ci = *(cdtINFO **)d;
    u32 color = ci ? gsEffEnvirColor(ci, 0, 0) : 0;
    wrsfxSCORCHMARK::Add(3.0f, 10.0f, pTexHole, &p, color, 0x41504546, 0, 0);
    objOBJ *em = objFindName(inst->root, (char *)"emitter");
    if (!em)
        return 0;
    em->SetStateProcYes(0x1008, 0);
    inst->flags |= 0x10;
    large->AttachEmitter(small_, 0, 3);
    large->flags |= 0xC0000;
    if (!partSetAreaEmissionObj(cloud, em, -1, 0) || !partSetAreaEmissionObj(large, em, -1, 0))
        return 0;
    if (!WR_FIELD(int, d, 4)) {
        m3dV plane[2];
        appFloorPlane(&p, plane, 5.0f);
        large->SetCDTPlane(1, 0, (m3dPLANE *)plane);
        small_->SetCDTPlane(1, 0, (m3dPLANE *)plane);
    }
    large->StartEmission();
    small_->StartEmission();
    cloud->StartEmission();
    gsSysSound->Play(&wrsfxAppearSnd, &p);
    msgSystem.PostMsg(this, 2, this);
    return 0;
}

// v12 0x00468970  (feb 0x0045EBA0)
entENTITY *__fastcall wrsfxAPPEAR::Create(animINST *i)
{
    return new wrsfxAPPEAR;
}

// v12 0x00467D10  (feb 0x0045DE00)
// The floor under pos: the static level objects a ray from 0.5 over pos down to the level's lowest point passes;
// the first one marked floor (flag 1) gives its box. 0 when there is none.
int __fastcall wrsfxFindFloor(m3dV *pos, m3dBOX *box)
{
    m3dV a = *pos, b = *pos;
    a.y += 0.5f;
    cdtCOLL_SCN *c = gsScenePtr->coll;
    b.y = c->grid.bbMin.y;
    c->MakeIndListRay(&a, &b, cdtOflPtr, 0);
    box->CalcStart();
    for (int i = 0; i < cdtOflPtr->nObj; i++) {
        objOBJ *o = (objOBJ *)gsScenePtr->coll->GetID2Ptr(cdtOflPtr->obj[i]);
        if (o->unk0C & 1) {
            box->CalcUpdate((m3dBOX *)&o->bbox);
            return 1;
        }
    }
    return 0;
}
