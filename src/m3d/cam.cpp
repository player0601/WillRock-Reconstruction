// cam.cpp -- Will Rock 1.2 (v12.exe): camCAMERA core (projection set-up, movement, orientation, animation)
//
// Reconstructed from the Feb 2003 build (m3d:cam.obj) and checked against v12.
// Every function of this object is identical in 1.2 except for compiler noise inside the inlined
// SSE m3dSqrt (register allocation), i.e. no behavioural change.
// v12 range 0x00569B00 .. 0x0056C52F (feb 0x005517F0 .. 0x0055449F).
//
// The SIMD branches of the inlined m3dSqrt / matrix-vector products (SSE, 3DNow!, x87 selected by
// m3dSimdType) all compute the same value; only the plain x87 form is written here.

#include "cam.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00569ED0 (feb 0x00551BC0) -> v12 0x009AEAD0 (feb 0x00963780)

#define CAM_DEF_NEAR      0.1f        // 0x3DCCCCCD
#define CAM_DEF_FAR       100000.0f   // 0x47C35000
#define CAM_DEF_ASPECT    0.75f       // 0x3F400000
#define CAM_DEF_ANGLE_HOR 80.0f       // 0x42A00000
#define CAM_DEF_WIN_DX    640.0f      // 0x44200000
#define CAM_DEF_WIN_DY    480.0f      // 0x43F00000

// _m3dNormalize: the inline of m3d_lib.h

// width of the near-plane rectangle for a field of view in degrees
static inline float _camNearSize(float angleDeg, float zNear)
{
    float t = m3dTan(angleDeg * M3D_PI * (1.0f / 180.0f) * 0.5f);
    return t * zNear + t * zNear;
}
// the other FOV from one FOV and the aspect (2*atan(tan(a/2)*k), degrees)
static inline float _camOtherAngle(float angleDeg, float k)
{
    float a = m3dATan(m3dTan(angleDeg * 0.5f * M3D_PI * (1.0f / 180.0f)) * k);
    return (a + a) / M3D_PI * 180.0f;
}

// inline helper: re-derive the viewport from angleHor / angleVert / zNear
void camCAMERA::_ViewportFromAngles()
{
    SetViewportDx(_camNearSize(angleHor, zNear));
    SetViewportDy(_camNearSize(angleVert, zNear));
}

// inline helper (expanded in every movement function): re-orthonormalise the three axis rows of
// c2w (length only) and rebuild w2c = c2w^-1 (same code as _camMatrInvert, inlined).
void camCAMERA::_UpdateW2C()
{
    for (int r = 0; r < 3; r++) {
        float l = 0.0f;
        for (int c = 0; c < 3; c++)
            l = c2w.m[r][c] * c2w.m[r][c] + l;
        l = 1.0f / m3dSqrt(l);
        for (int c = 0; c < 3; c++)
            c2w.m[r][c] = l * c2w.m[r][c];
    }
    w2c = c2w;
    _camMatrInvert(&w2c);
}

// inline helper (body of SetLookAt; also expanded in SetInterest, ResetVertRotation, Animate):
// builds an upright basis looking along dir (Y = world up projected, falls back to X) at org.
void camCAMERA::_Orient(m3dV *org, m3dV *dir)
{
    m3dV o = *org;
    m3dV f = *dir;
    m3dNormalize(&f);
    float k  = m3dVUnitY.x * f.x + m3dVUnitY.y * f.y + m3dVUnitY.z * f.z;
    m3dV up  = { m3dVUnitY.x - f.x * k, m3dVUnitY.y - f.y * k, m3dVUnitY.z - f.z * k };
    if (!_m3dNormalize(&up))
        up = m3dVUnitX;
    m3dV right = { up.z * f.y - f.z * up.y, f.z * up.x - up.z * f.x, up.y * f.x - f.y * up.x };
    SetParameters(&o, &up, &right, &f);
}


