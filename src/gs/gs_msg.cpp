// gs_msg.cpp -- gs.lib frame messages (Will Rock 1.2, v12.exe; feb obj gs:gs_msg.obj)
//
// The game's messages (GS_MSG_*) and one game frame of them: the delayed and posted messages due, then GS_MSG_FRAME to
// every address (in priority order), then the next frame's queue. PARTIAL: gsMsgInit, _gsMsgProcessFrameProfile and
// the time scale (gsMsgSet / GetElapsedTimeScale). Not yet: gsMsgCallFrame (v12 0x00510B90: the elapsed time, the
// FPS statistics), _gsMsgCalcElapsedTime.

#include <windows.h>
#include <mmsystem.h>
#include "../Sources/game.h"
#include "gs_mp_misc.h"

float gsTimeScale = 1.0f;               // v12 0x006354E8 (feb 0x005F72A0)
float gsElapsedTime;                    // v12 0x0098E9F4 (feb 0x009482DC)
float gsElapsedTimeLevel;               // v12 0x0098EA1C (feb 0x00948304)
int   _eioIsUpdSuspend;                 // v12 0x0098EA0C (feb 0x009482F4) set when gsMsgChangeMode suspends or resumes
float gsNextElapsedTime = -1.0f;        // v12 0x006354EC (1.2 only, name ours) one frame's time, taken once by
                                        //   _gsMsgCalcElapsedTime (v12 0x00510B5F): a cinema skipped to its end
u32   gsTime;                           // v12 0x0098EA14 (feb 0x009482FC)
static u32 gsStatTime;                  // v12 0x0098EA10 the FPS statistics' last time (gsMsgCallFrame)
static u32 gsStartTime;                 // v12 0x0098E9EC // ? read by _gsMsgCalcElapsedTime
static u32 gsStatFrame;                 // v12 0x0098EA18 the FPS statistics' last frame number

// v12 0x005107A0  (feb 0x004FC420)
// The names of the game's messages (debug), the clock started, the first frame, and the two ids the message system
// treats apart: CHG_MODE reaches even locked addresses, FRAME ends with the destroy purge.
BOOL32 __fastcall gsMsgInit()
{
    msgSystem.RegisterMsg(GS_MSG_FRAME, (char *)"GS_MSG_FRAME");
    msgSystem.RegisterMsg(0x3E9, (char *)"GS_MSG_RENDER");
    msgSystem.RegisterMsg(0x3EC, (char *)"GS_MSG_LOAD_LEVEL");
    msgSystem.RegisterMsg(GS_MSG_INIT_LEVEL, (char *)"GS_MSG_INIT_LEVEL");
    msgSystem.RegisterMsg(GS_MSG_TERM_LEVEL, (char *)"GS_MSG_TERM_LEVEL");
    msgSystem.RegisterMsg(0x3F8, (char *)"GS_MSG_IACTION");
    msgSystem.RegisterMsg(0x3F9, (char *)"GS_MSG_CDT");
    msgSystem.RegisterMsg(0x3FB, (char *)"GS_MSG_SET_LEVEL_ANCHOR");
    msgSystem.RegisterMsg(0x3FC, (char *)"GS_MSG_ASK_KEEP_LEVEL_RELOAD");
    msgSystem.RegisterMsg(GS_MSG_INIT_MAP, (char *)"GS_MSG_INIT_MAP");
    msgSystem.RegisterMsg(0x400, (char *)"GS_MSG_MP_SERVERCLIENT");
    msgSystem.RegisterMsg(0x401, (char *)"GS_MSG_MP_SERVERCLIENT_SIMUL");
    msgSystem.RegisterMsg(0x402, (char *)"GS_MSG_MP_CREATE_PLAYER");
    msgSystem.RegisterMsg(0x403, (char *)"GS_MSG_MP_CREATE_ENT");
    msgSystem.RegisterMsg(0x404, (char *)"GS_MSG_MP_SYNC");
    msgSystem.RegisterMsg(0x405, (char *)"GS_MSG_MP_GET_SYNC_DATA");
    msgSystem.RegisterMsg(0x406, (char *)"GS_MSG_MP_CLIENT_READY");
    msgSystem.RegisterMsg(0x40A, (char *)"GS_MSG_MP_SHOW_MSG");
    msgSystem.RegisterMsg(0x40B, (char *)"GS_MSG_MP_UPD_REGULAR");
    msgSystem.RegisterMsg(0x40E, (char *)"GS_MSG_MP_NOTIFY");
    u32 t = timeGetTime();
    gsTime = t;
    gsStatTime = t;
    gsStartTime = t;
    gsStatFrame = 0;
    msgSystem.NextFrame();
    msgSystem.chgModeMsgId = GS_MSG_CHG_MODE;
    msgSystem.purgeMsgId = GS_MSG_FRAME;
    return 1;
}

// v12 0x00510CB0  (feb 0x004FC900)
// (it first calls an empty function with msgSystem, v12 0x00401380: the profiler's, compiled out)
void __fastcall _gsMsgProcessFrameProfile()
{
    msgSystem.DispatchAll(gsElapsedTime);
    msgSystem.SendAll(GS_MSG_FRAME, 0, 0);
    msgSystem.NextFrame();
}

// v12 0x00510CF0  (feb 0x004FC940)
// The game's time scale (slow motion 0.25); a negative one means 1.
void __fastcall gsMsgSetElapsedTimeScale(float sc)
{
    gsTimeScale = sc >= 0 ? sc : 1.0f;
}

// v12 0x00510D20  (feb 0x004FC970)
float __fastcall gsMsgGetElapsedTimeScale()
{
    return gsTimeScale;
}

// v12 0x00510D30  (1.2 only, name ours)
// The next frame lasts t seconds (above 0 only): cinSYSTEM's skip key jumps the cinema this far in one step.
void __fastcall gsMsgSetNextElapsedTime(float t)
{
    if (t > 0.0f)
        gsNextElapsedTime = t;
}

// v12 0x00510D50  (feb 0x004FC980)
// Everyone told the game's mode (GS_MSG_CHG_MODE with it). 1 (back in the game) lifts the suspension (gsAppState
// 0x2000000), 0 in single player suspends it; either marks the change (_eioIsUpdSuspend). (4 / 5: a cinema starts /
// ends, nothing else.)
void __fastcall gsMsgChangeMode(int mode)
{
    _eioIsUpdSuspend = 0;
    if (mode == 1) {
        if (gsAppState & 0x2000000) {
            gsAppState &= ~0x2000000;
            _eioIsUpdSuspend = 1;
        }
    } else if (mode == 0 && gsSysMP->mode == 1) {
        gsAppState |= 0x2000000;
        _eioIsUpdSuspend = 1;
    }
    msgSystem.SendAll(GS_MSG_CHG_MODE, &mode, 0);
}
