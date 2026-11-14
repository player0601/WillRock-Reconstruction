// gs_mp_misc.h -- Will Rock 1.2 (v12.exe, 2003-07-30) multiplayer system core
//
// Reconstructed documentation source for modders. Not compilable into the game.
// Reference: Feb 2003 build (feb.exe + WillRock.map, obj gs:gs_mp_misc.obj).
// Addresses: "v12 0x..." = 1.2 retail, "feb 0x..." = Feb 2003 build.
// Every 1.2 difference against feb is marked "// 1.2:". Unknowns are marked "// ?".
//
// ---------------------------------------------------------------------------
// Types assumed from other files (not defined here):
//   msgADDR, msgSYSTEM msgSystem (v12 0x008FA960), msgDATA, msgDATA_PACK,
//       msgSYSTEM::InsertAddr/DestroyAddr/RegisterMsgData/SendAll/SendMsg/Pack/UnPack
//   gsMP_SERVER  *gsSysServer   (v12 0x0098E870)   -- gs_mp_server.cpp
//   gsMP_CLIENT  *gsSysClient   (v12 0x0098E860)   -- gs_mp_client.cpp
//   gsMP_NETWORK *gsSysNetwork  (v12 0x0099D814)   -- gs_mp_network.cpp
//   gsMP_STAT                                       -- gs_mp_network.cpp (Clear v12 0x0052EA70)
//   gsDEMO gsDemo (v12 0x009A5C28): Init 0x0053F050, Term 0x0053F0A0,
//       Play_Start 0x0053F560, Record_Start 0x0053F1E0
//   uiUI_SYSTEM *uiSystem (v12 0x00994568)
//   int gsAppState (v12 0x0098E824)
//   scnSCENE *gsScenePtr (v12 0x0098E81C): FindInstName 0x004E6D90, FindNextInst 0x004E6E20,
//       FloorPlace 0x004DF2B0, IsVisiblePoint 0x004DEFD0, ConstrSphere 0x004DB000
//   animINST (GetPos 0x004CE3F0, GetDir 0x004CE400), animCREATE_DATA (ctor 0x004D1360),
//   entENTITY, entIACTIVE_OBJ, entFindNext 0x005086A0, entFindNextIO 0x0050C340
//   cdtREFINE (vtable 0x005FA0E0), m3dV, m3dMATR, fioFILE, fioFILE_MEM
//   m3dRandIntMax 0x0056E790, m3dMakeVTangRandom 0x0056ED00
//   _apCfgReadString 0x004AFA50, _apCfgReadInt 0x004AF7E0
//   1.2 quantizer helpers (new 1.2 module at v12 0x00514170.., name unknown, see .cpp):
//       mpQuantPos 0x00514870, mpDequantPos 0x00514900,
//       mpQuantDir8 0x00514180, mpDequantDir8 0x005141E0
// ---------------------------------------------------------------------------

#pragma once
#include "wr_sdk.h"
#include "gs_mp_conn.h"   // [integrator] dependency
#include "gs_mp_network.h"   // [integrator] dependency
// [integrator] functions of the new 1.2 position-update module (v12 0x00514170..), named in gs_mp_misc.cpp
int  __fastcall mpRegisterUpdPosPack();                                                     // v12 0x00514990
void __fastcall mpQuantPos(m3dV *p, unsigned short *qx, unsigned short *qz, unsigned short *qy); // v12 0x00514870
void __fastcall mpDequantPos(unsigned short qx, unsigned short qz, unsigned short qy, m3dV *p); // v12 0x00514900
void __fastcall mpQuantDir8(m3dV *v, unsigned char *pitch, unsigned char *yaw);             // v12 0x00514180
void __fastcall mpDequantDir8(unsigned char pitch, unsigned char yaw, m3dV *v);             // v12 0x005141E0
   // engine core types + subsystem classes (re/sdk); was "assumed from other files"

