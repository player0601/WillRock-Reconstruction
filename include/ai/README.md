# Subsystem "ai": enemies

Unofficial SDK for Will Rock 1.2 (`v12.exe`). The reference is the Feb 2003 beta (`feb.exe`, with a map file).
Static analysis only.

| file | content |
|---|---|
| `ai_npc.h` | the framework: `aiNPC`, `aiNPC_WR`, `aiNPC_WR_FLY`, `aiENEMY`, flags, the full vtable (116 slots) with v12/feb addresses, the field layout, globals |
| `ai_world.h` | world-level AI: `aiPLANNER` (attack director), spawn domains/areas, waves, arenas, and the level-object naming they rely on |
| `enemies.h` | one block per enemy class (23 classes, 25 registration names): factory, size, vtable, animation sequences, keys, overrides, the init constants with their v12 store address, spawned projectiles/effects, own fields |
| `FEB_VS_12.md` | what changed for every enemy between the beta and 1.2, with addresses |
| `function_index.tsv` | all 1239 feb functions of the AI objs with their v12 twin (892 found) and how each twin was found |

Assumed from `re/sdk/core` / `engine_core`: `msgSYSTEM`/`msgADDR`/`msgDATA`, `entIACTIVE_OBJ`/`entENTITY` (`entity.h`),
`animINST`, `objOBJ`, `m3dV`/`m3dMATR`, `cdtINFO`, `scnDOMAIN`/`gsDOMAIN`, `psSHEET`, `dmgDAMAGE` (`game_shared/damage.h`),
`physMATH`, `sncCHANNEL`.

Work files (scripts that rebuild every table here, plus the raw comparison output) are in `re/work/sdk_ai/`.

---------------------------------------------------------------------------------------------------------------

## 1. How the AI works

### Class tree
```
entENTITY
 └─ aiNPC                      gs_shared:ai_ctrl*.obj   vtable v12 0x00608878  size v12 0x543 (feb 0x51B)
     └─ aiNPC_WR               ai_npc_wr.obj            (abstract, no vtable)  size 0x567
         ├─ aiNPC_WR_FLY        ai_npc_wr_fly.obj        size 0x56F   -> aiBIRD, aiAMUR_GENERIC, aiCROC, aiMEDUSA
         ├─ aisSTATUE           ais_statue.obj           -> aisATLAS, aisDISKBL, aisPERSEUS, aisSPHINX
         ├─ aiGLAD_GENERIC      -> aiGLAD_KNIFE, aiGLAD_MACE
         ├─ aiSKEL_GENERIC      -> aiSKEL_SPEAR ("ai_skel"), aiSKEL_SWORD ("ai_skel_kamik")
         ├─ aiCENT              -> aiSKETAVR
         ├─ aiMINO              -> aiMINO_ATRIUM
         └─ aiRAT, aiSATYR, aiTIGER ("ai_tiger"+"ai_lion"), aiCERBER_GENERIC, aiCYCLOP, aiGEFEST_GENERIC, aiZEVS
```
Every enemy is registered with `entRegisterClass("ai_xxx", '$NPC', 0x32, Create, group, 0)` in its `aiXxxInit()`.
`wrAPP_SYSTEM::InitSubsys` calls all of these at start-up. A level instance whose class name starts with the
registered name, for example `ai_tiger_arena_01`, is built by that factory. **The rest of the class name is
parsed for tags** (`ParseNameClass`, see section 3).

### Lifecycle
1. **Create** (`aiXxx::Create`): `operator new(size)`, then `aiNPC::aiNPC` (v12 0x00547A60) sets the defaults
   listed in `ai_npc.h`, then the class fields and the vtable.
2. **INIT** (msg, `ProcessINIT` chain: class -> `aiNPC_WR::ProcessINIT` v12 0x00452D70 -> `aiNPC::ProcessINIT`
   v12 0x00548120):
   * The base reads the `AI_PROP` keys and calls `vslot 25 ParseNameClass`.
   * If it was spawned by a spawn domain (`aiINIT_DATA`), it takes its position from
     `aiDOMAIN_SPAWN::GetSpawnPosDir`.
   * The NPC starts **inactive** (`AIF_INACTIVE`) and declares itself MP-syncable.
   * aiNPC_WR creates the passive-physics object (`physDRIFT_MATH`) and reads Aiming / ObstAvoid / isFallback /
     isWayPTurn.
   * The class sets its own constants: health, speeds, radii. These are the TWEAK constants in `enemies.h`.
     They are written **after** the base read the instance keys, so a class constant overrides any
     `AI_PROP` key for the same field.
   * The class creates its voice channel (`sncCHANNEL_SPH`) and finds bones such as "ROOT".
