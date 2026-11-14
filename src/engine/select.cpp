// select.cpp -- engine.lib named selections (Will Rock 1.2, v12.exe; feb obj engine:select.obj)
//
// v12 0x004F4270..0x004F441F, all "exact". A selection is a list of object names read from the level
// (.lg chunk 0x21D); Prepare sorts it so Find can binary-search.

#include "engine.h"

// v12 0x004F4270  (feb 0x004E0030)
BOOL32 selSEL::Init(int count)
{
    n = count;
    selOBJ *a = (selOBJ *)operator new(count * sizeof(selOBJ));
    if (a)
        for (int i = 0; i < count; i++) {
            a[i].id = 0;
            a[i].name[0] = 0;
        }
    obj = a;
    return a != 0;
}

// v12 0x004F42D0  (feb 0x004E0090)
int selSEL::Find(char *s)
{
    if (!(flags & 1)) {
        for (int i = 0; i < n; i++)
            if (((apNAME *)obj[i].name)->IsName(s))
                return i;
        return -1;
    }
    selOBJ *r = (selOBJ *)bsearch(s, obj, n, sizeof(selOBJ), _selSearchSelObj);
    if (r)
        return (int)(r - obj);
    return -1;
}

// v12 0x004F4370  (feb 0x004E0130)
void selSEL::Prepare()
{
    flags |= 1;
    qsort(obj, n, sizeof(selOBJ), _selCompSelObj);
}

// v12 0x004F4390  (feb 0x004E0150)
int __cdecl _selCompSelObj(const void *a, const void *b)
{
    return strcmp(((const selOBJ *)a)->name, ((const selOBJ *)b)->name);
}

// v12 0x004F43E0  (feb 0x004E01A0)
int __cdecl _selSearchSelObj(const void *key, const void *b)
{
    return strcmp((const char *)key, ((const selOBJ *)b)->name);
}
