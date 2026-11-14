// wr_win32_compat.h -- minimal Win32 + DirectPlay 8 declarations (Will Rock unofficial SDK, integrator)
//
// PURPOSE: lets the reconstructed multiplayer sources (re/mpsrc/network*.cpp) be syntax-checked on a machine without
// the Windows / DirectX SDK, and lists in ONE place every Win32/DirectPlay symbol the network layer uses: this is the
// surface a port must replace (DirectPlay8 -> ENet / raw UDP / SDL_net; mutexes -> std::mutex / SDL_mutex).
// It is used ONLY when _WIN32 is not defined. On Windows the real <windows.h>, <dplay8.h>, <dpaddr.h> are used.
// Interfaces are declared in the real COM vtable order (IUnknown first). Numeric constants are the DirectX 8.1/9 SDK
// values as far as known; any marked "// ?" must be taken from the real SDK before relying on them.
#pragma once
#include "../wr_cc.h"   // __stdcall etc. for GCC/Clang
#if !defined(_WIN32)
#include <stddef.h>
#include <stdint.h>
#include <wchar.h>
#include <string.h>

// ---------------------------------------------------------------- Win32 base types
typedef int            BOOL;
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef uint32_t       DWORD;      // 32-bit like on Win32
typedef int32_t        LONG;
typedef uint32_t       ULONG;
typedef unsigned int   UINT;
typedef int32_t        HRESULT;
typedef wchar_t        WCHAR;      // NOTE: 4 bytes on Linux; the game's strings are UTF-16 (use char16_t in a port)
typedef void          *HANDLE;
typedef void          *HWND;
typedef void          *HINSTANCE;
typedef void          *LPVOID, *PVOID;
typedef const char    *LPCSTR;
typedef char          *LPSTR;
typedef const WCHAR   *LPCWSTR;
typedef WCHAR         *LPWSTR;
typedef uintptr_t      UINT_PTR, DWORD_PTR;
#ifndef WINAPI
#  define WINAPI __stdcall
#endif
#ifndef CALLBACK
#  define CALLBACK __stdcall
#endif
#ifndef TRUE
#  define TRUE  1
#  define FALSE 0
#endif
#define INFINITE        0xFFFFFFFFu
#define WAIT_OBJECT_0   0x00000000u
#define WAIT_TIMEOUT    0x00000102u
#define WAIT_FAILED     0xFFFFFFFFu
#define S_OK            ((HRESULT)0)
#define S_FALSE         ((HRESULT)1)
#define E_FAIL          ((HRESULT)0x80004005)
#define E_OUTOFMEMORY   ((HRESULT)0x8007000E)
#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#define FAILED(hr)      (((HRESULT)(hr)) < 0)
#define CLSCTX_INPROC_SERVER 0x1

struct GUID { uint32_t Data1; uint16_t Data2, Data3; uint8_t Data4[8]; };
typedef GUID IID, CLSID;
typedef const GUID &REFGUID, &REFIID, &REFCLSID;
inline bool operator==(const GUID &a, const GUID &b) { return memcmp(&a, &b, sizeof(GUID)) == 0; }

// Win32 functions used by drv:network*.obj (v12 imports; a port maps them as noted)
HANDLE WINAPI CreateMutexA(void *sec, BOOL initialOwner, LPCSTR name);   // -> std::mutex / SDL_CreateMutex
DWORD  WINAPI WaitForSingleObject(HANDLE h, DWORD ms);                   // -> lock (with timeout)
BOOL   WINAPI ReleaseMutex(HANDLE h);                                    // -> unlock
BOOL   WINAPI CloseHandle(HANDLE h);
void   WINAPI Sleep(DWORD ms);                                           // -> SDL_Delay / nanosleep
DWORD  WINAPI GetTickCount();                                            // -> SDL_GetTicks
DWORD  WINAPI timeGetTime();
int    WINAPI MultiByteToWideChar(UINT cp, DWORD fl, LPCSTR s, int n, LPWSTR d, int dn);
int    WINAPI WideCharToMultiByte(UINT cp, DWORD fl, LPCWSTR s, int n, LPSTR d, int dn, LPCSTR def, BOOL *used);
HRESULT WINAPI CoInitialize(void *);
void    WINAPI CoUninitialize();
HRESULT WINAPI CoCreateInstance(REFCLSID clsid, void *outer, DWORD ctx, REFIID iid, void **out);
#define CP_ACP 0
const char *WINAPI DXGetErrorString8A(HRESULT hr);                       // dxerr8.lib (DirectX 8 SDK) error text

