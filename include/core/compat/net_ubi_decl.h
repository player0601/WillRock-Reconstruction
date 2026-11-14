// net_ubi_decl.h -- declarations the reconstructed DirectPlay/Ubi.com driver (re/mpsrc/network_dpubi.cpp) calls.
// (Will Rock unofficial SDK, integrator)
//
// The Ubi.com lobby (netUBI_* wrappers over the libgs* Ubi.com SDK) is OUT OF SCOPE for this project and was not
// analysed. These are only the member names and argument shapes visible at the call sites in network_dpubi.cpp,
// with the v12 addresses written there, so that file can be syntax-checked. No layout is claimed (no data members).
// The Ubi.com service has been offline for many years; a port would drop this path and keep the LAN/IP DirectPlay
// path (or its replacement), see re/sdk/PORTING.md.
#pragma once
#ifdef _WIN32
#include <dplay8.h>
#else
#include "wr_win32_compat.h"
#endif

class netDRV_DPUBI;
#ifndef _WIN32
struct DPN_APPLICATION_DESC;
#endif

struct netUBI_NEW_SERVER {               // record handed to netDRV_DPUBI::Ubi_Client_OnServerFound (fields as used)
    char        name[64];                // +0x00
    int         maxPlayers;              // +0x40
    int         curPlayers;              // +0x44
    const unsigned char *dataBegin;      // +0x48 lobby game-info blob
    const unsigned char *dataEnd;        // +0x4C
    void       *ubiKey;                  // +0x50
    int         netVersion;              // +0x54 1.2
};

class netUBI_SERVERREG {                 // lobby registration of a hosted game, 0x16C bytes (ctor v12 0x00595BC0)
public:
    netUBI_SERVERREG(netDRV_DPUBI *drv, int ladder);
    void Shutdown();                     // v12 0x00595C80
    int  IsConnected();                  // v12 0x00595D90
    int  IsError();                      // v12 0x00595DA0
    void Think();                        // v12 0x00595DC0
    const char *GetStatusString();       // v12 0x00596160
    void NotifyClientLeave(const char *name); // v12 0x00596A60
    const char *GetErrorString();        // v12 0x00596A80
    void UpdateGameInfo();               // v12 0x00596EC0
    int  GetMatchStatus();               // v12 0x00596F90
    void RestartLadder();                // v12 0x00597020
    void StopLadder();                   // v12 0x00597050
    void StartMatchData();               // v12 0x00597080
    void SendMatchData(int key, int a, int b); // v12 0x005970D0
    void FinishMatchData();              // v12 0x00597110
};

class netUBI_CLIENT {                    // lobby browser / join, 0x138 bytes (ctor v12 0x005973D0)
public:
    netUBI_CLIENT(netDRV_DPUBI *drv);
    void Shutdown();                     // v12 0x00597520
    void Think();                        // v12 0x005975C0
    void EnumServers();                  // v12 0x005978B0
    void JoinGame(void *ubiKey);         // v12 0x00597930
    int  IsConnected();                  // v12 0x005979C0
    int  IsConnectError();               // v12 0x005979F0
    int  IsJoined();                     // v12 0x00597A10
    int  IsJoinError();                  // v12 0x00597A40
    const char *GetConnectErrorString(); // v12 0x00597A60
    const char *GetJoinErrorString();    // v12 0x00597A80
    const char *GetStatusString();       // v12 0x00597AA0
    void GetServerAddress(void *key, char *addr, char *altAddr, unsigned short *port); // v12 0x00597B00 (name from mpsrc)
    int  IsServersEqual(void *a, void *b);  // v12 0x00597B30
    void OnServerDelete(void *key);      // v12 0x00597B80
    void StopBrowsing(int keepList);     // v12 0x00597B90
    void DisconnectFromGame();           // v12 0x00597C10
    void NotifyServerJoined();           // v12 0x00597C50
};

// netDP_PINGER (1.2, DirectPlay based ping of lobby-listed servers, 0x7C bytes, ctor v12 0x00595340)
class netDP_PINGER {
public:
    netDP_PINGER();
    void Init(DPN_APPLICATION_DESC *desc, void *query); // v12 0x00595440 CoCreate DirectPlay8Client + address
    void StartPing(const char *addr, const char *altAddr, unsigned short port); // v12 0x00595550
    int  IsIdle();                       // v12 0x00595650 busy flag +0x6C == 0
    int  GetPing();                      // v12 0x00595660 +0x64 (9999 until an answer arrives)
};

void __fastcall InitializeSockets(char *err);   // v12 0x005A1A00 Ubi.com service (not reconstructed)
void __fastcall UninitializeSockets();          // Ubi.com service (not reconstructed)
