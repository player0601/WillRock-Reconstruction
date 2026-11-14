// ai_satyr.cpp -- the satyr archer, its arrow and the arrow's landing dust (Will Rock 1.2, v12.exe; feb objs
// ai_satyr.obj, pjl_arrow.obj, sfx_satyr.obj)
//
// "ai_satyr": shoots volleys of five arrows. A sniper (one placed in the level: not spawned) stands and shoots
// whenever it may; a runner (spawned) shoots from afar but runs off when the player comes within 20, about its
// spawn domain's area if it has one. Hit, it flinches (WOUNDED); dead, it falls (DEATH1 / DEATH2, chosen at birth).
//
// satyrFlags (+0x567): 0x10 hit (WOUNDED till it plays), 0x1000000 appearing (spawned, till APPEAR's action frame).
// unk108 sound bits: 1 appear, 2 death 1, 4 death 2, 8 hit, 0x10 the shot, 0x20 running, 0x40 fell to the ground.
//
// "pjl_satyr_arrow" (pjlARROW_GENERIC : pjlNPC_PJL<pjlARROW_GENERIC_SFX>): flies in a ballistic arc with a trail,
// aligns with its speed; stuck in the level it quivers (a layer state told to the clients) and the dust rises
// ("sfx_satir_arrow_land"); hits a player within 0.85 by itself (the player's hit box is not in its collision).

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"
#include "../../include/game_shared/projectile.h"

// v12 0x0064CB60.. the sounds, 0x68 apart (static init v12 0x00455F90..0x0045621F)
static sncSOUND_DESCR_3D satyrSnd[10];
enum { SSND_HIT, SSND_DEATH1, SSND_DEATH2, SSND_RUN, SSND_SHOT, SSND_EXPLOS, SSND_FALL, SSND_ARROW_FLY,
       SSND_ARROW_HIT_GROUND, SSND_ARROW_HIT_BODY };

static struct aiSATYR_SND_INIT {
    aiSATYR_SND_INIT()
    {
        static const struct { const char *name; int flags; float mn, mx; } s[10] = {
            { "satyr_being_hit", 0x40000, 25.5f, 70.0f },
            { "satyr_death_1", 0x40000, 30.0f, 80.0f },
            { "satyr_death_2", 0x40000, 30.0f, 80.0f },
            { "satyr_run", 0, 8.0f, 25.0f },
            { "satyr_shot_arrow", 0, 15.0f, 50.0f },
            { "char_explos", 0, 25.5f, 70.0f },
            { "satyr_fall_ground", 0x10000, 8.0f, 25.0f },
            { "satyr_arrow_fly", 0x10002, 4.0f, 20.0f },
            { "satyr_arrow_hit_ground", 0x20000, 5.2f, 30.0f },
            { "satyr_arrow_hit_body", 0x20000, 5.2f, 30.0f },
        };
        for (int i = 0; i < 10; i++) {
            sncSOUND_DESCR_3D *d = &satyrSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].mn;
            d->maxDist = s[i].mx;
            d->vtbl = 0;
        }
    }
} aiSatyrSndInit;

// v12 0x00457140
static sncSOUND_DESCR_3D *__fastcall satyrGetSound(int i) { return &satyrSnd[i]; }

static m3dSPL_LINEAR1D *satSpl(int n)
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

// ================================================================================================================
// the arrow
// ================================================================================================================
static txmTEXTURE *pjlArrowTexTrail;                    // v12 0x0064CF70 pjlARROW_GENERIC::pTexTrail "sfx_arrow_trail"

class pjlARROW_TRAIL : public partEMITTER_PHYS {
public:
    pjlARROW_TRAIL(float size);
};

