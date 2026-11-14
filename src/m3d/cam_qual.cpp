// cam_qual.cpp -- Will Rock 1.2 (v12.exe): camCAMERA view-frustum visibility tests ("POV qualification")
//
// Reconstructed from the Feb 2003 build (m3d:cam_qual.obj). All functions identical in 1.2 (only the
// inlined SSE m3dSqrt differs in register allocation).
// v12 range 0x0057CA90 .. 0x0057E33F (feb 0x00564160 .. 0x00565A1F).
//
// The frustum is the pyramid eye + 4 corner rays of the near-plane viewport, closed by the near plane.
// Volumes are tested with a separating-axis test: the 5 frustum plane normals, the volume's 3 axes,
// and the 3x5 cross products of volume axes with frustum edge rays (5 + 3 + 15 = 23 axes).
// Results: 1 = outside (cull), -1 = fully inside, 0 = partially visible.
// HOOK: object/portal culling for rendering goes through QualBoxPOV / QualSpherePOV.

#include "cam.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0057CF60 (feb 0x00564630) -> v12 0x009AED84 (feb 0x00963A34)

static const float CAM_AXIS_EPS  = 0.001f;        // __real@3a83126f (v12 0x005FB874): degenerate axis
static const float CAM_QUAL_EPS  = M3D_EPSILON;   // v12 0x00609B34 (feb 0x005DE648)

// half-extent axes of the box under test (only the diagonal is ever written; zero-initialised .bss)
static m3dV _camBoxAxes[3];                       // v12 0x009AED60 (feb 0x00963A10)

// Projection of the frustum onto 'axis' vs. projection of the volume (centre c, half axes h).
// rays: the 4 corner rays (all must project with the same sign, else the axis is not usable: -1).
// radius(h) differs between the axis-aligned and the oriented version.
template <bool OBB>
static inline int _camSeparate(m3dV *axis, m3dV *c, m3dV *h, m3dV *eye, m3dV *ray, float dist)
{
    float l = m3dSqrt(axis->x * axis->x + axis->y * axis->y + axis->z * axis->z);
    if (m3dFAbs(l) < CAM_AXIS_EPS)
        return -1;
    float p0 = m3dDotProduct(axis, &ray[0]);
    bool  pos = p0 > CAM_QUAL_EPS;
    float p1 = m3dDotProduct(axis, &ray[1]);
    if ((p1 > CAM_QUAL_EPS) != pos) return -1;
    float p2 = m3dDotProduct(axis, &ray[2]);
    if ((p2 > CAM_QUAL_EPS) != pos) return -1;
    float p3 = m3dDotProduct(axis, &ray[3]);
    if ((p3 > CAM_QUAL_EPS) != pos) return -1;
    float e  = m3dDotProduct(axis, eye);
    float r;
    float cc = m3dDotProduct(axis, c);
    if (OBB)
        r = m3dFAbs(m3dDotProduct(axis, &h[2])) + m3dFAbs(m3dDotProduct(axis, &h[1])) + m3dFAbs(m3dDotProduct(axis, &h[0]));
    else
        r = m3dFAbs(h[2].z * axis->z) + m3dFAbs(h[1].y * axis->y) + m3dFAbs(axis->x * h[0].x);
    if (!pos) {                                   // frustum extends towards -axis: use the largest ray
        float m = p3;
        if (m < p2) m = p2;
        if (m < p1) m = p1;
        if (m < p0) m = p0;
        float f = m * dist + e;
        if (f < cc - r) return 1;
        if (f <= cc + r) return 0;
        return -1;
    } else {                                      // towards +axis: smallest ray
        float m = p3;
        if (p2 < m) m = p2;
        if (p1 < m) m = p1;
        if (p0 < m) m = p0;
        float f = m * dist + e;
        if (r + cc < f) return 1;
        if (cc - r <= f) return 0;
        return -1;
    }
}


// v12 0x0057CA90  (feb 0x00564160)
// Rebuilds the frustum data (pov) from the current matrix and viewport. Call after moving the camera.
void camCAMERA::CalcPOVQual()
{
    float hx = vpDx * 0.5f, hy = vpDy * 0.5f;
    pov.cornerDist = m3dSqrt(hx * hx + hy * hy + zNear * zNear);
    GetOrigin(&pov.eye);
    for (int i = 0; i < 4; i++) {
        m3dV p = { viewPoly.vert[i].x, viewPoly.vert[i].y, zNear };
        m3dTransformVector(&p, &c2w, &pov.cornerRay[i]);
        m3dNormalize(&pov.cornerRay[i]);
    }
    for (int i = 0; i < 3; i++)
        m3dCrossProduct(&pov.cornerRay[i], &pov.cornerRay[i + 1], &pov.planeN[i]);
    m3dCrossProduct(&pov.cornerRay[3], &pov.cornerRay[0], &pov.planeN[3]);
    m3dV back = { 0.0f, 0.0f, -1.0f };
    m3dTransformVector(&back, &c2w, &pov.planeN[4]);
    pov.planeD[0] = 0.0f;
    pov.planeD[1] = 0.0f;
    pov.planeD[2] = 0.0f;
    pov.planeD[3] = 0.0f;
    pov.planeD[4] = m3dDotProduct(&pov.planeN[4], &pov.cornerRay[0]) * pov.cornerDist;
}


