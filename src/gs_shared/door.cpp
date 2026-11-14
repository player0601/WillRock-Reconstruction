// door.cpp -- doorDOOR, the animated door: plays its OPEN / CLOSE sequences when a player (or an NPC) comes within
// DOOR.radOpen, stays shut while it has locks, tells the players "locked" / "unlocked"
// (Will Rock 1.2, v12.exe; feb obj gs_shared:door.obj)
//
// Rebuilt from the 1.2 code (v12 0x0055B250..0x0055C12A, vtable 0x00609158). Fields sit at feb + 0x1C (entENTITY
// grew). 1.2 changes, from the disassembly:
//   - the range test is one function, IsInRange (NPCs measured from their GetCenter); the one-way test there lets
//     an open door stay open while the player is behind it (feb: IsPlrBeforeDoor); the "locked" notify is sent
//     when a player is within 3.0 (feb: half radOpen);
//   - "unlocked" goes through the clients like "locked" (notify 2) instead of straight to the HUD;
//   - the notify message is packed: { type u16 at +4, flags +6, sequence +0xA, time +0xE } (feb: dwords);
//   - a fix for level 7_5: the door instance "Actor_OFF" unlocks 5 s after the domain "&dom_kill" lost its locks;
//   - TERM is ProcessTERM (empty), INIT_LEVEL is handled (the 7_5 fix), the slots moved (subclass slots from +0x64).

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"

extern float gsElapsedTime;                 // v12 0x0098E9F4

float doorDefaultRadOpen = 18.0f;           // v12 0x00637944 (feb 15.0)
static float doorLockMsgRad = 3.0f;         // v12 0x00637948
static const float doorEps = 1e-6f;         // v12 0x00609154
float doorEpsSq;                            // v12 0x009AE77C (static init v12 0x0055C0B0)
static struct doorEPS_INIT { doorEPS_INIT() { doorEpsSq = doorEps * doorEps; } } doorEpsInit;

static entIACTIVE_OBJ *doorKillDom;         // v12 0x009AE798 "&dom_kill" of lev_7_5 (name ours)
static float doorKillTime;                  // v12 0x009AE780 (name ours)

#pragma pack(push, 1)
struct doorSYNC : msgDATA {                 // the sync block (GET/READ_SYNC_DATA), v12 0x009AE788, size 0x10
    u32   flags;                            // +0x04
    int   seq;                              // +0x08
    float time;                             // +0x0C
};
struct doorNOTIFY : msgDATA {               // GS_MSG_MP_NOTIFY 0x40E to the clients
    short type;                             // +0x04 -100 sync block, 0 state, 1 locked, 2 unlocked
    union {
        doorSYNC *sync;                     // +0x06 (type -100)
        struct {
            u32   flags;                    // +0x06
            int   seq;                      // +0x0A
            float time;                     // +0x0E
        };
    };
};
#pragma pack(pop)
static doorSYNC doorSync;
static int doorSyncInited;                  // v12 0x009AE784 bit 0

static void doorSyncInit()
{
    if (!(doorSyncInited & 1)) {
        doorSyncInited |= 1;
        doorSync.packId = 0;
        doorSync.tag = 0x40;
        doorSync.flags = 0;
        doorSync.seq = -1;
        doorSync.size = 0x10;
    }
}

// v12 0x0055B250  (feb 0x00543B00)
int __fastcall doorInit(void)
{
    return entRegisterClass((char *)"door", 'DOOR', 0x28, doorDOOR::Create, 0, 1) != 0;
}

