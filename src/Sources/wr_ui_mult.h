// wr_ui_mult.h -- Will Rock 1.2 (v12.exe, 2003-07-30), reconstructed for modders.
//
// The multiplayer front-end windows (ui.tpl element classes) and the small
// base class they share.  This header also carries the declarations that
// wr_ui_join.cpp, wr_ui_server.cpp and wr_go_mp.cpp use, so the three
// menu files read against one set of types.
//
// Source objects (feb 2003 reference build, db/WillRock.map):
//   wr_ui_mult.obj   wrUI_MP_WND, wrUI_CONN_WND, wrUI_DRVINIT_WND, wrUI_PSETUP_WND,
//                    wrUI_IPADDR_WND, wrUI_WAITSERVER_WND::OnChildNotify
//   wr_ui_join.obj   wrUI_JOIN_WINDOW, wrUI_JOIN_INFO, wrUI_WAITBROWSER_WND, wrUI_WAITJOIN_WND
//   wr_ui_server.obj wrUI_WAITSERVER_WND, wrUI_ADVSERV_WINDOW / _INFO / _RECORD
//
// Static analysis only.  This file documents; it is not meant to compile into the game.
//
// Conventions
//   "// v12 0x........  (feb 0x........)"  function addresses in the two builds.
//   "// +0xNNN (feb +0xNNN)"               member offsets in v12 / feb.
//   "// 1.2:"   differs from the feb build (most of these are already in 1.0 retail).
//   "// ?"      not resolved by static analysis.
//   Class / method / global names come from db/WillRock.map; member and local
//   names are descriptive (no symbols exist for them).
//
// ---------------------------------------------------------------------------
// Types assumed from other files (not reconstructed here)
//   uiELEMENT (ui engine)             ctor v12 0x005189E0, FindChildElement 0x00518C70,
//       StartAppearing 0x00518E10, OnChildNotify 0x00519090, and the shared
//       "return 1" stub 0x00519080 used as the base ProcessInit / ProcessUpdate /
//       OnPreInput / OnKeyboard (the feb decompile prints it as uiELEMENT::OnChar).
//       fields: +0x04 flags, +0x08 parent, +0x10 next sibling, +0x14 first child,
//               +0x68 byte flags (bit 1: "password row" marker, see PSETUP),
//               +0x6C uiRENDER_INFO
//   uiAREA::SetTextW 0x00528CD0, uiAREA::Init 0x00528B10
//   uiRENDER_INFO::SetTex 0x0052A0D0, uiRENDER_ELEM::SetTextId 0x005299C0
//   uiTEXT_INPUT (edit box) -- 1.2: Get/SetEditText are now virtual (see slots below)
//   wrUI_STRINGS  PreInit 0x004A0280, Clear 0x004A02E0, Compact 0x004A0350,
//       AddString(int strId, ulong value) 0x004A03A0, AddString(const char* key, ulong value) 0x004A0430,
//       AddString(const ushort* text, ulong value) 0x004A04D0, GetStr 0x004A05C0
//   wrUI_TEXT_SCROLL::UpdateArrows 0x0049FB90
//   wrUI_SCROLL_BAR::SetWindow 0x004A26C0, SetPos 0x004A2720
//   wrUI_WINDOW_SCROLL<REC,INFO> (template, wr_ui_ctrl.cpp) -- ProcessInit/OnKeyboard/
//       OnScrollNotify/ProcessScroll of the JOIN list: 0x0048FA00/0x0048FB00/0x0048FC20/0x0048FE70
//   wrUI_SYSTEM *uiSystem (v12 0x00994568)
//       vt+0x10 ShowWindow(const char* name, int flags)   (8 = modal/"wait" layer ?)
//       vt+0x14 MessageBox(const ushort* title, const ushort* text, int, int, int, const char* nextWindow)
//       vt+0xA0 GetNextLocation(char* buf, int size) -> wrUI_ADVSERV_WINDOW::GetNextLocation (see wr_go_mp.cpp) // ?
//       wrUI_SYSTEM::StartLoadingAction(int, int mpMode, const char* level, int errStrId) 0x0048CDF0
//   gsSTRINGS *gsStrings (v12 0x009946DC): GetStringId 0x0051CF20, GetStringIdFmt (cdecl) 0x0051CFD0,
//       GetStringById 0x0051D120, AsciiToUnicode 0x0051D1C0, UnicodeToAscii 0x0051D200
//   gsMP_NETWORK *gsSysNetwork (v12 0x0099D814) -- see gs_mp_network.h for every facade address
//   txmMANAGER *txmManager (v12 0x006561B8), txmMANAGER::Add 0x004B3410
//   apCfg (ini, fastcall): _apCfgReadInt 0x004AF7E0, _apCfgReadBool 0x004AF940,
//       _apCfgReadFloat 0x004AF9D0, _apCfgReadString 0x004AFA50,
//       apCfgWriteInt 0x004AFAD0, apCfgWriteBool 0x004AFB70, apCfgWriteFloat 0x004AFBD0,
//       apCfgWriteString 0x004AFC20
//   char apBuffer[0x1000] (v12 0x00654160) -- shared scratch buffer
//   int gsAppState (v12 0x0098E824): 0x08000000 = started with -dedicated
//   const int gsNetVersionId = 10020000 (constant 0x0098E4A0 pushed as an immediate)
// ---------------------------------------------------------------------------