3. **INIT_LEVEL**: `SettleSurface`/`Colorize`, position history, facing.
4. **Activation.** An inactive NPC becomes active (`vslot 94 Activate`: clears `AIF_INACTIVE`, sets
   `AIF_WAS_ACTIVATED`, registers with `aiAttackPlanner`) when one of these happens:
   * it can see its enemy and `IsReadyActivate()` returns true. Not possible with `isPlrNoVisibility`.
     Hidden altar opponents are not ready either;
   * it is hit (`ProcessCDT_FIREABLE`);
   * it receives the IAction `ACTIVATE`/`UNLOCK` while unlocked, or `ACTIVATE_PLR_VISIBILITY`.

   `Think` deactivates it again (`CheckDeactivateCondition`) when the enemy is not seen for `timeDeactivate`, or
   is further away than 1.7x / 2x `radActivate`.
5. **Death.**
   * `ProcessCDT_FIREABLE` subtracts the damage (NPC-on-NPC damage x0.3), calls `NotifyHealthChange`, and when
     health <= 0 calls `StartDeath` (vslot 34): `AIF_DEAD`, NPC-death broadcast (msg 0x4CA; feb 0x4C8), stats.
   * `UpdateDead` runs the corpse timer: `corpseTime` 5 s, then `corpseFadeTime` 4 s, then the NPC is removed.
   * aiNPC_WR adds blood (`StartBlood`, which honours `wrAppState == 2`, no-blood builds), fall-back
     (knock-back) animations, and gibbing (`DecideExplosion`/`Explode`). An explosive hit (weapon types 6, 9 or 10)
     within 1 m gibs the NPC.
6. **Inactivity death.** `aiNPC::DieOfInactivity` (v12 0x0054C110): an NPC that was activated but stays inactive
   for 20 s (30 s while a boss is registered) sends itself 5000 special damage. This removes NPCs that got stuck.
7. **1.2: out-of-level removal.** An NPC whose position leaves the scene box (+50) is deleted (FEB_VS_12 F3).

### Per frame (`aiNPC::ProcessFRAME` v12 0x00548760)
```
if cinema active (and not boss) -> return                   (1.2: bosses keep running, F4)
if AIF_AI_OFF -> return
if [Debug] Decomp_OFF_AI (or aiDbgAllOff) and this != aiDbgSelectedNPC and authoritative -> return
authoritative (entFlags&4) ? vslot 68 UpdateOnServerOnly : vslot 69 UpdateOnClientOnly
vslot 70 UpdateOnServerAndClient                               (animation, sound, colour)

UpdateOnServerOnly (v12 0x0054F440):
  DecideEnemy                 every 6 frames (10 when inactive): ShouldChangeEnemy -> SelectNewEnemy, IsPlrVisible
  FillToPlayerCoords          enemyDist / enemyAngle
  no boss -> DieOfInactivity
  1.2: outside level box -> PostMsg(MSG_MSG_TERM)
  inactive: IsReadyActivate ? Activate : return
  GetPrevPositions
  if alive:  Think (vslot 59: class AI + deactivation)
             1.2: vslot 58 CanPlanMove? else skip planning
             SelectGoal_P / SelectGoal (vslot 38) -> goalPos (planner points: attack, meet, flee, crumb)
             MoveToGoal (vslot 44) -> ClampGoalFrame (turn limits)
  Animate (vslot 49)          EvalAnimGoalSeq picks the sequence, ProcessActionFrame handles anim events
  ProcessPhysics              voluntary (walk, gravity, floor, ConstrainMotion) or passive (physDRIFT_MATH)
  if alive:  AnalyzeMove (stuck/cycle detection -> StartResolveCycle), PostThink, UpdateTimers
  else UpdateDead
```

### Perception, targeting, navigation
* **Target.** `aiENEMY` (12 bytes at v12 +0x4DF) holds the player or entity being fought. `DecideEnemy` re-evaluates
  it. In MP the planner filters players (`aiPLANNER::StartFilteringPlayers` and related).
* **Seeing.** `IsPlrVisible` does a line test from the eyes. `AIF_ENEMY_VISIBLE` is cached per decision.
  `isSniper`/`_sniper` sets `shootRange` to 800.
