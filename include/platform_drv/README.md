# platform_drv: the Win32/DirectX platform layer of Will Rock 1.2 (porting map)

This covers the `drv:*` objects except the network driver (that one is in `re/mpsrc`), plus the Win32 shell around them
(`gs_windows`, `ap_*`, cutscenes). It is the file to start from for an OS or device port.

Files:

| file | contents |
|---|---|
| `input.h` | `inpINPUT` / `_inpINPUT` (DirectInput 7): the 340-channel model, vtable, fields, DIK->VK table, `gsINP_SYSTEM` hooks |
| `sound_drv.h` | `_sndSYSTEM` (DirectSound 8 + DirectShow music), `_sndBUFFER`, `sndMUSIC`, `snmMANAGER` entry points |
| `app_win.h` | `gsWIN_APP_SYSTEM`, `_gsWndProc`, `gsAppState`, frame timer, `.ini` config (`apCfg*`), message boxes, crash filter, `wrCUTSCENE` (DirectShow video) |
| render | already in `re/sdk/engine_render/d3d_driver.h` + its README (vidDRIVER/d3dDRIVER/d3dREND_DRIVER, all vtables verified). Not repeated here. |
| `re/work/sdk_platform_drv/` | `imports_callers.json` (machine-readable import->caller map), `imp2.py` / `gen_table.py` (regenerate it), `drvfuncs.tsv` (every drv/input function feb->v12) |

Shared core types assumed from `re/sdk/core`: `m3dV`, `apNAME`, `msgADDR`, `fioFILE`.

## Which DirectX, really

| part | interface | evidence |
|---|---|---|
| render | **DirectDraw 7 + Direct3D 7** (`IDirectDraw7`, `IDirect3DDevice7`), not D3D8 | imports `DirectDrawCreateEx`, `DirectDrawEnumerateA`; `d3dEnumDevices` loads `DirectDrawEnumerateExA` by `GetProcAddress` |
| sound | **DirectSound 8** in 1.2 (feb: DirectSound 1) | v12 imports DSOUND ordinal 11 `DirectSoundCreate8`, feb ordinal 1 `DirectSoundCreate`; buffers QI `IID_IDirectSoundBuffer8` |
| music + movies | **DirectShow** (`CLSID_FilterGraph`, `IGraphBuilder`, `IMediaControl`, `IMediaSeeking`, `IBasicAudio`, `IVideoWindow`, `IMediaEventEx`) | `_sndSYSTEM::InitMusic` v12 0x004B5910, `wrCUTSCENE::Play` v12 0x004A5910 |
| input | **DirectInput 7** (`DirectInputCreateW(…,0x700)`, fallback 0x300), `IDirectInputDevice7W` | `_inpINPUT::Init` v12 0x0058E7A0 |
| version check | `_gsDetectDx9` (DxDiag `dwDirectXVersionMajor`) only warns | v12 0x004A4640 |
| video codec | no Bink. Two MPEG-1 files `..\movies\ubilogo.mpg`, `..\movies\saber3d.mpg` | strings v12 0x00632620/0x00632644 |

## How it works

### Lifecycle (one process, one window)
1. CRT `WinMainCRTStartup` -> game `WinMain` builds `wrAPP_SYSTEM` (a `gsWIN_APP_SYSTEM`).
2. `gsWIN_APP_SYSTEM::Init` (v12 0x004A47E0): `InitCommonControls(Ex)`, `RegisterClassExA` (class `SABER3D_WIN_CLASS_NAME`, proc `_gsWndProc` v12 0x004A4E00),
   `CreateWindowExA(0, class, title, 0x880000, 0,0, 640,480)`; system menu stripped. `IsAlreadyRunning` uses `FindWindowA`.
3. `InitSubsys` (wrAPP_SYSTEM) creates the systems. Platform pieces: `vidInit` (v12 0x004B3AF0: `d3dEnumDevices`, driver list),
   `gsSND_SYSTEM::Init` (`sndSYSTEM::make`, `Init`, `SetFormat`, `snmMANAGER`), `gsINP_SYSTEM::Init` (v12 0x0052BC00: `inpINPUT::Make`,
   `Init`, `SetInputWnd(apHMainWindow)`), COM `CoInitialize` (gsAPP_SYSTEM::InitAll), `apCfgInit` for the `.ini`.
4. `MainLoop` (v12 0x004A4C60), each iteration:
   `PeekMessage/TranslateMessage/DispatchMessage` until empty -> `ProcessActDeact` (slot +0x28) -> `WaitMessage` if inactive ->
   if a cutscene runs: `gspCutscene` slot +0x10, else `gsMsgCallFrame` (v12 0x00510B90, the whole game frame: timer
   `_gsMsgCalcElapsedTime` v12 0x005109A0 with `QueryPerformanceCounter`, input `gsINP_SYSTEM::ProcessInput` -> `inpINPUT::GetInput`,
   game update, render via `vidDriver`/`d3dREND_DRIVER`, sound `SetUpListener`/`CommitSettings`/`UpdateMusic`) -> `gsINP_SYSTEM::Acquire`.
5. Quit: `gsAppState |= 8`, `Term` (DestroyWindow, UnregisterClass), CoUninitialize.

### Input per frame
`inpINPUT::GetInput` (v12 0x004B67F0): `prev = cur`, zero the axis channels, call `GetKeyboardInput` (slot +0x48, buffered
`GetDeviceData`, 30 events), `GetMouseInput` (+0x5C, `GetDeviceState(DIMOUSESTATE)`; delta = l{X,Y,Z} * sensitivity * 0.025659),
`GetJoystickInput` (+0x68), subtract `held`, split each axis into +/- channels, apply `remap`. `gsiCOMMAND::GetInput` then turns bound
key sequences (from the input `.ini` layouts of the `gsiCONTEXT`s) into game commands.
**Joystick/gamepad is dead code in both builds**: `_inpINPUT::InitJoystick` (slot +0x60) is the shared `xor eax,eax; ret` stub
(v12 0x00502570), so `pJoystick` (+0x19DC) stays NULL and force feedback can never initialise. A port that wants gamepads only has
to fill channels 260..291 (buttons) and 306.. (axes): the binding layer already understands them.

### Sound
`_sndSYSTEM::Init` (v12 0x004B5120) enumerates devices (`DirectSoundEnumerateA` -> `_sndDSEnumProc` v12 0x004B5790), creates the device
(`DirectSoundCreate8`), `SetCooperativeLevel(apHMainWindow, DSSCL_PRIORITY)`, creates a looping 3D primary buffer, sets the format and
gets the `IDirectSound3DListener` (rolloff 1.0, the game then sets 0.5). Samples are RIFF/WAVE files, memory-mapped
(`CreateFileA`/`CreateFileMappingA`/`MapViewOfFile` in `_sndBUFFER::InitFile` v12 0x004B9450) and parsed by `sndParseWaveResource`
(v12 0x004BAB40). `snmMANAGER` caches samples by name with up to 10 duplicate buffers (`IDirectSound::DuplicateSoundBuffer`).
Game code talks only to `sncCHANNEL` (Snd_chn.obj, portable) which drives `_sndBUFFER` virtuals (3D position/velocity, min/max distance,
volume in dB). Music: `SetMusic` (v12 0x004B5AA0) finds the file with `fnmFindDisc`, converts the name with `MultiByteToWideChar`,
`IGraphBuilder::RenderFile`, `IMediaControl::Run`; `UpdateMusic` loops via `IMediaSeeking`; volume via `IBasicAudio::put_Volume`.

### Video modes
Covered in `re/sdk/engine_render/README.md`: `vidGetDriverList` (v12 0x004B3B60; desktop bpp via `GetDC`/`GetDeviceCaps`),
`d3dEnumDevices` v12 0x004B7F30, modes in `vidDRIVER` (`AddMode`/`FindMode` v12 0x004B3DD0/0x004B3ED0), `SetMode` v12 0x004B3F50,
`d3dDRIVER::InitHW` v12 0x004BAF90. Windowed vs fullscreen window styling: `gsWIN_APP_SYSTEM::ResizeWin` v12 0x004A49D0.

### Data files read by this layer
- `.ini` files through `GetPrivateProfileString`/`WritePrivateProfileString` (`apCfg*`; path = current directory + name); Ubi.com
  client also reads its own ini (`netUBI_*`).
- registry `HKLM` (install path / scale detection): `gsCfgScaleDetect` v12 0x00531CE0, `wrAPP_SYSTEM::InitSubsys` (feb 0x0049C400).
- `*.wav` (RIFF PCM) samples, music files through DirectShow (any format DirectShow can render), `..\movies\*.mpg`.
- `*.bmp` via GDI (`pctReadBMP` v12 0x00504FA0) in addition to the engine's own texture formats.

