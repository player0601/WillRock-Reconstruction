// ai_skel_sword.cpp -- the sword skeleton that charges (Will Rock 1.2, v12.exe; feb obj ai_skel_sword.obj)
//
// Registered as "ai_skel_kamik": runs at the enemy, at 20 m starts its attack and swings through it, hurting while the
// swing lasts (swordFlags 0x20). After a swing it goes on (0x10000), or 1 time in 5 it breaks; hitting a wall on the
// way breaks it too. No explosion: it falls apart like any other skeleton.

#include "game.h"

static_assert(sizeof(aiSKEL_SWORD) == 0x577, "aiSKEL_SWORD size");

// v12 0x0045BC90  (feb 0x00452F80)
entENTITY *__fastcall aiSKEL_SWORD::Create(animINST *i)
{
    aiSKEL_SWORD *s = new aiSKEL_SWORD;
    if (!s)
        return 0;
    s->wrFlags = 0;
    s->bloodObj = 0;
    s->unk98 = 5;
    return s;
}

// v12 0x0045BCE0  (feb 0x00452FD0)
int aiSKEL_SWORD::ProcessINIT(void *init)
{
    swordFlags = 0;
    aiSKEL_GENERIC::ProcessINIT(init);
    maxHealth = 2.0f;
    health = 2.0f;
    navArriveDist = 20.0f;
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    maxTurnStep = 20.0f;
    alertAngle = 180.0f;
    moveSpeed = 11.0f;
    turnSpeed = 180.0f;
    aiFlags2 |= 0x20002;
    swordObj = objFindName(inst->root, (char *)"SWORD_&lod");
    if (!swordObj)
        swordObj = objFindName(inst->root, (char *)"SWORD");
    if (!swordObj)
        return 0;
    GetBlueClothes();
    ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, EvalAnimGoalSeq(), 0.0f, 0);
    return 1;
}

// v12 0x0045BDE0  (feb 0x004530C0)
void aiSKEL_SWORD::ProcessINIT_LEVEL()
{
    GetBlueClothes();
    aiNPC_WR::ProcessINIT_LEVEL();
}

// The sword against the enemy: by the distance of the blade 15 (0.3 m) to 2 (2.5 m); a knock towards its facing.
// Returns 0 when the blade is too far. (Inline in both callers.)
int aiSKEL_SWORD::Strike()
{
    dmgGENERAL g;
    if (swordFlags & 0x20) {
        m3dV b, t;
        inst->Validate(4);
        swordObj->GetOrigin(&b);
        GetPlayerTargetPos(&t);
        float dx = t.x - b.x, dy = t.y - b.y, dz = t.z - b.z;
        float d = m3dSqrt(dx * dx + dy * dy + dz * dz);
        if (d > 2.5f)
            return 0;
        g.pos = b;
        if (d < 0.3f)
            g.amount = 15.0f;
        else
            g.amount = 15.0f - (d - 0.3f) * 5.909091f;
    } else {
        inst->GetPos(&g.pos);
        g.amount = 2.0f;
    }
    if (enemy.ent)
        msgSystem.SendMsg(enemy.ent, 0x4C6, &g, this);
    m3dV e, p, d;
    enemy.GetPos(&e);
    GetPosRooted(&p);
    inst->GetDir(&d);
    if (m3dIsBelongPointCone(&e, &p, &d, 1.5f, 0.2f, 60.0f)) {
        m3dV v = { e.x - p.x, 0, e.z - p.z };
        m3dNormalize(&v);
        v.y = 0.3f;
        m3dNormalize(&v);
        v.x *= 10.0f;
        v.y *= 10.0f;
        v.z *= 10.0f;
        enemy.ApplySpeed(&v);
    }
    return 1;
}

// v12 0x0045BE00  (feb 0x004530D0)
void aiSKEL_SWORD::ProcessFRAME()
{
    aiNPC_WR::ProcessFRAME();
    if (!(aiFlags & 0x200000) && (swordFlags & 0x20)) {
        if (Strike())
            StartDeath3(0, 0, 1);
    }
}

