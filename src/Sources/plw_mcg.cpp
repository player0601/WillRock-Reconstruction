// plw_mcg.cpp -- pwpMACHINEGUN, the Tommy gun (weapon 3): no clip, 10 shots a second (Will Rock 1.2, v12.exe;
// feb obj plw_mcg.obj)
//
// Rebuilt from the 1.2 code (v12 0x0040FC30..0x00410376, vtable 0x005FA5E0): the class, its INIT / FRAME and the
// target slot. Not rebuilt: the shot's own parts (ShowSFX +0xDC v12 0x0040FDD0, FillCDTShootList +0xF8 v12
// 0x0040FDF0) -- pwpBULLET_WPN's run until then.

#include "game.h"

// v12 0x0040FC30  (feb 0x0040E5D0)
int __fastcall pwpMCGInit()
{
    return entRegisterClass((char *)"pwp_mcg", 'PMCG', 0x50, pwpMACHINEGUN::Create, 0, 1) != 0;
}

// v12 0x0040FC60  (feb 0x0040E600)
// No clip (no reload frame), 10 shots a second, damage 6, 1000 at most, none at first; the flare (0.9), the smoke
// (0.8) and shells thrown at 0.0018..0.025 over 0.4 s; hit effects at 0.4.
int pwpMACHINEGUN::ProcessINIT()
{
    if (!pwpBULLET_WPN::ProcessINIT())
        return 0;
    clipSize = -1;
    clip = -1;
    reloadActFrame = 0;
    SetShootRate(10.0f);
    damage = 6.0f;
    maxAmmo = 1000.0f;
    SetAmmo(0.0f);
    target = 0;
    targetSince = 0;
    CreateFlare(0.9f);
    CreateSmoke(0.8f);
    m3dSPL_LINEAR1D *spl = new m3dSPL_LINEAR1D;
    if (spl) {
        spl->flags = 0;
        spl->type = 0;
        spl->nKp = 0;
        spl->kpValueSize = 8;
        spl->kpStride = 8;
        spl->dim = 1;
        spl->kpBytes = 0;
        spl->kp = 0;
        spl->pad[0] = spl->pad[1] = spl->pad[2] = 0;
        spl->AllocKpList(2);
    }
    spl->SetKp(0, 0.0f, 0.00184f);
    spl->SetKp(1, 0.4f, 0.025f);
    CreateShell(0.6f, 0.0f, spl);
    flareScale = 0.4f;
    return 1;
}

// v12 0x0040FD70  (feb 0x0040E710)
// The target is forgotten when its timer runs out (while the gun is out).
void pwpMACHINEGUN::ProcessFRAME()
{
    pwpBULLET_WPN::ProcessFRAME();
    if (inst->flags & 1)
        return;
    if (m3dUpdateTimeField(&targetTimer, gsElapsedTime)) {
        target = 0;
        targetSince = 0;
    }
}

// v12 0x0040FDB0  (feb 0x0040E750)  slot +0x94 (msg 3)
// An entity going away: no longer the target.
void pwpMACHINEGUN::ProcessDESTROY_ADDR(void *e)
{
    if (target == e)
        target = 0;
}

// v12 0x00410330  (feb 0x0040ECD0)
entENTITY *__fastcall pwpMACHINEGUN::Create(animINST *)
{
    pwpMACHINEGUN *w = new pwpMACHINEGUN;   // operator new(0x1EF), pwpBULLET_WPN ctor
    if (!w)
        return 0;
    w->target = 0;
    w->targetSince = 0;
    w->wpnIndex = PWP_MACHINEGUN;
    return w;
}

// v12 0x0040FDD0  (feb ?)
// A bullet's hit, and a shell out.
void pwpMACHINEGUN::ShowSFX(pwpMCG_RAY *r)
{
    pwpBULLET_WPN::ShowSFX(r);
    StartShells(1);
}

// v12 0x0040FDF0  (feb 0x0040E790)
// One ray: the aim help's NPC, else at random in a spread growing with the distance -- 0.1 within 3, 0.5 from 80 on,
// the up-down spread 0.7 of the sideways one; one that hits nothing goes where the crosshair points.
int pwpMACHINEGUN::FillCDTShootList(m3dV *org, cdtINFO *aim, cdtINFO *hits, int max)
{
    entCDT_REFINE_EXCL ref(0, plrPlayer->inst, 0);
    hits[0] = *aim;
    float dx = org->x - aim->point[0], dy = org->y - aim->point[1], dz = org->z - aim->point[2];
    float d = m3dSqrt(dx * dx + dy * dy + dz * dz), s;
    if (d < 3.0f)
        s = 0.1f;
    else if (d > 80.0f)
        s = 0.5f;
    else
        s = (d - 3.0f) * 0.005194805f + 0.1f;
    m3dV dir = { aim->point[0] - org->x, aim->point[1] - org->y, aim->point[2] - org->z };
    float len = m3dSqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    m3dNormalize(&dir);
    m3dMATR m;
    m._MakeLCS2WCS_VZ(org, &dir);
    int n = FillCDTShootListSpread(org, &dir, len + 10.0f, atanf(s / len) * (180.0f / 3.1415927f), hits, 1);
    if (plrPlayer->moveFlags & 0x800)
        ref.cls.bits |= 1;
    for (; n < 1; n++) {
        m3dV v = { m3dRandRange(-s, s), m3dRandRange(-s, s) * 0.7f, len }, w;
        float *mm = (float *)&m;
        w.x = v.x * mm[0] + v.y * mm[4] + v.z * mm[8];
        w.y = v.x * mm[1] + v.y * mm[5] + v.z * mm[9];
        w.z = v.x * mm[2] + v.y * mm[6] + v.z * mm[10];
        float k = 1.0f / m3dSqrt(w.x * w.x + w.y * w.y + w.z * w.z);
        w.x *= k;
        w.y *= k;
        w.z *= k;
        if (!gsScenePtr->ShootRay(org, &w, len + 10.0f, 0, &ref, &hits[n]))
            hits[n] = *aim;
    }
    return n;
}
