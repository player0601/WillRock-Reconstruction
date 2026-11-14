// dynamic.cpp -- dynDYNAMIC, the "dynamic" entity class (Will Rock 1.2, v12.exe; feb obj gs:dynamic.obj)
//
// v12 0x0050C3C0..0x0050D020. An animated level object: plays its template's sequences on the IActions START (with a
// sequence name, cycling or once) / STOP / TOGGLE, optionally from the start ([DYNAMIC] isStartAnim / isCycleAnim).
// gsLVL_SYSTEM::LoadInstAll makes every placed template instance without a class one of these (or its template's
// class), so the level's actors get an entity -- and through entCreate their collision (animINST::CDTInit).
// 1.2 changes, from the disassembly: two new fields first (a client's animation catch-up: ProcessFRAME advances by
// the frame plus a part of timeDebt); every message goes to entENTITY::ProcessMsg afterwards; INIT asks a 5 s MP
// update period; the server notifies stops of a single long sequence (DYNF_LONG_SINGLE_SEQ, entFlags 0x80); the
// per-frame MP sync of a cycling animation and the 0x40E case 2 are gone.
// Not rebuilt: the MP slots (+0x1C GetMP_UPD_STATE 0x0050CA00, +0x20 0x0050CAA0, +0x34 / +0x38 / +0x3C sync data).

#include "game.h"
#include "../gs/gs_mp_server.h"

#pragma pack(push, 1)
struct dynMP_MSG {                  // the server's notify to the clients (0x40E at the clients)
    u8    tag0, tag1;               // 0, 0x40
    u16   size;                     // 0x16 started, 0x12 stopped
    int16_t   type;                     // 0 started, 1 stopped (-100: a copy, data at +6)
    u32   dynFlags;                 // +0x06
    int   seq;                      // +0x0A
    float time;                     // +0x0E
    int   animMode;                 // +0x12 (started only)
};
#pragma pack(pop)

// v12 0x0050C3C0  (feb 0x004F9920)
int __fastcall dynInit()
{
    return entRegisterClass((char *)"dynamic", 0x2444594E, 0x28, dynDYNAMIC::Create, 0, 1) != 0;
}

// v12 0x0050C400  (feb 0x004F9960)
// INIT / FRAME / TERM to the own slots; the clients' copy of a start or stop (0x40E); the IActions START <seq>
// (cycle, time from the action's data) / STOP / TOGGLE (stop, or go on with the current sequence where it is).
BOOL32 dynDYNAMIC::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    u8 *d = (u8 *)data;
    if (id <= 1000) {
        if (id == 1000)
            ProcessFRAME();
        else if (id == 1)
            ProcessINIT(data);
        else if (id == 2)
            ProcessTERM();
    } else if (id == 0x40E) {
        int type = *(int16_t *)(d + 4);
        if (type == -100) {
            u8 *m = *(u8 **)(d + 6);
            dynFlags = *(u32 *)(m + 4);
            inst->SetAnimSeq(*(int *)(m + 8), *(float *)(m + 0xC));
            animMode = *(int *)(m + 0x10);
        } else if (type == 0) {
            dynFlags = *(u32 *)(d + 6);
            inst->SetAnimSeq(*(int *)(d + 0xA), *(float *)(d + 0xE));
            animMode = *(int *)(d + 0x12);
        } else if (type == 1) {
            dynFlags = *(u32 *)(d + 6);
            inst->SetAnimSeq(*(int *)(d + 0xA), *(float *)(d + 0xE));
        }
    } else if (id == 0x3F8) {                           // IACTION
        apNAME *a = (apNAME *)(d + 8);
        if (a->IsName((char *)"START")) {
            int s = inst->tpl->FindSeqName((char *)(d + 0x60));
            if (s != -1)
                StartAnim(s, *(float *)(d + 0x50), *(int *)(d + 0x58));
        } else if (a->IsName((char *)"STOP")) {
            StopAnim();                                 // (inline in the original)
        } else if (a->IsName((char *)"TOGGLE")) {
            if (dynFlags & DYNF_PLAYING)
                StopAnim();
            else
                StartAnim(inst->seq, inst->seqTime, animMode == 0);
        }
    }
    return entENTITY::ProcessMsg(id, data, from, ret);
}

