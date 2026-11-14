// cinema.h -- Will Rock 1.2 unofficial SDK, game_systems: in-engine scripted cinematics (gs_shared:cinema)
// A cinema is an instance of class "cinema" (cinDATA_ENTITY) whose sheet holds a key list ([FRAME_n] actions,
// [FRAME_LAST]); it starts once its lock count ([CINEMA] nmbLocks, lowered by the level's UNLOCK IActions) is 0.
// cinSYSTEM plays it: input to the "CIN" context, GS_MSG_CINEMA_START; the cinema instance's first sequence runs and
// the scene's own animated camera follows its frame (camCAMERA::Animate into gsCameraPtr); each [FRAME_n] fires its
// actions as the frame passes; a fade spline darkens the screen (SetColorFullScr); at the end [FRAME_LAST] and
// GS_MSG_CINEMA_END. Rebuilt in src/gs_shared/cinema.cpp (v12 0x00544100..0x005461A0).
// Video (bink-like) cut-scenes are a different object: wrCUTSCENE (gspCutscene, vtbl v12 0x00606040).
#pragma once
#include "gs_common.h"
#include "sound.h"
#include "../engine_core/entity.h"

class m3dSPL_LINEAR1D;

// Messages besides GS_MSG_CINEMA_START / _END (gs_common.h), 1.2 ids (+2 against feb; names ours): to an actor's
// entity (the player's for "player_3d") when it starts acting, and when it stops.
#define GS_MSG_CINEMA_ACTOR_ON  0x4CC   // feb 0x4CA
#define GS_MSG_CINEMA_ACTOR_OFF 0x4CD   // feb 0x4CB

// One action of a frame (vtbl v12 0x00608718: Init 0x004EADA0 "return 1", UpdateFrame 0x00403950 empty).
class cinACTION {
public:
    virtual int  Init(psSHEET *s, char *sect, char *key);   // v12 0x004EADA0
    virtual void UpdateFrame(int frame);                    // v12 0x00403950
};
class cinACN_CREATE_ENT : public cinACTION {        // "create_": %sActor %sClassName %sTplName. new 0xC4, vtbl 0x00608730
public:
    char actor[0x40], cls[0x40], tpl[0x40];         // +0x04 / +0x44 / +0x84
    int  Init(psSHEET *s, char *sect, char *key);   // v12 0x00545860 (feb 0x00542AB0)
    void UpdateFrame(int frame);                    // v12 0x00545940 (feb 0x00542B90) entCreate(class, tpl)
};
class cinACN_DESTROY_ENT : public cinACTION {       // "destroy_": %sActor. new 0x44, vtbl 0x00608728
public:
    char actor[0x40];                               // +0x04
    int  Init(psSHEET *s, char *sect, char *key);   // v12 0x00545990 (feb 0x00542BE0)
    void UpdateFrame(int frame);                    // v12 0x00545A00 (feb 0x00542C50) PostMsg DESTROY (2)
};
class cinACN_ANIMATE : public cinACTION {           // "animate_": %sActor [%sSeqName]. new 0x84, vtbl 0x00608720
public:
    char actor[0x40], seq[0x40];                    // +0x04 / +0x44
    int  Init(psSHEET *s, char *sect, char *key);   // v12 0x00545A40 (feb 0x00542C90)
    void UpdateFrame(int frame);                    // v12 0x00545AF0 (feb 0x00542D40)
};
class cinACN_CONTRAINT : public cinACTION {         // (sic) "constraint_": %sActor %sActorTo [%sObjTo]. new 0xC4
public:                                             // vtbl 0x00608708
    char actor[0x40], actorTo[0x40], objTo[0x40];   // +0x04 / +0x44 / +0x84
    int  Init(psSHEET *s, char *sect, char *key);   // v12 0x00545B20 (feb 0x00542D70)
    void UpdateFrame(int frame);                    // v12 0x00545C00 (feb 0x00542E50)
};
class cinACN_COMMAND : public cinACTION {           // "command_": an IAction ("cinematic_command"). new 0xC0
public:                                             // vtbl 0x00608710
    entIACTION_DATA data;                           // +0x04 (0xBC)
    int  Init(psSHEET *s, char *sect, char *key);   // v12 0x00545C90 (feb 0x00542EE0)
    void UpdateFrame(int frame);                    // v12 0x00545CB0 (feb 0x00542F00) ExecuteIAction
};
class cinACN_FADE : public cinACTION {              // "fade_": %sFrameLen %sIsOut. new 0xC, vtbl 0x006086E8
public:
    int  frameLen;                                  // +0x04
    int  isOut;                                     // +0x08
    int  Init(psSHEET *s, char *sect, char *key);   // v12 0x00545CC0 (feb 0x00542F10)
    void UpdateFrame(int frame);                    // v12 0x00545D60 (feb 0x00542FB0) the fade spline from this frame on
};
class cinACN_SOUND : public cinACTION {             // "sound_": a gsSOUND_DESCR + %sActor [%sObj]. new 0x138
public:                                             // vtbl 0x00608700
    gsSOUND_DESCR descr;                            // +0x04 (0xB4)
    char actor[0x40], obj[0x40];                    // +0xB8 / +0xF8
    int  Init(psSHEET *s, char *sect, char *key);   // v12 0x00546010 (feb 0x00543260)
    void UpdateFrame(int frame);                    // v12 0x005460A0 (feb 0x005432F0)
};
class cinACN_MUSIC : public cinACTION {             // "music_": %sName. new 0x44, vtbl 0x006086F8; its UpdateFrame is
public:                                             // the shared empty one in 1.2 (v12 0x0051AAF0)
    char name[0x40];                                // +0x04
    int  Init(psSHEET *s, char *sect, char *key);   // v12 0x00546100 (feb 0x00543350)
};
class cinACN_CAMERA_CUT : public cinACTION {        // "camera_cut_": only the base's slots (vtbl 0x006086F0); its frame
};                                                  // gets cameraCut: the camera held on the frame's own time

