// level.h -- Will Rock 1.2 unofficial SDK, game_systems: level system (gs:gs_level, gs_level_init, gs_level_prel)
// Assumes from re/sdk/core: msgADDR (base, 0x2C bytes: vtbl,+4..+0x28, +0x10 'RDAM' tag, +0x1C priority),
//   apNAME (0x40 char name), psSHEET (0x10), fioFILE_MEM, scnSCENE, objOBJ, animINST, animTPL, txmTEXTURE.
// Layout of gsLVL_SYSTEM is IDENTICAL in feb and v12 (ctor v12 0x00536700 == feb 0x00520F00 field by field).
#pragma once
#include "gs_common.h"

// One preloaded level/scene file kept in memory between loads (array of 10 in gsLVL_SYSTEM)
struct gsLVL_PRELOAD {                     // size 0x48
    char         name[0x40];               // +0x00 scene name ("SCENE_%d" name= from <PathLev>\<lev_xx>.dsl, or level file)
    fioFILE_MEM *file;                     // +0x40 whole .lg file read into memory (fioFILE_MEM, new(300))
    void        *data;                     // +0x44 // ? loaded data handle, cleared in PreloadInit
};

// Texture -> material substitution entry (two tables of 50 at +0x714, ctor loops 2 x 0x1A94)
struct gsLVL_TEX2MTL { char raw[0x88]; };  // ctor v12 0x00537920 (feb 0x00522060)  // ? fields
struct gsLVL_TEX2MTL_TABLE {               // size 0x1A94
    int            count;                  // +0x0000
    gsLVL_TEX2MTL  e[0x32];                // +0x0004
};

enum gsLVL_PRELOAD_FLAGS {                 // gsLVL_SYSTEM::preloadFlags (+0x28), from WILLROCK.INI [Level]
    LVLPRE_COMM    = 0x01,  // PreloadComm   : preload list <PathLev>\default.dsl
    LVLPRE_MP      = 0x02,  // PreloadMP     : preload list <PathLev>\m_play.dsl
    LVLPRE_SCN     = 0x04,  // PreloadScn
    LVLPRE_SCNUPD  = 0x08,  // PreloadScnUpd
    LVLPRE_LEV     = 0x10,  // PreloadLev    : keep all scenes of a level (lev_NN_*) in memory, list from <lev>.dsl [SCENE_n] name
};

// gspSTART_GAME_DATA: payload of START_GAME / LOAD_LEVEL (0x3EC) / NEW_MAP, 0x10C bytes (from re/mpsrc/gs_play.h).
// gsLVL_SYSTEM keeps the current one at +0x3C3C (re/mpsrc/wr_go_mp.cpp).
#pragma pack(push, 1)
struct gspSTART_GAME_DATA : msgDATA {    // packId 0, flags 0x40, size 0x10C
    char level[260];                     // +0x004 level file ([Level] File)
    int  mode;                           // +0x108 gsMP_MODE (1 Single, 2 Server, 3 Client, 4 ServerOnly; -1 = not set)
};
#pragma pack(pop)
class gsLVL_SYSTEM : public msgADDR {      // size >= 0x3D90 (wrLVL_SYSTEM, new in gsAPP_SYSTEM::InitAll)  vtbl v12 0x00608430 (feb 0x005DCF58)
public:
    // msgADDR +0x00..+0x27 ; tag +0x10 = 'RDAM', +0x1C = 0x0A (priority)  verified ctor v12 0x005367C1
    int            addrLock;               // +0x024 msgADDR's lock count (msgSYSTEM skips a locked address; the C++ msgADDR
                                           //   ends at 0x24, each class declares it; zeroed by the ctor)
    unsigned       preloadFlags;           // +0x028 v12 +0x028 (feb +0x028) LVLPRE_*  verified v12 PreloadInit 0x00542B60
    char           pathScn[0x104];         // +0x02C v12 (feb +0x02C) INI [Level] PathScn (scene .lg files) verified v12 0x00536873
    char           pathDB[0x104];          // +0x130 v12 (feb +0x130) INI [Level] PathDB  (templates/database) verified v12 0x00536856
    char           pathLev[0x104];         // +0x234 v12 (feb +0x234) INI [Level] PathLev (.dsl level descriptions) verified 0x0053688D
    char           anchorInst[0x40];       // +0x338 v12 (feb +0x338) reload-anchor instance name (PrepareReloadAnchor)
    m3dMATR        anchorMatr;             // +0x378 v12 (feb +0x378) anchor matrix before reload  // ? type (0x40 bytes)
    psSHEET       *preloadDefault;         // +0x3B8 v12 (feb +0x3B8) default.dsl   verified v12 PreloadTerm
    psSHEET       *preloadMP;              // +0x3BC v12 (feb +0x3BC) m_play.dsl
    apNAME         curLevelName;           // +0x3C0 v12 (feb +0x3C0) "lev_NN" prefix of the preloaded level
    apNAME         curSceneName;           // +0x400 v12 (feb +0x400) last scene name
    int            nPreload;               // +0x440 v12 (feb +0x440) <= 10
    gsLVL_PRELOAD  preload[10];            // +0x444 v12 (feb +0x444)  (ctor loop 10 x 0x48 at v12 0x0053675A)
    gsLVL_TEX2MTL_TABLE tex2mtl[2];        // +0x714 v12 (feb +0x714) bump/extra material tables (InitBumpMtl, _AddExtraObjMtls)