// v12 0x0050C6A0  (feb 0x004F9C50)
// [DYN_PROP] placing on the ground (every frame, on water, by the origin); the first sequence at its start; a
// loaded-from-scene instance (0x40) gets priority 0x28 and is synchronised in MP; [DYNAMIC] isStartAnim (default:
// when there are sequences) / isCycleAnim (1). The server marks a single sequence longer than 10 s and asks a 5 s
// MP update period.
BOOL32 dynDYNAMIC::ProcessINIT(void *)
{
    int v;
    if (inst->flags & 0x40)
        inst->GetInt((char *)"DYN_PROP", (char *)"PLACE_GROUND", &v);
    if (inst->GetInt((char *)"DYN_PROP", (char *)"PLACE_GROUND_EVERY_FRAME", &v) && v)
        dynFlags |= DYNF_PLACE_EVERY_FRAME;
    if (inst->GetInt((char *)"DYN_PROP", (char *)"PLACE_GROUND_ON_WATER", &v) && v)
        dynFlags |= DYNF_PLACE_ON_WATER;
    if (inst->GetInt((char *)"DYN_PROP", (char *)"PLACE_GROUND_ORIGIN", &v) && v)
        dynFlags |= DYNF_PLACE_ORIGIN;
    animTPL *t = inst->tpl;
    if (t && t->nSeq > 0)
        inst->SetAnimSeq(0, 0.0f);
    if (inst->flags & 0x40)
        priority = 0x28;
    t = inst->tpl;
    int start = t && t->nSeq > 0 ? 1 : 0;
    inst->GetBool((char *)"DYNAMIC", (char *)"isStartAnim", &start);
    int cycle = 1;
    inst->GetBool((char *)"DYNAMIC", (char *)"isCycleAnim", &cycle);
    if (inst->flags & 0x40)
        gsSysServer->DeclSyncable(this);
    if (start)
        StartAnim(0, 0.0f, cycle);
    if (entFlags & 4) {
        t = inst->tpl;
        if (t && t->nSeq == 1 && t->seq[0].lengthMs * 0.001f > 10.0f)
            dynFlags |= DYNF_LONG_SINGLE_SEQ;
        SetMPUpdPeriod(5.0f);
    }
    return 1;
}

// v12 0x0050C880  (feb 0x004F9DD0)
// While playing: the animation advances by the frame (a client also by catchUpRate * frame of its time debt, at
// most the debt); a one-shot sequence at its end stops (events 0x20 | 8) and the server tells the clients.
void dynDYNAMIC::ProcessFRAME()
{
    if (!(dynFlags & DYNF_PLAYING))
        return;
    float extra;
    if (entFlags & 1) {
        float k = gsElapsedTime * catchUpRate;
        if (fabsf(k) > fabsf(timeDebt)) {
            extra = timeDebt;
            timeDebt = 0;
        } else {
            timeDebt = timeDebt - k;
            extra = k;
        }
    } else {
        extra = 0;
    }
    if (!inst->AdvanceTime(gsElapsedTime + extra, animMode, 0))
        return;
    animTPL *t = inst->tpl;
    if (!t || t->nSeq <= 0 || animMode != 1)
        return;
    pendingEvents |= 0x20;
    pendingEvents |= 8;
    dynFlags &= ~DYNF_PLAYING;
    if (!(entFlags & 4) || mpId == -1)
        return;
    dynMP_MSG m;
    m.tag0 = 0;
    m.tag1 = 0x40;
    m.size = 0x12;
    m.type = 1;
    m.dynFlags = dynFlags;
    m.seq = inst->seq;
    m.time = inst->seqTime;
    SendMP_Server_2_ClientAll((msgDATA *)&m, 0x20000);
    if (dynFlags & DYNF_LONG_SINGLE_SEQ)
        entFlags &= ~0x80u;
}

// v12 0x00403A20 (the shared empty function)
void dynDYNAMIC::ProcessTERM()
{
}

// v12 0x0050CE00  (feb 0x004FA100)
// Sequence seq from time, cycling or once; playing (event 4); the server tells the clients (and, for a cycling
// or long single sequence, sets entFlags 0x10).
void dynDYNAMIC::StartAnim(int seq, float time, int cycle)
{
    animMode = cycle ? 0 : 1;
    inst->SetAnimSeq(seq, time);
    dynFlags |= DYNF_PLAYING;
    pendingEvents |= 4;
    if (!(entFlags & 4) || mpId == -1)
        return;
    dynMP_MSG m;
    m.tag0 = 0;
    m.tag1 = 0x40;
    m.size = 0x16;
    m.type = 0;
    m.dynFlags = dynFlags;
    m.seq = seq;
    m.time = time;
    m.animMode = animMode;
    SendMP_Server_2_ClientAll((msgDATA *)&m, 0x20000);
    if ((dynFlags & DYNF_LONG_SINGLE_SEQ) || animMode == 0)
        entFlags |= 0x10;
}

// v12 0x0050CEE0  (feb 0x004FA1C0)
// Not playing (event 8); the server tells the clients where it stopped.
void dynDYNAMIC::StopAnim()
{
    pendingEvents |= 8;
    dynFlags &= ~DYNF_PLAYING;
    if (!(entFlags & 4) || mpId == -1)
        return;
    dynMP_MSG m;
    m.tag0 = 0;
    m.tag1 = 0x40;
    m.size = 0x12;
    m.type = 1;
    m.dynFlags = dynFlags;
    m.seq = inst->seq;
    m.time = inst->seqTime;
    SendMP_Server_2_ClientAll((msgDATA *)&m, 0x20000);
    if (dynFlags & DYNF_LONG_SINGLE_SEQ)
        entFlags &= ~0x80u;
}

// v12 0x0050CFA0  (feb 0x004FA260)
entENTITY *__fastcall dynDYNAMIC::Create(animINST *)
{
    void *mem = operator new(0x19B);
    if (!mem)
        return 0;
    dynDYNAMIC *d = new (mem) dynDYNAMIC;
    d->catchUpRate = 1.0f;
    d->timeDebt = 0;
    d->dynFlags = 0;
    d->unk177[0] = d->unk177[1] = d->unk177[2] = d->unk177[3] = 0;
    d->unk187 = m3dVZero;
    d->unk193 = 0;
    return d;
}
