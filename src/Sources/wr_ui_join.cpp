// wr_ui_join.cpp -- Will Rock 1.2 (v12.exe): server browser and the join wait windows.
// Reconstructed documentation source; NOT meant to compile into the game.
// Types: wr_ui_join.h.
//
// Original object: wr_ui_join.obj (feb 0x0048CB30..0x0048DD2F, v12 0x00498710..0x00499A3F, 0x00499AA0).
//
// Flow:  "setup_player" NEXT (mpMode 1/4) -> Browser_Start -> "mp_wait_browser"
//        Browser_IsStarted -> "join"   (list refreshed every Drv_GetRefreshInterval() s, or REFR)
//        JOIN / double click -> Join(): version check -> Browser_Stop(1), Client_JoinBrowsedGame(id),
//        "mp_wait_join" -> Client_IsJoined -> StartLoadingAction(1, 3 /*client*/, "")
//
// 1.2 summary: list sorted (same version first, then by ping), a ping and a version column,
// version check by gsMP_SERVER_INFO::versionId (10020000) instead of the blob's dataVersion,
// dedicated servers shown with one player slot less, list capped at 64 entries (feb could write 65).

#include "wr_ui_join.h"
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

// ===========================================================================
// wrUI_JOIN_INFO  (one list row)
// ===========================================================================

// v12 0x00498710  (feb 0x0048CB30)
// Sorts the row's text elements left to right, marks the row as a focusable list row.
void wrUI_JOIN_INFO::Init(objOBJ* obj)
{
    uiAREA::Init(obj);
    renderInfo.SetColorText(0x5C000000);                          // dim alpha
    qsort(renderInfo.elems, renderInfo.nElems, 0xAC, _compareRectX);
    flags |= 0x8006;
    renderInfo.flags40 |= 0x10;                                   // +0xAC  // ?
}

