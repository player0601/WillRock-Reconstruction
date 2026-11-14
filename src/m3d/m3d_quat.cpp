// m3d_quat.cpp -- Will Rock 1.2 (v12.exe): quaternions of the m3d library ({w,x,y,z}, angles in degrees)
//
// Reconstructed from the Feb 2003 build (m3d:m3d_quat.obj) and checked against v12.
// v12 range 0x0057F720 .. 0x0057FF3F (feb 0x00566E00 .. 0x00567640).
// 1.2: m3dQUAT::Identity has no out-of-line copy any more (always inlined); nothing else changed.
// Users: animation (m3dSPL_QUAT keys, bone blending via m3dSlerpQuat), camera paths.

#include "m3d_geom.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0057FE80 (feb 0x00567590) -> v12 0x009AED9C


// v12 0x0057F720  (feb 0x00566E00)
// Component-wise comparison with tolerance eps (q and -q are NOT treated as equal).
BOOL32 __fastcall m3dIsEqualQuat_A(m3dQUAT *a, m3dQUAT *b, float eps)
{
    if (m3dFAbs(a->x - b->x) < eps &&
        m3dFAbs(a->y - b->y) < eps &&
        m3dFAbs(a->z - b->z) < eps)
        return m3dFAbs(b->w - a->w) < eps;
    return 0;
}


// v12 0x0057F830  (feb 0x00566F10)
// Spherical linear interpolation a -> b at t (classic Watt & Watt code). Flips b in place when
// -b is closer to a. Near-opposite quaternions use a perpendicular one (only x,y,z blended there).
void __fastcall m3dSlerpQuat(m3dQUAT *a, m3dQUAT *b, float t, m3dQUAT *out)
{
    float sw = a->w + b->w, sx = a->x + b->x, sy = a->y + b->y, sz = a->z + b->z;
    float dw = a->w - b->w, dx = a->x - b->x, dy = a->y - b->y, dz = a->z - b->z;
    if (sw * sw + sx * sx + sy * sy + sz * sz < dw * dw + dx * dx + dy * dy + dz * dz) {
        b->w *= -1.0f;
        b->x *= -1.0f;
        b->y *= -1.0f;
        b->z *= -1.0f;
    }
    float p[4] = { a->x, a->y, a->z, a->w };            // x,y,z,w order as in the original
    float q[4] = { b->x, b->y, b->z, b->w };
    float r[4];
    float cosom = b->w * p[3] + b->x * p[0] + b->y * p[1] + b->z * p[2];
    if (cosom + 1.0f <= M3D_EPSILON) {
        // ~opposite: rotate 90 degrees through a perpendicular quaternion
        r[0] = -p[1];
        r[1] =  p[0];
        r[2] = -p[3];
        r[3] =  p[2];
        float sclp = m3dSin((1.0f - t) * M3D_PI * 0.5f);
        float sclq = m3dSin(M3D_PI * t * 0.5f);
        for (int i = 0; i < 3; i++)
            r[i] = sclq * r[i] + sclp * p[i];
    } else {
        float sclp, sclq;
        if (1.0f - cosom <= M3D_EPSILON) {              // very close: plain lerp
            sclp = 1.0f - t;
            sclq = t;
        } else {
            float omega = m3dACos(cosom);
            float sinom = m3dSin(omega);
            sclp = m3dSin((1.0f - t) * omega) * (1.0f / sinom);
            sclq = m3dSin(omega * t) * (1.0f / sinom);
        }
        for (int i = 0; i < 4; i++)
            r[i] = sclp * p[i] + sclq * q[i];
    }
    out->x = r[0];
    out->w = r[3];
    out->y = r[1];
    out->z = r[2];
}


// v12 none -- always inlined in 1.2  (feb 0x00567160)
// Identity rotation.
void m3dQUAT::Identity()
{
    w = 1.0f;
    x = 0.0f;
    y = 0.0f;
    z = 0.0f;
}


