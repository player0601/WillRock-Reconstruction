// m3d_spl.cpp -- Will Rock 1.2 (v12.exe): m3dSPL keyframe splines (linear 1D/2D/3D, Hermite, Lagrange,
// Bezier 2D/3D, quaternion, colour) -- animation tracks, camera paths, particle and colour controllers.
//
// Reconstructed from the Feb 2003 build (m3d:m3d_spl.obj) and checked against v12.
// v12 range 0x00565E90 .. 0x005691A0 (feb 0x0054E300 .. 0x005510E0).
// 1.2 changes: m3dSPL_LINEAR1D::GetValue got a fast path for raw 2/3-key splines and clamps t to the
// segment; the inline Get/SetKpValue of LINEAR1D/QUAT/COLOR are now emitted in this range (COMDAT placement).
// Everything else is byte-identical to the beta.

#include "m3d_spl.h"
#include <string.h>
#include <new>

M3D_DECLARE_EPS_SQ();   // _$E2: feb 0x0054EFC0 -> feb 0x00963738-area (v12 ?)

static const float SPL_EPS         = M3D_EPSILON;   // v12 0x00609938 (feb 0x005DE45C)
static const float SPL_BIG         = M3D_INFINITY;  // feb 0x005DE460
static const float SPL_MINMAX_EPS  = 0.003f;        // __real@3b449ba6: "dimension is constant" in Optimize
static const float SPL_PACK_RANGE  = 32000.0f;      // __real@46fa0000 (feb 0x005DE570)
static const float SPL_PACK_UNIT   = 1.0f / 32000;  // __real@3803126f (feb 0x005DE56C)

int m3dSPL_kpPackNmb = 6;                           // ?kpPackNmb@m3dSPL@@1HA  v12 0x00637C20 (feb 0x005F99CC)

// m3dIntFloor: inline fistp(x - 0.49999899) (qqq v12 0x00633BA8) == floor in the game's ranges
static inline int splIntFloor(float v) { return (int)__builtin_floorf(v); }

static inline float *kpAt(m3dSPL *s, int i)           // GetKpPtr, as float*
{
    if (i >= s->nKpMax) i = s->nKpMax;
    return (float *)((char *)s->kp + s->kpStride * i);
}

// Shared by every linear GetValue: interpolates one channel between keys (ta,va) and (tb,vb);
// coincident keys give the average.
static inline float splLerp(float t, float ta, float va, float tb, float vb)
{
    if (m3dFAbs(tb - ta) < SPL_EPS)
        return (va + vb) * 0.5f;
    if (tb < ta) { float x = ta; ta = tb; tb = x; x = va; va = vb; vb = x; }
    return (t - ta) * (vb - va) / (tb - ta) + va;
}


// =============================================================================================
// m3dSPL
// =============================================================================================

// v12 0x00565E90  (feb 0x0054E300)
// Factory: allocates an empty spline of the given m3dSPL_TYPE (key size / dimension per type).
m3dSPL *m3dSPL::Make(int type)
{
    m3dSPL *s = 0;
    switch (type) {
    case M3D_SPL_LINEAR1D: if ((s = new (std::nothrow) m3dSPL_LINEAR1D)) m3dSPL_Init(s, 0, 0x08, 1); break;
    case M3D_SPL_LINEAR2D: if ((s = new (std::nothrow) m3dSPL_LINEAR2D)) m3dSPL_Init(s, 1, 0x0C, 2); break;
    case M3D_SPL_LINEAR3D: if ((s = new (std::nothrow) m3dSPL_LINEAR3D)) m3dSPL_Init(s, 2, 0x10, 3); break; // ctor inlined
    case M3D_SPL_HERMIT:   if ((s = new (std::nothrow) m3dSPL_HERMIT))   m3dSPL_Init(s, 3, 0x10, 1); break;
    case M3D_SPL_BEZIER2D: if ((s = new (std::nothrow) m3dSPL_BEZIER2D)) m3dSPL_Init(s, 4, 0x1C, 2); break;
    case M3D_SPL_BEZIER3D: if ((s = new (std::nothrow) m3dSPL_BEZIER3D)) m3dSPL_Init(s, 5, 0x28, 3); break;
    case M3D_SPL_LAGRANGE: if ((s = new (std::nothrow) m3dSPL_LAGRANGE)) m3dSPL_Init(s, 6, 0x08, 1); break;
    case M3D_SPL_QUAT:     if ((s = new (std::nothrow) m3dSPL_QUAT))     m3dSPL_Init(s, 7, 0x14, 4); break; // ctor inlined
    case M3D_SPL_COLOR:    if ((s = new (std::nothrow) m3dSPL_COLOR))    m3dSPL_Init(s, 8, 0x14, 4); break;
    }
    return s;
}


// v12 0x00566090  (feb 0x0054E500)
// Factory + key list of n points (n == -1 keeps the current count, i.e. 0).
m3dSPL *m3dSPL::Make(int type, int n)
{
    m3dSPL *s = Make(type);
    if (!s)
        return 0;
    if (!s->AllocKpList(n)) {                       // inlined
        s->__vecDelDtor(1);
        return 0;
    }
    return s;
}