// v12 0x00457D80
// A thin streak: lives 0.3 s, grows to 4x size at a fifth of its life, faint (alpha 50), carried a little by the
// arrow's speed.
pjlARROW_TRAIL::pjlARROW_TRAIL(float size) : partEMITTER_PHYS(200.0f, 0.3f)
{
    SetIntensity(200.0f, 0);
    SetTimeLifePart(0.3f, 0.0f);
    m3dSPL_LINEAR1D *sz = satSpl(3);
    float s4 = size * 4.0f;
    sz->SetKp(0, 0.0f, size * 0.0f);
    sz->SetKp(1, 0.2f, s4);
    sz->SetKp(2, 1.0f, size * 0.0f);
    SetSize(s4, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = satSpl(3);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.2f, 50.49f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(pjlArrowTexTrail);
    SetTextureRotAnim(90.0f, 0.0f, 0.0f, 0.0f);
    SetTextureScale(1.0f, 0.0f, 0.125f, 0.0f);
    flags |= 4;
    flags |= 0x100000;
    cone = 180.0f;
    coneSpl = 0;
    speedK = 0.05f;
    speedKRand = 0;
    speedA = 0;
    speedARand = 0;
    speedASpl = 0;
    flags |= 0x409;
    flags |= 0x40;
    flags |= 0x200;
}

partEMITTER_PHYS *__fastcall pjlArrowNewTrail(float s)
{
    return new pjlARROW_TRAIL(s);
}

#include "../../include/ai/pjl_arrow.h"

// v12 0x00457F70  (feb 0x0044F6E0)
entENTITY *__fastcall pjlARROW_GENERIC::Create(animINST *)
{
    return new pjlARROW_GENERIC;
}

// v12 0x004571B0
int __fastcall pjlSatyrArrowInit()
{
    if (!entRegisterClass((char *)"pjl_satyr_arrow", 0x41525257, 0x32, pjlARROW_GENERIC::Create, 0, 1))
        return 0;
    return (pjlArrowTexTrail = txmManager->Add((char *)"sfx_arrow_trail", 0x40003, 1)) != 0;
}

// v12 0x00457200
// The flight's sound channel; born inside a wall (the line from the shooter's head or chest to it blocked): hit at
// once; the part that turns.
BOOL32 pjlARROW_GENERIC::ProcessINIT(physINIT *p)
{
    if (!physPHYS::ProcessINIT(p))
        return 0;
    pjlPJL_INIT *in = (pjlPJL_INIT *)p;
    damage = in->damage;
    radius = in->radius;
    isGeneral = in->isGeneral;
    weaponType = in->weaponType;
    termDelay = in->termDelay;
    explosive = in->explosive;
    if (entFlags & 4) {
        if (in->ownerId != 0xFFFFFFFF) {
            entIACTIVE_OBJ *e = gsSysMP->FindMPID((int)in->ownerId, 4);
            if (e)
                AddEntCDTExclude((entENTITY *)e);
        }
        ownerId = in->ownerId;
    }
    sfx.StartSFX(this, in);
    if (HasSound()) {
        float mn, mx;
        GetSndDist(&mn, &mx);
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        if (c) {
            c->minDist = mn;
            c->maxDist = mx;
        }
        sndChannel = c;
        if (!sndChannel)
            return 0;
        gsSysSound->AddChannel(sndChannel, this, 0);
    }
    pjlFlags |= 0x12;                                   // flying; 0x10 "launch" (no sound 3 for an arrow)
    if ((entFlags & 4) && in->ownerId != 0xFFFFFFFF) {
        entENTITY *owner = (entENTITY *)gsSysMP->FindMPID((int)in->ownerId, 4);
        if (owner && owner->unk98 != 0x14) {
            cdtREFINE ref(2, owner->inst);
            cdtINFO ci;
            memset(&ci, 0, sizeof(ci));
            m3dV op, ap;
            owner->inst->GetPos(&op);
            inst->GetPos(&ap);
            m3dV chest = op;
            chest.y = op.y + 0.3f;
            m3dV head = op;
            head.y = ((aiNPC *)owner)->Slot90() + op.y;         // (owner vtable +0x168: heightUp1)
            if (!gsScenePtr->IsVisiblePoint(&head, &ap, 1, &ref, &ci) ||
                !gsScenePtr->IsVisiblePoint(&chest, &ap, 1, &ref, &ci)) {
                pjlHIT_MSG m;
                m.packId = 0;
                m.tag = 0x40;
                m.size = 0x24;
                entENTITY *he = ci.inst ? ci.inst->ent : 0;
                m.mpId = he ? (u32)he->mpId : 0xFFFFFFFF;
                m.ci0 = ci.flags;
                m.ci8 = (u32)ci.objId;
                m.ciC = (u32)ci.faceId;
                m.point = *(m3dV *)ci.point;
                memcpy(&m.ci1C, &ci.dist, 4);
                msgSystem.PostMsgData(this, 0x4D3, &m, this);
            }
        }
    }
    rotCenter = objFindName(inst->root, (char *)"rot_center");
    if (!rotCenter)
        return 0;
    layerState = 0;
    vibTimer = 0;
    savedLocal.Identity();
    termDelay = 8.0f;                                   // stuck arrows stay 8 s
    return 1;
}

// v12 0x00457730
// Flying: physics (unless a cut-scene holds the action), the trail, the sounds, the layer state to the clients.
// Always: aligned, quivering. On the server: a player within 0.85 of it is hit.
void pjlARROW_GENERIC::ProcessFRAME()
{
    if (!(pjlFlags & PJLF_HIT)) {
        if (!((gsSysCinema->state & 1) && gsSysCinema->IsSuspendAction())) {
            physPHYS::ProcessFRAME();
            sfx.UpdateSFX(this);
            PlaySounds();
        }
        if (GetLayerAppState() != lastLayerState) {
            pjlEV_LAYER ev;
            ev.packId = 0;
            ev.tag = 0x40;
            ev.size = 0xA;
            ev.type = 7;
            ev.state = GetLayerAppState();
            gsSysServer->SendMsgClientAll(this, 0x40E, &ev, 0x10000, 0xFFFFFFFD, 0xFFFFFFFD);
            lastLayerState = ev.state;
        }
    }
    AlignWithSpeed();
    Vibrate();
    if ((entFlags & 4) && !(pjlFlags & PJLF_HIT)) {
        m3dV at;
        if (entNearestPlayerDist(this, 1.0f, &at, 0) < 0.85f) {
            entENTITY *who = 0;
            entNearestPlayerDist(this, 1.0f, &at, &who);
            cdtINFO ci;
            memset(&ci, 0, sizeof(ci));
            ci.flags |= 2;
            ci.inst = who ? who->inst : 0;
            math->nHits++;
            inst->GetPos((m3dV *)ci.point);
            msgSystem.SendMsg(this, 0x3F9, &ci, this);
            if (math)
                math->vel[0] = math->vel[1] = math->vel[2] = 0;
            termDelay = 0;
            Terminate();
        }
    }
}

// v12 0x00457550
// In flight the turning part points along the speed (keeping its scale).
void pjlARROW_GENERIC::AlignWithSpeed()
{
    if (pjlFlags & PJLF_HIT)
        return;
    m3dV v = *(m3dV *)math->vel;
    m3dV p;
    ((m3dMATR *)math->matr)->GetOrigin(&p);             // (physMATH::GetPos)
    inst->Validate(4);
    float sx, sy, sz;
    ((m3dMATR *)rotCenter->ltm)->GetScale(&sx, &sy, &sz);
    m3dMATR m, inv;
    m.Identity();
    m._MakeLCS2WCS_VZ(&p, &v);
    m.Scale(sx, sy, sz, 1);
    ((m3dMATR *)rotCenter->parent->ltm)->Invert(&inv);
    m3dMultiplyMatr(&m, &inv, (m3dMATR *)rotCenter->local);
    rotCenter->DeclareProcAnim();
    inst->Invalidate(4);
    inst->Validate(4);
}

// v12 0x00457650
// Stuck: back to the pose it stuck in; while quivering, a random tilt that dies away with the timer.
void pjlARROW_GENERIC::Vibrate()
{
    if (!(pjlFlags & PJLF_HIT))
        return;
    memcpy(rotCenter->local, &savedLocal, sizeof(savedLocal));
    inst->Invalidate(4);
    if (!(layerState & 2))
        return;
    if (m3dUpdateTimeField(&vibTimer, gsElapsedTime)) {
        layerState &= ~2;
        return;
    }
    float a = vibTimer * 3.3333333f;
    float rx = m3dRandMax(a + a) - a;
    float rz = m3dRandMax(a + a) - a;
    ((m3dMATR *)rotCenter->local)->RotateX(rx, 1);
    ((m3dMATR *)rotCenter->local)->RotateZ(rz, 1);
    rotCenter->DeclareProcAnim();
}

// v12 0x00457A70
// Into the level: the dust where it struck, and it quivers 0.6 s. Into anything else: gone at once.
void pjlARROW_GENERIC::ShowCollisSFX(pjlEV_CDT *ev)
{
    if (ev->kind != 1) {
        termDelay = 0;
        Terminate();
        return;
    }
    animCREATE_DATA cd;
    cd.matr._MakeLCS2WCS_VY(&ev->pos, &ev->normal);
    float size = 0.3f;
    if (entCreate(gsScenePtr, (char *)"sfx_satir_arrow_land", (char *)"sfx_satir_arrow_land", &cd, &size, 0)) {
        layerState |= 2;
        vibTimer = 0.6f;
        memcpy(&savedLocal, rotCenter->local, sizeof(savedLocal));
    }
}

// v12 0x00457B30
// 0 flying, 1 into the level, 2 into a body (the satyr's sounds 7..9); no sound for the throw.
sncSOUND_DESCR_3D *pjlARROW_GENERIC::GetSound(int i)
{
    if (i < 0 || i > 2)
        return 0;
    return satyrGetSound(i + SSND_ARROW_FLY);
}

// ================================================================================================================
// the dust where an arrow struck the level
// ================================================================================================================
txmTEXTURE *wrsfxSATIR_ARROW_LAND::pTexCloud;
txmTEXTURE *wrsfxSATIR_ARROW_LAND::pTexPart;

// v12 0x004812A0
int __fastcall wrsfxSatirArrowLandInit()
{
    if (!(wrsfxSATIR_ARROW_LAND::pTexCloud = txmManager->Add((char *)"sfx_mcg_cloud", 0x40003, 1)))
        return 0;
    if (!(wrsfxSATIR_ARROW_LAND::pTexPart = txmManager->Add((char *)"sfx_mcg_part", 0x40003, 1)))
        return 0;
    return entRegisterClass((char *)"sfx_satir_arrow_land", 0x24534658, 0x50, wrsfxSATIR_ARROW_LAND::Create, 0, 1) != 0;
}

// v12 0x00481310
// A puff: a burst in the first sixth of a second, 1.5..2 s of slow smoke.
wrsfxSATIR_ARROW_LAND_CLOUD::wrsfxSATIR_ARROW_LAND_CLOUD(float size) : partEMITTER_PHYS(0x32, 300.0f, 3.0f)
{
    m3dSPL_LINEAR1D *in = satSpl(4);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 200.0f);
    in->SetKp(2, 0.13333334f, 200.0f);
    in->SetKp(3, 0.16666667f, 0.0f);
    SetIntensity(0.0f, in);
    SetTimeLifePart(1.5f, 0.5f);
    float half = size * 0.5f;
    SetSize(half, size * 0.25f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = satSpl(2);
    a->SetKp(0, 0.0f, 27.285f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0x3FFFFFFF, 0, a);
    SetTexture(wrsfxSATIR_ARROW_LAND::pTexCloud);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 90.0f);
    flags |= 4;
    speedA = size;
    speedARand = half;
    speedASpl = 0;
    flags |= 8;
    flags |= 0x2000;
    flags |= 0x8000;
    SetDegNearParams(3.0f, 0.3f);
    SetDegFarParams(10.0f, 30.0f, 2.0f, 0.1f);
}

