// pssheet.cpp -- engine.lib property sheets ("[SECTION]" / "key = value" scripts)
// (Will Rock 1.2, v12.exe; feb obj engine:pssheet.obj)
//
// v12 0x004F4420..0x004F5C4F, all "exact" except GetInt/GetFloat (callgraph: same code, the inlined lookups
// are laid out differently). GetHEX has no listed twin; its 1.2 copy is at 0x004F49E0 (same code, "%x").
// The original's quirks are kept: the first-line loop of ParseString tests the rest of the text instead of
// the line, a header on a last line without CR is dropped, AddVar can return the parent's variable (so
// SetStr then writes into the parent), _psStripSpaces keeps as many trailing spaces as it removed leading ones.

#include "engine.h"

apSTATE psSHEET::mode;                 // v12 0x00978750 (feb 0x00934508); _$E8 v12 0x004F4420 sets 0

// One-level lookup as inlined everywhere: the first section of that name in s, then the key in it.
static inline psVAR *psFind(psSHEET *s, char *sect, char *key)
{
    for (int i = 0; i < s->nSect; i++) {
        psSECT *c = &s->sect[i];
        if (!stricmp(c->name, sect)) {
            for (int k = 0; k < c->nVar; k++)
                if (!stricmp(c->var[k].name, key))
                    return &c->var[k];
            return 0;
        }
    }
    return 0;
}
// s's own sections, then its parent's own sections (_FindSect).
static inline psVAR *psFind2(psSHEET *s, char *sect, char *key)
{
    psVAR *v = psFind(s, sect, key);
    if (v || !s->parent)
        return v;
    return psFind(s->parent, sect, key);
}
// The trim inlined in ParseString / AddVar: same algorithm as _psStripSpaces.
static inline void psTrim(char *s)
{
    int lead = 0;
    for (char *p = s; *p && (*p == ' ' || *p == '\t'); p++)
        lead++;
    int n = (int)strlen(s);
    char *p = s + n - 1;
    while (*p == ' ' || (*p == '\t' && p >= s)) {
        n--;
        p--;
    }
    int i = 0;
    for (; i < n; i++)
        s[i] = s[i + lead];
    s[i] = 0;
}

// v12 0x004F4430  (feb 0x004E01F0)
BOOL32 psSHEET::GetStr(char *s, char *key, char *out, int max)
{
    if (_GetStr(s, key, out, max))
        return 1;
    if (!parent)
        return 0;
    return parent->_GetStr(s, key, out, max);
}

// v12 0x004F4490  (feb 0x004E0250)
BOOL32 psSHEET::GetInt(char *s, char *key, int *out)
{
    psVAR *v = psFind2(this, s, key);
    if (v && sscanf(v->value, "%d", out) == 1)
        return 1;
    if (!parent)
        return 0;
    v = psFind2(parent, s, key);
    if (!v)
        return 0;
    return sscanf(v->value, "%d", out) == 1;
}

// v12 0x004F4650  (feb 0x004E0410)
// A number, or text starting with "yes" (1; anything else 0). 1 when the key exists.
BOOL32 psSHEET::GetBool(char *s, char *key, int *out)
{
    psVAR *v = psFind2(this, s, key);
    if (!v) {
        if (!parent)
            return 0;
        v = psFind2(parent, s, key);
        if (!v)
            return 0;
    }
    if (sscanf(v->value, "%d", out) == 1)
        return 1;
    *out = 0;
    if (!strnicmp(v->value, "yes", 3))
        *out = 1;
    return 1;
}

// v12 0x004F4820  (feb 0x004E05E0)
BOOL32 psSHEET::GetFloat(char *s, char *key, float *out)
{
    psVAR *v = psFind2(this, s, key);
    if (v && sscanf(v->value, "%f", out) == 1)
        return 1;
    if (!parent)
        return 0;
    v = psFind2(parent, s, key);
    if (!v)
        return 0;
    return sscanf(v->value, "%f", out) == 1;
}

