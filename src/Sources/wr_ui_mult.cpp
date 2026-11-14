// wr_ui_mult.cpp -- Will Rock 1.2 (v12.exe): multiplayer menu windows (connection type,
// driver init, player setup, IP address).  Reconstructed documentation source;
// NOT meant to compile into the game.  Types: wr_ui_mult.h.
//
// Original object: wr_ui_mult.obj (feb 0x00490120..0x00490E3F, v12 0x0049C050..0x0049CE5F).
//
// Menu flow (window names are ui.tpl names, shown with uiSystem vt+0x10):
//
//   "connection"  --LANS/LANC/IPC/UBIS/UBIC-->  Drv_Create(type, 10020000)
//        |                                       "mp_wait_drvinit" (wrUI_DRVINIT_WND)
//        v                                        polls Drv_IsCreated / Drv_IsError
//   "setup_player" (wrUI_PSETUP_WND)  --NEXT-->  by mpMode:
//        0 LAN server, 3 Ubi server  -> "create_server"   (wrUI_ADVSERV_WINDOW, wr_ui_server.cpp)
//        1 LAN client, 4 Ubi client  -> Browser_Start + "mp_wait_browser" (wr_ui_join.cpp)
//        2 IP client                 -> "ip_addr" (wrUI_IPADDR_WND) -> Client_JoinAddress + "mp_wait_join"

#include "wr_ui_mult.h"
// [integrator: all MP headers, so cross-object types are complete for the syntax check]
#include "network.h"
#include "network_dpubi.h"
#include "network_dpubi_dp.h"
#include "mp_messages.h"
#include "gs_mp_conn.h"
#include "gs_mp_network.h"
#include "gs_mp_misc.h"
#include "gs_mp_client.h"
#include "gs_mp_server.h"
#include "gs_play.h"
#include "wr_go_mp.h"
#include "wr_ui_mult.h"
#include "wr_ui_server.h"
#include "wr_ui_join.h"

int wrUI_CONN_WND::mpMode = -1;                               // v12 0x0064F8E8  (feb 0x0060EE60)

const wchar_t* wrUI_BlankNameChars = L" .,`'\"=-_+<>?|\\/!^&*";  // v12 0x00605AF0 -> 0x00631794 // ? exact contents not re-dumped for 1.2

// ===========================================================================
// wrUI_MP_WND
// ===========================================================================

// v12 0x0049C050  (feb 0x00490120)
// ctor: no status line yet.
wrUI_MP_WND::wrUI_MP_WND()
    : uiELEMENT()
{
    statusStr  = NULL;
    statusTime = 0;
}

// v12 0x0049C070  (feb 0x00490140)
// Finds the "status_str" child and marks the window as wanting updates (flag 8).
int wrUI_MP_WND::ProcessInit(void* data)
{
    statusStr = (uiAREA*)FindChildElement("status_str");
    flags |= 8;                                   // ? "needs ProcessUpdate" flag
    return uiELEMENT::ProcessInit(data);          // shared stub 0x00519080 (returns 1)
}

// v12 0x0049C0A0  (feb 0x00490170)
// Shows a network status code as text: localized "UI_NET_STATUS_<code>" if it exists,
// else the raw code; only the last line of a multi-line string is shown.
void wrUI_MP_WND::SetStatusStr(const char* status, float time)
{
    if (statusStr == NULL)
        return;

    statusTime = time;
    int id = gsStrings->GetStringIdFmt("UI_NET_STATUS_%s", status);
    const wchar_t* text = id ? gsStrings->GetStringById(id)
                             : gsStrings->AsciiToUnicode(status);

    const wchar_t* lastLine = wcsrchr(text, L'\n');
    if (lastLine)
        statusStr->SetTextW(lastLine + 1, 0x7FFFFFFF);
    else
        statusStr->SetTextW(text, 0x7FFFFFFF);
}

// v12 0x0049C120  (feb 0x004901F0)
// Swallows Tab so focus cannot leave the wait window.
int wrUI_MP_WND::OnPreInput(uiINPUT_INFO* in)
{
    if (in->type == 6 && in->key == 9)            // key down, Tab
        return 0;
    return uiELEMENT::OnPreInput(in);             // 0x00519080
}

// v12 0x0049C140  (feb 0x00490210)
// Escape acts like the BACK button: sends a fake 'BTNC'/'BACK' notify to itself.
int wrUI_MP_WND::OnKeyboard(uiINPUT_INFO* in)
{
    uiCHILD_NOTIFY n = { NULL, 0, 0 };
    if (in->type == 6 && in->key == 7) {          // key down, Escape
        n.child = this;
        n.type  = UINOTIFY_BUTTON;
        n.code  = UIBTN_BACK;
        OnChildNotify(&n);                        // vt+0x5C
    }
    return uiELEMENT::OnKeyboard(in);             // 0x00519080
}

