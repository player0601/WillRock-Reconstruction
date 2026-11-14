// Will Rock unofficial SDK - wrsob* scene objects
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
#include "../game_shared/world_objects.h"   // bttnBUTTON (integrator)
// entity class name "nch_rigid", entRegisterClass sig 0x52474442, prio 50
// Create: v12 0x00488160 (feb 0x0047DBD0); vtable v12 0x00603A58 (feb 0x005D8698)
struct nchRIGID : rgdRIGID {  // size v12 0xF37 (feb 0xF1B), grew +0x1C (all from base)
    uint32_t           f_F33; // v12 +0xF33 (feb +0xF17) init 1
    // vtable overrides (v12 slots that point into this class):
    // slot +0x6C v12 0x004893E0 (feb 0x0047ED50) nchRIGID::ProcessFRAME
    // non-virtual / static:
    // v12 0x00488160 (feb 0x0047DBD0) nchRIGID::Create
    // v12 0x004893E0 (feb 0x0047ED50) nchRIGID::ProcessFRAME
};

// entity class name "sob_blade", entRegisterClass sig 0x2444594E, prio 40
// Create: v12 0x00487100 (feb 0x0047CB90); vtable v12 0x00603868 (feb 0x005D8498)
struct wrsobBLADE : dynDYNAMIC {  // size v12 0x1BF (feb 0x19B), grew +0x24 (all from base)
    uint32_t           bladeFlags; // v12 +0x19B (feb +0x177) BLADEF_ (sob_blade.cpp)
    int                axis;       // v12 +0x19F (feb +0x17B) init 2: Z, [SOB_BLADE] isForwAxisX / isUseAxisX ...
    float              killMin;    // v12 +0x1A3 (feb +0x17F) [SOB_BLADE] killRangeMin: trackObj on the axis
    float              killMax;    // v12 +0x1A7 (feb +0x183) killRangeMax
    scnDOMAIN         *killArea;   // v12 +0x1AB (feb +0x187) the killArea object's outline (0: near the plane)
    objOBJ            *trackObj;   // v12 +0x1AF (feb +0x18B) (0: action frames say when)
    m3dV               trackPrev;  // v12 +0x1B3 (feb +0x18F) trackObj's origin last frame
    ~wrsobBLADE();                                                  // v12 0x004871C0
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00486600 (src/Sources/sob_blade.cpp)
    void ProcessMP_GET_SYNC_DATA(void *d);                          // v12 0x00486F30
    void ProcessMP_WRITE_SYNC_DATA(gsMSG_MP_SAVELOAD *d);           // v12 0x005567C0 (the button's)
    void ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d);            // v12 0x00486FC0
    static entENTITY *__fastcall Create(animINST *inst);            // v12 0x00487100
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x004871C0 (feb 0x0047CC40) ??_EwrsobBLADE@@UAEPAXI@Z
    // slot +0x04 v12 0x00486600 (feb 0x0047C130) wrsobBLADE::ProcessMsg
    // non-virtual / static:
    // v12 0x00486600 (feb 0x0047C130) wrsobBLADE::ProcessMsg
    // v12 0x00487100 (feb 0x0047CB90) wrsobBLADE::Create
};

// entity class name "sob_geyser", entRegisterClass sig 0x2444594E, prio 40
// Create: v12 0x00487270 (feb 0x0047CCF0); vtable v12 0x006038E0 (feb 0x005D8510)
struct wrsobGEYSER : entENTITY {  // size v12 0x18B (feb 0x16F), grew +0x1C (all from base)
    uint32_t           f_16B; // v12 +0x16B (feb +0x14F) init 4294967295
    uint32_t           f_16F; // v12 +0x16F (feb +0x153) init 0.6666666865348816
    uint32_t           f_173; // v12 +0x173 (feb +0x157) init 0
    objOBJ*            f_177; // v12 +0x177 (feb +0x15B) init 0
    uint32_t           f_17B; // v12 +0x17B (feb +0x15F) init 0
    objOBJ*            f_17F; // v12 +0x17F (feb +0x163) init 0
    uint32_t           f_183; // v12 +0x183 (feb +0x167) init 0
    objOBJ*            f_187; // v12 +0x187 (feb +0x16B) init 0
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x004872D0 (feb 0x0047CD50) ??_GwrsobGEYSER@@MAEPAXI@Z
    // slot +0x04 v12 0x00487A20 (feb 0x0047D4A0) wrsobGEYSER::ProcessMsg
    // non-virtual / static:
    // v12 0x00487270 (feb 0x0047CCF0) wrsobGEYSER::Create
    // v12 0x00487960 (feb 0x0047D3E0) wrsobGEYSER::UpdateState
    // v12 0x00487A20 (feb 0x0047D4A0) wrsobGEYSER::ProcessMsg
};

