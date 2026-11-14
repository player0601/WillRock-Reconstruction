// Will Rock unofficial SDK - wrsfx* one-shot effect entities
// Generated from static analysis (re/work/sdk_sfx_sob/gen.py) and hand-checked; v12 = 1.2 (re/bin/v12.exe), feb = Feb-2003 symbol build.
// Assumes from re/sdk/core: entENTITY (feb 0x14F, v12 0x16B bytes, #pragma pack(1)), dynDYNAMIC, physPHYS, rgdRIGID,
//   objOBJ, animINST, msgADDR, m3dV, m3dMATR, txmTEXTURE, sncCHANNEL.
// Game structs are byte-packed (fields at odd offsets such as +0x14F). Fields named f_XXX by v12 offset where meaning is unknown.
// "// ?" = v12 offset derived as feb + class growth but no v12 instruction seen touching it.
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#pragma pack(push,1)
#include <stdint.h>
#include "particles.h"
#include "../game_shared/world_objects.h"   // dstrDESTROY, bttnBUTTON (integrator)
// entity class name "sfx_statue_break_part", entRegisterClass sig 0x2444594E, prio 40
// Create: v12 0x00434E50 (feb 0x0042F0D0); vtable v12 0x005FC688 (feb 0x005D1828)
struct aisSTATUE_BREAK_PART_SFX : entENTITY {  // size v12 0x183 (feb 0x173), grew +0x10 (all from base)
    partEMITTER_PHYS*  f_15F; // v12 +0x15F (feb +0x14F) // ?
    partEMITTER_PHYS*  f_163; // v12 +0x163 (feb +0x153) // ?
    float              f_16B; // v12 +0x16B (feb +0x15B)
    uint32_t           f_173; // v12 +0x173 (feb +0x163)
    uint32_t           f_177; // v12 +0x177 (feb +0x167)
    uint32_t           f_17B; // v12 +0x17B (feb +0x16B) // ?
    uint32_t           f_17F; // v12 +0x17F (feb +0x16F)
    // vtable overrides (v12 slots that point into this class):
    // slot +0x04 v12 0x00434EA0 (feb 0x0042F120) aisSTATUE_BREAK_PART_SFX::ProcessMsg
    // non-virtual / static:
    // v12 0x00434E50 (feb 0x0042F0D0) aisSTATUE_BREAK_PART_SFX::Create
    // v12 0x00434EA0 (feb 0x0042F120) aisSTATUE_BREAK_PART_SFX::ProcessMsg
    // v12 0x00434ED0 (feb 0x0042F150) aisSTATUE_BREAK_PART_SFX::ProcessINIT
    // v12 0x00435000 (feb 0x0042F2E0) aisSTATUE_BREAK_PART_SFX::ProcessFRAME
};

// entity class name "sfx_egg", entRegisterClass sig 0x42444547, prio 55
// Create: v12 0x004641F0 (feb 0x0045A850); vtable v12 0x00600618 (feb 0x005D51F8)
struct birdEGG_SFX : dynDYNAMIC {  // size v12 0x1A7 (feb 0x183); src/Sources/bird_egg.cpp
    objOBJ*            emitterObj; // v12 +0x19B (feb +0x177) "emitter"
    partEMITTER_PHYS*  sparks1;    // v12 +0x19F (feb +0x17B) birdEGG_SPARKS_1 (+ its trail)
    partEMITTER_PHYS*  sparks2;    // v12 +0x1A3 (feb +0x17F) birdEGG_SPARKS_2 (+ its trail)
    BOOL32 ProcessINIT(void *init);                              // v12 0x004631E0 (feb 0x00459A50)
    static entENTITY *__fastcall Create(animINST *i);            // v12 0x004641F0 (feb 0x0045A850)
};

// entity class name "dyn_destroy", entRegisterClass sig 0x24445354, prio 40
// Create: v12 0x00553FF0 (feb 0x0053B6E0); vtable v12 0x00608E48 (feb 0x005DD898)
#if 0 // [integrator] dstrDESTROY is defined in game_shared/world_objects.h (same layout, named fields)
struct dstrDESTROY : entENTITY {  // size v12 0x183 (feb 0x167), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F) init 0 // ?
    uint32_t           f_16F; // v12 +0x16F (feb +0x153) init 5.0 // ?
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 1.0 // ?
    uint32_t           f_177; // v12 +0x177 (feb +0x15B) init 1.0 // ?
    float              f_17B; // v12 +0x17B (feb +0x15F) // ?
    float              f_17F; // v12 +0x17F (feb +0x163) init 0 // ?
    // vtable overrides (v12 slots that point into this class):
    // non-virtual / static:
};
#endif


// entity class name "sfx_bsk_bullet", entRegisterClass sig 0x5042534B, prio 45
// Create: v12 0x00408470 (feb 0x00407F20); vtable v12 0x005F9C80 (feb 0x005CEF80)
// pwpBSK_BULLET : pjlPJL<pwpBSK_PJL_SFX> -- rebuilt in src/Sources/plw_bsk.cpp (the template fields from +0x16F, its
//   flight voice +0x1B0)

// entity class name "sfx_mcg_hit", entRegisterClass sig 0x504D4347, prio 45
// Create: v12 0x0040A2B0 (feb 0x004098B0); vtable v12 0x005F9F60 (feb 0x005CF1F8)
struct pwpMCG_HIT : dynDYNAMIC {  // size v12 0x19B (feb 0x177), grew +0x24 (all from base)
    // vtable overrides (v12 slots that point into this class):
    // non-virtual / static:
};

// entity class name "sfx_mdg_pjl", entRegisterClass sig 0x504D4447, prio 45
// Create: v12 0x004104B0 (feb 0x0040EE50); vtable v12 0x005FA710 (feb 0x005CF950)
struct pwpMEDUSA_PJL : physPHYS {  // size v12 0x1D0 (feb 0x1AC), grew +0x24 (all from base)
    uint32_t           f_177; // v12 +0x177 (feb +0x153) init 0 // ?
    uint32_t           f_17B; // v12 +0x17B (feb +0x157) init 0 // ?
    uint32_t           f_17F; // v12 +0x17F (feb +0x15B) init -1.0 // ?
    uint32_t           f_183; // v12 +0x183 (feb +0x15F) init 0 // ?
    uint32_t           f_187; // v12 +0x187 (feb +0x163) init 4294967295 // ?
    uint32_t           f_18B; // v12 +0x18B (feb +0x167) init 4294967295 // ?
    uint32_t           f_18F; // v12 +0x18F (feb +0x16B) init 4294967295 // ?
    uint32_t           f_193; // v12 +0x193 (feb +0x16F) init -1.0 // ?
    uint32_t           f_198; // v12 +0x198 (feb +0x174) init 0 // ?
    uint32_t           f_19C; // v12 +0x19C (feb +0x178) init 0 // ?
    int32_t            f_1A0; // v12 +0x1A0 (feb +0x17C) init 0 // ?
    float              f_1CC; // v12 +0x1CC (feb +0x1A8) // ?
    // vtable overrides (v12 slots that point into this class):
    // non-virtual / static:
};

// entity class name "sfx_shovel", entRegisterClass sig 0x5053564C, prio 80
// Create: v12 0x004153C0 (feb 0x00413A50); vtable v12 0x005FAD30 (feb 0x005CFF18)
struct pwpSVL_HIT : dynDYNAMIC {  // size v12 0x1AB (feb 0x187), grew +0x24 (all from base)
    partEMITTER_PHYS*  f_19B; // v12 +0x19B (feb +0x177) init 0 // ?
    partEMITTER_PHYS*  f_19F; // v12 +0x19F (feb +0x17B) init 0 // ?
    partEMITTER_PHYS*  f_1A3; // v12 +0x1A3 (feb +0x17F) init 0 // ?
    objOBJ*            f_1A7; // v12 +0x1A7 (feb +0x183) init 0 // ?
    // vtable overrides (v12 slots that point into this class):
    // non-virtual / static:
};

// entity class name "sfx_water_expl", entRegisterClass sig 0x5042534B, prio 45
// Create: v12 0x0041CFC0 (feb 0x0041A560); vtable v12 0x005FB480 (feb 0x005D0630)
struct pwpWSFX_EXPL : entENTITY {  // size v12 0x16B (feb 0x14F), grew +0x1C (all from base)
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0041CFF0 (feb 0x0041A590) ??_EpwpWSFX_EXPL@@UAEPAXI@Z
    // slot +0x04 v12 0x0041C920 (feb 0x00419EF0) pwpWSFX_EXPL::ProcessMsg
    // non-virtual / static:
    // v12 0x0041C920 (feb 0x00419EF0) pwpWSFX_EXPL::ProcessMsg
    // v12 0x0041CFC0 (feb 0x0041A560) pwpWSFX_EXPL::Create
};

// entity class name "sfx_water_splash", entRegisterClass sig 0x504D4347, prio 45
// Create: v12 0x0041CF60 (feb 0x0041A500); vtable v12 0x005FB418 (feb 0x005D05C8)
struct pwpWSFX_SPLASH : entENTITY {  // size v12 0x16B (feb 0x14F), grew +0x1C (all from base)
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0041CF90 (feb 0x0041A530) ??_EpwpWSFX_SPLASH@@UAEPAXI@Z
    // slot +0x04 v12 0x0041C370 (feb 0x00419910) pwpWSFX_SPLASH::ProcessMsg
    // non-virtual / static:
    // v12 0x0041C370 (feb 0x00419910) pwpWSFX_SPLASH::ProcessMsg
    // v12 0x0041CF60 (feb 0x0041A500) pwpWSFX_SPLASH::Create
};

// entity class name "rigid_explos", entRegisterClass sig 0x52474442, prio 50
// Create: v12 0x00527780 (feb 0x005122D0); vtable v12 0x006080E8 (feb 0x005DCC28)
struct rgdRIGID_EXPLOS : entENTITY {  // size v12 0x16B (feb 0x14F), grew +0x1C (all from base)
    // vtable overrides (v12 slots that point into this class):
    // non-virtual / static:
};

