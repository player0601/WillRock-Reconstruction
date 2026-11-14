// ui_rnd.cpp -- gs.lib UI drawing: page rectangles of UI objects, render elements (rectangle / 3D object +
// text), the align flags
// (Will Rock 1.2, v12.exe; feb objs gs:ui_rnd.obj, and uiGetObjRenderRect / GetTextFlags from gs:ui_main.obj)
//
// Rebuilt from the beta decompile (the 1.2 functions are "exact" / "callgraph" matches, same offsets).
// The UI page is the "ui_top" object's size (1024 x 768 in ui.tpl); a UI object's page position is its world
// x and -world y. A 4-vertex object becomes a uiRECT: its world vertices sorted into top-left, top-right,
// bottom-right, bottom-left by the object's box (0.2 tolerance), clamped to 0..1026 x 0..769. Rects and text
// are scaled from the page to the screen by uiSystem->scale (screen / page).
// The wrUI_SYSTEM virtuals the code calls are written out: +0x58 GetFont(name) = fontManager->GetFont,
// +0x5C GetDefaultFont = GetFont(GetFontName(1)) ("font_b"), +0x60 the default align (returns 1, centerx),
// +0x64 GetTextFlags. Not rebuilt yet: SetAlpha / SetAlphaText (elements and render info).

#include "../engine/engine.h"
#include "../../include/engine_render/texture.h"
#include "../../include/engine_render/rend_driver.h"
#include "../../include/game_systems/strings.h"
#include "../../include/frontend/ui_core.h"
#include "../../include/frontend/wr_ui.h"
#include <wchar.h>

// The parts of uiSystem (a wrUI_SYSTEM) read here
static inline float uiPageW() { return ((uiUI_SYSTEM *)uiSystem)->screenW; }
static inline float uiPageH() { return ((uiUI_SYSTEM *)uiSystem)->screenH; }
static inline m2dV  uiScale() { return ((uiUI_SYSTEM *)uiSystem)->scale; }
char uiFontNames[3][0x40] = { "font_s", "font_b", "font_h" }; // wrUI_SYSTEM::GetFontName v12 0x0048B750 defaults ([Language] tplFontS/B/H)
static uiFONT *uiSysGetFont(const char *name) { return fontManager->GetFont(name); }       // slot +0x58 v12 0x0048D5C0
static uiFONT *uiSysGetDefaultFont() { return uiSysGetFont(uiFontNames[1]); }            // slot +0x5C v12 0x0048F7D0
static const unsigned uiDefaultAlign = 1;                                                 // slot +0x60 v12 0x004031F0

// v12 0x00519CD0  (feb 0x00504910) uiUI_SYSTEM::GetTextFlags
unsigned __fastcall uiGetTextFlags(const char *s)
{
    unsigned f = 0;
    if (!strstr(s, "left")) {
        if (strstr(s, "centerx"))
            f = 1;
        else if (strstr(s, "right"))
            f = 2;
    }
    if (strstr(s, "top"))
        f |= 4;
    else if (strstr(s, "centery"))
        f |= 0xC;
    else if (strstr(s, "bottom"))
        f |= 8;
    else
        strstr(s, "baseline");                          // (looked for, no bit)
    if (strstr(s, "fixed"))
        f |= 0x10;
    return f;
}

// v12 0x0051B3C0  (feb 0x00505FE0)
void __fastcall uiUCoord2SCoord(m2dV *in, m2dV *out)
{
    out->x = uiPageW() * in->x;
    out->y = uiPageH() * in->y;
}

// World position of vertex i (x87 path of the game: row vectors through the object's ltm)
static inline void uiWorldVert(objOBJ *o, int i, m3dV *out)
{
    m3dV *v = &o->geom->vert[i];
    if (o->state & 1) {                                 // (the vertices are already world)
        *out = *v;
        return;
    }
    const float *m = o->ltm;
    out->x = v->x * m[0] + v->y * m[4] + v->z * m[8] + m[12];
    out->y = v->x * m[1] + v->y * m[5] + v->z * m[9] + m[13];
    out->z = v->x * m[2] + v->y * m[6] + v->z * m[10] + m[14];
}

