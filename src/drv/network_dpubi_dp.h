// network_dpubi_dp.h
//
// Will Rock 1.2 (v12.exe) - DirectPlay 8 transport of the "DPUBI" network driver.
// Reconstructed from feb build drv:network_dpubi_dp.obj (WillRock.map) + v12 disassembly.
// Documentation source for modders: reads like the original, is NOT meant to compile into the game.
//
// Types assumed from other files (not defined here):
//   netDRV_DPUBI              network_dpubi.h  - the driver; we only call its virtuals (slots listed below)
//   arrVector<T>              ap container     - { T* data; int size; int capacity; }  (12 bytes)
//   apMalloc / apFree / apRealloc               - engine allocator (v12 0x004AF5F0 / 0x004AF610 / 0x004AF620)
//   _apCfgReadBool(void** cfg, const char* section, const char* key, int* out)   fastcall, v12 0x004AF940
//   DXGetErrorString8A        dxerr8.lib, v12 0x005AB8F0 (feb 0x0058F9B0)
//   IDirectPlay8Server / IDirectPlay8Client / IDirectPlay8Address, DPN_* structs   <dplay8.h>, <dpaddr.h>
//
// Object sizes: netDP_SERVER 0x38 (feb 0x14), netDP_CLIENT 0x48 (feb 0x3C).

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
#ifdef _WIN32
#include <dpaddr.h>
#include <dxerr8.h>
#endif

class netDRV_DPUBI;

// ---------------------------------------------------------------------------------------------
// netDRV_DPUBI virtuals called from this file (v12 vtable 0x0060A698). Names are by meaning;
// the feb map names exist only for the feb slot numbers (shifted in 1.2).
//
//   server side                                                       v12 impl
//   slot +0xB0  unsigned short Dp_GetGamePort()                       0x005923D0  reads drv+0x7C
//   slot +0xB4  void  Dp_Server_SetBoundPort(int port)                0x005923E0  1.2: NEW
//   slot +0xB8  int   Dp_Server_OnEnumQuery(const void* data, DWORD size, arrVector<char>* reply)
//                                                                     0x00592470  0 = do not answer
//   slot +0xBC  void  Dp_Server_FillAppDesc(DPN_APPLICATION_DESC*)    0x00592400
//   slot +0xC0  int   Dp_Server_OnConnectAttempt(const void* data, DWORD size,
//                                arrVector<char>* reply, void** ppvPlayerContext)
//                                                                     0x005926F0  0 = reject
//   slot +0xC4  void  Dp_Server_OnPlayerCreated(DPNID, void* ctx)     0x005928B0
//   slot +0xC8  void  Dp_Server_OnPlayerDestroyed(DPNID)              0x005928F0
//   slot +0xCC  void  Dp_Server_OnDataReceived(DPNID, const void*, int len)   0x00592980
//
//   client side
//   slot +0xD0  void  Dp_Client_FillAppDesc(DPN_APPLICATION_DESC*)    0x00592A00
//   slot +0xD4  void  Dp_Client_GetEnumData(arrVector<char>* out)     0x00592A50  1.2: NEW (? exact content: see mp_browser.md)
//   slot +0xD8  void  Dp_Client_GetConnectData(arrVector<char>* out)  0x00592BB0  1.2: NEW (version + player name)
//   slot +0xDC  HRESULT Dp_Client_OnConnectResult(HRESULT, const void* reply, DWORD size) 0x00592D10  1.2: NEW
//   slot +0xE0  void  Dp_Client_OnEnumStart()                         0x00592D40  1.2: NEW (? ++drv+0x7E)
//   slot +0xE4  void  Dp_Client_OnServerFound(netDP_NEW_SERVER*, const void* resp, DWORD size) 0x00592D50
//   slot +0xE8  void  Dp_Client_OnEnumEnd()                           0x00593000
//   slot +0xEC  void  Dp_Client_OnDataReceived(const void*, int len)  0x00593020
//
// Driver data touched here (offsets in netDRV):
//   drv+0x08  int nBytesSent      ? (network.h calls +0x08 an arrVector; this file only does "+= len")
//   drv+0x0C  int nBytesReceived  ?
// ---------------------------------------------------------------------------------------------

// Game send flags (arg "flags" of Send). Not named in the map.
#define netSEND_RELIABLE   0x01   // -> DPNSEND_GUARANTEED
#define netSEND_TIMEOUT    0x02   // unreliable only: 500 ms send timeout

// 1.2 coalescer: maximum DirectPlay payload built by Flush. v12 0x0059883B / 0x0059876B / 0x005988D1 / 0x005997FC
#define netDP_MAX_PACKET   700    // 0x2BC

// ---------------------------------------------------------------------------------------------
// netDP_REF_PTR<T>  - tiny COM smart pointer (inline template, emitted in this obj)
// ---------------------------------------------------------------------------------------------
template <class T>
struct netDP_REF_PTR
{
    T* p;                                                        // +0x00

