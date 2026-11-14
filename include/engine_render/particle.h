// Will Rock 1.2 unofficial SDK - engine render: particle emitters
// objs: engine:particle.obj, Partrend.obj, partobj.obj
// Assumes from re/sdk/core: camCAMERA, animINST, objOBJ, m3dV, m3dMATR, m3dTRI, m3dPLANE, m3dSPL_COLOR, m3dSPL_LINEAR1D
//
// !! LAYOUT CHANGED IN RETAIL: partEMITTER_PHYS gained a 4-byte float at +0x34 (initialised to 5.0f together with +0x24).
// Every feb field at >= +0x34 is at +4 in v12 (checked field by field in ConstructorInit, v12 0x004D1810 vs feb 0x004C0260).
// That is why most partEMITTER_PHYS methods only matched by callgraph. Do not reuse feb offsets for this class.
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include <stdint.h>

#define PART_MAX_EMITTERS   600    // TWEAK: _partEmitList size. v12 mov ecx,0x258 at 0x004D1752 (partInit clear),
                                   //        cmp 0x258 at 0x004D1B39 and 0x004D1C67 (ConstructorInit: find free slot)
#define PART_MAX_VISIBLE    2000   // TWEAK: _partTmpList (sorted visible particles per frame). v12 cmp [_partNTmp],0x7D0 at 0x004D549E (RegisterPart)

class partEMITTER_PHYS;
// One particle (0x3C bytes for the base class; derived emitters may pass a larger size to ConstructorInit).
struct partPART {
    float    age;        // +0x00 0 .. 1 over the life; > 1 dead (KillParticle: 10)
    float    invLife;    // +0x04 1 / life
    float    size;       // +0x08
    uint32_t color;      // +0x0C ARGB (PartUpdateVisible)
    m3dV     pos;        // +0x10
    m3dV     vel;        // +0x1C
    float    texU, texV; // +0x28 / +0x2C texture scales
    float    rot;        // +0x30 angle
    float    rotSpeed;   // +0x34
    uint8_t  flags;      // +0x38 1 the emitter's flag 1 at birth, 4 dying (PartKill), 8 at rest (no more bounces)
    uint8_t  nHit;       // +0x39 bounces (PartApplyCDT)
    uint16_t frame;      // +0x3A texture frame offset
};
struct partEMITTER_VIS;            // visibility callback object (game: gsRENDER_EMITTER_VIS)

extern partEMITTER_PHYS *_partEmitList[PART_MAX_EMITTERS]; // v12 0x008FB1B0 (feb 0x008B6F90)
extern partPART         *_partTmpList[PART_MAX_VISIBLE];   // v12 0x008FBB10 (feb 0x008B78F0)
extern int               _partNTmp;                         // v12 0x008FDA54 (feb 0x008B9834)
extern class txmTEXTURE *_partDefaultTex;                   // v12 0x008FDA50 (feb 0x008B9830) "particle" texture loaded by partInit

int __fastcall partInit();                                           // v12 0x004D1730 (feb 0x004C0180) exact
void __fastcall partTerm();                                           // v12 0x004D1770 (feb 0x004C01C0) exact
void __fastcall partUpdate(camCAMERA *, float dt);                    // v12 0x004D41C0 (feb 0x004C51E0) per frame: Update all emitters
void __fastcall partRender(camCAMERA *, int pass, partEMITTER_VIS *); // v12 0x004D42F0 (feb 0x004C5310) per frame from gsRENDER_SYSTEM::RenderFrame. HOOK
void __fastcall partDestroyEmitters();                                // v12 0x004D42A0 (feb 0x004C52C0) exact
int __fastcall partGetNEmit();                                       // v12 0x004D42D0 (feb 0x004C52F0)
void __fastcall partAttachInst(partEMITTER_PHYS *, animINST *);       // v12 0x004D9B70 (feb 0x004CABB0)
void __fastcall partDetachInst(animINST *);                           // v12 0x004D9B80 (feb 0x004CABC0)
int __fastcall partSetAreaEmissionObj(partEMITTER_PHYS *, objOBJ *, int, m3dV *); // v12 0x004D9BF0 (feb 0x004CAC30) exact

