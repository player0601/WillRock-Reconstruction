// wr_go_mp.cpp -- Will Rock 1.2 (v12.exe): wrGO_SYSTEM multiplayer rules, end of round
// and map rotation.  Reconstructed documentation source; NOT meant to compile.
// Types: wr_go_mp.h.
//
// Original object: wr_game_opt.obj.  Only the multiplayer-relevant functions are
// reconstructed (ProcessFRAME, ProcessINIT_LEVEL, ProcessINIT_GAME and the 1.2
// announcer).  Treasure / sync-data functions are left out.
//
// Round life cycle (server = gsSysMP->mode 2 or 4):
//   INIT_GAME   read the [Multiplayer] rule keys into wrSysGO
//   INIT_LEVEL  remove health/armour pickups if disabled, reset the round timers
//   FRAME       while playing: check frag / time / team-treasure limits -> broadcast
//               wrGO_MSG_GAMEOVER (GS_MSG_MP_NOTIFY sub 0); the round-over state
//               (gsAppState 0x80000) is set by whoever handles that message  // ? not traced
//   FRAME       when the round is over: show the result, wait for the "fire" key (or,
//               1.2 dedicated, 10 s), send GS_MSG_TERM_MAP (0x3F3), ask the server menu for
//               the next map and post GS_MSG_MP_NEW_MAP (0x411) to gspSysPlay 0.5 s later
//               -> gspPLAY_SYSTEM::StartNewGameMap v12 0x005347B0.

#include "wr_go_mp.h"
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

wrGO_SYSTEM* wrSysGO;                                             // v12 0x0064FF5C

