// ai_ctrl.cpp -- aiNPC, the control layer of every NPC (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_ctrl.obj)
//
// Reads the AI_PROP keys, passes the messages to its own slots, and each frame runs Update* (server / client / both).

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"

int aiDbgThinking;                                      // v12 0x009A89A8
aiNPC *aiDbgSelectedNPC;                                // v12 0x009A89BC
int aiDbgAllOff;                                        // v12 0x009A89C8
aiPLANNER *aiAttackPlanner;                             // v12 0x009A8C78
static void *aiCfgCache;                                // v12 0x009A89AC _apCfgReadBool's cache

// v12 0x00548A40  (feb 0x00535120)
static void aiTransformPoint(m3dMATR *m, m3dV *p)
{
    m3dV o;
    o.x = p->x * m->m[0][0] + p->y * m->m[1][0] + p->z * m->m[2][0] + m->m[3][0];
    o.y = p->x * m->m[0][1] + p->y * m->m[1][1] + p->z * m->m[2][1] + m->m[3][1];
    o.z = p->x * m->m[0][2] + p->y * m->m[1][2] + p->z * m->m[2][2] + m->m[3][2];
    *p = o;
}

// The creation data of a spawned NPC: its domain by MP id.  vtable v12 0x00608864, object v12 0x009A89B0
class aiINIT_DATA_PACK : public msgDATA_PACK {
public:
    // v12 0x005489D0
    BOOL32 Pack(fioFILE_MEM *f, msgDATA *d)
    {
        if (!msgDATA_PACK::Pack(f, d))
            return 0;
        aiINIT_DATA *i = (aiINIT_DATA *)d;
        if (i->spawnDomain)
            gsSysMP->WriteEIO((fioFILE *)f, (entIACTIVE_OBJ *)i->spawnDomain);
        return 1;
    }
    // v12 0x00548A10
    msgDATA *UnPack(fioFILE_MEM *f)
    {
        aiINIT_DATA *i = (aiINIT_DATA *)msgDATA_PACK::UnPack(f);
        if (i->spawnDomain)
            i->spawnDomain = (aiDOMAIN_SPAWN *)gsSysMP->ReadEIO((fioFILE *)f);
        return (msgDATA *)i;
    }
};
static aiINIT_DATA_PACK aiInitDataPack;

// v12 0x005489A0
aiINIT_DATA::aiINIT_DATA()
{
    msgDATA *d = (msgDATA *)hdr;
    d->packId = (u8)aiInitDataPack.id;
    d->tag = 0x40;
    d->size = 0x10;
    *(u32 *)(hdr + 4) = 0x41495349;
    spawnDomain = 0;
    spawnParam = 0;
}

// v12 0x005478E0  (feb 0x005340B0)
// Not rebuilt: aiaArenaInit (v12 0x00553460), the second message packer (v12 0x009A89C0).
int __fastcall aiInit()
{
    if (!aiPlannerInit() || !aiSpawnInit())
        return 0;
    msgSystem.RegisterMsgData(&aiInitDataPack);
    return 1;
}

// v12 0x00547920  (feb 0x005340F0)
// Not rebuilt: aiaArenaTerm (v12 0x005534B0).
void __fastcall aiTerm()
{
    aiPlannerTerm();
    aiSpawnTerm();
}

// v12 0x00547930  (feb 0x00534100)
// The next NPC ('$NPC' class id) after prev. Unless `all`: only those whose instance is not hidden, alive, and awake.
aiNPC *aiNPC::GetNext(aiNPC *prev, int all)
{
    entENTITY *e = entFindNext(prev);
    while (e) {
        if (e->GetClassID() == AI_SIG_NPC) {
            if (all)
                break;
            aiNPC *a = (aiNPC *)e;
            if (!(e->inst->flags & 1) && !a->IsDead() && !(a->aiFlags & 0x10000))
                break;
        }
        e = entFindNext(e);
    }
    return (aiNPC *)e;
}

