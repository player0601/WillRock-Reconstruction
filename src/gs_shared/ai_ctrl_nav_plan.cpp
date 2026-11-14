// ai_ctrl_nav_plan.cpp -- where an NPC wants to go (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_ctrl_nav_plan.obj)
//
// 1.2 changes: SelectGoal checks aiFlags 0x20 with the enemy in sight only; StartResolveCycle handles water, the crumb trail
// and the new aiFlags 0x4000 ("give up this goal"); the arrival radius on a crumb is wider.

#include "../Sources/game.h"

// v12 0x0054FB60  (feb 0x00535760)
int aiNPC::SelectGoal_P()
{
    if (aiFlags & 0x10)
        return 1;
    if (aiFlags & 0x10000000)
        aiFlags &= ~8u;
    return SelectGoal();
}

// v12 0x0054FB90  (feb 0x00535790)
int aiNPC::SelectGoal()
{
    if ((aiFlags & 0x20) && (aiFlags & 0x10000000)) {
        aiAttackPlanner->GetEnemyMeetPoint(this, unk37F, &wayPointPos);
        if (aiFlags & 0x40000)
            inst->GetPos(&goalPos);
        else
            goalPos = wayPointPos;
        return 1;
    }
    if (aiFlags & 0x40000) {
        inst->GetPos(&goalPos);
        GetPlayerTargetPos(&wayPointPos);
        return 1;
    }
    if (aiFlags & 0x10000000) {
        aiAttackPlanner->GetAttackGoalPoint(this, &goalPos);
        GetPlayerTargetPos(&wayPointPos);
        return 1;
    }
    aiAttackPlanner->GetCrumbGoalPoint(this, &goalPos);
    wayPointPos = goalPos;
    if (aiFlags2 & 0x40)
        aiAttackPlanner->ClipPointByNavArea(this, &goalPos);
    aiFlags |= 8;
    return 1;
}

// v12 0x0054FD40  (feb 0x005358A0)
// Which side to step: 1 the left (a clear way, and from there the goal is in sight), 0 the right.
int aiNPC::SelectBasicStrafeDirection()
{
    cdtREFINE ref(2, 0);
    cdtINFO hit;
    memset(&hit, 0, sizeof(hit));
    m3dV eye, r, p;
    inst->GetPos(&eye);
    eye.y += heightCDT;
    inst->GetDirRight(&r);
    int h0 = gsScenePtr->ShootRay(&eye, &r, 10.0f, 0, &ref, &hit);
    float d0 = h0 ? hit.dist : 10.0f;
    r.x = -r.x;
    r.y = -r.y;
    r.z = -r.z;
    int h1 = gsScenePtr->ShootRay(&eye, &r, 10.0f, 0, &ref, &hit);
    float d1 = hit.dist;
    if (!h1 && (d1 = 10.0f, !h0)) {
        p.x = r.x * 10.0f + eye.x;
        p.y = r.y * 10.0f + eye.y;
        p.z = r.z * 10.0f + eye.z;
        int v0 = gsScenePtr->IsVisiblePoint(&p, &goalPos, 0, &ref, 0);
        r.x = -r.x;
        r.y = -r.y;
        r.z = -r.z;
        p.x = r.x * d0 + eye.x;
        p.y = r.y * d0 + eye.y;
        p.z = r.z * d0 + eye.z;
        int v1 = gsScenePtr->IsVisiblePoint(&p, &goalPos, 0, &ref, 0);
        if (!v0) {
            if (v1)
                return 0;
        } else if (!v1) {
            return 1;
        }
        return m3dRandSelect(1, 0);
    }
    return d0 > d1 ? 1 : 0;
}

// v12 0x0054FF80  (feb 0x00535AE0)
void aiNPC::StopStrafe()
{
    navFlags &= ~0x11u;
    strafeTimeLeft = 0;
    noStrafeTime = 0;
}

