// entity.h -- Will Rock 1.2 unofficial SDK, engine_core: entIACTIVE_OBJ / entENTITY / entity class registry
//
// objs covered: gs:ent_interact.obj, gs:entity.obj, gs:ent_mp.obj (MP hooks of the base classes)
// Assumed: msgADDR, msgSYSTEM, apNAME (core_types.h), animINST / animTPL (anim.h), objOBJ (object.h),
//   m3dV, m3dMATR, m3dCOLOR {float r,g,b,a}, psSHEET, cdtINFO (collision.h), scnSCENE, animCREATE_DATA,
//   gsMSG_MP_SYNC / gsMSG_MP_SAVELOAD / gsEVENT (re/mpsrc).
//
// Class tree:  msgADDR (0x24)
//               +-- entIACTIVE_OBJ  "interactive object": IAction scripts, lock counters, MP id. sign 'ITNE'
//                    |   size v12 0x98 (feb 0x94). Also the base of every gsDOMAIN_LIST (domains/systems).
//                    +-- entENTITY  anything placed in the world with an animINST. sign '$TNE'
//                         size v12 0x16B (feb 0x14F), packed (#pragma pack(1): fields at odd offsets)
//                         +-- physPHYS (physics.h), plrPLAYER, aiNPC..., pjl projectiles, wrsfx..., wrsob...
//
// !!! 1.2 LAYOUT CHANGE (the main thing a port of a feb-era mod must fix) !!!
//   entIACTIVE_OBJ: feb ints +0x3C/+0x40 became two u16 at +0x3C/+0x3E (-4 bytes); everything from feb +0x44
//     moves -4 (apNAME name 0x44->0x40, flags 0x88->0x84, mpId 0x8C->0x88 ...); then 1.2 appends 8 bytes
//     (+0x90 float 1/30, +0x94). Net: base grew 0x94 -> 0x98 (+4).
//   entENTITY: feb +0x94..+0xDF -> +4; 1.2 inserts an m3dV at v12 +0xE4; feb +0xE0..+0x144 -> +0x10;
//     1.2 inserts 0x10 bytes at v12 +0x155; feb +0x145.. -> +0x20. Size 0x14F -> 0x16B (+0x1C).
//   vtable: 1.2 inserted slot 8 (ProcessMP_UPD_STATE) in entIACTIVE_OBJ, so iactive slots >= 8 are +1;
//     entENTITY lost the three MP_UPD_POS slots and gained one (see table). Derived classes add +0x1C to
//     every own field and start their own virtuals at v12 slot 25 (feb 26).
//   Every offset below was read from an instruction in v12 (ctor 0x00506500 / 0x0050AC00, PostProcessMsg
//   0x005068A0, physInit 0x00508A10, vtable slots) unless marked "// ?".

#pragma once
#include "core_types.h"
#include "object.h"
#include "collision.h"   // cdtREFINE (entCDT_REFINE_*)

class animINST; class animTPL; class scnSCENE; class animCREATE_DATA; class cdtINFO;
struct gsMSG_MP_SYNC; struct gsMSG_MP_SAVELOAD; struct gsEVENT; struct m3dCOLOR;
struct gsMSG_MP_SAVELOAD {         // GS_MSG_MP_WRITE/READ_SYNC_DATA (saved games): the entity's sync block + the file
    struct msgDATA *data;           // +0x00 in: the block to write; READ: 0 -> the entity points it at its own static block
    class fioFILE  *file;           // +0x04 chunks 0xF0.. of the entity
};

#pragma pack(push, 1)

