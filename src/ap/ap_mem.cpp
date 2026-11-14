// ap_mem.cpp -- ap.lib memory wrappers (Will Rock 1.2, v12.exe; feb obj ap:ap_mem.obj)
//
// v12 0x004AF5F0..0x004AF66x, identical to feb. In the release build every wrapper is a plain jump into the
// statically linked MSVC CRT heap; the debug build counted bytes in _apTotalAlloced, which is still
// allocated but never touched. Hook point: replace these four to put the whole engine on another heap
// (all engine containers, arrVector included, allocate through them).

#include "ap.h"

int _apTotalAlloced = 0;                 // v12 0x00655168 ? (feb 0x00610F88) unused in release

// v12 0x004AF5F0  (feb 0x004A0A60)
// malloc.
void *apMalloc(size_t n) { return malloc(n); }

// v12 0x004AF600  (feb 0x004A0A70)
// calloc.
void *apCalloc(size_t n, size_t sz) { return calloc(n, sz); }

// v12 0x004AF610  (feb 0x004A0A80)
// free.
void apFree(void *p) { free(p); }

// v12 0x004AF620  (feb 0x004A0A90)
// realloc (realloc(NULL,n) is how arrVector allocates).
void *apRealloc(void *p, size_t n) { return realloc(p, n); }

// v12 0x004AF630  (feb 0x004A0AA0)
// strdup on the CRT heap (calls malloc directly, not apMalloc); NULL if out of memory.
char *apStrdup(char *s)
{
    char *d = (char *)malloc(strlen(s) + 1);
    if (d)
        strcpy(d, s);
    return d;
}
