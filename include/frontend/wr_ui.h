// wr_ui.h -- Will Rock 1.2 unofficial SDK, subsystem "frontend": the Will Rock UI (menus, HUD, message boxes,
// save/load, options) built on the engine UI library (ui_core.h).
//
// objs covered (feb map): wr_ui.obj (wrUI_SYSTEM + most widget classes), wr_ui_wnd.obj (scroll bar, message box,
//   main / in-game main windows, loading window), wr_ui_hud.obj (wrUIHUD_*), wr_ui_load.obj (save/load windows),
//   wr_ui_video.obj, wr_ui_audio.obj, wr_ui_ctrl.obj, wr_ui_gameopt.obj, wr_ui_comics.obj, wr_ui_credits.obj,
//   wr_ui_frm.obj, wr_ui_ig.obj, wr_ui_textinp.obj, wr_ui_txtscrl.obj, wr_ui_sinfo.obj, wr_ui_hscores.obj, wr_curs.obj.
//   NOT here: wr_ui_mult/join/server (multiplayer menus: see re/mpsrc).
//
// Assumed from re/sdk/core: msgADDR, objOBJ, txmTEXTURE, pctPICTURE, sncSOUND_DESCR, plrPLAYER (player/player.h),
//   gsSTRINGS (game_systems/strings.h), and everything listed at the top of ui_core.h.
//
// Offsets: "// v12 +0xNN (feb +0xNN)". The UI data layouts did not move between feb and 1.2 except where a size
// change is listed (element_types.tsv). Offsets without "?" were read from the v12 instruction given.
// Vtable warning: uiUI_SYSTEM got a NEW virtual at slot +0x20 in 1.2. Every wrUI_SYSTEM slot from feb +0x20 up is
// +4 in 1.2 (feb ShowMsgW +0x20 -> v12 +0x24, feb Activate +0x38 -> v12 +0x3C, feb PlaySnd(int) +0x84 -> v12 +0x88).
// Code that calls uiSystem->vt[...] with feb offsets crashes on 1.2. Full slot lists for all 70 UI classes:
// vtables.txt. Every function: function_index.tsv. Every widget type name: element_types.tsv.

#pragma once
#include "ui_core.h"
#pragma pack(push, 1)

class wrUIHUD_MSG; class wrUIHUD_UH_WINDOW; class wrUI_MSGBOX_WINDOW; class wrUI_LOADING_WND;
struct sncSOUND_DESCR; struct plrPLAYER;

// ================================================================================================ wrUI_SYSTEM
// The one UI object (global uiSystem). sizeof 0x16C (math_app/app.h: new 0x16C in InitSubsys).
// vtable v12 0x00603F28 (feb 0x005D8B88): 41 slots in 1.2, 40 in feb.
class wrUI_SYSTEM : public uiUI_SYSTEM {
public:
    virtual void Notify(int ev, void *p);                                 // slot +0x00 v12 0x0051AAB0 (feb 0x005056E0) uiUI_SYSTEM
    virtual void Update();                                                // slot +0x04 v12 0x0048C3E0 (feb 0x00481690) per frame: cursor, UI key (cmd 0x39), quicksave/load keys, HUD visibility
    virtual void Render();                                                // slot +0x08 v12 0x0051A620 (feb 0x00505260)
    virtual uiELEMENT *GetActiveFrame();                                  // slot +0x0C v12 0x0048D5A0 (feb 0x00482440)
    virtual int  ShowFrame(const char *frame, int how);                   // slot +0x10 v12 0x0048D5B0 (feb 0x00482450) HOOK: open any menu page by name (how = 8 normal)
    virtual void MsgBoxW(const wrWCHAR *cap, const wrWCHAR *text, int buttons, int defBtn, uiELEMENT *notify, const char *backFrame); // slot +0x14 v12 0x0048C940 (feb 0x00481AD0)
    virtual void MsgBoxW_Startup(const wrWCHAR *cap, const wrWCHAR *text, int buttons, int defBtn, uiELEMENT *notify, const char *backFrame); // slot +0x18 v12 0x0048C9A0 (feb 0x00481B30)
    virtual void ShowHudFrame(int which);                                 // slot +0x1C v12 0x0048C100 (feb 0x004813B0) 0 "hud" (+HideHUD), 1 "hud_cross", 2 outro comics "cmsOutro" -> "main"
    virtual int  IsMsgShown();                                            // slot +0x20 v12 0x0048F7F0 (new in 1.2) -> hud_messages->vt[+0x60] (v12 0x00495010): a line is up (NotifyMsgUse)
    virtual void ShowMsgW(const wrWCHAR *s, float time, int prio); // slot +0x24 v12 0x0048F800 (feb +0x20 0x004843D0) HOOK: HUD text message (-> wrUIHUD_MSG 0x00495060)
    virtual void ClearMsg(int a);                                         // slot +0x28 v12 0x0048F820 (feb +0x24 0x004843F0) (-> wrUIHUD_MSG 0x00494F50)
    virtual void OnSysInput(uiSYSINPUT_INFO *in);                         // slot +0x2C v12 0x0051A6F0 (feb +0x28 0x00505330) WM_CHAR/mouse from _gsWndProc
    virtual int  RegisterDefaultElements();                               // slot +0x30 v12 0x0048B810 (feb +0x2C 0x00480B50) HOOK: register extra widget types here
    virtual int  Init();                                                  // slot +0x34 v12 0x0048BC90 (feb +0x30 0x00480FB0) finds hud/msgbox/loading children
    virtual void Term();                                                  // slot +0x38 v12 0x0048BD90 (feb +0x34 0x004810B0)
    virtual int  Activate(int state, const char *frame);                  // slot +0x3C v12 0x0048BDC0 (feb +0x38 0x004810E0) HOOK: menu open (uiSYS_STATE); frame "null" = keep
    virtual void Deactivate();                                            // slot +0x40 v12 0x0048BFE0 (feb +0x3C 0x004812A0) back to game: input ctx "GAME", restore time scale
    virtual const char *GetTplName();                                     // slot +0x44 v12 0x0048F7B0 (feb +0x40 0x00484390) returns "ui" (the UI template file)
    virtual const char *GetSceneName();                                   // slot +0x48 v12 0x0048F7B0 (feb +0x44 0x00484390) also "ui"
    virtual const char *GetTypeName(int defType);                         // slot +0x4C v12 0x00519CA0 (feb +0x48 0x005048E0) "area"/"window"/"frame"/"frm_grp"
    virtual int  GetNumFonts();                                           // slot +0x50 v12 0x0048F7C0 (feb +0x4C 0x004843A0) returns 3
    virtual const char *GetFontName(int i);                               // slot +0x54 v12 0x0048B750 (feb +0x50 0x00480A90) [Language] tplFontS/tplFontB/tplFontH, default font_s/font_b/font_h
    virtual uiFONT *GetFont(int i);                                       // slot +0x58 v12 0x0048D5C0 (feb +0x54 0x00482460)
    virtual uiFONT *GetDefaultFont(int i);                                // slot +0x5C v12 0x0048F7D0 (feb +0x58 0x004843B0)
    virtual int  _slot60();                                               // slot +0x60 v12 0x004031F0 (feb +0x5C 0x004036A0) stub
    virtual unsigned GetTextFlags(const char *s);                         // slot +0x64 v12 0x00519CD0 (feb +0x60 0x00504910) parses "left|centerx|..."
    virtual int  StartAppear(uiRENDER_INFO *r, int how);                  // slot +0x68 v12 0x0048C210 (feb +0x64 0x004814C0) 0.2 s fade (alloc 12 bytes at r+0xB8)
    virtual int  ProcessAppear(uiRENDER_INFO *r, int how);                // slot +0x6C v12 0x0048C2A0 (feb +0x68 0x00481550)
    virtual void FinishAppear(uiRENDER_INFO *r, int how);                 // slot +0x70 v12 0x0048C3A0 (feb +0x6C 0x00481650)
    virtual void PlayAmbientMusic(int on);                                // slot +0x74 v12 0x0048CA00 (feb +0x70 0x00481B90) menu music
    virtual int  IsSndPlaying(int id);                                    // slot +0x78 v12 0x0048CA80 (feb +0x74 0x00481C00)
    virtual void StopAllSnd();                                            // slot +0x7C v12 0x0048CC40 (feb +0x78 0x00481DC0)
    virtual void StopSnd(int id);                                         // slot +0x80 v12 0x0048CC00 (feb +0x7C 0x00481D80)
    virtual int  PlaySnd(sncSOUND_DESCR *d);                              // slot +0x84 v12 0x0048CB70 (feb +0x80 0x00481CF0)
    virtual int  PlaySnd(int id);                                         // slot +0x88 v12 0x0048CAE0 (feb +0x84 0x00481C60) UI sound id (wrGO uses 5/6 for the MP countdown beeps)
    virtual void QuickSave(int autosave);                                 // slot +0x8C v12 0x0048CC70 (feb +0x88 0x00481DF0) HOOK: blocked during cinematics / !IsCanSaveNow
    virtual void QuickLoadLast();                                         // slot +0x90 v12 0x0048CD30 (feb +0x8C 0x00481EB0)
    virtual void ShowPlayerPain(int on, unsigned long color);             // slot +0x94 v12 0x0048CDC0 (feb +0x90 0x00481F40) red flash 0.3 s (player.h calls it)
    virtual void ShowLoadingScreen(const char *level);                    // slot +0x98 v12 0x0048CE40 (feb +0x94 0x00481FC0) HOOK: loading screen
    virtual void HideLoadingScreen(int failed);                           // slot +0x9C v12 0x0048CEB0 (feb +0x98 0x00482020) on failure: msgbox UI_ERROR + back to "main"
    virtual void GetNextMpLevel(char *out, int n);                        // slot +0xA0 v12 0x0048CF70 (feb +0x9C 0x004820D0) map rotation from the advanced-server window
    // (feb had no destructor slot in this table; the 40/41 slot counts above are complete)

