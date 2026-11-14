# game_shared: shared gameplay objects (Will Rock 1.2)

Part of the unofficial Will Rock modding SDK and porting map. Built by static analysis of `re/bin/v12.exe` (1.2),
using the Feb 2003 build (`feb.exe`, `WillRock.map`, Ghidra decompile) as the reference. Nothing was executed.

| file | contents |
|---|---|
| `damage.h` | `dmgDAMAGE` / `dmgGENERAL` / `dmgWEAPON` damage payloads (the **1.2 layout**), damage types, `dmgDISPATCHER`, the explosion-damage API |
| `world_objects.h` | `itemITEM` (pickups), `doorDOOR`, `bttnBUTTON`, `trmpTRAMPOLINE` (jump pad), `dstrDESTROY` (destructible props), `gmSharedInit` order |
| `weapon_base.h` | `wpnWEAPON` base weapon, what game_shared needs from `plgsACTION`, a note on the `pjlPJL<SFX>` projectile template |

Address convention: `// v12 0x00XXXXXX (feb 0x00XXXXXX)` and `// v12 +0xNN (feb +0xNN)`. `// ?` = not confirmed by
a v12 instruction. Fields marked "verified" were read from v12 code (instruction address given in the header).

## What is in gs_shared, and what this SDK covers

`gs_shared` is the game-independent gameplay library (Saber reused it between projects). `gmSharedInit`
(v12 0x00557AE0) starts all of it:

| obj | prefix | what it is | covered here |
|---|---|---|---|
| damage | `dmg` | damage messages, radial explosion damage | **yes** |
| item | `item` | pickups (base class; concrete pickups are game side, `wrItemInit`) | **yes** |
| door | `door` | doors | **yes** |
| button | `bttn` | buttons and levers | **yes** |
| trampoline | `trmp` | jump pads | **yes** |
| dyn_destroy | `dstr` | breakable props and debris | **yes** |
| weapon | `wpn` | weapon base class | **yes** (base only; pwp* belong to weapons) |
| plgs_action | `plgs` | player base used by AI and objects | interface only (player subsystem) |
| gm_shared | `gmShared` | init/term | yes |
| ai_*, aia_arena, aiw_wave | `ai` | the AI framework (controllers, planner, spawn, arenas, waves) | no: AI subsystem |
| cinema | `cin` | cutscene scripts | no |
| dom_misc | `dom` | trigger volumes: `domKILL_DOMAIN` (kill zone, sends GS_MSG_DAMAGE), `domTPT_DOMAIN` (teleport), `domACT_DOMAIN` (IAction trigger), `domCLN_DOMAIN` | listed only |
| water, ripple, caustics, flare, blink_light, runway_lgt, part_eff | `water/rpl/cau/flr/blkl/rwl/peff` | visual level effects | no: render/effects |

Projectiles (`pjl*`) are not in gs_shared: they are a header template `pjlPJL<SFX> : physPHYS` instantiated
in each weapon/NPC obj (`plw_*`, `ai_*_pjl`). Explosions are `dmgDispatchExplosDamage` plus a visual sfx owned by the caller.

## How it works

**Creation.** Each class registers a factory with `entRegisterClass(name, signature, 0x28, Create, ...)`
(item 'MTI$', door 'ROOD', button 'NTTB', trampoline 'PMRT', dyn_destroy 'TSD$'). The level loader creates an entity
for every template instance whose class name matches, and sends msg 1 (INIT). `ProcessINIT` reads the instance property
sheet (sections `ITEM`, `DOOR`, `BUTTON`, `TRAMPOLINE`, `DESTROY`), looks up animation sequences by name
(`OPEN`/`CLOSE`, `PUSH`/`POP`) and registers for MP sync (`gsMP_SERVER::DeclSyncable`).

**Per frame** (msg 1000): item spins and bobs and tests every player (`gsMP_SERVER::GetNextPlayer`) with `IsCanPick`
and then `Pick`. Door: `IsReadyOpen` scans players (and NPCs when allowed) within `radOpen`, honours one-way and
lock count, and plays OPEN/CLOSE. Button advances its animation and broadcasts its state to clients. dyn_destroy fades
the debris after the break.

**Logic wiring.** Objects talk to level scripts through IActions: the named command message `GS_MSG_IACTION` (0x3F8,
apNAME at data+8: `PICK`, `UNLOCK`, `OPEN`, `CLOSE`, `PUSH`, `POP`) comes in, and event bits are raised on entity +0x84
for scripts to react (button: 1 push start, 2 pop start, 4 pushed, 8 popped; item: 0x20 picked). The lock count at
entIACTIVE_OBJ +0x30 (`nmbLocks` / `nmbPicks`) gates doors and buttons: an UNLOCK decrements it.

**Damage.** A hit is `msgSystem.SendMsg(target, GS_MSG_DAMAGE = 0x4C6, dmg, attacker)`. Weapons fill a `dmgWEAPON` through
`wpnWEAPON::MakeDamage`. Explosions call `dmgDispatchExplosDamage`: it collects up to 50 animINSTs in range, then for each one
works out the falloff (full damage inside radius/3, 25% at the radius, linear in between), checks line of sight, and sends the
message (or calls a `dmgDISPATCHER`). `dstrDESTROY` adds up `amount` until it reaches `DESTROY.damage`, then `StartExplos`.

