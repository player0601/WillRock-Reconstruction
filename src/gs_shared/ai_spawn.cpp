// ai_spawn.cpp -- spawn domains (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_spawn.obj)
//
// A "&dom_ai..." object spawns enemies of one class while a player is inside it: [AI_SPAWN] oppClass / oppName, up to
// nSpawn, nSpawnFirst at the start, then one each timeOppSpawn, at most nMaxSimult(Same) alive, timeAfterKill after a
// kill. Its areas are its sub-domains; where in them is the distribution (distant, even, formation) or a merged list
// of their points. Completes after nKillComplete kills, or (1.2) 10 s after the last spawn with none of its own alive.

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"

aiDOMAIN_LIST_SPAWN *aiSpawnDomList;      // v12 0x009A89E8 (feb 0x0095D96C)
static float aiSpawnOppCoeff = 1.0f;      // v12 0x0063721C (feb 0x005F8F74)
int aiSpawnEver;                          // v12 0x009A89F0 a spawn has happened (only written)

// The closest living player to a place, in XZ; else the first player; else zero. v12 0x0054AE40
static void aiGetClosestPlayerPos(m3dV *from, m3dV *out)
{
    float best = 10000.0f;
    int found = 0;
    *out = m3dVZero;
    for (entENTITY *p = gsSysMP->GetNextPlayer(0); p; p = gsSysMP->GetNextPlayer(p)) {
        if (wrEVCall<BOOL32>(p, 0x74) || !wrEVCall<BOOL32>(p, 0x90))
            continue;
        m3dV q;
        wrEVCall<void>(p, 0x64, &q);
        float dx = q.x - from->x, dz = q.z - from->z;
        float d = m3dSqrt(dx * dx + dz * dz);
        if (d < best) {
            best = d;
            *out = q;
            found = 1;
        }
    }
    if (found)
        return;
    entENTITY *p = gsSysMP->GetNextPlayer(0);
    if (p)
        wrEVCall<void>(p, 0x64, out);
    else
        *out = m3dVZero;
}

static inline aiSPAWN_AREA *subArea(scnDOMAIN *s)
{
    return s ? (aiSPAWN_AREA *)((u8 *)s - 0x98) : 0;
}

// --------------------------------------------------------------------------------------------- the list

// v12 0x00548D50  (feb 0x00539490)
int __fastcall aiSpawnInit()
{
    aiSpawnDomList = new aiDOMAIN_LIST_SPAWN;
    if (!aiSpawnDomList)
        return 0;
    aiSpawnDomList->_3c8 = 0;
    aiSpawnDomList->_3cc = -1;
    msgSystem.InsertAddr(aiSpawnDomList, 0x24535953, 0x53);
    return 1;
}

// v12 0x00548DC0  (feb 0x00539500)
void __fastcall aiSpawnTerm()
{
    msgSystem.DestroyAddr(aiSpawnDomList);
    aiSpawnDomList = 0;
}

// v12 0x00548DE0  (feb 0x00539520)
void __fastcall aiSpawnSetOppCoeff(float k)
{
    aiSpawnOppCoeff = (k > 0 && k <= 1.0f) ? k : 1.0f;
}

// v12 0x00548E20
float __fastcall aiSpawnGetOppCoeff()
{
    return aiSpawnOppCoeff;
}

// v12 0x00548E30  (feb 0x00539560)
int aiDOMAIN_LIST_SPAWN::IsAddObj(objOBJ *o)
{
    return strstr(o->name, "&dom_ai") != 0;
}

// v12 0x0054B060  (feb 0x0053B2F0)
gsDOMAIN *aiDOMAIN_LIST_SPAWN::MakeDomain()
{
    return new aiDOMAIN_SPAWN;
}

// v12 0x00548E50  (feb 0x00539580)
scnDOMAIN *aiDOMAIN_LIST_SPAWN::MakeSubDomain(gsDOMAIN *d)
{
    aiSPAWN_AREA *a = new aiSPAWN_AREA;
    if (!a)
        return 0;
    a->classId = 0x24444F4D;
    a->priority = 0x53;
    msgSystem.InsertAddr(a, 0, 0);
    a->domain = (aiDOMAIN_SPAWN *)d;
    return a;
}