    // ---- data (wrUI_SYSTEM own part, after uiUI_SYSTEM 0x130). Verified in v12 Init 0x0048BC90..0x0048BD79.
    wrUI_LOADING_WND  *loadingWnd;   // v12 +0x130 (feb +0x130) "loading_frame"/"loading_wnd"   v12 0x0048BD61
    uiELEMENT         *hudWnd;       // v12 +0x134 (feb +0x134) "hud"/"hud_wnd" (wrUIHUD_WINDOW)  v12 0x0048BCC4
    uiELEMENT         *hudMpWnd;     // v12 +0x138 (feb +0x138) "hud_mp_wnd" (DM scores)       v12 0x0048BCD6
    uiELEMENT         *hudSpWnd;     // v12 +0x13C (feb +0x13C) "hud_sp_wnd" (SP level stats)  v12 0x0048BD0C
    uiELEMENT         *hudThWnd;     // v12 +0x140 (feb +0x140) "hud_th_wnd" (treasure hunt)   v12 0x0048BCFA
    wrUIHUD_UH_WINDOW *hudUhWnd;     // v12 +0x144 (feb +0x144) "hud_uh_wnd" (pain flash, ping) v12 0x0048BD1E, ShowPlayerPain 0x0048CDC4
    void SendChat(const wrWCHAR *text, int team);   // v12 0x0048CF90 (name ours) the HUD chat line to the server -- not rebuilt
    uiELEMENT         *hudCoopWnd;   // v12 +0x148 (feb +0x148) "hud_coop_wnd"                  v12 0x0048BCE8
    wrUIHUD_MSG       *hudMessages;  // v12 +0x14C (feb +0x14C) "hud_messages"                  v12 0x0048BD34, 0x0048F7F0
    wrUI_MSGBOX_WINDOW*msgBox;       // v12 +0x150 (feb +0x150) "msgbox_wnd"                    v12 0x0048BD4A
    class wrUI_WOOFER *woofer;       // v12 +0x154 (feb +0x154) the menu sound entity (set by its ProcessINIT, v12 0x0048D348)
    int                _158;         // v12 +0x158 (feb +0x158) init -1                        v12 0x0048BD69
    int                _15C;         // v12 +0x15C (feb +0x15C)
    int                loadErrStrId; // v12 +0x160 (feb +0x160) string id shown if loading fails (StartLoadingAction)
    float              savedTimeScale; // v12 +0x164 (feb +0x164) game time scale before the in-game menu froze it  v12 0x0048BFB5
    pctPICTURE        *savePicture;  // v12 +0x168 (feb +0x168) 128x128 screenshot grabbed on menu open (save thumbnail) v12 0x0048BDD4
    // sizeof 0x16C