    // vtable (v12 0x00608430, 17 slots; wrLVL_SYSTEM v12 0x006061E8 adds slot +0x44)
    virtual ~gsLVL_SYSTEM();                                         // slot +0x00 v12 0x005367D0 (feb 0x00520FD0)
    virtual int  ProcessMsg(int id, void *data, msgADDR *from, int *res); // slot +0x04 v12 0x00537540 (feb 0x00521C80) LOAD_LEVEL/LOAD_GAME/INIT_MAP/TERM_MAP/INIT/TERM_LEVEL
    virtual int  ProcessINIT();                                      // slot +0x08 (stub return 0)
    virtual int  ProcessTERM();                                      // slot +0x0C (stub)
    virtual int  ProcessINIT_LEVEL();                                // slot +0x10 v12 0x00537810 (feb 0x00521F50)
    virtual void ProcessTERM_LEVEL();                                // slot +0x14 v12 0x00401380 (empty)
    virtual void ProcessFRAME();                                     // slot +0x18 v12 0x00401380 (wrLVL: v12 0x004AB420 feb 0x0049D160)
    virtual int  LoadLevel(gsLOAD_SCENE_PARAM *p);                   // slot +0x1C v12 0x00536910 (feb 0x00521110) (wrLVL: v12 0x004ABA20)
    virtual void DestroyLevel(gsLOAD_SCENE_PARAM *p);                // slot +0x20 v12 0x00536B90 (feb 0x005213A0)
    void UnloadLevel(void *p) { DestroyLevel((gsLOAD_SCENE_PARAM *)p); } // re/mpsrc name for slot +0x20
    virtual animTPL *FindTplName(char *name);                       // slot +0x24 v12 0x00537230 (feb 0x00521AC0) loads template from PathDB on demand
    virtual void ObjLoadNotify(objOBJ *o);                           // slot +0x28 v12 0x00535680 (feb 0x0051FD00) -> SetExtraObjFlags etc.
    virtual void InstLoadNotify(animINST *i);                        // slot +0x2C v12 0x00535700 (feb 0x0051FD80) (wrLVL v12 0x004ABA50 spawns entities)
    virtual void ObjDestroyNotify(objOBJ *o);                        // slot +0x30 v12 0x005357A0 (feb 0x0051FE20)
    virtual void InstDestroyNotify(animINST *i);                     // slot +0x34 v12 0x00535800 (feb 0x0051FE80)
    virtual void TplDestroyNotify(int);                              // slot +0x38 v12 0x00535870 (feb 0x0051FEF0)
    virtual void TplDestroyNotify(animTPL *t);                       // slot +0x3C v12 0x00535850 (feb 0x0051FED0)
    virtual int  InitBumpMtl();                                      // slot +0x40 v12 0x00535600 (feb 0x0051FC80) (wrLVL v12 0x004AAE50)

    gsLVL_SYSTEM();                                                  // v12 0x00536700 (feb 0x00520F00)
    int  Init();                                                     // v12 0x00536810 (feb 0x00521010) new scnSCENE(0x8C) -> gsScenePtr, read paths, PreloadInit
    void Term();                                                     // v12 0x005368D0 (feb 0x005210D0)
    int  PrepareReloadAnchor(gsLOAD_SCENE_PARAM *p);                 // v12 0x00536D70 (feb 0x00521570)
    int  ApplyReloadAnchor(gsLOAD_SCENE_PARAM *p);                   // v12 0x00536E20 (feb 0x00521620) SendAll GS_MSG_SET_LEVEL_ANCHOR (0x3FB)
    int  LoadInstAll();                                              // v12 0x00536EF0 (feb 0x005216F0) instantiate every animINST of the scene
    int  LoadTpl(char *name, animTPL *t);                            // v12 0x00537720 (feb 0x00521E60)
    void SetExtraObjFlags(objOBJ *o);                                // v12 0x00535900 (feb 0x0051FF80) "&tag" name parsing (see below)
    void SetExtraLightFlags(scnSCENE *s);                            // v12 0x00535CB0 (feb 0x00520330)
    void SetTexAnimObjFlags(objOBJ *o);                              // v12 0x00535CC0 (feb 0x00520340)
    void AddExtraObjMtls(objOBJ *o);                                 // v12 0x005360C0 (feb 0x00520740)
    void _AddExtraObjMtls(objOBJ *o);                                // v12 0x00536130 (feb 0x005207B0)
    int  PreloadInit();                                              // v12 0x00542B60 (feb 0x0052D0D0) INI flags + default.dsl / m_play.dsl
    void PreloadTerm();                                              // v12 0x00542D50 (feb 0x0052D2C0)
    int  PreloadInitLevel(char *file);                               // v12 0x00542DB0 (feb 0x0052D320) decide which .lg files to keep in RAM
    void PreloadTermLevel();                                         // v12 0x00543160 (feb 0x0052D6D0)
    int  PreloadLoadScene(char *file);                               // v12 0x005431E0 (feb 0x0052D750) load scene from preloaded memory file or disk
    void PreloadUnloadData();                                        // v12 0x00543420 (feb 0x0052D990)
    void UpdatePreloadData(int);                                     // v12 0x005436E0 (feb 0x0052DC50?) the preload info (off: returns) -- not rebuilt
    void PreloadList(psSHEET *dsl);                                  // v12 0x005438F0 (feb 0x0052DE40) preload every template listed in a .dsl
    void PreloadDumpInfo();                                          // feb 0x0052DC50 (not matched in v12 // ?)
    // [integrator] re/mpsrc reads the current start data here (wr_go_mp.cpp, v12 offset +0x3C3C):
    gspSTART_GAME_DATA &StartData() { return *(gspSTART_GAME_DATA *)((char *)this + 0x3C3C); }
};

