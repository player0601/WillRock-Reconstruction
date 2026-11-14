// gs_level_init.cpp -- gsLVL_SYSTEM, the parts of level start-up rebuilt so far: the "&tag" flags level designers
// put in object names (Will Rock 1.2, v12.exe; feb obj gs:gs_level_init.obj)
//
// SetExtraObjFlags: v12 0x00535900..0x00535C9F, the feb code apart from &trda / &trf / &trs / &trl, which 1.2 sets
// on the object alone (_SetStateRendYes; feb: SetStateRendYes, the whole subtree). It is called by
// gsLVL_SYSTEM::ObjLoadNotify (v12 0x00535680, not rebuilt) for every object of the level and of its templates.
//
// The tags (strstr on the name, so anywhere in it):
//   &dom   domain volume: procState 0x40008 on the subtree, state 0x38          (not drawn, no collision)
//   &h     hidden: procState 0x11008 (with &nc: 8 only); the geometry's +0x5E cleared
//   &plrc  procState |= 0x1008: not drawn, collides with the player only (plrPLAYER's collision filter, v12
//          0x00425BEC, takes objects with "&plrc" in the name)
//   &aic   procState |= 0x1008: the same for AI (the AI filter tests the name)
//   &nch   procState 0x40000 on the subtree; else &nc: procState |= 0x40000 (no collision)
//   &tra N / &trb N  transparency 0x40 / 0x80 with alpha N (default 255)       &trda  render state 0x1000
//   &cc X  constant colour ARGB X (hex)        &ln  state |= 8        &vis  procState 0x20000 on the subtree
//   &nf    state |= 0x10      &ns, kukla  state |= 0x20      &mmlod  state |= 0x800      &fl  +0x0C |= 1
//   &bb    a billboard modifier (objMOD_BILLBOARD, &bbv: about the vertical axis)   &ds  state |= 4
//   &trf / &trs / &trl  render state 0x2000000 / 0x4000000 / 0x8000000
//   (&lt and &argb are looked for and ignored.)

#include "../Sources/game.h"
#include "../wr_vcall.h"

// objMOD_BILLBOARD (engine:obj_mod.obj, not rebuilt): 0x9C bytes, ctor v12 0x004E06B0, slot +0x04 attaches it to
// an object, +0x04 dword flags (0x2000: vertical axis).
struct objMOD_BILLBOARD_ {
    void *vtbl;
    u32   flags;
    u8    _08[0x9C - 0x08];
};
objMOD_BILLBOARD_ *__fastcall objMakeBillboard();      // operator new(0x9C) + objMOD_BILLBOARD ctor

// v12 0x005358C0  (feb 0x0051FF40)  the level system's slot +0x2C InstLoadNotify
int __fastcall gsLvlInstLoadNotify(animINST *i, void *)
{
    wrEVCall<void>(gsSysLevel, 0x2C, i);
    return 1;
}

// v12 0x005358E0  (feb 0x0051FF60)  slot +0x34 InstDestroyNotify
int __fastcall gsLvlInstDestroyNotify(animINST *i, void *)
{
    wrEVCall<void>(gsSysLevel, 0x34, i);
    return 1;
}

// One texture layer's "&atu_x" / "&atv_y" style scroll speeds in the object's name: a scroll modifier when either is set.
static void lvlTexScroll(objOBJ *o, const char *tagU, const char *fmtU, const char *tagV, const char *fmtV, int layer)
{
    float u = 0, v = 0;
    if (char *p = strstr(o->name, tagU))
        if (sscanf(p, fmtU, &u) != 1)
            apMsg((char *)"Cannot read U scorolling parameters for object %s. Effect ignored", o->name);
    if (char *p = strstr(o->name, tagV))
        if (sscanf(p, fmtV, &v) != 1)
            apMsg((char *)"Cannot read V scorolling parameters for object %s. Effect ignored", o->name);
    if (fabsf(u) < 1e-6f && fabsf(v) < 1e-6f)
        return;
    objMOD_TEX_SCROLL *m = new objMOD_TEX_SCROLL(layer);
    m->uSpeed = u;
    m->vSpeed = v;
    m->AddObj(o);
}