// v12 0x0051AD90  (feb 0x005059B0)
void __fastcall uiGetObjRenderRect(objOBJ *o, uiVERTEX *out)
{
    float bmin[3], bmax[3];
    if (!o->child) {                                    // the object's (world) box
        bmin[0] = o->bbox.min[0]; bmin[1] = o->bbox.min[1]; bmin[2] = o->bbox.min[2];
        bmax[0] = o->bbox.max[0]; bmax[1] = o->bbox.max[1]; bmax[2] = o->bbox.max[2];
    } else {                                            // with children: the box of its own 4 vertices
        bmin[0] = bmin[1] = bmin[2] = 1e30f;            // (m3dBOX::CalcStart)
        bmax[0] = bmax[1] = bmax[2] = -1e30f;
        for (int i = 0; i < 4; i++) {
            m3dV p;
            uiWorldVert(o, i, &p);
            float c[3] = { p.x, p.y, p.z };
            for (int k = 0; k < 3; k++) {               // (m3dBOX::CalcUpdate)
                if (c[k] < bmin[k]) bmin[k] = c[k];
                if (c[k] > bmax[k]) bmax[k] = c[k];
            }
        }
    }
    static const float zero[8] = { 0 };
    const float *uv = o->geom->texCoord[0] ? (const float *)o->geom->texCoord[0] : zero;
    const float eps = 0.2f;                             // __real@3e4ccccd
    for (int i = 0; i < 4; i++, uv += 2) {
        m3dV p;
        uiWorldVert(o, i, &p);
        int slot;
        if (fabsf(p.x - bmin[0]) < eps && fabsf(p.y - bmin[1]) < eps)
            slot = 3;                                   // bottom-left
        else if (fabsf(p.x - bmax[0]) < eps && fabsf(p.y - bmax[1]) < eps)
            slot = 1;                                   // top-right
        else if (fabsf(p.x - bmax[0]) < eps)
            slot = 2;                                   // bottom-right
        else
            slot = 0;                                   // top-left
        out[slot].x = p.x;
        out[slot].y = -p.y;
        out[slot].u = uv[0];
        out[slot].v = uv[1];
    }
    for (int i = 0; i < 4; i++) {
        if (out[i].x < 2.0f)
            out[i].x = 0;
        else if (out[i].x > 1022.0f)
            out[i].x = 1026.0f;
        if (out[i].y < 2.0f)
            out[i].y = 0;
        if (out[i].y > 766.0f)
            out[i].y = 769.0f;
    }
}

// v12 0x00528D50  (feb 0x005138B0)
void uiRECT::Render(camCAMERA *cam)
{
    m2dV sc = uiScale();
    rendCFG cfg;
    rendVERTEX v[4];
    cfg.color = color;
    cfg.flags = 0x80AF;
    for (int i = 0; i < 4; i++) {
        float x = this->v[i].x, y = this->v[i].y;
        if (x < 0)
            x = 0;
        else if (x > 1026.0f)
            x = 1026.0f;
        if (y < 0)
            y = 0;
        else if (y > 769.0f)
            y = 769.0f;
        v[i].u = this->v[i].u;
        v[i].v = this->v[i].v;
        v[i].x = sc.x * x;
        v[i].y = sc.y * y;
        v[i].z = 0.1f;                                  // (left as on the stack in the game)
        v[i].rhw = 1.0f;
    }
    cfg.InitMtl1(tex);
    rendDrv->RenderPolyCompat(cam, 4, v, &cfg);
}

// v12 0x00528F40  (feb 0x00513AA0)
uiRENDER_ELEM::uiRENDER_ELEM()
{
    name[0] = 0;
    flags40 = 0;
    obj = 0;
    rect.tex = 0;
    rect.color = 0xFFFFFFFF;
    font = 0;
    textId = 1;                                         // the empty string
    textCap = 0;
    fontSize.x = 1.0f;
    fontSize.y = 1.0f;
    align = 0;
    colorText = 0xFFFFFFFF;
}

