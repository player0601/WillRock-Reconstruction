// plw_stg.cpp -- pwpSTG, the shotgun (weapon 2): 5 shells a clip, a glow of sparkles at its "spark_source" on each
// shot (Will Rock 1.2, v12.exe; feb obj plw_stg.obj)
//
// Rebuilt from the 1.2 code (v12 0x00414340..0x0041516A, vtable 0x005FABB0): the class, its INIT / TERM / FRAME /
// sound slots and the two sparkle emitters. Not rebuilt: the shot's own parts (PlayShootSound +0xF0 v12 0x00414D40,
// FillCDTShootList +0xF8 v12 0x004147A0, GetRadiusNoSdrAround +0x100 v12 0x00414DA0) -- pwpBULLET_WPN's run until then.

#include "game.h"
#include "../m3d/m3d_spl.h"
#include "../wr_vcall.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30

sncSOUND_DESCR_3D pwpStgSnd[7];                         // v12 0x00641408 shot + recharge (3), last shot, recharge,
                                                        //   rotate, shell
txmTEXTURE *pwpSTG::pTexGlow;                           // v12 0x00641208 "sfx_egg_glow"

// v12 0x00414340  static initialiser (the sncSOUND_DESCR_3D constructor inline)
static struct pwpSTG_SND_INIT {
    pwpSTG_SND_INIT()
    {
        static const char *names[7] = { "wpn_shotgun_shoot_recharge", "wpn_shotgun_shoot_recharge_1",
                                        "wpn_shotgun_shoot_recharge_2", "wpn_shotgun_shoot_last",
                                        "wpn_shotgun_recharge", "wpn_shotgun_rotate", "wpn_shotgun_drop_shell" };
        for (int i = 0; i < 7; i++) {
            sncSOUND_DESCR_3D *d = &pwpStgSnd[i];
            d->name.SetName((char *)names[i]);
            d->flags = 0;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = 0.0f;
            d->maxDist = 0.0f;
            d->vtbl = 0;                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} pwpStgSndInit;

// the m3dSPL_LINEAR1D constructor (inline, or m3dSPL::ctor(0, 8, 1) v12 0x00403670)
static m3dSPL_LINEAR1D *stgSpl(int n)
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

// v12 0x004144A0  (feb 0x00412B60)
int __fastcall pwpSTGInit()
{
    if (!entRegisterClass((char *)"pwp_stg", 'PSTG', 0x50, pwpSTG::Create, 0, 1))
        return 0;
    if (!(pwpSTG::pTexGlow = txmManager->Add((char *)"sfx_egg_glow", 0x40003, 1)))
        return 0;
    return sncLinkSndListData(7, pwpStgSnd) != 0;
}

// v12 0x00414500  (feb ?)
// Event 1: the recharge (FORCE_RECHARGE's action frame 0); event 2: the rotation (IDLE_1's action frame 0).
void pwpSTG::Anim2Sound()
{
    if (inst->IsActionFrame(9, 0)) {
        unk108 |= 1;
        return;
    }
    if (inst->IsActionFrame(2, 0))
        unk108 |= 2;
}

// v12 0x00414550  (feb ?)
void pwpSTG::UpdateSound()
{
    if (unk108 & 1) {
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpStgSnd[4], 0);
    } else if (unk108 & 2) {
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpStgSnd[5], 0);
    }
    pwpWEAPON::UpdateSound();
}

