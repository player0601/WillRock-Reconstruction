// core_types.h -- Will Rock (Saber Interactive, 2003) UNOFFICIAL modding SDK, subsystem "engine_core"
//
// Authoritative layouts of the engine's base types: apNAME, arrVector<T>, lstNODE/lstLIST, dsDYN_ARRAY,
// msgADDR, msgMSG, msgSYSTEM, msgDATA / msgDATA_PACK, plus the ap* runtime (memory, log, cfg).
// Static analysis only. Target build: 1.2 (v12.exe). Reference: Feb 2003 build (feb.exe + WillRock.map).
//
// Conventions
//   "// v12 +0xNN (feb +0xNN)"            field offset in 1.2 and in the Feb build
//   "// v12 0x00XXXXXX (feb 0x00XXXXXX)"  address of a function / global
//   "// ?"                                meaning or v12 value not verified by an instruction
//   All classes are MSVC 6, 32-bit, __thiscall, no RTTI used by the game. 1 = TRUE in every int "BOOL".
//   ICF: MSVC folded identical bodies, so one address can carry several map names (e.g. every empty
//   "return 0" virtual). Treat folded slots as "default implementation", not as the named function.
//
// Assumed elsewhere (re/sdk/core): m3dV {float x,y,z}, m3dMATR (4x4 float, 0x40 bytes), m3dBOX, psSHEET,
// fioFILE_MEM (declared in fileio.h of this folder).

#pragma once
#include <stddef.h>

#if !defined(_MSC_VER) && !defined(__fastcall)   // let the headers parse with GCC/Clang (documentation / tooling)
#  if defined(__i386__)
#    define __fastcall __attribute__((fastcall))
#    define __cdecl    __attribute__((cdecl))
#  else
#    define __fastcall
#    define __cdecl
#  endif
#endif

typedef int BOOL32;              // the game's "int" booleans
typedef unsigned long  u32;
typedef unsigned short u16;
typedef unsigned char  u8;

struct m3dV;
struct m3dMATR;
class  psSHEET;
class  fioFILE_MEM;

// =====================================================================================================
// apNAME -- fixed 64-byte name buffer (engine "ap" = application layer, lib ap.lib)
// =====================================================================================================
// No vtable. Used embedded (entIACTIVE_OBJ::name, gsDOMAIN_*, IAction names...).
struct apNAME {
    char str[0x40];                                   // +0x00
    operator char *()             { return str; }     // [integrator] lets an apNAME be passed as a C string
    operator const char *() const { return str; }    // (str is NUL-terminated, SetName copies max 0x3F and forces str[0x3E]=0)
    void SetName(char *s);                            // v12 0x004AF580 (feb 0x004A09F0) strncpy(str,s,0x3F); str[0x3E]=0  (note: really truncates at 62 chars)
    BOOL32 IsName(char *s);                           // v12 0x004AF5A0 ? (feb 0x004A0A10) case-SENSITIVE strcmp()==0
};
// size 0x40

// =====================================================================================================
// arrVector<T> -- the engine's std::vector replacement (ap/arr*.h). Header-only template.
// =====================================================================================================
// Verified from arrVector<uiFONT_LETTER>::arrVector(int) (feb 0x00518480): data=0,cap=0,count=0;
// if (n>0) data=apRealloc(0,n*sizeof(T)), cap=n; count=n.  Storage always comes from apRealloc/apFree
// (= CRT realloc/free), so a mod may free/grow it with the CRT of the game (msvcrt is NOT used: the CRT
// is statically linked, call the game's apRealloc/apFree below, never your own CRT).
template <class T> struct arrVector {
    T   *data;                                        // +0x00
    union { int count; int size; };                   // +0x04 number of used elements (re/mpsrc calls it "size")
    int  capacity;                                    // +0x08 allocated elements
    // Header-only helpers with the game's semantics (storage from apRealloc/apFree). Not game addresses.
    T   &operator[](int i) { return data[i]; }
    void Reserve(int n);                              // grow capacity to >= n
    void Resize(int n);                               // count = n (grows)
    void Append(const T *src, int n);                 // copy n elements to the end
    void Append(const T &v) { Append(&v, 1); }
    void Insert(int pos, const T &v, int n = 1);
    void Insert(int pos, const T *v, int n);         // pointer form (re/mpsrc)
    void Add(const T &v) { Append(&v, 1); }
    void Erase(int pos, int n = 1);
    void Free();                                      // apFree(data), all zero
    void Clear() { count = 0; }
    void Realloc(int n);                              // capacity = n (frees cut elements' payloads in some T)                       // size = 0, keeps the storage
    // Methods are instantiated per T (Resize/Reserve/Realloc/Insert/Sort/FindSorted); addresses differ per T.
};
// size 0x0C (all builds)

