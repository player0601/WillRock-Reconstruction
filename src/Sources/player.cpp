// player.cpp -- the local player: movement input, first-person control and physics
// (Will Rock 1.2, v12.exe; feb obj player.obj)
//
// Rebuilt so far: the frame (ProcessFRAME, ProcessMsg) with the input (ProcessInput: movement, weapon choice, reload,
// zoom, trigger, powerups, use, the debug keys), the camera (UpdateCamera), the aim ray and auto-aim (UpdateCursor,
// GetTargetInst), the first-person weapon (AnimateWeapon, SetCurWpn, IsReadyShoot, GetNextPriorWeapon), use
// (ProcessUse, NotifyMsgUse, IsCanUse), the timers and powerups (UpdateTimers, ActivatePowerup), and the movement
// path -- ControlFirstPerson, ProcessPhysics, UpdateUnderwater -- with their helpers (input smoothing, collision
// filters, the small virtual slots they call). Not yet: the noclip flight, the body (Animate, SetupGhostRendState),
// the death / revive, sounds and the MP / save code.
// All of it is read from the 1.2 disassembly (the beta decompile gives the structure; 1.2 changed it):
//   - ControlFirstPerson: mouse look is no longer scaled by the filtered frame time (the filter still runs);
//   - ProcessPhysics: the push-out runs twice (non-physics instances first, keeping the height only on
//     ai_croc; then the level and physics objects), the last contact is kept for the slope slide, the
//     landing test uses the new "vertically still" timer, falling damage starts later (+40) after a
//     jump-pad launch, hurting floors (template flag 2) send damage, and the end no longer writes the
//     entity's gravity vector;
//   - plrCDT_REFINE: walks through dying NPCs except the five bosses, collides with "&plrc" objects.
//
// Virtual slots of the player, its weapons and other entities are called by their v12 offset (wrEVCall: in this build
// the msgADDR classes' slots sit 4 further, see wr_vcall.h), as the original does:
//   +0x64 GetPos, +0x68 SetPos, +0x6C GetDir, +0x74 IsDead, +0xA4 ApplySpeed.

#include "game.h"
#include "../../include/frontend/wr_ui.h"
#include "../gs/gs_mp_server.h"

BOOL32 __fastcall waterIsUnderWater(m3dV *p, float *level, float r);   // gs_shared water.cpp
#include "../../include/frontend/wr_msgs.h"   // wrMSG_SYSTEM::ShowMsg: Will's lines

// ---------------------------------------------------------------------------------------------
// globals (player.obj .data/.bss)
// ---------------------------------------------------------------------------------------------
plrPLAYER     *plrPlayer = 0;                      // v12 0x00641BF8 (feb 0x006035F8)
_plrMOVE_CTRL  _plrCtrlForw = { 0, 6.0f };         // v12 0x00641B1C (feb 0x00603528) static init v12 0x0041D020
_plrMOVE_CTRL  _plrCtrlSide = { 0, 6.0f };         // v12 0x00641BF0 (feb 0x006035F0)
m3dV           _plrPosJumpStart;                   // v12 0x00641B24 (feb 0x00603534)
static _plrFPS_FILTER _plrFiltTime  = { 1, { 0.25f, 0.25f, 0.25f, 0.25f } };  // v12 0x00641B30 (init 0x0041D050)
static _plrFPS_FILTER _plrFiltPitch = { 1, { 0.25f, 0.25f, 0.25f, 0.25f } };  // v12 0x00641B84 (init 0x0041D0B0)
static _plrFPS_FILTER _plrFiltYaw   = { 1, { 0.25f, 0.25f, 0.25f, 0.25f } };  // v12 0x00641BA8 (init 0x0041D080)
cdtINFO        _plrLastCDTInfo;                    // v12 0x00641BFC (init 0x0041D0E0: zero, point = m3dVZero)
int            plrSlideOnSlopes = 1;               // v12 0x0062B3D4 (feb 0x005EE370)
float          _plrPwrCostTbl[3] = { 50.0f, 50.0f, 50.0f };  // v12 0x0062B868 (feb 0x005EE740) gold per powerup

// Damage messages ProcessPhysics / UpdateUnderwater send to the MP client (function statics, built once).
#pragma pack(push, 1)
struct plrDMG_MSG { u8 b0, b1; u16 size; u8 type, sub; float amount; u32 source; };   // 0x0E bytes
#pragma pack(pop)

// ---------------------------------------------------------------------------------------------
// input helpers
// ---------------------------------------------------------------------------------------------

// v12 0x00425D20  (feb 0x004226A0)  inline of the input header, emitted here
int gsINP_SYSTEM::IsToggled(int cmd)
{
    gsINP_ACTIONS *a = actions;
    if (cmd < a->first || cmd >= a->last)
        return 0;
    return a->states[cmd - a->first].flags & 2;
}

// v12 0x0041D130  (feb 0x0041A680)
float _plrFPS_FILTER::Update(float x)
{
    if (reset) {
        h[0] = x;
        h[1] = h[0];  h[2] = h[1];  h[3] = h[2];
    } else {
        h[0] = h[1];  h[1] = h[2];  h[2] = h[3];
        h[3] = x;
    }
    float r = 0;
    for (int i = 0; i < 4; i++)
        r += h[i] * w[i];
    reset = 0;
    return r;
}

// v12 0x0041D1A0  (feb 0x0041A720)
// Ramp the axis towards dir at 'rate' per second; with no input it decays to 0. Airborne (> 0.5 s above
// the floor) the control is weak: pushing on in the same direction ramps at a third of the rate, letting go
// decays ten times slower, and the value is not clamped to +-1 (momentum is kept).
void _plrMOVE_CTRL::Update(int dir)
{
    int air = plrPlayer->airTime > 0.5f;
    float step = gsElapsedTime * rate;
    if (air) {
        if ((dir > 0 && value > 0) || (dir < 0 && value < 0))
            step *= 0.33333334f;                        // v12 0x005FA978
        else if (dir == 0) {
            step *= 0.1f;
            goto decay;
        }
    } else {
        if (value < -1.0f)
            value = -1.0f;
        else if (value > 1.0f)
            value = 1.0f;
    }
    if (dir > 0) {
        value += step;
        if (value < -1.0f) {
            value = -1.0f;
            return;
        }
        if (value > 1.0f && !air)
            value = 1.0f;
        return;
    }
    if (dir < 0) {
        if (value > 1.0f)
            value = 1.0f;
        value -= step;
        if (value < -1.0f && !air)
            value = -1.0f;
        return;
    }
decay:
    if (value > 0) {
        value -= step;
        if (value < 0)
            value = 0;
    } else if (value < 0) {
        value += step;
        if (value > 0)
            value = 0;
    }
}

// v12 0x0041D320  (feb 0x0041A8A0)
// Smoothstep of the axis on [-1, 1]; beyond +1 (airborne momentum) it grows to 1.5 at 20. Below -1 the
// original compares against +1 instead of -1, so a backwards overshoot gives -1 (kept as is).
float _plrMOVE_CTRL::GetValue()
{
    float v = value;
    if (!(v < -1.0f) && !(v > 1.0f)) {
        float t = (v - -1.0f) * 0.5f;
        return (3.0f - (t + t)) * t * t * 2.0f - 1.0f;
    }
    if (v > 1.0f)
        return v > 20.0f ? 1.5f : (v - 1.0f) * (1.0f / 38.0f) + 1.0f;
    if (v < 1.0f)
        return -1.0f;
    return v > 20.0f ? -1.5f : -1.0f - (v - 1.0f) * (1.0f / 38.0f);   // unreachable
}

// ---------------------------------------------------------------------------------------------
// plrCDT_REFINE
// ---------------------------------------------------------------------------------------------

// v12 0x00425AD0  (feb 0x00422480)
// Dying NPCs are walked through, except the bosses; other players only while alive; the rest as
// entCDT_REFINE_EXCL (mode 2: only physics/dynamic instances).
BOOL32 plrCDT_REFINE::IsInclInst(animINST *i)
{
    static const char *const bosses[5] = { "ai_cyclop", "ai_gefest", "ai_medusa", "ai_zeus", "ai_croc" };
    if (aiIsNPCDead(i->ent, 0)) {
        int k;
        for (k = 0; k < 5; k++)
            if (strstr(i->tplName ? i->tplName : (char *)"", bosses[k]))
                break;
        if (k == 5)
            return 0;
    }
    entENTITY *e = i->ent;
    if (e->classId == 0x504C5952) {                     // 'PLYR'
        if (wrEVCall<BOOL32>(e, 0x74))                   // IsDead
            return 0;
        if (i->ent->unk98 == 1)
            return 1;
    }
    if ((mode & 2) && !(i->flags & 0x48))
        return 0;
    return entCDT_REFINE_EXCL::IsInclInst(i);
}

// v12 0x00425CC0
// A shown object whose name contains the filter's.
BOOL32 objFILTER_HIDDEN_NAME::IsUse(objOBJ *o)
{
    if (o->procState & 8)
        return 0;
    return strstr(o->name, name) != 0;
}

// v12 0x00425BE0  (feb 0x00422570)
// Objects named "&plrc..." are player-only collision geometry: always solid for the player.
BOOL32 plrCDT_REFINE::IsInclObj(objOBJ *o)
{
    if (o->name && strstr(o->name, "&plrc"))
        return 1;
    u32 ps = o->procState;
    return ((ps & 0x1F) == 0 || (ps & 0x10000)) && !(ps & 0x40000);
}

// ---------------------------------------------------------------------------------------------
// small virtual slots
// ---------------------------------------------------------------------------------------------

// v12 0x0041DC30  (feb 0x0041B0C0)
// An address going away: the body, the aim target; TERM_LEVEL / 0x3F2: the weapon's sound stops and the zoom ends;
// 0x3F7: a TH round restarts the server's player at a start; an IAction "USE_KEY" spends the key; 0x4CE the normal
// time scale; 0x4CF slow motion again when it is active. Then the shared messages.
BOOL32 plrPLAYER::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 3:                                             // MSG_MSG_DESTROY_ADDR
        if (data == (void *)body)
            body = 0;
        if (gscCursor.aimInfo->inst && (void *)gscCursor.aimInfo->inst->ent == data) {
            gscCursor.aimInfo->inst = 0;
            gscCursor.aimInfo->flags &= ~2u;
            realTimer = unk_2A3 + 1.0f;                 // a new aim ray next frame
        }
        break;
    case 0x3F2:
        wrEVCall<void>(weapons[curWeapon], 0xCC);       // StopSound
        ClearZoom();
        break;
    case 0x3F5:                                         // TERM_LEVEL
        wrEVCall<void>(weapons[curWeapon], 0xCC);
        if (WR_FIELD(int, gsSysMP, 0x28) != 1)
            ClearZoom();
        break;
    case 0x3F7:
        if (WR_FIELD(int, wrSysGO, 0x98) == 2 && (entFlags & 4)) {
            m3dV pos, dir;
            gsSysMP->GetStartPosDir(0, &pos, &dir);
            wrEVCall<void>(this, 0x70, &pos, &dir, 1);  // SetPosDir
        }
        break;
    case 0x3F8:                                         // IACTION
        if (((apNAME *)((char *)data + 8))->IsName((char *)"USE_KEY"))
            wrEVCall<void>(this, 0x98);                 // UseKey
        break;
    case 0x4CE:
        gsMsgSetElapsedTimeScale(1.0f);
        break;
    case 0x4CF:
        if (powerup[PLR_PWR_TIMESCALE].state == PLR_PWR_ACTIVE)
            ActivatePowerup(PLR_PWR_TIMESCALE);
        break;
    }
    return plrPLAYER_COMMON::ProcessMsg(id, data, from, ret) ? 1 : 0;
}

// v12 0x0041DE70  (feb 0x0052F2B0)
void plrPLAYER::GetPos(m3dV *out)
{
    inst->GetPos(out);
}

// v12 0x0041DE80  (feb 0x0052F2C0)
void plrPLAYER::GetDir(m3dV *out)
{
    inst->GetDir(out);
}

// v12 0x0041DE90  (feb 0x0041B1B0)
void plrPLAYER::SetPos(m3dV *p)
{
    m3dV c = *p;
    inst->SetPos(&c);
}

// v12 0x0041DED0  (feb 0x0041B1F0)  slot +0x70
// Put the player at pos facing dir (without them: the start position the MP system picks for it). onFloor: dropped
// onto the floor under it first (as far as 2e6 down, from 3 up; the floor contact kept). The camera there, level;
// with gsAppState 0x800 the floor contact taken again (30 down); the weapon lit from the floor; the place kept;
// moved (entFlags / flags2 0x200).
void plrPLAYER::SetPosDir(m3dV *pos, m3dV *dir, int onFloor)
{
    cdtREFINE ref;
    ref.mode = 0;
    ref.queryFlags = 0;
    ref.exclude = 0;
    m3dV P, D, t;
    if (!pos || !dir) {
        gsSysMP->SelectStartPosDir(&P, &D, this);
        pos = &P;
        dir = &D;
    }
    if (onFloor) {
        t = *pos;
        gsScenePtr->FloorPlace(&t, 2000000.0f, pos, 3.0f, 0, 0, &ref, &floorInfo);
    }
    pos->y += eyeHeight;
    *(m3dV *)lastPos = *pos;
    pitch = 90.0f;
    gsCameraPtr->ResetVertRotation();
    pos->y -= eyeHeight;
    inst->SetPos(pos);
    inst->SetDir_XZ(dir);
    cdtREFINE ref2;
    ref2.mode = 2;
    ref2.queryFlags = 0;
    ref2.exclude = 0;
    if (gsAppState & 0x800) {
        m3dV o;
        inst->GetPos(&t);
        gsScenePtr->FloorPlace(&t, 30.0f, &o, 1.0f, 1.0f, 0, &ref2, &floorInfo);
    }
    weapons[curWeapon]->ApplyColor(&floorInfo, 0);
    bodyDir = *dir;
    placePos2 = *pos;
    placePos = *pos;
    entFlags |= 0x200;
    flags2 |= 0x200;
}

// v12 0x00425E50  (feb 0x00422720)
BOOL32 plrPLAYER_COMMON::IsDead()
{
    return moveFlags & 2;
}

// v12 0x00423780  (feb 0x00420340)
void plrPLAYER::ApplySpeed(m3dV *v, int set)
{
    if (set) {
        vel = *v;
        if (v->y > 5.0f)
            moveFlags |= PLR_MF_JUMP_UP;
        return;
    }
    vel.x += v->x;
    vel.y += v->y;
    vel.z += v->z;
}

// ---------------------------------------------------------------------------------------------
// ProcessFRAME
// ---------------------------------------------------------------------------------------------

#pragma pack(push, 1)
struct plrNOTIFY_MSG : msgDATA {                        // GS_MSG_MP_NOTIFY 0x40E: a 6-byte notify (type 8: reload)
    short type;                                         // +0x04
};
struct plrWPN_MSG : msgDATA {                           // GS_MSG_MP_NOTIFY 0x40E: the weapon switched (0x0E bytes)
    short type;                                         // +0x04 1
    int   wpn;                                          // +0x06
    int   animate;                                      // +0x0A
};
#pragma pack(pop)

