// wr_ui_hud.cpp -- the in-game HUD (Will Rock 1.2, v12.exe; feb obj wr_ui_hud.obj)
//
// Rebuilt from the 1.2 code (v12 addresses first), checked against the Feb 2003 decompile.
// ui.tpl frame "hud" holds the "hud_wnd" window (type wr_hud_wnd = wrUIHUD_WINDOW). Its children are icon bars
// (wrh_iconbar = wrUIHUD_ICONBAR: a "Tablichka" plate with a "text" and an "icon" render element; the icon is a
// 4 x 4 sheet that turns once when its number changes) and the "underwater" air slider (wr_slider).
// Every frame wrUIHUD_WINDOW::ProcessUpdate reads plrPlayer and refreshes the bars that changed.
//
// What changed from the beta (feb) in 1.2, all followed here:
//   - player fields moved (moveFlags +0x1F3 for +0x29B, armor +0x203, money +0x207, weapons +0x1AF / +0x1B3,
//     powerups +0x223), IsDead is vtable slot +0x74 (feb +0x78), the weapon index is +0x98 (feb +0x94);
//   - the ammo number is "ammo beyond the clip" (wpnWEAPON ammo - clip) and the colt shows " - /<clip>";
//   - the armor sheets are "armor%i_4x4" and the class is ftol(armor * 0.04) clamped to 0..3;
//   - the money bar follows wrSysGO->mode (shown in single player and co-op only);
//   - ProcessUpdate keeps the shown weapon at +0x100 (feb: the init flag).
// Not rebuilt here yet: wruiHUDRegister and the other HUD windows (MP / COOP / TH / SP / UH), wrUIHUD_MSG,
// wrUIHUD_DBGMSG, wrUIHUD_ZOOM, wrUIHUD_SLIDER.

#include "game.h"
#include "../gs/gs_mp_server.h"   // gsSysMP->mode, gsSysNetwork (the HUD ping)
#include <wchar.h>
#include <stdio.h>

extern float gsElapsedTime;                 // v12 0x0098E9F4 (feb 0x009482DC)

static const float HUD_CELL_TIME = 0.0625f; // __real@3d800000: one sheet cell (16 cells = one turn in 1 s)

// The render elements of an icon bar (uiRENDER_INFO::SetElementIdx in Init): 0 = "text", 1 = "icon"
static inline uiRENDER_ELEM *hudText(uiELEMENT *bar) { return &((uiAREA *)bar)->renderInfo.elems[0]; }
static inline uiRENDER_ELEM *hudIcon(uiELEMENT *bar) { return &((uiAREA *)bar)->renderInfo.elems[1]; }
static inline wrUIHUD_ICONBAR *hudBar(uiELEMENT *bar) { return (wrUIHUD_ICONBAR *)bar; }

// The repeated "value changed" step: an empty text is skipped by uiRENDER_INFO::Render
static void hudSetText(uiELEMENT *bar, const wrWCHAR *s)
{
    uiRENDER_ELEM *t = hudText(bar);
    if (!s[0])
        t->flags40 |= UIRF_SKIP;
    else
        t->flags40 &= ~UIRF_SKIP;
    t->SetTextW(s, 0x7FFFFFFF);
}

// ... and one more turn of the icon (unless it turns forever, -1)
static void hudSpin(uiELEMENT *bar)
{
    wrUIHUD_ICONBAR *b = hudBar(bar);
    if (b->spins >= 0)
        b->spins = b->spins < 2 ? 1 : b->spins;
}

static inline void hudSetIcon(uiELEMENT *bar, txmTEXTURE *t)
{
    uiRENDER_ELEM *i = hudIcon(bar);
    if (t != i->rect.tex)
        i->rect.tex = t;
}

// The health / boss colour: from (255, 91, 50) when empty to (169, 181, 215) = 0xFFA9B5D7 when full,
// t = the fraction left (0..1). The same colour goes to the text and to the plate (v12 0x004976E5).
static unsigned hudLerpColor(float t)
{
    float s = 1.0f - t;
    int a = (int)(255.0f * s + 255.0f * t);
    int r = (int)(255.0f * s + 169.0f * t);
    int g = (int)(91.0f * s + 181.0f * t);
    int b = (int)(50.0f * s + 215.0f * t);
    return (((unsigned)(r & 0xFF) | (unsigned)a << 8) << 8 | (unsigned)(g & 0xFF)) << 8 | (unsigned)(b & 0xFF);
}

static inline float hudClamp01(float t)
{
    if (t < 0.0f)
        return 0.0f;
    if (t > 1.0f)
        return 1.0f;
    return t;
}

// ---------------------------------------------------------------------------------------------------- iconbar

// v12 0x004983C0  (feb 0x0048C820)
uiELEMENT *__fastcall wrUIHUD_ICONBAR::Create()
{
    wrUIHUD_ICONBAR *b = new wrUIHUD_ICONBAR;
    b->spins = 0;
    b->cell = 0;
    b->cellTimer = HUD_CELL_TIME;
    return b;
}

// v12 0x00494C70  (feb 0x00489840)
void wrUIHUD_ICONBAR::Init(objOBJ *o)
{
    uiAREA::Init(o);
    renderInfo.SetElementIdx("text", 0);
    renderInfo.SetElementIdx("icon", 1);
    renderInfo.elems[0].colorText = 0xFFA9B5D7;
    renderInfo.rect.color = 0xFFA9B5D7;
}

