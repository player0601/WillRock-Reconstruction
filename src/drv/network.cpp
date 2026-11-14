// network.cpp -- netDRV base network driver (Will Rock 1.2, v12.exe; feb obj drv:network.obj)
//
// Reconstructed from the feb decompile and the v12 disassembly (v12 range 0x0058F2E0..0x005902BF).
// "// 1.2:" marks a difference from the Feb 2003 build. "// ?" marks what is not known.
// Almost everything here is unchanged except for the field offsets (+4 from the inserted
// netVersion, +0xC overall); the real 1.2 changes are the new fields and the three new virtuals.

#include "network.h"
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

#define NET_MUTEX_TIMEOUT 15000          // 0x3A98 ms, every WaitForSingleObject in this file

int        netPckProcessed;              // v12 0x009AF174
netDBGSTAT netDbgStat;                   // v12 0x009AF178

// ===========================================================================
// netDRV_SERVER_INFO
// ===========================================================================

// v12 0x0058F2E0  (feb 0x004A6C10)
// Default server entry.
netDRV_SERVER_INFO::netDRV_SERVER_INFO()
{
    // mainData / data2 arrVectors zeroed
    id             = -1;
    name[0]        = 0;
    // 1.2: netVersion (+0x44) is NOT initialised here; the driver fills it in OnServerFound
    maxPlayers     = -1;
    curPlayers     = -1;
    ping           = 0;                  // 1.2: was 9999 in feb
    unk6C          = 1;
    bNoPrune       = 0;
    driverData     = NULL;
    enumGeneration = -1;
}

// ===========================================================================
// netDRV
// ===========================================================================

// v12 0x0058F320  (feb 0x004A6C50)
// Constructor: zero the arrays, default port 7777, create the two mutexes, reset debug counters.
netDRV::netDRV()
{
    // unk08, mainData, data2, servers, events: {NULL, 0, 0}
    // vtable = 0x0060A5E0
    callback           = NULL;
    serverName[0]      = 0;
    maxPlayers         = 0;
    bMainDataPublished = 1;
    unk78              = 1;
    enumGeneration     = 0;
    gamePort           = 7777;           // 0x1E61 -- overwritten by Server_Start
    hEventsMutex       = CreateMutexA(NULL, FALSE, NULL);
    hServersMutex      = CreateMutexA(NULL, FALSE, NULL);
    lastServerId       = 0;
    bServerListUpdated = 0;
    bGameStarted       = 0;
    bFrozen            = 0;
    bAcceptServers     = 1;              // 1.2: new field
    bDedicated         = 0;              // 1.2: new field
    // netVersion (+0x14) is not set here; Drv_Init sets it.
    memset(&netDbgStat, 0, sizeof(netDbgStat));
}

// v12 0x0058F4E0  (feb 0x004A6DF0)   (scalar deleting dtor ??_G: v12 0x0058F400, slot +0x00)
// Destructor: close mutexes, free events (with payloads), servers (with blobs) and the data arrays.
netDRV::~netDRV()
{
    CloseHandle(hEventsMutex);
    CloseHandle(hServersMutex);
    for (int i = 0; i < events.size; i++)
        apFree(events.data[i].data.data);
    apFree(events.data);
    for (int i = 0; i < servers.size; i++) {
        apFree(servers.data[i].data2.data);
        servers.data[i].mainData.~arrVector();      // v12 0x0058FF80 (just apFree)
    }
    apFree(servers.data);
    apFree(data2.data);
    apFree(mainData.data);
    // unk08 is not freed (same in feb)
    // OnServerDelete is NOT called here: derived dtors must free driverData themselves.
}

// v12 0x0058F5B0  (feb 0x004A6EC0)
// Install the event callback (the game passes gsMP_NETWORK::OnNetDrvEvent 0x0052FBB0).
void netDRV::SetCallback(netDRV_CALLBACK cb)
{
    callback = cb;
}

// v12 0x0058F5C0  (feb 0x004A6ED0)   slot +0x04
// Per-frame update of the base: deliver all queued events.
void netDRV::Think()
{
    ProcessEvents(0);
}

