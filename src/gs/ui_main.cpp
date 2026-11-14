// ui_main.cpp -- gs.lib UI: the element tree (uiELEMENT), the element factory registry and the base containers
// (Will Rock 1.2, v12.exe; feb obj gs:ui_main.obj)
//
// Rebuilt from the 1.2 code (v12 addresses), checked against the Feb 2003 decompile (same layout in both builds;
// uiUI_SYSTEM's virtuals from feb +0x20 up moved +4 in 1.2, so the type-name call below is slot +0x4C, feb +0x48).
// Not rebuilt here yet: uiFRAME_GROUP, uiCURSOR, and of uiUI_SYSTEM everything but the constructor and
// GetTypeName (Init / Activate / Update / Render / OnSysInput / focus).

#include "../engine/engine.h"
#include "../../include/frontend/ui_core.h"
#include "../../include/frontend/wr_ui.h"
#include <string.h>
#include <stdio.h>
#include <new>

static inline uiUI_SYSTEM *uiSys() { return (uiUI_SYSTEM *)uiSystem; }

// ---------------------------------------------------------------------------------------------------- uiBOX

// v12 0x005188F0  (feb 0x00503540)
// 1 when b lies strictly inside this box, or when the two overlap (edges included).
int uiBOX_::IsBelongBox(uiBOX_ *b)
{
    if (min.x < b->min.x && min.y < b->min.y && b->max.x < max.x && b->max.y < max.y)
        return 1;
    if (min.x <= b->max.x && min.y <= b->max.y && b->min.x <= max.x && b->min.y <= max.y)
        return 1;
    return 0;
}

// v12 0x00518980  (feb 0x005035D0)
// Grow the box to hold v. BUG (original, both builds): the "y below the top" branch tests max.x against v.x
// and then writes max.y, so a box only grows downwards when the point is also right of it.
void uiBOX_::AddVertex(m2dV *v)
{
    if (v->x < min.x)
        min.x = v->x;
    else if (max.x < v->x)
        max.x = v->x;
    if (v->y < min.y)
        min.y = v->y;
    else if (max.x < v->x)
        max.y = v->y;
}

// ---------------------------------------------------------------------------------------------------- uiELEMENT

// v12 0x005189E0  (feb 0x00503630)
uiELEMENT::uiELEMENT()
{
    bboxMin = m2dVZero;
    bboxMax = m2dVZero;
    flags = 0;
    parent = 0;
    firstChild = 0;
    next = 0;
    prev = 0;
    name[0] = 0;
    groupMask = 0;
}

// v12 0x0048F380 (emitted with the wr_ui.obj windows; feb 0x00482420)
uiELEMENT::~uiELEMENT()
{
}

// v12 0x00518A30  (feb 0x00503680)
// The element's box from its object's bounding box (page units: x, -y), its name, the "group%i" flags, then one
// child element per child object: the type is the object's UI_PROP "type", else the default child type of this
// element (GetElemDefType, -1 = objects without a type get no element). The children's boxes are merged in.
// HOOK: every element reads its layout here.
void uiELEMENT::Init(objOBJ *o)
{
    objOBJ *first = o ? o->child : 0, *c = first;
    int wrapped = 0;
    bboxMin.x = o->bbox.min[0];
    bboxMin.y = -o->bbox.max[1];
    bboxMax.x = o->bbox.max[0];
    bboxMax.y = -o->bbox.min[1];
    psSHEET *ps = o->props;
    strncpy(name, o->name, 0x40);
    name[0x3F] = 0;
    char type[0x40];
    if (ps) {
        groupMask = 0;
        for (int i = 0; i < 32; i++) {
            int on;
            sprintf(type, "group%i", i);
            if (ps->GetBool((char *)"UI_PROP", type, &on) && on)
                groupMask |= 1u << i;
        }
    }
    while (first && !(wrapped && c == first)) {
        int def = GetElemDefType();
        if (def != -1) {
            if (!c->props || !c->props->GetStr((char *)"UI_PROP", (char *)"type", type, 0x40)) {
                strncpy(type, uiSystem->GetTypeName(def), 0x40);   // wrUI_SYSTEM slot +0x4C (feb +0x48)
                type[0x3F] = 0;
            }
            uiELEMENT *e = uiElementCreate(type, c, this);
            if (e) {
                bbox.AddVertex(&e->bboxMin);           // (inlined uiBOX::AddVertex, with its bug)
                bbox.AddVertex(&e->bboxMax);
            }
        }
        c = c->next;
        wrapped = 1;
    }
}

