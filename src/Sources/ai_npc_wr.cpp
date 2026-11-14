// ai_npc_wr.cpp -- the game's layer over aiNPC: blood, gibs, knock-back, altar opponents, areas (Will Rock 1.2, v12.exe;
// feb obj ai_npc_wr.obj)

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../../include/sfx_sob/sfx_api.h"
#include "../../include/frontend/wr_game.h"

int aiNPC_WR::dbgShow;                                  // v12 0x0064C928

// v12 0x00452CE0  (feb 0x0044A720)
void aiNPC_WR::ParseNameClass()
{
    const char *c = inst->className ? inst->className : "";
    aiNPC::ParseNameClass();
    if (strstr(c, "_niche")) {
        if (aiFlags & 0x10)
            aiFlags2 |= 0x20;
        else
            apLogErr((char *)"AI: niche actor '%s' has no waypoint", inst->name ? inst->name : "");
    }
    if (strstr(c, "_noappearsfx"))
        wrFlags |= 0x2000000;
}

// v12 0x00452D70  (feb 0x0044A7C0)
int aiNPC_WR::ProcessINIT(void *init)
{
    if (!aiNPC::ProcessINIT(init))
        return 0;
    wrFlags = 0;
    fallbackAngle = 90.0f;
    waterDepth = 2.0f;
    physDRIFT_MATH *ph = new physDRIFT_MATH;
    if (ph) {
        ph->flags = 0;
        ph->unkA8 = 0;
        ph->owner = this;
        ((m3dMATR *)ph->matr)->Identity();
    }
    passivePhys = ph;
    ph->drift = m3dVZero;
    shootFallbackTimer = 0;
    unk523 = 0;
    physINIT d;
    d.packId = 0;
    d.tag = 0x40;
    d.size = 0x38;
    d.vel[0] = d.vel[1] = d.vel[2] = 0;
    d.accel[0] = m3dVUnitY.x * -12.0f;
    d.accel[1] = m3dVUnitY.y * -12.0f;
    d.accel[2] = m3dVUnitY.z * -12.0f;
    d.radius = 0.6f;
    d.lifetime = -1.0f;
    d.unk24 = -1;
    d.unk28[0] = 0;
    *(float *)&d.unk28[1] = 0.8f;
    d.unk28[2] = 2;
    d.unk28[3] = 0;
    passivePhys->ProcessINIT(&d);
    inst->flags &= ~0x10u;
    char s[0x40];
    if (GetStr((char *)"AI_PROP", (char *)"Aiming", s, 0x40) && !_stricmp(s, "Plain"))
        wrFlags |= 0x10;
    int b;
    if (IsObstAvoidForced())
        navFlags &= ~0x20000u;
    else if (GetBool((char *)"AI_PROP", (char *)"ObstAvoid", &b) && b)
        navFlags &= ~0x20000u;
    else
        navFlags |= 0x20000;
    if (GetBool((char *)"AI_PROP", (char *)"isFallback", &b) && !b)
        wrFlags |= 0x100;
    else
        wrFlags &= ~0x100u;
    if (GetBool((char *)"AI_PROP", (char *)"isWayPTurn", &b) && b)
        wrFlags |= 0x400;
    else
        wrFlags &= ~0x400u;
    if (GetBool((char *)"AI_PROP", (char *)"isNoSFXAppear", &b) && b)
        wrFlags |= 0x2000000;
    wrFlags |= 0x10000;
    if ((aiFlags & 0x10) && (aiFlags & 0x100000)) {
        m3dV p;
        inst->GetPos(&p);
        m3dV to = { goalPos.x - p.x, goalPos.y - p.y, goalPos.z - p.z };
        inst->SetDir_XZ(&to);
        inst->GetDir(&dir);
    }
    if (wrSysStat)
        WR_FIELD(int, wrSysStat, 0x98)++;
    return 1;
}

