// wr_core.h -- Will Rock (Saber Interactive, 2003) UNOFFICIAL modding SDK: authoritative shared core types.
//
// Static analysis only. Target build 1.2 (v12.exe); reference Feb 2003 build (feb.exe + WillRock.map).
// Every subsystem header in re/sdk/<subsystem>/ includes this file (directly or through
// re/sdk/core/core_types.h) instead of declaring the core types itself.
//
// Where the core types live (one definition each, no duplicates):
//   m2dV m3dV m3dVINT m3dCOLOR m3dMATR m3dQUAT m3dPOLY m3dVOL m3dBOX m3dSPHERE m3dOBB m3dSPL ...  -> math_app/m3d.h
//   apNAME arrVector<T> lstNODE/lstLIST msgADDR msgMSG msgDATA msgDATA_PACK msgSYSTEM ap* runtime   -> engine_core/core_types.h
//   objOBJ objGEOM                                                                                 -> engine_core/object.h
//   animINST animTPL animSEQ                                                                       -> engine_core/anim.h
//   cdtINFO cdtREFINE                                                                              -> engine_core/collision.h
//   entIACTION_DATA entIACTIVE_OBJ entMP_DEADRECKON entENTITY entCLASS_INFO                        -> engine_core/entity.h
//   physINIT physMATH physPHYS                                                                     -> engine_core/physics.h
//   fioCHUNK fioFILE fioFILE_DISK fioFILE_MEM                                                      -> engine_core/fileio.h
//
// Reconciliation notes (conflicts between subsystem headers, decided by the v12 bytes):
//   * entENTITY size: v12 0x16B (feb 0x14F). entIACTIVE_OBJ's two net sequence ints became u16 in 1.2
//     (name +0x40, flags +0x84, MP id +0x88 in v12; feb +0x44/+0x88/+0x8C), then 8 bytes were appended.
//     Consequence used by every gameplay subsystem: "entENTITY subclass field v12 = feb + 0x1C".
//     game_systems saw domain fields shift by +4 only: domains derive from entIACTIVE_OBJ, not entENTITY.
//   * Vtables: entIACTIVE_OBJ gained slot 8 (ProcessMP_UPD_STATE); entENTITY dropped 3 MP_UPD_POS slots and
//     gained 1: subclass virtuals start at slot 25 in v12 (26 in feb). The per-class slot rules
//     (player.h, weapons.h, ai_npc.h) are consistent with this.
//   * m3dBOX is a class (vtable, m3dVOL base) in m3d.h; anim.h's "struct m3dBOX;" forward declaration only
//     produces a harmless MSVC/GCC struct/class tag warning.
//   * gsAppState: declared int in game_systems and uint32_t in platform_drv. The authoritative declaration
//     is the one in the globals table below (u32). Subsystem duplicates are guarded by WR_CORE_GLOBALS.
//   * partEMITTER_PHYS (+4 bytes at +0x34 in 1.2) is defined once, in engine_render/particle.h;
//     sfx_sob/particles.h documents the same class from the effects side and is guarded.
//
// Builds: feb = 2003-02-21 beta (WillRock.map), jun = 1.0 (2003-06-16), v12 = 1.2 (2003-07-30, the target).

#pragma once

// Win32 headers first, then drop the macros that rename the game's own methods
// (wrUI_SYSTEM::MessageBox, objOBJ::GetObject, ...): the game names win.
#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#undef MessageBox
#undef GetObject
#undef CreateFont
#undef DrawText
#undef LoadImage
#undef SendMessage
#undef PostMessage
#undef GetCurrentTime
#undef CreateWindow
#undef GetMessage
#undef FindResource
#undef LoadString
#undef PlaySound
#undef RegisterClass
#undef min
#undef max
#endif

#ifndef WR_CORE_H
#define WR_CORE_H
#define WR_SDK_CORE 1    // subsystem headers test this to drop their placeholder core types

#include "wr_cc.h"               // __fastcall/__cdecl/__stdcall/__thiscall shims for GCC/Clang

#include <stddef.h>
#if !defined(WR_NO_CRT)   // define WR_NO_CRT for freestanding layout checks (g++ -m32 -ffreestanding)
#include <string.h>   // the game links the CRT statically; reconstructed code uses str*/mem*/sprintf
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <math.h>
#include <stdarg.h>
#if !defined(_WIN32)
// MSVC 6 swprintf has no size argument; the reconstructed sources call it that way.
inline int swprintf(wchar_t *dst, const wchar_t *fmt, ...) { va_list a; va_start(a, fmt); int r = vswprintf(dst, 0x1000, fmt, a); va_end(a); return r; }
#endif
#include <new>          // placement new (re/mpsrc constructs members in place)
#if !defined(_MSC_VER)
#  include <strings.h>
#  ifndef _strcmpi
#    define _strcmpi strcasecmp   // MSVC CRT name used by the game code
#  endif
#  ifndef _stricmp
#    define _stricmp strcasecmp
#  endif
#endif
#endif
#if defined(_MSC_VER) && _MSC_VER < 1600
   typedef signed char int8_t;   typedef unsigned char  uint8_t;
   typedef short       int16_t;  typedef unsigned short uint16_t;
   typedef int         int32_t;  typedef unsigned int   uint32_t;
