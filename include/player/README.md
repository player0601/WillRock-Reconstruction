# Player subsystem (Will Rock 1.2): unofficial SDK notes

Header: `player.h`. Evidence and helper scripts: `re/work/sdk_player/`. `al.py` aligns a feb function with
its v12 twin. `votes.txt` holds displacement votes across 120+ function pairs. `calls.py` propagates twins through
aligned call sites. `pairs_checked.json` and `ratios.json` give the alignment quality. `vt_player.txt` has the raw vtables.
Verified gameplay values come from `re/results/player.json` (91 items, high confidence).

## Objects

| class | entity name | v12 size (feb) | role |
|---|---|---|---|
| `plgsACTION : entENTITY` | (none) | ends at 0x1AB (0x23B) | "Player for the game layer": the AI reads its bread crumbs. Global `plgsPlayer`. |
| `plrPLAYER_COMMON : plgsACTION` | (none) | about 0x28B (?) | Shared data: weapons[11], health (entity +0xD0), armor, money, team, 3 powerups, movement params, pickups, MP sync and save/load. |
| `plrPLAYER : COMMON` | `player` ('PLYR') | 0x357 (0x3CF) | The local first-person player: input, physics, camera, use, timers. Global `plrPlayer`. |
| `plrPLAYER_MP : COMMON` | `player_mp` | 0x293 (0x313) | Remote players, and the local player's visible body (`plrPLAYER::body`). |
| `plrCAM_CTRL` | (none) | 0x4C (same) | Zoom FOV, recoil kick, death camera. |
| `plrHEIGHT_CTRL` | (none) | 0x20 (same) | Eye-height spring (crouch, steps). |
| `_plrMOVE_CTRL` | (none) | 8 | Smoothed forward/side axes (`_plrCtrlForw`, `_plrCtrlSide`). |
| `plrWPN_CHROME`, `plrBLOOD_HANDS` | (none) | (shaders) | View-model shaders (plr_sfx.obj). Blood hands is disabled by `wrAppState & 2` (DisplayBlood=0). |

## Lifecycle
1. `plrInit` (v12 0x0041D420) registers `player` and `player_mp` with `entRegisterClass` and loads the
   `plr_pwr_qd`/`plr_pwr_ua` textures. `plrCommInit` (0x004265A0) runs first.
2. The level creates `player`, which calls `plrPLAYER::Create` 0x0041D4D0 (new 0x357, inline ctor, sets `plrPlayer`).
3. `ProcessINIT` (slot 31, 0x0041D600) calls COMMON::ProcessINIT (0x00427160). That creates 3 sound voices and the
   11 `pwp_*` weapon entities. ProcessINIT then sets eye height 1.6, crouch 0.9 and gravity 30. It also creates
   `plrHEIGHT_CTRL`, `plrCAM_CTRL`, the `player_mp` body and the `sob_tra_camera`.
4. `ProcessINIT_GAME` (slot 47), `ProcessINIT_MAP` (slot 45: pitch 90, reads `[Multiplayer] PlayerSkin`), and
   `ProcessINIT_LEVEL` (slot 32: clears velocities, sets moveflag 0x8000000).
5. `SetDefaultValues` (slot 65 -> COMMON 0x00427760) sets health 100, armor 0 and money 0, plus starting weapons
   (colt 14 ammo, more on later `lev_N_M` levels).

## Per-frame flow (`plrPLAYER::ProcessFRAME`, v12 slot 33, 0x0041E0C0)
- `ProcessInput` (call site 0x0041E31E -> 0x0041F790) reads gsINP actions. It sets `maxSpeed`/`accel` every frame
  (walk 5/30, run 10/90) and handles weapon selection, ZOOM (-> camCtrl->ZoomNotify, sites 0x0041FEEC/0x0041FFCF),
  USE, the POWERUP0..2 keys (-> `ActivatePowerup`, sites 0x004201C3/0x00420216/0x00420269), TOGGLE_TEAM,
  SWITCH_MODE and, in 1.2 only, the DBG_1..9 cheat block (0x00420277..0x0042089C).
- `ControlFirstPerson` (0x004229B0) or `ControlFlyThrough` (0x00422570): jump 10.0, pitch clamp 5..175.
- `ProcessPhysics` (0x004208B0): gravity (30 / water 2 / none), ground friction 200, floor height.
- Weapon shoot -> `plrCAM_CTRL::ShootNotify` (site 0x0041E3AA) for camera kick.
- `UpdateTimers` (site 0x0041E5BA -> 0x004251A0): ticks the powerup timers in real time. Slow-mo powerup 2 sets
  the global time scale to 0.25 and resets it to 1.0 when it expires.
