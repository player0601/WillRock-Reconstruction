// network_dpubi_dp.cpp   (feb obj drv:network_dpubi_dp.obj)
//
// Will Rock 1.2 (v12.exe): DirectPlay 8 server / client used by netDRV_DPUBI for all game traffic
// (LAN and Internet; the Ubi.com part only finds servers). See network_dpubi_dp.h for layouts and
// the driver callback slots.
//
// Summary of 1.2 changes in this file
//   * Message coalescing (netDP_PACKER): Send() only queues, Flush() (once per driver frame,
//     called from 0x00590B9D / 0x00590BAC) packs records with the same target+flags into packets
//     of <= 700 bytes with a 1/2-byte length prefix per record. The receive handlers split them.
//   * Close() is followed by Sleep(2000) when it succeeded (everywhere a session is closed).
//   * Initialize() flag DPNINITIALIZE_DISABLELINKTUNING is now optional: ini [Multiplayer] NoLinkTuning.
//   * Host() reports the actually bound port back to the driver (port+0..+10 retry existed in feb).
//   * Enum query / connect request user data are produced by the driver as binary blobs
//     (feb: a 36-char name string); replies are kept in replyBuffers until DPN_MSGID_RETURN_BUFFER.
//   * CONNECT_COMPLETE reply data is handed to the driver (version check result).
//   * IsServersEqual first asks IDirectPlay8Address::IsEqual.
//   * New: GetPing (server/client), GetNumPlayers (server), full HRESULT -> text table in GetErrorDesc.
//
// String addresses are v12.

#include "network_dpubi_dp.h"
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
template<> void arrVector<char>::Insert(int pos, const char& v, int count);
template<> void arrVector< netDP_REF_PTR<IDirectPlay8Address> >::Resize(int n);
template<> void arrVector< netDP_REF_PTR<IDirectPlay8Address> >::Insert(int pos, const netDP_REF_PTR<IDirectPlay8Address>& v, int count);
template<> void arrVector< netDP_REF_PTR<IDirectPlay8Address> >::Reserve(int n);

static const char s_Empty[] = "";                         // 0x00640358

char netDP_ErrorBuf[256];                                 // 0x009AF6F8 ? size unknown (feb 0x00964288)

// ============================================================================================
//  netDP_PACKER  (1.2: NEW)
// ============================================================================================

// v12 0x00597FD0  (no feb twin)
// Returns the next record after 'prev' (or the first one when prev == NULL) that is not yet sent.
// When a scan from the start finds nothing, the whole queue is emptied.
netDP_QREC* netDP_PACKER::NextRecord(netDP_QREC* prev)
{
    int fromStart;
    netDP_QREC* rec;

    if (prev == NULL)
    {
        if (queue.size == 0)
            return NULL;
        rec = (netDP_QREC*)queue.data;
        if (rec->len >= 0)                       // bit31 clear = still pending
            return rec;
        fromStart = 1;
    }
    else
    {
        rec       = prev;
        fromStart = 0;
    }

    for (;;)
    {
        rec = (netDP_QREC*)((char*)rec + (rec->len & 0x7FFFFFFF) + sizeof(netDP_QREC));
        if ((char*)rec >= queue.data + queue.size)
            break;
        if (rec->len >= 0)
            return rec;
    }

    if (fromStart)
        queue.Resize(0);                         // inlined arrVector<char>::Resize(0)
    return NULL;
}

// v12 0x00598050  (no feb twin)
// Builds one packet in 'out': the first pending record plus every later pending record with the
// same dpnid and flags, as long as the packet stays <= maxSize. Used records get bit31 in len.
// Records for other targets stay queued for the next call. Returns the number of records packed.
int netDP_PACKER::PopPacket(DPNID* dpnid, int* flags, int* size, char** data, int maxSize)
{
    out.Resize(0);                                // inlined (0x0059805B..0x00598073)

    netDP_QREC* rec = NextRecord(NULL);
    if (rec == NULL)
        return 0;

    *dpnid = rec->dpnid;
    *flags = rec->flags;
    int count = 1;
    AppendRecord(rec + 1, rec->len, 0x7FFFFFFF); // the first record always goes, even if > maxSize
    rec->len |= 0x80000000;

    while ((rec = NextRecord(rec)) != NULL)
    {
        if (rec->dpnid != *dpnid || rec->flags != *flags)
            continue;
        if (!AppendRecord(rec + 1, rec->len, maxSize))
            break;                               // packet full: the rest waits for the next PopPacket
        rec->len |= 0x80000000;
        count++;
    }

    *size = out.size;
    *data = out.data;
    return count;
}

// v12 0x00598130  (no feb twin)
// Appends [length prefix][data] to 'out'. Returns 0 (and appends nothing) if it would exceed maxSize.
int netDP_PACKER::AppendRecord(const void* data, int len, int maxSize)
{
    int hdrLen = (len > 0x80) ? 2 : 1;

    if (out.size + hdrLen + len > maxSize)
        return 0;

    int need = out.size + hdrLen + len;          // exact reserve (inlined Reserve, 0x00598166..0x00598190)
    if (need > out.capacity)
    {
        if (need < out.size) out.Resize(need);
        out.data     = (char*)apRealloc(out.data, need);
        out.capacity = need;
    }

    char b;
    if (hdrLen == 1)
    {
        b = (char)((len - 1) | 0x80);
        out.Insert(out.size, b, 1);              // 0x00599E70
    }
    else
    {
        b = (char)((len - 1) >> 8);              // high byte, bit7 clear (len <= 0x8000 assumed, not checked)
        out.Insert(out.size, b, 1);
        b = (char)(len - 1);                     // low byte (inlined Insert)
        out.Insert(out.size, b, 1);
    }

    if (len != 0)                                // inlined Insert of len bytes
        out.Insert(out.size, (const char*)data, len);

    return 1;
}

// ============================================================================================
//  netDP_SERVER
// ============================================================================================

// v12 0x005982C0  (feb 0x0057D190)
// Constructor: no DirectPlay object yet.
netDP_SERVER::netDP_SERVER(netDRV_DPUBI* d)
{
    pServer.p  = NULL;
    // 1.2: packer.queue / packer.out / replyBuffers zeroed (+0x14..+0x34)
    drv        = d;
    bStarted   = 0;
    hrLast     = 0;
    errContext = s_Empty;
}