// v12 0x0055B290  (feb 0x00543B40)
BOOL32 doorDOOR::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 1000:                              // FRAME
        ProcessFRAME();
        return 0;
    case MSG_MSG_INIT:
        ProcessINIT();
        return 0;
    case MSG_MSG_TERM:
        ProcessTERM();
        return 0;
    case 0x3F4: {                           // INIT_LEVEL: the 7_5 fix finds its domain
        doorKillDom = 0;
        const char *n = inst->name ? inst->name : "";
        if (strcmp(n, "Actor_OFF") || !strstr(gsSceneFullName, "lev_7_5_"))
            return 0;
        for (doorKillDom = entFindNextIO(0); doorKillDom; doorKillDom = entFindNextIO(doorKillDom))
            if (!strcmp(doorKillDom->name.str, "&dom_kill")) {
                doorKillTime = 0.0f;
                return 0;
            }
        return 0;
    }
    case 0x3F8: {                           // IACTION
        apNAME *a = (apNAME *)((char *)data + 8);
        if (a->IsName((char *)"OPEN")) {
            flags |= DOORF_CMD_OPEN;
            flags &= ~DOORF_CMD_CLOSE;
        } else if (a->IsName((char *)"CLOSE")) {
            flags |= DOORF_CMD_CLOSE;
            flags &= ~DOORF_CMD_OPEN;
        }
        return 0;
    }
    case 0x40E:                             // MP_NOTIFY
        ProcessMP_NOTIFY((msgDATA *)data);
        return 0;
    }
    return 0;
}

// v12 0x0055B410  (feb 0x00543C10)
// Needs the sequences OPEN and CLOSE; OPEN is stretched to the length of CLOSE. Starts closed.
BOOL32 doorDOOR::ProcessINIT()
{
    seqOpen = inst->tpl->FindSeqName((char *)"OPEN");
    if (seqOpen == -1)
        return 0;
    seqClose = inst->tpl->FindSeqName((char *)"CLOSE");
    if (seqClose == -1)
        return 0;
    float lc = inst->GetSeqLength(seqClose);
    closeLen = lc;
    if (!(fabsf(lc - inst->GetSeqLength(seqOpen)) < doorEps))
        inst->tpl->seq[seqOpen].lengthMs = closeLen * 1000.0f;
    inst->SetAnimSeq(seqClose, closeLen);
    flags |= DOORF_INITED;
    if (inst->flags & 0x40)
        priority = 0x28;
    int b;
    inst->GetInt((char *)"DOOR", (char *)"nmbLocks", &nLocks);
    if (inst->GetBool((char *)"DOOR", (char *)"isOneWay", &b) && b)
        flags |= DOORF_ONE_WAY;
    if (inst->GetBool((char *)"DOOR", (char *)"isOpenOnce", &b) && b)
        flags |= DOORF_OPEN_ONCE;
    radOpen = doorDefaultRadOpen;
    inst->GetFloat((char *)"DOOR", (char *)"radOpen", &radOpen);
    if (inst->GetBool((char *)"DOOR", (char *)"isOpen", &b) && b)
        flags |= DOORF_CMD_OPEN;
    if (inst->GetBool((char *)"DOOR", (char *)"isOpenForever", &b) && b)
        flags |= DOORF_OPEN_FOREVER;
    gsSysServer->DeclSyncable(this);
    return 1;
}

