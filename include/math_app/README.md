# math_app: m3d math library + application framework (Will Rock 1.2)

This is part of an unofficial modding SDK and porting map for Will Rock (Saber Interactive, 2003). It was built by static analysis only. The target is 1.2 (`v12.exe`). The reference is the Feb 2003 build (`feb.exe`, `WillRock.map`, Ghidra decompile).

Files in this folder:

| file | content |
|---|---|
| `m3d.h` | m3d value types (m3dV, m3dMATR, m3dQUAT, m3dPOLY/m2dPOLY, m3dCOLOR), the bounding volumes (m3dVOL, m3dBOX, m3dPRISM, m3dSPHERE, m3dOBB) with their vtables, splines (m3dSPL), controllers, noise, voxGRID, random numbers, the key free functions and the m3d globals |
| `m3d_index.h` | Lookup table of all 335 m3d library functions (feb map) with their v12 address and how each was matched |
| `app.h` | WinMain, gsAPP_SYSTEM → gsWIN_APP_SYSTEM → wrAPP_SYSTEM, InitAll order, main loop, window procedure, gsAppState flags, command line, frame clock, crash handler, and the remaining ap.lib helpers |
| `win32_imports.tsv` | Every Win32/DirectX import of v12.exe (202 functions) with its IAT slot and every v12 call site. This is the porting checklist for the whole executable. |

These live elsewhere: `apNAME`, `arrVector`, `apMalloc/apFree/apRealloc`, `apLog`, and the INI reader `apCfg*` are in `re/sdk/engine_core/core_types.h`. They are not repeated here.

---

## 1. m3d: how it works

`m3d.lib` contains only portable math: x87 floats, no OS calls, and no global state apart from a few constants and the SIMD selector. **Between feb and 1.2, 228 of the 335 indexed functions (m3d.lib including its camera-transform objs) are byte-identical apart from relocations; most of the rest were located by call graph, vtable or prologue bytes** (marked `exact` in `m3d_index.h`). The struct layouts did not change at all.

- **Coordinates:** Y is up. The `m3xz*` functions work on the ground plane. Angles passed to the API are in **degrees**: `RotateX/Y/Z`, `SetAngAxis` and `MakeRotRay` multiply by π/180 internally.
- **m3dMATR:** a 4×4 float matrix, row-major, using row vectors (`p' = p·M`, the Direct3D convention).
  - Rows are X axis, Y axis, Z axis, then origin (`+0x30`).
  - The `op` argument of `Translate/Rotate/Scale/Transform` takes three values: 0 = set, 1 = local (pre-multiply), 2 = global (post-multiply).
- **m3dQUAT** is stored as `{w,x,y,z}`.
- **Polygons** have a fixed capacity of 20 vertices. `m3dPOLY` is 0xF4 bytes and `m2dPOLY` is 0xA4 bytes. Clipping functions never allocate memory.
- **Bounding volumes** share one 13-slot vtable layout. The `type` field at `+4` holds 1 = BOX, 2 = PRISM, 3 = SPHERE, 4 = OBB, and `m3dVOL::Make(type)` is the factory. These volumes are used by collision (cdt*), visibility, the animation bounding volumes (`abv*_UPD` derive from them) and the camera `Qual*POV` frustum tests.
- **Splines:** `m3dSPL::Make(type)` builds one of nine types: linear 1D/2D/3D, Hermite, Bézier 2D/3D, Lagrange, quaternion and colour.
  - Key points are a packed array (`kp` at `+0x24`, stride at `+0x18`) whose first float is the time.
  - `FindSeg` does a binary search.
  - Animation, camera paths and the `m3dCTRL_*` particle/light controllers use these splines.
- **Random numbers come from two sources:**
  - `m3dRandom*` and `m3dMakeV*Random` call the CRT `rand()`, so they share the CRT seed.
  - `m3dRND_GEN` is a private LCG (`seed*0x343FD+0x269EC3`) for reproducible streams.
- **SIMD:**
  - At init, `_m3dDetectSIMD` runs CPUID and sets `m3dSimdType` to 0 = SSE, 1 = 3DNow!, 2 = none.
  - `TransformPointList/TransformVectorList` and the inlined `m3dNormalize` copies branch on that value. The inlined copies are static per .obj, for example at v12 0x00402D80 and 0x00414080.
  - **Port/mod note:** writing 2 to `m3dSimdType` (v12 0x00637C24) after `m3dInit` forces the plain x87 paths everywhere. On a modern CPU the 3DNow! path is never selected.
