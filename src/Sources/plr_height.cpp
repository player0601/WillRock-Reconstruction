// plr_height.cpp -- the player's eye height: crouch blend and the dip after a landing
// (Will Rock 1.2, v12.exe; feb obj plr_height.obj)
//
// v12 0x0042D4E0..0x0042D8B7 (plrHEIGHT_CTRL), 0x0042D8C0..0x0042DE80 (plrCAM_CTRL: zoom, recoil, death view, the camera
// each frame) and the inline m3dSPL_BEZIER2D::SetKpValue emitted here.
// The landing dip is a 3-key 2D Bezier spline evaluated over [0, 2] (integer parametrisation); its y is a
// factor on the eye height. Harder landings dip longer (0.05 s .. 0.55 s) and deeper (factor 0.6 .. 1.0).

#include "game.h"

// v12 0x0042D4E0  (feb 0x00428410)
plrHEIGHT_CTRL::plrHEIGHT_CTRL()
{
    t = -1.0f;
    duration = -1.0f;
    value = 1.6f;
    lowH = 1.0f;
    highH = 1.6f;
    crouchT = 0;
    crouchTime = 0.12f;
    m3dSPL_BEZIER2D *s = (m3dSPL_BEZIER2D *)operator new(sizeof(m3dSPL_BEZIER2D));
    if (s) {
        new (s) m3dSPL_BEZIER2D;                        // vtable v12 0x005FB8A0
        m3dSPL_Init(s, 4, 0x1C, 2);                     // M3D_SPL_BEZIER2D, key {t, p, in, out}, 2 values
        s->AllocKpList(3);
    }
    spline = s;
}

// v12 0x0042D570  (feb 0x004284A0)
plrHEIGHT_CTRL::~plrHEIGHT_CTRL()
{
    if (spline)
        spline->__vecDelDtor(1);
}

// v12 0x0042D580  (feb 0x004284B0)
// Start the landing dip for a landing at vertical speed vy (only when no dip is playing).
void plrHEIGHT_CTRL::ApplySpeed(float vy)
{
    if (duration > 0)
        return;
    if (vy < -20.0f)
        duration = 0.55f;
    else if (vy > -3.0f)
        duration = 0.05f;
    else
        duration = 0.55f - (vy - -20.0f) * (1.0f / 34.0f);
    t = 0;
    m2dV p = { 0, 1.0f }, in = { 0, 1.0f }, out = { 0, 0 };
    if (vy < -40.0f)
        out.y = 0.6f;
    else if (vy > 0)
        out.y = 1.0f;
    else
        out.y = (vy - -40.0f) * 0.01f + 0.6f;
    spline->SetKp(0, 0.0f, &p, &in, &out);
    float w;                                            // depth of the dip: 0.5 (hard) .. 1.0
    if (vy < -40.0f)
        w = 0.5f;
    else if (vy > 0)
        w = 1.0f;
    else
        w = (vy - -40.0f) * 0.0125f + 0.5f;
    p.x = 1.0f;   p.y = w;
    in.x = 0.85f; in.y = w;
    out.x = 1.15f; out.y = w;
    spline->SetKp(1, duration * 0.5f, &p, &in, &out);
    p.x = 2.0f;   p.y = 1.0f;
    in.x = 1.9f;  in.y = 1.0f;
    out.x = 2.1f; out.y = 1.0f;
    spline->SetKp(2, duration, &p, &in, &out);
}

