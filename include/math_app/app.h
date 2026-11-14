// app.h -- Will Rock UNOFFICIAL modding SDK, subsystem "math_app" (part 2: application framework)
//
// WinMain, the application objects (gsAPP_SYSTEM -> gsWIN_APP_SYSTEM -> wrAPP_SYSTEM), subsystem creation order,
// gsRun / InitAll / MainLoop / TermAll, the window procedure, the frame clock, gsAppState flags, command line,
// crash handler (apGPF_FILTER) and the small Win32 helper layer of ap.lib (ap_wind.obj).
// Static analysis only. Target 1.2 (v12.exe); reference Feb 2003 (feb.exe + map + decompile).
// Conventions: see m3d.h. "// ?" = not verified by a v12 instruction.
//
// Assumed from re/sdk/core (engine_core/core_types.h): apNAME, apMalloc/apFree/apRealloc, apLog/apLogErr,
// apCfgInit/_apCfgRead*/apCfgWrite*, msgSYSTEM (msgSystem). Only forward-declared here:
class gsAPP_SYSTEM; class gsLVL_SYSTEM; class gsRENDER_SYSTEM; class gsINP_SYSTEM; class gsSND_SYSTEM;
class gsVIS_SYSTEM; class pshSHADOW_SYSTEM; class mrrMIRROR_SYSTEM; class uiUI_SYSTEM; class uiFONT_MANAGER;
class gsSTRINGS; class gsCFG_SYSTEM; class gsMP_SERVER; class gsMP_CLIENT; class gsMP_SYSTEM; class gsCUTSCENE;
class wrSTAT_SYSTEM; class camCAMERA;
struct HINSTANCE__; struct HWND__; struct _EXCEPTION_POINTERS;

#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
typedef int BOOL32;

// =====================================================================================================
// Entry point
// =====================================================================================================
// v12: _WinMain@16 = 0x004AA010 (feb 0x0049BDC0). The CRT entry is _WinMainCRTStartup (feb 0x005687A8).
// v12 code before 0x004AA010 (0x004AA000) is the static initialiser that sets wrGpfFilter's vtable.
int __stdcall WinMain(HINSTANCE__ *hInst, HINSTANCE__ *hPrev, char *cmdLine, int nShow); // v12 0x004AA010 (feb 0x0049BDC0)
//  1. apHInstance = hInst; wrAppCmdLine = cmdLine; wrAppState = 0; apGPF_FILTER::pFilter = &wrGpfFilter
//  2. gsAppSystem = new wrAPP_SYSTEM (0x94) via gsWIN_APP_SYSTEM("WR_WIN_CLASS","Will Rock", icon 0x67)
//  3. v12 ONLY: if (gsAppSystem->IsAlreadyRunning()) exit   (feb did this inside gsRun)
//  4. new: gsStrings(wrSTRINGS 0x30; feb 0x34), gsSysLevel(wrLVL_SYSTEM 0x3D90), gsSysRender(wrRENDER_SYSTEM 0x3C),
//     gsSysInput(wrINP_SYSTEM 0x70), uiSystem(wrUI_SYSTEM 0x16C), fontManager(0x10), gsSysSound(wrSND_SYSTEM 0xF4; feb 0xF8),
//     gsSysCfg(0x0C), gsSysVis(gsVIS_SYSTEM 0x3D4; feb 0x3D0), gsSysProjShd(wrSHADOW_SYSTEM 0x58C4),
//     gsSysMirror(0x4414), wrSysStat(wrSTAT_SYSTEM 0x144; feb 0x140), gsSysServer(gsMP_SERVER 0x2CD4; feb 0xA00),
//     gsSysClient(gsMP_CLIENT 0x281C; feb 0x510), gspCutscene(wrCUTSCENE 4)
//  5. gsAppState |= 0x8000; v12 ONLY: if strstr(cmdLine,"-dedicated") gsAppState |= GSAPP_DEDICATED (0x08000000)
//  6. gsRun(); then delete every subsystem in the same order and return 0.
// A replacement launcher / port re-implements exactly this list (sizes above are the v12 object sizes).