    wrUI_SYSTEM();                                                        // v12 0x0048B6F0 (feb 0x00480A30)
    void StartLoadingAction(int a, int b, const char *level, int errStrId); // v12 0x0048CDF0 (feb 0x00481F70)

    // [integrator] names the reconstructed multiplayer sources (re/mpsrc) use for the slots above (no new slots)
    int  ShowWindow(const char *frame, int how)            { return ShowFrame(frame, how); }          // +0x10
    void MessageBox(const wrWCHAR *cap, const wrWCHAR *text, int buttons, int defBtn, int notify, const char *back)
                                                           { MsgBoxW(cap, text, buttons, defBtn, (uiELEMENT *)(size_t)notify, back); } // +0x14
    void ShowHudMessage(const wrWCHAR *s, float t, int prio) { ShowMsgW(s, t, prio); }       // +0x24
    void ShowCenterText(const wrWCHAR *s, float t, int prio) { ShowMsgW(s, t, prio); }       // +0x24
    int  SetActiveWindow(int state, const char *frame)     { return Activate(state, frame); }        // +0x3C
    int  SetMode(int state, const char *frame)             { return Activate(state, frame); }        // +0x3C
    int  ShowMainMenu(int state, const char *frame)        { return Activate(state, frame); }        // +0x3C
    void vfunc38()                                         { Term(); }                               // +0x38
    void vfunc40()                                         { Deactivate(); }                         // +0x40
    int  Slot88(int sndId)                                 { return PlaySnd(sndId); }                // +0x88 UI sound
    void ShowLoading(const char *level)                    { ShowLoadingScreen(level); }             // +0x98
    void SetLoadingLevel(const char *level)                { ShowLoadingScreen(level); }             // +0x98
    void HideLoading(int failed)                           { HideLoadingScreen(failed); }            // +0x9C
    void vfunc9C(int failed)                               { HideLoadingScreen(failed); }            // +0x9C
    void GetNextLocation(char *out, int n)                 { GetNextMpLevel(out, n); }               // +0xA0
    using uiUI_SYSTEM::SetKbdFocus;
};
// extern uiUI_SYSTEM *uiSystem (ui_core.h, v12 0x00994568) points to this object: cast to wrUI_SYSTEM*.
inline wrUI_SYSTEM *wrUi() { return (wrUI_SYSTEM *)uiSystem; }

// Frame (page) names in the "ui" template that code opens with ShowFrame / Activate (strings in v12):
//   "main" (main menu), "ig_main" (in-game menu), "hud", "hud_cross", "msgbox", "load_frame"/"load_wnd",
//   "save_frame"/"save_wnd", "ig_load", "ig_save", "loading_frame", "comics_frame", "splash_frame", "options",
//   "controls", "connection", "create_server", "join", "setup_player", "single_player" / "sp_level" (1.2),
//   "adv_server_wnd", "credits_static_wnd", "mp_wait_browser", "mp_wait_drvinit", "mp_wait_join", "mp_wait_server".