// v12 0x00569B00  (feb 0x005517F0)
// Default camera: near 0.1, far 100000, 80 deg horizontal FOV, aspect 0.75, 640x480 window,
// looking down -Z at the origin.
camCAMERA::camCAMERA()
{
    zNear    = CAM_DEF_NEAR;
    zFar     = CAM_DEF_FAR;
    aspectHW = CAM_DEF_ASPECT;
    angleHor = CAM_DEF_ANGLE_HOR;
    angleVert = _camOtherAngle(CAM_DEF_ANGLE_HOR, CAM_DEF_ASPECT);
    float tv = m3dTan(angleVert * M3D_PI * (1.0f / 180.0f) * 0.5f);
    float th = m3dTan(M3D_PI * (80.0f / 180.0f) * 0.5f);
    SetViewportDx(th * (2.0f * CAM_DEF_NEAR));            // constant-folded: 0.2
    SetViewportDy(tv * (2.0f * CAM_DEF_NEAR));
    c2w.Identity();
    c2w.m[2][2] = -1.0f;
    _UpdateW2C();
    winX = 0.0f;
    winY = 0.0f;
    splAngleVert = 0;
    splUnused    = 0;
    splPos       = 0;
    splInterest  = 0;
    winDx  = CAM_DEF_WIN_DX;
    winDy  = CAM_DEF_WIN_DY;
    scaleX = CAM_DEF_WIN_DX / vpDx;
    scaleY = CAM_DEF_WIN_DY / vpDy;
}


// v12 0x00569DC0  (feb 0x00551AB0)
// Deletes the animation splines.
camCAMERA::~camCAMERA()
{
    // m3dSPL vtable slot +0x04 = scalar deleting destructor, flag 1 = free memory
    if (splAngleVert) splAngleVert->__vecDelDtor(1);
    if (splUnused)    splUnused->__vecDelDtor(1);
    if (splPos)       splPos->__vecDelDtor(1);
    if (splInterest)  splInterest->__vecDelDtor(1);
}


// v12 0x00569E10  (feb 0x00551B00)
// Width of the near-plane viewport (centred): updates the 4-vertex view polygon and the pixel scale.
void camCAMERA::SetViewportDx(float dx)
{
    float l = dx * -0.5f;
    viewPoly.nVert = 4;
    viewPoly.vert[0].x = l;
    float r = dx * 0.5f;
    viewPoly.vert[1].x = r;
    viewPoly.vert[2].x = r;
    viewPoly.vert[3].x = l;
    vpX    = l;
    vpDx   = dx;
    scaleX = winDx / dx;
}


// v12 0x00569E70  (feb 0x00551B60)
// Height of the near-plane viewport (centred).
void camCAMERA::SetViewportDy(float dy)
{
    float b = dy * -0.5f;
    viewPoly.nVert = 4;
    viewPoly.vert[0].y = b;
    viewPoly.vert[1].y = b;
    float t = dy * 0.5f;
    viewPoly.vert[2].y = t;
    viewPoly.vert[3].y = t;
    vpDy   = dy;
    vpY    = b;
    scaleY = winDy / dy;
}


// v12 0x00569EF0  (feb 0x00551BE0)
// Stores the aspect ratio and re-derives the viewport from the CURRENT angles (angleVert is not
// recomputed here; call SetAngleHor afterwards to apply the new aspect).
// TWEAK (widescreen): the third-party widescreen patch in v12 hooks the callers, not this function.
void camCAMERA::SetAspectRatioHW(float a)
{
    aspectHW = a;
    _ViewportFromAngles();
}


// v12 0x00569FF0  (feb 0x00551CE0)
// Horizontal FOV in degrees; vertical FOV follows from the aspect ratio.
void camCAMERA::SetAngleHor(float deg)
{
    angleHor  = deg;
    angleVert = _camOtherAngle(deg, aspectHW);
    _ViewportFromAngles();
}


// v12 0x0056A120  (feb 0x00551E10)
// Vertical FOV in degrees; horizontal FOV follows from the aspect ratio.
void camCAMERA::SetAngleVert(float deg)
{
    angleVert = deg;
    angleHor  = _camOtherAngle(deg, 1.0f / aspectHW);    // tan(v/2) / aspect
    _ViewportFromAngles();
}