// v12 0x00548F00  (feb 0x00539630)
void aiDOMAIN_LIST_SPAWN::DeleteSubDomain(scnDOMAIN *s)
{
    msgSystem.DestroyAddr(subArea(s));
}

// v12 0x00559550  (feb 0x00541000)
// aiwWAVE, the "&eio_aiw" objects' weighted spawning, is not rebuilt (no such object in the levels tried).
int aiDOMAIN_LIST_SPAWN::CreateWaves(objOBJ *o)
{
    if (o->name && strstr(o->name, "&eio_aiw"))
        apLogErr((char *)"aiwWAVE not rebuilt: %s", o->name);
    objOBJ *c = o->child;
    if (c) {
        objOBJ *s = c;
        do {
            if (!CreateWaves(s))
                return 0;
            s = s->next;
        } while (s != c);
    }
    return 1;
}

// v12 0x00548F30  (feb 0x00539660)
void aiDOMAIN_LIST_SPAWN::ProcessLEVEL_LOADED()
{
    gsDOMAIN_LIST::ProcessLEVEL_LOADED();
    CreateWaves(gsScenePtr->root);
}

// v12 0x00548F50  (feb 0x00539680)
int aiDOMAIN_LIST_SPAWN::ProcessINIT_LEVEL()
{
    return gsDOMAIN_LIST::ProcessINIT_LEVEL() != 0;
}

// v12 0x00548F60  (feb 0x00539690)
void aiDOMAIN_LIST_SPAWN::Update()
{
    if (gsSysMP->mode != 3)
        gsDOMAIN_LIST::Update();
}

// ------------------------------------------------------------------------------------------- the domain

// v12 0x0054B060's ctor part  (feb 0x0053B2F0)
aiDOMAIN_SPAWN::aiDOMAIN_SPAWN()
{
    nMerged = 0;
    merged = 0;
}

// v12 0x0054B000  (feb 0x0053B290)
aiDOMAIN_SPAWN::~aiDOMAIN_SPAWN()
{
    nSub = 0;
    if (merged)
        operator delete(merged);
    merged = 0;
}

