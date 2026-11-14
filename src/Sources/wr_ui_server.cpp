// wr_ui_server.cpp -- Will Rock 1.2 (v12.exe): "create_server" window (all the
// [Multiplayer] Server* ini keys, server info + main-data blob, Server_Start) and
// "mp_wait_server".  Reconstructed documentation source; NOT meant to compile.
// Types: wr_ui_server.h / wr_ui_mult.h.
//
// Original object: wr_ui_server.obj (feb 0x00490E20..0x0049225F, v12 0x0049CE60..0x0049E97F).
//
// Flow:  "setup_player" NEXT (mpMode 0/3) -> "create_server"
//        StartAppearing: LoadOptions (ini -> fields), mode = -1, ResetList (builds rows)
//        row changes: ResetList (rows -> fields, rebuild if mode/location changed)
//        CREA button: ResetList, SaveOptions(1) (fields -> ini, Server_Start, Server_SetMainData),
//                     "mp_wait_server" -> Server_IsStarted -> StartLoadingAction(1, 2, level)
//
// ini summary, section [Multiplayer] (read in LoadOptions unless noted):
//   key                     field   default  type   notes
//   ServerName              edit    "Unnamed" str   max 32 chars in the edit box
//   ServerUseLadderPatch    +0xA40  1        bool   Ubi.com ladder; forced 0 unless DM  (feb key ServerUseLadder, always 0)
//   ServerMaxPlayers        +0xA1C  4        int    list 2..16 (DM), 2..12 (TH), 2..6 (COOP)  (feb default 8)
//   ServerTimeLimit         +0xA20  0        float  seconds in the ini, minutes in the UI (x 1/60, x 60)
//   ServerFragLimit         +0xA24  30       int
//   ServerPlrGODAfterDeath  +0xA28  2        float  spawn protection, seconds
//   Difficulty              +0xA2C  1        int    COOP only (0..2)
//   ServerAllowHealth       +0xA30  1        int
//   ServerAllowArmor        +0xA34  1        int
//   ServerPlrIsUnlimitAmmo  +0xA38  0        int
//   ServerPlrIsFriendlyFire +0xA3C  1        int
//   ServerItemOnPick        +0xA44  "delete" str    1.2: "keep" -> 1 (COOP: items stay for every player)
//   ServerAutoBalanceTeam   +0xA48  0        int    1.2: TH only
//   ServerMode              +0xA10           str    SINGLE/DM/COOP/TH; read in ResetList, default DM
//   ServerLocation          (write only)     str    start map of the chosen location
//   ServerLocation_<mode>   (ResetList)      str    last map per mode; also the map-rotation start (wr_go_mp.cpp)
//   ServerPort              (SaveOptions)    int    default 7777 (0x1E61) when the key is missing
// The ladder-only keys of the Ubi.com service are not covered here.

#include "wr_ui_server.h"
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

const char* wrUI_MP_MODE_NAMES[4] = { "SINGLE", "DM", "COOP", "TH" };   // v12 0x00605B64

wrUI_LOCATION wrUI_Locations[16];                                      // v12 0x0064F8F8

// ===========================================================================
// static initialiser  v12 0x0049CE60  (feb _$E35 0x00490E20)
// Builds the location table.
// 1.2: feb only had { "lev_mpdm_1_rotunda", 1 << WRMP_DM }.
// ===========================================================================
static void wrUI_InitLocations()
{
    static const int COOP  = 1 << WRMP_COOP;                       // 4
    static const int DM_TH = (1 << WRMP_DM) | (1 << WRMP_TREASHUNT);  // 0x0A

    // entries 0..2 are written field by field, 3..15 through helper 0x0049D0E0
    wrUI_Locations[0]  = { { "lev_1_1_outdoor", "lev_1_2_vase_temple", "lev_1_3_rotunda", "lev_1_4_corridors", "lev_1_5_atrium" }, COOP };
    wrUI_Locations[1]  = { { "lev_2_1_amph", "lev_2_2_city", "lev_2_3_gallery", "lev_2_4_cages", "lev_2_5_coliseum" }, COOP };
    wrUI_Locations[2]  = { { "lev_3_1_plateau", "lev_3_2_altar", "lev_3_3_courtyard" }, COOP };
    wrUI_Locations[3]  = { { "lev_4_1_stonemine", "lev_4_2_plateau", "lev_4_3_hephaestus" }, COOP };
    wrUI_Locations[4]  = { { "lev_5_1_plateau", "lev_5_2_citadel", "lev_5_3_undeground" }, COOP };    // sic
    wrUI_Locations[5]  = { { "lev_6_1_cave", "lev_6_2_courtyard", "lev_6_3_temple", "lev_6_4_rat_room" }, COOP };
    wrUI_Locations[6]  = { { "lev_7_1_swings", "lev_7_2_forest", "lev_7_3_river", "lev_7_4_city", "lev_7_5_medusa" }, COOP };
    wrUI_Locations[7]  = { { "lev_8_1_altar", "lev_8_2_cages", "lev_8_3_lift", "lev_8_4_amph", "lev_8_5_press", "lev_8_6_cyl" }, COOP };
    wrUI_Locations[8]  = { { "lev_9_1_plateau", "lev_9_2_ruins", "lev_9_3_underwater", "lev_9_4_courtyard" }, COOP };
    wrUI_Locations[9]  = { { "lev_10_1_lava", "lev_10_2_battle", "lev_10_3_stairs", "lev_10_4_zeus" }, COOP };
    wrUI_Locations[10] = { { "lev_mpdm_1_rotunda" }, DM_TH };
    wrUI_Locations[11] = { { "lev_mpdm_2_city"    }, DM_TH };
    wrUI_Locations[12] = { { "lev_mpdm_3_quarry"  }, DM_TH };
    wrUI_Locations[13] = { { "lev_mpdm_4_cave"    }, DM_TH };
    wrUI_Locations[14] = { { "lev_mpdm_5_lom"     }, DM_TH };
    wrUI_Locations[15] = { { "lev_mpdm_6_towers"  }, DM_TH };
}

