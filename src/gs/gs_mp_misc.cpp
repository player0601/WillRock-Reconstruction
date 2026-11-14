// gs_mp_misc.cpp -- Will Rock 1.2 (v12.exe) : gsMP_SYSTEM and message packs
//
// Reconstructed from v12 disassembly against the Feb 2003 build (gs:gs_mp_misc.obj).
// Documentation for modders; not meant to compile into the game.
// "// 1.2:" = changed or new in 1.2 versus feb.  "// ?" = unknown / unverified.
// v12 range of this object: 0x0050D650 .. 0x0050F13F (feb 0x004FA8B0 .. 0x004FC41F).

#include "gs_mp_misc.h"
// [integrator: all MP headers, so cross-object types are complete for the syntax check]
#include "network.h"
#include "network_dpubi.h"
#include "network_dpubi_dp.h"
#include "mp_messages.h"
#include "gs_mp_conn.h"
#include "gs_mp_network.h"
#include "gs_mp_misc.h"
#include "gs_mp_client.h"
#include "gs_mp_server.h"
#include "gs_play.h"
#include "wr_go_mp.h"
#include "wr_ui_mult.h"
#include "wr_ui_server.h"
#include "wr_ui_join.h"

gsMP_SYSTEM            *gsSysMP;          // v12 0x0098E850
gsMSG_MP_NOTIFY_EV_SYNC gsMsgEvSyncPack;  // v12 0x0098E854 (id byte 0x0098E858)
gsMSG_MP_PTR_PACK       gsMsgMpPtrPack;   // v12 0x0098E864 (id byte 0x0098E868)
gsMSG_MP_JOIN_PACK      gsMsgJoinPack;    // v12 0x0098E9E0 (id byte 0x0098E9E4)  1.2: new
float                   gsMPEpsilonSq;    // v12 0x0098E86C  1.2: new

static const float MP_EPSILON      = 1e-6f;       // v12 0x00607A74
static const float MP_FAR_DISTANCE = 2000000.0f;  // v12 0x00607A78 (feb 0x005DC5C8 was the same role)


// v12 0x0050DD20  (feb none)
// 1.2: static initializer (registered in the CRT init table at v12 0x0062A4B8).
static void _$E_gsMPEpsilonSq()
{
    gsMPEpsilonSq = MP_EPSILON * MP_EPSILON;
}


// v12 0x0050D650  (feb 0x004FA910)
// Creates gsSysNetwork and gsSysMP and registers them and the server/client with the msg system.
int __fastcall gsMPInit()
{
    gsSysNetwork = new gsMP_NETWORK;                 // size 0xE0, ctor v12 0x0052EC20
    gsSysNetwork->name     = '$NET';                 // +0x14 = 0x244E4554
    gsSysNetwork->priority = 0x12;                   // +0x1C
    msgSystem.InsertAddr(gsSysNetwork, '$SYS', 0);   // 0x24535953

    gsMP_SYSTEM *mp = (gsMP_SYSTEM *)operator new(0x664);
    if (mp) {
        // msgADDR part
        mp->field04 = mp->field08 = mp->field0C = 0;
        mp->magic   = 'MADR';                        // +0x10 = 0x4D414452
        mp->field14 = mp->field18 = mp->field1C = mp->field20 = mp->field24 = 0;
        mp->unk644  = 0;
        new (&mp->stat) gsMP_STAT;                   // v12 0x0050F120
        // vtable = gsMP_SYSTEM (v12 0x00607A94)
        mp->mode        = 0;
        mp->modeName[0] = "Single";                  // v12 0x006354AC
        mp->modeName[1] = "Server";                  // v12 0x006354A4
        mp->modeName[2] = "Client";                  // v12 0x0063549C
        mp->modeName[3] = "ServerOnly";              // v12 0x00635490
    }
    gsSysMP = mp;
    gsSysMP->name     = '$PM$';                      // 0x244D5024
    gsSysMP->priority = 0x14;
    msgSystem.InsertAddr(gsSysMP, '$SYS', 0);

    gsSysServer->name     = 'MPSR';                  // 0x4D505352
    gsSysServer->priority = 0x19;
    msgSystem.InsertAddr(gsSysServer, '$SYS', 0);

    gsSysClient->name     = 'MPCL';                  // 0x4D50434C
    gsSysClient->priority = 0x19;
    msgSystem.InsertAddr(gsSysClient, '$SYS', 0);
    return 1;
}


// v12 0x0050D7B0  (feb 0x004FAA70)
// Unregisters gsSysMP and gsSysNetwork.
void __fastcall gsMPTerm()
{
    msgSystem.DestroyAddr(gsSysMP);
    msgSystem.DestroyAddr(gsSysNetwork);
}


// v12 0x0050D7E0  (feb 0x004FAAA0)
// Scalar deleting destructor: frees the stat buffer and destroys the msgADDR base.
void *gsMP_SYSTEM::__vecDelDtor(unsigned flags)
{
    apFree(*(void **)((char *)this + 0x658));        // gsMP_STAT buffer (stat + 0x10)
    msgADDR::~msgADDR();
    if (flags & 1)
        operator delete(this);
    return this;
}