// v12 0x0058F5D0  (feb 0x004A6EE0)
// Deliver queued events to the callback on the game thread. The queue is swapped out under the
// mutex; if a callback freezes the driver, the undelivered rest is put back in front of the queue.
// onlyDisconnects != 0: deliver only netEV_CLIENT_LEFT (used from DPUBI dtor / Server_Stop /
// Client_DisconnectFromGame, v12 callers 0x0059046E, 0x0059113F, 0x00591A3A).
void netDRV::ProcessEvents(int onlyDisconnects)
{
    arrVector<netDRV_EVENT> local;               // {NULL,0,0}
    local.Resize(0);
    int i = 0;

    if (bGameStarted && !bFrozen && callback &&
        WaitForSingleObject(hEventsMutex, NET_MUTEX_TIMEOUT) == WAIT_OBJECT_0)
    {
        // swap(events, local)
        arrVector<netDRV_EVENT> tmp = events; events = local; local = tmp;
        ReleaseMutex(hEventsMutex);

        for (i = 0; i < local.size; i++) {
            netDRV_EVENT* ev = &local.data[i];
            if (ev->type == netEV_NONE)
                continue;
            if (onlyDisconnects && ev->type != netEV_CLIENT_LEFT)
                continue;
            callback(ev);                        // __fastcall, ecx = ev
            if (bFrozen) { i++; break; }         // callback froze the driver (e.g. level change)
        }

        netPckProcessed = local.size - i;        // (name from map; it is the count NOT processed)

        if (i < local.size) {
            // put the unprocessed tail back at the front of the live queue
            WaitForSingleObject(hEventsMutex, NET_MUTEX_TIMEOUT);
            int n = local.size - i;
            netDRV_EVENT empty;                  // type 0, clientId -1
            events.Insert(0, empty, n);          // inlined: grow, memmove, copy-construct n empties
            for (int k = 0; i < local.size; i++, k++)
                swap(events.data[k], local.data[i]);   // 5 dwords swapped
            ReleaseMutex(hEventsMutex);
        }
    }

    for (int k = 0; k < local.size; k++)
        apFree(local.data[k].data.data);
    apFree(local.data);
}

// v12 0x0058F870  (feb 0x004A7180)   slot +0x0C (feb +0x08)
// Base init: clear game-started/frozen and store the 1.2 init parameters.
void netDRV::Drv_Init(const netDRV_INIT* init)
{
    bGameStarted = 0;
    bFrozen      = 0;
    bDedicated   = init->bDedicated;     // 1.2: new
    netVersion   = init->netVersion;     // 1.2: new (10020000)
}

// v12 0x0058F8A0  (feb 0x004A71A0)   slot +0x18
const char* netDRV::Drv_GetStatus()
{
    return bGameStarted ? "Game in progress"     // v12 0x0063B094
                        : "Driver started";      // v12 0x0063B084
}

// v12 0x0058F8C0  (feb 0x004A71C0)   slot +0x1C
void netDRV::Drv_NotifyGameStarted() { bGameStarted = 1; }

// v12 0x0058F8D0  (feb 0x004A71D0)   slot +0x20
int netDRV::Drv_IsGameStarted() { return bGameStarted; }

// v12 0x0058F8E0  (feb 0x004A71E0)   slot +0x24
// Freeze / unfreeze event delivery.
void netDRV::Drv_Freeze(int freeze) { bFrozen = freeze; }

// v12 0x0058FA20  (feb 0x004A7410)   slot +0x2C
// Browser auto-refresh interval of the base driver (none).
float netDRV::Drv_GetRefreshInterval() { return 0.0f; }   // constant v12 0x005F9470

// v12 0x0058F8F0  (feb 0x004A71F0)   slot +0x30
// Base part of starting a server: name, max players, initial main data, port.
void netDRV::Server_Start(const netDRV_SERVER_CREATE* sc)
{
    strncpy(serverName, sc->name, 64);
    serverName[63] = 0;
    maxPlayers = sc->maxPlayers;
    if (bDedicated)                       // 1.2: dedicated host takes no player slot,
        maxPlayers++;                     //      so the advertised maximum gets one extra
    mainData.Clear();                     // size = 0 (Realloc(0) only if size and capacity < 0)
    mainData.Append(sc->dataBegin, sc->dataEnd - sc->dataBegin);
    data2.Clear();
    bMainDataPublished = 0;
    unk78              = 1;
    gamePort           = sc->port;
    bGameStarted       = 0;
}

