#include <stdarg.h>
// box_level.cpp -- test level for the reconstructed engine: a box to stand on, a floor, steps, ramps and a wall,
// drawn in flat colours, with the player moved by the rebuilt 1.2 player code (ControlFirstPerson +
// ProcessPhysics + the scene collision). No weapons, sounds, HUD, entities or scripts.
//
// What is the game's code here: the scene (scnSCENE ctor / InitLevel / AllocInst), objects (objOBJ, Obj_alloc,
// Obj_misc, normals, boxes), the level collision grid (cdtCOLL_SCN::CalcData2) and the instance grid
// (cdtCOLL_ANIM), all scene queries (FloorPlace, ConstrSphere, IsVisiblePoint ...), the player physics,
// mouse look, move smoothing, eye height and m3d. What is the test's: building the meshes (the level loader,
// sceneio, is not rebuilt yet), the player setup in place of entity creation (ProcessINIT values), the input
// table in place of gsINP_SYSTEM, and the Direct3D 7 drawing.
//
// Controls: mouse look, W/S/A/D move, Space jump, Ctrl or C crouch, Shift walk (always run, the game's
// default), arrow keys look, [ ] mouse sensitivity, Esc quit. The game's bin\\willrock.ini settings: --game <install dir>.
// --selftest: scripted run with a fixed time step; prints the player state and writes screenshots (PPM).

#include "../src/engine/engine.h"
#include "../include/weapons/weapons.h"
#include "../include/engine_render/particle.h"
#include "../include/ai/ai_npc.h"
#include "../include/ai/ai_world.h"
#include "../include/frontend/wr_game.h"
#include "../include/engine_render/shader.h"
#include "tst_pte.h"
#include "../src/wr_vcall.h"
#include "../src/gs/gs_mp_misc.h"
#include "../src/gs/gs_mp_server.h"
#include "../include/frontend/wr_msgs.h"
#include "../src/m3d/m3d_spl.h"
u32 __fastcall gsEffEnvirColor(cdtINFO *ci, m3dCOLOR *col, m3dCOLOR *dyn);
#include "plat_d3d7.h"
#include "test_assets.h"
#include "test_menu.h"
#include "test_ents.h"
#include "../include/frontend/wr_ui.h"
#include <dinput.h>
#include <math.h>
static void tstGameLogLevel(const char *path, int nObj);   // (test) the gameplay log (see tstGameLog)
static void tstGameLogPlayer(plrPLAYER *p);

// PLACEHOLDER: leaving the menu for a level stops the menu music at once (the game: the level's own music takes the
// track -- the level music is not rebuilt); the main window starts track 0 again when it appears.
static void menuMusicOff()
{
    if (uiSystem)
        ((wrUI_SYSTEM *)uiSystem)->PlayAmbientMusic(-2);
}

// (test) full screen (VIDEO_FullScr and F11) on. (The dark textures once blamed on it were an editor-baked
// lightmap found before textures\lm; see willrock-placeholders.)
static const int s_allowFullScreen = 1;
static int s_scrW = 1024, s_scrH = 768;            // window size; full screen: [Video] VIDEO_SizeX / SizeY
#define SCR_W s_scrW
#define SCR_H s_scrH

// ---------------------------------------------------------------------------------------------
// meshes
// ---------------------------------------------------------------------------------------------
struct tstOBJ { objOBJ *o; unsigned color; };
static tstOBJ s_objs[64];
static int    s_nObj;

static void addTri(objOBJ *o, int f, int a, int b, int c, const m3dV *want)
{
    m3dV *v = o->geom->vert, n;
    m3dGetPlanePerpVector(&v[a], &v[b], &v[c], &n);     // swap to make (b-a)x(c-b) face outwards
    if (n.x * want->x + n.y * want->y + n.z * want->z < 0) {
        int t = b; b = c; c = t;
    }
    u16 *rec = (u16 *)((u8 *)o->geom->faceList + o->geom->faceSize * f);
    rec[0] = 0;
    rec[1] = (u16)a;
    rec[2] = (u16)b;
    rec[3] = (u16)c;
}

// Convex mesh from vertices + quads/tris (outward normal from the centroid).
static objOBJ *makeMesh(const char *name, const m3dV *vs, int nv, const int (*faces)[4], int nf, unsigned color)
{
    objOBJ *o = objOBJ::Make();
    o->SetName((char *)name);
    o->_AllocGeom();
    o->_AllocVertList(nv);
    for (int i = 0; i < nv; i++)
        o->geom->vert[i] = vs[i];
    int nt = 0;
    for (int f = 0; f < nf; f++)
        nt += faces[f][3] >= 0 ? 2 : 1;
    o->_AllocFaceList(nt, 0);
    m3dV c = { 0, 0, 0 };
    for (int i = 0; i < nv; i++) {
        c.x += vs[i].x / nv;
        c.y += vs[i].y / nv;
        c.z += vs[i].z / nv;
    }
    int t = 0;
    for (int f = 0; f < nf; f++) {
        const int *q = faces[f];
        m3dV fc = { (vs[q[0]].x + vs[q[1]].x + vs[q[2]].x) / 3, (vs[q[0]].y + vs[q[1]].y + vs[q[2]].y) / 3,
                    (vs[q[0]].z + vs[q[1]].z + vs[q[2]].z) / 3 };
        m3dV out = { fc.x - c.x, fc.y - c.y, fc.z - c.z };
        addTri(o, t++, q[0], q[1], q[2], &out);
        if (q[3] >= 0)
            addTri(o, t++, q[0], q[2], q[3], &out);
    }
    s_objs[s_nObj].o = o;
    s_objs[s_nObj].color = color;
    s_nObj++;
    return o;
}

static objOBJ *makeBox(const char *name, float x0, float y0, float z0, float x1, float y1, float z1, unsigned color)
{
    m3dV v[8] = { { x0, y0, z0 }, { x1, y0, z0 }, { x1, y0, z1 }, { x0, y0, z1 },
                  { x0, y1, z0 }, { x1, y1, z0 }, { x1, y1, z1 }, { x0, y1, z1 } };
    static const int q[6][4] = { { 0, 1, 2, 3 }, { 4, 5, 6, 7 }, { 0, 1, 5, 4 },
                                 { 1, 2, 6, 5 }, { 2, 3, 7, 6 }, { 3, 0, 4, 7 } };
    return makeMesh(name, v, 8, q, 6, color);
}

// Ramp rising along +x from (x0, y0) to (x1, y1) over z0..z1.
static objOBJ *makeRamp(const char *name, float x0, float x1, float y0, float y1, float z0, float z1, unsigned color)
{
    m3dV v[6] = { { x0, y0, z0 }, { x1, y0, z0 }, { x1, y1, z0 }, { x0, y0, z1 }, { x1, y0, z1 }, { x1, y1, z1 } };
    static const int q[5][4] = { { 0, 1, 4, 3 }, { 0, 2, 5, 3 }, { 1, 2, 5, 4 }, { 0, 1, 2, -1 }, { 3, 4, 5, -1 } };
    return makeMesh(name, v, 6, q, 5, color);
}

// ---------------------------------------------------------------------------------------------
// player (in place of entCreate("player") + plrPLAYER::ProcessINIT)
// ---------------------------------------------------------------------------------------------
static void *s_plrVtbl[80];

static void __thiscall vtGetPos(plrPLAYER *p, m3dV *o)          { p->GetPos(o); }
static void __thiscall vtGetCenter(plrPLAYER *p, m3dV *o)       { if (p->inst) p->inst->GetCenter(o); else *o = m3dVZero; }  // (entENTITY::GetCenter v12 0x00507D00)
static void __thiscall vtSetPos(plrPLAYER *p, m3dV *v)          { p->SetPos(v); }
static void __thiscall vtGetDir(plrPLAYER *p, m3dV *o)          { p->GetDir(o); }
static BOOL32 __thiscall vtIsDead(plrPLAYER *p)                 { return p->IsDead(); }
static void __thiscall vtApplySpeed(plrPLAYER *p, m3dV *v, int s) { p->ApplySpeed(v, s); }
static BOOL32 __thiscall vtIsStayOn(plrPLAYER *p, animINST *a, objOBJ *o, int r) { return p->IsStayOn(a, o, r); }
static BOOL32 __thiscall vtGetFloorInfo(plrPLAYER *p, cdtINFO *ci) { return p->plrPLAYER::GetFloorInfo(ci); }
static void __thiscall vtSetPosDir(plrPLAYER *p, m3dV *pos, m3dV *dir, int f) { p->SetPosDir(pos, dir, f); }
static float __thiscall vtGetGravity(plrPLAYER *p)               { return p->GetGravity(); }
static BOOL32 __thiscall vtProcessMsg(plrPLAYER *p, int id, void *d, msgADDR *f, int *r) { return p->plrPLAYER::ProcessMsg(id, d, f, r); }
// (test) the player is in the address list, as the game's (the MP client finds it there by its id), but its life
// cycle is the test's own -- made in makePlayer, its frame run by playerFrame -- so those broadcasts are not taken
static BOOL32 __thiscall vtPreProcessMsg(plrPLAYER *p, int id, void *d, msgADDR *f, int *r)
{
    switch (id) {
    case 1: case 2: case 1000:                          // INIT, TERM, FRAME
    case 0x3EF: case 0x3F0: case 0x3F1: case 0x3F2: case 0x3F3: case 0x3F4: case 0x3F5: case 0x3F6:  // game / map / level
        return 1;
    }
    return p->entENTITY::PreProcessMsg(id, d, f, r);
}
static BOOL32 __thiscall vtPostProcessMsg(plrPLAYER *p, int id, void *d, msgADDR *f, int *r)
{ return p->entENTITY::PostProcessMsg(id, d, f, r); }
static void __thiscall vtProcessSHOOT(plrPLAYER *p, void *m)       { p->ProcessSHOOT(m); }
static void __thiscall vtProcessSHOOT_BODY(plrPLAYER *p, void *m)  { p->ProcessSHOOT_BODY(m); }
static void __thiscall vtUseKey(plrPLAYER *p)                      { p->UseKey(); }
static void __thiscall vtSetCurWpn(plrPLAYER *p, int w, int a)     { p->SetCurWpn(w, a); }
static void __thiscall vtSelectSkin(plrPLAYER *p, int sk)          { p->SelectSkin(sk); }
static void __thiscall vtUpdateSound(plrPLAYER *p)                 { p->UpdateSound(); }
static BOOL32 __thiscall vtReturn1(plrPLAYER *)                     { return 1; }   // (v12 0x004031F0)
static void __thiscall vtRevitalize(plrPLAYER *p, int full)        { p->Revitalize(full); }
static void __thiscall vtProcessMP_NOTIFY(plrPLAYER *p, void *d)     { p->plrPLAYER::ProcessMP_NOTIFY(d); }
static void __thiscall vtStartDeath(plrPLAYER *p, plrEV_START_DEATH *m)
{
    p->plrPLAYER::StartDeath(m);
    if (getenv("WR_DEATHLOG"))                          // (test) what the death did
        printf("death: source %d sub %d body %d world3 %d -> moveFlags 0x%X unk108 0x%X%c", m->source, m->sub,
               m->body, m->world3, p->moveFlags, p->unk108, 10);
}
static void __thiscall vtUpdateKillsCount(plrPLAYER *p, int n)       { p->UpdateKillsCount(n); }
static void __thiscall vtProcessPowerup(plrPLAYER *p, int t, void *d) { p->ProcessPowerup(t, d); }
static BOOL32 __thiscall vtReturn0(plrPLAYER *)                      { return 0; }
static void __thiscall vtSetDefaultValues(plrPLAYER *p)              { p->plrPLAYER::SetDefaultValues(); }
static void __thiscall vtProcessFRAME(plrPLAYER *p)                { p->ProcessFRAME(); }
static BOOL32 __thiscall vtPick(plrPLAYER *p, entENTITY *item)    { return p->Pick(item); }
static BOOL32 __thiscall vtProcessPICK(plrPLAYER *p, void *d)
{
    if (getenv("WR_PICKLOG"))                           // (test) what is taken
        printf("pick: kind 0x%X amount %.0f health %.0f armor %.0f money %.0f%c", *(int *)((u8 *)d + 4),
               *(float *)((u8 *)d + 8), p->health, p->armor, p->money, 10);
    BOOL32 r = p->ProcessPICK(d);
    if (getenv("WR_PICKLOG"))
        printf("pick: taken %d -> health %.0f armor %.0f money %.0f%c", r, p->health, p->armor, p->money, 10);
    return r;
}
static BOOL32 __thiscall vtPickWeapon(plrPLAYER *p, int w, float n)  { return p->PickWeapon(w, n); }
static BOOL32 __thiscall vtPickAmmo(plrPLAYER *p, int w, float n)    { return p->PickAmmo(w, n); }
static BOOL32 __thiscall vtPickHealth(plrPLAYER *p, float n)         { return p->PickHealth(n); }
static BOOL32 __thiscall vtPickArmor(plrPLAYER *p, float n)          { return p->PickArmor(n); }
static BOOL32 __thiscall vtPickPowerup(plrPLAYER *p, int i)          { return p->PickPowerup(i); }
static BOOL32 __thiscall vtPickKey(plrPLAYER *p)                     { return p->PickKey(); }
static BOOL32 __thiscall vtPickMoney(plrPLAYER *p, float n)          { return p->PickMoney(n); }
static BOOL32 __thiscall vtPickTreasure(plrPLAYER *p, float n)       { return p->PickTreasure(n); }
static void __thiscall vtProcessCDT_FIREABLE(plrPLAYER *p, dmgDAMAGE *d) { p->plrPLAYER::ProcessCDT_FIREABLE(d); }
static void __thiscall vtNotifyHealthChange(plrPLAYER *p, dmgDAMAGE *d)  { p->plrPLAYER::NotifyHealthChange(d); }
static BOOL32 __thiscall vtIsActive(plrPLAYER *p)  { return p->plgsACTION::IsActive(); }
static BOOL32 __thiscall vtSlot90(plrPLAYER *p)    { return p->flags2 & 8; }             // (v12 0x00425E60)
template <int N> static void __thiscall vtTrap(plrPLAYER *p)
{
    fprintf(stderr, "player virtual slot +0x%X not set up in the test\n", N * 4 - WR_MSGADDR_VTBL_SHIFT);
    ExitProcess(3);
}
template <int N> struct vtTraps {
    static void Fill(void **t) { vtTraps<N - 1>::Fill(t); t[N - 1] = (void *)vtTrap<N - 1>; }
};
template <> struct vtTraps<0> { static void Fill(void **) {} };

static gsiCOMMAND    s_cmds[64];
static gsINP_ACTIONS s_actions = { 0, s_cmds, 0, 64 };
static u8            s_inpBuf[0x100];
// PLACEHOLDER (test) for the input contexts (gsINP_SYSTEM::AddCtxes / wrINP_CTX_*, not rebuilt; the test drives
// one command table itself): the "GAME" one above and the cinema's "CIN" one -- its command 0 skips (here Space,
// Enter or the left button) -- with GetCtxByName (+0x1C), SetDefCtx (+0x20) and SetCurCtx over the bare buffer
static gsiCOMMAND    s_cinCmds[1];
static gsINP_ACTIONS s_cinActions = { 0, s_cinCmds, 0, 1 };
static void *s_inpVtbl[16];
#define INP_VT(off) s_inpVtbl[((off) + WR_MSGADDR_VTBL_SHIFT) / 4]
static gsiCONTEXT *__thiscall inpGetCtxByName(gsINP_SYSTEM *, const char *n)
{
    return !stricmp(n, "CIN") ? (gsiCONTEXT *)&s_cinActions : (gsiCONTEXT *)&s_actions;
}
static void __thiscall inpSetDefCtx(gsINP_SYSTEM *in) { in->actions = &s_actions; }
void gsINP_SYSTEM::SetCurCtx(gsiCONTEXT *c) { actions = (gsINP_ACTIONS *)c; }
// PLACEHOLDER (test) for gsRENDER_SYSTEM (wrRENDER_SYSTEM 0x3C, its Init / frame not rebuilt): a zeroed block --
// SetColorFullScr (rebuilt) keeps the full-screen colour there and the test draws it (RenderColorFullScr)
static u8 s_rendBuf[0x40];
// the frame's time; one set by gsMsgSetNextElapsedTime (a cinema skipped) is taken once, as _gsMsgCalcElapsedTime
// (v12 0x00510B5F, not rebuilt) does
// as _gsMsgCalcElapsedTime (v12 0x005109A0): the real time times the game's time scale (slow motion 0.25), at most
// 0.1 s; then the next-frame override
static float tstFrameTime(float dt)
{
    dt *= gsTimeScale;
    if (dt > 0.1f)
        dt = 0.1f;
    if (gsNextElapsedTime > 0.0f) {
        dt = gsNextElapsedTime;
        gsNextElapsedTime = -1.0f;
    }
    return dt;
}
// the fake vtable has the layout of a msgADDR class in this build (wr_vcall.h): slot 'off' of the original at
// off + WR_MSGADDR_VTBL_SHIFT
#define PLR_VT(off) s_plrVtbl[((off) + WR_MSGADDR_VTBL_SHIFT) / 4]

// The player in the address list, as entCreate's entRegister puts the game's (msgADDR magic 'MADR', entENTITY sign
// '$TNE'), the one before it taken out (the test keeps the old players: a level change makes a new one)
static void tstPlayerRegister(plrPLAYER *p)
{
    static plrPLAYER *reg;
    if (reg == p)
        return;
    if (reg)
        msgSystem.RemoveAddr(reg);
    *(u32 *)((char *)p + 0x10) = 0x4D414452;
    p->sign = 0x454E5424;
    msgSystem.InsertAddr(p, 0, 0x46);
    reg = p;
}

static plrPLAYER *makePlayer(scnSCENE *scn, m3dV *at)
{
    vtTraps<80>::Fill(s_plrVtbl);
    PLR_VT(0x04) = (void *)vtProcessMsg;
    PLR_VT(0x08) = (void *)vtPreProcessMsg;
    PLR_VT(0x0C) = (void *)vtPostProcessMsg;
    PLR_VT(0x54) = (void *)vtGetCenter;
    PLR_VT(0x64) = (void *)vtGetPos;
    PLR_VT(0x68) = (void *)vtSetPos;
    PLR_VT(0x6C) = (void *)vtGetDir;
    PLR_VT(0x74) = (void *)vtIsDead;
    PLR_VT(0x78) = (void *)vtIsActive;
    PLR_VT(0x90) = (void *)vtSlot90;
    PLR_VT(0x84) = (void *)vtProcessFRAME;
    PLR_VT(0x98) = (void *)vtUseKey;
    PLR_VT(0xA4) = (void *)vtApplySpeed;
    PLR_VT(0xA0) = (void *)vtIsStayOn;
    PLR_VT(0x44) = (void *)vtGetFloorInfo;
    PLR_VT(0x70) = (void *)vtSetPosDir;
    PLR_VT(0xA8) = (void *)vtGetGravity;
    PLR_VT(0xCC) = (void *)vtProcessSHOOT;
    PLR_VT(0xD0) = (void *)vtProcessSHOOT_BODY;
    PLR_VT(0x108) = (void *)vtSetCurWpn;
    PLR_VT(0x124) = (void *)vtSelectSkin;
    PLR_VT(0x120) = (void *)vtUpdateSound;
    PLR_VT(0x88) = (void *)vtReturn1;                   // (a target: plgsACTION::ProcessFRAME)
    PLR_VT(0x10C) = (void *)vtRevitalize;
    PLR_VT(0xC8) = (void *)vtProcessCDT_FIREABLE;
    PLR_VT(0x9C) = (void *)vtPick;
    PLR_VT(0xC4) = (void *)vtProcessPICK;
    PLR_VT(0xDC) = (void *)vtPickWeapon;
    PLR_VT(0xE0) = (void *)vtPickAmmo;
    PLR_VT(0xE4) = (void *)vtPickHealth;
    PLR_VT(0xE8) = (void *)vtPickArmor;
    PLR_VT(0xEC) = (void *)vtPickPowerup;
    PLR_VT(0xF0) = (void *)vtPickKey;
    PLR_VT(0xF4) = (void *)vtPickMoney;
    PLR_VT(0xF8) = (void *)vtPickTreasure;
    PLR_VT(0x110) = (void *)vtNotifyHealthChange;
    PLR_VT(0x8C) = (void *)vtProcessMP_NOTIFY;
    PLR_VT(0x114) = (void *)vtStartDeath;
    PLR_VT(0x118) = (void *)vtUpdateKillsCount;
    PLR_VT(0xFC) = (void *)vtProcessPowerup;
    PLR_VT(0x11C) = (void *)vtReturn0;                  // first person
    PLR_VT(0x104) = (void *)vtSetDefaultValues;

    // the player's instance: database\player.tpl as entRegister makes it (the test keeps its own vtable, so not
    // through entCreate("player")); its animation system is plrPLAYER_COMMON::SetupAniSystem's (rebuilt), played by
    // plrPLAYER::Animate. Without the template (the box level) a bare one, no animation.
    animINST *i = 0;
    if (!gsSysLevel && !getenv("WR_NOBODY"))
        tstLevelSystemInit(scn);
    if (gsSysLevel)                                     // (the level system works on the level now loading: after a
        tstLevelSystemSetScene(scn);                    //  level change the old scene is gone)
    if (gsSysLevel && !getenv("WR_NOBODY")) {
        animTPL *t = wrEVCall<animTPL *>(gsSysLevel, 0x24, (char *)"player");   // FindTplName
        animCREATE_DATA cd;
        if (t)
            i = scn->CreateInst(t, &cd, -1);
    }
    int real = i != 0;
    if (!i) {
        i = scn->AllocInst();
        i->root = objOBJ::Make();
        i->root->SetName((char *)"player");
        i->root->inst = i;
        i->nObj = 1;
        i->objTable = (objOBJ **)apCalloc(1, 4);
        i->objTable[0] = i->root;
        i->link = (animINST::animLINK *)apCalloc(1, 0x40); // (no animation system: state 0, no body turn lag)
    }
    scn->SetStateYesInst(i, 0x10);                      // the player's own instance never collides with itself
    i->SetName((char *)"player");
    i->SetPos(at);

    size_t sz = sizeof(plrPLAYER) > 0x357 ? sizeof(plrPLAYER) : 0x357;
    plrPLAYER *p = (plrPLAYER *)apCalloc(1, (int)sz + 16);
    *(void **)p = s_plrVtbl;
    p->classId = 0x504C5952;                            // 'PLYR'
    p->inst = i;
    i->ent = p;
    if (real) {                                         // entRegister's part (v12 0x00507FF0)
        if (!i->tpl->aniSystem)
            p->plrPLAYER_COMMON::SetupAniSystem();
        i->AbvCalcBVolEnclos(4, 2, -1.0f);
        i->CreateAnimSys();
        objInitLOD(i->root);
        i->LODInit();
    }
    // plrPLAYER::ProcessINIT (v12 0x0041D62C..0x0041D64C) and INIT_LEVEL values
    p->eyeHeight = 1.6f;
    p->stepHeight = 0.9f;
    p->gravity = 30.0f;
    p->radius = 0.8f;
    p->moveFlags &= ~1u;
    p->heightCtrl = new plrHEIGHT_CTRL;
    p->camCtrl = new plrCAM_CTRL;                       // ProcessINIT v12 0x0041D6DB (new 0x4C)
    p->unk_1E3 = (int)apCalloc(1, 0x53);                // the sync block (plrPLAYER_COMMON::ProcessINIT v12 0x00427466)
    // the three voices, as plrPLAYER_COMMON::ProcessINIT makes them (v12 0x00427199): cone channels heard to 30, not
    // positional for the first-person player (slot +0x11C: 0), in the sound system's list
    for (int k = 0; k < 3; k++) {
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        c->maxDist = 30.0f;
        c->flags |= 1;
        p->sndChannel[k] = c;
        if (gsSysSound)
            gsSysSound->AddChannel(c, p, 0);
    }
    p->unk_2A3 = 0.2f;                                  // ProcessINIT_LEVEL v12 0x0041D943: the aim ray every 0.2 s
    p->flashTimer = -1.0f;                              //   v12 0x0041D94D: no screen flash
    p->entFlags |= 4;                                   // on the server (single player: the local server makes the player)
    p->unk_263 = PLR_WPN_COUNT;                         // PLACEHOLDER for plrPLAYER::ProcessINIT (v12 0x0041D889, not
                                                        // rebuilt): no weapon asked for during a switch
    if (!p->InitSFX())                                  // the chrome weapons and the blood on the hands (plr_sfx.cpp)
        printf("player: InitSFX failed (plr_wpn_* / plr_blood_hands textures)\n");
    p->flashPeriod = 0.8f;                              //   v12 0x0041D962
    p->pitch = 90.0f;
    p->flags2 |= 8;                                     // controls enabled
    p->cfg.bits = (p->cfg.bits & ~6u) | 1;              // plrPLAYER_COMMON_CFG ctor (v12 0x004265C0): always run
    p->health = PLR_HEALTH_DEFAULT;                     // plrPLAYER::SetDefaultValues v12 0x004277B9
    // (test) the weapon slots: until the pwp_* weapons are rebuilt the shovel is a bare pwpWEAPON block with only
    // the fields the HUD reads (index 0, no clip); the real one is pwpSHOVEL::Create v12 0x00416E70
    p->weapons = (pwpWEAPON **)apCalloc(4, 11);
    pwpWEAPON *svl = (pwpWEAPON *)apCalloc(1, 0x200);
    svl->wpnIndex = PWP_SHOVEL;
    svl->clip = -1;
    p->weapons[0] = svl;
    p->curWeapon = 0;
    plrPlayer = p;
    plgsPlayer = p;                                     // as plrPLAYER::Create
    tstPlayerRegister(p);
    return p;
}

