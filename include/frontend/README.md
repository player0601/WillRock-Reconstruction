# Frontend (Will Rock 1.2): unofficial SDK notes

This subsystem covers menus, the HUD, message boxes, save/load, the options pages, game rules (`wrGO_SYSTEM`),
the debug keys and the localisation keys. It does not cover the multiplayer menus (`wr_ui_mult/join/server`),
which are in `re/mpsrc`.

| file | contents |
|---|---|
| `ui_core.h` | Engine UI library (`gs:ui_*.obj`): `uiELEMENT`, `uiAREA`, frames, buttons, scrolls, text input, fonts, `uiUI_SYSTEM`, the widget registry |
| `wr_ui.h` | `wrUI_SYSTEM` (full 1.2 vtable), Will Rock widgets, menus, options, save/load, comics, HUD (`wrUIHUD_*`), `wrCURSORS` |
| `wr_game.h` | `wrGO_SYSTEM` (game rules, 1.2 layout), `[GameOpt]` options, DBG cheat keys, localisation key list |
| `element_types.tsv` | All 63 widget type names used in the `ui` template: class, v12/feb factory, size, v12 vtable, 1.2 changes |
| `vtables.txt` | 87 vtables from the frontend objs (UI classes plus neighbours), slot by slot, v12 and feb, aligned by code similarity |
| `layout_check.cpp` | static_asserts of the key sizes/offsets (uiELEMENT 0x6C, wrUI_SYSTEM 0x16C, wrUIHUD_WINDOW 0x104, wrGO_SYSTEM 0x11C); passes `g++ -m32 -fsyntax-only` |
| `function_index.tsv` | 595 frontend functions: feb address, v12 address and how the match was made (factory / vtable-aligned / exact / manual) |

The evidence and scripts are in `re/work/sdk_frontend/`:
- `vtalign.py` aligns vtables by instruction similarity.
- `vtlistalign.py` pairs whole vtable lists.
- `elemreg.py` scans for `uiElementRegister` calls.
- `strrefs.py` lists the string references in the v12 UI code (`strrefs_v12.txt`).
- `fnstarts.py` lists v12 function starts.

## How it works

### Objects
- **`uiSystem`** (v12 0x00994568) is a `wrUI_SYSTEM`, 0x16C bytes. It owns the widget tree, the UI camera, the
  cursor, the widget registry and pointers to the important windows (HUD windows, message box, loading window).
- **Widgets** are `uiELEMENT` subclasses, 0x6C bytes or more. The tree uses `parent` (+0x08), `prev` (+0x0C), `next` (+0x10) and `firstChild` (+0x14).
  Each widget has a name (+0x18, 64 chars) and a screen rectangle (+0x58..+0x67). Flags at +0x04: 0x10 = mouse focus, 0x800 = disabled, 0x2000 = hidden.
- **`wrSysGO`** (v12 0x0064FF5C) is a `wrGO_SYSTEM`, a message-system object. It holds the rules of the current game: mode,
  difficulty, MP limits and level-end handling.
- **`_wrCursors`** (v12 0x0064FF40) holds the menu cursor and crosshair textures.

### Data: the `ui` template
The whole UI is one 3D template file named **`ui`**. `wrUI_SYSTEM::GetTplName` returns `"ui"`. It is loaded by
`uiUI_SYSTEM::Init` (feb 0x00504AC0, v12 0x00519E80) through `gsSysLevel->vt[+0x24]` and instanced into the scene.
- The object **`ui_top`** becomes the root `uiFRAME_GROUP` (`uiSystem+0x12C`). Its size sets the UI coordinate space (+0xEC/+0xF0).
- **`ui_camera`** is the orthographic camera that renders the UI.
- **`ui_cursor`** is the mouse cursor.

