// fname.cpp -- engine.lib file names: path/extension helpers and the case-insensitive disk lookup
// (Will Rock 1.2, v12.exe; feb obj engine:fname.obj)
//
// v12 0x004FEAF0..0x004FFAEF, every function "exact" (the fnmDIR destructor and Term only differ by call
// targets). Every fnm* result is the one static buffer fnmBuffer: copy it before the next fnm* call.
// fnmFindDisc keeps a listing of each directory asked for (fnmDIR, with all its subdirectories) and looks
// names up in it case-insensitively; on a miss it scans the disk (_fnmFindDisc) and rebuilds the listing.
// Original quirks kept:
//  - fnmFindDisc returns fnmBuffer after RebuildContent has reused it (a file found only on the disk scan
//    comes back as the last path RebuildContent built, not the file);
//  - fnmDIR::FindName also searches the directory's 'next' (the next cached directory for a top-level one);
//  - RebuildContent does not free the old file names; _fnmFindDisc leaves its find handle open on a hit;
//  - fnmMakeFullName reads ext[0] before testing ext for 0; fnmTerm does not reset fnmExeDir.

#include "engine.h"
#include <io.h>
#include <windows.h>

char    fnmPathSep = '\\';     // v12 0x00634B70 (feb 0x005F6940)
char    fnmBuffer[0x200];      // v12 0x00984BF8 (feb 0x009409B8)
char   *fnmExeDir;             // v12 0x00984DF8 (feb 0x00940BB8)
fnmDIR  fnmDirRoot;            // v12 0x00984AE0 (feb 0x009408A0) (zeroed by _$E6, Term'd at exit by _$E4)

// buf += path separator unless empty or already ending with one (inlined everywhere)
static inline void fnmAddSep(char *buf)
{
    int n = (int)strlen(buf);
    if (n > 0 && buf[n - 1] != fnmPathSep) {
        buf[n] = fnmPathSep;
        buf[n + 1] = 0;
    }
}

// The body shared by ~fnmDIR, Term and the deleting destructor: free the file names, destroy the
// subdirectories and the following siblings.
static void fnmDirFree(fnmDIR *d)
{
    for (int i = 0; i < d->nFiles; i++)
        apFree(d->files[i]);
    apFree(d->files);
    d->nFiles = 0;
    d->files = 0;
    if (d->sub) {
        d->sub->Term();
        operator delete(d->sub);
    }
    d->sub = 0;
    if (d->next) {
        d->next->Term();
        operator delete(d->next);
    }
    d->next = 0;
}

// v12 0x004FEBB0  (feb 0x004EC5D0)
fnmDIR::~fnmDIR()
{
    fnmDirFree(this);
}

// v12 0x004FEC40  (feb 0x004EC660)
// The exe's directory (with its trailing separator) into fnmExeDir.
BOOL32 __fastcall fnmInit()
{
    fnmPathSep = '\\';
    if (!fnmExeDir) {
        char *src;
        if (GetModuleFileNameA((HINSTANCE)apHInstance, fnmBuffer, 0x200))
            src = fnmBuffer;
        else
            src = strrchr(_pgmptr, fnmPathSep) ? _pgmptr : (char *)"";
        fnmExeDir = apStrdup(src);
        char *p = strrchr(fnmExeDir, fnmPathSep);
        if (p)
            p[1] = 0;
    }
    return 1;
}

// v12 0x004FECD0  (feb 0x004EC6F0)
void __fastcall fnmTerm()
{
    fnmDirFree(&fnmDirRoot);
    apFree(fnmExeDir);
}

// v12 0x004FED70  (feb 0x004EC790)
// Replace the extension of name's last component (ext 0: remove it), or append one.
char *__fastcall fnmAddExt(char *name, char *ext)
{
    strcpy(fnmBuffer, name);
    char *s = strrchr(fnmBuffer, fnmPathSep);
    char *d = strrchr(fnmBuffer, '.');
    if (!s)
        s = fnmBuffer;
    if (d && s <= d) {
        if (!ext) {
            *d = 0;
            return fnmBuffer;
        }
        if (*ext == '.')
            ext++;
        strcpy(d + 1, ext);
        return fnmBuffer;
    }
    if (ext) {
        if (*ext != '.')
            strcat(fnmBuffer, ".");
        strcat(fnmBuffer, ext);
    }
    return fnmBuffer;
}