**Multiplayer.** All of these are server-authoritative: clients send requests (`SendMP_Client_2_Server`, e.g. button
toggle), and the server sends `GS_MSG_MP_NOTIFY` (0x40E in 1.2) sub-codes back (item: 1 move-on-unlock with position,
2 unlock; button: -99 state, -100 sync, 0 toggle request). Join-time state goes through `ProcessMP_GET/WRITE/READ_SYNC_DATA`.
See `re/mpsrc`.

**Data files/formats.** Nothing of its own: everything comes from level template instance properties (`.tpl`/scene,
read by engine_core `sio*`) and model object names (`tip`, `geom_orig`, `geom_debris`, `geom_debris_left`).

## Property keys (for level makers)

| section | key | default | effect |
|---|---|---|---|
| ITEM | isPlaceGround | 1 | drop to floor |
| ITEM | isMoveUponUnlock | 0 | on unlock, appear above whoever unlocked it |
| ITEM | isRotated | 1 | spin 70 deg/s |
| ITEM | nmbPicks | 0 | hidden until picked N times (lock count) |
| ITEM | sfxName | "sfx_item" | attached effect |
| ITEM | height / heightDisp | 0.95 / 0.6 | lift on unlock / bob amplitude (x0.4) |
| DOOR | nmbLocks, isOneWay, isOpenOnce, isOpen, isOpenForever | 0 | see doorFLAGS |
| DOOR | radOpen | 18.0 (feb 15.0) | trigger radius; key needed within radOpen/2 |
| BUTTON | nmbLocks, isPushOnce, isCanShoot | 0 | isCanShoot: damage pushes it |
| TRAMPOLINE | height, target, speedXZ, randShift | 20, -, 0, 0 | with target: ballistic throw to it; height auto 15..30 |
| DESTROY | damage, iniSpeed, iniMomentum, isPlaceGround | 2.0, 4.0, 0.05 | HP, debris launch speed and spin |

## 1.2 vs feb differences seen

- **Layout:** every field of these classes moved +0x1C (entENTITY grew). Sizes item 0x193->0x1AF, door 0x16B->0x187,
  button 0x15F->0x17B, trampoline 0x17B->0x197, dyn_destroy 0x167->0x183 (checked in each v12 `Create`). animINST* at
  entity +0xC0 (feb +0xBC).
- **Vtables:** one slot inserted at +0x28 and one dropped before the ProcessINIT block, so subclass slots from ProcessINIT on keep
  the feb slot number minus 1 (ProcessINIT +0x68 -> +0x64), and the MP slots +0x30..+0x38 move to +0x34..+0x3C.
- **dmgDAMAGE** went from 16 to 14 bytes: byte type + new subtype byte, so `amount` is at +6 and `sourceId` at +0xA. The type numbers moved up by one (0 is new).
  dmgGENERAL 0x1C->0x1A, dmgWEAPON 0x32->0x30.
- **Message ids:** IACTION stays 0x3F8; query 0x3FB->0x3FC; MP_NOTIFY 0x40C->0x40E; damage 0x4C5->0x4C6.
- **Explosions:** `dmgDispatchExplosDamage` now returns at once on an MP client (gsSysMP->mode == 3). The visibility test
  shoots a second ray (the eye point is offset by 1.5 and 1.0) before it rejects a target. // ? exact second-ray geometry
- **Door range test:** 1.2 factors the per-actor test into a new helper (v12 0x0055BB20) that also accepts NPCs (signature 'CPN$', position from vslot +0x54) and inlines IsPlrBeforeDoor (angle < 90 deg).
- **Tuning:** item pickup vertical tolerance 2.0 -> 2.3, door default radius 15 -> 18.
- v12 has junk bytes after `ret` that decode as fake imported calls (ShellExecuteA, RegCloseKey, ...) inside this range;
  they are never executed. Do not list them as dependencies.

## TWEAK table (v12)

| what | value | v12 address | notes |
|---|---|---|---|
| door default radOpen | 18.0 | 0x00637944 | one reader (door INIT 0x0055B5B8), safe to patch the constant |
| item pickup horizontal radius | 2.5 | const 0x005F98A4, read at 0x0055741B | **pooled** (38 users): patch the fcomp operand at 0x0055741D to point at your own float |
| item pickup vertical tolerance | 2.3 | const 0x005FA974, read at 0x00557440 | pooled? operand at 0x00557442 |
| item spin speed | 70 deg/s | 0x005FB63C, read at 0x005571CF | pooled |
| item bob period | 2.0 | immediate at 0x00556FF2 (`mov [esi+0x17F],0x40000000`) | patch the imm32 at 0x00556FF8 |
| explosion inner radius factor | 1/3 | 0x005FA978, read at 0x005466FE | pooled |
| explosion edge damage factor | 0.25 | 0x005F9ED0, read at 0x00546725 | pooled |
| explosion LOS eye height | 1.5 | 0x005F93A4, read at 0x0054684D / 0x00546991 | pooled |
| explosion max targets | 50 | `push 0x32` at 0x00546438 | buffer is 50 entries, do not raise |
| trampoline auto height 15/30 over 20..60 m | | 0x005FA490 / 0x005FA4C8 / 0x005F93F0 / 0x005FB65C | pooled |
| weapon defaults (rate 1, maxAmmo 100, dmg 10, ammo 10) | | ctor 0x00543CC0 immediates | subclasses overwrite |
| damage msg id | 0x4C6 | | for sending damage from a mod |

