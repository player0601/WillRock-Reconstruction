# engine_core: the engine's non-render side (Will Rock 1.2)

Part of the unofficial Will Rock modding SDK and porting map. It was built by static analysis of
`re/bin/v12.exe` (1.2, 2003-07-30), using the Feb 2003 build (`feb.exe` + `WillRock.map` + Ghidra decompile) as
the reference. Nothing was executed.

| file | contents |
|---|---|
| `core_types.h` | **Authoritative** layouts: `apNAME`, `arrVector<T>`, `lstNODE/lstLIST`, `msgADDR`, `msgMSG`, `msgDATA`, `msgDATA_PACK`, `msgSYSTEM`. Also the ap runtime (memory, log callbacks, ini config) and the frame clock (`gsElapsedTime` & co.) |
| `object.h` | **Authoritative** `objOBJ` (mesh/hierarchy node, 0xF8), `objGEOM` (partial), object search helpers |
| `entity.h` | **Authoritative** `entIACTIVE_OBJ` and `entENTITY` (with the 1.2 layout shift), both vtables slot by slot, `entMP_DEADRECKON`, IAction data, entity class registry (`entRegisterClass`/`entCreate`) |
| `anim.h` | `animINST` (0x14C), `animTPL` (0xCC), `animSEQ`, the animation and bounding-volume API, scene pools |
| `collision.h` | `cdtINFO`, `cdtREFINE` (query filter), scene query API (ShootRay/FloorPlace/...), cdtCOLL_* classes |
| `physics.h` | `physINIT`, `physMATH` (point-mass integrator), `physPHYS` entity, rigid-body (`rgdRIGID*`) index |
| `fileio.h` | `fioFILE` / `fioFILE_DISK` / `fioFILE_MEM`, `fioCHUNK` format, file-name search (`fnm*`), scene/template readers (`sio*`, chunk ids, `.tpl` layout) |
| `function_index.tsv` | every function of the covered objs: feb address, v12 twin, match quality (exact = byte-identical modulo relocations; callgraph/vtable = **unverified**), name |

Address convention in the headers: `// v12 0x00XXXXXX (feb 0x00XXXXXX)` for code and data, and
`// v12 +0xNN (feb +0xNN)` for fields. `// ?` marks anything not confirmed by at least one v12 instruction.

## Objs covered

- **engine:** Object, Obj_alloc, Obj_misc, obj_access, obj_iter, obj_usr_data, Obj_anim, anim, anim_anim,
  anim_ctrl, anim_chn, anim_clean, animkinem, Animbvol, abv_encl, cdt_sys, cdt_scn, cdt_obj, cdt_sys_anim,
  Cdtm_sys, cdto_math, cdt2_sgen, scn_coll, msgaddr, fileio, fname, llist, data_struct, sceneio,
  sceneio_misc, sceneio_scn, sceneio_obj, sceneio_tpl.
- **gs:** entity, ent_interact, ent_mp (base-class MP hooks only), physics, phys_rigid, phys_rigid_ball,
  phys_rigid_cdt, phys_rigid_explos, gs_msg (frame clock and message pump).
- **ap:** ap_mem, ap_comm (log), ap_cfg (ini), ap_gpf (crash filter, listed only).

The following belong to other subsystems: Scene.obj internals and every render obj (`engine_render`),
psSHEET/pssheet (property sheets, which only appear here as the `props` pointer), and the MP protocol
(`re/mpsrc`, which already documents ent_mp and the message ids).

## How it works

### Main objects
- **msgSYSTEM `msgSystem`** (v12 0x008FA960) is the heart of the engine. Every live thing is a `msgADDR`
  that sits in one global list, sorted by `priority`. Messages are integers and carry an optional
  `msgDATA*` payload. They are either sent synchronously (`SendMsg`, `SendAll` = broadcast), posted for the
  next frame (`PostMsg*`, max 500 per frame, no overflow check), or posted with a delay (`PostTimeMsg*`).
  A receiver handles a message in three virtual steps: `PreProcessMsg` → `ProcessMsg` → `PostProcessMsg`.
  The first step that returns non-zero stops the chain.
- **entIACTIVE_OBJ / entENTITY**: game objects. `entENTITY` binds a `msgADDR` to an **animINST** (a placed
  instance of an **animTPL** template, which holds its own **objOBJ** tree). Entity classes are created by
  name through the class registry.
