// ap_cfg.cpp -- ap.lib INI configuration (Will Rock 1.2, v12.exe; feb obj ap:ap_cfg.obj)
//
// v12 0x004AF670..0x004AFFF9 (+ switch tables to 0x004B001F), byte-identical to feb apart from addresses.
// A thin cache over Get/WritePrivateProfileStringA on one file (WILLROCK.INI, opened by
// gsAPP_SYSTEM::InitAll with apCfgInit(name, 1) = relative to the current directory).
//
// How the cache works: every key read or written is kept in apCfgSects[] (section -> keys). The
// _apCfgRead* entry points take a "void **cache": the call sites (the apCfgRead* macros of the original
// header) pass the address of a static pointer, so after the first successful lookup the key record is
// reached directly and the file is never read again. Consequence for mods: editing the INI on disk while
// the game runs has no effect on values already read; write through apCfgWrite* (updates cache + file).

#include "ap.h"

int         apCfgNSects = 0;             // v12 0x00655170 (feb 0x00610F90)
apCFG_SECT *apCfgSects  = NULL;          // v12 0x00655174 (feb 0x00610F94)
char        apCfgFile[MAX_PATH];         // v12 0x00655178 (feb 0x00610F98)

#define CFG_NUM_LEN 0x20                 // text buffer for numbers / bools

// v12 0x004AF670  (feb 0x004A0AE0)
// Open the INI: relative = 1 prefixes the current directory. Fails (with apMsg) if the file does not exist.
BOOL32 __fastcall apCfgInit(char *file, int relative)
{
    char path[MAX_PATH];
    if (relative) {
        GetCurrentDirectoryA(MAX_PATH, path);
        strcat(path, "\\");
        strcat(path, file);
        file = path;
    }
    if (_access(file, 0) != 0) {
        apMsg("Cannot open file %s", file);
        return 0;
    }
    strncpy(apCfgFile, file, MAX_PATH);
    apCfgNSects = 0;
    apCfgSects  = NULL;
    return 1;
}

// v12 0x004AF730  (feb 0x004A0BA0)
// Free the whole cache and flush the INI (WritePrivateProfileString(NULL,NULL,NULL) flushes the cache of Windows).
void __fastcall apCfgTerm()
{
    for (int s = 0; s < apCfgNSects; s++) {
        apCFG_SECT *sect = &apCfgSects[s];
        for (int k = 0; k < sect->nKeys; k++) {
            apCFG_KEY *key = sect->keys[k];
            if (key->type == APCFG_STRING || key->type == APCFG_BINARY)
                apFree(key->v.data);
            apFree(sect->keys[k]);
        }
        apFree(sect->keys);
    }
    apFree(apCfgSects);
    WritePrivateProfileStringA(NULL, NULL, NULL, apCfgFile);
    apCfgFile[0] = 0;
    // ? apCfgSects / apCfgNSects are not reset (a second apCfgTerm would double free)
}

// Base for strtol/strtoul: "0x"/"0X" prefix = hex, otherwise decimal.
static inline int CfgNumBase(const char *s)
{
    return (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) ? 16 : 10;
}

// v12 0x004AF7E0  (feb 0x004A0C50)
// Read an int (decimal or 0x hex). Returns 0 if the key is absent (out untouched).
BOOL32 __fastcall _apCfgReadInt(void **cache, char *sect, char *key, int *out)
{
    if (_apCfgSectGetKey(cache, sect, key, out, APCFG_INT, 4))
        return 1;
    char buf[CFG_NUM_LEN];
    if (!GetPrivateProfileStringA(sect, key, "", buf, CFG_NUM_LEN, apCfgFile))
        return 0;
    *out = strtol(buf, NULL, CfgNumBase(buf));
    _apCfgSectSetKey(sect, key, out, APCFG_INT, 4);
    return 1;
}