static void *_plrCfgPlrLegs;                            // v12 0x00641C1C the cfg cache of [Video] VIDEO_PlrLegs
static void *_plrCfgWpnShdr;                            // v12 0x00641B80 the cfg cache of [Video] VIDEO_WpnShdr

// the inline linear map of the screen flash: t from x0..x1 to y0..y1 (the ends taken in increasing x)
static inline float _plrLinear(float t, float x0, float x1, float y0, float y1)
{
    if (x1 < x0) {
        float a = x0; x0 = x1; x1 = a;
        a = y0; y0 = y1; y1 = a;
    }
    return y0 + (t - x0) * (y1 - y0) / (x1 - x0);
}

// v12 0x0041E0C0  (feb 0x0041B550)
// The player's frame: back to the start when lost beyond 10000; while the game runs the shared frame, the video
// options (legs, weapon shadow), the input, the camera and the aim, the shot (with the camera's recoil), the
// reload, the first-person weapon at the camera (the medusa gun and the bomb face back) lit from the floor,
// its animation, the body, the timers and the screen flash.
void plrPLAYER::ProcessFRAME()
{
    if (inst->flags & 2)
        inst->flags &= ~2u;
    m3dV p;
    inst->GetPos(&p);
    if (fabsf(p.x) > 10000.0f || fabsf(p.y) > 10000.0f || fabsf(p.z) > 10000.0f)
        wrEVCall<void>(this, 0x70, (void *)0, (void *)0, 1);    // SetPosDir: the start
    moveFlags &= 0xFFDF7F9Fu;
    if (!(gsAppState & 0x40000) || (gsAppState & 0x400000)) {
        u8 *sync = (u8 *)unk_1E3;                       // the 0x53-byte sync block (ProcessINIT)
        sync[0x51] = (flags2 & 0x40) ? sync[0x51] | 0x10 : sync[0x51] & ~0x10;
        sync[0x51] = (flags2 & 0x80) ? sync[0x51] | 0x20 : sync[0x51] & ~0x20;
        sync[0x51] = (flags2 & 0x200) ? sync[0x51] | 0x40 : sync[0x51] & ~0x40;
        return;
    }
    if (moveFlags & PLR_MF_LEVEL_START) {
        _plrFiltTime.reset = 1;
        _plrFiltYaw.reset = 1;
        _plrFiltPitch.reset = 1;
    }
    if ((moveFlags & 2) && !(moveFlags & 0x40000000)) {
        const char *key = (gsAppState & 0x8000) ? "GAME_PLAYER_IS_DEAD_RETAIL"
                          : WR_FIELD(int, gsSysMP, 0x28) == 1 ? "GAME_PLAYER_IS_DEAD_DEBUG"
                                                              : "GAME_PLAYER_IS_DEAD_DEBUG_MP";
        uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId(key)), 0.5f, 0);
    }
    plrPLAYER_COMMON::ProcessFRAME();
    int v = 0;
    _apCfgReadBool(&_plrCfgPlrLegs, (char *)"Video", (char *)"VIDEO_PlrLegs", &v);
    flags2 = v ? flags2 | 1 : flags2 & ~1u;
    _apCfgReadBool(&_plrCfgWpnShdr, (char *)"Video", (char *)"VIDEO_WpnShdr", &v);
    flags2 = v ? flags2 | 2 : flags2 & ~2u;
    SetupGhostRendState();
    if (!(gsAppState & 0x1800) && !(moveFlags & 0x400000) && !(WR_FIELD(u8, gsSysCinema, 0x80) & 1)) {
        ProcessInput();
    } else {
        _plrCtrlForw.Update(0);
        _plrCtrlSide.Update(0);
    }
    if (!(gsAppState & 0x1000)) {
        UpdateCamera();
        UpdateCursor();
    }
    pwpWEAPON *w = weapons[curWeapon];
    if (moveFlags & 0x200000) {
        m3dV aim = gscCursor.aimPoint;
        if (wrEVCall<int>(w, 0x68, (void *)this, &aim))  // Shoot
            camCtrl->ShootNotify(WR_FIELD(int, w, 0x98));
    }
    if ((moveFlags & 0x80) || (moveFlags & 0x10000000)) {
        if (entFlags & 2) {
            plrNOTIFY_MSG m;
            m.packId = 0;
            m.tag = 0x40;
            m.size = 6;
            m.type = 8;
            wrEVCall<int>(gsSysClient, 0x2C, (void *)plrPlayer, 0x40E, (void *)&m, 0x20000);
        }
        weapons[curWeapon]->Recharge();
    }
    m3dV org, fw;
    gsCameraPtr->GetOrigin(&org);
    gsCameraPtr->GetVForw(&fw);
    w = weapons[curWeapon];
    animINST *wi = w->inst;
    if (WR_FIELD(int, w, 0x98) == PWP_MEDUSA || WR_FIELD(int, w, 0x98) == PWP_BOMB) {
        fw.x = -fw.x;
        fw.y = -fw.y;
        fw.z = -fw.z;
    }
    m3dMATR m;
    m.Identity();
    m._MakeLCS2WCS_VZ(&org, &fw);
    wi->Transform(&m, 0);
    AnimateWeapon();
    cdtREFINE ref(2, 0);
    int blend = !((moveFlags & 0x10) && !(moveFlags & 8));   // putting a weapon on: at once
    if (gsAppState & 0x800) {
        inst->GetPos(&org);
        gsScenePtr->FloorPlace(&org, 30.0f, &fw, 1.0f, 1.0f, 0, &ref, &floorInfo);
    }
    weapons[curWeapon]->ApplyColor(&floorInfo, blend);
    Animate();
    // (an empty function of the player is called here)
    UpdateTimers();
    flags2 &= ~0xC0u;
    moveFlags &= ~(u32)PLR_MF_LEVEL_START;
    if (flags2 & 0x200)
        flags2 &= ~0x300u;
    if (flashTimer >= 0) {
        float half = flashPeriod * 0.5f, f;
        if (flashTimer < half)
            f = _plrLinear(flashTimer, 0, half, 0, 1.0f);
        else
            f = _plrLinear(flashTimer, half, flashPeriod, 1.0f, 0);
        gsSysRender->SetColorFullScr(((u32)(int)(f * f * 255.0f) & 0xFF) << 24);
    }
}

// ---------------------------------------------------------------------------------------------
// ProcessInput
// ---------------------------------------------------------------------------------------------

#pragma pack(push, 1)
struct plrTEAM_MSG : msgDATA {                          // GS_MSG_MP_NOTIFY 0x40E: TOGGLE_TEAM (0x0B bytes)
    short type;                                         // +0x04 0x0B
    int   team;                                         // +0x06 the other team
    u8    request;                                      // +0x0A 1
};
#pragma pack(pop)

static int _plrDbgSkin;                                 // v12 0x00641C24 DBG_5 skin counter (name ours)
static void *_plrCfgDecompOffAI;                        // v12 0x00641C20 the cfg cache of [Debug] Decomp_OFF_AI

// v12 0x0041F790  (feb 0x0041C960)
// The movement (ProcessInputMove), then -- alive -- weapon selection (number keys, next / prev), the reload key,
// the crossbow zoom, the trigger (moveFlags 0x40 pulled, 0x200000 shoot this frame), the powerup keys, god mode,
// the "use" tip and the use key, the team key; dead or alive the screenshot key and the debug keys DBG_1..DBG_9.
void plrPLAYER::ProcessInput()
{
    ProcessInputMove();
    gsINP_SYSTEM *in = gsSysInput;
    if (wrEVCall<BOOL32>(this, 0x74)) {                  // IsDead
        moveFlags &= ~0x200000u;
    } else {
        // the number keys: the first owned, non-empty other weapon whose key went down
        int slot = 0;
        for (int cmd = CMD_WPN_1; cmd < CMD_WPN_NEXT; cmd++, slot++) {
            if (!in->IsOn(cmd) || !in->IsToggled(cmd) || slot == curWeapon)
                continue;
            pwpWEAPON *w = weapons[slot];
            if (w && (w->flags & WPNF_ENABLED) && !w->IsEmpty())
                break;
        }
        if (slot == PLR_WPN_COUNT) {                    // none: the one asked for while a switch was running
            slot = unk_263;
            unk_263 = PLR_WPN_COUNT;
        }
        if (slot < PLR_WPN_COUNT && slot != curWeapon && !(moveFlags & 0x10)) {
            moveFlags |= 0x18;
            nextWeapon = slot;
            ClearZoom();
        } else {
            unk_263 = slot;
        }
        // next / previous weapon
        if (!(moveFlags & 0x10)) {
            int dir = 0;
            if (in->IsOn(CMD_WPN_NEXT) && in->IsToggled(CMD_WPN_NEXT))
                dir = -1;
            else if (in->IsOn(CMD_WPN_PREV) && in->IsToggled(CMD_WPN_PREV))
                dir = 1;
            if (dir) {
                int i = curWeapon;
                for (;;) {
                    i += dir;
                    if (i < 0)
                        i = PLR_WPN_COUNT - 1;
                    else if (i >= PLR_WPN_COUNT)
                        i = 0;
                    pwpWEAPON *w = weapons[i];
                    if (w && (w->flags & WPNF_ENABLED) && !w->IsEmpty()) {
                        nextWeapon = i;
                        moveFlags |= 0x18;
                        ClearZoom();
                        break;
                    }
                    if (i == curWeapon)
                        break;
                }
            }
        }
        // the reload key (not the shovel, the machine gun, the rocket launcher, the bomb)
        if (!(moveFlags & 0x10000000) && curWeapon != PLR_WPN_MCG && curWeapon != PLR_WPN_BSK &&
            curWeapon != PLR_WPN_SVL && curWeapon != PLR_WPN_BMB) {
            pwpWEAPON *w = weapons[curWeapon];
            int spare = (int)(w->ammo - w->clip);
            if (spare <= 0)
                spare = 0;
            if (in->IsOn(CMD_FORCE_RECHARGE) && in->IsToggled(CMD_FORCE_RECHARGE) && w->clip != w->clipSize &&
                spare) {
                moveFlags |= 0x10000000;
                ClearZoom();
            }
        }
        // the crossbow's zoom: the key toggles it, letting go of it while zooming in zooms back out
        if (curWeapon == PLR_WPN_CRB && !(moveFlags & 0x12)) {
            if (in->IsOn(CMD_ZOOM) && in->IsToggled(CMD_ZOOM)) {
                camCtrl->ZoomNotify(1);
                pwpWEAPON *w = weapons[curWeapon];
                if (moveFlags & 0x20000000) {
                    uiSystem->ShowHudFrame(0);
                    moveFlags &= ~0x20000000u;
                    w->inst->flags &= ~1u;
                    w->SetZoom(0);
                } else {
                    w->inst->flags |= 1;                // the weapon model hidden behind the scope
                    moveFlags |= 0x20000000;
                    uiSystem->ShowHudFrame(1);
                    w->SetZoom(1);
                }
            } else if (!in->IsOn(CMD_ZOOM) && in->IsToggled(CMD_ZOOM)) {
                camCtrl->ZoomNotify(0);
            }
        }
        // the trigger: the shovel and the colt fire once per press, the others while held
        if (in->IsOn(CMD_SHOOT) && IsReadyShoot()) {
            if (curWeapon == PLR_WPN_CLT || curWeapon == PLR_WPN_SVL) {
                if (in->IsOn(CMD_SHOOT) && in->IsToggled(CMD_SHOOT))
                    moveFlags |= 0x40;
            } else {
                moveFlags |= 0x40;
            }
        }
        // the atomic gun fires its loaded charge when the key is let go
        if (curWeapon == PLR_WPN_AMG && !in->IsOn(CMD_SHOOT) && in->IsToggled(CMD_SHOOT) && IsReadyShoot()) {
            moveFlags |= 0x40;
            ((pwpATOMICGUN *)weapons[curWeapon])->ReleaseShot();
        }
        moveFlags |= 0x200000;
        pwpWEAPON *w = weapons[curWeapon];
        if (!(w->flags2 & PWPF2_SHOOT_ON_ACTFRAME)) {
            if (!(moveFlags & 0x40))
                moveFlags &= ~0x200000u;
        } else if (!wrEVCall<BOOL32>(w, 0xC4) || (moveFlags & 0x80) || (moveFlags & 0x10000000)) {   // IsShooting
            moveFlags &= ~0x200000u;
        }
        // the powerup keys
        if (!(plrPlayer->moveFlags & 0x40000000)) {
            if (in->IsOn(CMD_ACTIVATE_QUAD_DAMAGE) && in->IsToggled(CMD_ACTIVATE_QUAD_DAMAGE))
                ActivatePowerup(PLR_PWR_QUAD_DAMAGE);
            if (in->IsOn(CMD_ACTIVATE_UNHOLY_ARMOR) && in->IsToggled(CMD_ACTIVATE_UNHOLY_ARMOR))
                ActivatePowerup(PLR_PWR_UNHOLY_ARMOR);
            if (in->IsOn(CMD_ACTIVATE_SLOW_MOTION) && in->IsToggled(CMD_ACTIVATE_SLOW_MOTION))
                ActivatePowerup(PLR_PWR_TIMESCALE);
        }
        if (in->IsOn(CMD_DBG_3) && in->IsToggled(CMD_DBG_3))
            moveFlags ^= PLR_MF_GOD;
        if (in->IsOn(CMD_DBG_7) && in->IsToggled(CMD_DBG_7))
            aiNPC_WR::DbgToggleShow();
        NotifyMsgUse();
        if (in->IsOn(CMD_USE) && in->IsToggled(CMD_USE))
            ProcessUse();
        if (in->IsOn(CMD_TOGGLE_TEAM) && in->IsToggled(CMD_TOGGLE_TEAM) && WR_FIELD(int, wrSysGO, 0x98) == 3 &&
            !(moveFlags & 0x40000000)) {
            plrTEAM_MSG m;
            m.packId = 0;
            m.tag = 0x40;
            m.size = 0x0B;
            m.type = 0x0B;
            m.team = 1 - team;
            m.request = 1;
            wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x40E, (void *)&m, 0x20000);
        }
    }
    if (in->IsOn(CMD_SCREENSHOT) && in->IsToggled(CMD_SCREENSHOT))
        WR_FIELD(u32, gsSysRender, 4) |= 8;
    if (in->IsOn(CMD_DBG_8) && in->IsToggled(CMD_DBG_8)) {   // suicide
        dmgGENERAL d;
        inst->GetPos(&d.pos);
        d.type = DMG_NO_ARMOUR;
        d.amount = 300.0f;
        msgSystem.SendMsg(this, GS_MSG_DAMAGE, &d, this);
    }
    if (in->IsOn(CMD_DBG_8) && in->IsToggled(CMD_DBG_8))
        ReleaseTreasures(0, -1);
    if (in->IsOn(CMD_DBG_4) && in->IsToggled(CMD_DBG_4) && !(moveFlags & 0x40000000) && !(gsAppState & 0x8000)) {
        int full = GetAsyncKeyState(VK_SHIFT) < 0;      // Shift: with the weapons and ammo
        if (entFlags & 2) {
            flags2 |= 0x40;
            if (full)
                flags2 |= 0x80;
            entFlags |= 0x200;
        }
        wrEVCall<void>(this, 0x10C, full);               // Revitalize
    }
    if (in->IsOn(CMD_DBG_5) && in->IsToggled(CMD_DBG_5))
        for (entENTITY *e = entFindNext(0); e; e = entFindNext(e))
            if (e->classId == 0x504C5952) {             // 'PLYR': the call here is to an empty function
            }
    if (in->IsOn(CMD_DBG_5) && in->IsToggled(CMD_DBG_5))
        for (int i = 0; i < WR_FIELD(int, aiSpawnDomList, 0xA0); i++) {
            aiDOMAIN_SPAWN *d = WR_FIELD(aiDOMAIN_SPAWN *, aiSpawnDomList, 0xA4 + i * 4);
            if (d->IsSpawning())
                d->StopSpawning();
        }
    if (in->IsOn(CMD_DBG_6) && in->IsToggled(CMD_DBG_6)) {   // kill every NPC but the bosses
        static dmgDAMAGE kill(DMG_SPECIAL);             // v12 0x00641B68 (guard 0x00641B55 bit 1)
        kill.amount = 5000.0f;
        kill.subtype = 2;
        for (aiNPC *n = aiNPC::GetNext(0, 1); n; n = aiNPC::GetNext(n, 1))
            if (n->nLocks <= 0 && !(n->aiFlags2 & AIF2_BOSS))
                wrEVCall<int>(gsSysClient, 0x2C, (void *)n, GS_MSG_DAMAGE, (void *)&kill, 0x20000);
    }
    // (a second IsOn / IsToggled test of DBG_7 here has an empty body)
    if (in->IsOn(CMD_DBG_9) && in->IsToggled(CMD_DBG_9))
        gsAppState ^= 0x8000;
    if (in->IsPressed(CMD_DBG_1)) {
        int off = 0;
        _apCfgReadBool(&_plrCfgDecompOffAI, (char *)"Debug", (char *)"Decomp_OFF_AI", &off);
        off = !off;
        apCfgWriteBool((char *)"Debug", (char *)"Decomp_OFF_AI", off);
    }
    if (in->IsPressed(CMD_DBG_5)) {
        _plrDbgSkin = (_plrDbgSkin + 1) % 6;
        wrEVCall<void>(this, 0x124, _plrDbgSkin);        // SetSkin
    }
}