// v12 0x004FEE40  (feb 0x004EC860)
// dir + separator + name + "." + ext. forceExt: name's own extension is replaced (ext 0: removed).
char *__fastcall fnmMakeFullName(char *dir, char *name, char *ext, int forceExt)
{
    if (!dir)
        strcpy(fnmBuffer, name);
    else {
        strcpy(fnmBuffer, dir);
        fnmAddSep(fnmBuffer);
        strcat(fnmBuffer, name);
    }
    if (*ext == '.')
        ext++;
    if (forceExt) {
        if (!ext)
            return fnmAddExt(fnmBuffer, 0);
        if (strrchr(name, '.'))
            *strrchr(fnmBuffer, '.') = 0;
    }
    strcat(fnmBuffer, ".");
    strcat(fnmBuffer, ext);
    return fnmBuffer;
}

// v12 0x004FEF60  (feb 0x004EC980)
// The directory part, without the trailing separator ("" for a bare name).
char *__fastcall fnmGetPath(char *full)
{
    strcpy(fnmBuffer, full);
    int n = (int)strlen(fnmBuffer);
    if (fnmBuffer[n - 1] != fnmPathSep) {
        char *p = strrchr(fnmBuffer, fnmPathSep);
        if (!p) {
            fnmBuffer[0] = 0;
            return fnmBuffer;
        }
        *p = 0;
        return fnmBuffer;
    }
    fnmBuffer[n - 1] = 0;
    return fnmBuffer;
}

// v12 0x004FEFE0  (feb 0x004ECA00)
// The last component (after the last separator, else after a drive colon).
char *__fastcall fnmGetNameExt(char *full)
{
    char *p = strrchr(full, fnmPathSep);
    if (!p && !(p = strrchr(full, ':'))) {
        strcpy(fnmBuffer, full);
        return fnmBuffer;
    }
    strcpy(fnmBuffer, p + 1);
    return fnmBuffer;
}

// v12 0x004FF040  (feb 0x004ECA60)
// The last component without its extension.
char *__fastcall fnmGetName(char *full)
{
    char *p = strrchr(full, fnmPathSep);
    if (!p && !(p = strrchr(full, ':')))
        strcpy(fnmBuffer, full);
    else
        strcpy(fnmBuffer, p + 1);
    char *d = strrchr(fnmBuffer, '.');
    if (d)
        *d = 0;
    return fnmBuffer;
}

// v12 0x004FF0B0  (feb 0x004ECAD0)
// The real path of dir\name.ext (case-insensitive, also in subdirectories of dir; ext 0 = any extension),
// or 0. dir's listing is cached on first use.
char *__fastcall fnmFindDisc(char *dir, char *name, char *ext)
{
    fnmDIR *d;
    for (d = fnmDirRoot.next; d; d = d->next)
        if (!stricmp(d->name, dir))
            break;
    if (!d) {
        d = (fnmDIR *)operator new(sizeof(fnmDIR));
        if (!d)
            return 0;
        d->nFiles = 0;
        d->files = 0;
        d->next = 0;
        d->sub = 0;
        d->name[0] = 0;
        d->next = fnmDirRoot.next;
        fnmDirRoot.next = d;
        if (!d->RebuildContent(dir))
            return 0;
    }
    char *r = d->FindName(name, ext);
    if (r)
        return r;
    r = _fnmFindDisc(dir, name, ext);
    if (!r || d->RebuildContent(dir))
        return r;
    return 0;
}

// v12 0x004FF190  (feb 0x004ECBB0)
// The disk scan: dir\name.ext readable as written, else a case-insensitive match of name (ext 0 only) in
// dir, else the same search in every subdirectory.
char *__fastcall _fnmFindDisc(char *dir, char *name, char *ext)
{
    char tmp[260];
    _finddata_t fd;
    if (ext) {
        if (!dir)
            strcpy(fnmBuffer, name);
        else {
            strcpy(fnmBuffer, dir);
            fnmAddSep(fnmBuffer);
            strcat(fnmBuffer, name);
        }
        if (*ext == '.')
            ext++;
        strcat(fnmBuffer, ".");
        strcat(fnmBuffer, ext);
        if (!access(fnmBuffer, 4))
            return fnmBuffer;
    }
    strcpy(fnmBuffer, dir);
    fnmAddSep(fnmBuffer);
    strcat(fnmBuffer, "*.*");
    intptr_t h = _findfirst(fnmBuffer, &fd);
    if (h == -1)                                        // original: one pass over an unset find record
        return 0;
    for (;;) {
        if (!(fd.attrib & _A_SUBDIR) || fd.name[0] == '.') {
            if (!ext) {
                strcpy(tmp, fd.name);
                char *d = strrchr(tmp, '.');
                if (d)
                    *d = 0;
                if (!stricmp(name, tmp))
                    break;
            }
        } else {
            strcpy(tmp, dir);
            strcat(tmp, "\\");
            strcat(tmp, fd.name);
            if (fnmFindDisc(tmp, name, ext))
                return fnmBuffer;
        }
        if (_findnext(h, &fd)) {
            _findclose(h);
            return 0;
        }
    }
    strcpy(fnmBuffer, dir);
    fnmAddSep(fnmBuffer);
    strcat(fnmBuffer, fd.name);
    return fnmBuffer;
}

