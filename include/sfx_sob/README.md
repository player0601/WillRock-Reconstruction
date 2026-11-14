# sfx_sob: special effects (wrsfx*), scene objects (wrsob*), particles and decals

Unofficial modding SDK for Will Rock (Saber Interactive, 2003). Built by static analysis only.
Target build is **1.2** (`re/bin/v12.exe`). Every address also gives the Feb-2003 symbol build (`feb`) twin.

## Files

| file | contents |
|---|---|
| `particles.h` | `partEMITTER_PHYS` (the particle emitter every effect uses) with verified v12 field offsets, its 12-slot vtable, the `part*` free functions, `_partEmitList`, and the `pteSPOT` / `pteSPOT_ANIM` decal API |
| `sfx_api.h` | Spawn entry points: `entCreate`, `wrsfxStartBlood`, `wrsfxCreateBloodAir`, `wrsfxStartChunkMeat`, `wrsfxSCORCHMARK::Add`, `wrsfxAPPEAR::Start`, the melt helpers, the Init/Term chain, and `wrAppState` |
| `sfx_entities.h` | 60 effect entity classes (wrsfx*, plus pwpWSFX/aisSTATUE/birdEGG/weapon-hit sfx). Each has its class-name string, entRegisterClass sig/prio, Create, vtable, size in both builds, own fields with both offsets, overridden slots and methods |
| `sob_entities.h` | 10 scene-object classes (wrsob* and nchRIGID) in the same layout |
| `function_index.tsv` | 584 functions from 48 objs: feb addr, v12 addr, how it was matched, obj, mangled name |

Generator and raw data are in `re/work/sdk_sfx_sob/`. `gen.py` rebuilds the two entity headers. Other files there: `create.json` (sizes/vtables), `fieldcheck.json`, `tweaks.tsv`, `engine_funcs.tsv` and `classtable.md`.

How the matches were made: `exact` comes from the shared tables. `callprop` pairs functions by call order, starting from verified Create and vtable seeds. `gap` is the only candidate left between two matched neighbours. `manual` and `vtable(checked)` were confirmed by prologue, callers or a vtable slot. I rejected 5 table matches that pointed outside the subsystem's address range. Three of them were fixed through the vtables:
- wrsfxLAVA::ProcessMsg is v12 0x004792C0, not 0x00507200.
- wrsfxLAVA_EXPL::ProcessMsg is v12 0x00478CB0.
- the wrsfxLAVA_EXPL scalar dtor is v12 0x0047A010.

## How it works

### Three layers
1. **Particles (engine).** A `partEMITTER_PHYS` is a plain C++ object, not an entity. It owns a pool of `partPART` records (0x3C bytes each). Its constructor registers it in the global `_partEmitList[600]`. Once per frame, `partUpdate(cam, dt)` (v12 0x004D41C0) walks that list and calls `Update` (vtable slot +0x0C) on each emitter. `Update` handles emission, aging, CDT (particle collision against up to 4 planes), and parent/child attachment. Then `partRender` (v12 0x004D42F0) qsorts the visible particles by Z and builds camera-facing quads through `rendCFG`. An emitter deletes itself when state bit 0x20 is set. That happens automatically when flag 0x200 is set, it has stopped emitting and it has no live particles left. Subclasses customise behaviour by overriding `PartCreate` (+0x1C), `PartUpdate` (+0x20) and `PartApplyCDT` (+0x2C). About 150 tiny emitter subclasses exist (for example `wrsfxBARREL_EXPLODE_FIRE`). Their ctor only calls the base ctor and then a run of `Set*` calls (texture, size, colour, lifetime, rate).
2. **Effect entities (wrsfx\*).** Each visible effect is an `entENTITY` (or a `dynDYNAMIC`/`physPHYS`/`dstrDESTROY` subclass). It is registered by name with `entRegisterClass(name, sig, prio, Create, ...)` inside `wrsfx*Init`. Its `ProcessMsg` creates the emitters on INIT and holds them in its own fields; those are the `partEMITTER_PHYS*` members in the headers. On FRAME it moves or fades them. When its timer runs out it stops emission and deletes itself. Many effects also start a 3D sound through `sncCHANNEL`. The sound tables are linked with `sncLinkSndListData(id, table)`.
3. **Scene objects (wrsob\*).** These are level-placed interactive props: the rotating blade trap (`sob_blade`), the geyser with its idle and burst smoke emitters (`sob_geyser`), the catapult (`sob_katapult`; whether it is a trap or a launcher is not checked), breakable wall niches (`sob_niche`, which use `nchRIGID` shatter physics), the spike trap (`sob_spike`), the trading altar with its button and camera (`sob_tra_altar/_button/_camera`) and the tree with animated leaves (`sob_tree`). They react to messages (trigger, damage, MP sync) and spawn wrsfx effects. Doors, lifts and movers are **not** in sob. They are generic engine `dyn*`/`dynDYNAMIC` objects driven by level scripts and belong to the dynamics/engine subsystem. Breakables are `dstrDESTROY` (engine) plus the `wrsfxDESTROY_*` subclasses here (vase, tripod, mirror, barrel). Each of those overrides `ProcessCDT_FIREABLE` or provides a `dmgDISPATCHER`.