## 1.2 vs feb differences seen
- Imports: feb `DSOUND!1 DirectSoundCreate` -> v12 `DSOUND!11 DirectSoundCreate8`; v12 adds `USER32!SetWindowLongA` (used twice in
  `gsWIN_APP_SYSTEM::ResizeWin`, v12 0x004A4A52/0x004A4AD9: real windowed/fullscreen style switch); feb's `WS2_32!gethostname` is gone.
- `_sndSYSTEM` object 0x1676 -> 0x16A2 bytes, vtable 24 -> 22 slots, sound fields moved (+0x30 for the DS part, +0x2C for music, -4 for the
  head); `_sndBUFFER` rewritten for DirectSound 8 (float dB volume, new apply slot). See `sound_drv.h` and `re/results/sound.json`.
- Input driver: identical layout and vtable order (object 0x2FEC both); only addresses moved, and the default mouse sensitivity in the
  `_inpINPUT` ctor changed 2.05 -> 2.005 (0x400051EC) for mouse (0x0058E705) and joystick (0x0058E75D).
- `gsWIN_APP_SYSTEM`: same vtable (12 slots) and fields.
- The Win32 developer config dialogs (`gsCFG_SYSTEM::Init/Show/Term/ShowMsg`, feb 0x00524880..0x00524900) have no located twin in v12;
  their dialog procedures (`_gsCfg*_WND`) still have code in v12 0x00532400..0x0053DC00 (GetDlgItem etc. calls), so they may just be unreachable. // ?
- v12 also contains a third-party widescreen patch (code caves 0x005F80ED..0x005F8270 and section .text1 0x009E8000; see AGENT_BRIEF).

## TWEAK table
| constant / limit | value | v12 address | notes |
|---|---|---|---|
| input object size | 0x2FEC | 0x0058E691 (imm of `push`) | grow only together with new fields |
| number of input channels | 0x154 (340) | loops in `inpINPUT` ctor 0x004B6190 / `GetInput` 0x004B67F0 / `SyncAcquire` 0x0058E8C0 | fixed array size: not a simple patch |
| default mouse sensitivity | v12 2.005f (0x400051EC; feb 2.05f) | imm32 at 0x0058E70B (`mov [esi+0x1990],0x400051EC` at 0x0058E705) | overwritten by `ApplySettings` from config |
| mouse sensitivity clamp | [0.01, 4.0] | `SetMouseSensitivity` v12 0x00594510: floats at 0x005FAA38 (0.01) and 0x005F9870 (4.0) | constants are shared by other code: patch the `mov [ecx+0x1990],imm` at 0x00594521/0x00594541 instead |
| keyboard/mouse DirectInput buffer | 30 events | `InitKeyboard` 0x00594C10 / `InitMouse` 0x00594482 (`mov [esp+0x18],0x1E`) | |
| DirectInput version | 0x700, retry 0x300 | 0x0058E7B2 / 0x0058E7CB | |
| sound cooperative level | 2 (DSSCL_PRIORITY) | `_sndSYSTEM::Init` 0x004B5120 (call `[edx+0x18]` at 0x004B518B) | |
| primary buffer caps | 0x91 | `_sndSYSTEM::Init`: `mov [esp+0x14],0x91` at 0x004B5240 | |
| buffer caps OR | 0x00040000 -> 0x00040020 adds CTRLFREQUENCY (pitch) | 0x004B96B0 (imm32) | from results/sound.json |
| startup window size | 640x480 | `gsWIN_APP_SYSTEM::Init` 0x004A48CB (`push 0x1E0`), 0x004A48D0 (`push 0x280`) | |
| window style | 0x880000 | `gsWIN_APP_SYSTEM::Init` | |
| cutscene video rect | 640x480 centred | `PlayMedia` 0x004A5670 | |

## HOOK points
| hook | v12 address | use |
|---|---|---|
| `_gsWndProc` | 0x004A4E00 (stdcall, 4 args) | extra window messages, overlays, IME, raw input |
| `gsWIN_APP_SYSTEM::MainLoop` / vtable slot +0x14 of 0x00605CC8 | 0x004A4C60 | replace the pump (SDL_PollEvent) |
| `gsMsgCallFrame` | 0x00510B90 | per-frame callback for mods |
| `_gsMsgCalcElapsedTime` | 0x005109A0 | frame limiter / timescale |
| `inpINPUT::GetInput` | 0x004B67F0 | inject input: write `cur[]` channels after the original (gamepad support, macros) |
| `_inpINPUT` vtable 0x0060A568 slot +0x60 (`InitJoystick`) / +0x68 (`GetJoystickInput`) | 0x00502570 / 0x00594270 | enable gamepads (XInput/SDL) by replacing these two slots and setting flag 4 |
| `_sndSYSTEM` vtable 0x00606788 | slots above | swap the whole audio backend (OpenAL) without touching game code |
| `_sndSYSTEM::SetMusic` (slot +0x40) | 0x004B5AA0 | custom music (ogg) player |
| `wrCUTSCENE::Play` | 0x004A5910 | skip or replace intro movies |
| `apLog` / `apLogErr` | 0x004AF480 / 0x004AF4D0 | capture the engine log |
| `_apCfgRead*` | 0x004AF7E0.. | override config values / redirect the ini to a user folder |
| `apGPF_FILTER::SetupFilter` | 0x004B0630 | install your own crash handler |
| `DirectInputCreateW` thunk 0x005AA5B0, `DirectDrawCreateEx` thunk 0x0058E672, `DirectSoundCreate8` thunk 0x0058E678 | | wrapper-DLL style interception (dinput.dll proxy = WRML) |

## PORTING notes
A port has to replace exactly these layers; everything else (entities, AI, physics, particles, `sncCHANNEL`, `pte` shaders, UI) is
plain 32-bit C++ on top of them:

1. **Window + loop** (`gsWIN_APP_SYSTEM`, `_gsWndProc`): SDL2 window and event loop. Translate SDL events to what `_gsWndProc` produced:
   activation (`ProcessActDeact`), `WM_CHAR` text for `uiTEXT_INPUT`, quit. Single-instance check optional.
2. **Render** (`vidDRIVER`/`d3dDRIVER`/`d3dREND_DRIVER`, `txmD3D_MANAGER`): see `re/sdk/engine_render/README.md`; new GL/Vulkan/GLES
   classes implementing the same vtables. Fixed-function D3D7 state (fog, stencil shadows, w-buffer, texture stage blend) must be emulated.
   On Android/GLES: no W-buffer, DXT textures need a decoder or ETC/ASTC re-encode.
3. **Input** (`_inpINPUT`): implement the 29 virtuals over SDL: keyboard channels are indexed by DirectInput scan code (DIK), so a
   port needs an SDL_Scancode -> DIK table; mouse relative motion -> `mouseDelta` (SDL relative mouse mode); `GetKeyName` -> SDL_GetKeyName;
   gamepad -> channels 260.. and 306.. (SDL_GameController), force feedback -> SDL_Haptic / SDL_GameControllerRumble. Touch devices:
   synthesize mouse deltas and key channels.
4. **Sound** (`_sndSYSTEM`, `_sndBUFFER`, `_sndSTR_BUFFER`): OpenAL Soft maps almost 1:1 (listener = `SetUpListener`, source per buffer,
   `Duplicate` = new source sharing the AL buffer, volume dB -> gain = 10^(dB/20), rolloff = `SetAttenuationPow`, doppler). Music
   (DirectShow) -> a decoder (dr_mp3/stb_vorbis/libsndfile) streaming into an AL source.
