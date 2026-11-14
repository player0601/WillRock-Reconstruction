// sfx_blood.cpp -- blood sprays and meat chunks (Will Rock 1.2, v12.exe; feb obj sfx_blood.obj)
//
// wrsfxStartBlood picks the spray for a hit (one, two to the sides, four upwards) and makes the flying
// droplets (wrsfxBLOOD_AIR, a particle emitter). wrsfxStartChunkMeat throws the gibs (wrsfxCHUNK, a physPHYS
// entity "sfx_chunk"); a minotaur's pieces are "ai_mino_chunk_regen" that grow a new minotaur.
//
// Not rebuilt: the decals (wrsfxBLOOD_SURF / wrsfxBLOOD_AIR_SURF are pteSPOT / pteSPOT_ANIM shaders, which are
// not rebuilt). Their places are marked PLACEHOLDER.

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../gs/gs_mp_server.h"
#include "../../include/sfx_sob/sfx_api.h"

u32 __fastcall gsEffEnvirColor(cdtINFO *ci, m3dCOLOR *col, m3dCOLOR *dyn);   // v12 0x005130E0 (gs:gs_eff)
int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list);         // v12 0x004B1E30 (drv:Snd_chn.obj)

txmTEXTURE *wrsfxBloodTexAir;                           // "sfx_blood_anim_8x4" the droplets
txmTEXTURE *wrsfxBloodTexGround;                        // "sfx_blood_floor_1" the pools (decals, not rebuilt)
static const char wrsfxNameChunkDefault[] = "chunk_meat";

// v12 0x0064EC98: "chunk_hit_floor", a chunk lands (static init v12 0x0046A8F0)
static sncSOUND_DESCR_3D chunkHitSnd;
static struct wrsfxBLOOD_SND_INIT {
    wrsfxBLOOD_SND_INIT()
    {
        sncSOUND_DESCR_3D *d = &chunkHitSnd;
        d->name.SetName((char *)"chunk_hit_floor");
        d->flags = 0;
        d->volume = 0.0f;
        d->delay = 0;
        d->count = 0;
        d->time = 0;
        d->data = 0;
        d->dataFlags = 0;
        d->minDist = 30.0f;
        d->maxDist = 200.0f;
        d->vtbl = 0;                                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
    }
} wrsfxBloodSndInit;

static m3dSPL_LINEAR1D *bloodSpl(int n)
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

struct bloodPLANE {                                     // an m3dPLANE as the emitters take it: a point and the normal
    m3dV pos;
    m3dV n;
};

// ---- the droplets ------------------------------------------------------------------------------------------
// size v12 0x2D0 (feb 0x2CC); vtable v12 0x00601010 (name ours: slot +0x2C overridden)
class wrsfxBLOOD_AIR : public partEMITTER_PHYS {
public:
    float surfTime;                                     // v12 +0x2C4 the pool's growing time (CreateBloodAir arg 7)
    float surfSize;                                     // v12 +0x2C8 the pool's radius (arg 6)
    int   hitDone;                                      // v12 +0x2CC the first droplet that lands makes the pool
    wrsfxBLOOD_AIR(float size, m3dV *pos, m3dV *dir, m3dV *vel, m3dPLANE *plane, float surfSize, float surfTime,
                   m3dCOLOR *col);
    int PartApplyCDT(partPART *p, m3dV *v, float t);
};

