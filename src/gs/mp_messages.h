// mp_messages.h -- Will Rock 1.2 (v12.exe): catalogue of game / multiplayer message ids
//
// Documentation for modders (static analysis). All ids go through msgSYSTEM (v12 0x008FA960):
//   SendMsg(dst, id, data, src) v12 0x004D03C0, SendAll(id, data, src) 0x004D06F0,
//   PostMsg 0x004D0410, PostTimeMsgData(dst, id, data, src, delay) 0x004D0570, RegisterMsg 0x004D09E0.
// Network messages travel inside a gsMSG_MP_WRAP (gs_mp_conn.h) as u16 id + u32 MPID + payload.
//
// Names: GS_MSG_* in "quotes" are the original strings registered by gsMsgInit
//   (v12 0x005107A0, feb 0x004FC420). Other names are ours (marked "named here").
// Renumbering in 1.2: one id was inserted at 0x3FA (everything from feb 0x3FA up is +1) and one more
//   at 0x40C (everything from feb 0x40B/0x40C up is +2). Always use the v12 column for 1.2.
//
// Direction: S = server, C = client, L = local (same process, not on the wire).
//
// Types assumed from other files: msgDATA, m3dV, m3dMATR, animCREATE_DATA, gsMSG_MP_WRAP.

#pragma once
#include "wr_sdk.h"   // engine core types + subsystem classes (re/sdk); was "assumed from other files"

//            name                               v12      feb     dir    payload / notes
// ----- engine / session (local) ------------------------------------------------------------
#define GS_MSG_FRAME                     0x3E8 // 0x3E8   L      none. "GS_MSG_FRAME": once per frame (gsMP_* tick, timeouts)
#define GS_MSG_RENDER                    0x3E9 // 0x3E9   L      "GS_MSG_RENDER"
#define GS_MSG_START_GAME                0x3EA // 0x3EA   L->gspSysPlay  gspSTART_GAME_DATA* (named here)
#define GS_MSG_STOP_GAME                 0x3EB // 0x3EB   L->gspSysPlay  none (named here)
#define GS_MSG_LOAD_LEVEL                0x3EC // 0x3EC   L, S->C  gspSTART_GAME_DATA (level name at +4). "GS_MSG_LOAD_LEVEL"
#define GS_MSG_INIT_GAME                 0x3EF // 0x3EF   L      SendAll from StartGame ("GS_MSG_INIT_GAME" log text)
#define GS_MSG_TERM_GAME                 0x3F0 // 0x3F0   L      SendAll from StopGame
#define GS_MSG_INIT_MAP                  0x3F1 // 0x3F1   L      "GS_MSG_INIT_MAP". 1.2 client sends it once per new map (MPCL_INIT_MAP)
#define GS_MSG_TERM_MAP                  0x3F2 // 0x3F2   L      ? id/name pairing from log strings only
#define GS_MSG_PRE_TERM                  0x3F3 // 0x3F3   L      SendAll from StopGame before TERM_GAME (named here)
#define GS_MSG_INIT_LEVEL                0x3F4 // 0x3F4   L      "GS_MSG_INIT_LEVEL"
#define GS_MSG_TERM_LEVEL                0x3F5 // 0x3F5   L      "GS_MSG_TERM_LEVEL"
#define GS_MSG_IACTION                   0x3F8 // 0x3F8   L      "GS_MSG_IACTION": LOCK/UNLOCK etc. (name at data+8)
#define GS_MSG_CDT                       0x3F9 // 0x3F9   L      "GS_MSG_CDT"
#define GS_MSG_NEW_3FA                   0x3FA // --      ?      1.2: new id (sent from v12 0x00509600, physMATH frame area)  // ? meaning
#define GS_MSG_SET_LEVEL_ANCHOR          0x3FB // 0x3FA   L      "GS_MSG_SET_LEVEL_ANCHOR"
#define GS_MSG_ASK_KEEP_LEVEL_RELOAD     0x3FC // 0x3FB   L      "GS_MSG_ASK_KEEP_LEVEL_RELOAD"
//                                                0x3FC in feb (gsMsgChangeMode / sound / wrGO): v12 0x3FD ?  not verified

// ----- multiplayer ---------------------------------------------------------------------------
#define GS_MSG_MP_SERVERCLIENT           0x400 // 0x3FF   L      gsMSG_MP_WRAP*: an unwrapped net message, delivered to gsSysServer / gsSysClient
#define GS_MSG_MP_SERVERCLIENT_SIMUL     0x401 // 0x400   L      raw packed packet (loopback "simulated network", MPCL_SIMUL_PACK); client: ReceiveMsg(data,-1)
#define GS_MSG_MP_CREATE_PLAYER          0x402 // 0x401   C->S, S->C  C->S: join request (player name, create data);
                                               //                     S->C: reply to the joining client = gsMSG_MP_CREATE_ENT of its own player
