# Enemies: what changed between the Feb 2003 beta and 1.2

Every claim below compares the feb code (`db/WillRock.map` + `db/mergedfiles.cpp`) with the v12 code at the
address given. Notation: `feb 0xAAAAAAAA -> v12 0xBBBBBBBB`. Field offsets use the v12 layout unless marked feb.
"Verified" means I read both instruction sequences. "Lead" means the automated comparison shows the difference
(different constants/strings in matched functions, `work/sdk_ai/allfc_clean.txt`, `febdiff.txt`) but I did not
work out what it does.

How the comparison was done (so it can be repeated):
1. **Class matching.** Each enemy is matched through its `entRegisterClass("ai_xxx", ...)` call. The class string
   is the same in both builds, so the `Create` function, the allocation size and the vtable follow
   (`work/sdk_ai/reg.py`).
2. **Vtable alignment.** feb and v12 vtables were aligned with one slot map for all 24 enemy vtables
   (see `ai_npc.h`). Every enemy agrees with the map from slot 59 up.
3. **Function and data comparison.** Each matched function pair was compared by float constants
   (`fcdiff.py`), by stored immediates (`stores.py`, which pairs `this+off = value` writes) and by referenced
   strings (`sdiff.py`).
4. **Coverage.** 892 of the 1239 feb functions in the AI objs have a v12 twin (`function_index.tsv`).

**No enemies were added or removed.** Both builds register the same 25 NPC class names. Two NPC classes are
registered twice: `ai_tiger`/`ai_lion` share one class, and so do `ai_glad_knife`/`ai_glad_mace` with their
generic parent. The only renames are two projectile classes, Cerberus's `pjl_cerber_fireball` and
`pjl_cerber_acid`, which are now `pjl_crb_frb` and `pjl_crb_acd`.

---------------------------------------------------------------------------------------------------------------

## 0. Changes that affect every enemy (framework)