// entity class name "sob_katapult", entRegisterClass sig 0x54524D50, prio 40
// Create: v12 0x00487F70 (feb 0x0047D9F0); vtable v12 0x006039E0 (feb 0x005D8618)
struct wrsobKATAPULT : dynDYNAMIC {  // size v12 0x1AF (feb 0x18B), grew +0x24 (all from base)
    objOBJ*            cup;    // v12 +0x19B (feb +0x177) the object "chasha": the player stands on it (0: the katapult does nothing)
    float              height; // v12 +0x19F (feb +0x17B) [KATAPULT] height (20): how far above the start the throw goes
    m3dV               target; // v12 +0x1A3 (feb +0x17F) the position of the instance [KATAPULT] target
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x00487DB0 (src/Sources/sob_katapult.cpp)
    static entENTITY *__fastcall Create(animINST *inst);            // v12 0x00487F70
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x00488030 (feb 0x0047DAA0) ??_GwrsobKATAPULT@@UAEPAXI@Z
    // slot +0x04 v12 0x00487DB0 (feb 0x0047D830) wrsobKATAPULT::ProcessMsg
    // non-virtual / static:
    // v12 0x00487DB0 (feb 0x0047D830) wrsobKATAPULT::ProcessMsg
    // v12 0x00487F70 (feb 0x0047D9F0) wrsobKATAPULT::Create
};

int __fastcall wrsobInit();               // v12 0x00486560 (feb 0x0047C090)
int __fastcall wrsobInitTree();           // v12 0x0048ADC0 (feb 0x00480060)
int __fastcall wrsobInitKatapult();      // v12 0x00487D80 (feb 0x0047D800)

// entity class name "sob_niche", entRegisterClass sig 0x2444594E, prio 40
// Create: v12 0x00489440 (feb 0x0047EDB0); vtable v12 0x00603B30 (feb 0x005D8770)
struct wrsobNICHE : entENTITY {  // size v12 0x183 (feb 0x163), grew +0x20 (all from base)  src/Sources/sob_niche.cpp
    int                exploded;     // v12 +0x16B (feb +0x14F) broken (synchronised)
    int                f_16F;        // v12 +0x16F (feb +0x153) 0: damage breaks it // ?
    float              distFromEdge; // v12 +0x173 (feb +0x157) NICHE distFromEdge (1.2 a wall, 0.05 a floor / roof)
    struct sncSOUND_DESCR_3D *sndBreak; // v12 +0x177 (1.2) NICHE SoundBreak (radii SoundBreakRadMin 15 / Max 80)
    txmTEXTURE*        tex;          // v12 +0x17B (feb +0x15B) NICHE tex: the broken faces' (else "stones_oblom")
    int                nPiece;       // v12 +0x17F (feb +0x15F) NICHE nPiece, 70
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);   // v12 0x004881E0 (feb 0x0047DC50)
    void   ProcessINIT();                                             // v12 0x004882F0 (feb 0x0047DD10)
    void   ProcessMP_GET_SYNC_DATA(void *d);                          // v12 0x00488500 (feb 0x0047DE20)
    void   ProcessMP_READ_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d);     // v12 0x00488570 (feb 0x0047DF10)
    void   StartExplos(int silent);                                   // v12 0x00488660 (feb 0x0047E000)
    static entENTITY *__fastcall Create(animINST *i);                 // v12 0x00489440 (feb 0x0047EDB0)
};
int __fastcall wrsobInitNiche();          // v12 0x004880C0 (feb 0x0047DB30)