// feb 0x005038A0 (v12 inlined)
// The child of this element on the way down to e (e itself when it is a direct child), 0 when e is not below.
uiELEMENT *uiELEMENT::GetPathToChild(uiELEMENT *e)
{
    uiELEMENT *p = e;
    while (p) {
        uiELEMENT *up = p->parent;
        if (up == this)
            return p;
        p = up;
    }
    return 0;
}

// v12 0x00518C70  (feb 0x005038C0)
// By name: the direct children first, then each child's subtree in order. HOOK target for mods.
uiELEMENT *uiELEMENT::FindChildElement(const char *n)
{
    for (uiELEMENT *c = firstChild; c; c = c->next)
        if (!strcmp(c->name, n))
            return c;
    for (uiELEMENT *c = firstChild; c; c = c->next)
        if (uiELEMENT *r = c->FindChildElement(n))
            return r;
    return 0;
}

// v12 0x0048D580 (the shared "return -1"; feb 0x0049A730): no default child type
int uiELEMENT::GetElemDefType()
{
    return -1;
}

// v12 0x00518D30  (feb 0x00503980)
int uiELEMENT::CanAcceptKbdFocus()
{
    return (flags & UIEF_CAN_KBD) && !(flags & (UIEF_INACTIVE | UIEF_DISABLED | UIEF_HIDDEN));
}

// v12 0x00518D10  (feb 0x00503960)
int uiELEMENT::CanAcceptMouseFocus()
{
    return (flags & UIEF_CAN_MOUSE) && !(flags & (UIEF_INACTIVE | UIEF_DISABLED | UIEF_HIDDEN));
}

// v12 0x00518CF0  (feb 0x00503940)
void uiELEMENT::Enable(int on)
{
    if (on)
        flags &= ~UIEF_DISABLED;
    else
        flags |= UIEF_DISABLED;
}

// v12 0x00518D50  (feb 0x005039A0)
int uiELEMENT::CanBeTabstop()
{
    return !(flags & (UIEF_INACTIVE | UIEF_DISABLED | UIEF_HIDDEN)) && (flags & UIEF_TABSTOP);
}

// v12 0x00518D70  (feb 0x005039C0)
// Tab order: rows of 4 page units top to bottom (rounded), and in a row right before left.
int uiELEMENT::GetTabstopId()
{
    int y = (int)bboxMin.y, x = (int)bboxMin.x;
    return ((y + 2) >> 2) * 0x4000 - x;
}

// v12 0x00518E10  (feb 0x00503A60)
// how != 0: appear (shown), 0: disappear. Returns 1 while some child still has an effect running.
int uiELEMENT::StartAppearing(int how)
{
    if (how)
        flags &= ~UIEF_INACTIVE;
    int any = 0;
    for (uiELEMENT *c = firstChild; c; c = c->next)
        if (c->StartAppearing(how))
            any = 1;
    if (any) {
        if (how)
            flags = (flags & ~UIEF_INACTIVE) | UIEF_APPEARING;
        else
            flags |= UIEF_DISAPPEARING;
    }
    return any;
}

// v12 0x00518EA0  (feb 0x00503AF0)
void uiELEMENT::FinishAppearing(int)
{
    for (uiELEMENT *c = firstChild; c; c = c->next)
        c->FinishAppearing(0);
    if (!(flags & UIEF_DISAPPEARING)) {
        OnAppeared(1);
    } else {
        flags |= UIEF_INACTIVE;
        OnAppeared(0);
    }
    flags &= ~(UIEF_APPEARING | UIEF_DISAPPEARING | UIEF_EFFECT);
}

