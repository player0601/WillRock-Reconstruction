// fileio.h -- Will Rock 1.2 unofficial SDK, engine_core: file IO (fioFILE*), file names / search paths
//             (fnm*), chunked binary format and the scene/template readers (sio*)
//
// objs covered: engine:fileio.obj, engine:fname.obj, engine:sceneio.obj, sceneio_misc.obj, sceneio_scn.obj,
//               sceneio_obj.obj, sceneio_tpl.obj
// Assumed: psSHEET, scnSCENE, animTPL (anim.h), objOBJ (object.h), m3dSPL, m3dVOL, camCAMERA, selSEL.
//
// There is NO archive / pack file system: every asset is a loose file opened with the C runtime
// (fopen/fread/fseek/fwrite/stat/access) through fioFILE_DISK, or read whole into memory (fioFILE_MEM).
// Name lookup is case-insensitive through a cached directory listing (fnmFindDisc, _findfirst/_findnext).
// All layouts identical feb/v12 (every method "exact"; vtables verified slot by slot).

#pragma once
#include "core_types.h"
class scnSCENE; class animTPL; class objOBJ; class psSHEET;

enum fioFLAGS {                    // fioFILE::flags (+0x04), the "mode" argument of Init
    FIO_READ      = 0x01,          // size taken from stat()
    FIO_WRITE     = 0x02,          // "wb" (with READ: "rb+")
    FIO_CHUNKS    = 0x04,          // keep a chunk table; Term() back-patches chunk sizes (write) -- level/tpl loaders pass 5
    FIO_SWAP      = 0x08,          // byte-swap every element on read (ReadData elemSize) -- big-endian data support
    FIO_EXT_BUF   = 0x10,          // fioFILE_MEM: buffer supplied by caller, never freed/grown
    FIO_ERROR     = 0x20,          // sticky error (short read/write, seek failure, OOM). Term() returns !error
};
enum fioSEEK { FIO_SEEK_CUR = 0, FIO_SEEK_SET = 1, FIO_SEEK_END = 2 };

#pragma pack(push, 1)
struct fioCHUNK {                  // 14 bytes in memory, 6 bytes on disk (id + size)
    u16  id;                       // +0x00 chunk id (0xFFFF = none, 1 = end marker "last chunk")
    u32  size;                     // +0x02 byte size of the chunk body (SkipChunk seeks by it)
    u32  start;                    // +0x06 file position of the header
    u32  index;                    // +0x0A index in the writer's chunk table
};
#pragma pack(pop)

// vtable fioFILE v12 0x00605548 (feb 0x005DA0B8) (7 slots, verified)
class fioFILE {
public:
    virtual void *__vecDelDtor(unsigned flags);              // slot +0x00 v12 0x00492840 (feb 0x00487630) deleting dtor (MSVC slot 0)
    virtual BOOL32 Init(char *name, int flags);              // slot +0x04 v12 0x004E76D0 (feb 0x004D7A40)
    virtual BOOL32 Term();                                   // slot +0x08 v12 0x004E7760 (feb 0x004D7AD0)
    virtual void   Seek(int ofs, fioSEEK how);               // slot +0x0C v12 0x004E77F0 (feb 0x004D7B60)
    virtual BOOL32 IsEOF();                                  // slot +0x10 v12 0x004E7820 (feb 0x004D7B90)
    virtual void   ReadData(void *dst, int bytes, int elemSize);  // slot +0x14 v12 0x004E7A60 (feb 0x004D7DD0)
    virtual void   WriteData(void *src, int bytes, int elemSize); // slot +0x18 v12 0x004E7BB0 (feb 0x004D7F20)
    u32    flags;                  // +0x004 fioFLAGS
    char   name[0x104];            // +0x008 MAX_PATH copy of the file name
    u32    pos;                    // +0x10C current position
    u32    size;                   // +0x110 file size (stat) / written size
    int    nChunk;                 // +0x114 chunk table entries
    int    capChunk;               // +0x118 (grows by 100)
    u32   *chunkTbl;               // +0x11C pairs {start, end}
    // non-virtual helpers
    void   WriteChunk(u16 id, fioCHUNK *c);                  // v12 0x004E7840 (feb 0x004D7BB0)
    void   UpdateChunk(fioCHUNK *c);                         // v12 0x004E78E0 (feb 0x004D7C50)
    int    ReadChunk(fioCHUNK *c);                           // v12 0x004E7910 (feb 0x004D7C80) returns id
    void   SkipChunk(fioCHUNK *c);                           // v12 0x004E7960 (feb 0x004D7CD0)
    void   RevertChunk(fioCHUNK *c);                         // v12 0x004E7980 (feb 0x004D7CF0) seek back to header (0 = -6)
    void   WriteLastChunk();                                 // v12 0x004E79B0 (feb 0x004D7D20) id 1
    BOOL32 ReadLastChunk();                                  // v12 0x004E7A00 (feb 0x004D7D70)
    void   Read(char *str, int max);                         // v12 0x004E7B30 (feb 0x004D7EA0) NUL-terminated string
    void   Write(char *str);                                 // v12 0x004E7BE0 (feb 0x004D7F50)
    class m3dSPL *ReadSpline();                              // v12 0x004E7C10 (feb 0x004D7F80) type, nKp, key points
    BOOL32 ReadVol(class m3dVOL *vol);                       // v12 0x004E7C80 (feb 0x004D7FF0) into an existing volume (type must match)
    class m3dVOL *ReadVol();                                 // v12 0x004E7DD0 (feb 0x004D8140) m3dVOL::Make(type) + ReadVol
    // [integrator] typed wrappers used by re/mpsrc (inlined ReadData/WriteData calls in the game)
    template <class T> void Read (T *dst, int bytes, int elemSize) { ReadData((void *)dst, bytes, elemSize); }
    template <class T> void Write(T *src, int bytes, int elemSize) { WriteData((void *)src, bytes, elemSize); }
};
// size 0x120