    netDP_REF_PTR() : p(0) {}                                    // [integrator] zero-initialised form used inline
    netDP_REF_PTR(T* ptr, int addRef);                           // v12 0x00599DD0 (feb 0x0057E440)
    const netDP_REF_PTR& operator=(const netDP_REF_PTR& o);      // v12 0x00599E00 (feb 0x0057E470)
    operator T*() const { return p; }
    T* operator->() const { return p; }
public:     // [integrator] was protected; the driver code calls Release() directly
    void Release();                                              // v12 0x00599E50 (feb 0x0057E4B0)
    void AddRef() const;                                         // v12 0x00599E40 (feb 0x0057E4D0)
};

// ---------------------------------------------------------------------------------------------
// netDP_SERVER_DATA - per browsed server, kept by the driver's server list (feb name).
// ---------------------------------------------------------------------------------------------
struct netDP_SERVER_DATA                                         // 0x18 bytes
{
    netDP_REF_PTR<IDirectPlay8Address> pHostAddr;                // +0x00 ENUM_HOSTS_RESPONSE.pAddressSender
    netDP_REF_PTR<IDirectPlay8Address> pDeviceAddr;              // +0x04 ENUM_HOSTS_RESPONSE.pAddressDevice
    GUID                               guidInstance;             // +0x08 app desc guidInstance
};

// netDP_NEW_SERVER - stack record handed to Dp_Client_OnServerFound (feb name).
struct netDP_NEW_SERVER                                          // 1.2: 0x50 bytes (feb 0x58)
{
    char               name[64];        // +0x00 pwszSessionName narrowed, max 63 chars
    DWORD              maxPlayers;      // +0x40 dwMaxPlayers
    DWORD              curPlayers;      // +0x44 dwCurrentPlayers
    DWORD              ping;            // +0x48 dwRoundTripLatencyMS
    netDP_SERVER_DATA* serverData;      // +0x4C 1.2: (feb +0x54; feb had pvResponseData/size at +0x4C/+0x50,
                                        //        1.2 passes them as arguments instead)
};

// ---------------------------------------------------------------------------------------------
// netDP_PACKER - 1.2: NEW. Game-level message coalescer embedded in netDP_SERVER (+0x14) and
// netDP_CLIENT (+0x30). "?" class name: not in the feb map (the class did not exist in feb);
// method names follow mp_transport.md.
//
// queue: packed records   { DPNID dpnid; int flags; int len /*bit31 = already sent*/; char data[len]; }
// out  : packet being built: records as  [hdr][data]  where
//          len <= 0x80 : hdr = 1 byte  0x80 | (len-1)
//          len >  0x80 : hdr = 2 bytes big-endian (len-1), bit15 clear   (=> max record 0x8000 bytes)
// ---------------------------------------------------------------------------------------------
struct netDP_QREC
{
    DPNID dpnid;                         // +0x00 target (client side: 0xFFFFFFFF)
    int   flags;                         // +0x04 netSEND_*
    int   len;                           // +0x08 bit31 set = consumed
    // char data[len];                   // +0x0C
};

struct netDP_PACKER
{
    arrVector<char> queue;               // +0x00 netDP_QREC stream
    arrVector<char> out;                 // +0x0C current packet

    netDP_QREC* NextRecord(netDP_QREC* prev);                    // v12 0x00597FD0
    int  PopPacket(DPNID* dpnid, int* flags, int* size, char** data, int maxSize); // v12 0x00598050
    int  AppendRecord(const void* data, int len, int maxSize);   // v12 0x00598130
};

// ---------------------------------------------------------------------------------------------
// netDP_SERVER
// ---------------------------------------------------------------------------------------------
class netDP_SERVER
{
public:
    netDP_SERVER(netDRV_DPUBI* drv);                             // v12 0x005982C0 (feb 0x0057D190)
    ~netDP_SERVER();                                             // v12 0x00598300 (feb 0x0057D1B0)

    void        Shutdown();                                      // v12 0x005983C0 (feb 0x0057D210)
    void        StartServer();                                   // v12 0x00598410 (feb 0x0057D250)
    const char* GetErrorDesc();                                  // v12 0x005986C0 (feb 0x0057D4C0)
    const char* GetStatusDesc();                                 // v12 0x00598810 (feb 0x0057D560)
    void        Flush();                                         // v12 0x00598830 1.2: NEW
    void        Send(DPNID to, const void* data, int len, int flags); // v12 0x00598900 (feb 0x0057D580)
    void        DisconnectPlayer(DPNID id);                      // v12 0x00598A40 (feb 0x0057D5F0)
    void        StopServer();                                    // v12 0x00598A70 (feb 0x0057D620)
    int         GetPing(DPNID id);                               // v12 0x00598AC0 1.2: NEW ? name
    int         GetPlayerPing(DPNID id) { return GetPing(id); }  // [integrator] network_dpubi.cpp name
    int         GetNumPlayers();                                 // v12 0x00598B00 1.2: NEW ? name

public:     // [integrator] was protected; netDRV_DPUBI reads these fields
    HRESULT     MsgHandler(DWORD msgId, void* msg);              // v12 0x00598B60 (feb 0x0057D660)
    static HRESULT WINAPI DpMsgHandler(void* ctx, DWORD msgId, void* msg); // v12 0x00598EA0 (feb 0x0057D7C0)