static inline unsigned uiPackColor(m3dCOLOR *c)
{
    return ((unsigned)(int)c->a << 24) | (((unsigned)(int)c->r & 0xFF) << 16) | (((unsigned)(int)c->g & 0xFF) << 8)
         | ((unsigned)(int)c->b & 0xFF);
}

// v12 0x00529170  (feb 0x00513CD0)
void uiRENDER_ELEM::Init(objOBJ *o)
{
    ((apNAME *)name)->SetName(o->name);
    psSHEET *ps = o->props;
    int b;
    if (ps && ps->GetBool((char *)"UI_PROP", (char *)"objHIDDEN", &b) && b == 1)
        flags40 |= UIRF_HIDDEN;
    if ((o->props && o->props->GetBool((char *)"UI_PROP", (char *)"obj3D", &b) && b == 1) || o->nVert != 4) {
        obj = o;
        flags40 |= UIRF_OBJ3D;
        o->UpdRenderState();
        return;
    }
    rect.tex = o->GetFaceTexRendMtl(0, 0);
    uiGetObjRenderRect(o, rect.v);
    m3dCOLOR col;
    if (o->props && o->props->GetColor((char *)"UI_PROP", (char *)"color", &col))
        rect.color = uiPackColor(&col);
    else
        rect.color = 0xFFFFFFFF;
    char s[0x100];
    if (!ps) {
        font = uiSysGetDefaultFont();
        align = uiDefaultAlign;
        return;
    }
    if (ps->GetStr((char *)"UI_PROP", (char *)"textID", s, 0x100)) {
        flags40 |= UIRF_TEXT;
        int id = gsStrings->GetStringId(s);
        if (id == 0) {                                  // unknown id: shown as "!<id>"
            memmove(s + 1, s, strlen(s) + 1);
            s[0] = '!';
            const wrWCHAR *w = gsStrings->AsciiToUnicode(s);
            int n = (int)wcslen(w) + 1;
            if (!textCap) {
                textCap = n;
                text = (wrWCHAR *)apMalloc(n * 2);
            } else if (textCap < n) {
                textCap = n;
                text = (wrWCHAR *)apRealloc(text, n * 2);
            }
            memcpy(text, w, (n - 1) * 2);
            text[n - 1] = 0;
        } else {
            if (textCap) {
                apFree(text);
                textCap = 0;
            }
            textId = id;
        }
    }
    if (ps->GetStr((char *)"UI_PROP", (char *)"textNEW", s, 0x100)) {
        flags40 |= UIRF_TEXT;
        int id = gsStrings->GetStringId(s);
        if (textCap) {
            apFree(text);
            textCap = 0;
        }
        textId = id;
    }
    if (ps->GetStr((char *)"UI_PROP", (char *)"fontID", s, 0x100))
        font = uiSysGetFont(s);
    else
        font = uiSysGetDefaultFont();
    if (ps->GetStr((char *)"UI_PROP", (char *)"align", s, 0x100))
        align = uiGetTextFlags(s);
    else
        align = uiDefaultAlign;
    if (ps->GetColor((char *)"UI_PROP", (char *)"colorText", &col))
        colorText = uiPackColor(&col);
    float k;
    if (ps->GetFloat((char *)"UI_PROP", (char *)"scaleText", &k)) {
        fontSize.y = k;
        fontSize.x = k;
    }
}

// v12 0x005297A0  (feb 0x00514300)
m2dV *uiRENDER_ELEM::GetTextStartPos(m2dV *out)
{
    float x, y;
    switch (align & 3) {
    case 1:  x = (rect.v[1].x + rect.v[0].x) * 0.5f; break;
    case 2:  x = rect.v[1].x; break;
    default: x = rect.v[0].x; break;
    }
    switch (align & 0xC) {
    case 4:   y = rect.v[3].y; break;
    case 0xC: y = (rect.v[3].y + rect.v[1].y) * 0.5f; break;
    default:  y = rect.v[0].y; break;
    }
    out->x = x;
    out->y = y;
    return out;
}