// v12 0x004F49E0  (feb 0x004E07A0)
BOOL32 psSHEET::GetHEX(char *s, char *key, u32 *out)
{
    psVAR *v = psFind2(this, s, key);
    if (v && sscanf(v->value, "%x", out) == 1)
        return 1;
    if (!parent)
        return 0;
    v = psFind2(parent, s, key);
    if (!v)
        return 0;
    return sscanf(v->value, "%x", out) == 1;
}

// v12 0x004F4BA0  (feb 0x004E0960)
BOOL32 psSHEET::GetColor(char *s, char *key, m3dCOLOR *out)
{
    if (_GetColor(s, key, out))
        return 1;
    if (!parent)
        return 0;
    return parent->_GetColor(s, key, out);
}

// v12 0x004F4BF0  (feb 0x004E09B0)
void psSHEET::Empty()
{
    for (int i = 0; i < nSect; i++) {
        for (int k = 0; k < sect[i].nVar; k++) {
            apFree(sect[i].var[k].name);
            apFree(sect[i].var[k].value);
        }
        apFree(sect[i].var);
        apFree(sect[i].name);
    }
    apFree(sect);
    sect = 0;
    nSect = 0;
    apFree(src);
    src = 0;
}

// v12 0x004F4C90  (feb 0x004E0A50)
BOOL32 psSHEET::SetString(char *text)
{
    Empty();
    src = apStrdup(text);
    return src != 0;
}

// Next CR LF line of p into line (stripped); returns the text after it, 0 at the end of the text.
static inline char *psGetLine(char *p, char *line)
{
    char *cr = strchr(p, '\r');
    if (!cr) {
        strcpy(line, p);
        return 0;
    }
    strncpy(line, p, cr - p + 1);
    line[cr - p] = 0;
    _psStripSpaces(line);
    return cr + 2;
}

// v12 0x004F4CC0  (feb 0x004E0A80)
// Parse src into sections. Returns 0 (or -5 when a non-empty line has no '='), -1 text before the first
// "[", -2 empty section name, -3 out of memory. A key's first definition wins.
int psSHEET::ParseString()
{
    char secName[0x100];
    char line[0x1000];
    char *p = src;
    if (!p)
        return 0;
    do {                                                // (tests the rest of the text, not the line)
        if (!p)
            return 0;
        char *cr = strchr(p, '\r');
        if (!cr) {
            strcpy(line, p);
            return 0;
        }
        strncpy(line, p, cr - p + 1);
        line[cr - p] = 0;
        _psStripSpaces(line);
        p = cr + 2;
    } while (!strlen(p));
    int res = 0;
    for (;;) {
        if (!p) {
            if (mode.bits & 1) {
                apFree(src);
                src = 0;
            }
            return res;
        }
        if (line[0] != '[')
            return -1;
        char *t = strtok(line + 1, "]");
        if (!t)
            return -2;
        strcpy(secName, t);
        psTrim(secName);
        for (;;) {
            if (!p) {
                p = 0;
                break;
            }
            char *q = psGetLine(p, line);
            if (line[0] == '[') {                       // next section: read its header again from p
                char *cr = strchr(p, '\r');
                if (cr) {
                    strncpy(line, p, cr - p + 1);
                    line[cr - p] = 0;
                    _psStripSpaces(line);
                    p = cr + 2;
                } else {
                    strcpy(line, p);
                    p = 0;
                }
                break;
            }
            p = q;
            if (!strlen(line))
                continue;
            char *eq = strchr(line, '=');
            if (!eq) {
                res = -5;
                continue;
            }
            *eq = 0;
            psVAR *v = AddVar(secName, line);
            if (!v)
                return -3;
            if (v->value)
                continue;
            v->value = apStrdup(eq + 1);
            if (!v->value)
                return -3;
            psTrim(v->value);
        }
    }
}