// v12 0x00598300  (feb 0x0057D1B0)
// Destructor: cancels everything, closes the session and frees the 1.2 buffers.
netDP_SERVER::~netDP_SERVER()
{
    if (pServer)
    {
        pServer->CancelAsyncOperation(NULL, DPNCANCEL_ALL_OPERATIONS);   // vt+0x14, 0x8000
        if (SUCCEEDED(pServer->Close(DPNCLOSE_IMMEDIATE)))               // vt+0x5C, 1
            Sleep(2000);                                                 // 1.2: NEW
        pServer = netDP_REF_PTR<IDirectPlay8Server>(NULL, 1);
    }

    // 1.2: NEW
    for (int i = 0; i < replyBuffers.size; i++)
        apFree(replyBuffers.data[i]);
    apFree(replyBuffers.data);
    apFree(packer.out.data);
    apFree(packer.queue.data);

    pServer.Release();
}

// v12 0x005983C0  (feb 0x0057D210)
// Cancels, closes and releases the IDirectPlay8Server (bStarted is left as is).
void netDP_SERVER::Shutdown()
{
    if (pServer)
    {
        pServer->CancelAsyncOperation(NULL, DPNCANCEL_ALL_OPERATIONS);
        if (SUCCEEDED(pServer->Close(DPNCLOSE_IMMEDIATE)))
            Sleep(2000);                                                 // 1.2: NEW
        pServer.Release();
        pServer.p = NULL;
    }
}

// v12 0x00598410  (feb 0x0057D250)
// Creates IDirectPlay8Server and hosts on the driver's game port; on "address in use" style
// error 0x80158320 retries port+1 .. port+10. Port 0 = let DirectPlay choose.
void netDP_SERVER::StartServer()
{
    netDP_REF_PTR<IDirectPlay8Server>  server(NULL, 1);   // [esp+0x10]
    netDP_REF_PTR<IDirectPlay8Address> addr(NULL, 1);     // [esp+0x14]
    DPN_APPLICATION_DESC               appDesc;           // [esp+0x28]
    IDirectPlay8Address*               devAddr;           // [esp+0x24]
    int                                noLinkTuning;      // [esp+0x20]

    if (pServer)
    {
        if (bStarted && SUCCEEDED(pServer->Close(DPNCLOSE_IMMEDIATE)))
            Sleep(2000);                                  // 1.2: NEW
        pServer = netDP_REF_PTR<IDirectPlay8Server>(NULL, 1);
    }
    bStarted = 0;

    server.Release();
    hrLast = CoCreateInstance(CLSID_DirectPlay8Server, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDirectPlay8Server, (void**)&server.p);   // CLSID 0x0060A348, IID 0x0060A328
    if (FAILED(hrLast))
    {
        errContext = "DPlayServer create: ";                               // 0x0063BC3C
        return;
    }

    // 1.2: link tuning can be switched off from the ini (feb: always DPNINITIALIZE_DISABLELINKTUNING)
    DWORD initFlags = 0;
    if (_apCfgReadBool(NULL, "Multiplayer", "NoLinkTuning", &noLinkTuning) && noLinkTuning)  // 0x0062B444, 0x0063BC2C
        initFlags = DPNINITIALIZE_DISABLELINKTUNING;                       // 4

    hrLast = server->Initialize(this, DpMsgHandler, initFlags);            // vt+0x0C
    if (FAILED(hrLast))
    {
        errContext = "DPlayServer initialize: ";                           // 0x0063BC10
        return;                                                            // (refs released)
    }

    addr.Release();
    hrLast = CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDirectPlay8Address, (void**)&addr.p);   // 0x0060A378 / 0x0060A368
    if (FAILED(hrLast))
    {
        errContext = "DPlayAddr1 create: ";                                // 0x0063BBFC
        return;
    }

    hrLast = addr->SetSP(&CLSID_DP8SP_TCPIP);                              // vt+0x34, 0x0060A318
    if (FAILED(hrLast))
    {
        errContext = "DPlayAddr1 provider: ";                              // 0x0063BBE4
        return;
    }

    DWORD port    = drv->Dp_GetGamePort() & 0xFFFF;                        // slot +0xB0
    DWORD maxPort = (port == 0) ? 0 : port + 10;                           // TWEAK: retry range (lea edi,[eax+0xA] 0x005985B1)

    // ? if port > maxPort (only possible on overflow) or the loop runs out, the code jumps to the
    //   success tail at 0x00598661 without a successful Host(); unreachable for a 16-bit port.
    while ((int)port <= (int)maxPort)
    {
        if (port != 0)
        {
            hrLast = addr->AddComponent(DPNA_KEY_PORT, &port, sizeof(DWORD), DPNA_DATATYPE_DWORD);  // vt+0x48, L"port" 0x0063B120
            if (FAILED(hrLast))
            {
                errContext = "DPlayAddr1 port: ";                          // 0x0063BBD0
                return;
            }
        }

        drv->Dp_Server_FillAppDesc(&appDesc);                              // slot +0xBC
        devAddr = addr;
        hrLast = server->Host(&appDesc, &devAddr, 1, NULL, NULL, NULL, 0); // vt+0x34
        if (SUCCEEDED(hrLast))
        {
            drv->Dp_Server_SetBoundPort(port);                             // slot +0xB4  1.2: NEW
            break;
        }
        if (hrLast != 0x80158320 || (int)port >= (int)maxPort)             // ? DPNERR_* name; game text "Invalid device address"
        {
            errContext = "DPlay host create: ";                            // 0x0063BBBC
            return;
        }
        port++;                                                            // try the next port
        hrLast = 0;
    }

    // success (0x00598661): keep the server object
    pServer  = server;
    bStarted = 1;
}

