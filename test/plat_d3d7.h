// plat_d3d7.h -- minimal platform layer for the box test level, on the engine's own APIs:
// DirectDraw 7 + Direct3D 7 (IDirect3DDevice7, T&L HAL) for drawing and DirectInput 7 for input,
// as willrock.exe 1.2 imports them (DDRAW.dll DirectDrawCreateEx, DINPUT.dll DirectInputCreateW).
// This is NOT the game's renderer (drv:D3d_*.obj, not rebuilt yet): it only draws flat-coloured triangles.
#pragma once

struct platVERT { float x, y, z; unsigned color; };    // D3DFVF_XYZ | D3DFVF_DIFFUSE
void rndClipPlane(const float *plane);
void rndTrisAlphaZ(const platVERT *v, int n);
void rndTrisAlpha(const platVERT *v, int n);       // alpha blended, no depth written

extern int platFullScreen, platBPP, platTripleBuf, platRefresh;   // [Video] VIDEO_FullScr / BPP / TripleBuf
int   platInit(int w, int h, const char *title, int hidden);
void  platTerm();
void *platHwnd();                                       // the window (gsWINDOWS: _apHMainWindow)
int   platIsFullScreen();                              // the mode actually in use
int   platToggleFullScreen(int w, int h);              // TEMPORARY (F11): full screen <-> window at w x h
int   platPump();                                       // 0 = quit
double platTime();                                      // seconds
void  platSetTitle(const char *s);

// input: DIK_* scancodes; mouse movement in DirectInput counts since the last call
void  platInput(unsigned char keys[256], int *mdx, int *mdy);

// rendering: row-vector matrices (D3D convention, same as m3dMATR)
void  rndBegin(const float view[16], const float proj[16], unsigned clearColor);
void  rndTris(const platVERT *v, int n);              // n triangles
void  rndTrisAlpha(const platVERT *v, int n);         // blended by colour alpha, no depth writes
void  rndOverlay(unsigned argb);                      // tint the whole view
void  rndEnd();
int   rndScreenshot(const char *ppmPath);              // the back buffer as a binary PPM
const char *rndDeviceName();

// textures (test only): ARGB8888 managed texture surfaces, and screen-space textured triangles for the UI
struct platVERT2D { float x, y, z, rhw; unsigned color; float u, v; };   // D3DFVF_XYZRHW | DIFFUSE | TEX1
void *rndTexCreate(int w, int h);                       // 0 = failed
int   rndTexUpload(void *tex, const unsigned *argb, int w, int h);
void  rndTexFree(void *tex);
void  rndTris2D(void *tex, const platVERT2D *v, int n); // n triangles, alpha-blended, no depth test
struct platVERT3T { float x, y, z; unsigned color; float u, v; };      // D3DFVF_XYZ | DIFFUSE | TEX1
void  rndSetTransforms(const float view[16], const float proj[16]);
void  rndTrisTex(void *tex, const platVERT3T *v, int n, int blend); // n triangles; blend: 0 opaque (alpha test), 1 alpha
void  rndClearDepth();
void  rndSetCull(int mode);   // D3DCULL: 1 none, 2 clockwise, 3 counter-clockwise (kept until changed)
int   rndWidth();
int   rndHeight();
int   platMouseButtons();                              // bit 0 left, bit 1 right (as of the last platInput)
struct platVERT3T2 { float x, y, z; unsigned color; float u, v, u2, v2; };  // XYZ | DIFFUSE | TEX2
void  rndTrisTex2(void *tex, void *lightmap, const platVERT3T2 *v, int n, int blend); // base * lightmap
