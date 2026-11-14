// wr_go_mp.h -- Will Rock 1.2 (v12.exe): the multiplayer part of wrGO_SYSTEM ("game
// options" system, wrSysGO): game rules read from [Multiplayer], frag/time/treasure
// limits, end of round, and the next-map (rotation) request.
// Reconstructed documentation source; NOT meant to compile into the game.
//
// Source object: wr_game_opt.obj (feb 0x00498790..0x00499C3F; v12 0x004A5C?? .. 0x004A7?? --
// the automatic match tables have no twins for this object, the v12 addresses below
// were found by hand from the call sites in ProcessMsg v12 0x004A5FC0).
//
// Types assumed from other files
//   entIACTIVE_OBJ (base of wrGO_SYSTEM; 1.2 entity base +4 bytes, so most fields are feb + 4)
//       +0x88 mpid (feb +0x8C)
//   gsMP_SYSTEM *gsSysMP (0x0098E850): +0x28 mode (1 Single, 2 Server, 3 Client, 4 ServerOnly),
//       IsRegularMPID 0x0050E690, GetNStartPos 0x0050E570, GetNextPlayer 0x0050E750
//   gsMP_SERVER *gsSysServer (0x0098E870): DeclSyncable 0x0050F5A0, GetNextPlayer 0x0050F440,
//       SendMsgClientAll(src, msg, data, flags, skip1, skip2) 0x00510580
//       (wr.py labels 0x00510580 "m3dMATR::RemoveScale"; that label is wrong)
//   gsMP_NETWORK ladder facade: Server_ResetLadder 0x0052FAC0, Server_StopLadder 0x0052FAE0,
//       Server_GetLadderStatus 0x0052FB00  (Ubi.com ladder; not reconstructed further)
//   msgSYSTEM msgSystem (0x008FA960): SendAll 0x004D06F0, PostMsg 0x004D0410,
//       PostTimeMsgData(dst, msg, data, src, float delay) 0x004D0570
//   gspPLAY_SYSTEM *gspSysPlay (0x009A01DC): StopGame 0x005346A0, NotifyEvent 0x00533F90
//   gsLVL_SYSTEM *gsSysLevel (0x0098E84C): +0x3C3C current gspSTART_GAME_DATA
//   gsINP_SYSTEM *gsSysInput (0x0098E838): IsToggled 0x00425D20
//   wrUI_SYSTEM *uiSystem (0x00994568) slots used here (1.2; feb = slot - 4 from +0x24 on):
//       +0x24 ShowCenterText(const ushort*, float time, int prio)   (feb +0x20)   // ? name
//       +0x3C ShowMainMenu(int, int) ?                                (feb +0x38)
//       +0x88 Slot88(int)            (6, 5 by the announcer; 1 by the join list)  // ? unidentified
//       +0x98 SetLoadingLevel(const char* level) ?   1.2: new call     // ? name
//       +0xA0 GetNextLocation(char* buf, int size) -> wrUI_ADVSERV_WINDOW::GetNextLocation  (feb +0x9C)
//   gsSTRINGS::Printf 0x0051C760 (cdecl, returns a ushort*), gsSTRINGS::GetStringById 0x0051D140 (buffer overload)
//   gsAppState 0x0098E824 bits: 0x4 game running, 0x4000 ?, 0x20000 ?, 0x80000 round over, 0x08000000 -dedicated
//   gsElapsedTime 0x0098E9F4, gsElapsedTimeLevel 0x0098EA1C (seconds since level start)
//   plrPLAYER fields read here (v12): +0x20F team (feb +0x2B3), +0x213 frags (feb +0x2B7),
//       +0x21B treasures (feb +0x2BF), +0xC0 animINST* -> +0x18 name

#pragma once
#include "wr_sdk.h"   // engine core types + subsystem classes (re/sdk); was "assumed from other files"

#include "gs_mp_server.h"

// game type, same numbering as the server menu (wr_ui_server.h wrMP_MODE)
enum wrGO_GAMETYPE { WRGO_SINGLE = 0, WRGO_DM = 1, WRGO_COOP = 2, WRGO_TH = 3 };

