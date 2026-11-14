// input.h -- Will Rock 1.2 unofficial SDK, game_systems: action mapping (gs:gs_input + wr_input contexts)
// gsINP_SYSTEM owns an engine inpINPUT (DirectInput 7 (DirectInputCreateW, version 0x700) lives in the engine input driver, not here) and up to 8
// "contexts" (gsiCONTEXT). A context = a table of commands (gsiCOMMAND, 0x9C bytes) each bound to key sequences;
// layouts are read from / written to a psSHEET file (GetFileName/GetSectionName). The current context is polled
// each frame by ProcessInput; game code asks "is command N active / axis value".
// Assumes from core: msgADDR, inpINPUT (engine), psSHEET. Layout of gsINP_SYSTEM identical in both builds
// (ctor v12 0x0052BB30 == feb 0x005166A0; SetCurCtx v12 0x0052BE20 uses +0x40/+0x44 like feb).
#pragma once
#include "gs_common.h"

struct gsiKEY_SEQ { char *Read(char *s); };          // Read v12 0x0052B400 (feb 0x00515F70) parses "KEY+KEY" text
struct gsiCOMMAND {                                   // size 0x9C (both builds: push 0x9C at v12 0x0052B783)
    unsigned flags;                                   // +0x00 0x40 = axis/analog command (wrINP_CTX_GAME::Init sets it on 5,6,0xC,0x25)
    // +0x04.. key sequences (Get/Set/FindKSeq), +0x44 float value (scaled by mouse sensitivity) // ? exact
    u8   _04[0x9C - 0x04];                            // (size 0x9C: gsINP_SYSTEM::IsOn indexes the array)
    int  Read(char *s);                               // v12 0x0052B480 (feb 0x00515FF0)
    void GetInput(inpINPUT *in, int, int);            // v12 0x0052B510 (feb 0x00516080)
    void GetKSeq(int, int*, int*);                    // v12 0x0052B680
    void SetKSeq(int, int, int*);                     // v12 0x0052B6C0
    int  FindKSeq(int, int*);                         // v12 0x0052B6F0
};
class gsiCONTEXT {                                    // vtbl v12 0x00605E68 (feb 0x005DCDB0?), 16 slots, see vtables.txt
public:
    gsiCOMMAND *cmds;          // +0x04 v12 (feb +0x04) array [last-first]
    int         firstCmd;      // +0x08 v12 (feb +0x08)
    int         lastCmd;       // +0x0C v12 (feb +0x0C)   verified ctor v12 0x0052B760
    gsiCONTEXT(int first, int last);                 // v12 0x0052B760 (feb 0x005162D0)
    virtual ~gsiCONTEXT();                           // +0x00
    virtual int  Init();                             // +0x04 v12 0x0052B780 (feb 0x005162F0) new cmds (0x9C each)
    virtual void Term();                             // +0x08 v12 0x0052B7E0
    virtual void Clear();                            // +0x0C v12 0x0052B800 (called on activate/deactivate)
    virtual const char *GetName();                   // +0x10 "GAME", "UI", "CIN" ...
    virtual void GetCmdName(int id, char *out);      // +0x14 command id -> "MOVE_FORWARD"...
    virtual const char *GetFileName();               // +0x18 v12 0x0052B840
    virtual const char *GetSectionName();            // +0x1C v12 0x0052B850
    virtual int  LoadLayout();                       // +0x20 v12 0x0052B860 (feb 0x005163D0)  HOOK: custom bindings
    virtual int  SaveLayout();                       // +0x24 v12 0x0052B930 (feb 0x005164A0)
    virtual int  ActionKey(); virtual int AxisXForw(); virtual int AxisXBack(); // +0x28.. +0x30
    virtual int  AxisYForw(); virtual int AxisYBack(); virtual int MaxNmbFrSwAxis(); // +0x34.. +0x3C
};
// Contexts: wrINP_CTX_GAME vtbl v12 0x00605E28 (Init v12 0x004A7DE0, GetCmdName v12 0x004A8040), wrINP_CTX_GAME_DEFAULTS v12 0x00605DE8,
//           wrINP_CTX_UI v12 0x00605DA8, gsiCONTEXT_CINEMATIC v12 0x006082A0 ("CIN", LoadLayout v12 0x0052BAF0)

