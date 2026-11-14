// Will Rock 1.2 unofficial SDK - engine render: textures (txm*)
// objs: drv:txm.obj (rebuilt: src/engine/txm.cpp), drv:d3d_txm.obj (the D3D driver side, not rebuilt)
// Pictures (pct*) are in picture.h.
// txmMANAGER/txmTEXTURE methods are exact matches in v12 -> offsets below verified unchanged
// (e.g. v12 txmMANAGER::Add 0x004B3410: [ebx+esi*4+0x150], cmp 0x20544558 at +0x48).
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include "picture.h"
#include <stdint.h>

class txmMANAGER;
extern txmMANAGER *txmManager;   // v12 0x006561B8 (feb 0x00611FD8) the texture manager (a txmD3D_MANAGER), 284 xrefs v12

#define TXM_MAX_TEX    1000      // TWEAK: texture slots. v12 mov ecx,0x3E8 at 0x004B314F (ctor), cmp 0x3E8 at 0x004B3446 (Add),
                                 //        0x004B364C/0x004B3664/0x004B366C (Alloc), 0x004B361D (Find); NewFrame/LoadAll/PurgeAll loop too
#define TXM_MAX_LAYER  5
#define TXM_MAGIC_TEX   0x20544558  // 'TEX '  txmTEXTURE
#define TXM_MAGIC_LAYER 0x4C544558  // 'TEXL'  txmTEXTURE_LAY (animated/layered)

// txmTEXTURE::flags (+0x04). Low bits come from the caller (Add flags, e.g. 0x40003 used by most game code,
// 0x100 for scene textures, 0xC00 for the built-in ones), the rest from the picture's extra data
// (SetTexParams v12 0x004B28A0, feb 0x004A3C40) and the manager's state.
enum txmTEX_FLAGS {
    TXM_F_CLAMP_U     = 0x00000001,  // pct extra bit 0x08; GetColorUV clamps u instead of wrapping
    TXM_F_CLAMP_V     = 0x00000002,  // pct extra bit 0x10; clamps v
    TXM_F_ALPHA       = 0x00000004,  // picture format has alpha (pctIsAlpha); GetColorUV reads alpha
    TXM_F_EXTRA_04    = 0x00000008,  // pct extra bit 0x04 (whiteLM sets it)
    TXM_F_EXTRA_02    = 0x00000010,  // pct extra bit 0x02 (detailNeutral sets it)
    TXM_F_EXTRA_40    = 0x00000020,
    TXM_F_SCENE       = 0x00000100,  // added by a scene / template: PurgeAll deletes it
    TXM_F_ADDED       = 0x00000200,  // set by every successful Add; PurgeAll keeps 0xE00
    TXM_F_NO_FILE     = 0x00000400,  // built-in (white, black...): never loaded from disk
    TXM_F_BUILTIN     = 0x00000800,
    TXM_F_EXTRA_01    = 0x00001000,
    TXM_F_EXTRA_20    = 0x00002000,
    TXM_F_COMPRESSED  = 0x00008000,  // picture format 0x0C..0x11 (DXT)
    TXM_F_UNLOADABLE  = 0x00040000,  // LoadAll skips it; PurgeAll unloads it
    TXM_F_LOADED      = 0x01000000,  // picture data set
    TXM_F_USED_FRAME  = 0x02000000,  // cleared every NewFrame
    TXM_F_CREATED     = 0x08000000,  // Create done (size / format fixed)
    TXM_F_DIRTY       = 0x10000000,  // layers changed (txmTEXTURE_LAY::Update)
    TXM_F_NOT_FOUND   = 0x20000000,  // the file was not found: Load(tex, 0) does not retry
};
// The mode of txmMANAGER::Add (from the 1.2 code; the old SDK names were wrong)
enum txmADD_MODE {
    TXM_ADD_PLAIN   = -1,            // a plain texture, nothing read (scene textures: loaded later by LoadAll)
    TXM_ADD_LAYERED = 0,             // a layered texture (txmTEXTURE_LAY)
    TXM_ADD_HEADER  = 1,             // find the file, read its header (flags from the picture)
    TXM_ADD_LOAD    = 2,             // find the file and load it now
};
enum txmTEX_STATE_OPER { TXM_STATE_SET = 0, TXM_STATE_CLEAR = 1, TXM_STATE_TOGGLE = 2 };

