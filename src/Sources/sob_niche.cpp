// sob_niche.cpp -- breakable walls / floors / roofs ("sob_niche") and their dust ("sfx_niche_explode")
// (Will Rock 1.2, v12.exe; feb objs sob_niche.obj, sfx_niche.obj)
//
// A niche breaks when a script activates or unlocks it, when damage reaches it (unless f_16F, Zeus's roofs), or
// when the server says so (GS_MSG_MP_NOTIFY type 0; type -100 a loaded game's state). Then "wall_crash" (or its
// own NICHE SoundBreak), the dust, and the wall cut into nPiece stones that fly ("nch_rigid").
//
// PLACEHOLDER (test): the cutting and the stones -- nchSHATTER_RIGID_START::StartExplos v12 0x00488D10 with
// shtSHATTER::Start v12 0x0051FC60, DetermineEdgeShards v12 0x00488F20, CreatePiece v12 0x00488930 and the rigid
// bodies nchRIGID / rgdRIGID (phys_rigid*.obj) are not rebuilt: the wall just goes (hidden, out of the collision)
// behind the dust; nchBreakStart below keeps the rest of StartExplos (the planes the dust bounces on, the dust).

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"
#include "../../include/sfx_sob/sob_entities.h"

static txmTEXTURE *nchTexStones;                        // v12 0x0064F3D4 "stones_oblom"
static sncSOUND_DESCR_3D nchSndCrash;                   // v12 0x0064F3E8 "wall_crash" 15..80 (static init v12 0x00488060)
static struct nchSND_INIT {
    nchSND_INIT()
    {
        nchSndCrash.name.SetName((char *)"wall_crash");
        nchSndCrash.flags = 0;
        nchSndCrash.volume = 0;
        nchSndCrash.delay = 0;
        nchSndCrash.count = 0;
        nchSndCrash.time = 0;
        nchSndCrash.data = 0;
        nchSndCrash.dataFlags = 0;
        nchSndCrash.minDist = 15.0f;
        nchSndCrash.maxDist = 80.0f;
        nchSndCrash.vtbl = 0;
    }
} nchSndInit;

// v12 0x004880C0  (feb 0x0047DB30)
// (PLACEHOLDER (test): "nch_rigid" (nchRIGID::Create v12 0x00488160) is not registered here: the test's stand-in is)
int __fastcall wrsobInitNiche()
{
    if (!entRegisterClass((char *)"sob_niche", 0x2444594E, 0x28, wrsobNICHE::Create, 0, 1))
        return 0;
    if (!(nchTexStones = txmManager->Add((char *)"stones_oblom", 0x40000, 1)))
        return 0;
    if (!wrsfxNicheInit())
        return 0;
    if (!sncLinkSndListData(1, &nchSndCrash)) {
        apMsg((char *)"Cannot link sound data");
        return 0;
    }
    return 1;
}

// v12 0x00489440  (feb 0x0047EDB0)
entENTITY *__fastcall wrsobNICHE::Create(animINST *)
{
    wrsobNICHE *n = new wrsobNICHE;
    if (!n)
        return 0;
    n->exploded = 0;
    n->f_16F = 0;
    n->sndBreak = 0;
    n->tex = 0;
    n->nPiece = 70;
    return n;
}

// v12 0x004881E0  (feb 0x0047DC50)
BOOL32 wrsobNICHE::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    switch (id) {
    case 1:
        ProcessINIT();
        break;
    case 2:                                             // (1.2) its own break sound let go
        if (sndBreak) {
            ((sncSOUND_DESCR *)sndBreak)->UnlinkData();
            operator delete(sndBreak);
            sndBreak = 0;
        }
        break;
    case 0x3F8: {                                       // IACTION
        apNAME *c = (apNAME *)((char *)data + 8);
        if (c->IsName((char *)"ACTIVATE") || c->IsName((char *)"UNLOCK"))
            StartExplos(0);
        break;
    }
    case 0x40E: {                                       // MP_NOTIFY
        int t = *(short *)((char *)data + 4);
        if (t == -100) {
            msgDATA *st = *(msgDATA **)((char *)data + 6);
            if (WR_FIELD(int, st, 4))
                StartExplos(1);
        } else if (t == 0) {
            StartExplos(0);
        }
        break;
    }
    case 0x4C6:                                         // damage
        if ((entFlags & 4) && !f_16F)
            StartExplos(0);
        break;
    }
    return 0;
}