// v12 0x00481510
// Grit: thrown up and out, falling back and bouncing on the struck plane.
wrsfxSATIR_ARROW_LAND_PART::wrsfxSATIR_ARROW_LAND_PART(float size) : partEMITTER_PHYS(0x28, 100.0f, 4.0f)
{
    m3dSPL_LINEAR1D *in = satSpl(4);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 100.0f);
    in->SetKp(2, 0.13333334f, 100.0f);
    in->SetKp(3, 0.2f, 0.0f);
    SetIntensity(0.0f, in);
    SetTimeLifePart(3.0f, 1.0f);
    SetSize(size * 0.05f, size * 0.025f, 0, 1.0f);
    m3dSPL_LINEAR1D *a = satSpl(2);
    a->SetKp(0, 0.5f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0x80FFFFFF, 0, a);
    SetTexture(wrsfxSATIR_ARROW_LAND::pTexPart);
    SetTextureFrameAnim(2, 2, 0.0f, 3.0f);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 90.0f);
    flags |= 4;
    flags |= 2;
    speedASpl = 0;
    speedA = size * 5.0f;
    speedARand = size * 3.0f;
    speedBSpl = 0;
    gravity.x = 0;
    gravity.z = 0;
    speedB = size * 2.5f;
    speedBRand = size * 1.5f;
    gravity.y = size * -20.0f;
    SetCDTParam(0.0f, size * 0.3f, size * 0.5f, 3);
    flags |= 0x10000;
    flags |= 8;
    flags |= 0x2000;
    flags |= 0x8000;
    SetDegFarParams(10.0f, 30.0f, 2.0f, 0.1f);
}

