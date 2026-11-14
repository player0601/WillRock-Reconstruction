// gs_strings.cpp -- gs.lib localisation table gsSTRINGS (texts\strings.str)
// (Will Rock 1.2, v12.exe; feb obj gs:gs_strings.obj, + wr_strings.obj wrSTRINGS::Init)
//
// v12 0x0051C4A0..0x0051DB3F. The table keeps three arrays: the ascii ids (idChars, NUL separated), the
// UTF-16 texts (text, NUL separated) and strPtrs[string id] = offset of the text; records {id offset,
// string id} are kept sorted by id (1.2 inserts at the sorted position; the beta re-sorted on demand).
// String id 0 is "<null>" (the constructor's first text), the empty id "" is string id 1.
// strings.str: UTF-16 LE with a BOM, one record per line: ID <spaces/tabs> text; ';' starts a comment line.
// A word may be quoted ("..."); escapes \" \n and \0<hex digits to the end of the word>.
// ParseTableLine results: 0 ok, 1 bad escape, 2 non-ascii id, 3 duplicate id (stops the whole load).
// Not rebuilt yet: Printf / PrintfV / GetStringIdFmt (format strings with %s %d %f).
// LoadStringsTable: the game streams the file through a 0x100-char window; this reads it whole, with the
// same line splitting and the same stop on the first failing line.

#include "../engine/engine.h"
#include "../../include/game_systems/strings.h"
#include <wchar.h>
#include <stdarg.h>
#include <stdio.h>

gsSTRINGS *gsStrings;                  // v12 0x009946DC (feb 0x0094DE5C)

template <class T> static void strGrow(arrVector<T> *a, int need)
{
    if (a->capacity < need) {
        int c = a->capacity * 2;
        if (c < need)
            c = need;
        a->data = (T *)apRealloc(a->data, c * sizeof(T));
        a->capacity = c;
    }
}
template <class T> static void strInsert(arrVector<T> *a, int pos, const T *v, int n)
{
    strGrow(a, a->count + n);
    memmove(a->data + pos + n, a->data + pos, (a->count - pos) * sizeof(T));
    for (int i = 0; i < n; i++)
        a->data[pos + i] = v[i];
    a->count += n;
}

