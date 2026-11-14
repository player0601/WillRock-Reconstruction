// sob_katapult.cpp -- the catapult that throws the player (Will Rock 1.2, v12.exe; feb obj sob_katapult.obj)
//
// A "dynamic" whose sequence 1 (SHOOT) carries the throw on its action frames: bit 1 the player standing on the object
// "chasha" is thrown at the [KATAPULT] target, the arc [KATAPULT] height high; bit 2 the instance collides again. The
// level plays it with the IActions START "CHARGE" / "SHOOT". plrPLAYER::IsStayOn is more generous on this class (1.5).

#include "game.h"

// v12 0x00487D80  (feb 0x0047D800)
int __fastcall wrsobInitKatapult()
{
    return entRegisterClass((char *)"sob_katapult", 0x54524D50, 0x28, wrsobKATAPULT::Create, 0, 1) != 0;
}

// v12 0x00487DB0  (feb 0x0047D830)
// The dynamic's messages first. INIT: the height, the target's position, the cup. FRAME, playing the throw: its action
// frames (out of collision on the first, the player on the cup launched: event 0x20; back in on the second).
BOOL32 wrsobKATAPULT::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    dynDYNAMIC::ProcessMsg(id, data, from, ret);
    if (id == 1) {
        height = 20.0f;
        inst->GetFloat((char *)"KATAPULT", (char *)"height", &height);
        char name[0x40];
        if (inst->GetStr((char *)"KATAPULT", (char *)"target", name, 0x40)) {
            animINST *t = gsScenePtr->FindInstName(name);
            if (t)
                t->GetPos(&target);
        }
        cup = objFindNameN(inst->root, (char *)"chasha", 6);
    } else if (id == 1000 && cup && inst->seq == 1) {
        if (inst->actionFrames & 1) {
            inst->flags |= 0x10;
            if (!plgsPlayer || !wrEVCall<BOOL32>(plgsPlayer, 0xA0, inst, cup, 0))      // IsStayOn
                return 0;
            m3dV pos, vel;
            wrEVCall<void>(plgsPlayer, 0x64, &pos);                                     // GetPos
            cgmGetThrowSpeed_H(&pos, &target, height, -30.0f, &vel);
            wrEVCall<void>(plgsPlayer, 0xA4, &vel, 1);                                  // ApplySpeed
            pendingEvents |= 0x20;
        }
        if (inst->actionFrames & 2)
            inst->flags &= ~0x10u;
    }
    return 0;
}

// v12 0x00487F70  (feb 0x0047D9F0)
entENTITY *__fastcall wrsobKATAPULT::Create(animINST *i)
{
    void *mem = operator new(0x1AF);
    if (!mem)
        return 0;
    wrsobKATAPULT *k = new (mem) wrsobKATAPULT;
    k->catchUpRate = 1.0f;                              // (the dynamic's Create, inline)
    k->timeDebt = 0;
    k->dynFlags = 0;
    k->unk177[0] = k->unk177[1] = k->unk177[2] = k->unk177[3] = 0;
    k->unk187 = m3dVZero;
    k->unk193 = 0;
    k->cup = 0;
    k->height = 0;
    k->target = m3dVUndef;
    return k;
}
