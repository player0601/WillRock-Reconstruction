// ai_enemy.cpp -- who an NPC fights (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_enemy.obj)

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"

// v12 0x0054C250  (feb 0x00530FE0)
void aiENEMY::GetPos(m3dV *out)
{
    if (ent && ent->inst) {
        ent->inst->GetPos(out);
        return;
    }
    if (plgsPlayer)
        plgsPlayer->inst->GetPos(out);
    else
        *out = m3dVZero;
}

// v12 0x0054C2A0  (feb 0x00531010)
void aiENEMY::GetDir(m3dV *out)
{
    if (ent && ent->inst) {
        ent->inst->GetDir(out);
        return;
    }
    if (plgsPlayer)
        plgsPlayer->inst->GetDir(out);
    else
        *out = m3dVUnitZ;
}

// v12 0x0054C450  (feb 0x00530FC0)
void *aiENEMY::GetBreadCrumb(int back)
{
    if (plr)
        return aiAttackPlanner->GetPlrCrumbs((plgsACTION *)plr);
    if (plgsPlayer)
        return aiAttackPlanner->GetPlrCrumbs(plgsPlayer);
    return 0;
}

// v12 0x0054C480  (feb 0x00531230)
void aiENEMY::Assign(entENTITY *e)
{
    if (!e) {
        ent = 0;
        timer = 0;
        plr = 0;
        return;
    }
    if (e != ent)
        timer = 0;
    ent = e;
    plr = e->classId == AI_SIG_PLAYER ? (plrPLAYER *)e : 0;
}

// v12 0x0054C4C0  (feb 0x00531270)
int aiENEMY::IsDead()
{
    if (ent) {
        if (ent->classId == AI_SIG_NPC)
            return ((aiNPC *)ent)->aiFlags & 0x200000;
        if (ent->classId == AI_SIG_PLAYER)
            return wrEVCall<BOOL32>(ent, 0x74);
    }
    return 1;
}

// v12 0x0054C4F0  (feb 0x005312A0)
int aiENEMY::IsActive()
{
    if (plr)
        return wrEVCall<BOOL32>(plr, 0x78);
    return ent && ent->classId == AI_SIG_NPC;
}

#pragma pack(push, 1)
struct aiSPEED_MSG : msgDATA {                          // 0x12 bytes
    short type;                                         // +0x04 -95
    m3dV  v;                                            // +0x06
};
#pragma pack(pop)

// v12 0x0054C520  (feb 0x005312D0)
// 1.2: a player's push goes through the server (to its client).
void aiENEMY::ApplySpeed(m3dV *v)
{
    if (!ent)
        return;
    if (ent->classId == AI_SIG_NPC) {
        ((aiNPC *)ent)->ApplySpeed(v);
    } else if (ent->classId == AI_SIG_PLAYER) {
        aiSPEED_MSG m;
        m.packId = 0;
        m.tag = 0x40;
        m.size = 0x12;
        m.type = -95;
        m.v = *v;
        gsSysServer->SendMsgClient(ent->mpOwner, ent, 0x40E, &m, 0x20000);
    }
}

// v12 0x0054C5A0  (feb 0x00531310)
void aiENEMY::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (ent)
        msgSystem.SendMsg(ent, 0x4C6, d, 0);
}

// v12 0x0054C5D0  (feb 0x00531340)
void aiENEMY::UpdateTimers()
{
    timer += gsElapsedTime;
}

// v12 0x0054C5E0  (feb 0x00531350)
// A look from the eyes to a random point round the target: inside radActivate, in the alert angle, nothing in the way.
BOOL aiNPC::IsPlrVisible(m3dV *pos)
{
    entCDT_REFINE_EXCL ref(2, inst, 1);
    if (aiFlags & 0x20000)
        return 0;
    m3dV eye, d, t;
    inst->GetPos(&eye);
    eye.y += heightUp1;
    inst->GetDir(&d);
    float a = m3dRandom() * 180.0f;
    t.x = sinf(a) * 0.6f + pos->x;
    t.y = m3dRandRange(1.0f, 2.0f) + pos->y;
    t.z = cosf(a) * 0.6f + pos->z;
    float front, back;
    if (aiFlags & 0x10000) {
        front = radActivate;
        back = radActivateBack;
    } else {
        front = back = radActivate * 1.5f;
    }
    return gsScenePtr->IsVisiblePointCone(&eye, &d, front, back, alertAngle, &t, 0, &ref, 0) != 0;
}

