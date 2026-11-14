// ap_gpf.cpp -- ap.lib crash handler (Will Rock 1.2, v12.exe; feb obj ap:ap_gpf.obj)
//
// v12 0x004B0630..0x004B0962. SetupFilter (called from apInit) installs GPFExeptFunc as the unhandled
// exception filter. On an access violation it appends a report to <exe dir>\<pFilter->GetFileName()>
// (WillRock_GPF.log, the game's wrGPF_FILTER) or <exe dir>\ap_gpf.log, then returns 0
// (EXCEPTION_CONTINUE_SEARCH: the normal Windows crash box follows). Other exception codes are ignored.
//
// 1.2: the report now also dumps the CPU registers from the CONTEXT ("Registers:" block).
// 1.2: the build date string is "Jun 16 2003" (__DATE__ of the ap.lib build shipped in 1.0/1.2), feb "Feb 21 2003".
//
// Hook points: apGPF_FILTER::pFilter (set your own object to change the file name and append extra info
// through slot +4), or call GPFExeptFunc(NULL) to write a "Record (no-crash)" snapshot at any time.

#include "ap.h"

apGPF_FILTER *apGPF_FILTER::pFilter    = NULL;   // v12 0x00656180 (feb 0x00611FA0)
int           apGPF_FILTER::isInFilter = 0;      // v12 0x00656184 (feb 0x00611FA4) re-entrancy guard
MEMORYSTATUS  apGpfStartupMem;                   // v12 0x00656188 (feb 0x00611FA8)

#define AP_BUILD_DATE "Jun 16 2003"              // 1.2: feb "Feb 21 2003"
#define MB (1000000)                             // the report's "megabytes"

// v12 0x004B0630  (feb 0x004A1A80)
// Install the exception filter and remember the memory status at start-up.
BOOL32 apGPF_FILTER::SetupFilter()
{
    SetUnhandledExceptionFilter(GPFExeptFunc);
    GlobalMemoryStatus(&apGpfStartupMem);
    return 1;
}

// v12 0x004B0650  (feb 0x004A1AA0)
// Write the crash report (e == NULL: write a snapshot without exception info).
long __stdcall apGPF_FILTER::GPFExeptFunc(_EXCEPTION_POINTERS *e)
{
    char         path[0x400];
    DWORD        nameLen = 0x10;
    time_t       now;
    MEMORYSTATUS ms;

    if (isInFilter == 0) {
        isInFilter = 1;
        if ((e == NULL || e->ExceptionRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION) &&
            GetModuleFileNameA((HMODULE)apHInstance, path, 1000)) {
            char *slash = strrchr(path, '\\');
            if (slash)
                slash[1] = 0;
            if (pFilter)
                strcat(path, pFilter->GetFileName());
            else
                strcat(path, "ap_gpf.log");

            FILE *f = fopen(path, "at");
            if (f) {
                fprintf(f, "\n\n---------------------------------------------------------------\n");
                if (e)
                    fprintf(f, "GPF occured at 0x%08X\n", e->ExceptionRecord->ExceptionAddress);
                time(&now);
                if (e)
                    fprintf(f, "Build Date %s;  Crash Date: %s", AP_BUILD_DATE, ctime(&now));
                else
                    fprintf(f, "Build Date %s;  Record (no-crash) Date: %s", AP_BUILD_DATE, ctime(&now));

                GetComputerNameA(path, &nameLen);
                fprintf(f, "Computer Name: %s\n", path);

                GlobalMemoryStatus(&ms);
                fprintf(f, "Cur     Mem: Phisical: %d (Avail %d),    Swap: %d (Avail %d),  \n",
                        ms.dwTotalPhys / MB, ms.dwAvailPhys / MB, ms.dwTotalPageFile / MB, ms.dwAvailPageFile / MB);
                fprintf(f, "Startup Mem: Phisical: %d (Avail %d),    Swap: %d (Avail %d),  \n",
                        apGpfStartupMem.dwTotalPhys / MB, apGpfStartupMem.dwAvailPhys / MB,
                        apGpfStartupMem.dwTotalPageFile / MB, apGpfStartupMem.dwAvailPageFile / MB);

                // 1.2: register dump (new)
                if (e && e->ContextRecord) {
                    CONTEXT *c = e->ContextRecord;
                    fprintf(f, "Registers:\n");
                    fprintf(f, "EAX = 0x%08X    EBX = 0x%08X    ECX = 0x%08X    EDX = 0x%08X\n",
                            c->Eax, c->Ebx, c->Ecx, c->Edx);
                    // the label says EIP but the value printed is Esp (CONTEXT +0xC4), as in the shipped code
                    fprintf(f, "ESI = 0x%08X    EDI = 0x%08X    EBP = 0x%08X    EIP = 0x%08X\n",
                            c->Esi, c->Edi, c->Ebp, c->Esp);
                    fprintf(f, "GS = 0x%04X    FS = 0x%04X    ES = 0x%04X    DS = 0x%04X    CS = 0x%04X     SS = 0x%04X\n",
                            c->SegGs, c->SegFs, c->SegEs, c->SegDs, c->SegCs, c->SegSs);
                }
                fflush(f);
                if (pFilter)
                    pFilter->DumpInfo(f, path);  // slot +4: game appends its own info (video driver...)
                fclose(f);
            }
        }
    }
    isInFilter = 0;
    return EXCEPTION_CONTINUE_SEARCH;
}