// helper: start map of (location, subLocation), with the range guard 1.2 inlines in
// SaveOptions (0x0049D8FF) and ResetList (0x0049DDA3).  // ? not a separate function in the binary
static const char* LocationMap(int loc, int sub)
{
    if (loc < 0 || loc > 16)                     // sic: 16 passes, one past the table end
        return wrUI_Locations[0].maps[0];
    const wrUI_LOCATION& L = wrUI_Locations[loc];
    if (sub < 0 || sub > 8)                      // sic: 8 passes, reads modeMask as a pointer
        return L.maps[0];
    while (L.maps[sub] == NULL)                  // step back to the last existing map
        sub--;
    return L.maps[sub];
}

// ===========================================================================
// wrUI_WAITSERVER_WND  ("mp_wait_server")
// ===========================================================================

// v12 0x0049D130  (feb 0x00490E40)
// On open: show the server status.
int wrUI_WAITSERVER_WND::StartAppearing(int start)
{
    if (start)
        SetStatusStr(gsSysNetwork->Server_GetStatus(), 0.0f);     // 0x0052F3A0
    return uiELEMENT::StartAppearing(start);
}

// v12 0x0049D160  (feb 0x00490E70)
// Per frame: when the DirectPlay host is up, read back the main data and start the
// level as server (StartLoadingAction(1, 2, level)); on error show
// UI_ERR_SERVER_REGISTER_ERROR and go back to "connection".
// 1.2: blob is 0x30 bytes with dataVersion 0x8D (feb 0x34 / 0x7B).
int wrUI_WAITSERVER_WND::ProcessUpdate(void* data)
{
    wrMP_MAIN_DATA md;
    int            size;

    memset(&md, 0, sizeof(md));
    md.dataVersion = 0x8D;
    md._29         = 1;

    if ((flags & 0x140) == 0) {
        if (gsSysNetwork->Server_IsStarted()) {                   // 0x0052F330
            size = 0x30;
            gsSysNetwork->Server_GetMainData(&md, &size);         // 0x0052F480
            ((wrUI_SYSTEM*)uiSystem)->StartLoadingAction(1, 2, md.level,
                    gsStrings->GetStringId("UI_ERR_CANT_START_GAME"));   // 0x0048CDF0
            return uiELEMENT::ProcessUpdate(data);
        }
        if (gsSysNetwork->Server_IsError()) {                     // 0x0052F360
            int id = gsStrings->GetStringIdFmt("UI_NET_STATUS_%s", gsSysNetwork->Server_GetStatus());
            const wchar_t* detail = id ? gsStrings->GetStringById(id)
                                       : gsStrings->AsciiToUnicode(gsSysNetwork->Server_GetStatus());
            const wchar_t* title = gsStrings->GetStringById(gsStrings->GetStringId("UI_ERR_SERVER_REGISTER_ERROR"));
            uiSystem->MessageBox(title, detail, 0, 0, 0, "connection");
            return uiELEMENT::ProcessUpdate(data);
        }
    }
    SetStatusStr(gsSysNetwork->Server_GetStatus(), 0.0f);
    return uiELEMENT::ProcessUpdate(data);
}

// ===========================================================================
// wrUI_ADVSERV_RECORD / wrUI_ADVSERV_INFO
// ===========================================================================

// v12 0x0049D2D0  (feb 0x00490FE0)
wrUI_ADVSERV_RECORD::wrUI_ADVSERV_RECORD()
{
    // wrUI_STRINGS part (+0x00..+0x14) zeroed
    sel     = 0;
    labelId = 1;
    id      = 0;
}

