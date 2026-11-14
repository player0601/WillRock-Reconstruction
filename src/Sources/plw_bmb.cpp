// plw_bmb.cpp -- pwpBOMB, the bomb (grenade, weapon 9): thrown, one at a time (Will Rock 1.2, v12.exe; feb obj
// plw_bmb.obj)
//
// Rebuilt from the 1.2 code (v12 0x004067E0..0x00408200, vtable 0x005F9BA0): the class, its INIT / sequences /
// sound slots, the throw (Shoot +0x68 v12 0x00406F10, ProcessSHOOT +0x98 v12 0x00406D20). Its FRAME slot (v12
// 0x00406D10) only jumps to pwpWEAPON's. The thrown bomb "pwp_bmb_pjl" and its explosion "pwp_bmb_pjl_expl"
// (Create v12 0x00406A30 / 0x00406B00) with their emitters: the end of this file.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../gs/gs_mp_misc.h"
#include "../gs/gs_mp_client.h"
#include "../../include/game_shared/projectile.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30
entENTITY *__fastcall pwpBMB_PJL_Create(animINST *i);     // v12 0x00406A30 "pwp_bmb_pjl" (below)
entENTITY *__fastcall pwpBMB_PJL_EXPL_Create(animINST *i); // v12 0x00406B00 "pwp_bmb_pjl_expl" (below)

sncSOUND_DESCR_3D pwpBmbSnd[4];                         // v12 0x006401A8 throw, fall, explosion, water hit
txmTEXTURE *pwpBOMB::pTexSmokeExpl;                     // v12 0x00640348 "sfx_discus_land_cloud"
txmTEXTURE *pwpBOMB::pTexFlameExpl;                     // v12 0x0064034C "pwp_bmb_pjl_expl_flame"
txmTEXTURE *pwpBOMB::pTexGritExpl;                      // v12 0x00640350 "sfx_mcg_part"
txmTEXTURE *pwpBOMB::pTexScorchMark;                    // v12 0x00640354 "sfx_bsk_scorch_mark"

// v12 0x004067E0  static initialiser (the sncSOUND_DESCR_3D constructor inline)
static struct pwpBMB_SND_INIT {
    pwpBMB_SND_INIT()
    {
        static const struct { const char *name; u32 flags; float minDist, maxDist; } s[4] = {
            { "wpn_bomb_shoot", 0, 0.0f, 0.0f },
            { "wpn_bomb_fall", 0x40000, 0.0f, 0.0f },
            { "wpn_bomb_explode", 0, 150.0f, 200.0f },
            { "wpn_machinegun_hit_water", 0, 40.0f, 140.0f },
        };
        for (int i = 0; i < 4; i++) {
            sncSOUND_DESCR_3D *d = &pwpBmbSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].minDist;
            d->maxDist = s[i].maxDist;
            d->vtbl = 0;                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} pwpBmbSndInit;

// v12 0x00406930  (feb 0x00406410)
int __fastcall pwpBMBInit()
{
    if (!entRegisterClass((char *)"pwp_bmb", 'PBMB', 0x50, pwpBOMB::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"pwp_bmb_pjl", 'PBMB', 0x50, pwpBMB_PJL_Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"pwp_bmb_pjl_expl", 0x24534658, 0x50, pwpBMB_PJL_EXPL_Create, 0, 1))
        return 0;
    if (!(pwpBOMB::pTexSmokeExpl = txmManager->Add((char *)"sfx_discus_land_cloud", 0x40003, 1)))
        return 0;
    if (!(pwpBOMB::pTexFlameExpl = txmManager->Add((char *)"pwp_bmb_pjl_expl_flame", 0x40003, 1)))
        return 0;
    if (!(pwpBOMB::pTexGritExpl = txmManager->Add((char *)"sfx_mcg_part", 0x40000, 1)))
        return 0;
    if (!(pwpBOMB::pTexScorchMark = txmManager->Add((char *)"sfx_bsk_scorch_mark", 0x40000, 1)))
        return 0;
    return sncLinkSndListData(4, pwpBmbSnd) != 0;
}

// v12 0x00406B80  (feb ?)
// pwpWEAPON's sequences, and from SHOOT straight on to RECHARGE / FORCE_RECHARGE, IDLE to TAKE_OFF, SHOOT to IDLE
// and to itself; TAKE_OFF_EMPTY's action frame 0 at frame 189.
BOOL32 pwpBOMB::SetupAniSystem()
{
    if (!pwpWEAPON::SetupAniSystem())
        return 0;
    animSYSTEM *as = *(animSYSTEM **)((char *)inst->tpl + 0xA4);
    wrVCall<void>(as, 4, 7, 8, 1, 8, 0);
    wrVCall<void>(as, 4, 7, 9, 1, 9, 0);
    wrVCall<void>(as, 4, 1, 4, 0, 4, 0);
    wrVCall<void>(as, 4, 7, 1, 1, 1, 0);
    wrVCall<void>(as, 4, 7, 7, 1, 8, 0);
    inst->tpl->seq[5].actionFrame[0] = 189;
    return 1;
}

// v12 0x00406C20  (feb ?)
// Event 1: the throw (SHOOT's action frame 2).
void pwpBOMB::Anim2Sound()
{
    if (inst->IsActionFrame(7, 2))
        unk108 |= 1;
}

// v12 0x00406C50  (feb ?)
void pwpBOMB::UpdateSound()
{
    if (unk108 & 1)
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpBmbSnd[0], 0);
    pwpWEAPON::UpdateSound();
}

