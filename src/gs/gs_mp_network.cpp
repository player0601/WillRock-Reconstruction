// gs_mp_network.cpp - Will Rock 1.2 (v12.exe) multiplayer network facade
//
// Reconstructed documentation source.  NOT meant to compile into the game.
// Original object: gs:gs_mp_network.obj (feb 0x00519430..0x0051A5CF,
// v12 0x0052EA70..0x0052FE5F).  See gs_mp_network.h for the types.
//
// netDRV_DPUBI virtual slots used here (v12 offset, feb offset in brackets).
// 1.2 inserted three slots, shifting everything after them:
//   +0x04 Think             [+0x04]     +0x50 Server_Stop           [+0x48]
//   +0x08 Flush        1.2: NEW         +0x54 Server_StartMatchData [+0x4C]
//   +0x0C Drv_Init          [+0x08]     +0x58 Server_SetMatchData   [+0x50]
//   +0x10 Drv_IsInited      [+0x0C]     +0x5C Server_FinishMatchData[+0x54]
//   +0x14 Drv_IsError       [+0x10]     +0x60 Server_GetMatchStatus [+0x58]
//   +0x18 Drv_GetStatus     [+0x14]     +0x64 Server_StopLadder     [+0x5C]
//   +0x1C Drv_NotifyGameStarted [+0x18] +0x68 Server_ResetLadder    [+0x60]
//   +0x20 Drv_IsGameStarted [+0x1C]     +0x6C Browser_Start         [+0x64]
//   +0x24 Drv_Freeze        [+0x20]     +0x70 Browser_IsStarted     [+0x68]
//   +0x28 Drv_IsLadderSupported [+0x24] +0x74 Browser_IsError       [+0x6C]
//   +0x2C Drv_GetRefreshInterval [+0x28]+0x78 Browser_GetStatus     [+0x70]
//   +0x30 Server_Start      [+0x2C]     +0x7C Browser_RefreshServers[+0x74]
//   +0x34 Server_IsStarted  [+0x30]     +0x80 Browser_Stop          [+0x78]
//   +0x38 Server_IsError    [+0x34]     +0x84 Client_JoinGame       [+0x7C]
//   +0x3C Server_GetStatus  [+0x38]     +0x88 Client_IsJoined       [+0x80]
//   +0x40 Server_GetClientPing 1.2: NEW +0x8C Client_IsError        [+0x84]
//   +0x44 Server_SendToClient [+0x3C]   +0x90 Client_GetStatus      [+0x88]
//   +0x48 Server_SetMainData [+0x40]    +0x94 Client_GetPing   1.2: NEW
//   +0x4C Server_GetMainData [+0x44]    +0x98 Client_SendToServer   [+0x8C]
//                                       +0x9C Client_DisconnectFromGame [+0x90]

#include "gs_mp_network.h"
// [integrator: all MP headers, so cross-object types are complete for the syntax check]
#include "network.h"
#include "network_dpubi.h"
#include "network_dpubi_dp.h"
#include "mp_messages.h"
#include "gs_mp_conn.h"
#include "gs_mp_network.h"
#include "gs_mp_misc.h"
#include "gs_mp_client.h"
#include "gs_mp_server.h"
#include "gs_play.h"
#include "wr_go_mp.h"
#include "wr_ui_mult.h"
#include "wr_ui_server.h"
#include "wr_ui_join.h"
// [integrator] explicit specializations defined below, declared before their first use
template<> void arrVector<gsMP_STAT::TRAFFIC_REC>::Resize(int);
template<> void arrVector<gsMP_STAT::TRAFFIC_REC>::Reserve(int);
template<> void arrVector<gsMP_STAT::TRAFFIC_REC>::Realloc(int);

#define NET_TIMEOUT_DRV       10.0f   // 0x41200000  Drv_Create
#define NET_TIMEOUT_DEFAULT   20.0f   // 0x41A00000  server / browser / join / ladder
#define NET_TIMEOUT_EXTEND     5.0f   // 0x40A00000 @0x005F980C  per progress event
#define NET_TIMEOUT_EXPIRED   -1.0f   // 0xBF800000
#define NET_DEFAULT_REFRESH   20.0f   // @0x005F93F0  Drv_GetRefreshInterval without driver
#define NET_PING_UNKNOWN      9999    // 0x270F
#define NET_DRV_SIZE          0x296   // sizeof(netDRV_DPUBI) 1.2 (feb 0x27A)

