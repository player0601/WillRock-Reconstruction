// wr_game.h -- Will Rock 1.2 unofficial SDK, subsystem "frontend": game rules (wrGO_SYSTEM, wr_game_opt.obj),
// game options ([GameOpt]), debug keys / cheats (DBG_1..9), and the localisation keys the frontend uses.
//
// Assumed from re/sdk/core: msgADDR, msgDATA, msgSYSTEM msgSystem, entIACTIVE_OBJ (engine_core/entity.h: v12 size
//   0x98, feb 0x94; 1.2 inserted a vtable slot at +0x20), gsEVENT, gsMSG_MP_SAVELOAD, fioFILE, m3dV,
//   plrPLAYER_COMMON (player/player.h), gsMP_SYSTEM gsSysMP (+0x28 = session type), gsAppState.
//
// Offsets: "// v12 +0xNN (feb +0xNN)". All wrGO_SYSTEM offsets were read from v12 instructions (addresses given).
// The object grew from 0xB4 (feb) to 0x11C (1.2): the feb fields moved +4 (bigger entIACTIVE_OBJ base) and 1.2
// added fields from +0xB8 (auto-balance, level-end guard, chat line, countdown announcer).

#pragma once
#pragma pack(push, 1)

#include "ui_core.h"   // __fastcall shim for non-MSVC syntax checks
struct gsEVENT; struct gsMSG_MP_SAVELOAD; struct m3dV; class plrPLAYER_COMMON; class msgADDR;
#ifndef WR_SDK_CORE   // the integrator's core header defines the real entIACTIVE_OBJ (v12 0x98 bytes incl. vptr)
class entIACTIVE_OBJ { public: virtual ~entIACTIVE_OBJ(); unsigned char _base[0x94]; }; // placeholder: 4 + 0x94 = 0x98
#endif

// ------------------------------------------------------------------------------------------------ game modes
// wrGO_SYSTEM::mode. [Multiplayer] ServerMode is compared with the string table v12 0x00605D04
// {"SINGLE","DM","COOP","TH"} (feb 0x005DA7E4 {"SINGLE","DM","COOP","TREASHUNT"}: the ini value for treasure hunt
// changed from TREASHUNT to TH in retail); an unknown string gives DM. Single player always gives 0.
enum wrGAME_MODE { WRGM_SINGLE = 0, WRGM_DM = 1, WRGM_COOP = 2, WRGM_TH = 3 /* treasure hunt, the team mode */ };
// gsSysMP+0x28 (session type, engine): 1 = single player, 2 = MP server, 3 = MP client, 4 = dedicated/listen ? (2 and 4 run the server rules)

// ------------------------------------------------------------------------------------------------ wrGO_SYSTEM
// Global wrSysGO. Created by wrGOInit, registered in msgSystem with id 'SYS$' prio 0x14, name "WRGO_SYSTEM",
// object type 'OGRW' (+0x14 = 0x5752474F).
struct wrGO_ANNOUNCER_ { int nextTime, nextTreasures, nextFrags; void Update(); /* v12 0x004A5CE0 */ };
class wrGO_SYSTEM : public entIACTIVE_OBJ {
public:
    // vtable v12 0x00605D18 (feb 0x005DA7F4): 16 slots in 1.2 (15 in feb). Only these are overridden:
    // slot +0x04 ProcessMsg                v12 0x004A5FC0 (feb 0x00498870)
    // slot +0x34 ProcessMP_GET_SYNC_DATA   v12 0x004A72E0 (feb +0x30 0x00499650)  mode + difficulty to clients
    // slot +0x38 ProcessMP_WRITE_SYNC_DATA v12 0x004A73B0 (feb +0x34 0x004996C0)  savegame chunk 0xF1 (difficulty)
    // slot +0x3C ProcessMP_READ_SYNC_DATA  v12 0x004A7460 (feb +0x38 0x00499740)  chunks 0xF0 mode, 0xF1 difficulty
    // (the 1.2 slot +0x20 inserted in entIACTIVE_OBJ moves the feb slots >= +0x20 by +4)