// v12 0x0057CF80  (feb 0x00564650)
// Axis-aligned box vs. frustum.
int camCAMERA::QualBoxPOV(m3dBOX *b)
{
    int inside = 0;
    _camBoxAxes[0].x = (b->vMax.x - b->vMin.x) * 0.5f;
    _camBoxAxes[1].y = (b->vMax.y - b->vMin.y) * 0.5f;
    _camBoxAxes[2].z = (b->vMax.z - b->vMin.z) * 0.5f;
    m3dV c = { (b->vMin.x + b->vMax.x) * 0.5f, (b->vMin.y + b->vMax.y) * 0.5f, (b->vMin.z + b->vMax.z) * 0.5f };

    // the 5 frustum planes
    for (int i = 0; i < 5; i++) {
        m3dV *n = &pov.planeN[i];
        float d  = n->y * pov.eye.y + n->x * pov.eye.x + n->z * pov.eye.z + pov.planeD[i];
        float cc = n->x * c.x + n->y * c.y + n->z * c.z;
        float r  = m3dFAbs(_camBoxAxes[2].z * n->z) + m3dFAbs(_camBoxAxes[1].y * n->y) + m3dFAbs(_camBoxAxes[0].x * n->x);
        if (d < cc - r)
            return 1;
        if (r + cc < d)
            inside++;
    }
    if (inside == 5)
        return -1;
    // box axes
    for (int a = 0; a < 3; a++) {
        int res = _camSeparateBBoxPOV(&pov, &_camBoxAxes[a], &c, _camBoxAxes, &pov.eye, pov.cornerRay, pov.cornerDist);
        if (res == 1) return 1;
        if (res == -1) inside++;
    }
    // box axis x frustum plane normal
    for (int a = 0; a < 3; a++) {
        for (int k = 0; k < 5; k++) {
            m3dV ax;
            m3dCrossProduct(&_camBoxAxes[a], &pov.planeN[k], &ax);
            int res = _camSeparateBBoxPOV(&pov, &ax, &c, _camBoxAxes, &pov.eye, pov.cornerRay, pov.cornerDist);
            if (res == 1) return 1;
            if (res == -1) inside++;
        }
    }
    return (inside != 23) - 1;
}


// v12 0x0057D210  (feb 0x005648E0)
// Separating-axis step for an axis-aligned box (h = diagonal half extents).
int __fastcall _camSeparateBBoxPOV(camPOV_QUAL *q, m3dV *axis, m3dV *c, m3dV h[3], m3dV *eye, m3dV ray[4], float dist)
{
    return _camSeparate<false>(axis, c, h, eye, ray, dist);
}


// v12 0x0057D5D0  (feb 0x00564CA0)
// Oriented box vs. frustum (OBB: corner org + 3 full edge vectors).
int camCAMERA::QualOBBPOV(m3dOBB *b)
{
    m3dV h[3];
    for (int a = 0; a < 3; a++)
        h[a] = b->ax[a] * 0.5f;
    m3dV c = { (b->ax[2].x + b->ax[1].x + b->ax[0].x) * 0.5f + b->org.x,
               (b->ax[2].y + b->ax[1].y + b->ax[0].y) * 0.5f + b->org.y,
               (b->ax[2].z + b->ax[1].z + b->ax[0].z) * 0.5f + b->org.z };
    int inside = 0;
    for (int i = 0; i < 5; i++) {
        int res = _camSeparateOBBPOV_AxisPOV(&pov, i, &c, h);
        if (res == 1) return 1;
        if (res == -1) inside++;
    }
    if (inside == 5)
        return -1;
    for (int a = 0; a < 3; a++) {
        int res = _camSeparateOBBPOV(&pov, &h[a], &c, h, &pov.eye, pov.cornerRay, pov.cornerDist);
        if (res == 1) return 1;
        if (res == -1) inside++;
    }
    for (int a = 0; a < 3; a++) {
        for (int k = 0; k < 5; k++) {
            m3dV ax;
            m3dCrossProduct(&h[a], &pov.planeN[k], &ax);
            int res = _camSeparateOBBPOV(&pov, &ax, &c, h, &pov.eye, pov.cornerRay, pov.cornerDist);
            if (res == 1) return 1;
            if (res == -1) inside++;
        }
    }
    return (inside != 23) - 1;
}