void __fastcall gsRun();            // v12 0x0050D030 (feb 0x004FA2E0) if (InitAll()) { MainLoop(); TermAll(); }  (v12: no IsAlreadyRunning here)
void __fastcall gsMsgStopLoop();    // v12 0x00510980 (feb 0x004FC600) gsAppState |= GSAPP_QUIT -> MainLoop exits. Call it to quit cleanly.
void __fastcall gsMsgCallFrame();   // v12 0x00510B90 (feb 0x004FC7E0) ONE GAME FRAME: _gsMsgCalcElapsedTime, FPS stat, _gsMsgProcessFrameProfile
void _gsMsgProcessFrameProfile(); // v12 0x00510CB0 (feb ?) msgSYSTEM::DispatchAll (v12 0x004D01A0) + per-system frame processing
void __fastcall _gsMsgCalcElapsedTime();     // v12 0x005109A0 (feb 0x004FC610) frame clock, see "Frame clock" below
BOOL32 __fastcall _gsDetectDx9();            // v12 0x004A4640 (feb 0x00497240) exit(0) from wrAPP_SYSTEM::Init when DirectX 9 is missing

// =====================================================================================================
// Application objects
// =====================================================================================================
class gsAPP_SYSTEM {                                  // abstract base, size 0x04 (only the vptr)
public:
    virtual void *__vecDelDtor(unsigned flags);       // slot +0x00
    virtual BOOL32 Init() = 0;                        // slot +0x04 create window (called inside InitAll after apCfgInit)
    virtual BOOL32 InitSubsys();                      // slot +0x08 game-specific subsystems (late in InitAll)
    virtual void   Term() = 0;                        // slot +0x0C
    virtual void   TermSubsys();                      // slot +0x10
    virtual void   MainLoop() = 0;                    // slot +0x14
    virtual char  *GetIniFName() = 0;                 // slot +0x18 name passed to apCfgInit
    virtual BOOL32 IsAlreadyRunning();                // slot +0x1C
    virtual void   ActivateApp();                     // slot +0x20 called at the end of InitAll
    virtual void   ResizeWin();                       // slot +0x24
    virtual void   ProcessActDeact();                 // slot +0x28 focus / alt-tab handling, once per loop
    virtual BOOL32 UpdateLaunch();                    // slot +0x2C called each loop while active+fullscreen ?
    BOOL32 InitAll();                                 // v12 0x0050D0A0 (feb 0x004FA360) whole engine init, order below
    void   TermAll();                                 // v12 0x0050D4A0 (feb 0x004FA760) reverse order
};
// vtable gsAPP_SYSTEM v12 0x00607A44 (feb 0x005DC594); ctor v12 0x0050D060 (feb 0x004FA320)

class gsWIN_APP_SYSTEM : public gsAPP_SYSTEM {        // Win32 window + message pump; size 0x90
public:
    char className[0x40];                             // v12 +0x04 (feb +0x04) ctor v12 0x004A478E "lea eax,[esi+4]"; default "SABER3D_WIN_CLASS_NAME"
    char windowName[0x40];                            // v12 +0x44 (feb +0x44) ctor v12 0x004A47AD; default "SABER3D_WIN_NAME"
    int  iconResId;                                   // v12 +0x84 (feb +0x84) ctor v12 0x004A47C5; <0 = no icon
    int  _88, _8C;                                    // ? not touched by the ctor
    gsWIN_APP_SYSTEM(char *cls, char *title, int icon); // v12 0x004A4770 (feb 0x00497370)
    // overrides
    BOOL32 Init();             // v12 0x004A47E0 (feb 0x004973E0) InitCommonControls(Ex), RegisterClassEx(_gsWndProc),
                               //   CreateWindowEx(0, cls, title, style, 0,0,640,480); style feb 0x00880000,
                               //   v12 0x10880000 (adds WS_VISIBLE); v12 also saves GetWindowLong(GWL_STYLE) in
                               //   gsWinOrigStyle (v12 0x0064FF30, used by ResizeWin); removes 10 system-menu items.
    void   Term();             // v12 0x004A4960 (feb 0x00497550) ShowWindow(0), DestroyWindow, UnregisterClass
    BOOL32 IsAlreadyRunning(); // v12 0x004A49A0 (feb 0x00497590) FindWindow(className) -> bring to front, return 1
    void   ResizeWin();        // v12 0x004A49D0 (feb 0x004975C0) [Video] FullScr/SizeX/SizeY -> MoveWindow or full-screen SetWindowPos
    void   ProcessActDeact();  // v12 0x004A4B10 (feb 0x004976D0) alt-tab: gsVideoTermHW / gsVideoSetHWMode, sound mute
    void   MainLoop();         // v12 0x004A4C60 (feb 0x00497820) see "Main loop"
    void   ActivateApp();      // v12 0x004A4DD0 (feb 0x00497990) SetFocus + SendMessage(0x401)
};
// vtable gsWIN_APP_SYSTEM v12 0x00605CC8 (feb 0x005DA7A8)