// v12 0x005295C0  (feb 0x00514120)
void uiRENDER_ELEM::Render(camCAMERA *cam)
{
    if (!(flags40 & UIRF_HIDDEN)) {
        if (flags40 & UIRF_OBJ3D) {
            if (!obj->nFace)
                return;
            objGEOM *g = obj->geom;
            unsigned c = g->constColor;
            if ((obj->rendState & 0x40) && (c >> 24) != 0xFF) {  // see-through: colour times its alpha, opaque
                double a = (double)(c >> 24) * (double)0.0039215688593685627f;   // __real@3b808081, kept on the FPU
                unsigned r = (unsigned)(int)((double)((c >> 16) & 0xFF) * a);
                unsigned gg = (unsigned)(int)((double)((c >> 8) & 0xFF) * a);
                unsigned bb = (unsigned)(int)((double)(c & 0xFF) * a);
                c = (((r | 0xFFFFFF00) << 8 | (gg & 0xFF)) << 8) | (bb & 0xFF);
            }
            g->unk62 = (int)c;
            rendDrv->RenderObj(obj, 0, 0);
            return;
        }
        rect.Render(cam);
    }
    if (flags40 & UIRF_TEXT) {
        m2dV sc = uiScale(), p, end;
        GetTextStartPos(&p);
        m2dV fs = { sc.x * fontSize.x, sc.y * fontSize.y };
        const wrWCHAR *s = textCap ? text : gsStrings->GetStringById(textId);
        font->PrintW(&end, cam, s, p.x * sc.x, p.y * sc.y, colorText, &fs, align);
    }
}

// v12 0x00529DB0  (feb 0x00514910)
void uiRENDER_INFO::Init(objOBJ *o)
{
    uiRENDER_ELEM::Init(o);
    objOBJ *c = o->child;
    if (c)
        do {
            if (nElems + 1 > capElems) {
                int n = capElems * 2;
                if (n <= nElems + 1)
                    n = nElems + 1;
                elems = (uiRENDER_ELEM *)apRealloc(elems, n * sizeof(uiRENDER_ELEM));
                capElems = n;
            }
            new (&elems[nElems]) uiRENDER_ELEM;
            nElems++;
            elems[nElems - 1].Init(c);
            c = c->next;
        } while (c != o->child);
    if (nElems != capElems) {
        elems = (uiRENDER_ELEM *)apRealloc(elems, nElems * sizeof(uiRENDER_ELEM));
        capElems = nElems;
    }
}

// v12 0x00529FB0  (feb 0x00514B10)
void uiRENDER_INFO::Render(camCAMERA *cam)
{
    uiRENDER_ELEM::Render(cam);
    for (int i = 0; i < nElems; i++)
        if (!(elems[i].flags40 & UIRF_SKIP))
            elems[i].Render(cam);
}

// v12 0x00529860  (feb 0x005143C0)
// Move the rectangle so its centre is at c.
void uiRENDER_ELEM::MoveTo(m2dV *c)
{
    float cx = (rect.v[2].x + rect.v[0].x) * 0.5f, cy = (rect.v[2].y + rect.v[0].y) * 0.5f;
    for (int i = 0; i < 4; i++) {
        rect.v[i].x = (c->x - cx) + rect.v[i].x;
        rect.v[i].y = (c->y - cy) + rect.v[i].y;
    }
}

// v12 0x005298C0  (feb 0x00514420)
// The rectangle (or the 3D object's box, in page units), grown by the text's box unless the text is "fixed" (0x10).
void uiRENDER_ELEM::CalcBBox(uiBOX_ *out)
{
    if (!obj) {
        out->min.x = rect.v[0].x;
        out->min.y = rect.v[0].y;
        out->max.x = rect.v[2].x;
        out->max.y = rect.v[2].y;
    } else {
        out->min.x = obj->bbox.min[0];
        out->min.y = -obj->bbox.max[1];
        out->max.x = obj->bbox.max[0];
        out->max.y = -obj->bbox.min[1];
    }
    if ((flags40 & UIRF_TEXT) && !(flags40 & UIRF_10)) {
        m2dV fs = fontSize, mn, mx, p;
        const wrWCHAR *s = textCap ? text : gsStrings->GetStringById(textId);
        GetTextStartPos(&p);
        font->GetStringRectW(p.x, p.y, s, &fs, align, &mn, &mx);
        out->AddVertex(&mn);
        out->AddVertex(&mx);
    }
}