// entity class name "sob_spike", entRegisterClass sig 0x2444594E, prio 40
// Create: v12 0x00489860 (feb 0x0047F1E0); vtable v12 0x00603BA0 (feb 0x005D87E0)
struct wrsobSPIKE : dynDYNAMIC {  // size v12 0x1A7 (feb 0x183), grew +0x24 (all from base)
    uint32_t           f_19B; // v12 +0x19B (feb +0x177) init 2
    animINST*          f_19F; // v12 +0x19F (feb +0x17B) init 0
    objOBJ*            f_1A3; // v12 +0x1A3 (feb +0x17F)
    // vtable overrides (v12 slots that point into this class):
    // slot +0x04 v12 0x004894F0 (feb 0x0047EE70) wrsobSPIKE::ProcessMsg
    // non-virtual / static:
    // v12 0x004894F0 (feb 0x0047EE70) wrsobSPIKE::ProcessMsg
    // v12 0x00489860 (feb 0x0047F1E0) wrsobSPIKE::Create
};

// sob_trading_altar.cpp, sfx_trading_altar.cpp
struct wrsobTRA_BUTTON;
struct wrsfxTRADING_ALTAR_EMITTER : partEMITTER_PHYS {   // size 0x2C4, vtable v12 0x00603C28
    wrsfxTRADING_ALTAR_EMITTER(float scale);             // v12 0x00489B00 (feb 0x0047F300)
};

// entity class name "sob_tra_altar", entRegisterClass sig 0x2444594E, prio 40
// Create: v12 0x0048AD20 (feb 0x0047FFD0); vtable v12 0x00603D38 (feb 0x005D8988)
struct wrsobTRA_ALTAR : entENTITY {  // size v12 0x18F (feb 0x167), grew +0x28 (all from base)
    int                state;       // v12 +0x16B 0 idle, 1 the sequence, 2 the effect
    float              time;        // v12 +0x16F what is left of the stage
    partEMITTER_PHYS  *emitter;     // v12 +0x173 wrsfxTRADING_ALTAR_EMITTER
    wrsobTRA_BUTTON   *button;      // v12 +0x177 the one pushed
    objOBJ            *placeEffect; // v12 +0x17B "place_effect"
    entENTITY         *buyer;       // v12 +0x17F
    entENTITY         *sfx;         // v12 +0x183 "sfx_trading_altar"
    sncCHANNEL        *sndOn;       // v12 +0x187 the sale's sounds (20..70)
    sncCHANNEL        *sndLoop;     // v12 +0x18B the glow's loop (15..55)
    static txmTEXTURE *pTexPart;    // v12 0x0064F600
    static sncSOUND_DESCR_3D snd[4]; // v12 0x0064F460
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x0048A5E0 (feb 0x0047F5F0)
    void   StartPowerup(wrsobTRA_BUTTON *b);    // v12 0x00489CC0 (feb 0x0047F4E0)
    void   StartEffect(int pw);                 // v12 0x00489DB0 (name ours)
    void   EndPowerup();                        // v12 0x00489ED0 (name ours)
    BOOL32 ProcessINIT(void *init);             // v12 0x00489F90 (name ours)
    void   ProcessFRAME();                      // v12 0x0048A3D0 (name ours)
    void   ProcessMP_NOTIFY(void *d);           // v12 0x0048A530 (name ours)
    static entENTITY *__fastcall Create(animINST *i);   // v12 0x0048AD20 (feb 0x0047FFD0)
};

// entity class name "sob_tra_button", entRegisterClass sig 0x4254544E, prio 40
// Create: v12 0x0048ABB0 (feb 0x0047FE70); vtable v12 0x00603C58 (feb 0x005D88A0)
struct wrsobTRA_BUTTON : bttnBUTTON {  // size v12 0x193 (feb 0x177), grew +0x1C (all from base)
    objOBJ            *playerPos;   // v12 +0x17B "player_pos": where the buyer stands
    animINST          *altarInst;   // v12 +0x17F the nearest "sob_tra_altar"
    objOBJ            *faceObj;     // v12 +0x183 the first object with faces
    int                powerup;     // v12 +0x187 plrPOWERUP_ID, -1
    int                locked;      // v12 +0x18B resting after a sale
    float              lockTime;    // v12 +0x18F
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // slot +0x04 v12 0x0048A830 (feb 0x0047FAF0)
    BOOL32 Toggle(entENTITY *by);               // slot +0x64 v12 0x0048A750 (feb 0x0047FAB0)
    BOOL32 IsCanPush(entENTITY *by);            // slot +0x68 v12 0x0048A670 (feb 0x0047F9B0)
    static entENTITY *__fastcall Create(animINST *i);   // v12 0x0048ABB0 (feb 0x0047FE70)
};