class wrAPP_SYSTEM : public gsWIN_APP_SYSTEM {        // Will Rock game application; size 0x94
public:
    int _90;                                          // v12 +0x90 (feb +0x90) zeroed in WinMain (v12 0x004AA05D) ?
    char  *GetIniFName();      // slot +0x18 v12 0x004AA610 (feb 0x0049C390) returns "WILLROCK.INI" (v12 string 0x00632EC4)
    BOOL32 Init();             // slot +0x04 v12 0x004AA620 (feb 0x0049C3A0) v12: wrSTRINGS::Init first (moved here from
                               //   InitSubsys), _gsDetectDx9 (exit if 0), "-safemode" -> [CONFIG] IsScaleInited=No,
                               //   IsScaleMinimal=Yes, then gsWIN_APP_SYSTEM::Init
    BOOL32 InitSubsys();       // slot +0x08 v12 0x004AA6A0 (feb 0x0049C400) gmSharedInit, wrCURSORS::Init, plrInit,
                               //   render+menu windows, aiMino/Cent/Bird/Satyr/aisStatue/Perseus/Sphinx/aiSkeletonsAll/
                               //   GladiatorsAll/Tiger/Amur/Rat/Cyclop/Cerber/Gefest/Croc/Medusa/Zevs Init, dntInit,
                               //   wrsfxInit, wrsobInit, wrsfxDestrInit, wrMsgInit, wrStatInit, wrItemInit, wrGOInit,
                               //   then HKLM\SOFTWARE\Saber Interactive\Will Rock "Language"=="German" -> wrAppState |= 1|2
    void   TermSubsys();       // slot +0x10 v12 0x004AACC0 (feb 0x0049C9E0)
    void   ActivateApp();      // slot +0x20 v12 0x004AAD90 (feb 0x0049CAD0)
    BOOL32 UpdateLaunch();     // slot +0x2C v12 0x004AADD0 (feb 0x0049CB10)
};
// vtable wrAPP_SYSTEM v12 0x00606230 (feb 0x005DAD08) -- 12 slots, slot order identical in both builds:
//   +0x00 0x004AC490 dtor | +0x04 0x004AA620 Init | +0x08 0x004AA6A0 InitSubsys | +0x0C 0x004A4960 Term |
//   +0x10 0x004AACC0 TermSubsys | +0x14 0x004A4C60 MainLoop | +0x18 0x004AA610 GetIniFName |
//   +0x1C 0x004A49A0 IsAlreadyRunning | +0x20 0x004AAD90 ActivateApp | +0x24 0x004A49D0 ResizeWin |
//   +0x28 0x004A4B10 ProcessActDeact | +0x2C 0x004AADD0 UpdateLaunch

// gsAPP_SYSTEM::InitAll order (v12 0x0050D0A0, identical to feb; any "0" return -> apMsgBox("%s() failed") + QUIT):
//   CoInitialize(0) -> apInit -> fnmInit -> apCfgInit(GetIniFName(),1) -> gsAppState|=INIT -> app->Init (window)
//   -> m3dInit -> pctInit -> objInit -> psSHEET::mode|=1 -> cdtoInit -> fntInit -> msgSystem.Init -> gsSysInput->Init
//   -> gsMPInit -> gsVideoInit -> gsVideoApplySettings -> partInit -> pteInit -> pteSpotInit -> pteFogInit
//   -> pteEnvMapInit -> gsCameraPtr = new camCAMERA(0x20C) -> gsMsgInit -> gscInit -> gsSysRender->Init
//   -> gsSysSound->Init -> gsSysLevel->Init -> gssInit -> entInit -> dynInit -> physInit -> gsSysVis->Init (v12 slot +0x78,
//   feb +0x74) -> gsSysProjShd->Init -> gsSysMirror->Init -> app->InitSubsys -> uiSystem->Init (v12 slot +0x34, feb +0x30)
//   -> gsSysMP->Init -> gspInit -> gslInit -> gsAppState = (gsAppState & ~INIT) | ACTIVE -> app->ActivateApp.
// HOOK: wrAPP_SYSTEM::InitSubsys (vtable slot +0x08 at v12 0x00606238) is the place to register new entity classes
//   (entRegisterClass) after the game's own and before the UI/level start.

