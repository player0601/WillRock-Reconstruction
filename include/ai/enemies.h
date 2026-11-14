// enemies.h -- Will Rock 1.2 unofficial SDK, subsystem "ai": every enemy class
//
// One block per enemy class: registration name(s), factory, allocation size, vtable, animation sequence table,
// instance keys it reads, its virtual overrides, the constants its init code writes (= TWEAK points, with the v12
// address of the store) and the projectiles / effects it spawns.  The class bodies list the class-own fields from
// db/classdb.json (feb layout) with their v12 offset: "// ? offset by shift" means the v12 offset was extrapolated
// from the nearest verified store; all other offsets were matched store-by-store between the feb and v12 code
// (work/sdk_ai/stores.py). Field names f_XXX are unnamed (feb offset).
//
// Shared base: ai_npc.h (aiNPC / aiNPC_WR / aiNPC_WR_FLY). Assumed from re/sdk/core: entENTITY, objOBJ, m3dV ...
// Every enemy is registered with entRegisterClass(name, AI_SIG_NPC '$NPC', type 0x32, Create, group, 0) in its
// aiXxxInit() (called from wrAPP_SYSTEM::InitSubsys); perseus/sphinx use sig '$DST', type 0x28.
// Common keys for every enemy (section AI_PROP, read by aiNPC/aiNPC_WR, see README.md): radCDT radActivate
//   radActivateBack isSniper isPlrNoVisibility actorWayPoint navArea navBanCross size timeIdleAfterSpawn heightUpTester
//   Aiming ObstAvoid isFallback isWayPTurn isNoSFXAppear isAltarOpp.
//
// Field naming: health = entENTITY v12 +0xD0 (feb +0xCC), maxHealth = v12 +0xD4 (feb +0xD0).
// "@0x..." = v12 address of the instruction (or of the register load feeding it) that writes the value.
#pragma once
#include "ai_npc.h"
#pragma pack(push, 1)
class wrsfxRAT_SPARKS; class pteTEX2;
// [integrator] intermediate bases given a padded body so the subclasses below compile. The pad is the gap between
// the end of aiNPC_WR (v12 0x567) and the first field any subclass in this file declares: an UPPER bound, // ?
class aiGLAD_GENERIC : public aiNPC_WR {  // shared gladiator code (src/Sources/ai_glad_generic.cpp), no vtable of its own
public:
    u32        gladFlags;             // v12 +0x567 (feb +0x53F) 2 may attack, 4 slash, 8 hit reaction, 0x10/0x20 hit
                                      //   (light / heavy), 0x40 fat, 0x80 death sound, 0x200 appearing (spawned)
    float      throwTimer;            // v12 +0x56B (feb +0x543) till it may throw (counted down; 1.2: Slot67 < 2 s)
    int        throwsLeft;            // v12 +0x56F (feb +0x547) throws before closing in (2 thin / 3 fat, +2/+1 spawned)
    int        hitSeq;                // v12 +0x573 (feb +0x54B) the hit reaction being played (knife: 5)
    int        hitLight;              // v12 +0x577 (feb +0x54F) the reaction was a light one
    sncCHANNEL *voice;                // v12 +0x57B (feb +0x553)
    float      minThrowDist;          // v12 +0x57F (feb +0x557) 5
    objOBJ    *attackFocus;           // v12 +0x583 (feb +0x55B) "AttackFocus"
    u8         _587[0x5BB - 0x587];   // v12 +0x587 // ?
    BOOL32 SetupAniSystem();                                    // v12 0x00448530 (feb 0x00440770)
    int  ProcessINIT(void *init);                               // v12 0x00448970 (feb 0x00440B70)
    void ParseNameClass();                                      // v12 0x00448BF0 (feb 0x00440E00)
    void FillAttackFlag();                                      // v12 0x00448CC0 (feb 0x00440ED0)
    void Think();                                               // v12 0x00448E80 (feb 0x00441090)
    void UpdateTimers();                                        // v12 0x00448F10 (1.2)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x00448F30 (feb 0x00441120)
    int  StartPassivePhysics(dmgDAMAGE *d);                     // v12 0x00448FC0 (feb 0x004411B0)
    void PlaySounds();                                          // v12 0x00448FE0 (feb 0x004411D0)
    void FillSoundFlags();                                      // v12 0x00449170 (feb 0x00441320; 1.2 took in the base ProcessActionFrame)
    int  Slot67();                                              // v12 0x004494E0 (1.2) an enemy seen and thrown at within 2 s
    virtual void LaunchOneProjectile() = 0;                     // [116] +0x1D0 (knife v12 0x0044A110, mace v12 0x0044BAF0)
};
sncSOUND_DESCR_3D *__fastcall aiGladGetSound(int i);           // v12 0x004494D0 (feb 0x004416A0)
int __fastcall aiGladiatorsAllInit();                          // v12 0x004484B0 (feb 0x004406F0)
class wrsfxSKELETON_DEATH;
// aiSKEL_RIGID_START : rgdRIGID_START  the model broken into flying pieces (rgdRIGID, not rebuilt: StartExplos is a
// PLACEHOLDER on the test side). A local of StartDeath; only what the skeleton fills.
struct aiSKEL_RIGID_START {
    wrsfxSKELETON_DEATH *sfx;           // +0x10 the dust and shards
    m3dV impulse;                       // +0x14 the pieces' speed
    void StartExplos(entENTITY *e);     // feb 0x004507D0
};
class aiSKEL_GENERIC : public aiNPC_WR {  // shared skeleton code (src/Sources/ai_skel_generic.cpp), no vtable of its own
public:
    u32         skelFlags;              // v12 +0x567 (feb +0x53F) see ai_skel_generic.cpp
    sncCHANNEL *voice;                  // v12 +0x56B (feb +0x543)
    BOOL32 SetupAniSystem();                    // v12 0x00458500 (feb 0x0044FAE0)
    int  ProcessINIT(void *init);               // v12 0x00458930 (feb 0x0044FF10)
    void ParseNameClass();                      // v12 0x00458B00 (feb 0x00450120)
    int  IsReadyActivate();                     // v12 0x00458C20 (feb 0x00450220)
    void GetBlueClothes();                      // v12 0x00458C40 (feb 0x00450240)
    int  Activate(int force);                   // v12 0x00458C80 (feb 0x00450280)
    void StartDeath(dmgDAMAGE *d);              // v12 0x00458D90 (feb 0x004503A0)
    virtual void StartDeath3(dmgDAMAGE *d, cdtINFO *ci, int shatter); // [116] +0x1D0 v12 0x00458DA0 (feb 0x004503C0)
    virtual int  IsDeadVoluntary() { return 0; }                      // [117] +0x1D4 v12 0x0048DF30
    int  SelectGoal();                          // v12 0x00459000 (feb 0x00450620)
    void MoveToGoal();                          // v12 0x00459080 (feb 0x004506A0)
    int  GetStateEntExcludeCDT();               // v12 0x004590A0 (feb 0x004506C0)
    void PlaySounds();                          // v12 0x00459410 (feb 0x00450A10)
    void UpdateDead();                          // v12 0x004595C0 (feb 0x00450BA0)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);     // v12 0x004595D0 (feb 0x00450BB0)
    int  Slot43();                              // v12 0x0045A960
    int  GetLayer2State();                      // v12 0x0045CD90
    void SetLayer2State(int s);                 // v12 0x0045A950
    int  IsGibbable() { return 0; }             // v12 0x0048DF30
    int  IsAnimSeqGoodForFallback() { return 0; }
};
int __fastcall aiSkeletonsAllInit();            // v12 0x00458490 (feb 0x0044FA70)
class aisSTATUE : public aiNPC_WR {       // statues (ais_statue.obj): atlas, diskobolus, perseus, sphinx. vtable feb 0x005D15F0
public: u8 _statue[0x5BF - 0x567];      // v12 +0x567..+0x5BE (feb +0x53F..+0x596) // ?
};


// ================================================================================================
// aiRAT : aiNPC_WR   registered as "ai_rat"
//   Small rat that runs at the player and self-destructs within 1.8 m (aiRAT::Think: StartDeath with a static 'special' damage of
//   5000 in 1.2 (10000 in feb) -> wrsfxRAT_EXPL). Sparks emitter while trotting. 1.2: IsGibbable vslot returns 0 (no gibbing).
//   Create v12 0x004553A0 (feb 0x0044CC50): operator new(0x583) (feb 0x55B); vtable v12 0x005FF040 (feb 0x005D3DC8), 116 slots (feb 113)
//   SetupAniSystem v12 0x004553F0 (feb 0x0044CCA0 aiRAT::SetupAniSystem): anim sequence slots in order:
//     0=RAT_SEQ_IDLE 1=RAT_SEQ_JUMP 2=RAT_SEQ_TROT 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x004553F0 SetupAniSystem | +0x07C 0x00455E20 FillSoundFlags
//     +0x080 0x00455CE0 PlaySounds | +0x088 0x00455A50 StartDeath | +0x098 0x00448E70 SelectGoal
//     +0x0A4 0x00455AA0 SetMovementSpeed | +0x0B0 0x00455C60 MoveToGoal | +0x0C8 0x004557D0 EvalAnimGoalSeq
//     +0x0CC 0x004558B0 GetAnimPlayRate | +0x0D0 0x00455860 ProcessActionFrame | +0x0EC 0x00455B60 Think
//     +0x0F4 0x00455CA0 UpdateTimers | +0x0F8 0x00455A80 Deactivate | +0x120 0x0045CD90 GetLayer3State*
//     +0x12C 0x0045A950 SetLayer3State* | +0x140 0x00455510 ProcessINIT | +0x14C 0x00455C20 ProcessFRAME
//     +0x150 0x00455DA0 ProcessCDT_FIREABLE | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x16C 0x00455DF0 Freeze
//     +0x198 0x00455F00 AdjustSpeedsOnFallbackStart
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x0045553E
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x00455544
//     ProcessINIT        maxHealth            = 2f         v12 +0x0D4 @0x00455538
//     ProcessINIT        radCDT               = 0.3f       v12 +0x187 @0x0045558E
//     ProcessINIT        heightLowTester      = 0.3f       v12 +0x18B @0x00455594
//     ProcessINIT        health               = 2f         v12 +0x0D0 @0x0045554A
//     ProcessINIT        navArriveDist        = 0.5f       v12 +0x1B3 @0x00455566
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x00455570
//     ProcessINIT        moveSpeed            = 6f         v12 +0x16F @0x0045557A
//     ProcessINIT        turnSpeed            = 70f        v12 +0x177 @0x00455584
//     ProcessINIT        heightCDT            = 0.6f       v12 +0x18F @0x0045559A
//     ProcessINIT        corpseTime           = 1f         v12 +0x4BB @0x004555A4
//     ProcessINIT        corpseFadeTime       = 1f         v12 +0x4BF @0x004555AE
//     ProcessINIT        shootRange           = 50f        v12 +0x1A7 @0x004555B8
//     ProcessINIT        aimAngle             = 20f        v12 +0x1A3 @0x004555C2
//     ProcessINIT        maxTurnStep          = 5f         v12 +0x17B @0x004555CC
//     ProcessINIT        fallbackAngle        = 10f        v12 +0x553 @0x004555D6
//     ProcessINIT        f_53F                = 0          v12 +0x567 @0x004555E0
//     ProcessINIT        f_547                = 0          v12 +0x56F @0x004555E6
//     ProcessINIT        f_54B                = 0          v12 +0x573 @0x004555EC
//     ProcessINIT        f_54F                = 0          v12 +0x577 @0x004555F2
//     ProcessINIT        bloodObj             = 0          v12 +0x54F @0x0045564E
//     ProcessINIT        f_543                = 0          v12 +0x56B @0x00455739
//     SetMovementSpeed   turnSpeed            = 0          v12 +0x177 @0x00455AB6
//     SetMovementSpeed   moveSpeed            = 2.22857f   v12 +0x16F @0x00455AD9
//     SetMovementSpeed   turnSpeed            = 70f        v12 +0x177 @0x00455AE9
//     SetMovementSpeed   moveSpeed            = 9.48936f   v12 +0x16F @0x00455B06
//     SetMovementSpeed   moveSpeed            = 1f         v12 +0x16F @0x00455B43
//     SetMovementSpeed   turnSpeed            = 120f       v12 +0x177 @0x00455B4D
//     Think  self-destruct distance 1.8 m: feb 0x0044D3E0 compares enemyDist with __real_3fe66666; v12 aiRAT::Think 0x00455B60
//     Think  self-destruct damage 5000.0 written @0x00455BF3 (v12) (feb 10000.0 @0x0044D473)
//   spawned entities:
//     "sfx_rat_expl" Create v12 0x004811A0 (feb 0x00476E20) size 0x17B (feb 0x15F) vt v12 0x00602F20 (feb 0x005D7B18)
//     "sfx_rat_sparks" Create v12 0x00481220 (feb 0x00476EA0) size 0x17F (feb 0x163) vt v12 0x00602F88 (feb 0x005D7B80)
// ================================================================================================
class aiRAT : public aiNPC_WR {
public:
    u32        ratFlags;              // v12 +0x567 (feb +0x53F) 1 stop the animation, 8 / 0x10 / 0x20 jump, 0x40 awake, 0x80 in the air, 0x100 blown up
    sncCHANNEL_CONE *voice;           // v12 +0x56B (feb +0x543)
    float      timerB;                // v12 +0x56F (feb +0x547)
    float      timerA;                // v12 +0x573 (feb +0x54B)
    float      sndTimer;              // v12 +0x577 (feb +0x54F) FillSoundFlags: until the next squeak
    objOBJ    *focusObj;              // v12 +0x57B (feb +0x553) "AttackFocus": where the blast starts
    struct wrsfxRAT_SPARKS *sparks;   // v12 +0x57F (feb +0x557)
    static entENTITY *__fastcall Create(animINST *i);            // v12 0x004553A0 (src/Sources/ai_rat.cpp)
    BOOL32 SetupAniSystem();                                     // v12 0x004553F0
    int  ProcessINIT(void *init);                                // v12 0x00455510
    int  EvalAnimGoalSeq();                                      // v12 0x004557D0
    void ProcessActionFrame();                                   // v12 0x00455860
    float GetAnimPlayRate();                                     // v12 0x004558B0
    void ExplodeDead();                                          // v12 0x004558D0
    void StartDeath(dmgDAMAGE *d);                               // v12 0x00455A50
    void Deactivate();                                           // v12 0x00455A80
    void SetMovementSpeed();                                     // v12 0x00455AA0
    void Think();                                                // v12 0x00455B60
    void ProcessFRAME();                                         // v12 0x00455C20
    void MoveToGoal();                                           // v12 0x00455C60
    void UpdateTimers();                                         // v12 0x00455CA0
    void PlaySounds();                                           // v12 0x00455CE0
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                      // v12 0x00455DA0
    void Freeze();                                               // v12 0x00455DF0
    void FillSoundFlags();                                       // v12 0x00455E20
    void AdjustSpeedsOnFallbackStart(dmgWEAPON *w, float *back, float *up); // v12 0x00455F00
    int  IsGibbable();                                           // 1.2: 0
    // size v12 0x583 (feb 0x55B)
};

// ================================================================================================
// aiSATYR : aiNPC_WR   registered as "ai_satyr"
//   Satyr archer: runs, keeps distance, shoots pjl_satyr_arrow (sniper/running goal-sequence variants).
//   feb IsArmoredObject made hits on 'bow_','cyl4','quiver','arrow_' objects harmless -- removed in 1.2.
//   Create v12 0x00457150 (feb 0x0044E800): operator new(0x583) (feb 0x557); vtable v12 0x005FF268 (feb 0x005D3FA8), 118 slots (feb 115)
//   SetupAniSystem v12 0x00456280 (feb 0x0044D970 aiSATYR::SetupAniSystem): anim sequence slots in order:
//     0=SATYR_SEQ_IDLE 1=SATYR_SEQ_SHOOT_ARROW 2=SATYR_SEQ_WOUNDED 3=SATYR_SEQ_RUN 4=SATYR_SEQ_DEATH1 
//     5=SATYR_SEQ_DEATH2 6=SATYR_SEQ_APPEAR 
//   class-specific instance keys (section key -> v12 code address):
//     AI_PROP isNoSFXAppear  @0x00456722
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00456280 SetupAniSystem | +0x064 0x004567A0 ParseNameClass
//     +0x07C 0x00456FA0 FillSoundFlags | +0x080 0x00457080 PlaySounds | +0x088 0x004568A0 StartDeath
//     +0x098 0x00456940 SelectGoal | +0x0A4 0x004564A0 SetMovementSpeed | +0x0B0 0x0043F8E0 MoveToGoal*
//     +0x0C8 0x00456CE0 EvalAnimGoalSeq | +0x0D0 0x00456D50 ProcessActionFrame | +0x0F4 0x00456F70 UpdateTimers*
//     +0x124 0x0045CD90 GetLayer3State* | +0x130 0x0045A950 SetLayer3State* | +0x140 0x00456500 ProcessINIT
//     +0x144 0x0043B090 ProcessTERM* | +0x150 0x00456880 ProcessCDT_FIREABLE | +0x15C 0x00430A10 ProcessMP_NOTIFY*
//     +0x194 0x004571A0 IsAnimSeqGoodForFallback | +0x1A0 0x004568B0 Explode | +0x1C4 0x00403200 IsArmoredObject
//     +0x1D0 0x00456CB0 EvalSniperGoalSeq | +0x1D4 0x00456CC0 EvalRunningGoalSeq
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     SetMovementSpeed   moveSpeed            = 15f        v12 +0x16F @0x004564A7
//     SetMovementSpeed   turnSpeed            = 360f       v12 +0x177 @0x004564C2
//     SetMovementSpeed   turnSpeed            = 180f       v12 +0x177 @0x004564D4
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x00456551
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x00456557
//     ProcessINIT        radCDT               = 0.7f       v12 +0x187 @0x00456562
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x0045656C
//     ProcessINIT        aimAngle             = 180f       v12 +0x1A3 @0x00456572
//     ProcessINIT        heightLowTester      = 0.84f      v12 +0x18B @0x00456587
//     ProcessINIT        fallbackAngle        = 50f        v12 +0x553 @0x00456591
//     ProcessINIT        maxTurnStep          = 70f        v12 +0x17B @0x0045659B
//     ProcessINIT        shootRange           = 120f       v12 +0x1A7 @0x004565A5
//     ProcessINIT        f_547                = 0          v12 +0x56F @0x004565BF
//     ProcessINIT        f_54B                = 0          v12 +0x573 @0x004565C5
//     ProcessINIT        maxHealth            = 7f         v12 +0x0D4 @0x004565CB
//     ProcessINIT        health               = 7f         v12 +0x0D0 @0x004565D1
//     ProcessINIT        f_543                = 0          v12 +0x56B @0x004566FA
//     ProcessINIT        bloodObj             = 0          v12 +0x54F @0x00456759
//     ProcessINIT        heightCDT            = 1.4f       v12 +0x18F @0x0045657D   1.2 NEW
//     ProcessINIT        heightUp1            = 2.3f       v12 +0x3A3 @0x004565AF   1.2 NEW
//     ParseNameClass     f_553                = 0          v12 +0x57F @0x004567AE
//     ParseNameClass     unk37F               = 40f        v12 +0x39F @0x004567C1
//     ParseNameClass     f_54F                = 5          v12 +0x577 @0x004567D1
//     ParseNameClass     f_54F                = 5          v12 +0x577 @0x004567FF
//     ParseNameClass     f_553                = 0          v12 +0x57F @0x00456813
//     ParseNameClass     f_54F                = 5          v12 +0x577 @0x00456819
//   spawned entities:
//     "pjl_satyr_arrow" Create v12 0x00457F70 (feb 0x0044F6E0) size 0x208 (feb 0x1EC) vt v12 0x005FF480 (feb 0x005D41B0)
//     "sfx_satir_arrow_land" Create v12 0x004819C0 (feb 0x00477640) size 0x17B (feb 0x15F) vt v12 0x00603050 (feb 0x005D7C50)
// ================================================================================================
class aiSATYR : public aiNPC_WR {   // src/Sources/ai_satyr.cpp
public:
    u32        satyrFlags;            // v12 +0x567 (feb +0x53F) 0x10 wounded (hit), 0x1000000 appearing (spawned)
    sncCHANNEL *voice;                // v12 +0x56B (feb +0x543) sncCHANNEL_SPH 20..150
    float      shootTimer;            // v12 +0x56F (feb +0x547) after a volley: 0.1 s before the next may start
    float      moveTimer;             // v12 +0x573 (feb +0x54B) a runner keeps its goal this long (3 s)
    int        arrowsLeft;            // v12 +0x577 (feb +0x54F) in this volley (5)
    int        deathSeq;              // v12 +0x57B (1.2) DEATH1 or DEATH2, chosen at birth
    class aiAREA *area;               // v12 +0x57F (feb +0x553) its spawn domain's area: a runner runs about in it
    // size v12 0x583 (feb 0x557)

    static entENTITY *__fastcall Create(animINST *i);           // v12 0x00457150 (feb 0x0044E800)
    BOOL32 SetupAniSystem();                                    // v12 0x00456280 (feb 0x0044D970)
    void SetMovementSpeed();                                    // v12 0x004564A0
    int  ProcessINIT(void *init);                               // v12 0x00456500
    void ParseNameClass();                                      // v12 0x004567A0
    void Think();                                               // v12 0x00456850
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x00456880
    void StartDeath(dmgDAMAGE *d);                              // v12 0x004568A0
    void Explode(dmgDAMAGE *d, int dead);                       // v12 0x004568B0
    int  SelectGoal();                                          // v12 0x00456940
    int  EvalAnimGoalSeq();                                     // v12 0x00456CE0
    void ProcessActionFrame();                                  // v12 0x00456D50
    void LaunchOneProjectile();                                 // v12 0x00456D90
    void UpdateTimers();                                        // v12 0x00456F70
    void FillSoundFlags();                                      // v12 0x00456FA0
    void PlaySounds();                                          // v12 0x00457080
    int  IsAnimSeqGoodForFallback();                            // v12 0x004571A0
    void GetShootArrowPhysParam(m3dV *from, struct pjlPJL_INIT *init); // v12 0x00457B50
    virtual int  EvalSniperGoalSeq();                           // [116] +0x1D0 v12 0x00456CB0
    virtual int  EvalRunningGoalSeq();                          // [117] +0x1D4 v12 0x00456CC0
};
int __fastcall aiSatyrInit();                                   // v12 0x00456220
int __fastcall pjlSatyrArrowInit();                             // v12 0x004571B0