### Spawning
- **From code:** `entCreate(gsScenePtr, "sfx_xxx", templateName, &animCREATE_DATA, initData, 0)` (v12 0x005082B0). The class-name string is the key. `animCREATE_DATA` carries the transform, built with `m3dMATR::MakeLCS2WCS_VY(pos, up)`. `initData` is class specific; `sfx_appear`, for example, gets a `cdtINFO*` and a flag. Some classes have static starters that do this for you (see `sfx_api.h`).
- **From templates/levels:** the same class names can be placed in a level's entity data. The template name (2nd string) chooses a parameter block. `sfx_*` and `sob_*` names appear in level files exactly as in the class table below.
- **Blood:** `wrsfxStartBlood` → `wrsfxCreateBloodAir` creates flying-droplet emitters (`wrsfxBLOOD_AIR`). When a droplet hits geometry, `wrsfxBLOOD_AIR::PartApplyCDT` (v12 0x0046C4D0) leaves a `wrsfxBLOOD_AIR_SURF`/`wrsfxBLOOD_SURF` decal (`pteSPOT_ANIM`).
- **Decals:** `wrsfxSCORCHMARK::Add` and the blood surfaces are `pteSPOT`s. Before adding one, every spawner calls `pteMANAGER::FindShaderAround(type, &sig, pos, radius)` so that decals do not pile up.

### Lifecycle and per-frame flow
`wrsfxInit` / `wrsobInit` / `partInit` / `pteSpotInit` / `peffInit` run at startup: textures are loaded, classes registered and sounds linked. At level start, entities get INIT and INIT_LEVEL. Then, every frame:
1. Entity FRAME messages run, and each effect updates its emitters' position, CS and intensity.
2. `partUpdate` runs.
3. The renderer calls `partRender` and the `pteSPOT::PrepareFrame/ShadeFace` decal path.

At level end, `partDestroyEmitters` (v12 0x004D42A0) and `wrsfxTerm`/`wrsobTerm` run.

### Data files read
- **Textures:** about 97 `txmMANAGER::Add(name, 0x40003, 1)` calls with names such as `particle`, `sfx_cloud`, `sfx_appear_part`, `sfx_appear_hole`, `sfx_blood_floor_1`, `sfx_blood_anim_8x4`, `sfx_bsk_scorch_mark`, `sfx_acd_drops`, `sfx_mcg_part`, `sfx_ripple`, ... You can replace any of these texture files without touching code.
- **Config:** `GameOpt/DisplayBlood` (in wrsfxBloodInit) and `Video/VIDEO_LowScorchQuality` (in SCORCHMARK::Add).
- **Sounds:** static `sncSOUND_DESCR_3D` tables in .data, linked by id.
- No particle-definition files: every emitter parameter is hard-coded in its ctor. That makes those ctors the place to tweak.