// v12 0x00406C80  (feb ?)
// One in the hand (action frames: throw 0, recharge 1), 25 a second, damage 110, 20 at most, none at first; the
// model's normals for the lighting.
int pwpBOMB::ProcessINIT()
{
    if (!pwpWEAPON::ProcessINIT())
        return 0;
    inst->root->AllocVertNormalList();
    flags2 |= 2;
    shootActFrame = 0;
    reloadActFrame = 1;
    clipSize = 1;
    clip = 1;
    SetShootRate(25.0f);
    damage = 110.0f;
    maxAmmo = 20.0f;
    SetAmmo(0.0f);
    return 1;
}

#pragma pack(push, 1)
// The throw asked for (msg 0x4C7 to the server): gsEVENT type 9 (its ctor v12 0x00417000 inline, the shot packer's
// id), from where and which way.
struct pwpBMB_SHOT : msgDATA {
    u8    type;                     // +0x04 9
    m3dV  org;                      // +0x05 the hand (GetOrgDirTip)
    m3dV  dir;                      // +0x11 the camera's forward direction
};                                  // 0x1D
// The thrown bomb's start (entCreate data, physINIT and the bomb's own): 60 a second along the throw, -19.6 down.
struct pwpBMB_PJL_INIT {
    physINIT ph;                    // +0x00 vel, accel, radius 0.15, lifetime 120, unk24 -1, 0.2, 0.7 (size word 0x58)
    float damage;                   // +0x38 GetDamage
    int   ownerId;                  // +0x3C the thrower's mpId
    m3dV  pos;                      // +0x40
    m3dV  dir;                      // +0x4C
};                                  // 0x58
#pragma pack(pop)

extern msgDATA_PACK pwpPackShot;                        // v12 0x0064188C (pwp_comm.cpp) -- not rebuilt

// v12 0x00406F10
// The base weapon's checks (with the owner as the target), then from the hand along the camera's forward direction:
// the throw sent to the server; a client takes the bomb at once.
int pwpBOMB::Shoot(entENTITY *, m3dV *aim)
{
    if (!wpnWEAPON::Shoot((entENTITY *)owner, aim))
        return 0;
    m3dV org, dir;
    GetOrgDirTip(&org, &dir);
    gsCameraPtr->GetVForw(&dir);
    m3dNormalize(&dir);
    pwpBMB_SHOT m;
    m.packId = (u8)pwpPackShot.id;
    m.tag = 0x40;
    m.size = 0x1D;
    m.type = 9;
    m.org = org;
    m.dir = dir;
    gsSysClient->SendMsgServer(plrPlayer, 0x4C7, &m, 0x20000);
    if (!(entFlags & 4))
        SubtractAmmo(1.0f);
    return 1;
}