// ================================================================================================
// aiSKEL_SPEAR : aiSKEL_GENERIC   registered as "ai_skel"
//   Skeleton warrior (ai_skel): spear melee + spear throw (pjl_spear_skel), shatters into pieces on death (aiSKEL_RIGID_START).
//   Shared skeleton code: aiSKEL_GENERIC (ProcessINIT v12 0x00458930 / feb 0x0044FF10, StartDeath v12 0x00458DA0 / feb 0x004503C0).
//   Create v12 0x004595E0 (feb 0x00450BC0): operator new(0x59F) (feb 0x577); vtable v12 0x005FF598 (feb 0x005D4280), 118 slots (feb 115)
//   SetupAniSystem v12 0x00458500 (feb 0x0044FAE0 aiSKEL_GENERIC::SetupAniSystem): anim sequence slots in order:
//     0=SKEL_SPEAR_SEQ_RUN_SPEAR 1=SKEL_SPEAR_SEQ_THROW_MOVE 2=SKEL_SPEAR_SEQ_THROW_STAY 3=SKEL_SPEAR_SEQ_IDLE_EMPTY 
//     4=SKEL_SPEAR_SEQ_SWING_A 5=SKEL_SPEAR_SEQ_SWING_B 6=SKEL_SPEAR_SEQ_SWING_SPEAR 7=SKEL_SPEAR_SEQ_APPEAR_WEAPON 
//     8=SKEL_SPEAR_SEQ_RUN_SWORD 9=SKEL_SPEAR_SEQ_IDLE_SWORD 10=SKEL_SPEAR_SEQ_IDLE_SPEAR 
//     11=SKEL_SPEAR_SEQ_APPEAR_SP_SKEL 12=SKEL_SPEAR_SEQ_APPEAR_SW_SKEL 13=SKEL_SWORD_SEQ_SW_RUN 
//     14=SKEL_SWORD_SEQ_SW_ATTACK 15=SKEL_SWORD_SEQ_SW_IDLE 16=SKEL_SWORD_SEQ_SW_APPEAR 
//     17=SKEL_SPEAR_SEQ_STAND_SPEAR 18=SKEL_SPEAR_SEQ_STAND_REG_SWORD 19=SKEL_SPEAR_SEQ_STAND_KAMIKAZE 
//     20=SKEL_SPEAR_SEQ_STAB 
//   class-specific instance keys (section key -> v12 code address):
//     ANIM_PROP scale  @0x004596E5
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00458500 SetupAniSystem* | +0x064 0x00459790 ParseNameClass
//     +0x074 0x004590A0 GetStateEntExcludeCDT* | +0x07C 0x0045A2D0 FillSoundFlags | +0x080 0x00459410 PlaySounds*
//     +0x084 0x0045A880 UpdateSFX | +0x088 0x0045C8C0 StartDeath* | +0x08C 0x004595C0 UpdateDead*
//     +0x098 0x0045A7F0 SelectGoal | +0x0A4 0x0045A670 SetMovementSpeed | +0x0B0 0x0045A800 MoveToGoal
//     +0x0C8 0x00459B30 EvalAnimGoalSeq | +0x0D0 0x00459DE0 ProcessActionFrame | +0x0E0 0x0043B900 GetPlayerTargetPos*
//     +0x0E4 0x00458C20 IsReadyActivate* | +0x0EC 0x0045A5B0 Think | +0x0F4 0x0045A830 UpdateTimers
//     +0x120 0x0045CD90 GetLayer3State* | +0x124 0x0045C600 GetLayer3State* | +0x12C 0x0045A950 SetLayer3State*
//     +0x130 0x0045C610 SetLayer3State* | +0x140 0x00459630 ProcessINIT | +0x144 0x0043B090 ProcessTERM*
//     +0x148 0x004598B0 ProcessINIT_LEVEL | +0x150 0x004595D0 ProcessCDT_FIREABLE* | +0x15C 0x00430A10 ProcessMP_NOTIFY*
//     +0x178 0x0045A8C0 Activate | +0x1C4 0x00403200 IsArmoredObject* | +0x1D0 0x00458DA0 StartDeath*
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        f_547                = 0          v12 +0x56F @0x00459645
//     ProcessINIT        nav                  = 15f        v12 +0x1AB @0x004596AB
//     ProcessINIT        f_56B                = 15f        v12 +0x593 @0x004596B1
//     ProcessINIT        turnSpeed            = 90f        v12 +0x177 @0x004596B7
//     ProcessINIT        f_56F                = 90f        v12 +0x597 @0x004596BD
//     ProcessINIT        f_54B                = 20f        v12 +0x573 @0x004596D3
//     ProcessINIT        maxTurnStep          = 20f        v12 +0x17B @0x004596D9
//     ProcessINIT        maxHealth            = 2f         v12 +0x0D4 @0x004596F1
//     ProcessINIT        navArriveDist        = 1.6f       v12 +0x1B3 @0x004596FB
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x00459705
//     ProcessINIT        moveSpeed            = 13f        v12 +0x16F @0x0045970F
//     ProcessINIT        health               = 2f         v12 +0x0D0 @0x00459719
//     ProcessINIT        f_54F                = 0          v12 +0x577 @0x00459723
//     ProcessINIT        f_557                = 0          v12 +0x57F @0x00459729
//     ProcessINIT        f_553                = 0          v12 +0x57B @0x0045972F
//     ParseNameClass     nav                  = 0.1f       v12 +0x1AB @0x00459853
//     ParseNameClass     f_56B                = 0          v12 +0x593 @0x0045985F
//     ParseNameClass     f_56F                = 150f       v12 +0x597 @0x0045986F
//     SetMovementSpeed   turnSpeed            = 90f        v12 +0x177 @0x0045A684
//     SetMovementSpeed   turnSpeed            = 120f       v12 +0x177 @0x0045A6BA
//     SetMovementSpeed   turnSpeed            = 90f        v12 +0x177 @0x0045A6E9
//     SetMovementSpeed   turnSpeed            = 90f        v12 +0x177 @0x0045A77E
//     SetMovementSpeed   moveSpeed            = 13f        v12 +0x16F @0x0045A788
//     SetMovementSpeed   turnSpeed            = 225f       v12 +0x177 @0x0045A797
//     SetMovementSpeed   moveSpeed            = 6.5f       v12 +0x16F @0x0045A7A1
//     SetMovementSpeed   turnSpeed            = 90f        v12 +0x177 @0x0045A7C5
//     SetMovementSpeed   turnSpeed            = 0          v12 +0x177 @0x0045A7D4
//   spawned entities:
//     "pjl_spear_skel" Create v12 0x0045A9D0 (feb 0x00451F50) size 0x208 (feb 0x1EC) vt v12 0x005FF780 (feb 0x005D4458)
//     "sfx_skel_spear_appear" Create v12 0x004835A0 (feb 0x00479210) size 0x18F (feb 0x173) vt v12 0x00603230 (feb 0x005D7E30)
//     "sfx_skel_spear_land" Create v12 0x00483650 (feb 0x004792C0) size 0x17F (feb 0x163) vt v12 0x00603298 (feb 0x005D7E98)
//     "sfx_skel_death" Create v12 0x004836D0 (feb 0x00479340) size 0x1AF (feb 0x193) vt v12 0x006031C8 (feb 0x005D7DC8)
// ================================================================================================
class wrsfxSKELETON_SPEAR_APPEAR;
class aiSKEL_SPEAR : public aiSKEL_GENERIC {
public:
    u32        weaponFlags;           // v12 +0x56F (feb +0x547) see src/Sources/ai_skel_spear.cpp
    float      throwAngle;            // v12 +0x573 (feb +0x54B) 20
    float      timer577;              // v12 +0x577 (feb +0x54F) runs from 10 again and again
    float      throwTimer;            // v12 +0x57B (feb +0x553)
    float      weaponTimer;           // v12 +0x57F (feb +0x557)
    objOBJ    *curWeapon;             // v12 +0x583 (feb +0x55B)
    objOBJ    *spearObj;              // v12 +0x587 (feb +0x55F)
    objOBJ    *swordObj;              // v12 +0x58B (feb +0x563)
    int        lastStab;              // v12 +0x58F (feb +0x567)
    float      throwMin;              // v12 +0x593 (feb +0x56B) 15
    float      throwMax;              // v12 +0x597 (feb +0x56F) 90
    wrsfxSKELETON_SPEAR_APPEAR *appearSfx; // v12 +0x59B (feb +0x573)
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x004595E0 (feb 0x00450BC0)
    int  ProcessINIT(void *init);       // v12 0x00459630 (feb 0x00450C10)
    void ParseNameClass();              // v12 0x00459790 (feb 0x00450D70)
    void ProcessINIT_LEVEL();           // v12 0x004598B0 (feb 0x00450E90)
    void ShowWeapon();
    void SetupWeaponOnINIT();           // v12 0x004598D0 (feb 0x00450EB0)
    int  EvalSwordAnimGoalSeq();        // v12 0x004599B0 (feb 0x00450F90)
    int  EvalSpearAnimGoalSeq();        // v12 0x00459A70 (feb 0x00451050)
    int  EvalAnimGoalSeq();             // v12 0x00459B30 (feb 0x00451110)
    void SetupWeaponByDistance();       // v12 0x00459C40 (feb 0x00451220)
    void ProcessActionFrame();          // v12 0x00459DE0 (feb 0x004513D0)
    void FillSoundFlags();              // v12 0x0045A2D0 (feb 0x004518C0)
    void FillAttackFlag();              // v12 0x0045A420 (feb 0x00451A10)
    void Think();                       // v12 0x0045A5B0 (feb 0x00451BC0)
    void SetMovementSpeed();            // v12 0x0045A670 (feb 0x00451C20)
    int  SelectGoal();                  // v12 0x0045A7F0 (feb 0x00451DA0)
    void MoveToGoal();                  // v12 0x0045A800 (feb 0x00451DB0)
    void UpdateTimers();                // v12 0x0045A830 (feb 0x00451DE0)
    void Slot33();                      // v12 0x0045A880 (feb 0x00451E30) UpdateSFX
    int  Activate(int force);           // v12 0x0045A8C0 (feb 0x00451E70)
    void LaunchOneProjectile();         // v12 0x0045B520 (feb 0x00452AD0) src/Sources/ai_skel_spear_pjl.cpp
    void AimSpearToPlayer(m3dV *org, struct pjlPJL_INIT *in);  // v12 0x0045B2C0 (feb 0x00452890)
    // size v12 0x59F (feb 0x577)
};

// ================================================================================================
// aiSKEL_SWORD : aiSKEL_GENERIC   registered as "ai_skel_kamik"
//   Kamikaze sword skeleton (ai_skel_kamik): charges and collides (ClassifyCollisionTarget / ProcessCDT).
//   Create v12 0x0045BC90 (feb 0x00452F80): operator new(0x577) (feb 0x54F); vtable v12 0x005FF908 (feb 0x005D45C8), 119 slots (feb 116)
//   SetupAniSystem v12 0x00458500 (feb 0x0044FAE0 aiSKEL_GENERIC::SetupAniSystem): anim sequence slots in order:
//     0=SKEL_SPEAR_SEQ_RUN_SPEAR 1=SKEL_SPEAR_SEQ_THROW_MOVE 2=SKEL_SPEAR_SEQ_THROW_STAY 3=SKEL_SPEAR_SEQ_IDLE_EMPTY 
//     4=SKEL_SPEAR_SEQ_SWING_A 5=SKEL_SPEAR_SEQ_SWING_B 6=SKEL_SPEAR_SEQ_SWING_SPEAR 7=SKEL_SPEAR_SEQ_APPEAR_WEAPON 
//     8=SKEL_SPEAR_SEQ_RUN_SWORD 9=SKEL_SPEAR_SEQ_IDLE_SWORD 10=SKEL_SPEAR_SEQ_IDLE_SPEAR 
//     11=SKEL_SPEAR_SEQ_APPEAR_SP_SKEL 12=SKEL_SPEAR_SEQ_APPEAR_SW_SKEL 13=SKEL_SWORD_SEQ_SW_RUN 
//     14=SKEL_SWORD_SEQ_SW_ATTACK 15=SKEL_SWORD_SEQ_SW_IDLE 16=SKEL_SWORD_SEQ_SW_APPEAR 
//     17=SKEL_SPEAR_SEQ_STAND_SPEAR 18=SKEL_SPEAR_SEQ_STAND_REG_SWORD 19=SKEL_SPEAR_SEQ_STAND_KAMIKAZE 
//     20=SKEL_SPEAR_SEQ_STAB 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00458500 SetupAniSystem* | +0x064 0x00458B20 ParseNameClass*
//     +0x074 0x004590A0 GetStateEntExcludeCDT* | +0x07C 0x0045BFE0 FillSoundFlags | +0x080 0x00459410 PlaySounds*
//     +0x088 0x0045C8C0 StartDeath* | +0x08C 0x004595C0 UpdateDead* | +0x098 0x0045C160 SelectGoal
//     +0x0B0 0x0045C190 MoveToGoal | +0x0C8 0x0045BF80 EvalAnimGoalSeq | +0x0D0 0x0045BE10 ProcessActionFrame
//     +0x0E0 0x0043B900 GetPlayerTargetPos* | +0x0E4 0x00458C20 IsReadyActivate* | +0x0EC 0x0045C080 Think
//     +0x120 0x0045CD90 GetLayer3State* | +0x124 0x0045C600 GetLayer3State* | +0x12C 0x0045A950 SetLayer3State*
//     +0x130 0x0045C610 SetLayer3State* | +0x140 0x0045BCE0 ProcessINIT | +0x144 0x0043B090 ProcessTERM*
//     +0x148 0x0045BDD0 ProcessINIT_LEVEL | +0x14C 0x0045BDE0 ProcessFRAME | +0x150 0x004595D0 ProcessCDT_FIREABLE*
//     +0x154 0x0045C5A0 ProcessCDT | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x178 0x00458C80 Activate*
//     +0x1C4 0x00403200 IsArmoredObject | +0x1D0 0x00458DA0 StartDeath* | +0x1D4 0x0045C5D0 IsDeadVoluntary
//     +0x1D8 0x0045C1D0 ClassifyCollisionTarget
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        f_547                = 0          v12 +0x56F @0x0045BCE8
//     ProcessINIT        maxHealth            = 2f         v12 +0x0D4 @0x0045BD01
//     ProcessINIT        health               = 2f         v12 +0x0D0 @0x0045BD0C
//     ProcessINIT        navArriveDist        = 20f        v12 +0x1B3 @0x0045BD18
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x0045BD1E
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x0045BD24
//     ProcessINIT        maxTurnStep          = 20f        v12 +0x17B @0x0045BD2A
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x0045BD41
//     ProcessINIT        moveSpeed            = 11f        v12 +0x16F @0x0045BD47
//     ProcessINIT        turnSpeed            = 180f       v12 +0x177 @0x0045BD51
//   spawned entities:
//     "sfx_skel_death" Create v12 0x004836D0 (feb 0x00479340) size 0x1AF (feb 0x193) vt v12 0x006031C8 (feb 0x005D7DC8)
// ================================================================================================
class aiSKEL_SWORD : public aiSKEL_GENERIC {
public:
    u32        swordFlags;            // v12 +0x56F (feb +0x547) see src/Sources/ai_skel_sword.cpp
    objOBJ    *swordObj;              // v12 +0x573 (feb +0x54B)
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x0045BC90 (feb 0x00452F80)
    int  ProcessINIT(void *init);       // v12 0x0045BCE0 (feb 0x00452FD0)
    void ProcessINIT_LEVEL();           // v12 0x0045BDE0 (feb 0x004530C0)
    int  Strike();
    void ProcessFRAME();                // v12 0x0045BE00 (feb 0x004530D0)
    void ProcessActionFrame();          // v12 0x0045BFE0 (feb 0x004533F0)
    int  EvalAnimGoalSeq();             // v12 0x0045C0A0 (feb 0x00453560)
    void FillSoundFlags();              // v12 0x0045C0F0 (feb 0x004535C0)
    void Think();                       // (feb 0x00453660)
    int  SelectGoal();                  // v12 0x0045C160 (feb 0x00453740)
    void MoveToGoal();                  // (feb 0x00453770)
    virtual int ClassifyCollisionTarget(cdtINFO *ci); // [118] +0x1D8 v12 0x0045C1D0 (feb 0x004537B0)
    void ProcessCDT(cdtINFO *ci);       // v12 0x0045C5A0 (feb 0x00453810)
    int  IsDeadVoluntary();             // v12 0x0045C5D0 (feb 0x00453B40)
    // size v12 0x577 (feb 0x54F)
};

// ================================================================================================
// aiGLAD_KNIFE : aiGLAD_GENERIC   registered as "ai_glad_knife"
//   Gladiator with throwing knives (pjl_knife_glad). Shared code aiGLAD_GENERIC (ai_glad_generic.obj):
//   ProcessINIT v12 0x00448970 (feb 0x00440B70), SetupAniSystem feb 0x00440770.
//   Create v12 0x00449510 (feb 0x004416E0): operator new(0x5BB) (feb 0x593); vtable v12 0x005FE148 (feb 0x005D2FD8), 117 slots (feb 114)
//   SetupAniSystem v12 0x00448530 (feb 0x00440770 aiGLAD_GENERIC::SetupAniSystem): anim sequence slots in order:
//     0=GLAD_SEQ_IDLE_KNIFE 1=GLAD_SEQ_RUN_KNIFE 2=GLAD_SEQ_APPEAR_KNIFE 3=GLAD_SEQ_THROW_KNIFE 
//     4=GLAD_SEQ_SLASH_KNIFE 5=GLAD_SEQ_SHOT_KNIFE 6=GLAD_SEQ_DEATH_KNIFE 7=GLAD_SEQ_IDLE_FAT_KNIFE 
//     8=GLAD_SEQ_RUN_FAT_KNIFE 9=GLAD_SEQ_APPEAR_FAT_KNIFE 10=GLAD_SEQ_THROW_FAT_KNIFE 11=GLAD_SEQ_SLASH_FAT_KNIFE 
//     12=GLAD_SEQ_SHOT_FAT_KNIFE 13=GLAD_SEQ_DEATH_FAT_KNIFE 14=GLAD_SEQ_IDLE_MACE 15=GLAD_SEQ_RUN_MACE 
//     16=GLAD_SEQ_APPEAR_MACE 17=GLAD_SEQ_THROW_MACE 18=GLAD_SEQ_SLASH1_MACE 19=GLAD_SEQ_SHOT_MACE 
//     20=GLAD_SEQ_DEATH_MACE 21=GLAD_SEQ_SLASH2_MACE 22=GLAD_SEQ_IDLE_FAT_MACE 23=GLAD_SEQ_RUN_FAT_MACE 
//     24=GLAD_SEQ_APPEAR_FAT_MACE 25=GLAD_SEQ_THROW_FAT_MACE 26=GLAD_SEQ_SLASH1_FAT_MACE 27=GLAD_SEQ_SHOT_FAT_MACE 
//     28=GLAD_SEQ_DEATH_FAT_MACE 29=GLAD_SEQ_SLASH2_FAT_MACE 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00448530 SetupAniSystem* | +0x064 0x00448BF0 ParseNameClass*
//     +0x07C 0x00449170 FillSoundFlags* | +0x080 0x00448FE0 PlaySounds* | +0x098 0x00448E70 SelectGoal*
//     +0x0B0 0x0043F8E0 MoveToGoal* | +0x0C8 0x004495D0 EvalAnimGoalSeq | +0x0D0 0x0044A2D0 ProcessActionFrame
//     +0x0EC 0x00448E80 Think* | +0x0F4 0x00448F10 UpdateTimers* | +0x124 0x0045CD90 GetLayer3State*
//     +0x130 0x0045A950 SetLayer3State* | +0x140 0x00449560 ProcessINIT | +0x148 0x004495A0 ProcessINIT_LEVEL
//     +0x150 0x00448F30 ProcessCDT_FIREABLE* | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x188 0x00448FC0 StartPassivePhysics*
//     +0x194 0x00548680 IsAnimSeqGoodForFallback* | +0x1C4 0x00403200 IsArmoredObject* | +0x1D0 0x0044A110 LaunchOneProjectile
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        fallbackAngle        = 90f        v12 +0x553 @0x00449575
//     ProcessINIT        f_54B                = 5          v12 +0x573 @0x0044957F
//   spawned entities:
//     "pjl_knife_glad" Create v12 0x00449710 (feb 0x004418E0) size 0x1DC (feb 0x1C0) vt v12 0x005FE320 (feb 0x005D31A0)
// ================================================================================================
class aiGLAD_KNIFE : public aiGLAD_GENERIC {             // src/Sources/ai_glad_knife.cpp
public:
    // size v12 0x5BB (feb 0x593)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x00449510 (feb 0x004416E0)
    int  ProcessINIT(void *init);                               // v12 0x00449560 (feb 0x00441730)
    void ProcessINIT_LEVEL();                                   // v12 0x004495A0 (feb 0x00441770)
    int  EvalAnimGoalSeq();                                     // v12 0x004495D0 (feb 0x004417A0)
    void AimKnifeToPlayer(m3dV *org, struct pjlPJL_INIT *in);          // v12 0x00449EB0 (feb 0x00442170)
    void LaunchOneProjectile();                                 // v12 0x0044A110 (feb 0x004423D0)
    void ProcessActionFrame();                                  // v12 0x0044A2D0 (feb 0x00442580)
};

// ================================================================================================
// aiGLAD_MACE : aiGLAD_GENERIC   registered as "ai_glad_mace"
//   Gladiator with a flaming flail/mace thrown as pjl_mace_glad (explodes: sfx_glad_mace_expl). Appears with a glow emitter.
//   Create v12 0x0044A9D0 (feb 0x00442B10): operator new(0x5CB) (feb 0x5A3); vtable v12 0x005FE478 (feb 0x005D32D8), 117 slots (feb 114)
//   SetupAniSystem v12 0x00448530 (feb 0x00440770 aiGLAD_GENERIC::SetupAniSystem): anim sequence slots in order:
//     0=GLAD_SEQ_IDLE_KNIFE 1=GLAD_SEQ_RUN_KNIFE 2=GLAD_SEQ_APPEAR_KNIFE 3=GLAD_SEQ_THROW_KNIFE 
//     4=GLAD_SEQ_SLASH_KNIFE 5=GLAD_SEQ_SHOT_KNIFE 6=GLAD_SEQ_DEATH_KNIFE 7=GLAD_SEQ_IDLE_FAT_KNIFE 
//     8=GLAD_SEQ_RUN_FAT_KNIFE 9=GLAD_SEQ_APPEAR_FAT_KNIFE 10=GLAD_SEQ_THROW_FAT_KNIFE 11=GLAD_SEQ_SLASH_FAT_KNIFE 
//     12=GLAD_SEQ_SHOT_FAT_KNIFE 13=GLAD_SEQ_DEATH_FAT_KNIFE 14=GLAD_SEQ_IDLE_MACE 15=GLAD_SEQ_RUN_MACE 
//     16=GLAD_SEQ_APPEAR_MACE 17=GLAD_SEQ_THROW_MACE 18=GLAD_SEQ_SLASH1_MACE 19=GLAD_SEQ_SHOT_MACE 
//     20=GLAD_SEQ_DEATH_MACE 21=GLAD_SEQ_SLASH2_MACE 22=GLAD_SEQ_IDLE_FAT_MACE 23=GLAD_SEQ_RUN_FAT_MACE 
//     24=GLAD_SEQ_APPEAR_FAT_MACE 25=GLAD_SEQ_THROW_FAT_MACE 26=GLAD_SEQ_SLASH1_FAT_MACE 27=GLAD_SEQ_SHOT_FAT_MACE 
//     28=GLAD_SEQ_DEATH_FAT_MACE 29=GLAD_SEQ_SLASH2_FAT_MACE 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00448530 SetupAniSystem* | +0x064 0x00448BF0 ParseNameClass*
//     +0x07C 0x00449170 FillSoundFlags* | +0x080 0x00448FE0 PlaySounds* | +0x084 0x0044BCB0 UpdateSFX
//     +0x098 0x00448E70 SelectGoal* | +0x0B0 0x0043F8E0 MoveToGoal* | +0x0C8 0x0044ABF0 EvalAnimGoalSeq
//     +0x0D0 0x0044BF70 ProcessActionFrame | +0x0EC 0x00448E80 Think* | +0x0F4 0x00448F10 UpdateTimers*
//     +0x124 0x0045CD90 GetLayer3State* | +0x130 0x0045A950 SetLayer3State* | +0x140 0x0044AA30 ProcessINIT
//     +0x144 0x0044ABB0 ProcessTERM | +0x14C 0x0044BD30 ProcessFRAME | +0x150 0x00448F30 ProcessCDT_FIREABLE*
//     +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x188 0x00448FC0 StartPassivePhysics* | +0x194 0x00548680 IsAnimSeqGoodForFallback*
//     +0x1C4 0x00403200 IsArmoredObject* | +0x1D0 0x0044BAF0 LaunchOneProjectile
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        f_593                = 0          v12 +0x5BB @0x0044AA6E
//     ProcessINIT        f_59B                = 0          v12 +0x5C3 @0x0044AAA0
//     ProcessINIT        maxHealth            = 12f        v12 +0x0D4 @0x0044AB62
//     ProcessINIT        navArriveDist        = 2f         v12 +0x1B3 @0x0044AB6C
//     ProcessINIT        fallbackAngle        = 100f       v12 +0x553 @0x0044AB76
//     ProcessINIT        f_54B                = 0x13       v12 +0x573 @0x0044AB80
//     ProcessINIT        radCDT               = 1f         v12 +0x187 @0x0044AB8A   1.2 NEW
//     ProcessINIT        heightCDT            = 1.7f       v12 +0x18F @0x0044AB94   1.2 NEW
//   spawned entities:
//     "pjl_mace_glad" Create v12 0x0044AF70 (feb 0x004430A0) size 0x1E0 (feb 0x1C4) vt v12 0x005FE680 (feb 0x005D34D0)
//     "sfx_glad_mace_expl" Create v12 0x004770E0 (feb 0x0046D400) size 0x17F (feb 0x163) vt v12 0x00602150 (feb 0x005D6D40)
// ================================================================================================
class aiGLAD_MACE : public aiGLAD_GENERIC {              // src/Sources/ai_glad_mace.cpp
public:
    partEMITTER_PHYS *glowL;          // v12 +0x5BB (feb +0x593) aiGLAD_MACE_APPEAR_GLOW
    objOBJ    *emitterL;              // v12 +0x5BF (feb +0x597) "emitter_left"
    partEMITTER_PHYS *glowR;          // v12 +0x5C3 (feb +0x59B)
    objOBJ    *emitterR;              // v12 +0x5C7 (feb +0x59F) "emitter_right"
    // size v12 0x5CB (feb 0x5A3)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x0044A9D0 (feb 0x00442B10)
    int  ProcessINIT(void *init);                               // v12 0x0044AA30 (feb 0x00442B70)
    void ProcessTERM();                                         // v12 0x0044ABB0 (feb 0x00442CE0)
    int  EvalAnimGoalSeq();                                     // v12 0x0044ABF0 (feb 0x00442D20)
    void AimMaceToPlayer(m3dV *org, struct pjlPJL_INIT *in);    // v12 0x0044B890 (feb 0x00443AC0)
    void LaunchOneProjectile();                                 // v12 0x0044BAF0 (feb 0x00443D20)
    void Slot33();                                              // v12 0x0044BCB0 (feb UpdateSFX 0x00443ED0)
    void ProcessFRAME();                                        // v12 0x0044BD30 (feb 0x00443F50)
    void ProcessActionFrame();                                  // v12 0x0044BF70 (feb 0x00444190)
};