// v12 0x00494CC0  (feb 0x00489890)
// Appearing starts the icon at rest.
int wrUIHUD_ICONBAR::StartAppearing(int how)
{
    if (how) {
        cell = 0;
        UpdateTexture();
    }
    return uiAREA::StartAppearing(how);
}

// v12 0x00494CF0  (feb 0x004898C0)
// The icon's corners on sheet cell (cell % 4, cell / 4).
void wrUIHUD_ICONBAR::UpdateTexture()
{
    uiVERTEX *v = renderInfo.elems[1].rect.v;
    v[0].u = (float)(cell % 4) * 0.25f;
    v[0].v = (float)(cell / 4) * 0.25f;
    v[1].u = (float)(cell % 4 + 1) * 0.25f;
    v[1].v = (float)(cell / 4) * 0.25f;
    v[2].u = (float)(cell % 4 + 1) * 0.25f;
    v[2].v = (float)(cell / 4 + 1) * 0.25f;
    v[3].u = (float)(cell % 4) * 0.25f;
    v[3].v = (float)(cell / 4 + 1) * 0.25f;
}

// v12 0x00494E20  (feb 0x004899F0)
// Turning: a cell every 0.0625 s; each time cell 0 is left one requested turn is used up.
int wrUIHUD_ICONBAR::ProcessUpdate(void *p)
{
    if ((cell || spins) && m3dUpdateTimeField(&cellTimer, gsElapsedTime)) {
        if (!cell && spins > 0)
            spins--;
        cellTimer = HUD_CELL_TIME;
        cell = (cell + 1) % 16;
        UpdateTexture();
    }
    return uiELEMENT::ProcessUpdate(p);
}

// ---------------------------------------------------------------------------------------------------- window

// v12 0x004970C0  (feb 0x0048B590)
wrUIHUD_WINDOW::wrUIHUD_WINDOW()
{
    keyBar = healthBar = armorBar = moneyBar = ammoBar = bossBar = 0;
    bossShown = 1;
    for (int i = 0; i < 10; i++)
        texAmmo[i] = 0;
    texAtomic = 0;
    for (int i = 0; i < 4; i++)
        texArmor[i] = 0;
    texKey = texHealth = texGold = 0;
    lastWeapon = 0;
    underwater = 0;
    lastHealth = lastArmor = lastAmmo = lastMoney = _fc = -1;
    texPower[0] = texPower[1] = texPower[2] = 0;
}

// v12 0x0048DF30 (the shared "return 0" of the wr windows)
// Child objects without a UI_PROP type become areas (uiUI_SYSTEM::GetTypeName(0) = "area"). Without this override
// (uiELEMENT's -1) uiELEMENT::Init would make no child elements at all.
int wrUIHUD_WINDOW::GetElemDefType()
{
    return 0;
}

// v12 0x00498460  (feb 0x0048C890)
// v12 0x00494BB0  (feb 0x00489780)
// The bar's rectangle from its template's: the left edge goes from the right end (0) to the full width (1).
void wrUIHUD_SLIDER::SetPosition(float pos)
{
    if (pos < 0.0f)
        pos = 0.0f;
    else if (pos > 1.0f)
        pos = 1.0f;
    u8 *e = (u8 *)renderInfo.elems;
    float x0 = WR_FIELD(float, e, 0x5C), y0 = WR_FIELD(float, e, 0x60);
    float x1 = WR_FIELD(float, e, 0x7C), y1 = WR_FIELD(float, e, 0x80);
    float x = x1 + (x0 - x1) * pos;
    WR_FIELD(float, this, 0xB8) = x1;
    WR_FIELD(float, this, 0xBC) = y0;
    WR_FIELD(float, this, 0xC8) = x;
    WR_FIELD(float, this, 0xCC) = y0;
    WR_FIELD(float, this, 0xD8) = x;
    WR_FIELD(float, this, 0xDC) = y1;
    WR_FIELD(float, this, 0xE8) = x1;
    WR_FIELD(float, this, 0xEC) = y1;
}

// v12 0x00494C50  (feb 0x00489820)
int wrUIHUD_SLIDER::ProcessInit(void *p)
{
    SetPosition(1.0f);
    return uiAREA::ProcessInit(p);
}

// v12 0x00498180  (feb 0x0048C660)
uiELEMENT *__fastcall wrUIHUD_SLIDER::Create()
{
    return new wrUIHUD_SLIDER;
}

uiELEMENT *__fastcall wrUIHUD_WINDOW::Create()
{
    return new wrUIHUD_WINDOW;
}

