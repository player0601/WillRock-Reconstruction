# Will Rock unofficial modding SDK and porting map

Will Rock (Saber Interactive, 2003) was never released with source code or a modding SDK. This folder is a
reconstruction made 23 years later by **static analysis only** (the executables were never run). It contains:

- C++ headers that describe the game's classes, memory layouts, virtual tables, globals and functions, with the
  address of everything in the **1.2 patch** (`v12.exe`), and the address in the **February 2003 beta** (`feb.exe`,
  the build that has a full symbol map) next to it.
- One README per subsystem: how it works, what changed between the beta and 1.2, a TWEAK table (constants and limits
  you can patch, with addresses), HOOK points, and PORTING notes.
- `PORTING.md`: every Windows/DirectX dependency of the game, grouped, with what replaces it on Linux, macOS and
  Android, a realistic porting plan and the open questions.

The reconstructed multiplayer sources in `re/mpsrc/` (13 files, ~8,600 lines) build against these headers (syntax
check, see "Build status").

This is unofficial work. Copy protection, CD keys and licensing were not analysed or documented. The Ubi.com
lobby libraries (libgs*) are out of scope.

## Conventions used in every header

| notation | meaning |
|---|---|
| `// v12 0x00XXXXXX (feb 0x00XXXXXX)` | address of a function or global in 1.2 and in the Feb 2003 beta |
| `// v12 +0xNN (feb +0xNN)` | offset of a field in 1.2 and in the beta (they often differ, see below) |
| `// slot +0xNN v12 0x... (feb 0x...)` | virtual-table slot and its implementation in both builds |
| `// ?` | not confirmed by a v12 instruction: treat as a lead, check it before relying on it |
| `[integrator] ...` / `re/mpsrc` | added while merging the subsystems or to fit the reconstructed multiplayer sources |

All classes are MSVC 6, 32-bit, `__thiscall`, `#pragma pack(1)` where the game has odd offsets. Free functions are
mostly `__fastcall` (the declarations were checked against the beta's symbol mangling: 234 checked, 117 fixed by the
integrator). Text is UTF-16 (`wrWCHAR`, which is `wchar_t` on Windows).

## The three builds

| id | file | date | use |
|---|---|---|---|
| feb | `re/bin/feb.exe` | 2003-02-21 | beta with `db/WillRock.map`: every function and global is named. WRML (the existing mod loader) targets this build today. |
| jun | `re/bin/jun.exe` | 2003-06-16 | 1.0 retail, no symbols |
| v12 | `re/bin/v12.exe` | 2003-07-30 | **official 1.2 patch, the SDK's target**. Contains a third-party widescreen patch (code caves at 0x005F80ED..0x005F8270 and section .text1 at 0x009E8000). |

Addresses are fixed: the images have no relocations and load at 0x400000.

**Do not reuse beta offsets on 1.2.** The most important differences:
- The entity base grew: `entENTITY` is 0x16B bytes in 1.2 (0x14F in the beta), so every field of every
  `entENTITY` subclass (player, enemies, weapons, items, effects) is **+0x1C** in 1.2. Classes derived only from
  `entIACTIVE_OBJ` (domains, game rules) moved **+4**.
- Virtual tables changed: `entIACTIVE_OBJ` gained slot 8, `entENTITY` lost three MP slots and gained one, so subclass
  virtuals start at slot 25 instead of 26. Each subsystem header gives its own slot-mapping rule (player, AI, weapons, UI).
- The player object was reordered, not just shifted (`player/player.h` lists each region).
- Message ids above about 0x3F5 were renumbered (not by a fixed amount); `game_systems/gs_common.h` has the 1.2 table.
- The particle emitter gained a float at +0x34 (fields after it are +4).

## Subsystem map

