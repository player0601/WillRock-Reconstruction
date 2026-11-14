// camera.h -- Will Rock 1.2 unofficial SDK, math_app: camCAMERA (m3d.lib: cam.obj, cam_trans.obj, cam_qual.obj)
//
// The full class (layout verified by the rebuilt m3d sources, src_recon/src/m3d/cam*.cpp).
// Conventions: camera space looks down +Z (ProjPoint: x' = x/z*near), Y up, X right.
// c2w rows: X = right, Y = up, Z = forward (view direction), origin = eye. The world is right-handed, so the
// game's c2w has a mirrored basis (GetDirRight of an instance is -X).
#pragma once
#include "m3d.h"

// Frustum ("point of view") data rebuilt by CalcPOVQual; used by the Qual*POV visibility tests.
// Return convention of every Qual*POV / _camSeparate*: 1 = completely outside (culled),
// -1 = completely inside, 0 = intersects.
struct camPOV_QUAL {                       // camCAMERA +0x16C, size 0x90
    float cornerDist;                      // +0x00 (cam +0x16C) |eye -> corner of the near rectangle|
    float planeD[5];                       // +0x04 (cam +0x170) plane offsets: 4 sides = 0, near = n.eye*cornerDist ?
    m3dV  cornerRay[4];                    // +0x18 (cam +0x184) world directions eye -> near-rect corners (normalised)
    m3dV  planeN[5];                       // +0x48 (cam +0x1B4) side normals cornerRay[i] x cornerRay[i+1], [4] = -forward
    m3dV  eye;                             // +0x84 (cam +0x1F0) eye position (GetOrigin)
};

class camCAMERA {
public:
    m3dMATR  c2w;                          // +0x000 camera -> world (rows right/up/forward/eye)
    m3dMATR  w2c;                          // +0x040 world -> camera (inverse of c2w, kept in sync)
    float    zNear;                        // +0x080 default 0.1
    float    zFar;                         // +0x084 default 100000
    m2dPOLY  viewPoly;                     // +0x088 near-plane viewport rectangle (4 verts, camera units)
    float    vpX, vpY, vpDx, vpDy;         // +0x12C viewport rectangle on the near plane
    float    winX, winY, winDx, winDy;     // +0x13C screen window in pixels (default 0,0,640,480)
    float    angleHor;                     // +0x14C horizontal FOV, degrees (default 80)
    float    angleVert;                    // +0x150 vertical FOV, degrees
    float    aspectHW;                     // +0x154 height/width (default 0.75)
    float    scaleX, scaleY;               // +0x158 winDx/vpDx, winDy/vpDy (near plane -> pixels)
    m3dV     interest;                     // +0x160 last SetInterest point
    camPOV_QUAL pov;                       // +0x16C
    m3dSPL  *splAngleVert;                 // +0x1FC animated vertical FOV (1D)   -- deleted by the dtor
    m3dSPL  *splUnused;                    // +0x200 ? deleted by the dtor, never evaluated
    m3dSPL  *splPos;                       // +0x204 animated eye position (3D)
    m3dSPL  *splInterest;                  // +0x208 animated look-at target (3D)
    // size 0x20C