// ================================================================================================ widgets
// All are created by name through uiElementRegister (table: element_types.tsv). Sizes are identical in both
// builds except: wr_txt_inp 0x140->0x141, wr_cinfo 0x158->0x159, wr_advserv_wnd 0xA40->0xA4C, wr_comics 0x1A8->0x1B0,
// wrh_msg 0x140->0x144, wr_hud_uh_wnd 0x78->0x84, wr_hud_mp_wnd 0xAC->0xB0, wr_hud_coop_wnd 0xAC->0xE4.
class wrUI_BUTTON : public uiAREA {      // "wr_btn", sizeof 0x1B8, vtbl v12 0x00604038 (feb 0x005D8C90), 32 slots
public:
    // Init +0x38 v12 0x0048B500 (feb 0x00480710); ProcessCommand +0x64 v12 0x0048B5F0 (feb 0x00480800) HOOK: button click;
    // GetActionSoundId +0x60 v12 0x0048B620; OnMouseFocus +0x30 v12 0x0048B670 (rollover sound)
    static uiELEMENT *__fastcall Create();   // v12 0x0048D5D0 (feb 0x00482470)
};
class wrUI_BUTTON_WEBSITE : public wrUI_BUTTON { };  // "wr_website": ProcessCommand v12 0x0048B6A0 opens www.saber3d.com (ShellExecute, see PORTING)
class wrUI_IG_BUTTON : public wrUI_BUTTON { };       // "wr_igbtn" 0x1F8: in-game menu button (UI_PROP ig_frameID). ProcessCommand v12 0x004986B0
class wrUI_IG_RESUME : public wrUI_BUTTON { };       // "wr_igresume": ProcessCommand v12 0x004985E0 = Deactivate (resume game)
class wrUI_CR_BACK : public wrUI_BUTTON { };         // "wr_cback" credits back button, ProcessCommand v12 0x00492D10
class wrUI_TEXT_INPUT : public uiTEXT_INPUT { };     // "wr_txt_inp" 0x141 (feb 0x140), vtbl v12 0x006041B8 (32 slots, feb 30: see ui_core.h)
// [integrator] list/picker helpers used by the multiplayer windows (re/mpsrc/wr_ui_*.cpp); layouts from those files.
struct wrUI_STRINGS {                                // 0x18 bytes: a list of {text,value} choices (wr_ui_ctrl.obj)
    int      _00, _04, _08;                          // +0x00..+0x08 ?  (zeroed by the wrUI_ADVSERV_RECORD ctor)
    struct ENTRY { const wrWCHAR *text; unsigned long value; } *entries; // +0x0C
    int      count;                                  // +0x10
    int      _14;                                    // +0x14 ? capacity
    void PreInit(int, int);                          // v12 0x004A0280
    void Clear();                                    // v12 0x004A02E0
    void Compact();                                  // v12 0x004A0350
    void AddString(int strId, unsigned long value);  // v12 0x004A03A0
    void AddString(const char *key, unsigned long value);           // v12 0x004A0430
    void AddString(const wrWCHAR *text, unsigned long value);// v12 0x004A04D0
    const wrWCHAR *GetStr(int i);             // v12 0x004A05C0
};
class wrUI_SCROLL_BAR;
class wrUI_TEXT_SCROLL : public uiSCROLL {           // uiSCROLL (+0x12C curValue) with left/right arrow elements
public:
    uiELEMENT *arrows[2];                            // +0x130  // ?
    void UpdateArrows();                             // v12 0x0049FB90
    virtual void OnScrolled();                       // (feb ?OnScrolled@wrUI_TEXT_SCROLL)
};                                                   // size 0x138
class wrUI_STR_SCROLL : public wrUI_TEXT_SCROLL {    // "wr_txt_scrl" 0x150
public:
    wrUI_STRINGS strings;                            // +0x138
    // re/mpsrc calls it wrUI_LIST: virtual +0x60 MoveSel(int delta), +0x64 GetCount(); current index at +0x12C
};
typedef wrUI_STR_SCROLL wrUI_LIST;
int  __cdecl    _compareRectX(const void *a, const void *b);       // v12 0x00495A90 qsort callback for render elements (re/mpsrc)
int  __fastcall _uiGetLevelNameStrId(const char *level, int *strId, int *b, int c); // v12 0x0048D3E0 (re/mpsrc) // ? args
void __fastcall sub_0049F740(const char *level); // v12 0x0049F740, inside wrUI_SI_PLAYER_WND::ProcessInit range: unidentified level-name helper called by wrGO (re/mpsrc) // ?                   // re/mpsrc name
class wrUI_STR_SCROLL_DOC : public uiSCROLL { };     // (previous one-line entry:) "wr_txt_scrl" 0x150 string list picker (wrUI_STRINGS list: AddString v12 0x004A03A0/0x004A0430/0x004A04D0)
class wrUI_NUM_SCROLL : public uiSCROLL { };         // "wr_num_scrl" 0x140 number picker: SetCurInt v12 0x0048DC30, GetCurInt v12 0x0048DC50
class wrUI_SWITCH : public uiSCROLL { };             // "wr_switch" 0x140 on/off (UI_ON/UI_OFF). Init v12 0x004A0160
class wrUI_CROSSHAIR : public uiSCROLL { };          // "wr_crosshair" 0x138 crosshair picker; OnScrolled v12 0x004945E0 previews via wrCURSORS
class wrUI_SCROLL_BAR : public uiAREA {             // "wr_scr_bar" 0x160
public:
    void  SetWindow(uiELEMENT *owner, int range, int page); // v12 0x004A26C0 (args from re/mpsrc) // ?
    void  SetPos(float pos);                          // v12 0x004A2720 // ? arg type
    float GetPos();                                   // v12 0x004A2350
};
class wrUI_ANIMTEX : public uiAREA { };              // "wr_animtex" 0x148 animated texture (nFrames, frameTime, texFramesX/Y)
class wrUI_WINDOW : public uiELEMENT { };            // "wr_wnd" 0x6C, vtbl v12 0x00604400. Init v12 0x004A1F80
// wrUI_WINDOW_SCROLL<REC,INFO>: scrolling list window template (wr_ui_ctrl.cpp), instantiated for the server list
// (JOIN) and the advanced-server options (ADVSERV). Layout from re/mpsrc/wr_ui_join.h / wr_ui_server.h (v12 = feb).
class wrUI_SCROLL_BAR;
template <class REC, class INFO> class wrUI_WINDOW_SCROLL : public wrUI_WINDOW {
public:
    union { int selRec; int selRow; };               // +0x6C -1 = none
    int    scrollFlags;                              // +0x70 bit0 auto-scroll to selection
    int    topRow;                                   // +0x74
    int    nRecords;                                 // +0x78
    REC    records[64];                              // +0x7C
    int    nRowWidgets;                              // after records
    INFO  *rowWidgets[32];
    wrUI_SCROLL_BAR *scrollBar;
    float  scrollSpeed;                              // 0.02 when auto-scrolling
    int    needScroll;
    // JOIN: ProcessInit v12 0x0048FA00, OnKeyboard 0x0048FB00, OnScrollNotify 0x0048FC20, OnKbdFocus 0x0048FDC0,
    //       SetFocusToSelected 0x0048FDE0, SetSelectedRec 0x0048FE30, ProcessScroll 0x0048FE70
    // ADVSERV: OnKeyboard 0x0048ED10, ProcessScroll 0x0048F1D0, UpdateContext 0x00490C50, OnChildNotify 0x00490D10,
    //       ProcessUpdate 0x00490870, OnScrollNotify 0x00490A50
    void SetSelectedRec(int i, int flags);
    void SetFocusToSelected();
    void ProcessScroll();
    void UpdateContext();
};
class wrUI_FRAME : public uiELEMENT { };             // "wr_frm" 0x74, vtbl v12 0x00604468. OnKeyboard v12 0x004942E0 (Esc = back)

// ---- message box: "msgbox_wnd" (wrUI_SYSTEM+0x150), children msgbox_head, msgbox_line%02i, msgbox_btn%02i
class wrUI_MSGBOX_WINDOW : public uiELEMENT {        // "wr_msgbox_wnd" 0xB8, vtbl v12 0x00605C00 (feb 0x005DA6E0)
public:
    void Initialize(uiELEMENT *notify, int buttons, int defBtn, const char *backFrame); // v12 0x004A2DC0 (feb 0x00495A80)
    void SetCaptionText(const wrWCHAR *s);    // v12 0x004A2E10 (feb 0x00495AD0)
    void SetMessageText(const wrWCHAR *s);    // v12 0x004A2E30 (feb 0x00495AF0) word-wraps into msgbox_line%02i
    void SetButtonsMask(int mask);                   // v12 0x004A2AF0 (feb 0x004957B0)
    // OnKeyboard v12 0x004A2950, OnChildNotify v12 0x004A29B0 (reports the pressed button to `notify`)
};

// ---- main menus
class wrUI_MAIN_WINDOW : public uiELEMENT { };       // "wr_main_wnd": OnChildNotify v12 0x004A30B0 (feb 0x00495D20) quit -> UI_MSGCAP_QUIT
class wrUI_IG_MAIN_WINDOW : public uiELEMENT { };    // "wr_igmain_wnd": StartAppearing v12 0x004A3180, OnChildNotify v12 0x004A3290 (stop game: UI_MSGCAP_STOP_GAME)
class wrUI_NEWGAME_WINDOW : public uiELEMENT { };    // "wr_newgame_wnd": difficulty choice
class wrUI_SPLAYER_WINDOW : public uiELEMENT { };    // "wr_splayer_wnd" NEW in 1.2 (vtbl v12 0x006044C8): single-player page
class wrUI_SPLEVEL_WINDOW : public uiELEMENT { };    // "wr_splevel_wnd" NEW in 1.2 (0x7C, vtbl v12 0x00604598): mission/level/sub-level select.
                                                     //   init v12 0x0049F210, notify 0x0049F360 (children mission/level/sublevel, reads [Level] pass),
                                                     //   0x0049F670 UI_SPLEVEL_SUBLEVEL_FMT, level table v12 0x0064FB4C (16-byte rows, 0x2A rows),
                                                     //   unlock hash v12 0x0049F8A0. See README "Level select".
