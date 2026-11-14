// ui_font.cpp -- gs.lib UI bitmap fonts built from a template (font_s / font_b / font_h .tpl)
// (Will Rock 1.2, v12.exe; feb obj gs:ui_font.obj)
//
// Rebuilt from the beta decompile (1.2: all "exact" matches). A font template has an object "ui_font" whose
// children "page_<first char>" hold lines (any child with children); each line's children are the letter
// quads, in the font texture. Letters of a page are numbered from <first char> in reading order: lines from
// the top, letters by x. A letter keeps its page-unit quad, moved to x = 0 at its left edge and y = 0 at the
// line's top. Missing characters print as letter 0 of page 0. Space advances without drawing.
// Kept: GetStringSizeW / AdjustStringPosW only step the string in proportional mode (the "fixed" flag 0x10
// would loop forever; no UI text uses it); uiFONT_MANAGER::CreateFont needs the level's template loader
// (gsSysLevel), so the test builds fonts with uiFONT::Create and links them into the list itself.

#include "../engine/engine.h"
#include "../../include/engine_render/texture.h"
#include "../../include/engine_render/rend_driver.h"
#include "../../include/frontend/ui_core.h"

template <class T> static void fntGrow(arrVector<T> *a, int need)
{
    if (a->capacity < need) {
        int c = a->capacity * 2;
        if (c <= need)
            c = need;
        a->data = (T *)apRealloc(a->data, c * sizeof(T));
        a->capacity = c;
    }
}
template <class T> static void fntShrink(arrVector<T> *a)
{
    if (a->count != a->capacity) {
        a->data = (T *)apRealloc(a->data, a->count * sizeof(T));
        a->capacity = a->count;
    }
}

// v12 0x0052C750  (feb 0x00517290)
uiFONT_PAGE::uiFONT_PAGE()
{
    tex = 0;
    first = 0;
    letters.data = 0;
    letters.count = 0;
    letters.capacity = 0;
}

static inline BOOL32 fntIsPage(const char *n)
{
    return strlen(n) > 5 && !strncmp(n, "page_", 5);
}

// v12 0x0052C770  (feb 0x005172B0)
void uiFONT::Create(objOBJ *o)
{
    pages.count = 0;
    objOBJ *c = o ? o->child : 0;
    if (c)
        do {
            if (c->name && fntIsPage(c->name))
                AddPage(atoi(c->name + 5), c);
            c = c->next;
        } while (c != o->child);
    for (int i = 1; i < pages.count; i++)              // arrVector<uiFONT_PAGE>::Sort (by first)
        for (int j = i; j > 0 && pages.data[j - 1].first > pages.data[j].first; j--) {
            uiFONT_PAGE t = pages.data[j];
            pages.data[j] = pages.data[j - 1];
            pages.data[j - 1] = t;
        }
    ascent = 0;
    descent = 0;
    for (int p = 0; p < pages.count; p++)
        for (int i = 0; i < pages.data[p].letters.count; i++) {
            uiFONT_LETTER *l = &pages.data[p].letters.data[i];
            if (-l->v[0].y > ascent)
                ascent = -l->v[0].y;
            if (l->v[2].y > descent)
                descent = l->v[2].y;
        }
}

struct uiFONT_TMP_LINE { float top; objOBJ *obj; };

// v12 0x0052C8F0  (feb 0x00517430)
void uiFONT::AddPage(int firstChar, objOBJ *o)
{
    arrVector<uiFONT_TMP_LINE> lines = { 0, { 0 }, 0 };
    fntGrow(&pages, pages.count + 1);
    new (&pages.data[pages.count]) uiFONT_PAGE;
    pages.count++;
    uiFONT_PAGE *page = &pages.data[pages.count - 1];
    objOBJ *c = o->child;
    if (c)
        do {
            if (c->child) {
                fntGrow(&lines, lines.count + 1);
                uiFONT_TMP_LINE *l = &lines.data[lines.count++];
                l->obj = c;
                l->top = 0;
                if (c->nVert) {                         // the top of the line: min of -world y
                    for (int i = 0; i < c->nVert; i++) {
                        m3dV *v = &c->geom->vert[i];
                        float y;
                        if (c->state & 1)
                            y = v->y;
                        else
                            y = v->x * c->ltm[1] + v->y * c->ltm[5] + v->z * c->ltm[9] + c->ltm[13];
                        if (i == 0 || -y < l->top)
                            l->top = -y;
                    }
                }
            }
            c = c->next;
        } while (c != o->child);
    for (int i = 1; i < lines.count; i++)              // arrStdCmp<float>: by top
        for (int j = i; j > 0 && lines.data[j - 1].top > lines.data[j].top; j--) {
            uiFONT_TMP_LINE t = lines.data[j];
            lines.data[j] = lines.data[j - 1];
            lines.data[j - 1] = t;
        }
    for (int i = 0; i < lines.count; i++)
        AddLettersLine(page, lines.data[i].obj, lines.data[i].top);
    if (page->letters.count == 0)
        pages.count--;                                  // an empty page is dropped
    else {
        fntShrink(&page->letters);
        page->first = firstChar;
    }
    apFree(lines.data);
}