// =====================================================================================================
// lstNODE / lstLIST -- intrusive doubly linked lists (engine:llist.obj)
// =====================================================================================================
struct lstNODE {
    lstNODE *prev;                                    // +0x00 verified lstLIST_SIMPLE::Insert v12 0x004FE060
    lstNODE *next;                                    // +0x04 msgADDR::GetNext: next = *(node+4) - 4
};

// lstLIST_SIMPLE (vtable v12 0x00606364): only the head. lstLIST (vtable v12 0x00606EA8) adds the tail and a
// count; lstPLIST adds a sorted insert by the virtual Compare. Rebuilt in src/engine/llist.cpp.
class lstLIST_SIMPLE {
public:
    virtual void *__vecDelDtor(unsigned flags);                         // [0] v12 0x004FE560 (feb 0x004EBF80)
    virtual void  Insert(lstNODE *n, lstNODE *before);                  // [1] v12 0x004FE060 (feb 0x004EBA80) before = 0: at head
    virtual void  Add(lstNODE *n);                                      // [2] v12 0x004AC310
    virtual void  Remove(lstNODE *n);                                   // [3] v12 0x004FE0B0 (feb 0x004EBAD0)
    virtual void  Clear();                                              // [4] v12 0x004FE0F0 (feb 0x004EBB10)
    lstNODE *head;                                                      // +0x04
    lstLIST_SIMPLE() : head(0) {}
    void Destroy() {}
};
class lstLIST : public lstLIST_SIMPLE {
public:
    void *__vecDelDtor(unsigned flags);                                 // [0] v12 0x004EAD50 (folded with msgADDR_LIST's)
    void  Insert(lstNODE *n, lstNODE *before);                          // [1] v12 0x004FE110 (feb 0x004EBB30)
    void  Remove(lstNODE *n);                                           // [3] v12 0x004FE180 (feb 0x004EBBA0)
    virtual int GetNumItems();                                          // [5] v12 0x004AC320 (feb 0x0049DF30)
    lstNODE *tail;                                                      // +0x08
    int      count;                                                     // +0x0C
    lstLIST() : tail(0), count(0) {}                                    // v12 0x004CFDA0
};
class lstPLIST : public lstLIST {
public:
    virtual int  Compare(lstNODE *a, lstNODE *b);                       // [6] 1: a goes before b (base folded "return 0", v12 0x004036D0)
    virtual void Insert(lstNODE *n);                                    // [7] v12 0x004FE1D0 (feb 0x004EBBF0) before the first greater
    using lstLIST::Insert;
    lstNODE *FindFirstGreater_Fast(lstNODE *n);                         // v12 0x004FE260 (feb 0x004EBC80) binary search
};
extern lstNODE *lstPListSortBuf[];                                      // v12 0x00978F58 the nodes FindFirstGreater_Fast searches // ? name

// =====================================================================================================
// dsDYN_ARRAY -- growable array of fixed-size records (engine:data_struct.obj)
// =====================================================================================================
// ctor  v12 0x004EF940 (feb 0x004DBFE0)   Init(elemSize, growBy, void **extPtr, ...) v12 0x004EF9C0 (feb 0x004DC060)
// AddEntry v12 0x004EFA40 (feb 0x004DC0E0)  Expand v12 0x004EFAB0 (feb 0x004DC150)  Term/ReleaseMem v12 0x004EFA10 (feb 0x004DC0B0)
// Layout not needed by mods. // ?

