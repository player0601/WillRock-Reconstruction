// dom_misc.cpp -- the action, cleanup, teleport and kill domains (Will Rock 1.2, v12.exe; feb obj gs_shared:dom_misc.obj)
//
// v12 0x0055C130..0x0055CF90. Four domain lists made at start-up (domMiscInit), each collecting the level objects
// named "&dom_act" / "&dom_clean" / "&dom_tpt" / "&dom_kill" (gsDOMAIN_LIST does the collecting and the per-frame
// point test; these give the reactions).
// 1.2 changes, from the disassembly: the action domain's IAction ACTIVATE / DEACTIVATE also run Enter / Leave and an
// UNLOCK runs Enter again; the teleport domain changes the level on an IAction ACTIVATE too (flag 0x10000), its message
// is 0x4CB (feb 0x4C9) and the in-level teleport notify 0x40E (feb 0x40C); the kill damage is the 1.2 14-byte record
// (type 0, subtype 1, 5000) on 0x4C6 (feb {type 4, 10000} on 0x4C5).
// Not rebuilt: the action domain's MP sync data (v12 0x0055C4D0 / 0x0055C5C0).

#include "game.h"
#include "../gs/gs_mp_server.h"

domCLN_DOMAIN_LIST  *domClnList;     // v12 0x009AE7A4
domACT_DOMAIN_LIST  *domActList;     // v12 0x009AE7B8
domTPT_DOMAIN_LIST  *domTptList;     // v12 0x009AE7BC
domKILL_DOMAIN_LIST *domKillList;    // v12 0x009AE924

#define DOM_IN        0x00001        // a player is inside (gsDOMAIN_LIST::Update)
#define DOM_ACTIVE    0x00100        // action: active; kill: no player
#define DOM_ONCE      0x00200
#define DOM_FIRED     0x00400        // action: fired; teleport: the anchor is known
#define DOM_LEVEL     0x00800        // teleport: a level change
#define DOM_COMPLETE  0x02000        // teleport: the level is complete
#define DOM_DELAYED   0x08000
#define DOM_ACTIVATED 0x10000        // teleport (1.2): ACTIVATE'd

// v12 0x0055C130  (feb 0x00544B60)
int __fastcall domMiscInit()
{
    domTptList = new domTPT_DOMAIN_LIST;
    if (!domTptList)
        return 0;
    msgSystem.InsertAddr(domTptList, 0x56, 0);
    domActList = new domACT_DOMAIN_LIST;
    if (!domActList)
        return 0;
    msgSystem.InsertAddr(domActList, 0x56, 0);
    domClnList = new domCLN_DOMAIN_LIST;
    if (!domClnList)
        return 0;
    msgSystem.InsertAddr(domClnList, 0x56, 0);
    domKillList = new domKILL_DOMAIN_LIST;
    if (!domKillList)
        return 0;
    msgSystem.InsertAddr(domKillList, 0x56, 0);
    return 1;
}

// v12 0x0055C240  (feb 0x00544C70)
void __fastcall domMiscTerm()
{
    msgSystem.DestroyAddr(domTptList);
    domTptList = 0;
    msgSystem.DestroyAddr(domActList);
    domActList = 0;
    msgSystem.DestroyAddr(domClnList);
    domClnList = 0;
    msgSystem.DestroyAddr(domKillList);
    domKillList = 0;
}

// v12 0x0055C2A0 / 0x0055C6C0 / 0x0055C780 / 0x0055CB80
int domACT_DOMAIN_LIST::IsAddObj(objOBJ *o) { return strstr(o->name, "&dom_act") != 0; }
int domCLN_DOMAIN_LIST::IsAddObj(objOBJ *o) { return strstr(o->name, "&dom_clean") != 0; }
int domTPT_DOMAIN_LIST::IsAddObj(objOBJ *o) { return strstr(o->name, "&dom_tpt") != 0; }
int domKILL_DOMAIN_LIST::IsAddObj(objOBJ *o) { return strstr(o->name, "&dom_kill") != 0; }