- **objOBJ**: a hierarchy node with name, two matrices (local and world), a bounding box and shared
  geometry (`objGEOM`).
- **cdt**: voxel-grid collision for the static level (`cdtCOLL_SCN`) and per instance (`cdtCOLL_ANIM`). It is
  queried through `scnSCENE::ShootRay/FloorPlace/GetClosestPoint/ConstrSphere` with a `cdtREFINE` filter.
- **physMATH / physPHYS**: a simple Euler integrator with lifetime and floor lighting. `rgdRIGID` provides
  full rigid bodies for debris.
- **fio / fnm / sio**: loose files read with the CRT, a chunked binary format and case-insensitive lookup.

### Object lifecycle (entities)
1. **Registration** (game start): `xxxInit()` calls
   `entRegisterClass(name, FourCC, priority, Create, defaultTpl, exact)` (v12 0x00507D60). The entry lands in
   `entClassTable` (v12 0x00984E70, stride 0x90).
2. **Creation**: `entCreate(scene, className, tplName, createData, initData, flags)` (v12 0x005082B0):
   - The template comes from `gsSysLevel` vslot +0x24, which loads `<level>/<tpl>.tpl` through
     `gsLVL_SYSTEM::LoadTpl` (v12 0x00537720).
   - `scnSCENE::CreateInst` takes a slot from the pool of 1000 and duplicates the object tree.
   - `entCreate(className, inst, initData, flags)` calls the class's `Create(animINST*)` (operator new +
     ctor).
   - `entRegister` (v12 0x00507FF0) links `inst->ent` and `ent->inst`, then calls
     `msgSystem.InsertAddr(ent, classId, 0)`. It sets up the animation system, bounding volumes, LOD and
     collision.
   - Finally it calls `SendMsg(ent, MSG_MSG_INIT=1, initData)`, plus `INIT_GAME`/`INIT_LEVEL`/`INIT_MAP`
     when the game is already running. In MP it also announces the entity to the clients.
   - An entity that does not handle INIT (`*ret == 0`) destroys itself ("Automatic destroy of entity").
3. **Per frame**: the entity receives `GS_MSG_FRAME` (1000) through `SendAll`. `entENTITY::PostProcessMsg`
   then:
   - runs ON_ACTION_FRAME IActions;
   - estimates velocity from the position delta (unless `ENTF_REMOTE`);
   - updates the 3D sound position;
   - does the MP frame update;
   - does the colour blend.
4. **Destruction**:
   - `PostMsg(ent, MSG_MSG_TERM=2)` → on TERM, `PostProcessMsg` frees the extra buffer, ends the follow and
     calls `msgSystem.DestroyAddr(ent)`. That broadcasts `MSG_MSG_DESTROY_ADDR`=3 to everyone with flag 2,
     marks the entity DEAD and destroys its animINST.
   - The C++ object is deleted at the end of the `SendAll(FRAME)` purge.
   - **Never `delete` an entity yourself.**

### Per-frame flow (one game tick)
`gsWIN_APP_SYSTEM::MainLoop` (v12 0x004A4C60, Win32 message pump with `WaitMessage`/PeekMessage) calls
`gsMsgCallFrame` (v12 0x00510B90) once per frame:
1. `_gsMsgCalcElapsedTime` (v12 0x005109A0):
   - Measures dt with QueryPerformanceCounter, falling back to timeGetTime.
   - If dt < 0.005 s it calls `Sleep()`, which caps the game at about 200 fps.
   - Multiplies dt by `gsTimeScale`, clamps it to 0.1 s, and applies the overrides `gsFixedElapsed` and
     (1.2 only) `gsNextElapsedOnce`.
2. Measures fps (every >1000 ms) → `gsMP_NETWORK::UpdStat` and `vidDRIVER::SetFPS`.
3. `_gsMsgProcessFrameProfile` (v12 0x00510CB0):
   - `msgSystem.DispatchAll(dt)` handles the delayed messages, then everything posted last frame (sorted by
     priority).
   - `msgSystem.SendAll(GS_MSG_FRAME)` gives every address its frame update in list order, then purges
     dead addresses.
   - `msgSystem.NextFrame()` swaps the queues and does `frameNmb++`.