- **Lifecycle:** `m3dInit` (v12 0x005695C0) is called from `gsAPP_SYSTEM::InitAll`, right after the window is created. It runs DetectSIMD, Init2DMatr, Init3DMatr, then InitNoise (the global `m3dNoise` Perlin table). `m3dTerm` (v12 0x005695F0) frees the noise table.

### 1.2 vs feb (m3d)

- `m3dBOX` and `m3dOBB` gained their own `QualPlane` override in vtable slot `+0x30`:
  - BOX v12 0x0056E660 tests the 8 corners.
  - OBB v12 0x00578E50.
  - In feb both used `m3dVOL::QualPlane`, which takes the enclosing sphere, scales its radius by 0.7 and runs the sphere test.
  - Effect: plane culling and splitting of boxes became exact in 1.2.
- `m3dQUAT::Identity` no longer exists out of line in v12; it is always inlined.
- These functions were recompiled with a different stack layout (same vtable slot): `m3dSPL_LINEAR1D::GetValue` (v12 0x00566ED0), `m2dMATR::MakeRot` (?), and `m3dClampVector_*` (v12 address not found).
- Everything else is identical. Address shift feb → v12 inside m3d is +0x17000 … +0x19000.

---

## 2. Application: how it starts, runs and stops

```
WinMain (v12 0x004AA010)
 ├─ store hInstance / lpCmdLine, install wrGPF_FILTER (crash log "WillRock_GPF.log")
 ├─ gsAppSystem = new wrAPP_SYSTEM("WR_WIN_CLASS","Will Rock",icon 0x67)
 ├─ [1.2] if IsAlreadyRunning() -> quit       (FindWindow on the class name)
 ├─ new every gs* subsystem object (list + sizes in app.h)
 ├─ gsAppState |= 0x8000 (disables Alt+Enter); [1.2] "-dedicated" -> gsAppState |= 0x08000000
 └─ gsRun (v12 0x0050D030)
      ├─ gsAPP_SYSTEM::InitAll (v12 0x0050D0A0): CoInitialize, apInit, fnmInit, apCfgInit("WILLROCK.INI"),
      │     app->Init (DX9 check via DxDiag COM, "-safemode", RegisterClassEx + CreateWindowEx 640x480),
      │     m3dInit, pct/obj/cdto/fnt, msgSystem, input, gsMPInit, gsVideoInit+ApplySettings, particles,
      │     pte*, camera, gsMsgInit, gsc, render, sound, level, gss, ent, dyn, phys, vis, shadows, mirrors,
      │     app->InitSubsys (all Will Rock game classes), uiSystem, gsSysMP, gsp, gsl, app->ActivateApp
      ├─ app->MainLoop (v12 0x004A4C60): PeekMessage pump -> ProcessActDeact -> gsMsgCallFrame
      └─ gsAPP_SYSTEM::TermAll (v12 0x0050D4A0) -> back in WinMain, delete all subsystems
```

**Per frame.** One loop iteration of `MainLoop` runs the following steps. It is single-threaded, with a variable time step.

1. Pump all window messages (`_gsWndProc` v12 0x004A4E00).
2. Call `ProcessActDeact` (alt-tab / focus handling).
3. When minimised, call `WaitMessage`.
4. Call `gsMsgCallFrame` (v12 0x00510B90). It runs these steps in order:
   1. `_gsMsgCalcElapsedTime` (v12 0x005109A0) computes the frame clock.
   2. It computes the FPS figure, which is written to `vidDRIVER::SetFPS`.
   3. `_gsMsgProcessFrameProfile` (v12 0x00510CB0) runs `msgSYSTEM::DispatchAll`. Through it, every system and entity gets its frame message, and rendering happens.
5. Call `gsSysInput->Acquire()`.
6. If a cutscene is active, `gspCutscene->Update` replaces steps 4 and 5.

**Frame clock.**

- It uses `QueryPerformanceCounter`, falling back to `timeGetTime`.
- A frame shorter than 5 ms makes the game `Sleep`, so there is a hard 200 FPS cap.
- The result is multiplied by a game-speed float at v12 0x006354E8 (1.0) and clamped to 0.1 s.
- 1.2 adds a one-shot override (v12 0x006354EC) and keeps the fixed-step override (v12 0x0098EA04).
- The result is stored in `gsElapsedTime` (v12 0x0098E9F4). Every Process function reads this value.

