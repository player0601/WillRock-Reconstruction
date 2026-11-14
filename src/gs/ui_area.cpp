// ui_area.cpp -- gs.lib UI: uiAREA, an element drawn from its render info (rectangle / object + text + child
// render elements). Base of every button, scroll, HUD bar.
// (Will Rock 1.2, v12.exe; feb obj gs:ui_area.obj; all v12 twins are "exact" / vtable matches)
//
// The fades are wrUI_SYSTEM's: StartAppear (slot +0x68, feb +0x64), ProcessAppear (+0x6C), FinishAppear (+0x70),
// called with 0x40 (appear) or 0x100 (disappear).

#include "../engine/engine.h"
#include "../../include/frontend/ui_core.h"
#include "../../include/frontend/wr_ui.h"

// v12 0x00528B10  (feb 0x00513670)
void uiAREA::Init(objOBJ *o)
{
    uiELEMENT::Init(o);
    renderInfo.Init(o);
    renderInfo.CalcBBox(&bbox);
}

// v12 0x00528B40  (feb 0x005136A0)
// Nothing (and no children) while hidden or faded out.
int uiAREA::ProcessRender(void *cam)
{
    if (flags & (UIEF_HIDDEN | UIEF_INACTIVE))
        return 0;
    renderInfo.Render((camCAMERA *)cam);
    return 1;
}

// v12 0x00528B70  (feb 0x005136D0)
int uiAREA::StartAppearing(int how)
{
    unsigned f = how ? (flags & ~UIEF_INACTIVE) | UIEF_APPEARING : flags | UIEF_DISAPPEARING;
    flags = f | UIEF_EFFECT;
    // 1.2 passes 0x140 when appearing (v12 0x00528B9F "and al,0x40; add eax,0x100"); feb passed 0x40
    uiSystem->StartAppear(&renderInfo, how ? UIEF_APPEARING | UIEF_DISAPPEARING : UIEF_DISAPPEARING);
    uiELEMENT::StartAppearing(how);
    return 1;
}

// v12 0x00528BC0  (feb 0x00513720)
int uiAREA::ProcessAppearing(int)
{
    int how;
    if (flags & UIEF_APPEARING)
        how = UIEF_APPEARING;
    else if (flags & UIEF_DISAPPEARING)
        how = UIEF_DISAPPEARING;
    else
        return 0;
    if ((flags & UIEF_EFFECT) && !uiSystem->ProcessAppear(&renderInfo, how))
        flags &= ~UIEF_EFFECT;
    if (!uiELEMENT::ProcessAppearing(0) && !(flags & UIEF_EFFECT))
        return 0;
    return 1;
}

// v12 0x00528C20  (feb 0x00513780)
void uiAREA::FinishAppearing(int)
{
    if (flags & UIEF_EFFECT) {
        int how;
        if (flags & UIEF_APPEARING)
            how = UIEF_APPEARING;
        else if (flags & UIEF_DISAPPEARING)
            how = UIEF_DISAPPEARING;
        else
            return;
        uiSystem->FinishAppear(&renderInfo, how);
        flags &= ~UIEF_EFFECT;
    }
    uiELEMENT::FinishAppearing(0);
}

// v12 0x00528C70  (feb 0x005137D0)
void uiAREA::SetFont(uiFONT *f)
{
    if (f == renderInfo.font)
        return;
    renderInfo.SetFont(f);
    renderInfo.CalcBBox(&bbox);
}

// v12 0x00528CA0  (feb 0x00513800)
void uiAREA::SetTextId(int strId)
{
    renderInfo.SetTextId(strId);
    renderInfo.CalcBBox(&bbox);
}

// v12 0x00528CD0  (feb 0x00513830)
void uiAREA::SetTextW(const wrWCHAR *s, int maxLen)
{
    renderInfo.SetTextW(s, maxLen);
    renderInfo.CalcBBox(&bbox);
}

// feb 0x00513860
void uiAREA::AppendTextW(const wrWCHAR *s, int maxLen)
{
    renderInfo.AppendTextW(s, maxLen);
    renderInfo.CalcBBox(&bbox);
}