// entity class name "sfx_altar", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00465A20 (feb 0x0045BFC0); vtable v12 0x00600820 (feb 0x005D53E0)
struct wrsfxALTAR : dynDYNAMIC {  // size v12 0x1AF (feb 0x18B), grew +0x24 (all from base)
    uint32_t           f_19B; // v12 +0x19B (feb +0x177) init 0
    partEMITTER_PHYS*  f_19F; // v12 +0x19F (feb +0x17B) init 0
    sncCHANNEL*        f_1A3; // v12 +0x1A3 (feb +0x17F) init 0
    uint32_t           f_1AB; // v12 +0x1AB (feb +0x187) init 0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00465AD0 (feb 0x0045C050) ??_GwrsfxALTAR@@MAEPAXI@Z
    // slot +0x04 v12 0x00466430 (feb 0x0045C9B0) wrsfxALTAR::ProcessMsg
    // slot +0x38 v12 0x00466290 (feb 0x0045C810) wrsfxALTAR::ProcessMP_WRITE_SYNC_DATA
    // slot +0x3C v12 0x00466310 (feb 0x0045C890) wrsfxALTAR::ProcessMP_READ_SYNC_DATA
    // slot +0x64 v12 0x00465EC0 (feb 0x0045C440) wrsfxALTAR::ProcessINIT
    // slot +0x6C v12 0x004661C0 (feb 0x0045C740) wrsfxALTAR::ProcessTERM
    // slot +0x100 v12 0x004669B0 (feb 0x0045CEC0) ??_EwrsfxALTAR_EXPLOSION_EMITTER@@UAEPAXI@Z
    // slot +0x120 v12 0x00466A00 (feb 0x0045CF10) wrsfxALTAR_EXPLOSION_EMITTER::PartUpdate
    // slot +0x160 v12 0x004677A0 (feb 0x0045DAD0) ??_EwrsfxALTAR_FIREBALL@@UAEPAXI@Z
    // slot +0x1C4 v12 0x00466C60 (feb 0x0045D170) wrsfxALTAR_FIREBALL::ProcessINIT
    // slot +0x1C8 v12 0x00467060 (feb 0x0045D570) wrsfxALTAR_FIREBALL::ProcessTERM
    // slot +0x1CC v12 0x00466EB0 (feb 0x0045D3C0) wrsfxALTAR_FIREBALL::ProcessFRAME
    // slot +0x1D0 v12 0x004670D0 (feb 0x0045D5E0) wrsfxALTAR_FIREBALL::ProcessDESTROY_ENT
    // slot +0x1D4 v12 0x00467100 (feb 0x0045D610) wrsfxALTAR_FIREBALL::ProcessCDT
    // slot +0x1D8 v12 0x00467530 (feb 0x0045D900) wrsfxALTAR_FIREBALL::ProcessMP_NOTIFY
    // non-virtual / static:
    // v12 0x00465A20 (feb 0x0045BFC0) wrsfxALTAR::Create
    // v12 0x00465E00 (feb 0x0045C380) wrsfxALTAR::ToggleEffect
    // v12 0x00465EC0 (feb 0x0045C440) wrsfxALTAR::ProcessINIT
    // v12 0x004661C0 (feb 0x0045C740) wrsfxALTAR::ProcessTERM
    // v12 0x00466290 (feb 0x0045C810) wrsfxALTAR::ProcessMP_WRITE_SYNC_DATA
    // v12 0x00466310 (feb 0x0045C890) wrsfxALTAR::ProcessMP_READ_SYNC_DATA
    // v12 0x00466430 (feb 0x0045C9B0) wrsfxALTAR::ProcessMsg
};

// entity class name "sfx_altar_fireball", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00467730 (feb 0x0045DA60); vtable v12 0x00600980 (feb 0x005D5548)
struct wrsfxALTAR_FIREBALL : physPHYS {  // size v12 0x197 (feb 0x17B), grew +0x1C (all from base)
    uint32_t           f_16F; // v12 +0x16F (feb +0x153) init 0
    int32_t            f_173; // v12 +0x173 (feb +0x157)
    uint32_t           f_177; // v12 +0x177 (feb +0x15B)
    objOBJ*            f_17B; // v12 +0x17B (feb +0x15F)
    float              f_17F; // v12 +0x17F (feb +0x163)
    uint32_t           f_183; // v12 +0x183 (feb +0x167) init 0
    uint32_t           f_187; // v12 +0x187 (feb +0x16B) init 1.0
    partEMITTER_PHYS*  f_18B; // v12 +0x18B (feb +0x16F) init 0
    objOBJ*            f_18F; // v12 +0x18F (feb +0x173)
    sncCHANNEL*        f_193; // v12 +0x193 (feb +0x177)
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x004677A0 (feb 0x0045DAD0) ??_EwrsfxALTAR_FIREBALL@@UAEPAXI@Z
    // slot +0x64 v12 0x00466C60 (feb 0x0045D170) wrsfxALTAR_FIREBALL::ProcessINIT
    // slot +0x68 v12 0x00467060 (feb 0x0045D570) wrsfxALTAR_FIREBALL::ProcessTERM
    // slot +0x6C v12 0x00466EB0 (feb 0x0045D3C0) wrsfxALTAR_FIREBALL::ProcessFRAME
    // slot +0x70 v12 0x004670D0 (feb 0x0045D5E0) wrsfxALTAR_FIREBALL::ProcessDESTROY_ENT
    // slot +0x74 v12 0x00467100 (feb 0x0045D610) wrsfxALTAR_FIREBALL::ProcessCDT
    // slot +0x78 v12 0x00467530 (feb 0x0045D900) wrsfxALTAR_FIREBALL::ProcessMP_NOTIFY
    // non-virtual / static:
    // v12 0x00466C60 (feb 0x0045D170) wrsfxALTAR_FIREBALL::ProcessINIT
    // v12 0x00466EB0 (feb 0x0045D3C0) wrsfxALTAR_FIREBALL::ProcessFRAME
    // v12 0x00467060 (feb 0x0045D570) wrsfxALTAR_FIREBALL::ProcessTERM
    // v12 0x004670D0 (feb 0x0045D5E0) wrsfxALTAR_FIREBALL::ProcessDESTROY_ENT
    // v12 0x00467100 (feb 0x0045D610) wrsfxALTAR_FIREBALL::ProcessCDT
    // v12 0x00467180 (feb 0x0045D6D0) wrsfxALTAR_FIREBALL::StartFireBall
    // v12 0x00467530 (feb 0x0045D900) wrsfxALTAR_FIREBALL::ProcessMP_NOTIFY
    // v12 0x00467580 (feb 0x0045D950) wrsfxALTAR_FIREBALL::ProcessAltarOppUnhide
    // v12 0x00467730 (feb 0x0045DA60) wrsfxALTAR_FIREBALL::Create
};

// entity class name "sfx_appear", entRegisterClass sig 0x24534658, prio 45
// Create: v12 0x00468970 (feb 0x0045EBA0); vtable v12 0x00600B10 (feb 0x005D56D8)
class wrsfxAPPEAR_CLOUD : public partEMITTER_PHYS { public: wrsfxAPPEAR_CLOUD(float size); };           // v12 0x00468160 (feb 0x0045E340)
class wrsfxAPPEAR_LARGE_PART : public partEMITTER_PHYS { public: wrsfxAPPEAR_LARGE_PART(float size); }; // v12 0x00468310 (feb 0x0045E4F0)
class wrsfxAPPEAR_SMALL_PART : public partEMITTER_PHYS { public: wrsfxAPPEAR_SMALL_PART(float size); }; // v12 0x004684E0 (feb 0x0045E6C0)
int __fastcall wrsfxInit();              // v12 0x00467830 (feb 0x0045DB60)
struct wrsfxAPPEAR : entENTITY {  // size v12 0x16B (feb 0x14F), grew +0x1C (all from base)
    static txmTEXTURE *pTexCloud, *pTexPart, *pTexHole;
    BOOL32 ProcessMsg(int id, void *d, msgADDR *from, int *ret);
    static entENTITY *__fastcall Create(animINST *i);
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x004689A0 (feb 0x0045EBD0) ??_EwrsfxAPPEAR@@UAEPAXI@Z
    // slot +0x04 v12 0x00468790 (feb 0x0045E970) wrsfxAPPEAR::ProcessMsg
    // non-virtual / static:
    // v12 0x004686D0 (feb 0x0045E8B0) wrsfxAPPEAR::Start
    // v12 0x00468790 (feb 0x0045E970) wrsfxAPPEAR::ProcessMsg
    // v12 0x00468970 (feb 0x0045EBA0) wrsfxAPPEAR::Create
};

// entity class name "sfx_atlas_globe_appear", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00469690 (feb 0x0045F8C0); vtable v12 0x00600C10 (feb 0x005D57E0)
struct wrsfxATLAS_GLOBE_APPEAR : entENTITY {  // size v12 0x183 (feb 0x167), grew +0x1C (all from base)
    partEMITTER_PHYS*  f_16B; // v12 +0x16B (feb +0x14F)
    partEMITTER_PHYS*  f_16F; // v12 +0x16F (feb +0x153)
    objOBJ*            f_173; // v12 +0x173 (feb +0x157)
    objOBJ*            f_177; // v12 +0x177 (feb +0x15B) // ?
    uint32_t           f_17B; // v12 +0x17B (feb +0x15F) init 2.0
    uint32_t           f_17F; // v12 +0x17F (feb +0x163) init 1
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x004696E0 (feb 0x0045F910) ??_EwrsfxATLAS_GLOBE_APPEAR@@UAEPAXI@Z
    // slot +0x04 v12 0x00468F60 (feb 0x0045F190) wrsfxATLAS_GLOBE_APPEAR::ProcessMsg
    // non-virtual / static:
    // v12 0x00468EE0 (feb 0x0045F110) wrsfxATLAS_GLOBE_APPEAR::Start
    // v12 0x00468F60 (feb 0x0045F190) wrsfxATLAS_GLOBE_APPEAR::ProcessMsg
    // v12 0x00469690 (feb 0x0045F8C0) wrsfxATLAS_GLOBE_APPEAR::Create
};