// =====================================================================================================
// Main loop  (gsWIN_APP_SYSTEM::MainLoop v12 0x004A4C60)
// =====================================================================================================
// while (!(gsAppState & GSAPP_QUIT)) {
//     background = !gsSysMP || gsSysMP->state==1 || ([Debug] SwitchWindow == No) ; set/clear GSAPP_BACKGROUND
//     gsAppState |= GSAPP_PUMPING; PeekMessage/TranslateMessage/DispatchMessage until empty;
//     this->ProcessActDeact();
//     if (BACKGROUND && !ACTIVE) WaitMessage();             // sleeps while minimised in single player
//     gsAppState &= ~GSAPP_PUMPING;
//     if (CUTSCENE_PLAYING && gspCutscene) gspCutscene->Update()   (vtable +0x10)
//     else if (ACTIVE&&FULLSCREEN&&!(INIT|QUIT)) gsAppSystem->UpdateLaunch() ...
//     else if (no 0xA00100 / INIT / QUIT bits) { gsMsgCallFrame(); gsSysInput->Acquire(); }
// }
// The game is single threaded: one gsMsgCallFrame per loop iteration, no fixed-step simulation.

long __stdcall _gsWndProc(HWND__ *h, unsigned msg, unsigned wp, long lp); // v12 0x004A4E00 (feb 0x004979C0)
//   every message first goes to gspCutscene (vtable +0x14).  WM_ACTIVATE(6) -> gsSysInput->Acquire;
//   WM_CLOSE(0x10) -> gsMsgStopLoop; WM_ERASEBKGND(0x14) / WM_NCHITTEST(0x84) / WM_INPUTLANGCHANGEREQUEST(0x50) handled;
//   WM_ACTIVATEAPP(0x1C) -> ACTIVE/FULLSCREEN bits; WM_KEYDOWN/UP(0x100/0x101) -> Esc / console keys, gsVideoShowOptions;
//   WM_CHAR(0x102) -> uiSystem vtable +0x28 (text input); WM_SYSKEYDOWN(0x104) Alt+Enter -> gsVideoSetHWMode(1)
//   (window/fullscreen toggle); WM_SYSCOMMAND(0x112) blocks SC_SCREENSAVE/SC_MONITORPOWER/SC_MOVE/SC_SIZE/
//   SC_KEYMENU...; WM_LBUTTONDOWN/RBUTTONDOWN -> skip cutscene; 0x401 (WM_USER+1) = activate request.
//   v12 jump table: index table 0x004A5244, targets 0x004A5220.