// v12 0x0056A250  (feb 0x00551F40)
// Near plane distance; the viewport is rescaled so the FOV stays the same.
void camCAMERA::SetNearPlane(float n)
{
    zNear = n;
    _ViewportFromAngles();
}


// v12 0x0056A350  (feb 0x00552040)
// Pixel window the near-plane viewport maps to.
void camCAMERA::SetViewWindow(float x, float y, float dx, float dy)
{
    winX  = x;
    winY  = y;
    winDx = dx;
    winDy = dy;
    scaleX = dx / vpDx;
    scaleY = dy / vpDy;
}


// v12 0x0056A3A0  (feb 0x00552090)
// Arbitrary (off-centre) near-plane viewport rectangle.
void camCAMERA::SetViewPort(float x, float y, float dx, float dy)
{
    viewPoly.vert[0].x = x;
    viewPoly.vert[0].y = y;
    viewPoly.vert[1].x = x + dx;
    viewPoly.vert[2].x = x + dx;
    viewPoly.vert[1].y = y;
    viewPoly.vert[3].x = x;
    viewPoly.vert[2].y = y + dy;
    viewPoly.vert[3].y = y + dy;
    vpX  = x;
    vpY  = y;
    vpDx = dx;
    vpDy = dy;
    scaleX = winDx / dx;
    scaleY = winDy / dy;
}


// v12 0x0056A440  (feb 0x00552130)
// Places the eye.
void camCAMERA::MoveTo(m3dV *p)
{
    c2w.m[3][0] = p->x;
    c2w.m[3][1] = p->y;
    c2w.m[3][2] = p->z;
    _UpdateW2C();
}


// v12 0x0056A5F0  (feb 0x005522F0)
// Moves forward (along the view direction) by d.
void camCAMERA::MoveRelForw(float d)
{
    c2w.Translate(0.0f, 0.0f, -d, 1);
    _UpdateW2C();
}


// v12 0x0056A7A0  (feb 0x005524B0)
// Strafes along the camera X axis.
void camCAMERA::MoveRelSide(float d)
{
    c2w.Translate(d, 0.0f, 0.0f, 1);
    _UpdateW2C();
}


// v12 0x0056A950  (feb 0x00552660)
// Moves along the camera Y axis.
void camCAMERA::MoveRelUp(float d)
{
    c2w.Translate(0.0f, d, 0.0f, 1);
    _UpdateW2C();
}


// v12 0x0056AB00  (feb 0x00552810)
// Moves by a vector given in camera space.
void camCAMERA::MoveRel(m3dV *d)
{
    c2w.Translate(d->x, d->y, d->z, 1);
    _UpdateW2C();
}


// v12 0x0056ACC0  (feb 0x005529D0)
// Pitch: rotation about the camera's own X axis (degrees).
void camCAMERA::TurnRelAroundX(float deg)
{
    c2w.RotateX(deg, 1);
    _UpdateW2C();
}


// v12 0x0056AE70  (feb 0x00552B80)
// Yaw about the world vertical through the eye (degrees).
void camCAMERA::TurnAroundVert(float deg)
{
    float ox = c2w.m[3][0], oy = c2w.m[3][1], oz = c2w.m[3][2];
    c2w.m[3][0] = 0.0f;
    c2w.m[3][1] = 0.0f;
    c2w.m[3][2] = 0.0f;
    c2w.RotateY(deg, 2);
    c2w.m[3][1] = oy;
    c2w.m[3][0] = ox;
    c2w.m[3][2] = oz;
    _UpdateW2C();
}