## 1.2 vs Feb differences
- **Entity base grew.** `entENTITY` is 0x14F bytes in feb and 0x16B in v12 (+0x1C). `dynDYNAMIC` grew by +0x24 and `physPHYS`/`dstrDESTROY` by +0x1C. All wrsfx/wrsob own fields therefore move by a constant, and in the checked code no class changed its own layout. I checked this per class against the v12 code (`fieldcheck.json`): 63 classes. 5 of them have v12 accesses that the constant shift does not explain: wrsfxALTAR_FIREBALL, wrsfxDISCUS_LAND, wrsfxLAVA_EXPL, aisSTATUE_BREAK_PART_SFX and wrsobTRA_BUTTON. Check those before you hook their fields.
- **`partEMITTER_PHYS` grew 0x2C0 → 0x2C4.** The new float at v12 +0x34 is `sizeStart+sizeEnd`, cached by `SetSize`. Every feb offset ≥ 0x34 is +4 in v12. Emitter subclasses grew by 4 for the same reason. In 1.2, `SetSize` also releases the old size spline.
- **Blood can be switched off.** `wrsfxCreateBloodAir` (v12 0x0046AA30) returns at once if `wrAppState & 2`. The decals and the hands overlay check the same flag. The bit is set from GameOpt/DisplayBlood=0 or from the German/OEM flags; see `re/results/build_differences.md`. feb has no check.
- Vtables are the same length for the sfx classes. In v12 the shared base slots point to new COMDAT-folded stubs.
- Some stack frames grew (`wrsfxStartBlood` 0x88 → 0x8C, `SCORCHMARK::Add` 0x5C → 0x60), and `m3dVZero` moved (feb 0x0096373C → v12 0x009AEA84).

## TWEAK table (v12)

| what | value | v12 address | notes |
|---|---|---|---|
| Max live emitters | 600 (0x258) | imm at 0x004D1753 (partInit), 0x004D1B3A (ConstructorInit) | The array at 0x008FB1B0 is fixed .bss followed by `_partTmpList`. To raise it, relocate the array and patch every reference (partInit/Term/Update/DestroyEmitters/GetNEmit/ctor/dtor; see `xref v12 0x8fb1b0`) |
| Particle record size | 0x3C | push imm at 0x004D17CF (and 0x004D17E0 ctor) | do not change |
| Particles per emitter (float ctor) | `rate*life*1.5 + 3`, minimum 3 | 1.5 at 0x005F93A4, 3.0 at 0x005F93AC (`cmp eax,3` at 0x004D17BA) | The two __real constants are shared with other code. Patch the code to point at your own constant instead of editing them |
| Default particle size | 5.0 | ConstructorInit 0x004D1841 (`mov eax,0x40A00000`) | also the default sizeMax |
| Monster spawn-in decal spacing | 6.0 | push imm at 0x00468716 | radius for FindShaderAround in wrsfxAPPEAR::Start |
| Blood on/off | bit 2 of `wrAppState` | dword 0x0064FFA8 | set it at runtime to force blood off. Clearing it forces blood on, even in the German build |
| Low scorch quality | radius x2 vs x1 | SCORCHMARK::Add 0x00467DE0 (reads `Video/VIDEO_LowScorchQuality`) | |
| Per-effect constants (lifetimes, sizes, colours, rates) | hard-coded | the ctor of each emitter subclass (`??0wrsfx*` rows in function_index.tsv). The `Create` immediates are in `re/work/sdk_sfx_sob/tweaks.tsv` | e.g. wrsfxDUST +0x187 = 5.0 at 0x00472383, wrsobNICHE +0x17F = 70 at 0x0048947C (meaning unverified) |

## HOOK points (v12)