// v12 0x0050D820  (feb 0x004FAAE0)
// Registers the MP message packs and calls Init on the server and client objects.
int gsMP_SYSTEM::Init()
{
    msgSystem.RegisterMsgData(&gsMsgMpPtrPack);      // v12 0x0098E864 (feb 0x00948268)
    msgSystem.RegisterMsgData(&gsMsgJoinPack);       // 1.2: v12 0x0098E9E0, replaces feb's
                                                     //      UPD_POS pack (feb 0x00948278) here
    msgSystem.RegisterMsgData(&gsMsgEvSyncPack);     // v12 0x0098E854 (feb 0x00948254)
    mpRegisterUpdPosPack();                          // 1.2: v12 0x00514990, registers 0x0098EC28
                                                     //      (position-update pack, new module) ?
    gsSysServer->Init();                             // vt +0x10
    gsSysClient->Init();                             // vt +0x10
    return 1;
}


// v12 0x0050D870  (feb 0x004FAB30)
// Closes the UI and terminates server, client and demo.
void gsMP_SYSTEM::Term()
{
    if (uiSystem) {
        uiSystem->vfunc40();                         // 1.2: slot +0x40 (feb +0x3C)  ?
        uiSystem->vfunc38();                         // 1.2: slot +0x38 (feb +0x34)  ?
    }
    gsSysServer->Term();                             // vt +0x14
    gsSysClient->Term();                             // vt +0x14
    gsDemo.Term();
}


// v12 0x0050D8B0  (feb 0x004FAB70)
// Sets the MP mode and initialises the server and client halves with mode flags.
// Server/Client InitGame (vt +0x18) take a flags word:
//   1 = single, 2 = networked server, 0x20 = no server (pure client),
//   0x10 = remote client, +4 = demo, 0x200>>8 trick below gives 2|((state&0x400)>>8).
int gsMP_SYSTEM::InitGame(int newMode)
{
    mode = newMode;
    gsDemo.Init();

    switch (mode) {                                  // jump table v12 0x0050D998
    case gsMP_MODE_SINGLE:
        gsSysServer->InitGame(1);
        gsSysClient->InitGame(1);
        break;

    case gsMP_MODE_SERVER:
        if (!gsSysServer->InitGame(2))
            return 0;
        // client flags = 2 | 4 when demo-recording (gsAppState 0x400 >> 8)
        if (!gsSysClient->InitGame(((gsAppState & GS_APP_DEMO_RECORD) | 0x200) >> 8))
            return 0;
        break;

    case gsMP_MODE_SERVER_ONLY:                      // 1.2: explicit case for dedicated;
        if (!gsSysServer->InitGame(0))               //      feb fell through with nothing
            return 0;                                //      initialised for mode 4
        if (!gsSysClient->InitGame(2))
            return 0;
        break;

    case gsMP_MODE_CLIENT:
        if (!gsSysServer->InitGame(0x20))
            return 0;
        if (!gsSysClient->InitGame((gsAppState & GS_APP_DEMO_PLAY) ? 0x14 : 0x10))
            return 0;
        break;
    }
    return 1;
}


// v12 0x0050D9B0  (feb 0x004FAC30)
// Terminates the game on server and client.
void gsMP_SYSTEM::TermGame()
{
    gsSysServer->TermGame();                         // vt +0x1C
    gsSysClient->TermGame();                         // vt +0x1C (tail jump)
}


// v12 0x0050E8C0  (feb none; feb built it inline in JoinGame)
// 1.2: constructor of the 0x164-byte join message.
gsMSG_MP_JOIN_PLAYER::gsMSG_MP_JOIN_PLAYER()
{
    packId = 0; flags = 0x40; size = 8;
    next   = NULL;
    packId = gsMsgMpPtrPack.id;                      // byte 0x0098E868 (inlined base ctor)  ?
    mpid   = -1;
    unk0CC = -1;
    new (&create) animCREATE_DATA;                   // scale 1,1,1; s16 -1; float 0; identity
    name[0] = entName[0] = entClass[0] = 0;
    size   = 0x164;
    packId = gsMsgJoinPack.id;                       // byte 0x0098E9E4
}


// v12 0x0050D9D0  (feb 0x004FAC50)
// Joins the local machine to the game: server/client JoinGame, then asks the server to
// create the local player and announces it.
int gsMP_SYSTEM::JoinGame()
{
    gsMSG_MP_JOIN_PLAYER   join;                     // 1.2: inlined ctor (see 0x0050E8C0)
    gsMSG_MP_PLAYER_ACTIVE act;

    if (mode == gsMP_MODE_SERVER || mode == gsMP_MODE_SERVER_ONLY) {
        if (!gsSysServer->JoinGame(-1))              // vt +0x20; 1.2: takes an arg (-1) ?
            return 0;
    } else if (mode == gsMP_MODE_SINGLE || mode == gsMP_MODE_CLIENT) {
        gsSysClient->JoinGame(0);                    // vt +0x20; 1.2: arg 0 (feb: no arg);
    }                                                // 1.2: mode 0 now calls neither

    if (gsAppState & GS_APP_DEMO_PLAY) {
        gsDemo.Play_Start(NULL);
        return 1;
    }

    if (mode != gsMP_MODE_SERVER_ONLY) {             // 1.2: dedicated host has no local player
        strcpy(join.name, "");
        _apCfgReadString(NULL, "Multiplayer", "PlayerName", join.name, 64);
        strcpy(join.entName,  "player");             // v12 0x0062B404
        strcpy(join.entClass, "player");
        // vt +0x2C = SendMsgServer(ent, msg, data, flags) ?
        gsSysClient->SendMsgServer(NULL, GS_MSG_MP_JOIN_PLAYER /*0x402*/, &join, 0x20000);
    }

    if (mode == gsMP_MODE_SERVER || mode == gsMP_MODE_SINGLE) {
        act.packId = 0; act.flags = 0x40; act.size = 0x0C;
        act.mpid   = gsSysServer->GetPlayer(0)->mpid;   // entity +0x88 (feb +0x8C)
        act.active = 1;
        gsSysClient->SendMsgServer(NULL, GS_MSG_MP_PLAYER_ACTIVE /*0x406*/, &act, 0x20000);
        msgSystem.SendAll(GS_MSG_MP_PLAYER_ENTER /*0x408*/, gsSysServer->GetPlayer(0), NULL);
    }
    else if (mode == gsMP_MODE_SERVER_ONLY) {        // 1.2: new branch
        act.packId = 0; act.flags = 0x40; act.size = 0x0C;
        act.mpid   = -1;
        act.active = 1;
        gsSysClient->SendMsgServer(NULL, GS_MSG_MP_PLAYER_ACTIVE, &act, 0x20000);
    }
    else if (mode == gsMP_MODE_CLIENT) {
        gsAppState |= GS_APP_CLIENT_WAITLVL;         // 0x400000
    }

    if (gsAppState & GS_APP_DEMO_RECORD)
        gsDemo.Record_Start();                       // v12 0x0053F1E0
    return 1;
}