// entity class name "sfx_atlas_globe_land", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00469710 (feb 0x0045F940); vtable v12 0x00600C78 (feb 0x005D5848)
struct wrsfxATLAS_GLOBE_LAND : entENTITY {  // size v12 0x17B (feb 0x15F), grew +0x1C (all from base)
    partEMITTER_PHYS*  f_16B; // v12 +0x16B (feb +0x14F)
    uint32_t           f_16F; // v12 +0x16F (feb +0x153) init 1.1330000162124634
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 0
    uint32_t           f_177; // v12 +0x177 (feb +0x15B) init 1
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00469760 (feb 0x0045F990) ??_EwrsfxATLAS_GLOBE_LAND@@UAEPAXI@Z
    // slot +0x04 v12 0x00469510 (feb 0x0045F740) wrsfxATLAS_GLOBE_LAND::ProcessMsg
    // non-virtual / static:
    // v12 0x00469480 (feb 0x0045F6B0) wrsfxATLAS_GLOBE_LAND::Start
    // v12 0x00469510 (feb 0x0045F740) wrsfxATLAS_GLOBE_LAND::ProcessMsg
    // v12 0x00469710 (feb 0x0045F940) wrsfxATLAS_GLOBE_LAND::Create
};

// entity class name "sfx_barrel_expl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00469820 (feb 0x0045FA50); vtable v12 0x00600CE0 (feb 0x005D58B8)
struct wrsfxBARREL_EXPLODE : entENTITY {  // size v12 0x17F (feb 0x163); src/Sources/sfx_barrel.cpp
    partEMITTER_PHYS*  fireSrc;  // v12 +0x16B (feb +0x14F) wrsfxBARREL_EXPLODE_FIRE_SRC (carries the fire)
    partEMITTER_PHYS*  fire;     // v12 +0x16F (feb +0x153) wrsfxBARREL_EXPLODE_FIRE
    partEMITTER_PHYS*  glow;     // v12 +0x173 (feb +0x157) wrsfxBARREL_EXPLODE_GLOW
    partEMITTER_PHYS*  parts;    // v12 +0x177 (feb +0x15B) wrsfxBARREL_EXPLODE_PARTS
    float              timer;    // v12 +0x17B (feb +0x15F) 1.2 s, then gone
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x0046A000 (feb 0x00460230)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x00469820 (feb 0x0045FA50)
};

// entity class name "sfx_bird_death", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0046A870 (feb 0x00460AA0); vtable v12 0x00600E48 (feb 0x005D5A20)
struct wrsfxBIRD_DEATH : entENTITY {  // size v12 0x17B (feb 0x15F), grew +0x1C (all from base)
    partEMITTER_PHYS*  f_16B; // v12 +0x16B (feb +0x14F)
    uint32_t           f_16F; // v12 +0x16F (feb +0x153) init 6.0
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 0.5
    uint32_t           f_177; // v12 +0x177 (feb +0x15B) init 1
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0046A8C0 (feb 0x00460AF0) ??_EwrsfxBIRD_DEATH@@UAEPAXI@Z
    // slot +0x04 v12 0x0046A730 (feb 0x00460960) wrsfxBIRD_DEATH::ProcessMsg
    // non-virtual / static:
    // v12 0x0046A6C0 (feb 0x004608F0) wrsfxBIRD_DEATH::Start
    // v12 0x0046A730 (feb 0x00460960) wrsfxBIRD_DEATH::ProcessMsg
    // v12 0x0046A870 (feb 0x00460AA0) wrsfxBIRD_DEATH::Create
};

// entity class name "sfx_bridge", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0046D4C0 (feb 0x00463660); vtable v12 0x006010E8 (feb 0x005D5CC8)
struct wrsfxBRIDGE : dynDYNAMIC {  // size v12 0x1A3 (feb 0x17F), grew +0x24 (all from base)
    entENTITY*         f_19B; // v12 +0x19B (feb +0x177)
    uint32_t           f_19F; // v12 +0x19F (feb +0x17B)
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0046D560 (feb 0x004636F0) ??_EwrsfxBRIDGE@@UAEPAXI@Z
    // slot +0x04 v12 0x0046D2A0 (feb 0x00463440) wrsfxBRIDGE::ProcessMsg
    // non-virtual / static:
    // v12 0x0046D2A0 (feb 0x00463440) wrsfxBRIDGE::ProcessMsg
    // v12 0x0046D4C0 (feb 0x00463660) wrsfxBRIDGE::Create
};

// entity class name "sfx_bridge_crash", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0046D430 (feb 0x004635D0); vtable v12 0x00601080 (feb 0x005D5C60)
struct wrsfxBRIDGE_CRASH : entENTITY {  // size v12 0x1AF (feb 0x193), grew +0x1C (all from base)
    int32_t            f_16B; // v12 +0x16B (feb +0x14F)
    objOBJ*            f_16F; // v12 +0x16F (feb +0x153)
    int32_t            f_173; // v12 +0x173 (feb +0x157) // ?
    objOBJ*            f_177; // v12 +0x177 (feb +0x15B) // ?
    int32_t            f_17B; // v12 +0x17B (feb +0x15F) // ?
    objOBJ*            f_17F; // v12 +0x17F (feb +0x163) // ?
    int32_t            f_183; // v12 +0x183 (feb +0x167) // ?
    objOBJ*            f_187; // v12 +0x187 (feb +0x16B) // ?
    int32_t            f_18B; // v12 +0x18B (feb +0x16F) // ?
    objOBJ*            f_18F; // v12 +0x18F (feb +0x173) // ?
    int32_t            f_193; // v12 +0x193 (feb +0x177) // ?
    objOBJ*            f_197; // v12 +0x197 (feb +0x17B) // ?
    int32_t            f_19B; // v12 +0x19B (feb +0x17F) // ?
    objOBJ*            f_19F; // v12 +0x19F (feb +0x183) // ?
    uint32_t           f_1A3; // v12 +0x1A3 (feb +0x187) init 1
    uint32_t           f_1A7; // v12 +0x1A7 (feb +0x18B) init 0
    int32_t            f_1AB; // v12 +0x1AB (feb +0x18F) init 3
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0046D490 (feb 0x00463630) ??_EwrsfxBRIDGE_CRASH@@UAEPAXI@Z
    // slot +0x04 v12 0x0046D1F0 (feb 0x00463390) wrsfxBRIDGE_CRASH::ProcessMsg
    // non-virtual / static:
    // v12 0x0046CAA0 (feb 0x00462C40) wrsfxBRIDGE_CRASH::ProcessINIT
    // v12 0x0046D160 (feb 0x00463300) wrsfxBRIDGE_CRASH::Start
    // v12 0x0046D1F0 (feb 0x00463390) wrsfxBRIDGE_CRASH::ProcessMsg
    // v12 0x0046D430 (feb 0x004635D0) wrsfxBRIDGE_CRASH::Create
};

// entity class name "sfx_cerber_pjl_acid_expl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0046E4B0 (feb 0x004646A0); vtable v12 0x006012C0 (feb 0x005D5EA0)
struct wrsfxCERBER_ACID_EXPL : entENTITY {  // size v12 0x177 (feb 0x15B); src/Sources/sfx_cerber.cpp
    partEMITTER_PHYS*  smoke; // v12 +0x16B (feb +0x14F) wrsfxCERBER_ACID_EXPL_SMOKE (left to itself)
    partEMITTER_PHYS*  glow;  // v12 +0x16F (feb +0x153) wrsfxCERBER_ACID_EXPL_GLOW (left to itself)
    float              timer; // v12 +0x173 (feb +0x157) 1.0 (set as it goes; not read)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x0046E320 (feb 0x00464500)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x0046E4B0 (feb 0x004646A0)
};

// entity class name "sfx_cerber_pjl_expl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0046E430 (feb 0x00464620); vtable v12 0x00601258 (feb 0x005D5E38)
struct wrsfxCERBER_FIREBALL_EXPL : entENTITY {  // size v12 0x17B (feb 0x15F); src/Sources/sfx_cerber.cpp
    partEMITTER_PHYS*  sparks; // v12 +0x16B (feb +0x14F) wrsfxCERBER_FIREBALL_EXPL_SPARKS (left to itself)
    partEMITTER_PHYS*  burn;   // v12 +0x16F (feb +0x153) wrsfxCERBER_FIREBALL_EXPL_BURN, over "emitter"
    partEMITTER_PHYS*  glow;   // v12 +0x173 (feb +0x157) wrsfxCERBER_FIREBALL_EXPL_GLOW
    float              timer;  // v12 +0x177 (feb +0x15B) 1.5 (set as it goes; not read)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x0046DD40 (feb 0x00463EB0)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x0046E430 (feb 0x00464620)
};

// entity class name "sfx_chunk", entRegisterClass sig 0x43484E4B, prio 50
// Create: v12 0x0046C840 (feb 0x004629E0); vtable v12 0x00600FD0 (feb 0x005D5BA8)
struct wrsfxCHUNK : physPHYS {  // size v12 0x18F (feb 0x173); src/Sources/sfx_blood.cpp
    int32_t            isRegen;    // v12 +0x16F (feb +0x153) a minotaur's piece that grows a new one (not meat)
    uint32_t           isSmall;    // v12 +0x173 (feb +0x157) a shorter, smaller blood pool where it lands
    float              timer;      // v12 +0x177 (feb +0x15B) runs down once landed: the regeneration / the animation
    float              fadeTimer;  // v12 +0x17B (feb +0x15F) landed: it goes when this runs out
    int32_t            regenerated;// v12 +0x17F (feb +0x163)
    int32_t            landed;     // v12 +0x183 (feb +0x167)
    int32_t            domNmb;     // v12 +0x187 (feb +0x16B) the spawn domain of the NPC it came from (-1 none)
    uint32_t           domData;    // v12 +0x18B (feb +0x16F)
    BOOL32 ProcessINIT(physINIT *p);                              // +0x64 v12 0x0046BC30 (feb 0x00461DC0)
    void   ProcessTERM();                                         // +0x68 v12 0x0046BF80 (feb 0x00462100) empty
    void   ProcessFRAME();                                        // +0x6C v12 0x0046BD00 (feb 0x00461E70)
    BOOL32 ProcessCDT(cdtINFO *ci);                               // +0x74 v12 0x0046BED0 (feb 0x00462050)
    void   ProcessMP_NOTIFY(msgDATA *ev);                         // +0x78 v12 0x0046BF90 (feb 0x00462110)
    void   EV_CDTAction(struct wrsfxCHUNK_EV_CDT *ev);            // v12 0x0046C020 (feb 0x004621A0)
    static entENTITY *__fastcall Create(animINST *i);             // v12 0x0046C840 (feb 0x004629E0)
};

