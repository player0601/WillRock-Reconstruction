// wr_bases.h -- opaque engine base classes used by several subsystems (Will Rock unofficial SDK, integrator)
// Only the size is known for these; derived classes in sfx_sob / game_shared add their own fields after them.
// Sizes are taken from the first field offset of the derived classes (sfx_sob headers) and the Create "push size"
// in feb; "// ?" = v12 size derived, not read from a v12 Create.
#pragma once
#include "wr_core.h"
#pragma pack(push,1)

// dynDYNAMIC -- "dynamic": an animated level object (lifts, ropes, movers driven by IActions), and the class of
// every placed template instance without one (gsLVL_SYSTEM::LoadInstAll). gs dynamic.obj, rebuilt in
// src/gs/dynamic.cpp. Size v12 0x19B (Create v12 0x0050CFA0 "push 0x19B"; feb 0x177), vtable v12 0x00603498.
// 1.2 put two new fields first (the MP clients' catch-up), every beta field is 8 bytes up after entENTITY's +0x1C.
// Base of 14 wrsfx* effects (first own field v12 +0x19B).
enum dynFLAGS {                       // dynDYNAMIC::dynFlags
    DYNF_PLACE_EVERY_FRAME = 0x01,    // [DYN_PROP] PLACE_GROUND_EVERY_FRAME
    DYNF_PLACE_ON_WATER    = 0x02,    // PLACE_GROUND_ON_WATER
    DYNF_PLACE_ORIGIN      = 0x04,    // PLACE_GROUND_ORIGIN
    DYNF_PLAYING           = 0x08,    // StartAnim .. StopAnim / the end of a one-shot sequence
    DYNF_LONG_SINGLE_SEQ   = 0x10     // (server) one sequence longer than 10 s: its stop clears entFlags 0x80
};
class dynDYNAMIC : public entENTITY {
public:
    // vtable v12 0x00603498 (28 slots). Overrides: +0x04 ProcessMsg; the MP slots +0x1C 0x0050CA00, +0x20
    // 0x0050CAA0, +0x34 0x0050CB10, +0x38 0x0050CBB0, +0x3C 0x0050CCD0 (NOT rebuilt: the base ones run);
    // its own slots below.
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);       // +0x04 v12 0x0050C400 (feb 0x004F9960)
    virtual BOOL32 ProcessINIT(void *init);                                       // +0x64 v12 0x0050C6A0 (feb 0x004F9C50)
    virtual void   ProcessFRAME();                                                // +0x68 v12 0x0050C880 (feb 0x004F9DD0)
    virtual void   ProcessTERM();                                                 // +0x6C v12 0x00403A20 (empty)
    void StartAnim(int seq, float time, int cycle);   // v12 0x0050CE00 (feb 0x004FA100)
    void StopAnim();                                  // v12 0x0050CEE0 (feb 0x004FA1C0)
    static entENTITY *__fastcall Create(animINST *inst); // v12 0x0050CFA0 (feb 0x004FA260)
    float    catchUpRate;     // +0x16B v12 NEW 1.0: how fast a client takes in timeDebt (ProcessFRAME)
    float    timeDebt;        // +0x16F v12 NEW 0: animation time a client still has to catch up (MP)
    u32      dynFlags;        // +0x173 (feb +0x14F) dynFLAGS
    u32      unk177[4];       // +0x177 (feb +0x153..+0x15F) 0 // ?
    m3dV     unk187;          // +0x187 (feb +0x163) zero // ?
    u32      unk193;          // +0x193 (feb +0x16F) 0 // ?
    int      animMode;        // +0x197 (feb +0x173) AdvanceTime mode: 0 cycling, 1 once (StartAnim's cycle == 0)
};
int __fastcall dynInit();     // v12 0x0050C3C0 (feb 0x004F9920) registers "dynamic" 'DYN$' priority 0x28

// rgdRIGID -- rigid-body piece (engine rigid*.obj); size v12 0xF33 ? (feb 0xF17). Base of nchRIGID (sob_niche).
// Internals not located in 1.2 (see engine_core/README.md open questions).
class rgdRIGID : public entENTITY {
public:
    uint8_t _rgd[0xF33 - 0x16B];      // ? base is assumed to be entENTITY
};

// water / lava surface entities (gs_shared:water.obj, rebuilt in src/gs_shared/water.cpp); functions in
// game_shared/world_objects.h
class pteFOG_GROUND;
class waterLIQUID : public entENTITY {           // size v12 0x173 (feb 0x157)
public:
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x0054B180 INIT / TERM / INIT_LEVEL
    virtual BOOL32 InitObj(objOBJ *o, psSHEET *ps);              // [25] +0x64 v12 0x0054B460 one surface's [WATER]
    float          maxAmp;     // v12 +0x16B (feb +0x14F) the highest wave amplitude of its surfaces
    pteFOG_GROUND *fog;        // v12 +0x16F (feb +0x153) the volume ([FOG] section of the template)
    BOOL32 IsUnderWater(m3dV *p, float r, float *level);         // v12 0x0054BA30 (feb 0x00530810)
};
class waterWATER : public waterLIQUID {          // "water": entity class bit 1 (unkBC). vtable v12 0x00608BF0
public:
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x0054BAE0
    BOOL32 InitObj(objOBJ *o, psSHEET *ps);                      // v12 0x0054BB20 + REFLECTION shader
    static entENTITY *__fastcall Create(animINST *inst);         // v12 0x0054BC50
};
class waterLAVA : public waterLIQUID {           // "lava": entity class bit 2. vtable v12 0x00608C58
public:
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x0054BBE0
    BOOL32 InitObj(objOBJ *o, psSHEET *ps);                      // v12 0x0054BC20
    static entENTITY *__fastcall Create(animINST *inst);         // v12 0x0054BC90
};
// flrFLARE -- lens-flare entity (flare.obj). Size unknown (sfx_sob's 0x40000 was the texture size argument of
// flrFLARE::Create feb 0x0053C310, not the object size). Treat as opaque; only use pointers to it.
class flrFLARE : public entENTITY {
public:
    // size ?
};

#pragma pack(pop)
