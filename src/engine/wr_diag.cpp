// wr_diag.cpp -- reconstruction support (not an original obj): what a not-yet-rebuilt function does.
#include <windows.h>
#include <stdio.h>

extern "C" void wrNotRebuilt(const char *fn, unsigned v12addr)
{
    char msg[512];
    _snprintf(msg, sizeof(msg), "Not reconstructed yet:\n%s\n(original: WillRock.exe 1.2 at 0x%08X)", fn, v12addr);
    msg[sizeof(msg) - 1] = 0;
    OutputDebugStringA(msg);
    fprintf(stderr, "%s\n", msg);
    MessageBoxA(0, msg, "Will Rock reconstruction", MB_ICONSTOP);
    ExitProcess(3);
}
