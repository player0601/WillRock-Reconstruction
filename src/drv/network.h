// network.h -- netDRV: base class of the multiplayer network drivers (Will Rock 1.2, v12.exe)
//
// Reconstructed for documentation (feb obj: drv:network.obj). Not meant to compile into the game.
// Only concrete driver in the game: netDRV_DPUBI (network_dpubi.cpp, vtable v12 0x0060A698),
// which derives from netDRV and overrides most virtuals.
//
// Types assumed from other files:
//   arrVector<T>          engine growable array {T* data; int size; int capacity;}  (12 bytes)
//   apFree / apRealloc    engine allocator  (v12 0x004AF610 / 0x004AF620)
//   netDRV_INIT           built in gsMP_NETWORK::Drv_Create (v12 0x0052EF10), see below
//   netDRV_SERVER_CREATE  built in gsMP_NETWORK::Server_Start, see below
//   netDRV_CLIENT_JOIN    {int serverId; const char* addr; const char* user; const char* pass;} (mp_browser.md)
//   netDRV_DPUBI          derived driver (network_dpubi.cpp)
//   gsMP_NETWORK::OnNetDrvEvent  the one callback the game installs (v12 0x0052FBB0)
//   Win32: HANDLE, CreateMutexA, WaitForSingleObject, ReleaseMutex, CloseHandle

#pragma once
#include <utility>
using std::swap;   // [integrator] network.cpp swaps event records
#include "wr_sdk.h"   // engine core types + subsystem classes (re/sdk); was "assumed from other files"
#ifdef _WIN32
#include <windows.h>
#else
#include "compat/wr_win32_compat.h"   // integrator: Win32/DirectPlay8 declarations for non-Windows syntax checks / ports
#endif

// ---------------------------------------------------------------------------
// Event queue
// ---------------------------------------------------------------------------
// Event types (consumed by gsMP_NETWORK::OnNetDrvEvent, v12 0x0052FBB0, jump table 0x0052FD74):
enum netDRV_EVENT_TYPE                 // [integrator] names from network.cpp and gs_mp_network.cpp merged
{
    netEV_NONE              = 0,  // erased slot (EraseClientEvents) -- skipped by ProcessEvents
    NETDRV_EV_DRV_PROGRESS      = 1,  // driver init still progressing  -> extend drvTimeout
    NETDRV_EV_SERVER_PROGRESS   = 2,  // server creation progressing     -> extend serverTimeout
    netEV_CLIENT_CONNECTED  = 3,  NETDRV_EV_CLIENT_CONNECTED = 3,  // server: client joined -> gsSysServer vt+0x20(client)
    netEV_CLIENT_LEFT       = 4,  NETDRV_EV_CLIENT_GONE      = 4,  // server: client left (the only type ProcessEvents(1) delivers)
    netEV_DATA_TO_SERVER    = 5,  NETDRV_EV_SERVER_DATA      = 5,  // data from clientId to the server -> gsMP_SERVER::ReceiveMsg
    NETDRV_EV_BROWSER_PROGRESS  = 6,  // browser progressing             -> extend browserTimeout
    NETDRV_EV_CLIENT_PROGRESS   = 7,  // join progressing                -> extend clientTimeout
    netEV_DATA_TO_CLIENT    = 8,  NETDRV_EV_CLIENT_DATA      = 8,  // data from the server (clientId = -1) -> gsSysClient vt+0x30
};

struct netDRV_EVENT                       // 0x14 bytes, unchanged in 1.2
{
    int                   type;           // +0x00 netDRV_EVENT_TYPE
    union {
        int               clientId;       // +0x04 -1 = none
        unsigned long     client;         //       (gs_mp_network.cpp name)
    };
    arrVector<unsigned char> data;        // +0x08 copied payload: data.data = bytes, data.size = length (+0x0C)

    netDRV_EVENT() : type(0), clientId(-1) { data.data = 0; data.count = 0; data.capacity = 0; }
    netDRV_EVENT(const netDRV_EVENT& src);           // v12 0x005901B0 (feb 0x004A7A90)
};

typedef void (__fastcall *netDRV_CALLBACK)(const netDRV_EVENT* ev);   // ecx = event