// v12 0x0050DC00  (feb 0x004FAE20)
// Leaves the game: deactivates the host player and sends "player leave" for every player.
void gsMP_SYSTEM::LeaveGame()
{
    if (mode == gsMP_MODE_SERVER || mode == gsMP_MODE_SINGLE) {
        gsMSG_MP_PLAYER_ACTIVE act;
        act.packId = 0; act.flags = 0x40; act.size = 0x0C;
        act.mpid   = gsSysServer->GetPlayer(0)->mpid;   // +0x88 (feb +0x8C)
        act.active = 0;
        gsSysClient->SendMsgServer(NULL, GS_MSG_MP_PLAYER_ACTIVE /*0x406, feb 0x405*/, &act, 0x20000);
    }
    for (entENTITY *p = NULL;;) {
        p = (mode == gsMP_MODE_CLIENT) ? gsSysClient->GetNextPlayer(p)
                                       : gsSysServer->GetNextPlayer(p);
        if (!p) break;
        msgSystem.SendAll(GS_MSG_MP_PLAYER_LEAVE /*0x409, feb 0x408*/, p, NULL);
    }
}


// v12 0x0050DCC0  (feb 0x004FAEE0)
// Sets the MP role bits (mask 6) on every active object according to the mode.
void gsMP_SYSTEM::SetupMPStates()
{
    unsigned bits;                                   // (feb reused 'this' uninitialised for
    switch (mode) {                                  //  unknown modes; same in 1.2: arg slot)
    case gsMP_MODE_SINGLE:
    case gsMP_MODE_SERVER:
    case gsMP_MODE_SERVER_ONLY:                      // 1.2: mode 4 added (authoritative)
        bits = 6; break;
    case gsMP_MODE_CLIENT:
        bits = 2; break;
    case gsMP_MODE_NONE:
        bits = 0; break;
    default:
        bits = /* ? stale stack value */ 0; break;
    }
    for (entIACTIVE_OBJ *o = entFindNextIO(NULL); o; o = entFindNextIO(o))
        o->mpFlags = (o->mpFlags & ~6u) | bits;      // 1.2: +0x84 (feb +0x88)
}


// v12 0x0050DD40  (feb 0x004FAF40)
// Dispatches frame / level init / level term messages to the virtuals.
int gsMP_SYSTEM::ProcessMsg(int msg, void *data, msgADDR *from, int *res)
{
    if (msg == GS_MSG_FRAME)            ProcessFRAME();        // slot +0x10
    else if (msg == GS_MSG_INIT_LEVEL)  ProcessINIT_LEVEL();   // slot +0x14
    else if (msg == GS_MSG_TERM_LEVEL)  ProcessTERM_LEVEL();   // slot +0x18
    return 0;
}


// v12 0x0050DD80  (feb 0x004FAFA0)
// Collects the level's start positions ("start_pos", "start_pos_coop", "start_pos*").
// 1.2: afterwards drops every start position onto the floor (FloorPlace).
void gsMP_SYSTEM::ProcessINIT_LEVEL()
{
    nStartPos = 0;
    int n = 0;

    if (mode == gsMP_MODE_SINGLE) {
        animINST *i = gsScenePtr->FindInstName("start_pos");          // v12 0x006354C4
        if (!i) return;
        i->GetPos(&startPos[0]);
        i->GetDir(&startDir[0]);
        nStartPos = 1;
        return;                                      // single player: no floor placement
    }

    animINST *i = gsScenePtr->FindInstName("start_pos_coop");         // v12 0x006354B4
    if (!i) i = gsScenePtr->FindInstName("start_pos");
    if (i) {
        i->GetPos(&startPos[0]);
        i->GetDir(&startDir[0]);
        n = 1;                                       // slot 0 = the main/coop start
    }

    for (i = gsScenePtr->FindNextInst(NULL); i; i = gsScenePtr->FindNextInst(i)) {
        const char *nm = i->name ? i->name : "";     // inst +0x18
        if (strncmp(nm, "start_pos", strlen("start_pos")) != 0) continue;
        if (strcmp(nm, "start_pos") == 0)      continue;   // (inlined repe cmpsb, 10 bytes)
        if (strcmp(nm, "start_pos_coop") == 0) continue;   // (15 bytes)
        i->GetPos(&startPos[n]);
        i->GetDir(&startDir[n]);
        if (++n >= GS_MP_MAX_START_POS) break;       // limit 64 (0x40)
    }
    nStartPos = n;

    // 1.2: new -- snap each start position to the floor below it
    for (int k = 0; k < nStartPos; ++k) {
        cdtREFINE refine;                            // vtable v12 0x005FA0E0, zero-initialised
        m3dV floor = {0, 0, 0};
        gsScenePtr->FloorPlace(&startPos[k], MP_FAR_DISTANCE /*2e6*/, &floor,
                               3.0f, 0, 0, &refine, 0);   // ? argument meanings
        startPos[k] = floor;
    }
}


