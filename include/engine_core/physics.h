// physics.h -- Will Rock 1.2 unofficial SDK, engine_core: simple physics (physPHYS / physMATH) and rigid
//              bodies (rgdRIGID: debris, shatter pieces, barrels)
//
// objs covered: gs:physics.obj, gs:phys_rigid.obj, gs:phys_rigid_ball.obj, gs:phys_rigid_cdt.obj,
//               gs:phys_rigid_explos.obj   (player movement physics is NOT here: plrPLAYER, see results/player.json)
// Assumed: entENTITY (entity.h), animINST, cdtINFO / cdtREFINE (collision.h), m3dV, m3dMATR, msgDATA.
//
// physPHYS is an entity class ("phys", FourCC 'PHYS' 0x50485953, priority 0x28) that owns one physMATH:
// a point-mass integrator with gravity, collision response against the scene and a lifetime.
// Projectiles, gibs and thrown items use it through derived classes. Integration is explicit Euler with
// the frame dt (gsElapsedTime, max 0.1 s):  vel += accel*dt;  pos += vel*dt  (physMATH::CalcNewPos).

#pragma once
#include "core_types.h"
#include "entity.h"

#pragma pack(push, 1)
struct physINIT /* : msgDATA */ {  // data of MSG_MSG_INIT for physPHYS (copied to physMATH+0x5C, 0x38 bytes)
    u8    packId;                  // +0x00 msgDATA header
    u8    tag;                     // +0x01 '@'
    short size;                    // +0x02 0x38
    float vel[3];                  // +0x04 initial velocity      -> physMATH::vel
    float accel[3];                // +0x10 constant acceleration (gravity) -> physMATH::accel. |accel.xz| > 0.2 -> MP flag 0x20 else 0x40
    float radius;                  // +0x1C default 0.1           // ? meaning (collision radius)
    float lifetime;                // +0x20 default 2,000,000 s (= forever, const v12 0x006079A0 / feb 0x005DC4F4)
    union { int unk24; int maxHits; };       // +0x24 default 1: after this many hits it ends (< 1: never)
    union {
        int unk28[4];              // +0x28..+0x34 zero
        struct {
            float bounce;          // +0x28 how much of the speed across the surface a hit keeps
            float friction;        // +0x2C how much of the speed along it
            int   cdtMode;         // +0x30 the collision refine's mode (cdtREFINE_MODE)
            u32   cdtCls;          // +0x34 the entity classes it does not hit (entCDT_REFINE_EXCL)
        };
    };
};                                 // size 0x38
#pragma pack(pop)

enum physMATH_FLAGS {              // physMATH::flags (+0x04)
    PHYSM_SKIP_FRAME = 0x01,
    PHYSM_CDT_ANIM   = 0x02,       // a miss of the level still tests the instances near (a sphere)
    PHYSM_SPHERE     = 0x04,       // moves as a sphere of init.radius (else a ray)
    PHYSM_HIT        = 0x08,       // hit something this frame       // set by INIT: first ProcessPhysics only clears it
    PHYSM_AT_REST    = 0x10,       // > 3 consecutive "IsAtRest" frames: integration stops
    PHYSM_FLOOR_LIGHT= 0x20,       // physPHYS re-lights from the floor colour each frame (FloorPlace + ApplyColor)
};