// ================================================================================================
// aiMINO : aiNPC_WR   registered as "ai_mino"
//   Minotaur: runs, slashes with the axe (SEKIRA), throws mino_ax, can start as a statue (AI_PROP isStatue / '_statue'),
//   red variant (AI_MINO isRed / '_gen2'), regenerating (AI_MINO isRegen) with a chrome regen effect. 1.2 merged ParseNameClass into ProcessINIT.
//   Create v12 0x00452750 (feb 0x0044A1A0): operator new(0x59F) (feb 0x573); vtable v12 0x005FEBF0 (feb 0x005D39A8), 118 slots (feb 115)
//   SetupAniSystem v12 0x00450410 (feb 0x00447FB0 aiMINO::SetupAniSystem): anim sequence slots in order:
//     0=MINO_SEQ_UNUSED 1=MINO_SEQ_APPEAR 2=MINO_SEQ_RUN 3=MINO_SEQ_THROW_AX 4=MINO_SEQ_SLASH 5=MINO_SEQ_SLASH_2 
//     6=MINO_SEQ_SLASH_3 7=MINO_SEQ_SLASH_4 8=MINO_SEQ_DEATH 9=MINO_SEQ_DEATH_FALLBACK 10=MINO_SEQ_STATUE2LIFE 
//     11=MINO_SEQ_REGENERATION 12=MINO_SEQ_HIT_FALLBACK 13=MINO_SEQ_HIT_FALLBACK_2 14=MINO_SEQ_IDLE 
//   class-specific instance keys (section key -> v12 code address):
//     AI_MINO isRed  @0x00450BC0
//     AI_MINO isRegen  @0x00450D73
//     AI_PROP isStatue  @0x00450C56
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00450410 SetupAniSystem | +0x064 0x00450B80 ParseNameClass
//     +0x06C 0x00452240 SettleSurface | +0x07C 0x00452290 FillSoundFlags | +0x080 0x004523F0 PlaySounds
//     +0x084 0x004655D0 UpdateSFX | +0x088 0x00451050 StartDeath | +0x08C 0x00451180 UpdateDead*
//     +0x098 0x00451420 SelectGoal | +0x0A4 0x004526F0 SetMovementSpeed | +0x0B0 0x00451190 MoveToGoal
//     +0x0C8 0x00451710 EvalAnimGoalSeq | +0x0CC 0x004516F0 GetAnimPlayRate | +0x0D0 0x00451830 ProcessActionFrame
//     +0x0DC 0x00451B50 GetWpnTip* | +0x0E0 0x0043B900 GetPlayerTargetPos* | +0x0E4 0x004526D0 IsReadyActivate
//     +0x0EC 0x004518D0 Think | +0x0F4 0x00451D90 UpdateTimers* | +0x124 0x0045CD90 GetLayer3State
//     +0x130 0x0045A950 SetLayer3State* | +0x140 0x00450900 ProcessINIT | +0x144 0x0043B090 ProcessTERM
//     +0x14C 0x00450E00 ProcessFRAME | +0x150 0x00460030 ProcessCDT_FIREABLE | +0x154 0x00450E70 ProcessCDT
//     +0x158 0x00450F50 ProcessASK_KEEP_LEVEL_RELOAD | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x164 0x00450E40 ProcessDESTROY_ENT
//     +0x178 0x00451670 Activate | +0x194 0x00450EF0 IsAnimSeqGoodForFallback | +0x198 0x00450F20 AdjustSpeedsOnFallbackStart
//     +0x1A0 0x004527E0 Explode | +0x1A4 0x004651B0 InitSFX | +0x1AC 0x00465580 TermSFX
//     +0x1B0 0x004527D0 DoesNPCAllowSFXAppear | +0x1C4 0x00403200 IsArmoredObject | +0x1D0 0x00451160 SelectDeathAnim
//     +0x1D4 0x00452080 ThrowAx
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        turnSpeed            = 90f        v12 +0x177 @0x00450916
//     ProcessINIT        fallbackAngle        = 90f        v12 +0x553 @0x0045091C
//     ProcessINIT        radActivate          = 90f        v12 +0x4CB @0x00450922
//     ProcessINIT        radCDT               = 0.9f       v12 +0x187 @0x00450934   CHANGED, feb 0.7f
//     ProcessINIT        heightLowTester      = 1.75f      v12 +0x18B @0x0045093E
//     ProcessINIT        heightCDT            = 1.3f       v12 +0x18F @0x00450948   CHANGED, feb 1.1f
//     ProcessINIT        moveSpeed            = 11f        v12 +0x16F @0x00450952
//     ProcessINIT        minTurnRadius        = 7.7f       v12 +0x173 @0x0045095C
//     ProcessINIT        maxTurnStep          = 45f        v12 +0x17B @0x00450966
//     ProcessINIT        aimAngle             = 45f        v12 +0x1A3 @0x0045096C
//     ProcessINIT        radActivateBack      = 20f        v12 +0x4CF @0x00450972
//     ProcessINIT        navArriveDist        = 2f         v12 +0x1B3 @0x0045097C
//     ProcessINIT        f_547                = 1f         v12 +0x573 @0x0045098C
//     ProcessINIT        f_56F                = 0          v12 +0x59B @0x00450996
//     ProcessINIT        nav                  = 10f        v12 +0x1AB @0x0045099C
//     ProcessINIT        f_543                = 0          v12 +0x56B @0x00450A35
//     ProcessINIT        maxHealth            = 15f        v12 +0x0D4 @0x00450A5C
//     ProcessINIT        health               = 15f        v12 +0x0D0 @0x00450A62
//     ProcessINIT        f_54B                = 0          v12 +0x577 @0x00450B4E
//     ParseNameClass     maxHealth            = 7f         v12 +0x0D4 @0x00450CEB
//     ParseNameClass     health               = 7f         v12 +0x0D0 @0x00450CF1
//     ParseNameClass     alertAngle           = 360f       v12 +0x19B @0x00450D07
//     ParseNameClass     shootRange           = 300f       v12 +0x1A7 @0x00450D11
//     ParseNameClass     radActivate          = 12f        v12 +0x4CB @0x00450D44
//     ParseNameClass     radActivateBack      = 12f        v12 +0x4CF @0x00450D4A
//     ParseNameClass     fallbackAngle        = 80f        v12 +0x553 @0x00450DCA
//   spawned entities:
//     "mino_ax" Create v12 0x00464CB0 (feb 0x0045B3D0) size 0x1BC (feb 0x1A0) vt v12 0x006006C8 (feb 0x005D52A8)
// ================================================================================================
class aiMINO : public aiNPC_WR {   // src/Sources/ai_mino.cpp (minoFlags bits: see there)
public:
    u32        minoFlags;             // v12 +0x567 (feb +0x53F) GetLayer3State / SetLayer3State
    float      axTimer;               // v12 +0x56B (feb +0x543) till the next axe throw
    float      roarTimer;             // v12 +0x56F (1.2 new) every 2..3 s: minoFlags 0x20000 at random (Slot67)
    float      animRate;              // v12 +0x573 (feb +0x547) 0.9..1.25 while regrowing
    sncCHANNEL *voice;                // v12 +0x577 (feb +0x54B)
    entENTITY *axAppear;              // v12 +0x57B (feb +0x54F) "sfx_ax_appear" (the axe's flash on the slash)
    class objMOD_TEX_FRAME_ANIM *axAnim[2]; // v12 +0x57F/+0x583 its two planes' frame animations
    float      axAppearTimer;         // v12 +0x587 0.8 s shown
    class pteTEX2 *stoneTex;          // v12 +0x58B the statue's stone (blended away by STATUE2LIFE)
    class pteTEX2 *regenTex;          // v12 +0x58F a regrown one's chrome -> red
    u32        f_593;                 // v12 +0x593 (zeroed by Create)
    entENTITY *f_597;                 // v12 +0x597 kept over a level reload with axAppear // ?
    int        axCount;               // v12 +0x59B axes left to throw
    // size v12 0x59F (feb 0x573)

    static entENTITY *__fastcall Create(animINST *i);           // v12 0x00452750 (feb 0x0044A1A0)
    static aiNPC *__fastcall Regenerate(physPHYS *chunk);       // v12 0x00452510 (feb 0x0044A090)
    static void __fastcall ChunkCreateNotify(physPHYS *chunk);  // (feb 0x0045BD70, mino_sfx.obj)
    BOOL32 SetupAniSystem();                                    // v12 0x00450410 (feb 0x00447FB0)
    int  ProcessINIT(void *init);                               // v12 0x00450900 (feb 0x004484A0)
    void ParseNameClass();                                      // v12 0x00450B80 (feb 0x004487A0)
    void ProcessFRAME();                                        // v12 0x00450E00 (feb 0x00448A30)
    void ProcessDESTROY_ENT(entENTITY *e);                      // v12 0x00450E40 (feb 0x00448A70)
    void ProcessCDT(cdtINFO *ci);                               // v12 0x00450E70 (feb 0x00448AA0)
    int  IsAnimSeqGoodForFallback();                            // v12 0x00450EF0 (feb 0x00448B20)
    void AdjustSpeedsOnFallbackStart(dmgWEAPON *w, float *back, float *up); // v12 0x00450F20 (feb 0x00448B50)
    int  IsArmoredObject(char *name);                           // v12 0x00403200 (1.2: the shared return 0)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x00460030 (feb 0x00448BA0)
    int  ProcessASK_KEEP_LEVEL_RELOAD();                        // v12 0x00450F50 (feb 0x00448BD0)
    void Explode3(dmgDAMAGE *d, int pieces, int kind);          // v12 0x00450F80 (feb 0x00448C00) Explode(d, int, int)
    void StartDeath(dmgDAMAGE *d);                              // v12 0x00451050 (feb 0x00448CD0)
    void MoveToGoal();                                          // v12 0x00451190 (feb 0x00448E00)
    int  SelectGoal();                                          // v12 0x00451420 (feb 0x00449090)
    int  Activate(int force);                                   // v12 0x00451670 (feb 0x004492D0)
    float GetAnimPlayRate();                                    // v12 0x004516F0 (feb 0x00449350)
    int  EvalAnimGoalSeq();                                     // v12 0x00451710 (feb 0x00449370)
    void ProcessActionFrame();                                  // v12 0x00451830 (feb 0x00449490)
    void Think();                                               // v12 0x004518D0 (feb 0x00449530)
    void GetWpnTip(m3dV *out);                                  // v12 0x00451B50
    void SingleProcessSlash(int seq, int want);                 // v12 0x00451B80 (feb 0x004497B0)
    void UpdateTimers();                                        // v12 0x00451D90 (feb 0x004499D0)
    void SettleSurface();                                       // v12 0x00452240 (feb 0x00449DF0)
    void FillSoundFlags();                                      // v12 0x00452290 (feb 0x00449E40)
    void PlaySounds();                                          // v12 0x004523F0 (feb 0x00449F70)
    int  IsReadyActivate();                                     // v12 0x004526D0 (feb 0x0044A150)
    void SetMovementSpeed();                                    // v12 0x004526F0 (feb 0x0044A170)
    int  Slot67();                                              // v12 0x00452720 (1.2 new)
    int  DoesNPCAllowSFXAppear();                               // v12 0x004527D0 (feb 0x0044A220)
    void Explode(dmgDAMAGE *d, int dead);                       // v12 0x004527E0 (feb 0x0044A230)
    void GetPlayerTargetPos(m3dV *out);                         // v12 0x0043B900 (shared)
    int  GetLayer3State();                                      // v12 0x0045CD90 (shared)
    void SetLayer3State(int s);                                 // v12 0x0045A950 (shared)
    int  InitSFX();                                             // v12 0x004651B0 (feb 0x0045B7A0, mino_sfx.obj)
    void TermSFX();                                             // v12 0x00465580 (feb 0x0045BB80)
    void Slot33();                                              // v12 0x004655D0 (feb 0x0045BBD0) UpdateSFX
    virtual int  SelectDeathAnim();                             // [116] +0x1D0 v12 0x00451160 (feb 0x00448DE0)
    virtual void ThrowAx();                                     // [117] +0x1D4 v12 0x00452080 (feb 0x00449C30)
};
int __fastcall aiMinoInit();                                    // (feb 0x00447F40)
int __fastcall minoSFXInit();                                   // v12 0x004650D0 (feb 0x0045B6C0)

// ================================================================================================
// aiMINO_ATRIUM : aiMINO   registered as "ai_mino_atrium"
//   Minotaur variant that stays inside an arena stage (aiaDOMAIN_LIST_ARENA::GetNPCStageNmb).
//   Create v12 0x00452800 (feb 0x0044A250): operator new(0x5A3) (feb 0x577); vtable v12 0x005FEDC8 (feb 0x005D3B78), 118 slots (feb 115)
//   SetupAniSystem v12 0x00450410 (feb 0x00447FB0 aiMINO::SetupAniSystem): anim sequence slots in order:
//     0=MINO_SEQ_UNUSED 1=MINO_SEQ_APPEAR 2=MINO_SEQ_RUN 3=MINO_SEQ_THROW_AX 4=MINO_SEQ_SLASH 5=MINO_SEQ_SLASH_2 
//     6=MINO_SEQ_SLASH_3 7=MINO_SEQ_SLASH_4 8=MINO_SEQ_DEATH 9=MINO_SEQ_DEATH_FALLBACK 10=MINO_SEQ_STATUE2LIFE 
//     11=MINO_SEQ_REGENERATION 12=MINO_SEQ_HIT_FALLBACK 13=MINO_SEQ_HIT_FALLBACK_2 14=MINO_SEQ_IDLE 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00450410 SetupAniSystem* | +0x064 0x00450B80 ParseNameClass*
//     +0x06C 0x00452240 SettleSurface* | +0x07C 0x00452290 FillSoundFlags* | +0x080 0x004523F0 PlaySounds*
//     +0x084 0x004655D0 UpdateSFX* | +0x088 0x00451050 StartDeath* | +0x08C 0x00451180 UpdateDead*
//     +0x098 0x00451420 SelectGoal* | +0x0A4 0x004526F0 SetMovementSpeed* | +0x0B0 0x00452980 MoveToGoal
//     +0x0C8 0x00451710 EvalAnimGoalSeq* | +0x0CC 0x004516F0 GetAnimPlayRate* | +0x0D0 0x00451830 ProcessActionFrame*
//     +0x0DC 0x00451B50 GetWpnTip* | +0x0E0 0x0043B900 GetPlayerTargetPos* | +0x0E4 0x004526D0 IsReadyActivate*
//     +0x0EC 0x004518D0 Think* | +0x0F4 0x00451D90 UpdateTimers* | +0x124 0x0045CD90 GetLayer3State*
//     +0x130 0x0045A950 SetLayer3State* | +0x140 0x00452880 ProcessINIT | +0x144 0x0043B090 ProcessTERM*
//     +0x14C 0x00450E00 ProcessFRAME* | +0x150 0x00460030 ProcessCDT_FIREABLE* | +0x154 0x00450E70 ProcessCDT*
//     +0x158 0x00450F50 ProcessASK_KEEP_LEVEL_RELOAD* | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x164 0x00450E40 ProcessDESTROY_ENT*
//     +0x178 0x00452930 Activate | +0x188 0x00452B20 StartPassivePhysics | +0x194 0x00450EF0 IsAnimSeqGoodForFallback*
//     +0x198 0x00450F20 AdjustSpeedsOnFallbackStart* | +0x1A0 0x004527E0 Explode* | +0x1A4 0x004651B0 InitSFX*
//     +0x1AC 0x00465580 TermSFX* | +0x1B0 0x004527D0 DoesNPCAllowSFXAppear* | +0x1C4 0x00403200 IsArmoredObject*
//     +0x1D0 0x00452AE0 SelectDeathAnim | +0x1D4 0x00452960 ThrowAx
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        radActivateBack      = 150f       v12 +0x4CF @0x004528CD
//     ProcessINIT        radActivate          = 150f       v12 +0x4CB @0x004528D3
//     ProcessINIT        shootRange           = 300f       v12 +0x1A7 @0x004528F9
//     ProcessINIT        f_573                = 0          v12 +0x59F @0x0045290D
//     ProcessINIT        shootRange           = 300f       v12 +0x1A7 @0x00452917
// ================================================================================================
class aiMINO_ATRIUM : public aiMINO {                  // src/Sources/ai_mino.cpp
public:
    int        axesLeft;              // v12 +0x59F (feb +0x573) axes it may throw from its stage (2..5) while the player
                                      //   is not in the arena
    // size v12 0x5A3 (feb 0x577)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x00452800 (feb 0x0044A250)
    int  ProcessINIT(void *init);                               // v12 0x00452880 (feb 0x0044A2D0)
    int  Activate(int force);                                   // v12 0x00452930 (feb 0x0044A380)
    void ThrowAx();                                             // v12 0x00452960 (feb 0x0044A3B0)
    void MoveToGoal();                                          // v12 0x00452980 (feb 0x0044A3D0)
    int  SelectDeathAnim();                                     // v12 0x00452AE0 (feb 0x0044A520) exact
    int  StartPassivePhysics(dmgDAMAGE *d);                     // v12 0x00452B20 (feb 0x0044A560)
};