// ------------------------------------------------------------------------------------------------
// IActions: designer-scripted reactions read from the instance property sheet ("ON_ACTIVATE",
// "ON_START", "ON_STOP", "ON_ACTION[_n]", "ON_COMPLETE", "ON_LOCK", "ON_UNLOCK", "ON_LOCK_CMD",
// "ON_UNLOCK_CMD", "ON_ACTION_FRAME_\"seq\"_n" for entities). Executed by ExecuteIAction.
// ------------------------------------------------------------------------------------------------
enum entIACTION_EVENT {             // bit = pending event in entIACTIVE_OBJ::pendingEvents and IACTION_DATA::eventMask
    IAEV_LOCK        = 0x040,       // lock count 0 -> 1      (GS_MSG_IACTION "LOCK")
    IAEV_UNLOCK      = 0x080,       // lock count 1 -> 0      (GS_MSG_IACTION "UNLOCK")
    IAEV_LOCK_CMD    = 0x100,       // every LOCK message
    IAEV_UNLOCK_CMD  = 0x200,       // every UNLOCK message
    IAEV_DISABLED    = 0x10000000,  // pendingEvents: IActions off ([IACTIVE] isIActionEnabled No, DISABLE_IACTION)
    // 0x01..0x20: ACTIVATE / DEACTIVATE / START / STOP / ACTION / COMPLETE (ActionName2State v12 0x0050BA30)
};
struct entIACTION_DATA {            // size 0xBC (ParseIActionData v12 0x0050BC20). Its first 0xA0 bytes are the msgDATA
                                    // the targets get with GS_MSG_IACTION (0x3F8): header, command, arguments
    msgHDR hdr;                     // +0x00 pack 0, tag 0x40, size 0xA0 (AddIAction)
    u32   eventMask;                // +0x04 entIACTION_EVENT bits that trigger it; 0x20000000: objName is a name prefix
    apNAME cmd;                     // +0x08 [ON_xxx] cmd: what the targets do ("OPEN", "LOCK", "SET", ...)
    int   i1, i2;                   // +0x48 +0x4C [ON_xxx] i1_..., i2_... (the first key starting so)
    float f1, f2;                   // +0x50 +0x54 f1_..., f2_...
    int   b1, b2;                   // +0x58 +0x5C b1_..., b2_...
    char  s1[0x40];                 // +0x60 s1_...
    int   nObj;                     // +0xA0 target names
    union { apNAME *objName; void *heapData; }; // +0xA4 objName, objName_1, ...: the targets (apFree'd by TermIActive)
    float delay;                    // +0xA8 s; 0: sent at once (SendMsg), else PostTimeMsgData
    int   animSeq;                  // +0xAC entENTITY: ON_ACTION_FRAME sequence index (-1 any)
    int   actionFrame;              // +0xB0 entENTITY: action-frame bit index (-1 any)
    int   nAction;                  // +0xB4 runs left (default 0x5FFFFFFF)
    int   condNmbLocks;             // +0xB8 -1: always; else only while the owner's nLocks equals it
};
void entIACTION_Init(entIACTION_DATA *d); // entIACTION::ctor v12 0x0050C390 (feb 0x004F98F0): header, the first key of each kind

