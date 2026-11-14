// saveload.h -- Will Rock 1.2 unofficial SDK, game_systems: save/load (gs:gs_saveload)
// Assumes from re/sdk/core: fioFILE (vtbl: +0x04 Open(name,mode) +0x08 Close +0x0C Seek(pos,mode) +0x18 Write(p,n[,cnt]),
//   +0x110? cur size dword at [file+0x44*4]), fioFILE_DISK, fioFILE_MEM, fioCHUNK, msgDATA, entIACTIVE_OBJ, gsMSG_MP_SYNC (mpsrc).
// A save game is the MULTIPLAYER SYNC STREAM written to disk: every entIACTIVE_OBJ gets gsMP_SERVER::CreateSyncInfo
// (the same packet a joining MP client receives) and CreateSyncUsrInfo; then the timed-message queue. Load replays it.
#pragma once
#include "gs_common.h"

// Chunk ids (fioFILE::WriteChunk) -- same values in v12 (pushes in v12 WriteFileHeaderEx 0x00530767..0x0053088F)
enum gslCHUNK {
    GSL_CHK_HEADER     = 0x0F1,  // container
    GSL_CHK_MAGIC      = 0x0F2,  // dword 'FGSS' (0x53534746, written to gslSL+8; verified v12 0x00530066)
    GSL_CHK_SCENE      = 0x0F3,  // string: level file name+ext (fnmGetNameExt(gsSceneFullName))
    GSL_CHK_NSYNC      = 0x0F4,  // dword patched at end: number of sync records
    GSL_CHK_NSYNCUSR   = 0x0F5,  // dword patched at end: number of user-sync records
    GSL_CHK_HDR_EX     = 0x10E,  // extended header container (menu info)
    GSL_CHK_PICTURE    = 0x10F,  // pctPICTURE screenshot (gslHDR_EX::picture, pctWrite)
    GSL_CHK_DESCR      = 0x110,  // description; v12 writes a fixed 0x100-byte block (push 0x100 at v12 0x005307C8) // ? feb wrote var length
    GSL_CHK_TIME       = 0x111,  // dword gslHDR_EX::time
    GSL_CHK_NEVENTS    = 0x112,  // dword gspSysPlay->nEvents (+0x28)
    GSL_CHK_EVENT      = 0x113,  // string per gspEVENT (game-play statistics/events)
    GSL_CHK_SYNCUSR    = 0x138,  // one per CreateSyncUsrInfo record (WriteMsgSyncUsr)
};

struct gslHDR_EX {                 // passed by the menu to gslSave/gslLoad/gslGetHdrEx
    pctPICTURE *picture;           // +0x00 thumbnail (NULL = none)
    unsigned    time;              // +0x04 play time / date  // ? unit
    char        descr[0x100];      // +0x08 description shown in the load menu  // ? exact size (0x100 in v12)
};

class gslSL {                      // global instance gslSaveLoad v12 0x0099D828 (feb 0x00956FA8)
public:
    int        posNSync;           // +0x000 v12 (feb +0x000) file pos of GSL_CHK_NSYNC value
    int        posNSyncUsr;        // +0x004 v12 (feb +0x004)
    unsigned   magic;              // +0x008 v12 (feb +0x008) 'FGSS'
    char       scene[0x104];       // +0x00C v12 (feb +0x00C)
    int        nSync;              // +0x110 v12 (feb +0x110)
    int        nSyncUsr;           // +0x114 v12 (feb +0x114)
    fioFILE   *file;               // +0x118 v12 (feb +0x118) fioFILE_DISK (new 0x124 in gslInit)  = global v12 0x0099D940
    unsigned char usrBuf[10000];   // +0x11C v12: 0x2710 bytes  (feb: 0x400 bytes)  user-sync scratch buffer
    fioFILE_MEM *memFile;          // +0x282C v12 (feb +0x51C) = global v12 0x009A0054. verified v12 WriteMsgSyncUsr 0x00530F63
                                   //   (v12 calls memFile->vfunc+0x1C(usrBuf, 10000, 0x16) to wrap the buffer)
    int  Save(char *file, gslHDR_EX *hdr);          // v12 0x0052FFD0 (feb 0x0051A740) only when gsSysMP->mode(+0x28)==1 (single player)
    int  Load(char *file, gslHDR_EX *hdr);          // v12 0x00530260 (feb 0x0051A9B0) gsMsgChangeMode, gsAppState|=0x20000, SendAll 0x3EF, 0x3F1...
protected:
    int  WriteFileHeader();                         // v12 0x00530440 (feb 0x0051AB90)
    int  ReadFileHeader();                          // v12 0x005305C0 (feb 0x0051AD10)
    int  WriteFileHeaderEx(gslHDR_EX *h);           // v12 0x00530720 (feb 0x0051AE70)
    int  ReadFileHeaderEx(gslHDR_EX *h);            // v12 0x00530900 (feb 0x0051B050)
    int  WriteMsgSync(entIACTIVE_OBJ *e, gsMSG_MP_SYNC *s);   // v12 0x00530A70 (feb 0x0051B1C0)
    int  WriteMsgSyncUsr(entIACTIVE_OBJ *e, msgDATA *d);      // v12 0x00530F30 (feb 0x0051B640) sends GS_MSG 0x40F (feb 0x40D) to e
    int  ReadMsgSync();                             // v12 0x00531040 (feb 0x0051B750)
    int  ReadMsgSyncUsr();                          // v12 0x00531420 (feb 0x0051BB00)
    int  WriteTimeMsgQueue();                       // v12 0x00531570 (feb 0x0051BC50) pending delayed msgSYSTEM messages
    int  ReadTimeMsgQueue();                        // v12 0x005318A0 (feb 0x0051BF80)
};
extern gslSL gslSaveLoad;                           // v12 0x0099D828 (feb 0x00956FA8)

int  __fastcall gslInit();                          // v12 0x0052FE60 (feb 0x0051A5D0) new fioFILE_DISK(0x124) + fioFILE_MEM(0x12C)
void __fastcall gslTerm();                          // v12 0x0052FF20 (feb 0x0051A690)
int  __fastcall gslSave(char *file, gslHDR_EX *h);  // v12 0x0052FF60 (feb 0x0051A6D0)  HOOK: quick-save / autosave
int  __fastcall gslLoad(char *file, gslHDR_EX *h);  // v12 0x0052FF70 (feb 0x0051A6E0)  also reached by GS_MSG_LOAD_GAME (0x3ED)
int  __fastcall gslGetHdrEx(char *file, gslHDR_EX *h); // v12 0x0052FF80 (feb 0x0051A6F0) menu preview
// GS_MSG id used: user sync request = 0x40F in v12 (feb 0x40D)  verified push 0x40F at v12 0x00530F95.