// v12 0x0049D300  (feb 0x00491010)
// Binds a row widget to a record: label, current choice, arrows.
void wrUI_ADVSERV_INFO::SetRecord(wrUI_ADVSERV_RECORD* rec)
{
    this->rec = rec;                                  // +0x138
    flags &= ~0x2800;                                 // visible, enabled
    caption->SetTextId(rec->labelId);                 // +0x13C
    curIndex = this->rec->sel;                        // +0x12C
    if (this->rec->count == 1 && arrows[0] != NULL)   // single choice: hide both arrows
        for (int i = 0; i < 2; i++)
            *(unsigned *)((char *)arrows[i] + 0x40) |= 8;   // [integrator] +0x40 of the arrow element, as reconstructed // ?
    UpdateArrows();                                   // 0x0049FB90
}

// v12 0x0049D370  (feb 0x00491080)  -- shared with wrUI_VIDEO_INFO::ClearRecord
void wrUI_ADVSERV_INFO::ClearRecord()
{
    rec = NULL;
    flags |= 0x2800;                                  // hidden, disabled
}

// v12 0x0049D390  (feb 0x004910A0)
// Text of choice 'index': the string, or the numeric value ("%i") when the entry has no text.
const ushort* wrUI_ADVSERV_INFO::GetStr(int index)
{
    if (rec == NULL)
        return L"";                                    // 0x0064F8B8
    const ushort* s = rec->GetStr(index);              // 0x004A05C0
    if (*s == 0) {
        swprintf((wchar_t*)apBuffer, L"%i", rec->entries[index].value);
        return (const ushort*)apBuffer;
    }
    return s;
}

// v12 0x0049D3E0 (vtable slot of wrUI_ADVSERV_INFO; feb 0x00493450 lives in another obj)
// The arrows changed the choice: store it in the record.
void wrUI_ADVSERV_INFO::OnScrolled()
{
    rec->sel = curIndex;
}

// ===========================================================================
// wrUI_ADVSERV_WINDOW  ("create_server")
// ===========================================================================

// v12 0x0049D410  (feb 0x00491120)
// Finds the controls, collects the row widgets (children with flag 0x8000) and
// sets up the scroll bar.
int wrUI_ADVSERV_WINDOW::ProcessInit(void* data)
{
    nRecords = 0;
    serverNameEdit = (uiTEXT_INPUT*)FindChildElement("server_name");
    serverNameEdit->SetMaxLen(0x20);                  // vt+0x6C (feb +0x64)
    topRow = 0;
    selRow = -1;
    scrollBar = (wrUI_SCROLL_BAR*)parent->FindChildElement("scrollbar");

    nRowWidgets = 0;                                  // first pass only counts (result discarded)
    for (uiELEMENT* c = firstChild; c; c = c->next)
        if (c->flags & 0x8000)
            nRowWidgets++;
    nRowWidgets = 0;
    for (uiELEMENT* c = firstChild; c; c = c->next)
        if (c->flags & 0x8000) {
            rowWidgets[nRowWidgets] = (wrUI_ADVSERV_INFO*)c;
            c->flags &= ~0x400;
            nRowWidgets++;
        }

    if (scrollBar) {
        scrollBar->flags &= ~0x400;
        scrollBar->flags &= ~0x4;
        scrollBar->flags &= ~0x8;
        int range = nRecords - nRowWidgets;
        if (range < 0) range = 0;
        scrollBar->SetWindow(this, range, 1);
        scrollBar->SetPos(0);
    }
    flags |= 0x40C;
    return 1;
}

// v12 0x0049D530  (feb 0x00491240)
int wrUI_ADVSERV_WINDOW::StartAppearing(int start)
{
    if (start) {
        LoadOptions();
        mode = -1;                                    // force a full rebuild
        ResetList();
    }
    return uiELEMENT::StartAppearing(start);
}

// v12 0x0049D560  (feb 0x00491270)
// CREA: commit the rows, save, start the server, wait.  Row value changed on the
// MODE or MAP row: rebuild.  Row focus: remember the selected row.
// 1.2: the rebuild test looks at the changed row's id (MODE or MAP) instead of feb's
//      "first visible row while scrolled to the top".
void wrUI_ADVSERV_WINDOW::OnChildNotify(uiCHILD_NOTIFY* n)
{
    if (n->type == UINOTIFY_BUTTON) {
        if (n->code == UIBTN_CREATE) {
            ResetList();
            SaveOptions(1);
            uiSystem->ShowWindow("mp_wait_server", 8);
            return;
        }
    }
    else if (n->type == UINOTIFY_SCROLLCHG && n->code == UINOTIFY_CHANGE) {
        wrUI_ADVSERV_RECORD* rec = ((wrUI_ADVSERV_INFO*)n->child)->rec;     // +0x138
        if (rec && (rec->id == ADVSERV_ROW_MODE || rec->id == ADVSERV_ROW_MAP))   // 1.2
            ResetList();
    }

    if (n->type != UINOTIFY_FOCUS) {
        uiELEMENT::OnChildNotify(n);
        return;
    }

    for (int i = 0; i < nRowWidgets; i++) {
        if (n->child == rowWidgets[i]) {
            int row = topRow + i;
            if (row < nRecords && n->code != 0) {
                if (nRecords != 0 && row != -1)
                    selRow = row < 0 ? 0 : row;
                else
                    selRow = -1;
            }
            return;
        }
    }
}