// v12 0x00406D20
// On the server: the bomb taken, the throw passed on to the clients, the bomb made -- "pwp_bmb_pjl" at the hand,
// facing the throw, 0.3 of its size, flying at 60 along it under twice the gravity, with the weapon's damage.
void pwpBOMB::ProcessSHOOT(void *msg)
{
    pwpBMB_SHOT *m = (pwpBMB_SHOT *)msg;
    pwpBMB_PJL_INIT p;
    p.ph.packId = 0;
    p.ph.tag = 0x40;
    p.ph.size = 0x58;
    p.ph.vel[0] = p.ph.vel[1] = p.ph.vel[2] = 0.0f;
    p.ph.accel[0] = p.ph.accel[1] = p.ph.accel[2] = 0.0f;
    p.ph.radius = 0.1f;
    p.ph.lifetime = 2000000.0f;
    p.ph.unk24 = 1;
    p.ph.unk28[0] = p.ph.unk28[1] = p.ph.unk28[2] = p.ph.unk28[3] = 0;
    p.damage = -1.0f;
    animCREATE_DATA cd;
    if (!(entFlags & 4))
        return;
    SubtractAmmo(1.0f);
    gsSysServer->SendMsgClientAll(owner, 0x4C7, (msgDATA *)msg, 0xA0000, 0xFFFFFFFF, 0xFFFFFFFD);
    const float speed = 60.000004f, fall = -19.6f;      // v12 0x005F9AA0 / 0x005F9A9C
    float f;
    p.ph.lifetime = 120.0f;
    f = 0.7f;
    memcpy(&p.ph.unk28[1], &f, 4);
    f = 0.2f;
    memcpy(&p.ph.unk28[0], &f, 4);
    p.ph.vel[0] = m->dir.x * speed;
    p.ph.vel[1] = m->dir.y * speed;
    p.ph.vel[2] = m->dir.z * speed;
    p.ph.unk24 = -1;
    p.ph.radius = 0.15f;
    p.ph.accel[0] = m3dVUnitY.x * fall;
    p.ph.accel[1] = m3dVUnitY.y * fall;
    p.ph.accel[2] = m3dVUnitY.z * fall;
    p.damage = GetDamage();
    p.ownerId = owner->mpId;
    p.pos = m->org;
    p.dir = m->dir;
    ((m3dMATR *)&cd.matr)->MakeLCS2WCS_VZ(&m->org, &m->dir);
    cd.scale.x = 0.3f;
    cd.scale.y = 0.3f;
    cd.scale.z = 0.3f;
    entCreate(gsScenePtr, (char *)"pwp_bmb_pjl", (char *)"pwp_bmb_pjl", &cd, &p, 0);
}

// v12 0x004081B0  (feb 0x00407C60)
entENTITY *__fastcall pwpBOMB::Create(animINST *)
{
    pwpBOMB *w = new pwpBOMB;               // operator new(0x1CB), pwpWEAPON ctor
    if (!w)
        return 0;
    w->sndChannel = 0;
    w->owner = 0;
    w->unk1C3 = 1.0f;
    w->wpnIndex = PWP_BOMB;
    return w;
}

// ================================================================================================================
// The thrown bomb (pwpBMB_PJL : physPHYS, vtable v12 0x005F99B8, size 0x1A7) and its explosion (pwpBMB_PJL_EXPL).
// It bounces (with the fall's sound) and, once at rest 2 s on, goes off when something not of its own class is
// within 3 m; a hit on a body, a damage message or its end set it off too. In water it slows and sinks (a splash,
// no harm). Going off: 6 m of the thrower's damage (weapon 9), the flame and smoke, a scorch mark.

struct wrsfxSCORCHMARK {
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
};                                                      // v12 0x00467DE0
int __fastcall wrsfxFindFloor(m3dV *pos, m3dBOX *box);  // v12 0x00467D10 (sfx_appear.cpp)