The rest of the tree follows the object hierarchy. For every object, `uiELEMENT::Init` reads the property sheet section
`UI_PROP`. The key `type` selects the widget class from the registry (`element_types.tsv`). The other keys set
groups, text ids, fonts, colours, alignment, sounds and animation frames (see the list in `ui_core.h`). If `type` is
missing, the parent's `GetElemDefType()` picks one of the base types: `area`, `window`, `frame` or `frm_grp`.

Fonts: `GetNumFonts()` returns 3. The names come from `[Language] tplFontS / tplFontB / tplFontH` (defaults
`font_s`, `font_b`, `font_h`). They are bitmap fonts built from template objects (`uiFONT::Create`).
Text: every caption is a `gsSTRINGS` key (`textID`). The key list is in `wr_game.h`.
Comics (logo, intro, outro, between missions): `wrUI_COMICS_FRAME::Start(name)` plays a scripted sheet. Its
events are `ShowPic`, `ClearPic`, `ShowText`, `ClearText`, `Delay`, `WaitSound`, `FadeIn` and `FadeOut`. The names come from `[Language] cmsLogo / cmsIntro / cmsOutro`.

### Lifecycle
1. `wrAPP_SYSTEM::InitSubsys` creates `wrUI_SYSTEM` (0x16C) and calls `wrGOInit` (v12 0x004A5BD0), which
   allocates 0x11C bytes and registers `"WRGO_SYSTEM"` with `msgSystem`.
2. `uiSystem->Init()` (v12 slot +0x34, 0x0048BC90) runs `uiUI_SYSTEM::Init`. That calls `RegisterDefaultElements`
   (v12 0x0048B810; it registers the 60+ `wr_*` types and calls `wruiHUDRegister` 0x00494AB0), creates the fonts,
   loads and builds the template, then finds `hud`/`hud_wnd`, the per-mode HUD windows, `hud_messages`, `msgbox_wnd`
   and `loading_wnd`.
3. `Activate(state, frame)` (v12 slot +0x3C) opens a menu. State 1 is the main menu: the first time it plays the
   `cmsLogo` comic, otherwise it shows frame `main`. State 2 is the in-game menu `ig_main`. State 3 is the HUD.
   In states 1 and 2 it switches the input context to `UI` and the mouse to UI mode, and freezes game time.
   On entering, a 128x128 screenshot is grabbed (`pctCreate` + `vidDriver->vt[0x28]`) to use as the save thumbnail.
4. `Deactivate()` (slot +0x40) switches back to the `GAME` input context, restores the time scale and re-applies the camera.
5. `HideLoadingScreen(failed)` either returns to the HUD (state 3) or shows `UI_ERROR` and goes back to `main`.

### Per frame
- `wrUI_SYSTEM::Update` (v12 0x0048C3E0) moves the cursor from `gsSysInput+0x2C/+0x30` and turns the UI key
  (command 0x39) into `OnSysInput` events. In single player it handles QUICKSAVE (0x28) and QUICKLOAD (0x29).
  It shows the score window for the current mode (DM/COOP/TH by `wrSysGO->mode`) while SHOWFRAGS (0x27) is
  held, when the player is dead, or after the level ends.
- The engine's `uiUI_SYSTEM::Update/Render` walk the tree. Each widget runs `ProcessUpdate` (slot +0x44) and
  `ProcessRender` (+0x40). Fades use `StartAppear/ProcessAppear/FinishAppear` (0.2 s alpha ramps).
- Input: `_gsWndProc` sends WM_CHAR and mouse events to `uiSystem->OnSysInput` (slot +0x2C). This only works while
  `gsAppState & 0x1000`. Events go to the capture element, else to the tree; focus is handled with `SetMouseFocus`/`SetKbdFocus`.
  Widgets report clicks and changes to their window through `OnChildNotify` (slot +0x5C). Buttons call `ProcessCommand` (+0x64).