// ===========================================================================
// v12 0x004A6440  (feb 0x00498B50)
// wrGO_SYSTEM::ProcessFRAME -- per-frame rules check / end-of-round handling.
// ===========================================================================
void wrGO_SYSTEM::ProcessFRAME()
{
    if (!(gsAppState & 4))                                        // 1.2: only while a game runs
        return;

    // 1.2: in Single mode re-declare the object if its MPID is not a regular one
    if (!gsSysMP->IsRegularMPID(mpid) && gsSysMP->mode == 1) {    // 0x0050E690
        mpid = -1;
        gsSysServer->DeclSyncable(this);                          // 0x0050F5A0
    }

    // -----------------------------------------------------------------------
    // round over
    // -----------------------------------------------------------------------
    if (gsAppState & 0x80000) {
        roundOverTime += gsElapsedTime;                           // 1.2

        int mode = gsSysMP->mode;
        if (mode == 1) {
            // 1.2: "GAME_LEVEL_COMPLETE" (feb "DEMO_OVER")
            uiSystem->ShowCenterText(gsStrings->GetStringById(gsStrings->GetStringId("GAME_LEVEL_COMPLETE")), 0.1f, 0);
        }
        else if (mode == 2 || mode == 4) {                        // 1.2: also the dedicated server (feb: 2 only)
            switch (gsSysNetwork->Server_GetLadderStatus()) {     // 0x0052FB00
            case 1:
                /* Ubi.com service: not reconstructed (dead service / licensing) --
                   shows GAME_MSG_LADDER_DATA_SENDING and returns until the upload ends */
                return;
            case 2:     // ladder upload ok    -> GAME_MSG_LADDER_DATA_SENT (3 s), StopLadder
            case 3:     // ladder upload failed-> GAME_MSG_LADDER_DATA_SENT_FAIL (3 s), StopLadder
                /* Ubi.com service: not reconstructed (dead service / licensing) */
                gsSysNetwork->Server_StopLadder();                // 0x0052FAE0
                break;
            default:
                uiSystem->ShowCenterText(gsStrings->GetStringById(gsStrings->GetStringId("GAME_OVER_SERVER")), 0.1f, 0);
                break;
            }
        }
        else {                                                    // client: just show it and wait for the server
            uiSystem->ShowCenterText(gsStrings->GetStringById(gsStrings->GetStringId("GAME_OVER_CLIENT")), 0.1f, 0);
            return;
        }

        if (mapChangePosted)                                      // 1.2: only once per round
            return;

        // continue on the "fire" action (input action 16, toggled this frame) ...
        bool go = false;
        gsINP_ACTIONS* act = gsSysInput->actions;                 // +0x40
        if (act->first <= 16 && act->last > 16 &&                 // action 16 in range
            (act->states[16 - act->first].flags & 1) &&           // 0x9C-byte records  // ? layout
            gsSysInput->IsToggled(16))                            // 0x00425D20
            go = true;
        // ... or, 1.2: on a dedicated server automatically after 10 s
        if (!go && (gsAppState & 0x08000000) && roundOverTime > 10.0f)   // const 0x005F93EC
            go = true;
        if (!go)
            return;

        gsSysNetwork->Server_ResetLadder();                       // 0x0052FAC0
        if (gsAppState & 0x4000) {                                // ? (paused / menu mode)
            gsMsgChangeMode(3);                                   // 0x00510D50
            gsAppState &= ~0x4000;
        }
        msgSystem.SendAll(0x3F3, NULL, NULL);                     // GS_MSG_TERM_MAP
        apMsg("GS_MSG_TERM_MAP");

        if (gsSysMP->mode == 1) {
            // 1.2: Single: reload the current level data instead of feb's
            //      "StopGame + back to the menu"   // ? meaning of the 0x3EC restart
            gspSTART_GAME_DATA* cur = &gsSysLevel->StartData();     // +0x3C3C
            uiSystem->SetLoadingLevel(cur->level);                // vt+0x98
            mapChangePosted = 1;
            msgSystem.PostTimeMsgData(gsSysLevel, 0x3EC, cur, NULL, 0.5f);   // 0x004D0570
            return;
        }

        if (gameType != WRGO_DM && gameType != WRGO_TH && gameType != WRGO_COOP) {   // 1.2: COOP added
            gspSysPlay->StopGame();                               // 0x005346A0
            uiSystem->ShowMainMenu(1, 0);                         // vt+0x3C (feb +0x38)  // ? name
            return;
        }

        // ---- next map ----
        gspSTART_GAME_DATA d;                                     // 0x10C bytes
        d.hdr.type  = 0;
        d.hdr.flags = 0x40;
        d.hdr.size  = 0x10C;
        d.level[0]  = 0;                                          // char level[0x104] at +4
        d.mode      = -1;                                         // +0x108
        uiSystem->GetNextLocation(d.level, 0x104);                // vt+0xA0 -> wrUI_ADVSERV_WINDOW::GetNextLocation (wr_ui_server.cpp)
        d.mode = 2;                                               // start as server
        uiSystem->SetLoadingLevel(d.level);                       // 1.2: vt+0x98
        mapChangePosted = 1;                                      // 1.2
        msgSystem.PostTimeMsgData(gspSysPlay, 0x411, &d, NULL, 0.5f);   // call at 0x004A67D7
        // 1.2: GS_MSG_MP_NEW_MAP = 0x411 posted with a 0.5 s delay;
        //      feb: PostMsgData(gspSysPlay, 0x40F, &d) at once
        return;
    }

    // -----------------------------------------------------------------------
    // playing: limits (server only)
    // -----------------------------------------------------------------------
    int mode = gsSysMP->mode;
    if (mode == 2 || mode == 4) {
        wrGO_MSG_GAMEOVER m;
        if (gameType == WRGO_DM || gameType == WRGO_TH) {
            m.hdr.type = 0; m.hdr.flags = 0x40; m.hdr.size = 0x52;
            m.sub = 0; m.gameOver = 0; m._4E = 0;
            ((char*)&m.winner)[0] = 0;

            if (fragLimit > 0) {
                for (plrPLAYER* p = (plrPLAYER*)gsSysServer->GetNextPlayer(NULL); p;
                     p = (plrPLAYER*)gsSysServer->GetNextPlayer(p)) {
                    if (p->frags >= fragLimit) {                  // +0x213
                        gameOverSent = 1;
                        const char* name = p->anim->name ? p->anim->name : "";
                        winnerName.SetName(name);                 // 1.2: winner name sent along
                        m.gameOver = 1;
                        m.winner   = winnerName;
                        gsSysServer->SendMsgClientAll(wrSysGO, 0x40E, &m, 0x20000 /*guaranteed*/, -3, -3);
                    }
                }
            }
            if (timeLimit > 0.0f && timeLimit < gsElapsedTimeLevel) {
                m.gameOver = 1;
                gsSysServer->SendMsgClientAll(wrSysGO, 0x40E, &m, 0x20000, -3, -3);
            }
        }

        if (gameType == WRGO_TH) {
            // team treasure totals; limit = 80 % of GetNStartPos()   // ? why start positions
            m.hdr.type = 0; m.hdr.flags = 0x40; m.hdr.size = 0x52;
            m.sub = 0; m.gameOver = 0; m._4E = 0;
            ((char*)&m.winner)[0] = 0;
            int t0 = 0, t1 = 0;
            for (plrPLAYER* p = (plrPLAYER*)gsSysServer->GetNextPlayer(NULL); p;
                 p = (plrPLAYER*)gsSysServer->GetNextPlayer(p)) {
                if (p->team == 0)      t0 += p->treasures;        // +0x20F, +0x21B
                else if (p->team == 1) t1 += p->treasures;
            }
            int limit = (int)(gsSysMP->GetNStartPos() * 0.8f);    // const 0x005F9E98
            if (t0 >= limit || t1 >= limit) {
                gameOverSent = 1;
                m.gameOver   = 1;
                if (t0 >= limit && t1 >= limit) winnerTeam = -1;  // 1.2: winner team
                else if (t0 >= limit)           winnerTeam = 0;
                else                            winnerTeam = 1;
                m.winner = winnerName;
                gsSysServer->SendMsgClientAll(wrSysGO, 0x40E, &m, 0x20000, -3, -3);
            }
        }
    }

    // 1.2: "N left" announcements (every mode)
    announcer.Update();                                           // 0x004A5CE0

    // 1.2: every 5 s the level time is broadcast (GS_MSG_MP_NOTIFY sub 1)
    if (m3dUpdateTimeField(&timeSyncTimer, gsElapsedTime)) {
        wrGO_MSG_TIMESYNC t = { { 0, 0x40, 0x0A }, 1, gsElapsedTimeLevel };
        gsSysServer->SendMsgClientAll(this, 0x40E, &t, 0xD0000, -3, -3);   // ? flags 0xD0000 = 0x80000|0x40000|0x10000
        timeSyncTimer = 5.0f;
    }
}