// v12 0x004882F0  (feb 0x0047DD10)
// Synchronised; its NICHE keys: how near the edge a piece stays put (a flat one 0.05, else 1.2), the pieces, the
// broken faces' texture, (1.2) its own break sound.
void wrsobNICHE::ProcessINIT()
{
    gsSysServer->DeclSyncable(this);
    unkBC |= 4;
    m3dV d;
    inst->GetDir(&d);
    distFromEdge = fabsf(d.y) < 0.2f ? 1.2f : 0.05f;
    inst->GetFloat((char *)"NICHE", (char *)"distFromEdge", &distFromEdge);
    inst->GetInt((char *)"NICHE", (char *)"nPiece", &nPiece);
    char s[0x40];
    if (inst->GetStr((char *)"NICHE", (char *)"tex", s, 0x40))
        tex = txmManager->Add(s, 0x100, 1);
    if (!inst->GetStr((char *)"NICHE", (char *)"SoundBreak", s, 0x40))
        return;
    float mn, mx;
    if (!inst->GetFloat((char *)"NICHE", (char *)"SoundBreakRadMin", &mn))
        mn = 15.0f;
    if (!inst->GetFloat((char *)"NICHE", (char *)"SoundBreakRadMax", &mx))
        mx = 80.0f;
    sncSOUND_DESCR_3D *sd = (sncSOUND_DESCR_3D *)operator new(sizeof(sncSOUND_DESCR_3D));
    if (sd) {
        memset(sd, 0, sizeof(*sd));
        sd->name.SetName(s);
        sd->minDist = mn;
        sd->maxDist = mx;
    }
    sndBreak = sd;
    if (!((sncSOUND_DESCR *)sndBreak)->LinkData(0, 0)) {
        operator delete(sndBreak);
        sndBreak = 0;
    }
}

#pragma pack(push, 1)
struct nchSYNC : msgDATA {                              // the synchronised state (8 bytes)
    int exploded;                                       // +0x04
};
#pragma pack(pop)
static nchSYNC nchSyncData;                             // v12 0x0064F3D8

// v12 0x00488500  (feb 0x0047DE20)
void wrsobNICHE::ProcessMP_GET_SYNC_DATA(void *d)
{
    nchSYNC **pp = (nchSYNC **)d;
    if (!*pp) {
        nchSyncData.packId = 0;
        nchSyncData.tag = 0x40;
        nchSyncData.size = 8;
        nchSyncData.exploded = 0;
        *pp = &nchSyncData;
    }
    (*pp)->exploded = exploded;
}

// v12 0x00488570  (feb 0x0047DF10)
// A saved game's state: chunk 0xF0 holds "exploded".
void wrsobNICHE::ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    nchSYNC *st = (nchSYNC *)d->data;
    if (!st) {
        nchSyncData.packId = 0;
        nchSyncData.tag = 0x40;
        nchSyncData.size = 8;
        nchSyncData.exploded = 0;
        st = &nchSyncData;
        d->data = st;
    }
    fioCHUNK c;
    memset(&c, 0, sizeof(c));
    while (d->file->ReadChunk(&c) && c.id != 1)
        if (c.id == 0xF0)
            d->file->Read(&st->exploded, 4, 4);
}

// the planes the dust bounces on (rgdPLANE_SET, 5 at most: m3dPLANE 0x18 each, the count at +0x78)
struct nchPLANES {
    struct { m3dV p, n; } plane[5];
    int n;
    void AddPlane(m3dV *p, m3dV *nrm)                   // rgdPLANE_SET::AddPlane v12 0x00528980
    {
        if (n >= 5)
            return;
        plane[n].p = *p;
        plane[n].n = *nrm;
        n++;
    }
};