- `wrGO_SYSTEM::ProcessFRAME` (v12 0x004A6440) checks the MP frag/time/treasure limits and sends the game-over
  event. After `gsAppState & 0x80000` (level finished) it shows `GAME_LEVEL_COMPLETE`, waits for USE (0x10), or
  10 s on `-dedicated`, then posts LOAD_LEVEL with the saved next-level request (build_differences.md section 1).
  New in 1.2: `AnnounceCountdown` (0x004A5CE0) prints the time/frags/treasures left and beeps with UI sounds 5 and 6.

### HUD
`hud` frame:
- `hud_wnd` (`wrUIHUD_WINDOW`): health, armor, money, ammo icon bars, boss bar and powerups. Updated in
  ProcessUpdate (v12 0x00497570).
- `hud_messages` (`wrUIHUD_MSG`): centred messages and console lines through `uiSystem->ShowMsgW`.
- `hud_uh_wnd`: the pain flash (`ShowPlayerPain`) and, in 1.2, the ping display and the chat input.
- `hud_sp_wnd`: end-of-level statistics.
- `hud_mp_wnd`, `hud_coop_wnd`, `hud_th_wnd`: score tables.
- `wrh_dbg_msg`: FPS/profiler overlay. It is only active when the 0x8000 "release lock" is off.

### Save / load
Save files are `..\saves\<[Level] pathSAVEGAME>\save%04i.wrs`, each with a screenshot texture (`wr_ui_screenshot`).
The windows are `wr_wnd_save` / `wr_wnd_load`, both based on `wrUI_LS_WINDOW`:
- `RescanFileNames` v12 0x0049A5D0
- `BuildSaveName` 0x0049A530
- `SaveGame` 0x0049B0F0
- `LoadLastSave` 0x0049BAB0

Quick save, quick load and auto save are refused during cinematics (`gsSysCinema+0x80 & 1`) and when
`plrPLAYER::IsCanSaveNow/IsCanLoadNow` returns 0. The save data itself is written by the game_systems save code
(saveload.h). `wrGO_SYSTEM` adds chunks 0xF0 (mode) and 0xF1 (difficulty).

### Level select (1.2 only)
`wr_splayer_wnd` / `wr_splevel_wnd` let the player pick mission, level and sub-level.
- Level table: v12 0x0064FB4C, 0x2A rows of 16 bytes.
- The `[Level] pass` value is compared with a hash of each level name (v12 0x0049F8A0: each character is `^0x55`, `*7`, `^0x77`,
  then mapped to a-z/A-Z/0-9). A match unlocks every level up to that one.

## 1.2 vs feb differences seen in this subsystem
- **`uiUI_SYSTEM` vtable:** a new slot at +0x20 (0x0048F7F0, forwards to a new `wrUIHUD_MSG` virtual). All later
  `uiSystem` slots move +4. This is the most important difference for code ported from feb.
- **`uiTEXT_INPUT`:** two new virtual slots (+0x60/+0x64, SetEditText/GetEditText ?). Every text-input subclass slot after +0x5C moves +8.
- **New widgets:** `wrh_chat_input` (chat, already in 1.0), `wr_splayer_wnd`, `wr_splevel_wnd` (level select, 1.2 only).
  `wrUIHUD_MSG`, `wrUIHUD_UH_WINDOW`, `wrUIHUD_MP_WINDOW`, `wrUIHUD_COOP_WINDOW`, `wrUI_COMICS`, `wrUI_ADVSERV_WINDOW`,
  `wrUI_CTRL_INFO` and `wrUI_TEXT_INPUT` grew (sizes in `element_types.tsv`).
- **`wrGO_SYSTEM`:** 0xB4 -> 0x11C bytes. The feb fields are +4 (base class). New in 1.2: auto-balance teams, level-end guard and
  timer, chat line, countdown announcer. Message ids renumbered (0x406..0x40C -> 0x407..0x40E), new cases 0x3F2 / 0x4CE / 0x4CF.