// v12 0x00518F00  (feb 0x00503B50)
// 1 while this element or a child is still fading.
int uiELEMENT::ProcessAppearing(int)
{
    unsigned how;
    if (flags & UIEF_APPEARING)
        how = UIEF_APPEARING;
    else if (flags & UIEF_DISAPPEARING)
        how = UIEF_DISAPPEARING;
    else
        return 0;
    int any = 0;
    for (uiELEMENT *c = firstChild; c; c = c->next)
        if (c->ProcessAppearing(0))
            any = 1;
    if (any)
        return 1;
    if (flags & UIEF_EFFECT)
        return 1;
    flags &= ~how;
    if (how == UIEF_DISAPPEARING) {
        flags |= UIEF_INACTIVE;
        OnAppeared(0);
        return 0;
    }
    OnAppeared(1);
    return 0;
}

// v12 0x0051AAF0 (shared "ret 4"; feb 0x004CDA70)
void uiELEMENT::OnAppeared(int)
{
}

// v12 0x0048D590 (shared "return 1"; feb 0x004C46B0)
int uiELEMENT::OnMouseFocus(uiELEMENT *)
{
    return 1;
}

// v12 0x0048D590 (feb 0x004C46B0)
int uiELEMENT::OnKbdFocus(uiELEMENT *)
{
    return 1;
}

// v12 0x00518E00  (feb 0x00503A50)
// 1 = draw the children too (the element is not UIEF_HIDDEN)
int uiELEMENT::ProcessRender(void *)
{
    return (~flags >> 13) & 1;
}

// v12 0x00519080 (the shared "return 1" of the input / process defaults; feb 0x00524AE0)
int uiELEMENT::ProcessInit(void *)            { return 1; }
int uiELEMENT::ProcessUpdate(void *)          { return 1; }
int uiELEMENT::OnPreInput(uiINPUT_INFO *)     { return 1; }
int uiELEMENT::OnMouseButton(uiINPUT_INFO *)  { return 1; }
int uiELEMENT::OnKeyboard(uiINPUT_INFO *)     { return 1; }
int uiELEMENT::OnChar(uiINPUT_INFO *)         { return 1; }

// v12 0x00519090  (feb 0x00503CD0)
// Widgets report to their window: passed up until an element handles it.
void uiELEMENT::OnChildNotify(uiCHILD_NOTIFY *n)
{
    if (parent)
        parent->OnChildNotify(n);
}

// ---------------------------------------------------------------------------------------------------- registry

// v12 0x0051AB00  (feb 0x00505720)
// Add a type (once): uiSystem's array of { name, create } (+0x118 / +0x11C / +0x120). HOOK: add custom widget types.
int __fastcall uiElementRegister(const char *type, uiELEMENT *(__fastcall *create)())
{
    uiUI_SYSTEM *u = uiSys();
    for (int i = 0; i < u->nElemTypes; i++)
        if (!strcmp(type, u->elemTypes[i].type))
            return 1;
    int n = u->nElemTypes + 1;
    if (u->capElemTypes < n) {
        int cap = u->capElemTypes * 2;
        if (cap <= n)
            cap = n;
        u->elemTypes = (uiELEMENT_INFO *)apRealloc(u->elemTypes, cap * sizeof(uiELEMENT_INFO));
        u->capElemTypes = cap;
    }
    uiELEMENT_INFO *e = &u->elemTypes[u->nElemTypes++];
    memset(e, 0, sizeof(*e));
    strncpy(e->type, type, 0x40);
    e->type[0x3F] = 0;
    e->create = create;
    return 1;
}