// ---- game-mode values (gsMP_SYSTEM::mode, +0x28) --------------------------
enum gsMP_MODE
{
    gsMP_MODE_NONE        = 0,
    gsMP_MODE_SINGLE      = 1,   // "Single"
    gsMP_MODE_SERVER      = 2,   // "Server"     listen server (host plays)
    gsMP_MODE_CLIENT      = 3,   // "Client"
    gsMP_MODE_SERVER_ONLY = 4,   // "ServerOnly" dedicated (-dedicated forces it, see gs_play.cpp)
};

// ---- gsAppState bits used in this file ------------------------------------
#define GS_APP_DEMO_RECORD     0x00000400
#define GS_APP_DEMO_PLAY       0x00000800
#define GS_APP_CLIENT_WAITLVL  0x00400000   // client: waiting for level from server
#define GS_APP_DEDICATED       0x08000000   // "-dedicated" on the command line

// ---- message ids used in this file (v12 numbering) ------------------------
#define GS_MSG_FRAME              0x3E8   // per-frame tick (msgADDR::ProcessFRAME)
#define GS_MSG_INIT_LEVEL         0x3F4
#define GS_MSG_TERM_LEVEL         0x3F5
#define GS_MSG_MP_JOIN_PLAYER     0x402   // 1.2: was 0x401 in feb. client->server "create my player"
#define GS_MSG_MP_PLAYER_ACTIVE   0x406   // 1.2: was 0x405. {mpid, active}  name ?
#define GS_MSG_MP_PLAYER_ENTER    0x408   // 1.2: was 0x407. SendAll, data = player entity
#define GS_MSG_MP_PLAYER_LEAVE    0x409   // 1.2: was 0x408. SendAll, data = player entity
#define ENT_MSG_GET_POS           0x415   // ? entity query, fills m3dV (used by start-pos picking)

#define MP_ID_NONE     (-1)
#define MP_ID_SERVER   (-2)   // ? meaning of -2 / -3 inferred from IsRegularMPID only
#define MP_ID_ALL      (-3)

#define GS_MP_MAX_START_POS  64

// ---------------------------------------------------------------------------
// msgDATA header (common to all messages, 4 bytes) -- from msg system
//   u8  packId;   // id byte of the registered msgDATA_PACK (0 = plain copy)
//   u8  flags;    // always 0x40 here  ?
//   u16 size;     // total size of the message struct
// ---------------------------------------------------------------------------

// gsMSG_MP_PTR -- a message wrapping another message (v12 ctor 0x0050E870, feb 0x004FBEC0)
struct gsMSG_MP_PTR : msgDATA
{
    msgDATA *data;          // +0x04  inner message, packed recursively
    gsMSG_MP_PTR();         // v12 0x0050E870 (feb 0x004FBEC0)
};                          // size 8, packId = gsMsgMpPtrPack.id (v12 0x0098E868)

// gsEV_SYNC -- event-sync notification (v12 ctor 0x0050E890, feb 0x004FBEE0)
#pragma pack(push, 1)
struct gsEV_SYNC : msgDATA            // [integrator] merged with gs_mp_server.h's copy
{
    union { short ev; short code; };                 // +0x04  init -100        // 1.2: was int at +4 (feb)
    union { msgDATA *data; msgDATA *pData; float fRate; }; // +0x06 inner message (code -200: float rate) // 1.2: was +0x08 (feb)
    gsEV_SYNC();            // v12 0x0050E890 (feb 0x004FBEE0) [integrator: declared]
};                          // 1.2: size 0x0A (feb 0x0C); packId = v12 0x0098E858
#pragma pack(pop)

// gsMSG_MP_PLAYER_ACTIVE -- 12-byte plain message sent with 0x406 (name ?)
struct gsMSG_MP_PLAYER_ACTIVE : msgDATA   // packId 0, flags 0x40, size 0x0C
{
    int mpid;               // +0x04  player's MP id (entity +0x88), -1 for dedicated host
    int active;             // +0x08  1 = joined, 0 = leaving
};