// =====================================================================================================
// ap runtime: memory, logging, ini config  (lib ap.lib: ap_mem / ap_comm / ap_cfg / ap_gpf)
// =====================================================================================================
// Memory: plain jumps into the statically linked MSVC CRT heap. There is no custom allocator, no pools
// and no tracking in retail (_apTotalAlloced feb 0x00610F88 exists but the release wrappers do not update it).
void *apMalloc(size_t n);                             // v12 0x004AF5F0 (feb 0x004A0A60)  jmp _malloc  (v12 _malloc 0x005814A2)
void *apCalloc(size_t n, size_t sz);                  // v12 0x004AF600 (feb 0x004A0A70)  jmp _calloc  (v12 0x005815DC)
void  apFree(void *p);                                // v12 0x004AF610 (feb 0x004A0A80)  jmp _free    (v12 0x00581719)
void *apRealloc(void *p, size_t n);                   // v12 0x004AF620 (feb 0x004A0A90)  jmp _realloc (v12 0x00581802)
char *apStrdup(char *s);                              // v12 0x004AF630 (feb 0x004A0AA0)  malloc+copy
void *operator_new(size_t n);                         // v12 0x0057FF90 (feb 0x005676A0)  ??2@YAPAXI@Z
// operator delete: feb 0x00568??? (CRT) -- use apFree-compatible CRT free; objects are always new'ed by the game.

// Logging: each function only forwards to a callback pointer (NULL = message dropped). The app installs
// them at start-up. A mod can capture every engine message by writing its own function into the pointer.
// Callback signature: void __cdecl cb(char *fmt, ...).
void apMsgBox(char *fmt, ...);                        // v12 0x004AF3E0 (feb 0x004A0850)
void apMsg(char *fmt, ...);                           // v12 0x004AF430 (feb 0x004A08A0)
void apLog(char *fmt, ...);                           // v12 0x004AF480 (feb 0x004A08F0)
void apLogErr(char *fmt, ...);                        // v12 0x004AF4D0 (feb 0x004A0940)
void __fastcall apSetMsgBoxCB(void (*cb)(char *, ...));          // v12 0x004AF540 (feb 0x004A09B0)
void __fastcall apSetMsgCB(void (*cb)(char *, ...));             // v12 0x004AF550 (feb 0x004A09C0)
void __fastcall apSetLogCB(void (*cb)(char *, ...));             // v12 ? (between 0x004AF550 and 0x004AF570) (feb 0x004A09D0)
void __fastcall apSetLogErrCB(void (*cb)(char *, ...));          // v12 0x004AF570 (feb 0x004A09E0)
extern void (*apMsgBoxCB)(char *, ...);               // v12 0x00655164 (feb 0x00610F84)
extern void (*apMsgCB)(char *, ...);                  // v12 0x00655160 (feb 0x00610F80)
extern void (*apLogCB)(char *, ...);                  // v12 0x00652060 (feb 0x0060F320)
extern void (*apLogErrCB)(char *, ...);               // v12 0x00654158 (feb 0x0060FF78)
extern void *apHMainWindow;                           // v12 0x00652018 (feb 0x0060F2D8)  HWND
extern void *apHInstance;                             // v12 0x0065201C (feb 0x0060F2DC)  HINSTANCE
extern char *apTitle;                                 // v12 0x00652020 (feb 0x0060F2E0)
extern char  apBuffer[];                              // v12 0x00654160 (feb 0x0060FF80)  shared sprintf scratch buffer