// v12 0x00498770  (feb 0x0048CB90)
// Fills a row from the browser data of one server.
void wrUI_JOIN_INFO::SetRecord(wrUI_JOIN_RECORD* rec)
{
    wrMP_MAIN_DATA   md;
    gsMP_SERVER_INFO info;
    wchar_t          text[0x50];
    int              size, strId, unused;

    memset(&md, 0, sizeof(md));
    md.dataVersion = 0x8D;
    md._29         = 1;

    if (!gsSysNetwork->Browser_IsServerIdValid(rec->serverId)) {  // 0x0052F630
        for (int i = 0; i < 3; i++)                               // only the first 3 columns (as feb)
            renderInfo.elems[i].SetTextW(L"", 0x7FFFFFFF);
        return;
    }

    gsSysNetwork->Browser_GetServerInfo(rec->serverId, &info);   // 0x0052F660
    size = 0x30;
    gsSysNetwork->Browser_GetServerMainData(rec->serverId, &md, &size);   // 0x0052F6D0

    // 1.2: blobs from older servers (dataVersion <= 0x8C) have no flags; a 1.2 blob
    // with _29 == 0 is shown as "???" / "DM"
    if (md.dataVersion <= 0x8C) {
        md._29 = 0;
        md.dedicated = 0;
    } else if (md._29 < 1) {
        strcpy(md.level, "???");
        strcpy(md.mode, "DM");
        md.dedicated = 0;
    }
    md.level[0x1F] = 0;
    md.mode[4]     = 0;
    if (md.dedicated & 1) {                                       // 1.2: hide the dedicated host's own slot
        info.curPlayers--;
        info.maxPlayers--;
    }

    // server name, cut to the row width with "..."
    // 1.2: feb printed "v%i.%i %S" (version from (dataVersion - 23) / 100, % 100) + name here
    gsStrings->AsciiToUnicode(info.name, text, 0x50);             // 0x0051D180
    float width = renderInfo.rect.v[1].x - renderInfo.rect.v[0].x; // +0x5C - +0x4C of renderInfo
    wchar_t* end = text + wcslen(text) - 1;                       // ? exact start of the cut
    bool cut = false;
    while (end > text) {
        m2dV sz;
        if ((renderInfo.font->GetStringSizeW(&sz, text, &renderInfo.fontSize, 0), sz.x) <= width)   // [integrator] SDK form: size returned in sz
            break;
        *end-- = 0;
        cut = true;
    }
    if (cut)
        wcscat(text, L"...");
    renderInfo.SetTextW(text, 0x7FFFFFFF);

    // [0] ping  (1.2: new column)
    int ping = info.ping;
    if (ping == 0)
        wcsncpy(text, L"???", 0x50);
    else if (ping == 9999 || ping == -9999)
        wcsncpy(text, L"---", 0x50);                              // 9999 = no answer
    else if (ping > 0 && ping <= 20)
        wcsncpy(text, L"<20", 0x50);
    else {
        if (ping < 0) ping = -ping;                               // negative = estimated ?  // ?
        if (ping > 999) ping = 999;
        swprintf(text, L"%i", ping);
    }
    renderInfo.elems[0].SetTextW(text, 0x7FFFFFFF);

    // [1] players
    swprintf(text, L"%i/%i", info.curPlayers, info.maxPlayers);
    renderInfo.elems[1].SetTextW(text, 0x7FFFFFFF);

    // [2] mode: "GAME_MP_MODE_ABB_<mode>" or the raw string
    int id = gsStrings->GetStringIdFmt("GAME_MP_MODE_ABB_%s", md.mode);
    if (id)
        renderInfo.elems[2].SetTextW(gsStrings->GetStringById(id), 0x7FFFFFFF);
    else {
        gsStrings->AsciiToUnicode(md.mode, text, 0x50);
        renderInfo.elems[2].SetTextW(text, 0x7FFFFFFF);
    }

    // [3] map: localized level name (1.2: _uiGetLevelNameStrId; feb "GAME_MP_MAP_%s")
    if (_uiGetLevelNameStrId(md.level, &strId, &unused, 0))       // 0x0048D3E0
        renderInfo.elems[3].SetTextId(strId);
    else
        renderInfo.elems[3].SetTextW(gsStrings->AsciiToUnicode(md.level), 0x7FFFFFFF);

    // row colour: other network version -> tinted (0x64 in the red/green byte)
    unsigned long color = (unsigned long)(renderInfo.colorText >> 24) << 24; // byte +0x117 (colorText alpha)
    if (info.versionId != 10020000)                               // 1.2 (feb: md.dataVersion != 0x7B)
        color |= 0x640000;
    renderInfo.SetColorText(color);

    // [4] version (1.2: new column)
    if ((unsigned)info.versionId < 10010000)                      // 0x98BD90
        swprintf(text, L"v1.0");
    else
        swprintf(text, L"v%i.%i", info.versionId / 10000000, (info.versionId % 10000000) / 10000);
    renderInfo.elems[4].SetTextW(text, 0x7FFFFFFF);

    renderInfo.CalcBBox(&bbox);                                   // +0x58
}

// v12 0x00498B90  (feb 0x0048CE90)
// Empty row: string id 1 (blank) as main text, "" in every column.
void wrUI_JOIN_INFO::ClearRecord()
{
    renderInfo.SetTextW(gsStrings->GetStringById(1), 0x7FFFFFFF);
    for (int i = 0; i < renderInfo.nElems; i++)
        renderInfo.elems[i].SetTextW(L"", 0x7FFFFFFF);
}

// v12 0x00499AA0  (feb 0x0048DD80)
// Click focuses the row; double click (event 4) sends 'DBLC'/'INFO' to the window (= Join).
int wrUI_JOIN_INFO::OnMouseButton(uiINPUT_INFO* in)
{
    if (in->type == 2) {                                          // button down
        uiSystem->SetKbdFocus(this);
        return uiELEMENT::OnMouseButton(in);                      // 0x00519080
    }
    if (in->type == 4) {                                          // double click
        uiSystem->SetKbdFocus(this);
        uiCHILD_NOTIFY n = { this, UINOTIFY_DBLCLICK, 'INFO' };
        parent->OnChildNotify(&n);
    }
    return uiELEMENT::OnMouseButton(in);
}

// ===========================================================================
// wrUI_JOIN_WINDOW  ("join")
// ===========================================================================