// v12 0x0057D810  (feb 0x00564EE0)
// Separating-axis step for an oriented box (h = 3 half edge vectors).
int __fastcall _camSeparateOBBPOV(camPOV_QUAL *q, m3dV *axis, m3dV *c, m3dV h[3], m3dV *eye, m3dV ray[4], float dist)
{
    return _camSeparate<true>(axis, c, h, eye, ray, dist);
}


// v12 0x0057DC40  (feb 0x00565310)
// One frustum plane vs. an oriented box: 1 outside, -1 inside, 0 crossing (-1 for a degenerate normal).
int __fastcall _camSeparateOBBPOV_AxisPOV(camPOV_QUAL *q, int plane, m3dV *c, m3dV h[3])
{
    m3dV n = q->planeN[plane];
    float l = m3dSqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (m3dFAbs(l) < CAM_AXIS_EPS)
        return -1;
    float d  = n.x * q->eye.x + n.y * q->eye.y + n.z * q->eye.z + q->planeD[plane];
    float cc = n.x * c->x + n.y * c->y + n.z * c->z;
    float r  = m3dFAbs(m3dDot(n, h[2])) + m3dFAbs(m3dDot(n, h[1])) + m3dFAbs(m3dDot(n, h[0]));
    if (d < cc - r)
        return 1;
    if (r + cc < d)
        return -1;
    return 0;
}


// v12 0x0057DE60  (feb 0x00565540)
// Prism vs. frustum, approximated by the OBB spanned by vertices 0,1,2 and the top of vertex 0.
// ? assumes a 4-sided prism (GetVertices writes base then top; vertex 4 = top of vertex 0).
int camCAMERA::QualPrismPOV(m3dPRISM *p)
{
    m3dV v[2 * M3D_POLY_MAX_VERT];
    p->GetVertices(v);
    m3dV h[3];
    h[0] = (v[1] - v[0]) * 0.5f;
    h[1] = (v[2] - v[1]) * 0.5f;
    h[2] = (v[4] - v[0]) * 0.5f;            // ? GetVertices layout: index 4 = v0 raised (feb stack -0x30)
    m3dV c = { (v[6].x + v[0].x) * 0.5f, (v[6].y + v[0].y) * 0.5f, (v[6].z + v[0].z) * 0.5f };  // ? opposite corner
    int inside = 0;
    for (int i = 0; i < 5; i++) {
        int res = _camSeparateOBBPOV_AxisPOV(&pov, i, &c, h);
        if (res == 1) return 1;
        if (res == -1) inside++;
    }
    if (inside == 5)
        return -1;
    for (int a = 0; a < 3; a++) {
        int res = _camSeparateOBBPOV(&pov, &h[a], &c, h, &pov.eye, pov.cornerRay, pov.cornerDist);
        if (res == 1) return 1;
        if (res == -1) inside++;
    }
    for (int a = 0; a < 3; a++) {
        for (int k = 0; k < 5; k++) {
            m3dV ax;
            m3dCrossProduct(&h[a], &pov.planeN[k], &ax);
            int res = _camSeparateOBBPOV(&pov, &ax, &c, h, &pov.eye, pov.cornerRay, pov.cornerDist);
            if (res == 1) return 1;
            if (res == -1) inside++;
        }
    }
    return (inside != 23) - 1;
}


// v12 0x0057E0E0  (feb 0x005657C0)
// Sphere vs. view: behind the near plane -> 1; touching the near plane -> 0; otherwise the sphere's
// projected bounding rectangle on the near plane (roots of the cone-tangent quadratics in x and y)
// is clipped against the viewport and the clip result is returned.
int camCAMERA::QualSpherePOV(m3dSPHERE *s)
{
    float r = s->radius;
    m3dV p;
    TransformPoint(&s->center, &p);
    if (p.z + r < zNear)
        return 1;
    if (p.z - r < CAM_QUAL_EPS * 100.0f + zNear)
        return 0;
    float d   = m3dSqrt(p.z * p.z + p.x * p.x + p.y * p.y);
    float c2  = 1.0f - (r / d) * (r / d);       // cos^2 of the cone half angle
    m3dNormalize(&p);
    float n   = zNear;
    float a   = (p.y * p.y + p.x * p.x) - c2;
    float b2n = n * p.z + n * p.z;
    float cz  = p.z * p.z - c2;
    float y0, y1, x0, x1;
    m3dSolveQuadraticEq(a, b2n * p.y, (p.x * p.x + cz) * n * n, &y0, &y1);
    m3dSolveQuadraticEq(a, b2n * p.x, (p.y * p.y + cz) * n * n, &x0, &x1);
    m2dPOLY rect, out;
    rect.nVert = 4;
    rect.vert[0].x = x1; rect.vert[0].y = y0;
    rect.vert[1].x = x1; rect.vert[1].y = y1;
    rect.vert[2].x = x0; rect.vert[2].y = y1;
    rect.vert[3].x = x0; rect.vert[3].y = y0;
    return m2dClipPolyRect(&rect, &viewPoly, &out);
}