// v12 0x00481760  (feb 0x004773E0)
void wrsfxSATIR_ARROW_LAND::ProcessINIT(float *data)
{
    float size = data ? *data : 0.1f;
    inst->Scale(size, size, size, 2);
    inst->SetAnimSeq(0, 0);
    inst->flags |= 0x10;
    if (!(cloud = new wrsfxSATIR_ARROW_LAND_CLOUD(size)))
        return;
    if (!(part = new wrsfxSATIR_ARROW_LAND_PART(size)))
        return;
    if (!(area = objFindName(inst->root, (char *)"&h_&nc_area")))
        return;
    if (!partSetAreaEmissionObj(cloud, area, -1, 0))
        return;
    if (!partSetAreaEmissionObj(part, area, -1, 0))
        return;
    struct { m3dV pos, n; } plane;
    ((m3dMATR *)inst->matr)->GetOrigin(&plane.pos);
    ((m3dMATR *)inst->matr)->GetAxisY(&plane.n);
    part->SetCDTPlane(1, 0, (m3dPLANE *)&plane);
    cloud->StartEmission();
    part->StartEmission();
    timer = 4.1f;
}

// v12 0x004818D0  (feb 0x00477550)
BOOL32 wrsfxSATIR_ARROW_LAND::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id == 1) {
        ProcessINIT((float *)data);
    } else if (id == 2) {
        cloud->StopEmission();
        part->StopEmission();
        if (cloud)
            cloud->__vecDelDtor(1);
        cloud = 0;
        if (part)
            part->__vecDelDtor(1);
        part = 0;
    } else if (id == 1000) {
        if (m3dUpdateTimeField(&timer, gsElapsedTime)) {
            msgSystem.PostMsg(this, 2, this);
            return 0;
        }
        inst->AdvanceTime(gsElapsedTime, 1, 0);
    }
    return 0;
}

