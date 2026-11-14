// m3d_noise.cpp -- Will Rock 1.2 (v12.exe): Perlin gradient noise of the m3d library
//
// Reconstructed from the Feb 2003 build (m3d:m3d_noise.obj) and checked against v12.
// v12 range 0x00574570 .. 0x00574B70 (feb 0x0055BFB0 .. 0x0055C5A0). No logic change in 1.2.
//
// One global instance m3dNoise (m3dNOISE_GRAD, 512 gradients) is built by m3dInit -> m3dInitNoise.
// Users: m3dCTRL_*_NOISE controllers (light flicker, wobbling particles, camera shake).
// Mod hook point: m3dNOISE_GRAD::GetValue (v12 0x005748C0) -- returns [-1,1].

#include "m3d_geom.h"
#include "m3d_noise.h"

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x00574A90 (feb 0x0055C4D0) -> v12 0x009AEAFC

m3dNOISE *m3dNoise   = 0;     // v12 0x009AEB00 (feb 0x009637B0)
m3dSPL   *m3dNoiseSpl = 0;    // v12 0x009AEB04 (feb 0x009637B4) unnamed in the map; 4-key Lagrange spline t = 0..3 // ? purpose


// v12 0x00574570  (feb 0x0055BFB0)
// Creates the helper Lagrange spline (keys at t = 0,1,2,3) and the global gradient noise (512 entries).
BOOL32 __fastcall m3dInitNoise()
{
    m3dSPL_LAGRANGE *spl = new m3dSPL_LAGRANGE;        // inline ctor: flags 0, type 6, kpValueSize 8, stride 8, dim 1
    m3dNoiseSpl = spl;
    if (!spl)
        return 0;
    spl->AllocKpList(4);
    for (int i = 0; i < 4; i++) {
        int k = (i < m3dNoiseSpl->nKpMax) ? i : m3dNoiseSpl->nKpMax;
        *(float *)((char *)m3dNoiseSpl->kp + m3dNoiseSpl->kpStride * k) = (float)i;   // key time
    }
    m3dNoise = new m3dNOISE_GRAD;                      // size 0x200, gradient stride 0xC
    if (!m3dNoise) {
        m3dNoise = 0;
        return 0;
    }
    m3dNoise->Fill(0);
    return 1;
}


// v12 0x00574660  (feb 0x0055C0A0)
// Deletes the spline and the noise (pointers are left dangling, as in the original).
void __fastcall m3dTermNoise()
{
    if (m3dNoiseSpl)
        m3dNoiseSpl->__vecDelDtor(1);
    if (m3dNoise)
        m3dNoise->__vecDelDtor(1);
}


// v12 0x00574690  (feb 0x0055C0D0)
// Base Fill: (re)allocates hash + gradient tables and fills the hash with a random permutation
// (inlined copy of _noiseFillHash). Gradients are left untouched. 'seed' is unused.
BOOL32 m3dNOISE::Fill(int seed)
{
    hash = (int *)apRealloc(hash, size * 4);
    if (!hash)
        return 0;
    grad = (float *)apRealloc(grad, gradStride * size);
    if (!grad)
        return 0;
    int  n   = size;
    int *out = hash;
    int *tmp = (int *)apCalloc(n, 4);
    if (!tmp)
        return 0;
    for (int i = 0; i < n; i++)
        tmp[i] = i;
    int left = n;
    for (int i = 0; i < n; i++) {
        int k = (int)(m3dRandom() * left);
        int j = -1, v;
        do {                                           // skip k+1 still-unused entries
            do v = tmp[++j]; while (v == -1);
        } while (--k > 0);
        out[i] = v;
        left--;
        tmp[j] = -1;
    }
    apFree(tmp);
    return 1;
}


// v12 0x00574770  (feb 0x0055C1B0)
// Gradient Fill: random permutation + random unit gradients (z uniform, longitude 'phi').
// NB the longitude is rand*360 fed to cos/sin as RADIANS (original quirk, harmless).
// 1.2: same logic (inlined m3dSqrt rescheduled).
BOOL32 m3dNOISE_GRAD::Fill(int seed)
{
    hash = (int *)apRealloc(hash, size * 4);
    if (!hash)
        return 0;
    grad = (float *)apRealloc(grad, gradStride * size);
    if (!grad)
        return 0;
    if (!_noiseFillHash(hash, size))
        return 0;
    float *g = grad;
    for (int i = 0; i < size; i++) {
        float rz  = m3dRandom();
        float z   = 1.0f - (rz + rz);                  // uniform in [-1,1]
        float r   = m3dSqrt(1.0f - z * z);
        float phi = m3dRandom() * 360.0f;
        g[2] = z;
        g[0] = m3dCos(phi) * r;
        g[1] = m3dSin(phi) * r;
        g += 3;
    }
    return 1;
}


// v12 0x005748C0  (feb 0x0055C300)
// Classic 3D gradient (Perlin) noise with the cubic s-curve, remapped: values below -0.3 give -1,
// above 0.3 give 1, linear in between.
float m3dNOISE_GRAD::GetValue(m3dV *p)
{
    const float *pc = &p->x;
    int   ip[3];
    float fp[3], sp[3];
    for (int i = 0; i < 3; i++) {
        ip[i] = m3dIntFloor(pc[i]);
        fp[i] = pc[i] - (float)ip[i];
        sp[i] = (3.0f - (fp[i] + fp[i])) * fp[i] * fp[i];
    }
    unsigned mask = size - 1;
    float va[2];
    for (int a = 0; a < 2; a++) {
        float vb[2];
        for (int b = 0; b < 2; b++) {
            float vc[2];
            for (int c = 0; c < 2; c++) {
                int h = hash[(hash[(hash[(ip[2] + c) & mask] + ip[1] + b) & mask] + ip[0] + a) & mask];
                const float *g = (const float *)((const char *)grad + h * gradStride);
                vc[c] = (fp[0] - a) * g[0] + (fp[1] - b) * g[1] + (fp[2] - c) * g[2];
            }
            vb[b] = (vc[1] - vc[0]) * sp[2] + vc[0];
        }
        va[a] = (vb[1] - vb[0]) * sp[1] + vb[0];
    }
    float v = (va[1] - va[0]) * sp[0] + va[0];
    if (v < -0.3f)
        return -1.0f;
    if (0.3f < v)
        return 1.0f;
    return (v - -0.3f) * 3.3333333f - 1.0f;
}


// v12 0x00574AB0  (feb 0x0055C4F0)
// Fills hash[0..n) with a random permutation of 0..n-1 (CRT rand). Returns 0 when out of memory.
BOOL32 __fastcall _noiseFillHash(int *hash, int n)
{
    int *tmp = (int *)apCalloc(n, 4);
    if (!tmp)
        return 0;
    for (int i = 0; i < n; i++)
        tmp[i] = i;
    int left = n;
    for (int i = 0; i < n; i++) {
        int k = (int)(m3dRandom() * left);
        int j = -1, v;
        do {
            do v = tmp[++j]; while (v == -1);
        } while (--k > 0);
        hash[i] = v;
        left--;
        tmp[j] = -1;
    }
    apFree(tmp);
    return 1;
}


// v12 0x00574B40  (feb 0x0055C580) -- shared by m3dNOISE and m3dNOISE_GRAD (scalar deleting dtor)
// Frees both tables.
void *m3dNOISE::__vecDelDtor(unsigned f)
{
    apFree(grad);
    apFree(hash);
    if (f & 1)
        operator delete(this);
    return this;
}
