// gs_mp_client.h -- Will Rock 1.2 (v12.exe) multiplayer: the game-level client (gsMP_CLIENT)
//
// Reconstructed for modders from static analysis. Original object: gs:gs_mp_client.obj.
// The two client filters (gsMP_CLIENT_FILTER_*) are documented here too; in the original they
// live in gs_mp_misc.obj (SNAPSHOT) and gs_mp_server.obj (CLIENT2).
//
// Types assumed from other files:
//   msgADDR (base, vtable v12 0x00606E98), msgDATA, msgSYSTEM msgSystem (v12 0x008FA960)
//   gsMP_COMM (gs_mp_misc.h)       -- base class: msgADDR fields +0x00..+0x67, gsMP_CONNECTION conn at +0x68
//   gsMP_CONNECTION, gsMSG_MP_WRAP (gs_mp_conn.h)
//   gsMP_SYSTEM*  gsSysMP      v12 0x0098E850   (mode at +0x28: 1 Single, 2 Server, 3 Client, 4 ServerOnly)
//   gsMP_CLIENT*  gsSysClient  v12 0x0098E860
//   gsMP_SERVER*  gsSysServer  v12 0x0098E870   (client table +0x2784, stride 0x54)
//   gsMP_NETWORK* gsSysNetwork v12 0x0099D814   (gsMP_STAT traffic counter at +0xC4)
//   gspPLAY_SYSTEM* gspSysPlay v12 0x009A01DC;  gsLVL_SYSTEM* gsSysLevel v12 0x0098E84C
//   scnSCENE* gsScenePtr v12 0x0098E81C;  wrUI_SYSTEM* uiSystem v12 0x00994568;  gsSTRINGS* gsStrings v12 0x009946DC
//   gsDEMO gsDemo v12 0x009A5C28;  msgADDR* gsDemoServer v12 0x009A8464
//   int gsAppState v12 0x0098E824;  float gsElapsedTime v12 0x0098E9F4
//   entENTITY / entIACTIVE_OBJ, animINST, animCREATE_DATA, m3dV, m3dMATR, apNAME
//   gsMSG_MP_SYNC, gsMSG_MP_SHOW_MSG, gsMSG_MP_CREATE_ENT, gsMSG_MP_CLIENT_READY (mp_messages.h)
//
// Entity fields used (v12 offsets; feb in brackets) -- 1.2 entity base is 4 bytes SMALLER here:
//   +0x14 int   classId                  [+0x14]
//   +0x20 int   magic '$TNE' 0x454E5424 = "is an entENTITY"   [+0x20]
//   +0x30 int   state (copied from sync)  [+0x30]
//   +0x40 apNAME name                    [+0x44]
//   +0x80 int   (1.2: set from gsMSG_MP_SYNC+0x18 in Single mode)   // ? meaning
//   +0x84 uint  mpFlags  (1 synced, 2 created-by-net, 4 local-owned, 0x800 got UPD_REGULAR)  [+0x88]
//   +0x88 int   mpid     (-3 = not yet synced)  [+0x8C]
//   +0x8C int   ownerClient                  [+0x90]
//   +0xC0 animINST* anim                     [+0xBC]
//   vtable +0x18 IdentifyMPSysSyncData(gsMSG_MP_SYNC*)   [+0x18]
//   vtable +0x58 OnMPSynchronized()  // ? name; feb slot +0x60

#pragma once
#include "wr_sdk.h"
#include "mp_messages.h"   // [integrator] dependency
#include "gs_mp_misc.h"   // [integrator] dependency
   // engine core types + subsystem classes (re/sdk); was "assumed from other files"
#include "gs_mp_conn.h"

// ------------------------------------------------------------------------------------------
// gsMP_CLIENT::flags (+0x2780, feb +0x470) -- set by gsMP_SYSTEM::InitGame(mode)
enum
{
    MPCL_LOCAL       = 0x01,   // Single player: messages go straight to entities
    MPCL_SERVER_HERE = 0x02,   // server runs in this process (listen / single): no network for client->server
    MPCL_DEMO        = 0x04,   // demo playback: messages go to gsDemoServer
    MPCL_SIMUL_PACK  = 0x08,   // debug: pack locally and deliver as GS_MSG_MP_SERVERCLIENT_SIMUL
    MPCL_NETWORKED   = 0x10,   // real network client (mode 3)
    MPCL_INIT_MAP    = 0x40,   // 1.2: broadcast GS_MSG_INIT_MAP after the next delayed level load
};

#define GS_MP_MAX_LOCAL_PLAYERS 16      // AddClient cap (cmp 0x10 at v12 0x00539259)

struct gsMP_CLIENT_PLAYER                 // 8 bytes
{
    entENTITY* ent;                       // +0
    int        active;                    // +4 1 after CLIENT_READY
};