// v12 0x004145A0  (feb 0x00412C60)
// 5 a clip (action frames: recharge 0, shot 1 = SHOOT's first frame), 1.4 shots a second, damage 4 (a pellet),
// 200 at most; the flare, the smoke, shells (1.0, 0.005); hit effects at 0.25; the two sparkle emitters and the
// "spark_source" they sit on.
int pwpSTG::ProcessINIT()
{
    if (!pwpBULLET_WPN::ProcessINIT())
        return 0;
    clipSize = 5;
    clip = 5;
    reloadActFrame = 0;
    shootActFrame = 1;
    animSEQ *sh = &inst->tpl->seq[7];
    sh->actionFrame[1] = (int)sh->frameStart;
    flags2 |= 3;
    SetShootRate(1.4f);
    damage = 4.0f;
    maxAmmo = 200.0f;
    CreateFlare(1.0f);
    CreateSmoke(1.0f);
    CreateShell(1.0f, 0.005f, 0);
    flareScale = 0.25f;
    sparkle[0] = new pwpSTG_SPARKLE_1;
    if (!sparkle[0])
        return 0;
    sparkle[1] = new pwpSTG_SPARKLE_2;
    if (!sparkle[1])
        return 0;
    sparkSource = objFindName(inst->root, (char *)"spark_source");
    return sparkSource != 0;
}

// v12 0x004146E0  (feb ?)
// The sparkles follow the spark source; on a shot (SHOOT's action frames) they start again and the shells fly.
void pwpSTG::ProcessFRAME()
{
    pwpBULLET_WPN::ProcessFRAME();
    if (inst->flags & 1)
        return;
    inst->Validate(4);
    for (int i = 0; i < 2; i++)
        sparkle[i]->SetCS((m3dMATR *)sparkSource->ltm);
    if (inst->seq != 7)
        return;
    if (inst->actionFrames & 2)
        for (int i = 0; i < 2; i++) {
            sparkle[i]->StopEmission();
            sparkle[i]->StartEmission();
        }
    if (inst->actionFrames & 4)
        StartShells(1);
}

// v12 0x00414770  (feb ?)
void pwpSTG::ProcessTERM()
{
    pwpBULLET_WPN::ProcessTERM();
    for (int i = 0; i < 2; i++)
        if (sparkle[i])
            wrVCall<void *>(sparkle[i], 0, 1u);         // its deleting dtor (out of the emitter list)
}

// v12 0x00414D90
sncSOUND_DESCR_3D *pwpSTG::GetShellSoundDesc()
{
    return &pwpStgSnd[6];
}

// v12 0x00415170  (feb 0x00413830)
entENTITY *__fastcall pwpSTG::Create(animINST *)
{
    pwpSTG *w = new pwpSTG;                 // operator new(0x1EB), pwpBULLET_WPN ctor
    if (!w)
        return 0;
    w->wpnIndex = PWP_STG;
    return w;
}