// entity class name "sfx_cyclop_death", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004707A0 (feb 0x00466A60); vtable v12 0x00601660 (feb 0x005D6240)
struct wrsfxCYCLOP_DEATH : entENTITY {  // size v12 0x177 (feb 0x15B), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F)
    uint32_t           f_16F; // v12 +0x16F (feb +0x153) init 0
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 17.100000381469727
    // vtable overrides (v12 slots that point into this class):
    // slot +0x04 v12 0x00470480 (feb 0x00466710) wrsfxCYCLOP_DEATH::ProcessMsg
    // non-virtual / static:
    // v12 0x00470480 (feb 0x00466710) wrsfxCYCLOP_DEATH::ProcessMsg
    // v12 0x004707A0 (feb 0x00466A60) wrsfxCYCLOP_DEATH::Create
};

// entity class name "sfx_cyclop_hit", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00470720 (feb 0x004669E0); vtable v12 0x006015F8 (feb 0x005D61D8)
struct wrsfxCYCLOP_HIT : entENTITY {  // size v12 0x177 (feb 0x15B), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F)
    uint32_t           f_16F; // v12 +0x16F (feb +0x153)
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 7.099999904632568
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00470770 (feb 0x00466A30) ??_EwrsfxCYCLOP_HIT@@MAEPAXI@Z
    // slot +0x04 v12 0x0046F9E0 (feb 0x00465C10) wrsfxCYCLOP_HIT::ProcessMsg
    // non-virtual / static:
    // v12 0x0046F9E0 (feb 0x00465C10) wrsfxCYCLOP_HIT::ProcessMsg
    // v12 0x00470720 (feb 0x004669E0) wrsfxCYCLOP_HIT::Create
};

// entity class name "sfx_cyclop_walk", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004706A0 (feb 0x00466960); vtable v12 0x00601590 (feb 0x005D6170)
struct wrsfxCYCLOP_WALK : entENTITY {  // size v12 0x177 (feb 0x15B), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F) init 0
    uint32_t           f_16F; // v12 +0x16F (feb +0x153) init 0
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 5.0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x004706F0 (feb 0x004669B0) ??_EwrsfxCYCLOP_WALK@@UAEPAXI@Z
    // slot +0x04 v12 0x0046EF00 (feb 0x004650F0) wrsfxCYCLOP_WALK::ProcessMsg
    // non-virtual / static:
    // v12 0x0046EF00 (feb 0x004650F0) wrsfxCYCLOP_WALK::ProcessMsg
    // v12 0x004706A0 (feb 0x00466960) wrsfxCYCLOP_WALK::Create
};

// entity class name "barrel", entRegisterClass sig 0x24445354, prio 40
// Create: v12 0x00471720 (feb 0x00467A50); vtable v12 0x00601928 (feb 0x005D6508)
struct wrsfxDESTROY_BARREL : dstrDESTROY {  // size v12 0x183, vtable v12 0x00601928 (feb 0x005D6508)
    void ProcessCDT_FIREABLE(dmgDAMAGE *d);                       // +0x70 v12 0x00470E70 (feb 0x004671B0)
    static entENTITY *__fastcall Create(animINST *inst);          // v12 0x00471720 (feb 0x00467A50)
};
struct wrsfxDESTROY_BARREL_DMG_DISPATCHER : dmgDISPATCHER {       // vtable v12 0x006016C8
    void Dispatch(entENTITY *target, dmgDAMAGE *d, entENTITY *attacker);   // v12 0x00470D40 (feb 0x00467080)
};

// entity class name "mirror", entRegisterClass sig 0x24445354, prio 40
// Create: v12 0x004716E0 (feb 0x00467A10); vtable v12 0x006018A0 (feb 0x005D6480)
struct wrsfxDESTROY_MIRROR : dstrDESTROY {  // size v12 0x183, vtable v12 0x006018A0 (feb 0x005D6480)
    BOOL32 ProcessINIT();                                         // +0x64 v12 0x00470CB0 (feb 0x00466FF0)
    static entENTITY *__fastcall Create(animINST *inst);          // v12 0x004716E0 (feb 0x00467A10)
};

// entity class name "tripod", entRegisterClass sig 0x24445354, prio 40
// Create: v12 0x00471690 (feb 0x004679C0); vtable v12 0x00601818 (feb 0x005D63F8)
struct wrsfxDESTROY_TRIPOD : dstrDESTROY {  // size v12 0x18B (feb 0x16F) = wrsfxDESTROY_FLAME; src/Sources/sfx_destroy.cpp
    entENTITY *flame;          // v12 +0x183 (feb +0x167) the "sfx_flame" on its "flame_source"
    objOBJ    *flameEmitter;   // v12 +0x187 (feb +0x16B) "flame_emitter" (hidden): the burning bits fall from it
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // +0x04 v12 0x00470C70 (feb 0x00466FB0) FLAME's
    BOOL32 ProcessINIT();                                         // +0x64 v12 0x004709E0 (feb 0x00466CE0)
    void   ProcessTERM();                                         // +0x6C v12 0x00470B30 (feb 0x00466E30)
    void   ProcessINIT_LEVEL();                                   // +0x78 v12 0x00470AD0 (feb 0x00466DD0)
    void   StartExplos(int silent);                               // +0x80 v12 0x00470B60 (feb 0x00466E60)
    static entENTITY *__fastcall Create(animINST *inst);          // v12 0x00471690 (feb 0x004679C0)
};

// entity class name "vase_destroy", entRegisterClass sig 0x24445354, prio 40
// Create: v12 0x00471650 (feb 0x00467980); vtable v12 0x00601790 (feb 0x005D6370)
struct wrsfxDESTROY_VASE : dstrDESTROY {  // size v12 0x183, vtable v12 0x00601790 (feb 0x005D6370)
    void ProcessCDT_FIREABLE(dmgDAMAGE *d);                       // +0x70 v12 0x004708E0 (feb 0x00466BA0)
    static entENTITY *__fastcall Create(animINST *inst);          // v12 0x00471650 (feb 0x00467980)
};
class wrsfxDESTROY_VASE_PARTS : public partEMITTER_PHYS {         // vtable v12 0x0060172C, new 0x2C4
public:
    wrsfxDESTROY_VASE_PARTS(m3dPLANE *floor);                     // v12 0x00471280 (feb 0x004675B0)
};
class wrsfxDESTROY_VASE_SMOKE : public partEMITTER_PHYS {         // vtable v12 0x0060175C, new 0x2C4
public:
    wrsfxDESTROY_VASE_SMOKE();                                    // v12 0x00471480 (feb 0x004677B0)
};
extern txmTEXTURE *wrsfxDestrTexPart;                             // v12 0x0064ED4C "sfx_mcg_part"
extern txmTEXTURE *wrsfxDestrTexCloud;                            // v12 0x0064ED50 "sfx_mcg_cloud"
extern txmTEXTURE *wrsfxDestrTexGlow;                             // v12 0x0064ED44 "sfx_egg_glow"
int __fastcall wrsfxDestrInit();                                  // v12 0x004707F0 (feb 0x00466AB0)

// entity class name "sfx_discus_land", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00472250 (feb 0x00468580); vtable v12 0x00601A58 (feb 0x005D6638)
struct wrsfxDISCUS_LAND : entENTITY {  // size v12 0x183 (feb 0x167), grew +0x1C (all from base)
    partEMITTER_PHYS*  f_16B; // v12 +0x16B (feb +0x14F)
    objOBJ*            f_16F; // v12 +0x16F (feb +0x153)
    int32_t            f_173; // v12 +0x173 (feb +0x157)
    objOBJ*            f_177; // v12 +0x177 (feb +0x15B)
    partEMITTER_PHYS*  f_17B; // v12 +0x17B (feb +0x15F)
    uint32_t           f_17F; // v12 +0x17F (feb +0x163) init 6.0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x004722B0 (feb 0x004685E0) ??_EwrsfxDISCUS_LAND@@UAEPAXI@Z
    // slot +0x04 v12 0x00472220 (feb 0x00468550) wrsfxDISCUS_LAND::ProcessMsg
    // non-virtual / static:
    // v12 0x00471F50 (feb 0x00468280) wrsfxDISCUS_LAND::ProcessINIT
    // v12 0x00472220 (feb 0x00468550) wrsfxDISCUS_LAND::ProcessMsg
    // v12 0x00472250 (feb 0x00468580) wrsfxDISCUS_LAND::Create
};

// entity class name "sfx_dust", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00472330 (feb 0x00468660); vtable v12 0x00601AC8 (feb 0x005D66A8)
struct wrsfxDUST : entENTITY {  // size v12 0x193 (feb 0x177), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F) init 0
    uint32_t           f_16F; // v12 +0x16F (feb +0x153) init 0
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 0
    uint32_t           f_177; // v12 +0x177 (feb +0x15B) init 0
    objOBJ*            f_17B; // v12 +0x17B (feb +0x15F) init 0
    uint32_t           f_17F; // v12 +0x17F (feb +0x163) init 0
    uint32_t           f_183; // v12 +0x183 (feb +0x167) init 1.0
    uint32_t           f_187; // v12 +0x187 (feb +0x16B) init 5.0
    uint32_t           f_18B; // v12 +0x18B (feb +0x16F) init 1.0
    uint32_t           f_18F; // v12 +0x18F (feb +0x173) init 1.0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x004723B0 (feb 0x004686E0) ??_EwrsfxDUST@@UAEPAXI@Z
    // slot +0x04 v12 0x004725C0 (feb 0x004688F0) wrsfxDUST::ProcessMsg
    // non-virtual / static:
    // v12 0x00472330 (feb 0x00468660) wrsfxDUST::Create
    // v12 0x004725C0 (feb 0x004688F0) wrsfxDUST::ProcessMsg
};

