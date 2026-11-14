// network_dpubi.h -- netDRV_DPUBI: the DirectPlay 8 + Ubi.com multiplayer driver (Will Rock 1.2, v12.exe)
//
// Reconstructed for documentation (feb objs: drv:network_dpubi.obj, drv:network_dpubi_ubi.obj).
// Not meant to compile into the game. Static analysis of v12.exe against the Feb 2003 build.
// "1.2:" marks a difference from feb; "?" marks something not proven.
//
// This is the only concrete netDRV in the game. gsMP_NETWORK::Drv_Create (v12 0x0052EF10) does
// `new netDRV_DPUBI` (0x296 bytes), calls Drv_Init (slot +0x0C), and installs OnNetDrvEvent.
// Driver type (netDRV_INIT.type): 0 = LAN / direct IP (DirectPlay only), 1 = Ubi.com lobby + DirectPlay.
//
// Types assumed from other files:
//   netDRV, netDRV_EVENT, netDRV_SERVER_INFO, netDRV_INIT, netDRV_SERVER_CREATE,
//   netDRV_CLIENT_JOIN, arrVector<T>                       network.h (drv:network.obj)
//   netDP_SERVER, netDP_CLIENT, netDP_NEW_SERVER,
//   netDP_SERVER_DATA, CharToWChar                         drv:network_dpubi_dp.obj (mp_transport.md)
//   netDP_PINGER (name ours)                               1.2: NEW helper class, v12 0x00595340..0x00595A0F,
//                                                          (between _inpINPUT and network_dpubi_ubi.obj); an
//                                                          IDirectPlay8Client that EnumHosts one Ubi-listed server
//                                                          to measure its ping (see netDRV_DPUBI::UpdateUbiPings)
//   netUBI_SERVERREG, netUBI_CLIENT, netUBI_NEW_SERVER,
//   InitializeSockets / UninitializeSockets                drv:network_dpubi_ubi.obj + Ubi.com SDK
//                                                          /* Ubi.com service: not reconstructed (dead service / licensing) */
//   apFree, apRealloc, apBuffer (char[0x800] scratch)      engine
//   DirectPlay 8: DPN_APPLICATION_DESC, DPNID, DPNSESSION_CLIENT_SERVER, DPNERR_SESSIONFULL (dplay8.h)
//   Win32: HANDLE, CreateMutexA, WaitForSingleObject, ReleaseMutex, CloseHandle

#pragma once
#include "wr_sdk.h"   // engine core types + subsystem classes (re/sdk); was "assumed from other files"
#ifdef _WIN32
#include <windows.h>
#else
#include "compat/wr_win32_compat.h"   // integrator: Win32/DirectPlay8 declarations for non-Windows syntax checks / ports
#endif
#ifdef _WIN32
#include <dplay8.h>
#endif
#include "network.h"
#include "compat/net_ubi_decl.h"   // [integrator] Ubi.com wrapper declarations (out of scope, call shapes only)

class netDP_SERVER;
class netDP_CLIENT;
class netDP_PINGER;
class netUBI_SERVERREG;
class netUBI_CLIENT;
struct netDP_NEW_SERVER;
struct netUBI_NEW_SERVER;
struct netDP_SERVER_DATA;

// ---------------------------------------------------------------------------
// Constants (tweak points for modders; v12 addresses where the constant lives)
// ---------------------------------------------------------------------------
#define NET_VERSION_ID               10020000    // passed by gsMP_NETWORK::Drv_Create, kept in netDRV::netVersion (+0x14)

// DirectPlay application GUID (DPN_APPLICATION_DESC.guidApplication), v12 0x0060A688 (feb 0x005DF0DC).
// 1.2 / 1.0 retail: {C3BD3D5B-CD74-4532-9B63-2CBDC2C51C89}.  feb: ...-9B63-2ABD...  (feb cannot see retail).
extern const GUID netDPUBI_AppGuid;

// Payload header of every driver-level DirectPlay blob (enum query/response, connect data/reply).
// All little-endian.  hdrLen = offset of the variable part (name or main data).
//   type 1      : normal game blob
//   type 0xFFFF : ping probe (1.2: NEW, sent by netDP_PINGER), answered with an empty reply
#define NETDP_BLOB_GAME              1
#define NETDP_BLOB_PING              0xFFFF