// v12 0x0050DF90  (feb 0x004FB140)
// Forgets the start positions.
void gsMP_SYSTEM::ProcessTERM_LEVEL()
{
    nStartPos = 0;
}


// v12 0x0050DFA0  (feb 0x004FB150)
// Picks a spawn point and direction for a (re)spawning player.
// 1.2: third argument 'who' (the entity spawning; its anim inst is excluded from the
//      collision test), new single-start-pos branch that scatters players around the
//      one start point, and other players' positions are queried with ENT_MSG_GET_POS
//      (0x415) instead of animINST::GetPos(ent->inst).
void gsMP_SYSTEM::SelectStartPosDir(m3dV *pos, m3dV *dir, entENTITY *who)
{
    if (nStartPos == 0) {
        // no start points: camera position of the scene, facing +X
        gsScenePtr->camera->GetPos(pos);             // (scene+0x44)+0xC0, vt +0x24  ?
        *dir = m3dVUnitX;                            // feb wrote UnitX into *pos (bug);
        return;                                      // 1.2: writes *dir  (verify: v12 stores
    }                                                //  to [esp+0x8C] = dir)

    if (nStartPos == 1) {                            // 1.2: new branch
        *pos = startPos[0];
        *dir = startDir[0];
        int nPl = (mode == gsMP_MODE_CLIENT) ? gsSysClient->GetNPlayer()
                                             : gsSysServer->GetNPlayer();
        if (nPl < 2)
            return;

        // collision query object
        cdtINFO_SPHERE cd;                           // vtable v12 0x005FA0D4, type 2 ?
        cd.ignoreInst = who ? who->inst : NULL;      // ent +0xC0
        cd.pos        = m3dVZero;
        m3dV cand, tang, hit;

        for (int attempt = 0; attempt < 3; ++attempt) {      // tweak: 3 attempts
            m3dMakeVTangRandom(&tang, &m3dVUnitY);           // random horizontal dir
            cand.x = pos->x + tang.x * 4.0f;                 // tweak: scatter radius 4.0
            cand.y = pos->y + tang.y * 4.0f + 1.0f;
            cand.z = pos->z + tang.z * 4.0f;
            pos->y += 1.0f;                                  // lifted 1.0 each attempt
            if (!gsScenePtr->IsVisiblePoint(pos, &cand, 0, (cdtREFINE *)&hit, (cdtINFO *)&cd)) {   // [integrator] SDK signature; argument roles unverified
                // blocked: stop half a unit before the hit point
                float t = cd.hitFraction - 0.5f;             // cd +0x30  ?
                if (t < 0.0f) t = 0.0f;
                cand = *pos + tang * t;
            }
            gsScenePtr->ConstrSphere(&cand, &cand, 0.7f, 16, (cdtREFINE *)&cd, 0);   // [integrator] SDK signature  // push out of walls
                                                             // tweak: radius 0.7, 16 iters
            bool tooClose = false;
            for (entENTITY *p = NULL;
                 (p = (mode == gsMP_MODE_CLIENT) ? gsSysClient->GetNextPlayer(p)
                                                 : gsSysServer->GetNextPlayer(p)); ) {
                m3dV pp;
                msgSystem.SendMsg(p, ENT_MSG_GET_POS /*0x415*/, &pp, NULL);
                if (m3dLength(pp - cand) < 0.4f) {           // tweak: 0.4 min separation
                    tooClose = true;
                    break;
                }
            }
            if (!tooClose) break;
        }
        *pos = cand;                                 // last candidate is used even if crowded
        return;
    }

    // several start points: choose the one farthest from every player
    int best = 0;
    int nPl = (mode == gsMP_MODE_CLIENT) ? gsSysClient->GetNPlayer()
                                         : gsSysServer->GetNPlayer();
    if (nPl >= 2) {
        float bestDist = 0.0f;
        for (int k = 0; k < nStartPos; ++k) {
            float minDist = MP_FAR_DISTANCE;         // 2000000.0
            for (entENTITY *p = NULL;
                 (p = (mode == gsMP_MODE_CLIENT) ? gsSysClient->GetNextPlayer(p)
                                                 : gsSysServer->GetNextPlayer(p)); ) {
                m3dV pp;
                msgSystem.SendMsg(p, ENT_MSG_GET_POS, &pp, NULL);   // 1.2 (feb: inst GetPos)
                float d = m3dSqrt(dist2(pp, startPos[k]));
                if (d < minDist) minDist = d;
            }
            if (minDist > bestDist) { bestDist = minDist; best = k; }
        }
    }
    *pos = startPos[best];
    *dir = startDir[best];
}