| # | change | evidence |
|---|---|---|
| F1 | **Object layout.** `sizeof(aiNPC)` grew from 0x51B to 0x543, so every enemy grew by 0x28 (tiger, minotaur and satyr by 0x2C, medusa by 0x30). Shift rule: see `ai_npc.h`. | ctor feb 0x00534210 -> v12 0x00547A60 |
| F2 | **7 new virtual slots** (v12 slots 8, 23, 43, 58, 66, 67 and 90; slots 8 and 23 come from entENTITY). The feb slot `SetDirection` (feb slot 56) was removed. **Slot 58 is a new gate.** In `UpdateOnServerOnly`, after `Think`, v12 calls `vslot 58`. If it returns 0, the NPC skips SelectGoal, PlanMove and MoveToGoal. aiNPC_WR implements it as `!(wrFlags & 8)`, so altar opponents that are still hidden can no longer walk. | v12 0x0054F51C `call [edx+0xE8]`; impl v12 0x00453F00 |
| F3 | **NPCs that leave the level are deleted.** New per-frame test in `UpdateOnServerOnly`: if the NPC's position is outside the scene bounding box expanded by 50 units, v12 calls `PostMsg(this, MSG_MSG_TERM)`. Physics rigid bodies use the same helper. feb has no such test, so an NPC that fell through the world thought forever. | v12 0x0054F491..0x0054F4AF, helper v12 0x00508EB0 (`push 0x42480000` = 50.0) |
| F4 | **Cut-scenes.** feb `aiNPC::ProcessFRAME` stopped every NPC while `cinSYSTEM::IsActive()` was true. v12 stops them only when the cinema also reports a second condition (v12 0x00544C70, meaning unknown), and NPCs with `AIF2_BOSS` keep thinking. v12 also skips the whole frame of an authoritative copy (`entFlags & 4`, the bit that selects UpdateOnServerOnly) whose lock count is above 0. | feb 0x00534EA0 vs v12 0x00548760..0x005487B8 |
| F5 | **Death bookkeeping.** v12 `aiNPC::StartDeath` also calls a planner method (v12 0x00550FE0, probably `UnRegisterNPC`), broadcasts message 0x4CA (feb 0x4C8, renumbered) and writes the MP state byte `+0x10C = 0x8B`. A death with damage subtype 2 (the "die of inactivity" special damage) presets `deadTimer = corpseTime - 0.1`, so the corpse disappears almost at once. | feb 0x00530BE0 -> v12 0x0054BE00..0x0054BE9C |
| F6 | **Damage struct.** The damage struct in `ProcessCDT_FIREABLE` uses the 1.2 `dmgDAMAGE` layout: type is a byte at +4, amount is at +6. NPC-on-NPC damage (type 4, feb type 3) is still scaled by 0.3, and a hit still wakes an inactive NPC. | feb 0x00530AF0 -> v12 0x0054BD10 |
| F7 | **Armoured parts removed.** `IsArmoredObject` (v12 slot 113, feb 110) is an override in feb that makes hits on named parts harmless, for example a satyr's bow or quiver, the minotaur's axe "SEKIRA", Atlas's globe, the centaur's hook, the skeleton/gladiator weapons and the statues' pedestals. In 1.2 every enemy uses the base stub `return 0`, except **Cyclops, Gefest and Zeus**, which keep theirs. The strings `quiver`, `bow_`, `luk1`, `strela2`, `plr_hook2`, `grapia_fin`, `revol18_` are gone from v12. | v12 0x004533B6 `call [edx+0x1C4]`; slot 113 = 0x00403200 in 21 of 24 vtables; kept: cyclop v12 0x00444870, gefest 0x004467F0, zevs 0x00460C20 |
| F8 | **`ParseNameClass` moved to the base class.** It is now implemented in `aiNPC` (feb: pure virtual) and handles `"_sniper"` there: `shootRange = 800`, `AIF_SNIPER`, MP byte 0x82. `aiNPC_WR::ParseNameClass` no longer does this. Instead it adds **`"_noappearsfx"`** in the class name, which sets `wrFlags |= 0x2000000` (no appear effect) and replaces the `isNoSFXAppear` lookups in InitSFX/SyncSFX. | v12 0x005484F0; v12 0x00452D4A |
| F9 | **Constructor defaults.** `+0x193` (feb +0x177, used by ConstrainMotion) changed from 30.0 to 1.1. New field `+0x1B7` = 0.3, set to 0.6 by Cyclops and Gefest. New fields at `+0x3E3` (initial value not checked) and `+0x4EF` (ctor writes 0). All other defaults are unchanged: speed 3.5, turn 180, radCDT 0.7, radActivate 30, radActivateBack 20, deactivate time 40, corpse 5 s plus 4 s fade, DecideEnemy every 6 frames (10 when idle). | ctor v12 0x00547A60 |
| F10 | **3D voice channel.** The per-enemy channel (`sncCHANNEL_SPH`) used min/max distance 30/150 (some classes 10/65) written by each ProcessINIT. 1.2 writes 0 / 1.0 there, so the distances now come from the sound system. This shows up as a "-30,-150 +1" difference in nearly every ProcessINIT and is not an AI change. | e.g. tiger feb 0x00454DBC -> v12 0x0045DC02 |
| F11 | **New 1.2 overrides of virtual slot 43** (base `return 1`): skeleton spear and sword (v12 0x0045A960). **New overrides of slot 67:** diskobolus (0x00431DE0), gladiators (0x004494E0) and minotaur (0x00452720). **Slot 66:** medusa (0x0044CF30). **Slot 90:** diskobolus (0x00431E40). | vtables |
| F12 | Spawn domains (`aiDOMAIN_SPAWN`) gained a dword at +0x128. Fields after it moved +4. Keys and defaults are unchanged. | Init feb 0x005396C0 -> v12 0x00548F90 |
| F13 | `aiNPC_WR::ProcessMP_NOTIFY` also handles ACTIVATE/DEACTIVATE events, so clients see activation. **Lead.** | feb 0x0044AF20 -> v12 0x00432380 (strings) |