// v12 0x004819C0  (feb 0x00477640)
entENTITY *__fastcall wrsfxSATIR_ARROW_LAND::Create(animINST *)
{
    wrsfxSATIR_ARROW_LAND *e = new wrsfxSATIR_ARROW_LAND;
    if (!e)
        return 0;
    e->cloud = 0;
    e->part = 0;
    e->area = 0;
    e->timer = 0;
    return e;
}

// ================================================================================================================
// the satyr
// ================================================================================================================

// v12 0x00456220  (feb 0x0044D920)
int __fastcall aiSatyrInit()
{
    if (!entRegisterClass((char *)"ai_satyr", 0x244E5043, 0x32, aiSATYR::Create, (char *)"ai_satyr", 0))
        return 0;
    if (!pjlSatyrArrowInit())
        return 0;
    if (!sncLinkSndListData(10, satyrSnd))
        return 0;
    return wrsfxSatirArrowLandInit() != 0;
}

// v12 0x00457150  (feb 0x0044E800)
entENTITY *__fastcall aiSATYR::Create(animINST *)
{
    aiSATYR *s = new aiSATYR;
    if (!s)
        return 0;
    s->wrFlags = 0;
    s->bloodObj = 0;
    s->satyrFlags = 0;
    s->voice = 0;
    s->shootTimer = 0;
    s->moveTimer = 0;
    s->arrowsLeft = 0;
    s->deathSeq = 4;
    s->area = 0;
    s->unk98 = 9;
    return s;
}

// v12 0x00456280  (feb 0x0044D970)
BOOL32 aiSATYR::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 7;
    u32 *blk = (u32 *)operator new(0x3BC);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 7;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 7; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    static const char *names[7] = { "SATYR_SEQ_IDLE", "SATYR_SEQ_SHOOT_ARROW", "SATYR_SEQ_WOUNDED", "SATYR_SEQ_RUN",
                                    "SATYR_SEQ_DEATH1", "SATYR_SEQ_DEATH2", "SATYR_SEQ_APPEAR" };
    for (int i = 0; i < 7; i++)
        _entAnimSetSeqName(t, i, (char *)names[i]);
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(7, 0);
    if (!s)
        return 0;
    t->aniSystem = s;
    s->SetTranTypeToAny(0, 0, 1, -1);
    s->SetTranCycleAnim(0);
    s->SetTranCycleAnim(3);
    s->SetTranParamFromAny(2, 0, 1, 2);
    s->SetTranCycleAnim(2);
    s->SetTranParamFromAny(4, 0, 1, 4);
    s->SetTranParamFromAny(5, 0, 1, 5);
    s->SetTranParam(3, 1, 0, 1, 1);
    s->SetTranParam(4, 4, 2, 4, 1);
    s->SetTranParam(5, 5, 2, 5, 1);
    s->SetTranParam(4, 5, 2, 4, 1);
    s->SetTranParam(5, 4, 2, 5, 1);
    if (old)
        operator delete((u32 *)old - 1);
    t->seq[4].unk84 |= 1;                               // (the deaths are moved by their root)
    t->seq[5].unk84 |= 1;
    return 1;
}

