# Player weapons: Feb 2003 beta vs 1.2

Static comparison of `feb.exe` (2003-02-21, with map) and `v12.exe` (1.2 patch). Every pair of functions below was
matched by vtable slot, by call position or by the match table, and then compared instruction by instruction.
The function list is in `function_index.tsv` and the tools are in `re/work/sdk_weapons/`:
- `inits.py`: field stores and virtual-call arguments in each `ProcessINIT`.
- `pinit.py`: the projectile launch data.
- `fcmp.py`: constants, strings and callees that exist in only one build.
- `sbs.py`: side-by-side diff of one function.

The raw diff of every function is in `re/work/sdk_weapons/sbs/`.

Addresses are given as **v12 (feb)**.

## 1. What did NOT change (all 11 weapons)

The base values set in each weapon's `ProcessINIT` are the same in both builds. `inits.py` compares every store and
argument after the +0x1C field shift, and they match exactly. That covers fire rate, damage, max ammo, clip size and
the action-frame ids.

| weapon | rate (shots/s) | damage | max ammo | clip |
|---|---|---|---|---|
| Shovel | 25 (anim-bound) | 120 | infinite (`wpnflags|=4`) | - |
| Colt | 0 (anim-bound) | 10 | ? | 14 |
| Shotgun (STG) | 1.4 | 4 per pellet | 200 | 5 |
| Machine gun | 10 | 6 | 1000 | - |
| Minigun | 10 | 8 | 1000 (start 500) | 100 |
| Crossbow | 25 | 60 | 50 | 6 |
| Acid gun | 25 | 60 | 100 | 5 |
| Atomic gun | 25 | 200 | 20 | 4 |
| Bazooka | 0.714286 | 90 | 100 | none (-1) |
| Medusa | 1 | 50 | 50 | 3 |
| Dynamite (BOMB) | 25 | 110 | 20 | 1 |

Other things that are the same in both builds:
- **Projectile speeds** (results/weapon_npc.json PROJ_*, re-checked with `pinit.py`): acid 128, atomic 28,
  dynamite 60, bazooka 50, crossbow 297.6, medusa 55.8. Gravity factors are also unchanged: atomic -9.8,
  dynamite -19.6, medusa +2.5 (it rises). Lifetimes are unchanged. The only exception is the Medusa launch data
  (section 3).
- **Hitscan spread** (FillCDTShootList). The constant multisets are equal, and only the code generation and the
  field offsets differ:
  - machine gun: 0.1 at 3 m or less, up to 0.5 at 80 m or more.
  - shotgun: 0.15 at 5 m or less, up to 4.0 at 80 m or more.
  - minigun: 0.45 at 5 m or less, up to 2.0 at 80 m or more.
- **`GetDamage`**: v12 0x004176F0 (0x00415310). The x4 multiplier when the owner has power-up 0 (Titan) is
  unchanged.
- **The animation sequence names** (`PWP_SEQ_*`, `AMG_SEQ_*`, `PWP_MNG_SEQ_*`, ...) and all weapon, SFX and sound
  name strings are the same. A string scan of the weapon code range finds no string that was added or removed.
- **`AdjustTiming`**: v12 0x00417720 (0x00415340), the per-weapon animation speed table. Its constants are the same.

So the 1.2 weapons balance the same way as the beta. The differences below are about networking, sound and visual
effects, plus one real gameplay change: the Medusa projectile.

## 2. Changes common to every weapon

1. **The shot pipeline was rebuilt for multiplayer.**
   - **feb:** `Shoot` (vslot 27) calls `wpnWEAPON::Shoot` and posts msg 0x4C6 to itself. `ProcessSHOOT`
     (vslot 38) then creates the projectile or applies the damage, and on the server calls
     `gsMP_SERVER::SendMsgClientAll(0x4C6)`.
   - **v12:** `Shoot` (vslot 26) computes the origin and direction itself (`GetOrgDirTip` 0x00544050) and builds a
     `gsEVENT`. You can see it in every v12 `Shoot`, for example bazooka 0x00408870, which calls 0x00543E20,
     0x00544050 and `gsEVENT::ctor`. The message id is 0x4C7.
   - **v12, shovel / Colt / shotgun / machine gun / minigun:** vslot 38 is a shared relay (0x0040AC20) that
     re-sends msg **0x4C8**. `pwpWEAPON::ProcessMsg` 0x00417FF0 routes that message to the new vslot +0xB4
     ("ShootBody", for example 0x0040AC80), which does the work.

   Hooking slot 38 still catches every shot, but the message layout is different (see
   results/weapon_npc.json, note 2).
2. **Projectile launches now go through `m3dMATR::RemoveScale`.** Every v12 projectile `ProcessSHOOT` calls it
   before building the launch matrix, and no feb `ProcessSHOOT` does (fcmp `call_only_v12`). The likely effect is
   that a scaled weapon model no longer distorts the projectile direction. (?)