// v12 0x00497170  (feb 0x0048B640)
// The icon sheets, the bars by name, and their resting icons. The key bar is never shown in 1.2.
int wrUIHUD_WINDOW::ProcessInit(void *p)
{
    char nm[0x100];
    texKey = txmManager->Add((char *)"key_4x4", 0, TXM_ADD_PLAIN);
    texHealth = txmManager->Add((char *)"helf_4x4", 0, TXM_ADD_PLAIN);
    texGold = txmManager->Add((char *)"gold_4x4", 0, TXM_ADD_PLAIN);
    texAmmo[PWP_SHOVEL] = txmManager->Add((char *)"lopata_item_4x4", 0, TXM_ADD_PLAIN);
    texAmmo[PWP_COLT] = txmManager->Add((char *)"pulia_4x4", 0, TXM_ADD_PLAIN);
    texAmmo[PWP_STG] = txmManager->Add((char *)"shotgun_ammo_4x4", 0, TXM_ADD_PLAIN);
    texAmmo[PWP_MACHINEGUN] = txmManager->Add((char *)"mashinegun_ammo_4x4", 0, TXM_ADD_PLAIN);
    texAmmo[PWP_MINIGUN] = txmManager->Add((char *)"minigun_ammo_4x4", 0, TXM_ADD_PLAIN);
    texAmmo[PWP_CROSSBOW] = txmManager->Add((char *)"crossbow_ammo_4x4", 0, TXM_ADD_PLAIN);
    texAmmo[PWP_ACIDGUN] = txmManager->Add((char *)"acid_ammo_4x4", 0, TXM_ADD_PLAIN);
    texAtomic = txmManager->Add((char *)"atomic_ammo_4x4", 0, TXM_ADD_PLAIN);   // = texAmmo[PWP_ATOMICGUN]
    texAmmo[PWP_BASOOKA] = txmManager->Add((char *)"bazooka_ammo_4x4", 0, TXM_ADD_PLAIN);
    texAmmo[PWP_MEDUSA] = txmManager->Add((char *)"meduzagun_item_4x4", 0, TXM_ADD_PLAIN);
    texAmmo[PWP_BOMB] = txmManager->Add((char *)"pwp_bmb_ammo_4x4", 0, TXM_ADD_PLAIN);
    for (int i = 0; i < 4; i++) {
        sprintf(nm, "armor%i_4x4", i + 1);
        texArmor[i] = txmManager->Add(nm, 0, TXM_ADD_PLAIN);
    }
    texPower[0] = txmManager->Add((char *)"power_qdam_4x4", 0, TXM_ADD_PLAIN);
    texPower[1] = txmManager->Add((char *)"power_shield_4x4", 0, TXM_ADD_PLAIN);
    texPower[2] = txmManager->Add((char *)"power_chasi_4x4", 0, TXM_ADD_PLAIN);
    for (int i = 0; i < 3; i++) {                       // the powerup bars turn forever while shown
        sprintf(nm, "powerup%02i", i + 1);
        powerup[i] = FindChildElement(nm);
        hudBar(powerup[i])->spins = -1;
        hudSetIcon(powerup[i], texPower[i]);
    }
    keyBar = FindChildElement("key_bar");
    bossBar = FindChildElement("boss_bar");
    healthBar = FindChildElement("health_bar");
    armorBar = FindChildElement("armor_bar");
    moneyBar = FindChildElement("money_bar");
    ammoBar = FindChildElement("ammo_bar");
    underwater = FindChildElement("underwater");
    hudBar(keyBar)->spins = 0;
    hudBar(bossBar)->spins = 0;
    hudBar(healthBar)->spins = 0;
    hudBar(armorBar)->spins = 0;
    hudBar(moneyBar)->spins = 0;
    hudBar(ammoBar)->spins = 0;
    hudSetIcon(healthBar, texHealth);
    hudSetIcon(armorBar, texArmor[0]);
    hudSetIcon(moneyBar, texGold);
    hudSetIcon(ammoBar, texAmmo[PWP_SHOVEL]);
    hudSetIcon(bossBar, texHealth);
    hudText(keyBar)->flags40 |= UIRF_SKIP;
    hudText(keyBar)->SetTextId(1);
    hudSetIcon(keyBar, texKey);
    bossShown = 1;
    return uiELEMENT::ProcessInit(p);
}

