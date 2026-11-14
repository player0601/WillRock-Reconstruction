// Will Rock 1.2 unofficial SDK - engine render: rendDRIVER (engine-side renderer front end)
// objs: engine:Rend_ini.obj, Rend_obj.obj, Rend_sort.obj, rend_vis.obj, Rend_anim.obj
//
// Assumes from re/sdk/core: objOBJ, animINST, animTPL, scnSCENE (scene.h), camCAMERA, m3dV, m3dMATR,
//                           m3dPOLY, m2dV, m3dSPHERE, objUSR_DATA, lstLIST/lstNODE
// Offsets: "v12 +0xNN (feb +0xNN)". All rendDRIVER methods listed as "exact" are byte-identical
// (addresses masked) in v12, so the offsets they use are verified unchanged.
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include <stdint.h>

// ---------------------------------------------------------------- globals
class rendDRIVER;
extern rendDRIVER *rendDrv;            // v12 0x00978728 (feb 0x009344EC)  the active renderer (a d3dREND_DRIVER), 168 xrefs in v12

// ---------------------------------------------------------------- flags (rendDRIVER::flags, +0x04)
enum rendDRIVER_FLAGS {
    REND_F_ENABLED_INIT   = 0x001,  // set by ctor
    REND_F_CLIPPLANE      = 0x010,  // user clip plane enabled (BeginCamera transforms +0x0C/+0x18 to camera space)
    REND_F_DISABLED       = 0x080,  // tested as "(char)flags < 0" in gsRENDER_SYSTEM::RenderFrame: skip whole scene render  // ?
    REND_F_NO_WORLD       = 0x100,  // RenderFrame: skip mirrors, vis, shadows, particles (menu/backdrop mode)  // ?
    REND_F_2PASS          = 0x400,  // "VIDEO_2PassRend" (gs_rend strings)  // ?
};

// render sort/queue buffers --------------------------------------------------------------
// rnsMNG::Purge(int pass, int bucket): bucket 0..2 = transparent types, 3 = after particles, 4 = opaque
struct rnsBUF_OPAQUE  { uint8_t raw[0x324]; };      // size 0x324 (feb ctor loop stride) v12 ctor 0x004EAD20 exact
struct rnsBUF_TRANSP  { uint8_t raw[0x40];  };      // size 0x40, v12 ctor 0x004EAD70 exact

struct rnsMNG {                                      // embedded in rendDRIVER at +0x8C
    int            pass;                             // v12 +0x00 (feb +0x00) ctor writes 1  // ?
    rnsBUF_OPAQUE  opaque[2];                        // v12 +0x04 (feb +0x04)  (rendDRIVER +0x90)
    rnsBUF_TRANSP  transp[2];                        // v12 +0x64C (feb +0x64C) (rendDRIVER +0x6D8)
    // ... up to rendDRIVER +0x1E04

    void InitLevel(scnSCENE *);                      // v12 0x00503EC0 (feb 0x004F1850) callgraph
    void StoreObj(objOBJ *);                         // v12 0x005040A0 (feb 0x004F1A30) queue an object for this frame
    void Purge(int pass, int bucket);                // v12 0x00504370 (feb 0x004F1D60) draw+flush one bucket
    int  IsEmpty(int pass);                          // v12 0x00504590 (feb 0x004F20B0) exact
    void StorePoly(int n, struct rendVERTEX *, struct rendCFG *); // v12 0x005045F0 (feb 0x004F2110) exact (sorted transparent polys)
    void PurgePoly();                                // v12 0x00504680 (feb 0x004F21A0) exact
    void ObjDestroyNotify(objOBJ *);                 // v12 0x00504700 (feb 0x004F2220)
    void UnLinkObj(objOBJ *);                        // v12 0x005049E0 (feb 0x004F2460) exact
    void UnLinkInst(animINST *);                     // v12 0x00504C60 (feb 0x004F2650)
};

