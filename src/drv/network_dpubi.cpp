// network_dpubi.cpp -- netDRV_DPUBI: DirectPlay 8 + Ubi.com multiplayer driver (Will Rock 1.2, v12.exe)
//
// Reconstructed for documentation (feb obj: drv:network_dpubi.obj). Not meant to compile into the game.
// See network_dpubi.h for the layout, the vtable and the types assumed from other files.
// Every function: "// v12 <addr>  (feb <addr>)" + one line.  "// 1.2:" = changed since feb.  "// ?" = unproven.
//
// Notation: arrVector<T> {T* data; int count; int capacity;}.  actions.Add(x) stands for the inlined
// arrVector<int>::Insert(actions.count, x) the compiler emitted (call 0x0051DC10 or the inline grow+memmove).

#include "network_dpubi.h"
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

// Game -> DirectPlay application GUID (DPN_APPLICATION_DESC.guidApplication).  v12 0x0060A688.
// 1.2 / 1.0 retail {C3BD3D5B-CD74-4532-9B63-2CBDC2C51C89};  feb had ...9B63-2ABD... (byte 10: 0x2A -> 0x2C).
const GUID netDPUBI_AppGuid = { 0xC3BD3D5B, 0xCD74, 0x4532, { 0x9B, 0x63, 0x2C, 0xBD, 0xC2, 0xC5, 0x1C, 0x89 } };

static const char s_InvalidName[]      = "\x01invalid player name\x01";  // v12 0x0063B0EC
static const char s_InitializingServer[] = "Initializing server";          // v12 0x0063B0D8
static const char s_BrowserStarted[]   = "Browser started";              // v12 0x0063B0B8
static const char s_AddrFmt[]          = "%s:%i";                        // v12 0x0063B104

// Small helper used by the blob builders below: append n raw bytes to an arrVector<unsigned char>.
// (Inlined everywhere in the binary: grow to max(2*cap, need) with apRealloc, memmove, byte copy.)
static inline void AppendBytes(arrVector<unsigned char>* v, const void* p, int n);

// Header of every driver blob (enum query / enum response / connect data / connect reply).
#pragma pack(push, 1)
struct netDPUBI_BLOB_HDR
{
    unsigned short type;        // +0 NETDP_BLOB_GAME (1) or NETDP_BLOB_PING (0xFFFF)
    unsigned short hdrLen;      // +2 offset of the variable part
};
#pragma pack(pop)

// ===========================================================================
// construction
// ===========================================================================

// v12 0x005902C0  (feb 0x005769D0)
// ctor: zero all state, init the Ubi.com SDK sockets, create the player-table mutex.
netDRV_DPUBI::netDRV_DPUBI()
    : netDRV()                                         // 0x0058F320
{
    actions.data = NULL; actions.capacity = 0; actions.count = 0;
    players.data = NULL; players.capacity = 0; players.count = 0;
    action       = netACT_NONE;
    bUbi         = 0;
    userName[0] = password[0] = gameName[0] = gameVersion[0] = 0;
    joinServerId = -1;
    for (int i = 0; i < 2; i++) joinAddr[i][0] = 0;
    bInited = bServerStarted = bBrowserStarted = bClientJoined = 0;
    server    = NULL;
    client    = NULL;
    serverReg = NULL;
    ubiClient = NULL;
    pinger    = NULL;                                  // 1.2: NEW member
    bLadder   = 0;
    bDpEnumEnded  = 0;                                 // 1.2: NEW member
    bUbiEnumEnded = 0;                                 // 1.2: NEW member
    // ? pingServerId (+0x26E) is not initialised here; Think action 10 sets it to -1.

    char err[129] = "";                                // 1 + 0x20 dwords, zeroed
    InitializeSockets(err);                            // 0x005A1A00  /* Ubi.com service: not reconstructed */
    hPlayersMutex = CreateMutexA(NULL, FALSE, NULL);
}

// v12 0x005903E0  (feb 0x00576AE0)   (scalar deleting dtor ??_G at 0x005903C0)
// dtor: drop every player, flush disconnect events, delete all sub-objects.
netDRV_DPUBI::~netDRV_DPUBI()
{
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    for (int i = 0; i < players.count; i++)
    {
        if (players.data[i].flags != 0)
        {
            ReleaseMutex(hPlayersMutex);
            RemovePlayer(i);
            WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
        }
    }
    ReleaseMutex(hPlayersMutex);
    ProcessEvents(1);                                  // deliver pending netEV_CLIENT_LEFT only

    if (server)
    {
        server->Shutdown();                            // 0x005983C0
        delete server;                                 // ~netDP_SERVER 0x00598300
        server = NULL;                                 // 1.2: pointers are now cleared (feb did not)
    }
    if (client)
    {
        ClearServersList();
        client->Shutdown();                            // 0x00598F80
        delete client;                                 // ~netDP_CLIENT 0x00598F10
        client = NULL;
    }
    if (serverReg)
    {
        serverReg->Shutdown();                         // 0x00595C80
        delete serverReg;                              // virtual ??_G (vtbl+0)
        serverReg = NULL;
    }
    if (ubiClient)
    {
        ubiClient->Shutdown();                         // 0x00597520
        delete ubiClient;                              // virtual ??_G (vtbl+0x20)
        ubiClient = NULL;
    }
    if (pinger)                                        // 1.2: NEW
    {
        delete pinger;                                 // ~netDP_PINGER 0x00595380
        pinger = NULL;
    }
    UninitializeSockets();                             // 0x005A1AA0  /* Ubi.com service: not reconstructed */
    CloseHandle(hPlayersMutex);
    apFree(players.data);
    apFree(actions.data);
    // ~netDRV() 0x0058F4E0 (tail jump)
}

// ===========================================================================
// Think: the action state machine
// ===========================================================================

