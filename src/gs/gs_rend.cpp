// gs_rend.cpp -- gs.lib render system gsRENDER_SYSTEM (Will Rock 1.2, v12.exe; feb obj gs:gs_rend.obj)
//
// Only what the rebuilt code calls so far. The render frame (RenderFrame v12 0x0051BA50) and the rest are not
// rebuilt (the test renders on its own; see test/test_menu.cpp tstDrawScene).

#include "../Sources/game.h"

// v12 0x0051C0D0 (name ours) -- an empty function in 1.2 ("ret 0xC"); the player's damage handler calls it for
// explosions (type 2) with 0x28FF1414, 1, 0.2 (the beta flashed the screen here)
void gsRENDER_SYSTEM::Shake(unsigned, int, float)
{
}

// v12 0x0051C0E0  (feb 0x00506C70)
// The full-screen overlay's colour (ARGB) for RenderColorFullScr: the damage / pick-up flash, a cinema's fade.
void gsRENDER_SYSTEM::SetColorFullScr(unsigned argb)
{
    fullScrColor[0] = (float)((argb >> 16) & 0xFF);
    fullScrColor[1] = (float)((argb >> 8) & 0xFF);
    fullScrColor[2] = (float)(argb & 0xFF);
    fullScrColor[3] = (float)(argb >> 24);
    fullScrSet = 1;
    fullScrOn = 1;
}

// The per-instance part of gsRENDER_SYSTEM::EndFrame (v12 0x0051B990, feb 0x005065A0; the rest -- the driver's end
// of frame, slot +0x14, and the particle manager's v12 0x004D8B50 -- is not rebuilt): every instance plays the
// sounds of its entity's animation ([SOUND] animSeq / actionFrame: doors, gates, Zeus's throne ...), then this
// frame's animation flags go (0x2000 animating, 0x4000, 0x8000 sequence started; unk08 0x200), and an instance
// with 0x1000000 has its vertex buffers marked to rebuild. (name ours)
void __fastcall gsEndFrameInsts(int updAnimSnd)
{
    objMODIFIER::elapsedTime = 0;
    for (animINST *i = gsScenePtr->FindNextInst(0); i; i = gsScenePtr->FindNextInst(i)) {
        if (updAnimSnd && i->ent && i->ent->nSndChannels > 0)
            gsSysSound->UpdateEntAnim(i->ent);
        i->flags &= ~0xE000u;
        i->unk08 &= ~0x200u;
        if (i->flags & 0x1000000)
            i->root->SetStateVBNo(0x1F000);
    }
}