// entity class name "sfx_flame", entRegisterClass sig 0x24444E54, prio 45
// Create: v12 0x00472FE0 (feb 0x00469310); vtable v12 0x00601BD8 (feb 0x005D67B8)
struct wrsfxFLAME : entENTITY {  // size v12 0x183 (feb 0x167), grew +0x1C (all from base)
    uint32_t           flameFlags; // v12 +0x16B (feb +0x14F) 1 day and night, 2 / 4 messages 0x4D0 / 0x4D1, 8 burning
    class m3dCTRL_1D_SPL *cycle;   // v12 +0x16F (feb +0x153) the day and night cycle: above 0.5 it burns
    class pteSHADER   *light;      // v12 +0x173 (feb +0x157) [FLAME] isLight: a pteSPOT_ANIM, not rebuilt (always 0 here)
    partEMITTER_PHYS  *smoke;      // v12 +0x177 (feb +0x15B) puffs as it goes out
    partEMITTER_PHYS  *sparks;     // v12 +0x17B (feb +0x15F) burst as it lights
    uint32_t           f_17F;      // v12 +0x17F (feb +0x163) init 0
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00473260 (src/Sources/sfx_flame.cpp)
    BOOL32 SetupAniSystem();                                        // v12 0x00473040
    void   SfxStart();                                              // (inline in ProcessMsg) lit: sparks, light, event 1
    void   SfxStop();                                               // (inline in ProcessMsg) out: smoke, no light, event 2
    static entENTITY *__fastcall Create(animINST *inst);            // v12 0x00472FE0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00473BC0 (feb 0x00469EE0) ??_GwrsfxFLAME@@UAEPAXI@Z
    // slot +0x04 v12 0x00473260 (feb 0x00469590) wrsfxFLAME::ProcessMsg
    // slot +0x40 v12 0x00473040 (feb 0x00469370) wrsfxFLAME::SetupAniSystem
    // non-virtual / static:
    static txmTEXTURE *pTexSpark;  // v12 0x0064ED6C "sfx_flame_spark" (wrsfxFlameInit)
    static txmTEXTURE *pTexSmoke;  // v12 0x0064ED70 "sfx_mcg_cloud"
    // v12 0x00472FE0 (feb 0x00469310) wrsfxFLAME::Create
    // v12 0x00473040 (feb 0x00469370) wrsfxFLAME::SetupAniSystem
    // v12 0x00473260 (feb 0x00469590) wrsfxFLAME::ProcessMsg
};

int __fastcall wrsfxFlameInit();    // v12 0x00472AD0 (feb 0x00468E00)

// entity class name "flare", entRegisterClass sig 0x24464C52, prio 85
// Create: v12 0x00473C20 (feb 0x00469F40); vtable v12 0x00601C58 (feb 0x005D6840)
struct wrsfxFLARE : flrFLARE {  // size ? [integrator: 0x40000 was the texture-size argument of flrFLARE::Create, not the size], grew +0x0 (all from base)
    // vtable overrides (v12 slots that point into this class):
    // non-virtual / static:
};

// entity class name "sfx_fountain", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004750C0 (feb 0x0046B3E0); vtable v12 0x00601E20 (feb 0x005D6A10)
struct wrsfxFOUNTAIN : entENTITY {  // size v12 0x19B (feb 0x17F); src/Sources/sfx_fountain.cpp
    struct PART {
        partEMITTER_PHYS* emitter;  // the jet's base emitter (its particles' birth)
        partEMITTER_PHYS* child;    // what each of them emits (0 for the drops)
        objOBJ*           area;     // "&h_&nc_Area*"
    } part[4];                      // v12 +0x16B (feb +0x14F) Top, Med, Bot, Drops
    BOOL32 ProcessINIT(float s);                                   // v12 0x00474B70 (feb 0x0046AE90)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00475060 (feb 0x0046B380)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x004750C0 (feb 0x0046B3E0)
};

// entity class name "sfx_gefest_expl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00476450 (feb 0x0046C770); vtable v12 0x00602020 (feb 0x005D6C10)
struct wrsfxGEFEST_EXPL : entENTITY {  // size v12 0x17B (feb 0x15F); src/Sources/sfx_gefest.cpp
    partEMITTER_PHYS*  parts;  // v12 +0x16B (feb +0x14F) wrsfxGEFEST_EXPL_PARTS
    partEMITTER_PHYS*  smoke;  // v12 +0x16F (feb +0x153) wrsfxGEFEST_EXPL_SMOKE
    partEMITTER_PHYS*  glow;   // v12 +0x173 (feb +0x157) wrsfxGEFEST_EXPL_GLOW
    float              timer;  // v12 +0x177 (feb +0x15B) 1.3 s, then gone
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00476170 (feb 0x0046C490)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x00476450 (feb 0x0046C770)
};

// entity class name "sfx_gefest_hit", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004763D0 (feb 0x0046C6F0); vtable v12 0x00601FB8 (feb 0x005D6BA8)
struct wrsfxGEFEST_HIT : entENTITY {  // size v12 0x17B (feb 0x15F); src/Sources/sfx_gefest.cpp
    partEMITTER_PHYS*  lgtn;   // v12 +0x16B (feb +0x14F) wrsfxGEFEST_HIT_LGTN
    partEMITTER_PHYS*  glow;   // v12 +0x16F (feb +0x153) wrsfxGEFEST_HIT_GLOW
    partEMITTER_PHYS*  sparks; // v12 +0x173 (feb +0x157) wrsfxGEFEST_HIT_SPARKS
    float              timer;  // v12 +0x177 (feb +0x15B) 1.5 s, then gone
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00475930 (feb 0x0046BC50)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x004763D0 (feb 0x0046C6F0)
};

// entity class name "sfx_glad_mace_expl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004770E0 (feb 0x0046D400); vtable v12 0x00602150 (feb 0x005D6D40)
struct wrsfxGLAD_MACE_EXPL : entENTITY {  // size v12 0x17F (feb 0x163); src/Sources/sfx_glad.cpp
    partEMITTER_PHYS*  burn;     // v12 +0x16B (feb +0x14F)
    partEMITTER_PHYS*  glow;     // v12 +0x16F (feb +0x153)
    partEMITTER_PHYS*  sparks;   // v12 +0x173 (feb +0x157) from "sparks_emitter"
    partEMITTER_PHYS*  burn1;    // v12 +0x177 (feb +0x15B)
    float              timer;    // v12 +0x17B (feb +0x15F) 1 s, then gone
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00476E60 (feb 0x0046D180)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x004770E0 (feb 0x0046D400)
};

// entity class name "sfx_item", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004771B0 (feb 0x0046D4D0); vtable v12 0x006021C0 (feb 0x005D6DB0)
struct wrsfxITEM : dynDYNAMIC {  // size v12 0x1A7 (feb 0x183), grew +0x24 (all from base)
    uint32_t           f_19B; // v12 +0x19B (feb +0x177) init 0
    uint32_t           f_19F; // v12 +0x19F (feb +0x17B) init 0
    uint32_t           f_1A3; // v12 +0x1A3 (feb +0x17F) init 0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00477250 (feb 0x0046D560) ??_GwrsfxITEM@@UAEPAXI@Z
    // slot +0x04 v12 0x00477280 (feb 0x0046D590) wrsfxITEM::ProcessMsg
    // non-virtual / static:
    // v12 0x004771B0 (feb 0x0046D4D0) wrsfxITEM::Create
    // v12 0x00477280 (feb 0x0046D590) wrsfxITEM::ProcessMsg
};

// entity class name "lava", entRegisterClass sig 0x4C415641, prio 45
// Create: v12 0x0047A040 (feb 0x00470080); vtable v12 0x00602600 (feb 0x005D71D0)
struct wrsfxLAVA : waterLAVA {  // size v12 0x19B (feb 0x17F), grew +0x1C (all from base)
    int32_t            f_173; // v12 +0x173 (feb +0x157)
    int32_t            f_177; // v12 +0x177 (feb +0x15B)
    uint32_t           f_17B; // v12 +0x17B (feb +0x15F)
    int32_t            f_17F; // v12 +0x17F (feb +0x163) init 100 // ?
    float              f_183; // v12 +0x183 (feb +0x167)
    int32_t            f_187; // v12 +0x187 (feb +0x16B) init 2
    int32_t            f_18B; // v12 +0x18B (feb +0x16F) init 1
    float              f_18F; // v12 +0x18F (feb +0x173) // ?
    float              f_193; // v12 +0x193 (feb +0x177) // ?
    float              f_197; // v12 +0x197 (feb +0x17B) // ?
    // vtable overrides (v12 slots that point into this class):
    // slot +0x04 v12 0x004792C0 (feb 0x0046F310) wrsfxLAVA::ProcessMsg
    // non-virtual / static:
    // v12 0x004792C0 (feb 0x0046F310) wrsfxLAVA::ProcessMsg
};

// entity class name "sfx_lava_expl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00479FC0 (feb 0x00470000); vtable v12 0x00602598 (feb 0x005D7168)
struct wrsfxLAVA_EXPL : entENTITY {  // size v12 0x17B (feb 0x15F), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F)
    uint32_t           f_16F; // v12 +0x16F (feb +0x153)
    uint32_t           f_173; // v12 +0x173 (feb +0x157)
    uint32_t           f_177; // v12 +0x177 (feb +0x15B) init 1.0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0047A010 (feb 0x00470050) ??_GwrsfxLAVA_EXPL@@MAEPAXI@Z
    // slot +0x04 v12 0x00478CB0 (feb 0x0046ED00) wrsfxLAVA_EXPL::ProcessMsg
    // non-virtual / static:
    // v12 0x00478CB0 (feb 0x0046ED00) wrsfxLAVA_EXPL::ProcessMsg
    // v12 0x00479FC0 (feb 0x00470000) wrsfxLAVA_EXPL::Create
};

