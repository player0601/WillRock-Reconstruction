// anim_ctrl.cpp -- engine.lib animation control: the transition tables between an entity's sequences (animSYSTEM,
// animSYSTEM_GROUP) and the per-instance state machine that plays them (animSYSTEM_INST)
// (Will Rock 1.2, v12.exe; feb obj engine:anim_ctrl.obj)
//
// v12 0x004C9D90..0x004CA2CF (animSYSTEM) and 0x004CA2D0..0x004CAA8F (animSYSTEM_GROUP): every function an exact twin
// of the feb code. A transition table holds n x n entries, entry n * to + from: the kind (1 go, 3 wait for the end),
// the sequence to play on the way (0xFFFF none) and whether to blend; each sequence has a parameter block. A group
// system adds sub-sequence groups (a weapon's sequences in up to n sub-variants, e.g. the player's body per weapon):
// per sequence the number of sub-sequences that follow it, and a name suffix per group.
// Then animSYSTEM_INST (v12 0x004CAA90..0x004CB3BF: AdvanceAnimation changed in 1.2 -- a 'force' argument, the end
// flag, the carried-over time kept inside the new sequence, the blend weight clamped, the action frames of a
// switch tested only when the sequence moved), animINST::CreateAnimSys and IsActionFrame.

#include "engine.h"

// ---------------------------------------------------------------------------------------------- animSYSTEM

// v12 0x004C9D90  (feb 0x004B9EE0)
// n sequences: every transition "wait for the end" (3) with no sequence between, blended; to itself or to another
// sequence: "go" (1) to that sequence, blended when it is another one. nParam parameter blocks (-1, 0, 0.1 s, 0).
animSYSTEM::animSYSTEM(int nSeq, int nPar)
{
    n = nSeq;
    int nn = nSeq * nSeq;
    animTRAN *t = (animTRAN *)operator new(nn * sizeof(animTRAN));
    if (t)
        for (int i = 0; i < nn; i++) {
            t[i].type = 3;
            t[i].next = 0xFFFF;
            t[i].blend = 1;
        }
    tran = t;
    for (int from = 0; from < n; from++)
        for (int to = 0; to < n; to++) {
            animTRAN *e = &tran[n * to + from];
            e->type = 1;
            e->next = (u16)to;
            e->blend = from == to ? 0 : 1;
        }
    nParam = nPar;
    animSEQ_PARAM *p = (animSEQ_PARAM *)operator new(nPar * sizeof(animSEQ_PARAM));
    if (p)
        for (int i = 0; i < nPar; i++) {
            p[i].seq = -1;
            p[i].flags = 0;
            p[i].blendTime = 0.1f;
            p[i].unk10 = 0;
        }
    param = p;
}