// INI config (Win32 GetPrivateProfileString / WritePrivateProfileString on a file, see README PORTING)
BOOL32 __fastcall apCfgInit(char *file, int mode);               // v12 0x004AF670 (feb 0x004A0AE0)
void __fastcall   apCfgTerm();                                   // v12 0x004AF730 (feb 0x004A0BA0)
BOOL32 __fastcall _apCfgReadInt  (void **cache, char *sect, char *key, int *out);        // v12 0x004AF7E0 (feb 0x004A0C50)
BOOL32 __fastcall _apCfgReadDword(void **cache, char *sect, char *key, u32 *out);        // v12 0x004AF890 (feb 0x004A0D00)
BOOL32 __fastcall _apCfgReadBool (void **cache, char *sect, char *key, int *out);        // v12 0x004AF940 (feb 0x004A0DB0) "Yes"/"No"
BOOL32 __fastcall _apCfgReadFloat(void **cache, char *sect, char *key, float *out);      // v12 0x004AF9D0 (feb 0x004A0E40)
BOOL32 __fastcall _apCfgReadString(void **cache, char *sect, char *key, char *out, int n);// v12 0x004AFA50 (feb 0x004A0EC0)
BOOL32 __fastcall apCfgWriteInt(char *sect, char *key, int v);   // v12 0x004AFAD0 (feb 0x004A0F40)
// apCfgWriteDword v12 0x004AFB20, WriteBool 0x004AFB70, WriteFloat 0x004AFBD0, WriteString 0x004AFC20 (feb 0x004A0F90..0x004A1090)
// apGPF_FILTER::SetupFilter feb 0x004A1A80: SetUnhandledExceptionFilter -> writes "ap_gpf.log" crash report.

// =====================================================================================================
// Message system: msgADDR (receiver), msgMSG (queued message), msgSYSTEM (the global dispatcher)
// =====================================================================================================
// Every game object that receives messages derives from msgADDR (entities, domains, systems).
// Dispatch order for one message (msgSYSTEM::Dispatch): PreProcessMsg (slot +8) -> if it returns 0,
// ProcessMsg (slot +4) -> if it returns 0, PostProcessMsg (slot +0xC). A non-zero return stops the chain.
//
// Built-in engine message ids (msgSYSTEM::Init registers them):
enum msgSYS_ID {
    MSG_MSG_INIT         = 1,     // "MSG_MSG_INIT"   data = create params (entity: physINIT*, ...). Not re-sent once flag 1 set.
    MSG_MSG_TERM         = 2,     // "MSG_MSG_TERM"   destroy request (PostMsg(addr,2) = delete later). Sets flag 4.
    MSG_MSG_DESTROY_ADDR = 3,     // "MSG_MSG_DESTROY_ADDR" data = the msgADDR* being destroyed; sent to every addr with flag 2
};
// Game ids (gs layer) start at 1000 = GS_MSG_FRAME. Full v12/feb table: re/mpsrc/mp_messages.h.
// 1.2 renumbered ids: +1 for feb ids >= 0x3FA, +2 for feb ids >= 0x40B. FRAME(1000), INIT_LEVEL(0x3F4),
// TERM_LEVEL(0x3F5), IACTION(0x3F8), CDT(0x3F9) are unchanged.

enum msgADDR_FLAGS {               // msgADDR::flags (+0x0C)
    MSGADDR_INITED        = 0x01,  // set after MSG_MSG_INIT was processed (INIT never delivered twice)
    MSGADDR_WANT_DESTROY_NOTIFY = 0x02, // receives MSG_MSG_DESTROY_ADDR for every other destroyed address
    MSGADDR_TERMINATING   = 0x04,  // MSG_MSG_TERM posted: only TERM is delivered from now on
    MSGADDR_DEAD          = 0x08,  // DestroyAddr called: purged (deleted) at end of SendAll(FRAME) / PurgeDestroyAddrAll
};

