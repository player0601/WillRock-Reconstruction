// gs_common.h -- Will Rock 1.2 unofficial SDK, subsystem "game_systems": shared globals, ids, small structs
//
// Covers objs (feb map): gs:gs_init (globals only; the app loop is in re/sdk/math_app/app.h), gs:gs_msg (ids),
//   and the bits every other header of this folder uses.
// Assumed from re/sdk/core (engine_core): msgADDR, msgSYSTEM msgSystem, msgDATA, apNAME, arrVector<T>, psSHEET,
//   objOBJ, animINST, animTPL, scnSCENE, scnDOMAIN, camCAMERA, entIACTIVE_OBJ, entENTITY, m3dV, m3dMATR, m3dBOX,
//   fioFILE / fioFILE_MEM / fioFILE_DISK, sncCHANNEL (sound), txmTEXTURE.
//
// Conventions: "v12 +0xNN (feb +0xNN)" = field offset in 1.2 and in the Feb-2003 build. "// ?" = not verified by an
//   instruction in v12. Every function line gives the v12 address first. All __thiscall unless said otherwise;
//   "__fastcall" free functions take args in ecx/edx (MSVC /Gr build: all free gs functions are fastcall).
#pragma once
#include "../core/core_types.h"   // msgADDR, msgDATA, apNAME, arrVector, m3dV, m3dMATR (integrator)

typedef int BOOL32;
class scnSCENE; class camCAMERA; class gsAPP_SYSTEM; class gsLVL_SYSTEM; class gsRENDER_SYSTEM; class gsINP_SYSTEM;
class gsSND_SYSTEM; class gsVIS_SYSTEM; class cinSYSTEM; class gsSTRINGS; class gslSL; class mrrMIRROR_SYSTEM;
class gsIFACE_SYSTEM; class gsCUTSCENE; class gsDEMO_CLIENT : public msgADDR { };  // [integrator] demo playback pseudo-client; layout not reconstructed
 class gsDEMO_SERVER : public msgADDR { };  // [integrator] demo playback pseudo-server; layout not reconstructed
class gsDEMO {                                        // gs_demo.obj: demo recorder/player, global object gsDemo
public:
    int  Init();                                      // feb 0x00529400
    void Term();                                      // feb 0x00529450
    int  Record_Start();                              // v12 0x0053F1E0 (re/mpsrc) (feb 0x00529590)
    void Record_SaveMsg(void *wrap);                  // v12 0x0053F4F0 (re/mpsrc) (feb 0x005298A0) arg gsMSG_MP_WRAP*
    int  Play_Start(char *file);                      // feb 0x00529910
    void Play_DispatchFrame();                        // feb 0x00529AF0
};
extern gsDEMO         gsDemo;                         // feb 0x0095D090; v12 ?
void __fastcall gsVideoTermHW();                      // v12 0x00533E50 (re/mpsrc) (feb 0x0051E590) shut the video driver down
class domTPT_DOMAIN_LIST; class domACT_DOMAIN_LIST; class domCLN_DOMAIN_LIST; class domKILL_DOMAIN_LIST;
class gsDOMAIN_LIST;

