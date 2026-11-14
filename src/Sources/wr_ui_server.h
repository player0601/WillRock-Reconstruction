// wr_ui_server.h -- Will Rock 1.2 (v12.exe): "create_server" (advanced server options)
// window and the "mp_wait_server" wait window.  Reconstructed documentation source;
// NOT meant to compile into the game.
//
// Source object: wr_ui_server.obj (feb 0x00490E20..0x0049225F, v12 0x0049CE60..0x0049E97F).
//
// Types assumed from other files: everything listed at the top of wr_ui_mult.h, plus
//   wrUI_WINDOW_SCROLL<wrUI_ADVSERV_RECORD, wrUI_ADVSERV_INFO> (wr_ui_ctrl.cpp template):
//       OnKeyboard v12 0x0048ED10, ProcessScroll 0x0048F1D0, UpdateContext 0x00490C50,
//       OnChildNotify 0x00490D10, ProcessUpdate 0x00490870 (slot +0x44), OnScrollNotify 0x00490A50 (slot +0x60)
//   wrUI_TEXT_SCROLL (base of wrUI_ADVSERV_INFO), wrUI_STRINGS (base of wrUI_ADVSERV_RECORD)
//   gsMP_SERVER_INFO (gs_mp_network.h)

#pragma once
#include "wr_sdk.h"   // engine core types + subsystem classes (re/sdk); was "assumed from other files"

#include "wr_ui_mult.h"

// ---------------------------------------------------------------------------
// Game modes as the server menu numbers them (index into wrUI_MP_MODE_NAMES)
// ---------------------------------------------------------------------------
enum wrMP_MODE
{
    WRMP_SINGLE    = 0,     // "SINGLE"  (not offered)
    WRMP_DM        = 1,     // "DM"      deathmatch
    WRMP_COOP      = 2,     // "COOP"    cooperative (the single-player episodes)
    WRMP_TREASHUNT = 3,     // "TH"      treasure hunt   1.2: name "TH" (feb "TREASHUNT")
};
// v12 0x00605B64 (feb 0x005DA64C): { "SINGLE", "DM", "COOP", "TH" }
extern const char* wrUI_MP_MODE_NAMES[4];

// ---------------------------------------------------------------------------
// 1.2: the map ("location") table.  feb had one entry {"lev_mpdm_1_rotunda", 2};
// 1.2 has 16 entries of up to 8 maps each, built by the static initialiser
// v12 0x0049CE60 (feb _$E35 0x00490E20).
// ---------------------------------------------------------------------------
struct wrUI_LOCATION
{
    const char* maps[8];    // +0x00  level file names, NULL-terminated list; maps[0] = start map
    int         modeMask;   // +0x20  bit (1 << wrMP_MODE): modes this entry is offered for
};                          // 0x24 bytes

// v12 0x0064F8F8 .. 0x0064FB37 (16 entries)  (feb 0x0060EE68, 1 entry of 8 bytes {name, mask})
//   [0..9]  episodes 1..10 (lev_1_1_outdoor .. lev_10_4_zeus)      mask 4      = COOP
//   [10..15] lev_mpdm_1_rotunda, _2_city, _3_quarry, _4_cave, _5_lom, _6_towers
//                                                                   mask 0x0A   = DM | TH
extern wrUI_LOCATION wrUI_Locations[16];
#define WRUI_NUM_LOCATIONS 16

// small ctor-like helper used by the initialiser: v12 0x0049D0E0
// wrUI_LOCATION::Set(const char* map0, int modeMask, const char* map1, ... const char* map7)  // ? name

// ---------------------------------------------------------------------------
// Server "main data": the blob the host publishes with Server_SetMainData and that
// browsers read with Browser_GetServerMainData (wr_ui_join.cpp).  1.2: 0x30 bytes (feb 0x34).
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct wrMP_MAIN_DATA
{
    int     dataVersion;    // +0x00  0x8D (141)        (feb 0x7B)
    char    level[0x20];    // +0x04  start map, e.g. "lev_mpdm_1_rotunda"
    char    mode[5];        // +0x24  "DM" / "COOP" / "TH"  (feb char[0x10])
    unsigned short _29;     // +0x29  1  // ? meaning unknown (protocol sub-version?)
    unsigned char dedicated;// +0x2B  1.2: (gsAppState >> 27) & 1
    unsigned char _2C[4];   // +0x2C  0
};
#pragma pack(pop)

