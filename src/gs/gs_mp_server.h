// gs_mp_server.h -- Will Rock 1.2 (v12.exe, 2003-07-30), reconstructed for modders.
//
// gsMP_SERVER: the game-level multiplayer server. It owns the client table (one
// record per connected DirectPlay player, 16 max), turns network packets into
// engine messages, creates the joining players' entities, and sends every
// server->client game message (SendMsgClient / SendMsgClientAll).
//
// Source object: gs:gs_mp_server.obj (feb 2003 reference build). Static analysis only.
// This file documents; it is not meant to compile into the game.
//
// Conventions
//   "// v12 0x........  (feb 0x........)"  function addresses in the two builds.
//   "// +0xNNNN (feb +0xNNN)"              member offsets in v12 / feb.
//   "// 1.2:"   differs from the feb build. Spot checks (ctor, InitGame, GetNextPlayer)
//               show most of these changes are already in 1.0 retail (jun.exe).
//   "// ?"      not resolved by static analysis.
//   Names of classes, methods and globals come from db/WillRock.map. Member and
//   local names are descriptive (no symbols exist for them).
//
// ---------------------------------------------------------------------------
// Types assumed from other files (not reconstructed here)
//   msgADDR, msgSYSTEM msgSystem (v12 0x008FA960), msgDATA        -- gs_msg / engine message bus
//       msgSYSTEM::SendMsg   v12 0x004D03C0 (dst, id, data, src)
//       msgSYSTEM::PostMsg   v12 0x004D0410 (dst, id, data)
//   gsMP_COMM (vtable v12 0x00607AE4), gsMP_CONNECTION (at this+0x68)  -- gs_mp_comm / gs_mp_conn
//       gsMP_CONNECTION::Pack 0x0053FC10, UnPack 0x0053FC80, GetBuffer 0x0053FCF0, GetBufferSize 0x0053FD00
//       gsMP_COMM::PrepareMsgWrap 0x0053FD10, PrepareConn 0x0053FD40
//   gsMP_SYSTEM  *gsSysMP      (v12 0x0098E850)  mode at +0x28: 1 Single, 2 Server, 3 Client, 4 ServerOnly
//       FindPlrLocal 0x0050E700, FindMPID 0x0050E6B0 (feb FindEnt), IsRegularMPID 0x0050E690,
//       SelectStartPosDir 0x0050DFA0 (1.2: 3 args), ConnSpeed factor 0x0050E5E0 (1.2, name ?)
//   gsMP_CLIENT  *gsSysClient  (v12 0x0098E860)
//   gsMP_SERVER  *gsSysServer  (v12 0x0098E870)
//   gsDEMO_CLIENT *gsDemoClient (v12 0x009A8460)
//   gsMP_NETWORK *gsSysNetwork (v12 0x0099D814)  Server_Stop 0x0052F3F0, Server_SendDataToClient 0x0052F420,
//       gsMP_STAT traffic counter at +0xC4 (AddTraffic 0x0052EAD0)
//   gsMP_CLIENT_FILTER_SNAPSHOT::IsSend v12 0x00516410 (gs_msg.cpp) -- reads client rec +0x4C/+0x50 and +0x2CD0
//   entIACTIVE_OBJ / entENTITY  (entity.cpp) v12 field offsets used here:
//       +0x14 class/type id, +0x30 ?, +0x40 apNAME name (feb +0x44), +0x80 ? (1.2, sent in sync),
//       +0x84 MP flags (feb +0x88), +0x88 MPID (feb +0x8C), +0x8C owner client id (feb +0x90),
//       +0xC0 animINST* (feb +0xBC)
//       vtable: +0x10 IsMPSynchableExisting, +0x14 GetMPSysSyncData, +0x1C (1.2) get UPD_REGULAR msg
//   entFindNextIO 0x0050C340, entFindNext 0x005086A0, entCreate 0x005082B0 / 0x00508450 (overload with type)
//   animINST::SetName 0x004CD500, animINST::MakeCreateData 0x004CDB80, animCREATE_DATA (ctor 0x004D1360)
//   apNAME::SetName 0x004AF580, m3dV, m3dMATR::_MakeLCS2WCS_VZ 0x00562A20
//   _apCfgReadInt 0x004AF7E0, sprintf 0x0057FFA9
//   globals: gsAppState 0x0098E824 (0x08000000 = -dedicated), gsScenePtr 0x0098E81C,
//            gsSceneFullName 0x0098E718, gsFrameCounter? 0x008FACC8 (feb 0x008B6A88, name ?)
// ---------------------------------------------------------------------------