// entity class name "sfx_lava_pjl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00479B90 (feb 0x0046FBF0); vtable v12 0x00602458 (feb 0x005D7048)
struct wrsfxLAVA_PJL : physPHYS {  // size v12 0x19C (feb 0x17C), grew +0x20 (all from base)
    uint32_t           f_17B; // v12 +0x17B (feb +0x15B) init -1.0
    uint32_t           f_17F; // v12 +0x17F (feb +0x15F) init 0
    uint32_t           f_183; // v12 +0x183 (feb +0x163) init 4294967295
    uint32_t           f_187; // v12 +0x187 (feb +0x167) init 4294967295
    uint32_t           f_18B; // v12 +0x18B (feb +0x16B) init 4294967295
    uint32_t           f_18F; // v12 +0x18F (feb +0x16F) init -1.0 // ?
    uint32_t           f_194; // v12 +0x194 (feb +0x174) init 0 // ?
    uint32_t           f_198; // v12 +0x198 (feb +0x178) init 0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00479DA0 (feb 0x0046FFD0) ??_E?$pjlPJL@UwrsfxLAVA_TRAIL@@@@MAEPAXI@Z
    // slot +0x80 v12 0x004782D0 (feb 0x0046E5E0) wrsfxLAVA_PJL::ShowCollisSFX
    // non-virtual / static:
    // v12 0x004782D0 (feb 0x0046E5E0) wrsfxLAVA_PJL::ShowCollisSFX
    // v12 0x00479B90 (feb 0x0046FBF0) wrsfxLAVA_PJL::Create
};

// entity class name "sfx_lava_start", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00479B10 (feb 0x0046FB70); vtable v12 0x006023F0 (feb 0x005D6FE0)
struct wrsfxLAVA_START : entENTITY {  // size v12 0x177 (feb 0x15B), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F)
    uint32_t           f_16F; // v12 +0x16F (feb +0x153)
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 1.5
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00479B60 (feb 0x0046FBC0) ??_GwrsfxLAVA_START@@MAEPAXI@Z
    // slot +0x04 v12 0x00477C50 (feb 0x0046DF60) wrsfxLAVA_START::ProcessMsg
    // non-virtual / static:
    // v12 0x00477C50 (feb 0x0046DF60) wrsfxLAVA_START::ProcessMsg
    // v12 0x00479B10 (feb 0x0046FB70) wrsfxLAVA_START::Create
};

// entity class name "sfx_medusa_death", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0047B650 (feb 0x00471680); vtable v12 0x006028D0 (feb 0x005D74B8)
struct wrsfxMEDUSA_DEATH : dynDYNAMIC {  // size v12 0x1B3 (feb 0x18F), grew +0x24 (all from base)
    uint32_t           f_19B; // v12 +0x19B (feb +0x177)
    uint32_t           f_19F; // v12 +0x19F (feb +0x17B)
    uint32_t           f_1A3; // v12 +0x1A3 (feb +0x17F)
    uint32_t           f_1A7; // v12 +0x1A7 (feb +0x183)
    uint32_t           f_1AB; // v12 +0x1AB (feb +0x187)
    uint32_t           f_1AF; // v12 +0x1AF (feb +0x18B) init 2.0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0047B700 (feb 0x00471720) ??_EwrsfxMEDUSA_DEATH@@MAEPAXI@Z
    // slot +0x64 v12 0x0047B2C0 (feb 0x00471300) wrsfxMEDUSA_DEATH::ProcessINIT
    // slot +0x6C v12 0x0047B4C0 (feb 0x00471500) wrsfxMEDUSA_DEATH::ProcessTERM
    // non-virtual / static:
    // v12 0x0047B2C0 (feb 0x00471300) wrsfxMEDUSA_DEATH::ProcessINIT
    // v12 0x0047B4C0 (feb 0x00471500) wrsfxMEDUSA_DEATH::ProcessTERM
    // v12 0x0047B650 (feb 0x00471680) wrsfxMEDUSA_DEATH::Create
};

// entity class name "sfx_medusa_expl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0047B570 (feb 0x004715B0); vtable v12 0x00602860 (feb 0x005D7440)
struct wrsfxMEDUSA_EXPL : dynDYNAMIC {  // size v12 0x1B3 (feb 0x18F), grew +0x24 (all from base)
    uint32_t           f_19B; // v12 +0x19B (feb +0x177)
    objOBJ*            f_19F; // v12 +0x19F (feb +0x17B)
    uint32_t           f_1A3; // v12 +0x1A3 (feb +0x17F)
    uint32_t           f_1A7; // v12 +0x1A7 (feb +0x183)
    objOBJ*            f_1AB; // v12 +0x1AB (feb +0x187)
    uint32_t           f_1AF; // v12 +0x1AF (feb +0x18B) init 1.0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0047B620 (feb 0x00471650) ??_EwrsfxMEDUSA_EXPL@@MAEPAXI@Z
    // slot +0x64 v12 0x0047A730 (feb 0x00470770) wrsfxMEDUSA_EXPL::ProcessINIT
    // slot +0x68 v12 0x0047B540 (feb 0x00471580) wrsfxMEDUSA_EXPL::ProcessFRAME
    // slot +0x6C v12 0x0047A930 (feb 0x00470970) wrsfxMEDUSA_EXPL::ProcessTERM
    // slot +0xD8 v12 0x0047B540 (feb 0x00471580) wrsfxMEDUSA_EXPL::ProcessFRAME
    // non-virtual / static:
    // v12 0x0047A730 (feb 0x00470770) wrsfxMEDUSA_EXPL::ProcessINIT
    // v12 0x0047A930 (feb 0x00470970) wrsfxMEDUSA_EXPL::ProcessTERM
    // v12 0x0047B540 (feb 0x00471580) wrsfxMEDUSA_EXPL::ProcessFRAME
    // v12 0x0047B570 (feb 0x004715B0) wrsfxMEDUSA_EXPL::Create
};

// entity class name "sfx_mirrors", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0047C9B0 (feb 0x00472720); vtable v12 0x006029C0 (feb 0x005D75A8)
struct wrsfxMIRRORS : entENTITY {  // size v12 0x17B (feb 0x15F), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F)
    uint32_t           f_16F; // v12 +0x16F (feb +0x153)
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 0
    uint32_t           f_177; // v12 +0x177 (feb +0x15B) init 0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0047CA00 (feb 0x00472770) ??_EwrsfxMIRRORS@@MAEPAXI@Z
    // slot +0x04 v12 0x0047C500 (feb 0x00472460) wrsfxMIRRORS::ProcessMsg
    // non-virtual / static:
    // v12 0x0047C500 (feb 0x00472460) wrsfxMIRRORS::ProcessMsg
    // v12 0x0047C9B0 (feb 0x00472720) wrsfxMIRRORS::Create
};

// entity class name "sfx_niche_explode", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0047D4C0 (feb 0x00473230); vtable v12 0x00602A28 (feb 0x005D7618)
struct wrsfxNICHE_EXPLODE : entENTITY {  // size v12 0x187 (feb 0x16B), grew +0x1C (all from base)  src/Sources/sob_niche.cpp
    struct { partEMITTER_PHYS *emitter; objOBJ *area; } part[3]; // v12 +0x16B grit 1, grit 2 (bouncing), dust
    float              timer;        // v12 +0x183 (feb +0x167) 7.5 s, then it goes
    static txmTEXTURE *pTexDust, *pTexGrit;                           // v12 0x0064EE54 / 0x0064EE58
    int    ProcessINIT(void *data);                                   // v12 0x0047D220 (feb 0x00472F90)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);   // v12 0x0047D3D0 (feb 0x00473140)
    static entENTITY *__fastcall Create(animINST *i);                 // v12 0x0047D4C0 (feb 0x00473230)
};
int __fastcall wrsfxNicheInit();          // v12 0x0047CA30

// entity class name "sfx_opp_appear", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0047D640 (feb 0x004733B0); vtable v12 0x00602B28 (feb 0x005D7720)
struct wrsfxOPP_APPEAR : entENTITY {  // size v12 0x173 (feb 0x157), grew +0x1C (all from base)
    partEMITTER_PHYS*  f_16B; // v12 +0x16B (feb +0x14F) init 0
    partEMITTER_PHYS*  f_16F; // v12 +0x16F (feb +0x153) init 0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0047D680 (feb 0x004733F0) ??_EwrsfxOPP_APPEAR@@MAEPAXI@Z
    // slot +0x04 v12 0x0047DAD0 (feb 0x00473840) wrsfxOPP_APPEAR::ProcessMsg
    // non-virtual / static:
    // v12 0x0047D640 (feb 0x004733B0) wrsfxOPP_APPEAR::Create
    // v12 0x0047DAD0 (feb 0x00473840) wrsfxOPP_APPEAR::ProcessMsg
};

// entity class name "sfx_perseus", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0047F010 (feb 0x00474CB0); vtable v12 0x00602C60 (feb 0x005D7858)
struct wrsfxPERSEUS : entENTITY {  // size v12 0x1BB (feb 0x187), grew +0x34 (all from base)
    uint32_t           f_183; // v12 +0x183 (feb +0x14F)
    objOBJ*            f_187; // v12 +0x187 (feb +0x153) // ?
    objOBJ*            f_18B; // v12 +0x18B (feb +0x157) // ?
    objOBJ*            f_18F; // v12 +0x18F (feb +0x15B) // ?
    partEMITTER_PHYS*  f_193; // v12 +0x193 (feb +0x15F) // ?
    uint32_t           f_197; // v12 +0x197 (feb +0x163) // ?
    uint32_t           f_19B; // v12 +0x19B (feb +0x167) init 0
    int32_t            f_19F; // v12 +0x19F (feb +0x16B)
    uint32_t           f_1A3; // v12 +0x1A3 (feb +0x16F) init 1
    uint32_t           f_1A7; // v12 +0x1A7 (feb +0x173) init 1
    float              f_1AB; // v12 +0x1AB (feb +0x177) // ?
    float              f_1AF; // v12 +0x1AF (feb +0x17B) // ?
    float              f_1B3; // v12 +0x1B3 (feb +0x17F) // ?
    uint32_t           f_1B7; // v12 +0x1B7 (feb +0x183) // ?
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0047F090 (feb 0x00474D20) ??_GwrsfxPERSEUS@@UAEPAXI@Z
    // slot +0x04 v12 0x0047E300 (feb 0x00474070) wrsfxPERSEUS::ProcessMsg
    // slot +0x38 v12 0x0047ED40 (feb 0x00474B00) wrsfxPERSEUS::ProcessMP_WRITE_SYNC_DATA
    // slot +0x3C v12 0x0047EDF0 (feb 0x00474BB0) wrsfxPERSEUS::ProcessMP_READ_SYNC_DATA
    // slot +0x40 v12 0x0047E130 (feb 0x00473EA0) wrsfxPERSEUS::SetupAniSystem
    // non-virtual / static:
    // v12 0x0047E130 (feb 0x00473EA0) wrsfxPERSEUS::SetupAniSystem
    // v12 0x0047E2C0 (feb 0x00474030) wrsfxPERSEUS::Enable
    // v12 0x0047E300 (feb 0x00474070) wrsfxPERSEUS::ProcessMsg
    // v12 0x0047ED40 (feb 0x00474B00) wrsfxPERSEUS::ProcessMP_WRITE_SYNC_DATA
    // v12 0x0047EDF0 (feb 0x00474BB0) wrsfxPERSEUS::ProcessMP_READ_SYNC_DATA
    // v12 0x0047F010 (feb 0x00474CB0) wrsfxPERSEUS::Create
};

