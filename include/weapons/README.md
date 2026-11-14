# Weapons subsystem (pwp* / plw_*.obj), Will Rock 1.2

Unofficial modding SDK and porting map, built by static analysis of `v12.exe`. The reference is `feb.exe` with
`WillRock.map`. Neither exe was run.

Files in this folder:

| file | what is in it |
|---|---|
| `weapons.h` | `pwpWEAPON`, `pwpBULLET_WPN` and the 11 weapon classes: fields (v12 and feb offsets), the weapon index enum, the msg ids, the TWEAK table of base values, the key functions and **every vtable in slot order** (v12 and feb implementation for each slot) |
| `projectiles.h` | the 6 player projectiles: launch values, classes, vtables, damage dispatch, and the `pjlPJL<SFX>` template |
| `FEB_VS_12.md` | per-weapon differences between the Feb 2003 beta and 1.2, with addresses |
| `function_index.tsv` | 288 matched functions (feb, v12, name, mnemonic similarity, confidence, how matched) |

The base class `wpnWEAPON` (ammo, rate, damage, `shootPos`, `MakeDamage`) is in `../game_shared/weapon_base.h`,
and `dmgDAMAGE` / `dmgWEAPON` are in `../game_shared/damage.h`.

## How it works

**Objects.** Each weapon is an entity created by class name (`entRegisterClass` in `pwp*Init`, called from
`pwpInit` v12 0x00416F00?). The class names are `pwp_svl`, `pwp_clt`, `pwp_stg`, `pwp_mcg`, `pwp_mng`, `pwp_crb`,
`pwp_acd`, `pwp_amg`, `pwp_bsk`, `pwp_mdg` and `pwp_bmb`. The player owns one instance per weapon (`owner` is at
`+0x1A3`).

The weapon is drawn as the first-person model: its `animINST` is at `+0xC0`, and that model's `objOBJ` sits under
it. Animation sequences are bound by name in `SetupAniSystem`:
- shared names: `PWP_SEQ_IDLE`, `IDLE_1`, `PUT_ON`, `TAKE_OFF`, `TAKE_OFF_EMPTY`, `SHOOT`, `RECHARGE`,
  `FORCE_RECHARGE`, `RUN`, `UNUSED`;
- per-weapon names: `AMG_SEQ_LOAD_ROCKET`, `PWP_MNG_SEQ_START_SHOOT`, `CLT_SEQ_PREPARE_TO_SHOOT`, and so on.

The sequences themselves come from each weapon's template (`.tpl`) and its animation data, loaded by the engine
under the class name.

**Lifecycle.**
1. `Create` (allocates the class size, stores the weapon index at `+0x98`, sets the vtable).
2. `ProcessINIT`, which:
   - sets the rate, damage, max ammo, clip and action-frame ids;
   - creates the sound channel (`sncCHANNEL_SPH`, 10 to 50 m);
   - finds the model parts by name (`clip`, `rocket`, `flame_source`, `shell_source`, `drops_source`, ...).
3. Every frame, `ProcessFRAME`:
   - `wpnWEAPON::ProcessFRAME` counts the cooldown down;
   - `Anim2Sound` and `UpdateSound` play sounds on animation events (`+0x1C7`);
   - the remote-copy model is hidden;
   - weapon-specific work runs, such as clip throwing, minigun spin and shell spawning.
4. `ProcessTERM`.

**Firing.**

```
plrPLAYER input FIRE -> weapon vslot +0x68 Shoot(target, aim)
   wpnWEAPON::Shoot (0x00543E20): cooldown / ammo checks -> GetOrgDirTip (tip object) -> gsEVENT shot msg 0x4C7
   bullet weapons first fill a ray list: FillCDTShootList (+0xF8) -> FillCDTShootListSpread (+0xFC)
msg 0x4C7 -> wpnWEAPON::ProcessMsg -> vslot +0x98 ProcessSHOOT
   projectile weapons: SubtractAmmo(1) (+0x64), build pjlPJL_INIT, entCreate("<pjl class>") (see projectiles.h)
   shovel / bullet weapons: shared relay 0x0040AC20 -> msg 0x4C8 -> pwpWEAPON::ProcessMsg -> vslot +0xB4 ShootBody:
        per ray: target found by net id, dmgWEAPON (damage = GetDamage()) sent as GS_MSG_DAMAGE 0x4C6,
        then ShowShotSFX (+0xD8) -> ShowSFX (+0xDC) per ray: flare, hit sparks, decal, water splash
reload: Recharge() when the RECHARGE sequence reaches reloadActFrame (IsReadyRecharge, +0xC8)
zoom:   SetZoom(int) sets flags2 bit 8 (crossbow, Colt)
```