// ===========================================================================
// v12 0x004A5CE0  (new in 1.2)
// wrGO_ANNOUNCER::Update -- centre-screen countdowns:
//   GAME_MSG_TIMELEFT       (DM/TH with a time limit): at 10-s steps above 10 s left,
//                           5-s steps above 5 s, then every second
//   GAME_MSG_FRAGSLEFT      (DM): leader's frags to the limit, once below 4
//   GAME_MSG_TREASURESLEFT  (TH): leading team (UI_HUD_MP_TEAM_%i) treasures to 80 % of
//                           GetNStartPos(), once below 6
// Each shows for 3 s (ShowCenterText) and calls uiSystem slot +0x88 with 6 (5 for the last seconds).
// ? slot +0x88 is unidentified (a sound/flash cue?); the join list calls it with 1 after scrolling
// ===========================================================================
void wrGO_ANNOUNCER::Update()
{
    wrGO_SYSTEM* go = wrSysGO;

    if (go->gameType != WRGO_COOP && go->gameType != WRGO_SINGLE && go->timeLimit > 0.0f) {
        int left = (int)(go->timeLimit - gsElapsedTimeLevel + 0.9f);       // const 0x005F98D8
        if (left != 0 && left == nextTime) {
            uiSystem->ShowCenterText(gsStrings->Printf("GAME_MSG_TIMELEFT", left), 3.0f, 0);
            if (nextTime > 10)      { nextTime -= 10; uiSystem->Slot88(6); }
            else if (nextTime > 5)  { nextTime -= 5;  uiSystem->Slot88(6); }
            else                    { nextTime -= 1;  uiSystem->Slot88(5); }
        }
    }

    if (go->gameType == WRGO_DM && go->fragLimit != 0) {
        int best = 100000;
        plrPLAYER* leader = NULL;
        for (plrPLAYER* p = (plrPLAYER*)gsSysMP->GetNextPlayer(NULL); p;      // 0x0050E750
             p = (plrPLAYER*)gsSysMP->GetNextPlayer(p)) {
            if (p->frags == 0)
                continue;
            int left = go->fragLimit - p->frags;
            if (left < best || (left == best && (rand() & 1))) {
                best   = left;
                leader = p;
            }
        }
        if (best < nextFrags) {
            const char* name = leader->anim->name ? leader->anim->name : "";
            uiSystem->ShowCenterText(gsStrings->Printf("GAME_MSG_FRAGSLEFT", name, best), 3.0f, 0);
            uiSystem->Slot88(6);
        }
        nextFrags = best;
        if (best > 4)
            nextFrags = 4;
    }

    if (go->gameType == WRGO_TH) {
        int limit = (int)(gsSysMP->GetNStartPos() * 0.8f);
        int left[2] = { limit, limit };
        for (plrPLAYER* p = (plrPLAYER*)gsSysMP->GetNextPlayer(NULL); p;
             p = (plrPLAYER*)gsSysMP->GetNextPlayer(p)) {
            if (p->team == 0)      left[0] -= p->treasures;
            else if (p->team == 1) left[1] -= p->treasures;
        }
        int team = (left[0] < left[1]) ? 0 : (left[0] > left[1]) ? 1 : (rand() & 1);
        if (left[team] < nextTreasures) {
            wchar_t teamName[0x100];
            gsStrings->GetStringById(gsStrings->GetStringIdFmt("UI_HUD_MP_TEAM_%i", team + 1),
                                     teamName, 0x100);            // 0x0051D140
            uiSystem->ShowCenterText(gsStrings->Printf("GAME_MSG_TREASURESLEFT", teamName, left[team]), 3.0f, 0);
            uiSystem->Slot88(6);
        }
        nextTreasures = left[team];
        if (left[team] > 6)
            nextTreasures = 6;
    }
}

