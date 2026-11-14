// m3d_matr.cpp -- Will Rock 1.2 (v12.exe): 4x4 matrices (m3dMATR) of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_matr.obj) and checked against v12.
// v12 range 0x005609A0 .. 0x00564AD0 (feb 0x005496C0 .. 0x0054D5B0).
// 1.2: three NEW functions: m3dRotatePointAxis (v12 0x00560E80, name ?), m3dMATR::MakeScale
//      (v12 0x00561F80) and m3dMATR::MakeRotX (v12 0x00561FB0). Everything else is unchanged
//      (several functions differ only by the rescheduled inline m3dSqrt / point transform).
//
// Conventions: row vectors, p' = p*M; rows 0..2 = local X, Y, Z axes, row 3 = origin.
// "op": 0 = SET, 1 = LOCAL (M = T*M: apply in the matrix's own frame), 2 = GLOBAL (M = M*T).
// Angles are DEGREES. The game is x87-only through m3dSimdType == 2; the SSE/3DNow! branches of
// m3dMultiplyMatr / TransformPointList / the inlined transforms compute the same values.

#include "m3d_geom.h"
#include <string.h>

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00561150 (feb 0x00549CA0) -> v12 0x009AEA38

m3dMATR m3dMatrIdentity;            // v12 0x009AEA40 (feb 0x009636F8)
static m3dMATR m3dMatrRotZ;         // v12 0x009AE978 (feb 0x00963630) scratch of RotateZ (unnamed in the map)
static m3dMATR m3dMatrRotX;         // v12 0x009AE9B8 (feb 0x00963670) scratch of RotateX
static m3dMATR m3dMatrRotY;         // v12 0x009AE9F8 (feb 0x009636B0) scratch of RotateY

// m2dMATR (m2d_matr.obj, other batch): 3x3 affine 2D matrix
class m2dMATR {
public:
    float m[3][3];
    BOOL32 MakePolyPoly(m2dPOLY *src, m2dPOLY *dst);    // v12 ? (feb m2d_matr.obj)
};

static inline void m3dSetIdentity(m3dMATR *m)
{
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = m->m[3][3] = 1.0f;
}

// Up vector for a basis around 'v' (unit): Y, or X when v is (almost) vertical; made orthogonal to v.
static inline void m3dMakeUpFor(const m3dV *v, m3dV *up)
{
    m3dV u = m3dVUnitY;
    if (m3dFAbs(v->x) < M3D_EPSILON && m3dFAbs(v->z) < M3D_EPSILON)
        u = m3dVUnitX;
    float k = u.z * v->z + u.x * v->x + v->y * u.y;
    up->x = u.x - v->x * k;
    up->y = u.y - k * v->y;
    up->z = u.z - k * v->z;
    m3dNormalize(up);
}


// v12 0x005609A0  (feb 0x005496C0)
// Library init: the three rotation scratch matrices and m3dMatrIdentity start as identities.
BOOL32 __fastcall m3dInit3DMatr()
{
    m3dSetIdentity(&m3dMatrRotX);
    m3dSetIdentity(&m3dMatrRotY);
    m3dSetIdentity(&m3dMatrRotZ);
    m3dSetIdentity(&m3dMatrIdentity);
    return 1;
}


// v12 0x00560A20  (feb 0x00549740)
// out = a + b (element-wise).
void __fastcall m3dAddMatr(m3dMATR *a, m3dMATR *b, m3dMATR *out)
{
    const float *pa = &a->m[0][0], *pb = &b->m[0][0];
    float *po = &out->m[0][0];
    for (int i = 0; i < 16; i++)
        po[i] = pa[i] + pb[i];
}


// v12 0x00560A50  (feb 0x00549770)
// out = a * b. The x87 path assumes column 3 of a is (0,0,0,1) for rows 0..2 (affine matrices);
// the SSE / 3DNow! paths do the full product. out must not alias a or b.
void __cdecl m3dMultiplyMatr(m3dMATR *a, m3dMATR *b, m3dMATR *out)
{
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 4; c++)
            out->m[r][c] = b->m[1][c] * a->m[r][1] + b->m[2][c] * a->m[r][2] + b->m[0][c] * a->m[r][0];
    for (int c = 0; c < 4; c++)
        out->m[3][c] = b->m[2][c] * a->m[3][2] + b->m[1][c] * a->m[3][1] + b->m[0][c] * a->m[3][0]
                     + b->m[3][c] * a->m[3][3];
}