gsMP_NETWORK*      gsSysNetwork;      // 0x0099D814
gsMP_NET_FLUSHER*  gsNetFlusher;      // 0x0099D81C  1.2: new

// v12 0x0052F1B0  (feb 0x00519B70 "_$E2")  static initializer: a file-static squared epsilon
static float s_epsSq = 1e-6f * 1e-6f; // 0x0099D818 = [0x00608368]^2; never read in this file // ?

//============================================================================
// gsMP_STAT
//============================================================================

// v12 0x0050F120  (feb 0x004FC400, inline in gs_mp_misc.obj)  empty traffic table, then Clear
gsMP_STAT::gsMP_STAT()
{
    traffic.data = NULL; traffic.capacity = 0; traffic.size = 0;
    Clear();
}

// v12 0x0052EA70  (feb 0x00519430)  zero the rates and drop all traffic records
void gsMP_STAT::Clear()
{
    rateA = 0; rateB = 0; drvValue10 = 0;
    traffic.Resize(0);          // inlined arrVector shrink: zero-fill loop is dead for size 0
    windowTime = 0;
}

// v12 0x0052EAD0  (feb 0x00519490)  add 'bytes' to the record 'id' (append a new record if missing)
void gsMP_STAT::AddTraffic(int id, int bytes)
{
    int i;
    for (i = 0; i < traffic.size; i++)
        if (traffic.data[i].id == id) {
            traffic.data[i].totalBytes  += bytes;
            traffic.data[i].windowBytes += bytes;
            break;
        }
    if (i == traffic.size) {
        // arrVector::Insert(size, 1) inlined: grow x2, memmove, zero new slot
        int n = traffic.size + 1;
        if (traffic.capacity < n)
            traffic.Reserve(traffic.capacity * 2 > n ? traffic.capacity * 2 : n);
        memset(&traffic.data[traffic.size], 0, sizeof(TRAFFIC_REC));
        traffic.size++;
        traffic.data[traffic.size - 1].id          = id;
        traffic.data[traffic.size - 1].totalBytes  = bytes;
        traffic.data[traffic.size - 1].windowBytes = bytes;
    }
}
// Callers (v12): gsMP_SERVER::SendMsgClient 0x0051054B, gsMP_CLIENT 0x0053960A.

// v12 0x0052FDA0  (feb: inlined in the ctor)  arrVector<TRAFFIC_REC>::Resize(n): realloc to n, zero new tail
// 1.2: emitted out of line; only caller is gsMP_NETWORK::gsMP_NETWORK with n = 0.  Name invented. // ?
template<> void arrVector<gsMP_STAT::TRAFFIC_REC>::Resize(int n)
{
    if (n > size) {
        if (n > capacity) { data = (gsMP_STAT::TRAFFIC_REC*)apRealloc(data, n << 4); capacity = n; }
        for (int i = size; i < n; i++) memset(&data[i], 0, sizeof(data[i]));
    }
    size = n;
}

// v12 0x0052FE00  (feb 0x0051A570)  grow capacity to at least n
template<> void arrVector<gsMP_STAT::TRAFFIC_REC>::Reserve(int n)
{
    if (n > capacity) {
        if (n < size) size = n;
        data = (gsMP_STAT::TRAFFIC_REC*)apRealloc(data, n << 4);
        capacity = n;
    }
}

// v12 0x0052FE30  (feb 0x0051A5A0)  set capacity to exactly n
template<> void arrVector<gsMP_STAT::TRAFFIC_REC>::Realloc(int n)
{
    if (n != capacity) {
        if (n < size) size = n;
        data = (gsMP_STAT::TRAFFIC_REC*)apRealloc(data, n << 4);
        capacity = n;
    }
}

//============================================================================
// gsMP_NET_FLUSHER  (1.2: new, no feb twin)
//============================================================================

// v12 0x0052EBD0  (no feb)  destructor: clears the global pointer
gsMP_NET_FLUSHER::~gsMP_NET_FLUSHER()
{
    gsNetFlusher = NULL;        // 1.2
    // msgADDR::~msgADDR (0x004CFCC0); operator delete if (flags & 1)
}

