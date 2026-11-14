// Will Rock 1.2 unofficial SDK - video/render driver layer (THE porting boundary)
// objs: drv:vid.obj (vidDRIVER base), drv:D3d_coll.obj (enumeration), drv:D3d_drv.obj, d3d_cfg.obj, D3d_prep_cfg.obj,
//       D3d_rnd*.obj (d3dREND_DRIVER), d3d_postbuf.obj, d3d_txm.obj, d3d_misc.obj
// API: DirectDraw 7 + Direct3D 7 (IDirectDraw7 / IDirect3D7 / IDirect3DDevice7), hardware T&L required.
// Assumes from re/sdk/core: scnSCENE, objOBJ, animINST, camCAMERA, m3dV
// d3dDRIVER/vidDRIVER methods are exact in v12 for the ones listed exact; InitDrv (vtable match) re-checked by hand:
// v12 0x004BAE10 uses [esi+0x254], lea [esi+0x774], [esi+0x778], [esi+0x1D1C] exactly like feb.
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include <stdint.h>

class vidDRIVER; class d3dDRIVER; class d3dREND_DRIVER;
extern vidDRIVER *vidDriver;   // v12 0x006561C4 (feb 0x00611FE4) active video driver (97 xrefs)
extern d3dDRIVER *d3dDriver;   // v12 0x00656218 (feb 0x0061204C) same object typed as d3dDRIVER (324 xrefs)

// D3D state caches (d3d_cfg.obj) - redundant-state filters in front of SetRenderState/SetTextureStageState
extern uint8_t _d3dStateFloatCache[];      // v12 0x00656258 (feb 0x00612090)
extern uint8_t _d3dStageStateFloatCache[]; // v12 0x00656270 (feb 0x006120A8)
extern uint8_t _d3dStateCache[];           // v12 0x00656308 (feb 0x00612140)
extern uint8_t _d3dStageStateCache[];      // v12 0x00656518 (feb 0x00612350)
// vertex buffers (D3d_rnd_vbuf.obj, d3d_postbuf.obj)
extern uint8_t d3dVBMng[];                 // v12 0x006AC730 (feb 0x00668528) d3dVBUF_MNG
extern void   *d3dIdxVert;                 // v12 0x006567F0 (feb 0x00612620) d3dVERTEX_IDX[]
extern uint16_t d3dIdxFaceInd[];           // v12 0x006A2AD8 (feb 0x0065E8E8)
extern uint8_t _d3drPostBuf[];             // v12 0x008A59D8 (feb 0x00861798) _d3drPOST_BUF
extern uint8_t _d3drSdrVList[];            // v12 0x008DD480 (feb 0x00899240) d3dVERTEX_SDR[] (shader-processed verts)
extern uint8_t _d3drSdrRendVertMng[];      // v12 0x006F6B08 (feb 0x006B28C8)
extern uint8_t _d3drFaceDataBuf[];         // v12 0x006F6BD8 (feb 0x006B2998)
extern uint8_t _d3drSdrDataBuf[];          // v12 0x006F7EC8 (feb 0x006B3C88)
// _d3dObjDot3Color feb 0x0065BA08: v12 twin not confirmed (auto map gives 0x006869F8 with 0 xrefs)  // ?

int __fastcall  vidGetDriverList(vidDRIVER ***);      // v12 0x004B3B60 (feb 0x004A4F00) exact; GetDC/GetDeviceCaps/ReleaseDC for desktop bpp
int __fastcall  d3dEnumDevices();                     // v12 0x004B7F30 (feb 0x004A84F0) exact; LoadLibrary-less: GetModuleHandle("DDRAW.DLL") +
                                           //   GetProcAddress("DirectDrawEnumerateExA"), fallback DirectDrawEnumerateA
int __stdcall d3dEnumDDDeviceCB(void *guid, char *, char *, void *, void *hmon); // v12 0x004B8510 (feb 0x004A8AD0) calls DirectDrawCreateEx
int __stdcall d3dEnumDDDeviceCB_OLD(void *guid, char *, char *, void *);         // v12 0x004B84F0 (feb 0x004A8AB0)
// d3dEnumD3DDeviceCB feb 0x004A8E40, d3dEnumDisplayModesCB feb 0x004A9040, d3dEnumTextureFormatsCB feb 0x004A90D0: v12 ?
int __stdcall d3dEnumZBufferFormatsCB(void *pf, void *); // v12 0x004B8F30 (feb 0x004A94F0) exact

struct vidMODE {           // display mode record
    uint32_t caps;         // +0x00 bit 0x02 windowed-capable?, 0x04, 0x08  // ?
    uint32_t _04;
    int      width;        // +0x08
    int      height;       // +0x0C
    int      bpp;          // +0x10  // ?
    uint32_t flags;        // +0x14 SetMode flags: 1 fullscreen?, 2 no zbuffer/backbuffer?, 4, 8, 0x20  (low byte stored)  // ?
    // +0x94 z-buffer format index
};