// v12 0x005986C0  (feb 0x0057D4C0)
// Text for the last error. 1.2: table of known HRESULTs (feb knew only two), else
// "Saber internal error %X:\n" + context + DXGetErrorString8A(hr).
const char* netDP_SERVER::GetErrorDesc()
{
    HRESULT hr = hrLast;
    switch ((DWORD)hr)
    {
    case 0x80158260:                                                       // feb: "Player with such a name is already ..."
    case 0x80158560: return "Player already connected";                    // 0x0063BBA0  (0x80158560 = AddPlayer failed, see Dp_Server_OnConnectAttempt)
    case 0x80158180: return "TPC/IP initialization failed";                // 0x0063BB80  (sic)
    case 0x80158060: return "User already connected";                      // 0x0063BB68
    case 0x80158320: return "Invalid device address";                      // 0x0063BB50
    case 0x80158420: return "Invalid player name";                         // 0x0063BB3C
    case 0x80158370: return "Wrong host address";                          // 0x0063BB28
    case 0x80158450: return "Invalid URL";                                 // 0x0063BB1C
    case 0x80158610: return "Server is full";                              // 0x0063BB0C
    case 0x80158630: return "Timeout";                                     // 0x0063BB04
    case 0x8015A001: return "Invalid server version";                      // 0x0063BAD0  game code (driver connect reply)
    case 0x8015A000: return "Bad connection data";                         // 0x0063BABC  game code
    }
    sprintf(netDP_ErrorBuf, "Saber internal error %X:\n", hr);             // 0x0063BAE8
    strcat(netDP_ErrorBuf, errContext);
    strcat(netDP_ErrorBuf, DXGetErrorString8A(hr));
    return netDP_ErrorBuf;
}

// v12 0x00598810  (feb 0x0057D560)
// Status line for the UI.
const char* netDP_SERVER::GetStatusDesc()
{
    return bStarted ? "Server started"      // 0x0063B0A8
                    : "Starting server";    // 0x0063B294
}

// v12 0x00598830  1.2: NEW
// Called once per driver frame: sends every queued record as coalesced packets (<= 700 bytes).
void netDP_SERVER::Flush()
{
    DPNID          to;
    int            flags, size;
    char*          data;
    DPN_BUFFER_DESC buf;
    DPNHANDLE      hAsync;
    int            first = 1;               // ? local set to 1 then cleared each loop; never read otherwise

    while (packer.PopPacket(&to, &flags, &size, &data, netDP_MAX_PACKET) > 0)   // TWEAK: 0x2BC at 0x0059883B and 0x005988D1
    {
        if (first) first = 0;

        DWORD sendFlags, timeout = 0;
        if (flags & netSEND_RELIABLE)
            sendFlags = DPNSEND_GUARANTEED;                       // 0x08  1.2: (feb 0x88 = GUARANTEED|PRIORITY_HIGH)
        else
        {
            sendFlags = DPNSEND_NOCOMPLETE | DPNSEND_NONSEQUENTIAL; // 0x12
            if (flags & netSEND_TIMEOUT)
                timeout = 500;                                    // TWEAK: 0x1F4 at 0x005988A6
        }

        drv->nBytesSent += size;                                  // drv+0x08
        buf.dwBufferSize = size;
        buf.pBufferData  = (BYTE*)data;
        hAsync = 0;
        pServer->SendTo(to, &buf, 1, timeout, NULL, &hAsync, sendFlags);   // vt+0x38, call at 0x005988CA
    }
}

// v12 0x00598900  (feb 0x0057D580)
// 1.2: only queues {to, flags, len, data} in the packer; Flush() sends.
// feb sent immediately with SendTo (and counted drv->nBytesSent here).
void netDP_SERVER::Send(DPNID to, const void* data, int len, int flags)
{
    netDP_QREC hdr;
    hdr.dpnid = to;
    hdr.flags = flags;
    hdr.len   = len;

    // exact reserve for header + data, then two inlined arrVector<char>::Insert
    packer.queue.Reserve(packer.queue.size + len + sizeof(hdr));
    packer.queue.Insert(packer.queue.size, (const char*)&hdr, sizeof(hdr));
    if (len != 0)
        packer.queue.Insert(packer.queue.size, (const char*)data, len);
}

// v12 0x00598A40  (feb 0x0057D5F0)
// Kicks a client and tells the driver at once.
void netDP_SERVER::DisconnectPlayer(DPNID id)
{
    pServer->DestroyClient(id, NULL, 0, 0);                      // vt+0x60
    drv->Dp_Server_OnPlayerDestroyed(id);                        // slot +0xC8 (feb +0xB0)
}

// v12 0x00598A70  (feb 0x0057D620)
// Closes the hosted session and releases the server object.
void netDP_SERVER::StopServer()
{
    if (pServer)
    {
        if (bStarted && SUCCEEDED(pServer->Close(DPNCLOSE_IMMEDIATE)))
            Sleep(2000);                                         // 1.2: NEW
        pServer.Release();
        pServer.p = NULL;
    }
    bStarted = 0;
}

// v12 0x00598AC0  1.2: NEW (? name)
// Round trip time to a client in ms, -1 without a server. Called by the driver at 0x00590FCE.
int netDP_SERVER::GetPing(DPNID id)
{
    if (!pServer)
        return -1;
    DPN_CONNECTION_INFO info;                                    // 0x5C bytes, only dwSize set
    info.dwSize = sizeof(info);
    pServer->GetConnectionInfo(id, &info, 0);                    // vt+0x80; HRESULT not checked
    return info.dwRoundTripLatencyMS;
}

// v12 0x00598B00  1.2: NEW (? name)
// Current player count from DirectPlay's application desc (0 on failure / no server).
// Reached through a jmp thunk at 0x005930AA in netDRV_DPUBI.
int netDP_SERVER::GetNumPlayers()
{
    if (!pServer)
        return 0;
    BYTE  buffer[0x120];                                         // desc + strings
    DWORD size = 0x120;
    memset(buffer, 0, sizeof(buffer));
    ((DPN_APPLICATION_DESC*)buffer)->dwSize = sizeof(DPN_APPLICATION_DESC);  // 0x48
    HRESULT hr = pServer->GetApplicationDesc((DPN_APPLICATION_DESC*)buffer, &size, 0);  // vt+0x1C
    return SUCCEEDED(hr) ? ((DPN_APPLICATION_DESC*)buffer)->dwCurrentPlayers : 0;
}