// [part of v12 0x0041F790..0x0041FA9E, not a function of its own] the movement part of ProcessInput:
// walk/run speeds, crouch, noclip toggle, then ControlFirstPerson and ProcessPhysics.
void plrPLAYER::ProcessInputMove()
{
    maxSpeed = 5.0f;
    accel = 30.0f;
    if (!gsSysInput->IsOn(CMD_CROUCH)) {
        BOOL32 autoRun = cfg.bits & 1;                  // ACCEL inverts "always run"
        if (autoRun ? !gsSysInput->IsOn(CMD_ACCEL) : gsSysInput->IsOn(CMD_ACCEL)) {
            maxSpeed = 10.0f;
            accel = 90.0f;
        }
    }
    if (IsActivePowerup(PLR_PWR_TIMESCALE))             // titan motion: the speed kept in slow motion
        accel = 1.0f / gsMsgGetElapsedTimeScale() * accel * 0.8f;
    if (gsSysInput->IsOn(CMD_CROUCH)) {
        if (!(moveFlags & 0x200) && floorHeight <= 0.1f) {
            moveFlags |= 0x200;
            unk108 |= 0x20000000;
        }
    } else {
        moveFlags &= ~0x200u;
    }
    if (!(gsAppState & 0x8000) && gsSysInput->IsOn(CMD_SWITCH_MODE) && gsSysInput->IsToggled(CMD_SWITCH_MODE)) {
        if (moveFlags & PLR_MF_FLYTHROUGH) {            // out of the fly-through: the player where the camera is,
            m3dV org, fw;                               //   facing its way, level
            gsCameraPtr->GetOrigin(&org);
            gsCameraPtr->GetVForw(&fw);
            fw.y = 0;
            m3dNormalize(&fw);
            wrEVCall<void>(this, 0x70, &org, &fw, 0);   // SetPosDir
        }
        moveFlags ^= PLR_MF_FLYTHROUGH;
        if (flyReenter) {                               // (never set: dead code in 1.2)
            static int flyToggle;                       // v12 0x00641BDC
            flyToggle = 1 - flyToggle;
            if (!flyToggle)
                moveFlags |= PLR_MF_FLYTHROUGH;
        }
    }
    m3dV prev;
    inst->GetPos(&prev);
    if (!(flags2 & 8)) {
        uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId((char *)"GAME_WAIT_FOR_SERVER")), 0.1f, 0);
    } else if (moveFlags & PLR_MF_FLYTHROUGH) {
        ControlFlyThrough();
    } else if (!(moveFlags & 2)) {
        ControlFirstPerson();
    }
    ProcessPhysics(&prev);
}

// [not rebuilt yet] original v12 0x00422570 (feb 0x0041F130): noclip movement
void plrPLAYER::ControlFlyThrough()
{
    WR_NOT_REBUILT(0x00422570);
}

// ---------------------------------------------------------------------------------------------
// ControlFirstPerson
// ---------------------------------------------------------------------------------------------

// v12 0x004229B0  (feb 0x0041F570)
// Mouse look (pitch clamped to 5..175), the walk/strafe axes, jumping, and the lower-body turn lag.
void plrPLAYER::ControlFirstPerson()
{
    float yawMul = 4000.0f, pitchMul = 3000.0f;
    if (WR_FIELD(float, gsCameraPtr, 0x14C) < 50.0f) {  // zoomed (camera horizontal angle)
        yawMul = 800.0f;
        pitchMul = 600.0f;
    }
    float mx = gsSysInput->mouseDX * 0.2f;
    float my = gsSysInput->mouseDY * 0.2f;
    _plrFiltTime.Update(gsElapsedTime);                 // 1.2: result unused
    float d = _plrFiltPitch.Update(my * pitchMul * 0.01f);
    float np = d + pitch;
    if (!(np > 175.0f) || d < 0) {
        if (!(np < 5.0f) || d > 0) {
            inst->RotateX(d, 1);
            pitch += d;
        }
    }
    float yaw = _plrFiltYaw.Update(mx * yawMul * -0.01f);
    m3dV p, dir, right;
    inst->GetPos(&p);
    inst->Translate(&m3dVZero, 0);
    inst->RotateY(yaw, 2);
    inst->SetPos(&p);
    inst->GetDir(&dir);
    inst->GetDirRight(&right);
    if (!(moveFlags & 0xC00)) {                         // walking: move in the horizontal plane
        dir.y = 0;
        right.y = 0;
    }
    m3dNormalize(&dir);
    m3dNormalize(&right);
    gsINP_SYSTEM *in = gsSysInput;
    if (moveFlags & 0x100) {                            // jumping: the last direction keys stay latched
        if (in->IsOn(CMD_MOVE_FORWARD))  unk_1F7 = (unk_1F7 & ~0x80u) | 0x40;
        if (in->IsOn(CMD_MOVE_BACKWARD)) unk_1F7 = (unk_1F7 & ~0x40u) | 0x80;
        if (in->IsOn(CMD_MOVE_LEFT))     unk_1F7 = (unk_1F7 & ~0x20u) | 0x10;
        if (in->IsOn(CMD_MOVE_RIGHT))    unk_1F7 = (unk_1F7 & ~0x10u) | 0x20;
    } else {
        unk_1F7 &= 0xFFFFFF0F;
    }
    int f;
    if (in->IsOn(CMD_MOVE_FORWARD) || (unk_1F7 & 0x40))
        f = 1;
    else if (in->IsOn(CMD_MOVE_BACKWARD) || (unk_1F7 & 0x80))
        f = -1;
    else
        f = 0;
    _plrCtrlForw.Update(f);
    if (in->IsOn(CMD_MOVE_LEFT) || (unk_1F7 & 0x10))
        f = -1;
    else if (in->IsOn(CMD_MOVE_RIGHT) || (unk_1F7 & 0x20))
        f = 1;
    else
        f = 0;
    _plrCtrlSide.Update(f);
    if (in->IsOn(CMD_MOVE_FORWARD) || in->IsOn(CMD_MOVE_BACKWARD) || in->IsOn(CMD_MOVE_LEFT) || in->IsOn(CMD_MOVE_RIGHT))
        moveFlags |= 0x20;
    in = gsSysInput;
    if (in->IsOn(CMD_JUMP) && in->IsToggled(CMD_JUMP) && !(moveFlags & 0x100) && floorHeight < 0.2f &&
        !(moveFlags & 0x200) && (floorInfo.flags & 2) &&
        (!floorInst || floorInst->ent->classId != 0x54524D50)) {   // not on a 'PMRT' (jump pad) entity
        m3dV jd;
        wrEVCall<void>(this, 0x6C, &jd);                 // GetDir
        jd.y = 0;
        m3dNormalize(&jd);
        moveFlags |= 0x100;
        m3dV v = m3dVZero;
        if (gsSysInput->IsOn(CMD_MOVE_FORWARD)) {
            v.x = jd.x + jd.x;
            v.z = jd.z + jd.z;
        } else if (gsSysInput->IsOn(CMD_MOVE_BACKWARD)) {
            v.x = jd.x * -2.0f;
            v.z = jd.z * -2.0f;
        }
        v.y = 10.0f;
        wrEVCall<void>(this, 0xA4, &v, 0);               // ApplySpeed (add)
        unk108 |= 0x100;
        wrEVCall<void>(this, 0x64, &_plrPosJumpStart);   // GetPos
    }
    if ((moveFlags & 0x400) && dir.y > 0)               // swimming at the surface: no climbing out by looking up
        dir.y = 0;
    inst->GetPos(&p);
    float side = _plrCtrlSide.GetValue() * maxSpeed;
    float forw = _plrCtrlForw.GetValue() * maxSpeed;
    ctrlVel.x = dir.x * forw + right.x * side;
    ctrlVel.y = dir.y * forw + side * right.y;
    ctrlVel.z = dir.z * forw + right.z * side;
    float len = m3dSqrt(ctrlVel.z * ctrlVel.z + ctrlVel.y * ctrlVel.y + ctrlVel.x * ctrlVel.x);
    if (len > maxSpeed) {
        float k = maxSpeed / m3dSqrt(ctrlVel.z * ctrlVel.z + ctrlVel.y * ctrlVel.y + ctrlVel.x * ctrlVel.x);
        ctrlVel.x *= k;
        ctrlVel.y *= k;
        ctrlVel.z *= k;
    }
    moveFlags &= 0xF9FFFFFF;
    inst->GetDir(&dir);
    int st = WR_FIELD(int, inst->link, 0x20);           // animation system state of the body
    if (st != 2 && st != 0x35 && st != 0x3B) {
        moveFlags &= ~0x1000000u;
        bodyDir = dir;
        return;
    }
    float a = -m3xzAngleVector(&bodyDir, &dir);         // how far the view is turned away from the legs
    if (fabsf(a) > 25.0f)
        moveFlags |= 0x1000000;
    if (!(moveFlags & 0x1000000))
        return;
    float aa = fabsf(a);
    float k = aa < 30.0f ? 1.0f : (aa > 45.0f ? 10.0f : (aa - 30.0f) * 0.6f + 1.0f);
    float sgn = a > 0 ? 1.0f : (a < 0 ? -1.0f : 0.0f);
    float turn = -(sgn * gsElapsedTime * (k * 80.0f));
    if (!(a < 0)) {
        if (fabsf(a) > 10.0f)
            moveFlags |= 0x4000000;
    } else {
        if (fabsf(a) > 10.0f)
            moveFlags |= 0x2000000;
    }
    m3dMATR r;
    r.MakeRotY(turn);
    m3dTransformVector(&bodyDir, &r, &bodyDir);
    if (fabsf(a) < 3.0f || fabsf(a) < fabsf(turn))
        moveFlags &= ~0x1000000u;
}

// ---------------------------------------------------------------------------------------------
// ProcessPhysics
// ---------------------------------------------------------------------------------------------