// Connect reply codes (u32 in the 8-byte connect reply {u16 1, u16 8, u32 code}).
// The client turns them into its connect HRESULT in Dp_Client_GetConnectResult (slot +0xDC).
#define NETDP_CONNECT_OK             0x8015A000  // also sent (with reject) when the connect data is malformed
#define NETDP_CONNECT_BAD_VERSION    0x8015A001  // client versionId != server netVersion (10020000)
#define NETDP_CONNECT_REFUSED        0x80158560  // DPNERR_SESSIONFULL: name in use / equals host name / table add failed

// Sentinel player name used when a blob carries no valid name (v12 0x0063B0EC).
// The \x01 bytes guarantee it never equals a typed name.
#define NETDP_INVALID_NAME           "\x01invalid player name\x01"

// Mutex wait for the player table (WaitForSingleObject), everywhere in this file.
#define NETDPUBI_MUTEX_WAIT_MS       10000       // 0x2710

// Browser auto-refresh interval returned by Drv_GetRefreshInterval (slot +0x2C), seconds:
//   Ubi.com: 600.0f (v12 0x005FFFD4)   LAN: 15.0f (v12 0x005FA490)

// Ping reported for a Ubi.com server whose probe did not answer.
#define NETDP_PING_UNKNOWN           9999        // 0x270F

// ---------------------------------------------------------------------------
// Action queue.  The driver never blocks: public calls append actions to `actions`,
// Think() runs the current one (`action`) and FinishAction() pops the next.
// Values are the jump-table indices of Think (v12 table 0x00590B24, 18 entries; feb 0x00576C6C).
// The enum names are ours (enums are not in the feb map).
// ---------------------------------------------------------------------------
enum netDPUBI_ACTION
{
    netACT_NONE              = 0,  // group separator; Server_Stop/Browser_Stop/Client_Disconnect drop the
                                   // queued actions up to the next 0
    netACT_SRV_DP_START      = 1,  // new netDP_SERVER, StartServer
    netACT_SRV_DP_WAIT       = 2,  // wait netDP_SERVER started / error
    netACT_SRV_UBI_REGISTER  = 3,  // new netUBI_SERVERREG                     (Ubi only)
    netACT_SRV_UBI_WAIT      = 4,  // wait SERVERREG connected / error          (Ubi only)
    netACT_SRV_READY         = 5,  // bServerStarted = 1
    netACT_BRW_CLEAR         = 6,  // ClearServersList
    netACT_BRW_DP_START      = 7,  // new netDP_CLIENT, StartBrowser(joinAddr[], 2)
    netACT_BRW_DP_WAIT       = 8,  // wait browser started / error
    netACT_BRW_UBI_CONNECT   = 9,  // new netUBI_CLIENT                          (Ubi only)
    netACT_BRW_UBI_WAIT      = 10, // wait Ubi client connected; 1.2: then creates the netDP_PINGER
    netACT_JOIN_DP_FIND      = 11, // EnumHosts the join address until a DPlay server shows up
    netACT_BRW_READY         = 12, // bBrowserStarted = 1, first RefreshServers
    netACT_JOIN_UBI_JOIN     = 13, // netUBI_CLIENT::JoinGame(server)            (Ubi only)
    netACT_JOIN_UBI_WAIT     = 14, // wait lobby join; Ubi_Client_OnJoinServerAddress fills joinAddr[]
    netACT_JOIN_DP_CONNECT   = 15, // netDP_CLIENT::JoinGame(server)
    netACT_JOIN_DP_WAIT      = 16, // wait DPlay connected / error
    netACT_JOIN_READY        = 17, // bClientJoined = 1
};

// Server-side player table entry (arrVector stride 0x2C, unchanged from feb).
struct netDRV_DPUBI_PLAYER
{
    enum
    {
        PF_DP_PENDING   = 1,   // DirectPlay connect accepted (Dp_Server_OnConnectionAttempt), not yet created
        PF_DP_CONNECTED = 2,   // DirectPlay player created, dpnid valid
        PF_UBI          = 4,   // registered through the Ubi.com lobby (Ubi_Server_OnPlayerJoined)
    };
    unsigned int flags;        // +0x00  0 = free slot
    char         name[36];     // +0x04  player name from the connect data / lobby (strncpy 0x24, [0x23]=0)
    DPNID        dpnid;        // +0x28  valid while PF_DP_CONNECTED
};                              // 0x2C. The game-side client id is the INDEX in this table.

// Browser_Start parameter (feb name netDRV_BROWSER_CREATE).
#if 0 // [integrator] merged into network.h
struct netDRV_BROWSER_CREATE
{
    const char* userName;      // +0x00 copied to userName (+0xEA)
    const char* password;      // +0x04 copied to password (+0x12A); mp_browser.md calls it the host list ?
};
#endif