// v12 0x0045BFE0  (feb 0x004533F0)
void aiSKEL_SWORD::ProcessActionFrame()
{
    if (inst->IsActionFrame(0x10, 0))
        skelFlags &= ~4u;
    if (inst->IsActionFrame(0xE, 0))
        turnSpeed = 0;
    if (inst->IsActionFrame(0xE, 1))
        swordFlags |= 0x20;
    if (inst->IsActionFrame(0xE, 2))
        swordFlags &= ~0x20u;
    if (inst->IsActionFrame(0xE, 3)) {
        aiFlags &= ~0x1020u;
        if (m3dRandom() >= 0.2f) {
            if (m3dRandom() >= 0.1f) {
                turnSpeed = 180.0f;
                swordFlags |= 0x10000;
            } else {
                StartDeath3(0, 0, 0);
            }
        } else {
            swordFlags |= 0x400;
            if (!(aiFlags & 0x200000))
                StartDeath3(0, 0, 0);
        }
    }
    if (inst->IsActionFrame(0xC, 0))
        swordFlags &= ~0x10000u;
    if (inst->IsActionFrame(0x12, 0))
        skelFlags &= ~0x20u;
}

// v12 0x0045C0A0  (feb 0x00453560)
int aiSKEL_SWORD::EvalAnimGoalSeq()
{
    if (skelFlags & 4)
        return 0x10;
    if (enemy.IsDead())
        return 0xF;
    if (swordFlags & 0x10000)
        return 0xC;
    return ((aiFlags & 0x201000) != 0) + 0xD;
}

// v12 0x0045C0F0  (feb 0x004535C0)
void aiSKEL_SWORD::FillSoundFlags()
{
    if (aiFlags & 0x200000) {
        unk108 = 4;
        return;
    }
    if (!(inst->flags & 0x4000))
        return;
    switch (inst->seq) {
    case 0xC:
        unk108 = 0x1000;
        break;
    case 0xD:
        unk108 = 1;
        break;
    case 0xE:
        unk108 = 0x40;
        break;
    case 0xF:
        unk108 = 0;
        break;
    case 0x10:
        unk108 = 2;
        break;
    case 0x12:
        unk108 = 0x4000;
        break;
    }
}

// v12 0x0045C160 (+0x98 SelectGoal; Think at v12 0x0045C1?? -- feb 0x00453660)
// Running: the attack starts when the point to meet the enemy at is within 20 m.
void aiSKEL_SWORD::Think()
{
    aiNPC_WR::Think();
    if (inst->seq == 0xD && !(aiFlags & 0x1000))
        swordFlags |= 0x8000;
    if (!(skelFlags & 3) && (swordFlags & 0x8000)) {
        m3dV p, t;
        inst->GetPos(&p);
        if (aiAttackPlanner->GetEnemyMeetPoint(this, 11.0f, &t)) {
            float dx = p.x - t.x, dy = p.y - t.y, dz = p.z - t.z;
            if (dx * dx + dy * dy + dz * dz < 400.0f) {
                swordFlags &= ~0x8000u;
                aiFlags |= 0x1020;
                unk37F = moveSpeed;
            }
        }
    }
}

// v12 0x0045C160  (feb 0x00453740)
int aiSKEL_SWORD::SelectGoal()
{
    if (inst->seq != 0xD)
        aiFlags |= 0x200;
    return aiSKEL_GENERIC::SelectGoal();
}

// (feb 0x00453770)
void aiSKEL_SWORD::MoveToGoal()
{
    if (skelFlags & 1)
        return;
    navArriveDist = inst->seq != 0xE ? 6.0f : 22.0f;
    aiSKEL_GENERIC::MoveToGoal();
}

// v12 0x0045C1D0  (feb 0x004537B0)
// What it ran into: 0 the level or a fixed thing, 1 a player, 2 an NPC of kind 0, 3 another NPC, -1 the rest.
int aiSKEL_SWORD::ClassifyCollisionTarget(cdtINFO *ci)
{
    if (!ci->inst || (ci->inst->flags & 0x48))
        return 0;
    entENTITY *e = (entENTITY *)ci->inst->ent;
    if (e->classId == 0x504C5952)
        return 1;
    if (e->classId == 0x244E5043)
        return (e->unk98 != 0) + 2;
    return -1;
}

// v12 0x0045C5A0 is the slot's jump; (feb 0x00453810)
void aiSKEL_SWORD::ProcessCDT(cdtINFO *ci)
{
    int k = ClassifyCollisionTarget(ci);
    if (k == 0 || k == 3) {
        StartDeath3(0, ci, 1);
        return;
    }
    if (k != 1 || (aiFlags & 0x200000))
        return;
    Strike();
    StartDeath3(0, 0, 1);
}

// v12 0x0045C5D0  (feb 0x00453B40)
int aiSKEL_SWORD::IsDeadVoluntary()
{
    return swordFlags & 0x400;
}
