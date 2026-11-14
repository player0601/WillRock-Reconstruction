// wr_ui_join.h -- Will Rock 1.2 (v12.exe): the server browser ("join"), its row
// widget, and the "mp_wait_browser" / "mp_wait_join" wait windows.
// Reconstructed documentation source; NOT meant to compile into the game.
//
// Source object: wr_ui_join.obj (feb 0x0048CB30..0x0048DD2F, v12 0x00498710..0x00499A3F;
// wrUI_JOIN_INFO::OnMouseButton was placed after wr_ui_load.obj's code: v12 0x00499AA0).
//
// Types assumed from other files: everything listed at the top of wr_ui_mult.h, plus
//   wrUI_WINDOW_SCROLL<wrUI_JOIN_RECORD, wrUI_JOIN_INFO> (template in wr_ui_ctrl.cpp; instantiated here):
//       ProcessInit v12 0x0048FA00, OnKeyboard 0x0048FB00, OnScrollNotify 0x0048FC20,
//       OnKbdFocus 0x0048FDC0, SetFocusToSelected 0x0048FDE0, ProcessScroll 0x0048FE70,
//       SetSelectedRec 0x0048FE30 (COMDAT shared with the wrUI_LS_* instantiation)
//   uiFONT::GetStringSizeW 0x0052D090, uiRENDER_INFO::SetColorText 0x0052A090,
//   uiRENDER_INFO::SetAlphaText 0x0052A2D0, uiRENDER_INFO::CalcBBox 0x0052A460,
//   uiRENDER_ELEM::SetTextW 0x00529A10, uiUI_SYSTEM::SetKbdFocus 0x0051A570
//   _uiGetLevelNameStrId(const char* level, int* strId, int* ?, int) fastcall 0x0048D3E0 (1.2 caller)
//   _compareRectX 0x00495A90 (qsort callback for render elements)
//   m3dUpdateTimeField(float*, float) 0x00565D20, gsElapsedTime 0x0098E9F4
//   gsMP_SERVER_INFO, wrMP_MAIN_DATA (gs_mp_network.h, wr_ui_server.h)

#pragma once
#include "wr_sdk.h"   // engine core types + subsystem classes (re/sdk); was "assumed from other files"

#include "wr_ui_mult.h"
#include "wr_ui_server.h"

// a list record is just the browser's server id (gsMP_NETWORK::Browser_GetNextServer)
struct wrUI_JOIN_RECORD
{
    int serverId;           // +0x00
};

// ---------------------------------------------------------------------------
// one row of the server list.  vtable v12 0x006050D0
//   main text: server name (feb: "v%i.%i <name>")
//   columns (uiRENDER_ELEM, 0xAC bytes each, at renderInfo.elems = +0x118):
//     1.2:  [0] ping   [1] players "%i/%i"   [2] mode   [3] map   [4] version
//     feb:  [0] players [1] mode  [2] map
// ---------------------------------------------------------------------------
class wrUI_JOIN_INFO : public uiAREA
{
public:
    virtual void Init(objOBJ* obj);                     // slot +0x38  v12 0x00498710  (feb 0x0048CB30)
    virtual int  OnMouseButton(uiINPUT_INFO* in);       // slot +0x50  v12 0x00499AA0  (feb 0x0048DD80)
    void SetRecord(wrUI_JOIN_RECORD* rec);              // v12 0x00498770  (feb 0x0048CB90)
    void ClearRecord();                                 // v12 0x00498B90  (feb 0x0048CE90)

    // +0x58  uiBOX bbox
    // +0x6C  uiRENDER_INFO renderInfo   (+0xB8/+0xC8 text box x0/x1, +0xFC uiFONT*, +0x108 font size,
    //                                    +0x110 ?, +0x117 alpha byte, +0x118 elems*, +0x11C nElems)
};

// ---------------------------------------------------------------------------
// "join": the server list.  vtable v12 0x00605130
// ---------------------------------------------------------------------------
class wrUI_JOIN_WINDOW : public wrUI_WINDOW_SCROLL<wrUI_JOIN_RECORD, wrUI_JOIN_INFO>
{
public:
    virtual int  StartAppearing(int start);             // slot +0x20  v12 0x00498C10  (feb 0x0048CEF0)
    virtual int  ProcessUpdate(void* data);             // slot +0x44  v12 0x00499050  (feb 0x0048D3B0)
    virtual void OnChildNotify(uiCHILD_NOTIFY* n);      // slot +0x5C  v12 0x00498DC0  (feb 0x0048D0A0)
protected:
    void Join();                                        // v12 0x00498F80  (feb 0x0048D260)

    // 1.2: new, qsort callback for the server list
    static int __cdecl CompareServers(const void* a, const void* b);   // v12 0x00499490  (no feb twin; name invented)

    // template helper emitted out of line in 1.2 (inlined in feb's template ProcessUpdate 0x0048DA20)
    void ScrollTo(int top);                             // v12 0x004998B0  (name invented)

    // layout (v12 = feb)
    //   +0x6C  int   selRec            (-1 = none)
    //   +0x70  int   scrollFlags       bit0 = auto-scroll to selection, bit1 = ?
    //   +0x74  int   topRow
    //   +0x78  int   nRecords
    //   +0x7C  wrUI_JOIN_RECORD records[64]
    //   +0x17C int   nRowWidgets
    //   +0x180 wrUI_JOIN_INFO* rowWidgets[32]
    //   +0x200 wrUI_SCROLL_BAR* scrollBar
    //   +0x204 float scrollSpeed        (0.02 when auto-scrolling)
    //   +0x208 int   needScroll
    //   +0x20C float refreshTimer       counts down; Drv_GetRefreshInterval() on reset
public:
    float refreshTimer;                 // +0x20C [integrator: was a layout comment]
};

// "mp_wait_browser": waits for Browser_Start.  vtable v12 0x00605000
class wrUI_WAITBROWSER_WND : public wrUI_MP_WND
{
public:
    virtual int  StartAppearing(int start);             // slot +0x20  v12 0x004995E0  (feb 0x0048D750)
    virtual int  ProcessUpdate(void* data);             // slot +0x44  v12 0x00499610  (feb 0x0048D780)
    virtual void OnChildNotify(uiCHILD_NOTIFY* n);      // slot +0x5C  v12 0x00499590  (feb 0x0048D700)
};

// "mp_wait_join": waits for the DirectPlay connect.  vtable v12 0x00605198
class wrUI_WAITJOIN_WND : public wrUI_MP_WND
{
public:
    virtual int  StartAppearing(int start);             // slot +0x20  v12 0x00499760  (feb 0x0048D8D0)
    virtual int  ProcessUpdate(void* data);             // slot +0x44  v12 0x00499790  (feb 0x0048D900)
    virtual void OnChildNotify(uiCHILD_NOTIFY* n);      // slot +0x5C  v12 0x00499720  (feb 0x0048D890)
};

// notify sent by a double-clicked row (wrUI_JOIN_INFO::OnMouseButton)
#define UINOTIFY_DBLCLICK   'DBLC'   // 0x44424C43, code 'INFO' (0x494E464F)
#undef  UIBTN_JOIN
#undef  UIBTN_REFRESH
#define UIBTN_JOIN          'JOIN'   // 0x4A4F494E
#define UIBTN_REFRESH       'REFR'   // 0x52454652