| folder | covers | main headers |
|---|---|---|
| `core/` | **start here**: the shared core (one definition of every base type), the main globals table, calling-convention shims, compat headers | `wr_core.h`, `wr_bases.h`, `compat/` |
| `wr_sdk.h` | umbrella header: includes every subsystem | |
| `engine_core/` | message system (`msgSYSTEM`, `msgADDR`, `msgDATA`), `arrVector`, `apNAME`, objects, animation, collision, physics, entities and the entity class registry, files and the scene/template readers, frame clock | `core_types.h`, `entity.h`, `object.h`, `anim.h`, `collision.h`, `physics.h`, `fileio.h` |
| `math_app/` | m3d math library (vectors, matrices, quaternions, volumes, splines, noise) and the application framework (WinMain, init order, main loop, window procedure, command line); `win32_imports.tsv` = all 202 imports with call sites | `m3d.h`, `app.h` |
| `engine_render/` | renderer: scene, textures, CPU shaders (pte*), particles, the DirectDraw 7 / Direct3D 7 driver classes, level file chunks | `rend_driver.h`, `d3d_driver.h`, `scene.h`, `texture.h`, `shader.h`, `particle.h` |
| `platform_drv/` | DirectInput 7 input, DirectSound 8 + DirectShow sound/music/movies, Win32 application shell, ini config, full import table with replacements | `input.h`, `sound_drv.h`, `app_win.h` |
| `game_systems/` | level system, save/load, sound and music systems, render frame driver, cinematics, domains (triggers), visibility, strings, input contexts, message ids | `gs_common.h`, `level.h`, `saveload.h`, `domains.h`, ... |
| `game_shared/` | damage, explosions, items, doors, buttons, trampolines, destructibles, the weapon base | `damage.h`, `world_objects.h`, `weapon_base.h` |
| `player/` | the player classes (single player and MP), camera and height controllers, powerups, input actions | `player.h` |
| `weapons/` | the 11 player weapons and their projectiles; `FEB_VS_12.md` | `weapons.h`, `projectiles.h` |
| `ai/` | 23 enemy classes, the shared NPC base (116 virtual slots), the AI director, spawn areas, arenas; `FEB_VS_12.md` | `ai_npc.h`, `ai_world.h`, `enemies.h` |
| `sfx_sob/` | 60 effect entities (`sfx_*`) and 10 scene objects (`sob_*`), the effect spawn API, decals | `particles.h`, `sfx_api.h`, `sfx_entities.h`, `sob_entities.h` |
| `frontend/` | UI library (widgets, fonts, registry), the Will Rock UI and HUD, game rules (`wrGO_SYSTEM`), game options, debug keys, statistics | `ui_core.h`, `wr_ui.h`, `wr_game.h` |
| `re/mpsrc/` (outside) | reconstructed multiplayer source: network driver (DirectPlay 8), server, client, sync, MP UI windows, game rules MP part | `MULTIPLAYER_GUIDE.md` |

Each folder's `README.md` has the TWEAK and HOOK tables; `function_index.tsv` (or `functions_index.h`,
`m3d_index.h`) maps every beta function of that subsystem to its 1.2 address and says how it was matched.

## How a modder uses it

1. **Build a 32-bit DLL.** Use MSVC (x86) or clang-cl, or mingw `-m32`. Include `wr_sdk.h` (add `re/sdk` and
   `re/sdk/core` to the include path). Define `WR_REQUIRE_32BIT` to get a compile-time check. On non-Windows hosts,
   `-fshort-wchar` gives the game's 2-byte text type.
2. **Get the DLL into the process.** The existing loader, WRML (`modding/`), is a `dinput.dll` proxy: the game imports
   exactly one function from `DINPUT.dll` (`DirectInputCreateW`), so a `dinput.dll` in the game folder loads first.
   WRML verifies the bytes at every address before it patches. Today WRML supports the beta build; the addresses in
   this SDK are what a 1.2 port of WRML needs (`modding/tools/gen_addr.py` is where they go).
3. **Hook by address.** Typical hook points (1.2 addresses; each README has more):

   | what | where |
   |---|---|
   | one game frame | `gsMsgCallFrame` 0x00510B90 |
   | every message to every object | `msgSYSTEM::Dispatch` 0x004D0340 |
   | register or replace an entity class (spawn names from levels) | `entRegisterClass` 0x00507D60; best place: `gmSharedInit` 0x00557AE0 or `InitSubsys` (vtable slot at 0x00606238) |
   | create / iterate entities, spawn any effect by name | `entCreate` 0x005082B0, `entFindNext` 0x005086A0 |
   | player per frame / damage taken | `plrPLAYER` vtable 0x005FB4F8 slot 33 (0x0041E0C0) / slot 50 (0x0041E730) |
   | weapon shot / damage | `wpnWEAPON::Shoot` 0x00543E20, `GetDamage` 0x004176F0 |
   | explosions | `dmgDispatchExplosDamage` 0x00546410 |
   | level load | `gsLVL_SYSTEM::ProcessMsg` 0x00537540, main load 0x00536950 |
   | render frame | `gsRENDER_SYSTEM::RenderFrame` 0x0051BA50 |
   | texture by name / new CPU shader type | `txmMANAGER::Add` 0x004B3410 / `pteMANAGER::RegisterClass` 0x004D8480 |
   | UI: open a page / HUD text / new widget type | 0x0048D5B0 / 0x0048F800 / `uiElementRegister` 0x0051AB00 |
   | engine log | log callback pointers 0x00655160, 0x00652060, 0x00654158, 0x00655164 |

