// msgaddr.cpp -- engine.lib message addresses: the global list of addresses (every entity, domain, system) and
// walking it (Will Rock 1.2, v12.exe; feb obj engine:msgaddr.obj)
//
// msgSystem and its queues: the address list (msgADDR_LIST, sorted by priority), the two 500-message arrays
// (posted this frame / dispatched this frame) and the time queue; InsertAddr / RemoveAddr / DestroyAddr and the
// purge; SendMsg, SendAll, PostMsg / PostTimeMsg (with data), DispatchAll and NextFrame. Also the msgADDR
// accessors that 1.2 emitted out of line (GetSign 0x005089C0, GetNext 0x005089D0, GetClassID 0x00548C90).
// An address is linked into msgSYSTEM::addrList through its node at +0x04 (next node at +0x08).
// Not yet: Pack / UnPack and the msgDATA_PACK default packer bodies.

#include "engine.h"
#include <stdlib.h>
#include <string.h>

msgSYSTEM    msgSystem;           // v12 0x008FA960 (feb 0x008B6720)
msgDATA_PACK msgDataPackDefault;  // v12 0x008FB194
int          msgInPurgeDelete;    // v12 0x008FB19C (feb 0x008B6F5C)

// msgADDR +0x24: zeroed by every msgADDR constructor (inline), declared by the classes that use it
// (entIACTIVE_OBJ::unk24, its lock count). While it is > 0 the address gets only the change-mode broadcast,
// and its delayed messages do not count down.
static inline int msgAddrLock(msgADDR *a)
{
    return *(int *)((char *)a + 0x24);
}

static inline msgMSG_TIME *msgTimeOf(lstNODE *n)
{
    return n ? (msgMSG_TIME *)((char *)n - 0x14) : 0;
}

// v12 0x004CFBA0  static initialiser (atexit 0x004CFC40)
msgSYSTEM::msgSYSTEM()
{
    cur = 0;
    next = 0;
    dispatchIdx = 0;
    curCount = 0;
    nextCount = 0;
    capacity = 0;
    unk18 = 0;
    frameNmb = 0;
    nRegMsg = 0;
    unk690 = -1;
    nPendingDestroy = 0;
    chgModeMsgId = -1;
    purgeMsgId = -1;
}

// v12 0x004CFC40
msgSYSTEM::~msgSYSTEM()
{
    timeQueue.Clear();
    operator delete(cur);
    cur = 0;
    operator delete(next);
    next = 0;
}

// v12 0x004CFD60  (feb 0x004BE2B0)
int msgADDR_LIST::Compare(lstNODE *a, lstNODE *b)
{
    msgADDR *x = a ? (msgADDR *)((char *)a - 4) : 0;
    msgADDR *y = b ? (msgADDR *)((char *)b - 4) : 0;
    return x->priority < y->priority;
}

// v12 0x004CFCD0  (feb 0x004BE220)
// Number the address within its class id (+0x18), then the sorted insert.
void msgADDR_LIST::Insert(lstNODE *n)
{
    msgADDR *a = n ? (msgADDR *)((char *)n - 4) : 0;
    int i;
    for (i = 0; i < nClass; i++)
        if (cls[i].classId == a->classId)
            break;
    if (i == nClass) {
        cls[nClass].classId = a->classId;
        cls[nClass].n = 0;
        nClass++;
    }
    a->unk18 = cls[i].n;
    lstPLIST::Insert(n);
    cls[i].n++;
}

// v12 0x004CFDC0  (feb 0x004BE330)  msgQUEUE::DeleteMsgByAddr
// Messages to or from a are not dropped: their to / from become 0 (DispatchAll skips to == 0).
void msgSYSTEM::DeleteMsgByAddr(msgADDR *a)
{
    for (int i = dispatchIdx; i < curCount; i++) {
        if (cur[i].to == a)
            cur[i].to = 0;
        if (cur[i].from == a)
            cur[i].from = 0;
    }
    for (int i = 0; i < nextCount; i++) {
        if (next[i].to == a)
            next[i].to = 0;
        if (next[i].from == a)
            next[i].from = 0;
    }
}

