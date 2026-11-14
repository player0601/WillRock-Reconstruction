// sfx_water.cpp -- a stream of water ("sfx_water_stream") (Will Rock 1.2, v12.exe; feb obj sfx_water.obj)
//
// A jet poured from where the instance stands, along its direction (WATER size, falling WATER height to the
// plane under it); where it lands, the instance moves (its "emitter": two clouds of spray), and a sound plays
// there. (1.2: the sound heard from 3 to 15 m, feb 15 to 40.)

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"

static txmTEXTURE *waterTex1;                           // v12 wrsfxWATER::pTexWater1 "sfx_water_1"
static txmTEXTURE *waterTex2;                           // v12 wrsfxWATER::pTexWater2 "sfx_water_2"

// v12 0x0064F1B0 its sound (static init v12 0x00484D60): "object_fountain", looped
static sncSOUND_DESCR_3D waterSnd;
static struct wrsfxWATER_SND_INIT {
    wrsfxWATER_SND_INIT()
    {
        waterSnd.name.SetName((char *)"object_fountain");
        waterSnd.flags = 0x40002;
        waterSnd.volume = 0;
        waterSnd.delay = 0;
        waterSnd.count = 0;
        waterSnd.time = 0;
        waterSnd.data = 0;
        waterSnd.dataFlags = 0;
        waterSnd.minDist = 0;
        waterSnd.maxDist = 0;
        waterSnd.vtbl = 0;                              // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
    }
} wrsfxWaterSndInit;

static m3dSPL_LINEAR1D *watSpl(int n)
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

class wrsfxWATER_STREAM_CLOUD_2 : public partEMITTER_PHYS { public: wrsfxWATER_STREAM_CLOUD_2(float s); };
class wrsfxWATER_STREAM_CLOUD_1 : public partEMITTER_PHYS { public: wrsfxWATER_STREAM_CLOUD_1(float s); };
class wrsfxWATER_STREAM : public partEMITTER_PHYS {
public:
    m3dV land;                                          // +0x2C4 (feb +0x2C0) where it meets the plane (PredictCDT)
    wrsfxWATER_STREAM(float s, m3dV *pos, m3dV *dir, float height);
};