// v12 0x0049D650  (feb 0x00491360)
// ini -> fields (see the table at the top of this file).
// 1.2: new keys ServerUseLadderPatch, ServerItemOnPick, ServerAutoBalanceTeam;
//      ServerMaxPlayers default 8 -> 4; every field after +0xA14 moved +4.
void wrUI_ADVSERV_WINDOW::LoadOptions()
{
    char  buf[256];
    float f;

    if (!_apCfgReadString(NULL, "Multiplayer", "ServerName", buf, 0x100))
        strcpy(buf, "Unnamed");
    serverNameEdit->SetEditText(gsStrings->AsciiToUnicode(buf));  // vt+0x60

    location    = 0;
    subLocation = 0;                                              // 1.2

    useLadder = 1;                                                // 1.2 (feb: = 0, not read)
    _apCfgReadBool(NULL, "Multiplayer", "ServerUseLadderPatch", &useLadder);   // 0x004AF940

    maxPlayers = 4;                                               // 1.2: feb 8
    _apCfgReadInt(NULL, "Multiplayer", "ServerMaxPlayers", &maxPlayers);

    timeLimit = 0;
    if (_apCfgReadFloat(NULL, "Multiplayer", "ServerTimeLimit", &f))
        timeLimit = (int)(f * (1.0f / 60.0f));                    // const 0x005F9258

    fragLimit = 30;
    _apCfgReadInt(NULL, "Multiplayer", "ServerFragLimit", &fragLimit);

    godAfterDeath = 2;
    if (_apCfgReadFloat(NULL, "Multiplayer", "ServerPlrGODAfterDeath", &f))
        godAfterDeath = (int)f;

    difficulty = 1;
    _apCfgReadInt(NULL, "Multiplayer", "Difficulty", &difficulty);
    allowHealth = 1;
    _apCfgReadInt(NULL, "Multiplayer", "ServerAllowHealth", &allowHealth);
    allowArmor = 1;
    _apCfgReadInt(NULL, "Multiplayer", "ServerAllowArmor", &allowArmor);
    unlimitedAmmo = 0;
    _apCfgReadInt(NULL, "Multiplayer", "ServerPlrIsUnlimitAmmo", &unlimitedAmmo);
    friendlyFire = 1;
    _apCfgReadInt(NULL, "Multiplayer", "ServerPlrIsFriendlyFire", &friendlyFire);

    // 1.2: new
    keepItems = 0;
    if (_apCfgReadString(NULL, "Multiplayer", "ServerItemOnPick", buf, 0x100))
        keepItems = (memcmp(buf, "keep", 5) == 0);
    autoBalanceTeams = 0;
    _apCfgReadInt(NULL, "Multiplayer", "ServerAutoBalanceTeam", &autoBalanceTeams);
}