// Free functions (fastcall)
int  __fastcall gsLoadLevel1(gsLOAD_SCENE_PARAM *p);       // v12 0x00536940 (feb 0x00521140) nested load (counter v12 0x009A5A3C? feb 0x0095CEAC) // ? v12 global
int  __fastcall __gsLoadLevel(gsLOAD_SCENE_PARAM *p);      // v12 0x00536950 (feb 0x00521150) THE level load: anchor, DestroyLevel, txm LoadAll,
                                                           //   copy name to gsSceneFullName, PreloadInitLevel, PreloadLoadScene, LoadInstAll,
                                                           //   SendAll 0x3F6, 0x3F4 (INIT_LEVEL), gsAppState |= 0x40000, 0x3F7, [0x3F1]
int  __fastcall _gsLvlLoadTexErrorCB(txmTEXTURE *t);       // v12 0x00536EE0 (feb 0x005216E0)
void __fastcall _gsLvlCalcCDT2(scnSCENE *s);               // v12 0x00537870 (feb 0x00521FB0) collision tree build
int  __fastcall gsLvlInstLoadNotify(animINST *i, void *);  // v12 0x005358C0 (feb 0x0051FF40)
int  __fastcall gsLvlInstDestroyNotify(animINST *i, void*);// v12 0x005358E0 (feb 0x0051FF60)
int  __fastcall _gsLvlPreloadListFind(psSHEET *s, char *);
// What the destroy notifies call on the mirror and projected-shadow systems (classes not rebuilt yet): an object's
// mirror and an instance's shadow taken out of their lists
#include "mirror.h"
class pshSHADOW_SYSTEM {
public:
    void TermInst(animINST *a);                                  // v12 0x00517580 -- not rebuilt
    void UpdateClear(void *vidClear);                            // v12 0x00517AF0 -- not rebuilt
    int  AddExcludeList(animINST *a, int id);                    // v12 0x00517630 -- not rebuilt
    void MergeShadowSources(animINST *dst, animINST *src);       // v12 0x00517780 -- not rebuilt
}; // v12 0x00543B10 (feb 0x0052E060)

// Static copy of the last GS_MSG_LOAD_LEVEL payload (ProcessMsg copies 0x55 dwords here)
extern gsLOAD_SCENE_PARAM gsLvlLoadParam;  // v12 0x009A58E8 (feb 0x0095CD58) verified push 0x9A58E8 at v12 0x0053765D

// =====================================================================================================
// Object-name tags parsed by gsLVL_SYSTEM::SetExtraObjFlags (v12 0x00535900, strstr on objOBJ name +0x18).
// Same strings in v12 (e.g. "&mmlod" pushed at v12 0x00535B77, "&plrc" at 0x00535984). Level designers' API.
// =====================================================================================================
//  "&dom"    domain volume: SetStateProcYes(0x40008) + SetStateYes(0x38) -> invisible, no collision
//  "&h"      hidden (SetStateProcYes 0x11008; with "&nc" a different mask)    "&nc"  no collision (obj flags |= 0x40000)
//  "&nch"    no-collision exception for "&nc" matching                           "&plrc"/"&aic" player-only / AI-only clip (flags |= 0x1008)
//  "&tra %d" transparency type 0x40 alpha %d      "&trb %d" type 0x80          "&trda" SetStateRendYes(0x1000)  "&trf" "&trl" "&trs" other transp. modes // ?
//  "&cc %x"  constant colour ARGB                 "&lt" "&ln" light flags ("&ln": obj |= 8)   "&argb" vertex colour  // ?
//  "&vis"    SetStateProcYes(0x20000) (always visible / vis-portal)   "&nf" obj |= 0x10 (no fog?)  "&ns" obj |= 0x20 (no shadow?) // ? meaning
//  "&mmlod"  mip/LOD flag        "&bb" "&bbv" billboard (objMOD_BILLBOARD, "&bbv" vertical axis)   "&ds" "&fl" // ? (double-sided, flare)