// GS_MSG_MP_NOTIFY (0x40E in 1.2, 0x40C in feb) payloads sent by wrSysGO
#pragma pack(push, 1)
struct wrGO_MSG_GAMEOVER : msgDATA            // sub 0, 0x52 bytes (feb 0x0C: {hdr, int sub, int over})   [integrator: was "{ msgHDR hdr; ...", hdr is a msgDATA alias]
{
    short  sub;                     // +0x04  0
    int    gameOver;                // +0x06  1 = round over
    apNAME winner;                  // +0x0A  0x48 bytes, copy of wrGO_SYSTEM::winnerName  1.2: new
    int    _4E;                     // +0x4E  0   // ? (winner team is not copied here)
};
struct wrGO_MSG_TIMESYNC : msgDATA            // sub 1, 0x0A bytes   1.2: new   [integrator: was "{ msgHDR hdr; ...", hdr is a msgDATA alias]
{
    short  sub;                     // +0x04  1
    float  levelTime;               // +0x06  gsElapsedTimeLevel of the server
};
#pragma pack(pop)

// "N minutes/frags/treasures left" announcer state.  1.2: new (wrGO_SYSTEM +0x110)
struct wrGO_ANNOUNCER
{
    int nextTime;                   // +0x00  seconds-left value to announce next
    int nextTreasures;              // +0x04  clamped to 6
    int nextFrags;                  // +0x08  clamped to 4
    void Update();                  // v12 0x004A5CE0  (no feb twin)
};

#if 0 // [integrator] merged into sdk/frontend/wr_game.h (its +0xC0..+0x110 names were taken from here, verified in v12)
class wrGO_SYSTEM : public entIACTIVE_OBJ
{
public:
    virtual int ProcessMsg(int msg, void* data, msgADDR* from, int* res);   // slot +0x04  v12 0x004A5FC0  (feb 0x00498870)
    void ProcessFRAME();                // v12 0x004A6440  (feb 0x00498B50)
    void ProcessINIT_LEVEL();           // v12 0x004A6B80  (feb 0x00499060)
    void ProcessINIT_GAME();            // v12 0x004A6CE0  (feb 0x00499180)

    // fields (v12; feb = v12 - 4 for the ones that existed)
    int     gameType;           // +0x98  (feb +0x94)  wrGO_GAMETYPE
    float   godAfterDeath;      // +0x9C  (feb +0x98)  ServerPlrGODAfterDeath, -1 = off
    int     friendlyFire;       // +0xA0  (feb +0x9C)
    int     unlimitedAmmo;      // +0xA4  (feb +0xA0)
    int     itemMode;           // +0xA8  (feb +0xA4)  0 single/COOP-delete, 1 DM/TH, 2 COOP "keep"
    int     difficulty;         // +0xAC  (feb +0xA8)
    float   timeLimit;          // +0xB0  (feb +0xAC)  seconds, ServerTimeLimit (default 120)
    int     fragLimit;          // +0xB4  (feb +0xB0)  ServerFragLimit (default 20), -1 unless DM
    int     autoBalanceTeams;   // +0xB8  1.2: new     ServerAutoBalanceTeam
    int     mapChangePosted;    // +0xBC  1.2: set when the next map was requested; reset in INIT_LEVEL
    apNAME  winnerName;         // +0xC0  0x48 bytes
    int     winnerTeam;         // +0x100 TH: -1 draw, 0, 1
    int     gameOverSent;       // +0x104
    float   timeSyncTimer;      // +0x108 1.2: 5 s period of wrGO_MSG_TIMESYNC
    float   roundOverTime;      // +0x10C 1.2: seconds since the round ended (-1 at level start)
    wrGO_ANNOUNCER announcer;   // +0x110 1.2
};
#endif


extern wrGO_SYSTEM* wrSysGO;
extern const char* wrGO_MODE_NAMES[4];   // v12 0x00605D04 (feb 0x005DA7E4): SINGLE, DM, COOP, TH    // v12 0x0064FF5C  (feb 0x0060EFDC)