// v12 0x005660F0  (feb 0x0054E560)
// (Re)allocates the key list for n points (n == -1: keep nKp). Contents are kept (apRealloc).
BOOL32 m3dSPL::AllocKpList(int n)
{
    if (n == -1)
        n = nKp;
    int bytes = kpStride * n;
    void *p = apRealloc(kp, bytes);
    if (!p)
        return 0;
    kpBytes = bytes;
    kp      = p;
    nKp     = n;
    nKpMax  = n;
    return 1;
}


// v12 0x00566140  (feb 0x0054E5B0)
// Deep copy (keys, template key, dim map).
// NB as compiled nKp/nKpMax and the pack scale are NOT copied (the copy has 0 keys until AllocKpList). // ?
m3dSPL *m3dSPL::Duplicate()
{
    m3dSPL *d = Make(type);
    if (!d)
        return 0;
    void *p = apRealloc(d->kp, kpBytes);
    if (!p) {
        d->__vecDelDtor(1);
        return 0;
    }
    d->kp          = p;
    d->kpBytes     = kpBytes;
    d->flags      |= flags;
    d->kpValueSize = kpValueSize;
    d->kpStride    = kpStride;
    d->dim         = dim;
    d->kpBytes     = kpBytes;
    memcpy(d->kp, kp, kpBytes);
    if (SPL_DEFKP(this)) {
        SPL_DEFKP(d) = (float *)apMalloc(kpValueSize);
        if (!SPL_DEFKP(d))
            return 0;                               // leaks d (as compiled)
        memcpy(SPL_DEFKP(d), SPL_DEFKP(this), kpValueSize);
    }
    if (SPL_DIMMAP(this)) {
        SPL_DIMMAP(d) = (unsigned char *)apMalloc(dim);
        if (!SPL_DIMMAP(d))
            return 0;
        memcpy(SPL_DIMMAP(d), SPL_DIMMAP(this), dim);
    }
    return d;
}


// v12 0x00566230  (feb 0x0054E6A0)
// Removes n keys starting at i and shrinks the list.
BOOL32 m3dSPL::RemoveKp(int i, int n)
{
    if (n < 1)
        return 1;
    int end = i + n;
    if (end < nKp) {
        int e = end < nKpMax ? end : nKpMax;
        int s = i   < nKpMax ? i   : nKpMax;
        memmove((char *)kp + kpStride * s, (char *)kp + kpStride * e, (nKp - i - n) * kpStride);
    }
    return AllocKpList(nKp - n);                    // inlined (incl. the n == -1 quirk)
}


// v12 0x005662E0  (feb 0x0054E750)
// Decodes key i into out[] = { t, value[dim] (, extras) } whatever the storage (see m3dSPL_FLAGS_EXT).
void m3dSPL::GetKp(int i, void *outV)
{
    float *src = kpAt(this, i);
    float *out = (float *)outV;
    if (!src || !out)
        return;

    if (kpValueSize == kpStride) {                  // raw storage
        memcpy(out, src, kpStride);
        return;
    }

    BOOL32 hasTime = (flags & M3D_SPLF_NO_TIME) == 0;
    if (hasTime) out[0] = src[0];
    else         out[0] = (float)i;

    int N = m3dSPL_kpPackNmb;
    if (flags & M3D_SPLF_DIM_MAP) {
        memcpy(out + 1, SPL_DEFKP(this) + 1, kpValueSize - 4);     // constant channels
        unsigned char *map = SPL_DIMMAP(this);
        if (flags & M3D_SPLF_PACKED) {
            int r = i % N;
            float *base = (float *)((char *)kp + (((N - 1) * kpStride) / 2 + kpStride) * (i / N));
            if (r != 0) {
                short *d = (short *)((char *)base + kpStride + (kpStride / 2) * (r - 1));
                for (int k = 0; k < dim; k++)
                    if (map[k])
                        out[map[k]] = (float)d[k] * SPL_PACKSCALE(this) + base[k];   // ? base[k] does not skip t
                return;
            }
            src = base;
        }
        src += hasTime;
        for (int k = 0; k < dim; k++, src++)
            if (map[k])
                out[map[k]] = *src;
        return;
    }

    if (!(flags & M3D_SPLF_NO_TIME))
        return;
    if (flags & M3D_SPLF_PACKED) {
        int r = i % N;
        float *base = (float *)((char *)kp + (((N - 1) * kpStride) / 2 + kpStride) * (i / N));
        if (r != 0) {
            short *d = (short *)((char *)base + kpStride + (kpStride / 2) * (r - 1));
            for (int k = 0; k < dim; k++)
                out[1 + k] = (float)d[k] * SPL_PACKSCALE(this) + base[k];
            return;
        }
        src = base;
    }
    memcpy(out + 1, src, kpStride);
}


// v12 0x005664E0  (feb 0x0054E950)
// Segment of t: index i with kp[i].t <= t < kp[i+1].t; -2 before the first key (or CONST), -1 at/after the last.
int m3dSPL::FindSeg(float t)
{
    if (flags & M3D_SPLF_CONST)
        return -2;
    if (!(flags & M3D_SPLF_INTEGER_ARG)) {
        if (t < kpAt(this, nKpMax > 0 ? 0 : nKpMax)[0])
            return -2;
        int hi = nKp - 1;
        if (t >= kpAt(this, hi)[0])
            return -1;
        int lo = 0;
        while (hi - lo > 1) {                       // binary search
            int mid = (hi + lo) / 2;
            if (t < kpAt(this, mid)[0]) hi = mid;
            else                        lo = mid;
        }
        return lo;
    }
    if (t < SPL_EPS)
        return -2;
    if (t > ((float)nKp - 1.0f) - SPL_EPS)
        return -1;
    return splIntFloor(t);
}


