// wr_ui.cpp -- Will Rock's UI system (wrUI_SYSTEM) and its widgets (Will Rock 1.2, v12.exe; feb obj wr_ui.obj)
//
// Rebuilt from the 1.2 code (v12 addresses first), checked against the Feb 2003 decompile.
// So far: the constructor and the appear / disappear fade every uiAREA uses (StartAppear / ProcessAppear /
// FinishAppear: a 0.2 s ramp of the text and rectangle alpha). Not rebuilt yet: Init / Activate / Update and the
// frame handling, the message boxes, loading screen, sounds, quick save / load, and the wrUI_* widgets.

#include "game.h"

extern float gsElapsedTime;                 // v12 0x0098E9F4 (feb 0x009482DC)

static const float UI_APPEAR_TIME = 0.2f;   // __real@3e4ccccd (v12 0x005FA15C)

// The 12 bytes StartAppear keeps at uiRENDER_INFO +0xB8 while a fade runs
struct wrUI_APPEAR {
    float timeLeft;                         // +0 from 0.2 s down
    float alphaText;                        // +4 the text alpha to come back to (colorText >> 24)
    float alpha;                            // +8 the rectangle alpha (rect.color >> 24)
};

// v12 0x0048B6F0  (feb 0x00480A30)
wrUI_SYSTEM::wrUI_SYSTEM()
{
    hudMessages = 0;
    hudMpWnd = 0;
    hudThWnd = 0;
    hudCoopWnd = 0;
    hudWnd = 0;
    msgBox = 0;
    woofer = 0;
    savePicture = 0;
    _15C = 0;
    loadErrStrId = 0;
    _158 = -1;
}

// v12 0x0048C210  (feb 0x004814C0)
// Keep the alphas the element should end at; an appear (anything but 0x100, 1.2 passes 0x140) starts from 0.
int wrUI_SYSTEM::StartAppear(uiRENDER_INFO *r, int how)
{
    wrUI_APPEAR *a = new wrUI_APPEAR;
    if (!a)
        return 0;
    a->timeLeft = 0.0f;
    a->alphaText = 0.0f;
    a->alpha = 0.0f;
    a->timeLeft = UI_APPEAR_TIME;
    a->alphaText = (float)(int)((unsigned char *)&r->colorText)[3];
    a->alpha = (float)(int)((unsigned char *)&r->rect.color)[3];
    r->appear = a;
    if (how != 0x100) {
        r->SetAlphaText(0.0f);
        r->SetAlpha(0.0f);
    }
    return 1;
}

// v12 0x0048C2A0  (feb 0x00481550)
// One frame of the ramp; 0 (and the alphas back) when it is over.
int wrUI_SYSTEM::ProcessAppear(uiRENDER_INFO *r, int how)
{
    wrUI_APPEAR *a = (wrUI_APPEAR *)r->appear;
    if (m3dUpdateTimeField(&a->timeLeft, gsElapsedTime)) {
        r->SetAlphaText(a->alphaText);
        r->SetAlpha(a->alpha);
        delete a;
        r->appear = 0;
        return 0;
    }
    if (!(a->timeLeft >= 0.0f))
        a->timeLeft = 0.0f;
    else if (a->timeLeft > UI_APPEAR_TIME)
        a->timeLeft = UI_APPEAR_TIME;
    float done = UI_APPEAR_TIME - a->timeLeft;
    float t = done * a->alphaText * 5.0f;           // (5 = 1 / 0.2)
    float c = done * a->alpha * 5.0f;
    if (how == 0x100) {
        r->SetAlphaText(a->alphaText - t);
        r->SetAlpha(a->alpha - c);
        return 1;
    }
    r->SetAlphaText(t);
    r->SetAlpha(c);
    return 1;
}

// v12 0x0048C3A0  (feb 0x00481650)
// Cut the fade short: the alphas back at once.
void wrUI_SYSTEM::FinishAppear(uiRENDER_INFO *r, int how)
{
    wrUI_APPEAR *a = (wrUI_APPEAR *)r->appear;
    r->SetAlphaText(a->alphaText);
    r->SetAlpha(a->alpha);
    delete a;
    r->appear = 0;
}

// ---------------------------------------------------------------------------------------------------- menu sound

// v12 0x0048CA00  (feb 0x00481B90)
// Play menu music track id (0 "i_wanna_rock", 1 "intro", 2 "outro"); -1 stops the current one with a fade-out,
// -2 at once.
void wrUI_SYSTEM::PlayAmbientMusic(int id)
{
    wrUI_WOOFER *w = woofer;
    if (!w)
        return;
    if (id != -1 && id != -2) {
        if (sndSystem)
            sndSystem->SetMusic(w->music[id], 0);
        w->curMusic = id;
        return;
    }
    if (sndSystem && w->curMusic != -1)
        sndSystem->StopMusic(w->music[w->curMusic], id != -2);
    w->curMusic = -1;
}

