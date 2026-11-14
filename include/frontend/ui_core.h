// ui_core.h -- Will Rock 1.2 unofficial SDK, subsystem "frontend": the engine UI library (gs:ui_*.obj)
//
// objs covered (feb map): gs:ui_main.obj (uiELEMENT, uiFRAME, uiFRAME_GROUP, uiWINDOW, uiCURSOR, uiUI_SYSTEM,
//   element registry), gs:ui_area.obj (uiAREA), gs:ui_rnd.obj (uiRENDER_INFO/uiRENDER_ELEM), gs:ui_btn.obj
//   (uiBUTTON), gs:ui_scroll.obj (uiSCROLL), gs:ui_textinp.obj (uiTEXT_INPUT), gs:ui_font.obj (uiFONT,
//   uiFONT_MANAGER). The Will Rock classes built on top are in wr_ui.h.
//
// Assumed from re/sdk/core: objOBJ (+0x18 char* name, +0x30 next sibling, +0x38 first child, +0xC8..+0xD8 bbox,
//   +0xF0 psSHEET* props), psSHEET, animTPL, animINST, camCAMERA, txmTEXTURE, pctPICTURE, m2dV {float x,y},
//   m3dV, arrVector<T>, msgADDR.
//
// Offsets: "// v12 +0xNN (feb +0xNN)". The UI library did not change its data layout between feb and 1.2: every
// offset marked without "?" was read from a v12 instruction (address given) and matches the feb decompile.
// The uiELEMENT vtable (24 slots, +0x00..+0x5C) is the same in both builds. uiUI_SYSTEM gained ONE virtual at
// +0x20 in 1.2, so every uiUI_SYSTEM/wrUI_SYSTEM slot from feb +0x20 upwards moves +4 (see wr_ui.h).
// Classes are #pragma pack(1) (the game uses odd offsets everywhere).

#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#if !defined(_MSC_VER) && !defined(__fastcall)
#  if defined(__i386__)
#    define __fastcall __attribute__((fastcall))
#  else
#    define __fastcall            /* syntax-check / documentation builds on other targets */
#  endif
#endif
#pragma pack(push, 1)

class uiELEMENT; class uiUI_SYSTEM; class uiFONT; class uiRENDER_INFO;
struct objOBJ; struct psSHEET; struct animINST; struct camCAMERA; struct txmTEXTURE; struct pctPICTURE;
#ifndef WR_SDK_CORE
struct m2dV { float x, y; };   // (defined in math_app/m3d.h when the core is included)
#endif

// ------------------------------------------------------------------------------------------------ flags
// uiELEMENT::flags (+0x04). Verified: Enable v12 0x00518CF0 (and/or ah,8 = 0x800); SetMouseFocus (feb 0x00505110)
// sets/clears 0x10; wrUI_SYSTEM::ShowHudFrame/Update (v12 0x0048C100/0x0048C3E0) set/clear 0x2000 on HUD windows.
// The rest from gs:ui_main.cpp / ui_area.cpp (rebuilt, v12 0x00518D10..0x00518F00, 0x00528B40..0x00528C20).
enum uiELEM_FLAGS {
    UIEF_INACTIVE      = 0x00000001,   // faded out / not shown: set when a disappear finishes, cleared by StartAppearing(1)
    UIEF_CAN_MOUSE     = 0x00000002,   // CanAcceptMouseFocus
    UIEF_CAN_KBD       = 0x00000004,   // CanAcceptKbdFocus
    UIEF_KEYS_ALWAYS   = 0x00000008,   // ProcessAction 5 / 6: gets keys and chars without the keyboard focus // ? name
    UIEF_MOUSE_FOCUS   = 0x00000010,   // element has the mouse focus
    UIEF_APPEARING     = 0x00000040,   // an appear is running
    UIEF_DISAPPEARING  = 0x00000100,   // a disappear is running
    UIEF_EFFECT        = 0x00000200,   // uiAREA: the wrUI_SYSTEM fade of its render info is running
    UIEF_TABSTOP       = 0x00000400,   // CanBeTabstop
    UIEF_DISABLED      = 0x00000800,   // Enable(0) sets it; disabled elements ignore input
    UIEF_HIDDEN        = 0x00002000,   // not rendered / not updated (HUD windows are toggled with this bit)
};