// v12 0x0049D880  (feb 0x00491510)
// fields -> ini; with startServer != 0 also fills gsMP_SERVER_INFO and the main-data
// blob and starts the DirectPlay host.
// 1.2: info.versionId = 10020000 (new field, gsMP_SERVER_INFO grew to 0x58);
//      blob 0x30 bytes, dataVersion 0x8D, +0x2B dedicated flag;
//      location is (location, subLocation); ladder kept only in DM;
//      writes the 1.2 keys.
void wrUI_ADVSERV_WINDOW::SaveOptions(int startServer)
{
    wrMP_MAIN_DATA   md;
    gsMP_SERVER_INFO info;
    char             name[256];
    int              port;

    memset(&md, 0, sizeof(md));
    md.dataVersion = 0x8D;                                        // feb 0x7B
    md._29         = 1;

    apCfgWriteString("Multiplayer", "ServerMode", wrUI_MP_MODE_NAMES[mode]);
    gsStrings->UnicodeToAscii(serverNameEdit->GetEditText(), name, 0x100);
    apCfgWriteString("Multiplayer", "ServerName", name);

    const char* level = LocationMap(location, subLocation);       // 1.2 (feb: wrUI_Locations[location].name)
    apCfgWriteString("Multiplayer", "ServerLocation", level);

    if (mode != WRMP_DM)                                          // 1.2: feb cleared it always
        useLadder = 0;
    apCfgWriteBool  ("Multiplayer", "ServerUseLadderPatch",  useLadder);
    apCfgWriteInt   ("Multiplayer", "ServerMaxPlayers",      maxPlayers);
    apCfgWriteFloat ("Multiplayer", "ServerTimeLimit",       (float)timeLimit * 60.0f);  // const 0x005FB65C
    apCfgWriteInt   ("Multiplayer", "ServerFragLimit",       fragLimit);
    apCfgWriteFloat ("Multiplayer", "ServerPlrGODAfterDeath",(float)godAfterDeath);
    apCfgWriteInt   ("Multiplayer", "Difficulty",            difficulty);
    apCfgWriteInt   ("Multiplayer", "ServerAllowHealth",     allowHealth);
    apCfgWriteInt   ("Multiplayer", "ServerAllowArmor",      allowArmor);
    apCfgWriteInt   ("Multiplayer", "ServerPlrIsUnlimitAmmo",unlimitedAmmo);
    apCfgWriteInt   ("Multiplayer", "ServerPlrIsFriendlyFire", friendlyFire);
    apCfgWriteString("Multiplayer", "ServerItemOnPick",      keepItems ? "keep" : "delete");  // 1.2
    apCfgWriteInt   ("Multiplayer", "ServerAutoBalanceTeam", autoBalanceTeams);              // 1.2

    if (!startServer)
        return;

    strncpy(info.name, name, 0x40);
    info.name[0x3F]  = 0;
    info.versionId   = 10020000;                                  // 1.2: new field
    info.maxPlayers  = maxPlayers;
    info.ladder      = useLadder ? mode + 1 : 0;
    if (!_apCfgReadInt(NULL, "Multiplayer", "ServerPort", &port))
        port = 7777;                                              // 0x1E61
    info.port        = port;
    info.curPlayers  = 0;
    info.ping        = 0;

    strncpy(md.level, level, 0x20);
    md.level[0x1F] = 0;
    strncpy(md.mode, wrUI_MP_MODE_NAMES[mode], 5);                // feb 0x10 bytes
    md.mode[4]     = 0;
    md.dedicated   = (gsAppState >> 27) & 1;                      // 1.2

    gsSysNetwork->Server_Start(&info);                            // 0x0052F290
    gsSysNetwork->Server_SetMainData(&md, 0x30);                  // 0x0052F470 (feb 0x34)
}

