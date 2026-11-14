# engine_render: Will Rock rendering side (unofficial SDK)

Target: v12 (`re/bin/v12.exe`, 1.2). Reference: feb build + `db/WillRock.map`. Static analysis only.
Every address is given as `v12 0x... (feb 0x...)`. **Verification rule used here:** a function marked *exact* in
`tables/v12.functions.tsv` has identical code once absolute addresses are masked. Struct displacements are not masked, so every field
offset such a function uses is the same in v12. Offsets that come only from feb or from callgraph matches are marked `// ?`.

## Files
| file | contents |
|---|---|
| `rend_driver.h` | `rendDRIVER` (engine renderer front end, 44-slot vtable), `rnsMNG` sort buffers, `rendCFG`, global `rendDrv` |
| `scene.h` | `scnSCENE` (level container, tpl/inst pools), scene file chunk ids (`sioReadScene`), `scnDOMAIN` |
| `texture.h` | `txmMANAGER`, `txmTEXTURE`, `txmD3D_*`, flags, BMP loader |
| `shader.h` | `pteMANAGER`/`pteSHADER` CPU face/vertex "shaders" (env maps, spot lights, ground fog...), class registry |
| `particle.h` | `partEMITTER_PHYS`, particle globals and limits (**layout changed in retail**) |
| `d3d_driver.h` | `vidDRIVER` / `d3dDRIVER` / `d3dREND_DRIVER` (DirectDraw7/D3D7 backend, the porting boundary), D3D caches/buffers |
| `functions_index.h` | generated: all 1013 functions of the covered objs by class, 934 with v12 address + match kind |
| `vtables_dump.txt` | slot by slot feb/v12 dump of 18 vtables with per-slot agreement check (`ok`) |

Work files: `re/work/sdk_engine_render/` (`functions.tsv`, `globals_map.txt`, `api_calls.txt`, `gen.py`, `vt.py`, `dc.sh`).

### Objs covered
engine: `Scene, scn_coll, scn_domain, scn_vis, sceneio, sceneio_misc, sceneio_obj, sceneio_scn, sceneio_tpl, Rend_ini, Rend_obj,
Rend_sort, Rend_anim, rend_vis, Partrend, particle, partobj, pte_envmap, pte_fog, pte_sdr_static, pte_shader, pte_spot, pte_spot_a,
pte_spot_e, pte_tex2, lod, obj_mod, obj_mod_geom, obj_mod_tex, obj_normals, obj_split, Object, Obj_misc, picture, pic_bmp, pic_tga,
pic_pic, pic_decompress, pssheet, font`.
drv (the render backend, needed for porting): `vid, txm, D3d_coll, D3d_drv, d3d_cfg, D3d_prep_cfg, D3d_rnd, D3d_rnd_vbuf,
D3d_rnd_idx, D3d_rnd_misc, D3d_rnd_fvf, d3d_txm, d3d_postbuf, d3d_misc`.
Not covered here: anim*/cdt* (animation/collision), `gs:gs_rend` (game-side `gsRENDER_SYSTEM`, see render_ui results), the mirror/shadow systems
(`mrr*`, `psh*`), lights `lgt*` (only referenced). `Object/Obj_misc/obj_mod*` (objOBJ itself) is listed in the index only; objOBJ belongs to core.
There are no `lit*`/`msh*` objs in this engine. Meshes are `objOBJ` (core), lights are `lgtLIGHT_GROUP` + `scnSCENE::lights`, and
visibility is in `scn_vis` (ray/LOS), `rendDRIVER::QualObj/QualInst` (frustum) and `gsSysVis` (game-side portal/vis system, vtable slots +0x7C/+0x80).

## How it works
**Objects.** `vidDriver`/`d3dDriver` (one object) owns DirectDraw/D3D. `d3dDRIVER::CreateRendDriver` makes the `d3dREND_DRIVER` stored in
`rendDrv`, and `CreateTexManager` makes the `txmD3D_MANAGER` stored in `txmManager`. The level is a `scnSCENE` (game global `gsScenePtr`):
a static `objOBJ` tree (`root`, flattened to `objList` in `InitLevel`), pools of 300 templates and 1000 instances (`animTPL`/`animINST`),
lights, a collision scene, named selections, a script sheet and a camera. `pteManager` holds per-object CPU shaders created from
property sheets. Particles are a flat array of up to 600 `partEMITTER_PHYS`.