// Extra fields of netDRV_SERVER_CREATE read by DPUBI::Server_Start (packed after the u16 port at +0x10).
//   +0x12 int          bLadder   (only kept when the driver is Ubi)
//   +0x16 const char*  userName
//   +0x1A const char*  password

// ---------------------------------------------------------------------------
// netDRV_DPUBI.  1.2 size 0x296 (feb 0x27A).  vtable v12 0x0060A698 (feb 0x005DF0F0).
// 1.2: 75 slots (+0x000..+0x128); feb had 65 (+0x000..+0x100).
// ---------------------------------------------------------------------------
class netDRV_DPUBI : public netDRV
{
public:
    netDRV_DPUBI();                                                                // v12 0x005902C0 (feb 0x005769D0)
    virtual ~netDRV_DPUBI();                                                       // slot +0x000  ??_G 0x005903C0, ~ 0x005903E0

    // ---- netDRV overrides (slot order = netDRV, see network.h) ----
    virtual void Think();                                                          // slot +0x004  0x00590590
    virtual void Drv_Flush();                                                      // slot +0x008  0x00590B90  1.2: NEW
    virtual void Drv_Init(const netDRV_INIT* init);                                // slot +0x00C  0x00590BC0
    virtual int  Drv_IsInited();                                                   // slot +0x010  0x00590C60
    //           Drv_Slot14                                                        // slot +0x014  0x00502570 shared "return 0" (base)
    //           Drv_GetStatus                                                     // slot +0x018  0x0058F8A0 (base)
    virtual void Drv_NotifyGameStarted();                                          // slot +0x01C  0x00590C70
    //           Drv_IsGameStarted / Drv_Freeze                                    // slot +0x020 / +0x024 (base)
    virtual int  Drv_IsLadderSupported();                                          // slot +0x028  0x00590CA0
    virtual float Drv_GetRefreshInterval();                                        // slot +0x02C  0x00590B70

    virtual void Server_Start(const netDRV_SERVER_CREATE* sc);                     // slot +0x030  0x00590CB0
    virtual int  Server_IsStarted();                                               // slot +0x034  0x00590EB0
    virtual int  Server_IsError();                                                 // slot +0x038  0x00590EC0
    virtual const char* Server_GetStatus();                                        // slot +0x03C  0x00590F00
    virtual int  Server_GetClientPing(int clientIdx);                              // slot +0x040  0x00590F80  1.2: NEW (name ours)
    virtual void Server_SendToClient(int client, const void* d, int len, int flags); // slot +0x044 0x00591000
    //           Server_SetMainData / Server_GetMainData                           // slot +0x048 / +0x04C (base)
    virtual void Server_Stop();                                                    // slot +0x050  0x00591070
    virtual void Server_StartMatchData();                                          // slot +0x054  0x00591190
    virtual void Server_SetMatchData(const char* key, int a, int b);               // slot +0x058  0x005911A0
    virtual void Server_FinishMatchData();                                         // slot +0x05C  0x005911C0
    virtual int  Server_GetMatchStatus();                                          // slot +0x060  0x005911D0
    virtual void Server_StopLadder();                                              // slot +0x064  0x00591160
    virtual void Server_ResetLadder();                                             // slot +0x068  0x00591170

    virtual void Browser_Start(const netDRV_BROWSER_CREATE* bc);                   // slot +0x06C  0x005911F0
    virtual int  Browser_IsStarted();                                              // slot +0x070  0x005913E0
    virtual int  Browser_IsError();                                                // slot +0x074  0x005913F0
    virtual const char* Browser_GetStatus();                                       // slot +0x078  0x00591430
    virtual void Browser_RefreshServers();                                         // slot +0x07C  0x005914B0
    virtual void Browser_Stop(int bKeepList);                                      // slot +0x080  0x005914D0