// ===========================================================================
// wrUI_CONN_WND  ("connection")
// ===========================================================================

// v12 0x0049C1A0  (feb 0x00490270)
// On open: forget the previous choice and stop any network driver.
// 1.2: in a -dedicated process the client-only buttons are disabled instead of
//      feb's "grey out the two Ubi.com buttons" code.
int wrUI_CONN_WND::StartAppearing(int start)
{
    if (start) {
        mpMode = -1;
        gsSysNetwork->Drv_Stop();                                 // 0x0052F120

        // 1.2: feb always did, for "conn_btn04" and "conn_btn05":
        //        btn->flags |= 0x800; btn->renderInfo.SetAlphaText(40.0f);
        //      (the Ubi.com buttons were shown but dimmed in the Feb build).
        //      1.2 leaves all buttons enabled, except on a dedicated server:
        if (gsAppState & 0x08000000) {                            // -dedicated
            uiELEMENT* b;
            if ((b = FindChildElement("conn_btn02")) != NULL) b->Enable(0);   // vt+0x10
            if ((b = FindChildElement("conn_btn03")) != NULL) b->Enable(0);
            if ((b = FindChildElement("conn_btn05")) != NULL) b->Enable(0);
            // ? which button is which is defined in ui.tpl (not checked); conn_btn01 and
            //   conn_btn04 stay usable -- presumably the two "host" choices
        }
    }
    return uiELEMENT::StartAppearing(start);                      // 0x00518E10
}

// v12 0x0049C230  (feb 0x004902F0)
// A connection button was pressed: remember the mode, create the network driver
// (type 0 = DirectPlay only, type 1 = DirectPlay + Ubi.com lobby) and show
// "mp_wait_drvinit".
// 1.2: Drv_Create takes the network version id (10020000) as a second argument.
// 1.2: new button 'UBIA' opens the Ubi.com sign-up page.
void wrUI_CONN_WND::OnChildNotify(uiCHILD_NOTIFY* n)
{
    if (n->type != UINOTIFY_BUTTON) {
        uiELEMENT::OnChildNotify(n);                              // 0x00519090
        return;
    }

    switch (n->code) {
    case UIBTN_LAN_SERVER:  mpMode = 0; gsSysNetwork->Drv_Create(0, 10020000); break;   // 0x0052EF10
    case UIBTN_LAN_CLIENT:  mpMode = 1; gsSysNetwork->Drv_Create(0, 10020000); break;
    case UIBTN_IP_CLIENT:   mpMode = 2; gsSysNetwork->Drv_Create(0, 10020000); break;
    case UIBTN_BACK:        gsSysNetwork->Drv_Stop();                          break;
    case UIBTN_UBI_CLIENT:  mpMode = 4; gsSysNetwork->Drv_Create(1, 10020000); break;
    case UIBTN_UBI_SERVER:
        mpMode = 3;
        gsSysNetwork->Drv_Create(1, 10020000);
        uiSystem->ShowWindow("mp_wait_drvinit", 8);               // shown twice (also below), as in feb
        break;
    case UIBTN_UBI_ACCOUNT:                                       // 1.2: new
        /* Ubi.com service: not reconstructed (dead service / licensing) */
        // (reads [Multiplayer] "UbiComAccoutURL" with a hard-coded https://secure.ubi.com/...
        //  default into apBuffer and ShellExecuteA(NULL, "open", apBuffer, "", NULL, SW_SHOWNORMAL);
        //  mpMode stays -1 so the window does not change)
        break;
    }

    if (mpMode != -1) {
        uiSystem->ShowWindow("mp_wait_drvinit", 8);
        return;
    }
    uiELEMENT::OnChildNotify(n);
}

// ===========================================================================
// wrUI_DRVINIT_WND  ("mp_wait_drvinit")  /  wrUI_WAITSERVER_WND::OnChildNotify
// ===========================================================================

// v12 0x0049C390  (feb 0x004903E0)  -- also wrUI_DRVINIT_WND's slot +0x5C (identical code folded)
// BACK (or Escape): stop the driver and return to "connection".
void wrUI_WAITSERVER_WND::OnChildNotify(uiCHILD_NOTIFY* n)
{
    if (n->type == UINOTIFY_BUTTON && n->code == UIBTN_BACK) {
        gsSysNetwork->Drv_Stop();
        uiSystem->ShowWindow("connection", 0);
        return;
    }
    uiELEMENT::OnChildNotify(n);
}