// =====================================================================================================
// Globals
// =====================================================================================================
extern gsAPP_SYSTEM    *gsAppSystem;      // v12 0x0098E848 (feb 0x00948248)
// extern int              gsAppState;       // v12 0x0098E824 (feb 0x00948224)  GSAPP_* flags below   [integrator: declared once in core/wr_core.h / engine_core/core_types.h]
extern int              wrAppState;       // v12 0x0064FFA8 (feb 0x0060F018) 1 = German build/registry, 2 = NO BLOOD (tested by plrBLOOD_HANDS, aiNPC_WR::StartBlood)
extern char            *wrAppCmdLine;     // v12 0x0064FF9C (feb 0x0060F00C) raw lpCmdLine
// extern HINSTANCE__     *apHInstance;      // v12 0x0065201C (feb 0x0060F2DC)   [integrator: declared once in core/wr_core.h / engine_core/core_types.h]
// extern HWND__          *apHMainWindow;    // v12 0x00652018 (feb 0x0060F2D8)   [integrator: declared once in core/wr_core.h / engine_core/core_types.h]
// extern char             apTitle[];        // v12 0x00652020 (feb 0x0060F2E0) ?   [integrator: declared once in core/wr_core.h / engine_core/core_types.h]
extern char             apBuffer[];       // v12 0x00654160 (feb 0x0060FF80) shared sprintf buffer ?
extern unsigned         gsWinOrigStyle;   // v12 0x0064FF30 (new in 1.2) GWL_STYLE saved by gsWIN_APP_SYSTEM::Init
extern gsLVL_SYSTEM    *gsSysLevel;       // v12 0x0098E84C
extern gsRENDER_SYSTEM *gsSysRender;      // v12 0x0098E844
extern gsINP_SYSTEM    *gsSysInput;       // v12 0x0098E838
extern gsSND_SYSTEM    *gsSysSound;       // v12 0x0098E830
extern gsVIS_SYSTEM    *gsSysVis;         // v12 0x0098E83C
extern mrrMIRROR_SYSTEM*gsSysMirror;      // v12 0x0098E820
extern gsMP_SYSTEM     *gsSysMP;          // v12 0x0098E850
extern gsMP_SERVER     *gsSysServer;      // v12 0x0098E870
extern gsMP_CLIENT     *gsSysClient;      // v12 0x0098E860
extern pshSHADOW_SYSTEM*gsSysProjShd;     // v12 0x00994550
extern wrUI_SYSTEM     *uiSystem;         // v12 0x00994568
extern gsSTRINGS       *gsStrings;        // v12 0x009946DC
extern uiFONT_MANAGER  *fontManager;      // v12 0x0099D808
extern gsCFG_SYSTEM    *gsSysCfg;         // v12 0x009A5BB4 (stored by WinMain v12 0x004AA1F1; symbol not resolved by the tool)
extern gsCUTSCENE      *gspCutscene;      // v12 0x009A57CC
extern wrSTAT_SYSTEM   *wrSysStat;        // v12 0x006500D0

enum gsAPP_STATE_FLAGS {                  // gsAppState bits (meaning from MainLoop/ProcessActDeact/_gsWndProc/InitAll)
    GSAPP_ACTIVE        = 0x00000001,     // initialised and focused
    GSAPP_QUIT          = 0x00000008,     // set by gsMsgStopLoop / init failure; MainLoop exits
    GSAPP_INIT          = 0x00000010,     // inside InitAll
    GSAPP_PUMPING       = 0x00000020,     // inside the PeekMessage loop
    GSAPP_NO_ALTENTER   = 0x00008000,     // set by WinMain: _gsWndProc only toggles fullscreen on Alt+Enter when this bit
                                          //   is CLEAR (v12 0x004A51A5 "test ah,ah"). Clear the bit to re-enable Alt+Enter.
    GSAPP_FULLSCREEN    = 0x00100000,     // ?
    GSAPP_SWITCHING     = 0x00200000,     // alt-tab / video mode change in progress
    GSAPP_CUTSCENE      = 0x00800000,     // cutscene owns the frame
    GSAPP_BACKGROUND    = 0x01000000,     // may WaitMessage when inactive
    GSAPP_REINIT_VIDEO  = 0x04000000,     // ?
    GSAPP_DEDICATED     = 0x08000000      // v12 only: "-dedicated" on the command line (tested by wrUI_SYSTEM::Update,
                                          //   wrUI_MP_WND, wrUI_MSGBOX_WINDOW, wrGO*)
};

// Command line switches recognised (strstr on wrAppCmdLine, nothing else parses it):
//   "-safemode"  (wrAPP_SYSTEM::Init)  resets the auto-detected scale/quality to minimal
//   "-dedicated" (WinMain, v12 only)   dedicated server mode