// ------------------------------------------------------------------------------------------------
// entIACTIVE_OBJ   vtable v12 0x00605D58 (feb 0x005DA830)   ctor v12 0x0050AC00 (feb 0x004F8290)
// ------------------------------------------------------------------------------------------------
enum entFLAGS {                     // entIACTIVE_OBJ::entFlags  (v12 +0x84, feb +0x88)
    ENTF_REMOTE        = 0x01,      // driven from network/physics: entENTITY skips its own velocity estimate  // ? name
    ENTF_IACTIONS_ON   = 0x04,      // IActions enabled ("isIActionEnabled")
    ENTF_MP_STATE_RECV = 0x40,      // 1.2: ProcessMP_UPD_STATE stored a new dead-reckoning block
};
class entIACTIVE_OBJ : public msgADDR {        // msgADDR occupies +0x00..+0x23
public:
    // --- vtable (slot +0x00..+0x0C = msgADDR) ---------------------------------------------------
    //  slot  v12 offs  v12 impl     feb offs  feb impl     meaning
    //  [ 0]  +0x00     0x00559A40   +0x00     0x00512910   scalar deleting dtor (shared with gsDOMAIN_LIST)
    //  [ 1]  +0x04     0x004802A0   +0x04     0x00499C50   ProcessMsg (base: return 0)
    //  [ 2]  +0x08     0x0050AC80   +0x08     0x004F8300   PreProcessMsg: INIT clears events, FRAME filtered by gsAppState&0x40000,
    //                                                      IACTION LOCK/UNLOCK counting, MP GET/WRITE/READ_SYNC_DATA routing
    //  [ 3]  +0x0C     0x0050AEF0   +0x0C     0x004F8490   PostProcessMsg: fires pending IActions, forwards to MP clients
    //  [ 4]  +0x10     0x0050A560   +0x10     0x004F7D10   BOOL IsMPSynchableExisting()  (mpId != -1/-3 and !(flags&0xC))
    //  [ 5]  +0x14     0x0050A5A0   +0x14     0x004F7D50   void GetMPSysSyncData(gsMSG_MP_SYNC*)
    //  [ 6]  +0x18     0x0050A6F0   +0x18     0x004F7E60   BOOL IdentifyMPSysSyncData(gsMSG_MP_SYNC*)
    //  [ 7]  +0x1C     0x0050AAC0   +0x1C     0x00403680   GetMP_UPD_STATE(msg**): v12 writes u16 seq (+0x3C) into msg+4; feb returned 0
    //  [ 8]  +0x20     0x00403950   --        --           1.2 NEW: ProcessMP_UPD_STATE(msg*)  (base: empty, ret 4)   (our name)
    //  [ 9]  +0x24     0x0050B680   +0x20     0x004F8BC0   void ParseIActions(psSHEET*)
    //  [10]  +0x28     0x0050BB40   +0x24     0x004F9080   int  AddIAction(psSHEET*, char *name, int)
    //  [11]  +0x2C     0x00403A20   +0x28     0x00403690   ProcessLOCK()   (base empty)  -- called when lock count goes up
    //  [12]  +0x30     0x00403A20   +0x2C     0x00403690   ProcessUNLOCK() (base empty)
    //  [13]  +0x34     0x0051AAF0   +0x30     0x004CDA70   ProcessMP_GET_SYNC_DATA(void*)            (base empty)
    //  [14]  +0x38     0x0051AAF0   +0x34     0x004CDA70   ProcessMP_WRITE_SYNC_DATA(gsMSG_MP_SAVELOAD*)  (base empty)
    //  [15]  +0x3C     0x0051AAF0   +0x38     0x004CDA70   ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD*)   (base empty)
    //  total: v12 16 slots, feb 15 slots
    // --- the virtual slots above, declared (v12 order; feb order in the table) ----------------
    virtual BOOL32 IsMPSynchableExisting();                         // [ 4] +0x10 v12 0x0050A560
    virtual void   GetMPSysSyncData(struct gsMSG_MP_SYNC *s);        // [ 5] +0x14 v12 0x0050A5A0
    virtual BOOL32 IdentifyMPSysSyncData(struct gsMSG_MP_SYNC *s);   // [ 6] +0x18 v12 0x0050A6F0
    virtual int    GetMP_UPD_STATE(void **msg);                      // [ 7] +0x1C v12 0x0050AAC0 (re/mpsrc: GetMP_UPD_REGULAR)
    virtual int    ProcessMP_UPD_STATE(void *msg);                   // [ 8] +0x20 v12 0x00403950 1.2 NEW
    virtual void   ParseIActions(psSHEET *ps);                       // [ 9] +0x24 v12 0x0050B680
    virtual int    AddIAction(psSHEET *ps, char *name, int arg);     // [10] +0x28 v12 0x0050BB40
    virtual void   ProcessLOCK();                                    // [11] +0x2C
    virtual void   ProcessUNLOCK();                                  // [12] +0x30
    virtual void   ProcessMP_GET_SYNC_DATA(void *d);                 // [13] +0x34
    virtual void   ProcessMP_WRITE_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d); // [14] +0x38
    virtual void   ProcessMP_READ_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d);  // [15] +0x3C
    entIACTIVE_OBJ();                                                // v12 0x0050AC00 (feb 0x004F8290)
    virtual ~entIACTIVE_OBJ();                                       // v12 0x00527DC0: TermIActive, then msgADDR's
    virtual BOOL32 PreProcessMsg(int id, void *data, msgADDR *from, int *ret);  // [ 2] +0x08 v12 0x0050AC80
    virtual BOOL32 PostProcessMsg(int id, void *data, msgADDR *from, int *ret); // [ 3] +0x0C v12 0x0050AEF0
    int GetMP_UPD_REGULAR(void **msg) { return GetMP_UPD_STATE(msg); } // re/mpsrc name for slot 7
    int IsTimeToSend(float rate);         // v12 0x0050A4B0 MP send-rate limiter on +0x90/+0x94 (re/mpsrc) // ? exact math
    void SetMPUpdPeriod(float t);         // v12 0x0050A400 mpUpdPeriod = t, below 0 -> 1/3 s, at least 0.025, at most 10
    void OnMPSynchronized();              // re/mpsrc name: entENTITY slot +0x58 (= AssignANIM_ST_ANIMATE_HIDDEN, feb map name)
    // --- fields ---------------------------------------------------------------------------------
    int      unk24;                 // +0x24 v12 +0x24 (feb +0x24) the address lock (msgSYSTEM skips locked addresses; gsMsgChangeMode)
    int      nIActions;             // +0x28 v12 +0x28 (feb +0x28) verified v12 PostProcessMsg 0x005068A0
    entIACTION_DATA *iActions;      // +0x2C v12 +0x2C (feb +0x2C)
    union { int nLocks; int field30; int state; }; // [re/mpsrc: "state", copied in MP sync (catapult)]    // +0x30 [alias field30: re/mpsrc] v12 +0x30 (feb +0x30) LOCK/UNLOCK counter ("nmbLocks")
    int      nSndChannels;          // +0x34 v12 +0x34 (feb +0x34) >0 -> gsSND_SYSTEM::UpdateEntPos each frame (v12 0x00506AF6 area)
    int     *sndChannels;           // +0x38 v12 the entity's channels: indices into gsSysSound->entChannels (AddChannel)
    u16      mpSeqOut;              // +0x3C v12 u16 (feb int +0x3C) MP update sequence counter (GetMP_UPD_STATE ++)
    u16      mpSeqIn;               // +0x3E v12 u16 (feb int +0x40) last accepted incoming sequence
    apNAME   name;                  // +0x40 v12 +0x40 (feb +0x44) instance name (logs "%s Locked")
    union { u32 pendingEvents; u32 field80; }; // +0x80 [alias field80: re/mpsrc] v12 +0x80 (feb +0x84) entIACTION_EVENT bits, cleared each PostProcessMsg
    union { u32 entFlags; u32 mpFlags; };  // +0x84 [alias mpFlags: re/mpsrc] v12 +0x84 (feb +0x88) entFLAGS (verified: v12 0x0050A7D8 or 0x40; PostProcessMsg test 4)
    union { int mpId; int mpid; };         // +0x88 [alias mpid: re/mpsrc] v12 +0x88 (feb +0x8C) -1 = not networked, -3 = local only (ctor)
    union { int mpOwner; int ownerClient; int connId; }; // +0x8C v12 +0x8C (feb +0x90) init -3: owning MP client id (re/mpsrc: ownerClient, -3 = none)
    float    mpUpdPeriod;           // +0x90 v12 only, init 1/30 s (0x3D088889)  // ? meaning (MP state send interval)
    float    mpUpdTimer;            // +0x94 v12 only, init 0  // ?

    void TermIActive();                                   // v12 0x0050B100 (feb 0x004F8640) free IAction table
    void PostProcessCHG_MODE_NOTIFY(void *d);             // v12 0x0050B090 (feb 0x004F85D0) msg 0x3FD (1.2 id)
    static void ExecuteIAction(entIACTION_DATA *a, entIACTIVE_OBJ *self); // v12 0x0050B150 (feb 0x004F8690) (called from v12 entENTITY::PostProcessMsg)
    static int  ActionName2State(char *name);             // v12 0x0050BA30 (feb 0x004F8F70)
    static BOOL32 ParseIActionData(psSHEET*, char*, char*, entIACTION_DATA*, char*); // v12 0x0050BC20 (feb 0x004F9160) // callgraph
    void PrepareMP_UPD_STATE(void *msg);                  // feb 0x004F9820 (1.2: merged into vslot 7)
    void SendMP_Server_2_ClientAll(msgDATA *d, int flags); // v12 0x0050C2E0 (feb 0x004F9840) GS_MSG_MP_NOTIFY to every client
    void SendMP_Client_2_Server(msgDATA *d, int flags);    // v12 0x0050C310 (feb 0x004F9870)
};
// size v12 0x98 (feb 0x94)
void __fastcall entRegMsgMPDispatch(int id);                        // feb 0x004F9800 messages PostProcessMsg forwards to MP clients
entIACTIVE_OBJ __fastcall *entFindNextIO(entIACTIVE_OBJ *prev);      // v12 0x0050C340 (feb 0x004F98A0) iterate all 'ITNE'+'$TNE' addresses