// ---------------------------------------------------------------------------------------------
// the game's settings (bin\willrock.ini): what gsINP_SYSTEM::ApplyMouseSettings (v12 0x0052C5E0) and the
// player's options read. Without an ini the engine defaults apply: inpINPUT mouse sensitivity 2.005
// (ctor, v12 0x0058E705), no invert, always run (the player config ctor).
// ---------------------------------------------------------------------------------------------
static float s_mouseSens = 2.005f;
static int   s_invertMouse = 0;
static int   s_alwaysRun = 1;                           // the player config's default
static char  s_iniPath[MAX_PATH];

static int iniBool(const char *sect, const char *key, int def)
{
    char b[32];
    GetPrivateProfileStringA(sect, key, def ? "Yes" : "No", b, sizeof(b), s_iniPath);
    return !strnicmp(b, "Yes", 3);                      // _apCfgReadBool: true only for "Yes..."
}

static void loadIniValues();

// --game <dir> (the install folder), else the WR_GAME_DIR environment variable, else the folder found from the exe's
// place (see findGameDir), else ./willrock.ini.
static void loadGameIni(const char *dir, int argc, char **argv)
{
    s_iniPath[0] = 0;
    if (dir) {
        _snprintf(s_iniPath, MAX_PATH - 1, "%s\\bin\\willrock.ini", dir);
        if (GetFileAttributesA(s_iniPath) == INVALID_FILE_ATTRIBUTES)
            _snprintf(s_iniPath, MAX_PATH - 1, "%s\\willrock.ini", dir);
    } else {
        GetFullPathNameA("willrock.ini", MAX_PATH, s_iniPath, 0);
    }
    if (GetFileAttributesA(s_iniPath) == INVALID_FILE_ATTRIBUTES) {
        // No ini: the engine default 2.005 is far too fast for a modern high-DPI mouse (DirectInput gives
        // raw counts, 0.2 degrees each at sensitivity 1), so the test starts lower.
        s_mouseSens = 0.5f;
        printf("no willrock.ini (%s): sensitivity %.3f (test default; pass --game <install dir>)\n", s_iniPath,
               s_mouseSens);
        s_iniPath[0] = 0;
    } else {
        loadIniValues();
    }
    for (int i = 1; i + 1 < argc; i++)                  // --sens <value>: override (test only)
        if (!strcmp(argv[i], "--sens")) {
            float v = (float)atof(argv[i + 1]);
            s_mouseSens = v < 0.01f ? 0.01f : (v > 4.0f ? 4.0f : v);
        }
}

static void loadIniValues()
{
    char b[64];
    GetPrivateProfileStringA("Input", "MouseSens", "", b, sizeof(b), s_iniPath);
    if (b[0]) {
        float s = (float)atof(b);
        s_mouseSens = s < 0.01f ? 0.01f : (s > 4.0f ? 4.0f : s);   // inpINPUT::SetMouseSensitivity clamp
    }
    s_invertMouse = iniBool("Input", "InvertMouse", 0);
    s_alwaysRun = iniBool("GameOpt", "AlwaysRun", 1);
    printf("%s: MouseSens %.3f  InvertMouse %d  AlwaysRun %d\n", s_iniPath, s_mouseSens, s_invertMouse, s_alwaysRun);
}

// ---------------------------------------------------------------------------------------------
// frame
// ---------------------------------------------------------------------------------------------
struct tstKEYS { int dik, cmd; };
static const tstKEYS s_keys[] = {
    { DIK_W, CMD_MOVE_FORWARD }, { DIK_S, CMD_MOVE_BACKWARD }, { DIK_A, CMD_MOVE_LEFT }, { DIK_D, CMD_MOVE_RIGHT },
    { DIK_SPACE, CMD_JUMP }, { DIK_LCONTROL, CMD_CROUCH }, { DIK_C, CMD_CROUCH }, { DIK_LSHIFT, CMD_ACCEL },
    { DIK_RSHIFT, CMD_ACCEL }, { DIK_E, CMD_USE }, { DIK_RETURN, CMD_SHOOT }, { DIK_1, CMD_WPN_1 }, { DIK_2, CMD_WPN_2 },
    { DIK_3, CMD_WPN_3 }, { DIK_4, CMD_WPN_4 }, { DIK_5, CMD_WPN_5 }, { DIK_6, CMD_WPN_6 }, { DIK_7, CMD_WPN_7 },
    { DIK_8, CMD_WPN_8 }, { DIK_9, CMD_WPN_9 }, { DIK_0, CMD_WPN_10 }, { DIK_MINUS, CMD_WPN_11 }, { DIK_Q, CMD_WPN_NEXT }, { DIK_R, CMD_FORCE_RECHARGE },
    // the powerups (asked for): I quad damage, O unholy armor (immortality), P slow motion
    { DIK_I, CMD_ACTIVATE_QUAD_DAMAGE }, { DIK_O, CMD_ACTIVATE_UNHOLY_ARMOR },
    { DIK_P, CMD_ACTIVATE_SLOW_MOTION }, { DIK_TAB, CMD_SHOWFRAGS },
};
static int s_mouseShoot;                                // the left mouse button this frame (CMD_SHOOT)

static int s_cinSkip;                                   // (test) --cintest: the skip key pressed
static void setInput(const unsigned char keys[256], float mdx, float mdy)
{
    static unsigned char last[64];
    unsigned char now[64];
    memset(now, 0, sizeof(now));
    for (unsigned k = 0; k < sizeof(s_keys) / sizeof(s_keys[0]); k++)
        if (keys[s_keys[k].dik] & 0x80)
            now[s_keys[k].cmd] = 1;
    // the debug keys as controls.ini binds them: DBG_n = Alt + the number key n (DIK_1 + n - 1); with Alt held the
    // number keys are not weapon choices
    if ((keys[DIK_LMENU] | keys[DIK_RMENU]) & 0x80)
        for (int n = 0; n < 9; n++)
            if (keys[DIK_1 + n] & 0x80) {
                now[CMD_WPN_1 + n] = 0;
                now[CMD_DBG_1 + n] = 1;
            }
    if (s_mouseShoot)
        now[CMD_SHOOT] = 1;
    if (getenv("WR_TABTEST"))                           // (test) TAB held: the statistics
        now[CMD_SHOWFRAGS] = 1;
    // PLACEHOLDER (test, asked for): only the shovel, the colt, the shotgun, the machinegun, the minigun, the bomb and
    // the bazooka fire -- the other weapons' shots (their Shoot / ProcessSHOOT and projectiles) are not rebuilt; they
    // idle, run, come out and go away.
    if (plrPlayer && plrPlayer->curWeapon != PLR_WPN_SVL && plrPlayer->curWeapon != PLR_WPN_CLT && plrPlayer->curWeapon != PLR_WPN_STG && plrPlayer->curWeapon != PLR_WPN_MCG &&
        plrPlayer->curWeapon != PLR_WPN_MNG && plrPlayer->curWeapon != PLR_WPN_BMB && plrPlayer->curWeapon != PLR_WPN_BSK)
        now[CMD_SHOOT] = 0;
    for (int c = 0; c < 64; c++)
        s_cmds[c].flags = (now[c] ? 1 : 0) | (now[c] != last[c] ? 2 : 0);   // on / changed this frame
    memcpy(last, now, sizeof(last));
    static int cinLast;
    int cin = (keys[DIK_SPACE] & 0x80) || (keys[DIK_RETURN] & 0x80) || s_mouseShoot || s_cinSkip;
    s_cinCmds[0].flags = (cin ? 1 : 0) | (cin != cinLast ? 2 : 0);
    cinLast = cin;
    gsSysInput->mouseDX = mdx;
    gsSysInput->mouseDY = mdy;
}

static void cross(const m3dV *a, const m3dV *b, m3dV *o)
{
    o->x = a->y * b->z - a->z * b->y;
    o->y = a->z * b->x - a->x * b->z;
    o->z = a->x * b->y - a->y * b->x;
}

// The view of plrPLAYER::UpdateCamera (v12 0x00422340, normal branch): eye = position + eye height,
// forward = GetDir, right = GetDirRight.
static void cameraFromPlayer(plrPLAYER *p, float view[16], m3dV *eyeOut)
{
    m3dV pos, d, r, u;
    p->inst->GetPos(&pos);
    p->inst->GetDir(&d);
    p->inst->GetDirRight(&r);
    m3dNormalize(&d);
    m3dNormalize(&r);
    cross(&r, &d, &u);                                 // GetDirRight is -X: up = right x forward
    m3dV e = { pos.x, pos.y + p->eyeHeight, pos.z };
    if ((gsSysCinema && (gsSysCinema->state & 1)) ||   // a cinema: plrPLAYER::UpdateCamera leaves the camera alone,
        (p->camCtrl && (p->camCtrl->deathEnd > 1e-6f || p->camCtrl->attached))) { // cinSYSTEM animates it; death, altar:
        gsCameraPtr->GetOrigin(&e);                     //   plrCAM_CTRL::Update placed it
        gsCameraPtr->GetVRight(&r);
        gsCameraPtr->GetVUp(&u);
        gsCameraPtr->GetVForw(&d);
        float v[16] = { r.x, u.x, d.x, 0, r.y, u.y, d.y, 0, r.z, u.z, d.z, 0,
                        -(r.x * e.x + r.y * e.y + r.z * e.z), -(u.x * e.x + u.y * e.y + u.z * e.z),
                        -(d.x * e.x + d.y * e.y + d.z * e.z), 1 };
        memcpy(view, v, sizeof(v));
        *eyeOut = e;
        return;
    }
    if (const char *cb = getenv("WR_CAMBACK")) {        // (test) the eye pulled back (and up) along the view, to see
        float k = (float)atof(cb);                      //   what is drawn of the body from outside
        e.x -= d.x * k; e.y -= d.y * k; e.z -= d.z * k;
    }
    if (plrCAM_CTRL *cc = p->camCtrl)                   // the recoil kick (plrCAM_CTRL::Update v12 0x0042DB40: out and
        if (cc->kickTime >= 1e-6f && cc->kickT <= cc->kickTime) {   //  back over kickTime, in camera space): the view
            float half = cc->kickTime * 0.5f;           //  kicks, the weapon placed at the camera kicks with it
            m3dV k;
            if (cc->kickT < half)
                m3dLerpVector_L(&m3dVZero, &cc->kick, 0, half, cc->kickT, &k);
            else
                m3dLerpVector_L(&cc->kick, &m3dVZero, half, cc->kickTime, cc->kickT, &k);
            e.x += r.x * k.x + u.x * k.y + d.x * k.z;
            e.y += r.y * k.x + u.y * k.y + d.y * k.z;
            e.z += r.z * k.x + u.z * k.y + d.z * k.z;
        }
    gsCameraPtr->SetParameters(&e, &u, &r, &d);         // the game camera (UpdateCamera's call)
    float v[16] = { r.x, u.x, d.x, 0, r.y, u.y, d.y, 0, r.z, u.z, d.z, 0,
                    -(r.x * e.x + r.y * e.y + r.z * e.z), -(u.x * e.x + u.y * e.y + u.z * e.z),
                    -(d.x * e.x + d.y * e.y + d.z * e.z), 1 };
    memcpy(view, v, sizeof(v));
    *eyeOut = e;
}

static platVERT *s_vb;
static int       s_nvb;

static void buildVB()
{
    int n = 0;
    for (int i = 0; i < s_nObj; i++)
        n += s_objs[i].o->nFace * 3;
    s_vb = (platVERT *)malloc(n * sizeof(platVERT));
    m3dV L = { 0.4f, 1.0f, 0.25f };
    m3dNormalize(&L);
    int k = 0;
    for (int i = 0; i < s_nObj; i++) {
        objOBJ *o = s_objs[i].o;
        unsigned c = s_objs[i].color;
        for (int f = 0; f < o->nFace; f++) {
            m3dV *nn = &o->geom->faceNormal[f];
            float lit = nn->x * L.x + nn->y * L.y + nn->z * L.z;
            float s = 0.45f + 0.55f * (lit > 0 ? lit : 0);
            unsigned r = (unsigned)(((c >> 16) & 255) * s), g = (unsigned)(((c >> 8) & 255) * s), b = (unsigned)((c & 255) * s);
            unsigned col = 0xFF000000u | (r << 16) | (g << 8) | b;
            for (int v = 0; v < 3; v++) {
                m3dV w;
                objFaceVertW(o, f, v, &w);
                s_vb[k].x = w.x; s_vb[k].y = w.y; s_vb[k].z = w.z;
                s_vb[k].color = col;
                k++;
            }
        }
    }
    s_nvb = k;
}

// ---------------------------------------------------------------------------------------------
// the physics test area (north of the box, z > 15) and the pool (south-east)
// ---------------------------------------------------------------------------------------------
static const float s_slopeDeg[6] = { 15, 30, 40, 45, 50, 60 };
static const float s_stepH[6]    = { 0.3f, 0.6f, 0.9f, 1.0f, 1.2f, 1.5f };
static const float s_ledgeH[6]   = { 1.5f, 1.8f, 2.0f, 2.2f, 2.5f, 3.0f };
static float laneZ(int k) { return 16.0f + 4.0f * k; } // lane k spans z [laneZ, laneZ + 3]

// n-sided vertical prism (a round column)
static objOBJ *makeColumn(const char *name, float cx, float cz, float r, float y0, float y1, int n, unsigned color)
{
    m3dV v[2 * 32 + 2];
    int f[32 * 3][4], nf = 0;
    for (int i = 0; i < n; i++) {
        float a = 6.2831853f * i / n;
        v[i].x = cx + r * cosf(a); v[i].y = y0; v[i].z = cz + r * sinf(a);
        v[n + i] = v[i]; v[n + i].y = y1;
    }
    v[2 * n].x = cx; v[2 * n].y = y0; v[2 * n].z = cz;
    v[2 * n + 1].x = cx; v[2 * n + 1].y = y1; v[2 * n + 1].z = cz;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        f[nf][0] = i; f[nf][1] = j; f[nf][2] = n + j; f[nf][3] = n + i; nf++;
        f[nf][0] = 2 * n; f[nf][1] = i; f[nf][2] = j; f[nf][3] = -1; nf++;
        f[nf][0] = 2 * n + 1; f[nf][1] = n + i; f[nf][2] = n + j; f[nf][3] = -1; nf++;
    }
    return makeMesh(name, v, 2 * n + 2, f, nf, color);
}

// A roof ridge along z: rises from both x sides to y1 at the middle.
static objOBJ *makeRidge(const char *name, float x0, float x1, float y1, float z0, float z1, unsigned color)
{
    float xm = (x0 + x1) * 0.5f;
    m3dV v[6] = { { x0, 0, z0 }, { x1, 0, z0 }, { xm, y1, z0 }, { x0, 0, z1 }, { x1, 0, z1 }, { xm, y1, z1 } };
    static const int q[5][4] = { { 0, 1, 4, 3 }, { 0, 2, 5, 3 }, { 1, 2, 5, 4 }, { 0, 1, 2, -1 }, { 3, 4, 5, -1 } };
    return makeMesh(name, v, 6, q, 5, color);
}

static void buildTestArea(objOBJ *root)
{
    char nm[32];
    for (int k = 0; k < 6; k++) {                       // slopes rising along +x from x 20, 6 long
        float h = 6.0f * tanf(s_slopeDeg[k] * 3.14159265f / 180.0f);
        sprintf(nm, "slope%02d", (int)s_slopeDeg[k]);
        makeRamp(nm, 20, 26, 0, h, laneZ(k), laneZ(k) + 3, 0x5080C0 + k * 0x101000)->Attach(root);
    }
    for (int k = 0; k < 6; k++) {                       // single steps of different heights at x -30
        sprintf(nm, "stepH%d", k);
        makeBox(nm, -30, 0, laneZ(k), -26, s_stepH[k], laneZ(k) + 3, 0xB09050)->Attach(root);
    }
    for (int k = 0; k < 6; k++) {                       // jump ledges at x -18 (8 deep)
        sprintf(nm, "ledge%d", k);
        makeBox(nm, -18, 0, laneZ(k), -10, s_ledgeH[k], laneZ(k) + 3, 0x9070B0)->Attach(root);
    }
    // crouch tunnel: ceiling at 2.0 (standing: eye 1.6 + radius 0.8 = 2.4 does not fit; crouched 1.0 + 0.8 does)
    makeBox("tunnelL", 0, 0, 15.5f, 8, 3, 16, 0x707070)->Attach(root);
    makeBox("tunnelR", 0, 0, 19.5f, 8, 3, 20, 0x707070)->Attach(root);
    makeBox("tunnelTop", 0, 2.0f, 15.5f, 8, 2.5f, 20, 0x606060)->Attach(root);
    // gaps between pillars: 1.4 (narrower than the body, 2 x 0.8) and 1.8
    makeBox("gapA1", 10, 0, 24, 11, 3, 25.3f, 0xA0A0A0)->Attach(root);
    makeBox("gapA2", 10, 0, 26.7f, 11, 3, 28, 0xA0A0A0)->Attach(root);
    makeBox("gapB1", 10, 0, 30, 11, 3, 31.1f, 0xA0A0A0)->Attach(root);
    makeBox("gapB2", 10, 0, 32.9f, 11, 3, 34, 0xA0A0A0)->Attach(root);
    makeColumn("column", -6, 26, 2, 0, 3, 16, 0xC8C8B0)->Attach(root);   // round wall to slide along
    makeRidge("ridge", -10, -6, 1.5f, 32, 34, 0xB06040)->Attach(root);   // 37 degree roof ridge
    // gap jumps on platforms 1 high: gaps 3 and 6
    makeBox("jumpP1", -10, 0, 36, -6, 1, 39, 0x60A060)->Attach(root);
    makeBox("jumpP2", -3, 0, 36, 1, 1, 39, 0x60A060)->Attach(root);
    makeBox("jumpP3", 7, 0, 36, 11, 1, 39, 0x60A060)->Attach(root);
    // the pool: walls 4 high around x 24..36, z -36..-24; water surface at 3.5; a ramp up to the rim and a
    // shallow shelf inside (2.5 high: 1.0 of water above it)
    makeBox("poolW1", 24, 0, -36, 36, 4, -35.5f, 0x8090A0)->Attach(root);
    makeBox("poolW2", 24, 0, -24.5f, 36, 4, -24, 0x8090A0)->Attach(root);
    makeBox("poolW3", 24, 0, -36, 24.5f, 4, -24, 0x8090A0)->Attach(root);
    makeBox("poolW4", 35.5f, 0, -36, 36, 4, -24, 0x8090A0)->Attach(root);
    makeRamp("poolRamp", 17, 24, 0, 4.04f, -31, -29, 0x80A0C0)->Attach(root);
    makeBox("poolShelf", 24.5f, 0, -35.5f, 27.5f, 2.5f, -24.5f, 0x708090)->Attach(root);
}

// The water: as lev_1_5_atrium's Actor_pool (class "water", [WATER] maxAmp .1, freqXZ .5, freqTime 2.2,
// [FOG] depth 10, color 255,14,30,44). Its "pool_fog" object is the volume's outline: the real scnDOMAIN
// (scn_domain.cpp) is built from it, the bottom set as pteFOG_GROUND::InitParam does (surface - depth), and
// an entity entry is put into waterTable for waterIsUnderWater / plrPLAYER::UpdateUnderwater. The surface
// is drawn here (the game's objMOD_NOISE_DEFORM waves and water shaders are not rebuilt): a grid moved by
// sine waves of the same amplitude and frequencies.
// (waterTable: game_shared/world_objects.h)
static const float s_waterTop = 3.5f, s_waterAmp = 0.1f, s_waterFreqXZ = 0.5f, s_waterFreqT = 2.2f;
static const float s_poolX0 = 24.5f, s_poolX1 = 35.5f, s_poolZ0 = -35.5f, s_poolZ1 = -24.5f;
static pteFOG_GROUND_DATA s_fog;
static u8 s_waterEnt[0x200];

static void buildWater()
{
    m3dV v[4] = { { s_poolX0, s_waterTop, s_poolZ0 }, { s_poolX1, s_waterTop, s_poolZ0 },
                  { s_poolX1, s_waterTop, s_poolZ1 }, { s_poolX0, s_waterTop, s_poolZ1 } };
    static const int q[1][4] = { { 0, 1, 2, 3 } };
    objOBJ *fogObj = makeMesh("pool_fog", v, 4, q, 1, 0);
    s_nObj--;                                           // not drawn, not part of the level geometry
    objCalcLTM(fogObj);
    memset(s_fog.shader, 0, sizeof(s_fog.shader));
    s_fog.depth = 10.0f;
    s_fog.domain[0].obj = fogObj;                       // pteFOG_GROUND::SetDomainObj
    if (s_fog.domain[0].Init())
        s_fog.nDomain = 1;
    s_fog.domain[0].yMin = s_fog.domain[0].yMax - s_fog.depth;   // InitParam
    s_fog.domain[0].CalcBBox();
    *(u32 *)(s_waterEnt + 0xBC) |= 1;                   // waterWATER::ProcessMsg(1): "water" class bit
    *(float *)(s_waterEnt + 0x16B) = s_waterAmp;        // waterLIQUID::InitObj: max(amplitude)
    *(pteFOG_GROUND_DATA **)(s_waterEnt + 0x16F) = &s_fog;
    waterTable[0] = (waterLIQUID *)s_waterEnt;          // waterLIQUID::ProcessMsg(1)
}

static double s_time;

static void drawWater(int under)
{
    enum { N = 22 };
    static platVERT vb[N * N * 6];
    float t = (float)s_time;
    int k = 0;
    for (int i = 0; i < N; i++)
        for (int j = 0; j < N; j++) {
            float xs[2] = { s_poolX0 + (s_poolX1 - s_poolX0) * i / N, s_poolX0 + (s_poolX1 - s_poolX0) * (i + 1) / N };
            float zs[2] = { s_poolZ0 + (s_poolZ1 - s_poolZ0) * j / N, s_poolZ0 + (s_poolZ1 - s_poolZ0) * (j + 1) / N };
            static const int c[6][2] = { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 0 }, { 1, 1 }, { 0, 1 } };
            for (int n = 0; n < 6; n++) {
                float x = xs[c[n][0]], z = zs[c[n][1]];
                float w = sinf(x * s_waterFreqXZ + t * s_waterFreqT) * cosf(z * s_waterFreqXZ * 1.3f + t * s_waterFreqT * 0.7f);
                vb[k].x = x; vb[k].z = z;
                vb[k].y = s_waterTop + s_waterAmp * w;
                unsigned b = 150 + (unsigned)(40 * w);
                vb[k].color = (under ? 0x60000000u : 0x9C000000u) | (30u << 16) | (90u << 8) | b;
                k++;
            }
        }
    rndTrisAlpha(vb, k / 3);
}

// ---------------------------------------------------------------------------------------------
// a real level (DEBUG > PLAY LEVEL): the .lg through the rebuilt sioReadScene, its own collision grid
// (chunk 0x21F) under scnSCENE::InitLevel, the player at the "start_pos" instance (position = its matrix
// row 3, facing = its Z axis, as the level start puts the player; the editor reads it the same way)
// ---------------------------------------------------------------------------------------------
static scnSCENE  *s_lvl;                                // the level being played (0: the box level)

static const char *s_gameDir = ".";
int __fastcall aiPlannerInit();                     // ai_att_planner.cpp

static int s_wantShovel = 1;                            // --noshovel: the bare HUD block instead

// PLACEHOLDER: the player's frame as the game sends it (FRAME -> plrPLAYER::ProcessMsg -> ProcessFRAME); with no
// rebuilt weapon in the hands (the box level, no game data) only its movement (ProcessInputMove)
// (test) the constant colour of the first object with faces in a tree (--colortest)
static u32 tstFirstConstColor(objOBJ *o)
{
    if (!o)
        return 0;
    if (o->nFace > 0 && o->geom)
        return o->geom->constColor;
    objOBJ *c = o->child;
    if (c)
        do {
            u32 r = tstFirstConstColor(c);
            if (r)
                return r;
            c = c->next;
        } while (c != o->child);
    return 0;
}

static void playerFrame(plrPLAYER *p)
{
    gssScreenDX = SCR_W;                                // (the window can change size: F11, full screen)
    gssScreenDY = SCR_H;
    gscCursor.UpdatePosMouse();                         // gssINP_SYSTEM::ProcessInput's first call (v12 0x00535228)
    tstGameLogPlayer(p);
    if (p->weapons[p->curWeapon] && p->weapons[p->curWeapon]->inst && uiSystem)
        p->ProcessFRAME();
    else
        p->ProcessInputMove();
    // PLACEHOLDER (test) for the game's key handler (v12 0x004AB7CB, not rebuilt): USE while dead. Single player there
    // is uiSystem->QuickLoadLast (slot +0x90, the save games are not rebuilt), so the multiplayer branch is taken here:
    // RevitalizeStart(0) -> the server's notify 3 -> 4 -> Revitalize(0) at a start position (all rebuilt)
    if (wrEVCall<BOOL32>(p, 0x74) && !(p->moveFlags & 0x40000000) && gsSysInput->IsOn(CMD_USE) &&
        gsSysInput->IsToggled(CMD_USE))
        p->RevitalizeStart(0);
}