// v12 0x00560DF0  (feb 0x00549B10)
// Rows 0..2 equal within epsRot, row 3 (origin) within epsPos.
BOOL32 __fastcall m3dIsEqualMatr_A(m3dMATR *a, m3dMATR *b, float epsRot, float epsPos)
{
    for (int r = 0; r < 3; r++)
        for (int c = 0; c < 4; c++)
            if (epsRot < m3dFAbs(a->m[r][c] - b->m[r][c]))
                return 0;
    for (int c = 0; c < 4; c++)
        if (epsPos < m3dFAbs(a->m[3][c] - b->m[3][c]))
            return 0;
    return 1;
}


// v12 0x00560E80  (feb -- )
// 1.2: NEW. Rotates point p by 'deg' degrees about the axis through 'center' -> out
// (= MakeRotRay + point transform). Callers: objFILTER_PART_SOURCE::IsUse (v12 0x00467B59, 0x00467C87).
// Name unknown (not in the Feb map) // ?
void __fastcall m3dRotatePointAxis(m3dV *center, m3dV *axis, float deg, m3dV *p, m3dV *out)
{
    m3dQUAT q;
    m3dMATR m;
    q.SetAngAxis(deg, axis);
    q.GetMatr(&m);
    m.Translate(-center->x, -center->y, -center->z, M3D_OP_LOCAL);
    m.Translate(center->x, center->y, center->z, M3D_OP_GLOBAL);
    m3dTransformPoint(p, &m, out);
}


// v12 0x00561050  (feb 0x00549BA0)
// Identity.
void m3dMATR::Identity()
{
    m3dSetIdentity(this);
}


// v12 0x00561080  (feb 0x00549BD0)
// Translate by a vector (see the float version).
void m3dMATR::Translate(m3dV *v, int op)
{
    Translate(v->x, v->y, v->z, op);
}


// v12 0x005610A0  (feb 0x00549BF0)
// SET: origin = (x,y,z); LOCAL: origin += x*axisX + y*axisY + z*axisZ; GLOBAL: origin += (x,y,z).
void m3dMATR::Translate(float x, float y, float z, int op)
{
    if (op == M3D_OP_SET) {
        m[3][0] = x;
        m[3][1] = y;
        m[3][2] = z;
    } else if (op == M3D_OP_LOCAL) {
        m[3][0] = x * m[0][0] + z * m[2][0] + y * m[1][0] + m[3][0];
        m[3][1] = z * m[2][1] + y * m[1][1] + x * m[0][1] + m[3][1];
        m[3][2] = z * m[2][2] + y * m[1][2] + x * m[0][2] + m[3][2];
    } else if (op == M3D_OP_GLOBAL) {
        m[3][0] = x + m[3][0];
        m[3][1] = y + m[3][1];
        m[3][2] = z + m[3][2];
    }
}


// v12 0x00561170  (feb 0x00549CC0)
// Rotation about X by 'deg' (SET replaces the whole matrix).
void m3dMATR::RotateX(float deg, int op)
{
    float a = M3D_PI * deg * M3D_DEG2RAD;
    float c = m3dCos(a), s = m3dSin(a);
    m3dMatrRotX.m[1][1] = c;
    m3dMatrRotX.m[1][2] = s;
    m3dMatrRotX.m[2][1] = -s;
    m3dMatrRotX.m[2][2] = c;
    if (op == M3D_OP_SET) {
        *this = m3dMatrRotX;
        return;
    }
    m3dMATR t;
    if (op == M3D_OP_GLOBAL)
        m3dMultiplyMatr(this, &m3dMatrRotX, &t);
    else
        m3dMultiplyMatr(&m3dMatrRotX, this, &t);
    *this = t;
}


// v12 0x00561210  (feb 0x00549D60)
// Rotation about Y by 'deg'.
void m3dMATR::RotateY(float deg, int op)
{
    float a = M3D_PI * deg * M3D_DEG2RAD;
    float c = m3dCos(a), s = m3dSin(a);
    m3dMatrRotY.m[0][0] = c;
    m3dMatrRotY.m[0][2] = -s;
    m3dMatrRotY.m[2][0] = s;
    m3dMatrRotY.m[2][2] = c;
    if (op == M3D_OP_SET) {
        *this = m3dMatrRotY;
        return;
    }
    m3dMATR t;
    if (op == M3D_OP_GLOBAL)
        m3dMultiplyMatr(this, &m3dMatrRotY, &t);
    else
        m3dMultiplyMatr(&m3dMatrRotY, this, &t);
    *this = t;
}