// v12 0x004564A0
void aiSATYR::SetMovementSpeed()
{
    moveSpeed = 15.0f;
    if (inst->seq == 3) {
        turnSpeed = 360.0f;
    } else {
        turnSpeed = 180.0f;
        aiFlags |= 0x200;                               // stands
    }
    if (arrowsLeft > 0)
        aiFlags |= 0x200;
}

// v12 0x00456500
int aiSATYR::ProcessINIT(void *init)
{
    if (!(flags2 & 1))
        aiFlags |= 0x40000;                             // placed in the level: a sniper
    if (!aiNPC_WR::ProcessINIT(init))
        return 0;
    aiFlags2 |= 0x8002;
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    radCDT = 0.7f;
    alertAngle = 180.0f;
    aimAngle = 180.0f;
    heightCDT = 1.4f;
    heightLowTester = 0.84f;
    fallbackAngle = 50.0f;
    maxTurnStep = 70.0f;
    shootRange = 120.0f;
    heightUp1 = 2.3f;
    shootTimer = 0;
    moveTimer = 0;
    maxHealth = health = 7.0f;
    if (!(bloodObj = objFindName(inst->root, (char *)"CENTRE")))
        return 0;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
    if (c) {
        c->minDist = 20.0f;
        c->maxDist = 150.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    int noSfx = 0;
    GetInt((char *)"AI_PROP", (char *)"isNoSFXAppear", &noSfx);   // (read, unused in 1.2)
    if (!(rootObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    if (flags2 & 1)
        satyrFlags |= 0x1000000;                        // spawned: appears
    deathSeq = m3dRandSelect(4, 5);
    return 1;
}

// v12 0x004567A0
// A sniper never moves (aiFlags 0x20); a spawned one runs about its domain's area, if it has one.
void aiSATYR::ParseNameClass()
{
    aiNPC_WR::ParseNameClass();
    area = 0;
    if (aiFlags & 0x40000) {
        aiFlags |= 0x20;
        unk37F = 40.0f;
        arrowsLeft = 5;
        return;
    }
    if (spawnDomain)
        area = WR_FIELD(aiAREA *, spawnDomain, 0xDC);
    arrowsLeft = 5;
}

// v12 0x00456850
void aiSATYR::Think()
{
    aiNPC_WR::Think();
    if (inst->seq == 2)
        satyrFlags &= ~0x10u;                           // the flinch is playing
}

// v12 0x00456880
int aiSATYR::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    satyrFlags |= 0x10;
    aiNPC_WR::ProcessCDT_FIREABLE(d);
    return 1;
}

// v12 0x004568A0
void aiSATYR::StartDeath(dmgDAMAGE *d)
{
    aiNPC_WR::StartDeath(d);
}

// v12 0x004568B0
void aiSATYR::Explode(dmgDAMAGE *d, int dead)
{
    m3dV *from = 0;
    if (d && (d->type == 1 || d->type == 3 || d->type == 4))
        from = &((dmgWEAPON *)d)->hitPos;
    m3dV c;
    bloodObj->GetOrigin(&c);
    StartChunkMeat(3, dead, &c, from);
    StartBlood(7, dead, &c, 0, 0, from);
    gsSysSound->Play(&satyrSnd[SSND_EXPLOS], &c);
    msgSystem.PostMsg(this, 2, this);
}

// the volley may start: it sees the enemy, nothing holds it, the last volley is over
static int satyrMayShoot(aiSATYR *s)
{
    return (s->aiFlags & 0x10000000) && !(s->aiFlags & 0x80000008) && !(s->wrFlags & 0x200) &&
           s->shootTimer <= 1e-6f;
}

// v12 0x00456940
int aiSATYR::SelectGoal()
{
    if (aiFlags & 0x40000) {                            // a sniper: shoots when it may, else as any NPC
        if (satyrMayShoot(this)) {
            aiFlags |= 0x1000;
            arrowsLeft = 1;
        } else {
            aiFlags &= ~0x1000u;
        }
        return aiNPC::SelectGoal();
    }
    m3dV e, pr;
    enemy.GetPos(&e);
    GetPosRooted(&pr);
    if (moveTimer < 1e-6f || (aiFlags & 0x400000)) {
        int shoot = 0;
        if (satyrMayShoot(this)) {
            if (aiFlags & 0x40000) {
                shoot = 1;
            } else {
                m3dV p, q;
                inst->GetPos(&p);
                enemy.GetPos(&q);
                shoot = !(m3dLength(p - q) < 20.0f);    // too near: away
            }
        }
        if (shoot) {
            if (!arrowsLeft)
                arrowsLeft = 5;
            aiFlags |= 0x1000;
            inst->GetPos(&goalPos);
        } else {
            arrowsLeft = 0;
            aiFlags &= ~0x1000u;
            moveTimer = 3.0f;
            if (area) {
                area->GetRandomPos(&goalPos, &e);
                goalPos.y = pr.y;
            } else {
                aiNPC::SelectGoal();
            }
        }
    }
    if (!(aiFlags & 0x1000) && (aiFlags & 1) && moveTimer < 1e-6f) {
        if (!enemy.IsDead() && (aiFlags & 0x10000000)) {
            if (!arrowsLeft)
                arrowsLeft = 5;
            aiFlags |= 0x1000;
            inst->GetPos(&goalPos);
        } else {
            aiFlags &= ~0x1000u;
            arrowsLeft = 0;
            aiNPC::SelectGoal();
            moveTimer = 3.0f;
        }
    }
    wayPointPos = inst->seq == 1 ? e : goalPos;         // shooting: faces the enemy
    return 1;
}

// v12 0x00456CB0
int aiSATYR::EvalSniperGoalSeq()
{
    return (aiFlags >> 12) & 1;                         // SHOOT_ARROW or IDLE
}

// v12 0x00456CC0
int aiSATYR::EvalRunningGoalSeq()
{
    if (aiFlags & 1)
        return (aiFlags >> 28) & 1;
    return (aiFlags & 0x1000) ? 1 : 3;                  // SHOOT_ARROW or RUN
}

// v12 0x00456CE0
int aiSATYR::EvalAnimGoalSeq()
{
    if (aiFlags & 0x200000)
        return deathSeq;
    if (satyrFlags & 0x10)
        return 2;
    if (enemy.IsDead())
        return 0;
    if (satyrFlags & 0x1000000)
        return 6;
    if (aiFlags & 0x40000)
        return EvalSniperGoalSeq();
    return EvalRunningGoalSeq();
}

// v12 0x00456D50
void aiSATYR::ProcessActionFrame()
{
    if (inst->IsActionFrame(1, 0))
        LaunchOneProjectile();
    if (inst->IsActionFrame(6, 0))
        satyrFlags &= ~0x1000000u;
}

// v12 0x00456D90
// One arrow from the bow's arrow ("arrow_&lod01_4"): 12 damage, a 0.3 sphere, 7 s at most, two hits.
void aiSATYR::LaunchOneProjectile()
{
    pjlPJL_INIT in;
    memset(&in, 0, sizeof(in));
    in.phys.packId = 0;
    in.phys.tag = 0x40;
    in.phys.size = 0x55;
    in.phys.radius = 0.3f;
    in.phys.lifetime = 7.0f;
    in.phys.maxHits = 2;
    in.phys.bounce = 0;
    in.phys.friction = 0;
    in.phys.cdtMode = 0xA0;
    in.phys.cdtCls = 0;
    in.damage = 12.0f;
    in.radius = 1.5f;
    in.weaponType = -1;
    in.isGeneral = 4;
    in.ownerId = (u32)mpId;
    in.termDelay = -1.0f;
    in.explosive = 0;
    struct { pjlPJL_INIT i; u32 extra; } blk;           // (1.2 sends 0x55 bytes: a zero dword after the init)
    animCREATE_DATA cd;
    inst->Validate(4);
    objOBJ *arrow = objFindName(inst->root, (char *)"arrow_&lod01_4");
    if (!arrow)
        return;
    m3dV pos;
    arrow->GetOrigin(&pos);
    if (enemyDist < 2.0f) {                             // point blank: from its middle, not through the enemy
        m3dV pr;
        GetPosRooted(&pr);
        pos.x = pr.x;
        pos.z = pr.z;
    }
    GetShootArrowPhysParam(&pos, &in);
    memcpy(&cd.matr, arrow->ltm, sizeof(cd.matr));
    cd.matr.Translate(&pos, 0);
    blk.i = in;
    blk.extra = 0;
    if (!entCreate(gsScenePtr, (char *)"pjl_satyr_arrow", (char *)"pjl_satyr_arrow", &cd, &blk, 0))
        return;
    if (--arrowsLeft <= 0)
        shootTimer = 0.1f;
}

// v12 0x00457B50
// Aimed at the enemy's meeting point 40 ahead (1 above it), or at the enemy itself when that fails or when told so
// (wrFlags 0x10); more than 10 degrees off its facing, straight ahead. 40 m/s across, gravity 10, the upward speed
// that lands it there.
void aiSATYR::GetShootArrowPhysParam(m3dV *from, pjlPJL_INIT *init)
{
    m3dV dir, v, at;
    inst->GetDir(&dir);
    int haveMeet = 0;
    if (!(wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(this, 40.0f, &at)) {
        at.y += 1.0f;
        v.x = at.x - from->x;
        v.y = 0;
        v.z = at.z - from->z;
        m3dNormalize(&v);
        haveMeet = 1;
    }
    if (!haveMeet) {
        enemy.GetPos(&at);
        v = dir;
    }
    dir.y = 0;
    float ang = fabsf(m3dAngleVector(&v, &dir));
    if (ang > 10.0f)
        v = dir;
    v.x *= 40.0f;
    v.y *= 40.0f;
    v.z *= 40.0f;
    float dx = from->x - at.x, dz = from->z - at.z;
    float t = sqrtf(dx * dx + dz * dz) * 0.025f;        // the flight's time
    float dy = at.y - from->y;
    init->phys.vel[0] = v.x;
    init->phys.vel[1] = v.y;
    init->phys.vel[2] = v.z;
    init->phys.vel[1] = (dy - t * t * -5.0f) / t;
    init->phys.accel[0] = m3dVUnitY.x * -10.0f;
    init->phys.accel[1] = m3dVUnitY.y * -10.0f;
    init->phys.accel[2] = m3dVUnitY.z * -10.0f;
}

// v12 0x00456F70
void aiSATYR::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&shootTimer, gsElapsedTime);
    m3dUpdateTimeField(&moveTimer, gsElapsedTime);
}

// v12 0x00456FA0
void aiSATYR::FillSoundFlags()
{
    if (inst->IsActionFrame(1, 1))
        unk108 |= 0x10;                                 // the shot
    if ((aiFlags & 0x8000) && passivePhys->nHits == 1)
        unk108 |= 0x40;                                 // fell to the ground
    if (inst->flags & 0x4000) {
        switch (inst->seq) {
        case 2: unk108 |= 8; break;
        case 3: unk108 |= 0x20; break;
        case 4: unk108 |= 2; break;
        case 5: unk108 |= 4; break;
        case 6: unk108 |= 1; break;
        }
    }
}

// v12 0x00457080
void aiSATYR::PlaySounds()
{
    if (unk108 & 2)
        voice->Play(&satyrSnd[SSND_DEATH1], 0);
    u32 f = unk108;
    if (f & 4)
        voice->Play(&satyrSnd[SSND_DEATH2], 0);
    else if (f & 8)
        voice->Play(&satyrSnd[SSND_HIT], 0);
    else if (f & 0x10)
        voice->Play(&satyrSnd[SSND_SHOT], 0);
    else if (!(f & 1)) {
        if (f & 0x20)
            voice->Play(&satyrSnd[SSND_RUN], 0);
        else if (f & 0x40)
            voice->Play(&satyrSnd[SSND_FALL], 0);
    }
    aiNPC::PlaySounds();
}

// v12 0x004571A0
int aiSATYR::IsAnimSeqGoodForFallback()
{
    return deathSeq == 5;
}