// v12 0x0056B050  (feb 0x00552D60)
// Removes pitch and roll: looks horizontally along the current heading (or +Z if looking straight up/down).
void camCAMERA::ResetVertRotation()
{
    m3dV dir = { c2w.m[2][0], 0.0f, c2w.m[2][2] };
    float l = m3dSqrt(dir.x * dir.x + dir.z * dir.z);
    if (m3dFAbs(l) < M3D_EPSILON) {
        dir.x = 0.0f; dir.y = 0.0f; dir.z = 1.0f;
    } else {
        m3dNormalize(&dir);
    }
    m3dV org = { c2w.m[3][0], c2w.m[3][1], c2w.m[3][2] };
    _Orient(&org, &dir);
}


// v12 0x0056B2C0  (feb 0x00552FD0)
// Looks at a world point (keeps the eye, Y up) and remembers it as the interest point.
void camCAMERA::SetInterest(m3dV *p)
{
    m3dV dir = { p->x - c2w.m[3][0], p->y - c2w.m[3][1], p->z - c2w.m[3][2] };
    m3dV org = { c2w.m[3][0], c2w.m[3][1], c2w.m[3][2] };
    _Orient(&org, &dir);
    interest = *p;
}


// v12 0x0056B460  (feb 0x00553170)
// Looks along a direction (keeps the eye, Y up).
void camCAMERA::SetLookAt(m3dV *dir)
{
    m3dV org = { c2w.m[3][0], c2w.m[3][1], c2w.m[3][2] };
    _Orient(&org, dir);
}


// v12 0x0056B5B0  (feb 0x005532C0)
// Camera up axis (world).
void camCAMERA::GetVUp(m3dV *out)
{
    out->x = c2w.m[1][0]; out->y = c2w.m[1][1]; out->z = c2w.m[1][2];
}


// v12 0x0056B5D0  (feb 0x005532E0)
// Camera right axis (world).
void camCAMERA::GetVRight(m3dV *out)
{
    out->x = c2w.m[0][0]; out->y = c2w.m[0][1]; out->z = c2w.m[0][2];
}


// v12 0x0056B5F0  (feb 0x00553300)
// View direction (world). The linker folded GetLookAt and GetVForw into one body.
void camCAMERA::GetLookAt(m3dV *out)
{
    out->x = c2w.m[2][0]; out->y = c2w.m[2][1]; out->z = c2w.m[2][2];
}


// v12 0x0056B5F0  (feb 0x00553300)
// View direction (world); identical COMDAT to GetLookAt.
void camCAMERA::GetVForw(m3dV *out)
{
    out->x = c2w.m[2][0]; out->y = c2w.m[2][1]; out->z = c2w.m[2][2];
}


// v12 0x0056B610  (feb 0x00553320)
// World point -> camera space (w2c, with translation).
void camCAMERA::TransformPoint(m3dV *in, m3dV *out)
{
    m3dTransformPoint(in, &w2c, out);
}


// v12 0x0056B790  (feb 0x005534A0)
// Camera-space point -> world.
void camCAMERA::TransformPointC2W(m3dV *in, m3dV *out)
{
    m3dTransformPoint(in, &c2w, out);
}


// v12 0x0056B910  (feb 0x00553620)
// World vector -> camera space (rotation only).
void camCAMERA::TransformVector(m3dV *in, m3dV *out)
{
    m3dTransformVector(in, &w2c, out);
}


// v12 0x0056BA60  (feb 0x00553770)
// Eye position.
void camCAMERA::GetOrigin(m3dV *out)
{
    out->x = c2w.m[3][0]; out->y = c2w.m[3][1]; out->z = c2w.m[3][2];
}


// v12 0x0056BA80  (feb 0x00553790)
// Replaces (op 0) or multiplies the camera matrix: op 1 = m*c2w (local), op 2 = c2w*m (global).
void camCAMERA::TransformMatrC2W(m3dMATR *m, int op)
{
    m3dMATR r;
    if (op == 0)
        r = *m;
    else if (op == 1)
        m3dMultiplyMatr(m, &c2w, &r);
    else if (op == 2)
        m3dMultiplyMatr(&c2w, m, &r);
    c2w = r;                                  // op outside 0..2 copies an uninitialised matrix, as compiled
    _UpdateW2C();
}