**Data files read by this layer:**

- `WILLROCK.INI`, through `apCfgInit`/`apCfg*`. Sections used here: `[Video] FullScr / SizeX / SizeY`, `[Debug] SwitchWindow`, `[Debug] Decomp_ProfCallFr`, `[CONFIG] IsScaleInited / IsScaleMinimal`.
- The registry key `HKLM\SOFTWARE\Saber Interactive\Will Rock`, value `Language`. If it equals `German`, `wrAppState |= 1|2`, which turns on the low-gore mode.
- Crash logs: `ap_gpf.log` (generic) and `WillRock_GPF.log` (wrGPF_FILTER::GetFileName).

### 1.2 vs feb (application)

| change | where |
|---|---|
| `-dedicated` command-line switch → `gsAppState` bit 0x08000000. The UI and game-over code test this bit. | WinMain v12 0x004AA3D0 |
| `IsAlreadyRunning` moved from gsRun into WinMain | v12 0x004AA077 |
| `wrSTRINGS::Init` moved from InitSubsys to the front of `wrAPP_SYSTEM::Init` | v12 0x004AA623 |
| The window is created with `WS_VISIBLE` (style 0x10880000, was 0x00880000), and its original style is saved in a new global (v12 0x0064FF30) that `ResizeWin` uses | v12 0x004A48DA / 0x004A490D |
| One-shot "next frame delta" override added to the frame clock | v12 0x00510B5F |
| Subsystem object sizes changed: gsMP_SERVER 0xA00→0x2CD4, gsMP_CLIENT 0x510→0x281C, wrSTRINGS 0x34→0x30, gsSND 0xF8→0xF4, gsVIS 0x3D0→0x3D4, wrSTAT 0x140→0x144 | WinMain |
| Vtable slots moved by one in `InitAll`: gsVIS Init +0x74→+0x78, uiSystem Init +0x30→+0x34 | v12 0x0050D387 / 0x0050D409 |
| wrAPP/gsWIN/gsAPP vtables: same 12 slots in the same order | |

---

## 3. TWEAK table (v12 addresses)

| what | v12 address | value / how |
|---|---|---|
| Game speed (time scale) | 0x006354E8 (.data float) | 1.0. Example: 0.5 gives slow motion for everything that uses `gsElapsedTime` |
| One-shot next-frame delta | 0x006354EC (.data float) | -1.0 = off. Any value > 0 is used for one frame, then reset |
| Fixed frame delta | 0x0098EA04 (float) | 0 = off |
| FPS cap (min frame time 5 ms) | 0x005FB650 | **shared** `__real@3ba3d70a`. Do not patch the constant; patch the `fcomp/fld` at v12 0x00510A2F / 0x00510A3C to point at your own float, or NOP the `Sleep` at 0x00510A54 |
| Max frame delta 0.1 s | 0x00510B04 (immediate `0x3DCCCCCD`) + compare const 0x005F91BC (shared) | Patch the immediate and the compare together |
| Initial window size 640×480 | push 0x1E0 / 0x280 at v12 0x004A48CB / 0x004A48D0 | `ResizeWin` then applies `[Video] SizeX/SizeY` |
| Window class / title | "WR_WIN_CLASS" 0x00632EA8, "Will Rock" 0x00632EB8 | Changing the class lets two copies run, because `IsAlreadyRunning` matches on the class name |
| INI file name | "WILLROCK.INI" 0x00632EC4 (returned by v12 0x004AA610) | |
| Alt+Enter fullscreen toggle | bit 0x8000 of `gsAppState` (0x0098E824) | Cleared = enabled. WinMain sets it at v12 0x004AA3CD (`or ch,0x80`) |
| Low gore (German) | `wrAppState` 0x0064FFA8 bit 2 | |
| Force x87 math | `m3dSimdType` 0x00637C24 = 2 | |
| m3dVOL plane-test sphere factor | 0x005F9808 (0.7f, maybe shared) | |
| Polygon vertex limit | 20 (compile-time; struct sizes) | Not tweakable |

## 4. HOOK points