    // [integrator] names merged with re/mpsrc/wr_go_mp.h; +0xC0..+0x10C re-checked in v12 (0x004A6A4A winnerTeam = -1/0/1,
    // 0x004A7B30 returns &winnerName when gameOverSent, 0x004A6CB9 timeSyncTimer = 5.0). The earlier chat guesses were wrong.
    union { int mode; int gameType; }; // v12 +0x98 (feb +0x94) wrGAME_MODE. v12 0x004A5CF5, 0x004A6D39, 0x004A7990
    float godAfterDeath;    // v12 +0x9C (feb +0x98) [Multiplayer] ServerPlrGODAfterDeath, default -1 (off); forced -1 in SP. v12 0x004A6ECD..0x004A6EF3
    int   friendlyFire;     // v12 +0xA0 (feb +0x9C) ServerPlrIsFriendlyFire, default 1; 0 in SP. v12 0x004A6EF9
    int   unlimitedAmmo;    // v12 +0xA4 (feb +0xA0) ServerPlrIsUnlimitAmmo, default 0; 0 in SP.  v12 0x004A6F21
    union { int itemOnPick; int itemMode; }; // v12 +0xA8 (feb +0xA4) 0 = SP/remove, 1 = DM/TH (respawn), 2 = COOP "keep" (ServerItemOnPick)
    int   difficulty;       // v12 +0xAC (feb +0xA8) [Multiplayer] Difficulty 0 easy / 1 normal / 2 hard; forced 1 in DM and TH. v12 0x004A6E0F, 0x004A7B00
                            //   >= 2 enables aiPLANNER::UsePredictiveAiming; 0 calls aiSpawnSetOppCoeff(0.6) (v12 0x004A6E5E)
    float timeLimit;        // v12 +0xB0 (feb +0xAC) ServerTimeLimit (minutes as float), default 120. v12 0x004A6E68, 0x004A5D0C
    int   fragLimit;        // v12 +0xB4 (feb +0xB0) ServerFragLimit, default 20; 1.2 forces -1 unless DM (feb: always read). v12 0x004A6E86..0x004A6EAD
    union { int autoBalanceTeam; int autoBalanceTeams; }; // v12 +0xB8 (new) ServerAutoBalanceTeam, default 0. v12 0x004A6EB3
    union { int levelEndLoading; int mapChangePosted; };  // v12 +0xBC (new) one-shot guard: next level already requested
    apNAME winnerName;      // v12 +0xC0 (new) winner name copied into wrGO_MSG_GAMEOVER; GetWinnerName v12 0x004A7B30
    int   winnerTeam;       // v12 +0x100 (new) TH: -1 draw, 0, 1 (v12 0x004A6A4A..0x004A6AA2)
    int   gameOverSent;     // v12 +0x104 (new) round over, GAMEOVER sent
    float timeSyncTimer;    // v12 +0x108 (new) 5 s period of wrGO_MSG_TIMESYNC (v12 0x004A6CB9)
    union { float levelEndTimer; float roundOverTime; }; // v12 +0x10C (new) seconds since the round ended (-1 at level start); dedicated: next map at 10 s
    union {
        struct { int nextTimeAnnounce, nextFragAnnounce, nextTreasAnnounce; }; // v12 +0x110..+0x118 (new) "N left" announcer   // ? order
        wrGO_ANNOUNCER_ announcer;                                                     // re/mpsrc view (order as mpsrc) // ?  // re/mpsrc view (order as mpsrc) // ?
    };
    // sizeof v12 0x11C (feb 0xB4). verified: push 0x11C at v12 0x004A5BD2, init stores 0x004A5BF2..0x004A5C5C

