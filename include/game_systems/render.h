// render.h -- Will Rock 1.2 unofficial SDK, game_systems: gsRENDER_SYSTEM frame driver (gs:gs_rend)
// Assumes from re/sdk/core + engine_render: rendDRIVER *rendDrv, vidDRIVER *vidDriver (the D3D8 back-end lives there),
//   camCAMERA, scnSCENE, pteMANAGER *pteManager, txmMANAGER *txmManager, pshSHADOW_SYSTEM *gsSysProjShd, vidCLEAR.
// gsRENDER_SYSTEM is NOT a msgADDR. Layout identical in both builds (ctor v12 0x0051B6C0 == feb 0x005062D0). Object = wrRENDER_SYSTEM (0x3C).
#pragma once
#include "gs_common.h"

enum gsREND_FLAGS {               // gsRENDER_SYSTEM::flags (+0x04)
    GSREND_IN_FRAME   = 0x01,     // set by BeginFrame, cleared by EndFrame
    GSREND_FIRST      = 0x04,     // set in ctor: first SwapBuffers skips present
    GSREND_10         = 0x10,     // cleared by BeginFrame/SwapBuffers  // ?
};
class gsRENDER_SYSTEM {           // vtbl v12 0x00606260 (feb 0x005DAD38); wrRENDER_SYSTEM vtbl v12 0x00606198 (feb 0x005DAC70)
public:
    unsigned flags;               // +0x04 v12 (feb +0x04) gsREND_FLAGS
    int      _08, _0C, _10;       // +0x08..+0x10
    float    fullScrColor[4];     // +0x14 v12 (feb +0x14) r, g, b, a 0..255 (SetColorFullScr): the flash / fade overlay
    float    fullScr24, fullScr28;// +0x24 / +0x28 (RenderColorFullScr compares +0x28) // ?
    int      fullScrOn;           // +0x2C SetColorFullScr 1 // ?
    int      fullScrSet;          // +0x30 SetColorFullScr 1: drawn this frame (RenderColorFullScr clears it after drawing)
    // wrRENDER_SYSTEM: +0x34..+0x3B // ?