// v12 0x0055B670  (feb 0x00543E60)
// Opens (OPEN played from the mirrored point of CLOSE) while IsReadyOpen, else closes. Events: 1 starts opening,
// 2 closed, 0x20 an isOpenOnce door closed behind every player. A change of state goes to the clients.
void doorDOOR::ProcessFRAME()
{
    if (entFlags & ENTF_REMOTE)
        inst->AdvanceTime(gsElapsedTime, 1, 0);
    else {
        u32 oldFlags = flags;
        int oldSeq = inst->seq;
        float t = inst->seqTime;
        const char *n = inst->name ? inst->name : "";
        if (!strcmp(n, "Actor_OFF") && doorKillDom && doorKillDom->nLocks <= 0) {
            doorKillTime += gsElapsedTime;
            if (doorKillTime > 5.0f)
                nLocks = 0;
        }
        if (IsReadyOpen()) {
            if (flags & DOORF_INITED)
                pendingEvents |= 1;
            flags &= ~DOORF_INITED;
            flags |= DOORF_OPEN_STATE;
            flags |= DOORF_NPC_OPEN;
            if (oldSeq != seqOpen)
                inst->SetAnimSeq(seqOpen, closeLen - t);
            inst->AdvanceTime(gsElapsedTime, 1, 0);
        } else {
            if (oldSeq != seqClose)
                inst->SetAnimSeq(seqClose, closeLen - t);
            if (inst->AdvanceTime(gsElapsedTime, 1, 0) && (flags & DOORF_OPEN_STATE)) {
                if (flags & DOORF_OPEN_ONCE) {
                    entENTITY *p;
                    for (p = gsSysServer->GetNextPlayer(0); p; p = gsSysServer->GetNextPlayer(p)) {
                        m3dV pp, dp, dir, d;
                        p->inst->GetPos(&pp);
                        inst->GetPos(&dp);
                        inst->GetDir(&dir);
                        d.x = dp.x - pp.x;
                        d.y = dp.y - pp.y;
                        d.z = dp.z - pp.z;
                        if (m3dAngleVector(&d, &dir) < 90.0f)
                            break;
                    }
                    if (!p) {
                        flags |= DOORF_LOCKED_HOLD;
                        pendingEvents |= 0x20;
                    }
                }
                pendingEvents |= 2;
                flags &= ~DOORF_OPEN_STATE;
                flags |= DOORF_INITED;
            }
        }
        if (oldFlags != flags || oldSeq != inst->seq) {
            doorNOTIFY m;
            m.packId = 0;
            m.tag = 0x40;
            m.size = 0x12;
            m.type = 0;
            m.flags = flags;
            m.seq = inst->seq;
            m.time = inst->seqTime;
            gsSysServer->SendMsgClientAll(this, 0x40E, &m, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
        }
    }
    if (entFlags & 4)
        lockMsgTimer += gsElapsedTime;
    if (entFlags & 2)
        lockShowTimer += gsElapsedTime;
}

// v12 0x0055B990  (feb 0x00544120)
// Open when a player is in range (a door with locks instead tells that player "locked", at most every 2 s, when
// within 3.0), when commanded OPEN, or (NPC doors not commanded CLOSE) when an NPC is in range. Open-forever doors
// stay open; a door held by an isOpenOnce close never opens.
BOOL32 doorDOOR::IsReadyOpen()
{
    u32 f = flags;
    if (f & DOORF_LOCKED_HOLD)
        return 0;
    if ((f & DOORF_OPEN_STATE) && (f & DOORF_OPEN_FOREVER))
        return 1;
    m3dV pos;
    inst->GetPos(&pos);
    BOOL32 any = 0;
    for (entENTITY *p = gsSysServer->GetNextPlayer(0); p; p = gsSysServer->GetNextPlayer(p)) {
        BOOL32 in = IsInRange(p, radOpen);
        any |= in;
        if (nLocks > 0 && in && IsInRange(p, doorLockMsgRad) && lockMsgTimer > 2.0f) {
            lockMsgTimer = 0.0f;
            doorNOTIFY m;
            m.packId = 0;
            m.tag = 0x40;
            m.size = 6;
            m.type = 1;
            gsSysServer->SendMsgClient(p->mpOwner, this, 0x40E, &m, 0x10000);
        }
    }
    if (nLocks > 0)
        return 0;
    if (any) {
        flags &= ~DOORF_CMD_OPEN;
        flags &= ~DOORF_CMD_CLOSE;
        return 1;
    }
    if (flags & DOORF_CMD_OPEN)
        return 1;
    if (!(flags & DOORF_CMD_CLOSE) && (flags & DOORF_NPC_OPEN))
        for (aiNPC *a = aiNPC::GetNext(0, 0); a; a = aiNPC::GetNext(a, 0))
            if (IsInRange(a, radOpen))
                return 1;
    return 0;
}

// v12 0x0055BB20 (1.2 only; name ours)
// e within rad of the door horizontally and rad/2 vertically. One-way doors: not while e is behind the door,
// unless the door is open and not closing.
BOOL32 doorDOOR::IsInRange(entENTITY *e, float rad)
{
    BOOL32 in = 0;
    m3dV ep, dp;
    if (e->classId == AI_SIG_NPC)
        e->GetCenter(&ep);
    else
        e->inst->GetPos(&ep);
    inst->GetPos(&dp);
    float dx = ep.x - dp.x, dz = ep.z - dp.z;
    float dist = m3dSqrt(dx * dx + dz * dz);
    if (dist < rad && fabsf(ep.y - dp.y) < rad * 0.5f)
        in = 1;
    if (flags & DOORF_ONE_WAY) {
        m3dV pp, dir, d;
        e->inst->GetPos(&pp);
        inst->GetPos(&dp);
        inst->GetDir(&dir);
        d.x = dp.x - pp.x;
        d.y = dp.y - pp.y;
        d.z = dp.z - pp.z;
        if (m3dAngleVector(&d, &dir) < 90.0f) {
            if (!(flags & DOORF_OPEN_STATE) || inst->seq == seqClose)
                return 0;
        }
    }
    return in;
}

// v12 0x0055BD10  (feb 0x00544710)
// From the server: -100 the whole state (sync block), 0 state + sequence, 1 "locked" (shown at most every 8 s),
// 2 "unlocked".
void doorDOOR::ProcessMP_NOTIFY(msgDATA *ev)
{
    doorNOTIFY *m = (doorNOTIFY *)ev;
    switch (m->type) {
    case -100:
        flags = m->sync->flags;
        inst->SetAnimSeq(m->sync->seq, m->sync->time);
        break;
    case 0:
        flags = m->flags;
        inst->SetAnimSeq(m->seq, m->time);
        break;
    case 1:
        if (lockShowTimer > 8.0f) {
            if (uiSystem)
                uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId("GAME_MSG_DOOR_LOCKED")),
                                   2.0f, 0);
            flags |= DOORF_SHOWN_LOCKED;
            lockShowTimer = 0.0f;
        }
        break;
    case 2:
        if (uiSystem)
            uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId("GAME_MSG_DOOR_UNLOCKED")),
                               1.5f, 0);
        break;
    }
}

