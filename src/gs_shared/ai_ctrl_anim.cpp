// ai_ctrl_anim.cpp -- an NPC's animation (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_ctrl_anim.obj)
//
// A sequence's flags (animSEQ +0x84) say how it moves the actor: 1 its "ROOT" object carries the instance, 2 turns about the
// pivot, 4 ...; the animation system (animSYSTEM_INST) plays what EvalAnimGoalSeq asks for.

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"

// v12 0x00572440: a float in lo..hi as a byte
u8 __fastcall m3dFloatToByte(float v, float lo, float hi)
{
    if (v < lo)
        v = lo;
    else if (v > hi)
        v = hi;
    return (u8)(int)((v - lo) * 255.0f / (hi - lo));
}

// v12 0x00572540
float __fastcall m3dByteToFloat(u8 b, float lo, float hi)
{
    return (float)b * (hi - lo) / 255.0f + lo;
}

static inline int seqFlagsOf(animINST *a)
{
    return (int)a->tpl->seq[a->seq].unk84;
}

// v12 0x0054F630  (feb 0x00535400)
void aiNPC::GetPosRooted(m3dV *out)
{
    if (rootObj && (seqFlagsOf(inst) & 1)) {
        objValidateLTM(rootObj);
        rootObj->GetOrigin(out);
        return;
    }
    inst->GetPos(out);
}

// v12 0x0054F6A0  (feb 0x00535470)
void aiNPC::SetPosRooted(m3dV *p)
{
    m3dV to = *p;
    if (rootObj) {
        m3dV r, o;
        GetPosRooted(&r);
        inst->GetPos(&o);
        to.x = o.x - r.x + p->x;
        to.y = o.y - r.y + p->y;
        to.z = o.z - r.z + p->z;
    }
    inst->Translate(&to, 0);
}

// v12 0x0054F770  (feb 0x00535540)
// A sequence that moved the actor by its root has wrapped (flags 0x8000 on the instance): the instance takes the root's
// matrix. Then the matrix to take next time. 1.2: a team game's client is told (entFlags 0x400 / 0x100).
void aiNPC::MoveRootedPivoted()
{
    int f = seqFlagsOf(inst);
    if ((seqFlags & 1) && (inst->flags & 0x8000)) {
        int rooted = (f & 1) && (f & 4) && (inst->flags & 0x4000);
        if (!rooted) {
            inst->Transform(&rootedMatr, 0);
            inst->GetPos((m3dV *)lastPos);
            entFlags |= 0x100;
        }
        if (gsSysMP->mode == 3) {
            entFlags &= ~0x400u;
            if (!rooted) {
                *(m3dV *)mpState.pos = *(m3dV *)lastPos; // +0x10D the snapshot's position, +0x131 its speed
                *(m3dV *)mpState.vel = m3dVZero;
            }
        }
    }
    if (f & 1) {
        if (rootObj && (seqFlagsOf(inst) & 1)) {
            objValidateLTM(rootObj);
            memcpy(&rootedMatr, rootObj->ltm, sizeof(float) * 16);
            rootedMatr.RemoveScale();
        } else {
            memcpy(&rootedMatr, inst->matr, sizeof(float) * 16);
        }
        if (gsSysMP->mode == 3)
            entFlags |= 0x400;
    }
}

// v12 0x0054F900  NEW in 1.2 (name ours)
// A client plays what the server said: once through for aiFlags 0x4000000 (aiFlags 0x2000000 when it gets there).
void aiNPC::AdvanceClientAnim(int seq, float dt)
{
    animSYSTEM_INST *s = (animSYSTEM_INST *)inst->link;
    if (aiFlags & 0x2000000) {
        s->AdvanceAnimation(inst, seq, dt, 0);
    } else {
        s->AdvanceAnimation(inst, seq, dt, (aiFlags & 0x4000000) ? 1 : 0);
        if ((inst->flags & 0x8000) && inst->seq == goalSeq2)
            aiFlags |= 0x2000000;
    }
    aiFlags &= ~0x4000000u;
}

// v12 0x0054F990  (feb 0x005355C0)
void aiNPC::Animate()
{
    seqFlags = seqFlagsOf(inst);
    int seq;
    if (entFlags & 4)
        seq = EvalAnimGoalSeq();
    else if (aiFlags & 0x2000000)
        seq = goalSeq2;
    else
        seq = goalSeq;
    if (flags2 & 0x10) {
        animRate2 = 1.0f;
    } else if (entFlags & 4) {
        float r = GetAnimPlayRate() * unk513;
        animRate2 = r;
        animRate2 = m3dByteToFloat(m3dFloatToByte(r, 0.0f, 5.0f), 0.0f, 5.0f);
    }
    float dt = gsElapsedTime * animRate2;
    int was = inst->seq;
    if (!(flags2 & 0x10)) {
        if (entFlags & 4)
            ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, seq, dt, 0);
        else
            AdvanceClientAnim(seq, dt);
    }
    if (inst->seq != was)
        entFlags |= 0x100;
    AnimateProcedural();
    if (inst->actionFrames)
        Slot79();
    MoveRootedPivoted();
    if ((entFlags & 4) && inst->actionFrames)
        ProcessActionFrame();
}