// v12 0x00598B60  (feb 0x0057D660)
// DirectPlay server callback. Jump table 0x00598E70, index table 0x00598E8C (ids 0xFFFF0007..0x13).
HRESULT netDP_SERVER::MsgHandler(DWORD msgId, void* msg)
{
    arrVector<char> reply;            // 1.2: NEW, filled by the driver
    reply.Resize(0);

    if (!bStarted)
    {
        apFree(reply.data);
        return 1;                     // ? non-DPN_OK: reject / ignore everything before Host succeeded
    }

    switch (msgId)
    {
    case DPN_MSGID_CREATE_PLAYER:                                // 0xFFFF0007 -> 0x00598DD7
    {
        DPNMSG_CREATE_PLAYER* m = (DPNMSG_CREATE_PLAYER*)msg;
        drv->Dp_Server_OnPlayerCreated(m->dpnidPlayer, m->pvPlayerContext);   // slot +0xC4 (feb +0xAC)
        break;
    }

    case DPN_MSGID_DESTROY_PLAYER:                               // 0xFFFF0009 -> 0x00598DF2
    {
        DPNMSG_DESTROY_PLAYER* m = (DPNMSG_DESTROY_PLAYER*)msg;
        drv->Dp_Server_OnPlayerDestroyed(m->dpnidPlayer);        // slot +0xC8 (feb +0xB0)
        break;
    }

    case DPN_MSGID_ENUM_HOSTS_QUERY:                             // 0xFFFF000A -> 0x00598BC4
    {
        // 1.2: raw query blob goes to the driver (feb: strncpy'd 36-char string)
        DPNMSG_ENUM_HOSTS_QUERY* m = (DPNMSG_ENUM_HOSTS_QUERY*)msg;
        reply.Resize(0);
        if (!drv->Dp_Server_OnEnumQuery(m->pvReceivedData, m->dwReceivedDataSize, &reply))  // slot +0xB8
        {
            apFree(reply.data);
            return 1;                                            // do not answer this enumeration
        }
        void* buf = NULL;
        if (reply.size > 0)
        {
            buf = apMalloc(reply.size);
            replyBuffers.Insert(replyBuffers.size, buf, 1);      // freed on RETURN_BUFFER / dtor
            memcpy(buf, reply.data, reply.size);
        }
        m->pvResponseData     = buf;                             // msg+0x18
        m->dwResponseDataSize = reply.size;                      // msg+0x1C
        break;
    }

    case DPN_MSGID_INDICATE_CONNECT:                             // 0xFFFF000E -> 0x00598C88
    {
        DPNMSG_INDICATE_CONNECT* m = (DPNMSG_INDICATE_CONNECT*)msg;
        reply.Resize(0);
        if (!drv->Dp_Server_OnConnectAttempt(m->pvUserConnectData, m->dwUserConnectDataSize,
                                             &reply, &m->pvPlayerContext))   // slot +0xC0
        {
            // rejected: the reply (error code for the client) is copied and lent to DirectPlay
            void* buf = NULL;
            if (reply.size > 0)
            {
                buf = apMalloc(reply.size);
                replyBuffers.Insert(replyBuffers.size, buf, 1);
                memcpy(buf, reply.data, reply.size);
            }
            m->pvReplyData     = buf;                            // msg+0x0C
            m->dwReplyDataSize = reply.size;                     // msg+0x10
            apFree(reply.data);
            return 1;                                            // refuse the connection
        }
        // accepted
        void* buf = NULL;
        if (reply.size > 0)
        {
            buf = apMalloc(reply.size);
            replyBuffers.Insert(replyBuffers.size, buf, 1);
            // ? 1.2 BUG: no memcpy on this path (0x00598D51..0x00598D83): the accept reply sent to the
            //   client is uninitialised heap memory of the right size.
        }
        m->pvReplyData     = buf;
        m->dwReplyDataSize = reply.size;
        break;
    }

    case DPN_MSGID_RECEIVE:                                      // 0xFFFF0011 -> 0x00598E09
    {
        // 1.2: receive-side splitter for netDP_PACKER packets
        DPNMSG_RECEIVE* m = (DPNMSG_RECEIVE*)msg;
        drv->nBytesReceived += m->dwReceiveDataSize;             // drv+0x0C
        const BYTE* p   = m->pReceiveData;
        const BYTE* end = m->pReceiveData + m->dwReceiveDataSize;
        while (p)
        {
            int len;
            const BYTE* rec;
            if (p[0] & 0x80) { len = (p[0] & 0x7F) + 1;          rec = p + 1; }
            else             { len = ((p[0] << 8) | p[1]) + 1;   rec = p + 2; }
            p = rec + len;
            if (p == end)                                        // ? no "> end" check: a malformed packet
                p = NULL;                                        //   makes this read past the buffer
            drv->Dp_Server_OnDataReceived(m->dpnidSender, rec, len);   // slot +0xCC (feb +0xB4, whole buffer)
        }
        break;
    }

    case DPN_MSGID_RETURN_BUFFER:                                // 0xFFFF0013 -> 0x00598D88  1.2: NEW
    {
        DPNMSG_RETURN_BUFFER* m = (DPNMSG_RETURN_BUFFER*)msg;
        for (int i = 0; i < replyBuffers.size; i++)
        {
            if (replyBuffers.data[i] == m->pvBuffer)
            {
                memmove(&replyBuffers.data[i], &replyBuffers.data[i + 1],
                        (replyBuffers.size - (i + 1)) * sizeof(void*));
                replyBuffers.size--;
                // ? the buffer itself is not apFree'd here (only removed from the list) - leak per reply
                break;
            }
        }
        break;
    }

    default:                                                     // -> 0x00598C6F
        break;
    }

    apFree(reply.data);
    return DPN_OK;
}

// v12 0x00598EA0  (feb 0x0057D7C0)
// Static trampoline registered with IDirectPlay8Server::Initialize.
HRESULT WINAPI netDP_SERVER::DpMsgHandler(void* ctx, DWORD msgId, void* msg)
{
    return ((netDP_SERVER*)ctx)->MsgHandler(msgId, msg);
}

// ============================================================================================
//  netDP_CLIENT
// ============================================================================================

// v12 0x00598EC0  (feb 0x0057D7E0)
// Constructor (curHost +0x2C is not initialised, as in feb).
netDP_CLIENT::netDP_CLIENT(netDRV_DPUBI* d)
{
    pClient.p = NULL;
    hosts.data = NULL; hosts.capacity = 0; hosts.size = 0;
    // 1.2: packer (+0x30..+0x44) zeroed (feb: enumData +0x30..+0x38)
    drv          = d;
    bInitialized = 0;
    bConnected   = 0;
    hrLast       = 0;
    errContext   = s_Empty;
    hEnum        = 0;
    hConnect     = 0;
}