// v12 0x0055CE90 / 0x0055CED0 / 0x0055CF10 / 0x0055CF50
gsDOMAIN *domACT_DOMAIN_LIST::MakeDomain() { return new domACT_DOMAIN; }
gsDOMAIN *domCLN_DOMAIN_LIST::MakeDomain() { return new domCLN_DOMAIN; }
gsDOMAIN *domKILL_DOMAIN_LIST::MakeDomain() { return new domKILL_DOMAIN; }
gsDOMAIN *domTPT_DOMAIN_LIST::MakeDomain() { return new domTPT_DOMAIN; }

// ---------------------------------------------------------------------------------------------------------- action

// v12 0x0055C2C0  (feb 0x00544CF0)
BOOL32 domACT_DOMAIN::Init()
{
    if (!gsDOMAIN::Init())
        return 0;
    gsSysServer->DeclSyncable(this);
    return 1;
}

// v12 0x0055C2F0  (feb 0x00544D20)
// Inside, active, not locked, and not an act-once that has fired: its ON_ACTIVATE IActions (event 1).
void domACT_DOMAIN::Enter()
{
    u32 f = domFlags;
    if ((f & DOM_IN) && (!(f & DOM_ONCE) || !(f & DOM_FIRED)) && nLocks <= 0 && (f & DOM_ACTIVE)) {
        pendingEvents |= 1;
        domFlags |= DOM_FIRED;
    }
}

// v12 0x0055C330  (feb 0x00544D60)
// The same for leaving: ON_DEACTIVATE (event 2).
void domACT_DOMAIN::Leave()
{
    u32 f = domFlags;
    if ((!(f & DOM_ONCE) || !(f & DOM_FIRED)) && nLocks <= 0 && (f & DOM_ACTIVE)) {
        pendingEvents |= 2;
        domFlags |= DOM_FIRED;
    }
}

// v12 0x0055C370
void domACT_DOMAIN::ProcessUNLOCK()
{
    Enter();
}

// v12 0x0055C380  (feb 0x00544DB0)
// INIT_LEVEL: [DOM_ACTION] isActive / isActOnce (both 1 by default); IActions ACTIVATE / DEACTIVATE; the MP notify.
BOOL32 domACT_DOMAIN::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x3F4) {
        int active = 1, once = 1;
        psSHEET *ps = obj->props;
        if (ps) {
            ps->GetBool((char *)"DOM_ACTION", (char *)"isActive", &active);
            ps->GetBool((char *)"DOM_ACTION", (char *)"isActOnce", &once);
            if (active)
                domFlags |= DOM_ACTIVE;
        } else {
            domFlags |= DOM_ACTIVE;
        }
        if (once)
            domFlags |= DOM_ONCE;
    } else if (id == 0x3F8) {
        apNAME *a = (apNAME *)((u8 *)data + 8);
        if (a->IsName((char *)"ACTIVATE")) {
            domFlags |= DOM_ACTIVE;
            Enter();
        } else if (a->IsName((char *)"DEACTIVATE")) {
            domFlags &= ~DOM_ACTIVE;
            Leave();
        }
    } else if (id == 0x40E) {
        ProcessMP_NOTIFY((msgDATA *)data);
    }
    return gsDOMAIN::ProcessMsg(id, data, from, ret);
}

// v12 0x0055C4B0  (feb 0x00544EE0)
void domACT_DOMAIN::ProcessMP_NOTIFY(msgDATA *ev)
{
    u8 *d = (u8 *)ev;
    if (*(int *)(d + 4) == -100)
        domFlags = *(u32 *)(*(u8 **)(d + 8) + 4);
}

// ---------------------------------------------------------------------------------------------------------- cleanup