4. **Read and write game state** through the globals table at the end of `core/wr_core.h` (every v12 address there
   was re-checked to have v12 instruction references), e.g. `gsSysLevel`, `uiSystem`, `wrSysGO`, `msgSystem`,
   `gsElapsedTime`, `gsTimeScale`.
5. **Patch tweaks.** Each README's TWEAK table gives the instruction address of a limit or constant. Several
   floating-point constants are shared by unrelated code (for example the 200.0 used by both ground friction and the
   health cap): patch the instruction operand, not the constant.

Calling the game: declare a function pointer with the right convention and the v12 address, e.g.
`auto entCreate = (entENTITY *(__fastcall *)(scnSCENE*, char*, char*, animCREATE_DATA*, void*, int))0x005082B0;`
Member functions are `__thiscall` (call them through a `__fastcall` thunk with a dummy EDX, or with MSVC's
`__thiscall` pointer-to-member casts).

## Can it be compiled again? (status)

- **For mods: yes.** Every header compiles on its own and through `wr_sdk.h`, and a 32-bit layout check
  (`re/work/sdk_core/layout_check.cpp`) asserts 32 class sizes and key offsets (entity 0x16B, player 0x357,
  NPC 0x543/0x567, UI 0x6C/0x16C, game rules 0x11C, emitter 0x2C4 ...) and passes.
- **The multiplayer sources (`re/mpsrc`): yes, as a syntax check.** All 13 `.cpp` files pass
  `g++ -std=c++17 -fsyntax-only -fpermissive -w -I re/sdk -I re/sdk/core -I re/mpsrc` (they also pass without
  `-fpermissive` except for 18 const/void-pointer conversions). They do not link into a game: they reference engine
  functions that exist only inside the exe.
- **The whole game: not from this.** Only the multiplayer layer has function bodies. The other subsystems are
  described (layouts, addresses, behaviour) but their code is still machine code in the exe. A full source port means
  rewriting those bodies from the beta decompile the way `re/mpsrc` was done; `PORTING.md` sizes that work.

### Build status (integrator run)

| check | result |
|---|---|
| each `re/sdk/**/*.h` alone (`g++ -std=c++17 -fsyntax-only -fpermissive -w -I re/sdk -I re/sdk/core`) | 53 / 53 pass (incl. `wr_sdk.h` and `core/compat/`) |
| `re/sdk/wr_sdk.h` (everything together) | pass |
| 32-bit layout check (`g++ -m32 -ffreestanding -fshort-wchar ... layout_check.cpp`) | pass |
| `re/mpsrc/*.cpp` (the required command) | 13 / 13 pass |

Scripts: `re/work/sdk_core/check_h.sh`, `check_mp.sh`; the MP integration is reproducible from the untouched copy in
`re/work/sdk_core/mpsrc_orig/` with `python3 re/work/sdk_core/apply_mp.py` (runs `merge_mp.py` and `fix_mp.py`).

### What the integrator changed to get there

- Wrote `core/wr_core.h` (one definition of each core type, the globals table, forward declarations for every
  subsystem class) and `core/wr_bases.h` (padded `dynDYNAMIC`, `rgdRIGID`, `waterLAVA`, `flrFLARE` bases).
- Merged duplicate definitions, keeping the losing copy under `#if 0` as documentation: the particle emitter
  (engine_render + sfx_sob layouts, checked in v12: `+0x38` is the ARGB colour, default 0x7FFFFFFF, not "maxEmit"),
  `dstrDESTROY`, `gsAPP_STATE`/`gsWIN_APP_SYSTEM`, `uiCHILD_NOTIFY` (`type` at +4, `code` at +8),
  `wrGO_SYSTEM` (+0xC0..+0x10C re-read in v12: winner name, winner team, game-over flag, 5 s time sync; the frontend's
  earlier "chat" guesses were wrong), and 18 duplicate message and driver structs between `re/mpsrc` files.
- Declared the virtual slots of `entIACTIVE_OBJ` (4..15) and `entENTITY` (16..24), `animCREATE_DATA` (0x96 bytes,
  from its v12 constructor), `wrSTAT_SYSTEM`, `gsMP_COMM`, the UI list/scroll templates and render elements.
- Fixed 117 free-function calling conventions against the beta mangling.
- Where two reconstructions used different names for the same bytes, both names are kept as anonymous-union aliases
  (same offset) or inline forwarding methods (same slot). Nothing was removed from `re/mpsrc`; the few call-site edits
  are marked `[integrator]` (renames, a misplaced parenthesis in `ShowMPMsg`, explicit argument casts).

See `PORTING.md` for the platform inventory, the porting plan and the open questions.