4. `gsElapsedTimeLevel += gsElapsedTime`.

Rendering is driven by the render system on `GS_MSG_RENDER` (0x3E9) and by the app loop; see
`engine_render`.

### Data files and formats read here
- **`.tpl`** (template, one per model). A chunk stream with chunks 0x2E4/0x2E5: the header is `'APTL'` plus
  the template name and class name (64 bytes each). Then come textures, the object tree, skins, object
  animations, sequences, bounding volumes, the property-sheet script, named selections and the end chunk.
  Loaded by `sioReadTemplate` (v12 0x0059E900).
- **Scene / level file**. Also a chunk stream (`sioReadScene` v12 0x0059E680) with these chunks:
  - 0xF0 objects;
  - 0x154/0x155 textures;
  - 0x1B8 instances (`'SNIA'` + name/tpl/class + matrix + script);
  - 0x1EA templates;
  - 0x21C/0x21F collision grid;
  - 0x21D selections;
  - 0x21E global script;
  - 0x280 lights;
  - 0x3AC camera.
- **Chunk format** (`fioCHUNK`): `u16 id; u32 size;` followed by the body. Id 1 ends a list. Data is
  little-endian. The `FIO_SWAP` flag can byte-swap per element, which is useful for a big-endian port.
- **INI files** through `apCfg*` (Win32 private-profile API).

## 1.2 vs Feb differences found in this subsystem

1. **Entity layout.** The `entIACTIVE_OBJ` MP sequence fields shrank to u16 (+0x3C/+0x3E). Everything after
   them moves −4 (`name` 0x44→0x40, `entFlags` 0x88→0x84, `mpId` 0x8C→0x88), and 8 bytes were appended
   (0x90 = 1/30 s, 0x94). `entENTITY` gains 4+0xC+0x10 bytes and is 0x16B instead of 0x14F. The full mapping
   is in `entity.h`. **Every subclass field moves +0x1C** (e.g. `physPHYS::math` 0x14F→0x16B).
2. **Entity vtables.**
   - `entIACTIVE_OBJ` inserted slot 8 `ProcessMP_UPD_STATE`, so its slots 8.. are +1.
   - `entENTITY` removed `GetMP_UPD_POS`/`UpdateMP_UPD_POS`/`ProcessMP_UPD_POS` and added an unknown 1-arg
     slot 23.
   - The net effect is that subclass virtuals start at v12 slot 25 instead of feb slot 26 (−1), while base
     slots 9..15 are +1. Hooks by slot index from feb-era mods (WRML) must be remapped.
   - `results/player.json` (VS_PLR_FRAME 34→33) is consistent with this.
3. **Message ids** above 0x3F9 were renumbered (+1, then +2 from feb 0x40B). The physPHYS MP_NOTIFY is now
   0x40E and a new id 0x3FA resets the dead-reckoning vectors. See `re/mpsrc/mp_messages.h`.
4. **Frame clock.** 1.2 adds a one-shot dt override (`gsNextElapsedOnce` v12 0x006354EC, setter v12
   0x00510D30), which is used after loads.
5. **Last position.** `entENTITY::lastPos` and a new second vector are initialised to `m3dVUndef` (v12
   0x009AEA90) instead of being left uninitialised. This avoids a velocity spike on the first frame.
6. Unchanged: `objOBJ`, `animINST`, `animTPL`, `msgSYSTEM`, `fioFILE*`, `physMATH`, `cdt*`, pools and limits.
   Their code matches feb almost byte for byte; only the addresses move.

## TWEAK table (v12)

