// plat_d3d7.cpp -- Win32 window, DirectDraw 7 / Direct3D 7 renderer and DirectInput 7 input for the box test.
#define DIRECTINPUT_VERSION 0x0700
#define INITGUID
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include <dinput.h>
#include <stdio.h>
#include <string.h>
#include "plat_d3d7.h"

static HWND                 s_hwnd;
static int                  s_w, s_h, s_quit, s_active = 1;
static LPDIRECTDRAW7        s_dd;
static LPDIRECTDRAWSURFACE7 s_primary, s_back, s_zbuf;
static LPDIRECT3D7          s_d3d;
static LPDIRECT3DDEVICE7    s_dev;
static LPDIRECTINPUT7       s_di;
static LPDIRECTINPUTDEVICE7 s_kbd, s_mouse;
static const char          *s_devName = "none";
static int                  s_mouseBtn;
static int                  s_mouseSkip = 2;        // reads to drop after (re)acquiring: they carry old movement
static LARGE_INTEGER        s_freq, s_t0;
static int                  s_full;                 // the display mode is ours (exclusive full screen)

// [Video] VIDEO_FullScr / VIDEO_BPP / VIDEO_TripleBuf, set before platInit (the game's video config, d3dDRIVER +0x210)
int platFullScreen, platBPP = 32, platTripleBuf, platRefresh;

static LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    switch (m) {
    case WM_CLOSE:
        s_quit = 1;
        return 0;
    case WM_ACTIVATEAPP:
        s_active = (int)w;
        if (s_active) {
            if (s_kbd) s_kbd->Acquire();
            if (s_mouse) s_mouse->Acquire();
            s_mouseSkip = 2;
        }
        return 0;
    }
    return DefWindowProcA(h, m, w, l);
}

static HRESULT WINAPI EnumZCB(LPDDPIXELFORMAT pf, LPVOID ctx)
{
    DDPIXELFORMAT *out = (DDPIXELFORMAT *)ctx;
    if ((pf->dwFlags & DDPF_ZBUFFER) && (pf->dwZBufferBitDepth == 16 || pf->dwZBufferBitDepth == 24)) {
        *out = *pf;
        if (pf->dwZBufferBitDepth == 24)
            return D3DENUMRET_CANCEL;
    }
    return D3DENUMRET_OK;
}

#ifdef WR_WIDESCREEN
// (test, box_test_widescreen.exe) JackFuste's "widescreen patch", the same one byte, done in memory: his D3DIM700.DLL
// is Windows 7's d3dim700.dll 6.1.7600.16385 with file offset 0x1CEB6 (RVA 0x1DAB6) 08 -> 80. That byte is in
//     mov eax, 0x800 / cmp [ebp-0x94], eax / ja fail / cmp [ebp-0x98], eax / ja fail
// the check that the render target is at most 2048 x 2048 when the Direct3D device is made; patched it is
// 0x8000 (32768), so modes wider than 2048 (ultrawide, 4K, several monitors) get a device. Nothing else (field
// of view, HUD) is changed by it. The same instructions are looked for in whichever d3dim700.dll is loaded.
// Under dgVoodoo (its DDraw.dll next to the exe) d3dim700.dll is never loaded and there is nothing to patch.
static void wsPatchD3DIM()
{
    static int done;
    if (done)
        return;
    done = 1;
    HMODULE m = GetModuleHandleA("d3dim700.dll");
    if (!m) {
        printf("widescreen: d3dim700.dll not loaded (dgVoodoo or another wrapper does Direct3D 7), nothing to patch\n");
        return;
    }
    unsigned char *base = (unsigned char *)m;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, sec++) {
        if (!(sec->Characteristics & IMAGE_SCN_MEM_EXECUTE))
            continue;
        unsigned char *p = base + sec->VirtualAddress, *e = p + sec->Misc.VirtualSize - 30;
        for (; p < e; p++) {
            // B8 00 08 00 00 | 39 85 xx xx xx xx | 0F 87 xx xx xx xx | 39 85 xx xx xx xx | 0F 87
            if (p[0] != 0xB8 || p[1] != 0 || p[3] != 0 || p[4] != 0 || p[5] != 0x39 || p[6] != 0x85 ||
                p[11] != 0x0F || p[12] != 0x87 || p[17] != 0x39 || p[18] != 0x85 || p[23] != 0x0F || p[24] != 0x87)
                continue;
            if (p[2] == 0x80) {
                printf("widescreen: d3dim700.dll is patched already (+0x%lX)\n", (unsigned long)(p + 2 - base));
                return;
            }
            if (p[2] != 0x08)
                continue;
            DWORD old;
            if (VirtualProtect(p + 2, 1, PAGE_EXECUTE_READWRITE, &old)) {
                p[2] = 0x80;                            // 0x800 -> 0x8000
                VirtualProtect(p + 2, 1, old, &old);
                FlushInstructionCache(GetCurrentProcess(), p + 2, 1);
                printf("widescreen: d3dim700.dll +0x%lX 08 -> 80 (render target limit 2048 -> 32768)%s\n",
                       (unsigned long)(p + 2 - base), (p + 2 - base) == 0x1DAB6 ? ", JackFuste's byte" : "");
            }
            return;
        }
    }
    printf("widescreen: the 2048 check was not found in this d3dim700.dll\n");
}
#endif