void tstShowLoading(const char *level);                 // test_menu.cpp

static int s_quietLoad;
void __fastcall gsEndFrameInsts(int updAnimSnd);   // src/gs/gs_rend.cpp
// The level being played ends: gsLVL_SYSTEM::DestroyLevel (rebuilt) -- its entities get TERM, the scene's level
// data freed. (Also on the way back to the box level, so nothing of it keeps running or is left for the next one.)
static void endLevel()
{
    if (!(s_lvl && gsSysLevel))
        return;
    tstRendDrvEnsure();
    gsScenePtr = s_lvl;
    gsSysLevel->gsLVL_SYSTEM::DestroyLevel(0);
    if (waterTable[0] == (waterLIQUID *)s_waterEnt)      // (test) the box level's pool survives
        waterTable[0] = 0;
    delete s_lvl;                                       // (test) a new scnSCENE per level (the game reuses its one)
    s_lvl = 0;
    gsScenePtr = 0;
}

static plrPLAYER *startLevel(const char *path)
{
    if (!txmManager) {                                  // (no menu: textures were never set up)
        tstHaveDevice = 1;
        if (!tstTexInit(s_gameDir))
            return 0;
    }
    tstSndStart(s_gameDir);                             // (test) once: the message and sound systems
    endLevel();                                         // the previous level ends

    if (!s_quietLoad)                                   // (a teleport, not level complete: "Loading..." only)
        tstShowLoading(path);                           // the loading screen while it loads
    // PLACEHOLDER for gsLVL_SYSTEM::PreloadInitLevel (v12 0x00542DB0, not rebuilt): with the preload switched off
    // ([Level] has no Preload* keys) what it leaves is its last step -- no texture marked used (TXM_F_ADDED)
    txmManager->MngTexState(TXM_F_ADDED, TXM_STATE_CLEAR);
    scnSCENE *s = tstLoadScene(path);
    if (!s)
        return 0;
    tstLevelObjFlags(s);                                // the "&tag" object flags (&plrc, &dom, &h, ...)
    objCalcLTM(s->root);
    objCalcBBox(s->root);
    for (int k = 0; k < 300; k++)                       // the level's own templates (doors, lifts...): start pose
        if (s->tplPool[k].magic && s->tplPool[k].root)
            tstPoseTemplate(&s->tplPool[k]);
    gsScenePtr = s;
    if (!s->InitLevel()) {
        printf("InitLevel failed: %s\n", path);
        return 0;
    }
    {   // PLACEHOLDER (test, the level loader's order not rebuilt): the hidden domain objects ("&dom", procState 8)
        // get no face normals from InitLevel, yet 1.2 has them (a spawner's DistPointObj reads them; it crashed)
        struct N { static void walk(objOBJ *o) { if (!o) return;
            if (o->geom && o->nFace > 0 && !o->geom->faceNormal)
                o->_CalcFaceNormals();
            objOBJ *f = o->child; int more = 0;
            for (objOBJ *c = f; c && (!more || c != f); c = c->next, more = 1) walk(c); } };
        N::walk(s->root);
    }
    if (getenv("WR_NORMCHK")) {                         // (test) objects with faces but no face normals
        struct F { static void walk(objOBJ *o) { if (!o) return;
            if (o->geom && o->nFace > 0 && !o->geom->faceNormal)
                printf("normchk: %s (procState %X state %X, %d faces)%c", o->name ? o->name : "?", o->procState, o->state, o->nFace, 10);
            objOBJ *f = o->child; int more = 0;
            for (objOBJ *c = f; c && (!more || c != f); c = c->next, more = 1) walk(c); } };
        F::walk(s->root);
    }
    if (!s->coll)
        printf("level has no collision grid (chunk 0x21F / 0x21C): nothing to stand on\n");
    m3dV pos = { 0, 0, 0 }, dir = { 0, 0, 1 };
    // the start positions as the game takes them: gsMP_SYSTEM::ProcessINIT_LEVEL (rebuilt; the level's INIT_LEVEL)
    // and SelectStartPosDir (rebuilt; plrPLAYER_COMMON::ProcessINIT_MAP). PLACEHOLDER (test): 1.2 plays an MP map
    // (lev_mpdm_*) only as an MP game, so for one the positions are collected as the host's (mode 2, Server) does --
    // every "start_pos*" (lom has only start_pos_1..18), put on the floor; the test itself stays single player.
    int found;
    {
        int mode = gsSysMP->mode;
        if (strstr(path, "lev_mp"))
            gsSysMP->mode = gsMP_MODE_SERVER;
        gsSysMP->gsMP_SYSTEM::ProcessINIT_LEVEL();
        gsSysMP->mode = mode;
        found = gsSysMP->nStartPos;
        if (found)
            gsSysMP->SelectStartPosDir(&pos, &dir, 0);
    }
    float L = sqrtf(dir.x * dir.x + dir.z * dir.z);
    if (L < 1e-4f) { dir.x = 0; dir.z = 1; } else { dir.x /= L; dir.z /= L; }
    tstGameLogLevel(path, s->nObj);
    printf("level %s: %d objects, coll %p, start_pos %s at %.2f %.2f %.2f facing %.2f %.2f\n", path, s->nObj,
           (void *)s->coll, found ? "found" : "MISSING", pos.x, pos.y, pos.z, dir.x, dir.z);
    if (const char *fn = getenv("WR_OBJFIND")) {        // (test) the objects named like that, scene and templates
        struct F { static void walk(objOBJ *o, const char *fn, const char *where) {
            if (!o) return;
            if (o->name && strstr(o->name, fn)) {
                txmTEXTURE *tx = o->nFace > 0 ? o->GetFaceTexMtlNmb(0, 0) : 0;
                printf("objfind %s: %s faces %d procState %X state %X rend %X tex %s%c", where, o->name, o->nFace,
                       (unsigned)o->procState, (unsigned)o->state, (unsigned)o->rendState, tx ? tx->name : "-", 10);
            }
            objOBJ *c = o->child;
            if (c) do { walk(c, fn, where); c = c->next; } while (c != o->child);
        } };
        F::walk(s->root, fn, "scene");
        for (int k = 0; k < 300; k++)
            if (s->tplPool[k].magic && s->tplPool[k].root)
                F::walk(s->tplPool[k].root, fn, s->tplPool[k].tplName);
    }
    tstEntsGameDir = s_gameDir;                         // (the templates FindTplName loads: player.tpl first)
    plrPLAYER *p = makePlayer(s, &pos);
    p->inst->SetPosDir_XZ(&pos, &dir);
    p->vel.x = p->vel.y = p->vel.z = 0;
    p->pitch = 90.0f;
    if (!s_alwaysRun)
        p->cfg.bits &= ~1u;
    s_lvl = s;
    tstEntsGameDir = s_gameDir;
    // PLACEHOLDER: gsLVL_SYSTEM::ProcessINIT_LEVEL (not rebuilt) starts the shaders with the level (the water's
    // fog volumes); the previous level's end first
    if (waterTable[0] == (waterLIQUID *)s_waterEnt)      // (test) the box level's pool is not in this level
        waterTable[0] = 0;
    pteManager->TermLevel();
    pteManager->InitLevel(s);
    tstEntsStart(s, p);                                 // the level's doors and buttons (test host)
    if (s_wantShovel)                                   // the rebuilt shovel in the hands (--noshovel: not)
        if (pwpWEAPON *w = tstEntsWeapon(s, p, "pwp_svl"))
            p->weapons[0] = w;
    if (s_wantShovel) {                                 // the colt in slot 1 (key 2), put away
        if (pwpWEAPON *w = tstEntsWeapon(s, p, "pwp_clt")) {
            p->weapons[1] = w;
            w->inst->flags |= 1;
        }
        // the other weapons (as plrPLAYER_COMMON::ProcessINIT's loop over the classes), put away
        static const char *cls[PLR_WPN_COUNT] = { 0, 0, "pwp_stg", "pwp_mcg", "pwp_crb", "pwp_acd", "pwp_bsk", "pwp_mdg",
                                                  "pwp_mng", "pwp_bmb", "pwp_amg" };
        for (int i = 2; i < PLR_WPN_COUNT; i++)
            if (_entFindClass((char *)cls[i]) >= 0)
                if (pwpWEAPON *w = tstEntsWeapon(s, p, cls[i])) {
                    p->weapons[i] = w;
                    w->inst->flags |= 1;
                }
    }
    // the start values (plrPLAYER::SetDefaultValues, v12 0x0041F0E0 -> COMMON 0x00427760): all 11 weapon slots are
    // needed; PLACEHOLDER while the other 9 weapons are not created: its colt / shovel lines only
    {
        int all = 1;
        for (int i = 0; i < PLR_WPN_COUNT; i++)
            if (!p->weapons[i])
                all = 0;
        if (all) {
            p->SetDefaultValues();
        } else if (p->weapons[PLR_WPN_CLT]) {
            p->weapons[PLR_WPN_CLT]->SetAmmo(14.0f);
            p->weapons[PLR_WPN_CLT]->Recharge();
            if (p->weapons[PLR_WPN_SVL])
                p->weapons[PLR_WPN_SVL]->SetAmmo(1.0f);
        }
        // (test, asked for) every weapon made is at hand with all it can carry (its maxAmmo) and a full clip: the
        // machine gun 1000, the acid gun and the bazooka 100... (ammo counts the clip: the HUD shows 900 / 100)
        if (!getenv("WR_STARTAMMO"))
            for (int i = 0; i < PLR_WPN_COUNT; i++)
                if (pwpWEAPON *w = p->weapons[i])
                    if (w->inst && w->maxAmmo > 0.0f) {
                        w->Enable(1);                   // (selectable: SetCurWpn wants WPNF_ENABLED)
                        w->SetAmmo(w->maxAmmo);
                        w->Recharge();
                        printf("weapon %d: ammo %.0f of %.0f, clip %d of %d%c", i, w->ammo, w->maxAmmo, w->clip,
                               w->clipSize, 10);
                    }
    }
    // PLACEHOLDER (test) for the body part of plrPLAYER::ProcessINIT (v12 0x0041D6E1..0x0041D7DE; the rest of it the
    // test does above): the "player_mp" entity -- the local player's body, mpType 2, named "player_3d", out of
    // collision and drawn apart (0x10 | 0x1000), not synchronised (entFlags 0x30 off), its LOD switch off --
    // then the ghost render state (the legs), its weapons kept to the mirror pass, shown; the player's weapons kept
    // out of the body's shadow. entCreate runs its INIT (plrPLAYER_MP::ProcessINIT: the 11 "_item" weapons).
    if (p->inst->tpl && !getenv("WR_NOBODY") && tstPlayerMpInit()) {
        *(int *)((char *)p + 0x88) = -3;
        p->body = (plrPLAYER_MP *)entCreate(s, (char *)"player_mp", (char *)"player_mp", 0, 0, 0);
        if (p->body) {
            plrPLAYER_MP *b = p->body;
            *(int *)((char *)b + 0x98) = 2;   // mpType
            b->inst->flags |= 0x10;
            b->inst->flags |= 0x1000;
            b->entFlags &= ~0x30u;
            b->inst->SetName((char *)"player_3d");
            if (b->inst->lods)
                *(float *)((char *)b->inst->lods + 0x1F8) = -1.0f;
            msgSystem.SendMsg(b, GS_MSG_INIT_GAME, 0, 0);    // (the game's INIT_GAME and the level's INIT_LEVEL went out
            msgSystem.SendMsg(b, GS_MSG_INIT_LEVEL, 0, 0);   //  before it; INIT_GAME switches the local body's weapons on)
            p->SetupGhostRendState();
            b->SetupGhostWpnState();
            b->inst->flags &= ~1u;
            for (int k = 0; k < PLR_WPN_COUNT; k++)
                if (p->weapons[k])
                    gsSysProjShd->AddExcludeList(p->weapons[k]->inst, b->shadowExcl);
            // PLACEHOLDER (test) for plrPLAYER::ProcessINIT_LEVEL -> Revitalize (v12 0x0041D9B9 / 0x0041F1B2, not
            // rebuilt): the outfit put on again, the player's (-1 at the start: the first), so one Skin_0N is shown.
            // WR_SKIN (test) picks another. DELETE when Revitalize is rebuilt.
            p->SelectSkin(getenv("WR_SKIN") ? atoi(getenv("WR_SKIN")) : p->skinNo);
            // (the same Revitalize's SetDefaultValues puts the weapon in the hands, plrPLAYER::SetCurWpn -> the body's: its
            // animation group by the weapon -- the body here comes after the test's own weapon set-up)
            wrEVCall<void>(b, 0x108, p->curWeapon, 0);
            printf("body: player_mp %s, %d sequences, holders %s%c", b->inst->tpl->tplName, b->inst->tpl->nSeq,
                   b->holders && b->holders[0] ? "found" : "MISSING", 10);
            if (getenv("WR_BODYDUMP"))                  // (test) the body's object tree
                tstDumpObj(b->inst->root, 0, 30);
        } else
            printf("body: player_mp not created%c", 10);
    }
    // as plrPLAYER_COMMON::ProcessINIT_MAP (v12, not rebuilt) in single player: SetPosDir(start, on the floor) -- the
    // test found start_pos itself (gsMP_SYSTEM::ProcessINIT_LEVEL not run); it lights the weapon in the hands
    if (p->weapons[p->curWeapon])
        wrEVCall<void>(p, 0x70, &pos, &dir, 1);
    printf("start: on the floor at %.2f %.2f %.2f\n", pos.x, pos.y, pos.z);
    tstPlayerJoin(p);
    // PLACEHOLDER (test) for this part of plrPLAYER::ProcessINIT (v12 0x0041D7E0, not rebuilt): the altar's camera
    if (!p->tradeCamera && gsSysLevel)
        p->tradeCamera = entCreate(s, (char *)"sob_tra_camera", (char *)"sfx_trading_altar_camera", 0, 0, 0);
    if (const char *dn = getenv("WR_INSTDUMP")) {       // (test) one instance: flags, entity, objects
        animINST *a = s->FindInstName((char *)dn);
        if (a) {
            printf("instdump %s: flags %X tpl %s class %s ent %p snd %d%c", dn, a->flags, a->tplName ? a->tplName : "-",
                   a->className ? a->className : "-", (void *)a->ent, a->ent ? a->ent->nSndChannels : 0, 10);
            tstDumpObj(a->root, 0, 10);
        }
    }
    if (const char *fn = getenv("WR_OBJFIND"))          // (test) WR_OBJFIND in the instances' own trees
        for (int k = 0; k < 1000; k++) {
            animINST *a = s->InstPtr(k);
            if (a->magic != 0x41494E53 || !a->root)
                continue;
            struct G { static void walk(objOBJ *o, const char *fn, animINST *a) {
                if (o->name && strstr(o->name, fn))
                    printf("objfind inst %s (ent %p): %s faces %d procState %X state %X%c", a->name ? a->name : "?",
                           (void *)a->ent, o->name, o->nFace, (unsigned)o->procState, (unsigned)o->state, 10);
                objOBJ *c = o->child;
                if (c) do { walk(c, fn, a); c = c->next; } while (c != o->child);
            } };
            G::walk(a->root, fn, a);
        }
    // as __gsLoadLevel after the instances (LoadInstAll) and their textures: the templates and textures the level
    // no longer uses let go -- gsLVL_SYSTEM::PreloadUnloadData (rebuilt), the menu's kept (test)
    if (gsSysLevel) {
        tstMenuMarkTex();
        if (getenv("WR_TEXDBG")) {                      // (test) what the purge will see
            int n[4] = { 0, 0, 0, 0 };
            for (txmTEXTURE *t = txmManager->GetNext(0); t; t = txmManager->GetNext(t))
                n[((t->flags & 0x100) ? 2 : 0) + ((t->flags & 0xE00) ? 1 : 0)]++;
            printf("texdbg: plain free %d plain kept %d scene free %d scene kept %d%c", n[0], n[1], n[2], n[3], 10);
        }
        gsSysLevel->gsLVL_SYSTEM::PreloadUnloadData();
    }
    // PLACEHOLDER (test) for the loading screen's frames (gsRENDER_SYSTEM::EndFrame while the level loads): the
    // animation flags the instances got at their init (0x8000 "sequence started") are gone before the first frame
    // plays sounds -- else every instance whose first sequence has a [SOUND] animSeq sounds at once (lev_1_2's
    // start_pos marker has the doors' looping grind on its cycling sequence 0: it never stopped)
    gsEndFrameInsts(0);
    {                                                   // (test) the process memory after the load (leaks across levels)
        struct { DWORD cb, pf; SIZE_T peak, ws, a, b, c, d, pagefile, peakPage; } pm = { sizeof(pm) };
        typedef BOOL (WINAPI *K32PMI)(HANDLE, void *, DWORD);
        K32PMI f = (K32PMI)GetProcAddress(GetModuleHandleA("kernel32.dll"), "K32GetProcessMemoryInfo");
        if (f && f(GetCurrentProcess(), &pm, sizeof(pm)))
            printf("memory: working set %u MB, committed %u MB%c", (unsigned)(pm.ws >> 20), (unsigned)(pm.pagefile >> 20), 10);
        int nt = 0, nl = 0;
        double mb = 0;
        for (txmTEXTURE *t = txmManager->GetNext(0); t; t = txmManager->GetNext(t)) {
            nt++;
            if (t->flags & TXM_F_LOADED) {
                nl++;
                mb += (double)t->w * t->h * 4 * 1.33 / 1048576.0;
            }
        }
        printf("memory: %d textures, %d loaded (about %.0f MB with mips)%c", nt, nl, mb, 10);
    }
    return p;
}

// PLACEHOLDER (test side) for the level change of a teleport domain: wrGAME (GS_MSG_TELEPORT_LEVEL, not level
// complete) -> GS_MSG_LOAD_LEVEL -> gsLVL_SYSTEM::LoadLevel (v12, not rebuilt): PrepareReloadAnchor keeps the anchor
// instance's matrix, the entities inside the domain (the player) survive the reload, ApplyReloadAnchor sends them
// GS_MSG_SET_LEVEL_ANCHOR with M = inverse(old anchor) * new anchor (entENTITY::PostProcessMsg: Transform(M, 2)).
// Here: the new level is started as from the menu and the player's state (health, armor, money, view, weapon and
// its clip) and matrix * M carried over.
static plrPLAYER *teleportLevel(plrPLAYER *p, gsLOAD_SCENE_PARAM *rq)
{
    char path[0x200];
    if (!strchr(rq->level, '.') && !strchr(rq->level, '\\'))   // (fnmMakeFullName(PathScn, name, "lg"))
        snprintf(path, sizeof path, "%s/scenes/%s.lg", s_gameDir, rq->level);
    else
        snprintf(path, sizeof path, "%s", rq->level);
    m3dMATR old, inv, pm;
    animINST *a = rq->anchorInst[0] ? s_lvl->FindInstName(rq->anchorInst) : 0;
    if (a)
        memcpy(&old, a->matr, sizeof old);
    else
        old.Identity();
    memcpy(&pm, p->inst->matr, sizeof pm);
    float health = p->health, armor = p->armor, money = p->money, pitch = p->pitch;
    int cur = p->curWeapon, clip[PLR_WPN_COUNT], have[PLR_WPN_COUNT];
    unsigned wflags[PLR_WPN_COUNT];
    float ammo[PLR_WPN_COUNT];
    for (int i = 0; i < PLR_WPN_COUNT; i++) {
        have[i] = p->weapons[i] != 0;
        clip[i] = have[i] ? p->weapons[i]->clip : 0;
        ammo[i] = have[i] ? p->weapons[i]->ammo : 0;
        wflags[i] = have[i] ? p->weapons[i]->flags : 0;
    }
    plrPOWERUP pw[3];                                   // (the game keeps the whole player: the powerups held or
    memcpy(pw, p->powerup, sizeof pw);                  //  running, the treasures, the key, the kills, too)
    int treasures = p->treasures, frags = p->frags, deaths = p->unk_217;
    unsigned keyFlag = p->moveFlags & 0x1000;
    printf("teleport: to %s (anchor %s)\n", path, rq->anchorInst);
    plrPLAYER *q = startLevel(path);
    if (!q)
        return p;
    m3dMATR nw, m, out;
    a = rq->anchorInst[0] ? s_lvl->FindInstName(rq->anchorInst) : 0;
    if (a)
        memcpy(&nw, a->matr, sizeof nw);
    else {
        printf("teleport: anchor %s not in the new level\n", rq->anchorInst);
        nw = old;
    }
    if (!old.Invert(&inv))
        inv.Identity();
    m3dMultiplyMatr(&inv, &nw, &m);
    m3dMultiplyMatr(&pm, &m, &out);
    m3dV pos = { out.m[3][0], out.m[3][1], out.m[3][2] }, dir = { out.m[2][0], 0, out.m[2][2] };
    float L = sqrtf(dir.x * dir.x + dir.z * dir.z);
    if (L < 1e-4f) { dir.x = 0; dir.z = 1; } else { dir.x /= L; dir.z /= L; }
    // not level complete: the player stays where the domain had them (moved by the anchor's change; with no anchor,
    // in the same place), put there by plrPLAYER::SetPosDir so the floor under them (its lightmap) lights the weapon
    // again; level complete: the new level's start_pos (startLevel put them there)
    if (!rq->resetLevelTime)
        wrEVCall<void>(q, 0x70, &pos, &dir, 1);
    q->health = health;
    q->armor = armor;
    q->money = money;
    q->pitch = pitch;
    for (int i = 0; i < PLR_WPN_COUNT; i++)
        if (q->weapons[i] && have[i]) {
            q->weapons[i]->flags = wflags[i];
            q->weapons[i]->ammo = ammo[i];
            q->weapons[i]->clip = clip[i];
        }
    memcpy(q->powerup, pw, sizeof pw);
    if (pw[PLR_PWR_TIMESCALE].state == PLR_PWR_ACTIVE)  // (slow motion goes on)
        gsMsgSetElapsedTimeScale(0.25f);
    q->treasures = treasures;
    q->frags = frags;
    q->unk_217 = deaths;
    q->moveFlags |= keyFlag;
    if (q->weapons[cur] && cur != q->curWeapon)
        q->SetCurWpn(cur, 0);
    printf("teleport: player at %.2f %.2f %.2f facing %.2f %.2f\n", pos.x, pos.y, pos.z, dir.x, dir.z);
    if (getenv("WR_TPTLOOP")) {                         // (test) on into the next level's first teleport domain
        tstEntsToTpt(q);
        m3dV pp, dd = { q->inst->matr[8], 0, q->inst->matr[10] };
        q->inst->GetPos(&pp);
        wrEVCall<void>(q, 0x70, &pp, &dd, 1);
    }
    return q;
}

// PLACEHOLDER (test side) for wrLVL_SYSTEM::ProcessMsg's GS_MSG_TELEPORT_LEVEL (v12 0x004AB6AE) and wrGO_SYSTEM's
// level-over frame (v12 0x004A6491): not level complete -> "UI_LOADING" centre text, the load 0.2 s later, the
// player kept by the anchor; level complete -> gsAppState 0x80000, "GAME_LEVEL_COMPLETE" until use, then
// GS_MSG_TERM_MAP and the level loaded 0.5 s later from its start (the player's inventory kept).
static gsLOAD_SCENE_PARAM s_lcReq;
static int s_lcState;
static float s_lcTimer;
static void lcCenter(const char *id, float t, int prio)
{
    if (uiSystem && gsStrings)
        uiSystem->ShowCenterText(gsStrings->GetStringById(gsStrings->GetStringId((char *)id)), t, prio);
}
// the level is in: the message gone, a request still queued from the old level dropped (it would fire again)
static void lcDone()
{
    static const wrWCHAR none[1] = { 0 };
    if (uiSystem)
        uiSystem->ShowCenterText(none, 0.01f, 0);
    gsLOAD_SCENE_PARAM rq;
    while (tstTakeTeleport(&rq))
        ;
}
// (test, asked for) level complete pauses the game till use is pressed and the next level is in
static int lcPaused() { return s_lcState == 1 || s_lcState == 3; }
static plrPLAYER *levelChangeFrame(plrPLAYER *p)
{
    gsLOAD_SCENE_PARAM rq;
    if (!s_lcState && tstTakeTeleport(&rq)) {
        s_lcReq = rq;
        if (rq.resetLevelTime) {
            s_lcState = 1;
            gsAppState |= 0x80000;
            printf("levelchange: level complete (%s), paused till use%c", rq.level, 10);
        } else {
            s_lcState = 2;
            s_lcTimer = 0.2f;
            printf("levelchange: loading %s%c", rq.level, 10);
            lcCenter("UI_LOADING", 1.0f, 4);
        }
    }
    if (s_lcState == 1) {
        lcCenter("GAME_LEVEL_COMPLETE", 0.1f, 0);
        if (gsSysInput && gsSysInput->IsOn(CMD_USE) && gsSysInput->IsToggled(CMD_USE)) {
            msgSystem.SendAll(0x3F3, 0, 0);             // (0x80000 stays till the load: the domain sends no more)
            s_lcState = 3;
            s_lcTimer = 0.5f;
        }
    } else if (s_lcState == 2 && (s_lcTimer -= gsElapsedTime) <= 0) {
        s_lcState = 0;
        s_quietLoad = 1;
        p = teleportLevel(p, &s_lcReq);
        s_quietLoad = 0;
        lcDone();
    } else if (s_lcState == 3 && (s_lcTimer -= gsElapsedTime) <= 0) {
        s_lcState = 0;
        s_lcReq.anchorInst[0] = 0;                      // (from the new level's start_pos)
        p = teleportLevel(p, &s_lcReq);
        gsAppState &= ~0x80000;
        lcDone();
    }
    return p;
}