// v12 0x00566600  (feb 0x0054EA70)
// Analyses / compresses the key list. what: 1 detect integer t, 2 detect constant, 8 strip constant
// channels (DIM_MAP), 4 drop t (needs INTEGER_ARG), 0x10 pack keys as 16-bit deltas. Returns 0 on alloc failure.
// TWEAK: the 0.003 "constant channel" threshold and 32000 pack range decide animation compression accuracy.
BOOL32 m3dSPL::Optimize(int what)
{
    m3dSPL_EXT *self = (m3dSPL_EXT *)this;
    if (what & 1) {
        int i = 0;
        for (; i < nKp; i++)
            if (!(m3dFAbs(*(float *)self->GetKpPtr(i) - (float)i) < SPL_EPS))
                break;
        if (i == nKp)
            flags |= M3D_SPLF_INTEGER_ARG;
    }
    if (what & 2) {
        void *k0 = self->GetKpPtr(0);
        int i = 1;
        for (; i < nKp; i++)
            if (!CompareKP(k0, self->GetKpPtr(i)))
                break;
        if (i == nKp)
            flags |= M3D_SPLF_CONST;
    }
    if (nKp < 2)
        return 1;

    float mn[20], mx[20];
    if ((what & 8) && !(flags & M3D_SPLF_DIM_MAP)) {
        unsigned char map[20] = { 0 };              // map[0] = 0: t
        int cnt = 1;
        int last = nKp - 1 < nKpMax ? nKp - 1 : nKpMax;
        GetMinMaxValue(kpAt(this, nKpMax > 0 ? 0 : nKpMax)[0], kpAt(this, last)[0], 1.0f, mn, mx);
        for (int d = 0; d < dim; d++)
            if (!(m3dFAbs(mx[d] - mn[d]) < SPL_MINMAX_EPS))
                map[cnt++] = (unsigned char)(d + 1);
        if (cnt - 1 < dim) {
            float *def = (float *)apMalloc(kpValueSize);
            SPL_DEFKP(this) = def;
            if (!def)
                return 0;
            memcpy(def, kp, kpValueSize);
            unsigned char *dm = (unsigned char *)apMalloc(dim);
            SPL_DIMMAP(this) = dm;
            if (!dm)
                return 0;
            memcpy(dm, map + 1, dim);
            char *dst = (char *)kp;
            int newStride = kpStride - 4 + (cnt - dim) * 4;
            for (int k = 0; k < nKp; k++)
                for (int m = 0; m < cnt; m++, dst += 4)
                    memmove(dst, (char *)kpAt(this, k) + map[m] * 4, 4);
            kpStride = newStride;
            AllocKpList(nKp);                       // inlined, failure ignored
            flags |= M3D_SPLF_DIM_MAP;
        }
    }

    if ((what & 4) && !(flags & M3D_SPLF_NO_TIME) && (flags & M3D_SPLF_INTEGER_ARG)) {
        char *dst = (char *)kp;
        int newStride = kpStride - 4;
        for (int k = 0; k < nKp; k++, dst += newStride)
            memmove(dst, (char *)kpAt(this, k) + 4, newStride);
        kpStride = newStride;
        AllocKpList(nKp);
        flags |= M3D_SPLF_NO_TIME;
    }

    if (what & 0x10) {
        int N = m3dSPL_kpPackNmb;
        float maxDelta = 0.0f;
        for (int k = 0; k < nKp; k++) {
            GetKp(k - k % N, mn);
            GetKp(k, mx);
            for (int d = 0; d < dim + 1; d++)
                if (m3dFAbs(mx[d] - mn[d]) >= maxDelta)
                    maxDelta = m3dFAbs(mx[d] - mn[d]);
        }
        int total = 0;
        for (int k = 0; k < nKp; k++) {
            int stride = kpStride;
            int r = k % N;
            char  *group = (char *)kp + (((N - 1) * stride) / 2 + stride) * (k / N);
            float *src   = kpAt(this, k);
            if (r == 0) {
                memmove(group, src, stride);
                total += stride;
            } else {
                short *d    = (short *)(group + stride + (stride / 2) * (r - 1));
                float  inv  = 1.0f / maxDelta;
                float *base = (float *)group;
                for (unsigned j = 0; j < (unsigned)stride >> 2; j++)
                    d[j] = (short)(int)((src[j] - base[j]) * inv * SPL_PACK_RANGE);
                total += stride / 2;
            }
        }
        SPL_PACKSCALE(this) = maxDelta * SPL_PACK_UNIT;
        void *p = apRealloc(kp, total);
        if (p) { kp = p; kpBytes = total; }
        flags |= M3D_SPLF_PACKED;
    }
    return 1;
}


// (feb 0x00550F40, private inline; v12 copy ~0x00568FA0)
// Address of key i (clamped to nKpMax).
void *m3dSPL_EXT::GetKpPtr(int i)
{
    if (i >= nKpMax) i = nKpMax;
    return (char *)kp + kpStride * i;
}


// v12 0x00566B70  (feb 0x0054EFE0)
// True (and sets INTEGER_ARG) when key i has t == i for all keys.
BOOL32 m3dSPL_EXT::IsIntegerArg()
{
    int i = 0;
    for (; i < nKp; i++)
        if (!(m3dFAbs(kpAt(this, i)[0] - (float)i) < SPL_EPS))
            break;
    if (i == nKp) {
        flags |= M3D_SPLF_INTEGER_ARG;
        return 1;
    }
    return 0;
}


