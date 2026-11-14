// ai_skel_spear_pjl.cpp -- the skeleton's thrown spear ("pjl_spear_skel") (Will Rock 1.2, v12.exe; feb obj
// ai_skel_spear_pjl.obj)
//
// pjlSPEAR: the satyr's arrow over again (pjlARROW_GENERIC: the same quiver once stuck, the same "born in a wall"
// check) with its own look: a trail along its body ("trail_emitter", 400 a second), its length along its flight
// (Y), "sfx_skel_spear_land" where it sticks, a player within 1 m of it hit. 10 damage, a 0.3 sphere, 7 s; thrown
// at 27 m/s under gravity 10 at where the planner says the enemy will be (within the skeleton's throw angle).

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../gs/gs_mp_server.h"
#include "../../include/sfx_sob/sfx_api.h"
#include "../../include/game_shared/projectile.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list);      // v12 0x004B1E30
void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int skip);  // v12 0x005130D0
float __fastcall entNearestPlayerDist(entENTITY *self, float yOff, m3dV *out, entENTITY **who);  // v12 0x00547450

static txmTEXTURE *spearTexTrail;                       // v12 0x0064D608 pjlSPEAR::pTexTrail "sfx_skel_spear_appear_part"

// v12 0x0064CF80.. the first three of the skeletons' 16 sounds (static init v12 0x00458060): the spear's own
static sncSOUND_DESCR_3D spearSnd[3];
static struct pjlSPEAR_SND_INIT {
    pjlSPEAR_SND_INIT()
    {
        static const struct { const char *name; int flags; float mn, mx; } s[3] = {
            { "skel_spear_fly", 0x10002, 4.0f * 1.0f, 20.0f * 1.0f },
            { "skel_spear_hit_ground", 0x20000, 4.0f * 1.3f, 20.0f * 1.5f },
            { "skel_spear_hit_body", 0x20000, 4.0f * 1.3f, 20.0f * 1.5f },
        };
        for (int i = 0; i < 3; i++) {
            sncSOUND_DESCR_3D *d = &spearSnd[i];
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
} pjlSpearSndInit;

static m3dSPL_LINEAR1D *spearSpl(int n)
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

// v12 0x0045B6B0
// Along the spear: 400 a second, 0.2 s each, 1.5 x s shrinking to nothing, fading from 127.5, turning, at s plus
// 0.1 of its speed.
class pjlSPEAR_TRAIL : public partEMITTER_PHYS {
public:
    pjlSPEAR_TRAIL(float s) : partEMITTER_PHYS(400.0f, 0.2f)
    {
        SetIntensity(400.0f, 0);
        SetTimeLifePart(0.2f, 0.0f);
        m3dSPL_LINEAR1D *sz = spearSpl(2);
        float a = s * 1.5f;
        sz->SetKp(0, 0.0f, a);
        sz->SetKp(1, 1.0f, s * 0.0f);
        SetSize(a, 0.0f, sz, 1.0f);
        m3dSPL_LINEAR1D *al = spearSpl(2);
        al->SetKp(0, 0.0f, 127.5f);
        al->SetKp(1, 1.0f, 0.0f);
        SetColor(0xFFFFFFFF, 0, al);
        SetTexture(spearTexTrail);
        SetTextureRotAnim(0.0f, 180.0f, 0.0f, 90.0f);
        flags |= 4;
        speedK = 0.1f;
        speedKRand = 0;
        speedA = s;
        speedARand = 0;
        speedASpl = 0;
        flags |= 0x200;
        flags |= 0x409;
        flags |= 0xA000;
        SetDegNearParams(3.0f, 0.3f);
    }
};

struct pjlSPEAR_SFX {                                   // 8 bytes, at pjlPJL +0x16F
    partEMITTER_PHYS *trail;
    objOBJ           *trailObj;                         // "trail_emitter"
    pjlSPEAR_SFX() : trail(0), trailObj(0) {}
    // (v12 inside pjlPJL<pjlSPEAR_SFX>::ProcessINIT, feb 0x004527F0)
    int StartSFX(physPHYS *p, pjlPJL_INIT *)
    {
        animINST *a = p->inst;
        if (!(trail = new pjlSPEAR_TRAIL(0.07f)))
            return 0;
        if (!(trailObj = objFindName(a->root, (char *)"trail_emitter")))
            return 0;
        if (trail)
            partSetAreaEmissionObj(trail, trailObj, -1, 0);
        trail->StartEmission();
        return 1;
    }
    // v12 0x0045BC60: the trail kept along the spear
    void UpdateSFX(physPHYS *)
    {
        if (trail)
            partSetAreaEmissionObj(trail, trailObj, -1, 0);
    }
    void StopSFX(physPHYS *)
    {
        if (trail) {
            trail->StopEmission();
            trail = 0;
        }
    }
};

#pragma pack(push, 1)
class pjlSPEAR : public pjlNPC_PJL<pjlSPEAR_SFX> {      // v12 size 0x208 (feb 0x1EC), vtable v12 0x005FF780
public:
    objOBJ  *rotCenter;                                 // +0x1BC "rot_center"
    int      layerState;                                // +0x1C0 2: quivering (stuck)
    float    vibTimer;                                  // +0x1C4 0.6 s of quivering
    m3dMATR  savedLocal;                                // +0x1C8 rotCenter's local as it stuck
    // size v12 0x208

    pjlSPEAR() : rotCenter(0), layerState(0), vibTimer(0) { savedLocal.Identity(); }
    static entENTITY *__fastcall Create(animINST *i);   // v12 0x0045A9D0 (feb 0x00451F50)
    BOOL32 ProcessINIT(physINIT *p);                    // v12 0x0045AAC0 (feb 0x00452040)
    void   ProcessFRAME();                              // v12 0x0045B020 (feb 0x00452590)
    void   AlignWithSpeed();                            // v12 0x0045AF10 (feb 0x00452460)
    void   Vibrate();                                   // v12 0x00457650 (shared with pjlARROW_GENERIC)
    void   GenerateCDTUponHittingPlayer();              // v12 0x0045AE30 (feb 0x00452380)
    void   ShowCollisSFX(pjlEV_CDT *ev);                // v12 0x0045B170 (feb 0x00452710)
    sncSOUND_DESCR_3D *GetSound(int i);                 // v12 0x0045B000 (feb 0x00452550)
    int    GetLayerAppState() { return layerState; }    // v12 0x0045BC80
    void   SetLayerAppState(int s) { layerState = s; }
};
#pragma pack(pop)

entENTITY *__fastcall pjlSPEAR::Create(animINST *)
{
    return new pjlSPEAR;
}

// v12 0x0045A980  (feb 0x00451F00)
int __fastcall aiSkelSpearPjlInit()
{
    if (!entRegisterClass((char *)"pjl_spear_skel", 0x53504541, 0x32, pjlSPEAR::Create, 0, 1))
        return 0;
    if (!(spearTexTrail = txmManager->Add((char *)"sfx_skel_spear_appear_part", 0x40003, 1)))
        return 0;
    return sncLinkSndListData(3, spearSnd) != 0;        // (in the game: part of the skeletons' list of 16)
}

// v12 0x0045AAC0  (feb 0x00452040)
// 1000 s, up to 10 hits, no bounce; then as the arrow: the flight's sound, born in a wall -> hit at once, the part
// that turns, 8 s stuck.
BOOL32 pjlSPEAR::ProcessINIT(physINIT *p)
{
    p->lifetime = 1000.0f;
    p->friction = 0;
    p->bounce = 0;
    p->maxHits = 10;
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
    pjlFlags |= 0x12;
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
            head.y = ((aiNPC *)owner)->Slot90() + op.y;
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
    termDelay = 8.0f;
    return 1;
}

// v12 0x0045AE30  (feb 0x00452380)
// The nearest player hit where the spear is (a hit counted before and after).
void pjlSPEAR::GenerateCDTUponHittingPlayer()
{
    m3dV at;
    entENTITY *who = 0;
    entNearestPlayerDist(this, 1.5f, &at, &who);
    cdtINFO ci;
    memset(&ci, 0, sizeof(ci));
    ci.flags |= 2;
    math->nHits++;
    inst->GetPos((m3dV *)ci.point);
    ci.inst = who ? who->inst : 0;
    msgSystem.SendMsg(this, 0x3F9, &ci, this);
    math->nHits++;
}

// v12 0x0045B020  (feb 0x00452590)
// Flying: physics (unless a cut-scene holds the action), the trail, the sounds, the layer state to the clients.
// Always: aligned, quivering. On the server: a player within 1 m of it is hit.
void pjlSPEAR::ProcessFRAME()
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
        if (entNearestPlayerDist(this, 1.5f, &at, 0) < 1.0f) {
            GenerateCDTUponHittingPlayer();
            if (math)
                math->vel[0] = math->vel[1] = math->vel[2] = 0;
            inst->root->SetStateProcYes(8, 0);
        }
    }
}