// v12 0x0051AC60  (feb 0x00505880)
// Make an element of a registered type, append it to parent's children, then Init it from o.
uiELEMENT *__fastcall uiElementCreate(const char *type, objOBJ *o, uiELEMENT *parent)
{
    uiUI_SYSTEM *u = uiSys();
    for (int i = 0; i < u->nElemTypes; i++) {
        if (strcmp(type, u->elemTypes[i].type))
            continue;
        uiELEMENT *e = u->elemTypes[i].create();
        if (!e)
            return 0;
        e->parent = parent;
        if (parent) {
            if (!parent->firstChild)
                parent->firstChild = e;
            else {
                uiELEMENT *last = parent->firstChild;
                while (last->next)
                    last = last->next;
                last->next = e;
                e->prev = last;
            }
        }
        e->Init(o);
        return e;
    }
    return 0;
}

// v12 0x0051AD40  (feb 0x00505960)
// Destroy the subtree (children first), unlink, and delete.
void __fastcall uiElementDestroy(uiELEMENT *e)
{
    while (e->firstChild)
        uiElementDestroy(e->firstChild);
    if (!e->prev) {
        if (e->parent)
            e->parent->firstChild = e->next;
    } else
        e->prev->next = e->next;
    if (e->next)
        e->next->prev = e->prev;
    delete e;
}

// ---------------------------------------------------------------------------------------------------- containers
// The four base types (uiUI_SYSTEM::RegisterDefaultElements: "area", "window", "frame", "frm_grp").

// feb 0x00506120 (v12 inlined into RegisterDefaultElements' table)
uiELEMENT *__fastcall uiAREA::Create()
{
    return new uiAREA;
}

// feb 0x005061B0
uiELEMENT *__fastcall uiWINDOW::Create()
{
    return new uiWINDOW;
}

// feb 0x00403680 (a shared "return 0"): a window's untyped children are areas
int uiWINDOW::GetElemDefType()
{
    return 0;
}

// feb 0x004036A0 (a shared "return 1"): a frame's untyped children are windows
int uiFRAME::GetElemDefType()
{
    return 1;
}

// feb 0x00506210
uiELEMENT *__fastcall uiFRAME::Create()
{
    return new uiFRAME;
}

// ---------------------------------------------------------------------------------------------------- the tree walk

// v12 0x005190A0  (feb 0x00503CE0)
// One uiACTION through the subtree, parent first. Init / update / render stop below an element whose handler
// returns 0. The input actions first offer the event to OnPreInput (0 = swallowed), then to the children (any 0
// stops it), then to the element's own handler; keys and chars only reach the focused element or one that takes
// them always. UIA_SHOW (arg 0 = hide) sets / clears UIEF_INACTIVE on the whole subtree. Returns 1 = handled /
// go on.
int uiELEMENT::ProcessAction(int action, void *arg)
{
    int r;
    switch (action) {
    case UIA_INIT:
    case UIA_UPDATE:
    case UIA_RENDER:
        r = action == UIA_INIT ? ProcessInit(arg) : action == UIA_UPDATE ? ProcessUpdate(arg) : ProcessRender(arg);
        if (!r)
            return 0;
        for (uiELEMENT *c = firstChild; c; c = c->next)
            c->ProcessAction(action, arg);
        return 1;
    case UIA_MOUSE_MOVE:
    case UIA_MOUSE_BUTTON:
    case UIA_KEYBOARD:
    case UIA_CHAR: {
        if (!OnPreInput((uiINPUT_INFO *)arg))
            return 0;
        int all = 1;
        for (uiELEMENT *c = firstChild; c; c = c->next)
            if (!c->ProcessAction(action, arg))
                all = 0;
        if (!all)
            return 0;
        uiINPUT_INFO *in = (uiINPUT_INFO *)arg;
        if (action == UIA_MOUSE_MOVE)
            r = OnMouseMove(in);
        else if (action == UIA_MOUSE_BUTTON)
            r = OnMouseButton(in);
        else {
            if (!(flags & UIEF_KEYS_ALWAYS) && this != uiSys()->kbdFocus)
                return 1;
            r = action == UIA_KEYBOARD ? OnKeyboard(in) : OnChar(in);
        }
        return r ? 1 : 0;
    }
    case UIA_SHOW:
        if (!arg)
            flags |= UIEF_INACTIVE;
        else
            flags &= ~UIEF_INACTIVE;
        for (uiELEMENT *c = firstChild; c; c = c->next)
            c->ProcessAction(UIA_SHOW, arg);
        return 1;
    }
    return 0;
}