5. **Movies** (`wrCUTSCENE`): pl_mpeg (single header MPEG-1) or FFmpeg rendering to a texture; or skip.
6. **Files**: the engine already uses CRT `fopen`/`_findfirst`; on POSIX replace `_findfirst/_findnext` with `opendir`, make lookups
   case-insensitive (the game data uses mixed case and backslashes `..\movies\`), and memory-map with `mmap`.
7. **Config**: replace `GetPrivateProfileString`/`WritePrivateProfileString` with an INI library; registry reads -> config file.
8. **Timing**: `QueryPerformanceCounter` -> `SDL_GetPerformanceCounter`; `timeGetTime` -> `SDL_GetTicks`; `Sleep` -> `SDL_Delay`.
9. **Threads**: only the network driver uses OS sync objects (`CreateMutexA`, `WaitForSingleObject`, `ReleaseMutex`) -> SDL mutexes.
10. **Network**: Winsock calls are in the clSocket/Ubi.com libs (`re/mpsrc`), BSD sockets are nearly identical.
11. **ABI**: everything is 32-bit MSVC (`__thiscall`, `__fastcall` for many free functions, MSVC RTTI/EH, STLport). A binary-level port
    (e.g. box86/box64 + Wine on ARM Linux/Android) is the cheapest route today; a source-level port needs the reconstructed code.

## Full import table of v12.exe (202 functions, 14 DLLs)
Generated by `re/work/sdk_platform_drv/imp2.py` + `gen_table.py`: callers are found in the clean feb build (IAT-referencing
`call [iat]`, `jmp [iat]`, `mov reg,[iat]` instructions) and mapped to v12 through `tables/v12.functions.tsv`, then checked to
have a reference to the same import in v12 within the function (v12 has junk filler bytes that copy real code, so a raw byte search
of v12 alone gives false callers). "v12 call ~0x..." = the twin function is not in the tables; the address is the nearest matching
v12 call site. DirectX entry points are called through import thunks: the thunk callers were taken from `wr.py xref`.
Counts: KERNEL32 95 (66 used only by the C runtime), USER32 56, WS2_32 25, GDI32 8, ADVAPI32 3, OLE32 3, COMCTL32 2, DDRAW 2, DSOUND 2,
OLEAUT32 2, COMDLG32 1, DINPUT 1, SHELL32 1, WINMM 1.

| DLL | import | what it does here | v12 callers (function v12 addr) | port replacement |
|---|---|---|---|---|
| ADVAPI32.DLL | RegCloseKey | same | wrAPP_SYSTEM::InitSubsys (feb 0x0049C400; v12 call ~0x004AAC35); gsCfgScaleDetect 0x00531CE0; netUBI_DOWNLOADER::StartDownload (feb 0x0057B060; v12 call ~0x00595AEE) | same |
| ADVAPI32.DLL | RegOpenKeyExA | read install path / CPU scale values from HKLM | wrAPP_SYSTEM::InitSubsys (feb 0x0049C400; v12 call ~0x004AABD4); gsCfgScaleDetect 0x00531CE0; netUBI_DOWNLOADER::StartDownload (feb 0x0057B060; v12 call ~0x00595A98) | config file or SDL_GetBasePath |
| ADVAPI32.DLL | RegQueryValueExA | same | wrAPP_SYSTEM::InitSubsys (feb 0x0049C400; v12 call ~0x004AABE7); gsCfgScaleDetect 0x00531CE0; netUBI_DOWNLOADER::StartDownload (feb 0x0057B060; v12 call ~0x00595ACE) | same |
| COMCTL32.DLL | InitCommonControlsEx | common controls for the config dialogs | gsWIN_APP_SYSTEM::Init 0x004A47E0 | drop (no Win32 dialogs) |
| COMCTL32.DLL | ord17 | InitCommonControls | gsWIN_APP_SYSTEM::Init 0x004A47E0 | drop |
| COMDLG32.DLL | GetOpenFileNameA | file-open dialog (dev tool wifOpenFile) | thunk v12 0x0058E666; wifOpenFile (feb 0x004A1490, v12 near 0x004B0076) - dev/editor file picker | drop or SDL_ShowOpenFileDialog / tinyfiledialogs |
| DDRAW.DLL | DirectDrawCreateEx | create IDirectDraw7 | thunk v12 0x0058E672; called by d3dDRIVER::InitDrv 0x004BAE10 (+0x2A) and d3dEnumDDDeviceCB 0x004B8510 (call at 0x004B8570) | SDL_CreateWindow + GL/Vulkan context (see engine_render README) |
| DDRAW.DLL | DirectDrawEnumerateA | enumerate display adapters | thunk v12 0x0058E66C; d3dEnumDevices 0x004B7F30 (fallback when GetProcAddress("DirectDrawEnumerateExA") fails) | SDL_GetNumVideoDisplays |
| DINPUT.DLL | DirectInputCreateW | create IDirectInput7 (keyboard, mouse, joystick, force feedback) | thunk v12 0x005AA5B0; _inpINPUT::Init 0x0058E7A0 (version 0x700, retry 0x300) | SDL events / SDL_GetKeyboardState / SDL_GetRelativeMouseState / SDL_GameController + SDL_Haptic |
| DSOUND.DLL | ord11 | DirectSoundCreate8: IDirectSound8 device | DirectSoundCreate8 (feb imported ord 1 DirectSoundCreate). thunk v12 0x0058E678; _sndSYSTEM::Init 0x004B5120 (calls at 0x004B5170, 0x004B51D5) | OpenAL alcOpenDevice/alcCreateContext (or SDL_audio + mixer, miniaudio) |
| DSOUND.DLL | ord2 | DirectSoundEnumerateA: list sound devices | DirectSoundEnumerateA. thunk v12 0x0058E67E; _sndSYSTEM::Init 0x004B5120 (+0x0E, callback 0x004B5790 = _sndDSEnumProc) | alcGetString(ALC_ALL_DEVICES_SPECIFIER) / SDL_GetAudioDeviceName |
| GDI32.DLL | CreateCompatibleDC | BMP decode via GDI | pctReadBMP 0x00504FA0 | stb_image / SDL_LoadBMP |
| GDI32.DLL | DeleteDC | BMP decode cleanup | pctReadBMP 0x00504FA0 | stb_image |
| GDI32.DLL | DeleteObject | BMP decode cleanup | pctReadBMPHdr 0x00504F20 | stb_image |
| GDI32.DLL | GetDeviceCaps | desktop BITSPIXEL | vidGetDriverList 0x004B3B60 | SDL_GetDesktopDisplayMode |
| GDI32.DLL | GetObjectA | BMP header via GDI | pctReadBMPHdr 0x00504F20; pctReadBMP 0x00504FA0 | stb_image |
| GDI32.DLL | GetPixel | BMP decode pixel-by-pixel | pctReadBMP 0x00504FA0 | stb_image |
| GDI32.DLL | GetStockObject | window background brush | gsWIN_APP_SYSTEM::Init 0x004A47E0 | drop |
| GDI32.DLL | SelectObject | BMP decode via GDI | pctReadBMP 0x00504FA0 | stb_image |
| KERNEL32.DLL | CloseHandle | close file/mutex handles (sound, network driver) | ?_EnetDRV (feb 0x004A6D10; v12 twin not located); ?1netDRV (feb 0x004A6DF0; v12 twin not located); _sndBUFFER::Init 0x004B9450; _sndSTR_BUFFER::Init (feb 0x004AA5C0; v12 call ~0x004BA11D); __close_lk 0x00586FEF; __sopen 0x0058C4A3; ?1netDRV_DPUBI (feb 0x00576AE0; v12 call ~0x0058F4E7); ?1CMsgQueue 0x005D4FE0 | close / SDL_DestroyMutex |
| KERNEL32.DLL | CompareStringA | MSVC C runtime (LIBCMT) internals | ___crtCompareStringA 0x0058E0A1 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | CompareStringW | MSVC C runtime (LIBCMT) internals | ___crtCompareStringA 0x0058E0A1 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | CreateDirectoryA | MSVC C runtime (LIBCMT) internals | _mkdir 0x005A16E0 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | CreateFileA | open .wav for memory mapping (_sndBUFFER::Init(char*)) | _sndBUFFER::Init 0x004B9450; _sndSTR_BUFFER::Init (feb 0x004AA5C0; v12 call ~0x004BA0E5); __sopen 0x0058C4A3 | fopen/mmap or SDL_RWFromFile |
| KERNEL32.DLL | CreateFileMappingA | map .wav file | _sndBUFFER::Init 0x004B9450; _sndSTR_BUFFER::Init (feb 0x004AA5C0; v12 call ~0x004BA103) | mmap or read into memory |
| KERNEL32.DLL | CreateMutexA | netDRV thread lock (network driver, see mpsrc) | ?0netDRV (feb 0x004A6C50; v12 twin not located); ?0netDRV_DPUBI (feb 0x005769D0; v12 call ~0x0058F329); ?0CMsgQueue 0x005D4F60 | SDL_CreateMutex / pthread_mutex |
| KERNEL32.DLL | DeleteCriticalSection | MSVC C runtime (LIBCMT) internals | __fcloseall (feb 0x00573382; v12 call ~0x0058BCD1) | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | DeleteFileA | MSVC C runtime (LIBCMT) internals | _remove 0x00582506 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | EnterCriticalSection | MSVC C runtime (LIBCMT) internals | __lock 0x005861E3; __lock_file 0x00586315; __lock_file2 0x00586344; __getstream 0x00587490; __alloc_osfhnd 0x0058C1C2; __lock_fhandle 0x0058C422 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | ExitProcess | MSVC C runtime (LIBCMT) internals | _doexit 0x00580FCC; _fast_error_exit (feb 0x005688D5; v12 call ~0x00581225) | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | FileTimeToLocalFileTime | MSVC C runtime (LIBCMT) internals | __stat 0x00581ED2; ___timet_from_ft 0x005828E4 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | FileTimeToSystemTime | MSVC C runtime (LIBCMT) internals | __stat 0x00581ED2; ___timet_from_ft 0x005828E4 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | FindClose | MSVC C runtime (LIBCMT) internals | __stat 0x00581ED2; __findclose 0x005828C4 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | FindFirstFileA | MSVC C runtime (LIBCMT) internals | __stat 0x00581ED2; __findfirst 0x0058272F | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | FindNextFileA | MSVC C runtime (LIBCMT) internals | __findnext 0x005827FC | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | FlushFileBuffers | MSVC C runtime (LIBCMT) internals | __commit 0x0058709D | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | FreeEnvironmentStringsA | MSVC C runtime (LIBCMT) internals | ___crtGetEnvironmentStringsA 0x00586946 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | FreeEnvironmentStringsW | MSVC C runtime (LIBCMT) internals | ___crtGetEnvironmentStringsA 0x00586946 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetACP | MSVC C runtime (LIBCMT) internals | _getSystemCP 0x0058BEF2 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetCPInfo | MSVC C runtime (LIBCMT) internals | __setmbcp 0x0058BD45; _setSBUpLow 0x0058BF98; ___crtCompareStringA 0x0058E0A1 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetCommandLineA | MSVC C runtime (LIBCMT) internals | _WinMainCRTStartup (feb 0x005687A8; v12 call ~0x00581161) | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetComputerNameA | crash report header (apGPF_FILTER) | apGPF_FILTER::GPFExeptFunc (feb 0x004A1AA0; v12 call ~0x004B07CC) | gethostname |
| KERNEL32.DLL | GetCurrentDirectoryA | ini path base (apCfgInit); CRT | apCfgInit 0x004AF670; __getdrive 0x00589262; __getdcwd_lk 0x0058D077 | getcwd / SDL_GetBasePath |
| KERNEL32.DLL | GetCurrentProcess | MSVC C runtime (LIBCMT) internals | _doexit 0x00580FCC | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetCurrentThreadId | MSVC C runtime (LIBCMT) internals | __mtinit 0x00584EF1; __getptd 0x00584F58 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetDriveTypeA | MSVC C runtime (LIBCMT) internals | __stat 0x00581ED2; __validdrive 0x0058D14C | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetEnvironmentStrings | MSVC C runtime (LIBCMT) internals | ___crtGetEnvironmentStringsA 0x00586946 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetEnvironmentStringsW | MSVC C runtime (LIBCMT) internals | ___crtGetEnvironmentStringsA 0x00586946 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetEnvironmentVariableA | MSVC C runtime (LIBCMT) internals | ___heap_select 0x00586C61 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetFileAttributesA | MSVC C runtime (LIBCMT) internals | __access 0x00581B31 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetFileType | MSVC C runtime (LIBCMT) internals | __ioinit 0x00586A78; __sopen 0x0058C4A3 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetFullPathNameA | MSVC C runtime (LIBCMT) internals | __fullpath 0x005891BD; __getdcwd_lk 0x0058D077 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetLastError | MSVC C runtime (LIBCMT) internals | __access 0x00581B31; __stat 0x00581ED2; _remove 0x00582506; __findfirst 0x0058272F; __findnext 0x005827FC; __getptd 0x00584F58; __close_lk 0x00586FEF; __commit 0x0058709D; __write_lk 0x00587195; __fullpath 0x005891BD; __lseek_lk 0x0058941A; __read_lk 0x0058972F; __sopen 0x0058C4A3; __chsize_lk 0x0058D9F1; _mkdir 0x005A16E0 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetLocalTime | save-game time stamps (_gsGetCurTime); CRT time() | _gsGetCurTime 0x004A4580; _time 0x00580D92 | localtime_r |
| KERNEL32.DLL | GetModuleFileNameA | exe path (fnmInit base dir, crash log) | apGPF_FILTER::GPFExeptFunc (feb 0x004A1AA0; v12 call ~0x004B06A4); fnmInit 0x004FEC40; __setargv 0x005866F9; ___heap_select 0x00586C61; __NMSG_WRITE 0x00586E3F | SDL_GetBasePath |
| KERNEL32.DLL | GetModuleHandleA | GetModuleHandle("DDRAW.DLL") for DirectDrawEnumerateExA; CRT | d3dEnumDevices 0x004B7F30; _WinMainCRTStartup (feb 0x005687A8; v12 call ~0x005811AF); __ms_p5_mp_test_fdiv 0x00582E0C; __GetLinkerVersion 0x00586C34 | drop |
| KERNEL32.DLL | GetOEMCP | MSVC C runtime (LIBCMT) internals | _getSystemCP 0x0058BEF2 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetPrivateProfileIntA | read .ini int | netUBI_SERVERREG::OnNextRouterAddress (feb 0x0057B840; v12 call ~0x0059620F); netUBI_CLIENT::StartConnect (feb 0x0057CAB0; v12 call ~0x0059780F) | portable INI parser |
| KERNEL32.DLL | GetPrivateProfileStringA | read .ini (apCfg*, WillRock.ini/ubi.ini) | _apCfgReadInt 0x004AF7E0; _apCfgReadDword 0x004AF890; _apCfgReadBool 0x004AF940; _apCfgReadFloat 0x004AF9D0; _apCfgReadString 0x004AFA50; netUBI_SERVERREG::OnNextRouterAddress (feb 0x0057B840; v12 call ~0x00596248); netUBI_CLIENT::StartConnect (feb 0x0057CAB0; v12 call ~0x005977B0) | portable INI parser (inih) |
| KERNEL32.DLL | GetProcAddress | DirectDrawEnumerateExA lookup; CRT | d3dEnumDevices 0x004B7F30; __ms_p5_mp_test_fdiv 0x00582E0C; ___crtMessageBoxA 0x0058C139 | drop |
| KERNEL32.DLL | GetStartupInfoA | MSVC C runtime (LIBCMT) internals | _WinMainCRTStartup (feb 0x005687A8; v12 call ~0x0058118C); __ioinit 0x00586A78 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetStdHandle | MSVC C runtime (LIBCMT) internals | __ioinit 0x00586A78; __NMSG_WRITE 0x00586E3F | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetStringTypeA | MSVC C runtime (LIBCMT) internals | ___crtGetStringTypeA 0x0058B574 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetStringTypeW | CRT / UI text | ___crtGetStringTypeA 0x0058B574 | libc |
| KERNEL32.DLL | GetSystemTime | MSVC C runtime (LIBCMT) internals | _time 0x00580D92 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetTimeZoneInformation | MSVC C runtime (LIBCMT) internals | _time 0x00580D92; __tzset_lk 0x0058B6EB | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetVersion | MSVC C runtime (LIBCMT) internals | _WinMainCRTStartup (feb 0x005687A8; v12 call ~0x00581101) | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GetVersionExA | MSVC C runtime (LIBCMT) internals | ___heap_select 0x00586C61 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | GlobalMemoryStatus | RAM size for detail auto-scale (gsCfgScaleInit) and crash log | apGPF_FILTER::SetupFilter 0x004B0630; apGPF_FILTER::GPFExeptFunc (feb 0x004A1AA0; v12 call ~0x004B07EA); gsCfgScaleInit (feb 0x0051C200; v12 call ~0x00531B2E); gsCfgScaleDetect 0x00531CE0 | sysconf(_SC_PHYS_PAGES) / SDL_GetSystemRAM |
| KERNEL32.DLL | HeapAlloc | MSVC C runtime (LIBCMT) internals | __heap_alloc 0x005814E0; _calloc 0x005815DC; _realloc 0x00581802; ___sbh_heap_init 0x00587573; ___sbh_alloc_new_region 0x00587C18; ___old_sbh_new_region 0x005880BA | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | HeapCreate | MSVC C runtime (LIBCMT) internals | __heap_init 0x00586DA9 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | HeapDestroy | MSVC C runtime (LIBCMT) internals | __heap_init 0x00586DA9 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | HeapFree | MSVC C runtime (LIBCMT) internals | _free 0x00581719; ___sbh_free_block 0x005875E6; ___sbh_alloc_new_region 0x00587C18; ___old_sbh_new_region 0x005880BA; ___old_sbh_release_region 0x005881FE | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | HeapReAlloc | MSVC C runtime (LIBCMT) internals | _realloc 0x00581802; ___sbh_alloc_new_region 0x00587C18 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | HeapSize | MSVC C runtime (LIBCMT) internals | __msize 0x00583B6D | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | InitializeCriticalSection | MSVC C runtime (LIBCMT) internals | __mtinitlocks 0x005861BA; __lock 0x005861E3; __getstream 0x00587490; __alloc_osfhnd 0x0058C1C2; __lock_fhandle 0x0058C422 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | InterlockedDecrement | MSVC C runtime (LIBCMT) internals | _toupper 0x00588BAA; _tolower 0x00589B66; _wctomb 0x0058A28B; _mbtowc 0x0058A435 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | InterlockedExchange | STLport spin lock; CRT | _M_do_lock@?$_STLP_mutex_spin@$0A 0x005A8640; _M_acquire_lock@_STLP_mutex_base@_STL 0x005A8880; _M_allocate@?$__node_alloc@$00$0A 0x005A8D10 | std::atomic / SDL_AtomicSet |
| KERNEL32.DLL | InterlockedIncrement | MSVC C runtime (LIBCMT) internals | _toupper 0x00588BAA; _tolower 0x00589B66; _wctomb 0x0058A28B; _mbtowc 0x0058A435 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | IsBadCodePtr | Win32 base API used by CRT | _ValidateExecute 0x005BDC2F | host libc |
| KERNEL32.DLL | IsBadReadPtr | Win32 base API used by CRT | _ValidateRead 0x005BDBF7 | host libc |
| KERNEL32.DLL | IsBadWritePtr | Win32 base API used by CRT | _ValidateWrite 0x005BDC13 | host libc |
| KERNEL32.DLL | LCMapStringA | MSVC C runtime (LIBCMT) internals | ___crtLCMapStringA 0x0058C772 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | LCMapStringW | MSVC C runtime (LIBCMT) internals | ___crtLCMapStringA 0x0058C772 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | LeaveCriticalSection | MSVC C runtime (LIBCMT) internals | __unlock 0x00586244; __unlock_file 0x00586367; __unlock_file2 0x00586396; __alloc_osfhnd 0x0058C1C2; __unlock_fhandle 0x0058C481 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | LoadLibraryA | MSVC C runtime (LIBCMT) internals | ___crtMessageBoxA 0x0058C139 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | MapViewOfFile | map .wav file | _sndBUFFER::Init 0x004B9450; _sndSTR_BUFFER::Init (feb 0x004AA5C0; v12 call ~0x004BA12E) | mmap |
| KERNEL32.DLL | MultiByteToWideChar | char->wchar for DirectShow file names, UI strings | PlayMedia@?%C:\PROJECTS\WillRock\Sources\wr_cutscene.cpp324185239 0x004A5670; _sndSYSTEM::SetMusic 0x004B5AA0; __mbtowc_lk 0x0058A492; ___crtGetStringTypeA 0x0058B574; ___crtLCMapStringA 0x0058C772; ___crtCompareStringA 0x0058E0A1 | mbstowcs / iconv / SDL_iconv |
| KERNEL32.DLL | QueryPerformanceCounter | high-res frame timer | _gsMsgCalcElapsedTime 0x005109A0; gsCfgScaleDetect 0x00531CE0; PTIMER_fn_bResetStart 0x005C4E80; PTIMER_fn_ulElapsedMiliSeconds 0x005C4EB0 | SDL_GetPerformanceCounter |
| KERNEL32.DLL | QueryPerformanceFrequency | timer frequency | _gsMsgCalcElapsedTime 0x005109A0; gsCfgScaleDetect 0x00531CE0; PTIMER_fn_bResetStart 0x005C4E80 | SDL_GetPerformanceFrequency |
| KERNEL32.DLL | RaiseException | MSVC C runtime (LIBCMT) internals | __raise_exc 0x00585A02; 8::__CxxThrowException 0x005BD10E | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | ReadFile | MSVC C runtime (LIBCMT) internals | __read_lk 0x0058972F | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | ReleaseMutex | unlock netDRV mutex | netDRV::ProcessEvents (feb 0x004A6EE0; v12 twin not located); netDRV::UnlockServers (feb 0x004A76C0; v12 call ~0x0058FDE9); netDRV::AddEvent (feb 0x004A76D0; v12 call ~0x0058FEFD); netDRV::EraseClientEvents (feb 0x004A77F0; v12 call ~0x0058FF7B); ?1netDRV_DPUBI (feb 0x00576AE0; v12 call ~0x0058F66E); netDRV_DPUBI::Server_Start (feb 0x00577230; v12 call ~0x0058FDE9); netDRV_DPUBI::Server_SendToClient (feb 0x00577500; v12 call ~0x0058FDE9); netDRV_DPUBI::FindDPlayPlayerPre (feb 0x005780A0; v12 call ~0x00590D39); netDRV_DPUBI::AddDPlayPlayer_Pre (feb 0x005781A0; v12 call ~0x00590D39); netDRV_DPUBI::AddDPlayPlayer_Fin (feb 0x005783C0; v12 call ~0x00590D39); netDRV_DPUBI::AddUbiPlayer (feb 0x00578460; v12 call ~0x00590D39); netDRV_DPUBI::RemovePlayer (feb 0x00578510; v12 call ~0x00590FDE); netDRV_DPUBI::Dp_Server_OnQuery (feb 0x00578700; v12 call ~0x00591045); netDRV_DPUBI::Dp_Server_OnPlayerDisconnected (feb 0x00578850; v12 call ~0x00591045); netDRV_DPUBI::Dp_Server_OnDataReceived (feb 0x005788D0; v12 call ~0x00591045); netDRV_DPUBI::Ubi_Server_OnPlayerDisconnected (feb 0x00578E20; v12 call ~0x00593186); CMsgQueue::Clear 0x005D5080; CMsgQueue::GetMessageA 0x005D50E0; CMsgQueue::GetFrontMessageType 0x005D5360; CMsgQueue::AddMessage 0x005D5540 | SDL_UnlockMutex |
| KERNEL32.DLL | RtlUnwind | MSVC C runtime (LIBCMT) internals | 16::_RtlUnwind 0x0058E660 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | SetEndOfFile | MSVC C runtime (LIBCMT) internals | __chsize_lk 0x0058D9F1 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | SetEnvironmentVariableA | MSVC C runtime (LIBCMT) internals | ___crtsetenv 0x0058E349 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | SetFilePointer | MSVC C runtime (LIBCMT) internals | __lseek_lk 0x0058941A | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | SetHandleCount | MSVC C runtime (LIBCMT) internals | __ioinit 0x00586A78 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | SetLastError | MSVC C runtime (LIBCMT) internals | __getptd 0x00584F58 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | SetStdHandle | MSVC C runtime (LIBCMT) internals | __set_osfhnd 0x0058C2E5; __free_osfhnd 0x0058C361 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | SetUnhandledExceptionFilter | crash handler apGPF_FILTER (writes a GPF log); C++ EH | apGPF_FILTER::SetupFilter 0x004B0630; __CxxSetUnhandledExceptionFilter (feb 0x00593279; v12 call ~0x005BDBE0); __CxxRestoreUnhandledExceptionFilter (feb 0x0059328A; v12 call ~0x005BDBF2) | signal(SIGSEGV)/sigaction or drop |
| KERNEL32.DLL | Sleep | yield / frame limiter / thread waits | d3dDRIVER::SwapBuffers 0x004BB820; _gsMsgCalcElapsedTime 0x005109A0; _M_do_lock@?$_STLP_mutex_spin@$0A 0x005A8640; _M_acquire_lock@_STLP_mutex_base@_STL 0x005A8880; _M_allocate@?$__node_alloc@$00$0A 0x005A8D10 | SDL_Delay |
| KERNEL32.DLL | TerminateProcess | MSVC C runtime (LIBCMT) internals | _doexit 0x00580FCC | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | TlsAlloc | MSVC C runtime (LIBCMT) internals | __mtinit 0x00584EF1 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | TlsGetValue | MSVC C runtime (LIBCMT) internals | __getptd 0x00584F58 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | TlsSetValue | MSVC C runtime (LIBCMT) internals | __mtinit 0x00584EF1; __getptd 0x00584F58 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | UnhandledExceptionFilter | MSVC C runtime (LIBCMT) internals | __XcptFilter 0x00586470 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | UnmapViewOfFile | unmap .wav | _sndBUFFER::Init 0x004B9450; _sndSTR_BUFFER::Init (feb 0x004AA5C0; v12 call ~0x004BA157); _sndSTR_BUFFER::Term (feb 0x004AA720; v12 call ~0x004BA71F) | munmap |
| KERNEL32.DLL | VirtualAlloc | MSVC C runtime (LIBCMT) internals | ___sbh_alloc_new_region 0x00587C18; ___sbh_alloc_new_group 0x00587CC9; ___old_sbh_new_region 0x005880BA; ___old_sbh_alloc_block 0x005883B2 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | VirtualFree | MSVC C runtime (LIBCMT) internals | ___sbh_free_block 0x005875E6; ___old_sbh_new_region 0x005880BA; ___old_sbh_release_region 0x005881FE; ___old_sbh_decommit_pages 0x00588254 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | WaitForSingleObject | lock netDRV mutex | netDRV::ProcessEvents (feb 0x004A6EE0; v12 twin not located); netDRV::LockServers (feb 0x004A76A0; v12 call ~0x0058FDD1); netDRV::AddEvent (feb 0x004A76D0; v12 call ~0x0058FE0B); netDRV::EraseClientEvents (feb 0x004A77F0; v12 call ~0x0058FF21); ?1netDRV_DPUBI (feb 0x00576AE0; v12 call ~0x0058F624); netDRV_DPUBI::Server_Start (feb 0x00577230; v12 call ~0x0058FDD1); netDRV_DPUBI::Server_SendToClient (feb 0x00577500; v12 call ~0x0058FE0B); netDRV_DPUBI::FindDPlayPlayerPre (feb 0x005780A0; v12 call ~0x00590D20); netDRV_DPUBI::AddDPlayPlayer_Pre (feb 0x005781A0; v12 call ~0x00590D20); netDRV_DPUBI::AddDPlayPlayer_Fin (feb 0x005783C0; v12 call ~0x00590D20); netDRV_DPUBI::AddUbiPlayer (feb 0x00578460; v12 call ~0x00590D20); netDRV_DPUBI::RemovePlayer (feb 0x00578510; v12 call ~0x00590D20); netDRV_DPUBI::Dp_Server_OnQuery (feb 0x00578700; v12 call ~0x00591012); netDRV_DPUBI::Dp_Server_OnPlayerDisconnected (feb 0x00578850; v12 call ~0x00591012); netDRV_DPUBI::Dp_Server_OnDataReceived (feb 0x005788D0; v12 call ~0x00591012); netDRV_DPUBI::Ubi_Server_OnPlayerDisconnected (feb 0x00578E20; v12 call ~0x0059310B); CMsgQueue::Clear 0x005D5080; CMsgQueue::GetMessageA 0x005D50E0; CMsgQueue::GetFrontMessageType 0x005D5360; CMsgQueue::AddMessage 0x005D5540 | SDL_LockMutex |
| KERNEL32.DLL | WideCharToMultiByte | wchar->char (UI strings, ini); CRT | ___crtGetEnvironmentStringsA 0x00586946; __wctomb_lk 0x0058A2E4; __tzset_lk 0x0058B6EB; ___crtLCMapStringA 0x0058C772; ___wtomb_environ 0x0058DE39 | wcstombs / SDL_iconv |
| KERNEL32.DLL | WriteFile | MSVC C runtime (LIBCMT) internals | __NMSG_WRITE 0x00586E3F; __write_lk 0x00587195 | host libc / C++ runtime: nothing to port |
| KERNEL32.DLL | WritePrivateProfileStringA | write .ini | apCfgTerm 0x004AF730; apCfgWriteInt 0x004AFAD0; apCfgWriteDword 0x004AFB20; apCfgWriteBool 0x004AFB70; apCfgWriteFloat 0x004AFBD0; apCfgWriteString 0x004AFC20 | portable INI writer |
| OLE32.DLL | CoCreateInstance | create DirectShow FilterGraph (music, cutscenes), DirectPlay objects, DX9 detection | _gsDetectDx9 0x004A4640; wrCUTSCENE::Play 0x004A5910; _sndSYSTEM::InitMusic 0x004B5910; netDP_SERVER::StartServer (feb 0x0057D250; v12 twin not located); netDP_CLIENT::StartBrowser (feb 0x0057D930; v12 twin not located); netDP_CLIENT::EnumHosts (feb 0x0057DC60; v12 twin not located) | music: libvorbis/minimp3/dr_mp3 + OpenAL stream; video: libmpeg2/FFmpeg/pl_mpeg; drop DX9 check |
| OLE32.DLL | CoInitialize | COM init (for DirectShow music/video and DirectPlay) | gsAPP_SYSTEM::InitAll (feb 0x004FA360; v12 call ~0x0050D0A7) | drop |
| OLE32.DLL | CoUninitialize | COM shutdown | wrCUTSCENE::Play 0x004A5910; gsAPP_SYSTEM::TermAll (feb 0x004FA760; v12 call ~0x0050D5C0) | drop |
| OLEAUT32.DLL | VariantClear | same | _gsDetectDx9 0x004A4640 | drop |
| OLEAUT32.DLL | VariantInit | VARIANT for the DX9 detect query (IDxDiag-like) | _gsDetectDx9 0x004A4640 | drop |
| SHELL32.DLL | ShellExecuteA | open the web site from the main menu button | wrUI_BUTTON_WEBSITE::ProcessCommand 0x0048B6A0 | SDL_OpenURL |
| USER32.DLL | AdjustWindowRectEx | window size for client area | gsWIN_APP_SYSTEM::ResizeWin 0x004A49D0 | SDL_SetWindowSize |
| USER32.DLL | ClientToScreen | windowed-mode blit rect in SwapBuffers/CopyBuffer/ClearSurfaces | d3dDRIVER::SwapBuffers 0x004BB820; d3dDRIVER::CopyBuffer 0x004BBBC0; d3dDRIVER::ClearSurfaces 0x004BBCC0 | not needed with GL swap |
| USER32.DLL | CreateDialogParamA | in-game Win32 config/debug dialogs (gsCFG_SYSTEM) | gsCFG_SYSTEM::Init (feb 0x00524880; v12 twin not located); _gsCfgMain_WND (feb 0x00524AF0; v12 twin not located) | drop or Dear ImGui |
| USER32.DLL | CreateWindowExA | main window 640x480 WS_OVERLAPPED/WS_SYSMENU (0x880000) | gsWIN_APP_SYSTEM::Init 0x004A47E0 | SDL_CreateWindow |
| USER32.DLL | DefWindowProcA | default window procedure in _gsWndProc | _gsWndProc 0x004A4E00 | drop: translate SDL events |
| USER32.DLL | DeleteMenu | strip system menu | gsWIN_APP_SYSTEM::Init 0x004A47E0 | drop |
| USER32.DLL | DestroyWindow |  | gsWIN_APP_SYSTEM::Term 0x004A4960; gsCFG_SYSTEM::Term (feb 0x005248E0; v12 twin not located) |  |
| USER32.DLL | DialogBoxParamA | modal config dialog | _gsCfgControl_WND (feb 0x005266D0; v12 twin not located) | drop or ImGui |
| USER32.DLL | DispatchMessageA | message pump | gsWIN_APP_SYSTEM::MainLoop 0x004A4C60; gsCFG_SYSTEM::Show (feb 0x00524900; v12 twin not located); _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CB7D) | SDL_PollEvent |
| USER32.DLL | EnableWindow | dialog controls | _gsCfgLoadSave_WND (feb 0x00524E10; v12 twin not located); _gsCfgMP_WND (feb 0x005255D0; v12 twin not located); _gsCfgVideo_WND (feb 0x00525A80; v12 twin not located); _gsCfgSound_WND (feb 0x00526260; v12 twin not located) | drop |
| USER32.DLL | EndDialog | dialog | _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053C9F4) | drop |
| USER32.DLL | FindWindowA | single-instance check by class name | gsWIN_APP_SYSTEM::IsAlreadyRunning 0x004A49A0 | lock file / named semaphore |
| USER32.DLL | GetAsyncKeyState | Alt/Shift check in WndProc, key held at startup (safe mode video?) | _gsWndProc 0x004A4E00; gsVideoInit (feb 0x0051D960; v12 call ~0x005332BD) | SDL_GetKeyboardState / SDL_GetModState |
| USER32.DLL | GetClientRect | client size (cutscene video window, dialogs) | PlayMedia@?%C:\PROJECTS\WillRock\Sources\wr_cutscene.cpp324185239 0x004A5670; _ListView_AddColumn 0x004B0350; _gsCfgControl_WND (feb 0x005266D0; v12 twin not located) | SDL_GetWindowSize |
| USER32.DLL | GetDC | desktop DC for bpp query | vidGetDriverList 0x004B3B60 | SDL_GetDesktopDisplayMode |
| USER32.DLL | GetDlgItem | dialog controls | TabControl_AddPage 0x004B02F0; gsCFG_SYSTEM::ShowMsg (feb 0x005245F0; v12 twin not located); gsCFG_SYSTEM::ShowMsgErr (feb 0x00524660; v12 twin not located); gsCFG_SYSTEM::Show (feb 0x00524900; v12 twin not located); _gsCfgMain_WND (feb 0x00524AF0; v12 twin not located); _gsCfgLoadSave_WND (feb 0x00524E10; v12 twin not located); _gsCfgMP_WND (feb 0x005255D0; v12 twin not located); _gsCfgVideo_WND (feb 0x00525A80; v12 twin not located); _gsCfgSound_WND (feb 0x00526260; v12 twin not located); _gsCfgControl_WND (feb 0x005266D0; v12 twin not located); _gsCfgUpdateCmdList 0x0053C860; _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CAC7) | drop |
| USER32.DLL | GetDlgItemInt | dialog controls | _gsCfgScale_WND (feb 0x0051CBC0; v12 call ~0x00532537); _gsCfgDlg2Config (feb 0x0051D4D0; v12 call ~0x00532E13); _gsCfgDebug_WND (feb 0x005270E0; v12 call ~0x0053CF0C) | drop |
| USER32.DLL | GetDlgItemTextA | dialog controls | GetDlgItemFloat 0x004B02C0; _gsCfgLoadSave_WND (feb 0x00524E10; v12 twin not located); _gsCfgMP_WND (feb 0x005255D0; v12 twin not located); _gsCfgDebug_WND (feb 0x005270E0; v12 call ~0x0053CCC4); _gsCfgDebug_1_WND (feb 0x00527C00; v12 call ~0x0053D7D8) | drop |
| USER32.DLL | GetDoubleClickTime | UI double click interval | _gswinGetDblClickTime (feb 0x00497210; v12 call ~0x004A4615); _inpINPUT::GetMouseInfo 0x00594980 | constant 500 ms or SDL hint |
| USER32.DLL | GetMessageA | modal dialog loop | gsCFG_SYSTEM::Show (feb 0x00524900; v12 twin not located) | drop |
| USER32.DLL | GetParent | dialog | _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CA5A) | drop |
| USER32.DLL | GetSubMenu | dialog menu | _gsCfgControl_WND (feb 0x005266D0; v12 twin not located) | drop |
| USER32.DLL | GetSystemMenu | strip system menu | gsWIN_APP_SYSTEM::Init 0x004A47E0 | drop |
| USER32.DLL | GetSystemMetrics | screen size, mouse button swap | gsWIN_APP_SYSTEM::ResizeWin 0x004A49D0; _inpINPUT::GetMouseInfo 0x00594980 | SDL_GetDesktopDisplayMode |
| USER32.DLL | GetWindowLongA | read window style | gsWIN_APP_SYSTEM::ResizeWin 0x004A49D0; _gsCfgControl_WND (feb 0x005266D0; v12 twin not located) | drop |
| USER32.DLL | GetWindowRect | window geometry | gsWIN_APP_SYSTEM::ResizeWin 0x004A49D0; _ListView_GetMenuPos 0x004B0420; _gsCfgMain_WND (feb 0x00524AF0; v12 twin not located); _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CA32) | SDL_GetWindowPosition/Size |
| USER32.DLL | InvalidateRect | dialog repaint | _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CAC1) | drop |
| USER32.DLL | LoadCursorA | arrow cursor | gsWIN_APP_SYSTEM::Init 0x004A47E0 | SDL_CreateSystemCursor |
| USER32.DLL | LoadImageA | window icon; BMP loading for pctReadBMP | gsWIN_APP_SYSTEM::Init 0x004A47E0; pctReadBMPHdr 0x00504F20; pctReadBMP 0x00504FA0 | SDL_SetWindowIcon; SDL_LoadBMP / stb_image |
| USER32.DLL | LoadMenuA | dialog menu | _gsCfgControl_WND (feb 0x005266D0; v12 twin not located) | drop |
| USER32.DLL | MapWindowPoints | list view context menu | _ListView_GetMenuPos 0x004B0420 | drop |
| USER32.DLL | MessageBeep | dialog beep | _gsCfgVideo_WND (feb 0x00525A80; v12 twin not located) | drop |
| USER32.DLL | MessageBoxA | error / yes-no message boxes (mbShow*, apMsgBox) | _gsDetectDx9 0x004A4640; mbShowOk 0x004B0160; mbShowStop 0x004B01A0; mbShowYesNo 0x004B01E0 | SDL_ShowSimpleMessageBox / SDL_ShowMessageBox |
| USER32.DLL | MoveWindow | resize window | gsWIN_APP_SYSTEM::ResizeWin 0x004A49D0; _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CAA2) | SDL_SetWindowSize |
| USER32.DLL | PeekMessageA | message pump in MainLoop | gsWIN_APP_SYSTEM::MainLoop 0x004A4C60; _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CB47) | SDL_PollEvent |
| USER32.DLL | PostMessageA | post WM_CLOSE / app messages | _gsWndProc 0x004A4E00; gsCFG_SYSTEM::Show (feb 0x00524900; v12 twin not located) | SDL_PushEvent |
| USER32.DLL | RegisterClassA | fallback of RegisterClassExA | gsWIN_APP_SYSTEM::Init 0x004A47E0 | drop |
| USER32.DLL | RegisterClassExA | window class "SABER3D" with _gsWndProc | gsWIN_APP_SYSTEM::Init 0x004A47E0 | drop |
| USER32.DLL | ReleaseDC | same | vidGetDriverList 0x004B3B60 | same |
| USER32.DLL | ScreenToClient | config dialog | _gsCfgMain_WND (feb 0x00524AF0; v12 twin not located) | drop |
| USER32.DLL | SendDlgItemMessageA | dialog controls | SetCBItemState 0x004B0220; GetCBItemState 0x004B0250; _gsCfgScale_WND (feb 0x0051CBC0; v12 call ~0x0053253D); _gsCfgVideo_WND (feb 0x00525A80; v12 twin not located); _gsCfgSound_WND (feb 0x00526260; v12 twin not located); _gsCfgControl_WND (feb 0x005266D0; v12 twin not located) | drop |
| USER32.DLL | SendMessageA | app message 0x401 (activate), list-view/tab controls of dialogs | gsWIN_APP_SYSTEM::ActivateApp 0x004A4DD0; TabControl_AddPage 0x004B02F0; _ListView_AddColumn 0x004B0350; _ListView_GetMenuPos 0x004B0420; _ListView_AddItem 0x004B04D0; _ListView_GetSelected 0x004B0550; _ListView_GetSelectedData 0x004B05A0; gsCFG_SYSTEM::ShowMsg (feb 0x005245F0; v12 twin not located); gsCFG_SYSTEM::ShowMsgErr (feb 0x00524660; v12 twin not located); gsCFG_SYSTEM::Show (feb 0x00524900; v12 twin not located); _gsCfgMain_WND (feb 0x00524AF0; v12 twin not located); _gsCfgUpdateCmdList 0x0053C860 | direct call / drop dialogs |
| USER32.DLL | SetDlgItemInt | dialog controls | _gsCfgScale_WND (feb 0x0051CBC0; v12 call ~0x00532699); _gsCfgConfig2Dlg (feb 0x0051CD90; v12 call ~0x00532771); _gsCfgDebug_WND (feb 0x005270E0; v12 call ~0x0053D5BC) | drop |
| USER32.DLL | SetDlgItemTextA | dialog controls | SetDlgItemFloat 0x004B0270; _gsCfgLoadSave_WND (feb 0x00524E10; v12 twin not located); _gsCfgMP_WND (feb 0x005255D0; v12 twin not located); _gsCfgDebug_WND (feb 0x005270E0; v12 call ~0x0053D0D9); _gsCfgDebug_1_WND (feb 0x00527C00; v12 call ~0x0053DA5E) | drop |
| USER32.DLL | SetFocus | focus main window | gsWIN_APP_SYSTEM::ActivateApp 0x004A4DD0; _gsCfgControl_WND (feb 0x005266D0; v12 twin not located) | SDL_RaiseWindow |
| USER32.DLL | SetForegroundWindow | bring to front (already running, after cutscene) | gsWIN_APP_SYSTEM::IsAlreadyRunning 0x004A49A0; PlayMedia@?%C:\PROJECTS\WillRock\Sources\wr_cutscene.cpp324185239 0x004A5670 | SDL_RaiseWindow |
| USER32.DLL | SetRect | RECT helper | d3dDRIVER::ClearSurfaces 0x004BBCC0 | inline |
| USER32.DLL | SetWindowLongA | set window style windowed/fullscreen | - | SDL_SetWindowFullscreen / SDL_SetWindowBordered |
| USER32.DLL | SetWindowPos | reposition window on mode change | gsWIN_APP_SYSTEM::ResizeWin 0x004A49D0; _gsCfgMain_WND (feb 0x00524AF0; v12 twin not located) | SDL_SetWindowPosition / SDL_SetWindowFullscreen |
| USER32.DLL | ShowCursor | hide/show cursor | wifOpenFile (feb 0x004A1490; v12 call ~0x004B0076) | SDL_ShowCursor |
| USER32.DLL | ShowWindow | show/minimize window | wrUI_BUTTON_WEBSITE::ProcessCommand 0x0048B6A0; gsWIN_APP_SYSTEM::Init 0x004A47E0; gsWIN_APP_SYSTEM::Term 0x004A4960; gsWIN_APP_SYSTEM::IsAlreadyRunning 0x004A49A0; gsWIN_APP_SYSTEM::ProcessActDeact 0x004A4B10; gsCFG_SYSTEM::Term (feb 0x005248E0; v12 twin not located); gsCFG_SYSTEM::Show (feb 0x00524900; v12 twin not located); _gsCfgMain_WND (feb 0x00524AF0; v12 twin not located); _gsCfgVideo_WND (feb 0x00525A80; v12 twin not located); _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CAA8) | SDL_ShowWindow/SDL_MinimizeWindow |
| USER32.DLL | TrackPopupMenu | dialog popup menu | _gsCfgControl_WND (feb 0x005266D0; v12 twin not located) | drop |
| USER32.DLL | TranslateMessage | message pump | gsWIN_APP_SYSTEM::MainLoop 0x004A4C60; gsCFG_SYSTEM::Show (feb 0x00524900; v12 twin not located); _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CB72) | SDL_PollEvent / SDL_TEXTINPUT |
| USER32.DLL | UnregisterClassA | window class cleanup | gsWIN_APP_SYSTEM::Term 0x004A4960 | drop |
| USER32.DLL | UpdateWindow | repaint | gsWIN_APP_SYSTEM::Init 0x004A47E0; gsCFG_SYSTEM::ShowMsg (feb 0x005245F0; v12 twin not located); gsCFG_SYSTEM::ShowMsgErr (feb 0x00524660; v12 twin not located); gsCFG_SYSTEM::Show (feb 0x00524900; v12 twin not located); _gsCfgShortCut_WND (feb 0x00526E90; v12 call ~0x0053CAB3) | drop |
| USER32.DLL | WaitMessage | sleep while inactive | gsWIN_APP_SYSTEM::MainLoop 0x004A4C60 | SDL_WaitEvent |
| WINMM.DLL | timeGetTime | ms timer | snmMANAGER::Init (feb 0x004A8010; v12 twin not located); snmMANAGER::BeginFrame (feb 0x004A84A0; v12 call ~0x004B7E22); gsMsgInit (feb 0x004FC420; v12 twin not located); _gsMsgCalcElapsedTime 0x005109A0; gsDEMO::Play_DispatchFrame (feb 0x00529AF0; v12 call ~0x0053F801); _inpINPUT::GetMouseInfo 0x00594980 | SDL_GetTicks |
| WS2_32.DLL | WSAGetLastError | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::CreateTCP 0x005A1F00; clSocket::CreateUDP 0x005A1F40; clSocket::Bind 0x005A1F80; clSocket::Connect 0x005A1FE0; clSocket::Connect 0x005A2140; clSocket::Send 0x005A2250; clSocket::SendTo 0x005A2290; clSocket::Receive 0x005A2310; clSocket::ReceiveFrom 0x005A2360; clSocket::GetPeerAddress 0x005A2550; clSocket::GetLocalAddress 0x005A25C0; clSocket::Close 0x005A2640; clSocket::SetNonBlocking 0x005A2670; clSocket::SetRemoveNagle 0x005A26B0; clSocket::SetReUseAddr 0x005A2700; clSocket::SetBroadcast 0x005A2750; clSocket::SetSendBuffer 0x005A27A0; clSocket::SetRcvBuffer 0x005A27E0 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | WSAIoctl | BSD-socket API (clSocket, GS/Ubi.com libs) | GetNetInterface 0x005A1B10 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | WSAStartup | Winsock init (Ubi.com/DirectPlay replacement transport) | 4::_InitializeSockets (feb 0x005854E0; v12 call ~0x005A1A10) | drop (BSD sockets need no init) |
| WS2_32.DLL | __WSAFDIsSet | BSD-socket API (clSocket, GS/Ubi.com libs) | 8::___WSAFDIsSet 0x005BDC5E | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | bind | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::Bind 0x005A1F80 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | closesocket | BSD-socket API (clSocket, GS/Ubi.com libs) | GetNetInterface 0x005A1B10; clSocket::Close 0x005A2640 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | connect | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::Connect 0x005A1FE0; clSocket::Connect 0x005A2140 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | gethostbyname | BSD-socket API (clSocket, GS/Ubi.com libs) | 4::_InitializeSockets (feb 0x005854E0; v12 call ~0x005A1A48); MakeSocketAddress 0x005A1D50; clSocket::AddrToInt 0x005A1E80 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | getpeername | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::GetPeerAddress 0x005A2550 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | getsockname | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::GetLocalAddress 0x005A25C0 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | getsockopt | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::Connect 0x005A1FE0; clSocket::Connect 0x005A2140 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | htonl | BSD-socket API (clSocket, GS/Ubi.com libs) | IPIsInRange 0x005A1AD0; clDataSerialize::SerializeBin 0x005C21F0; clMessage::PutMessageSize 0x005C4FB0; CGLSessionLib::SessionSend_NetInterface 0x005D6460 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | htons | BSD-socket API (clSocket, GS/Ubi.com libs) | MakeSocketAddress 0x005A1D50; MakeSocketAddress 0x005A1E10 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | inet_addr | BSD-socket API (clSocket, GS/Ubi.com libs) | IPDotToDecimal 0x005A1AB0; clSocket::AddrToInt 0x005A1E80 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | inet_ntoa | BSD-socket API (clSocket, GS/Ubi.com libs) | 4::_InitializeSockets (feb 0x005854E0; v12 call ~0x005A1A64); clSocket::AddrToInt 0x005A1E80; clSocket::GetLocalAddress 0x005A25C0 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | ioctlsocket | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::SetNonBlocking 0x005A2670 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | ntohl | BSD-socket API (clSocket, GS/Ubi.com libs) | clTCPClient::ConnectHost 0x005BE140; clDataLong::AddNetValue 0x005C1630; clDataSerialize::ExtractBin 0x005C1BB0; clMessage::GetMessageSize 0x005C4F20 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | ntohs | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::ReceiveFrom 0x005A2360; clSocket::ReceiveFrom 0x005A24C0; clSocket::GetPeerAddress 0x005A2550; clSocket::GetLocalAddress 0x005A25C0 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | recv | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::Receive 0x005A2310 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | recvfrom | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::ReceiveFrom 0x005A2360; clSocket::ReceiveFrom 0x005A24C0 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | select | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::Connect 0x005A1FE0; clSocket::Connect 0x005A2140 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | send | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::Send 0x005A2250 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | sendto | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::SendTo 0x005A2290 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | setsockopt | BSD-socket API (clSocket, GS/Ubi.com libs) | clSocket::SetRemoveNagle 0x005A26B0; clSocket::SetReUseAddr 0x005A2700; clSocket::SetBroadcast 0x005A2750; clSocket::SetSendBuffer 0x005A27A0; clSocket::SetRcvBuffer 0x005A27E0 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
| WS2_32.DLL | socket | BSD-socket API (clSocket, GS/Ubi.com libs) | GetNetInterface 0x005A1B10; clSocket::CreateTCP 0x005A1F00; clSocket::CreateUDP 0x005A1F40 | POSIX sockets, nearly 1:1 (ioctlsocket->fcntl/ioctl, closesocket->close, WSAGetLastError->errno, __WSAFDIsSet->FD_ISSET) |
