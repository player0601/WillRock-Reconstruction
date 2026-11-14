// ap_wind.cpp -- ap.lib Win32 helpers (Will Rock 1.2, v12.exe; feb obj ap:ap_wind.obj)
//
// v12 0x004B0020..0x004B0623, identical to feb. Mostly helpers for the in-house tool dialogs
// (common open-file dialog, check boxes, float edit boxes, tab and list-view controls); the game itself
// only uses the mbShow* message boxes. Hook point: mbShow* (all modal boxes owned by apHMainWindow,
// which matters in fullscreen).

#include "ap.h"

char          wifFileName[0x100];        // v12 0x00655980 (feb 0x006117A0)
char          wifFileTitle[0x400];       // v12 0x00655A80 (feb 0x006118A0)
OPENFILENAMEA wifOfn;                    // v12 0x00656010 (feb 0x00611E30)

// v12 0x004B0020  (feb 0x004A1490)
// GetOpenFileName wrapper. Returns the chosen path (static buffer) or NULL if cancelled.
// Real meaning of the arguments (SDK names in app.h differ, see recon_fixes/03_ap.md):
//   owner (NULL = main window), initial dir, filter, default extension, dialog title, initial file name, OFN_* flags
char *wifOpenFile(HWND__ *owner, char *initDir, char *filter, char *defExt, char *title, char *file, unsigned flags)
{
    if (file == NULL)
        file = "";
    strcpy(wifFileName, file);
    strcpy(wifFileTitle, "");

    wifOfn.lStructSize       = sizeof(OPENFILENAMEA);   // 0x4C (pre-Win2000 layout)
    if (owner == NULL)
        owner = apHMainWindow;
    wifOfn.hwndOwner         = owner;
    wifOfn.hInstance         = (HINSTANCE)apHInstance;
    wifOfn.lpstrFilter       = filter;
    wifOfn.lpstrCustomFilter = NULL;
    wifOfn.nMaxCustFilter    = 0;
    wifOfn.nFilterIndex      = 1;
    wifOfn.lpstrFile         = wifFileName;
    wifOfn.nMaxFile          = 0x100;
    wifOfn.lpstrFileTitle    = wifFileTitle;
    wifOfn.nMaxFileTitle     = 0x400;
    wifOfn.lpstrInitialDir   = initDir;
    wifOfn.lpstrTitle        = title;
    wifOfn.Flags             = flags;
    wifOfn.nFileOffset       = 0;
    wifOfn.nFileExtension    = 0;
    wifOfn.lpstrDefExt       = defExt;
    wifOfn.lCustData         = 0;
    wifOfn.lpfnHook          = NULL;
    wifOfn.lpTemplateName    = NULL;

    ShowCursor(TRUE);                    // the game hides the cursor; show it for the dialog
    if (!GetOpenFileNameA(&wifOfn)) {
        ShowCursor(FALSE);
        return NULL;
    }
    ShowCursor(FALSE);
    return wifOfn.lpstrFile;
}

// v12 0x004B0160  (feb 0x004A15B0)
// printf-style information box (MB_ICONINFORMATION).
int mbShowOk(char *fmt, ...)
{
    char buf[0x100];
    va_list va; va_start(va, fmt);
    vsprintf(buf, fmt, va);
    va_end(va);
    return MessageBoxA(apHMainWindow, buf, apTitle, MB_ICONINFORMATION);
}

// v12 0x004B01A0  (feb 0x004A15F0)
// printf-style error box (MB_ICONSTOP).
int mbShowStop(char *fmt, ...)
{
    char buf[0x100];
    va_list va; va_start(va, fmt);
    vsprintf(buf, fmt, va);
    va_end(va);
    return MessageBoxA(apHMainWindow, buf, apTitle, MB_ICONSTOP);
}

// v12 0x004B01E0  (feb 0x004A1630)
// printf-style question box (MB_YESNO|MB_ICONQUESTION); returns IDYES/IDNO.
int mbShowYesNo(char *fmt, ...)
{
    char buf[0x100];
    va_list va; va_start(va, fmt);
    vsprintf(buf, fmt, va);
    va_end(va);
    return MessageBoxA(apHMainWindow, buf, apTitle, MB_YESNO | MB_ICONQUESTION);
}

// v12 0x004B0220  (feb 0x004A1670)
// Check/uncheck a dialog check box.
void SetCBItemState(HWND dlg, UINT id, int on)
{
    SendDlgItemMessageA(dlg, id, BM_SETCHECK, on != 0, 0);
}