// =====================================================================================================
// Frame clock  (_gsMsgCalcElapsedTime v12 0x005109A0)
// =====================================================================================================
// extern unsigned gsTime; [integrator: declared once as u32 in engine_core/core_types.h]                   // v12 0x0098EA14 (feb 0x009482FC) timeGetTime() of this frame (ms)
extern float    gsElapsedTime;            // v12 0x0098E9F4 (feb 0x009482DC) frame delta in seconds (what every Process uses)
extern float    gsElapsedTimeLevel;       // v12 0x0098EA1C (feb 0x00948304) accumulated level time
extern float    gsTimeScale;              // v12 0x006354E8 (=1.0, .data) delta multiplier -> game speed. Name is ours.
extern float    gsNextFrameTime;          // v12 0x006354EC (=-1.0) v12 only: if >0 used once as the next delta, then reset to -1
extern float    gsFixedFrameTime;         // v12 0x0098EA04 if non-zero, every delta = this value (fixed-step/demo) ?
// Algorithm: delta from QueryPerformanceCounter (fallback timeGetTime*0.001); if delta < 0.005 s it Sleep()s the rest
// (=> hard 200 FPS cap, const v12 0x005FB650 is a SHARED __real@3ba3d70a); delta *= gsTimeScale; clamp to 0.1 s
// (immediate in v12 0x00510B04 + shared const 0x005F91BC); then the fixed/next-frame overrides.

// =====================================================================================================
// Crash handler (ap_gpf.obj + wr_main.obj)
// =====================================================================================================
class apGPF_FILTER {
public:
    virtual char *GetFileName();                      // slot +0x00 wr: v12 0x004AC4C0 (feb 0x0049E0D0) "WillRock_GPF.log"
    virtual void  DumpInfo(void *ctx, char *buf);     // slot +0x04 wr: v12 0x004ABF20 (feb 0x0049DB70) appends video driver info
    static BOOL32 SetupFilter();                      // v12 0x004B0630 (feb 0x004A1A80) SetUnhandledExceptionFilter(GPFExeptFunc), GlobalMemoryStatus
    static long __stdcall GPFExeptFunc(_EXCEPTION_POINTERS *e); // v12 0x004B0650 ? (feb 0x004A1AA0) writes "GPF occured at 0x%08X", build date, memory, computer name
    static apGPF_FILTER *pFilter;                     // v12 0x00656180 (feb 0x00611FA0)
    static int isInFilter;                            // v12 0x00656184 ? (feb 0x00611FA4)
};
// wrGPF_FILTER instance v12 0x0064FFA4 (vtable v12 0x00606038, feb 0x005DAB0C); v12 MEMORYSTATUS at 0x00656188.

// =====================================================================================================
// ap.lib remaining helpers (ap_comm.obj / ap_mem.obj / ap_wind.obj).  ap_cfg / log / memory: see engine_core.
// =====================================================================================================
BOOL32 apInit();                                      // v12 0x004AF370 (feb 0x004A07E0)
void   apTerm();                                      // v12 0x004AF3A0 (feb 0x004A0810)
void   apMsgBox(char *fmt, ...);                      // v12 0x004AF3E0 (feb 0x004A0850) error box (tool labels it "plrPLAYER_COMMON::ProcessTERM": wrong)
void   apMsg(char *fmt, ...);                         // v12 0x004AF430 (feb 0x004A08A0)
void __fastcall   apSetMsgBoxCB(void (*cb)(char *, ...));        // v12 0x004AF540 ? (feb 0x004A09B0) one of four 16-byte setters at v12 0x004AF540..0x004AF570
char  *apStrdup(char *s);                             // v12 0x004AF630 (feb 0x004A0AA0)
void  *apCalloc(unsigned n, unsigned sz);             // v12 0x004AF600 (feb 0x004A0A70)
char  *wifOpenFile(HWND__ *w, char *title, char *filter, char *ext, char *dir, char *name, unsigned flags); // v12 0x004B0020 ? (feb 0x004A1490) GetOpenFileName wrapper
int    mbShowOk(char *fmt, ...);                      // v12 0x004B0160 (feb 0x004A15B0) MessageBox wrappers
int    mbShowStop(char *fmt, ...);                    // v12 0x004B01A0 (feb 0x004A15F0)
int    mbShowYesNo(char *fmt, ...);                   // v12 0x004B01E0 (feb 0x004A1630)
// dialog helpers (tools/editor dialogs; same in both builds): SetCBItemState v12 0x004B0220, GetCBItemState 0x004B0250,
// SetDlgItemFloat 0x004B0270, GetDlgItemFloat 0x004B02C0, TabControl_AddPage 0x004B02F0, _ListView_AddColumn 0x004B0350,
// _ListView_GetMenuPos 0x004B0420, _ListView_AddItem 0x004B04D0, _ListView_GetSelected 0x004B0550,
// _ListView_GetSelectedData 0x004B05A0  (feb 0x004A1670..0x004A19F0).