// The menu's sounds, played on the woofer's four channels: 0 menu_flame_loop (looping), 1 menu_scroll, 2 menu_select,
// 3 menu_back, 4 menu_accept, 5 mp_notify_timer, 6 mp_notify_beep (waits for the one before). Linked to their files at
// the end of the UI registration (sncLinkSndListData(7, ...) at v12 0x0048BC59).
sncSOUND_DESCR wrUISnd[7];                  // v12 0x0064F608

// v12 0x0048B0C0  static initialiser (the sncSOUND_DESCR constructor inline, v12 0x004038F0)
static struct wrUI_SND_INIT {
    wrUI_SND_INIT()
    {
        static const char *names[7] = { "menu_flame_loop", "menu_scroll", "menu_select", "menu_back", "menu_accept",
                                        "mp_notify_timer", "mp_notify_beep" };
        for (int i = 0; i < 7; i++) {
            sncSOUND_DESCR *d = &wrUISnd[i];
            d->name.SetName((char *)names[i]);
            d->flags = i == 0 ? SNCD_INF_LOOP : i == 6 ? SNCD_QUEUE_NEXT : 0;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->vtbl = 0;                    // (sncSOUND_DESCR vtable v12 0x005F94AC)
        }
    }
} wrUISndInit;

// v12 0x0048CA80  (feb 0x00481C00)
// Whether the sound PlaySnd gave handle h to still plays (a channel that ended forgets it).
int wrUI_SYSTEM::IsSndPlaying(int h)
{
    wrUI_WOOFER *w = woofer;
    if (!w)
        return 0;
    for (int i = 0; i < 4; i++)
        if (w->chnSnd[i] == h) {
            if (!w->chn[i]->IsPlay(-1)) {
                w->chnSnd[i] = -1;
                return 0;
            }
            return 1;
        }
    return 0;
}

// v12 0x0048CAE0  (feb 0x00481C60)
// UI sound id on the first idle channel; returns its handle, -1 when none is idle (or the UI is locked / no woofer).
int wrUI_SYSTEM::PlaySnd(int id)
{
    if (_128)
        return -1;
    wrUI_WOOFER *w = woofer;
    if (!w)
        return -1;
    int i;
    for (i = 0; i < 4; i++)
        if (!w->chn[i]->IsPlay(-1))
            break;
    if (i >= 4)
        return -1;
    w->chnSnd[i] = w->_18B;
    w->chn[i]->Play(&wrUISnd[id], 0);
    return w->_18B++;
}

// v12 0x0048CB70  (feb 0x00481CF0)
// The same with any sound.
int wrUI_SYSTEM::PlaySnd(sncSOUND_DESCR *d)
{
    if (_128)
        return -1;
    wrUI_WOOFER *w = woofer;
    if (!w || !d)
        return -1;
    int i;
    for (i = 0; i < 4; i++)
        if (!w->chn[i]->IsPlay(-1))
            break;
    if (i >= 4)
        return -1;
    w->chnSnd[i] = w->_18B;
    w->chn[i]->Play(d, 0);
    return w->_18B++;
}

// v12 0x0048CC00  (feb 0x00481D80)
void wrUI_SYSTEM::StopSnd(int h)
{
    wrUI_WOOFER *w = woofer;
    if (!w)
        return;
    for (int i = 0; i < 4; i++)
        if (w->chnSnd[i] == h) {
            w->chnSnd[i] = -1;
            w->chn[i]->Stop();
        }
}

// v12 0x0048CC40  (feb 0x00481DC0)
void wrUI_SYSTEM::StopAllSnd()
{
    wrUI_WOOFER *w = woofer;
    if (!w)
        return;
    for (int i = 0; i < 4; i++) {
        w->chnSnd[i] = -1;
        w->chn[i]->Stop();
    }
}

// v12 0x0048F680
entENTITY *__fastcall wrUI_WOOFER::Create(animINST *)
{
    wrUI_WOOFER *w = new wrUI_WOOFER;       // operator new(0x19F), entENTITY ctor
    if (!w)
        return 0;
    for (int i = 0; i < 4; i++) {
        w->chn[i] = 0;
        w->chnSnd[i] = -1;
    }
    w->_18B = 0;
    w->music[0] = w->music[1] = w->music[2] = 0;
    return w;
}

// v12 0x0048D120
// INIT: the channels and tracks; TERM: the music stopped (fading), the tracks freed, the menu told.
BOOL32 wrUI_WOOFER::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == MSG_MSG_INIT)
        ProcessINIT();
    else if (id == MSG_MSG_TERM) {
        if (sndSystem && curMusic != -1)
            sndSystem->StopMusic(music[curMusic], 1);
        curMusic = -1;
        for (int i = 0; i < 3; i++) {
            operator delete(music[i]);
            music[i] = 0;
        }
        curMusic = -1;
        ((wrUI_SYSTEM *)uiSystem)->woofer = 0;
    }
    return 0;
}