static int fail(const char *what, HRESULT hr)
{
    char b[256];
    sprintf(b, "%s failed (0x%08lX)", what, (unsigned long)hr);
    fprintf(stderr, "%s\n", b);
    if (!s_full)                                        // full screen: logged only, the windowed fallback follows
        MessageBoxA(s_hwnd, b, "box test", MB_OK | MB_ICONERROR);
    return 0;
}

// the surfaces and the device for the current mode (s_full): full screen as d3dDRIVER::InitHW, else windowed
static int createDisplay(int w, int h)
{
    HRESULT hr;
    DDSURFACEDESC2 sd;
    if (s_full) {
        // d3dDRIVER::InitHW (v12 D3d_drv.cpp, feb 0x004AB730), full screen: exclusive cooperative level
        // 0x1011, the configured mode, a flippable primary with 1 back buffer (2 with VIDEO_TripleBuf)
        printf("Initialising rendering to full screen %d x %d x %d\n", w, h, platBPP);
        SetForegroundWindow(s_hwnd);                    // (test) exclusive mode wants the window in front
        hr = s_dd->SetCooperativeLevel(s_hwnd, DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_FPUPRESERVE);
        if (FAILED(hr)) return fail("SetCooperativeLevel(full screen)", hr);
        printf("Setting display mode %d x %d x %d\n", w, h, platBPP);
        hr = s_dd->SetDisplayMode(w, h, platBPP, platRefresh, 0);
        if (FAILED(hr)) return fail("SetDisplayMode", hr);
        printf("Creating a flippable primary surface\n");
        memset(&sd, 0, sizeof(sd));
        sd.dwSize = sizeof(sd);
        sd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_FLIP | DDSCAPS_COMPLEX | DDSCAPS_3DDEVICE;
        sd.dwBackBufferCount = platTripleBuf ? 2 : 1;
        hr = s_dd->CreateSurface(&sd, &s_primary, 0);
        if (FAILED(hr)) return fail("CreateSurface(flippable primary)", hr);
        DDSCAPS2 caps;
        memset(&caps, 0, sizeof(caps));
        caps.dwCaps = DDSCAPS_BACKBUFFER;
        hr = s_primary->GetAttachedSurface(&caps, &s_back);
        if (FAILED(hr)) return fail("GetAttachedSurface(back buffer)", hr);
        ShowCursor(FALSE);
    } else {
        hr = s_dd->SetCooperativeLevel(s_hwnd, DDSCL_NORMAL | DDSCL_FPUPRESERVE);   // InitHW windowed: 0x1008
        if (FAILED(hr)) return fail("SetCooperativeLevel", hr);
        memset(&sd, 0, sizeof(sd));
        sd.dwSize = sizeof(sd);
        sd.dwFlags = DDSD_CAPS;
        sd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        hr = s_dd->CreateSurface(&sd, &s_primary, 0);
        if (FAILED(hr)) return fail("CreateSurface(primary)", hr);
        LPDIRECTDRAWCLIPPER clip;
        if (SUCCEEDED(s_dd->CreateClipper(0, &clip, 0))) {
            clip->SetHWnd(0, s_hwnd);
            s_primary->SetClipper(clip);
            clip->Release();
        }
        memset(&sd, 0, sizeof(sd));
        sd.dwSize = sizeof(sd);
        sd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        sd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE | DDSCAPS_VIDEOMEMORY;
        sd.dwWidth = w;
        sd.dwHeight = h;
        hr = s_dd->CreateSurface(&sd, &s_back, 0);
        if (FAILED(hr)) return fail("CreateSurface(back)", hr);
    }
    if (!s_d3d) {
        hr = s_dd->QueryInterface(IID_IDirect3D7, (void **)&s_d3d);
        if (FAILED(hr)) return fail("QueryInterface(IDirect3D7)", hr);
    }
#ifdef WR_WIDESCREEN
    wsPatchD3DIM();                                     // (d3dim700.dll is loaded by now; the device is made below)
#endif
    DDPIXELFORMAT zpf;
    memset(&zpf, 0, sizeof(zpf));
    s_d3d->EnumZBufferFormats(IID_IDirect3DHALDevice, EnumZCB, &zpf);
    if (zpf.dwSize) {
        memset(&sd, 0, sizeof(sd));
        sd.dwSize = sizeof(sd);
        sd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
        sd.ddsCaps.dwCaps = DDSCAPS_ZBUFFER | DDSCAPS_VIDEOMEMORY;
        sd.dwWidth = w;
        sd.dwHeight = h;
        sd.ddpfPixelFormat = zpf;
        if (SUCCEEDED(s_dd->CreateSurface(&sd, &s_zbuf, 0)))
            s_back->AddAttachedSurface(s_zbuf);
    }
    hr = s_d3d->CreateDevice(IID_IDirect3DTnLHalDevice, s_back, &s_dev);
    s_devName = "Direct3D 7 T&L HAL";
    if (FAILED(hr)) {
        hr = s_d3d->CreateDevice(IID_IDirect3DHALDevice, s_back, &s_dev);
        s_devName = "Direct3D 7 HAL";
    }
    if (FAILED(hr)) return fail("CreateDevice", hr);
    D3DVIEWPORT7 vp = { 0, 0, (DWORD)w, (DWORD)h, 0.0f, 1.0f };
    s_dev->SetViewport(&vp);
    return 1;
}