// PLACEHOLDER (test renderer) for Partrend.obj's partRender / partEMITTER_PHYS::Register / RegisterPart (v12
// 0x004D42F0 / 0x004D5980 / 0x004D5470, not rebuilt): every live particle as a camera-facing quad of its size, turned
// by its angle, in its colour (PartUpdateVisible first, as Register does), alpha blended, no depth writes. The frame
// is a cell of a frameFirst x frameCount grid (the shells' "sfx_mcg_shell_2x2": SetTextureFrameAnim(2, 2, 0, 4), a
// random one of 4) // ?
// v12 0x004D9BB0 / 0x004D9BD0 (engine Partrend.obj, used only by Register)
static int partIsOwnerInstHidden(partEMITTER_PHYS *e)  { return e->attachInst ? e->attachInst->flags & 3 : 0; }
static int partIsOwnerInstVisible(partEMITTER_PHYS *e) { return e->attachInst ? e->attachInst->flags & 0x80000000 : 1; }

static void tstDrawParticles(const float view[16])
{
    static platVERT3T *v;
    static int nMax;
    m3dV r = { view[0], view[4], view[8] }, u = { view[1], view[5], view[9] };
    int nSeen = 0, nAll = 0;
    for (int k = 0; k < PART_MAX_EMITTERS; k++) {
        partEMITTER_PHYS *e = _partEmitList[k];
        if (!e)
            continue;
        nAll++;
        // partEMITTER_PHYS::Register's tests (v12 0x004D5980; the level's PVS, the camera's portal test, is not
        // rebuilt, so only the view): nothing to do; the owner hidden; outside the view; an attached one only while
        // its owner was drawn this frame. Only then is it seen (state 0x40): partUpdate runs a paused one
        // (0x8000000) only when seen, so what is out of sight costs nothing. 0x400000: never drawn.
        if (!(e->flags & 0x8000000) && (!e->nPart || (e->flags & 0x400000)))
            continue;
        if (partIsOwnerInstHidden(e))
            continue;
        if (!e->attachInst || (e->flags & 0x20800000)) {
            // (partEMITTER_PHYS::CalcBBox v12 0x004D5C00 is not rebuilt: the box of the live particles)
            struct { const void *vtbl; int type; m3dV mn, mx; } box = { m3dBOX_vftable, 1 };
            ((m3dBOX *)&box)->m3dBOX::CalcStart();
            for (int i = 0; i < e->nPart; i++) {
                partPART *q = e->partPtrs[(e->first + i) % e->maxPart];
                struct { const void *vtbl; int type; m3dV mn, mx; } one = { m3dBOX_vftable, 1 };
                float h = q->size > 0.1f ? q->size : 0.1f;
                one.mn.x = q->pos.x - h; one.mn.y = q->pos.y - h; one.mn.z = q->pos.z - h;
                one.mx.x = q->pos.x + h; one.mx.y = q->pos.y + h; one.mx.z = q->pos.z + h;
                ((m3dBOX *)&box)->m3dBOX::CalcUpdate((m3dBOX *)&one);
            }
            if (e->nPart && gsCameraPtr->QualBoxPOV((m3dBOX *)&box) == 1)
                continue;
        }
        if (e->attachInst && !(e->flags & 0x20800000) && !partIsOwnerInstVisible(e))
            continue;
        nSeen++;
        e->state |= 0x40;
        if (e->flags & 0x400000)
            continue;
        if (e->nPart <= 0)
            continue;
        if (nMax < e->nPart * 6) {
            nMax = e->nPart * 6;
            v = (platVERT3T *)realloc(v, nMax * sizeof(platVERT3T));
        }
        int n = 0;
        for (int i = 0; i < e->nPart; i++) {
            partPART *q = e->partPtrs[(e->first + i) % e->maxPart];
            if (q->age > 1.0f)
                continue;
            e->PartUpdateVisible(1, &q);
            if (e->sizeSpl && !(e->flags & 0x40000000))      // (a size spline: sampled when drawn, partRender)
                e->sizeSpl->GetValue(q->age, &q->size, 0);
            float a = q->rot * 3.14159265f / 180.0f, c = cosf(a), sn = sinf(a);
            float h = q->size;
            m3dV ax = { (r.x * c + u.x * sn) * h, (r.y * c + u.y * sn) * h, (r.z * c + u.z * sn) * h };
            m3dV ay = { (u.x * c - r.x * sn) * h, (u.y * c - r.y * sn) * h, (u.z * c - r.z * sn) * h };
            if (e->flags & 0x1000000) {                 // the quad squeezed in the emitter's frame (partRender v12 0x004D4522)
                float *M = (float *)&e->scaleCS;
                m3dV *ab[2] = { &ax, &ay };
                for (int w = 0; w < 2; w++) {
                    m3dV q0 = *ab[w];
                    float lx = (q0.x * M[0] + q0.y * M[1] + q0.z * M[2]) * e->scaleCSVal[0];
                    float ly = (q0.x * M[4] + q0.y * M[5] + q0.z * M[6]) * e->scaleCSVal[1];
                    float lz = (q0.x * M[8] + q0.y * M[9] + q0.z * M[10]) * e->scaleCSVal[2];
                    ab[w]->x = lx * M[0] + ly * M[4] + lz * M[8];
                    ab[w]->y = lx * M[1] + ly * M[5] + lz * M[9];
                    ab[w]->z = lx * M[2] + ly * M[6] + lz * M[10];
                }
            }
            int nx = e->frameFirst > 0 ? e->frameFirst : 1, ny = e->frameCount > 0 ? e->frameCount : 1;
            int f = (q->frame + (int)(q->age / q->invLife * e->frameFps)) % (nx * ny);
            float u0 = (float)(f % nx) / nx, u1 = u0 + 1.0f / nx, v0 = (float)(f / nx) / ny, v1 = v0 + 1.0f / ny;
            const float cx[4] = { -1, 1, 1, -1 }, cy[4] = { -1, -1, 1, 1 }, cu[4] = { u0, u1, u1, u0 }, cv[4] = { v1, v1, v0, v0 };
            static const int tri[6] = { 0, 1, 2, 0, 2, 3 };
            for (int j = 0; j < 6; j++) {
                int t = tri[j];
                platVERT3T *w = &v[n++];
                w->x = q->pos.x + ax.x * cx[t] + ay.x * cy[t];
                w->y = q->pos.y + ax.y * cx[t] + ay.y * cy[t];
                w->z = q->pos.z + ax.z * cx[t] + ay.z * cy[t];
                w->color = q->color;
                w->u = cu[t];
                w->v = cv[t];
            }
        }
        if (n && e->texture)                            // on first use (the effect textures are TXM_F_UNLOADABLE:
            txmManager->Load(e->texture, 0);            // the driver loads them when drawn)
        if (n)
            rndTrisTex(tstTexSurface(e->texture), v, n / 3, e->flags & 1 ? 2 : 1);   // additive: partRender v12 0x004D4F3A
    }
    static int fr;
    if (getenv("WR_AILOG") && ++fr % 60 == 0)            // (test) what the view test leaves running
        printf("emitters: %d seen of %d%c", nSeen, nAll, 10);
}

// TWEAK (test): WR_NOFLASH=1 hides the muzzle flash (pwpBULLET_WPN::flare, an entity StartAnim'ed by the shot) --
// its instance flagged 1 (not drawn, as rendDRIVER::QualInst). Shown by default since 2026-11-14 (user's wish).
static void hideFlares(plrPLAYER *p)
{
    static int show = -1;                               // (asked for) shown by box_test_flare.exe (an exe named
    if (show < 0) {                                     //  "*flare*") or WR_FLASH=1; hidden otherwise
        char exe[MAX_PATH] = "";
        GetModuleFileNameA(0, exe, MAX_PATH);
        _strlwr(exe);
        show = (strstr(exe, "flare") || getenv("WR_FLASH")) && !getenv("WR_NOFLASH");
    }
    if (show && p->weapons) {                           // PLACEHOLDER (test renderer): the flare drawn additive (its
        for (int i = 1; i < PLR_WPN_COUNT; i++) {       //  material's blend is not read here), so it comes in the
            pwpWEAPON *w = p->weapons[i];               //  blended pass after the gun and the gun hides part of it
            if (w && (i == PLR_WPN_CLT || i == PLR_WPN_STG || i == PLR_WPN_MCG)) {
                entENTITY *f = ((pwpBULLET_WPN *)w)->flare;
                if (f && f->inst && f->inst->root && !(f->inst->root->rendState & 0x40))
                    f->inst->root->SetStateRendYes(0x40);
            }
        }
    }
    if (show || !p->weapons)
        return;
    for (int i = 1; i < PLR_WPN_COUNT; i++) {
        pwpWEAPON *w = p->weapons[i];
        if (w && (i == PLR_WPN_CLT || i == PLR_WPN_STG || i == PLR_WPN_MCG)) {   // (the minigun: a stand-in, no flare)
            entENTITY *f = ((pwpBULLET_WPN *)w)->flare;
            if (f && f->inst)
                f->inst->flags |= 1;
        }
    }
}


extern vidDRIVER *vidDriver;
static void __thiscall tstVisNothing(void *, void *) {}
static int __thiscall tstVisYes(void *, void *) { return 1; }
static int __thiscall tstVisYes2(void *, void *, void *) { return 1; }
static void __thiscall tstVidClear(void *, void *) { rndClearDepth(); }
void vidDRIVER::ToggleModeParam(uint32_t, int) {}       // PLACEHOLDER (test): v12 0x004B4080, no modes here

// PLACEHOLDER (test renderer) for rendDRIVER::BeginCamera's transforms: the view of a camera, its window the other way
// round for a picture in a mirror; planes as the render frame's
extern camCAMERA *tstSceneCam;
extern int tstCullFlip, tstInMirror;
static void cameraView(camCAMERA *c, float *view, float *pr, m3dV *eye)
{
    m3dV e, u, r, d;
    c->GetParameters(&e, &u, &r, &d);
    float v[16] = { r.x, u.x, d.x, 0, r.y, u.y, d.y, 0, r.z, u.z, d.z, 0,
                    -(r.x * e.x + r.y * e.y + r.z * e.z), -(u.x * e.x + u.y * e.y + u.z * e.z),
                    -(d.x * e.x + d.y * e.y + d.z * e.z), 1 };
    memcpy(view, v, sizeof(v));
    float hor = gsCameraPtr->angleHor > 1 ? gsCameraPtr->angleHor : gssCamAngleHor;
    float ahw = gsCameraPtr->aspectHW > 0 ? gsCameraPtr->aspectHW : gssCamAspectHW;
    float zn = 0.05f, zf = 1300.0f, xs = 1.0f / tanf(0.5f * hor * 3.14159265f / 180.0f), ys = xs / ahw;
    float q[16] = { c->winDx < 0 ? -xs : xs, 0, 0, 0, 0, ys, 0, 0, 0, 0, zf / (zf - zn), 1, 0, 0, -zn * zf / (zf - zn), 0 };
    memcpy(pr, q, sizeof(q));
    *eye = e;
}

void tstSetCameraView(camCAMERA *c)
{
    float view[16], pr[16];
    m3dV e;
    cameraView(c, view, pr, &e);
    rndSetTransforms(view, pr);
}

// PLACEHOLDER (test renderer) for gsRENDER_SYSTEM::RenderFrame (v12 0x0051BA50, not rebuilt), as far as what a mirror
// sees needs it: mirrors in the mirror (unless pass 8), the scene and the particles from its camera
static void __thiscall rfRenderFrame(void *, camCAMERA *c, int pass)
{
    if (!(pass & 8))
        gsSysMirror->Render(c);
    float view[16], pr[16];
    m3dV eye;
    cameraView(c, view, pr, &eye);
    rndSetTransforms(view, pr);
    camCAMERA *oldCam = tstSceneCam;
    int oldFlip = tstCullFlip, oldIn = tstInMirror;
    tstSceneCam = c;
    tstCullFlip = c->winDx < 0;
    tstInMirror = 0;
    objMODIFIER::SetViewPos(&eye);
    float e[3] = { eye.x, eye.y, eye.z };
    // the first-person weapon belongs to the near pass, which this frame does not have: not in the picture
    plrPLAYER *me = (plrPLAYER *)plrPlayer;
    animINST *fp = me && me->weapons && me->curWeapon >= 0 && me->curWeapon < PLR_WPN_COUNT && me->weapons[me->curWeapon]
                       ? me->weapons[me->curWeapon]->inst : 0;
    u32 fpFlags = fp ? fp->flags : 0;
    if (fp)
        fp->flags |= 1;
    tstDrawScene(s_lvl, e);
    tstDrawParticles(view);
    if (fp)
        fp->flags = fpFlags;
    tstSceneCam = oldCam;
    tstCullFlip = oldFlip;
    tstInMirror = oldIn;
}

void __fastcall gsEndFrameInsts(int updAnimSnd);   // src/gs/gs_rend.cpp
static void tstAnimSndLog() { if (!getenv("WR_ANIMSND")) return; for (animINST *i = s_lvl->FindNextInst(0); i; i = s_lvl->FindNextInst(i)) if (i->ent && i->ent->nSndChannels > 0 && (i->flags & 0x8000)) printf("animsnd: %s seq %d starts%c", i->name ? i->name : "", i->seq, 10); }
static void render(plrPLAYER *p)
{
    hideFlares(p);
    float view[16], proj[16];
    m3dV eye;
    cameraFromPlayer(p, view, &eye);
    if (pteManager && s_lvl && gsCameraPtr) {           // PLACEHOLDER for the render frame's shader steps (gsRENDER_SYSTEM
        pteManager->UpdateAllShaders(gsElapsedTime);    //  ::BeginFrame v12 0x0051B79D, RenderFrame 0x0051BB7F / 0x0051BB92)
        pteManager->ActivateAllShaders(gsCameraPtr);
        pteManager->PrepareFrame(s_lvl, gsCameraPtr);
        static int logged;
        if (getenv("WR_PTELOG") && !logged++) {         // (test) the shaders made from the level
            printf("pte: %d shaders%c", *(int *)((char *)pteManager + 0x1F44), 10);
            for (pteSHADER *sh = pteManager->FindNextShader(0); sh; sh = pteManager->FindNextShader(sh))
                if (pteFOG_GROUND *f = dynamic_cast<pteFOG_GROUND *>(sh))
                    printf("  fog %s: domains %d objs %d insts %d camOut %d top %.1f depth %.1f far %.1f int %.2f..%.2f col %.0f %.0f %.0f flags %X on %X%c",
                           f->obj && f->obj->name ? f->obj->name : "?", f->nDomain, f->nObj, f->nInst, f->camOut, f->planePt.y,
                           f->depth, f->farDist, f->nearIntensity, f->farIntensity, f->color[0], f->color[1], f->color[2],
                           f->paramFlags, f->flags, 10);
            for (pteSHADER *sh = pteManager->FindNextShader(0); sh; sh = pteManager->FindNextShader(sh))
                if (pteFOG_GROUND *f = dynamic_cast<pteFOG_GROUND *>(sh))
                    if (objOBJ *o = f->domain[0].obj)        // (test) the outline's corners in the world
                        for (int k = 0; k < o->nVert && k < 12; k++) {
                            m3dV *q = &o->geom->vert[k];
                            float *m = o->ltm;
                            printf("    outline %s v%d %.1f %.1f %.1f%c", o->name ? o->name : "?", k,
                                   q->x * m[0] + q->y * m[4] + q->z * m[8] + m[12], q->x * m[1] + q->y * m[5] + q->z * m[9] + m[13],
                                   q->x * m[2] + q->y * m[6] + q->z * m[10] + m[14], 10);
                        }
        }
    }
    if (s_lvl) {                                        // a loaded level: textured, lightmapped
        // the game camera's field of view (horizontal, and the vertical from its height / width ratio: the picture is
        // stretched to the window as the game does) and the render frame's planes (v12 0x0051BAEB: near 0.05, far 1300)
        float hor = gsCameraPtr && gsCameraPtr->angleHor > 1 ? gsCameraPtr->angleHor : gssCamAngleHor;
        float ahw = gsCameraPtr && gsCameraPtr->aspectHW > 0 ? gsCameraPtr->aspectHW : gssCamAspectHW;
        float zn = 0.05f, zf = 1300.0f, xs = 1.0f / tanf(0.5f * hor * 3.14159265f / 180.0f), ys = xs / ahw;
        float pr[16] = { xs, 0, 0, 0, 0, ys, 0, 0, 0, 0, zf / (zf - zn), 1, 0, 0, -zn * zf / (zf - zn), 0 };
        // PLACEHOLDER: as the game's render frame (v12 0x0051BB04, not rebuilt): the modifiers' frame time and
        // view position (the water waves are worked out near it only)
        objMODIFIER::elapsedTime = gsElapsedTime;
        objMODIFIER::SetViewPos(&eye);
        double tp[8];                                   // (test) where the frame's time goes (WR_RTIME=1)
        tp[0] = platTime();
        rndBegin(view, pr, 0xFF000000);                  // the void is black, as the game clears
        if (gsSysMirror) {                               // (as the render frame, v12 0x0051BAB6: before anything else)
            static int mlog;                             // (test) WR_MIRLOG=1: the level's mirrors
            if (getenv("WR_MIRLOG") && !mlog && gsSysMirror->nMirrors && gsSysMirror->mirror[0].normal.x + gsSysMirror->mirror[0].normal.y + gsSysMirror->mirror[0].normal.z != 0) {
                mlog = 1;
                for (int i = 0; i < gsSysMirror->nMirrors; i++) {
                    mrrMIRROR *m = &gsSysMirror->mirror[i];
                    printf("mirror %d %s flags %X at %.1f %.1f %.1f normal %.2f %.2f %.2f color %.0f %.0f %.0f %.0f faces %d state %X%c", i, m->obj->name, m->flags,
                           m->point.x, m->point.y, m->point.z, m->normal.x, m->normal.y, m->normal.z, m->color.r, m->color.g, m->color.b, m->color.a, m->obj->nFace, m->obj->state, 10);
                }
            }
            tstInMirror = 1;
            gsSysMirror->Render(gsCameraPtr);
            tstInMirror = 0;
            rndSetTransforms(view, pr);
            rendDrv->camera = gsCameraPtr;
            objMODIFIER::SetViewPos(&eye);
        }
        float e[3] = { eye.x, eye.y, eye.z };
        tp[1] = platTime();
        tstDrawScene(s_lvl, e);
        tp[2] = platTime();
        tstDrawParticles(view);
        tp[3] = platTime();
        // the first-person buffer (as the render frame v12 0x0051BDDA): depth cleared, near 0.05, far 100
        rndClearDepth();
        zf = 100.0f;
        pr[10] = zf / (zf - zn);
        pr[14] = -zn * zf / (zf - zn);
        rndSetTransforms(view, pr);
        tp[4] = platTime();
        tstDrawLayer = 1;
        tstDrawScene(s_lvl, e);
        tstDrawLayer = 0;
        // PLACEHOLDER for gsRENDER_SYSTEM::RenderColorFullScr (v12 0x0051C160, not rebuilt): the full-screen colour
        // (SetColorFullScr: a cinema's fade, the damage flash) over the picture -- for the frame it was set in only
        // (RenderColorFullScr clears +0x30 after drawing; its timed flash, +0x24 / +0x28, is not used here)
        if (gsSysRender->fullScrSet) {
            float *c = gsSysRender->fullScrColor;
            if (c[3] > 0.5f)
                rndOverlay(((unsigned)c[3] << 24) | ((unsigned)c[0] << 16) | ((unsigned)c[1] << 8) | (unsigned)c[2]);
            gsSysRender->fullScrSet = 0;
        }
        tp[5] = platTime();
        tstHudDraw();
        tstDrawWatermark();
        tp[6] = platTime();
        rndEnd();
        tp[7] = platTime();
        static int nf;
        if (getenv("WR_RTIME") && ++nf % 60 == 0)
            printf("rtime: begin %.1f scene %.1f particles %.1f pickups %.1f layer1 %.1f hud %.1f end %.1f ms%c",
                   (tp[1] - tp[0]) * 1e3, (tp[2] - tp[1]) * 1e3, (tp[3] - tp[2]) * 1e3, (tp[4] - tp[3]) * 1e3,
                   (tp[5] - tp[4]) * 1e3, (tp[6] - tp[5]) * 1e3, (tp[7] - tp[6]) * 1e3, 10);
        return;
    }
    float zn = 0.05f, zf = 1000.0f, ys = 1.0f / tanf(0.5f * 70.0f * 3.14159265f / 180.0f), xs = ys * SCR_H / SCR_W;
    float pr[16] = { xs, 0, 0, 0, 0, ys, 0, 0, 0, 0, zf / (zf - zn), 1, 0, 0, -zn * zf / (zf - zn), 0 };
    memcpy(proj, pr, sizeof(pr));
    int under = (p->moveFlags & PLR_MF_WATER) != 0;
    rndBegin(view, proj, under ? 0xFF0E1E2C : 0xFF8FB4D8); // sky / the pool's [FOG] colour 14,30,44
    rndTris(s_vb, s_nvb / 3);
    drawWater(under);
    if (under)
        rndOverlay(0x900E1E2C);                         // under water: the fog colour over everything
    tstHudDraw();
    tstDrawWatermark();
    rndEnd();
}

static void status(plrPLAYER *p, char *b)
{
    m3dV pos, d;
    p->inst->GetPos(&pos);
    p->inst->GetDir(&d);
    sprintf(b, "pos %.2f %.2f %.2f  vel %.2f %.2f %.2f  dir %.2f %.2f %.2f  pitch %.1f  floorH %.2f  eye %.2f  mf %08X  [%s]",
            pos.x, pos.y, pos.z, p->vel.x, p->vel.y, p->vel.z, d.x, d.y, d.z, p->pitch, p->floorHeight, p->eyeHeight,
            p->moveFlags, rndDeviceName());
}

// one scripted run: place the player, then hold each step's key until its end time
struct tstSTEP { float until; int dik; float mdx, mdy; int dik2; }; // mdx/mdy: mouse per frame (gsINP units)
static void runScript(plrPLAYER *p, const char *name, m3dV *pos, m3dV *dir, const tstSTEP *st, int n, int shots)
{
    unsigned char keys[256];
    char line[256];
    int quiet = name[0] == '#';
    p->inst->SetPosDir_XZ(pos, dir);
    p->vel.x = p->vel.y = p->vel.z = 0;
    p->pitch = 90.0f;                                   // SetPosDir_XZ levels the view
    if (!quiet)
        printf("--- %s\n", name);
    float t = 0, nextLog = 0, end = st[n - 1].until, maxY = -1e9f;
    u32 seen = 0;
    int si = 0, shot = 0;
    while (t < end) {
        while (si < n - 1 && t >= st[si].until)
            si++;
        memset(keys, 0, sizeof(keys));
        if (st[si].dik)
            keys[st[si].dik] = 0x80;
        if (st[si].dik2)
            keys[st[si].dik2] = 0x80;
        setInput(keys, st[si].mdx, st[si].mdy);
        gsElapsedTime = 1.0f / 60.0f;
        p->ProcessInputMove();
        m3dV q;
        p->inst->GetPos(&q);
        if (q.y > maxY)
            maxY = q.y;
        seen |= p->moveFlags;
        if (!quiet && t >= nextLog) {
            status(p, line);
            printf("t=%.2f %s\n", t, line);
            nextLog += 0.25f;
        }
        render(p);
        if (s_lvl) tstAnimSndLog(), gsEndFrameInsts(1);                   // the frame end (gsRENDER_SYSTEM::EndFrame: anim sounds)
        if (shots && ((shot == 0 && t > 0.5f) || (shot == 1 && t > 3.0f) || (shot == 2 && t > end - 0.1f))) {
            char fn[64];
            sprintf(fn, "box_test_%d.ppm", shot);
            rndScreenshot(fn);
            shot++;
        }
        platPump();
        t += 1.0f / 60.0f;
        s_time += 1.0 / 60.0;
    }
    m3dV e;
    p->inst->GetPos(&e);
    printf("%-28s start %6.2f %5.2f %6.2f  end %6.2f %5.2f %6.2f  maxY %5.2f  flags seen %08X%s\n", name + quiet,
           pos->x, pos->y, pos->z, e.x, e.y, e.z, maxY, seen, (seen & PLR_MF_WATER) ? " (under water)" : "");
}

// One line per case: where the player ends up (always run: 10 units/s).
#define RUN(nm, px, py, pz, dx, dz, steps) do { m3dV P = { px, py, pz }, D = { dx, 0, dz };                 \
        runScript(p, nm, &P, &D, steps, sizeof(steps) / sizeof(steps[0]), 0); } while (0)