    int   ProcessMsg(int msg, void *data, msgADDR *from, int *res); // v12 0x004A5FC0 (feb 0x00498870)
    void  ProcessFRAME();                    // v12 0x004A6440 (feb 0x00498B50) HOOK: MP limits, level-complete / next level, demo-over (feb)
    void  ProcessINIT_LEVEL();               // v12 0x004A6B80 (feb 0x00499060) removes health/armor items if ServerAllowHealth/Armor = 0 (DM/TH)
    void  ProcessINIT_GAME();                // v12 0x004A6CE0 (feb 0x00499180) HOOK: reads all [Multiplayer] rules above
    void  ProcessMP_NOTIFY(gsEVENT *ev);     // v12 0x004A7010 (feb 0x004994A0) ev 0 = map complete (lev_10_4 -> outro comics + credits), -100 = sync
    void  GenerateMP_Treasure(int n, m3dV *pos);    // v12 0x004A75C0 (feb 0x00499840) TH: spawn item_mpth_sword/helmet/shield
    void  SelectMP_TreasureDefaultPos(m3dV *out);   // v12 0x004A7690 (feb 0x00499910) ?
    int   SelectDefaultTeam(plrPLAYER_COMMON *p);   // v12 0x004A7990 (feb 0x00499B60) ? (TH: smaller team; v12 0x004A7A10 is a second, similar team function)
    int   IsCanChangeTeam(plrPLAYER_COMMON *p);     // v12 0x004A7A10 (name ours) the auto-balance lets the player change teams -- not rebuilt
    int   GetNMP_TreasureTotal();            // v12 0x004A7AC0 (feb 0x00499BE0) ?
    int   GetNMP_TreasureVictory();          // v12 0x004A7AD0 (feb 0x00499BF0) ? total * 0.8
    float GetPlrDmgScale();                  // v12 0x004A7B00 (feb 0x00499C20) damage taken by the player: easy 0.5, normal 1.0, hard 1.5 (feb hard = 2.0)
    void  AnnounceCountdown(int *state);     // v12 0x004A5CE0 (new in 1.2) GAME_MSG_TIMELEFT/FRAGSLEFT/TREASURESLEFT + UI sound 5/6
};
// wrSTAT_SYSTEM: per-level / per-player statistics (wr_stat.obj), global wrSysStat v12 0x006500D0. size v12 0x144 (feb 0x140).
// [integrator] declarations only, from the feb map; v12 twins marked "callgraph" are unverified.
class wrSTAT_SYSTEM : public entIACTIVE_OBJ {
public:
    void Notify_PlrHit(plrPLAYER_COMMON *p, dmgDAMAGE *d);   // v12 0x004AE830 (feb 1 arg) (wr_stat.cpp)
public:
    void  Notify_GameBegin();                 // v12 0x004AE8E0 (re/mpsrc) (feb 0x004A0020)
    void  Notify_GameEnd();                   // feb 0x004A0050
    void  Notify_AddPlayer(plrPLAYER_COMMON *p);  // feb 0x004A0080
    void  Notify_StopPlayer(plrPLAYER_COMMON *p); // feb 0x004A0190
    void  Notify_NPCDie(unsigned long id);    // feb 0x0049F930
    void  Notify_NPCDmg(unsigned long id, int dmg); // feb 0x0049F9D0
    void  Notify_WpnHit(unsigned long id, int wpn);  // v12 0x004AE1B0 (feb 0x0049FA10?) a weapon hit (name ours)
    void  Notify_WpnShot(plrPLAYER_COMMON *p, pwpWEAPON *w);  // v12 0x004AE710 a shot (name ours) (wr_stat.cpp)
    void  Notify_WpnTime(plrPLAYER_COMMON *p, pwpWEAPON *w);  // v12 0x004AE660 the frame's time with a weapon (name ours)
    void  Notify_Pick(plrPLAYER_COMMON *p, int kind, float n);  // v12 0x004AE3C0 an item taken (name ours)
    void  Notify_Death(plrPLAYER_COMMON *p);              // v12 0x004AE520 a death (name ours)
    char *GetPlrRecord(plrPLAYER_COMMON *p);              // v12 0x004AD0D0 (name ours) the player's record (0: none; inline in the Notify_*)
    void  Notify_Kill(plrPLAYER_COMMON *p, int n);        // v12 0x004AE5C0 frags += n (name ours)
    void  Notify_CreateItem(int type, float val);   // v12 0x004AE2F0 (feb 0x0049FA80) (wr_stat.cpp)
    float GetMapTime();                                   // v12 0x004AEB60 (feb 0x004A0220) (wr_stat.cpp)
    int   GetMapKills(plrPLAYER_COMMON *p, int *total);   // v12 0x004AEB70 (feb 0x004A0230)
    int   GetMapSecrets(plrPLAYER_COMMON *p, int *total); // v12 0x004AEC20 (feb 0x004A02D0)
    int   GetMapMoney(plrPLAYER_COMMON *p, int *total);   // v12 0x004AECD0 (feb 0x004A0370)
    int   GetFavouriteWpn(plrPLAYER_COMMON *p);           // v12 0x004AED80 (feb 0x004A0410)
    float GetShootAccuracy(plrPLAYER_COMMON *p, int *shots, int *hits); // v12 0x004AEE50 (feb 0x004A04D0)
    void  ProcessFRAME();                                 // v12 0x004AD2EC (ProcessMsg's frame case, name ours)
};
int  __fastcall wrGOInit();                  // v12 0x004A5BD0 (feb 0x00498790) called by wrAPP_SYSTEM::InitSubsys
void __fastcall wrGOTerm();                  // v12 0x004A5CC0 (feb 0x00498850)
extern wrGO_SYSTEM *wrSysGO;                 // v12 0x0064FF5C (feb 0x0060EFDC)  verified v12 0x004A5C70