// ---------------------------------------------------------------- rendDRIVER
// size >= 0x1E08 (ctor clears +0x1E04). Abstract: the real object is d3dREND_DRIVER (d3d_driver.h).
class rendDRIVER {
public:
    // vtable v12 0x00607608 (feb 0x005DC1F0), 44 slots, same order in both builds.
    // Slot names taken from the d3dREND_DRIVER override (v12 0x00606BA0) where the base slot is a stub.
    virtual ~rendDRIVER();                                           // slot +0x00 v12 0x004EA450 (feb 0x004DA790)
    virtual int  InitHW(scnSCENE *);                                 // slot +0x04 v12 0x004EA540 (feb 0x004DA880)
    virtual void TermHW();                                           // slot +0x08 v12 0x0051AAF0 (feb 0x00403BE0) empty stub in base
    virtual void SetBVolUse(int);                                    // slot +0x0C v12 0x004C2C70 (feb 0x004B3310)
    virtual int  BeginFrame(scnSCENE *);                             // slot +0x10 v12 0x004EA9A0 (feb 0x004DACE0)
    virtual void EndFrame();                                         // slot +0x14 v12 0x004EA9C0 (feb 0x004DAD00)
    virtual int  FilterObj(objOBJ *);                                // slot +0x18 v12 0x00403200 (feb 0x004036B0) stub
    virtual int  FilterInst(animINST *);                             // slot +0x1C v12 0x00403200 (feb 0x004036B0) stub
    virtual void PreDrawObj(objOBJ *);                               // slot +0x20 v12 0x004036D0 (feb 0x00403390) stub
    virtual void PostDrawObj(objOBJ *);                              // slot +0x24 v12 0x004036D0 (feb 0x00403390) stub  // ? name
    virtual void PreDrawScene();                                     // slot +0x28 v12 0x00403A20 (feb 0x00403690) stub, called at end of BeginCamera
    virtual void PurgeSWBuffer();                                    // slot +0x2C v12 0x00403A20 (feb 0x00403690) stub
    virtual void BeginCamera(camCAMERA *, float zNear, float zFar);  // slot +0x30 v12 0x004EA5A0 (feb 0x004DA8E0)
    virtual void EndCamera();                                        // slot +0x34 v12 0x004EA690 (feb 0x004DA9D0)
    virtual void *GetVListPtr(int n) = 0;                            // slot +0x38 pure
    virtual int  DrawFace(/*...*/) = 0;                              // slot +0x3C pure
    virtual int  DrawPoly(int n, struct rendV3D *, struct rendCFG *) = 0; // slot +0x40 pure
    virtual void DrawLine(/*...*/);                                  // slot +0x44 v12 0x00403940 (feb 0x0057BCE0) stub
    virtual int  DrawPrimitive(void *v, int n, rendCFG *);           // slot +0x48 v12 0x004EADA0 (feb 0x004DB0C0)
    virtual int  DrawIndexedPrimitive(void *v, int nv, uint16_t *idx, int ni, rendCFG *); // slot +0x4C v12 0x004EADB0 (feb 0x004DB0D0)
    virtual void Configure(rendCFG *);                               // slot +0x50 v12 0x005AA550 (feb 0x0057C690) stub
    virtual void ConfigureTransp(camCAMERA *);                       // slot +0x54 v12 0x0051AAF0 (feb 0x004CDA70) stub
    virtual void FlushHWQueue();                                     // slot +0x58 v12 0x00403A20 (feb 0x00403690) stub
    virtual void PurgeSWBuffer2();                                   // slot +0x5C v12 0x00403A20 (feb 0x00403690) stub
    virtual void ConfigureClipPlane(m3dV *org, m3dV *nrm);           // slot +0x60 v12 0x004EA600 (feb 0x004DA940)
    virtual void EnableClipPlane(int);                               // slot +0x64 v12 0x004EA670 (feb 0x004DA9B0)
    virtual void SetMtlCoeff(/*...*/);                               // slot +0x68 v12 0x00403950 (feb 0x00403870) stub
    virtual void SetLight(void *light15dw);                          // slot +0x6C v12 0x00403950 stub (InitLevel passes +0x3C block)
    virtual void EnableLight(int);                                   // slot +0x70 v12 0x00403950 stub
    virtual void InvalidateObjDynVBuf(objOBJ *);                     // slot +0x74 v12 0x00403950 stub
    virtual void SetVBStatFrames(int);                               // slot +0x78 v12 0x0051B6B0 (feb 0x0050DF70) stub
    virtual void InvalidateSignCFG();                                // slot +0x7C v12 0x00403A20 stub
    virtual void *ObjAllocPrepCFG(objOBJ *);                         // slot +0x80 v12 0x0048D590 (feb 0x004C46B0) stub (returns 0)
    virtual void InvalidatePrepCFG(objOBJ *);                        // slot +0x84 v12 0x004EAA30 (feb 0x004DAD70)
    virtual void InvalidatePrepCFG(scnSCENE *);                      // slot +0x88 v12 0x004EA9E0 (feb 0x004DAD20)
    virtual void ValidatePrepCFG(objOBJ *, int, struct rendCFG_PREP *); // slot +0x8C v12 0x004EAB20 (feb 0x004DAE60)
    virtual void ValidatePrepCFG(objOBJ *);                          // slot +0x90 v12 0x004EAAB0 (feb 0x004DADF0)
    virtual int  IsValidPrepCFG(objOBJ *);                           // slot +0x94 v12 0x004EAB60 (feb 0x004DAEA0)
    virtual void InitLevel(scnSCENE *);                              // slot +0x98 v12 0x004EA6A0 (feb 0x004DA9E0)
    virtual void TermLevel(scnSCENE *);                              // slot +0x9C v12 0x004EA7A0 (feb 0x004DAAE0)
    virtual void InstLoadNotify(animINST *);                         // slot +0xA0 v12 0x004EA7B0 (feb 0x004DAAF0)
    virtual void InstDestroyNotify(animINST *);                      // slot +0xA4 v12 0x0051AAF0 (feb 0x004CDA70) stub
    virtual void ObjLoadNotify(objOBJ *);                            // slot +0xA8 v12 0x004EA8E0 (feb 0x004DAC20)
    virtual void ObjDestroyNotify(objOBJ *);                         // slot +0xAC v12 0x004EA980 (feb 0x004DACC0)