class wrUI_LOADING_WND : public uiELEMENT {          // "wr_loading_wnd" 0x74 (wrUI_SYSTEM+0x130)
public:
    void SetParams(int a, int b, const char *level); // v12 0x004A3390 (feb 0x00495F90)
    void UpdateLevelName(const char *level);         // v12 0x004A33D0 (feb 0x00495FD0) LEVEL_NAME_%i / MISSION_NAME_%i strings
    void ShowFrame();                                // v12 0x004A3440 (feb 0x00496040)
    int  IsActive();                                 // v12 0x004A3460 (feb 0x00496060)
};
class wrUI_SPLASH_FRM : public uiELEMENT { };        // "wr_frm_splash": logo frame, any key continues (OnKeyboard v12 0x0048F7A0)

// ---- options
class wrUI_GAME_OPT_WINDOW : public uiELEMENT {      // "wr_game_opt_wnd" 0x6C, vtbl v12 0x006049D0 (feb 0x005D9548)
public:
    void LoadSettings();   // v12 0x00494790 (feb 0x00489540) [GameOpt] -> children (see wr_game.h GameOpt keys)
    void SaveSettings();   // v12 0x00494950 (feb: inside OnChildNotify 0x00489450)  children -> [GameOpt], then wrCURSORS::SetCursorType
    // ProcessInit v12 0x00494690 fills "autoweapon" with UI_AUTOWPN_%i strings; StartAppearing v12 0x00494740
};
class wrUI_AUDIO_WND : public uiELEMENT {            // "wr_awnd" 0x79: sfx_volume, music_volume, reverse_stereo
public:
    void ApplySettings();   // v12 0x00491050 (feb 0x00485F20)
    void RestoreSettings(); // v12 0x00491160 (feb 0x00486030)
};
class wrUI_VIDEO_WINDOW : public uiELEMENT {         // "wr_vwnd" 0xA14: the video options list (wrUI_VIDEO_INFO rows)
public:
    void LoadSettings();          // v12 0x004A0FA0 (feb 0x00493C60) reads [Video] VIDEO_* keys
    void ApplySettings();         // v12 0x004A1550 (feb 0x00494210) writes them, re-inits the driver
    void InitAdaptersList();      // v12 0x004A18C0 (feb 0x00494580)
    void UpdateBppList();         // v12 0x004A19D0 (feb 0x00494690)
    void InitResolutionsList();   // v12 0x004A1B20 (feb 0x004947E0)
    void UpdateResolutionsList(); // v12 0x004A1BD0 (feb 0x00494890)
};
class wrUI_CTRL_WINDOW : public uiELEMENT {          // "wr_cwnd" 0xA0C: key bindings (wrUI_CTRL_INFO rows "UI_CTRL_CMD_%s")
public:
    int StoreCommand(void *rec);  // v12 0x00493F20 (feb 0x00488C40) writes a binding back to the GAME input context
    // ProcessInit v12 0x004934A0, OnChildNotify v12 0x00493A70. feb hid DBG_* rows unless gsAppState&0x8000 is clear.
};
class wrUI_CADV_WINDOW : public uiELEMENT { };       // "wr_cadv_wnd" mouse: mouse_sens ([Input] MouseSens), inv_mouse ([Input] InvertMouse)

// ---- save / load
// Files: "..\saves\" + [Level] pathSAVEGAME + "save%04i.wrs" (BuildSaveName v12 0x0049A530); thumbnail "wr_ui_screenshot".
class wrUI_LS_WINDOW : public uiELEMENT {            // common base of load and save windows
public:
    void ChangePicture(int idx);     // v12 0x0049A3E0 (feb 0x0048E700)
    void BuildSaveName(int n, char *out); // v12 0x0049A530 (feb 0x0048E850)
    void RescanFileNames();          // v12 0x0049A5D0 (feb 0x0048E8F0) lists save%04i.wrs
};
class wrUI_SAVE_WINDOW : public wrUI_LS_WINDOW {     // "wr_wnd_save" 0x4320, vtbl v12 0x006046D0
public:
    int  SaveGame(int slot);         // v12 0x0049B0F0 (feb 0x0048F390) ?  HOOK: save
    void QuickSave(int autosave);    // feb 0x0048F640 (v12 ?) UI_QUICKSAVE / UI_AUTOSAVE, GAME_MSG_GAME_SAVED
};
class wrUI_LOAD_WINDOW : public wrUI_LS_WINDOW {     // "wr_wnd_load" 0x431C, vtbl v12 0x00604660
public:
    int  LoadLastSave();             // v12 0x0049BAB0 (feb 0x0048FB80) quick load = newest file
};