// v12 0x00590590  (feb 0x00576C50)
// Per-frame tick (gsMP_NETWORK::ProcessMsg msg 1000 -> slot +0x04): run the current action, pump
// the sub-objects, and drop the "started/joined" flags when the transport went away.
void netDRV_DPUBI::Think()
{
    // 1.2: deferred list pruning (enumeration ended while a join had bAcceptServers == 0)
    if (bDpEnumEnded)  { PruneServers(0); bDpEnumEnded  = 0; }
    if (bUbiEnumEnded) { PruneServers(1); bUbiEnumEnded = 0; }

    switch (action)                                    // jump table 0x00590B24, 0..17
    {
    case netACT_NONE:
        break;                                         // -> FinishAction

    case netACT_SRV_DP_START:
        if (server)
            Server_Stop();                             // slot +0x50
        server = new netDP_SERVER(this);               // 0x38 bytes, ctor 0x005982C0 (feb 0x14 bytes)
        server->StartServer();                         // 0x00598410 (called even when new failed, as in feb)
        break;

    case netACT_SRV_DP_WAIT:
        if (server->bStarted)                          // netDP_SERVER +0x08
        {
            // 1.2: publish the (possibly re-bound) game info to the lobby right away
            if (bUbi && serverReg)
                serverReg->UpdateGameInfo();           // 0x00596EC0
            break;
        }
        if (server->hrError >= 0)                      // netDP_SERVER +0x0C
            goto think_tail;                           // still starting
        Server_Stop();
        break;

    case netACT_SRV_UBI_REGISTER:
        serverReg = new netUBI_SERVERREG(this, bLadder);   // 0x16C bytes, ctor 0x00595BC0
        break;

    case netACT_SRV_UBI_WAIT:
        if (serverReg->IsConnected())                  // 0x00595D90
            break;
        if (serverReg->IsError())                      // 0x00595DA0
            Server_Stop();                             // (no FinishAction: Server_Stop reset the queue)
        goto think_tail;

    case netACT_SRV_READY:
        bServerStarted = 1;
        break;

    case netACT_BRW_CLEAR:
        ClearServersList();                            // 0x0058FC80
        break;

    case netACT_BRW_DP_START:
    {
        if (client)
        {
            client->DisconnectFromGame();              // 0x00599970
            client->StopBrowsing(0);                   // 0x005994A0
        }
        const char* hosts[2];
        for (unsigned i = 0; i < 2; i++)
            hosts[i] = joinAddr[i];                    // "" = broadcast (LAN), else "ip[:port]"
        client = new netDP_CLIENT(this);               // 0x48 bytes, ctor 0x00598EC0 (feb 0x3C)
        client->StartBrowser(hosts, 2);                // 0x00598FE0
        break;
    }

    case netACT_BRW_DP_WAIT:
        if (client->bStarted)                          // netDP_CLIENT +0x08
            break;
        if (client->hrError < 0)                       // netDP_CLIENT +0x10
            Browser_Stop(0);                           // slot +0x80
        goto think_tail;

    case netACT_BRW_UBI_CONNECT:
        ubiClient = new netUBI_CLIENT(this);           // 0x138 bytes, ctor 0x005973D0
        break;

    case netACT_BRW_UBI_WAIT:
        if (ubiClient->IsConnected())                  // 0x005979C0
        {
            // 1.2: NEW -- create a DirectPlay pinger so Ubi.com-listed servers get a real ping.
            arrVector<unsigned char> query;            // {NULL,0,0}
            query.Resize(0);                           // 0x005900E0
            if (pinger)
            {
                delete pinger;                         // 0x00595380
            }
            pinger = NULL;
            DPN_APPLICATION_DESC desc;
            Dp_Client_FillAppDesc(&desc);              // slot +0xD0
            pinger = new netDP_PINGER;                 // 0x7C bytes, ctor 0x00595340
            Dp_Client_BuildPingQuery(&query);          // slot +0xAC: {0xFFFF, 6, 0}
            pinger->Init(&desc, &query);               // 0x00595440: CoCreate DirectPlay8Client + address
            pingServerId = -1;
            apFree(query.data);
            break;
        }
        if (ubiClient->IsConnectError())               // 0x005979F0
            Browser_Stop(0);
        goto think_tail;

    case netACT_JOIN_DP_FIND:
    {
        if (client->hrError < 0)
        {
            Client_DisconnectFromGame();               // slot +0x9C
            Browser_Stop(0);                           // slot +0x80
            break;
        }
        if (!client->bEnumerating)                     // netDP_CLIENT +0x18
            client->EnumHosts();                       // 0x00599320  1.2: no argument (feb passed userName)
        arrVector<netDRV_SERVER_INFO>* list = LockServers();
        for (int i = 0; i < list->count; i++)
        {
            if (list->data[i].bNoPrune == 0)           // +0x70 source: 0 = found by DirectPlay
            {
                joinServerId = list->data[i].id;
                FinishAction();                        // -> netACT_JOIN_DP_CONNECT
                break;
            }
        }
        UnlockServers();
        goto think_tail;
    }

    case netACT_BRW_READY:
        bBrowserStarted = 1;
        Browser_RefreshServers();                      // slot +0x7C
        break;

    case netACT_JOIN_UBI_JOIN:
    {
        arrVector<netDRV_SERVER_INFO>* list = LockServers();
        int   idx      = FindServerIdx(joinServerId);  // 0x0058FC40
        void* ubiKey   = list->data[idx].driverData;   // +0x74
        bAcceptServers = 0;                            // 1.2: list changes are deferred during the call
        UnlockServers();                               // 1.2: unlocked BEFORE the call (feb held the lock)
        ubiClient->JoinGame(ubiKey);                   // 0x00597930
        bAcceptServers = 1;
        break;
    }

    case netACT_JOIN_UBI_WAIT:
        if (ubiClient->IsJoined())                     // 0x00597A10
            break;
        if (!ubiClient->IsJoinError())                 // 0x00597A40
            goto think_tail;
        Client_DisconnectFromGame();
        break;

    case netACT_JOIN_DP_CONNECT:
    {
        arrVector<netDRV_SERVER_INFO>* list = LockServers();
        int idx = FindServerIdx(joinServerId);
        netDP_SERVER_DATA* sd = (netDP_SERVER_DATA*)list->data[idx].driverData;
        bAcceptServers = 0;                            // 1.2
        UnlockServers();                               // 1.2: unlocked before the call
        client->JoinGame(sd);                          // 0x00599530  1.2: one argument (feb: sd, userName);
                                                       //   the connect data now comes from slot +0xD8
        bAcceptServers = 1;
        break;
    }

    case netACT_JOIN_DP_WAIT:
        if (client->bJoined)                           // netDP_CLIENT +0x0C
            break;
        if (client->hrError >= 0)
            goto think_tail;
        Client_DisconnectFromGame();
        break;

    case netACT_JOIN_READY:
        bClientJoined = 1;
        break;

    default:
        goto think_tail;
    }
    FinishAction();

think_tail:
    netDRV::Think();                                   // 0x0058F5C0: ProcessEvents

    if (serverReg)
    {
        if (!bMainDataPublished)                       // netDRV +0x74, cleared by Server_SetMainData / SetGamePort
            serverReg->UpdateGameInfo();               // 0x00596EC0
        serverReg->Think();                            // 0x00595DC0
    }
    if (ubiClient)
        ubiClient->Think();                            // 0x005975C0
    bMainDataPublished = 1;

    if (bServerStarted && (!server || !server->bStarted))
        bServerStarted = 0;

    if (bBrowserStarted)
    {
        if (bUbi)
        {
            if (!ubiClient || !ubiClient->IsConnected())
            {
                Browser_Stop(0);
                if (pinger)                            // 1.2: NEW (redundant: Browser_Stop already did it)
                    delete pinger;
                pinger = NULL;
            }
        }
        else if (!client || !client->bStarted)
            Browser_Stop(0);
    }

    if (bClientJoined)
    {
        if (!client || !client->bJoined)
            Client_DisconnectFromGame();
        if (bUbi && (!ubiClient || !ubiClient->IsJoined() || ubiClient->IsJoinError()))
            Client_DisconnectFromGame();
    }

    UpdateUbiPings();                                  // 1.2: NEW, 0x005934C0
}

// v12 0x00591B10  (feb 0x00577FC0)
// Post the state event for the finished action group and pop the next queued action.
void netDRV_DPUBI::FinishAction()
{
    // Event types 2/6/7 only bump gsMP_NETWORK timers (see network.h / mp_session.md):
    //   2 = server part done, 6 = browser part done, 7 = client part done.
    int posted = 0;
    if (action >= 1 && action <= 5)  { AddEvent(2, -1, NULL, 0); posted = 1; }
    if (action >= 6 && action <= 12) { AddEvent(6, -1, NULL, 0); posted = 1; }

    if ((action >= 13 && action <= 17) || action == 6 || action == 11)
        AddEvent(7, -1, NULL, 0);
    else if (!posted && action != 0)                   // 1.2: `action != 0` test added (feb posted 2,6,7 for action 0)
    {
        AddEvent(2, -1, NULL, 0);
        AddEvent(6, -1, NULL, 0);
        AddEvent(7, -1, NULL, 0);
    }

    action = netACT_NONE;
    if (actions.count != 0)
    {
        action = actions.data[0];
        memmove(actions.data, actions.data + 1, actions.count * 4 - 4);
        actions.count--;
    }
}

// ===========================================================================
// Drv_*
// ===========================================================================

// v12 0x00590B90  (no feb twin)
// 1.2: NEW slot +0x08. Once per frame: flush the coalesced send queues of the DirectPlay wrappers.
void netDRV_DPUBI::Drv_Flush()
{
    if (server) server->Flush();                       // 0x00598830 (packs records into <= 700-byte packets)
    if (client) client->Flush();                       // 0x00599760
    netDRV::Drv_Flush();                               // 0x00401380 (empty)
}

// v12 0x00590BC0  (feb 0x00577140)
// Remember the driver type; Ubi.com drivers also keep the game name / version for the lobby login.
void netDRV_DPUBI::Drv_Init(const netDRV_INIT* init)
{
    netDRV::Drv_Init(init);                            // 0x0058F870 (1.2: stores bDedicated, netVersion)
    bUbi = init->type;                                 // 1.2: +0x08 (feb +0x00).  0x00590BCF `8B 47 08`: the
                                                       //   documented 3-byte patch point to force LAN-only
    if (bUbi)
    {
        strncpy(gameName, init->gameName, 64);         gameName[63] = 0;
        strncpy(gameVersion, init->gameVersion, 64);   gameVersion[63] = 0;
    }
    else
    {
        userName[0] = 0;
        password[0] = 0;
        gameName[0] = 0;
        gameVersion[0] = 0;
    }
    joinServerId = -1;
    for (int i = 0; i < 2; i++) joinAddr[i][0] = 0;
    bInited = 1;
}

// v12 0x00590C60  (feb 0x005771E0)
int netDRV_DPUBI::Drv_IsInited()                       { return bInited; }

