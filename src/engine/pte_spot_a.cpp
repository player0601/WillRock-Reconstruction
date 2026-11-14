// pte_spot_a.cpp -- CPU shader "spot": a spot light driven by controllers (Will Rock 1.2, v12.exe;
// feb obj engine:pte_spot_a.obj)
//
// pteSPOT_ANIM: a pteSPOT (kind 1, "lspot") whose radius, position and both colours follow time controllers,
// read each Update. From a level sheet: "ctrl_pos" "follow" (its instance or the "object" of it), "ctrl_rad"
// "noise" (radMin .. radMax at radFreq). The game's effects (blood, scorch marks, the egg's glow) set them by hand.
// The follow controller (animCTRL_3D_FOLLOW, v12 vtable not rebuilt) is made here as pteSPOT_FOLLOW, same layout.

#include "engine.h"
#include "../m3d/m3d_ctrl.h"
#include "../../include/engine_render/shader.h"

// animCTRL_3D_FOLLOW (size 0x2C, kind 2): the position of an instance, or of an object of it.
class pteSPOT_FOLLOW : public m3dCTRL_3D {
public:
    animINST *inst;                                     // +0x24
    objOBJ   *obj;                                      // +0x28
    void GetValue(m3dV *out) { ((animCTRL_3D_FOLLOW *)this)->GetValue(out); } // v12 0x004CEC10
};
static_assert(sizeof(pteSPOT_FOLLOW) == 0x2C, "animCTRL_3D_FOLLOW");

static pteSPOT_FOLLOW *spotNewFollow(animINST *a, objOBJ *o)
{
    pteSPOT_FOLLOW *c = new pteSPOT_FOLLOW;
    if (!c)
        return 0;
    m3dCtrlTimeInit(c);
    c->value = m3dVZero;
    c->inst = a;
    c->obj = o;
    c->_14 = 2;
    return c;
}
// (the inline animCTRL_3D_FOLLOW ctor the weapons' projectiles use: a spot following the instance)
m3dCTRL_3D *__fastcall pteSpotNewFollow(animINST *a, objOBJ *o)
{
    return spotNewFollow(a, o);
}

// v12 0x004D7BB0  (feb 0x004C8BF0)
int __fastcall pteSpotAnimInit()
{
    return pteManager->RegisterClass((char *)"spot", 0x14, pteSPOT_ANIM::Create) != 0;
}

// v12 0x004D7BD0  (feb 0x004C8C10) exact
pteSHADER *__fastcall pteSPOT_ANIM::Create(animINST *, objOBJ *)
{
    return new pteSPOT_ANIM;
}

// v12 0x004039B0
// The controllers go with it.
void *pteSPOT_ANIM::__vecDelDtor(unsigned f)
{
    if (ctrlRad)
        ctrlRad->__vecDelDtor(1);
    if (ctrlPos)
        ctrlPos->__vecDelDtor(1);
    if (ctrlColorLM)
        ctrlColorLM->__vecDelDtor(1);
    if (ctrlColorBase)
        ctrlColorBase->__vecDelDtor(1);
    return pteSPOT::__vecDelDtor(f);
}

// v12 0x004D7C20  (feb 0x004C8C60) exact
// When on: each controller advanced and read (radius, position, light-map colour, base colour); one that has
// stopped switches the spot off when it should (spotFlags 4).
void pteSPOT_ANIM::Update(float dt)
{
    if (!(flags & 1))
        return;
    float r = radius;
    if (ctrlRad) {
        ctrlRad->Update(dt);
        if (ctrlRad->flags & 1) {
            ctrlRad->GetValue(&r);
            SetRadius(r);
        } else if (spotFlags & 4)
            Enable(0);
    }
    if (ctrlPos) {
        ctrlPos->Update(dt);
        if (ctrlPos->flags & 1) {
            m3dV p;
            ctrlPos->GetValue(&p);
            SetOrg(&p);
        } else if (spotFlags & 4)
            Enable(0);
    }
    m3dCOLOR c = { 0, 0, 0, 0 };
    if (ctrlColorLM) {
        ctrlColorLM->Update(dt);
        if (ctrlColorLM->flags & 1) {
            ctrlColorLM->GetValue(&c);
            colorLM = c;
        } else if (spotFlags & 4)
            Enable(0);
    }
    if (ctrlColorBase) {
        ctrlColorBase->Update(dt);
        if (ctrlColorBase->flags & 1) {
            ctrlColorBase->GetValue(&c);
            colorBase = c;
            return;
        }
        if (spotFlags & 4)
            Enable(0);
    }
}