// v12 0x00548F90  (feb 0x005396C0)
BOOL32 aiDOMAIN_SPAWN::Init()
{
    psSHEET *s = obj->props;
    if (!s)
        return 1;
    oppClass[0] = 0;
    oppName[0] = 0;
    s->GetStr((char *)"AI_SPAWN", (char *)"oppClass", oppClass, 0x40);
    if (oppClass[0])
        entGetClassTplName(oppClass, oppName);
    if (!oppName[0])
        s->GetStr((char *)"AI_SPAWN", (char *)"oppName", oppName, 0x40);
    if (!oppName[0]) {
        strcpy(oppName, oppClass);
        char *c = strstr(oppName, "_");
        if (c && (c = strstr(c + 1, "_")))
            *c = 0;
    }
    if (!oppName[0] && !oppClass[0]) {
        apLogErr((char *)"Incorrect AI spawning params for domain %s", obj->name);
        return 0;
    }
    if (!gsDOMAIN::Init())
        return 0;
    domFlags |= 0x100;
    nSpawn = -1;
    s->GetInt((char *)"AI_SPAWN", (char *)"nSpawn", &nSpawn);
    nSpawnFirst = 1;
    s->GetInt((char *)"AI_SPAWN", (char *)"nSpawnFirst", &nSpawnFirst);
    nKillComplete = nSpawn - 1 > 1 ? nSpawn - 1 : 1;
    s->GetInt((char *)"AI_SPAWN", (char *)"nKillComplete", &nKillComplete);
    nKilled = 0;
    timeOppSpawn = 0.5f;
    s->GetFloat((char *)"AI_SPAWN", (char *)"timeOppSpawn", &timeOppSpawn);
    timeAfterKill = strstr(oppClass, "ai_skel") ? 0.2f : 0.5f;
    s->GetFloat((char *)"AI_SPAWN", (char *)"timeAfterKill", &timeAfterKill);
    killTimer = 0;
    nSpawned = 0;
    nMaxSimult = -1;
    s->GetInt((char *)"AI_SPAWN", (char *)"nMaxSimult", &nMaxSimult);
    nMaxSimultSame = -1;
    s->GetInt((char *)"AI_SPAWN", (char *)"nMaxSimultSame", &nMaxSimultSame);
    if (nMaxSimult >= 3) {
        nMaxSimult = (int)(nMaxSimult * aiSpawnOppCoeff);
        if (nMaxSimult < 3)
            nMaxSimult = 3;
    }
    if (nMaxSimultSame >= 3) {
        nMaxSimultSame = (int)(nMaxSimultSame * aiSpawnOppCoeff);
        if (nMaxSimultSame < 3)
            nMaxSimultSame = 3;
    }
    int isExplicit = 0;
    s->GetInt((char *)"AI_SPAWN", (char *)"isExplicit", &isExplicit);
    doneTimer = 0;
    strcpy(apBuffer, "distant");
    s->GetStr((char *)"AI_SPAWN", (char *)"distrib", apBuffer, 0x1000);
    if (!strcmp(apBuffer, "distant"))
        domFlags |= 0x1000;
    else if (!strcmp(apBuffer, "even"))
        domFlags |= 0x800;
    else if (!strcmp(apBuffer, "formation"))
        domFlags |= 0x2000;
    int merge = isExplicit == 0;
    s->GetBool((char *)"AI_SPAWN", (char *)"areaMerge", &merge);
    timer = 2000000.0f;
    if (!nSub) {
        scnDOMAIN *a = list->AddSubDomain(this, obj);
        if (!a || !subArea(a))
            return 0;
        if (!AddSubDomain(a))
            return 0;
    }
    if (merge) {
        domFlags |= 0x4000;
        FillMergedPointsList();
    } else if (isExplicit) {
        domFlags |= 0xC000;
        FillExplicitMergedPointsList();
        if (nMaxSimultSame == -1)
            nMaxSimultSame = nMerged;
    }
    domFlags |= 0x200;
    // aiaDOMAIN_LIST_ARENA::AssignArena (v12 0x00553500) is not rebuilt: no arena gets this domain.
    gsSysServer->DeclSyncable(this);
    return 1;
}

// v12 0x005493E0  (feb 0x00539B10)
void aiDOMAIN_SPAWN::Update(m3dV *)
{
    if (gsSysMP->mode == 3)
        return;
    timer += gsElapsedTime;
    killTimer += gsElapsedTime;
    if (nLocks <= 0 && !(domFlags & 0x400) && nSpawned >= nSpawn) {
        doneTimer += gsElapsedTime;
        if (doneTimer > 10.0f) {
            doneTimer = 0;
            aiNPC *n;
            for (n = aiNPC::GetNext(0, 1); n; n = aiNPC::GetNext(n, 1))
                if (n->spawnDomain == this)
                    break;
            if (!n && !(domFlags & 0x400)) {
                pendingEvents |= 0x20;
                domFlags |= 0x400;
                apLog((char *)"domSpawn \"%s\": COMPLETED", obj->name);
            }
        }
    }
    if ((domFlags & 1) && (domFlags & 0x200) && IsReadySpawn()) {
        int n = nSpawned ? 1 : nSpawnFirst;
        while (n-- > 0)
            SpawnSingle();
    }
}

// v12 0x00549520  (feb 0x00539B90)
BOOL32 aiDOMAIN_SPAWN::IsReadySpawn()
{
    if (!(domFlags & 1) || nLocks > 0 || (domFlags & 0x400))
        return 0;
    if (timer < timeOppSpawn)
        return 0;
    if (nSpawn > 0 && nSpawned >= nSpawn)
        return 0;
    if (timeAfterKill - 1e-6f > killTimer)
        return 0;
    if (nMaxSimult > 0 && aiNPC::GetNOpp(0, oppName) >= nMaxSimult)
        return 0;
    if (nMaxSimultSame > 0 && aiNPC::GetNOpp(oppClass[0] ? oppClass : oppName, 0) >= nMaxSimultSame)
        return 0;
    int i;
    for (i = 0; i < nSub; i++)
        if (subArea(sub[i])->nLocks <= 0)
            break;
    return i != nSub;
}