// v12 0x00590C70  (feb 0x005771F0)
// Tell the lobby that the joined game really started.
void netDRV_DPUBI::Drv_NotifyGameStarted()
{
    if (ubiClient && ubiClient->IsJoined())
        ubiClient->NotifyServerJoined();               // 0x00597C50
    netDRV::Drv_NotifyGameStarted();                   // 0x0058F8C0
}

// v12 0x00590CA0  (feb 0x00577220)
int netDRV_DPUBI::Drv_IsLadderSupported()              { return bUbi; }

// v12 0x00590B70  (feb 0x00577120)
// Browser auto-refresh interval in seconds (wrUI_JOIN_WINDOW counts it down, then RefreshServers).
float netDRV_DPUBI::Drv_GetRefreshInterval()
{
    if (bUbi)
        return 600.0f;                                 // 0x005FFFD4
    return 15.0f;                                      // 0x005FA490
}

// v12 0x00548C90  (no feb twin; shared one-instruction stub)
// 1.2: NEW slot +0xA8. The protocol version id (10020000) given to Drv_Create.
int netDRV_DPUBI::Drv_GetNetVersion()                  { return netVersion; }   // netDRV +0x14

// ===========================================================================
// Server
// ===========================================================================

// v12 0x00590CB0  (feb 0x00577230)
// Queue: [0] DP start, DP wait, (Ubi register, Ubi wait), ready.
void netDRV_DPUBI::Server_Start(const netDRV_SERVER_CREATE* sc)
{
    netDRV::Server_Start(sc);                          // 0x0058F8F0: name, max players, main data, port
    const char* user = *(const char**)((const char*)sc + 0x16);
    const char* pass = *(const char**)((const char*)sc + 0x1A);
    strncpy(userName, user, 64);  userName[63] = 0;
    strncpy(password, pass, 64);  password[63] = 0;
    bLadder = bUbi ? *(const int*)((const char*)sc + 0x12) : 0;

    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    players.Resize(0);                                 // 0x00593940
    ReleaseMutex(hPlayersMutex);

    actions.Add(netACT_NONE);
    actions.Add(netACT_SRV_DP_START);
    actions.Add(netACT_SRV_DP_WAIT);
    if (bUbi)
    {
        actions.Add(netACT_SRV_UBI_REGISTER);
        actions.Add(netACT_SRV_UBI_WAIT);
    }
    actions.Add(netACT_SRV_READY);
}

// v12 0x00590EB0  (feb 0x00577430)
int netDRV_DPUBI::Server_IsStarted()                   { return bServerStarted; }

// v12 0x00590EC0  (feb 0x00577440)
int netDRV_DPUBI::Server_IsError()
{
    if (server && server->hrError < 0)
        return 1;
    if (bUbi && serverReg && serverReg->IsError())
        return 1;
    return 0;
}

// v12 0x00590F00  (feb 0x00577480)
// Status line for wrUI_WAITSERVER_WND.
const char* netDRV_DPUBI::Server_GetStatus()
{
    if (server && server->hrError < 0)
        return server->GetErrorDesc();                 // 0x005986C0
    if (bUbi && serverReg && serverReg->IsError())
        return serverReg->GetErrorString();            // 0x00596A80
    if (server && !server->bStarted)
        return server->GetStatusDesc();                // 0x00598810
    if (bUbi && serverReg)
        return serverReg->GetStatusString();           // 0x00596160
    return s_InitializingServer;
}

// v12 0x00590F80  (no feb twin)
// 1.2: NEW slot +0x40. Round-trip latency to a client (index in the player table), -1 if unknown.
int netDRV_DPUBI::Server_GetClientPing(int clientIdx)
{
    if (!server)
        return -1;
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    int ping;
    if (clientIdx >= 0 && clientIdx < players.count)
        ping = server->GetPlayerPing(players.data[clientIdx].dpnid);  // 0x00598AC0
                                                       //   (IDirectPlay8Server::GetConnectionInfo, see network.h)
    else
        ping = -1;
    ReleaseMutex(hPlayersMutex);
    return ping;
}

// v12 0x00591000  (feb 0x00577500)
// Map the game client id (player-table index) to its DPNID and queue the data.
void netDRV_DPUBI::Server_SendToClient(int client, const void* d, int len, int flags)
{
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    DPNID to = (DPNID)client;                          // out of range: passed through unchanged as a DPNID
                                                       // ? (0 would be DPNID_ALL_PLAYERS_GROUP; whether the game uses it is not checked)
    if (client >= 0 && client < players.count)
        to = players.data[client].dpnid;
    ReleaseMutex(hPlayersMutex);
    server->Send(to, d, len, flags);                   // 0x00598900  1.2: enqueue; Drv_Flush sends
}

// v12 0x00591070  (feb 0x00577570)
// Leave the lobby, close the DirectPlay host, cancel the queued server actions.
void netDRV_DPUBI::Server_Stop()
{
    if (serverReg)
    {
        serverReg->Shutdown();
        delete serverReg;
        serverReg = NULL;
    }
    if (server)
    {
        server->StopServer();                          // 0x00598A70
        server->Shutdown();                            // 0x005983C0 (Close + Sleep(2000), mp_transport.md)
        delete server;
        server = NULL;
    }
    if (action >= 1 && action <= 5)
        action = netACT_NONE;

    int n = 0;                                         // drop queued actions up to the next group separator
    while (n < actions.count && actions.data[n] != netACT_NONE)
        n++;
    if (n)
    {
        memmove(actions.data, actions.data + n, (actions.count - n) * 4);
        actions.count -= n;
    }
    ProcessEvents(1);
    bGameStarted   = 0;                                // netDRV +0x8A
    bServerStarted = 0;
}

// v12 0x00591160  (feb 0x00577660)
void netDRV_DPUBI::Server_StopLadder()
{
    if (serverReg) serverReg->StopLadder();            // 0x00597050  /* Ubi.com service */
}

// v12 0x00591170  (feb 0x00577670)
void netDRV_DPUBI::Server_ResetLadder()
{
    if (serverReg && bLadder) serverReg->RestartLadder();  // 0x00597020  /* Ubi.com service */
}

// v12 0x00591190  (feb 0x00577690)
void netDRV_DPUBI::Server_StartMatchData()
{
    if (serverReg) serverReg->StartMatchData();        // 0x00597080  /* Ubi.com service */
}

// v12 0x005911A0  (feb 0x005776A0)
void netDRV_DPUBI::Server_SetMatchData(const char* key, int a, int b)
{
    if (serverReg) serverReg->SendMatchData(key, a, b);    // 0x005970D0  /* Ubi.com service */
}

// v12 0x005911C0  (feb 0x005776C0)
void netDRV_DPUBI::Server_FinishMatchData()
{
    if (serverReg) serverReg->FinishMatchData();       // 0x00597110  /* Ubi.com service */
}

// v12 0x005911D0  (feb 0x005776D0)
int netDRV_DPUBI::Server_GetMatchStatus()
{
    if (serverReg) return serverReg->GetMatchStatus(); // 0x00596F90  /* Ubi.com service */
    return 0;
}

// ===========================================================================
// Browser
// ===========================================================================

// v12 0x005911F0  (feb 0x005776F0)
// Queue: [0] clear list, (DP browser start+wait | Ubi connect+wait), ready (+ first refresh).
void netDRV_DPUBI::Browser_Start(const netDRV_BROWSER_CREATE* bc)
{
    netDRV::Browser_Start(bc);                         // 0x0051AAF0 (shared empty stub)
    strncpy(userName, bc->userName, 64);  userName[63] = 0;
    strncpy(password, bc->password, 64);  password[63] = 0;
    for (int i = 0; i < 2; i++) joinAddr[i][0] = 0;    // empty host list -> DirectPlay broadcast enumeration
    joinServerId = -1;

    actions.Add(netACT_NONE);
    actions.Add(netACT_BRW_CLEAR);
    if (bUbi)
    {
        actions.Add(netACT_BRW_UBI_CONNECT);
        actions.Add(netACT_BRW_UBI_WAIT);
    }
    else
    {
        actions.Add(netACT_BRW_DP_START);
        actions.Add(netACT_BRW_DP_WAIT);
    }
    actions.Add(netACT_BRW_READY);
}

// v12 0x005913E0  (feb 0x005778E0)
int netDRV_DPUBI::Browser_IsStarted()                  { return bBrowserStarted; }