// v12 0x0050E4F0  (feb 0x004FB380)
// Returns start position/direction idx; an out-of-range idx picks a random one.
void gsMP_SYSTEM::GetStartPosDir(int idx, m3dV *pos, m3dV *dir)
{
    if (idx < 0 || idx >= nStartPos) {
        idx = m3dRandIntMax(nStartPos);
        if (idx < 0 || idx >= nStartPos)
            idx = 0;
    }
    if (pos) *pos = startPos[idx];
    if (dir) *dir = startDir[idx];
}


// v12 0x0050E570  (feb 0x004FB400)
// Returns the number of start positions.
int gsMP_SYSTEM::GetNStartPos()
{
    return nStartPos;
}


// v12 0x0050E580  (feb 0x004FB410)
// Mode number -> name ("Single".."ServerOnly"), NULL when out of range.
char *gsMP_SYSTEM::Mode2String(int m)
{
    if (m > 0 && m < 5)
        return modeName[m - 1];
    return NULL;
}


// v12 0x0050E5A0  (feb 0x004FB430)
// Mode name -> number (case-insensitive), 0 when unknown.
int gsMP_SYSTEM::String2Mode(char *s)
{
    for (int m = 1; m < 5; ++m)
        if (_strcmpi(s, modeName[m - 1]) == 0)
            return m;
    return 0;
}


// v12 0x0050E5E0  (feb none)
// 1.2: new. [CONFIG] ConnSpeed (0/1/2) -> rate multiplier 1.0 / 2.0 / 4.0.
// Callers: gsMP_SERVER::InitGame v12 0x0051035F (stored at server +0x2CD0) and
//          gsMP_CLIENT init v12 0x005391E9 (stored at client +0x2788).
float gsMP_SYSTEM::ConnSpeedScale(int connSpeed)
{
    switch (connSpeed) {
    case 1:  return 2.0f;          // v12 0x005F9AA4
    case 2:  return 4.0f;          // v12 0x005F9870
    case 0:
    default: return 1.0f;          // v12 0x005F942C
    }
}


// v12 0x0050E610  (feb 0x004FB470)
// Reads [Multiplayer] Mode from the config; defaults to Single (1).
int gsMP_SYSTEM::ReadMode()
{
    char buf[64];
    if (!_apCfgReadString(NULL, "Multiplayer", "Mode", buf, 64))
        strcpy(buf, gsSysMP->modeName[0]);
    for (int m = 1; m < 5; ++m)
        if (_strcmpi(buf, gsSysMP->modeName[m - 1]) == 0)
            return m ? m : 1;
    return 1;
}


// v12 0x0050E690  (feb 0x004FB4F0)
// True for a real MP id (not -1, -2 or -3).
int gsMP_SYSTEM::IsRegularMPID(unsigned long id)
{
    return id != (unsigned long)-1 && id != (unsigned long)-3 && id != (unsigned long)-2;
}


// v12 0x0050E6B0  (feb 0x004FB510, map alias FindEnt)
// Finds the active object with MP id 'id' (optionally requiring mpFlags & flagMask).
entIACTIVE_OBJ *gsMP_SYSTEM::FindMPID(int id, int flagMask)
{
    if (id == -1 || id == -3 || id == -2)
        return NULL;
    for (entIACTIVE_OBJ *o = entFindNextIO(NULL); o; o = entFindNextIO(o)) {
        if (o->mpid != id) continue;                 // 1.2: +0x88 (feb +0x8C)
        if (flagMask == 0 || (o->mpFlags & flagMask))   // 1.2: +0x84 (feb +0x88)
            return o;
    }
    return NULL;
}


// v12 0x0050E700  (feb 0x004FBD50)
// Returns the first entity whose template name is "player" (the local player).
entENTITY *gsMP_SYSTEM::FindPlrLocal()
{
    for (entENTITY *e = entFindNext(NULL); e; e = entFindNext(e)) {
        const char *nm = e->inst->tplName;           // 1.2: inst at +0xC0 (feb +0xBC); +0x1C
        if (!nm) nm = "";
        if (memcmp(nm, "player", 7) == 0)
            return e;
    }
    return NULL;
}


// v12 0x0050E750  (feb 0x004FBDA0)
// Iterates players through the client or the server table depending on mode.
entENTITY *gsMP_SYSTEM::GetNextPlayer(entENTITY *prev)
{
    if (mode == gsMP_MODE_CLIENT)
        return gsSysClient->GetNextPlayer(prev);     // v12 0x00539400
    return gsSysServer->GetNextPlayer(prev);         // v12 0x0050F440
}


// v12 0x0050E770  (feb 0x004FBDC0)
// Save/load helper: writes the MP id of the entity owning 'inst'.
int gsMP_SYSTEM::WriteInst(fioFILE *f, animINST *inst)
{
    f->Write(&inst->owner->mpid, 4, 4);              // inst +0x13C -> ent +0x88 (feb +0x8C)
    return 1;
}


// v12 0x0050E7A0  (feb 0x004FBDF0)
// Save/load helper: reads an MP id and returns that entity's anim inst.
// 1.2: checks IsRegularMPID and compares with the id read (feb decompile shows "== 4").
animINST *gsMP_SYSTEM::ReadInst(fioFILE *f)
{
    int id;
    f->Read(&id, 4, 4);                              // vt +0x14
    if (id == -1 || id == -3 || id == -2)
        return NULL;
    for (entIACTIVE_OBJ *o = entFindNextIO(NULL); o; o = entFindNextIO(o))
        if (o->mpid == id)
            return ((entENTITY *)o)->inst;           // +0xC0 [integrator: cast, inst is an entENTITY field]
    return NULL;
}