// vtable msgADDR: v12 0x00606E98 (feb 0x005DB918), 4 slots
class msgADDR {
public:
    virtual ~msgADDR();                                                    // slot +0x00 v12 0x004CFE50 (feb 0x004BE3C0) scalar deleting dtor (arg 1 = free)
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);   // slot +0x04 v12 0x004802A0 (feb 0x00499C50) base: return 0
    virtual BOOL32 PreProcessMsg(int id, void *data, msgADDR *from, int *ret);// slot +0x08 v12 0x004802A0 (feb 0x00499C50) base: return 0
    virtual BOOL32 PostProcessMsg(int id, void *data, msgADDR *from, int *ret);// slot +0x0C v12 0x004802A0 (feb 0x00499C50) base: return 0

    union { lstNODE node; struct { u32 field04, field08; }; }; // [aliases: re/mpsrc]
    //  node                 // +0x04 v12 +0x04 (feb +0x04) link in msgSYSTEM::addrList; GetNext() = node.next - 4
    union { u32 flags; u32 field0C; u32 addrFlags; };                // +0x0C v12 +0x0C (feb +0x0C) msgADDR_FLAGS (verified SendMsg/PostMsg/Dispatch v12 0x004D03C0..)
    u32     magic;                // +0x10 v12 +0x10 (feb +0x10) 'MADR' (0x4D414452) set by every ctor
    union { u32 classId; u32 addrId; u32 name; u32 field14; }; // +0x14 [aliases: re/mpsrc]
    //  classId v12 +0x14 (feb +0x14) FourCC class id ('PLYR'=0x504C5952, 'PHYS'=0x50485953, ...). GetClassID
    union { u32 unk18; u32 field18; };                // +0x18 ?
    union { u32 priority; u32 order; u32 field1C; }; // +0x1C [aliases: re/mpsrc]
    //  priority v12 +0x1C (feb +0x1C) sort key in addrList (InsertAddr 3rd arg, entity class "priority" 0x28/0x32/0x46...)
    union { u32 sign; u32 field20; };                 // +0x20 v12 +0x20 (feb +0x20) kind signature: 'ITNE'(0x454E5449)=entIACTIVE_OBJ, '$TNE'(0x454E5424)=entENTITY. GetSign

    u32 GetClassID();             // v12 0x00548C90 (feb 0x004F6390) return classId (feb body folded with animSYSTEM_GROUP::GetNGroup)
    u32 GetSign();                // v12 0x005089C0 (feb 0x004F63A0) return sign
    msgADDR *GetNext();           // v12 0x005089D0 (feb 0x004F63B0) next address in the global list (all entities, domains...)
};
// size 0x24

// One queued message. Two ping-pong arrays of 500 live in msgSYSTEM. Sorted by priority each frame.
struct msgMSG {                   // size 0x14 (ctor feb 0x004BE620)
    msgADDR *to;                  // +0x00
    msgADDR *from;                // +0x04
    int      id;                  // +0x08 (-1 = free)
    void    *data;                // +0x0C msgDATA* for PostMsgData (owned copy)  // ?
    int      unk10;               // +0x10
};
// Delayed message (PostTimeMsg*): list node in msgTIME_QUEUE, remaining time float at +0x10, next at +0x18 (-0x14 bias)
// A message waiting in msgSYSTEM's time queue (new 0x1C in PostTimeMsg v12 0x004D0460). GetNextTimeMsg v12 0x004D0A20
// (feb 0x004BEF90). Saved/restored by gslSL::Write/ReadTimeMsgQueue.
struct msgMSG_TIME {
    msgADDR *to;                  // +0x00
    msgADDR *from;                // +0x04
    int      id;                  // +0x08
    void    *data;                // +0x0C apMalloc'ed copy (PostTimeMsgData)
    float    time;                // +0x10 seconds left (DispatchAll subtracts dt)
    lstNODE  node;                // +0x14 link in the time queue
};

// msgDATA: every message payload that can be posted/packed/sent over the network starts with this header.
// msgHDR: the same 4 bytes seen as a plain header (re/mpsrc writes "msg.hdr.size").
struct msgHDR {
    u8    type;                   // +0x00 = msgDATA::packId
    u8    flags;                  // +0x01 = msgDATA::tag (0x40)
    u16   size;                   // +0x02
};
struct msgDATA {
    union {                       // [integrator] one layout, the names used by the different reconstructions
        struct {
            u8   packId;          // +0x00 index into msgSYSTEM::dataPack[] (RegisterMsgData); 0xFF = null on the wire
            u8   tag;             // +0x01 '@' (0x40) for a valid msgDATA (IsValidMsgData)
            short size;           // +0x02 total byte size (>0)
        };
        struct { u8 type; u8 flags; };   // mpsrc names for +0x00 / +0x01
        msgHDR hdr;               // whole header
    };
    // payload follows
};
// msgDATA_PACK: serializer for one msgDATA type. vtable v12 0x00606E90 (feb 0x005DB910)
class msgDATA_PACK {
public:
    virtual BOOL32   Pack(fioFILE_MEM *f, msgDATA *d);   // slot +0x00 v12 0x004D0B60 (feb 0x004BF0D0) default: raw copy of d->size bytes // ?
    virtual msgDATA *UnPack(fioFILE_MEM *f);             // slot +0x04 v12 0x004D0BB0 (feb 0x004BF120)
    int id;                                              // +0x04 assigned by RegisterMsgData (index), <1 before
    msgDATA_PACK() : id(-1) {}                           // inline (v12 0x004CFCA0 for msgDataPackDefault)
};
extern msgDATA_PACK msgDataPackDefault;                  // v12 0x008FB194 the plain packer, registered by msgSYSTEM::Init // ? name