// entity class name "sob_tra_camera", entRegisterClass sig 0x2444594E, prio 40
// Create: v12 0x0048AC60 (feb 0x0047FF20); vtable v12 0x00603CC8 (feb 0x005D8910)
struct wrsobTRA_CAMERA : dynDYNAMIC {  // size v12 0x1A7 (feb 0x183), grew +0x24 (all from base)
    objOBJ            *cam[3];      // v12 +0x19B "camera_qd", "camera_ua", "camera_sm"
    BOOL32 ProcessINIT(void *init);             // slot +0x64 v12 0x0048AB30 (feb 0x0047FDF0)
    static entENTITY *__fastcall Create(animINST *i);   // v12 0x0048AC60 (feb 0x0047FF20)
};
int __fastcall wrsobInitTradingAltar();         // v12 0x00489A60 (feb 0x0047F270)

// entity class name "sfx_trading_altar", entRegisterClass sig 0x24534658, prio 0x50
struct wrsfxTRALTAR_PLAYER_GLOW : partEMITTER_PHYS {    // size 0x2C4, vtable v12 0x006033C8
    wrsfxTRALTAR_PLAYER_GLOW(float scale, int kind);    // v12 0x00483FD0 (feb 0x00479C40)
};
struct wrsfxTRALTAR_TRAIL_EMITTER : partEMITTER_PHYS {  // size 0x2C4, vtable v12 0x006033F8
    wrsfxTRALTAR_TRAIL_EMITTER(float scale, int kind);  // v12 0x00484200 (feb 0x00479E70)
};
struct wrsfxTRADING_ALTAR_INIT : msgDATA {              // its init data (0x0C bytes)
    float scale;                // +0x04
    int   kind;                 // +0x08 the powerup
};
struct wrsfxTRADING_ALTAR : dynDYNAMIC {  // size v12 0x1BB
    struct TEX { txmTEXTURE *part, *body, *trail, *glow; };     // v12 0x0064F16C, 3 of them
    static TEX tex[3];
    partEMITTER_PHYS  *glow;        // v12 +0x19B
    struct { partEMITTER_PHYS *emitter; objOBJ *obj; } trail[3]; // v12 +0x19F "plane01".."plane03"
    float              life;        // v12 +0x1B7 5 s
    BOOL32 ProcessINIT(void *init);             // slot +0x64 v12 0x00484400 (feb 0x0047A070)
    void   ProcessFRAME();                      // slot +0x68 v12 0x00484690 (feb 0x0047A300)
    void   ProcessTERM();                       // slot +0x6C v12 0x00484640 (feb 0x0047A2B0)
    static entENTITY *__fastcall Create(animINST *i);   // v12 0x00484700 (feb 0x0047A370)
};
int __fastcall wrsfxTradingAltarInit();         // v12 0x00483E90 (feb 0x00479B00)

// entity class name "sob_tree", entRegisterClass sig 0x54524545, prio 40
// Create: v12 0x0048B060 (feb 0x00480300); vtable v12 0x00603DA0 (feb 0x005D89F8)
struct wrsobTREE : entENTITY {  // size v12 0x16B (feb 0x14F), grew +0x1C (all from base)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x0048AFD0 (src/Sources/sob_tree.cpp)
    void   InitLeaves(objOBJ *o, int vertical);                     // v12 0x0048ADF0
    void   InitLeavesRoot(objOBJ *o);                               // v12 0x0048AF20
    static entENTITY *__fastcall Create(animINST *inst);            // v12 0x0048B060
    // vtable overrides (v12 slots that point into this class):
    // slot +0x00 v12 0x0048B090 (feb 0x00480330) ??_EwrsobTREE@@UAEPAXI@Z
    // slot +0x04 v12 0x0048AFD0 (feb 0x00480270) wrsobTREE::ProcessMsg
    // non-virtual / static:
    // v12 0x0048ADF0 (feb 0x00480090) wrsobTREE::InitLeaves
    // v12 0x0048AF20 (feb 0x004801C0) wrsobTREE::InitLeavesRoot
    // v12 0x0048AFD0 (feb 0x00480270) wrsobTREE::ProcessMsg
    // v12 0x0048B060 (feb 0x00480300) wrsobTREE::Create
};

#pragma pack(pop)