class vidDRIVER {
public:
    // vtable v12 0x006066F8 (feb 0x005DB1B8), 34 slots
    virtual ~vidDRIVER();                                  // slot +0x00 v12 0x004B3D80 (feb 0x004A5120)
    virtual int  InitDrv();                                // slot +0x04 v12 0x004B3F30 (feb 0x004A52D0)
    virtual void TermDrv();                                // slot +0x08 v12 0x004B3F40 (feb 0x004A52E0)
    virtual int  SetMode(vidMODE *, void *hwnd, uint32_t flags); // slot +0x0C v12 0x004B3F50 (feb 0x004A52F0) exact
    virtual int  InitHW();                                 // slot +0x10 v12 0x004B4040 (feb 0x004A53E0)
    virtual void TermHW();                                 // slot +0x14 v12 0x004B4060 (feb 0x004A5400)
    virtual int  BeginScene();                             // slot +0x18 v12 0x004B4070 (feb 0x004A5410)
    virtual void EndScene();                               // slot +0x1C
    virtual void SwapBuffers();                            // slot +0x20 pure
    virtual void ClearBuffers(/*...*/);                    // slot +0x24 pure (RenderFrame calls vidDriver slot +0x24)
    virtual int  ReadBuffer(/*...*/);                      // slot +0x28 pure (screenshots)
    virtual void _2c();                                    // slot +0x2C pure
    virtual void CopyBuffer(/*...*/);                      // slot +0x30 pure
    virtual void SetNTexStageUse(int);                     // slot +0x34 v12 0x004B4020 (feb 0x004A53C0)
    virtual void SetCamNearPlane(float);                   // slot +0x38 v12 0x004B46B0 (feb 0x004A5A50)
    virtual void SetNearFarZValue(float, float);           // slot +0x3C v12 0x004B4660 (feb 0x004A5A00)
    virtual void GetNearFarZValue(float *, float *);       // slot +0x40 v12 0x004B4680 (feb 0x004A5A20)
    virtual float GetGamma();                              // slot +0x44 v12 0x004B46C0 (feb 0x004A5A60)
    virtual int  SetGamma(float);                          // slot +0x48 v12 0x004B46E0 (feb 0x004A5A80) range check then store +0x218
    virtual void ConfigureFog(/*...*/);                    // slot +0x4C pure
    virtual void ConfigureStencil(/*...*/);                // slot +0x50 pure
    virtual void InvalidateSignatureCFG();                 // slot +0x54
    virtual void ToggleMipMapFiltr(int);                   // slot +0x58..+0x60
    virtual void _5c(); virtual void _60();
    virtual void ToggleFog(int);                           // slot +0x64
    virtual void ToggleZCompare(int);                      // slot +0x68
    virtual void ToggleZWrite(int);                        // slot +0x6C
    virtual void ToggleWBuffer(int);                       // slot +0x70
    virtual void ToggleStencil(int);                       // slot +0x74
    virtual void ToggleFSAA(int);                          // slot +0x78
    virtual class txmMANAGER *CreateTexManager();          // slot +0x7C
    virtual class rendDRIVER *CreateRendDriver();          // slot +0x80
    virtual void DestroyRendDriver();                      // slot +0x84
    // non-virtual
    void ToggleModeParam(uint32_t mask, int on);           // v12 0x004B4080 (feb 0x004A5420) callgraph; used by RenderFrame to switch render-state groups

    uint32_t flags;        // v12 +0x04 (feb +0x04) 0x04 windowed allowed, 0x40 gamma ok, 0x100, 0x200, 0x10000, 0x1000000 HW initialised, 0x2000000 mode pending
    char     name[0x208];  // v12 +0x08 (feb +0x08) driver description (apLog "Creating Direct draw driver %s")  // ? size
    vidMODE *mode;         // v12 +0x210 (feb +0x210) current mode
    void    *hwnd;         // v12 +0x214 (feb +0x214) target window
    float    gamma;        // v12 +0x218 (feb +0x218)
};

