// m2d_matr.cpp -- Will Rock 1.2 (v12.exe): m2dMATR, 3x3 matrices for 2D affine transforms
//
// Reconstructed from the Feb 2003 build (m3d:m2d_matr.obj) and checked against v12.
// v12 range 0x0057C420 .. 0x0057CA8F (feb 0x00563B00 .. 0x0056415F).
//
// Layout: m[3][3] row-major, row vectors: p' = (x, y, 1) * M; row 2 is the translation.
// "op": 0 = set, 1 = local (M = T*M), 2 = global (M = M*T)  (same convention as m3dMATR).
//
// SDK gap: math_app/m3d.h declares m2dMATR as a plain struct without methods, so the methods are
// reconstructed on m2dMATR_ (m2d_matr.h), a method-only view of the same 0x24-byte struct.

#include "m2d_matr.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0057CA70 (feb 0x00564140) -> v12 0x009AED38 (feb 0x009639E8)

m2dMATR        m2dMatrIdentity;   // v12 0x009AED14 (feb 0x009639C4)
static m2dMATR _m2dMatrRot;       // v12 0x009AED3C (feb 0x009639EC) scratch rotation used by Rotate


// v12 0x0057C420  (feb 0x00563B00)
// Initialises the identity matrices (the scratch rotation keeps row 2 = (0,0,1) for Rotate).
BOOL32 __fastcall m3dInit2DMatr()
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            _m2dMatrRot.m[i][j] = 0.0f;
    for (int i = 0; i < 3; i++)
        _m2dMatrRot.m[i][i] = 1.0f;
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m2dMatrIdentity.m[i][j] = 0.0f;
    for (int i = 0; i < 3; i++)
        m2dMatrIdentity.m[i][i] = 1.0f;
    return 1;
}


// v12 0x0057C470  (feb 0x00563B50)
// Identity.
void m2dMATR_::Identity()
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m[i][j] = 0.0f;
    for (int i = 0; i < 3; i++)
        m[i][i] = 1.0f;
}


// v12 0x0057C4A0  (feb 0x00563B80)
// Scale: op 0 sets the diagonal, op 1 scales the axis rows, op 2 scales the columns (incl. translation).
void m2dMATR_::Scale(float sx, float sy, int op)
{
    if (op == 0) {
        m[0][0] = sx;
        m[1][1] = sy;
    } else if (op == 1) {
        for (int i = 0; i < 2; i++) {
            m[0][i] = sx * m[0][i];
            m[1][i] = sy * m[1][i];
        }
    } else if (op == 2) {
        for (int r = 0; r < 3; r++) {
            m[r][0] = sx * m[r][0];
            m[r][1] = sy * m[r][1];
        }
    }
}


// v12 0x0057C510  (feb 0x00563BF0)
// Translation: op 0 sets, op 1 moves along the matrix axes, op 2 moves in parent space.
void m2dMATR_::Translate(float x, float y, int op)
{
    if (op == 0) {
        m[2][0] = x;
        m[2][1] = y;
    } else if (op == 1) {
        m[2][0] = x * m[0][0] + y * m[1][0] + m[2][0];
        m[2][1] = y * m[1][1] + x * m[0][1] + m[2][1];
    } else if (op == 2) {
        m[2][0] = x + m[2][0];
        m[2][1] = y + m[2][1];
    }
}


// v12 0x0057C580  (feb 0x00563C60)
// Rotation by deg degrees: op 0 replaces the whole matrix, op 2 = M*R, any other op = R*M.
void m2dMATR_::Rotate(float deg, int op)
{
    float a = M3D_PI * deg * (1.0f / 180.0f);
    float c = m3dCos(a), s = m3dSin(a);
    _m2dMatrRot.m[0][0] = c;
    _m2dMatrRot.m[0][1] = s;
    _m2dMatrRot.m[1][0] = -s;
    _m2dMatrRot.m[1][1] = c;
    if (op == 0) {
        *(m2dMATR *)this = _m2dMatrRot;
        return;
    }
    float t[3][3];
    if (op == 2) {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                float sum = 0.0f;
                for (int k = 0; k < 3; k++)
                    sum = m[i][k] * _m2dMatrRot.m[k][j] + sum;
                t[i][j] = sum;
            }
    } else {
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) {
                float sum = 0.0f;
                for (int k = 0; k < 3; k++)
                    sum = _m2dMatrRot.m[i][k] * m[k][j] + sum;
                t[i][j] = sum;
            }
    }
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m[i][j] = t[i][j];
}


// v12 0x0057C6C0  (feb 0x00563DA0)
// Pure rotation matrix (note the opposite sign convention to Rotate: m01 = -sin).
// 1.2: the whole matrix is cleared first (rep stosd, 9 dwords). In the beta only the 2x2 block was
//      written, so translation / row 2 kept stale values. Row 2 is now (0,0,0) -- m22 is NOT set to 1.
void m2dMATR_::MakeRot(float deg)
{
    for (int i = 0; i < 3; i++)                  // 1.2: new
        for (int j = 0; j < 3; j++)
            m[i][j] = 0.0f;
    float a = M3D_PI * deg * (1.0f / 180.0f);
    float c = m3dCos(a);
    m[0][0] = c;
    float s = m3dSin(a);
    m[0][1] = -s;
    m[1][0] = s;
    m[1][1] = c;
}


// v12 0x0057C700  (feb 0x00563DD0)
// Affine map taking the first three vertices of 'from' onto those of 'to'. 0 if 'from' is degenerate.
BOOL32 m2dMATR_::MakePolyPoly(m2dPOLY *from, m2dPOLY *to)
{
    m3dMATR pts, inv;
    for (int i = 0; i < 3; i++) {                // columns = source points (x, y, 1)
        pts.m[0][i] = from->vert[i].x;
        pts.m[1][i] = from->vert[i].y;
        pts.m[2][i] = 1.0f;
        pts.m[3][i] = 0.0f;
        pts.m[i][3] = 0.0f;
    }
    pts.m[3][3] = 1.0f;
    if (!pts.Invert(&inv))
        return 0;
    float r[3];
    float v[3] = { to->vert[0].x, to->vert[1].x, to->vert[2].x };
    for (int j = 0; j < 3; j++) {
        float sum = 0.0f;
        for (int i = 0; i < 3; i++)
            sum = v[i] * inv.m[i][j] + sum;
        r[j] = sum;
    }
    float res[3][3];
    res[0][2] = 0.0f;
    res[1][2] = 0.0f;
    res[2][2] = 1.0f;
    res[0][0] = r[0]; res[1][0] = r[1]; res[2][0] = r[2];
    float w[3] = { to->vert[0].y, to->vert[1].y, to->vert[2].y };
    for (int j = 0; j < 3; j++) {
        float sum = 0.0f;
        for (int i = 0; i < 3; i++)
            sum = w[i] * inv.m[i][j] + sum;
        r[j] = sum;
    }
    res[0][1] = r[0]; res[1][1] = r[1]; res[2][1] = r[2];
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            m[i][j] = res[i][j];
    return 1;
}
