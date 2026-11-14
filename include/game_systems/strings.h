// strings.h -- Will Rock 1.2 unofficial SDK, game_systems: localisation table gsSTRINGS (gs:gs_strings)
// Data file: INI [..] fileTexts (default "..\texts\strings.str", written back by wrSTRINGS::Init v12 0x004AF300),
//   UTF-16 text, one "ID<tab/space>text" record per line, ';' = comment, escapes via ReadEscChar (\n, \t, \\ ..).
// Lookup: GetStringId(ascii id) -> index (binary search on the sorted Record table); GetStringById(i) -> wchar_t*.
// Printf/PrintfV: game format strings with %s/%d/%f inserted into UTF-16.
// Assumes arrVector<T> from core: { T *data; int count; int capacity; } (12 bytes).
#pragma once
#include "gs_common.h"

class gsSTRINGS {                  // NO vtable. size v12 0x30 (feb 0x34); object = wrSTRINGS; global gsStrings v12 0x009946DC (feb 0x0094DE5C)
public:
    struct Record { int idOfs; int textOfs; };   // 8 bytes: offsets into idChars / text  (feb decompile: *(+4 + i*8))
    arrVector<char>           idChars;  // +0x00 v12 (feb +0x00) ascii ids        verified v12 ctor 0x0051C4AF
    arrVector<wrWCHAR> text;      // +0x0C v12 (feb +0x0C) UTF-16 strings   verified lea [esi+0xC] v12 0x0051C4A9
    arrVector<void*>          strPtrs;  // +0x18 v12 (feb +0x18) id -> wchar_t*   verified lea [esi+0x18]
    arrVector<Record>         records;  // +0x24 v12 (feb +0x24) sorted by id     verified lea [esi+0x24]
    // feb only: int needSort at +0x30 (=1 in ctor). 1.2 removed it (ctor v12 0x0051C4A0 has no store to +0x30).

    gsSTRINGS();                                          // v12 0x0051C4A0 (feb 0x00507030) adds the empty string as id 0
    int  AddString(const char *id, const wrWCHAR *s);  // v12 0x0051C550 (feb 0x005070F0) HOOK: inject/override texts
    const wrWCHAR *Printf(int id, ...);            // v12 0x0051C740 (feb 0x005072B0)
    const wrWCHAR *Printf(const char *id, ...);    // v12 0x0051C760 (feb 0x005072D0)
    const wrWCHAR *PrintfV(const wrWCHAR *fmt, char *va); // v12 0x0051C840 (feb 0x005073C0)
    int  GetStringId(const char *id);                     // v12 0x0051CF20 (feb 0x00507AA0) logs "GS_STRINGS: Can't find string"
    int  GetStringIdFmt(const char *fmt, ...);            // v12 0x0051CFD0 (feb 0x00507C20)
    const wrWCHAR *GetStringById(int i);           // v12 0x0051D120 (feb 0x00507DA0) HOOK: every UI/hud text read
    void GetStringById(int i, wrWCHAR *out, int n);// v12 0x0051D140 (feb 0x00507DC0)
    void AsciiToUnicode(const char*, wrWCHAR*, int);// v12 0x0051D180 (feb 0x00507E00) plain byte widening (no code page)
    const wrWCHAR *AsciiToUnicode(const char*);    // v12 0x0051D1C0 (feb 0x00507E40)
    int  UnicodeToAscii(const wrWCHAR*, char*, int);// v12 0x0051D200 (feb 0x00507E80) chars >= 0x80 dropped/replaced
    int  LoadStringsTable(const char *file);              // v12 0x0051D730 (feb 0x00508420) fioFILE_DISK read, ParseTableLine per line
protected:
    int  ParseTableLine(wrWCHAR *line);            // v12 0x0051D250 (feb 0x00507ED0)
    int  ReadEscChar(wrWCHAR **p, wrWCHAR*);// v12 0x0051D6B0 (feb 0x005083A0)
    void PrintfInsertStringA(wrWCHAR**, wrWCHAR*, const char*);       // v12 0x0051CD80
    void PrintfInsertFloat(wrWCHAR**, wrWCHAR*, float, int, wrWCHAR, int); // v12 0x0051CDC0
};
extern gsSTRINGS *gsStrings;   // v12 0x009946DC (feb 0x0094DE5C)