// vtable fioFILE_DISK v12 0x00605564 (feb 0x005DA0D4)
class fioFILE_DISK : public fioFILE {
public:
    // slots 1..6 overridden: Init v12 0x004E7E20 (feb 0x004D8190) fopen("rb"/"wb"/"rb+"),
    //   Term v12 0x004E7F20 (feb 0x004D8290) fclose, Seek v12 0x004E7FC0 (feb 0x004D8330) fseek,
    //   ReadData v12 0x004E8030 (feb 0x004D83A0) fread, WriteData v12 0x004E80E0 (feb 0x004D8450) fwrite
    BOOL32 Init(char *name, int flags);                      // slot +0x04 v12 0x004E7E20 (feb 0x004D8190)
    BOOL32 Term();                                           // slot +0x08 v12 0x004E7F20 (feb 0x004D8290)
    void   Seek(int ofs, fioSEEK how);                       // slot +0x0C v12 0x004E7FC0 (feb 0x004D8330)
    void   ReadData(void *dst, int bytes, int elemSize);     // slot +0x14 v12 0x004E8030 (feb 0x004D83A0)
    void   WriteData(void *src, int bytes, int elemSize);    // slot +0x18 v12 0x004E80E0 (feb 0x004D8450)
    void *fp;                      // +0x120 FILE* (CRT)
    static void DeleteFile(const char *name);                // v12 0x004E8140 (feb 0x004D84B0)
};
// size 0x124

// vtable fioFILE_MEM v12 0x00608394 (feb 0x005DCEBC) 10 slots
class fioFILE_MEM : public fioFILE {
public:
    // Init(name,flags) v12 0x004E8150 (feb 0x004D84C0): READ = load whole file with fopen/fread; WRITE = 100001-byte buffer
    // Term v12 0x004E8370, Seek 0x004E8480, ReadData 0x004E84C0, WriteData 0x004E8570 (feb 0x004D86E0..0x004D88E0)
    // slot +0x1C Init(void *buf, int size, int flags) v12 0x004E82F0 (feb 0x004D8660)  -- wrap an existing buffer
    // slot +0x20 GetBuf()    v12 0x00531B00 (feb 0x0051C1E0)
    // slot +0x24 GetCurPtr() v12 0x00531B10 (feb 0x0051C1F0)
    BOOL32 Init(char *name, int flags);                      // slot +0x04 v12 0x004E8150 (feb 0x004D84C0)
    BOOL32 Term();                                           // slot +0x08 v12 0x004E8370 (feb 0x004D86E0)
    void   Seek(int ofs, fioSEEK how);                       // slot +0x0C v12 0x004E8480 (feb 0x004D87F0)
    void   ReadData(void *dst, int bytes, int elemSize);     // slot +0x14 v12 0x004E84C0 (feb 0x004D8830)
    void   WriteData(void *src, int bytes, int elemSize);    // slot +0x18 v12 0x004E8570 (feb 0x004D88E0)
    virtual BOOL32 Init(void *buf, int size, int flags);     // slot +0x1C v12 0x004E82F0 (feb 0x004D8660) wrap a buffer (0x11 read, 0x12 write)
    virtual u8    *GetBuf();                                 // slot +0x20 v12 0x00531B00 (feb 0x0051C1E0)
    virtual u8    *GetCurPtr();                              // slot +0x24 v12 0x00531B10 (feb 0x0051C1F0)
    u8   *buf;                     // +0x120
    u8   *cur;                     // +0x124
    int   capacity;                // +0x128 (write buffer starts at 0x186A1 = 100001 bytes)
};
// size 0x12C. Used for savegames, network packets (msgSYSTEM::Pack/UnPack), demo recording.