// v12 0x0049DBA0  (feb 0x004917E0)
// 1) If the list is built: copy every row's choice into its field, and save the
//    location as ServerLocation_<mode>.
// 2) Work out the wanted mode (the MODE row, else [Multiplayer] ServerMode, else DM).
// 3) If mode or location changed: look up ServerLocation_<mode> in the table to
//    preselect location/subLocation, DoResetList, re-bind the visible row widgets,
//    clamp the selection.
// 1.2: rows SUBL/LADD/ITEM/AUTB; location search walks all 16 entries and all maps
//      (feb: one entry); rebuild also when only the location changed.
void wrUI_ADVSERV_WINDOW::ResetList()
{
    char key[64];
    int  newMode   = -1;
    int  oldLocation = location;

    if (mode != -1) {
        wrUI_ADVSERV_RECORD* r = &records[0];
        for (int i = 0; i < nRecords; i++, r++) {
            int v = r->entries[r->sel].value;                     // {text,value} pairs, 8 bytes
            switch (r->id) {
            case ADVSERV_ROW_MODE:    newMode       = v;     break;
            case ADVSERV_ROW_MAP:     location      = v;     break;
            case ADVSERV_ROW_SUBLOC:  subLocation   = v - 1; break;   // 1.2 (row shows 1..n)
            case ADVSERV_ROW_LADDER:  useLadder     = v;     break;
            case ADVSERV_ROW_MAXPL:   maxPlayers    = v;     break;
            case ADVSERV_ROW_TIME:    timeLimit     = v;     break;
            case ADVSERV_ROW_FRAG:    fragLimit     = v;     break;
            case ADVSERV_ROW_GOAD:    godAfterDeath = v;     break;
            case ADVSERV_ROW_DIFF:    difficulty    = v;     break;
            case ADVSERV_ROW_HEALTH:  allowHealth   = v;     break;
            case ADVSERV_ROW_ARMOR:   allowArmor    = v;     break;
            case ADVSERV_ROW_AMMO:    unlimitedAmmo = v;     break;
            case ADVSERV_ROW_FF:      friendlyFire  = v;     break;
            case ADVSERV_ROW_ITEMS:   keepItems     = v;     break;   // 1.2
            case ADVSERV_ROW_AUTOBAL: autoBalanceTeams = v;  break;   // 1.2
            }
        }
        sprintf(key, "ServerLocation_%s", wrUI_MP_MODE_NAMES[mode]);
        apCfgWriteString("Multiplayer", key, LocationMap(location, subLocation));
    }

    if (newMode == -1) {
        _apCfgReadString(NULL, "Multiplayer", "ServerMode", key, 0x40);
        for (newMode = 0; newMode < 4; newMode++)
            if (strcmp(key, wrUI_MP_MODE_NAMES[newMode]) == 0)
                break;
        if (newMode == 4)
            newMode = WRMP_DM;
        // note: "SINGLE" in the ini is accepted and gives mode 0
    }

    if (mode == newMode && location == oldLocation)               // 1.2: also compares the location
        return;

    mode = newMode;
    sprintf(key, "ServerLocation_%s", wrUI_MP_MODE_NAMES[mode]);
    if (!_apCfgReadString(NULL, "Multiplayer", key, apBuffer, 0x1000))
        apBuffer[0] = 0;

    // find the saved map; default = first location offered for this mode
    int mask  = 1 << mode;
    int first = -1;
    int loc, sub = 0;
    for (loc = 0; loc < WRUI_NUM_LOCATIONS; loc++) {
        if (!(wrUI_Locations[loc].modeMask & mask))
            continue;
        if (first == -1)
            first = loc;
        for (sub = 0; sub < 8 && wrUI_Locations[loc].maps[sub]; sub++)
            if (strcmp(wrUI_Locations[loc].maps[sub], apBuffer) == 0)
                goto found;
    }
    location    = (first == -1) ? 0 : first;
    subLocation = 0;
    goto build;
found:
    location    = loc;
    subLocation = sub;
build:
    DoResetList();

    if (scrollBar) {
        int range = nRecords - nRowWidgets;
        if (range < 0) range = 0;
        scrollBar->SetWindow(this, range, 1);
    }

    int nVisible = nRecords - topRow;
    if (nVisible > nRowWidgets)
        nVisible = nRowWidgets;
    int i = 0;
    for (; i < nVisible; i++) {
        rowWidgets[i]->Enable(1);                                 // vt+0x10
        rowWidgets[i]->SetRecord(&records[topRow + i]);           // inlined
    }
    for (; i < nRowWidgets; i++) {
        rowWidgets[i]->ClearRecord();                             // inlined
        rowWidgets[i]->Enable(0);
    }

    if (nRecords != 0 && selRow != -1)
        selRow = (selRow >= nRecords) ? nRecords - 1 : (selRow < 0 ? 0 : selRow);
    else
        selRow = -1;
}

// v12 0x0049E0F0  (feb 0x00491C20)
// Adds a numeric row with values from..to-1 (optionally preceded by one named
// entry 'extraStr' = 'extraValue'); selects 'value' if it is in range.  Unchanged.
void wrUI_ADVSERV_WINDOW::AddNumRec(int id, const char* label, int value, int from, int to,
                                    const char* extraStr, int extraValue)
{
    wrUI_ADVSERV_RECORD* r = &records[nRecords];
    r->Clear();
    r->labelId = gsStrings->GetStringId(label);
    r->id      = id;
    int hasExtra = (extraStr != NULL);
    r->PreInit(hasExtra - from + to, 0);
    if (extraStr)
        r->AddString(extraStr, extraValue);
    for (int v = from; v < to; v++)
        r->AddString(1, v);                                       // id 1 = no text -> GetStr prints "%i"
    r->Compact();

    if ((extraStr == NULL || value != extraValue) && value >= from && value < to)
        r->sel = hasExtra - from + value;
    else
        r->sel = 0;
    nRecords++;
}

// v12 0x0049E1D0  (feb 0x00491D00)
// __cdecl, 'this' on the stack.  Row with an explicit value list ending in -1.  Unchanged.
void __cdecl wrUI_ADVSERV_WINDOW::AddNumRec(int id, const char* label, int value,
                                            const char* extraStr, int extraValue, ...)
{
    wrUI_ADVSERV_RECORD* r = &records[nRecords];
    r->Clear();
    r->labelId = gsStrings->GetStringId(label);
    r->id      = id;
    r->sel     = 0;
    if (extraStr)
        r->AddString(extraStr, extraValue);

    va_list ap;
    va_start(ap, extraValue);
    for (int v = va_arg(ap, int); v != -1; v = va_arg(ap, int)) {
        r->AddString(1, v);
        if (value == v)
            r->sel = r->count - 1;
    }
    va_end(ap);
    r->Compact();
    nRecords++;
}

