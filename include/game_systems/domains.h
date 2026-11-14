// domains.h -- Will Rock 1.2 unofficial SDK, game_systems: domains (gs:gs_domain, gs_shared:dom_misc, wr_msgs, gs_sound, gs_vis)
// A "domain" is a level volume (an objOBJ whose name contains "&dom...") that fires Enter/Leave/Update when the
// player (or an update point) is inside. Each kind has a LIST object (entIACTIVE_OBJ, one per kind) that collects the
// tagged objects at level load (Collect -> IsAddObj -> MakeDomain) and a DOMAIN object per volume.
// Assumes from re/sdk/core: entIACTIVE_OBJ (size feb 0x94, v12 0x98: +4 inside the base, see core/entity.h),
//   scnDOMAIN (engine volume test, vtbl at its +0), m3dBOX, objOBJ (+0x18 name, +0xF0 psSHEET* properties), psSHEET.
// OFFSET RULE: every field of gsDOMAIN / gsDOMAIN_LIST below the entIACTIVE_OBJ base moved +4 in 1.2 (verified ctors
//   v12 0x00527A20 / 0x00527D80 vs feb 0x00512570 / 0x005128D0). Offsets relative to the scnDOMAIN sub-object are unchanged.
#pragma once
#include "gs_common.h"

// ---------------------------------------------------------------------------------------------------------
// scnDOMAIN part (engine). Domain virtuals Init/Enter/Leave/Update are called with this = scnDOMAIN sub-object.
// Offsets here are RELATIVE TO THE scnDOMAIN sub-object (gsDOMAIN v12 +0x98, feb +0x94) and identical in both builds.
// ---------------------------------------------------------------------------------------------------------
enum gsDOM_FLAGS {               // scnDOMAIN +0x3C  (gsDOMAIN v12 +0xD4, feb +0xD0) verified v12 domACT_DOMAIN::Enter 0x0055C2F0
    DOMF_ACTIVE      = 0x0001,   // [DOM_ACTION] isActive
    DOMF_INITED      = 0x0002,   // gsDOMAIN::Init done
    DOMF_NO_PLAYER   = 0x0100,   // domKILL: [DOM_KILL] isNoPlr (does not kill the player); domACT: "can fire" // ? dual use
    DOMF_ACT_ONCE    = 0x0200,   // [DOM_ACTION] isActOnce
    DOMF_FIRED       = 0x0400,   // already fired (act once) / TPT: anchor found
    DOMF_TPT_NOANCH  = 0x0800,   // TPT: no anchor -> level change by sceneName
    DOMF_TPT_LVLCOMP = 0x2000,   // [TELEPORT] isLevelComplete
    DOMF_TPT_DELAYED = 0x8000,   // [TELEPORT] isDelayed
};

class gsDOMAIN_LIST;
class gsDOMAIN : public entIACTIVE_OBJ, public scnDOMAIN {   // size v12 0x130 (feb 0x12C); scnDOMAIN at v12 +0x98
public:  // vtbl(entIACTIVE_OBJ) v12 0x00608190 (feb 0x005DCCD8), vtbl(scnDOMAIN) v12 0x00608160 (feb 0x005DCCA8)
    // entIACTIVE_OBJ base: tag +0x14 = '$DOM' (0x24444F4D), +0x1C = 0x53 priority (verified v12 0x00527A8E)
    unsigned domFlags;             // v12 +0x0D4 (feb +0x0D0) (scnDOMAIN +0x3C) gsDOM_FLAGS (bit 1: a point is inside)
    int      nSub;                 // v12 +0x0D8 (feb +0x0D4) (scnDOMAIN +0x40) sub-domains (max 0x13: objects linked or [DOMAIN] subDom%d)
    scnDOMAIN *sub[0x13];          // v12 +0x0DC (feb +0x0D8)
    u32      _128;                 // v12 +0x128 (feb +0x124) not referenced by gs_domain // ?
    gsDOMAIN_LIST *list;           // v12 +0x12C (feb +0x128) (scnDOMAIN +0x94) the list that made it (Collect)
    gsDOMAIN();                                    // v12 0x00527A20 (feb 0x00512570)
    ~gsDOMAIN();                                   // v12 0x00527AF0 (deleting 0x00527AA0, scnDOMAIN side 0x005284A0)
    void  *__vecDelDtor(unsigned flags);           // scnDOMAIN slot 0 (v12 0x005284A0: this -= 0x98, the deleting dtor)
    BOOL32 Init();                                 // scnDOMAIN slot 1 v12 0x00527B30 (feb 0x00512680) [DOMAIN] height, isExactIncl, subDom1..n
    void   Term();                                 // scnDOMAIN slot 2 v12 0x00527D10 (feb 0x00512860)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x00527D60 (feb 0x005128B0) MSG_INIT: Init
    virtual void Update(m3dV *pos);                // [16] base empty (v12 0x00403950) per update point, every frame
    virtual void Enter();                          // [17] base empty (v12 0x00403A20) the first point came in
    virtual void Leave();                          // [18] base empty (v12 0x00403A20) the last point went out
    int  AddSubDomain(scnDOMAIN *d);               // v12 0x00527D30 (feb 0x00512880)
};