BOOL32 __fastcall fioIsExist(char *name);                    // v12 0x004E7690 (feb 0x004D7A00) access(name,0)==0
BOOL32 __fastcall fioIsReadOnly(char *name);                 // feb 0x004D7A20 access(name,2)!=0

// ------------------------------------------------------------------------------------------------
// File names (fname.obj). All functions return a pointer to ONE static 0x200 buffer (fnmBuffer):
// copy the result before calling another fnm* function.
// ------------------------------------------------------------------------------------------------
BOOL32 __fastcall fnmInit();                                 // v12 0x004FEC40 (feb 0x004EC660) exe directory via GetModuleFileNameA
void   __fastcall fnmTerm();                                 // v12 0x004FECD0 (feb 0x004EC6F0)
char * __fastcall fnmAddExt(char *name, char *ext);          // v12 0x004FED70 (feb 0x004EC790) replace/append extension
char * __fastcall fnmMakeFullName(char *dir, char *name, char *ext, int forceExt); // v12 0x004FEE40 (feb 0x004EC860)
char * __fastcall fnmGetPath(char *full);                    // v12 0x004FEF60 (feb 0x004EC980)
char * __fastcall fnmGetNameExt(char *full);                 // v12 0x004FEFE0 (feb 0x004ECA00)
char * __fastcall fnmGetName(char *full);                    // v12 0x004FF040 (feb 0x004ECA60)
char * __fastcall fnmFindDisc(char *dir, char *name, char *ext); // v12 0x004FF0B0 (feb 0x004ECAD0) case-insensitive lookup in a
                                                             //   cached fnmDIR listing of 'dir' (rebuilt on miss); returns real-case path or 0
char * __fastcall _fnmFindDisc(char *dir, char *name, char *ext);// v12 0x004FF190 (feb 0x004ECBB0) direct access() + _findfirst scan
// fnmDIR -- a cached directory listing (fnmFindDisc). The static fnmDirRoot heads the list of cached
// directories (root.next); each one holds its files and, through sub / sub->next, its subdirectories.
class fnmDIR {
public:
    char   name[0x104];            // +0x000 directory path as given
    int    nFiles;                 // +0x104
    char **files;                  // +0x108 file names (apStrdup), no directories
    fnmDIR *next;                  // +0x10C next sibling (or next cached directory)
    fnmDIR *sub;                   // +0x110 first subdirectory
    ~fnmDIR();                                               // v12 0x004FEBB0 (feb 0x004EC5D0)
    void   Term();                                           // v12 0x004FF4B0 (feb 0x004ECED0)
    char  *FindName(char *name, char *ext);                  // v12 0x004FF540 (feb 0x004ECF60)
    BOOL32 RebuildContent(char *dir);                        // v12 0x004FF7E0 (feb 0x004ED200)
    void  *__scalarDelDtor(unsigned flags);                  // v12 0x004FFAA0 (feb 0x004ED4C0) ??_G
};
// size 0x114
extern fnmDIR fnmDirRoot;          // v12 0x00984AE0 (feb 0x009408A0) _$E6 v12 0x004FEAF0 / _$E4 0x004FEB20
extern char  fnmPathSep;           // v12 0x00634B70 (feb 0x005F6940) = '\\' (set by fnmInit)
extern char  fnmBuffer[0x200];     // v12 0x00984BF8 (feb 0x009409B8) shared result buffer
extern char *fnmExeDir;            // v12 0x00984DF8 (feb 0x00940BB8) directory of the exe