// v12 0x004C9E80  (feb 0x004B9FD0)
void *animSYSTEM::__vecDelDtor(unsigned f)
{
    operator delete(param);
    operator delete(tran);
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004C9EC0  (feb 0x004BA010)
void animSYSTEM::SetTranParam(int from, int to, u8 type, u16 next, u8 blend)
{
    animTRAN *e = &tran[n * to + from];
    e->type = type;
    e->next = next;
    e->blend = blend;
}

// v12 0x004C9F00  (feb 0x004BA050)
// Both ways between a and b: type, straight to the other, blended.
void animSYSTEM::SetTranParamSymm(int a, int b, u8 type)
{
    animTRAN *e = &tran[n * b + a];
    e->type = type;
    e->next = (u16)b;
    e->blend = 1;
    e = &tran[n * a + b];
    e->type = type;
    e->next = (u16)a;
    e->blend = 1;
}

// v12 0x004C9F60  (feb 0x004BA0B0)
// From seq to every other: the kind, the blend and (next >= 0) the sequence on the way.
void animSYSTEM::SetTranTypeToAny(int from, int type, u8 blend, int next)
{
    for (int to = 0; to < n; to++) {
        tran[n * to + from].type = (u8)type;
        tran[n * to + from].blend = blend;
        if (next >= 0)
            tran[n * to + from].next = (u16)next;
    }
}

// v12 0x004C9FC0  (feb 0x004BA110)
// From every sequence to seq.
void animSYSTEM::SetTranParamFromAny(int to, int type, u8 blend, int next)
{
    for (int from = 0; from < n; from++) {
        tran[n * to + from].type = (u8)type;
        tran[n * to + from].blend = blend;
        tran[n * to + from].next = (u16)next;
    }
}

// v12 0x004CA030  (feb 0x004BA180)
// seq loops: to itself at once, not blended.
void animSYSTEM::SetTranCycleAnim(int seq)
{
    animTRAN *e = &tran[(n + 1) * seq];
    e->type = 1;
    e->next = (u16)seq;
    e->blend = 0;
}

// v12 0x004CA060  (feb 0x004BA1B0)
// a and b alternate: a goes on to b, b back to a, each waiting for its end (the old way of a two-part cycle).
void animSYSTEM::SetTranCycle2Anim(int a, int b)
{
    animTRAN *e = &tran[(n + 1) * a];
    e->type = 1;
    e->next = (u16)b;
    e->blend = 0;
    e = &tran[n * b + a];
    e->type = 3;
    e->next = 0xFFFF;
    e->blend = 0;
    e = &tran[n * a + b];
    e->type = 1;
    e->next = (u16)a;
    e->blend = 0;
    e = &tran[(n + 1) * b];
    e->type = 3;
    e->next = 0xFFFF;
    e->blend = 0;
}

// v12 0x004CA100  (feb 0x004BA250)
// Two cycles (a1, a2) and (b1, b2) merge: a1 -> b1 goes by b2, b2 -> a2 waits; and the other way.
void animSYSTEM::SetTranMergeCycles(int a1, int a2, int b1, int b2, u8 blend)
{
    animTRAN *e = &tran[n * b1 + a1];
    e->type = 1;
    e->next = (u16)b2;
    e->blend = blend;
    e = &tran[n * b2 + a2];
    e->type = 3;
    e->next = 0xFFFF;
    e->blend = 1;
    e = &tran[n * a1 + b1];
    e->type = 1;
    e->next = (u16)a2;
    e->blend = blend;
    e = &tran[n * a2 + b2];
    e->type = 3;
    e->next = 0xFFFF;
    e->blend = 1;
}

// v12 0x004CA1B0  (feb 0x004BA300)
// Sequence to behaves as from: its row and column copied (the moves into from now go to to), from -> to at once.
void animSYSTEM::DuplicateTranParam(int from, int to)
{
    for (int i = 0; i < n; i++)
        tran[n * i + to] = tran[n * i + from];
    for (int i = 0; i < n; i++) {
        animTRAN *e = &tran[n * to + i];
        *e = tran[n * from + i];
        if (e->next == (u32)from)
            e->next = (u16)to;
    }
    animTRAN *e = &tran[n * to + from];
    e->type = 1;
    e->next = (u16)to;
    e->blend = 1;
}

// v12 0x004CA240  (feb 0x004BA390)
int animSYSTEM::GetTranType(int from, int to, int sub)
{
    if (from == -1)
        return 0;
    return tran[n * to + from].type;
}

// v12 0x004CA2A0  (feb 0x004BA3F0)
int animSYSTEM::GetNextAnimSeq(int from, int to, int sub)
{
    if (from == -1)
        return to;
    return tran[n * to + from].next;
}

// v12 0x004CA270  (feb 0x004BA3C0)
int animSYSTEM::GetTranTypeBlend(int from, int to, int sub)
{
    if (from == -1)
        return 0;
    return tran[n * to + from].blend;
}

// ---------------------------------------------------------------------------------------------- animSYSTEM_GROUP

// v12 0x004CA2D0  (feb 0x004BA420)
// n sequences in groups of nGroup sub-variants; nParam parameter blocks.
animSYSTEM_GROUP::animSYSTEM_GROUP(int nSeq, int nGrp, int nPar) : animSYSTEM(nSeq, nPar)
{
    nGroup = nGrp;
    nSub = (u8 *)apCalloc(nSeq, 1);
    char *s = (char *)operator new(nGroup * 0x40);
    if (s)
        for (int i = 0; i < nGroup; i++)
            s[i * 0x40] = 0;
    suffix = s;
}

// v12 0x004CA340  (feb 0x004BA490)
void *animSYSTEM_GROUP::__vecDelDtor(unsigned f)
{
    apFree(nSub);
    operator delete(suffix);
    operator delete(param);
    operator delete(tran);
    if (f & 1)
        operator delete(this);
    return this;
}

// feb 0x004BA4E0 (inline in 1.2)
void animSYSTEM_GROUP::DeclareGroupSeq(int seq, int nSubSeq)
{
    nSub[seq] = (u8)nSubSeq;
}

// feb 0x004BA500 (inline in 1.2)
void animSYSTEM_GROUP::SetGroupSuffix(int g, char *s)
{
    ((apNAME *)(suffix + g * 0x40))->SetName(s);
}

// v12 0x004CA3D0  (feb 0x004BA520)
char *animSYSTEM_GROUP::GetGroupSuffix(int g)
{
    if (!suffix)
        return (char *)"";
    return suffix + g * 0x40;
}

// v12 0x004CA3F0  (feb 0x004BA540)
// Between grouped sequences: every sub-variant of from to the matching sub-variant of to, by way of the matching
// sub-variant of next.
void animSYSTEM_GROUP::SetTranParam(int from, int to, u8 type, u16 next, u8 blend)
{
    int g = nGroup;
    int nFrom = nSub[from] ? nSub[from] * g : g;
    int baseFrom = nSub[from] != 0;
    int nTo = nSub[to] * g;
    int nNext = nSub[next] ? nSub[next] * g : g;
    int baseNext = nSub[next] != 0;
    for (int i = 0; i < nFrom; i++)
        for (int j = 0; j < nTo; j++) {
            animTRAN *e = &tran[(j % nTo + 1 + to) * n + i % nFrom + baseFrom + from];
            e->type = type;
            e->next = (u16)(j % nNext + baseNext + next);
            e->blend = blend;
        }
}

// v12 0x004CA510  (feb 0x004BA660)
void animSYSTEM_GROUP::SetTranParamSymm(int a, int b, u8 type)
{
    int cnt = nGroup * nSub[a];
    for (int k = 0; k < cnt; k++) {
        int x = a + 1 + k, y = b + 1 + k;
        animTRAN *e = &tran[n * y + x];
        e->type = type;
        e->next = (u16)y;
        e->blend = 1;
        e = &tran[n * x + y];
        e->type = type;
        e->next = (u16)x;
        e->blend = 1;
    }
}

// v12 0x004CA5A0  (feb 0x004BA6F0)
// From seq (an ungrouped one: as the base, the blend taken from next; a grouped one: each sub-variant, next > 0 the
// matching sub-variant of next) to every sequence.
void animSYSTEM_GROUP::SetTranTypeToAny(int from, int type, u8 blend, int next)
{
    u8 ns = nSub[from];
    if (!ns) {
        for (int to = 0; to < n; to++) {
            tran[n * to + from].type = (u8)type;
            tran[n * to + from].blend = (u8)next;
        }
        return;
    }
    int x = from;
    for (int k = 0; k < (int)(nGroup * ns); k++) {
        x++;
        if (next < 1) {
            for (int to = 0; to < n; to++) {
                tran[n * to + x].type = (u8)type;
                tran[n * to + x].blend = blend;
            }
        } else {
            int nx = next + 1 + k;
            for (int to = 0; to < n; to++) {
                tran[n * to + x].type = (u8)type;
                tran[n * to + x].blend = blend;
                if (nx >= 0)
                    tran[n * to + x].next = (u16)nx;
            }
        }
    }
}

// v12 0x004CA6D0 (a shared empty function): grouped systems take no "from any"
void animSYSTEM_GROUP::SetTranParamFromAny(int to, int type, u8 blend, int next)
{
}

// v12 0x004CA6E0  (feb 0x004BA820)
// Every sub-variant of seq loops.
void animSYSTEM_GROUP::SetTranCycleAnim(int seq)
{
    for (int k = 0; k < nGroup; k++, seq++) {
        animTRAN *e = &tran[(n + 1) * seq];
        e->type = 1;
        e->next = (u16)seq;
        e->blend = 0;
    }
}

// v12 0x004CA720  (feb 0x004BA860)
void animSYSTEM_GROUP::SetTranCycle2Anim(int a, int b)
{
    for (int i = 0; i < nGroup; i++)
        for (int j = 0; j < nGroup; j++) {
            int y = b + j;
            animTRAN *e = &tran[n * (a - b + y) + i + a];
            e->type = 1;
            e->next = (u16)(b + j);
            e->blend = 0;
            e = &tran[n * y + i + a];
            e->type = 3;
            e->next = 0xFFFF;
            e->blend = 1;
            e = &tran[n * (a - b + y) + i + b];
            e->type = 1;
            e->next = (u16)(j + a);
            e->blend = 0;
            e = &tran[n * y + i + b];
            e->type = 3;
            e->next = 0xFFFF;
            e->blend = 1;
        }
}

// v12 0x004CA820  (feb 0x004BA960)
void animSYSTEM_GROUP::SetTranMergeCycles(int a1, int a2, int b1, int b2, u8 blend)
{
    for (int i = 0; i < nGroup; i++, a1++) {
        int x2 = a2 - (a1 - i) + a1;                    // a2 + i
        for (int j = 0; j < nGroup; j++) {
            int y2 = b2 + j, y1 = b1 - b2 + y2;         // b2 + j, b1 + j
            animTRAN *e = &tran[n * y1 + a1];
            e->type = 1;
            e->next = (u16)y2;
            e->blend = blend;
            e = &tran[n * y2 + x2];
            e->type = 3;
            e->next = 0xFFFF;
            e->blend = 1;
            e = &tran[n * a1 + y1];
            e->type = 1;
            e->next = (u16)x2;
            e->blend = blend;
            e = &tran[n * x2 + y2];
            e->type = 3;
            e->next = 0xFFFF;
            e->blend = 1;
        }
    }
}

// v12 0x004CA940  (feb 0x004BAA80)
void animSYSTEM_GROUP::DuplicateTranParam(int from, int to)
{
    int cnt = nGroup * nSub[from];
    for (int x = from + 1; x - 1 - from < cnt; x++)
        animSYSTEM::DuplicateTranParam(x, to - from + x);
}

// v12 0x004CA9A0  (feb 0x004BAAE0)
int animSYSTEM_GROUP::GetTranType(int from, int to, int sub)
{
    if (from == -1)
        return 0;
    return tran[(to + 1 + sub) * n + from].type;
}

// v12 0x004CAA60  (feb 0x004BABA0)
int animSYSTEM_GROUP::GetNextAnimSeq(int from, int to, int sub)
{
    int s = to + 1 + sub;
    if (from != -1)
        s = tran[n * s + from].next;
    return s;
}

// v12 0x004CA9D0  (feb 0x004BAB10)
int animSYSTEM_GROUP::GetTranTypeBlend(int from, int to, int sub)
{
    if (from == -1)
        return 0;
    return tran[(to + 1 + sub) * n + from].blend;
}

// v12 0x004CAA00  (feb 0x004BAB40)
// The first sequence of seq's group row: seq + 1 for a group head, else from the head before it.
int animSYSTEM_GROUP::GetBaseSeqNmb(int seq)
{
    if (nSub[seq])
        return seq + 1;
    for (int i = seq; i >= 0; i--)
        if (nSub[i])
            return ((seq - i - 1) / nGroup) * nGroup + 1 + i;
    return -1;
}

// v12 0x004CAA40  (feb 0x004BAB80)
int animSYSTEM_GROUP::GetNSubSeq(int seq)
{
    return nSub[seq];
}

// the slots after animSYSTEM's vtable (see anim.h)
int animSYSTEM::GetBaseSeqNmb(int seq)
{
    return seq;
}

int animSYSTEM::GetNSubSeq(int seq)
{
    return 0;
}

// ---------------------------------------------------------------------------------------------- animSYSTEM_INST

static float animEps = 1e-6f;               // v12 0x00606CE0 (feb 0x005DB840)

// v12 0x004CAA90  (feb 0x004BABD0)
// The template's system, no sequence yet, per sequence a word of action-frame bits on the instance, a channel per
// parameter block.
animSYSTEM_INST::animSYSTEM_INST(animINST *a)
{
    sys = a->tpl->aniSystem;
    cur = -1;
    prev = -1;
    blendT = 0;
    blendLen = 0;
    groupBase = -1;
    group = -1;
    groupNext = -1;
    groupT = 0;
    groupLen = 0;
    blendKind = 0;
    a->seqActionFrames = (u32 *)apCalloc(4, a->tpl ? a->tpl->nSeq : 0);
    int n = sys->nParam;
    animCHN *c = (animCHN *)operator new(n * sizeof(animCHN));
    if (c)
        for (int i = 0; i < n; i++) {
            *(int *)((char *)&c[i] + 0) = 0;
            *(int *)((char *)&c[i] + 8) = 0;
            c[i].actionFrames = 0;
        }
    chn = c;
    for (int i = 0; i < sys->nParam; i++)
        chn[i].param = &sys->param[i];
}

// v12 0x004CAB50  (feb 0x004BAC90)
animSYSTEM_INST::~animSYSTEM_INST()
{
    operator delete(chn);
}

// v12 0x004EADC0  (name ours)
// A channel not running (state 0) starts again from time 0.
void animCHN::Start()
{
    if (*(int *)_00 == 0) {
        *(float *)(_00 + 8) = 0;
        *(int *)_00 = 1;
    }
}

// v12 0x004CAB60  (feb 0x004BACA0)
// One step of dt towards sequence seq (out of range: 0; -1: none) through the transition table: at once ("go", or
// force), or when the playing one ends ("wait": a non-looping one stops at its end and the instance gets flag
// 0x100 -- 1.2). On a change: the cross-fade or same-phase blend, the new sequence at the carried-over time (1.2:
// kept just inside its length). Then the group blend (SetGroupCur), the sequence blend, the channels, and the action
// frames crossed this step (instance +0x28; per sequence +0x2C). 1 when the sequence changed or ended.
int animSYSTEM_INST::AdvanceAnimation(animINST *a, int seq, float dt, int force)
{
    int ended = 0;
    animSEQ *sq = a->tpl->seq;
    if (!sys)
        return 0;
    int oldSeq = a->seq;
    float oldFrame = oldSeq == -1 ? 0.0f : sq[oldSeq].TimeToFrame(a->seqTime);
    int next, type;
    if (seq < -1 || seq >= sys->n)
        seq = 0;
    if (seq == -1) {
        next = -1;
        type = 1;
    } else {
        next = sys->GetNextAnimSeq(cur, seq, group);
        type = sys->GetTranType(cur, seq, group);
        if (next == -1)
            next = 0;
        if (type == 3)
            type = 1;
    }
    if (force) {
        next = seq;
        type = 0;
    }
    int still = cur == -1 || sq[cur].dir == 2;
    float newTime = 0;
    int switched = 0;
    if (cur != next && type == 0) {
        switched = 1;
        prevTime = a->seqTime;
    } else {
        if (still)
            goto playing;
        newTime = dt + a->seqTime;
        float len = sq[cur].lengthMs * 0.001f;
        if (newTime <= len) {
            a->seqTime = newTime;
            goto playing;
        }
        if (type != 1) {
            a->seqTime = len;
            ended = 1;
            a->unk08 |= 0x100;
            goto playing;
        }
        newTime -= len;
        switched = 1;
        prevTime = len;
    }
    {
        animSEQ *ns = &sq[next];
        if (ns->dir == 2) {
            if (next != -1)
                goto playing;
            blendKind = 0;
        } else if (next == -1) {
            blendKind = 0;
        } else {
            blendKind = sys->GetTranTypeBlend(cur, seq, group);
        }
        switch (blendKind) {
        case 2:
            newTime = prevTime / sq[cur].lengthMs * ns->lengthMs;
            // fall through
        case 0:
            a->StopSeqBlend();
            break;
        case 1:
            blendLen = ns->lengthMs * 0.001f * 0.142857f;
            if (blendLen > 0.3f)
                blendLen = 0.3f;
            break;
        }
        blendT = 0;
        if (next != -1) {
            float len = ns->lengthMs * 0.001f;
            if (!(newTime < len))
                newTime = len - 0.01f;
        }
        a->SetAnimSeq(next, newTime);
        groupBase = sys->GetBaseSeqNmb(next);
        prev = cur;
        cur = next;
    }
playing:
    if (cur != -1 && sq[cur].dir != 2) {
        if (switched) {
            a->flags |= 0xA000;
            if (oldSeq != a->seq)
                a->flags |= 0x4000;
        }
        if (animEps * 10.0f < dt && !ended) {
            a->Invalidate(8);
            a->flags |= 0x2000;
        }
    }
    if (groupLen > 0.1f) {                              // the group blend (SetGroupCur)
        if (groupT <= groupLen) {
            a->SetSeqBlendParam(groupNext + groupBase, a->seqTime, cur, a->seqTime, 1.0f - groupT / groupLen);
            groupT += dt;
        } else {
            groupLen = 0;
            a->StopSeqBlend();
            cur = groupBase + groupNext;
            a->SetAnimSeq(cur, a->seqTime);
            group = groupNext;
        }
        blendKind = 0;
    }
    if (blendKind) {                                    // the sequence blend
        if (blendT > blendLen) {
            blendLen = 0;
            a->StopSeqBlend();
            blendKind = 0;
        } else {
            float t = dt + blendT;
            blendT = t;
            float w = t >= blendLen ? 1.0f : t / blendLen;
            if (blendKind == 1)
                a->SetSeqBlendParam(prev, prevTime, cur, a->seqTime, w);
        }
    }
    if (a->seq != -1)
        sq[a->seq].TimeToFrame(a->seqTime);             // (result unused)
    for (int i = 0; i < sys->nParam; i++)
        chn[i].AdvanceAnimation(dt);
    a->actionFrames = 0;
    for (int i = 0; i < 10; i++) {                      // the action frames crossed this step
        int hit = 0;
        int s = a->seq;
        if (switched) {
            if (s != -1) {
                float f0 = sq[s].TimeToFrame(0), f1 = sq[s].TimeToFrame(a->seqTime);
                if (!(animEps * 10.0f > fabsf(f0 - f1)))
                    hit = sq[s].IsActionFrame(i, f0, f1);
            }
            if (oldSeq != -1) {
                float f1 = type == 1 ? sq[oldSeq].TimeToFrame(sq[oldSeq].lengthMs * 0.001f) : animEps + oldFrame;
                hit |= sq[oldSeq].IsActionFrame(i, oldFrame, f1);
            }
        } else if (s != -1) {
            hit = sq[s].IsActionFrame(i, oldFrame, sq[s].TimeToFrame(a->seqTime));
        }
        if (hit)
            a->actionFrames |= 1u << i;
    }
    if (u32 *bits = a->seqActionFrames) {               // per sequence
        memset(bits, 0, (a->tpl ? a->tpl->nSeq : 0) * 4);
        for (int i = 0; i < 10; i++) {
            int s = a->seq;
            if (s == -1)
                continue;
            float lo, f1;
            if (switched) {
                lo = sq[s].TimeToFrame(0);
                f1 = sq[s].TimeToFrame(a->seqTime);
                if (animEps * 10.0f > fabsf(lo - f1))
                    continue;
            } else {
                f1 = sq[s].TimeToFrame(a->seqTime);
                lo = oldFrame;
            }
            if (sq[s].IsActionFrame(i, lo, f1))
                bits[a->seq] |= 1u << i;
        }
        for (int i = 0; i < 10; i++)
            if (switched && oldSeq != -1) {
                float f1 = type == 1 ? sq[oldSeq].TimeToFrame(sq[oldSeq].lengthMs * 0.001f) : animEps + oldFrame;
                if (sq[oldSeq].IsActionFrame(i, oldFrame, f1))
                    bits[oldSeq] |= 1u << i;
            }
        for (int k = 10; k; k--)                        // (ten times in the original)
            for (int i = 0; i < sys->nParam; i++)
                bits[chn[i].param->seq] = chn[i].actionFrames;
    }
    return switched | ended;
}

// v12 0x004CB390  (feb 0x004BB3E0)
// The sub-variant group: the first one at once, a change blended over 'blend' seconds.
void animSYSTEM_INST::SetGroupCur(int g, float blend)
{
    if (groupNext == -1) {
        group = g;
        groupNext = g;
        return;
    }
    if (g != groupNext) {
        groupT = 0;
        groupLen = blend;
        groupNext = g;
    }
}

// v12 0x004CB3D0  (feb 0x004BB420)
BOOL32 animINST::CreateAnimSys()
{
    if (!tpl->aniSystem)
        return 0;
    animSYSTEM_INST *s = (animSYSTEM_INST *)operator new(sizeof(animSYSTEM_INST));
    if (!s)
        return 0;
    link = (animLINK *)new (s) animSYSTEM_INST(this);
    return 1;
}

// v12 0x004CB410  (feb 0x004BB460)
BOOL32 animINST::IsActionFrame(int s, int bit)
{
    if (!seqActionFrames || bit < 0 || s < 0)
        return 0;
    if (s >= (tpl ? tpl->nSeq : 0))
        return 0;
    return seqActionFrames[s] & (1u << bit);
}

// ------------------------------------------------------------------------------------------- animCHN: the overlays
// A channel plays its parameter block's sequence (param->unk0C) over the objects below param->flags (an idHier),
// with the parts its mask (param->unk10) names: state +0 running, +4 inside that subtree while the tree is walked,
// +8 its time, weight, the action frames it crossed.

// v12 0x004EADE0  (feb 0x004DB100)
// Its time on by dt: done past the sequence's length; the weight rises over the blend time at the start and falls
// over it at the end; the action frames between the old and the new time (clamped to the length).
void animCHN::AdvanceAnimation(float dt)
{
    int    &state = *(int *)_00;
    float  &time = *(float *)(_00 + 8);
    animSEQ *sq = (animSEQ *)param->unk0C;
    float len = sq->lengthMs * 0.001f;
    actionFrames = 0;
    if (!state)
        return;
    float t = dt + time;
    time = t;
    if (t > len)
        state = 0;
    else if (t < param->blendTime)
        weight = param->blendTime > 0.0f ? t / param->blendTime : 1.0f;
    else if (t > len - param->blendTime)
        weight = len - (len - param->blendTime) > 0.0f ? (len - t) / (len - (len - param->blendTime)) : 0.0f;
    else
        weight = 1.0f;
    float f0 = sq->TimeToFrame(t - dt);
    float cur = time;
    if (cur < 0.0f)
        cur = 0.0f;
    if (cur > len)
        cur = len;
    float f1 = sq->TimeToFrame(cur);
    for (int i = 0; i < 10; i++)
        if (sq->IsActionFrame(i, f0, f1))
            actionFrames |= 1u << i;
}

// v12 0x004EB000  (feb 0x004DB320)
void animCHN::BegObjHier(objOBJ *o)
{
    if (param->flags == o->idHier)
        *(int *)(_00 + 4) = 1;
}

// v12 0x004EB020  (feb 0x004DB340)
void animCHN::EndObjHier(objOBJ *o)
{
    if (param->flags == o->idHier)
        *(int *)(_00 + 4) = 0;
}

// v12 0x004EB040  (feb 0x004DB360)
// Running, inside its subtree, and taking this part (4 transform, 0x200 modifiers).
BOOL32 animCHN::IsActive(objOBJ *, int what)
{
    if (!*(int *)_00 || !*(int *)(_00 + 4))
        return 0;
    return (param->unk10 & what) != 0;
}

// v12 0x004EB070  (feb 0x004DB390)
float animCHN::GetFrameCur()
{
    return ((animSEQ *)param->unk0C)->TimeToFrame(*(float *)(_00 + 8));
}
