// Will Rock 1.2 unofficial SDK - platform_drv: application shell, window, message loop, timers, config (.ini),
// crash filter, cutscene video.  objs: gs_windows.obj, ap:ap_comm/ap_cfg/ap_wind/ap_gpf/ap_mem.obj, gs:gs_msg.obj
// (timer only), wr_cutscene.obj.  These are the Win32 pieces a port replaces besides the three drivers
// (render: re/sdk/engine_render/d3d_driver.h, sound: sound_drv.h, input: input.h).
// Assumes from re/sdk/core: msgSYSTEM (gsMsgCallFrame dispatch), apNAME.
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include "../math_app/app.h"   // gsAPP_STATE, gsWIN_APP_SYSTEM (integrator)
#include <stdint.h>

// ---------------------------------------------------------------- globals
extern void    *apHMainWindow;    // v12 0x00652018 (feb 0x0060F2D8) HWND of the game window (read by SetFocus in ActivateApp v12 0x004A4DD0)
extern void    *apHInstance;      // v12 0x0065201C (feb 0x0060F2DC) HINSTANCE (map twin, used by DirectInputCreateW)
// extern uint32_t gsAppState;       // v12 0x0098E824 (feb 0x00948224) app state bits (MainLoop v12 0x004A4C63)   [integrator: declared once in core/wr_core.h / engine_core/core_types.h]
extern class gsAPP_SYSTEM *gsAppSystem; // v12 0x0098E848 (feb 0x00948248)  // ? map twin
extern class gsMP_SYSTEM  *gsSysMP;     // v12 0x0098E850 (feb 0x00948250) (MainLoop v12 0x004A4C89)
extern class gsCUTSCENE   *gspCutscene; // v12 0x009A57CC (feb 0x0095CC3C) (WndProc v12 0x004A4E00)

// [integrator] gsAPP_STATE and gsWIN_APP_SYSTEM are defined authoritatively in math_app/app.h (included above).
// This copy is kept as documentation (its slot comments agree with app.h).
#if 0
enum gsAPP_STATE {                // gsAppState bits seen in MainLoop / WndProc
    GSAPP_ACTIVE        = 0x00000001,  // window active (render every frame)
    GSAPP_QUIT          = 0x00000008,  // leave MainLoop
    GSAPP_PAUSED        = 0x00000010,  // ?
    GSAPP_IN_PUMP       = 0x00000020,  // set while pumping messages
    GSAPP_NO_FRAME_MASK = 0x00A00100,  // any of these: skip gsMsgCallFrame  // ? meanings
    GSAPP_FOCUS_LOCK    = 0x00100000,  // ?
    GSAPP_CUTSCENE      = 0x00800000,  // cutscene playing: call gspCutscene slot +0x10 instead of a frame
    GSAPP_WAIT_INACTIVE = 0x01000000,  // inactive and not in MP server: WaitMessage() (Debug/SwitchWindow ini key)
};

// ---------------------------------------------------------------- window application
class gsWIN_APP_SYSTEM /* : gsAPP_SYSTEM */ {
public:
    // vtable v12 0x00605CC8 (feb 0x005DA7A8), 12 slots
    virtual ~gsWIN_APP_SYSTEM();          // slot +0x00 v12 0x004AC490 (feb 0x0049E0A0)
    virtual int  Init();                  // slot +0x04 v12 0x004A47E0 (feb 0x004973E0) InitCommonControls(Ex), RegisterClassEx(_gsWndProc),
                                          //   CreateWindowExA(style 0x880000, 640x480), ShowWindow, strip system menu
    virtual int  InitSubsys();            // slot +0x08 (base stub; wrAPP_SYSTEM overrides: registry, subsystems)
    virtual void Term();                  // slot +0x0C v12 0x004A4960 (feb 0x00497550) DestroyWindow, UnregisterClass
    virtual void TermSubsys();            // slot +0x10
    virtual void MainLoop();              // slot +0x14 v12 0x004A4C60 (feb 0x00497820) PeekMessage pump, slot +0x28, gsMsgCallFrame, input Acquire
    virtual int  UpdateLaunch();          // slot +0x18
    virtual int  IsAlreadyRunning();      // slot +0x1C v12 0x004A49A0 (feb 0x00497590) FindWindowA(class) + SetForegroundWindow
    virtual void ActivateApp();           // slot +0x20 v12 0x004A4DD0 (feb 0x00497990) SetFocus + SendMessage(0x401,1)
    virtual void ResizeWin();             // slot +0x24 v12 0x004A49D0 (feb 0x004975C0) window style/size for windowed vs fullscreen
    virtual void ProcessActDeact();       // slot +0x28 v12 0x004A4B10 (feb 0x004976D0) activate/deactivate: driver/sound/input acquire
    virtual char *GetIniFName();          // slot +0x2C

