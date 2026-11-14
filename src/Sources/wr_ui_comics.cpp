// wr_ui_comics.cpp -- the comics: the logo, the intro, the outro (Will Rock 1.2, v12.exe; feb obj wr_ui_comics.obj)
//
// wrUI_COMICS_FRAME::Start reads a .cms (texts\ati_logo.cms, intro.cms, outro.cms: willrock.ini [Language] cmsLogo,
// cmsIntro, cmsOutro) into a sheet and shows the page "comics_frame". Its area "comics" (wrUI_COMICS) has ui.tpl's
// frames as render elements -- the pictures (big_frame, frame_full_1 ...) first, then the text lines (btext_1_01 ..)
// -- and plays the sheet's sections in the order GetNextSection walks them: EVENT_1, EVENT_1_1, EVENT_1_2, EVENT_2 ...
// Each event can clear (ClearPic, ClearText, FadeOut), show a picture (ShowPic + PicTexture) or text (ShowText +
// TextId, a line per frame), fading in / out (FadeInTime, FadeOutTime), start a sound (Sound, on channel SoundId)
// or the music (Music intro / outro); the next one waits for Delay, a sound (WaitSound) and the fades. The developers'
// notes on the keys: texts\comics (December 2002; there the picture key is still "ShowFrame").

#include "game.h"

// v12 0x004914E0  (feb 0x004863A0)
// The elements by height, a text line (render flag 2) below every picture.
static int __cdecl _compareRectY(const void *a, const void *b)
{
    const uiRENDER_ELEM *x = (const uiRENDER_ELEM *)a, *y = (const uiRENDER_ELEM *)b;
    float fx = ((x->flags40 & 2) ? 10000.0f : 0.0f) + x->rect.v[0].y;
    float fy = ((y->flags40 & 2) ? 10000.0f : 0.0f) + y->rect.v[0].y;
    if (fx < fy)
        return -1;
    return fx == fy ? 0 : 1;
}

static void comicsElemsResize(wrUI_COMICS *c, int n)        // arrVector<wrUI_COMICS_ELEM>::Realloc (v12 0x004927D0)
{
    if (n > c->nElem && n > c->capElem) {
        c->elem = (wrUI_COMICS_ELEM *)apRealloc(c->elem, n * sizeof(wrUI_COMICS_ELEM));
        c->capElem = n;
    }
    c->nElem = n;
}

uiELEMENT *__fastcall wrUI_COMICS::Create()
{
    wrUI_COMICS *c = new wrUI_COMICS;
    if (!c)
        return 0;
    c->snd = 0;
    c->nSnd = c->capSnd = 0;
    for (int i = 0; i < 4; i++)
        c->sndHandle[i] = -1;
    c->elem = 0;
    c->nElem = c->capElem = 0;
    c->nPic = 0;
    c->fast = 0;
    c->event[0] = 0;
    c->waitSnd = 0;
    c->delay = 0;
    c->sheet = 0;
    c->ownSheet = 0;
    return c;
}

// v12 0x004913F0  (feb 0x004862B0)
// The elements sorted (pictures, then text lines), all hidden; nPic counts the pictures.
int wrUI_COMICS::ProcessInit(void *p)
{
    if (!uiAREA::ProcessInit(p))
        return 0;
    qsort(renderInfo.elems, renderInfo.nElems, sizeof(uiRENDER_ELEM), _compareRectY);
    comicsElemsResize(this, renderInfo.nElems);
    for (int i = 0; i < nElem; i++) {
        elem[i].state = 3;
        renderInfo.elems[i].flags40 |= 8;
    }
    int i = nElem - 1;
    while (i >= 0 && (renderInfo.elems[i].flags40 & 2))
        i--;
    waitSnd = 0;
    nPic = i + 1;
    return 1;
}

// v12 0x00491650  (feb 0x00486410)
// Shown or hidden: what was playing stops; hidden, the menu music fades.
int wrUI_COMICS::StartAppearing(int how)
{
    Stop(0);
    if (!how)
        ((wrUI_SYSTEM *)uiSystem)->PlayAmbientMusic(-1);
    return uiAREA::StartAppearing(how);
}