// v12 0x005913F0  (feb 0x005778F0)
int netDRV_DPUBI::Browser_IsError()
{
    if (client && client->hrError < 0)
        return 1;
    if (bUbi && ubiClient && ubiClient->IsConnectError())
        return 1;
    return 0;
}

// v12 0x00591430  (feb 0x00577930)
const char* netDRV_DPUBI::Browser_GetStatus()
{
    if (ubiClient && ubiClient->IsConnectError())
        return ubiClient->GetConnectErrorString();     // 0x00597A60
    if (client && client->hrError < 0)
        return client->GetErrorDesc();                 // 0x005995E0
    if (ubiClient && !ubiClient->IsConnected())
        return ubiClient->GetStatusString();           // 0x00597AA0
    if (client && !client->bStarted)
        return client->GetStatusDesc();                // 0x00599730
    return s_BrowserStarted;
}

// v12 0x005914B0  (feb 0x005779B0)
// Re-query the server list (lobby request or a new DirectPlay EnumHosts round).
void netDRV_DPUBI::Browser_RefreshServers()
{
    if (bUbi)
        ubiClient->EnumServers();                      // 0x005978B0
    else
        client->EnumHosts();                           // 0x00599320  1.2: no host argument (feb: userName ptr)
}

// v12 0x005914D0  (feb 0x005779E0)
// Stop browsing; bKeepList = 1 when going on to join (keeps the list and the netDP_CLIENT).
void netDRV_DPUBI::Browser_Stop(int bKeepList)
{
    if (ubiClient) ubiClient->StopBrowsing(bKeepList); // 0x00597B90
    if (client)    client->StopBrowsing(bKeepList);    // 0x005994A0
    if (pinger)                                        // 1.2: NEW
        delete pinger;
    pinger = NULL;

    if (!bKeepList)
    {
        ClearServersList();
        if (client)
        {
            client->Shutdown();
            delete client;
            client = NULL;
        }
        if (ubiClient)
        {
            ubiClient->Shutdown();
            delete ubiClient;
            ubiClient = NULL;
        }
    }
    if (action >= 6 && action <= 12)
        action = netACT_NONE;

    int n = 0;
    while (n < actions.count && actions.data[n] != netACT_NONE)
        n++;
    if (n)
    {
        memmove(actions.data, actions.data + n, (actions.count - n) * 4);
        actions.count -= n;
    }
    bBrowserStarted = 0;
}

// v12 0x00591A60  (feb 0x00577F10)
// Same server? Only entries of the same source are compared, by the owning transport.
int netDRV_DPUBI::IsServersEqual(const netDRV_SERVER_INFO* a, const netDRV_SERVER_INFO* b)
{
    int src = a->bNoPrune;                             // +0x70 source (feb +0x6C): 0 DirectPlay, 1 Ubi.com
    if (src != b->bNoPrune)
        return 0;
    if (src == 0)
        return client->IsServersEqual((netDP_SERVER_DATA*)a->driverData,
                                      (netDP_SERVER_DATA*)b->driverData);   // 0x005999B0 (tail jump)
    if (src == 1)
        return ubiClient->IsServersEqual(a->driverData, b->driverData);     // 0x00597B30
    return 1;
}

// v12 0x00591AC0  (feb 0x00577F70)
// Free the transport-private data of a list entry.
void netDRV_DPUBI::OnServerDelete(netDRV_SERVER_INFO* s)
{
    if (s->bNoPrune == 0)
    {
        client->OnServerDelete((netDP_SERVER_DATA*)s->driverData);  // 0x00599A00
        bServerListUpdated = 1;                        // netDRV +0x86
        return;
    }
    if (s->bNoPrune == 1)
        ubiClient->OnServerDelete(s->driverData);      // 0x00597B80
}

// v12 0x005921F0  (no feb twin; replaces the bodies of feb Dp_Client_OnServerEnumEnd 0x00578C40
//                  and Ubi_Client_OnServerEnumEnd 0x00579190)
// 1.2: NEW. Remove entries of `source` not seen in the current enumeration generation.
void netDRV_DPUBI::PruneServers(int source)
{
    arrVector<netDRV_SERVER_INFO>* list = LockServers();
    for (int i = 0; i < list->count; i++)
    {
        netDRV_SERVER_INFO* e = &list->data[i];
        int gen = enumGeneration;                      // netDRV +0x7E
        if (e->enumGeneration == gen)
            continue;
        if (e->bNoPrune == source)
        {
            OnServerDelete(e);                         // slot +0xA4
            apFree(list->data[i].data2.data);          // inlined ~netDRV_SERVER_INFO
            apFree(list->data[i].mainData.data);
            memmove(&list->data[i], &list->data[i + 1], (list->count - (i + 1)) * sizeof(netDRV_SERVER_INFO));
            list->count--;
            i--;
            bServerListUpdated = 1;
        }
        else
            e->enumGeneration = gen;                   // other source: keep, mark as current
    }
    UnlockServers();
}

// v12 0x005934C0  (no feb twin)
// 1.2: NEW. Ubi.com browser only: read the finished ping, then hand every unpinged Ubi.com
// entry (ping <= 0) to the netDP_PINGER, which EnumHosts it with the 0xFFFF probe.
void netDRV_DPUBI::UpdateUbiPings()
{
    if (!pinger || !ubiClient || !pinger->IsIdle())   // 0x00595650: busy flag +0x6C == 0
        return;

    arrVector<netDRV_SERVER_INFO>* list = LockServers();
    int idx = FindServerIdx(pingServerId);
    if (idx != -1)
    {
        int ping = pinger->GetPing();                  // 0x00595660: +0x64 (9999 until an answer arrives)
        if (ping <= 0)
            ping = NETDP_PING_UNKNOWN;                 // 9999
        if (list->data[idx].ping != ping)
        {
            list->data[idx].ping = ping;
            bServerListUpdated = 1;
        }
    }
    UnlockServers();
    pingServerId = -1;

    // ? no break: every candidate is handed to the pinger in turn.  StartPing cancels the
    //   previous enumeration (CancelAsyncOperation(0, DPNCANCEL_ENUM)), so per round only the LAST
    //   candidate is effectively measured; the others are retried on the next idle frame.
    for (int i = 0; i < list->count; i++)
    {
        netDRV_SERVER_INFO* e = &list->data[i];
        if (e->ping > 0 || e->bNoPrune != 1)
            continue;
        pingServerId = e->id;
        char  addr[0x80], altAddr[0x80];              // ? sizes: stack frame 0x104 holds both
        unsigned short port;
        ubiClient->GetServerAddress(e->driverData, addr, altAddr, &port);   // 0x00597B00 (name ours)
        pinger->StartPing(addr, altAddr, port);        // 0x00595550
    }
    // ? the list is read here after UnlockServers (as in the binary).
}

// ===========================================================================
// Client
// ===========================================================================

// v12 0x005915F0  (feb 0x00577AE0)
// Join a browsed server (j->serverId) or an address (j->serverId == -1, j->addr "ip:port").
// Queue: [0] (Ubi: lobby join, wait, then DP find) | (by address: clear, DP browse, DP find),
//        DP connect, DP wait, clear list, joined.
void netDRV_DPUBI::Client_JoinGame(const netDRV_CLIENT_JOIN* j)
{
    strncpy(userName, j->user, 64);  userName[63] = 0;
    strncpy(password, j->pass, 64);  password[63] = 0;
    netDRV::Client_JoinGame(j);                        // 0x0058FB40
    for (int i = 0; i < 2; i++) joinAddr[i][0] = 0;
    if (j->addr)
    {
        strncpy(joinAddr[0], j->addr, 64);
        joinAddr[0][63] = 0;
    }
    joinServerId = j->serverId;

    if (client)
        client->DisconnectFromGame();                  // 0x00599970
    if (pinger)                                        // 1.2: NEW
        delete pinger;
    pinger = NULL;

    actions.Add(netACT_NONE);
    if (bUbi)
    {
        actions.Add(netACT_JOIN_UBI_JOIN);
        actions.Add(netACT_JOIN_UBI_WAIT);
        actions.Add(netACT_BRW_CLEAR);
        actions.Add(netACT_BRW_DP_START);              // browses joinAddr[] filled by the lobby
        actions.Add(netACT_BRW_DP_WAIT);
        actions.Add(netACT_JOIN_DP_FIND);
        actions.Add(netACT_JOIN_DP_CONNECT);
    }
    else
    {
        if (joinServerId == -1)                        // by address: enumerate that host first
        {
            actions.Add(netACT_BRW_CLEAR);
            actions.Add(netACT_BRW_DP_START);
            actions.Add(netACT_BRW_DP_WAIT);
            actions.Add(netACT_JOIN_DP_FIND);
        }
        actions.Add(netACT_JOIN_DP_CONNECT);
    }
    actions.Add(netACT_JOIN_DP_WAIT);
    actions.Add(netACT_BRW_CLEAR);
    actions.Add(netACT_JOIN_READY);
}