#pragma once
#include "wr_sdk.h"   // engine core types + subsystem classes (re/sdk); was "assumed from other files"

#include "gs_mp_network.h"

// ---------------------------------------------------------------------------
// uiCHILD_NOTIFY: what a child element sends to its parent window (vt slot +0x5C)
// ---------------------------------------------------------------------------
#if 0 // [integrator] defined in sdk/frontend/ui_core.h (layout taken from here)
struct uiCHILD_NOTIFY
{
    uiELEMENT*  child;      // +0x00
    int         type;       // +0x04  four-char code
    int         code;       // +0x08  four-char code (button id, ...)
};
#endif


// notify types
#define UINOTIFY_BUTTON      'BTNC'   // 0x42544E43 button clicked, code = button id
#define UINOTIFY_SCROLLCHG   'TSCR'   // 0x54534352 list/spin value changed, code = 'CHNG'
#define UINOTIFY_CHANGE      'CHNG'   // 0x43484E47
#define UINOTIFY_INPUT       'INP'    // 0x00494E50 edit box, code = 'ENTR' (Enter pressed)
#define UINOTIFY_ENTER       'ENTR'   // 0x454E5452
#define UINOTIFY_FOCUS       'SCOF'   // 0x464F4353 row got focus (ADVSERV)

// button ids used by the MP windows (set in ui.tpl)
#define UIBTN_BACK           'BACK'   // 0x4241434B
#define UIBTN_NEXT           'NEXT'   // 0x4E455854
#define UIBTN_LAN_SERVER     'LANS'   // 0x4C414E53  "Create LAN server"   -> mpMode 0
#define UIBTN_LAN_CLIENT     'LANC'   // 0x4C414E43  "Join LAN game"       -> mpMode 1
#define UIBTN_IP_CLIENT      'IPC'    // 0x00495043  "Join I-Net server"   -> mpMode 2
#define UIBTN_UBI_SERVER     'UBIS'   // 0x55424953  Ubi.com host          -> mpMode 3
#define UIBTN_UBI_CLIENT     'UBIC'   // 0x55424943  Ubi.com browse        -> mpMode 4
#define UIBTN_UBI_ACCOUNT    'UBIA'   // 0x55424941  1.2: "create Ubi.com account" (opens a web page)
#define UIBTN_CREATE         'CREA'   // 0x43524541  ADVSERV "Create server"
#define UIBTN_JOIN           'JOIN'   // ?  see wr_ui_join.cpp
#define UIBTN_REFRESH        'REFR'   // ?  see wr_ui_join.cpp

// uiINPUT_INFO (keyboard event, first dword = event type)
#if 0 // [integrator] defined in sdk/frontend/ui_core.h (same offsets; +0x04/+0x08 = m2dV pos)
struct uiINPUT_INFO
{
    int type;       // +0x00  6 = key down
    int _04, _08;   // ?
    int key;        // +0x0C  7 = Escape, 9 = Tab (game key codes, not VK_)
};
#endif


// virtual slots of uiELEMENT used by these files (v12; feb identical up to +0x5C)
//   +0x10 Enable(int)          +0x20 StartAppearing(int)   +0x3C ProcessInit(void*)
//   +0x44 ProcessUpdate(void*) +0x48 OnPreInput(uiINPUT_INFO*)
//   +0x54 OnKeyboard(uiINPUT_INFO*)                          +0x5C OnChildNotify(uiCHILD_NOTIFY*)
// uiTEXT_INPUT (edit box) extra slots -- 1.2: three new virtuals, so SetMaxLen moved:
//   +0x60 SetEditText(const ushort*)   1.2: was the non-virtual uiTEXT_INPUT::SetEditText
//   +0x64 GetEditText()                1.2: was the non-virtual uiTEXT_INPUT::GetEditText
//   +0x6C SetMaxLen(int)               (feb +0x64)
// wrUI list/spin control (player_skin, connection_type):
//   +0x60 MoveSel(int delta)  +0x64 GetCount()   field +0x12C = current index, +0x138 wrUI_STRINGS

// ---------------------------------------------------------------------------
// wrUI_MP_WND: base of every "please wait" MP window (status line + Esc = Back)
// vtable v12 0x00605AF8 (feb 0x005DA5E0), size 0x74
// ---------------------------------------------------------------------------
class wrUI_MP_WND : public uiELEMENT
{
public:
    wrUI_MP_WND();                                            // v12 0x0049C050  (feb 0x00490120)
    virtual int  ProcessInit(void* data);                     // slot +0x3C  v12 0x0049C070
    virtual int  OnPreInput(uiINPUT_INFO* in);                // slot +0x48  v12 0x0049C120
    virtual int  OnKeyboard(uiINPUT_INFO* in);                // slot +0x54  v12 0x0049C140

protected:
    void SetStatusStr(const char* status, float time);        // v12 0x0049C0A0  (feb 0x00490170)