// v12 0x0049E270  (feb 0x00491DA0)
// Builds the option rows for the current mode.
// 1.2: the MODE row lists DM, COOP and TH (feb: DM only); the MAP row lists every
//      location of the mode (feb: one); new rows SUBLOCATION (COOP), LADDER (DM,
//      Ubi.com driver only), KEEPITEMS (COOP), AUTOBALANCE_TEAMS (TH).
//
//   row                          modes        values
//   UI_ADVSERV_GAME_MODE         all          GAME_MP_MODE_FULL_DM/_COOP/_TH (values 1..3)
//   UI_ADVSERV_LOCATION          all          GAME_MP_MAP_<map0> (or the raw name) per location
//   UI_ADVSERV_SUBLOCATION       COOP         1..n maps of the location  (1.2)
//   UI_ADVSERV_LADDER            DM + ladder  no/yes                      (1.2 row; Ubi.com only)
//   UI_ADVSERV_MAX_PLAYERS       all          2..6 COOP, 2..12 TH, 2..16 DM
//   UI_ADVSERV_GOAD              DM, TH       none,1,2,3,4,5 s
//   UI_ADVSERV_TIMELIMIT         DM, TH       none,1,2,3,5,10,15,20,30,45,60,90,120,180 min
//   UI_ADVSERV_FRAGLIMIT         DM           none,1,5,10,15,20,30,50,100,150,200
//   UI_ADVSERV_DIFFICULTY        COOP         UI_ADVSERV_DIFF_0..2
//   UI_ADVSERV_FRIENDLY_FIRE     COOP, TH     no/yes
//   UI_ADVSERV_ALLOW_HEALTH/ARMOR, UNLIMITED_AMMO   DM, TH   no/yes
//   UI_ADVSERV_KEEPITEMS         COOP         UI_ADVSERVER_ITEMS_ONE / _ALL   (1.2)
//   UI_ADVSERV_AUTOBALANCE_TEAMS TH           no/yes                          (1.2)
void wrUI_ADVSERV_WINDOW::DoResetList()
{
    wrUI_ADVSERV_RECORD* r;
    nRecords = 0;
    if (mode == -1)
        return;

    // MODE
    r = &records[0];
    nRecords = 1;
    r->Clear();
    r->labelId = gsStrings->GetStringId("UI_ADVSERV_GAME_MODE");
    r->id      = ADVSERV_ROW_MODE;
    r->PreInit(3, 0x80);
    for (int m = WRMP_DM; m < 4; m++)                             // 1.2: feb added only DM and forced mode = DM
        r->AddString(gsStrings->GetStringIdFmt("GAME_MP_MODE_FULL_%s", wrUI_MP_MODE_NAMES[m]), m);
    r->Compact();
    r->sel = mode - 1;

    // MAP
    r = &records[nRecords++];
    r->Clear();
    r->labelId = gsStrings->GetStringId("UI_ADVSERV_LOCATION");
    r->id      = ADVSERV_ROW_MAP;
    r->PreInit(0x10, 0);                                          // feb (1, 0)
    r->sel     = 0;
    for (int loc = 0; loc < WRUI_NUM_LOCATIONS; loc++) {
        if (!(wrUI_Locations[loc].modeMask & (1 << mode)))
            continue;
        int id = gsStrings->GetStringIdFmt("GAME_MP_MAP_%s", wrUI_Locations[loc].maps[0]);
        if (id)
            r->AddString(id, loc);
        else
            r->AddString(gsStrings->AsciiToUnicode(wrUI_Locations[loc].maps[0]), loc);   // 0x004A04D0
        if (loc == location)
            r->sel = r->count - 1;
    }
    r->Compact();

    // SUBLOCATION (1.2): COOP and the location has more than one map
    if (mode == WRMP_COOP && wrUI_Locations[location].maps[1] != NULL) {
        int n = 0;
        while (n < 8 && wrUI_Locations[location].maps[n])
            n++;
        AddNumRec(ADVSERV_ROW_SUBLOC, "UI_ADVSERV_SUBLOCATION", subLocation + 1, 1, n + 1, NULL, 0);
    }

    // LADDER (1.2 row): DM with a driver that supports the Ubi.com ladder
    if (mode == WRMP_DM && gsSysNetwork->Drv_IsLadderSupported()) {   // 0x0052F1D0
        /* Ubi.com service: not reconstructed (dead service / licensing) -- no/yes row
           'LADD' "UI_ADVSERV_LADDER" bound to useLadder */
    }

    // MAX PLAYERS
    int maxTo = (mode == WRMP_COOP) ? 7 : (mode != WRMP_TREASHUNT ? 17 : 13);
    AddNumRec(ADVSERV_ROW_MAXPL, "UI_ADVSERV_MAX_PLAYERS", maxPlayers, 2, maxTo, NULL, 0);

    if (mode != WRMP_COOP) {
        AddNumRec(ADVSERV_ROW_GOAD, "UI_ADVSERV_GOAD", godAfterDeath,
                  "UI_ADVSERV_GOAD_NONE", 0, 1, 2, 3, 4, 5, -1);
        AddNumRec(ADVSERV_ROW_TIME, "UI_ADVSERV_TIMELIMIT", timeLimit,
                  "UI_ADVSERV_TIMELIMIT_NONE", 0, 1, 2, 3, 5, 10, 15, 20, 30, 45, 60, 90, 120, 180, -1);
    }
    if (mode == WRMP_DM)
        AddNumRec(ADVSERV_ROW_FRAG, "UI_ADVSERV_FRAGLIMIT", fragLimit,
                  "UI_ADVSERV_FRAGLIMIT_NONE", 0, 1, 5, 10, 15, 20, 30, 50, 100, 150, 200, -1);

    if (mode == WRMP_COOP) {
        r = &records[nRecords++];
        r->Clear();
        r->labelId = gsStrings->GetStringId("UI_ADVSERV_DIFFICULTY");
        r->id      = ADVSERV_ROW_DIFF;
        r->PreInit(3, 0);
        for (int d = 0; d < 3; d++)
            r->AddString(gsStrings->GetStringIdFmt("UI_ADVSERV_DIFF_%i", d), d);
        r->Compact();
        r->sel = difficulty;
    }

    if (mode == WRMP_COOP || mode == WRMP_TREASHUNT)
        AddYesNoRow(ADVSERV_ROW_FF, "UI_ADVSERV_FRIENDLY_FIRE", friendlyFire);   // inlined, see below

    if (mode != WRMP_COOP) {
        AddYesNoRow(ADVSERV_ROW_HEALTH, "UI_ADVSERV_ALLOW_HEALTH",   allowHealth);
        AddYesNoRow(ADVSERV_ROW_ARMOR,  "UI_ADVSERV_ALLOW_ARMOR",    allowArmor);
        AddYesNoRow(ADVSERV_ROW_AMMO,   "UI_ADVSERV_UNLIMITED_AMMO", unlimitedAmmo);
    }
    if (mode == WRMP_COOP) {                                      // 1.2
        r = &records[nRecords];
        r->Clear();
        r->labelId = gsStrings->GetStringId("UI_ADVSERV_KEEPITEMS");
        r->id      = ADVSERV_ROW_ITEMS;
        r->PreInit(2, 0x10);
        r->AddString("UI_ADVSERVER_ITEMS_ONE", 0);
        r->AddString("UI_ADVSERVER_ITEMS_ALL", 1);
        r->Compact();
        r->sel = keepItems;
        nRecords++;
    }
    if (mode == WRMP_TREASHUNT)                                   // 1.2
        AddYesNoRow(ADVSERV_ROW_AUTOBAL, "UI_ADVSERV_AUTOBALANCE_TEAMS", autoBalanceTeams);
}