// v12 0x0046C1C0  (feb 0x00462340)
wrsfxBLOOD_AIR::wrsfxBLOOD_AIR(float size, m3dV *pos, m3dV *dir, m3dV *vel, m3dPLANE *plane, float sSize,
                               float sTime, m3dCOLOR *col)
    : partEMITTER_PHYS(0x2D, 200.0f, 5.0f)
{
    surfTime = sTime;
    surfSize = sSize;
    hitDone = 0;
    // a puff: 200 a second for 1/8 s, from 1/24 s on
    m3dSPL_LINEAR1D *in = bloodSpl(4);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.041666668f, 200.0f);
    in->SetKp(2, 0.125f, 200.0f);
    in->SetKp(3, 0.20833333f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(0.6f, 0.0f);
    flags |= 0x200;
    m3dSPL_LINEAR1D *sz = bloodSpl(2);
    sz->SetKp(0, 0.0f, size * 0.425f);
    sz->SetKp(1, 1.0f, size * 0.7f);
    SetSize(size * 0.75f, size * 0.25f, sz, 1.0f);
    u32 c = (u32)(int)col->a;
    c = (c << 8) | ((u32)(int)col->r & 0xFF);
    c = (c << 8) | ((u32)(int)col->g & 0xFF);
    c = (c << 8) | ((u32)(int)col->b & 0xFF);
    SetColor(c, 0, 0);
    gravity.x = 0;
    gravity.y = -30.0f;
    gravity.z = 0;
    speedA = 2.5f;
    speedARand = 0.625f;
    speedASpl = 0;
    speedB = size * 1.25f;
    speedBRand = size * 0.3125f;
    speedBSpl = 0;
    speedK = 0.7f;
    speedKRand = 0;
    spreadSpl = 0;
    spreadMin = 0;
    spread = size * 0.05f;
    SetTexture(wrsfxBloodTexAir);
    flags |= 0x408;
    SetCDTPlane(1, 0, plane);
    flags |= 0x30000;
    SetPosDirSpeed(pos, dir, vel);
    m3dV hit;
    float t;
    PredictCDT(&hit, &t);
    SetTimeLifePart(t + 1.0f, 0.0f);
    float fps = 1.0f / t;
    if (fps >= 200.0f)
        fps = 200.0f;
    SetTextureFrameAnim(8, 4, fps, 0.0f);
    flags |= 2;
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 0.0f);
    flags |= 4;
}

// v12 0x0046C4D0  (feb 0x00462650)
// A droplet hit the floor plane: the first one leaves a growing pool (unless one is near already).
int wrsfxBLOOD_AIR::PartApplyCDT(partPART *p, m3dV *v, float t)
{
    if (!partEMITTER_PHYS::PartApplyCDT(p, v, t))
        return 0;
    if (!hitDone) {
        hitDone = 1;
        // PLACEHOLDER (test): the pool (pteMANAGER::FindShaderAround 2 x surfSize, then a wrsfxBLOOD_AIR_SURF
        // pteSPOT_ANIM of radius surfSize growing for surfTime in the emitter's colour) is not made: pteSPOT is
        // not rebuilt.
    }
    return 1;
}

// ---- init --------------------------------------------------------------------------------------------------

// v12 0x0046A950  (feb 0x00460B80)
int __fastcall wrsfxBloodInit()
{
    if (!(wrsfxBloodTexAir = txmManager->Add((char *)"sfx_blood_anim_8x4", 0x40003, 1)))
        return 0;
    if (!(wrsfxBloodTexGround = txmManager->Add((char *)"sfx_blood_floor_1", 0x40003, 1)))
        return 0;
    if (!entRegisterClass((char *)"sfx_chunk", 0x43484E4B, 0x32, wrsfxCHUNK::Create, 0, 1))
        return 0;
    if (!sncLinkSndListData(1, &chunkHitSnd)) {
        apMsg((char *)"Cannot link sound data");
        return 0;
    }
    return 1;
}

// ---- the sprays --------------------------------------------------------------------------------------------

// v12 0x0046AA30  (feb 0x00460C10)
// One spray of droplets from pos; without a direction it flies towards the camera, 90 degrees round it.
void __fastcall wrsfxCreateBloodAir(m3dV *pos, m3dV *dir, float size, m3dV *vel, m3dPLANE *plane, float surfSize,
                                    float surfTime, m3dCOLOR *col)
{
    if (wrAppState & 2)
        return;
    m3dV v = vel ? *vel : m3dVZero;
    m3dV d;
    if (!dir) {
        m3dV c;
        gsCameraPtr->GetOrigin(&c);
        d.x = c.x - pos->x;
        d.y = c.y - pos->y;
        d.z = c.z - pos->z;
        m3dNormalize(&d);
        m3dMakeVUnitConusRandom(&d, 90.0f, &d);
    } else {
        d = *dir;
    }
    wrsfxBLOOD_AIR *e = new wrsfxBLOOD_AIR(size, pos, &d, &v, plane, surfSize, surfTime, col);
    if (e) {
        e->StartEmission();
        plrPlayer->NotifySFXBloodHands(pos);
    }
}

