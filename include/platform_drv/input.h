// Will Rock 1.2 unofficial SDK - platform_drv: input driver (drv:inp.obj, inp_intr.obj, inp_kbd.obj, inp_ms.obj,
// inp_joy.obj, inp_FF.obj).  API: DirectInput 7 (DirectInputCreateW, version 0x700, fallback 0x300).
// Assumes from re/sdk/core: m3dV (unused here), msgADDR (gsINP_SYSTEM::ProcessMsg).
// Static analysis only. v12 = 1.2 (willrock.exe 2003-07-30), feb = 2003-02-21 reference build.
// Every v12 offset below was checked in at least one v12 instruction unless marked "// ?".
// The input driver is byte-identical in layout between feb and v12 (object size 0x2FEC in both:
//   inpINPUT::Make v12 0x0058E690 "push 0x2FEC", feb 0x00575D80 "push 0x2FEC").
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include <stdint.h>

// ---------------------------------------------------------------------------------------------
// Channel model. inpINPUT keeps one float per "channel"; 0x154 (340) channels.
//   0..255   keyboard, indexed by DirectInput scan code (DIK_*). 1.0 = down.
//   256..259 mouse buttons 0..3      (written at +0x40C = channel 256, GetMouseInput; DIMOFS_BUTTON0..3 = 0x0C..0x0F + 244)
//   260..291 joystick buttons 0..31  (+0x41C, GetJoystickInput)
//   300..339 20 analogue axes, each a pair (+ direction, - direction): GetInput splits a negative value into the
//            next channel. 300/301 mouse X, 302/303 mouse Y, 304/305 wheel, 306.. joystick X,Y,Z,Rx,Ry,Rz,slider0,1
//            (+0x4D4..+0x50C) then 2 POV hats.
// gsiCOMMAND (gs_input) binds a command to a key sequence of channel numbers; key names come from
// _inpINPUT::GetKeyName (DirectInput GetObjectInfo / joystick object names).
// ---------------------------------------------------------------------------------------------
enum {
    INP_NCHANNELS      = 0x154,     // 340
    INP_CH_MOUSE_BTN0  = 256,
    INP_CH_JOY_BTN0    = 260,
    INP_CH_AXIS0       = 300,       // mouse X+
    INP_CH_MOUSE_YPOS  = 302,
    INP_CH_WHEEL       = 304,
    INP_CH_JOY_AXIS0   = 306,
};

enum inpFLAGS {                      // inpINPUT::flags (+0x04)
    INP_F_KEYBOARD   = 0x01,         // GetInput calls slot +0x48 GetKeyboardInput
    INP_F_MOUSE      = 0x02,         // GetInput calls slot +0x5C GetMouseInput
    INP_F_JOYSTICK   = 0x04,         // GetInput calls slot +0x68 GetJoystickInput (never set: InitJoystick is a stub)
    INP_F_FF         = 0x08,         // force feedback enabled (EnableFF)
    INP_F_MOUSE_BUF  = 0x20,         // mouse buttons also read through buffered GetDeviceData
    INP_F_MOUSE_ABS  = 0x40,         // ? mouse axes go to the absolute accumulator instead of the delta channels
};

enum inpCHFLAGS {                    // per-channel byte flags (+0xFFC[340])
    INP_CH_RELEASED  = 0x08,         // set by ReleaseKey, cleared every GetInput
};

// DirectInput property / cooperative values used (for a port these are the semantics to reproduce)
//   keyboard: c_dfDIKeyboard, SetCooperativeLevel(hwnd, 6 = NONEXCLUSIVE|FOREGROUND), DIPROP_BUFFERSIZE = 30
//   mouse:    c_dfDIMouse (DIMOUSESTATE, 0x10 bytes), coop 0x15 (EXCLUSIVE|FOREGROUND|NOWINKEY) when a window
//             is given, 6 otherwise; DIPROP_BUFFERSIZE = 30
//   joystick: c_dfDIJoystick (DIJOYSTATE, 0x50 bytes), coop 5 (EXCLUSIVE|BACKGROUND?) / 6, DIPROP_DEADZONE