// helper used three times, inlined each time (feb: UpdateContext body)
static void BindRows(wrUI_JOIN_WINDOW* w)
{
    int n = w->nRecords - w->topRow;
    if (n > w->nRowWidgets) n = w->nRowWidgets;
    int i = 0;
    for (; i < n; i++) {
        w->rowWidgets[i]->Enable(1);
        w->rowWidgets[i]->SetRecord(&w->records[w->topRow + i]);
    }
    for (; i < w->nRowWidgets; i++) {
        w->rowWidgets[i]->ClearRecord();                          // inlined
        w->rowWidgets[i]->Enable(0);
    }
}

// selected row bright (alpha 207), others dim (92)
static void HighlightRows(wrUI_JOIN_WINDOW* w)
{
    for (int i = 0; i < w->nRowWidgets; i++)
        w->rowWidgets[i]->renderInfo.SetAlphaText(i == w->selRec - w->topRow ? 207.0f : 92.0f);
}

// v12 0x00498C10  (feb 0x0048CEF0)
// On open: empty list, reset the refresh timer, show the client version.
// 1.2: version text "v1.2" (feb "v1.0").
int wrUI_JOIN_WINDOW::StartAppearing(int start)
{
    if (start) {
        nRecords = 0;
        selRec   = -1;
        BindRows(this);                                           // all rows cleared and disabled
        SetSelectedRec(selRec, 1);                                // 0x0048FE30
        refreshTimer = gsSysNetwork->Drv_GetRefreshInterval();    // 0x0052F270

        wchar_t ver[32];
        swprintf(ver, L"v%i.%i", 1, 2);                           // 1.2 (feb 1, 0)
        ((uiAREA*)FindChildElement("version"))->SetTextW(ver, 0x7FFFFFFF);
        HighlightRows(this);
    }
    return uiELEMENT::StartAppearing(start);
}

// v12 0x00498DC0  (feb 0x0048D0A0)
// BACK: stop browsing, back to "setup_player".  JOIN or double click: Join().
// REFR: refresh now.  Row focus: select it and re-highlight.  Unchanged in 1.2.
void wrUI_JOIN_WINDOW::OnChildNotify(uiCHILD_NOTIFY* n)
{
    if (n->type == UINOTIFY_BUTTON) {
        switch (n->code) {
        case UIBTN_BACK:
            gsSysNetwork->Browser_Stop(0);                        // 0x0052F5A0
            uiSystem->ShowWindow("setup_player", 0);
            break;
        case UIBTN_JOIN:
            Join();
            break;
        case UIBTN_REFRESH:
            refreshTimer = gsSysNetwork->Drv_GetRefreshInterval();
            gsSysNetwork->Browser_RefreshServers();               // 0x0052F5D0
            break;
        }
    }
    else if (n->type == UINOTIFY_DBLCLICK) {
        if (n->code == 'INFO')
            Join();
        return;
    }
    else if (n->type == UINOTIFY_FOCUS) {
        for (int i = 0; i < nRowWidgets; i++) {
            if (n->child == rowWidgets[i]) {
                int row = topRow + i;
                if (row < nRecords && n->code != 0)
                    selRec = (nRecords != 0 && row != -1) ? (row < 0 ? 0 : row) : -1;
                break;
            }
        }
        HighlightRows(this);
        return;
    }

    if (n->type != UINOTIFY_FOCUS) {                              // (always true here)
        uiELEMENT::OnChildNotify(n);
        return;
    }
}