// v12 0x00549640 (name ours)
BOOL32 aiDOMAIN_SPAWN::IsSpawning()
{
    if (!(domFlags & 1) || nLocks > 0 || (domFlags & 0x400))
        return 0;
    return !(nSpawn > 0 && nSpawned >= nSpawn);
}

// v12 0x00549680 (name ours) the debug key's stop
void aiDOMAIN_SPAWN::StopSpawning()
{
    domFlags |= 0x400;
}

// v12 0x005496D0  (feb 0x00539CB0)
void aiDOMAIN_SPAWN::Spawn()
{
    int n = nSpawned ? 1 : nSpawnFirst;
    while (n-- > 0)
        SpawnSingle();
}

// v12 0x00549700  (feb 0x00539CE0)
void aiDOMAIN_SPAWN::SpawnSingle()
{
    animCREATE_DATA cd;
    aiINIT_DATA init;
    aiSpawnEver = 1;
    m3dV pos, d;
    int area = GetSpawnPosDir(&pos, &d);
    init.spawnDomain = this;
    init.spawnParam = area;
    m3dMATR m;
    m.MakeLCS2WCS_VZ(&pos, &d);
    cd.matr = m;
    entENTITY *e = entCreate(gsScenePtr, oppClass, oppName, &cd, &init, 0);
    if (!e)
        return;
    if (e->classId == 0x244E5043) {
        ((aiNPC *)e)->DecideEnemy();
        wrEVCall<void>(e, 0x178, 0);
    }
    if (!nSpawned)
        pendingEvents |= 1;
    timer = 0;
    nSpawned++;
    pendingEvents |= 4;
    if (nSpawn > 0 && nSpawned >= nSpawn)
        pendingEvents |= 2;
    subArea(sub[area])->pendingEvents |= 4;
    apLog((char *)"domSpawn \"%s\": Spawned: %d", obj->name, nSpawned);
}

// v12 0x00549860  (feb 0x00539E40)
// The place and the facing (to the closest player) of the next one. Returns the index of its area.
int aiDOMAIN_SPAWN::GetSpawnPosDir(m3dV *pos, m3dV *d)
{
    m3dV c, pl;
    if (domFlags & 0x4000) {
        if (domFlags & 0x8000) {
            GetFromMergedListSpawnPosDir(pos, d);
            return 0;
        }
        GetFromMergedListSpawnPosDir(pos, 0);
        ((m3dBOX *)&bbox)->m3dBOX::GetCenter(&c);
        aiGetClosestPlayerPos(&c, &pl);
        d->x = pl.x - pos->x;
        d->y = pl.y - pos->y;
        d->z = pl.z - pos->z;
        m3dNormalize(d);
        return 0;
    }
    ((m3dBOX *)&bbox)->m3dBOX::GetCenter(&c);
    aiGetClosestPlayerPos(&c, &pl);
    int i = 0;
    if (domFlags & 0x1000) {
        for (i = 0; i < nSub; i++)
            if (subArea(sub[i])->nLocks <= 0 && sub[i]->IsBelongPoint(&pl))
                break;
        if (i == nSub) {
            float best = 2000000.0f;
            int found = -1;
            cdtINFO ci = {};
            for (int k = 0; k < nSub; k++)
                if (subArea(sub[k])->nLocks <= 0 &&
                    gsScenePtr->DistPointObj(sub[k]->obj, &pl, best, 1, &ci)) {
                    best = ci.dist;
                    found = k;
                }
            i = found == -1 ? 0 : found;
        }
    } else if (domFlags & 0x800) {
        i = (int)m3dRandRange(0, (float)nSub + 0.5f);
    } else if (domFlags & 0x2000) {
        i = (int)m3dRandRange(0, (float)nSub + 0.5f);
    }
    float hi = (float)(nSub - 1);
    if (hi < 0)
        hi = 0;
    float fi = (float)i;
    if (fi < 0)
        fi = 0;
    int idx = (int)(fi > hi ? hi : fi);
    aiSPAWN_AREA *a = subArea(sub[idx]);
    a->areaFlags &= ~0xFu;
    if (domFlags & 0x1000)
        a->areaFlags |= 2;
    else if (domFlags & 0x800)
        a->areaFlags |= 1;
    else if (domFlags & 0x2000)
        a->areaFlags |= 4;
    aiAREA_GET_POINT_PARAM q;
    q.pos = m3dVUndef;
    q.domain = 0;
    ((m3dBOX *)&a->bbox)->m3dBOX::GetCenter(&c);
    aiGetClosestPlayerPos(&c, &q.pos);
    if (!(a->areaFlags & 3) && (a->areaFlags & 4))
        q.domain = this;
    a->GetRandomPos(pos, &q);
    d->x = q.pos.x - pos->x;
    d->y = 0;
    d->z = q.pos.z - pos->z;
    m3dNormalize(d);
    return idx;
}