// v12 0x00547990  (feb 0x00534160)
// How many live opponents of a class or (not an NPC) of a template the scene has.
int aiNPC::GetNOpp(char *className, char *tplName)
{
    int n = 0;
    for (animINST *i = gsScenePtr->FindNextInst(0); i; i = gsScenePtr->FindNextInst(i)) {
        entENTITY *e = i->ent;
        if (!e)
            continue;
        if (e->GetClassID() == AI_SIG_NPC) {
            aiNPC *a = (aiNPC *)e;
            if (a->IsDead())
                continue;
            if (a->aiFlags & 0x100000) {
                if ((a->aiFlags & 0x10000) && !(a->aiFlags & 0x80000))
                    continue;
            } else if (a->aiFlags & 0x10000) {
                continue;
            }
            if (!className || strstr(i->className, className))
                n++;
        } else if (tplName && strstr(i->tplName, tplName)) {
            n++;
        }
    }
    return n;
}

// v12 0x00547A60  (feb 0x00534210)
aiNPC::aiNPC()
{
    navFlags = 0;
    moveSpeed = 3.5f;
    minTurnRadius = 5.0f;
    turnSpeed = 180.0f;
    maxTurnStep = 90.0f;
    rootObj = 0;
    pivotObj = 0;
    radCDT = 0.7f;
    heightLowTester = 0.9f;
    heightCDT = 1.1f;
    unk177 = 1.1f;
    unk17B = 0;
    alertAngle = 180.0f;
    gravity = 30.0f;
    aimAngle = 90.0f;
    shootRange = 40.0f;
    meetDist = 10.0f;
    navStuckTime = 2.0f;
    navArriveDist = 0.5f;
    new1B7 = 0.3f;
    navAnalyzeDist = 4.0f;
    navAnalyzeAngle = 10.0f;
    navUnk1A7 = 20.0f;
    new (navArea) aiAREA;
    new (navBanCross) scnDOMAIN;
    passivePhys373 = 0.25f;
    fallAccel = -3.0f;
    fallSpeedMin = 0;
    unk37F = -888.0f;
    heightUp1 = 1.7f;
    heightUpTester = 1.1f;
    obstPos = m3dVUndef;
    obstEnd = m3dVUndef;
    resolveTimer = 0;
    resolvePeriod = 1.0f;
    strafeDuration = 2.0f;
    strafeTimeLeft = 0;
    noStrafeTime = 10.0f;
    strafeDir = m3dVUndef;
    dir = m3dVUndef;
    enemyDist = 888.0f;
    enemyHeight = 888.0f;
    enemyAngle = 888.0f;
    goalDir = m3dVUndef;
    goalAngleSigned = 888.0f;
    goalAngle = 888.0f;
    crumbID = -1;
    arriveTime = 0;
    prevPos[0] = prevPos[1] = prevPos[2] = m3dVUndef;
    speed = m3dVZero;
    passivePhys = 0;
    fallSpeed = 0;
    aiFlags = 0;
    aiFlags2 = 0;
    memset(&floor, 0, sizeof(floor));
    floorGap = 0;
    fallDist = 0;
    enemyCheckPeriodActive = 6;
    enemyCheckPeriodIdle = 10;
    enemyCheckCounter = 0;
    colorizeFrames = 5;
    colorizePhase = (int)m3dRandRange(0, 5.0f);
    sndChannel = 0;
    deadTimer = 0;
    corpseTime = 5.0f;
    corpseFadeTime = 4.0f;
    spawnDomain = 0;
    spawnParam = -1;
    radActivate = 30.0f;
    radActivateBack = 20.0f;
    timeNotSeen = 0;
    timeDeactivate = 40.0f;
    enemyChangeTimer = 15.0f;
    enemy.Assign(0);
    goalSeq = -1;
    goalSeq2 = 0;
    animRate2 = 1.0f;
    rootedMatr.Identity();
    seqFlags = 0;
    unk513 = 1.0f;
    unk108 = 0;
    maxHealth = 2.0f;
    health = 2.0f;
}