// ---- comics / credits (data driven: psSHEET sections, see README)
// One render element's fade (wrUI_COMICS::elem, 0xE bytes)
#pragma pack(push, 1)
struct wrUI_COMICS_ELEM {
    int   state;                  // +0x00 0 shown, 1 fading in, 2 fading out, 3 hidden
    float time;                   // +0x04 into the fade
    float len;                    // +0x08 the fade's length
    u8    alpha;                  // +0x0C the picture's full alpha
    u8    alphaText;              // +0x0D the text's
};
#pragma pack(pop)
// The comic player (src/Sources/wr_ui_comics.cpp): an area whose elements are ui.tpl's "comics" frames, the pictures
// first (sorted by height), then the text lines; a .cms sheet played event by event (EVENT_1, EVENT_1_1 ...).
class wrUI_COMICS : public uiAREA {                  // "wr_comics" 0x1B0 (feb 0x1A8)
public:
    int  ProcessInit(void *p);                                   // +0x3C v12 0x004913F0 (feb 0x004862B0)
    int  StartAppearing(int how);                                // +0x20 v12 0x00491650 (feb 0x00486410)
    int  ProcessUpdate(void *p);                                 // +0x44 v12 0x00491690 (feb 0x00486440)
    void Stop(int drop);                                         // v12 0x00491910 (feb 0x004866C0)
    void RemoveElement(int i, float time);                       // v12 0x004919D0 (feb 0x00486760)
    void AddText(char *frame, const wrWCHAR *s, unsigned long flags, const m3dCOLOR *col, float fadeIn); // v12 0x00491A80
    void ProcessNextEvent();                                     // v12 0x00491CE0 (feb 0x00486A90)
    psSECT *GetNextSection();                                    // v12 0x00492390 (feb 0x00487140)
    static uiELEMENT *__fastcall Create();                       // (RegisterDefaultElements' "wr_comics")
    sncSOUND_DESCR **snd;         // v12 +0x128 arrVector: the sounds the comic loaded
    int   nSnd, capSnd;           // v12 +0x12C, +0x130
    int   sndHandle[4];           // v12 +0x134 the UI sound playing on each channel (SoundId), -1 none
    wrUI_COMICS_ELEM *elem;       // v12 +0x144 arrVector: one per render element
    int   nElem, capElem;         // v12 +0x148, +0x14C
    int   nPic;                   // v12 +0x150 the elements before this are pictures, the rest text lines
    int   fast;                   // v12 +0x154 time runs 5 times as fast (cleared by a Sound)
    char  event[0x40];            // v12 +0x158 the section being played
    u32   waitSnd;                // v12 +0x198 bit n: wait until channel n's sound ends
    float delay;                  // v12 +0x19C to the next event
    psSHEET *sheet;               // v12 +0x1A0 the comic (0: none playing)
    int   ownSheet;               // v12 +0x1A4
    u32   _1a8, _1ac;
};
// The page that holds it ("comics_frame"): Start loads a .cms and shows the page; at its end the page tells
// 'notify' (OnChildNotify 'COMI', param) or, without one, the main menu comes.
class wrUI_COMICS_FRAME : public uiELEMENT {         // "wr_frm_comics" 0x80 (a wrUI_FRAME)
public:
    int  GetElemDefType();                                       // +0x04 v12 0x004031F0 (wrUI_FRAME's "return 1": windows)
    int  ProcessInit(void *p);                                   // +0x3C v12 0x00492460 (feb 0x00487210)
    int  StartAppearing(int how);                                // +0x20 v12 0x00492660 (feb 0x00487410)
    int  ProcessUpdate(void *p);                                 // +0x44 v12 0x004926C0 (feb 0x00487470)
    int  OnKeyboard(uiINPUT_INFO *in);                           // +0x54 v12 0x00492750 (feb 0x00487500) Esc: the comic ends
    static uiELEMENT *__fastcall Create();
    static int __fastcall Start(uiELEMENT *notify, int param, const char *file, int how); // v12 0x00492480 (feb 0x00487230) HOOK: play a comic ([Language] cmsLogo / cmsIntro / cmsOutro)
    u32   _6c;                    // v12 +0x6C (wrUI_FRAME)
    uiELEMENT *back;              // v12 +0x70 (wrUI_FRAME::ProcessInit) its "back" element
    uiELEMENT *notify;            // v12 +0x74 told at the end
    wrUI_COMICS *comics;          // v12 +0x78 the player ("comics")
    int   param;                  // v12 +0x7C
};
class wrUI_CR_FRAME : public uiELEMENT { };          // "wr_cfrm" credits: ShowNextPage v12 0x004929F0, strings UI_CREDITS_STRING_%i_%i