// msgQUEUE (base of msgSYSTEM), msgTIME_QUEUE (vtable v12 0x00606E50, feb 0x005DB8D0, an lstPLIST),
// msgADDR_LIST (vtable v12 0x00606E70, feb 0x005DB8F0, lstPLIST sorted by msgADDR::priority:
//   Compare v12 0x004CFD60 (feb 0x004BE2B0), Insert v12 0x004CFCD0 (feb 0x004BE220)).

// msgADDR_LIST (vtable v12 0x00606E70, feb 0x005DB8F0): lstPLIST of msgADDR::node sorted by msgADDR::priority. Insert
// also numbers the addresses of one class id in insert order (msgADDR +0x18). 100 class ids.
class msgADDR_LIST : public lstPLIST {
public:
    int  Compare(lstNODE *a, lstNODE *b);                     // [6] v12 0x004CFD60 (feb 0x004BE2B0) a.priority < b.priority
    void Insert(lstNODE *n);                                  // [7] v12 0x004CFCD0 (feb 0x004BE220)
    using lstLIST::Insert;
    int  nClass;                                              // +0x10
    struct { u32 classId; int n; } cls[100];                  // +0x14 addresses inserted per class id
    msgADDR_LIST() : nClass(0) {}
};
// msgTIME_QUEUE (vtable v12 0x00606E50, feb 0x005DB8D0): lstPLIST of msgMSG_TIME::node, Compare is the base
// "return 0", so messages queue in post order.
class msgTIME_QUEUE : public lstPLIST {
public:
    void *__vecDelDtor(unsigned flags);                       // [0] v12 0x004CFF20
    void  DeleteMsgByAddr(msgADDR *a);                        // v12 0x004CFE90 (feb 0x004BE400)
    int          nReady;                                      // +0x10 messages whose time ran out (DispatchAll)
    msgMSG_TIME *cursor;                                      // +0x14 DispatchAll's walk
    msgTIME_QUEUE() : nReady(0), cursor(0) {}
};

// The one global instance: msgSystem.
class msgSYSTEM /* : msgQUEUE */ {
public:
    msgMSG       *cur;            // +0x000 v12 +0x000 (feb +0x000) array[500] being dispatched this frame
    msgMSG       *next;           // +0x004 v12 +0x004 (feb +0x004) array[500] receiving PostMsg (swapped by NextFrame)
    int           dispatchIdx;    // +0x008 (NextFrame zeroes)
    int           curCount;       // +0x00C
    int           nextCount;      // +0x010 PostMsg: next[nextCount++]  (NO overflow check: >500 posts per frame corrupts memory)
    int           capacity;       // +0x014 = 500 (0x1F4)
    int           unk18;          // +0x018 ?
    msgADDR_LIST  addrList;       // +0x01C v12 +0x01C (feb +0x01C) every address, by priority (GetFirstAddr = head - 4)
    msgTIME_QUEUE timeQueue;      // +0x350 v12 +0x350 (feb +0x350) messages posted with a delay
    int           frameNmb;       // +0x368 v12 +0x368 (feb +0x368) GetFrameNmb v12 0x00510DF0 (feb 0x004FCA20); ++ in NextFrame
    int           nRegMsg;        // +0x36C v12 +0x36C (feb +0x36C)
    struct { int id; char *name; } regMsg[100]; // +0x370 v12 +0x370 (feb +0x370) debug names (RegisterMsg; no bound check)
    int           unk690;         // +0x690 ? (gap before +0x694)
    int           nPendingDestroy;// +0x694 v12 +0x694 (feb +0x694) DestroyAddr ++ / purge --
    int           nDataPack;      // +0x698 v12 +0x698 (feb +0x698) verified v12 Init 0x004D0065
    msgDATA_PACK *dataPack[100];  // +0x69C v12 +0x69C (feb +0x69C) by msgDATA::packId (limit 100)
    int           chgModeMsgId;   // +0x82C = 0x3FC set by gsMsgInit (feb; v12 value ?, ids renumbered)
    int           purgeMsgId;     // +0x830 = 1000 (GS_MSG_FRAME): SendAll(purgeMsgId) ends with the destroy purge

