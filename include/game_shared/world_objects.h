// world_objects.h -- Will Rock 1.2 unofficial SDK, game_shared: level gameplay objects
//   itemITEM (item.obj), doorDOOR (door.obj), bttnBUTTON (button.obj), trmpTRAMPOLINE (trampoline.obj),
//   dstrDESTROY (dyn_destroy.obj), gmSharedInit (gm_shared.obj)
//
// Assumes from re/sdk/core (engine_core): entIACTIVE_OBJ, entENTITY (size v12 0x16B, feb 0x14F), animINST, animTPL,
//   objOBJ, m3dV, msgDATA, apNAME, gsMP_SERVER *gsSysServer, gsMP_SYSTEM *gsSysMP, gsElapsedTime, entRegisterClass.
//   From re/mpsrc: gsEVENT, gsMSG_MP_SAVELOAD, gsMSG_MP_SYNC, GS_MSG_* ids (mp_messages.h).
//   plgsACTION is the player base (player subsystem). rgdRIGID*/shtSHATTER from engine_core physics.h.
//
// LAYOUT RULE (verified per class below): every field declared by these classes sits at feb offset + 0x1C in v12,
// because entENTITY grew 0x1C bytes in retail. Fields of the bases you see touched here:
//   entIACTIVE_OBJ +0x1C priority (0x28 when inst flag 0x40), +0x30 lockCount ("nmbLocks"/"nmbPicks"; not shifted)
//   entENTITY animINST *inst: v12 +0xC0 (feb +0xBC)  (door INIT "mov ecx,[esi+0xC0]" v12 0x0055B5BE)
// Packed: #pragma pack(1).
//
// Every class is created by name from a level template ("CLASS" key of an instance) through entRegisterClass:
//   entRegisterClass(name, signature, priority 0x28, Create, ...). Properties are read in ProcessINIT from the
//   instance property sheet with animINST::GetBool/GetInt/GetFloat/GetStr(inst, SECTION, key, &val).
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
class plgsACTION; class aiNPC;
#pragma pack(push, 1)

// Message ids handled here (v12 values; see re/mpsrc/mp_messages.h):
//   1 INIT, 2 TERM, 3 (address removed: clear pointers), 1000 FRAME (0x3E8), 0x3F4 INIT_LEVEL,
//   0x3F8 GS_MSG_IACTION (apNAME at data+8: "PICK","UNLOCK","OPEN","CLOSE","PUSH","POP"), 0x3FC (feb 0x3FB) query,
//   0x40E GS_MSG_MP_NOTIFY (feb 0x40C), 0x4C6 GS_MSG_DAMAGE (feb 0x4C5).
//   itemITEM::ProcessMsg v12 0x00556D50 compares 0x3F4, 3, 0x3E8, 0x3F8, 0x3FC, 0x40E (verified).