// v12 0x00598F10  (feb 0x0057D820)
// Destructor. 1.2: calls Shutdown() (feb had it inlined without the Sleep).
netDP_CLIENT::~netDP_CLIENT()
{
    Shutdown();
    apFree(packer.out.data);                                     // 1.2
    apFree(packer.queue.data);                                   // 1.2 (feb: enumData)
    for (int i = 0; i < hosts.size; i++)
        hosts.data[i].Release();
    apFree(hosts.data);
    pClient.Release();
}

// v12 0x00598F80  (feb 0x0057D8E0)
// Cancels enumeration/connect, closes the session if connected and releases the client.
void netDP_CLIENT::Shutdown()
{
    if (pClient)
    {
        if (bInitialized)
        {
            pClient->CancelAsyncOperation(NULL, DPNCANCEL_ALL_OPERATIONS);  // vt+0x18
            hEnum    = 0;
            hConnect = 0;
            if (bConnected && SUCCEEDED(pClient->Close(DPNCLOSE_IMMEDIATE)))  // vt+0x38
                Sleep(2000);                                     // 1.2: NEW
        }
        pClient.Release();
        pClient.p = NULL;
    }
}

// v12 0x00598FE0  (feb 0x0057D930)
// Builds the list of addresses to enumerate from two "host[:port]" strings (empty = skip;
// no strings at all = one NULL address, i.e. LAN broadcast), then creates and initialises a new
// IDirectPlay8Client. The second parameter is not used (loop is fixed at 2 entries).
void netDP_CLIENT::StartBrowser(const char** hostStr, int /*nHosts ? unused*/)
{
    netDP_REF_PTR<IDirectPlay8Address> tmp(NULL, 1);
    WCHAR  wHost[128];                                           // [esp+0x24]
    DWORD  port;                                                 // [esp+0x20]
    int    noLinkTuning;

    if (pClient && bConnected)
    {
        if (SUCCEEDED(pClient->Close(DPNCLOSE_IMMEDIATE)))
            Sleep(2000);                                         // 1.2: NEW
        hConnect   = 0;
        bConnected = 0;
    }
    hConnect = 0;

    if (pClient)
    {
        if (bInitialized)
        {
            hEnum   = 0;
            curHost = 0;
            hosts.Resize(0);
        }
        pClient = netDP_REF_PTR<IDirectPlay8Client>(NULL, 1);
    }

    hEnum   = 0;
    curHost = 0;
    hosts.Resize(0);

    for (int i = 0; i < 2; i++)                                  // TWEAK: fixed 2 host strings (cmp ecx,2 at 0x005991D5)
    {
        const char* s = hostStr[i];
        if (s == NULL || *s == 0)
            continue;

        netDP_REF_PTR<IDirectPlay8Address> empty(NULL, 1);
        hosts.Insert(hosts.size, empty, 1);
        empty.Release();

        netDP_REF_PTR<IDirectPlay8Address>& a = hosts.data[hosts.size - 1];
        a.Release();
        hrLast = CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER,
                                  IID_IDirectPlay8Address, (void**)&a.p);
        if (FAILED(hrLast))
        {
            errContext = "DPlayAddr1 create: ";                  // 0x0063BBFC
            return;
        }
        hrLast = a->SetSP(&CLSID_DP8SP_TCPIP);
        if (FAILED(hrLast))
        {
            errContext = "DPlayAddr1 service: ";                 // 0x0063BC88
            return;
        }

        // widen (max 127 chars), then cut at ':' and use the rest as port
        int n = 0;
        for (const char* c = s; *c && n < 0x7F; c++, n++)
            wHost[n] = (WCHAR)(signed char)*c;
        wHost[n] = 0;

        const char* colon = strchr(s, ':');
        if (colon)
        {
            port = atoi(colon + 1);
            wHost[colon - s] = 0;
            hrLast = a->AddComponent(DPNA_KEY_PORT, &port, sizeof(DWORD), DPNA_DATATYPE_DWORD);   // L"port" 0x0063B120
            if (FAILED(hrLast))
            {
                errContext = "DPlayAddr1 port: ";                // 0x0063BBD0
                return;
            }
        }
        hrLast = a->AddComponent(DPNA_KEY_HOSTNAME, wHost, (wcslen(wHost) + 1) * 2,
                                 DPNA_DATATYPE_STRING);          // L"hostname" 0x0063B10C
        if (FAILED(hrLast))
        {
            errContext = "DPlayAddr1 hostaddr: ";                // 0x0063BC70
            return;
        }
    }

    if (hosts.size == 0)
    {
        netDP_REF_PTR<IDirectPlay8Address> none(NULL, 0);        // NULL host = broadcast on the LAN
        hosts.Insert(hosts.size, none, 1);
    }

    pClient.Release();
    hrLast = CoCreateInstance(CLSID_DirectPlay8Client, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDirectPlay8Client, (void**)&pClient.p);   // 0x0060A358 / 0x0060A338
    if (FAILED(hrLast))
    {
        errContext = "DPlayClient create: ";                     // 0x0063BCA0
        return;
    }

    DWORD initFlags = 0;                                         // 1.2: feb always 4
    if (_apCfgReadBool(NULL, "Multiplayer", "NoLinkTuning", &noLinkTuning) && noLinkTuning)
        initFlags = DPNINITIALIZE_DISABLELINKTUNING;

    hrLast = pClient->Initialize(this, DpMsgHandler, initFlags); // vt+0x0C
    if (FAILED(hrLast))
    {
        errContext = "DPlayClient initialize: ";                 // 0x0063BC54
        pClient.Release();
        pClient.p = NULL;
        return;
    }
    bInitialized = 1;
}