**Data and resources used.** No weapon file is parsed by this code directly. It names things that the engine loads:
- templates and models by class name;
- particle-system templates: `sfx_mcg_*`, `sfx_bsk_*`, `sfx_acd_*`, `sfx_mdg_*`, `sfx_shovel*`, `sfx_water_*`;
- sound descriptors: `wpn_<weapon>_<event>`, for example `wpn_minigun_shoot`, `wpn_basooka_fly`,
  `wpn_acidgun_recharge`, registered with `sncLinkSndListData` in each obj's static initialiser;
- textures through `txmMANAGER::Add`;
- rigid "ammo" props: `pwp_acd_ammo`, `pwp_amg_clip`, `pwp_clt_clip`, `pwp_crb_clip` and `pwp_mdg_clip`.

`re/work/sdk_weapons/strings_feb.txt` lists every string with the functions that use it. 1.2 has the same strings.

## 1.2 against Feb

The balance is the same: rate, damage, ammo, clips, projectile speeds and spread did not change.

1.2 changed:
- the multiplayer shot pipeline (`gsEVENT`, relay msg 0x4C8, net-id hit resolution, replay of the ray effects);
- the minigun sound, flare and shells;
- floor collision for ejected shells;
- the bazooka flight-sound range (10/45 m to 7/25 m) and impact flash (7/14 to 3/6);
- the acid trail;
- the **Medusa projectile**: one launch float went from 0.01 to 0.3, probably the collision radius, and the
  petrify cloud got hit boxes for players and one more NPC type.

Details are in `FEB_VS_12.md`.

## TWEAK table (v12)

| what | where (v12) | how |
|---|---|---|
| fire rate per weapon | `push imm32` before `call [edx+0xA0]` in each ProcessINIT (table in weapons.h) | imm at site+1 (float) |
| damage per weapon | `mov [esi+0x177], imm32` in ProcessINIT (weapons.h table) | imm at site+6 |
| max ammo | `mov [esi+0x173], imm32` | imm at site+6 |
| clip size | `mov [esi+0x1A7], ...` (some via a register) | site in weapons.h |
| Titan damage multiplier x4 | `pwpWEAPON::GetDamage` 0x00417710 `fmul [0x005F9870]` (pooled 4.0) | hook GetDamage, or patch the operand to point at another constant |
| infinite ammo (global) | `wrSysGO` 0x0064FF5C `+0xA4` != 0 | write the flag |
| infinite ammo per weapon | `wpnWEAPON::flags` `+0x16B` bit 4 (shovel sets it at 0x00415524) | |
| projectile speeds | fmul operand sites, results/weapon_npc.json PROJ_* (bazooka 50, acid 128, crossbow 297.6) | pooled constants: repoint the operand |
| Medusa ball radius (?) | ProcessSHOOT 0x004108F2 `mov [esp+0x2C], 0x3E99999A` (0.3) | imm at site+4 |
| machine-gun spread | 0x0040FEED (0.1 near), 0x0040FF08 (0.5 far) | imm at site+3 (`mov [ebp-0x14],imm32`) |
| minigun spread | 0x004137FD (0.45), 0x00413818 (2.0) | same |
| minigun shells per shot | 0x00413F6B `push 4.0`, 0x00413F70 `push 1.0` (`m3dRandRange`) | imm at site+1 |
| bazooka flight sound min/max | 0x004093CA (7.0), 0x004093D4 (25.0) | imm at site+6 |
| shell bounce plane | `StartShells` 0x0040B270 (0.5), 0x0040B280 (5.0) | imm at site+1 |

## HOOK points

- **Every shot (all weapons):** `wpnWEAPON::Shoot` 0x00543E20. The return value means fired or not.
- **The authoritative shot:** vslot +0x98 of each vtable. For bullet weapons and the shovel, hook vslot +0xB4
  (`ShootBody`, 0x0040AC80 / 0x00415B00) to see the real ray message after the relay.
