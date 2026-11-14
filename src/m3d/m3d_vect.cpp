// m3d_vect.cpp -- Will Rock 1.2 (v12.exe): vector angles, basis decomposition, mirror/reflect, the vector constants
//
// Reconstructed from the Feb 2003 build (m3d:m3d_vect.obj) and checked against v12.
// v12 range 0x00564AE0 .. 0x00565B24 (feb 0x0054D5C0 .. 0x0054E2FF). Code is unchanged in 1.2: the "callgraph"
// matches of the index differ only by the recompiled SSE path of the inline m3dSqrt.
// Angles are in DEGREES unless noted (m3dWCS2SphereCoord returns radians).

#include "m3d_vect.h"

static const float VECT_EPS     = M3D_EPSILON;    // v12 0x00609918 (feb 0x005DE448)
static const float VECT_PI      = M3D_PI;         // v12 0x0060991C (feb 0x005DE44C)
static const float VECT_COS_LIM = 0.99999f;       // __real@3f7ff972 v12 0x0060992C (feb 0x005DE454)
static const float VECT_BASIS_K = 0.1f;           // __real@3dcccccd (degenerate-basis tolerance = 0.1*eps)

// ---- globals (.data, statically initialised) --------------------------------------------------
m3dV m3dVUnitX  = { 1.0f, 0.0f, 0.0f };    // v12 0x00637BD0 (feb 0x005F9984)
m3dV m3dVUnitY  = { 0.0f, 1.0f, 0.0f };    // v12 0x00637BDC (feb 0x005F9990)
m3dV m3dVUnitZ  = { 0.0f, 0.0f, 1.0f };    // v12 0x00637BE8 (feb 0x005F999C)
m3dV m3dVOneXYZ = { 1.0f, 1.0f, 1.0f };    // v12 0x00637BF4 (feb 0x005F99A8)
m2dV m2dVUnitX  = { 1.0f, 0.0f };          // v12 0x00637C00 ? (feb 0x005F99B4)
m2dV m2dVUnitY  = { 0.0f, 1.0f };          // v12 0x00637C08 ? (feb 0x005F99BC)
m2dV m2dVOneXY  = { 1.0f, 1.0f };          // v12 0x00637C10 ? (feb 0x005F99C4)

// ---- globals (.bss) -----------------------------------------------------------------------------
m3dV    m3dVZero;                          // v12 0x009AEA84 (feb 0x0096373C)
m2dV    m2dVZero;                          // v12 0x009AEA9C (feb 0x00963754)
m3dVINT m3dVIntZero;                       // v12 0x009AEAAC (feb 0x00963764)

// _$E7  v12 0x00564AE0 (feb 0x0054D5C0): dynamic initializer of m3dVUndef
m3dV m3dVUndef = { M3D_UNDEF_COORD, M3D_UNDEF_COORD, M3D_UNDEF_COORD };   // v12 0x009AEA90 (feb 0x00963748)
// v12 0x00564B20  NEW in 1.2 (name ours; SSE / 3DNow! / x87 paths, x87 = m3dNormalize per vector)
void __fastcall m3dNormalizeList(int n, m3dV *v)
{
    for (int i = 0; i < n; i++)
        m3dNormalize(&v[i]);
}

// _$E10 v12 0x00564B00 (feb 0x0054D5E0): dynamic initializer of m2dVUndef
m2dV m2dVUndef = { M3D_UNDEF_COORD, M3D_UNDEF_COORD };                     // v12 0x009AEAA4 (feb 0x0096375C)

// _$E2 v12 ? (feb 0x0054E120) -> feb 0x00963738
M3D_DECLARE_EPS_SQ();


static inline float vLen(const m3dV *v) { return m3dSqrt(v->x * v->x + v->y * v->y + v->z * v->z); }
static inline float vDot(const m3dV *a, const m3dV *b) { return a->x * b->x + a->y * b->y + a->z * b->z; }
static inline float clampCos(float c) { return c < -1.0f ? -1.0f : (c > 1.0f ? 1.0f : c); }


// v12 0x00564DA0  (feb 0x0054D600)
// Angle (degrees) between two UNIT vectors (no normalisation).
float __fastcall _m3dAngleVector(m3dV *a, m3dV *b)
{
    float c = clampCos(vDot(a, b));
    return m3dACos(c) / VECT_PI * 180.0f;
}


// v12 0x00564E10  (feb 0x0054D670)
// Cosine of the angle between two arbitrary vectors, clamped to [-1,1]; 1 if either is ~zero.
float __fastcall m3dAngleVector_COS(m3dV *a, m3dV *b)
{
    float la = vLen(a);
    float l  = vLen(b) * la;
    if (m3dFAbs(l) < VECT_EPS)
        return 1.0f;
    return clampCos(vDot(a, b) / l);
}


// v12 0x00565010  (feb 0x0054D870)
// Angle (degrees, 0..180) between two arbitrary vectors; 0 if either is ~zero.
float __fastcall m3dAngleVector(m3dV *a, m3dV *b)
{
    float la = vLen(a);
    float lb = vLen(b);
    float c;
    if (m3dFAbs(lb * la) < VECT_EPS)
        c = 1.0f;
    else
        c = clampCos(vDot(a, b) / (lb * la));
    return 180.0f / VECT_PI * m3dACos(c);
}