// v12 0x0054FFA0  (feb 0x00535B00)
void aiNPC::StartJump(float spd, float up)
{
    if (aiFlags & 0x800)
        return;
    m3dV d;
    inst->GetDir(&d);
    d.y = 0;
    m3dNormalize(&d);
    aiFlags |= 0x800;
    speed.y = up;
    speed.x += d.x * spd;
    speed.y += d.y * spd;
    speed.z += d.z * spd;
    mpState.flags = 0x8B;
}

// v12 0x00550050  (feb 0x00535BA0)
// Moved less than a fifth of what it was asked to: aiFlags 4, a cycle.
int aiNPC::CheckCycle()
{
    if (navFlags & 4)
        return 0;
    if ((aiFlags & 0x211) || (maxTurnStep < enemyAngle && !(aiFlags & 2))) {
        aiFlags &= ~4u;
        return 0;
    }
    m3dV p;
    inst->GetPos(&p);
    float dx = prevPos[0].x - p.x, dz = prevPos[0].z - p.z;
    float moved = m3dSqrt(dx * dx + dz * dz);
    if (fabsf(prevPos[0].y - p.y) <= gsElapsedTime) {
        float gx = prevPos[0].x - prevPos[2].x, gz = prevPos[0].z - prevPos[2].z;
        if (moved <= m3dSqrt(gx * gx + gz * gz) * 0.2f) {
            aiFlags |= 4;
            return 1;
        }
    }
    aiFlags &= ~4u;
    return 0;
}

// (inline three times in StartResolveCycle, once in AvoidObstacles)
static void aiStartStrafe(aiNPC *n)
{
    int left = n->noStrafeTime > 0.5f ? n->SelectBasicStrafeDirection() : (n->navFlags & 0x100);
    n->inst->GetDirRight(&n->strafeDir);
    if (left) {
        n->navFlags |= 0x110;
        n->strafeDir.x = -n->strafeDir.x;
        n->strafeDir.y = -n->strafeDir.y;
        n->strafeDir.z = -n->strafeDir.z;
    } else {
        n->navFlags &= ~0x110u;
    }
    n->strafeTimeLeft = n->strafeDuration;
    n->navFlags |= 1;
}

// v12 0x00550280  (feb 0x00535DF0)
void aiNPC::StartResolveCycle()
{
    if (aiFlags & 0x80000000) {                         // in water
        if (aiFlags & 0x10000000) {
            if (!(navFlags & 5))
                aiStartStrafe(this);
        } else {
            StartJump(-0.5f, 15.0f);
        }
        return;
    }
    if ((aiFlags & 0x10) && !Slot43())
        return;
    if (aiFlags & 8) {                                  // on the crumb trail
        if (cycleTime > 0.4f && !(navFlags & 5))
            aiStartStrafe(this);
        if (Slot43() || cycleTime > 1.0f)
            aiFlags |= 0x4000;
        return;
    }
    if (Slot67()) {
        StopStrafe();
        return;
    }
    if (!(navFlags & 5))
        aiStartStrafe(this);
}

// v12 0x00550590  (feb 0x00535F00)
// The speed first (a class may mark itself standing, aiFlags 0x200, there), arrived: standing; with aiFlags2 0x8000 and
// the enemy in sight, near enough to the goal counts as arrived; obstacles; the direction last -- standing, towards
// wayPointPos (the enemy, for a shooter). (Rebuilt first in feb's order, the direction first: a standing satyr then
// never turned to the player.)
void aiNPC::PlanMove()
{
    SetMovementSpeed();
    if (aiFlags & 1)
        aiFlags |= 0x200;
    if (!(aiFlags & 0x10) && (aiFlags2 & 0x8000) && (aiFlags & 0x10000000)
        && (!(aiFlags2 & 0x10000) || meetDist <= enemyDist)) {
        m3dV p;
        inst->GetPos(&p);
        float dx = p.x - goalPos.x, dy = p.y - goalPos.y, dz = p.z - goalPos.z;
        if (m3dSqrt(dx * dx + dy * dy + dz * dz) < navStuckTime)
            aiFlags |= 0x201;
    }
    AvoidObstacles();
    FillGoalDirection();
}