// v12 0x005918B0  (feb 0x00577D80)
int netDRV_DPUBI::Client_IsJoined()                    { return bClientJoined; }

// v12 0x005918C0  (feb 0x00577D90)
int netDRV_DPUBI::Client_IsError()
{
    if (client && client->hrError < 0)
        return 1;
    if (ubiClient && ubiClient->IsJoinError())
        return 1;
    return 0;
}

// v12 0x00591900  (feb 0x00577DD0)
const char* netDRV_DPUBI::Client_GetStatus()
{
    if (ubiClient && ubiClient->IsJoinError())
        return ubiClient->GetJoinErrorString();        // 0x00597A80
    if (client && client->hrError < 0)
        return client->GetErrorDesc();                 // 0x005995E0 (shows the reply code, e.g. 0x8015A001)
    if (ubiClient && !ubiClient->IsJoined())
        return ubiClient->GetStatusString();
    if (client)
        return client->GetStatusDesc();
    return "";                                         // 0x00640358
}

// v12 0x00591970  (no feb twin)
// 1.2: NEW slot +0x94. Round-trip latency to the server, -1 when not joined.
int netDRV_DPUBI::Client_GetPing()
{
    if (client && bClientJoined)
        return client->GetPing();                      // 0x00599A60 (tail jump)
    return -1;
}

// v12 0x00591990  (feb 0x00577E40)
void netDRV_DPUBI::Client_SendToServer(const void* d, int len, int flags)
{
    client->Send(d, len, flags);                       // 0x00599830 (tail jump)  1.2: enqueue; Drv_Flush sends
}

// v12 0x005919A0  (feb 0x00577E50)
void netDRV_DPUBI::Client_DisconnectFromGame()
{
    if (client)    client->DisconnectFromGame();       // 0x00599970
    if (ubiClient) ubiClient->DisconnectFromGame();    // 0x00597C10
    if ((action >= 13 && action <= 17) || action == 6 || action == 11)
        action = netACT_NONE;

    int n = 0;
    while (n < actions.count && actions.data[n] != netACT_NONE)
        n++;
    if (n)
    {
        memmove(actions.data, actions.data + n, (actions.count - n) * 4);
        actions.count -= n;
    }
    ProcessEvents(1);
    bGameStarted  = 0;                                 // netDRV +0x8A
    bClientJoined = 0;
}

// ===========================================================================
// Player table (server side).  Client id seen by the game = index in `players`.
// ===========================================================================

// v12 0x00591C00  (feb 0x005780A0)
// Is `name` already used? -2 = it is the host's own name, index = in use, -1 = free.
int netDRV_DPUBI::FindDPlayPlayerPre(const char* name)
{
    if (!bDedicated)                                   // 1.2: a dedicated host has no player of its own
    {                                                  //      (netDRV +0x96); feb always compared
        if (strcmp(userName, name) == 0)
            return -2;
    }
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    int i;
    for (i = 0; i < players.count; i++)
    {
        if ((players.data[i].flags & (netDRV_DPUBI_PLAYER::PF_DP_PENDING | netDRV_DPUBI_PLAYER::PF_DP_CONNECTED))
            && strcmp(players.data[i].name, name) == 0)
            break;
    }
    if (i >= players.count)
    {
        ReleaseMutex(hPlayersMutex);
        return -1;
    }
    ReleaseMutex(hPlayersMutex);
    return i;
}

// v12 0x00591D10  (feb 0x005781A0)
// Reserve a slot for a connecting DirectPlay player.  LAN: first free slot (or a pending one with the
// same name).  Ubi.com: only a name the lobby registered (PF_UBI) is accepted.  Returns index or -1.
int netDRV_DPUBI::AddDPlayPlayer_Pre(const char* name)
{
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    if (bUbi)
    {
        WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);   // taken twice (recursive), as in feb
        for (int i = 0; i < players.count; i++)
        {
            if ((players.data[i].flags & netDRV_DPUBI_PLAYER::PF_UBI) && strcmp(players.data[i].name, name) == 0)
            {
                ReleaseMutex(hPlayersMutex);
                if (i == -1) { ReleaseMutex(hPlayersMutex); return -1; }       // (dead test, kept from the source)
                netDRV_DPUBI_PLAYER* p = &players.data[i];
                if (p->flags & netDRV_DPUBI_PLAYER::PF_DP_CONNECTED)
                {
                    ReleaseMutex(hPlayersMutex);
                    return -1;
                }
                p->flags |= netDRV_DPUBI_PLAYER::PF_DP_PENDING;
                ReleaseMutex(hPlayersMutex);
                return i;
            }
        }
        ReleaseMutex(hPlayersMutex);
        ReleaseMutex(hPlayersMutex);
        return -1;                                     // not announced by the lobby
    }

    int i;
    for (i = 0; i < players.count; i++)
    {
        unsigned f = players.data[i].flags;
        if (f == 0)
            break;                                     // free slot
        if (f == netDRV_DPUBI_PLAYER::PF_DP_PENDING && strcmp(name, players.data[i].name) == 0)
            break;                                     // retry of a pending connect
    }
    if (i == players.count)
    {
        netDRV_DPUBI_PLAYER blank = { 0 };
        players.Insert(players.count, blank, 1);       // 0x005939A0
    }
    players.data[i].flags |= netDRV_DPUBI_PLAYER::PF_DP_PENDING;
    strncpy(players.data[i].name, name, 0x24);
    players.data[i].name[0x23] = 0;
    ReleaseMutex(hPlayersMutex);
    return i;
}

// v12 0x00591F30  (feb 0x005783C0)
// DirectPlay created the player: bind its DPNID to the reserved slot and post netEV_CLIENT_CONNECTED.
int netDRV_DPUBI::AddDPlayPlayer_Fin(DPNID dpnid, int idx)
{
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    if (idx >= 0 && idx < players.count)
    {
        unsigned f = players.data[idx].flags;
        if ((f & netDRV_DPUBI_PLAYER::PF_DP_PENDING) && !(f & netDRV_DPUBI_PLAYER::PF_DP_CONNECTED))
        {
            players.data[idx].flags = f | netDRV_DPUBI_PLAYER::PF_DP_CONNECTED;
            players.data[idx].dpnid = dpnid;
            ReleaseMutex(hPlayersMutex);
            AddEvent(netEV_CLIENT_CONNECTED, idx, NULL, 0);    // -> gsMP_SERVER::AddClient
            return idx;
        }
    }
    ReleaseMutex(hPlayersMutex);
    server->DisconnectPlayer(dpnid);                   // 0x00598A40: no reservation -> kick
    return -1;
}

// v12 0x00591FD0  (feb 0x00578460)
// Lobby announced a player (Ubi.com host): reserve a slot flagged PF_UBI.
int netDRV_DPUBI::AddUbiPlayer(const char* name)
{
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    int i;
    for (i = 0; i < players.count; i++)
        if (players.data[i].flags == 0)
            break;
    if (i == players.count)
    {
        netDRV_DPUBI_PLAYER blank = { 0 };
        players.Insert(players.count, blank, 1);
    }
    players.data[i].flags |= netDRV_DPUBI_PLAYER::PF_UBI;
    strncpy(players.data[i].name, name, 0x24);
    players.data[i].name[0x23] = 0;
    ReleaseMutex(hPlayersMutex);
    return i;
}

