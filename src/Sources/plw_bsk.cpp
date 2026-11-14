// plw_bsk.cpp -- pwpBASOOKA, the rocket launcher (weapon 6): no clip, the rocket shown in the tube while loaded, a
// smoke-and-flame "sfx_bsk_smoke" at the muzzle (Will Rock 1.2, v12.exe; feb obj plw_bsk.obj)
//
// Rebuilt from the 1.2 code (v12 0x004081F0..0x00409FFF, vtable 0x005F9DC0): the class, its INIT / TERM / FRAME.
// Not rebuilt: the shot (Shoot +0x68 v12 0x00408870, ProcessSHOOT +0x98 v12 0x00408950) and the rocket
// "sfx_bsk_bullet" (Create v12 0x00408470) with its emitters.

#include "game.h"
#include "../../include/core/wr_bases.h"
#include "../gs/gs_mp_server.h"
#include "../gs/gs_mp_client.h"
#include "../../include/game_shared/projectile.h"
#include "../m3d/m3d_spl.h"
#include "../m3d/m3d_ctrl.h"
#include "../../include/engine_render/shader.h"
#include "../../include/sfx_sob/sfx_api.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30
entENTITY *__fastcall pwpBSK_BULLET_Create(animINST *i);  // v12 0x00408470 "sfx_bsk_bullet" (below)

sncSOUND_DESCR_3D pwpBskSnd[4];                         // v12 0x00640378 shot, flight, hit, water hit
txmTEXTURE *pwpBASOOKA::pTexExplos;                     // v12 0x00640370 "sfx_bsk_explos"
txmTEXTURE *pwpBASOOKA::pTexScorchMark;                 // v12 0x00640518 "sfx_bsk_scorch_mark"
txmTEXTURE *pwpBASOOKA::pTexSmoke;                      // v12 0x00640360 "sfx_bsk_smoke"
txmTEXTURE *pwpBASOOKA::pTexTrail;                      // v12 0x00640368 "sfx_cloud"
txmTEXTURE *pwpBASOOKA::pTexRipple;                     // v12 0x00640364 "sfx_ripple"

