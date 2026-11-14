// plw_bul_wpn.cpp -- pwpBULLET_WPN, the base of the hit-scan weapons (colt, shotgun, machine gun, minigun): the
// shot's rays, their hits, the muzzle flare / smoke, the ejected shells, the hit effects
// (Will Rock 1.2, v12.exe; feb obj plw_bul_wpn.obj)
//
// Rebuilt from the 1.2 code (v12 0x0040A1C0..0x0040C5AF, vtable 0x005F9FD0). Fields at feb + 0x1C. 1.2 changes,
// from the disassembly:
//   - the shot is one message of up to 20 packed ray records (pwpMCG_RAY, 0x15 bytes: hit class in the high
//     nibble of byte 0, bit 2 "second ray", bit 1 "make sound"; target mpId; object / face id; point), sent to the
//     server (0x4C7) and to this side (0x4C8 -> ShootBody). feb sent the face normal and the light colour, 1.2
//     lets every side work them out (ShowSFX);
//   - a ray that hits a water volume entity (class bits 1) gets a second ray past it (200 m) and a splash;
//   - Shoot starts from the crosshair's own hit (v12 0x0098EAC8) and the screen crosshair (0x0098EA9C/A0);
//   - ShootBody credits the statistics once per shot (NPC / player hit), the damage carries the target instance
//     and the object hit; the flare and smoke are restarted in ShowShotSFX (new slot +0xD8);
//   - the shell channel is put in mute groups 0x1400; pwpBulWpnInit registers a message packer (v12 0x006408EC).
// Also the shells' and the muzzle smoke's emitters (pwpMCG_SHELL / pwpMCG_MUZZLE_SMOKE) and StartShells. Not rebuilt
// here yet: FillCDTShootListSpread (v12 0x0040B360, the aim help towards NPCs of the spread weapons), the hit
// effects (pwpMCG_HIT 0x0040A2B0.., pwpMCG_PARTS / _SMOKE, pwpEV_CDT_MCG) and the packer.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../gs/gs_mp_misc.h"
#include "../gs/gs_mp_client.h"