// v12 0x004F5030  (feb 0x004E0DF0)
// Rebuild src from the sections ("[name]\r\n" and "key = value\r\n" lines).
BOOL32 psSHEET::CompileString()
{
    u32 size = 0;
    for (int i = 0; i < nSect; i++) {
        size += (u32)strlen(sect[i].name) + 1 + 4;
        for (int k = 0; k < sect[i].nVar; k++)
            size += (u32)strlen(sect[i].var[k].name) + 1 + (u32)strlen(sect[i].var[k].value) + 1 + 3;
    }
    char *p = (char *)apRealloc(src, size);
    src = p;
    if (!p)
        return 0;
    for (int i = 0; i < nSect; i++) {
        sprintf(p, "[%s]\r\n", sect[i].name);
        p += strlen(sect[i].name) + 4;
        for (int k = 0; k < sect[i].nVar; k++) {
            sprintf(p, "%s = %s\r\n", sect[i].var[k].name, sect[i].var[k].value);
            p += strlen(sect[i].var[k].value) + 1 + 3 + strlen(sect[i].var[k].name) + 1;
        }
    }
    *p = 0;
    return 1;
}

// v12 0x004F51D0  (feb 0x004E0F90)
psSECT *psSHEET::FindSect(char *name)
{
    for (int i = 0; i < nSect; i++)
        if (!stricmp(sect[i].name, name))
            return &sect[i];
    if (parent)
        for (int i = 0; i < parent->nSect; i++)
            if (!stricmp(parent->sect[i].name, name))
                return &parent->sect[i];
    return 0;
}

// v12 0x004F5250  (feb 0x004E1010)
// First key of the section that contains 'part' (this, then the parent).
psVAR *psSHEET::FindVarStr(char *s, char *part)
{
    for (int i = 0; i < nSect; i++) {
        psSECT *c = &sect[i];
        if (!stricmp(c->name, s)) {
            for (int k = 0; k < c->nVar; k++)
                if (strstr(c->var[k].name, part))
                    return &c->var[k];
            break;
        }
    }
    if (!parent)
        return 0;
    for (int i = 0; i < parent->nSect; i++) {
        psSECT *c = &parent->sect[i];
        if (!stricmp(c->name, s)) {
            for (int k = 0; k < c->nVar; k++)
                if (strstr(c->var[k].name, part))
                    return &c->var[k];
            return 0;
        }
    }
    return 0;
}

// v12 0x004F5340  (feb 0x004E1100)
// The variable key of section s: found in this or the parent's own sections, else added to this.
psVAR *psSHEET::AddVar(char *s, char *key)
{
    psSECT *c = 0;
    for (int i = 0; i < nSect; i++)
        if (!stricmp(sect[i].name, s)) {
            c = &sect[i];
            break;
        }
    if (!c) {
        psSECT *a = (psSECT *)apRealloc(sect, (nSect + 1) * sizeof(psSECT));
        sect = a;
        if (!a)
            return 0;
        c = &a[nSect++];
        c->name = 0;
        c->nVar = 0;
        c->var = 0;
        c->name = apStrdup(s);
        if (!c->name)
            return 0;
        psTrim(c->name);
    }
    psVAR *v = psFind(this, s, key);
    if (v)
        return v;
    if (parent) {
        v = psFind(parent, s, key);
        if (v)
            return v;
    }
    psVAR *a = (psVAR *)apRealloc(c->var, c->nVar * sizeof(psVAR) + sizeof(psVAR));
    c->var = a;
    if (!a)
        return 0;
    v = &a[c->nVar++];
    v->name = 0;
    v->value = 0;
    v->name = apStrdup(key);
    if (!v->name)
        return 0;
    psTrim(v->name);
    return v;
}

// v12 0x004F55C0  (feb 0x004E1380)
psSECT *psSHEET::_FindSect(char *name)
{
    for (int i = 0; i < nSect; i++)
        if (!stricmp(sect[i].name, name))
            return &sect[i];
    return 0;
}

// v12 0x004F5610  (feb 0x004E13D0)
psVAR *psSHEET::_FindVar(char *s, char *key)
{
    return psFind(this, s, key);
}

// v12 0x004F5690  (feb 0x004E1450)
// Copy the value (without one pair of surrounding quotes, at most max chars) into out.
BOOL32 psSHEET::_GetStr(char *s, char *key, char *out, int max)
{
    psVAR *v = psFind(this, s, key);
    if (!v) {
        if (!parent)
            return 0;
        v = parent->_FindVar(s, key);
        if (!v)
            return 0;
    }
    char *t = v->value;
    int n = (int)strlen(t);
    if (t[n - 1] == '"')
        n--;
    n -= (*t == '"');
    strncpy(out, t + (*t == '"'), (n <= max ? n : max) + 1);
    out[max < n ? max : n] = 0;
    return 1;
}