// vtable physMATH v12 0x005FEFD4 (feb 0x005D3D78) (results/engine.json VT_PHYSM). No destructor slot.
class physMATH {
public:
    virtual BOOL32 ProcessINIT(physINIT *p);          // slot +0x00 v12 0x005094B0 (feb 0x004F6E10) copy init, vel=p->vel, flags|=1
    virtual void   ProcessTERM();                     // slot +0x04 v12 0x00509520 (feb 0x004F6EA0) (deletes itself) // ?
    virtual void   ProcessFRAME();                    // slot +0x08 v12 0x00509540 (feb 0x004F6EC0) ProcessPhysics; age += dt; lifetime -> TERM owner
    virtual void   ProcessDESTROY_ENT(entENTITY *e);  // slot +0x0C v12 0x0050A2B0 (feb 0x004F7BC0) drop references to e
    virtual BOOL32 ProcessCDT(cdtINFO *c);            // slot +0x10 v12 0x00403200 (feb 0x004036B0) base: return 0 (hit callback)
    virtual BOOL32 IsAtRest();                        // slot +0x14 v12 0x004547D0 (feb 0x0044C140) |vel| small
    virtual void   ProcessPhysics();                  // slot +0x18 v12 0x005095A0 (feb 0x004F6F20) (results/engine.json VS_PHYSD_PROCESS)
    virtual void   CalcNewPos(m3dV *from, m3dV *to);  // slot +0x1C v12 0x00509660 (feb 0x004F6FB0) Euler step
    u32       flags;               // +0x04 physMATH_FLAGS
    float     matr[16];            // +0x08 m3dMATR current transform (copied from/to the animINST)
    float     vel[3];              // +0x48 velocity (verified CalcNewPos v12 0x00509660)
    float     age;                 // +0x54 seconds alive
    int       restFrames;          // +0x58 consecutive at-rest frames (>3 -> PHYSM_AT_REST)
    physINIT  init;                // +0x5C copy of the init data: accel at +0x6C, radius +0x78, lifetime +0x7C
    union { u8 unk94[0xA8 - 0x94]; entENTITY *excl[5]; };   // +0x94 entities it passes through
    union { int unkA8; int nExcl; };    // +0xA8 0 after INIT
    entENTITY *owner;              // +0xAC the physPHYS (gets TERM when lifetime runs out)
    union { int unkB0; int nHits; };    // +0xB0 hits so far
    void ProcessCollision(m3dV *from, m3dV *to);      // v12 0x00509740 (feb 0x004F7090) sphere sweep vs scene, calls ProcessCDT
    void AddEntCDTExclude(entENTITY *e);              // feb 0x004F7C00
    void GetPos(m3dV *out);                           // feb 0x004F7C50
    void SetPos(m3dV *p);                             // feb 0x004F7C60
};
// size 0xB4 (verified: physPHYS::Create v12 0x00508A10 "push 0xB4")

class physDRIFT_MATH : public physMATH {  // vtable v12 0x005FEFB4; size 0xC0
public:
    m3dV drift;                                       // +0xB4
    physDRIFT_MATH() : physMATH() {}
    void CalcNewPos(m3dV *from, m3dV *to);            // v12 0x005096C0 (feb 0x004F7010)
    BOOL32 IsAtRest();                                // v12 0x00454850 (feb 0x0044C250)
};
// physDRIFT_MATH: physMATH + constant drift added to every step. vtable v12 0x005FEFB4 (feb 0x005D3D58),
//   CalcNewPos v12 0x005096C0 (feb 0x004F7010).  Extra field +0xB4 float drift[3] -> size 0xC0.

// physPHYS entity.  vtable v12 0x006079A8 (feb 0x005DC4F8)   factory physPHYS::Create v12 0x00508A10 (feb 0x004F63F0)
#pragma pack(push, 1)
class physPHYS : public entENTITY {                   // src/gs_shared/physics.cpp
public:
    physPHYS();                                                          // (inline in the factories)
    ~physPHYS();                                                         // v12 ? (feb 0x004F6620) deletes the math
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);      // +0x04 v12 0x00508C70 (feb 0x004F6650)
    void   ProcessMP_GET_SYNC_DATA(void *d);                             // +0x34 v12 0x00509230 (feb 0x004F6BB0)
    void   ProcessMP_WRITE_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d);       // +0x38 v12 0x005092D0 (feb 0x004F6C50)
    void   ProcessMP_READ_SYNC_DATA(struct gsMSG_MP_SAVELOAD *d);        // +0x3C v12 0x00509340 (feb 0x004F6CC0)
    virtual BOOL32 ProcessINIT(physINIT *p);                             // +0x64 v12 0x00508D30 (feb 0x004F66D0)
    virtual void   ProcessTERM();                                        // +0x68 v12 0x00508E90 (feb 0x004F6960)
    virtual void   ProcessFRAME();                                       // +0x6C v12 0x00508F50 (feb 0x004F6980)
    virtual void   ProcessDESTROY_ENT(entENTITY *e);                     // +0x70 v12 0x00509170 (feb 0x004F6AF0)
    virtual BOOL32 ProcessCDT(cdtINFO *ci);                              // +0x74 v12 0x00509190 (feb 0x004F6B10)
    virtual void   ProcessMP_NOTIFY(msgDATA *ev);                        // +0x78 v12 0x00509480 (feb 0x004F6DE0)
    void   AddEntCDTExclude(entENTITY *e);                               // v12 0x005091D0 (feb 0x004F6B50)
    static entENTITY *__fastcall Create(animINST *i);                    // v12 0x00508A10 (feb 0x004F63F0)
    physMATH *math;                // +0x16B v12 (feb +0x14F) verified v12 0x00508AF1
};
// size v12 0x16F (feb 0x153)
#pragma pack(pop)
BOOL32 __fastcall physInit();      // v12 0x005089E0 (feb 0x004F63C0) entRegisterClass("phys",'PHYS',0x28,physPHYS::Create,0,1); rgdInit()