// =================================================================================================================
// itemITEM -- pickup ("item"). Bobs and spins, picked up by a player standing close. Subclassed by the game
// (wrItemInit creates the concrete pickups: health/ammo/weapons/keys, vtable v12 0x00605EE8 is one of them).
// size v12 0x1AF (feb 0x193): Create v12 0x00557A50 "push 0x1AF". signature 'MTI$' (0x2449544D), class "item".
// =================================================================================================================
enum itemFLAGS {               // itemITEM::flags
    ITEMF_ACTIVE        = 0x0001, // visible & pickable (set at end of INIT)
    ITEMF_PLACE_GROUND  = 0x0002, // ITEM.isPlaceGround (default 1)
    ITEMF_MOVE_ON_UNLOCK= 0x0004, // ITEM.isMoveUponUnlock or nmbPicks>0: on unlock teleports to the unlocker (+height)
    ITEMF_ROTATED       = 0x0008, // ITEM.isRotated (default 1): spins 70 deg/s about Y
    ITEMF_HIDDEN_NOPICK = 0x0010, // ? not pickable while set
    ITEMF_FORCE_HIDDEN  = 0x0020, // inst hidden every frame
    ITEMF_PICK_SEEN     = 0x1000, // first time a player was in range (raises entity event 0x20)
};
class itemITEM : public entENTITY {
public:
    // own virtuals (v12 order, from slot 25; vtable v12 0x00605F88, feb 0x005DAA58)
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);    // +0x04 v12 0x00556D50 (feb 0x0053E1A0)
    virtual BOOL32 GetFloorInfo(cdtINFO *ci);                                   // +0x44 (wrITEM v12 0x004A9FB0, feb 0x0049BD60)
    virtual BOOL32 ProcessINIT();                  // [25] +0x64 v12 0x00556E60 (feb 0x0053E2B0)
    virtual void   ProcessFRAME();                 // [26] +0x68 v12 0x00557090 (feb 0x0053E4E0) bob / spin, the pick test
    virtual void   ProcessTERM();                  // [27] +0x6C v12 0x00557080 (feb 0x0053E4D0) DestroySfx
    virtual void   ProcessINIT_LEVEL();            // [28] +0x70 v12 0x00557060 (feb 0x0053E4B0) on the floor
    virtual void   ProcessMP_NOTIFY(void *ev);     // [29] +0x74 v12 0x005575D0 (feb 0x0053EA20)
    virtual void   Enable(int on);                 // [30] +0x78 v12 0x005578B0 (feb 0x0053EDC0)
    virtual void   ShowUnhideSfx();                // [31] +0x7C v12 0x005578F0 (feb 0x0053EE00)
    virtual BOOL32 CreateSfx(char *name);          // [32] +0x80 v12 0x00557950 (feb 0x0053EE60)
    virtual void   DestroySfx();                   // [33] +0x84 v12 0x00557A00 (feb 0x0053EF10)
    virtual void   ShowSfx(int on);                // [34] +0x88 v12 0x00557A20 (feb 0x0053EF30)
    virtual void   UpdateSfx();                    // [35] +0x8C v12 0x00403A20 (shared empty) every frame while active // ? name
    virtual void   Pick(plgsACTION *by);           // [36] +0x90 v12 0x005575B0 (feb 0x0053EA00) HOOK: subclasses give the bonus here
    virtual BOOL32 IsCanPick(plgsACTION *by);      // [37] +0x94 v12 0x00557340 (feb 0x0053E790) HOOK: pickup range test
    // not rebuilt: GetMPSysSyncData (+0x14 v12 0x005578A0), ProcessMP_GET / WRITE / READ_SYNC_DATA (+0x34 0x005576F0,
    // +0x38 0x004A73B0, +0x3C 0x00557790) -- the base ones run

    unsigned int flags;        // v12 +0x16B (feb +0x14F) itemFLAGS. INIT "or [esi+0x16B]" verified
    float        amount;       // v12 +0x16F (feb +0x153) the bonus (wrITEM::ProcessINIT per template), given by the player's Pick
    float        height;       // v12 +0x173 (feb +0x157) ITEM.height (0.95): over the floor / the unlocker. verified
    float        bobAmp;       // v12 +0x177 (feb +0x15B) ITEM.heightDisp (0.6) * 0.4. verified
    entENTITY   *sfx;          // v12 +0x17B (feb +0x15F) the attached sfx entity (cleared on msg 3)
    float        bobPeriod;    // v12 +0x17F (feb +0x163) 2.0 s. verified "mov [esi+0x17F],0x40000000"
    float        bobTime;      // v12 +0x183 (feb +0x167) verified
    float        bobOffset;    // v12 +0x187 (feb +0x16B) the Y displacement applied now. verified
    cdtINFO      floor;        // v12 +0x18B (feb +0x16F) the floor under it (INIT_LEVEL's FloorPlace; GetFloorInfo copies it)
    float        hideTimer;    // v12 +0x1AB (feb +0x18F) MP respawn: the server re-enables it when it runs out. verified (INIT)
    // size 0x1AF

    void Unlock(entIACTIVE_OBJ *by);                            // v12 0x00557460 (feb 0x0053E8B0) IAction PICK / UNLOCK
    static entENTITY *__fastcall Create(animINST *inst);        // v12 0x00557A50 (feb 0x0053EF60)
};
static_assert(sizeof(itemITEM) == 0x1AF, "itemITEM size (v12 0x1AF)");
int __fastcall itemInit(void);   // v12 0x00556CF0 (feb 0x0053E140) entRegisterClass("item",'MTI$',0x28,Create) + link sound list
extern sncSOUND_DESCR_3D itemUnlockSound; // v12 0x009AE648 (feb 0x00963320) "object_portal_appear", played on unlock when not moved

