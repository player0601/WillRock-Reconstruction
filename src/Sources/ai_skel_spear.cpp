// ai_skel_spear.cpp -- the skeleton warrior (Will Rock 1.2, v12.exe; feb obj ai_skel_spear.obj)
//
// Registered as "ai_skel": _sword (blue armour, sword), _spear, _flexible (either, by chance, and blue). It holds one
// weapon: the sword strikes and stabs close; the spear strikes close and is thrown from 15 to 90 m, then a new one
// appears in its hand. A sniper stays and throws. weaponFlags: 2 holds the spear, 4 the sword, 8 a weapon appears,
// 0x10 always the spear, 0x20 always the sword, 0x40 flexible.
// The thrown spear: pjlSPEAR, src/Sources/ai_skel_spear_pjl.cpp (with LaunchOneProjectile / AimSpearToPlayer).

#include "game.h"
#include "../../include/sfx_sob/sfx_api.h"

static_assert(sizeof(aiSKEL_SPEAR) == 0x59F, "aiSKEL_SPEAR size");

// v12 0x004595E0  (feb 0x00450BC0)
entENTITY *__fastcall aiSKEL_SPEAR::Create(animINST *i)
{
    aiSKEL_SPEAR *s = new aiSKEL_SPEAR;
    if (!s)
        return 0;
    s->wrFlags = 0;
    s->bloodObj = 0;
    s->unk98 = 5;
    return s;
}

// v12 0x00459630  (feb 0x00450C10)
int aiSKEL_SPEAR::ProcessINIT(void *init)
{
    float size = 0.07f;
    weaponFlags = 0;
    swordObj = objFindName(inst->root, (char *)"SWORD_&lod");
    spearObj = objFindName(inst->root, (char *)"SPEAR_&lod");
    if (!swordObj || !spearObj)
        return 0;
    if (!aiSKEL_GENERIC::ProcessINIT(init))
        return 0;
    meetDist = 15.0f;
    throwMin = 15.0f;
    turnSpeed = 90.0f;
    throwMax = 90.0f;
    throwAngle = 20.0f;
    maxTurnStep = 20.0f;
    maxHealth = 2.0f;
    navArriveDist = 1.6f;
    alertAngle = 180.0f;
    moveSpeed = 13.0f;
    health = 2.0f;
    timer577 = 0;
    weaponTimer = 0;
    throwTimer = 0;
    aiFlags2 |= 0x10002;
    GetFloat((char *)"ANIM_PROP", (char *)"scale", &size);
    appearSfx = (wrsfxSKELETON_SPEAR_APPEAR *)entCreate(gsScenePtr, (char *)"sfx_skel_spear_appear",
                                                        (char *)"sfx_skel_spear_appear", 0, &size, 0);
    if (!appearSfx)
        return 0;
    appearSfx->owner = this;
    rootObj->state |= 0x200;
    return 1;
}

// v12 0x00459790  (feb 0x00450D70)
void aiSKEL_SPEAR::ParseNameClass()
{
    const char *cls = inst->className ? inst->className : "";
    aiSKEL_GENERIC::ParseNameClass();
    if (strstr(cls, "_sword"))
        weaponFlags |= 0x20;
    if (strstr(cls, "_spear"))
        weaponFlags |= 0x10;
    if (strstr(cls, "_flexible"))
        weaponFlags |= 0x40;
    if (aiFlags & 0x40000)
        weaponFlags = (weaponFlags & ~0x60u) | 0x10;
    if (skelFlags & 1)
        weaponFlags = (weaponFlags & ~0x10u) | 0x20;
    if (aiFlags & 0x40000) {
        meetDist = 0.1f;
        throwMin = 0;
        weaponFlags |= 0x10;
        throwMax = 150.0f;
    }
    SetupWeaponOnINIT();
    ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, EvalAnimGoalSeq(), 0.0f, 0);
}

// v12 0x004598B0  (feb 0x00450E90)
void aiSKEL_SPEAR::ProcessINIT_LEVEL()
{
    if (weaponFlags & 0x60)
        GetBlueClothes();
    aiNPC_WR::ProcessINIT_LEVEL();
}

