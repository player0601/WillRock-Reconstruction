// gs_mp_conn.cpp -- Will Rock 1.2 (v12.exe) multiplayer: gsMP_CONNECTION Pack/UnPack, gsMSG_MP_WRAP
//
// Reconstructed source (documentation for modders). Original object: gs:gs_mp_conn.obj.
// Every function: "// v12 <addr>  (feb <addr>)". 1.2 changes against the Feb 2003 build: "// 1.2:".
//
// Wire format of one game message (what gsMP_NETWORK::Client_SendDataToServer / Server_SendDataToClient
// hand to the DirectPlay driver; the driver then coalesces several of these into <=700 byte DPlay packets,
// see mp_transport.md):
//
//   1.2 (v12):                          feb (Feb 2003):
//   +0  u16  msg id                     +0  u8   '<' (0x3C)
//   +2  u32  target MPID                +1  u16  msg id
//   +6  ...  msgSYSTEM::Pack(data)      +3  u32  target MPID
//                                       +7  ...  msgSYSTEM::Pack(data)
//                                       +n  u8   '>' (0x3E)
//
// No compression, no encryption, no checksum. Integers are little-endian (x86 memory copies).
// The sender's client id is NOT on the wire: the receiving server learns it from the DPNID.

#include "gs_mp_conn.h"
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

extern msgSYSTEM msgSystem;                 // v12 0x008FA960

// ---------------------------------------------------------------------------------------------
// v12 0x0053FB30  (feb 0x00529F00)
// Constructor: allocates the fioFILE_MEM that writes into / reads from 'buffer'.
gsMP_CONNECTION::gsMP_CONNECTION()
{
    // vtable = gsMP_CONNECTION::`vftable' (v12 0x006085BC)
    file = new fioFILE_MEM;                 // operator new(0x12C); inline fioFILE_MEM ctor zeroes
                                            // +0x04,+0x08(byte),+0x10C..+0x128; vtable v12 0x00608394
                                            // (if new fails, file = NULL)
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053FBE0  (feb 0x00529FB0)
// Destructor: deletes the memory file.
// The scalar deleting destructor (vtable slot +0x00, v12 0x0053FBA0, feb 0x00529F70) is the same
// body followed by operator delete(this) when (flags & 1).
gsMP_CONNECTION::~gsMP_CONNECTION()
{
    if (file)
        delete file;                        // file->vtbl[0](1)
    file = NULL;
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053FC10  (feb 0x00529FE0)
// Serialises 'wrap' into this->buffer. Length afterwards: GetBufferSize().
void gsMP_CONNECTION::Pack(gsMSG_MP_WRAP* wrap)
{
    file->Init(buffer, GS_MP_CONN_BUFSIZE, 0x12);         // 1.2: 0x2710 (feb 0x400); 0x12 = write
    // 1.2: feb wrote the begin marker here:  file->WriteData("<", 1, 1);   (removed)
    file->WriteData(&wrap->msg,  2, 2);                    // u16 msg id
    file->WriteData(&wrap->mpid, 4, 4);                    // u32 target MPID
    msgSystem.Pack(file, wrap->data);                      // payload, by msgSYSTEM's per-message packer
    // 1.2: feb wrote the end marker here:    file->WriteData(">", 1, 1);   (removed)
    file->Term();
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053FC80  (feb 0x0052A070)
// Parses one received packet into 'wrap'. wrap->data points into msgSYSTEM's unpack storage
// (valid until the next UnPack). size == -1 means "whole buffer".
void gsMP_CONNECTION::UnPack(gsMSG_MP_WRAP* wrap, void* packet, int size)
{
    if (size == -1)
        size = GS_MP_CONN_BUFSIZE;                         // 1.2: 0x2710 (feb 0x400)

    file->Init(packet, size, 0x11);                        // 0x11 = read
    // 1.2: feb read (and ignored, never checked) the '<' marker byte here.
    file->ReadData(&wrap->msg,  2, 2);
    file->ReadData(&wrap->mpid, 4, 4);
    wrap->data = msgSystem.UnPack(file);
    // 1.2: feb read (and ignored) the '>' marker byte here.
    file->Term();
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053FCF0  (feb 0x0052A110)
// Returns the packet buffer.
char* gsMP_CONNECTION::GetBuffer()
{
    return buffer;                                         // this + 4
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053FD00  (feb 0x0052A120)
// Returns the number of bytes produced by the last Pack().
int gsMP_CONNECTION::GetBufferSize()
{
    return *(int*)((char*)file + 0x110);                   // fioFILE_MEM field +0x110  // ? named field
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053FD10  (feb 0x0052A130)
// Fills a wrap for 'ent' (NULL = the MP system object itself).
void gsMP_COMM::PrepareMsgWrap(gsMSG_MP_WRAP* wrap, entIACTIVE_OBJ* ent, int msg, msgDATA* data)
{
    wrap->msg  = (short)msg;
    wrap->data = data;
    if (ent == NULL)
        wrap->mpid = MPID_SYSTEM;                          // -2
    else
        wrap->mpid = ent->mpid;                            // 1.2: entity +0x88 (feb +0x8C)
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053FD40  (feb 0x0052A160)
// Same as conn.Pack(wrap) on this gsMP_COMM's own connection (inlined copy in the original).
void gsMP_COMM::PrepareConn(gsMSG_MP_WRAP* wrap)
{
    gsMP_CONNECTION& c = conn;                             // this + 0x68; file at +0x277C (feb +0x46C)
    c.file->Init(c.buffer, GS_MP_CONN_BUFSIZE, 0x12);      // 1.2: 0x2710 (feb 0x400)
    // 1.2: '<' marker removed
    c.file->WriteData(&wrap->msg,  2, 2);
    c.file->WriteData(&wrap->mpid, 4, 4);
    msgSystem.Pack(c.file, wrap->data);
    // 1.2: '>' marker removed
    c.file->Term();
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053FDB0  (feb 0x0052A1F0 "_$E2")
// Compiler-generated static initializer of this .obj: g = c * c with c = 1e-6f (0x358637BD).
// v12: [0x009A8468] = [0x006085B8] * [0x006085B8].   // ? what the squared constant is used for
