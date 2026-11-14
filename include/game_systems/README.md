# game_systems: Will Rock 1.2 unofficial SDK

Covers the `gs:*` objects of the Feb-2003 map except multiplayer (`gs_mp_*`, `gs_play`: see `re/mpsrc/`):
the level system, save/load, sound and music, the render frame driver, cinematics, domains (triggers, kill, teleport,
clean, sound, music, visibility, message, secret), strings/localisation and input action mapping.
The message ids and globals live in `gs_common.h`, and every class has its own header:

| header | classes |
|---|---|
| gs_common.h | globals (gsSys* pointers, gsAppState bits), gsMSG ids, gsLOAD_SCENE_PARAM, domain tag table |
| level.h | gsLVL_SYSTEM, gsLVL_PRELOAD, gsLVL_TEX2MTL(_TABLE), preload flags, "&tag" object flags |
| saveload.h | gslSL, gslHDR_EX, chunk ids, gsl* free functions |
| sound.h | gsSND_SYSTEM, gsCHANNEL_LIST, gsSOUND_DESCR, gsDOMAIN_LIST_SND, gsDOMAIN_LIST_MUS |
| domains.h | gsDOMAIN, gsDOMAIN_LIST, domACT/CLN/KILL/TPT_DOMAIN, gsDOMAIN_SND/MUS, msg/secret/arena pointers |
| vis.h | gsVIS_SYSTEM, gsDOMAIN_VIS, gsVIS_ENABLER |
| render.h | gsRENDER_SYSTEM (wrRENDER_SYSTEM), gsRENDER_EMITTER_VIS |
| cinema.h | cinSYSTEM, cinFRAME, cinACTOR, cinACTION and its 9 cinACN_* actions |
| strings.h | gsSTRINGS (wrSTRINGS) |
| input.h | gsINP_SYSTEM (wrINP_SYSTEM), gsiCONTEXT (+4 contexts), gsiCOMMAND, gsiKEY_SEQ, game command ids |
| vtables.txt | 65 vtables, every slot as v12 address (feb address) and name |
| function_index.tsv | 466 functions and globals: obj, feb, v12, match method, symbol |

Shared core types are assumed from `re/sdk/core`: msgADDR, msgSYSTEM, msgDATA, apNAME, arrVector, psSHEET, objOBJ,
animINST/TPL, entIACTIVE_OBJ, entENTITY, scnSCENE, scnDOMAIN, m3dV/MATR/BOX and fioFILE*.

## How it works

**Systems.** `gsAPP_SYSTEM::InitAll` (v12 0x0050D0A0) creates each system and stores it in a `gsSys*` global
(`gs_common.h`). Most systems are `msgADDR`s registered with `msgSystem`, so they get `GS_MSG_FRAME` (0x3E8),
`GS_MSG_INIT_LEVEL` (0x3F4), `GS_MSG_TERM_LEVEL` (0x3F5) and so on. The `wr*` classes (wrLVL_SYSTEM, wrSND_SYSTEM,
wrRENDER_SYSTEM, wrINP_SYSTEM, wrSTRINGS) are the game-side subclasses that are actually instantiated.

**Frame.** `gsMsgCallFrame` (v12 0x00510B90) sends FRAME to everyone. During that pass the domain lists run `Update`
(point-in-volume → Enter/Leave/Update), cinSYSTEM steps its script, gsSND_SYSTEM moves the listener and
gsINP_SYSTEM polls the current context. Rendering then runs `gsRENDER_SYSTEM::BeginFrame` (only while gsAppState has
0x40000, the level-active bit), then `RenderFrame(camera)`, `EndFrame` and `SwapBuffers`. All drawing goes through the
engine's `rendDrv` and `vidDriver`.

**Level load.** Something sends `GS_MSG_LOAD_LEVEL` (0x3EC) with a `gsLOAD_SCENE_PARAM`. A teleport domain does this
at a level exit, and so do the menu and the MP code. `gsLVL_SYSTEM::ProcessMsg` (v12 0x00537540) copies the payload
to a static at v12 0x009A58E8. A bare name becomes `<PathScn>\name.lg`. `LoadLevel` then calls `__gsLoadLevel`
(v12 0x00536950), which does the following in order:

1. PrepareReloadAnchor.
2. DestroyLevel (TERM_LEVEL; with keepEntities, entities that answer 0x3FC survive).
3. txmManager->LoadAll.
4. Copies the name to gsSceneFullName.
5. Preload: decides which `.lg` files to keep in RAM. It uses `<PathLev>\lev_NN.dsl` `[SCENE_n] name`, plus
   `default.dsl` and `m_play.dsl` template preload lists.
6. Loads the scene and runs LoadInstAll. ObjLoadNotify applies the "&tag" flags (level.h). wrLVL InstLoadNotify
   spawns the entities.
7. SendAll 0x3F6 (the domain lists Collect their objects) and 0x3F4 INIT_LEVEL.
8. gsAppState |= 0x40000, then 0x3F7, and optionally 0x3F1 (level clock reset).
9. ApplyReloadAnchor (0x3FB) moves kept entities by the anchor delta.

**Data files.** These are read through `psSHEET`, the engine's INI-like sheets:
- WILLROCK.INI `[Level] PathDB/PathScn/PathLev, PreloadComm/MP/Scn/ScnUpd/Lev`, `[Sound] Sound, Path`,
  `[Video] VIDEO_2PassRend, Wait_VSync, Wait_Pending, Gamma_Changes, Gamma`, `[Input] HideMouse, SyncState, UseJoy`
  and `fileTexts`.
- `.lg` scene files and `.dsl` level descriptions.
- Per-object property sheets (`objOBJ+0xF0`) with sections `[DOMAIN] height isExactIncl subDomN`,
  `[DOM_ACTION] isActive isActOnce`, `[DOM_KILL] isNoPlr`, `[TELEPORT] nmbLocks anchor sceneName isLevelComplete isDelayed`,
  `[SOUND]/SOUND_CHN`, `[MUSIC] cur/curMusic`, `[DOM_VIS] vis_N` and `[DOM_MSG] msg isShowOnce`.
- The scene sheet `[MUSIC] soft/hard/fight(Music), cur(Music)`.
- `..\texts\strings.str` (UTF-16 `ID text`, `;` comments).
- The input layout sheets (`gsiCONTEXT::GetFileName/GetSectionName`).

**Save/load.** A save game is the multiplayer join snapshot written to disk. `gslSL::Save` (v12 0x0052FFD0) only
runs in single player (`gsSysMP+0x28 == 1`). It writes a chunked file (`'FGSS'` magic, scene name, extended header
with thumbnail, description and play-event strings), then one `gsMP_SERVER::CreateSyncInfo` record per active entity
(non-'$TNE' entities first), then CreateSyncUsrInfo records, then the delayed-message queue. `Load` replays it with
gsAppState 0x20000 set and sends 0x3EF and 0x3F1. So a new entity class is saved correctly if, and only if, its MP
sync is correct (ProcessMP_GET/WRITE/READ_SYNC_DATA).

**Domains.** Level designers put `&dom_xxx` in an object's name. On LEVEL_LOADED every list runs `Collect`, which
calls `IsAddObj` (strstr) and then `MakeDomain`. There are at most **100 domains per list**. Each domain reads its
sheet in `Init` and gets Enter/Leave/Update from its list's per-frame `Update`, using `GetUpdatePoint` (camera or
player). The act, music and vis domains declare themselves MP-syncable, so their state is also saved.

**Cinematics.** cinSYSTEM collects cinDATA_ENTITY instances at INIT_LEVEL. When one is triggered it switches input
to the `"CIN"` context, sends 0x4CE (CINEMA_START), steps `cinFRAME` keys against the controlling animation time and
runs the `cinACN_*` actions (create/destroy entity, animate, constraint, command, fade, sound, music, camera cut).
It sends 0x4CF when it ends.

## 1.2 vs Feb 2003 differences (verified in v12 code)