Unchanged (verified): the inactivity death (`DieOfInactivity` v12 0x0054C110: 20 s, 30 s with a boss,
5000 damage via msg 0x4C6), `CheckDeactivateCondition` thresholds (1.7x/2x radActivate, 0.9x/1x
timeDeactivate), the AI_PROP keys read by `aiNPC::ProcessINIT`, and the debug switch `[Debug] Decomp_OFF_AI`.

---------------------------------------------------------------------------------------------------------------

## 1. Per enemy

Sizes are allocation sizes (`operator new` in Create). Slots are vtable entry counts.

### Rat (`ai_rat`, aiRAT) -- size 0x55B -> 0x583, slots 113 -> 116
* **Weaker self-destruct.** The suicide damage the rat sends itself within 1.8 m of the player dropped from
  10000 to **5000** (feb 0x0044D473 -> v12 0x00455BF3, `aiRAT::Think`). Verified. Both values kill the rat.
  What changes is the magnitude passed on to the death/explosion code.
* **No longer gibbed by explosions.** New 1.2 override of slot 109 (`IsGibbable`) returns 0 (v12 vtable 0x005FF040
  +0x1B4 = 0x0048DF30). feb used the default 1, so `aiNPC_WR::DecideExplosion` could gib a rat hit by a
  rocket/bomb within 1 m. Verified.
* ProcessINIT: health/maxHealth 2, radCDT 0.3, speed 6 and corpse 1 s/1 s are unchanged. Only the sound-channel
  values changed (F10).
* `wrsfxRAT_SPARKS_EMITTER::ctor` has new particle constants (0.5, 4.0). Lead.

### Satyr (`ai_satyr`) -- 0x557 -> 0x583 (+0x2C: one new class field), slots 115 -> 118
* Armoured bow/quiver/arrow parts removed (F7). Verified.
* **New 1.2 `Think` override** (v12 0x00456850; feb used `aiNPC_WR::Think`). Lead: it adds behaviour before or
  after the generic think.
* **Taller collision.** ProcessINIT now also writes `heightCDT = 1.4` (@0x0045657D) and `heightUp1 = 2.3`
  (@0x004565AF), so the satyr's collision cylinder is taller. Verified (stores).
* Arrow (`pjl_satyr_arrow`, shared `pjlARROW_GENERIC::ProcessINIT` feb 0x0044E8C0 -> v12 0x00457200): the feb
  constants 1.5/360/5/30 were replaced by 0.3 and 8.0. The 360/5/30 are sound-channel values (F10). The
  1.5 -> 0.3 is the same pattern as every NPC projectile (see "Projectiles"). Lead.

### Skeleton spear (`ai_skel`) and kamikaze sword skeleton (`ai_skel_kamik`) -- 0x577 -> 0x59F, 0x54F -> 0x577
* Armoured parts removed (F7).
* **New 1.2 override of slot 43** (v12 0x0045A960) for both. Lead.
* `aiSKEL_GENERIC::StartDeath` (feb 0x004503C0 -> v12 0x00458DA0) no longer builds the 5000-damage struct
  itself; the same applies to minotaur, bird and sketavr. Lead: that work moved into base `StartDeath` (F5).
* `aiSKEL_RIGID_START::StartExplos` (shatter on death) gained constants 1000.0 and 0.5 (v12 0x00459328 and
  0x0045932E). These are probably the impulse and time for the bone pieces. Lead.
* Spear projectile: same as the satyr arrow.
* `wrsfxSKELETON_SPEAR_LAND_GRIT` particle constants were removed from the ctor. Lead.