// v12 0x0049C3D0  (feb 0x00490420)
// On open: show the driver status ("UI_NET_STATUS_<code>"); SetStatusStr inlined.
int wrUI_DRVINIT_WND::StartAppearing(int start)
{
    if (start) {
        const char* status = gsSysNetwork->Drv_GetStatus();      // 0x0052F0D0
        if (statusStr) {
            statusTime = 0;
            int id = gsStrings->GetStringIdFmt("UI_NET_STATUS_%s", status);
            const wchar_t* text = id ? gsStrings->GetStringById(id) : gsStrings->AsciiToUnicode(status);
            const wchar_t* last = wcsrchr(text, L'\n');
            statusStr->SetTextW(last ? last + 1 : text, 0x7FFFFFFF);
        }
    }
    return uiELEMENT::StartAppearing(start);
}

// v12 0x0049C460  (feb 0x004904B0)
// Per frame: driver up -> "setup_player"; driver failed -> message box
// UI_ERR_NET_DRV_INIT_ERROR (then "connection"); else refresh the status line.
// Not polled while the window is appearing/disappearing (flags & 0x140).
int wrUI_DRVINIT_WND::ProcessUpdate(void* data)
{
    if ((flags & 0x140) == 0) {
        if (gsSysNetwork->Drv_IsCreated()) {                      // 0x0052F060
            uiSystem->ShowWindow("setup_player", 0);
            return uiELEMENT::ProcessUpdate(data);
        }
        if (gsSysNetwork->Drv_IsError()) {                        // 0x0052F090
            int id = gsStrings->GetStringIdFmt("UI_NET_STATUS_%s", gsSysNetwork->Drv_GetStatus());
            const wchar_t* detail = id ? gsStrings->GetStringById(id)
                                       : gsStrings->AsciiToUnicode(gsSysNetwork->Drv_GetStatus());
            const wchar_t* title = gsStrings->GetStringById(gsStrings->GetStringId("UI_ERR_NET_DRV_INIT_ERROR"));
            uiSystem->MessageBox(title, detail, 0, 0, 0, "connection");   // vt+0x14
            return uiELEMENT::ProcessUpdate(data);
        }
    }

    // still working (or appearing): show the status code
    const char* status = gsSysNetwork->Drv_GetStatus();
    if (statusStr) {
        statusTime = 0;
        int id = gsStrings->GetStringIdFmt("UI_NET_STATUS_%s", status);
        const wchar_t* text = id ? gsStrings->GetStringById(id) : gsStrings->AsciiToUnicode(status);
        const wchar_t* last = wcsrchr(text, L'\n');
        statusStr->SetTextW(last ? last + 1 : text, 0x7FFFFFFF);
    }
    return uiELEMENT::ProcessUpdate(data);
}

// ===========================================================================
// wrUI_PSETUP_WND  ("setup_player")
// ===========================================================================

// v12 0x0049C5F0  (feb 0x00490640)
// Finds the controls, fills the skin list (6 skins) and the connection-speed list
// (UI_CONNECTION_1.. while the string exists, max 98), sets edit lengths.
// 1.2: a -dedicated process disables the skin selector.
int wrUI_PSETUP_WND::ProcessInit(void* data)
{
    playerName     = (uiTEXT_INPUT*)FindChildElement("player_name");
    playerPassword = (uiTEXT_INPUT*)FindChildElement("player_password");
    playerSkin     = (wrUI_LIST*)   FindChildElement("player_skin");
    charPicture    =                FindChildElement("char_pct");
    connectionType = (wrUI_LIST*)   FindChildElement("connection_type");

    if (gsAppState & 0x08000000)                                  // 1.2: dedicated: no skin choice
        playerSkin->Enable(0);

    playerSkin->strings.PreInit(6, 0x60);                         // +0x138
    {                                                             // UI_SKIN_1 .. UI_SKIN_6, value 0
        int i = 0;
        do {
            i++;
            playerSkin->strings.AddString(gsStrings->GetStringIdFmt("UI_SKIN_%i", i), 0);
        } while (i < 6);
    }
    playerSkin->strings.Compact();

    connectionType->strings.PreInit(8, 0x100);
    connectionType->strings.Clear();
    for (int i = 0; i < 99; i++) {
        int id = gsStrings->GetStringIdFmt("UI_CONNECTION_%i", i + 1);
        if (id == 0)
            break;
        connectionType->strings.AddString(id, i);                 // value = 0-based index
    }
    connectionType->strings.Compact();

    playerName->SetMaxLen(0x14);                                  // 20 chars   (vt+0x6C, feb +0x64)
    playerPassword->SetMaxLen(0x20);                              // 32 chars
    playerPassword->inputFlags |= 4;                              // +0x13D (feb +0x13C): password mode (*)  // ? field moved by one byte
    return uiELEMENT::ProcessInit(data);
}