// v12 0x0042D770  (feb 0x004286A0)
float plrHEIGHT_CTRL::Update()
{
    if (plrPlayer->moveFlags & 0x200)                   // crouching
        crouchT += gsElapsedTime;
    else
        crouchT -= gsElapsedTime;
    float lo = 0, hi = crouchTime, c = crouchT;         // inline clamp(c, lo, hi)
    if (c < lo) { lo = c; c = 0; }
    if (hi < lo) c = lo;
    else if (!(hi > c)) c = hi;
    crouchT = c;
    float a = crouchTime, l = lowH, h = highH, z = 0;
    if (a < 0) { z = a; a = 0; float tmp = h; h = l; l = tmp; }   // (dead: crouchTime >= 0)
    value = (l - h) * (crouchT - z) / (a - z) + h;
    if (duration < 0)
        return value;
    t += gsElapsedTime;
    if (!(t < duration)) {
        duration = -1.0f;
        return value;
    }
    m2dV r;
    spline->GetValue(t / duration * 2.0f, &r, 0);
    return r.y * value;
}

// v12 0x0042D8B0  (feb 0x004287E0)
void plrHEIGHT_CTRL::Reset()
{
    duration = -1.0f;
}

// ---------------------------------------------------------------------------------------------- plrCAM_CTRL

// v12 0x0042D8C0  (feb 0x004287F0)
plrCAM_CTRL::plrCAM_CTRL()
{
    zoomState = 0;
    zoomFov = 0;
    savedFov = 0;
    zoomT = 0;
    attached = 0;
    kick = m3dVZero;
    kickT = 0;
    kickTime = 0;
    deathTime = 0;
    deathEnd = 0;
}

// v12 0x0042D900  (feb 0x00428830)
float plrCAM_CTRL::GetZoomStrength()
{
    return tanf(savedFov * 3.1415927f * (1.0f / 180.0f)) /
           tanf(gsCameraPtr->angleHor * 3.1415927f * (1.0f / 180.0f));
}

// v12 0x0042D940  (feb 0x00428870)
// on: start zooming in to 55 degrees narrower (from off only); off: zoom back out (when zoomed or zooming).
void plrCAM_CTRL::ZoomNotify(int on)
{
    if (!on) {
        if (zoomState)
            zoomState = 2;
        return;
    }
    if (zoomState) {
        gsCameraPtr->SetAngleHor(savedFov);
        zoomState = 0;
        return;
    }
    zoomState = 1;
    savedFov = gsCameraPtr->angleHor;
    zoomT = 0;
    zoomFov = savedFov - 55.0f;
}

// v12 0x0042D9A0  (feb 0x004288D0)
void plrCAM_CTRL::ZoomClear()
{
    if (zoomState) {
        gsCameraPtr->SetAngleHor(savedFov);
        zoomState = 0;
    }
}

// v12 0x0042D9C0  (feb 0x004288F0)
// The recoil kick of a shot of weapon type wpn (only when the last one is over): colt, shotgun, rocket launcher,
// atomic gun. The others (the shovel among them) kick nothing.
void plrCAM_CTRL::ShootNotify(int wpn)
{
    if (kickTime > 1e-6f)
        return;
    kickT = 0;
    switch (wpn) {
    case 1:
        kickTime = 0.07f;
        kick.x = m3dRandRange(-0.03f, 0.03f);
        kick.y = m3dRandMax(0.04f);
        kick.z = -m3dRandMax(0.07f);
        break;
    case 2:
        kickTime = 0.35f;
        kick.y = 0.03f;
        kick.z = -0.2f;
        kick.x = 0;
        break;
    case 6:
        kickTime = 0.5f;
        kick.z = -0.2f;
        kick.y = 0.1f;
        kick.x = 0;
        break;
    case 10:
        kickTime = 0;
        kick.z = -0.5f;
        kick.y = 0.1f;
        kick.x = 0;
        break;
    }
}

// v12 0x0042DAB0  (feb 0x004289E0)
void plrCAM_CTRL::DeathNotify(int quiet)
{
    deathEnd = 1.0f;
    deathTime = quiet ? 0.0f : -10.0f;
    gsCameraPtr->GetOrigin(&deathOrigin);
    gsCameraPtr->GetVForw(&deathLookAt);
}

// v12 0x0042DB00  (feb 0x00428A30)
void plrCAM_CTRL::RevitalizeNotify()
{
    deathEnd = 0;
}

