// gs_play.cpp -- Will Rock 1.2 (v12.exe) : gspPLAY_SYSTEM (start / stop / map change)
//
// Reconstructed from v12 disassembly against the Feb 2003 build (gs:gs_play.obj).
// Documentation for modders; not meant to compile into the game.
// "// 1.2:" = changed or new in 1.2 versus feb.  "// ?" = unknown / unverified.
// v12 range of this object: 0x00533EB0 .. 0x00534BC5 (feb 0x0051E610 .. 0x0051F24F).

#include "gs_play.h"
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

gspPLAY_SYSTEM *gspSysPlay;                       // v12 0x009A01DC
gspEVENT        _gspEventList[GSP_MAX_EVENTS];    // v12 0x009A01E0
static char     s_eventText[0x200];               // v12 0x009A5320 (GetString buffer, size ?)

static float s_epsSqA;                            // v12 0x009A0064
static float s_epsSqB;                            // v12 0x009A007C
static const float GSP_EPS_A = 1e-6f;             // v12 0x006083C0
static const float GSP_EPS_B = 1e-6f;             // v12 0x006083C4


// v12 0x00533EB0  (feb _$E? in 0x0051E610..)
// Static initializer: squared epsilon (init table v12 0x0062A53C). User unknown (?).
static void _$E_epsSqA() { s_epsSqA = GSP_EPS_A * GSP_EPS_A; }

// v12 0x00533ED0  (feb _$E9 0x0051E610 ?)
// Static initializer: empties the name of all 200 event slots.
static void _$E_eventList()
{
    for (int i = 0; i < GSP_MAX_EVENTS; ++i)
        *(char *)&_gspEventList[i].name = 0;      // apNAME first byte
}

// v12 0x005348D0  (feb _$E2 0x0051EFB0 ?)
// Static initializer: second squared epsilon (init table v12 0x0062A544). User unknown (?).
static void _$E_epsSqB() { s_epsSqB = GSP_EPS_B * GSP_EPS_B; }


// v12 0x00533EF0  (feb 0x0051E630)
// Creates gspSysPlay and registers it with the message system.
int __fastcall gspInit()
{
    gspPLAY_SYSTEM *p = (gspPLAY_SYSTEM *)operator new(0x30);
    if (p) {
        p->field04 = p->field08 = p->field0C = 0;
        p->magic   = 'MADR';                      // +0x10 = 0x4D414452
        p->field14 = p->field18 = p->field1C = p->field20 = p->field24 = 0;
        p->nEvents = 0;
        p->evCounter = 0;
        // vtable = gspPLAY_SYSTEM (v12 0x006083C8)
    }
    gspSysPlay = p;
    gspSysPlay->name     = 'GSP$';                // 0x47535024 ("$PSG" in memory)
    gspSysPlay->priority = 0x15;
    msgSystem.InsertAddr(gspSysPlay, '$SYS', 0);
    return 1;
}


// v12 0x00533F70  (feb 0x0051E6B0)
// Unregisters gspSysPlay.
void __fastcall gspTerm()
{
    msgSystem.DestroyAddr(gspSysPlay);
}


// v12 0x00533F90  (feb 0x0051E6D0)
// Appends an entry to the 200-slot event log (oldest dropped when full).
// Callers (1.2): wrGOTerm-area game code v12 0x004A60BD..0x004A6E01 (START/STOP, and the new
// PLAYER_JOIN 0x004A6227 / PLAYER_LEAVE 0x004A62C9 with style -1), gslSL::Save 0x0053022B,
// wrRENDER_SYSTEM::RenderSetOptions 0x004AC1E4.
void gspPLAY_SYSTEM::NotifyEvent(int type, char *name, int style)
{
    if (nEvents >= GSP_MAX_EVENTS) {               // 200 = 0xC8
        if (nEvents - 1 > 0)
            memmove(&_gspEventList[0], &_gspEventList[1], (nEvents - 1) * sizeof(gspEVENT));
        --nEvents;
    }
    time_t now;
    time(&now);

    gspEVENT &e = _gspEventList[nEvents];
    e.type = type;
    e.id   = evCounter;
    if (name)
        e.name.SetName(fnmGetName(name));
    e.mpMode  = gsSysMP->mode;
    e.style   = style;
    e.time    = now;
    e.nSdr    = *(int *)((char *)pteManager + 0x1F44);
    e.nInst   = gsScenePtr->GetNInst();
    e.nTpl    = gsScenePtr->GetNTpl();
    e.nEmit   = partGetNEmit();
    e.nSndChn = gsSysSound->GetNChn();
    ++nEvents;
    ++evCounter;
}