static sndMUSIC *wrMakeTrack(const char *name, u32 f)
{
    sndMUSIC *m = (sndMUSIC *)operator new(0x50);
    if (m) {
        m->_00 = 0;
        m->name[0] = 0;
        m->volumeDb = 0.0f;
        m->pos[0] = m->pos[1] = 0;
    }
    ((apNAME *)m->name)->SetName((char *)name);
    m->_00 |= f;
    m->volumeDb = 0.0f;
    m->SetMuteGroup(0, 1);                  // the menu music plays through mute groups 1 and 5
    m->SetMuteGroup(0, 5);
    return m;
}

// v12 0x0048D1B0  (feb 0x004821A0)
// Four plain channels (also not muted by groups 1 and 5) and the three tracks; the menu gets this woofer.
void wrUI_WOOFER::ProcessINIT()
{
    for (int i = 0; i < 4; i++) {
        sncCHANNEL *c = (sncCHANNEL *)operator new(0x74);
        if (c)
            new (c) sncCHANNEL;
        chn[i] = c;
        c->SetMuteGroup(0, 1);
        chn[i]->SetMuteGroup(0, 5);
        gsSysSound->AddChannel(chn[i], this, 0);
    }
    music[0] = wrMakeTrack("i_wanna_rock", 1);
    music[1] = wrMakeTrack("intro", 2);
    music[2] = wrMakeTrack("outro", 2);
    curMusic = -1;
    ((wrUI_SYSTEM *)uiSystem)->woofer = this;
}

// v12 0x0048CDC0  (feb 0x00481F40)
// The HUD's full-screen flash: 0.3 s in color, or off.
void wrUI_SYSTEM::ShowPlayerPain(int on, unsigned long color)
{
    if (on)
        hudUhWnd->ShowPain(0.3f, 0, color);
    else
        hudUhWnd->ShowPain(0, 1, 0);
}


// v12 0x0048F7F0 (new in 1.2)
int wrUI_SYSTEM::IsMsgShown()
{
    return hudMessages->IsMsgShown();
}

// v12 0x0048F800  (feb 0x004843D0)
void wrUI_SYSTEM::ShowMsgW(const wrWCHAR *s, float time, int prio)
{
    hudMessages->ShowMsgW(s, time, prio);
}

// v12 0x0048F820  (feb 0x004843F0)
void wrUI_SYSTEM::ClearMsg(int)
{
    hudMessages->ClearMsg();
}


// v12 0x0048D3E0  (feb 0x00482290, multiplayer maps only)
// The string ids of a level's names, from its file name. Single player (sp, "lev_<m>_..."): MISSION_NAME_<m> and
// LEVEL_NAME_<m>. Else the multiplayer map GAME_MP_MAP_<name>; failing that, the name with a "_<2..9>_<letter>"
// part changed to "_1" ("dm_3_x" -> "dm_1"); 1 / 1 and 0 when nothing matches. 1.2: the single-player part is new.
int __fastcall _uiGetLevelNameStrId(const char *level, int *strId, int *b, int sp)
{
    char name[0x40];
    const char *f = strrchr(level, '\\');
    f = f ? f + 1 : level;
    if (sp) {
        const char *u = strchr(f, '_');
        if (u) {
            int m = atoi(u + 1);
            int id = gsStrings->GetStringIdFmt("MISSION_NAME_%i", m);
            if (id) {
                *strId = id;
                *b = gsStrings->GetStringIdFmt("LEVEL_NAME_%i", m);
                return 1;
            }
        }
    }
    const char *dot = strchr(f, '.');
    if (dot) {
        int n = (int)(dot - f) + 1;
        strncpy(name, f, n < 0x41 ? n : 0x40);
        if (n > 0x40)
            n = 0x40;
        name[n - 1] = 0;
        f = name;
    }
    int id = gsStrings->GetStringIdFmt("GAME_MP_MAP_%s", f);
    if (id) {
        *strId = id;
        *b = 1;
        return 1;
    }
    if (f != name) {
        strncpy(name, f, 0x40);
        name[0x3F] = 0;
    }
    for (char *c = name + 1; c[-1]; c++) {
        if (c[-1] == '_' && c[0] > '1' && c[0] <= '9' && c[1] == '_' && c[2] > '9') {
            c[0] = '1';
            c[2] = 0;
            id = gsStrings->GetStringIdFmt("GAME_MP_MAP_%s", name);   // (v12 0x0051D080, the twin)
            if (id) {
                *strId = id;
                *b = 1;
                return 1;
            }
        }
    }
    *strId = 1;
    *b = 1;
    return 0;
}

// v12 0x004031F0 (a shared "return 1"; feb +0x5C 0x004036A0): the default text flags (1: centred across) -- what the
// comics give a text without TextFlags
int wrUI_SYSTEM::_slot60()
{
    return 1;
}