// v12 0x005612B0  (feb 0x00549E00)
// Rotation about Z by 'deg'.
void m3dMATR::RotateZ(float deg, int op)
{
    float a = M3D_PI * deg * M3D_DEG2RAD;
    float c = m3dCos(a), s = m3dSin(a);
    m3dMatrRotZ.m[0][0] = c;
    m3dMatrRotZ.m[0][1] = s;
    m3dMatrRotZ.m[1][0] = -s;
    m3dMatrRotZ.m[1][1] = c;
    if (op == M3D_OP_SET) {
        *this = m3dMatrRotZ;
        return;
    }
    m3dMATR t;
    if (op == M3D_OP_GLOBAL)
        m3dMultiplyMatr(this, &m3dMatrRotZ, &t);
    else
        m3dMultiplyMatr(&m3dMatrRotZ, this, &t);
    *this = t;
}


// v12 0x00561350  (feb 0x00549EA0)
// LOCAL: scales the axis rows; GLOBAL: scales the columns (origin included);
// SET: removes the current scale (LOCAL 1/s) and then applies (x,y,z) GLOBALLY.
void m3dMATR::Scale(float x, float y, float z, int op)
{
    if (op == M3D_OP_SET) {
        float sx, sy, sz;
        GetScale(&sx, &sy, &sz);
        Scale(1.0f / sx, 1.0f / sy, 1.0f / sz, M3D_OP_LOCAL);
        op = M3D_OP_GLOBAL;
    }
    if (op == M3D_OP_LOCAL) {
        for (int c = 0; c < 3; c++) {
            m[0][c] = x * m[0][c];
            m[1][c] = y * m[1][c];
            m[2][c] = z * m[2][c];
        }
    } else if (op == M3D_OP_GLOBAL) {
        for (int r = 0; r < 4; r++) {
            m[r][0] = x * m[r][0];
            m[r][1] = y * m[r][1];
            m[r][2] = z * m[r][2];
        }
    }
}


// v12 0x00561420  (feb 0x00549F70)
// SET: this = t; LOCAL: this = t*this; GLOBAL: this = this*t.
void m3dMATR::Transform(m3dMATR *t, int op)
{
    if (op == M3D_OP_SET) {
        *this = *t;
        return;
    }
    m3dMATR r;
    if (op == M3D_OP_GLOBAL)
        m3dMultiplyMatr(this, t, &r);
    else
        m3dMultiplyMatr(t, this, &r);
    *this = r;
}


// v12 0x00561480  (feb 0x00549FD0)
void m3dMATR::GetOrigin(m3dV *o) { o->x = m[3][0]; o->y = m[3][1]; o->z = m[3][2]; }

// v12 0x005614A0  (feb 0x00549FF0)
void m3dMATR::GetAxisX(m3dV *o)  { o->x = m[0][0]; o->y = m[0][1]; o->z = m[0][2]; }

// v12 0x005614C0  (feb 0x0054A010)
void m3dMATR::GetAxisY(m3dV *o)  { o->x = m[1][0]; o->y = m[1][1]; o->z = m[1][2]; }

// v12 0x005614E0  (feb 0x0054A030)
void m3dMATR::GetAxisZ(m3dV *o)  { o->x = m[2][0]; o->y = m[2][1]; o->z = m[2][2]; }


// v12 0x00561500  (feb 0x0054A050)
// Origin as three floats.
void m3dMATR::GetTranslate(float *x, float *y, float *z)
{
    *x = m[3][0];
    *y = m[3][1];
    *z = m[3][2];
}