// v12 0x0054C720  (feb 0x00531480)
int aiNPC::ShouldChangeEnemy()
{
    if (!enemy.ent || enemy.IsDead() || !(aiFlags & 0x10000000) || CheckDeactivateCondition(0))
        return 1;
    return (aiFlags2 & 0x40000) && enemyChangeTimer < enemy.timer;
}

// v12 0x0054C7B0  (feb 0x00531510)
void aiNPC::DecideEnemy()
{
    if (enemyCheckCounter > 0) {
        enemyCheckCounter--;
        return;
    }
    if ((inst->flags & 2) && gsSysMP->mode == 1) {
        enemyCheckCounter = 0;
        aiFlags &= ~0x10000000u;
        enemy.ent = 0;
        enemy.plr = 0;
        enemy.timer = 0;
        return;
    }
    enemyCheckCounter = (aiFlags & 0x10000) ? enemyCheckPeriodIdle : enemyCheckPeriodActive;
    int saw = 0;
    if (ShouldChangeEnemy()) {
        aiFlags &= ~0x10000000u;
        entENTITY *e = SelectNewEnemy(&saw);
        if (!e) {
            enemy.ent = 0;
            enemy.plr = 0;
            enemy.timer = 0;
        } else {
            if (e != enemy.ent)
                enemy.timer = 0;
            enemy.ent = e;
            enemy.plr = e->classId == AI_SIG_PLAYER ? (plrPLAYER *)e : 0;
        }
    }
    if (!enemy.ent)
        return;
    if (!saw) {
        aiFlags &= ~0x10000000u;
        m3dV p;
        enemy.GetPos(&p);
        if (IsPlrVisible(&p))
            aiFlags |= 0x10000000;
    }
    if (enemy.ent)
        flags |= 2;
}

// v12 0x0054C970  (feb 0x005316A0)
entENTITY *aiNPC::SelectNewEnemy(int *sawIt)
{
    entENTITY *e = 0;
    *sawIt = 0;
    aiAttackPlanner->StartFilteringPlayers();
    aiAttackPlanner->FilterPlayersFor(this);
    if (!aiAttackPlanner->HaveAChoiceFromFiltered(&e))
        return e;
    aiAttackPlanner->RemoveHiddenFromFiltered(this);
    if (!aiAttackPlanner->HaveAChoiceFromFiltered(&e))
        return e;
    aiAttackPlanner->SortFilteredPlayersByDist(this);
    e = SelectFromSortedPlayers();
    *sawIt = 1;
    if (!e)
        e = (entENTITY *)aiAttackPlanner->GetFilteredByIdx(0);
    return e;
}

// v12 0x0054CA20  (feb 0x00531720)
entENTITY *aiNPC::SelectFromSortedPlayers()
{
    int n = WR_FIELD(int, aiAttackPlanner, 0x398);
    plgsACTION *first = 0;
    int i = 0;
    m3dV p;
    for (; i < n; i++) {
        first = aiAttackPlanner->GetFilteredByIdx(i);
        wrEVCall<void>(first, 0x64, &p);
        if (IsPlrVisible(&p)) {
            aiFlags |= 0x10000000;
            break;
        }
    }
    if ((aiFlags2 & 0x40000) && enemyChangeTimer < enemy.timer && (entENTITY *)first == enemy.ent) {
        for (; i < n; i++) {
            plgsACTION *a = aiAttackPlanner->GetFilteredByIdx(i);
            wrEVCall<void>(a, 0x64, &p);
            if (IsPlrVisible(&p))
                return a;
        }
    }
    return first;
}

// v12 0x0054C340  (feb 0x00531090): how fast the enemy (else the local player) moves
float aiENEMY::GetSpeedScalar()
{
    const float *v;
    if (ent)
        v = ent->velocity;
    else if (plgsPlayer)
        v = ((entENTITY *)plgsPlayer)->velocity;
    else
        v = &m3dVZero.x;
    return m3dSqrt(v[2] * v[2] + v[1] * v[1] + v[0] * v[0]);
}