    camCAMERA();                                             // v12 0x00569B00
    ~camCAMERA();                                            // v12 0x00569DC0
    void SetViewportDx(float dx);                            // v12 0x00569E10
    void SetViewportDy(float dy);                            // v12 0x00569E70
    void SetAspectRatioHW(float a);                          // v12 0x00569EF0
    void SetAngleHor(float deg);                             // v12 0x00569FF0
    void SetAngleVert(float deg);                            // v12 0x0056A120
    void SetNearPlane(float n);                              // v12 0x0056A250
    void SetViewWindow(float x, float y, float dx, float dy);// v12 0x0056A350
    void SetViewPort(float x, float y, float dx, float dy);  // v12 0x0056A3A0
    void MoveTo(m3dV *p);                                    // v12 0x0056A440
    void MoveRelForw(float d);                               // v12 0x0056A5F0
    void MoveRelSide(float d);                               // v12 0x0056A7A0
    void MoveRelUp(float d);                                 // v12 0x0056A950
    void MoveRel(m3dV *d);                                   // v12 0x0056AB00
    void TurnRelAroundX(float deg);                          // v12 0x0056ACC0
    void TurnAroundVert(float deg);                          // v12 0x0056AE70
    void ResetVertRotation();                                // v12 0x0056B050
    void SetInterest(m3dV *p);                               // v12 0x0056B2C0
    void SetLookAt(m3dV *dir);                               // v12 0x0056B460
    void GetVUp(m3dV *out);                                  // v12 0x0056B5B0
    void GetVRight(m3dV *out);                               // v12 0x0056B5D0
    void GetLookAt(m3dV *out);                               // v12 0x0056B5F0 (folded with GetVForw)
    void GetVForw(m3dV *out);                                // v12 0x0056B5F0
    void TransformPoint(m3dV *in, m3dV *out);                // v12 0x0056B610 world -> camera
    void TransformPointC2W(m3dV *in, m3dV *out);             // v12 0x0056B790 camera -> world
    void TransformVector(m3dV *in, m3dV *out);               // v12 0x0056B910 world -> camera (no translation)
    void GetOrigin(m3dV *out);                               // v12 0x0056BA60
    void GetPos(m3dV *out) { GetOrigin(out); }               // ? name used by re/mpsrc (gs_mp_misc) for the eye position
    void TransformMatrC2W(m3dMATR *m, int op);               // v12 0x0056BA80
    void GetParameters(m3dV *org, m3dV *up, m3dV *right, m3dV *forw);  // v12 0x0056BC90
    void SetParameters(m3dV *org, m3dV *up, m3dV *right, m3dV *forw);  // v12 0x0056BD00
    void GetViewWindowRayDir(float sx, float sy, m3dV *out); // v12 0x0056BF00 screen pixel -> world ray
    void Animate(float t, camCAMERA *dst);                   // v12 0x0056C090

    // cam_trans.obj
    void ProjPoly(m3dPOLY *in, m2dPOLY *out);                // v12 0x00579E10
    void ProjPoint(m3dV *in, m2dV *out);                     // v12 0x00579E70
    BOOL32 BackProjPoly(m2dPOLY *in, m3dPOLY *plane, m3dPOLY *out, m3dV *n, int flipN); // v12 0x00579EA0
    static void   __fastcall ProjPolyOrtho(m3dPOLY *in, m2dPOLY *out);                  // v12 0x00579F80
    static BOOL32 __fastcall BackProjPolyOrtho(m2dPOLY *in, m3dPOLY *plane, m3dPOLY *out, m3dV *n, int flipN); // v12 0x00579FC0

    // cam_qual.obj
    void CalcPOVQual();                                      // v12 0x0057CA90
    int  QualBoxPOV(m3dBOX *b);                              // v12 0x0057CF80
    int  QualOBBPOV(m3dOBB *b);                              // v12 0x0057D5D0
    int  QualPrismPOV(m3dPRISM *p);                          // v12 0x0057DE60
    int  QualSpherePOV(m3dSPHERE *s);                        // v12 0x0057E0E0

private:
    void _ViewportFromAngles();                              // inline in the original (SetViewportDx/Dy pair)
    void _UpdateW2C();                                       // inline in the original (normalise + _camMatrInvert)
    void _Orient(m3dV *org, m3dV *dir);                      // inline in the original (SetLookAt body)
};

void __fastcall _camMatrInvert(m3dMATR *m);                 // v12 0x0056C480 rigid inverse in place

int __fastcall _camSeparateBBoxPOV(camPOV_QUAL *q, m3dV *axis, m3dV *c, m3dV h[3], m3dV *eye, m3dV ray[4], float dist);  // v12 0x0057D210
int __fastcall _camSeparateOBBPOV (camPOV_QUAL *q, m3dV *axis, m3dV *c, m3dV h[3], m3dV *eye, m3dV ray[4], float dist);  // v12 0x0057D810
int __fastcall _camSeparateOBBPOV_AxisPOV(camPOV_QUAL *q, int plane, m3dV *c, m3dV h[3]);                                // v12 0x0057DC40