#pragma once
#include "wr_sdk.h"
#include "gs_mp_conn.h"   // [integrator] dependency
#include "mp_messages.h"   // [integrator] dependency
#include "gs_mp_misc.h"   // [integrator] dependency
#include "gs_mp_client.h"   // [integrator] dependency
#include "gs_play.h"   // [integrator] dependency
   // engine core types + subsystem classes (re/sdk); was "assumed from other files"

// special client ids (DPNID values; descriptive names)
#define MPCLIENT_LOCAL   0xFFFFFFFF   // -1: the host's own client (listen server) / "to the local gsMP_CLIENT"
#define MPCLIENT_DEMO    0xFFFFFFFE   // -2: demo recorder (gsDemoClient)
#define MPCLIENT_NONE    0xFFFFFFFD   // -3: empty slot / entity not owned by a client

// mpid special values (entIACTIVE_OBJ +0x88)
#define MPID_UNDECLARED  0xFFFFFFFF   // -1: not yet declared syncable
#define MPID_WRAP_SYSTEM 0xFFFFFFFE   // -2: in a gsMSG_MP_WRAP: addressed to gsMP_SERVER itself, not an entity
#define MPID_CLIENTSIDE  0xFFFFFFFD   // -3: client mode: never synced from here

// gsMP_SERVER::flags (+0x2780), the argument of InitGame
#define MPSRV_LOCAL      0x01   // single player: the only "client" is the local one (slot 0)
#define MPSRV_HOSTPLAYER 0x02   // a local player exists on this machine (set with 0x01, or listen server)
#define MPSRV_SIMUL      0x08   // loop-back through gsMP_CONNECTION::Pack (simulate network)
#define MPSRV_CLIENTMODE 0x20   // gsMP_SYSTEM::InitGame(3 Client) passes 0x20: server object idle
// gsMP_SYSTEM::InitGame v12 0x0050D8B0 calls server->InitGame with: Single 1, Server 2, Client 0x20, ServerOnly(dedicated) 0

// SendMsgClient flags (passed to gsMP_NETWORK::Server_SendDataToClient)
#define MPSEND_GUARANTEED 0x20000   // DPNSEND_GUARANTEED
#define MPSEND_ONLYREADY  0x80000   // with 0x10000: skip clients whose record is not "ready" (0x90000 mask)