void aiSKEL_SPEAR::ShowWeapon()
{
    if (curWeapon == swordObj) {
        spearObj->SetStateProcYes(8, 0);
        swordObj->SetStateProcNo(8, 0);
    } else {
        swordObj->SetStateProcYes(8, 0);
        spearObj->SetStateProcNo(8, 0);
    }
}

// v12 0x004598D0  (feb 0x00450EB0)
void aiSKEL_SPEAR::SetupWeaponOnINIT()
{
    SetupWeaponByDistance();
    int spear;
    if (weaponFlags & 0x40) {
        weaponFlags &= ~0x40u;
        spear = m3dRandom() >= 0.5f;
        weaponFlags |= spear ? 0x10 : 0x20;
    } else {
        spear = weaponFlags & 2;
    }
    if (spear) {
        curWeapon = spearObj;
        weaponFlags = (weaponFlags & ~4u) | 2;
    } else {
        curWeapon = swordObj;
        weaponFlags = (weaponFlags & ~2u) | 4;
    }
    ShowWeapon();
}

// v12 0x004599B0  (feb 0x00450F90)
int aiSKEL_SPEAR::EvalSwordAnimGoalSeq()
{
    if (skelFlags & 0x70)
        return 0x11;
    if (skelFlags & 4)
        return 0xC;
    if (enemy.IsDead())
        return 9;
    if (!(weaponFlags & 6) || (aiFlags & 0x1000))
        return 8;
    if (!(aiFlags & 1))
        return (aiFlags & 4) ? 9 : 8;
    if (skelFlags & 8)
        return 9;
    if (lastStab == 0x14)
        return m3dRandSelect(4, 5);
    if (lastStab == 4)
        return m3dRandSelect(0x14, 5);
    return m3dRandSelect(0x14, 4);
}

// v12 0x00459A70  (feb 0x00451050)
int aiSKEL_SPEAR::EvalSpearAnimGoalSeq()
{
    if (skelFlags & 0x70)
        return 0x13;
    if (skelFlags & 4)
        return 0xB;
    if (enemy.IsDead())
        return 10;
    if (weaponFlags & 6) {
        if (aiFlags & 0x1000)
            return 1;
        if (aiFlags & 1)
            return 6;
        if (weaponTimer > 0.01f)
            return 10;
        return (aiFlags & 4) ? 10 : 0;
    }
    return 0;
}

// v12 0x00459B30  (feb 0x00451110)
int aiSKEL_SPEAR::EvalAnimGoalSeq()
{
    int s = inst->seq;
    if (s == 0x14 || s == 4 || s == 5)
        lastStab = s;
    if (!(aiFlags & 0x40000)) {
        if (weaponFlags & 4)
            return EvalSwordAnimGoalSeq();
        if (weaponFlags & 2)
            return EvalSpearAnimGoalSeq();
        if (skelFlags & 0x74)
            return 7;
        if (enemy.IsDead())
            return 3;
        if (!(weaponFlags & 6))
            return 7;
        if (aiFlags & 0x1000)
            return 7;
        return ((aiFlags & 1) << 2) | 3;
    }
    // a sniper
    if (skelFlags & 0x70)
        return 0x13;
    if (skelFlags & 4)
        return 0xB;
    if (enemy.IsDead())
        return 10;
    if (!(weaponFlags & 6))
        return 7;
    return (aiFlags & 0x1000) ? 2 : 10;
}

// v12 0x00459C40  (feb 0x00451220)
// Always one weapon by the flags; else by the distance: from 32 m the spear.
void aiSKEL_SPEAR::SetupWeaponByDistance()
{
    m3dV p, t;
    inst->GetPos(&p);
    GetPlayerTargetPos(&t);
    int spear;
    if (weaponFlags & 0x10)
        spear = 1;
    else if (weaponFlags & 0x20)
        spear = 0;
    else {
        float dx = t.x - p.x, dy = t.y - p.y, dz = t.z - p.z;
        spear = m3dSqrt(dx * dx + dy * dy + dz * dz) > 32.0f;
    }
    if (spear) {
        curWeapon = spearObj;
        weaponFlags = (weaponFlags & ~4u) | 2;
    } else {
        curWeapon = swordObj;
        weaponFlags = (weaponFlags & ~2u) | 4;
    }
    ShowWeapon();
}

