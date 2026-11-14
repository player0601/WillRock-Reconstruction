// wr_curs.cpp -- wrCURSORS, the crosshairs and the mouse cursor (Will Rock 1.2, v12.exe; feb obj wr_curs.obj)
//
// v12 0x004A5460..0x004A5640, all of the object. Six crosshairs ("cursor_00_00".."cursor_05_01": each a normal and
// an on-target kind) and the mouse cursor (12, no texture) are registered with the gsc cursors; the crosshair is
// locked at the screen centre. The cursor in use is 2 * crosshair + type ([GameOpt] Crosshair 6 and more: off).

#include "game.h"
#include <stdio.h>

wrCURSORS *wrCURSORS::pInstance;            // v12 0x0064FF40
wrCURSORS *_wrCursors;                      // v12 0x0064FF3C

// v12 0x004A5460  (feb 0x00498020)
int __fastcall wrCURSORS::Init()
{
    wrCURSORS *c = (wrCURSORS *)operator new(sizeof(wrCURSORS));
    if (c) {
        pInstance = c;
        c->crosshair = 0;
        c->type = 0;
        _wrCursors = c;
    } else {
        _wrCursors = 0;
    }
    gscAllocCursors(13);
    char name[0x100];
    for (int i = 0; i < 6; i++)
        for (int t = 0; t < 2; t++) {
            sprintf(name, "cursor_%02i_%02i", i, t);
            if (!gscRegisterCursor(t + i * 2, name, 0.5f, 0.5f, 1.0f))
                return 0;
        }
    gscRegisterCursor(12, 0, 0, 0, 0);
    gscCursor.Lock(0.5f, 0.5f);
    int ch;
    if (!_apCfgReadInt(0, (char *)"GameOpt", (char *)"Crosshair", &ch)) {
        pInstance->crosshair = 0;
        gscCursor.cursor = pInstance->type;
        return 1;
    }
    pInstance->crosshair = ch;
    gscCursor.cursor = ch < 6 ? pInstance->type + ch * 2 : ch * 2;
    return 1;
}

// v12 0x004A55B0  (feb 0x00498170)
void __fastcall wrCURSORS::Term()
{
    gscFreeCursors();
    if (_wrCursors) {
        pInstance = 0;
        operator delete(_wrCursors);
    }
}

// v12 0x004A55D0  (feb 0x00498190)
txmTEXTURE *wrCURSORS::GetCursorTexture(int ch, int t)
{
    return gscGetCursorTexture(t + ch * 2);
}

// v12 0x004A55F0  (feb 0x004981B0)
void wrCURSORS::SetCursor(int ch)
{
    crosshair = ch;
    gscCursor.cursor = ch < 6 ? type + ch * 2 : ch * 2;
}

// v12 0x004A5620  (feb 0x004981E0)
void wrCURSORS::SetCursorType(int t)
{
    type = t;
    gscCursor.cursor = crosshair < 6 ? t + crosshair * 2 : crosshair * 2;
}