// v12 0x00599320  (feb 0x0057DC60)
// Starts one asynchronous EnumHosts on hosts[curHost] and advances curHost. MsgHandler calls it
// again on ASYNC_OP_COMPLETE until every host was tried (round robin over 'hosts').
// 1.2: no argument; the enum payload comes from the driver (feb: a string arg kept in enumData).
void netDP_CLIENT::EnumHosts()
{
    netDP_REF_PTR<IDirectPlay8Address> devAddr(NULL, 1);
    arrVector<char>                    userData;                 // 1.2: NEW
    DPN_APPLICATION_DESC               appDesc;

    userData.Resize(0);

    if (hEnum != 0 || hosts.size == 0)                           // 1.2: hosts.size check added (feb divided by it)
    {
        apFree(userData.data);
        return;
    }

    curHost = curHost % hosts.size;
    drv->Dp_Client_FillAppDesc(&appDesc);                        // slot +0xD0

    devAddr.Release();
    hrLast = CoCreateInstance(CLSID_DirectPlay8Address, NULL, CLSCTX_INPROC_SERVER,
                              IID_IDirectPlay8Address, (void**)&devAddr.p);
    if (FAILED(hrLast))
    {
        errContext = "DPlayAddr2 create: ";                      // 0x0063BCDC
        apFree(userData.data);
        return;
    }
    hrLast = devAddr->SetSP(&CLSID_DP8SP_TCPIP);
    if (FAILED(hrLast))
    {
        errContext = "DPlayAddr2 service: ";                     // 0x0063BCC4
        apFree(userData.data);
        return;
    }

    drv->Dp_Client_GetEnumData(&userData);                       // slot +0xD4  1.2: NEW
    drv->Dp_Client_OnEnumStart();                                // slot +0xE0  1.2: NEW

    hrLast = pClient->EnumHosts(&appDesc, hosts.data[curHost], devAddr,
                                userData.data, userData.size,
                                0, 0, 0,                         // TWEAK: count/retry/timeout = DirectPlay defaults
                                NULL, &hEnum, 0);                // vt+0x14, call at 0x0059943D
    curHost++;
    if (FAILED(hrLast))
        errContext = "EnumHosts: ";                              // 0x0063BCB8

    apFree(userData.data);
}

// v12 0x005994A0  (feb 0x0057DE00)
// Stops browsing: forgets the host list; releases the client unless keepClient.
void netDP_CLIENT::StopBrowsing(int keepClient)
{
    if (!pClient)
        return;

    if (bInitialized)
    {
        hEnum   = 0;
        curHost = 0;
        hosts.Resize(0);                                         // inlined (Release each / Reserve)
    }
    if (!keepClient)
    {
        pClient.Release();
        pClient.p = NULL;
    }
}

// v12 0x00599530  (feb 0x0057DE90)
// Connects to a browsed server. 1.2: the connect payload (version id + player name) comes
// from the driver (feb: a string argument).
void netDP_CLIENT::JoinGame(netDP_SERVER_DATA* sd)
{
    arrVector<char>      connData;                               // 1.2: NEW
    DPN_APPLICATION_DESC appDesc;

    connData.Resize(0);
    drv->Dp_Client_FillAppDesc(&appDesc);                        // slot +0xD0
    if (hEnum != 0)
    {
        drv->Dp_Client_OnEnumEnd();                              // slot +0xE8
        hEnum = 0;                                               // ? the pending enum is not cancelled here
    }
    drv->Dp_Client_GetConnectData(&connData);                    // slot +0xD8  1.2: NEW

    hrLast = pClient->Connect(&appDesc, sd->pHostAddr, sd->pDeviceAddr, NULL, NULL,
                              connData.data, connData.size,
                              NULL, &hConnect, 0);               // vt+0x1C, call at 0x005995A9
    if (FAILED(hrLast))
        errContext = "Connect: ";                                // 0x0063BCF0

    apFree(connData.data);
}

// v12 0x005995E0  (feb 0x0057DF20)
// Same table as netDP_SERVER::GetErrorDesc (separate copy, reads hrLast +0x10 / errContext +0x14).
const char* netDP_CLIENT::GetErrorDesc()
{
    HRESULT hr = hrLast;
    switch ((DWORD)hr)
    {
    case 0x80158260:
    case 0x80158560: return "Player already connected";
    case 0x80158180: return "TPC/IP initialization failed";
    case 0x80158060: return "User already connected";
    case 0x80158320: return "Invalid device address";
    case 0x80158420: return "Invalid player name";
    case 0x80158370: return "Wrong host address";
    case 0x80158450: return "Invalid URL";
    case 0x80158610: return "Server is full";
    case 0x80158630: return "Timeout";
    case 0x8015A001: return "Invalid server version";
    case 0x8015A000: return "Bad connection data";
    }
    sprintf(netDP_ErrorBuf, "Saber internal error %X:\n", hr);
    strcat(netDP_ErrorBuf, errContext);
    strcat(netDP_ErrorBuf, DXGetErrorString8A(hr));
    return netDP_ErrorBuf;
}

// v12 0x00599730  (feb 0x0057DFC0)  unchanged
// Status line for the UI.
const char* netDP_CLIENT::GetStatusDesc()
{
    if (hConnect)     return "Client connecting";       // 0x0063BD0C
    if (hEnum)        return "Searching for servers";   // 0x0063BA70
    if (bInitialized) return "Client ready";            // 0x0063BCFC
    return "Initializing client";                       // 0x0063BA3C
}

// v12 0x00599760  1.2: NEW
// Client twin of netDP_SERVER::Flush (target id in the queue is 0xFFFFFFFF and ignored).
void netDP_CLIENT::Flush()
{
    DPNID          to;
    int            flags, size;
    char*          data;
    DPN_BUFFER_DESC buf;
    DPNHANDLE      hAsync;
    int            first = 1;                                    // ? unused, as in the server version

    while (packer.PopPacket(&to, &flags, &size, &data, netDP_MAX_PACKET) > 0)   // 0x2BC at 0x0059976B / 0x005997FC
    {
        if (first) first = 0;

        DWORD sendFlags, timeout = 0;
        if (flags & netSEND_RELIABLE)
            sendFlags = DPNSEND_GUARANTEED;                      // 1.2: feb 0x88
        else
        {
            sendFlags = DPNSEND_NOCOMPLETE | DPNSEND_NONSEQUENTIAL;
            if (flags & netSEND_TIMEOUT)
                timeout = 500;                                   // 0x005997D6
        }

        drv->nBytesSent += size;
        buf.dwBufferSize = size;
        buf.pBufferData  = (BYTE*)data;
        hAsync = 0;
        pClient->Send(&buf, 1, timeout, NULL, &hAsync, sendFlags);   // vt+0x20, call at 0x005997F5
    }
}