// v12 0x004AF890  (feb 0x004A0D00)
// Read an unsigned dword (decimal or 0x hex).
BOOL32 __fastcall _apCfgReadDword(void **cache, char *sect, char *key, u32 *out)
{
    if (_apCfgSectGetKey(cache, sect, key, out, APCFG_DWORD, 4))
        return 1;
    char buf[CFG_NUM_LEN];
    if (!GetPrivateProfileStringA(sect, key, "", buf, CFG_NUM_LEN, apCfgFile))
        return 0;
    *out = strtoul(buf, NULL, CfgNumBase(buf));
    _apCfgSectSetKey(sect, key, out, APCFG_DWORD, 4);
    return 1;
}

// v12 0x004AF940  (feb 0x004A0DB0)
// Read a bool: true only if the value starts with "Yes" (case-insensitive), anything else is false.
BOOL32 __fastcall _apCfgReadBool(void **cache, char *sect, char *key, int *out)
{
    if (_apCfgSectGetKey(cache, sect, key, out, APCFG_BOOL, 4))
        return 1;
    char buf[CFG_NUM_LEN];
    if (!GetPrivateProfileStringA(sect, key, "", buf, CFG_NUM_LEN, apCfgFile))
        return 0;
    *out = _strnicmp(buf, "Yes", 3) == 0;
    _apCfgSectSetKey(sect, key, out, APCFG_BOOL, 4);
    return 1;
}

// v12 0x004AF9D0  (feb 0x004A0E40)
// Read a float (atof).
BOOL32 __fastcall _apCfgReadFloat(void **cache, char *sect, char *key, float *out)
{
    if (_apCfgSectGetKey(cache, sect, key, out, APCFG_FLOAT, 4))
        return 1;
    char buf[CFG_NUM_LEN];
    if (!GetPrivateProfileStringA(sect, key, "", buf, CFG_NUM_LEN, apCfgFile))
        return 0;
    *out = (float)atof(buf);
    _apCfgSectSetKey(sect, key, out, APCFG_FLOAT, 4);
    return 1;
}

// v12 0x004AFA50  (feb 0x004A0EC0)
// Read a string into out[n].
BOOL32 __fastcall _apCfgReadString(void **cache, char *sect, char *key, char *out, int n)
{
    if (_apCfgSectGetKey(cache, sect, key, out, APCFG_STRING, n))
        return 1;
    if (!GetPrivateProfileStringA(sect, key, "", out, n, apCfgFile))
        return 0;
    _apCfgSectSetKey(sect, key, out, APCFG_STRING, strlen(out));
    return 1;
}

// v12 0x004AFAD0  (feb 0x004A0F40)
// Write an int (decimal) to cache and file.
BOOL32 __fastcall apCfgWriteInt(char *sect, char *key, int v)
{
    char buf[CFG_NUM_LEN];
    _apCfgSectSetKey(sect, key, &v, APCFG_INT, 4);
    _itoa(v, buf, 10);
    return WritePrivateProfileStringA(sect, key, buf, apCfgFile);
}

// v12 0x004AFB20  (feb 0x004A0F90)
// Write a dword as "0x" + hex.
BOOL32 __fastcall apCfgWriteDword(char *sect, char *key, u32 v)
{
    char buf[CFG_NUM_LEN];
    _apCfgSectSetKey(sect, key, &v, APCFG_DWORD, 4);
    buf[0] = '0';
    buf[1] = 'x';
    _itoa(v, buf + 2, 16);               // itoa, not ultoa: fine for hex (no sign for radix 16)
    return WritePrivateProfileStringA(sect, key, buf, apCfgFile);
}

// v12 0x004AFB70  (feb 0x004A0FE0)
// Write a bool as "Yes"/"No".
BOOL32 __fastcall apCfgWriteBool(char *sect, char *key, int v)
{
    char buf[CFG_NUM_LEN];
    _apCfgSectSetKey(sect, key, &v, APCFG_BOOL, 4);
    strcpy(buf, v ? "Yes" : "No");
    return WritePrivateProfileStringA(sect, key, buf, apCfgFile);
}

// v12 0x004AFBD0  (feb 0x004A1040)
// Write a float with "%f".
BOOL32 __fastcall apCfgWriteFloat(char *sect, char *key, float v)
{
    char buf[CFG_NUM_LEN];
    _apCfgSectSetKey(sect, key, &v, APCFG_FLOAT, 4);
    sprintf(buf, "%f", (double)v);
    return WritePrivateProfileStringA(sect, key, buf, apCfgFile);
}

