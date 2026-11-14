// cgm_search.cpp -- Will Rock 1.2 (v12.exe): cgmSEARCH sorted index lists with binary search
//
// Reconstructed from the Feb 2003 build (m3d:cgm_search.obj); every function is byte-identical in 1.2.
// v12 range 0x0057E340 .. 0x0057E75F (feb 0x00565A20 .. 0x00565E3F).

#include "cgm.h"
#include <stdlib.h>

M3D_DECLARE_EPS_SQ();   // _$E2: v12 0x0057E740 (feb 0x00565E20) -> v12 0x009AED88 (feb 0x00963A38)

static cgmSEARCH *_cgmSearchCur;   // v12 0x009AED8C (feb 0x00963A3C) list being sorted by QSortCB

static inline void *_cgmItemKey(cgmSEARCH *s, int i) { return s->data + s->keyOffset + i * s->stride; }

// _m3dNormalize: the inline of m3d_lib.h


// v12 0x0057E340  (feb 0x00565A20)
// Builds the index 0..n-1 (allocated unless 'index' is given) and sorts it with CmpItem.
BOOL32 cgmSEARCH::Init(int nItem, int itemStride, void *items, int *idx)
{
    data   = (char *)items;
    stride = itemStride;
    n      = nItem;
    if (!idx) {
        index = (int *)operator new(nItem * 4);
        if (!index)
            return 0;
        flags |= CGM_SEARCH_OWN_INDEX;
    } else {
        index = idx;
    }
    for (int i = 0; i < n; i++)
        index[i] = i;
    _cgmSearchCur = this;
    qsort(index, n, 4, QSortCB);
    return 1;
}


// v12 0x0057E3D0  (feb 0x00565AB0)
// Binary search for 'key'; narrows [lo, hi] until fewer than 3 positions remain and returns lo
// (mode -1, first not-less) or hi (mode 1). Returns a position in index[], not an item number.
// ? any other mode never narrows (endless loop) -- as compiled.
int cgmSEARCH::Search(void *key, int mode)
{
    int lo = 0, hi = n - 1;
    int span = hi;
    for (;;) {
        if (span < 3)
            return mode != CGM_SEARCH_LOWER ? hi : lo;
        int mid = (hi + lo) / 2;
        int c = CmpItem(_cgmItemKey(this, index[mid]), key);
        int nhi;
        if (mode == CGM_SEARCH_LOWER) {
            nhi = mid;
            if (c == -1) { nhi = hi; lo = mid; }
        } else {
            nhi = hi;
            if (mode == CGM_SEARCH_UPPER) {
                nhi = mid;
                if (c != 1) { nhi = hi; lo = mid; }
            }
        }
        span = nhi - lo;
        hi = nhi;
    }
}


// v12 0x0057E450  (feb 0x00565B30)
// Frees an owned index array and resets the list.
void cgmSEARCH::Term()
{
    if (flags & CGM_SEARCH_OWN_INDEX)
        operator delete(index);
    stride = -1;
    index  = 0;
    n      = 0;
    data   = 0;
}


// v12 0x0057E480  (feb 0x00565B60)
// Vector list: the sort axis is the normalised diagonal of the bounding box of all keys (Y if fewer
// than 2 items or degenerate); sorting runs with tolerance 0, the caller's tolerance is restored after.
BOOL32 cgmSEARCH_VLIST::Init(int nItem, int itemStride, void *items, int *idx)
{
    m3dBOX box;                                      // stack m3dBOX (vtable + type 1)
    box.type = M3D_VOL_BOX;
    if (dim == -1)
        dim = (unsigned)itemStride >> 2;
    if (nItem < 2) {
        axis = m3dVUnitY;
    } else {
        float v[3] = { m3dVZero.x, m3dVZero.y, m3dVZero.z };
        box.CalcStart();
        for (int i = 0; i < nItem; i++) {
            const float *src = (const float *)((char *)items + keyOffset + i * itemStride);
            for (int k = 0; k < dim; k++)
                v[k] = src[k];
            box.CalcUpdate((m3dV *)v);
        }
        axis.x = box.vMax.x - box.vMin.x;
        axis.y = box.vMax.y - box.vMin.y;
        axis.z = box.vMax.z - box.vMin.z;
        if (!_m3dNormalize(&axis))
            axis = m3dVUnitY;
    }
    float t = tol;
    tol = 0.0f;
    if (cgmSEARCH::Init(nItem, itemStride, items, idx)) {
        tol = t;
        return 1;
    }
    return 0;
}


// v12 0x0057E5D0  (feb 0x00565CB0)
// cgmSEARCH::Term (inlined) + clears tolerance and axis.
void cgmSEARCH_VLIST::Term()
{
    if (flags & CGM_SEARCH_OWN_INDEX)
        operator delete(index);
    stride = -1;
    index  = 0;
    n      = 0;
    data   = 0;
    tol    = 0.0f;
    axis   = m3dVZero;
}


// v12 0x0057E620  (feb 0x00565D00)
// Compares the projections of two keys on the sort axis: -1 / 0 (within tol) / 1.
int cgmSEARCH_VLIST::CmpItem(void *a, void *b)
{
    const float *pa = (const float *)a, *pb = (const float *)b, *ax = &axis.x;
    float da = 0.0f, db = 0.0f;
    for (int i = 0; i < dim; i++) {
        da = pa[i] * ax[i] + da;
        db = ax[i] * pb[i] + db;
    }
    if (db - tol <= da) {
        if (da <= db + tol)
            return 0;
        return 1;
    }
    return -1;
}


// v12 0x0057E6A0  (feb 0x00565D80)
// Signed 8/16/32-bit integer compare. ? For any other key size the POINTER value of a is compared.
int cgmSEARCH_INT_LIST::CmpItem(void *a, void *b)
{
    int d;
    if (size == 1)
        d = *(signed char *)a - *(signed char *)b;
    else if (size == 2)
        d = *(short *)a - *(short *)b;
    else if (size == 4)
        d = *(int *)a - *(int *)b;
    else
        d = (int)(long)a;                            // sic
    if (d < 0)
        return -1;
    return d > 0;
}


// v12 0x0057E700  (feb 0x00565DE0)
// qsort callback: compares the keys of two index entries of the list being initialised.
int __cdecl cgmSEARCH::QSortCB(const void *a, const void *b)
{
    cgmSEARCH *s = _cgmSearchCur;
    return s->CmpItem(_cgmItemKey(s, *(const int *)a), _cgmItemKey(s, *(const int *)b));
}