// v12 0x00491690  (feb 0x00486440)
// The fades run (5 times as fast with 'fast'); the sounds waited for are checked; with nothing running and the delay
// over, the next event.
int wrUI_COMICS::ProcessUpdate(void *p)
{
    if (!sheet || (flags & UIEF_APPEARING))
        return uiAREA::ProcessUpdate(p);
    int fading = 0, fadingOut = 0;
    float dt = gsElapsedTime;
    if (fast) {
        dt = gsElapsedTime * 5.0f;
        delay = 0;
    }
    for (int i = 0; i < nElem; i++) {
        wrUI_COMICS_ELEM *e = &elem[i];
        uiRENDER_ELEM *r = &renderInfo.elems[i];
        float k;
        if (e->state == 1) {
            e->time += dt;
            if (e->time < e->len) {
                k = e->time / e->len;
                fading = 1;
            } else {
                e->state = 0;
                e->time = e->len;
                k = 1.0f;
            }
        } else if (e->state == 2) {
            e->time += dt;
            if (e->time < e->len) {
                fadingOut = 1;
                k = 1.0f - e->time / e->len;
            } else {
                e->time = e->len;
                k = 0;
                RemoveElement(i, 0);
            }
        } else {
            continue;
        }
        r->SetAlpha((float)e->alpha * k);
        r->SetAlphaText((float)e->alphaText * k);
    }
    wrUI_SYSTEM *ui = (wrUI_SYSTEM *)uiSystem;
    for (int n = 0; n < 4; n++) {
        if (sndHandle[n] != -1 && !ui->IsSndPlaying(sndHandle[n]))
            sndHandle[n] = -1;
        if (sndHandle[n] == -1)
            waitSnd &= ~(1u << n);
        else if (waitSnd & (1u << n))
            fading = 1;
    }
    if (!fading) {
        if (delay > 0) {
            delay -= dt;
            return uiAREA::ProcessUpdate(p);
        }
        if (!fadingOut)
            ProcessNextEvent();
    }
    return uiAREA::ProcessUpdate(p);
}

// v12 0x00491910  (feb 0x004866C0)
// The sounds stopped, every element hidden; drop: the comic is over (its sheet freed).
void wrUI_COMICS::Stop(int drop)
{
    wrUI_SYSTEM *ui = (wrUI_SYSTEM *)uiSystem;
    for (int n = 0; n < 4; n++)
        if (sndHandle[n] != -1) {
            ui->StopSnd(sndHandle[n]);
            sndHandle[n] = -1;
        }
    waitSnd = 0;
    for (int i = 0; i < nElem; i++)
        RemoveElement(i, 0);
    if (drop) {
        if (ownSheet && sheet) {
            sheet->Empty();
            operator delete(sheet);
        }
        sheet = 0;
    }
    delay = 0;
}

// v12 0x004919D0  (feb 0x00486760)
// Element i hidden at once (time 0) or faded out over time (a fade already running only gets shorter).
void wrUI_COMICS::RemoveElement(int i, float time)
{
    wrUI_COMICS_ELEM *e = &elem[i];
    if (time == 0) {
        e->state = 3;
        renderInfo.elems[i].flags40 |= 8;
        return;
    }
    if (e->state == 2) {
        if (time < e->len - e->time) {
            float len = e->len;
            e->len = time;
            e->time = e->time / len * time;
        }
    } else if (e->state != 3) {
        e->state = 2;
        e->time = 0;
        e->len = time;
    }
}

// v12 0x00491A80  (feb 0x00486830)
// The text's lines into the free text elements frame_01, frame_02 ... (one taken already is skipped), each in the
// colour, fading in.
void wrUI_COMICS::AddText(char *frame, const wrWCHAR *s, unsigned long flags, const m3dCOLOR *col, float fadeIn)
{
    int base = (int)strlen(frame);
    if (!*s)
        return;
    for (int line = 1;; line++) {
        sprintf(frame + base, "_%02i", line);
        int i = nPic;
        while (i < nElem && strcmp(renderInfo.elems[i].name, frame))
            i++;
        if (i != nElem && elem[i].state != 3)
            continue;                                   // (taken: the next line number)
        if (i == nElem)
            return;
        const wrWCHAR *end = s;
        while (*end != 10 && *end)
            end++;
        uiRENDER_ELEM *r = &renderInfo.elems[i];
        wrUI_COMICS_ELEM *e = &elem[i];
        r->SetTextW(s, (int)(end - s));
        r->align = flags;
        r->colorText = ((u32)(int)col->a << 24) | (((u32)(int)col->r & 0xFF) << 16) | (((u32)(int)col->g & 0xFF) << 8) |
                       ((u32)(int)col->b & 0xFF);
        e->alpha = 0xFF;
        e->alphaText = (u8)(int)col->a;
        if (fadeIn == 0) {
            e->state = 0;
        } else {
            e->state = 1;
            e->len = fadeIn;
            e->time = 0;
            r->SetAlpha(0);
            r->SetAlphaText(0);
        }
        r->flags40 &= ~8u;
        if (!*end)
            return;
        s = end + 1;
        if (!*s)
            return;
    }
}