class txmTEXTURE {                                          // size 0x78
public:
    // vtable v12 0x00606654 (feb 0x005DB114), 14 slots (verified)
    virtual void *__vecDelDtor(unsigned flags);             // slot +0x00 v12 0x004B1F10 (feb 0x004A32B0)
    virtual int  FindDisk(char *dir, char *ext, char *out); // slot +0x04 v12 0x004B1F40 (feb 0x004A32E0)
    virtual int  Load(char *path, char *ext, int layer);    // slot +0x08 v12 0x004B20C0 (feb 0x004A3460)
    virtual void Unload(int layer);                         // slot +0x0C v12 0x004B2140 (feb 0x004A34E0)
    virtual int  Create(int w, int h, int nMip, int fmt);   // slot +0x10 v12 0x004B1F90 (feb 0x004A3330)
    virtual int  Fill(pctPIXEL *c, int layer);              // slot +0x14 v12 0x004B2160 (feb 0x004A3500)
    virtual int  Set(pctPICTURE *p, int layer);             // slot +0x18 v12 0x004B2070 (feb 0x004A3410) upload a picture
    virtual int  Update(float dt);                          // slot +0x1C v12 0x0048D590 shared "return 1"
    virtual int  RequestSize(int *w, int *h);               // slot +0x20 v12 0x004036D0 shared "return 0" (0 = ok)
    virtual int  CreateDrv(pctHEADER *h);                   // slot +0x24 v12 0x0048D590 "return 1" -- driver hooks,
    virtual int  LoadDrv(pctPICTURE *p);                    // slot +0x28 v12 0x0048D590 "return 1"    overridden by
    virtual void UnloadDrv();                               // slot +0x2C v12 0x00403A20 "ret"         txmD3D_TEX
    virtual int  GetColor(int x, int y, pctPIXEL *c, int layer); // slot +0x30 v12 0x004B39C0 "return 1"
    virtual int  SetColor(int x, int y, pctPIXEL *c, int layer); // slot +0x34 v12 0x004B39C0 "return 1"

    txmTEXTURE(char *name, int flags);                      // v12 0x004B1EC0 (feb 0x004A3260) exact
    uint32_t GetColorUV(float u, float v);                  // v12 0x004B21C0 (feb 0x004A3560) exact
    pctPICTURE *ReadResample(char *path, char *name, char *ext, int keepSize); // v12 0x004B2700 (feb 0x004A3AA0) exact
    void SetTexParams(pctHEADER *h, pctEXTRA_DATA *e);      // v12 0x004B28A0 (feb 0x004A3C40) exact

    uint32_t flags;      // v12 +0x04 (feb +0x04) txmTEX_FLAGS
    char     name[0x40]; // v12 +0x08 (feb +0x08) apNAME; txmMANAGER::Add stricmp()s it
    uint32_t magic;      // v12 +0x48 (feb +0x48) TXM_MAGIC_TEX / TXM_MAGIC_LAYER
    int      w, h;       // v12 +0x4C (feb +0x4C) set by Create
    int      nMip, fmt;  // v12 +0x54 (feb +0x54) ctor = -1
    int      _5c, _60;   // v12 +0x5C (feb +0x5C)
    int      memSize;    // v12 +0x64 (feb +0x64) pctGetMemSize of the created picture (LoadAll sums it)
    int      _68;        // v12 +0x68 (feb +0x68) ctor = -1  // ?
    int      listNmb;    // v12 +0x6C (feb +0x6C) ctor = -1; slot in txmMANAGER::tex[] (GetListNmb v12 0x004B3970 returns tex[listNmb])
    int      extra;      // v12 +0x70 (feb +0x70) pctEXTRA_DATA+4 (SetTexParams)
    void    *drvData;    // v12 +0x74 (feb +0x74)  // ? (D3D surface handle in txmD3D_TEX)
};