// uiUI_SYSTEM::state (+0x04) = the argument of wrUI_SYSTEM::Activate (vt +0x38 feb / +0x3C v12).
enum uiSYS_STATE {
    UIS_INACTIVE  = 0,    // game running, no menu
    UIS_MAINMENU  = 1,    // main menu (frame "main"; first time: comics "cmsLogo" from [Language])
    UIS_INGAME    = 2,    // in-game menu (frame "ig_main"), game time scale saved in wrUI_SYSTEM+0x164 and frozen
    UIS_HUD       = 3,    // HUD mode while playing (frames "hud" / "hud_cross"); also used while loading
};

// uiSYSINPUT_INFO::type, the event fed to uiUI_SYSTEM::OnSysInput (feb 0x00505330, uiSystem vt +0x28).
// It is a no-op unless gsAppState & 0x1000.  0 = mouse move, 1 = mouse button (down/up, double click by
// gswinGetDblClickTime), 2 = key, 3 = char (WM_CHAR is routed here by _gsWndProc, see math_app/app.h).
struct uiSYSINPUT_INFO { int type; int param; int param2; };   // ? exact layout beyond +0x04
struct uiINPUT_INFO    { int type; m2dV pos; int key; };      // element-level event (ProcessAction 3/4)   // ?
// [integrator] layout from re/mpsrc/wr_ui_mult.h: child, four-char notify type ('BTNC','TSCR','INP','SCOF'), code
// (button id 'BACK','NEXT'..., or 'CHNG'/'ENTR'). The earlier "code at +4" guess was wrong.
struct uiCHILD_NOTIFY  { uiELEMENT *child; int type; union { int code; int param; }; }; // OnChildNotify argument (vt +0x5C)

// ------------------------------------------------------------------------------------------------ uiELEMENT
// Base of every UI widget. sizeof = 0x6C (both builds: wr_wnd/wr_main_wnd factories allocate 0x6C,
// v12 0x0048DD60 / feb 0x00482C00).  Elements form a tree built from the "ui" template (see README).
struct uiBOX_ {                     // screen rectangle (gs:ui_main.obj class uiBOX)
    m2dV min, max;
    int  IsBelongBox(uiBOX_ *b);        // v12 0x005188F0 (feb 0x00503540) b inside this, or overlapping
    void AddVertex(m2dV *v);            // v12 0x00518980 (feb 0x005035D0) grow to hold v (see the note in ui_main.cpp)
};
class uiELEMENT {
public:
    typedef uiBOX_ uiBOX;
    // vtable v12 0x00603E68 (feb 0x005D8AC8), 24 slots, identical order in both builds.
    virtual ~uiELEMENT();                                   // slot +0x00 v12 0x0048F380 (feb 0x00482420)
    virtual int  GetElemDefType();                          // slot +0x04 v12 0x0048D580 (feb 0x0049A730) -1 = no default child type
    virtual int  CanAcceptKbdFocus();                       // slot +0x08 v12 0x00518D30 (feb 0x00503980)
    virtual int  CanAcceptMouseFocus();                     // slot +0x0C v12 0x00518D10 (feb 0x00503960)
    virtual void Enable(int on);                            // slot +0x10 v12 0x00518CF0 (feb 0x00503940) flags 0x800
    virtual int  CanBeTabstop();                            // slot +0x14 v12 0x00518D50 (feb 0x005039A0)
    virtual int  GetTabstopId();                            // slot +0x18 v12 0x00518D70 (feb 0x005039C0)
    virtual uiELEMENT *FindTabstop(void *search);           // slot +0x1C v12 0x00518DA0 (feb 0x005039F0)
    virtual int  StartAppearing(int how);                   // slot +0x20 v12 0x00518E10 (feb 0x00503A60) fade-in/out start
    virtual void FinishAppearing(int how);                  // slot +0x24 v12 0x00518EA0 (feb 0x00503AF0)
    virtual int  ProcessAppearing(int how);                 // slot +0x28 v12 0x00518F00 (feb 0x00503B50)
    virtual void OnAppeared(int how);                       // slot +0x2C v12 0x0051AAF0 (feb 0x004CDA70) empty
    virtual int  OnMouseFocus(uiELEMENT *other);            // slot +0x30 v12 0x0048D590 (feb 0x004C46B0) returns 1
    virtual int  OnKbdFocus(uiELEMENT *other);              // slot +0x34 v12 0x0048D590 (feb 0x004C46B0) returns 1
    virtual void Init(objOBJ *o);                           // slot +0x38 v12 0x00518A30 (feb 0x00503680) HOOK: read layout + create children
    virtual int  ProcessInit(void *p);                      // slot +0x3C v12 0x00519080 (feb 0x00524AE0) after the whole tree exists
    virtual int  ProcessRender(void *p);                    // slot +0x40 v12 0x00518E00 (feb 0x00503A50)
    virtual int  ProcessUpdate(void *p);                    // slot +0x44 v12 0x00519080 (feb 0x00524AE0) per frame
    virtual int  OnPreInput(uiINPUT_INFO *in);              // slot +0x48 v12 0x00519080
    virtual int  OnMouseMove(uiINPUT_INFO *in);             // slot +0x4C v12 0x00518F90 (feb 0x00503BE0)
    virtual int  OnMouseButton(uiINPUT_INFO *in);           // slot +0x50 v12 0x00519080
    virtual int  OnKeyboard(uiINPUT_INFO *in);              // slot +0x54 v12 0x00519080
    virtual int  OnChar(uiINPUT_INFO *in);                  // slot +0x58 v12 0x00519080
    virtual void OnChildNotify(uiCHILD_NOTIFY *n);          // slot +0x5C v12 0x00519090 (feb 0x00503CD0) widgets report to their window

