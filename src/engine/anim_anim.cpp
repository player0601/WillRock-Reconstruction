// anim_anim.cpp -- engine.lib animation sequences: time <-> frame of a sequence, its action frames, playing a
// sequence on an instance (Will Rock 1.2, v12.exe; feb obj engine:anim_anim.obj)
//
// v12 0x004D0BF0..0x004D16FF. All exact twins of the feb code except animSEQ::IsActionFrame, which 1.2 rewrote
// (one tolerance of 0.0005 on both ends of the crossed range, and a wrapped range is tested as two halves).
// Constants: eps 1e-6 (v12 0x00606EC0), 0.0005 (0x00606EC4).

#include "engine.h"

static const float aaEps = 1e-6f;
static const float aaActEps = 0.0005f;

// v12 0x004D0BF0  (feb 0x004C46C0)
// The frame shown t seconds into the sequence: t runs over [0, length] and the frame over the sequence's range,
// from frameStart (dir 0) or from frameEnd back (dir 1), plus the offset at +0x48; outside the range it wraps.
float animSEQ::TimeToFrame(float t)
{
    if (lengthMs < 0.001f)
        lengthMs = 0.5f;
    float out, x, period;
    if (dir == 0) {
        float len = lengthMs * 0.001f;
        float tLo = 0.0f, tHi = len, fLo = 0.0f, fHi = frameRange;
        if (len < 0.0f) {
            tHi = 0.0f;
            tLo = len;
            fLo = frameRange;
            fHi = 0.0f;
        }
        float f = fLo;
        if (tLo <= t) {
            f = fHi;
            if (t <= tHi) {
                float a0 = tLo, a1 = tHi, b0 = fLo, b1 = fHi;
                if (tHi < tLo) {
                    a0 = tHi;
                    a1 = tLo;
                    b0 = fHi;
                    b1 = fLo;
                }
                f = (t - a0) * (b1 - b0) / (a1 - a0) + b0;
            }
        }
        out = f + unk48 + frameStart;
        period = (aaEps * 100.0f + frameEnd) - frameStart;
        x = out - frameStart;
    } else {
        if (dir != 1)
            return 0.0f;
        float len = lengthMs * 0.001f;
        float tLo = 0.0f, tHi = len, fLo = 0.0f, fHi = frameRange;
        if (len < 0.0f) {
            tHi = 0.0f;
            tLo = len;
            fLo = frameRange;
            fHi = 0.0f;
        }
        float f = fHi;
        if (tLo <= t) {
            f = fLo;
            if (t <= tHi) {
                float a0 = tLo, a1 = tHi, b0 = fHi, b1 = fLo;
                if (tHi < tLo) {
                    a0 = tHi;
                    a1 = tLo;
                    b0 = fLo;
                    b1 = fHi;
                }
                f = (t - a0) * (b1 - b0) / (a1 - a0) + b0;
            }
        }
        out = f + unk48 + frameEnd;
        period = (aaEps * 100.0f + frameStart) - frameEnd;
        x = out - frameEnd;
    }
    if (!(fabsf(period) < aaEps))
        x = (float)floor((double)(x / period)) * period;
    return out - x;
}

// v12 0x004D0EF0  (feb 0x004C49C0)
// The time of a frame (the inverse of TimeToFrame, without the wrap). Kept as the game has it: for dir 1 the
// frame range is mapped end -> 0 and start -> length.
float animSEQ::FrameToTime(float f)
{
    if (dir == 2)
        return 0.0f;
    float a = frameStart, b = frameEnd;
    float tA = 0.0f, tB = lengthMs * 0.001f;
    if (dir == 0) {
        float lo = a, hi = b;                       // lo at time tA, hi at time tB
        if (b < a) {
            hi = a;
            lo = b;
            tA = tB;
            tB = 0.0f;
        }
        if (f < lo)
            return tA;
        if (f <= hi) {
            float a0 = lo, a1 = hi, b0 = tA, b1 = tB;
            if (hi < lo) {
                a0 = hi;
                a1 = lo;
                b0 = tB;
                b1 = tA;
            }
            return (f - a0) * (b1 - b0) / (a1 - a0) + b0;
        }
    } else {
        float hi = a, lo = b;
        if (a < b) {
            lo = a;
            hi = b;
            tA = tB;
            tB = 0.0f;
        }
        if (f < lo)
            return tA;
        if (f <= hi) {
            float a0 = lo, a1 = hi, b0 = tA, b1 = tB;
            if (hi < lo) {
                a0 = hi;
                a1 = lo;
                b0 = tB;
                b1 = tA;
            }
            tB = (f - a0) * (b1 - b0) / (a1 - a0) + b0;
        }
    }
    return tB;
}

// v12 0x004D10C0  (feb 0x004C4B90)
void animSEQ::PrepareDir()
{
    if (frameStart < frameEnd) {
        dir = 0;
        if (frameRange < 0.1f)
            frameRange = frameEnd - frameStart;
    } else if (frameStart > frameEnd) {
        dir = 1;
        if (frameRange < 0.1f)
            frameRange = frameStart - frameEnd;
    } else
        dir = 2;
}