// v12 0x004B0250  (feb 0x004A16A0)
// Is a dialog check box checked.
int GetCBItemState(HWND dlg, UINT id)
{
    return SendDlgItemMessageA(dlg, id, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

// v12 0x004B0270  (feb 0x004A16C0)
// Set an edit control to a float ("%f").
void SetDlgItemFloat(HWND dlg, UINT id, float v)
{
    char buf[0x100];
    sprintf(buf, "%f", (double)v);
    SetDlgItemTextA(dlg, id, buf);
}

// v12 0x004B02C0  (feb 0x004A1710)
// Read a float from an edit control (first 32 chars).
float GetDlgItemFloat(HWND dlg, UINT id)
{
    char buf[0x100];
    GetDlgItemTextA(dlg, id, buf, 0x20);
    return (float)atof(buf);
}

// v12 0x004B02F0  (feb 0x004A1740)
// Insert a text tab at index idx into the tab control dlg/id.
void __fastcall TabControl_AddPage(HWND dlg, int id, int idx, char *text)
{
    TCITEMA ti;
    memset(&ti, 0, sizeof(ti));
    ti.mask       = TCIF_TEXT;
    ti.pszText    = text;
    ti.cchTextMax = strlen(text);
    SendMessageA(GetDlgItem(dlg, id), TCM_INSERTITEMA, idx, (LPARAM)&ti);
}

// v12 0x004B0350  (feb 0x004A17A0)
// Insert a list-view column. col -1 = append (index from the header item count); width < 0 = percent of
// the client width; image != -1 adds an image to the header.
int __fastcall _ListView_AddColumn(HWND lv, int col, char *text, int width, int image)
{
    LVCOLUMNA c;
    memset(&c, 0, sizeof(c));
    c.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    c.fmt      = LVCFMT_LEFT;
    c.iSubItem = col;
    if (col == -1)
        c.iSubItem = SendMessageA((HWND)SendMessageA(lv, LVM_GETHEADER, 0, 0), HDM_GETITEMCOUNT, 0, 0);
    c.pszText = text;
    if (width < 0) {
        RECT rc;
        GetClientRect(lv, &rc);
        c.cx = rc.right * width / -100;
    } else {
        c.cx = width;
    }
    c.fmt |= LVCFMT_BITMAP_ON_RIGHT;
    if (image != -1) {
        c.iImage = image;
        c.mask  |= LVCF_IMAGE;
        c.fmt   |= LVCFMT_BITMAP_ON_RIGHT | LVCFMT_COL_HAS_IMAGES | LVCFMT_IMAGE;   // 0x9800
    }
    // ? the message still uses "col" (-1 when appending) as wParam, as in the original
    return SendMessageA(lv, LVM_INSERTCOLUMNA, col, (LPARAM)&c);
}

// v12 0x004B0420  (feb 0x004A1870)
// Screen position for a context menu: below the label of the first selected item, else the window corner.
void __fastcall _ListView_GetMenuPos(HWND lv, POINT *pt)
{
    int n = SendMessageA(lv, LVM_GETITEMCOUNT, 0, 0);
    int i;
    for (i = 0; i < n; i++)
        if (SendMessageA(lv, LVM_GETITEMSTATE, i, LVIS_SELECTED))
            break;
    RECT rc;
    if (i != n) {
        rc.left = LVIR_LABEL;
        if (SendMessageA(lv, LVM_GETITEMRECT, i, (LPARAM)&rc)) {
            MapWindowPoints(lv, NULL, (POINT *)&rc, 2);
            pt->x = rc.left;
            pt->y = rc.bottom;
            return;
        }
    }
    GetWindowRect(lv, &rc);
    pt->x = rc.left;
    pt->y = rc.top;
}

// v12 0x004B04D0  (feb 0x004A1920)
// Insert a list-view item (item -1 = append) with image and user data; text optional.
DWORD __fastcall _ListView_AddItem(HWND lv, int item, int sub, char *text, int image, DWORD data)
{
    LVITEMA it;
    memset(&it, 0, 10 * 4);              // only the first 10 dwords (pre-comctl 4.70 size)
    it.iItem = item;
    if (item == -1)
        it.iItem = SendMessageA(lv, LVM_GETITEMCOUNT, 0, 0);
    it.mask     = LVIF_IMAGE | LVIF_PARAM | LVIF_NORECOMPUTE;   // 0x806
    it.iSubItem = sub;
    if (text) {
        it.mask   |= LVIF_TEXT;
        it.pszText = text;
    }
    it.iImage = image;
    it.lParam = data;
    return SendMessageA(lv, LVM_INSERTITEMA, 0, (LPARAM)&it);
}

// v12 0x004B0550  (feb 0x004A19A0)
// Index of the first selected item, -1 if none.
int __fastcall _ListView_GetSelected(HWND lv)
{
    int n = SendMessageA(lv, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < n; i++)
        if (SendMessageA(lv, LVM_GETITEMSTATE, i, LVIS_SELECTED))
            return i;
    return -1;
}

// v12 0x004B05A0  (feb 0x004A19F0)
// lParam of the FOCUSED item (not the selected one); -1 if LVM_GETITEM fails, 0 if no item has focus.
DWORD __fastcall _ListView_GetSelectedData(HWND lv)
{
    int n = SendMessageA(lv, LVM_GETITEMCOUNT, 0, 0);
    for (int i = 0; i < n; i++) {
        if (SendMessageA(lv, LVM_GETITEMSTATE, i, LVIS_FOCUSED)) {
            LVITEMA it;
            memset(&it, 0, 10 * 4);
            it.mask  = LVIF_PARAM;
            it.iItem = i;
            if (SendMessageA(lv, LVM_GETITEMA, 0, (LPARAM)&it))
                return it.lParam;
            return (DWORD)-1;
        }
    }
    return 0;
}