    unsigned   flags;          // v12 +0x04 (feb +0x04) uiELEM_FLAGS. verified v12 0x00518CF6
    uiELEMENT *parent;         // v12 +0x08 (feb +0x08) set by uiElementCreate (feb 0x00505880)
    uiELEMENT *prev;           // v12 +0x0C (feb +0x0C) previous sibling
    uiELEMENT *next;           // v12 +0x10 (feb +0x10) next sibling
    uiELEMENT *firstChild;     // v12 +0x14 (feb +0x14)
    char       name[0x40];     // v12 +0x18 (feb +0x18) = objOBJ name, strncpy 0x40 (v12 0x00518A65 lea [esi+0x18])
    union {
        struct {
    m2dV       bboxMin;        // v12 +0x58 (feb +0x58) screen rect from obj bbox (x = obj+0xC8, y = -obj+0xD8). v12 0x00518A62
    m2dV       bboxMax;        // v12 +0x60 (feb +0x60)                                              v12 0x00518A70 (+0x5C)
        };
        uiBOX_ bbox;                            // [alias bbox: re/mpsrc]
    };
    union { unsigned groupMask; unsigned char byteFlags68; }; // v12 +0x68 (feb +0x68) bit i = UI_PROP "group%i" true (i < 32)
                               //   (re/mpsrc: byteFlags68, bit 1 marks the password row in the player setup window)
    // sizeof 0x6C

    uiELEMENT();                                            // feb 0x00503630 (v12: inlined in the factories)
    uiELEMENT *FindChildElement(const char *name);          // v12 0x00518C70 (feb 0x005038C0) recursive search by name. HOOK target for mods
    uiELEMENT *GetPathToChild(uiELEMENT *c);                // feb 0x005038A0 the child of this on the way down to c
    int        ProcessAction(int action, void *arg);        // v12 0x005190A0 (feb 0x00503CE0) uiACTION to the subtree
};

// Element factory registry. Each entry 0x44 bytes: char type[0x40]; uiELEMENT *(*create)(); (feb 0x00505880).
// Stored in uiUI_SYSTEM +0x118 (array) / +0x11C (count) / +0x120 (capacity).
// uiELEMENT::ProcessAction codes (the switch at v12 0x005190B3)
enum uiACTION { UIA_INIT = 0, UIA_UPDATE = 1, UIA_RENDER = 2, UIA_MOUSE_MOVE = 3, UIA_MOUSE_BUTTON = 4,
                UIA_KEYBOARD = 5, UIA_CHAR = 6, UIA_SHOW = 7 };
struct uiELEMENT_INFO { char type[0x40]; uiELEMENT *(__fastcall *create)(); };
int        __fastcall uiElementRegister(const char *type, uiELEMENT *(__fastcall *create)()); // v12 0x0051AB00 (feb 0x00505720) HOOK: add custom widget types
uiELEMENT *__fastcall uiElementCreate(const char *type, objOBJ *o, uiELEMENT *parent);     // feb 0x00505880 (v12 inlined/ ?)
void       __fastcall uiElementDestroy(uiELEMENT *e);                                       // feb 0x00505960
void       __fastcall uiUCoord2SCoord(m2dV *in, m2dV *out);                                // feb 0x00505FE0 UI units -> screen
// Base types registered by uiUI_SYSTEM::RegisterDefaultElements (feb 0x00504670): "area" (uiAREA), "window"
// (uiWINDOW), "frame" (uiFRAME), "frm_grp" (uiFRAME_GROUP). Will Rock adds 60+ types (element_types.tsv).