// v12 0x0058FA10  (feb 0x004A7300)   slot +0x3C
const char* netDRV::Server_GetStatus() { return "Server started"; }   // v12 0x0063B0A8

// v12 0x00501B80 (shared stub)   slot +0x40   1.2: NEW
// Round-trip time to a client in ms; the base driver does not know (-1).
int netDRV::Server_GetClientPing(int /*clientIdx*/) { return -1; }

// v12 0x0058FA30  (feb 0x004A7310)   slot +0x48
// Replace the server main data blob (advertised in enum replies / lobby) and mark it for publishing.
void netDRV::Server_SetMainData(const void* data, int len)
{
    mainData.Clear();
    if (len == 0) {
        bMainDataPublished = 0;
        return;
    }
    mainData.Append((const unsigned char*)data, len);   // inlined grow (x2) + memmove + copy loop
    bMainDataPublished = 0;
}

// v12 0x0058FAF0  (feb 0x004A73D0)   slot +0x4C
// Copy the main data out if the buffer is large enough; always returns the size in *len.
void netDRV::Server_GetMainData(void* data, int* len)
{
    if (data && mainData.size <= *len)
        memcpy(data, mainData.data, mainData.size);
    *len = mainData.size;
}

// v12 0x0058FB30  (feb 0x004A7420)   slot +0x78
const char* netDRV::Browser_GetStatus() { return "Browser started"; } // v12 0x0063B0B8

// v12 0x0058FB40  (feb 0x004A7430)   slot +0x84
// Base part of joining: a client is never "in game" until the server says so.
void netDRV::Client_JoinGame(const netDRV_CLIENT_JOIN* /*j*/) { bGameStarted = 0; }

// v12 0x0058FB50  (feb 0x004A7440)   slot +0x90
const char* netDRV::Client_GetStatus() { return "Client started"; }   // v12 0x0063B0C8

// v12 0x0058FB60   slot +0x94   1.2: NEW
// Round-trip time to the server in ms; unknown in the base driver.
int netDRV::Client_GetPing() { return -1; }

// v12 0x0058FB70  (feb 0x004A7450)
// Returns and clears the "server list changed" flag (polled by wrUI_JOIN_WINDOW each frame).
int netDRV::Browser_IsServerListUpdated()
{
    int r = bServerListUpdated;
    bServerListUpdated = 0;
    return r;
}

// v12 0x0058FB90  (feb 0x004A7470)
// Iterate server ids: prevId -1 gives the first; returns -1 after the last (or if prevId is gone).
int netDRV::Browser_GetNextServer(int prevId)
{
    int idx = 0;
    if (prevId != -1)
        idx = FindServerIdx(prevId) + 1;          // inlined; -1 + 1 = 0 restarts from the top
    if (idx >= servers.size)
        return -1;
    return servers.data[idx].id;
}
// Note: no lock is taken here or in Browser_GetServerInfo (same in feb); the browser thread
// (DirectPlay callbacks) modifies the list under hServersMutex.

// v12 0x0058FBF0  (feb 0x004A74D0)
// Entry for a server id, or NULL.
const netDRV_SERVER_INFO* netDRV::Browser_GetServerInfo(int id)
{
    for (int i = 0; i < servers.size; i++)
        if (servers.data[i].id == id)
            return &servers.data[i];
    return NULL;
}

// v12 0x0058FC40  (feb 0x004A7520)
// Index of a server id in the list, -1 if absent.
int netDRV::FindServerIdx(int id)
{
    for (int i = 0; i < servers.size; i++)
        if (servers.data[i].id == id)
            return i;
    return -1;
}

// v12 0x0058FC70  (feb 0x004A7550)
// Next unique server id (1, 2, 3 ...; never reused while the driver lives).
int netDRV::NewServerId()
{
    return ++lastServerId;
}

// v12 0x0058FC80  (feb 0x004A7560)
// Empty the browser list: OnServerDelete for each entry, free the blobs, size = 0, flag "updated".
void netDRV::ClearServersList()
{
    if (servers.size == 0)
        return;
    bServerListUpdated = 1;
    for (int i = 0; i < servers.size; i++)
        OnServerDelete(&servers.data[i]);               // slot +0xA4 (feb +0x98)
    for (int i = 0; i < servers.size; i++) {
        apFree(servers.data[i].data2.data);
        apFree(servers.data[i].mainData.data);
    }
    // (dead arrVector::Resize(0) template code for size < 0 follows in the binary; it inlines the
    //  netDRV_SERVER_INFO ctor, with the 1.2 defaults ping = 0, enumGeneration = -1)
    servers.size = 0;
    // No lock is taken here; callers hold LockServers() (? not checked for every caller).
}