// v12 0x00566C10  (feb 0x0054F080)
// True (and sets CONST) when every key compares equal (CompareKP) to the first.
BOOL32 m3dSPL_EXT::IsConst()
{
    void *k0 = kpAt(this, nKpMax > 0 ? 0 : nKpMax);
    int i = 1;
    for (; i < nKp; i++)
        if (!CompareKP(k0, kpAt(this, i)))
            break;
    if (i == nKp) {
        flags |= M3D_SPLF_CONST;
        return 1;
    }
    return 0;
}


// v12 0x004036E0 (folded, named ??_Em3dSPL_BEZIER2D in the map; slot 1 of every spline vtable)
// The deleting destructor shared by all spline types: the key points and the two extra buffers (+0x28, +0x2C).
void *m3dSPL::__vecDelDtor(unsigned flags)
{
    apFree(kp);
    apFree((void *)(intptr_t)pad[0]);
    apFree((void *)(intptr_t)pad[1]);
    if (flags & 1)
        operator delete(this);
    return this;
}

// v12 0x00566C80  (feb 0x0054F0F0)
// Default key comparison: byte-equal values (time ignored).
int m3dSPL::CompareKP(void *a, void *b)
{
    return memcmp((char *)a + 4, (char *)b + 4, kpValueSize - 4) == 0;
}


// v12 0x00566CB0  (feb 0x0054F120)
// Resamples [t0,t1] every `step` into a new spline of `type` (-1 = same type).
// NB the new spline leaks if its key list cannot be allocated (as compiled).
m3dSPL *m3dSPL::Sample(float t0, float t1, float step, int newType)
{
    if (newType == -1)
        newType = type;
    m3dSPL *s = Make(newType);
    if (!s)
        return 0;
    int n = 1 - splIntFloor(-((t1 - t0) / step));  // ceil((t1-t0)/step) + 1
    if (!s->AllocKpList(n))
        return 0;
    float v[20];
    for (int i = 0; i < n; i++) {
        float t = (float)i * step + t0;
        GetValue(t, v, 0);
        kpAt(s, i)[0] = t;
        s->SetKpValue(i, v);
    }
    return s;
}


// v12 0x00566DB0  (feb 0x0054F220)
// Per-channel min/max of the curve sampled every `step` over [t0,t1].
void m3dSPL::GetMinMaxValue(float t0, float t1, float step, void *mnV, void *mxV)
{
    float *mn = (float *)mnV, *mx = (float *)mxV;
    for (int d = 0; d < dim; d++) {
        mn[d] =  SPL_BIG;
        mx[d] = -SPL_BIG;
    }
    int n = 1 - splIntFloor(-((t1 - t0) / step));
    float v[20];
    for (int i = 0; i < n; i++) {
        GetValue((float)i * step + t0, v, 0);
        for (int d = 0; d < dim; d++) {
            if (v[d] < mn[d]) mn[d] = v[d];
            if (v[d] > mx[d]) mx[d] = v[d];
        }
    }
}


// =============================================================================================
// m3dSPL_LINEAR1D
// =============================================================================================

// v12 0x00566ED0  (feb 0x0054F340)
// Piecewise-linear scalar curve; constant before the first / after the last key.
// 1.2: fast path for raw (uncompressed) splines with <= 3 keys that reads the key array directly,
//      and the segment interpolation clamps t to the segment (feb extrapolated inside a segment).
void m3dSPL_LINEAR1D::GetValue(float t, void *outV, void *deriv)
{
    float *out = (float *)outV;
    // 1.2: inline helper "lerp between two keys, clamped"
    auto lerpKp = [t](const float *a, const float *b) -> float {
        const float *lo = a, *hi = b;
        if (b[0] < a[0]) { lo = b; hi = a; }
        if (t < lo[0])  return lo[1];
        if (t > hi[0])  return hi[1];
        return splLerp(t, lo[0], lo[1], hi[0], hi[1]);
    };

    if (nKp <= 3 && kpValueSize == kpStride) {      // 1.2: new fast path
        const float *k = (const float *)kp;
        if (nKp == 2)
            *out = lerpKp(k, k + 2);
        else if (nKp == 3)
            *out = (t < k[2]) ? lerpKp(k, k + 2) : lerpKp(k + 2, k + 4);
        else
            *out = k[1];                            // ? nKp 0/1 (reads key 0 even when empty)
        return;
    }

    float a[2 + 16], b[2 + 16];                     // GetKp writes kpValueSize bytes
    int seg = FindSeg(t);
    if (seg == -2) { GetKp(0, a);       *out = a[1]; return; }
    if (seg == -1) { GetKp(nKp - 1, a); *out = a[1]; return; }
    GetKp(seg, a);
    GetKp(seg + 1, b);
    *out = lerpKp(a, b);                            // 1.2: clamped (feb: plain splLerp)
}


// v12 0x00567470  (feb 0x0054F4B0)
// Sets key i (raw storage).
void m3dSPL_LINEAR1D::SetKp(int i, float t, float v)
{
    kpAt(this, i)[0] = t;
    kpAt(this, i)[1] = v;
}