// v12 0x0045AF10  (feb 0x00452460)
// In flight the turning part lies along the speed (its Y axis; keeping its scale).
void pjlSPEAR::AlignWithSpeed()
{
    if (pjlFlags & PJLF_HIT)
        return;
    m3dV v = *(m3dV *)math->vel;
    if (v.x * v.x + v.y * v.y + v.z * v.z < 1e-10f)     // (test guard: stopped by the player hit, the game builds a
        return;                                         //  matrix from a zero vector -- NaN, which crashes the test renderer)
    m3dV p;
    ((m3dMATR *)math->matr)->GetOrigin(&p);
    inst->Validate(4);
    float sx, sy, sz;
    ((m3dMATR *)rotCenter->ltm)->GetScale(&sx, &sy, &sz);
    m3dMATR m, inv;
    m.Identity();
    m._MakeLCS2WCS_VY(&p, &v);
    m.Scale(sx, sy, sz, 1);
    ((m3dMATR *)rotCenter->parent->ltm)->Invert(&inv);
    m3dMultiplyMatr(&m, &inv, (m3dMATR *)rotCenter->local);
    rotCenter->DeclareProcAnim();
    inst->Invalidate(4);
}

// v12 0x00457650 (pjlARROW_GENERIC::Vibrate, shared)
void pjlSPEAR::Vibrate()
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

