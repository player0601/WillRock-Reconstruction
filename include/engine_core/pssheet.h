// pssheet.h -- Will Rock 1.2 unofficial SDK, engine_core: property sheets (engine:pssheet.obj)
//
// A property sheet is the engine's script / ini format: "[SECTION]" lines followed by "key = value" lines,
// CR LF separated. Used by .dsl level lists, the [ANIM_PROP] block of templates and instances, object
// scripts, the global scene script (.lg chunk 0x21E), willrock.ini-style config and the UI.
// Lookups are case-insensitive. A sheet may have a parent (+0x0C): GetInt/GetFloat/GetBool/GetHEX/GetStr/
// GetColor fall back to the parent and the parent's parent (instance sheet -> template sheet -> class).
// Values keep their text; GetStr strips one pair of quotes.
#pragma once
#include "core_types.h"

struct m3dCOLOR;

struct psVAR {                     // size 8
    char *name;                    // +0x00 apStrdup, trimmed
    char *value;                   // +0x04 apStrdup, trimmed (0 while being parsed)
};
struct psSECT {                    // size 0x0C
    char  *name;                   // +0x00
    int    nVar;                   // +0x04
    psVAR *var;                    // +0x08 apRealloc'ed array
};

class psSHEET {                    // size 0x10 (operator new(0x10) + 4 zero dwords inline, e.g. _sioReadPSheet)
public:
    int      nSect;                // +0x00
    psSECT  *sect;                 // +0x04 apRealloc'ed array
    char    *src;                  // +0x08 the source text (SetString / Load), freed after parsing if mode & 1
    psSHEET *parent;               // +0x0C fallback sheet for the Get* lookups
    static apSTATE mode;           // v12 0x00978750 (feb 0x00934508) bit 1: drop the source text after ParseString
                                   //   (set by gsAPP_SYSTEM, v12 0x0050D15B)

    psSHEET() : nSect(0), sect(0), src(0), parent(0) {}
    BOOL32 GetStr(char *sect, char *key, char *out, int max);       // v12 0x004F4430 (feb 0x004E01F0) this, then parent
    BOOL32 GetInt(char *sect, char *key, int *out);                  // v12 0x004F4490 (feb 0x004E0250) "%d"
    BOOL32 GetBool(char *sect, char *key, int *out);                 // v12 0x004F4650 (feb 0x004E0410) number or "yes..."
    BOOL32 GetFloat(char *sect, char *key, float *out);              // v12 0x004F4820 (feb 0x004E05E0) "%f"
    BOOL32 GetHEX(char *sect, char *key, u32 *out);                  // v12 0x004F49E0 (feb 0x004E07A0) "%x"
    BOOL32 GetColor(char *sect, char *key, m3dCOLOR *out);           // v12 0x004F4BA0 (feb 0x004E0960) "r,g,b[,a]" or "a,r,g,b"
    void   Empty();                                                  // v12 0x004F4BF0 (feb 0x004E09B0)
    BOOL32 SetString(char *text);                                    // v12 0x004F4C90 (feb 0x004E0A50)
    int    ParseString();                                            // v12 0x004F4CC0 (feb 0x004E0A80) 0 ok, < 0 error code
    BOOL32 CompileString();                                          // v12 0x004F5030 (feb 0x004E0DF0) sections -> src text
    psSECT *FindSect(char *name);                                    // v12 0x004F51D0 (feb 0x004E0F90) this, then parent
    psVAR  *FindVarStr(char *sect, char *part);                      // v12 0x004F5250 (feb 0x004E1010) first key containing part
    BOOL32 SetStr(char *sect, char *key, char *value);               // v12 0x004F59C0 (feb 0x004E1780) stores "\"value\""
    BOOL32 Save(char *file);                                         // v12 0x004F5A20 (feb 0x004E17E0)
    BOOL32 Load(char *file);                                         // v12 0x004F5AD0 (feb 0x004E1890)
private:
    psVAR  *AddVar(char *sect, char *key);                           // v12 0x004F5340 (feb 0x004E1100) find or add
    psSECT *_FindSect(char *name);                                   // v12 0x004F55C0 (feb 0x004E1380) this only
    psVAR  *_FindVar(char *sect, char *key);                         // v12 0x004F5610 (feb 0x004E13D0) this only
    BOOL32 _GetStr(char *sect, char *key, char *out, int max);       // v12 0x004F5690 (feb 0x004E1450) this + parent's own
    BOOL32 _GetColor(char *sect, char *key, m3dCOLOR *out);          // v12 0x004F5780 (feb 0x004E1540) this + parent's own
};
void __fastcall _psStripSpaces(char *s);                             // v12 0x004F5BE0 (feb 0x004E19A0)