// v12 0x004530C0  (feb 0x0044AAE0)
void aiNPC_WR::ProcessTERM()
{
    TermSFX();
    if (passivePhys) {
        passivePhys->ProcessTERM();
        passivePhys = 0;
    }
    aiNPC::ProcessTERM();
}

// v12 0x004530F0  (feb 0x0044AB10)
void aiNPC_WR::Explode(dmgDAMAGE *d, int dead)
{
    m3dV p;
    GetPosRooted(&p);
    wrsfxStartChunkMeat(3, 0, &p, 0, this);
    StartBlood(7, 0, &p, 0, 0, 0);
    inst->flags |= 1;
}

// v12 0x00453140  (feb 0x0044AB60)
// An explosive weapon (6, 9, 10) going off within a metre gibs it.
void aiNPC_WR::DecideExplosion(dmgDAMAGE *d)
{
    if (!d || d->type != 1 || (wrFlags & 4) || !IsGibbable())
        return;
    dmgWEAPON *w = (dmgWEAPON *)d;
    if (w->weaponType != 6 && !(w->weaponType > 8 && w->weaponType < 11))
        return;
    m3dV p;
    GetPosRooted(&p);
    float dx = p.x - w->hitPos.x, dz = p.z - w->hitPos.z;
    if (m3dSqrt(dx * dx + dz * dz) < 1.0f)
        Explode(d, 0);
}

// v12 0x00453270  (feb 0x0044AC90)
void aiNPC_WR::StartDeath(dmgDAMAGE *d)
{
    aiNPC::StartDeath(d);
    if (!(entFlags & 4))
        return;
    if (wrSysStat && d)
        wrSysStat->Notify_NPCDie(d->sourceId);
    if ((entFlags & 4) && IsAnimSeqGoodForFallback() && StartPassivePhysics(d)) {
        aiFlags |= 0x100;
        DecideExplosion(d);
        mpState.flags = 0x8B;
    }
}

// v12 0x004532F0  NEW in 1.2 (name ours: the slot's override)
// Near the end of the death sequence, after 0.2 s: the pose is kept (animINST v12 0x004F2E20).
void aiNPC_WR::UpdateDead()
{
    if (!(wrFlags & 0x4000000) && deadTimer > 0.2f) {
        float len = inst->tpl->seq[inst->seq].lengthMs * 0.001f * 0.8f;
        if (len < inst->seqTime) {
            inst->SnapshotPose();
            wrFlags |= 0x4000000;
        }
    }
    aiNPC::UpdateDead();
}

// v12 0x00453370  (feb 0x0044AD00)
int aiNPC_WR::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (d->type == 1) {
        int t = ((dmgWEAPON *)d)->weaponType;
        if (t == 3 || t == 1 || t == 8 || t == 2) {
            objOBJ *o = objFindID(inst->root, ((dmgWEAPON *)d)->unk2E);
            if (o && IsArmoredObject(o->name))
                return 1;
        }
    }
    float hp = health - d->amount;
    aiNPC::ProcessCDT_FIREABLE(d);
    if (wrFlags & 1)
        return 1;
    wrFlags |= 1;
    if (!bloodObj)
        return 1;
    if (!(wrFlags & 0x1000004) && IsGibbable() && (aiFlags & 0x200000) && maxHealth * -4.0f > hp) {
        Explode(d, 1);
        wrFlags |= 0x1000000;
        return 1;
    }
    if (!(wrFlags & 6) && (corpseFadeTime + corpseTime) * 0.5f >= deadTimer) {
        m3dV *at = (d->type == 1 || d->type == 3 || d->type == 4) ? &((dmgGENERAL *)d)->pos : 0;
        m3dV o;
        bloodObj->GetOrigin(&o);
        int kind = -1;
        if (d->type == 1) {
            int t = ((dmgWEAPON *)d)->weaponType;
            if (t == 6)
                kind = 1;
            else if (t == 8 || t == 3 || t == 1 || t == 0 || t == 2)
                kind = 0;
        } else {
            kind = 2;
        }
        if (kind >= 0)
            StartBlood(kind, aiFlags & 0x200000, &o, 0, (m3dV *)velocity, at);
    }
    if (!(aiFlags & 0x200000))
        SetInitShootFallBack((dmgWEAPON *)d);
    return 1;
}