// v12 0x00497570  (feb 0x0048BA40)
// HOOK: every HUD number. Each bar is refreshed only when its value changed (the window keeps the last ones).
int wrUIHUD_WINDOW::ProcessUpdate(void *p)
{
    wrWCHAR s[32];
    plrPLAYER *pl = plrPlayer;
    if (!pl || (flags & UIEF_HIDDEN))
        return uiELEMENT::ProcessUpdate(p);
    if (keyBar)
        keyBar->flags |= UIEF_HIDDEN;                   // single player 1.2: no key bar

    // the boss (aiPLANNER's boss NPC): percent of its health, the colour by it, its portrait
    bossShown |= (~bossBar->flags >> 13) & 1;
    if (bossBar) {
        float cur, max;
        int type;
        int have = aiAttackPlanner->GetBossHealth(&cur, &max, &type);        if (have != bossShown) {
            bossShown = have;
            if (have) {
                hudBar(bossBar)->spins = -1;
                bossBar->flags &= ~UIEF_HIDDEN;
            } else
                bossBar->flags |= UIEF_HIDDEN;
        }
        if (bossShown) {
            int pc = (int)(100.0f / max * cur + 0.9);
            if (pc < 1)
                pc = 1;
            _snwprintf((wchar_t *)s, 32, L"%i", pc);
            hudSetText(bossBar, s);
            unsigned c = hudLerpColor(hudClamp01(cur / max));
            hudText(bossBar)->colorText = c;
            ((uiAREA *)bossBar)->renderInfo.rect.color = c;
            txmTEXTURE *t;
            switch (type) {
            case 0x11: t = txmManager->Add((char *)"boss_cyclop_4x4", 0, TXM_ADD_PLAIN); break;
            case 0x12: t = txmManager->Add((char *)"boss_gefest_4x4", 0, TXM_ADD_PLAIN); break;
            case 0x13: t = txmManager->Add((char *)"boss_meduza_4x4", 0, TXM_ADD_PLAIN); break;
            case 0x14: t = txmManager->Add((char *)"boss_zeus_4x4", 0, TXM_ADD_PLAIN); break;
            default:   t = texHealth; break;
            }
            hudSetIcon(bossBar, t);
        }
    }

    // health: never below 0, at least 1 while alive; god mode shows UI_HUD_GOD
    if (healthBar) {
        int h;
        if (pl->moveFlags & PLR_MF_GOD)
            h = 9999;
        else {
            h = (int)pl->health;
            if (h < 0)
                h = 0;
            if (!wrEVCall<BOOL32>(pl, 0x74) && !h)        // IsDead
                h = 1;
        }
        if (h != lastHealth) {
            lastHealth = h;
            float t;
            if (plrPlayer->moveFlags & PLR_MF_GOD) {
                int id = gsStrings->GetStringId((char *)"UI_HUD_GOD");
                uiRENDER_ELEM *tx = hudText(healthBar);
                if (id == 0 || id == 1)
                    tx->flags40 |= UIRF_SKIP;
                else
                    tx->flags40 &= ~UIRF_SKIP;
                tx->SetTextId(id);
                t = 1.0f;
            } else {
                _snwprintf((wchar_t *)s, 32, L"%i", h);
                hudSetText(healthBar, s);
                t = hudClamp01(plrPlayer->health * 0.01f);
            }
            unsigned c = hudLerpColor(t);
            hudText(healthBar)->colorText = c;
            ((uiAREA *)healthBar)->renderInfo.rect.color = c;
            hudSpin(healthBar);
        }
    }

    // armor: the number, and the sheet by armour class ftol(armor / 25) (0..3)
    if (armorBar) {
        int a = (int)plrPlayer->armor;
        if (a != lastArmor) {
            lastArmor = a;
            _snwprintf((wchar_t *)s, 32, L"%i", a);
            int k = (int)(plrPlayer->armor * 0.04f);
            if (k < 0)
                k = 0;
            else if (k >= 4)
                k = 3;
            hudSetText(armorBar, s);
            hudSetIcon(armorBar, texArmor[k]);
            hudSpin(armorBar);
        }
    }

    // money: single player and co-op only
    if (moneyBar) {
        if (wrSysGO->mode == 0 || wrSysGO->mode == 2) {
            int m = (int)plrPlayer->money;
            if (m != lastMoney) {
                lastMoney = m;
                _snwprintf((wchar_t *)s, 32, L"%i", m);
                hudSetText(moneyBar, s);
                hudSpin(moneyBar);
            }
            moneyBar->flags &= ~UIEF_HIDDEN;
        } else
            moneyBar->flags |= UIEF_HIDDEN;
    }

    // ammo of the current weapon, by weapon: shovel "-", colt " - /<clip>", machinegun and bazooka the ammo
    // (turns only when it goes up), the others "<beyond the clip>/<clip>"
    if (ammoBar) {
        pwpWEAPON *w = plrPlayer->weapons[plrPlayer->curWeapon];
        int clip = w->clip;
        int n = (int)(w->ammo - (float)clip);
        int spin = n != lastAmmo || w != lastWeapon;
        switch (w->wpnIndex) {
        case PWP_SHOVEL:
            wcscpy((wchar_t *)s, L"-");
            if (w == lastWeapon)
                spin = 0;
            break;
        case PWP_COLT:
            _snwprintf((wchar_t *)s, 32, L" - /%3i", clip);
            if (w == lastWeapon)
                spin = 0;
            break;
        case PWP_MACHINEGUN:
        case PWP_BASOOKA:
            _snwprintf((wchar_t *)s, 32, L"%3i", (int)w->ammo);
            if (n <= lastAmmo && w == lastWeapon)
                spin = 0;
            break;
        default:
            _snwprintf((wchar_t *)s, 32, L"%3i/%3i", n, clip);
            break;
        }
        lastWeapon = w;
        lastAmmo = n;
        if (spin)
            hudSpin(ammoBar);
        txmTEXTURE *t = (&texAmmo[0])[w->wpnIndex];     // index 10 reads texAtomic, right after the array
        if (!t)
            t = texAmmo[PWP_SHOVEL];
        hudSetIcon(ammoBar, t);
        hudSetText(ammoBar, s);
    }

    // powerups: held = icon only, active = icon turning + seconds left (red under 6, yellow under 16)
    for (int i = 0; i < 3; i++) {
        uiELEMENT *b = powerup[i];
        plrPOWERUP *pw = &plrPlayer->powerup[i];
        if (pw->state == PLR_PWR_NONE) {
            hudBar(b)->spins = 0;
            hudIcon(b)->flags40 |= UIRF_SKIP;
        } else {
            hudIcon(b)->flags40 &= ~UIRF_SKIP;
            if (pw->state == PLR_PWR_ACTIVE) {
                hudBar(b)->spins = -1;
                int sec = (int)(pw->timeLeft + 0.5f);
                if (sec <= 5)
                    hudText(b)->colorText = 0xFFDC0000;
                else if (sec <= 15)
                    hudText(b)->colorText = 0xFFF0DC00;
                else
                    hudText(b)->colorText = 0xFFA9B5D7;
                _snwprintf((wchar_t *)s, 32, L"%2.1f", (double)pw->timeLeft);
                hudSetText(b, s);
                continue;
            }
            hudBar(b)->spins = 0;
        }
        hudText(b)->flags40 |= UIRF_SKIP;
        hudText(b)->SetTextId(1);
    }

    // the air left under water (hidden from 0.99 up, and when dead)
    if (underwater && !wrEVCall<BOOL32>(plrPlayer, 0x74)) {
        float air = plrPlayer->GetUnderwaterTimeLeft();
        if (air < 0.99f) {
            ((wrUIHUD_SLIDER *)underwater)->SetPosition(air);
            ((uiAREA *)underwater)->renderInfo.rect.color = 0xC832C8FF;
            underwater->flags &= ~UIEF_HIDDEN;
        } else
            underwater->flags |= UIEF_HIDDEN;
    }
    return uiELEMENT::ProcessUpdate(p);
}

