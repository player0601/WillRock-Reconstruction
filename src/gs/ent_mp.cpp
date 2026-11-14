// ent_mp.cpp -- gs.lib the multiplayer side of entities (Will Rock 1.2, v12.exe; feb obj gs:ent_mp.obj)
//
// Only entPackInitDataPacked so far (entRegister keeps the packed INIT data of an entity so a server can send it
// to clients that join later). 1.2 packs into a 10000-byte buffer (feb 0x400). The rest of ent_mp.obj (the MP
// sync slots of entENTITY, entUnpackInitDataPacked ...) is not rebuilt.

#include "../Sources/game.h"

extern fioFILE_MEM *gsMemFile;                      // v12 0x009A0054 the shared memory file (owner not rebuilt)
static u8 _entInitPackBuf[10000];                   // v12 0x0099D944 (feb 0x009570C4, 0x400 bytes)

// v12 0x0050AAE0  (feb 0x004F8170)
BOOL32 __fastcall entPackInitDataPacked(entENTITY *e, msgDATA *d)
{
    if (!d)
        return 1;
    gsMemFile->Init(_entInitPackBuf, 10000, 0x16);
    msgSystem.Pack(gsMemFile, d);
    short n = (short)gsMemFile->size;
    void *p = apMalloc(n);
    e->extraBuf = p;
    if (!p)
        return 0;
    memcpy(p, _entInitPackBuf, n);
    e->extraBufLen = n;
    gsMemFile->Term();
    return 1;
}

// v12 0x0050A400
// The MP update period: below 0 a third of a second, at least 0.025 s, at most 10 s.
void entIACTIVE_OBJ::SetMPUpdPeriod(float t)
{
    if (t < 0.0f)
        mpUpdPeriod = 1.0f / 3.0f;
    else if (t < 0.025f)
        mpUpdPeriod = 0.025f;
    else if (t > 10.0f)
        mpUpdPeriod = 10.0f;
    else
        mpUpdPeriod = t;
}