    char className[0x40];   // v12 +0x04 (feb +0x04) default "SABER3D_WIN_CLASS_NAME" (ctor v12 0x004A4770 +0x16)
    char windowName[0x40];  // v12 +0x44 (feb +0x44) default "SABER3D_WIN_NAME"
    int  iconResId;         // v12 +0x84 (feb +0x84) <0 = no icon (Init v12 0x004A4818)
};
#endif

long __stdcall _gsWndProc(void *hwnd, unsigned msg, unsigned wp, long lp); // v12 0x004A4E00 (feb 0x004979C0) forwards to gspCutscene first,
                                                                           //   WM_ACTIVATEAPP/0x401/WM_CHAR(0x102)/WM_SYSKEY..., DefWindowProcA
void __fastcall  _gsGetCurTime(int *y, int *mo, int *d, int *h, int *mi, int *s);    // v12 0x004A4580 (feb 0x00497180) GetLocalTime (save names)
float __fastcall _gswinGetDblClickTime();                                            // feb 0x00497210 (v12 ~0x004A4600, GetDoubleClickTime call 0x004A4615)
int __fastcall   _gsDetectDx9();                                                     // v12 0x004A4640 (feb 0x00497240) CLSID_DxDiagProvider,
                                                                          //   "DxDiag_SystemInfo"/"dwDirectXVersionMajor"; MessageBox if < 9

// ---------------------------------------------------------------- frame timer (gs_msg.obj)
void __fastcall gsMsgCallFrame();            // v12 0x00510B90 (feb 0x004FC7E0) one game frame (messages, update, render)
void __fastcall _gsMsgCalcElapsedTime();     // v12 0x005109A0 (feb 0x004FC610) QueryPerformanceCounter/Frequency delta, Sleep() limiter
// gsMsgInit (feb 0x004FC420) timeGetTime: v12 twin not located by tables (calls near v12 0x00510932/0x00510992)