    virtual ~gsRENDER_SYSTEM();                       // slot +0x00 v12 0x004AC330
    virtual int  Init();                              // slot +0x04 v12 0x0051B6F0 (feb 0x00506300) (wr: v12 0x004ABA90)
    virtual void Term();                              // slot +0x08 (empty)
    virtual int  InitLevel();                         // slot +0x0C v12 0x0051B720 (feb 0x00506330)
    virtual void TermLevel();                         // slot +0x10 (empty)
    virtual void BeginFrame();                        // slot +0x14 v12 0x0051B750 (feb 0x00506360) if gsAppState&0x40000: rendDrv->BeginScene(scene),
                                                      //   shaders+particles update (skipped when gsAppState&0x2004000), then SwapBuffers
    virtual void EndFrame(int updAnimSnd);            // slot +0x18 v12 0x0051B990 (feb 0x005065A0) rendDrv->End, per-inst anim sounds
    virtual void RenderFrame(camCAMERA *c, int);      // slot +0x1C v12 0x0051BA50 (feb 0x00506640) (wr: v12 0x004ABB30) HOOK: whole 3D frame
    virtual void SwapBuffers();                       // slot +0x20 v12 0x0051B7D0 (feb 0x005063E0) INI [Video] VIDEO_2PassRend, Wait_VSync,
                                                      //   Wait_Pending, Gamma_Changes, Gamma; vidDriver->Present(+0x20)/Clear(+0x24)
    virtual void RenderSky(camCAMERA *c);             // slot +0x24 v12 0x0051AAF0 (empty in gs)
    virtual void HideTitle();                         // slot +0x28 v12 0x0051B740 (feb 0x00506350) (wr: v12 0x004ABB60)
    virtual void slot2C();                            // slot +0x2C (empty)
    virtual void RenderSky2(camCAMERA *c);            // slot +0x30 (wr: RenderSky v12 0x004ABB50)
    virtual void RenderPostEff(camCAMERA *c);         // slot +0x34 (wr: v12 0x004ABB40) HOOK: post effects
    virtual void slot38(camCAMERA *c);                // slot +0x38 (empty)
    virtual void RenderSetOptions();                  // slot +0x3C v12 0x0051BF30 (feb 0x00506AD0) (wr: v12 0x004ABB70)
    virtual void _SaveScreenShot(camCAMERA *c);       // slot +0x40 v12 0x0051BF50 (feb 0x00506AF0)
    virtual void RenderColorFullScr(camCAMERA *c);    // slot +0x44 v12 0x0051C160 (feb 0x00506CF0)
    virtual void slot48();                            // slot +0x48 (empty)
    void Shake(unsigned argb, int on, float time);    // v12 0x0051C0D0 (name ours) empty in 1.2 (an explosion hit calls it)
    void SetColorFullScr(unsigned argb);              // v12 0x0051C0E0 (feb 0x00506C70) HOOK: damage/pickup screen flash
};
// gscCURSOR: the crosshair / mouse cursor of the game systems (src/gs/gs_cursor.cpp). The player's aim ray
// (plrPLAYER::UpdateCursor) writes its result here too. Global gscCursor v12 0x0098EA94.
struct gscCURSOR {
    u32       flags;                      // +0x00 2 = locked at lockX / lockY (Lock)
    int       cursor;                     // +0x04 the cursor in use (wrCURSORS: type + 2 * crosshair)
    float     x, y;                       // +0x08 screen position (the aim ray goes through it)
    m3dV      aimPoint;                   // +0x10 where the aim ray hits (plrPLAYER::UpdateCursor)
    u32       hide;                       // +0x1C one bit per reason to hide it (SetHide)
    float     lockX, lockY;               // +0x20 (Lock, in UI units)
    u32       color;                      // +0x28 ARGB: the target's health (green / yellow / red), else grey
    float     scale;                      // +0x2C smaller with the distance of what it points at
    entENTITY *aimEnt;                    // +0x30 the entity under it
    cdtINFO  *aimInfo;                    // +0x34 the aim ray's hit (gscInit allocates it)
    void Lock(float x, float y);          // v12 0x00513E50 (feb 0x00500E40)
    void SetHide(int show, int bit);      // v12 0x00513F10 (1.2 only, name ours) show: clear the bit, else set it
    void UpdatePosMouse();                // v12 0x00513EC0 (the mouse cursor; the locked crosshair stays put)
    void Draw(camCAMERA *cam);            // v12 0x00513F40 (gssREND_SYSTEM::ProcessMsg) the screen quad
};
struct gscCURSOR_TEX { float hotX, hotY, scale; txmTEXTURE *tex; };  // 0x10, gscRegisterCursor
extern gscCURSOR      gscCursor;          // v12 0x0098EA94
extern int            gscNCursors;        // v12 0x0098EA8C
extern gscCURSOR_TEX *gscCursors;         // v12 0x0098EACC
int  __fastcall gscInit();                // v12 0x00513C50
void __fastcall gscTerm();                // v12 0x00513CC0
void __fastcall gscAllocCursors(int n);   // v12 0x00513D20 (feb 0x00500D10)
void __fastcall gscFreeCursors();         // v12 0x00513D80
txmTEXTURE *__fastcall gscGetCursorTexture(int i);  // v12 0x00513DB0
int  __fastcall gscRegisterCursor(int i, char *tex, float hotX, float hotY, float scale); // v12 0x00513DC0 (feb 0x00500DB0)
void __fastcall gssUCoord2SCoord(m2dV *in, m2dV *out);   // v12 0x00535580 UI units -> screen pixels
extern float gssCamAngleHor, gssCamAspectHW;   // v12 0x00636BD8 75 / 0x00636BDC 0.75 the game camera (gs_scr2d.cpp)
extern int gssScreenDX, gssScreenDY;      // v12 0x009A57D0 / 0x009A57D4 (gs_scr2d.cpp) the screen in pixels

class gsRENDER_EMITTER_VIS {      // vtbl v12 0x00607F44 (feb 0x005DCA7C) particle-emitter visibility adaptor
    virtual int IsVisUpdateReq();              // v12 0x0051BF00
    virtual int IsVisible(m3dV *p);            // v12 0x0051BF10
    virtual int IsVisible(m3dBOX *b);          // v12 0x0051BF20
};