// ---------------------------------------------------------------------------------------------------- pain / chat / ping

// v12 0x00498480
uiELEMENT *__fastcall wrUIHUD_UH_WINDOW::Create()
{
    wrUIHUD_UH_WINDOW *w = new wrUIHUD_UH_WINDOW;
    w->painTime = 0;
    w->painLen = 1.0f;
    w->chatTeam = 0;
    w->chatInput = 0;
    return w;
}

// v12 0x0048DF30 (the shared "return 0" of the wr windows)
int wrUIHUD_UH_WINDOW::GetElemDefType()
{
    return 0;
}

// v12 0x00496BA0  (feb 0x0048B4A0)
// A flash of time seconds in color, unless a longer one is still running; clear: none (the time 0).
void wrUIHUD_UH_WINDOW::ShowPain(float time, int clear, unsigned long color)
{
    if (!(time <= painTime) || clear) {
        painTime = time;
        painLen = time;
        if (bloodPic)
            ((uiAREA *)bloodPic)->renderInfo.SetColor(color);
    }
}

// v12 0x00496BE0
// The flash picture, the chat line (hidden) and the ping text; on a dedicated server the flash is black.
int wrUIHUD_UH_WINDOW::ProcessInit(void *p)
{
    if (!uiELEMENT::ProcessInit(p))
        return 0;
    bloodPic = FindChildElement("blood_pic");
    chatInput = FindChildElement("hud_chat_input");
    ping = FindChildElement("ping");
    if (chatInput)
        chatInput->flags |= UIEF_HIDDEN;
    if (gsAppState & 0x8000000) {
        uiRENDER_INFO *ri = &((uiAREA *)bloodPic)->renderInfo;
        for (int i = 0; i < ri->nElems; i++)
            ri->elems[i].SetTex(txmManager->Add((char *)"black", 0, -1));
    }
    return 1;
}

// v12 0x00496CA0
// The chat line reports Enter (sent, to the team or all) or Esc: back to the game's input, the line hidden.
void wrUIHUD_UH_WINDOW::OnChildNotify(uiCHILD_NOTIFY *n)
{
    if (n->child == chatInput && n->type == 0x494E50) {                // 'PNI'
        int go = 0;
        if (n->code == 0x454E5452) {                                   // 'ENTR'
            const wrWCHAR *t = wrEVCall<const wrWCHAR *>(chatInput, 0x64);  // the edit line's text
            ((wrUI_SYSTEM *)uiSystem)->SendChat(t, chatTeam == 1);
            go = 1;
        } else if (n->code == 0x455343) {                              // 'ESC'
            go = 1;
        }
        if (go) {
            gsSysInput->SetCurCtx(wrEVCall<gsiCONTEXT *>(gsSysInput, 0x1C, "GAME"));
            wrEVCall<void>(gsSysInput, 0x18);                          // ProcessInput twice: the keys flushed
            wrEVCall<void>(gsSysInput, 0x18);
            uiSystem->SetMouseFocus(0);
            uiSystem->SetKbdFocus(0);
            chatInput->flags |= UIEF_HIDDEN;
        }
    }
    uiELEMENT::OnChildNotify(n);
}

// v12 0x00496D60
// The ping text is shown in a client game (MP mode 3) with [GameOpt] DisplayPing.
int wrUIHUD_UH_WINDOW::StartAppearing(int how)
{
    if (how) {
        int on;
        if (gsSysMP->mode == 3 && _apCfgReadBool(0, (char *)"GameOpt", (char *)"DisplayPing", &on) && on) {
            ping->flags &= ~UIEF_HIDDEN;
            return uiELEMENT::StartAppearing(how);
        }
        ping->flags |= UIEF_HIDDEN;
    }
    return uiELEMENT::StartAppearing(how);
}