## HOOK points

| hook | v12 | use |
|---|---|---|
| `gmSharedInit` | 0x00557AE0 | after it returns, call `entRegisterClass` for your own entity classes |
| `dmgDispatchExplosDamage` | 0x00546410 | global explosion scaling, friendly-fire rules |
| `dmgDispatchExplosDamageInst` | 0x005464C0 | per-target explosion filter |
| `dmgDISPATCHER::Dispatch` | 0x005463E0 | default damage delivery |
| any entity `ProcessMsg` case 0x4C6 | | damage received |
| `itemITEM` vslot +0x94 IsCanPick / +0x90 Pick | 0x00557340 / 0x005575B0 | pickup range, pickup effects (patch the vtable 0x00605F88 or subclass vtables) |
| `doorDOOR::IsReadyOpen` vslot +0x70 | 0x0055B990 | force doors open/locked |
| `bttnBUTTON::Toggle` vslot +0x64 | 0x005569D0 | button use |
| `dstrDESTROY` vslot +0x70 / +0x80 | 0x00554370 / 0x00554570 | breakable damage / break |
| `wpnWEAPON::MakeDamage` vslot +0x74, `GetDamage` +0xA4 | 0x00543ED0 / 0x005440F0 | weapon damage |

## PORTING notes

The objects in this subsystem make **no OS, Win32 or DirectX calls.** Everything goes through engine APIs that a port has to
supply anyway:
- **engine_core:** `msgSYSTEM::SendMsg/RegisterMsgData`, `entRegisterClass`, `animINST` (GetBool/GetInt/GetFloat/GetStr property access,
  SetAnimSeq, AdvanceTime, GetPos/GetDir/Translate, Validate/Invalidate), `animTPL::FindSeqName`, `objFindName`,
  `objOBJ::SetStateProcYes/No/SetTransparency/GetOrigin`, `scnSCENE::MakeAnimList/IsVisiblePoint/FindInstName/SetStateYesInst`,
  `rgdSHATTER_RIGID_START` (debris physics), `cgmGetThrowSpeed_H`.
- **math (math_app):** `m3dSqrt` comes in three paths chosen by `m3dSimdType` (0 SSE `rsqrtss`, 1 3DNow! `pfrsqrt`, 2 x87). A
  port must keep the x87/C path only: 3DNow! does not exist on modern CPUs and is never available outside x86. `m3dRandMax/RandRange`,
  `m3dMakeVTangRandom`, `m3dUpdateTimeField`.
- **sound:** `gsSND_SYSTEM::Play`, `sncLinkSndListData` (platform_drv sound).
- **MP:** `gsMP_SERVER::DeclSyncable/GetNextPlayer/SendMsgClientAll`, `entIACTIVE_OBJ::SendMP_*`, `gsMP_SYSTEM::FindEnt/IsRegularMPID` (re/mpsrc).
- **Data layout:** all structs are `#pragma pack(1)` and the damage payload has unaligned floats and dwords (+6, +0xA). On ARM or
  any other strict-alignment target, read them with memcpy. Save and network data (`ProcessMP_*_SYNC_DATA`) is little-endian raw struct
  memory.
- These are x86 `__thiscall` / `__fastcall` with packed, odd offsets. A source port should re-declare the classes rather than
  overlaying memory.

## Open questions

- v12 addresses still to confirm: `dmgWEAPON_PACK` vtable, the `plgsACTION` vtable and `plgsPlayer` (resolved in this pass: itemITEM::Unlock 0x00557460, trmp GetSpeedThrow 0x00558920, dmgIsHiddenObstacles 0x00546930, dmgGetOrgDir 0x00546A20, wpnWEAPON::SetAmmo 0x00543D60 / AddAmmo 0x00543D70; doorDOOR::IsPlrBeforeDoor is inlined in 1.2 into helper 0x0055BB20).
  `dmgIsHiddenObstacles`, `dmgGetOrgDir`, `dmgWEAPON_PACK` vtable, the `plgsACTION` vtable and `plgsPlayer`.
- `dmgDAMAGE` types 3 and 4 (feb 2 and 3) are only partly named. The exact geometry of the 1.2 second LOS ray is not confirmed.
- The `dstrDESTROY` and `trmpTRAMPOLINE` field offsets follow the verified +0x1C rule and their sizes. Only one field each was re-read in v12.