// ---------------------------------------------------------------------------
// one option row: a wrUI_STRINGS choice list plus label and id.  0x24 bytes.
// ---------------------------------------------------------------------------
struct wrUI_ADVSERV_RECORD : public wrUI_STRINGS        // +0x00..+0x17 wrUI_STRINGS (+0x0C entries {text,value}[], +0x10 count)
{
    int     sel;            // +0x18  selected entry
    int     labelId;        // +0x1C  string id of the row label (1 = none)
    int     id;             // +0x20  four-char row id, see ADVSERV_ROW_*

    wrUI_ADVSERV_RECORD();  // v12 0x0049D2D0  (feb 0x00490FE0)
};

// row ids (wrUI_ADVSERV_RECORD::id) and the window field they feed (ResetList)
#define ADVSERV_ROW_MODE     'MODE'   // 0x4D4F4445 -> new mode (triggers a rebuild)
#define ADVSERV_ROW_MAP      'MAP'    // 0x004D4150 -> location          (+0xA14)
#define ADVSERV_ROW_SUBLOC   'SUBL'   // 0x5355424C -> sub-location - 1  (+0xA18)  1.2: new
#define ADVSERV_ROW_LADDER   'LADD'   // 0x4C414444 -> useLadder         (+0xA40)
#define ADVSERV_ROW_MAXPL    'MXPL'   // 0x4D58504C -> maxPlayers        (+0xA1C)
#define ADVSERV_ROW_TIME     'TIME'   // 0x54494D45 -> timeLimit (min)   (+0xA20)
#define ADVSERV_ROW_FRAG     'FRAG'   // 0x46524147 -> fragLimit         (+0xA24)
#define ADVSERV_ROW_GOAD     'GOAD'   // 0x474F4144 -> godAfterDeath (s) (+0xA28)
#define ADVSERV_ROW_DIFF     'DIFF'   // 0x44494646 -> difficulty        (+0xA2C)
#define ADVSERV_ROW_HEALTH   'HEAL'   // 0x4845414C -> allowHealth       (+0xA30)
#define ADVSERV_ROW_ARMOR    'ARMR'   // 0x41524D52 -> allowArmor        (+0xA34)
#define ADVSERV_ROW_AMMO     'UNLA'   // 0x554E4C41 -> unlimitedAmmo     (+0xA38)
#define ADVSERV_ROW_FF       'FF'     // 0x00004646 -> friendlyFire      (+0xA3C)
#define ADVSERV_ROW_ITEMS    'ITEM'   // 0x4954454D -> keepItems         (+0xA44)  1.2: new
#define ADVSERV_ROW_AUTOBAL  'AUTB'   // 0x41555442 -> autoBalanceTeams  (+0xA48)  1.2: new

// ---------------------------------------------------------------------------
// one visible row widget.  vtable shared with wrUI_VIDEO_INFO (Init/ClearRecord folded)
// ---------------------------------------------------------------------------
class wrUI_ADVSERV_INFO : public wrUI_TEXT_SCROLL
{
public:
    void SetRecord(wrUI_ADVSERV_RECORD* rec);           // v12 0x0049D300  (feb 0x00491010)
    void ClearRecord();                                 // v12 0x0049D370  (feb 0x00491080)
    virtual const ushort* GetStr(int index);            // v12 0x0049D390  (feb 0x004910A0)
    virtual void Init(objOBJ* obj);                     // = wrUI_VIDEO_INFO::Init (feb 0x004910F0)  // ? v12 address not pinned
    virtual void OnScrolled();                          // v12 0x0049D3E0  (feb 0x00493450, other obj): rec->sel = curIndex

    // +0x12C curIndex, +0x130 arrow elements[2] (wrUI_TEXT_SCROLL)
    wrUI_ADVSERV_RECORD* rec;      // +0x138 [integrator: was a layout comment]
    uiRENDER_ELEM*       caption;  // +0x13C
};