struct cinFRAME {                  // size 0x34 (cinSYSTEM v12 +0x280, 50 of them; +0xCA8 the last)
    float      time;               // +0x00 the frame number (-1: unused)
    cinACTION *act[10];            // +0x04
    int        nAct;               // +0x2C
    int        cameraCut;          // +0x30
    int  Init(float t, psSHEET *s, char *sect);    // v12 0x00545350 (feb 0x005425A0)
};
struct cinACTOR {                  // size 0x18 (cinSYSTEM v12 +0x9C, 20 of them)
    animINST        *inst;         // +0x00 the acting instance (its entity gets flags2 0x10: not animated by itself)
    animINST        *conInst;      // +0x04 constrained to this instance ...
    objOBJ          *conObj;       // +0x08 ... or to this object of it
    int              seq;          // +0x0C the sequence it plays (-1 none)
    int              noAnimSys;    // +0x10 1: the template has no transition table (plain AdvanceTime)
    sncCHANNEL_CONE *chn;          // +0x14 its voice (PlaySound)
    int  Animate(char *seq);                        // v12 0x00544EC0 (feb 0x00542110)
    int  PlaySound(char *obj, gsSOUND_DESCR *d);    // v12 0x00544FB0 (feb 0x00542200)
    void Term();                                    // v12 0x00545120 (feb 0x00542370)
    void UpdateFrame();                             // v12 0x005451D0 (feb 0x00542420)
};

#pragma pack(push, 1)
class cinDATA_ENTITY : public entENTITY {           // class "cinema" 'CINE'; new 0x16F (feb 0x14F), vtbl v12 0x00608738
public:
    int isSuspendAction;                            // +0x16B (1.2) [CINEMA] isSuspendAction (Create: 1): the game in
                                                    //   mode 4 while it plays (gsMsgChangeMode 4 / 5)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);  // slot +0x04 v12 0x00544180 (feb 0x00541570)
    static entENTITY *__fastcall Create(animINST *i);                // v12 0x00546170 (feb 0x005433C0)
};
#pragma pack(pop)

class cinSYSTEM : public msgADDR {  // size v12 0xCE0 (feb 0xCDC); gsSysCinema v12 0x0098E840. vtbl v12 0x006086D0 (feb 0x005DDB48)
public:  // tag +0x10 'RDAM', +0x14 'CINE' (0x43494E45), +0x1C = 0x50 (verified v12 ctor 0x005441F0)
    int            addrLock;        // +0x024 msgADDR's lock count (msgSYSTEM skips a locked address; the C++ msgADDR
                                    //   ends at 0x24, each class declares it; zeroed by the ctor)
    gsiCONTEXT    *inpCtx;          // +0x028 input context "CIN" (GetCtxByName) verified v12 0x00544469
    int            nCinemas;        // +0x02C
    cinDATA_ENTITY *cinema[20];     // +0x030 the level's cinemas (INIT_LEVEL)
    union { unsigned state; unsigned flags; }; // [alias flags: re/mpsrc]
    //               state          // +0x080 1 playing, 2 the sequence ended, 4 enabled (wrGO: single player)
    animINST      *ctrlInst;        // +0x084 the playing cinema's instance (its frame +0xE4 drives everything)
    cinDATA_ENTITY *curEnt;         // +0x088 (1.2) the playing cinema
    psSHEET       *sheet;           // +0x08C (feb +0x88) its sheet (the instance's, else the template's)
    animSEQ       *curData;         // +0x090 (feb +0x8C) its first sequence: frameStart +0x40, frameEnd +0x44, length +0x50
    m3dSPL_LINEAR1D *fade;          // +0x094 (feb +0x90) the fade spline: frame -> brightness 0..1
    int            nActors;         // +0x098 (never raised: only ResetData's sweep of all 20 ends them)
    cinACTOR       actor[20];       // +0x09C
    int            nFrames;         // +0x27C
    cinFRAME       frame[50];       // +0x280
    cinFRAME       lastFrame;       // +0xCA8 [FRAME_LAST], run at the end
    int            curFrame;        // +0xCDC the next frame to fire (-1 none)
    cinSYSTEM();                                       // v12 0x005441F0 (feb 0x005415B0); dtor v12 0x005442D0
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);  // slot +0x04 v12 0x00544300 (feb 0x005416C0)
    virtual int  ProcessINIT_LEVEL();                  // slot +0x10 v12 0x005443D0 (feb 0x00541790) collect the cinemas
    virtual void ProcessFRAME();                       // slot +0x14 v12 0x00544450 (feb 0x00541810) HOOK: cinematics
    void     CreateData(cinDATA_ENTITY *e);            // v12 0x005448B0 (feb 0x00541C20)
    void     ResetData();                              // v12 0x00544B70 (feb 0x00541ED0)
    int      IsSuspendAction();                        // v12 0x00544C70 (1.2 only, name ours)
    void     Stop();                                   // v12 0x00544C90 (1.2 only, name ours) end the playing cinema
    cinACTOR *FindActor(char *name);                   // v12 0x00544D80 (feb 0x00541FD0)
};
int  __fastcall cinInit();   // v12 0x00544100 (feb 0x005414F0)
void __fastcall cinTerm();   // v12 0x00544160 (feb 0x00541550)