// v12 0x00592080  (feb 0x00578510)
// Free a player slot: tell the lobby, kick the DirectPlay player, post netEV_CLIENT_LEFT.
void netDRV_DPUBI::RemovePlayer(int idx)
{
    if (idx == -1)
        return;
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    if (idx < 0 || idx >= players.count)
    {
        ReleaseMutex(hPlayersMutex);
        return;
    }
    int bWasConnected = 0;
    netDRV_DPUBI_PLAYER* p = &players.data[idx];
    unsigned oldFlags = p->flags;
    p->flags = 0;                                      // 1.2: cleared up front (feb cleared bit by bit)

    if ((oldFlags & netDRV_DPUBI_PLAYER::PF_UBI) && serverReg)
    {
        char name[256];
        strncpy(name, p->name, 0x100);
        name[255] = 0;
        ReleaseMutex(hPlayersMutex);
        serverReg->NotifyClientLeave(name);            // 0x00596A60  /* Ubi.com service */
        WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
        p = &players.data[idx];
    }
    if (oldFlags & netDRV_DPUBI_PLAYER::PF_DP_CONNECTED)
    {
        if (server)
        {
            DPNID id = p->dpnid;
            ReleaseMutex(hPlayersMutex);
            server->DisconnectPlayer(id);              // 0x00598A40
            WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
        }
        bWasConnected = 1;
    }
    ReleaseMutex(hPlayersMutex);

    if (oldFlags)                                      // 1.2: only for a slot that was in use
    {
        EraseClientEvents(idx);                        // 0x0058FF10
        if (bWasConnected)
            AddEvent(netEV_CLIENT_LEFT, idx, NULL, 0); // -> gsMP_SERVER::RemoveClient
    }
}

// ===========================================================================
// DirectPlay server callbacks (called from netDP_SERVER::MsgHandler, DirectPlay worker threads)
// ===========================================================================

// v12 0x005923D0  (feb 0x00578DE0)
// Port to Host() on (also Ubi_Server_GetHostPort, slot +0x108).
unsigned short netDRV_DPUBI::Dp_Server_GetGamePort()    { return gamePort; }   // netDRV +0x7C, default 7777
unsigned short netDRV_DPUBI::Ubi_Server_GetHostPort()   { return gamePort; }   // same function

// v12 0x005923E0  (no feb twin)
// 1.2: NEW slot +0xB4. netDP_SERVER reports the port it really bound (after the +1..+10 retry);
// the main data is marked dirty so the lobby gets the new port.
void netDRV_DPUBI::Dp_Server_SetGamePort(unsigned short port)
{
    bMainDataPublished = 0;                            // netDRV +0x74
    gamePort = port;
}

// v12 0x00592400  (feb 0x00578690)
// Fill the DPN_APPLICATION_DESC used to Host().
void netDRV_DPUBI::Dp_Server_FillAppDesc(DPN_APPLICATION_DESC* desc)
{
    memset(desc, 0, sizeof(*desc));                    // 0x12 dwords
    netDP_CLIENT::CharToWChar(serverName, (WCHAR*)apBuffer, 0x800);  // 0x00599DA0 (fastcall src, dst), session name
    desc->dwSize          = sizeof(DPN_APPLICATION_DESC);    // 0x48
    desc->dwFlags         = DPNSESSION_CLIENT_SERVER;        // 1
    desc->guidApplication = netDPUBI_AppGuid;                // 0x0060A688
    desc->dwMaxPlayers    = maxPlayers;                      // netDRV +0x58 (no +1 for the dedicated host)
    desc->pwszSessionName = (WCHAR*)apBuffer;                // 0x00654160
}

// v12 0x00592470  (feb 0x00578700)
// ENUM_HOSTS_QUERY: decide whether to answer and build the response data.
//   query  : {u16 type, u16 hdrLen, u16 nameLen, ...}  name at q+hdrLen (Dp_Client_BuildEnumQuery)
//   reply  : {u16 1, u16 10, u32 netVersion, u16 mainDataLen} + main data (mode, map ... see gsMP_SERVER)
// 1.2: rewritten.  feb took the name as a C string and returned pointers to the main data;
// 1.2 parses a binary query, answers the 0xFFFF ping probe, and prefixes the version id.
int netDRV_DPUBI::Dp_Server_OnQuery(const void* q, int qLen, arrVector<unsigned char>* reply)
{
    const unsigned char* b = (const unsigned char*)q;
    if (qLen < 4)
        return 0;                                      // no answer
    unsigned short type = *(const unsigned short*)b;
    if (type == NETDP_BLOB_PING)
        return 1;                                      // 1.2: ping probe -> empty answer (latency only)
    unsigned hdrLen = *(const unsigned short*)(b + 2);
    if (qLen < (int)hdrLen + 1)
        return 0;
    const char* name = s_InvalidName;
    if (type != NETDP_BLOB_GAME)
        return 0;

    if (bUbi)
    {
        name = (const char*)b + hdrLen;
        unsigned short nameLen = *(const unsigned short*)(b + 4);
        if (nameLen == 0 || name[nameLen - 1] != 0)
            name = s_InvalidName;
        // Ubi.com host answers only players the lobby announced (PF_UBI) -- invisible to LAN scans.
        WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
        int i;
        for (i = 0; i < players.count; i++)
            if ((players.data[i].flags & netDRV_DPUBI_PLAYER::PF_UBI) && strcmp(players.data[i].name, name) == 0)
                break;
        if (i >= players.count)
        {
            ReleaseMutex(hPlayersMutex);
            return 0;
        }
        ReleaseMutex(hPlayersMutex);
        if (i == -1)
            return 0;                                  // (dead test)
    }

#pragma pack(push, 1)
    struct { unsigned short type, hdrLen; int version; unsigned short dataLen; } hdr;
#pragma pack(pop)
    hdr.type    = NETDP_BLOB_GAME;
    hdr.hdrLen  = 10;
    hdr.version = netVersion;                          // netDRV +0x14 = 10020000
    hdr.dataLen = (unsigned short)mainData.count;      // netDRV +0x60
    AppendBytes(reply, &hdr, 10);
    AppendBytes(reply, mainData.data, mainData.count); // netDRV +0x5C
    return 1;
}

// v12 0x005926F0  (feb 0x00578800)
// INDICATE_CONNECT: accept or reject a joining client and write the 8-byte reply {u16 1, u16 8, u32 code}.
//   connect data: {u16 type=1, u16 hdrLen, u32 versionId, u16 nameLen} + name at data+hdrLen
//   (built by Dp_Client_BuildConnectData).  Returns 1 = accept, 0 = reject.
// 1.2: rewritten (feb took a C string name, had no version check and no reply data).
int netDRV_DPUBI::Dp_Server_OnConnectionAttempt(const void* d, int len, int* pPlayerCtx,
                                                arrVector<unsigned char>* reply)
{
#pragma pack(push, 1)
    struct { unsigned short type, len; unsigned int code; } rep = { 1, 8, NETDP_CONNECT_OK };
#pragma pack(pop)
    const unsigned char* b = (const unsigned char*)d;

    if (len < 4)
        goto reject;
    {
        unsigned hdrLen = *(const unsigned short*)(b + 2);
        if (len < (int)hdrLen + 1 || *(const unsigned short*)b != NETDP_BLOB_GAME)
            goto reject;                               // malformed: rejected with code 0x8015A000

        int version = *(const int*)(b + 4);
        const char* name = (const char*)b + hdrLen;
        unsigned short nameLen = *(const unsigned short*)(b + 8);
        if (nameLen == 0 || name[nameLen - 1] != 0)
            name = s_InvalidName;

        if (version != netVersion)                     // 10020000
        {
            rep.code = NETDP_CONNECT_BAD_VERSION;      // 0x8015A001 -> client "UI_ERR_SERVER_VERSION"-like text
            goto reject;
        }
        if (FindDPlayPlayerPre(name) == -1)            // free (not -2 host name, not an index in use)
        {
            *pPlayerCtx = AddDPlayPlayer_Pre(name);    // becomes the DirectPlay player context
            if (*pPlayerCtx != -1)
            {
                AppendBytes(reply, &rep, 8);           // code 0x8015A000
                return 1;
            }
        }
        rep.code = NETDP_CONNECT_REFUSED;              // 0x80158560 (DPNERR_SESSIONFULL)
    }
reject:
    AppendBytes(reply, &rep, 8);
    return 0;
}

// v12 0x005928B0  (feb 0x00578840)
// CREATE_PLAYER: finish the reservation made in OnConnectionAttempt.
void netDRV_DPUBI::Dp_Server_OnPlayerConnected(DPNID dpnid, int playerCtx)
{
    AddDPlayPlayer_Fin(dpnid, playerCtx);
    if (bUbi && serverReg)
        serverReg->UpdateGameInfo();                   // 1.2: NEW -- lobby sees the new player count
}