// Messages wrGO_SYSTEM::ProcessMsg handles (v12 jump table 0x004A631C / index 0x004A6354, base 1000).
// Ids >= 0x3F5 were renumbered in retail; both columns given.
enum wrGO_MSG {
    WRGO_MSG_FRAME        = 0x3E8,  // feb 0x3E8 -> ProcessFRAME
    WRGO_MSG_INIT_GAME    = 0x3EF,  // feb 0x3EF -> ProcessINIT_GAME
    WRGO_MSG_GAME_END     = 0x3F0,  // feb 0x3F0 -> wrSTAT Notify_GameEnd + gspPLAY NotifyEvent(1)
    WRGO_MSG_START_TH     = 0x3F1,  // feb 0x3F1 -> TH server: one treasure per start position
    WRGO_MSG_3F2          = 0x3F2,  // new case in 1.2 (level complete broadcast)                 // ?
    WRGO_MSG_INIT_LEVEL   = 0x3F4,  // feb 0x3F4 -> ProcessINIT_LEVEL
    WRGO_MSG_QUERY        = 0x3FD,  // feb 0x3FC -> returns 1
    WRGO_MSG_PLR_ADD      = 0x407,  // feb 0x406 -> server sends sync (0x40E) to the new client
    WRGO_MSG_PLR_JOINED   = 0x408,  // feb 0x407 -> wrSTAT AddPlayer, "GAME_PLAYER_JOINED" message
    WRGO_MSG_PLR_LEFT     = 0x409,  // feb 0x408 -> wrSTAT StopPlayer, "GAME_PLAYER_DISCONNECTED"
    WRGO_MSG_MP_NOTIFY    = 0x40E,  // feb 0x40C -> ProcessMP_NOTIFY (gsEVENT)
    WRGO_MSG_4CE          = 0x4CE,  // new in 1.2: gscCursor call (v12 0x00513F10)                // ?
    WRGO_MSG_4CF          = 0x4CF,  // new in 1.2: gscCursor call                                  // ?
};