* **Navigation.** There is no navmesh. Each NPC steers straight to `goalPos`. Goals come from the class's
  `SelectGoal`, which uses the aiPLANNER helpers:
  * `GetAttackGoalPoint` spreads attackers around the player;
  * `GetEnemyMeetPoint` intercepts (pounce);
  * `GetFleeGoalPoint`;
  * `GetCrumbGoalPoint` follows the player's breadcrumb trail when line of sight is lost.

  Goals are clipped to the instance's `navArea` polygon (`ClipPointByNavArea`), and `navBanCross` areas are
  avoided (`RestrictGoalByBanCross`). Waypoints come from `actorWayPoint`; `"_niche"` actors wait at theirs.
  Obstacle avoidance is in `AvoidObstacles`/`StartResolveCycle`. `CheckCycle` detects oscillation using the
  position history.
* **Physics.** `ProcessVoluntaryPhysics` moves the actor by `speed`, applies `gravity`, floor-places it with the
  height testers (`radCDT`, `heightCDT`, `heightUpTester`) and calls `ConstrainMotion`. On hits and death,
  `StartPassivePhysics` switches to a physDRIFT_MATH body (knock-back, falls).
* **Flying enemies** (`aiNPC_WR_FLY`) override ProcessVoluntaryPhysics and SettleSurface. They keep a flight
  height; Medusa's comes from the `AI_MEDUSA navHeight` key.

### Attack logic and animation hookup
* `SetupAniSystem` (vslot 16) names the sequences, for example `TIGER_SEQ_GALLOP`, and builds the `animSYSTEM`
  transition table. The names are matched against the model's sequences. The tables are in `enemies.h`.
* `EvalAnimGoalSeq` (vslot 50) chooses the next sequence from distance, angle, timers and random rolls. Melee
  and launches happen in `ProcessActionFrame` (vslot 52) on the animation's **action frames**
  (`animINST::IsActionFrame(seq, n)`):
  * melee sends a damage message (0x4C6 in v12, 0x4C5 in feb, data `dmgGENERAL`) to the target, scaled by a
    reach function, for example `aiTIGER::CalcDamageToTarget`;
  * ranged attacks create a projectile entity with `entCreate("pjl_xxx", ...)` (`LaunchOneProjectile`,
    `ThrowBall`, `ThrowDisk`, `AimAxToPlayer`, `LaunchOneBolt`, ...). Projectiles are `pjlPJL<SFX>` physics
    entities; each has its own registered class, listed per enemy in `enemies.h`.
* Sounds: `FillSoundFlags` (vslot 31) sets bits in `entENTITY +0x108` (feb +0xF8). `PlaySounds` (vslot 32) plays
  the class's `sncSOUND_DESCR_3D` list (`_aiTigerSndList`, ...) on `sndChannel`.

### Spawning (see `ai_world.h`)
Level objects named `&dom_ai...` are spawn domains. Their property sheet section `AI_SPAWN` gives the class,
counts and timing (`oppClass`, `nSpawn`, `nSpawnFirst`, `nKillComplete`, `timeOppSpawn`, `timeAfterKill`,
`nMaxSimult`, `nMaxSimultSame`, `isExplicit`, `distrib`, `areaMerge`, `nPoint`, `isShowPoints`).
* `&eio_aiw...` waves pick between child domains by `AI_SPAWN ratio`.
* `&dom_arena...` arenas run staged fights.
* Spawned NPCs get `aiINIT_DATA` with their domain.
* MP clients never spawn.
* `aiSpawnSetOppCoeff(c)` (v12 0x00548DE0) scales `nMaxSimult` values of 3 or more (and in 1.2 Medusa's helper
  cap). wrGO sets it to 0.6 when `[Multiplayer] Difficulty = 0` (single player forces difficulty 1). The same
  code enables predictive aiming for difficulty >= 2 (`aiPLANNER::UsePredictiveAiming`, v12 0x004A6E55).

### Multiplayer
* NPCs simulate on the server or single player. Clients run `UpdateOnClientOnly`.
* State is replicated through the entity MP slots: `ProcessMP_GET_SYNC_DATA`, `ProcessMP_WRITE_SYNC_DATA` and
  `ProcessMP_READ_SYNC_DATA` (`aiSYNC_DATA`, 0x5C bytes, ctor v12 0x0054D740), `GetMP_UPD_STATE`, and the 1.2
  slot 8 `ProcessMP_UPD_STATE`.
* `ProcessMP_NOTIFY` events: 1 = damage stat, 2 = death stat, 3 = blood, and in 1.2 activate/deactivate.