// v12 0x004FF4B0  (feb 0x004ECED0)
void fnmDIR::Term()
{
    fnmDirFree(this);
}

// v12 0x004FF540  (feb 0x004ECF60)
// name (ext 0: any extension) among the files, then in the subdirectories, then in 'next'. The result is
// dir\<file as on disk> for ext 0, else fnmMakeFullName(dir, name, ext, 1).
char *fnmDIR::FindName(char *nm, char *ext)
{
    for (int i = 0; i < nFiles; i++) {
        if (!ext) {
            fnmGetName(files[i]);
            if (!stricmp(fnmBuffer, nm)) {
                char *f = files[i];
                strcpy(fnmBuffer, name);
                fnmAddSep(fnmBuffer);
                strcat(fnmBuffer, f);
                return fnmBuffer;
            }
        } else {
            char *p = fnmAddExt(nm, ext);
            if (!stricmp(files[i], p)) {
                strcpy(fnmBuffer, name);
                fnmAddSep(fnmBuffer);
                strcat(fnmBuffer, nm);
                if (*ext == '.')
                    ext++;
                if (!ext)
                    return fnmAddExt(fnmBuffer, 0);
                if (strrchr(nm, '.'))
                    *strrchr(fnmBuffer, '.') = 0;
                strcat(fnmBuffer, ".");
                strcat(fnmBuffer, ext);
                return fnmBuffer;
            }
        }
    }
    char *r;
    if (sub && (r = sub->FindName(nm, ext)))
        return r;
    if (next && (r = next->FindName(nm, ext)))
        return r;
    return 0;
}

// v12 0x004FF7E0  (feb 0x004ED200)
// List dir: its files into 'files', its subdirectories (recursively) under 'sub'.
BOOL32 fnmDIR::RebuildContent(char *dir)
{
    strcpy(name, dir);
    if (sub) {
        fnmDIR *s = sub;
        for (int i = 0; i < s->nFiles; i++)
            apFree(s->files[i]);
        apFree(s->files);
        s->nFiles = 0;
        s->files = 0;
        if (s->sub)
            s->sub->__scalarDelDtor(1);
        s->sub = 0;
        if (s->next)
            s->next->__scalarDelDtor(1);
        s->next = 0;
        operator delete(s);
    }
    sub = 0;
    nFiles = 0;
    strcpy(fnmBuffer, name);
    fnmAddSep(fnmBuffer);
    strcat(fnmBuffer, "*.*");
    _finddata_t fd;
    intptr_t h = _findfirst(fnmBuffer, &fd);
    if (h == -1)                                        // original: one pass over an unset find record
        return 1;
    for (;;) {
        if (fd.name[0] != '.') {
            if (!(fd.attrib & _A_SUBDIR)) {
                files = (char **)apRealloc(files, nFiles * 4 + 4);
                if (!files)
                    return 0;
                files[nFiles] = apStrdup(fd.name);
                if (!files[nFiles])
                    return 0;
                nFiles++;
            } else {
                fnmDIR *d = (fnmDIR *)operator new(sizeof(fnmDIR));
                if (!d)
                    return 0;
                d->nFiles = 0;
                d->files = 0;
                d->next = 0;
                d->sub = 0;
                d->name[0] = 0;
                if (!sub)
                    sub = d;
                else {
                    d->next = sub->next;
                    sub->next = d;
                }
                strcpy(fnmBuffer, name);
                fnmAddSep(fnmBuffer);
                strcat(fnmBuffer, fd.name);
                if (!d->RebuildContent(fnmBuffer))
                    return 0;
            }
        }
        if (_findnext(h, &fd)) {
            _findclose(h);
            return 1;
        }
    }
}

// v12 0x004FFAA0  (feb 0x004ED4C0)
void *fnmDIR::__scalarDelDtor(unsigned flags)
{
    for (int i = 0; i < nFiles; i++)
        apFree(files[i]);
    apFree(files);
    nFiles = 0;
    files = 0;
    if (sub)
        sub->__scalarDelDtor(1);
    sub = 0;
    if (next)
        next->__scalarDelDtor(1);
    next = 0;
    if (flags & 1)
        operator delete(this);
    return this;
}