// ---------------------------------------------------------------------------
// One browsed server.  1.2: 0x7C bytes (feb 0x78) -- netVersion inserted at +0x44.
// ---------------------------------------------------------------------------
struct netDRV_SERVER_INFO
{
    int    id;                         // +0x00 (feb +0x00) netDRV::NewServerId(), -1 = unused
    char   name[64];                   // +0x04 (feb +0x04)
    int    netVersion;                 // +0x44 1.2: NEW. e.g. 10020000; NOT set by the ctor
    int    maxPlayers;                 // +0x48 (feb +0x44)  -1 = unknown
    int    curPlayers;                 // +0x4C (feb +0x48)  -1 = unknown
    int    ping;                       // +0x50 (feb +0x4C)  1.2: ctor default 0 (feb 9999)
    arrVector<unsigned char> mainData; // +0x54 (feb +0x50)  game "main data" blob (mode, map ...)
    arrVector<unsigned char> data2;    // +0x60 (feb +0x5C)  ? second blob, freed with the entry
    int    unk6C;                      // +0x6C (feb +0x68)  ? ctor sets 1
    int    bNoPrune;                   // +0x70 (feb +0x6C)  0: entry dropped when not seen in the
                                       //        last enumeration (see netDRV_DPUBI prune, v12 0x005921F0);
                                       //        mp_browser.md reads it as "source 0=DPlay 1=Ubi"
    void*  driverData;                 // +0x74 (feb +0x70)  driver private (netDP_SERVER_DATA* / Ubi key),
                                       //        freed by the driver in OnServerDelete (slot +0xA4)
    int    enumGeneration;             // +0x78 (feb +0x74)  netDRV::enumGeneration when last seen, -1 new

    netDRV_SERVER_INFO();              // v12 0x0058F2E0 (feb 0x004A6C10)
};

// ---------------------------------------------------------------------------
// Parameter blocks
// ---------------------------------------------------------------------------
struct netDRV_INIT                     // Drv_Init argument (driver slot +0x0C, feb +0x08). 1.2 layout (feb: {int type; char* game; char* ver;})
{
    union { int bDedicated; int dedicated; };  // +0x00 1.2: gsAppState & 0x08000000 (-dedicated)
    union { int netVersion; int versionId; };  // +0x04 1.2: 10020000, checked on DPlay connect and in the browser
    int         type;                  // +0x08 driver type (0 = LAN/DPlay, 1 = Ubi.com); the well known patch 0x590BCF reads it
    const char* gameName;              // +0x0C "WILLROCK" (feb "WILLROCK_DEMO") (Ubi.com only)
    const char* gameVersion;           // +0x10 "WRPC1.0"     (Ubi.com only)
};

#pragma pack(push, 1)
struct netDRV_SERVER_CREATE            // Server_Start argument (driver slot +0x30, feb +0x2C), packed, built on the stack
{                                      // [integrator] network.cpp and gs_mp_network.cpp disagree on +0x00/+0x08/+0x0C // ?
    union { const char* name; const struct gsMP_SERVER_INFO* info; };      // +0x00
    int                  maxPlayers;   // +0x04 info->maxPlayers (+0x44)
    union { const unsigned char* dataBegin; int reserved0; };              // +0x08 initial main data
    union { const unsigned char* dataEnd;   int reserved1; };              // +0x0C
    unsigned short       port;         // +0x10 info->port (+0x50)
    int                  ladder;       // +0x12 info->ladder (+0x54) for type 1, 0 for LAN (kept only by the Ubi driver)
    const char*          userName;     // +0x16 gsMP_NETWORK::userName
    const char*          password;     // +0x1A gsMP_NETWORK::password for type 1, "" for LAN
};
#pragma pack(pop)

// Browser_Start argument (driver slot +0x6C, feb +0x64)
struct netDRV_BROWSER_CREATE
{
    const char* userName;              // +0x00 gsMP_NETWORK::userName, copied to DPUBI userName (+0xEA)
    union { const char* hosts; const char* password; }; // +0x04 "" (LAN) or gsMP_NETWORK::password (type 1); DPUBI copies to +0x12A // ?
};

// Client_JoinGame argument (driver slot +0x84, feb +0x7C)
struct netDRV_CLIENT_JOIN
{
    int         serverId;       // +0x00  browser id, or -1 = join by address
    union { const char* address;  const char* addr; };  // +0x04  "host:port" when serverId == -1, else NULL
    union { const char* userName; const char* user; };  // +0x08
    union { const char* password; const char* pass; };  // +0x0C
};

// ---------------------------------------------------------------------------
// netDRV.  1.2 size 0xBA (feb 0xAE).  vtable v12 0x0060A5E0 (feb 0x005DB388), 42 slots (feb 39).
// ---------------------------------------------------------------------------
class netDRV
{
public:
    netDRV();                                                    // v12 0x0058F320
    virtual ~netDRV();                                           // slot +0x00  (??_G v12 0x0058F400, ~ 0x0058F4E0)