// v12 0x0057FA80  (feb 0x00567180)
// Conjugate (inverse of a unit quaternion).
void m3dQUAT::Invert()
{
    x *= -1.0f;
    y *= -1.0f;
    z *= -1.0f;
}


// v12 0x0057FAB0  (feb 0x005671B0)
// Rotation of 'deg' degrees about the unit axis.
void m3dQUAT::SetAngAxis(float deg, m3dV *axis)
{
    float h = deg * 0.5f * M3D_PI * M3D_DEG2RAD;
    float s = m3dSin(h);
    w = m3dCos(h);
    x = s * axis->x;
    y = s * axis->y;
    z = s * axis->z;
}


// v12 0x0057FAF0  (feb 0x005671F0)
// Rotation matrix of the quaternion (need not be unit length); origin = 0.
void m3dQUAT::GetMatr(m3dMATR *out)
{
    float s  = 2.0f / (w * w + x * x + y * y + z * z);
    float ys = s * y, zs = s * z;
    float wx = s * x * w, xx = s * x * x;
    out->m[0][0] = 1.0f - (zs * z + ys * y);
    out->m[0][1] = ys * x + zs * w;
    out->m[0][2] = zs * x - ys * w;
    out->m[1][0] = ys * x - zs * w;
    out->m[1][1] = 1.0f - (zs * z + xx);
    out->m[1][2] = wx + zs * y;
    out->m[2][0] = zs * x + ys * w;
    out->m[2][1] = zs * y - wx;
    out->m[2][2] = 1.0f - (ys * y + xx);
    out->m[3][3] = 1.0f;
    out->m[0][3] = 0.0f;
    out->m[1][3] = 0.0f;
    out->m[2][3] = 0.0f;
    out->m[3][0] = 0.0f;
    out->m[3][1] = 0.0f;
    out->m[3][2] = 0.0f;
}


// v12 0x0057FC20  (feb 0x00567320)
// Quaternion from a pure rotation matrix (Shoemake: trace path, or largest diagonal element).
// 1.2: same logic (inlined m3dSqrt rescheduled).
void m3dQUAT::SetRotMatr(m3dMATR *m)
{
    static const int nxt[3] = { 1, 2, 0 };             // a local int[3] in the original
    float q[4];                                        // x,y,z,w
    float tr = m->m[2][2] + m->m[1][1] + m->m[0][0] + 1.0f;
    if (tr <= M3D_EPSILON) {
        int i = (m->m[0][0] < m->m[1][1]) ? 1 : 0;
        if (m->m[i][i] < m->m[2][2])
            i = 2;
        int j = nxt[i], k = nxt[j];
        float s = m3dSqrt((m->m[i][i] - (m->m[k][k] + m->m[j][j])) + 1.0f);
        q[i] = s * 0.5f;
        s = 0.5f / s;
        q[3] = (m->m[j][k] - m->m[k][j]) * s;
        q[j] = (m->m[j][i] + m->m[i][j]) * s;
        q[k] = (m->m[k][i] + m->m[i][k]) * s;
    } else {
        float s = m3dSqrt(tr);
        q[3] = s * 0.5f;
        s = 0.5f / s;
        q[0] = (m->m[1][2] - m->m[2][1]) * s;
        q[1] = (m->m[2][0] - m->m[0][2]) * s;
        q[2] = (m->m[0][1] - m->m[1][0]) * s;
    }
    z = q[2];
    w = q[3];
    x = q[0];
    y = q[1];
}


// v12 0x0057FEA0  (feb 0x005675B0)
// Quaternion from any matrix: removes the scale (inverse scale * m), drops the origin, then SetRotMatr.
void m3dQUAT::SetMatr(m3dMATR *m)
{
    float sx, sy, sz;
    m3dMATR s, inv;
    m->GetScale(&sx, &sy, &sz);
    s.Identity();
    s.Scale(sx, sy, sz, M3D_OP_SET);
    s.Invert(&inv);
    m3dMultiplyMatr(&inv, m, &s);
    s.Translate(&m3dVZero, M3D_OP_SET);
    SetRotMatr(&s);
}