// v12 0x004081F0  static initialiser (the sncSOUND_DESCR_3D constructor inline)
static struct pwpBSK_SND_INIT {
    pwpBSK_SND_INIT()
    {
        static const struct { const char *name; u32 flags; float minDist, maxDist; } s[4] = {
            { "wpn_basooka_shoot", 0, 0.0f, 0.0f },     { "wpn_basooka_fly", 2, 8.0f, 30.0f },
            { "wpn_basooka_hit", 0, 50.0f, 140.0f },    { "wpn_basooka_hit_water", 0, 40.0f, 140.0f },
        };
        for (int i = 0; i < 4; i++) {
            sncSOUND_DESCR_3D *d = &pwpBskSnd[i];
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
} pwpBskSndInit;

// v12 0x00408370  (feb 0x00407E20)
int __fastcall pwpBSKInit()
{
    if (!entRegisterClass((char *)"pwp_bsk", 'PBSK', 0x50, pwpBASOOKA::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"sfx_bsk_bullet", 'PBSK', 0x2D, pwpBSK_BULLET_Create, 0, 1))
        return 0;
    if (!(pwpBASOOKA::pTexExplos = txmManager->Add((char *)"sfx_bsk_explos", 0x40003, 1)))
        return 0;
    if (!(pwpBASOOKA::pTexScorchMark = txmManager->Add((char *)"sfx_bsk_scorch_mark", 0x40003, 1)))
        return 0;
    if (!(pwpBASOOKA::pTexSmoke = txmManager->Add((char *)"sfx_bsk_smoke", 0x40003, 1)))
        return 0;
    if (!(pwpBASOOKA::pTexTrail = txmManager->Add((char *)"sfx_cloud", 0x40003, 1)))
        return 0;
    if (!(pwpBASOOKA::pTexRipple = txmManager->Add((char *)"sfx_ripple", 0x40003, 1)))
        return 0;
    return sncLinkSndListData(4, pwpBskSnd) != 0;
}

// v12 0x004084F0  (feb ?)
// No clip (reload action frame 2), a shot every 1.4 s, damage 90, 100 at most, none at first; the model has no
// collision and starts hidden. At the muzzle: the "sfx_bsk_smoke" (a "dynamic", drawn on top, no collision, its
// sequence at 0.35 of its length, played once) with its "&bb_&trl_flame" stepping through a 4 x 4 sheet in 0.25 s;
// the "rocket" in the tube.
int pwpBASOOKA::ProcessINIT()
{
    animCREATE_DATA cd;
    if (!pwpWEAPON::ProcessINIT())
        return 0;
    SetShootRate(0.71428573f);
    clipSize = -1;
    clip = -1;
    reloadActFrame = 2;
    damage = 90.0f;
    inst->flags |= 0x10;
    inst->flags |= 1;
    SetAmmo(0.0f);
    inst->root->AllocVertNormalList();
    m3dV org, dir;
    GetOrgDirTip(&org, &dir);
    cd.matr.MakeLCS2WCS_VZ(&org, &dir);
    smoke = entCreate(gsScenePtr, (char *)"dynamic", (char *)"sfx_bsk_smoke", &cd, 0, 0);
    if (!smoke)
        return 0;
    smoke->inst->root->SetStateRendYes(0x1000000);
    smoke->inst->root->SetStateYes(0x10);
    smoke->inst->flags |= 0x10;
    animSEQ *s0 = &smoke->inst->tpl->seq[0];
    s0->lengthMs = s0->lengthMs * 0.35f;
    s0 = &smoke->inst->tpl->seq[0];
    s0->actionFrame[0] = (int)s0->frameStart + 1;
    ((dynDYNAMIC *)smoke)->StartAnim(0, smoke->inst->GetSeqLength(0), 0);
    smoke->flags2 |= 4;
    objOBJ *fl = objFindName(smoke->inst->root, (char *)"&bb_&trl_flame");
    if (!fl)
        return 0;
    trailFlame = new objMOD_TEX_FRAME_ANIM(0);
    trailFlame->SetParams(4, 4, 0.25f, 0.0f, -1, -1.0f, -1.0f, 0);
    trailFlame->AddObj(fl);
    maxAmmo = 100.0f;
    rocketObj = objFindName(inst->root, (char *)"rocket");
    return rocketObj != 0;
}

// v12 0x00408740  (feb ?)
void pwpBASOOKA::ProcessTERM()
{
    if (smoke) {
        smoke->flags2 &= ~4u;
        msgSystem.PostMsg(smoke, MSG_MSG_TERM, 0);
    }
    pwpWEAPON::ProcessTERM();
}

// v12 0x00408780  (feb ?)
// While out: the rocket hidden on SHOOT's action frame 0, shown again (with ammo) on SHOOT action frame 1 / PUT_ON
// action frame 1; the flame sheet restarted with the smoke's sequence (its action frame 0); the smoke at the muzzle.
void pwpBASOOKA::ProcessFRAME()
{
    pwpWEAPON::ProcessFRAME();
    if (inst->flags & 1)
        return;
    if (inst->IsActionFrame(7, 0)) {
        rocketObj->SetStateProcYes(8, 0);
    } else if (ammo > 0.0f) {
        if (inst->IsActionFrame(7, 1) || inst->IsActionFrame(6, 1))
            rocketObj->SetStateProcNo(8, 0);
    }
    inst->Validate(4);
    if (smoke->inst->actionFrames & 1) {
        trailFlame->cur = 0;
        trailFlame->time = 0;
        trailFlame->ApplyFrameNmb(0);
    }
    m3dV org;
    GetOrgDirTip(&org, 0);
    smoke->inst->SetPos(&org);
}

// v12 0x00409FA0  (feb 0x004095D0)
entENTITY *__fastcall pwpBASOOKA::Create(animINST *)
{
    pwpBASOOKA *w = new pwpBASOOKA;         // operator new(0x1D7), pwpWEAPON ctor
    if (!w)
        return 0;
    w->sndChannel = 0;
    w->owner = 0;
    w->trailFlame = 0;
    w->smoke = 0;
    w->rocketObj = 0;
    w->unk1C3 = 1.0f;
    w->wpnIndex = PWP_BASOOKA;
    return w;
}

// (the inline m3dSPL_LINEAR1D / m3dCTRL_1D_SPL / m3dCOLOR(u32) constructors of the weapons' effects)
static m3dSPL_LINEAR1D *pjlSpl(int n)
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
static m3dCTRL_1D *pjlCtrlSpl(float len, m3dSPL_LINEAR1D *s)
{
    m3dCTRL_1D_SPL *c = new m3dCTRL_1D_SPL;
    if (!c)
        return 0;
    m3dCtrlTimeInit(c);
    c->length = len;
    c->value = 0;
    c->spl = s;
    c->flags |= 6;
    return c;
}
static m3dCOLOR pjlColor(u32 c)
{
    m3dCOLOR k;
    k.r = (float)((c >> 16) & 0xFF);
    k.g = (float)((c >> 8) & 0xFF);
    k.b = (float)(c & 0xFF);
    k.a = (float)(c >> 24);
    return k;
}

// ================================================================================================================
// The shot and the rocket "sfx_bsk_bullet" (pwpBSK_BULLET : pjlPJL<pwpBSK_PJL_SFX>, vtable v12 0x005F9C80, size
// 0x1B4): 25 s at most, 50 a second along the aim and 50 more each second, 0.2 big, the weapon's damage over 5 m
// (weapon 6). Its trail, a light following it, it fades in over its first 3 m; its own flight voice (7..25).

struct wrsfxSCORCHMARK {
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
};                                                      // v12 0x00467DE0

#pragma pack(push, 1)
// The shot asked for (msg 0x4C7 to the server): gsEVENT type 6 (its ctor v12 0x00417000 inline), from the tip and
// which way.
struct pwpBSK_SHOT : msgDATA {
    u8    type;                     // +0x04 6
    m3dV  org;                      // +0x05 the tip (GetOrgDirTip)
    m3dV  dir;                      // +0x11 to the aim
};                                  // 0x1D
#pragma pack(pop)
extern msgDATA_PACK pwpPackShot;                        // v12 0x0064188C (pwp_comm.cpp) -- not rebuilt

// v12 0x00408870  (feb 0x00408330; 1.2: the shared shot event, msg 0x4C7, a client takes the rocket at once)
int pwpBASOOKA::Shoot(entENTITY *, m3dV *aim)
{
    if (!wpnWEAPON::Shoot((entENTITY *)owner, aim))
        return 0;
    m3dV org, dir;
    GetOrgDirTip(&org, &dir);
    dir.x = aim->x - org.x;
    dir.y = aim->y - org.y;
    dir.z = aim->z - org.z;
    m3dNormalize(&dir);
    pwpBSK_SHOT m;
    m.packId = (u8)pwpPackShot.id;
    m.tag = 0x40;
    m.size = 0x1D;
    m.type = 6;
    m.org = org;
    m.dir = dir;
    gsSysClient->SendMsgServer(plrPlayer, 0x4C7, &m, 0x20000);
    if (!(entFlags & 4))
        SubtractAmmo(1.0f);
    return 1;
}

// v12 0x00408950  (feb 0x00408420)
// On the server: the rocket taken, the shot passed on to the clients, the rocket made at the tip facing the shot.
// Everywhere: the muzzle smoke at the tip, played again; the shot's sound.
void pwpBASOOKA::ProcessSHOOT(void *msg)
{
    pwpBSK_SHOT *m = (pwpBSK_SHOT *)msg;
    pjlPJL_INIT p;
    memset(&p, 0, sizeof(p));
    p.phys.packId = 0;
    p.phys.tag = 0x40;
    p.phys.size = 0x55;
    p.phys.radius = 0.1f;
    p.phys.lifetime = 2000000.0f;
    p.phys.unk24 = 1;
    p.damage = -1.0f;
    p.radius = 0;
    p.weaponType = -1;
    p.isGeneral = -1;
    p.ownerId = 0xFFFFFFFF;
    p.termDelay = -1.0f;
    p.explosive = 1;
    animCREATE_DATA cd;
    if (entFlags & 4) {
        SubtractAmmo(1.0f);
        gsSysServer->SendMsgClientAll(owner, 0x4C7, (msgDATA *)msg, 0xA0000, 0xFFFFFFFF, 0xFFFFFFFD);
        const float speed = 50.0f;                      // v12 0x005F9E94
        p.phys.lifetime = 25.0f;
        p.phys.unk28[0] = p.phys.unk28[1] = 0;
        p.phys.vel[0] = m->dir.x * speed;
        p.phys.vel[1] = m->dir.y * speed;
        p.phys.vel[2] = m->dir.z * speed;
        p.phys.unk24 = 1000;
        p.phys.radius = 0.2f;
        p.phys.accel[0] = m->dir.x * speed;
        p.phys.accel[1] = m->dir.y * speed;
        p.phys.accel[2] = m->dir.z * speed;
        p.damage = GetDamage();
        p.radius = 5.0f;
        p.isGeneral = 1;
        p.weaponType = 6;
        p.ownerId = (u32)owner->mpId;
        cd.matr.MakeLCS2WCS_VZ(&m->org, &m->dir);
        entCreate(gsScenePtr, (char *)"sfx_bsk_bullet", (char *)"sfx_bsk_bullet", &cd, &p, 0);
    }
    m3dV org, dir;
    GetOrgDirTip(&org, &dir);
    smoke->inst->SetPos(&org);
    ((dynDYNAMIC *)smoke)->StartAnim(0, 0.0f, 0);
    if (sndChannel)
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpBskSnd[0], 0);
}

// ---------------------------------------------------------------------------------------------- the rocket

// (feb 0x004087E0, v12 0x00408D30) the smoke trail: 100 a second (after 1/12 s), 0.8 (+-0.2) s, growing 0.05 ->
// 0.65 of 0.25 x s, a faint white (102), carried along at 0.02 x s of the rocket's speed.
class pwpBSK_TRAIL : public partEMITTER_PHYS {
public:
    pwpBSK_TRAIL(float s) : partEMITTER_PHYS(100.0f, 1.0f)
    {
        m3dSPL_LINEAR1D *k = pjlSpl(4);
        k->SetKp(0, 0.0f, 0.0f);
        k->SetKp(1, 0.083333336f, 0.0f);
        k->SetKp(2, 0.16666667f, 100.0f);
        k->SetKp(3, 0.625f, 100.0f);
        SetIntensity(100.0f, k);
        flags |= 0x200;
        SetTimeLifePart(0.8f, 0.2f);
        k = pjlSpl(3);
        k->SetKp(0, 0.0f, 0.05f);
        k->SetKp(1, 0.5f, 0.45f);
        k->SetKp(2, 1.0f, 0.65f);
        float z = s * 0.0f;
        SetSize(s * 0.25f, z, k, 1.0f);
        k = pjlSpl(3);
        k->SetKp(0, 0.0f, 0.0f);
        k->SetKp(1, 0.15f, 102.0f);
        k->SetKp(2, 1.0f, 0.0f);
        SetColor(0xFFFFFF, 0, k);
        SetTextureRotAnim(0.0f, 180.0f, 0.0f, 180.0f);
        flags |= 4;
        speedA = z;
        speedARand = z;
        speedK = s * 0.02f;
        speedASpl = 0;
        speedKRand = z;
        cone = 180.0f;
        coneSpl = 0;
        flags |= 0x10;
        SetTexture(pwpBASOOKA::pTexTrail);
        flags |= 0x408;
    }
};
// (feb 0x00408640, v12 0x00408B90) the blast: 55 frames of a 4 x 4 sheet, 3 x s, 40 a second for 5/24 s, from
// within s, out at 3 x s (+-0.8 x s) in an 85-degree cone round the face's normal.
class pwpBSK_EXPLOS : public partEMITTER_PHYS {
public:
    pwpBSK_EXPLOS(float s) : partEMITTER_PHYS(0x37, 40.0f, 1.0f)
    {
        m3dSPL_LINEAR1D *k = pjlSpl(3);
        k->SetKp(0, 0.0f, 40.0f);
        k->SetKp(1, 0.20833333f, 40.0f);
        k->SetKp(2, 0.625f, 0.0f);
        SetIntensity(0.0f, k);
        SetAnimEmission(4.0f, 0.0f);
        flags |= 0x200;
        SetTimeLifePart(1.0f, 0.0f);
        float a = s * 3.0f;
        SetSize(a, s * 0.0f, 0, 1.0f);
        SetColor(0xFFFFFFFF, 0, 0);
        SetTextureFrameAnim(4, 4, 1.0f, 0.0f);
        flags |= 2;
        SetTextureRotAnim(0.0f, 180.0f, 0.0f, 0.0f);
        flags |= 4;
        speedA = a;
        speedASpl = 0;
        spread = s;
        speedARand = s * 0.8f;
        spreadSpl = 0;
        spreadMin = 0;
        cone = 85.0f;
        coneSpl = 0;
        SetTexture(pwpBASOOKA::pTexExplos);
        flags |= 0x408;
        SetDegNearParams(10.0f, 0.1f);
    }
};

// The rocket's effect: the trail, the light, where it was fired from.
struct pwpBSK_PJL_SFX {
    partEMITTER_PHYS *trail;       // +0x00
    pteSPOT_ANIM     *spot;        // +0x04
    m3dV              start;       // +0x08
    // v12 0x00408F90  (feb 0x00408A30) its sequence, see-through at first; a light (3, orange) following it; the trail
    int StartSFX(physPHYS *ph, pjlPJL_INIT *)
    {
        animINST *a = ph->inst;
        a->SetAnimSeq(0, 0.0f);
        a->root->SetTransparency(0x80, 0);
        spot = new pteSPOT_ANIM;
        if (!spot)
            return 0;
        spot->SetCtrlPos(pteSpotNewFollow(a, 0));
        spot->SetRadius(3.0f);
        m3dCOLOR base;
        base.r = 85.0f;
        base.g = 66.0f;
        base.b = 33.0f;
        base.a = 255.0f;
        spot->SetColor(pjlColor(0xFFFFC850), base);
        spot->Register();
        trail = new pwpBSK_TRAIL(1.0f);
        if (!trail)
            return 0;
        a->GetPos(&start);
        trail->SetPosDirSpeed(&start, 0, 0);
        trail->StartEmission();
        return 1;
    }
    // (v12 0x00409A60 inline) its animation; seen more the further it is from where it was fired (all at 3 m); the
    // trail follows
    void UpdateSFX(physPHYS *ph)
    {
        animINST *a = ph->inst;
        a->Validate(4);
        m3dV p;
        a->GetPos(&p);
        a->AdvanceTime(gsElapsedTime, WR_FIELD(int, a, 0xE0) ? 1 : 0, 0);
        float dx = p.x - start.x, dy = p.y - start.y, dz = p.z - start.z;
        float d = m3dSqrt(dx * dx + dy * dy + dz * dz);
        float t;
        if (!(d > 0.0f))
            t = 0.0f;
        else if (!(d <= 3.0f))
            t = 255.0f;
        else
            t = d * 85.0f;
        a->root->SetTransparency(0x80, (int)t);
        if (trail)
            trail->SetPosDirSpeed(&p, 0, 0);
    }
    // (v12 0x00409A00 / 0x00409CE0 inline) the light dropped (unless the physics keeps it: +0xB0), the trail stopped
    void StopSFX(physPHYS *ph)
    {
        if (spot && !WR_FIELD(int, ph->math, 0xB0)) {
            spot->__vecDelDtor(1);
            spot = 0;
        }
        if (trail) {
            trail->StopEmission();
            trail = 0;
        }
    }
};

#pragma pack(push, 1)
class pwpBSK_BULLET : public pjlPJL<pwpBSK_PJL_SFX> {
public:
    u8          _1A4[0xC];         // +0x1A4
    sncCHANNEL *flyVoice;          // +0x1B0 the flight's (a sphere, 7..25)
    BOOL32 ProcessINIT(physINIT *p);                    // v12 0x00409120 (feb 0x00408BC0)
    void   ShowCollisSFX(pjlEV_CDT *ev);                // v12 0x004094B0 (feb 0x00408ED0)
};
#pragma pack(pop)

// v12 0x00408470  (feb 0x00407F20)
entENTITY *__fastcall pwpBSK_BULLET_Create(animINST *)
{
    pwpBSK_BULLET *b = new pwpBSK_BULLET;
    if (!b)
        return 0;
    b->sfx.trail = 0;
    b->sfx.spot = 0;
    b->flyVoice = 0;
    return b;
}

// v12 0x00409120  (feb 0x00408BC0)
// The template's INIT (its effect, its sound channel), flying; the physics flag 2; shown, drawn after; "BULLET"
// turned along its flight (its Y axis back along the speed); its own flight voice playing.
BOOL32 pwpBSK_BULLET::ProcessINIT(physINIT *p)
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
    pjlFlags |= 0x12;
    math->flags |= 2;
    inst->flags |= 0x10;
    inst->flags |= 0x1000;
    objOBJ *b = objFindName(inst->root, (char *)"BULLET");
    m3dV back;
    back.x = math->vel[0] * -1.0f;
    back.y = math->vel[1] * -1.0f;
    back.z = math->vel[2] * -1.0f;
    m3dV pos;
    math->GetPos(&pos);
    inst->Validate(4);
    if (b) {
        float sx, sy, sz;
        ((m3dMATR *)((u8 *)b + 0x3C))->GetScale(&sx, &sy, &sz);
        m3dMATR m, inv;
        m.Identity();
        m._MakeLCS2WCS_VY(&pos, &back);
        m.Scale(sx, sy, sz, 1);
        ((m3dMATR *)(WR_FIELD(u8 *, b, 0x2C) + 0x3C))->Invert(&inv);
        m3dMultiplyMatr(&m, &inv, (m3dMATR *)((u8 *)b + 0x7C));
        b->DeclareProcAnim();
    }
    inst->Invalidate(4);
    inst->Validate(4);
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;          // (a sncCHANNEL_SPH there)
    if (c) {
        c->minDist = 7.0f;
        c->maxDist = 25.0f;
    }
    flyVoice = c;
    if (!flyVoice)
        return 0;
    gsSysSound->AddChannel(flyVoice, this, 0);
    flyVoice->Play((sncSOUND_DESCR *)&pwpBskSnd[1], 0);
    return 1;
}