**Level load.** `sioSetSourceFile` + `sioReadScene` read a chunked `fioFILE` (chunk ids in `scene.h`: 0xF0 objects, 0x154/0x155 textures,
0x1B8 instances, 0x1EA templates, 0x21C/0x21F collision, 0x21D selections, 0x21E scene script, 0x280 lights, 0x3AC camera). Then
`scnSCENE::InitLevel`, `rendDRIVER::InitLevel` (default light, `SetLight`/`EnableLight`), `pteMANAGER::InitLevel`, and the driver
`ObjLoadNotify`/`InstLoadNotify`/`Obj*InitHWNotify`, which build vertex buffers and "prep CFG" (per-object cached render configs).
Textures are loaded by name through `txmMANAGER::Add` (search path in `txmMANAGER+0x08`, `fnmFindDisc`), decoded by `pct*` (BMP/TGA/own PIC with
decompression), and uploaded with `HwDownload`.

**Per frame** (`gsRENDER_SYSTEM::RenderFrame` v12 0x0051BA50, feb 0x00506640):
1. mirrors (`mrrMIRROR_SYSTEM::Render`), then `rendDrv->BeginCamera(cam, 0.05, 1300.0)` (slot +0x30)
2. `objMODIFIER::elapsedTime/viewPos` set (texture scroll/anim, billboard, noise modifiers)
3. game visibility `gsSysVis` slots +0x7C/+0x80
4. `pteManager->ActivateAllShaders(cam)`, `PrepareFrame(scene, cam)`
5. projected shadows `ConfigureTransp`
6. `rendDrv->RenderObjListStatic(scene->nObj, scene->objList, ...)` (static world), then for each instance (`scnSCENE::FindNextInst`)
   `rendDRIVER::RenderInst` (QualInst frustum test, LOD `animINST::LODApply`, then `_RenderObj`)
7. second camera (`BeginCamera(cam, 0.4, 1300)`) for sky/far objects. `rnsMNG::Purge(pass, 4)` opaque, shadow finish, `Purge(pass,0..2)`
   transparent buckets, `partRender(cam, pass, vis)`, `Purge(pass,3)`
8. optional near pass (`BeginCamera(cam, 0.05, 100)`, pass 1) for first-person weapon, clear Z through `vidDriver` slot +0x24
9. `rnsMNG::PurgePoly` (sorted loose polys), `EndCamera`. Then `gsRENDER_SYSTEM::EndFrame/SwapBuffers` → `d3dDRIVER::SwapBuffers`.
Inside the driver, objects go `PreDrawObj` → `pteMANAGER::ShadeFaceWCS/ShadeVert` (CPU shading into `_d3drSdrVList`) → `DrawIndexedPrimitive`
via the vertex-buffer manager `d3dVBMng`, with D3D state filtered through `_d3dStateCache`/`_d3dStageStateCache`.

## 1.2 vs feb differences found
- **partEMITTER_PHYS grew 4 bytes:** new float at v12 +0x34 (init 5.0). All later fields moved +4 (see `particle.h`). Most of its methods only callgraph-match.
- Every vtable checked (rendDRIVER, d3dREND_DRIVER, vidDRIVER, d3dDRIVER, txmMANAGER, txmD3D_MANAGER, txmTEXTURE, txmD3D_TEX, pteSHADER, pteSPOT,
  pteFOG_GROUND, scnDOMAIN, partEMITTER_PHYS, objMODIFIER, objLOD, rnsSIGNATURE) has **the same slot count and order** in v12. Unlike
  the entity classes, there are no new slots in the render vtables.
- scnSCENE, rendDRIVER, d3dDRIVER, txmMANAGER/txmTEXTURE, pteMANAGER/pteSHADER: unchanged layouts (exact ctors). animINST pool stride is still 0x14C and animTPL still 0xCC.
- All limits (300 tpl, 1000 inst, 1000 textures, 600 emitters, 2000 visible particles) are unchanged.
- The third-party widescreen patch in v12 (`.text1` 0x009E8000, caves 0x005F80ED..0x005F8270) hooks camera/projection. It was not analysed here. Check for conflicts before hooking `BeginCamera`/FOV.