// gsMP_CLIENT: v12 object size 0x281C, vtable 0x006084EC (feb 0x005DD014), instance *gsSysClient.
class gsMP_CLIENT : public gsMP_COMM
{
public:
    // --- gsMP_COMM / msgADDR part ---
    //   +0x0C  uint  addrFlags (|= 2 in InitGame)
    //   +0x10  'RDAM' 0x4D414452 (msgADDR tag)
    //   +0x1C  int   10    // ? msgADDR priority
    //   +0x68  gsMP_CONNECTION conn                     (1.2: 0x2718 bytes, feb 0x408)
    unsigned int       flags;          // +0x2780 (feb +0x470)  MPCL_*
    int                localClientId;  // +0x2784 (feb +0x474)  -3 none; -1 when the server is local
    float              connSpeedRate;  // +0x2788 1.2: [CONFIG] ConnSpeed -> 1.0/2.0/4.0 (feb +0x47C: fixed 0.05 s send interval)
    float              serverRate;     // +0x278C 1.2: value received in GS_MSG_MP_NOTIFY sub -200 (init 1.0)
    // feb only: +0x478 int sendNow, +0x47C float interval(0.05), +0x480 float timer -- removed in 1.2
    gsMP_CLIENT_PLAYER players[GS_MP_MAX_LOCAL_PLAYERS]; // +0x2790 (feb +0x484)
    int                nPlayers;       // +0x2810 (feb +0x504)
    int                iterPlayer;     // +0x2814 (feb +0x508)  GetNextPlayer cursor
    entENTITY*         localPlayer;    // +0x2818 (feb +0x50C)

    gsMP_CLIENT();                                                         // v12 0x00539100 (feb 0x00523730)
    virtual ~gsMP_CLIENT();                                                // slot +0x00  v12 0x0053F4C0 (feb 0x00529870)
    virtual int  ProcessMsg(int msg, void* data, msgADDR* from, int* res); // slot +0x04  v12 0x00539670 (feb 0x00523C20)
    // slots +0x08,+0x0C msgADDR::PostProcessMsg; +0x10 (gsMP_COMM Init, empty); +0x14 empty
    virtual int  InitGame(int flags);                                      // slot +0x18  v12 0x00539190 (feb 0x005237B0)
    virtual void TermGame();                                               // slot +0x1C  v12 0x00539220 (feb 0x00523820)
    virtual int  AddClient(entENTITY* ent);                                // slot +0x20  v12 0x00539250 (feb 0x00523850)
    int JoinGame(entENTITY* ent) { return AddClient(ent); }               // [integrator] gs_mp_misc.cpp name for slot +0x20
    virtual void RemoveClient(entENTITY* ent);                             // slot +0x24  v12 0x005392E0 (feb 0x005238E0)
    virtual void ActivateClient(unsigned long mpid);                       // slot +0x28  v12 0x00539350 (feb 0x00523950)
    virtual void SendMsgServer(entIACTIVE_OBJ* to, int msg, msgDATA* data, int netFlags); // slot +0x2C v12 0x005394D0 (feb 0x00523A80)
    virtual void ReceiveMsg(void* packet, int size);                       // slot +0x30  v12 0x00539D30 (feb 0x00524270)
    virtual void ShowMPMsg(gsMSG_MP_SHOW_MSG* m);                          // slot +0x34  v12 0x0053A050 (feb 0x00524560)

    entENTITY*      GetNextPlayer(entENTITY* prev);                        // v12 0x00539400 (feb 0x00523A00)
    int             GetNPlayer();                                          // v12 0x005394B0 (feb 0x00523A60)
    entIACTIVE_OBJ* Synchronize(gsMSG_MP_SYNC* sync);                      // v12 0x00539D90 (feb 0x005242D0)
};

// ------------------------------------------------------------------------------------------
// Client filters: used by gsMP_SERVER::SendMsgClientAll(ent, msg, data, netFlags, filter) to
// decide per client slot whether to send. Built on the stack in gsMP_SYSTEM's entity update.
class gsMP_CLIENT_FILTER
{
public:
    virtual int IsSend(int clientSlot) = 0;                                // slot +0x00
};

// Skip two client ids (usually the entity's owner and -1).
// v12 vtable 0x00607AF4 (feb 0x005DC600)
class gsMP_CLIENT_FILTER_CLIENT2 : public gsMP_CLIENT_FILTER
{
public:
    union { int exclude1; unsigned long skip1; };    // +0x04  [integrator] merged with gs_mp_server.h's copy
    union { int exclude2; unsigned long skip2; };    // +0x08
    gsMP_CLIENT_FILTER_CLIENT2() {}
    gsMP_CLIENT_FILTER_CLIENT2(unsigned long a, unsigned long b) : skip1(a), skip2(b) {}
    virtual int IsSend(int clientSlot);     // v12 0x00510760 (feb 0x004FDF90)
};

// Snapshot filter: CLIENT2 + per-client send rate.
// v12 vtable 0x00607D40 (feb 0x005DC604)
class gsMP_CLIENT_FILTER_SNAPSHOT : public gsMP_CLIENT_FILTER_CLIENT2
{
public:
    entIACTIVE_OBJ* ent;                    // +0x0C (filled by the caller)  // ? in feb not read by IsSend
    virtual int IsSend(int clientSlot);     // v12 0x00516410 (feb 0x004FC310)
};