// v12 0x0052EC00  (no feb)  on the frame tick, flush the driver's coalesced send queues
int gsMP_NET_FLUSHER::ProcessMsg(int msg, void* data, msgADDR* from, int* res)
{
    if (msg == 1000 /*0x3E8 frame tick*/ && drv != NULL)
        drv->Flush();           // 1.2: driver slot +0x08 -> netDP_SERVER Flush 0x00598830 /
                                //      netDP_CLIENT Flush 0x00599760 (700-byte packets)
    return 0;
}

//============================================================================
// gsMP_NETWORK
//============================================================================

// v12 0x0052EC20  (feb 0x00519590)  constructor; 1.2 also creates and registers the flusher
gsMP_NETWORK::gsMP_NETWORK()
{
    // msgADDR base ctor inlined: +4..+0x24 = 0, +0x10 = 'MADR'
    // gsMP_STAT ctor inlined (traffic = {NULL,0,0}; Clear())
    stat.traffic.data = NULL; stat.traffic.capacity = 0; stat.traffic.size = 0;
    stat.traffic.Resize(0);                     // 1.2: out-of-line call 0x0052FDA0 (feb inline)
    stat.Clear();                               // inlined (Reserve(0) path)

    drvType  = -1;
    drv      = NULL;
    userName[0] = 0;
    password[0] = 0;
    drvTimeout = serverTimeout = browserTimeout = clientTimeout = ladderTimeout = 0;
    stat.Clear();                               // inlined a second time (Realloc(0) path); feb same

    // 1.2: per-frame flush helper
    gsMP_NET_FLUSHER* f = new gsMP_NET_FLUSHER; // operator new(0x2C), ctor inlined
    if (f) {
        // msgADDR fields: +4,+8,+0xC = 0, +0x10 = 'MADR', +0x18/+0x20/+0x24 = 0
        f->addrId   = '$NEF';                   // +0x14 0x244E4546 (overwritten by InsertAddr) // ?
        f->order    = 0x54;                     // +0x1C  (84)  position in the msg loop    // ? meaning
        f->drv      = NULL;
        gsNetFlusher = f;
        msgSystem.InsertAddr(f, '$SYS', 0);     // v12 0x004D0820; sets +0x14 = '$SYS'
    }
}

// v12 0x0052EDA0  (feb 0x005196B0)  scalar deleting destructor: tear down the driver
gsMP_NETWORK::~gsMP_NETWORK()
{
    msgSystem.DestroyAddr(gsNetFlusher);        // 1.2: v12 0x004D08A0
    if (drv) {
        if (drvTimeout > 0) drvTimeout = 0;
        drv->Server_Stop();                     // +0x50
        drv->Client_DisconnectFromGame();       // +0x9C
        drv->Browser_Stop(0);                   // +0x80
        delete drv;                             // slot +0x00 (1)
        drv = NULL;
        gsNetFlusher->drv = NULL;               // 1.2 (no NULL check on gsNetFlusher)
    }
    apFree(stat.traffic.data);
    // msgADDR::~msgADDR (0x004CFCC0)
}

// v12 0x0052EE50  (feb 0x00519740)  per-frame statistics: KB/s from driver byte counters, 1 s windows
void gsMP_NETWORK::UpdStat(float dt)
{
    if (!drv) return;
    float k = dt * 1024.0f;                     // __real@44800000 @0x0060838C
    stat.rateB      = (float)drv->bytesB / k;   // drv+0x0C                       // ? direction
    stat.rateA      = (float)drv->bytesA / k;   // drv+0x08
    stat.drvValue10 = drv->value10;             // drv+0x10                       // ?
    stat.windowTime += dt;
    if (stat.windowTime > 1.0f) {
        for (int i = 0; i < stat.traffic.size; i++) {
            stat.traffic.data[i].bytesPerSec = (int)(stat.traffic.data[i].windowBytes / stat.windowTime);
            stat.traffic.data[i].windowBytes = 0;
        }
        stat.windowTime = 0;
    }
    drv->bytesA = 0;
    drv->bytesB = 0;
}