// v12 0x004D1130  (feb 0x004C4C00, rewritten in 1.2)
// Did going from frame f0 to f1 pass action frame `bit`? (Action frames below 1 are unset.) A step shorter than
// 10 eps (dir 0) or 100 eps (otherwise) counts only when it ends on the frame.
BOOL32 animSEQ::IsActionFrame(int bit, float f0, float f1)
{
    float af = (float)actionFrame[bit];
    if (af < 1.0f)
        return 0;
    float d = f0 - f1;
    if (dir == 0) {
        float e = aaEps * 10.0f;
        if (fabsf(d) < e)
            return fabsf(f1 - af) < e;
        if (f0 < f1)
            return f0 < af + aaActEps && !(f1 + aaActEps < af);
        if (!(f1 + aaActEps < af))
            return 1;
        return !(f0 - aaActEps >= af);
    }
    float e = aaEps * 100.0f;
    if (fabsf(d) < e)
        return fabsf(f1 - af) < e;
    if (f0 > f1)                                        // backward: f0 down to f1 (v12 0x004D12EF / 0x004D1304)
        return !(af - aaActEps >= f0) && !(af + aaActEps < f1);
    if (!(f1 + aaActEps < af))
        return 1;
    return f0 - aaActEps < af;
}

// v12 0x004D13A0  (feb 0x004C4E50)
// Play sequence s (-1: none) from time t. Out-of-range indices become -1.
void animINST::SetAnimSeq(int s, float t)
{
    int n = tpl ? tpl->nSeq : 0;
    if (s < -1 || s >= n)
        s = -1;
    if (s != -1)
        Invalidate(0x18);
    else
        Validate(0xC);
    seq = s;
    if (s != -1)
        seqTime = t;
    if (bvSeq) {
        if (s == -1) {
            if (tpl && tpl->nSeq > 0)
                bvCur = bvSeq;
        } else
            bvCur = (abvBVOL *)((char *)bvSeq + s * 0x24);
        Invalidate(0x100);
    }
    flags |= ANIM_ST_SEQ_CHANGED;
}

// v12 0x004D1450  (feb 0x004C4F00)
// Move the current sequence on by dt seconds. mode 0 loops; mode 1 stops at the end (the time past the end in
// *overflow), and reports no action frames once it was already at the end. Returns 1 when the time passed the
// end (either mode), when there is no sequence or it has no length; 0 for frozen instances (flags & 3, unless
// 4) except the template "Actor_cap_1". actionFrames gets the action frames crossed.
BOOL32 animINST::AdvanceTime(float dt, int mode, float *overflow)
{
    BOOL32 ret = 0;
    int past = 0;
    animSEQ *seqs = tpl->seq;
    if (stricmp(tplName ? tplName : (char *)"", "Actor_cap_1") && (flags & 3) && !(flags & 4))
        return 0;
    if (seq == -1)
        return 1;
    if (fabsf(seqs[seq].lengthMs) < aaEps)
        return 1;
    float f0 = seqs[seq].TimeToFrame(seqTime);
    float t0 = seqTime;
    float t = dt + seqTime;
    animSEQ *sq = &seqs[seq];
    seqTime = t;
    float len = sq->lengthMs * 0.001f;
    if (t > len) {
        if (mode == 0)
            seqTime = t - len;
        else if (mode == 1) {
            if (overflow)
                *overflow = t - len;
            float l = seqs[seq].lengthMs * 0.001f;
            sq = &seqs[seq];
            if (l - aaEps < seqTime - dt)
                past = 1;
            seqTime = l;
        }
        ret = 1;
    }
    float f1 = sq->TimeToFrame(seqTime);
    frame = f1;
    actionFrames = 0;
    if (!past)
        for (int i = 0; i < 10; i++)
            if (seqs[seq].IsActionFrame(i, f0, f1))
                actionFrames |= 1u << i;
    if (!(fabsf(t0 - seqTime) < aaEps)) {
        flags |= 0x2000;
        Invalidate(8);
    }
    return ret;
}

// v12 0x004D1690  (feb 0x004C5140)
// Blend from sequence s0 at time t0 (s0 -1: the current one, t0 < 0: the current time) into s1 at t1, weight w.
void animINST::SetSeqBlendParam(int s0, float t0, int s1, float t1, float w)
{
    if (s0 == -1)
        s0 = seq;
    blendSeq0 = s0;
    blendSeq1 = s1;
    if (t0 < 0.0f)
        t0 = seqTime;
    blendTime0 = t0;
    blendWeight = w;
    blendTime1 = t1;
    flags |= 0x200;
}

// v12 0x004D1360  (feb 0x004C4E10)
// All 0x96 bytes zero, then scale 1, no sequence (-1), the identity start matrix.
animCREATE_DATA::animCREATE_DATA()
{
    memset(this, 0, 0x96);
    seqId = -1;
    unk0E = 0;
    scale.z = 1.0f;
    scale.y = 1.0f;
    scale.x = 1.0f;
    matr.Identity();
}

// v12 0x004D16E0
void animINST::StopSeqBlend()
{
    flags &= ~0x200u;
}

// v12 0x004D16F0  (feb 0x004C51A0)
void animINST::GetBlendParams(float *f0, float *f1, float *w)
{
    *f0 = blendFrame0;
    *f1 = blendFrame1;
    *w = blendWeight;
}