// Binary search of an id; -1 when absent (arrVector<Record>::FindSorted v12 0x0051DB40). *at = insert position.
static int strFind(gsSTRINGS *s, const char *id, int *at)
{
    int lo = 0, hi = s->records.count;
    while (lo < hi) {
        int mid = (lo + hi) / 2;
        int c = strcmp(s->idChars.data + s->records.data[mid].idOfs, id);
        if (c == 0) {
            if (at)
                *at = mid;
            return mid;
        }
        if (c < 0)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (at)
        *at = lo;
    return -1;
}

// v12 0x0051C4A0  (feb 0x00507030)
gsSTRINGS::gsSTRINGS()
{
    memset(this, 0, sizeof(*this));
    static const wrWCHAR nul[] = L"<null>";
    strInsert(&text, 0, nul, (int)wcslen(nul));
    int zero = 0;
    strInsert(&strPtrs, strPtrs.count, (void **)&zero, 1);
    AddString("", L"");
}

// v12 0x0051C550  (feb 0x005070F0)
int gsSTRINGS::AddString(const char *id, const wrWCHAR *s)
{
    if (strFind(this, id, 0) != -1)
        return 0;
    void *ofs = (void *)(intptr_t)text.count;
    strInsert(&strPtrs, strPtrs.count, &ofs, 1);
    Record r;
    r.idOfs = idChars.count;
    r.textOfs = strPtrs.count - 1;                      // (the string id)
    strInsert(&idChars, idChars.count, id, (int)strlen(id) + 1);
    strInsert(&text, text.count, s, (int)wcslen(s) + 1);
    int at;
    strFind(this, id, &at);
    strInsert(&records, at, &r, 1);
    return strPtrs.count - 1;
}

// v12 0x0051CF20  (feb 0x00507AA0)
int gsSTRINGS::GetStringId(const char *id)
{
    int i = strFind(this, id, 0);
    if (i == -1) {
        apLog((char *)"GS_STRINGS: Can't find string \"%s\"", id);
        return 0;
    }
    return records.data[i].textOfs;
}

// v12 0x0051D120  (feb 0x00507DA0)
const wrWCHAR *gsSTRINGS::GetStringById(int i)
{
    return text.data + (intptr_t)strPtrs.data[i];
}

// v12 0x0051D140  (feb 0x00507DC0)
void gsSTRINGS::GetStringById(int i, wrWCHAR *out, int n)
{
    const wrWCHAR *s = GetStringById(i);
    wrWCHAR *end = out + n - 1;
    for (; *s && out < end; s++)
        *out++ = *s;
    *out = 0;
}

// v12 0x0051D180  (feb 0x00507E00)
void gsSTRINGS::AsciiToUnicode(const char *a, wrWCHAR *out, int n)
{
    wrWCHAR *end = out + n - 1;
    for (; *a && out < end; a++)
        *out++ = (wrWCHAR)*a;                           // (sign-extended char, as in the game)
    *out = 0;
}

// v12 0x0051D1C0  (feb 0x00507E40) into the shared apBuffer (0x1000 bytes)
const wrWCHAR *gsSTRINGS::AsciiToUnicode(const char *a)
{
    static wrWCHAR buf[0x800];                          // (apBuffer in the game)
    wrWCHAR *p = buf;
    for (; *a && p < buf + 0x7FF; a++)
        *p++ = (wrWCHAR)*a;
    *p = 0;
    return buf;
}

// v12 0x0051D200  (feb 0x00507E80)
int gsSTRINGS::UnicodeToAscii(const wrWCHAR *s, char *out, int n)
{
    char *end = out + n - 1;
    int ok = 1;
    for (; *s && out < end; s++, out++) {
        if (*s < 0x80)
            *out = (char)*s;
        else {
            *out = '?';
            ok = 0;
        }
    }
    *out = 0;
    return ok;
}

// v12 0x0051D6B0  (feb 0x005083A0) hex digits up to the end of the text
int gsSTRINGS::ReadEscChar(wrWCHAR **p, wrWCHAR *out)
{
    wrWCHAR *s = *p;
    *out = 0;
    for (;;) {
        wrWCHAR c = *s;
        if (c >= '0' && c <= '9')
            c = (wrWCHAR)(*out * 0x100 + c - 0x30);
        else if (c >= 'a' && c <= 'f')
            c = (wrWCHAR)(*out * 0x100 + c - 0x57);
        else if (c >= 'A' && c <= 'F')
            c = (wrWCHAR)(*out * 0x100 + c - 0x26);     // (sic: 'A' - 0x26 = 0x1B, not 10)
        else
            return 1;
        s++;
        *out = c;
        if (!*s) {
            *p = s;
            return 0;
        }
    }
}

// v12 ? (feb 0x00508070) one word: blanks skipped, quotes group, backslash escapes
static int strReadWord(gsSTRINGS *self, wrWCHAR **pp, arrVector<wrWCHAR> *w, int (gsSTRINGS::*esc)(wrWCHAR **, wrWCHAR *))
{
    wrWCHAR *p = *pp;
    int inQuote = 0, escape = 0;
    w->count = 0;
    while (*p == ' ' || *p == '\t')
        p++;
    while (*p) {
        wrWCHAR c = *p;
        if (!escape) {
            if (c == '"')
                inQuote = !inQuote;
            else if (!inQuote && (c == ' ' || c == '\t'))
                break;
            else if (c == '\\')
                escape = 1;
            else
                strInsert(w, w->count, &c, 1);
            p++;
        } else {
            if (c == '"') {
                strInsert(w, w->count, &c, 1);
                p++;
            } else if (c == '0') {
                wrWCHAR v = 0;
                strInsert(w, w->count, &v, 1);
                if ((self->*esc)(&p, &w->data[w->count - 1]))
                    return 1;
            } else if (c == 'n') {
                wrWCHAR nl = '\n';
                strInsert(w, w->count, &nl, 1);
                p++;
            } else
                return 1;
            escape = 0;
        }
    }
    wrWCHAR z = 0;
    strInsert(w, w->count, &z, 1);
    *pp = p;
    return 0;
}

// v12 0x0051D250  (feb 0x00507ED0)
int gsSTRINGS::ParseTableLine(wrWCHAR *line)
{
    arrVector<wrWCHAR> w;
    w.data = 0;
    w.count = 0;
    w.capacity = 0;
    strGrow(&w, 0x100);
    int r = 0;
    if (*line != ';') {
        r = strReadWord(this, &line, &w, &gsSTRINGS::ReadEscChar);
        if (r == 0 && w.data[0]) {
            char id[0x100];
            char *o = id;
            int ok = 1;
            for (wrWCHAR *s = w.data; *s && o < id + 0xFF; s++, o++) {
                if (*s < 0x80)
                    *o = (char)*s;
                else {
                    *o = '?';
                    ok = 0;
                }
            }
            *o = 0;
            if (!ok)
                r = 2;
            else {
                int i = strFind(this, id, 0);
                if (i == -1)
                    apLog((char *)"GS_STRINGS: Can't find string \"%s\"", id);   // (logged for every new id)
                else if (records.data[i].textOfs != 0)
                    r = 3;
                if (r == 0) {
                    r = strReadWord(this, &line, &w, &gsSTRINGS::ReadEscChar);
                    if (r == 0)
                        AddString(id, w.data);
                }
            }
        }
    }
    apFree(w.data);
    return r;
}

// v12 0x0051D730  (feb 0x00508420)
int gsSTRINGS::LoadStringsTable(const char *file)
{
    fioFILE_DISK f;
    if (!f.Init((char *)file, FIO_READ))
        return 5;
    int n = (int)f.size / 2;
    wrWCHAR *buf = (wrWCHAR *)apMalloc((n + 1) * 2);
    f.ReadData(buf, n * 2, 2);
    buf[n] = 0;
    f.Term();
    if (n < 1 || buf[0] != 0xFEFF) {
        apFree(buf);
        return 1;
    }
    int r = 0;
    int i = 1;
    while (i < n && r == 0) {
        while (i < n && (buf[i] == '\n' || buf[i] == '\r'))
            i++;
        int start = i;
        while (i < n && buf[i] != '\n' && buf[i] != '\r')
            i++;
        if (start >= n)
            break;
        buf[i] = 0;
        r = ParseTableLine(buf + start);
        i++;
    }
    apFree(buf);
    return r;
}

// v12 0x004AF300  (feb 0x004A0770) wrSTRINGS::Init: [Language] fileTexts, default ..\texts\strings.str
int wrStringsInit(gsSTRINGS *s, const char *file)
{
    return s->LoadStringsTable(file) == 0;
}

// v12 0x0051CFD0  (feb 0x00507C20); v12 0x0051D080 is an identical twin
// The id of the string named by the formatted text (in apBuffer); 0 when there is none (no log, unlike GetStringId).
int gsSTRINGS::GetStringIdFmt(const char *fmt, ...)
{
    va_list a;
    va_start(a, fmt);
    vsprintf(apBuffer, fmt, a);
    va_end(a);
    int i = strFind(this, apBuffer, 0);
    if (i == -1)
        return 0;
    return records.data[i].textOfs;
}

// ---------------------------------------------------------------------------------------------- Printf

#define PF_END ((wrWCHAR *)(apBuffer + 0xFFE))           // v12 0x0065515E: the last character apBuffer takes

static void pfPutW(wrWCHAR **o, const wrWCHAR *s)
{
    while (*s && *o < PF_END)
        *(*o)++ = *s++;
}

// v12 0x0051CD80  (feb 0x00507900)
void gsSTRINGS::PrintfInsertStringA(wrWCHAR **o, wrWCHAR *end, const char *s)
{
    while (*s && *o < end)
        *(*o)++ = (wrWCHAR)(short)*s++;
}

// v12 0x0051CDC0  (feb 0x00507940)
// The integer part (padded with pad to width), then the fraction's digits -- prec of them, or (prec -1) while
// anything is left -- with no point between (as the game writes it).
void gsSTRINGS::PrintfInsertFloat(wrWCHAR **o, wrWCHAR *end, float v, int width, wrWCHAR pad, int prec)
{
    char b[16];
    itoa((int)v, b, 10);
    if (width) {
        int n = width - (int)strlen(b);
        while (n > 0 && *o < end) {
            *(*o)++ = pad;
            n--;
        }
    }
    for (char *c = b; *c && *o < end; c++)
        *(*o)++ = (wrWCHAR)*c;
    if (v < 0.0f)
        v = -v;
    if (prec == -1) {
        while (v > 1e-13f) {
            v = (v - (float)(int)v) * 10.0f;
            if (*o < end)
                *(*o)++ = (wrWCHAR)('0' + (int)v);
        }
    } else if (prec > 0) {
        for (; prec; prec--) {
            v = (v - (float)(int)v) * 10.0f;
            if (*o < end)
                *(*o)++ = (wrWCHAR)('0' + (int)v);
        }
    }
}

// v12 0x0051C840  (feb 0x005073C0)
// The game's formatter into apBuffer (UTF-16): %[0][width][.prec]type with s (wide), S (ascii), d / i, f, I (a
// string table index), t (spaces to column width), %% ; after the type an argument number (1..) may follow (and
// an optional '|'), and then every field must be numbered. Errors are written in: <UNEXPECTED_EOL>,
// <UNNUMBERED_ARG>, <INVALID_FIELD>.
const wrWCHAR *gsSTRINGS::PrintfV(const wrWCHAR *fmt, char *va)
{
    static const wrWCHAR types[] = { 's', 'S', 'd', 'i', 'f', 'I', 0 };   // v12 0x00635978
    static const wrWCHAR eEol[] = { '<','U','N','E','X','P','E','C','T','E','D','_','E','O','L','>',0 };
    static const wrWCHAR eUnn[] = { '<','U','N','N','U','M','B','E','R','E','D','_','A','R','G','>',0 };
    static const wrWCHAR eInv[] = { '<','I','N','V','A','L','I','D','_','F','I','E','L','D','>',0 };
    void *args[16];                                     // (an arrVector<void*> in the game)
    int nArgs = 0;
    wrWCHAR *o = (wrWCHAR *)apBuffer;
    int numbered = 0;                                   // 1 numbered fields, 2 plain ones
    const wrWCHAR *f = fmt;
    while (*f) {
        if (*f != '%') {
            if (o < PF_END)
                *o++ = *f;
            f++;
            continue;
        }
        const wrWCHAR *q = f + 1;
        wrWCHAR pad = ' ';
        if (*q == '0') {
            pad = '0';
            q++;
        }
        int width = 0;
        while (*q >= '0' && *q <= '9')
            width = width * 10 + (*q++ - '0');
        int prec = -1;
        if (*q == '.') {
            q++;
            prec = 0;
            while (*q >= '0' && *q <= '9')
                prec = prec * 10 + (*q++ - '0');
        }
        wrWCHAR type = *q;
        if (!type) {
            pfPutW(&o, eEol);
            break;
        }
        f = q + 1;
        void *val = 0;
        if (wcschr(types, type)) {
            if (numbered != 2 && *f > '0' && *f <= '9') {
                numbered = 1;
                int idx = 0;
                while (*f >= '0' && *f <= '9')
                    idx = idx * 10 + (*f++ - '0');
                while (nArgs < idx && nArgs < 16) {
                    args[nArgs++] = *(void **)va;
                    va += 4;
                }
                val = idx >= 1 && idx <= 16 ? args[idx - 1] : 0;
                if (*f == '|')
                    f++;
            } else if (numbered == 1) {
                pfPutW(&o, eUnn);
                continue;
            } else {
                numbered = 2;
                val = *(void **)va;
                va += 4;
            }
        }
        switch (type) {
        case '%':
            if (o < PF_END)
                *o++ = '%';
            break;
        case 'I':
            pfPutW(&o, GetStringById((int)(intptr_t)val));
            break;
        case 'S':
            for (const char *c = (const char *)val; *c && o < PF_END; c++)
                *o++ = (wrWCHAR)(short)*c;
            break;
        case 'd':
        case 'i': {
            char b[16];
            itoa((int)(intptr_t)val, b, 10);
            if (width) {
                int n = width - (int)strlen(b);
                while (n > 0 && o < PF_END) {
                    *o++ = pad;
                    n--;
                }
            }
            PrintfInsertStringA(&o, PF_END, b);
            break;
        }
        case 'f': {
            float v;
            memcpy(&v, &val, 4);
            PrintfInsertFloat(&o, PF_END, v, width, pad, prec);
            break;
        }
        case 's':
            pfPutW(&o, (const wrWCHAR *)val);
            break;
        case 't':
            for (int n = width - (int)(o - (wrWCHAR *)apBuffer); n > 0 && o < PF_END; n--)
                *o++ = ' ';
            break;
        default:
            pfPutW(&o, eInv);
            break;
        }
    }
    if (o <= PF_END)
        *o = 0;
    *PF_END = 0;
    return (const wrWCHAR *)apBuffer;
}

// v12 0x0051C740  (feb 0x005072B0)
const wrWCHAR *gsSTRINGS::Printf(int id, ...)
{
    return PrintfV(GetStringById(id), (char *)(&id + 1));
}

// v12 0x0051C760  (feb 0x005072D0)
// The string named id (none: the first, logged) formatted with the arguments.
const wrWCHAR *gsSTRINGS::Printf(const char *id, ...)
{
    return PrintfV(GetStringById(GetStringId(id)), (char *)(&id + 1));
}