#pragma pack(push, 1)
class pwpBMB_PJL : public physPHYS {
public:
    int      inWater;          // +0x16F slowed and sinking
    float    damage;           // +0x173 the weapon's
    cdtINFO  lastHit;          // +0x177 where it last touched the level
    float    timer;            // +0x197 since thrown
    int      exploded;         // +0x19B
    int      ownerId;          // +0x19F the thrower's mpId
    sncCHANNEL *voice;         // +0x1A3
    BOOL32 ProcessINIT(physINIT *p);                    // v12 0x00406FE0 (feb 0x00406AA0)
    void   ProcessTERM();                               // v12 0x00407370 (feb 0x00406E30)
    void   ProcessFRAME();                              // v12 0x004073A0 (feb 0x00406E60)
    BOOL32 ProcessCDT(cdtINFO *ci);                     // v12 0x004071B0 (feb 0x00406C70)
    void   ProcessMP_NOTIFY(msgDATA *ev);               // v12 0x00407570 (feb 0x00407030)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00407590 (feb 0x00407050)
    void   Explode(cdtINFO *ci, int water);             // v12 0x004075F0 (feb 0x004070B0)
    void   ShowCollisSFX(pjlEV_CDT *ev);                // v12 0x00407720 (feb 0x004071D0)
};
class pwpBMB_PJL_EXPL : public entENTITY {              // size v12 0x17B
public:
    partEMITTER_PHYS *smoke;   // +0x16B
    partEMITTER_PHYS *flame;   // +0x16F
    partEMITTER_PHYS *grit;    // +0x173
    float    timer;            // +0x177 5 s
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);  // (feb 0x00407900)
};
#pragma pack(pop)

// v12 0x00406A30  (feb 0x00406510)
entENTITY *__fastcall pwpBMB_PJL_Create(animINST *)
{
    pwpBMB_PJL *b = new pwpBMB_PJL;
    if (!b)
        return 0;
    b->inWater = 0;
    b->damage = 0;
    memset(&b->lastHit, 0, sizeof(b->lastHit));
    b->timer = 0;
    b->exploded = 0;
    b->ownerId = -1;
    b->voice = 0;
    return b;
}

// v12 0x00406B00  (feb 0x004065E0)
entENTITY *__fastcall pwpBMB_PJL_EXPL_Create(animINST *)
{
    pwpBMB_PJL_EXPL *e = new pwpBMB_PJL_EXPL;
    if (!e)
        return 0;
    e->smoke = 0;
    e->flame = 0;
    e->grit = 0;
    e->timer = 0;
    return e;
}

// v12 0x00406FE0  (feb 0x00406AA0)
// Shown (no shadow), physics flags 6, its damage and thrower (not hit by it), a sphere voice (20..70).
BOOL32 pwpBMB_PJL::ProcessINIT(physINIT *p)
{
    if (!physPHYS::ProcessINIT(p))
        return 0;
    pwpBMB_PJL_INIT *in = (pwpBMB_PJL_INIT *)p;
    inst->flags &= ~0x10u;
    inst->root->SetStateYes(0x10);
    WR_FIELD(u32, this, 0xBC) |= 4;
    math->flags |= 6;
    damage = in->damage;
    ownerId = in->ownerId;
    if (entFlags & 4) {
        entIACTIVE_OBJ *e = gsSysMP->FindMPID(in->ownerId, 4);
        if (e)
            AddEntCDTExclude((entENTITY *)e);
    }
    if (WR_FIELD(int, gsSysMP, 0x28) != 4) {
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;      // (a sncCHANNEL_SPH there)
        if (c) {
            c->minDist = 20.0f;
            c->maxDist = 70.0f;
        }
        voice = c;
        if (!voice)
            return 0;
        gsSysSound->AddChannel(voice, this, 0);
    }
    return 1;
}

// v12 0x004071B0  (feb 0x00406C70)
// Into water (an entity of flags2 bit 1): slowed (its physINIT copy: bounce -0.5, friction 0.5), sinking, the
// splash. Into another entity: it goes off. Else (the level) the place kept. Moving faster than 0.1: the fall's sound.
BOOL32 pwpBMB_PJL::ProcessCDT(cdtINFO *ci)
{
    u8 *pi = (u8 *)math + 0x5C;
    if (ci->inst && ci->inst->ent && (WR_FIELD(u32, ci->inst->ent, 0xBC) & 1)) {
        *(float *)(pi + 0x2C) = 0.5f;
        *(float *)(pi + 0x28) = -0.5f;
        AddEntCDTExclude(ci->inst->ent);
        inWater = 1;
        Explode(ci, 1);
        return 0;
    }
    BOOL32 r = physPHYS::ProcessCDT(ci);
    if (ci->inst) {
        Explode(ci, 0);
        r = 1;
    } else
        memcpy(&lastHit, ci, sizeof(lastHit));
    inst->flags &= ~0x10u;
    float *v = velocity;
    float sp = m3dSqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
    if (!(fabsf(sp) < 0.1f) && voice)
        voice->Play((sncSOUND_DESCR *)&pwpBmbSnd[1], 0);
    return r;
}