// ================================================================================================ HUD
// The HUD lives in the "hud" frame. wrUI_SYSTEM::Update shows/hides the per-mode windows each frame
// (flag UIEF_HIDDEN): hud_mp_wnd in DM (wrGO mode 1), hud_coop_wnd in COOP (2), hud_th_wnd in TH (3), while
// SHOWFRAGS (cmd 0x27) is held or the player is dead or the game is over (gsAppState & 0x80000).
// Registered by wruiHUDRegister (v12 0x00494AB0, called from RegisterDefaultElements v12 0x0048BC11).
class wrUIHUD_WINDOW : public uiELEMENT {            // "wr_hud_wnd" sizeof 0x104 (both), vtbl v12 0x006058D0 (feb 0x005DA3C0)
public:
    // ProcessInit v12 0x00497170 (feb 0x0048B640) loads the icon textures and finds the bars; offsets verified there.
    // ProcessUpdate v12 0x00497570 (feb 0x0048BA40) HOOK: every HUD number (health, armor, money, ammo, boss, powerups).
    uiELEMENT  *keyBar;          // v12 +0x6C (feb +0x6C) "key_bar" (always hidden in ProcessUpdate)       v12 0x004973DB
    uiELEMENT  *healthBar;       // v12 +0x70 (feb +0x70) "health_bar" (shows UI_HUD_GOD in god mode)     v12 0x004973EA
    uiELEMENT  *armorBar;        // v12 +0x74 (feb +0x74) "armor_bar"                                      v12 0x004973FC
    uiELEMENT  *moneyBar;        // v12 +0x78 (feb +0x78) "money_bar"                                      v12 0x0049740B
    uiELEMENT  *ammoBar;         // v12 +0x7C (feb +0x7C) "ammo_bar"                                       v12 0x0049741A
    uiELEMENT  *bossBar;         // v12 +0x80 (feb +0x80) "boss_bar" (aiPLANNER::GetBossHealth)           v12 0x00497429
    uiELEMENT  *powerup[3];      // v12 +0x84 (feb +0x84) "powerup01".."powerup03"
    uiELEMENT  *underwater;      // v12 +0x90 (feb +0x90) "underwater" (air bar)                           v12 0x00497434
    int         bossShown;       // v12 +0x94 (feb +0x94) init 1
    txmTEXTURE *texKey;          // v12 +0x98 (feb +0x98) key_4x4                                          v12 0x00497190
    txmTEXTURE *texHealth;       // v12 +0x9C (feb +0x9C) helf_4x4 (also the default boss icon)           v12 0x004971AA
    txmTEXTURE *texArmor[4];     // v12 +0xA0 (feb +0xA0) armor1_4x4..armor4_4x4
    txmTEXTURE *texAmmo[10];     // v12 +0xB0 (feb +0xB0) lopata(+0xB0) pulia(+0xB4) shotgun(+0xB8) mashinegun(+0xBC) crossbow(+0xC0)
                                 //   acid(+0xC4) bazooka(+0xC8) meduzagun(+0xCC) minigun(+0xD0) pwp_bmb(+0xD4)   v12 0x004971DE..0x004972E0
    txmTEXTURE *texAtomic;       // v12 +0xD8 (feb +0xD8) atomic_ammo_4x4                                  v12 0x00497294
    txmTEXTURE *texGold;         // v12 +0xDC (feb +0xDC) gold_4x4                                         v12 0x004971C4
    txmTEXTURE *texPower[3];     // v12 +0xE0 (feb +0xE0) power_qdam / power_shield / power_chasi          v12 0x0049733B..0x0049736D
    int         lastHealth;      // v12 +0xEC (feb +0xEC) cached values, -1 = redraw
    int         lastArmor;       // v12 +0xF0 (feb +0xF0)
    int         lastMoney;       // v12 +0xF4 (feb +0xF4)
    int         lastAmmo;        // v12 +0xF8 (feb +0xF8) ammo beyond the clip (or the count shown)
    int         _fc;             // v12 +0xFC ctor -1, not read in 1.2
    union { pwpWEAPON *lastWeapon; int inited; }; // v12 +0x100 the weapon shown (ProcessUpdate 0x00497E6D); feb: ProcessInit's done flag
    wrUIHUD_WINDOW();                                // v12 0x004970C0 (feb 0x0048B590)
    virtual int  GetElemDefType();                   // slot +0x04 v12 0x0048DF30 (shared "return 0"): untyped children are "area"
    virtual int  ProcessInit(void *p);               // slot +0x3C v12 0x00497170 (feb 0x0048B640)
    virtual int  ProcessUpdate(void *p);             // slot +0x44 v12 0x00497570 (feb 0x0048BA40)
    static uiELEMENT *__fastcall Create();           // v12 0x00498460 (feb 0x0048C890)
};
class wrUIHUD_UH_WINDOW : public uiELEMENT {         // "wr_hud_uh_wnd" 0x84 (feb 0x78), wrUI_SYSTEM+0x144: full-screen pain flash; chat line, ping (1.2)
public:                                              // vtbl v12 0x00605870; src/Sources/wr_ui_hud.cpp
    float      painTime;         // v12 +0x6C counts down (m3dUpdateTimeField) 0 at first
    float      painLen;          // v12 +0x70 the flash's length (1.0 at first): blood_pic alpha = 255 * painTime / painLen
    uiELEMENT *bloodPic;         // v12 +0x74 "blood_pic"
    int        chatTeam;         // v12 +0x78 1: the open chat line goes to the team (CHAT_TEAM)
    uiELEMENT *chatInput;        // v12 +0x7C "hud_chat_input" (1.2)
    uiELEMENT *ping;             // v12 +0x80 "ping" (1.2, [GameOpt] DisplayPing in MP mode 3)
    static uiELEMENT *__fastcall Create();                     // v12 0x00498480
    virtual int  GetElemDefType();                             // slot +0x04 v12 0x0048DF30 (shared "return 0")
    virtual int  StartAppearing(int how);                      // slot +0x20 v12 0x00496D60
    virtual int  ProcessInit(void *p);                         // slot +0x3C v12 0x00496BE0
    virtual int  ProcessUpdate(void *p);                       // slot +0x44 v12 0x00496DE0
    virtual void OnChildNotify(uiCHILD_NOTIFY *n);             // slot +0x5C v12 0x00496CA0
    void ShowPain(float time, int clear, unsigned long color); // v12 0x00496BA0 (feb 0x0048B4A0)
};
struct wrHUD_CON_LINE {                               // 12 bytes per console line (1.2; feb 8 without the flags)
    int   fading;          // +0 0 shown, 1 fading out
    float time;            // +4 time left (10 s shown, then 1.5 s fade)
    int   flags;           // +8 the ShowMsgW flags of the line (4: fade four times faster)
};
class wrUIHUD_MSG : public uiAREA {                  // "wrh_msg" 0x144 (feb 0x140), wrUI_SYSTEM+0x14C: centred messages + console lines
public:
    // vtable v12 0x00605A18 (25 slots): uiAREA's with ProcessInit +0x3C, ProcessUpdate +0x44, the deleting dtor and a
    // new slot +0x60 IsMsgShown.
    // render elements: [0, nCon) the "con_msg*" console lines (top to bottom), [nCon, nElems) the centred message lines
    int   fading;          // v12 +0x128 (feb +0x128) the message: 0 shown, 1 fading out (or gone)
    float time;            // v12 +0x12C (feb +0x12C) its time left (then a 1.5 s fade)
    int   msgFlags;        // v12 +0x130 NEW: the flags of the last ShowMsgW (4: fast fade)
    int   nCon;            // v12 +0x134 (feb +0x130) console lines
    wrHUD_CON_LINE *con;   // v12 +0x138 (feb +0x134) their state
    int   nConUsed;        // v12 +0x13C (feb +0x138) = nCon
    int   conCap;          // v12 +0x140 (feb +0x13C) records allocated
    static uiELEMENT *__fastcall Create();                     // v12 0x004981E0 (feb 0x0048C7E0)
    virtual ~wrUIHUD_MSG();                                    // slot +0x00 v12 0x00498250 (feb 0x0048C730)
    virtual int  ProcessInit(void *p);                         // slot +0x3C v12 0x004953A0 (feb 0x00489E50)
    virtual int  ProcessUpdate(void *p);                       // slot +0x44 v12 0x00495540 (feb 0x0048A010)
    virtual int  IsMsgShown();                                 // slot +0x60 v12 0x00495010 NEW: a message line is up
    void ShowMsgW(const wrWCHAR *s, float time, int prio); // v12 0x00495060 (feb 0x00489B70)
    void ClearMsg();                                 // v12 0x00494F50 (feb 0x00489AC0)
    void RemoveConMsgLine(int i);                    // v12 0x00495740 (feb 0x0048A1C0)
};
class wrUIHUD_DBGMSG : public uiAREA { };            // "wrh_dbg_msg": debug overlay (ShowFPS_PPF, ShowProfInfo), only when gsAppState & 0x8000 is CLEAR. ProcessInit v12 0x00495840
class wrUIHUD_ICONBAR : public uiAREA {              // "wrh_iconbar" 0x134: icon + number bar (health, armor, ammo...)
public:                                              // render element 0 = "text", 1 = "icon" (a 4 x 4 sheet)
    int   spins;           // v12 +0x128 (feb +0x128) full turns of the icon still to show; -1 = turn forever
    int   cell;            // v12 +0x12C (feb +0x12C) the sheet cell shown (0..15, 0 = at rest)
    float cellTimer;       // v12 +0x130 (feb +0x130) time to the next cell (0.0625 s)
    virtual void Init(objOBJ *o);                    // slot +0x38 v12 0x00494C70 (feb 0x00489840)
    virtual int  StartAppearing(int how);            // slot +0x20 v12 0x00494CC0 (feb 0x00489890)
    virtual int  ProcessUpdate(void *p);             // slot +0x44 v12 0x00494E20 (feb 0x004899F0)
    static uiELEMENT *__fastcall Create();           // v12 0x004983C0 (feb 0x0048C820)
protected:
    void UpdateTexture();  // v12 0x00494CF0 (feb 0x004898C0)
};
class wrUIHUD_SLIDER : public uiAREA {               // "wr_slider" 0x128 (sld_area / sld_box)
public:
    virtual void SetPosition(float pos);             // slot +0x60 v12 0x00494BB0 (feb 0x00489780) 0..1
    int ProcessInit(void *p);                        // v12 0x00494C50 (feb 0x00489820)
    static uiELEMENT *__fastcall Create();           // v12 0x00498180 (feb 0x0048C660)
};
class wrUIHUD_ZOOM : public uiAREA { };              // "wrh_scope_zoom" 0x128: crossbow zoom factor text UI_HUD_ZOOM; ProcessUpdate v12 0x00494F00
class wrUIHUD_CHAT_INPUT : public uiTEXT_INPUT { };  // "wrh_chat_input" 0x141 NEW in 1.0/1.2 (vtbl v12 0x00605670), CHAT_MSG_ALL/TEAM (cmds 0x2C/0x2D)
class wrUIHUD_SP_WINDOW : public uiELEMENT {         // "wr_hud_sp_wnd" 0x84, wrUI_SYSTEM+0x13C: the SP statistics (TAB held,
public:                                              //   or the level over); vtbl v12 0x00605810; src/Sources/wr_ui_hud.cpp
    uiELEMENT *spTime;           // v12 +0x6C "sp_time"
    uiELEMENT *spKills;          // v12 +0x70 "sp_kills"
    uiELEMENT *spSecrets;        // v12 +0x74 "sp_secrets"
    uiELEMENT *spMoney;          // v12 +0x78 "sp_money"
    uiELEMENT *spFavWpn;         // v12 +0x7C "sp_fav_wheapon"
    uiELEMENT *spAccuracy;       // v12 +0x80 "sp_accuracy"
    static uiELEMENT *__fastcall Create();                     // v12 0x00498590 (feb 0x0048C9B0)
    virtual int  GetElemDefType();                             // slot +0x04 v12 0x0048DF30 (shared "return 0": its children are areas)
    virtual int  ProcessInit(void *p);                         // slot +0x3C v12 0x004968C0 (feb 0x0048B1C0)
    virtual int  ProcessUpdate(void *p);                       // slot +0x44 v12 0x00496950 (feb 0x0048B250)
};
                                                     //   (UI_HUD_SP_MAP_*_FMT). ProcessInit v12 0x004968C0, ProcessUpdate v12 0x00496950