class inpINPUT {                     // drv:inp.obj, abstract base; vtable v12 0x0060A568 is _inpINPUT's
public:
    // ---- vtable (as _inpINPUT, v12 0x0060A568, feb 0x005DF068), 29 slots, slot order identical in both builds
    virtual ~inpINPUT();                                 // slot +0x00 v12 0x0058F2C0 (feb 0x005769B0)
    virtual int  Init(int *chanMap);                     // slot +0x04 v12 0x0058E7A0 (feb 0x00575E90) DirectInputCreateW then inpINPUT::Init
    virtual void Term();                                 // slot +0x08 v12 0x004B6270 (feb 0x004A6590)
    virtual void SyncAcquire(int acquire);               // slot +0x0C v12 0x0058E8C0 (feb 0x00575FB0) Acquire/Unacquire kbd, mouse, joy
    virtual int  SetInputWnd(void *hwnd, int);           // slot +0x10 v12 0x0058E800 (feb 0x00575EF0) SetCooperativeLevel on every device
    virtual void SetMouseSensitivity(float s);           // slot +0x14 v12 0x00594510 (feb 0x0057A1F0) clamps to [0.01, 4.0]
    virtual void GetMouseInfo(struct inpMOUSE_INFO *);   // slot +0x18 v12 0x00594980 (feb 0x0057A660) GetSystemMetrics/GetDoubleClickTime/timeGetTime
    virtual void SetMousePos(float x, float y);          // slot +0x1C v12 0x005948D0 (feb 0x0057A5B0)
    virtual void SetJoystickDeadZone(float);             // slot +0x20 v12 0x005941E0 (feb 0x00579EC0) DIPROP_DEADZONE, stores +0x2540
    virtual int  EnableFF(int on);                       // slot +0x24 v12 0x00593AF0 (feb 0x005797C0)
    virtual int  CreateFF(int id, uint32_t type, uint32_t flags, ...); // slot +0x28 v12 0x00593B70 (feb 0x00579840) // ? v12 by slot
    virtual int  DestroyFF(int id);                      // slot +0x2C v12 0x00594050 (feb 0x00579D20)
    virtual int  PlayFF(int id);                         // slot +0x30 v12 0x005940B0 (feb 0x00579D80)
    virtual int  StopFF(int id);                         // slot +0x34 v12 0x00594110 (feb 0x00579DE0)
    virtual int  IsPlayFF(int id);                       // slot +0x38 v12 0x00594160 (feb 0x00579E30)
    virtual int  GetKeyName(int ch, uint16_t *out);      // slot +0x3C v12 0x0058E960 (feb 0x00576050) // ? v12 by slot
    // private driver slots (_inpINPUT)
    virtual int  InitKeyboard();                         // slot +0x40 v12 0x00594C10 (feb 0x0057A8F0) also fills the DIK->VK table
    virtual void TermKeyboard();                         // slot +0x44 v12 0x00595090 (feb 0x0057AD70)
    virtual void GetKeyboardInput();                     // slot +0x48 v12 0x00595250 (feb 0x0057AF30)
    virtual void SetKeyboardLayout(int);                 // slot +0x4C v12 0x005950B0 (feb 0x0057AD90)
    virtual void FlushKeyboard();                        // slot +0x50 v12 0x00595200 (feb 0x0057AEE0)
    virtual int  InitMouse();                            // slot +0x54 v12 0x00594450 (feb 0x0057A130)
    virtual void TermMouse();                            // slot +0x58 v12 0x005944F0 (feb 0x0057A1D0)
    virtual void GetMouseInput();                        // slot +0x5C v12 0x00594560 (feb 0x0057A240) GetDeviceState(DIMOUSESTATE)
    virtual int  InitJoystick();                         // slot +0x60 v12 0x00502570 (feb 0x00579E90) STUB "xor eax,eax; ret" (shared COMDAT)
    virtual void TermJoystick();                         // slot +0x64 v12 0x005941C0 (feb 0x00579EA0)
    virtual void GetJoystickInput();                     // slot +0x68 v12 0x00594270 (feb 0x00579F50) GetDeviceState(DIJOYSTATE) into a global
    virtual int  InitFF();                               // slot +0x6C v12 0x00593A60 (feb 0x00579730)
    virtual void TermFF();                               // slot +0x70 v12 0x00593AC0 (feb 0x00579790)