- `UpdateCamera` (0x00422340) -> `plrCAM_CTRL::Update` (site 0x00422416 -> 0x0042DB40), then `UpdateCursor` (aim ray).

## Damage, pain, HUD
- Damage arrives as `ProcessCDT_FIREABLE` (slot 50, 0x0041E730). It is skipped when `moveFlags & 4` (god).
  It applies armor through COMMON::GetHealthArmorDiff, then calls `uiSystem->ShowPlayerPain`
  (wrUI_SYSTEM slot 37 in 1.2, 36 in feb; v12 0x00496BA0). The flash colour and time are immediates at
  v12 0x0041EAB9 / 0x0041EAB2, and the shield/normal variants are at 0x0041EA75 / 0x0041EA88. Red or white is chosen by whether the Unholy Armor powerup is active.
- HUD values are pushed by message **0x40E** (feb 0x40C) `msgDATA{0,0x40,0xC,kind,value}` through
  `gsMP_SERVER::SendMsgClientAll`. Kinds: health -0x61, armor 5, money 0xB, powerup 6.

## Saving
Save/load and MP sync use the same path: `ProcessMP_WRITE_SYNC_DATA` (slot 14, v12 0x004297D0) and
`ProcessMP_READ_SYNC_DATA` (slot 15, 0x00429B80) serialise into `plrSYNC_DATA` (0x180 bytes, static storage
from `GetPlrSyncDataStorage` 0x004265D0). `plrSYNC_WPN_DATA` holds 12 bytes per weapon. feb also has
`IsCanSaveNow`/`IsCanLoadNow` (0x00422320/0x004223E0); I did not locate their v12 twins (open question).

## 1.2 vs feb differences seen
- **The layout was reordered between classes** (see the header banner). Fields moved from plrPLAYER into COMMON
  and back. plgsACTION shrank by 0xAC. Never port a feb offset with one delta.
- The vtable has 75 slots (feb 71): new slots 8, 36, 44, 53, 54 and 74, and the entENTITY MP_UPD_POS group was collapsed.
- Message id 0x40C -> 0x40E.
- SetDefaultValues (v12 0x00427760) can **keep weapons/ammo** (`flags2 & 4`, or `gsSysMP->mode != 1` and
  `wrSysGO->mode == 2` = COOP). feb always reset to the colt.
- PickHealth/PickArmor: feb refused a pickup when the value was above 99 (or 199). 1.2 refuses it when the value is at least 100 (or 200). The caps are unchanged.
- ProcessPhysics no longer writes `-gravity` into entity accel. UpdateTimers returns early when `gsAppState & 0x200`.
  ApplySpeed(set) sets moveflag 0x40000 when vy > 5. A 4th ActivatePowerup call site in ProcessMsg (0x0041DE12)
  re-activates slow-mo.
- DBG cheat handlers exist only in 1.2 (god, revive, kill-all, lock toggle; see `results/build_differences.md` 5).
- Unchanged: plrCAM_CTRL, plrHEIGHT_CTRL, powerup cost table {50,50,50}, powerup time 30 s, slow-mo 0.25.

## TWEAK table (v12)
| what | value | where to patch (v12) |
|---|---|---|
| walk max speed | 5.0 | imm32 at 0x0041F79E+6 |
| walk accel | 30.0 | 0x0041F7A8+6 |
| run max speed | 10.0 | 0x0041F844+6 |
| run accel | 90.0 | 0x0041F84E+6 |
| jump speed | 10.0 | 0x00422FE7+3 |
| gravity | 30.0 | 0x00421FE5+6 (ProcessPhysics) and 0x0041D642+6 (ProcessINIT) |
| water gravity | 2.0 | 0x00421FC0+6 |
| ground friction | 200.0 | const 0x005FB64C (**shared** with the 200 health/armor cap: patch the operand at 0x00421ACF to point to your own float instead) |
| pitch max / min | 175 / 5 | consts 0x005FB6C8 / 0x005F980C, used at 0x00422A45 / 0x00422A66 (repoint the operands, they may be shared) |
| eye height / crouch factor | 1.6 / 0.9 | 0x0041D62C+6 / 0x0041D638+6 |
| start health | 100.0 | 0x004277B9+6 |
| armor cap (small / big) | 100 / 200 | stores in PickArmor 0x00428A91+6 (100); 200 store follows 0x00428A9D |
| health caps | 100 / 200 | PickHealth 0x00428900.. (thresholds: <11 or >=150 counts as big; 11.0 const 0x005FB6F4) |
| powerup duration | 30.0 | in ActivatePowerup 0x004257B0 (imm 0x41F00000) |
| slow-mo scale | 0.25 | in ActivatePowerup (push 0x3E800000) |
| powerup gold cost | 50,50,50 | `_plrPwrCostTbl` 0x0062B868 (3 floats, data) |
| weapon count | 11 | loops `cmp 0x2C`; INIT_LEVEL `mov [esi+0x263],0xB` at 0x0041D889 |

