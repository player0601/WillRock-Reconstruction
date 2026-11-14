// gs_mp_network.h - Will Rock 1.2 (v12.exe) multiplayer network facade
//
// Reconstructed documentation source (static analysis of v12.exe against the
// Feb 2003 build feb.exe + WillRock.map).  NOT meant to compile into the game.
// Original object: gs:gs_mp_network.obj
//
// Types assumed from other files (not defined here):
//   msgADDR, msgSYSTEM msgSystem (0x008FA960)      engine:msgaddr.cpp
//   netDRV, netDRV_DPUBI (vtable 0x0060A698)        drv:network_dpubi*.cpp
//   netDRV_SERVER_INFO (0x7C bytes in 1.2)          drv:network.cpp (see mp_browser.md)
//   gsMP_SERVER (gsSysServer 0x0098E870)            gs:gs_mp_server.cpp
//   gsMP_CLIENT (gsSysClient 0x0098E860)            gs:gs_mp_client.cpp
//   arrVector<T> {T* data; int size; int capacity;} engine template
//   apRealloc / apFree                              engine memory
//   gsAppState (0x0098E824), gsElapsedTime (0x0098E9F4)
//
// Every member offset is the v12 offset; "(feb +0xNN)" given where it differs.

#pragma once
#include "wr_sdk.h"
#include "network.h"   // [integrator] netDRV, netDRV_EVENT and the parameter structs   // engine core types + subsystem classes (re/sdk); was "assumed from other files"

class msgADDR;
class netDRV;
struct netDRV_SERVER_INFO;

// ---------------------------------------------------------------------------
// Driver interface structs (passed by pointer to the netDRV vtable)
// ---------------------------------------------------------------------------

// Drv_Init argument (driver slot +0x0C, feb +0x08)
#if 0 // [integrator] merged into network.h (same offsets, both member names)
struct netDRV_INIT
{
    int         dedicated;      // +0x00  1.2: gsAppState & 0x08000000 (-dedicated); feb: this was 'type'
    int         versionId;      // +0x04  1.2: 10020000, checked on DPlay connect and in the browser
    int         type;           // +0x08  0 = LAN/DirectPlay only, 1 = DirectPlay + Ubi.com lobby
                                //        (the well known patch 0x590BCF reads this field)
    const char* gameName;       // +0x0C  "WILLROCK"  (feb "WILLROCK_DEMO"); only set for type 1
    const char* gameVersion;    // +0x10  "WRPC1.0";                         only set for type 1
};
#endif


// Server_Start argument (driver slot +0x30, feb +0x2C).  Packed (unaligned
// members at +0x16/+0x1A), built on the stack in gsMP_NETWORK::Server_Start.
#pragma pack(push, 1)
#if 0 // [integrator] merged into network.h (same offsets, both member names)
struct netDRV_SERVER_CREATE
{
    const struct gsMP_SERVER_INFO* info;  // +0x00
    int         maxPlayers;     // +0x04  info->maxPlayers (+0x44)
    int         reserved0;      // +0x08  0                      // ?
    int         reserved1;      // +0x0C  0                      // ?
    unsigned short port;        // +0x10  info->port (+0x50)
    int         ladder;         // +0x12  info->ladder (+0x54) for type 1, 0 for LAN
    const char* userName;       // +0x16  gsMP_NETWORK::userName
    const char* password;       // +0x1A  gsMP_NETWORK::password for type 1, "" for LAN
};
#endif

#pragma pack(pop)

// Browser_Start argument (driver slot +0x6C, feb +0x64)
#if 0 // [integrator] merged into network.h (same offsets, both member names)
struct netDRV_BROWSER_CREATE
{
    const char* userName;       // +0x00  gsMP_NETWORK::userName
    const char* hosts;          // +0x04  "" (LAN broadcast) or gsMP_NETWORK::password (type 1) // ? field reused
};
#endif