// ------------------------------------------------------------------------------------------------ uiAREA
// An element with a render description (text + textures). Base of buttons, scrolls, HUD pieces.
// uiRENDER_ELEM / uiRENDER_INFO (gs:ui_rnd.obj): how an element draws. Layout from the beta decompile of
// uiRENDER_ELEM::Init / Render / GetTextStartPos and uiRENDER_INFO::Init (same offsets in 1.2; rebuilt in src/gs/ui_rnd.cpp).
// A 4-vertex object (not obj3D) is a flat textured rectangle (uiRECT) in UI page units: x = world x, y = -world y,
// read from its world vertices by uiGetObjRenderRect. Anything else (obj3D=yes, or not 4 vertices) is drawn as a
// 3D object through the UI camera. The text (textID / textNEW) is drawn with a uiFONT at GetTextStartPos.
struct uiVERTEX { float x, y, u, v; };   // 0x10, page units + texture coordinates
class uiRECT {                           // 0x48
public:
    txmTEXTURE *tex;                     // +0x00 the object's first face texture (GetFaceTexRendMtl(0, 0))
    uiVERTEX    v[4];                    // +0x04 0 top-left, 1 top-right, 2 bottom-right, 3 bottom-left (by the bbox)
    unsigned    color;                   // +0x44 ARGB (UI_PROP color, default -1)
    void Render(camCAMERA *cam);         // v12 0x00528D50 (feb 0x005138B0) clamp to 1026 x 769, scale, RenderPolyCompat
};
enum uiRENDER_FLAGS {
    UIRF_HIDDEN = 0x01,                  // objHIDDEN: the rectangle / object is not drawn (the text still is)
    UIRF_TEXT   = 0x02,                  // has textID / textNEW
    UIRF_OBJ3D  = 0x04,                  // drawn as an object (obj3D, or not a 4-vertex quad)
    UIRF_SKIP   = 0x08,                  // uiRENDER_INFO::Render skips this sub-element
    UIRF_10     = 0x10,                  // (wrUI_JOIN_INFO sets it)   // ?
};
class uiFONT;
// The text part of a render element (uiRENDER_ELEM +0x94): its own string, or a gsSTRINGS id when cap is 0.
struct uiRENDER_ELEM_TEXT {
    union { wrWCHAR *text; int textId; };
    int textCap;
    wrWCHAR *GetString();                // v12 0x00528E70 (feb 0x005139D0)
};