// v12 0x00534100  (feb 0x0051E840)
// Returns event idx or NULL.
gspEVENT *gspPLAY_SYSTEM::GetEvent(int idx)
{
    if (idx >= 0 && idx < nEvents)
        return &_gspEventList[idx];
    return NULL;
}


// v12 0x00534130  (feb 0x0051E870)
// Formats an event for the log/console into a static buffer.
// 1.2: two new event types and a label column ("Scene" / "namePlr").
char *gspEVENT::GetString()
{
    sprintf(s_eventText, " #: %d", id);            // v12 0x00636278 (result overwritten below)

    const char *evName, *label;
    switch (type) {                               // jump table v12 0x00534210, 7 entries
    case GSP_EV_START_GAME:   evName = "START_GAME";   label = "Scene";   break;
    case GSP_EV_STOP_GAME:    evName = "STOP_GAME";    label = "Scene";   break;
    case GSP_EV_LOAD_GAME:    evName = "LOAD_GAME";    label = "Scene";   break;
    case GSP_EV_SAVE_GAME:    evName = "SAVE_GAME";    label = "Scene";   break;
    case GSP_EV_CRASH_GAME:   evName = "CRASH_GAME";   label = "Scene";   break;
    case GSP_EV_PLAYER_JOIN:  evName = "PLAYER_JOIN";  label = "namePlr"; break;   // 1.2: new
    case GSP_EV_PLAYER_LEAVE: evName = "PLAYER_LEAVE"; label = "namePlr"; break;   // 1.2: new
    default:                  evName = "???";
                              label  = /* ? stale stack slot */ NULL;   break;
    }
    // v12 0x006361A0
    sprintf(s_eventText,
            "#: %d  Ev: %s  Time: %s         %s: %s Mode: %s  Style: %d\n"
            "         nInst: %d  nTpl: %d  nEmit: %d  nSdr: %d",
            id, evName, ctime(&time), label, (char *)&name,
            gsSysMP->Mode2String(mpMode), style, nInst, nTpl, nEmit, nSdr);
    return s_eventText;
}


// v12 0x00534230  (feb 0x0051E930)
// First start after boot: creates the local "player" entity and either opens the menu
// ([Video] UseMenu) or starts the game directly from the config.
int gspPLAY_SYSTEM::StartFirst()
{
    int useMenu = 0;
    _apCfgReadBool(NULL, "Video", "UseMenu", &useMenu);
    if (useMenu)
        gsAppState |= 0x2000;                      // menu mode

    gsAppState |= 0x40000;
    entENTITY *plr = NULL;
    if (!(gsAppState & GS_APP_DEDICATED))          // 1.2: dedicated server spawns no local player
        plr = entCreate(gsScenePtr, "player", "player", NULL, NULL, 0);
    gsAppState &= ~0x40000;

    if (!plr && !(gsAppState & GS_APP_DEDICATED)) {   // 1.2: failure ignored when dedicated
        gsAppState &= ~0x10;
        return 0;
    }

    for (entENTITY *e = entFindNext(NULL); e; e = entFindNext(e))
        e->flags &= ~1u;                           // 1.2: +0xB8 (feb +0xB4)

    if (!(gsAppState & 0x2000)) {
        if (!StartGame(NULL)) {
            gsAppState &= ~0x10;
            return 0;
        }
        if (uiSystem)
            uiSystem->SetMode(3, 0);               // 1.2: slot +0x3C (feb +0x38), name ?
    } else if (uiSystem) {
        uiSystem->SetMode(1, 0);
    }
    gsAppState &= ~0x10;
    return 1;
}