// v12 0x00407370  (feb 0x00406E30): not gone off yet -- now
void pwpBMB_PJL::ProcessTERM()
{
    if (!exploded)
        Explode(&lastHit, 0);
    physPHYS::ProcessTERM();
}

// v12 0x004073A0  (feb 0x00406E60)
// In water: slowed (-8 a second). On the server, at rest 2 s on: anything not of its class within 3 m sets it off.
void pwpBMB_PJL::ProcessFRAME()
{
    if (inWater) {
        float *v = math->vel;
        float dt = gsElapsedTime;
        v[0] = v[0] * -8.0f * dt + v[0];
        v[1] = v[1] * -8.0f * dt + v[1];
        v[2] = v[2] * -8.0f * dt + v[2];
    }
    physPHYS::ProcessFRAME();
    if (!(entFlags & 4))
        return;
    timer += gsElapsedTime;
    if (exploded || !(math->flags & 8) || !(timer >= 2.0f))
        return;
    m3dV p;
    inst->GetPos(&p);
    animINST *found[10];
    int n = 0;
    dmgCollectInst(&p, 3.0f, found, &n, 10);
    for (int i = 0; i < n; i++) {
        animINST *a = found[i];
        if ((a->flags & 0x40) || a->ent == this)
            continue;
        const char *mine = inst->className ? inst->className : "";
        const char *its = a->className ? a->className : "";
        if (strcmp(its, mine)) {
            Explode(&lastHit, 0);
            msgSystem.PostMsg(this, 2, this);
            return;
        }
    }
}

// v12 0x00407570  (feb 0x00407030)
void pwpBMB_PJL::ProcessMP_NOTIFY(msgDATA *ev)
{
    if (((pjlEV_CDT *)ev)->type == 0)
        ShowCollisSFX((pjlEV_CDT *)ev);
}

// v12 0x00407590  (feb 0x00407050; 1.2: the damage message 0x4C6)
BOOL32 pwpBMB_PJL::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x4C6 && !exploded) {
        Explode(&lastHit, 0);
        msgSystem.PostMsg(this, 2, this);
    }
    return physPHYS::ProcessMsg(id, data, from, ret);
}

// v12 0x004075F0  (feb 0x004070B0)
// On the server, at a hit point: the event to the clients (the face's normal, the light there), its look here, and
// out of water 6 m of damage (weapon 9).
void pwpBMB_PJL::Explode(cdtINFO *ci, int water)
{
    pjlEV_CDT ev;
    ev.packId = 0;
    ev.tag = 0x40;
    ev.size = 0x26;
    ev.type = 0;
    ev.kind = 1;
    ev.normal = m3dVUnitY;
    ev.color = 0;
    if (!(ci->flags & 2) || !(entFlags & 4))
        return;
    ev.kind = water == 0;
    ev.color = gsEffEnvirColor(ci, 0, 0);
    ev.pos = *(m3dV *)ci->point;
    gsScenePtr->GetCDTFace(ci, 0, &ev.normal);
    gsSysServer->SendMsgClientAll(this, 0x40E, &ev, 0xA0000, 0xFFFFFFFF, 0xFFFFFFFD);
    exploded = 1;
    ShowCollisSFX(&ev);
    if (water)
        return;
    dmgDispatchExplosDamage(this, (m3dV *)ci->point, damage, 6.0f, 1, 9, ci, (u32)ownerId, 0);
    // (the game then tells wrSysStat of the hit when someone was hurt: Notify_WpnHit(ownerId, 9))
}