// v12 0x0052EF10  (feb 0x00519800)  destroy any old driver, create netDRV_DPUBI of 'type' and init it
// 1.2: second argument versionId (callers pass 10020000 from 0x0098E4A0), dedicated flag,
//      game name "WILLROCK" instead of "WILLROCK_DEMO", driver object 0x296 bytes.
void gsMP_NETWORK::Drv_Create(int type, int versionId)
{
    netDRV_INIT init;

    if (drv) {                                  // same sequence as Drv_Stop
        if (drvTimeout > 0) drvTimeout = 0;
        drv->Server_Stop();                     // +0x50
        drv->Client_DisconnectFromGame();       // +0x9C
        drv->Browser_Stop(0);                   // +0x80
        delete drv;
        drv = NULL;
        gsNetFlusher->drv = NULL;               // 1.2
    }

    drvType = type;
    init.dedicated = gsAppState & 0x08000000;   // 1.2: -dedicated
    switch (type) {
    case 0:                                     // LAN: gameName/gameVersion left uninitialised
        init.type = 0;
        break;
    case 1:                                     // Ubi.com
        init.type        = 1;
        init.gameName    = "WILLROCK";          // 0x00635C68  1.2: was "WILLROCK_DEMO"
        init.gameVersion = "WRPC1.0";           // 0x00635C60
        break;
    default:
        goto done;
    }
    drv = new netDRV_DPUBI;                     // operator new(0x296), ctor 0x005902C0
    gsNetFlusher->drv = drv;                    // 1.2
    init.versionId = versionId;                 // 1.2
    drv->Drv_Init(&init);                       // +0x0C -> 0x00590BC0
done:
    if (drv) {
        userName[0] = 0;
        password[0] = 0;
        drv->SetCallback(OnNetDrvEvent);        // netDRV::SetCallback 0x0058F5B0
        drvTimeout = NET_TIMEOUT_DRV;           // 10 s
    }
}

// v12 0x0052F060  (feb 0x005198E0)  1 when the driver finished init (stops the timer)
int gsMP_NETWORK::Drv_IsCreated()
{
    if (drv && drv->Drv_IsInited()) {           // +0x10
        drvTimeout = 0;
        return 1;
    }
    return 0;
}

// v12 0x0052F090  (feb 0x00519910)  driver error or init timed out
int gsMP_NETWORK::Drv_IsError()
{
    if (drv && drv->Drv_IsError()) return 1;    // +0x14
    return drvTimeout < 0;
}

// v12 0x0052F0D0  (feb 0x00519950)  status text for the "mp_wait_drvinit" window
const char* gsMP_NETWORK::Drv_GetStatus()
{
    if (!drv) return "Unknown";                 // 0x00635C84
    if (drv->Drv_IsError()) return drv->Drv_GetStatus();   // +0x14 / +0x18
    if (drvTimeout < 0)     return "Init timeout";         // 0x00635C74
    return drv->Drv_GetStatus();
}

// v12 0x0052F120  (feb 0x005199A0)  stop everything and delete the driver
void gsMP_NETWORK::Drv_Stop()
{
    if (!drv) return;
    if (drvTimeout > 0) drvTimeout = 0;
    drv->Server_Stop();                         // +0x50
    drv->Client_DisconnectFromGame();           // +0x9C
    drv->Browser_Stop(0);                       // +0x80
    delete drv;                                 // slot +0x00, flag 1
    drv = NULL;
    gsNetFlusher->drv = NULL;                   // 1.2
}

// v12 0x0052F190  (feb 0x00519A00)  "LAN" / "UBI" / ""
const char* gsMP_NETWORK::Drv_GetIdString()
{
    switch (drvType) {
    case 0:  return "LAN";                      // 0x00635C8C
    case 1:  return "UBI";                      // 0x00635C90
    default: return "";                         // 0x00640358
    }
}

// v12 0x0052F1D0  (feb: no gsMP_NETWORK twin; driver slot existed as feb +0x24)  1.2: new
// Used by wrUI_ADVSERV_WINDOW (call 0x0049E423) to enable the ladder option.
int gsMP_NETWORK::Drv_IsLadderSupported()
{
    if (!drv) return 0;
    return drv->Drv_IsLadderSupported();        // +0x28
}

// v12 0x0052F1E0  (feb 0x00519A20)  only the Ubi.com driver asks for a password
int gsMP_NETWORK::Drv_IsPasswordRequired()
{
    return drvType == 1;
}

// v12 0x0052F200  (feb 0x00519A40)  store the (Ubi.com) user name and password, 63 chars max
void gsMP_NETWORK::Drv_SetUserNameAndPassword(const char* user, const char* pw)
{
    strncpy(userName, user, 0x40); userName[0x3F] = 0;
    strncpy(password, pw,   0x40); password[0x3F] = 0;
}

// v12 0x0052F240  (feb 0x00519A80)
int gsMP_NETWORK::Drv_IsGameStarted()
{
    return drv ? drv->Drv_IsGameStarted() : 0;  // +0x20
}