// v12 0x00547EC0  (feb 0x00534640)
BOOL32 aiNPC::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 1:
        if (ProcessINIT(data)) {
            ParseNameClass();
            SetupHeightCDTParams();
        }
        break;
    case 2:
        ProcessTERM();
        break;
    case 3:
        ProcessDESTROY_ENT((entENTITY *)data);
        break;
    case 1000:
        ProcessFRAME();
        break;
    case 0x3F4:
        ProcessINIT_LEVEL();
        break;
    case 0x3F8:
        ProcessIACTION(data);
        break;
    case 0x40E:
        ProcessMP_NOTIFY((gsEVENT *)data);
        break;
    case 0x4C6:
        *ret = ProcessCDT_FIREABLE((dmgDAMAGE *)data);
        return 1;
    case 0x4C5:
        ProcessCDT((cdtINFO *)data);
        break;
    case 0x4C7:
        *ret = ProcessASK_KEEP_LEVEL_RELOAD();
        return 1;
    case 0x4C8:
        if (data) {                                     // the world moved: so do its remembered places
            aiTransformPoint((m3dMATR *)data, &prevPos[0]);
            aiTransformPoint((m3dMATR *)data, &prevPos[1]);
        }
        break;
    }
    return 0;
}

// v12 0x00548120  (feb 0x005348A0)
int aiNPC::ProcessINIT(void *init)
{
    aiINIT_DATA *d = (aiINIT_DATA *)init;
    if (d && d->spawnDomain) {
        aiFlags |= 0x100000;
        spawnDomain = d->spawnDomain;
        spawnParam = d->spawnParam;
        // Left at the origin by its creator: the domain is asked (1.2 still puts it back at the origin)
        m3dMATR m;
        m._MakeLCS2WCS_VZ(&m3dVZero, &m3dVUnitZ);
        if (m3dIsEqualMatr_A(&m, (m3dMATR *)&inst->matr, 1e-6f, 1e-6f)) {
            m3dV pos, dirv;
            spawnDomain->GetSpawnPosDir(&pos, &dirv);
            m._MakeLCS2WCS_VZ(&m3dVZero, &m3dVUnitZ);
            inst->Transform(&m, 0);
        }
    }
    inst->SetAnimSeq(0, 0);
    entFlags |= 0x20;
    unkBC |= 4;
    aiFlags |= 0x1410000;
    mpState.flags = 0x8B;
    inactivityTimer = 0;
    cycleTime = 0;
    GetFloat((char *)"AI_PROP", (char *)"radCDT", &radCDT);
    GetFloat((char *)"AI_PROP", (char *)"radActivate", &radActivate);
    GetFloat((char *)"AI_PROP", (char *)"radActivateBack", &radActivateBack);
    int b;
    if (GetInt((char *)"AI_PROP", (char *)"isSniper", &b)) {
        if (b)
            aiFlags |= 0x40000;
        else
            aiFlags &= ~0x40000u;
    }
    if (GetInt((char *)"AI_PROP", (char *)"isPlrNoVisibility", &b) && b)
        aiFlags |= 0x20000;
    char s[0x40];
    if (GetStr((char *)"AI_PROP", (char *)"actorWayPoint", s, 0x40)) {
        animINST *w = gsScenePtr->FindInstName(s);
        if (w) {
            if (!w->ent || w->ent->nLocks <= 0) {
                aiFlags |= 0x10;
                w->GetPos(&goalPos);
                wayPointPos = goalPos;
            }
        } else {
            apLogErr((char *)"AI: waypoint '%s' is not found for actor %s", s, GetLogableName());
        }
    }
    if (GetStr((char *)"AI_PROP", (char *)"navArea", s, 0x40)) {
        objOBJ *o = objFindName(gsScenePtr->root, s);
        if (o) {
            ((scnDOMAIN *)navArea)->obj = o;
            o->SetStateProcYes(8, 0);
            if (((scnDOMAIN *)navArea)->Init())
                aiFlags2 |= 8;
        } else {
            apLogErr((char *)"AI: nav area '%s' is not found for actor %s", s, GetLogableName());
        }
    }
    if (GetStr((char *)"AI_PROP", (char *)"navBanCross", s, 0x40)) {
        objOBJ *o = objFindName(gsScenePtr->root, s);
        if (o) {
            ((scnDOMAIN *)navBanCross)->obj = o;
            o->SetStateProcYes(8, 0);
            if (((scnDOMAIN *)navBanCross)->Init())
                aiFlags2 |= 0x10;
        } else {
            apLogErr((char *)"AI: ban cross area '%s' is specified, but is not found for actor %s", s, GetLogableName());
        }
    }
    float size;
    if (GetFloat((char *)"AI_PROP", (char *)"size", &size))
        inst->Scale(size, size, size, 2);
    gsSysServer->DeclSyncable(this);
    idleTimer = 0;
    GetFloat((char *)"AI_PROP", (char *)"timeIdleAfterSpawn", &idleTimer);
    return 1;
}