static void physicsCases(plrPLAYER *p)
{
    char nm[64];
    printf("--- physics cases\n");
    static const tstSTEP walk3[] = { { 0.5f, 0 }, { 3.5f, DIK_W } };
    static const tstSTEP walk15[] = { { 0.5f, 0 }, { 2.0f, DIK_W } };
    static const tstSTEP jumpRun[] = { { 0.3f, DIK_W }, { 0.4f, DIK_W, 0, 0, DIK_SPACE }, { 0.8f, DIK_W }, { 1.6f, 0 } };
    static const tstSTEP crouchWalk[] = { { 0.3f, DIK_C }, { 2.5f, DIK_W, 0, 0, DIK_C } };
    static const tstSTEP idle2[] = { { 2.0f, 0 } };
    static const tstSTEP swimUp[] = { { 1.0f, 0, 0, -1.5f }, { 3.0f, DIK_W } };   // look up, swim forward
    for (int k = 0; k < 6; k++) {
        sprintf(nm, "#slope %2.0f deg (top %.2f)", s_slopeDeg[k], 6.0f * tanf(s_slopeDeg[k] * 3.14159265f / 180.0f));
        RUN(nm, 14.0f, 0.0f, laneZ(k) + 1.5f, 1, 0, walk3);
    }
    for (int k = 0; k < 6; k++) {
        sprintf(nm, "#step %.1f", s_stepH[k]);
        RUN(nm, -35.0f, 0.0f, laneZ(k) + 1.5f, 1, 0, walk15);
    }
    for (int k = 0; k < 6; k++) {
        sprintf(nm, "#jump onto ledge %.1f", s_ledgeH[k]);
        RUN(nm, -21.5f, 0.0f, laneZ(k) + 1.5f, 1, 0, jumpRun);
    }
    for (int k = 1; k < 5; k++) {                       // stand still half way up (slide?)
        float h = 3.0f * tanf(s_slopeDeg[k] * 3.14159265f / 180.0f);
        sprintf(nm, "#idle on %2.0f deg slope", s_slopeDeg[k]);
        RUN(nm, 23.0f, h + 0.5f, laneZ(k) + 1.5f, 1, 0, idle2);
    }
    RUN("#tunnel standing", -3.0f, 0.0f, 17.75f, 1, 0, walk15);
    RUN("#tunnel crouched", -3.0f, 0.0f, 17.75f, 1, 0, crouchWalk);
    RUN("#gap 1.4 (body 1.6)", 14.0f, 0.0f, 26.0f, -1, 0, walk15);
    RUN("#gap 1.8", 14.0f, 0.0f, 32.0f, -1, 0, walk15);
    RUN("#round column, glancing", -6.6f, 0.0f, 20.0f, 0.15f, 1, walk15);
    RUN("#ridge top, standing", -8.0f, 3.0f, 33.0f, 0, 1, idle2);
    RUN("#gap jump 3", -9.0f, 1.0f, 37.5f, 1, 0, jumpRun);
    RUN("#gap jump 6", -2.0f, 1.0f, 37.5f, 1, 0, jumpRun);
    RUN("#pool: ramp, rim, in", 15.0f, 0.0f, -30.0f, 1, 0, walk3);
    RUN("#pool: in deep water", 31.0f, 2.0f, -30.0f, 1, 0, idle2);
    RUN("#pool: swim up and out", 31.0f, 1.0f, -30.0f, -1, 0, swimUp);
    RUN("#pool: shelf (wading)", 26.0f, 2.5f, -30.0f, 0, 1, idle2);
}

// ---------------------------------------------------------------------------------------------
static const char *argValue(int argc, char **argv, const char *key)
{
    for (int i = 1; i + 1 < argc; i++)
        if (!strcmp(argv[i], key))
            return argv[i + 1];
    return 0;
}

// (test) no --game / WR_GAME_DIR: the install folder found from where the exe is or was started (the folder itself
// or its bin, as the game runs from bin), the one holding database\ui.tpl; returns 0 if none
static const char *findGameDir()
{
    static char dir[MAX_PATH];
    char base[2][MAX_PATH], t[MAX_PATH];
    GetModuleFileNameA(0, base[0], MAX_PATH);
    if (char *s = strrchr(base[0], '\\'))
        *s = 0;
    GetCurrentDirectoryA(MAX_PATH, base[1]);
    for (int i = 0; i < 2; i++)
        for (int up = 0; up < 2; up++) {
            _snprintf(t, MAX_PATH - 1, up ? "%s\\.." : "%s", base[i]);
            GetFullPathNameA(t, MAX_PATH, dir, 0);
            _snprintf(t, MAX_PATH - 1, "%s\\database\\ui.tpl", dir);
            if (GetFileAttributesA(t) != INVALID_FILE_ATTRIBUTES)
                return dir;
        }
    return 0;
}

void tstSetCull(int on, int mode);                      // test_menu.cpp

// (test) free space on the drive the exe runs from, in MB (-1: unknown)
static int s_scriptedRun;                               // --levelshot: never stop on a message box
static long long tstDiskFreeMB()
{
    char path[MAX_PATH], root[4] = "C:\\";
    if (GetModuleFileNameA(0, path, MAX_PATH) && path[1] == ':')
        root[0] = path[0];
    ULARGE_INTEGER freeBytes;
    if (!GetDiskFreeSpaceExA(root, &freeBytes, 0, 0))
        return -1;
    return (long long)(freeBytes.QuadPart >> 20);
}
static long long tstDiskLowMB() { const char *e = getenv("WR_DISKLOWMB"); return e ? atoll(e) : 1024; }  // under this D3D fails
#define TST_DISK_LOW_MB tstDiskLowMB()

// (test) one line appended to crash_log.txt next to the exe
static void tstCrashLog(const char *line)
{
    char path[MAX_PATH];
    if (!GetModuleFileNameA(0, path, MAX_PATH))
        return;
    char *sl = strrchr(path, '\\');
    strcpy(sl ? sl + 1 : path, "crash_log.txt");
    if (FILE *f = fopen(path, "a")) {
        SYSTEMTIME t;
        GetLocalTime(&t);
        fprintf(f, "%04d-%02d-%02d %02d:%02d:%02d %s\n", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, line);
        fclose(f);
    }
}

// (test) the gameplay log: the last 300 events kept in memory, written to crash_gameplay.txt only on a crash that is
// not the full-disk one (then the disk is the cause, and there is no room to write it)
static char s_gameLog[300][200];
static int  s_gameLogN;
static double s_gameT0;
static void tstGameLog(const char *fmt, ...)
{
    char *d = s_gameLog[s_gameLogN++ % 300];
    int n = _snprintf(d, 16, "[%7.2f] ", s_gameT0 ? platTime() - s_gameT0 : 0.0);
    va_list a;
    va_start(a, fmt);
    _vsnprintf(d + n, 199 - n, fmt, a);
    va_end(a);
    d[199] = 0;
}
static void tstGameLogWrite(const char *why)
{
    char path[MAX_PATH];
    if (!GetModuleFileNameA(0, path, MAX_PATH))
        return;
    char *sl = strrchr(path, '\\');
    strcpy(sl ? sl + 1 : path, "crash_gameplay.txt");
    FILE *f = fopen(path, "w");
    if (!f)
        return;
    fprintf(f, "Will Rock decompilation build -- what happened before the crash (oldest first)\n%s\n\n", why);
    int first = s_gameLogN > 300 ? s_gameLogN - 300 : 0;
    for (int i = first; i < s_gameLogN; i++)
        fprintf(f, "%s\n", s_gameLog[i % 300]);
    fclose(f);
}


static void tstGameLogLevel(const char *path, int nObj)
{
    if (!s_gameT0)
        s_gameT0 = platTime();
    tstGameLog("level loaded: %s (%d objects)", path, nObj);
}
// once a second the player's state, and every change of weapon, life or health
static void tstGameLogPlayer(plrPLAYER *p)
{
    static double next;
    static int lastW = -1, lastDead = -1;
    static float lastHp = -1;
    float hp = *(float *)((char *)p + 0xD0);          // (entENTITY health, v12 +0xD0)
    int dead = wrEVCall<BOOL32>(p, 0x74) != 0;
    if (p->curWeapon != lastW)
        tstGameLog("weapon -> %d", p->curWeapon);
    if (lastDead != -1 && dead != lastDead)
        tstGameLog(dead ? "PLAYER DIED" : "player alive again");
    if (lastHp >= 0 && hp < lastHp - 0.5f)
        tstGameLog("player hit: %.0f -> %.0f", lastHp, hp);
    lastW = p->curWeapon;
    lastDead = dead;
    lastHp = hp;
    if (const char *tc = getenv("WR_TESTCRASH"))       // (test) WR_TESTCRASH=<seconds>: crash on purpose, for the logs
        if (s_gameT0 && platTime() - s_gameT0 > atof(tc))
            *(volatile int *)0 = 1;
    if (platTime() < next)
        return;
    next = platTime() + 1.0;
    m3dV q;
    p->inst->GetPos(&q);
    int nAi = 0;
    for (aiNPC *n = aiNPC::GetNext(0, 0); n && nAi < 999; n = aiNPC::GetNext(n, 0))
        nAi++;
    tstGameLog("player at %.1f %.1f %.1f hp %.0f weapon %d moveFlags %X | enemies alive %d | entities %d", q.x, q.y, q.z, hp,
               p->curWeapon, p->moveFlags, nAi, tstEntsCount());
}

// (test) "module+offset" of an address (any loaded dll or the exe)
static void tstAddrName(unsigned a, char *out, int n)
{
    MEMORY_BASIC_INFORMATION mi;
    char path[MAX_PATH];
    if (VirtualQuery((void *)a, &mi, sizeof(mi)) && mi.AllocationBase &&
        GetModuleFileNameA((HMODULE)mi.AllocationBase, path, MAX_PATH)) {
        const char *b = strrchr(path, '\\');
        _snprintf(out, n - 1, "%s+0x%X", b ? b + 1 : path, a - (unsigned)mi.AllocationBase);
    } else
        _snprintf(out, n - 1, "%08X", a);
    out[n - 1] = 0;
}

// (test) the advanced part: where (module+offset), the registers, the stack's return addresses into any module,
// and crash.dmp (a minidump for Visual Studio / WinDbg) next to the exe
static void tstCrashDetails(EXCEPTION_POINTERS *x)
{
    CONTEXT *c = x->ContextRecord;
    char nm[128], line[512];
    tstAddrName(c->Eip, nm, sizeof(nm));
    _snprintf(line, sizeof(line) - 1, "  at %s  eax %08lX ebx %08lX ecx %08lX edx %08lX esi %08lX edi %08lX ebp %08lX esp %08lX",
              nm, c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp);
    line[sizeof(line) - 1] = 0;
    tstCrashLog(line);
    printf("%s\n", line);
    unsigned *sp = (unsigned *)c->Esp;
    for (int i = 0, n = 0; i < 4096 && n < 32 && !IsBadReadPtr(sp + i, 4); i++) {
        MEMORY_BASIC_INFORMATION mi;
        if (!VirtualQuery((void *)sp[i], &mi, sizeof(mi)) || !(mi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE |
                                                                              PAGE_EXECUTE_READWRITE)))
            continue;
        tstAddrName(sp[i], nm, sizeof(nm));
        _snprintf(line, sizeof(line) - 1, "  stack +%03X: %s", i * 4, nm);
        line[sizeof(line) - 1] = 0;
        tstCrashLog(line);
        n++;
    }
    typedef BOOL(WINAPI * MDW)(HANDLE, DWORD, HANDLE, int, void *, void *, void *);
    HMODULE dh = LoadLibraryA("dbghelp.dll");
    MDW wr = dh ? (MDW)GetProcAddress(dh, "MiniDumpWriteDump") : 0;
    char path[MAX_PATH];
    if (wr && GetModuleFileNameA(0, path, MAX_PATH)) {
        char *sl = strrchr(path, '\\');
        strcpy(sl ? sl + 1 : path, "crash.dmp");
        HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
        if (f != INVALID_HANDLE_VALUE) {
            struct { DWORD tid; EXCEPTION_POINTERS *ep; BOOL client; } ei = { GetCurrentThreadId(), x, FALSE };
            BOOL ok = wr(GetCurrentProcess(), GetCurrentProcessId(), f, 0 /* MiniDumpNormal */, &ei, 0, 0);
            CloseHandle(f);
            tstCrashLog(ok ? "  minidump written: crash.dmp" : "  minidump failed (disk full?)");
        }
    }
}

// (test) a crash prints its code, address and the return addresses found on the stack (addr2line -f -C -e box_test.exe);
// it goes to crash_log.txt too, with the disk's free space: a full disk is told as the likely cause
static LONG WINAPI tstCrash(EXCEPTION_POINTERS *x)
{
    CONTEXT *c = x->ContextRecord;
    long long freeMB = tstDiskFreeMB();
    char line[256];
    _snprintf(line, sizeof(line) - 1, "CRASH %08lX at %08lX (data %08lX), disk free %lld MB%s",
              x->ExceptionRecord->ExceptionCode, c->Eip,
              x->ExceptionRecord->NumberParameters > 1 ? (unsigned long)x->ExceptionRecord->ExceptionInformation[1] : 0UL,
              freeMB, freeMB >= 0 && freeMB < TST_DISK_LOW_MB ? " -- DISK ALMOST FULL: most likely the cause" : "");
    line[sizeof(line) - 1] = 0;
    tstCrashLog(line);
    printf("%s\n", line);
    tstCrashDetails(x);
    if (!(freeMB >= 0 && freeMB < TST_DISK_LOW_MB))
        tstGameLogWrite(line);
    if (freeMB >= 0 && freeMB < TST_DISK_LOW_MB && !s_scriptedRun) {
        char msg[256];
        _snprintf(msg, sizeof(msg) - 1, "Game crashed: low on disk space.\n\n"
                  "Only %lld MB free on the game's drive. Direct3D and Windows need free space: "
                  "free a few GB and run it again.", freeMB);
        msg[sizeof(msg) - 1] = 0;
        MessageBoxA(0, msg, "Will Rock - disk full", MB_OK | MB_ICONERROR | MB_TOPMOST);
    }
    printf("CRASH %08lX at %08lX (data %08lX) esp %08lX%s", x->ExceptionRecord->ExceptionCode, c->Eip,
           x->ExceptionRecord->NumberParameters > 1 ? (unsigned long)x->ExceptionRecord->ExceptionInformation[1] : 0UL,
           c->Esp, "");
    HMODULE m = GetModuleHandleA(0);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((char *)m + ((IMAGE_DOS_HEADER *)m)->e_lfanew);
    unsigned lo = (unsigned)m + nt->OptionalHeader.BaseOfCode, hi = lo + nt->OptionalHeader.SizeOfCode;
    unsigned *sp = (unsigned *)c->Esp;
    printf(" base %08X (link 00400000) stack:", (unsigned)m);
    for (int i = 0, n = 0; i < 2048 && n < 24 && !IsBadReadPtr(sp + i, 4); i++)
        if (sp[i] >= lo && sp[i] < hi) {
            printf(" %08X", sp[i]);
            n++;
        }
    printf("\n");
    return EXCEPTION_EXECUTE_HANDLER;
}