#pragma pack(push, 1)
struct aiBLOOD_MSG : msgDATA {                          // 0x3E bytes
    short type;                                         // +0x04 3
    int   kind;                                         // +0x06
    u32   flags;                                        // +0x0A 1 dead, 2 no dir, 4 no vel, 8 no extra
    m3dV  pos;                                          // +0x0E
    m3dV  dir;                                          // +0x1A
    m3dV  vel;                                          // +0x26
    m3dV  extra;                                        // +0x32
};
#pragma pack(pop)

// v12 0x00453590  (feb 0x0044AF20)
void aiNPC_WR::ProcessMP_NOTIFY(gsEVENT *ev)
{
    aiNPC::ProcessMP_NOTIFY(ev);
    u8 *b = (u8 *)ev;
    short type = *(short *)(b + 4);
    if (type == -100) {
        SyncSFX(ev);
    } else if (type == 2) {
        if (wrSysStat) {
            dmgDAMAGE *d = *(dmgDAMAGE **)(b + 6);
            wrSysStat->Notify_NPCDie(d ? d->sourceId : (u32)-1);
        }
    } else if (type == 3) {
        aiBLOOD_MSG *m = (aiBLOOD_MSG *)ev;
        wrsfxStartBlood(m->kind, m->flags & 1, &m->pos, (m->flags & 2) ? 0 : &m->dir, (m->flags & 4) ? 0 : &m->vel,
                        (m->flags & 8) ? 0 : &m->extra);
    }
}

// v12 0x00453650  (feb 0x0044B070)
// To everyone, the server too; never in the no-blood mode (wrAppState 2).
void aiNPC_WR::StartBlood(int kind, int dead, m3dV *at, m3dV *d, m3dV *vel, m3dV *extra)
{
    if (!(entFlags & 4) || (wrAppState & 2) || Slot114())
        return;
    aiBLOOD_MSG m;
    m.packId = 0;
    m.tag = 0x40;
    m.size = 0x3E;
    m.type = 3;
    m.kind = kind;
    m.flags = (aiFlags & 0x200000) ? 1 : 0;
    m.pos = *at;
    if (d)
        m.dir = *d;
    else
        m.flags |= 2;
    if (vel)
        m.vel = *vel;
    else
        m.flags |= 4;
    if (extra)
        m.extra = *extra;
    else
        m.flags |= 8;
    gsSysServer->SendMsgClientAll(this, 0x40E, &m, 0x90000, -3, -3);
}

// v12 0x00453780  (feb 0x0044B1B0)
void aiNPC_WR::StartChunkMeat(int a, int b, m3dV *p, m3dV *d)
{
    wrsfxStartChunkMeat(a, b, p, d, this);
}

// v12 0x004537A0  (feb 0x0044B1D0)
int __fastcall ClassifyCollisionTarget(cdtINFO *ci)
{
    animINST *a = ci->inst;
    if (!a || (a->flags & 0x48))
        return 0;
    if (a->ent->classId == AI_SIG_PLAYER)
        return 1;
    if (a->ent->classId == AI_SIG_NPC)
        return (a->ent->unk98 != 0) + 2;
    return -1;
}

// v12 0x004537F0  (feb 0x0044B220)
// A hit knocks it back: a small jump away (a sniper not at all, unless dead).
void aiNPC_WR::SetInitShootFallBack(dmgWEAPON *w)
{
    if (w->type != 1 || (wrFlags & 0x120))
        return;
    if ((aiFlags & 0x40000) && !(aiFlags & 0x200000))
        return;
    if (!IsAnimSeqGoodForFallback())
        return;
    wrFlags |= 0x20;
    float back = -8.0f, up = 4.0f;
    AdjustSpeedsOnFallbackStart(w, &back, &up);
    float k = 90.0f / (fallbackAngle + 0.01f);
    StartJump(back * k, up * k);
}