struct txmLAYER {                                           // 0x4C
    uint32_t flags;                                         // +0x00 TXM_F_LOADED / TXM_F_DIRTY / 0x400
    char     name[0x40];                                    // +0x04 apNAME
    u8      *data;                                          // +0x44 the layer's picture data (w, h, nMip, fmt of the texture)
    float    weight;                                        // +0x48
    void SetWeight(float w);                                // v12 0x004B2930 (feb 0x004A3CD0) sets TXM_F_DIRTY when it changes
};

class txmTEXTURE_LAY : public txmTEXTURE {                  // layered/animated texture, magic 'TEXL', size 0x228
public:
    // vtable v12 0x00606690 (feb 0x005DB150), 16 slots: overrides 0..4 (Create = jmp txmTEXTURE::Create),
    // 6, 7, 12, 13, plus Save (+0x38) and a 5-argument "return 1" (+0x3C v12 0x004EADB0)
    void *__vecDelDtor(unsigned flags);                     // v12 0x004B2A20 (feb 0x004A3DC0)
    int  FindDisk(char *dir, char *ext, char *out);         // v12 0x004B2AB0 (feb 0x004A3E50)
    int  Load(char *path, char *ext, int layer);            // v12 0x004B2BD0 (feb 0x004A3F70)
    void Unload(int layer);                                 // v12 0x004B2EE0 (feb 0x004A4280)
    int  Create(int w, int h, int nMip, int fmt);           // v12 0x004B2B10 (feb 0x004A3EB0)
    int  Set(pctPICTURE *p, int layer);                     // v12 0x004B2B20 (feb 0x004A3EC0)
    int  Update(float dt);                                  // v12 0x004B3120 (feb 0x004A44C0)
    int  GetColor(int x, int y, pctPIXEL *c, int layer);    // v12 0x004B2F30 (feb 0x004A42D0)
    int  SetColor(int x, int y, pctPIXEL *c, int layer);    // v12 0x004B2FB0 (feb 0x004A4350)
    virtual int Save(char *dir, char *ext, int layer);      // slot +0x38 v12 0x004B2E10 (feb 0x004A41B0)
    virtual int unk3C(int, int, int, int, int);             // slot +0x3C v12 0x004EADB0 shared "return 1"  // ?

    txmTEXTURE_LAY(char *name, int flags);                  // v12 0x004B2990 (feb 0x004A3D30)
    int Add(char *layerName);                               // v12 0x004B3030 (feb 0x004A43D0) add a layer by texture name; -1 = failed
    txmLAYER *Layer(int i) { return (txmLAYER *)((u8 *)this + 0x7C + i * 0x4C); } // (index -1 is used by Set/Load)

    int      nLayer;                                        // +0x78
    txmLAYER lay[TXM_MAX_LAYER];                            // +0x7C
    u8       _1f8[0x228 - 0x1F8];                           // +0x1F8 (+0x1F8 and +0x224 zeroed by the ctor)
};

// D3D specialisations (drv:d3d_txm.obj)
class txmD3D_TEX : public txmTEXTURE {
    // vtable v12 0x00606B20 (feb 0x005DB688): overrides
    // slot +0x20 RequestSize v12 0x004C1BE0 (feb 0x004B22B0); +0x24 CreateDrv v12 0x004C1BF0 (feb 0x004B22C0)
    // slot +0x28 LoadDrv v12 0x004C1C00 (feb 0x004B22D0); +0x2C UnloadDrv v12 0x004C1C10 (feb 0x004B22E0)
    // slot +0x30 GetColor v12 0x004C1C80 (feb 0x004B2350); +0x34 SetColor v12 0x004C1CA0 (feb 0x004B2370)
};