// v12 0x00484ED0  (feb 0x0047ABC0)
// Fine spray: 50 a second, 0.8 (+-0.2) s each, 0.7 x s (+-0.3 x s), fading from 188, thrown 0.5 x s (+-0.25 x s)
// up and across.
wrsfxWATER_STREAM_CLOUD_2::wrsfxWATER_STREAM_CLOUD_2(float s) : partEMITTER_PHYS(50.0f, 1.0f)
{
    SetIntensity(50.0f, 0);
    SetTimeLifePart(0.8f, 0.2f);
    SetSize(s * 0.7f, s * 0.3f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = watSpl(2);
    a->SetKp(0, 0.0f, 187.68001f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    speedASpl = 0;
    speedBSpl = 0;
    speedA = s * 0.5f;
    speedARand = s * 0.25f;
    speedB = s * 0.5f;
    speedBRand = s * 0.25f;
    flags |= 0x8000;
    SetTextureRotAnim(180.0f, 0.0f, 0.0f, 0.0f);
    flags |= 4;
    SetTexture(waterTex1);
    flags |= 0x8002408;
}

// v12 0x00485030  (feb 0x0047AD20)
// The cloud: 50 a second, 1.5 (+-0.5) s each, growing 0.755 x s to 1.5 x s, fading in by 0.1 (97) and out,
// turning, 1.5 x s (+-0.1 x s) across.
wrsfxWATER_STREAM_CLOUD_1::wrsfxWATER_STREAM_CLOUD_1(float s) : partEMITTER_PHYS(50.0f, 2.0f)
{
    SetIntensity(50.0f, 0);
    SetTimeLifePart(1.5f, 0.5f);
    m3dSPL_LINEAR1D *sz = watSpl(2);
    sz->SetKp(0, 0.0f, s * 0.755f);
    float b = s * 1.5f;
    sz->SetKp(1, 1.0f, b);
    SetSize(b, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = watSpl(3);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.095f, 97.409996f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 180.0f);
    flags |= 4;
    speedASpl = 0;
    speedB = b;
    speedBSpl = 0;
    speedA = s * 0.0f;
    speedARand = s * 0.0f;
    speedBRand = s * 0.1f;
    flags |= 0x8000;
    SetTexture(waterTex2);
    flags |= 0x8002408;
}

// v12 0x00485200  (feb 0x0047AEF0)
// The jet: 100 a second, 1.1 (+-0.1) s each, s (+-0.1 x s), streaks (along their speed), fading in to 192,
// shot along dir at 3 x s (+-0.8 x s) from within 0.05, falling at 10, stopped by the plane `height` under pos
// (where it will land is worked out at once).
wrsfxWATER_STREAM::wrsfxWATER_STREAM(float s, m3dV *pos, m3dV *dir, float height) : partEMITTER_PHYS(100.0f, 1.3f)
{
    SetIntensity(100.0f, 0);
    SetTimeLifePart(1.1f, 0.1f);
    spread = 0.05f;
    spreadSpl = 0;
    spreadMin = 0;
    cone = 0;
    coneSpl = 0;
    SetSize(s, s * 0.1f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = watSpl(3);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.145f, 33.66f);
    a->SetKp(2, 1.0f, 191.76f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 0.0f, 0.0f, 0.0f);
    SetTextureScale(0.25f, 0.0f, 1.0f, 0.0f);
    flags |= 0x100004;
    gravity.x = 0;
    gravity.y = -10.0f;
    gravity.z = 0;
    speedA = s * 3.0f;
    speedASpl = 0;
    speedARand = s * 0.8f;
    SetTexture(waterTex2);
    flags |= 0x408;
    m3dV plane[2];
    plane[0].x = 0;
    plane[0].y = pos->y - height;
    plane[0].z = 0;
    plane[1] = m3dVUnitY;
    SetCDTPlane(1, 0, (m3dPLANE *)plane);
    flags |= 0x30000;
    SetPosDirSpeed(pos, dir, 0);
    PredictCDT(&land, 0);
    flags |= 0x8000000;
}

// v12 0x00485420  (feb 0x0047B110)
// 1: the jet from the instance (WATER height 1, size 1); the instance moved to where it lands (0.15 up), its
// "emitter" (hidden) giving the two clouds; all three attached to it; the sound (3..15 m). 2: the emitters gone.
BOOL32 wrsfxWATER::ProcessMsg(int id, void *, msgADDR *, int *)
{
    if (id == 1) {
        m3dV pos, dir;
        inst->GetPos(&pos);
        inst->GetDir(&dir);
        float height = 1.0f, size = 1.0f;
        inst->GetFloat((char *)"WATER", (char *)"height", &height);
        inst->GetFloat((char *)"WATER", (char *)"size", &size);
        wrsfxWATER_STREAM *st = new wrsfxWATER_STREAM(size, &pos, &dir, height);
        stream = st;
        if (!st)
            return 0;
        m3dV at = { st->land.x, st->land.y + 0.15f, st->land.z };
        ((m3dMATR *)inst->matr)->_MakeLCS2WCS_VY(&at, &m3dVUnitY);
        inst->Invalidate(0x80);
        if (!(emitterObj = objFindName(inst->root, (char *)"emitter")))
            return 0;
        emitterObj->SetStateProcYes(0x1008, 0);
        inst->flags |= 0x10;
        if (!(cloud1 = new wrsfxWATER_STREAM_CLOUD_1(size)))
            return 0;
        if (!(cloud2 = new wrsfxWATER_STREAM_CLOUD_2(size)))
            return 0;
        if (!partSetAreaEmissionObj(cloud1, emitterObj, -1, 0))
            return 0;
        if (!partSetAreaEmissionObj(cloud2, emitterObj, -1, 0))
            return 0;
        stream->StartEmission();
        cloud1->StartEmission();
        cloud2->StartEmission();
        partAttachInst(stream, inst);
        stream->flags |= 0x20000000;
        partAttachInst(cloud1, inst);
        cloud1->flags |= 0x20000000;
        partAttachInst(cloud2, inst);
        cloud2->flags |= 0x20000000;
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;      // (a sncCHANNEL_SPH there: a sphere)
        if (c) {
            c->minDist = 3.0f;
            c->maxDist = 15.0f;
        }
        voice = c;
        if (voice) {
            gsSysSound->AddChannel(voice, this, 0);
            voice->Play(&waterSnd, 0);
        }
    } else if (id == 2) {
        if (stream)
            stream->__vecDelDtor(1);
        stream = 0;
        if (cloud1)
            cloud1->__vecDelDtor(1);
        cloud1 = 0;
        if (cloud2)
            cloud2->__vecDelDtor(1);
        cloud2 = 0;
    }
    return 0;
}

// v12 0x00484E50  (feb 0x0047AB40)
entENTITY *__fastcall wrsfxWATER::Create(animINST *)
{
    wrsfxWATER *e = new wrsfxWATER;
    if (!e)
        return 0;
    e->emitterObj = 0;
    e->stream = 0;
    e->cloud1 = 0;
    e->cloud2 = 0;
    e->voice = 0;
    return e;
}

// v12 0x00484DC0  (feb 0x0047AAB0)
int __fastcall wrsfxWaterInit()
{
    if (!(waterTex1 = txmManager->Add((char *)"sfx_water_1", 0x40003, 1)))
        return 0;
    if (!(waterTex2 = txmManager->Add((char *)"sfx_water_2", 0x40003, 1)))
        return 0;
    if (!entRegisterClass((char *)"sfx_water_stream", 0x24534658, 0x50, wrsfxWATER::Create, 0, 1))
        return 0;
    if (!sncLinkSndListData(1, &waterSnd)) {
        apMsg((char *)"Cannot link sound data");
        return 0;
    }
    return 1;
}