// ------------------------------------------------------------------------------------------------ game options
// INI section [GameOpt] (apCfgRead*/Write*), edited by wrUI_GAME_OPT_WINDOW (v12 LoadSettings 0x00494790,
// SaveSettings 0x00494950). Child element names of the options page in brackets.
//   AutoWeapon   int  ("autoweapon", values UI_AUTOWPN_%i)   auto-switch to picked-up weapon; read by plrPLAYER ProcessINIT_MAP (v12 0x00427AAD)
//   HideHUD      bool ("hide_hud")   ShowHudFrame hides hud_wnd (v12 0x0048C109)
//   Crosshair    int  ("crosshair")  0 = off (UI_CUSOR_OFF), else crosshair texture index -> wrCURSORS::SetCursorType
//   Subtitles    bool ("subtitles")  voice subtitles (wrMSG_SYSTEM, game_systems/domains.h)
//   DisplayBlood bool ("blood")      0 -> wrAppState |= 2 (no blood), set at v12 0x0046A9E0; forced off by German/OEM flags (wrAppState & 5)
//   DisplayPing  bool ("ping")       1.2 only: HUD ping text in hud_uh_wnd
// Other UI keys: [Input] MouseSens, InvertMouse, MouseSensIface (controls window); [Language] tplFontS/B/H,
//   cmsLogo/cmsIntro/cmsOutro (comics), Language; [Level] pathSAVEGAME, pass (1.2 level select);
//   [Debug] SwitchWindow (website button only works when 1), ShowFPS_PPF, ShowProfInfo (debug HUD), Decomp_OFF_AI;
//   [Multiplayer] PlayerName, PlayerSkin, Server* (advanced server window, re/mpsrc).
extern int wrAppState;            // v12 0x0064FFA8 (feb 0x0060F018) 1 German, 2 no blood, 4 OEM ? (math_app/app.h)


// ------------------------------------------------------------------------------------------------ debug keys
// Game input command ids (wrINP_CTX_GAME::GetCmdName v12 0x004A8040, jump table 0x004A8568; feb 0x00499F20).
// 1.2 (and 1.0) inserted CHAT_MSG_ALL/TEAM at 0x2C/0x2D, so DBG_n is 0x2D+n in 1.2 and 0x2B+n in feb.
// Only 1.2 has handlers: plrPLAYER::ProcessInput v12 0x0041F790, block 0x00420277..0x0042089C. Keys must be bound
// in the input config (context "GAME", names DBG_1..DBG_9). Details and feb/jun enabling recipe:
// re/results/build_differences.md section 5.
enum wrDBG_CMD {
    WR_CMD_CHAT_ALL = 0x2C, WR_CMD_CHAT_TEAM = 0x2D,
    WR_CMD_DBG_1 = 0x2E,  // toggle [Debug] Decomp_OFF_AI (AI off)                                   v12 0x00420827
    WR_CMD_DBG_2 = 0x2F,  // no handler found                                                         // ?
    WR_CMD_DBG_3 = 0x30,  // GOD MODE: player flags ^= 4 (v12 plr+0x1F3); HUD shows UI_HUD_GOD           v12 0x004202B8
    WR_CMD_DBG_4 = 0x31,  // revive + full refill (Revitalize), only when gsAppState & 0x8000 is clear   v12 0x004205A3
    WR_CMD_DBG_5 = 0x32,  // toggle every AI spawn domain, cycle player skin 0..5                         v12 0x0042062A
    WR_CMD_DBG_6 = 0x33,  // KILL ALL NPCs (damage type 2, 5000)                                          v12 0x0042071D
    WR_CMD_DBG_7 = 0x34,  // toggle AI debug switch (v12 global 0x0064C928)                               v12 0x00420300
    WR_CMD_DBG_8 = 0x35,  // suicide (self damage 300) / drop TH treasures                               v12 0x004204DC
    WR_CMD_DBG_9 = 0x36,  // gsAppState ^= 0x8000 "release lock": unlocks SWITCH_MODE noclip (cmd 0x0C), DBG_4,
                          //   the debug HUD (wrh_dbg_msg) and Alt+Enter                                    v12 0x00420817
};
// Other UI-relevant commands: 0x10 USE (also "continue" on level complete), 0x27 SHOWFRAGS (score tables),
// 0x28 QUICKSAVE, 0x29 QUICKLOAD (wrUI_SYSTEM::Update, SP only), 0x2B SCREENSHOT, 0x39 the UI/menu key (Esc).
// Never-changed bit: WinMain sets gsAppState |= 0x8000 (v12 0x004AA3CD) before the game loop.