// v12 0x004094B0  (feb 0x00408ED0; 1.2: the light 3 -> 6, was 7 -> 14)
// Into water: the water's burst, the light gone, the water's sound. Else the blast along the face's normal, a
// scorch mark (3), the light flaring to 6 and going out in 1.5 s, the hit's sound. The trail stops.
void pwpBSK_BULLET::ShowCollisSFX(pjlEV_CDT *ev)
{
    animCREATE_DATA cd;
    if (!sfx.spot)
        return;
    m3dV p;
    inst->GetPos(&p);
    sncSOUND_DESCR_3D *snd;
    if (ev->kind == 0) {
        cd.matr._MakeLCS2WCS_VY(&ev->pos, &m3dVUnitY);
        entCreate(gsScenePtr, (char *)"sfx_water_expl", (char *)"sfx_water_expl", &cd, 0, 0);
        if (sfx.spot)
            sfx.spot->__vecDelDtor(1);
        snd = &pwpBskSnd[3];
    } else {
        pwpBSK_EXPLOS *x = new pwpBSK_EXPLOS(1.0f);
        if (!x)
            return;
        x->SetPosDirSpeed(&p, &ev->normal, 0);
        x->StartEmission();
        wrsfxSCORCHMARK::Add(3.0f, 10.0f, pwpBASOOKA::pTexScorchMark, &ev->pos, ev->color, 0x5343524D, 0, 0);
        m3dSPL_LINEAR1D *k = pjlSpl(4);
        k->SetKp(0, 0.0f, 3.0f);
        k->SetKp(1, 0.1f, 6.0f);
        k->SetKp(2, 0.2f, 6.0f);
        k->SetKp(3, 1.0f, 0.0f);
        sfx.spot->SetCtrlRad(pjlCtrlSpl(1.5f, k));
        sfx.spot->SetCurTime(0.0f);
        WR_FIELD(u32, sfx.spot, 0xF4) |= 4;
        WR_FIELD(u32, sfx.spot, 4) |= 0x40;
        snd = &pwpBskSnd[2];
    }
    sfx.spot = 0;
    gsSysSound->Play(snd, &p);
    if (sfx.trail) {
        sfx.trail->StopEmission();
        sfx.trail = 0;
    }
}
static_assert(sizeof(pwpBSK_BULLET) == 0x1B4, "pwpBSK_BULLET"); static_assert(offsetof(pwpBSK_BULLET, sfx) == 0x16F, "sfx");