// ================================================================================================
// aiCENT : aiNPC_WR   registered as "ai_cent"
//   Centaur: gallops, melee, hooks the player ('plr_hook'), throws pjl_trident. Explodes into gibs (Explode override).
//   Create v12 0x0043AA00 (feb 0x00434530): operator new(0x5BB) (feb 0x593); vtable v12 0x005FCF98 (feb 0x005D2050), 116 slots (feb 113)
//   SetupAniSystem v12 0x0043AA50 (feb 0x00434580 aiCENT::SetupAniSystem): anim sequence slots in order:
//     0=CENT_SEQ_UNUSED 1=CENT_SEQ_APPEAR 2=CENT_SEQ_APPEAR2RUN 3=CENT_SEQ_RUN 4=CENT_SEQ_ATTACK 
//     5=CENT_SEQ_BEING_HIT 6=CENT_SEQ_DEATH 7=CENT_SEQ_ATTACK_HOOK 8=CENT_SEQ_ATTACK_THROW 9=CENT_SEQ_ATTACK_RUN 
//     10=CENT_SEQ_NEIGH 11=CENT_SEQ_RUN2THROW 12=CENT_SEQ_THROW 13=CENT_SEQ_THROW2RUN 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA | +0x040 0x0043AA50 SetupAniSystem | +0x064 0x0043F710 ParseNameClass*
//     +0x07C 0x0043B9D0 FillSoundFlags | +0x080 0x0043BAB0 PlaySounds | +0x088 0x0043B170 StartDeath
//     +0x08C 0x00451180 UpdateDead | +0x098 0x0043BBD0 SelectGoal | +0x0A4 0x0043B1B0 SetMovementSpeed
//     +0x0B0 0x0043B300 MoveToGoal | +0x0C8 0x0043B7B0 EvalAnimGoalSeq | +0x0D0 0x0043B820 ProcessActionFrame
//     +0x0DC 0x00451B50 GetWpnTip | +0x0E0 0x0043B900 GetPlayerTargetPos | +0x0EC 0x0043B760 Think
//     +0x0F4 0x0043B930 UpdateTimers | +0x124 0x0045CD90 GetLayer3State* | +0x130 0x0045A950 SetLayer3State
//     +0x140 0x0043AD60 ProcessINIT | +0x144 0x0043B090 ProcessTERM* | +0x150 0x0043B0A0 ProcessCDT_FIREABLE
//     +0x158 0x0043B150 ProcessASK_KEEP_LEVEL_RELOAD | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x184 0x0043BF90 ApplySpeedTrampoline
//     +0x194 0x0043C480 IsAnimSeqGoodForFallback | +0x198 0x0043BFD0 AdjustSpeedsOnFallbackStart | +0x1A0 0x0043B0C0 Explode
//     +0x1C0 0x0043BBA0 SelectGoalToRunAway | +0x1C4 0x00403200 IsArmoredObject
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        radActivate          = 200f       v12 +0x4CB @0x0043AD6D
//     ProcessINIT        radActivateBack      = 200f       v12 +0x4CF @0x0043AD73
//     ProcessINIT        radCDT               = 0.7f       v12 +0x187 @0x0043AD7D
//     ProcessINIT        feb only: feb +0x173 = 1.1f (removed in 1.2)
//     ProcessINIT        moveSpeed            = 20f        v12 +0x16F @0x0043ADA6
//     ProcessINIT        minTurnRadius        = 14f        v12 +0x173 @0x0043ADB0
//     ProcessINIT        navArriveDist        = 1f         v12 +0x1B3 @0x0043ADBA
//     ProcessINIT        turnSpeed            = 120f       v12 +0x177 @0x0043ADC4
//     ProcessINIT        navUnk1A7            = 40f        v12 +0x1C7 @0x0043ADCE
//     ProcessINIT        maxTurnStep          = 30f        v12 +0x17B @0x0043ADD8
//     ProcessINIT        f_543                = 0          v12 +0x56B @0x0043AE0C
//     ProcessINIT        f_54B                = 0          v12 +0x573 @0x0043AE12
//     ProcessINIT        f_53F                = 0          v12 +0x567 @0x0043AE18
//     ProcessINIT        maxHealth            = 70f        v12 +0x0D4 @0x0043AE1E
//     ProcessINIT        health               = 70f        v12 +0x0D0 @0x0043AE24
//     ProcessINIT        fallbackAngle        = 150f       v12 +0x553 @0x0043AE2A
//     ProcessINIT        f_547                = 0          v12 +0x56F @0x0043AE34
//     ProcessINIT        f_54F                = 0          v12 +0x577 @0x0043AE3A
//     ProcessINIT        f_587                = 0          v12 +0x5AF @0x0043AE40
//     ProcessINIT        f_58B                = 0          v12 +0x5B3 @0x0043AF26
//     ProcessINIT        f_57F                = 50f        v12 +0x5A7 @0x0043AF91
//     ProcessINIT        aimAngle             = 20f        v12 +0x1A3 @0x0043AF9B
//     ProcessINIT        f_583                = 150f       v12 +0x5AB @0x0043AFA5
//     ProcessINIT        heightCDT            = 1.4f       v12 +0x18F @0x0043AD88   1.2 NEW
//     ProcessINIT        unk17B               = 0.9f       v12 +0x197 @0x0043AD92   1.2 NEW
//     ProcessINIT        heightUp1            = 2f         v12 +0x3A3 @0x0043AD9C   1.2 NEW
//     SetMovementSpeed   turnSpeed            = 270f       v12 +0x177 @0x0043B1CE
//     SetMovementSpeed   turnSpeed            = 40f        v12 +0x177 @0x0043B1DF
//     SetMovementSpeed   turnSpeed            = 110f       v12 +0x177 @0x0043B1EB
//     SetMovementSpeed   turnSpeed            = 0          v12 +0x177 @0x0043B1F7
//   spawned entities:
//     "pjl_trident" Create v12 0x0045CCD0 (feb 0x00454160) size 0x208 (feb 0x1EC) vt v12 0x005FFCF0 (feb 0x005D4978)
// ================================================================================================
class aiCENT : public aiNPC_WR {                        // src/Sources/ai_cent.cpp
public:
    u32        centFlags;             // v12 +0x567 (feb +0x53F) 1 charging (ATTACK), 2 the hook out (ATTACK's frames 0..1),
                                      //   4 appearing, 8 a straight run at the enemy, 0x10 a collision centre found,
                                      //   0x20 neighing, 0x800 a skeleton (ai_sketavr), 0x1000 throwing (the trident)
    float      attackPause;           // v12 +0x56B (feb +0x543) 5 s after a charge
    float      runTimer;              // v12 +0x56F (feb +0x547) till a new run goal (3, 4, 6 s; 1 after fleeing)
    float      trampleTimer;          // v12 +0x573 (feb +0x54B) 1.2 / 1.5 s after it struck or threw
    float      neighTimer;            // v12 +0x577 (feb +0x54F) time since the last neigh (> 9 s: NEIGH)
    m3dV       colisCenter;           // v12 +0x57B (feb +0x553) "cent_colis_center"'s position
    m3dV       hookPrev2;             // v12 +0x587 (feb +0x55F) (copied to hookPrev when a charge starts; never set)
    m3dV       hookPrev;              // v12 +0x593 (feb +0x56B) the hook's last place (the swept hook test)
    objOBJ    *hookObj;               // v12 +0x59F (feb +0x577) "plr_hook" (or "ATTACKfocus")
    objOBJ    *throwObj;              // v12 +0x5A3 (feb +0x57B) where the trident leaves (the hook)
    float      minThrowDist;          // v12 +0x5A7 (feb +0x57F) 50 (with wrFlags 0x20000; else 10)
    float      maxThrowDist;          // v12 +0x5AB (feb +0x583) 150
    float      throwTimer;            // v12 +0x5AF (feb +0x587) 4..8 s between tridents
    sncCHANNEL *voice;                // v12 +0x5B3 (feb +0x58B) a sphere, 20..100
    entENTITY *dust;                  // v12 +0x5B7 (feb +0x58F) "sfx_dust" following it
    // size v12 0x5BB (feb 0x593)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x0043AA00 (feb 0x00434530)
    BOOL32 SetupAniSystem();                                    // v12 0x0043AA50 (feb 0x00434580)
    int  ProcessINIT(void *init);                               // v12 0x0043AD60 (feb 0x00434890)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x0043B0A0 (feb 0x00434BC0)
    void Explode(dmgDAMAGE *d, int dead);                       // v12 0x0043B0C0 (feb 0x00434BE0)
    int  ProcessASK_KEEP_LEVEL_RELOAD();                        // v12 0x0043B150 (feb 0x00434C70)
    void StartDeath(dmgDAMAGE *d);                              // v12 0x0043B170 (feb 0x00434C90)
    void SetMovementSpeed();                                    // v12 0x0043B1B0 (feb 0x00434CE0)
    void MoveToGoal();                                          // v12 0x0043B300 (feb 0x00434E30)
    void Think();                                               // v12 0x0043B760 (feb 0x00435290)
    int  EvalAnimGoalSeq();                                     // v12 0x0043B7B0 (feb 0x004352E0)
    void ProcessActionFrame();                                  // v12 0x0043B820 (feb 0x00435350)
    void GetWpnTip(m3dV *out);                                  // v12 0x00451B50 (shared)
    void GetPlayerTargetPos(m3dV *out);                         // v12 0x0043B900 (shared)
    void UpdateTimers();                                        // v12 0x0043B930 (feb 0x00435460)
    void FillSoundFlags();                                      // v12 0x0043B9D0 (feb 0x00435500)
    void PlaySounds();                                          // v12 0x0043BAB0 (feb 0x004355E0)
    void SelectGoalToRunAway(float dist);                       // v12 0x0043BBA0 (feb 0x004356D0)
    int  SelectGoal();                                          // v12 0x0043BBD0 (feb 0x00435700)
    void StartRunStraight(int how);                             // v12 0x0043BC70 (feb 0x004357A0)
    void Throw();                                               // v12 0x0043BE70 (feb 0x004359A0)
    void GetHookPos(m3dV *out);                                 // v12 0x0043BF60 (feb 0x00435A90)
    void ApplySpeedTrampoline(m3dV *v);                         // v12 0x0043BF90 (feb 0x00435AC0) exact
    void AdjustSpeedsOnFallbackStart(dmgWEAPON *w, float *back, float *up); // v12 0x0043BFD0 (feb 0x00435B00)
    void GetShootPjlPhysParam(m3dV *org, physINIT *p);          // v12 0x0043C010 (feb 0x00435B80)
    void LaunchOneProjectile();                                 // v12 0x0043C250 (feb 0x00435DC0)
    int  CheckShootCond();                                      // v12 0x0043C3C0 (feb 0x00435F30)
    int  IsAnimSeqGoodForFallback();                            // v12 0x0043C480 (feb 0x00435FF0)
    int  GetLayer3State();                                      // v12 0x0045CD90 (shared)
    void SetLayer3State(int s);                                 // v12 0x0045A950 (shared)
};

// ================================================================================================
// aiSKETAVR : aiCENT   registered as "ai_sketavr"
//   Skeleton centaur (ai_sketavr): aiCENT behaviour with trident (pjl_trident) and skeleton death.
//   Create v12 0x0045C870 (feb 0x00453D50): operator new(0x5BB) (feb 0x593); vtable v12 0x005FFB18 (feb 0x005D47B0), 117 slots (feb 114)
//   SetupAniSystem v12 0x0043AA50 (feb 0x00434580 aiCENT::SetupAniSystem): anim sequence slots in order:
//     0=CENT_SEQ_UNUSED 1=CENT_SEQ_APPEAR 2=CENT_SEQ_APPEAR2RUN 3=CENT_SEQ_RUN 4=CENT_SEQ_ATTACK 
//     5=CENT_SEQ_BEING_HIT 6=CENT_SEQ_DEATH 7=CENT_SEQ_ATTACK_HOOK 8=CENT_SEQ_ATTACK_THROW 9=CENT_SEQ_ATTACK_RUN 
//     10=CENT_SEQ_NEIGH 11=CENT_SEQ_RUN2THROW 12=CENT_SEQ_THROW 13=CENT_SEQ_THROW2RUN 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x0045CC30 ProcessMP_GET_SYNC_DATA | +0x040 0x0043AA50 SetupAniSystem* | +0x064 0x0043F710 ParseNameClass*
//     +0x07C 0x0043B9D0 FillSoundFlags* | +0x080 0x0045CBA0 PlaySounds | +0x088 0x0045C8C0 StartDeath
//     +0x08C 0x00451180 UpdateDead* | +0x098 0x0043BBD0 SelectGoal* | +0x0A4 0x0043B1B0 SetMovementSpeed*
//     +0x0B0 0x0043B300 MoveToGoal* | +0x0C8 0x0043B7B0 EvalAnimGoalSeq* | +0x0D0 0x0043B820 ProcessActionFrame*
//     +0x0DC 0x00451B50 GetWpnTip* | +0x0E0 0x0043B900 GetPlayerTargetPos* | +0x0EC 0x0043B760 Think*
//     +0x0F4 0x0043B930 UpdateTimers* | +0x124 0x0045CD90 GetLayer3State* | +0x130 0x0045A950 SetLayer3State*
//     +0x140 0x0045C8E0 ProcessINIT | +0x144 0x0043B090 ProcessTERM* | +0x150 0x0043B0A0 ProcessCDT_FIREABLE*
//     +0x158 0x0043B150 ProcessASK_KEEP_LEVEL_RELOAD* | +0x15C 0x0045CC40 ProcessMP_NOTIFY | +0x184 0x0043BF90 ApplySpeedTrampoline*
//     +0x194 0x0043C480 IsAnimSeqGoodForFallback* | +0x198 0x0043BFD0 AdjustSpeedsOnFallbackStart* | +0x1A0 0x0043B0C0 Explode*
//     +0x1C0 0x0043BBA0 SelectGoalToRunAway* | +0x1C4 0x00403200 IsArmoredObject* | +0x1D0 0x0045C940 StartDeath
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        maxHealth            = 30f        v12 +0x0D4 @0x0045C900
//     ProcessINIT        health               = 30f        v12 +0x0D0 @0x0045C906
//     ProcessINIT        fallbackAngle        = 80f        v12 +0x553 @0x0045C917
//   spawned entities:
//     "pjl_trident" Create v12 0x0045CCD0 (feb 0x00454160) size 0x208 (feb 0x1EC) vt v12 0x005FFCF0 (feb 0x005D4978)
// ================================================================================================
class aiSKETAVR : public aiCENT {                       // src/Sources/ai_cent.cpp
public:
    // size v12 0x5BB (feb 0x593)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x0045C870 (feb 0x00453D50)
    int  ProcessINIT(void *init);                               // v12 0x0045C8E0 (feb 0x00453DA0)
    void StartDeath(dmgDAMAGE *d);                              // v12 0x0045C8C0 -> StartDeath(d, 0, 1)
    void StartDeath(dmgDAMAGE *d, cdtINFO *ci, int shatter);   // [116] +0x1D0 v12 0x0045C940 (feb 0x00453E00)
    void PlaySounds();                                          // v12 0x0045CBA0 (feb 0x00454050)
};
int __fastcall aiCentInit();                                    // v12 0x0043A6B0 (feb 0x004341D0) with the sketavr and the trident

// ================================================================================================
// aiTIGER : aiNPC_WR   registered as "ai_lion", "ai_tiger"
//   Tiger and lion (same class; lion = bit 0x8000 in f_53F flags, own sound list _aiLionSndList). Calm/angry idle, trot,
//   gallop, pounce (ATTACK_JUMP), left/right close bites. AI_TIGER isArena keeps it in the arena. Damage = aiTIGER::CalcDamageToTarget
//   (v12 0x0045E0C0 / feb 0x004551C0): reach factor f (1.5 within 1.1 m, falls to 0 at 1.7 m in 1.2; feb 1.0 m / 1.3 m), then
//   pounce 15*f (lion 20*f) or 2f / f+3, close bites 10*f (lion 13*f).
//   Tiger health 50 (ProcessINIT), lion 60 + radius field 130 (aiTIGER::ParseNameClass v12 0x0045D4B0, '_lion' sets bit 0x8000).
//   Create v12 0x0045D590 (feb 0x00454780): operator new(0x597) (feb 0x56B); vtable v12 0x005FFDD0 (feb 0x005D4A28), 116 slots (feb 113)
//   SetupAniSystem v12 0x0045D5E0 (feb 0x004547D0 aiTIGER::SetupAniSystem): anim sequence slots in order:
//     0=TIGER_SEQ_IDLE_CALM 1=TIGER_SEQ_IDLE_ROAR 2=TIGER_SEQ_IDLE_ANGRY 3=TIGER_SEQ_ATTACK_LCLOSE 4=TIGER_SEQ_TROT 
//     5=TIGER_SEQ_GALLOP 6=TIGER_SEQ_ATTACK_JUMP 7=TIGER_SEQ_SHOT_BY_BLAST 8=TIGER_SEQ_DEATH_BY_BLAST 
//     9=TIGER_SEQ_ATTACK_RCLOSE 10=TIGER_SEQ_GALL_TO_JUMP 11=TIGER_SEQ_JUMP_END_180 12=TIGER_SEQ_TURN_LEFT 
//     13=TIGER_SEQ_TURN_RIGHT 14=TIGER_SEQ_DEATH_STAY 15=TIGER_SEQ_SHOT_STAY 
//   class-specific instance keys (section key -> v12 code address):
//     AI_TIGER isArena  @0x0045DB35
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x0045D5E0 SetupAniSystem | +0x064 0x0045D4B0 ParseNameClass
//     +0x07C 0x0045F470 FillSoundFlags | +0x080 0x0045F310 PlaySounds | +0x098 0x0045EEA0 SelectGoal
//     +0x0A4 0x0045ECB0 SetMovementSpeed | +0x0B0 0x0045EF40 MoveToGoal | +0x0C8 0x0045DD90 EvalAnimGoalSeq
//     +0x0D0 0x0045E4E0 ProcessActionFrame | +0x0EC 0x0045E7E0 Think | +0x0F0 0x0045E9C0 PostThink
//     +0x0F4 0x0045F000 UpdateTimers | +0x120 0x0045CD90 GetLayer3State* | +0x12C 0x0045A950 SetLayer3State*
//     +0x140 0x0045DA70 ProcessINIT | +0x144 0x0043B090 ProcessTERM* | +0x150 0x0045F090 ProcessCDT_FIREABLE
//     +0x154 0x0045F280 ProcessCDT | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x194 0x0048DF30 IsAnimSeqGoodForFallback
//     +0x1C0 0x0045E4B0 SelectGoalToRunAway
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ParseNameClass     f_094                = 8          v12 +0x098 @0x0045D539
//     ParseNameClass     fallbackAngle        = 130f       v12 +0x553 @0x0045D546
//     ParseNameClass     maxHealth            = 60f        v12 +0x0D4 @0x0045D55B
//     ParseNameClass     health               = 60f        v12 +0x0D0 @0x0045D561
//     ParseNameClass     fallbackAngle        = 120f       v12 +0x553 @0x0045D569
//     ProcessINIT        maxHealth            = 50f        v12 +0x0D4 @0x0045DA95
//     ProcessINIT        navArriveDist        = 2.85f      v12 +0x1B3 @0x0045DA9B
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x0045DAA5
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x0045DAAB
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x0045DAB1
//     ProcessINIT        moveSpeed            = 3.5f       v12 +0x16F @0x0045DABB
//     ProcessINIT        turnSpeed            = 70f        v12 +0x177 @0x0045DAC5
//     ProcessINIT        slowAttackAngle      = 40f        v12 +0x58F @0x0045DACF   CHANGED, feb 30f
//     ProcessINIT        navUnk1A7            = 10f        v12 +0x1C7 @0x0045DAD9
//     ProcessINIT        health               = 50f        v12 +0x0D0 @0x0045DAE3
//     ProcessINIT        tigerFlags           = 0          v12 +0x567 @0x0045DAE9
//     ProcessINIT        curAttackSeq         = 0xE        v12 +0x56B @0x0045DAEF
//     ProcessINIT        attackSeq            = 3          v12 +0x56F @0x0045DAF9
//     ProcessINIT        f_54B                = 2          v12 +0x573 @0x0045DB03
//     ProcessINIT        f_54F                = 0          v12 +0x57B @0x0045DB47
//     ProcessINIT        f_553                = 0          v12 +0x57F @0x0045DB4D
//     ProcessINIT        f_557                = 0          v12 +0x583 @0x0045DB53
//     ProcessINIT        f_55B                = 0          v12 +0x587 @0x0045DB59
//     ProcessINIT        maxTurnStep          = 20f        v12 +0x17B @0x0045DB5F
//     ProcessINIT        corpseTime           = 20f        v12 +0x4BB @0x0045DB65
//     ProcessINIT        corpseFadeTime       = 17f        v12 +0x4BF @0x0045DB6B
//     ProcessINIT        f_55F                = 1          v12 +0x58B @0x0045DB75
//     ProcessINIT        tigerFlags           = 0x800      v12 +0x567 @0x0045DB7F
//     ProcessINIT        f_567                = 0          v12 +0x593 @0x0045DC85
//     ProcessINIT        bloodObj             = 0          v12 +0x54F @0x0045DCC8
//     ProcessINIT        new_v12_577          = 0          v12 +0x577 @0x0045DB41   1.2 NEW
//     SetMovementSpeed   moveSpeed            = 3.5f       v12 +0x16F @0x0045ED58
//     SetMovementSpeed   moveSpeed            = 16f        v12 +0x16F @0x0045ED67
//     SetMovementSpeed   moveSpeed            = 3.5f       v12 +0x16F @0x0045ED73
//     SetMovementSpeed   turnSpeed            = 70f        v12 +0x177 @0x0045EE0C
//     SetMovementSpeed   feb only: feb +0x15B = 270f (removed in 1.2)
//     SetMovementSpeed   feb only: feb +0x15B = 20f (removed in 1.2)
//     SetMovementSpeed   feb only: feb +0x15B = 0 (removed in 1.2)
//     SetMovementSpeed   feb only: feb +0x37F = 16f (removed in 1.2)
//     CalcDamageToTarget reach: full-damage distance 1.1 @0x0045E201 / 0x0045E355, zero at 1.7 @0x0045E221 / 0x0045E36E (feb 1.0 / 1.3)
//     lion maxHealth 60.0: register load before 0x0045D55B (aiTIGER::ParseNameClass)
// ================================================================================================
class aiTIGER : public aiNPC_WR {                       // src/Sources/ai_tiger.cpp
public:
    u32        tigerFlags;            // v12 +0x567 (feb +0x53F) 1 a light hit (SHOT_STAY), 2 a heavy one (SHOT_BY_BLAST),
                                      //   4 in a hit sequence, 8 the pounce bites now, 0x10 a close bite wanted, 0x20 a pounce
                                      //   wanted, 0x80 running off, 0x100 runs (16 m/s; "_arena" / AI_TIGER isArena),
                                      //   0x200 this attack has struck, 0x400 in the air, 0x800 "_sniper" (keeps its place),
                                      //   0x1000 turning, 0x2000 a turn wanted, 0x8000 a lion
    int        hitSeq;                // v12 +0x56B (feb +0x543) the hit / death sequence picked (init 0xE)
    int        attackSeq;             // v12 +0x56F (feb +0x547) the attack sequence (3 / 9 bites, 6 pounce, 11 the turn after)
    int        turnSeq;               // v12 +0x573 (feb +0x54B) TURN_LEFT (12) / TURN_RIGHT (13)
    float      attackPause;           // v12 +0x577 (feb +0x54F) 2 s after a pounce, 0.5 s after a bite
    float      timer57B;              // v12 +0x57B (feb +0x553) counts down, never set
    float      fleeTimer;             // v12 +0x57F (feb +0x557) 3 s of running off
    float      hitTimer;              // v12 +0x583 1.2 NEW: 0.2 s after its body struck the enemy (clears 0x200)
    float      turnTimer;             // v12 +0x587 (feb +0x55B) 0.5 s after a turn
    int        hitCanEnd;             // v12 +0x58B (feb +0x55F) the hit sequence was not a death one
    float      slowAttackAngle;       // v12 +0x58F (feb +0x563) 40 (feb 30): the bite cone
    sncCHANNEL *voice;                // v12 +0x593 (feb +0x567) a sphere
    // size v12 0x597 (feb 0x56B)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x0045D590 (feb 0x00454780)
    void ParseNameClass();                                      // v12 0x0045D4B0 (feb 0x004546A0)
    BOOL32 SetupAniSystem();                                    // v12 0x0045D5E0 (feb 0x004547D0)
    int  ProcessINIT(void *init);                               // v12 0x0045DA70 (feb 0x00454C30)
    int  SelectHitSeq();                                        // v12 0x0045DCE0 (name ours; inside EvalAnimGoalSeq in feb)
    float GetAnimPlayRate();                                    // v12 0x0045DD70 (1.2 NEW) SHOT_STAY twice as fast
    int  EvalAnimGoalSeq();                                     // v12 0x0045DD90 (feb 0x00454EA0)
    int  CalcDamageToTarget(dmgGENERAL *g);                     // v12 0x0045E0C0 (feb 0x004551C0)
    void SelectGoalToRunAway(float dist);                       // v12 0x0045E4B0 (feb 0x00455450)
    void ProcessActionFrame();                                  // v12 0x0045E4E0 (feb 0x00455480)
    void Think();                                               // v12 0x0045E7E0 (feb 0x00455740)
    void PostThink();                                           // v12 0x0045E9C0 (feb 0x00455880)
    void FillAttackFlags();                                     // v12 0x0045EA40 (feb 0x00455900)
    void SetMovementSpeed();                                    // v12 0x0045ECB0 (feb 0x00455B70)
    int  SelectGoal();                                          // v12 0x0045EEA0 (feb 0x00455D20)
    void MoveToGoal();                                          // v12 0x0045EF40 (feb 0x00455DC0)
    void UpdateTimers();                                        // v12 0x0045F000 (feb 0x00455E80)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x0045F090 (feb 0x00455EF0)
    void Kick();                                                // v12 0x0045F170 (feb 0x00455FD0)
    void ProcessCDT(cdtINFO *ci);                               // v12 0x0045F280 (feb 0x004560E0)
    void PlaySounds();                                          // v12 0x0045F310 (feb 0x00456160)
    void FillSoundFlags();                                      // v12 0x0045F470 (feb 0x004562F0)
    int  GetLayer2State();                                      // v12 0x0045CD90 (shared): tigerFlags
    void SetLayer2State(int s);                                 // v12 0x0045A950 (shared)
};
int __fastcall aiTigerInit();                                   // v12 0x0045D440 (feb 0x00454630) "ai_lion", "ai_tiger"