---------------------------------------------------------------------------------------------------------------

## 2. Data the AI reads

**Instance property sheet**, section `AI_PROP`. `aiNPC::Get*` falls back to the spawn domain's sheet.

| key | type | effect |
|---|---|---|
| radCDT | float | collision radius (default 0.7; most classes overwrite it) |
| radActivate / radActivateBack | float | activation and deactivation radius (default 30 / 20; most classes use 300) |
| isSniper | int | sniper mode |
| isPlrNoVisibility | int | never activates by sight |
| actorWayPoint | str | instance name of a waypoint to walk to |
| navArea / navBanCross | str | scene object (polygon) to stay inside / never cross |
| size | float | scales the model (`animINST::Scale`) |
| timeIdleAfterSpawn | float | idle time before acting |
| heightUpTester | float | step-up height test (the level `AI_PROP` sheet is also read) |
| Aiming | str | "Plain" disables predictive aiming for this NPC |
| ObstAvoid, isFallback, isWayPTurn | bool | obstacle avoidance / knock-back / turn at waypoint |
| isNoSFXAppear, isAltarOpp | bool | no appear effect / spawns hidden behind an altar until msg 0x4D2 |
| isStatue | bool | minotaur starts as a statue |
| area | str | bird flight area object |
| timeLifeBall | float | atlas globe lifetime |

Class-specific sections: `AI_TIGER isArena`, `AI_MINO isRed/isRegen`, `AI_MEDUSA navHeight`, `AI_STATUE isActive`.
Spawn keys: `AI_SPAWN` (see above).

**Name tags** in the instance class name: `_sniper`, `_niche`, `_noappearsfx` (1.2), `_lion`, `_arena`,
`_statue`, `_gen2`, `_atrium`, `_stairs`, `_air` (1.2, atlas).

**Config.** `[Debug] Decomp_OFF_AI` (bool, read every frame). `[Multiplayer] Difficulty`.

**Animations.** The model's template must contain the sequence names of `SetupAniSystem` (in `enemies.h`).
Action-frame numbers in the animation data trigger hits and throws.

---------------------------------------------------------------------------------------------------------------

## 3. 1.2 vs feb in short (details in `FEB_VS_12.md`)
* **Layout.** `aiNPC` grew by 0x28: three new dwords plus the entity growth. 7 vtable slots were added, one removed.
* **New behaviour:**
  * a slot-58 movement gate;
  * removal of NPCs that leave the level;
  * bosses keep running during cut-scenes;
  * corpses vanish quickly after the inactivity death;
  * death unregisters from the planner.
* **Armoured parts are gone** for everyone except Cyclops, Gefest and Zeus.
* **Retuned enemies:**
  * Zeus has 30000 health (feb 10000);
  * tiger, Cerberus and crocodile have longer melee reach;
  * Cerberus no longer sprints at 19 m/s when turning;
  * the rat's self-destruct damage is halved and rats cannot be gibbed;
  * Medusa's helper spawns scale with difficulty;
  * bigger or more correct collision cylinders for the statues, minotaur, centaur, satyr and mace gladiator;
  * a coliseum Cyclops activation fix;
  * Cerberus projectile classes are renamed.
* **No new and no removed enemy classes.**

---------------------------------------------------------------------------------------------------------------

## 4. TWEAK table (v12)

**How to patch.**
* *Instance constants.* Write the float at the instruction's immediate (or the `mov reg, imm32` that feeds it).
  The ProcessINIT stores look like `c7 86 <off32> <imm32>` (imm at instruction+6) or `b8/b9/ba <imm32>` (imm at +1).
* *Defaults for all NPCs.* Patch the aiNPC ctor (v12 0x00547A60).
* *Data constants.* Patch the `.rdata` float, but other code shares it. Prefer the instruction.

Full per-class lists are in `enemies.h`.