struct IUnknown {
    virtual HRESULT WINAPI QueryInterface(REFIID, void **) = 0;
    virtual ULONG   WINAPI AddRef() = 0;
    virtual ULONG   WINAPI Release() = 0;
};

// ---------------------------------------------------------------- DirectPlay 8 (dplay8.h / dpaddr.h)
typedef DWORD  DPNID;
typedef DWORD  DPNHANDLE;
typedef HRESULT (WINAPI *PFNDPNMESSAGEHANDLER)(void *ctx, DWORD msgId, void *msg);

#define DPN_OK                          S_OK
#define DPNSUCCESS_EQUAL                ((HRESULT)0x005E0001)      // ?
#define DPNERR_GENERIC                  E_FAIL
#define DPNERR_SESSIONFULL              ((HRESULT)0x80158500)      // ? MAKE_DPNHRESULT(0x500)
#define DPNID_ALL_PLAYERS_GROUP         0
#define DPN_MSGID_OFFSET                0xFFFF0000u
#define DPN_MSGID_ASYNC_OP_COMPLETE     (DPN_MSGID_OFFSET | 0x0003)
#define DPN_MSGID_CONNECT_COMPLETE      (DPN_MSGID_OFFSET | 0x0005)
#define DPN_MSGID_CREATE_PLAYER         (DPN_MSGID_OFFSET | 0x0007)
#define DPN_MSGID_DESTROY_PLAYER        (DPN_MSGID_OFFSET | 0x0009)
#define DPN_MSGID_ENUM_HOSTS_QUERY      (DPN_MSGID_OFFSET | 0x000A)
#define DPN_MSGID_ENUM_HOSTS_RESPONSE   (DPN_MSGID_OFFSET | 0x000B)
#define DPN_MSGID_INDICATE_CONNECT      (DPN_MSGID_OFFSET | 0x000E)
#define DPN_MSGID_RECEIVE               (DPN_MSGID_OFFSET | 0x0011)
#define DPN_MSGID_RETURN_BUFFER         (DPN_MSGID_OFFSET | 0x0013)
#define DPN_MSGID_SEND_COMPLETE         (DPN_MSGID_OFFSET | 0x0014)
#define DPN_MSGID_TERMINATE_SESSION     (DPN_MSGID_OFFSET | 0x0016)
#define DPNSEND_NOCOPY                  0x0001
#define DPNSEND_NOCOMPLETE              0x0002
#define DPNSEND_COMPLETEONPROCESS       0x0004
#define DPNSEND_GUARANTEED              0x0008
#define DPNSEND_NONSEQUENTIAL           0x0010
#define DPNSEND_NOLOOPBACK              0x0020
#define DPNSEND_PRIORITY_LOW            0x0040
#define DPNSEND_PRIORITY_HIGH           0x0080
#define DPNCLOSE_IMMEDIATE              0x00000001
#define DPNCANCEL_CONNECT               0x0001
#define DPNCANCEL_ENUM                  0x0002
#define DPNCANCEL_SEND                  0x0004
#define DPNCANCEL_ALL_OPERATIONS        0x8000
#define DPNSESSION_CLIENT_SERVER        0x0001
#define DPNSESSION_MIGRATE_HOST         0x0004
#define DPNINITIALIZE_DISABLEPARAMVAL   0x0001
#define DPNINITIALIZE_DISABLELINKTUNING 0x0002
#define DPNA_DATATYPE_STRING            0x00000001
#define DPNA_DATATYPE_DWORD             0x00000002
#define DPNA_DATATYPE_GUID              0x00000003
#define DPNA_DATATYPE_BINARY            0x00000004
#define DPNA_DATATYPE_STRING_ANSI       0x00000005
#define DPNA_KEY_HOSTNAME               L"hostname"
#define DPNA_KEY_PORT                   L"port"