u32 __fastcall gsEffEnvirColor(cdtINFO *ci, m3dCOLOR *col, m3dCOLOR *dyn); // v12 0x005130E0 (gs_effect.cpp)
extern msgDATA_PACK pwpBulPack;                         // v12 0x006408EC (1.2 only, name ours) -- not rebuilt
sncSOUND_DESCR_3D pwpBulSnd[5];                         // v12 0x00640520: 0 shot, 1 shell, 2 wall hit, 3 flesh, 4 water
// v12 0x0040A000  static initialiser (it was never defined here: the build made zeroed data of it, nameless sounds --
// the Tommy gun's and the hits' sounds were silent)
static struct pwpBUL_SND_INIT {
    pwpBUL_SND_INIT()
    {
        static const struct { const char *name; u32 flags; float volume; int delay; float minDist, maxDist; } s[5] = {
            { "wpn_machinegun_shoot", 0, 0.0f, 0, 0.0f, 0.0f },
            { "wpn_machinegun_shell", 0, -15.0f, 0, 0.0f, 0.0f },
            { "wpn_machinegun_hit", 0x400000, 0.0f, 200, 5.0f, 25.0f },
            { "wpn_machinegun_hit_body", 0x400000, 0.0f, 200, 5.0f, 15.0f },
            { "wpn_machinegun_hit_water", 0x400000, 0.0f, 200, 5.0f, 20.0f },
        };
        for (int i = 0; i < 5; i++) {
            sncSOUND_DESCR_3D *d = &pwpBulSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = s[i].volume;
            d->delay = s[i].delay;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].minDist;
            d->maxDist = s[i].maxDist;
            d->vtbl = 0;                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} pwpBULSndInit;
int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30
int __fastcall wrsfxFindFloor(m3dV *pos, m3dBOX *box);  // v12 0x00467D10 (sfx_appear.cpp)
// the crosshair ray's hit and the crosshair on screen: gscCursor.aimInfo (v12 0x0098EAC8) / .x .y (0x0098EA9C)

struct pwpMCG_PARTS { static txmTEXTURE *pTexPart; };
struct pwpMCG_SMOKE { static txmTEXTURE *pTexSmoke; };
struct pwpMCG_MUZZLE_SMOKE_TEX { static txmTEXTURE *pTexSmoke; };
struct pwpMCG_SHELL_TEX { static txmTEXTURE *pTexShell; };
txmTEXTURE *pwpMCG_PARTS::pTexPart;                     // v12 0x0064072C
txmTEXTURE *pwpMCG_SMOKE::pTexSmoke;                    // v12 0x00640728
txmTEXTURE *pwpMCG_MUZZLE_SMOKE_TEX::pTexSmoke;         // v12 0x006408E4 (pwpMCG_MUZZLE_SMOKE::pTexSmoke)
txmTEXTURE *_pwpBulTexScorchMark;                       // v12 0x006408F8
txmTEXTURE *pwpMCG_SHELL_TEX::pTexShell;                // v12 0x006408F4 (pwpMCG_SHELL::pTexShell)
static const float pwpBulEps = 1e-6f;                   // v12 0x005F9F48
float pwpBulEpsSq;                                      // v12 0x00640730 (static init v12 0x0040A890)
static struct pwpBUL_EPS_INIT { pwpBUL_EPS_INIT() { pwpBulEpsSq = pwpBulEps * pwpBulEps; } } pwpBulEpsInit;

// not rebuilt yet (effect classes)
entENTITY *__fastcall pwpMCG_HIT_Create(animINST *i);  // v12 0x0040A2B0 (pwpMCG_HIT::Create, 0x19B bytes)

// the emitters' run-time fields not named yet (particle.cpp's Update is not rebuilt)
#define PART_F(e, off) (*(float *)((u8 *)(e) + (off)))
#define PART_P(e, off) (*(void **)((u8 *)(e) + (off)))

// pwpMCG_SHELL (0x2CC, vtable v12 0x005FA160): the ejected shells, ringing on the floor (their sound channel and
// sound in +0x2C4 / +0x2C8).
class pwpMCG_SHELL : public partEMITTER_PHYS {
public:
    sncCHANNEL       **snd;                             // +0x2C4
    sncSOUND_DESCR_3D *sndDesc;                         // +0x2C8
    pwpMCG_SHELL(sncCHANNEL **ch, sncSOUND_DESCR_3D *d, float speed, float spread, m3dSPL_LINEAR1D *size);
    int PartApplyCDT(partPART *p, m3dV *v, float t);    // slot +0x2C v12 0x0040C010
};
// pwpMCG_MUZZLE_SMOKE (0x2C4, vtable v12 0x005FA194, no overrides): the puff at the muzzle.
class pwpMCG_MUZZLE_SMOKE : public partEMITTER_PHYS {
public:
    pwpMCG_MUZZLE_SMOKE(float scale);                   // v12 0x0040C090
};

// a new 1D curve of n key points (the inline m3dSPL_LINEAR1D construction)
static m3dSPL_LINEAR1D *pwpNewSpl1D(int n)
{
    m3dSPL_LINEAR1D *s = new m3dSPL_LINEAR1D;
    if (s) {
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
    }
    return s;
}

// v12 0x0040BE00
// 50 a second, 4.5 (+-1.5) s each, the size from the curve (0.008 .. 0.025 over the first 1/6 s, unless given),
// fading out at the end, two frames, spinning; thrown at speed (+- 20 %) with gravity 5, bouncing (0.4) on the
// level; 0.011 the size scale.
pwpMCG_SHELL::pwpMCG_SHELL(sncCHANNEL **ch, sncSOUND_DESCR_3D *d, float speed, float spread, m3dSPL_LINEAR1D *size)
    : partEMITTER_PHYS(50.0f, 6.0f)
{
    sndDesc = d;
    snd = ch;
    SetTimeLifePart(4.5f, 1.5f);
    if (!size) {
        size = pwpNewSpl1D(2);
        size->SetKp(0, 0.0f, 0.008f);
        size->SetKp(1, 0.16666667f, 0.025f);
    }
    SetSize(0.011f, 0.0f, size, 1.0f);
    m3dSPL_LINEAR1D *a = pwpNewSpl1D(3);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 0.9f, 255.0f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureFrameAnim(2, 2, 0.0f, 4.0f);
    SetTextureRotAnim(0.0f, 60.0f, 0.0f, 576.0f);
    flags |= 4;
    flags |= 2;
    PART_F(this, 0xBC) = speed * 0.2f;
    PART_F(this, 0xE0) = 0;
    PART_F(this, 0xE4) = -5.0f;
    PART_F(this, 0xE8) = 0;
    PART_F(this, 0xB8) = speed;
    PART_P(this, 0xC0) = 0;
    PART_F(this, 0xD0) = 1.0f;
    PART_F(this, 0xD4) = 0;
    PART_F(this, 0xEC) = spread;
    PART_P(this, 0xF4) = 0;
    PART_F(this, 0xF0) = 0;
    SetTexture(pwpMCG_SHELL_TEX::pTexShell);
    flags |= 0x400;
    flags |= 8;
    SetCDTParam(0.015f, 0.4f, 0.3f, 4);
    flags |= 0x10000;
    flags |= 0x10000000;
    flags |= 0x40000000;
}

// v12 0x0040C010
// A shell's first bounce (byte +0x39 == 1) rings at its place, unless the channel still plays.
int pwpMCG_SHELL::PartApplyCDT(partPART *p, m3dV *v, float t)
{
    if (!partEMITTER_PHYS::PartApplyCDT(p, v, t))
        return 0;
    if (((u8 *)p)[0x39] == 1 && !(*snd)->IsPlay(-1)) {
        wrVCall<void>(*snd, 0x14, (m3dV *)((u8 *)p + 0x10), 0, 0);   // the channel's position
        wrVCall<void>(*snd, 4, (sncSOUND_DESCR *)sndDesc, 0);        // Play
    }
    return 1;
}

// v12 0x0040C090
// 15 particles; a burst: the intensity curve 0 -> 30 at 1/8 -> 0 at 5/6 of a 1 s window; 1 (+-0.5) s each; scale
// * (0.075 + 0.025) in size, fading in and out, turning; speed scale * 0.1.
pwpMCG_MUZZLE_SMOKE::pwpMCG_MUZZLE_SMOKE(float scale) : partEMITTER_PHYS(15, 30.0f, 1.5f)
{
    m3dSPL_LINEAR1D *in = pwpNewSpl1D(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.125f, 30.0f);
    in->SetKp(2, 0.8333333f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(1.0f, 0.0f);
    SetTimeLifePart(1.0f, 0.5f);
    float b = scale * 0.025f;
    SetSize(scale * 0.075f, b, 0, 1.0f);
    m3dSPL_LINEAR1D *a = pwpNewSpl1D(3);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.21f, 76.5f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 90.0f);
    float sp = scale * 0.1f;
    flags |= 4;
    PART_P(this, 0xC0) = 0;
    PART_F(this, 0xFC) = 36.0f;
    PART_F(this, 0xB8) = sp;
    PART_F(this, 0xBC) = sp;
    PART_P(this, 0x100) = 0;
    PART_F(this, 0xEC) = b;
    PART_P(this, 0xF4) = 0;
    PART_F(this, 0xF0) = 0;
    SetTexture(pwpMCG_MUZZLE_SMOKE_TEX::pTexSmoke);
    flags |= 0x400;
    flags |= 8;
    flags |= 0x10000000;
}
struct wrsfxSCORCHMARK {
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
                                                        // v12 0x00467DE0 (feb 0x0045DED0) // ? arg names
};

#pragma pack(push, 1)
struct pwpMCG_RAY {                                     // one ray of a shot, 0x15 bytes
    u8    cls;                                          // +0x00 hit class << 4 (0 water, 1 wall / thing, 2 flesh, 3 none)
                                                        //       | 4 second ray | 2 sound | 1 ? (ctor: low bits 3)
    int   targetId;                                     // +0x01 mpId of the entity hit, -1 none
    short objId;                                        // +0x05
    m3dV  point;                                        // +0x07
    short faceId;                                       // +0x13
};
struct pwpMCG_SHOOT_V12 : msgDATA {                     // 0x1AA bytes
    u8    wpn;                                          // +0x04 weapon index (0xFF at first)
    u8    nRays;                                        // +0x05
    pwpMCG_RAY ray[20];                                 // +0x06
};
#pragma pack(pop)

static inline void pwpRayInit(pwpMCG_RAY *r)            // v12 0x0040C590
{
    r->cls = (r->cls & 0xF3) | 3;
    r->targetId = -1;
    r->objId = -1;
}

// v12 0x0040A1C0  (feb 0x004097D0)
int __fastcall pwpBulWpnInit()
{
    if (!entRegisterClass((char *)"sfx_mcg_hit", 'PMCG', 0x2D, pwpMCG_HIT_Create, 0, 1))
        return 0;
    if (!(pwpMCG_PARTS::pTexPart = txmManager->Add((char *)"sfx_mcg_part", 0x40003, 1)))
        return 0;
    if (!(pwpMCG_SMOKE::pTexSmoke = txmManager->Add((char *)"sfx_mcg_cloud", 0x40003, 1)))
        return 0;
    if (!(pwpMCG_MUZZLE_SMOKE_TEX::pTexSmoke = txmManager->Add((char *)"sfx_mcg_cloud", 0x40003, 1)))
        return 0;
    if (!(_pwpBulTexScorchMark = txmManager->Add((char *)"sfx_mcg_scorch_mark", 0x40003, 1)))
        return 0;
    if (!(pwpMCG_SHELL_TEX::pTexShell = txmManager->Add((char *)"sfx_mcg_shell_2x2", 0x40003, 1)))
        return 0;
    if (!sncLinkSndListData(5, pwpBulSnd))
        return 0;
    msgSystem.RegisterMsgData(&pwpBulPack);
    return 1;
}

// v12 0x0040A370  (feb 0x00409960)
pwpBULLET_WPN::pwpBULLET_WPN()
{
    flare = 0;
    muzzleSmoke = 0;
    shells = 0;
    flareScale = 1.0f;
    wpnIndex = -1;
    shellSnd = 0;
    *(u32 *)((char *)this + 0xC) |= 2;      // msgADDR flags: gets MSG_MSG_DESTROY_ADDR
}

// v12 0x0040A3D0  (feb 0x004099C0)
int pwpBULLET_WPN::ProcessINIT()
{
    if (!pwpWEAPON::ProcessINIT())
        return 0;
    inst->root->AllocVertNormalList();
    return 1;
}

// v12 0x0040A400  (feb 0x004099F0)
// The muzzle flare: a "dynamic" entity "sfx_mcg_flare", drawn with the weapon (render state 0x1000000), no
// collision, its first sequence played, scaled when scale != 1.
int pwpBULLET_WPN::CreateFlare(float scale)
{
    flare = entCreate(gsScenePtr, (char *)"dynamic", (char *)"sfx_mcg_flare", (animCREATE_DATA *)0, 0, 0);
    if (!flare)
        return 0;
    flare->inst->root->SetStateRendYes(0x1000000);
    flare->inst->root->SetStateYes(0x10);
    flare->inst->flags |= 0x10;
    ((dynDYNAMIC *)flare)->StartAnim(0, flare->inst->GetSeqLength(0), 0);
    flare->flags2 |= 4;
    if (scale != 1.0f)
        flare->inst->Scale(scale, scale, scale, 0);
    return 1;
}

// v12 0x0040A4E0  (feb 0x00409AE0)
int pwpBULLET_WPN::CreateSmoke(float scale)
{
    muzzleSmoke = new pwpMCG_MUZZLE_SMOKE(scale);
    return muzzleSmoke != 0;
}

// v12 0x0040A530  (feb 0x00409B30)
sncSOUND_DESCR_3D *pwpBULLET_WPN::GetShellSoundDesc()
{
    return &pwpBulSnd[1];
}

// v12 0x0040A540  (feb 0x00409B40)
// The shell emitter and its sound channel (a sphere, 1..5).
int pwpBULLET_WPN::CreateShell(float a, float b, m3dSPL_LINEAR1D *spl)
{
    sncCHANNEL **ch = &shellSnd;
    for (int i = 0; i < 1; i++, ch++) {
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        if (c) {
            c->minDist = 1.0f;
            c->maxDist = 5.0f;
        }
        *ch = c;
        if (!c)
            return 0;
        c->flags |= 0x1400;
        gsSysSound->AddChannel(*ch, this, 0);
    }
    shells = new pwpMCG_SHELL(&shellSnd, GetShellSoundDesc(), a, b, spl);
    return shells != 0;
}

// v12 0x0040A6C0  (feb 0x00409CC0)
// The flare and the smoke follow the muzzle.
void pwpBULLET_WPN::ProcessFRAME()
{
    pwpWEAPON::ProcessFRAME();
    if (inst->flags & 1)
        return;
    inst->Validate(4);
    m3dV org;
    GetOrgDirTip(&org, 0);
    if (flare)
        flare->inst->SetPos(&org);
    if (muzzleSmoke)
        muzzleSmoke->SetPosDirSpeed(&org, &m3dVUnitY, 0);
}

// v12 0x0040A730  (feb 0x00409D30)
void pwpBULLET_WPN::ProcessTERM()
{
    pwpWEAPON::ProcessTERM();               // (the shared empty function)
    if (muzzleSmoke)
        wrVCall<void *>(muzzleSmoke, 0, 1u);
    if (shells)
        wrVCall<void *>(shells, 0, 1u);
    if (flare) {
        flare->flags2 &= ~4u;
        msgSystem.PostMsg(flare, MSG_MSG_TERM, 0);
    }
}

// v12 0x0040A790  (feb 0x00409D90, rewritten in 1.2)
// One ray record from a collision hit. 0 (no record) when an instance without an entity, or not an entity, is hit.
int pwpBULLET_WPN::PrepareShootMsg(pwpMCG_RAY *r, cdtINFO *ci)
{
    if (!ci->inst)
        r->targetId = -1;
    else {
        entENTITY *e = ci->inst->ent;
        if (!e)
            return 0;
        if (e->sign != 0x454E5424)
            return 0;
        r->targetId = e->mpId;
    }
    r->objId = (short)ci->objId;
    if (!(ci->flags & 2))
        r->cls = (r->cls & 0xF) | 0x30;
    else {
        if (!ci->inst)
            r->cls = (r->cls & 0xF) | 0x10;
        else {
            entENTITY *e = ci->inst->ent;
            if (e->unkBC & 1)               // a water volume
                r->cls &= 0xF;
            else if (e->classId == AI_SIG_NPC)
                r->cls = (r->cls & 0xF) | (wrEVCall<BOOL32>(e, 0x1C8) ? 0x10 : 0x20);
            else if (e->classId == AI_SIG_PLAYER)
                r->cls = (r->cls & 0xF) | 0x20;
            else
                r->cls = (r->cls & 0xF) | 0x10;
        }
        r->faceId = (short)ci->faceId;
    }
    r->point = *(m3dV *)ci->point;
    return 1;
}

// v12 0x0040A8B0  (feb 0x00409EB0)
// The shot: the rays FillCDTShootList gives from the muzzle (at most 10), a second ray past water, then the
// message to the server and (when this side does not run the weapon) to itself.
int pwpBULLET_WPN::Shoot(entENTITY *target, m3dV *at)
{
    cdtINFO aim;
    memset(&aim, 0, sizeof(aim));
    *(m3dV *)aim.point = m3dVZero;
    cdtINFO hits[10];
    for (int i = 0; i < 10; i++) {
        memset(&hits[i], 0, sizeof(cdtINFO));
        *(m3dV *)hits[i].point = m3dVZero;
    }
    aim = *gscCursor.aimInfo;
    CorrectAim(&aim, at);
    if (!wpnWEAPON::Shoot(owner, at))
        return 0;
    pwpMCG_SHOOT_V12 m;
    m.packId = 0;
    m.tag = 0x40;
    m.wpn = 0xFF;
    m.size = 5;
    m.nRays = 0;
    for (int i = 0; i < 20; i++)
        pwpRayInit(&m.ray[i]);
    m.size = 0x1AA;
    m.packId = *((u8 *)&pwpBulPack + 4);
    entCDT_REFINE_EXCL r(1, plrPlayer->inst, 1);
    m3dV org, dir;
    GetOrgDirTip(&org, &dir);
    if (!(aim.flags & 2))
        *(m3dV *)aim.point = *at;
    int n = FillCDTShootList(&org, &aim, hits, 10);
    if ((unsigned)n > 10)
        n = 10;
    m.wpn = (u8)wpnIndex;
    int k = 0;
    pwpMCG_RAY *rec = m.ray;
    for (int i = 0; i < n; i++) {
        if (!PrepareShootMsg(rec, &hits[i]))
            continue;
        rec->cls &= ~4;
        k++;
        pwpMCG_RAY *cur = rec++;
        if (cur->cls & 0xF0)
            continue;
        m3dV from, d;                       // water: a second ray past it
        if (i > 0) {
            from = org;
            d.x = hits[i].point[0] - org.x;
            d.y = hits[i].point[1] - org.y;
            d.z = hits[i].point[2] - org.z;
        } else {
            gsCameraPtr->GetViewWindowRayDir(gscCursor.x, gscCursor.y, &d);
            gsCameraPtr->TransformPointC2W(&m3dVZero, &from);
        }
        m3dNormalize(&d);
        if (gsScenePtr->ShootRay(&from, &d, 200.0f, 0, &r, &aim) && PrepareShootMsg(rec, &aim)) {
            rec->cls |= 4;
            k++;
            rec++;
        }
    }
    m.nRays = (u8)k;
    gsSysClient->SendMsgServer(plrPlayer, 0x4C7, &m, 0x20000);
    if (!(entFlags & 4)) {
        SubtractAmmo(1.0f);
        msgSystem.SendMsg(plrPlayer, 0x4C8, &m, this);
    }
    return 1;
}

// v12 0x0040AC20 (shared with pwpSHOVEL)
void pwpBULLET_WPN::ProcessSHOOT(void *msg)
{
    if (entFlags & 4)
        gsSysServer->SendMsgClientAll(owner, 0x4C7, (msgDATA *)msg, 0x90000, 0xFFFFFFFF, owner->mpOwner);
    msgSystem.SendMsg(owner, 0x4C8, msg, this);
}

// v12 0x0040AC80  (feb ProcessSHOOT 0x0040A280)
// The server takes the round and damages every ray's target (the statistics count one hit per shot on an NPC or
// a player); every side shows the shot and plays its sound.
void pwpBULLET_WPN::ShootBody(void *msg)
{
    pwpMCG_SHOOT_V12 *m = (pwpMCG_SHOOT_V12 *)msg;
    dmgWEAPON d;
    animCREATE_DATA cd;                     // (constructed and unused in the game)
    (void)cd;
    if (entFlags & 4) {
        SubtractAmmo(1.0f);
        int counted = 0;
        for (int i = 0; i < m->nRays; i++) {
            pwpMCG_RAY *r = &m->ray[i];
            if (r->targetId == -1)
                continue;
            entENTITY *e = (entENTITY *)gsSysMP->FindMPID(r->targetId, 0);
            if (!e)
                continue;
            if (!counted && (e->classId == AI_SIG_NPC || e->classId == AI_SIG_PLAYER)) {
                wrSysStat->Notify_WpnHit(owner->mpId, wpnIndex);
                counted = 1;
            }
            MakeDamage(&d, &r->point);
            d.sourceId = owner->mpId;
            d.unk2A = (int)(intptr_t)e->inst;
            d.unk2E = r->objId;
            msgSystem.SendMsg(e, 0x4C6, &d, this);
        }
    }
    ShowShotSFX(msg);
    PlayShootSound();
}

// v12 0x0040ADC0 (1.2 only)
// The hit effects of every ray, the flare (turned at random) and the smoke restarted at the muzzle.
void pwpBULLET_WPN::ShowShotSFX(void *msg)
{
    pwpMCG_SHOOT_V12 *m = (pwpMCG_SHOOT_V12 *)msg;
    for (int i = 0; i < m->nRays; i++)
        ShowSFX(&m->ray[i]);
    m3dV org, dir;
    GetOrgDirTip(&org, &dir);
    if (flare) {
        flare->inst->SetPos(&org);
        flare->inst->RotateY(m3dRandMax(180.0f), 1);
        ((dynDYNAMIC *)flare)->StartAnim(0, 0.0f, 0);
    }
    if (muzzleSmoke) {
        muzzleSmoke->StopEmission();
        muzzleSmoke->StartEmission();
    }
}

// v12 0x0040AE70  (feb 0x0040A380, rewritten in 1.2)
// One ray: wall / thing -> scorch mark (lit like the spot, sized 0.065 - rand 0.025 by GetScorchmarkScale) and,
// for a first ray, the "sfx_mcg_hit" effect on the surface (turned at random) with the wall sound; flesh -> the
// flesh sound; water -> the water sound and a "sfx_water_splash".
void pwpBULLET_WPN::ShowSFX(pwpMCG_RAY *r)
{
    animCREATE_DATA cd;
    int c = r->cls >> 4;
    if (c == 2 || c == 0) {
        cd.matr._MakeLCS2WCS_VY(&r->point, &m3dVUnitY);
        if (!(r->cls & 0xF0)) {
            gsSysSound->Play(&pwpBulSnd[4], &r->point);
            entCreate(gsScenePtr, (char *)"sfx_water_splash", (char *)"sfx_water_splash", &cd, 0, 0);
            return;
        }
        if (r->cls & 2)
            gsSysSound->Play(&pwpBulSnd[3], &r->point);
        return;
    }
    if (c != 1)
        return;
    cdtINFO ci;
    ci.flags = 2;
    ci.inst = 0;
    ci.objId = r->objId;
    ci.faceId = r->faceId;
    *(m3dV *)ci.point = r->point;
    ci.dist = 0.0f;
    if (r->targetId != -1) {
        entENTITY *e = (entENTITY *)gsSysMP->FindMPID(r->targetId, 0);
        if (e) {
            ci.inst = e->inst;
            ci.flags |= 0x100;
        }
    }
    u32 color = gsEffEnvirColor(&ci, 0, 0);
    m3dV n;
    gsScenePtr->GetCDTFace(&ci, 0, &n);
    float s = 0.065f - m3dRandMax(0.025f);
    float rad = GetRadiusNoSdrAround();
    wrsfxSCORCHMARK::Add(GetScorchmarkScale() * s, 10.0f, _pwpBulTexScorchMark, &r->point, color, 'PWPB', rad, 0.0f);
    if (r->cls & 4)
        return;
    cd.matr._MakeLCS2WCS_VY(&r->point, &n);
    m3dMATR rot;
    rot.MakeRotY(m3dRandMax(360.0f));
    cd.matr.Transform(&rot, 1);
    if (entCreate(gsScenePtr, (char *)"sfx_mcg_hit", (char *)"sfx_mcg_hit", &cd, &flareScale, 0) && (r->cls & 2))
        gsSysSound->Play(&pwpBulSnd[2], &r->point);
}

// v12 0x0040B080  (feb 0x0040A580)
void pwpBULLET_WPN::PlayShootSound()
{
    ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpBulSnd[0], 0);
}