#define GS_MSG_MP_CREATE_ENT             0x403 // 0x402   S->C   gsMSG_MP_CREATE_ENT: spawn a replicated entity (other players)
#define GS_MSG_MP_SYNC                   0x404 // 0x403   S->C   gsMSG_MP_SYNC; classId -1 = begin (purge unbound), -2 = end -> client READY
#define GS_MSG_MP_GET_SYNC_DATA          0x405 // 0x404   L      entity vtbl +0x34 (feb +0x30): fill sync data
#define GS_MSG_MP_CLIENT_READY           0x406 // 0x405   C->S, S->C, L  gsMSG_MP_CLIENT_READY
#define GS_MSG_MP_407                    0x407 // 0x406   L      used by plrPLAYER / wrGO_SYSTEM  // ? meaning
#define GS_MSG_MP_PLAYER_ENTER           0x408 // 0x407   L      SendAll(entENTITY* player) from JoinGame / client after sync (named here)
#define GS_MSG_MP_PLAYER_LEAVE           0x409 // 0x408   L      SendAll from LeaveGame (named here)
#define GS_MSG_MP_SHOW_MSG               0x40A // 0x409   S->C   gsMSG_MP_SHOW_MSG (kill message). "GS_MSG_MP_SHOW_MSG"
#define GS_MSG_MP_UPD_REGULAR            0x40B // 0x40A   C<->S  regular entity update (pos/state). 1.2 name "GS_MSG_MP_UPD_REGULAR" (feb "GS_MSG_MP_UPD_POS")
                                               //                1.2: carries u16 sequence at +4, dropped if older than entity +0x3E
                                               //                (feb used NOTIFY sub -99 with a u32 sequence for this)
#define GS_MSG_MP_UPD_PACK_A             0x40C // --      C->S   1.2: new. Sent instead of 0x40B when entity mpFlags & 0x100 (net flags 0x10000) (named here)
#define GS_MSG_MP_UPD_PACK_B             0x40D // --?     C<->S  1.2: sent when entity mpFlags & 0x200, guaranteed (0x20000). Client folds 0x40C/0x40D into 0x40B
                                               //                and sets entity mpFlags |= 0x800.  // ? whether this is feb 0x40B renumbered
#define GS_MSG_MP_NOTIFY                 0x40E // 0x40C   C<->S  "GS_MSG_MP_NOTIFY": header + s16 sub-code (feb: int) + data, sub-codes below
#define GS_MSG_MP_WRITE_SYNC_DATA        0x40F // 0x40D   L      gsMSG_MP_SAVELOAD, entity vtbl +0x38 (feb +0x34) (named after ProcessMP_WRITE_SYNC_DATA)
#define GS_MSG_MP_READ_SYNC_DATA         0x410 // 0x40E   L      gsMSG_MP_SAVELOAD, entity vtbl +0x3C (feb +0x38)
#define GS_MSG_MP_NEW_MAP                0x411 // 0x40F   L->gspSysPlay, S->C  gspSTART_GAME_DATA: StartNewGameMap (v12 0x005347B0);
                                               //                posted by wrGO end-of-round with 0.5 s delay; client: wait for level
#define GS_MSG_MP_LEVEL_RESTART          0x412 // 0x410   S->C   none; sent to all clients by gspPLAY_SYSTEM on LOAD_LEVEL (named here)
#define GS_MSG_MP_RESPAWN_ALL            0x413 // 0x411   L->gspSysPlay  after level load: server sends every client its start pos (NOTIFY -97) and SyncClient
#define GS_MSG_MP_DELAYED_LOAD           0x414 // 0x412   L      client posts it to itself 0.5 s after LOAD_LEVEL (named here)

// GS_MSG_MP_NOTIFY sub-codes (s16 at +4 in 1.2; int in feb)
#define MP_NOTIFY_SPAWN_POS   (-97)   // 1.2 (feb -95): gsMSG_MP_NOTIFY_POS, server/client -> player entity: start position + direction
#define MP_NOTIFY_SEQUENCE    (-99)   // feb only: u32 sequence filter in entIACTIVE_OBJ::PreProcessMsg; 1.2 moved it into 0x40B
#define MP_NOTIFY_RATE        (-200)  // gsMSG_MP_NOTIFY_RATE: C->S snapshot rate (stored in server client rec +0x50);
                                      // 1.2: value = client's ConnSpeed preset 1/2/4 (feb: fixed 0.05 s); 1.2 client also accepts it (S->C)

// Network send flags (4th arg of SendMsgServer / SendMsgClient, passed to the driver)
#define MPNET_GUARANTEED      0x20000 // -> DPNSEND_GUARANTEED
#define MPNET_TIMEOUT         0x40000 // non-guaranteed with 500 ms timeout
//      0x10000, 0x80000          1.2: also used by the regular update (0x50000, 0x90000, 0xD0000)  // ? meaning

#pragma pack(push, 1)