// v12 0x0046AB50  (feb 0x00460D20)
// A hit's blood. type: 0 one spray (from 'from' with vel), 1/2 two to the sides, 4 a pool on the surface,
// 6/7 four upwards; 3 and 5 nothing. kind != 0: the narrower, stronger spray. The colour is the light where
// it lands.
void __fastcall wrsfxStartBlood(int type, int kind, m3dV *pos, m3dV *dir, m3dV *vel, m3dV *from)
{
    (void)dir;                                          // (not read in 1.2 either)
    if (wrAppState & 2)
        return;
    cdtREFINE ref(2, 0);
    cdtINFO ci;
    memset(&ci, 0, sizeof(ci));
    bloodPLANE plane;
    plane.pos = *pos;
    plane.n = m3dVUnitY;
    m3dCOLOR col = { 0, 0, 0, 0 };
    u32 c;
    int found = gsScenePtr->FloorPlace(pos, 2000000.0f, &plane.pos, 0.05f, 0.0f, 0, &ref, &ci);
    if (!found)
        found = gsScenePtr->GetClosestPoint(pos, 3.0f, 0, &ref, &ci);
    if (found) {
        c = gsEffEnvirColor(&ci, 0, 0);
        col.b = (float)(c & 0xFF);
        col.g = (float)((c >> 8) & 0xFF);
        col.r = (float)((c >> 16) & 0xFF);
        col.a = (float)(c >> 24);
    } else {
        col.r = col.g = col.b = col.a = 255.0f;
    }
    m3dV d;
    if (from) {
        d.x = from->x - pos->x;
        d.y = from->y - pos->y;
        d.z = from->z - pos->z;
    } else {
        d = m3dVUnitX;
    }
    m3dV v;
    switch (type) {
    case 0: {
        m3dV *at = from ? from : pos;                   // (1.2 passes 'from' with no check; every caller gives one)
        if (kind) {
            m3dMakeVUnitConusRandom(&m3dVUnitY, 45.0f, &v);
            float k = m3dRandRange(3.5f, 7.0f);
            v.x *= k;
            v.y *= k;
            v.z *= k;
            wrsfxCreateBloodAir(at, 0, 0.4f, &v, (m3dPLANE *)&plane, 0.7f, 2.0f, &col);
        } else {
            wrsfxCreateBloodAir(at, 0, 0.4f, vel, (m3dPLANE *)&plane, 0.7f, 2.0f, &col);
        }
        return;
    }
    case 1:
    case 2: {
        m3dV s;                                         // to the side: d x up
        s.x = d.y * m3dVUnitY.z - d.z * m3dVUnitY.y;
        s.y = d.z * m3dVUnitY.x - d.x * m3dVUnitY.z;
        s.z = d.x * m3dVUnitY.y - d.y * m3dVUnitY.x;
        m3dNormalize(&s);
        m3dV b;
        m3dV *w = vel ? vel : &m3dVZero;                // (1.2 reads vel with no check)
        b.x = m3dVUnitY.x * 5.0f + w->x;
        b.y = m3dVUnitY.y * 5.0f + w->y;
        b.z = m3dVUnitY.z * 5.0f + w->z;
        v.x = s.x * 5.0f + b.x;
        v.y = s.y * 5.0f + b.y;
        v.z = s.z * 5.0f + b.z;
        wrsfxCreateBloodAir(pos, &s, 0.8f, &v, (m3dPLANE *)&plane, 1.2f, 2.0f, &col);
        s.x = -s.x;
        s.y = -s.y;
        s.z = -s.z;
        v.x = s.x * 5.0f + b.x;
        v.y = s.y * 5.0f + b.y;
        v.z = s.z * 5.0f + b.z;
        wrsfxCreateBloodAir(pos, &s, 0.8f, &v, (m3dPLANE *)&plane, 1.2f, 2.0f, &col);
        return;
    }
    case 4:
        // PLACEHOLDER (test): a wrsfxBLOOD_SURF pool (0x168 bytes, SetParams(plane pos, normalized vel, 5.0,
        // colour), Register, CalcStaticData) -- pteSPOT is not rebuilt.
        return;
    case 6:
    case 7:
        for (int i = 0; i < 4; i++) {
            float k;
            if (kind) {
                m3dMakeVUnitConusRandom(&m3dVUnitY, 45.0f, &v);
                k = m3dRandRange(5.0f, 10.0f);
            } else {
                m3dMakeVUnitConusRandom(&m3dVUnitY, 90.0f, &v);
                k = m3dRandRange(2.5f, 5.0f);
            }
            v.x *= k;
            v.y *= k;
            v.z *= k;
            wrsfxCreateBloodAir(pos, 0, 0.8f, &v, (m3dPLANE *)&plane, 1.2f, 2.0f, &col);
        }
        return;
    default:
        return;
    }
}

// ---- the chunks --------------------------------------------------------------------------------------------