// v12 0x00534360  (feb 0x0051EA50)
// Starts a game: stops the old one, initialises gsMP_SYSTEM in the requested mode, loads the
// level (server/single) and joins.  d == NULL reads [Multiplayer] Mode and [Level] File.
int gspPLAY_SYSTEM::StartGame(gspSTART_GAME_DATA *d)
{
    static gsMSG_LOAD_LEVEL s_load;                // v12 0x009A5678, guard 0x009A0080,
                                                   // atexit stub 0x005345B0 (size 0x154)
    gspSTART_GAME_DATA local;                      // packId 0, flags 0x40, size 0x10C
    local.mode     = -1;
    local.level[0] = 0;
    char modeStr[64];

    if (gsAppState & 0x4000) {                     // leave special input/message mode
        gsMsgChangeMode(3);
        gsAppState &= ~0x4000;
    }
    StopGame();
    gsSysNetwork->Drv_Freeze(0);

    if (!d) {
        local.mode = gsMP_MODE_SINGLE;
        if (_apCfgReadString(NULL, "Multiplayer", "Mode", modeStr, 64)) {
            local.mode = gsSysMP->String2Mode(modeStr);
            if (local.mode == 0)
                local.mode = gsMP_MODE_SINGLE;
        }
        _apCfgReadString(NULL, "Level", "File", local.level, 260);
        d = &local;
    }

    if (gsAppState & GS_APP_DEDICATED)             // 1.2: new. -dedicated always runs as
        d->mode = gsMP_MODE_SERVER_ONLY;           //      "ServerOnly" (patches caller's data)

    if (!gsSysMP->InitGame(d->mode))
        return 0;

    msgSystem.SendAll(GS_MSG_INIT_GAME /*0x3EF*/, NULL, NULL);
    apMsg("GS_MSG_INIT_GAME");

    if (gsSysMP->mode != gsMP_MODE_CLIENT) {       // clients get the level from the server
        gsAppState |= 4;                           // game running
        strcpy(s_load.level, d->level);
        s_load.unk148 = 0;
        msgSystem.SendMsg(gsSysLevel, GS_MSG_LOAD_LEVEL /*0x3EC*/, &s_load, NULL);
        uiSystem->vfunc9C(0);                      // 1.2: slot +0x9C (feb +0x98)  ?
    }

    if (!gsSysMP->JoinGame())
        return 0;
    gsSysNetwork->Drv_NotifyGameStarted();

    if (gsSysMP->mode != gsMP_MODE_CLIENT) {
        msgSystem.SendAll(GS_MSG_INIT_MAP /*0x3F1*/, NULL, NULL);
        apMsg("GS_MSG_INIT_MAP");
    }
    return 1;
}


// v12 0x005345C0  (feb 0x0051EC90)
// Loads a saved game (single player): InitGame, gslLoad, JoinGame.
int gspPLAY_SYSTEM::LoadGame(gspSTART_GAME_DATA *d)
{
    static gsMSG_LOAD_LEVEL s_unused;              // v12 0x009A0088, guard 0x009A0078,
                                                   // atexit stub 0x00534690 (never sent)
    StopGame();
    if (!gsSysMP->InitGame(d->mode))
        return 0;
    gslLoad(d->level, NULL);
    if (!gsSysMP->JoinGame())
        return 0;
    gsSysNetwork->Drv_NotifyGameStarted();
    uiSystem->vfunc9C(0);                          // 1.2: slot +0x9C (feb +0x98)
    gsAppState |= 4;
    return 1;
}


// v12 0x005346A0  (feb 0x0051ED70)
// Stops the running game: leave, broadcast term messages, stop the network driver,
// clear the level and terminate gsMP_SYSTEM.  (unchanged in 1.2 apart from addresses)
void gspPLAY_SYSTEM::StopGame()
{
    if (gsAppState & 4)
        gsSysMP->LeaveGame();

    if (gsAppState & 4) {
        if (gsAppState & 0x4000) {
            gsMsgChangeMode(3);
            gsAppState &= ~0x4000;
        }
        msgSystem.SendAll(GS_MSG_PRE_TERM_GAME /*0x3F3*/, NULL, NULL);
        msgSystem.SendAll(GS_MSG_TERM_GAME     /*0x3F0*/, NULL, NULL);
        apMsg("GS_MSG_TERM_GAME");
    }

    if (gsSysNetwork->Drv_IsGameStarted())
        gsSysNetwork->Drv_Stop();                  // closes the DirectPlay session

    if (gsAppState & 4) {
        gsMSG_LOAD_LEVEL empty;                    // all zero, size 0x154
        empty.packId = 0; empty.flags = 0x40; empty.size = 0x154;
        empty.level[0] = 0; empty.unk108[0] = 0;
        empty.unk148 = empty.keepSession = empty.unk150 = 0;
        gsSysLevel->UnloadLevel(&empty);           // vt +0x20, name ?
    }

    bool wasRunning = (gsAppState & 4) != 0;
    gsAppState &= ~GS_APP_CLIENT_WAITLVL;          // 0x400000
    if (wasRunning) {
        gsSysMP->TermGame();
        gsAppState &= ~4;
    }
}