class d3dDRIVER : public vidDRIVER {
public:
    // vtable v12 0x00606A30 (feb 0x005DB598), 34 slots, all verified (vtables_dump.txt). PORT: reimplement every slot.
    //  +0x04 InitDrv 0x004BAE10  +0x08 TermDrv 0x004BAEA0  +0x10 InitHW 0x004BAF90  +0x14 TermHW 0x004BB6C0
    //  +0x18 BeginScene 0x004BB790  +0x1C EndScene 0x004BB7D0  +0x20 SwapBuffers 0x004BB820  +0x24 ClearBuffers 0x004BBA00
    //  +0x28 ReadBuffer 0x004BBB10  +0x30 CopyBuffer 0x004BBBC0  +0x3C SetNearFarZValue 0x004BE590  +0x48 SetGamma 0x004BE5F0
    //  +0x4C ConfigureFog 0x004BE710  +0x50 ConfigureStencil 0x004BE140  +0x54 InvalidateSignatureCFG 0x004BBE50
    //  +0x58/5C/60 ToggleMipMapFiltr 0x004BE8F0  +0x64 ToggleFog 0x004BE920  +0x68 ToggleZCompare 0x004BE990
    //  +0x6C ToggleZWrite 0x004BE9F0  +0x70 ToggleWBuffer 0x004BEA50  +0x74 ToggleStencil 0x004BE520  +0x78 ToggleFSAA 0x004BEAD0
    //  +0x7C CreateTexManager 0x004BAF50  +0x80 CreateRendDriver 0x004BBED0  +0x84 DestroyRendDriver 0x004BBF00   (all v12)
    void ClearSurfaces(/*...*/);       // feb 0x004AC440 (ClientToScreen/SetRect for windowed blits)

    void    *ddGuid;           // v12 +0x254 (feb +0x254) GUID* passed to DirectDrawCreateEx
    // +0x5A4 z-buffer pixel format flags; +0x6D0 z-buffer formats[] (0x20 each)
    void    *pDD7;             // v12 +0x774 (feb +0x774) IDirectDraw7*
    void    *pD3D7;            // v12 +0x778 (feb +0x778) IDirect3D7* (QueryInterface IID_IDirect3D7)
    void    *pPrimary;         // v12 +0x780 (feb +0x780) IDirectDrawSurface7* primary/front
    void    *pBack;            // v12 +0x784 (feb +0x784) back buffer (GetAttachedSurface)
    uint8_t  zbufDesc[0x20];   // v12 +0x788 (feb +0x788)
    uint8_t  primDesc[0x7C];   // v12 +0xF8C (feb +0xF8C) DDSURFACEDESC2 of primary (GetSurfaceDesc)
    uint8_t  backDesc[0x7C];   // v12 +0x1008 (feb +0x1008) DDSURFACEDESC2 of back buffer
    void    *pGammaCtl;        // v12 +0x1118 (feb +0x1118) IDirectDrawGammaControl*  // ?
    uint8_t  ddCaps[0x7C];     // v12 +0x1D1C (feb +0x1D1C) DDCAPS (dwSize = 0x7C), filled by IDirectDraw7::GetCaps (vtbl +0x30)
};

class d3dREND_DRIVER /* : public rendDRIVER */ {
    // vtable v12 0x00606BA0 (feb 0x005DB708), 46 slots (44 rendDRIVER + 2 new), all verified. PORT: this is the draw API.
    //  +0x00 dtor 0x004C1EC0  +0x04 InitHW 0x004C1EF0  +0x08 TermHW 0x004C1FF0  +0x10 BeginFrame 0x004C2180  +0x14 EndFrame 0x004C21D0
    //  +0x1C FilterInst 0x004BBF70  +0x20 PreDrawObj 0x004C31D0  +0x28 PreDrawScene 0x004C2E20  +0x2C PurgeSWBuffer 0x004BBF60
    //  +0x30 BeginCamera 0x004BF4E0  +0x34 EndCamera 0x004BF810  +0x38 GetVListPtr 0x004C5020  +0x3C DrawFace 0x004C3770
    //  +0x40 DrawPoly 0x004BBFC0  +0x44 DrawLine 0x004BC180  +0x4C DrawIndexedPrimitive 0x004C23E0  +0x50 Configure 0x004C2D80
    //  +0x54 ConfigureTransp 0x004C2D50  +0x58 FlushHWQueue 0x004BBF20  +0x60 ConfigureClipPlane 0x004BF160  +0x64 EnableClipPlane 0x004BF210
    //  +0x68 SetMtlCoeff 0x004BF340  +0x6C SetLight 0x004BF410  +0x70 EnableLight 0x004BF4B0  +0x74 InvalidateObjDynVBuf 0x004C2620
    //  +0x78 SetVBStatFrames 0x004C2A40  +0x7C InvalidateSignCFG 0x004C2C80  +0x80 ObjAllocPrepCFG 0x004C58B0
    //  +0x88 InvalidatePrepCFG(scn) 0x004C5890  +0x8C ValidatePrepCFG 0x004C6030  +0x90 ValidatePrepCFG 0x004C2C90  +0x94 IsValidPrepCFG 0x004C5C70
    //  +0x98 InitLevel 0x004C26C0  +0x9C TermLevel 0x004C2760  +0xA0 InstLoadNotify 0x004C2900  +0xA4 InstDestroyNotify 0x004C2910
    //  +0xA8 ObjLoadNotify 0x004C2890  +0xAC ObjDestroyNotify 0x004C28C0  +0xB0 ObjInitHWNotify 0x004C29B0  +0xB4 ObjTplInitHWNotify 0x004C2920
    //  (all v12; feb addresses in vtables_dump.txt)
};