extern const GUID CLSID_DirectPlay8Client, CLSID_DirectPlay8Server, CLSID_DirectPlay8Address, CLSID_DP8SP_TCPIP;
extern const GUID IID_IDirectPlay8Client, IID_IDirectPlay8Server, IID_IDirectPlay8Address;

struct DPN_APPLICATION_DESC {
    DWORD dwSize, dwFlags;
    GUID  guidInstance, guidApplication;
    DWORD dwMaxPlayers, dwCurrentPlayers;
    WCHAR *pwszSessionName, *pwszPassword;
    void  *pvReservedData;      DWORD dwReservedDataSize;
    void  *pvApplicationReservedData; DWORD dwApplicationReservedDataSize;
};
struct DPN_BUFFER_DESC { DWORD dwBufferSize; BYTE *pBufferData; };
struct DPN_CONNECTION_INFO {
    DWORD dwSize, dwRoundTripLatencyMS, dwThroughputBPS, dwPeakThroughputBPS;
    DWORD dwBytesSentGuaranteed, dwPacketsSentGuaranteed, dwBytesSentNonGuaranteed, dwPacketsSentNonGuaranteed;
    DWORD dwBytesRetried, dwPacketsRetried, dwBytesDropped, dwPacketsDropped;
    DWORD dwMessagesTransmittedHighPriority, dwMessagesTimedOutHighPriority;
    DWORD dwMessagesTransmittedNormalPriority, dwMessagesTimedOutNormalPriority;
    DWORD dwMessagesTransmittedLowPriority, dwMessagesTimedOutLowPriority;
    DWORD dwBytesReceivedGuaranteed, dwPacketsReceivedGuaranteed, dwBytesReceivedNonGuaranteed, dwPacketsReceivedNonGuaranteed;
    DWORD dwMessagesReceived;
};
struct IDirectPlay8Address;
struct DPNMSG_ASYNC_OP_COMPLETE  { DWORD dwSize; DPNHANDLE hAsyncOp; void *pvUserContext; HRESULT hResultCode; };
struct DPNMSG_CONNECT_COMPLETE   { DWORD dwSize; DPNHANDLE hAsyncOp; void *pvUserContext; HRESULT hResultCode;
                                   void *pvApplicationReplyData; DWORD dwApplicationReplyDataSize; };
struct DPNMSG_CREATE_PLAYER      { DWORD dwSize; DPNID dpnidPlayer; void *pvPlayerContext; };
struct DPNMSG_DESTROY_PLAYER     { DWORD dwSize; DPNID dpnidPlayer; void *pvPlayerContext; DWORD dwReason; };
struct DPNMSG_ENUM_HOSTS_QUERY   { DWORD dwSize; IDirectPlay8Address *pAddressSender, *pAddressDevice;
                                   void *pvReceivedData; DWORD dwReceivedDataSize; DWORD dwMaxResponseDataSize;
                                   void *pvResponseData; DWORD dwResponseDataSize; void *pvResponseContext; };
struct DPNMSG_ENUM_HOSTS_RESPONSE{ DWORD dwSize; IDirectPlay8Address *pAddressSender, *pAddressDevice;
                                   const DPN_APPLICATION_DESC *pApplicationDescription;
                                   void *pvResponseData; DWORD dwResponseDataSize; void *pvUserContext; DWORD dwRoundTripLatencyMS; };
struct DPNMSG_INDICATE_CONNECT   { DWORD dwSize; void *pvUserConnectData; DWORD dwUserConnectDataSize;
                                   void *pvReplyData; DWORD dwReplyDataSize; void *pvReplyContext; void *pvPlayerContext;
                                   IDirectPlay8Address *pAddressPlayer, *pAddressDevice; };
struct DPNMSG_RECEIVE            { DWORD dwSize; DPNID dpnidSender; void *pvPlayerContext;
                                   BYTE *pReceiveData; DWORD dwReceiveDataSize; DPNHANDLE hBufferHandle; };
struct DPNMSG_RETURN_BUFFER      { DWORD dwSize; HRESULT hResultCode; void *pvBuffer; void *pvUserContext; };
struct DPNMSG_TERMINATE_SESSION  { DWORD dwSize; HRESULT hResultCode; void *pvTerminateData; DWORD dwTerminateDataSize; };