#pragma pack(push, 1)
struct wrsfxCHUNK_INIT {                                // the init data of "sfx_chunk" (size 0x4C)
    physINIT phys;                                      // +0x00
    int      isRegen;                                   // +0x38
    int      isSmall;                                   // +0x3C
    float    timer;                                     // +0x40
    int      domNmb;                                    // +0x44
    u32      domData;                                   // +0x48
};
struct wrsfxCHUNK_EV : msgDATA {                        // GS_MSG_MP_NOTIFY 0x40E of a chunk
    short type;                                         // +0x04 0 regenerate (a minotaur's piece), 1 landed
};
struct wrsfxCHUNK_EV_CDT : wrsfxCHUNK_EV {              // type 1, size 0x22
    m3dV pos;                                           // +0x06 where
    m3dV dir;                                           // +0x12 the way it flew (normalized)
    int  pad;                                           // +0x1E 0
};
#pragma pack(pop)

// v12 0x005282E0  (feb gs_domain.obj) gsDOMAIN_LIST::FindDomNmb: the domain's index in the list, -1 none
static int domFindNmb(gsDOMAIN_LIST *l, void *d)
{
    for (int i = 0; i < l->nDomains; i++)
        if ((void *)l->domain[i] == d)
            return i;
    return -1;
}

// v12 0x0046B1B0  (feb 0x00461360)
// Throws the pieces of a body blown apart, fanned out away from 'from'. type: 0 small (3), 1/2 a minotaur's
// (2 meat + 2 or 3 pieces that regrow, at most 7 minotaurs), 3 big (4), 4 many little ones (10) any way.
void __fastcall wrsfxStartChunkMeat(int type, int kind, m3dV *pos, m3dV *from, aiNPC_WR *npc)
{
    wrsfxCHUNK_INIT init;
    memset(&init, 0, sizeof(init));
    init.phys.tag = 0x40;
    init.phys.size = 0x4C;
    init.phys.radius = 0.1f;
    init.phys.lifetime = 2000000.0f;
    init.phys.maxHits = 1;
    init.domNmb = -1;
    animCREATE_DATA cd;
    if (gsSysMP->mode == 3)                             // a client: the server throws them
        return;
    float speedK = 1.0f;
    float yLo = -0.3f, yHi = 0.3f;
    int nMeat = 3, nRegen = 0;
    char regenName[0x40];
    regenName[0] = 0;
    float sizeK = 1.0f;
    switch (type) {
    case 0:
        speedK = 0.5f;
        nMeat = 3;
        if (kind) {
            yLo = 0.2f;
            yHi = 0.5f;
        }
        break;
    case 1:
    case 2: {
        nRegen = (type != 1) + 2;
        int n = aiNPC::GetNOpp(0, (char *)"ai_mino");
        if (n + nRegen > 7) {
            nRegen = 7 - n;
            if (nRegen < 1)
                nRegen = 1;
        }
        speedK = 0.6f;
        nMeat = 2;
        strcpy(regenName, "ai_mino_chunk_regen");
        if (npc && npc->spawnDomain)                    // the spawn domain counts the minotaurs to come
            *(int *)((char *)npc->spawnDomain + 0x1D4) += nRegen;
        break;
    }
    case 3:
        speedK = 0.75f;
        nMeat = 4;
        if (kind) {
            yLo = 0.2f;
            yHi = 0.5f;
        }
        break;
    case 4:
        speedK = 0.4f;
        nMeat = 10;
        if (kind) {
            yLo = -0.1f;
            yHi = 0.1f;
        }
        sizeK = 0.4f;
        break;
    }
    m3dV d;
    if (from) {
        d.x = pos->x - from->x;
        d.z = pos->z - from->z;
    } else {
        d = m3dVUnitX;
    }
    d.y = 0;
    m3dNormalize(&d);
    init.phys.accel[0] = m3dVUnitY.x * -10.0f;
    init.phys.accel[1] = m3dVUnitY.y * -10.0f;
    init.phys.accel[2] = m3dVUnitY.z * -10.0f;
    init.phys.maxHits = -1;
    init.phys.bounce = 0;
    init.phys.friction = 0.8f;
    init.phys.cdtMode = 2;
    init.phys.cdtCls = 1;
    m3dMATR r;
    r.MakeRotY(-60.0f);
    m3dV t = d;
    d.x = t.x * r.m[0][0] + t.y * r.m[1][0] + t.z * r.m[2][0];
    d.y = t.x * r.m[0][1] + t.y * r.m[1][1] + t.z * r.m[2][1];
    d.z = t.x * r.m[0][2] + t.y * r.m[1][2] + t.z * r.m[2][2];
    int total = nMeat + nRegen;
    float step = 120.0f / (float)(total - 1);
    int iMeat = 0, iRegen = 0;
    for (int i = 0; i < total; i++) {
        int meat;
        if (iMeat >= nMeat)
            meat = 0;
        else if (iRegen >= nRegen)
            meat = 1;
        else
            meat = m3dRandom() < 0.5f;
        if (meat)
            iMeat++;
        else
            iRegen++;
        m3dV p;
        float ox = m3dRandRange(-0.1f, 0.1f);
        float oz = m3dRandRange(-0.1f, 0.1f);
        p.x = ox + pos->x;
        p.y = pos->y;
        p.z = oz + pos->z;
        p.y = m3dRandRange(yLo, yHi) + p.y;
        m3dV back = { -d.x, -d.y, -d.z };
        cd.matr.MakeLCS2WCS_VZ(&p, &back);
        m3dV v;
        if (type == 4)
            m3dMakeVUnitRandom(&v);
        else
            v = d;
        v.y = m3dRandRange(0.3f, 0.45f);
        float k = m3dRandRange(8.0f, 15.0f) * speedK;
        init.phys.vel[0] = v.x * k;
        init.phys.vel[1] = v.y * k;
        init.phys.vel[2] = v.z * k;
        const char *name;
        float scale;
        if (meat) {
            name = wrsfxNameChunkDefault;
            cd.matr.RotateY(m3dRandMax(360.0f), 1);
            scale = m3dRandMax(1.0f) + 1.0f;
            init.phys.radius = 0.2f;
            init.timer = m3dRandMax(1.5f) + 1.5f;
        } else {
            name = regenName;
            scale = 1.0f - m3dRandMax(0.3f);
            init.phys.radius = 0.9f;
            init.timer = m3dRandRange(0.6f, 1.0f);
        }
        cd.scale.x = cd.scale.y = cd.scale.z = scale * sizeK;
        init.phys.lifetime = 60.0f;
        init.isRegen = !meat;
        if (npc) {
            init.domNmb = domFindNmb(aiSpawnDomList, npc->spawnDomain);
            init.domData = (u32)npc->spawnParam;
        }
        entENTITY *e = entCreate(gsScenePtr, (char *)"sfx_chunk", (char *)name, &cd, &init, 0);
        if (!e)
            return;
        ((physPHYS *)e)->AddEntCDTExclude((entENTITY *)npc);
        if (m3dRandom() < 0.5f) {
            objOBJ *b = objFindName(e->inst->root, (char *)"bone");
            if (b)
                b->SetStateYes(1);
        }
        r.MakeRotY(m3dRandRange(step * 0.5f, step * 1.5f));
        t = d;
        d.x = t.x * r.m[0][0] + t.y * r.m[1][0] + t.z * r.m[2][0];
        d.y = t.x * r.m[0][1] + t.y * r.m[1][1] + t.z * r.m[2][1];
        d.z = t.x * r.m[0][2] + t.y * r.m[1][2] + t.z * r.m[2][2];
    }
}