3. **Projectile launch data grew from 0x69 to 0x6D bytes**: feb `mov word [esp+..],0x69` against v12 `0x6D`. The
   `pwpOPP_SFX_START` effect record grew by 8 bytes, which v12 zeroes. One example is acid `DispatchDamage`
   0x00402926.
4. **Every projectile gets a generic sound channel** in `pjlPJL<SFX>::ProcessINIT`, for example `<ACD>` 0x00403210
   (0x00403080). When vslot +0x8C returns true, the code reads min/max from vslot +0x9C, creates
   `new sncCHANNEL(min,max,360,360)` and calls `gsSND_SYSTEM::AddChannel` (v12 0x00511910). The feb template has no
   such code.
5. **The weapon model is hidden on remote copies every frame.** `pwpWEAPON::ProcessFRAME` 0x00417F50 (0x00415BB0)
   now checks `IsWeaponMP` (vslot +0xC0) every frame. It switches the hands' sound channel to local or 3D and sets
   or clears render flag 0x1000000 on the model. feb did this once, in `ProcessINIT_LEVEL` 0x00415B60. The v12
   `ProcessINIT_LEVEL` slot (0x00417F40) only calls `sndChannel->vslot3`.
6. **The vtables changed.** v12 adds slots +0x8C (returns 1), +0xB4 (ShootBody) and +0xCC (sound channel call).
   Bullet weapons also get +0xD8, a new `ShowShotSFX` that loops over 0x15-byte ray records. One feb entity slot
   was removed. The full slot table is in `weapons.h`.
7. **The infinite-ammo flag moved.** It is `wrSysGO+0xA0` in feb and `+0xA4` in v12 (`pwpWEAPON::SubtractAmmo`
   0x00417680). The behaviour is the same.

## 3. Per weapon

### Shovel (pwpSHOVEL, plw_svl.obj)
- Rate 25, damage 120, infinite ammo, flags2 `|=3`: all unchanged (v12 `ProcessINIT` 0x004154A0).
- **The melee hit is resolved over the network.** v12 `Shoot` 0x004155A0 (0x00413C10) no longer calls the target
  directly; it sends the 0x4C7/0x4C8 shot message (`msgSYSTEM::SendMsg`). `ShootBody` 0x00415B00 (feb
  `ProcessSHOOT` 0x00414110) finds the victim by network id with `gsMP_SYSTEM::FindEnt` before it sends the
  `dmgWEAPON`. In the beta, `ProcessSHOOT` built the `dmgWEAPON` itself and broadcast `SendMsgClientAll`.
- The line-of-sight test uses `animINST::GetCenter` (v12 0x004158A4) instead of the victim's vslot +0x5C, plus
  1.5 m in both builds. The 1.5 m `dmgCollectInst` radius is unchanged.

### Colt (pwpCOLT, plw_clt.obj)
- Clip 14, damage 10 and the `flags2` set-up are unchanged (v12 `ProcessINIT` 0x0040C800). The only differences
  are vslot renumbering (+0xD4 to +0xE0 and so on).
- It shares the bullet-weapon changes: the shot relay and shell-floor collision (see the machine gun).

### Shotgun (pwpSTG, plw_stg.obj)
- Rate 1.4, damage 4, max ammo 200, clip 5 and the spread 0.15 to 4.0 are unchanged (`ProcessINIT` 0x004145A0,
  `FillCDTShootList` 0x004147A0).
- It shares the bullet-weapon changes.

### Machine gun (pwpMACHINEGUN, plw_mcg.obj)
- Rate 10, damage 6, max ammo 1000 and the spread 0.1 to 0.5 are unchanged.
- **The muzzle flash and hit effects are replayed from the network message.** `ShowSFX` 0x0040FDD0 (0x0040E770)
  now forwards to the new `pwpBULLET_WPN::ShowSFX(rec)` 0x0040AE70. Base slot +0xD8 0x0040ADC0 replays every ray
  record of a received shot message, so remote players see each bullet's impact.
- **Ejected shells collide with the floor.** `pwpBULLET_WPN::StartShells` 0x0040B0A0 (0x0040A5A0) now calls
  `scnSCENE::FloorPlace` and `partEMITTER_PHYS::SetCDTPlane(…, 0.5, 5.0)`, with pushes at 0x0040B270 and
  0x0040B280. Shells bounce on the ground instead of falling through it. This is new in 1.2: neither call exists
  in feb.

### Minigun (pwpMINIGUN, plw_mng.obj)
Full details are in re/results/build_differences.md section 2, verified there instruction by instruction.
- Rate 10, damage 8, start ammo 500, max 1000, clip 100 and the spread 0.45 to 2.0 are unchanged.
- **The shoot-sound stutter is fixed.** `PlayShootSound` 0x00413660 (0x00411E20): feb called `Stop()` and
  `Play()` on every shot at 10 shots per second. 1.2 keeps the loop running and never cuts off
  `stop_shoot`/`recharge`.
