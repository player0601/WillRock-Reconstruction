# Porting Will Rock 1.2: platform inventory, plan and open questions

This file brings together the porting notes of every subsystem (`platform_drv/README.md` has the full 202-row
import table, `math_app/win32_imports.tsv` the raw call sites). Addresses are v12 (1.2) unless noted.

## 1. What the game really uses

| area | API actually used (checked against the import table) | notes |
|---|---|---|
| Graphics | **DirectDraw 7 + Direct3D 7** (`DirectDrawCreateEx`, `DirectDrawEnumerateA/ExA`), fixed-function pipeline | not D3D8/9. A COM DxDiag query only checks that DirectX 9 is installed (`_gsDetectDx9` 0x004A4640). |
| Input | **DirectInput 7** (`DirectInputCreateW`, version 0x700) | keyboard and mouse only: joystick init is a stub returning 0 in both builds (0x00502570). |
| Sound | **DirectSound 8** (`DirectSoundCreate8`, by ordinal), 3D buffers and listener | the beta used `DirectSoundCreate`. |
| Music, movies | **DirectShow** (COM graph) | two MPEG-1 movies (`..\movies\ubilogo.mpg`, `saber3d.mpg`); no Bink. |
| Network | **DirectPlay 8** client/server over TCP/IP (COM, `CoCreateInstance`), plus the Ubi.com lobby | DirectPlay has no static import; WS2_32 (25 imports) is used by the statically linked Ubi.com library (`clSocket`), which is out of scope. |
| Window, loop | USER32 (56 imports): class, window, message pump, dialogs, message boxes | single window, single thread, variable time step |
| Files | CRT `fopen/fread/fseek/stat/access`, `_findfirst/_findnext` | loose files, no archive format; case-insensitive lookups; `\` separator (0x00634B70) |
| Config | `GetPrivateProfileStringA` / `WritePrivateProfileStringA` (ini), 3 registry reads (language, German low-gore flag) | |
| Time | `timeGetTime`, `QueryPerformanceCounter/Frequency`, `Sleep` | 200 FPS cap by `Sleep` (0x00510A54), dt clamped to 0.1 s |
| GDI | 8 imports: BMP loading through `LoadImageA` + `GetPixel` loop (`pctReadBMP` 0x00504FA0), desktop colour depth | |
| Misc | `ShellExecuteA` (website button), `GetOpenFileNameA` (dev dialog), `SetUnhandledExceptionFilter` (crash log), `CoInitialize` | |
| C runtime | statically linked MSVC 6 CRT (66 of the 95 KERNEL32 imports are CRT-only) | `qsort` order and x87 precision matter for sync, see §4 |

Code that makes **no** OS calls at all: entities, AI, player, weapons, damage, physics, collision, particles, the CPU
"pte" shaders, animation, scene management, UI widgets and game rules. They only talk to the engine through the
classes listed below, which is what makes a port tractable.

## 2. The replaceable layers (what a port writes)

| layer | classes / functions (v12) | replace with (Linux / macOS / Android) |
|---|---|---|
| Window + main loop | `gsWIN_APP_SYSTEM` vtable 0x00605CC8 (12 slots), `_gsWndProc` 0x004A4E00, `MainLoop` 0x004A4C60 | SDL2/SDL3 window + event loop; forward activation, `WM_CHAR` text (UI text input) and quit |
| Renderer | `vidDRIVER`/`d3dDRIVER` (34 slots), `rendDRIVER`/`d3dREND_DRIVER` (46 slots), `txmD3D_MANAGER`/`txmD3D_TEX`; created by `CreateRendDriver`/`CreateTexManager` | OpenGL 2.1/3.3 core, GLES 2/3 (Android), or Vulkan/Metal via a wrapper. Must emulate fixed-function fog, W-buffer, stencil shadows, texture-stage blending, user clip planes; DXT textures need S3TC or a CPU decoder on GLES |
| Input | `_inpINPUT` vtable 0x0060A568 (29 slots); 340 float channels indexed by DirectInput scan code | SDL input with an SDL_Scancode -> DIK table; relative mouse mode; SDL_GameController can fill the unused joystick channels (gamepad support for free); touch -> synthesized mouse/keys |
| Sound | `_sndSYSTEM` vtable 0x00606788 (22 slots), `_sndBUFFER` vtable 0x006068D0, `_sndSTR_BUFFER` | OpenAL Soft (near 1:1: listener, sources, gain = 10^(dB/20), rolloff, doppler) or SDL_audio + a mixer |
| Music | `SetMusic` 0x004B5AA0 (DirectShow graph) | dr_mp3 / stb_vorbis / libsndfile streaming into an AL source |
| Movies | `wrCUTSCENE::Play` 0x004A5910 | pl_mpeg (single header MPEG-1) or FFmpeg; or skip them |
| Network | `netDRV_DPUBI` (`re/mpsrc/network_dpubi*.cpp`), netDRV vtable 0x0060A5E0 (42 slots) | ENet / raw UDP / SDL_net implementing the same netDRV interface (server start, browse, join, send, events). Drop the Ubi.com path (service offline). Wire format is defined in `re/mpsrc` |
| Files | `fioFILE_DISK` (fopen), `fnmFindDisc` 0x004FF0B0 (`_findfirst`), `fnmPathSep` | POSIX `fopen`, `opendir` + case-insensitive match, `/` paths; Android: AAssetManager or extract to storage |
| Config | `apCfg*` 0x004AF670.. (ini), registry reads in `wrAPP_SYSTEM::InitSubsys` | a small ini library; registry -> ini keys |
| Time | `_gsMsgCalcElapsedTime` 0x005109A0 | `SDL_GetPerformanceCounter`, `SDL_GetTicks`, `SDL_Delay` |
| Sync objects | `CreateMutexA`/`WaitForSingleObject`/`ReleaseMutex` (network driver only) | `std::mutex` / SDL mutex |
| BMP / images | `pctReadBMP`, `pctReadBMPHdr` (GDI) | stb_image or a 30-line BMP reader |
| Dialogs, shell | `MessageBoxA`, `ShellExecuteA`, `GetOpenFileNameA` (dev only) | `SDL_ShowSimpleMessageBox`, `SDL_OpenURL`; drop the dev dialog |
| Crash handler | `SetUnhandledExceptionFilter` (`apGPF_FILTER`) | signal handler / platform crash reporter |

`core/compat/wr_win32_compat.h` lists, in one place, every Win32 and DirectPlay 8 symbol the network layer uses; it
is what the multiplayer sources compile against on non-Windows hosts, and it doubles as the checklist for the network
replacement.

## 3. Three realistic ways to run Will Rock elsewhere

### A. Keep the original x86 binary (days, works today)

Run `WillRock.exe` 1.2 unchanged under **Wine** (Linux/macOS) or **Box86/Box64 + Wine** (ARM Linux, Android via
Winlator-style launchers). D3D7 goes through WineD3D or DXVK's D3D7-capable paths; DirectInput, DirectSound and
DirectPlay are provided by Wine. Mods keep working (WRML is a `dinput.dll` proxy; use a DLL override so Wine loads the
native proxy). Costs: x86 emulation on ARM, Wine's D3D7 quality, no native touch controls.
The SDK helps here only for mods and fixes (e.g. gamepad support by replacing the two joystick slots of `_inpINPUT`).

### B. Reimplement the platform layer under the original game code (weeks to a few months)

Keep the game's own x86 code, but replace every OS-facing class in §2 with new ones written against SDL2 + OpenGL
+ OpenAL + ENet, injected at the factory points (`CreateRendDriver`, `CreateTexManager`, the input/sound constructors,
the netDRV factory in `gsMP_NETWORK::Drv_Create` 0x0052EF10). The vtables are documented slot by slot, so the
replacements can be written as normal C++ classes with the same virtual order.

Two sub-variants:
- **B1, in-process on Windows/Wine**: a mod DLL swaps the drivers (good for a modern renderer, widescreen, gamepad,
  a working LAN/Internet netcode without DirectPlay).
- **B2, a static recompiler / loader** that maps the exe's x86 code and supplies the platform layer natively (the
  approach of projects that "lift" old PC games). Still x86-only unless combined with Box86/Box64 or a binary
  translator; the platform layer is then native code.

### C. Full source port (large: many months)

Rewrite the game logic as C++ source from the beta decompile (`db/mergedfiles.cpp`), using the SDK layouts and the
1.2 differences, the way `re/mpsrc` was rebuilt for multiplayer. Function counts from the subsystem indexes give the
size of the job: AI 1,239 beta functions, renderer 1,013, engine core 677, frontend 595, effects 584, game systems
466, m3d 335, weapons 288, plus player, shared game objects and platform drivers: roughly 5,500 functions, of which
the multiplayer layer (~8,600 lines) is done. Only this option gives native ARM/64-bit builds, but it must also
reproduce the details in §4 or saves, demos and multiplayer with original clients break.

Recommended path: **A** now for players, **B1** for the improvements people want (renderer, input, netcode), and
grow **C** subsystem by subsystem with the SDK as the specification, testing each rewritten piece against the
original binary through hooks.

## 4. Things a port must reproduce exactly

- **32-bit, packed layouts.** Entities and messages use `#pragma pack(1)` with odd offsets; save files and network
  packets are raw copies of those structs (`msgSYSTEM::Pack`, the MP sync record). A 64-bit port needs explicit
  serialisation.