// v12 0x004208B0  (feb 0x0041D5D0)
// One physics step of the player body (prev = position before ControlFirstPerson). Velocity = vel (gravity,
// jumps, slides, platform momentum) + ctrlVel (walking). The body is a sphere of 'radius' around the eye;
// the floor is found by a ray from 'stepHeight' above (so steps are climbed), the sphere is pushed out of
// the geometry, and landings stop the fall, dip the eye (plrHEIGHT_CTRL) and may hurt.
void plrPLAYER::ProcessPhysics(m3dV *prev)
{
    cdtINFO info;
    info.flags = 0; info.inst = 0; info.objId = 0; info.faceId = 0;
    info.point[0] = m3dVZero.x; info.point[1] = m3dVZero.y; info.point[2] = m3dVZero.z;
    info.dist = 0;
    plrCDT_REFINE ref(0, plrPlayer->inst, 1);           // vtable v12 0x005FB664; walk through water volumes (1.2: mode 0, beta 2)
    m3dV pos, out;
    inst->GetPos(&pos);
    if (moveFlags & PLR_MF_WATER) {                     // water drag
        float r = 1.0f / gsElapsedTime;
        float kh = 5.0f < r ? 5.0f : r;
        float kv = 20.0f < r ? 20.0f : r;
        vel.x = (1.0f - gsElapsedTime * kh) * vel.x;
        vel.z = (1.0f - gsElapsedTime * kh) * vel.z;
        vel.y = (1.0f - gsElapsedTime * kv) * vel.y;
    }
    vel.y = vel.y - gsElapsedTime * gravity;
    if (plrSlideOnSlopes && (floorInfo.flags & 2) &&
        (floorHeight < 0.05f || (pushTime > 0.4f && floorHeight < 5.0f))) {
        m3dV n = m3dVUnitY;                             // (left unset by the original when neither info is valid)
        if (floorHeight < 0.05f)
            gsScenePtr->GetCDTFace(&floorInfo, 0, &n);
        else if (_plrLastCDTInfo.flags & 2)
            gsScenePtr->GetCDTFace(&_plrLastCDTInfo, 0, &n);
        float ang = m3dAngleVector(&n, &m3dVUnitY);
        float dn = n.z * m3dVUnitY.z + n.y * m3dVUnitY.y + n.x * m3dVUnitY.x;
        m3dV t = { n.x - m3dVUnitY.x * dn, n.y - m3dVUnitY.y * dn, n.z - m3dVUnitY.z * dn };
        float k = ang < 45.0f ? 0.0f : (ang > 60.0f ? 1.0f : (ang - 45.0f) * (1.0f / 15.0f));
        k = k * gsElapsedTime * 300.0f;                 // steeper than 45 degrees: slide down
        vel.x += t.x * k;
        vel.y += t.y * k;
        vel.z += t.z * k;
    }
    float dt = gsElapsedTime;
    pos.x = dt * ctrlVel.x + (dt * vel.x + pos.x);
    pos.y = dt * ctrlVel.y + (dt * vel.y + pos.y);
    pos.z = dt * ctrlVel.z + (dt * vel.z + pos.z);
    inst->Translate(&pos, 0);
    float up;
    if (moveFlags & 0xC00)
        up = 2.5f;
    else if (!(gsElapsedTime * gravity <= stepHeight))
        up = gsElapsedTime * gravity * 1.1f;
    else
        up = stepHeight;
    gsScenePtr->FloorPlace(&pos, 2000000.0f, &out, up, 0, 0, &ref, &floorInfo);
    if ((moveFlags & 0xC00) && (moveFlags & 0x20)) {    // swimming and moving: also probe ahead
        m3dV d = { velocity[0], velocity[1], velocity[2] };
        if (!_m3dNormalize(&d))
            inst->GetDir(&d);
        float k = radius + 0.1f;
        m3dV ahead = { d.x * k + pos.x, d.y * k + pos.y, d.z * k + pos.z };
        m3dV o2;
        if (gsScenePtr->FloorPlace(&ahead, 2000000.0f, &o2, up, 0, 0, &ref, &info) && !(o2.y <= out.y)) {
            floorInfo = info;
            out = o2;
        }
    }
    if (!(floorInfo.flags & 2))
        out = pos;
    // --- moving platforms: carried by an instance with flags 0x40 when standing on it ---
    if (!(floorInfo.flags & 2) || !floorInst || !(floorInst->flags & 0x40) || !(floorHeight < 0.1f)) {
        if (platInst) {                                 // stepped off: keep its velocity
            vel.x += platVel.x;
            vel.y = platVel.y + vel.y;
            vel.z = platVel.z + vel.z;
        }
        platInst = 0;
        platObj = 0;
    } else {
        objOBJ *o = gsScenePtr->GetCDTObj(&floorInfo);
        floorInst->Validate(4);
        m3dMATR *ltm = (m3dMATR *)o->ltm;
        if (!platInst || platInst != floorInst || o != platObj) {
            platVel = m3dVZero;
        } else {
            m3dV w;
            m3dTransformPoint(&platLocalPos, ltm, &w);
            m3dV dlt = { w.x - platLastPos.x, 0, w.z - platLastPos.z };
            out.x += dlt.x;
            out.z += dlt.z;
            float r = 1.0f / gsElapsedTime;
            platVel.x = dlt.x * r;
            platVel.y = 0.0f * r;
            platVel.z = dlt.z * r;
            m3dV dw;
            m3dTransformVector(&platLocalDir, ltm, &dw);
            float a = m3xzAngleVector(&platLastDir, &dw);
            inst->RotateY(a, 2);
            m3dMATR rot;
            rot.MakeRotY(a);
            m3dTransformVector(&bodyDir, &rot, &bodyDir);
        }
        platLastPos = out;
        inst->GetDir(&platLastDir);
        m3dMATR inv;
        ltm->Invert(&inv);
        m3dTransformPoint(&platLastPos, &inv, &platLocalPos);
        m3dTransformVector(&platLastDir, &inv, &platLocalDir);
        platInst = floorInst;
        platObj = o;
    }
    UpdateUnderwater();
    u32 mf = moveFlags;
    if ((mf & 0xC00) && !(pos.y <= floorPoint.y))
        out = pos;
    if (mf & 0x2000) {                                  // still rising from a step-up
        if (pos.y < out.y)
            moveFlags = mf & ~0x2000u;
        if (!(pos.y <= out.y))
            out.y = pos.y;
    } else if (!(vel.y <= 0) && !(pos.y <= out.y)) {
        out.y = pos.y;                                  // going up: the floor does not pull down
    }
    if (stepHeight + floorPoint.y < pos.y) {            // above step height: fall freely this frame
        float vy = vel.y <= -1.0f ? vel.y : -1.0f;
        if (out.y < gsElapsedTime * vy + pos.y) {
            out.y = pos.y;
            moveFlags |= 0x2000;
        }
    }
    if (floorInfo.flags & 2)
        floorHeight = out.y - floorPoint.y;
    if (!(floorHeight <= 0.1f) && !(moveFlags & 0xC00) && pushTime < 0.4f)
        airTime += gsElapsedTime;
    else
        airTime = 0;
    fallDist = pos.y - prev->y + fallDist;
    if (floorHeight > 0.2f)
        heightCtrl->Reset();
    eyeHeight = heightCtrl->Update();
    prev->y = eyeHeight + prev->y;
    out.y = out.y + eyeHeight;
    float outY = out.y;
    // --- push the eye sphere out: first non-physics instances (height kept only on the crocodile) ---
    ref.mode = 0xD;
    int res = gsScenePtr->ConstrSphere(prev, &out, radius, 0x20, &ref, &info);
    if (!(res && info.inst && strstr(info.inst->className ? info.inst->className : (char *)"", "ai_croc")))
        out.y = outY;
    // --- then the level and the physics objects ---
    ref.mode = 2;
    if (gsScenePtr->ConstrSphere(prev, &out, radius, 0x30, &ref, &info)) {
        _plrLastCDTInfo = info;
        res = 1;
    }
    cdtREFINE vis;                                      // vtable v12 0x005FA0E0, mode 2
    vis.mode = 2;
    if (!gsScenePtr->IsVisiblePoint(prev, &out, 0x30, &vis, 0) && gsScenePtr->IsVisiblePoint(&out, prev, 0x30, &vis, 0))
        out = *prev;                                    // pushed through a wall: stay
    out.y = out.y - eyeHeight;
    prev->y = prev->y - eyeHeight;
    inst->Translate(&out, 0);
    if (!res) {
        pushTime = 0;
    } else {
        pushTime = gsElapsedTime + pushTime;
        float dx = prev->x - out.x, dz = prev->z - out.z;
        if (m3dSqrt(dz * dz + dx * dx) > 0.005f)
            moveFlags |= 0x8000;                        // pushed sideways
    }
    // --- floor contact: friction, landing ---
    if (!(floorInfo.flags & 2)) {
        vel = m3dVZero;
        moveFlags &= ~0x100u;
        goto end;
    }
    if (!(floorHeight < 0.1f) && !(moveFlags & 0xC00) && pushTime <= 0.4f)
        goto end;
    {
        m3dV h = { vel.x, 0, vel.z };
        if (_m3dNormalize(&h)) {                        // ground friction 200 / s on the horizontal velocity
            float k = gsElapsedTime * 200.0f;
            h.x *= k; h.y *= k; h.z *= k;
            if (!(h.z * h.z + h.x * h.x <= vel.x * vel.x + vel.z * vel.z)) {
                vel.z = 0;
                vel.x = 0;
            } else {
                vel.x = vel.x - h.x;
                vel.y = vel.y - h.y;
                vel.z = vel.z - h.z;
            }
        }
    }
    if (vel.y < 1e-6f * 10.0f && (floorHeight < 0.05f || stillTime > 0.5f || (moveFlags & 0xC00))) {
        u32 m = moveFlags;
        if ((m & 0x100) || fallDist < -3.0f)            // landing event for the animation / sounds
            unk108 |= (m & 0xC00) ? 0x80 : 0x240;
        moveFlags = m & ~0x100u;
        if (vel.y < -4.0f)
            heightCtrl->ApplySpeed(vel.y);
        if (fallDist < -4.0f && vel.y < -10.0f) {
            static plrDMG_MSG fall;                     // v12 0x00641B58, built once (guard 0x00641B56 bit 1)
            static int fallInit;
            if (!fallInit) {
                fallInit = 1;
                fall.b0 = 0; fall.b1 = 0x40; fall.size = 0x0E; fall.type = 5; fall.sub = 0;
            }
            float base = (moveFlags & PLR_MF_JUMP_UP) ? 40.0f : 0.0f;   // 1.2: jump pads give 40 free
            float x = -fallDist - base;
            int send = 0;
            if (moveFlags & 0xC00) {                    // into water: from 35 m, 25 at most
                if (x < 35.0f)
                    fall.amount = 0;
                else if (!(x <= 70.0f)) {
                    fall.amount = 25.0f;
                    send = 1;
                } else {
                    fall.amount = (x - 35.0f) * (25.0f / 35.0f);
                    send = fall.amount > 0.1f;
                }
            } else {                                    // from 20 m, 40 at most
                if (x < 20.0f)
                    fall.amount = 0;
                else if (!(x <= 50.0f)) {
                    fall.amount = 40.0f;
                    send = 1;
                } else {
                    fall.amount = (x - 20.0f) * (40.0f / 30.0f);
                    send = fall.amount > 0.1f;
                }
            }
            if (send && gsSysClient)                    // (never 0 in the game; the test level has no client)
                wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x4C6, (void *)&fall, 0x20000);
        }
        vel.y = 0;
        fallDist = 0;
    }
    moveFlags &= ~(u32)PLR_MF_JUMP_UP;
    if (floorInst && (WR_FIELD(u8, floorInst->ent, 0xBC) & 2) && floorHeight < 1.0f &&
        !wrEVCall<BOOL32>(this, 0x74)) {                 // standing on a hurting entity (class bit 2): 200 / s
        static plrDMG_MSG burn;                         // v12 0x00641BCC (guard 0x00641B56 bit 2)
        static int burnInit;
        if (!burnInit) {
            burnInit = 1;
            burn.b0 = 0; burn.b1 = 0x40; burn.size = 0x0E; burn.type = 0; burn.sub = 0;
        }
        burn.source = WR_FIELD(u32, floorInst->ent, 0x88);
        burn.sub = 4;
        burn.amount = gsElapsedTime * 200.0f;
        if (gsSysClient)
            wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x4C6, (void *)&burn, 0x20000);
    }
end:
    {
        float vf = (out.y - prev->y) / gsElapsedTime;   // vertical speed the floor allowed
        if (!(vf <= 0) && vel.y < 0) {
            vel.y = 0;
        } else {
            float sgn = !(vel.y <= 0) ? 1.0f : (vel.y < 0 ? -1.0f : 0.0f);
            float av = fabsf(vel.y), af = fabsf(vf);
            float lo = 0, hi = av;                      // inline clamp(af, 0, |vel.y|)
            if (av < 0) { lo = av; hi = 0; }
            float c = af < lo ? lo : (hi < af ? hi : af);
            vel.y = c * sgn;
        }
    }
    stillTime = fabsf(vel.y) < 0.1f ? gsElapsedTime + stillTime : 0;
    if (moveFlags & PLR_MF_WATER)
        gravity = 2.0f;
    else if (moveFlags & PLR_MF_NO_GRAVITY)
        gravity = 0;
    else
        gravity = 30.0f;
}

// ---------------------------------------------------------------------------------------------
// UpdateUnderwater
// ---------------------------------------------------------------------------------------------

// v12 0x00422020  (feb 0x0041ED00)
// Water state from the camera: under the surface = WATER (0x800, gravity 2); within 0.25 above it while
// swimming = NO_GRAVITY (0x400, floating). Leaving the water sets the breath flags; shallow water below
// the eye sets 0x20000. Under water the breath time runs; from 26 s on, 10 damage per second.
void plrPLAYER::UpdateUnderwater()
{
    m3dV cam;
    gsCameraPtr->GetOrigin(&cam);
    u32 wasWater = moveFlags & PLR_MF_WATER;
    float *level = &WR_FIELD(float, this, 0x1CF);
    if (!(moveFlags & PLR_MF_LEVEL_START) && waterIsUnderWater(&cam, level, 0.3f)) {
        u32 m = moveFlags;
        float k = (m & PLR_MF_NO_GRAVITY) ? -0.25f : 0.0f;
        if (k + *level < cam.y)
            moveFlags = (m & ~0x800u) | 0x400;
        else
            moveFlags = (m & ~0x400u) | 0x800;
    } else {
        moveFlags &= ~0xC00u;
    }
    if (wasWater && !(moveFlags & 0x8000800)) {         // came up: gasp (by how long we were under)
        float b = breathTime * 4.0f;
        u32 f = WR_FIELD(u32, this, 0x1FF);
        if (b < 33.0f)
            f |= 1;
        else if (b < 66.0f)
            f |= 2;
        else
            f |= 4;
        WR_FIELD(u32, this, 0x1FF) = f;
        entFlags |= 0x200;
    }
    moveFlags &= ~0x20000u;
    if (!(moveFlags & 0xC00)) {
        entCDT_REFINE_INCL wref(1, 0, 1);               // vtable v12 0x005FB674: only water volumes
        cdtINFO wi = { 0 };
        wi.point[0] = m3dVZero.x; wi.point[1] = m3dVZero.y; wi.point[2] = m3dVZero.z;
        if (gsScenePtr->FloorPlace(&cam, 1.6f, &cam, 0, 0, 0, &wref, &wi) && (floorInfo.flags & 2) &&
            !(floorPoint.y + 1.5f <= wi.point[1]))
            moveFlags |= 0x20000;                       // wading: a water surface less than 1.5 above the floor
    }
    if ((moveFlags & PLR_MF_WATER) && !wrEVCall<BOOL32>(this, 0x74)) {
        breathTime = gsElapsedTime + breathTime;
        if (!(breathTime < 26.0f)) {
            static plrDMG_MSG drown;                    // v12 0x00641BE0 (guard 0x00641B54 bit 1)
            static int drownInit;
            if (!drownInit) {
                drownInit = 1;
                drown.b0 = 0; drown.b1 = 0x40; drown.size = 0x0E; drown.type = 0; drown.sub = 0;
            }
            drown.amount = 10.0f;
            drown.sub = 5;
            if (gsSysClient)
                wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x4C6, (void *)&drown, 0x20000);
            breathTime = breathTime - 1.0f;
        }
        return;
    }
    if (!(breathTime <= 1e-6f)) {                       // recover 20 times as fast
        breathTime = breathTime - gsElapsedTime * 20.0f;
        if (!(breathTime > 1e-6f))
            breathTime = 0;
    }
}

// v12 0x00422310  (feb 0x0041EEA0)
// The air bar: 1 at the surface, 0 after 25 s under water (breathTime counts up in UpdateUnderwater).
float plrPLAYER::GetUnderwaterTimeLeft()
{
    if (!(breathTime < 25.0f))
        return 0.0f;
    return (25.0f - breathTime) * 0.04f;
}

#pragma pack(push, 1)
struct plrCAM_MSG : msgDATA {                           // GS_MSG_MP_NOTIFY 0x40E: the camera (0x36 bytes)
    short type;                                         // +0x04 0x12
    m3dV  org, right, up, forw;                         // +0x06 +0x12 +0x1E +0x2A
};
#pragma pack(pop)