- **Difficulty damage scale (`GetPlrDmgScale`):** hard is **1.5** in 1.2, 2.0 in feb. Easy 0.5 and normal 1.0 are unchanged.
- **Frag limit:** 1.2 forces -1 (no limit) in every mode except DM. The feb beta applied it in all MP modes.
- **ServerMode:** the ini string for treasure hunt is `TH` in 1.2, `TREASHUNT` in feb.
- **Level end:** feb showed `DEMO_OVER` and returned to the main menu. Retail shows `GAME_LEVEL_COMPLETE` and loads the next level.
- **Game options:** `DisplayPing` (1.2) and `DisplayBlood` (1.0+) are new. `wrUI_GAME_OPT_WINDOW::SaveSettings` is a separate function in 1.2.
- **DBG keys:** ids moved +2 (chat commands inserted). Only 1.2 has handlers (in the player, see `wr_game.h`).
- **Website button:** it only opens `www.saber3d.com` if `[Debug] SwitchWindow` is 1. It minimises the game window first.

## TWEAK table (v12 addresses)
| what | value | v12 address | how to change |
|---|---|---|---|
| Hard difficulty damage scale | 1.5 (`fld [0x005F93A4]`) | 0x004A7B17 | point the `fld` at your own float (the constant is shared, do not patch it in place) |
| Easy difficulty damage scale | 0.5 (`fld [0x005F9254]`) | 0x004A7B1E | same |
| Default MP time limit | 120.0 (minutes) | imm at 0x004A6E7B (`mov [eax],0x42F00000`) | used only when the ini key is missing |
| Default frag limit | 20 | imm at 0x004A6E99 | also init 0x004A5C2C |
| Frag limit only in DM | `cmp [ebp+0x98],1 / mov [esi],-1` | 0x004A6EA4..0x004A6EAD | NOP the `mov` to use the frag limit in COOP and TH too |
| Easy opponent coefficient | 0.6 | push at 0x004A6E5E | `aiSpawnSetOppCoeff` |
| Predictive aiming threshold | difficulty >= 2 | 0x004A6E4B | |
| Countdown announcer starts | 30 / 6 / 4 | 0x004A5C48 / 0x004A5C52 / 0x004A5C5C (wrGOInit) | ? semantics |
| Level-end auto advance on a dedicated server | 10.0 s | in ProcessFRAME near 0x004A6621 | |
| Fade time of UI elements | 0.2 s (`0x3E4CCCCD`) | wrUI_SYSTEM::StartAppear 0x0048C210 | |
| Pain flash time | 0.3 s | push at 0x0048CDD5 | |
| Number of UI fonts | 3 | `mov eax,3` at 0x0048F7C0 | add a font (also patch GetFontName) |
| Save thumbnail size | 128x128 | wrUI_SYSTEM::Activate 0x0048BDDA (`mov eax,0x80`) | |

## HOOK points
| goal | hook | v12 |
|---|---|---|
| Add a new widget type | call `uiElementRegister(name, factory)` after `wrUI_SYSTEM::RegisterDefaultElements` | 0x0051AB00 / 0x0048B810 |
| Open any menu page | `uiSystem->vt[+0x10](frame, 8)` (ShowFrame), or `vt[+0x3C](state, frame)` (Activate) | 0x0048D5B0 / 0x0048BDC0 |
| On-screen message | `uiSystem->vt[+0x24](wtext, seconds, prio)` (ShowMsgW) | 0x0048F800 -> 0x00495060 |
| Message box | `uiSystem->vt[+0x14](caption, text, buttons, def, notifyElem, backFrame)` | 0x0048C940 |
| Custom HUD values | after `wrUIHUD_WINDOW::ProcessUpdate` | 0x00497570 |
| Find a widget | `uiELEMENT::FindChildElement(uiSystem->root, "name")` | 0x00518C70 |
| Game rules on new game | after `wrGO_SYSTEM::ProcessINIT_GAME` (edit `wrSysGO` fields) | 0x004A6CE0 |
| Level end / next level | `wrGO_SYSTEM::ProcessFRAME` level-complete branch | 0x004A6440 (0x004A64D9 text, 0x004A66E1 load) |
| Player damage scale | `GetPlrDmgScale` | 0x004A7B00 |
| Save / quick save | `wrUI_SYSTEM::QuickSave` (slot +0x8C), `wrUI_SAVE_WINDOW::SaveGame` | 0x0048CC70 / 0x0049B0F0 |
| Loading screen | `ShowLoadingScreen` / `HideLoadingScreen` (slots +0x98/+0x9C) | 0x0048CE40 / 0x0048CEB0 |
| Comics / cutscene stills | `wrUI_COMICS_FRAME::Start` | 0x00492480 |
| Options page read/write | `wrUI_GAME_OPT_WINDOW::LoadSettings/SaveSettings` | 0x00494790 / 0x00494950 |
| Cheats from a mod | set `plr flags ^= 4` (god), or `gsAppState ^= 0x8000` (unlock debug features) | see `wr_game.h` |