// v12 0x004F5780  (feb 0x004E1540)
// "r,g,b" (alpha 255) or "a,r,g,b"; fewer than three numbers fail.
BOOL32 psSHEET::_GetColor(char *s, char *key, m3dCOLOR *out)
{
    float c[4] = { 0, 0, 0, 0 };                        // second, third, fourth, first
    psVAR *v = psFind(this, s, key);
    if (!v) {
        if (!parent)
            return 0;
        v = psFind(parent, s, key);
        if (!v)
            return 0;
    }
    if (sscanf(v->value, "%f", &c[3]) != 1)
        return 0;
    char *p = strchr(v->value, ',');
    if (!p || sscanf(p + 1, "%f", &c[0]) != 1)
        return 0;
    p = strchr(p + 1, ',');
    if (!p || sscanf(p + 1, "%f", &c[1]) != 1)
        return 0;
    float *o = (float *)out;
    p = strchr(p + 1, ',');
    if (!p) {
        o[0] = c[3];
        o[3] = 255.0f;
        o[1] = c[0];
        o[2] = c[1];
        return 1;
    }
    if (sscanf(p + 1, "%f", &c[2]) != 1)
        return 0;
    o[0] = c[0];
    o[1] = c[1];
    o[2] = c[2];
    o[3] = c[3];
    return 1;
}

// v12 0x004F59C0  (feb 0x004E1780)
// Set key to "\"value\"" (adds the key if needed).
BOOL32 psSHEET::SetStr(char *s, char *key, char *value)
{
    psVAR *v = AddVar(s, key);
    if (!v)
        return 0;
    v->value = (char *)apRealloc(v->value, (int)strlen(value) + 3);
    if (!v->value)
        return 0;
    sprintf(v->value, "\"%s\"", value);
    return 1;
}

// v12 0x004F5A20  (feb 0x004E17E0)
BOOL32 psSHEET::Save(char *file)
{
    fioFILE_DISK f;
    f.flags = 0; f.name[0] = 0; f.pos = 0; f.size = 0; f.nChunk = 0; f.capChunk = 0; f.chunkTbl = 0; f.fp = 0;
    if (!CompileString())
        return 0;
    if (!f.fioFILE_DISK::Init(file, 2))
        return 0;
    f.WriteData(src, (int)strlen(src), 1);
    return f.fioFILE_DISK::Term();
}

// v12 0x004F5AD0  (feb 0x004E1890)
// (On a parse error the file is left open, as in the original.)
BOOL32 psSHEET::Load(char *file)
{
    fioFILE_DISK f;
    f.flags = 0; f.name[0] = 0; f.pos = 0; f.size = 0; f.nChunk = 0; f.capChunk = 0; f.chunkTbl = 0; f.fp = 0;
    if (!f.fioFILE_DISK::Init(file, 1))
        return 0;
    char *buf = (char *)apMalloc(f.size + 1);
    if (!buf)
        return 0;
    f.ReadData(buf, f.size, 1);
    buf[f.size] = 0;
    Empty();
    src = apStrdup(buf);
    if (!src)
        return 0;
    apFree(buf);
    if (ParseString())
        return 0;
    return f.fioFILE_DISK::Term();
}

// v12 0x004F5BE0  (feb 0x004E19A0)
void __fastcall _psStripSpaces(char *s)
{
    int lead = 0;
    for (char *p = s; *p && (*p == ' ' || *p == '\t'); p++)
        lead++;
    int n = (int)strlen(s);
    char *p = s + n - 1;
    while (*p == ' ' || (*p == '\t' && p >= s)) {
        n--;
        p--;
    }
    if (n <= 0) {
        *s = 0;
        return;
    }
    for (int i = 0; i < n; i++)
        s[i] = s[i + lead];
    s[n] = 0;
}