// v12 0x005928F0  (feb 0x00578850)
// DESTROY_PLAYER: free the slot of that DPNID.
void netDRV_DPUBI::Dp_Server_OnPlayerDisconnected(DPNID dpnid)
{
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    int i;
    for (i = 0; i < players.count; i++)
        if ((players.data[i].flags & netDRV_DPUBI_PLAYER::PF_DP_CONNECTED) && players.data[i].dpnid == dpnid)
            break;
    if (i >= players.count)
        i = -1;
    ReleaseMutex(hPlayersMutex);
    RemovePlayer(i);
    if (bUbi && serverReg)
        serverReg->UpdateGameInfo();                   // 1.2: NEW
}

// v12 0x00592980  (feb 0x005788D0)
// RECEIVE (one de-coalesced record): post netEV_DATA_TO_SERVER for the sender's client index.
void netDRV_DPUBI::Dp_Server_OnDataReceived(DPNID dpnid, void* data, int len)
{
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    int i;
    for (i = 0; i < players.count; i++)
        if ((players.data[i].flags & netDRV_DPUBI_PLAYER::PF_DP_CONNECTED) && players.data[i].dpnid == dpnid)
            break;
    if (i < players.count)
    {
        ReleaseMutex(hPlayersMutex);
        if (i != -1)
            AddEvent(netEV_DATA_TO_SERVER, i, data, len);  // -> gsMP_SERVER::ReceiveMsg
        return;
    }
    ReleaseMutex(hPlayersMutex);                       // unknown sender: dropped
}

// ===========================================================================
// DirectPlay client callbacks (netDP_CLIENT / netDP_PINGER)
// ===========================================================================

// v12 0x00592A00  (feb 0x00578950)
// DPN_APPLICATION_DESC for EnumHosts / Connect: only the application GUID matters.
void netDRV_DPUBI::Dp_Client_FillAppDesc(DPN_APPLICATION_DESC* desc)
{
    memset(desc, 0, sizeof(*desc));
    desc->dwSize          = sizeof(DPN_APPLICATION_DESC);    // 0x48
    desc->dwFlags         = 0;
    desc->guidApplication = netDPUBI_AppGuid;
}

// v12 0x00592320  (no feb twin)
// 1.2: NEW slot +0xAC. Enum payload of the Ubi.com pinger: {u16 0xFFFF, u16 6, u16 0}.
void netDRV_DPUBI::Dp_Client_BuildPingQuery(arrVector<unsigned char>* out)
{
    unsigned short q[3] = { NETDP_BLOB_PING, 6, 0 };
    AppendBytes(out, q, 6);
}

// v12 0x00592A50  (no feb twin)
// 1.2: NEW slot +0xD4. Enum query: {u16 1, u16 6, u16 nameLen} + userName incl. NUL.
// (A Ubi.com host uses the name to answer only lobby-announced players.)
void netDRV_DPUBI::Dp_Client_BuildEnumQuery(arrVector<unsigned char>* out)
{
    unsigned short nameLen = (unsigned short)(strlen(userName) + 1);
    unsigned short q[3] = { NETDP_BLOB_GAME, 6, nameLen };
    AppendBytes(out, q, 6);
    AppendBytes(out, userName, nameLen);
}

// v12 0x00592BB0  (no feb twin)
// 1.2: NEW slot +0xD8. Connect data (netDP_CLIENT::JoinGame 0x0059957B):
// {u16 1, u16 10, u32 netVersion, u16 nameLen} + userName incl. NUL.
void netDRV_DPUBI::Dp_Client_BuildConnectData(arrVector<unsigned char>* out)
{
#pragma pack(push, 1)
    struct { unsigned short type, hdrLen; int version; unsigned short nameLen; } h;
#pragma pack(pop)
    h.type    = NETDP_BLOB_GAME;
    h.hdrLen  = 10;
    h.version = netVersion;                            // 10020000
    h.nameLen = (unsigned short)(strlen(userName) + 1);
    AppendBytes(out, &h, 10);
    AppendBytes(out, userName, h.nameLen);
}

// v12 0x00592D10  (no feb twin)
// 1.2: NEW slot +0xDC. CONNECT_COMPLETE (netDP_CLIENT::MsgHandler 0x00599AEC): on failure, replace
// DirectPlay's HRESULT by the server's reply code (0x8015A001 version / 0x80158560 refused ...).
HRESULT netDRV_DPUBI::Dp_Client_GetConnectResult(HRESULT hr, const void* reply, int replyLen)
{
    if (hr >= 0)
        return hr;
    const unsigned char* b = (const unsigned char*)reply;
    if (replyLen < 4)
        return hr;
    if (replyLen < *(const unsigned short*)(b + 2) + 1)
        return hr;
    if (*(const unsigned short*)b != NETDP_BLOB_GAME)
        return hr;
    return *(const HRESULT*)(b + 4);
}

// v12 0x00592D40  (feb 0x00578EE0)
// New enumeration round (also Ubi_Client_OnServerEnumBegin, slot +0x11C).
void netDRV_DPUBI::Dp_Client_OnServerEnumBegin()        { enumGeneration++; }   // netDRV +0x7E
void netDRV_DPUBI::Ubi_Client_OnServerEnumBegin()       { enumGeneration++; }   // same function

// v12 0x00592D50  (feb 0x005789A0)
// ENUM_HOSTS_RESPONSE: add or update a DirectPlay server in the browser list.
// 1.2: takes the response blob {u16 1, u16 hdrLen, u32 version, u16 mainDataLen} + main data;
// stores the server's version id (UI sorts/greys by it), ignores replies while bAcceptServers == 0,
// and keeps ping >= 1.
void netDRV_DPUBI::Dp_Client_OnServerFound(netDP_NEW_SERVER* ns, const void* resp, int respLen)
{
    netDRV_SERVER_INFO info;                           // ctor 0x0058F2E0
    if (!bAcceptServers)                               // 1.2: netDRV +0x92 (cleared during a join)
        return;                                        // (temp info destroyed: 0x0058FF80)
    if (respLen < 4)
        return;
    const unsigned char* b = (const unsigned char*)resp;
    unsigned hdrLen = *(const unsigned short*)(b + 2);
    if (respLen < (int)hdrLen || *(const unsigned short*)b != NETDP_BLOB_GAME)
        return;

    int version          = *(const int*)(b + 4);
    unsigned mainDataLen = *(const unsigned short*)(b + 8);
    const unsigned char* md = b + hdrLen;

    strncpy(info.name, ns->name, 64);                  // netDP_NEW_SERVER +0x00 (session name)
    info.name[63]       = 0;
    info.maxPlayers     = ns->maxPlayers;              // +0x40
    info.curPlayers     = ns->curPlayers;              // +0x44
    info.ping           = ns->ping;                    // +0x48
    info.driverData     = ns->serverData;              // +0x4C netDP_SERVER_DATA* (address etc.)
    info.bNoPrune       = 0;                           // source: DirectPlay
    info.enumGeneration = enumGeneration;
    info.mainData.Resize(0);
    AppendBytes(&info.mainData, md, mainDataLen);      // 1.2: from the response (feb: ns +0x4C/+0x50)
    info.netVersion     = version;                     // 1.2: NEW (no filter here; UI greys mismatches)
    if (info.ping == 0)
        info.ping = 1;                                 // 1.2: 0 is reserved for "not measured"

    arrVector<netDRV_SERVER_INFO>* list = LockServers();
    int i;
    for (i = 0; i < list->count; i++)
        if (list->data[i].bNoPrune == 0 && IsServersEqual(&list->data[i], &info))   // slot +0xA0
            break;
    if (i < list->count)
    {
        netDRV_SERVER_INFO* e = &list->data[i];
        info.id = e->id;                               // keep the id the UI selected
        OnServerDelete(e);                             // slot +0xA4: free the old driver data
        e->id = info.id;
        memcpy(e->name, info.name, 64);
        e->netVersion = info.netVersion;
        e->maxPlayers = info.maxPlayers;
        e->curPlayers = info.curPlayers;
        e->ping       = info.ping;
        e->mainData   = info.mainData;                 // arrVector<u8>::operator= 0x005935D0
        e->data2      = info.data2;
        e->unk6C      = info.unk6C;
        e->bNoPrune   = info.bNoPrune;
        e->driverData = info.driverData;
        e->enumGeneration = info.enumGeneration;
    }
    else
    {
        info.id = NewServerId();                       // 0x0058FC70
        list->Insert(list->count, &info, 1);           // 0x00593760
    }
    bServerListUpdated = 1;
    UnlockServers();
    // info dtor: apFree(data2), apFree(mainData)
}