- `stop_shoot_1` plays once through a flag (`flags2 |= 0x20`) instead of on every shot.
- There is a new net "shooting state" handler (slot +0x9C, 0x00414000) and a `ProcessINIT_LEVEL` override
  (0x00413F20). Remote players' miniguns now spin and sound right; in the beta they were silent or wrong.
- The muzzle flare is attached to the barrel tip (`GetOrgDirTip`) instead of the shot position.
- **The shell ejector was replaced.** The beta's own `ShowMngShells` emitter was dropped in favour of the generic
  bullet shell spawner `vslot +0xEC(ftol(m3dRandRange(1,4)))`, which throws 1 to 4 shells per shot
  (`ProcessFRAME` 0x00413F40, pushes 4.0/1.0 at 0x00413F6B and 0x00413F70).
- The class grew 4 bytes: the `Create` call allocates 0x1F3 in v12 and 0x1D3 in feb.

### Crossbow (pwpCROSSBOW, plw_crb.obj)
- Rate 25, damage 60, max 50, clip 6 and bolt speed 297.6 are unchanged.
- The bolt's other launch values are also unchanged: 100, 0.3, 1000 and scale 0.2 (`pinit.py`).
- The point-blank case is unchanged: when the aim ray already hits an entity, no bolt is fired, and
  `pwpOPP_SFX::Start` (burn, `2*GetDamage()`, type 4) is applied directly. In v12 that code sits earlier in
  `ProcessSHOOT` 0x0040D9C0, at 0x0040DA7E (0x2.0f). In feb it is at 0x0040D07A.
- **The explosion effect is merged into one routine.** `pwpCROSSBOW::ShowCollisSFX` (feb 0x0040D1F0, called twice)
  became a single call to 0x0040E090 at the end of v12 `ProcessSHOOT`. That routine builds the explosion with
  `_MakeLCS2WCS_VY`. The behaviour looks the same. (?)
- Its projectile, `pwpCROSSBOW_PJL`, gets the generic 1.2 sound channel from section 2, item 4.

### Acid gun (pwpACIDGUN, plw_acd.obj)
- Rate 25, damage 60, max 100, clip 5, projectile speed 128, and the projectile values 0.3 / 1 / 1000 are unchanged.
- **The acid-drip trail on the projectile is bigger.** In `pwpACD_PJL_SFX::StartSFX` 0x00402300 (0x00402290),
  the argument to `pwpACD_PJL_DROPS` is 0.15 in v12 (push at 0x00402433) against 0.1 in feb (0x004023A8).
- **The trail no longer follows the flight direction.** feb also set the drop emitter direction from the
  normalised velocity times 5 (`m3dNormalize` at 0x004023F7 and `fmul 5.0`). That code is gone in v12, so the
  drops are no longer thrown along the flight path.
- **The empty-canister throw happens in a different place.** `ThrowClip` (feb 0x00401B90, the "pwp_acd_ammo" rigid
  body) is inlined into v12 `ProcessFRAME` 0x004019B0. Its constants are unchanged: -0.3, -0.5, 0.1 to 1.3, 10
  and 5. The 0.8333 s timer `+0x1DB` (feb `+0x1BF`) is now armed in `ProcessFRAME` (0x00401BCF); feb armed it
  in `ProcessSHOOT`.
- The acid effect on targets (`DispatchDamage` 0x00402920, (0x004027A0): `pwpOPP_SFX` type 2, value 5, 3.0 s) is
  unchanged. Only the struct grew.

### Atomic gun (pwpATOMICGUN, plw_amg.obj)
- Rate 25, damage 200, max 20, clip 4, speed 28, gravity -9.8 and lifetime 100 are unchanged.
- **The charge calculation runs on every machine.** In feb, `Shoot` 0x00404110 compared the charge with 0.7 and
  scaled it by 1/0.7 (1.42857). In v12 that test moved into `ProcessSHOOT` 0x00404550: `fcomp 0.7` at 0x0040466E
  and `fmul 1.42857` at 0x00404689. Clients now compute the same rocket from the network message.
- The rocket flight sound (`sncCHANNEL_3D` 10 to 50 m) is unchanged. It is created in feb 0x00404FD1 and v12
  0x00405470.

### Bazooka (pwpBASOOKA, plw_bsk.obj)
- Rate 0.714 (1.4 s), damage 90 and speed 50 are unchanged. The other launch values (25 / 0.2 / 1000) are also
  unchanged.