// v12 0x00561520  (feb 0x0054A070)
// Lengths of the three axis rows; all three negated when the basis is left-handed (mirror).
// (The original transforms m3dVUnitX/Y/Z as vectors to get the rows, twice.)
// 1.2: same logic (inlined m3dSqrt rescheduled).
void m3dMATR::GetScale(float *x, float *y, float *z)
{
    m3dV ax, ay, az;
    m3dTransformVector(&m3dVUnitX, this, &ax);
    *x = m3dLength(&ax);
    m3dTransformVector(&m3dVUnitY, this, &ay);
    *y = m3dLength(&ay);
    m3dTransformVector(&m3dVUnitZ, this, &az);
    *z = m3dLength(&az);
    m3dNormalize(&ax);
    m3dNormalize(&ay);
    m3dNormalize(&az);
    m3dV c;
    m3dCross(&ax, &ay, &c);
    if (m3dDot(&c, &az) <= 0.0f) {
        *x *= -1.0f;
        *y *= -1.0f;
        *z *= -1.0f;
    }
}


// v12 0x00561F80  (feb -- )
// 1.2: NEW. Pure scale matrix diag(x, y, z, 1). Callers e.g. v12 0x00501A93.
void m3dMATR::MakeScale(float x, float y, float z)
{
    memset(m, 0, sizeof(m));
    m[0][0] = x;
    m[1][1] = y;
    m[2][2] = z;
    m[3][3] = 1.0f;
}


// v12 0x00561FB0  (feb -- )
// 1.2: NEW. Pure rotation about X by 'deg' (twin of MakeRotY). Callers e.g. v12 0x00514215.
void m3dMATR::MakeRotX(float deg)
{
    memset(m, 0, sizeof(m));
    float a = M3D_PI * deg * M3D_DEG2RAD;
    float c = m3dCos(a), s = m3dSin(a);
    m[1][1] = c;
    m[1][2] = s;
    m[2][1] = -s;
    m[2][2] = c;
    m[0][0] = 1.0f;
    m[3][3] = 1.0f;
}


// v12 0x00562000  (feb 0x0054AAE0)
// Pure rotation about Y by 'deg'.
void m3dMATR::MakeRotY(float deg)
{
    memset(m, 0, sizeof(m));
    float a = M3D_PI * deg * M3D_DEG2RAD;
    float c = m3dCos(a), s = m3dSin(a);
    m[0][0] = c;
    m[0][2] = -s;
    m[2][0] = s;
    m[2][2] = c;
    m[1][1] = 1.0f;
    m[3][3] = 1.0f;
}


// v12 0x00562050  (feb 0x0054AB30)
// Shortest rotation taking direction 'from' to direction 'to' (identity when parallel).
// 1.2: same logic (inlined m3dSqrt rescheduled).
void m3dMATR::MakeRot(m3dV *from, m3dV *to)
{
    m3dV a = *from, b = *to, axis;
    m3dNormalize(&a);
    m3dNormalize(&b);
    m3dCross(&a, &b, &axis);
    if (m3dFAbs(m3dLength(&axis)) < M3D_EPSILON) {
        m3dSetIdentity(this);
        return;
    }
    m3dNormalize(&axis);
    m3dQUAT q;
    q.SetAngAxis(_m3dAngleVector(&a, &b), &axis);
    q.GetMatr(this);
}


// v12 0x00562230  (feb 0x0054AD10)
// Same rotation as MakeRot(from, to) but about the point 'center'.
// 1.2: same logic (inlined m3dSqrt rescheduled).
void m3dMATR::MakeRot(m3dV *center, m3dV *from, m3dV *to)
{
    m3dV a = *from, b = *to, axis;
    m3dNormalize(&a);
    m3dNormalize(&b);
    m3dCross(&a, &b, &axis);
    if (m3dFAbs(m3dLength(&axis)) < M3D_EPSILON) {
        m3dSetIdentity(this);
    } else {
        m3dNormalize(&axis);
        m3dQUAT q;
        q.SetAngAxis(_m3dAngleVector(&a, &b), &axis);
        q.GetMatr(this);
    }
    Translate(-center->x, -center->y, -center->z, M3D_OP_LOCAL);
    Translate(center->x, center->y, center->z, M3D_OP_GLOBAL);
}


// v12 0x00562450  (feb 0x0054AF30)
// Rotation of 'deg' degrees about the ray (org, unit dir).
void m3dMATR::MakeRotRay(m3dV *org, m3dV *dir, float deg)
{
    m3dQUAT q;
    q.SetAngAxis(deg, dir);
    q.GetMatr(this);
    Translate(-org->x, -org->y, -org->z, M3D_OP_LOCAL);
    Translate(org->x, org->y, org->z, M3D_OP_GLOBAL);
}