// Client_JoinGame argument (driver slot +0x84, feb +0x7C)
#if 0 // [integrator] merged into network.h (same offsets, both member names)
struct netDRV_CLIENT_JOIN
{
    int         serverId;       // +0x00  browser id, or -1 = join by address
    const char* address;        // +0x04  "host:port" when serverId == -1, else NULL
    const char* userName;       // +0x08
    const char* password;       // +0x0C
};
#endif


// Event delivered by the driver to gsMP_NETWORK::OnNetDrvEvent (netDRV::AddEvent 0x0058FDF0)
#if 0 // [integrator] merged into network.h (same offsets, both member names)
enum netDRV_EVENT_TYPE
{
    NETDRV_EV_DRV_PROGRESS      = 1,  // driver init still progressing  -> extend drvTimeout
    NETDRV_EV_SERVER_PROGRESS   = 2,  // server creation progressing     -> extend serverTimeout
    NETDRV_EV_CLIENT_CONNECTED  = 3,  // server: client joined           -> gsSysServer vt+0x20(client)
    NETDRV_EV_CLIENT_GONE       = 4,  // server: client left             -> gsSysServer vt+0x24(client)
    NETDRV_EV_SERVER_DATA       = 5,  // server: data from client        -> gsMP_SERVER::ReceiveMsg
    NETDRV_EV_BROWSER_PROGRESS  = 6,  // browser progressing             -> extend browserTimeout
    NETDRV_EV_CLIENT_PROGRESS   = 7,  // join progressing                -> extend clientTimeout
    NETDRV_EV_CLIENT_DATA       = 8,  // client: data from server        -> gsSysClient vt+0x30
};
#endif


#if 0 // [integrator] merged into network.h (same offsets, both member names)
struct netDRV_EVENT
{
    int         type;           // +0x00  netDRV_EVENT_TYPE
    unsigned long client;       // +0x04  game client id (events 3,4,5); -1 for 8
    void*       data;           // +0x08
    int         size;           // +0x0C
};
#endif


// ---------------------------------------------------------------------------
// gsMP_SERVER_INFO : game-side server description (UI <-> network), 0x58 bytes in 1.2
// ---------------------------------------------------------------------------
struct gsMP_SERVER_INFO
{
    char        name[0x40];     // +0x00
    int         versionId;      // +0x40  1.2: NEW (10020000); every field below moved +4
    int         maxPlayers;     // +0x44  (feb +0x40)
    int         curPlayers;     // +0x48  (feb +0x44)  browser only
    int         ping;           // +0x4C  (feb +0x48)  browser only; 9999 if id invalid
    int         port;           // +0x50  (feb +0x4C)  Server_Start reads the low 16 bits; browser writes 0 here
    int         ladder;         // +0x54  (feb +0x50)  Ubi ladder mode (0 = none)
};

// ---------------------------------------------------------------------------
// gsMP_STAT : traffic statistics (embedded in gsMP_NETWORK at +0xC4)
// ---------------------------------------------------------------------------
class gsMP_STAT
{
public:
    struct TRAFFIC_REC              // 0x10 bytes
    {
        int     id;             // +0x00  key passed to AddTraffic (message/category id) // ?
        int     totalBytes;     // +0x04  since Clear
        int     windowBytes;    // +0x08  in the current 1 s window
        int     bytesPerSec;    // +0x0C  computed by gsMP_NETWORK::UpdStat
    };

    float   rateA;              // +0x00  drv->byteCounterA(+0x08) / (dt*1024) = KB/s  // ? in/out
    float   rateB;              // +0x04  drv->byteCounterB(+0x0C) / (dt*1024) = KB/s  // ? in/out
    int     drvValue10;         // +0x08  copy of drv+0x10                          // ? meaning
    float   windowTime;         // +0x0C  seconds accumulated in the current window
    union {
        arrVector<TRAFFIC_REC> traffic; // +0x10 {data,size,capacity}
        struct { int f10, f14, f18; };  // [integrator] names used by the inline ctor copy in gs_mp_misc.cpp
    };