// Pick test (IsCanPick, v12 0x00557340): horizontal distance(item, player GetPos vslot +0x64) < 2.5
//   (v12 const 0x005F98A4, pooled, 38 users) AND |dy| < 2.3 (v12 0x005FA974; feb used 2.0). Then player
//   vslot (feb +0x9C) "accept item" decides, then Pick().
// FRAME: spin = gsElapsedTime * 70.0 (v12 0x005FB63C pooled), bob triangle wave over bobPeriod, amplitude bobAmp.

// =================================================================================================================
// doorDOOR -- animated door with OPEN/CLOSE sequences, opens for players (and NPCs) in radius.
// size v12 0x187 (feb 0x16B): Create v12 0x0055C0D0 "push 0x187". signature 'ROOD' (0x444F4F52), class "door".
// =================================================================================================================
enum doorFLAGS {
    DOORF_OPEN_STATE   = 0x0001, // fully open
    DOORF_INITED       = 0x0002,
    DOORF_ONE_WAY      = 0x0004, // DOOR.isOneWay: player must be in front (angle(door dir, to player) < 90)
    DOORF_NPC_OPEN     = 0x0008, // NPCs within radius may open it // ?
    DOORF_LOCKED_HOLD  = 0x0020, // isOpenOnce door that closed with every player behind it: never opens again
    DOORF_SHOWN_LOCKED = 0x0200, // ProcessMP_NOTIFY showed GAME_MSG_DOOR_LOCKED
    DOORF_CMD_OPEN     = 0x0040, // DOOR.isOpen or IAction "OPEN"
    DOORF_CMD_CLOSE    = 0x0080, // IAction "CLOSE"
    DOORF_OPEN_ONCE    = 0x0100, // DOOR.isOpenOnce
    DOORF_OPEN_FOREVER = 0x0400, // DOOR.isOpenForever
};
class doorDOOR : public entENTITY {
public:
    // vtable v12 0x00609158 (feb 0x005DDC90). The overrides of the entENTITY slots:
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);       // +0x04 v12 0x0055B290 (feb 0x00543B40)
    virtual void   ProcessUNLOCK();                                               // +0x30 v12 0x0055C060 (feb 0x00544A90)
    virtual void   ProcessMP_GET_SYNC_DATA(void *d);                              // +0x34 v12 0x0055BEB0 (feb 0x005447F0)
    virtual void   ProcessMP_WRITE_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d);        // +0x38 v12 0x005567C0 (feb 0x00544880) folded with button's
    virtual void   ProcessMP_READ_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d);         // +0x3C v12 0x0055BF40 (feb 0x00544970)
    // the door's own slots (v12 from +0x64; feb from +0x68):
    virtual BOOL32 ProcessINIT();                                                 // +0x64 v12 0x0055B410 (feb 0x00543C10)
    virtual void   ProcessFRAME();                                                // +0x68 v12 0x0055B670 (feb 0x00543E60)
    virtual void   ProcessTERM();                                                 // +0x6C v12 0x00403A20 (empty)
    virtual BOOL32 IsReadyOpen();                                                 // +0x70 v12 0x0055B990 (feb 0x00544120) HOOK: return 1 to force open
    virtual void   ProcessMP_NOTIFY(msgDATA *ev);                                 // +0x74 v12 0x0055BD10 (feb 0x00544710)

    unsigned int flags;        // v12 +0x16B (feb +0x14F) doorFLAGS. verified
    int          seqOpen;      // v12 +0x16F (feb +0x153) animTPL seq "OPEN". verified
    int          seqClose;     // v12 +0x173 (feb +0x157) seq "CLOSE". verified
    float        radOpen;      // v12 +0x177 (feb +0x15B) DOOR.radOpen, default 18.0 (feb 15.0). verified
    float        closeLen;     // v12 +0x17B (feb +0x15F) CLOSE length in s (the OPEN sequence is stretched to it). verified
    float        lockMsgTimer; // v12 +0x17F (feb +0x163) runs while entFlags & 4; the "door locked" notify needs > 2.0 s
    float        lockShowTimer;// v12 +0x183 (feb +0x167) runs while entFlags & 2; the HUD shows "locked" again after 8.0 s
    // DOOR.nmbLocks -> entIACTIVE_OBJ +0x30 (locked while > 0). size 0x187

    int IsPlrBeforeDoor(plgsACTION *p);                     // feb 0x00544680; not in 1.2 (folded into IsInRange)
    BOOL32 IsInRange(entENTITY *e, float rad);              // v12 0x0055BB20 (1.2 only, name ours) radius + one-way test
    static entENTITY *__fastcall Create(animINST *inst);    // v12 0x0055C0D0 (feb 0x00544B00)
};
int __fastcall doorInit(void);         // v12 0x0055B250 (feb 0x00543B00)
extern float doorDefaultRadOpen;       // v12 0x00637944 = 18.0 (feb 0x005F970C = 15.0), 1 reader: safe TWEAK
// Strings used for HUD notify: "GAME_MSG_DOOR_LOCKED" v12 0x006379E0, "GAME_MSG_DOOR_UNLOCKED".