// v12 0x00496DE0
// The flash fades with its time left (always full on a dedicated server); in MP the chat keys open the chat
// line (CHAT_ALL, CHAT_TEAM in team games: wrGO mode 3); the line closes when it loses the focus; the ping.
int wrUIHUD_UH_WINDOW::ProcessUpdate(void *p)
{
    m3dUpdateTimeField(&painTime, gsElapsedTime);
    if (gsAppState & 0x8000000) {
        bloodPic->flags &= ~UIEF_HIDDEN;
        ((uiAREA *)bloodPic)->renderInfo.SetAlpha(255.0f);
    } else if (painTime != 0.0f) {
        ((uiAREA *)bloodPic)->renderInfo.SetAlpha(painTime / painLen * 255.0f);
        bloodPic->flags &= ~UIEF_HIDDEN;
    } else {
        bloodPic->flags |= UIEF_HIDDEN;
    }
    if (chatInput && (chatInput->flags & UIEF_HIDDEN)) {
        int open = 0;
        if (gsSysMP->mode != 4 && gsSysMP->mode != 1 && gsSysInput->IsOn(CMD_CHAT_MSG_ALL) && gsSysInput->IsToggled(CMD_CHAT_MSG_ALL)) {
            open = 1;
            chatTeam = 0;
        }
        if (gsSysMP->mode != 4 && *(int *)((char *)wrSysGO + 0x98) == 3 && gsSysInput->IsOn(CMD_CHAT_MSG_TEAM) &&
            gsSysInput->IsToggled(CMD_CHAT_MSG_TEAM)) {
            chatTeam = 1;
            open = 1;
        }
        if (open) {
            chatInput->flags &= ~UIEF_HIDDEN;
            gsSysInput->SetCurCtx(wrEVCall<gsiCONTEXT *>(gsSysInput, 0x1C, "CHAT"));
            gsSysInput->SetMouseMode(0);
            wrEVCall<void>(gsSysInput, 0x18);
            wrEVCall<void>(gsSysInput, 0x18);
            uiSystem->SetMouseFocus(chatInput);
            uiSystem->SetKbdFocus(chatInput);
            wrEVCall<void>(chatInput, 0x60, (const wrWCHAR *)L"");  // the edit line emptied
        }
    }
    if (chatInput && !(chatInput->flags & UIEF_HIDDEN) &&
        (uiSystem->mouseFocus != chatInput || uiSystem->kbdFocus != chatInput || (gsAppState & 0x80000))) {
        gsSysInput->SetCurCtx(wrEVCall<gsiCONTEXT *>(gsSysInput, 0x1C, "GAME"));
        wrEVCall<void>(gsSysInput, 0x18);
        wrEVCall<void>(gsSysInput, 0x18);
        uiSystem->SetMouseFocus(0);
        uiSystem->SetKbdFocus(0);
        chatInput->flags |= UIEF_HIDDEN;
    }
    if (!(ping->flags & UIEF_HIDDEN))
        ((uiAREA *)ping)->SetTextW(gsStrings->Printf("UI_HUD_PING", gsSysNetwork->Client_GetPing()), 0x7FFFFFFF);
    return uiELEMENT::ProcessUpdate(p);
}


// ---------------------------------------------------------------------------------------------------- messages
// wrUIHUD_MSG ("wrh_msg", the HUD's "hud_messages"): the centred message lines (ShowMsgW: shown for its time, then a
// 1.5 s fade) and the console lines ("con_msg*" elements, 10 s each then a 1.5 s fade, scrolled up when full).
// 1.2 changes, from the disassembly: a flags field (+0x130) and 12-byte console records (with the flags); flag 4
// fades four times faster, flag 8 writes the console only; console lines are kept unless gsAppState 0x8000000 is
// clear in the game mode 0 (beta: modes 0 and 2); console lines last 10 s (beta 7); IsMsgShown.

#define HUD_MSG_FADE  1.5f     // v12 0x3FC00000
#define HUD_MSG_ALPHA 170.0f   // v12 0x006056EC: the text alpha per second of fade left

static inline void hudHide(uiRENDER_ELEM *e) { e->flags40 |= UIRF_SKIP; }
static inline void hudShow(uiRENDER_ELEM *e) { e->flags40 &= ~UIRF_SKIP; }

// v12 0x004981E0  (feb 0x0048C7E0)
uiELEMENT *__fastcall wrUIHUD_MSG::Create()
{
    wrUIHUD_MSG *m = new wrUIHUD_MSG;
    m->renderInfo.elems = 0;
    m->renderInfo.capElems = 0;
    m->renderInfo.nElems = 0;
    m->renderInfo.appear = 0;
    m->con = 0;
    m->conCap = 0;
    m->nConUsed = 0;
    return m;
}

// v12 0x00498250  (feb 0x0048C730)
wrUIHUD_MSG::~wrUIHUD_MSG()
{
    apFree(con);
}

// v12 0x00494F50  (feb 0x00489AC0)
// Every line hidden; the message gone and the console lines reset.
void wrUIHUD_MSG::ClearMsg()
{
    for (int i = nCon; i < renderInfo.nElems; i++)
        hudHide(&renderInfo.elems[i]);
    fading = 1;
    time = 0;
    for (int i = 0; i < nConUsed; i++) {
        hudHide(&renderInfo.elems[i]);
        con[i].fading = 1;
        con[i].time = 0;
    }
}

// v12 0x00495010
int wrUIHUD_MSG::IsMsgShown()
{
    for (int i = nCon; i < renderInfo.nElems; i++)
        if (!(renderInfo.elems[i].flags40 & UIRF_SKIP))
            return 1;
    return 0;
}