#else
#  include <stdint.h>
#endif

// Pointer-size check: the game is 32-bit. A mod DLL must be built 32-bit (MSVC x86, mingw -m32, clang -m32).
#if defined(WR_REQUIRE_32BIT)
static_assert(sizeof(void *) == 4, "Will Rock mods must be built for 32-bit x86");
#endif

// ---- the core types ------------------------------------------------------------------------------------
#include "../math_app/m3d.h"          // m3dV / m3dMATR / m3dQUAT / m3dBOX ... (defines BOOL32 too)
#include "../engine_core/core_types.h"// apNAME, arrVector<T>, lstLIST, msgADDR, msgMSG, msgDATA, msgSYSTEM, ap*
#include "../engine_core/fileio.h"    // fioFILE, fioFILE_DISK, fioFILE_MEM, fioCHUNK
#include "../engine_core/object.h"    // objOBJ
#include "../engine_core/anim.h"      // animINST, animTPL, animSEQ
#include "../engine_core/collision.h" // cdtINFO, cdtREFINE
#include "../engine_core/entity.h"    // entIACTIVE_OBJ, entENTITY, entCLASS_INFO
#include "../engine_core/physics.h"   // physPHYS, physMATH

// ---- forward declarations for classes owned by subsystems (so any header can hold pointers to them) ----
class  rendDRIVER; class d3dREND_DRIVER; class vidDRIVER; class d3dDRIVER;                  // engine_render
class  txmMANAGER; class txmTEXTURE; class pteMANAGER; class pteSHADER; class scnSCENE;    // engine_render
class  partEMITTER_PHYS; class camCAMERA; class fntMANAGER;                                 // engine_render
class  gsSND_SYSTEM; class gsINP_SYSTEM; class gsVIS_SYSTEM; class gsRENDER_SYSTEM;        // game_systems
class  gsLVL_SYSTEM; class cinSYSTEM; class mrrMIRROR_SYSTEM; class gsIFACE_SYSTEM;         // game_systems
class  sncCHANNEL; class sncCHANNEL_CONE;                                                    // game_systems (sound)
class  gsMP_SYSTEM; class gsMP_SERVER; class gsMP_CLIENT;                                   // mpsrc
class  uiUI_SYSTEM; class wrUI_SYSTEM; class wrGO_SYSTEM;                                                      // frontend
class  plrPLAYER_COMMON; class plrPLAYER; class plrPLAYER_MP;                               // player
class  pwpWEAPON; class wpnWEAPON;                                                          // weapons / game_shared
class  aiNPC; class aiNPC_WR;                                                               // ai
class  inpINPUT; class _sndSYSTEM; class gsCFG_SYSTEM; class pshSHADOW_SYSTEM; class wrSTAT_SYSTEM;
struct dmgDAMAGE;
class  m3dSPL_COLOR; class m3dSPL_LINEAR1D; struct m3dPLANE;                                 // math_app (spline kinds, plane)
class  cdtCOLL_SCN; class selSEL; class lgtLIGHT_GROUP; class scnDOMAIN;                    // engine (scene parts)
class  plgsACTION; class rgdRIGID; struct rgdINIT_PARAMS; class shtSHATTER;                  // game_shared / physics
class  gsSOUND_DESCR; class gsiCONTEXT; class pctPICTURE; class sncCHANNEL_3D; struct sncSOUND_DESCR_3D;  // game_systems
struct m3dTRI; struct partPART; struct partEMITTER_VIS; class wrsfxPERSEUS;
// The game's text is UTF-16. In MSVC 6 (the game's compiler) wchar_t is a typedef of unsigned short (2 bytes), which is
// why the map mangles text parameters as "unsigned short". wrWCHAR is that type. Build mods with MSVC/clang-cl or
// mingw (wchar_t is 2 bytes there); on Linux/macOS hosts add -fshort-wchar so sizes match (a port should use char16_t).
typedef wchar_t wrWCHAR;
#if defined(__GLIBC__) || defined(__APPLE__) || defined(__unix__)
#  define ushort wrWCHAR         // re/mpsrc name (libc already typedefs ushort as unsigned short, so a macro here)
#else
typedef wrWCHAR ushort;          // re/mpsrc name
#endif
typedef int BOOL;     // Win32 BOOL (same typedef as <windows.h>, so both may be included)