class gsDOMAIN_LIST : public entIACTIVE_OBJ {      // size v12 0x3C8 (new 0x3C8 in domMiscInit v12 0x0055C132; feb 0x234)  vtbl v12 0x006081E0 (feb 0x005DCD20), 30 slots (+1 in 1.2)
public:  // tag +0x14 = '$DOL' (0x24444F4C), +0x1C = 0x52
    int        curDomain;          // v12 +0x098 (feb +0x094) = -1; the update point being passed to Update (-1 between)
    u32        _9c;                // v12 +0x09C (feb +0x098) 1: a point is in some domain, 2: none is, 4: changed this frame (music list: 0x100 too)
    int        nDomains;           // v12 +0x0A0 (feb +0x09C) max 100 (cmp 0x64 at v12 0x00528332 in Collect)
    gsDOMAIN  *domain[100];        // v12 +0x0A4 (feb +0x0A0)
    int        nSub;               // v12 +0x234 (feb +0x230) the sub-domains made for the domains (cmp 0x63)
    scnDOMAIN *sub[100];           // v12 +0x238 (1.2 only)
    gsDOMAIN_LIST();                               // v12 0x00527D80 (feb 0x005128D0)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x00528440 (feb 0x00512FA0)
    virtual scnDOMAIN *AddSubDomain(gsDOMAIN *d, char *name);   // [16] v12 0x00528150 (feb 0x00512CB0) by object name in the scene
    virtual scnDOMAIN *AddSubDomain(gsDOMAIN *d, objOBJ *o);    // [17] v12 0x00528180 (feb 0x00512CE0) shared by name
    virtual void Update();                         // [18] v12 0x00527F30 (feb 0x00512A90) per FRAME: point-in-volume -> Enter/Leave/Update
    virtual void Enter();                          // [19] base empty (v12 0x00403A20) the first domain was entered
    virtual void Leave();                          // [20] base empty (v12 0x00403A20) no domain has a point any more
    virtual int  GetNUpdatePoint();                // [21] v12 0x00527EE0 (feb 0x00512A40) the server's players
    virtual int  GetUpdatePoint(int i, m3dV *p);   // [22] v12 0x00527EF0 (feb 0x00512A50) player i's position, 1.6 up
    virtual int  IsAddObj(objOBJ *o) = 0;          // [23] per kind: strstr(name,"&dom_xxx")
    virtual gsDOMAIN *MakeDomain() = 0;            // [24] per kind: new <DOMAIN>
    virtual scnDOMAIN *MakeSubDomain(gsDOMAIN *d); // [25] v12 0x004ACF70 new scnDOMAIN
    virtual void DeleteSubDomain(scnDOMAIN *s);    // [26] v12 0x004ACFB0
    virtual void ProcessLEVEL_LOADED();            // [27] v12 0x00528420 (feb 0x00512F80) Collect every "&tag" object
    virtual int  ProcessINIT_LEVEL();              // [28] v12 0x00527DE0 (feb 0x00512940) Init all domains
    virtual void ProcessTERM_LEVEL();              // [29] v12 0x00527E40 (feb 0x005129A0)
    gsDOMAIN *FindDomain(char *name);              // v12 0x005280F0 (feb 0x00512C50)
    int  FindDomainIndex(gsDOMAIN *d);             // v12 0x005282E0 (name ours) -1: not in the list
    int  Collect(objOBJ *o);                       // v12 0x00528310 (feb 0x00512E70)
};