    // ---- non-virtual (inp.obj)
    int  InitBase(int *chanMap);                         // inpINPUT::Init       v12 0x004B61F0 (feb 0x004A6510)
    void FlushBuffers();                                 // v12 0x004B62A0 (feb 0x004A65C0)
    int  StartGetShortcut();                             // v12 0x004B62B0 (feb 0x004A65D0) start "press a key" capture (options menu)
    int  ContinueGetShortcut(int, int *, int *);         // v12 0x004B6340 (feb 0x004A6660)
    int  GetShortcutName(int *seq, int n, uint16_t *out, int, int); // v12 0x004B6540 (feb 0x004A6860)
    void GetMultipleKeyValue(int n, int *seq, float *, float *, float *); // v12 0x004B6660 (feb 0x004A6980) value of a key chord
    void ReleaseKey(int n, int *seq);                    // v12 0x004B67B0 (feb 0x004A6AD0) sets INP_CH_RELEASED
    void GetInput();                                     // v12 0x004B67F0 (feb 0x004A6B10) per frame: prev=cur, poll devices, split axes, remap
    static inpINPUT *Make();                             // v12 0x0058E690 (feb 0x00575D80) new(0x2FEC) + _inpINPUT ctor (v12 0x0058E6B0)

    // ---- data (v12 offsets == feb offsets)
    uint32_t flags;               // v12 +0x0004 (feb +0x0004) inpFLAGS
    uint32_t _08;                 // v12 +0x0008 (feb +0x0008)
    float    cur[INP_NCHANNELS];  // v12 +0x000C (feb +0x000C) current value per channel
    float    prev[INP_NCHANNELS]; // v12 +0x055C (feb +0x055C) previous frame (GetInput copies cur -> prev)
    float    held[INP_NCHANNELS]; // v12 +0x0AAC (feb +0x0AAC) subtracted from cur each frame (released-key mask); zeroed by SyncAcquire (v12 0x0058E8C0)
    uint8_t  chFlags[INP_NCHANNELS]; // v12 +0x0FFC (feb +0x0FFC) inpCHFLAGS; padded to +0x1150
    int32_t  remap[INP_NCHANNELS];   // v12 +0x1150 (feb +0x1150) channel redirect, -1 = none (ctor fills 0xFFFFFFFF)
};