## TWEAK table (v12 instruction addresses; patch the immediate)
| constant | value | v12 address(es) |
|---|---|---|
| scene template pool | 300 (0x12C) | push 0x004E5E23; cmp 0x004E5EAD, 0x004E6426, 0x004E6E73, 0x004E6E96 |
| scene instance pool | 1000 (0x3E8) | push 0x004E5E4D; cmp 0x004E64A2, 0x004E6526; end-offset 0x510E0 at 0x004E5E92, 0x004E6213, 0x004E5F86 |
| texture slots | 1000 (0x3E8) | ctor 0x004B314F; Add 0x004B3446; Find 0x004B361D; Alloc 0x004B364C/64/6C (array is inline in txmMANAGER at +0x150, so raising it needs a bigger object: ctor is in d3dDRIVER::CreateTexManager v12 0x004BAF50) |
| particle emitters | 600 (0x258) | 0x004D1752 (clear), 0x004D1B39, 0x004D1C67. The array is static at 0x008FB1B0, so it needs relocating to grow |
| visible particles/frame | 2000 (0x7D0) | 0x004D549E (array 0x008FBB10 static) |
| near/far planes main pass | 0.05 / 1300.0 | push 0x3D4CCCCD at 0x0051BAE5, push 0x44A28000 at 0x0051BAE0 (draw distance) |
| far pass | 0.4 / 1300.0 | 0x0051BC84 / 0x0051BC7F |
| weapon pass | 0.05 / 100.0 | 0x0051BDE8 / 0x0051BDE3 |
| default particle part size | 0x3C | partEMITTER_PHYS(float,float) v12 0x004D1790 |
| pteMANAGER cdtOFL | 500, 3000 | pteInit v12 0x004D8370 |

## HOOK points (v12)
- `gsRENDER_SYSTEM::RenderFrame` 0x0051BA50: the whole 3D frame (insert custom passes before EndCamera).
- `rendDRIVER::BeginCamera` (vtbl `rendDrv`+0x30 → d3d 0x004BF4E0) / `EndCamera` (0x004BF810): per-camera setup.
- `rendDRIVER::RenderInst` 0x0059B900, `RenderObj` 0x004FC1E0, `d3dREND_DRIVER::PreDrawObj` 0x004C31D0: per-object draw / tint / hide.
- `rendDRIVER::RenderLine/RenderSphere/RenderCross/RenderQuad` (0x004E87E0/0x004E8650/0x004E8A50/0x004EA0B0): debug drawing for mods.
- `txmMANAGER::Add` 0x004B3410: texture replacement packs (redirect names/paths).
- `pteMANAGER::RegisterClass` 0x004D8480: register a new shader type `{name, priority, factory}` that level sheets can use. `CreateShader` 0x004D8650/0x004D86D0.
- `sioReadScene` 0x0059E680: custom chunks in level files (default branch skips unknown chunks).
- `scnSCENE::CreateInst` 0x004E6620 / `DestroyInst` 0x004E6CD0: spawn/despawn of every visual instance.
- `partEMITTER_PHYS::Update` (vtbl +0x0C, 0x004D3730), `partRender` 0x004D42F0.
- `d3dDRIVER::SwapBuffers` 0x004BB820: overlay/post-processing/screenshot point. `SetGamma` 0x004BE5F0.

## PORTING notes
Everything platform-specific is behind `vidDRIVER`/`d3dDRIVER` (34 slots), `rendDRIVER`/`d3dREND_DRIVER` (46 slots) and `txmMANAGER`/`txmTEXTURE`
driver slots (`HwInit/HwTerm/HwDownload/HwDelete`, `CreateDrv/LoadDrv/UnloadDrv/RequestSize/GetColor/SetColor`). The engine side (scene, pte shaders, sort
buffers, particles, LOD, modifiers) is portable C++ that only calls these virtuals. A port therefore writes three classes (e.g. `glDRIVER`, `glREND_DRIVER`,
`txmGL_MANAGER`/`txmGL_TEX`) and returns them from `CreateRendDriver`/`CreateTexManager`.