// v12 0x005484F0  (a 1.2 base)
void aiNPC::ParseNameClass()
{
    if (strstr(inst->className, "_sniper")) {
        shootRange = 800.0f;
        aiFlags |= 0x40000;
        mpState.flags = 0x82;
    }
}

// v12 0x00548540  (feb 0x00534C90)
void aiNPC::SetupHeightCDTParams()
{
    if (!GetFloat((char *)"AI_PROP", (char *)"heightUpTester", &heightUpTester) && gsScenePtr && gsScenePtr->sheet)
        gsScenePtr->sheet->GetFloat((char *)"AI_PROP", (char *)"heightUpTester", &heightUpTester);
    if (heightUpTester > heightCDT)
        heightCDT = heightUpTester;
}

// v12 0x005485A0  (feb 0x00534CF0)
void aiNPC::ProcessINIT_LEVEL()
{
    SettleSurface();
    Colorize(0);
    inst->GetPos(&prevPos[0]);
    prevPos[1] = prevPos[0];
    prevPos[2] = prevPos[0];
    inst->GetDir(&dir);
    crumbID = -1;
}

// v12 0x00548620  (feb 0x00534D70)
void aiNPC::ProcessTERM()
{
    if (aiFlags2 & 8) {
        ((scnDOMAIN *)navArea)->Term();
        if (aiFlags2 & 0x10)
            ((scnDOMAIN *)navBanCross)->Term();
    }
    PlaySounds();
    if (aiFlags2 & 0x40000)
        aiAttackPlanner->SetBoss(0);
}

// v12 0x00548680  (feb 0x0049A5B0)
int aiNPC::ProcessASK_KEEP_LEVEL_RELOAD()
{
    return 1;
}

// v12 0x00548690  (feb 0x00534DD0)
void aiNPC::ProcessDESTROY_ENT(entENTITY *e)
{
    if (enemy.ent == e)
        enemy.Assign(0);
}

// v12 0x005486C0  (feb 0x00534E00)
void aiNPC::ProcessIACTION(void *ia)
{
    if (((apSTATE *)&pendingEvents)->IsState(0x80))
        Activate(1);
    apNAME *n = (apNAME *)((u8 *)ia + 8);
    if (n->IsName((char *)"ACTIVATE")) {
        if (!nLocks)
            Activate(1);
    } else if (n->IsName((char *)"ACTIVATE_PLR_VISIBILITY")) {
        ActivatePlrVisibility();
    } else if (n->IsName((char *)"UNLOCK")) {
        if (nLocks < 1)
            Activate(1);
    }
}