// v12 0x00599830  (feb 0x0057DFF0)
// 1.2: queues the message (dpnid = 0xFFFFFFFF); Flush() sends. Reached via jmp at 0x00591996.
void netDP_CLIENT::Send(const void* data, int len, int flags)
{
    netDP_QREC hdr;
    hdr.dpnid = 0xFFFFFFFF;
    hdr.flags = flags;
    hdr.len   = len;

    packer.queue.Reserve(packer.queue.size + len + sizeof(hdr));
    packer.queue.Insert(packer.queue.size, (const char*)&hdr, sizeof(hdr));
    if (len != 0)
        packer.queue.Insert(packer.queue.size, (const char*)data, len);
}

// v12 0x00599970  (feb 0x0057E060)
// Leaves the current game (closes the client session, keeps the object).
void netDP_CLIENT::DisconnectFromGame()
{
    if (pClient && bConnected)
    {
        if (SUCCEEDED(pClient->Close(DPNCLOSE_IMMEDIATE)))
            Sleep(2000);                                         // 1.2: NEW
        hConnect   = 0;
        bConnected = 0;
    }
    hConnect = 0;
}

// v12 0x005999B0  (feb 0x0057E090)
// Same server? 1.2: equal host addresses (DPNSUCCESS_EQUAL 0x00158005) count first;
// otherwise compare the instance GUIDs (feb: GUID only).
int netDP_CLIENT::IsServersEqual(netDP_SERVER_DATA* a, netDP_SERVER_DATA* b)
{
    if (a->pHostAddr && b->pHostAddr &&
        a->pHostAddr->IsEqual(b->pHostAddr) == DPNSUCCESS_EQUAL) // vt+0x1C
        return 1;
    return memcmp(&a->guidInstance, &b->guidInstance, sizeof(GUID)) == 0;
}

// v12 0x00599A00  (feb 0x0057E0C0)  unchanged
// Deletes a netDP_SERVER_DATA (driver callback when a server leaves the list).
void netDP_CLIENT::OnServerDelete(netDP_SERVER_DATA* sd)
{
    // compiler-generated: inlined destructor of the two ref pointers (+ a redundant pair of Releases
    // exactly as in feb: the first two Release calls do not clear the pointers)
    if (sd->pHostAddr.p)   sd->pHostAddr.p->Release();
    if (sd->pDeviceAddr.p) sd->pDeviceAddr.p->Release();
    sd->pDeviceAddr.Release();
    sd->pHostAddr.Release();
    operator delete(sd);
}

// v12 0x00599A60  1.2: NEW (? name)
// Round trip time to the server in ms, -1 without a client. Reached via jmp at 0x00591986.
int netDP_CLIENT::GetPing()
{
    if (!pClient)
        return -1;
    DPN_CONNECTION_INFO info;
    info.dwSize = sizeof(info);                                  // 0x5C
    pClient->GetConnectionInfo(&info, 0);                        // vt+0x50; HRESULT not checked
    return info.dwRoundTripLatencyMS;
}

// v12 0x00599A90  (feb 0x0057E120)
// DirectPlay client callback. Jump table 0x00599D54, index table 0x00599D6C (ids 0xFFFF0003..0x16).
HRESULT netDP_CLIENT::MsgHandler(DWORD msgId, void* msg)
{
    if (!bInitialized)
        return 1;

    switch (msgId)
    {
    case DPN_MSGID_ASYNC_OP_COMPLETE:                            // 0xFFFF0003 -> 0x00599B16
    {
        DPNMSG_ASYNC_OP_COMPLETE* m = (DPNMSG_ASYNC_OP_COMPLETE*)msg;
        if (m->hAsyncOp == hEnum)
        {
            hEnum = 0;
            if (curHost < hosts.size && !bConnected)
                EnumHosts();                                     // next host (0x00599B38)
            else
                drv->Dp_Client_OnEnumEnd();                      // slot +0xE8 (feb +0xC4)
        }
        else if (m->hAsyncOp == hConnect)
        {
            hConnect = 0;
            hrLast   = m->hResultCode;
            if (FAILED(hrLast))
                errContext = "Connect fail: ";                   // 0x0063BD20
            else
                hrLast = 1;
        }
        break;
    }

    case DPN_MSGID_CONNECT_COMPLETE:                             // 0xFFFF0005 -> 0x00599ACA
    {
        DPNMSG_CONNECT_COMPLETE* m = (DPNMSG_CONNECT_COMPLETE*)msg;
        if (m->hAsyncOp == hConnect)
        {
            hConnect = 0;
            // 1.2: the driver decodes the server's reply blob (version check etc.)
            hrLast = drv->Dp_Client_OnConnectResult(m->hResultCode, m->pvApplicationReplyData,
                                                    m->dwApplicationReplyDataSize);   // slot +0xDC
            if (FAILED(hrLast))
                errContext = "Connection result: ";              // 0x0063BD30
            bConnected = SUCCEEDED(hrLast);
        }
        break;
    }

    case DPN_MSGID_ENUM_HOSTS_RESPONSE:                          // 0xFFFF000B -> 0x00599BA7
    {
        if (hEnum == 0)
            break;
        DPNMSG_ENUM_HOSTS_RESPONSE* m = (DPNMSG_ENUM_HOSTS_RESPONSE*)msg;
        const DPN_APPLICATION_DESC* ad = m->pApplicationDescription;

        netDP_NEW_SERVER ns;
        int n = 0;
        for (const WCHAR* w = ad->pwszSessionName; *w && n < 0x3F; w++, n++)   // TWEAK: 63-char name
            ns.name[n] = (char)*w;
        ns.name[n]    = 0;
        ns.maxPlayers = ad->dwMaxPlayers;
        ns.curPlayers = ad->dwCurrentPlayers;
        ns.ping       = m->dwRoundTripLatencyMS;

        netDP_SERVER_DATA* sd = new netDP_SERVER_DATA;           // 0x18 bytes, refs start NULL
        sd->guidInstance = ad->guidInstance;
        sd->pHostAddr    = netDP_REF_PTR<IDirectPlay8Address>(m->pAddressSender, 1);
        sd->pDeviceAddr  = netDP_REF_PTR<IDirectPlay8Address>(m->pAddressDevice, 1);
        ns.serverData    = sd;
        // extra AddRef on both (the copy owned by the driver's server list; balanced in OnServerDelete)
        if (sd->pHostAddr.p)   sd->pHostAddr.p->AddRef();
        if (sd->pDeviceAddr.p) sd->pDeviceAddr.p->AddRef();

        drv->Dp_Client_OnServerFound(&ns, m->pvResponseData, m->dwResponseDataSize);  // slot +0xE4 (feb +0xC0)
        break;
    }

    case DPN_MSGID_RECEIVE:                                      // 0xFFFF0011 -> 0x00599CF6
    {
        // 1.2: splitter, same format as the server side
        DPNMSG_RECEIVE* m = (DPNMSG_RECEIVE*)msg;
        drv->nBytesReceived += m->dwReceiveDataSize;
        const BYTE* p   = m->pReceiveData;
        const BYTE* end = p + m->dwReceiveDataSize;
        while (p)
        {
            int len;
            const BYTE* rec;
            if (p[0] & 0x80) { len = (p[0] & 0x7F) + 1;          rec = p + 1; }
            else             { len = ((p[0] << 8) | p[1]) + 1;   rec = p + 2; }
            p = rec + len;
            if (p == end)
                p = NULL;
            drv->Dp_Client_OnDataReceived(rec, len);             // slot +0xEC (feb +0xC8)
        }
        break;
    }

    case DPN_MSGID_TERMINATE_SESSION:                            // 0xFFFF0016 -> 0x00599B98
        bConnected = 0;
        break;

    default:                                                     // -> 0x00599B0A
        break;
    }
    return DPN_OK;
}

