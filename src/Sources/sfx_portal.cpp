// sfx_portal.cpp -- the portals (Will Rock 1.2, v12.exe; feb obj sfx_portal.obj)
//
// Flames and sparks over the object "emitter" (which is not drawn), a looped sound, and wrsfxPORTAL_VIEW: the picture
// [PORTAL] tex over the portal, wobbling and flickering. The IActions START / STOP. The "object_portal_appear" sound is
// linked, and its channel made, but nothing here plays it.

#include "game.h"
#include "../../include/engine_render/shader.h"
#include "../gs/gs_mp_server.h"
#include "../m3d/m3d_spl.h"

txmTEXTURE *wrsfxPORTAL::pTexFlame;                     // v12 0x0064F120 "sfx_flame"
txmTEXTURE *wrsfxPORTAL::pTexSpark;                     // v12 0x0064F04C "sfx_mcg_glow"
sncSOUND_DESCR_3D wrsfxPORTAL::snd[2];                  // v12 0x0064F050 "object_portal_appear", "object_flame" (looped)

static struct wrsfxPORTAL_SND_INIT {
    wrsfxPORTAL_SND_INIT()
    {
        static const char *names[2] = { "object_portal_appear", "object_flame" };
        for (int i = 0; i < 2; i++) {
            sncSOUND_DESCR_3D *d = &wrsfxPORTAL::snd[i];
            d->name.SetName((char *)names[i]);
            d->flags = i == 0 ? 0x10000 : 0x10002;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = 0.0f;
            d->maxDist = 0.0f;
            d->vtbl = 0;                                // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} wrsfxPortalSndInit;

// the m3dSPL_LINEAR1D constructor inline (v12 0x0047FAE7)
static m3dSPL_LINEAR1D *portalSpl(int n)
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

// v12 0x0047F360  (feb 0x00474FF0)
int __fastcall wrsfxPortalInit()
{
    if (!(wrsfxPORTAL::pTexFlame = txmManager->Add((char *)"sfx_flame", 0x40003, 1)))
        return 0;
    if (!(wrsfxPORTAL::pTexSpark = txmManager->Add((char *)"sfx_mcg_glow", 0x40003, 1)))
        return 0;
    if (!pteManager->RegisterClass((char *)"wrsfx_portal", 0x1E, wrsfxPORTAL_VIEW::Create))
        return 0;
    if (!entRegisterClass((char *)"sfx_portal", 0x24534658, 0x50, wrsfxPORTAL::Create, 0, 1))
        return 0;
    if (!sncLinkSndListData(2, wrsfxPORTAL::snd)) {
        apMsg((char *)"Cannot link sound data");
        return 0;
    }
    return 1;
}

// v12 0x0047F410  (feb 0x004750A0)
entENTITY *__fastcall wrsfxPORTAL::Create(animINST *i)
{
    void *mem = operator new(0x187);
    if (!mem)
        return 0;
    wrsfxPORTAL *p = new (mem) wrsfxPORTAL;
    p->emitter = 0;
    p->flame = 0;
    p->sparks = 0;
    p->view = 0;
    p->sndAppear = 0;
    p->sndLoop = 0;
    p->portalFlags = 0;
    return p;
}

wrsfxPORTAL_VIEW::wrsfxPORTAL_VIEW()
{
    time = 0;
    speed = 0.7f;
    wobble = 0.05f;
    noiseScale = 3.0f;
}

// v12 0x0047F4A0  (feb 0x00475130)
pteSHADER *__fastcall wrsfxPORTAL_VIEW::Create(animINST *i, objOBJ *o)
{
    return new wrsfxPORTAL_VIEW;
}

// v12 0x0047F500
char *wrsfxPORTAL_VIEW::GetBlendTypeDef()
{
    return (char *)"blend";
}

// v12 0x0047F530  (feb 0x004751C0)
// The picture is laid out from the "&ds_view" object's position.
void wrsfxPORTAL_VIEW::Init()
{
    pteEMAP::Init();
    matrL2W.Identity();
    if (inst) {
        viewObj = objFindName(inst->root, (char *)"&ds_view");
        if (viewObj) {
            inst->Validate(4);
            memcpy(&matrL2W, viewObj->ltm, sizeof(float) * 16);
        }
    }
    SetStateRend(0xBB);
}

// v12 0x0047F590  (feb 0x00475220)
void wrsfxPORTAL_VIEW::PrepareFrame(scnSCENE *s, camCAMERA *c)
{
    pteEMAP::PrepareFrame(s, c);
    if (viewObj)
        viewObj->inst->Validate(0x600);
}

// v12 0x0047F5D0  (feb 0x00475260)
void wrsfxPORTAL_VIEW::Update(float dt)
{
    time += dt;
}

// v12 0x0047F5F0  (feb 0x00475280)
// A vertex's texture coordinates, one texture over 7 units round the portal's centre moved by two noises, and its alpha
// by a third: 100 + 155 * sqrt(noise).
int wrsfxPORTAL_VIEW::ShadeVert(objOBJ *o, int v, void *out)
{
    m3dV *src = &o->geom->vert[v];
    m3dV w;
    if (o->state & 1) {
        w = *src;
    } else {
        const float *l = o->ltm;
        w.x = src->x * l[0] + src->y * l[4] + src->z * l[8] + l[12];
        w.y = src->x * l[1] + src->y * l[5] + src->z * l[9] + l[13];
        w.z = src->x * l[2] + src->y * l[6] + src->z * l[10] + l[14];
    }
    const float *m = (const float *)&matrW2L;
    m3dV q;
    q.x = w.x * m[0] + w.y * m[4] + w.z * m[8] + m[12];
    q.y = w.x * m[1] + w.y * m[5] + w.z * m[9] + m[13];
    q.z = w.x * m[2] + w.y * m[6] + w.z * m[10] + m[14];
    m3dV n;
    n.x = q.x * noiseScale;
    n.y = q.y * noiseScale + speed * time;
    n.z = q.z * noiseScale;
    float d0 = (wrVCall<float>(m3dNoise, 8, &n) + 1.0f) * 0.5f * wobble;
    n.z += 3.1415927f * 39.0f;
    float d1 = (wrVCall<float>(m3dNoise, 8, &n) + 1.0f) * 0.5f * wobble;
    u8 *d = (u8 *)out;
    *(float *)(d + 1) = q.x * 0.14285715f + d0 + 0.5f;
    *(float *)(d + 5) = 0.5f - q.z * 0.14285715f + d1;
    n.x = q.x * 1.5f;
    n.y = q.y * 1.5f + time;
    n.z = q.z * 1.5f;
    float k = (float)pow((wrVCall<float>(m3dNoise, 8, &n) + 1.0f) * 0.5f, 0.5);
    float a;
    if (k < 0)
        a = 100.0f;
    else if (k > 1.0f)
        a = 255.0f;
    else
        a = k * 155.0f + 100.0f;
    *(u32 *)(d + 9) = (((u32)(int)a & 0xFF) << 24) | 0xFFFFFF;
    return 1;
}

// v12 0x004802A0
int wrsfxPORTAL_VIEW::ShadeFace(objOBJ *, int, pteSDR_INFO *, ptePOLY_DATA *)
{
    return 0;
}

// v12 0x0048D590
int wrsfxPORTAL_VIEW::IsActive(int)
{
    return 1;
}

// v12 0x004E2EB0
void wrsfxPORTAL_VIEW::GetMatrL2W(m3dMATR *out)
{
    *out = matrL2W;
}

// v12 0x0047FAC0  (feb 0x00475750)
wrsfxPORTAL_FLAME::wrsfxPORTAL_FLAME() : partEMITTER_PHYS(1000.0f, 1.0f)
{
    SetTimeLifePart(0.65f, 0.35f);
    m3dSPL_LINEAR1D *sz = portalSpl(3);
    sz->SetKp(0, 0.0f, 0.0f);
    sz->SetKp(1, 0.35f, 0.7f);
    sz->SetKp(2, 1.0f, 0.0f);
    SetSize(0.7f, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = portalSpl(3);
    a->SetKp(0, 0.0f, 61.2f);
    a->SetKp(1, 0.1f, 198.135f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFF7F7F7F, 0, a);
    gravity.x = 0;
    gravity.y = 7.0f;
    gravity.z = 0;
    speedA = 0;
    speedARand = 1.0f;
    speedASpl = 0;
    speedB = 0;
    speedBRand = 1.0f;
    speedBSpl = 0;
    flags |= 0x8000;
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 150.0f);
    flags |= 4;
    SetTexture(wrsfxPORTAL::pTexFlame);
    flags |= 0x400;
    flags |= 8;
    flags |= 0x2000;
    flags |= 1;
    SetDegFarParams(20.0f, 40.0f, 2.0f, 0.3f);
    flags |= 0x8000000;
}

// v12 0x0047FC90  (feb 0x00475920)
wrsfxPORTAL_SPARKS::wrsfxPORTAL_SPARKS() : partEMITTER_PHYS(200.0f, 0.8f)
{
    SetTimeLifePart(0.6f, 0.2f);
    SetSize(0.2f, 0.0f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = portalSpl(3);
    a->SetKp(0, 0.0f, 127.5f);
    a->SetKp(1, 0.435f, 255.0f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    gravity.x = 0;
    gravity.y = 7.0f;
    gravity.z = 0;
    speedA = 0;
    speedARand = 1.0f;
    speedASpl = 0;
    flags |= 0x8000;
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 0.0f);
    flags |= 4;
    SetTexture(wrsfxPORTAL::pTexSpark);
    flags |= 0x2000;
    SetDegFarParams(20.0f, 40.0f, 2.0f, 0.3f);
    flags |= 0x8000000;
}

// v12 0x0047FE10  (feb 0x00475AA0)
BOOL32 wrsfxPORTAL::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 1) {
        emitter = objFindName(inst->root, (char *)"emitter");
        if (!emitter)
            return 0;
        emitter->SetStateProcYes(0x1008, 0);
        inst->flags |= 0x10;
        if (!(flame = new wrsfxPORTAL_FLAME))
            return 0;
        if (!(sparks = new wrsfxPORTAL_SPARKS))
            return 0;
        if (!partSetAreaEmissionObj(flame, emitter, -1, 0))
            return 0;
        if (!partSetAreaEmissionObj(sparks, emitter, -1, 0))
            return 0;
        partAttachInst(flame, inst);
        partAttachInst(sparks, inst);
        view = 0;
        char s[0x40];
        if (inst->GetStr((char *)"PORTAL", (char *)"tex", s, 0x40)) {
            if (txmTEXTURE *t = txmManager->Add(s, 0x100, 1)) {
                view = new wrsfxPORTAL_VIEW;
                view->SetInst(inst);
                view->Register();
                view->tex = t;
            }
        }
        if (!(sndAppear = new sncCHANNEL))
            return 0;
        gsSysSound->AddChannel(sndAppear, this, 0);
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        if (!c)
            return 0;
        c->minDist = 5.0f;
        c->maxDist = 20.0f;
        sndLoop = c;
        gsSysSound->AddChannel(sndLoop, this, 0);
        int start = 1;
        inst->GetBool((char *)"PORTAL", (char *)"isStartSFX", &start);
        Enable(start);
    } else if (id == 2) {
        flame->__vecDelDtor(1);
        flame = 0;
        sparks->__vecDelDtor(1);
        sparks = 0;
    } else if (id == 0x3F8) {
        apNAME *n = (apNAME *)((u8 *)data + 8);
        if (n->IsName((char *)"START"))
            Enable(1);
        else if (n->IsName((char *)"STOP"))
            Enable(0);
    }
    return 0;
}

// v12 0x004801D0  (feb 0x00475E60)
// As the game has it: both ways set bit 1, so after the first start any further call stops it.
void wrsfxPORTAL::Enable(int on)
{
    if (on && !(portalFlags & 1)) {
        flame->StartEmission();
        sparks->StartEmission();
        if (view)
            view->Enable(1);
        sndLoop->Play(&snd[1], 0);
        portalFlags |= 1;
        pendingEvents |= 1;
    } else if (portalFlags & 1) {
        flame->StopEmission();
        sparks->StopEmission();
        if (view)
            view->Enable(0);
        sndLoop->Play(0, 0);
        portalFlags |= 1;
        pendingEvents |= 2;
    }
}