| what | feb | v12 | evidence |
|---|---|---|---|
| entIACTIVE_OBJ base size → every gsDOMAIN / gsDOMAIN_LIST / domain-list field | 0x94 | 0x98 (+4) | ctors v12 0x00527A20, 0x00527D80 |
| gsDOMAIN size / gsDOMAIN_LIST size | 0x12C / 0x234 | 0x130 / 0x238 | same |
| gsDOMAIN_LIST_SND / _MUS alloc | 0x3C4 / 0x538 | 0x3C8 / 0x53C | v12 0x0051104A, 0x0051109A |
| music list name strings (soft/hard/fight/cur) | +0x3C8/+0x418/+0x468/+0x4B4 | +0x3CC/+0x41C/+0x46C/+0x4B8 | v12 0x005128F0 |
| entIACTIVE_OBJ-derived vtables (domain lists, gsVIS_SYSTEM) | n slots | +1 slot inserted at +0x20 (ProcessMP_UPD_STATE) | vtables.txt |
| gslSL user-sync scratch buffer | 0x400 bytes, memfile at +0x51C | **10000 bytes**, memfile at +0x282C | v12 0x00530F63..0x00530F71 |
| user-sync request message | 0x40D | 0x40F | push at v12 0x00530F95 |
| save ext-header description chunk 0x110 | variable | fixed 0x100-byte write | push 0x100 at v12 0x005307C8 (?) |
| cinSYSTEM size | 0xCDC | 0xCE0: 4-byte field inserted at +0x8C, everything after it +4 | ctor v12 0x005441F0, ProcessFRAME 0x005445EF |
| gsSTRINGS size | 0x34 (has `needSort` flag at +0x30) | 0x30 (flag removed) | ctor v12 0x0051C4A0 |
| gsSND_SYSTEM size | 0xF8 | 0xF4 (tail only; +0x00..+0xEC identical) | gs_common.h |
| gsVIS_SYSTEM size | 0x3D0 | 0x3D4 | gs_common.h |
| message ids >= 0x3FA | 0x3FA,0x3FB,0x3FC,0x40C,0x4C5,0x4CC,0x4CD | 0x3FB,0x3FC,0x3FD,0x40E,0x4C6,0x4CE,0x4CF | gs_common.h |
| unchanged layouts | gsLVL_SYSTEM, gslSL header part, gsSND_SYSTEM head, gsRENDER_SYSTEM, gsINP_SYSTEM, gsiCONTEXT, gsiCOMMAND (0x9C) | same | ctor comparisons |

## TWEAK table (v12)

| constant | value | v12 address | note |
|---|---|---|---|
| max domains per list | 100 | `cmp [esi+0xA0],0x64` at 0x00528332 (imm8 at 0x00528338) | the array is inline (100 ptrs at +0xA4). Raising it needs a bigger list object |
| max sub-domains per domain | 0x13 | gsDOMAIN::Init 0x00527B30 (?) | inline array |
| pooled 3D one-shot sound channels | 30 | `cmp eax,0x1E` at 0x005112E0 | array of 30 at gsSND_SYSTEM+0x34 |
| preloaded scenes per level | 10 | ctor loop `mov ecx,0xA` at 0x0053675A | inline array |
| input contexts | 8 | `cmp eax,8` at 0x0052BE35 etc. | inline array |
| save user-sync buffer | 10000 | `push 0x2710` at 0x00530F71 | inline in gslSL |
| gsLVL priority / sound priority / cinema priority / input priority | 0x0A / 0x57 / 0x50 / 0x0F | 0x005367C1 / 0x00510FE6 / 0x00544212 / ctor 0x0052BB30 | msgADDR +0x1C (message order) |
| kill-domain damage | type 4, 10000.0 | push 0x4C6 at 0x0055CD44 | instant death |
| INI keys | see the data-files list | strings at 0x0063652C (PathDB), 0x00636524 (PathScn), 0x006356D4 (Path) | |

## HOOK points (v12)

| want | hook |
|---|---|
| level about to load / name rewrite | `gsLVL_SYSTEM::ProcessMsg` 0x00537540 (case 0x3EC), or `__gsLoadLevel` 0x00536950 (fastcall, ecx = param) |
| level fully loaded | GS_MSG_LEVEL_STARTED 0x3F7 (register a msgADDR), or the end of `__gsLoadLevel` |
| custom "&tags" on level objects | `gsLVL_SYSTEM::ObjLoadNotify` (slot +0x28, 0x00535680) or `SetExtraObjFlags` 0x00535900 |
| spawn per instance | wrLVL_SYSTEM::InstLoadNotify (slot +0x2C, 0x004ABA50) |
| new domain kind | subclass gsDOMAIN_LIST (IsAddObj/MakeDomain) and new it in a GS_MSG_INIT hook; see domMiscInit 0x0055C130 |
| triggers / teleports | domACT_DOMAIN::Enter 0x0055C2F0, domTPT_DOMAIN::Update 0x0055C920 |
| save/load | gslSave 0x0052FF60, gslLoad 0x0052FF70 (fastcall). Mod data rides in CreateSyncUsrInfo / ProcessMP_*_SYNC_DATA |
| music | gsDOMAIN_LIST_MUS::SetMusicTempo 0x00512BF0 (fastcall, ecx = list, edx = "soft"/"hard"/"fight") |
| sounds | gsSND_SYSTEM::Play 0x00512210, CreateChannel 0x00511B20 |
| frame render | gsRENDER_SYSTEM vtable slots: RenderFrame +0x1C, RenderPostEff +0x34 (wrRENDER vtbl 0x00606198), SetColorFullScr 0x0051C0E0 |
| culling | gsVIS_SYSTEM::IsVisible(objOBJ*) slot +0x98 (0x00538930) |
| cinematics | cinSYSTEM::ProcessFRAME 0x00544450, messages 0x4CE/0x4CF |
| text override / localisation | gsSTRINGS::GetStringById 0x0051D120, AddString 0x0051C550, LoadStringsTable 0x0051D730 |
| key bindings / new actions | gsiCONTEXT::LoadLayout 0x0052B860, wrINP_CTX_GAME::GetCmdName 0x004A8040, gsINP_SYSTEM::ProcessInput 0x0052BFD0 |