class partEMITTER_PHYS {
public:
    // vtable v12 0x00606ED0 (feb 0x005DB950), 12 slots, all verified
    virtual void *__vecDelDtor(unsigned flags);                          // slot +0x00 v12 0x00402FF0 (feb 0x0047F4C0) (folded)
    virtual void SetColor(uint32_t, m3dSPL_COLOR *, m3dSPL_LINEAR1D *);  // slot +0x04 v12 0x004D1D10 (feb 0x004C0760)
    virtual void GetColor(uint32_t *, m3dSPL_COLOR **, m3dSPL_LINEAR1D **); // slot +0x08 v12 0x004D1D60 (feb 0x004C0780)
    virtual int  Update(camCAMERA *, float dt);                          // slot +0x0C v12 0x004D3730 (feb 0x004C2120) HOOK: per-emitter tick
    virtual void Emit(int n, float dt);                                  // slot +0x10 v12 0x004038E0 (feb 0x004035E0) = UpdateEmit
    virtual void PartHit(partPART *p, float t);                          // slot +0x14 v12 0x00403860 (empty) a particle hit a CDT plane (name ours)
    virtual void Register(camCAMERA *, partEMITTER_VIS *);               // slot +0x18 v12 0x004D5980 (feb 0x004C6980) push visible particles
    virtual void PartCreate(int, partPART **, float, int, int);          // slot +0x1C v12 0x004D2860 (feb 0x004C1250)
    virtual void PartUpdate(int, partPART **, float);                    // slot +0x20 v12 0x004D35A0 (feb 0x004C1F90)
    virtual void PartUpdateVisible(int, partPART **);                    // slot +0x24 v12 0x004D2290 (feb 0x004C0C80)
    virtual void PartKill(int n, partPART **list);                       // slot +0x28 v12 0x00403860 (empty) dying particles (UpdateErase)
    virtual int  PartApplyCDT(partPART *, m3dV *, float);                // slot +0x2C v12 0x004D3CA0 (feb 0x004C2690) particle collision
    // partEMITTER_PHYS_FLY: vtable feb 0x005DB980, Update v12 0x004D40E0 (feb 0x004C2AE0)