// v12 0x00549C00  (feb 0x0053A1A0)
BOOL32 aiDOMAIN_SPAWN::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 1000:
        for (int i = 0; i < nSub; i++) {
            aiSPAWN_AREA *a = subArea(sub[i]);
            if (a->areaFlags & 0x20)
                for (int k = 0; k < a->nPoint; k++) {
                    m3dV p = a->points[k].pos;
                    rendDrv->RenderCross(gsCameraPtr, &p, 2.0f, 0xFFFF0000);
                }
        }
        break;
    case 0x3F8: {
        apNAME *n = (apNAME *)((u8 *)data + 8);
        if (n->IsName((char *)"SPAWN")) {
            int count = WR_FIELD(int, data, 0x48);
            for (int i = 0; i < count; i++) {
                if (nSpawn > 0 && nSpawned >= nSpawn)
                    break;
                SpawnSingle();
            }
        } else if (!n->IsName((char *)"LOCK")) {
            n->IsName((char *)"UNLOCK");
        }
        break;
    }
    case 0x40E:
        ProcessMP_NOTIFY((gsEVENT *)data);
        break;
    case 0x4CA:
        if (((aiNPC *)from)->spawnDomain == this) {
            pendingEvents |= 0x10;
            nKilled++;
            apLog((char *)"domSpawn \"%s\": nKill: %d; nKillCompl: %d", obj->name, nKilled, nKillComplete);
            if (nKillComplete > 0 && nKilled >= nKillComplete && !(domFlags & 0x400)) {
                pendingEvents |= 0x20;
                domFlags |= 0x400;
                apLog((char *)"domSpawn \"%s\": COMPLETED", obj->name);
            }
            killTimer = 0;
        }
        break;
    }
    return gsDOMAIN::ProcessMsg(id, data, from, ret);
}

// v12 0x0054A3A0  (feb 0x0053A6A0)
// All the points of its areas.
void aiDOMAIN_SPAWN::FillMergedPointsList()
{
    nMerged = 0;
    for (int i = 0; i < nSub; i++)
        nMerged += subArea(sub[i])->nPoint;
    if (!nMerged)
        return;
    merged = (aiAREA_POINT *)operator new(nMerged * sizeof(aiAREA_POINT));
    int n = 0;
    for (int i = 0; i < nSub; i++) {
        aiSPAWN_AREA *a = subArea(sub[i]);
        for (int k = 0; k < a->nPoint; k++)
            merged[n++] = a->points[k];
    }
}

// v12 0x0054A4D0  (feb 0x0053A7D0)
// The places of the level's own NPCs of the class inside its first area: they become this domain's.
void aiDOMAIN_SPAWN::FillExplicitMergedPointsList()
{
    nMerged = 0;
    for (int i = 0; i < nSub; i++)
        for (aiNPC *n = aiNPC::GetNext(0, 1); n; n = aiNPC::GetNext(n, 1)) {
            if (!strstr(oppClass, n->inst->className ? n->inst->className : ""))
                continue;
            m3dV p;
            n->inst->GetPos(&p);
            if (sub[0]->IsBelongPoint(&p))
                nMerged++;
        }
    if (!nMerged) {
        apLogErr((char *)"Error: dom_spawn %s could not collect", obj->name);
        return;
    }
    merged = (aiAREA_POINT *)operator new(nMerged * sizeof(aiAREA_POINT));
    int k = 0;
    for (int i = 0; i < nSub; i++)
        for (aiNPC *n = aiNPC::GetNext(0, 1); n; n = aiNPC::GetNext(n, 1)) {
            if (!strstr(oppClass, n->inst->className ? n->inst->className : ""))
                continue;
            m3dV p, d;
            n->inst->GetPos(&p);
            n->inst->GetDir(&d);
            if (sub[0]->IsBelongPoint(&p)) {
                merged[k].pos = p;
                merged[k].dir = d;
                k++;
                n->spawnDomain = this;
                n->spawnParam = 0;
                nKillComplete++;
            }
        }
}