// v12 0x00489170 nchSHATTER_RIGID_START::DetermineCollisPlaneSet (feb 0x0047EAE0)
// What is ahead of the niche within 10, and ahead and below: the faces its pieces fall on.
static void nchDetermineCollisPlaneSet(nchPLANES *ps, entENTITY *e)
{
    cdtREFINE ref(2, 0);
    cdtINFO ci;
    memset(&ci, 0, sizeof(ci));
    m3dV p, d, nrm;
    e->inst->GetPos(&p);
    e->inst->GetDir(&d);
    if (gsScenePtr->ShootRay(&p, &d, 10.0f, 0, &ref, &ci)) {
        m3dV at = *(m3dV *)ci.point;
        gsScenePtr->GetCDTFace(&ci, 0, &nrm);
        ps->AddPlane(&at, &nrm);
    }
    e->inst->GetDir(&d);
    d.x -= m3dVUnitY.x;
    d.y -= m3dVUnitY.y;
    d.z -= m3dVUnitY.z;
    m3dNormalize(&d);
    if (gsScenePtr->ShootRay(&p, &d, 10.0f, 0, &ref, &ci)) {
        m3dV at = *(m3dV *)ci.point;
        gsScenePtr->GetCDTFace(&ci, 0, &nrm);
        ps->AddPlane(&at, &nrm);
    }
}

// v12 0x00488D10 nchSHATTER_RIGID_START::StartExplos, in part (see the PLACEHOLDER above)
// The dust (unless the scene is full: 700 instances), then the wall gone.
static void nchBreakStart(wrsobNICHE *n)
{
    if (!n->inst)
        return;
    objOBJ *o = objFind(n->inst->root, objIsWithFaces, 0);
    if (!o)
        return;
    n->inst->Validate(0x206);
    static nchPLANES ps;                                // (in the 0x5DF8-byte shatter object on the stack in v12)
    memset(&ps, 0, sizeof(ps));
    nchDetermineCollisPlaneSet(&ps, n);
    // PLACEHOLDER (test): shtSHATTER::Start (the cutting) and DetermineEdgeShards -- no stones
    int nShard = 0;                                     // (the shatter's +0x5DDC: the pieces left on the edges)
    if (gsScenePtr->GetNInst() < 700 && !nShard) {
        animCREATE_DATA cd;
        n->inst->GetMatrL2W(&cd.matr);
        struct { float size; int nPlane; void *planes; } d = { 0.2f, ps.n, ps.plane };
        entCreate(gsScenePtr, (char *)"sfx_niche_explode", (char *)"sfx_niche_explode", &cd, &d, 0);
    }
    // PLACEHOLDER (test): CreatePiece for each piece
    n->inst->flags |= 0x11;                             // the wall gone: hidden, out of the collision
}