// =========================================================================================================
// Main globals table. Every v12 address below was re-checked by the integrator: each has >= 2 v12
// instruction references (wr.py xref v12, see re/work/sdk_core/globs_check.txt).
// =========================================================================================================
#ifndef WR_CORE_GLOBALS
#define WR_CORE_GLOBALS
// time / frame (also in engine_core/core_types.h)
//   gsElapsedTime        v12 0x0098E9F4 (feb 0x009482DC)  float, dt of this frame (scaled, clamped 0.1)
//   gsElapsedTimeLevel   v12 0x0098EA1C (feb 0x00948304)
//   gsTime               v12 0x0098EA14 (feb 0x009482FC)  timeGetTime() ms of this frame
//   gsTimeScale          v12 0x006354E8 (feb 0x005F72A0)  game speed, 1.0
//   gsFixedElapsed       v12 0x0098EA04 (feb 0x009482EC)  fixed dt override
//   gsNextElapsedOnce    v12 0x006354EC (1.2 only)         one-shot dt override
//   gsFPS                v12 0x0098EA20 (feb 0x00948308)
//   msgSystem            v12 0x008FA960 (feb 0x008B6720)  the message system (object, not pointer)
// application / system singletons (pointers)
extern mrrMIRROR_SYSTEM *gsSysMirror;  // v12 0x0098E820 (feb 0x00948220)
extern u32               gsAppState;   // v12 0x0098E824 (feb 0x00948224) GSAPP_* bits, see math_app/app.h
extern camCAMERA        *gsCameraPtr;  // v12 0x0098E828 (feb 0x00948228)
extern gsSND_SYSTEM     *gsSysSound;   // v12 0x0098E830 (feb 0x00948230) a wrSND_SYSTEM
extern gsINP_SYSTEM     *gsSysInput;   // v12 0x0098E838 (feb 0x00948238) a wrINP_SYSTEM; +0x3C = inpINPUT*
extern gsVIS_SYSTEM     *gsSysVis;     // v12 0x0098E83C (feb 0x0094823C)
extern cinSYSTEM        *gsSysCinema;  // v12 0x0098E840 (feb 0x00948240)
extern gsRENDER_SYSTEM  *gsSysRender;  // v12 0x0098E844 (feb 0x00948244)
extern gsLVL_SYSTEM     *gsSysLevel;   // v12 0x0098E84C (feb 0x0094824C)
extern gsMP_SYSTEM      *gsSysMP;      // v12 0x0098E850 (feb 0x00948250)
extern gsMP_CLIENT      *gsSysClient;  // v12 0x0098E860 (feb ?) // ? feb twin not re-checked
extern gsMP_SERVER      *gsSysServer;  // v12 0x0098E870 (feb ?) // ? feb twin not re-checked
extern wrUI_SYSTEM      *uiSystem;     // v12 0x00994568 (feb 0x0094DCE8) the live object is a wrUI_SYSTEM (frontend/wr_ui.h)
extern wrGO_SYSTEM      *wrSysGO;      // v12 0x0064FF5C (feb 0x0060EFDC) game rules; +0xA4 infinite-ammo flag
extern int               wrAppState;   // v12 0x0064FFA8 (feb 0x0060F018) 1 German, 2 no blood (1.2)
// renderer
extern rendDRIVER       *rendDrv;      // v12 0x00978728 (feb 0x009344EC)
extern txmMANAGER       *txmManager;   // v12 0x006561B8 (feb 0x00611FD8)
extern vidDRIVER        *vidDriver;    // v12 0x006561C4 (feb 0x00611FE4)
extern d3dDRIVER        *d3dDriver;    // v12 0x00656218 (feb 0x0061204C) same object as vidDriver
extern pteMANAGER       *pteManager;   // v12 0x00974E10 (feb 0x00930BF8)
// sound / math / app
extern _sndSYSTEM       *sndSystem;    // v12 0x006561F8 (feb ?) see platform_drv/sound_drv.h
extern u32               gsFrameCounter; // v12 0x008FACC8 (feb 0x008B6A88) frame counter (name from re/mpsrc, 11 v12 refs) // ?
extern int               m3dSimdType;  // v12 0x00637C24 (feb 0x005F99D0) 2 = force x87
#endif

#include "wr_bases.h"         // opaque engine bases: dynDYNAMIC, rgdRIGID, waterLAVA, flrFLARE

// Where to look up any address: re/sdk/<subsystem>/function_index.tsv (or functions_index.h / m3d_index.h).
#endif // WR_CORE_H
