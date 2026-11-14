// button.cpp -- bttnBUTTON, a push button / lever: PUSH and POP sequences, used by a player (Toggle), by IActions
// PUSH / POP, or by a shot when BUTTON.isCanShoot (Will Rock 1.2, v12.exe; feb obj gs_shared:button.obj)
//
// Rebuilt from the 1.2 code (v12 0x00556200..0x00556C82, vtable 0x00608F40). Fields at feb + 0x1C. 1.2 changes,
// from the disassembly:
//   - every change of state (Toggle, the PUSH / POP IActions, the end of a sequence) goes to the clients as one
//     packed notify { type 1 at +4, flags +6, sequence +0xA } -- feb sent an MP_UPD_STATE every frame it moved;
//     the clients' Toggle request is notify 0 { mpId +6 };
//   - INIT raises event 0x800; the entity class bits are at +0xBC (feb +0xB8).

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"
#include "../gs/gs_mp_misc.h"

extern float gsElapsedTime;                 // v12 0x0098E9F4

static const float bttnEps = 1e-6f;         // v12 0x00608F3C
float bttnEpsSq;                            // v12 0x009AE628 (static init v12 0x00556BE0)
static struct bttnEPS_INIT { bttnEPS_INIT() { bttnEpsSq = bttnEps * bttnEps; } } bttnEpsInit;

#pragma pack(push, 1)
struct bttnSYNC : msgDATA {                 // the sync block (GET/READ_SYNC_DATA), v12 0x009AE62C, size 0x10
    u32   flags;                            // +0x04
    int   seq;                              // +0x08
    float time;                             // +0x0C
};
struct bttnNOTIFY : msgDATA {               // GS_MSG_MP_NOTIFY 0x40E
    short type;                             // +0x04 -100 sync block, 0 toggle request, 1 state
    union {
        bttnSYNC *sync;                     // +0x06 (type -100)
        int mpId;                           // +0x06 (type 0) who pressed it, -1 nobody
        struct {
            u32 flags;                      // +0x06 (type 1)
            int seq;                        // +0x0A
        };
    };
};
#pragma pack(pop)
static bttnSYNC bttnSync;
static int bttnSyncInited;                  // v12 0x009AE63C bit 0

static void bttnSyncInit()
{
    if (!(bttnSyncInited & 1)) {
        bttnSyncInited |= 1;
        bttnSync.packId = 0;
        bttnSync.tag = 0x40;
        bttnSync.flags = 0;
        bttnSync.seq = -1;
        bttnSync.size = 0x10;
    }
}

// The state notify (type 1), sent when this side runs the button (entFlags & 4).
static void bttnSendState(bttnBUTTON *b)
{
    bttnNOTIFY m;
    m.packId = 0;
    m.tag = 0x40;
    m.type = 1;
    m.flags = 0;
    m.seq = -1;
    m.size = 0xE;
    if (!(b->entFlags & 4))
        return;
    m.seq = b->inst->seq;
    m.flags = b->flags;
    b->SendMP_Server_2_ClientAll(&m, 0x20000);
}

// v12 0x00556200  (feb 0x0053D890)
int __fastcall bttnInit(void)
{
    return entRegisterClass((char *)"button", 'BTTN', 0x28, bttnBUTTON::Create, 0, 1) != 0;
}

