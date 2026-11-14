// vis.h -- Will Rock 1.2 unofficial SDK, game_systems: visibility cells (gs:gs_vis)
// gsVIS_SYSTEM is a gsDOMAIN_LIST of "&dom_vis" cells; each cell's sheet [DOM_VIS] lists which other cells are visible
// from it (vis, vis_1..). Objects outside the visible cells are culled (IsVisible). Assumes domains.h.
#pragma once
#include "domains.h"

class gsDOMAIN_VIS : public gsDOMAIN {     // vtbl v12 0x00606318/0x006062E4 (feb 0x005DABF0?/..)  // ? feb vtbl addr
    // Init v12 0x00537940 (feb 0x00522080)  Term v12 0x00537BE0 (feb 0x00522320)  Enter v12 0x00537C40  Leave v12 0x00537C50
    // ProcessMsg v12 0x00537CD0 (feb 0x00522410) handles GS_MSG_MP_NOTIFY 0x40E (feb 0x40C) "enabler" doors open/close vis
    // MP sync: GET v12 0x00537E70, WRITE v12 0x00537F30, READ v12 0x00538060
};
class gsVIS_ENABLER {                      // door/portal that switches visibility between cells, ref-counted per entity
    // FindEnabler(entENTITY*) v12 0x00539030 (feb 0x00523670); SetNEnable(entENTITY*, int) v12 0x00539070 (feb 0x005236B0)
};
class gsVIS_SYSTEM : public gsDOMAIN_LIST { // size v12 0x3D4 (feb 0x3D0); global gsSysVis v12 0x0098E83C. vtbl v12 0x006060A8, 38 slots (+1 in 1.2)
public:
    virtual int  Init();                          // slot +0x78 v12 0x00538250 (feb +0x74 0x00522990)
    virtual void SetViewPos(m3dV *p);             // slot +0x80 v12 0x005382A0 (feb 0x005229D0) HOOK: current cell from camera
    virtual void SetVisInfo(scnSCENE *s);         // slot +0x84 v12 0x00538500 (feb 0x00522C20)
    virtual int  IsVisUpdateReq();                // slot +0x88 v12 0x004AC3F0
    virtual int  IsVisible(m3dV *a, m3dV *b);     // slot +0x8C v12 0x00538C30 (feb 0x00523300)
    virtual int  IsVisible(m3dV *p);              // slot +0x90 v12 0x00538AB0 (feb 0x00523180)
    virtual int  IsVisible(m3dBOX *b);            // slot +0x94 v12 0x00538B70 (feb 0x00523240)
    virtual int  IsVisible(objOBJ *o);            // slot +0x98 v12 0x00538930 (feb 0x00523000)  HOOK: return 1 = disable cell culling
    // IsAddObj "&dom_vis" v12 0x00538280; _gsVisPropagateVisInfo v12 0x00538FB0 (feb 0x005235F0)
};