    // ---- non-virtual (all exact matches unless noted)
    rendDRIVER();                                                    // v12 0x004EA350 (feb 0x004DA690)
    void RenderObjListStatic(int n, objOBJ **list, int noWorld);     // v12 0x004FBE20 (feb 0x004E9910) draw the static level geometry list (scene +0x48/+0x4C)
    int  RenderObj(objOBJ *, int, int *);                            // v12 0x004FC1E0 (feb 0x004E9CD0)
    int  QualObj(objOBJ *, int, int *);                              // v12 0x004FC400 (feb 0x004E9EF0) visibility/frustum qualification
    int  _RenderObj(objOBJ *, int);                                  // v12 0x004FCBF0 (feb 0x004EA6C0) callgraph
    int  _RenderObj(objOBJ *, int, struct objSPLIT_ENTRY *);         // v12 0x004FCC70 (feb 0x004EA730)
    void RenderInst(animINST *, int);                                // v12 0x0059B900 (feb 0x0057FEE0) callgraph; called per instance by RenderFrame. HOOK: per-instance draw
    int  QualInst(animINST *, int *);                                // v12 0x0059BA70 (feb 0x0057FFD0) callgraph; instance visibility test
    void RenderObjListAnim(int n, objOBJ **, int);                   // v12 0x0059BCF0 (feb 0x00580250) exact
    int  ProjectPoint(camCAMERA *, m3dV *, m3dV *, m2dV *);          // v12 0x004FC020 (feb 0x004E9B10) world -> screen
    void RenderSphere(camCAMERA *, m3dSPHERE *, uint32_t argb);      // v12 0x004E8650 (feb 0x004D89C0) debug draw
    void RenderLine(camCAMERA *, m3dV *, m3dV *, uint32_t argb);     // v12 0x004E87E0 (feb 0x004D8B50) debug draw
    void RenderVector(camCAMERA *, m3dV *, m3dV *, float, uint32_t); // v12 0x004E89E0 (feb 0x004D8D50)
    void RenderCross(camCAMERA *, m3dV *, float, uint32_t);          // v12 0x004E8A50 (feb 0x004D8DC0)
    void RenderCS(camCAMERA *, float *matr, float len);              // v12 0x004E8AC0 (feb 0x004D8E30) the axes of a matrix
    int  RenderPoly(camCAMERA *, m3dPOLY *, int, rendCFG *);         // v12 0x004E9600 (feb 0x004D9940)
    int  _RenderPolyCompat(camCAMERA *, int, int, int scrDX, int scrDY, int n, struct rendVERTEX *, rendCFG *); // v12 0x004E9670 (feb 0x004D99B0) -- not rebuilt
    int  RenderPolyCompat(camCAMERA *, int n, struct rendVERTEX *, rendCFG *); // v12 0x004E9E70 (feb 0x004DA1B0) 2D/HUD-style polys
    void RenderQuad(camCAMERA *, float,float,float,float,float,float,float,float,float, rendCFG *); // v12 0x004EA0B0 (feb 0x004DA3F0)