// v12 0x00565220  (feb 0x0054DA90)
// Signed angle (degrees, -180..180) between the XZ projections of a and b; negative when b is
// clockwise from a seen from above. Temporarily zeroes a->y / b->y in place (restored on exit).
float __fastcall m3xzAngleVector(m3dV *a, m3dV *b)
{
    float ay = a->y, by = b->y;
    a->y = 0.0f;
    b->y = 0.0f;
    float la = vLen(a);
    float l  = vLen(b) * la;
    float c  = (m3dFAbs(l) < VECT_EPS) ? 1.0f : clampCos(vDot(a, b) / l);
    float ang = m3dACos(c) / VECT_PI * 180.0f;
    if (ang < 0.0f)        ang = 0.0f;
    else if (ang > 180.0f) ang = 180.0f;
    if (b->x * a->z - b->z * a->x < 0.0f)
        ang = -ang;
    a->y = ay;
    b->y = by;
    return ang;
}


// v12 0x005654C0  (feb 0x0054DD20)
// Decomposes v in the (non-orthogonal) basis a, b:  v = u*a + w*b.  Uses the largest component of
// a x b as the 2x2 determinant; for (anti)parallel a, b projects v onto whichever of them is non-zero.
void __fastcall m3dBasisCoord(m3dV *a, m3dV *b, m3dV *v, float *u, float *w)
{
    float dY = a->x * b->z - a->z * b->x;     // -(a x b).y
    float dZ = a->x * b->y - b->x * a->y;     //  (a x b).z
    float dX = a->z * b->y - a->y * b->z;     // -(a x b).x
    float tol = VECT_EPS * VECT_BASIS_K;

    if (m3dFAbs(dY) < tol && m3dFAbs(dZ) < tol && m3dFAbs(dX) < tol) {
        float l = vLen(a);
        if (l > VECT_EPS * VECT_BASIS_K) {
            *u = vDot(a, v) / l;              // ? divides by |a| once only (as compiled)
            *w = 0.0f;
            return;
        }
        l = vLen(b);
        if (l > VECT_EPS * VECT_BASIS_K) {
            *w = vDot(b, v) / l;
            *u = 0.0f;
            return;
        }
        *w = 0.0f;
        *u = 0.0f;
        return;
    }

    if (m3dFAbs(dY) > m3dFAbs(dZ)) {
        if (m3dFAbs(dX) < m3dFAbs(dY)) {      // XZ plane
            float k = 1.0f / dY;
            *u = (v->x * b->z - v->z * b->x) * k;
            *w = (v->z * a->x - a->z * v->x) * k;
            return;
        }
    } else if (m3dFAbs(dX) < m3dFAbs(dZ)) {   // XY plane
        float k = 1.0f / dZ;
        *u = (v->x * b->y - v->y * b->x) * k;
        *w = (v->y * a->x - v->x * a->y) * k;
        return;
    }
    float k = 1.0f / dX;                      // YZ plane
    *u = (v->z * b->y - v->y * b->z) * k;
    *w = (v->y * a->z - v->z * a->y) * k;
}


// v12 0x005658C0  (feb 0x0054E140)
// Signed angle (degrees) between two 2D vectors, via m3xzAngleVector on (x,0,y).
float __fastcall m2dAngleVector(m2dV *a, m2dV *b)
{
    m3dV va = { a->x, 0.0f, a->y };
    m3dV vb = { b->x, 0.0f, b->y };
    return m3xzAngleVector(&va, &vb);
}


// v12 0x005659B0  (feb 0x0054E180)
// Mirror of point p in the plane through pp with unit normal pn.
// NB argument order as compiled is (planePt, planeN, p, out); the SDK names them (p, pp, pn, out).
void __fastcall m3dMirrorPointPlane(m3dV *pp, m3dV *pn, m3dV *p, m3dV *out)
{
    m3dV d = { p->x - pp->x, p->y - pp->y, p->z - pp->z };
    float k = d.x * pn->x + d.y * pn->y + d.z * pn->z;
    out->x = d.x - (k * pn->x + k * pn->x) + pp->x;
    out->y = d.y - (k * pn->y + k * pn->y) + pp->y;
    out->z = d.z - (k * pn->z + k * pn->z) + pp->z;
}


// v12 0x00565A40  (feb 0x0054E210)
// out = v - 2*(n.v)*n : reflection of v about the unit normal n (arg order (n, v, out)).
void __fastcall m3dReflectVector(m3dV *n, m3dV *v, m3dV *out)
{
    float k = vDot(n, v) * -2.0f;
    out->x = k * n->x + v->x;
    out->y = k * n->y + v->y;
    out->z = k * n->z + v->z;
}


// v12 0x00565A80  (feb 0x0054E250)
// Unit vector -> spherical angles in RADIANS: *polar = acos(y) (0 = up), *azim = 0..2PI in XZ from +X.
void __fastcall m3dWCS2SphereCoord(m3dV *v, float *polar, float *azim)
{
    float a = m3dACos(v->y);
    *polar = a;
    float c = v->x / m3dCos(a);               // ? divides by cos (as compiled), sin would be the textbook form
    if (c < -VECT_COS_LIM) { *azim = VECT_PI; return; }
    if (c >  VECT_COS_LIM) { *azim = 0.0f;    return; }
    if (v->z >= 0.0f)
        *azim = m3dACos(c);
    else
        *azim = (VECT_PI + VECT_PI) - m3dACos(c);
}