// ================================================================================================
// aiCERBER_GENERIC : aiNPC_WR   registered as "ai_cerber"
//   Cerberus: three heads launch fireball / acid projectiles (1.2 class names pjl_crb_frb / pjl_crb_acd, feb pjl_cerber_fireball /
//   pjl_cerber_acid), tail and paw attacks. Paw: 10 damage inside 1.9 m (feb 1.5 m), zero at 2.3 m (feb 2.0 m).
//   Create v12 0x0043C870 (feb 0x00436340): operator new(0x59B) (feb 0x573); vtable v12 0x005FD1B0 (feb 0x005D2220), 117 slots (feb 114)
//   SetupAniSystem v12 0x0043C8C0 (feb 0x00436390 aiCERBER_GENERIC::SetupAniSystem): anim sequence slots in order:
//     0=CERBER_SEQ_IDLE 1=CERBER_SEQ_RUN 2=CERBER_SEQ_ATTACK_TAIL_RUN 3=CERBER_SEQ_ATTACK_HEADS 
//     4=CERBER_SEQ_ATTACK_TAIL_STAY 5=CERBER_SEQ_ATTACK_PAW 6=CERBER_SEQ_WOUNDED 7=CERBER_SEQ_DEATH 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x0043C8C0 SetupAniSystem | +0x07C 0x0043D230 FillSoundFlags
//     +0x080 0x0043D130 PlaySounds | +0x098 0x0043D020 SelectGoal | +0x0A4 0x0043D620 SetMovementSpeed
//     +0x0B0 0x0043F8E0 MoveToGoal* | +0x0C8 0x0043CD90 EvalAnimGoalSeq | +0x0D0 0x0043D310 ProcessActionFrame
//     +0x0EC 0x0043CE90 Think | +0x0F4 0x0043D060 UpdateTimers | +0x124 0x0045CD90 GetLayer3State*
//     +0x130 0x0045A950 SetLayer3State* | +0x140 0x0043CAA0 ProcessINIT | +0x150 0x0043D0A0 ProcessCDT_FIREABLE
//     +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x1D0 0x0043EFC0 LaunchOneProjectile
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x0043CABA
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x0043CAC0
//     ProcessINIT        maxHealth            = 50f        v12 +0x0D4 @0x0043CADD
//     ProcessINIT        navArriveDist        = 1.8f       v12 +0x1B3 @0x0043CAE3
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x0043CAED
//     ProcessINIT        moveSpeed            = 9.51815f   v12 +0x16F @0x0043CAF7
//     ProcessINIT        turnSpeed            = 70f        v12 +0x177 @0x0043CB01
//     ProcessINIT        f_56F                = 30f        v12 +0x597 @0x0043CB0B
//     ProcessINIT        radCDT               = 0.6f       v12 +0x187 @0x0043CB15
//     ProcessINIT        heightLowTester      = 0.72f      v12 +0x18B @0x0043CB29
//     ProcessINIT        heightCDT            = 1.2f       v12 +0x18F @0x0043CB33
//     ProcessINIT        f_55B                = 5f         v12 +0x583 @0x0043CB3D
//     ProcessINIT        shootRange           = 50f        v12 +0x1A7 @0x0043CB47
//     ProcessINIT        aimAngle             = 20f        v12 +0x1A3 @0x0043CB4D
//     ProcessINIT        fallbackAngle        = 100f       v12 +0x553 @0x0043CB53
//     ProcessINIT        maxTurnStep          = 20f        v12 +0x17B @0x0043CB5D
//     ProcessINIT        corpseTime           = 8f         v12 +0x4BB @0x0043CB63
//     ProcessINIT        corpseFadeTime       = 6f         v12 +0x4BF @0x0043CB6D
//     ProcessINIT        health               = 50f        v12 +0x0D0 @0x0043CB77
//     ProcessINIT        f_53F                = 0          v12 +0x567 @0x0043CB7D
//     ProcessINIT        f_543                = 0          v12 +0x56B @0x0043CB83
//     ProcessINIT        f_547                = 0          v12 +0x56F @0x0043CB89
//     ProcessINIT        f_54B                = 3f         v12 +0x573 @0x0043CB8F
//     ProcessINIT        f_54F                = 0          v12 +0x577 @0x0043CB99
//     ProcessINIT        f_557                = 6          v12 +0x57F @0x0043CB9F
//     ProcessINIT        sndChannel           = 0          v12 +0x4B3 @0x0043CC89
//     ProcessINIT        unk17B               = 0.15f      v12 +0x197 @0x0043CB1F   1.2 NEW
//     SetMovementSpeed   moveSpeed            = 4.75907f   v12 +0x16F @0x0043D648   CHANGED, feb 19.0363f
//     SetMovementSpeed   turnSpeed            = 84f        v12 +0x177 @0x0043D652   CHANGED, feb 70f
//     SetMovementSpeed   moveSpeed            = 9.51815f   v12 +0x16F @0x0043D65D
//     SetMovementSpeed   turnSpeed            = 70f        v12 +0x177 @0x0043D667
//     paw damage 10.0 / reach 1.9 / zero at 2.3: aiCERBER_GENERIC::ProcessActionFrame v12 0x0043D536 / 0x0043D562 / 0x0043D5CC
//     turn-run speed 4.759 @0x0043D648, turn 84 @0x0043D652; normal run 9.52 @0x0043D65D, turn 70 @0x0043D667
//   spawned entities:
//     "pjl_cerber_fireball" Create v12 -- (feb 0x004370E0) size 0x-- (feb 0x1A4) vt v12 -- (feb 0x005D23F0)
//     "pjl_cerber_acid" Create v12 -- (feb 0x004372E0) size 0x-- (feb 0x1A4) vt v12 -- (feb 0x005D2518)
//     "pjl_crb_frb" Create v12 0x0043D680 (feb --) size 0x1C0 (feb 0x--) vt v12 0x005FD398 (feb --)
//     "pjl_crb_acd" Create v12 0x0043DA60 (feb --) size 0x1C0 (feb 0x--) vt v12 0x005FD4E0 (feb --)
//     "sfx_cerber_pjl_expl" Create v12 0x0046E430 (feb 0x00464620) size 0x17B (feb 0x15F) vt v12 0x00601258 (feb 0x005D5E38)
//     "sfx_cerber_pjl_acid_expl" Create v12 0x0046E4B0 (feb 0x004646A0) size 0x177 (feb 0x15B) vt v12 0x006012C0 (feb 0x005D5EA0)
// ================================================================================================
class aiCERBER_GENERIC : public aiNPC_WR {              // src/Sources/ai_cerber.cpp
public:
    u32        cerbFlags;             // v12 +0x567 (feb +0x53F) 8 in WOUNDED, 0x10 a light hit, 0x20 a heavy one (or dying),
                                      //   0x100 a paw strike wanted
    float      headsPause;            // v12 +0x56B (feb +0x543) 0.1 s after the last head shot (counts down; not read)
    float      timer56F;              // v12 +0x56F (feb +0x547) counts down, never set
    float      shootTimer;            // v12 +0x573 (feb +0x54B) running: till a head spits (3..7 s)
    int        headShots;             // v12 +0x577 (feb +0x54F) shots left in ATTACK_HEADS / ATTACK_TAIL_STAY
    int        woundCanEnd;           // v12 +0x57B (feb +0x553) the hit sequence was not the death
    int        woundSeq;              // v12 +0x57F (feb +0x557) WOUNDED (6) or DEATH (7)
    float      f583;                  // v12 +0x583 (feb +0x55B) 5 (not read)
    objOBJ    *lHead;                 // v12 +0x587 (feb +0x55F) "AttackFocus_LHEAD"
    objOBJ    *rHead;                 // v12 +0x58B (feb +0x563) "AttackFocus_RHEAD"
    objOBJ    *tail;                  // v12 +0x58F (feb +0x567) "AttackFocus_TAIL" (the acid)
    objOBJ    *arm;                   // v12 +0x593 (feb +0x56B) "AttackFocus_ARM" (the paw)
    float      pawAngle;              // v12 +0x597 (feb +0x56F) 30
    // size v12 0x59B (feb 0x573)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x0043C870 (feb 0x00436340)
    BOOL32 SetupAniSystem();                                    // v12 0x0043C8C0 (feb 0x00436390)
    int  ProcessINIT(void *init);                               // v12 0x0043CAA0 (feb 0x00436570)
    int  EvalAnimGoalSeq();                                     // v12 0x0043CD90 (feb 0x00436850)
    void Think();                                               // v12 0x0043CE90 (feb 0x00436950)
    void FillAttackFlags();                                     // v12 0x0043CF30 (name ours; inside SelectGoal in feb)
    int  SelectGoal();                                          // v12 0x0043D020 (feb 0x00436A00)
    void UpdateTimers();                                        // v12 0x0043D060 (feb 0x00436B10)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x0043D0A0 (feb 0x00436B50)
    void PlaySounds();                                          // v12 0x0043D130 (feb 0x00436BE0)
    void FillSoundFlags();                                      // v12 0x0043D230 (feb 0x00436CB0)
    void ProcessActionFrame();                                  // v12 0x0043D310 (feb 0x00436D70)
    void SetMovementSpeed();                                    // v12 0x0043D620 (feb 0x00437080)
    void AimPjlToPlayer(m3dV *org, physINIT *p, float side, int tail); // v12 0x0043ED00 (feb 0x00438680)
    virtual void LaunchOneProjectile(objOBJ *o, float side, int flag); // [116] +0x1D0 v12 0x0043EFC0 (feb 0x00438940)
};
int __fastcall aiCerberInit();                                  // v12 0x0043C7E0 (feb 0x004362A0)

// ================================================================================================
// aiCROC : aiNPC_WR_FLY   registered as "ai_croc"
//   Crocodile (flying-class physics in water): idle / swim / bite / turn.
//   Create v12 0x0043FED0 (feb 0x00439640): operator new(0x58B) (feb 0x563); vtable v12 0x005FD6C8 (feb 0x005D26D0), 116 slots (feb 113)
//   SetupAniSystem v12 0x0043F270 (feb 0x00438BA0 aiCROC::SetupAniSystem): anim sequence slots in order:
//     0=CROC_SEQ_IDLE 1=CROC_SEQ_SWIM 2=CROC_SEQ_BITE 3=CROC_SEQ_TURN 4=CROC_SEQ_DEATH 
//   class-specific instance keys (section key -> v12 code address):
//     AI_PROP isNoSFXAppear  @0x0043F689
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x0043F270 SetupAniSystem | +0x064 0x0043F710 ParseNameClass*
//     +0x07C 0x0043FE70 FillSoundFlags | +0x080 0x0043FEA0 PlaySounds | +0x088 0x0043F730 StartDeath
//     +0x098 0x0043F7B0 SelectGoal | +0x0A4 0x0043F410 SetMovementSpeed | +0x0B0 0x0043F8E0 MoveToGoal*
//     +0x0C8 0x0043F8F0 EvalAnimGoalSeq | +0x0D0 0x0043FC30 ProcessActionFrame | +0x0EC 0x0043F9B0 Think
//     +0x0F4 0x0043F980 UpdateTimers | +0x124 0x0045C600 GetLayer3State* | +0x130 0x0045C610 SetLayer3State*
//     +0x140 0x0043F480 ProcessINIT | +0x144 0x00431260 ProcessTERM* | +0x14C 0x0043F720 ProcessFRAME*
//     +0x150 0x00460030 ProcessCDT_FIREABLE | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x194 0x0043FF20 IsAnimSeqGoodForFallback
//     +0x1A0 0x0043F740 Explode
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     SetMovementSpeed   turnSpeed            = 180f       v12 +0x177 @0x0043F42D
//     SetMovementSpeed   moveSpeed            = 10.272f    v12 +0x16F @0x0043F441
//     SetMovementSpeed   turnSpeed            = 45f        v12 +0x177 @0x0043F44B
//     SetMovementSpeed   turnSpeed            = 75f        v12 +0x177 @0x0043F456
//     SetMovementSpeed   moveSpeed            = 8.88f      v12 +0x16F @0x0043F460
//     SetMovementSpeed   moveSpeed            = 4.56f      v12 +0x16F @0x0043F46B
//     SetMovementSpeed   turnSpeed            = 45f        v12 +0x177 @0x0043F475
//     ProcessINIT        maxHealth            = 50f        v12 +0x0D4 @0x0043F4AC
//     ProcessINIT        health               = 50f        v12 +0x0D0 @0x0043F4B2
//     ProcessINIT        radCDT               = 1f         v12 +0x187 @0x0043F4CD
//     ProcessINIT        heightLowTester      = 3f         v12 +0x18B @0x0043F4D7
//     ProcessINIT        heightCDT            = -0.5f      v12 +0x18F @0x0043F4E1
//     ProcessINIT        unk17B               = 2.5f       v12 +0x197 @0x0043F4E7
//     ProcessINIT        navArriveDist        = 1.9f       v12 +0x1B3 @0x0043F4F1
//     ProcessINIT        timeDeactivate       = 120f       v12 +0x4D7 @0x0043F4FB
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x0043F505
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x0043F50B
//     ProcessINIT        f_557                = 30f        v12 +0x57F @0x0043F511
//     ProcessINIT        fallbackAngle        = 140f       v12 +0x553 @0x0043F51B
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x0043F525
//     ProcessINIT        maxTurnStep          = 70f        v12 +0x17B @0x0043F52F
//     ProcessINIT        f_54F                = 0          v12 +0x577 @0x0043F539
//     ProcessINIT        f_553                = 0          v12 +0x57B @0x0043F53F
//     ProcessINIT        corpseTime           = 10f        v12 +0x4BB @0x0043F545
//     ProcessINIT        corpseFadeTime       = 9f         v12 +0x4BF @0x0043F54F
//     ProcessINIT        f_55B                = 3          v12 +0x583 @0x0043F559
//     ProcessINIT        passivePhys373       = 0.25f      v12 +0x393 @0x0043F563
//     ProcessINIT        fallAccel            = -3f        v12 +0x397 @0x0043F56D
//     ProcessINIT        fallSpeedMin         = -1.5f      v12 +0x39B @0x0043F577
//     ProcessINIT        f_53F                = -0.5f      v12 +0x567 @0x0043F581
//     ProcessINIT        f_54B                = 0          v12 +0x573 @0x0043F663
//     ProcessINIT        bloodObj             = 0          v12 +0x54F @0x0043F6D1
// ================================================================================================
class aiCROC : public aiNPC_WR_FLY {                    // src/Sources/ai_croc.cpp
public:
    u32        crocFlags;             // v12 +0x56F (feb +0x547) 0x10000 turning, 0x100000 turn wanted, 0x1000000 spawned
    sncCHANNEL *voice;                // v12 +0x573 (feb +0x54B)
    float      biteTimer;             // v12 +0x577 (feb +0x54F) till it may bite again (1.5..3 s after one)
    float      fleeTimer;             // v12 +0x57B (feb +0x553) 1.2: swimming off (a bite that would not hurt: 2 s)
    float      biteAngle;             // v12 +0x57F (feb +0x557) 30: the enemy within this of its facing
    int        turnSeq;               // v12 +0x583 (feb +0x55B) 3
    objOBJ    *attackFocus;           // v12 +0x587 (feb +0x55F) "ATTACKfocus" (the jaws)
    // size v12 0x58B (feb 0x563)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x0043FED0 (feb 0x00439640)
    BOOL32 SetupAniSystem();                                    // v12 0x0043F270 (feb 0x00438BA0)
    void SetMovementSpeed();                                    // v12 0x0043F410 (feb 0x00438D40)
    int  ProcessINIT(void *init);                               // v12 0x0043F480 (feb 0x00438DB0)
    void Explode(dmgDAMAGE *d, int dead);                       // v12 0x0043F740 (feb 0x004390D0)
    int  SelectGoal();                                          // v12 0x0043F7B0 (feb 0x00439160)
    int  EvalAnimGoalSeq();                                     // v12 0x0043F8F0 (feb 0x00439240)
    void UpdateTimers();                                        // v12 0x0043F980 (feb 0x004392D0)
    void Think();                                               // v12 0x0043F9B0 (feb 0x00439300)
    void ProcessActionFrame();                                  // v12 0x0043FC30 (feb 0x00439380)
    void FillSoundFlags();                                      // v12 0x0043FE70 (feb 0x004395B0)
    void PlaySounds();                                          // v12 0x0043FEA0 (feb 0x00439610)
    int  IsAnimSeqGoodForFallback();                            // v12 0x0043FF20 (feb 0x00439690)
};
int __fastcall aiCrocInit();                                    // v12 0x0043F230 (feb 0x00438B60)

// ================================================================================================
// aiBIRD : aiMINO   registered as "ai_bird"
//   Stymphalian bird: flies inside an aiAREA (AI_PROP area, else 'ai_area_bird' object), soars and drops bird_egg bombs.
//   Create v12 0x00438D50 (feb 0x00432930): operator new(0x71B) (feb 0x6F3); vtable v12 0x005FCD20 (feb 0x005D1E10), 116 slots (feb 113)
//   SetupAniSystem v12 0x00438E00 (feb 0x004329E0 aiBIRD::SetupAniSystem): anim sequence slots in order:
//     0=BIRD_SEQ_UNUSED 1=BIRD_SEQ_FLY 2=BIRD_SEQ_THROW_EGG 3=BIRD_SEQ_DEATH 4=BIRD_SEQ_SOAR 
//   class-specific instance keys (section key -> v12 code address):
//     AI_PROP area  @0x004391F3
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00438E00 SetupAniSystem | +0x07C 0x0043A180 FillSoundFlags
//     +0x080 0x0043A200 PlaySounds | +0x088 0x00439460 StartDeath | +0x08C 0x00439490 UpdateDead
//     +0x098 0x00439850 SelectGoal | +0x0B0 0x004394D0 MoveToGoal | +0x0B4 0x004394E0 ClampGoalFrame
//     +0x0C8 0x00439A30 EvalAnimGoalSeq | +0x0D0 0x00439CC0 ProcessActionFrame | +0x0DC 0x00451B50 GetWpnTip*
//     +0x0E0 0x0043B900 GetPlayerTargetPos* | +0x0F4 0x00439EA0 UpdateTimers | +0x124 0x0045C600 GetLayer3State
//     +0x130 0x0045C610 SetLayer3State* | +0x140 0x00438FC0 ProcessINIT | +0x144 0x004392F0 ProcessTERM
//     +0x14C 0x00439320 ProcessFRAME | +0x150 0x00439420 ProcessCDT_FIREABLE | +0x154 0x00439360 ProcessCDT
//     +0x158 0x00439450 ProcessASK_KEEP_LEVEL_RELOAD | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x194 0x0043A300 IsAnimSeqGoodForFallback
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        radCDT               = 1.6f       v12 +0x187 @0x00438FD3
//     ProcessINIT        heightLowTester      = 1.6f       v12 +0x18B @0x00438FD9
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x00438FE4
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x00438FEA
//     ProcessINIT        heightCDT            = 0.6f       v12 +0x18F @0x00438FFB
//     ProcessINIT        moveSpeed            = 8f         v12 +0x16F @0x00439005
//     ProcessINIT        turnSpeed            = 70f        v12 +0x177 @0x0043900F
//     ProcessINIT        maxTurnStep          = 20f        v12 +0x17B @0x00439019
//     ProcessINIT        f_54B                = 5f         v12 +0x573 @0x0043904C
//     ProcessINIT        f_54F                = 0          v12 +0x577 @0x00439056
//     ProcessINIT        f_553                = 10f        v12 +0x57B @0x0043905C
//     ProcessINIT        f_557                = 0          v12 +0x57F @0x00439078
//     ProcessINIT        maxHealth            = 7f         v12 +0x0D4 @0x0043907E
//     ProcessINIT        health               = 7f         v12 +0x0D0 @0x00439084
//     ProcessINIT        fallbackAngle        = 10f        v12 +0x553 @0x0043908A
//     ProcessINIT        f_55B                = 0          v12 +0x583 @0x0043919E
//     ProcessINIT        f_563                = 0          v12 +0x58B @0x004391C6
//     ProcessINIT        f_563                = 0          v12 +0x58B @0x0043926E
//     ProcessINIT        passivePhys373       = 0.25f      v12 +0x393 @0x0043929A
//     ProcessINIT        fallAccel            = -3f        v12 +0x397 @0x004392A4
//     ProcessINIT        fallSpeedMin         = -0.17f     v12 +0x39B @0x004392AE
//     ProcessINIT        f_53F                = -4f        v12 +0x567 @0x004392B8
//     ProcessINIT        corpseTime           = 8f         v12 +0x4BB @0x004392C2
//     ProcessINIT        corpseFadeTime       = 6f         v12 +0x4BF @0x004392CC
//   spawned entities:
//     "bird_egg" Create v12 0x00463AF0 (feb 0x0045A450) size 0x1C0 (feb 0x1A4) vt v12 0x006004D0 (feb 0x005D50D0)
//     "sfx_egg" Create v12 0x004641F0 (feb 0x0045A850) size 0x1A7 (feb 0x183) vt v12 0x00600618 (feb 0x005D51F8)
//     "sfx_bird_death" Create v12 0x0046A870 (feb 0x00460AA0) size 0x17B (feb 0x15F) vt v12 0x00600E48 (feb 0x005D5A20)
// ================================================================================================
class aiBIRD : public aiNPC_WR_FLY {                  // src/Sources/ai_bird.cpp (the SDK's "aiMINO" base was wrong:
public:                                                 //   its vtable has aiNPC_WR_FLY's physics and SettleSurface)
    u32        birdFlags;             // v12 +0x56F (feb +0x547) layer 3 state: 2 the death effect started
    float      attackDelay;           // v12 +0x573 (feb +0x54B) till the next attack (5 at first, 10 after one)
    float      attackTime;            // v12 +0x577 (feb +0x54F) since the attack began
    float      timer57B;              // v12 +0x57B (1.2) 10, counted down  // ?
    float      wanderTimer;           // v12 +0x57F (feb +0x557) till the next point of its area
    sncCHANNEL *voice;                // v12 +0x583 (feb +0x55B)
    float      settleTimer;           // v12 +0x587 (feb +0x55F) the floor under it looked up again (1.5..2.5 s)
    aiAREA    *area;                  // v12 +0x58B (feb +0x563) where it flies about
    aiAREA     ownArea;               // v12 +0x58F (feb +0x567) its "AI_PROP area" (or "ai_area_bird") object's
    // size v12 0x71B (feb 0x6F3)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x00438D50 (feb 0x00432930)
    BOOL32 SetupAniSystem();                                    // v12 0x00438E00 (feb 0x004329E0)
    int  ProcessINIT(void *init);                               // v12 0x00438FC0 (feb 0x00432BA0)
    void ProcessTERM();                                         // v12 0x004392F0 (feb 0x00432EE0)
    void ProcessCDT(cdtINFO *ci);                               // v12 0x00439360 (feb 0x00432F20)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x00439420 (feb 0x00432FE0)
    void StartDeath(dmgDAMAGE *d);                              // v12 0x00439460 (feb 0x00433020)
    void UpdateDead();                                          // v12 0x00439490 (feb 0x00433060)
    void MoveToGoal();                                          // v12 0x004394D0 (feb 0x004330A0)
    void ClampGoalFrame(float speed, m3dV *dir, m3dV *goal);    // v12 0x004394E0 (feb 0x004330B0)
    int  SelectGoal();                                          // v12 0x00439850 (feb 0x00433430)
    int  EvalAnimGoalSeq();                                     // v12 0x00439A30 (feb 0x00433610)
    void ProcessActionFrame();                                  // v12 0x00439CC0 (feb 0x004338A0)
    void ProcessAttack();                                       // v12 0x00439D00 (feb 0x00433910)
    void UpdateTimers();                                        // v12 0x00439EA0 (feb 0x00433AB0)
    void ThrowEgg();                                            // v12 0x00439F40 (feb 0x00433B50)
    void FillSoundFlags();                                      // v12 0x0043A180 (feb 0x00433DA0)
    void PlaySounds();                                          // v12 0x0043A200 (feb 0x00433DF0)
    int  IsAnimSeqGoodForFallback();                            // v12 0x0043A300 (feb 0x00433EF0)
    void GetPlayerTargetPos(m3dV *out);                         // v12 0x0043B900 (shared)
    int  GetLayer3State();                                      // v12 0x0045C600 (shared)
    void SetLayer3State(int s);                                 // v12 0x0045C610 (shared)
};
int __fastcall aiBirdInit();                                    // v12 0x00438D00 (feb 0x004328E0)