## HOOK points (v12)
| hook | address | notes |
|---|---|---|
| player per-frame | vtable 0x005FB4F8 slot 33 (0x0041E0C0) | or call sites listed above |
| input | ProcessInput 0x0041F790 (call site 0x0041E31E); gsINP GetInput detour 0x004B67F0 (steal 5: `53 8b d9 55 56`) | |
| damage taken | slot 50 0x0041E730 | return without calling = invulnerable |
| pickups | slots 55..62 (weapon, ammo, health, armor, powerup, key, money, treasure) | return 0 = refuse pickup |
| powerup activation | ActivatePowerup 0x004257B0 (4 callers) | |
| death / revive | StartDeath slot 69 0x004247D0; Revitalize slot 67 0x0041F110 | |
| defaults/new game | SetDefaultValues slot 65 0x0041F0E0 | |
| camera | plrCAM_CTRL::Update 0x0042DB40 (site 0x00422416); ShootNotify 0x0042D9C0 | |
| pain flash | uiSystem (v12 0x00994568) vtable slot 37 | |
| aim | UpdateCursor 0x00423DE0; globals plrAimEnt 0x0098EAC4, plrAimPoint 0x0098EAA4 | |
| save/load | slots 14/15 | append your own data carefully: the plrSYNC_DATA size is fixed |

## PORTING notes
- **No direct OS / DirectX calls in the player code.** A scan of v12 0x0041D100..0x0042F000 found no import
  calls; the only hit, CoUninitialize at 0x0041D4B9, is junk padding. Everything goes through engine services:
  - input: `gsINP_SYSTEM` (IsOn/IsToggled/Power, key state at +0x188C). The DirectInput backend lives in the
    platform driver subsystem, so a port replaces that backend, not this code.
  - camera: `gsCameraPtr` camCAMERA (SetAngleHor, GetOrigin, GetLookAt; FOV at +0x14C).
  - sound: `gsSND_SYSTEM::AddChannel`, sncCHANNEL_SPH (3 per player).
  - time: `gsElapsedTime` (v12 0x0098E9F4), `gsMsgSetElapsedTimeScale` / `gsMsgGetElapsedTimeScale`.
  - config: `apCfgReadInt("Multiplayer","PlayerSkin")` (registry/INI backend in app).
  - UI: `uiSystem` vtable (ShowPlayerPain, ShowMsgW slot 9), `wrMSG_SYSTEM::ShowMsg`.
  - MP: `gsSysServer->SendMsgClientAll`, `gsSysClient` vtable +0x2C, `gsSysMP` (mode at +0x28).
- Floating point is x87 with float constants shared across the binary. A port must keep the float32 behaviour
  (for example the `fcomp`/`test ah,5` pickup tests) if MP demo/sync compatibility matters.
- The structures are `#pragma pack(1)` with odd offsets. A port should rebuild them as normal structs and
  keep the save format separate (`plrSYNC_DATA`).

## Open questions
- Exact COMMON / plrPLAYER boundary in v12 (about 0x28B) and the meaning of many `unk_*` fields.
- v12 twins of ProcessUse, IsCanSaveNow/IsCanLoadNow, SelectPosDirCOOP, RevitalizeStart, GetUnderwaterTimeLeft
  and IsIgnoreDmg.
- The new vtable slots 36, 44, 53, 54 and 74 (0x00425E60, 0x00427590, 0x0041F040, 0x0041DAB0, 0x00425970): what they do.
- The low-confidence twins COMMON::Revitalize 0x00428E50 and COMMON::ProcessCDT_FIREABLE 0x00428200.
