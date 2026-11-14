// gs_mp_conn.h -- Will Rock 1.2 (v12.exe) multiplayer: message wrapping / serialisation
//
// Reconstructed for modders from static analysis (feb 2003 build with symbols + v12 disassembly).
// Original object: gs:gs_mp_conn.obj. Not meant to compile into the game.
//
// Types assumed from other files:
//   msgADDR, msgDATA, msgSYSTEM msgSystem (v12 0x008FA960)      -- message system (gs_msg / msg_*.cpp)
//     msgSYSTEM::Pack   (fioFILE_MEM*, msgDATA*)  v12 0x004D0AC0 (feb 0x004BF030)
//     msgSYSTEM::UnPack (fioFILE_MEM*) -> msgDATA* v12 0x004D0B20 (feb 0x004BF090)
//   fioFILE_MEM  (memory file, 300 = 0x12C bytes, v12 vtable 0x00608394, feb 0x005DCEBC)  -- gs_saveload / fio
//   entIACTIVE_OBJ (entity with a multiplayer id)                                         -- ent_*.cpp
//   gsMP_COMM    (base of gsMP_SERVER / gsMP_CLIENT; owns a gsMP_CONNECTION at +0x68)     -- gs_mp_misc.h
//
// fioFILE_MEM virtuals used here (v12 vtable 0x00608394):
//   slot +0x08 Term()                                 -- close / finish
//   slot +0x14 ReadData(void* dst, int size, int elemSize)
//   slot +0x18 WriteData(const void* src, int size, int elemSize)
//   slot +0x1C Init(void* buf, int bufSize, int mode)  -- mode 0x12 = write, 0x11 = read
//   field +0x110: number of bytes written (used as the packet length)  // ? exact meaning (size vs. cur pos)

#pragma once
#include "wr_sdk.h"   // engine core types + subsystem classes (re/sdk); was "assumed from other files"

#pragma pack(push, 1)

// gsMSG_MP_WRAP: one game message addressed over the network. 0x0E bytes, packed.
// It is also the msgDATA of GS_MSG_MP_SERVERCLIENT (v12 0x400, feb 0x3FF) when delivered locally.
struct gsMSG_MP_WRAP
{
    int       mpid;      // +0x00 MPID of the target entity; -2 (MPID_SYSTEM) = the gsMP_SERVER/gsMP_CLIENT itself; -1 = none
    short     msg;       // +0x04 message id carried (msgSYSTEM id, e.g. 0x406 CLIENT_READY)
    int       client;    // +0x06 sending client id: -3 = unset, -1 = local / server   (not serialised)
    msgDATA*  data;      // +0x0A payload (msgSYSTEM packs it by its own header: u8,u8 flags,u16 size)
};

#pragma pack(pop)

enum
{
    MPID_NONE   = -1,
    MPID_SYSTEM = -2,     // message for the MP layer itself (PrepareMsgWrap with ent == NULL)
    MPCLIENT_UNSET = -3,
};

// 1.2: packet buffer grew from 0x400 (1024) to 0x2710 (10000) bytes.
#define GS_MP_CONN_BUFSIZE     0x2710       // feb 0x400
// feb wrote '<' (0x3C) before and '>' (0x3E) after every packet (feb data 0x005F89E4/0x005F89E5).
// 1.2: both marker bytes were removed from the wire format.
#define GS_MP_CONN_MARK_BEGIN  '<'          // feb only
#define GS_MP_CONN_MARK_END    '>'          // feb only

// gsMP_CONNECTION: serialises one gsMSG_MP_WRAP into a flat buffer and back.
// Size v12 0x2718 (feb 0x408). Embedded in gsMP_COMM at +0x68.
// v12 vtable 0x006085BC (feb 0x005DD0E4), one slot.
class gsMP_CONNECTION
{
public:
    // vtable                                             // +0x0000
    char          buffer[GS_MP_CONN_BUFSIZE];             // +0x0004 (feb +0x004, 0x400 bytes)
    fioFILE_MEM*  file;                                   // +0x2714 (feb +0x404) memory file over 'buffer'

    gsMP_CONNECTION();                                    // v12 0x0053FB30 (feb 0x00529F00)
    virtual ~gsMP_CONNECTION();                           // slot +0x00: scalar deleting dtor v12 0x0053FBA0 (feb 0x00529F70)
                                                          // plain dtor v12 0x0053FBE0 (feb 0x00529FB0)

    void   Pack  (gsMSG_MP_WRAP* wrap);                                // v12 0x0053FC10 (feb 0x00529FE0)
    void   UnPack(gsMSG_MP_WRAP* wrap, void* packet, int size);        // v12 0x0053FC80 (feb 0x0052A070)
    char*  GetBuffer();                                                // v12 0x0053FCF0 (feb 0x0052A110)
    int    GetBufferSize();                                            // v12 0x0053FD00 (feb 0x0052A120)
};

// gsMP_COMM: common base of gsMP_SERVER and gsMP_CLIENT. vtable v12 0x00607AE4 (feb ?), [integrator] declared here.
//   size 0x2780 (feb 0x470) = msgADDR (0x24) + 0x44 bytes + gsMP_CONNECTION (0x2718) at +0x68.
class gsMP_COMM : public msgADDR
{
public:
    unsigned char   _24[0x68 - 0x24];   // +0x24..+0x67 ? (+0x28 = 0 in the server ctor)
    gsMP_CONNECTION conn;               // +0x68 (gs_mp_server.cpp called it m_conn)
    virtual int  Init();                // slot +0x10 (gsMP_COMM: empty)
    virtual void Term();                // slot +0x14 (empty, v12 0x00538270 shared)
    void PrepareMsgWrap(gsMSG_MP_WRAP* wrap, entIACTIVE_OBJ* ent, int msg, msgDATA* data); // v12 0x0053FD10 (feb 0x0052A130)
    void PrepareConn   (gsMSG_MP_WRAP* wrap);                                              // v12 0x0053FD40 (feb 0x0052A160)
};
// gsMP_COMM helpers that live in gs_mp_conn.obj:
//   void gsMP_COMM::PrepareMsgWrap(gsMSG_MP_WRAP*, entIACTIVE_OBJ* ent, int msg, msgDATA* data); // v12 0x0053FD10 (feb 0x0052A130)
//   void gsMP_COMM::PrepareConn   (gsMSG_MP_WRAP*);                                              // v12 0x0053FD40 (feb 0x0052A160)
// gsMP_COMM layout used here: +0x68 gsMP_CONNECTION conn (so conn.buffer at +0x6C, conn.file at +0x277C; feb +0x46C).