    msgSYSTEM();                                                        // v12 0x004CFBA0 (static initialiser of msgSystem)
    ~msgSYSTEM();                                                       // v12 0x004CFC40 (atexit)
    void   DeleteMsgByAddr(msgADDR *a);                                 // v12 0x004CFDC0 (feb 0x004BE330) msgQUEUE's: clears to/from in both arrays
    void   Dispatch(msgADDR *to, int id, void *data, msgADDR *from, int *ret); // v12 0x004D0340 Pre/Process/PostProcessMsg
    BOOL32 Init();                                                      // v12 0x004CFF50 (feb 0x004BE4C0) new 2x500 msgMSG, registers ids 1..3
    void   Term();                                                      // v12 0x004D00D0 (feb 0x004BE640)
    void   DispatchAll(float dt);                                       // v12 0x004D01A0 (feb 0x004BE710) time queue (-= dt, fire <=0) then cur[] queue
    BOOL32 SendMsg(msgADDR *to, int id, void *data, msgADDR *from);     // v12 0x004D03C0 (feb 0x004BE930) synchronous; returns *ret of the receiver
    void   PostMsg(msgADDR *to, int id, msgADDR *from);                 // v12 0x004D0410 (feb 0x004BE980) next frame, no data
    void   PostTimeMsg(msgADDR *to, int id, msgADDR *from, float delay);// v12 0x004D0460 (feb 0x004BE9D0)
    void   PostMsgData(msgADDR *to, int id, msgDATA *d, msgADDR *from); // v12 0x004D04C0 (feb 0x004BEA30) copies d
    void   PostTimeMsgData(msgADDR *to, int id, msgDATA *d, msgADDR *from, float delay); // v12 0x004D0570 (feb 0x004BEAE0)
    void   DeleteAllMsg();                                              // v12 0x004D0650 (feb 0x004BEBC0)
    void   NextFrame();                                                 // v12 0x004D06B0 (feb 0x004BEC20) swap queues, qsort(next) by _msgQSortCompare v12 0x004CFE70, frameNmb++
    void   SendAll(int id, void *data, msgADDR *from);                  // v12 0x004D06F0 (feb 0x004BEC60) broadcast to every address in list order
    void   InsertAddr(msgADDR *a, u32 classId, u32 priority);           // v12 0x004D0820 (feb 0x004BED90) register a receiver (0 = keep current value)
    void   RemoveAddr(msgADDR *a);                                      // v12 0x004D0860 (feb 0x004BEDD0)
    void   DestroyAddr(msgADDR *a);                                     // v12 0x004D08A0 (feb 0x004BEE10) notify (msg 3) + flags |= 0xC
    void   PurgeDestroyAddrAll();                                       // v12 0x004D0920 (feb 0x004BEE90) delete every DEAD address (calls dtor slot 0 with 1)
    msgADDR *GetFirstAddr();                                            // v12 0x004D09D0 (feb 0x004BEF40)
    void   RegisterMsg(int id, char *name);                             // v12 0x004D09E0 (feb 0x004BEF50)
    msgMSG_TIME *GetNextTimeMsg(msgMSG_TIME *prev);                     // v12 0x004D0A20 (feb 0x004BEF90)
    BOOL32 IsValidMsgData(void *d);                                     // v12 0x004D0A50 (feb 0x004BEFC0)
    void   RegisterMsgData(msgDATA_PACK *p);                            // v12 0x004D0A80 (feb 0x004BEFF0)
    BOOL32 Pack(fioFILE_MEM *f, msgDATA *d);                            // v12 0x004D0AC0 (feb 0x004BF030) network/savegame serialization
    msgDATA *UnPack(fioFILE_MEM *f);                                    // v12 0x004D0B20 (feb 0x004BF090)
    int    GetFrameNmb();                                               // v12 0x00510DF0 (feb 0x004FCA20)
    void   StartFrame();                                                // feb 0x00403BE0 (inline helper, folded) // ?
};
// size 0x834 (feb: highest field +0x830; v12 same offsets verified at +0x354/+0x360/+0x368/+0x698/+0x69C)
extern msgSYSTEM msgSystem;                     // v12 0x008FA960 (feb 0x008B6720)
extern int       msgInPurgeDelete;              // v12 0x008FB19C (feb 0x008B6F5C) =1 while PurgeDestroyAddrAll deletes an object
// msgQUEUE::DeleteMsgByAddr v12 0x004CFDC0 (feb 0x004BE330); msgTIME_QUEUE::DeleteMsgByAddr v12 0x004CFE90 (feb 0x004BE400)