class uiRENDER_ELEM {                    // size 0xAC (qsort stride in wrUI_JOIN_INFO::Init)
public:
    char      name[0x40];                // +0x00 apNAME: the object's name
    unsigned  flags40;                   // +0x40 uiRENDER_FLAGS
    objOBJ   *obj;                       // +0x44 UIRF_OBJ3D: the object drawn
    uiRECT    rect;                      // +0x48 .. +0x8F
    uiFONT   *font;                      // +0x90 fontID (default: the UI's font 0)
    union { wrWCHAR *text; int textId; };// +0x94 own text when textCap != 0, else a gsSTRINGS id
    int       textCap;                   // +0x98
    m2dV      fontSize;                  // +0x9C scaleText (x and y), default 1
    unsigned  align;                     // +0xA4 GetTextFlags bits: 1 centerx 2 right, 4 top 8 bottom 0xC centery, 0x10 fixed
    unsigned  colorText;                 // +0xA8 ARGB (alpha byte +0xAB), default -1
    uiRENDER_ELEM();                                                   // v12 0x00528F40 (feb 0x00513AA0)
    void Init(objOBJ *o);                                              // v12 0x00529170 (feb 0x00513CD0)
    void Render(camCAMERA *cam);                                       // v12 0x005295C0 (feb 0x00514120)
    m2dV *GetTextStartPos(m2dV *out);                                  // v12 0x005297A0 (feb 0x00514300)
    void MoveTo(m2dV *center);                                         // v12 0x00529860 (feb 0x005143C0) centre the rect on a point
    void CalcBBox(uiBOX_ *out);                                        // v12 0x005298C0 (feb 0x00514420) rect (or object) + text
    void AppendTextW(const wrWCHAR *s, int maxLen);                    // v12 0x00529AB0 (feb 0x00514610)
    void CopyFrom(const uiRENDER_ELEM *src);                           // (inlined: the member-wise copy in SetElementIdx)
    void SetTextW(const wrWCHAR *s, int maxLen);   // v12 0x00529A10 (feb 0x00514570)
    void SetTextId(int strId);                     // v12 0x005299C0
    void SetTex(txmTEXTURE *t);                    // v12 0x00529DA0 (feb 0x00514900) the rectangle's texture
    void SetColorText(unsigned long argb);         // ?
    void SetAlpha(float a);                        // v12 0x00529C40 (feb 0x005147A0) rect alpha 0..255
    void SetAlphaText(float a);                    // v12 0x00529CF0 (feb 0x00514850) text alpha 0..255
};
class uiRENDER_INFO : public uiRENDER_ELEM {     // size 0xBC (uiAREA +0x6C..+0x127; uiAREA::Create feb 0x00506120 allocates 0x128)
public:
    uiRENDER_ELEM *elems;                // +0xAC (area +0x118) the child objects' render elements
    int            nElems;               // +0xB0
    int            capElems;             // +0xB4
    void          *appear;               // +0xB8 (area +0x124) wrUI_SYSTEM::StartAppear's fade data (12 bytes)
    uiRENDER_INFO() : elems(0), nElems(0), capElems(0), appear(0) {}   // (inlined in every factory, v12 0x004983E9)
    void Init(objOBJ *o);                          // v12 0x00529DB0 (feb 0x00514910)
    void Render(camCAMERA *cam);                   // v12 0x00529FB0 (feb 0x00514B10)
    void SetFont(uiFONT *f);                       // v12 0x0052A000 (feb 0x00514B60) this and every element
    void SetColor(unsigned long c);                // v12 0x0052A050 (feb 0x00514BB0)
    void SetColorText(unsigned long c);            // v12 0x0052A090 (feb 0x00514BF0)
    void SetTex(txmTEXTURE *t);                    // v12 0x0052A0D0 (feb 0x00514C30)
    void SetAlpha(float a);                        // v12 0x0052A110 (feb 0x00514C70) this and every element
    void SetAlphaText(float a);                    // v12 0x0052A2D0 (feb 0x00514E30) this and every element
    void CalcBBox(uiBOX_ *out);                    // v12 0x0052A460 (feb 0x00514FC0)
    uiRENDER_ELEM *FindElement(const char *name);  // v12 0x0052A4F0 (feb 0x00515050) this (by its own name) or an element
    void SetElementIdx(const char *name, int idx); // v12 0x0052A5C0 (feb 0x00515120) swap the named element into slot idx
};
void     __fastcall uiGetObjRenderRect(objOBJ *o, uiVERTEX *out);     // v12 0x0051AD90 (feb 0x005059B0) quad -> 4 page vertices
unsigned __fastcall uiGetTextFlags(const char *s);                   // = uiUI_SYSTEM::GetTextFlags v12 0x00519CD0 (feb 0x00504910)
class uiAREA : public uiELEMENT {
public:
    uiRENDER_INFO renderInfo;           // v12 +0x6C..+0x127 (sizeof(uiAREA) 0x128) [integrator]
    virtual int  StartAppearing(int how);           // slot +0x20 v12 0x00528B70 (feb 0x005136D0)
    virtual void FinishAppearing(int how);          // slot +0x24 v12 0x00528C20 (feb 0x00513780)
    virtual int  ProcessAppearing(int how);         // slot +0x28 v12 0x00528BC0 (feb 0x00513720)
    virtual void Init(objOBJ *o);                   // slot +0x38 v12 0x00528B10 (feb 0x00513670)
    virtual int  ProcessRender(void *cam);          // slot +0x40 v12 0x00528B40 (feb 0x005136A0)
    static uiELEMENT *__fastcall Create();          // feb 0x00506120 ("area")
    void SetTextW(const wrWCHAR *s, int maxLen);   // v12 0x00528CD0 (re/mpsrc)
    // vtable v12 0x00603EC8 (feb 0x005D8B28), 24 slots. Own: StartAppearing +0x20 v12 0x00528B70 (feb 0x005136D0),
    // FinishAppearing +0x24 v12 0x00528C20, ProcessAppearing +0x28 v12 0x00528BC0, Init +0x38 v12 0x00528B10
    // (feb 0x00513670), ProcessRender +0x40 v12 0x00528B40 (feb 0x005136A0).
    // +0x6C uiRENDER_INFO render;  (text, colours, textures; uiRENDER_INFO::Init feb 0x00514910 reads the UI_PROP keys)  // ? size
    void SetFont(uiFONT *f);            // v12 0x00528C70 (feb 0x005137D0)
    void SetTextId(int strId);          // v12 0x00528CA0 (feb 0x00513800)  gsSTRINGS id
    void AppendTextW(const wrWCHAR *s, int maxLen); // feb 0x00513860 (v12: no twin found, inlined?)
};
// UI_PROP keys read for areas/buttons (strings referenced from the v12 UI code, re/work/sdk_frontend/strrefs_v12.txt):
//   type, group%i, textID, fontID, color, colorText, dColor/dColorText/dFontID (disabled), pColor/pColorText/pFontID
//   (pressed), rColor/rColorText/rFontID (rollover), align (left|centerx|right|top|centery|bottom|baseline|fixed...
//   parsed by uiUI_SYSTEM::GetTextFlags feb 0x00504910), scaleText, isClipping, hotKeyID, idOn/idOff, sndId,
//   frameID, nFrames, frameTime, texFramesX/Y, firstFrameX/Y, transTime, priority, textDropable.