// v12 0x0055C6E0  (feb 0x00545090)
// IAction CLEAN: every NPC ('$NPC') and item ('$ITM') is tested against the volume (both builds use no result).
BOOL32 domCLN_DOMAIN::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x3F8 && ((apNAME *)((u8 *)data + 8))->IsName((char *)"CLEAN"))
        for (entENTITY *e = entFindNext(0); e; e = entFindNext(e))
            if (e->classId == 0x244E5043 || e->classId == 0x2449544D) {
                m3dV p;
                e->inst->GetPos(&p);
                IsBelongPoint(&p);
            }
    return gsDOMAIN::ProcessMsg(id, data, from, ret);
}

// ---------------------------------------------------------------------------------------------------------- kill

// v12 0x0055CBA0  (feb 0x00545500)
BOOL32 domKILL_DOMAIN::Init()
{
    if (!gsDOMAIN::Init())
        return 0;
    float h;
    if (!obj->props || !obj->props->GetFloat((char *)"DOMAIN", (char *)"height", &h))
        SetHeight(1.0f);
    int v;
    if (obj->props && obj->props->GetBool((char *)"DOM_KILL", (char *)"isNoPlr", &v) && v)
        domFlags |= DOM_ACTIVE;                         // (here: the players are spared)
    return 1;
}

#pragma pack(push, 1)
static struct { u8 id, flags; u16 size; u8 type, sub; float amount; u32 src; } domKillDmg = { 0, 0x40, 0xE, 0, 0, 0, 0 };
#pragma pack(pop)

// v12 0x0055CC30  (feb 0x00545590)
// Every frame on the server, unlocked: the NPCs in the box whose point is inside (not those with +0x46F 0x200000)
// and, unless isNoPlr, the players inside get the damage message 0x4C6 (subtype 1, 5000).
BOOL32 domKILL_DOMAIN::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    cdtREFINE ref;
    ref.mode = 0;
    ref.queryFlags = 0;
    ref.exclude = 0;
    if (id == 1000 && (entFlags & 4) && nLocks <= 0) {
        scnAOBJ list[100];
        int n = gsScenePtr->MakeAnimList((m3dBOX *)&bbox, &ref, list, 100);
        for (int i = 0; i < n; i++) {
            entENTITY *e = list[i].inst->ent;
            if (e->classId != 0x244E5043 || (*(u32 *)((u8 *)e + 0x46F) & 0x200000))
                continue;
            m3dV p;
            e->inst->GetPos(&p);
            if (IsBelongPoint(&p)) {
                domKillDmg.sub = 1;
                domKillDmg.amount = 5000.0f;
                msgSystem.SendMsg(e, 0x4C6, &domKillDmg, 0);
            }
        }
        if (!(domFlags & DOM_ACTIVE))
            for (entENTITY *e = gsSysServer->GetNextPlayer(0); e; e = gsSysServer->GetNextPlayer(e)) {
                m3dV p;
                e->inst->GetPos(&p);
                if (IsBelongPoint(&p)) {
                    domKillDmg.sub = 1;
                    domKillDmg.amount = 5000.0f;
                    msgSystem.SendMsg(e, 0x4C6, &domKillDmg, 0);
                }
            }
    }
    return gsDOMAIN::ProcessMsg(id, data, from, ret);
}

// ---------------------------------------------------------------------------------------------------------- teleport