// ================================================================================================
// aiAMUR_GENERIC : aiNPC_WR_FLY   registered as "ai_amur"
//   Amur (cupid): flying archer shooting pjl_amur_arrow. feb IsArmoredObject ('luk1','strela2','cyl3') removed in 1.2.
//   Create v12 0x00437180 (feb 0x00430FC0): operator new(0x5A7) (feb 0x57F); vtable v12 0x005FC9B0 (feb 0x005D1B00), 116 slots (feb 113)
//   SetupAniSystem v12 0x004371D0 (feb 0x00431010 aiAMUR_GENERIC::SetupAniSystem): anim sequence slots in order:
//     0=AMUR_SEQ_IDLE_FLY 1=AMUR_SEQ_TRAN_FLY2SHOOT 2=AMUR_SEQ_SHOOT 3=AMUR_SEQ_TRAN_SHOOT2FLY 4=AMUR_SEQ_WOUNDED 
//     5=AMUR_SEQ_START_DEATH 6=AMUR_SEQ_DEATH_LAY 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x004371D0 SetupAniSystem | +0x064 0x0043F710 ParseNameClass
//     +0x07C 0x00438250 FillSoundFlags | +0x080 0x00438130 PlaySounds | +0x088 0x004378A0 StartDeath
//     +0x098 0x00437AE0 SelectGoal | +0x0A4 0x00437FF0 SetMovementSpeed | +0x0B0 0x0043F8E0 MoveToGoal*
//     +0x0C8 0x00437800 EvalAnimGoalSeq | +0x0D0 0x004379C0 ProcessActionFrame | +0x0EC 0x00437790 Think
//     +0x0F4 0x00438050 UpdateTimers | +0x124 0x0045C600 GetLayer3State* | +0x130 0x004382F0 SetLayer3State
//     +0x140 0x00437420 ProcessINIT | +0x144 0x0043B090 ProcessTERM* | +0x14C 0x00437FD0 ProcessFRAME
//     +0x150 0x004380C0 ProcessCDT_FIREABLE | +0x154 0x00437900 ProcessCDT | +0x15C 0x00430A10 ProcessMP_NOTIFY*
//     +0x178 0x00445920 Activate | +0x1C4 0x00403200 IsArmoredObject
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x00437439
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x0043743F
//     ProcessINIT        shootRange           = 50f        v12 +0x1A7 @0x0043747B
//     ProcessINIT        maxTurnStep          = 50f        v12 +0x17B @0x00437497
//     ProcessINIT        maxHealth            = 7f         v12 +0x0D4 @0x00437451
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x00437457
//     ProcessINIT        moveSpeed            = 6f         v12 +0x16F @0x00437461
//     ProcessINIT        turnSpeed            = 60f        v12 +0x177 @0x00437467
//     ProcessINIT        f_577                = 5f         v12 +0x59F @0x00437471
//     ProcessINIT        aimAngle             = 20f        v12 +0x1A3 @0x00437481
//     ProcessINIT        corpseTime           = 7f         v12 +0x4BB @0x0043748B
//     ProcessINIT        corpseFadeTime       = 6f         v12 +0x4BF @0x00437491
//     ProcessINIT        timeDeactivate       = 120f       v12 +0x4D7 @0x0043749D
//     ProcessINIT        radCDT               = 1f         v12 +0x187 @0x004374A7   CHANGED, feb 3f
//     ProcessINIT        heightLowTester      = 3f         v12 +0x18B @0x004374B1
//     ProcessINIT        heightCDT            = 0          v12 +0x18F @0x004374BB
//     ProcessINIT        fallbackAngle        = 15f        v12 +0x553 @0x004374C1
//     ProcessINIT        health               = 7f         v12 +0x0D0 @0x004374CB
//     ProcessINIT        f_547                = 0          v12 +0x56F @0x004374D1
//     ProcessINIT        f_54F                = 0          v12 +0x577 @0x004374D7
//     ProcessINIT        f_553                = 0          v12 +0x57B @0x004374DD
//     ProcessINIT        f_563                = 0          v12 +0x58B @0x004374E3
//     ProcessINIT        f_567                = 1          v12 +0x58F @0x004374E9
//     ProcessINIT        f_56B                = 4          v12 +0x593 @0x004374F3
//     ProcessINIT        f_557                = 0          v12 +0x57F @0x004374FD
//     ProcessINIT        f_55F                = 0          v12 +0x587 @0x00437503
//     ProcessINIT        f_55B                = 2f         v12 +0x583 @0x00437509
//     ProcessINIT        f_573                = -12f       v12 +0x59B @0x004375E4
//     ProcessINIT        f_54B                = 0          v12 +0x573 @0x004376CE
//     ProcessINIT        bloodObj             = 0          v12 +0x54F @0x00437736
//     ProcessINIT        heightUp1            = 0.3f       v12 +0x3A3 @0x004375D4   1.2 NEW
//     SetMovementSpeed   moveSpeed            = 15f        v12 +0x16F @0x00438005
//     SetMovementSpeed   moveSpeed            = 6f         v12 +0x16F @0x00438011
//   spawned entities:
//     "pjl_amur_arrow" Create v12 0x00438710 (feb 0x004324E0) size 0x208 (feb 0x1EC) vt v12 0x005FCB90 (feb 0x005D1CD0)
// ================================================================================================
class aiAMUR_GENERIC : public aiNPC_WR_FLY {             // src/Sources/ai_amur.cpp
public:
    u32        amurFlags;             // v12 +0x56F (feb +0x547) 8 wounded (holding the sequence), 0x10 / 0x20 just hit
                                      //   (alive / falling), 0x40 attacks, 0x80 fallen, 0x200 attacking, 0x400 attack
                                      //   over (rest), 0x800 turn to the player first, 0xC00 at once when spawned
    sncCHANNEL *voice;                // v12 +0x573 (feb +0x54B) a sphere, 30..150
    float      shotPause;             // v12 +0x577 (feb +0x54F) 0.1 after the last arrow of a volley
    float      wanderTimer;           // v12 +0x57B (feb +0x553) 5: flying to a new point, no shooting
    float      attackTimer;           // v12 +0x57F (feb +0x557) the attack's length (5..10 s; 20..25 for 8 arrows)
    float      restTimer;             // v12 +0x583 (feb +0x55B) 2 s after an attack
    float      fastTimer;             // v12 +0x587 (feb +0x55F) 4 s at 15 m/s (the enemy out of range)
    int        shotsLeft;             // v12 +0x58B (feb +0x563) arrows of this attack (1, 3, 5, 8)
    int        woundEnds;             // v12 +0x58F (feb +0x567) 1: the wound's end releases the sequence (alive)
    int        woundSeq;              // v12 +0x593 (feb +0x56B) WOUNDED (4) or START_DEATH (5)
    float      hover;                 // v12 +0x597 (feb +0x56F) 6..12 m over the enemy
    float      fallDrag;              // v12 +0x59B (feb +0x573) -12: the fall's physics value, held till START_DEATH
    float      minShootDist;          // v12 +0x59F (feb +0x577) 5
    objOBJ    *attackObj;             // v12 +0x5A3 (feb +0x57B) "attack": where the arrows leave
    // size v12 0x5A7 (feb 0x57F)
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x00437180 (feb 0x00430FC0)
    BOOL32 SetupAniSystem();                                    // v12 0x004371D0 (feb 0x00431010)
    int  ProcessINIT(void *init);                               // v12 0x00437420 (feb 0x00431270)
    void Think();                                               // v12 0x00437790 (feb 0x004315C0)
    int  EvalAnimGoalSeq();                                     // v12 0x00437800 (feb 0x00431660)
    void StartDeath(dmgDAMAGE *d);                              // v12 0x004378A0 (feb 0x00431700)
    void ProcessCDT(cdtINFO *ci);                               // v12 0x00437900 (feb 0x00431760)
    void ProcessActionFrame();                                  // v12 0x004379C0 (feb 0x00431820)
    int  SelectGoal();                                          // v12 0x00437AE0 (feb 0x00431940)
    void ProcessFRAME();                                        // v12 0x00437FD0 (feb 0x00431D40)
    void SetMovementSpeed();                                    // v12 0x00437FF0 (feb 0x00431D60)
    void UpdateTimers();                                        // v12 0x00438050 (feb 0x00431DC0)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x004380C0 (feb 0x00431E30)
    void PlaySounds();                                          // v12 0x00438130 (feb 0x00431EA0)
    void FillSoundFlags();                                      // v12 0x00438250 (feb 0x00431FC0)
    void SetLayer3State(int s);                                 // v12 0x004382F0 (feb 0x004320C0)
    void GetShootArrowPhysParam(m3dV *org, physINIT *p);        // v12 0x00438330 (feb 0x00432100)
    void LaunchOneProjectile();                                 // v12 0x00438570 (feb 0x00432340)
};
int __fastcall aiAmurInit();                                    // v12 0x00437100 (feb 0x00430F50)

// ================================================================================================
// aiMEDUSA : aiNPC_WR_FLY   registered as "ai_medusa"
//   Medusa (boss, flying): AI_MEDUSA navHeight; when hit it spawns helper opponents (SelectOpponentToSpawn + entCreate)
//   while the planner has < 10 (feb) / < 10*aiSpawnOppCoeff (1.2) registered NPCs.
//   Create v12 0x0044ECC0 (feb 0x004469F0): operator new(0x5B7) (feb 0x587); vtable v12 0x005FE828 (feb 0x005D3638), 116 slots (feb 113)
//   SetupAniSystem v12 0x0044CBA0 (feb 0x00444B60 aiMEDUSA::SetupAniSystem): anim sequence slots in order:
//     0=MEDUSA_SEQ_FLY 1=MEDUSA_SEQ_ATTACK 2=MEDUSA_SEQ_WOUNDED 
//   class-specific instance keys (section key -> v12 code address):
//     AI_MEDUSA navHeight  @0x0044D1A0
//     AI_PROP isNoSFXAppear  @0x0044D167
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x0044CBA0 SetupAniSystem | +0x064 0x0044D340 ParseNameClass
//     +0x07C 0x0044E750 FillSoundFlags | +0x080 0x0044DB30 PlaySounds | +0x088 0x0044D730 StartDeath
//     +0x098 0x0044D840 SelectGoal | +0x0A4 0x0044CD00 SetMovementSpeed | +0x0B0 0x0043F8E0 MoveToGoal*
//     +0x0B4 0x0044DBE0 ClampGoalFrame | +0x0C8 0x0044DA20 EvalAnimGoalSeq | +0x0D0 0x0044DAB0 ProcessActionFrame
//     +0x0EC 0x0044E790 Think | +0x0F0 0x0044E7C0 PostThink | +0x0F4 0x0044DAE0 UpdateTimers
//     +0x124 0x0045C600 GetLayer3State* | +0x130 0x0045C610 SetLayer3State | +0x140 0x0044CF80 ProcessINIT
//     +0x144 0x0044CEA0 ProcessTERM | +0x14C 0x0043F720 ProcessFRAME* | +0x150 0x0044E3B0 ProcessCDT_FIREABLE
//     +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x178 0x0044CE10 Activate | +0x188 0x0044EB60 StartPassivePhysics
//     +0x1A0 0x0044D7B0 Explode
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     SetMovementSpeed   turnSpeed            = 70f        v12 +0x177 @0x0044CD1B
//     SetMovementSpeed   moveSpeed            = 6f         v12 +0x16F @0x0044CD25
//     SetMovementSpeed   turnSpeed            = 180f       v12 +0x177 @0x0044CD37
//     SetMovementSpeed   f_583                = 0          v12 +0x5AF @0x0044CDF2
//     ProcessINIT        maxHealth            = 6000f      v12 +0x0D4 @0x0044CFB0
//     ProcessINIT        health               = 6000f      v12 +0x0D0 @0x0044CFB6
//     ProcessINIT        heightCDT            = 2.5f       v12 +0x18F @0x0044CFC1
//     ProcessINIT        radCDT               = 3.5f       v12 +0x187 @0x0044CFCB
//     ProcessINIT        heightLowTester      = 3.5f       v12 +0x18B @0x0044CFD1
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x0044CFDC
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x0044CFE6
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x0044CFEC
//     ProcessINIT        maxTurnStep          = 70f        v12 +0x17B @0x0044CFF2
//     ProcessINIT        f_54F                = 0          v12 +0x577 @0x0044CFFC
//     ProcessINIT        f_553                = 0          v12 +0x57B @0x0044D002
//     ProcessINIT        timeDeactivate       = 3600f      v12 +0x4D7 @0x0044D008
//     ProcessINIT        feb only: feb +0x197 = 15f (removed in 1.2)
//     ProcessINIT        f_56F                = 0          v12 +0x5AB @0x0044D049
//     ProcessINIT        f_57F                = 0          v12 +0x5AF @0x0044D04F
//     ProcessINIT        f_583                = 0          v12 +0x5B3 @0x0044D058
//     ProcessINIT        f_54B                = 0          v12 +0x573 @0x0044D13F
//     ProcessINIT        f_56B                = 0          v12 +0x593 @0x0044D1AC
//     ProcessINIT        f_56B                = 0          v12 +0x593 @0x0044D20C
//     ProcessINIT        bloodObj             = 0          v12 +0x54F @0x0044D251
//     ProcessINIT        f_55B                = 0          v12 +0x583 @0x0044D286
//     ProcessINIT        f_55F                = 0          v12 +0x587 @0x0044D28C
//     ProcessINIT        navArriveDist        = 12f        v12 +0x1B3 @0x0044D012   1.2 NEW
//     ProcessINIT        new_v12_597          = 0          v12 +0x597 @0x0044D01C   1.2 NEW
//     ProcessINIT        new_v12_59B          = 0          v12 +0x59B @0x0044D292   1.2 NEW
//     ProcessINIT        new_v12_59B          = 0          v12 +0x59B @0x0044D2E9   1.2 NEW
//     ParseNameClass     corpseTime           = 0.9f       v12 +0x4BB @0x0044D370
//     ParseNameClass     corpseFadeTime       = 0.733333f  v12 +0x4BF @0x0044D37C
//     ParseNameClass     fallbackAngle        = 250f       v12 +0x553 @0x0044D38E
//     ParseNameClass     shootRange           = 200f       v12 +0x1A7 @0x0044D39B
//     ParseNameClass     fallbackAngle        = 10000f     v12 +0x553 @0x0044D3B3
//     ParseNameClass     shootRange           = 1000f      v12 +0x1A7 @0x0044D3C2
//     ParseNameClass     f_56F                = 0          v12 +0x597 @0x0044D414
//     ParseNameClass     f_56F                = 0          v12 +0x597 @0x0044D46B
//     helper spawn cap: 10.0 * aiSpawnOppCoeff, fmul @0x0044E47E (aiMEDUSA::ProcessCDT_FIREABLE)
//   spawned entities:
//     "pjl_medusa" Create v12 0x0044ED80 (feb 0x00446AB0) size 0x1D4 (feb 0x1B8) vt v12 0x005FEA08 (feb 0x005D3808)
//     "sfx_medusa_expl" Create v12 0x0047B570 (feb 0x004715B0) size 0x1B3 (feb 0x18F) vt v12 0x00602860 (feb 0x005D7440)
//     "sfx_medusa_death" Create v12 0x0047B650 (feb 0x00471680) size 0x1B3 (feb 0x18F) vt v12 0x006028D0 (feb 0x005D74B8)
// ================================================================================================
class aiMEDUSA : public aiNPC_WR_FLY {
public:
    u32        f_547;                 // v12 +0x56F (feb +0x547)
    int        f_54B;                 // v12 +0x573 (feb +0x54B)
    u32        f_54F;                 // v12 +0x577 (feb +0x54F) init 0
    u32        f_553;                 // v12 +0x57B (feb +0x553) init 0
    void*      f_55B;                 // v12 +0x583 (feb +0x55B) init 0
    int        f_55F;                 // v12 +0x587 (feb +0x55F) init 0
    objOBJ*    f_567;                 // v12 +0x58F (feb +0x567)
    u32        f_56B;                 // v12 +0x593 (feb +0x56B) init 0
    u32        f_56F;                 // v12 +0x597 (feb +0x56F) init 0
    u32        f_573;                 // v12 +0x59B (feb +0x573)  // ? offset by shift
    u32        f_577;                 // v12 +0x59F (feb +0x577)  // ? offset by shift
    u32        f_57B;                 // v12 +0x5A3 (feb +0x57B)  // ? offset by shift
    u32        f_57F;                 // v12 +0x5AF (feb +0x57F) init 0
    float      f_583;                 // v12 +0x5B3 (feb +0x583)
    // size v12 0x5B7 (feb 0x587)
};

// ================================================================================================
// aiCYCLOP : aiNPC_WR   registered as "ai_cyclop"
//   Cyclops (boss and mini, AI_PROP size scales it): walk, club attack, grab+munch the player, spit, stone throw (pjl_stone_cyclop).
//   Create v12 0x00441F30 (feb 0x0043AF00): operator new(0x5A7) (feb 0x57F); vtable v12 0x005FDA98 (feb 0x005D2A00), 116 slots (feb 113)
//   SetupAniSystem v12 0x00441F80 (feb 0x0043AF50 aiCYCLOP::SetupAniSystem): anim sequence slots in order:
//     0=CYCLOP_SEQ_APPEAR 1=CYCLOP_SEQ_WALK 2=CYCLOP_SEQ_ATTACK 3=CYCLOP_SEQ_IDLE 4=CYCLOP_SEQ_ROAR 
//     5=CYCLOP_SEQ_DEATH 6=CYCLOP_SEQ_GRAB 7=CYCLOP_SEQ_SPIT 8=CYCLOP_SEQ_MUNCH 9=CYCLOP_SEQ_BLINK 
//     10=CYCLOP_SEQ_SHOT 11=CYCLOP_SEQ_CLOSE_ATTACK 
//   class-specific instance keys (section key -> v12 code address):
//     AI_PROP size  @0x0044263E
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00441F80 SetupAniSystem | +0x064 0x004425A0 ParseNameClass
//     +0x078 0x004448D0 GetFloorPlaceRefiner | +0x080 0x00444530 PlaySounds | +0x098 0x00443EA0 SelectGoal
//     +0x0A4 0x00443FC0 SetMovementSpeed | +0x0B0 0x0043F8E0 MoveToGoal* | +0x0B4 0x004440B0 ClampGoalFrame
//     +0x0C8 0x00442AA0 EvalAnimGoalSeq | +0x0CC 0x00443AC0 GetAnimPlayRate | +0x0D0 0x00442E70 ProcessActionFrame
//     +0x0EC 0x00443D20 Think | +0x0F4 0x00444430 UpdateTimers | +0x124 0x0045CD90 GetLayer3State*
//     +0x130 0x0045A950 SetLayer3State* | +0x140 0x004422B0 ProcessINIT | +0x14C 0x004429E0 ProcessFRAME
//     +0x150 0x004444A0 ProcessCDT_FIREABLE | +0x15C 0x00444790 ProcessMP_NOTIFY | +0x178 0x00445920 Activate
//     +0x17C 0x004427E0 ActivatePlrVisibility | +0x194 0x004448B0 IsAnimSeqGoodForFallback | +0x1C4 0x00444870 IsArmoredObject
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x004422CA
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x004422D0
//     ProcessINIT        maxHealth            = 4000f      v12 +0x0D4 @0x004422E2
//     ProcessINIT        navArriveDist        = 4f         v12 +0x1B3 @0x004422E8
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x004422FC
//     ProcessINIT        moveSpeed            = 5.06f      v12 +0x16F @0x00442306
//     ProcessINIT        turnSpeed            = 90f        v12 +0x177 @0x00442310
//     ProcessINIT        f_577                = 5.2f       v12 +0x59F @0x0044231A
//     ProcessINIT        f_57B                = 1f         v12 +0x5A3 @0x00442324
//     ProcessINIT        f_563                = 30f        v12 +0x58B @0x0044232E
//     ProcessINIT        timeDeactivate       = 3600f      v12 +0x4D7 @0x00442338
//     ProcessINIT        health               = 4000f      v12 +0x0D0 @0x00442342
//     ProcessINIT        f_53F                = 1          v12 +0x567 @0x00442348
//     ProcessINIT        f_0F8                = 0          v12 +0x108 @0x00442352
//     ProcessINIT        f_547                = 9          v12 +0x56F @0x00442358
//     ProcessINIT        f_54B                = 9          v12 +0x573 @0x0044235E
//     ProcessINIT        maxTurnStep          = 20f        v12 +0x17B @0x0044236A
//     ProcessINIT        f_54F                = 0          v12 +0x577 @0x00442374
//     ProcessINIT        f_553                = 0          v12 +0x57B @0x0044237A
//     ProcessINIT        f_557                = 0          v12 +0x57F @0x00442380
//     ProcessINIT        f_567                = 0          v12 +0x58F @0x004423AB
//     ProcessINIT        f_543                = 0          v12 +0x56B @0x00442494
//     ProcessINIT        new_v12_1B7          = 0.6f       v12 +0x1B7 @0x004422F2   1.2 NEW
//     ParseNameClass     f_577                = -1f        v12 +0x59F @0x00442614
//     ParseNameClass     maxHealth            = 300f       v12 +0x0D4 @0x0044261E
//     ParseNameClass     health               = 300f       v12 +0x0D0 @0x00442624
//     ParseNameClass     f_57B                = 0.5f       v12 +0x5A3 @0x0044262A
//     ParseNameClass     corpseTime           = 10f        v12 +0x4BB @0x004426F7
//     ParseNameClass     corpseFadeTime       = 9f         v12 +0x4BF @0x00442701
//     ParseNameClass     fallbackAngle        = 400f       v12 +0x553 @0x0044270B
//     ParseNameClass     corpseTime           = 15f        v12 +0x4BB @0x0044273E
//     ParseNameClass     corpseFadeTime       = 14f        v12 +0x4BF @0x00442748
//     ParseNameClass     fallbackAngle        = 10000f     v12 +0x553 @0x00442752
//     SetMovementSpeed   turnSpeed            = 0          v12 +0x177 @0x00443FEB
//     SetMovementSpeed   turnSpeed            = 90f        v12 +0x177 @0x00444083
//     SetMovementSpeed   turnSpeed            = 108f       v12 +0x177 @0x00444092
//   spawned entities:
//     "pjl_stone_cyclop" Create v12 0x0043FF80 (feb 0x004396D0) size 0x1DC (feb 0x1B8) vt v12 0x005FD8A0 (feb 0x005D28A0)
//     "sfx_cyclop_walk" Create v12 0x004706A0 (feb 0x00466960) size 0x177 (feb 0x15B) vt v12 0x00601590 (feb 0x005D6170)
//     "sfx_cyclop_hit" Create v12 0x00470720 (feb 0x004669E0) size 0x177 (feb 0x15B) vt v12 0x006015F8 (feb 0x005D61D8)
//     "sfx_cyclop_death" Create v12 0x004707A0 (feb 0x00466A60) size 0x177 (feb 0x15B) vt v12 0x00601660 (feb 0x005D6240)
// ================================================================================================
class aiCYCLOP : public aiNPC_WR {   // src/Sources/ai_cyclop.cpp (cycFlags bits: see there)
public:
    u32        cycFlags;              // v12 +0x567 (feb +0x53F)
    sncCHANNEL *voice;                // v12 +0x56B (feb +0x543) sncCHANNEL_SPH
    int        shotSeq;               // v12 +0x56F (feb +0x547) the reaction to a hit (DecideShotSequence), init 9
    int        attackSeq;             // v12 +0x573 (feb +0x54B) the attack chosen, init 9
    float      attackTimer;           // v12 +0x577 (feb +0x54F) no new attack till it runs out
    float      spitTimer;             // v12 +0x57B (feb +0x553) till the next stone of a spit
    float      outsideTime;           // v12 +0x57F (feb +0x557) time outside its banned zone (0 while wrFlags 0x10000)
    float      clubBorder;            // v12 +0x583 (feb +0x55B) chance of a club attack (CalcBorderForClubAttack)
    float      spitBorder;            // v12 +0x587 (feb +0x55F) chance of a spit (CalcBorderForSpitAttack)
    float      f_58B;                 // v12 +0x58B (feb +0x563) 30 // ? (never read)
    int        returnCount;           // v12 +0x58F (feb +0x567) SelectGoals left walking back into its nav area
    objOBJ    *attackFocus;           // v12 +0x593 (feb +0x56B) "AttackFocus": the club's blow, the stones' mouth
    objOBJ    *lFoot;                 // v12 +0x597 (feb +0x56F) "LKOP"
    objOBJ    *rFoot;                 // v12 +0x59B (feb +0x573) "RKOP"
    float      closeDist;             // v12 +0x59F (feb +0x577) nearer: the close attack (5.2; a mini: -1, never)
    float      size;                  // v12 +0x5A3 (feb +0x57B) 1; a mini 0.5 x AI_PROP size
    // size v12 0x5A7 (feb 0x57F)