// gsMSG_MP_JOIN_PLAYER -- "create my player" (sent with 0x402).  name ? (not in feb map;
// feb built the same 0x164-byte layout inline in JoinGame and sent it raw).
// 1.2: has its own ctor (v12 0x0050E8C0) and its own compressing pack class
// (gsMSG_MP_JOIN_PACK, vtable v12 0x00607A84), see gs_mp_misc.cpp.
struct gsMSG_MP_JOIN_PLAYER : msgDATA     // packId = v12 0x0098E9E4, size 0x164
{
    msgDATA *next;          // +0x004 optional chained message (flag 0x20 in packed form)
    int      mpid;          // +0x008 init -1
    char     name[64];      // +0x00C [Multiplayer] PlayerName
    char     entName[64];   // +0x04C "player" (entity template name ?)
    char     entClass[64];  // +0x08C "player" (class / template prefix ?)
    short    unk0CC;        // +0x0CC init -1  ?
    animCREATE_DATA create; // +0x0CE scale m3dV @+0xCE, s16 @+0xDA (init -1), float @+0xDC,
                            //        m3dMATR start matrix @+0xE0 (create+0x12)
    gsMSG_MP_JOIN_PLAYER(); // v12 0x0050E8C0 [integrator: declared]
};                          // 0x164 bytes

// ---------------------------------------------------------------------------
// pack classes (vtable: slot +0 Pack, slot +4 UnPack)
// ---------------------------------------------------------------------------
class gsMSG_MP_PTR_PACK : public msgDATA_PACK          // vtable v12 0x00607A7C (feb 0x005DC5CC)
{                                                       // instance v12 0x0098E864 (feb 0x00948268)
public:
    virtual int      Pack  (fioFILE_MEM *f, msgDATA *d);   // slot +0x00  v12 0x0050E920
    virtual msgDATA *UnPack(fioFILE_MEM *f);               // slot +0x04  v12 0x0050E960
};

class gsMSG_MP_JOIN_PACK : public msgDATA_PACK          // 1.2: new. vtable v12 0x00607A84
{                                                       // instance v12 0x0098E9E0 (id byte +4)
public:                                                 // class name ? (not in feb map)
    virtual int      Pack  (fioFILE_MEM *f, msgDATA *d);   // slot +0x00  v12 0x0050E990
    virtual msgDATA *UnPack(fioFILE_MEM *f);               // slot +0x04  v12 0x0050EDA0
};

class gsMSG_MP_NOTIFY_EV_SYNC : public msgDATA_PACK     // vtable v12 0x00607A8C (feb 0x005DC5D4)
{                                                       // instance v12 0x0098E854 (feb 0x00948254)
public:
    virtual int      Pack  (fioFILE_MEM *f, msgDATA *d);   // slot +0x00  v12 0x0050F0B0
    virtual msgDATA *UnPack(fioFILE_MEM *f);               // slot +0x04  v12 0x0050F0F0
};
// 1.2: gsMSG_MP_UPD_POS / gsMSG_MP_UPD_POS_PACK (feb 0x004FBFF0..0x004FC1C0) and
//      gsMSG_MP_UPD_POS_PACK's vtable left this object. The 1.2 position-update pack
//      appears to be registered from v12 0x00514990 (instance 0x0098EC28) in the new
//      module at 0x00514170.. -- not reconstructed here (?).

// ---------------------------------------------------------------------------
// gsMP_SYSTEM -- top-level MP mode switch.  object size 0x664 (same as feb)
// vtable v12 0x00607A94 (feb 0x005DC5E4).  global gsSysMP v12 0x0098E850 (feb 0x00948250)
// ---------------------------------------------------------------------------
class gsMP_SYSTEM : public msgADDR
{
public:
    // --- msgADDR base: +0x00 vtable .. +0x24 ---
    // +0x10 = 'RDAM' magic, +0x14 = address name '$PM$', +0x1C = priority 0x14
    int    field24;                      // +0x024 [integrator] msgADDR is 0x24 bytes; zeroed by the ctor
    int    mode;                         // +0x028 gsMP_MODE (feb +0x28)
    // +0x02C: unused slot so that modeName[mode-1] == *(this + 0x2C + mode*4)
    char  *modeName[4];                  // +0x030 "Single","Server","Client","ServerOnly"
    m3dV   startPos[GS_MP_MAX_START_POS];// +0x040
    m3dV   startDir[GS_MP_MAX_START_POS];// +0x340
    int    nStartPos;                    // +0x640
    int    unk644;                       // +0x644 zeroed in ctor  ?
    gsMP_STAT stat;                      // +0x648 (its +0x10 buffer, +0x658, is apFree'd in dtor)