// ---- wrsfxCHUNK ---------------------------------------------------------------------------------------------

// v12 0x0046C840  (feb 0x004629E0)
// (the six fields zeroed: without it a piece made where an old one was freed began "regenerated" and "landed" --
//  it lay there till its 60 s were up and grew no minotaur)
entENTITY *__fastcall wrsfxCHUNK::Create(animINST *)
{
    wrsfxCHUNK *c = new wrsfxCHUNK;
    if (!c)
        return 0;
    c->isRegen = 0;
    c->isSmall = 0;
    c->timer = 0;
    c->fadeTimer = 0;
    c->regenerated = 0;
    c->landed = 0;
    return c;
}

// v12 0x0046BC30  (feb 0x00461DC0)
BOOL32 wrsfxCHUNK::ProcessINIT(physINIT *p)
{
    if (!physPHYS::ProcessINIT(p))
        return 0;
    wrsfxCHUNK_INIT *ci = (wrsfxCHUNK_INIT *)p;
    isRegen = ci->isRegen;
    timer = ci->timer;
    isSmall = ci->isSmall;
    domNmb = ci->domNmb;
    domData = ci->domData;
    inst->flags |= 0x10;
    if (!isRegen)
        inst->SetAnimSeq((int)m3dRandMax(1.999f), 0);  // one of the two tumbles
    math->flags |= PHYSM_SPHERE;
    if (isRegen) {
        aiMINO::ChunkCreateNotify(this);
        return 1;
    }
    if (wrAppState & 2)                                 // no blood: no gibs to see
        inst->flags |= 1;
    return 1;
}