// v12 0x005299C0  (feb 0x00514520)
void uiRENDER_ELEM::SetTextId(int strId)
{
    if (textCap) {
        apFree(text);
        text = 0;
        textCap = 0;
    }
    textId = strId;
}

// v12 0x00529A10  (feb 0x00514570)
// An own copy of at most maxLen characters; the buffer only grows.
void uiRENDER_ELEM::SetTextW(const wrWCHAR *s, int maxLen)
{
    int n = (int)wcslen((const wchar_t *)s);
    if (maxLen < n)
        n = maxLen;
    int need = n + 1 > 0 ? n + 1 : 0;
    if (!textCap) {
        textCap = need;
        text = (wrWCHAR *)apMalloc(need * 2);
    } else if (textCap < need) {
        textCap = need;
        text = (wrWCHAR *)apRealloc(text, need * 2);
    }
    memcpy(text, s, n * 2);
    text[n] = 0;
}

// v12 0x00529AB0  (feb 0x00514610)
// Append at most maxLen characters; a string-table text is first turned into an own copy.
void uiRENDER_ELEM::AppendTextW(const wrWCHAR *s, int maxLen)
{
    int n = (int)wcslen((const wchar_t *)s);
    if (maxLen < n)
        n = maxLen;
    int cur, need;
    if (textCap) {
        cur = (int)wcslen((const wchar_t *)text);
        need = cur + 1 + n;
        if (textCap < need) {
            textCap = need;
            text = (wrWCHAR *)apRealloc(text, need * 2);
        }
    } else {
        const wrWCHAR *str = gsStrings->GetStringById(textId);
        cur = (int)wcslen((const wchar_t *)str);
        need = cur + 1 + n;
        int len = cur + 1, cap = need < len ? len : need;
        textCap = cap;
        text = (wrWCHAR *)apMalloc(cap * 2);
        memcpy(text, str, (len - 1) * 2);
        text[len - 1] = 0;
    }
    memcpy(text + cur, s, n * 2);
    text[need - 1] = 0;
}

// (SetElementIdx's inlined copy) everything member-wise; an own text is duplicated, a string id is shared
void uiRENDER_ELEM::CopyFrom(const uiRENDER_ELEM *src)
{
    memcpy(name, src->name, sizeof(name));
    flags40 = src->flags40;
    obj = src->obj;
    rect = src->rect;
    font = src->font;
    if (!src->textCap) {
        if (textCap)
            apFree(text);
        textCap = 0;
        textId = src->textId;
    } else
        SetTextW(src->text, 0x7FFFFFFF);
    fontSize = src->fontSize;
    align = src->align;
    colorText = src->colorText;
}

// v12 0x0052A000  (feb 0x00514B60)
void uiRENDER_INFO::SetFont(uiFONT *f)
{
    if (font == f)
        return;
    font = f;
    for (int i = 0; i < nElems; i++)
        elems[i].font = f;
}

// v12 0x0052A050  (feb 0x00514BB0)
void uiRENDER_INFO::SetColor(unsigned long c)
{
    rect.color = c;
    for (int i = 0; i < nElems; i++)
        elems[i].rect.color = c;
}

// v12 0x0052A090  (feb 0x00514BF0)
void uiRENDER_INFO::SetColorText(unsigned long c)
{
    colorText = c;
    for (int i = 0; i < nElems; i++)
        elems[i].colorText = c;
}

// v12 0x0052A0D0  (feb 0x00514C30)
void uiRENDER_INFO::SetTex(txmTEXTURE *t)
{
    rect.tex = t;
    for (int i = 0; i < nElems; i++)
        elems[i].rect.tex = t;
}

