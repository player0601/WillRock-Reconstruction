// m3d_quant.cpp -- m3d.lib value quantisers (Will Rock 1.2, v12.exe; v12 0x00572240..0x0057243D, 1.2 only)
//
// A float in [mn, mx] to an n-bit code and back, clamped (the multiplayer position packing, mpQuantPos). Names
// ours. The 8-bit variants code an angle into a byte (the player's pitch in its sync block).

#include "m3d_lib.h"

// the clamped linear map the two share (inlined in both): y0 at x0 .. y1 at x1, the ends swapped when x1 < x0
static inline float m3dQuantLerp(float x, float x0, float y0, float x1, float y1)
{
    if (x1 < x0) {
        float t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }
    if (x < x0)
        return y0;
    if (!(x <= x1))
        return y1;
    return (y1 - y0) * (x - x0) / (x1 - x0) + y0;
}

// v12 0x00572240
int __fastcall m3dQuantize(int bits, float v, float mn, float mx)
{
    float n = (float)((2 << (bits - 1)) - 1);
    return (int)m3dQuantLerp(v, mn, 0.0f, mx, n);
}

// v12 0x00572330
float __fastcall m3dDequantize(unsigned code, int bits, float mn, float mx)
{
    float n = (float)((2 << (bits - 1)) - 1);
    return m3dQuantLerp((float)code, 0.0f, mn, n, mx);
}

// v12 0x00572440
// v to 0..255 over [mn, mx] (the player's pitch over 0..180: plrPLAYER::Animate).
int __fastcall m3dQuantize8(float v, float mn, float mx)
{
    return (int)m3dQuantLerp(v, mn, 0.0f, mx, 255.0f);
}

// v12 0x00572540
// A byte back to [mn, mx]: 0 is mn, 255 mx (plrPLAYER_MP::ReadAnimSync).
float __fastcall m3dDequantize8(int code, float mn, float mx)
{
    float c = (float)(code & 0xFF);
    if (!(c > 0.0f))
        return mn;
    if (c > 255.0f)
        return mx;
    return (mx - mn) * c * (1.0f / 255.0f) + mn;
}