// v12 0x0052F250  (feb 0x00519A90)
void gsMP_NETWORK::Drv_NotifyGameStarted()
{
    if (drv) drv->Drv_NotifyGameStarted();      // +0x1C
}

// v12 0x0052F260  (feb 0x00519AA0)
void gsMP_NETWORK::Drv_Freeze(int freeze)
{
    if (drv) drv->Drv_Freeze(freeze);           // +0x24
}

// v12 0x0052F270  (feb 0x00519AB0)  browser auto-refresh period in seconds
float gsMP_NETWORK::Drv_GetRefreshInterval()
{
    if (drv) return drv->Drv_GetRefreshInterval(); // +0x2C
    return NET_DEFAULT_REFRESH;                 // 20.0
}

//---------------------------------------------------------------------------- server

// v12 0x0052F290  (feb 0x00519AD0)  host a game; 20 s creation timeout
void gsMP_NETWORK::Server_Start(const gsMP_SERVER_INFO* info)
{
    netDRV_SERVER_CREATE sc;
    switch (drvType) {
    case 0:
        sc.info = info;
        sc.maxPlayers = info->maxPlayers;       // 1.2: +0x44 (feb +0x40)
        sc.reserved0 = 0; sc.reserved1 = 0;
        sc.port     = (unsigned short)info->port; // 1.2: +0x50 (feb +0x4C)
        sc.ladder   = 0;
        sc.userName = userName;
        sc.password = "";
        break;
    case 1:
        sc.info = info;
        sc.maxPlayers = info->maxPlayers;
        sc.reserved0 = 0; sc.reserved1 = 0;
        sc.port     = (unsigned short)info->port;
        sc.ladder   = info->ladder;             // 1.2: +0x54 (feb +0x50)
        sc.userName = userName;
        sc.password = password;
        break;
    default:
        goto done;
    }
    drv->Server_Start(&sc);                     // +0x30 -> 0x00590CB0
done:
    serverTimeout = NET_TIMEOUT_DEFAULT;        // 20 s (also set when no call was made)
}

// v12 0x0052F330  (feb 0x00519B90)
int gsMP_NETWORK::Server_IsStarted()
{
    if (drv && drv->Server_IsStarted()) {       // +0x34
        serverTimeout = 0;
        return 1;
    }
    return 0;
}

// v12 0x0052F360  (feb 0x00519BC0)
int gsMP_NETWORK::Server_IsError()
{
    if (drv && drv->Server_IsError()) return 1; // +0x38
    return serverTimeout < 0;
}

// v12 0x0052F3A0  (feb 0x00519C00)
const char* gsMP_NETWORK::Server_GetStatus()
{
    if (!drv) return "Unknown";
    if (drv->Server_IsError()) return drv->Server_GetStatus();   // +0x38 / +0x3C
    if (serverTimeout < 0)     return "Server create timeout";   // 0x00635C94
    return drv->Server_GetStatus();
}

// v12 0x0052F3F0  (feb 0x00519C50)
void gsMP_NETWORK::Server_Stop()
{
    if (!drv) return;
    drv->Server_Stop();                         // +0x50
    if (serverTimeout > 0) serverTimeout = 0;
}

// v12 0x0052F420  (feb 0x00519C80)  send to one client; translates game flags to driver flags
void gsMP_NETWORK::Server_SendDataToClient(int client, const void* data, int size, int flags)
{
    if (drv && drv->Server_IsStarted()) {       // +0x34
        serverTimeout = 0;
        int f = 0;
        if (flags & GSMP_SEND_RELIABLE) f = 1;
        if (flags & GSMP_SEND_TIMEOUT)  f |= 2;
        drv->Server_SendToClient(client, data, size, f); // +0x44 (tail jump)
    }
}

// v12 0x0052F470  (feb 0x00519CD0)  the 0x30-byte "main data" blob shown to browsers
void gsMP_NETWORK::Server_SetMainData(const void* data, int size)
{
    if (drv) drv->Server_SetMainData(data, size); // +0x48
}

// v12 0x0052F480  (feb 0x00519CE0)
void gsMP_NETWORK::Server_GetMainData(void* data, int* size)
{
    if (drv) drv->Server_GetMainData(data, size); // +0x4C
}

//---------------------------------------------------------------------------- browser