    static txmTEXTURE *pTexScorchMark;                          // v12 0x0064AF78 "sfx_ax_scorch_mark"
    static entENTITY *__fastcall Create(animINST *i);           // v12 0x00441F30 (feb 0x0043AF00)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // v12 0x004449A0
    BOOL32 SetupAniSystem();                                    // v12 0x00441F80 (feb 0x0043AF50)
    int  ProcessINIT(void *init);                               // v12 0x004422B0 (feb 0x0043B280)
    void ProcessTERM();                                         // v12 0x004427C0
    void ParseNameClass();                                      // v12 0x004425A0 (feb 0x0043B570)
    void ActivatePlrVisibility();                               // v12 0x004427E0 (feb 0x0043B780)
    void SendClubDamage();                                      // v12 0x00442840 (feb 0x0043B7E0)
    void Step(objOBJ *foot, u32 snd);                           // (inline in ProcessActionFrame)
    void ProcessFRAME();                                        // v12 0x004429E0 (feb 0x0043B970)
    int  DecideShotSequence();                                  // v12 0x00442A10 (feb 0x0043B9A0)
    int  ChooseAttack();                                        // (inline twice in EvalAnimGoalSeq)
    int  EvalAnimGoalSeq();                                     // v12 0x00442AA0 (feb 0x0043BA40)
    void ProcessActionFrame();                                  // v12 0x00442E70 (feb 0x0043BDA0)
    float GetAnimPlayRate();                                    // v12 0x00443AC0 (feb 0x0043C840)
    float CalcBorderForClubAttack();                            // v12 0x00443B50 (feb 0x0043C880)
    float CalcBorderForSpitAttack();                            // v12 0x00443C40 (feb 0x0043C970)
    void Think();                                               // v12 0x00443D20 (feb 0x0043CA50)
    void PostThink();                                           // v12 0x00443E70
    int  SelectGoal();                                          // v12 0x00443EA0 (feb 0x0043CB70)
    void SetMovementSpeed();                                    // v12 0x00443FC0 (feb 0x0043CC90)
    void ClampGoalFrame(float speed, m3dV *dir, m3dV *goal);    // v12 0x004440B0 (feb 0x0043CD80)
    void UpdateTimers();                                        // v12 0x00444430 (feb 0x0043D110)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x004444A0 (feb 0x0043D180)
    void FillSoundFlags();                                      // v12 0x00444500
    void PlaySounds();                                          // v12 0x00444530 (feb 0x0043D200)
    void ProcessMP_NOTIFY(gsEVENT *ev);                         // v12 0x00444790 (feb 0x0043D3A0)
    int  IsArmoredObject(char *name);                           // v12 0x00444870 (feb 0x0043D420)
    int  IsAnimSeqGoodForFallback();                            // v12 0x004448B0 (feb 0x0043D460)
    class cdtREFINE *GetFloorPlaceRefiner();                    // v12 0x004448D0 (feb 0x0043D480)
    void GetStonePhysParam(m3dV *from, struct pjlPJL_INIT *init); // v12 0x004408F0
    void LaunchOneStone();                                      // v12 0x00440B30 (a mini's spit)
    void LaunchSeveralStones();                                 // v12 0x00440DC0 (the boss's spit)
};
int __fastcall aiCyclopInit();                                  // v12 0x00441EA0 (feb 0x0043AE90)
int __fastcall aiCycStonePjlInit();                             // v12 0x0043FF30
int __fastcall wrsfxCyclopInit();                               // v12 0x0046E530 (feb 0x00464720)

// ================================================================================================
// aiGEFEST_GENERIC : aiNPC_WR   registered as "ai_gefest"
//   Hephaestus/Gefest (boss): hammer smash (+short smash), throws pjl_gefest fireballs, roar. 1.2 also reads AI_PROP size.
//   Create v12 0x00445180 (feb 0x0043D880): operator new(0x5AF) (feb 0x587); vtable v12 0x005FDCD0 (feb 0x005D2BE8), 118 slots (feb 115)
//   SetupAniSystem v12 0x004451F0 (feb 0x0043D8F0 aiGEFEST_GENERIC::SetupAniSystem): anim sequence slots in order:
//     0=GEFEST_SEQ_IDLE 1=GEFEST_SEQ_WALK 2=GEFEST_SEQ_SMASH 3=GEFEST_SEQ_THROW 4=GEFEST_SEQ_ROAR 
//     5=GEFEST_SEQ_WOUNDED 6=GEFEST_SEQ_DEATH 7=GEFEST_SEQ_SHORTSMASH 
//   class-specific instance keys (section key -> v12 code address):
//     AI_PROP size  @0x0044579A   (1.2 only)
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x004451F0 SetupAniSystem | +0x064 0x004456E0 ParseNameClass
//     +0x07C 0x00445F70 FillSoundFlags | +0x080 0x00445E90 PlaySounds | +0x084 0x00446A00 UpdateSFX
//     +0x098 0x00445DD0 SelectGoal | +0x0A4 0x00445D20 SetMovementSpeed | +0x0B0 0x0043F8E0 MoveToGoal*
//     +0x0B4 0x004440B0 ClampGoalFrame* | +0x0C8 0x00445930 EvalAnimGoalSeq | +0x0CC 0x004462B0 GetAnimPlayRate
//     +0x0D0 0x004460D0 ProcessActionFrame | +0x0EC 0x00445D90 Think | +0x0F4 0x00456F70 UpdateTimers
//     +0x124 0x0045CD90 GetLayer3State* | +0x130 0x0045A950 SetLayer3State* | +0x140 0x00445400 ProcessINIT
//     +0x144 0x004458B0 ProcessTERM | +0x14C 0x00445DF0 ProcessFRAME | +0x150 0x00445E20 ProcessCDT_FIREABLE
//     +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x16C 0x00446890 Freeze | +0x178 0x00445920 Activate
//     +0x194 0x00446B30 IsAnimSeqGoodForFallback | +0x1A0 0x00446840 Explode | +0x1C4 0x004467F0 IsArmoredObject
//     +0x1D0 0x00447AD0 LaunchOneProjectile | +0x1D4 0x00447C70 LaunchSeveralProjectiles
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x0044541A
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x00445420
//     ProcessINIT        radCDT               = 4.8f       v12 +0x187 @0x00445430
//     ProcessINIT        heightLowTester      = 4.8f       v12 +0x18B @0x00445436
//     ProcessINIT        maxHealth            = 5000f      v12 +0x0D4 @0x00445456
//     ProcessINIT        navArriveDist        = 1.8f       v12 +0x1B3 @0x0044545C
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x00445470
//     ProcessINIT        moveSpeed            = 5.1072f    v12 +0x16F @0x0044547A
//     ProcessINIT        turnSpeed            = 90f        v12 +0x177 @0x00445484
//     ProcessINIT        f_56B                = 5f         v12 +0x593 @0x0044548E
//     ProcessINIT        shootRange           = 50f        v12 +0x1A7 @0x00445498
//     ProcessINIT        aimAngle             = 20f        v12 +0x1A3 @0x004454A2
//     ProcessINIT        timeDeactivate       = 3600f      v12 +0x4D7 @0x004454A8
//     ProcessINIT        heightCDT            = 8.5f       v12 +0x18F @0x004454B2
//     ProcessINIT        heightUpTester       = 3f         v12 +0x3A7 @0x004454BC
//     ProcessINIT        heightUp1            = 9f         v12 +0x3A3 @0x004454C6
//     ProcessINIT        navAnalyzeDist       = 20f        v12 +0x1BB @0x004454D0
//     ProcessINIT        f_567                = 1f         v12 +0x58F @0x004454D6
//     ProcessINIT        health               = 5000f      v12 +0x0D0 @0x004454E0
//     ProcessINIT        f_53F                = 0          v12 +0x567 @0x004454E6
//     ProcessINIT        f_547                = 0          v12 +0x56F @0x004454EC
//     ProcessINIT        f_553                = 0          v12 +0x57B @0x004454F2
//     ProcessINIT        f_55B                = 5          v12 +0x583 @0x004454F8
//     ProcessINIT        f_54F                = 7          v12 +0x577 @0x00445502
//     ProcessINIT        f_543                = 0          v12 +0x56B @0x004455EE
//     ProcessINIT        bloodObj             = 0          v12 +0x54F @0x0044567B
//     ProcessINIT        new_v12_1B7          = 0.6f       v12 +0x1B7 @0x00445466   1.2 NEW
//     ParseNameClass     radCDT               = 3f         v12 +0x187 @0x0044573D
//     ParseNameClass     heightLowTester      = 3f         v12 +0x18B @0x00445743
//     ParseNameClass     maxHealth            = 400f       v12 +0x0D4 @0x00445762
//     ParseNameClass     health               = 400f       v12 +0x0D0 @0x00445768
//     ParseNameClass     heightCDT            = 5f         v12 +0x18F @0x0044576E
//     ParseNameClass     heightUpTester       = 2.5f       v12 +0x3A7 @0x00445774
//     ParseNameClass     heightUp1            = 5f         v12 +0x3A3 @0x0044577E
//     ParseNameClass     f_567                = 0.5f       v12 +0x58F @0x00445784
//     ParseNameClass     corpseTime           = 10f        v12 +0x4BB @0x004457C9
//     ParseNameClass     corpseFadeTime       = 9f         v12 +0x4BF @0x004457D3
//     ParseNameClass     fallbackAngle        = 400f       v12 +0x553 @0x004457DD
//     ParseNameClass     corpseTime           = 1000f      v12 +0x4BB @0x00445847
//     ParseNameClass     corpseFadeTime       = 999f       v12 +0x4BF @0x00445856
//     ParseNameClass     fallbackAngle        = 10000f     v12 +0x553 @0x00445866
//     SetMovementSpeed   turnSpeed            = 0          v12 +0x177 @0x00445D41
//     SetMovementSpeed   turnSpeed            = 90f        v12 +0x177 @0x00445D64
//     SetMovementSpeed   turnSpeed            = 108f       v12 +0x177 @0x00445D77
//   spawned entities:
//     "pjl_gefest" Create v12 0x00446B50 (feb 0x0043F0A0) size 0x1C0 (feb 0x1A4) vt v12 0x005FDF58 (feb 0x005D2E60)
//     "sfx_gefest_hit" Create v12 0x004763D0 (feb 0x0046C6F0) size 0x17B (feb 0x15F) vt v12 0x00601FB8 (feb 0x005D6BA8)
//     "sfx_gefest_expl" Create v12 0x00476450 (feb 0x0046C770) size 0x17B (feb 0x15F) vt v12 0x00602020 (feb 0x005D6C10)
// ================================================================================================
class aiGEFEST_GENERIC : public aiNPC_WR {   // src/Sources/ai_gefest.cpp (gefFlags bits: see there)
public:
    u32        gefFlags;              // v12 +0x567 (feb +0x53F) GetLayer3State / SetLayer3State
    sncCHANNEL_CONE *voice;           // v12 +0x56B (feb +0x543) his own sound channel (a sphere, 360 m)
    float      timer56F;              // v12 +0x56F (feb +0x547) counted down by UpdateTimers, init 0  // ? (never set)
    float      attackDelay;           // v12 +0x573 (feb +0x54B) no new attack while > 0.01  // ? (never set)
    int        attackSeq;             // v12 +0x577 (feb +0x54F) the attack chosen (3 throw, 2 smash, 7 short smash), init 7
    u32        f_57B;                 // v12 +0x57B (feb +0x553) init 0  // ?
    u32        reactAlive;            // v12 +0x57F (feb +0x557) EvalAnimGoalSeq: 1 unless dead when it reacted  // ?
    int        reactSeq;              // v12 +0x583 (feb +0x55B) the reaction to a hit (5 wounded, 1 walk, 6 death), init 5
    float      smashChance;           // v12 +0x587 (feb +0x55F) FillAttackFlag
    float      throwChance;           // v12 +0x58B (feb +0x563) CalcBorderForThrowAttack
    float      size;                  // v12 +0x58F (feb +0x567) 1; a mini 0.5 x AI_PROP size
    float      f_593;                 // v12 +0x593 (feb +0x56B) 5  // ? (never read)
    objOBJ    *attackFocus;           // v12 +0x597 (feb +0x56F) "AttackFocus": the smash and the throws
    partEMITTER_PHYS *carbons;        // v12 +0x59B (feb +0x573) the coals in his hand (aiGEFEST_CARBONS)
    objOBJ    *carbonsObj;            // v12 +0x59F (feb +0x577) "carbons_source"
    partEMITTER_PHYS *bowlCarbons;    // v12 +0x5A3 (feb +0x57B) the coals in his bowl (aiGEFEST_BOWL_CARBONS)
    partEMITTER_PHYS *bowlGlow;       // v12 +0x5A7 (feb +0x57F) its glow (aiGEFEST_BOWL_GLOW)
    objOBJ    *bowlObj;               // v12 +0x5AB (feb +0x583) "bowl_source"
    // size v12 0x5AF (feb 0x587)

    static entENTITY *__fastcall Create(animINST *i);           // v12 0x00445180 (feb 0x0043D880)
    BOOL32 SetupAniSystem();                                    // v12 0x004451F0 (feb 0x0043D8F0)
    int  ProcessINIT(void *init);                               // v12 0x00445400 (feb 0x0043DB00)
    void ParseNameClass();                                      // v12 0x004456E0 (feb 0x0043DDC0)
    void ProcessTERM();                                         // v12 0x004458B0 (feb 0x0043DF10)
    int  Activate(int a);                                       // v12 0x00445920 (feb 0x0043DF70)
    int  EvalAnimGoalSeq();                                     // v12 0x00445930 (feb 0x0043DFA0)
    float CalcBorderForThrowAttack();                           // v12 0x00445A90 (feb 0x0043E120)
    void FillAttackFlag();                                      // v12 0x00445B60 (feb 0x0043E1F0)
    void SetMovementSpeed();                                    // v12 0x00445D20 (feb 0x0043E3A0)
    void Think();                                               // v12 0x00445D90 (feb 0x0043E410)
    int  SelectGoal();                                          // v12 0x00445DD0 (feb 0x0043E440)
    void ProcessFRAME();                                        // v12 0x00445DF0 (feb 0x0043E460)
    void ClampGoalFrame(float speed, m3dV *dir, m3dV *goal);    // v12 0x004440B0 (shared with aiCYCLOP)
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d);                     // v12 0x00445E20 (feb 0x0043E4A0)
    void PlaySounds();                                          // v12 0x00445E90 (feb 0x0043E510)
    void FillSoundFlags();                                      // v12 0x00445F70 (feb 0x0043E620)
    void ProcessActionFrame();                                  // v12 0x004460D0 (feb 0x0043E6C0)
    float GetAnimPlayRate();                                    // v12 0x004462B0 (feb 0x0043E8F0)
    int  IsArmoredObject(char *name);                           // v12 0x004467F0 (feb 0x0043EDE0)
    void Explode(dmgDAMAGE *d, int dead);                       // v12 0x00446840 (feb 0x0043EE30)
    void Freeze();                                              // v12 0x00446890 (feb 0x0043EE80)
    int  StartSFX();                                            // v12 0x004468E0 (feb 0x0043EED0)
    void Slot33();                                              // v12 0x00446A00 (feb 0x0043EFF0) UpdateSFX
    int  IsAnimSeqGoodForFallback();                            // v12 0x00446B30 (feb 0x0043F080)
    void UpdateTimers();                                        // v12 0x00456F70 (feb 0x0044E610)
    int  GetLayer3State();                                      // v12 0x0045CD90 (shared)
    void SetLayer3State(int s);                                 // v12 0x0045A950 (shared)
    void AimPjlToPlayer(m3dV *from, struct pjlPJL_INIT *init);  // v12 0x00447840 (feb 0x0043FD10)
    virtual void LaunchOneProjectile(objOBJ *from);             // [116] +0x1D0 v12 0x00447AD0 (feb 0x0043FFA0)
    virtual void LaunchSeveralProjectiles(objOBJ *from);        // [117] +0x1D4 v12 0x00447C70 (feb 0x00440170)
    virtual sncSOUND_DESCR_3D *GetSound(int i);                 // [118] +0x1D8 v12 0x00446090 the boss's or a mini's
};
int __fastcall aiGefestInit();                                  // v12 0x00445070 (feb 0x0043D790)

// ================================================================================================
// aiZEVS : aiNPC_WR   registered as "ai_zevs"
//   Zeus (final boss): bash/blow attacks, lightning bolts pjl_zevs aimed at '&dom_ai_zevs_bolts' targets, death SFX (stars/lightning).
//   Health 30000 in 1.2 (10000 in feb).
//   Create v12 0x004616B0 (feb 0x00458170): operator new(0x737) (feb 0x70F); vtable v12 0x00600058 (feb 0x005D4C80), 116 slots (feb 113)
//   SetupAniSystem v12 0x0045F9A0 (feb 0x004566A0 aiZEVS::SetupAniSystem): anim sequence slots in order:
//     0=ZEVS_SEQ_BREATHE 1=ZEVS_SEQ_BASH 2=ZEVS_SEQ_LOOK 3=ZEVS_SEQ_BLOW 4=ZEVS_SEQ_TURN 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x004612B0 ProcessMP_GET_SYNC_DATA | +0x038 0x004613D0 ProcessMP_WRITE_SYNC_DATA | +0x03C 0x00461480 ProcessMP_READ_SYNC_DATA
//     +0x040 0x0045F9A0 SetupAniSystem | +0x064 0x00460550 ParseNameClass | +0x080 0x004603F0 PlaySounds
//     +0x088 0x00460120 StartDeath | +0x098 0x00460150 SelectGoal | +0x0A4 0x0045FBD0 SetMovementSpeed
//     +0x0B0 0x0043F8E0 MoveToGoal* | +0x0C8 0x00460180 EvalAnimGoalSeq | +0x0D0 0x00460740 ProcessActionFrame
//     +0x0EC 0x00460600 Think | +0x0F4 0x00460250 UpdateTimers | +0x124 0x0045CD90 GetLayer3State*
//     +0x130 0x0045A950 SetLayer3State* | +0x140 0x0045FBE0 ProcessINIT | +0x144 0x0045FFF0 ProcessTERM
//     +0x148 0x0045FEB0 ProcessINIT_LEVEL | +0x14C 0x00448F00 ProcessFRAME | +0x150 0x00460030 ProcessCDT_FIREABLE
//     +0x15C 0x00461270 ProcessMP_NOTIFY | +0x178 0x0045FF60 Activate | +0x1C4 0x00460C20 IsArmoredObject
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        feb only: feb +0x0D0 = 10000f (removed in 1.2)
//     ProcessINIT        feb only: feb +0x0CC = 10000f (removed in 1.2)
//     ProcessINIT        fallbackAngle        = 10000f     v12 +0x553 @0x0045FC53
//     ProcessINIT        radActivate          = 300f       v12 +0x4CB @0x0045FC19
//     ProcessINIT        radActivateBack      = 300f       v12 +0x4CF @0x0045FC1F
//     ProcessINIT        maxHealth            = 30000f     v12 +0x0D4 @0x0045FC35   CHANGED, feb 15f
//     ProcessINIT        health               = 30000f     v12 +0x0D0 @0x0045FC3F   CHANGED, feb 15f
//     ProcessINIT        heightCDT            = 40f        v12 +0x18F @0x0045FC49
//     ProcessINIT        alertAngle           = 180f       v12 +0x19B @0x0045FC5D
//     ProcessINIT        maxTurnStep          = 70f        v12 +0x17B @0x0045FC67
//     ProcessINIT        f_547                = 5f         v12 +0x56F @0x0045FC71   CHANGED, feb 0
//     ProcessINIT        timeDeactivate       = 3600f      v12 +0x4D7 @0x0045FC7B
//     ProcessINIT        corpseFadeTime       = 0.333333f  v12 +0x4BF @0x0045FC85
//     ProcessINIT        corpseTime           = 1.66667f   v12 +0x4BB @0x0045FC8F
//     ProcessINIT        f_543                = 0          v12 +0x56B @0x0045FD87
//     ProcessINIT        f_54F                = 30000f     v12 +0x577 @0x0045FE73
//     ProcessINIT        radCDT               = 15f        v12 +0x187 @0x0045FC02   1.2 NEW
//     ProcessINIT        heightLowTester      = 15f        v12 +0x18B @0x0045FC08   1.2 NEW
//     health 30000: maxHealth @0x0045FC35, health @0x0045FC3F (aiZEVS::ProcessINIT)
//   spawned entities:
//     "pjl_zevs" Create v12 0x00461800 (feb 0x004582D0) size 0x1CC (feb 0x1A8) vt v12 0x00600230 (feb 0x005D4E50)
//     "sfx_zeus_lgtn" Create v12 0x00486490 (feb 0x0047BFD0) size 0x1A7 (feb 0x183) vt v12 0x006037F0 (feb 0x005D8418)
// ================================================================================================
class aiZEVS : public aiNPC_WR {         // src/Sources/ai_zevs.cpp
public:
    u32        zevsFlags;            // v12 +0x567 (feb +0x53F) 0x100 the bolts' area made, 0x10000 a "_mini" Zeus
    sncCHANNEL_CONE *voice;          // v12 +0x56B (feb +0x543) his own sound channel (a sphere, 360 m)
    float      boltTimer;            // v12 +0x56F (feb +0x547) to the next bolt of a BASH's series
    float      blowTimer;            // v12 +0x573 (feb +0x54B) BLOW (2) chosen while it runs
    float      fadeIn;               // v12 +0x577 (feb +0x54F) the appearing: 3 -> 0 (alpha 255 - 84 * it)
    objOBJ    *attackFocus;          // v12 +0x57B (feb +0x553) "ATTACKfocus"
    objOBJ    *garp;                 // v12 +0x57F (feb +0x557) "Garp_lod_8": where a series' bolts come from
    int        boltsLeft;            // v12 +0x583 (feb +0x55B) bolts of this series still to come
    objOBJ    *emitter;              // v12 +0x587 (feb +0x55F) "emitter*": the lightning round him (death SFX)
    animINST  *roofs[5];             // v12 +0x58B (feb +0x563) "Actor_roof_break_01".. : break as his health goes
    m3dV       boltFrom;             // v12 +0x59F (feb +0x577)
    aiAREA     boltArea;             // v12 +0x5AB (feb +0x583) "&dom_ai_zevs_bolts": where the bolts land
    // size v12 0x737 (feb 0x70F)
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x004616B0 (feb 0x00458170)
    BOOL32 SetupAniSystem();             // +0x040 v12 0x0045F9A0
    void ParseNameClass();               // +0x064 v12 0x00460550
    void FillSoundFlags();               // +0x07C v12 0x00460320
    void PlaySounds();                   // +0x080 v12 0x004603F0
    void StartDeath(dmgDAMAGE *d);       // +0x088 v12 0x00460120
    int  SelectGoal();                   // +0x098 v12 0x00460150
    void SetMovementSpeed();             // +0x0A4 v12 0x0045FBD0
    int  EvalAnimGoalSeq();              // +0x0C8 v12 0x00460180
    void ProcessActionFrame();           // +0x0D0 v12 0x00460740
    void Think();                        // +0x0EC v12 0x00460600
    void UpdateTimers();                 // +0x0F4 v12 0x00460250
    int  ProcessINIT(void *init);        // +0x140 v12 0x0045FBE0
    void ProcessTERM();                  // +0x144 v12 0x0045FFF0
    void ProcessINIT_LEVEL();            // +0x148 v12 0x0045FEB0
    int  ProcessCDT_FIREABLE(dmgDAMAGE *d); // +0x150 v12 0x00460030
    int  Activate(int force);            // +0x178 v12 0x0045FF60
    void StartMainSFX();                 // v12 0x00460050
    void MakeRoofsIgnoreCDT();           // v12 0x00460680
    void LaunchOneBolt();                // v12 0x00460A30 -- not rebuilt
    void LaunchOneProjectile();          // v12 0x004607B0 -- not rebuilt
};
int __fastcall aiZevsInit();             // v12 0x0045F910 (feb 0x00456610)

