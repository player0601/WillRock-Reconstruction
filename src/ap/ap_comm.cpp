// ap_comm.cpp -- ap.lib common part (Will Rock 1.2, v12.exe; feb obj ap:ap_comm.obj)
//
// Reconstructed from the feb decompile and the v12 disassembly (v12 0x004AF370..0x004AF5EC).
// The whole object is byte-identical between feb and 1.2 except for the moved globals.
// "// 1.2:" marks a difference from the Feb 2003 build, "// ?" what is not certain.
//
// Hook points: the four apSet*CB setters (or the callback pointers themselves) receive every engine
// message box / message / log line; apMsgBox is what every "xxx() failed" init error goes through.

#include "ap.h"

// ---------------------------------------------------------------- globals (v12 .data/.bss order)
extern "C" HWND apHMainWindow = NULL;       // v12 0x00652018 (feb 0x0060F2D8)
void      *apHInstance        = NULL;       // v12 0x0065201C (feb 0x0060F2DC)  HINSTANCE (SDK type: void*)
char       apTitle[AP_TITLE_LEN];           // v12 0x00652020 (feb 0x0060F2E0)  MessageBox caption
apMSG_CB   apLogCB            = NULL;       // v12 0x00652060 (feb 0x0060F320)
apLOG_FILE apLogFiles[AP_LOG_FILES];        // v12 0x00652068 (feb 0x0060F328)
char       apMsgBuf[AP_MSG_BUF_LEN];        // v12 0x00653F58 (feb 0x0060FD78)
apMSG_CB   apLogErrCB         = NULL;       // v12 0x00654158 (feb 0x0060FF78)
char       apBuffer[AP_BUFFER_LEN];         // v12 0x00654160 (feb 0x0060FF80)  general scratch buffer for callers
apMSG_CB   apMsgCB            = NULL;       // v12 0x00655160 (feb 0x00610F80)
apMSG_CB   apMsgBoxCB         = NULL;       // v12 0x00655164 (feb 0x00610F84)

// v12 0x004AF370  (feb 0x004A07E0)
// Library init: clear the log-file table and install the crash filter.
BOOL32 apInit()
{
    for (int i = 0; i < AP_LOG_FILES; i++) {
        apLogFiles[i].name[0] = 0;
        apLogFiles[i].fp      = NULL;
    }
    apGPF_FILTER::SetupFilter();
    return 1;
}

// v12 0x004AF3A0  (feb 0x004A07E0)
// Library shutdown: close every log file still in use.
void apTerm()
{
    for (int i = 0; i < AP_LOG_FILES; i++) {
        if (strlen(apLogFiles[i].name) != 0)
            fclose(apLogFiles[i].fp);
        apLogFiles[i].fp = NULL;
    }
}

// The four forwarders below share one body: NULL fmt => cb(NULL) (a "flush"/"close" request),
// otherwise format into apMsgBuf and pass the text. No callback => message dropped.
#define AP_FORWARD(cb, fmt)                                   \
    do {                                                      \
        if (fmt == NULL) {                                    \
            if (cb) cb(NULL);                                 \
        } else if (cb) {                                      \
            va_list va; va_start(va, fmt);                    \
            vsprintf(apMsgBuf, fmt, va);                      \
            va_end(va);                                       \
            cb(apMsgBuf);                                     \
        }                                                     \
    } while (0)

// v12 0x004AF3E0  (feb 0x004A0850)
// Error message box (through apMsgBoxCB).
void apMsgBox(char *fmt, ...)
{
    AP_FORWARD(apMsgBoxCB, fmt);
}

// v12 0x004AF430  (feb 0x004A08A0)
// Informational message (through apMsgCB).
void apMsg(char *fmt, ...)
{
    AP_FORWARD(apMsgCB, fmt);
}

// v12 0x004AF480  (feb 0x004A08F0)
// Log line (through apLogCB).
void apLog(char *fmt, ...)
{
    AP_FORWARD(apLogCB, fmt);
}

// v12 0x004AF4D0  (feb 0x004A0940)
// Error log line: sent to apLogErrCB and also copied to the normal log.
void apLogErr(char *fmt, ...)
{
    if (fmt == NULL) {
        if (apLogErrCB) apLogErrCB(NULL);   // the plain log is NOT told in the NULL case
        return;
    }
    va_list va;
    if (apLogErrCB) {
        va_start(va, fmt);
        vsprintf(apMsgBuf, fmt, va);
        va_end(va);
        apLogErrCB(apMsgBuf);
    }
    if (apLogCB) {
        va_start(va, fmt);
        vsprintf(apMsgBuf, fmt, va);
        va_end(va);
        apLogCB(apMsgBuf);
    }
}

// v12 0x004AF540  (feb 0x004A09B0)
// Install the message-box callback.
void __fastcall apSetMsgBoxCB(void (*cb)(char *, ...)) { apMsgBoxCB = cb; }

// v12 0x004AF550  (feb 0x004A09C0)
// Install the message callback.
void __fastcall apSetMsgCB(void (*cb)(char *, ...)) { apMsgCB = cb; }

// v12 0x004AF560  (feb 0x004A09D0)
// Install the log callback.
void __fastcall apSetLogCB(void (*cb)(char *, ...)) { apLogCB = cb; }

// v12 0x004AF570  (feb 0x004A09E0)
// Install the error-log callback.
void __fastcall apSetLogErrCB(void (*cb)(char *, ...)) { apLogErrCB = cb; }

// v12 0x004AF580  (feb 0x004A09F0)
// Copy a name; note it really keeps only 62 characters (str[0x3E] forced to 0).
void apNAME::SetName(char *s)
{
    strncpy(str, s, 0x3F);
    str[0x3E] = 0;
}

// v12 0x004AF5A0  (feb 0x004A0A10)
// Case-sensitive compare (inlined strcmp) == 0.
BOOL32 apNAME::IsName(char *s)
{
    return strcmp(str, s) == 0;
}