    virtual void Client_JoinGame(const netDRV_CLIENT_JOIN* j);                     // slot +0x084  0x005915F0
    virtual int  Client_IsJoined();                                                // slot +0x088  0x005918B0
    virtual int  Client_IsError();                                                 // slot +0x08C  0x005918C0
    virtual const char* Client_GetStatus();                                        // slot +0x090  0x00591900
    virtual int  Client_GetPing();                                                 // slot +0x094  0x00591970  1.2: NEW (name ours)
    virtual void Client_SendToServer(const void* d, int len, int flags);           // slot +0x098  0x00591990
    virtual void Client_DisconnectFromGame();                                      // slot +0x09C  0x005919A0
protected:
    virtual int  IsServersEqual(const netDRV_SERVER_INFO* a, const netDRV_SERVER_INFO* b); // slot +0x0A0  0x00591A60
    virtual void OnServerDelete(netDRV_SERVER_INFO* s);                            // slot +0x0A4  0x00591AC0
public:
    // ---- netDRV_DPUBI's own slots: DirectPlay side (called by netDP_SERVER / netDP_CLIENT / netDP_PINGER) ----
    virtual int  Drv_GetNetVersion();                                              // slot +0x0A8  0x00548C90 (shared stub
                                                                                   //   `mov eax,[ecx+0x14]`)  1.2: NEW (name ours)
    virtual void Dp_Client_BuildPingQuery(arrVector<unsigned char>* out);          // slot +0x0AC  0x00592320  1.2: NEW (name ours)
    virtual unsigned short Dp_Server_GetGamePort();                                // slot +0x0B0  0x005923D0 (feb +0x09C)
    virtual void Dp_Server_SetGamePort(unsigned short port);                       // slot +0x0B4  0x005923E0  1.2: NEW (name ours)
    virtual int  Dp_Server_OnQuery(const void* q, int qLen, arrVector<unsigned char>* reply); // slot +0x0B8 0x00592470 (feb +0x0A0)
    virtual void Dp_Server_FillAppDesc(DPN_APPLICATION_DESC* desc);                // slot +0x0BC  0x00592400 (feb +0x0A4)
    virtual int  Dp_Server_OnConnectionAttempt(const void* d, int len, int* pPlayerCtx,
                                               arrVector<unsigned char>* reply);    // slot +0x0C0  0x005926F0 (feb +0x0A8)
    virtual void Dp_Server_OnPlayerConnected(DPNID dpnid, int playerCtx);          // slot +0x0C4  0x005928B0 (feb +0x0AC)
    virtual void Dp_Server_OnPlayerDisconnected(DPNID dpnid);                      // slot +0x0C8  0x005928F0 (feb +0x0B0)
    virtual void Dp_Server_OnDataReceived(DPNID dpnid, void* data, int len);       // slot +0x0CC  0x00592980 (feb +0x0B4)
    virtual void Dp_Client_FillAppDesc(DPN_APPLICATION_DESC* desc);                // slot +0x0D0  0x00592A00 (feb +0x0B8)
    virtual void Dp_Client_BuildEnumQuery(arrVector<unsigned char>* out);          // slot +0x0D4  0x00592A50  1.2: NEW (name ours)
    virtual void Dp_Client_BuildConnectData(arrVector<unsigned char>* out);        // slot +0x0D8  0x00592BB0  1.2: NEW (name ours)
    virtual HRESULT Dp_Client_GetConnectResult(HRESULT hr, const void* reply, int replyLen); // slot +0x0DC 0x00592D10 1.2: NEW (name ours)
    virtual void Dp_Client_OnServerEnumBegin();                                    // slot +0x0E0  0x00592D40 (feb +0x0BC)
    virtual void Dp_Client_OnServerFound(netDP_NEW_SERVER* ns,
                                         const void* resp, int respLen);            // slot +0x0E4  0x00592D50 (feb +0x0C0, 1 arg)
    virtual void Dp_Client_OnServerEnumEnd();                                      // slot +0x0E8  0x00593000 (feb +0x0C4)
    virtual void Dp_Client_OnDataReceived(void* data, int len);                    // slot +0x0EC  0x00593020 (feb +0x0C8)
    // [integrator] names network_dpubi_dp.cpp uses for the same slots (no new slots)
    unsigned short Dp_GetGamePort()                     { return Dp_Server_GetGamePort(); }          // +0xB0
    void Dp_Server_SetBoundPort(unsigned short p)       { Dp_Server_SetGamePort(p); }                // +0xB4
    template <class V> int Dp_Server_OnEnumQuery(const void* q, int n, V* r) { return Dp_Server_OnQuery(q, n, (arrVector<unsigned char>*)r); } // +0xB8
    template <class V> int Dp_Server_OnConnectAttempt(const void* d, int n, V* reply, void** ctx)  // +0xC0
                                                        { return Dp_Server_OnConnectionAttempt(d, n, (int*)ctx, (arrVector<unsigned char>*)reply); } // ? the two reconstructions order the last two args differently
    void Dp_Server_OnPlayerCreated(DPNID id, void* ctx) { Dp_Server_OnPlayerConnected(id, (int)(size_t)ctx); } // +0xC4
    void Dp_Server_OnPlayerDestroyed(DPNID id)          { Dp_Server_OnPlayerDisconnected(id); }      // +0xC8
    template <class V> void Dp_Client_GetEnumData(V* o)    { Dp_Client_BuildEnumQuery((arrVector<unsigned char>*)o); }       // +0xD4
    template <class V> void Dp_Client_GetConnectData(V* o) { Dp_Client_BuildConnectData((arrVector<unsigned char>*)o); }     // +0xD8
    HRESULT Dp_Client_OnConnectResult(HRESULT hr, const void* r, int n) { return Dp_Client_GetConnectResult(hr, r, n); } // +0xDC
    void Dp_Client_OnEnumStart()                        { Dp_Client_OnServerEnumBegin(); }           // +0xE0
    void Dp_Client_OnEnumEnd()                          { Dp_Client_OnServerEnumEnd(); }             // +0xE8