// v12 0x005347B0  (feb 0x0051EE80)
// Server side map change inside a running session (map rotation / admin "next map"):
// freezes the driver, deactivates clients, tells them the new map and reloads locally
// with keepSession = 1.
// 1.2: also runs on a dedicated server (mode 4); feb only for mode 2.
int gspPLAY_SYSTEM::StartNewGameMap(gspSTART_GAME_DATA *d)
{
    static gsMSG_LOAD_LEVEL s_load;                // v12 0x009A5520, guard 0x009A5674,
                                                   // atexit stub 0x005348F0
    if (gsAppState & 0x4000) {
        gsMsgChangeMode(3);
        gsAppState &= ~0x4000;
    }
    int m = gsSysMP->mode;
    if (m == gsMP_MODE_SERVER || m == gsMP_MODE_SERVER_ONLY) {   // 1.2: + mode 4
        gsSysNetwork->Drv_Freeze(1);               // stop processing client traffic
        gsSysServer->DeActivateClientAll();
        gsSysServer->SendMsgClientAll(NULL, GS_MSG_MP_NEW_MAP /*0x411, feb 0x40F*/, d,
                                      0x20000, -1, -3);
        gsAppState |= 4;
        strcpy(s_load.level, d->level);
        s_load.unk148      = 0;
        s_load.keepSession = 1;                    // tells the level loader to keep the MP session
        msgSystem.SendMsg(this, GS_MSG_LOAD_LEVEL /*0x3EC*/, &s_load, NULL);
    }
    return 1;
}


// v12 0x00534900  (feb 0x0051EFD0)
// Message handler of the play system.  Jump table: byte index v12 0x00534B9C (msg-0x3EA,
// 42 entries), targets v12 0x00534B80.
int gspPLAY_SYSTEM::ProcessMsg(int msg, void *data, msgADDR *from, int *res)
{
    switch (msg) {
    case GS_MSG_START_GAME:                        // 0x3EA
        StartGame((gspSTART_GAME_DATA *)data);
        return 0;

    case GS_MSG_STOP_GAME:                         // 0x3EB
        StopGame();
        return 0;

    case GS_MSG_LOAD_LEVEL:                        // 0x3EC (self-sent by StartNewGameMap)
        msgSystem.PostMsgData(gsSysLevel, GS_MSG_LOAD_LEVEL, (msgDATA *)data, NULL);
        if (gsSysMP->mode != gsMP_MODE_SINGLE) {
            gsSysServer->SendMsgClientAll(NULL, GS_MSG_MP_LEVEL_CHANGE /*0x412, feb 0x410*/,
                                          NULL, 0x20000, -1, -3);
            gsSysServer->DeActivateClientAll();
            for (entENTITY *p = gsSysMP->GetNextPlayer(NULL); p; p = gsSysMP->GetNextPlayer(p))
                if (p->connId != -1)               // 1.2: +0x8C (feb +0x90)
                    gsSysServer->SendMsgClient(p->connId, NULL, GS_MSG_LOAD_LEVEL,
                                               data, 0x20000);       // vt +0x2C
        }
        return 0;

    case GS_MSG_LEVEL_LOADED_2:                      // 0x3F7   1.2: feb reacted to 0x3F1
        msgSystem.PostMsg(this, GS_MSG_MP_PLACE_PLAYERS /*0x413, feb 0x411*/, NULL);
        return 0;

    case GS_MSG_MP_NEW_MAP:                        // 0x411 (feb 0x40F)
        StartNewGameMap((gspSTART_GAME_DATA *)data);
        return 0;

    case GS_MSG_MP_PLACE_PLAYERS: {                // 0x413 (feb 0x411)
        // after a (re)load: give every connected player a start position, resync it,
        // then unfreeze the network driver.  Slot 0 is skipped (idx starts at 1).
        int m = gsSysMP->mode;
        if (m != gsMP_MODE_SERVER && m != gsMP_MODE_SERVER_ONLY)
            return 0;
        int idx = 1;                               // (reuses the 'data' arg slot)
        for (entENTITY *p = gsSysMP->GetNextPlayer(NULL); p; p = gsSysMP->GetNextPlayer(p)) {
            if (p->connId == -1)                   // +0x8C: no network connection (host player)
                continue;
            gsEV_SET_START_POS ev;
            ev.packId = 0; ev.flags = 0x40;
            ev.size   = 0x1E;                      // 30
            ev.ev     = -97;                       // 1.2: (feb -95)
            ev.pos    = m3dVUndef;
            ev.dir    = m3dVUndef;
            m3dV pos, dir;
            if (gsSysMP->GetNStartPos() <= 1) {    // 1.2: new. one start point -> scatter
                gsSysMP->SelectStartPosDir(&pos, &dir, p);
            } else {
                gsSysMP->GetStartPosDir(idx, &pos, &dir);
                ++idx;                             // round-robin over the start points
            }
            ev.pos = pos;
            ev.dir = dir;
            // 1.2: delivered locally to the player entity via msgSystem
            //      (feb: gsMP_SERVER::SendMsgClient(p, 0x40C, ...))
            msgSystem.SendMsg(p, GS_MSG_MP_SET_START /*0x40E*/, &ev, NULL);
            gsSysServer->SyncClient(p);            // v12 0x0050F710 (feb SyncClient)  ?
        }
        gsSysNetwork->Drv_Freeze(0);
        return 0;
    }
    }
    return 0;
}