// v12 0x0058FDC0  (feb 0x004A76A0)
// Take the server list mutex (15 s timeout, result ignored) and return the list.
arrVector<netDRV_SERVER_INFO>* netDRV::LockServers()
{
    WaitForSingleObject(hServersMutex, NET_MUTEX_TIMEOUT);
    return &servers;
}

// v12 0x0058FDE0  (feb 0x004A76C0)
void netDRV::UnlockServers()
{
    ReleaseMutex(hServersMutex);
}

// v12 0x0058FDF0  (feb 0x004A76D0)
// Queue an event (called from DirectPlay / Ubi threads); the payload is copied.
void netDRV::AddEvent(int type, int clientId, const void* data, int len)
{
    WaitForSingleObject(hEventsMutex, NET_MUTEX_TIMEOUT);
    netDRV_EVENT empty;                                  // {0, -1, {}}
    events.Insert(events.size, empty, 1);                // v12 0x0058FF90
    netDRV_EVENT& ev = events.data[events.size - 1];
    ev.type     = type;
    ev.clientId = clientId;
    if (len)
        ev.data.Append((const unsigned char*)data, len); // inlined grow + memmove + copy
    ReleaseMutex(hEventsMutex);
}
// No limit on the queue length (grows x2).

// v12 0x0058FF10  (feb 0x004A77F0)
// Neutralise all queued events of a client (type 0, clientId -1) so they are skipped.
void netDRV::EraseClientEvents(int clientId)
{
    WaitForSingleObject(hEventsMutex, NET_MUTEX_TIMEOUT);
    for (int i = 0; i < events.size; i++) {
        if (events.data[i].clientId == clientId) {
            events.data[i].clientId = -1;
            events.data[i].type     = netEV_NONE;
        }
    }
    ReleaseMutex(hEventsMutex);
}

// ===========================================================================
// Template instances emitted in this obj (bodies identical to feb, "exact" matches)
// ===========================================================================
// v12 0x0058FF80  arrVector<unsigned char>::~arrVector     apFree(data)
// v12 0x0058FF90  (feb 0x004A7870)  arrVector<netDRV_EVENT>::Insert(int pos, const netDRV_EVENT&, int n)
// v12 0x005900E0  (feb 0x004A79C0)  arrVector<char>::Resize(int)
// v12 0x00590110  (feb 0x004A79F0)  arrVector<netDRV_EVENT>::Resize(int)   new elems {0,-1,{}}
// v12 0x005901B0  (feb 0x004A7A90)  netDRV_EVENT::netDRV_EVENT(const netDRV_EVENT&)  deep copy of data
// v12 0x00590230  (feb 0x004A7B10)  arrVector<unsigned char>::Realloc(int)
// v12 0x00590260  (feb 0x004A7B40)  arrVector<netDRV_EVENT>::Realloc(int)  frees payloads of cut elems
// Growth rule everywhere: newCap = max(cap * 2, needed).
//
// Shared one-line stubs used in the base vtable (COMDAT-folded with unrelated classes):
//   0x00401380 ret            (slots +0x08 Drv_Flush, +0x50, +0x7C, +0x9C)
//   0x00403A20 ret            (+0x54, +0x5C, +0x64, +0x68)
//   0x00502570 xor eax,eax    (+0x14, +0x28, +0x38, +0x74, +0x88, +0x8C)
//   0x0048DF30 xor eax,eax    (+0x60)
//   0x00548680 mov eax,1      (+0x10, +0x34, +0x70)
//   0x00501B80 or eax,-1; ret 4   (+0x40)   0x004CA6D0 ret 0x10 (+0x44)
//   0x00403940 ret 0xC (+0x58)  0x0051AAF0 ret 4 (+0x6C, +0x80, +0xA4)
//   0x0051C0D0 ret 0xC (+0x98)  0x00522F80 mov eax,1; ret 8 (+0xA0)