// entity class name "sfx_portal", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x0047F410 (feb 0x004750A0); vtable v12 0x00602D08 (feb 0x005D7900)
struct wrsfxPORTAL : entENTITY {  // size v12 0x187 (feb 0x16B), grew +0x1C (all from base)
    objOBJ            *emitter;    // v12 +0x16B (feb +0x14F) the object "emitter": the particles start on it, not drawn
    partEMITTER_PHYS  *flame;      // v12 +0x16F (feb +0x153)
    partEMITTER_PHYS  *sparks;     // v12 +0x173 (feb +0x157)
    class wrsfxPORTAL_VIEW *view;  // v12 +0x177 (feb +0x15B) the [PORTAL] tex on the portal
    sncCHANNEL        *sndAppear;  // v12 +0x17B (feb +0x15F)
    sncCHANNEL_CONE   *sndLoop;    // v12 +0x17F (feb +0x163)
    uint32_t           portalFlags;// v12 +0x183 (feb +0x167) 1 started
    static txmTEXTURE *pTexFlame;  // v12 0x0064F120
    static txmTEXTURE *pTexSpark;  // v12 0x0064F04C
    static sncSOUND_DESCR_3D snd[2]; // v12 0x0064F050
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x0047FE10 (src/Sources/sfx_portal.cpp)
    void   Enable(int on);                                          // v12 0x004801D0
    static entENTITY *__fastcall Create(animINST *inst);            // v12 0x0047F410
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0047F470 (feb 0x00475100) ??_GwrsfxPORTAL@@UAEPAXI@Z
    // slot +0x04 v12 0x0047FE10 (feb 0x00475AA0) wrsfxPORTAL::ProcessMsg
    // non-virtual / static:
    // v12 0x0047F410 (feb 0x004750A0) wrsfxPORTAL::Create
    // v12 0x0047FE10 (feb 0x00475AA0) wrsfxPORTAL::ProcessMsg
    // v12 0x004801D0 (feb 0x00475E60) wrsfxPORTAL::Enable
};

// the shader "wrsfx_portal": the [PORTAL] tex over the portal's objects, wobbling (size 0x124, vtable v12 0x00602D70)
class wrsfxPORTAL_VIEW : public pteEMAP {
public:
    float   time;               // +0x110
    float   speed;              // +0x114 0.7: the noise's movement up
    float   wobble;             // +0x118 0.05: how far the noise moves the texture
    float   noiseScale;         // +0x11C 3
    objOBJ *viewObj;            // +0x120 "&ds_view"
    wrsfxPORTAL_VIEW();                                 // (inline, v12 0x0047FFE9)
    static pteSHADER *__fastcall Create(animINST *i, objOBJ *o);    // v12 0x0047F4A0
    char *GetBlendTypeDef();                            // v12 0x0047F500
    void  Init();                                       // v12 0x0047F530
    void  PrepareFrame(scnSCENE *s, camCAMERA *c);      // v12 0x0047F590
    void  Update(float dt);                             // v12 0x0047F5D0
    int   ShadeVert(objOBJ *o, int v, void *out);       // v12 0x0047F5F0
    int   ShadeFace(objOBJ *, int, struct pteSDR_INFO *, struct ptePOLY_DATA *); // v12 0x004802A0 0
    int   IsActive(int);                                // v12 0x0048D590 1
    void  GetMatrL2W(m3dMATR *out);                     // v12 0x004E2EB0 matrL2W
};

class wrsfxPORTAL_FLAME : public partEMITTER_PHYS {
public:
    wrsfxPORTAL_FLAME();                // v12 0x0047FAC0 (feb 0x00475750)
};
class wrsfxPORTAL_SPARKS : public partEMITTER_PHYS {
public:
    wrsfxPORTAL_SPARKS();               // v12 0x0047FC90 (feb 0x00475920)
};
class wrsfxRAT_SPARKS_EMITTER : public partEMITTER_PHYS { public: wrsfxRAT_SPARKS_EMITTER(float size); };   // v12 0x00480380
class wrsfxRAT_EXPL_BURN : public partEMITTER_PHYS { public: wrsfxRAT_EXPL_BURN(float size); };             // v12 0x004808F0
class wrsfxRAT_EXPL_SMOKE : public partEMITTER_PHYS { public: wrsfxRAT_EXPL_SMOKE(float size); };           // v12 0x00480B50
class wrsfxRAT_EXPL_GLOW : public partEMITTER_PHYS { public: wrsfxRAT_EXPL_GLOW(float size); };             // v12 0x00480D80
int __fastcall wrsfxRatInit();          // v12 0x004802B0 (feb 0x00475F30)
int __fastcall aiRatInit();             // v12 0x00455350 (feb 0x0044CC00)
int __fastcall wrsfxPortalInit();       // v12 0x0047F360 (feb 0x00474FF0)

// entity class name "sfx_rat_expl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004811A0 (feb 0x00476E20); vtable v12 0x00602F20 (feb 0x005D7B18)
struct wrsfxRAT_EXPL : entENTITY {  // size v12 0x17B (feb 0x15F), grew +0x1C (all from base)
    partEMITTER_PHYS *burn;   // v12 +0x16B
    partEMITTER_PHYS *smoke;  // v12 +0x16F
    partEMITTER_PHYS *glow;   // v12 +0x173
    float timer;              // v12 +0x177 2 s, then it goes
    static txmTEXTURE *pTexBurn, *pTexSmoke, *pTexGlow;
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00480FA0 (src/Sources/sfx_rat.cpp)
    static entENTITY *__fastcall Create(animINST *i);               // v12 0x004811A0
};

// entity class name "sfx_rat_sparks", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00481220 (feb 0x00476EA0); vtable v12 0x00602F88 (feb 0x005D7B80)
struct wrsfxRAT_SPARKS : entENTITY {  // size v12 0x17F (feb 0x163), grew +0x1C (all from base)
    partEMITTER_PHYS *emitter; // v12 +0x16B
    entENTITY *owner;          // v12 +0x16F
    objOBJ *source;            // v12 +0x173 the owner's "source_sparks"
    int   started;             // v12 +0x177
    float size;                // v12 +0x17B
    static txmTEXTURE *pTexSparks;
    void Start();                                                   // v12 0x00480550
    void Stop();                                                    // v12 0x004806B0
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x004806F0
    static entENTITY *__fastcall Create(animINST *i);               // v12 0x00481220
};

// entity class name "sfx_satir_arrow_land", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004819C0 (feb 0x00477640); vtable v12 0x00603050 (feb 0x005D7C50)
class wrsfxSATIR_ARROW_LAND_CLOUD : public partEMITTER_PHYS { public: wrsfxSATIR_ARROW_LAND_CLOUD(float size); }; // v12 0x00481310
class wrsfxSATIR_ARROW_LAND_PART : public partEMITTER_PHYS { public: wrsfxSATIR_ARROW_LAND_PART(float size); };   // v12 0x00481510
int __fastcall wrsfxSatirArrowLandInit();  // v12 0x004812A0 (in aiSatyrInit)
struct wrsfxSATIR_ARROW_LAND : entENTITY {  // size v12 0x17B (feb 0x15F), grew +0x1C (all from base)  src/Sources/ai_satyr.cpp
    partEMITTER_PHYS  *cloud;   // v12 +0x16B (feb +0x14F) "sfx_mcg_cloud"
    partEMITTER_PHYS  *part;    // v12 +0x16F (feb +0x153) "sfx_mcg_part" (bouncing on the hit plane)
    objOBJ            *area;    // v12 +0x173 (feb +0x157) "&h_&nc_area" where both emit
    float              timer;   // v12 +0x177 (feb +0x15B) 4.1 s, then it goes
    static txmTEXTURE *pTexCloud, *pTexPart;    // v12 0x0064F13C / 0x0064F140
    void   ProcessINIT(float *size);                                // v12 0x00481760 (feb 0x004773E0)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x004818D0 (feb 0x00477550)
    static entENTITY *__fastcall Create(animINST *i);               // v12 0x004819C0 (feb 0x00477640)
};