### Gladiators (`ai_glad_knife`, `ai_glad_mace`) -- 0x593 -> 0x5BB, 0x5A3 -> 0x5CB
* Armoured weapon parts removed (F7).
* **Bigger collision.** `aiGLAD_GENERIC::ProcessINIT` (feb 0x00440B70 -> v12 0x00448970) changed a 1.2 constant
  to 1.3 (@0x004489FE) and adds 2.1 (@0x00448A3A). The mace gladiator now also writes `radCDT = 1.0` and
  `heightCDT = 1.7` (v12 0x0044AB8A and 0x0044AB94). Verified (stores).
* Knife/mace projectiles: in feb, `pjlKNIFE::ProcessINIT` and `pjlMACE::ProcessINIT` set a spin
  ("rot_center", 350 or 550 deg/s, -90). In v12 these functions are much shorter, and the "rot_center" string
  is no longer referenced from them. `aiGLAD_KNIFE::AimKnifeToPlayer` lost its 35/-4.5/-9 aiming table
  (feb 0x00442170 -> v12 0x00449EB0). Lead: the knife throw and aiming were rewritten.

### Minotaur (`ai_mino`, `ai_mino_atrium`) -- 0x573 -> 0x59F (+0x2C), 0x577 -> 0x5A3
* **`ParseNameClass` was inlined into ProcessINIT** (v12 0x00450900 reads `AI_MINO isRed`, `AI_MINO isRegen`,
  `AI_PROP isStatue` and tests `"_statue"`, `"_gen2"`, `"_atrium"` in the name).
* **Bigger collision.** `radCDT` 0.7 -> **0.9** (@0x00450934) and `heightCDT` 1.1 -> **1.3** (@0x00450948).
  Verified.
* Axe no longer armoured (F7). The `aiMINO::ProcessCDT_FIREABLE` override was replaced by a shared wrapper
  (v12 0x00460030 = `aiNPC_WR::ProcessCDT_FIREABLE` + return 1). Lead: the feb override also tested "SEKIRA".
* `aiMINO::AimAxToPlayer` (feb 0x004499F0 -> v12 0x00451E30) gained constants 0.1 and 60.0 (@0x00451ECE,
  0x00451EDD). Lead: probably an angle limit for the axe throw.
* New override of the 1.2 slot 67 (v12 0x00452720).

### Centaur (`ai_cent`) and skeleton centaur (`ai_sketavr`) -- 0x593 -> 0x5BB each
* **Taller collision.** ProcessINIT (feb 0x00434890 -> v12 0x0043AD60): `heightCDT` 1.1 -> **1.4** (@0x0043AD88),
  new `+0x197 = 0.9` (@0x0043AD92) and new `heightUp1 = 2.0` (@0x0043AD9C). Verified.
* The hook part is no longer armoured (F7).

### Tiger / Lion (`ai_tiger`, `ai_lion`) -- 0x56B -> 0x597 (+0x2C), slots 113 -> 116
* **Longer bite reach.** `aiTIGER::CalcDamageToTarget` (feb 0x004551C0 -> v12 0x0045E0C0): full damage now
  within **1.1 m** (feb 1.0), falling to zero at **1.7 m** (feb 1.3, slope 2.5 instead of 5). v12 has the
  falloff code twice (two target cases). The pounce/bite multipliers 15/20/3/13/10 are unchanged.
  Verified (float sequences, v12 0x0045E201..0x0045E3D1).
* ProcessINIT: class field `+0x58F` (feb +0x563) 30 -> **40** (@0x0045DACF). `FillAttackFlags` uses it as the
  angle limit for slowing down before an attack. New field `+0x577` = 0. Verified.
* `aiTIGER::IsAnimSeqGoodForFallback` override removed (now `return 0`): tigers no longer pick a fall-back
  animation when knocked back. A new `GetAnimPlayRate` override was added (v12 0x0045DD70). Verified (vtable).