class _inpINPUT : public inpINPUT {   // drv:inp_intr.obj; size 0x2FEC
public:
    void    *pDI;                 // v12 +0x16A0 (feb +0x16A0) IDirectInput7W* (Init: lea esi,[edi+0x16A0] at v12 0x0058E7AB)
    void    *hwnd;                // v12 +0x16A4 (feb +0x16A4) window given to SetInputWnd
    void    *pKeyboard;           // v12 +0x16A8 (feb +0x16A8) IDirectInputDevice7W* (SetInputWnd v12 0x0058E80B)
    uint8_t  kbdBuf[0x1E0];       // v12 +0x16AC (feb +0x16AC) DIDEVICEOBJECTDATA[30] buffered events (also used by mouse buffered read)
    uint8_t  _188c[0x100];        // v12 +0x188C ? keyboard state / scratch
    void    *pMouse;              // v12 +0x198C (feb +0x198C) IDirectInputDevice7W* (SetInputWnd v12 0x0058E82C, GetMouseInput v12 0x00594566)
    float    mouseSens;           // v12 +0x1990 (feb +0x1990) ctor default: v12 2.005 (0x400051EC at 0x0058E705), feb 2.05 (0x40033333); used by GetMouseInput fmul at v12 0x005945BA
    float    _1994[1];            // v12 +0x1994 ?
    float    mouseAbs[3];         // v12 +0x1998 (feb +0x1998) accumulated x, y, wheel (clamped to the screen)
    float    mouseDelta[3];       // v12 +0x19A4 (feb +0x19A4) this frame: lX/lY/lZ * sens * 0.025659
    float    _19b0;               // v12 +0x19B0 (feb +0x19B0) smoothing factor (x0.3)  // ?
    int32_t  dims[4];             // v12 +0x19B4 (feb +0x19B4) DIMOUSESTATE lX,lY,lZ,rgbButtons
    uint8_t  _19c4[0x18];         //  +0x19C4..+0x19D8
    void    *pJoystick;           // v12 +0x19DC (feb +0x19DC) IDirectInputDevice7W* - always NULL in shipped builds (InitJoystick stub)
    float    joySens;             // v12 +0x19E0 (feb +0x19E0) ctor default v12 2.005 (0x400051EC at 0x0058E75D), feb 2.05
    // +0x19E4 .. +0x2540 joystick object names / effect data (0x55F dwords zeroed by ctor)
    float    joyDeadZone;         // v12 +0x2540 (feb +0x2540)
    int      ffSupported;         // v12 +0x2544 (feb +0x2544) InitFF: DIDEVCAPS flags & DIDC_FORCEFEEDBACK
    void    *ffEffect[30];        // v12 +0x2548 (feb +0x2548) IDirectInputEffect*
    // +0x25C0 .. +0x2F5C effect parameters; +0x2F60/+0x2F64 = -1, +0x2F68.. ; +0x2FE8 last field
};

// Globals
extern uint8_t  g_inpJoyState[0x50];  // v12 0x009AF190 (feb 0x00963E28) DIJOYSTATE filled by GetJoystickInput (push 0x9AF190 at v12 0x0059428E)
extern int32_t  g_inpDik2Vk[0xFF];    // v12 0x009AF1F0 (feb 0x00963E88) scan code -> Windows VK, -1 = none; built in InitKeyboard
                                      //   (v12 0x00594CB3 "mov edi,0x9AF1F0; rep stosd" then per-key stores, [1]=VK_ESCAPE ...)
                                      //   Used for text entry/keyboard-layout key names. PORT: SDL scancode -> keycode.

// gs-side owner (gs:gs_input.obj): gsINP_SYSTEM, contexts gsiCONTEXT, commands gsiCOMMAND (bindings in the input .ini)
class gsINP_SYSTEM;
extern gsINP_SYSTEM *gsSysInput;      // v12 0x0098E838 (feb 0x00948238); +0x3C = inpINPUT* (Init v12 0x0052BC0D "mov [esi+0x3C],eax" after Make)
int  gsINP_SYSTEM_Init(gsINP_SYSTEM *);             // v12 0x0052BC00 (feb 0x00516770) Make, Init, SetInputWnd(apHMainWindow), contexts
void gsINP_SYSTEM_ProcessInput(gsINP_SYSTEM *);     // v12 0x0052BFD0 (feb 0x00516B40) per frame: inpINPUT::GetInput + commands -> game
void gsINP_SYSTEM_Acquire(gsINP_SYSTEM *);          // v12 0x0052C5D0 (feb 0x00517130) called from MainLoop every frame
int  gsINP_SYSTEM_ApplySettings(gsINP_SYSTEM *);    // v12 0x0052C680 (feb 0x005171E0) mouse sensitivity / invert from config
void gsiCOMMAND_GetInput(void *cmd, inpINPUT *, int, int); // v12 0x0052B510 (feb 0x00516080) evaluate one bound command