// v12 0x00403860 (the shared empty 2-argument function)
void pwpBULLET_WPN::CorrectAim(cdtINFO *, m3dV *)
{
}

// v12 0x0040B0A0  (feb 0x0040A5A0)  slot +0xEC
// n shells out of "shell_source" along its Z axis with the weapon's speed, tinted like the weapon. They bounce on
// one plane: the owner's floor (its floor contact, flag 2), else the floor object under the source
// (wrsfxFindFloor), else far below; 1.2 then moves the plane to the floor 5 m under the point where the shell
// would fall through it (PredictCDT + scnSCENE::FloorPlace), or 2e6 down when there is none.
void pwpBULLET_WPN::StartShells(int n)
{
    cdtINFO fl;
    memset(&fl, 0, sizeof(fl));
    m3dBOX box;
    if (!shells)
        return;
    objOBJ *src = objFindName(inst->root, (char *)"shell_source");
    if (!src)
        return;
    m3dV org, axis;
    src->GetOrigin(&org);
    src->GetAxisZ(&axis);
    m3dV pl[2];                                         // m3dPLANE: a point, the normal
    m3dV *pp = &pl[0], *pn = &pl[1];
    if (owner->GetFloorInfo(&fl) && (fl.flags & 2)) {
        pp->x = fl.point[0];
        pp->y = fl.point[1];
        pp->z = fl.point[2];
    } else if (wrsfxFindFloor(&org, &box)) {
        *pp = box.vMin;
    } else {
        *pp = m3dVZero;
        pp->y = -1000.0f;
    }
    *pn = m3dVUnitY;
    shells->SetColor(colDiffuse, 0, 0);
    shells->SetCDTPlane(1, 0, (m3dPLANE *)pl);
    shells->SetPosDirSpeed(&org, &axis, (m3dV *)velocity);
    m3dV hit;
    if (shells->PredictCDT(&hit, 0)) {
        cdtINFO ci;
        memset(&ci, 0, sizeof(ci));
        cdtREFINE ref;
        ref.mode = 0xA;
        ref.queryFlags = 0;
        ref.exclude = 0;
        if (gsScenePtr->FloorPlace(&hit, 5.0f, &hit, 0.5f, 0, 0, &ref, &ci))
            *pp = hit;
        else
            pp->y = -2000000.0f;
        shells->SetCDTPlane(1, 0, (m3dPLANE *)pl);
    }
    shells->StartEmission();
    shells->Emit(n, 0);
    shells->StopEmission();
}