// v12 0x004538B0  (feb 0x0044B2C0)
void aiNPC_WR::Think()
{
    aiNPC::Think();
    if (!(aiFlags & 0x800))
        wrFlags &= ~0x20u;
    if ((aiFlags & 0x40000) || !(aiFlags2 & 0x80) || enemyDist <= shootRange)
        wrFlags &= ~0x200u;
    else
        wrFlags |= 0x200;
    if (wrFlags & 0x10000)
        wrFlags &= ~0x40000u;
}

// v12 0x00453920  (feb 0x0044B330)
void aiNPC_WR::PostThink()
{
    if ((aiFlags & 0x10) && (aiFlags & 1) && (wrFlags & 0x400))
        OrientTowardsPlayer();
    aiNPC::PostThink();
}

// v12 0x004539B0  (feb 0x0044B3C0)
// The speed a death throws it with, by weapon: away from the hit, up, less with distance for a blast.
void aiNPC_WR::SetInitDeadSpeed(dmgWEAPON *w, m3dV *v)
{
    m3dV p, d;
    float dist;
    GetPosRooted(&p);
    dmgGetOrgDir(w, &p, &d, v, &dist);
    float k = 0;
    switch (w->weaponType) {
    case 0:
    case 1:
        v->y = 2.0f;
        break;
    case 2:
        k = dist >= 0.0f ? (dist <= 20.0f ? 2.0f - dist * 0.075f : 0.5f) : 2.0f;
        v->y = 3.5f;
        break;
    case 3:
    case 8:
        v->y = 3.1f;
        break;
    case 4:
        v->y = 3.0f;
        break;
    case 6:
    case 9:
        k = dist >= 0.0f ? (dist <= 15.0f ? 1.0f - dist * 0.0533333f : 0.2f) : 1.0f;
        v->y = 3.5f;
        break;
    }
    if (k) {
        v->x *= k;
        v->z *= k;
    }
    float s = 90.0f / (fallbackAngle + 0.01f);
    v->x *= s;
    v->y *= s;
    v->z *= s;
}

// v12 0x00453B80  (feb 0x0044B590)
int aiNPC_WR::StartPassivePhysics(dmgDAMAGE *d)
{
    if (!d || d->type != 1)
        return 0;
    int t = ((dmgWEAPON *)d)->weaponType;
    if ((t == 3 || t == 1) && m3dRandom() > 0.5f)
        return 0;
    if (t == 7 || t == 5)
        return 0;
    physINIT i;
    i.vel[0] = i.vel[1] = i.vel[2] = 0;
    i.radius = 0.6f;
    i.lifetime = -1.0f;
    i.unk24 = -1;
    i.unk28[0] = 0;
    *(float *)&i.unk28[1] = 0.8f;
    i.unk28[2] = 2;
    i.unk28[3] = 0;
    i.packId = 0;
    i.tag = 0x40;
    i.size = 0x38;
    SetInitDeadSpeed((dmgWEAPON *)d, (m3dV *)i.vel);
    i.accel[0] = m3dVUnitY.x * -12.0f;
    i.accel[1] = m3dVUnitY.y * -12.0f;
    i.accel[2] = m3dVUnitY.z * -12.0f;
    passivePhys->ProcessINIT(&i);
    fallSpeed = passivePhys373;
    m3dMATR m;
    memcpy(&m, inst->matr, sizeof(m));
    m3dV r;
    GetPosRooted(&r);
    m.Translate(&r, 0);
    m.Translate(0, i.accel[2] + fallSpeed, 0, 2);
    memcpy(passivePhys->matr, &m, sizeof(m));
    passivePhys->flags |= 4;
    passivePhys->AddEntCDTExclude(this);
    if (inst)
        inst->flags &= ~0x10u;
    return 1;
}