    netDRV_DPUBI*                     drv;           // +0x00 (feb +0x00)
    netDP_REF_PTR<IDirectPlay8Server> pServer;       // +0x04 (feb +0x04)
    int                               bStarted;      // +0x08 (feb +0x08) Host() succeeded
    union { HRESULT hrLast; HRESULT hrError; };      // +0x0C (feb +0x0C) [alias hrError: network_dpubi.cpp]
    const char*                       errContext;    // +0x10 (feb +0x10) "DPlayServer create: " ...
    netDP_PACKER                      packer;        // +0x14 1.2: NEW
    arrVector<void*>                  replyBuffers;  // +0x2C 1.2: NEW enum/connect reply buffers lent to DPlay
};                                                   // sizeof 0x38

// ---------------------------------------------------------------------------------------------
// netDP_CLIENT
// ---------------------------------------------------------------------------------------------
class netDP_CLIENT
{
public:
    netDP_CLIENT(netDRV_DPUBI* drv);                             // v12 0x00598EC0 (feb 0x0057D7E0)
    ~netDP_CLIENT();                                             // v12 0x00598F10 (feb 0x0057D820)

    void        Shutdown();                                      // v12 0x00598F80 (feb 0x0057D8E0)
    void        StartBrowser(const char** hosts, int nHosts);    // v12 0x00598FE0 (feb 0x0057D930)
    void        EnumHosts();                                     // v12 0x00599320 (feb 0x0057DC60, had const char* arg)
    void        StopBrowsing(int keepClient);                    // v12 0x005994A0 (feb 0x0057DE00)
    void        JoinGame(netDP_SERVER_DATA* sd);                 // v12 0x00599530 (feb 0x0057DE90, had const char* arg)
    const char* GetErrorDesc();                                  // v12 0x005995E0 (feb 0x0057DF20)
    const char* GetStatusDesc();                                 // v12 0x00599730 (feb 0x0057DFC0)
    void        Flush();                                         // v12 0x00599760 1.2: NEW
    void        Send(const void* data, int len, int flags);      // v12 0x00599830 (feb 0x0057DFF0)
    void        DisconnectFromGame();                            // v12 0x00599970 (feb 0x0057E060)
    int         IsServersEqual(netDP_SERVER_DATA* a, netDP_SERVER_DATA* b); // v12 0x005999B0 (feb 0x0057E090)
    void        OnServerDelete(netDP_SERVER_DATA* sd);           // v12 0x00599A00 (feb 0x0057E0C0)
    int         GetPing();                                       // v12 0x00599A60 1.2: NEW ? name

    static void __fastcall CharToWChar(const char* src, wrWCHAR* dst, int dstLen);   // [integrator] UTF-16 text type // v12 0x00599DA0 (feb 0x0057E410)

public:     // [integrator] was protected; netDRV_DPUBI reads these fields
    HRESULT     MsgHandler(DWORD msgId, void* msg);              // v12 0x00599A90 (feb 0x0057E120)
    static HRESULT WINAPI DpMsgHandler(void* ctx, DWORD msgId, void* msg); // v12 0x00599D80 (feb 0x0057E3F0)

    netDRV_DPUBI*                      drv;          // +0x00 (feb +0x00)
    netDP_REF_PTR<IDirectPlay8Client>  pClient;      // +0x04 (feb +0x04)
    union { int bInitialized; int bStarted; };       // +0x08 (feb +0x08) [aliases: network_dpubi.cpp]
    union { int bConnected; int bJoined; };          // +0x0C (feb +0x0C)
    union { HRESULT hrLast; HRESULT hrError; };      // +0x10 (feb +0x10)
    const char*                        errContext;   // +0x14 (feb +0x14)
    union { DPNHANDLE hEnum; int bEnumerating; };    // +0x18 (feb +0x18) pending EnumHosts (non-zero while enumerating)
    DPNHANDLE                          hConnect;     // +0x1C (feb +0x1C) pending Connect
    arrVector< netDP_REF_PTR<IDirectPlay8Address> > hosts; // +0x20 (feb +0x20) addresses to enumerate
    int                                curHost;      // +0x2C (feb +0x2C) round-robin index (not set by ctor)
    netDP_PACKER                       packer;       // +0x30 1.2: replaces feb arrVector<unsigned char> enumData (+0x30)
};                                                   // sizeof 0x48

// Template helpers emitted in this obj
//   arrVector<netDP_REF_PTR<IDirectPlay8Address>>::Resize(int)            v12 0x00599F00 (feb 0x0057E4E0)
//   arrVector<netDP_REF_PTR<IDirectPlay8Address>>::Insert(int, const&, int) v12 0x00599F80 (feb 0x0057E560)
//   arrVector<netDP_REF_PTR<IDirectPlay8Address>>::Reserve(int)           v12 0x0059A050 (feb 0x0057E630)
//   arrVector<char>::Insert(int pos, const char& v, int n)                v12 0x00599E70 1.2: NEW in this obj (used by AppendRecord)

// Globals
extern char netDP_ErrorBuf[];   // v12 0x009AF6F8 (feb 0x00964288) shared GetErrorDesc buffer