// ---------------------------------------------------------------------------------------------------------
// Concrete kinds (gs_shared:dom_misc).  domMiscInit v12 0x0055C130 (feb 0x00544B60) creates the 4 lists, domMiscTerm v12 0x0055C240
// ---------------------------------------------------------------------------------------------------------
// rebuilt in src/gs_shared/dom_misc.cpp. Domain flags (gsDOMAIN::domFlags): 1 a player is inside (gsDOMAIN_LIST::Update),
// 0x100 active / (kill) no player, 0x200 act once, 0x400 fired / (teleport) anchor found, 0x800 level change,
// 0x2000 level complete, 0x8000 delayed, 0x10000 (1.2, teleport) ACTIVATE'd.
class domACT_DOMAIN : public gsDOMAIN {   // "&dom_act" trigger: its IActions (ON_ACTIVATE on entering, ON_DEACTIVATE on leaving)
public:  // [DOM_ACTION] isActive (default 1), isActOnce (default 1). vtbl v12 0x006093E8 / 0x006093B8 (feb 0x005DDF20/0x005DDEEC)
    BOOL32 Init();                                 // scnDOMAIN slot 1 v12 0x0055C2C0 (feb 0x00544CF0) + DeclSyncable
    void   Enter();                                // [17] v12 0x0055C2F0 (feb 0x00544D20) IAction event 1
    void   Leave();                                // [18] v12 0x0055C330 (feb 0x00544D60) IAction event 2
    void   ProcessUNLOCK();                        // [12] v12 0x0055C370 (1.2: Enter again)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x0055C380 (feb 0x00544DB0)
    void   ProcessMP_NOTIFY(msgDATA *ev);          // v12 0x0055C4B0 (feb 0x00544EE0)
    // MP_GET_SYNC_DATA v12 0x0055C4D0, MP_READ_SYNC_DATA v12 0x0055C5C0: not rebuilt
};
class domCLN_DOMAIN : public gsDOMAIN {   // "&dom_clean": IAction "CLEAN" (looks at the NPCs / items inside; the result is unused)
public:
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x0055C6E0 (feb 0x00545090)
};
class domKILL_DOMAIN : public gsDOMAIN {  // "&dom_kill": NPCs and players inside get the 0x4C6 damage {type 0, sub 1, 5000}
public:
    BOOL32 Init();                                 // scnDOMAIN slot 1 v12 0x0055CBA0 (feb 0x00545500) [DOMAIN] height (1), [DOM_KILL] isNoPlr
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x0055CC30 (feb 0x00545590)
};
class domTPT_DOMAIN : public gsDOMAIN {   // "&dom_tpt": teleport inside the level (to the anchor) or a level change
public:
    char sceneName[0x40];          // v12 +0x130 (feb +0x12C) (scnDOMAIN +0x98) [TELEPORT] sceneName: the level to change to
    char anchor[0x40];             // v12 +0x170 (feb +0x16C) (scnDOMAIN +0xD8) [TELEPORT] anchor: the instance (both levels)
    float anchorMatr[16];          // v12 +0x1B0 (feb +0x1AC) (scnDOMAIN +0x118) its matrix (teleport inside the level)
    domTPT_DOMAIN() { sceneName[0] = 0; anchor[0] = 0; }  // inline (MakeDomain v12 0x0055CF50)
    BOOL32 Init();                                 // scnDOMAIN slot 1 v12 0x0055C7A0 (feb 0x00545150)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x0055C8D0 (1.2: IAction ACTIVATE)
    void   Update(m3dV *pos);                      // [16] v12 0x0055C920 (feb 0x00545290) -> GS_MSG_TELEPORT_LEVEL 0x4CB / MP notify
};
class domACT_DOMAIN_LIST : public gsDOMAIN_LIST { public: int IsAddObj(objOBJ *o); gsDOMAIN *MakeDomain(); };
class domCLN_DOMAIN_LIST : public gsDOMAIN_LIST { public: int IsAddObj(objOBJ *o); gsDOMAIN *MakeDomain(); };
class domTPT_DOMAIN_LIST : public gsDOMAIN_LIST { public: int IsAddObj(objOBJ *o); gsDOMAIN *MakeDomain(); };
class domKILL_DOMAIN_LIST : public gsDOMAIN_LIST { public: int IsAddObj(objOBJ *o); gsDOMAIN *MakeDomain(); };
extern domCLN_DOMAIN_LIST  *domClnList;  // v12 0x009AE7A4 (feb 0x00963450) vtbl v12 0x00609250
extern domACT_DOMAIN_LIST  *domActList;  // v12 0x009AE7B8 (feb 0x00963468) vtbl v12 0x006092C8
extern domTPT_DOMAIN_LIST  *domTptList;  // v12 0x009AE7BC (feb 0x00963470) vtbl v12 0x00609340
extern domKILL_DOMAIN_LIST *domKillList; // v12 0x009AE924 (feb 0x009635DC) vtbl v12 0x006091D8
int  __fastcall domMiscInit();           // v12 0x0055C130 (feb 0x00544B60)
void __fastcall domMiscTerm();           // v12 0x0055C240 (feb 0x00544C70)
// IsAddObj: ACT v12 0x0055C2A0, CLN 0x0055C6C0, TPT 0x0055C780, KILL 0x0055CB80.  MakeDomain: ACT 0x0055CE90, CLN 0x0055CED0, KILL 0x0055CF10, TPT 0x0055CF50