// v12 0x00459DE0  (feb 0x004513D0)
void aiSKEL_SPEAR::ProcessActionFrame()
{
    dmgGENERAL g;
    if (inst->IsActionFrame(0xC, 0) || inst->IsActionFrame(0xB, 0)) {
        skelFlags &= ~4u;
        if (skelFlags & 0x80) {
            skelFlags &= ~0x80u;
            StartJump(0.5f, 1.5f);
        }
    }
    if (inst->IsActionFrame(1, 0) || inst->IsActionFrame(2, 0))
        LaunchOneProjectile();
    if (inst->IsActionFrame(1, 1) || inst->IsActionFrame(2, 1)) {
        weaponFlags &= ~2u;
        aiFlags &= ~0x1020u;
        spearObj->SetStateProcYes(8, 0);
        swordObj->SetStateProcYes(8, 0);
    }
    if (inst->IsActionFrame(4, 0) || inst->IsActionFrame(5, 0) || inst->IsActionFrame(6, 0) ||
        inst->IsActionFrame(6, 1) || inst->IsActionFrame(0x14, 0)) {
        m3dV p, d, hit, t;
        inst->GetPos(&p);
        inst->GetDir(&d);
        hit.x = d.x * 1.6f + p.x;
        hit.y = d.y * 1.6f + p.y;
        hit.z = d.z * 1.6f + p.z;
        GetPlayerTargetPos(&t);
        float dx = t.x - hit.x, dy = t.y - hit.y, dz = t.z - hit.z;
        float dist = m3dSqrt(dx * dx + dy * dy + dz * dz);
        g.pos = hit;
        float full = inst->IsActionFrame(0x14, 0) ? 6.0f : 3.0f;
        if (dist < 1.0f)
            g.amount = full;
        else if (dist > 1.5f)
            g.amount = 0;
        else
            g.amount = full - (dist - 1.0f) * full * 2.0f;
        if (g.amount > 0.1f && enemy.ent)
            msgSystem.SendMsg(enemy.ent, 0x4C6, &g, this);
    }
    if (inst->IsActionFrame(7, 0)) {
        SetupWeaponByDistance();
        weaponFlags |= 8;
    }
    if (inst->IsActionFrame(7, 1))
        ShowWeapon();
    if (inst->IsActionFrame(7, 2) && curWeapon != swordObj) {
        weaponTimer = 0;
        throwTimer = 0;
    }
    if (inst->IsActionFrame(0x13, 0) || inst->IsActionFrame(0x11, 0))
        skelFlags &= ~0x20u;
}

// v12 0x0045A2D0  (feb 0x004518C0)
void aiSKEL_SPEAR::FillSoundFlags()
{
    if (aiFlags & 0x200000) {
        unk108 |= 4;
        return;
    }
    if (!(inst->flags & 0x4000))
        return;
    switch (inst->seq) {
    case 0:
    case 8:
        unk108 |= 1;
        break;
    case 1:
    case 2:
        unk108 |= 0x10;
        break;
    case 4:
        unk108 |= 0x200;
        break;
    case 5:
        unk108 |= 0x400;
        break;
    case 6:
        unk108 |= 0x20;
        break;
    case 7:
        unk108 |= 8;
        break;
    case 0xB:
        unk108 |= (skelFlags & 0xC0) ? 0x2000 : 2;
        break;
    case 0xC:
        unk108 |= (skelFlags & 0xC0) ? 0x1000 : 2;
        break;
    case 0x11:
    case 0x13:
        unk108 |= 0x4000;
        break;
    case 0x14:
        unk108 |= 0x100;
        break;
    }
}