// v12 0x00556240  (feb 0x0053D8D0)
BOOL32 bttnBUTTON::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 1000:                              // FRAME
        if (!(entFlags & 4)) {
            inst->AdvanceTime(gsElapsedTime, 1, 0);
            return 0;
        }
        if (!(flags & (BTTNF_PUSHING | BTTNF_POPPING)))
            return 0;
        if (!inst->AdvanceTime(gsElapsedTime, 1, 0))
            return 0;
        if (flags & BTTNF_PUSHING) {
            flags |= BTTNF_PUSHED;
            pendingEvents |= 4;
        }
        if (flags & BTTNF_POPPING) {
            flags |= BTTNF_UP;
            pendingEvents |= 8;
        }
        flags &= ~(BTTNF_PUSHING | BTTNF_POPPING);
        bttnSendState(this);
        return 0;
    case MSG_MSG_INIT: {                    // starts up (at the end of POP)
        seqPush = inst->tpl->FindSeqName((char *)"PUSH");
        if (seqPush == -1)
            return 0;
        seqPop = inst->tpl->FindSeqName((char *)"POP");
        if (seqPop == -1)
            return 0;
        float l = inst->GetSeqLength(seqPush);
        pushLen = l;
        inst->SetAnimSeq(seqPop, l);
        flags |= BTTNF_UP;
        int b;
        inst->GetInt((char *)"BUTTON", (char *)"nmbLocks", &nLocks);
        if (inst->GetBool((char *)"BUTTON", (char *)"isPushOnce", &b) && b)
            flags |= BTTNF_PUSH_ONCE;
        unkBC |= 4;
        gsSysServer->DeclSyncable(this);
        flags |= BTTNF_NO_SHOOT;
        if (inst->GetBool((char *)"BUTTON", (char *)"isCanShoot", &b) && b)
            flags &= ~BTTNF_NO_SHOOT;
        pendingEvents |= 0x800;
        return 0;
    }
    case 0x3F8: {                           // IACTION PUSH / POP
        apNAME *a = (apNAME *)((char *)data + 8);
        if (a->IsName((char *)"PUSH")) {
            if (!(flags & BTTNF_UP))
                return 0;
            flags &= ~BTTNF_UP;
            flags &= ~BTTNF_POPPING;
            flags |= BTTNF_PUSHING;
            flags |= BTTNF_WAS_PUSHED;
            inst->SetAnimSeq(seqPush, 0.0f);
            pendingEvents |= 1;
            bttnSendState(this);
        } else if (a->IsName((char *)"POP")) {
            if (!(flags & BTTNF_PUSHED))
                return 0;
            if ((flags & BTTNF_PUSH_ONCE) && (flags & BTTNF_WAS_PUSHED))
                return 0;
            flags &= ~BTTNF_PUSHED;
            flags &= ~BTTNF_PUSHING;
            flags |= BTTNF_POPPING;
            inst->SetAnimSeq(seqPop, 0.0f);
            pendingEvents |= 2;
            bttnSendState(this);
        }
        return 0;
    }
    case 0x40E: {                           // MP_NOTIFY
        bttnNOTIFY *m = (bttnNOTIFY *)data;
        switch (m->type) {
        case -100:
            flags = m->sync->flags;
            inst->SetAnimSeq(m->sync->seq, m->sync->time);
            break;
        case 0:
            Toggle((entENTITY *)gsSysMP->FindMPID(m->mpId, 0));
            break;
        case 1:
            if (inst->seq != m->seq)
                inst->SetAnimSeq(m->seq, 0.0f);
            flags = m->flags;
            break;
        }
        return 0;
    }
    case 0x4C6:                             // DAMAGE
        if (!(flags & BTTNF_NO_SHOOT))
            Toggle(0);
        return 0;
    }
    return 0;
}

// v12 0x00556730  (feb 0x0053DD40)
void bttnBUTTON::ProcessMP_GET_SYNC_DATA(void *d)
{
    msgDATA **out = (msgDATA **)d;
    if (!*out) {
        bttnSyncInit();
        *out = &bttnSync;
    }
    bttnSYNC *s = (bttnSYNC *)*out;
    s->flags = flags;
    s->seq = inst->seq;
    s->time = inst->seqTime;
    entENTITY::ProcessMP_GET_SYNC_DATA(d);
}