// AddYesNoRow is not a function in the binary: every yes/no row is this block, inlined:
//     r = &records[nRecords]; r->Clear(); r->labelId = GetStringId(label); r->id = id;
//     r->PreInit(2, 0x10); r->AddString("UI_SNO", 0); r->AddString("UI_SYES", 1);
//     r->Compact(); r->sel = value; nRecords++;

// v12 0x0049E8B0  (feb 0x00492200)
// Map rotation: called at the end of a round (wr_go_mp.cpp).  Advances 'location' to
// the next table entry offered for the current mode (wrapping at 16), returns its
// first map and republishes the main-data blob with the new level.
// 1.2: rewritten.  feb looped on the mode bit and always returned location 0
//      (i.e. never rotated) and did not touch the main data.
void wrUI_ADVSERV_WINDOW::GetNextLocation(char* level, int size)
{
    wrMP_MAIN_DATA md;
    int            mdSize;

    memset(&md, 0, sizeof(md));
    md.dataVersion = 0x8D;
    md._29         = 1;

    int mask = 1 << mode;
    int loc  = location;
    do {
        loc = (loc + 1) & 0xF;                                    // 16 entries
    } while (!(wrUI_Locations[loc].modeMask & mask) && loc != location);
    location = loc;
    // note: subLocation is not reset; COOP rotates whole episodes and starts at maps[0]

    strncpy(level, wrUI_Locations[loc].maps[0], size);
    level[size - 1] = 0;

    mdSize = 0x30;
    gsSysNetwork->Server_GetMainData(&md, &mdSize);
    strncpy(md.level, level, 0x20);
    md.level[0x1F] = 0;
    gsSysNetwork->Server_SetMainData(&md, 0x30);
}

// _$E2 of wr_ui_server.obj (v12 0x0049D3F0, feb 0x004917C0): g_float_0x64F8F0 = c*c (header constant, unused here)