// Sound / music domains (gs:gs_sound) -- lists owned by gsSND_SYSTEM (+0x28 / +0x2C), see sound.h
class gsDOMAIN_SND : public gsDOMAIN {   // "&dom_snd": ambient loop while inside. size v12 0x138; vtbl v12 0x00607C58 / 0x00607C24
public:  // the object sheet's SOUND_CHN / SOUND sections (gsSND_SYSTEM::CreateChannel), [DOM_SND] fadeTime
    float fadeTime;                // v12 +0x130 [DOM_SND] fadeTime (default 2 s): -30 dB to 0 dB
    float fade;                    // v12 +0x134 s faded in: grows inside, falls outside; below 0 the sounds stop
    gsDOMAIN_SND() : fadeTime(2.0f), fade(0.0f) {}  // inline (MakeDomain v12 0x00513040)
    BOOL32 Init();                                 // scnDOMAIN slot 1 v12 0x005122B0 (feb 0x004FF4C0)
    void   Term();                                 // scnDOMAIN slot 2 v12 0x00512400
    void   Update(m3dV *pos);                      // [16] v12 0x00512500 (feb 0x004FF690) restarts and fades
    void   Enter();                                // [17] v12 0x00512480 (feb 0x004FF5E0) every channel plays
    void   Leave();                                // [18] v12 0x00401380 (empty)
    void   UpdateFade(float dt);                   // v12 0x005125C0 (name ours)
};
class gsDOMAIN_MUS : public gsDOMAIN {   // "&dom_mus": sets music tempo when entered. size v12 0x170; vtbl v12 0x00607CD8 / 0x00607CA4
public:
    char tempo[0x40];              // v12 +0x130 (feb +0x12C) (scnDOMAIN +0x98) [MUSIC] curMusic / cur = "soft" | "hard" | "fight"
    gsDOMAIN_MUS() { tempo[0] = 0; }               // inline (MakeDomain v12 0x00513090)
    BOOL32 Init();                                 // scnDOMAIN slot 1 v12 0x005127B0 (feb 0x004FF770)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x00512830 IAction "SET"
    void   Enter();                                // [17] v12 0x00512810
};

// Message / secret domains (wr_msgs.obj, game side but listed here)
//  wrMSG_SYSTEM    vtbl v12 0x006063F8 : "&dom_msg"    wrDOMAIN_MSG    vtbl v12 0x006064A0/0x00606470 [DOM_MSG] msg, isShowOnce
//  wrSECRET_SYSTEM vtbl v12 0x00606380 : "&dom_secret" wrDOMAIN_SECRET vtbl v12 0x00606520/0x006064EC (secret counter, IAction FIND)
//  feb: wrMSG_SYSTEM::ShowMsg 0x0049E340, ::ProcessMsg 0x0049E410, wrDOMAIN_MSG::Init 0x0049E5B0, ::Enter 0x0049E600,
//       wrSECRET_SYSTEM::ProcessMsg 0x0049E740  (v12: use tables/v12.functions.tsv)  // ? v12 addresses
// Arena / AI domains: aiaDOMAIN_LIST_ARENA vtbl v12 0x00608D18, aiaDOMAIN_ARENA v12 0x00608DC0/0x00608D90 -> see re/sdk/ai.
