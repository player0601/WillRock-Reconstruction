// ap.h -- private header of ap.lib (Will Rock 1.2, v12.exe), the engine's "application layer":
//   ap_comm.obj  init/term, message/log forwarding, apNAME, the app globals
//   ap_mem.obj   malloc/free wrappers
//   ap_cfg.obj   cached INI reader/writer (WILLROCK.INI)
//   ap_gpf.obj   unhandled-exception filter (crash log)
//   ap_wind.obj  small Win32 dialog/common-control helpers
// Original path: C:\PROJECTS\WillRock\Sources\..\ap\ (decompile header "ap\ap_comm.cpp").
//
// The SDK (src_recon/include) is included for the shared declarations. Two of its declarations do not
// match the real definitions (see re/work/recon_fixes/03_ap.md), so they are renamed while the SDK is
// parsed and redeclared correctly below:
//   apTitle        is a char[64] array (mbShow* push its address), not a char*
//   apHMainWindow  has C linkage (map symbol "_apHMainWindow", no C++ decoration)
#pragma once

#ifndef WR_TODO
#define WR_TODO(a) ((void)0)
#endif

#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <io.h>

#define apTitle       apTitle_SDK_decl
#define apHMainWindow apHMainWindow_SDK_decl
#include "wr_sdk.h"
#undef apTitle
#undef apHMainWindow

typedef void (*apMSG_CB)(char *, ...);

// ---------------------------------------------------------------- ap_comm globals
#define AP_TITLE_LEN     0x40
#define AP_LOG_FILES     10
#define AP_MSG_BUF_LEN   0x200
#define AP_BUFFER_LEN    0x1000

// One slot of the log-file table cleared by apInit and closed by apTerm. ?
// Nothing in 1.2 opens these files any more (the opener was removed from ap_comm with the debug build). ?
struct apLOG_FILE {
    FILE *fp;                            // +0x00
    char  name[0x104];                   // +0x04 MAX_PATH; "" = slot unused
};                                       // size 0x108

extern "C" HWND  apHMainWindow;          // v12 0x00652018 (feb 0x0060F2D8)
extern char      apTitle[AP_TITLE_LEN];  // v12 0x00652020 (feb 0x0060F2E0)
extern apLOG_FILE apLogFiles[AP_LOG_FILES]; // v12 0x00652068 (feb 0x0060F328)  name is ours
extern char      apMsgBuf[AP_MSG_BUF_LEN];  // v12 0x00653F58 (feb 0x0060FD78)  vsprintf target of apMsg*/apLog*; name is ours

// ---------------------------------------------------------------- ap_mem globals
extern int _apTotalAlloced;              // v12 0x00655168 ? (feb 0x00610F88) never updated in release

// ---------------------------------------------------------------- ap_cfg
// The INI cache: one section record per [section] ever read or written, each holding the keys.
enum apCFG_TYPE {
    APCFG_BOOL   = 1,
    APCFG_INT    = 2,
    APCFG_DWORD  = 3,
    APCFG_FLOAT  = 4,
    APCFG_STRING = 5,
    APCFG_BINARY = 6,
};

struct apCFG_KEY {
    apNAME name;                         // +0x00
    int    type;                         // +0x40 apCFG_TYPE
    int    size;                         // +0x44 4 for scalars, strlen for strings, byte count for binary
    union {                              // +0x48
        int    i;
        u32    dw;
        float  f;
        char  *str;                      //  APCFG_STRING: apStrdup'ed
        void  *data;                     //  APCFG_BINARY: apMalloc'ed
    } v;
};                                       // size 0x4C (apCalloc(1,0x4C))

struct apCFG_SECT {
    apNAME      name;                    // +0x00
    int         nKeys;                   // +0x40
    apCFG_KEY **keys;                    // +0x44 apRealloc'ed array of apCalloc'ed keys
};                                       // size 0x48

extern int         apCfgNSects;          // v12 0x00655170 (feb 0x00610F90)  names are ours
extern apCFG_SECT *apCfgSects;           // v12 0x00655174 (feb 0x00610F94)
extern char        apCfgFile[MAX_PATH];  // v12 0x00655178 (feb 0x00610F98)  full path of the INI

BOOL32 __fastcall _apCfgSectGetKey(void **cache, char *sect, char *key, void *out, int type, int size); // v12 0x004AFC60
BOOL32 __fastcall _apCfgSectSetKey(char *sect, char *key, void *val, int type, int size);              // v12 0x004AFDE0
BOOL32 __fastcall apCfgWriteDword (char *sect, char *key, u32 v);     // v12 0x004AFB20
BOOL32 __fastcall apCfgWriteBool  (char *sect, char *key, int v);     // v12 0x004AFB70
BOOL32 __fastcall apCfgWriteFloat (char *sect, char *key, float v);   // v12 0x004AFBD0
BOOL32 __fastcall apCfgWriteString(char *sect, char *key, char *v);   // v12 0x004AFC20

// ---------------------------------------------------------------- ap_gpf
extern MEMORYSTATUS apGpfStartupMem;     // v12 0x00656188 (feb 0x00611FA8)  name is ours

// ---------------------------------------------------------------- ap_wind
extern OPENFILENAMEA wifOfn;             // v12 0x00656010 (feb 0x00611E30)  names are ours
extern char wifFileName[0x100];          // v12 0x00655980 (feb 0x006117A0)
extern char wifFileTitle[0x400];         // v12 0x00655A80 (feb 0x006118A0) ?size

void   SetCBItemState(HWND dlg, UINT id, int on);              // v12 0x004B0220
int    GetCBItemState(HWND dlg, UINT id);                      // v12 0x004B0250
void   SetDlgItemFloat(HWND dlg, UINT id, float v);            // v12 0x004B0270
float  GetDlgItemFloat(HWND dlg, UINT id);                     // v12 0x004B02C0
void   __fastcall TabControl_AddPage(HWND dlg, int id, int idx, char *text);              // v12 0x004B02F0
int    __fastcall _ListView_AddColumn(HWND lv, int col, char *text, int width, int image); // v12 0x004B0350
void   __fastcall _ListView_GetMenuPos(HWND lv, POINT *pt);                                // v12 0x004B0420
DWORD  __fastcall _ListView_AddItem(HWND lv, int item, int sub, char *text, int image, DWORD data); // v12 0x004B04D0
int    __fastcall _ListView_GetSelected(HWND lv);                                         // v12 0x004B0550
DWORD  __fastcall _ListView_GetSelectedData(HWND lv);                                     // v12 0x004B05A0