// v12 0x00422340  (feb 0x0041EED0)
// The game camera at the eyes (the player instance's right, up and forward), then the camera control (zoom, recoil,
// death view). Debug views: moveFlags 0x100000 from 3 behind, noclip a marker sphere and the axes; gsAppState 0x400
// sends the camera to the clients.
void plrPLAYER::UpdateCamera()
{
    if (gsAppState & 0x800)
        return;
    if (!(moveFlags & PLR_MF_FLYTHROUGH)) {
        m3dV pos, dir, right;
        inst->GetPos(&pos);
        pos.y += eyeHeight;
        inst->GetDir(&dir);
        inst->GetDirRight(&right);
        if (!(moveFlags & 0x100000)) {
            m3dV up;
            up.x = right.y * dir.z - right.z * dir.y;
            up.y = right.z * dir.x - right.x * dir.z;
            up.z = right.x * dir.y - right.y * dir.x;
            gsCameraPtr->SetParameters(&pos, &up, &right, &dir);
            camCtrl->Update();
        } else {
            m3dV eye;
            eye.x = pos.x - dir.x * 3.0f;
            eye.z = pos.z - dir.z * 3.0f;
            eye.y = pos.y - dir.y * 3.0f + 1.0f;
            gsCameraPtr->MoveTo(&eye);
            gsCameraPtr->SetInterest(&pos);
        }
    } else {
        struct { const void *vtbl; int type; m3dV center; float radius; } s;   // an m3dSPHERE (vtable v12 0x005FB68C,
        s.type = 3;                                                             //  not available here)
        s.vtbl = 0;
        inst->GetPos(&s.center);
        s.center.y += eyeHeight;
        s.radius = 0.7f;
        rendDrv->RenderSphere(gsCameraPtr, (m3dSPHERE *)&s, 0xFFFFFFFF);
        rendDrv->RenderCS(gsCameraPtr, inst->matr, 1.2f);
    }
    if (gsAppState & 0x400) {
        plrCAM_MSG m;
        m.packId = 0;
        m.tag = 0x40;
        m.type = 0x12;
        m.size = 0x36;
        gsCameraPtr->GetParameters(&m.org, &m.up, &m.right, &m.forw);
        gsSysServer->SendMsgClientAll(this, 0x40E, &m, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
    }
}

// v12 0x00423890  (feb 0x00420430)
// The first-person weapon's sequence from the state (taking off, putting on, reloading, shooting, running, the idle
// flourish after 10 s), the switch when it is put away (action frames 4 / 5), the end of the switch (6), the reload
// when the clip runs dry (or the next weapon when the ammo does), and the footsteps (action frame 3 of the run).
void plrPLAYER::AnimateWeapon()
{
    pwpWEAPON *w = weapons[curWeapon];
    u32 mf = moveFlags;
    if (mf & 2)
        return;
    animINST *wi = w->inst;
    if ((gsAppState & 0x800) && !(entFlags & 4)) {      // replay: the recorded sequence
        wrEVCall<void>(w, 0xB8, unk_307, gsElapsedTime);
    } else {
        int seq;
        if (mf & 8)
            seq = w->ammo > 0.0f ? 4 : 5;               // TAKE_OFF / TAKE_OFF_EMPTY
        else if (mf & 0x10)
            seq = 6;                                    // PUT_ON
        else if (mf & 0x80)
            seq = 8;                                    // RECHARGE
        else if (mf & 0x10000000)
            seq = 9;                                    // FORCE_RECHARGE
        else if (mf & 0x40)
            seq = 7;                                    // SHOOT
        else if ((mf & 0x20) && (floorHeight < 0.1f || (mf & 0xC00)))
            seq = 3;                                    // RUN
        else if ((w->flags2 & 1) && unk_293 > 10.0f)
            seq = 2;                                    // IDLE_1
        else
            seq = 1;                                    // IDLE
        wrEVCall<void>(w, 0xB8, seq, gsElapsedTime);     // AdvanceAnimation
        if (wi->IsActionFrame(5, 0) || wi->IsActionFrame(4, 0)) {
            wrEVCall<void>(this, 0x108, nextWeapon, 0);  // SetCurWpn
            w = weapons[nextWeapon];
            wi = w->inst;
            if (w->clip <= 0) {
                if (entFlags & 2) {
                    plrNOTIFY_MSG m;
                    m.packId = 0;
                    m.tag = 0x40;
                    m.size = 6;
                    m.type = 8;
                    wrEVCall<int>(gsSysClient, 0x2C, (void *)plrPlayer, 0x40E, (void *)&m, 0x20000);
                }
                weapons[curWeapon]->Recharge();
            }
            wrEVCall<void>(w, 0xB8, 6, gsElapsedTime);
            moveFlags &= ~8u;
        } else if (wi->IsActionFrame(6, 0) && seq != 4) {
            moveFlags &= ~0x18u;
        }
        if (wrEVCall<BOOL32>(w, 0xC8) && w->clip <= 0) { // IsReadyRecharge, the clip empty
            if (w->ammo <= 0.0f) {
                moveFlags |= 0x18;
                nextWeapon = GetNextPriorWeapon();
            } else if (w->clipSize != -1) {
                moveFlags |= 0x80;
            }
            ClearZoom();
        } else if (wrEVCall<int>(w, 0xBC) != 7) {
            moveFlags &= ~0x10000080u;
        }
        if (wrEVCall<int>(w, 0xBC) == 1 && !(moveFlags & 0x40))
            unk_293 += gsElapsedTime;                   // idle time (IDLE_1 after 10 s)
        else
            unk_293 = 0;
    }
    if (wi->IsActionFrame(3, 0)) {                      // a step: left, then right below
        if (moveFlags & 0x20000)
            unk108 |= 0x10;
        else if (moveFlags & 0xC00)
            unk108 |= 0x800000;
        else if (floorHeight < 0.2f && !(moveFlags & 0x200))
            unk108 |= 1;
    } else if (wi->IsActionFrame(3, 1)) {
        if (moveFlags & 0x20000)
            unk108 |= 0x20;
        else if (!(moveFlags & 0xC00) && floorHeight < 0.2f && !(moveFlags & 0x200))
            unk108 |= 2;
    }
}

// v12 0x00423C90  (feb 0x00420790)
// Weapon w (owned) in the hands: at once (animate 0: the old model hidden, the new shown), or by putting the current
// one away first. The body and the ghost render state follow.
void plrPLAYER::SetCurWpn(int w, int animate)
{
    pwpWEAPON *nw = weapons[w];
    if (!nw || !(nw->flags & WPNF_ENABLED))
        return;
    plrPLAYER_COMMON::SetCurWpn(w, animate);
    if (!(gsAppState & 0x800) && (entFlags & 2)) {
        plrWPN_MSG m;
        m.packId = 0;
        m.tag = 0x40;
        m.type = 1;
        m.size = 0x0E;
        m.wpn = w;
        m.animate = animate;
        wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x40E, (void *)&m, 0x20000);
    }
    if (!animate) {
        if (weapons[curWeapon])
            weapons[curWeapon]->inst->flags |= 1;
        curWeapon = w;
        weapons[w]->inst->flags &= ~1u;
    } else {
        nextWeapon = w;
        moveFlags |= 0x18;
        ClearZoom();
    }
    if (body)
        wrEVCall<void>(body, 0x108, w, animate);
    SetupGhostRendState();
}

// v12 0x00423DE0  (feb 0x00420840)
// The aim ray through the crosshair, at most 5 times a second unless shooting: what it hits (gscCursor.aimInfo /
// aimPoint / aimEnt; when it hits nothing, the point 50 ahead on the view plane), the auto-aim for the bullet
// weapons (GetTargetInst), the crosshair's colour (the target's health) and size (its distance).
void plrPLAYER::UpdateCursor()
{
    m3dV dir, from;
    entCDT_REFINE_EXCL ref(0, inst, 0);
    gsCameraPtr->GetViewWindowRayDir(gscCursor.x, gscCursor.y, &dir);
    gsCameraPtr->TransformPointC2W(&m3dVZero, &from);
    cdtINFO *ci = gscCursor.aimInfo;
    if (realTimer > unk_2A3 || (moveFlags & 0x200000)) {
        realTimer = 0;
        m3dNormalize(&dir);
        u32 shooting = moveFlags & 0x200000;
        if (!shooting || (moveFlags & PLR_MF_WATER))
            ref.cls.bits |= 1;                          // not through water volumes
        if (!shooting)
            ref.mode |= 0xA;                            // instances only
        pwpWEAPON *w = weapons[curWeapon];
        float range = WR_FIELD(int, w, 0x98) == PWP_CROSSBOW && (w->flags2 & PWPF2_ZOOM) ? 1500.0f : 150.0f;
        int hit = gsScenePtr->ShootRay(&from, &dir, range, 0x200, &ref, gscCursor.aimInfo);
        int found = hit;
        if (!shooting) {                                // then the level up to that instance
            cdtINFO ci2;
            ci2.flags = 0;
            ci2.inst = 0;
            ci2.objId = 0;
            ci2.faceId = 0;
            *(m3dV *)ci2.point = m3dVZero;
            ci2.dist = 0;
            if (hit)
                range = gscCursor.aimInfo->dist < 150.0f ? gscCursor.aimInfo->dist : 150.0f;
            ref.mode = (ref.mode & ~0xAu) | 1;
            if (gsScenePtr->ShootRay(&from, &dir, range, 0x200, &ref, &ci2)) {
                *gscCursor.aimInfo = ci2;
                found = 1;
            }
        }
        if (found) {
            range = gscCursor.aimInfo->dist;
            gsScenePtr->GetCDTObj(gscCursor.aimInfo);
        } else {
            range = 100.0f;
        }
        ci = gscCursor.aimInfo;
        if (!(ci->inst && (WR_FIELD(u32, ci->inst->ent, 0xBC) & 4)) && curWeapon != PLR_WPN_CRB &&
            curWeapon != PLR_WPN_BSK && curWeapon != PLR_WPN_ACD && curWeapon != PLR_WPN_AMG &&
            curWeapon != PLR_WPN_BMB && curWeapon != PLR_WPN_MDG) {
            GetTargetInst(&from, &dir, range, gscCursor.aimInfo, 3.0f);
            ci = gscCursor.aimInfo;
        }
    }
    gscCursor.aimEnt = 0;
    if (ci->flags & 2) {
        gscCursor.aimPoint = *(m3dV *)ci->point;
        if (ci->inst)
            gscCursor.aimEnt = ci->inst->ent;
    } else {
        m3dV p, d, pp, n, hitp;
        wrEVCall<void>(this, 0x64, &p);                  // GetPos
        p.y += eyeHeight;
        wrEVCall<void>(this, 0x6C, &d);                  // GetDir
        m3dNormalize(&d);
        n.x = d.x * -1.0f;
        n.y = d.y * -1.0f;
        n.z = d.z * -1.0f;
        pp.x = d.x * 50.0f + p.x;
        pp.y = d.y * 50.0f + p.y;
        pp.z = d.z * 50.0f + p.z;
        if (!m3dIsectRayVNPlane(&from, &dir, &pp, &n, &hitp, 0)) {
            hitp.x = d.x * 50.0f + p.x;
            hitp.z = d.z * 50.0f + p.z;
            hitp.y = p.y + 1.0f;
        }
        gscCursor.aimPoint = hitp;
    }
    wrCURSORS::pInstance->SetCursorType(1);
    u32 color = 0xFFDFDFDF;
    entENTITY *e = gscCursor.aimEnt;
    if (e) {
        float h;
        int target = 0;
        if (e->classId == 0x244E5043) {                 // '$NPC' (not one that hides its health)
            if (!(WR_FIELD(u32, e, 0x46F) & 0x200000)) {
                h = WR_FIELD(float, e, 0xD0) / WR_FIELD(float, e, 0xD4);
                target = 1;
            }
        } else if (e->classId == 0x504C5952 && !wrEVCall<BOOL32>(e, 0x74)) {   // 'PLYR', alive
            h = WR_FIELD(float, e, 0xD0) * 0.01f;
            target = 1;
        }
        if (target) {
            if (h > 0.5f)
                color = 0xBF00FF00;
            else if (h > 0.25f)
                color = 0xBFFFFF00;
            else
                color = 0xBFFF0000;
            wrCURSORS::pInstance->SetCursorType(0);
        }
    }
    float scale;
    if (gscCursor.aimInfo->flags & 2) {
        float d = gscCursor.aimInfo->dist;
        if (d < 10.0f)
            scale = 0.9f;
        else if (d > 70.0f)
            scale = 0.65f;
        else
            scale = 0.9f - (d - 10.0f) * (1.0f / 240.0f);
    } else {
        scale = 0.65f;
    }
    gscCursor.scale = scale;
    gscCursor.color = color;
    if (!wrEVCall<BOOL32>(this, 0x74) && curWeapon != PLR_WPN_SVL && !(moveFlags & 0x20000000))
        gscCursor.SetHide(1, 0);
}

// v12 0x00424310  (feb 0x00420D50)
// The auto-aim: the living NPC within range of the ray (and 20 or more away) nearest to it in angle -- the angle
// allowed is maxAng times 0.5 up to 30, falling to 0.2 at 70 and beyond -- and in sight: its "target" object (else
// 1 above its origin) becomes the hit of ci. Note the original takes the fall-back position and the distance from
// the last NPC it looked at, not the chosen one.
animINST *plrPLAYER::GetTargetInst(m3dV *from, m3dV *dir, float range, cdtINFO *ci, float maxAng)
{
    plrCDT_REFINE_NPC ref(plrPlayer->inst);
    m3dV to, p, d;
    to.x = range * dir->x + from->x;
    to.y = range * dir->y + from->y;
    to.z = range * dir->z + from->z;
    scnAOBJ list[20];
    int n = gsScenePtr->MakeAnimList(from, &to, 3.0f, &ref, list, 20);
    float bestAng = maxAng;
    animINST *best = 0, *last = 0;
    for (int i = 0; i < n; i++) {
        animINST *a = list[i].inst;
        if (a->ent->classId != 0x244E5043)              // '$NPC'
            continue;
        last = a;
        objOBJ *o = objFindName(a->root, (char *)"target");
        if (o) {
            objValidateLTM(o);
            o->GetOrigin(&p);
        } else {
            a->GetPos(&p);
            p.y += 1.0f;
        }
        d.x = p.x - from->x;
        d.y = p.y - from->y;
        d.z = p.z - from->z;
        float dist = m3dSqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        if (dist < 20.0f || dist > range)
            continue;
        float ang = m3dAngleVector(&d, dir);
        float dd = m3dSqrt(d.x * d.x + d.y * d.y + d.z * d.z);
        float k;
        if (dd < 30.0f)
            k = 0.5f;
        else if (dd > 70.0f)
            k = 0.2f;
        else
            k = 0.5f - (dd - 30.0f) * 0.0075f;
        if (!(ang < maxAng * k) || !(ang < bestAng))
            continue;
        bestAng = ang;
        best = list[i].inst;
    }
    if (!best || !ci)
        return best;
    objOBJ *o = objFindName(best->root, (char *)"target");
    if (o) {
        o->GetOrigin((m3dV *)ci->point);
    } else {
        last->GetPos((m3dV *)ci->point);
        ci->point[1] += 1.0f;
    }
    cdtREFINE vis(2, 0);
    if (!gsScenePtr->IsVisiblePoint(from, (m3dV *)ci->point, 0, &vis, 0))
        return 0;
    ci->objId = 0;
    ci->inst = best;
    ci->faceId = 0;
    d.x = p.x - from->x;
    d.y = p.y - from->y;
    d.z = p.z - from->z;
    ci->dist = m3dSqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    ci->flags = (ci->flags & ~0x100u) | 2;
    return best;
}