// v12 0x0040B340  (feb 0x0040A740)
// One ray: the crosshair's own hit.
int pwpBULLET_WPN::FillCDTShootList(m3dV *, cdtINFO *aim, cdtINFO *hits, int)
{
    hits[0] = *aim;
    return 1;
}

// v12 0x00431E30 (the shared "return 1.0")
float pwpBULLET_WPN::GetScorchmarkScale()
{
    return 1.0f;
}

// v12 0x0040C580  (feb 0x0040B680)
float pwpBULLET_WPN::GetRadiusNoSdrAround()
{
    return 0.085f;
}

// v12 0x0040B360  (feb ?)
// The aim help of the spread guns: living NPCs near the line of the shot (3 round it, len long) whose "target" (else 1
// over their position) is within twice angle of it and in sight get a ray each, at most max.
int pwpBULLET_WPN::FillCDTShootListSpread(m3dV *org, m3dV *dir, float len, float angle, cdtINFO *hits, int max)
{
    plrCDT_REFINE_NPC ref(plrPlayer->inst);
    m3dV end = { dir->x * len + org->x, dir->y * len + org->y, dir->z * len + org->z };
    cdtREFINE vis(2, 0);
    scnAOBJ list[20];
    int n = gsScenePtr->MakeAnimList(org, &end, 3.0f, &ref, list, 20);
    int k = 0;
    for (int i = 0; i < n && k < max; i++) {
        animINST *a = list[i].inst;
        if (a->ent->classId != AI_SIG_NPC)
            continue;
        m3dV p;
        if (objOBJ *t = objFindName(a->root, (char *)"target")) {
            objValidateLTM(t);
            t->GetOrigin(&p);
        } else {
            a->GetPos(&p);
            p.y += 1.0f;
        }
        m3dV d = { p.x - org->x, p.y - org->y, p.z - org->z };
        if (!(m3dAngleVector(&d, dir) < angle + angle))
            continue;
        if (!gsScenePtr->IsVisiblePoint(org, &p, 0, &vis, 0))
            continue;
        cdtINFO *h = &hits[k++];
        h->inst = a;
        h->objId = 0;
        h->faceId = 0;
        h->dist = m3dSqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        h->flags = (h->flags & ~0x100u) | 2;
    }
    return k;
}