// v12 0x005674B0  (feb 0x0054F4F0)
// Keys equal when the values differ by < 1e-4.
int m3dSPL_LINEAR1D::CompareKP(void *a, void *b)
{
    return m3dFAbs(((float *)b)[1] - ((float *)a)[1]) < 1e-4f;   // __real@38d1b717
}


// v12 0x00568FB0  (feb 0x004033A0, inline; ICF-shared by LINEAR1D/HERMIT/LAGRANGE)
// 1.2: COMDAT now emitted with m3d_spl.obj.
void m3dSPL_LINEAR1D::GetKpValue(int i, void *out)
{
    *(float *)out = kpAt(this, i)[1];
}


// v12 0x00569040  (feb 0x004033D0, inline; shared by LINEAR1D/LAGRANGE)
// 1.2: COMDAT now emitted with m3d_spl.obj.
void m3dSPL_LINEAR1D::SetKpValue(int i, void *v)
{
    kpAt(this, i)[1] = *(float *)v;
}


// =============================================================================================
// m3dSPL_LINEAR2D
// =============================================================================================

// v12 0x00567510  (feb 0x0054F550)
// Piecewise-linear 2D curve.
void m3dSPL_LINEAR2D::GetValue(float t, void *outV, void *deriv)
{
    float *out = (float *)outV;
    float a[20], b[20];
    int seg = FindSeg(t);
    if (seg == -2 || seg == -1) {
        GetKp(seg == -2 ? 0 : nKp - 1, a);
        out[0] = a[1];
        out[1] = a[2];
        return;
    }
    GetKp(seg, a);
    GetKp(seg + 1, b);
    out[0] = splLerp(t, a[0], a[1], b[0], b[1]);
    out[1] = splLerp(t, a[0], a[2], b[0], b[2]);
}


// v12 0x00567710  (feb 0x0054F750)
// Keys equal when both coordinates differ by < 0.001.
int m3dSPL_LINEAR2D::CompareKP(void *av, void *bv)
{
    float *a = (float *)av, *b = (float *)bv;
    return m3dFAbs(a[1] - b[1]) < 0.001f && m3dFAbs(a[2] - b[2]) < 0.001f;   // __real@3a83126f
}


// v12 0x00568F10  (feb 0x00550F60, inline; shared with BEZIER2D)
void m3dSPL_LINEAR2D::GetKpValue(int i, void *out)
{
    float *k = kpAt(this, i);
    ((float *)out)[0] = k[1];
    ((float *)out)[1] = k[2];
}


// v12 0x00568F40  (feb 0x00550F90, inline)
void m3dSPL_LINEAR2D::SetKpValue(int i, void *v)
{
    float *k = kpAt(this, i);
    k[1] = ((float *)v)[0];
    k[2] = ((float *)v)[1];
}


// =============================================================================================
// m3dSPL_LINEAR3D
// =============================================================================================

// v12 0x005677B0  (feb 0x0054F7F0)
// Piecewise-linear 3D path (camera / object paths).
void m3dSPL_LINEAR3D::GetValue(float t, void *outV, void *deriv)
{
    float *out = (float *)outV;
    float a[20], b[20];
    int seg = FindSeg(t);
    if (seg == -2 || seg == -1) {
        GetKp(seg == -2 ? 0 : nKp - 1, a);
        out[0] = a[1]; out[1] = a[2]; out[2] = a[3];
        return;
    }
    GetKp(seg, a);
    GetKp(seg + 1, b);
    for (int d = 1; d <= 3; d++)
        out[d - 1] = splLerp(t, a[0], a[d], b[0], b[d]);
}


// v12 0x00567A60  (feb 0x0054FAA0)
// Position of the key nearest in time to t (no interpolation).
void m3dSPL_LINEAR3D::GetValueClosest(float t, void *outV, void *deriv)
{
    float *out = (float *)outV;
    float a[20], b[20];
    int seg = FindSeg(t);
    const float *k = a;
    if (seg == -2)       GetKp(0, a);
    else if (seg == -1)  GetKp(nKp - 1, a);
    else {
        GetKp(seg, a);
        GetKp(seg + 1, b);
        if (b[0] - t <= t - a[0])
            k = b;
    }
    out[0] = k[1]; out[1] = k[2]; out[2] = k[3];
}


// v12 0x00567B50  (feb 0x0054FB90)
// Keys equal when all three coordinates differ by < 0.003.
int m3dSPL_LINEAR3D::CompareKP(void *av, void *bv)
{
    float *a = (float *)av, *b = (float *)bv;
    return m3dFAbs(a[1] - b[1]) < 0.003f && m3dFAbs(a[2] - b[2]) < 0.003f &&
           m3dFAbs(a[3] - b[3]) < 0.003f;
}


// v12 0x00567C40  (feb 0x0054FC80)
// Distance from point p to the polyline; optional segment index, segment parameter and closest point.
// Segments are visited in a scrambled order (stride 0x13D4FD mod nSeg) -- as compiled.
float m3dSPL_LINEAR3D::DistPoint(void *p, int *segOut, float *tOut, void *closestOut)
{
    float best = SPL_BIG;
    int   bestSeg = -1;
    float bestT = 0.0f;                             // ? uninitialised in the original when nothing is hit
    m3dV  bestC = { 0, 0, 0 };
    int   nSeg = nKp - 1;
    int   idx = 0;
    for (int left = nSeg; left > 0; left--) {
        idx = (idx + 0x13D4FD) % nSeg;
        int i0 = idx     < nKpMax ? idx     : nKpMax;
        int i1 = idx + 1 < nKpMax ? idx + 1 : nKpMax;
        float d, tt;
        m3dV  c;
        if (m3dDistPointLSeg((m3dV *)p, (m3dV *)(kpAt(this, i0) + 1), (m3dV *)(kpAt(this, i1) + 1),
                             best, &d, &c, &tt) && d < best) {
            best = d; bestC = c; bestT = tt; bestSeg = idx;
        }
    }
    if (segOut)     *segOut = bestSeg;
    if (tOut)       *tOut = bestT;
    if (closestOut) *(m3dV *)closestOut = bestC;
    return best;
}