// v12 0x00424C10  (feb ProcessUse 0x004215C0)
// The use key: the first instance within 10 the player can use (IsCanUse) -- a powerup altar sells its powerup
// when the gold is there (else a message says why not), anything else is used (entity slot +0x64).
void plrPLAYER::ProcessUse()
{
    plrCDT_REFINE_BUTTON ref(plrPlayer->inst);
    m3dV pos;
    inst->GetPos(&pos);
    scnAOBJ list[20];
    int n = gsScenePtr->MakeAnimList(&pos, &pos, 10.0f, &ref, list, 20);
    int i;
    for (i = 0; i < n; i++)
        if (IsCanUse(list[i].inst, 0))
            break;
    if (i == n)
        return;
    int pwr = -1;                                       // (the original leaves it unset for other entity types:
    switch (WR_FIELD(int, list[i].inst->ent, 0x98)) {   //  only type 0 and the altars 1000..1002 can be used)
    case 1000: pwr = PLR_PWR_QUAD_DAMAGE; break;
    case 1001: pwr = PLR_PWR_UNHOLY_ARMOR; break;
    case 1002: pwr = PLR_PWR_TIMESCALE; break;
    }
    if (pwr != -1 && !IsCanBuyPowerup(pwr)) {
        if (powerup[pwr].state)
            uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId("GAME_MSG_POWERUP_POSSESSED")), 3.5f, 0);
        else
            uiSystem->ShowMsgW(gsStrings->Printf("GAME_MSG_NOT_ENOUGH_GOLD", (int)_plrPwrCostTbl[pwr]), 8.0f, 0);
        return;
    }
    wrEVCall<void>(list[i].inst->ent, 0x64, (void *)this);   // Use
}

// v12 0x00424DB0  (feb 0x00421760)
// Once a second, when no HUD message is up: the "use" tip of every usable instance within 10 (the altars name
// their powerup).
void plrPLAYER::NotifyMsgUse()
{
    plrCDT_REFINE_BUTTON ref(plrPlayer->inst);
    if (timer_29B < 1.0f || uiSystem->IsMsgShown())
        return;
    m3dV pos;
    timer_29B = 0;
    inst->GetPos(&pos);
    scnAOBJ list[20];
    float dist;
    int n = gsScenePtr->MakeAnimList(&pos, &pos, 10.0f, &ref, list, 20);
    for (int i = 0; i < n; i++) {
        if (!IsCanUse(list[i].inst, &dist))
            continue;
        const char *tip;
        switch (WR_FIELD(int, list[i].inst->ent, 0x98)) {
        case 0:    tip = "GAME_TIP_USE"; break;
        case 1000: tip = "GAME_TIP_BUY_TITAN_DAMAGE"; break;
        case 1001: tip = "GAME_TIP_BUY_IMMORTALITY"; break;
        case 1002: tip = "GAME_TIP_BUY_TITAN_MOTION"; break;
        default:   continue;
        }
        uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId(tip)), 1.0f, 0);
    }
}

// v12 0x00424F20  (feb 0x004218C0)
// Can the player use (push) this instance: within 4 of the player's height + 1, the entity's IsCanPush
// (slot +0x68) agrees, and it is in front -- a cone of 5 and 90 degrees from the player -- or, for a
// "katapult", nearer than 10 on the ground. *dist (may be 0) = the distance on the ground.
BOOL32 plrPLAYER::IsCanUse(animINST *a, float *dist)
{
    BOOL32 r = 0;
    m3dV me, dir, it;
    inst->GetPos(&me);
    inst->GetDir(&dir);
    me.y += 1.0f;
    a->GetPos(&it);
    if (fabsf(me.y - it.y) > 4.0f)
        return 0;
    it.y = me.y;
    if (dist) {
        float dx = it.x - me.x, dy = 0.0f, dz = it.z - me.z;
        *dist = m3dSqrt(dx * dx + dy * dy + dz * dz);
    }
    if (!wrEVCall<BOOL32>(a->ent, 0x68, this))
        return 0;
    if (strstr(a->name ? a->name : (char *)"", "katapult")) {
        float dx = it.x - me.x, dz = it.z - me.z;
        if (m3dSqrt(dx * dx + dz * dz) < 10.0f)
            r = 1;
        return r;
    }
    if (m3dIsBelongPointCone(&it, &me, &dir, 5.0f, 0.0f, 90.0f))
        r = 1;
    return r;
}

// v12 0x00425680  (feb 0x00422030)
BOOL32 plrPLAYER::HasKey()
{
    return (moveFlags >> 12) & 1;
}

// v12 0x00425690  (feb 0x00422040)
void plrPLAYER::UseKey()
{
    moveFlags &= ~PLR_MF_KEY;
}

#pragma pack(push, 1)
struct plrPOWERUP_MSG : msgDATA {                       // GS_MSG_MP_NOTIFY 0x40E: a powerup switched (0x0E bytes)
    short type;                                         // +0x04 7
    int   id;                                           // +0x06 plrPOWERUP_ID
    int   on;                                           // +0x0A
};
#pragma pack(pop)

// v12 0x0041E730  (feb 0x0041BA90)
// The hit (not when already dead): COMMON takes the health; a weapon hit (type 1, subtypes 6 / 9 / 4) or an
// explosion (types 3 / 4) of more than 20 (or more than 7, 80 % of the time) pushes the living player away from
// its point (upwards 0.7 .. 1.3, less the faster he moves); more than 1 point: the HUD's pain flash (red, blue
// with the unholy armor; red also smears blood on the hands from the hit's point); an explosion (type 2) calls
// the render system's (empty in 1.2) shake. timer_297 restarts.
void plrPLAYER::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (health < 0.0f)
        return;
    plrPLAYER_COMMON::ProcessCDT_FIREABLE(d);
    m3dV from;
    int push = 0;
    if (d->type == 1) {
        int sub = *(int *)((u8 *)d + 0x1A);
        if (sub == 6 || sub == 9 || sub == 4)
            push = 1;
    } else if (d->type == 3 || d->type == 4) {
        push = 1;
    }
    m3dV pos;
    if (push) {
        from = *(m3dV *)((u8 *)d + 0x0E);
        if (((d->amount > 7.0f && m3dRandom() > 0.2f) || d->amount > 20.0f) && health > 0.0f) {
            inst->GetPos(&pos);
            pos.y = pos.y + 0.7f;
            m3dV dir;
            dir.x = pos.x - from.x;
            dir.y = pos.y - from.y;
            dir.z = pos.z - from.z;
            if (dir.y < 0.0f)
                dir.y = 0;
            m3dNormalize(&dir);
            if (dir.y < 0.7f)
                dir.y = 0.7f;
            else if (dir.y > 1.3f)
                dir.y = 1.3f;
            m3dNormalize(&dir);
            float sp = m3dSqrt(vel.x * vel.x + vel.y * vel.y + vel.z * vel.z), f;
            if (sp < 1.0f)
                f = 1.0f;
            else if (sp > 6.0f)
                f = 0.05f;
            else
                f = 1.0f - (sp - 1.0f) * 0.19f;
            f = f * 12.0f;
            dir.x = dir.x * f;
            dir.y = dir.y * f;
            dir.z = dir.z * f;
            wrEVCall<void>(this, 0xA4, &dir, 0);        // ApplySpeed
        }
    }
    wrEVCall<void>(this, 0x64, &pos);                   // GetPos
    switch (d->type) {
    case 1: case 2: case 3: case 4:
        pos = *(m3dV *)((u8 *)d + 0x0E);
        break;
    }
    if (d->amount > 1.0f) {
        u32 c;
        if (IsActivePowerup(PLR_PWR_UNHOLY_ARMOR)) {
            c = 0x800050C8;
        } else {
            NotifySFXBloodHands(&pos);
            c = 0xFFFF0000;
        }
        if (uiSystem)                                   // (test: none without the menu)
            wrUi()->ShowPlayerPain(1, c);
    }
    if (!(entENTITY::flags2 & 4) && d->type == 2)
        gsSysRender->Shake(0x28FF1414, 1, 0.2f);
    timer_297 = 0;
}

// v12 0x004259F0  (feb 0x00422360)
// COMMON's part, then "hurt" (flags2 0x20) while alive.
void plrPLAYER::NotifyHealthChange(dmgDAMAGE *d)
{
    plrPLAYER_COMMON::NotifyHealthChange(d);
    if (!(health <= 1e-6f))
        plrPLAYER_COMMON::flags2 |= 0x20;
}

// v12 0x004251A0  (feb 0x00421B50)
// The player's clocks: two in game time, one in real time (game time over the time scale); the screen flash runs out;
// unless paused for a save (gsAppState 0x200), the active powerups count down in real time and end (slow motion:
// the normal time scale again).
void plrPLAYER::UpdateTimers()
{
    float ts = gsMsgGetElapsedTimeScale();
    timer_297 += gsElapsedTime;
    timer_29B += gsElapsedTime;
    float inv = 1.0f / ts;
    realTimer += inv * gsElapsedTime;
    if (!(flashTimer < 0)) {
        flashTimer += gsElapsedTime;
        if (!(flashTimer < flashPeriod))
            flashTimer = -1.0f;
    }
    if (gsAppState & 0x200)
        return;
    for (int i = 0; i < PLR_PWR_COUNT; i++) {
        if (powerup[i].state != PLR_PWR_ACTIVE)
            continue;
        powerup[i].timeLeft -= inv * gsElapsedTime;
        if (powerup[i].timeLeft >= 0)
            continue;
        plrPOWERUP_MSG m;
        m.packId = 0;
        m.tag = 0x40;
        m.id = 0;
        m.on = 0;
        m.type = 7;
        m.size = 0x0E;
        if (!IsActivePowerup(i))
            continue;
        powerup[i].state = PLR_PWR_NONE;
        powerup[i].timeLeft = 0;
        if (i == PLR_PWR_TIMESCALE)
            gsMsgSetElapsedTimeScale(1.0f);
        unk108 |= 0x10000000;
        m.id = i;
        m.on = 0;
        wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x40E, (void *)&m, 0x20000);
    }
}

// v12 0x004256B0  (feb 0x00422060)  inline, also emitted
// Out of the crossbow's zoom: the camera's view angle back, the normal HUD, the weapon model shown again.
void plrPLAYER::ClearZoom()
{
    if (!(moveFlags & 0x20000000))
        return;
    camCtrl->ZoomClear();
    uiSystem->ShowHudFrame(0);
    moveFlags &= ~0x20000000u;
    weapons[curWeapon]->inst->flags &= ~1u;
    weapons[curWeapon]->SetZoom(0);
}

// v12 0x00425710  (feb 0x004220C0)
// The current weapon can fire: under water only the shovel, colt, shotgun, machine gun and minigun; it has ammo,
// it is ready itself (slot +0x78) and it is not being put away, taken out, reloaded or in its idle flourish.
int plrPLAYER::IsReadyShoot()
{
    pwpWEAPON *w = weapons[curWeapon];
    int seq = wrEVCall<int>(w, 0xBC);                    // GetCurAnimSeq
    if (moveFlags & PLR_MF_WATER) {
        int t = WR_FIELD(int, w, 0x98);
        if (t != PWP_SHOVEL && t != PWP_COLT && t != PWP_STG && t != PWP_MINIGUN && t != PWP_MACHINEGUN)
            return 0;
    }
    if (!(w->ammo > 0.0f))
        return 0;
    if (!wrEVCall<int>(w, 0x78))                         // IsReadyShoot
        return 0;
    if (seq == 4 || seq == 5 || seq == 6 || seq == 8 || seq == 9 || seq == 2)
        return 0;                                       // TAKE_OFF(_EMPTY), PUT_ON, (FORCE_)RECHARGE, IDLE_1
    return 1;
}

// v12 0x004257B0  (feb 0x00422160)
// A held powerup becomes active for 30 s (slow motion: the time scale 0.25); in single player Will says so.
// While loading / in MP replay (gsAppState & 0x20200) only an already active one is re-announced.
void plrPLAYER::ActivatePowerup(int id)
{
    plrPOWERUP_MSG m;
    m.packId = 0;
    m.id = 0;
    m.on = 0;
    m.tag = 0x40;
    m.type = 7;
    m.size = 0x0E;
    if (!(gsAppState & 0x20200)) {
        if (powerup[id].state != PLR_PWR_HELD)
            return;
        powerup[id].state = PLR_PWR_ACTIVE;
        powerup[id].timeLeft = PLR_POWERUP_TIME;
        unk108 |= 0x8000000;
    } else if (powerup[id].state != PLR_PWR_ACTIVE) {
        return;
    }
    if (id == PLR_PWR_TIMESCALE)
        gsMsgSetElapsedTimeScale(PLR_POWERUP_SLOWMO);
    m.id = id;
    m.on = 1;
    wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x40E, (void *)&m, 0x20000);
    if (WR_FIELD(int, gsSysMP, 0x28) == 1 && !(gsAppState & 0x20000))
        wrMSG_SYSTEM::ShowMsg((char *)"LIN_P_vengeance_mine", 1.5f);
}

// v12 0x00425A40  (feb 0x004223F0)
// The weapon to change to when this one is out of ammo: the next owned, non-empty one above, else below down to the
// shotgun, else the colt.
int plrPLAYER::GetNextPriorWeapon()
{
    for (int i = curWeapon + 1; i < PLR_WPN_COUNT; i++) {
        pwpWEAPON *w = weapons[i];
        if (w && (w->flags & WPNF_ENABLED) && !w->IsEmpty())
            return i;
    }
    for (int i = curWeapon - 1; i >= 2; i--) {
        pwpWEAPON *w = weapons[i];
        if (w && (w->flags & WPNF_ENABLED) && !w->IsEmpty())
            return i;
    }
    return 1;
}

// v12 0x00425C30  (feb 0x004225C0)
// The use search sees buttons ('NTTB') and instances without an entity, then the base rules.
BOOL32 plrCDT_REFINE_BUTTON::IsInclInst(animINST *i)
{
    entENTITY *e = i->ent;
    if (e && e->classId != 0x4254544E)
        return 0;
    return cdtREFINE::IsInclInst(i);
}

// v12 0x00425C60
// Living NPCs only (and instances without an entity), then the base rules.
BOOL32 plrCDT_REFINE_NPC::IsInclInst(animINST *i)
{
    entENTITY *e = i->ent;
    if (e && e->classId != 0x244E5043)
        return 0;
    if (aiIsNPCDead(e, 0))
        return 0;
    return cdtREFINE::IsInclInst(i);
}