- **Text** is UTF-16 (`wrWCHAR`; `wchar_t` is 4 bytes on Linux: use `char16_t`).
- **Floating point.** The game uses x87 with SSE/3DNow! paths in `m3dSqrt` and matrix code (chosen by `m3dSimdType`
  0x00637C24; 2 forces x87). Demos and lockstep-sensitive code drift slightly under pure SSE.
- **qsort order.** The message queue is sorted with MSVC's unstable `qsort` each frame; a different sort order changes
  message order for equal priorities.
- **Case-insensitive file names** and `\` separators in data (`..\movies\...`).
- **Fixed-function look**: fog, stencil shadows and the CPU "pte" lighting (environment maps, spot lights, caustics)
  are part of the art.

## 5. Open questions (consolidated)

Integrator (found while merging):
- `netDRV +0x08/+0x0C/+0x10`: one reconstruction saw an `arrVector`, two others see byte counters (sent / received).
  The counter view is used by two independent functions (`gsMP_NETWORK::UpdStat` 0x0052EE50 and the DirectPlay
  send/receive paths) and is likely right.
- `netDRV_SERVER_CREATE +0x00/+0x08/+0x0C`: `name`/`dataBegin`/`dataEnd` (network.cpp) vs `info`/reserved
  (gs_mp_network.cpp).
- `Dp_Server_OnConnectionAttempt`: the two reconstructions disagree on the order of the reply buffer and the
  player-context arguments.
- `animINST +0x134`: documented as the property sheet pointer, but the MP code copies it into create/sync messages as
  an "animSeed". One of the two meanings is wrong.
- Messages 0x3F6 and 0x3F7 are both sent by `__gsLoadLevel` (v12 0x00536AE2 and 0x00536B3D). Both reconstructions
  had called their one "LEVEL_LOADED"; the mpsrc macro for 0x3F7 is now `GS_MSG_LEVEL_LOADED_2`. Real names unknown.
- `cdtINFO_SPHERE` (vtable 0x005FA0D4) and the argument roles of the `IsVisiblePoint`/`ConstrSphere` calls in
  `gsMP_SYSTEM::SelectStartPosDir`.
- v12 addresses of `gsDemo`, `_scnCalcRadRenderOff`, `m3dUpdateTimeField`, most `wrSTAT_SYSTEM` methods, `camCAMERA`.
- game_systems said "Direct3D 8 / DirectInput 8"; the import table shows DirectDraw 7/Direct3D 7 and DirectInput 7
  (corrected in `game_systems/input.h`).

From the subsystems (details in each README):
- engine_core: size of the address list in `msgSYSTEM` (+0x1C..+0x350); meaning of the 1.2 fields entity +0xE4,
  +0x155, slot 23, `entIACTIVE_OBJ` +0x90/+0x94; class-registry priority direction and capacity.
- engine_render: pteEMAP / pteSPOT_ANIM v12 vtables; rendDRIVER light block (+0x3C..+0x74); interaction of the
  third-party widescreen patch with `BeginCamera` / field of view.
- math_app: unknown fields of `wrAPP_SYSTEM`/`gsWIN_APP_SYSTEM`; two `gsAppState` bits named from beta logic only.
- platform_drv: `_sndBUFFER` slots +0x0C..+0x60; the sound-device list layout; dev config dialog reachability.
- game_systems: where the 4 extra bytes sit in the active-object base; the new cinema field; save extended header;
  game command ids from 0x19 up.
- game_shared: `dmgWEAPON_PACK` and `plgsACTION` v12 vtables; names of damage types 3 and 4.
- player: exact `plrPLAYER_COMMON`/`plrPLAYER` boundary (~0x28B); new vtable slots 36, 44, 53, 54, 74.
- weapons: the Medusa launch float/int; crossbow and atomic gun `Create`/sizes in v12; new slots +0x8C/+0xCC.
- ai: several constant changes marked "Lead"; meaning of the new NPC fields and slots; weak match for
  `aiNPC::IsPlrVisible` (0x005593E0).
- sfx_sob: 88 unmatched functions; particle record layout; five classes whose 1.2 code is not explained by the +0x1C rule.
- frontend: input/child-notify event layouts beyond what MP uses; new UI slot +0x20 and HUD slot +0x60 (probably chat);
  messages 0x3F2, 0x4CE, 0x4CF.