## PORTING notes
The frontend has almost no direct OS calls. It is written against engine services, and a port must provide those:

| dependency | used by | what a port replaces |
|---|---|---|
| `ShellExecuteA` (SHELL32) and `ShowWindow(SW_MINIMIZE)` (USER32) | `wrUI_BUTTON_WEBSITE::ProcessCommand` v12 0x0048B6CC / 0x0048B6E4 | platform "open URL", or drop it |
| `_apHMainWindow` (HWND) | same | window handle of the port's windowing layer |
| `gswinGetDblClickTime` (engine wrapper around `GetDoubleClickTime`) | `uiUI_SYSTEM::OnSysInput` (feb 0x00505330) | a constant (for example 0.5 s) |
| `_gsWndProc` -> `uiSystem->OnSysInput` (WM_CHAR, mouse) | math_app/app.h | feed SDL text/mouse events as `uiSYSINPUT_INFO` |
| `gsINP_SYSTEM` command states (`IsToggled`, inline `cmds[(id-first)*0x9C] & 1/2`) | Update, wrGO, menus | game_systems/input.h |
| `vidDriver->vt[0x28]` (grab back buffer into a `pctPICTURE`) | Activate / QuickSave thumbnails | read pixels from the port's renderer |
| `scnSCENE::CreateInst`, `camCAMERA`, `txmMANAGER::Add`, `uiFONT::PrintW` | the whole UI draws as 3D quads with the UI camera | engine_render |
| `apCfgRead*/apCfgWrite*` (ini files) | options, rules, fonts, comics | math_app (ini layer) |
| `fioFILE` chunks | save files, comics sheets | engine_core/fileio.h |
| `gsSTRINGS` (UTF-16 strings) | all text | game_systems/strings.h. The UI uses 16-bit `wchar_t` (`swprintf`, `ushort*`), so on Linux use `char16_t`, not 4-byte `wchar_t` |

Other import references inside the v12 UI address range (DirectInputCreateW, DirectDrawCreateEx,
GetDoubleClickTime, RegOpenKeyExA, timeGetTime...) are junk bytes in the padding between functions (for example
0x00494E19 and 0x00498459 sit right before function starts), not calls. Video options (`wrUI_VIDEO_WINDOW`) only
list adapters and modes through the render driver interface, never through DirectX directly.

## Open questions
- `uiINPUT_INFO` / `uiSYSINPUT_INFO` / `uiCHILD_NOTIFY` exact layouts (marked `?`).
- The new `wrUI_SYSTEM` slot +0x20 and the new `wrUIHUD_MSG` slot +0x60: probably chat lines (1.2 chat input), not confirmed.
- The `wrGO_SYSTEM` countdown fields +0x110..+0x118 and the chat fields +0xC0..+0x104 are partly guessed.
- Messages 0x3F2, 0x4CE and 0x4CF in `wrGO_SYSTEM::ProcessMsg` (1.2) are not identified.
- DBG_2 has no handler that I found.
- 68 functions (mostly `_$E` static initialisers and a few HUD updates) have no v12 address in `function_index.tsv`.
  The vtables give the rest.