// v12 0x00453D90  (feb 0x0044B7A0)
void aiNPC_WR::ProcessINIT_LEVEL()
{
    aiNPC::ProcessINIT_LEVEL();
    InitSFX();
    inst->GetPos((m3dV *)areaFlags);
}

// v12 0x00453DC0  (feb 0x0044B7D0)
// Its appearing effect; an altar opponent stays hidden (message 0x4D4 brings the fireball), four times as strong.
int aiNPC_WR::InitSFX()
{
    int altar = 0;
    if (!(flags2 & 1) || (gsAppState & 0x20000))
        return 1;
    GetBool((char *)"AI_PROP", (char *)"isAltarOpp", &altar);
    if (DoesNPCAllowSFXAppear() && !(wrFlags & 0x2000000) && !altar) {
        m3dV p;
        inst->GetPos(&p);
        wrsfxAPPEAR_Start(&p, aiFlags2 & 1, 0);
    }
    if (altar && spawnDomain) {
        if (entFlags & 4) {
            msgSystem.PostMsg(this, 0x4D4, this);
            wrFlags |= 8;
        }
        inst->flags |= 1;
        health *= 4.0f;
    }
    return 1;
}

// v12 0x00453EB0  (feb 0x0044B8D0)
void aiNPC_WR::SyncSFX(gsEVENT *)
{
    int altar = 0;
    GetBool((char *)"AI_PROP", (char *)"isAltarOpp", &altar);
}

// v12 0x00453EE0  (feb 0x0044B920)
int aiNPC_WR::IsReadyActivate()
{
    if (wrFlags & 8)
        return 0;
    return aiNPC::IsReadyActivate();
}

// v12 0x00453F00  NEW in 1.2
int aiNPC_WR::CanPlanMove()
{
    return !(wrFlags & 8);
}

// v12 0x00453F20  (feb 0x0044B940)
void aiNPC_WR::ProcessFRAME()
{
    aiNPC::ProcessFRAME();
    wrFlags &= ~1u;
}

// v12 0x00453F40  (feb 0x0044B960)
void aiNPC_WR::OrientTowardsPlayer()
{
    m3dV p, t;
    inst->GetPos(&p);
    GetPlayerTargetPos(&t);
    m3dV to = { t.x - p.x, t.y - p.y, t.z - p.z };
    inst->SetDir_XZ(&to);
    inst->GetDir(&dir);
}

// v12 0x00453FB0  (feb 0x0044B9D0)
void aiNPC_WR::CorrectGoalByWaterLevel()
{
    m3dV p;
    inst->GetPos(&p);
    float y = goalPos.y - waterDepth;
    waterIsUnderWater(&p, &y, 0.1f);
    if (goalPos.y <= y + waterDepth)
        goalPos.y = y + waterDepth;
}

// v12 0x00454030  (feb 0x0044BA50)
void aiNPC_WR::UpdateTimers()
{
    aiNPC::UpdateTimers();
    m3dUpdateTimeField(&shootFallbackTimer, gsElapsedTime);
    m3dUpdateTimeField(&unk523, gsElapsedTime);
}

// v12 0x00454060 (name ours)
void __fastcall aiNPC_WR::DbgToggleShow()
{
    dbgShow = !dbgShow;
}

// v12 0x00454370  (feb 0x0044BCE0)
int aiNPC_WR::SelectEscapeWhenOut()
{
    if ((wrFlags & 0x40000) && !(aiFlags & 1))
        return 1;
    if (wrFlags & 0x10000)
        return 0;
    wrFlags |= 0x40000;
    aiAttackPlanner->GetFleeGoalPoint(this, m3dRandRange(4.0f, 8.0f), &goalPos);
    wayPointPos = goalPos;
    return 1;
}