int main(int argc, char **argv)
{
    setvbuf(stdout, 0, _IONBF, 0);                      // (test) the log survives a crash
    SetUnhandledExceptionFilter(tstCrash);
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--levelshot"))
            s_scriptedRun = 1;
    {
        long long freeMB = tstDiskFreeMB();             // (test) warned of at the start, before it crashes
        if (freeMB >= 0 && freeMB < TST_DISK_LOW_MB) {
            char line[160];
            _snprintf(line, sizeof(line) - 1, "WARNING: disk almost full (%lld MB free) -- the game may crash", freeMB);
            line[sizeof(line) - 1] = 0;
            printf("%s\n", line);
            tstCrashLog(line);
        }
    }
    int selftest = argc > 1 && !strcmp(argv[1], "--selftest");
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--noshovel"))
            s_wantShovel = 0;
    const char *gameDir = argValue(argc, argv, "--game");
    if (!gameDir)
        gameDir = getenv("WR_GAME_DIR");
    static char fullDir[MAX_PATH];
    if (gameDir && GetFullPathNameA(gameDir, MAX_PATH, fullDir, 0))   // (test) absolute: the start-up moves to bin
        gameDir = fullDir;
    if (!gameDir && (gameDir = findGameDir()) != 0)
        printf("game folder: %s\n", gameDir);
    if (gameDir)
        s_gameDir = gameDir;
    tstEntsGameDir = s_gameDir;                         // (the box level's player.tpl too, not only an .lg level's)
    if (const char *dump = argValue(argc, argv, "--dump-lg")) {      // --dump-lg <file.lg> [--depth n]: no window
        m3dInit();
        m3dSimdType = M3D_SIMD_NONE;
        objInit();
        tstTexInit(gameDir ? gameDir : ".");
        scnSCENE *s = tstLoadScene(dump);
        const char *d = argValue(argc, argv, "--depth");
        if (s)
            tstDumpScene(s, d ? atoi(d) : 3);
        return s ? 0 : 1;
    }
    if (argc > 1 && !strcmp(argv[1], "--msgtest"))     // (test) the rebuilt message system on msgSystem
        return tstMsgTest();
    if (const char *lg = argValue(argc, argv, "--checklevel")) {     // --checklevel <file.lg> [--obj a,b]: no window
        m3dInit();
        m3dSimdType = M3D_SIMD_NONE;
        objInit();
        tstTexInit(gameDir ? gameDir : ".");
        return tstCheckLevel(lg, argValue(argc, argv, "--obj"));
    }
    if (const char *dump = argValue(argc, argv, "--dump-tpl")) {     // --dump-tpl <file.tpl> [--depth n]: no window
        m3dInit();
        m3dSimdType = M3D_SIMD_NONE;
        objInit();
        if (!tstTexInit(gameDir ? gameDir : "."))
            printf("texture manager init failed\n");
        animTPL *t = tstLoadTemplate(dump);
        const char *d = argValue(argc, argv, "--depth");
        if (t)
            tstDumpObj(t->root, 0, d ? atoi(d) : 99);
        return t ? 0 : 1;
    }
    loadGameIni(gameDir, argc, argv);                            // [Video]: the display mode, as the game's video config
    if (s_iniPath[0] && !selftest && !argValue(argc, argv, "--levelshot") && !argValue(argc, argv, "--windowed")
        && !(argc > 1 && !strcmp(argv[argc - 1], "--windowed")) && s_allowFullScreen && iniBool("Video", "VIDEO_FullScr", 0)) {
        platFullScreen = 1;
        s_scrW = GetPrivateProfileIntA("Video", "VIDEO_SizeX", 1024, s_iniPath);
        s_scrH = GetPrivateProfileIntA("Video", "VIDEO_SizeY", 768, s_iniPath);
        platBPP = GetPrivateProfileIntA("Video", "VIDEO_BPP", 32, s_iniPath);
        platTripleBuf = iniBool("Video", "VIDEO_TripleBuf", 0);
        if (const char *fs = argValue(argc, argv, "--fullsize"))   // (test) another mode, e.g. 1234x777
            sscanf(fs, "%dx%d", &s_scrW, &s_scrH);
    }
    if (!platInit(SCR_W, SCR_H, "Will Rock reconstruction - box test", selftest))
        return 1;
    apHMainWindow = (HWND)platHwnd();                   // as gsWINDOWS::Create (the game window)
    if (argValue(argc, argv, "--sndtest") || (argc > 1 && !strcmp(argv[argc - 1], "--sndtest")))
        return tstSoundTest(gameDir ? gameDir : ".");
    s_scrW = rndWidth();                                // (test) full screen may have fallen back to a window
    s_scrH = rndHeight();
    platFullScreen = platIsFullScreen();
    m3dInit();                                          // as WinMain: SIMD detection, 2D/3D template matrices, noise
    m3dSimdType = M3D_SIMD_NONE;                        // the rebuilt m3d code is the x87 path
    objInit();
    cdtoInit();

    scnSCENE *scn = new scnSCENE;
    gsScenePtr = scn;
    objOBJ *root = objOBJ::Make();
    root->SetName((char *)"root");
    makeBox("ground", -60, -1, -60, 60, 0, 60, 0x4E7A44)->Attach(root);
    makeBox("box", -4, 0, -4, 4, 3, 4, 0xC0703A)->Attach(root);         // the box the player starts on
    makeBox("step1", 4, 0, -2, 6, 0.5f, 2, 0xB0B040)->Attach(root);
    makeBox("step2", 6, 0, -2, 8, 1.0f, 2, 0xA0A040)->Attach(root);
    makeBox("step3", 8, 0, -2, 10, 1.5f, 2, 0x909040)->Attach(root);
    makeRamp("ramp30", -16, -6, 0, 5.77f, 6, 10, 0x4070C0)->Attach(root); // 30 degrees: walkable
    makeRamp("ramp55", -16, -10, 0, 8.57f, -10, -6, 0x8040A0)->Attach(root); // 55 degrees: slides
    makeBox("wall", 12, 0, -12, 13, 4, 12, 0x808080)->Attach(root);
    makeBox("pillar", -2, 3, -2, -1, 5, -1, 0xD0D0D0)->Attach(root);     // on the box: push-out test
    buildTestArea(root);
    root->SetIDHier(0);
    objCalcLTM(root);
    objCalcBBox(root);
    for (int k = 0; k < s_nObj; k++) {                  // bounding spheres (the level loader reads them from the .lg)
        objOBJ *o = s_objs[k].o;
        m3dSPHERE *bs = (m3dSPHERE *)&o->geom->bsphere;
        bs->center.x = (o->bbox.min[0] + o->bbox.max[0]) * 0.5f;
        bs->center.y = (o->bbox.min[1] + o->bbox.max[1]) * 0.5f;
        bs->center.z = (o->bbox.min[2] + o->bbox.max[2]) * 0.5f;
        bs->radius = 0;
        objCalcBSphereCenter(o, bs);
    }
    scn->root = root;
    // the level collision grid: in the game it comes precomputed from the .lg (chunk 0x21F); here it is
    // computed like the tools do, then InitLevel runs again to make the instance grid over it
    if (!scn->InitLevel())
        return 2;
    cdtCOLL_SCN *coll = new cdtCOLL_SCN;
    coll->data = scn;
    if (!coll->CalcData2(4.0f, 4.0f, 4.0f, 60, 60, 60, 0))
        return 3;
    scn->coll = coll;
    if (!scn->InitLevel())
        return 4;
    buildVB();
    buildWater();

    // (test) the game systems the HUD reads that are not rebuilt: the game-mode object (mode 0, single player), as a
    // zeroed block of its v12 size
    tstPteInit();                                       // the shader manager (pteInit) and the fog class
    wrSysGO = (wrGO_SYSTEM *)apCalloc(1, 0x11C);
    wrSysGO->difficulty = 1;                            // PLACEHOLDER: [Multiplayer] Difficulty normal (ProcessINIT_GAME, not rebuilt)
    gsSysCinema = (cinSYSTEM *)apCalloc(1, 0x200);         // (test) the cut-scene system, idle (+0x80 bit 1 clear)
    gsSysInput = (gsINP_SYSTEM *)s_inpBuf;
    gsSysInput->actions = &s_actions;
    INP_VT(0x1C) = (void *)inpGetCtxByName;
    INP_VT(0x20) = (void *)inpSetDefCtx;
    *(void **)s_inpBuf = s_inpVtbl;
    gsSysRender = (gsRENDER_SYSTEM *)s_rendBuf;
    {   // PLACEHOLDER (test) the systems the mirrors call: RenderFrame above; gsSysVis (not rebuilt): everything visible;
        // vidDriver (not rebuilt): its depth clear, the modes as the test draws (0x4000: lightmaps)
        static void *rfVt[16], *visVt[64], *vidVt[16];
        static char visBuf[0x40], vidBuf[0x220], vidMode[0x40];
        rfVt[(0x1C + WR_MSGADDR_VTBL_SHIFT) / 4] = (void *)rfRenderFrame;
        *(void ***)s_rendBuf = rfVt;
        visVt[(0x80 + WR_MSGADDR_VTBL_SHIFT) / 4] = (void *)tstVisNothing;
        visVt[(0x84 + WR_MSGADDR_VTBL_SHIFT) / 4] = (void *)tstVisNothing;
        visVt[(0x8C + WR_MSGADDR_VTBL_SHIFT) / 4] = (void *)tstVisYes2;
        visVt[(0x90 + WR_MSGADDR_VTBL_SHIFT) / 4] = (void *)tstVisYes;
        visVt[(0x94 + WR_MSGADDR_VTBL_SHIFT) / 4] = (void *)tstVisYes;
        visVt[(0x98 + WR_MSGADDR_VTBL_SHIFT) / 4] = (void *)tstVisYes;
        *(void ***)visBuf = visVt;
        gsSysVis = (gsVIS_SYSTEM *)visBuf;
        vidVt[0x24 / 4] = (void *)tstVidClear;
        *(void ***)vidBuf = vidVt;
        *(char **)(vidBuf + 0x210) = vidMode;
        *(u32 *)(vidMode + 0x14) = 0x4000;
        vidDriver = (vidDRIVER *)vidBuf;
        gsSysMirror = new mrrMIRROR_SYSTEM;
        gsSysMirror->Init();
    }
    gsCameraPtr = new camCAMERA;                        // the game camera (the render system's, not rebuilt)
    gsCameraPtr->SetViewWindow(0, 0, (float)SCR_W, (float)SCR_H);
    // as gssWND_RENDER::ProcessAct (v12 0x0053E0B3, not rebuilt) sets up the game camera: height / width 0.75,
    // horizontal angle 75 degrees (not zoomed)
    gsCameraPtr->SetAspectRatioHW(gssCamAspectHW);
    gsCameraPtr->SetAngleHor(gssCamAngleHor);
    gscInit();                                          // the aim ray's record
    // PLACEHOLDER for the render system's video mode set-up (not rebuilt): the screen size gs_scr2d works with
    gssScreenDX = SCR_W;
    gssScreenDY = SCR_H;
    // until the UI is up (tstHudDraw runs the real wrCURSORS::Init): the crosshair locked at the centre, no textures
    gscCursor.Lock(0.5f, 0.5f);
    static wrCURSORS cursors;
    wrCURSORS::pInstance = _wrCursors = &cursors;
    m3dV spawn = { 0, 3.0f, 0 };
    if (!txmManager && !selftest) {                     // player.tpl's textures (the box level has none of its own)
        tstHaveDevice = 1;
        tstTexInit(s_gameDir);
    }
    if (!aiAttackPlanner)                               // (box level: the HUD's boss bar reads it; no AI runs here)
        aiPlannerInit();
    plrPLAYER *p = makePlayer(scn, &spawn);
    if (selftest) {                                     // diagnostics of the collision set-up
        m3dV pp;
        p->inst->GetPos(&pp);
        printf("spawn %.2f %.2f %.2f  nObj %d  grid %d %d %d  vox %.2f %.2f %.2f  cellObj %p\n", pp.x, pp.y, pp.z,
               scn->nObj, coll->grid.nx, coll->grid.ny, coll->grid.nz, coll->grid.voxSize.x, coll->grid.voxSize.y,
               coll->grid.voxSize.z, (void *)coll->cellObj);
        cdtREFINE r(0, 0);
        cdtINFO ci;
        m3dV org = { 0.5f, 10, 0.5f }, dn = { 0, -1, 0 };
        int hit = coll->ShootRay(&org, &dn, 100.0f, 0, &ci);
        printf("coll ShootRay down from (0.5,10,0.5): %d dist %.3f obj %d face %d\n", hit, ci.dist, ci.objId, ci.faceId);
        m3dV fo;
        hit = scn->FloorPlace(&org, 100.0f, &fo, 0, 0, 0, &r, &ci);
        printf("scene FloorPlace: %d -> %.3f %.3f %.3f\n", hit, fo.x, fo.y, fo.z);
    }

    unsigned char keys[256];
    char line[256];
    if (selftest) {
        // scripted runs at a fixed 1/60 s step. Will Rock's world is right-handed: facing +z, +x is on the left.
        static const tstSTEP box[] = {      // settle, walk off the box, jump, crouch, strafe right
            { 1.0f, 0 }, { 2.5f, DIK_W }, { 2.6f, DIK_SPACE }, { 3.5f, DIK_W }, { 4.0f, DIK_C }, { 5.0f, DIK_D }, { 6.0f, 0 } };
        static const tstSTEP steps[] = {    // up the three steps (0.5 each), down and into the wall at x 12
            { 3.0f, DIK_W }, { 3.5f, 0 } };
        static const tstSTEP ramp30[] = {   // up the 30 degree ramp, off its top
            { 3.0f, DIK_W }, { 4.0f, 0 } };
        static const tstSTEP ramp55[] = {   // try to walk up the 55 degree ramp, then let go
            { 2.0f, DIK_W }, { 4.0f, 0 } };
        static const tstSTEP look[] = {     // mouse: turn right, look up, walk forward
            { 1.0f, 0, 2.0f, 0 }, { 1.5f, 0, 0, -2.0f }, { 2.5f, DIK_W, 0, 0 } };
        static const tstSTEP speed[] = {    // always run: W alone runs (10), W + Shift walks (5), W + crouch (5)
            { 1.0f, DIK_W }, { 2.0f, DIK_W, 0, 0, DIK_LSHIFT }, { 3.0f, DIK_W, 0, 0, DIK_C }, { 3.5f, 0 } };
        m3dV px = { 1, 0, 0 }, pz = { 0, 0, 1 };
        m3dV s4 = { -45, 0, -40 };
        m3dV s0 = { 0, 3, 0 }, s1 = { 4.5f, 0.5f, 0 }, s2 = { -19, 0, 8 }, s3 = { -19, 0, -8 };
        runScript(p, "box", &s0, &pz, box, sizeof(box) / sizeof(box[0]), 1);
        runScript(p, "steps", &s1, &px, steps, sizeof(steps) / sizeof(steps[0]), 0);
        runScript(p, "ramp30", &s2, &px, ramp30, sizeof(ramp30) / sizeof(ramp30[0]), 0);
        runScript(p, "ramp55", &s3, &px, ramp55, sizeof(ramp55) / sizeof(ramp55[0]), 0);
        runScript(p, "look", &s0, &pz, look, sizeof(look) / sizeof(look[0]), 0);
        runScript(p, "speed", &s4, &pz, speed, sizeof(speed) / sizeof(speed[0]), 0);
        physicsCases(p);
        {                                               // realistic mouse: ~10 counts per frame at MouseSens 1
            float k = 10 * 1.0f * 0.025659f;
            tstSTEP mouse[] = { { 0.5f, 0, k, 0 }, { 0.8f, 0, 0, k }, { 1.4f, 0, 0, -k }, { 1.9f, 0, -k, 0 }, { 2.2f, 0 } };
            runScript(p, "mouse right, down, up, left", &s4, &pz, mouse, sizeof(mouse) / sizeof(mouse[0]), 0);
        }
        fflush(stdout);
        platTerm();
        return 0;
    }
    loadGameIni(gameDir, argc, argv);
    if (!s_alwaysRun)
        p->cfg.bits &= ~1u;
    // the front end: the game's main menu (ui.tpl / ui.lg) with the test's DEBUG entry; --nomenu skips it
    int menu = !argValue(argc, argv, "--nomenu") && !(argc > 1 && !strcmp(argv[argc - 1], "--nomenu"));
    if (menu) {
        tstHaveDevice = 1;
        tstMenuShot = argValue(argc, argv, "--menushot");
        tstMenuShotFrame = argValue(argc, argv, "--menuframe");
        if (!tstTexInit(gameDir ? gameDir : ".") || !tstMenuInit(gameDir ? gameDir : "."))
            printf("menu data not found (pass --game <install dir>): starting the level\n");
        else {
            int r = tstMenuRun(0);
            menuMusicOff();
            if (r == TST_MENU_QUIT) {
                platTerm();
                return 0;
            }
            if (r == TST_MENU_PLAYLG)
                if (plrPLAYER *q = startLevel(tstMenuLevelPath))
                    p = q;
        }
    }
    plrPLAYER *boxPlayer = p;
    if (const char *lg = argValue(argc, argv, "--level"))    // --level <file.lg>: straight into a level
        if (plrPLAYER *q = startLevel(lg)) {
            p = q;
            // (test) WR_RELOADN=<n> [WR_RELOAD2=<other.lg>]: n more loads (alternating with the other level) --
            // the memory line after each shows whether a level change leaves anything behind
            int n = getenv("WR_RELOADN") ? atoi(getenv("WR_RELOADN")) : 0;
            for (int i = 0; i < n; i++)
                if (plrPLAYER *r = startLevel((i & 1) || !getenv("WR_RELOAD2") ? lg : getenv("WR_RELOAD2")))
                    p = r;
        }
    if (argValue(argc, argv, "--domtest") || (argc > 1 && !strcmp(argv[argc - 1], "--domtest")))
        tstEntsToDomain(p);
    int cinTest = argValue(argc, argv, "--cintest") || (argc > 1 && !strcmp(argv[argc - 1], "--cintest"));
    if (cinTest)                                        // (test) into an action domain (10_4: the cinema's trigger)
        tstEntsToActDomain(p, argValue(argc, argv, "--cintest"));
    if (const char *gp = getenv("WR_GOTOPOS")) {        // (test) WR_GOTOPOS="x y z": the player there
        m3dV pos, dir = { 0, 0, 1 };
        if (sscanf(gp, "%f %f %f", &pos.x, &pos.y, &pos.z) == 3)
            wrEVCall<void>(p, 0x70, &pos, &dir, 1);
    }
    for (int i = 1; i < argc; i++)
        if (!strcmp(argv[i], "--alphahud"))             // (test, asked for) the early build's HUD: tstHudDraw
            _putenv((char *)"WR_ALPHAHUD=1");
    if (const char *g = argValue(argc, argv, "--gotoobj")) {    // (test) the middle of a scene object's box
        if (objOBJ *o = objFindName(s_lvl->root, (char *)g)) {
            m3dV c = { 0, 0, 0 };
            float *lt = o->ltm;
            for (int i = 0; i < o->nVert; i++) {
                m3dV *v = &o->geom->vert[i];
                c.x += ((o->state & 1) ? v->x : v->x * lt[0] + v->y * lt[4] + v->z * lt[8] + lt[12]) / o->nVert;
                c.y += ((o->state & 1) ? v->y : v->x * lt[1] + v->y * lt[5] + v->z * lt[9] + lt[13]) / o->nVert;
                c.z += ((o->state & 1) ? v->z : v->x * lt[2] + v->y * lt[6] + v->z * lt[10] + lt[14]) / o->nVert;
            }
            printf("gotoobj %s: %.2f %.2f %.2f%c", g, c.x, c.y, c.z, 10);
            p->inst->SetPos(&c);
        }
    }
    if (const char *g = argValue(argc, argv, "--gotodom")) {
        void tstEntsToSpawnDomain(entENTITY *player, const char *name);
        tstEntsToSpawnDomain(p, g);
    }
    if (const char *g = argValue(argc, argv, "--goto"))  // (test) --goto <instance>: 3 m in front of it, facing it
        if (s_lvl)
            if (animINST *a = s_lvl->FindInstName((char *)g)) {
                m3dV pos = { a->matr[12] + 3.0f * a->matr[8], a->matr[13] + 0.5f, a->matr[14] + 3.0f * a->matr[10] };
                m3dV dir = { -a->matr[8], 0, -a->matr[10] };
                wrEVCall<void>(p, 0x70, &pos, &dir, 1);
                printf("goto %s: at %.2f %.2f %.2f\n", g, pos.x, pos.y, pos.z);
            }
    if (argValue(argc, argv, "--tpttest") || (argc > 1 && !strcmp(argv[argc - 1], "--tpttest"))) { // (on the floor above the domain: SetPosDir)
        tstEntsToTpt(p);
        m3dV pos, dir = { p->inst->matr[8], 0, p->inst->matr[10] };
        p->inst->GetPos(&pos);
        wrEVCall<void>(p, 0x70, &pos, &dir, 1);
    }
    if (const char *c = argValue(argc, argv, "--cull"))  // (test) none / cw / ccw: the side the view keeps
        tstSetCull(strcmp(c, "none") != 0, !strcmp(c, "ccw") ? 3 : 2);
    if (argValue(argc, argv, "--watertest") || (argc > 1 && !strcmp(argv[argc - 1], "--watertest")))
        tstEntsToWater(p);
    if (const char *shot = argValue(argc, argv, "--levelshot")) {  // (test) 1 s settle, 3 s walk, two pictures
        char path[0x200];
        if (!menu && gameDir && !txmManager)            // (the box level loads no textures: the UI needs the manager)
            tstTexInit(gameDir);
        if (!menu && gameDir && !tstMenuInit(gameDir))  // the UI data (fonts, ui.tpl) for the HUD
            printf("no UI data: no HUD\n");
        if (const char *ls = argValue(argc, argv, "--loadshot")) {  // (test) the loading screen of this level
            tstShowLoading(argValue(argc, argv, "--level"));
            rndScreenshot(ls);
        }
        float t = 0, walkEnd = 4.0f;
        if (const char *w = argValue(argc, argv, "--walktime"))   // (test) a longer walk: doors open and close
            walkEnd = (float)atof(w);
        while (t < walkEnd) {
            memset(keys, 0, sizeof(keys));
            static int swDone;                          // (test) WR_SWITCHAT=<t>: at t s the level WR_RELOAD2 (a level change mid-play)
            if (getenv("WR_SWITCHAT") && getenv("WR_RELOAD2") && t >= (float)atof(getenv("WR_SWITCHAT")) && !swDone++)
                if (plrPLAYER *q = startLevel(getenv("WR_RELOAD2")))
                    p = q;
            static int bhDone; if (getenv("WR_BLOODHANDS") && t >= 2.0f && !bhDone++) {  // (test) blood 1 m beside the player at 2 s
            m3dV b = { p->inst->matr[12] + 1.0f, p->inst->matr[13], p->inst->matr[14] };
            p->NotifySFXBloodHands(&b);
            printf("bloodhands: notified, shader on %d\n", p->bloodHands[0] ? (int)(*(u32 *)((char *)p->bloodHands[0] + 4) & 1) : -1);
        }
        if (getenv("WR_GOD"))                           // (test) WR_GOD=1: god mode (no damage taken)
                p->moveFlags |= PLR_MF_GOD;
            if (t > 1.0f && !getenv("WR_STILL"))           // (test) WR_STILL=1: the player stands
                keys[DIK_W] = 0x80;
            if (gsSysCinema && (gsSysCinema->state & 1) && getenv("WR_CINSKIP")) {   // (test) WR_CINSKIP=1: a playing cinema
                static int sk;                                                     //   skipped (the skip key, a frame on, a frame off)
                keys[DIK_SPACE] = (++sk & 1) ? 0x80 : 0;
            }
            static int turnFr;                                  // (test) --turntest: stand, 90 degrees every 0.5 s, a picture of each
            int turnTest = argValue(argc, argv, "--turntest") || (argc > 1 && !strcmp(argv[argc - 1], "--turntest"));
            if (turnTest) {
                keys[DIK_W] = 0;
                if (++turnFr % 30 == 0) {
                    m3dV o = { p->inst->matr[12], p->inst->matr[13], p->inst->matr[14] };
                    m3dV d = { p->inst->matr[10], 0, -p->inst->matr[8] };
                    p->inst->SetPosDir_XZ(&o, &d);
                }
            }
            if (const char *pw = getenv("WR_PWRTEST")) {        // (test) power-up n (0 titan damage, 1 armor, 2 titan
                int fr = (int)(t * 60.0f + 0.5f);               //   motion) held and switched on at 1 s
                if (fr == 60) {
                    int i = atoi(pw);
                    p->powerup[i].state = PLR_PWR_HELD;
                    p->ActivatePowerup(i);
                    printf("pwrtest: power-up %d on (state %d)%c", i, p->powerup[i].state, 10);
                }
                if (fr == 120)
                    printf("pwrtest: frame time %.4f (scale %.2f), power-up left %.1f%c", gsElapsedTime, gsTimeScale, p->powerup[atoi(pw)].timeLeft, 10);
            }
            if (const char *wc = getenv("WR_GOTOWAYPOINT")) {    // (test) WR_GOTOWAYPOINT=<class>: once an NPC of that
                static int wpDone;                              //   class lives, the player on its way point (AI_PROP
                if (!wpDone) {                                  //   actorWayPoint; none: its goal), facing it
                    for (aiNPC *a = aiNPC::GetNext(0, 1); a; a = aiNPC::GetNext(a, 1)) {
                        const char *cn = a->inst->className ? a->inst->className : "";
                        if (strnicmp(cn, wc, strlen(wc)))
                            continue;
                        m3dV wp = a->wayPointPos;
                        if (wp.x == 0 && wp.y == 0 && wp.z == 0)
                            wp = a->goalPos;
                        m3dV np = { a->inst->matr[12], a->inst->matr[13], a->inst->matr[14] };
                        m3dV dir = { np.x - wp.x, 0, np.z - wp.z };
                        float l = sqrtf(dir.x * dir.x + dir.z * dir.z);
                        if (l > 0.001f) {
                            dir.x /= l;
                            dir.z /= l;
                        } else {
                            dir.z = 1.0f;
                        }
                        m3dV pos = { wp.x, wp.y + 0.5f, wp.z };
                        wrEVCall<void>(p, 0x70, &pos, &dir, 1);
                        printf("gotowaypoint %s: way point %.2f %.2f %.2f (goal %.2f %.2f %.2f), it at %.2f %.2f %.2f, %.1f m%c",
                               cn, a->wayPointPos.x, a->wayPointPos.y, a->wayPointPos.z, a->goalPos.x, a->goalPos.y,
                               a->goalPos.z, np.x, np.y, np.z, l, 10);
                        wpDone = 1;
                        break;
                    }
                }
            }
            if (const char *kn = getenv("WR_KILLNPC")) {         // (test) WR_KILLNPC="t[,step]": at t s (then every step s)
                static float next = -1;                         //   a lethal shotgun hit on the nearest live NPC
                float t0 = 0, st = 0;
                sscanf(kn, "%f,%f", &t0, &st);
                if (next < 0)
                    next = t0;
                if (t >= next) {
                    next = st > 0 ? next + st : 1e9f;
                    m3dV o = { p->inst->matr[12], p->inst->matr[13], p->inst->matr[14] };
                    aiNPC *best = 0;
                    float bd = 1e9f;
                    for (aiNPC *a = aiNPC::GetNext(0, 1); a; a = aiNPC::GetNext(a, 1)) {
                        float dx = a->inst->matr[12] - o.x, dz = a->inst->matr[14] - o.z;
                        const char *ks = getenv("WR_KILLSKIP");  // (test) WR_KILLSKIP=<class>: spared
                        if (ks && a->inst->className && !strnicmp(a->inst->className, ks, strlen(ks)))
                            continue;
                        if (!(a->aiFlags & 0x600000) && dx * dx + dz * dz < bd) {
                            bd = dx * dx + dz * dz;
                            best = a;
                        }
                    }
                    if (best) {
                        dmgWEAPON d;
                        d.type = 1;
                        d.subtype = 0;
                        d.amount = 1000.0f;
                        d.sourceId = p->mpId;
                        best->inst->GetPos(&d.hitPos);
                        d.hitPos.y += 1.0f;
                        d.weaponType = 2;
                        d.origin = o;
                        printf("killnpc: %s at %.1f %.1f %.1f%c", best->inst->name ? best->inst->name : "?", d.hitPos.x,
                               d.hitPos.y, d.hitPos.z, 10);
                        msgSystem.SendMsg(best, 0x4C6, &d, 0);
                    }
                }
            }
            if (getenv("WR_SHOOTTEST")) {                       // (test) the colt, a shot every 0.5 s at the nearest NPC
                int fr = (int)(t * 60.0f + 0.5f);
                if (getenv("WR_FLAREDUMP") && p->weapons[p->curWeapon]) {   // (test) the muzzle flare's state
                    int wt = *(int *)((char *)p->weapons[p->curWeapon] + 0x98);
                    entENTITY *fl = (wt == 1 || wt == 2 || wt == 3) ? ((pwpBULLET_WPN *)p->weapons[p->curWeapon])->flare : 0;
                    if (fl && fl->inst) {
                        animINST *fi = fl->inst;
                        m3dV fp;
                        fi->GetPos(&fp);
                        printf("flare: fr %d inst flags %X seq %d time %.3f pos %.2f %.2f %.2f", fr, fi->flags, fi->seq, fi->seqTime, fp.x, fp.y, fp.z);
                        objOBJ *o = fi->root, *f0 = o ? o->child : 0;
                        if (o)
                            printf(" | %s st %X rs %X ps %X", o->name ? o->name : "?", o->state, o->rendState, o->procState);
                        int more = 0;
                        for (objOBJ *c = f0; c && (!more || c != f0); c = c->next, more = 1)
                            printf(" | %s st %X rs %X ps %X col %08X sc %.3f", c->name ? c->name : "?", c->state, c->rendState,
                                   c->procState, *(u32 *)((char *)c + 0x5E), c->ltm[0]);
                        printf("%c", 10);
                    }
                }
                if (fr >= 18 && fr < 21 && !getenv("WR_STARTWPN"))   // (WR_STARTWPN: keep that weapon)
                    keys[DIK_2] = 0x80;
                float h0 = 1.5f, h1 = 5.0f;                         // (held: 1.5..5 s, or WR_HOLDFIRE="a,b" seconds)
                if (const char *hf = getenv("WR_HOLDFIRE"))
                    sscanf(hf, "%f,%f", &h0, &h1);
                s_mouseShoot = getenv("WR_HOLDFIRE") ? fr >= (int)(h0 * 60) && fr < (int)(h1 * 60) : fr >= 90 && fr % 30 < 3;
                if (getenv("WR_HOLDFIRE") && fr % 10 == 0 && p->weapons[p->curWeapon])
                    printf("holdfire: fr %d shoot %d wpn %d seq %d clip %d ammo %.0f moveFlags %X ready %d/%d cur %d wflags %X%c", fr, s_mouseShoot,
                           p->curWeapon, p->weapons[p->curWeapon]->inst->seq, p->weapons[p->curWeapon]->clip,
                           p->weapons[p->curWeapon]->ammo, p->moveFlags, p->IsReadyShoot(),
                           wrEVCall<int>(p->weapons[p->curWeapon], 0x78), wrEVCall<int>(p->weapons[p->curWeapon], 0xBC),
                           p->weapons[p->curWeapon]->flags, 10);
                keys[DIK_W] = 0;
                m3dV o = { p->inst->matr[12], p->inst->matr[13], p->inst->matr[14] };
                aiNPC *best = 0;
                float bd = 1e9f;
                for (aiNPC *a = aiNPC::GetNext(0, 1); a; a = aiNPC::GetNext(a, 1)) {
                    float dx = a->inst->matr[12] - o.x, dz = a->inst->matr[14] - o.z;
                    float dd = dx * dx + dz * dz;
                    if (!(a->aiFlags & 0x200000) && dd < bd) {
                        bd = dd;
                        best = a;
                    }
                }
                if (best) {
                    m3dV d = { best->inst->matr[12] - o.x, 0, best->inst->matr[14] - o.z };
                    p->inst->SetPosDir_XZ(&o, &d);
                }
                if (const char *vn = getenv("WR_RAYTO")) {      // (test) a ray from the eye to an instance: what it hits
                    static int rf;
                    animINST *ta = gsScenePtr->FindInstName((char *)vn);
                    if (ta && ++rf % 30 == 0) {
                        m3dV e = { o.x, o.y + p->eyeHeight, o.z }, c;
                        ta->GetPos(&c);
                        m3dV d = { c.x - e.x, c.y + 0.4f - e.y, c.z - e.z };
                        float len = m3dSqrt(d.x * d.x + d.y * d.y + d.z * d.z);
                        m3dNormalize(&d);
                        cdtINFO ci;
                        memset(&ci, 0, sizeof(ci));
                        entCDT_REFINE_EXCL rr(1, p->inst, 1); int hit = gsScenePtr->ShootRay(&e, &d, len + 3.0f, 0, &rr, &ci);
                        if (objOBJ *ln = objFindName(ta->root, (char *)"lift_null"))
                            printf("lift: seq %d frame %.1f null y %.2f player y %.2f%c", ta->seq, ta->frame, ln->ltm[13], o.y, 10);
                        printf("rayto %s: bv %p useEncl %d unkBC %X at %.2f %.2f %.2f col %08X flags %X hit %d inst %s dist %.2f of %.2f%c", vn, ta->bvCur, rr.IsUseBVolEncl(ta), ta->ent ? ta->ent->unkBC : 0, c.x, c.y, c.z, ta->ent ? ta->ent->colAmbient : 0,
                               ta->flags, hit, ci.inst && ci.inst->name ? ci.inst->name : "-", ci.dist, len, 10);
                    }
                }
            }
            if (const char *sw = argValue(argc, argv, "--swingtest")) {  // (test) a swing every 0.5 s
                int fr = (int)(t * 60.0f + 0.5f);
                s_mouseShoot = fr > 120 && fr % 30 < 3;
                if (s_mouseShoot && p->weapons[p->curWeapon])   // (test) why a press does or does not fire
                    printf("swingtest: fire fr %d ready %d entFlags %X wflags %X ammo %.0f seq %d mf %08X%c", fr, p->IsReadyShoot(), (unsigned)p->weapons[p->curWeapon]->entFlags,
                           p->weapons[p->curWeapon]->wpnWEAPON::flags, p->weapons[p->curWeapon]->ammo,
                           p->weapons[p->curWeapon]->GetCurAnimSeq(), p->moveFlags, 10);
                int ws = atoi(sw) > 1 && atoi(sw) < PLR_WPN_COUNT ? atoi(sw) : PLR_WPN_SVL;   // (N > 1: weapon slot N)
                if (fr >= 60 && fr < 120 && p->curWeapon != ws && p->weapons[ws])   // (the shovel -- or slot N -- in
                                                        //  the hand first, once the switch is taken)
                    p->SetCurWpn(ws, 1);
                if (fr % (getenv("WR_WPNFRAME") ? 3 : 15) == 0)
                    printf("swingtest: t=%.2f mf %08X weapon %d seq %d clip %d ammo %.0f%c", t, p->moveFlags, p->curWeapon,
                           p->weapons[p->curWeapon] ? p->weapons[p->curWeapon]->GetCurAnimSeq() : -1,
                           p->weapons[p->curWeapon] ? p->weapons[p->curWeapon]->clip : -1,
                           p->weapons[p->curWeapon] ? p->weapons[p->curWeapon]->ammo : -1.0f, 10);
                if (getenv("WR_WPNFRAME") && p->weapons[p->curWeapon]) {   // (test) the held weapon's clip position
                    animINST *wi = p->weapons[p->curWeapon]->inst;
                    printf("  wpn frame: inst seq %d time %.3f frame %.2f actbits %X%c", wi->seq, wi->seqTime,
                           wi->seq >= 0 ? wi->tpl->seq[wi->seq].TimeToFrame(wi->seqTime) : -1.0f,
                           (unsigned)wi->actionFrames, 10);
                }
            }
            if (argValue(argc, argv, "--colttest") || (argc > 1 && !strcmp(argv[argc - 1], "--colttest"))) {
                // (test) the colt (key 2) at 0.3 s, a shot every 0.5 s from 1.5 s, the shells logged
                int fr = (int)(t * 60.0f + 0.5f);
                if (fr >= 18 && fr < 21)
                    keys[DIK_2] = 0x80;
                s_mouseShoot = fr >= 90 && fr % 30 < 3;
                keys[DIK_W] = 0;
                if (fr >= 88 && fr <= 112 && p->weapons[1])  // (test) the frames the colt plays round a shot
                    printf("coltanim: fr %d seq %d frame %.2f seqTime %.3f\n", fr, p->weapons[1]->inst->seq,
                           p->weapons[1]->inst->frame, p->weapons[1]->inst->seqTime);
                if (fr % 15 == 0) {
                    pwpWEAPON *w = p->curWeapon >= 0 && p->curWeapon < 11 ? p->weapons[p->curWeapon] : 0;
                    partEMITTER_PHYS *sh = w && p->curWeapon == 1 ? ((pwpBULLET_WPN *)w)->shells : 0;
                    printf("colttest: t=%.2f wpn %d seq %d clip %d shells %d", t, p->curWeapon,
                           w ? w->GetCurAnimSeq() : -1, w ? w->clip : -1, sh ? sh->nPart : -1);
                    if (sh && sh->nPart)
                        for (int i = 0; i < sh->nPart && i < 3; i++) {
                            partPART *q = sh->partPtrs[(sh->first + i) % sh->maxPart];
                            printf(" [%.2f %.2f %.2f age %.2f hit %d]", q->pos.x, q->pos.y, q->pos.z, q->age, q->nHit);
                        }
                    printf("  emitters %d\n", partGetNEmit());
                }
            }
            if (const char *av = getenv("WR_ALTARVIEW")) {   // (test) x m in front of the altar, looking at it; its parts
                static int avFr;
                animINST *ai = s_lvl ? s_lvl->FindInstName((char *)"Actor_altar") : 0;
                keys[DIK_W] = 0;
                if (ai && ++avFr == 20) {
                    m3dV o, d;
                    ai->GetPos(&o);
                    float k = (float)atof(av);
                    d.x = 1; d.y = 0; d.z = 0;
                    m3dV at2 = { o.x - k, o.y + 1.0f, o.z };
                    p->inst->SetPosDir_XZ(&at2, &d);
                    p->pitch = 90.0f;
                    printf("altarview: altar at %.1f %.1f %.1f flags %X seq %d nSeq %d%c", o.x, o.y, o.z, ai->flags, ai->seq,
                           ai->tpl ? ai->tpl->nSeq : -1, 10);
                    for (int oi = 0; oi < ai->nObj; oi++) {
                        objOBJ *q = ai->objTable[oi];
                        m3dV c;
                        q->GetOrigin(&c);
                        txmTEXTURE *tx = q->nFace > 0 ? q->GetFaceTexMtlNmb(0, 0) : 0;
                        float lo[3] = { 1e9f, 1e9f, 1e9f }, hi[3] = { -1e9f, -1e9f, -1e9f };
                        for (int vi = 0; q->geom && q->geom->vert && vi < q->nVert; vi++)
                            for (int ax = 0; ax < 3; ax++) {
                                m3dV *vv = &q->geom->vert[vi];
                                float w = vv->x * q->ltm[ax] + vv->y * q->ltm[4 + ax] + vv->z * q->ltm[8 + ax] + q->ltm[12 + ax];
                                if (w < lo[ax]) lo[ax] = w;
                                if (w > hi[ax]) hi[ax] = w;
                            }
                        if (q->nFace > 0)
                            printf("altarview: size %s: %.2f x %.2f x %.2f, y %.2f..%.2f, ltm scale %.3f %.3f %.3f%c", q->name ? q->name : "?", hi[0] - lo[0],
                                   hi[1] - lo[1], hi[2] - lo[2], lo[1], hi[1], sqrtf(q->ltm[0] * q->ltm[0] + q->ltm[1] * q->ltm[1] + q->ltm[2] * q->ltm[2]),
                                   sqrtf(q->ltm[4] * q->ltm[4] + q->ltm[5] * q->ltm[5] + q->ltm[6] * q->ltm[6]),
                                   sqrtf(q->ltm[8] * q->ltm[8] + q->ltm[9] * q->ltm[9] + q->ltm[10] * q->ltm[10]), 10);
                        printf("altarview: obj %d %s state %X proc %X rend %X faces %d tex %s%c", oi, q->name ? q->name : "?", q->state,
                               q->procState, q->rendState, q->nFace, tx ? tx->name : "-", 10);
                    }
                    for (int ki = 0; ki < 1000; ki++) {         // who is in the way: anything around the altar
                        animINST *ia = s_lvl->InstPtr(ki);
                        if (ia->magic != 0x41494E53 || !ia->root || ia == ai)
                            continue;
                        for (int oi = 0; oi < ia->nObj; oi++) {
                            objOBJ *q = ia->objTable[oi];
                            if (!q->nFace || !q->geom || !q->geom->vert)
                                continue;
                            float lo[3] = { 1e9f, 1e9f, 1e9f }, hi[3] = { -1e9f, -1e9f, -1e9f };
                            for (int vi = 0; vi < q->nVert; vi++)
                                for (int ax = 0; ax < 3; ax++) {
                                    m3dV *vv = &q->geom->vert[vi];
                                    float w = vv->x * q->ltm[ax] + vv->y * q->ltm[4 + ax] + vv->z * q->ltm[8 + ax] + q->ltm[12 + ax];
                                    if (w < lo[ax]) lo[ax] = w;
                                    if (w > hi[ax]) hi[ax] = w;
                                }
                            if (lo[0] < o.x + 4 && hi[0] > o.x - 4 && lo[2] < o.z + 4 && hi[2] > o.z - 4 && lo[1] < o.y + 10 && hi[1] > o.y) {
                                txmTEXTURE *tx = q->GetFaceTexMtlNmb(0, 0);
                                printf("altarnear: inst %s class %s obj %s proc %X faces %d tex %s size %.1f %.1f %.1f y %.1f..%.1f%c", ia->name ? ia->name : "?",
                                       ia->className ? ia->className : "-", q->name ? q->name : "?", q->procState, q->nFace, tx ? tx->name : "-",
                                       hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2], lo[1], hi[1], 10);
                            }
                        }
                    }
                    wrsobTRA_ALTAR *al = (wrsobTRA_ALTAR *)ai->ent;
                    if (al && al->emitter)
                        printf("altarview: emitter flags %X state %X nArea %d pos %.1f %.1f %.1f attach %p%c", al->emitter->flags,
                               al->emitter->state, al->emitter->nArea, al->emitter->pos.x, al->emitter->pos.y, al->emitter->pos.z,
                               (void *)al->emitter->attachInst, 10);
                }
            }
            if (const char *at = getenv("WR_ALTARTEST")) {   // (test) buy powerup button <name> at 1 s, with 1000 gold
                static int atFr;
                atFr++;
                animINST *bi = s_lvl ? s_lvl->FindInstName((char *)at) : 0;
                wrsobTRA_BUTTON *b = bi ? (wrsobTRA_BUTTON *)bi->ent : 0;
                keys[DIK_W] = 0;
                if (b && atFr == 30) {
                    m3dV o, d, a;
                    b->faceObj->GetOrigin(&o);
                    b->playerPos->GetOrigin(&a);
                    b->playerPos->GetAxisZ(&d);
                    p->inst->SetPosDir_XZ(&a, &d);
                    p->money = 1000.0f;
                    printf("altartest: at %.1f %.1f %.1f, button at %.1f %.1f %.1f, powerup %d type %d gold %.0f altar %s%c", a.x, a.y, a.z,
                           o.x, o.y, o.z, b->powerup, b->unk98, p->money,
                           b->altarInst && b->altarInst->name ? b->altarInst->name : "NONE", 10);
                }
                if (b && atFr == 60) {
                    p->ProcessUse();
                    printf("altartest: USE -> altar state %d buyer %p moveFlags %08X camera %d%c",
                           ((wrsobTRA_ALTAR *)b->altarInst->ent)->state, (void *)((wrsobTRA_ALTAR *)b->altarInst->ent)->buyer,
                           p->moveFlags, p->camCtrl->attached, 10);
                }
                if (b && atFr == 660) {                         // (the powerup used: slot's key, ActivatePowerup)
                    p->ActivatePowerup(b->powerup);
                    float h0 = p->health;
                    dmgDAMAGE dm(0);
                    dm.amount = 30.0f;
                    dm.sourceId = (unsigned)-1;
                    msgSystem.SendMsg(p, 0x4C6, &dm, 0);
                    printf("altartest: powerup used -> state %d left %.1f; hit 30: health %.0f -> %.0f; weapon damage x%.0f; time scale %.2f%c", p->powerup[b->powerup].state,
                           p->powerup[b->powerup].timeLeft, h0, p->health, p->weapons[p->curWeapon]->GetDamage() / (p->weapons[p->curWeapon]->damage > 0 ? p->weapons[p->curWeapon]->damage : 1.0f),
                           gsMsgGetElapsedTimeScale(), 10);
                }
                if (b && atFr > 60 && atFr % 60 == 0) {
                    wrsobTRA_ALTAR *al = (wrsobTRA_ALTAR *)b->altarInst->ent;
                    if (al->sfx) {
                        wrsfxTRADING_ALTAR *fx = (wrsfxTRADING_ALTAR *)al->sfx;
                        partEMITTER_PHYS *es[4] = { fx->glow, fx->trail[0].emitter, fx->trail[1].emitter, fx->trail[2].emitter };
                        for (int ei = 0; ei < 4; ei++) {
                            partEMITTER_PHYS *e = es[ei];
                            float mx = 0, sum = 0;
                            for (int pi = 0; pi < e->nPart; pi++) {
                                partPART *q = e->partPtrs[(e->first + pi) % e->maxPart];
                                e->PartUpdateVisible(1, &q);
                                if (q->size > mx) mx = q->size;
                                sum += q->size;
                            }
                            printf("altarpart: %d n %d max %.2f avg %.2f rate %.0f state %X pos %.1f %.1f %.1f tex %s%c", ei, e->nPart, mx, e->nPart ? sum / e->nPart : 0,
                                   e->emitRate, e->state, e->pos.x, e->pos.y, e->pos.z, e->texture ? e->texture->name : "-", 10);
                        }
                    }
                    if (al->emitter)
                        printf("altaridle: intensity %.1f far %.2f,%.2f max %d n %d state %X flags %X rate %.0f pos %.1f %.1f %.1f tex %s size %.2f color %08X%c", al->emitter->intensity, al->emitter->degFarK[0], al->emitter->degFarK[1], al->emitter->maxPart, al->emitter->nPart, al->emitter->state, al->emitter->flags,
                               al->emitter->emitRate, al->emitter->pos.x, al->emitter->pos.y, al->emitter->pos.z, al->emitter->texture ? al->emitter->texture->name : "-",
                               al->emitter->nPart ? al->emitter->partPtrs[al->emitter->first]->size : 0, al->emitter->nPart ? al->emitter->partPtrs[al->emitter->first]->color : 0, 10);
                    static int sfxDumped;
                    if (al->sfx && !sfxDumped++) {
                        animINST *ia = al->sfx->inst;
                        printf("altarsfx: scale? seq %d nSeq %d frame %.0f flags %X%c", ia->seq, ia->tpl ? ia->tpl->nSeq : -1, ia->frame, ia->flags, 10);
                        for (int oi = 0; oi < ia->nObj; oi++) {
                            objOBJ *q = ia->objTable[oi];
                            txmTEXTURE *tx = q->nFace > 0 ? q->GetFaceTexMtlNmb(0, 0) : 0;
                            float lo[3] = { 1e9f, 1e9f, 1e9f }, hi[3] = { -1e9f, -1e9f, -1e9f };
                            for (int vi = 0; q->geom && q->geom->vert && vi < q->nVert; vi++)
                                for (int ax = 0; ax < 3; ax++) {
                                    m3dV *vv = &q->geom->vert[vi];
                                    float w = vv->x * q->ltm[ax] + vv->y * q->ltm[4 + ax] + vv->z * q->ltm[8 + ax] + q->ltm[12 + ax];
                                    if (w < lo[ax]) lo[ax] = w;
                                    if (w > hi[ax]) hi[ax] = w;
                                }
                            printf("altarsfx: %s proc %X rend %X faces %d tex %s const %08X size %.1f %.1f %.1f%c", q->name ? q->name : "?", q->procState, q->rendState,
                                   q->nFace, tx ? tx->name : "-", q->geom ? q->geom->constColor : 0, hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2], 10);
                        }
                    }
                    printf("altartest: t=%.2f state %d time %.2f sfx %p camera %d powerup held %d,%d,%d gold %.0f locked %d%c", t, al->state, al->time,
                           (void *)al->sfx, p->camCtrl->attached, p->powerup[0].state, p->powerup[1].state, p->powerup[2].state, p->money, b->locked, 10);
                }
            }
            if (const char *kt = getenv("WR_KATTEST")) {  // (test) as played: to the button, USE (charge), 5 s, onto the
                static int ktFr;                          //   cup, 1 s, USE (throw). WR_KATOFS: x,y,z from the cup's origin
                ktFr++;
                char bn[0x48];
                _snprintf(bn, sizeof(bn) - 1, "%s_button", kt);
                animINST *ki = s_lvl ? s_lvl->FindInstName((char *)kt) : 0;
                animINST *bi = s_lvl ? s_lvl->FindInstName(bn) : 0;
                wrsobKATAPULT *k = ki ? (wrsobKATAPULT *)ki->ent : 0;
                keys[DIK_W] = 0;
                if (k && bi && ktFr == 30) {                    // to the button, facing it
                    m3dV bp, kp, d;
                    bi->GetPos(&bp);
                    ki->GetPos(&kp);
                    d.x = bp.x - kp.x; d.y = 0; d.z = bp.z - kp.z;
                    m3dNormalize(&d);
                    m3dV at = { bp.x - d.x * 1.5f, bp.y, bp.z - d.z * 1.5f };
                    p->inst->SetPosDir_XZ(&at, &d);
                    p->vel.x = p->vel.y = p->vel.z = 0;
                    p->pitch = 90.0f;
                    printf("kattest: to the button at %.1f %.1f %.1f%c", at.x, at.y, at.z, 10);
                }
                if (k && bi && bi->ent && (ktFr == 60 || ktFr == 420)) {
                    wrEVCall<void>(bi->ent, 0x64, (void *)p);   // Use (as plrPLAYER::ProcessUse)
                    printf("kattest: t=%.2f USE -> seq %d frame %.0f%c", t, ki->seq, ki->frame, 10);
                }
                if (k && k->cup && ktFr == 360) {               // on top of the cup, as high as its box
                    m3dV o, d = { 0, 0, 1 };
                    (objFindName(ki->root, (char *)"throw") ? objFindName(ki->root, (char *)"throw") : k->cup)->GetOrigin(&o);
                    float ox = 0, oy = 1.0f, oz = 0;
                    if (getenv("WR_KATOFS"))
                        sscanf(getenv("WR_KATOFS"), "%f,%f,%f", &ox, &oy, &oz);
                    o.x += ox; o.y += oy; o.z += oz;
                    p->inst->SetPosDir_XZ(&o, &d);
                    p->vel.x = p->vel.y = p->vel.z = 0;
                    printf("kattest: onto the cup at %.1f %.1f %.1f, target %.1f %.1f %.1f height %.0f%c", o.x, o.y, o.z,
                           k->target.x, k->target.y, k->target.z, k->height, 10);
                }
                if (k && ktFr >= 345 && ktFr % 15 == 0) {
                    m3dV pp;
                    p->inst->GetPos(&pp);
                    printf("kattest: t=%.2f player %.1f %.1f %.1f vel %.1f %.1f %.1f seq %d frame %.0f stayon %d floor %s obj %d (cup %d) h %.2f%c",
                           t, pp.x, pp.y, pp.z, p->vel.x, p->vel.y, p->vel.z, ki->seq, ki->frame,
                           wrEVCall<BOOL32>(p, 0xA0, ki, k->cup, 0), p->floorInst && p->floorInst->name ? p->floorInst->name : "-",
                           p->unk_247, k->cup->idHier, p->floorHeight, 10);
                }
            }
            if (const char *ml = getenv("WR_MSGSHOW")) {  // (test) at 1 s the IAction a level's ON_* sends: "msg", SHOW, s1
                static int mlFr;
                if (++mlFr == 60) {
                    static u8 ia[0xA0];
                    ((apNAME *)(ia + 8))->SetName((char *)"SHOW");
                    strcpy((char *)ia + 0x60, ml);
                    msgSystem.SendMsg(wrMsgSystem, 0x3F8, ia, 0);
                    printf("msgshow: %s, sound linked %d, playing %d%c", ml, wrMsgDescr.data != 0,
                           wrMsgChannel->IsPlay(&wrMsgDescr), 10);
                }
            }
            if (getenv("WR_MUSLOG") && gsSysSound && gsSysSound->musDomains) {  // (test) the music's tempo when it changes
                static char lastMus[0x40];
                gsDOMAIN_LIST_MUS *ml = (gsDOMAIN_LIST_MUS *)gsSysSound->musDomains;
                if (strcmp(lastMus, ml->curName.str)) {
                    strcpy(lastMus, ml->curName.str);
                    printf("music: t=%.2f tempo track %s (soft %s, hard %s, fight %s), sndSystem %p%c", t, ml->curName.str,
                           ml->soft.name, ml->hard.name, ml->fight.name, (void *)sndSystem, 10);
                }
            }
            if (const char *tour = getenv("WR_TOUR")) {   // (test) WR_TOUR=a.lg,b.lg,..: the trigger held, the weapons in
                static int tourFr, tourAt;               //   turn, the next level every WR_TOURT frames (crashes on a
                static const int wkeys[] = { DIK_1, DIK_2, DIK_3, DIK_4, DIK_5, DIK_6, DIK_7, DIK_8, DIK_9 };  // change)
                int every = getenv("WR_TOURT") ? atoi(getenv("WR_TOURT")) : 180;
                s_mouseShoot = 1;
                keys[wkeys[(tourFr / 30) % 9]] = (tourFr % 30) < 2 ? 0x80 : 0;
                if (++tourFr % every == 0) {
                    char name[260];
                    const char *e = tour;
                    for (int i = 0; i < tourAt && e; i++) {
                        e = strchr(e, ',');
                        if (e)
                            e++;
                    }
                    if (!e) {
                        tourAt = 0;
                        e = tour;
                    }
                    const char *c = strchr(e, ',');
                    int n = c ? (int)(c - e) : (int)strlen(e);
                    tourAt++;
                    memcpy(name, e, n);
                    name[n] = 0;
                    printf("tour: to %s (frame %d, weapon %d, %s)%c", name, tourFr, p->curWeapon, "firing", 10);
                    if (plrPLAYER *q = startLevel(name))
                        p = q;
                    t = 1.0f;
                }
            }
            if (const char *ds = argValue(argc, argv, "--deathtest")) {  // (test) killed at 0.5 s by a damage message
                int fr = (int)(t * 60.0f + 0.5f);              //   (0x4C6, the world's, subtype ds), revived at 6 s
                if (fr == 30) {
                    dmgDAMAGE d(0);
                    d.subtype = (u8)atoi(ds);
                    d.amount = 1000.0f;
                    d.sourceId = (unsigned)-1;
                    msgSystem.SendMsg(p, 0x4C6, &d, 0);
                    printf("deathtest: hit 1000 (subtype %d) -> health %.1f dead %d (mpid %d flags 0x%X found %d "
                           "clients %d)%c", d.subtype, p->health, wrEVCall<BOOL32>(p, 0x74), p->mpId, p->entFlags,
                           gsSysMP->FindMPID(p->mpId, 2) == (entIACTIVE_OBJ *)p, gsSysServer->nClients, 10);
                }
                if (fr > 30 && fr % 30 == 0 && fr <= 390) {
                    m3dV o;
                    gsCameraPtr->GetOrigin(&o);
                    printf("deathtest: t=%.2f dead %d camera y %.2f deathTime %.2f%c", t, wrEVCall<BOOL32>(p, 0x74), o.y,
                           p->camCtrl->deathTime, 10);
                }
                if (fr == 360) {                               // what USE does while dead (playerFrame)
                    p->RevitalizeStart(0);
                    m3dV o;
                    p->inst->GetPos(&o);
                    printf("deathtest: respawned -> health %.1f dead %d at %.1f %.1f %.1f%c", p->health,
                           wrEVCall<BOOL32>(p, 0x74), o.x, o.y, o.z, 10);
                }
                keys[DIK_W] = 0;
            }
            if (const char *fh = argValue(argc, argv, "--falltest")) {  // (test) dropped from fh metres at 0.5 s
                int fr = (int)(t * 60.0f + 0.5f);
                if (fr == 30) {
                    m3dV fp;
                    p->inst->GetPos(&fp);
                    fp.y += (float)atof(fh);
                    p->inst->SetPos(&fp);
                    p->vel.x = p->vel.y = p->vel.z = 0;
                    printf("falltest: up %.1f m, health %.1f armor %.1f\n", (float)atof(fh), p->health, p->armor);
                }
                if (fr > 30 && fr % 15 == 0) {
                    m3dV fp;
                    p->inst->GetPos(&fp);
                    printf("falltest: t=%.2f y %.2f vel.y %.2f health %.1f\n", t, fp.y, p->vel.y, p->health);
                }
                keys[DIK_W] = 0;
            }
            {   // (test) --colttest: after the reload (from 10 s) the view turns right and down
                int fr = (int)(t * 60.0f + 0.5f);
                int turn = (argValue(argc, argv, "--colttest") || !strcmp(argv[argc - 1], "--colttest")) && fr >= (getenv("WR_TURNAT") ? atoi(getenv("WR_TURNAT")) : 600);
                float sweep = 0;                    // (test) --usemsg: from 1.5 s the view turns left, then right
                if ((argValue(argc, argv, "--usemsg") || !strcmp(argv[argc - 1], "--usemsg")) && fr >= 90 && fr < 210)
                    sweep = fr < 150 ? -4.0f : 4.0f;
                setInput(keys, turn ? 6.0f : sweep, turn && fr < (getenv("WR_TURNAT") ? atoi(getenv("WR_TURNAT")) : 600) + 40 ? 4.0f : 0.0f);
            }
            gsElapsedTime = tstFrameTime(1.0f / 60.0f);
            if (!lcPaused()) {
                playerFrame(p);
                if (s_lvl)
                    tstEntsFrame();
                if (wrSysStat)
                    wrSysStat->ProcessFRAME();
            }
            {
                gsLOAD_SCENE_PARAM rq;
                if (s_lvl)
                    p = levelChangeFrame(p);
            }
            int fr = (int)(t * 60.0f + 0.5f);
            if (s_lvl && !strcmp(argv[argc - 1], "--picktest")) {     // (test) the key: its locks (6 vases), shown
                static float keyLogT;
                if (t >= keyLogT)
                    if (animINST *k = s_lvl->FindInstName((char *)"Actor_key")) {
                        keyLogT = t + 0.5f;
                        printf("picktest: t=%.2f key locks %d flags %X%c", t, k->ent ? k->ent->nLocks : -99, k->flags, 10);
                    }
            }
            if (s_lvl && (argValue(argc, argv, "--picktest") || !strcmp(argv[argc - 1], "--picktest")) && fr >= 30 &&
                fr % 20 == 0) {                     // (test) --picktest: the vases broken, then to the next item
                if (fr == 30 && getenv("WR_HEALTH"))   // (test) hurt first, so health packs are taken
                    { p->health = (float)atof(getenv("WR_HEALTH")); printf("picktest: health set %.0f at t=%.2f%c", p->health, t, 10); }
                if (fr == 40)                       //   every 1/3 s
                    tstBreakVases();
                for (int i = 0; i < tstPickupCount(); i++) {
                    m3dV c;
                    static int next;                    // (one after another: one not taken is passed by)
                    int k = (next + i) % (tstPickupCount() ? tstPickupCount() : 1);
                    if (tstPickupBox(k, &c)) {
                        m3dV d = { 0, 0, 1 };
                        p->inst->SetPosDir_XZ(&c, &d);
                        next = k + 1;
                        break;
                    }
                }
            }
            if (const char *tf = getenv("WR_TPTFIRE")) {  // (test) WR_TPTFIRE=domain[,sec]: a teleport domain
                static int tfDone;                                //   unlocked and activated (default at 2 s)
                char b[96];
                strncpy(b, tf, 95);
                b[95] = 0;
                char *c = strchr(b, ',');
                float at = 2.0f;
                if (c) { *c = 0; at = (float)atof(c + 1); }
                if (!tfDone && t >= at && domTptList) {
                    tfDone = 1;
                    for (int i = 0; i < domTptList->nDomains; i++) {
                        gsDOMAIN *d = domTptList->domain[i];
                        if (!d->obj || !d->obj->name || stricmp(d->obj->name, b))
                            continue;
                        d->nLocks = 0;
                        d->domFlags |= 0x10000;                   // (DOM_ACTIVATED, dom_misc.cpp)
                        printf("tptfire: %s (flags %X)%c", b, d->domFlags, 10);
                    }
                }
            }
            if (const char *ia = getenv("WR_IACTION")) {  // (test) WR_IACTION=obj,cmd[,s1[,sec]]: a script command
                static int iaDone;                                //   to a named entity (default at 2 s)
                char b[160], *f[4] = { 0, 0, (char *)"", (char *)"2" };
                strncpy(b, ia, 159);
                b[159] = 0;
                int nf = 0;
                for (char *q = strtok(b, ","); q && nf < 4; q = strtok(0, ","))
                    f[nf++] = q;
                if (!iaDone && nf >= 2 && t >= (float)atof(f[3])) {
                    iaDone = 1;
                    entIACTION_DATA d;
                    memset(&d, 0, sizeof(d));
                    entIACTION_Init(&d);
                    d.cmd.SetName(f[1]);
                    strncpy(d.s1, f[2], 0x3F);
                    entENTITY *e;
                    for (e = entFindNext(0); e; e = entFindNext(e))
                        if (e->name.IsName(f[0]))
                            break;
                    printf("iaction: %s %s %s -> %s%c", f[0], f[1], f[2], e ? "sent" : "no such entity", 10);
                    if (e)
                        msgSystem.SendMsg(e, 0x3F8, &d, e);
                }
            }
            if (getenv("WR_STARTWPN") && fr == 20) {     // (test) WR_STARTWPN=n: that weapon in the hand
                int w = atoi(getenv("WR_STARTWPN"));
                if (w >= 0 && w < PLR_WPN_COUNT && p->weapons[w]) {
                    p->weapons[w]->Enable(1);
                    p->SetCurWpn(w, 1);
                }
            }
            if ((argValue(argc, argv, "--wpntest") || !strcmp(argv[argc - 1], "--wpntest")) && fr >= 30) {
                // (test) --wpntest: every created weapon in turn (2 s each, walking the second second), its sequence
                static int wi = -1, at = 0;
                if (fr >= at) {
                    int left = PLR_WPN_COUNT;
                    do
                        wi = (wi + 1) % PLR_WPN_COUNT;
                    while (!p->weapons[wi] && left-- > 0);
                    if (p->weapons[wi]->ammo <= 0.0f)
                        p->weapons[wi]->SetAmmo(1.0f);
                    p->weapons[wi]->Enable(1);
                    p->SetCurWpn(wi, 1);
                    at = fr + 120;
                    printf("wpntest switch to %d (%p) -> cur %d next %d%c", wi, (void *)p->weapons[wi], p->curWeapon, p->nextWeapon, 10);
                }
                if (fr % 15 == 0)
                    printf("wpntest t=%.2f weapon %d cur %d seq %d (%s) shown %d%c", t, wi, p->curWeapon,
                           p->weapons[p->curWeapon]->inst->seq,
                           p->weapons[p->curWeapon]->inst->tpl->seq[p->weapons[p->curWeapon]->inst->seq].name,
                           !(p->weapons[p->curWeapon]->inst->flags & 1), 10);
                if (fr % 120 >= 60)
                    keys[DIK_W] = 0x80;
            }
            if (s_lvl && argValue(argc, argv, "--tramptest") && fr >= 30 && fr % 10 == 0)
                for (int i = 0; i < tstEntsCount(); i++)  // (test) stand on the first trampoline, log the throw
                    if (!stricmp(tstEnt(i)->inst->className, "trampoline")) {
                        trmpTRAMPOLINE *tr = (trmpTRAMPOLINE *)tstEnt(i);
                        if (fr == 30) {
                            m3dV tp;
                            tr->inst->GetPos(&tp);
                            tp.y += 0.3f;
                            p->inst->SetPos(&tp);
                            p->vel.x = p->vel.y = p->vel.z = 0;
                            // TEMPORARY: the player cannot stand on the pad yet (not in the instance collision grid),
                            // so the launch IsStayOn would allow is done directly: GetSpeedThrow + ApplySpeed(set)
                            m3dV lv;
                            tr->GetSpeedThrow(p->GetGravity(), &lv);
                            p->ApplySpeed(&lv, 1);
                            printf("tramptest: launch velocity %.2f %.2f %.2f\n", lv.x, lv.y, lv.z);
                            cdtREFINE rf(0, 0);
                            scnAOBJ al[20];
                            int na = s_lvl->MakeAnimList(&tp, &tp, 2.0f, &rf, al, 20);
                            for (int q = 0; q < na; q++)
                                printf("tramptest: near the pad: %s flags %X\n", al[q].inst->name ? al[q].inst->name : "?",
                                       al[q].inst->flags);
                            printf("tramptest: pad inst flags %X, %d near\n", tr->inst->flags, na);
                            printf("tramptest: %s height %.2f target %d (%.1f %.1f %.1f) speedXZ %.2f randShift %.2f\n",
                                   tr->name.str, tr->height, tr->hasTarget, tr->targetPos.x, tr->targetPos.y,
                                   tr->targetPos.z, tr->speedXZ, tr->randShift);
                        }
                        m3dV pp;
                        p->inst->GetPos(&pp);
                        printf("tramptest: t=%.2f y %.2f vel.y %.2f floor %s\n", t, pp.y, p->vel.y,
                               p->floorInst == tr->inst ? "pad" : p->floorInst ? "other" : "none");
                        break;
                    }
            if (argValue(argc, argv, "--colortest") && fr % 30 == 0) {   // (test) the held weapon's light
                pwpWEAPON *w = p->weapons[p->curWeapon];
                m3dV pp;
                p->inst->GetPos(&pp);
                if (w && w->inst && w->inst->root)
                    printf("colortest: t=%.2f pos %.1f %.1f %.1f floor %s diffuse %08X ambient %08X const %08X\n", t, pp.x,
                           pp.y, pp.z, p->floorInfo.inst ? "inst" : (p->floorInfo.flags & 2) ? "level" : "-", w->colDiffuse,
                           w->colAmbient, tstFirstConstColor(w->inst->root));
            }
            if (argValue(argc, argv, "--crosstest") && fr % 30 == 0) {   // (test) the crosshair's state
                cdtINFO *ci = gscCursor.aimInfo;
                printf("crosstest: t=%.2f at %.0f %.0f cursor %d color %08X scale %.2f hit %d dist %.2f inst %s ent %s\n", t,
                       gscCursor.x, gscCursor.y, gscCursor.cursor, gscCursor.color, gscCursor.scale, (ci->flags & 2) != 0,
                       ci->dist, ci->inst && ci->inst->name ? ci->inst->name : "-",
                       gscCursor.aimEnt && gscCursor.aimEnt->inst->className ? gscCursor.aimEnt->inst->className : "-");
            }
            if (s_lvl && (argValue(argc, argv, "--usemsg") || !strcmp(argv[argc - 1], "--usemsg"))) {
                // (test) stand 1.5 m in front of the first button, facing it: the use tip (NotifyMsgUse)
                keys[DIK_W] = 0;
                if (fr == 10)
                    for (int i = 0; i < tstEntsCount(); i++)
                        if (!stricmp(tstEnt(i)->inst->className, "button")) {
                            m3dV bp, at, d = { 0, 0, 1 };
                            tstEnt(i)->inst->GetPos(&bp);
                            at.x = bp.x; at.y = bp.y - 1.0f; at.z = bp.z - 1.5f;
                            p->inst->SetPosDir_XZ(&at, &d);
                            p->vel.x = p->vel.y = p->vel.z = 0;
                            printf("usemsg: at %s, IsCanUse %d\n", tstEnt(i)->name.str, p->IsCanUse(tstEnt(i)->inst, 0));
                            {
                                plrCDT_REFINE_BUTTON ref(p->inst);
                                scnAOBJ list[20];
                                int n = gsScenePtr->MakeAnimList(&at, &at, 10.0f, &ref, list, 20);
                                printf("usemsg: MakeAnimList %d", n);
                                for (int k = 0; k < n; k++)
                                    printf(" [%s ent %p 98=%d]", list[k].inst->name ? list[k].inst->name : "",
                                           (void *)list[k].inst->ent,
                                           list[k].inst->ent ? list[k].inst->ent->unk98 : -9);
                                printf("  button inst %p flags %X\n", (void *)tstEnt(i)->inst, tstEnt(i)->inst->flags);
                                {
                                    m3dBOX bb;
                                    tstEnt(i)->inst->GetBBox(&bb);
                                    gsScenePtr->collAnim->MakeIndList(&bb, cdtOflPtr, 0);
                                    int in = 0;
                                    for (int q = 0; q < cdtOflPtr->nObj; q++)
                                        if (cdtOflPtr->obj[q] == tstEnt(i)->inst->index)
                                            in = 1;
                                    printf("usemsg: grid in %d (%d ids) box %.1f %.1f %.1f .. %.1f %.1f %.1f classId %08X at %.1f %.1f %.1f\n",
                                           in, cdtOflPtr->nObj, bb.vMin.x, bb.vMin.y, bb.vMin.z, bb.vMax.x, bb.vMax.y,
                                           bb.vMax.z, tstEnt(i)->classId, at.x, at.y, at.z);
                                }
                            }
                            break;
                        }
            }
            if (s_lvl && argValue(argc, argv, "--usetest") && fr == 10)
                for (int i = 0; i < tstEntsCount(); i++)  // (test) plrPLAYER::IsCanUse in front of / away from it
                    if (!stricmp(tstEnt(i)->inst->className, "button")) {
                        animINST *bi = tstEnt(i)->inst;
                        m3dV bp, at, d = { 0, 0, 1 }, back = { 0, 0, -1 };
                        bi->GetPos(&bp);
                        at.x = bp.x; at.y = bp.y - 1.0f; at.z = bp.z - 1.5f;
                        float dist = -1;
                        p->inst->SetPosDir_XZ(&at, &d);
                        int front = p->IsCanUse(bi, &dist);
                        p->inst->SetPosDir_XZ(&at, &back);
                        int away = p->IsCanUse(bi, 0);
                        at.y = bp.y + 6.0f;
                        p->inst->SetPosDir_XZ(&at, &d);
                        int high = p->IsCanUse(bi, 0);
                        p->moveFlags |= PLR_MF_KEY;
                        int k1 = p->HasKey();
                        p->UseKey();
                        printf("usetest: IsCanUse %s front %d (dist %.2f) away %d too high %d; HasKey %d after UseKey %d\n",
                               tstEnt(i)->name.str, front, dist, away, high, k1, p->HasKey());
                        break;
                    }
            if (s_lvl && argValue(argc, argv, "--usetest") && (fr == 30 || fr == 150))
                for (int i = 0; i < tstEntsCount(); i++)  // (test) press / release the first button
                    if (!stricmp(tstEnt(i)->inst->className, "button")) {
                        bttnBUTTON *b = (bttnBUTTON *)tstEnt(i);
                        printf("usetest: %s flags %X seq %d -> toggle %d", b->name.str, b->flags, b->inst->seq,
                               b->Toggle(p));
                        printf(" -> flags %X seq %d\n", b->flags, b->inst->seq);
                        break;
                    }
            if (s_lvl && argValue(argc, argv, "--usetest") && (fr == 120 || fr == 240))
                for (int i = 0; i < tstEntsCount(); i++)
                    if (!stricmp(tstEnt(i)->inst->className, "button")) {
                        printf("usetest: %s now flags %X seq %d time %.2f\n", tstEnt(i)->name.str,
                               ((bttnBUTTON *)tstEnt(i))->flags, tstEnt(i)->inst->seq, tstEnt(i)->inst->seqTime);
                        break;
                    }
            if (fr % 30 == 0 && getenv("WR_TRACK"))     // (test) the position / sequence of the named entities
                for (int i = 0; i < tstEntsCount(); i++)
                    if (strstr(tstEnt(i)->name.str, getenv("WR_TRACK"))) {
                        m3dV tp;
                        tstEnt(i)->inst->GetPos(&tp);
                        printf("track t=%.2f %s at %.2f %.2f %.2f seq %d time %.2f\n", t, tstEnt(i)->name.str, tp.x, tp.y, tp.z,
                               tstEnt(i)->inst->seq, tstEnt(i)->inst->seqTime);
                        if (objOBJ *ch = tstEnt(i)->inst->root ? tstEnt(i)->inst->root->child : 0)
                            printf("  child x axis %.2f %.2f %.2f  y %.2f\n", ch->local[0], ch->local[1], ch->local[2], ch->local[13]);
                        m3dBOX tb;
                        tstEnt(i)->inst->GetBBox(&tb);
                        printf("      box y %.2f .. %.2f\n", tb.vMin.y, tb.vMax.y);
                    }
            if (getenv("WR_PLRSND")) {                  // (test) what the player's three voices start playing
                static sncSOUND_DESCR *last[3];
                for (int k = 0; k < 3; k++) {
                    sncCHANNEL *c = p->sndChannel[k];
                    sncSOUND_DESCR *d = c ? c->cur : 0;
                    if (d != last[k])
                        printf("plrsnd: t=%.2f voice %d %s (buffer %s)%c", t, k, d ? d->name.str : "-",
                               c && c->buffer ? "yes" : "no", 10);
                    last[k] = d;
                }
            }
            if (fr % 30 == 0) {
                status(p, line);
                printf("t=%.2f %s\n", t, line);
                if (getenv("WR_BODYANIM") && p->body && p->body->inst->link) {   // (test) the body's animation
                    animSYSTEM_INST *si = (animSYSTEM_INST *)p->body->inst->link;
                    objOBJ *ft = objFindName(p->body->inst->root, (char *)"RFOOT");
                    if (si->sys) {
                        int nx = si->sys->GetNextAnimSeq(si->cur, p->body->seqTarget, si->group);
                        int ty = si->sys->GetTranType(si->cur, p->body->seqTarget, si->group);
                        animSEQ *sq = p->body->inst->tpl->seq;
                        printf("body anim: group %d next %d type %d %s dir %d len %.0f inst seq %d nSeq %d%c", si->group, nx, ty,
                               nx >= 0 && nx < p->body->inst->tpl->nSeq ? sq[nx].name : "?",
                               nx >= 0 && nx < p->body->inst->tpl->nSeq ? sq[nx].dir : -9,
                               nx >= 0 && nx < p->body->inst->tpl->nSeq ? sq[nx].lengthMs : -1.0f, p->body->inst->seq,
                               p->body->inst->tpl->nSeq, 10);
                    }
                    printf("body anim: sys %p tplsys %p n %d target %d cur %d frame %.2f flags %X b8 %X flags2 %X entFlags %X RFOOT %.3f %.3f %.3f%c", (void *)si->sys, (void *)p->body->inst->tpl->aniSystem, si->sys ? si->sys->n : -1, p->body->seqTarget,
                           si->cur, si->chn ? si->chn[0].GetFrameCur() : -1.0f, (unsigned)p->body->inst->flags,
                           (unsigned)*((u8 *)p->body + 0xB8), *(u32 *)((char *)p->body + 0x1FB), (unsigned)p->body->entFlags,
                           ft ? ft->ltm[12] : 0.0f, ft ? ft->ltm[13] : 0.0f, ft ? ft->ltm[14] : 0.0f, 10);
                }
                if (s_lvl && cinTest)
                    tstCinemaLog();
                if (s_lvl)
                    tstEntsLog(p);
                if (s_lvl)
                    tstWaterLog(p);
            }
            if (fr == 61 && s_lvl) {                            // the light the held weapon gets (gsEffEnvirColor)
                m3dCOLOR col, dyn;
                u32 c = gsEffEnvirColor(&p->floorInfo, &col, &dyn);
                printf("floor light %08X  col %.0f %.0f %.0f  dyn %.0f %.0f %.0f\n", c, col.r, col.g, col.b, dyn.r, dyn.g, dyn.b);
            }
            if (getenv("WR_BODYDUMP") && plrPlayer && plrPlayer->body && (int)(t * 60.0f + 0.5f) == 90) {
                static const char *nm[] = { "tors1&lod", "Legs1&lod", "DOWN_BODY_02&lod_1", "hat1&lod", "telo_UV&lod_1", "Legs2" };
                for (int k = 0; k < 6; k++)
                    if (objOBJ *o = objFindName(plrPlayer->body->inst->root, (char *)nm[k]))
                        printf("bodystate %s: procState %X state %X%c", nm[k], (unsigned)o->procState, (unsigned)o->state, 10);
                printf("bodystate inst flags %X pitch %.1f%c", (unsigned)plrPlayer->body->inst->flags, plrPlayer->pitch, 10);
            }
            static int pitched;                         // (test) WR_PITCH: once settled, the view turned to that
            if (getenv("WR_PITCH") && !pitched && t > 0.3f) {   //   pitch (170: looking down) as a mouse move does
                float d = (float)atof(getenv("WR_PITCH")) - p->pitch;  //   (ControlFirstPerson: RotateX, pitch += d)
                p->inst->RotateX(d, 1);
                p->pitch += d;
                pitched = 1;
            }
            double r0 = platTime();
            render(p);                                 // every frame: the HUD's fade and icon turns run per update
            if (s_lvl) tstAnimSndLog(), gsEndFrameInsts(1);                 // the frame end (gsRENDER_SYSTEM::EndFrame: anim sounds)
            {                                           // (test) the render's own time, after the 1 s settle
                static double sum, mx;
                static int nr;
                double dt = (platTime() - r0) * 1000.0;
                if (t > 1.0f) {
                    sum += dt; nr++;
                    if (dt > mx) mx = dt;
                }
                if (t + 1.0f / 60.0f >= walkEnd && nr)
                    printf("render: %d frames, %.1f ms a frame (most %.1f); %s%c", nr, sum / nr, mx, tstDrawStats(), 10);
            }
            if (turnTest && turnFr % 30 == 29) {
                _snprintf(path, sizeof(path) - 1, "%s_t%d.ppm", shot, turnFr / 30);
                rndScreenshot(path);
            }
            int sf[4] = { 60, 239, 20, 120 };            // _2: 1/3 s in (HUD icons turning); _3: 2 s
            if (const char *sa = argValue(argc, argv, "--shotat"))   // (test) the four frames: "a,b,c,d"
                sscanf(sa, "%d,%d,%d,%d", &sf[0], &sf[1], &sf[2], &sf[3]);
            for (int k = 0; k < 4; k++)
                if (fr == sf[k]) {
                    _snprintf(path, sizeof(path) - 1, "%s_%d.ppm", shot, k);
                    rndScreenshot(path);
                }
            t += 1.0f / 60.0f;
        }
        platTerm();
        return 0;
    }
    double last = platTime(), lastTitle = 0;
    int lastBr = 0;
    while (platPump()) {
        int mx, my;
        platInput(keys, &mx, &my);
        static int f11Was, isFull = -1;                 // TEMPORARY (test): F11 full screen <-> window
        if (isFull < 0)
            isFull = platFullScreen;
        int f11 = (keys[DIK_F11] & 0x80) != 0;
        static int f11Frame;                            // (test) --f11test: F11 at frames 60 and 180, quit at 300
        if (argValue(argc, argv, "--f11test") || (argc > 1 && !strcmp(argv[argc - 1], "--f11test"))) {
            f11Frame++;
            f11 = f11Frame == 60 || f11Frame == 180;
            if (f11Frame == 150 && argValue(argc, argv, "--f11shot"))   // (test) a picture while in full screen
                rndScreenshot(argValue(argc, argv, "--f11shot"));
            if (f11Frame == 300) {
                printf("f11test: done, full screen %d, frame %d x %d\n", isFull, s_scrW, s_scrH);
                break;
            }
        }
        if (s_allowFullScreen && f11 && !f11Was) {
            int w = 1024, h = 768;
            if (!isFull) {
                w = s_iniPath[0] ? GetPrivateProfileIntA("Video", "VIDEO_SizeX", 0, s_iniPath) : 0;
                h = s_iniPath[0] ? GetPrivateProfileIntA("Video", "VIDEO_SizeY", 0, s_iniPath) : 0;
                if (w <= 0 || h <= 0) {
                    w = GetSystemMetrics(SM_CXSCREEN);
                    h = GetSystemMetrics(SM_CYSCREEN);
                }
            }
            int okT = platToggleFullScreen(w, h);
            printf("f11: to %s %d x %d: %s\n", isFull ? "window" : "full screen", w, h, okT ? "ok" : "FAILED");
            if (okT) {
                isFull = okT == 2 ? 0 : !isFull;
                s_scrW = rndWidth();
                s_scrH = rndHeight();
            }
        }
        f11Was = f11;
        static int escWas;                              // (test) Esc on its press only: leaving the menu with it
        int escNow = (keys[DIK_ESCAPE] & 0x80) != 0;    //   (still held) does not open it again
        int escPress = escNow && !escWas;
        escWas = escNow;
        if (escPress) {
            if (!menu)
                break;
            if (gsSysSound)                             // the game's sounds held while the menus are up
                gsSysSound->entChannels->Mute(1, 0);  // (only the entities' channels: the menu music and clicks play on)
            int r = tstMenuRun(1);                      // the in-game menu (ig_main)
            menuMusicOff();
            int fresh = 0;
            if (r == TST_MENU_STOP) {                   // STOP GAME: back to the main menu
                r = tstMenuRun(0);
                menuMusicOff();
                fresh = 1;
            }
            if (gsSysSound) {
                gsSysSound->entChannels->Mute(0, 0);
                gsDOMAIN_LIST_MUS *ml = gsSysSound->musDomains;    // the menu's music stopped the level's: its track again
                if (s_lvl && !fresh && r != TST_MENU_PLAYLG && r != TST_MENU_QUIT && ml && ml->cur && sndSystem)
                    sndSystem->SetMusic(ml->cur, 1);
            }
            if (r == TST_MENU_QUIT)
                break;
            if (r == TST_MENU_PLAYLG) {
                if (plrPLAYER *q = startLevel(tstMenuLevelPath))
                    p = q;
            } else if (fresh && s_lvl) {               // a new game from the main menu: the box level
                endLevel();                             //   (the level played so far ends first)
                gsScenePtr = scn;
                m3dV sp = { 0, 3.0f, 0 };               // a new box player at its start (the level's end took every
                p = boxPlayer = makePlayer(scn, &sp);   //   entity, the old box player too)
                if (pteManager) {                       //   its shaders (the chrome, the blood hands) on the box again
                    pteManager->TermLevel();
                    pteManager->InitLevel(scn);
                }
            }
            last = platTime();
            platInput(keys, &mx, &my);
            continue;
        }
        // [ and ] (test only): sensitivity x0.8 / x1.25 per press, within the game's clamp
        int br = (keys[DIK_LBRACKET] & 0x80) ? -1 : (keys[DIK_RBRACKET] & 0x80) ? 1 : 0;
        if (br && br != lastBr) {
            s_mouseSens *= br > 0 ? 1.25f : 0.8f;
            s_mouseSens = s_mouseSens < 0.01f ? 0.01f : (s_mouseSens > 4.0f ? 4.0f : s_mouseSens);
        }
        lastBr = br;
        // _inpINPUT mouse axis (v12 0x005945A7): counts * MouseSens * 0.025659; InvertMouse negates Y
        // (ApplyMouseSettings sets inpINPUT flag 0x80). gsINP_SYSTEM then hands it to ControlFirstPerson as
        // the TURN commands' value (v12 0x0052C2F6 / 0x0052C37B).
        float k = s_mouseSens * 0.025659f;
        float lx = mx * k, ly = (s_invertMouse ? -my : my) * k;
        // arrow keys (test only): turn 120 and pitch 90 degrees per second, through the same TURN value
        // (ControlFirstPerson: yaw = value * 0.2 * 40 degrees, pitch = value * 0.2 * 30 degrees)
        float kdt = (float)(platTime() - last);
        if (kdt > 0.1f)
            kdt = 0.1f;
        if (keys[DIK_LEFT] & 0x80)  lx -= 120.0f * kdt / 8.0f;
        if (keys[DIK_RIGHT] & 0x80) lx += 120.0f * kdt / 8.0f;
        if (keys[DIK_UP] & 0x80)    ly -= 90.0f * kdt / 6.0f;
        if (keys[DIK_DOWN] & 0x80)  ly += 90.0f * kdt / 6.0f;
        s_mouseShoot = platMouseButtons() & 1;          // the left button: SHOOT
        setInput(keys, lx, ly);
        double now = platTime();
        float dt = (float)(now - last);
        last = now;
        gsElapsedTime = tstFrameTime(dt < 0.1f ? dt : 0.1f);
        s_time += gsElapsedTime;
        if (gsElapsedTime <= 0)
            gsElapsedTime = 0.001f;
        if (!lcPaused()) {
            playerFrame(p);
            if (s_lvl)
                tstEntsFrame();
            if (wrSysStat)                              // (test: wrSTAT_SYSTEM gets no GS_MSG_FRAME here)
                wrSysStat->ProcessFRAME();
        }
        if (s_lvl)
            p = levelChangeFrame(p);
        render(p);
        if (s_lvl) tstAnimSndLog(), gsEndFrameInsts(1);                   // the frame end (gsRENDER_SYSTEM::EndFrame: anim sounds)
        if (now - lastTitle > 0.25) {
            char st[256];
            status(p, st);
            float hs = sqrtf(p->ctrlVel.x * p->ctrlVel.x + p->ctrlVel.z * p->ctrlVel.z);
            _snprintf(line, sizeof(line) - 1, "sens %.3f ([ ])  speed %.1f/%.0f%s  %s", s_mouseSens, hs, p->maxSpeed,
                      (p->cfg.bits & 1) ? " always-run" : "", st);
            line[sizeof(line) - 1] = 0;
            platSetTitle(line);
            lastTitle = now;
        }
    }
    platTerm();
    return 0;
}