    virtual void Think();                                        // slot +0x04  v12 0x0058F5C0
    virtual void Drv_Flush();                                    // slot +0x08  1.2: NEW (base: empty, 0x00401380).
                                                                 //   name ours; DPUBI 0x00590B90 flushes the
                                                                 //   netDP_SERVER / netDP_CLIENT send queues
    virtual void Drv_Init(const netDRV_INIT* init);              // slot +0x0C  v12 0x0058F870 (feb +0x08)
    virtual int  Drv_IsInited();                                 // slot +0x10  shared stub 0x00548680: return 1
    virtual int  Drv_Slot14();                                   // slot +0x14  ? unnamed in feb map; returns 0
    virtual const char* Drv_GetStatus();                         // slot +0x18  v12 0x0058F8A0
    virtual void Drv_NotifyGameStarted();                        // slot +0x1C  v12 0x0058F8C0
    virtual int  Drv_IsGameStarted();                            // slot +0x20  v12 0x0058F8D0
    virtual void Drv_Freeze(int freeze);                         // slot +0x24  v12 0x0058F8E0
    virtual int  Drv_IsLadderSupported();                        // slot +0x28  return 0
    virtual float Drv_GetRefreshInterval();                      // slot +0x2C  v12 0x0058FA20: 0.0f

    virtual void Server_Start(const netDRV_SERVER_CREATE* sc);   // slot +0x30  v12 0x0058F8F0
    virtual int  Server_IsStarted();                             // slot +0x34  return 1
    virtual int  Server_IsError();                               // slot +0x38  return 0
    virtual const char* Server_GetStatus();                      // slot +0x3C  v12 0x0058FA10
    virtual int  Server_GetClientPing(int clientIdx);            // slot +0x40  1.2: NEW, base returns -1
                                                                 //   (name ours; DPUBI 0x00590F80 ->
                                                                 //   IDirectPlay8Server::GetConnectionInfo
                                                                 //   .dwRoundTripLatencyMS)
    virtual void Server_SendToClient(int client, const void* d, int len, int flags); // slot +0x44  empty
    virtual void Server_SetMainData(const void* data, int len);  // slot +0x48  v12 0x0058FA30
    virtual void Server_GetMainData(void* data, int* len);       // slot +0x4C  v12 0x0058FAF0
    virtual void Server_Stop();                                  // slot +0x50  empty
    virtual void Server_StartMatchData();                        // slot +0x54  empty
    virtual void Server_SetMatchData(int, int, int);             // slot +0x58  empty (ret 0xC)
    virtual void Server_FinishMatchData();                       // slot +0x5C  empty
    virtual int  Server_GetMatchStatus();                        // slot +0x60  return 0
    virtual void Server_StopLadder();                            // slot +0x64  empty
    virtual void Server_ResetLadder();                           // slot +0x68  empty

    virtual void Browser_Start(const void* create);              // slot +0x6C  empty
    virtual int  Browser_IsStarted();                            // slot +0x70  return 1
    virtual int  Browser_IsError();                              // slot +0x74  return 0
    virtual const char* Browser_GetStatus();                     // slot +0x78  v12 0x0058FB30
    virtual void Browser_RefreshServers();                       // slot +0x7C  empty
    virtual void Browser_Stop(int);                              // slot +0x80  empty

    virtual void Client_JoinGame(const netDRV_CLIENT_JOIN* j);   // slot +0x84  v12 0x0058FB40
    virtual int  Client_IsJoined();                              // slot +0x88  return 0
    virtual int  Client_IsError();                               // slot +0x8C  return 0
    virtual const char* Client_GetStatus();                      // slot +0x90  v12 0x0058FB50
    virtual int  Client_GetPing();                               // slot +0x94  1.2: NEW, v12 0x0058FB60 returns -1
                                                                 //   (name ours; DPUBI 0x00591970 ->
                                                                 //   IDirectPlay8Client::GetConnectionInfo)
    virtual void Client_SendToServer(const void* d, int len, int flags); // slot +0x98  empty
    virtual void Client_DisconnectFromGame();                    // slot +0x9C  empty
    virtual int  IsServersEqual(const netDRV_SERVER_INFO* a, const netDRV_SERVER_INFO* b); // slot +0xA0  return 1
    virtual void OnServerDelete(netDRV_SERVER_INFO* s);          // slot +0xA4  empty
    // netDRV_DPUBI appends its own slots from +0xA8 (see network_dpubi.h)

    void SetCallback(netDRV_CALLBACK cb);                        // v12 0x0058F5B0