// v12 0x0052CE80  (feb 0x005179C0)
void uiFONT::AddLettersLine(uiFONT_PAGE *page, objOBJ *line, float top)
{
    arrVector<uiFONT_LETTER> tmp = { 0, { 0 }, 0 };
    objOBJ *c = line ? line->child : 0;
    if (c)
        do {
            txmTEXTURE *t = c->GetFaceTexRendMtl(0, 0);
            if (!page->tex)
                page->tex = t;
            fntGrow(&tmp, tmp.count + 1);
            uiGetObjRenderRect(c, tmp.data[tmp.count++].v);
            c = c->next;
        } while (c != line->child);
    for (int i = 1; i < tmp.count; i++)                // by left x
        for (int j = i; j > 0 && tmp.data[j - 1].v[0].x > tmp.data[j].v[0].x; j--) {
            uiFONT_LETTER t = tmp.data[j];
            tmp.data[j] = tmp.data[j - 1];
            tmp.data[j - 1] = t;
        }
    for (int i = 0; i < tmp.count; i++) {
        uiFONT_LETTER l = tmp.data[i];
        float x0 = l.v[0].x;
        for (int k = 0; k < 4; k++) {
            l.v[k].x -= x0;
            l.v[k].y -= top;
        }
        fntGrow(&page->letters, page->letters.count + 1);
        page->letters.data[page->letters.count++] = l;
    }
    apFree(tmp.data);
}

// (inlined in every user) binary search of the page holding c
uiFONT_LETTER *uiFONT::GetLetter(int c)
{
    int lo = 0, hi = pages.count;
    while (lo != hi) {
        int mid = (hi + lo) >> 1;
        uiFONT_PAGE *p = &pages.data[mid];
        if (p->first <= c) {
            if (c < p->letters.count + p->first)
                return &p->letters.data[c - p->first];
            lo = mid + 1;
        } else
            hi = mid;
    }
    return &pages.data[0].letters.data[0];
}

static inline float fntAdvance(uiFONT *f, const wrWCHAR *s, int flags)
{
    float w = 0;
    for (; *s; s++)
        w += (flags & 0x10) ? f->fixedW : f->GetLetter(*s)->v[2].x - f->GetLetter(*s)->v[0].x;
    return w;
}

// v12 0x0052D090  (feb 0x00517BD0)
void uiFONT::GetStringSizeW(m2dV *out, const wrWCHAR *s, const m2dV *scale, int flags)
{
    float w = fntAdvance(this, s, flags);
    out->x = w * scale->x;
    out->y = (descent + ascent) * scale->y;
}

// v12 0x0052D180  (feb 0x00517CC0)
void uiFONT::AdjustStringPosW(m2dV *pos, const wrWCHAR *s, const m2dV *scale, int flags)
{
    if ((flags & 3) == 1)
        pos->x -= fntAdvance(this, s, flags) * scale->x * 0.5f;
    else if ((flags & 3) == 2)
        pos->x -= fntAdvance(this, s, flags) * scale->x;
    switch (flags & 0xC) {
    case 4:
        pos->y = ascent * scale->y + pos->y;
        break;
    case 8:
        pos->y = pos->y - descent * scale->y;
        break;
    case 0xC:
        pos->y = ((ascent + descent) * 0.5f - descent) * scale->y + pos->y;
        break;
    }
}

// v12 0x0052D3C0  (feb 0x00517F00)
// The box of a string printed at (x, y) with these align flags: from the aligned start, ascent above, descent below.
void uiFONT::GetStringRectW(float x, float y, const wrWCHAR *s, const m2dV *scale, int flags, m2dV *min, m2dV *max)
{
    m2dV p = { x, y };
    AdjustStringPosW(&p, s, scale, flags);
    float w = fntAdvance(this, s, flags);
    min->x = p.x;
    min->y = p.y - ascent * scale->y;
    max->x = w * scale->x + p.x;
    max->y = descent * scale->y + p.y;
}

// v12 0x0052D4C0  (feb 0x00518000)
void uiFONT::PrintW(m2dV *end, camCAMERA *cam, const wrWCHAR *s, float x, float y, unsigned long color,
                    const m2dV *scale, int flags)
{
    m2dV p = { x, y };
    AdjustStringPosW(&p, s, scale, flags);
    rendCFG cfg;
    rendVERTEX v[4];
    cfg.flags = 0x4080AF;
    cfg.color = (unsigned)color;
    txmTEXTURE *cur = 0;
    for (int i = 0; i < 4; i++) {
        v[i].z = 0.1f;
        v[i].rhw = 1.0f;
    }
    for (; *s; s++) {
        int c = *s;
        uiFONT_PAGE *page = &pages.data[0];
        uiFONT_LETTER *l = &page->letters.data[0];
        int lo = 0, hi = pages.count;                   // (GetLetter, plus the page for its texture)
        while (lo != hi) {
            int mid = (hi + lo) >> 1;
            uiFONT_PAGE *q = &pages.data[mid];
            if (q->first <= c) {
                if (c < q->letters.count + q->first) {
                    page = q;
                    l = &q->letters.data[c - q->first];
                    break;
                }
                lo = mid + 1;
            } else
                hi = mid;
        }
        if (cur != page->tex) {
            cfg.InitMtl1(page->tex);
            cur = page->tex;
            // (the game also calls rendDrv vtable +0x50 (cfg, 0) here: set the render state once per texture)
        }
        for (int k = 0; k < 4; k++) {
            v[k].x = l->v[k].x * scale->x + p.x;
            v[k].y = l->v[k].y * scale->y + p.y;
            v[k].u = l->v[k].u;
            v[k].v = l->v[k].v;
        }
        if (c != ' ')
            rendDrv->RenderPolyCompat(cam, 4, v, &cfg);
        p.x += (flags & 0x10) ? fixedW * scale->x : v[2].x - v[0].x;
    }
    end->x = p.x;
    end->y = p.y;
}

// v12 0x0052D6C0  (feb 0x00518200)
uiFONT *uiFONT_MANAGER::GetFont(const char *name)
{
    for (uiFONT *f = first; f; f = f->next)
        if (!strcmp(f->name, name))
            return f;
    return 0;
}