// v12 0x00495060  (feb 0x00489B70)
// prio 1: also a console line (2: not again when the last one says the same); 8: the console only. The message is
// split at its new lines over the message lines and shown for time seconds.
void wrUIHUD_MSG::ShowMsgW(const wrWCHAR *s, float t, int prio)
{
    wrWCHAR buf[256];
    if (!(gsAppState & 0x8000000) && wrSysGO->mode == 0)
        prio &= ~1;
    if (prio & 1) {
        int i = 0;
        if (nConUsed > 0) {
            for (; i < nConUsed; i++)
                if (renderInfo.elems[i].flags40 & UIRF_SKIP)
                    break;
            if (i > 0 && (prio & 2)) {
                uiRENDER_ELEM_TEXT *last = (uiRENDER_ELEM_TEXT *)((u8 *)&renderInfo.elems[i - 1] + 0x94);
                if (!wcscmp((const wchar_t *)s, (const wchar_t *)last->GetString()))
                    i--;
            }
        }
        if (i == nConUsed) {
            RemoveConMsgLine(0);
            i--;
        }
        renderInfo.elems[i].SetTextW(s, 0x7FFFFFFF);
        renderInfo.elems[i].SetAlphaText(255.0f);
        hudShow(&renderInfo.elems[i]);
        con[i].time = 10.0f;
        con[i].fading = 0;
        con[i].flags = prio;
    }
    if (prio & 8)
        return;
    for (int i = nCon; i < renderInfo.nElems; i++)
        hudHide(&renderInfo.elems[i]);
    int line = nCon;
    fading = 1;
    time = 0;
    const wrWCHAR *nlp = (const wrWCHAR *)wcschr((const wchar_t *)s, L'\n');
    while (nlp) {
        if (line < renderInfo.nElems) {
            int n = (int)(nlp - s);
            wcsncpy((wchar_t *)buf, (const wchar_t *)s, n + 1);
            buf[n] = 0;
            renderInfo.elems[line].SetTextW(buf, 0x7FFFFFFF);
            renderInfo.elems[line].SetAlphaText(255.0f);
            hudShow(&renderInfo.elems[line]);
            line++;
        }
        s = nlp + 1;
        nlp = (const wrWCHAR *)wcschr((const wchar_t *)s, L'\n');
    }
    if (line < renderInfo.nElems) {
        wcsncpy((wchar_t *)buf, (const wchar_t *)s, 0x100);
        buf[255] = 0;
        renderInfo.elems[line].SetTextW(buf, 0x7FFFFFFF);
        renderInfo.elems[line].SetAlphaText(255.0f);
        hudShow(&renderInfo.elems[line]);
    }
    fading = 0;
    time = t;
    msgFlags = prio;
}

// v12 0x00495500 (feb 0x00489FB0) qsort: by the top edge
static int __cdecl _compareRectY(const void *a, const void *b)
{
    float ya = ((const uiRENDER_ELEM *)a)->rect.v[0].y, yb = ((const uiRENDER_ELEM *)b)->rect.v[0].y;
    if (ya < yb)
        return -1;
    if (ya == yb)
        return 0;
    return 1;
}

// v12 0x004953A0  (feb 0x00489E50)
// The lines sorted top to bottom; the leading "con_msg*" ones are the console; everything hidden.
int wrUIHUD_MSG::ProcessInit(void *p)
{
    uiELEMENT::ProcessInit(p);
    qsort(renderInfo.elems, renderInfo.nElems, sizeof(uiRENDER_ELEM), _compareRectY);
    int n = 0;
    while (n < renderInfo.nElems && strstr(renderInfo.elems[n].name, "con_msg"))
        n++;
    nCon = n;
    if (n > nConUsed && n > conCap) {
        con = (wrHUD_CON_LINE *)apRealloc(con, n * sizeof(wrHUD_CON_LINE));
        conCap = n;
    }
    nConUsed = n;
    for (int i = nCon; i < renderInfo.nElems; i++)
        hudHide(&renderInfo.elems[i]);
    fading = 1;
    time = 0;
    for (int i = 0; i < nConUsed; i++) {
        hudHide(&renderInfo.elems[i]);
        con[i].fading = 1;
        con[i].time = 0;
    }
    return 1;
}

// v12 0x00495540  (feb 0x0048A010)
// The message's time, then its fade (four times faster with flag 4); each console line's the same way, the
// faded ones scrolled out.
int wrUIHUD_MSG::ProcessUpdate(void *)
{
    float dt = gsElapsedTime;
    if (fading) {
        if (msgFlags & 4)
            dt *= 4.0f;
        if (m3dUpdateTimeField(&time, dt)) {
            for (int i = nCon; i < renderInfo.nElems; i++)
                hudHide(&renderInfo.elems[i]);
        } else {
            for (int i = nCon; i < renderInfo.nElems; i++)
                renderInfo.elems[i].SetAlphaText(time * HUD_MSG_ALPHA);
        }
    } else if (m3dUpdateTimeField(&time, dt)) {
        fading = 1;
        time = HUD_MSG_FADE;
    }
    for (int i = 0; i < nConUsed; i++) {
        if (renderInfo.elems[i].flags40 & UIRF_SKIP)
            break;
        wrHUD_CON_LINE *c = &con[i];
        float d = gsElapsedTime;
        if (c->fading) {
            if (c->flags & 4)
                d *= 4.0f;
            if (m3dUpdateTimeField(&c->time, d)) {
                RemoveConMsgLine(i);
                i--;
            } else {
                renderInfo.elems[i].SetAlphaText(con[i].time * HUD_MSG_ALPHA);
            }
        } else if (m3dUpdateTimeField(&c->time, d)) {
            con[i].fading = 1;
            con[i].time = HUD_MSG_FADE;
        }
    }
    return 1;
}