// v12 0x004CFE90  (feb 0x004BE400)
void msgTIME_QUEUE::DeleteMsgByAddr(msgADDR *a)
{
    msgMSG_TIME *m = msgTimeOf(head);
    while (m) {
        if (m->to == a) {
            msgMSG_TIME *d = m;
            m = msgTimeOf(d->node.next);
            if (d->time <= 0.0f)
                nReady--;
            if (cursor == d)
                cursor = msgTimeOf(d->node.next);
            Remove(&d->node);
            operator delete(d);
        } else {
            if (m->from == a)
                m->from = 0;
            m = msgTimeOf(m->node.next);
        }
    }
}

// v12 0x004CFF20
void *msgTIME_QUEUE::__vecDelDtor(unsigned flags)
{
    Clear();
    if (flags & 1)
        operator delete(this);
    return this;
}

// v12 0x004CFE70
static int _msgQSortCompare(const void *a, const void *b)
{
    msgADDR *x = ((msgMSG *)a)->to, *y;
    if (x && (y = ((msgMSG *)b)->to) != 0)
        return x->priority - y->priority;
    return 0;
}

// v12 0x004CFF50  (feb 0x004BE4C0)
BOOL32 msgSYSTEM::Init()
{
    msgMSG *q = (msgMSG *)operator new(500 * sizeof(msgMSG));
    if (q)
        for (int i = 0; i < 500; i++) {
            q[i].to = 0;
            q[i].from = 0;
            q[i].id = -1;
            q[i].data = 0;
            q[i].unk10 = 0;
        }
    cur = q;
    if (!q)
        return 0;
    q = (msgMSG *)operator new(500 * sizeof(msgMSG));
    if (q)
        for (int i = 0; i < 500; i++) {
            q[i].to = 0;
            q[i].from = 0;
            q[i].id = -1;
            q[i].data = 0;
            q[i].unk10 = 0;
        }
    next = q;
    if (!q)
        return 0;
    capacity = 500;
    msgSystem.RegisterMsg(MSG_MSG_INIT, (char *)"MSG_MSG_INIT");     // (inline, on the global)
    msgSystem.RegisterMsg(MSG_MSG_TERM, (char *)"MSG_MSG_TERM");
    msgSystem.RegisterMsg(MSG_MSG_DESTROY_ADDR, (char *)"MSG_MSG_DESTROY_ADDR");
    if (msgDataPackDefault.id <= 0 && nDataPack < 100) {             // RegisterMsgData, inline
        msgDataPackDefault.id = nDataPack;
        dataPack[nDataPack] = &msgDataPackDefault;
        nDataPack++;
    }
    return 1;
}

// v12 0x004D00D0  (feb 0x004BE640)
void msgSYSTEM::Term()
{
    operator delete(cur);
    cur = 0;
    operator delete(next);
    next = 0;
    PurgeDestroyAddrAll();
}

// v12 0x004D01A0  (feb 0x004BE710)
// The delayed messages count down by dt and the ones that ran out are dispatched (in queue order), then this
// frame's array. (A ready count with no ready message left would loop for ever, original.)
void msgSYSTEM::DispatchAll(float dt)
{
    int ret = 0;
    timeQueue.nReady = 0;
    timeQueue.cursor = msgTimeOf(timeQueue.head);
    for (msgMSG_TIME *m = timeQueue.cursor; m; m = msgTimeOf(m->node.next)) {
        if (msgAddrLock(m->to) > 0)
            continue;
        m->time -= dt;
        if (m->time <= 0.0f)
            timeQueue.nReady++;
    }
    while (timeQueue.nReady > 0) {
        while (timeQueue.cursor && timeQueue.cursor->time > 0.0f)
            timeQueue.cursor = msgTimeOf(timeQueue.cursor->node.next);
        msgMSG_TIME *t = timeQueue.cursor;
        if (!t)
            continue;
        timeQueue.cursor = msgTimeOf(t->node.next);
        timeQueue.Remove(&t->node);
        timeQueue.nReady--;
        Dispatch(t->to, t->id, t->data, t->from, &ret);
        t->to = 0;
        t->from = 0;
        t->id = -1;
        t->time = 0.0f;
        apFree(t->data);
        t->data = 0;
        operator delete(t);
    }
    while (curCount - dispatchIdx > 0) {
        if (!cur[dispatchIdx].to)
            while (dispatchIdx < curCount && !cur[++dispatchIdx].to)
                ;
        if (dispatchIdx < curCount) {
            msgMSG *q = &cur[dispatchIdx++];
            Dispatch(q->to, q->id, q->data, q->from, &ret);
            q->to = 0;
            q->from = 0;
            q->id = -1;
            q->unk10 = 0;
            apFree(q->data);
            q->data = 0;
        }
    }
}