class txmMANAGER {                                          // size 0x1100
public:
    // vtable v12 0x006066D0 (feb 0x005DB190), 9 slots (verified)
    virtual void *__vecDelDtor(unsigned flags);             // slot +0x00 v12 0x004B3190 (feb 0x004A4530)
    virtual void NewFrame(float dt);                        // slot +0x04 v12 0x004B3980 (feb 0x004A4D20) tex->Update for all 1000 slots
    virtual int  HwInit();                                  // slot +0x08 v12 0x004031F0 "return 1"
    virtual void HwTerm();                                  // slot +0x0C v12 0x00403A20 "ret"
    virtual void SetSource(txmTEXTURE *t);                  // slot +0x10 v12 0x00403950 "ret 4"
    virtual int  HwDownload(txmTEXTURE *t);                 // slot +0x14 v12 0x0048D590 "return 1"  PORT: upload to GPU
    virtual void HwDelete(txmTEXTURE *t);                   // slot +0x18 v12 0x00403950 "ret 4"     PORT: free GPU texture
    virtual txmTEXTURE *MakeTex(char *name, int flags);     // slot +0x1C v12 0x004B39D0 (feb 0x004A4D70) factory
    virtual txmTEXTURE *MakeTexLayer(char *name, int flags);// slot +0x20 v12 0x004B3A40 (feb 0x004A4DE0) factory
    // txmD3D_MANAGER vtable v12 0x00606AB8 (feb 0x005DB620): NewFrame 0x004C1540, HwInit 0x004C1780, HwTerm 0x004C1800,
    //   SetSource 0x004C1560, HwDownload 0x004C1830, HwDelete 0x004C1940, MakeTex 0x004BBE60, MakeTexLayer 0x004BBE80 (v12)

    txmMANAGER();                                           // v12 0x004B3140 (feb 0x004A44E0)
    void Destroy();                                         //          (feb 0x004A4580 ~txmMANAGER)
    int  Init(char *texPath, char *ext);                    // v12 0x004B3220 (feb 0x004A45C0) + the built-in textures
    txmTEXTURE *Add(char *name, int flags, txmADD_MODE mode); // v12 0x004B3410 (feb 0x004A47B0) HOOK: every texture by name
    txmTEXTURE *Add(char *name, int flags, int mode) { return Add(name, flags, (txmADD_MODE)mode); }
    void Del(txmTEXTURE *t);                                // v12 0x004B35C0 (feb 0x004A4960)
    txmTEXTURE *Find(char *name);                           // v12 0x004B35F0 (feb 0x004A4990)
    txmTEXTURE *Alloc(char *name, int flags, int layered);  // v12 0x004B3640 (feb 0x004A49E0)
    int  Load(txmTEXTURE *t, int retry);                    // v12 0x004B36D0 (feb 0x004A4A70)
    int  LoadAll(int (__fastcall *onFail)(txmTEXTURE *));   // v12 0x004B37C0 (feb 0x004A4B60) returns the failures
    int  PurgeAll();                                        // v12 0x004B3830 (feb 0x004A4BD0)
    void MngTexState(int bits, txmTEX_STATE_OPER op);       // v12 0x004B38D0 (feb 0x004A4C70)
    txmTEXTURE *GetNext(txmTEXTURE *t);                     // v12 0x004B3930 (feb 0x004A4CD0) iterate all textures
    txmTEXTURE *GetListNmb(int i) { return tex[i]; }        // v12 0x004B3970 (feb 0x004A4D10) inline copy

    uint32_t flags;               // v12 +0x04 (feb +0x04) bit 0: keep layer pictures in memory (txmTEXTURE_LAY::Load)
    char     searchPath[0x104];   // v12 +0x08 (feb +0x08) texture directory list for fnmFindDisc
    char     ext[0x40];           // v12 +0x10C (feb +0x10C) texture file extension(s) // ? size
    int      nextFree;            // v12 +0x14C (feb +0x14C) Alloc starts its search here
    txmTEXTURE *tex[TXM_MAX_TEX]; // v12 +0x150 (feb +0x150)
    txmTEXTURE *white, *whiteLM, *black, *detailNeutral; // v12 +0x10F0 (feb +0x10F0) 1x1 built-ins (Init)
};