// v12 0x00495740  (feb 0x0048A1C0)
// Console line i out: the ones below move up (text, colour, flags, state); the last one emptied and hidden.
void wrUIHUD_MSG::RemoveConMsgLine(int i)
{
    int k = i + 1;
    for (; k < nConUsed; k++) {
        con[k - 1] = con[k];
        uiRENDER_ELEM *from = &renderInfo.elems[k], *to = &renderInfo.elems[k - 1];
        to->SetTextW(((uiRENDER_ELEM_TEXT *)((u8 *)from + 0x94))->GetString(), 0x7FFFFFFF);
        to->colorText = from->colorText;
        to->flags40 = from->flags40;
    }
    static const wrWCHAR empty[1] = { 0 };
    renderInfo.elems[k - 1].SetTextW(empty, 0x7FFFFFFF);
    hudHide(&renderInfo.elems[k - 1]);
}

// ================================================================================================ wrUIHUD_SP_WINDOW
// The single-player statistics window (TAB held, or the level over -- wrUI_SYSTEM::Update shows it).

// v12 0x00498590  (feb 0x0048C9B0)
uiELEMENT *__fastcall wrUIHUD_SP_WINDOW::Create()
{
    wrUIHUD_SP_WINDOW *w = new wrUIHUD_SP_WINDOW;
    w->spTime = w->spKills = w->spSecrets = w->spMoney = w->spFavWpn = w->spAccuracy = 0;
    return w;
}

// v12 0x0048DF30 (the shared "return 0" of the wr windows)
int wrUIHUD_SP_WINDOW::GetElemDefType()
{
    return 0;
}

// v12 0x004968C0  (feb 0x0048B1C0)
// Its six lines; hidden at first.
int wrUIHUD_SP_WINDOW::ProcessInit(void *p)
{
    if (!uiELEMENT::ProcessInit(p))
        return 0;
    spTime = FindChildElement("sp_time");
    spKills = FindChildElement("sp_kills");
    spSecrets = FindChildElement("sp_secrets");
    spMoney = FindChildElement("sp_money");
    spFavWpn = FindChildElement("sp_fav_wheapon");
    spAccuracy = FindChildElement("sp_accuracy");
    flags |= UIEF_HIDDEN;
    return 1;
}

// v12 0x00496950  (feb 0x0048B250)
// While shown: the level's time (h:m:s), the kills, secrets and gold of the level's totals, the weapon held longest,
// the accuracy (hits of shots, in percent rounded; no shots: 100).
int wrUIHUD_SP_WINDOW::ProcessUpdate(void *p)
{
    if (flags & UIEF_HIDDEN)
        return uiELEMENT::ProcessUpdate(p);
    int t = (int)wrSysStat->GetMapTime();
    ((uiAREA *)spTime)->renderInfo.elems->SetTextW(gsStrings->Printf("UI_HUD_SP_MAP_TIME_FMT", t / 3600, t / 60 % 60, t % 60), 0x7FFFFFFF);
    int total = 0, n = wrSysStat->GetMapKills((plrPLAYER_COMMON *)plrPlayer, &total);
    ((uiAREA *)spKills)->renderInfo.elems->SetTextW(gsStrings->Printf("UI_HUD_SP_MAP_KILLS_FMT", n, total), 0x7FFFFFFF);
    n = wrSysStat->GetMapSecrets((plrPLAYER_COMMON *)plrPlayer, &total);
    ((uiAREA *)spSecrets)->renderInfo.elems->SetTextW(gsStrings->Printf("UI_HUD_SP_MAP_SECRETS_FMT", n, total), 0x7FFFFFFF);
    n = wrSysStat->GetMapMoney((plrPLAYER_COMMON *)plrPlayer, &total);
    ((uiAREA *)spMoney)->renderInfo.elems->SetTextW(gsStrings->Printf("UI_HUD_SP_MAP_MONEY_FMT", n, total), 0x7FFFFFFF);
    int w = wrSysStat->GetFavouriteWpn((plrPLAYER_COMMON *)plrPlayer);
    ((uiAREA *)spFavWpn)->renderInfo.elems->SetTextW(gsStrings->GetStringById(gsStrings->GetStringIdFmt("WPN_%i", w)), 0x7FFFFFFF);
    int shots = 0, hits = 0;
    wrSysStat->GetShootAccuracy((plrPLAYER_COMMON *)plrPlayer, &shots, &hits);
    if (shots < hits)
        hits = shots;
    int pc = shots ? (shots / 2 + hits * 100) / shots : 100;
    ((uiAREA *)spAccuracy)->renderInfo.elems->SetTextW(gsStrings->Printf("UI_HUD_SP_MAP_ACCURACY_FMT", hits, shots, pc), 0x7FFFFFFF);
    return uiELEMENT::ProcessUpdate(p);
}
