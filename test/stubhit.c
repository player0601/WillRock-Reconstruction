/* stubhit.c -- target of the generated stubs (tools/gen_stubs.py): a function that is not rebuilt yet was called. */
#include <windows.h>
#include <stdio.h>

void wrStubHit(const char *name)
{
    char b[600];
    _snprintf(b, sizeof(b) - 1, "Not rebuilt yet: %s", name);
    b[sizeof(b) - 1] = 0;
    fprintf(stderr, "%s\n", b);
    fflush(stderr);
    MessageBoxA(0, b, "Will Rock reconstruction", MB_OK | MB_ICONSTOP);
    ExitProcess(2);
}