// =================================================================================================================
// bttnBUTTON -- push button / lever with PUSH and POP sequences, can be shot.
// size v12 0x17B (feb 0x15F): Create v12 0x00556C00 "push 0x17B". signature 'NTTB' (0x4254544E), class "button".
// =================================================================================================================
enum bttnFLAGS {
    BTTNF_PUSHED     = 0x01, BTTNF_PUSHING = 0x02, BTTNF_UP = 0x04, BTTNF_POPPING = 0x08,
    BTTNF_PUSH_ONCE  = 0x10, // BUTTON.isPushOnce
    BTTNF_WAS_PUSHED = 0x20,
    BTTNF_NO_SHOOT   = 0x40, // cleared by BUTTON.isCanShoot; when clear, GS_MSG_DAMAGE toggles the button
};
class bttnBUTTON : public entENTITY {
public:
    // vtable v12 0x00608F40 (feb 0x005DD9A0)
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);       // +0x04 v12 0x00556240 (feb 0x0053D8D0) INIT in msg 1
    virtual void   ProcessMP_GET_SYNC_DATA(void *d);                              // +0x34 v12 0x00556730 (feb 0x0053DD40)
    virtual void   ProcessMP_WRITE_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d);        // +0x38 v12 0x005567C0 (feb 0x00544880)
    virtual void   ProcessMP_READ_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d);         // +0x3C v12 0x005568B0 (feb 0x0053DDD0)
    virtual BOOL32 Toggle(entENTITY *by);                                         // +0x64 v12 0x005569D0 (feb 0x0053DEF0) HOOK: "use" of a button
    virtual BOOL32 IsCanPush(entENTITY *by);                                      // +0x68 v12 0x00556C60 (feb 0x0053E0B0)

    unsigned int flags;        // v12 +0x16B (feb +0x14F) bttnFLAGS, verified (ctor)
    int          seqPush;      // v12 +0x16F (feb +0x153) verified (ctor -1)
    int          seqPop;       // v12 +0x173 (feb +0x157) verified (ctor -1)
    float        pushLen;      // v12 +0x177 (feb +0x15B) PUSH length in s; POP starts there
    // BUTTON.nmbLocks -> +0x30. size 0x17B. Events raised on entity +0x80 (IAction triggers): 1 push start,
    // 2 pop start, 4 pushed, 8 popped, 0x800 initialised (1.2) -- level scripts hang their IActions on these.
    static entENTITY *__fastcall Create(animINST *inst);    // v12 0x00556C00 (feb 0x0053E050)
};
int __fastcall bttnInit(void);         // v12 0x00556200 (feb 0x0053D890)