// v12 0x0045B170  (feb 0x00452710)
// Into the level: "sfx_skel_spear_land" (0.3) where it struck, started, and it quivers 0.6 s. Else gone at once.
void pjlSPEAR::ShowCollisSFX(pjlEV_CDT *ev)
{
    if (ev->kind != 1) {
        termDelay = 0;
        Terminate();
        return;
    }
    animCREATE_DATA cd;
    cd.matr._MakeLCS2WCS_VY(&ev->pos, &ev->normal);
    float size = 0.3f;
    entENTITY *e = entCreate(gsScenePtr, (char *)"sfx_skel_spear_land", (char *)"sfx_skel_spear_land", &cd, &size, 0);
    if (e) {
        ((wrsfxSKELETON_SPEAR_LAND *)e)->Start();
        layerState |= 2;
        vibTimer = 0.6f;
        memcpy(&savedLocal, rotCenter->local, sizeof(savedLocal));
    }
}

// v12 0x0045B000  (feb 0x00452550): 0 flying, 1 into the level, 2 into a body
sncSOUND_DESCR_3D *pjlSPEAR::GetSound(int i)
{
    if (i < 0 || i > 2)
        return 0;
    return &spearSnd[i];
}

// ---------------------------------------------------------------------------------------------- the throw

// v12 0x0045B2C0  (feb 0x00452890)
// At 27 m/s flat towards the planner's meeting point (1.5 up) when that is within its throw angle, else ahead (the
// enemy's own position for the height); up so as to come down there under gravity 10.
void aiSKEL_SPEAR::AimSpearToPlayer(m3dV *org, pjlPJL_INIT *in)
{
    m3dV d, v, m;
    inst->GetDir(&d);
    d.y = 0;
    if (!(wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(this, 27.0f, &m)) {
        m.y += 1.5f;
        v.x = m.x - org->x;
        v.y = 0;
        v.z = m.z - org->z;
        m3dNormalize(&v);
        float a = fabsf(m3xzAngleVector(&v, &d));
        if (!(a <= throwAngle))
            v = d;
    } else {
        enemy.GetPos(&m);
        v = d;
    }
    v.x *= 27.0f;
    v.y *= 27.0f;
    v.z *= 27.0f;
    float dx = org->x - m.x, dz = org->z - m.z;
    float dist = m3dSqrt(dx * dx + dz * dz);
    float t = dist * 0.037037037f;                      // (dist / 27)
    in->phys.vel[0] = v.x;
    in->phys.vel[1] = v.y;
    in->phys.vel[2] = v.z;
    in->phys.vel[1] = ((m.y - org->y) - t * t * -5.0f) / t;
    in->phys.accel[0] = m3dVUnitY.x * -10.0f;
    in->phys.accel[1] = m3dVUnitY.y * -10.0f;
    in->phys.accel[2] = m3dVUnitY.z * -10.0f;
}

// v12 0x0045B520  (feb 0x00452AD0)
// Attacking: the spear in its hand ("SPEAR_&lod") thrown: 10 damage (all round 1.5), a 0.3 sphere, 7 s, one hit.
void aiSKEL_SPEAR::LaunchOneProjectile()
{
    struct { pjlPJL_INIT i; u32 extra; } blk;           // (0x55 bytes sent)
    pjlPJL_INIT &in = blk.i;
    memset(&blk, 0, sizeof(blk));
    in.phys.packId = 0;
    in.phys.tag = 0x40;
    in.phys.size = 0x55;
    in.phys.radius = 0.1f;
    in.phys.lifetime = 2000000.0f;
    in.phys.maxHits = 1;
    in.weaponType = -1;
    in.ownerId = (u32)-1;
    in.termDelay = -1.0f;
    in.explosive = 1;
    animCREATE_DATA cd;
    objOBJ *sp = objFindName(inst->root, (char *)"SPEAR_&lod");
    if (!sp)
        return;
    inst->Validate(4);
    m3dV org;
    sp->GetOrigin(&org);
    memcpy(&cd.matr, sp->ltm, sizeof(cd.matr));
    if (!(aiFlags & 0x1000))
        return;
    in.damage = 10.0f;
    in.radius = 1.5f;
    in.isGeneral = 4;
    in.phys.radius = 0.3f;
    in.phys.lifetime = 7.0f;
    in.phys.maxHits = 1;
    in.phys.friction = 0;
    in.phys.cdtMode = 2;
    in.ownerId = (u32)mpId;
    AimSpearToPlayer(&org, &in);
    entENTITY *e = entCreate(gsScenePtr, (char *)"pjl_spear_skel", (char *)"pjl_spear_skel", &cd, &blk, 0);
    if (e)
        gsEffApplyLMChar(e, &floor, 0);
}