| hook | v12 address | use |
|---|---|---|
| `wrAPP_SYSTEM::InitSubsys` | vtable slot `0x00606238` → 0x004AA6A0 | Register mod entity classes or messages after the game's own (call the original first) |
| `wrAPP_SYSTEM::TermSubsys` | slot `0x00606240` → 0x004AACC0 | Mod cleanup |
| `gsAPP_SYSTEM::InitAll` | 0x0050D0A0 | Wrap it for an early or late whole-engine init (the INI is already open after `apCfgInit`) |
| `gsMsgCallFrame` | 0x00510B90 (`call` from MainLoop) | The per-frame tick; good for mod scripts, overlays and timers |
| `_gsMsgCalcElapsedTime` | 0x005109A0 | Replace the clock (frame limiter, deterministic replay) |
| `_gsWndProc` | 0x004A4E00 (stored at v12 0x004A4839 into WNDCLASSEX) | Subclass for extra keys or windowed-mode fixes; a mod can also `SetWindowLong(GWL_WNDPROC)` on `apHMainWindow` (0x00652018) |
| `gsMsgStopLoop` | 0x00510980 | Quit cleanly |
| `apMsgBox` / `apLog` | 0x004AF3E0 / 0x004AF480 | Redirect error boxes to a log (headless / dedicated) |
| Crash handler | `apGPF_FILTER::pFilter` 0x00656180; wr filter object 0x0064FFA4 | Put your own `GetFileName/DumpInfo` vtable there to add mod info to crash logs |
| `m3dVOL` / `m3dSPL` vtables | see m3d.h | Rarely needed; these are pure math |

## 5. PORTING notes

**m3d.** It is portable C++. Reimplement it from `m3d.h` and `m3d_index.h`, or lift it unchanged.

- Everything except the SIMD paths is x87 code with no OS calls.
- The only CPU-specific code is `_m3dDetectSIMD` (CPUID), the SSE/3DNow! branches of `TransformPointList/TransformVectorList`, and the inlined normalise. Set `m3dSimdType = 2` and only the scalar paths are needed.
- Two dependencies: CRT `rand()` (`m3dRandom*`), and `apLog` for the SIMD message.
- For bit-exact gameplay (demos, multiplayer sync) note that the original computes in x87 80-bit precision with `fsin/fcos/fsqrt`. A port using SSE floats will drift slightly.

**Application layer.** The Win32 calls made by this subsystem, and what a port must replace (SDL2/GLFW equivalents in brackets):

| function (v12) | Win32/COM API | port replacement |
|---|---|---|
| WinMain 0x004AA010 | entry point, `lpCmdLine`, `strstr` switches | `main(argc,argv)`; keep `-safemode` and `-dedicated` |
| gsWIN_APP_SYSTEM::Init 0x004A47E0 | `InitCommonControls`, `InitCommonControlsEx`, `LoadImageA`, `LoadCursorA`, `GetStockObject`, `RegisterClassExA`, `RegisterClassA`, `CreateWindowExA`, `GetWindowLongA`, `ShowWindow`, `UpdateWindow`, `GetSystemMenu`, `DeleteMenu` | Create the window [SDL_CreateWindow]; common controls are only needed by editor dialogs |
| gsWIN_APP_SYSTEM::Term 0x004A4960 | `ShowWindow`, `DestroyWindow`, `UnregisterClassA` | SDL_DestroyWindow |
| IsAlreadyRunning 0x004A49A0 | `FindWindowA`, `ShowWindow`, `SetForegroundWindow` | Lock file or named mutex, or drop it |
| ResizeWin 0x004A49D0 | `GetWindowRect`, `GetWindowLongA`, `AdjustWindowRectEx`, `MoveWindow`, `GetSystemMetrics`, `SetWindowPos` | SDL_SetWindowSize / SDL_SetWindowFullscreen |
| ProcessActDeact 0x004A4B10 | `ShowWindow` (minimise/restore) + gsVideoTermHW/SetHWMode (render) + sound mute | Focus events [SDL_WINDOWEVENT_FOCUS_*] |
| MainLoop 0x004A4C60 | `PeekMessageA`, `TranslateMessage`, `DispatchMessageA`, `WaitMessage` | SDL_PollEvent / SDL_WaitEvent |
| ActivateApp 0x004A4DD0 | `SetFocus`, `SendMessageA(0x401)` | SDL_RaiseWindow |
| _gsWndProc 0x004A4E00 | `DefWindowProcA`, `GetAsyncKeyState` (Alt), WM_* table (see app.h) | Translate SDL events: text input → uiSystem slot +0x28, Alt+Enter, close → gsMsgStopLoop, focus → input Acquire |
| _gsMsgCalcElapsedTime 0x005109A0 | `timeGetTime` (WINMM), `QueryPerformanceFrequency/Counter`, `Sleep` | SDL_GetPerformanceCounter / std::chrono, SDL_Delay |
| gsAPP_SYSTEM::InitAll 0x0050D0A0 | `CoInitialize` | Not needed |
| _gsDetectDx9 0x004A4640 | `CoCreateInstance(CLSID_DxDiagProvider)` (DxDiag COM) | Remove; return 1 |
| wrAPP_SYSTEM::InitSubsys 0x004AA6A0 | `RegOpenKeyExA`, `RegQueryValueExA`, `RegCloseKey` (Language) | Read from a config file |
| apGPF_FILTER::SetupFilter 0x004B0630 / GPFExeptFunc 0x004B0650 | `SetUnhandledExceptionFilter`, `GlobalMemoryStatus`, `GetComputerNameA` | Signal handler / platform crash reporter |
| apMsgBox / mbShow* 0x004AF3E0 / 0x004B0160.. | `MessageBoxA` | SDL_ShowSimpleMessageBox |
| apCfg* (engine_core) | `GetPrivateProfileStringA/IntA`, `WritePrivateProfileStringA` | A small INI parser |
| ap_wind.obj dialog helpers 0x004B0220..0x004B05A0, wifOpenFile | `SendDlgItemMessageA`, `GetDlgItem*`, ListView/TabCtrl messages, `GetOpenFileNameA` | Only the in-game editor and debug dialogs use these; they can be stubbed |

