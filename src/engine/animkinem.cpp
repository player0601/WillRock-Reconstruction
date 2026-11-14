// animkinem.cpp -- engine.lib kinematics of animated instances: the local matrix of every object of the tree at
// the current frame, from its animation tracks, blended between two sequences and with the animation channels
// (Will Rock 1.2, v12.exe; feb obj engine:animkinem.obj)
//
// v12 0x005010B0..0x00501AFF. 1.2 changes, from the disassembly:
//   - feb's GetObjTRSV is gone: _CalcObjModelMatrix reads each track through animOBJ_ANIM::GetPos / GetRot /
//     GetScale (new, names ours) and only for the parts the track has (animOBJ_ANIM::flags 2 / 4 / 8);
//     a track without rotation gets a pure scale matrix (m3dMATR::MakeScale) or the identity; flags & 0x10 skips
//     the object; the modifiers are animated before the transform;
//   - the visibility track sets the transparency through objOBJ::_SetTransparency (feb: SetConstColor);
//   - GetVisibility returns whether there is a visibility track; ApplyAnim no longer propagates state 4;
//   - CalcObjModelMatrix skips the children of a hidden / culled object (procState & 0x1C).
//   - _animCombineTRS takes the track flags first (2 / 4 / 8 as in _CalcObjModelMatrix; animCleanConstSpl passes -1).

#include "engine.h"

// v12 0x005010B0
void animOBJ_ANIM::GetPos(float f, int closest, m3dV *out)
{
    if (spl14) {
        if (closest)
            spl14->GetValueClosest(f, out, 0);
        else
            spl14->GetValue(f, out, 0);
        return;
    }
    out->x = pos[0];
    out->y = pos[1];
    out->z = pos[2];
}

// v12 0x00501100
void animOBJ_ANIM::GetRot(float f, int closest, m3dQUAT *out)
{
    if (spl28) {
        if (closest)
            spl28->GetValueClosest(f, out, 0);
        else
            spl28->GetValue(f, out, 0);
        out->Invert();
        return;
    }
    memcpy(out, rot, sizeof(rot));
    out->Invert();
}

// v12 0x00501160
void animOBJ_ANIM::GetScale(float f, int closest, m3dV *out)
{
    if (spl38) {
        if (closest)
            spl38->GetValueClosest(f, out, 0);
        else
            spl38->GetValue(f, out, 0);
        return;
    }
    out->x = scale[0];
    out->y = scale[1];
    out->z = scale[2];
}

// v12 0x005011B0  (feb 0x004EF050)
// The visibility track at frame f, in [0, 1]. Between two keys that switch it fully on or off it snaps to the
// later state. 0 (nothing written) without a visibility track. `closest` is not used.
BOOL32 animOBJ_ANIM::GetVisibility(float f, int closest, float *out)
{
    (void)closest;
    if (!spl3C)
        return 0;
    float v;
    spl3C->GetValue(f, &v, 0);
    if (v > 0.0033333334f && v < 0.997f) {
        float k = (float)(int)lrintf(f - 0.49999899f);    // floor through fistp (m3dIntFloor)
        float a, b;
        spl3C->GetValue(k, &a, 0);
        spl3C->GetValue(k + 1.0f, &b, 0);
        if (a < 0.005f && b > 0.995f) {
            *out = 1.0f;
            return 1;
        }
        if (a > 0.995f && b < 0.005f) {
            *out = 0.0f;
            return 1;
        }
    }
    if (v < 0.0f)
        *out = 0.0f;
    else if (v > 1.0f)
        *out = 1.0f;
    else
        *out = v;
    return 1;
}

// v12 0x00501320  (feb 0x004EF1B0)
void animINST::ApplyAnim()
{
    if (root) {
        StartApplyAnim();
        CalcObjModelMatrix(root);
    }
}

// v12 0x00501340  (feb 0x004EF1E0)
// The frames to pose: the two blended sequences' (flags 0x200), else the current one.
void animINST::StartApplyAnim()
{
    animSEQ *seqs = tpl->seq;
    if (flags & 0x200) {
        if (blendSeq0 < 0)
            blendFrame0 = -2.0f;
        else
            blendFrame0 = seqs[blendSeq0].TimeToFrame(blendTime0);
        blendFrame1 = seqs[blendSeq1].TimeToFrame(blendTime1);
        return;
    }
    frame = seqs[seq].TimeToFrame(seqTime);
}

static inline int akNChn(animINST *i)
{
    animINST::animLINK *l = i->link;
    return l ? *(int *)(*(char **)l + 0xC) : 0;
}

static inline animCHN *akChn(animINST *i, int k)
{
    return (animCHN *)(*(char **)((char *)i->link + 4) + k * 0x18);
}

// v12 0x005013C0  (feb 0x004EF260)
// Pose o and (unless it is hidden or culled) its subtree, inside the BegObjHier / EndObjHier of every channel.
void animINST::CalcObjModelMatrix(objOBJ *o)
{
    for (int k = 0; k < akNChn(this); k++)
        akChn(this, k)->BegObjHier(o);
    if (!(flags & 0x400000) || !(o->state & 0x800)) {
        _CalcObjModelMatrix(o);
        u32 st = o->procState;
        if (!(st & 0x1C) || (st & 0xF000)) {
            objOBJ *first = o->child;
            int more = 0;
            for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
                CalcObjModelMatrix(c);
        }
    }
    for (int k = 0; k < akNChn(this); k++)
        akChn(this, k)->EndObjHier(o);
}