// v12 0x00548760  (feb 0x00534EA0)
// Nothing during a cut-scene that does not want it (the boss goes on); locked NPCs wait; the AI debug switch.
void aiNPC::ProcessFRAME()
{
    if ((gsSysCinema->state & 1) && gsSysCinema->IsSuspendAction() && !(aiFlags2 & 0x40000))
        return;
    if (!inst->root)
        return;
    if ((entFlags & 4) && nLocks > 0)
        return;
    if ((aiFlags & 0x1000000) && !(aiFlags & 0x810004))
        inst->flags &= ~0x2000000u;
    else
        inst->flags |= 0x2000000;
    if (aiFlags & 0x800000)
        return;
    int off = 0;
    _apCfgReadBool(&aiCfgCache, (char *)"Debug", (char *)"Decomp_OFF_AI", &off);
    if ((off | aiDbgAllOff) && this != aiDbgSelectedNPC && (entFlags & 4))
        return;
    aiDbgThinking = 1;
    if (entFlags & 4)
        UpdateOnServerOnly();
    else
        UpdateOnClientOnly();
    UpdateOnServerAndClient();
    aiDbgThinking = 0;
}

// v12 0x00548880  (feb 0x00534F90)
void aiNPC::UpdateTimers()
{
    if (aiFlags & 0x10000000)
        timeNotSeen = 0;
    else
        timeNotSeen += gsElapsedTime;
    if (IsInAnyStrafe()) {
        strafeTimeLeft -= gsElapsedTime;
        if (strafeTimeLeft < 0.0f)
            StopStrafe();
    } else {
        noStrafeTime += gsElapsedTime;
    }
    if (aiFlags & 0x100) {
        fallSpeed += gsElapsedTime * fallAccel;
        if (fallSpeed < fallSpeedMin)
            fallSpeed = fallSpeedMin;
    }
    m3dUpdateTimeField(&resolveTimer, gsElapsedTime);
    m3dUpdateTimeField(&idleTimer, gsElapsedTime);
    if (aiFlags & 4)
        cycleTime += gsElapsedTime;
    else
        cycleTime = 0;
    enemy.UpdateTimers();
}

// v12 0x00548990  (feb 0x00535070)
void aiNPC::PlaySounds()
{
    unk108 = 0;
}

// v12 0x00548D10  (feb 0x005353D0)
BOOL aiNPC::IsInAnyStrafe()
{
    return navFlags & 1;
}

// v12 0x00548D20  (feb 0x005353E0)
BOOL aiNPC::IsDead()
{
    return aiFlags & 0x200000;
}

// ---- the base slots with nothing in them --------------------------------------------------------------------------
void  aiNPC::FillSoundFlags() {}                         // v12 0x00403A20
void  aiNPC::Slot33() {}                                 // v12 0x00401380
void  aiNPC::Slot36() {}                                 // v12 0x00401380
void  aiNPC::SetMovementSpeed() {}                       // v12 0x00403A20
int   aiNPC::Slot43() { return 1; }                      // v12 0x004031F0
int   aiNPC::EvalAnimGoalSeq() { return 0; }             // v12 0x00502570
void  aiNPC::ProcessActionFrame() {}                     // v12 0x00401380
void  aiNPC::AnimateProcedural() {}                      // v12 0x00401380
int   aiNPC::Slot66(void *) { return 1; }                // v12 0x0048D590
int   aiNPC::Slot67() { return 0; }                      // v12 0x0048DF30
int   aiNPC::GetLayer1State() { return 0; }              // v12 0x0048DF30
int   aiNPC::GetLayer2State() { return 0; }              // v12 0x0048DF30
int   aiNPC::GetLayer3State() { return 0; }              // v12 0x0048DF30
void  aiNPC::SetLayer1State(int) {}                      // v12 0x00403950
void  aiNPC::SetLayer2State(int) {}                      // v12 0x00403950
void  aiNPC::SetLayer3State(int) {}                      // v12 0x00403950
void  aiNPC::Slot79() {}                                 // v12 0x00403A20
void  aiNPC::ProcessCDT(cdtINFO *) {}                    // v12 0x00403200
float aiNPC::Slot90() { return heightUp1; }              // v12 0x00430AD0
float aiNPC::GetGravity() { return gravity; }            // v12 0x00430AF0
float aiNPC::GetAnimPlayRate() { return 1.0f; }          // v12 0x00431E30
void  aiNPC::GetCenter(m3dV *out) { GetPosRooted(out); } // v12 0x00430AC0
void  aiNPC::Freeze() { aiFlags |= 0x800000; }           // v12 0x00430AE0