// Every msgDATA starts with this 4-byte header (msgSYSTEM packs by it).
#if 0 // [integrator] msgHDR is a core type (engine_core/core_types.h); msgDATA has a "hdr" member alias
struct msgHDR
{
    unsigned char  type;      // +0 0
    unsigned char  flags;     // +1 0x40
    unsigned short size;      // +2 total size of the payload including this header
};
#endif


// GS_MSG_MP_CLIENT_READY 0x406 -- 0x0C bytes
struct gsMSG_MP_CLIENT_READY : msgDATA     // [integrator] was "{ msgHDR hdr; ...": same bytes, hdr is a msgDATA member alias
{
    int    mpid;              // +4 player MPID (-1 = all, used when deactivating)
    int    ready;             // +8 1 = active, 0 = deactivate (level change)
};

// GS_MSG_MP_NOTIFY 0x40E sub -200 -- 0x0A bytes (1.2)
struct gsMSG_MP_NOTIFY_RATE : msgDATA     // [integrator] was "{ msgHDR hdr; ...": same bytes, hdr is a msgDATA member alias
{
    short  sub;               // +4 -200
    float  rate;              // +6 1.0 / 2.0 / 4.0 (1.2), 0.05 (feb; feb layout: int sub, float at +8)
};

// GS_MSG_MP_NOTIFY sub -97 -- 0x1E bytes (1.2; feb: size 0x20, int sub = -95)
struct gsMSG_MP_NOTIFY_POS : msgDATA     // [integrator] was "{ msgHDR hdr; ...": same bytes, hdr is a msgDATA member alias
{
    short  sub;               // +4 -97
    m3dV   origin;            // +6
    m3dV   dir;               // +0x12 (axis Z of the start matrix)
};

// GS_MSG_MP_CREATE_PLAYER (S->C reply) / GS_MSG_MP_CREATE_ENT -- v12 offsets (feb in brackets)
struct gsMSG_MP_CREATE_ENT : msgDATA   // [integrator] merged with gs_mp_server.h's copy
{
    void*           user;            // +0x04 passed to entCreate
    int             mpid;            // +0x08 filled by the server
    char            name[0x40];      // +0x0C 1.2: used (animINST::SetName); feb ignored it on the client  // ? size
    char            className[0x40]; // +0x4C
    union { char templ[0x40]; char kind[0x40]; }; // +0x8C
    short           type;            // +0xCC 1.2: s16 (feb int, 4 bytes)
    animCREATE_DATA create;          // +0xCE (feb +0xD0); start matrix at +0xE0 = create+0x12 (feb +0xE2)
};                                   // size 0x164;

// GS_MSG_MP_SYNC 0x404 -- v12 offsets (feb in brackets). 1.2 inserted 4 bytes at +0x18.
struct gsMSG_MP_SYNC : msgDATA        // [integrator] merged with gs_mp_server.h's copy. 1.2 inserted 4 bytes at +0x18.
{
    union { void* user; msgDATA* userData; };     // +0x04
    int             mpid;            // +0x08
    int             classId;         // +0x0C -1 begin, -2 end, else entity class id (entity +0x14)
    union { int create; int unk10; };// +0x10 1 = create new entity, 0 = bind existing level entity
    union { int state; int ent30; }; // +0x14 -> entity +0x30
    union { int extra; int ent80; }; // +0x18 1.2: new -> entity +0x80 (Single mode only)  // ? meaning
    union { char templ[0x40]; char str1[0x40]; };     // +0x1C (feb +0x18) always "" from the server
    union { char className[0x40]; char str2[0x40]; }; // +0x5C (feb +0x58)
    char            name[0x40];      // +0x9C (feb +0x98) entity name (ent+0x40); "katapult" special-cased in 1.2
    union { int type; long unkDC; }; // +0xDC (feb +0xD8) -1
    animCREATE_DATA createData;      // +0xE0 (feb +0xDC); matrix at +0xF2 (feb +0xEE). gs_mp_server.cpp's "create"
};                                   // size 0x176 (feb 0x172);

// GS_MSG_MP_SHOW_MSG 0x40A
struct gsMSG_MP_SHOW_MSG : msgDATA     // [integrator] was "{ msgHDR hdr; ...": same bytes, hdr is a msgDATA member alias
{
    char   killer[0x40];     // +0x04
    char   victim[0x40];     // +0x44
    int    weapon;           // +0x84 <0 = none -> "GAME_PLAYER_KILLED", else "GAME_PLAYER_KILLED_%d" (weapon+1)
    int    type;             // +0x88 0 = kill message (only type handled by the client)
};

// gspSTART_GAME_DATA (START_GAME / LOAD_LEVEL / NEW_MAP) -- 0x10C bytes
#if 0 // [integrator] defined in gs_play.h (same layout, : msgDATA)
struct gspSTART_GAME_DATA
{
    msgHDR hdr;              // size 0x10C
    char   level[0x104];     // +0x004
    int    mode;             // +0x108 1 Single, 2 Server, 3 Client, 4 ServerOnly
};
#endif


#pragma pack(pop)