// v12 0x0054A6F0  (feb 0x0053A9F0; feb's CalcWeightEven 0x0053AAE0 is inside it in 1.2)
void aiDOMAIN_SPAWN::GetFromMergedListSpawnPosDir(m3dV *pos, m3dV *d)
{
    if (domFlags & 0x1000) {
        CalcWeightDistant();
    } else {
        for (int i = 0; i < nMerged; i++)
            merged[i].weight = 0;
        for (aiNPC *n = aiNPC::GetNext(0, 1); n; n = aiNPC::GetNext(n, 1))
            if (n->spawnDomain == this) {
                m3dV p;
                n->inst->GetPos(&p);
                for (int i = 0; i < nMerged; i++) {
                    float dx = p.x - merged[i].pos.x, dz = p.z - merged[i].pos.z;
                    merged[i].weight += 1.0f / (dx * dx + dz * dz + 0.1f);
                }
            }
    }
    if (!merged) {
        sub[0]->GetRandomPoint(pos);
        if (d)
            *d = m3dVUnitZ;
        return;
    }
    float w = merged[0].weight;
    int best = 0;
    for (int i = 1; i < nMerged; i++)
        if (w > merged[i].weight) {
            w = merged[i].weight;
            best = i;
        }
    *pos = merged[best].pos;
    if (d)
        *d = merged[best].dir;
}

// v12 0x0054A8A0  (feb 0x0053AC60)
// The far half from the closest player gets a random weight below the near half's 1.
void aiDOMAIN_SPAWN::CalcWeightDistant()
{
    m3dV c, pl;
    ((m3dBOX *)&bbox)->m3dBOX::GetCenter(&c);
    aiGetClosestPlayerPos(&c, &pl);
    float far_ = 0;
    for (int i = 0; i < nMerged; i++) {
        float dx = merged[i].pos.x - pl.x, dz = merged[i].pos.z - pl.z;
        float dd = m3dSqrt(dx * dx + dz * dz);
        if (dd > far_)
            far_ = dd;
        merged[i].weight = 1.0f;
    }
    far_ *= 0.5f;
    for (int i = 0; i < nMerged; i++) {
        float dx = merged[i].pos.x - pl.x, dz = merged[i].pos.z - pl.z;
        if (m3dSqrt(dx * dx + dz * dz) > far_)
            merged[i].weight = m3dRandRange(0, 0.9f);
    }
}

// The sync block  v12 0x009A89CC
struct aiSPAWN_SYNC : msgDATA {
    u32 flags;        // +0x04
    int spawned;      // +0x08
    int killed;       // +0x0C
    int killComplete; // +0x10
};
static aiSPAWN_SYNC aiSpawnSync;

// v12 0x0054AC20  (feb 0x0053AF40)
void aiDOMAIN_SPAWN::ProcessMP_NOTIFY(gsEVENT *e)
{
    if (WR_FIELD(short, e, 4) == -100) {
        aiSPAWN_SYNC *s = WR_FIELD(aiSPAWN_SYNC *, e, 6);
        domFlags = s->flags;
        nKilled = s->killed;
        nSpawned = s->spawned;
        nKillComplete = s->killComplete;
    }
}

// v12 0x0054AC60  (feb 0x0053AF80)
void aiDOMAIN_SPAWN::ProcessMP_GET_SYNC_DATA(void *dv)
{
    msgDATA **pd = (msgDATA **)dv;
    aiSPAWN_SYNC *s = (aiSPAWN_SYNC *)*pd;
    if (!s) {
        s = &aiSpawnSync;
        s->packId = 0;
        s->tag = 0x40;
        s->size = 0x14;
        *pd = s;
    }
    s->killed = nKilled;
    s->killComplete = nKillComplete;
    s->spawned = nSpawned;
    s->flags = domFlags;
    entIACTIVE_OBJ::ProcessMP_GET_SYNC_DATA(dv);
}