// ------------------------------------------------------------------------------------------------ containers
class uiWINDOW : public uiELEMENT {                      // vtbl feb 0x005DC9B0
public:
    static uiELEMENT *__fastcall Create();               // feb 0x005061B0 ("window")
    int GetElemDefType();                                // slot +0x04 (feb 0x00403680, a shared "return 0"): "area"
};
class uiFRAME  : public uiELEMENT {                      // vtbl feb 0x005DCA10. a full screen page
public:
    static uiELEMENT *__fastcall Create();               // feb 0x00506210 ("frame")
    int GetElemDefType();                                // slot +0x04 (feb 0x004036A0, a shared "return 1"): "window"
    // ProcessUpdate feb 0x00503F50, ProcessRender feb 0x00503F60, OnPreInput feb 0x00503F80, OnAppeared feb 0x00503FA0
};
class uiFRAME_GROUP : public uiELEMENT {                 // vtbl feb 0x005DC8A8. the root ("ui_top"); shows one frame at a time
public:
    uiELEMENT *GetActiveFrame();                         // feb 0x00504090
    int  ShowFrame(const char *name, int how);           // feb 0x005040A0 / overload by pointer feb 0x00504170
    // ProcessUpdate feb 0x00504230 drives frame transitions
};

// ------------------------------------------------------------------------------------------------ widgets
class uiBUTTON : public uiAREA {                         // gs:ui_btn.obj
public:
    // adds virtuals after +0x5C: +0x60 GetActionSoundId (wr), +0x64 ProcessCommand (feb 0x00515B40 = notify parent),
    // GetRole feb 0x00515B80, GetHotKey feb 0x00515BB0. Input: OnMouseButton feb 0x00515BC0, OnKeyboard feb 0x00515CA0,
    // OnChar feb 0x00515DD0, OnKbdFocus feb 0x00515E00, OnMouseFocus feb 0x00515E80, Enable feb 0x00515F00.
    // Init feb 0x00515500 (v12 0x0052A... see function_index.tsv)
};
class uiSCROLL : public uiAREA {                         // gs:ui_scroll.obj: a value picker (left/right cycles values)
public:
    // Init feb 0x005192E0, OnKeyboard feb 0x00519380.  Extra virtuals +0x60 DoScroll, +0x64 GetNumStr,
    // +0x68 GetStr (string for value i).  Current value index at +0x12C (the options windows read
    // [elem+0x12C], e.g. v12 0x004947C5 / 0x0049480D).                                                           // ? name
    int  _128;                                        // +0x128 ?
    union { int curValue; int curIndex; }; // v12 +0x12C (feb +0x12C) verified v12 0x004947C5 (game options window LoadSettings) [alias curIndex: re/mpsrc]
    virtual void DoScroll(int delta);                 // slot +0x60 (re/mpsrc: MoveSel)
    virtual int  GetNumStr();                         // slot +0x64 (re/mpsrc: GetCount)
    virtual const wrWCHAR *GetStr(int i);             // slot +0x68
    void MoveSel(int d) { DoScroll(d); }
    int  GetCount()     { return GetNumStr(); }
};
class uiTEXT_INPUT : public uiAREA {                     // gs:ui_textinp.obj, vtbl feb 0x005DCE28
public:
    // ctor feb 0x00518C00, Init feb 0x00518CD0, OnKeyboard feb 0x00518E00 (v12 0x0052E3D0), OnChar feb 0x00518ED0
    // (v12 0x0052E4B0), ProcessUpdate feb 0x00519000 (caret).  feb GetEditText 0x005191B0 / SetEditText 0x005191D0 were
    // plain methods; in 1.2 the class has TWO MORE virtual slots (+0x60 v12 0x0052E810 SetEditText,
    // +0x64 v12 0x0052E7F0 GetEditText ?) so every wr text-input slot after +0x5C moves by +8 (wrUI_TEXT_INPUT 30 -> 32 slots).
    // [integrator] the 1.2 virtual slots after uiAREA's 24 (re/mpsrc/wr_ui_mult.h): +0x60 SetEditText, +0x64 GetEditText,
    // +0x68 ?, +0x6C SetMaxLen (feb +0x64)
    virtual void SetEditText(const wrWCHAR *s);       // slot +0x60 v12 0x0052E810
    virtual const wrWCHAR *GetEditText();             // slot +0x64 v12 0x0052E7F0 ?
    virtual void _slot68();                           // slot +0x68 ?
    virtual void SetMaxLen(int n);                    // slot +0x6C
    unsigned char _128[0x13D - 0x128];
    unsigned char inputFlags;                         // +0x13D (feb +0x13C) 4 = password mode (re/mpsrc) // ?
};