// v12 0x00407720  (feb 0x004071D0)
// Water: a splash (1.3) and its sound. Else a scorch mark (2), the explosion (0.15), its sound.
void pwpBMB_PJL::ShowCollisSFX(pjlEV_CDT *ev)
{
    animCREATE_DATA cd;
    sncSOUND_DESCR_3D *snd;
    cd.matr._MakeLCS2WCS_VY(&ev->pos, &ev->normal);
    if (ev->kind == 0) {
        float sz = 1.3f;
        entCreate(gsScenePtr, (char *)"sfx_water_splash", (char *)"sfx_water_splash", &cd, &sz, 0);
        snd = &pwpBmbSnd[3];
    } else {
        exploded = 1;
        wrsfxSCORCHMARK::Add(2.0f, 10.0f, pwpBOMB::pTexScorchMark, &ev->pos, ev->color, 0x5343524D, 0, 0);
        float sz = 0.15f;
        entCreate(gsScenePtr, (char *)"pwp_bmb_pjl_expl", (char *)"pwp_bmb_pjl_expl", &cd, &sz, 0);
        snd = &pwpBmbSnd[2];
    }
    gsSysSound->Play(snd, &ev->pos);
}

// ---------------------------------------------------------------------------------------------- the explosion

static m3dSPL_LINEAR1D *bmbSpl(int n)
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
static m3dSPL_LINEAR1D *bmbBurst(float t1, float t2, float t3, float v)
{
    m3dSPL_LINEAR1D *s = bmbSpl(4);
    s->SetKp(0, 0.0f, 0.0f);
    s->SetKp(1, t1, v);
    s->SetKp(2, t2, v);
    s->SetKp(3, t3, 0.0f);
    return s;
}
static m3dSPL_LINEAR1D *bmbFade(float t0, float a)
{
    m3dSPL_LINEAR1D *s = bmbSpl(2);
    s->SetKp(0, t0, a);
    s->SetKp(1, 1.0f, 0.0f);
    return s;
}

// (feb 0x004072C0) 70 puffs (in 1/6 s), 0.7 (+-0.1) s, 4 x s (+-s), faint (85), from within 8 x s at 5 x s (+-2.5 x s)
class pwpBMB_PJL_EXPL_SMOKE : public partEMITTER_PHYS {
public:
    pwpBMB_PJL_EXPL_SMOKE(float s) : partEMITTER_PHYS(70, 400.0f, 0.8f)
    {
        SetIntensity(0.0f, bmbBurst(0.033333335f, 0.13333334f, 0.16666667f, 400.0f));
        SetTimeLifePart(0.7f, 0.1f);
        SetSize(s * 4.0f, s, 0, 1.0f);
        SetColor(0xFFFFFFFF, 0, bmbFade(0.0f, 85.425f));
        SetTexture(pwpBOMB::pTexSmokeExpl);
        SetTextureRotAnim(180.5f, 179.5f, 0.0f, 0.0f);
        flags |= 4;
        cone = 180.0f;
        coneSpl = 0;
        spreadSpl = 0;
        spread = s * 8.0f;
        spreadMin = 0;
        speedASpl = 0;
        speedA = s * 5.0f;
        speedARand = s * 2.5f;
        flags |= 0x408;
        SetDegNearParams(3.0f, 0.1f);
    }
};
// (feb 0x004074C0) 30 flames (in 2/15 s), 0.2 (+-0.1) s, 9.1 x s (+-1.3 x s), from within 5 x s at 8 x s (+-4 x s)
class pwpBMB_PJL_EXPL_FLAME : public partEMITTER_PHYS {
public:
    pwpBMB_PJL_EXPL_FLAME(float s) : partEMITTER_PHYS(30, 100.0f, 0.3f)
    {
        SetIntensity(0.0f, bmbBurst(0.033333335f, 0.1f, 0.13333334f, 200.0f));
        SetTimeLifePart(0.2f, 0.1f);
        SetSize(s * 9.1f, s * 1.3f, 0, 1.0f);
        SetColor(0xFFFFFFFF, 0, bmbFade(0.0f, 127.5f));
        SetTexture(pwpBOMB::pTexFlameExpl);
        SetTextureRotAnim(180.5f, 179.5f, 0.0f, 30.0f);
        flags |= 4;
        cone = 180.0f;
        coneSpl = 0;
        spreadSpl = 0;
        spread = s * 5.0f;
        spreadMin = 0;
        speedASpl = 0;
        speedA = s * 8.0f;
        speedARand = s * 4.0f;
        flags |= 0x409;
        SetDegNearParams(3.0f, 0.1f);
    }
};
// (feb 0x004076C0) 120 bits of grit (in 1/6 s), 4 (+-1) s, 0.225 x s (+-0.075 x s), two frames, spinning, out of
// "grit_source" at 35 x s (+-7.5 x s) up and 17.5 x s (+-3.75 x s) across, falling at 50 x s, settling.
class pwpBMB_PJL_EXPL_GRIT : public partEMITTER_PHYS {
public:
    pwpBMB_PJL_EXPL_GRIT(float s) : partEMITTER_PHYS(120, 700.0f, 5.0f)
    {
        SetIntensity(0.0f, bmbBurst(0.033333335f, 0.13333334f, 0.16666667f, 700.0f));
        SetTimeLifePart(4.0f, 1.0f);
        SetSize(s * 0.225f, s * 0.075f, 0, 1.0f);
        SetColor(0xFFFFFFFF, 0, bmbFade(0.5f, 255.0f));
        SetTexture(pwpBOMB::pTexGritExpl);
        SetTextureRotAnim(180.5f, 179.5f, 0.0f, 90.0f);
        flags |= 4;
        SetTextureFrameAnim(2, 2, 0.0f, 3.0f);
        flags |= 2;
        spreadSpl = 0;
        spreadMin = 0;
        speedASpl = 0;
        spread = s * 8.0f;
        speedBSpl = 0;
        gravity.x = 0;
        gravity.z = 0;
        speedA = s * 35.0f;
        speedARand = s * 7.5f;
        speedB = s * 17.5f;
        speedBRand = s * 3.75f;
        gravity.y = s * -50.0f;
        SetCDTParam(0.0f, s * 0.3f, s * 0.3f, 3);
        flags |= 0x1A408;
    }
};