// v12 0x005624C0  (feb 0x0054AFA0)
// Local -> world matrix from a basis: rows x, y, z, origin org. One axis may be NULL (cross product
// of the other two: x = y*z, y = z*x, z = x*y).
void m3dMATR::MakeLCS2WCS(m3dV *org, m3dV *x, m3dV *y, m3dV *z)
{
    m3dV tmp;
    if (!x) { x = &tmp; m3dCross(y, z, &tmp); }
    if (!y) { y = &tmp; m3dCross(z, x, &tmp); }
    if (!z) { z = &tmp; m3dCross(x, y, &tmp); }
    m3dSetIdentity(this);
    m[0][0] = x->x; m[1][0] = y->x; m[2][0] = z->x;
    m[0][1] = x->y; m[1][1] = y->y; m[2][1] = z->y;
    m[0][2] = x->z; m[1][2] = y->z; m[2][2] = z->z;
    m[3][0] = org->x;
    m[3][1] = org->y;
    m[3][2] = org->z;
}


// v12 0x00562600  (feb 0x0054B0E0)
// World -> local matrix whose Z axis is 'vz' (Y kept as close to world up as possible).
void m3dMATR::_MakeWCS2LCS_VZ(m3dV *org, m3dV *vz)
{
    m3dV z = *vz, up;
    m3dNormalize(&z);
    m3dMakeUpFor(&z, &up);
    MakeWCS2LCS(org, 0, &up, &z);
}


// v12 0x00562730  (feb 0x0054B210)
// World -> local matrix of a basis (orthonormal): columns x, y, z, then origin -org in local axes.
// One axis may be NULL (cross product of the other two).
void m3dMATR::MakeWCS2LCS(m3dV *org, m3dV *x, m3dV *y, m3dV *z)
{
    m3dV tmp;
    if (!x) { x = &tmp; m3dCross(y, z, &tmp); }
    if (!y) { y = &tmp; m3dCross(z, x, &tmp); }
    if (!z) { z = &tmp; m3dCross(x, y, &tmp); }
    m[0][0] = x->x; m[0][1] = y->x; m[0][2] = z->x; m[0][3] = 0.0f;
    m[1][0] = x->y; m[1][1] = y->y; m[1][2] = z->y; m[1][3] = 0.0f;
    m[2][0] = x->z; m[2][1] = y->z; m[2][2] = z->z; m[2][3] = 0.0f;
    m[3][0] = 0.0f; m[3][1] = 0.0f; m[3][2] = 0.0f; m[3][3] = 1.0f;
    Translate(-org->x, -org->y, -org->z, M3D_OP_LOCAL);
}


// v12 0x00562870  (feb 0x0054B350)
// Local -> world matrix with Y = world up and Z = 'vz' flattened onto the ground (X when vz is
// vertical). Used for characters / spawn points that only yaw.
void m3dMATR::MakeLCS2WCS_VZ(m3dV *org, m3dV *vz)
{
    m3dV z = { vz->x, 0.0f, vz->z };
    if (!_m3dNormalize(&z))
        z = m3dVUnitX;
    MakeLCS2WCS(org, 0, &m3dVUnitY, &z);
}


// v12 0x005628F0  (feb 0x0054B3D0)
// Local -> world matrix whose X axis is 'vx' (Y as close to world up as possible).
void m3dMATR::_MakeLCS2WCS_VX(m3dV *org, m3dV *vx)
{
    m3dV x = *vx, up;
    m3dNormalize(&x);
    m3dMakeUpFor(&x, &up);
    MakeLCS2WCS(org, &x, &up, 0);
}


// v12 0x00562A20  (feb 0x0054B500)
// Local -> world matrix whose Z axis is 'vz' (Y as close to world up as possible).
void m3dMATR::_MakeLCS2WCS_VZ(m3dV *org, m3dV *vz)
{
    m3dV z = *vz, up;
    m3dNormalize(&z);
    m3dMakeUpFor(&z, &up);
    MakeLCS2WCS(org, 0, &up, &z);
}