// ------------------------------------------------------------------------------------------------ fonts
struct uiFONT_LETTER { uiVERTEX v[4]; };  // 0x40: the glyph quad, x from its left edge, y from the line's top
class uiFONT_PAGE {                      // 0x14: consecutive characters from 'first'
public:
    txmTEXTURE *tex;                     // +0x00 the page texture (first letter's)
    int         first;                   // +0x04 character code of letter 0 (object name "page_<first>")
    arrVector<uiFONT_LETTER> letters;    // +0x08 ordered by line (top first), then by x
    uiFONT_PAGE();                                       // v12 0x0052C750 (feb 0x00517290)
};
class uiFONT {                                           // gs:ui_font.obj. A bitmap font made of pages of letters, 0x60
public:
    unsigned  _00;                       // +0x00
    uiFONT   *next;                      // +0x04 uiFONT_MANAGER list
    char      name[0x40];                // +0x08 template name (font_s / font_b / font_h)
    float     fixedW;                    // +0x48 advance in "fixed" mode (flag 0x10)
    float     ascent;                    // +0x4C max -top of the letters
    float     descent;                   // +0x50 max bottom of the letters
    arrVector<uiFONT_PAGE> pages;        // +0x54 sorted by first
    void Create(objOBJ *o);                              // v12 0x0052C770 (feb 0x005172B0) from the template's "ui_font" object
    void AddPage(int first, objOBJ *o);                  // v12 0x0052C8F0 (feb 0x00517430)
    void AddLettersLine(uiFONT_PAGE *p, objOBJ *line, float top); // v12 0x0052CE80 (feb 0x005179C0)
    uiFONT_LETTER *GetLetter(int c);                     // (inlined binary search; page 0 letter 0 when missing)
    void GetStringSizeW(m2dV *out, const wrWCHAR *s, const m2dV *scale, int flags); // v12 0x0052D090 (feb 0x00517BD0)
    void GetStringRectW(float x, float y, const wrWCHAR *s, const m2dV *scale, int flags, m2dV *min, m2dV *max); // v12 0x0052D3C0 (feb 0x00517F00)
    void AdjustStringPosW(m2dV *pos, const wrWCHAR *s, const m2dV *scale, int flags); // v12 0x0052D180 (feb 0x00517CC0)
    void PrintW(m2dV *end, camCAMERA *cam, const wrWCHAR *s, float x, float y, unsigned long color,
                const m2dV *scale, int flags);           // v12 0x0052D4C0 (feb 0x00518000)
};
class uiFONT_MANAGER {                                   // global fontManager, sizeof 0x10 (math_app/app.h), vtbl v12 0x00606154 (feb 0x005DAC28)
public:
    void   *vtbl;                        // +0x00
    uiFONT *first;                       // +0x04 list of fonts (uiFONT::next)
    int     _08, _0c;
    uiFONT *GetFont(const char *name);                   // feb 0x00518200
    int     CreateFont(const char *name);                // v12 0x0052D720 (feb 0x00518260) (uiUI_SYSTEM::Init creates GetNumFonts() fonts by GetFontName(i))
    void    DestroyFont(const char *name);               // feb 0x005183A0
};
extern uiFONT_MANAGER *fontManager;                      // feb 0x00956F88 (v12: see math_app/app.h)