// ---------------------------------------------------------------------------
// "create_server" window.  vtable v12 0x006053A0
// ---------------------------------------------------------------------------
class wrUI_ADVSERV_WINDOW : public wrUI_WINDOW_SCROLL<wrUI_ADVSERV_RECORD, wrUI_ADVSERV_INFO>
{
public:
    virtual int  StartAppearing(int start);             // slot +0x20  v12 0x0049D530  (feb 0x00491240)
    virtual int  ProcessInit(void* data);               // slot +0x3C  v12 0x0049D410  (feb 0x00491120)
    virtual void OnChildNotify(uiCHILD_NOTIFY* n);      // slot +0x5C  v12 0x0049D560  (feb 0x00491270)

    // called by wrGO_SYSTEM at the end of a round (through uiSystem, see wr_go_mp.cpp)
    void GetNextLocation(char* level, int size);        // v12 0x0049E8B0  (feb 0x00492200)

protected:
    void LoadOptions();                                 // v12 0x0049D650  (feb 0x00491360)
    void SaveOptions(int startServer);                  // v12 0x0049D880  (feb 0x00491510)
    void ResetList();                                   // v12 0x0049DBA0  (feb 0x004917E0)
    void DoResetList();                                 // v12 0x0049E270  (feb 0x00491DA0)
    void AddNumRec(int id, const char* label, int value, int from, int to,
                   const char* extraStr, int extraValue);                  // v12 0x0049E0F0  (feb 0x00491C20)
    void __cdecl AddNumRec(int id, const char* label, int value,
                   const char* extraStr, int extraValue, ...);             // v12 0x0049E1D0  (feb 0x00491D00)  list ends with -1
    void AddYesNoRow(int id, const char* label, int value);   // not a real function: inlined block, written once for readability

    // layout (v12; offsets up to +0xA10 are unchanged from feb)
    //   +0x6C  int  selRow            (-1 = none)
    //   +0x74  int  topRow
    //   +0x78  int  nRecords
    //   +0x7C  wrUI_ADVSERV_RECORD records[64]      (0x24 each)
    //   +0x97C int  nRowWidgets
    //   +0x980 wrUI_ADVSERV_INFO* rowWidgets[32]    (children with flag 0x8000)
    //   +0xA00 wrUI_SCROLL_BAR* scrollBar           ("scrollbar" in parent)
    //   +0xA0C uiTEXT_INPUT* serverNameEdit         ("server_name", max 32 chars)
public:
    uiTEXT_INPUT* serverNameEdit;   // +0xA0C [integrator: was a layout comment]
    int  mode;              // +0xA10  wrMP_MODE, -1 = not built yet
    int  location;          // +0xA14  index into wrUI_Locations
    int  subLocation;       // +0xA18  1.2: NEW -- map index inside the location (COOP: start level)
    int  maxPlayers;        // +0xA1C  (feb +0xA18)  [Multiplayer] ServerMaxPlayers   default 4 (feb 8)
    int  timeLimit;         // +0xA20  (feb +0xA1C)  minutes; ini ServerTimeLimit is seconds (float)
    int  fragLimit;         // +0xA24  (feb +0xA20)  ServerFragLimit                  default 30
    int  godAfterDeath;     // +0xA28  (feb +0xA24)  ServerPlrGODAfterDeath (float s) default 2
    int  difficulty;        // +0xA2C  (feb +0xA28)  Difficulty                       default 1
    int  allowHealth;       // +0xA30  (feb +0xA2C)  ServerAllowHealth                default 1
    int  allowArmor;        // +0xA34  (feb +0xA30)  ServerAllowArmor                 default 1
    int  unlimitedAmmo;     // +0xA38  (feb +0xA34)  ServerPlrIsUnlimitAmmo           default 0
    int  friendlyFire;      // +0xA3C  (feb +0xA38)  ServerPlrIsFriendlyFire          default 1
    int  useLadder;         // +0xA40  (feb +0xA3C)  ServerUseLadderPatch (1.2 key; feb ServerUseLadder)
    int  keepItems;         // +0xA44  1.2: NEW  ServerItemOnPick "keep"/"delete"     default 0 ("delete")
    int  autoBalanceTeams;  // +0xA48  1.2: NEW  ServerAutoBalanceTeam                default 0
};

// wrUI_WAITSERVER_WND is declared in wr_ui_mult.h (its OnChildNotify lives in wr_ui_mult.obj).