// =====================================================================================================
// Globals (all verified in v12 by a load/store instruction; app-level ones are also in math_app/app.h)
// =====================================================================================================
extern char             gsSceneFullName[0x104]; // v12 0x0098E718 (feb 0x00948118) path of the loaded level (__gsLoadLevel copies param->level here)
extern scnSCENE        *gsScenePtr;       // v12 0x0098E81C (feb 0x0094821C) THE scene (new scnSCENE(0x8C) in gsLVL_SYSTEM::Init)
extern mrrMIRROR_SYSTEM*gsSysMirror;      // v12 0x0098E820 (feb 0x00948220)
// extern int              gsAppState;       // v12 0x0098E824 (feb 0x00948224) GSAPP_* flags (app.h); bits used here listed below   [integrator: declared once in core/wr_core.h / engine_core/core_types.h]
extern camCAMERA       *gsCameraPtr;      // v12 0x0098E828 (feb 0x00948228)
extern gsSND_SYSTEM    *gsSysSound;       // v12 0x0098E830 (feb 0x00948230) object is a wrSND_SYSTEM (size v12 0xF4, feb 0xF8)
extern gsIFACE_SYSTEM  *gsSysIFace;       // v12 0x0098E834 (feb 0x00948234)  // ? unused in 1.2
extern gsINP_SYSTEM    *gsSysInput;       // v12 0x0098E838 (feb 0x00948238) object is a wrINP_SYSTEM (0x70)
extern gsVIS_SYSTEM    *gsSysVis;         // v12 0x0098E83C (feb 0x0094823C) (0x3D4, feb 0x3D0)
extern cinSYSTEM       *gsSysCinema;      // v12 0x0098E840 (feb 0x00948240) created by cinInit (0xCE0, feb 0xCDC)
extern gsRENDER_SYSTEM *gsSysRender;      // v12 0x0098E844 (feb 0x00948244) object is a wrRENDER_SYSTEM (0x3C)
extern gsAPP_SYSTEM    *gsAppSystem;      // v12 0x0098E848 (feb 0x00948248)
extern gsLVL_SYSTEM    *gsSysLevel;       // v12 0x0098E84C (feb 0x0094824C) object is a wrLVL_SYSTEM (0x3D90)
extern float            gsElapsedTime;    // v12 0x0098E9F4 (feb 0x009482DC) frame delta (s)
extern int              _eioIsUpdSuspend; // v12 0x0098EA0C (feb 0x009482F4) IAction execution suspended (ExecuteIAction tests it)
// extern unsigned         gsTime;           // v12 0x0098EA14 (feb 0x009482FC)   [integrator: declared once in core/wr_core.h / engine_core/core_types.h]
extern float            gsElapsedTimeLevel; // v12 0x0098EA1C (feb 0x00948304) level clock, reset by GS_MSG_INIT_MAP (0x3F1)
extern gsSTRINGS       *gsStrings;        // v12 0x009946DC (feb 0x0094DE5C) object is a wrSTRINGS (v12 0x30, feb 0x34)
extern gslSL            gslSaveLoad;      // v12 0x0099D828 (feb 0x00956FA8) the save/load object (instance, not pointer)
extern gsCUTSCENE      *gspCutscene;      // v12 0x009A57CC (feb 0x0095CC3C) wrCUTSCENE (video cut-scenes, see README)
extern gsDEMO_CLIENT   *gsDemoClient;     // v12 0x009A8460 (feb 0x0095D5B8) demo playback pseudo-client
extern gsDEMO_SERVER   *gsDemoServer;     // v12 0x009A8464 (feb 0x0095D5BC)  // ? (data-map only)
extern domCLN_DOMAIN_LIST  *domClnList;   // v12 0x009AE7A4 (feb 0x00963450)
extern domACT_DOMAIN_LIST  *domActList;   // v12 0x009AE7B8 (feb 0x00963468)
extern domTPT_DOMAIN_LIST  *domTptList;   // v12 0x009AE7BC (feb 0x00963470)
extern domKILL_DOMAIN_LIST *domKillList;  // v12 0x009AE924 (feb 0x009635DC)

// gsAppState bits used by the game systems (full list: math_app/app.h)
enum gsAPPSTATE_GS {
    GSAPP_CINEMA_PLAYING  = 0x00000200, // set while cinSYSTEM plays a script cinema (skips gameplay input)
    GSAPP_PAUSED_MENU     = 0x00004000, // gslSL::Load clears it after gsMsgChangeMode(3)  // ? name
    GSAPP_LOADING_SAVE    = 0x00020000, // set during gslSL::Load
    GSAPP_LEVEL_ACTIVE    = 0x00040000, // set by __gsLoadLevel after INIT_LEVEL, cleared by DestroyLevel; gates
                                        // entIACTIVE_OBJ FRAME processing and gsRENDER_SYSTEM::BeginFrame
    GSAPP_LEVEL_TERM_REQ  = 0x00080000, // set by GS_MSG 0x3F2 (TERM_MAP); __gsLoadLevel clears it
};