// v12 message ids used here (feb id in brackets; names from gsMsgInit v12 0x005107A0)
// [integrator] the same ids are #defined in gs_mp_misc.h / mp_messages.h / gs_play.h (same values)
#undef GS_MSG_ENT_DESTROY
#undef GS_MSG_FRAME
#undef GS_MSG_LOAD_LEVEL
#undef GS_MSG_MP_SERVERCLIENT
#undef GS_MSG_MP_SERVERCLIENT_SIMUL
#undef GS_MSG_MP_CREATE_PLAYER
#undef GS_MSG_MP_CREATE_ENT
#undef GS_MSG_MP_SYNC
#undef GS_MSG_MP_GET_SYNC_DATA
#undef GS_MSG_MP_CLIENT_READY
#undef GS_MSG_MP_SHOW_MSG
#undef GS_MSG_MP_UPD_REGULAR
#undef GS_MSG_MP_UPD_CLIENT
#undef GS_MSG_MP_UPD_CLIENT2
#undef GS_MSG_MP_NOTIFY
enum {
    GS_MSG_ENT_DESTROY           = 3,      // [3]     entity is being deleted (sent by the entity)
    // GS_MSG_FRAME                 = 0x3E8,  // [0x3E8] per-frame tick  [integrator: defined in sdk/game_systems/gs_common.h, same value]
    // GS_MSG_LOAD_LEVEL            = 0x3EC,  // [0x3EC]  [integrator: defined in sdk/game_systems/gs_common.h, same value]
    GS_MSG_MP_SERVERCLIENT       = 0x400,  // [0x3FF] unpacked gsMSG_MP_WRAP
    GS_MSG_MP_SERVERCLIENT_SIMUL = 0x401,  // [0x400] packed buffer (loop-back / raw)
    GS_MSG_MP_CREATE_PLAYER      = 0x402,  // [0x401] join: create my player
    GS_MSG_MP_CREATE_ENT         = 0x403,  // [0x402]
    GS_MSG_MP_SYNC               = 0x404,  // [0x403]
    GS_MSG_MP_GET_SYNC_DATA      = 0x405,  // [0x404]
    GS_MSG_MP_CLIENT_READY       = 0x406,  // [0x405]
    GS_MSG_MP_SHOW_MSG           = 0x40A,  // [0x409]
    GS_MSG_MP_UPD_REGULAR        = 0x40B,  // [0x40A GS_MSG_MP_UPD_POS]
    GS_MSG_MP_UPD_CLIENT         = 0x40C,  // [0x40B] client->server update, rewritten to 0x40B (name ?)
    GS_MSG_MP_UPD_CLIENT2        = 0x40D,  // [none]  1.2: same, extra entity flags 0xA00 (name ?)
    // GS_MSG_MP_NOTIFY             = 0x40E,  // [0x40C] gsEV_SYNC carrier, s16 subcode at +4  [integrator: defined in sdk/game_systems/gs_common.h, same value]
};

// gsEV_SYNC / GS_MSG_MP_NOTIFY subcodes (s16 at +4)
#define MPNOTIFY_SYNC_DEFAULT  (-100)  // gsEV_SYNC ctor default (v12 0x0050E890)
#define MPNOTIFY_RATE          (-200)  // 0xFF38: client<->server snapshot rate (float at +6)

#pragma pack(push, 1)

// Engine message header (all msgDATA start with it)
#if 0 // [integrator] core type (engine_core/core_types.h): type/flags/size are member aliases there
struct msgDATA {
    unsigned char  type;    // +0  copied from a per-struct global id byte
    unsigned char  flags;   // +1  0x40
    unsigned short size;    // +2
};
#endif


// what travels inside the network packet (v12 ctor inline: {-1, -1, -3, 0})
#if 0 // [integrator] defined in gs_mp_conn.h
struct gsMSG_MP_WRAP {
    unsigned long  mpid;     // +0x0  target entity MPID, or MPID_WRAP_SYSTEM (-2)
    short          msg;      // +0x4  message id
    unsigned long  client;   // +0x6  sender client id (DPNID), -1 local
    msgDATA       *data;     // +0xA
};
#endif
                           // 0xE

// GS_MSG_MP_NOTIFY payload
#if 0 // [integrator] merged into gs_mp_misc.h
struct gsEV_SYNC : msgDATA { // size 0x0A, ctor v12 0x0050E890
    short code;              // +0x4
    union {                  // +0x6
        msgDATA *pData;      // GET_SYNC_DATA result
        float    fRate;      // code -200
    };
};
#endif


// GS_MSG_MP_CREATE_PLAYER / GS_MSG_MP_CREATE_ENT payload (ctor v12 0x0050E8C0, size 0x164)
#if 0 // [integrator] merged into mp_messages.h
struct gsMSG_MP_CREATE_ENT : msgDATA {
    void           *user;           // +0x04 passed to entCreate as user data
    unsigned long   mpid;           // +0x08 filled by the server
    char            name[0x40];     // +0x0C player name (CREATE_PLAYER) / entity name
    char            className[0x40];// +0x4C
    char            kind[0x40];     // +0x8C
    short           type;           // +0xCC (1.2: s16; feb read an int)
    animCREATE_DATA create;         // +0xCE (start matrix at +0xE0 = create+0x12) (feb +0xD0 / +0xE2)
};
#endif