| hook | address | use |
|---|---|---|
| `entCreate` | 0x005082B0 | See or veto every effect spawn by class name. Spawn your own effects |
| `partUpdate(cam, dt)` | 0x004D41C0 | Per-frame particle tick: global time scale or pause |
| `partEMITTER_PHYS` ctor / `ConstructorInit` | 0x004D1790 / 0x004D1810 | Particle budget: scale the max-particle count |
| `partRender` | 0x004D42F0 | Replace the particle renderer (port), or add soft particles |
| `wrsfxStartBlood` / `wrsfxCreateBloodAir` | 0x0046AB50 / 0x0046AA30 | Change or disable blood, recolour it |
| `wrsfxStartChunkMeat` | 0x0046B1B0 | Gibs |
| `wrsfxSCORCHMARK::Add` | 0x00467DE0 | Scorch decals |
| `pteMANAGER::FindShaderAround` | 0x004D9360 | Decal density |
| `wrsfxInit` / `wrsobInit` | 0x00467830 / 0x00486560 | Register new `sfx_*`/`sob_*` classes (call `entRegisterClass` after the original) |
| `<class>::ProcessMsg` | vtable slot +0x04 of each class (see headers) | Per-effect behaviour |
| Emitter vtables | `PartCreate` +0x1C, `PartUpdate` +0x20, `PartApplyCDT` +0x2C | Custom particle motion. Subclass `partEMITTER_PHYS` in a mod DLL and allocate 0x2C4 bytes |

## PORTING notes
The subsystem makes **no direct OS, Win32 or DirectX calls**. I scanned all sfx_/sob_/particle/part_eff/partobj/Partrend/pte_spot functions in the decompile, and the only imports used are engine APIs. A port has to supply these engine services:
- `txmMANAGER::Add` (textures by name; flags 0x40003)
- `rendCFG::InitMtl1` and the quad output in `partRender`/`RegisterPart` (Partrend.obj). This is the only rendering touch point, and it is where Direct3D would be replaced
- `pteSHADER`/`pteMANAGER` decal projection (`ShadeFace` per face)
- `camCAMERA::ProjPoint/TransformPoint`, plus m3d/m2d math
- `qsort` (CRT), `__ftol`
- `sncCHANNEL` (sound; DirectSound underneath, in the sound subsystem)
- `entRegisterClass` / `entCreate` / message system
- `apCfgReadBool` (registry/ini, used for DisplayBlood and LowScorchQuality)
- `apMalloc/apFree/apRealloc`

Everything is plain portable C++ on top of those. The packed (`pack(1)`) layouts and `__fastcall` statics must be kept when you link against original binaries, but they do not matter for a source port.

## Class table (entity class name → v12)