// ===========================================================================
// v12 0x004A6B80  (feb 0x00499060)
// wrGO_SYSTEM::ProcessINIT_LEVEL -- server side: remove the health items (item types
// 0x28..0x2B) when ServerAllowHealth = 0 and the armour items (0x2C..0x2F) when
// ServerAllowArmor = 0, in DM and TH only.  Resets the round state.
// 1.2: reset of mapChangePosted / timeSyncTimer / roundOverTime; Single-mode call 0x0049F740.
// ===========================================================================
void wrGO_SYSTEM::ProcessINIT_LEVEL()
{
    if (gsSysMP->mode == 1)
        sub_0049F740(gsSceneFullName);                            // 1.2  // ? (level-name/UI helper, not identified)

    mapChangePosted = 0;                                          // 1.2
    if (gsSysMP->mode == 3)                                       // client: the server owns the items
        return;

    int allow = 1;
    _apCfgReadInt(NULL, "Multiplayer", "ServerAllowHealth", &allow);
    if (!allow && (gameType == WRGO_DM || gameType == WRGO_TH)) {
        for (entENTITY* e = entFindNext(NULL); e; e = entFindNext(e))       // 0x005086A0
            if (e->classId == '$ITM' && e->itemType >= 0x28 && e->itemType <= 0x2B)   // +0x14, +0x98 (feb +0x94)
                msgSystem.PostMsg(e, 2, NULL);                    // 2 = destroy  // ?
    }

    allow = 1;
    _apCfgReadInt(NULL, "Multiplayer", "ServerAllowArmor", &allow);
    if (!allow && (gameType == WRGO_DM || gameType == WRGO_TH)) {
        for (entENTITY* e = entFindNext(NULL); e; e = entFindNext(e))
            if (e->classId == '$ITM' && e->itemType >= 0x2C && e->itemType <= 0x2F)
                msgSystem.PostMsg(e, 2, NULL);
    }

    timeSyncTimer = 5.0f;                                         // 1.2
    roundOverTime = -1.0f;                                        // 1.2
}