* `EvalAnimGoalSeq` (v12 0x0045DD90), `Think` (v12 0x0045E7E0), `ProcessCDT` (v12 0x0045F280) and
  `ProcessActionFrame` (v12 0x0045E4E0) gained new constants (0.2/3/0.8/4, 2.85/3.6, 0.2/3.6, 160/6). Lead:
  new random choices and distances for the pounce.
* `SetMovementSpeed` no longer writes run speed 16 and turn 270/20 directly (feb 0x00455B70 -> v12 0x0045ECB0).
  Lead: probably taken from fields now.
* Unchanged: tiger health 50, lion health 60 (`aiTIGER::ParseNameClass` v12 0x0045D4B0), radActivate 300.

### Cerberus (`ai_cerber`) -- 0x573 -> 0x59B, slots 114 -> 117
* **Projectile classes renamed** to `pjl_crb_frb` and `pjl_crb_acd` (v12 Create 0x0043D680 and 0x0043DA60;
  feb `pjl_cerber_fireball`/`pjl_cerber_acid`). `LaunchOneProjectile` (feb 0x00438940 -> v12 0x0043EFC0) adds
  1.0/25/1.5/8 and references `croc_bite`. Lead: new launch speed/spread values.
* **Longer paw reach.** `ProcessActionFrame` (feb 0x00436D70 -> v12 0x0043D310): paw damage 10 within **1.9 m**
  (feb 1.5) down to 0 at **2.3 m** (feb 2.0). The slope is 25 (feb 20) and a 0.5 threshold became 0.4.
  Verified.
* **Slower turning run.** `SetMovementSpeed` (v12 0x0043D620): when the enemy is closer than 20 m and more than
  80 deg off to the side, speed is now **4.76** (feb 19.04!) with turn speed **84** (feb 70). The normal run
  is unchanged (9.52 / 70). Verified (v12 0x0043D648 and 0x0043D652).
* ProcessINIT writes a new `+0x197 = 0.15` (@0x0043CB1F). `SelectGoal` lost the constants 2.4/1.0. Lead.

### Crocodile (`ai_croc`) -- 0x563 -> 0x58B
* **Bite changed.** `ProcessActionFrame` (feb 0x00439380 -> v12 0x0043FC30): reach 1.5 -> **1.9** and a second
  1.9; 1.0 -> 1.4; a damage/slope 15 -> **10.714**. Verified as constant changes.
* `ProcessCDT_FIREABLE` override replaced by the shared wrapper (v12 0x00460030). `SelectGoal` changed a 0.3
  to 0.8. Lead.

### Stymphalian bird (`ai_bird`) -- 0x6F3 -> 0x71B
* Only layout changes (F1). It also has a new debug string `"speed: %.2f, %.2f, %.2f"` in `aiBIRD::ProcessFRAME`
  (v12 0x00439320), and `StartDeath` no longer builds a 5000 struct (see skeleton). Gameplay constants in
  ProcessINIT are identical.

### Amur / cupid (`ai_amur`) -- 0x57F -> 0x5A7
* **Smaller collision.** `radCDT` 3.0 -> **1.0** (@0x004374A7). New `heightUp1 = 0.3` (@0x004375D4). Verified.
* Armoured bow and arrow parts removed (F7). `SelectGoal` has a new 0.2 (@0x00437F7F). Lead.

### Medusa (`ai_medusa`) -- 0x587 -> 0x5B7 (+0x30), slots 113 -> 116
* **Helper spawns now scale with difficulty.** `ProcessCDT_FIREABLE` (feb 0x00446130 -> v12 0x0044E3B0) was
  rewritten. It still spawns a helper opponent where it was hit (`SelectOpponentToSpawn` + `entCreate`), but:
  the cap is now **`10 * aiSpawnOppCoeff`** registered NPCs (v12 0x0044E479 calls the coeff getter 0x00548E20,
  `fmul 10.0`) instead of a flat 10; it also skips spawning while the class field `+0x5B3` is above 0
  (a cooldown; `fcomp 1e-6; test ah,5; jp skip` at v12 0x0044E4A1); and the feb
  "search 50 random points within 80 m away from other NPCs" loop was replaced by a helper (v12 0x0044E7E0,
  with a random 0.15/0.4 choice in v12 0x0044E550). Verified.