// =====================================================================================================
// Frame clock (gs:gs_msg.obj) -- see README "Per-frame flow"
// =====================================================================================================
extern float gsElapsedTime;        // v12 0x0098E9F4 (feb 0x009482DC) seconds of this frame, scaled, clamped to 0.1
extern float gsElapsedTimeLevel;   // v12 0x0098EA1C (feb 0x00948304) accumulated level time
extern u32   gsTime;               // v12 0x0098EA14 (feb 0x009482FC) timeGetTime() of this frame (ms)
extern float gsTimeScale;          // v12 0x006354E8 (feb 0x005F72A0) default 1.0; gsMsgSetElapsedTimeScale (<0 -> 1.0)
extern float gsFixedElapsed;       // v12 0x0098EA04 (feb 0x009482EC) if |x| >= tiny epsilon, overrides every frame dt (demo / debug)
extern float gsNextElapsedOnce;    // v12 0x006354EC (1.2 only, init -1.0) one-shot dt for the next frame, reset to -1 after use
extern float gsFPS;                // v12 0x0098EA20 (feb 0x00948308) measured fps, passed to vidDRIVER::SetFPS
extern int   eioIsUpdSuspend;      // v12 0x0098EA0C (feb 0x009482F4)
void __fastcall  gsMsgCallFrame();            // v12 0x00510B90 (feb 0x004FC7E0) one game tick: clock, dispatch, FRAME broadcast
void __fastcall  _gsMsgCalcElapsedTime();     // v12 0x005109A0 (feb 0x004FC610) QPC/timeGetTime, Sleep to >=5 ms, scale, clamp
void __fastcall  _gsMsgProcessFrameProfile(); // v12 0x00510CB0 (feb 0x004FC900) DispatchAll(dt); SendAll(1000); NextFrame()
void __fastcall  gsMsgSetElapsedTimeScale(float s); // v12 0x00510CF0 (feb 0x004FC940)
float __fastcall gsMsgGetElapsedTimeScale();  // v12 0x00510D20 (feb 0x004FC970)
void __fastcall  gsMsgSetNextElapsedTime(float t);  // v12 0x00510D30 (1.2 only, name ours) the next frame's time
extern float gsNextElapsedTime;     // v12 0x006354EC (1.2 only, name ours) -1 = none
void  gsMsgSetNextElapsed(float dt);     // v12 0x00510D30 (1.2 only; our name) sets gsNextElapsedOnce if dt > 0
void __fastcall  gsMsgChangeMode(int mode);   // v12 0x00510D50 (feb 0x004FC980) SendAll(CHG_MODE) (feb id 0x3FC)
BOOL32 __fastcall gsMsgInit();                // v12 0x005107A0 (feb 0x004FC420) registers GS_MSG_* names, starts clock

// apSTATE: a bit set (entity class masks, psSHEET::mode, ...).
#ifndef WR_APSTATE_DEFINED
#define WR_APSTATE_DEFINED
struct apSTATE {
    u32 bits;
    int IsState(u32 m) { return bits & m; }          // v12 0x00508970 (feb 0x004F6340)
};
#endif