| name | class | v12 Create | feb Create | v12 size | v12 vtable |
|---|---|---|---|---|---|
| barrel | wrsfxDESTROY_BARREL | 0x00471720 | 0x00467A50 | 0x183 | 0x00601928 |
| flare | wrsfxFLARE | 0x00473C20 | 0x00469F40 | 0x40000 | 0x00601C58 |
| lava | wrsfxLAVA | 0x0047A040 | 0x00470080 | 0x19B | 0x00602600 |
| mirror | wrsfxDESTROY_MIRROR | 0x004716E0 | 0x00467A10 | 0x183 | 0x006018A0 |
| nch_rigid | nchRIGID | 0x00488160 | 0x0047DBD0 | 0xF37 | 0x00603A58 |
| sfx_altar | wrsfxALTAR | 0x00465A20 | 0x0045BFC0 | 0x1AF | 0x00600820 |
| sfx_altar_fireball | wrsfxALTAR_FIREBALL | 0x00467730 | 0x0045DA60 | 0x197 | 0x00600980 |
| sfx_appear | wrsfxAPPEAR | 0x00468970 | 0x0045EBA0 | 0x16B | 0x00600B10 |
| sfx_atlas_globe_appear | wrsfxATLAS_GLOBE_APPEAR | 0x00469690 | 0x0045F8C0 | 0x183 | 0x00600C10 |
| sfx_atlas_globe_land | wrsfxATLAS_GLOBE_LAND | 0x00469710 | 0x0045F940 | 0x17B | 0x00600C78 |
| sfx_barrel_expl | wrsfxBARREL_EXPLODE | 0x00469820 | 0x0045FA50 | 0x17F | 0x00600CE0 |
| sfx_bird_death | wrsfxBIRD_DEATH | 0x0046A870 | 0x00460AA0 | 0x17B | 0x00600E48 |
| sfx_bridge | wrsfxBRIDGE | 0x0046D4C0 | 0x00463660 | 0x1A3 | 0x006010E8 |
| sfx_bridge_crash | wrsfxBRIDGE_CRASH | 0x0046D430 | 0x004635D0 | 0x1AF | 0x00601080 |
| sfx_cerber_pjl_acid_expl | wrsfxCERBER_ACID_EXPL | 0x0046E4B0 | 0x004646A0 | 0x177 | 0x006012C0 |
| sfx_cerber_pjl_expl | wrsfxCERBER_FIREBALL_EXPL | 0x0046E430 | 0x00464620 | 0x17B | 0x00601258 |
| sfx_chunk | wrsfxCHUNK | 0x0046C840 | 0x004629E0 | 0x18F | 0x00600FD0 |
| sfx_cyclop_death | wrsfxCYCLOP_DEATH | 0x004707A0 | 0x00466A60 | 0x177 | 0x00601660 |
| sfx_cyclop_hit | wrsfxCYCLOP_HIT | 0x00470720 | 0x004669E0 | 0x177 | 0x006015F8 |
| sfx_cyclop_walk | wrsfxCYCLOP_WALK | 0x004706A0 | 0x00466960 | 0x177 | 0x00601590 |
| sfx_discus_land | wrsfxDISCUS_LAND | 0x00472250 | 0x00468580 | 0x183 | 0x00601A58 |
| sfx_dust | wrsfxDUST | 0x00472330 | 0x00468660 | 0x193 | 0x00601AC8 |
| sfx_egg | birdEGG_SFX | 0x004641F0 | 0x0045A850 | 0x1A7 | 0x00600618 |
| sfx_flame | wrsfxFLAME | 0x00472FE0 | 0x00469310 | 0x183 | 0x00601BD8 |
| sfx_fountain | wrsfxFOUNTAIN | 0x004750C0 | 0x0046B3E0 | 0x19B | 0x00601E20 |
| sfx_gefest_expl | wrsfxGEFEST_EXPL | 0x00476450 | 0x0046C770 | 0x17B | 0x00602020 |
| sfx_gefest_hit | wrsfxGEFEST_HIT | 0x004763D0 | 0x0046C6F0 | 0x17B | 0x00601FB8 |
| sfx_glad_mace_expl | wrsfxGLAD_MACE_EXPL | 0x004770E0 | 0x0046D400 | 0x17F | 0x00602150 |
| sfx_item | wrsfxITEM | 0x004771B0 | 0x0046D4D0 | 0x1A7 | 0x006021C0 |
| sfx_lava_expl | wrsfxLAVA_EXPL | 0x00479FC0 | 0x00470000 | 0x17B | 0x00602598 |
| sfx_lava_pjl | wrsfxLAVA_PJL | 0x00479B90 | 0x0046FBF0 | 0x19C | 0x00602458 |
| sfx_lava_start | wrsfxLAVA_START | 0x00479B10 | 0x0046FB70 | 0x177 | 0x006023F0 |
| sfx_medusa_death | wrsfxMEDUSA_DEATH | 0x0047B650 | 0x00471680 | 0x1B3 | 0x006028D0 |
| sfx_medusa_expl | wrsfxMEDUSA_EXPL | 0x0047B570 | 0x004715B0 | 0x1B3 | 0x00602860 |
| sfx_mirrors | wrsfxMIRRORS | 0x0047C9B0 | 0x00472720 | 0x17B | 0x006029C0 |
| sfx_niche_explode | wrsfxNICHE_EXPLODE | 0x0047D4C0 | 0x00473230 | 0x187 | 0x00602A28 |
| sfx_opp_appear | wrsfxOPP_APPEAR | 0x0047D640 | 0x004733B0 | 0x173 | 0x00602B28 |
| sfx_perseus | wrsfxPERSEUS | 0x0047F010 | 0x00474CB0 | 0x1BB | 0x00602C60 |
| sfx_portal | wrsfxPORTAL | 0x0047F410 | 0x004750A0 | 0x187 | 0x00602D08 |
| sfx_rat_expl | wrsfxRAT_EXPL | 0x004811A0 | 0x00476E20 | 0x17B | 0x00602F20 |
| sfx_rat_sparks | wrsfxRAT_SPARKS | 0x00481220 | 0x00476EA0 | 0x17F | 0x00602F88 |
| sfx_satir_arrow_land | wrsfxSATIR_ARROW_LAND | 0x004819C0 | 0x00477640 | 0x17B | 0x00603050 |
| sfx_skel_death | wrsfxSKELETON_DEATH | 0x004836D0 | 0x00479340 | 0x1AF | 0x006031C8 |
| sfx_skel_spear_appear | wrsfxSKELETON_SPEAR_APPEAR | 0x004835A0 | 0x00479210 | 0x18F | 0x00603230 |
| sfx_skel_spear_land | wrsfxSKELETON_SPEAR_LAND | 0x00483650 | 0x004792C0 | 0x17F | 0x00603298 |
| sfx_sphinx_pjl_expl | wrsfxSPHINX_EXPL | 0x00483DF0 | 0x00479A60 | 0x177 | 0x00603360 |
| sfx_statue_break_part | aisSTATUE_BREAK_PART_SFX | 0x00434E50 | 0x0042F0D0 | 0x183 | 0x005FC688 |
| sfx_treasure | wrsfxTREASURE | 0x00484840 | 0x0047A4A0 | 0x1A7 | 0x00603510 |
| sfx_water_expl | pwpWSFX_EXPL | 0x0041CFC0 | 0x0041A560 | 0x16B | 0x005FB480 |
| sfx_water_splash | pwpWSFX_SPLASH | 0x0041CF60 | 0x0041A500 | 0x16B | 0x005FB418 |
| sfx_water_stream | wrsfxWATER | 0x00484E50 | 0x0047AB40 | 0x17F | 0x006035E8 |
| sfx_waterfall | wrsfxWATERFALL | 0x00485B80 | 0x0047B870 | 0x18F | 0x00603718 |
| sfx_zeus_lgtn | wrsfxZEUS_LGTN | 0x00486490 | 0x0047BFD0 | 0x1A7 | 0x006037F0 |
| sob_blade | wrsobBLADE | 0x00487100 | 0x0047CB90 | 0x1BF | 0x00603868 |
| sob_geyser | wrsobGEYSER | 0x00487270 | 0x0047CCF0 | 0x18B | 0x006038E0 |
| sob_katapult | wrsobKATAPULT | 0x00487F70 | 0x0047D9F0 | 0x1AF | 0x006039E0 |
| sob_niche | wrsobNICHE | 0x00489440 | 0x0047EDB0 | 0x183 | 0x00603B30 |
| sob_spike | wrsobSPIKE | 0x00489860 | 0x0047F1E0 | 0x1A7 | 0x00603BA0 |
| sob_tra_altar | wrsobTRA_ALTAR | 0x0048AD20 | 0x0047FFD0 | 0x18F | 0x00603D38 |
| sob_tra_button | wrsobTRA_BUTTON | 0x0048ABB0 | 0x0047FE70 | 0x193 | 0x00603C58 |
| sob_tra_camera | wrsobTRA_CAMERA | 0x0048AC60 | 0x0047FF20 | 0x1A7 | 0x00603CC8 |
| sob_tree | wrsobTREE | 0x0048B060 | 0x00480300 | 0x16B | 0x00603DA0 |
| tripod | wrsfxDESTROY_TRIPOD | 0x00471690 | 0x004679C0 | 0x18B | 0x00601818 |
| vase_destroy | wrsfxDESTROY_VASE | 0x00471650 | 0x00467980 | 0x183 | 0x00601790 |

## Open questions
- `partPART` layout is not reconstructed. The order of the two empty vtable stubs (PartDeath vs PartPostCDT, slots +0x14/+0x28) is not settled.
- v12 addresses are still missing for 88 functions (mostly emitter ctors that 1.2 inlined, `pwpOPP_SFX::Start`, `KillAll`, `SetCS`, `AttachEmitter`, `wrsobNICHE` MP sync).
- 93 own fields are marked `// ?`: the v12 offset is derived, not seen in an instruction. Meanings of most `f_XXX` fields are not yet named. Use the ctor init values in the headers together with the ProcessMsg decompile (`db/mergedfiles.cpp`).
- What `type`/`kind` mean in `wrsfxStartBlood`. Template parameter blocks for sfx in level data are not examined.
- Doors and lifts are generic `dyn*` objects and belong to the engine/dynamics subsystem, not here.