// ------------------------------------------------------------------------------------------------ localisation
// All user-visible text goes through gsSTRINGS (gsStrings, v12 0x009946DC): GetStringId(key) -> id,
// GetStringById(id) -> UTF-16, Printf(key, ...) (v12 0x0051C760). Keys used by the frontend (v12 code, full list
// with call sites in re/work/sdk_frontend/strrefs_v12.txt):
//   menus:     UI_OK UI_CANCEL UI_YES UI_NO UI_SYES UI_SNO UI_ON UI_OFF UI_LOW UI_HIGH UI_BACK UI_ERROR
//              UI_MSGCAP_QUIT UI_MSGTEXT_QUIT_ACK UI_MSGCAP_STOP_GAME UI_MSGTEXT_STOP_GAME_ACK
//   save/load: UI_QUICKSAVE UI_QUICKLOAD UI_AUTOSAVE UI_SAVEDATEFMT UI_SAVE_CONFIRM(_CAP) UI_DELETE_CONFIRM(_CAP)
//              UI_ERR_SAVING UI_ERR_CANT_LOAD_GAME GAME_MSG_GAME_SAVED GAME_MSG_GAME_LOADED GAME_MSG_SAVE_SCREENSHOT
//   options:   UI_AUTOWPN_%i UI_CUSOR_OFF UI_SKIN_%i UI_VIDEO_OPT_%i UI_VIDEO_LOD_%i UI_VIDEO_DETTEX_%i UI_VIDEO_BUMP_%i
//              UI_VIDEO_PROJSHD_%i UI_CTRL_CMD_%s UI_CTRL_NONE UI_CTRL_OR UI_CTRL_DEFAULTS_CAP UI_CTRL_DEFAULTS_CONFIRM
//   HUD:       UI_HUD_GOD UI_HUD_ZOOM UI_HUD_PING UI_HUD_MP_TEAM_%i UI_HUD_MP_TOTAL_DM_FL UI_HUD_MP_TOTAL_DM_FLTL
//              UI_HUD_MP_TOTAL_TH_TL UI_HUD_MP_TOTAL_TREASURES UI_HUD_SP_MAP_{TIME,KILLS,SECRETS,MONEY,ACCURACY}_FMT
//   game:      GAME_LEVEL_COMPLETE (1.0+) DEMO_OVER (feb only) GAME_MSG_TIMELEFT GAME_MSG_FRAGSLEFT GAME_MSG_TREASURESLEFT
//              GAME_MSG_PLAYER_WIN GAME_MSG_TEAM_WIN GAME_PLAYER_JOINED GAME_PLAYER_DISCONNECTED GAME_OVER_SERVER
//              GAME_OVER_CLIENT GAME_MSG_LADDER_DATA_SENDING/SENT/SENT_FAIL
//   levels:    LEVEL_NAME_%i MISSION_NAME_%i UI_SPLEVEL_SUBLEVEL_FMT (1.2) GAME_MP_MAP_%s GAME_MP_MODE_FULL_%s GAME_MP_MODE_ABB_%s
//   credits:   UI_CREDITS_STRING_%i_%i
//   MP/net:    UI_ERR_* UI_NET_STATUS_%s UI_CONNECTION_%i UI_DEDIC_SERVERMSG UI_DEDIC_SERVER_STATUS_MSG UI_ADVSERV_*

#pragma pack(pop)