    // ---- data
    // +0x00 vtable
    uint32_t flags;           // v12 +0x04 (feb +0x04) rendDRIVER_FLAGS; RenderFrame reads it (v12 0x0051BA50)
    int      enabled;         // v12 +0x08 (feb +0x08) ctor = 1
    m3dV     clipOrg;         // v12 +0x0C (feb +0x0C) clip plane point (world)
    m3dV     clipNrm;         // v12 +0x18 (feb +0x18) clip plane normal (world)
    m3dV     clipOrgCam;      // v12 +0x24 (feb +0x24) camera space copy (BeginCamera)
    m3dV     clipNrmCam;      // v12 +0x30 (feb +0x30)
    uint32_t light[15];       // v12 +0x3C (feb +0x3C) default directional light block: +0x40 pos, +0x4C dir, +0x58..+0x74 8 colour floats (255.0)  // ? exact meaning
    int      usrDataId;       // v12 +0x78 (feb +0x78) objOBJ::RegisterUsrData('RNDC')
    uint8_t  _7c[8];          // v12 +0x7C (feb +0x7C)
    camCAMERA *camera;        // v12 +0x84 (feb +0x84) set by BeginCamera
    int      _88;             // v12 +0x88 (feb +0x88)
    rnsMNG   sort;            // v12 +0x8C (feb +0x8C) render sort manager (RenderFrame: lea ecx,[rendDrv+0x8C] at v12 0x0051BCE0)
    // ...
    int      _1e04;           // v12 +0x1E04 (feb +0x1E04)
};

// rendCFG: per-material render configuration passed to the Draw* slots
struct rendMTL;
// rendVERTEX: a screen-space vertex of RenderPolyCompat (stride 0x30, from uiRECT::Render / uiFONT::PrintW)
struct rendVERTEX {
    float x, y, z, rhw;                        // +0x00 pixels, z 0.1, rhw 1 (the UI)
    unsigned _10, _14;                         // +0x10
    float u, v;                                // +0x18
    float _20[4];                              // +0x20
};
struct rendCFG {                               // 0xC0
    unsigned char _00[0xB8];                   // +0x00 materials (InitMtl1 sets material 1's texture)
    unsigned flags;                            // +0xB8 0x80AF rects, 0x4080AF font letters  // ? bits
    unsigned color;                            // +0xBC ARGB
    void   InitMtl1(class txmTEXTURE *);       // v12 0x004EABD0 (feb 0x004DAF10) callgraph
    rendMTL *FindMtlRendMtl(int);              // v12 0x004EABF0 (feb 0x004DAF30) exact
};