// v12 0x0049C750  (feb 0x00490780)
// On open: load the settings for the current driver.
int wrUI_PSETUP_WND::StartAppearing(int start)
{
    if (start)
        LoadSettings();
    return uiELEMENT::StartAppearing(start);
}

// v12 0x0049C770  (feb 0x004907A0)
// NEXT: save, refuse an empty/blank name, then go on according to mpMode
// (jump table v12 0x0049C8E4).  Skin list change: update the skin icon.
void wrUI_PSETUP_WND::OnChildNotify(uiCHILD_NOTIFY* n)
{
    if (n->type == UINOTIFY_BUTTON) {
        if (n->code != UIBTN_BACK) {
            SaveSettings();
            if (n->code == UIBTN_NEXT) {
                const wchar_t* name = playerName->GetEditText();  // 1.2: virtual vt+0x64
                if (*name == 0)
                    return;
                // the name must contain at least one character outside wrUI_BlankNameChars
                while (wcschr(wrUI_BlankNameChars, *name) != NULL) {
                    name++;
                    if (*name == 0)
                        return;
                }
                switch (wrUI_CONN_WND::mpMode) {
                case 0:     // LAN server
                case 3:     // Ubi.com server
                    uiSystem->ShowWindow("create_server", 0);
                    uiELEMENT::OnChildNotify(n);
                    return;
                case 1:     // LAN client
                case 4:     // Ubi.com client
                    gsSysNetwork->Browser_Start();                // 0x0052F490
                    uiSystem->ShowWindow("mp_wait_browser", 8);
                    uiELEMENT::OnChildNotify(n);
                    return;
                case 2:     // join by address
                    uiSystem->ShowWindow("ip_addr", 0);
                    uiELEMENT::OnChildNotify(n);
                    return;
                }
            }
        }
    }
    else if (n->type == UINOTIFY_SCROLLCHG && n->code == UINOTIFY_CHANGE && n->child == playerSkin) {
        txmTEXTURE* tex = txmManager->Add(wrUI_SkinIcons[playerSkin->curIndex], 0, -1);  // +0x12C
        if (tex) {
            charPicture->flags &= ~0x2000;                        // visible
            charPicture->renderInfo.SetTex(tex);
            uiELEMENT::OnChildNotify(n);
            return;
        }
        charPicture->flags |= 0x2000;                             // hidden
    }
    uiELEMENT::OnChildNotify(n);
}

// v12 0x0049C900  (feb 0x00490950)
// Writes [Multiplayer] PlayerName_<drv>, PlayerName, PlayerSkin, PlayerPassword_<drv>
// (only if the driver wants a password, i.e. Ubi.com), passes name/password to the
// driver.  <drv> = Drv_GetIdString() ("LAN" / "UBI").
// 1.2: also writes [CONFIG] ConnSpeed = connection_type index (read by
//      gsMP_SERVER::InitGame v12 0x00510348 -> snapshot rate factor 1/2/4).
void wrUI_PSETUP_WND::SaveSettings()
{
    char key[64];
    char password[64];
    char name[64];

    strcpy(key, "PlayerName_");
    strcat(key, gsSysNetwork->Drv_GetIdString());                // 0x0052F190
    if (gsStrings->UnicodeToAscii(playerName->GetEditText(), name, 0x40)) {
        apCfgWriteString("Multiplayer", key, name);
        apCfgWriteString("Multiplayer", "PlayerName", name);
    }
    apCfgWriteInt("Multiplayer", "PlayerSkin", playerSkin->curIndex);

    if (gsSysNetwork->Drv_IsPasswordRequired()) {                 // 0x0052F1E0
        strcpy(key, "PlayerPassword_");
        strcat(key, gsSysNetwork->Drv_GetIdString());
        if (gsStrings->UnicodeToAscii(playerPassword->GetEditText(), password, 0x40))
            apCfgWriteString("Multiplayer", key, password);
        /* Ubi.com service: not reconstructed (dead service / licensing) -- the password is
           only the Ubi.com account password; it is written to the ini in clear text */
    } else {
        password[0] = 0;
    }

    apCfgWriteInt("CONFIG", "ConnSpeed", connectionType->curIndex);   // 1.2: new
    gsSysNetwork->Drv_SetUserNameAndPassword(name, password);         // 0x0052F200
    // note: if UnicodeToAscii failed, 'name' is passed uninitialised (as in feb)
}