// v12 0x00488660  (feb 0x0047E000)
// Once (the server only while unlocked): the wall breaks, its sound (not when a saved state replays it), the
// clients told.
void wrsobNICHE::StartExplos(int silent)
{
    if (((entFlags & 4) && nLocks > 0) || exploded)
        return;
    pendingEvents |= 0x20;
    exploded = 1;
    nchBreakStart(this);
    if (!silent) {
        m3dV p;
        inst->GetPos(&p);
        gsSysSound->Play(sndBreak ? sndBreak : &nchSndCrash, &p);
    }
    if (entFlags & 4) {
#pragma pack(push, 1)
        struct : msgDATA { short type; } ev;
#pragma pack(pop)
        ev.packId = 0;
        ev.tag = 0x40;
        ev.size = 6;
        ev.type = 0;
        gsSysServer->SendMsgClientAll(this, 0x40E, &ev, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
    }
}

// ================================================================================================================
// sfx_niche_explode
// ================================================================================================================
txmTEXTURE *wrsfxNICHE_EXPLODE::pTexDust;
txmTEXTURE *wrsfxNICHE_EXPLODE::pTexGrit;

static m3dSPL_LINEAR1D *nchSpl(int n)
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

// v12 0x0047CA30
int __fastcall wrsfxNicheInit()
{
    if (!(wrsfxNICHE_EXPLODE::pTexDust = txmManager->Add((char *)"sfx_discus_land_cloud", 0x40003, 1)))
        return 0;
    if (!(wrsfxNICHE_EXPLODE::pTexGrit = txmManager->Add((char *)"sfx_mcg_part", 0x40003, 1)))
        return 0;
    return entRegisterClass((char *)"sfx_niche_explode", 0x24534658, 0x50, wrsfxNICHE_EXPLODE::Create, 0, 1) != 0;
}

// grit thrown out: a burst in the first 0.7 s, 5 +- 3 s, falling, bouncing on the planes
class wrsfxNICHE_GRIT1 : public partEMITTER_PHYS {
public:
    wrsfxNICHE_GRIT1(float size);
};
// grit dropping off, 2 s
class wrsfxNICHE_GRIT2 : public partEMITTER_PHYS {
public:
    wrsfxNICHE_GRIT2(float size);
};

static void nchGritCommon(partEMITTER_PHYS *e, float size)
{
    e->SetTimeLifePart(5.0f, 3.0f);
    e->SetSize(size * 0.325f, size * 0.175f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = nchSpl(2);
    a->SetKp(0, 0.52f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    e->SetColor(0xFFFFFFFF, 0, a);
    e->SetTexture(wrsfxNICHE_EXPLODE::pTexGrit);
    e->SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
    e->SetTextureFrameAnim(2, 2, 0.0f, 3.0f);
    e->flags |= 6;
}

// v12 0x0047CAA0
wrsfxNICHE_GRIT1::wrsfxNICHE_GRIT1(float size) : partEMITTER_PHYS(0xAF, 500.0f, 8.0f)
{
    m3dSPL_LINEAR1D *in = nchSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 500.0f);
    in->SetKp(2, 0.7f, 0.0f);
    SetIntensity(0.0f, in);
    nchGritCommon(this, size);
    speedA = speedARand = size * 10.0f;
    speedASpl = 0;
    speedBSpl = 0;
    gravity.x = 0;
    gravity.z = 0;
    speedB = size + size;
    speedBRand = size * 0.5f;
    gravity.y = size * -19.6f;
    SetCDTParam(size * 0.15f, 0.2f, 0.2f, 3);
    flags |= 0x10000;
    flags |= 8;
    flags |= 0xA000;
}

// v12 0x0047CCA0
wrsfxNICHE_GRIT2::wrsfxNICHE_GRIT2(float size) : partEMITTER_PHYS(30.0f, 8.0f)
{
    m3dSPL_LINEAR1D *in = nchSpl(3);
    in->SetKp(0, 0.033333335f, 0.0f);
    in->SetKp(1, 0.06666667f, 30.0f);
    in->SetKp(2, 2.0333333f, 0.0f);
    SetIntensity(0.0f, in);
    nchGritCommon(this, size);
    gravity.y = size * -9.8f;
    speedA = size;
    speedARand = 0;
    speedASpl = 0;
    gravity.x = 0;
    gravity.z = 0;
    SetCDTParam(size * 0.15f, 0.2f, 0.2f, 3);
    flags |= 0x10000;
    flags |= 8;
    flags |= 0xA000;
}

// dust: thrown out fast at first, slowing (its speed follows the emitter's time)
class wrsfxNICHE_DUST : public partEMITTER_PHYS {
public:
    m3dSPL_LINEAR1D *speedSpl;                          // v12 +0x2C4
    m3dSPL_LINEAR1D *randSpl;                           // v12 +0x2C8
    wrsfxNICHE_DUST(float size);
    ~wrsfxNICHE_DUST();
    void PartCreate(int n, partPART **list, float dt, int total, int start);
};

// v12 0x0047CF40
wrsfxNICHE_DUST::wrsfxNICHE_DUST(float size) : partEMITTER_PHYS(0x6E, 300.0f, 6.0f)
{
    m3dSPL_LINEAR1D *in = nchSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 300.0f);
    in->SetKp(2, 0.7f, 0.0f);
    SetIntensity(0.0f, in);
    SetTimeLifePart(4.5f, 1.5f);
    SetSize(size * 4.25f, size * 1.25f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = nchSpl(3);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.275f, 54.57f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wrsfxNICHE_EXPLODE::pTexDust);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 90.0f);
    flags |= 4;
    speedSpl = nchSpl(2);
    if (speedSpl) {
        speedSpl->SetKp(0, 0.033333335f, size + size);
        speedSpl->SetKp(1, 0.7f, size * 0.5f);
    }
    randSpl = nchSpl(2);
    if (randSpl) {
        randSpl->SetKp(0, 0.033333335f, size * 4.0f);
        randSpl->SetKp(1, 0.7f, size);
    }
    gravity.x = 0;
    gravity.z = 0;
    gravity.y = size * -1.0f;
    SetDegNearParams(3.0f, 0.1f);
    flags |= 8;
    flags |= 0xA000;
}