// GS_MSG_MP_SYNC payload (size 0x176; feb 0x172)
#if 0 // [integrator] merged into mp_messages.h ("create" is "createData" there)
struct gsMSG_MP_SYNC : msgDATA {
    msgDATA        *userData;       // +0x04
    unsigned long   mpid;           // +0x08
    unsigned long   classId;        // +0x0C entity +0x14; -1 = end of entity list, -2 = sync complete (SyncClient)
    unsigned long   unk10;          // +0x10 ?
    unsigned long   ent30;          // +0x14 entity +0x30 ?
    unsigned long   ent80;          // +0x18 1.2: entity +0x80 ?
    char            str1[0x40];     // +0x1C always ""
    char            str2[0x40];     // +0x5C always ""
    char            name[0x40];     // +0x9C entity name (ent+0x40)
    long            unkDC;          // +0xDC -1
    animCREATE_DATA create;         // +0xE0
};
#endif


// GS_MSG_MP_CLIENT_READY payload (0x0C)
#if 0 // [integrator] defined in mp_messages.h
struct gsMSG_MP_CLIENT_READY : msgDATA {
    unsigned long mpid;             // +0x4 player MPID or -1
    int           ready;            // +0x8 1 ready / 0 deactivated
};
#endif


// GS_MSG_LOAD_LEVEL payload as sent to a joining client (0x154)
#if 0 // [integrator] merged into gs_play.h
struct gsMSG_LOAD_LEVEL : msgDATA {
    char  level[0x104];             // +0x004 gsSceneFullName
    char  b108;                     // +0x108 0
    char  pad[0x3F];
    int   d148, d14C, d150;         // +0x148.. 0
};
#endif

#pragma pack(pop)

// One record per connected client. v12 stride 0x54 (feb 0x58).
struct gsMP_CLIENT_REC {
    unsigned long  id;          // +0x00 DPNID, MPCLIENT_LOCAL for the host's own player, MPCLIENT_NONE = free
    entENTITY     *player;      // +0x04 the client's player entity
    apNAME         name;        // +0x08 player name (0x40)
    int            ready;       // +0x48 set by GS_MSG_MP_CLIENT_READY, cleared by DeActivateClientAll
    int            readyFrame;  // +0x4C 1.2: frame counter (0x008FACC8) at CLIENT_READY (feb: +0x4C "send now" flag)
    float          rate;        // +0x50 snapshot rate/period sent by the client (NOTIFY -200); default 1.0 (feb 1/30)
                                //       feb had +0x54 countdown timer driven by GS_MSG_FRAME: removed in 1.2
};

// client filters: IsSend(clientIndex) decides per record
#if 0 // [integrator] merged into gs_mp_client.h
struct gsMP_CLIENT_FILTER {
    virtual int IsSend(int i) = 0;                    // slot +0x00
};
#endif

#if 0 // [integrator] merged into gs_mp_client.h
struct gsMP_CLIENT_FILTER_CLIENT2 : gsMP_CLIENT_FILTER {  // vtable v12 0x00607AF4
    unsigned long skip1;                              // +0x4
    unsigned long skip2;                              // +0x8
    gsMP_CLIENT_FILTER_CLIENT2(unsigned long a, unsigned long b) : skip1(a), skip2(b) {}
    virtual int IsSend(int i);                        // v12 0x00510760
};
#endif


#define MPSRV_MAX_CLIENTS 16