// v12 0x00535CC0  (feb 0x00520360)
// The texture animations in an object's name: "&atf_rows_cols_period[_time_frames_scaleU_scaleV]" the frames of a
// texture sheet; "&atu_x", "&atv_y" the first layer scrolls, "&atdu_x", "&atdv_y" the detail layer (2).
// Not rebuilt: "&ab_a_b_c_d" (objMOD_BLINK_TRANSP v12 0x004DD700).
void gsLVL_SYSTEM::SetTexAnimObjFlags(objOBJ *o)
{
    if (!o->name)
        return;
    if (char *p = strstr(o->name, "&atf")) {
        int rows, cols, frames = -1;
        float period, time = 0, su = -1.0f, sv = -1.0f;
        if (sscanf(p, "&atf_%d_%d_%f_%f_%d_%f_%f", &rows, &cols, &period, &time, &frames, &su, &sv) >= 3) {
            objMOD_TEX_FRAME_ANIM *m = new objMOD_TEX_FRAME_ANIM(0);
            m->SetParams(rows, cols, period, time, frames, su, sv, 1);
            m->AddObj(o);
        } else {
            apMsg((char *)"Cannot read texture animation parameters for object %s. Effect ignored", o->name);
        }
    }
    lvlTexScroll(o, "&atu", "&atu_%f", "&atv", "&atv_%f", 0);
    lvlTexScroll(o, "&atdu", "&atdu_%f", "&atdv", "&atdv_%f", 2);
}

// v12 0x00535900  (feb 0x0051FF80)
void gsLVL_SYSTEM::SetExtraObjFlags(objOBJ *o)
{
    if (!o->name)
        return;
    if (strstr(o->name, "&dom")) {
        o->SetStateProcYes(0x40008, 0);
        o->SetStateYes(0x38);
    }
    if (strstr(o->name, "&h")) {
        o->SetStateProcYes(strstr(o->name, "&nc") ? 8 : 0x11008, 0);
        *(u32 *)((char *)o->geom + 0x5E) = 0;
    }
    if (strstr(o->name, "&plrc"))
        o->procState |= 0x1008;
    if (strstr(o->name, "&aic"))
        o->procState |= 0x1008;
    if (strstr(o->name, "&nch"))
        o->SetStateProcYes(0x40000, 0);
    else if (strstr(o->name, "&nc"))
        o->procState |= 0x40000;
    if (char *p = strstr(o->name, "&tra")) {
        int a = 0xFF;
        sscanf(p, "&tra %d", &a);
        o->SetTransparency(0x40, (u8)a);
    }
    if (char *p = strstr(o->name, "&trb")) {
        int a = 0xFF;
        sscanf(p, "&trb %d", &a);
        o->SetTransparency(0x80, (u8)a);
    }
    if (strstr(o->name, "&trda"))
        o->_SetStateRendYes(0x1000);
    if (char *p = strstr(o->name, "&cc")) {
        u32 c;
        if (sscanf(p, "&cc %x", &c) == 1)
            o->SetConstColor(c, 0, 0);
        else
            apMsg((char *)"Cannot read constant color for object %s. Effect ignored", o->name);
    }
    strstr(o->name, "&lt");
    if (strstr(o->name, "&ln"))
        o->state |= 8;
    if (strstr(o->name, "&vis"))
        o->SetStateProcYes(0x20000, 0);
    strstr(o->name, "&argb");
    if (strstr(o->name, "&nf"))
        o->state |= 0x10;
    if (strstr(o->name, "&ns"))
        o->state |= 0x20;
    if (strstr(o->name, "&mmlod"))
        o->state |= 0x800;
    if (strstr(o->name, "&fl"))
        o->unk0C |= 1;
    if (strstr(o->name, "kukla"))
        o->state |= 0x20;
    if (strstr(o->name, "&bb")) {
        objMOD_BILLBOARD_ *b = objMakeBillboard();
        if (strstr(o->name, "&bbv"))
            b->flags |= 0x2000;
        wrVCall<void>(b, 4, o);
    }
    if (strstr(o->name, "&ds"))
        o->state |= 4;
    if (strstr(o->name, "&trf"))
        o->_SetStateRendYes(0x2000000);
    if (strstr(o->name, "&trs"))
        o->_SetStateRendYes(0x4000000);
    if (strstr(o->name, "&trl"))
        o->_SetStateRendYes(0x8000000);
}