    uiAREA*  statusStr;     // +0x6C  child "status_str"
    float    statusTime;    // +0x70  // ? written, never read in these files
};

// ---------------------------------------------------------------------------
// "connection" window: pick LAN / Internet / Ubi.com
// vtable v12 0x00604E60
// ---------------------------------------------------------------------------
class wrUI_CONN_WND : public uiELEMENT
{
public:
    // -1 none, 0 LAN server, 1 LAN client, 2 IP client, 3 Ubi server, 4 Ubi client
    static int mpMode;                                        // v12 0x0064F8E8  (feb 0x0060EE60)

    virtual int  StartAppearing(int start);                   // slot +0x20  v12 0x0049C1A0  (feb 0x00490270)
    virtual void OnChildNotify(uiCHILD_NOTIFY* n);            // slot +0x5C  v12 0x0049C230  (feb 0x004902F0)
};

// "mp_wait_drvinit": waits for gsMP_NETWORK::Drv_Create to finish.  vtable v12 0x00604EC8
class wrUI_DRVINIT_WND : public wrUI_MP_WND
{
public:
    virtual int  StartAppearing(int start);                   // slot +0x20  v12 0x0049C3D0  (feb 0x00490420)
    virtual int  ProcessUpdate(void* data);                   // slot +0x44  v12 0x0049C460  (feb 0x004904B0)
    virtual void OnChildNotify(uiCHILD_NOTIFY* n);            // slot +0x5C  = wrUI_WAITSERVER_WND::OnChildNotify (shared code)
};

// "mp_wait_server": waits for gsMP_NETWORK::Server_Start.  vtable v12 0x00605068
class wrUI_WAITSERVER_WND : public wrUI_MP_WND
{
public:
    virtual int  StartAppearing(int start);                   // slot +0x20  v12 0x0049D130  (feb 0x00490E40)
    virtual int  ProcessUpdate(void* data);                   // slot +0x44  v12 0x0049D160  (feb 0x00490E70)
    virtual void OnChildNotify(uiCHILD_NOTIFY* n);            // slot +0x5C  v12 0x0049C390  (feb 0x004903E0)
};

// "setup_player": name / password / skin / connection speed.  vtable v12 0x00604F98
class wrUI_PSETUP_WND : public uiELEMENT
{
public:
    virtual int  ProcessInit(void* data);                     // slot +0x3C  v12 0x0049C5F0  (feb 0x00490640)
    virtual int  StartAppearing(int start);                   // slot +0x20  v12 0x0049C750  (feb 0x00490780)
    virtual void OnChildNotify(uiCHILD_NOTIFY* n);            // slot +0x5C  v12 0x0049C770  (feb 0x004907A0)
protected:
    void SaveSettings();                                      // v12 0x0049C900  (feb 0x00490950)
    void LoadSettings();                                      // v12 0x0049CA90  (feb 0x00490AC0)

    uiTEXT_INPUT* playerName;     // +0x6C  "player_name"
    uiTEXT_INPUT* playerPassword; // +0x70  "player_password"
    wrUI_LIST*    playerSkin;     // +0x74  "player_skin"      (6 entries UI_SKIN_1..6)
    uiAREA*       charPicture;   // [integrator] an area (renderInfo.SetTex is called on it)
    //            charPicture;    // +0x78  "char_pct"         (skin icon)
    wrUI_LIST*    connectionType; // +0x7C  "connection_type"  (UI_CONNECTION_1..n)  1.2: saved as [CONFIG] ConnSpeed
};

// "ip_addr": type a host address.  vtable v12 0x00604F30
class wrUI_IPADDR_WND : public uiELEMENT
{
public:
    virtual int  ProcessInit(void* data);                     // slot +0x3C  v12 0x0049CD20  (feb 0x00490CF0)
    virtual int  StartAppearing(int start);                   // slot +0x20  v12 0x0049CD40  (feb 0x00490D10)
    virtual void OnChildNotify(uiCHILD_NOTIFY* n);            // slot +0x5C  v12 0x0049CDC0  (feb 0x00490D80)
protected:
    uiTEXT_INPUT* ipAddrEdit;     // +0x6C  "ip_addr_edit"
};

// characters that do not count as a name (a name made only of these is refused)
// v12 pointer 0x00605AF0 -> L" .,`'\"=-_+<>?|\\/!^&*" at 0x00631794 (feb 0x005DA5D8 -> 0x005F3B54)
extern const wchar_t* wrUI_BlankNameChars;

// SkinIcon_01.. texture names, 0xB8-byte records (v12 0x0062C040, feb 0x005EEC20)
extern char wrUI_SkinIcons[6][0xB8];

// static initialiser of wr_ui_mult.obj (_$E2, v12 0x0049CD00, feb 0x00490930):
//   g_float_0x64F8E4 = c * c, c = *(float*)0x00605AEC (= 1.0e-6f ...) // ? a header constant (eps squared), unused here