// v12 0x00491CE0  (feb 0x00486A90)
// The next section's keys (see the top of the file).
void wrUI_COMICS::ProcessNextEvent()
{
    char str[0x40];
    m3dCOLOR col = { 0, 0, 0, 0 };
    if (!GetNextSection()) {
        Stop(1);
        return;
    }
    wrUI_SYSTEM *ui = (wrUI_SYSTEM *)uiSystem;
    float fadeIn, fadeOut;
    if (sheet->GetFloat(event, (char *)"FadeInTime", &fadeIn))
        fadeIn *= 1.0f;                                 // (v12 0x005d9ff0: 1.0 in seconds) // ?
    else
        fadeIn = 0;
    if (sheet->GetFloat(event, (char *)"FadeOutTime", &fadeOut))
        fadeOut *= 1.0f;
    else
        fadeOut = 0;
    txmTEXTURE *tex = 0;
    if (sheet->GetStr(event, (char *)"PicTexture", str, 0x40))
        tex = txmManager->Add(str, 0, -1);
    int textId = 1;
    if (sheet->GetStr(event, (char *)"TextId", str, 0x40))
        textId = gsStrings->GetStringId(str);
    if (!sheet->GetColor(event, (char *)"TextColor", &col)) {
        col.r = col.g = col.b = 180.0f;
        col.a = 255.0f;
    }
    unsigned textFlags = sheet->GetStr(event, (char *)"TextFlags", str, 0x40) ? ui->GetTextFlags(str) : ui->_slot60();
    int sndId = 0;
    if (!sheet->GetInt(event, (char *)"SoundId", &sndId))
        sndId = 0;
    float fo;
    if (sheet->GetFloat(event, (char *)"FadeOut", &fo)) {
        for (int i = 0; i < nPic; i++)
            RemoveElement(i, fo);
        for (int i = nPic; i < nElem; i++)
            RemoveElement(i, fo);
    } else {
        if (sheet->GetStr(event, (char *)"ClearPic", str, 0x40)) {
            if (!strcmp(str, "all")) {
                for (int i = 0; i < nPic; i++)
                    RemoveElement(i, fadeOut);
            } else if (uiRENDER_ELEM *r = renderInfo.FindElement(str)) {
                RemoveElement((int)(r - renderInfo.elems), fadeOut);
            }
        }
        if (sheet->GetStr(event, (char *)"ClearText", str, 0x40))
            for (int i = nPic; i < nElem; i++)
                RemoveElement(i, fadeOut);
    }
    if (sheet->GetStr(event, (char *)"ShowPic", str, 0x40)) {
        if (uiRENDER_ELEM *r = renderInfo.FindElement(str)) {
            int i = (int)(r - renderInfo.elems);
            wrUI_COMICS_ELEM *e = &elem[i];
            if (tex) {
                r->SetTex(tex);
                e->alpha = 0xFF;
                e->alphaText = 0xFF;
            }
            if (fadeIn == 0) {
                e->state = 0;
            } else {
                e->state = 1;
                e->len = fadeIn;
                e->time = 0;
                r->SetAlpha(0);
                r->SetAlphaText(0);
            }
            r->flags40 &= ~8u;
        }
    }
    if (sheet->GetStr(event, (char *)"ShowText", str, 0x40))
        AddText(str, gsStrings->GetStringById(textId), textFlags, &col, fadeIn);
    if (sheet->GetStr(event, (char *)"Sound", str, 0x40)) {
        sncSOUND_DESCR *d = new sncSOUND_DESCR;
        if (d) {
            d->name.SetName(str);
            d->flags = 0;
            d->volume = 0;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
        }
        snd = (sncSOUND_DESCR **)apRealloc(snd, (nSnd + 1) * sizeof(*snd));
        snd[nSnd++] = d;
        if (!d || !sncLinkSndListData(1, (sncSOUND_DESCR_3D *)d)) {
            delete d;
            nSnd--;
        } else {
            if (sndHandle[sndId] != -1)
                ui->StopSnd(sndHandle[sndId]);
            sndHandle[sndId] = ui->PlaySnd(snd[nSnd - 1]);
        }
        fast = 0;
    }
    float dl;
    if (sheet->GetFloat(event, (char *)"Delay", &dl))
        delay = dl * 1.0f > delay ? dl * 1.0f : delay;
    int ws;
    if (sheet->GetInt(event, (char *)"WaitSound", &ws))
        waitSnd |= 1u << ws;
    if (sheet->GetStr(event, (char *)"Music", str, 0x40)) {
        if (!strcmp(str, "intro"))
            ui->PlayAmbientMusic(1);
        else if (!strcmp(str, "outro"))
            ui->PlayAmbientMusic(2);
    }
}