// v12 0x00550700  (feb 0x00536070)
void aiNPC::AnalyzeMove()
{
    float up = (aiFlags & 0x80000000) ? 6.0f : navAnalyzeDist;
    float arrive = (aiFlags & 8) ? 0.3f : navArriveDist;
    m3dV p;
    inst->GetPos(&p);
    float dx = p.x - goalPos.x, dz = p.z - goalPos.z;
    if (m3dSqrt(dx * dx + dz * dz) < arrive && fabsf(p.y - goalPos.y) < up) {
        aiFlags |= 1;
        arriveTime += gsElapsedTime;
    } else {
        arriveTime = 0;
        aiFlags &= ~1u;
    }
    m3dV to = { wayPointPos.x - p.x, wayPointPos.y - p.y, wayPointPos.z - p.z }, d;
    inst->GetDir(&d);
    if (fabsf(m3xzAngleVector(&d, &to)) < navAnalyzeAngle)
        aiFlags |= 2;
    else
        aiFlags &= ~2u;
    if (CheckCycle())
        StartResolveCycle();
}

// v12 0x00550900  (feb 0x00536270)
void aiNPC::UpdateUnderwater()
{
    aiFlags &= 0x7FFFFFFF;
    m3dV p;
    inst->GetPos(&p);
    if (waterIsUnderWater(&p, 0, 0.1f))
        aiFlags |= 0x80000000;
}

// v12 0x00550950  (feb 0x005362C0)
// Every resolvePeriod seconds a look moveSpeed * 2 periods ahead: anything in the way starts a strafe.
void aiNPC::AvoidObstacles()
{
    cdtREFINE ref(2, 0);
    if ((navFlags & 0xF0000) || (navFlags & 1) || resolveTimer > 0.0f)
        return;
    m3dV p, d;
    GetPosRooted(&p);
    inst->GetDir(&d);
    float len = moveSpeed * resolvePeriod + moveSpeed * resolvePeriod;
    float dx = p.x - obstPos.x, dy = p.y - obstPos.y, dz = p.z - obstPos.z;
    if (m3dSqrt(dx * dx + dy * dy + dz * dz) < len && m3dDistPointLine(&p, &obstPos, &obstEnd) < radCDT)
        return;
    obstPos = p;
    obstEnd.x = d.x + obstPos.x;
    obstEnd.y = d.y + obstPos.y;
    obstEnd.z = d.z + obstPos.z;
    resolveTimer = resolvePeriod;
    m3dV q = { d.x * len + p.x, d.y * len + p.y, d.z * len + p.z };
    if (!gsScenePtr->IsVisiblePoint(&p, &q, 0, &ref, 0) && !(navFlags & 5))
        aiStartStrafe(this);
}

// v12 0x00550C10  (feb 0x00536580)
void aiNPC::FillGoalDirection()
{
    int f = inst->tpl->seq[inst->seq].unk84;
    if ((f & 1) && (!(f & 2) || !IsPivotUsable())) {
        m3dV unused;
        inst->GetDir(&unused);
        return;
    }
    m3dV p, cur, to;
    inst->GetPos(&p);
    inst->GetDir(&cur);
    to.x = goalPos.x - p.x;
    to.z = goalPos.z - p.z;
    if (aiFlags & 0x40200) {
        to.x = wayPointPos.x - p.x;
        to.z = wayPointPos.z - p.z;
    }
    to.y = 0;
    if (!_m3dNormalize(&to))
        inst->GetDir(&to);
    goalAngleSigned = m3xzAngleVector(&to, &cur);
    goalAngle = fabsf(goalAngleSigned);
    goalDir = to;
    if (!(navFlags & 1))
        return;
    if (maxTurnStep < goalAngle) {
        navFlags &= ~0x11u;
        strafeTimeLeft = 0;
        noStrafeTime = 0;
        return;
    }
    float fwd, side;
    GetClampingSpeeds(moveSpeed, &fwd, &side);
    goalDir.x = to.x * fwd + side * strafeDir.x;
    goalDir.y = to.y * fwd + side * strafeDir.y;
    goalDir.z = to.z * fwd + side * strafeDir.z;
    _m3dNormalize(&goalDir);
}