    // browser list access (non-virtual, used by gsMP_NETWORK::Browser_*)
    int  Browser_IsServerListUpdated();                          // v12 0x0058FB70
    int  Browser_GetNextServer(int prevId);                      // v12 0x0058FB90
    const netDRV_SERVER_INFO* Browser_GetServerInfo(int id);     // v12 0x0058FBF0

public:     // [integrator] was protected; gsMP_NETWORK reads the counters
    // name aliases used by gs_mp_network.cpp (no new slots)
    void Flush()       { Drv_Flush(); }        // slot +0x08
    int  Drv_IsError() { return Drv_Slot14(); } // slot +0x14 (gs_mp_network.h: Drv_IsError)
    void ProcessEvents(int onlyDisconnects);                     // v12 0x0058F5D0
    int  FindServerIdx(int id);                                  // v12 0x0058FC40
    int  NewServerId();                                          // v12 0x0058FC70
    void ClearServersList();                                     // v12 0x0058FC80
    arrVector<netDRV_SERVER_INFO>* LockServers();                // v12 0x0058FDC0
    void UnlockServers();                                        // v12 0x0058FDE0
    void AddEvent(int type, int clientId, const void* data, int len); // v12 0x0058FDF0
    void EraseClientEvents(int clientId);                        // v12 0x0058FF10

    // ---- data (v12 offset, feb offset) ----
    netDRV_CALLBACK          callback;          // +0x04 (feb +0x04)
    union {                                     // [integrator] gs_mp_network.cpp (UpdStat v12 0x0052EE50) reads +0x08/+0x0C/+0x10
        arrVector<unsigned char> unk08;         //   as byte counters and resets them every frame; network.cpp saw an arrVector
        struct { int bytesA; int bytesB; int value10; };  // +0x08 / +0x0C / +0x10  byte counters
        struct { int nBytesSent; int nBytesReceived; int _10; }; // network_dpubi_dp.cpp: drv+0x08 sent, drv+0x0C received (confirms the counter view)
    };
    int                      netVersion;        // +0x14 1.2: NEW, from netDRV_INIT.netVersion (10020000);
                                                //        DPUBI compares a joining client's version with it
    char                     serverName[64];    // +0x18 (feb +0x14)
    int                      maxPlayers;        // +0x58 (feb +0x54)
    arrVector<unsigned char> mainData;          // +0x5C (feb +0x58) server "main data" (announced in enum replies)
    arrVector<unsigned char> data2;             // +0x68 (feb +0x64) ? cleared by Server_Start
    int                      bMainDataPublished;// +0x74 (feb +0x70) 0 = main data changed; DPUBI::Think pushes it
                                                //        to the lobby (feb: netUBI_SERVERREG::UpdateGameInfo) and sets 1
    int                      unk78;             // +0x78 (feb +0x74) ? ctor 1, Server_Start 1
    unsigned short           gamePort;          // +0x7C (feb +0x78) default 7777 (0x1E61)
    int                      enumGeneration;    // +0x7E (feb +0x7A) ++ per DPlay enumeration (DPUBI slot +0xE0)
    int                      lastServerId;      // +0x82 (feb +0x7E) NewServerId counter
    int                      bServerListUpdated;// +0x86 (feb +0x82) read-and-clear by Browser_IsServerListUpdated
    int                      bGameStarted;      // +0x8A (feb +0x86)
    int                      bFrozen;           // +0x8E (feb +0x8A) ProcessEvents stops while set
    int                      bAcceptServers;    // +0x92 1.2: NEW, ctor 1. DPUBI clears it around a join
                                                //        (0x0059093F/0x005909B5); OnServerFound ignores replies while 0;
                                                //        EnumEnd prunes only when set
    int                      bDedicated;        // +0x96 1.2: NEW, from netDRV_INIT.bDedicated
    HANDLE                   hEventsMutex;      // +0x9A (feb +0x8E) guards events
    HANDLE                   hServersMutex;     // +0x9E (feb +0x92) guards servers
    arrVector<netDRV_SERVER_INFO> servers;      // +0xA2 (feb +0x96) stride 0x7C
    arrVector<netDRV_EVENT>       events;       // +0xAE (feb +0xA2) stride 0x14
};                                              // sizeof 0xBA (feb 0xAE)

// Globals (drv:network.obj)
extern int        netPckProcessed;  // v12 0x009AF174 (feb 0x00612020) events left unprocessed by the last ProcessEvents
struct netDBGSTAT { int v[5]; };    // ? fields unknown (feb: read by wrUIHUD_DBGMSG::ProcessUpdate)
extern netDBGSTAT netDbgStat;       // v12 0x009AF178 (feb 0x00612024); 1.2: only the ctor's zeroing refers to it (? no readers found)
