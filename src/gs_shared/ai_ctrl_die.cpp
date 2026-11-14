// ai_ctrl_die.cpp -- gs_shared.lib NPC hits and death (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_ctrl_die.obj)

#include "../Sources/game.h"

// v12 0x0054BCD0  (feb 0x00530AB0)
// Is e an NPC ('CPN$') in its dying state (+0x46F bit 0x200000); *corpse = +0x473 bit 0x40000.
BOOL32 __fastcall aiIsNPCDead(entENTITY *e, int *corpse)
{
    if (!e || e->classId != 0x244E5043) {
        if (corpse)
            *corpse = 0;
        return 0;
    }
    if (corpse)
        *corpse = WR_FIELD(u32, e, 0x473) & 0x40000;
    return WR_FIELD(u32, e, 0x46F) & 0x200000;
}

// v12 0x0054BD10  (feb 0x00530AF0)
int aiNPC::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (!d || !(entFlags & 4))
        return 0;
    float k = d->type == 3 ? 0.3f : 1.0f;
    health -= k * d->amount;
    if (aiFlags & 0x200000)
        return 0;
    alertAngle = 180.0f;
    if (aiFlags & 0x10000) {
        char *c = inst->className;
        if (strstr(c ? c : (char *)"", "ai_mino"))
            nLocks = 0;
        Activate(1);
    }
    NotifyHealthChange(d);
    if (health <= 0.0f) {
        StartDeath(d);
        return 1;
    }
    Slot36();
    return 0;
}

// v12 0x0054BE00  (feb 0x00530BE0)
// 1.2: damage subtype 2 is an instant death (the corpse goes at once); the planner forgets it; the event is 0x4CA.
void aiNPC::StartDeath(dmgDAMAGE *d)
{
    health = 0;
    aiFlags |= 0x200000;
    if (d && d->subtype == 2)
        deadTimer = corpseTime - 0.1f;
    else
        deadTimer = 0;
    if (entFlags & 4) {
        if (aiAttackPlanner)
            aiAttackPlanner->UnRegisterNPC(this);
        msgSystem.SendAll(0x4CA, 0, this);
        if (!(aiFlags & 0x100000))
            pendingEvents |= 0x20;
        NotifyDeath(d);
        mpState.flags = 0x8B;
    }
}

// v12 0x0054BEB0  (feb 0x00530C50)
// The corpse fades over corpseFadeTime before corpseTime ends, then the entity goes.
void aiNPC::UpdateDead()
{
    inst->unk08 |= 4;
    if (objOBJ *o = inst->root) {
        float t0 = corpseFadeTime, t1 = corpseTime, v0 = 0.0f, v1 = 255.0f;
        if (t0 > t1) {                                  // (the two orders of the times)
            float x = t0;
            t0 = t1;
            t1 = x;
            v0 = 255.0f;
            v1 = 0.0f;
        }
        float t = deadTimer, a;
        if (t > t1)
            a = v1;
        else if (t < t0)
            a = v0;
        else
            a = (v1 - v0) * (t - t0) / (t1 - t0) + v0;
        u8 b = (u8)(int)a;
        o->SetConstColor((u32)b << 24, 1, 0);
        if (b < 0xF3)
            aiFlags &= ~0x1000000u;
    }
    deadTimer += gsElapsedTime;
    if ((entFlags & 4) && deadTimer > corpseTime)
        msgSystem.PostMsg(this, 2, this);
}

// v12 0x0054C040  (feb 0x00530DD0)
// An explosion among NPCs of one class goes to the others late, each with the same damage: a chain.
void aiDMG_DISPATCHER_TO_SIBLING::Dispatch(entENTITY *target, dmgDAMAGE *d, entENTITY *attacker)
{
    if (target->inst && attacker->inst) {
        char *a = target->inst->className;
        char *b = attacker->inst->className;
        if (!strcmp(a ? a : "", b ? b : "")) {
            if (amount > 1e-6f) {
                d->amount = amount;
                msgSystem.PostTimeMsgData(target, 0x4C6, (msgDATA *)d, attacker, m3dRandRange(0.2f, 1.0f));
            }
            return;
        }
    }
    dmgDISPATCHER::Dispatch(target, d, attacker);
}

// v12 0x0054C110  (feb 0x00530E90)
// A spawned NPC that sat unseen for 20 s (30 s with a boss around) dies, and its domain counts one more to spawn. 1.2:
// also those with aiFlags 0x8000000.
void aiNPC::DieOfInactivity()
{
    static dmgDAMAGE d(0);
    if (!(aiFlags & 0x8100000))
        return;
    if ((aiFlags & 0x80000) && (aiFlags & 0x10004))
        inactivityTimer += gsElapsedTime;
    if (aiFlags & 0x10000000)
        inactivityTimer = 0;
    float limit = aiAttackPlanner && *(int *)((u8 *)aiAttackPlanner + 0x28) > 0 ? 30.0f : 20.0f;
    if (inactivityTimer > limit) {
        d.amount = 5000.0f;
        d.subtype = 2;
        msgSystem.SendMsg(this, 0x4C6, &d, 0);
        if (spawnDomain && !(WR_FIELD(u32, spawnDomain, 0xD4) & 0x400)) {
            WR_FIELD(int, spawnDomain, 0x1C4)++;
            WR_FIELD(int, spawnDomain, 0x1D4)++;
        }
    }
}
