// ai_ctrl_LOD.cpp -- when an NPC wakes and sleeps, and its frame (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_ctrl_LOD.obj)
//
// 1.2 changes: an NPC that leaves the level's box is deleted (UpdateOnServerOnly); CanPlanMove can stop the plan (a hidden
// altar opponent); a sleeping NPC clears its sounds; activation changes are sent to the clients (entFlags 0x10 / 0x280).

#include "../Sources/game.h"

int __fastcall gsIsInstInLevelBox(scnSCENE *s, animINST *i);    // v12 0x00508EB0 (gs:entity.obj)

// v12 0x0054F190  (feb 0x00533C40)
int aiNPC::Activate(int force)
{
    if (nLocks < 1) {
        inactivityTimer = 0;
        aiFlags = (aiFlags & ~0x10000u) | 0x80000;
        aiAttackPlanner->RegisterNPC(this);
        NotifyAIState();
        entFlags |= 0x10;
    }
    return 1;
}

// v12 0x0054F1F0  (feb 0x00533C90)
void aiNPC::Deactivate()
{
    aiFlags |= 0x10000;
    aiAttackPlanner->UnRegisterNPC(this);
    NotifyAIState();
    *(m3dV *)mpState.vel = m3dVZero;                    // +0x131 the snapshot's speed
    entFlags |= 0x280;
}

// v12 0x0054F250  (feb 0x00533CC0)
void aiNPC::ActivatePlrVisibility()
{
    aiFlags &= ~0x20000u;
}

// v12 0x0054F260  (feb 0x00533CD0)
int aiNPC::IsReadyActivate()
{
    return !(aiFlags & 0x20000) && enemy.ent && (aiFlags & 0x10000000);
}

// v12 0x0054F290  (feb 0x00533D00)
// Enemy out of sight too long, or too far (2 x radActivate; 1.7 x when not strict and 0.9 of the time).
BOOL32 aiNPC::CheckDeactivateCondition(int strict)
{
    if (aiFlags & 0x10000000)
        return 0;
    float far_;
    if (strict) {
        if (timeNotSeen > timeDeactivate)
            return 1;
        far_ = radActivate + radActivate;
    } else {
        if (timeDeactivate * 0.9f < timeNotSeen)
            return 1;
        far_ = radActivate * 1.7f;
    }
    return far_ < enemyDist && radActivate > 0.0f;
}

// v12 0x0054F330  (feb 0x00533DA0)
void aiNPC::Think()
{
    if (CheckDeactivateCondition(1))
        Deactivate();
}

// v12 0x0054F350  (feb 0x00533DC0)
// Reached the crumb: on to the next plan. Standing too near an instance that kills (entity flags 2 on the floor's entity,
// nearer than its head): 5000 of damage, subtype 4. (1.2 no longer walks the crumb trail here.)
void aiNPC::PostThink()
{
    if ((aiFlags & 1) && (aiFlags & 8))
        aiFlags = (aiFlags & ~1u) | 0x40;
    if ((aiFlags & 0x10) && (aiFlags & 1))
        aiFlags &= ~0x11u;
    if (floor.inst && (((entENTITY *)floor.inst->ent)->unkBC & 2) && heightCDT + radCDT > floor.dist) {
        static dmgDAMAGE d(0);
        d.subtype = 4;
        d.amount = 5000.0f;
        msgSystem.SendMsg(this, 0x4C6, &d, 0);
    }
}

// v12 0x0054F440  (feb 0x00533EE0)
void aiNPC::UpdateOnServerOnly()
{
    if (nLocks > 0 || !inst)
        return;
    DecideEnemy();
    FillToPlayerCoords();
    float cur, max;
    int type;
    if (!aiAttackPlanner->GetBossHealth(&cur, &max, &type))
        DieOfInactivity();
    if (!gsIsInstInLevelBox(gsScenePtr, inst))
        msgSystem.PostMsg(this, 2, this);
    if (aiFlags & 0x10000) {
        if (!IsReadyActivate())
            return;
        Activate(0);
    }
    int alive = !(aiFlags & 0x600000);
    GetPrevPositions();
    if (alive) {
        Think();
        if (aiFlags & 0x10000)
            return;
        if (!CanPlanMove())
            return;
        SelectGoal_P();
        PlanMove();
        MoveToGoal();
    }
    Animate();
    ProcessPhysics();
    if (alive) {
        AnalyzeMove();
        PostThink();
        UpdateTimers();
        return;
    }
    if (aiFlags & 0x200000)
        UpdateDead();
    UpdateTimers();
}

// v12 0x0054F5A0  (feb 0x00534010)
void aiNPC::UpdateOnServerAndClient()
{
    Colorize(1);
    Slot33();
    if (aiFlags & 0x10000) {
        unk108 = 0;
    } else {
        FillSoundFlags();
        PlaySounds();
    }
    aiFlags &= 0xDFBFF9FF;
}