// v12 0x0055BEB0  (feb 0x005447F0)
void doorDOOR::ProcessMP_GET_SYNC_DATA(void *d)
{
    msgDATA **out = (msgDATA **)d;
    if (!*out) {
        doorSyncInit();
        *out = &doorSync;
    }
    doorSYNC *s = (doorSYNC *)*out;
    s->flags = flags;
    s->seq = inst->seq;
    s->time = inst->seqTime;
    entENTITY::ProcessMP_GET_SYNC_DATA(d);
}

// v12 0x005567C0  (feb 0x00544880; the linker folded it with bttnBUTTON's identical function)
// The sync block as chunks 0xF0 flags, 0xF1 sequence, 0xF2 time.
void doorDOOR::ProcessMP_WRITE_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    doorSYNC *s = (doorSYNC *)d->data;
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

// v12 0x0055BF40  (feb 0x00544970)
// Reads chunks 0xF0..0xF2 into the sync block (the door's own when d->data is 0) until the end chunk.
void doorDOOR::ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    doorSYNC *s = (doorSYNC *)d->data;
    if (!s) {
        doorSyncInit();
        s = &doorSync;
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

// v12 0x0055C060  (feb 0x00544A90)
// The last lock gone: every client shows "unlocked".
void doorDOOR::ProcessUNLOCK()
{
    if (nLocks)
        return;
    doorNOTIFY m;
    m.packId = 0;
    m.tag = 0x40;
    m.size = 6;
    m.type = 2;
    gsSysServer->SendMsgClientAll(this, 0x40E, &m, 0x90000, 0xFFFFFFFD, 0xFFFFFFFD);
}

// v12 0x00403A20 (the shared empty function)
void doorDOOR::ProcessTERM()
{
}

// v12 0x0055C0D0  (feb 0x00544B00)
entENTITY *__fastcall doorDOOR::Create(animINST *)
{
    doorDOOR *d = new doorDOOR;             // operator new(0x187), entENTITY ctor
    if (!d)
        return 0;
    d->flags = 0;
    d->seqOpen = -1;
    d->seqClose = -1;
    d->radOpen = 0.0f;
    d->closeLen = 0.0f;
    d->lockMsgTimer = 0.0f;
    d->lockShowTimer = 0.0f;
    return d;
}