// v12 0x00425D50  (1.2 only)  inline of the input header, emitted here
int gsINP_SYSTEM::IsPressed(int cmd)
{
    return IsOn(cmd) && IsToggled(cmd) ? 1 : 0;
}

// v12 0x00425E00  (feb 0x0052F190)  inline of damage.h, emitted here
dmgDAMAGE::dmgDAMAGE(int t)
{
    msgId = 0;
    msgFlags = 0x40;
    type = (u8)t;
    subtype = 0;
    size = 0x0E;
}

// v12 0x00425E30  (feb 0x00422780)
// The last floor contact (ProcessPhysics' FloorPlace).
BOOL32 plrPLAYER::GetFloorInfo(cdtINFO *ci)
{
    *ci = floorInfo;
    return 1;
}

// v12 0x00423800  (feb 0x004203A0)
// Does the player stand on this instance (and this object of it, when given): its floor is that instance, it is
// within 0.05 of it (1.5 on an "sob_katapult") and, with notRising, it is not moving up.
BOOL32 plrPLAYER::IsStayOn(animINST *a, objOBJ *o, int notRising)
{
    if (floorInst != a)
        return 0;
    if (o && unk_247 != o->idHier)
        return 0;
    const char *n = a->className ? a->className : "";
    float h = !strcmp(n, "sob_katapult") ? 1.5f : 0.05f;
    if (h < floorHeight)
        return 0;
    if (notRising && vel.y > 1.0e-6f)
        return 0;
    return 1;
}

// v12 0x00425E20  (feb 0x00422710)
float plgsACTION::GetGravity()
{
    return 30.0f;
}

static void *_plrCfgPlrLegs2;                           // v12 0x00641B78 SetupGhostRendState's cfg cache of VIDEO_PlrLegs

// v12 0x0041F110  (feb 0x0041C350)  plrPLAYER slot +0x10C
// Back alive (the debug key 4; full with Shift): the body standing and every part of it shown again, out of the
// crossbow's zoom, the shared part (weapons, gold, power-ups when full), the outfit put on again, the team (TH) or
// none, the movement stopped, the camera told, the draw state set up, "GAME_PLAYER_REVITALIZED" for 3 seconds.
void plrPLAYER::Revitalize(int full)
{
    if (body) {
        body->moveFlags &= ~2u;
        body->inst->root->SetStateProcNo(8, 0);
    }
    ClearZoom();
    plrPLAYER_COMMON::Revitalize(full);
    wrEVCall<void>(this, 0x124, skinNo);                 // SelectSkin
    if (WR_FIELD(int, wrSysGO, 0x98) == 3)
        wrEVCall<void>(this, 0x128, team);
    else
        team = -1;
    ctrlVel = m3dVZero;
    vel = m3dVZero;
    camCtrl->RevitalizeNotify();
    SetupGhostRendState();
    uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId((char *)"GAME_PLAYER_REVITALIZED")), 3.0f, 0);
}

// v12 0x00425890  (feb 0x004222F0)  plrPLAYER slot +0x124
// The outfit (out of range: the first) kept and put on the body; in TH (wrGO mode 3) the team's colours put on
// again (slot +0x128), else no team. The others told (MP 0x40E, type 10: the server to its clients, a client to the
// server), the body's draw state set up again.
void plrPLAYER::SelectSkin(int s)
{
#pragma pack(push, 1)
    struct { msgDATA h; short type; int skin; } m;
#pragma pack(pop)
    m.h.packId = 0;
    m.h.tag = 0x40;
    m.h.size = 0xA;
    m.type = 0xA;
    m.skin = -1;
    if (s <= -1 || s >= 6)
        s = 0;
    skinNo = s;
    if (body)
        body->SelectSkin(s);
    if (WR_FIELD(int, wrSysGO, 0x98) == 3)
        wrEVCall<void>(this, 0x128, team);
    else
        team = -1;
    m.skin = skinNo;
    if (entFlags & 4)
        gsSysServer->SendMsgClientAll(this, 0x40E, &m.h, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
    else
        wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x40E, (void *)&m.h, 0x20000);   // SendMsgServer
    SetupGhostRendState();
    body->SetupGhostWpnState();
}

// v12 0x0041F2E0  (feb 0x0041C4B0)
// What of the body is drawn. Out of a cut-scene the body is kept out of collision (0x10) and its weapon shown. In
// first person (alive, not in the noclip / camera modes 0x500000, plrPLAYER slot +0x78): the player's own weapon and
// the body shown, the body's weapon kept to the mirror / shadow passes (0x100000); with VIDEO_PlrLegs on and the
// view looking down (pitch 150 or more, not swimming / crouch-sliding 0xC00) the body is drawn but every object of
// it with faces is kept out of the main pass (procState 0x200) except the "DOWN_BODY" subtree -- the legs --
// else the whole body goes to the mirror pass (0x100000; without legs also to the shadow 0x2000000). Dead and
// fallen (0x10000): the body hidden, the first-person weapon hidden. Otherwise (third person, dead): the whole body
// drawn, the first-person weapon hidden, the body's too in the 0x40400000 modes; a dead body lies down (unk08 4).
// The body scaled to 0.073 (the template is in centimetres) and its weapon moved to its hand.
void plrPLAYER::SetupGhostRendState()
{
    if (!body)
        return;
    animINST *bi = body->inst;
    animINST *bw = body->weapons[body->curWeapon]->inst;
    if (WR_FIELD(u8, gsSysCinema, 0x80) & 1)
        bi->flags &= ~0x10u;
    else {
        bi->flags |= 0x10;
        body->weapons[body->curWeapon]->inst->flags &= ~1u;
    }
    bi->unk08 &= ~4u;
    bi->unk08 &= ~8u;
    if (!wrEVCall<BOOL32>(this, 0x74) && !(moveFlags & 0x500000) && wrEVCall<BOOL32>(this, 0x78)) {
        int legs = 1;
        _apCfgReadBool(&_plrCfgPlrLegs2, (char *)"Video", (char *)"VIDEO_PlrLegs", &legs);
        bw->flags |= 0x100000;
        if (!(moveFlags & 0x20000000))
            weapons[curWeapon]->inst->flags &= ~1u;
        bi->flags &= ~1u;
        bw->flags &= ~1u;
        if (legs) {
            if (!(pitch < 150.0f) && !(moveFlags & 0xC00)) {
                bi->flags &= ~0x100000u;
                bi->root->SetStateProcYes(0x200, objIsWithFaces);
                objFILTER_HIDDEN_NAME f((char *)"DOWN_BODY");
                if (objOBJ *d = objFind(bi->root, &f))
                    d->SetStateProcNo(0x200, 0);
                bi->flags &= ~0x2000000u;
                bw->flags &= ~0x2000000u;
            } else {
                bi->flags |= 0x100000;
                bi->unk08 |= 8;
            }
        } else {
            bi->flags |= 0x100000;
            bi->flags |= 0x2000000;
            bw->flags |= 0x2000000;
        }
    } else if (wrEVCall<BOOL32>(this, 0x74) && (moveFlags & 0x10000)) {
        bi->flags |= 1;
        bw->flags |= 0x100000;
        weapons[curWeapon]->inst->flags |= 1;
        goto done;
    } else {
        bi->flags &= ~0x100000u;
        bi->root->SetStateProcNo(0x200, objIsWithFaces);
        bi->flags &= ~1u;
        bw->flags &= ~0x100000u;
        weapons[curWeapon]->inst->flags |= 1;
        if (moveFlags & 0x40400000)
            bw->flags |= 1;
        bi->flags &= ~0x2000000u;
        bw->flags &= ~0x2000000u;
        if (wrEVCall<BOOL32>(this, 0x74))
            bi->unk08 |= 4;
    }
    if (body) {
        bodyScale = 0.073f;
        bi->Scale(0.073f, 0.073f, 0.073f, 0);
        body->UpdateWeapon();
    }
done:
    if (!wrEVCall<BOOL32>(this, 0x78))
        body->weapons[body->curWeapon]->inst->flags |= 1;
}

int __fastcall m3dQuantize8(float v, float mn, float mx);          // v12 0x00572440 (m3d_quant.cpp)
BOOL32 __fastcall m3dClampVector_L(m3dV *from, m3dV *to, float maxLen, m3dV *out); // v12 0x00572720 (m3d_lerp.cpp)
static int _plrAnimForceForw, _plrAnimForceBack;      // v12 0x00643290 / 0x00643294 never set in 1.2 (names ours)

// v12 0x0041F080  (name ours)
// The sync block's byte 0x51 follows flags2 0x40 / 0x80 / 0x200 (bits 0x10 / 0x20 / 0x40).
void plrPLAYER::SyncFlags2()
{
    u8 *sy = (u8 *)unk_1E3;
    sy[0x51] = (flags2 & 0x40) ? sy[0x51] | 0x10 : sy[0x51] & ~0x10;
    sy[0x51] = (flags2 & 0x80) ? sy[0x51] | 0x20 : sy[0x51] & ~0x20;
    sy[0x51] = (flags2 & 0x200) ? sy[0x51] | 0x40 : sy[0x51] & ~0x40;
}

// v12 0x0042CA00  (feb 0x00427B30)
// The player's animation and its body. The sequence: idle 1 (wounded 0x6A once asked, turning 0x34 / 0x3A); run 7
// strafing (unk_1F7 2 right / 1 left) or forward (4), back 0x12 (8); swimming 0x46, jumping 0x23, crouched 0x52 still /
// 0x4C moving, the power-up 0x40, dead 0x1D (drowned 0x58). The overlays: the shovel's and the bomb's attacks
// restarted on a shot (sync bits 0 / 1). Played (unless frozen). The body's place: 0.55 behind the eye on the
// view's level direction (at it with 0x400000) -- kept where it was while standing still idle on the floor for a
// second, moving at most 7 m/s on the ground (0x1000000), 1 up swimming / drowning. The sync block (sequence, pitch
// in a byte, the move bits, 0x800 / 0x400; entFlags 0x100 when they changed or an overlay started) to the body,
// which stands there facing the legs' direction and turns its back by the view's angle from it.
void plrPLAYER::Animate()
{
    int seq = 1;
    if (flags2 & 0x20) {
        flags2 &= ~0x20u;
        seq = 0x6A;
    }
    if (moveFlags & 0x2000000)
        seq = 0x34;
    else if (moveFlags & 0x4000000)
        seq = 0x3A;
    u8 *sy = (u8 *)unk_1E3;
    sy[0x50] &= ~3;
    animSYSTEM_INST *as = (animSYSTEM_INST *)inst->link;
    if (GetMountType(-1) == 3 && (moveFlags & 0x40)) {
        ((animCHN *)as->chn)->Start();
        sy[0x50] |= 1;
    }
    if (GetMountType(-1) == 4 && (moveFlags & 0x40)) {
        ((animCHN *)((char *)as->chn + 0x18))->Start();
        sy[0x50] |= 2;
    }
    unk_1F7 &= ~0xFu;
    if (_plrCtrlSide.GetValue() > 0.01f) {
        unk_1F7 |= 2;
        seq = 7;
    } else if (_plrCtrlSide.GetValue() < -0.01f) {
        unk_1F7 |= 1;
        seq = 7;
    }
    if (_plrCtrlForw.GetValue() > 0.01f || _plrAnimForceForw) {
        unk_1F7 |= 4;
        seq = 7;
    } else if (_plrCtrlForw.GetValue() < -0.01f || _plrAnimForceBack) {
        unk_1F7 |= 8;
        seq = 0x12;
    }
    if (moveFlags & 0xC00)
        seq = 0x46;
    if (moveFlags & 0x100)
        seq = 0x23;
    if (moveFlags & 0x200) {
        if (seq == 1)
            seq = 0x52;
        else if (seq == 7 || seq == 0x12)
            seq = 0x4C;
    }
    if (moveFlags & 0x400000)
        seq = 0x40;
    if (wrEVCall<BOOL32>(this, 0x74))                   // IsDead
        seq = (moveFlags & 0x800) ? 0x58 : 0x1D;
    if (entENTITY::flags2 & 0x10)
        return;
    as->AdvanceAnimation(inst, seq, gsElapsedTime, 0);
    m3dV pos, dir;
    inst->GetPos(&pos);
    inst->GetDir(&dir);
    dir.y = 0;
    m3dNormalize(&dir);
    int still = as->groupBase == 2 && !(moveFlags & 0x9000000) && fabsf(m3dVZero.x - vel.x) < 0.01f &&
                fabsf(m3dVZero.y - vel.y) < 0.01f && fabsf(m3dVZero.z - vel.z) < 0.01f &&
                fabsf(_plrCtrlForw.GetValue()) < 0.001f && fabsf(_plrCtrlSide.GetValue()) < 0.001f && !platInst &&
                timer_297 > 1.0f && !(moveFlags & 0x8000) && floorHeight < 0.1f;
    if (!still) {
        if (moveFlags & 0x400000)
            placePos = pos;
        else {
            placePos.x = pos.x - dir.x * 0.55f;
            placePos.y = pos.y - dir.y * 0.55f;
            placePos.z = pos.z - dir.z * 0.55f;
        }
        placePos2 = placePos;
    }
    if (!platInst && !(moveFlags & 0x8000) && (moveFlags & 0x1000000))
        m3dClampVector_L(&placePos2, &placePos, gsElapsedTime * 7.0f, &placePos);
    if (as->groupBase == 0x47 || as->groupBase == 0x59)
        placePos.y += 1.0f;
    placePos2 = placePos;
    sy[0x4F] = (u8)seq;
    sy[0x52] = (u8)m3dQuantize8(pitch, 0.0f, 180.0f);
    int b6 = (sy[0x50] & 0x40) != 0, b7 = (sy[0x50] & 0x80) != 0;
    if (b6 != ((moveFlags & 0x800) != 0))
        entFlags |= 0x100;
    if (b7 != ((moveFlags & 0x400) != 0))
        entFlags |= 0x100;
    sy[0x50] = (sy[0x50] & ~0x40) | (((moveFlags >> 11) & 1) << 6);
    sy[0x50] = (sy[0x50] & 0x7F) | (((moveFlags >> 10) & 1) << 7);
    SyncFlags2();
    sy[0x50] = (sy[0x50] & ~0x3C) | ((unk_1F7 << 2) & 0x3C);
    if (sy[0x50] & 3)
        entFlags |= 0x100;
    if (gsAppState & 0x800)
        return;
    wrEVCall<void>(body, 0x12C, sy);                    // ReadAnimSync
    body->inst->SetPosDir_XZ(&placePos, &bodyDir);
    inst->GetDir(&dir);
    body->bodyTwist = -m3xzAngleVector(&bodyDir, &dir);
}

