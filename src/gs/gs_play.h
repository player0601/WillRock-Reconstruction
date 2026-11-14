// gs_play.h -- Will Rock 1.2 (v12.exe, 2003-07-30) game start/stop ("play system")
//
// Reconstructed documentation source for modders. Not compilable into the game.
// Reference: Feb 2003 build (feb.exe + WillRock.map, obj gs:gs_play.obj).
// "// 1.2:" = changed or new versus feb.  "// ?" = unknown / unverified.
//
// ---------------------------------------------------------------------------
// Types assumed from other files (not defined here):
//   msgADDR, msgDATA, msgSYSTEM msgSystem (v12 0x008FA960): SendAll 0x004D06F0,
//       SendMsg 0x004D03C0, PostMsg 0x004D0410, PostMsgData 0x004D04C0,
//       InsertAddr 0x004D0820, DestroyAddr 0x004D08A0
//   gsMP_SYSTEM *gsSysMP (0x0098E850), gsMP_MODE, GS_APP_* bits   -- gs_mp_misc.h
//   gsMP_SERVER *gsSysServer (0x0098E870): DeActivateClientAll 0x0050FA30,
//       SendMsgClientAll 0x00510580 (wr.py mislabels it m3dMATR::RemoveScale),
//       SyncClient 0x0050F710 (?), vt +0x2C SendMsgClient(connId, ent, msg, data, flags)
//   gsMP_NETWORK *gsSysNetwork (0x0099D814): Drv_Freeze 0x0052F260,
//       Drv_NotifyGameStarted 0x0052F250, Drv_IsGameStarted 0x0052F240, Drv_Stop 0x0052F120
//   gsLVL_SYSTEM *gsSysLevel (0x0098E84C), uiUI_SYSTEM *uiSystem (0x00994568)
//   gsMsgChangeMode 0x00510D50, gslLoad 0x0052FF70, entCreate 0x005082B0,
//   entFindNext 0x005086A0, fnmGetName 0x004FF040, apNAME::SetName 0x004AF580,
//   apMsg 0x004AF430, _apCfgReadBool 0x004AF940, _apCfgReadString 0x004AFA50,
//   scnSCENE::GetNInst 0x004E6EC0 / GetNTpl 0x004E6EF0, partGetNEmit 0x004D42D0,
//   gsSND_SYSTEM::GetNChn 0x005122A0, pteManager (0x00974E10)
//   m3dV, m3dVUndef (v12 0x009AEA90)
// ---------------------------------------------------------------------------

#pragma once
#include "wr_sdk.h"
#include "mp_messages.h"   // [integrator] dependency
#include "gs_mp_misc.h"   // [integrator] dependency
   // engine core types + subsystem classes (re/sdk); was "assumed from other files"
#include <time.h>

// ---- message ids (v12 numbering) ------------------------------------------
#define GS_MSG_START_GAME        0x3EA   // data = gspSTART_GAME_DATA*
#define GS_MSG_STOP_GAME         0x3EB
#define GS_MSG_LOAD_LEVEL        0x3EC   // data = gsMSG_LOAD_LEVEL* (to gsSysLevel)
#define GS_MSG_INIT_GAME         0x3EF   // SendAll, "GS_MSG_INIT_GAME"
#define GS_MSG_TERM_GAME         0x3F0   // SendAll, "GS_MSG_TERM_GAME"
#define GS_MSG_INIT_MAP          0x3F1   // SendAll, "GS_MSG_INIT_MAP"
#define GS_MSG_PRE_TERM_GAME     0x3F3   // SendAll before TERM_GAME (name ?)
#define GS_MSG_LEVEL_LOADED_2    0x3F7   // [integrator] renamed from GS_MSG_LEVEL_LOADED: the SDK uses that name for 0x3F6 (also sent by __gsLoadLevel, v12 0x00536AE2)
//      GS_MSG_LEVEL_LOADED      0x3F7   // SendAll from __gsLoadLevel v12 0x00536B3D (name ?)
#define GS_MSG_MP_NEW_MAP        0x411   // 1.2: was 0x40F. server->clients: change map
#define GS_MSG_MP_LEVEL_CHANGE   0x412   // 1.2: was 0x410. server->clients before 0x3EC (name ?)
#define GS_MSG_MP_PLACE_PLAYERS  0x413   // 1.2: was 0x411. self-posted: (re)place all players
#define GS_MSG_MP_SET_START      0x40E   // 1.2: was 0x40C. to a player entity: start pos/dir

#define GSP_MAX_EVENTS  200              // event history size