// ------------------------------------------------------------------------------------------------
// Rigid bodies (phys_rigid*): full 6-DOF bodies with inertia tensor, friction points, support reaction,
// collision against a plane set built from the scene (rgdCDT_QUERY). Mostly unmatched in v12 by the
// automatic tables; feb addresses given, v12 only where verified by the match table "exact".
//   rgdInit v12 0x005234F0 (feb 0x0050E080)          rgdRIGID::Create feb 0x0050E0C0
//   rgdRIGID::ProcessINIT feb 0x0050E180  ProcessFRAME feb 0x0050EA80  ProcessPhysics feb 0x0050F1D0
//   ProcessPhysicSingleStep feb 0x0050F420  ComputeForces feb 0x0050F660  ApplyFriction feb 0x0050FDB0
//   CheckForCollisions feb 0x00510400  ResolveSingleCollision feb 0x00510D60  IntegrateSingleStep feb 0x00511210
//   CheckAtRest feb 0x00511560  CalcInertiaTensor feb 0x0050F0C0  CalcMass feb 0x0050F190
//   rgdRIGID_BALL (sphere): HullInit feb 0x00512300, CalcMass 0x00512330, CheckForCollisions 0x00512430
//   rgdRIGID_EXPLOS::Create feb 0x005122D0 (debris thrown by explosions)
//   rgdRIGID_START (spawner of pieces): StartExplos(objOBJ*) v12 0x00516580 (feb 0x005011F0),
//     CreatePiece v12 0x00516520 (feb 0x00501190), IsInclObj v12 0x00516620 (feb 0x00501290)
//   rgdPLANE_SET::AddPlane v12 0x00528980 (feb 0x005134E0), GetClosestPoint v12 0x005289B0
//   statics: rgdRIGID::isSimul feb 0x00956B1C, isCDTEdgeNo feb 0x00956B18, isCDTVertNo feb 0x00956F5C, cdtQuery feb 0x00956B28
// Gravity for rigid bodies and physMATH comes from the init data / constants in phys_rigid (not a global);
// player gravity is a separate immediate (results/player.json PHYS_GRAVITY v12 site 0x00421FE5 = 30.0).

// The INIT data of a rigid body ("nch_rigid" / rgdRIGID::ProcessINIT); as dstrDESTROY::StartExplos (v12 0x00554570)
// and dstrSHATTER_RIGID_START::CreatePiece fill it. The fields' meaning comes with rgdRIGID::ProcessINIT.
#pragma pack(push, 1)
struct rgdINIT_PARAMS : msgDATA {  // size 0xB8 (the header's)
    int   unk04;                   // +0x04 8
    m3dV  vel;                     // +0x08 the speed it flies off with
    m3dV  angVel;                  // +0x14 the turn it flies off with
    u8    unk20[0x98 - 0x20];      // (left as it was on the stack)
    int   unk98;                   // +0x98 0
    float unk9C;                   // +0x9C 0.6
    float unkA0;                   // +0xA0 0.8
    float unkA4;                   // +0xA4 10 (a barrel's pieces 20)
    float unkA8;                   // +0xA8 0.3
    float unkAC;                   // +0xAC -1 (dyn_destroy's pieces 7)
    float unkB0;                   // +0xB0 -1
    float unkB4;                   // +0xB4 -1 (dyn_destroy's pieces 3)
};
#pragma pack(pop)