// v12 0x00425310  (feb 0x00421CA0)  plrPLAYER slot +0xDC
// The shared part; a weapon had already reloads (the one in the hands with a clip not full: a forced reload);
// "GAME_MSG_GOT_<weapon>" for 3 s.
BOOL32 plrPLAYER::PickWeapon(int w, float n)
{
    int had = weapons[w]->wpnWEAPON::flags & 1;
    if (plrPLAYER_COMMON::PickWeapon(w, n) && had) {
        if (curWeapon == w && weapons[curWeapon]->clip != weapons[curWeapon]->clipSize)
            moveFlags |= 0x10000000;
        else
            weapons[w]->Recharge();
    }
    uiSystem->ShowMsgW(gsStrings->Printf("GAME_MSG_GOT_", WpnType2String(w)), 3.0f, 3);
    return 1;
}

// v12 0x004253C0  (feb 0x00421D10)  slot +0xE0
BOOL32 plrPLAYER::PickAmmo(int w, float n)
{
    if (!plrPLAYER_COMMON::PickAmmo(w, n))
        return 0;
    uiSystem->ShowMsgW(gsStrings->Printf("GAME_MSG_GOT_AMMO", WpnType2String(w), (int)n), 3.0f, 0);
    return 1;
}

// v12 0x00425430  (feb 0x00421D90)  slot +0xE4
BOOL32 plrPLAYER::PickHealth(float n)
{
    if (!plrPLAYER_COMMON::PickHealth(n))
        return 0;
    uiSystem->ShowMsgW(gsStrings->Printf("GAME_MSG_GOT_HEALTH", (int)n), 3.0f, 0);
    return 1;
}

// v12 0x00425490  (feb 0x00421E00)  slot +0xE8
BOOL32 plrPLAYER::PickArmor(float n)
{
    if (!plrPLAYER_COMMON::PickArmor(n))
        return 0;
    uiSystem->ShowMsgW(gsStrings->Printf("GAME_MSG_GOT_ARMOR", (int)n), 3.0f, 0);
    return 1;
}

// v12 0x004254F0  (feb 0x00421E70)  slot +0xF8
BOOL32 plrPLAYER::PickTreasure(float n)
{
    if (!plrPLAYER_COMMON::PickTreasure(n))
        return 0;
    uiSystem->ShowMsgW(gsStrings->Printf("GAME_MSG_GOT_TREASURE", (int)n), 3.0f, 1);
    return 1;
}

// v12 0x00425550  (feb 0x00421EE0)  slot +0xF4
BOOL32 plrPLAYER::PickMoney(float n)
{
    if (!plrPLAYER_COMMON::PickMoney(n))
        return 0;
    uiSystem->ShowMsgW(gsStrings->Printf("GAME_MSG_GOT_MONEY", (int)n), 3.0f, 0);
    return 1;
}

// v12 0x004255B0  (feb 0x00421F50)  slot +0xEC
BOOL32 plrPLAYER::PickPowerup(int i)
{
    if (!plrPLAYER_COMMON::PickPowerup(i))
        return 0;
    const char *key = i == 0 ? "GAME_MSG_GOT_QUAD_DAMAGE" : i == 1 ? "GAME_MSG_GOT_UNHOLY_ARMOR" : "GAME_MSG_GOT_POWERUP";
    uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId(key)), 2.0f, 1);
    return 1;
}

// v12 0x00425630  (feb 0x00421FD0)  slot +0xF0
BOOL32 plrPLAYER::PickKey()
{
    if (!plrPLAYER_COMMON::PickKey())
        return 0;
    uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId("GAME_MSG_GOT_KEY")), 3.0f, 1);
    return 1;
}

#pragma pack(push, 1)
struct plrTELEPORT_MSG : msgDATA {                      // GS_MSG_MP_NOTIFY 0x40E type 0 / 15 (0x4A bytes)
    short   type;                                       // +0x04
    m3dMATR mat;                                        // +0x06 where to
    int     flash;                                      // +0x46 0 at once, else behind the screen flash
};
#pragma pack(pop)

// v12 0x0041EAF0  (feb 0x0041BE50)  slot +0x08C
// COMMON's types, then the local player's: 1 the weapon switched (at once, or animated), -97 placed, 4 respawned
// (plrPlayer revived -- a client marks it -- and placed), 2 a weapon enabled, 5 a weapon's ammo and clip, -98 the
// death, 6 the armor, 18 the camera set, 0 teleported (behind the screen flash: as 15 half a flash later, single
// player, or at once) and 15 placed by a matrix, -100 the kept state restored (the dead camera, the level start, the
// breath, the pitch, the powerups that were on, the speed), 8 the reload, 9 the counts, 12 the gold, -95 pushed.
void plrPLAYER::ProcessMP_NOTIFY(void *d)
{
    u8 *m = (u8 *)d;
    plrPLAYER_COMMON::ProcessMP_NOTIFY(d);
    switch (*(short *)(m + 4)) {
    case 1: {
        plrWPN_MSG *w = (plrWPN_MSG *)m;
        if (!w->animate) {
            if (weapons[curWeapon])
                weapons[curWeapon]->inst->flags |= 1;
            curWeapon = w->wpn;
            weapons[curWeapon]->inst->flags &= ~1u;
            return;
        }
        moveFlags |= 0x18;
        nextWeapon = w->wpn;
        ClearZoom();                                                    // (inline)
        return;
    }
    case -97:
        wrEVCall<void>(this, 0x70, m + 6, m + 0x12, 1);                // SetPosDir
        return;
    case 4: {
        plrPLAYER *p = plrPlayer;
        int full = m[6];
        if (p->entFlags & 2) {
            p->flags2 |= 0x40;
            if (full)
                p->flags2 |= 0x80;
            p->entFlags |= 0x200;
        }
        wrEVCall<void>(p, 0x10C, full);                                 // Revitalize
        wrEVCall<void>(plrPlayer, 0x70, m + 8, m + 0x14, m[7] == 0);    // SetPosDir
        timer_297 = 0;
        return;
    }
    case 2:
        wrEVCall<void>(weapons[(signed char)m[6]], 0xA8, 1);           // Enable
        return;
    case 5: {
        pwpWEAPON *w = weapons[(signed char)m[6]];
        w->SetAmmo((float)*(short *)(m + 7));
        if (*(short *)(m + 9) != -2)
            weapons[(signed char)m[6]]->clip = *(short *)(m + 9);
        return;
    }
    case -98:
        wrEVCall<void>(this, 0x114, m);                                 // StartDeath
        return;
    case 6:
        armor = *(float *)(m + 6);
        return;
    case 18:
        gsCameraPtr->SetParameters((m3dV *)(m + 6), (m3dV *)(m + 0x1E), (m3dV *)(m + 0x12), (m3dV *)(m + 0x2A));
        return;
    case 0:
        if (!(flashTimer < 0))
            return;
        if (((plrTELEPORT_MSG *)m)->flash) {
            plrTELEPORT_MSG t;
            t.packId = 0;
            t.tag = 0x40;
            t.flash = 0;
            t.size = 0x4A;
            t.type = 0xF;
            t.mat = ((plrTELEPORT_MSG *)m)->mat;
            flashTimer = 0;
            if (WR_FIELD(int, gsSysMP, 0x28) == 1)
                msgSystem.PostTimeMsgData(this, 0x40E, &t, this, flashPeriod * 0.5f);
            else
                msgSystem.SendMsg(this, 0x40E, &t, this);
            return;
        }
        // fall through
    case 15: {
        inst->Transform(&((plrTELEPORT_MSG *)m)->mat, 0);
        m3dV dir;
        inst->GetDir(&dir);
        pitch = m3dAngleVector(&dir, &m3dVUnitY);
        fallDist = 0;
        return;
    }
    case -100: {
        if (wrEVCall<BOOL32>(this, 0x74))                               // IsDead
            camCtrl->DeathNotify(0);
        moveFlags |= PLR_MF_LEVEL_START;
        u8 *s = *(u8 **)(m + 6);
        breathTime = *(float *)(s + 0x590);
        m3dV dir;
        inst->GetDir(&dir);
        pitch = m3dAngleVector(&dir, &m3dVUnitY);
        for (int i = 0; i < PLR_PWR_COUNT; i++)
            if (powerup[i].state == PLR_PWR_ACTIVE)
                ActivatePowerup(i);
        wrEVCall<void>(this, 0xA4, s + 0x55C, 1);                      // ApplySpeed
        ClearZoom();                                                    // (inline)
        return;
    }
    case 8:
        weapons[curWeapon]->Recharge();
        return;
    case 9:
        frags = *(int *)(m + 6);
        unk_217 = *(int *)(m + 0xA);
        treasures = *(int *)(m + 0xE);
        return;
    case 12:
        money = *(float *)(m + 6);
        return;
    case -95:
        wrEVCall<void>(this, 0xA4, m + 6, 0);                          // ApplySpeed
        return;
    }
}

#pragma pack(push, 1)
struct plrRESPAWN_MSG : msgDATA {                       // GS_MSG_MP_NOTIFY 0x40E type 3: a respawn asked for (8 bytes)
    short type;                                         // +0x04 3
    u8    full;                                         // +0x06 Revitalize's argument
    u8    keepPos;                                      // +0x07 where the player is, not a start position
};
#pragma pack(pop)

// v12 0x0041F270  (feb 0x0041C450)
// A respawn asked of the server (wrGAME's USE key while dead, in multiplayer); in co-op with Shift held: on the spot.
void plrPLAYER::RevitalizeStart(int full)
{
    plrRESPAWN_MSG m;
    m.packId = 0;
    m.keepPos = 0;
    m.tag = 0x40;
    m.type = 3;
    m.size = 8;
    m.full = (u8)full;
    if (wrSysGO->mode == 2 && GetAsyncKeyState(VK_SHIFT) < 0)
        m.keepPos = 1;
    wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x40E, (void *)&m, 0x20000);
}

// v12 0x0041F0E0  (feb 0x0041C330)  slot +0x104
// COMMON's start values, the first-person view back, the body alive.
void plrPLAYER::SetDefaultValues()
{
    plrPLAYER_COMMON::SetDefaultValues();
    camCtrl->RevitalizeNotify();
    if (body)
        body->moveFlags &= ~2u;
}

// v12 0x004249C0  (feb 0x004213A0)  slot +0x0FC
// The trading altar's cut-scene. 0 in (data: powerup, altar pos and dir, player pos and dir): the controls off, placed
// where the altar's button says, the altar's camera at the altar playing, the view on its camera for the powerup, no
// weapon in the body's hands, Will's line. 1 out (data: powerup): the controls back, the first-person view, the
// powerup picked (the gold paid), the pitch from the direction, a line.
void plrPLAYER::ProcessPowerup(int type, void *data)
{
    int *d = (int *)data;
    if (type == 0) {
        moveFlags |= 0x400000;
        if (body)
            body->moveFlags |= 0x400000;
        m3dV *altarPos = (m3dV *)(d + 1), *altarDir = (m3dV *)(d + 4);
        m3dV *pos = (m3dV *)(d + 7), *dir = (m3dV *)(d + 10);
        inst->SetPos(pos);
        inst->SetDir_XZ(dir);
        bodyDir = *dir;
        placePos2 = *pos;
        placePos = *pos;
        tradeCamera->inst->SetPosDir_XZ(altarPos, altarDir);
        tradeCamera->inst->Validate(4);
        ((dynDYNAMIC *)tradeCamera)->StartAnim(0, 0, 0);
        camCtrl->PowerupNotify(1, (int)(intptr_t)d, ((wrsobTRA_CAMERA *)tradeCamera)->cam[*d]);
        if (body)
            wrEVCall<void>(body, 0x108, -1, 0);         // SetCurWpn: none
        if (WR_FIELD(int, gsSysMP, 0x28) == 1) {
            if (*d == 0)
                wrMSG_SYSTEM::ShowMsg((char *)"LIN_P_titan_damage", 1.5f);
            else if (*d == 1)
                wrMSG_SYSTEM::ShowMsg((char *)"LIN_P_immortality", 1.5f);
            else if (*d == 2)
                wrMSG_SYSTEM::ShowMsg((char *)"LIN_P_titan_motion", 1.5f);
        }
    } else if (type == 1) {
        int pw = *d;
        moveFlags &= ~0x400000u;
        if (body)
            body->moveFlags &= ~0x400000u;
        camCtrl->PowerupNotify(0, 0, 0);
        SetupGhostRendState();
        wrEVCall<BOOL32>(this, 0xEC, pw);               // PickPowerup
        m3dV dir;
        inst->GetDir(&dir);
        pitch = m3dAngleVector(&dir, &m3dVUnitY);
        if (WR_FIELD(int, gsSysMP, 0x28) == 1)
            wrMSG_SYSTEM::ShowMsg(m3dRandom() > 0.4 ? (char *)"LIN_P_use_power" : (char *)"LIN_P_power_lead_victory", 1.5f);
    }
}

// v12 0x004247D0  (feb 0x004211E0)  slot +0x114
// Once: COMMON's part; the body dies too; the powerups that are on end (sent as off; slow motion: the normal time
// scale); out of the crossbow's zoom; the death camera (falling unless the game holds the camera, gsAppState 0x1000);
// the body's look; a body killed by weapon 4, 5, 7 or 10 lets go of its weapon; the control velocity stopped; in
// single player Will's last words ("LIN_P_fool" three times in ten, else "LIN_P_failed_mortal"); the weapon's sound
// stopped.
void plrPLAYER::StartDeath(plrEV_START_DEATH *m)
{
    if (moveFlags & 2)
        return;
    plrPLAYER_COMMON::StartDeath(m);
    if (body)
        body->moveFlags |= 2;
    for (int i = 0; i < PLR_PWR_COUNT; i++) {
        if (powerup[i].state != PLR_PWR_ACTIVE)
            continue;
        plrPOWERUP_MSG pm;
        pm.packId = 0;
        pm.tag = 0x40;
        pm.id = 0;
        pm.on = 0;
        pm.type = 7;
        pm.size = 0x0E;
        if (!IsActivePowerup(i))
            continue;
        powerup[i].state = PLR_PWR_NONE;
        powerup[i].timeLeft = 0;
        if (i == PLR_PWR_TIMESCALE)
            gsMsgSetElapsedTimeScale(1.0f);
        unk108 |= 0x10000000;
        pm.id = i;
        pm.on = 0;
        wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x40E, (void *)&pm, 0x20000);
    }
    ClearZoom();                                                        // (inline)
    camCtrl->DeathNotify(!(gsAppState & 0x1000));
    SetupGhostRendState();
    if (body && (m->body == 4 || m->body == 5 || m->body == 7 || m->body == 10))
        wrEVCall<void>(body, 0x108, -1, 0);                             // SetCurWpn: none
    ctrlVel = m3dVZero;
    if (WR_FIELD(int, gsSysMP, 0x28) == 1)
        wrMSG_SYSTEM::ShowMsg(m3dRandom() > 0.3f ? (char *)"LIN_P_failed_mortal" : (char *)"LIN_P_fool", 1.5f);
    if (weapons[curWeapon])
        wrEVCall<void>(weapons[curWeapon], 0xCC);                       // StopSound
}