// v12 0x004D0340
// INIT reaches an address once; a dead one gets nothing, a terminating one only TERM. PreProcessMsg, then
// ProcessMsg, then PostProcessMsg, each only while the one before returned 0.
void msgSYSTEM::Dispatch(msgADDR *to, int id, void *data, msgADDR *from, int *ret)
{
    if (id == MSG_MSG_INIT && (to->flags & MSGADDR_INITED))
        return;
    if (to->flags & MSGADDR_DEAD)
        return;
    if ((to->flags & MSGADDR_TERMINATING) && id != MSG_MSG_TERM)
        return;
    if (to->PreProcessMsg(id, data, from, ret))
        return;
    if (to->ProcessMsg(id, data, from, ret))
        return;
    to->PostProcessMsg(id, data, from, ret);
    if (id == MSG_MSG_INIT)
        to->flags |= MSGADDR_INITED;
}

// v12 0x004D03C0  (feb 0x004BE930)
BOOL32 msgSYSTEM::SendMsg(msgADDR *to, int id, void *data, msgADDR *from)
{
    int ret = 0;
    if ((to->flags & MSGADDR_TERMINATING) && id != MSG_MSG_TERM)
        return 0;
    Dispatch(to, id, data, from, &ret);
    return ret;
}

// v12 0x004D09D0  (feb 0x004BEF40)
msgADDR *msgSYSTEM::GetFirstAddr()
{
    char *n = *(char **)((char *)this + 0x20);
    return n ? (msgADDR *)(n - 4) : 0;
}

// v12 0x005089D0  (feb 0x004F63B0)
msgADDR *msgADDR::GetNext()
{
    char *n = *(char **)((char *)this + 8);
    return n ? (msgADDR *)(n - 4) : 0;
}

// v12 0x005089C0  (feb 0x004F63A0)
u32 msgADDR::GetSign()
{
    return sign;
}

// v12 0x00548C90  (feb 0x004F6390)
u32 msgADDR::GetClassID()
{
    return classId;
}

// v12 0x004D0410  (feb 0x004BE980)
// Queue a message without data for the next frame. Nothing reaches an address being terminated; posting
// MSG_MSG_TERM marks it so. (No overflow check on the 500-entry queue, original.)
void msgSYSTEM::PostMsg(msgADDR *to, int id, msgADDR *from)
{
    if (to->flags & MSGADDR_TERMINATING)
        return;
    msgMSG *m = &next[nextCount];
    m->from = from;
    m->to = to;
    m->id = id;
    m->unk10 = 0;
    nextCount++;
    if (id == MSG_MSG_TERM)
        to->flags |= MSGADDR_TERMINATING;
}

// v12 0x004D0460  (feb 0x004BE9D0)
void msgSYSTEM::PostTimeMsg(msgADDR *to, int id, msgADDR *from, float delay)
{
    if (to->flags & MSGADDR_TERMINATING)
        return;
    msgMSG_TIME *m = (msgMSG_TIME *)operator new(sizeof(msgMSG_TIME));
    if (!m)
        return;
    m->data = 0;
    m->node.prev = 0;
    m->node.next = 0;
    m->to = to;
    m->from = from;
    m->id = id;
    m->time = delay;
    timeQueue.Insert(&m->node);
}

// v12 0x004D04C0  (feb 0x004BEA30)
// Like PostMsg with a copy of d (its size at +2). (No overflow check, original.)
void msgSYSTEM::PostMsgData(msgADDR *to, int id, msgDATA *d, msgADDR *from)
{
    if (to->flags & MSGADDR_TERMINATING)
        return;
    msgMSG *m = &next[nextCount];
    m->to = to;
    m->from = from;
    m->id = id;
    m->unk10 = 0;
    if (d) {
        int size = ((msgHDR *)d)->size;
        m->data = apMalloc(size);
        if (m->data)
            memcpy(m->data, d, size);
    }
    nextCount++;
    if (id == MSG_MSG_TERM)
        to->flags |= MSGADDR_TERMINATING;
}