| constant / limit | value | where (v12) | notes |
|---|---|---|---|
| frame dt clamp | 0.1 s | compare 0x00510AF5 (`fcomp [0x005F91BC]`), store imm at 0x00510B04 (`mov [gsElapsedTime],0x3DCCCCCD`) | 0x005F91BC is a shared constant; patch the two sites, not the constant |
| min frame time / fps cap | 0.005 s | compares 0x00510A2F / 0x00510A3C vs `[0x005FB650]`; `Sleep` call 0x00510A54 | 0x005FB650 is used by 10 sites; NOP the Sleep to uncap |
| global time scale | 1.0 | data 0x006354E8 (`gsTimeScale`); setter 0x00510CF0 | slow motion / fast forward |
| forced dt every frame | off | data 0x0098EA04 (`gsFixedElapsed`) | any value with abs >= epsilon becomes dt |
| one-shot dt | -1 | data 0x006354EC | 1.2 only |
| message queue size | 500 per frame | `msgSYSTEM::Init` 0x004CFF50: `push 0x2710` at 0x004CFF55 and 0x004CFF9B (= 500·0x14), `mov edx,0x1F4` 0x004CFF6F, `mov edi,0x1F4` 0x004CFFB1, `mov [esi+0x14],0x1F4` 0x004CFFDA | PostMsg has no bound check |
| msgDATA pack types | 100 | `RegisterMsgData` 0x004D0A91 (`cmp eax,0x64`) | array inside msgSYSTEM (+0x69C), cannot grow in place |
| registered message names | 100 | msgSYSTEM+0x370 | no bound check |
| scene instances (animINST) | 1000 × 0x14C | `scnSCENE::ctor` 0x004E5E4D (`push 0x3E8`), 0x004E5E92 (`cmp eax,0x510E0`), `AllocInst` 0x004E6526, `CreateInst` 0x004E6671, `_FindInstEmpty` ~0x004E64A0 | all sites must agree |
| templates (animTPL) | 300 × 0xCC | ctor 0x004E5E23 (`push 0x12C`), 0x004E5EAD, `AllocTpl` 0x004E6426 | |
| entity classes | unbounded (no check) | `entNClass` 0x00984E68, table 0x00984E70 | real capacity of the static table unknown (?) |
| object user-data kinds | 5 | `objOBJ::RegisterUsrData` 0x004FFB30 | |
| physMATH default lifetime | 2,000,000 s | const 0x006079A0 (read by physPHYS::Create 0x00508AA0) | |
| physMATH rest frames | > 3 | `physMATH::ProcessPhysics` 0x005095A0 | |
| FloorPlace max probe | 10000 | `scnSCENE::FloorPlace` 0x004DF2B0 | |
| player gravity / speeds | see `results/player.json` | | not in this subsystem |

## HOOK points (v12)

| what | where | how |
|---|---|---|
| Capture all engine logs | callback pointers `apMsgCB` 0x00655160, `apLogCB` 0x00652060, `apLogErrCB` 0x00654158, `apMsgBoxCB` 0x00655164 | write a `void __cdecl f(char*,...)` into the pointer (or call the setters 0x004AF540..0x004AF570) |
| Every message to every object | `msgSYSTEM::Dispatch` 0x004D0340 (`__thiscall(addr,id,data,from,int*ret)`) | detour: see/modify/suppress any message |
| Synchronous sends / broadcasts | `SendMsg` 0x004D03C0 (prologue `51 8B 44 24 08`, steal 5), `SendAll` 0x004D06F0 | `results/engine.json` DT_MSG_SEND |
| Per-frame tick | `gsMsgCallFrame` 0x00510B90, or `_gsMsgProcessFrameProfile` 0x00510CB0 | run mod logic once per frame with a valid dt |
| New entity types / override existing | `entRegisterClass` 0x00507D60 | registering an existing name **replaces** its factory: subclass `entENTITY` with v12 offsets and chain to the original Create |
| Entity creation | `entCreate` 0x005082B0 / `entRegister` 0x00507FF0 | spawn anything by class + template name |
| Iterate entities | `entFindNext(0..)` 0x005086A0, `entFindNextIO` 0x0050C340, `msgSystem.GetFirstAddr` 0x004D09D0 + `msgADDR::GetNext` 0x005089D0 | filter by `sign`/`classId` |
| Per-entity behaviour | vtable slots (entity.h). A class's vtable can be patched per class | e.g. `entENTITY` PostProcessMsg slot +0x0C 0x005068A0 |
| Template / asset loading | `gsLVL_SYSTEM::LoadTpl` 0x00537720, `sioReadTemplate` 0x0059E900, `fioFILE_DISK::Init` 0x004E7E20 (results DT_FIO_DISK_INIT), `fioFILE_MEM::Init` 0x004E8150, `fioIsExist` 0x004E7690 | redirect paths for a mod folder / virtual file system |
| File name resolution | `fnmMakeFullName` 0x004FEE40, `fnmFindDisc` 0x004FF0B0 | mod-folder overrides (all lookups return `fnmBuffer` 0x00984BF8) |
| Collision queries | `scnSCENE::ShootRay` 0x004DE9F0, `FloorPlace` 0x004DF2B0, `ConstrSphere` 0x004DB000 | custom filters: derive from `cdtREFINE` (vtable 0x005FA0E0) |
| Physics | physMATH vtable 0x005FEFD4 slot 7 `CalcNewPos` 0x00509660 (Euler step), slot 6 `ProcessPhysics` 0x005095A0 | gravity / drag / bounce mods |
| Time | `gsMsgSetElapsedTimeScale` 0x00510CF0, `gsTimeScale` 0x006354E8 | bullet-time |
| Memory | `apMalloc/apCalloc/apFree/apRealloc` 0x004AF5F0..0x004AF620 (jmp to CRT) | leak tracking / replacing the allocator: patch the 4 jmps |