// v12 0x00518F90  (feb 0x00503BE0)
// A focusable element under the cursor (a 20 x 20 box around the hot spot, inside or overlapping) claims the
// mouse: it is written to the event (+0x10) and the walk stops.
int uiELEMENT::OnMouseMove(uiINPUT_INFO *in)
{
    if (CanAcceptMouseFocus()) {
        const char *u = (const char *)uiSys();
        float px = *(const float *)(u + 0x20) + *(const float *)(u + 0x18);   // cursor position + hot spot
        float py = *(const float *)(u + 0x24) + *(const float *)(u + 0x1C);
        float x0 = px - 10.0f, y0 = py - 10.0f, x1 = px + 10.0f, y1 = py + 10.0f;
        if ((bboxMin.x < x0 && bboxMin.y < y0 && x1 < bboxMax.x && y1 < bboxMax.y) ||
            (bboxMin.x <= x1 && bboxMin.y <= y1 && x0 <= bboxMax.x && y0 <= bboxMax.y)) {
            *(uiELEMENT **)((char *)in + 0x10) = this;
            return 0;
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------------------------------- uiUI_SYSTEM

// v12 0x00519930  (feb 0x00504570)
// The cursor's positions (in the raw cursor block: +0x18 position, +0x20 hot spot, +0x28 / +0x30 / +0x38) start
// at zero, the cursor's render element is constructed, scale (1, 1), double-click timer 1000.
uiUI_SYSTEM::uiUI_SYSTEM()
{
    char *b = (char *)this;
    *(int *)(b + 0x14) = 0;
    *(m2dV *)(b + 0x30) = m2dVZero;
    *(m2dV *)(b + 0x38) = m2dVZero;
    new (cursorRender) uiRENDER_ELEM;
    *(m2dV *)(b + 0x28) = m2dVZero;
    *(m2dV *)(b + 0x20) = m2dVZero;
    elemTypes = 0;
    capElemTypes = 0;
    nElemTypes = 0;
    state = 0;
    capture = 0;
    mouseFocus = 0;
    kbdFocus = 0;
    screenH = 0;
    screenW = 0;
    _f8 = 0;
    _f4 = 0;
    scale = m2dVOneXY;
    uiCamera = 0;
    _114 = 0;
    tplInst = 0;
    _128 = 0;
    dblClickTimer = 1000.0f;
}

// v12 0x0051AAB0  (feb 0x005056E0) uiUI_SYSTEM::Notify, slot 0 of both vtables
// 'HALT' (a fatal error): the menu comes up (Activate(1, "null") keeps the frame) with a message box built from
// the event: { buttons, caption, text, back frame }.
void wrUI_SYSTEM::Notify(int ev, void *p)
{
    if (ev != 0x48414C54)                              // 'HALT'
        return;
    void **e = (void **)p;
    Activate(1, "null");
    MsgBoxW_Startup((const wrWCHAR *)e[1], (const wrWCHAR *)e[2], (int)(size_t)e[0], 0, 0, (const char *)e[3]);
}

// (the SDK's base uiUI_SYSTEM declares the same slot; the one live UI system is a wrUI_SYSTEM)
void uiUI_SYSTEM::Notify(int ev, void *p)
{
    ((wrUI_SYSTEM *)this)->wrUI_SYSTEM::Notify(ev, p);
}

// v12 0x00519CA0  (feb 0x005048E0) uiUI_SYSTEM::GetTypeName (the SDK declares the slot on wrUI_SYSTEM)
// The type of a child object without one, by its parent's GetElemDefType.
const char *wrUI_SYSTEM::GetTypeName(int defType)
{
    switch (defType) {
    case 0: return "area";
    case 1: return "window";
    case 2: return "frame";
    }
    return 0;
}