// v12 0x005567C0  (feb 0x00544880)
// The sync block as chunks 0xF0 flags, 0xF1 sequence, 0xF2 time.
void bttnBUTTON::ProcessMP_WRITE_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    bttnSYNC *s = (bttnSYNC *)d->data;
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    u32 v;
    d->file->WriteChunk(0xF0, &c);
    v = s->flags;
    d->file->WriteData(&v, 4, 4);
    d->file->UpdateChunk(&c);
    d->file->WriteChunk(0xF1, &c);
    v = (u32)s->seq;
    d->file->WriteData(&v, 4, 4);
    d->file->UpdateChunk(&c);
    d->file->WriteChunk(0xF2, &c);
    memcpy(&v, &s->time, 4);
    d->file->WriteData(&v, 4, 4);
    d->file->UpdateChunk(&c);
    d->file->WriteLastChunk();
    entENTITY::ProcessMP_WRITE_SYNC_DATA(d);
}

// v12 0x005568B0  (feb 0x0053DDD0)
void bttnBUTTON::ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    bttnSYNC *s = (bttnSYNC *)d->data;
    if (!s) {
        bttnSyncInit();
        s = &bttnSync;
        d->data = s;
        s->packId = 0;
        s->tag = 0x40;
        s->size = 0x10;
        s->flags = 0;
        s->seq = -1;
    }
    for (int ok = d->file->ReadChunk(&c); ok && c.id != 1; ok = d->file->ReadChunk(&c)) {
        void *dst = 0;
        if (c.id == 0xF0)
            dst = &s->flags;
        else if (c.id == 0xF1)
            dst = &s->seq;
        else if (c.id == 0xF2)
            dst = &s->time;
        if (dst)
            d->file->ReadData(dst, 4, 4);
    }
    entENTITY::ProcessMP_READ_SYNC_DATA(d);
}

// v12 0x005569D0  (feb 0x0053DEF0)
// Push an up button, pop a pushed one (not a push-once button already pushed). A client only asks the server.
// Returns 1 when something happened; the game returns `by` itself when the button is moving.
BOOL32 bttnBUTTON::Toggle(entENTITY *by)
{
    if (nLocks > 0)
        return 0;
    if (!(entFlags & 4)) {
        bttnNOTIFY m;
        m.packId = 0;
        m.tag = 0x40;
        m.type = 0;
        m.size = 0xA;
        m.mpId = by ? by->mpId : -1;
        SendMP_Client_2_Server(&m, 0x20000);
        return 1;
    }
    if (flags & BTTNF_UP) {
        flags &= ~BTTNF_UP;
        flags &= ~BTTNF_POPPING;
        flags |= BTTNF_PUSHING;
        flags |= BTTNF_WAS_PUSHED;
        inst->SetAnimSeq(seqPush, 0.0f);
        pendingEvents |= 1;
        bttnSendState(this);
        return 1;
    }
    if (!(flags & BTTNF_PUSHED))
        return (BOOL32)(intptr_t)by;
    if ((flags & BTTNF_PUSH_ONCE) && (flags & BTTNF_WAS_PUSHED))
        return 0;
    flags &= ~BTTNF_PUSHED;
    flags &= ~BTTNF_PUSHING;
    flags |= BTTNF_POPPING;
    inst->SetAnimSeq(seqPop, 0.0f);
    pendingEvents |= 2;
    bttnSendState(this);
    return 1;
}

// v12 0x00556C00  (feb 0x0053E050)
entENTITY *__fastcall bttnBUTTON::Create(animINST *)
{
    bttnBUTTON *b = new bttnBUTTON;         // operator new(0x17B), entENTITY ctor
    if (!b)
        return 0;
    b->flags = 0;
    b->seqPush = -1;
    b->seqPop = -1;
    b->pushLen = 0.0f;
    b->unk98 = 0;
    return b;
}

// v12 0x00556C60  (feb 0x0053E0B0)
BOOL32 bttnBUTTON::IsCanPush(entENTITY *)
{
    if (nLocks > 0)
        return 0;
    if ((flags & BTTNF_PUSH_ONCE) && (flags & BTTNF_WAS_PUSHED))
        return 0;
    return 1;
}