| what | value | v12 address |
|---|---|---|
| NPC default speed / turn speed / maxTurnStep | 3.5 / 180 / 90 | ctor 0x00547A60 (`[esi+0x16F]`, `[esi+0x177]`, `[esi+0x17B]`) |
| default radActivate / radActivateBack | 30 / 20 | ctor (`[esi+0x4CB]`, `[esi+0x4CF]`) |
| default deactivate time / enemy-change time | 40 / 15 | ctor `[esi+0x4D7]`, `[esi+0x4DB]` |
| corpse stay / fade | 5 / 4 s | ctor `[esi+0x4BB]` = edx(5.0), `[esi+0x4BF]` = 4.0 |
| DecideEnemy period active / idle | 6 / 10 frames | ctor `[esi+0x49F]`, `[esi+0x4A3]` |
| sniper shoot range | 800 | 0x00548516 |
| inactivity death time (no boss / boss) | 20 / 30 s | `__real@41a00000` / `__real@41f00000` used at 0x0054C1BB / 0x0054C1AD (shared constants: patch the compare) |
| inactivity death damage | 5000 | 0x0054C1D9 (imm 0x459C4000) |
| NPC-on-NPC damage factor | 0.3 | `fld [0x5F9B18]` at 0x0054BD47 (shared constant) |
| out-of-level margin | 50 | 0x00508ED3 (also used by rigid bodies) |
| spawn opponent coefficient | global float | 0x0063721C (set via `aiSpawnSetOppCoeff` 0x00548DE0, read 0x00548E20) |
| rat health | 2 | 0x00455538 |
| satyr health | 7 | 0x004565CB |
| skeleton (spear / kamikaze) health | 2 / 2 | 0x004596F1 / 0x0045BD01 |
| gladiator health (generic) / mace | 15 / 12 | 0x004489A6 / 0x0044AB62 |
| minotaur health / run speed | 15 / 11 | 0x00450A5C / 0x00450952 |
| centaur health / speed | 70 / 20 | 0x0043AE1E / 0x0043ADA6 |
| skeleton centaur health | 30 | 0x0045C900 |
| tiger health | 50 | `mov ecx,0x42480000` at 0x0045DA89 (written to maxHealth 0x0045DA95 and health 0x0045DAE3) |
| lion health | 60 | 0x0045D55B (aiTIGER::ParseNameClass) |
| tiger bite reach (full / zero) | 1.1 / 1.7 m | 0x0045E201, 0x0045E221 (and 0x0045E355, 0x0045E36E) |
| Cerberus health / run | 50 / 9.52 | 0x0043CADD / 0x0043CAF7 |
| Cerberus paw reach / zero | 1.9 / 2.3 m | 0x0043D536 / 0x0043D562 |
| crocodile health | 50 | 0x0043F4AC |
| bird health / speed | 7 / 8 | 0x0043907E / 0x00439005 |
| amur health / speed | 7 / 6 | 0x00437451 / 0x00437461 |
| Medusa health | 6000 | 0x0044CFB0 |
| Medusa helper cap factor | 10 x coeff | `fmul [0x5F93EC]` at 0x0044E47E |
| Cyclops health | 4000 | 0x004422E2 |
| Gefest health | 5000 | 0x00445456 |
| **Zeus health** | **30000** | 0x0045FC35 (max), 0x0045FC3F (cur) |
| Atlas / Discobolus / Perseus / Sphinx health | 60 / 50 / 25 / 25 | 0x0042F863 / 0x00430FA9 / 0x00432067 / 0x0043352D |
| rat self-destruct damage | 5000 | 0x00455BF3 |
| spawn: nMaxSimult scale clamp | >= 3 | 0x005491E6 / 0x0054920C |

---------------------------------------------------------------------------------------------------------------

## 5. HOOK points (v12)

| purpose | function | address |
|---|---|---|
| add a new enemy class | `entRegisterClass(name, '$NPC', 0x32, Create, group, 0)` from a mod init (see `engine_core/entity.h`) | 0x00507D60 |
| every enemy's think | `aiNPC::ProcessFRAME` | 0x00548760 |
| server AI step | `aiNPC::UpdateOnServerOnly` (vslot +0x110) | 0x0054F440 |
| damage taken | `aiNPC::ProcessCDT_FIREABLE` (base) / `aiNPC_WR::ProcessCDT_FIREABLE` (blood, gibs) | 0x0054BD10 / 0x00453370 |
| death | `aiNPC::StartDeath` / `aiNPC_WR::StartDeath` | 0x0054BE00 / 0x00453270 |
| activation | `aiNPC::Activate` / `Deactivate` | 0x0054F190 / 0x0054F1F0 |
| per-instance setup (keys) | `aiNPC::ProcessINIT` / `aiNPC_WR::ProcessINIT` | 0x00548120 / 0x00452D70 |
| name tags | `aiNPC::ParseNameClass` / `aiNPC_WR::ParseNameClass` | 0x005484F0 / 0x00452CE0 |
| target choice | `aiNPC::DecideEnemy` | 0x0054C7B0 |
| goal points | `aiPLANNER::GetAttackGoalPoint` / `GetEnemyMeetPoint` / `GetFleeGoalPoint` / `GetCrumbGoalPoint` | 0x005517E0 / 0x00551390 / 0x00551240 / 0x00559270 |
| spawning | `aiDOMAIN_SPAWN::SpawnSingle` / `IsReadySpawn` / `Init` | 0x00549700 / 0x00549520 / 0x00548F90 |
| difficulty | `aiSpawnSetOppCoeff` / `aiPLANNER::UsePredictiveAiming` | 0x00548DE0 / 0x00552050 |
| iterate NPCs | `aiNPC::GetNext(prev, flag)` | 0x00547930 |
| per-class behaviour | vtable slots in `enemies.h`: replace a slot pointer, e.g. tiger `CalcDamageToTarget` 0x0045E0C0 is called directly; `EvalAnimGoalSeq` is slot +0xC8 | vtables |