// v12 0x004AFC20  (feb 0x004A1090)
// Write a string.
BOOL32 __fastcall apCfgWriteString(char *sect, char *key, char *v)
{
    _apCfgSectSetKey(sect, key, v, APCFG_STRING, strlen(v));
    return WritePrivateProfileStringA(sect, key, v, apCfgFile);
}

// Lookup helpers (inlined in the original).
static apCFG_SECT *CfgFindSect(char *sect)
{
    for (int i = 0; i < apCfgNSects; i++)
        if (apCfgSects[i].name.IsName(sect))
            return &apCfgSects[i];
    return NULL;
}

static apCFG_KEY *CfgFindKey(apCFG_SECT *s, char *key)
{
    for (int i = 0; i < s->nKeys; i++)
        if (s->keys[i]->name.IsName(key))
            return s->keys[i];
    return NULL;
}

// v12 0x004AFC60  (feb 0x004A10D0)
// Fetch a cached value: through *cache when already resolved, else by section/key name (and fill *cache).
// Returns 0 when the key has not been cached yet. The stored type is not checked against "type".
BOOL32 __fastcall _apCfgSectGetKey(void **cache, char *sect, char *key, void *out, int type, int size)
{
    apCFG_KEY *k;
    if (cache && *cache) {
        k = (apCFG_KEY *)*cache;
    } else {
        apCFG_SECT *s = CfgFindSect(sect);
        if (!s)
            return 0;
        k = CfgFindKey(s, key);
        if (!k)
            return 0;
        if (cache)
            *cache = k;
    }
    switch (type) {
    case APCFG_BOOL:
    case APCFG_FLOAT:
    case APCFG_INT:
    case APCFG_DWORD:
        *(u32 *)out = k->v.dw;
        return 1;
    case APCFG_STRING: {
        int n = k->size + 1;
        if (size <= n) n = size;
        strncpy((char *)out, k->v.str, n);
        return 1;
    }
    case APCFG_BINARY: {
        int n = k->size;
        if (size <= n) n = size;
        memcpy(out, k->v.data, n);
        return 1;
    }
    }
    return 1;
}

// v12 0x004AFDE0  (feb 0x004A1250)
// Store a value in the cache, creating the section/key records on first use.
BOOL32 __fastcall _apCfgSectSetKey(char *sect, char *key, void *val, int type, int size)
{
    apCFG_SECT *s = CfgFindSect(sect);
    if (!s) {
        apCfgSects = (apCFG_SECT *)apRealloc(apCfgSects, (apCfgNSects + 1) * sizeof(apCFG_SECT));
        if (!apCfgSects)
            return 0;
        s = &apCfgSects[apCfgNSects];
        memset(s, 0, sizeof(*s));
        s->name.SetName(sect);
        apCfgNSects++;
    }
    apCFG_KEY *k = CfgFindKey(s, key);
    if (!k) {
        s->keys = (apCFG_KEY **)apRealloc(s->keys, (s->nKeys + 1) * sizeof(apCFG_KEY *));
        if (!s->keys)
            return 0;
        k = (apCFG_KEY *)apCalloc(1, sizeof(apCFG_KEY));
        if (!k)
            return 0;
        s->keys[s->nKeys++] = k;
        k->name.SetName(key);
    }
    switch (type) {
    case APCFG_BOOL:
    case APCFG_FLOAT:
    case APCFG_INT:
    case APCFG_DWORD:
        k->size = 4;
        k->type = type;
        k->v.dw = *(u32 *)val;
        return 1;
    case APCFG_STRING:
        k->size = size;
        apFree(k->v.str);
        k->v.str = apStrdup((char *)val);
        if (!k->v.str)
            return 0;
        break;
    case APCFG_BINARY:
        k->size = size;                  // ? the old block is leaked when a binary key is re-set (as in the original)
        k->v.data = apMalloc(size);
        if (!k->v.data)
            return 0;
        memcpy(k->v.data, val, size);
        break;
    }
    k->type = type;
    return 1;
}