// v12 0x00593000  (feb 0x00578C40)
// Enumeration round over: drop DirectPlay entries that did not answer.
void netDRV_DPUBI::Dp_Client_OnServerEnumEnd()
{
    if (!bAcceptServers)
    {
        bDpEnumEnded = 1;                              // 1.2: deferred to Think (join in progress)
        return;
    }
    PruneServers(0);                                   // 1.2: shared body
}

// v12 0x00593020  (feb 0x00578D60)
// Data from the server: post netEV_DATA_TO_CLIENT (-> gsMP_CLIENT, vt+0x30).
void netDRV_DPUBI::Dp_Client_OnDataReceived(void* data, int len)
{
    AddEvent(netEV_DATA_TO_CLIENT, -1, data, len);
}

// ===========================================================================
// Ubi.com callbacks.  The SDK side is not reconstructed:
// /* Ubi.com service: not reconstructed (dead service / licensing) */
// ===========================================================================

// v12 0x00593060  (feb 0x00578DA0)
const char* netDRV_DPUBI::Ubi_GetUserName()            { return userName; }
// v12 0x00593070  (feb 0x00578DB0)
const char* netDRV_DPUBI::Ubi_GetPassword()            { return password; }
// v12 0x00593040  (feb 0x00578D80)
const char* netDRV_DPUBI::Ubi_GetGameName()            { return gameName; }
// v12 0x00593050  (feb 0x00578D90)
const char* netDRV_DPUBI::Ubi_GetVersion()             { return gameVersion; }
// v12 0x00593080  (feb 0x00578DC0)  lobby room name = server name
const char* netDRV_DPUBI::Ubi_Server_GetHostName()     { return serverName; }      // netDRV +0x18
// v12 0x00593090  (feb 0x00578DD0)
int netDRV_DPUBI::Ubi_Server_GetMaxPlayers()           { return maxPlayers; }      // netDRV +0x58

// v12 0x005930A0  (no feb twin)
// 1.2: NEW slot +0x10C. Current player count for the lobby game info (netUBI_SERVERREG::BuildGameInfo).
int netDRV_DPUBI::Ubi_Server_GetCurPlayers()
{
    if (server)
        return server->GetNumPlayers();                // 0x00598B00: IDirectPlay8Server::GetApplicationDesc
                                                       //   .dwCurrentPlayers (0 on failure)
    return 0;                                          // ? `xor al,al`: only AL is cleared (upper bytes of EAX
                                                       //   are stale) -- callers may read it as a byte
}

// v12 0x005930C0  (feb 0x00578DF0)
// Raw main data for the lobby blob.
void netDRV_DPUBI::Ubi_Server_GetGameInfo(void** data, int* len)
{
    *data = mainData.data;
    *len  = mainData.count;
}

// v12 0x005930E0  (feb 0x00578E10)
void netDRV_DPUBI::Ubi_Server_OnPlayerJoined(const char* name)
{
    AddUbiPlayer(name);                                // tail jump 0x00591FD0
}

// v12 0x005930F0  (feb 0x00578E20)
// Lobby says a player left: free its slot.
void netDRV_DPUBI::Ubi_Server_OnPlayerDisconnected(const char* name)
{
    WaitForSingleObject(hPlayersMutex, NETDPUBI_MUTEX_WAIT_MS);
    for (int i = 0; i < players.count; i++)
    {
        if ((players.data[i].flags & netDRV_DPUBI_PLAYER::PF_UBI) && strcmp(players.data[i].name, name) == 0)
        {
            ReleaseMutex(hPlayersMutex);
            if (i != -1)
                RemovePlayer(i);
            return;
        }
    }
    ReleaseMutex(hPlayersMutex);
}

// v12 0x005931B0  (feb 0x00578EF0)
// Lobby returned one game server: add/update a source-1 entry (ping measured later by the pinger).
void netDRV_DPUBI::Ubi_Client_OnServerFound(netUBI_NEW_SERVER* ns)
{
    netDRV_SERVER_INFO info;
    if (!bAcceptServers)                               // 1.2
        return;

    strncpy(info.name, ns->name, 64);
    info.name[63]       = 0;
    info.maxPlayers     = ns->maxPlayers;              // +0x40
    info.curPlayers     = ns->curPlayers;              // +0x44
    info.ping           = 0;                           // "not measured" -> UpdateUbiPings
    info.bNoPrune       = 1;                           // source: Ubi.com
    info.driverData     = ns->ubiKey;                  // +0x50
    info.enumGeneration = enumGeneration;
    info.mainData.Resize(0);
    AppendBytes(&info.mainData, ns->dataBegin, ns->dataEnd - ns->dataBegin);   // +0x48 / +0x4C
    info.netVersion     = ns->netVersion;              // 1.2: +0x54, from the lobby game-info blob

    arrVector<netDRV_SERVER_INFO>* list = LockServers();
    int i;
    for (i = 0; i < list->count; i++)
        if (list->data[i].bNoPrune == 1 && IsServersEqual(&list->data[i], &info))
            break;
    if (i < list->count)
    {
        netDRV_SERVER_INFO* e = &list->data[i];
        info.id   = e->id;
        info.ping = -e->ping;                          // 1.2: keep the old ping, negated = "stale, re-measure"
                                                       //   (UI sorts by |ping|; UpdateUbiPings re-pings <= 0)
        OnServerDelete(e);
        e->id = info.id;
        memcpy(e->name, info.name, 64);
        e->netVersion = info.netVersion;
        e->maxPlayers = info.maxPlayers;
        e->curPlayers = info.curPlayers;
        e->ping       = info.ping;
        e->mainData.Resize(0);
        e->mainData.Insert(0, info.mainData.data, info.mainData.data + info.mainData.count);  // 0x00593690
        e->data2      = info.data2;
        e->unk6C      = info.unk6C;
        e->bNoPrune   = info.bNoPrune;
        e->driverData = info.driverData;
        e->enumGeneration = info.enumGeneration;
    }
    else
    {
        info.id = NewServerId();
        list->Insert(list->count, &info, 1);
    }
    bServerListUpdated = 1;
    UnlockServers();
}

// v12 0x00593430  (feb 0x00579190)
void netDRV_DPUBI::Ubi_Client_OnServerEnumEnd()
{
    if (!bAcceptServers)
    {
        bUbiEnumEnded = 1;                             // 1.2: deferred to Think
        return;
    }
    PruneServers(1);
}

// v12 0x00593450  (feb 0x005792B0)
// Lobby join finished: remember "ip:port" / "altip:port" for the DirectPlay browse+connect that follows.
void netDRV_DPUBI::Ubi_Client_OnJoinServerAddress(const char* ip, unsigned short port,
                                                  const char* altIp, unsigned short altPort)
{
    for (int i = 0; i < 2; i++) joinAddr[i][0] = 0;
    if (action == netACT_JOIN_UBI_WAIT)                // only while waiting for it
    {
        sprintf(joinAddr[0], s_AddrFmt, ip, port);
        sprintf(joinAddr[1], s_AddrFmt, altIp, altPort);
    }
}

// ===========================================================================
// helpers
// ===========================================================================
static inline void AppendBytes(arrVector<unsigned char>* v, const void* p, int n)
{
    if (n == 0)
        return;
    int need = v->count + n;
    if (v->capacity < need)
    {
        int cap = v->capacity * 2;
        if (cap <= need) cap = need;
        if (cap > v->capacity)
        {
            v->data = (unsigned char*)apRealloc(v->data, cap);
            v->capacity = cap;
        }
    }
    memcpy(v->data + v->count, p, n);
    v->count += n;
}

// arrVector<unsigned char>::operator=        v12 0x005935D0 (feb 0x00579320)
// arrVector<unsigned char>::Insert(pos,b,e)  v12 0x00593690 (feb 0x005793E0)
// arrVector<unsigned char>::Reserve          v12 0x00593730
// arrVector<netDRV_SERVER_INFO>::Insert       v12 0x00593760 (copy-constructs 0x7C entries)
// arrVector<netDRV_DPUBI_PLAYER>::Resize      v12 0x00593940 (feb 0x00579480)
// arrVector<netDRV_DPUBI_PLAYER>::Insert      v12 0x005939A0 (feb 0x005794E0)
// (template instances emitted into this obj; not reconstructed)