// v12 0x00569070  (feb 0x00551060, inline; shared with BEZIER3D)
void m3dSPL_LINEAR3D::GetKpValue(int i, void *out)
{
    *(m3dV *)out = *(m3dV *)(kpAt(this, i) + 1);
}


// v12 0x00568F70  (feb 0x00550FC0, inline)
void m3dSPL_LINEAR3D::SetKpValue(int i, void *v)
{
    *(m3dV *)(kpAt(this, i) + 1) = *(m3dV *)v;
}


// =============================================================================================
// m3dSPL_HERMIT
// =============================================================================================

// v12 0x00567D60  (feb 0x0054FDA0)
// Cubic Hermite scalar curve: key {t, v, tanIn, tanOut}; uses a.tanOut and b.tanIn, scaled by the segment length.
void m3dSPL_HERMIT::GetValue(float t, void *outV, void *deriv)
{
    float *out = (float *)outV;
    float a[20], b[20];
    int seg = FindSeg(t);
    if (seg == -2) { GetKp(0, a);       *out = a[1]; return; }
    if (seg == -1) { GetKp(nKp - 1, a); *out = a[1]; return; }
    GetKp(seg, a);
    GetKp(seg + 1, b);
    if (m3dFAbs(a[0] - b[0]) < SPL_EPS) {
        *out = (b[1] + a[1]) * 0.5f;
        return;
    }
    float dt = b[0] - a[0];
    float s  = (t - a[0]) / dt;
    float s2 = s * s, s3 = s2 * s;
    float h  = (s3 + s3) - s2 * 3.0f;               // 2s^3 - 3s^2
    *out = ((h + 1.0f) * a[1] - h * b[1]) + (s3 - s2) * dt * b[2] + dt * a[3] * ((s3 - (s2 + s2)) + s);
}


// v12 0x00568FB0 (folded with m3dSPL_LINEAR1D::GetKpValue)
void m3dSPL_HERMIT::GetKpValue(int i, void *out)
{
    *(float *)out = kpAt(this, i)[1];
}


// v12 0x00568FE0  (feb 0x00551000, inline)
// Sets the value and zeroes both tangents.
void m3dSPL_HERMIT::SetKpValue(int i, void *v)
{
    kpAt(this, i)[1] = *(float *)v;
    kpAt(this, i)[3] = 0.0f;
    kpAt(this, i)[2] = 0.0f;
}


// =============================================================================================
// m3dSPL_LAGRANGE
// =============================================================================================

// v12 0x00567ED0  (feb 0x0054FF10)
// Global Lagrange polynomial through all keys (exact key value when t hits a key of the segment).
void m3dSPL_LAGRANGE::GetValue(float t, void *outV, void *deriv)
{
    float *out = (float *)outV;
    float a[20], b[20];
    int seg = FindSeg(t);
    if (seg == -2)      { GetKp(0, a);       memcpy(b, a, kpValueSize); }
    else if (seg == -1) { GetKp(nKp - 1, b); memcpy(a, b, kpValueSize); }
    else                { GetKp(seg, a);     GetKp(seg + 1, b); }

    if (m3dFAbs(a[0] - t) < SPL_EPS) { *out = a[1]; return; }
    if (m3dFAbs(b[0] - t) < SPL_EPS) { *out = b[1]; return; }

    float sum = 0.0f;
    for (int i = 0; i < nKp; i++) {
        GetKp(i, a);
        float term = a[1];
        for (int j = 0; j < nKp; j++) {
            GetKp(j, b);
            if (j != i)
                term = ((t - b[0]) / (a[0] - b[0])) * term;
        }
        sum = term + sum;
    }
    *out = sum;
}


// v12 0x00568FB0 (folded with m3dSPL_LINEAR1D::GetKpValue)
void m3dSPL_LAGRANGE::GetKpValue(int i, void *out)
{
    *(float *)out = kpAt(this, i)[1];
}


// v12 0x00569040 (folded with m3dSPL_LINEAR1D::SetKpValue)
void m3dSPL_LAGRANGE::SetKpValue(int i, void *v)
{
    kpAt(this, i)[1] = *(float *)v;
}


// =============================================================================================
// m3dSPL_BEZIER (2D / 3D)
// =============================================================================================