// =================================================================================================================
// trmpTRAMPOLINE -- jump pad. Throws whoever lands on it either toward a target instance or along its dir.
// size v12 0x197 (feb 0x17B): Create v12 0x00558B20 "push 0x197". signature 'PMRT' (0x54524D50), class "trampoline".
// =================================================================================================================
class trmpTRAMPOLINE : public entENTITY {
public:
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);       // +0x04 v12 0x005586E0 (feb 0x00543640)
    virtual void   ProcessINIT();                                                 // +0x64 v12 0x005584B0 (feb 0x00543420)
    m3dV   unk16B;             // v12 +0x16B (feb +0x14F) m3dVUndef (Create), not read // ?
    float  timer;              // v12 +0x177 (feb +0x15B) 0..5 s: the instance is invalidated every 5 s
    float  height;             // v12 +0x17B (feb +0x15F) TRAMPOLINE.height (20.0); auto 15..30 from target distance // ? v12 by rule
    int    hasTarget;          // v12 +0x17F (feb +0x163) TRAMPOLINE.target found. verified
    m3dV   targetPos;          // v12 +0x183 (feb +0x167) // ? v12 by rule
    float  randShift;          // v12 +0x18F (feb +0x173) TRAMPOLINE.randShift // ?
    float  speedXZ;            // v12 +0x193 (feb +0x177) TRAMPOLINE.speedXZ (no target) // ?
    // size 0x197
    // vtable v12 0x00609030 (feb 0x005DDC20)
    // slot +0x04 v12 0x005586E0 (feb 0x00543640) ProcessMsg
    // slot +0x64 v12 0x005584B0 (feb 0x00543420) ProcessINIT verified (strings, +0x17F)
    void GetSpeedThrow(float gravity, m3dV *outVel);        // v12 0x00558920 (feb 0x00543870) called from ProcessMsg 0x005587F9/0x005588CC HOOK: launch velocity
    static entENTITY *__fastcall Create(animINST *inst);    // v12 0x00558B20 (feb 0x00543A80)
};
int __fastcall trmpInit(void);         // v12 0x00558480 (feb 0x005433F0)
// Auto height from horizontal distance d to target: d<20 -> 15, d>60 -> 30, else (d-20)*0.375+15.
//   v12 constants: 20.0 @0x005F93F0, 15.0 @0x005FA490, 60.0 @0x005FB65C, 30.0 @0x005FA4C8, 0.375 @0x005FB120 (pooled).