static void releaseDisplay()
{
    if (s_dev) { s_dev->Release(); s_dev = 0; }
    if (s_zbuf) { s_zbuf->Release(); s_zbuf = 0; }
    if (s_back && !s_full) s_back->Release();
    s_back = 0;
    if (s_primary) { s_primary->Release(); s_primary = 0; }
}

static void windowStyle(int w, int h)
{
    if (s_full) {                                       // gsWINDOWS: 0x880000 at 0,0 640x480
        SetWindowLongA(s_hwnd, GWL_STYLE, 0x880000);
        SetWindowPos(s_hwnd, HWND_TOP, 0, 0, 640, 480, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        return;
    }
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT r = { 0, 0, w, h };
    AdjustWindowRect(&r, style, FALSE);
    SetWindowLongA(s_hwnd, GWL_STYLE, style);
    SetWindowPos(s_hwnd, HWND_NOTOPMOST, 40, 40, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
}

// TEMPORARY (test): F11 -- switch between full screen and a window at w x h, keeping the DirectDraw object (the
// textures are managed, they survive the mode change). Returns 0 if the new mode could not be set up.
// (test) full screen refused (e.g. SetDisplayMode 0x80004001): back to the windowed renderer at 1024 x 768
static int fallBackWindowed()
{
    releaseDisplay();
    s_dd->RestoreDisplayMode();
    s_dd->SetCooperativeLevel(s_hwnd, DDSCL_NORMAL);
    ShowCursor(TRUE);
    s_full = 0;
    s_w = 1024;
    s_h = 768;
    windowStyle(s_w, s_h);
    printf("full screen failed: using the windowed renderer %d x %d\n", s_w, s_h);
    return createDisplay(s_w, s_h);
}

int platToggleFullScreen(int w, int h)
{
    releaseDisplay();
    if (s_full) {
        s_dd->RestoreDisplayMode();
        s_dd->SetCooperativeLevel(s_hwnd, DDSCL_NORMAL);
        ShowCursor(TRUE);
    }
    s_full = !s_full;
    s_w = w;
    s_h = h;
    windowStyle(w, h);
    int ok = createDisplay(w, h);
    if (!ok && s_full)
        ok = fallBackWindowed() ? 2 : 0;    // 2: asked for full screen, got a window
    s_mouseSkip = 2;
    return ok;
}

int platInit(int w, int h, const char *title, int hidden)
{
    HINSTANCE inst = GetModuleHandleA(0);
    s_w = w;
    s_h = h;
    QueryPerformanceFrequency(&s_freq);
    QueryPerformanceCounter(&s_t0);
    WNDCLASSA wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WndProc;
    wc.hInstance = inst;
    wc.hCursor = LoadCursor(0, IDC_ARROW);
    wc.lpszClassName = "WRBoxTest";
    s_full = platFullScreen && !hidden;
    if (s_full) {
        // as gsWINDOWS (gs_windows.cpp, feb 0x0053F???): WS_SYSMENU | WS_BORDER (0x880000) at 0,0 640x480, black
        // background; the exclusive display mode then covers the screen
        wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RegisterClassA(&wc);
        s_hwnd = CreateWindowExA(0, "WRBoxTest", title, 0x880000, 0, 0, 640, 480, 0, 0, inst, 0);
    } else {
        RegisterClassA(&wc);
        RECT r = { 0, 0, w, h };
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        AdjustWindowRect(&r, style, FALSE);
        s_hwnd = CreateWindowA("WRBoxTest", title, style, CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left,
                               r.bottom - r.top, 0, 0, inst, 0);
    }
    if (!s_hwnd)
        return 0;
    ShowWindow(s_hwnd, hidden ? SW_SHOWMINNOACTIVE : SW_SHOW);
    UpdateWindow(s_hwnd);

    HRESULT hr = DirectDrawCreateEx(0, (void **)&s_dd, IID_IDirectDraw7, 0);
    if (FAILED(hr)) return fail("DirectDrawCreateEx", hr);
    if (!createDisplay(w, h) && !(s_full && fallBackWindowed()))
        return 0;

    hr = DirectInputCreateEx(inst, DIRECTINPUT_VERSION, IID_IDirectInput7, (void **)&s_di, 0);
    if (FAILED(hr)) return fail("DirectInputCreateEx", hr);
    if (SUCCEEDED(s_di->CreateDeviceEx(GUID_SysKeyboard, IID_IDirectInputDevice7, (void **)&s_kbd, 0))) {
        s_kbd->SetDataFormat(&c_dfDIKeyboard);
        s_kbd->SetCooperativeLevel(s_hwnd, DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
        s_kbd->Acquire();
    }
    if (SUCCEEDED(s_di->CreateDeviceEx(GUID_SysMouse, IID_IDirectInputDevice7, (void **)&s_mouse, 0))) {
        s_mouse->SetDataFormat(&c_dfDIMouse);
        // exclusive + foreground as the game (coop 0x15 with NOWINKEY): the cursor is hidden while active
        s_mouse->SetCooperativeLevel(s_hwnd, DISCL_EXCLUSIVE | DISCL_FOREGROUND);
        s_mouse->Acquire();
    }
    return 1;
}

void platTerm()
{
    if (s_mouse) { s_mouse->Unacquire(); s_mouse->Release(); }
    if (s_kbd) { s_kbd->Unacquire(); s_kbd->Release(); }
    if (s_di) s_di->Release();
    releaseDisplay();
    if (s_d3d) s_d3d->Release();
    if (s_dd) {
        if (s_full) {                                   // d3dDRIVER::TermHW: back to the desktop mode
            s_dd->RestoreDisplayMode();
            s_dd->SetCooperativeLevel(s_hwnd, DDSCL_NORMAL);
            ShowCursor(TRUE);
        }
        s_dd->Release();
    }
    if (s_hwnd) DestroyWindow(s_hwnd);
}

int platPump()
{
    MSG m;
    while (PeekMessageA(&m, 0, 0, 0, PM_REMOVE)) {
        TranslateMessage(&m);
        DispatchMessageA(&m);
    }
    return !s_quit;
}

double platTime()
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return (double)(t.QuadPart - s_t0.QuadPart) / (double)s_freq.QuadPart;
}

void platSetTitle(const char *s)
{
    SetWindowTextA(s_hwnd, s);
}

void platInput(unsigned char keys[256], int *mdx, int *mdy)
{
    memset(keys, 0, 256);
    *mdx = *mdy = 0;
    if (!s_active)
        return;
    if (s_kbd && s_kbd->GetDeviceState(256, keys) == DIERR_INPUTLOST) {
        s_kbd->Acquire();
        s_kbd->GetDeviceState(256, keys);
    }
    DIMOUSESTATE ms;
    if (s_mouse) {
        HRESULT hr = s_mouse->GetDeviceState(sizeof(ms), &ms);
        if (hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED) {
            s_mouse->Acquire();
            s_mouseSkip = 2;
            hr = s_mouse->GetDeviceState(sizeof(ms), &ms);
        }
        if (SUCCEEDED(hr)) {
            s_mouseBtn = (ms.rgbButtons[0] & 0x80 ? 1 : 0) | (ms.rgbButtons[1] & 0x80 ? 2 : 0);
            if (s_mouseSkip > 0) {
                s_mouseSkip--;
                return;
            }
            // one frame of more than 400 counts is a stall or a re-acquire, not a hand movement
            if (ms.lX > -400 && ms.lX < 400 && ms.lY > -400 && ms.lY < 400) {
                *mdx = ms.lX;
                *mdy = ms.lY;
            }
        }
    }
}

static int s_cull = -1;                             // the device's cull mode (rndSetCull)

void rndBegin(const float view[16], const float proj[16], unsigned clear)
{
    if (s_primary->IsLost() == DDERR_SURFACELOST)
        s_dd->RestoreAllSurfaces();
    s_dev->Clear(0, 0, D3DCLEAR_TARGET | (s_zbuf ? D3DCLEAR_ZBUFFER : 0), clear, 1.0f, 0);
    s_dev->BeginScene();
    D3DMATRIX I;
    memset(&I, 0, sizeof(I));
    I._11 = I._22 = I._33 = I._44 = 1.0f;
    s_dev->SetTransform(D3DTRANSFORMSTATE_WORLD, &I);
    s_dev->SetTransform(D3DTRANSFORMSTATE_VIEW, (D3DMATRIX *)view);
    s_dev->SetTransform(D3DTRANSFORMSTATE_PROJECTION, (D3DMATRIX *)proj);
    s_dev->SetRenderState(D3DRENDERSTATE_LIGHTING, FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_ZENABLE, s_zbuf ? D3DZB_TRUE : D3DZB_FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    s_dev->SetRenderState(D3DRENDERSTATE_CULLMODE, D3DCULL_NONE);
    s_cull = D3DCULL_NONE;
    s_dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
    s_dev->SetTexture(0, 0);
}

// n triangles (DrawPrimitive counts vertices: at most 0xFFFF per call)
void rndTris(const platVERT *v, int n)
{
    rndSetCull(D3DCULL_NONE);
    while (n > 0) {
        int k = n > 0x5550 ? 0x5550 : n;
        s_dev->DrawPrimitive(D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE, (void *)v, k * 3, 0);
        v += k * 3;
        n -= k;
    }
}

// n triangles blended by their colour's alpha (no depth writes), e.g. a water surface
void rndTrisAlpha(const platVERT *v, int n)
{
    rndSetCull(D3DCULL_NONE);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    s_dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    s_dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    s_dev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    rndTris(v, n);
    s_dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
    s_dev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
}

// the whole view tinted by an ARGB colour (alpha = strength)
void rndOverlay(unsigned argb)
{
    rndSetCull(D3DCULL_NONE);
    struct { float x, y, z, rhw; unsigned c; } q[4] = {
        { 0, 0, 0, 1, argb }, { (float)s_w, 0, 0, 1, argb }, { 0, (float)s_h, 0, 1, argb }, { (float)s_w, (float)s_h, 0, 1, argb } };
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    s_dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    s_dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    s_dev->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    s_dev->DrawPrimitive(D3DPT_TRIANGLESTRIP, D3DFVF_XYZRHW | D3DFVF_DIFFUSE, q, 4, 0);
    s_dev->SetRenderState(D3DRENDERSTATE_ZENABLE, s_zbuf ? D3DZB_TRUE : D3DZB_FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
}

void rndEnd()
{
    s_dev->EndScene();
    if (s_full) {                                       // d3dDRIVER::SwapBuffers full screen: flip the chain
        if (s_primary->Flip(0, DDFLIP_WAIT) == DDERR_SURFACELOST)
            s_dd->RestoreAllSurfaces();                 // (alt-tab: d3dDRIVER::RestoreSurfaces)
        return;
    }
    POINT p = { 0, 0 };
    ClientToScreen(s_hwnd, &p);
    RECT dst = { p.x, p.y, p.x + s_w, p.y + s_h };
    if (s_primary->Blt(&dst, s_back, 0, DDBLT_WAIT, 0) == DDERR_SURFACELOST)
        s_dd->RestoreAllSurfaces();
}

int rndScreenshot(const char *path)
{
    DDSURFACEDESC2 sd;
    memset(&sd, 0, sizeof(sd));
    sd.dwSize = sizeof(sd);
    if (FAILED(s_back->Lock(0, &sd, DDLOCK_WAIT | DDLOCK_READONLY, 0)))
        return 0;
    FILE *f = fopen(path, "wb");
    if (!f) {
        s_back->Unlock(0);
        return 0;
    }
    fprintf(f, "P6\n%d %d\n255\n", s_w, s_h);
    DDPIXELFORMAT &pf = sd.ddpfPixelFormat;
    int bpp = pf.dwRGBBitCount / 8;
    for (int y = 0; y < s_h; y++) {
        unsigned char *row = (unsigned char *)sd.lpSurface + y * sd.lPitch;
        for (int x = 0; x < s_w; x++) {
            unsigned px = 0;
            memcpy(&px, row + x * bpp, bpp);
            unsigned char rgb[3];
            unsigned masks[3] = { pf.dwRBitMask, pf.dwGBitMask, pf.dwBBitMask };
            for (int c = 0; c < 3; c++) {
                unsigned m = masks[c], sh = 0;
                while (m && !(m & 1)) { m >>= 1; sh++; }
                unsigned v = (px >> sh) & m;
                rgb[c] = m ? (unsigned char)(v * 255 / m) : 0;
            }
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
    s_back->Unlock(0);
    return 1;
}

const char *rndDeviceName()
{
    return s_devName;
}

// ---------------------------------------------------------------------------------------------
// textures (test only)
void *rndTexCreate(int w, int h)
{
    DDSURFACEDESC2 sd;
    memset(&sd, 0, sizeof(sd));
    sd.dwSize = sizeof(sd);
    sd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    sd.dwWidth = w;
    sd.dwHeight = h;
    sd.ddsCaps.dwCaps = DDSCAPS_TEXTURE;
    int nMip = 1;                                       // a full mip chain (as the game's texture driver: the
    while ((w >> nMip) > 0 || (h >> nMip) > 0)          // distant surfaces filtered, not aliased)
        nMip++;
    if (nMip > 1) {
        sd.dwFlags |= DDSD_MIPMAPCOUNT;
        sd.dwMipMapCount = nMip;
        sd.ddsCaps.dwCaps |= DDSCAPS_MIPMAP | DDSCAPS_COMPLEX;
    }
    sd.ddsCaps.dwCaps2 = DDSCAPS2_TEXTUREMANAGE;
    sd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    sd.ddpfPixelFormat.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
    sd.ddpfPixelFormat.dwRGBBitCount = 32;
    sd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
    sd.ddpfPixelFormat.dwGBitMask = 0xFF00;
    sd.ddpfPixelFormat.dwBBitMask = 0xFF;
    sd.ddpfPixelFormat.dwRGBAlphaBitMask = 0xFF000000;
    LPDIRECTDRAWSURFACE7 s = 0;
    if (FAILED(s_dd->CreateSurface(&sd, &s, 0)))
        return 0;
    return s;
}

int rndTexUpload(void *tex, const unsigned *argb, int w, int h)
{
    LPDIRECTDRAWSURFACE7 s = (LPDIRECTDRAWSURFACE7)tex;
    DDSURFACEDESC2 sd;
    memset(&sd, 0, sizeof(sd));
    sd.dwSize = sizeof(sd);
    if (FAILED(s->Lock(0, &sd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0)))
        return 0;
    for (int y = 0; y < h; y++)
        memcpy((unsigned char *)sd.lpSurface + y * sd.lPitch, argb + y * w, w * 4);
    s->Unlock(0);
    // the smaller levels: each a 2x2 box filter of the one above
    unsigned *prev = (unsigned *)argb, *buf = 0;
    int pw = w, ph = h;
    LPDIRECTDRAWSURFACE7 lv = s;
    lv->AddRef();
    for (;;) {
        DDSCAPS2 caps;
        memset(&caps, 0, sizeof(caps));
        caps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
        LPDIRECTDRAWSURFACE7 next = 0;
        HRESULT hr = lv->GetAttachedSurface(&caps, &next);
        lv->Release();
        if (FAILED(hr) || !next)
            break;
        lv = next;
        int nw = pw > 1 ? pw / 2 : 1, nh = ph > 1 ? ph / 2 : 1;
        unsigned *cur = (unsigned *)malloc(nw * nh * 4);
        if (!cur) {                                     // (out of memory: the smaller levels stay as made)
            lv->Release();
            break;
        }
        for (int y = 0; y < nh; y++)
            for (int x = 0; x < nw; x++) {
                int x0 = x * 2 < pw ? x * 2 : pw - 1, x1 = x * 2 + 1 < pw ? x * 2 + 1 : pw - 1;
                int y0 = y * 2 < ph ? y * 2 : ph - 1, y1 = y * 2 + 1 < ph ? y * 2 + 1 : ph - 1;
                unsigned a = prev[y0 * pw + x0], b = prev[y0 * pw + x1], c = prev[y1 * pw + x0], d = prev[y1 * pw + x1];
                unsigned o = 0;
                for (int k = 0; k < 32; k += 8)
                    o |= ((((a >> k) & 0xFF) + ((b >> k) & 0xFF) + ((c >> k) & 0xFF) + ((d >> k) & 0xFF) + 2) >> 2) << k;
                cur[y * nw + x] = o;
            }
        memset(&sd, 0, sizeof(sd));
        sd.dwSize = sizeof(sd);
        if (SUCCEEDED(lv->Lock(0, &sd, DDLOCK_WAIT | DDLOCK_WRITEONLY, 0))) {
            for (int y = 0; y < nh; y++)
                memcpy((unsigned char *)sd.lpSurface + y * sd.lPitch, cur + y * nw, nw * 4);
            lv->Unlock(0);
        }
        free(buf);
        buf = cur;
        prev = cur;
        pw = nw;
        ph = nh;
    }
    free(buf);
    return 1;
}

void rndTexFree(void *tex)
{
    if (tex)
        ((LPDIRECTDRAWSURFACE7)tex)->Release();
}

void rndTris2D(void *tex, const platVERT2D *v, int n)
{
    rndSetCull(D3DCULL_NONE);
    s_dev->SetTexture(0, (LPDIRECTDRAWSURFACE7)tex);
    s_dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
    s_dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    s_dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    s_dev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    s_dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    s_dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    s_dev->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    s_dev->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    s_dev->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_NONE);
    s_dev->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    s_dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    s_dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    s_dev->SetRenderState(D3DRENDERSTATE_ZENABLE, D3DZB_FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    for (int i = 0; i < n; i += 0x5550) {
        int k = n - i < 0x5550 ? n - i : 0x5550;
        s_dev->DrawPrimitive(D3DPT_TRIANGLELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1, (void *)(v + i * 3), k * 3, 0);
    }
    s_dev->SetTexture(0, 0);
    s_dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
    s_dev->SetRenderState(D3DRENDERSTATE_ZENABLE, s_zbuf ? D3DZB_TRUE : D3DZB_FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
}

void rndSetTransforms(const float view[16], const float proj[16])
{
    s_dev->SetTransform(D3DTRANSFORMSTATE_VIEW, (D3DMATRIX *)view);
    s_dev->SetTransform(D3DTRANSFORMSTATE_PROJECTION, (D3DMATRIX *)proj);
}

void rndSetCull(int mode)
{
    if (s_cull != mode) {
        s_dev->SetRenderState(D3DRENDERSTATE_CULLMODE, (DWORD)mode);
        s_cull = mode;
    }
}

// the one user clip plane (world, a x + b y + c z + d >= 0 is kept); 0: off
void rndClipPlane(const float *plane)
{
    if (plane)
        s_dev->SetClipPlane(0, (D3DVALUE *)plane);
    s_dev->SetRenderState(D3DRENDERSTATE_CLIPPLANEENABLE, plane ? D3DCLIPPLANE0 : 0);
}

// n triangles blended by their colour's alpha, in the depth buffer
void rndTrisAlphaZ(const platVERT *v, int n)
{
    rndSetCull(D3DCULL_NONE);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
    s_dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
    s_dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, D3DBLEND_INVSRCALPHA);
    rndTris(v, n);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
}

void rndClearDepth()
{
    if (s_zbuf)
        s_dev->Clear(0, 0, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
}

int rndWidth() { return s_w; }
int rndHeight() { return s_h; }
int platIsFullScreen() { return s_full; }
void *platHwnd() { return s_hwnd; }

void rndTrisTex(void *tex, const platVERT3T *v, int n, int blend)
{
    rndSetCull(D3DCULL_NONE);
    s_dev->SetTexture(0, (LPDIRECTDRAWSURFACE7)tex);
    s_dev->SetTextureStageState(0, D3DTSS_COLOROP, tex ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
    s_dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    s_dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    s_dev->SetTextureStageState(0, D3DTSS_ALPHAOP, tex ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
    s_dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    s_dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    s_dev->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    s_dev->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    s_dev->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
    s_dev->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
    s_dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    if (blend) {
        s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        s_dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);
        s_dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, blend == 2 ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);   // 2: additive
        s_dev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
    } else {
        s_dev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
        s_dev->SetRenderState(D3DRENDERSTATE_ALPHAREF, 0x60);
        s_dev->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    }
    for (int i = 0; i < n; i += 0x5550) {
        int k = n - i < 0x5550 ? n - i : 0x5550;
        s_dev->DrawPrimitive(D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1, (void *)(v + i * 3), k * 3, 0);
    }
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    s_dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
    s_dev->SetTexture(0, 0);
}

int platMouseButtons()
{
    return s_mouseBtn;
}

// Two stages: the base texture times the lightmap (second coordinate set), times the vertex colour
void rndTrisTex2(void *tex, void *lm, const platVERT3T2 *v, int n, int blend)
{
    s_dev->SetTexture(0, (LPDIRECTDRAWSURFACE7)tex);
    s_dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    s_dev->SetTextureStageState(0, D3DTSS_COLOROP, tex ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
    s_dev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    s_dev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    s_dev->SetTextureStageState(0, D3DTSS_ALPHAOP, tex ? D3DTOP_MODULATE : D3DTOP_SELECTARG2);
    s_dev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    s_dev->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
    s_dev->SetTextureStageState(0, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    s_dev->SetTextureStageState(0, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    s_dev->SetTextureStageState(0, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
    s_dev->SetTextureStageState(0, D3DTSS_ADDRESS, D3DTADDRESS_WRAP);
    s_dev->SetTexture(1, (LPDIRECTDRAWSURFACE7)lm);
    s_dev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
    s_dev->SetTextureStageState(1, D3DTSS_COLOROP, lm ? D3DTOP_MODULATE : D3DTOP_DISABLE);
    s_dev->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    s_dev->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
    s_dev->SetTextureStageState(1, D3DTSS_ALPHAOP, lm ? D3DTOP_SELECTARG2 : D3DTOP_DISABLE);
    s_dev->SetTextureStageState(1, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
    s_dev->SetTextureStageState(1, D3DTSS_MAGFILTER, D3DTFG_LINEAR);
    s_dev->SetTextureStageState(1, D3DTSS_MINFILTER, D3DTFN_LINEAR);
    s_dev->SetTextureStageState(1, D3DTSS_MIPFILTER, D3DTFP_LINEAR);
    s_dev->SetTextureStageState(1, D3DTSS_ADDRESS, D3DTADDRESS_CLAMP);
    s_dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_GOURAUD);
    if (blend) {                                        // 1 alpha blend, 2 additive
        s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, TRUE);
        s_dev->SetRenderState(D3DRENDERSTATE_SRCBLEND, D3DBLEND_SRCALPHA);   // (additive: weighted by the texture's alpha)
        s_dev->SetRenderState(D3DRENDERSTATE_DESTBLEND, blend == 2 ? D3DBLEND_ONE : D3DBLEND_INVSRCALPHA);
        s_dev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, FALSE);
    } else {
        s_dev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, TRUE);
        s_dev->SetRenderState(D3DRENDERSTATE_ALPHAREF, 0x60);
        s_dev->SetRenderState(D3DRENDERSTATE_ALPHAFUNC, D3DCMP_GREATEREQUAL);
    }
    for (int i = 0; i < n; i += 0x5550) {
        int k = n - i < 0x5550 ? n - i : 0x5550;
        s_dev->DrawPrimitive(D3DPT_TRIANGLELIST, D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2, (void *)(v + i * 3), k * 3, 0);
    }
    s_dev->SetTexture(1, 0);
    s_dev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    s_dev->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHATESTENABLE, FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_ALPHABLENDENABLE, FALSE);
    s_dev->SetRenderState(D3DRENDERSTATE_ZWRITEENABLE, TRUE);
    s_dev->SetRenderState(D3DRENDERSTATE_SHADEMODE, D3DSHADE_FLAT);
    s_dev->SetTexture(0, 0);
}