## PORTING notes (what a new platform must replace)

The engine core is unusually portable. Only five places touch the OS, all thin:

| area | API used | where (v12, feb) | replace with |
|---|---|---|---|
| frame timer | `timeGetTime`, `QueryPerformanceFrequency/Counter`, `Sleep` | `_gsMsgCalcElapsedTime` 0x005109A0 (feb 0x004FC610), `gsMsgInit` 0x005107A0 | `clock_gettime(CLOCK_MONOTONIC)` / SDL_GetPerformanceCounter, `nanosleep` |
| main loop | `WaitMessage`, PeekMessage/DispatchMessage | `gsWIN_APP_SYSTEM::MainLoop` 0x004A4C60 | platform event pump (SDL_PollEvent); only needs to call `gsMsgCallFrame` |
| files | CRT `fopen/fread/fwrite/fseek/fclose`, `stat`, `access` | fioFILE_DISK 0x004E7E20.., fioFILE_MEM::Init 0x004E8150, fioIsExist 0x004E7690 | POSIX equivalents (same API) |
| directory listing / case | `_findfirst/_findnext`, `access`, `stricmp`, `'\\'` separator | `_fnmFindDisc` 0x004FF190, fnmDIR::RebuildContent 0x004FF7E0, `fnmPathSep` 0x00634B70 | `opendir/readdir`. **Lookups must stay case-insensitive** (assets are referenced with arbitrary case), so emulate it with the fnmDIR cache; map `\\` to `/` |
| exe directory | `GetModuleFileNameA`, `__pgmptr` | `fnmInit` 0x004FEC40 | `/proc/self/exe`, `SDL_GetBasePath` |
| config | `GetPrivateProfileStringA`, `WritePrivateProfileStringA`, `GetCurrentDirectoryA` | `apCfg*` 0x004AF670..0x004AFC20 | a small INI parser |
| crash log | `SetUnhandledExceptionFilter`, `GlobalMemoryStatus`, `GetComputerNameA` | `apGPF_FILTER` (feb 0x004A1A80) | drop or signal handler |
| memory | CRT `malloc/calloc/realloc/free` (statically linked) | ap_mem | libc |
| UI helpers | Win32 common dialogs / ListView (editor leftovers) | `ap_wind` (feb 0x004A1490..) | not used by the game at run time (?) |

Other porting facts:
- **Struct packing**: `entENTITY` and its subclasses are compiled with `#pragma pack(1)`, so their fields sit
  at odd offsets. The `.tpl`/`.scn` structures are read with raw `fread` into memory layouts (e.g.
  `fread(animINST, 4)` for the magic, `fread(matr, 0x40)`), so the loaders need explicit field-by-field
  reading on non-x86 or 64-bit targets. Pointers inside objects are 32-bit; a 64-bit port must change
  every layout in these headers.
- **Endianness**: all files are little-endian, and `fioFILE` already has a per-element byte-swap path
  (`FIO_SWAP`, ReadData elemSize). It is not used by the retail loaders, but it shows where to hook it.
- **Float / SIMD**: `m3dSqrt` dispatches on `m3dSimdType` (SSE rsqrtss / 3DNow! / x87). A port should use
  plain `sqrtf`. 3DNow! paths appear in physPHYS::ProcessINIT.
- **Sorting determinism**: `msgSYSTEM::NextFrame` uses CRT `qsort` (unstable) with `_msgQSortCompare` (v12
  0x004CFE70). A port must reproduce MSVC qsort ordering for equal keys, or MP/demo playback may diverge.
