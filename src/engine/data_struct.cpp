// data_struct.cpp -- engine.lib dsDYN_ARRAY, a growable array whose storage pointer may live in the
// caller (Will Rock 1.2, v12.exe; feb obj engine:data_struct.obj). v12 0x004EF940..0x004EFB0F, all "exact".
//
// The array never owns a typed buffer: 'ptr' points at the caller's pointer variable (or at ownPtr),
// and AddEntry/Expand grow *ptr with apRealloc by at least growBy elements at a time.

#include "engine.h"

// v12 0x004EF940  (feb 0x004DBFE0)
dsDYN_ARRAY::dsDYN_ARRAY(int size, int grow, void **p)
{
    elemSize = size;
    growBy   = grow;
    flags    = 0;
    count    = 0;
    capacity = 0;
    ownPtr   = 0;
    flags    = 1;
    ptr      = p ? p : &ownPtr;
}

// v12 0x004EF990  (feb 0x004DC030)
dsDYN_ARRAY::~dsDYN_ARRAY()
{
    if (flags & 1)
        ReleaseMem();
}

// v12 0x004EF9C0  (feb 0x004DC060)
BOOL32 dsDYN_ARRAY::Init(int size, int grow, void **p, int n, int cap)
{
    elemSize = size;
    count    = n;
    capacity = cap;
    growBy   = grow;
    ownPtr   = 0;
    ptr      = p ? p : &ownPtr;
    return 1;
}

// v12 0x004EFA10  (feb 0x004DC0B0)  also dsDYN_ARRAY::Term (identical code folded)
// NB: clears 'ptr' itself, so the array is unusable until the next Init.
void dsDYN_ARRAY::ReleaseMem()
{
    apFree(*ptr);
    ptr      = 0;
    count    = 0;
    elemSize = -1;
    capacity = 0;
    growBy   = -1;
    ownPtr   = 0;
}

// v12 0x004EFA40  (feb 0x004DC0E0)
// Make room for n more elements and count them.
BOOL32 dsDYN_ARRAY::AddEntry(int n)
{
    if (!Expand(n))
        return 0;
    count += n;
    return 1;
}

// v12 0x004EFAB0  (feb 0x004DC150)
BOOL32 dsDYN_ARRAY::Expand(int n)
{
    if (capacity < count + n) {
        int need = count - capacity + n;
        int grow = growBy > need ? growBy : need;
        void *q = apRealloc(*ptr, (capacity + grow) * elemSize);
        if (!q)
            return 0;
        *ptr = q;
        capacity += grow;
    }
    return 1;
}