// v12 0x00501480  (feb 0x004EF340, rewritten in 1.2)
// The local matrix (+0x7C) of one object at the current frame. Track flags: 2 position, 4 rotation, 8 scale,
// 0x10 not animated. Instance flags: 0x200 blend two sequences (unless 0x800000; objects with state 0x200 take
// only the second), 0x200000 read the tracks with GetValueClosest.
void animINST::_CalcObjModelMatrix(objOBJ *o)
{
    u32 st = o->procState;
    if (!(st & 0xF000)) {
        if (st & 8)
            return;
        if ((st & 1) && !o->child)
            return;
    }
    animOBJ_ANIM *oa = 0;
    if (o->unk20 >= 0) {
        oa = (animOBJ_ANIM *)((char *)tpl->objAnim + o->unk20 * 0x40);
        float vis;
        if (oa && oa->GetVisibility(frame, flags & 0x200000, &vis)) {
            int a = (int)(vis * 255.0f);
            if (a > 0 && a < 0xFE)
                o->_SetTransparency(0, (u8)a);
            if (a < 1) {
                if (!(o->procState & 4))
                    o->SetStateProcYes(4, 0);
            } else if (o->procState & 4) {
                o->SetStateProcNo(4, 0);
                o->_SetTransparency(0, (u8)a);
            }
        }
    }
    st = o->procState;
    if ((st & 0x1C) && !(st & 0xF000))
        return;
    if ((!(st & 0x3F) || (st & 0xF000)) && o->geom && o->geom->nModifier) {
        float frames[5], weights[5];
        int n = 0;
        for (int k = 0; k < akNChn(this); k++) {
            animCHN *c = akChn(this, k);
            if (c->IsActive(o, 0x200)) {
                frames[n] = c->GetFrameCur();
                weights[n] = c->weight;
                n++;
            }
        }
        frames[n] = frame;
        weights[n] = 1.0f;
        o->_AnimateModifiers(n + 1, frames, weights);
    }
    if (!oa || (oa->flags & 0x10))
        return;

    int closest = flags & 0x200000;
    m3dV pos, scl, p0, p1, s0, s1;
    m3dQUAT rot, q0, q1;
    float t;
    if ((flags & 0x200) && !(flags & 0x800000)) {
        float f0, f1, w;
        GetBlendParams(&f0, &f1, &w);
        if (!(o->state & 0x200)) {
            if (oa->flags & 2)
                oa->GetPos(f0, closest, &p0);
            if (oa->flags & 4)
                oa->GetRot(f0, closest, &q0);
            if (oa->flags & 8)
                oa->GetScale(f0, closest, &s0);
            if (oa->flags & 2)
                oa->GetPos(f1, closest, &p1);
            if (oa->flags & 4)
                oa->GetRot(f1, closest, &q1);
            if (oa->flags & 8)
                oa->GetScale(f1, closest, &s1);
            u32 tf = oa->flags;
            if (tf & 2)
                m3dLerpVector_L(&p0, &p1, 0.0f, 1.0f, w, &pos);
            if (tf & 4)
                m3dSlerpQuat(&q0, &q1, w, &rot);
            if (tf & 8)
                m3dLerpVector_L(&s0, &s1, 0.0f, 1.0f, w, &scl);
            goto channels;
        }
        t = f1;
        if (oa->flags & 2)
            oa->GetPos(t, closest, &pos);
    } else {
        t = frame;
        if (oa->flags & 2)
            oa->GetPos(t, closest, &pos);
    }
    if (oa->flags & 4)
        oa->GetRot(t, closest, &rot);
    if (oa->flags & 8)
        oa->GetScale(t, closest, &scl);

channels:
    for (int k = 0; k < akNChn(this); k++) {
        animCHN *c = akChn(this, k);
        if (!c->IsActive(o, 4))
            continue;
        float ft = c->GetFrameCur();
        if (oa->flags & 2)
            oa->GetPos(ft, flags & 0x200000, &p1);
        if (oa->flags & 4)
            oa->GetRot(ft, flags & 0x200000, &q1);
        if (oa->flags & 8)
            oa->GetScale(ft, flags & 0x200000, &s1);
        p0 = pos;
        q0 = rot;
        s0 = scl;
        float w = c->weight;
        u32 tf = oa->flags;
        if (tf & 2)
            m3dLerpVector_L(&p0, &p1, 0.0f, 1.0f, w, &pos);
        if (tf & 4)
            m3dSlerpQuat(&q0, &q1, w, &rot);
        if (tf & 8)
            m3dLerpVector_L(&s0, &s1, 0.0f, 1.0f, w, &scl);
    }

    u32 tf = oa->flags;
    m3dMATR *m = (m3dMATR *)o->local;
    if (tf & 4) {
        rot.GetMatr(m);
        if (tf & 8)
            m->Scale(scl.x, scl.y, scl.z, 1);
    } else if (tf & 8)
        m->MakeScale(scl.x, scl.y, scl.z);
    else
        m->Identity();
    if (tf & 2)
        m->Translate(&pos, 2);
}

// v12 0x00501AF0  (feb 0x004EF940; 1.2 adds the flags)
// A matrix from position, rotation and scale, each used only when its flag is set.
void __fastcall _animCombineTRS(u32 tf, m3dV *t, m3dQUAT *r, m3dV *s, m3dMATR *out)
{
    if (tf & 4) {
        r->GetMatr(out);
        if (tf & 8)
            out->Scale(s->x, s->y, s->z, 1);
    } else if (tf & 8)
        out->MakeScale(s->x, s->y, s->z);
    else
        out->Identity();
    if (tf & 2)
        out->Translate(t, 2);
}