// v12 0x0045A420  (feb 0x00451A10)
// Time to throw: sees the enemy, from throwMin to throwMax on the level, within throwAngle of its facing.
void aiSKEL_SPEAR::FillAttackFlag()
{
    if (inst->seq == 1 || inst->seq == 2)
        return;
    aiFlags &= ~0x1000u;
    if ((int)aiFlags < 0 || !(aiFlags & 0x10000000) || (wrFlags & 0x200))
        return;
    if (throwTimer > 0.01f)
        return;
    m3dV p, d, t;
    GetPosRooted(&p);
    inst->GetDir(&d);
    if (!(aiFlags & 0x40000) || (wrFlags & 0x10))
        enemy.GetPos(&t);
    else if (!aiAttackPlanner->GetEnemyMeetPoint(this, 27.0f, &t))
        return;
    float xz = (p.x - t.x) * (p.x - t.x) + (p.z - t.z) * (p.z - t.z);
    if (throwMin * throwMin <= xz && xz <= throwMax * throwMax && (weaponFlags & 2)) {
        m3dV to = { t.x - p.x, t.y - p.y, t.z - p.z };
        if (fabsf(m3dAngleVector(&to, &d)) <= throwAngle)
            aiFlags |= 0x1000;
    }
}

// v12 0x0045A5B0  (feb 0x00451BC0)
void aiSKEL_SPEAR::Think()
{
    aiNPC_WR::Think();
    FillAttackFlag();
    if (weaponFlags & 2) {
        unk37F = 27.0f;
        if (enemyDist >= 20.0f && !(wrFlags & 0x10))
            aiFlags |= 0x20;
        else
            aiFlags &= ~0x20u;
    }
}

// v12 0x0045A670  (feb 0x00451C20)
void aiSKEL_SPEAR::SetMovementSpeed()
{
    if (aiFlags & 0x40000) {
        turnSpeed = 90.0f;
        return;
    }
    int s = inst->seq;
    if (s != 0 && s != 8)
        aiFlags |= 0x200;
    if (aiFlags & 0x10) {
        turnSpeed = 120.0f;
        return;
    }
    if (s == 3 || s == 9 || s == 10) {
        turnSpeed = weaponTimer > 0.01f ? 90.0f : 0.0f;
        return;
    }
    if (s == 7) {
        turnSpeed = 90.0f;
        return;
    }
    if (s == 0xB || s == 0xC || s == 0x13 || s == 0x12 || s == 0x11) {
        turnSpeed = 120.0f;
        return;
    }
    m3dV d, t, p;
    inst->GetDir(&d);
    GetPlayerTargetPos(&t);
    inst->GetPos(&p);
    m3dV to = { t.x - p.x, 0, t.z - p.z };
    d.y = 0;
    if (m3dAngleVector_COS(&to, &d) > 0.2f) {
        turnSpeed = 90.0f;
        moveSpeed = 13.0f;
    } else {
        turnSpeed = 225.0f;
        moveSpeed = 6.5f;
    }
}

// v12 0x0045A7F0  (feb 0x00451DA0)
int aiSKEL_SPEAR::SelectGoal()
{
    return aiSKEL_GENERIC::SelectGoal();
}

// v12 0x0045A800  (feb 0x00451DB0)
void aiSKEL_SPEAR::MoveToGoal()
{
    if (inst->seq == 1)
        aiFlags |= 0x200;
    aiSKEL_GENERIC::MoveToGoal();
}

// v12 0x0045A830  (feb 0x00451DE0)
void aiSKEL_SPEAR::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    if (m3dUpdateTimeField(&timer577, gsElapsedTime))
        timer577 = 10.0f;
    m3dUpdateTimeField(&weaponTimer, gsElapsedTime);
    m3dUpdateTimeField(&throwTimer, gsElapsedTime);
}

// v12 0x0045A880  (feb 0x00451E30)
void aiSKEL_SPEAR::Slot33()
{
    if (weaponFlags & 8) {
        appearSfx->Start(curWeapon == swordObj);
        weaponFlags &= ~8u;
    }
}

// v12 0x0045A8C0  (feb 0x00451E70)
int aiSKEL_SPEAR::Activate(int force)
{
    if (weaponFlags & 2) {
        curWeapon = spearObj;
        weaponFlags &= ~4u;
    } else {
        curWeapon = swordObj;
        weaponFlags = (weaponFlags & ~2u) | 4;
    }
    ShowWeapon();
    return aiSKEL_GENERIC::Activate(force);
}