// v12 0x005680A0  (feb 0x005500E0)
// Cubic Bezier P0,C0,C1,P1 at t in [0,1] (clamped) for `dim` channels: value into out, d/dt into deriv
// (either may be 0).
void m3dSPL_BEZIER::GetValueDim(float t, int dimN, void *p0V, void *c0V, void *p1V, void *c1V,
                                void *outV, void *derivV)
{
    if (t < 0.0f)      t = 0.0f;
    else if (t > 1.0f) t = 1.0f;
    float u = 1.0f - t, t2 = t * t, u2 = u * u;
    float bP0 = 0, bC0 = 0, bC1 = 0, bP1 = 0;       // Bernstein weights
    float dP0 = 0, dC0 = 0, dC1 = 0, dP1 = 0;       // their derivatives
    if (outV) {
        bP0 = u2 * u;
        bP1 = t2 * t;
        bC0 = t * u2 * 3.0f;
        bC1 = t2 * u * 3.0f;
    }
    if (derivV) {
        dP0 = u2 * -3.0f;
        dP1 = t2 * 3.0f;
        dC0 = (t * 3.0f - 1.0f) * u * -3.0f;
        dC1 = (2.0f - t * 3.0f) * t * 3.0f;
    }
    float *p0 = (float *)p0V, *c0 = (float *)c0V, *p1 = (float *)p1V, *c1 = (float *)c1V;
    float *out = (float *)outV, *der = (float *)derivV;
    for (int d = 0; d < dimN; d++) {
        if (out) out[d] = c0[d] * bC0 + p0[d] * bP0 + c1[d] * bC1 + p1[d] * bP1;
        if (der) der[d] = c0[d] * dC0 + p0[d] * dP0 + c1[d] * dC1 + p1[d] * dP1;
    }
}


// v12 0x00568250  (feb 0x00550290)
// 2D Bezier path; t = key index + fraction (integer parametrisation).
void m3dSPL_BEZIER2D::GetValue(float t, void *out, void *deriv)
{
    int i = splIntFloor(t);
    t -= (float)i;
    if (i == nKp - 1) {                             // exactly on the last key
        t += 1.0f;
        i--;
    }
    float *a = kpAt(this, i), *b = kpAt(this, i + 1);
    GetValueDim(t, 2, a + 1, a + 5, b + 1, b + 3, out, deriv);   // P0, out-ctrl(a), P1, in-ctrl(b)
}


// v12 0x005682E0  (feb 0x00550320)
// Sets key i: time, point, incoming and outgoing control points.
void m3dSPL_BEZIER2D::SetKp(int i, float t, m2dV *p, m2dV *in, m2dV *outC)
{
    kpAt(this, i)[0] = t;
    *(m2dV *)(kpAt(this, i) + 1) = *p;
    *(m2dV *)(kpAt(this, i) + 3) = *in;
    *(m2dV *)(kpAt(this, i) + 5) = *outC;
}


// v12 0x00568F10 (folded with m3dSPL_LINEAR2D::GetKpValue)
void m3dSPL_BEZIER2D::GetKpValue(int i, void *out)
{
    *(m2dV *)out = *(m2dV *)(kpAt(this, i) + 1);
}


// v12 0x00568370  (feb 0x005503B0)
// 3D Bezier path; t = key index + fraction.
void m3dSPL_BEZIER3D::GetValue(float t, void *out, void *deriv)
{
    int i = splIntFloor(t);
    t -= (float)i;
    if (i == nKp - 1) {
        i = nKp - 2;
        t = 1.0f;
    }
    float *a = kpAt(this, i), *b = kpAt(this, i + 1);
    GetValueDim(t, 3, a + 1, a + 7, b + 1, b + 4, out, deriv);   // P0, out-ctrl(a), P1, in-ctrl(b)
}


// v12 0x00568400  (feb 0x00550440)
// Converts to a LINEAR3D polyline with n samples per segment, re-parametrised by arc length
// (key time = segment index + fraction of the segment length). Only type LINEAR3D is supported.
m3dSPL *m3dSPL_BEZIER3D::Convert(int newType, int n)
{
    if (newType != M3D_SPL_LINEAR3D)
        return 0;                                   // ? original returns an uninitialised register here
    int nSrc = nKp;
    m3dSPL *s = Make(M3D_SPL_LINEAR3D);
    if (!s)
        return 0;
    if (!s->AllocKpList((nSrc - 1) * n + 1)) {
        s->__vecDelDtor(1);
        return 0;
    }
    int k = 1;
    GetValue(0.0f, kpAt(s, s->nKpMax > 0 ? 0 : s->nKpMax) + 1, 0);
    for (int seg = 0; nKp != 1 && seg < nKp - 1; seg++)
        for (int j = 1; j <= n; j++, k++)
            GetValue((float)j / (float)n + (float)seg, kpAt(s, k) + 1, 0);

    if (nKp == 1 || nKp - 1 < 0)
        return s;
    int base = 0;
    for (int seg = 0; seg < nKp - 1; seg++, base += n) {
        float len = 0.0f;
        for (int j = 0; j < n; j++)
            len = m3dDist((m3dV *)(kpAt(s, base + j) + 1), (m3dV *)(kpAt(s, base + j + 1) + 1)) + len;
        kpAt(s, base)[0] = (float)seg;
        if (n > 0) {
            float inv = 1.0f / len;
            for (int j = 0; j < n; j++) {
                float d = m3dDist((m3dV *)(kpAt(s, base + j) + 1), (m3dV *)(kpAt(s, base + j + 1) + 1));
                kpAt(s, base + j + 1)[0] = inv * d + kpAt(s, base + j)[0];
            }
        }
    }
    return s;
}


// v12 0x00569070 (folded with m3dSPL_LINEAR3D::GetKpValue)
void m3dSPL_BEZIER3D::GetKpValue(int i, void *out)
{
    *(m3dV *)out = *(m3dV *)(kpAt(this, i) + 1);
}