// v12 0x0049CA90  (feb 0x00490AC0)
// Reads the values written by SaveSettings back into the controls.
// Defaults: name "WillRock", password "", skin 0 (valid 0..5).
// 1.2: also reads [CONFIG] ConnSpeed into connection_type (0 if missing or out of range).
void wrUI_PSETUP_WND::LoadSettings()
{
    char key[64];
    char value[256];
    int  skin;

    strcpy(key, "PlayerName_");
    strcat(key, gsSysNetwork->Drv_GetIdString());
    if (!_apCfgReadString(NULL, "Multiplayer", key, value, 0x100))
        strcpy(value, "WillRock");
    playerName->SetEditText(gsStrings->AsciiToUnicode(value));    // 1.2: virtual vt+0x60

    if (gsSysNetwork->Drv_IsPasswordRequired()) {
        strcpy(key, "PlayerPassword_");
        strcat(key, gsSysNetwork->Drv_GetIdString());
        if (!_apCfgReadString(NULL, "Multiplayer", key, value, 0x100))
            value[0] = 0;
        playerPassword->SetEditText(gsStrings->AsciiToUnicode(value));
        playerPassword->Enable(1);
        for (uiELEMENT* c = firstChild; c; c = c->next)           // show the password label row(s)
            if (c->byteFlags68 & 2)
                c->flags &= ~0x2000;
    } else {
        playerPassword->Enable(0);
        for (uiELEMENT* c = firstChild; c; c = c->next)           // hide them
            if (c->byteFlags68 & 2)
                c->flags |= 0x2000;
    }

    if (!_apCfgReadInt(NULL, "Multiplayer", "PlayerSkin", &skin) || skin < 0 || skin >= 6)
        skin = 0;
    playerSkin->MoveSel(skin - playerSkin->curIndex);             // vt+0x60

    txmTEXTURE* tex = txmManager->Add(wrUI_SkinIcons[playerSkin->curIndex], 0, -1);
    if (tex) {
        charPicture->flags &= ~0x2000;
        charPicture->renderInfo.SetTex(tex);
    } else {
        charPicture->flags |= 0x2000;
    }

    // 1.2: new
    int speed;
    if (!_apCfgReadInt(NULL, "CONFIG", "ConnSpeed", &speed) || speed < 0 || speed >= connectionType->GetCount())
        speed = 0;                                                // GetCount = vt+0x64
    connectionType->MoveSel(speed - connectionType->curIndex);
}

// ===========================================================================
// wrUI_IPADDR_WND  ("ip_addr")
// ===========================================================================

// v12 0x0049CD20  (feb 0x00490CF0)
int wrUI_IPADDR_WND::ProcessInit(void* data)
{
    ipAddrEdit = (uiTEXT_INPUT*)FindChildElement("ip_addr_edit");
    return uiELEMENT::ProcessInit(data);
}

// v12 0x0049CD40  (feb 0x00490D10)
// On open: prefill with [Multiplayer] ClientServerAddr (default "0.0.0.0").
int wrUI_IPADDR_WND::StartAppearing(int start)
{
    if (start) {
        char addr[256];
        if (!_apCfgReadString(NULL, "Multiplayer", "ClientServerAddr", addr, 0x100))
            strcpy(addr, "0.0.0.0");
        ipAddrEdit->SetEditText(gsStrings->AsciiToUnicode(addr));  // 1.2: virtual vt+0x60
    }
    return uiELEMENT::StartAppearing(start);
}

// v12 0x0049CDC0  (feb 0x00490D80)
// NEXT or Enter in the edit box: remember the address, start the DirectPlay join
// ("host" or "host:port", see gsMP_NETWORK::Client_JoinAddress) and wait in "mp_wait_join".
void wrUI_IPADDR_WND::OnChildNotify(uiCHILD_NOTIFY* n)
{
    char addr[256];

    if (!((n->type == UINOTIFY_BUTTON && n->code == UIBTN_NEXT) ||
          (n->type == UINOTIFY_INPUT  && n->code == UINOTIFY_ENTER))) {
        uiELEMENT::OnChildNotify(n);
        return;
    }
    gsStrings->UnicodeToAscii(ipAddrEdit->GetEditText(), addr, 0x100);
    apCfgWriteString("Multiplayer", "ClientServerAddr", addr);    // call at 0x0049CE20
    gsSysNetwork->Client_JoinAddress(addr);                       // 0x0052F7A0, call at 0x0049CE30
    uiSystem->ShowWindow("mp_wait_join", 8);
}

// _$E2 (v12 0x0049CD00, feb 0x00490930): static float init, see header.
