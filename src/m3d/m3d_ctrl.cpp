// m3d_ctrl.cpp -- Will Rock 1.2 (v12.exe): m3dCTRL_* time controllers (spline / noise driven values)
//
// Reconstructed from the Feb 2003 build (m3d:m3d_ctrl.obj) and checked against v12.
// All functions of this object are byte-identical in 1.2 (relocations only).
// v12 range 0x005691C0 .. 0x005695B2 (feb 0x00551130 .. 0x00551522).

#include "m3d_ctrl.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x005695A0 (feb 0x00551510) -> v12 0x009AEAC4 (feb 0x00963778)

// Maps the controller time t into [0,1] over [0,length]; a negative length runs the curve backwards.
// (inlined twice in the original: both GetValue functions of the SPL controllers)
static inline float _m3dCtrlNormTime(float t, float length)
{
    float t0 = 0.0f, t1 = length, v0 = 0.0f, v1 = 1.0f;
    if (length < 0.0f) {          // reversed
        t1 = 0.0f; t0 = length;
        v0 = 1.0f; v1 = 0.0f;
    }
    if (t < t0) return v0;
    if (t > t1) return v1;
    float a = t0, b = t1, va = v0, vb = v1;
    if (b < a) { b = t0; vb = v0; a = t1; va = v1; }    // never true after the swap above, kept as compiled
    return (t - a) * (vb - va) / (b - a) + va;
}


// v12 0x005691C0  (feb 0x00551130)
// Advances t by dt*speed; past the end it wraps (unless CLAMP) and disables itself if STOP_AT_END.
void m3dCTRL_TIME_::Update(float dt)
{
    unsigned f = flags;
    if (!(f & M3D_CTRL_RUNNING))
        return;
    float nt = dt * speed + t;
    t = nt;
    if (nt > length) {
        if (!(f & M3D_CTRL_CLAMP))
            t = nt - length;
        if (f & M3D_CTRL_STOP_AT_END)
            Enable(0);
    }
}


// v12 0x00569220  (feb 0x00551190)
// Evaluates the 1D spline at the normalised controller time.
void m3dCTRL_1D_SPL::GetValue(float *out)
{
    float u = _m3dCtrlNormTime(t, length);
    spl->GetValue(u, out, 0);
}


// v12 0x00569330  (feb 0x005512A0)
// Largest value the spline takes at its integer key positions (0 .. nKp-1).
float m3dCTRL_1D_SPL::GetMaxValue()
{
    float best = -M3D_INFINITY;
    int   n    = spl->nKp;
    for (int i = 0; i < n; i++) {
        float v;
        spl->GetValue((float)i, &v, 0);
        if (v > best)
            best = v;
    }
    return best;
}


// v12 0x005693A0  (feb 0x00551310)
// Evaluates the colour spline at the normalised controller time.
void m3dCTRL_COLOR_SPL::GetValue(m3dCOLOR *out)
{
    float u = _m3dCtrlNormTime(t, length);
    spl->GetValue(u, out, 0);
}


// v12 0x005694B0  (feb 0x00551420)
// Perlin noise along a line in noise space, mapped from [-1,1] to [vMin,vMax].
void m3dCTRL_1D_NOISE::GetValue(float *out)
{
    m3dV p;
    p.x = t * freq + M3D_PI;
    p.y = M3D_PI * 0.5f;
    p.z = p.y;
    float n = m3dNoiseGetValue(noise, &p);
    *out = (vMax - vMin) * (n + 1.0f) * 0.5f + vMin;
}


// v12 0x00569510  (feb 0x00551480)
// Upper bound of the noise controller.
float m3dCTRL_1D_NOISE::GetMaxValue()
{
    return vMax;
}


// v12 0x00569520  (feb 0x00551490)
// Noise-modulated colour: rgb = color.rgb * noise in [vMin,vMax], alpha = color.a.
void m3dCTRL_COLOR_NOISE::GetValue(m3dCOLOR *out)
{
    m3dV p;
    p.x = t * freq + M3D_PI;
    p.y = M3D_PI * 0.5f;
    p.z = p.y;
    float n = m3dNoiseGetValue(noise, &p);
    float k = (vMax - vMin) * (n + 1.0f) * 0.5f + vMin;
    out->a = color.a;
    out->r = k * color.r;
    out->g = k * color.g;
    out->b = k * color.b;
}