    partEMITTER_PHYS(float a, float b);                    // v12 0x004D1790 (feb 0x004C01E0) exact: ConstructorInit(this, 0x3C, max(3,ftol(..)), a, b)
    partEMITTER_PHYS(int, float, float);                   // v12 0x004D17E0 (feb 0x004C0230) exact
    int  ConstructorInit(int partSize, int maxPart, float, float); // v12 0x004D1810 (feb 0x004C0260)
    void StartEmission();                                  // v12 0x004D1C80 (feb 0x004C06D0)
    void StopEmission();                                   // v12 0x004D1CB0 (feb 0x004C0700)
    void SetAnimEmission(float, float);                    // v12 0x004D1CC0 (feb 0x004C0710)
    void SetIntensity(float, m3dSPL_LINEAR1D *);           // v12 0x004D1D90 (feb 0x004C07B0) exact
    void SetTimeLifePart(float, float);                    // v12 0x004D1DC0 (feb 0x004C07E0)
    void SetSize(float, float, m3dSPL_LINEAR1D *, float);  // v12 0x004D1DE0 (feb 0x004C0800)
    void SetTexture(class txmTEXTURE *);                   // v12 0x004D1E40 (feb 0x004C0830)
    void SetTextureScale(float, float, float, float);      // v12 0x004D1E50 (feb 0x004C0840)
    void SetTextureFrameAnim(int, int, float, float);      // v12 0x004D1E70 (feb 0x004C0860)
    void SetTextureRotAnim(float, float, float, float);    // v12 0x004D1ED0 (feb 0x004C0890)
    void SetDegNearParams(float, float);                   // v12 0x004D1EF0 (feb 0x004C08B0)
    void SetDegFarParams(float, float, float, float);      // v12 0x004D27E0 (feb 0x004C11D0)
    void SetPosDirSpeed(m3dV *pos, m3dV *dir, m3dV *speed);// v12 0x004D23D0 (feb 0x004C0DC0)
    void GetPosDirSpeed(m3dV *, m3dV *, m3dV *);           // v12 0x004D2530 (feb 0x004C0F20)
    int  SetAreaEmission(int, int, m3dTRI *, int);         // v12 0x004D25C0 (feb 0x004C0FB0)
    int  SetCDTPlane(int, int, m3dPLANE *);                // v12 0x004D2750 (feb 0x004C1140)
    void SetCDTParam(float, float, float, int);            // v12 0x004D2790 (feb 0x004C1180)
    void SetScaleCSVal(float, float, float);               // v12 0x004D2810 (feb 0x004C1200)
    void KillParticle(partPART *);                         // v12 0x004D2270 (feb 0x004C0C60)
    void Destroy();                                        // v12 0x004D1B60 (feb 0x004C05B0) ~partEMITTER_PHYS
    void SetCS(m3dMATR *m);                                // v12 0x004D25A0 (feb 0x004C0F90)
    void SetScaleCS(m3dMATR *m);                           // v12 (feb 0x004C1230)
    void AttachEmitter(partEMITTER_PHYS *e, int a, int b); // v12 (feb 0x004C11B0)
    void ClearParticles();                                 // v12 0x004D2280 (name ours) nPart and first = 0
    void UpdateUpdate(float dt);                           // v12 0x004D1F10 (feb 0x004C0900)
    void UpdateErase();                                    // v12 0x004D2070 (feb 0x004C0A60)
    int  UpdateGetNmbEmit(float dt);                       // v12 0x004D2120 (feb 0x004C0B10)
    void UpdateEmit(int n, float dt);                      // v12 0x004D21E0 (feb 0x004C0BD0)
    int  PredictCDT(m3dV *hit, float *t);                  // v12 0x004D3F20 (feb 0x004C2920)