- **Damage scaling:** `pwpWEAPON::GetDamage` 0x004176F0 (vslot +0xA4) is called for every hit and projectile.
- **Ammo use:** `pwpWEAPON::SubtractAmmo` 0x00417680 (vslot +0x64). Return 0 to block the shot.
- **Spread / pellets:** `FillCDTShootList` (vslot +0xF8) per bullet weapon. Base 0x0040B340; overrides are in
  weapons.h.
- **Projectile spawn:** each launcher's `ProcessSHOOT` (projectiles.h), or `entCreate` v12 0x005082B0 filtered by
  class name.
- **Projectile impact:** `ShowCollisSFX` and `DispatchDamage` per projectile class (projectiles.h).
- **Reload:** `pwpWEAPON::Recharge` 0x00417630.
- **Zoom:** `pwpWEAPON::SetZoom` 0x00418070.
- **Weapon-sound events:** `Anim2Sound` (vslot +0xB0) and `UpdateSound` (+0xAC).
- **New weapons:** register a class with `entRegisterClass` v12 0x00507D60, the way `pwp*Init` does. Derive from
  `pwpWEAPON` by copying a vtable (52 slots; 65 for bullet weapons) and replacing slots.

## PORTING notes

The weapon code itself has **no** Win32, DirectX, DirectSound or CRT-I/O imports: an IAT scan of every function in
plw_*/pwp_* finds no call. Everything goes through engine layers. A port has to provide these engine pieces,
grouped here by weapon-code call count (feb):

- **Particles and effects:** `partEMITTER_PHYS` (462 calls), `m3dSPL*` splines (412), `pteSPOT` / `pteSHADER`
  (decals and light spots), `objMOD_TEX_FRAME_ANIM`, `wrsfxSCORCHMARK`. These are the renderer side; see
  engine_render.
- **Animation and models:** `animINST` (147), `objOBJ` (94), `objFindName`, `_entAnimSetSeqName`, `dynDYNAMIC`.
- **Physics and collision:** `physPHYS` / `physMATH` (projectiles), `scnSCENE::ShootRay` / `IsVisiblePoint` /
  `FloorPlace`, `cdtREFINE`, `dmgCollectInst`.
- **Sound:** `gsSND_SYSTEM::AddChannel`, `sncCHANNEL` / `sncCHANNEL_3D` / `sncCHANNEL_SPH` (3D sound with min/max
  distance and cone; DirectSound3D underneath, in the sound subsystem).
- **Networking:** `gsMP_SYSTEM::FindEnt`, `gsMP_SERVER::SendMsgClientAll`, `gsEVENT`, and msgs 0x4C7/0x4C8.
- **Memory:** `operator new` / `delete` (MSVC CRT). Small sprintf calls build names such as `rocket%02d`.

Compiler and CPU issues a port must handle:
- Classes are **`#pragma pack(1)`** with odd offsets, using MSVC `thiscall` and single-inheritance vtables.
- Math is x87, with the inlined `m3dSqrt` fast paths using **SSE `rsqrtss`** (23 in v12) and **3DNow! (`femms`,
  0F 0F ops; 58 / 179 sites in v12)**, selected at run time. A non-x86 port must replace these inlined sqrt and
  vector paths. Reimplement `m3dSqrt` / `m3dNormalize` and recompile; do not translate the bytes.
- Gameplay timing relies on `gsElapsedTime` (the cooldown in `wpnWEAPON::ProcessFRAME`) and on animation action
  frames. Frame-rate independence is therefore inherited from the engine clock.

## Open questions

- The meaning of the Medusa launch fields changed in 1.2 (the float 0.01 to 0.3, the int 2 to 6).
- The exact `pjlPJL_INIT` layout (0x6D bytes in v12).
- The v12 `Create` addresses for the crossbow and the atomic gun, and their exact class sizes (rule: feb + 0x1C).
- The meaning of the new base vslots +0x8C (returns 1) and +0xCC (calls the sound channel's vslot 3).
- The minigun's 1.2-only fields at v12 `+0x1EF..+0x1F3`.
- The per-weapon own fields marked "// ?" in weapons.h.
- The starting ammo for most weapons. `SetAmmo` takes a register argument, so it was not resolved, except for the
  minigun (500).