// entity class name "sfx_skel_death", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004836D0 (feb 0x00479340); vtable v12 0x006031C8 (feb 0x005D7DC8)
struct wrsfxSKELETON_DEATH : entENTITY {  // size v12 0x1AF (feb 0x193), grew +0x1C (all from base)
    struct { partEMITTER_PHYS *shard, *dust; entENTITY *ent; objOBJ *obj; } piece[3]; // v12 +0x16B
    partEMITTER_PHYS   *emitter;     // v12 +0x19B
    entENTITY          *mainEnt;     // v12 +0x19F
    objOBJ             *mainObj;     // v12 +0x1A3
    float               size;        // v12 +0x1A7
    float               timer;       // v12 +0x1AB 4 s
    wrsfxSKELETON_DEATH() : emitter(0), mainEnt(0), mainObj(0), size(0), timer(0) { memset(piece, 0, sizeof(piece)); }
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);
    void   ProcessFRAME();
    static entENTITY *__fastcall Create(animINST *i);
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00483730 (feb 0x004793A0) ??_GwrsfxSKELETON_DEATH@@UAEPAXI@Z
    // slot +0x04 v12 0x00483540 (feb 0x004791B0) wrsfxSKELETON_DEATH::ProcessMsg
    // non-virtual / static:
    // v12 0x00482DA0 (feb 0x00478A20) wrsfxSKELETON_DEATH::AddPiece
    // v12 0x004833C0 (feb 0x00479030) wrsfxSKELETON_DEATH::ProcessFRAME
    // v12 0x00483450 (feb 0x004790C0) wrsfxSKELETON_DEATH::ProcessTERM
    // v12 0x004834C0 (feb 0x00479130) wrsfxSKELETON_DEATH::ProcessDESTROY_ENT
    // v12 0x00483540 (feb 0x004791B0) wrsfxSKELETON_DEATH::ProcessMsg
    // v12 0x004836D0 (feb 0x00479340) wrsfxSKELETON_DEATH::Create
};

// entity class name "sfx_skel_spear_appear", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x004835A0 (feb 0x00479210); vtable v12 0x00603230 (feb 0x005D7E30)
class wrsfxSKELETON_SPEAR_APPEAR_CYL : public partEMITTER_PHYS { public: wrsfxSKELETON_SPEAR_APPEAR_CYL(float size); }; // v12 0x00481B50 (feb 0x004777D0)
int __fastcall wrsfxSkeletonInit();      // v12 0x00481A40 (feb 0x004776C0)
struct wrsfxSKELETON_SPEAR_APPEAR : entENTITY {  // size v12 0x18F (feb 0x173), grew +0x1C (all from base)
    partEMITTER_PHYS  *emitter;   // v12 +0x16B (feb +0x14F)
    objOBJ            *cylinder;  // v12 +0x16F (feb +0x153)
    float              timer;     // v12 +0x173 (feb +0x157) 1.16 s
    int                started;   // v12 +0x177 (feb +0x15B)
    entENTITY         *owner;     // v12 +0x17B (feb +0x15F)
    objOBJ            *srcObj;    // v12 +0x17F (feb +0x163) the weapon
    float              sx, sy, sz;// v12 +0x183 (feb +0x167)
    static txmTEXTURE *pCylTex;
    void   Start(int sword);
    void   Follow();
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);
    static entENTITY *__fastcall Create(animINST *i);
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00483620 (feb 0x00479290) ??_EwrsfxSKELETON_SPEAR_APPEAR@@UAEPAXI@Z
    // slot +0x04 v12 0x00481F20 (feb 0x00477BA0) wrsfxSKELETON_SPEAR_APPEAR::ProcessMsg
    // non-virtual / static:
    // v12 0x00481DD0 (feb 0x00477A50) wrsfxSKELETON_SPEAR_APPEAR::Start
    // v12 0x00481F20 (feb 0x00477BA0) wrsfxSKELETON_SPEAR_APPEAR::ProcessMsg
    // v12 0x004835A0 (feb 0x00479210) wrsfxSKELETON_SPEAR_APPEAR::Create
};

// entity class name "sfx_skel_spear_land", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00483650 (feb 0x004792C0); vtable v12 0x00603298 (feb 0x005D7E98)
struct wrsfxSKELETON_SPEAR_LAND : entENTITY {  // size v12 0x17F (feb 0x163); src/Sources/sfx_skeleton.cpp
    partEMITTER_PHYS*  grit;     // v12 +0x16B (feb +0x14F) wrsfxSKELETON_SPEAR_LAND_GRIT
    partEMITTER_PHYS*  clouds;   // v12 +0x16F (feb +0x153) wrsfxSKELETON_SPEAR_LAND_CLOUDS
    objOBJ*            circle;   // v12 +0x173 (feb +0x157) "Circle" (the area they come from)
    float              timer;    // v12 +0x177 (feb +0x15B) 5.1 s (Start)
    int                running;  // v12 +0x17B (feb +0x15F)
    static txmTEXTURE *pGritTex, *pCloudTex;           // "sfx_mcg_part", "sfx_mcg_cloud"
    void   Start();                                                 // v12 0x00482680 (feb 0x00478300)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x004826C0 (feb 0x00478340)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x00483650 (feb 0x004792C0)
};

// entity class name "sfx_sphinx_pjl_expl", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00483DF0 (feb 0x00479A60); vtable v12 0x00603360 (feb 0x005D7F68)
struct wrsfxSPHINX_EXPL : entENTITY {  // size v12 0x177 (feb 0x15B), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F)
    uint32_t           f_16F; // v12 +0x16F (feb +0x153)
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 1.0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00483E40 (feb 0x00479AB0) ??_GwrsfxSPHINX_EXPL@@MAEPAXI@Z
    // slot +0x04 v12 0x00483BF0 (feb 0x00479860) wrsfxSPHINX_EXPL::ProcessMsg
    // non-virtual / static:
    // v12 0x00483BF0 (feb 0x00479860) wrsfxSPHINX_EXPL::ProcessMsg
    // v12 0x00483DF0 (feb 0x00479A60) wrsfxSPHINX_EXPL::Create
};

// entity class name "sfx_treasure", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00484840 (feb 0x0047A4A0); vtable v12 0x00603510 (feb 0x005D8130)
class wrsfxTREASURE_SPARKS_1 : public partEMITTER_PHYS { public: wrsfxTREASURE_SPARKS_1(float size); }; // v12 0x00484B00 (feb 0x0047A7F0)
class wrsfxTREASURE_SPARKS_2 : public partEMITTER_PHYS { public: wrsfxTREASURE_SPARKS_2(float size); }; // v12 0x00484BE0 (feb 0x0047A8D0)
int __fastcall wrsfxTreasureInit();      // v12 0x004847F0 (feb 0x0047A450)
struct wrsfxTREASURE : dynDYNAMIC {  // size v12 0x1A7 (feb 0x183), grew +0x24 (all from base)
    objOBJ*            emitterObj; // v12 +0x19B (feb +0x177) init 0
    partEMITTER_PHYS*  sparks1;    // v12 +0x19F (feb +0x17B) init 0
    partEMITTER_PHYS*  sparks2;    // v12 +0x1A3 (feb +0x17F) init 0
    static txmTEXTURE *pTexSparks;
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);
    static entENTITY *__fastcall Create(animINST *i);
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x004848E0 (feb 0x0047A530) ??_GwrsfxTREASURE@@UAEPAXI@Z
    // slot +0x04 v12 0x00484910 (feb 0x0047A560) wrsfxTREASURE::ProcessMsg
    // non-virtual / static:
    // v12 0x00484840 (feb 0x0047A4A0) wrsfxTREASURE::Create
    // v12 0x00484910 (feb 0x0047A560) wrsfxTREASURE::ProcessMsg
};

// entity class name "sfx_water_stream", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00484E50 (feb 0x0047AB40); vtable v12 0x006035E8 (feb 0x005D8210)
struct wrsfxWATER : entENTITY {  // size v12 0x17F (feb 0x163); src/Sources/sfx_water.cpp
    objOBJ*            emitterObj; // v12 +0x16B (feb +0x14F) "emitter" (where it lands)
    partEMITTER_PHYS*  stream;     // v12 +0x16F (feb +0x153) wrsfxWATER_STREAM
    partEMITTER_PHYS*  cloud1;     // v12 +0x173 (feb +0x157) wrsfxWATER_STREAM_CLOUD_1
    partEMITTER_PHYS*  cloud2;     // v12 +0x177 (feb +0x15B) wrsfxWATER_STREAM_CLOUD_2
    sncCHANNEL*        voice;      // v12 +0x17B (feb +0x15F) its sound (a sphere)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00485420 (feb 0x0047B110)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x00484E50 (feb 0x0047AB40)
};

// entity class name "sfx_waterfall", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00485B80 (feb 0x0047B870); vtable v12 0x00603718 (feb 0x005D8340)
struct wrsfxWATERFALL : entENTITY {  // size v12 0x18F (feb 0x173); src/Sources/sfx_wfall.cpp
    partEMITTER_PHYS*  foam[9];    // v12 +0x16B (feb +0x14F) one over each "&h_&nc_area*"
    BOOL32 ProcessINIT(float s);                                   // v12 0x00485A00 (feb 0x0047B6F0)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00485AE0 (feb 0x0047B7D0)
    static entENTITY *__fastcall Create(animINST *i);              // v12 0x00485B80 (feb 0x0047B870)
};

// entity class name "sfx_zeus_lgtn", entRegisterClass sig 0x24534658, prio 80
// Create: v12 0x00486490 (feb 0x0047BFD0); vtable v12 0x006037F0 (feb 0x005D8418)
struct wrsfxZEUS_LGTN : dynDYNAMIC {  // size v12 0x1A7 (feb 0x183), grew +0x24 (all from base)
    uint32_t           f_19B; // v12 +0x19B (feb +0x177)
    uint32_t           f_19F; // v12 +0x19F (feb +0x17B)
    uint32_t           f_1A3; // v12 +0x1A3 (feb +0x17F) init 1.7000000476837158
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00486530 (feb 0x0047C060) ??_EwrsfxZEUS_LGTN@@MAEPAXI@Z
    // slot +0x64 v12 0x00486210 (feb 0x0047BD90) wrsfxZEUS_LGTN::ProcessINIT
    // slot +0x68 v12 0x00486440 (feb 0x0047BF80) wrsfxZEUS_LGTN::ProcessFRAME
    // slot +0x6C v12 0x00486400 (feb 0x0047BF40) wrsfxZEUS_LGTN::ProcessTERM
    // non-virtual / static:
    // v12 0x00486210 (feb 0x0047BD90) wrsfxZEUS_LGTN::ProcessINIT
    // v12 0x00486400 (feb 0x0047BF40) wrsfxZEUS_LGTN::ProcessTERM
    // v12 0x00486440 (feb 0x0047BF80) wrsfxZEUS_LGTN::ProcessFRAME
    // v12 0x00486490 (feb 0x0047BFD0) wrsfxZEUS_LGTN::Create
};

#pragma pack(pop)