class gsMP_SERVER : public gsMP_COMM   // vtable v12 0x00607AB4 (feb 0x005DC610), sizeof 0x2CD4 (feb 0xA00)
{
public:
    // gsMP_COMM / msgADDR part: +0x00..+0x67, gsMP_CONNECTION conn at +0x68 (grew in 1.2: feb ended at +0x470)
    unsigned long    flags;                        // +0x2780 (feb +0x470)  MPSRV_*
    gsMP_CLIENT_REC  clients[MPSRV_MAX_CLIENTS];   // +0x2784 (feb +0x474)  stride 0x54 (feb 0x58)
    int              nClients;                     // +0x2CC4 (feb +0x9F4)
    int              iterPlayer;                   // +0x2CC8 (feb +0x9F8)  GetNextPlayer cursor
    unsigned long    nextMPID;                     // +0x2CCC (feb +0x9FC)
    float            connSpeed;                    // +0x2CD0 1.2: [CONFIG] ConnSpeed factor 1/2/4

    gsMP_SERVER();                                 // v12 0x0050F140
    ~gsMP_SERVER();                                // v12 0x0050F230

    // virtuals, vtable order
    virtual void *__vecDelDtor(unsigned int f);    // slot +0x00 v12 0x0050F1D0
    virtual int  ProcessMsg(int msg, void *data, msgADDR *from, int *ret); // slot +0x04 v12 0x0050FAB0
    //                                             // slot +0x08/+0x0C msgADDR::PreProcessMsg/PostProcessMsg (inherited)
    virtual int  Init();                           // slot +0x10 v12 0x005102C0
    //                                             // slot +0x14 Term (shared empty fn v12 0x00538270)
    virtual int  InitGame(int flags);              // slot +0x18 v12 0x005102E0
    virtual void TermGame();                       // slot +0x1C v12 0x00510380
    virtual int  AddClient(unsigned long id);      // slot +0x20 v12 0x0050F250
    int JoinGame(unsigned long id) { return AddClient(id); }   // [integrator] gs_mp_misc.cpp name for slot +0x20
    virtual void RemoveClient(unsigned long id);   // slot +0x24 v12 0x0050F290
    virtual void AddPlayer(entENTITY *ent, unsigned long id);   // slot +0x28 v12 0x0050F3B0
    virtual void SendMsgClient(unsigned long id, entIACTIVE_OBJ *ent, int msg, msgDATA *data, int flags); // slot +0x2C v12 0x00510420
    //                                             // slot +0x30 gsMP_COMM scalar deleting dtor (v12 0x0050F200), +0x34..+0x3C inherited

    entENTITY *GetNextPlayer(entENTITY *prev);     // v12 0x0050F440
    int        GetNPlayer();                       // v12 0x0050F530
    entENTITY *GetPlayer(int i);                   // v12 0x0050F550
    void       DeclSyncable(entIACTIVE_OBJ *o);    // v12 0x0050F5A0
    void       UnDeclSyncable(entIACTIVE_OBJ *o);  // v12 0x0050F600
    void       NotifyNewMPID(unsigned long mpid);  // v12 0x0050F620
    int        CreateSyncInfo(entIACTIVE_OBJ *o, gsMSG_MP_SYNC *m);   // v12 0x0050F640
    msgDATA   *CreateSyncUsrInfo(entIACTIVE_OBJ *o);                  // v12 0x0050F6D0
    void       SyncClient(entENTITY *player);      // v12 0x0050F710
    void       DeActivateClientAll();              // v12 0x0050FA30
    entENTITY *CreateEntitySyncable(gsMSG_MP_CREATE_ENT *m);          // v12 0x00510160
    void       SendMsgClient(entENTITY *ent, int msg, msgDATA *data, int flags);              // v12 0x005103C0
    void       SendMsgClientAll(entIACTIVE_OBJ *ent, int msg, msgDATA *data, int flags,
                                unsigned long skip1, unsigned long skip2);                   // v12 0x00510580
    void       SendMsgClientAll(entIACTIVE_OBJ *ent, int msg, msgDATA *data, int flags,
                                gsMP_CLIENT_FILTER *filter);                                  // v12 0x00510660
    void       ReceiveMsg(unsigned long client, void *buf, int size);                         // v12 0x005106F0

private:
    int FindClient(unsigned long id);   // not a real function: the search loop is inlined at every use
};

extern gsMP_SERVER *gsSysServer;   // v12 0x0098E870 (feb 0x00948274)