// ------------------------------------------------------------------------------------------------ uiUI_SYSTEM
// The UI manager. The live object is wrUI_SYSTEM (sizeof 0x16C, wr_ui.h). Global uiSystem.
class uiUI_SYSTEM {
public:
    // vtable: see wrUI_SYSTEM in wr_ui.h (the engine part is slots +0x00..+0x64). Only slot 0 declared here.
    virtual void Notify(int ev, void *p);                // slot +0x00 v12 0x0051AAB0 (feb 0x005056E0)
    int          state;          // v12 +0x04 (feb +0x04) uiSYS_STATE. verified v12 0x0048BDCB (Activate: cmp [ebx+4],3)
    uiELEMENT   *capture;        // v12 +0x08 (feb +0x08) element that captured the mouse (gets all events)
    uiELEMENT   *mouseFocus;     // v12 +0x0C (feb +0x0C) SetMouseFocus feb 0x00505110
    uiELEMENT   *kbdFocus;       // v12 +0x10 (feb +0x10) SetKbdFocus feb 0x005051B0                          // ?
    unsigned char cursor[0x2C];  // v12 +0x14 (feb +0x14) uiCURSOR (Init feb 0x00504320, MoveTo feb 0x00504430); +0x18 pos
    // +0x20 m2dV cursor hot-spot offset, +0x28 m2dV, +0x30/+0x38 m2dV
    unsigned char cursorRender[0xAC]; // v12 +0x40 (feb +0x40) uiRENDER_ELEM (ctor feb 0x00513AA0)                // ? size
    float        screenW;        // v12 +0xEC (feb +0xEC) width of "ui_top" in UI units (Init feb 0x00504AC0)
    float        screenH;        // v12 +0xF0 (feb +0xF0)
    int          _f4, _f8;       // +0xF4, +0xF8
    m2dV         scale;          // v12 +0xFC (feb +0xFC) = (1,1)
    float        dblClickTimer;  // v12 +0x104 (feb +0x104) init 1000.0
    m2dV         lastClickPos;   // v12 +0x108 (feb +0x108)
    camCAMERA   *uiCamera;       // v12 +0x110 (feb +0x110) from template object "ui_camera" (orthographic 0x20C camera)
    int          _114;           // +0x114
    uiELEMENT_INFO *elemTypes;   // v12 +0x118 (feb +0x118) registry array (uiElementCreate reads uiSystem+0x118/+0x11C)
    int          nElemTypes;     // v12 +0x11C (feb +0x11C)
    int          capElemTypes;   // v12 +0x120 (feb +0x120)
    animINST    *tplInst;        // v12 +0x124 (feb +0x124) the instanced "ui" template (scnSCENE::CreateInst)
    union { int _128; int updateLock; }; // +0x128 [re/mpsrc: updateLock, ++/-- around list rebuilds] // ?
    uiELEMENT   *root;           // v12 +0x12C (feb +0x12C) uiFRAME_GROUP made from "ui_top". verified v12 0x0048BC9E
    // sizeof(uiUI_SYSTEM) 0x130; wrUI_SYSTEM continues at +0x130.

    uiUI_SYSTEM();                                       // feb 0x00504570
    int  Init();                                         // feb 0x00504AC0 (v12 0x00519E80): fonts, load template "ui", build tree, camera, cursor
    void Term();                                         // feb 0x00504D50 (v12 0x0051A100)
    int  Activate(int state, const char *frame);         // feb 0x00504E10
    void Deactivate();                                   // feb 0x00505050
    void Update();                                       // feb 0x005052A0
    void Render();                                       // feb 0x00505260 (v12 0x0051A620) skipped when gsAppState & 1
    void OnSysInput(uiSYSINPUT_INFO *in);                // feb 0x00505330 (v12 0x0051A6F0)
    int  SetMouseFocus(uiELEMENT *e);                    // feb 0x00505110
    int  SetKbdFocus(uiELEMENT *e);                      // feb 0x005051B0
    void GetCursorPos(m2dV *out);                        // feb 0x00505680
    void Resize();                                       // feb 0x00504AA0
};
class wrUI_SYSTEM;
extern wrUI_SYSTEM *uiSystem;                            // v12 0x00994568 (feb 0x0094DCE8) (typed as the live class, wr_ui.h)

#pragma pack(pop)
