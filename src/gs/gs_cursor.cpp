// gs_cursor.cpp -- the game systems' cursor: the cursor textures and the crosshair state (gscCursor)
// (Will Rock 1.2, v12.exe; the gsc functions of gs.lib, v12 0x00513C50..0x00513F3C; the feb map gives no object
// name for them, the file is named after their prefix)
//
// Rebuilt so far: gscInit, gscTerm, gscAllocCursors, gscFreeCursors, gscGetCursorTexture, gscRegisterCursor,
// gscCURSOR::Lock, SetHide (1.2), UpdatePosMouse and Draw (the screen quad through rendDRIVER::_RenderPolyCompat).

#include "../Sources/game.h"

gscCURSOR      gscCursor;                   // v12 0x0098EA94
int            gscNCursors;                 // v12 0x0098EA8C
gscCURSOR_TEX *gscCursors;                  // v12 0x0098EACC

// v12 0x00513C50
// The aim ray's hit record (zeroed, point = m3dVZero) and the crosshair scale 1.
int __fastcall gscInit()
{
    cdtINFO *ci = (cdtINFO *)operator new(sizeof(cdtINFO));
    if (ci) {
        ci->flags = 0;
        ci->inst = 0;
        ci->objId = 0;
        ci->faceId = 0;
        *(m3dV *)ci->point = m3dVZero;
        ci->dist = 0;
    }
    gscCursor.aimInfo = ci;
    gscCursor.scale = 1.0f;
    return 1;
}

// v12 0x00513CC0
void __fastcall gscTerm()
{
    for (int i = 0; i < gscNCursors; i++)
        if (gscCursors[i].tex)
            txmManager->Del(gscCursors[i].tex);
    operator delete(gscCursor.aimInfo);
    operator delete(gscCursors);
}

// v12 0x00513D20  (feb 0x00500D10)
void __fastcall gscAllocCursors(int n)
{
    gscNCursors = n;
    gscCURSOR_TEX *t = (gscCURSOR_TEX *)operator new(n * sizeof(gscCURSOR_TEX));
    if (!t) {
        gscCursors = 0;
        return;
    }
    for (int i = 0; i < n; i++) {
        t[i].hotX = 0.5f;
        t[i].hotY = 0.5f;
        t[i].scale = 1.0f;
        t[i].tex = 0;
    }
    gscCursors = t;
}

// v12 0x00513D80
void __fastcall gscFreeCursors()
{
    gscNCursors = 0;
    operator delete(gscCursors);
    gscCursors = 0;
}

// v12 0x00513DB0
txmTEXTURE *__fastcall gscGetCursorTexture(int i)
{
    return gscCursors[i].tex;
}

// v12 0x00513DC0  (feb 0x00500DB0)
// Cursor i: texture tex (none when 0), hot spot and scale. 0 when the texture does not load (or i is negative and
// beyond the count -- the original's range test).
int __fastcall gscRegisterCursor(int i, char *tex, float hotX, float hotY, float scale)
{
    if (i < 0 && i >= gscNCursors)
        return 0;
    if (tex) {
        gscCursors[i].tex = txmManager->Add(tex, 0, 1);
        if (!gscCursors[i].tex)
            return 0;
    } else {
        gscCursors[i].tex = 0;
    }
    gscCursors[i].hotX = hotX;
    gscCursors[i].hotY = hotY;
    gscCursors[i].scale = scale;
    return 1;
}

// v12 0x00513E50  (feb 0x00500E40)
// Pins the cursor at (x, y) in UI units (the crosshair at the screen centre).
void gscCURSOR::Lock(float lx, float ly)
{
    lockX = lx;
    flags |= 2;
    lockY = ly;
    m2dV u, s;
    if (flags & 2) {
        u.x = lx;
        u.y = ly;
    } else {
        u.x = WR_FIELD(float, gsSysInput, 0x2C);
        u.y = WR_FIELD(float, gsSysInput, 0x30);
    }
    gssUCoord2SCoord(&u, &s);
    x = s.x;
    y = s.y;
}

// v12 0x00513F10  (1.2 only, name ours)
void gscCURSOR::SetHide(int show, int bit)
{
    if (show)
        hide &= ~(1u << bit);
    else
        hide |= 1u << bit;
}

// v12 0x00513EC0
// The cursor follows the mouse (the input system's UI position), unless it is locked.
void gscCURSOR::UpdatePosMouse()
{
    m2dV u, s;
    if (flags & 2) {
        u.x = lockX;
        u.y = lockY;
    } else {
        u.x = WR_FIELD(float, gsSysInput, 0x2C);
        u.y = WR_FIELD(float, gsSysInput, 0x30);
    }
    gssUCoord2SCoord(&u, &s);
    x = s.x;
    y = s.y;
}

// v12 0x00513F40
// The cursor's quad on the screen: 28 pixels times the texture's scale times the cursor's (the crosshair shrinks
// with the distance of what it points at), placed by the texture's hot spot, in the cursor's colour (the target's
// health); nothing while any hide bit is set or without a cursor / texture.
void gscCURSOR::Draw(camCAMERA *cam)
{
    if (hide)
        return;
    if (cursor == -1 || !gscCursors[cursor].tex)
        return;
    rendCFG cfg;
    cfg.flags = 0x2180AF;
    cfg.InitMtl1(gscCursors[cursor].tex);
    float w = (cursor == -1 ? 1.0f : gscCursors[cursor].scale * scale) * 28.0f;
    float h = (cursor == -1 ? 1.0f : gscCursors[cursor].scale * scale) * 28.0f;
    float hx = 0, hy = 0;
    if (cursor != -1) {
        hx = gscCursors[cursor].hotX;
        hy = gscCursors[cursor].hotY;
    }
    float x0 = x - hx * w, y0 = y - hy * h;
    float x1 = (1.0f - hx) * w + x, y1 = (1.0f - hy) * h + y;
    rendVERTEX v[4];
    memset(v, 0, sizeof(v));
    v[0].x = x0; v[0].y = y0; v[0].z = cam->zNear; v[0].u = 0; v[0].v = 0;
    v[1].x = x0; v[1].y = y1; v[1].z = cam->zNear; v[1].u = 0; v[1].v = 1.0f;
    v[2].x = x1; v[2].y = y1; v[2].z = cam->zNear; v[2].u = 1.0f; v[2].v = 1.0f;
    v[3].x = x1; v[3].y = y0; v[3].z = cam->zNear; v[3].u = 1.0f; v[3].v = 0;
    cfg.color = color;
    rendDrv->_RenderPolyCompat(cam, 0, 0, gssScreenDX, gssScreenDY, 4, v, &cfg);
}