// ---------------------------------------------------------------- ap layer (portable apart from the noted calls)
int  apInit();                    // v12 0x004AF370 (feb 0x004A07E0)
void apTerm();                    // v12 0x004AF3A0 (feb 0x004A0810)
void apMsg(char *fmt, ...);       // v12 0x004AF430 (feb 0x004A08A0)
void apLog(char *fmt, ...);       // v12 0x004AF480 (feb 0x004A08F0)  HOOK: route the engine log
void apLogErr(char *fmt, ...);    // v12 0x004AF4D0 (feb 0x004A0940)
// void *apMalloc(unsigned);  [integrator: declared once in engine_core/core_types.h as apMalloc(size_t)] // v12 0x004AF5F0 (feb 0x004A0A60)  CRT malloc wrappers
void  apFree(void *);             // v12 0x004AF610 (feb 0x004A0A80)
// .ini config (GetPrivateProfileString / WritePrivateProfileString; path = current dir + name)
int __fastcall  apCfgInit(char *iniName, int useCurDir);              // v12 0x004AF670 (feb 0x004A0AE0)
void __fastcall apCfgTerm();                                          // v12 0x004AF730 (feb 0x004A0BA0)
int __fastcall  _apCfgReadInt(void **cfg, char *sect, char *key, int *out);   // v12 0x004AF7E0 (feb 0x004A0C50)
int __fastcall  _apCfgReadDword(void **cfg, char *sect, char *key, uint32_t *); // v12 0x004AF890 (feb 0x004A0D00)
int __fastcall  _apCfgReadBool(void **cfg, char *sect, char *key, int *);     // v12 0x004AF940 (feb 0x004A0DB0)
int __fastcall  _apCfgReadFloat(void **cfg, char *sect, char *key, float *);  // v12 0x004AF9D0 (feb 0x004A0E40)
int __fastcall  _apCfgReadString(void **cfg, char *sect, char *key, char *, int); // v12 0x004AFA50 (feb 0x004A0EC0)
int __fastcall  apCfgWriteInt(char *sect, char *key, int);            // v12 0x004AFAD0 (feb 0x004A0F40)
int __fastcall  apCfgWriteDword(char *sect, char *key, uint32_t);     // v12 0x004AFB20 (feb 0x004A0F90)
int __fastcall  apCfgWriteBool(char *sect, char *key, int);           // v12 0x004AFB70 (feb 0x004A0FE0)
int __fastcall  apCfgWriteFloat(char *sect, char *key, float);        // v12 0x004AFBD0 (feb 0x004A1040)
int __fastcall  apCfgWriteString(char *sect, char *key, char *);      // v12 0x004AFC20 (feb 0x004A1090)
int __fastcall  _apCfgSectGetKey(void **, char *, char *, void *, int, int); // v12 0x004AFC60 (feb 0x004A10D0)
int __fastcall  _apCfgSectSetKey(char *, char *, void *, int, int);   // v12 0x004AFDE0 (feb 0x004A1250)
// message boxes (MessageBoxA)
int  mbShowOk(char *fmt, ...);    // v12 0x004B0160 (feb 0x004A15B0)
int  mbShowStop(char *fmt, ...);  // v12 0x004B01A0 (feb 0x004A15F0)
int  mbShowYesNo(char *fmt, ...); // v12 0x004B01E0 (feb 0x004A1630)
// crash filter (SetUnhandledExceptionFilter; writes a GPF report with GetModuleFileName/GetComputerName/GlobalMemoryStatus)
int  apGPF_FILTER_SetupFilter();  // v12 0x004B0630 (feb 0x004A1A80)
long __stdcall apGPF_FILTER_GPFExeptFunc(void *excPtrs); // feb 0x004A1AA0 (v12 ~0x004B0640.., not table-matched)
// Win32 dialog helpers (ap_wind.obj) used only by the developer config dialogs: SetCBItemState v12 0x004B0220,
// GetCBItemState 0x004B0250, SetDlgItemFloat 0x004B0270, GetDlgItemFloat 0x004B02C0, TabControl_AddPage 0x004B02F0,
// _ListView_* 0x004B0350..0x004B05A0. A port can drop them.

// ---------------------------------------------------------------- cutscene video (DirectShow, not Bink)
// Movies: "..\movies\ubilogo.mpg", "..\movies\saber3d.mpg" (MPEG-1), played by a FilterGraph rendering into
// IVideoWindow owned by apHMainWindow, centred 640x480.
class wrCUTSCENE {
public:
    void Update();                          // v12 0x004A58A0 (feb 0x00498460)
    void Play(int which);                   // v12 0x004A5910 (feb 0x004984D0) CoCreateInstance(CLSID_FilterGraph, IID_IGraphBuilder),
                                            //   QI IMediaControl/IVideoWindow/IMediaEventEx, PlayMedia, CoUninitialize on error
    void Abort();                           // v12 0x004A5B00 (feb 0x004986C0) IMediaControl::Stop + release
    void NotifyMsgHandler(void *, unsigned, int, int); // v12 0x004A5BA0 (feb 0x00498760) IVideoWindow::NotifyOwnerMessage
};
long PlayMedia(char *file, void *hinst);    // v12 0x004A5670 (feb 0x00498230) RenderFile, put_Owner, SetWindowPosition, Run