// v12 0x0056BC90  (feb 0x005539B0)
// Reads eye and axes; any pointer may be 0.
void camCAMERA::GetParameters(m3dV *org, m3dV *up, m3dV *right, m3dV *forw)
{
    if (org)   { org->x = c2w.m[3][0];   org->y = c2w.m[3][1];   org->z = c2w.m[3][2]; }
    if (up)    { up->x = c2w.m[1][0];    up->y = c2w.m[1][1];    up->z = c2w.m[1][2]; }
    if (right) { right->x = c2w.m[0][0]; right->y = c2w.m[0][1]; right->z = c2w.m[0][2]; }
    if (forw)  { forw->x = c2w.m[2][0];  forw->y = c2w.m[2][1];  forw->z = c2w.m[2][2]; }
}


// v12 0x0056BD00  (feb 0x00553A20)
// Sets eye and axes directly (axes are renormalised, not re-orthogonalised).
void camCAMERA::SetParameters(m3dV *org, m3dV *up, m3dV *right, m3dV *forw)
{
    c2w.Identity();
    c2w.m[0][0] = right->x; c2w.m[1][0] = up->x; c2w.m[2][0] = forw->x;
    c2w.m[0][1] = right->y; c2w.m[1][1] = up->y; c2w.m[2][1] = forw->y;
    c2w.m[0][2] = right->z; c2w.m[1][2] = up->z; c2w.m[2][2] = forw->z;
    c2w.m[3][0] = org->x;
    c2w.m[3][1] = org->y;
    c2w.m[3][2] = org->z;
    _UpdateW2C();
}


// v12 0x0056BF00  (feb 0x00553C20)
// Pixel (sx, sy) of the view window -> world direction of the ray through it (not normalised).
// HOOK: mouse picking / crosshair rays go through here.
void camCAMERA::GetViewWindowRayDir(float sx, float sy, m3dV *out)
{
    m3dV p;
    p.z = zNear;
    p.x = (sx - winX) / scaleX + vpX;
    p.y = ((winDy + winY) - sy) / scaleY + vpY;
    m3dTransformVector(&p, &c2w, out);
}


// v12 0x0056C090  (feb 0x00553DB0)
// Evaluates the camera splines at time t and applies them to dst (or to this camera when dst is 0):
// vertical FOV, eye position, look-at target.
void camCAMERA::Animate(float t, camCAMERA *dst)
{
    camCAMERA *c = dst ? dst : this;

    float v = angleVert;
    if (splAngleVert)
        splAngleVert->GetValue(t, &v, 0);
    c->angleVert = v;
    c->angleHor  = _camOtherAngle(v, 1.0f / c->aspectHW);      // inlined SetAngleVert
    c->SetViewportDx(_camNearSize(c->angleHor, c->zNear));
    c->SetViewportDy(_camNearSize(v, c->zNear));

    if (splPos) {
        m3dV p;
        splPos->GetValue(t, &p, 0);
        c->c2w.m[3][0] = p.x;
        c->c2w.m[3][1] = p.y;
        c->c2w.m[3][2] = p.z;
        c->_UpdateW2C();                                        // here not inlined: calls _camMatrInvert
    }

    if (splInterest) {
        m3dV target;
        splInterest->GetValue(t, &target, 0);
        c->SetInterest(&target);                                // inlined in the original
    }
}


// v12 0x0056C480  (feb 0x005541B0)
// In-place inverse of a rigid matrix: R^T with origin -org*R^T.
void __fastcall _camMatrInvert(m3dMATR *m)
{
    m3dMATR rt, t;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++)
            rt.m[i][j] = m->m[j][i];
        rt.m[i][3] = 0.0f;
    }
    rt.m[3][0] = 0.0f; rt.m[3][1] = 0.0f; rt.m[3][2] = 0.0f; rt.m[3][3] = 1.0f;
    t.Identity();
    t.Translate(-m->m[3][0], -m->m[3][1], -m->m[3][2], 0);
    m3dMultiplyMatr(&t, &rt, m);
}