* ProcessINIT (v12 0x0044CF80): `navArriveDist` 15 -> **12** (@0x0044D012). New: looks up the scene object
  **`&dom_action_OFF`** and builds a 0x3C-byte area object from it (v12 0x0044D29D..). Lead: probably a zone
  where Medusa does not act or spawn.
* New slot 66 override (v12 0x0044CF30).

### Cyclops (`ai_cyclop`) -- 0x57F -> 0x5A7
* **Level-specific fix.** `ActivatePlrVisibility` (v12 0x004427E0) activates the Cyclops at once if it is still
  inactive and the level name contains **`"2_5_coliseum"`**. It also no longer sets sound flag 4 there.
  Verified.
* Keeps `IsArmoredObject` ("dubina" club, "cherep" skull) (F7).
* New overrides: `ProcessMsg` (v12 0x004449A0), `FillSoundFlags` (v12 0x00444500) and `PostThink`
  (v12 0x00443E70). `GetAnimPlayRate` now returns per-sequence rates 1.2/2.4/1.6/1.4 (v12 0x00443AC0; feb
  2.9/1.0). `ProcessActionFrame` (feb 0x0043BDA0 -> v12 0x00442E70) is 25% smaller and lost the feb constants
  1.5/400/2/0.8/0.1/1.2/0.13/5, and `sfx_cyclop_death` is no longer created from there. Lead: the
  grab/munch/stomp code was reworked.
* ProcessINIT sets the new `+0x1B7 = 0.6` (@0x004422F2). `ParseNameClass` adds an 11.0 (@0x004426AF). Lead.
* Stone projectile `pjlSTONE::ProcessFRAME` (feb 0x00439B60, 1408 bytes -> v12 0x004405A0, 688 bytes) lost its
  10/4/40 constants. Lead: stone flight/split logic simplified.

### Gefest / Hephaestus (`ai_gefest`) -- 0x587 -> 0x5AF, slots 115 -> 119 (one class slot added)
* **New key:** `AI_PROP size` is now read (scales the model like the Cyclops; v12 0x0044579A).
  `ParseNameClass` changed 1.45 to 1.0/1.8. Verified (string and constants).
* `GetAnimPlayRate` 2.9 -> 1.4/1.8 (v12 0x004462B0). `ProcessFRAME` adds 108.0 (v12 0x00445E14). New field
  `+0x1B7 = 0.6`. New `ProcessMsg` override (v12 0x00446A90) and a 4th class-own virtual (v12 slot 118,
  0x00446090). Lead.
* Keeps `IsArmoredObject` ("molot" hammer, "gorshok_3" pot) (F7).

### Zeus (`ai_zevs`) -- 0x70F -> 0x737
* **Three times the health:** maxHealth/health **10000 -> 30000** (v12 0x0045FC35 and 0x0045FC3F,
  `aiZEVS::ProcessINIT`). ProcessINIT also now writes radCDT and heightLowTester 15/15, and a class field
  `+0x56F = 5.0`. Verified.
* **Activation rewritten** (feb 0x00456C00 -> v12 0x0045FF60):
  * feb always set field +0x54F = 3.0 and `SetConstColor`. v12 sets the same field (`+0x577`) to 3.0 with
    `SetTransparency(0x80)` (a fade-in) only in single player (`gsSysMP->mode == 1`), and to -1 otherwise.
  * The timer field +0x573 (feb +0x54B) is set to **8.0** instead of 10.0. Probably the first attack delay.
  * The global `0x0064E484` is set to 3.5.
  * Feb called `aiPLANNER::SetBoss` here. v12 does not, but `ProcessTERM` still clears the boss
    (v12 0x00460021).
  Verified.
