// gs_scr2d.cpp -- gs.lib the 2D screen: UI units (0..1) to screen pixels (Will Rock 1.2, v12.exe; feb obj gs:gs_scr2d.obj)
//
// Only the screen size and gssUCoord2SCoord so far (the gssWND rectangle helpers that follow it are not rebuilt).
// The render system sets the size when the video mode is set (not rebuilt; the test sets it to its window).

#include "../Sources/game.h"

int gssScreenDX;                            // v12 0x009A57D0 screen width in pixels
int gssScreenDY;                            // v12 0x009A57D4 screen height in pixels
float gssCamAngleHor = 75.0f;               // v12 0x00636BD8 (name ours) the game camera's horizontal angle, set by
float gssCamAspectHW = 0.75f;               // v12 0x00636BDC (name ours)   gssWND_RENDER::ProcessAct (v12 0x0053E0B3)

// v12 0x00535580  (feb 0x0051FC00)
void __fastcall gssUCoord2SCoord(m2dV *in, m2dV *out)
{
    out->x = (float)gssScreenDX * in->x;
    out->y = (float)gssScreenDY * in->y;
}