// ------------------------------------------------------------------------------------------------
// entMP_DEADRECKON -- 0x49-byte motion snapshot (network + local prediction). Same layout feb/v12.
// Filled by vslot GetDeadReckonData; 1.2 copies it verbatim into entENTITY::mpState.
// ------------------------------------------------------------------------------------------------
struct entMP_DEADRECKON {
    u8    flags;                    // +0x00 phys: 0x0B base, |0x20 moving fast (>0.2 horiz), |0x40 slow  (physPHYS::ProcessINIT)
    float pos[3];                   // +0x01 animINST position
    float right[3];                 // +0x0D matrix X axis
    float dir[3];                   // +0x19 matrix Z axis (player: normalized, Y zeroed)
    float vel[3];                   // +0x25 entENTITY::velocity
    float accel[3];                 // +0x31 GetAccel()
    float extra[3];                 // +0x3D ? (zeroed by INIT_LEVEL)
};                                  // size 0x49

// ------------------------------------------------------------------------------------------------
// entENTITY   vtable v12 0x00607938 (feb 0x005DC488)   ctor v12 0x00506500 (feb 0x004F3EF0)
// ------------------------------------------------------------------------------------------------
enum entFLAGS2 {                    // entENTITY::flags2 (v12 +0xB8, feb +0xB4)
    ENTF2_COLOR_BLEND = 0x0020,     // UpdateColorBlend every FRAME
    ENTF2_PHYS        = 0x0100,     // set by physPHYS::ProcessINIT
    ENTF2_NO_MP_POS   = 0x0200,     // UpdateMP_UPD_POS returns at once (feb)  // ?
};
class entENTITY : public entIACTIVE_OBJ {
public:
    // --- vtable (continues entIACTIVE_OBJ) ----------------------------------------------------
    //  slot  v12 offs  v12 impl     feb offs  feb impl     meaning
    //  [ 1]  +0x04     0x00502750   +0x04     0x0052D0A0   ProcessMsg (entENTITY: dispatch to ProcessXXX of subclasses)
    //  [ 2]  +0x08     0x00506740   +0x08     0x004F40E0   PreProcessMsg: INIT_LEVEL stores spawn state, msg 1 adds sound channels,
    //                                                      FRAME -> UpdateFollow, UPD msg -> ProcessMP_*
    //  [ 3]  +0x0C     0x005068A0   +0x0C     0x004F4250   PostProcessMsg: FRAME = action frames, velocity estimate, snd pos, colour blend;
    //                                                      INIT not handled -> "Automatic destroy of entity"; TERM -> destroy instance
    //  [ 4]  +0x10     0x0050A580   +0x10     0x004F7D30   IsMPSynchableExisting
    //  [ 5]  +0x14     0x0050A5F0   +0x14     0x004F7D60   GetMPSysSyncData
    //  [ 7]  +0x1C     0x0050A750   (+0x1C)   0x00403680   GetMP_UPD_STATE(msg**): 1.2 fills seq + GetDeadReckonData(msg+6)
    //  [ 8]  +0x20     0x0050A7C0   --        --           ProcessMP_UPD_STATE(msg*): memcpy(mpState, msg+6, 0x49); entFlags |= 0x40
    //  [ 9]  +0x24     0x00506DC0   +0x20     0x004F4780   ParseIActions (adds ON_ACTION_FRAME)
    //  [16]  +0x40     0x004031F0   +0x3C     0x004036A0   BOOL SetupAniSystem()          (base: return 1)
    //  [17]  +0x44     0x00403200   +0x40     0x004036B0   BOOL GetFloorInfo(cdtINFO*)    (base: return 0)
    //  [18]  +0x48     0x00507200   +0x44     0x004F4B90   BOOL FloorPlace(float yOfs, int applyColor, cdtINFO*)
    //  [19]  +0x4C     0x00403A30   +0x48     0x004036C0   void GetAccel(m3dV*)           (copies accel)
    //  [20]  +0x50     0x0050AA30   +0x4C     0x004F80E0   void GetDeadReckonData(entMP_DEADRECKON*)
    //  --    --        --           +0x50     0x004F7EC0   feb GetMP_UPD_POS      } removed in 1.2 (replaced by slots 7/8)
    //  --    --        --           +0x54     0x004F7F40   feb UpdateMP_UPD_POS   }
    //  --    --        --           +0x58     0x004F7EF0   feb ProcessMP_UPD_POS  }
    //  [21]  +0x54     0x00507D00   +0x5C     0x004F5690   void GetCenter(m3dV*)  (animINST::GetCenter or zero)
    //  [22]  +0x58     0x00507D30   +0x60     0x004F56C0   void AssignANIM_ST_ANIMATE_HIDDEN()
    //  [23]  +0x5C     0x00403950   --        --           1.2 NEW, 1 arg, base empty  // ? meaning
    //  [24]  +0x60     0x00506C00   +0x64     0x004F45C0   void ParsePS()  reads ANIM_PROP keys (ANIM_ST_IGNORE_CDT_ALWAYS,
    //                                                      ANIM_ST_EXCLUDE_HH, renderOff, constraintActor/Obj, isExactCDT)
    //  first subclass slot: v12 [25] +0x64 (feb [26] +0x68). total v12 25 slots, feb 26.
    //  (slots 6, 10..15 inherited unchanged from entIACTIVE_OBJ)
    // --- the slots entENTITY overrides (rebuilt in src/gs/entity.cpp) ----------------------------
    entENTITY();                                                     // v12 0x00506500 (feb 0x004F3EF0)
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);     // [ 1] +0x04 v12 0x00502750 (return 0)
    virtual BOOL32 PreProcessMsg(int id, void *data, msgADDR *from, int *ret);  // [ 2] +0x08 v12 0x00506740
    virtual BOOL32 PostProcessMsg(int id, void *data, msgADDR *from, int *ret); // [ 3] +0x0C v12 0x005068A0
    virtual void   ParseIActions(psSHEET *ps);                       // [ 9] +0x24 v12 0x00506DC0 (+ ON_ACTION_FRAME)
    // --- the new virtual slots of entENTITY, declared (v12 order) -------------------------------
    virtual BOOL32 SetupAniSystem();                                 // [16] +0x40
    virtual BOOL32 GetFloorInfo(cdtINFO *ci);                        // [17] +0x44
    virtual BOOL32 FloorPlace(float yOfs, int applyColor, cdtINFO *ci); // [18] +0x48
    virtual void   GetAccel(m3dV *out);                              // [19] +0x4C
    virtual void   GetDeadReckonData(entMP_DEADRECKON *out);         // [20] +0x50
    virtual void   GetCenter(m3dV *out);                             // [21] +0x54
    virtual void   AssignANIM_ST_ANIMATE_HIDDEN();                   // [22] +0x58
    virtual void   Slot23(void *arg);                                // [23] +0x5C 1.2 NEW // ?
    virtual void   ParsePS();                                        // [24] +0x60
    // --- fields (v12 offset first) --------------------------------------------------------------
    union { int unk98; int itemType; int wpnIndex; }; // [alias itemType: re/mpsrc, itemITEM type 0x28..0x2F health/armor;
                                                     //  wpnIndex: a weapon's pwpWEAPON_INDEX, read by the HUD v12 0x00497DE0]
    //       unk98                 // +0x98 v12 (feb +0x94) init -1
    union { int unk9C; u32 colAmbient; };  // +0x9C v12 (feb +0x98) init 0: packed ARGB added to the light (ApplyColor arg 1)
    union { int unkA0; u32 colDiffuse; };  // +0xA0 v12 (feb +0x9C) init -1: packed ARGB the light blends to (arg 2)
    union { int unkA4; u32 colSpecular; }; // +0xA4 v12 (feb +0xA0) init 0: packed ARGB (arg 3)
    float    color[4];              // +0xA8 v12 (feb +0xA4) init 255 x4: the current light r, g, b, a (UpdateColorBlend)
    u32      flags2;                // +0xB8 v12 (feb +0xB4) entFLAGS2 (verified v12 0x00506B0D test 0x20)
    int      unkBC;                 // +0xBC v12 (feb +0xB8)
    union { animINST *inst; animINST *anim; }; // +0xC0 [alias anim: re/mpsrc] v12 (feb +0xBC) the world instance (verified everywhere: [esi+0xC0])
    entENTITY *followEnt;           // +0xC4 v12 (feb +0xC0) SetFollow target
    objOBJ   *followObj;            // +0xC8 v12 (feb +0xC4) optional bone/object of the target
    float    *followOffset;         // +0xCC v12 (feb +0xC8) new'ed m3dMATR (0x40) offset, or 0
    union { float health; int unkD0; }; // +0xD0 v12 (feb +0xCC) health (plrPLAYER::SetDefaultValues v12 0x004277B9: 100.0)
    union { float maxHealth; int unkD4; }; // +0xD4 v12 (feb +0xD0) float (aiPLANNER::GetBossHealth v12 0x00551DD8 hands it out as float)
    float    lastPos[3];            // +0xD8 v12 (feb +0xD4) position last frame (v12 inits to m3dVUndef 0x009AEA90)
    float    lastPos2[3];           // +0xE4 v12 ONLY, init m3dVUndef  // ? (position two frames ago / MP)
    float    velocity[3];           // +0xF0 v12 (feb +0xE0) (pos - lastPos) / gsElapsedTime unless ENTF_REMOTE (verified v12 0x00506AB0)
    float    accel[3];              // +0xFC v12 (feb +0xEC) returned by GetAccel (verified v12 0x00403A34)
    int      unk108;                // +0x108 v12 (feb +0xF8)  // ?
    entMP_DEADRECKON mpState;       // +0x10C v12 (feb +0xFC) 0x49 bytes, ends +0x155 (feb +0x145)
    float    mpSmooth[3];           // +0x155 v12 ONLY: 0.1, 2.0, 0.2  // ? (dead-reckoning smoothing constants)
    int      unk161;                // +0x161 v12 ONLY, 0
    void    *extraBuf;              // +0x165 v12 (feb +0x145) apFree'd on TERM
    u16      extraBufLen;           // +0x169 v12 (feb +0x149)
    // feb had one more dword at +0x14B (MP sequence for ProcessMP_UPD_POS); 1.2 moved it into mpSeqIn.
    // --- non-virtual functions ---
    void   SetFollow(entENTITY *target, objOBJ *bone, int keepOffset); // v12 0x00506F60 (feb 0x004F4910)
    void   TermFollow();                                 // v12 0x005070A0 (feb 0x004F4A50)
    void   UpdateFollow(int force);                      // v12 0x005070E0 (feb 0x004F4A90) called on FRAME by PreProcessMsg
    void   ApplyColor();                                 // v12 0x00507370 (feb 0x004F4D00) FloorPlace(0, 1, 0) in place
    void   ApplyColor(cdtINFO *floor, int blend);        // v12 0x005073B0 (feb 0x004F4D40) light the model from the floor colour
    void   ApplyColor(m3dCOLOR *light, m3dCOLOR *add, m3dCOLOR *spec, int blend); // v12 0x00507430 (feb 0x004F4DC0) -> +0xA0 / +0x9C / +0xA4
    void   UpdateColorBlend();                           // v12 0x005075A0 (feb 0x004F4F30)
    u32    GetColor(m3dCOLOR *a, m3dCOLOR *b, int);      // v12 0x00507A80 (feb 0x004F5410)
};
inline void entIACTIVE_OBJ::OnMPSynchronized() { ((entENTITY *)this)->AssignANIM_ST_ANIMATE_HIDDEN(); }
// size v12 0x16B (feb 0x14F): physPHYS::Create allocates 0x16F/0x153 and stores its physMATH* at +0x16B/+0x14F.