// ================================================================================================
// aisATLAS : aisSTATUE   registered as "ai_atlas"
//   Atlas statue (aisSTATUE): raises and throws the globe (atlas_ball rigid body, AI_PROP timeLifeBall), shatters when destroyed.
//   Create v12 0x00430A80 (feb 0x0042B450): operator new(0x5EB) (feb 0x5C3); vtable v12 0x005FBA30 (feb 0x005D0CA0), 121 slots (feb 119)
//   SetupAniSystem v12 0x0042F630 (feb 0x0042A360 aisATLAS::SetupAniSystem): anim sequence slots in order:
//     0=ATLAS_SEQ_RAISE_BALL 1=ATLAS_SEQ_THROW 2=ATLAS_SEQ_APPEAR_NEW 3=ATLAS_SEQ_SIT_DOWN 4=ATLAS_SEQ_JUMP_DOWN 
//     5=ATLAS_SEQ_WALK 
//   class-specific instance keys (section key -> v12 code address):
//     AI_PROP timeLifeBall  @0x0042FC2C
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x0042F630 SetupAniSystem | +0x064 0x0042FB70 ParseNameClass
//     +0x088 0x00430830 StartDeath* | +0x098 0x0042FE20 SelectGoal | +0x0A4 0x00430130 SetMovementSpeed
//     +0x0C8 0x0042FEB0 EvalAnimGoalSeq | +0x0CC 0x00430B30 GetAnimPlayRate | +0x0D0 0x0042FF60 ProcessActionFrame
//     +0x0D4 0x004301B0 AnimateProcedural | +0x0D8 0x00434790 MoveRootedPivoted* | +0x0E0 0x00434920 GetPlayerTargetPos*
//     +0x0EC 0x004300F0 Think | +0x0F4 0x00430300 UpdateTimers* | +0x110 0x00434820 UpdateOnServerOnly*
//     +0x114 0x00434850 UpdateOnClientOnly* | +0x118 0x00434860 UpdateOnServerAndClient* | +0x120 0x0045CD90 GetLayer3State*
//     +0x124 0x00430B40 GetLayer3State* | +0x12C 0x0045A950 SetLayer3State* | +0x130 0x00430B50 SetLayer3State*
//     +0x140 0x0042F840 ProcessINIT | +0x144 0x0042FCA0 ProcessTERM | +0x150 0x00431810 ProcessCDT_FIREABLE
//     +0x158 0x00434800 ProcessASK_KEEP_LEVEL_RELOAD* | +0x15C 0x00430A10 ProcessMP_NOTIFY* | +0x160 0x00434810 ProcessIACTION*
//     +0x178 0x0042FE80 Activate | +0x1C4 0x00403200 IsArmoredObject | +0x1D0 0x004348F0 Shoot*
//     +0x1D4 0x0042FE60 ProcessShoot | +0x1D8 0x00434950 Colorize* | +0x1DC 0x00401380 UpdateSound
//     +0x1E0 0x004309A0 GetShatterRigid | +0x1E4 ? CheckPlrVisible
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        maxHealth            = 60f        v12 +0x0D4 @0x0042F863
//     ProcessINIT        radActivate          = 150f       v12 +0x4CB @0x0042F86D
//     ProcessINIT        radActivateBack      = 50f        v12 +0x4CF @0x0042F877
//     ProcessINIT        f_583                = 180f       v12 +0x5AB @0x0042F881
//     ProcessINIT        alertAngle           = 120f       v12 +0x19B @0x0042F88B
//     ProcessINIT        f_5A3                = 1.66667f   v12 +0x5CB @0x0042F895
//     ProcessINIT        f_58F                = 70f        v12 +0x5B7 @0x0042F8B1
//     ProcessINIT        aimAngle             = 20f        v12 +0x1A3 @0x0042F92C
//     ProcessINIT        fallbackAngle        = 10000f     v12 +0x553 @0x0042F943
//     ProcessINIT        navArriveDist        = 20f        v12 +0x1B3 @0x0042F932
//     ProcessINIT        f_597                = 0          v12 +0x5BF @0x0042F973
//     ProcessINIT        f_5A7                = 0          v12 +0x5CF @0x0042F990
//     ProcessINIT        f_59F                = 0          v12 +0x5C7 @0x0042F996
//     ProcessINIT        bloodObj             = 0          v12 +0x54F @0x0042F9CF
//     ProcessINIT        f_593                = 0          v12 +0x5BB @0x0042FABD
//     ProcessINIT        f_5BB                = 5f         v12 +0x5E3 @0x0042FB2A
//     ProcessINIT        heightUp1            = 4.5f       v12 +0x3A3 @0x0042F94D   1.2 NEW
//     ProcessINIT        radCDT               = 1.2f       v12 +0x187 @0x0042F957   1.2 NEW
//     ProcessINIT        heightLowTester      = 1.2f       v12 +0x18B @0x0042F95D   1.2 NEW
//     ProcessINIT        heightCDT            = 3.3f       v12 +0x18F @0x0042F963   1.2 NEW
//     ParseNameClass     radActivate          = 250f       v12 +0x4CB @0x0042FBEB
//     ParseNameClass     radActivateBack      = 250f       v12 +0x4CF @0x0042FBF3
//     ParseNameClass     f_587                = 0.5f       v12 +0x5AF @0x0042FB96
//     ParseNameClass     feb only: feb +0x58B = 40f (removed in 1.2)
//     ParseNameClass     shootRange           = 40f        v12 +0x1A7 @0x0042FBA7
//     ParseNameClass     shootRange           = 60f        v12 +0x1A7 @0x0042FBB3   1.2 NEW
//     ParseNameClass     shootRange           = 60f        v12 +0x1A7 @0x0042FBFF   1.2 NEW
//     ParseNameClass     radActivate          = 250f       v12 +0x4CB @0x0042FC53   1.2 NEW
//     ParseNameClass     radActivateBack      = 250f       v12 +0x4CF @0x0042FC5C   1.2 NEW
//     ParseNameClass     shootRange           = 60f        v12 +0x1A7 @0x0042FC68   1.2 NEW
//   spawned entities:
//     "atlas_ball" Create v12 0x00435C50 (feb 0x0042FDB0) size 0xF47 (feb 0xF27) vt v12 0x005FC758 (feb 0x005D18F8)
//     "sfx_atlas_globe_appear" Create v12 0x00469690 (feb 0x0045F8C0) size 0x183 (feb 0x167) vt v12 0x00600C10 (feb 0x005D57E0)
//     "sfx_atlas_globe_land" Create v12 0x00469710 (feb 0x0045F940) size 0x17B (feb 0x15F) vt v12 0x00600C78 (feb 0x005D5848)
// ================================================================================================
class aisATLAS : public aisSTATUE {
public:
    u32        f_597;                 // v12 +0x5BF (feb +0x597) init 0
    u32        f_59B;                 // v12 +0x5C3 (feb +0x59B)  // ? offset by shift
    u32        f_59F;                 // v12 +0x5C7 (feb +0x59F) init 0
    float      f_5A3;                 // v12 +0x5CB (feb +0x5A3) init 1.66667f
    u32        f_5A7;                 // v12 +0x5CF (feb +0x5A7) init 0
    objOBJ*    f_5AB;                 // v12 +0x5D3 (feb +0x5AB)  // ? offset by shift
    objOBJ*    f_5AF;                 // v12 +0x5D7 (feb +0x5AF)  // ? offset by shift
    objOBJ*    f_5B3;                 // v12 +0x5DB (feb +0x5B3)  // ? offset by shift
    objOBJ*    f_5B7;                 // v12 +0x5DF (feb +0x5B7)  // ? offset by shift
    float      f_5BB;                 // v12 +0x5E3 (feb +0x5BB) init 5f
    entENTITY* f_5BF;                 // v12 +0x5E7 (feb +0x5BF)  // ? offset by shift
    // size v12 0x5EB (feb 0x5C3)
};

// ================================================================================================
// aisDISKBL : aisSTATUE   registered as "ai_diskobolus"
//   Discobolus statue: throws diskbl_disk discs, jumps down from its pedestal, shatters.
//   Create v12 0x00431DF0 (feb 0x0042C450): operator new(0x5E3) (feb 0x5BB); vtable v12 0x005FBC78 (feb 0x005D0EA8), 121 slots (feb 118)
//   SetupAniSystem v12 0x00430DE0 (feb 0x0042B5E0 aisDISKBL::SetupAniSystem): anim sequence slots in order:
//     0=DISKBL_SEQ_THROW 1=DISKBL_SEQ_SIT_DOWN 2=DISKBL_SEQ_RUN 3=DISKBL_SEQ_JUMP 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00430DE0 SetupAniSystem | +0x064 0x00431230 ParseNameClass
//     +0x088 0x004349F0 StartDeath* | +0x098 0x00431440 SelectGoal | +0x0A4 0x00431660 SetMovementSpeed
//     +0x0C8 0x00431460 EvalAnimGoalSeq | +0x0D0 0x00431500 ProcessActionFrame | +0x0D4 0x004316C0 AnimateProcedural
//     +0x0D8 0x00434790 MoveRootedPivoted* | +0x0E0 0x00434920 GetPlayerTargetPos* | +0x0EC 0x004315F0 Think
//     +0x0F4 0x00431830 UpdateTimers | +0x110 0x00434820 UpdateOnServerOnly* | +0x114 0x00434850 UpdateOnClientOnly*
//     +0x118 0x00434860 UpdateOnServerAndClient* | +0x120 0x0045CD90 GetLayer3State* | +0x124 0x00430B40 GetLayer3State*
//     +0x12C 0x0045A950 SetLayer3State* | +0x130 0x00430B50 SetLayer3State* | +0x140 0x00430F80 ProcessINIT
//     +0x144 0x00431260 ProcessTERM | +0x150 0x00431810 ProcessCDT_FIREABLE | +0x158 0x00434800 ProcessASK_KEEP_LEVEL_RELOAD*
//     +0x15C 0x00430A10 ProcessMP_NOTIFY | +0x160 0x00434810 ProcessIACTION* | +0x178 0x00434880 Activate*
//     +0x1C4 0x00403200 IsArmoredObject | +0x1D0 0x004348F0 Shoot* | +0x1D8 0x00434950 Colorize*
//     +0x1DC 0x00401380 UpdateSound | +0x1E0 0x00431DC0 GetShatterRigid
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        maxHealth            = 50f        v12 +0x0D4 @0x00430FA9
//     ProcessINIT        radActivate          = 200f       v12 +0x4CB @0x00430FAF
//     ProcessINIT        radActivateBack      = 50f        v12 +0x4CF @0x00430FB9
//     ProcessINIT        f_583                = 240f       v12 +0x5AB @0x00430FBF
//     ProcessINIT        alertAngle           = 120f       v12 +0x19B @0x00430FC9
//     ProcessINIT        f_587                = 0.5f       v12 +0x5AF @0x00430FD3
//     ProcessINIT        f_58B                = 90f        v12 +0x5B3 @0x00430FDD
//     ProcessINIT        moveSpeed            = 8f         v12 +0x16F @0x00430FE7
//     ProcessINIT        f_58F                = 70f        v12 +0x5B7 @0x00430FED
//     ProcessINIT        aimAngle             = 20f        v12 +0x1A3 @0x0043104D
//     ProcessINIT        fallbackAngle        = 10000f     v12 +0x553 @0x00431057
//     ProcessINIT        f_597                = 0          v12 +0x5BF @0x00431067
//     ProcessINIT        f_59B                = 0          v12 +0x5C3 @0x0043106D
//     ProcessINIT        f_59F                = 0          v12 +0x5C7 @0x00431073
//     ProcessINIT        f_5A3                = 0          v12 +0x5CB @0x00431079
//     ProcessINIT        f_5A7                = 0          v12 +0x5CF @0x0043107F
//     ProcessINIT        f_5AB                = 0          v12 +0x5D3 @0x00431085
//     ProcessINIT        bloodObj             = 0          v12 +0x54F @0x004310E8
//     ProcessINIT        f_593                = 0          v12 +0x5BB @0x004311D3
//     ProcessINIT        radCDT               = 1.4f       v12 +0x187 @0x0043108B   1.2 NEW
//     ProcessINIT        heightCDT            = 2.5f       v12 +0x18F @0x00431095   1.2 NEW
//     ProcessINIT        heightUp1            = 4.2f       v12 +0x3A3 @0x0043109F   1.2 NEW
//     ProcessINIT        navArriveDist        = 8f         v12 +0x1B3 @0x004310A9   1.2 NEW
//   spawned entities:
//     "diskbl_disk" Create v12 0x00436860 (feb 0x004309C0) size 0x1C4 (feb 0x1A8) vt v12 0x005FC828 (feb 0x005D19C8)
//     "sfx_discus_land" Create v12 0x00472250 (feb 0x00468580) size 0x183 (feb 0x167) vt v12 0x00601A58 (feb 0x005D6638)
// ================================================================================================
class aisDISKBL : public aisSTATUE {
public:
    u32        f_597;                 // v12 +0x5BF (feb +0x597) init 0
    u32        f_59B;                 // v12 +0x5C3 (feb +0x59B) init 0
    u32        f_59F;                 // v12 +0x5C7 (feb +0x59F) init 0
    u32        f_5A3;                 // v12 +0x5CB (feb +0x5A3) init 0
    u32        f_5A7;                 // v12 +0x5CF (feb +0x5A7) init 0
    u32        f_5AB;                 // v12 +0x5D3 (feb +0x5AB) init 0
    objOBJ*    f_5AF;                 // v12 +0x5D7 (feb +0x5AF)  // ? offset by shift
    objOBJ*    f_5B3;                 // v12 +0x5DB (feb +0x5B3)  // ? offset by shift
    objOBJ*    f_5B7;                 // v12 +0x5DF (feb +0x5B7)  // ? offset by shift
    // size v12 0x5E3 (feb 0x5BB)
};

// ================================================================================================
// aisPERSEUS : aisSTATUE   registered as "ai_perseus"
//   Perseus statue: destroyable scripted statue (AI_STATUE isActive), sig '$DST' type 0x28.
//   Create v12 0x004324C0 (feb 0x0042CA50): operator new(0x5CF) (feb 0x5A7); vtable v12 0x005FBE78 (feb 0x005D1088), 121 slots (feb 118)
//   SetupAniSystem v12 0x004320F0 (feb 0x0042C720 aisPERSEUS::SetupAniSystem): anim sequence slots in order:
//     0=PERSEUS_SEQ_DESTROY 
//   class-specific instance keys (section key -> v12 code address):
//     AI_STATUE isActive  @0x0043207E
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x004 0x004323E0 ProcessMsg | +0x040 0x004320F0 SetupAniSystem | +0x064 0x00434730 ParseNameClass*
//     +0x088 0x004322F0 StartDeath | +0x098 0x004348C0 SelectGoal* | +0x0A4 0x004349D0 SetMovementSpeed*
//     +0x0D8 0x00434790 MoveRootedPivoted* | +0x0E0 0x00434920 GetPlayerTargetPos* | +0x0F4 0x004348E0 UpdateTimers*
//     +0x110 0x00434820 UpdateOnServerOnly* | +0x114 0x00434850 UpdateOnClientOnly* | +0x118 0x00434860 UpdateOnServerAndClient*
//     +0x120 0x0045CD90 GetLayer3State* | +0x12C 0x0045A950 SetLayer3State* | +0x140 0x00431F50 ProcessINIT
//     +0x144 0x00432290 ProcessTERM | +0x14C 0x00432210 ProcessFRAME | +0x150 0x004322C0 ProcessCDT_FIREABLE
//     +0x158 0x00434800 ProcessASK_KEEP_LEVEL_RELOAD* | +0x160 0x00434810 ProcessIACTION* | +0x164 0x00432350 ProcessDESTROY_ENT
//     +0x178 0x00434880 Activate* | +0x1C4 0x00403200 IsArmoredObject | +0x1D0 0x004348F0 Shoot*
//     +0x1D8 0x00434950 Colorize* | +0x1E0 0x00434AB0 GetShatterRigid*
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        maxHealth            = 25f        v12 +0x0D4 @0x00432067
//     ProcessINIT        health               = 25f        v12 +0x0D0 @0x0043206D
//     ProcessINIT        turnSpeed            = 0          v12 +0x177 @0x0043208E
//     ProcessINIT        fallbackAngle        = 10000f     v12 +0x553 @0x00432098
//   spawned entities:
//     "sfx_perseus" Create v12 0x0047F010 (feb 0x00474CB0) size 0x1BB (feb 0x187) vt v12 0x00602C60 (feb 0x005D7858)
// ================================================================================================
class aisPERSEUS : public aisSTATUE {
public:
    u32        f_597;                 // v12 +0x5BF (feb +0x597) init 0
    u32        f_59B;                 // v12 +0x5C3 (feb +0x59B) init 0
    objOBJ*    f_59F;                 // v12 +0x5C7 (feb +0x59F)
    wrsfxPERSEUS* f_5A3;                 // v12 +0x5CB (feb +0x5A3)
    // size v12 0x5CF (feb 0x5A7)
};

// ================================================================================================
// aisSPHINX : aisSTATUE   registered as "ai_sphinx"
//   Sphinx statue: shoots pjl_sphinx, breaks into parts (aisSPHINX_PARTS/SMOKE), sig '$DST' type 0x28.
//   Create v12 0x004340B0 (feb 0x0042E4C0): operator new(0x5E7) (feb 0x5BF); vtable v12 0x005FC0E8 (feb 0x005D12C8), 121 slots (feb 118)
//   SetupAniSystem v12 0x00433620 (feb 0x0042DC20 aisSPHINX::SetupAniSystem): anim sequence slots in order:
//     0=SPHINX_SEQ_ATTACK 1=SPHINX_SEQ_IDLE 
//   class virtual overrides (v12 slot offset, v12 impl, name; * = code shared with other classes / generic parent):
//     +0x034 0x00433790 ProcessMP_GET_SYNC_DATA* | +0x040 0x00433620 SetupAniSystem | +0x064 0x00434730 ParseNameClass*
//     +0x080 0x00433DE0 PlaySounds | +0x088 0x00433300 StartDeath | +0x098 0x004348C0 SelectGoal*
//     +0x0A4 0x004335E0 SetMovementSpeed | +0x0C8 0x00433610 EvalAnimGoalSeq | +0x0CC 0x00433600 GetAnimPlayRate
//     +0x0D8 0x00434790 MoveRootedPivoted* | +0x0E0 0x00434920 GetPlayerTargetPos* | +0x0EC 0x004337D0 Think
//     +0x0F4 0x00430300 UpdateTimers | +0x110 0x00434820 UpdateOnServerOnly* | +0x114 0x00434850 UpdateOnClientOnly*
//     +0x118 0x00434860 UpdateOnServerAndClient* | +0x120 0x0045CD90 GetLayer3State* | +0x124 0x00430B40 GetLayer3State
//     +0x12C 0x0045A950 SetLayer3State* | +0x130 0x00430B50 SetLayer3State | +0x140 0x004333F0 ProcessINIT
//     +0x144 0x004335B0 ProcessTERM | +0x148 0x00433730 ProcessINIT_LEVEL | +0x14C 0x0043F720 ProcessFRAME
//     +0x150 0x004332D0 ProcessCDT_FIREABLE | +0x158 0x00434800 ProcessASK_KEEP_LEVEL_RELOAD* | +0x15C 0x004337A0 ProcessMP_NOTIFY
//     +0x160 0x00434810 ProcessIACTION* | +0x178 0x00434880 Activate* | +0x1C4 0x00403200 IsArmoredObject
//     +0x1D0 0x004348F0 Shoot* | +0x1D8 0x00434950 Colorize* | +0x1E0 0x00434AB0 GetShatterRigid*
//   TWEAK constants written by the init code (patch the imm32 / register load at the v12 address):
//     ProcessINIT        radActivate          = 96f        v12 +0x4CB @0x0043350F
//     ProcessINIT        radActivateBack      = 24f        v12 +0x4CF @0x00433519
//     ProcessINIT        f_583                = 115.2f     v12 +0x5AB @0x00433523
//     ProcessINIT        maxHealth            = 25f        v12 +0x0D4 @0x0043352D
//     ProcessINIT        health               = 25f        v12 +0x0D0 @0x00433533
//     ProcessINIT        shootRange           = 30f        v12 +0x1A7 @0x00433539
//     ProcessINIT        aimAngle             = 20f        v12 +0x1A3 @0x00433543
//     ProcessINIT        f_597                = 0          v12 +0x5BF @0x0043354D
//     ProcessINIT        f_59B                = 0          v12 +0x5C3 @0x00433553
//     ProcessINIT        f_59F                = 0          v12 +0x5C7 @0x00433559
//     ProcessINIT        f_5A3                = 0          v12 +0x5CB @0x0043355F
//     ProcessINIT        fallbackAngle        = 10000f     v12 +0x553 @0x00433585
//   spawned entities:
//     "pjl_sphinx" Create v12 0x00434130 (feb 0x0042E540) size 0x1B8 (feb 0x19C) vt v12 0x005FC2D0 (feb 0x005D14A0)
//     "sfx_sphinx_pjl_expl" Create v12 0x00483DF0 (feb 0x00479A60) size 0x177 (feb 0x15B) vt v12 0x00603360 (feb 0x005D7F68)
// ================================================================================================
class aisSPHINX : public aisSTATUE {
public:
    u32        f_597;                 // v12 +0x5BF (feb +0x597) init 0
    u32        f_59B;                 // v12 +0x5C3 (feb +0x59B) init 0
    u32        f_59F;                 // v12 +0x5C7 (feb +0x59F) init 0
    int        f_5A3;                 // v12 +0x5CB (feb +0x5A3) init 0
    u32        f_5A7;                 // v12 +0x5CF (feb +0x5A7)  // ? offset by shift
    float      f_5AB;                 // v12 +0x5D3 (feb +0x5AB)  // ? offset by shift
    u32        f_5AF;                 // v12 +0x5D7 (feb +0x5AF)  // ? offset by shift
    msgADDR*   f_5B3;                 // v12 +0x5DB (feb +0x5B3)
    objOBJ*    f_5B7;                 // v12 +0x5DF (feb +0x5B7)
    objOBJ*    f_5BB;                 // v12 +0x5E3 (feb +0x5BB)  // ? offset by shift
    // size v12 0x5E7 (feb 0x5BF)
};

#pragma pack(pop)