// ------------------------------------------------------------------------------------------------
// Scene / template readers (sceneio*.obj). Files are fioCHUNK streams.
// ------------------------------------------------------------------------------------------------
// Chunk ids seen (feb == v12):
//   0x0F0 object tree   0x154/0x155 textures  0x1B8 instance list / 0x1B9 one instance / 0x1CC instance extra
//   0x1EA template list 0x21C collision (old) 0x21D named selections 0x21E global scene script (psSHEET)
//   0x21F collision     0x280 lights          0x2E4/0x2E5 template header (".tpl")  0x3AC camera   0x001 end
// .tpl file = 0x2E4 { 0x2E5 header: u32 'APTL', char tplName[64], char className[64] } [textures] object tree,
//   skin list, object anim list, anim sequences, bounding volumes, property sheet ("script"), named
//   selections, end chunk.  gsLVL_SYSTEM::LoadTpl v12 0x00537720 (feb 0x00521E60):
//   fnmMakeFullName(levelDir, name, "tpl", 1) -> fioFILE_DISK::Init(path, READ|CHUNKS) -> sioReadTemplate(tpl, 1).
class selSEL; class camCAMERA; class cdtCOLL_SCN;
void   __fastcall sioSetSourceFile(fioFILE *f);              // v12 0x0059E670 (feb 0x00582340)
BOOL32 __fastcall sioReadScene(scnSCENE *s);                 // v12 0x0059E680 (feb 0x00582350)
BOOL32 __fastcall sioReadSceneTex();                         // v12 0x0059E840 (feb 0x00582510)
BOOL32 __fastcall sioReadTemplate(animTPL *t, int withTex);  // v12 0x0059E900 (feb 0x005825D0)
// sceneio_misc.obj
BOOL32 __fastcall _sioReadLastChunk();                       // v12 0x0059EA80 (feb 0x00582750)
BOOL32 __fastcall _sioReadTex();                             // v12 0x0059EAB0 (feb 0x00582780) texture list -> txmManager, sioTexListNmb
BOOL32 __fastcall _sioReadPSheet(psSHEET **out);             // v12 0x0059ED10 (feb 0x005829E0)
BOOL32 __fastcall _sioReadNamedSel(selSEL *sel);             // v12 0x0059EDD0 (feb 0x00582AA0)
BOOL32 __fastcall _sioReadNamedSelList(int *n, selSEL **list); // v12 0x0059EF00 (feb 0x00582BD0)
camCAMERA *__fastcall _sioReadCamera();                      // v12 0x0059EFF0 (feb 0x00582CC0)
// sceneio_scn.obj
BOOL32 __fastcall _sioReadInstList(scnSCENE *s);             // v12 0x0059F1C0 (feb 0x00582E90)
BOOL32 __fastcall _sioReadTplList(scnSCENE *s);              // v12 0x0059F350 (feb 0x00583020)
BOOL32 __fastcall _sioReadLightList(scnSCENE *s);            // v12 0x0059F3E0 (feb 0x005830B0)
cdtCOLL_SCN *__fastcall _sioReadColl();                      // v12 0x0059F790 (feb 0x00583460) chunk 0x21F
cdtCOLL_SCN *__fastcall _sioReadColl_OLD();                  // v12 0x0059F9D0 (feb 0x005836A0) chunk 0x21C
// sceneio_obj.obj
objOBJ *__fastcall _sioReadObj();                            // v12 0x0059FBA0 (feb 0x00583870)
// sceneio_tpl.obj
BOOL32 __fastcall _sioReadAnimSeq(animTPL *t);               // v12 0x005A0BD0 (feb 0x00584710)
BOOL32 __fastcall _sioReadObjAnimList(animTPL *t);           // v12 0x005A0E20 (feb 0x00584960)
BOOL32 __fastcall _sioReadBVolList(animTPL *t);              // v12 0x005A1200 (feb 0x00584CE0)
BOOL32 __fastcall _sioReadObjSkinList(animTPL *t);           // v12 0x005A1440 (feb 0x00584F20)
extern fioFILE *sioFilePtr;        // v12 0x009AF8A0 (feb 0x0096442C) current source file
extern int     *sioTexListNmb;     // v12 0x009AF898 (feb 0x00964424)