- **The rocket flight sound is shorter range.** In `pwpBSK_BULLET::ProcessINIT` 0x00409120 (0x00408BC0), the sound
  channel min/max distance is **7 / 25 m** in v12 (0x004093CA / 0x004093D4) against **10 / 45 m** in feb
  (0x00408DE9 / 0x00408DF3). The generic 1.2 projectile channel is added as well.
- **The impact flash is smaller.** In `pwpBSK_BULLET::ShowCollisSFX` 0x004094B0 (0x00408ED0), the size spline keys
  are **3, 6, 6** in v12 against **7, 14, 14** in feb. feb pushes 7f/14f at 0x00409011, 0x0040901F and
  0x00409032; v12 uses the constant 3.0 at 0x005F9C78, and 3.0+3.0. Likewise, the 7.0 value in `StartSFX` became
  that 3.0 constant (feb 0x00408B19, v12 0x00409076).
- feb skipped the trail SFX and the sound when `gsSysMP->mode == 4` (feb `gsSysMP` 0x00948250; mode 3 = client per results/weapon_npc.json F_MP_NEXT_PLAYER, so 4 is probably the dedicated server; `cmp [ecx+0x28],4` at 0x00408C4A and
  0x00408D8A). v12 has no such test in this function. (?)

### Medusa (pwpMEDUSA, plw_mdg.obj): real gameplay changes
- Rate 1, damage 50, max 50, clip 3, speed 55.8 and the upward drift 2.5 are unchanged.
- **The launch data changed** (`ProcessSHOOT` 0x004107D0 (0x0040F180)):
  - one float changed from **0.01 to 0.3**: feb 0x0040F2B8 against v12 0x004108F2;
  - one int changed from **2 to 6**: feb 0x0040F283 against v12 0x004108C5.

  The float is most likely the projectile collision radius. The balloon becomes about 30 times "fatter", so it
  bursts on contact much more easily. The int is a flags field. (Both meanings are unconfirmed.)
- **The petrify cloud hits more targets.** In `pwpMEDUSA_PJL::MakeDamage` 0x004112D0 (0x0040FB60), the per-target
  hit box (two extents) is scaled before the overlap test:
  - feb: NPC type 0x11 x0.5, type 0x13 x0.9.
  - 1.2 adds NPC type 0x0F: x0.9 and x2.3 (0x0041168E / 0x00411696).
  - 1.2 adds **players** (tag 'PLYR'): x2.7 and x1.2 when player `+0x98 == 0`, otherwise x1.7 (0x004116B0,
    0x004116B8, 0x004116C0).

  In the beta the cloud only had special sizes for two monster types. In 1.2 it also covers players in
  multiplayer and one more monster type.

### Dynamite (pwpBOMB, plw_bmb.obj)
- `ProcessSHOOT` 0x00406D20 (0x004067E0) has a mnemonic similarity of 1.00, and all launch values are equal:
  speed 60, gravity -19.6, 120 / 0.7 / 0.2 / 0.15 / scale 0.3.
- `pwpBMB_PJL::ProcessFRAME` and `ProcessMsg` are identical.
- **`Explode` changed in three ways.** In `Explode` 0x004075F0 (0x004070B0):
  - The explosion damage record (`dmgGENERAL` size word) shrank from 0x28 to 0x26 bytes (feb 0x004070FA, v12
    0x0040763B). This follows the 14-byte 1.2 `dmgDAMAGE` (game_shared/damage.h).
  - The network notify message id changed from 0x40C to 0x40E (feb 0x00407172, v12 0x004076B3).
  - The beta's `gsSysMP->mode == 4` early-out (feb 0x00407183) is gone.

  v12 also passes 1 (feb 0) and a new argument 9 (0x0040770C). The radius damage itself still goes through
  `dmgDispatchExplosDamage` (v12 0x00546410).

## 4. Shared effects

- `pwpOPP_SFX::CalcRadDamage` 0x0041BAF0 (0x00419390): the constants differ only by the inlined `m3dSqrt`
  (rsqrtss) code. The radial damage logic is the same. (?)
- Every `ShowCollisSFX` (acid, atomic, dynamite, bazooka and crossbow) reads the hit record 2 bytes earlier:
  `add eax,0x18` became `0x16`, and `[eax+8]` became `[eax+6]`. This is the smaller 1.2 damage/event record, not
  a change in behaviour. The scorch-mark decal (`wrsfxSCORCHMARK::Add`, v12 0x00467DE0) is still spawned.

## 5. Confidence

- **High:** sections 1 and 2.1 to 2.3, plus the Bazooka, Medusa, Minigun, acid trail and shell-floor items. For
  each of these, the instruction and its address were read in both builds.
- **Medium (?):** the meaning of the Medusa float and int, and whether `gsSysMP` mode 4 is the dedicated server.
- **Not covered:** the NPC weapons (`aiNPC` ranged attacks, `pjlSPEAR`, ...). Those belong to the AI subsystem.