// v12 0x0052F490  (feb 0x00519CF0)  start server enumeration; 20 s timeout
void gsMP_NETWORK::Browser_Start()
{
    netDRV_BROWSER_CREATE bc;
    switch (drvType) {
    case 0:  bc.userName = userName; bc.hosts = "";       break;  // "" = LAN broadcast (push at 0x0052F4B4)
    case 1:  bc.userName = userName; bc.hosts = password; break;
    default: goto done;
    }
    drv->Browser_Start(&bc);                    // +0x6C -> 0x005911F0
done:
    browserTimeout = NET_TIMEOUT_DEFAULT;
}

// v12 0x0052F4E0  (feb 0x00519D40)
int gsMP_NETWORK::Browser_IsStarted()
{
    if (drv && drv->Browser_IsStarted()) {      // +0x70
        browserTimeout = 0;
        return 1;
    }
    return 0;
}

// v12 0x0052F510  (feb 0x00519D70)
int gsMP_NETWORK::Browser_IsError()
{
    if (drv && drv->Browser_IsError()) return 1; // +0x74
    return browserTimeout < 0;
}

// v12 0x0052F550  (feb 0x00519DB0)
const char* gsMP_NETWORK::Browser_GetStatus()
{
    if (!drv) return "Unknown";
    if (drv->Browser_IsError()) return drv->Browser_GetStatus();  // +0x74 / +0x78
    if (browserTimeout < 0)     return "Browser create timeout";  // 0x00635CAC
    return drv->Browser_GetStatus();
}

// v12 0x0052F5A0  (feb 0x00519E00)
void gsMP_NETWORK::Browser_Stop(int param)      // ? param meaning (UI passes 1, Drv_Stop passes 0)
{
    if (drv && drv->Browser_IsStarted()) {      // +0x70
        browserTimeout = 0;
        drv->Browser_Stop(param);               // +0x80
    }
}

// v12 0x0052F5D0  (feb 0x00519E30)
void gsMP_NETWORK::Browser_RefreshServers()
{
    if (drv && drv->Browser_IsStarted()) {
        browserTimeout = 0;
        drv->Browser_RefreshServers();          // +0x7C
    }
}

// v12 0x0052F600  (feb 0x00519E60)  read-and-clear "list changed" flag (drv+0x86)
int gsMP_NETWORK::Browser_IsServerListUpdated()
{
    if (drv && drv->Browser_IsStarted()) {
        browserTimeout = 0;
        return drv->netDRV::Browser_IsServerListUpdated(); // non-virtual 0x0058FB70
    }
    return 0;
}

// v12 0x0052F630  (feb 0x00519E90)  no NULL check on drv
int gsMP_NETWORK::Browser_IsServerIdValid(int id)
{
    return drv->netDRV::Browser_GetServerInfo(id) != NULL; // 0x0058FBF0
}

// v12 0x0052F650  (feb 0x00519EB0)  iterate ids: -1 -> first, returns -1 at the end
int gsMP_NETWORK::Browser_GetNextServer(int id)
{
    return drv->netDRV::Browser_GetNextServer(id); // tail jump 0x0058FB90
}

// v12 0x0052F660  (feb 0x00519EC0)  copy one browsed server into the UI struct
void gsMP_NETWORK::Browser_GetServerInfo(int id, gsMP_SERVER_INFO* out)
{
    const netDRV_SERVER_INFO* s = drv->netDRV::Browser_GetServerInfo(id);
    if (s == NULL) {                            // 1.2: feb dereferenced NULL
        out->name[0]    = 0;
        out->versionId  = 0;
        out->maxPlayers = 0;
        out->curPlayers = 0;
        out->port       = 0;
        out->ping       = NET_PING_UNKNOWN;     // 9999
        return;
    }
    strncpy(out->name, s->name, 0x40); out->name[0x3F] = 0;
    out->versionId  = s->netVersion;            // 1.2: entry +0x44
    out->maxPlayers = s->maxPlayers;            // entry +0x48 (feb +0x44)
    out->curPlayers = s->curPlayers;            // entry +0x4C (feb +0x48)
    out->ping       = s->ping;                  // entry +0x50 (feb +0x4C)
    out->port       = 0;
}