// Game command ids (wrINP_CTX_GAME::GetCmdName, feb 0x00499F20). 1..0x18 read from the feb switch; the v12 string pool
// (0x00632CA0 MOVE_FORWARD, ascending addresses descending ids) holds the rest: WPN_7..WPN_11, WPN_NEXT, WPN_PREV,
// ACTIVATE_QUAD_DAMAGE, ACTIVATE_UNHOLY_ARMOR, ACTIVATE_SLOW_MOTION, FORCE_RECHARGE, SHOOT, DBG_1..9, STOP_CINEMATIC, PAUSE,
// SHOWFRAGS, QUICKSAVE, QUICKLOAD, TOGGLE_TEAM, SCREENSHOT, CHAT_MSG_ALL, CHAT_MSG_TEAM, MOUSE_BTN, MS_FORWARD/BACKWARD/LEFT/RIGHT.
enum wrINP_GAME_CMD {
    CMD_MOVE_FORWARD = 1, CMD_MOVE_BACKWARD, CMD_LOOK_UP, CMD_LOOK_DOWN, CMD_ROTATE_LEFT, CMD_ROTATE_RIGHT,
    CMD_MOVE_UP, CMD_MOVE_DOWN, CMD_MOVE_LEFT, CMD_MOVE_RIGHT, CMD_LOOK_CENTER /*0xB*/, CMD_SWITCH_MODE /*0xC*/,
    CMD_ACCEL /*0xD*/, CMD_JUMP /*0xE*/, CMD_CROUCH /*0xF*/, CMD_USE = 0x10, CMD_UI, CMD_ZOOM, CMD_WPN_1 = 0x13,
    CMD_WPN_2, CMD_WPN_3, CMD_WPN_4, CMD_WPN_5, CMD_WPN_6 /*0x18*/,
    // >= 0x19: v12 wrINP_CTX_GAME::GetCmdName 0x004A8040 (jump table 0x004A8568), read case by case
    CMD_WPN_7 /*0x19*/, CMD_WPN_8, CMD_WPN_9, CMD_WPN_10, CMD_WPN_11 /*0x1D*/, CMD_WPN_NEXT /*0x1E*/, CMD_WPN_PREV,
    CMD_ACTIVATE_QUAD_DAMAGE /*0x20*/, CMD_ACTIVATE_UNHOLY_ARMOR, CMD_ACTIVATE_SLOW_MOTION, CMD_FORCE_RECHARGE /*0x23*/,
    CMD_SHOOT /*0x24*/, CMD_STOP_CINEMATIC, CMD_PAUSE, CMD_SHOWFRAGS /*0x27*/, CMD_QUICKSAVE, CMD_QUICKLOAD,
    CMD_TOGGLE_TEAM /*0x2A*/, CMD_SCREENSHOT, CMD_CHAT_MSG_ALL, CMD_CHAT_MSG_TEAM /*0x2D*/,
    CMD_DBG_1 /*0x2E*/, CMD_DBG_2, CMD_DBG_3, CMD_DBG_4, CMD_DBG_5, CMD_DBG_6, CMD_DBG_7, CMD_DBG_8, CMD_DBG_9 /*0x36*/,
};

// re/mpsrc view of a gsiCONTEXT's data: the command range and the 0x9C-byte command records.
struct gsINP_ACTIONS {
    void       *vtbl;                    // +0x00 (gsiCONTEXT vtable)
    gsiCOMMAND *states;                  // +0x04 = gsiCONTEXT::cmds, index cmd - first
    int         first;                   // +0x08 = firstCmd
    int         last;                    // +0x0C = lastCmd
};
class gsINP_SYSTEM : public msgADDR {   // object = wrINP_SYSTEM (0x70). vtbl gs v12 0x00608274, wr v12 0x0060616C. gsSysInput v12 0x0098E838
public:  // +0x1C = 0x0F priority
    int            addrLock;               // +0x024 msgADDR's lock count (msgSYSTEM skips a locked address; the C++ msgADDR
                                           //   ends at 0x24, each class declares it; zeroed by the ctor)
    int         _28[3];                  // +0x028..+0x030
    float       mouseDX, mouseDY;        // +0x034/+0x038 mouse movement of this frame (ControlFirstPerson * 0.2)
    inpINPUT   *input;                   // +0x03C v12 (feb +0x03C) engine input (Init(_apHMainWindow) = DirectInput 7 in the engine: DirectInputCreateW, version 0x700; see platform_drv/input.h)
    union { gsiCONTEXT *curCtx; struct gsINP_ACTIONS *actions; }; // [alias actions: re/mpsrc]
    //          curCtx                   // +0x040 v12 (feb +0x040) verified v12 0x0052BE3F / cinSYSTEM 0x0054446C
    gsiCONTEXT *ctx[8];                  // +0x044 v12 (feb +0x044) verified lea [edi+0x44] v12 0x0052BE2A, cmp 8
    int         _64;                     // +0x064
    virtual int  ProcessMsg(int,void*,msgADDR*,int*); // +0x04 v12 0x0052BBD0 (feb 0x00516740)
    virtual int  Init();                 // +0x10 v12 0x0052BC00 (feb 0x00516770) INI [Input] HideMouse, SyncState, UseJoy
    virtual void Term();                 // +0x14 v12 0x0052BD50
    virtual void ProcessInput();         // +0x18 v12 0x0052BFD0 (feb 0x00516B40) HOOK: per-frame command polling
    virtual gsiCONTEXT *GetCtxByName(const char*); // +0x1C v12 0x0052BDB0 (feb 0x00516920)
    virtual void SetDefCtx();            // +0x20 v12 0x0052BE60 (wr v12 0x004A87B0)
    virtual void AddCtxes();             // +0x24 v12 0x0052BEB0 (wr v12 0x004A8700)
    virtual void DestroyCtx(gsiCONTEXT*);// +0x28 v12 0x0052BF10
    void SetCurCtx(gsiCONTEXT *c);       // v12 0x0052BE20 (feb 0x00516990)
    int  IsToggled(int cmd);             // v12 0x00425D20 (feb 0x004226A0) command changed this frame, down or up (flags & 2;
                                         //   ProcessInput tests !IsOn && IsToggled for a release)
    int  IsPressed(int cmd);             // v12 0x00425D50 (1.2 only, name ours) IsOn && IsToggled, out of line
    int  IsOn(int cmd)                   // v12 0x00425CF0 (feb 0x00422670), inline everywhere: command held (flags & 1)
    { gsINP_ACTIONS *a = actions; return cmd >= a->first && cmd < a->last && (a->states[cmd - a->first].flags & 1); }
    int  FindFreeCtxIdx();               // v12 0x0052BF80
    int  SaveLayout();                   // v12 0x0052BFA0
    void Acquire();                      // v12 0x0052C5D0 (feb 0x00517130)
    void ApplyMouseSettings();           // v12 0x0052C5E0 (feb 0x00517140)
    void FlushBuffers();                 // v12 0x0052C670
    int  ApplySettings();                // v12 0x0052C680 (feb 0x005171E0)
    void SetMouseMode(int);              // v12 0x0052C710 (feb 0x00517270)
};