    // virtuals, in vtable order
    virtual void *__vecDelDtor(unsigned flags);                              // slot +0x00 v12 0x0050D7E0
    virtual int   ProcessMsg(int msg, void *data, msgADDR *from, int *res);  // slot +0x04 v12 0x0050DD40
    //            PostProcessMsg x2 inherited                                // slot +0x08,+0x0C
    virtual void  ProcessFRAME();                                            // slot +0x10 (inherited empty body) [integrator: declared]
    virtual void  ProcessINIT_LEVEL();                                       // slot +0x14 v12 0x0050DD80
    virtual void  ProcessTERM_LEVEL();                                       // slot +0x18 v12 0x0050DF90

    int   Init();                                   // v12 0x0050D820
    void  Term();                                   // v12 0x0050D870
    int   InitGame(int mode);                       // v12 0x0050D8B0
    void  TermGame();                               // v12 0x0050D9B0
    int   JoinGame();                               // v12 0x0050D9D0
    void  LeaveGame();                              // v12 0x0050DC00
    void  SetupMPStates();                          // v12 0x0050DCC0
    void  SelectStartPosDir(m3dV *pos, m3dV *dir, entENTITY *who);  // v12 0x0050DFA0 (1.2: +arg)
    void  GetStartPosDir(int idx, m3dV *pos, m3dV *dir);            // v12 0x0050E4F0
    int   GetNStartPos();                           // v12 0x0050E570
    char *Mode2String(int mode);                    // v12 0x0050E580
    int   String2Mode(char *s);                     // v12 0x0050E5A0
    float ConnSpeedScale(int connSpeed);            // v12 0x0050E5E0  1.2: new (name ?)
    int   ReadMode();                               // v12 0x0050E610
    int   IsRegularMPID(unsigned long id);          // v12 0x0050E690
    entIACTIVE_OBJ *FindMPID(int id, int flagMask); // v12 0x0050E6B0 (alias FindEnt)
    entENTITY *FindPlrLocal();                      // v12 0x0050E700
    entENTITY *GetNextPlayer(entENTITY *prev);      // v12 0x0050E750
    int   WriteInst(fioFILE *f, animINST *inst);    // v12 0x0050E770
    animINST *ReadInst(fioFILE *f);                 // v12 0x0050E7A0
    int   WriteEIO(fioFILE *f, entIACTIVE_OBJ *o);  // v12 0x0050E800
    entIACTIVE_OBJ *ReadEIO(fioFILE *f);            // v12 0x0050E820
    void  UpdateEntFrame(entIACTIVE_OBJ *e);        // v12 0x005149A0 (feb 0x004FB560?) MP state of an entity each FRAME // ? name
    // 1.2: UpdateEntFrame (feb 0x004FB560) and UpdateDeadReckonData (feb 0x004FB8D0)
    //      are no longer in this object (moved, see open questions in .cpp).
};

extern gsMP_SYSTEM  *gsSysMP;                   // v12 0x0098E850
extern gsMSG_MP_PTR_PACK       gsMsgMpPtrPack;  // v12 0x0098E864
extern gsMSG_MP_NOTIFY_EV_SYNC gsMsgEvSyncPack; // v12 0x0098E854
extern gsMSG_MP_JOIN_PACK      gsMsgJoinPack;   // v12 0x0098E9E0  1.2: new
extern float gsMPEpsilonSq;                     // v12 0x0098E86C  1.2: = 1e-6 * 1e-6

int  __fastcall gsMPInit();                     // v12 0x0050D650
void __fastcall gsMPTerm();                     // v12 0x0050D7B0