#pragma pack(pop)

// ------------------------------------------------------------------------------------------------
// Entity-class collision filters (gs:entity.obj). apSTATE: a bit set of entity classes (entENTITY::unkBC
// at +0xBC holds the entity's class bits: 1 = water volume, ...).
// ------------------------------------------------------------------------------------------------
class entCDT_REFINE_INCL : public cdtREFINE {        // vtable v12 0x005FB674 (feb 0x005DD200): only entities of these classes
public:
    apSTATE cls;                                     // +0x10
    BOOL32 IsInclInst(animINST *i);                  // v12 0x00508930 (feb 0x004F6300)
    entCDT_REFINE_INCL(int m, animINST *excl, u32 c) : cdtREFINE(m, excl) { cls.bits = c; }
};
class entCDT_REFINE_EXCL : public cdtREFINE {        // vtable feb 0x005CF368: every instance except entities of these classes
public:
    apSTATE cls;                                     // +0x10
    BOOL32 IsInclInst(animINST *i);                  // v12 0x00508980 (feb 0x004F6350)
    entCDT_REFINE_EXCL(int m, animINST *excl, u32 c) : cdtREFINE(m, excl) { cls.bits = c; }
};

// ------------------------------------------------------------------------------------------------
// Entity class registry: "class name" (from the level / template) -> factory.  This is THE hook to add
// new entity types: call entRegisterClass from your mod before the level loads.
// ------------------------------------------------------------------------------------------------
typedef entENTITY *(__fastcall *entCREATE_FN)(animINST *inst);   // e.g. physPHYS::Create
struct entCLASS_INFO {              // size 0x90, table v12 0x00984E70 (feb 0x00940C30)
    char          name[0x40];       // +0x00 class name (e.g. "phys", "ai_amur")
    u32           classId;          // +0x40 FourCC put into msgADDR::classId
    int           priority;         // +0x44 msgADDR::priority (message order: lower first?)  // ? direction
    entCREATE_FN  create;           // +0x48 factory
    char          tplName[0x40];    // +0x4C default template (entGetClassTplName)
    int           exactName;        // +0x8C 1 = full-name match, 0 = prefix match (strncmp of the registered name)
};
extern int           entNClass;     // v12 0x00984E68 (feb 0x00940C28) NO capacity check in entRegisterClass
extern entCLASS_INFO entClassTable[];// v12 0x00984E70 (feb 0x00940C30)