// v12 0x00498F80  (feb 0x0048D260)
// Joins the selected server if it runs the same network version, else shows
// UI_ERROR / UI_ERR_SERVER_VERSION.
// 1.2: checks gsMP_SERVER_INFO::versionId == 10020000 (feb: main-data dataVersion == 0x7B).
void wrUI_JOIN_WINDOW::Join()
{
    gsMP_SERVER_INFO info;

    if (selRec < 0 || selRec >= nRecords)
        return;

    gsSysNetwork->Browser_GetServerInfo(records[selRec].serverId, &info);
    if (info.versionId == 10020000) {                             // check at 0x00498FAF
        gsSysNetwork->Browser_Stop(1);                            // 1 = keep the driver for the join
        gsSysNetwork->Client_JoinBrowsedGame(records[selRec].serverId);   // 0x0052F740, call at 0x00498FD4
        uiSystem->ShowWindow("mp_wait_join", 8);
        return;
    }
    const ushort* text  = gsStrings->GetStringById(gsStrings->GetStringId("UI_ERR_SERVER_VERSION"));
    const ushort* title = gsStrings->GetStringById(gsStrings->GetStringId("UI_ERROR"));
    uiSystem->MessageBox(title, text, 0, 0, 0, parent->name);     // parent +0x18 = window name  // ?
}

// v12 0x00499490  (new in 1.2, no feb twin)
// qsort callback: servers with our version first, then by ping (0 counts as 9998,
// negative as its absolute value, minimum 10).
int __cdecl wrUI_JOIN_WINDOW::CompareServers(const void* a, const void* b)
{
    gsMP_SERVER_INFO ia, ib;
    gsSysNetwork->Browser_GetServerInfo(*(const int*)a, &ia);
    gsSysNetwork->Browser_GetServerInfo(*(const int*)b, &ib);

    if (ia.versionId == 10020000) {
        if (ib.versionId != 10020000)
            return -1;
    } else if (ib.versionId == 10020000) {
        return 1;
    }

    // (the binary fetches both infos a second time here)
    int pa = ia.ping, pb = ib.ping;
    pa = (pa == 0) ? 9998 : (pa < 0 ? -pa : pa);  if (pa < 10) pa = 10;
    pb = (pb == 0) ? 9998 : (pb < 0 ? -pb : pb);  if (pb < 10) pb = 10;
    if (pa < pb)
        return -1;
    return pa > pb;
}

// v12 0x00499050  (feb 0x0048D3B0)
// Per frame: report browser errors; when the driver says the list changed, rebuild
// it (max 64), sort it, keep the selection on the same server, rebind the rows;
// otherwise count down the refresh timer.  Then the scroll-window update (feb called
// wrUI_WINDOW_SCROLL<>::ProcessUpdate 0x0048DA20; 1.2 inlines it).
// 1.2: sorting, 64-entry cap, selection kept by server id after sorting.
int wrUI_JOIN_WINDOW::ProcessUpdate(void* data)
{
    if ((flags & 0x140) == 0) {
        if (gsSysNetwork->Browser_IsError()) {                    // 0x0052F510
            int id = gsStrings->GetStringIdFmt("UI_NET_STATUS_%s", gsSysNetwork->Browser_GetStatus());
            const ushort* detail = id ? gsStrings->GetStringById(id)
                                      : gsStrings->AsciiToUnicode(gsSysNetwork->Browser_GetStatus());
            uiSystem->MessageBox(gsStrings->GetStringById(gsStrings->GetStringId("UI_ERR_SERVER_LIST_ERROR")),
                                 detail, 0, 0, 0, "connection");
        }
        else if (!gsSysNetwork->Browser_IsStarted()) {            // 0x0052F4E0
            uiSystem->MessageBox(gsStrings->GetStringById(gsStrings->GetStringId("UI_ERR_SERVER_LIST_ERROR")),
                                 gsStrings->GetStringById(gsStrings->GetStringId("UI_ERR_SERVER_LIST_DISCONNECTED")),
                                 0, 0, 0, "connection");
        }
        else if (gsSysNetwork->Browser_IsServerListUpdated()) {   // 0x0052F600
            int selId = (selRec >= 0 && selRec < nRecords) ? records[selRec].serverId : -1;

            int n = 0;
            for (int id = gsSysNetwork->Browser_GetNextServer(-1);   // 0x0052F650
                 id != -1 && n < 64;                              // 1.2: feb allowed index 64 (65 entries)
                 id = gsSysNetwork->Browser_GetNextServer(id))
                records[n++].serverId = id;

            qsort(records, n, sizeof(wrUI_JOIN_RECORD), CompareServers);   // 1.2

            if (n != nRecords) {
                nRecords = n;
                int range = n - nRowWidgets;
                scrollBar->SetWindow(this, range < 0 ? 0 : range, 1);
            }

            // keep the selection on the same server (1.2: after sorting)
            int i;
            for (i = 0; i < nRecords; i++)
                if (records[i].serverId == selId) {
                    selRec = (nRecords != 0 && i != -1) ? (i < nRecords ? i : nRecords - 1) : -1;
                    break;
                }
            if (nRecords != 0 && i == nRecords)                   // not found: select the first row
                selRec = 0;                                       // (as compiled: nRecords > 0 ? 0 : nRecords - 1)

            BindRows(this);
            SetSelectedRec(selRec, 1);
            if (selRec != -1 &&
                ((selRec < topRow && topRow > 0) ||
                 (selRec >= topRow + nRowWidgets && topRow < nRecords - nRowWidgets))) {
                scrollSpeed  = 0.02f;                             // 0x3CA3D70A
                scrollFlags |= 1;
            }
            HighlightRows(this);
        }
        else if (m3dUpdateTimeField(&refreshTimer, gsElapsedTime)) {   // timer ran out
            gsSysNetwork->Browser_RefreshServers();
            refreshTimer = gsSysNetwork->Drv_GetRefreshInterval();
        }
    }

    // --- wrUI_WINDOW_SCROLL<wrUI_JOIN_RECORD, wrUI_JOIN_INFO>::ProcessUpdate, inlined ---
    if (needScroll) {                                             // +0x208
        if (selRec == -1 && nRecords != 0)
            SetSelectedRec(topRow, 1);
        if (selRec != -1) {
            if (selRec < topRow) {
                ScrollTo(selRec);                                 // 0x004998B0
                if (scrollBar) scrollBar->SetPos(topRow);
            } else if (selRec >= topRow + nRowWidgets) {
                ScrollTo(selRec - nRowWidgets);
                if (scrollBar) scrollBar->SetPos(topRow);
            }
            SetFocusToSelected();                                 // 0x0048FDE0
        }
        needScroll = 0;
    } else {
        ProcessScroll();                                          // 0x0048FE70
    }
    return uiELEMENT::ProcessUpdate(data);
}