// ===========================================================================
// v12 0x004A6CE0  (feb 0x00499180)
// wrGO_SYSTEM::ProcessINIT_GAME -- reads the game rules from [Multiplayer]
// (written by the server menu, wr_ui_server.cpp).  Clients take gameType from the
// server's sync data and stop after NotifyEvent.
//   key                       field   default  forced
//   ServerMode                gameType  DM     Single: 0
//   Difficulty                +0xAC   1        DM/TH: 1;  >= 2 -> predictive AI aiming;
//                                              0 -> aiSpawnSetOppCoeff(0.6)
//   ServerTimeLimit (float s) +0xB0   120
//   ServerFragLimit           +0xB4   20       not DM: -1           (1.2: -1 forced)
//   ServerAutoBalanceTeam     +0xB8   0                             (1.2: new)
//   ServerPlrGODAfterDeath    +0x9C   -1.0     Single: -1
//   ServerPlrIsFriendlyFire   +0xA0   1        Single: 0
//   ServerPlrIsUnlimitAmmo    +0xA4   0        Single/COOP: 0       (1.2: COOP forced 0)
//   ServerItemOnPick          +0xA8            Single 0, DM/TH 1, COOP "keep" 2 else 0
// ===========================================================================
void wrGO_SYSTEM::ProcessINIT_GAME()
{
    int  mpMode = gsSysMP->mode;
    char buf[64];

    mpid = -1;
    gsSysServer->DeclSyncable(this);
    wrSysStat->Notify_GameBegin();                                // 0x004AE8E0
    buf[0] = 0;
    _apCfgReadString(NULL, "Multiplayer", "ServerMode", buf, 0x40);

    if (mpMode == 1) {
        gameType = WRGO_SINGLE;
        gspSysPlay->NotifyEvent((gsAppState & 0x20000) ? 2 : 0, gsSceneFullName, -1);
    } else {
        if (mpMode == 2 || mpMode == 4) {
            for (gameType = 0; gameType < 4; gameType++)          // names table v12 0x00605D04
                if (strcmp(wrGO_MODE_NAMES[gameType], buf) == 0)
                    break;
            if (gameType == 4)
                gameType = WRGO_DM;
        }
        // 1.2: the third argument is the game type only for a client (0 for a server)   // ? feb passed gameType
        gspSysPlay->NotifyEvent(0, gsSceneFullName, mpMode == 3 ? gameType : 0);
        if (mpMode == 3)
            return;                                               // 1.2: client stops here
    }

    difficulty = 1;
    _apCfgReadInt(NULL, "Multiplayer", "Difficulty", &difficulty);
    if (gameType == WRGO_DM || gameType == WRGO_TH)
        difficulty = 1;
    aiAttackPlanner->UsePredictiveAiming(difficulty >= 2);        // 0x00552050
    if (difficulty == 0)
        aiSpawnSetOppCoeff(0.6f);                                 // 0x00548DE0

    timeLimit = 120.0f;
    _apCfgReadFloat(NULL, "Multiplayer", "ServerTimeLimit", &timeLimit);
    fragLimit = 20;
    _apCfgReadInt(NULL, "Multiplayer", "ServerFragLimit", &fragLimit);
    if (gameType != WRGO_DM)
        fragLimit = -1;                                           // 1.2
    autoBalanceTeams = 0;                                         // 1.2
    _apCfgReadInt(NULL, "Multiplayer", "ServerAutoBalanceTeam", &autoBalanceTeams);

    godAfterDeath = -1.0f;
    _apCfgReadFloat(NULL, "Multiplayer", "ServerPlrGODAfterDeath", &godAfterDeath);
    if (gameType == WRGO_SINGLE)
        godAfterDeath = -1.0f;

    friendlyFire = 1;
    _apCfgReadInt(NULL, "Multiplayer", "ServerPlrIsFriendlyFire", &friendlyFire);
    if (gameType == WRGO_SINGLE)
        friendlyFire = 0;

    unlimitedAmmo = 0;
    _apCfgReadInt(NULL, "Multiplayer", "ServerPlrIsUnlimitAmmo", &unlimitedAmmo);
    if (gameType == WRGO_SINGLE || gameType == WRGO_COOP)
        unlimitedAmmo = 0;

    if (gameType == WRGO_SINGLE)
        itemMode = 0;
    else if (gameType == WRGO_COOP) {
        _apCfgReadString(NULL, "Multiplayer", "ServerItemOnPick", buf, 0x40);
        itemMode = (memcmp(buf, "keep", 5) == 0) ? 2 : 0;
    } else
        itemMode = 1;

    if (mpMode == 1) gsSysCinema->flags |= 4;                     // +0x80: cut-scenes on in Single only
    else             gsSysCinema->flags &= ~4;
    gsScenePtr->flags &= ~1;                                      // +0x40
    if (mpMode == 2 || mpMode == 4)
        gsScenePtr->flags |= 1;                                   // ? "server scene" flag
    gsMsgSetElapsedTimeScale(1.0f);                               // 0x00510CF0
}

// wrGO_SYSTEM::ProcessMsg v12 0x004A5FC0 (feb 0x00498870): switch on (msg - 0x3E8), byte
// table 0x004A6354 / jump table 0x004A631C: 0x3E8 (frame) -> ProcessFRAME (call 0x004A5FEC),
// 0x3EF (GS_MSG_INIT_GAME) -> ProcessINIT_GAME (call 0x004A6097), 0x3F4 -> ProcessINIT_LEVEL
// (call 0x004A5FFD); 0x3F0, 0x3F1, 0x3F2, 0x3FD, 0x405 (? MP_NOTIFY / sync data) not reconstructed.
//
// static initialiser of wr_game_opt.obj (v12 0x004A6B60): g_float_0x64FF60 = c*c (header constant).