// v12 0x00562B50  (feb 0x0054B630)
// Local -> world matrix whose Y axis is 'vy'; X = world up made orthogonal to vy (original choice).
void m3dMATR::_MakeLCS2WCS_VY(m3dV *org, m3dV *vy)
{
    m3dV y = *vy, x;
    m3dNormalize(&y);
    m3dMakeUpFor(&y, &x);
    MakeLCS2WCS(org, &x, &y, 0);
}


// v12 0x00562C60  (feb 0x0054B740)
// Re-orthonormalises the rotation part (Gram-Schmidt keeping X, then Z = X*Y, Y = Z*X). Origin kept.
void m3dMATR::Orthonormalize()
{
    m3dV x = { m[0][0], m[0][1], m[0][2] };
    m3dV y = { m[1][0], m[1][1], m[1][2] };
    m3dV o = { m[3][0], m[3][1], m[3][2] };
    m3dV z;
    m3dNormalize(&x);
    m3dCross(&x, &y, &z);
    m3dNormalize(&z);
    m3dCross(&z, &x, &y);
    m3dNormalize(&y);
    MakeLCS2WCS(&o, &x, &y, &z);
}


// v12 0x00562D80  (feb 0x0054B860)
// World -> "triangle space": origin a, X along a->b, Y = face normal, Z = X*N.
void m3dMATR::MakeProjTri(m3dV *a, m3dV *b, m3dV *c)
{
    m3dPOLY tri;
    tri.nVert   = 3;
    tri.vert[0] = *a;
    tri.vert[1] = *b;
    tri.vert[2] = *c;
    m3dV n, x, z;
    m3dGetPolyNormal(&tri, &n);
    m3dSub(b, a, &x);
    m3dNormalize(&x);
    m3dCross(&x, &n, &z);
    m3dNormalize(&z);
    m3dSetIdentity(this);
    m[0][0] = x.x; m[0][1] = n.x; m[0][2] = z.x;
    m[1][0] = x.y; m[1][1] = n.y; m[1][2] = z.y;
    m[2][0] = x.z; m[2][1] = n.z; m[2][2] = z.z;
    Translate(-a->x, -a->y, -a->z, M3D_OP_LOCAL);
}


// v12 0x00562F00  (feb 0x0054B9E0)
// Affine matrix mapping polygon 'src' onto polygon 'dst' (texture / decal transfer): both are
// flattened into their own triangle space, a 2D affine map is fitted (m2dMATR::MakePolyPoly) and the
// normal direction is scaled by sqrt(area ratio). Returns 0 when it cannot be built.
// 1.2: same logic (inline point transform rescheduled).
BOOL32 m3dMATR::MakePolyPoly(m3dPOLY *src, m3dPOLY *dst)
{
    m3dMATR ms, md, mdInv, t;
    ms.MakeProjTri(&src->vert[0], &src->vert[1], &src->vert[2]);
    md.MakeProjTri(&dst->vert[0], &dst->vert[1], &dst->vert[2]);
    m3dPOLY ls, ld;
    for (int i = 0; i < src->nVert; i++)
        m3dTransformPoint(&src->vert[i], &ms, &ls.vert[i]);
    ls.nVert = src->nVert;
    for (int i = 0; i < dst->nVert; i++)
        m3dTransformPoint(&dst->vert[i], &md, &ld.vert[i]);
    ld.nVert = dst->nVert;
    m2dPOLY s2, d2;
    m3xzPoly3DToPoly2D(&ls, &s2);
    m3xzPoly3DToPoly2D(&ld, &d2);
    m2dMATR m2;
    if (!m2.MakePolyPoly(&s2, &d2))
        return 0;
    m3dMATR a;
    m3dSetIdentity(&a);
    a.m[0][0] = m2.m[0][0];
    a.m[2][0] = m2.m[1][0];
    a.m[0][2] = m2.m[0][1];
    a.m[2][2] = m2.m[1][1];
    a.m[3][2] = m2.m[2][1];
    a.m[3][0] = m2.m[2][0];
    float k = m3dSqrt(m3dFAbs(_m2dCalcMatrDet(&a, 0, 1, 2, 0, 1, 2) * a.m[3][3]));
    a.Scale(1.0f, k, 1.0f, M3D_OP_GLOBAL);
    if (!md.Invert(&mdInv))
        return 0;
    *this = ms;
    m3dMultiplyMatr(this, &a, &t);
    *this = t;
    m3dMultiplyMatr(this, &mdInv, &t);
    *this = t;
    return 1;
}