- **Calling conventions**: free functions are mostly `__fastcall` (`YI` in the mangled names), methods are
  `__thiscall`, and varargs are `__cdecl`. A reimplementation only matters at the boundaries with
  unported code.

## Corrections to the automatic match table (re/tables/v12.functions.tsv)

These were checked by disassembly. The tsv in this folder still shows the raw automatic values.

| feb function | table says | real v12 |
|---|---|---|
| entENTITY::ParsePS 0x004F45C0 | 0x0041DE70 | **0x00506C00** (vslot 24) |
| entENTITY::FloorPlace 0x004F4B90 | 0x00425E30 | **0x00507200** (vslot 18) |
| entENTITY::AssignANIM_ST_ANIMATE_HIDDEN 0x004F56C0 | 0x00506C00 | **0x00507D30** (vslot 22) |
| entENTITY::GetCenter 0x004F5690 | – | **0x00507D00** (vslot 21) |
| entENTITY::GetAccel 0x004036C0 | – | **0x00403A30** (vslot 19) |
| entENTITY::GetDeadReckonData 0x004F80E0 | 0x00403A30 | **0x0050AA30** (vslot 20) |
| entENTITY::ParseIActions 0x004F4780 | 0x0050A7C0 | **0x00506DC0** (vslot 9). 0x0050A7C0 is the new ProcessMP_UPD_STATE |
| entIACTIVE_OBJ::ParseIActions 0x004F8BC0 | – | **0x0050B680** |
| entIACTIVE_OBJ::AddIAction 0x004F9080 | 0x00506DC0 | **0x0050BB40** |
| entIACTIVE_OBJ::ExecuteIAction 0x004F8690 | 0x0050B090 | **0x0050B150** |
| entENTITY::GetMP_UPD_POS / UpdateMP_UPD_POS / ProcessMP_UPD_POS | 0x0050AA30 / 0x00507D00 / 0x00507D30 | **removed in 1.2** |
| msgSYSTEM::Init 0x004BE4C0 | – | **0x004CFF50** |
| gsMsgInit 0x004FC420 | – | **0x005107A0** |
| physPHYS::Create 0x004F63F0 | – | **0x00508A10** |
| physPHYS ProcessMsg/INIT/TERM/FRAME/DESTROY_ENT/CDT/MP_NOTIFY | – | 0x00508C70 / 0x00508D30 / 0x00508E90 / 0x00508F50 / 0x00509170 / 0x00509190 / 0x00509480 |
| physMATH::ProcessINIT 0x004F6E10 | – | **0x005094B0** |
| apMsgBox / apSet*CB | – | 0x004AF3E0 / 0x004AF540, 0x004AF550, 0x004AF570 |
| cdtREFINE ctor 0x0040B620 | – | **0x0040C520** |

## Open questions
- `msgSYSTEM` +0x1C..+0x350: the exact size of the embedded msgADDR_LIST is unknown. Only +0x1C (vtable)
  and +0x20 (head) are used directly.
- The meaning of entENTITY 1.2 additions: `+0xE4` (second undef vector), `+0x155` (0.1, 2.0, 0.2) and the new
  vslot 23 (1 arg). They are probably dead-reckoning smoothing parameters.
- The meaning of entIACTIVE_OBJ 1.2 `+0x90` (1/30 s) and `+0x94`. They are probably the MP update period
  and timer; check against `gsMP_*` update code in re/mpsrc.
- `entCLASS_INFO::priority` direction (lower = earlier?) and the static capacity of `entClassTable`.
- The bit meanings of `objOBJ::state/rendState/procState` and `animINST::flags` beyond those listed.
- The v12 addresses of SetFollow/TermFollow/UpdateFollow, entDestroy, apSetLogCB, fioIsReadOnly and
  rgdRIGID internals were not located.

## Layout self-check
`re/work/sdk_engine_core/layout_check.cpp` static-asserts the sizes and key offsets of every header against
the values verified in v12. Run it with `g++ -m32 -std=c++11 -fsyntax-only -nostdinc++ -Wno-invalid-offsetof layout_check.cpp`. The
headers must be compiled 32-bit; a 64-bit build changes pointer sizes, so the layouts no longer match the
game.