struct IDirectPlay8Address : IUnknown {                                  // dpaddr.h, vtable order
    virtual HRESULT WINAPI BuildFromURLW(WCHAR *) = 0;                   // +0x0C
    virtual HRESULT WINAPI BuildFromURLA(char *) = 0;                    // +0x10
    virtual HRESULT WINAPI Duplicate(IDirectPlay8Address **) = 0;        // +0x14
    virtual HRESULT WINAPI SetEqual(IDirectPlay8Address *) = 0;          // +0x18
    virtual HRESULT WINAPI IsEqual(IDirectPlay8Address *) = 0;           // +0x1C
    virtual HRESULT WINAPI Clear() = 0;                                  // +0x20
    virtual HRESULT WINAPI GetURLW(WCHAR *, DWORD *) = 0;                // +0x24
    virtual HRESULT WINAPI GetURLA(char *, DWORD *) = 0;                 // +0x28
    virtual HRESULT WINAPI GetSP(GUID *) = 0;                            // +0x2C
    virtual HRESULT WINAPI GetUserData(void *, DWORD *) = 0;             // +0x30
    virtual HRESULT WINAPI SetSP(const GUID *) = 0;                      // +0x34  (used, vt+0x34 in the game)
    virtual HRESULT WINAPI SetUserData(const void *, DWORD) = 0;         // +0x38
    virtual HRESULT WINAPI GetNumComponents(DWORD *) = 0;                // +0x3C
    virtual HRESULT WINAPI GetComponentByName(const WCHAR *, void *, DWORD *, DWORD *) = 0;            // +0x40
    virtual HRESULT WINAPI GetComponentByIndex(DWORD, WCHAR *, DWORD *, void *, DWORD *, DWORD *) = 0; // +0x44
    virtual HRESULT WINAPI AddComponent(const WCHAR *, const void *, DWORD, DWORD) = 0;               // +0x48 (used)
    virtual HRESULT WINAPI GetDevice(GUID *) = 0;
    virtual HRESULT WINAPI SetDevice(const GUID *) = 0;
    virtual HRESULT WINAPI BuildFromDPADDRESS(void *, DWORD) = 0;
};

