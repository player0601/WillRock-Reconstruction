// game.h -- private header of the reconstructed game code (WillRock.exe's own objs: player.obj, plr_*.obj, ...)
//
// The SDK (src_recon/include) is included as-is; this header adds what game code needs on top of it:
// the inline m3d helpers, the spline classes, the "not rebuilt" trap and slot-exact virtual calls.
#pragma once
#include <string.h>
#include <stdlib.h>
#include <new>
#include "wr_sdk.h"
#include "../m3d/m3d_lib.h"   // inline m3dNormalize / _m3dNormalize / m3dTransformPoint
#include "../m3d/m3d_spl.h"   // m3dSPL_BEZIER2D (plrHEIGHT_CTRL)

extern "C" void wrNotRebuilt(const char *fn, unsigned v12addr);
#define WR_NOT_REBUILT(addr) wrNotRebuilt(__PRETTY_FUNCTION__, addr)

#include "../wr_vcall.h"   // wrVCall: slot-exact virtual calls

// Byte-offset field access for the few fields the SDK has only as padding.
#define WR_FIELD(type, obj, off) (*(type *)((char *)(obj) + (off)))