// v12 0x0047D470
wrsfxNICHE_DUST::~wrsfxNICHE_DUST()
{
    delete speedSpl;
    delete randSpl;
}

// v12 0x0047CE90
// The birth speed by the emitter's time: fast at first.
void wrsfxNICHE_DUST::PartCreate(int n, partPART **list, float dt, int total, int start)
{
    float tt;
    memcpy(&tt, &total, 4);                             // (v12 reads this argument as a float: fsub [esp+0x20])
    float t = time - tt, s = 0, r = 0;
    speedSpl->GetValue(t, &s, 0);
    randSpl->GetValue(t, &r, 0);
    speedA = s;
    speedARand = r;
    speedB = s * 0.2f;
    speedASpl = 0;
    speedBRand = r * 0.2f;
    speedBSpl = 0;
    partEMITTER_PHYS::PartCreate(n, list, dt, total, start);
}

// v12 0x0047D220  (feb 0x00472F90)
// The model scaled to size; two grits and the dust from its three areas, the grits bouncing on the given planes.
int wrsfxNICHE_EXPLODE::ProcessINIT(void *data)
{
    struct D { float size; int nPlane; char *planes; } *d = (D *)data;
    inst->Scale(d->size, d->size, d->size, 0);
    inst->flags |= 0x10;
    if (!(part[0].emitter = new wrsfxNICHE_GRIT1(d->size)))
        return 0;
    if (!(part[0].area = objFindName(inst->root, (char *)"&h_&nc_areaGrit1")))
        return 0;
    if (!(part[1].emitter = new wrsfxNICHE_GRIT2(d->size)))
        return 0;
    if (!(part[1].area = objFindName(inst->root, (char *)"&h_&nc_areaGrit2")))
        return 0;
    if (!(part[2].emitter = new wrsfxNICHE_DUST(d->size)))
        return 0;
    if (!(part[2].area = objFindName(inst->root, (char *)"&h_&nc_areaDust")))
        return 0;
    for (int i = 0; i < d->nPlane; i++) {
        part[0].emitter->SetCDTPlane(d->nPlane, i, (m3dPLANE *)(d->planes + i * 0x18));
        part[1].emitter->SetCDTPlane(d->nPlane, i, (m3dPLANE *)(d->planes + i * 0x18));
    }
    for (int i = 0; i < 3; i++) {
        if (!partSetAreaEmissionObj(part[i].emitter, part[i].area, -1, 0))
            return 0;
        part[i].emitter->StartEmission();
    }
    timer = 7.5f;
    return 1;
}

// v12 0x0047D3D0  (feb 0x00473140)
BOOL32 wrsfxNICHE_EXPLODE::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id == 1) {
        ProcessINIT(data);
    } else if (id == 2) {
        for (int i = 0; i < 3; i++) {
            if (part[i].emitter)
                part[i].emitter->__vecDelDtor(1);
            part[i].emitter = 0;
            part[i].area = 0;
        }
    } else if (id == 1000) {
        if (m3dUpdateTimeField(&timer, gsElapsedTime))
            msgSystem.PostMsg(this, 2, 0);
        inst->AdvanceTime(gsElapsedTime, 1, 0);
    }
    return 0;
}

// v12 0x0047D4C0  (feb 0x00473230)
entENTITY *__fastcall wrsfxNICHE_EXPLODE::Create(animINST *)
{
    wrsfxNICHE_EXPLODE *e = new wrsfxNICHE_EXPLODE;
    if (!e)
        return 0;
    e->timer = 0;
    for (int i = 0; i < 3; i++) {
        e->part[i].emitter = 0;
        e->part[i].area = 0;
    }
    return e;
}