// v12 0x004543F0  (feb 0x0044BD60)
void aiNPC_WR::RestrictGoalByBanCross()
{
    if ((wrFlags & 0x40000) && (wrFlags & 0x80000))
        return;
    scnDOMAIN *dom = aiAttackPlanner->GetBanCrossDomain(this);
    if (!dom)
        return;
    m3dV r;
    GetPosRooted(&r);
    if (dom->IsLSegCrossBoundaryXZ(&r, &goalPos)) {
        SelectGoalToRunAway(m3dRandRange(4.0f, 8.0f));
        if (dom->IsLSegCrossBoundaryXZ(&r, &goalPos)) {
            goalPos = *(m3dV *)areaFlags;
            wayPointPos = goalPos;
            wrFlags |= 0x40000;
        }
    }
}

// v12 0x004544C0  (feb 0x0044BE30)
void aiNPC_WR::SelectGoalToRunAway(float dist)
{
    aiAttackPlanner->GetFleeGoalPoint(this, dist, &goalPos);
    wayPointPos = goalPos;
    wrFlags |= 0x40000;
}

// v12 0x00454510  (feb 0x0044BE80)
// Five times a second: are the enemy and the NPC in the nav area; out over a ban cross: back to the last good place.
void aiNPC_WR::UpdateInAreaFlags()
{
    if (!(aiFlags2 & 8)) {
        wrFlags |= 0x10000;
        return;
    }
    if (unk523 >= 1e-6f)
        return;
    scnDOMAIN *area = (scnDOMAIN *)navArea, *ban = (scnDOMAIN *)navBanCross;
    m3dV e, r;
    enemy.GetPos(&e);
    if (area->IsBelongPointXZ(&e))
        wrFlags |= 0x20000;
    else
        wrFlags &= ~0x20000u;
    GetPosRooted(&r);
    if (area->IsBelongPointXZ(&r)) {
        wrFlags = (wrFlags & ~0x80000u) | 0x10000;
        *(m3dV *)areaFlags = r;
    } else {
        wrFlags &= ~0x10000u;
    }
    if ((aiFlags2 & 0x10) && !(wrFlags & 0x10000) && ban->IsBelongPointXZ(&r)) {
        if (!(wrFlags & 0x80000)) {
            wrFlags |= 0x40000;
            goalPos = *(m3dV *)areaFlags;
            wayPointPos = goalPos;
        }
        wrFlags |= 0x80000;
    }
    unk523 = 0.2f;
    if ((wrFlags & 0x10000) && (aiFlags2 & 0x10) && ban->IsBelongPointXZ(&r))
        apLogErr((char *)"AI: nav ban areas intersection error at %f %f %f", (double)r.x, (double)r.y, (double)r.z);
}

// v12 0x004546E0  (feb 0x0044C050)
void aiNPC_WR::ProcessAltarOppUnhide()
{
    inst->flags &= ~1u;
    OrientTowardsPlayer();
    if ((entFlags & 4) && (wrFlags & 8))
        wrFlags &= ~8u;
}

// v12 0x00454780  (feb 0x0044C0F0)
BOOL32 aiNPC_WR::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x4D4)
        wrsfxALTAR_FIREBALL_StartFireBall(this, WR_FIELD(psSHEET *, WR_FIELD(u8 *, spawnDomain, 0xCC), 0xF0));
    return aiNPC::ProcessMsg(id, data, from, ret);
}

// ---- the base slots with nothing in them --------------------------------------------------------------------------
int  aiNPC_WR::IsAnimSeqGoodForFallback() { return 0; }  // v12 0x0048DF30
void aiNPC_WR::AdjustSpeedsOnFallbackStart(dmgWEAPON *, float *, float *) {}    // v12 0x00403940
void aiNPC_WR::TermSFX() {}                              // v12 0x00401380
int  aiNPC_WR::DoesNPCAllowSFXAppear() { return 1; }     // v12 0x004031F0
int  aiNPC_WR::IsGibbable() { return 1; }                // v12 0x004031F0
int  aiNPC_WR::IsObstAvoidForced() { return 0; }         // v12 0x0048DF30
int  aiNPC_WR::IsArmoredObject(char *) { return 0; }     // v12 0x00403200
int  aiNPC_WR::Slot114() { return 0; }                   // v12 0x0048DF30