    // ---- data (v12 offsets; feb +0x34 and later are 4 lower). Set by ConstructorInit v12 0x004D1810. -----------------
    uint32_t          flags;         // +0x04 0x80 emission timed (SetAnimEmission), 0x100 looped, 0x800 attached child,
                                     //       0x1000 colour by the emitter's time; 1 particles start "special" (byte +0x38)
    uint32_t          state;         // +0x08 1 emitting, 2 in the emission window, 4 / 8 started, 0x10 area set, 0x20
                                     //       to delete, 0x40 just started, 0x200 moved; ctor 0x2C0
    float             emitRate;      // +0x0C particles per second (ctor arg 1)
    float             intensity;     // +0x10 SetIntensity (ctor = emitRate)
    m3dSPL_LINEAR1D  *intensitySpl;  // +0x14 owned
    float             f_18;          // +0x18 ctor arg 2
    float             lifeMin;       // +0x1C SetTimeLifePart (ctor arg 2)
    float             lifeRand;      // +0x20
    float             sizeStart;     // +0x24 SetSize (5.0)
    float             sizeEnd;       // +0x28
    float             sizeRand;      // +0x2C 1.0
    m3dSPL_LINEAR1D  *sizeSpl;       // +0x30 owned
    float             sizeMax;       // +0x34 1.2 NEW: start + end, or the size spline's maximum (SetSize)
    uint32_t          color;         // +0x38 ARGB, 0x7FFFFFFF
    m3dSPL_COLOR     *colorSpl;      // +0x3C owned
    m3dSPL_LINEAR1D  *alphaSpl;      // +0x40 owned
    int32_t           maxPart;       // +0x44 the ring's size
    int32_t           first;         // +0x48 the oldest particle in the ring
    int32_t           nPart;         // +0x4C live particles
    partPART        **partPtrs;      // +0x50 maxPart pointers into the pool
    void             *partPool;      // +0x54 partSize * maxPart
    float             f_58;          // +0x58 0.5
    float             texScale[4];   // +0x5C SetTextureScale (1, 0, 1, 0: base and random of u and v)
    txmTEXTURE       *texture;       // +0x6C SetTexture (the "particle" texture at first)
    float             texRot[4];     // +0x70 SetTextureRotAnim
    int32_t           frameFirst;    // +0x80 SetTextureFrameAnim (1)
    int32_t           frameCount;    // +0x84 (1)
    float             frameFps;      // +0x88
    float             frameRand;     // +0x8C
    float             animEmitTime;  // +0x90 SetAnimEmission: emission window (1e6)
    float             animEmitRate;  // +0x94
    float             time;          // +0x98 the emitter's time (StartEmission: 0)
    float             degNear[2];    // +0x9C SetDegNearParams (1, 1)
    float             degFar[4];     // +0xA4 SetDegFarParams (1 x4)
    // the birth velocity (PartCreate): dirA * speedA + dirB * speedB + speed * speedK + tangent(speed) * speedT
    float             drag;          // +0xB4 velocity damping per second
    float             speedA;        // +0xB8 along a random direction in the cone round dir (or the area normal)
    float             speedARand;    // +0xBC
    m3dSPL_LINEAR1D  *speedASpl;     // +0xC0 owned; over the emitter's time
    float             speedB;        // +0xC4 along a random tangent of dir
    float             speedBRand;    // +0xC8
    m3dSPL_LINEAR1D  *speedBSpl;     // +0xCC owned
    float             speedK;        // +0xD0 times the emitter's speed
    float             speedKRand;    // +0xD4
    float             speedT;        // +0xD8 along a random tangent of the emitter's speed
    float             speedTRand;    // +0xDC
    m3dV              gravity;       // +0xE0
    float             spread;        // +0xEC the birth position's offset (radius)
    float             spreadMin;     // +0xF0
    m3dSPL_LINEAR1D  *spreadSpl;     // +0xF4 owned
    float             spreadH;       // +0xF8 != 0: a cylinder round dir of this height instead of a sphere
    float             cone;          // +0xFC degrees round dir (>= 180: any direction)
    m3dSPL_LINEAR1D  *coneSpl;       // +0x100 owned
    m3dV              pos;           // +0x104 SetPosDirSpeed
    m3dV              dir;           // +0x110 (unit Y)
    m3dV              speed;         // +0x11C
    m3dV              pos2;          // +0x128 // ?
    m3dV              dir2;          // +0x134 (unit Y) // ?
    m3dV              speed2;        // +0x140 // ?
    m3dMATR           cs;            // +0x14C SetCS (emission space)
    m3dMATR           scaleCS;       // +0x18C SetScaleCS
    float             scaleCSVal[3]; // +0x1CC SetScaleCSVal (1, 1, 1)
    int32_t           nArea;         // +0x1D8 SetAreaEmission: triangles
    m3dTRI           *area;          // +0x1DC
    m3dTRI           *areaPrev;      // +0x1E0
    m3dV             *areaNormal;    // +0x1E4
    m3dSPL_LINEAR1D  *areaSpl;       // +0x1E8 the areas' cumulative weights
    int32_t           cdtPlaneCount; // +0x1EC SetCDTPlane (collision planes for particles)
    uint8_t           cdtPlanes[4][0x18]; // +0x1F0 m3dPLANE[4]
    float             cdtDist;       // +0x250 SetCDTParam: kept this far over the plane
    float             cdtBounce;     // +0x254 the normal speed kept
    float             cdtFriction;   // +0x258 the tangential speed kept
    int32_t           cdtMaxHit;     // +0x25C bounces before rest (-1 any)
    partEMITTER_PHYS *parent;        // +0x260 AttachEmitter
    int32_t           parentArg;     // +0x264
    float             degFarK[2];    // +0x268 / +0x26C 1.0; Update: by the camera's distance from degFar (render
                                     //       scale, the emission's factor)
    uint8_t           _270[0x2C0 - 0x270];
    animINST         *attachInst;    // +0x2C0 0; partAttachInst (an item's sparks: its instance)
    // size 0x2C4 (pwpMakeShell / pwpMakeMuzzleSmoke allocate derived classes of 0x2CC / 0x2C4)
};