// v12 0x0050E800  (feb 0x004FBE50)
// Save/load helper: writes an active object's MP id.
int gsMP_SYSTEM::WriteEIO(fioFILE *f, entIACTIVE_OBJ *o)
{
    f->Write(&o->mpid, 4, 4);                        // +0x88
    return 1;
}


// v12 0x0050E820  (feb 0x004FBE70)
// Save/load helper: reads an MP id and returns that active object.
entIACTIVE_OBJ *gsMP_SYSTEM::ReadEIO(fioFILE *f)
{
    int id;
    f->Read(&id, 4, 4);
    if (id == -1 || id == -3 || id == -2)            // 1.2: inlined FindMPID(id, 0)
        return NULL;
    for (entIACTIVE_OBJ *o = entFindNextIO(NULL); o; o = entFindNextIO(o))
        if (o->mpid == id)
            return o;
    return NULL;
}


// v12 0x0050E870  (feb 0x004FBEC0)
// gsMSG_MP_PTR constructor.
gsMSG_MP_PTR::gsMSG_MP_PTR()
{
    packId = 0; flags = 0x40; data = NULL; size = 8;
    packId = gsMsgMpPtrPack.id;                      // byte v12 0x0098E868
}


// v12 0x0050E890  (feb 0x004FBEE0)
// gsEV_SYNC constructor.  1.2: packed 10-byte layout.
gsEV_SYNC::gsEV_SYNC()
{
    packId = 0; flags = 0x40;
    ev   = -100;                                     // 1.2: s16 at +4 (feb int)
    data = NULL;                                     // 1.2: +6 (feb +8)
    size = 0x0A;                                     // 1.2: (feb 0x0C)
    packId = gsMsgEvSyncPack.id;                     // byte v12 0x0098E858
}


// v12 0x0050E920  (feb 0x004FBF10)
// Packs the header then the wrapped message.
int gsMSG_MP_PTR_PACK::Pack(fioFILE_MEM *f, msgDATA *d)
{
    if (!msgDATA_PACK::Pack(f, d))
        return 0;
    if (((gsMSG_MP_PTR *)d)->data)
        msgSystem.Pack(f, ((gsMSG_MP_PTR *)d)->data);
    return 1;
}


// v12 0x0050E960  (feb 0x004FBF50)
// Unpacks the header then, if the pointer was set, the wrapped message.
msgDATA *gsMSG_MP_PTR_PACK::UnPack(fioFILE_MEM *f)
{
    gsMSG_MP_PTR *m = (gsMSG_MP_PTR *)msgDATA_PACK::UnPack(f);
    if (m->data)
        m->data = msgSystem.UnPack(f);
    return m;
}


// ===========================================================================
// 1.2: compressed join message pack (new in 1.2, no feb twin)
//
// Wire format written by Pack (all little-endian, no alignment):
//   u8   flags
//          0x01 uniform scale != 1  -> 1 float follows
//          0x02 non-uniform scale   -> 3 floats follow
//          0x04 explicit create s16 + float (+0xDA, +0xDC)
//          0x08 create s16 = 0, float = 0 (nothing written)
//          (neither 4 nor 8 = default -1 / 0)
//          0x10 entName starts with entClass -> only the suffix of entName is sent
//          0x20 a chained message (next) follows at the end
//   u32  mpid
//   str  name, str entClass, str entName(or suffix)  (fioFILE::Write strings, read max 63)
//   u16  unk0CC
//   [scale floats], [u16 + f32]
//   u16  posX, u16 posZ, u16 posYpacked     (origin of the start matrix, quantised)
//   u8   axisX pitch, u8 axisX yaw, u8 axisZ pitch, u8 axisZ yaw
//   [chained message]
// The feb build sent this message raw (0x164 = 356 bytes); 1.2 sends ~30-60 bytes.
// ===========================================================================

// Quantiser helpers (new 1.2 module at v12 0x00514170..; names are descriptive, not original):
//   mpQuantPos   v12 0x00514870: x,z in [-2000,2000] as 17-bit codes, y in [-250,250] as 14 bits.
//                out0 = x & 0xFFFF, out1 = z & 0xFFFF,
//                out2 = y14 | ((x >> 2) & 0x4000) | ((z >> 1) & 0x8000)
//   mpDequantPos v12 0x00514900: inverse.
//   mpQuantDir8  v12 0x00514180: pitch = angle(v, UnitY) in [0,180] -> 8 bits,
//                                yaw = xz-angle(v, UnitZ) in [-180,180] -> 8 bits.
//   mpDequantDir8 v12 0x005141E0: inverse (rotate UnitY by pitch, then RotY(-yaw)).
//   generic quantiser v12 0x00572240 (value, min, max; ecx = bits) clamps to the range,
//   dequantiser v12 0x00572330; 8-bit variants 0x00572440 / 0x00572540.
// TWEAK: a map whose start points lie outside x/z +-2000 or y +-250 gets clamped join
//        positions in 1.2 (resolution ~0.03 units in x/z and y).

int   __fastcall m3dQuantize(int bits, float v, float mn, float mx);          // v12 0x00572240 (m3d_quant.cpp)
float __fastcall m3dDequantize(unsigned code, int bits, float mn, float mx);  // v12 0x00572330