// =================================================================================================================
// dstrDESTROY -- destructible prop ("dyn_destroy"): accumulates damage, swaps geom_orig -> geom_debris, spawns
// rigid debris pieces / shatter. Base of wrsfxDESTROY_BARREL/VASE/MIRROR/FLAME (game side).
// size v12 0x183 (feb 0x167): Create v12 0x00553FF0 "push 0x183". signature 'TSD$' (0x24445354), class "dyn_destroy".
// Geometry naming contract in the model: "geom_orig", "geom_debris", "geom_debris_left".
// Properties (section DESTROY): damage (2.0 = HP), iniSpeed (4.0), iniMomentum (0.05), isPlaceGround.
// =================================================================================================================
enum dstrFLAGS {               // dstrDESTROY::flags
    DSTRF_ANIMATING = 0x01,    // the debris animation plays (a template with sequences)
    DSTRF_BROKEN    = 0x02,    // the animation is over (or broken silently): the debris waits, then fades
    DSTRF_THIS_FRAME= 0x04,    // broken this frame (ProcessFRAME clears it): no second break from the same hit
    DSTRF_FADING    = 0x08,
};
class dstrDESTROY : public entENTITY {  // vtable v12 0x00608E48 (feb 0x005DD898)
public:
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);            // +0x04 v12 0x00554030 (feb 0x0053B720)
    void   ProcessMP_GET_SYNC_DATA(void *d);                                   // +0x34 v12 0x00554410 (feb 0x0053BB00)
    void   ProcessMP_READ_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d);              // +0x3C v12 0x00554480 (feb 0x0053BB70)
    virtual BOOL32 ProcessINIT();                                              // +0x64 v12 0x005540B0 (feb 0x0053B7B0)
    virtual void   ProcessFRAME();                                             // +0x68 v12 0x005541D0 (feb 0x0053B8D0)
    virtual void   ProcessTERM();                                              // +0x6C v12 0x00403A20 (empty)
    virtual void   ProcessCDT_FIREABLE(struct dmgDAMAGE *d);                   // +0x70 v12 0x00554370 (feb 0x0053BA70)
    virtual void   ProcessDESTROY_ENT(entENTITY *e);                           // +0x74 v12 0x00403950 (empty)
    virtual void   ProcessINIT_LEVEL();                                        // +0x78 v12 0x00554180 (feb 0x0053B880)
    virtual void   ProcessMP_NOTIFY(msgDATA *ev);                              // +0x7C v12 0x005543C0 (feb 0x0053BAC0)
    virtual void   StartExplos(int silent);                                    // +0x80 v12 0x00554570 (feb 0x0053BC60)
    static entENTITY *__fastcall Create(animINST *inst);                       // v12 0x00553FF0 (feb 0x0053B6E0)

    unsigned int flags;        // v12 +0x16B (feb +0x14F) dstrFLAGS
    float  fadeDelay;          // v12 +0x16F (feb +0x153) 5 s after the animation, then the debris fades
    float  fadeTime;           // v12 +0x173 (feb +0x157) 1 s, counting down while it fades
    float  fadeLen;            // v12 +0x177 (feb +0x15B) 1 s
    float  health;             // v12 +0x17B (feb +0x15F) [DESTROY] damage (2)
    float  damageTaken;        // v12 +0x17F (feb +0x163) the damage so far
};
// size v12 0x183
class rgdSHATTER_RIGID_START {                              // the model shattered into pieces (shatter.obj, not rebuilt)
public:
    void StartExplos(entENTITY *e, int n, float speed, float momentum, int a, int b);  // v12 0x00516720
};
extern rgdSHATTER_RIGID_START dstrShatterStart;            // v12 0x009AE5C0
int __fastcall dstrInit(void);         // v12 0x00553CA0 (feb 0x0053B390)

// =================================================================================================================
// gm_shared: library init/term. Called once by the game (gmSHARED). Order (v12 0x00557AE0, each failure logs
// "%s() failed" and returns 0):
//   dmgInit 0x005461D0, flrInit 0x00554C20, waterInit 0x0054B0F0, rwlInit 0x005602A0, blklInit 0x00548680,
//   rplInit 0x0055ED10, peffInit 0x0055CFD0, aiInit 0x005478D0, dstrInit 0x00553CA0, domMiscInit 0x0055C130,
//   itemInit 0x00556CF0, doorInit 0x0055B250, trmpInit 0x00558480, cinInit 0x00544100, bttnInit 0x00556200,
//   cauInit 0x00559A70.
// =================================================================================================================
// =================================================================================================================
// water / lava (gs_shared:water.obj): a template of class "water" or "lava" whose objects are the surfaces
// ([WATER] waves, texture scroll / animation, REFLECTION) and "&fog" objects outlining the volume, which becomes a
// ground-fog shader's volumes ([FOG] depth: how far below the surface). Each one registers in waterTable for the
// player's queries (waterIsUnderWater).
// =================================================================================================================
// waterLIQUID / waterWATER / waterLAVA: core/wr_bases.h (wrsfxLAVA derives from waterLAVA)
int __fastcall waterInit(void);                                  // v12 0x0054B0F0 (feb 0x0052FED0) registers both
extern waterLIQUID *waterTable[30];                              // v12 0x009A89F8 (feb 0x0095D6E8)
BOOL32 __fastcall waterIsUnderWater(m3dV *p, float *level, float r); // v12 0x0054B130

int  __fastcall gmSharedInit(void);    // v12 0x00557AE0 (feb 0x0053EFF0) HOOK: register your own classes after it
void __fastcall gmSharedTerm(void);    // v12 0x00557CD0 (feb 0x0053F1E0) starts with the same call chain (cinTerm/domMiscTerm/aiTerm)

#pragma pack(pop)