// v12 0x00414DB0  (feb 0x00413470)
// Big glow sparkles: up to 100 a second for the first quarter of a 0.7 s burst, 0.15 s (+0.05) each, 0.036 big,
// turning, fading from half their life, at 0.25 in any direction, in the spark source's space.
pwpSTG_SPARKLE_1::pwpSTG_SPARKLE_1() : partEMITTER_PHYS(30.0f, 0.2f)
{
    SetTimeLifePart(0.15f, 0.05f);
    m3dSPL_LINEAR1D *in = stgSpl(2);
    in->SetKp(0, 0.0f, 100.0f);
    in->SetKp(1, 0.25f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(0.7f, 0.0f);
    SetSize(0.036f, 0.0f, 0, 1.0f);
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 0.0f);
    flags |= 4;
    m3dSPL_LINEAR1D *a = stgSpl(2);
    a->SetKp(0, 0.5f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    speedA = 0.25f;
    speedARand = 0.0f;
    speedASpl = 0;
    cone = 180.0f;
    coneSpl = 0;
    SetTexture(pwpSTG::pTexGlow);
    flags |= 0x400;
    flags |= 8;
    flags |= 0x800000;
}

// v12 0x00414F80  (feb 0x00413640)
// Small ones: the same burst, growing from 0 to 0.108 and back over their life (0.042 random), slower (0.1),
// stretched (texture scale 1, 0.15 +0.05).
pwpSTG_SPARKLE_2::pwpSTG_SPARKLE_2() : partEMITTER_PHYS(30.0f, 0.2f)
{
    SetTimeLifePart(0.15f, 0.05f);
    m3dSPL_LINEAR1D *in = stgSpl(2);
    in->SetKp(0, 0.0f, 100.0f);
    in->SetKp(1, 0.25f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(0.7f, 0.0f);
    m3dSPL_LINEAR1D *sz = stgSpl(2);
    sz->SetKp(0, 0.0f, 0.108333334f);
    sz->SetKp(1, 1.0f, 0.0f);
    SetSize(0.0f, 0.041666664f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = stgSpl(2);
    a->SetKp(0, 0.5f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    speedA = 0.1f;
    speedARand = 0.0f;
    speedASpl = 0;
    cone = 180.0f;
    coneSpl = 0;
    SetTexture(pwpSTG::pTexGlow);
    flags |= 0x400;
    flags |= 8;
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 0.0f);
    flags |= 4;
    SetTextureScale(1.0f, 0.0f, 0.15f, 0.05f);
    flags |= 0x800000;
}

// The shotgun's pellets as the last shot laid them out (ShowSFX's tracers)
m3dV  pwpStgRayDir[10];                                 // v12 0x00641290
float pwpStgRayLen[10];                                 // v12 0x00641210
m3dV  pwpStgRayOrg;                                     // v12 0x006413F8
int   pwpStgRayCount;                                   // v12 0x0064128C

// v12 0x004147A0  (feb ?)
// Ten pellets: the crosshair's hit, the aim help's NPCs, the rest at random in a spread growing with the distance --
// 0.15 within 5, 4 from 80 on -- the up-down spread 0.7 of the sideways one.
int pwpSTG::FillCDTShootList(m3dV *org, cdtINFO *aim, cdtINFO *hits, int max)
{
    entCDT_REFINE_EXCL ref(0, plrPlayer->inst, 0);
    hits[0] = *aim;
    float dx = org->x - aim->point[0], dy = org->y - aim->point[1], dz = org->z - aim->point[2];
    float d = m3dSqrt(dx * dx + dy * dy + dz * dz), s;
    if (d < 5.0f)
        s = 0.15f;
    else if (d > 80.0f)
        s = 4.0f;
    else
        s = (d - 5.0f) * 0.051333334f + 0.15f;
    m3dV dir = { aim->point[0] - org->x, aim->point[1] - org->y, aim->point[2] - org->z };
    float len = m3dSqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    m3dNormalize(&dir);
    m3dMATR m;
    m._MakeLCS2WCS_VZ(org, &dir);
    int n = FillCDTShootListSpread(org, &dir, len + 10.0f, atanf(s / len) * (180.0f / 3.1415927f), hits + 1, max - 1) + 1;
    if (plrPlayer->moveFlags & 0x800)
        ref.cls.bits |= 1;
    for (; n < 10; n++) {
        m3dV v = { m3dRandRange(-s, s), m3dRandRange(-s, s) * 0.7f, len }, w;
        float *mm = (float *)&m;
        w.x = v.x * mm[0] + v.y * mm[4] + v.z * mm[8];
        w.y = v.x * mm[1] + v.y * mm[5] + v.z * mm[9];
        w.z = v.x * mm[2] + v.y * mm[6] + v.z * mm[10];
        float k = 1.0f / m3dSqrt(w.x * w.x + w.y * w.y + w.z * w.z);
        w.x *= k;
        w.y *= k;
        w.z *= k;
        float l = len + 10.0f;
        if (!gsScenePtr->ShootRay(org, &w, l, 0, &ref, &hits[n]))
            hits[n] = hits[0];
        pwpStgRayDir[n] = w;
        pwpStgRayLen[n] = l;
    }
    pwpStgRayOrg = *org;
    pwpStgRayCount = n;
    return n;
}

// v12 0x00414D40
// A shot's sound: one of three, or with the clip empty the last one's.
void pwpSTG::PlayShootSound()
{
    if (!clip) {
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpStgSnd[3], 0);
        return;
    }
    ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpStgSnd[m3dRandIntRange(0, 2)], 0);
}

// v12 0x00414DA0
float pwpSTG::GetRadiusNoSdrAround()
{
    return 0.04f;
}