// v12 0x00514870
void __fastcall mpQuantPos(m3dV *p, unsigned short *qx, unsigned short *qz, unsigned short *qy)
{
    int y = m3dQuantize(14, p->y, -250.0f, 250.0f);
    int x = m3dQuantize(17, p->x, -2000.0f, 2000.0f);
    int z = m3dQuantize(17, p->z, -2000.0f, 2000.0f);
    *qx = (unsigned short)x;
    *qz = (unsigned short)z;
    *qy = (unsigned short)(((unsigned)x >> 2 & 0x4000) | ((unsigned)z >> 1 & 0x8000) | (y & 0x3FFF));
}

// v12 0x00514900
void __fastcall mpDequantPos(unsigned short qx, unsigned short qz, unsigned short qy, m3dV *p)
{
    p->x = m3dDequantize(((qy & 0x4000) << 2) | qx, 17, -2000.0f, 2000.0f);
    p->z = m3dDequantize(((qy & 0x8000) << 1) | qz, 17, -2000.0f, 2000.0f);
    p->y = m3dDequantize(qy & 0x3FFF, 14, -250.0f, 250.0f);
}

static inline bool NearlyEq(float a, float b, float eps) { return fabsf(a - b) < eps; }

// v12 0x0050E990  (feb none)
// 1.2: packs gsMSG_MP_JOIN_PLAYER in compressed form (see format above).
int gsMSG_MP_JOIN_PACK::Pack(fioFILE_MEM *f, msgDATA *d)
{
    gsMSG_MP_JOIN_PLAYER *m = (gsMSG_MP_JOIN_PLAYER *)d;
    unsigned char fl = 0;
    m3dV &s = *(m3dV *)&m->create;                   // scale at +0xCE

    if (!NearlyEq(s.x, s.y, 0.01f) || !NearlyEq(s.x, s.z, 0.01f))   // 0.01 = v12 0x005FAA38
        fl = 0x02;
    else if (!NearlyEq(s.x, 1.0f, 0.01f))
        fl = 0x01;

    short cs = m->create.s16_0C;                     // +0xDA
    float cf = m->create.f_0E;                       // +0xDC
    if (!(cs == -1 && fabsf(cf) < MP_EPSILON)) {
        if (cs == 0 && fabsf(cf) < MP_EPSILON) fl |= 0x08;
        else                                    fl |= 0x04;
    }
    if (strncmp(m->entName, m->entClass, strlen(m->entClass)) == 0) fl |= 0x10;
    if (m->next) fl |= 0x20;

    f->Write(&fl, 1, 1);
    f->Write(&m->mpid, 4, 1);
    f->Write(m->name);                               // fioFILE::Write(char*) v12 0x004E7BE0
    f->Write(m->entClass);
    f->Write((fl & 0x10) ? m->entName + strlen(m->entClass) : m->entName);
    int w = m->unk0CC;  f->Write(&w, 2, 2);

    if (fl & 0x01) {
        float v = s.x; f->Write(&v, 4, 4);
    } else if (fl & 0x02) {
        float v;
        v = s.x; f->Write(&v, 4, 4);
        v = s.y; f->Write(&v, 4, 4);
        v = s.z; f->Write(&v, 4, 4);
    }
    if (fl & 0x04) {
        w = cs; f->Write(&w, 2, 2);
        f->Write(&cf, 4, 4);
    }

    m3dMATR &mt = m->create.matr;                    // +0xE0
    m3dV o, ax;
    unsigned short qx, qz, qy;
    mt.GetOrigin(&o);
    mpQuantPos(&o, &qx, &qz, &qy);
    f->Write(&qx, 2, 2); f->Write(&qz, 2, 2); f->Write(&qy, 2, 2);

    unsigned char pitch, yaw;
    mt.GetAxisX(&ax);  mpQuantDir8(&ax, &pitch, &yaw);
    f->Write(&pitch, 1, 1); f->Write(&yaw, 1, 1);
    mt.GetAxisZ(&ax);  mpQuantDir8(&ax, &pitch, &yaw);
    f->Write(&pitch, 1, 1); f->Write(&yaw, 1, 1);

    if (m->next)
        msgSystem.Pack(f, m->next);
    return 1;
}


// v12 0x0050EDA0  (feb none)
// 1.2: unpacks a compressed join message into a static buffer (v12 0x0098E878, guard bit
//      0x0098E9DC, atexit stub 0x0050F0A0). The buffer is reset to defaults on every call.
msgDATA *gsMSG_MP_JOIN_PACK::UnPack(fioFILE_MEM *f)
{
    static gsMSG_MP_JOIN_PLAYER s_msg;               // 0x0098E878
    s_msg = gsMSG_MP_JOIN_PLAYER();                  // rep movsd 0x59 dwords

    unsigned char fl;
    f->Read(&fl, 1, 1);
    f->Read(&s_msg.mpid, 4, 4);
    f->Read(s_msg.name,     0x3F);                   // fioFILE::Read(char*, max) v12 0x004E7B30
    f->Read(s_msg.entClass, 0x3F);
    if (fl & 0x10)
        strcpy(s_msg.entName, s_msg.entClass);
    f->Read(s_msg.entName + strlen(s_msg.entName), 0x3F);
    f->Read(&s_msg.unk0CC, 2, 2);

    m3dV &s = *(m3dV *)&s_msg.create;
    if (fl & 0x01) {
        float v; f->Read(&v, 4, 4);
        s.x = s.y = s.z = v;
    } else if (fl & 0x02) {
        f->Read(&s.x, 4, 4); f->Read(&s.y, 4, 4); f->Read(&s.z, 4, 4);
    }
    if (fl & 0x04) {
        f->Read(&s_msg.create.s16_0C, 2, 2);
        f->Read(&s_msg.create.f_0E, 4, 4);
    } else if (fl & 0x08) {
        s_msg.create.s16_0C = 0;
        s_msg.create.f_0E   = 0.0f;
    }

    unsigned short qx, qz, qy;
    f->Read(&qx, 2, 2); f->Read(&qz, 2, 2); f->Read(&qy, 2, 2);
    m3dV o, ax, az;
    mpDequantPos(qx, qz, qy, &o);
    unsigned char pitch, yaw;
    f->Read(&pitch, 1, 1); f->Read(&yaw, 1, 1);  mpDequantDir8(pitch, yaw, &ax);
    f->Read(&pitch, 1, 1); f->Read(&yaw, 1, 1);  mpDequantDir8(pitch, yaw, &az);
    s_msg.create.matr.MakeLCS2WCS(&o, &ax, NULL, &az);   // v12 0x005624C0

    if (fl & 0x20)
        s_msg.next = msgSystem.UnPack(f);
    return &s_msg;
}


