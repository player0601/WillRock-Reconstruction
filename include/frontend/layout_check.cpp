// layout_check.cpp -- compile-time check of the frontend header layouts against the v12 sizes/offsets.
// g++ -m32 -fsyntax-only -Wno-invalid-offsetof layout_check.cpp   (or cl /c /Zs layout_check.cpp)
#include "wr_ui.h"
#include "wr_game.h"
#define offsetof(t,m) __builtin_offsetof(t,m)
static_assert(offsetof(uiELEMENT,groupMask)==0x68,"");
static_assert(sizeof(uiELEMENT)==0x6C,"");
static_assert(offsetof(uiUI_SYSTEM,root)==0x12C,"");
static_assert(sizeof(wrUI_SYSTEM)==0x16C,"");
static_assert(offsetof(wrUIHUD_WINDOW,inited)==0x100,"");
static_assert(sizeof(wrUIHUD_WINDOW)==0x104,"");
static_assert(offsetof(wrGO_SYSTEM,mode)==0x98,"");
static_assert(offsetof(wrGO_SYSTEM,chatValue)==0x100,"");
static_assert(sizeof(wrGO_SYSTEM)==0x11C,"");