// (feb 0x00407900)
// 1 (init, the size): shown, scaled; its planes "pPlane3/4" additive; smoke and flame at it, grit from
// "grit_source" (hidden) settling on the floor under it; its animation; 5 s. 2: the emitters gone. 1000: its time,
// its animation (hidden once over).
BOOL32 pwpBMB_PJL_EXPL::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id == 1) {
        float s = *(float *)data;
        inst->flags |= 0x10;
        inst->root->SetStateYes(0x10);
        inst->Scale(s, s, s, 0);
        m3dV p;
        inst->GetPos(&p);
        objOBJ *o = objFindName(inst->root, (char *)"pPlane3");
        if (!o)
            return 0;
        o->SetStateRendYes(0x60);
        if (!(o = objFindName(inst->root, (char *)"pPlane4")))
            return 0;
        o->SetStateRendYes(0x60);
        if (!(smoke = new pwpBMB_PJL_EXPL_SMOKE(s)))
            return 0;
        smoke->SetPosDirSpeed(&p, 0, 0);
        if (!(flame = new pwpBMB_PJL_EXPL_FLAME(s)))
            return 0;
        flame->SetPosDirSpeed(&p, 0, 0);
        if (!(grit = new pwpBMB_PJL_EXPL_GRIT(s)))
            return 0;
        objOBJ *g = objFindName(inst->root, (char *)"grit_source");
        if (!g)
            return 0;
        g->SetStateProcYes(8, 0);
        if (!partSetAreaEmissionObj(grit, g, -1, 0))
            return 0;
        m3dV pl[2];
        m3dBOX box;
        ((m3dMATR *)inst->matr)->GetOrigin(&pl[0]);
        if (wrsfxFindFloor(&pl[0], &box))
            pl[0].y = box.vMin.y;
        else
            pl[0] = m3dVZero;
        ((m3dMATR *)inst->matr)->GetAxisY(&pl[1]);
        grit->SetCDTPlane(1, 0, (m3dPLANE *)pl);
        inst->SetAnimSeq(0, 0.0f);
        inst->flags &= ~1u;
        smoke->StartEmission();
        flame->StartEmission();
        grit->StartEmission();
        timer = 5.0f;
    } else if (id == 2) {
        if (grit) {
            grit->__vecDelDtor(1);
            grit = 0;
        }
        if (flame) {
            flame->__vecDelDtor(1);
            flame = 0;
        }
        if (smoke) {
            smoke->__vecDelDtor(1);
            smoke = 0;
        }
    } else if (id == 1000) {
        if (m3dUpdateTimeField(&timer, gsElapsedTime))
            msgSystem.PostMsg(this, 2, this);
        if (inst->AdvanceTime(gsElapsedTime, 1, 0))
            inst->flags |= 1;
    }
    return 0;
}