* The own `ProcessFRAME` (feb 0x00456CC0) and `ProcessCDT_FIREABLE` (feb 0x00456D00, which set sound bits on
  hits) are gone: v12 uses the shared wrappers 0x00448F00 / 0x00460030. A new `FillSoundFlags` override
  (v12 0x00460320) probably took over the sound bits. `StartMainSFX`
  attaches to `"torso&ds"` instead of `"torso"`. `pjlZEVS::ProcessCDT` (v12 0x004624F0) has new 2.5/12/0.5/15
  constants. `LaunchOneBolt` lost a 0.7. Lead.
* Keeps `IsArmoredObject` ("cyl2_1", "sphere", "garp_lod") (F7).

### Atlas statue (`ai_atlas`) -- 0x5C3 -> 0x5EB, slots 119 -> 121
* **Proper collision cylinder:** ProcessINIT (v12 0x0042F840) now writes `radCDT = 1.2`, `heightLowTester = 1.2`,
  `heightCDT = 3.3` and `heightUp1 = 4.5` (feb left the 0.7/1.1 defaults). Verified.
* `ParseNameClass` (v12 0x0042FB70): new name tag **`"_air"`**; `shootRange` 40 -> **60** in several branches;
  radActivate/radActivateBack 250. Verified (stores and strings).
* `ThrowBall` (feb 0x0042B240 -> v12 0x00430600): a 13 became 11, and new 10/5/15/6/4/0.7 constants. The ball
  (`atlasBALL::ProcessINIT` v12 0x004355C0) gets 3000/0.1, and `ProcessFRAME` gets 2.5/-0.35/0.5. Lead: globe
  speed and bounce retuned.
* Globe no longer armoured. The `aisATLAS::UpdateSound` override is gone (the slot is the empty stub). New
  `ProcessMsg`, `FillSoundFlags` and `PlaySounds` overrides were added (v12 0x00430A20, 0x00430860,
  0x00430910; feb used the base/empty versions), so the sound code probably moved there.

### Discobolus statue (`ai_diskobolus`) -- 0x5BB -> 0x5E3
* **Collision:** `radCDT = 1.4`, `heightCDT = 2.5`, `heightUp1 = 4.2` and `navArriveDist = 8` are new in
  ProcessINIT (v12 0x0043108B..0x004310A9). Verified.
* `ThrowDisk` (v12 0x00431960) adds a 6.0 (@0x00431B56). Lead. Disc no longer armoured. New overrides of
  slots 67 and 90 and of FillSoundFlags/PlaySounds; the `UpdateSound` override was removed.

### Perseus statue (`ai_perseus`) -- 0x5A7 -> 0x5CF
* No longer armoured. New `ShouldChangeEnemy` override (v12 0x00432490). Otherwise layout only.

### Sphinx statue (`ai_sphinx`) -- 0x5BF -> 0x5E7
* No longer armoured. New `ShouldChangeEnemy` override (v12 0x00433E30). `StartDeath` adds 3.5 and `Think` adds 30.0 (v12 0x00433300 and 0x004337D0). Lead.

### Projectiles (all NPC `pjl*` classes)
Every NPC projectile's `ProcessINIT` (stone, gefest, medusa, sphinx, disc, axe, arrows, spear, knife, mace)
has the same difference: the feb constants `1.5, 360, 10, 65` (or `1.5, 360, 5, 30`) became `0.3` (+ `8.0` for
arrows). The 360/10/65 are the projectile's sound channel (F10). The 1.5 -> 0.3 is an `fadd` on a value read
from a table (`pjlSTONE` v12 0x004400A9). Lead: a lifetime or launch-height offset. Every `pjlPJL<...>::ProcessINIT`
template instance gained a 360.0. All projectile objects grew by 0x1C (the entity growth).