// ---- gspSTART_GAME_DATA : msgDATA  (size 0x10C) ----------------------------
#if 0 // [integrator] moved to sdk/game_systems/level.h (same layout)
struct gspSTART_GAME_DATA : msgDATA      // packId 0, flags 0x40, size 0x10C
{
    char level[260];                     // +0x004 level file ([Level] File)
    int  mode;                           // +0x108 gsMP_MODE (-1 = not set)
};
#endif


// ---- level-load message sent with 0x3EC (size 0x154). struct name ? -------
struct gsMSG_LOAD_LEVEL : msgDATA        // packId 0, flags 0x40, size 0x154 [integrator] merged with gs_mp_server.h's copy
{
    char level[260];                     // +0x004 gsSceneFullName
    union { char unk108[64]; struct { char b108; char pad[0x3F]; }; }; // +0x108 (first byte zeroed)  ?
    union { int unk148; int d148; };     // +0x148 zeroed before send   ?
    union { int keepSession; int d14C; };// +0x14C 1 = map change inside a running MP session
    union { int unk150; int d150; };     // +0x150                      ?
};

// ---- MP "set start position" event (sent with 0x40E) ------------------------
#pragma pack(push, 1)
struct gsEV_SET_START_POS : msgDATA      // size 0x1E   1.2: (feb 0x20, int event id)
{
    short ev;                            // +0x04  -97 (0xFF9F)   1.2: feb int -95 (0xFFFFFFA1)
    m3dV  pos;                           // +0x06  1.2: (feb +0x08)
    m3dV  dir;                           // +0x12  1.2: (feb +0x14)
};
#pragma pack(pop)

// ---- event log entry (size 0x68) --------------------------------------------
enum gspEVENT_TYPE
{
    GSP_EV_START_GAME   = 0,
    GSP_EV_STOP_GAME    = 1,
    GSP_EV_LOAD_GAME    = 2,
    GSP_EV_SAVE_GAME    = 3,
    GSP_EV_CRASH_GAME   = 4,
    GSP_EV_PLAYER_JOIN  = 5,             // 1.2: new
    GSP_EV_PLAYER_LEAVE = 6,             // 1.2: new
};

struct gspEVENT
{
    int    id;                           // +0x00 running counter (gspPLAY_SYSTEM::evCounter)
    int    type;                         // +0x04 gspEVENT_TYPE
    apNAME name;                         // +0x08 scene file name / player name (0x40 bytes)
    time_t time;                         // +0x48
    int    mpMode;                       // +0x4C gsSysMP->mode at the time
    int    style;                        // +0x50 caller argument ("Style")
    int    nSdr;                         // +0x54 pteManager +0x1F44
    int    nEmit;                        // +0x58 partGetNEmit()
    int    nInst;                        // +0x5C scene GetNInst()
    int    nTpl;                         // +0x60 scene GetNTpl()
    int    nSndChn;                      // +0x64 gsSysSound->GetNChn()
    char  *GetString();                  // v12 0x00534130
};
extern gspEVENT _gspEventList[GSP_MAX_EVENTS];   // v12 0x009A01E0 (feb 0x00957650)

// ---- gspPLAY_SYSTEM : msgADDR (size 0x30) -----------------------------------
// vtable v12 0x006083C8.  global gspSysPlay v12 0x009A01DC.
class gspPLAY_SYSTEM : public msgADDR
{
public:
    // +0x00..+0x24 msgADDR (+0x14 name '$PSG', +0x1C priority 0x15)
    int field24;                         // +0x24 [integrator] msgADDR is 0x24 bytes; zeroed by gspInit
    int nEvents;                         // +0x28 entries used in _gspEventList (<= 200)
    int evCounter;                       // +0x2C total events ever logged

    virtual void *__vecDelDtor(unsigned flags);   // slot +0x00 v12 0x00534C90 (shared/folded)
    virtual int   ProcessMsg(int msg, void *data, msgADDR *from, int *res);  // slot +0x04 v12 0x00534900
    //            PostProcessMsg x2 inherited                                // slot +0x08,+0x0C

    void      NotifyEvent(int type, char *name, int style);   // v12 0x00533F90
    gspEVENT *GetEvent(int idx);                              // v12 0x00534100
    int       StartFirst();                                   // v12 0x00534230
    int       StartGame(gspSTART_GAME_DATA *d);               // v12 0x00534360
    int       LoadGame(gspSTART_GAME_DATA *d);                // v12 0x005345C0
    void      StopGame();                                     // v12 0x005346A0
    int       StartNewGameMap(gspSTART_GAME_DATA *d);         // v12 0x005347B0
};

extern gspPLAY_SYSTEM *gspSysPlay;       // v12 0x009A01DC

int  __fastcall gspInit();               // v12 0x00533EF0
void __fastcall gspTerm();               // v12 0x00533F70