// v12 0x00492390  (feb 0x00487140)
// After EVENT_n: EVENT_n_1 if the sheet has it; else the next number at the same depth; else one level up, and so on.
psSECT *wrUI_COMICS::GetNextSection()
{
    if (!sheet)
        return 0;
    char *s = event;
    if (strlen(s) <= 0x37) {
        strcat(s, "_1");
        if (psSECT *x = sheet->FindSect(s))
            return x;
        s[strlen(s) - 2] = 0;
    }
    for (;;) {
        char *u = strrchr(s, '_');
        if (!u)
            return 0;
        itoa(atoi(u + 1) + 1, u + 1, 10);
        if (psSECT *x = sheet->FindSect(s))
            return x;
        *u = 0;
    }
}

// ------------------------------------------------------------------------------------------------ the page

uiELEMENT *__fastcall wrUI_COMICS_FRAME::Create()
{
    wrUI_COMICS_FRAME *f = new wrUI_COMICS_FRAME;
    if (!f)
        return 0;
    f->_6c = 0;
    f->back = 0;
    f->notify = 0;
    f->comics = 0;
    f->param = 0;
    return f;
}

// v12 0x004031F0 (wrUI_FRAME's, a shared "return 1"): the page's untyped children are windows
int wrUI_COMICS_FRAME::GetElemDefType()
{
    return 1;
}

// v12 0x00492460  (feb 0x00487210)
int wrUI_COMICS_FRAME::ProcessInit(void *p)
{
    comics = (wrUI_COMICS *)FindChildElement((char *)"comics");
    if (!uiELEMENT::ProcessInit(p))                     // wrUI_FRAME::ProcessInit (v12 0x004942B0): and "back"
        return 0;
    if (uiELEMENT *b = FindChildElement((char *)"back"))
        back = b;
    return 1;
}

// v12 0x00492480  (feb 0x00487230)
// The comic file into a sheet, the player given it from "EVENT" ([Debug] ZeroEvent to start elsewhere), the page shown.
int __fastcall wrUI_COMICS_FRAME::Start(uiELEMENT *notify, int param, const char *file, int how)
{
    fioFILE_DISK f;
    wrUI_COMICS_FRAME *page = (wrUI_COMICS_FRAME *)((uiUI_SYSTEM *)uiSystem)->root->FindChildElement((char *)"comics_frame");
    if (!f.Init((char *)file, 1))
        return 0;
    int n = (int)f.size;
    char *text = (char *)apMalloc(n + 1);
    f.ReadData(text, n, 1);
    f.Term();
    text[n] = 0;
    psSHEET *sh = (psSHEET *)operator new(sizeof(psSHEET));
    if (sh)
        memset(sh, 0, sizeof(psSHEET));
    sh->SetString(text);
    if (sh->ParseString()) {
        apFree(text);
        return 0;
    }
    if (!page)
        return 0;
    wrUI_COMICS *c = page->comics;
    page->notify = notify;
    page->param = param;
    c->Stop(1);
    c->sheet = sh;
    c->ownSheet = 1;
    c->delay = 0;
    if (!sh->GetStr((char *)"Debug", (char *)"ZeroEvent", c->event, 0x40))
        strcpy(c->event, "EVENT");
    ((wrUI_SYSTEM *)uiSystem)->ShowFrame(page->name, how);
    apFree(text);
    return 1;
}

// v12 0x00492660  (feb 0x00487410)
// The sounds stop; shown without the mouse cursor, hidden with it.
int wrUI_COMICS_FRAME::StartAppearing(int how)
{
    ((wrUI_SYSTEM *)uiSystem)->StopAllSnd();
    // PLACEHOLDER (uiCURSOR::Show v12 0x00519910 not rebuilt): the mouse cursor hidden while it plays, back after
    return uiELEMENT::StartAppearing(how);
}

// v12 0x004926C0  (feb 0x00487470)
// The comic over: 'notify' is told ('COMI' with param), or the main menu comes.
int wrUI_COMICS_FRAME::ProcessUpdate(void *p)
{
    if (!uiELEMENT::ProcessUpdate(p))                   // (uiFRAME::ProcessUpdate v12 0x00519310)
        return 0;
    if (comics->sheet)
        return 1;
    if (notify) {
        uiCHILD_NOTIFY n;
        n.child = this;
        n.type = 0x434F4D49;                            // 'COMI'
        n.param = param;
        notify->OnChildNotify(&n);
        notify = 0;
        return 1;
    }
    ((wrUI_SYSTEM *)uiSystem)->ShowFrame("main", 8);
    return 1;
}

// v12 0x00492750  (feb 0x00487500)
// Esc (key event 6, key 7): the comic ends.
int wrUI_COMICS_FRAME::OnKeyboard(uiINPUT_INFO *in)
{
    if (in->type == 6 && in->key == 7)
        if (wrUI_COMICS *c = (wrUI_COMICS *)FindChildElement((char *)"comics"))
            c->Stop(1);
    return uiELEMENT::OnKeyboard(in);                   // (wrUI_FRAME::OnKeyboard v12 0x004942E0: Esc = "back")
}