// v12 0x0052F6D0  (feb 0x00519F10)  copy the server's main-data blob; *size in/out
void gsMP_NETWORK::Browser_GetServerMainData(int id, void* buf, int* size)
{
    const netDRV_SERVER_INFO* s = drv->netDRV::Browser_GetServerInfo(id);
    if (s == NULL) { *size = 0; return; }       // 1.2: new NULL check
    if (buf != NULL && *size >= s->mainData.size) {
        memcpy(buf, s->mainData.data, s->mainData.size); // entry +0x54/+0x58 (feb +0x50/+0x54)
        *size = s->mainData.size;
        return;
    }
    // 1.2: feb only reported the needed size here.  v12 instead copies *size bytes
    // (truncated copy) into buf and leaves *size unchanged; with buf == NULL and
    // *size > 0 this writes to NULL.                                         // ? intent
    memcpy(buf, s->mainData.data, *size);
}

//---------------------------------------------------------------------------- client

// v12 0x0052F740  (feb 0x00519F60)  join a server from the browser list; 20 s join timeout
void gsMP_NETWORK::Client_JoinBrowsedGame(int id)
{
    if (drvType >= 0 && drvType <= 1) {
        netDRV_CLIENT_JOIN j;
        j.serverId = id;
        j.address  = NULL;
        j.userName = userName;
        j.password = password;
        drv->Client_JoinGame(&j);               // +0x84 -> 0x005915F0
    }
    clientTimeout = NET_TIMEOUT_DEFAULT;
}

// v12 0x0052F7A0  (feb 0x00519FB0)  join "host:port" directly (Join I-Net Server, call 0x0049CE30)
void gsMP_NETWORK::Client_JoinAddress(const char* addr)
{
    if (drvType >= 0 && drvType <= 1) {
        netDRV_CLIENT_JOIN j;
        j.serverId = -1;
        j.address  = addr;
        j.userName = userName;
        j.password = password;
        drv->Client_JoinGame(&j);               // +0x84
    }
    clientTimeout = NET_TIMEOUT_DEFAULT;
}

// v12 0x0052F800  (feb 0x0051A000)
int gsMP_NETWORK::Client_IsJoined()
{
    if (drv && drv->Client_IsJoined()) {        // +0x88
        clientTimeout = 0;
        return 1;
    }
    return 0;
}

// v12 0x0052F830  (feb 0x0051A030)  no NULL check on drv (both builds)
int gsMP_NETWORK::Client_IsError()
{
    if (drv->Client_IsError()) return 1;        // +0x8C
    return clientTimeout < 0;
}

// v12 0x0052F870  (feb 0x0051A070)  no NULL check on drv
const char* gsMP_NETWORK::Client_GetStatus()
{
    if (drv->Client_IsError()) return drv->Client_GetStatus();   // +0x8C / +0x90
    if (clientTimeout < 0)     return "Client create timeout";   // 0x00635CC4
    return drv->Client_GetStatus();
}

// v12 0x0052F8C0  (feb 0x0051A0C0)
void gsMP_NETWORK::Client_DisconnectFromGame()
{
    if (drv && drv->Client_IsJoined()) {
        clientTimeout = 0;
        drv->Client_DisconnectFromGame();       // +0x9C
    }
}

// v12 0x0052F8F0  (no feb)  1.2: new - round trip to the server in ms, -1 without driver
// Callers: HUD 0x0049707F (ping display), 0x0050CAB4.
int gsMP_NETWORK::Client_GetPing()
{
    if (!drv) return -1;
    return drv->Client_GetPing();               // 1.2: slot +0x94
}

// v12 0x0052F910  (feb 0x0051A0F0)  send to the server; same flag translation as the server side
void gsMP_NETWORK::Client_SendDataToServer(const void* data, int size, int flags)
{
    if (drv && drv->Client_IsJoined()) {        // +0x88
        clientTimeout = 0;
        int f = 0;
        if (flags & GSMP_SEND_RELIABLE) f = 1;
        if (flags & GSMP_SEND_TIMEOUT)  f |= 2;
        drv->Client_SendToServer(data, size, f); // +0x98
    }
}

//---------------------------------------------------------------------------- frame tick