// v12 0x004998B0  (template code; feb inlined it)
// Sets the first visible row (clamped), rebinds rows, clamps the selection and
// moves the scroll bar.  uiSystem+0x128 is a "batch update" counter  // ?
void wrUI_JOIN_WINDOW::ScrollTo(int top)
{
    if (top > nRecords - nRowWidgets) top = nRecords - nRowWidgets;
    if (top < 0) top = 0;
    if (topRow == top)
        return;
    topRow = top;
    uiSystem->updateLock++;                                       // +0x128
    BindRows(this);
    selRec = (nRecords != 0 && selRec != -1) ? (selRec >= nRecords ? nRecords - 1 : (selRec < 0 ? 0 : selRec)) : -1;
    uiSystem->updateLock--;
    uiSystem->Slot88(1);                                          // vt+0x88 (feb +0x84)  // ? unidentified
    if (scrollBar)
        scrollBar->SetPos(topRow);
}

// ===========================================================================
// wrUI_WAITBROWSER_WND  ("mp_wait_browser")        -- unchanged in 1.2
// ===========================================================================

// v12 0x00499590  (feb 0x0048D700)
void wrUI_WAITBROWSER_WND::OnChildNotify(uiCHILD_NOTIFY* n)
{
    if (n->type == UINOTIFY_BUTTON && n->code == UIBTN_BACK) {
        gsSysNetwork->Browser_Stop(0);
        uiSystem->ShowWindow("connection", 0);
        return;
    }
    uiELEMENT::OnChildNotify(n);
}

// v12 0x004995E0  (feb 0x0048D750)
int wrUI_WAITBROWSER_WND::StartAppearing(int start)
{
    if (start)
        SetStatusStr(gsSysNetwork->Browser_GetStatus(), 0.0f);    // 0x0052F550
    return uiELEMENT::StartAppearing(start);
}