**Whole-executable dependency picture** (from `win32_imports.tsv`, the checklist for other subsystems):

- **Imported DLLs:** KERNEL32 (95 imports, mostly the static CRT), USER32 (56), WS2_32 (25, network), GDI32 (8: font rasterising, `CreateCompatibleDC`/`GetPixel`), ole32/OLEAUT32 (COM: DxDiag plus probably DirectShow cutscenes), DDRAW (`DirectDrawCreateEx`/`DirectDrawEnumerateA`: adapter and mode enumeration), DSOUND (ordinals 2 = DirectSoundCreate? and 11), DINPUT (`DirectInputCreateW`), WINMM (`timeGetTime`), ADVAPI32 (registry), SHELL32 (`ShellExecuteA`), comdlg32 (`GetOpenFileNameA`), COMCTL32.
- **Direct3D is not imported statically.** The render driver resolves it at run time (`GetProcAddress` in `d3dEnumDevices`); the render subsystem documents it.
- **Threads and mutexes:** `CreateMutexA/WaitForSingleObject/ReleaseMutex`, 45 sites, belong to the network/Ubi layers, not to this subsystem.
- **CRT:** the CRT (LIBCMT) is linked statically, so a port does not need msvcrt. A mod injected into the original exe must call the game's own `apMalloc/apFree` (v12 0x004AF5F0 / 0x004AF610) for engine-owned memory.

## 6. Verification status and open questions

**Verified in v12 by instruction:**

- Every m3d struct offset. The exact-matched functions are byte-identical, and these were also checked directly:
  - BOX `GetCenter`
  - PRISM `IsBelongPoint` (+0xAC/+0xB0)
  - `m3dVOL::Make` (sizes and vtables)
  - SPHERE radius via `m3dVOL::QualPlane`
- gsWIN_APP_SYSTEM fields (ctor).
- All app vtables, WinMain, InitAll and MainLoop addresses.
- The `-dedicated` and `-safemode` handling.
- The frame clock globals.
- The Alt+Enter bit.

**Open questions:**

- The meaning of the `wrAPP_SYSTEM +0x90` and `gsWIN_APP_SYSTEM +0x88/+0x8C` fields.
- Exact `gsAppState` meanings for 0x00100000 and 0x04000000 (named from feb logic only).
- Parameter meanings of `IsIsectCyl`, `DistPoint` and the `QualPlane` return codes.
- `m3dSPL +0x10` and `+0x28..+0x30`.
- `m3dNOISE +0x04`.
- `voxGRID +0x0C/+0x10`.
- Colour channel order in `m3dCOLOR`.
- v12 addresses of `m3dClampVector_L(4 args)`, `m3dClampVector_A` and `m3dQUAT::Identity` (inlined).
- `apSetMsgBoxCB` and the other three setters: one of v12 0x004AF540..0x004AF570, which one is unconfirmed.
- `wifOpenFile` v12 0x004B0020 and `GPFExeptFunc` v12 0x004B0650 were located by bytes and prologue only.