// v12 0x0046BF80  (feb 0x00462100)
void wrsfxCHUNK::ProcessTERM()
{
    physPHYS::ProcessTERM();
}

static void chunkSendEv(wrsfxCHUNK *c, short type)
{
    wrsfxCHUNK_EV ev;
    ev.packId = 0;
    ev.tag = 0x40;
    ev.size = 6;
    ev.type = type;
    gsSysServer->SendMsgClientAll(c, 0x40E, &ev, 0x10000, 0xFFFFFFFD, 0xFFFFFFFD);
}

// v12 0x0046BD00  (feb 0x00461E70)
// Flies; once landed it lies there for a while; a minotaur's piece then regrows (event 0) and goes.
void wrsfxCHUNK::ProcessFRAME()
{
    if (entFlags & 4) {
        physPHYS::ProcessFRAME();
        const char *n = inst->name ? inst->name : "";
        float lift = strcmp(n, wrsfxNameChunkDefault) == 0 ? 0.1f : 0.07f;
        inst->Translate(0.0f, lift - math->init.radius, 0.0f, 1);
    }
    if (landed)
        timer -= gsElapsedTime;
    if ((entFlags & 4) && landed) {
        if ((timer < 0.0f || (math->flags & 0x10)) && isRegen && !regenerated)
            chunkSendEv(this, 0);
        if (m3dUpdateTimeField(&fadeTimer, gsElapsedTime)) {
            if (isRegen && !regenerated)
                chunkSendEv(this, 0);
            msgSystem.PostMsg(this, 2, 0);
        }
    }
    if (timer < 0.0f && !isRegen)
        inst->AdvanceTime(gsElapsedTime, 1, 0);
}

// v12 0x0046BED0  (feb 0x00462050)
// It hit something: every client hears and sees it land (event 1).
BOOL32 wrsfxCHUNK::ProcessCDT(cdtINFO *ci)
{
    if (!(entFlags & 4))
        return 0;
    if (math->flags & PHYSM_HIT)
        return 0;
    wrsfxCHUNK_EV_CDT ev;
    ev.packId = 0;
    ev.tag = 0x40;
    ev.size = 0x22;
    ev.type = 1;
    ev.pad = 0;
    ev.pos = *(m3dV *)((char *)ci + 0x10);
    ev.dir.x = math->vel[0];
    ev.dir.y = math->vel[1];
    ev.dir.z = math->vel[2];
    m3dNormalize(&ev.dir);
    gsSysServer->SendMsgClientAll(this, 0x40E, &ev, 0x10000, 0xFFFFFFFD, 0xFFFFFFFD);
    return 0;
}

// v12 0x0046BF90  (feb 0x00462110)
void wrsfxCHUNK::ProcessMP_NOTIFY(msgDATA *m)
{
    wrsfxCHUNK_EV *ev = (wrsfxCHUNK_EV *)m;
    if (ev->type == 1) {
        EV_CDTAction((wrsfxCHUNK_EV_CDT *)ev);
    } else if (ev->type == 0) {
        regenerated = 1;
        if (entFlags & 4) {
            aiNPC *n = aiMINO::Regenerate(this);         // the new minotaur takes the piece's spawn domain
            if (n && domNmb >= 0) {
                n->spawnParam = (int)domData;
                n->spawnDomain = (aiDOMAIN_SPAWN *)aiSpawnDomList->domain[domNmb];
            }
            msgSystem.PostMsg(this, 2, 0);
        }
    }
    physPHYS::ProcessMP_NOTIFY(m);
}

// v12 0x0046C020  (feb 0x004621A0)
// Landed: the slap, and a pool of blood (not over another one).
void wrsfxCHUNK::EV_CDTAction(wrsfxCHUNK_EV_CDT *ev)
{
    gsSysSound->Play(&chunkHitSnd, &ev->pos);
    float r = isSmall ? 2.5f : 5.0f;
    landed = 1;
    fadeTimer = r + 1.0f;
    if (wrAppState & 2)
        return;
    // PLACEHOLDER (test): the pool -- unless a 'GALB'/'FSLB' shader is within 4 (meat only), a wrsfxBLOOD_SURF
    // at ev->pos facing ev->dir, radius r (x 0.6 small), in the chunk's colour -- pteSPOT is not rebuilt.
}