    gsMP_STAT();                                    // v12 0x0050F120 (feb 0x004FC400, gs_mp_misc.obj)
    void Clear();                                   // v12 0x0052EA70
    void AddTraffic(int id, int bytes);             // v12 0x0052EAD0
};

// ---------------------------------------------------------------------------
// 1.2: per-frame flush helper.  A tiny msgADDR registered in msgSystem that, on
// the frame tick (msg 1000), calls the driver's new Flush slot (+0x08) so the
// packet coalescer in netDP_SERVER/netDP_CLIENT sends one <=700 byte packet per
// target per frame.  Original class name unknown; vtable 0x0060836C, 0x2C bytes.
// Global pointer at 0x0099D81C.
// ---------------------------------------------------------------------------
class gsMP_NET_FLUSHER : public msgADDR      // ? name invented
{
public:
    // msgADDR occupies +0x00..+0x27 (vptr, +0x10 'MADR' class tag, +0x14 address id,
    // +0x1C order/priority; see engine:msgaddr.cpp)
    unsigned int unk24;         // +0x24 [integrator] msgADDR is 0x24 bytes (engine_core/core_types.h); object is 0x2C
    netDRV* drv;                // +0x28  mirror of gsMP_NETWORK::drv, kept in sync

    virtual ~gsMP_NET_FLUSHER();                               // slot +0x00  v12 0x0052EBD0
    virtual int ProcessMsg(int msg, void* data, msgADDR* from, int* res); // slot +0x04  v12 0x0052EC00
    // slots +0x08/+0x0C: msgADDR::PostProcessMsg (inherited)
};
extern gsMP_NET_FLUSHER* gsNetFlusher;                         // 0x0099D81C (1.2)

// ---------------------------------------------------------------------------
// gsMP_NETWORK : facade over the network driver (gsSysNetwork)
// size 0xE0, vtable 0x0060837C (feb 0x005DCEA4)
// ---------------------------------------------------------------------------
class gsMP_NETWORK : public msgADDR
{
public:
    // +0x00..+0x27 msgADDR
    int            addrLock;               // +0x024 msgADDR's lock count (msgSYSTEM skips a locked address; the C++ msgADDR
                                           //   ends at 0x24, each class declares it; zeroed by the ctor)
    int         drvType;        // +0x28  -1 none, 0 LAN (DirectPlay), 1 Ubi.com
    netDRV*     drv;            // +0x2C  netDRV_DPUBI
    char        userName[0x40]; // +0x30
    char        password[0x40]; // +0x70  (also used as the "hosts" string for type-1 browsing)
    float       drvTimeout;     // +0xB0  >0 counting down, <0 = timed out, 0 = idle
    float       serverTimeout;  // +0xB4
    float       browserTimeout; // +0xB8
    float       clientTimeout;  // +0xBC
    float       ladderTimeout;  // +0xC0
    gsMP_STAT   stat;           // +0xC4..+0xDF

    gsMP_NETWORK();                                         // v12 0x0052EC20
    virtual ~gsMP_NETWORK();                                // slot +0x00 v12 0x0052EDA0 (scalar deleting)
    virtual int ProcessMsg(int msg, void* data, msgADDR* from, int* res); // slot +0x04 v12 0x0052F970
    // slots +0x08/+0x0C: msgADDR::PostProcessMsg

    void        UpdStat(float dt);                          // v12 0x0052EE50