struct IDirectPlay8Client : IUnknown {                                   // dplay8.h, vtable order
    virtual HRESULT WINAPI Initialize(void *ctx, PFNDPNMESSAGEHANDLER h, DWORD flags) = 0;              // +0x0C
    virtual HRESULT WINAPI EnumServiceProviders(const GUID *, const GUID *, void *, DWORD *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI EnumHosts(DPN_APPLICATION_DESC *, IDirectPlay8Address *host, IDirectPlay8Address *dev,
                                     void *data, DWORD size, DWORD count, DWORD retry, DWORD timeout,
                                     void *ctx, DPNHANDLE *h, DWORD flags) = 0;                           // +0x14
    virtual HRESULT WINAPI CancelAsyncOperation(DPNHANDLE h, DWORD flags) = 0;                          // +0x18
    virtual HRESULT WINAPI Connect(const DPN_APPLICATION_DESC *, IDirectPlay8Address *host, IDirectPlay8Address *dev,
                                   const void *sec, const void *cred, const void *data, DWORD size,
                                   void *ctx, DPNHANDLE *h, DWORD flags) = 0;                             // +0x1C
    virtual HRESULT WINAPI Send(const DPN_BUFFER_DESC *, DWORD n, DWORD timeout, void *ctx, DPNHANDLE *h, DWORD flags) = 0; // +0x20
    virtual HRESULT WINAPI GetSendQueueInfo(DWORD *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI GetApplicationDesc(DPN_APPLICATION_DESC *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI SetClientInfo(const void *, void *, DPNHANDLE *, DWORD) = 0;
    virtual HRESULT WINAPI GetServerInfo(void *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI GetServerAddress(IDirectPlay8Address **, DWORD) = 0;
    virtual HRESULT WINAPI Close(DWORD flags) = 0;                                                      // +0x38
    virtual HRESULT WINAPI ReturnBuffer(DPNHANDLE, DWORD) = 0;
    virtual HRESULT WINAPI GetCaps(void *, DWORD) = 0;
    virtual HRESULT WINAPI SetCaps(const void *, DWORD) = 0;
    virtual HRESULT WINAPI SetSPCaps(const GUID *, const void *, DWORD) = 0;
    virtual HRESULT WINAPI GetSPCaps(const GUID *, void *, DWORD) = 0;
    virtual HRESULT WINAPI GetConnectionInfo(DPN_CONNECTION_INFO *, DWORD) = 0;                          // +0x50
    virtual HRESULT WINAPI RegisterLobby(DPNHANDLE, void *, DWORD) = 0;
};

struct IDirectPlay8Server : IUnknown {                                   // dplay8.h, vtable order
    virtual HRESULT WINAPI Initialize(void *ctx, PFNDPNMESSAGEHANDLER h, DWORD flags) = 0;              // +0x0C
    virtual HRESULT WINAPI EnumServiceProviders(const GUID *, const GUID *, void *, DWORD *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI CancelAsyncOperation(DPNHANDLE h, DWORD flags) = 0;
    virtual HRESULT WINAPI GetSendQueueInfo(DPNID, DWORD *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI GetApplicationDesc(DPN_APPLICATION_DESC *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI SetServerInfo(const void *, void *, DPNHANDLE *, DWORD) = 0;
    virtual HRESULT WINAPI GetClientInfo(DPNID, void *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI GetClientAddress(DPNID, IDirectPlay8Address **, DWORD) = 0;
    virtual HRESULT WINAPI GetLocalHostAddresses(IDirectPlay8Address **, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI SetApplicationDesc(const DPN_APPLICATION_DESC *, DWORD) = 0;
    virtual HRESULT WINAPI Host(const DPN_APPLICATION_DESC *, IDirectPlay8Address **dev, DWORD n,
                                const void *sec, const void *cred, void *ctx, DWORD flags) = 0;          // +0x34
    virtual HRESULT WINAPI SendTo(DPNID, const DPN_BUFFER_DESC *, DWORD n, DWORD timeout, void *ctx, DPNHANDLE *h, DWORD flags) = 0;
    virtual HRESULT WINAPI CreateGroup(const void *, void *, void *, DPNHANDLE *, DWORD) = 0;
    virtual HRESULT WINAPI DestroyGroup(DPNID, void *, DPNHANDLE *, DWORD) = 0;
    virtual HRESULT WINAPI AddPlayerToGroup(DPNID, DPNID, void *, DPNHANDLE *, DWORD) = 0;
    virtual HRESULT WINAPI RemovePlayerFromGroup(DPNID, DPNID, void *, DPNHANDLE *, DWORD) = 0;
    virtual HRESULT WINAPI SetGroupInfo(DPNID, void *, void *, DPNHANDLE *, DWORD) = 0;
    virtual HRESULT WINAPI GetGroupInfo(DPNID, void *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI EnumPlayersAndGroups(DPNID *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI EnumGroupMembers(DPNID, DPNID *, DWORD *, DWORD) = 0;
    virtual HRESULT WINAPI Close(DWORD flags) = 0;
    virtual HRESULT WINAPI DestroyClient(DPNID, const void *, DWORD, DWORD) = 0;
    virtual HRESULT WINAPI ReturnBuffer(DPNHANDLE, DWORD) = 0;
    virtual HRESULT WINAPI GetPlayerContext(DPNID, void **, DWORD) = 0;
    virtual HRESULT WINAPI GetGroupContext(DPNID, void **, DWORD) = 0;
    virtual HRESULT WINAPI GetCaps(void *, DWORD) = 0;
    virtual HRESULT WINAPI SetCaps(const void *, DWORD) = 0;
    virtual HRESULT WINAPI SetSPCaps(const GUID *, const void *, DWORD) = 0;
    virtual HRESULT WINAPI GetSPCaps(const GUID *, void *, DWORD) = 0;
    virtual HRESULT WINAPI GetConnectionInfo(DPNID, DPN_CONNECTION_INFO *, DWORD) = 0;
    virtual HRESULT WINAPI RegisterLobby(DPNHANDLE, void *, DWORD) = 0;
};
#endif // !_WIN32