// v12 0x004D0570  (feb 0x004BEAE0)
void msgSYSTEM::PostTimeMsgData(msgADDR *to, int id, msgDATA *d, msgADDR *from, float delay)
{
    if (to->flags & MSGADDR_TERMINATING)
        return;
    msgMSG_TIME *m = (msgMSG_TIME *)operator new(sizeof(msgMSG_TIME));
    if (!m)
        return;
    m->data = 0;
    m->node.prev = 0;
    m->node.next = 0;
    m->to = to;
    m->from = from;
    m->id = id;
    m->time = delay;
    if (d) {
        int size = ((msgHDR *)d)->size;
        m->data = apMalloc(size);
        if (m->data)
            memcpy(m->data, d, size);
    }
    timeQueue.Insert(&m->node);
}

// v12 0x004D0650  (feb 0x004BEBC0)
// (The delayed messages are unlinked, not freed, original.)
void msgSYSTEM::DeleteAllMsg()
{
    for (int i = dispatchIdx; i < curCount; i++)
        cur[i].to = 0;
    curCount = 0;
    dispatchIdx = 0;
    for (int i = 0; i < nextCount; i++)
        next[i].to = 0;
    nextCount = 0;
    timeQueue.Clear();
    timeQueue.nReady = 0;
    timeQueue.cursor = 0;
}

// v12 0x004D06B0  (feb 0x004BEC20)
// This frame's posts become the next frame's dispatch, sorted by the receivers' priority.
void msgSYSTEM::NextFrame()
{
    msgMSG *q = cur;
    int n = nextCount;
    cur = next;
    next = q;
    dispatchIdx = 0;
    nextCount = 0;
    curCount = n;
    qsort(cur, n, sizeof(msgMSG), _msgQSortCompare);
    frameNmb++;
}

// v12 0x004D06F0  (feb 0x004BEC60)
// Every address in list order. If the next one was deleted meanwhile (no 'MADR' tag), it starts over from the
// first. Sending purgeMsgId (GS_MSG_FRAME) ends with the destroy purge (inline copy of PurgeDestroyAddrAll).
void msgSYSTEM::SendAll(int id, void *data, msgADDR *from)
{
    msgADDR *a = GetFirstAddr();
    while (a) {
        msgADDR *n = a->GetNext();
        if (msgAddrLock(a) <= 0 || id == chgModeMsgId) {
            int ret = 0;
            if (!(a->flags & MSGADDR_TERMINATING) || id == MSG_MSG_TERM)
                Dispatch(a, id, data, from, &ret);
        }
        if (n && n->magic != 0x4D414452)
            a = GetFirstAddr();
        else
            a = n;
    }
    if (id == purgeMsgId)
        PurgeDestroyAddrAll();
}

// v12 0x004D0820  (feb 0x004BED90)
void msgSYSTEM::InsertAddr(msgADDR *a, u32 classId, u32 priority)
{
    if (classId)
        a->classId = classId;
    if (priority)
        a->priority = priority;
    addrList.Insert(a ? &a->node : 0);
}

// v12 0x004D0860  (feb 0x004BEDD0)
void msgSYSTEM::RemoveAddr(msgADDR *a)
{
    DeleteMsgByAddr(a);
    timeQueue.DeleteMsgByAddr(a);
    addrList.Remove(a ? &a->node : 0);
}

// v12 0x004D08A0  (feb 0x004BEE10)
// Tells the addresses that asked (MSGADDR_WANT_DESTROY_NOTIFY) and marks a; the purge deletes it.
void msgSYSTEM::DestroyAddr(msgADDR *a)
{
    for (msgADDR *x = GetFirstAddr(); x; x = x->GetNext()) {
        if (x == a || !(x->flags & MSGADDR_WANT_DESTROY_NOTIFY))
            continue;
        int ret = 0;
        if (!(x->flags & MSGADDR_TERMINATING))
            Dispatch(x, MSG_MSG_DESTROY_ADDR, a, a, &ret);
    }
    a->flags |= MSGADDR_TERMINATING | MSGADDR_DEAD;
    nPendingDestroy++;
}