// v12 0x0054AD00  (feb 0x0053B140)
void aiDOMAIN_SPAWN::ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    aiSPAWN_SYNC *s = (aiSPAWN_SYNC *)d->data;
    if (!s) {
        s = &aiSpawnSync;
        memset(s, 0, sizeof(*s));
        s->tag = 0x40;
        s->size = 0x14;
        d->data = s;
    }
    for (int ok = d->file->ReadChunk(&c); ok && c.id != 1; ok = d->file->ReadChunk(&c)) {
        void *dst = 0;
        if (c.id == 0xF0)
            dst = &s->flags;
        else if (c.id == 0xF1)
            dst = &s->spawned;
        else if (c.id == 0xF2)
            dst = &s->killed;
        else if (c.id == 0xF3)
            dst = &s->killComplete;
        if (dst)
            d->file->ReadData(dst, 4, 4);
    }
    entIACTIVE_OBJ::ProcessMP_READ_SYNC_DATA(d);
}

// ----------------------------------------------------------------------------------------- a spawn area

// v12 0x00549FB0  (feb 0x0053A550)
BOOL32 aiSPAWN_AREA::Init()
{
    if (!aiAREA::Init())
        return 0;
    name.SetName(obj->name);
    ParseIActions(obj->props);
    return 1;
}

static BOOL32 isNotGroundBound(aiDOMAIN_SPAWN *d)
{
    return strstr(d->oppClass, "ai_bird") || strstr(d->oppClass, "ai_amur") || strstr(d->oppClass, "ai_croc");
}

// v12 0x00549FF0 (name ours): a wall within reach of the place, in any of the 6 directions
static BOOL32 aiIsWallNear(m3dV *p, float len, cdtREFINE *ref)
{
    static float dirs[6][3] = { { 0, -1, 0 }, { 0, 0, -1 }, { 1, 0, 0 }, { 0, 0, 1 }, { -1, 0, 0 }, { 0, 1, 0 } };
    for (int i = 0; i < 6; i++) {
        cdtINFO ci = {};
        if (gsScenePtr->ShootRay(p, (m3dV *)dirs[i], len, 1, ref, &ci) && (ci.flags & 1))
            return 1;
    }
    return 0;
}

// v12 0x0054A0E0  (feb 0x0053A590)
// Not above an object marked 2 (a ground-bound class); a rat fits in 0.3, with no wall by half as much again.
BOOL32 aiSPAWN_AREA::IsValidPoint(m3dV *p, float r)
{
    cdtREFINE ref(2, 0);
    cdtINFO ci = {};
    m3dV to;
    gsScenePtr->FloorPlace(p, 30.0f, &to, 1.0f, 1.0f, 0, &ref, &ci);
    if (ci.flags & 2) {
        objOBJ *o = gsScenePtr->GetCDTObj(&ci);
        if (o && (WR_FIELD(u8, o, 0xC) & 2) && domain && !isNotGroundBound(domain))
            return 0;
    }
    if (domain && strstr(domain->oppClass, "ai_rat"))
        r = 0.3f;
    if (!aiAREA::IsValidPoint(p, r))
        return 0;
    return !aiIsWallNear(p, r * 1.5f, &ref);
}

// v12 0x0054A260
// On the floor, half a metre up (a ground-bound class), pushed off the walls at its height.
void aiSPAWN_AREA::ConstrainPoint(m3dV *p, float r)
{
    cdtREFINE ref(2, 0);
    cdtINFO ci = {};
    if (domain && strstr(domain->oppClass, "ai_rat"))
        r = 0.4f;
    if (domain && !isNotGroundBound(domain)) {
        gsScenePtr->FloorPlace(p, 30.0f, p, 1.0f, 1.0f, 0, &ref, &ci);
        p->y += 0.5f;
    }
    float y = p->y;
    aiAREA::ConstrainPoint(p, r);
    p->y = y;
}