// =====================================================================================================
// Message ids used by these systems (v12 value first). Ids < 0x3FA are identical in both builds.
// Full catalogue: re/mpsrc/mp_messages.h. Ids marked "verified" were read from a push in v12 code.
// =====================================================================================================
enum gsMSG_ID_GS {
    GS_MSG_FRAME            = 0x3E8, // ProcessFRAME of every system
    GS_MSG_RENDER           = 0x3E9,
    GS_MSG_LOAD_LEVEL       = 0x3EC, // data = gsLOAD_SCENE_PARAM*: gsLVL_SYSTEM loads (bare name -> PathScn\name.lg)
    GS_MSG_LOAD_GAME        = 0x3ED, // data = char *saveFile: gsLVL_SYSTEM -> gslLoad(file,0)  (name ours)
    GS_MSG_INIT_GAME        = 0x3EF, // SendAll from gslSL::Load and StartGame; music list declares itself MP-syncable
    GS_MSG_INIT_MAP         = 0x3F1, // resets gsElapsedTimeLevel (sent when param->resetLevelTime)
    GS_MSG_TERM_MAP         = 0x3F2, // gsLVL_SYSTEM: gsAppState |= 0x80000  // ? name from mp_messages.h
    GS_MSG_INIT_LEVEL       = 0x3F4, // after load: every system ProcessINIT_LEVEL (domains Init, cinema scan...)
    GS_MSG_TERM_LEVEL       = 0x3F5, // DestroyLevel: every system ProcessTERM_LEVEL
    GS_MSG_LEVEL_LOADED     = 0x3F6, // SendAll just before INIT_LEVEL (domain lists: ProcessLEVEL_LOADED) (name ours)
    GS_MSG_LEVEL_STARTED    = 0x3F7, // SendAll after SetupMPStates (name ours)
    GS_MSG_IACTION          = 0x3F8, // data = IAction command; +0x08 apNAME cmd ("ACTIVATE","SET","SHOW","CLEAN"...), +0x50 float, +0x60 char arg[]
    GS_MSG_SET_LEVEL_ANCHOR = 0x3FB, // feb 0x3FA. data = m3dMATR delta (ApplyReloadAnchor, verified push 0x3FB at v12 0x00536E9E)
    GS_MSG_ASK_KEEP_LEVEL_RELOAD = 0x3FC, // feb 0x3FB. DestroyLevel asks entities whether they survive a reload
    GS_MSG_CHG_MODE         = 0x3FD, // feb 0x3FC. data = int* mode (gsMsgChangeMode); music restarts on 1/5  // ? v12 value
    GS_MSG_MP_PLAYER_LEAVE  = 0x409, // feb 0x408 (secret domain forgets the player)  // ? v12 value
    GS_MSG_MP_NOTIFY        = 0x40E, // feb 0x40C. verified (gsDOMAIN_VIS v12 0x00537E23). sub-codes: 0 = per-entity, -100 = full state
    GS_MSG_TELEPORT_LEVEL   = 0x4CB, // feb 0x4C9. verified (domTPT_DOMAIN::Update v12 0x0055CA5F) data = gsLOAD_SCENE_PARAM: the
                                     //   wr game posts it on as GS_MSG_LOAD_LEVEL (not level complete) (name ours)
    GS_MSG_KILL_DOMAIN_HIT  = 0x4C6, // feb 0x4C5. verified (domKILL_DOMAIN v12 0x0055CD44). data {msgDATA hdr size 0x10, int 4, float 10000}
                                     //   = damage message (type 4, amount 10000) -> instant death  (name ours)
    GS_MSG_CINEMA_START     = 0x4CE, // feb 0x4CC. verified (cinSYSTEM::ProcessFRAME v12 0x0054450D) (name ours)
    GS_MSG_CINEMA_END       = 0x4CF, // feb 0x4CD. verified (v12 0x0054486F) (name ours)
};
// NOTE: renumbering is NOT a flat +2 above 0x40B: 0x4C5 -> 0x4C6 (+1) but 0x4CC -> 0x4CE (+2). Look each id up.