// v12 0x004D7DC0  (feb 0x004C8E00) exact
// pteSPOT's keys; "ctrl_pos" "follow": the instance (or its "object"); none: following its own instance or object;
// "ctrl_rad" "noise": the radius flickering between radMin and radMax.
void pteSPOT_ANIM::Init()
{
    char t[0x20];
    pteSPOT::Init();
    pteSPOT_FOLLOW *c = 0;
    BOOL32 made = 0;
    if (ReadStr((char *)"ctrl_pos", t, 0x20)) {
        if (!memcmp(t, "follow", 7) && inst) {
            objOBJ *o = 0;
            if (ReadStr((char *)"object", t, 0x20))
                o = objFindName(inst->root, t);
            c = spotNewFollow(inst, o);
            if (!c && ctrlPos)
                ctrlPos->__vecDelDtor(1);
            ctrlPos = c;
            made = 1;
        }
    } else if (obj) {
        c = spotNewFollow(0, obj);
        if (!c && ctrlPos)
            ctrlPos->__vecDelDtor(1);
        ctrlPos = c;
        made = 1;
    } else if (inst) {
        c = spotNewFollow(inst, 0);
        if (!c && ctrlPos)
            ctrlPos->__vecDelDtor(1);
        ctrlPos = c;
        made = 1;
    }
    if (made && c) {
        m3dV p;
        c->GetValue(&p);
        SetOrg(&p);
    }
    if (ReadStr((char *)"ctrl_rad", t, 0x20) && !memcmp(t, "noise", 6)) {
        float freq = 1.0f, rMin = 0.5f, rMax = 2.0f;
        ReadFloat((char *)"radFreq", &freq);
        ReadFloat((char *)"radMin", &rMin);
        ReadFloat((char *)"radMax", &rMax);
        m3dCTRL_1D_NOISE *n = new m3dCTRL_1D_NOISE;
        if (n) {
            m3dCtrlTimeInit(n);
            n->value = 0;
            n->noise = m3dNoise;
            n->vMin = rMin;
            n->vMax = rMax;
            n->freq = freq;
            n->flags |= 4;
        }
        ctrlRad = n;
        if (n) {
            float r;
            n->GetValue(&r);
            SetRadius(r);
        }
    }
}

// v12 0x004D80A0  (feb 0x004C90E0) exact
void pteSPOT_ANIM::SetCtrlRad(m3dCTRL_1D *c)
{
    if (!this)
        return;
    ctrlRad = c;
    if (c) {
        float r;
        c->GetValue(&r);
        SetRadius(r);
    }
}

// v12 0x004D80D0  (feb 0x004C9110) exact
// (The old controller is deleted only when the new one is none.)
void pteSPOT_ANIM::SetCtrlPos(m3dCTRL_3D *c)
{
    if (!c && ctrlPos)
        ctrlPos->__vecDelDtor(1);
    ctrlPos = c;
    if (c) {
        m3dV p;
        c->GetValue(&p);
        SetOrg(&p);
    }
}

// v12 0x004D8120  (feb 0x004C9160) exact
void pteSPOT_ANIM::SetCtrlColorLM(m3dCTRL_COLOR *c)
{
    m3dCOLOR v = { 0, 0, 0, 0 };
    ctrlColorLM = c;
    c->GetValue(&v);
    colorLM = v;
}

// v12 0x004D8190  (feb 0x004C91D0) exact
void pteSPOT_ANIM::SetCtrlColorBase(m3dCTRL_COLOR *c)
{
    m3dCOLOR v = { 0, 0, 0, 0 };
    ctrlColorBase = c;
    c->GetValue(&v);
    colorBase = v;
}

// v12 0x004D8200  (feb 0x004C9240) exact
// Every running controller to time t.
void pteSPOT_ANIM::SetCurTime(float t)
{
    if (ctrlRad && (ctrlRad->flags & 1))
        ctrlRad->t = t;
    if (ctrlPos && (ctrlPos->flags & 1))
        ctrlPos->t = t;
    if (ctrlColorLM && (ctrlColorLM->flags & 1))
        ctrlColorLM->t = t;
    if (ctrlColorBase && (ctrlColorBase->flags & 1))
        ctrlColorBase->t = t;
}

// v12 0x004D8260  (feb 0x004C92A0)
float pteSPOT_ANIM::GetMaxRadius()
{
    return radius;
}

// v12 0x004D8270  (feb 0x004C92B0)
// Its own instance going: it goes too (1); the instance it follows going: it stops following.
int pteSPOT_ANIM::DeleteInstNotify(animINST *a)
{
    if (pteSHADER::DeleteInstNotify(a))
        return 1;
    if (ctrlPos && ctrlPos->_14 == 2 && ((animCTRL_3D_FOLLOW *)ctrlPos)->IsFollow(a)) {
        if (ctrlPos)
            ctrlPos->__vecDelDtor(1);
        ctrlPos = 0;
    }
    return 0;
}

// v12 0x004D82D0  (feb 0x004C9310)
int pteSPOT_ANIM::DeleteObjNotify(objOBJ *o)
{
    if (pteSHADER::DeleteObjNotify(o))
        return 1;
    if (ctrlPos && ctrlPos->_14 == 2 && ((animCTRL_3D_FOLLOW *)ctrlPos)->IsFollow(o)) {
        if (ctrlPos)
            ctrlPos->__vecDelDtor(1);
        ctrlPos = 0;
    }
    return 0;
}