// v12 0x004D0920  (feb 0x004BEE90)
// Deletes every dead address (scalar deleting destructor), again while a pass deleted some.
void msgSYSTEM::PurgeDestroyAddrAll()
{
    while (nPendingDestroy > 0) {
        int done = 0;
        msgADDR *a = GetFirstAddr();
        while (a) {
            msgADDR *n = a->GetNext();
            if (a->flags & MSGADDR_DEAD) {
                done = 1;
                DeleteMsgByAddr(a);
                timeQueue.DeleteMsgByAddr(a);
                addrList.Remove(&a->node);
                msgInPurgeDelete = 1;
                delete a;
                msgInPurgeDelete = 0;
                nPendingDestroy--;
            }
            a = n;
        }
        if (!done)
            break;
    }
}

// v12 0x004D09E0  (feb 0x004BEF50)
// A message id's name for the debug log (no bound check on the 100 entries, original).
void msgSYSTEM::RegisterMsg(int id, char *name)
{
    regMsg[nRegMsg].id = id;
    regMsg[nRegMsg].name = name;
    nRegMsg++;
}

// v12 0x004D0A20  (feb 0x004BEF90)
msgMSG_TIME *msgSYSTEM::GetNextTimeMsg(msgMSG_TIME *prev)
{
    return msgTimeOf(prev ? prev->node.next : timeQueue.head);
}

// v12 0x004CFE50  (feb 0x004BE3C0)
msgADDR::~msgADDR()
{
}

// v12 0x004802A0  (feb 0x00499C50) the three base handlers, folded "return 0"
BOOL32 msgADDR::ProcessMsg(int, void *, msgADDR *, int *)
{
    return 0;
}

BOOL32 msgADDR::PreProcessMsg(int, void *, msgADDR *, int *)
{
    return 0;
}

BOOL32 msgADDR::PostProcessMsg(int, void *, msgADDR *, int *)
{
    return 0;
}

// v12 0x004D0A50
// No data, or a header tagged 0x40 with a size.
BOOL32 msgSYSTEM::IsValidMsgData(void *d)
{
    if (!d)
        return 1;
    msgHDR *h = (msgHDR *)d;
    return h->flags == 0x40 && (short)h->size > 0;
}

// v12 0x004D0A80  (feb 0x004BEFF0)
// A packer gets the next index once (limit 100).
void msgSYSTEM::RegisterMsgData(msgDATA_PACK *p)
{
    if (p->id > 0 || nDataPack >= 100)
        return;
    p->id = nDataPack;
    dataPack[nDataPack] = p;
    nDataPack++;
}

// v12 0x004D0AC0  (feb 0x004BF030)
// One byte, the packer index (0xFF: no data), then the packer's bytes.
BOOL32 msgSYSTEM::Pack(fioFILE_MEM *f, msgDATA *d)
{
    if (!d) {
        u8 none = 0xFF;
        f->WriteData(&none, 1, 1);
        return 1;
    }
    f->WriteData(d, 1, 1);
    return dataPack[*(u8 *)d]->Pack(f, d);
}

// v12 0x004D0B20  (feb 0x004BF090)
msgDATA *msgSYSTEM::UnPack(fioFILE_MEM *f)
{
    u8 type;
    f->ReadData(&type, 1, 1);
    if (type == 0xFF)
        return 0;
    return dataPack[type]->UnPack(f);
}

// v12 0x004D0B60  (feb 0x004BF0D0)
// The size (2 bytes) and the whole data (header included). A size of -1 is not packed.
BOOL32 msgDATA_PACK::Pack(fioFILE_MEM *f, msgDATA *d)
{
    short size = ((msgHDR *)d)->size;
    if (size == -1)
        return 0;
    f->WriteData(&size, 2, 2);
    f->WriteData(d, size, 1);
    return 1;
}

// v12 0x004D0BB0  (feb 0x004BF120)
// The data is used in place in the file buffer (not copied).
msgDATA *msgDATA_PACK::UnPack(fioFILE_MEM *f)
{
    short size;
    f->ReadData(&size, 2, 2);
    msgDATA *d = (msgDATA *)f->GetCurPtr();
    f->Seek(size, FIO_SEEK_CUR);
    return d;
}