## PORTING notes

This subsystem is almost platform-free: it talks to engine abstractions (`rendDrv`, `vidDriver`, `txmManager`,
`pteManager`, `sncCHANNEL*`, `inpINPUT`, `fioFILE*`, `psSHEET`). These are the direct OS imports found by scanning
every v12 function of the index for IAT calls. Results from overlapping junk padding were discarded.

| API | where (v12) | replace with |
|---|---|---|
| WINMM `timeGetTime` | `_gsMsgCalcElapsedTime` 0x005109A0 (gsTime), gsMsgInit 0x005107A0, gsMsgStopLoop 0x00510980 | monotonic ms clock |
| KERNEL32 `QueryPerformanceFrequency/Counter` | `_gsMsgCalcElapsedTime` 0x005109AE.. (frame delta, falls back to timeGetTime) | high-resolution clock |
| KERNEL32 `Sleep` | `_gsMsgCalcElapsedTime` (FPS cap) | nanosleep |
| ole32 `CoInitialize/CoUninitialize` | gsAPP_SYSTEM::InitAll 0x0050D0A5 / TermAll 0x0050D4A0 | nothing (needed only by DirectX/DirectShow) |
| `_apHMainWindow` (HWND) | gsINP_SYSTEM::Init passes it to inpINPUT (DirectInput8 cooperative level) | window handle of the new platform layer |

Indirect dependencies a port must provide: Direct3D 8 behind `vidDriver`/`rendDrv` (SwapBuffers uses Present/Clear,
gamma ramp `vidDriver+0x48`, VSync), DirectSound behind `sncCHANNEL`/`sncCHANNEL_3D` (gsSND_SYSTEM only creates and
positions channels), and DirectInput 8 behind `inpINPUT`. Text is UTF-16 (`unsigned short`, 2 bytes): on Linux keep
`uint16_t`, not `wchar_t`. `AsciiToUnicode` is byte widening, so non-Latin-1 text needs the .str table. File paths are
built with `\` (`fnmMakeFullName`, `strchr(name,'\\')` in ProcessMsg), so a port must normalise separators. Save files
are raw little-endian structs: keep 32-bit layout and packing (`#pragma pack(1)` on chunk headers) to read old saves.

## Open questions

- gsDOMAIN_LIST_MUS `apNAME` at v12 +0x40 against +0x44 in the feb decompile. It may be a feb decompile artefact; check with feb disassembly.
- `entIACTIVE_OBJ` +0x84 (feb) is read as +0x80 in v12 domACT_DOMAIN::Enter. The base was reorganised, not just
  extended; core/entity.h should confirm where the 4 bytes went.
- Meaning of the new cinSYSTEM field at v12 +0x8C, and the exact cinFRAME count.
- gslHDR_EX layout beyond +0x08, and the new fixed 0x100 write in WriteFileHeaderEx.
- The exact numeric ids of game commands >= 0x19 in v12 (read the wrINP_CTX_GAME::GetCmdName jump table at v12 0x004A8040).
- v12 addresses of wrMSG_SYSTEM / wrSECRET_SYSTEM methods (only their vtables are placed).
- PreloadDumpInfo, SetTplObjFlags, FindDomNmb, CreateData/ResetData (cinema) and gsDOMAIN_VIS::ApplyMPEnablerData have
  no v12 match (probably inlined).