    // ---- Ubi.com side (called by netUBI_SERVERREG / netUBI_CLIENT) ----
    virtual const char* Ubi_GetUserName();                                         // slot +0x0F0  0x00593060 (feb +0x0CC)
    virtual const char* Ubi_GetPassword();                                         // slot +0x0F4  0x00593070 (feb +0x0D0)
    virtual const char* Ubi_GetGameName();                                         // slot +0x0F8  0x00593040 (feb +0x0D4)
    virtual const char* Ubi_GetVersion();                                          // slot +0x0FC  0x00593050 (feb +0x0D8)
    virtual const char* Ubi_Server_GetHostName();                                  // slot +0x100  0x00593080 (feb +0x0DC)
    virtual int  Ubi_Server_GetMaxPlayers();                                       // slot +0x104  0x00593090 (feb +0x0E0)
    virtual unsigned short Ubi_Server_GetHostPort();                               // slot +0x108  0x005923D0 = Dp_Server_GetGamePort
    virtual int  Ubi_Server_GetCurPlayers();                                       // slot +0x10C  0x005930A0  1.2: NEW (name ours)
    virtual void Ubi_Server_GetGameInfo(void** data, int* len);                    // slot +0x110  0x005930C0 (feb +0x0E8)
    virtual void Ubi_Server_OnPlayerJoined(const char* name);                      // slot +0x114  0x005930E0 (feb +0x0EC)
    virtual void Ubi_Server_OnPlayerDisconnected(const char* name);                // slot +0x118  0x005930F0 (feb +0x0F0)
    virtual void Ubi_Client_OnServerEnumBegin();                                   // slot +0x11C  0x00592D40 = Dp_Client_OnServerEnumBegin
    virtual void Ubi_Client_OnServerFound(netUBI_NEW_SERVER* ns);                  // slot +0x120  0x005931B0 (feb +0x0F8)
    virtual void Ubi_Client_OnServerEnumEnd();                                     // slot +0x124  0x00593430 (feb +0x0FC)
    virtual void Ubi_Client_OnJoinServerAddress(const char* ip, unsigned short port,
                                                const char* altIp, unsigned short altPort); // slot +0x128 0x00593450 (feb +0x100)

protected:
    // ---- non-virtual helpers ----
    void FinishAction();                                       // v12 0x00591B10 (feb 0x00577FC0)
    int  FindDPlayPlayerPre(const char* name);                 // v12 0x00591C00 (feb 0x005780A0)
    int  AddDPlayPlayer_Pre(const char* name);                 // v12 0x00591D10 (feb 0x005781A0)
    int  AddDPlayPlayer_Fin(DPNID dpnid, int idx);             // v12 0x00591F30 (feb 0x005783C0)
    int  AddUbiPlayer(const char* name);                       // v12 0x00591FD0 (feb 0x00578460)
    void RemovePlayer(int idx);                                // v12 0x00592080 (feb 0x00578510)
    void PruneServers(int source);                             // v12 0x005921F0  1.2: NEW (name ours); replaces the
                                                               //   bodies of feb Dp_/Ubi_Client_OnServerEnumEnd
    void UpdateUbiPings();                                     // v12 0x005934C0  1.2: NEW (name ours)