// v12 0x0052A460  (feb 0x00514FC0)
void uiRENDER_INFO::CalcBBox(uiBOX_ *out)
{
    uiBOX_ b;
    b.min = m2dVZero;
    b.max = m2dVZero;
    uiRENDER_ELEM::CalcBBox(out);
    for (int i = 0; i < nElems; i++) {
        elems[i].CalcBBox(&b);
        out->AddVertex(&b.min);
        out->AddVertex(&b.max);
    }
}

// v12 0x0052A4F0  (feb 0x00515050)
uiRENDER_ELEM *uiRENDER_INFO::FindElement(const char *n)
{
    if (!strcmp(name, n))
        return this;
    for (int i = 0; i < nElems; i++)
        if (!strcmp(elems[i].name, n))
            return &elems[i];
    return 0;
}

// v12 0x0052A5C0  (feb 0x00515120)
// Swap the element named n into slot idx (so code can address "text" / "icon" by number). Nothing when absent.
void uiRENDER_INFO::SetElementIdx(const char *n, int idx)
{
    for (int i = 0; i < nElems; i++) {
        if (strcmp(elems[i].name, n))
            continue;
        if (i == idx)
            return;
        uiRENDER_ELEM tmp;
        tmp.CopyFrom(&elems[i]);
        elems[i].CopyFrom(&elems[idx]);
        elems[idx].CopyFrom(&tmp);
        if (tmp.textCap)
            apFree(tmp.text);
        return;
    }
}

// The colour with a new alpha: r, g, b go through float and back (ftol) as in the original; the alpha is not
// masked (a value over 255 is shifted out)
static inline unsigned uiWithAlpha(unsigned c, float a)
{
    unsigned r = (c >> 16) & 0xFF, g = (c >> 8) & 0xFF, b = c & 0xFF;
    return ((((unsigned)(int)(float)r & 0xFF) | (unsigned)(int)a << 8) << 8 | ((unsigned)(int)(float)g & 0xFF)) << 8
         | ((unsigned)(int)(float)b & 0xFF);
}

// v12 0x00529C40  (feb 0x005147A0)
void uiRENDER_ELEM::SetAlpha(float a)
{
    rect.color = uiWithAlpha(rect.color, a);
}

// v12 0x00529CF0  (feb 0x00514850)
void uiRENDER_ELEM::SetAlphaText(float a)
{
    colorText = uiWithAlpha(colorText, a);
}

// v12 0x0052A110  (feb 0x00514C70)
// A 3D object gets the alpha as its transparency. NB (original, both builds): for the child elements the test and
// the object are this render info's own, not the element's, so a 3D info sets its object n + 1 times and leaves
// the children's colours alone.
void uiRENDER_INFO::SetAlpha(float a)
{
    if (flags40 & UIRF_OBJ3D)
        obj->SetTransparency(0x80, (u8)(int)a);
    else
        rect.color = uiWithAlpha(rect.color, a);
    for (int i = 0; i < nElems; i++) {
        if (flags40 & UIRF_OBJ3D)
            obj->SetTransparency(0x80, (u8)(int)a);
        else
            elems[i].rect.color = uiWithAlpha(elems[i].rect.color, a);
    }
}

// v12 0x0052A2D0  (feb 0x00514E30)
void uiRENDER_INFO::SetAlphaText(float a)
{
    colorText = uiWithAlpha(colorText, a);
    for (int i = 0; i < nElems; i++)
        elems[i].colorText = uiWithAlpha(elems[i].colorText, a);
}

// v12 0x00528E70  (feb 0x005139D0)
wrWCHAR *uiRENDER_ELEM_TEXT::GetString()
{
    if (!textCap)
        return (wrWCHAR *)gsStrings->GetStringById(textId);
    return text;
}

// v12 0x00529DA0  (feb 0x00514900)
void uiRENDER_ELEM::SetTex(txmTEXTURE *t)
{
    rect.tex = t;
}