// v12 0x00499610  (feb 0x0048D780)
// Browser up -> "join"; error -> UI_ERR_CANT_START_CLIENT box (then "connection").
int wrUI_WAITBROWSER_WND::ProcessUpdate(void* data)
{
    if ((flags & 0x140) == 0) {
        if (gsSysNetwork->Browser_IsStarted()) {
            uiSystem->ShowWindow("join", 0);
            return uiELEMENT::ProcessUpdate(data);
        }
        if (gsSysNetwork->Browser_IsError()) {
            int id = gsStrings->GetStringIdFmt("UI_NET_STATUS_%s", gsSysNetwork->Browser_GetStatus());
            const ushort* detail = id ? gsStrings->GetStringById(id)
                                      : gsStrings->AsciiToUnicode(gsSysNetwork->Browser_GetStatus());
            uiSystem->MessageBox(gsStrings->GetStringById(gsStrings->GetStringId("UI_ERR_CANT_START_CLIENT")),
                                 detail, 0, 0, 0, "connection");
            return uiELEMENT::ProcessUpdate(data);
        }
    }
    SetStatusStr(gsSysNetwork->Browser_GetStatus(), 0.0f);
    return uiELEMENT::ProcessUpdate(data);
}

// ===========================================================================
// wrUI_WAITJOIN_WND  ("mp_wait_join")              -- unchanged in 1.2
// ===========================================================================

// v12 0x00499720  (feb 0x0048D890)
void wrUI_WAITJOIN_WND::OnChildNotify(uiCHILD_NOTIFY* n)
{
    if (n->type == UINOTIFY_BUTTON && n->code == UIBTN_BACK) {
        gsSysNetwork->Client_DisconnectFromGame();                // 0x0052F8C0
        uiSystem->ShowWindow("connection", 0);
        return;
    }
    uiELEMENT::OnChildNotify(n);
}

// v12 0x00499760  (feb 0x0048D8D0)
int wrUI_WAITJOIN_WND::StartAppearing(int start)
{
    if (start)
        SetStatusStr(gsSysNetwork->Client_GetStatus(), 0.0f);     // 0x0052F870
    return uiELEMENT::StartAppearing(start);
}

// v12 0x00499790  (feb 0x0048D900)
// Connected -> start loading as client (the server sends the level);
// error -> UI_ERR_JOIN_GAME_ERROR box (then "connection").
int wrUI_WAITJOIN_WND::ProcessUpdate(void* data)
{
    if ((flags & 0x140) == 0) {
        if (gsSysNetwork->Client_IsJoined()) {                    // 0x0052F800
            ((wrUI_SYSTEM*)uiSystem)->StartLoadingAction(1, 3, "",
                    gsStrings->GetStringId("UI_ERR_CANT_START_GAME"));
            return uiELEMENT::ProcessUpdate(data);
        }
        if (gsSysNetwork->Client_IsError()) {                     // 0x0052F830
            int id = gsStrings->GetStringIdFmt("UI_NET_STATUS_%s", gsSysNetwork->Client_GetStatus());
            const ushort* detail = id ? gsStrings->GetStringById(id)
                                      : gsStrings->AsciiToUnicode(gsSysNetwork->Client_GetStatus());
            uiSystem->MessageBox(gsStrings->GetStringById(gsStrings->GetStringId("UI_ERR_JOIN_GAME_ERROR")),
                                 detail, 0, 0, 0, "connection");
            return uiELEMENT::ProcessUpdate(data);
        }
    }
    SetStatusStr(gsSysNetwork->Client_GetStatus(), 0.0f);
    return uiELEMENT::ProcessUpdate(data);
}

// _$E2 of wr_ui_join.obj (v12 0x00498BF0, feb 0x0048D390): g_float_0x64F8DC = c*c, header constant, unused here.

// v12 0x00495A90: qsort order of the render elements, by their left edge (+0x4C)
int __cdecl _compareRectX(const void *a, const void *b)
{
    float xa = *(const float *)((const u8 *)a + 0x4C), xb = *(const float *)((const u8 *)b + 0x4C);
    if (xa < xb)
        return -1;
    if (xa == xb)
        return 0;
    return 1;
}