// v12 0x0052F970  (feb 0x0051A150)  msg 1000: driver Think, then count down the five timeouts
int gsMP_NETWORK::ProcessMsg(int msg, void* data, msgADDR* from, int* res)
{
    if (msg == 1000) {
        if (drv) drv->Think();                  // +0x04
        // A timer only runs while > 0; it reaches "expired" by going below 0.
        // Landing exactly on 0.0 is mapped to -1 so it still counts as expired.
        float* t[5] = { &drvTimeout, &serverTimeout, &browserTimeout, &clientTimeout, &ladderTimeout };
        for (int i = 0; i < 5; i++)             // written out five times in the binary
            if (*t[i] > 0) {
                *t[i] -= gsElapsedTime;
                if (*t[i] == 0) *t[i] = NET_TIMEOUT_EXPIRED;
            }
    }
    return 0;
}
// NB: the driver's send queues are flushed by gsMP_NET_FLUSHER, not here (1.2).

//---------------------------------------------------------------------------- Ubi.com ladder
// Thin facade over driver slots.  /* Ubi.com service: not reconstructed (dead service / licensing) */

// v12 0x0052FAC0  (feb 0x0051A2A0)
void gsMP_NETWORK::Server_ResetLadder()
{
    if (drv) { drv->Server_ResetLadder(); ladderTimeout = 0; }   // +0x68
}

// v12 0x0052FAE0  (feb 0x0051A2C0)
void gsMP_NETWORK::Server_StopLadder()
{
    if (drv) { drv->Server_StopLadder(); ladderTimeout = 0; }    // +0x64
}

// v12 0x0052FB00  (feb 0x0051A2E0)  0 idle, 1 sending, 2 sent/ready, 3 failed or timed out
int gsMP_NETWORK::Server_GetLadderStatus()
{
    if (ladderTimeout < 0) return 3;
    if (drv) {
        switch (drv->Server_GetMatchStatus()) { // +0x60, jump table 0x0052FB40
        case 1: case 2: return 1;
        case 3:         return 2;
        case 4:         return 3;
        }
    }
    return 0;
}

// v12 0x0052FB60  (feb 0x0051A340)  begin match-result upload
void gsMP_NETWORK::Server_StartLadderSending()
{
    if (!drv) return;
    drv->Server_StartMatchData();               // +0x54
    if (drv->Server_GetMatchStatus() == 2)      // 1.2: timer armed only if the driver reports "sending"
        ladderTimeout = NET_TIMEOUT_DEFAULT;    // feb: armed unconditionally
}

// v12 0x0052FB90  (feb 0x0051A360)
void gsMP_NETWORK::Server_FinishLadderSending()
{
    if (drv) drv->Server_FinishMatchData();     // +0x5C
}

// v12 0x0052FBA0  (feb 0x0051A370)  8 call sites (one per result field)
void gsMP_NETWORK::Server_SendLadderData(const char* a, int b, int c)
{
    if (drv) drv->Server_SetMatchData(a, b, c); // +0x58
}

//---------------------------------------------------------------------------- driver events

// v12 0x0052FBB0  (feb 0x0051A380)  driver callback (fastcall, ecx = event); jump table 0x0052FD74
// Unchanged in 1.2 apart from addresses.
void __fastcall gsMP_NETWORK::OnNetDrvEvent(const netDRV_EVENT* ev)
{
    float* t;
    switch (ev->type) {
    case NETDRV_EV_DRV_PROGRESS:     t = &gsSysNetwork->drvTimeout;     goto extend;
    case NETDRV_EV_SERVER_PROGRESS:  t = &gsSysNetwork->serverTimeout;  goto extend;
    case NETDRV_EV_BROWSER_PROGRESS: t = &gsSysNetwork->browserTimeout; goto extend;
    case NETDRV_EV_CLIENT_PROGRESS:  t = &gsSysNetwork->clientTimeout;
    extend:
        // a running timer gets +5 s, but never less than 20 s
        if (*t > 0) {
            *t += NET_TIMEOUT_EXTEND;
            if (*t < NET_TIMEOUT_DEFAULT) *t = NET_TIMEOUT_DEFAULT;
        }
        return;

    case NETDRV_EV_CLIENT_CONNECTED:
        gsSysServer->AddClient(ev->client);     // vt+0x20
        return;
    case NETDRV_EV_CLIENT_GONE:
        gsSysServer->RemoveClient(ev->client);  // vt+0x24
        return;
    case NETDRV_EV_SERVER_DATA:
        gsSysServer->ReceiveMsg(ev->client, ev->data.data, ev->data.size); // v12 0x005106F0
        return;
    case NETDRV_EV_CLIENT_DATA:
        gsSysClient->ReceiveMsg(ev->data.data, ev->data.size);              // vt+0x30   // ? name
        return;
    }
}