BOOL32 __fastcall entRegisterClass(char *name, u32 classId, int priority, entCREATE_FN create, char *tplName, int exactName);
                                    // v12 0x00507D60 (feb 0x004F56F0) re-registering a name REPLACES the entry (mod override!)
entENTITY *__fastcall entCreate(scnSCENE *scn, char *className, char *instName, animCREATE_DATA *cd, void *initData, int flags);
                                    // v12 0x005082B0 (feb 0x004F5C80) creates animINST from template + entity + sends INIT // ? arg names
entENTITY *__fastcall entCreate(scnSCENE *scn, char *className, char *instName, int tplObjId, animCREATE_DATA *cd, void *initData, int flags);
                                    // v12 0x00508450 (feb 0x004F5E20)
entENTITY *__fastcall entCreate(char *className, animINST *inst, void *initData, int flags);
                                    // v12 0x00507EA0 (feb 0x004F5830) // callgraph match: entity for an existing instance
void   __fastcall entRegister(entENTITY *e, animINST *inst, void *initData); // v12 0x00507FF0 (feb 0x004F5980) InsertAddr + INIT
void   __fastcall entDestroy(entENTITY *e);                 // v12 ? (feb 0x004F5F20) PostMsg(e, MSG_MSG_TERM)
int    __fastcall _entFindClass(char *name);                // v12 0x00508600 (feb 0x004F5FD0)
BOOL32 __fastcall entGetClassTplName(char *cls, char *out); // v12 0x005085B0 (feb 0x004F5F80)
entENTITY *__fastcall entFindNext(entENTITY *prev);         // v12 0x005086A0 (feb 0x004F6070) iterate all '$TNE' (start with 0)
void   __fastcall entAnimAssignTplSeq(animTPL *t, void *seq, int n); // v12 0x00508820 (feb 0x004F61F0)
void   __fastcall _entAnimSetSeqGroupName(animTPL *t, int i, char *name); // v12 0x00508720 (feb 0x004F60F0) one per group
void   __fastcall _entAnimSetSeqName(animTPL *t, int i, char *name);  // v12 0x005086E0 (feb 0x004F60B0?) name a sequence slot
BOOL32 __fastcall entPackInitDataPacked(entENTITY *e, msgDATA *d);   // feb 0x004F8170 (1.2 ?)