// v12 0x005690B0  (feb 0x005510A0, inline)
// Sets the point; both control points are reset to m3dVZero (out-ctrl first, then in-ctrl = out-ctrl).
void m3dSPL_BEZIER3D::SetKpValue(int i, void *v)
{
    *(m3dV *)(kpAt(this, i) + 1) = *(m3dV *)v;
    *(m3dV *)(kpAt(this, i) + 7) = m3dVZero;
    *(m3dV *)(kpAt(this, i) + 4) = *(m3dV *)(kpAt(this, i) + 7);
}


// =============================================================================================
// m3dSPL_QUAT
// =============================================================================================

// v12 0x005687E0  (feb 0x00550830)
// Rotation track: slerp between the segment's keys.
void m3dSPL_QUAT::GetValue(float t, void *out, void *deriv)
{
    float a[20], b[20];
    int seg = FindSeg(t);
    if (seg != -2 && seg != -1) {
        GetKp(seg, a);
        GetKp(seg + 1, b);
        m3dSlerpQuat((m3dQUAT *)(a + 1), (m3dQUAT *)(b + 1), (t - a[0]) / (b[0] - a[0]), (m3dQUAT *)out);
        return;
    }
    GetKp(seg == -2 ? 0 : nKp - 1, a);
    *(m3dQUAT *)out = *(m3dQUAT *)(a + 1);
}


// v12 0x005688B0  (feb 0x00550900)
// Rotation of the key nearest in time to t.
void m3dSPL_QUAT::GetValueClosest(float t, void *out, void *deriv)
{
    float a[20], b[20];
    const float *k = a;
    int seg = FindSeg(t);
    if (seg == -2)      GetKp(0, a);
    else if (seg == -1) GetKp(nKp - 1, a);
    else {
        GetKp(seg, a);
        GetKp(seg + 1, b);
        if (b[0] - t <= t - a[0])
            k = b;
    }
    *(m3dQUAT *)out = *(m3dQUAT *)(k + 1);
}


// v12 0x00568990  (feb 0x005509E0)
// Keys equal when the rotations match within 0.003 (m3dIsEqualQuat_A).
int m3dSPL_QUAT::CompareKP(void *a, void *b)
{
    return m3dIsEqualQuat_A((m3dQUAT *)((float *)a + 1), (m3dQUAT *)((float *)b + 1), 0.003f) != 0;
}


// v12 0x00569140  (feb 0x004106E0, inline in plw_mdg.obj; ICF-shared with COLOR)
// 1.2: COMDAT now emitted with m3d_spl.obj.
void m3dSPL_QUAT::GetKpValue(int i, void *out)
{
    *(m3dQUAT *)out = *(m3dQUAT *)(kpAt(this, i) + 1);
}


// v12 0x00569180  (feb 0x00410720, inline in plw_mdg.obj; ICF-shared with COLOR)
// 1.2: COMDAT now emitted with m3d_spl.obj.
void m3dSPL_QUAT::SetKpValue(int i, void *v)
{
    *(m3dQUAT *)(kpAt(this, i) + 1) = *(m3dQUAT *)v;
}


// =============================================================================================
// m3dSPL_COLOR
// =============================================================================================

// v12 0x005689C0  (feb 0x00550A10)
// RGBA colour track, channel-wise linear.
void m3dSPL_COLOR::GetValue(float t, void *outV, void *deriv)
{
    float *out = (float *)outV;
    float a[20] = { 0 }, b[20] = { 0 };
    int seg = FindSeg(t);
    if (seg == -2 || seg == -1) {
        GetKp(seg == -2 ? 0 : nKp - 1, a);
        out[0] = a[1]; out[1] = a[2]; out[2] = a[3]; out[3] = a[4];
        return;
    }
    GetKp(seg, a);
    GetKp(seg + 1, b);
    for (int d = 1; d <= 4; d++)
        out[d - 1] = splLerp(t, a[0], a[d], b[0], b[d]);
}


// v12 0x00568D50  (feb 0x00550DA0)
// Sets key i: time and r, g, b, a.
void m3dSPL_COLOR::SetKp(int i, float t, float r, float g, float b, float a)
{
    float *k = kpAt(this, i);
    k[0] = t; k[1] = r; k[2] = g; k[3] = b; k[4] = a;
}


// v12 0x00568DE0  (feb 0x00550E30)
// Keys equal when all channels differ by < 0.003.
int m3dSPL_COLOR::CompareKP(void *av, void *bv)
{
    float *a = (float *)av, *b = (float *)bv;
    return m3dFAbs(b[1] - a[1]) < 0.003f && m3dFAbs(b[2] - a[2]) < 0.003f &&
           m3dFAbs(b[3] - a[3]) < 0.003f && m3dFAbs(b[4] - a[4]) < 0.003f;
}


// v12 0x00569140 (folded with m3dSPL_QUAT::GetKpValue)
void m3dSPL_COLOR::GetKpValue(int i, void *out)
{
    *(m3dQUAT *)out = *(m3dQUAT *)(kpAt(this, i) + 1);
}


// v12 0x00569180 (folded with m3dSPL_QUAT::SetKpValue)
void m3dSPL_COLOR::SetKpValue(int i, void *v)
{
    *(m3dQUAT *)(kpAt(this, i) + 1) = *(m3dQUAT *)v;
}
