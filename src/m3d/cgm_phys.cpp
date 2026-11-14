// cgm_phys.cpp -- Will Rock 1.2 (v12.exe): ballistic throw helpers (projectile launch velocities)
//
// Reconstructed from the Feb 2003 build (m3d:cgm_phys.obj); identical in 1.2 (only the inlined SSE
// m3dSqrt differs in register allocation).
// v12 range 0x00573790 .. 0x00573B2F (feb 0x0055B1C0 .. 0x0055B55F).
// Y is up; g is the vertical acceleration (negative for gravity).
// HOOK / TWEAK: AI grenade / lob throws (e.g. the thrown projectiles of NPCs) aim through these.

#include "m3d_lib.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00573B10 (feb 0x0055B540) -> v12 0x009AEAF0 (feb 0x009637A0)


// v12 0x00573790  (feb 0x0055B1C0)
// Launch velocity reaching 'to' with the given horizontal speed; returns the flight time.
float __fastcall cgmGetThrowSpeed_XZ(m3dV *from, m3dV *to, float speedXZ, float g, m3dV *vel)
{
    m3dV dir = { to->x - from->x, 0.0f, to->z - from->z };
    m3dNormalize(&dir);
    float dx = from->x - to->x, dz = from->z - to->z;
    float t = m3dSqrt(dx * dx + dz * dz) / speedXZ;
    float dy = to->y - from->y;
    vel->x = dir.x * speedXZ;
    vel->y = dir.y * speedXZ;
    float fall = t * t * g * 0.5f;
    vel->z = dir.z * speedXZ;
    vel->y = (dy - fall) / t;
    return t;
}


// v12 0x005738D0  (feb 0x0055B300)
// Launch velocity whose apex is h above the start (vy = sqrt(2|g|h)) and which lands on 'to';
// returns the flight time (later root), 0 and a zero horizontal speed if 'to' is out of reach.
float __fastcall cgmGetThrowSpeed_H(m3dV *from, m3dV *to, float h, float g, m3dV *vel)
{
    float ag = m3dFAbs(g);
    float vy = m3dSqrt(ag * h + ag * h);
    float t0, t1, t, speed;
    if (!m3dSolveQuadraticEq(g * 0.5f, vy, from->y - to->y, &t0, &t1)) {
        speed = 0.0f;
        t = 0.0f;
    } else {
        t = (t0 <= t1) ? t1 : t0;
        float dx = to->x - from->x, dz = to->z - from->z;
        speed = m3dSqrt(dx * dx + dz * dz) / t;
    }
    m3dV dir = { to->x - from->x, 0.0f, to->z - from->z };
    m3dNormalize(&dir);
    vel->x = dir.x * speed;
    vel->y = dir.y * speed;
    vel->z = speed * dir.z;
    vel->y = vy;
    return t;
}