// v12 0x00599D80  (feb 0x0057E3F0)
// Static trampoline registered with IDirectPlay8Client::Initialize.
HRESULT WINAPI netDP_CLIENT::DpMsgHandler(void* ctx, DWORD msgId, void* msg)
{
    return ((netDP_CLIENT*)ctx)->MsgHandler(msgId, msg);
}

// v12 0x00599DA0  (feb 0x0057E410)  unchanged
// ANSI -> UTF-16 by sign-extending each char (no code page); dstLen includes the terminator.
void __fastcall netDP_CLIENT::CharToWChar(const char* src, wrWCHAR* dst, int dstLen)
{
    for (int i = 0; i < dstLen - 1 && *src; i++)
        *dst++ = (unsigned short)(short)*src++;
    *dst = 0;
}

// ============================================================================================
//  netDP_REF_PTR<T>   (inline template instances, identical in feb and v12)
// ============================================================================================

// v12 0x00599DD0  (feb 0x0057E440)
// Wraps ptr, AddRef'ing it when addRef != 0.
template <class T>
netDP_REF_PTR<T>::netDP_REF_PTR(T* ptr, int addRef)
{
    p = ptr;
    if (addRef && ptr)
        ptr->AddRef();
}

// v12 0x00599E00  (feb 0x0057E470)
// Assignment: AddRef new, Release old.
template <class T>
const netDP_REF_PTR<T>& netDP_REF_PTR<T>::operator=(const netDP_REF_PTR<T>& o)
{
    if (o.p) o.p->AddRef();
    if (p) { p->Release(); p = NULL; }
    p = o.p;
    return *this;
}

// v12 0x00599E40  (feb 0x0057E4D0)
// AddRef if not NULL.
template <class T>
void netDP_REF_PTR<T>::AddRef() const
{
    if (p) p->AddRef();
}

// v12 0x00599E50  (feb 0x0057E4B0)
// Release and clear.
template <class T>
void netDP_REF_PTR<T>::Release()
{
    if (p) { p->Release(); p = NULL; }
}

// ============================================================================================
//  arrVector helpers emitted in this obj
// ============================================================================================

// v12 0x00599E70  1.2: NEW in this obj (no feb twin here)
// arrVector<char>::Insert(pos, value, count): inserts 'count' copies of one byte.
// Growth: capacity *= 2 (at least size+count).
template<> void arrVector<char>::Insert(int pos, const char& v, int count)
{
    if (count == 0)
        return;
    if (capacity < size + count)
    {
        int nc = capacity * 2;
        if (nc <= size + count) nc = size + count;
        if (nc > capacity)
        {
            if (nc < size) size = nc;
            data     = (char*)apRealloc(data, nc);
            capacity = nc;
        }
    }
    memmove(data + pos + count, data + pos, size - pos);
    for (int i = pos; i < pos + count; i++)
        data[i] = v;
    size += count;
}

// v12 0x00599F00  (feb 0x0057E4E0)  unchanged
// Resize: releases trimmed entries / zero-fills new ones (capacity grows exactly).
template<> void arrVector< netDP_REF_PTR<IDirectPlay8Address> >::Resize(int n)
{
    if (n < size)
    {
        for (int i = n; i < size; i++)
            data[i].Release();
        size = n;
        return;
    }
    if (n > size)
    {
        if (n > capacity)
        {
            data     = (netDP_REF_PTR<IDirectPlay8Address>*)apRealloc(data, n * 4);
            capacity = n;
        }
        for (int i = size; i < n; i++)
            data[i].p = NULL;
    }
    size = n;
}

// v12 0x00599F80  (feb 0x0057E560)  unchanged
// Insert 'count' copies of v at pos (AddRef each), capacity *= 2.
template<> void arrVector< netDP_REF_PTR<IDirectPlay8Address> >::Insert(int pos,
        const netDP_REF_PTR<IDirectPlay8Address>& v, int count)
{
    if (count == 0)
        return;
    if (capacity < size + count)
    {
        int nc = capacity * 2;
        if (nc <= size + count) nc = size + count;
        if (nc > capacity)
        {
            if (nc < size)
            {
                for (int i = nc; i < size; i++) data[i].Release();
                size = nc;
            }
            data     = (netDP_REF_PTR<IDirectPlay8Address>*)apRealloc(data, nc * 4);
            capacity = nc;
        }
    }
    memmove(&data[pos + count], &data[pos], (size - pos) * 4);
    for (int i = pos; i < pos + count; i++)
    {
        data[i].p = v.p;
        if (v.p) v.p->AddRef();
    }
    size += count;
}

// v12 0x0059A050  (feb 0x0057E630)  unchanged
// Reserve: grow capacity to n (exact).
template<> void arrVector< netDP_REF_PTR<IDirectPlay8Address> >::Reserve(int n)
{
    if (n > capacity)
    {
        if (n < size)
        {
            for (int i = n; i < size; i++) data[i].Release();
            size = n;
        }
        data     = (netDP_REF_PTR<IDirectPlay8Address>*)apRealloc(data, n * 4);
        capacity = n;
    }
}