    // ---- data: v12 offset (feb offset) ----
    netDP_SERVER*      server;            // +0x0BA (feb +0x0AE)  host side DirectPlay8Server wrapper (new 0x38 bytes; feb 0x14)
    netDP_CLIENT*      client;            // +0x0BE (feb +0x0B2)  DirectPlay8Client wrapper (new 0x48 bytes; feb 0x3C)
    netDP_PINGER*      pinger;            // +0x0C2  1.2: NEW. Ubi browser only (new 0x7C bytes)
    netUBI_SERVERREG*  serverReg;         // +0x0C6 (feb +0x0B6)  Ubi.com lobby registration (0x16C bytes)
    netUBI_CLIENT*     ubiClient;         // +0x0CA (feb +0x0BA)  Ubi.com lobby browser/join (0x138 bytes)
    arrVector<int>     actions;           // +0x0CE (feb +0x0BE)  queued netDPUBI_ACTION (count +0x0D2, cap +0x0D6)
    int                action;            // +0x0DA (feb +0x0CA)  current action (Think switch)
    int                bDpEnumEnded;      // +0x0DE  1.2: NEW. DPlay enum ended while bAcceptServers == 0; Think prunes
    int                bUbiEnumEnded;     // +0x0E2  1.2: NEW. same for the Ubi.com list
    int                bUbi;              // +0x0E6 (feb +0x0CE)  netDRV_INIT.type (0 LAN/IP, 1 Ubi.com)
    char               userName[64];      // +0x0EA (feb +0x0D2)  local player name (sent in enum query / connect data)
    char               password[64];      // +0x12A (feb +0x112)  Ubi.com password
    char               gameName[64];      // +0x16A (feb +0x152)  "WILLROCK..." (Ubi only)
    char               gameVersion[64];   // +0x1AA (feb +0x192)  "WRPC1.0"     (Ubi only)
    int                joinServerId;      // +0x1EA (feb +0x1D2)  netDRV_SERVER_INFO.id to join, -1 = by address
    char               joinAddr[2][64];   // +0x1EE (feb +0x1D6)  "ip:port" (+ alternate "altip:port" from Ubi.com)
    int                pingServerId;      // +0x26E  1.2: NEW. server id the pinger is measuring, -1 none
    int                bLadder;           // +0x272 (feb +0x256)  Ubi ladder match (netDRV_SERVER_CREATE +0x12)
    int                bInited;           // +0x276 (feb +0x25A)
    int                bServerStarted;    // +0x27A (feb +0x25E)
    int                bBrowserStarted;   // +0x27E (feb +0x262)
    int                bClientJoined;     // +0x282 (feb +0x266)
    HANDLE             hPlayersMutex;     // +0x286 (feb +0x26A)  guards players (taken recursively in places)
    arrVector<netDRV_DPUBI_PLAYER> players; // +0x28A (feb +0x26E) count +0x28E, cap +0x292
};                                        // sizeof 0x296 (feb 0x27A)

// ---------------------------------------------------------------------------
// drv:network_dpubi_ubi.obj (summary only)
// /* Ubi.com service: not reconstructed (dead service / licensing) */
//   netUBI_DOWNLOADER   gsinit.php download       StartDownload 0x00595A50, UbiHttpCallback 0x00595BA0
//   netUBI_SERVERREG    lobby registration (host) ctor 0x00595BC0, Shutdown 0x00595C80, IsConnected 0x00595D90,
//                       IsError 0x00595DA0, Think 0x00595DC0, GetStatusString 0x00596160,
//                       NotifyClientLeave 0x00596A60, GetErrorString 0x00596A80, UpdateGameInfo 0x00596EC0,
//                       GetMatchStatus 0x00596F90, RestartLadder 0x00597020, StopLadder 0x00597050,
//                       StartMatchData 0x00597080, SendMatchData 0x005970D0, FinishMatchData 0x00597110
//   netUBI_CLIENT       lobby browser (client)    ctor 0x005973D0, Shutdown 0x00597520, Think 0x005975C0,
//                       EnumServers 0x005978B0, JoinGame 0x00597930, IsConnected 0x005979C0,
//                       IsConnectError 0x005979F0, IsJoined 0x00597A10, IsJoinError 0x00597A40,
//                       GetConnectErrorString 0x00597A60, GetJoinErrorString 0x00597A80,
//                       GetStatusString 0x00597AA0, GetServerAddress 0x00597B00 (1.2, name ours),
//                       IsServersEqual 0x00597B30, OnServerDelete 0x00597B80, StopBrowsing 0x00597B90,
//                       DisconnectFromGame 0x00597C10, NotifyServerJoined 0x00597C50
//   netUBI_MINICLIENT   clMSClientClass callbacks (see mp_ubicom.md)
// ---------------------------------------------------------------------------