    // driver
    void        Drv_Create(int type, int versionId);        // v12 0x0052EF10  1.2: +versionId
    int         Drv_IsCreated();                            // v12 0x0052F060
    int         Drv_IsError();                              // v12 0x0052F090
    const char* Drv_GetStatus();                            // v12 0x0052F0D0
    void        Drv_Stop();                                 // v12 0x0052F120
    const char* Drv_GetIdString();                          // v12 0x0052F190
    int         Drv_IsLadderSupported();                    // v12 0x0052F1D0  1.2: new facade
    int         Drv_IsPasswordRequired();                   // v12 0x0052F1E0
    void        Drv_SetUserNameAndPassword(const char* user, const char* pw); // v12 0x0052F200
    int         Drv_IsGameStarted();                        // v12 0x0052F240
    void        Drv_NotifyGameStarted();                    // v12 0x0052F250
    void        Drv_Freeze(int freeze);                     // v12 0x0052F260
    float       Drv_GetRefreshInterval();                   // v12 0x0052F270

    // server
    void        Server_Start(const gsMP_SERVER_INFO* info); // v12 0x0052F290
    int         Server_IsStarted();                         // v12 0x0052F330
    int         Server_IsError();                           // v12 0x0052F360
    const char* Server_GetStatus();                         // v12 0x0052F3A0
    void        Server_Stop();                              // v12 0x0052F3F0
    void        Server_SendDataToClient(int client, const void* data, int size, int flags); // v12 0x0052F420
    void        Server_SetMainData(const void* data, int size); // v12 0x0052F470
    void        Server_GetMainData(void* data, int* size);  // v12 0x0052F480

    // browser
    void        Browser_Start();                            // v12 0x0052F490
    int         Browser_IsStarted();                        // v12 0x0052F4E0
    int         Browser_IsError();                          // v12 0x0052F510
    const char* Browser_GetStatus();                        // v12 0x0052F550
    void        Browser_Stop(int param);     // ? param               // v12 0x0052F5A0
    void        Browser_RefreshServers();                   // v12 0x0052F5D0
    int         Browser_IsServerListUpdated();              // v12 0x0052F600
    int         Browser_IsServerIdValid(int id);            // v12 0x0052F630
    int         Browser_GetNextServer(int id);              // v12 0x0052F650
    void        Browser_GetServerInfo(int id, gsMP_SERVER_INFO* out); // v12 0x0052F660
    void        Browser_GetServerMainData(int id, void* buf, int* size); // v12 0x0052F6D0

    // client
    void        Client_JoinBrowsedGame(int id);             // v12 0x0052F740
    void        Client_JoinAddress(const char* addr);       // v12 0x0052F7A0
    int         Client_IsJoined();                          // v12 0x0052F800
    int         Client_IsError();                           // v12 0x0052F830
    const char* Client_GetStatus();                         // v12 0x0052F870
    void        Client_DisconnectFromGame();                // v12 0x0052F8C0
    int         Client_GetPing();                           // v12 0x0052F8F0  1.2: new
    void        Client_SendDataToServer(const void* data, int size, int flags); // v12 0x0052F910

    // Ubi.com ladder facade (driver does the work; Ubi side not reconstructed)
    void        Server_ResetLadder();                       // v12 0x0052FAC0
    void        Server_StopLadder();                        // v12 0x0052FAE0
    int         Server_GetLadderStatus();                   // v12 0x0052FB00
    void        Server_StartLadderSending();                // v12 0x0052FB60
    void        Server_FinishLadderSending();               // v12 0x0052FB90
    void        Server_SendLadderData(const char* a, int b, int c); // v12 0x0052FBA0

protected:
    static void __fastcall OnNetDrvEvent(const netDRV_EVENT* ev); // v12 0x0052FBB0
};

extern gsMP_NETWORK* gsSysNetwork;                             // 0x0099D814 (feb 0x00956F94)

// gsMP_SERVER_INFO / gsMP_NETWORK send flags (game side -> driver flags)
#define GSMP_SEND_RELIABLE   0x20000   // -> driver flag 1 (DPNSEND_GUARANTEED)
#define GSMP_SEND_TIMEOUT    0x40000   // -> driver flag 2 (500 ms DPlay timeout, unreliable only)