// v12 0x0050F0B0  (feb 0x004FBF80)
// Packs the header then the event's inner message.  1.2: pointer at +6 (feb +8).
// (identical code is also referenced by vtable v12 0x0060886C -- COMDAT folding.)
int gsMSG_MP_NOTIFY_EV_SYNC::Pack(fioFILE_MEM *f, msgDATA *d)
{
    if (!msgDATA_PACK::Pack(f, d))
        return 0;
    if (((gsEV_SYNC *)d)->data)
        msgSystem.Pack(f, ((gsEV_SYNC *)d)->data);
    return 1;
}


// v12 0x0050F0F0  (feb 0x004FBFC0)
// Unpacks header and, if present, the inner message.  1.2: pointer at +6.
msgDATA *gsMSG_MP_NOTIFY_EV_SYNC::UnPack(fioFILE_MEM *f)
{
    gsEV_SYNC *m = (gsEV_SYNC *)msgDATA_PACK::UnPack(f);
    if (m->data)
        m->data = msgSystem.UnPack(f);
    return m;
}


// v12 0x0050F120  (feb 0x004FC400)
// gsMP_STAT constructor (inline copy emitted in this object).
gsMP_STAT::gsMP_STAT()
{
    f10 = 0; f18 = 0; f14 = 0;                       // +0x10 buffer, +0x14, +0x18
    Clear();                                         // v12 0x0052EA70
}


// ---------------------------------------------------------------------------
// Not present in this object in 1.2 (feb functions without a v12 twin here):
//   gsMP_SYSTEM::UpdateEntFrame        feb 0x004FB560  -- likely moved to the new module
//   gsMP_SYSTEM::UpdateDeadReckonData  feb 0x004FB8D0     at v12 0x00514170.. (e.g. the
//                                                         IsRegularMPID-guarded function at
//                                                         v12 0x005149A0)  ?
//   gsMSG_MP_UPD_POS ctor / gsMSG_MP_UPD_POS_PACK::Pack/UnPack  feb 0x004FBFF0..0x004FC1C0
//   gsMP_CLIENT_FILTER_SNAPSHOT::IsSend feb 0x004FC310
//   entMP_DEADRECKON ctor               feb 0x004FC340
// ---------------------------------------------------------------------------

u8 __fastcall m3dFloatToByte(float v, float lo, float hi);         // v12 0x00572440 (ai_ctrl_anim.cpp)
float __fastcall m3dByteToFloat(u8 b, float lo, float hi);         // v12 0x00572540

// v12 0x00514180: a direction as two bytes: its angle from straight up (0..180) and its heading round Y (-180..180)
void __fastcall mpQuantDir8(m3dV *v, unsigned char *pitch, unsigned char *yaw)
{
    float p = m3dAngleVector(v, &m3dVUnitY);
    float y = m3xzAngleVector(v, &m3dVUnitZ);
    *pitch = m3dFloatToByte(p, 0.0f, 180.0f);
    *yaw = m3dFloatToByte(y, -180.0f, 180.0f);
}

// v12 0x005141E0: back: straight up turned about X by the pitch, then about Y by minus the heading (the matrices of
// m3dMATR::MakeRotX / MakeRotY, rows times the vector)
void __fastcall mpDequantDir8(unsigned char pitch, unsigned char yaw, m3dV *v)
{
    float p = m3dByteToFloat(pitch, 0.0f, 180.0f);
    float y = m3dByteToFloat(yaw, -180.0f, 180.0f);
    float a = 3.14159265f * p * (1.0f / 180.0f);
    m3dV u;
    u.x = 0;
    u.y = __builtin_cosf(a);                                 // UnitY's row of RotX: (0, c, s)
    u.z = __builtin_sinf(a);
    float b = 3.14159265f * -y * (1.0f / 180.0f);
    float c = __builtin_cosf(b), s = __builtin_sinf(b);              // RotY: rows (c, 0, -s), (0, 1, 0), (s, 0, c)
    v->x = u.x * c + u.z * s;
    v->y = u.y;
    v->z = -u.x * s + u.z * c;
}

// v12 0x00510980  (feb 0x004FC600): MainLoop ends after this frame
void __fastcall gsMsgStopLoop()
{
    gsAppState |= 8;
}