// v12 0x0055C7A0  (feb 0x00545150)
// [TELEPORT] nmbLocks, anchor, sceneName (a level change), else the anchor instance's matrix (teleport inside the
// level); isLevelComplete, isDelayed.
BOOL32 domTPT_DOMAIN::Init()
{
    if (!gsDOMAIN::Init())
        return 0;
    psSHEET *ps = obj->props;
    if (!ps || !ps->FindSect((char *)"TELEPORT"))
        return 1;
    ps->GetInt((char *)"TELEPORT", (char *)"nmbLocks", &nLocks);
    ps->GetStr((char *)"TELEPORT", (char *)"anchor", anchor, 0x40);
    if (ps->GetStr((char *)"TELEPORT", (char *)"sceneName", sceneName, 0x40)) {
        domFlags |= DOM_LEVEL;
    } else if (animINST *a = gsScenePtr->FindInstName(anchor)) {
        memcpy(anchorMatr, a->matr, 0x40);
        domFlags |= DOM_FIRED;
    } else {
        apMsg((char *)"Cannot find anchor instance %s", anchor);
    }
    int v;
    if (ps->GetBool((char *)"TELEPORT", (char *)"isLevelComplete", &v) && v)
        domFlags |= DOM_COMPLETE;
    if (ps->GetBool((char *)"TELEPORT", (char *)"isDelayed", &v) && v)
        domFlags |= DOM_DELAYED;
    return 1;
}

// v12 0x0055C8D0
BOOL32 domTPT_DOMAIN::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x3F8 && ((apNAME *)((u8 *)data + 8))->IsName((char *)"ACTIVATE"))
        domFlags |= DOM_ACTIVATED;
    return gsDOMAIN::ProcessMsg(id, data, from, ret);
}

#pragma pack(push, 1)
struct domTPT_MP_MSG {               // the in-level teleport to a client (0x40E), 0x4A bytes
    u8    id, flags;
    u16   size;
    u16   type;                      // 0
    float matr[16];                  // +0x06 where to
    int   delayed;                   // +0x46
};
#pragma pack(pop)
static gsLOAD_SCENE_PARAM domTptLoad = { 0, 0x40, 0x154 };   // v12 0x009AE7C0

// v12 0x0055C920  (feb 0x00545290)
// On the server, unlocked (event 0x10 every frame): a level change once a player is inside or it was ACTIVATE'd --
// GS_MSG_TELEPORT_LEVEL 0x4CB to all with the level, the anchor, the domain as the keep filter (its first sub-domain
// when it has some; the entities inside survive) and "level complete" (not sent while gsAppState 0x80000);
// else each player inside (0.7 above the feet) is sent the anchor's matrix (0x40E).
void domTPT_DOMAIN::Update(m3dV *)
{
    if (!(entFlags & 4) || nLocks > 0)
        return;
    pendingEvents |= 0x10;
    u32 f = domFlags;
    if (f & DOM_LEVEL) {
        if (!(f & DOM_IN) && !(f & DOM_ACTIVATED))
            return;
        strcpy(domTptLoad.level, sceneName);
        strcpy(domTptLoad.anchorInst, anchor);
        if (!(domFlags & DOM_COMPLETE)) {
            domTptLoad.keepEntities = 0;
            domTptLoad.keepFilter = nSub > 0 ? (void *)sub[0] : (void *)(scnDOMAIN *)this;
            domTptLoad.resetLevelTime = 0;
        } else {
            domTptLoad.keepEntities = 0;
            domTptLoad.resetLevelTime = 1;
            if (gsAppState & 0x80000)
                return;
        }
        msgSystem.SendAll(0x4CB, &domTptLoad, 0);
        return;
    }
    if (!(f & DOM_FIRED))
        return;
    for (entENTITY *pl = gsSysServer->GetNextPlayer(0); pl; pl = gsSysServer->GetNextPlayer(pl)) {
        m3dV p;
        pl->inst->GetPos(&p);
        p.y += 0.7f;
        if (!IsBelongPoint(&p))
            continue;
        domTPT_MP_MSG m;
        m.id = 0;
        m.flags = 0x40;
        m.size = 0x4A;
        m.type = 0;
        memcpy(m.matr, anchorMatr, 0x40);
        m.delayed = (domFlags & DOM_DELAYED) ? 1 : 0;
        gsSysServer->SendMsgClient(pl, 0x40E, (msgDATA *)&m, 0x20000);
    }
}