APIs used by the covered code:
| API | where (feb fn → v12) | replace with |
|---|---|---|
| `DirectDrawCreateEx` (IID_IDirectDraw7), then QueryInterface IID_IDirect3D7 | `d3dDRIVER::InitDrv` 0x004BAE10, `d3dEnumDDDeviceCB` 0x004B8510 | GL/Vulkan/SDL context creation |
| `DirectDrawEnumerateExA` (via `GetModuleHandleA("DDRAW.DLL")`+`GetProcAddress`), `DirectDrawEnumerateA` fallback | `d3dEnumDevices` 0x004B7F30 | adapter/mode enumeration (SDL_GetDisplayMode) |
| IDirectDraw7: GetCaps (+0x30), SetCooperativeLevel (+0x50, flags 0x1011 fullscreen / 0x1008 windowed), SetDisplayMode (+0x54), CreateSurface (+0x18), EnumDisplayModes, EnumZBufferFormats; surface GetAttachedSurface (+0x30), GetSurfaceDesc (+0x58), Flip/Blt | `d3dDRIVER::InitHW` 0x004BAF90, `SwapBuffers` 0x004BB820, `ClearBuffers` 0x004BBA00, `ReadBuffer` 0x004BBB10, `CopyBuffer` 0x004BBBC0 | swapchain, default framebuffer, glReadPixels |
| IDirect3DDevice7: SetRenderState/SetTextureStageState (cached), SetTransform, SetLight/LightEnable, SetClipPlane, DrawIndexedPrimitiveVB, vertex buffers | `d3d_cfg`, `D3d_rnd*` (`d3dREND_DRIVER` slots), `d3dVBMng` | GL state + VBO/IBO; fixed-function lighting, fog and clip planes need shader emulation |
| IDirectDrawGammaControl | `d3dDRIVER::SetGamma` 0x004BE5F0 | SDL_SetWindowGammaRamp or a post-process shader |
| DirectDraw texture surfaces, texture-format enumeration (`d3dEnumTextureFormatsCB`) | `txmD3D_MANAGER` / `txmD3D_TEX` (d3d_txm.obj) | glTexImage2D. Formats 0x0C..0x11 look like DXT (`TXM_F_COMPRESSED`) |
| Win32 GDI `LoadImageA`, `GetObjectA`, `CreateCompatibleDC`, `SelectObject`, `GetPixel`, `DeleteDC`, `DeleteObject` | `pctReadBMPHdr` 0x00504F20, `pctReadBMP` 0x00504FA0 | portable BMP decoder (the GetPixel loop is also slow) |
| `GetDC`/`GetDeviceCaps`/`ReleaseDC` (desktop bpp) | `vidGetDriverList` 0x004B3B60 | platform display query |
| `ClientToScreen`, `SetRect` (windowed blit rectangles) | `d3dDRIVER::SwapBuffers`, `CopyBuffer`, `ClearSurfaces` | not needed with a real swapchain |
| HWND | `vidDRIVER::SetMode(mode, hwnd, flags)` stores at +0x214 | native window handle |

Other port concerns: 32-bit x86 MSVC `__thiscall`/`__fastcall` everywhere; the "pte" shaders and particles do all lighting on the CPU, so a port does not need
to rewrite them; D3D7 fixed-function fog (`ConfigureFog`), W-buffer and stencil shadows (`ConfigureStencil`, `ToggleStencil`) are the
features that need GPU-side equivalents.

## Open questions
- pteEMAP family v12 vtable address (the automatic data map is wrong) and pteSPOT_ANIM vtable.
- Exact meaning of many `rendDRIVER` +0x3C..+0x74 fields (light block) and `rnsMNG` internals. The vidMODE flag bits.
- `_d3dObjDot3Color` v12 address. The d3dEnum D3D/display-mode/texture-format callbacks are not matched in v12 yet (find them from the push-immediates in `d3dEnumDevices`/InitHW).
- 79 covered functions have no v12 twin (listed as `----------` in `functions_index.h`). Many are small inlined or folded helpers.
- partEMITTER_PHYS field names beyond the ctor defaults.