// =====================================================================================================
// gsLOAD_SCENE_PARAM -- payload of GS_MSG_LOAD_LEVEL and argument of gsLVL_SYSTEM::LoadLevel. size 0x154,
// same in both builds (ProcessMsg copies 0x55 dwords into a static, v12 0x009A58E8, feb 0x0095CD58).
// mpsrc/gs_play.h calls it gsMSG_LOAD_LEVEL; field meaning below is from gsLVL_SYSTEM.
// =====================================================================================================
struct gsLOAD_SCENE_PARAM /* : msgDATA */ {
    unsigned char  packId;            // +0x000 0
    unsigned char  flags;             // +0x001 0x40
    unsigned short size;              // +0x002 0x154
    char  level[0x104];               // +0x004 level file. No '.' and no '\\' -> fnmMakeFullName(PathScn, name, "lg")
    char  anchorInst[0x40];           // +0x108 optional: instance kept as reload anchor (PrepareReloadAnchor); "" = none
    int   keepEntities;               // +0x148 !=0: entities that answer GS_MSG_ASK_KEEP_LEVEL_RELOAD survive the reload
    int   resetLevelTime;             // +0x14C !=0: send GS_MSG_INIT_MAP (gsElapsedTimeLevel = 0) after load
    void *keepFilter;                 // +0x150 object, vcall +0x2C(animINST*) -> keep this entity  // ? type
};

// =====================================================================================================
// Object-name tags. Level designers mark objects by putting "&tag" into the object name (objOBJ +0x18).
// Domains are collected by gsDOMAIN_LIST::Collect -> IsAddObj (strstr on the name). Same strings in v12.
// =====================================================================================================
//  domain tag            list class (owner)                 global / where created
//  "&dom_snd"            gsDOMAIN_LIST_SND (gsSND_SYSTEM+0x28)  ambient sound domain, sheet [SOUND]/SOUND_CHN
//  "&dom_mus"            gsDOMAIN_LIST_MUS (gsSND_SYSTEM+0x2C)  music tempo domain [MUSIC] cur/curMusic
//  "&dom_vis"            gsVIS_SYSTEM (gsSysVis)              visibility cell [DOM_VIS] vis, vis_1..
//  "&dom_act"            domACT_DOMAIN_LIST (domActList)      trigger [DOM_ACTION] isActive, isActOnce
//  "&dom_clean"          domCLN_DOMAIN_LIST (domClnList)      IAction "CLEAN": removes corpses/items inside
//  "&dom_kill"           domKILL_DOMAIN_LIST (domKillList)    kill volume [DOM_KILL] isNoPlr
//  "&dom_tpt"            domTPT_DOMAIN_LIST (domTptList)      teleport / level exit [TELEPORT]
//  "&dom_msg"            wrMSG_SYSTEM                         subtitle/voice message [DOM_MSG] msg, isShowOnce
//  "&dom_secret"         wrSECRET_SYSTEM                      secret area (stats counter) IAction "FIND"
//  "&dom_arena"          aiaDOMAIN_LIST_ARENA                 AI arena (see weapon_npc / AI SDK)
//  "&dom_ai", "&dom_ai_zevs_bolts", "&dom_action_medusa"  AI-private domains (aiDOMAIN_SPAWN, boss scripts)
//  "&dom"                any domain object: SetExtraObjFlags makes it invisible/non-colliding (state 0x40008 | 0x38)
//  "&fog"                handled by waterLIQUID (water volume fog), not a domain list
// Render/collision tags handled by gsLVL_SYSTEM::SetExtraObjFlags: see level.h.