// v12 0x005634B0  (feb 0x0054BF90)
// In-place transpose.
void m3dMATR::Transpose()
{
    float t;
    t = m[0][1]; m[0][1] = m[1][0]; m[1][0] = t;
    t = m[0][2]; m[0][2] = m[2][0]; m[2][0] = t;
    t = m[0][3]; m[0][3] = m[3][0]; m[3][0] = t;
    t = m[1][2]; m[1][2] = m[2][1]; m[2][1] = t;
    t = m[1][3]; m[1][3] = m[3][1]; m[3][1] = t;
    t = m[2][3]; m[2][3] = m[3][2]; m[3][2] = t;
}


// v12 0x00563500  (feb 0x0054BFE0)
// Inverse through the adjugate (cofactors from 3x3 minors). The determinant is taken as
// det3(rotation part) * m33, i.e. the matrix is assumed affine. 0 when |det| < eps^2.
BOOL32 m3dMATR::Invert(m3dMATR *out)
{
    float det = _m2dCalcMatrDet(this, 0, 1, 2, 0, 1, 2);
    if (m3dFAbs(det * m[3][3]) < M3D_EPSILON * M3D_EPSILON)
        return 0;
    float inv = 1.0f / (det * m[3][3]);
    static const short others[4][3] = { { 1, 2, 3 }, { 0, 2, 3 }, { 0, 1, 3 }, { 0, 1, 2 } };
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) {
            const short *r = others[i], *c = others[j];
            float v = _m2dCalcMatrDet(this, r[0], r[1], r[2], c[0], c[1], c[2]);
            out->m[j][i] = ((i + j) % 2 == 0) ? v * inv : -(v * inv);
        }
    return 1;
}


// v12 0x00563700  (feb 0x0054C1E0)
// Removes the scale (mirror kept as a negative scale): Scale(1/sx, 1/sy, 1/sz, LOCAL).
// (GetScale is inlined in the original.)
// 1.2: same logic (inlined m3dSqrt rescheduled).
void m3dMATR::RemoveScale()
{
    float sx, sy, sz;
    GetScale(&sx, &sy, &sz);
    Scale(1.0f / sx, 1.0f / sy, 1.0f / sz, M3D_OP_LOCAL);
}


// v12 0x00564190  (feb 0x0054CC70)
// dst[i] = src[i] * M (points) for n entries with byte strides. src may equal dst (the x87 path
// uses a temporary then). SSE / 3DNow! paths in the original.
void __cdecl m3dMATR::TransformPointList(int n, void *src, int srcStride, void *dst, int dstStride)
{
    char *s = (char *)src, *d = (char *)dst;
    for (int i = 0; i < n; i++) {
        m3dV r;
        m3dTransformPoint((m3dV *)s, this, &r);
        *(m3dV *)d = r;
        s += srcStride;
        d += dstStride;
    }
}


// v12 0x00564600  (feb 0x0054D0E0)
// dst[i] = src[i] * M (vectors: no translation), same layout rules as TransformPointList.
void __cdecl m3dMATR::TransformVectorList(int n, void *src, int srcStride, void *dst, int dstStride)
{
    char *s = (char *)src, *d = (char *)dst;
    for (int i = 0; i < n; i++) {
        m3dV r;
        m3dTransformVector((m3dV *)s, this, &r);
        *(m3dV *)d = r;
        s += srcStride;
        d += dstStride;
    }
}


// v12 0x00564A00  (feb 0x0054D4E0)
// Determinant of the 3x3 sub-matrix with rows r0,r1,r2 and columns c0,c1,c2 (rule of Sarrus).
float __fastcall _m2dCalcMatrDet(m3dMATR *m, short r0, short r1, short r2, short c0, short c1, short c2)
{
    const float (*a)[4] = m->m;
    return ((a[r1][c2] * a[r2][c0] * a[r0][c1]
           + a[r0][c2] * a[r1][c0] * a[r2][c1]
           + a[r2][c2] * a[r0][c0] * a[r1][c1])
           - a[r0][c2] * a[r2][c0] * a[r1][c1])
           - a[r2][c2] * a[r1][c0] * a[r0][c1]
           - a[r1][c2] * a[r0][c0] * a[r2][c1];
}