// v12 0x0042DB10  (feb 0x00428A40)
void plrCAM_CTRL::PowerupNotify(int on, int a, objOBJ *o)
{
    if (!on) {
        attached = 0;
        return;
    }
    attached = 1;
    attachObj = o;
}

// v12 0x0042DB40  (feb 0x00428A70)
// The camera after UpdateCamera placed it at the eyes: the death view (from beside and above the body, pushed out
// of the walls, turning to look at it), the view of an object (PowerupNotify), else the zoom glide and the recoil
// kick (out and back over kickTime).
void plrCAM_CTRL::Update()
{
    cdtREFINE ref(2, 0);
    if (deathEnd > 1e-6f) {
        deathTime += gsElapsedTime;
        if (deathTime >= deathEnd)
            deathTime = deathEnd;
        m3dV p, r, at;
        plrPlayer->inst->GetPos(&p);
        plrPlayer->inst->GetDirRight(&r);
        r.y = 0;
        m3dNormalize(&r);
        at.x = r.x * 1.7f + p.x;
        at.z = r.z * 1.7f + p.z;
        at.y = r.y * 1.7f + p.y + 2.0f;
        if (deathTime < 0 || deathEnd - 0.1f < deathTime)
            deathOrigin = at;
        m3dV to;
        m3dClampVector_L(&deathOrigin, &at, gsElapsedTime * 5.0f, &to);
        gsScenePtr->ConstrSphere(&deathOrigin, &to, 0.55f, 0x10, &ref, 0);
        gsCameraPtr->MoveTo(&to);
        deathOrigin = to;
        m3dV pp, look, dir;
        wrEVCall<void>(plrPlayer, 0x64, &pp);            // GetPos
        pp.y += 1.0f;
        look.x = pp.x - to.x;
        look.y = pp.y - to.y;
        look.z = pp.z - to.z;
        m3dClampVector_A(&deathLookAt, &look, gsElapsedTime * 150.0f, &dir, 0, 0);
        gsCameraPtr->SetLookAt(&dir);
        deathLookAt = dir;
        return;
    }
    if (attached) {
        m3dV o, z;
        attachObj->GetOrigin(&o);
        gsCameraPtr->MoveTo(&o);
        attachObj->GetAxisZ(&z);
        gsCameraPtr->SetLookAt(&z);
        return;
    }
    if (zoomState == 1) {
        zoomT += gsElapsedTime;
        float fov = zoomFov - zoomT * 45.0f;
        if (fov <= 10.5f) {
            fov = 10.5f;
            zoomState = 2;
        }
        gsCameraPtr->SetAngleHor(fov);
    }
    if (!(kickTime >= 1e-6f))
        return;
    float t = kickT + gsElapsedTime;
    kickT = t;
    if (t > kickTime) {
        kickTime = 0;
        return;
    }
    float half = kickTime * 0.5f;
    m3dV d;
    if (t < half)
        m3dLerpVector_L(&m3dVZero, &kick, 0, half, t, &d);
    else
        m3dLerpVector_L(&kick, &m3dVZero, half, kickTime, t, &d);
    gsCameraPtr->MoveRel(&d);
}

// v12 0x0042DEB0  (feb 0x00428DD0)  inline of m3d_spl.h, emitted here
// Key i's point = v; both control points = (0, 0).
void m3dSPL_BEZIER2D::SetKpValue(int i, void *v)
{
    float *k = (float *)((char *)kp + kpStride * (i < nKpMax ? i : nKpMax));
    k[1] = ((float *)v)[0];
    k[2] = ((float *)v)[1];
    k[5] = m2dVZero.x;
    k[6] = m2dVZero.y;
    float *k2 = (float *)((char *)kp + kpStride * (i < nKpMax ? i : nKpMax));
    k2[3] = k[5];
    k2[4] = k[6];
}