Vtables are in `.rdata`. Patch slot pointers after making the page writable, or hook the function body.
Classes share implementations through COMDAT folding: v12 0x00460030 is used by croc, minotaur and Zeus, and
0x00453EF0 by most classes. Patching a shared body changes every user, so patch the vtable slot instead.

---------------------------------------------------------------------------------------------------------------

## 6. PORTING notes

* **No OS calls.** The AI code makes no Win32, DirectX or DirectInput calls. A scan of all 892 matched v12 AI
  functions found only CRT and engine calls; the few `__imp_` hits are in junk padding between functions. A
  port needs these, not OS work:
  * **CRT:** `strstr`, `_strnicmp`, `stricmp`, `sprintf`, `atexit` (function-static damage structs), `__ftol`,
    `rand` (once).
  * **Engine services:**
    * `msgSYSTEM` (SendMsg, PostMsg, SendAll);
    * `animINST` (pos/dir/scale, `SetAnimSeq`, `IsActionFrame`, `GetFloat` from `psSHEET`);
    * `objFindName`/`objFindID`;
    * `scnSCENE` (FloorPlace, FindInstName, scene box);
    * `gsSND_SYSTEM::AddChannel` and `sncCHANNEL`;
    * `entCreate`;
    * `_apCfgReadBool` (`[Debug] Decomp_OFF_AI`);
    * `m3dRandom`/`m3dRandRange`/`m3dRandSelect`.
* **x87 and SIMD square roots.** The code is x87 single precision. Many AI functions inline the engine's
  `m3dSqrt`, which has three paths chosen by the global `m3dSimdType`: SSE `rsqrtss`, 3DNow! `PFRSQRT`, or
  `fsqrt`. You can see them in `db/mergedfiles.cpp` as `PackedFloatingReciprocalSQRAprox`. A port must pick one.
  The approximate paths give slightly different distances, which matters for exact-reach thresholds such as
  tiger bites and rat triggers.
* **Determinism.** The `m3dRand*` streams drive attack choice. MP correctness relies on the server; clients do
  not run SelectGoal.
* **Layout.** Structures are `#pragma pack(1)` with odd offsets. A port that rewrites the classes must keep the
  layout only where data is shared with saves or MP packets: `aiSYNC_DATA`, `aiINIT_DATA`, and the entity MP
  state. Everything else is private.
* **Feb-era mods.** A mod written against the feb build must re-map every field with the shift rule in
  `ai_npc.h` and every vtable slot with the slot map. Message ids >= 0x3F5 changed: damage 0x4C5 -> 0x4C6,
  NPC died 0x4C8 -> 0x4CA.

## 7. Open questions
* Meaning of the 1.2-only fields `aiNPC +0x1B7` (0.3, 0.6 for Cyclops/Gefest), `+0x3E3` and `+0x4EF`, and of the
  new slots 43, 66, 67 and 90.
* The second cinema condition in 1.2 `ProcessFRAME` (v12 0x00544C70).
* v12 twins of feb `aiNPC::GetStr/GetInt/GetFloat`, `aiPLANNER::RegisterNPC/SetBoss`,
  `aiDOMAIN_SPAWN::Update` and `aiInit` were not located (callgraph-only gaps). They are listed in
  `function_index.tsv` with `--------`.
* What the many "Lead" constant changes in `FEB_VS_12.md` mean (Cyclops action frames, gladiator knife spin,
  projectile 1.5 -> 0.3).
* `aiNPC::IsPlrVisible` v12 0x005593E0 is a callgraph match with low similarity. Check it before hooking.