class wrUIHUD_MP_WINDOW : public uiELEMENT { };      // "wr_hud_mp_wnd" 0xB0: DM score table plrline%02i. ProcessUpdate v12 0x00495AD0
class wrUIHUD_COOP_WINDOW : public uiELEMENT { };    // "wr_hud_coop_wnd" 0xE4 (feb 0xAC). ProcessInit v12 0x00495E30
class wrUIHUD_TH_WINDOW : public uiELEMENT { };      // "wr_hud_th_wnd" 0xB4: treasure hunt teams UI_HUD_MP_TEAM_%i. ProcessUpdate v12 0x004965B0

// ================================================================================================ cursors
class wrCURSORS {                                    // wr_curs.obj (src/Sources/wr_curs.cpp). The crosshairs "cursor_%02i_%02i"
public:                                              //   ([GameOpt] Crosshair 0..5, a normal and an on-target kind) + the mouse cursor (12)
    int crosshair;                                   // +0x00 [GameOpt] Crosshair (6 and more: off)
    int type;                                        // +0x04 0 on a target, 1 normal
    static wrCURSORS *pInstance;                     // v12 0x0064FF40 (feb 0x0060EFC0)
    static int  __fastcall Init();                   // v12 0x004A5460 (feb 0x00498020) reads [GameOpt] Crosshair
    static void __fastcall Term();                   // v12 0x004A55B0 (feb 0x00498170)
    txmTEXTURE *GetCursorTexture(int crosshair, int type); // v12 0x004A55D0 (feb 0x00498190)
    void SetCursor(int crosshair);                   // v12 0x004A55F0 (feb 0x004981B0)
    void SetCursorType(int type);                    // v12 0x004A5620 (feb 0x004981E0)
};
extern wrCURSORS *_wrCursors;                        // v12 0x0064FF3C the same object as pInstance

// ================================================================================================ menu entities
// wrUI_WOOFER ("ui_snd_woofer", entity 'UIUI', feb ProcessINIT 0x004821A0): the menu's sounds: 4 UI channels and
// the three music tracks "i_wanna_rock" (looping), "intro", "outro". src/Sources/wr_ui.cpp.
// wrUI_SKULL ("wr_ui_cherep"): the animated skull in the menu scene.
class wrUI_WOOFER : public entENTITY {          // size v12 0x19F, vtable v12 0x00603FD0, Create v12 0x0048F680
public:
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // +0x04 v12 0x0048D120
    sncCHANNEL *chn[4];           // v12 +0x16B the UI sound channels (PlaySnd)
    int        chnSnd[4];         // v12 +0x17B the handle of the sound each plays (PlaySnd), -1 none
    int        _18B;              // v12 +0x18B the next PlaySnd handle
    sndMUSIC  *music[3];          // v12 +0x18F 0 "i_wanna_rock", 1 "intro", 2 "outro" (PlayAmbientMusic ids)
    int        curMusic;          // v12 +0x19B -1 none
    void ProcessINIT();                                  // v12 0x0048D1B0
    static entENTITY *__fastcall Create(animINST *i);    // v12 0x0048F680
};

extern sncSOUND_DESCR wrUISnd[7];                    // v12 0x0064F608 the UI sounds (wrUI_SYSTEM::PlaySnd(int) ids)
#pragma pack(pop)
