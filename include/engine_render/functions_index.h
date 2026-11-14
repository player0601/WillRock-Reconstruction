// Will Rock 1.2 - engine render: function index (generated from db/WillRock.map + re/tables/v12.functions.tsv)
// Every function of the covered render objs, grouped by class.
// Format: v12 address (feb address) [match kind] method   -- obj
// match kind: exact = identical code with absolute addresses masked (field offsets therefore identical in v12),
//             vtable/callgraph = located by vtable slot / call order (code changed or not unique; check before hooking),
//             "--------" = no v12 twin found yet (// ?).
// Hand-written declarations with descriptions are in the other headers; this file is the exhaustive address list.
#pragma once

// ---- (free functions)
//   v12 0x004B1EC0 (feb 0x004A3260) [exact    ] ??0txmTEXTURE@@QAE@PADH@Z  -- drv:txm.obj
//   v12 0x004B1F10 (feb 0x004A32B0) [vtable   ] ??_EtxmTEXTURE@@UAEPAXI@Z  -- drv:txm.obj
//   v12 0x004B1F10 (feb 0x004A32B0) [vtable   ] ??_GtxmTEXTURE@@UAEPAXI@Z  -- drv:txm.obj
//   v12 0x004B1F30 (feb 0x004A32D0) [callgraph] ??1txmTEXTURE@@UAE@XZ  -- drv:txm.obj
//   v12 0x004B2990 (feb 0x004A3D30) [exact    ] ??0txmTEXTURE_LAY@@QAE@PADH@Z  -- drv:txm.obj
//   v12 0x004B2A20 (feb 0x004A3DC0) [exact    ] ??_EtxmTEXTURE_LAY@@UAEPAXI@Z  -- drv:txm.obj
//   v12 0x004B2A20 (feb 0x004A3DC0) [exact    ] ??_GtxmTEXTURE_LAY@@UAEPAXI@Z  -- drv:txm.obj
//   v12 0x004B2A70 (feb 0x004A3E10) [exact    ] ??1txmTEXTURE_LAY@@UAE@XZ  -- drv:txm.obj
//   v12 0x004B3140 (feb 0x004A44E0) [exact    ] ??0txmMANAGER@@QAE@XZ  -- drv:txm.obj
//   v12 0x004B3190 (feb 0x004A4530) [exact    ] ??_GtxmMANAGER@@UAEPAXI@Z  -- drv:txm.obj
//   v12 0x004B3190 (feb 0x004A4530) [exact    ] ??_EtxmMANAGER@@UAEPAXI@Z  -- drv:txm.obj
//   v12 0x004B31E0 (feb 0x004A4580) [exact    ] ??1txmMANAGER@@UAE@XZ  -- drv:txm.obj
//   v12 0x004B3AF0 (feb 0x004A4E90) [exact    ] vidInit  -- drv:vid.obj
//   v12 ---------- (feb 0x004A4ED0) [none     ] vidTerm  -- drv:vid.obj
//   v12 0x004B3B60 (feb 0x004A4F00) [exact    ] vidGetDriverList  -- drv:vid.obj
//   v12 0x004B3C50 (feb 0x004A4FF0) [callgraph] vidFindBestDriver  -- drv:vid.obj
//   v12 0x004B3C70 (feb 0x004A5010) [exact    ] vidFindDriver  -- drv:vid.obj
//   v12 0x004B3CE0 (feb 0x004A5080) [exact    ] vidSelectDriver  -- drv:vid.obj
//   v12 0x004B3D10 (feb 0x004A50B0) [exact    ] ??0vidDRIVER@@QAE@H@Z  -- drv:vid.obj
//   v12 0x004B3D80 (feb 0x004A5120) [exact    ] ??_GvidDRIVER@@UAEPAXI@Z  -- drv:vid.obj
//   v12 0x004B3D80 (feb 0x004A5120) [exact    ] ??_EvidDRIVER@@UAEPAXI@Z  -- drv:vid.obj
//   v12 0x004B3DB0 (feb 0x004A5150) [callgraph] ??1vidDRIVER@@UAE@XZ  -- drv:vid.obj
//   v12 0x004B3EB0 (feb 0x004A5250) [callgraph] ??AvidDRIVER@@QAEPAUvidMODE@@H@Z  -- drv:vid.obj
//   v12 0x004B4730 (feb 0x004A5AD0) [exact    ] _vidCompDrv  -- drv:vid.obj
//   v12 0x004B7F30 (feb 0x004A84F0) [exact    ] d3dEnumDevices  -- drv:D3d_coll.obj
//   v12 0x004B7FD0 (feb 0x004A8590) [exact    ] d3dGetDrvList  -- drv:D3d_coll.obj
//   v12 0x004B84C0 (feb 0x004A8A80) [exact    ] d3dDropDrvList  -- drv:D3d_coll.obj
//   v12 ---------- (feb 0x004A8AB0) [none     ] d3dEnumDDDeviceCB_OLD  -- drv:D3d_coll.obj
//   v12 ---------- (feb 0x004A8AD0) [none     ] d3dEnumDDDeviceCB  -- drv:D3d_coll.obj
//   v12 ---------- (feb 0x004A8E40) [none     ] d3dEnumD3DDeviceCB  -- drv:D3d_coll.obj
//   v12 ---------- (feb 0x004A9040) [none     ] d3dEnumDisplayModesCB  -- drv:D3d_coll.obj
//   v12 ---------- (feb 0x004A90D0) [none     ] d3dEnumTextureFormatsCB  -- drv:D3d_coll.obj
//   v12 0x004B8F30 (feb 0x004A94F0) [exact    ] d3dEnumZBufferFormatsCB  -- drv:D3d_coll.obj
//   v12 0x004BAC40 (feb 0x004AB3E0) [exact    ] ??0d3dDRIVER@@QAE@XZ  -- drv:D3d_drv.obj
//   v12 0x004BADB0 (feb 0x004AB550) [exact    ] ??_Ed3dDRIVER@@UAEPAXI@Z  -- drv:D3d_drv.obj
//   v12 0x004BADB0 (feb 0x004AB550) [exact    ] ??_Gd3dDRIVER@@UAEPAXI@Z  -- drv:D3d_drv.obj
//   v12 0x004BBEA0 (feb 0x004AC620) [vtable   ] ??_GtxmD3D_MANAGER@@UAEPAXI@Z  -- drv:D3d_drv.obj
//   v12 0x004BBEA0 (feb 0x004AC620) [vtable   ] ??_EtxmD3D_MANAGER@@UAEPAXI@Z  -- drv:D3d_drv.obj
//   v12 0x004C1440 (feb 0x004B1BA0) [exact    ] ??0txmD3D_TEX@@QAE@PADH@Z  -- drv:d3d_txm.obj
//   v12 ---------- (feb 0x004B1BF0) [none     ] ??0txmD3D_TEX_LAY@@QAE@PADH@Z  -- drv:d3d_txm.obj
//   v12 0x004C1CC0 (feb 0x004B2390) [exact    ] ??_GtxmD3D_TEX@@UAEPAXI@Z  -- drv:d3d_txm.obj
//   v12 0x004C1CC0 (feb 0x004B2390) [exact    ] ??_EtxmD3D_TEX@@UAEPAXI@Z  -- drv:d3d_txm.obj
//   v12 0x004C1E00 (feb 0x004B24D0) [exact    ] ??_EtxmD3D_TEX_LAY@@UAEPAXI@Z  -- drv:d3d_txm.obj
//   v12 0x004C1E00 (feb 0x004B24D0) [exact    ] ??_GtxmD3D_TEX_LAY@@UAEPAXI@Z  -- drv:d3d_txm.obj
//   v12 0x004C1EA0 (feb 0x004B2570) [callgraph] ??0d3dREND_DRIVER@@QAE@XZ  -- drv:D3d_rnd_idx.obj
//   v12 0x004C1EC0 (feb 0x004B2590) [exact    ] ??_Ed3dREND_DRIVER@@UAEPAXI@Z  -- drv:D3d_rnd_idx.obj
//   v12 0x004C1EC0 (feb 0x004B2590) [exact    ] ??_Gd3dREND_DRIVER@@UAEPAXI@Z  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2B70 (feb 0x004B31F0) [exact    ] d3dDecideVBStatDyn  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2CA0 (feb 0x004B3340) [exact    ] d3dReadLock  -- drv:d3d_misc.obj
//   v12 0x004C2CF0 (feb 0x004B3390) [exact    ] d3dUnlock  -- drv:d3d_misc.obj
//   v12 0x004C2EB0 (feb 0x004B3560) [callgraph] d3drPrepareMaterial  -- drv:D3d_rnd.obj
//   v12 0x004C5290 (feb 0x004B5940) [exact    ] d3dMakeFaceIndGeo  -- drv:D3d_rnd.obj
//   v12 0x004C5BE0 (feb 0x004B6230) [exact    ] ??_Ed3dOBJ_UD_PREP_CFG@@UAEPAXI@Z  -- drv:D3d_prep_cfg.obj
//   v12 0x004C5BE0 (feb 0x004B6230) [exact    ] ??_Gd3dOBJ_UD_PREP_CFG@@UAEPAXI@Z  -- drv:D3d_prep_cfg.obj
//   v12 0x004C5C40 (feb 0x004B6290) [callgraph] ??0rendCFG_PREP@@QAE@XZ  -- drv:D3d_prep_cfg.obj
//   v12 0x004C6660 (feb 0x004B6BF0) [callgraph] _d3drGetMtlStgBlend  -- drv:D3d_prep_cfg.obj
//   v12 0x004C6B00 (feb 0x004B7090) [vtable   ] ??_GobjUSR_DATA@@UAEPAXI@Z  -- drv:D3d_prep_cfg.obj
//   v12 0x004C6B00 (feb 0x004B7090) [vtable   ] ??_EobjUSR_DATA@@UAEPAXI@Z  -- drv:D3d_prep_cfg.obj
//   v12 0x004C6B50 (feb 0x004B70E0) [callgraph] d3dPrepareVert  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C6FE0 (feb 0x004B7550) [callgraph] d3dCreateSharedVertList  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C7290 (feb 0x004B7800) [exact    ] d3dPrepareFaceIndSlice  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C7340 (feb 0x004B78B0) [callgraph] _d3dCalcObjDot3Color  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C8700 (feb 0x004B8AA0) [callgraph] d3dUpdateVertData  -- drv:D3d_rnd_vbuf.obj
//   v12 ---------- (feb 0x004BB5B0) [none     ] objIsNFace  -- engine:Obj_misc.obj
//   v12 ---------- (feb 0x004BB5C0) [none     ] objIsNVert  -- engine:Obj_misc.obj
//   v12 0x004CC810 (feb 0x004BC7B0) [exact    ] objConvertSpl  -- engine:Obj_misc.obj
//   v12 0x004CC8D0 (feb 0x004BC870) [exact    ] objFind  -- engine:Obj_misc.obj
//   v12 ---------- (feb 0x004BC8D0) [none     ] objFind  -- engine:Obj_misc.obj
//   v12 0x004CCB00 (feb 0x004BCAA0) [exact    ] objFindName  -- engine:Obj_misc.obj
//   v12 0x004CCB80 (feb 0x004BCB20) [callgraph] objFindNameN  -- engine:Obj_misc.obj
//   v12 0x004CCC00 (feb 0x004BCBA0) [callgraph] objFindID  -- engine:Obj_misc.obj
//   v12 0x004CCC70 (feb 0x004BCC10) [callgraph] objFindAnimNmb  -- engine:Obj_misc.obj
//   v12 0x004CCD50 (feb 0x004BCCF0) [exact    ] objMakePtrList  -- engine:Obj_misc.obj
//   v12 0x004CCE10 (feb 0x004BCDB0) [exact    ] objMakePtrList  -- engine:Obj_misc.obj
//   v12 0x004CCEC0 (feb 0x004BCE60) [exact    ] objMakePtrListID  -- engine:Obj_misc.obj
//   v12 0x004CCF40 (feb 0x004BCEE0) [callgraph] objMakeTexList  -- engine:Obj_misc.obj
//   v12 ---------- (feb 0x004BCF40) [none     ] _objCmpFace  -- engine:Obj_misc.obj
//   v12 ---------- (feb 0x004BCFC0) [none     ] _objCmpSplitEntry  -- engine:Obj_misc.obj
//   v12 0x004CD060 (feb 0x004BCFD0) [exact    ] _objBuildObjPtrList  -- engine:Obj_misc.obj
//   v12 0x004CD110 (feb 0x004BD080) [exact    ] _objIncludePtrListID  -- engine:Obj_misc.obj
//   v12 0x004CD190 (feb 0x004BD100) [callgraph] _objIncludeTexList  -- engine:Obj_misc.obj
//   v12 0x004DBF10 (feb 0x004BF1B0) [exact    ] objInit  -- engine:Object.obj
//   v12 ---------- (feb 0x004BF1D0) [none     ] objTerm  -- engine:Object.obj
//   v12 0x004DBF40 (feb 0x004BF1E0) [exact    ] ??0objOBJ@@AAE@XZ  -- engine:Object.obj
//   v12 0x004DBFF0 (feb 0x004BF290) [exact    ] ??1objOBJ@@QAE@XZ  -- engine:Object.obj
//   v12 0x004DC530 (feb 0x004BF7C0) [exact    ] ??4m3dSPHERE@@QAEAAV0@ABV0@@Z  -- engine:Object.obj
//   v12 0x004DC680 (feb 0x004BF910) [exact    ] objCalcLTM  -- engine:Object.obj
//   v12 0x004DC9E0 (feb 0x004BFC70) [callgraph] objCalcBSphereCenter  -- engine:Object.obj
//   v12 0x004DCCE0 (feb 0x004BFF70) [exact    ] objCalcBPrism  -- engine:Object.obj
//   v12 0x004DCD10 (feb 0x004BFFA0) [exact    ] objCalcBBox  -- engine:Object.obj
//   v12 0x004D1730 (feb 0x004C0180) [exact    ] partInit  -- engine:particle.obj
//   v12 0x004D1770 (feb 0x004C01C0) [exact    ] partTerm  -- engine:particle.obj
//   v12 0x004D1790 (feb 0x004C01E0) [exact    ] ??0partEMITTER_PHYS@@QAE@MM@Z  -- engine:particle.obj
//   v12 0x004D17E0 (feb 0x004C0230) [exact    ] ??0partEMITTER_PHYS@@QAE@HMM@Z  -- engine:particle.obj
//   v12 0x004D1B60 (feb 0x004C05B0) [callgraph] ??1partEMITTER_PHYS@@UAE@XZ  -- engine:particle.obj
//   v12 ---------- (feb 0x004C2A90) [none     ] ??0partEMITTER_PHYS_FLY@@QAE@MM@Z  -- engine:particle.obj
//   v12 0x004D41C0 (feb 0x004C51E0) [callgraph] partUpdate  -- engine:Partrend.obj
//   v12 0x004D42A0 (feb 0x004C52C0) [exact    ] partDestroyEmitters  -- engine:Partrend.obj
//   v12 0x004D42D0 (feb 0x004C52F0) [callgraph] partGetNEmit  -- engine:Partrend.obj
//   v12 0x004D42F0 (feb 0x004C5310) [callgraph] partRender  -- engine:Partrend.obj
//   v12 0x004D60B0 (feb 0x004C70D0) [exact    ] _partCompareParticleZ  -- engine:Partrend.obj
//   v12 0x004D60F0 (feb 0x004C7110) [exact    ] pteSpotInit  -- engine:pte_spot.obj
//   v12 0x004D6120 (feb 0x004C7140) [exact    ] ??0pteSPOT@@QAE@H@Z  -- engine:pte_spot.obj
//   v12 0x004D6250 (feb 0x004C7270) [exact    ] ??_GpteSPOT@@UAEPAXI@Z  -- engine:pte_spot.obj
//   v12 0x004D6250 (feb 0x004C7270) [exact    ] ??_EpteSPOT@@UAEPAXI@Z  -- engine:pte_spot.obj
//   v12 0x004D6290 (feb 0x004C72B0) [exact    ] ??1pteSPOT@@UAE@XZ  -- engine:pte_spot.obj
//   v12 0x004D7BB0 (feb 0x004C8BF0) [callgraph] pteSpotAnimInit  -- engine:pte_spot_a.obj
//   v12 0x004D8370 (feb 0x004C93B0) [exact    ] pteInit  -- engine:pte_shader.obj
//   v12 0x004D8420 (feb 0x004C9460) [exact    ] pteTerm  -- engine:pte_shader.obj
//   v12 0x004D8460 (feb 0x004C94A0) [vtable   ] ??_EpteMANAGER@@UAEPAXI@Z  -- engine:pte_shader.obj
//   v12 0x004D8460 (feb 0x004C94A0) [vtable   ] ??_GpteMANAGER@@UAEPAXI@Z  -- engine:pte_shader.obj
//   v12 0x004D8BB0 (feb 0x004C9BF0) [exact    ] ??0pteSHADER@@QAE@XZ  -- engine:pte_shader.obj
//   v12 0x004D8C90 (feb 0x004C9CD0) [exact    ] ??_GpteSHADER@@UAEPAXI@Z  -- engine:pte_shader.obj
//   v12 0x004D8C90 (feb 0x004C9CD0) [exact    ] ??_EpteSHADER@@UAEPAXI@Z  -- engine:pte_shader.obj
//   v12 0x004D8CE0 (feb 0x004C9D20) [exact    ] ??1pteSHADER@@UAE@XZ  -- engine:pte_shader.obj
//   v12 0x004D9B30 (feb 0x004CAB70) [exact    ] _pteSortFunc  -- engine:pte_shader.obj
//   v12 0x004D9B70 (feb 0x004CABB0) [callgraph] partAttachInst  -- engine:partobj.obj
//   v12 0x004D9B80 (feb 0x004CABC0) [callgraph] partDetachInst  -- engine:partobj.obj
//   v12 0x004D9BB0 (feb 0x004CABF0) [callgraph] partIsOwnerInstHidden  -- engine:partobj.obj
//   v12 0x004D9BD0 (feb 0x004CAC10) [callgraph] partIsOwnerInstVisible  -- engine:partobj.obj
//   v12 0x004D9BF0 (feb 0x004CAC30) [exact    ] partSetAreaEmissionObj  -- engine:partobj.obj
//   v12 0x004D9C20 (feb 0x004CAC60) [callgraph] partSetAreaEmissionObjList  -- engine:partobj.obj
//   v12 0x004DCEF0 (feb 0x004CEAA0) [vtable   ] ??_EobjMOD_TEX@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DCEF0 (feb 0x004CEAA0) [vtable   ] ??_EobjMOD_TEX_FRAME_ANIM@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DCEF0 (feb 0x004CEAA0) [vtable   ] ??_EobjMOD_TEX_SCROLL@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DCEF0 (feb 0x004CEAA0) [vtable   ] ??_GobjMOD_TEX_FRAME_ANIM@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DCEF0 (feb 0x004CEAA0) [vtable   ] ??_GobjMOD_TEX@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DCEF0 (feb 0x004CEAA0) [vtable   ] ??_GobjMOD_TEX_SCROLL@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DD040 (feb 0x004CEBF0) [exact    ] ??0objMOD_TEX_SCROLL@@QAE@H@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DD1F0 (feb 0x004CEDA0) [exact    ] ??0objMOD_TEX_FRAME_ANIM@@QAE@H@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DD4F0 (feb 0x004CF0A0) [callgraph] ??0objMOD_TEX_SEQ_ANIM@@QAE@H@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DD540 (feb 0x004CF0E0) [vtable   ] ??_GobjMOD_TEX_SEQ_ANIM@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DD540 (feb 0x004CF0E0) [vtable   ] ??_EobjMOD_TEX_SEQ_ANIM@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DD700 (feb 0x004CF290) [exact    ] ??0objMOD_BLINK_TRANSP@@QAE@XZ  -- engine:obj_mod_tex.obj
//   v12 0x004DD8E0 (feb 0x004CF470) [exact    ] ??0objMOD_NOISE_DEFORM_UV@@QAE@H@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DDC50 (feb 0x004CF7E0) [exact    ] ??_GobjMOD_NOISE_DEFORM_UV@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DDC50 (feb 0x004CF7E0) [exact    ] ??_EobjMOD_NOISE_DEFORM_UV@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DDCC0 (feb 0x004CF850) [exact    ] ??_EobjMOD_BLINK_TRANSP@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DDCC0 (feb 0x004CF850) [exact    ] ??_GobjMOD_BLINK_TRANSP@@UAEPAXI@Z  -- engine:obj_mod_tex.obj
//   v12 0x004DFE10 (feb 0x004D1B90) [exact    ] ??0objMODIFIER@@QAE@KH@Z  -- engine:obj_mod.obj
//   v12 0x004DFE40 (feb 0x004D1BC0) [vtable   ] ??_GobjMODIFIER@@UAEPAXI@Z  -- engine:obj_mod.obj
//   v12 0x004DFE40 (feb 0x004D1BC0) [vtable   ] ??_EobjMODIFIER@@UAEPAXI@Z  -- engine:obj_mod.obj
//   v12 0x004DFE60 (feb 0x004D1BE0) [callgraph] ??1objMODIFIER@@UAE@XZ  -- engine:obj_mod.obj
//   v12 ---------- (feb 0x004D23F0) [none     ] _objCmpObjModifier  -- engine:obj_mod.obj
//   v12 0x004E08E0 (feb 0x004D2650) [exact    ] ??1pteSDR_STATIC_DATA@@QAE@XZ  -- engine:pte_sdr_static.obj
//   v12 0x004E2110 (feb 0x004D3D30) [exact    ] pteEnvMapInit  -- engine:pte_envmap.obj
//   v12 ---------- (feb 0x004D3E60) [none     ] ??0pteEMAP@@QAE@XZ  -- engine:pte_envmap.obj
//   v12 0x004E2740 (feb 0x004D4360) [exact    ] ??0pteEMAP_CHROME@@QAE@XZ  -- engine:pte_envmap.obj
//   v12 0x004E2770 (feb 0x004D4390) [exact    ] ??_GpteEMAP_SPECULAR@@UAEPAXI@Z  -- engine:pte_envmap.obj
//   v12 0x004E2770 (feb 0x004D4390) [exact    ] ??_EpteEMAP_SPECULAR@@UAEPAXI@Z  -- engine:pte_envmap.obj
//   v12 0x004E2FB0 (feb 0x004D4BD0) [exact    ] ??0pteEMAP_REFLECT@@QAE@XZ  -- engine:pte_envmap.obj
//   v12 0x004E2FF0 (feb 0x004D4C10) [exact    ] ??_EpteEMAP_REFLECT_WATER@@UAEPAXI@Z  -- engine:pte_envmap.obj
//   v12 0x004E2FF0 (feb 0x004D4C10) [exact    ] ??_EpteEMAP_REFLECT@@UAEPAXI@Z  -- engine:pte_envmap.obj
//   v12 0x004E2FF0 (feb 0x004D4C10) [exact    ] ??_GpteEMAP_REFLECT_WATER@@UAEPAXI@Z  -- engine:pte_envmap.obj
//   v12 0x004E2FF0 (feb 0x004D4C10) [exact    ] ??_GpteEMAP_REFLECT@@UAEPAXI@Z  -- engine:pte_envmap.obj
//   v12 0x004E3020 (feb 0x004D4C40) [exact    ] ??_GpteEMAP_REFRACT@@UAEPAXI@Z  -- engine:pte_envmap.obj
//   v12 0x004E3020 (feb 0x004D4C40) [exact    ] ??_EpteEMAP_REFRACT@@UAEPAXI@Z  -- engine:pte_envmap.obj
//   v12 0x004E54C0 (feb 0x004D70E0) [exact    ] ??0pteTEX2@@QAE@XZ  -- engine:pte_tex2.obj
//   v12 0x004E5510 (feb 0x004D7130) [exact    ] ??_EpteTEX2@@UAEPAXI@Z  -- engine:pte_tex2.obj
//   v12 0x004E5510 (feb 0x004D7130) [exact    ] ??_GpteTEX2@@UAEPAXI@Z  -- engine:pte_tex2.obj
//   v12 0x004EA350 (feb 0x004DA690) [exact    ] ??0rendDRIVER@@QAE@XZ  -- engine:Rend_ini.obj
//   v12 0x004EA450 (feb 0x004DA790) [exact    ] ??_GrendDRIVER@@UAEPAXI@Z  -- engine:Rend_ini.obj
//   v12 0x004EA450 (feb 0x004DA790) [exact    ] ??_ErendDRIVER@@UAEPAXI@Z  -- engine:Rend_ini.obj
//   v12 0x004EA4D0 (feb 0x004DA810) [exact    ] ??1rendDRIVER@@UAE@XZ  -- engine:Rend_ini.obj
//   v12 ---------- (feb 0x004DB000) [none     ] ??1rnsSIGNATURE@@UAE@XZ  -- engine:Rend_ini.obj
//   v12 ---------- (feb 0x004DB010) [none     ] ??1rnsBUF_TRANSP_TYPE@@UAE@XZ  -- engine:Rend_ini.obj
//   v12 0x004EACF0 (feb 0x004DB030) [exact    ] ??_ErnsBUF_TRANSP_TYPE@@UAEPAXI@Z  -- engine:Rend_ini.obj
//   v12 0x004EACF0 (feb 0x004DB030) [exact    ] ??_GrnsBUF_TRANSP_TYPE@@UAEPAXI@Z  -- engine:Rend_ini.obj
//   v12 0x004EAD20 (feb 0x004DB060) [exact    ] ??0rnsBUF_OPAQUE@@QAE@XZ  -- engine:Rend_ini.obj
//   v12 0x004EAD70 (feb 0x004DB090) [exact    ] ??0rnsBUF_TRANSP@@QAE@XZ  -- engine:Rend_ini.obj
//   v12 0x004E5E10 (feb 0x004DD520) [exact    ] ??0scnSCENE@@QAE@XZ  -- engine:Scene.obj
//   v12 0x004E5F60 (feb 0x004DD670) [exact    ] ??1scnSCENE@@QAE@XZ  -- engine:Scene.obj
//   v12 0x004E7470 (feb 0x004DEB30) [callgraph] _scnCalcRadRenderOff  -- engine:Scene.obj
//   v12 0x004E74C0 (feb 0x004DEB60) [exact    ] ??0cdtCOLL_ANIM@@QAE@XZ  -- engine:Scene.obj
//   v12 0x004E7550 (feb 0x004DEBF0) [exact    ] ??_G?$lstNODE_CACHE@VcdtCOLL_ANIM_NODE@@@@UAEPAXI@Z  -- engine:Scene.obj
//   v12 0x004E7550 (feb 0x004DEBF0) [exact    ] ??_E?$lstNODE_CACHE@VcdtCOLL_ANIM_NODE@@@@UAEPAXI@Z  -- engine:Scene.obj
//   v12 0x004E7580 (feb 0x004DEC20) [exact    ] ?Init@?$lstNODE_CACHE@VcdtCOLL_ANIM_NODE@@@@UAEHH@Z  -- engine:Scene.obj
//   v12 0x004E7610 (feb 0x004DECB0) [exact    ] ?Term@?$lstNODE_CACHE@VcdtCOLL_ANIM_NODE@@@@UAEXXZ  -- engine:Scene.obj
//   v12 0x004E7640 (feb 0x004DECE0) [vtable   ] ??_GcdtCOLL_ANIM@@UAEPAXI@Z  -- engine:Scene.obj
//   v12 0x004E7640 (feb 0x004DECE0) [vtable   ] ??_EcdtCOLL_ANIM@@UAEPAXI@Z  -- engine:Scene.obj
//   v12 0x004E7660 (feb 0x004DED00) [vtable   ] ??_EcdtCOLL_ANIM_SCN@@UAEPAXI@Z  -- engine:Scene.obj
//   v12 0x004E7660 (feb 0x004DED00) [vtable   ] ??_GcdtCOLL_ANIM_SCN@@UAEPAXI@Z  -- engine:Scene.obj
//   v12 0x004E7480 (feb 0x004DEB40) [exact    ] _scnDumpInstList  -- engine:Scene.obj
//   v12 ---------- (feb 0x004DF430) [none     ] objBuildLOD  -- engine:lod.obj
//   v12 0x004F3A00 (feb 0x004DF7C0) [exact    ] objInitLOD  -- engine:lod.obj
//   v12 0x004F4240 (feb 0x004E0000) [vtable   ] ??_GobjSET@@UAEPAXI@Z  -- engine:lod.obj
//   v12 0x004F4240 (feb 0x004E0000) [vtable   ] ??_EobjSET@@UAEPAXI@Z  -- engine:lod.obj
//   v12 ---------- (feb 0x004DF880) [none     ] _lodCmpObjNVert  -- engine:lod.obj
//   v12 ---------- (feb 0x004DF8B0) [none     ] _lodCmpLODObj  -- engine:lod.obj
//   v12 ---------- (feb 0x004DF8E0) [none     ] _lodCmpLODSel  -- engine:lod.obj
//   v12 0x004F5BE0 (feb 0x004E19A0) [exact    ] _psStripSpaces  -- engine:pssheet.obj
//   v12 0x004F6E90 (feb 0x004E2C40) [exact    ] ??0objMOD_GEOM@@QAE@KH@Z  -- engine:obj_mod_geom.obj
//   v12 0x004F6EC0 (feb 0x004E2C70) [exact    ] ??_GobjMOD_GEOM@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004F6EC0 (feb 0x004E2C70) [exact    ] ??_EobjMOD_GEOM@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004F6F00 (feb 0x004E2CB0) [exact    ] ??1objMOD_GEOM@@UAE@XZ  -- engine:obj_mod_geom.obj
//   v12 0x004ECB80 (feb 0x004E2FC0) [callgraph] ??0objMOD_SKIN@@QAE@XZ  -- engine:obj_mod_geom.obj
//   v12 0x004F7210 (feb 0x004E4C40) [exact    ] ??0objMOD_NOISE_DEFORM@@QAE@XZ  -- engine:obj_mod_geom.obj
//   v12 0x004EF840 (feb 0x004E5090) [vtable   ] ??_GcgmSEARCH_VLIST@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004EF840 (feb 0x004E5090) [vtable   ] ??_EcgmSEARCH_VLIST@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004EF840 (feb 0x004E5090) [vtable   ] ??_EcgmSEARCH_INT_LIST@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004EF840 (feb 0x004E5090) [vtable   ] ??_GcgmSEARCH@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004EF840 (feb 0x004E5090) [vtable   ] ??_GcgmSEARCH_INT_LIST@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004EF840 (feb 0x004E5090) [vtable   ] ??_EcgmSEARCH@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004EF870 (feb 0x004E50C0) [vtable   ] ??_GobjMOD_SKIN@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004EF870 (feb 0x004E50C0) [vtable   ] ??_EobjMOD_SKIN@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004F76A0 (feb 0x004E51B0) [exact    ] ??_EobjMOD_NOISE_DEFORM@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004F76A0 (feb 0x004E51B0) [exact    ] ??_GobjMOD_NOISE_DEFORM@@UAEPAXI@Z  -- engine:obj_mod_geom.obj
//   v12 0x004F7FC0 (feb 0x004E5AB0) [exact    ] pctInit  -- engine:picture.obj
//   v12 ---------- (feb 0x004E6120) [none     ] pctTerm  -- engine:picture.obj
//   v12 0x004F8660 (feb 0x004E6150) [exact    ] pctReadHdr  -- engine:picture.obj
//   v12 0x004F86A0 (feb 0x004E6190) [callgraph] pctRead  -- engine:picture.obj
//   v12 0x004F86F0 (feb 0x004E61E0) [callgraph] pctRead  -- engine:picture.obj
//   v12 0x004F8740 (feb 0x004E6230) [exact    ] pctRead  -- engine:picture.obj
//   v12 0x004F8810 (feb 0x004E6300) [callgraph] pctRead  -- engine:picture.obj
//   v12 0x004F8B40 (feb 0x004E6630) [exact    ] pctWrite  -- engine:picture.obj
//   v12 0x004F8BC0 (feb 0x004E66B0) [callgraph] pctWrite  -- engine:picture.obj
//   v12 0x004F8E80 (feb 0x004E6970) [callgraph] pctCreate  -- engine:picture.obj
//   v12 0x004F8FD0 (feb 0x004E6AC0) [exact    ] pctDestroy  -- engine:picture.obj
//   v12 0x004F8FF0 (feb 0x004E6AE0) [callgraph] pctIsAlpha  -- engine:picture.obj
//   v12 0x004F9030 (feb 0x004E6B20) [callgraph] pctConvert  -- engine:picture.obj
//   v12 0x004F9620 (feb 0x004E7110) [exact    ] pctSetPixel  -- engine:picture.obj
//   v12 0x004F9870 (feb 0x004E7360) [callgraph] pctGetPixel  -- engine:picture.obj
//   v12 0x004F9B10 (feb 0x004E7600) [callgraph] pctGetBPP  -- engine:picture.obj
//   v12 0x004F9B30 (feb 0x004E7620) [callgraph] pctGetFormat  -- engine:picture.obj
//   v12 0x004F9B50 (feb 0x004E7640) [callgraph] pctGetFormat  -- engine:picture.obj
//   v12 0x004F9B70 (feb 0x004E7660) [callgraph] pctGetFormat  -- engine:picture.obj
//   v12 0x004F9B90 (feb 0x004E7680) [callgraph] pctGetMipMapMemSize  -- engine:picture.obj
//   v12 0x004F9C50 (feb 0x004E7740) [callgraph] pctGetMemSize  -- engine:picture.obj
//   v12 0x004F9D60 (feb 0x004E7850) [callgraph] pctCopyViaUnpackDXTBuf  -- engine:picture.obj
//   v12 0x004FA270 (feb 0x004E7D60) [callgraph] pctCopyDXTBuf  -- engine:picture.obj
//   v12 0x004FA350 (feb 0x004E7E40) [callgraph] pctCopyBuf  -- engine:picture.obj
//   v12 0x004FA980 (feb 0x004E8470) [callgraph] pctGetFromBuf  -- engine:picture.obj
//   v12 0x004FAEF0 (feb 0x004E89E0) [callgraph] pctGenMipMaps  -- engine:picture.obj
//   v12 0x004FB2B0 (feb 0x004E8DA0) [exact    ] pctGenMipMapLevel  -- engine:picture.obj
//   v12 0x004FB650 (feb 0x004E9140) [exact    ] pctGetMaxMipMapCount  -- engine:picture.obj
//   v12 0x004FB6A0 (feb 0x004E9190) [callgraph] pctResample  -- engine:picture.obj
//   v12 0x004FB900 (feb 0x004E93F0) [exact    ] pctCalcAveragePixel  -- engine:picture.obj
//   v12 0x00501050 (feb 0x004EEF80) [exact    ] ??0objMTL_CTRL_TBL@@QAE@XZ  -- engine:obj_split.obj
//   v12 0x00504F00 (feb 0x004F28F0) [callgraph] ??0objMTL@@QAE@XZ  -- engine:Rend_sort.obj
//   v12 0x00504EA0 (feb 0x004F2890) [exact    ] _rnsCompObjZ  -- engine:Rend_sort.obj
//   v12 0x00504EC0 (feb 0x004F28B0) [exact    ] _rnsComparePolyZ  -- engine:Rend_sort.obj
//   v12 0x00504F20 (feb 0x004F2910) [exact    ] pctReadBMPHdr  -- engine:pic_bmp.obj
//   v12 0x00504FA0 (feb 0x004F2990) [exact    ] pctReadBMP  -- engine:pic_bmp.obj
//   v12 0x005050C0 (feb 0x004F2AB0) [exact    ] pctSaveBMP  -- engine:pic_bmp.obj
//   v12 0x005052A0 (feb 0x004F2C90) [exact    ] pctReadTGAHdr  -- engine:pic_tga.obj
//   v12 0x00505340 (feb 0x004F2D30) [exact    ] pctReadTGA  -- engine:pic_tga.obj
//   v12 0x00505740 (feb 0x004F3130) [exact    ] pctReadPICHdr  -- engine:pic_pic.obj
//   v12 ---------- (feb 0x004F31F0) [none     ] pctReadPIC  -- engine:pic_pic.obj
//   v12 0x00505970 (feb 0x004F3360) [exact    ] _picReadHeader  -- engine:pic_pic.obj
//   v12 0x00505B30 (feb 0x004F3520) [exact    ] _picReadLineCompressed  -- engine:pic_pic.obj
//   v12 0x00505DD0 (feb 0x004F37C0) [callgraph] pctGetDXTPixel  -- engine:pic_decompress.obj
//   v12 0x00506090 (feb 0x004F3A80) [exact    ] pctLineCombColor565  -- engine:pic_decompress.obj
//   v12 0x00506120 (feb 0x004F3B10) [exact    ] UpdateCache1  -- engine:pic_decompress.obj
//   v12 0x00506250 (feb 0x004F3C40) [callgraph] pctGetPairByCode  -- engine:pic_decompress.obj
//   v12 0x00506300 (feb 0x004F3CF0) [exact    ] UpdateCache5  -- engine:pic_decompress.obj
//   v12 0x00506460 (feb 0x004F3E50) [exact    ] pctDecode  -- engine:pic_decompress.obj
//   v12 ---------- (feb 0x0057E690) [none     ] pteFogInit  -- engine:pte_fog.obj
//   v12 0x0059A1C0 (feb 0x0057E7A0) [exact    ] ??0pteFOG_GROUND@@QAE@PAVobjOBJ@@@Z  -- engine:pte_fog.obj
//   v12 0x0059A280 (feb 0x0057E860) [exact    ] ??_GpteFOG_GROUND@@UAEPAXI@Z  -- engine:pte_fog.obj
//   v12 0x0059A280 (feb 0x0057E860) [exact    ] ??_EpteFOG_GROUND@@UAEPAXI@Z  -- engine:pte_fog.obj
//   v12 0x0059B440 (feb 0x0057FA20) [exact    ] fntInit  -- engine:font.obj
//   v12 ---------- (feb 0x0057FAE0) [none     ] fntTerm  -- engine:font.obj
//   v12 0x0059B520 (feb 0x0057FB00) [vtable   ] ??_EfntMANAGER@@UAEPAXI@Z  -- engine:font.obj
//   v12 0x0059B520 (feb 0x0057FB00) [vtable   ] ??_GfntMANAGER@@UAEPAXI@Z  -- engine:font.obj
//   v12 0x0059E670 (feb 0x00582340) [callgraph] sioSetSourceFile  -- engine:sceneio.obj
//   v12 0x0059E680 (feb 0x00582350) [callgraph] sioReadScene  -- engine:sceneio.obj
//   v12 0x0059E840 (feb 0x00582510) [exact    ] sioReadSceneTex  -- engine:sceneio.obj
//   v12 0x0059E900 (feb 0x005825D0) [exact    ] sioReadTemplate  -- engine:sceneio.obj
//   v12 0x0059EA80 (feb 0x00582750) [exact    ] _sioReadLastChunk  -- engine:sceneio_misc.obj
//   v12 0x0059EAB0 (feb 0x00582780) [exact    ] _sioReadTex  -- engine:sceneio_misc.obj
//   v12 0x0059ED10 (feb 0x005829E0) [exact    ] _sioReadPSheet  -- engine:sceneio_misc.obj
//   v12 0x0059EDD0 (feb 0x00582AA0) [exact    ] _sioReadNamedSel  -- engine:sceneio_misc.obj
//   v12 0x0059EF00 (feb 0x00582BD0) [exact    ] _sioReadNamedSelList  -- engine:sceneio_misc.obj
//   v12 0x0059EFF0 (feb 0x00582CC0) [callgraph] _sioReadCamera  -- engine:sceneio_misc.obj
//   v12 0x0059F1C0 (feb 0x00582E90) [exact    ] _sioReadInstList  -- engine:sceneio_scn.obj
//   v12 0x0059F350 (feb 0x00583020) [exact    ] _sioReadTplList  -- engine:sceneio_scn.obj
//   v12 0x0059F3E0 (feb 0x005830B0) [callgraph] _sioReadLightList  -- engine:sceneio_scn.obj
//   v12 0x0059F790 (feb 0x00583460) [callgraph] _sioReadColl  -- engine:sceneio_scn.obj
//   v12 0x0059F9D0 (feb 0x005836A0) [callgraph] _sioReadColl_OLD  -- engine:sceneio_scn.obj
//   v12 0x0059FB80 (feb 0x00583850) [callgraph] ??0cdtCOLL_OBJ@@QAE@XZ  -- engine:sceneio_scn.obj
//   v12 0x0059FBA0 (feb 0x00583870) [callgraph] _sioReadObj  -- engine:sceneio_obj.obj
//   v12 0x005A0BD0 (feb 0x00584710) [exact    ] _sioReadAnimSeq  -- engine:sceneio_tpl.obj
//   v12 0x005A0E20 (feb 0x00584960) [callgraph] _sioReadObjAnimList  -- engine:sceneio_tpl.obj
//   v12 0x005A1200 (feb 0x00584CE0) [callgraph] _sioReadBVolList  -- engine:sceneio_tpl.obj
//   v12 0x005A1440 (feb 0x00584F20) [callgraph] _sioReadObjSkinList  -- engine:sceneio_tpl.obj

// ---- txmTEXTURE
//   v12 0x004B1F40 (feb 0x004A32E0) [exact    ] txmTEXTURE::FindDisk  -- drv:txm.obj
//   v12 0x004B1F90 (feb 0x004A3330) [exact    ] txmTEXTURE::Create  -- drv:txm.obj
//   v12 0x004B2070 (feb 0x004A3410) [exact    ] txmTEXTURE::Set  -- drv:txm.obj
//   v12 0x004B20C0 (feb 0x004A3460) [exact    ] txmTEXTURE::Load  -- drv:txm.obj
//   v12 0x004B2140 (feb 0x004A34E0) [vtable   ] txmTEXTURE::Unload  -- drv:txm.obj
//   v12 0x004B2160 (feb 0x004A3500) [exact    ] txmTEXTURE::Fill  -- drv:txm.obj
//   v12 0x004B21C0 (feb 0x004A3560) [exact    ] txmTEXTURE::GetColorUV  -- drv:txm.obj
//   v12 0x004B2700 (feb 0x004A3AA0) [exact    ] txmTEXTURE::ReadResample  -- drv:txm.obj
//   v12 0x004B28A0 (feb 0x004A3C40) [exact    ] txmTEXTURE::SetTexParams  -- drv:txm.obj
//   v12 0x004B39C0 (feb 0x004A4D60) [vtable   ] txmTEXTURE::SetColor  -- drv:txm.obj
//   v12 0x004B39C0 (feb 0x004A4D60) [vtable   ] txmTEXTURE::GetColor  -- drv:txm.obj

// ---- txmLAYER
//   v12 0x004B2930 (feb 0x004A3CD0) [exact    ] txmLAYER::SetWeight  -- drv:txm.obj

// ---- txmTEXTURE_LAY
//   v12 0x004B2AB0 (feb 0x004A3E50) [exact    ] txmTEXTURE_LAY::FindDisk  -- drv:txm.obj
//   v12 0x004B2B10 (feb 0x004A3EB0) [vtable   ] txmTEXTURE_LAY::Create  -- drv:txm.obj
//   v12 0x004B2B20 (feb 0x004A3EC0) [exact    ] txmTEXTURE_LAY::Set  -- drv:txm.obj
//   v12 0x004B2BD0 (feb 0x004A3F70) [exact    ] txmTEXTURE_LAY::Load  -- drv:txm.obj
//   v12 0x004B2E10 (feb 0x004A41B0) [exact    ] txmTEXTURE_LAY::Save  -- drv:txm.obj
//   v12 0x004B2EE0 (feb 0x004A4280) [exact    ] txmTEXTURE_LAY::Unload  -- drv:txm.obj
//   v12 0x004B2F30 (feb 0x004A42D0) [vtable   ] txmTEXTURE_LAY::GetColor  -- drv:txm.obj
//   v12 0x004B2FB0 (feb 0x004A4350) [vtable   ] txmTEXTURE_LAY::SetColor  -- drv:txm.obj
//   v12 0x004B3030 (feb 0x004A43D0) [exact    ] txmTEXTURE_LAY::Add  -- drv:txm.obj
//   v12 0x004B3120 (feb 0x004A44C0) [vtable   ] txmTEXTURE_LAY::Update  -- drv:txm.obj
//   v12 0x004EADB0 (feb 0x004DB0D0) [vtable   ] txmTEXTURE_LAY::BlendDrv  -- engine:Rend_ini.obj

// ---- txmMANAGER
//   v12 0x004B3220 (feb 0x004A45C0) [exact    ] txmMANAGER::Init  -- drv:txm.obj
//   v12 0x004B3410 (feb 0x004A47B0) [exact    ] txmMANAGER::Add  -- drv:txm.obj
//   v12 0x004B35C0 (feb 0x004A4960) [exact    ] txmMANAGER::Del  -- drv:txm.obj
//   v12 0x004B35F0 (feb 0x004A4990) [exact    ] txmMANAGER::Find  -- drv:txm.obj
//   v12 0x004B3640 (feb 0x004A49E0) [exact    ] txmMANAGER::Alloc  -- drv:txm.obj
//   v12 0x004B36D0 (feb 0x004A4A70) [exact    ] txmMANAGER::Load  -- drv:txm.obj
//   v12 0x004B37C0 (feb 0x004A4B60) [exact    ] txmMANAGER::LoadAll  -- drv:txm.obj
//   v12 0x004B3830 (feb 0x004A4BD0) [exact    ] txmMANAGER::PurgeAll  -- drv:txm.obj
//   v12 0x004B38D0 (feb 0x004A4C70) [exact    ] txmMANAGER::MngTexState  -- drv:txm.obj
//   v12 0x004B3930 (feb 0x004A4CD0) [exact    ] txmMANAGER::GetNext  -- drv:txm.obj
//   v12 0x004B3970 (feb 0x004A4D10) [callgraph] txmMANAGER::GetListNmb  -- drv:txm.obj
//   v12 0x004B3980 (feb 0x004A4D20) [exact    ] txmMANAGER::NewFrame  -- drv:txm.obj
//   v12 0x004B39D0 (feb 0x004A4D70) [exact    ] txmMANAGER::MakeTex  -- drv:txm.obj
//   v12 0x004B3A40 (feb 0x004A4DE0) [exact    ] txmMANAGER::MakeTexLayer  -- drv:txm.obj

// ---- vidDRIVER
//   v12 0x004B3DD0 (feb 0x004A5170) [exact    ] vidDRIVER::AddMode  -- drv:vid.obj
//   v12 0x004B3ED0 (feb 0x004A5270) [exact    ] vidDRIVER::FindMode  -- drv:vid.obj
//   v12 0x004B3F30 (feb 0x004A52D0) [callgraph] vidDRIVER::InitDrv  -- drv:vid.obj
//   v12 0x004B3F40 (feb 0x004A52E0) [vtable   ] vidDRIVER::TermDrv  -- drv:vid.obj
//   v12 0x004B3F50 (feb 0x004A52F0) [exact    ] vidDRIVER::SetMode  -- drv:vid.obj
//   v12 0x004B4020 (feb 0x004A53C0) [vtable   ] vidDRIVER::SetNTexStageUse  -- drv:vid.obj
//   v12 0x004B4040 (feb 0x004A53E0) [vtable   ] vidDRIVER::InitHW  -- drv:vid.obj
//   v12 0x004B4060 (feb 0x004A5400) [callgraph] vidDRIVER::TermHW  -- drv:vid.obj
//   v12 0x004B4070 (feb 0x004A5410) [vtable   ] vidDRIVER::BeginScene  -- drv:vid.obj
//   v12 0x004B4080 (feb 0x004A5420) [callgraph] vidDRIVER::ToggleModeParam  -- drv:vid.obj
//   v12 0x004B4660 (feb 0x004A5A00) [callgraph] vidDRIVER::SetNearFarZValue  -- drv:vid.obj
//   v12 0x004B4680 (feb 0x004A5A20) [exact    ] vidDRIVER::GetNearFarZValue  -- drv:vid.obj
//   v12 0x004B46B0 (feb 0x004A5A50) [vtable   ] vidDRIVER::SetCamNearPlane  -- drv:vid.obj
//   v12 0x004B46C0 (feb 0x004A5A60) [vtable   ] vidDRIVER::GetGamma  -- drv:vid.obj
//   v12 0x004B46E0 (feb 0x004A5A80) [exact    ] vidDRIVER::SetGamma  -- drv:vid.obj

// ---- d3dDRIVER
//   v12 0x004BAE10 (feb 0x004AB5B0) [exact    ] d3dDRIVER::InitDrv  -- drv:D3d_drv.obj
//   v12 0x004BAEA0 (feb 0x004AB640) [exact    ] d3dDRIVER::TermDrv  -- drv:D3d_drv.obj
//   v12 0x004BAEE0 (feb 0x004AB680) [exact    ] d3dDRIVER::AreCompressedFormatsAvialiable  -- drv:D3d_drv.obj
//   v12 0x004BAF50 (feb 0x004AB6F0) [exact    ] d3dDRIVER::CreateTexManager  -- drv:D3d_drv.obj
//   v12 0x004BAF90 (feb 0x004AB730) [vtable   ] d3dDRIVER::InitHW  -- drv:D3d_drv.obj
//   v12 0x004BB6C0 (feb 0x004ABE60) [exact    ] d3dDRIVER::TermHW  -- drv:D3d_drv.obj
//   v12 0x004BB790 (feb 0x004ABF30) [vtable   ] d3dDRIVER::BeginScene  -- drv:D3d_drv.obj
//   v12 0x004BB7D0 (feb 0x004ABF60) [vtable   ] d3dDRIVER::EndScene  -- drv:D3d_drv.obj
//   v12 0x004BB820 (feb 0x004ABF90) [exact    ] d3dDRIVER::SwapBuffers  -- drv:D3d_drv.obj
//   v12 0x004BBA00 (feb 0x004AC170) [exact    ] d3dDRIVER::ClearBuffers  -- drv:D3d_drv.obj
//   v12 0x004BBB10 (feb 0x004AC280) [exact    ] d3dDRIVER::ReadBuffer  -- drv:D3d_drv.obj
//   v12 0x00522F80 (feb 0x004AC330) [vtable   ] d3dDRIVER::WriteBuffer  -- drv:D3d_drv.obj
//   v12 0x004BBBC0 (feb 0x004AC340) [exact    ] d3dDRIVER::CopyBuffer  -- drv:D3d_drv.obj
//   v12 0x004BBCC0 (feb 0x004AC440) [exact    ] d3dDRIVER::ClearSurfaces  -- drv:D3d_drv.obj
//   v12 0x004BBDC0 (feb 0x004AC540) [exact    ] d3dDRIVER::RestoreSurfaces  -- drv:D3d_drv.obj
//   v12 0x004BBE50 (feb 0x004AC5D0) [vtable   ] d3dDRIVER::InvalidateSignatureCFG  -- drv:D3d_drv.obj
//   v12 0x004BBED0 (feb 0x004AC650) [exact    ] d3dDRIVER::CreateRendDriver  -- drv:D3d_rnd_misc.obj
//   v12 0x004BBF00 (feb 0x004AC680) [vtable   ] d3dDRIVER::DestroyRendDriver  -- drv:D3d_rnd_misc.obj
//   v12 0x004BC300 (feb 0x004ACA60) [callgraph] d3dDRIVER::ConfigureAlphaBlend  -- drv:d3d_cfg.obj
//   v12 0x004BC5F0 (feb 0x004ACD50) [callgraph] d3dDRIVER::ConfigureTexBlend  -- drv:d3d_cfg.obj
//   v12 0x004BC7D0 (feb 0x004ACF30) [exact    ] d3dDRIVER::ConfigureStageTcHwID  -- drv:d3d_cfg.obj
//   v12 0x004BC8A0 (feb 0x004AD000) [callgraph] d3dDRIVER::ConfigureStageBlend  -- drv:d3d_cfg.obj
//   v12 0x004BE140 (feb 0x004AE8A0) [vtable   ] d3dDRIVER::ConfigureStencil  -- drv:d3d_cfg.obj
//   v12 0x004BE520 (feb 0x004AEC80) [exact    ] d3dDRIVER::ToggleStencil  -- drv:d3d_cfg.obj
//   v12 0x004BE590 (feb 0x004AECF0) [exact    ] d3dDRIVER::SetNearFarZValue  -- drv:d3d_cfg.obj
//   v12 0x004BE5F0 (feb 0x004AED50) [exact    ] d3dDRIVER::SetGamma  -- drv:d3d_cfg.obj
//   v12 0x004BE710 (feb 0x004AEE70) [exact    ] d3dDRIVER::ConfigureFog  -- drv:d3d_cfg.obj
//   v12 0x004BE8F0 (feb 0x004AF050) [exact    ] d3dDRIVER::ToggleFiltration  -- drv:d3d_cfg.obj
//   v12 0x004BE8F0 (feb 0x004AF050) [exact    ] d3dDRIVER::ToggleMipMap  -- drv:d3d_cfg.obj
//   v12 0x004BE8F0 (feb 0x004AF050) [exact    ] d3dDRIVER::ToggleMipMapFiltr  -- drv:d3d_cfg.obj
//   v12 0x004BE920 (feb 0x004AF080) [exact    ] d3dDRIVER::ToggleFog  -- drv:d3d_cfg.obj
//   v12 0x004BE990 (feb 0x004AF0F0) [exact    ] d3dDRIVER::ToggleZCompare  -- drv:d3d_cfg.obj
//   v12 0x004BE9F0 (feb 0x004AF150) [exact    ] d3dDRIVER::ToggleZWrite  -- drv:d3d_cfg.obj
//   v12 0x004BEA50 (feb 0x004AF1B0) [exact    ] d3dDRIVER::ToggleWBuffer  -- drv:d3d_cfg.obj
//   v12 0x004BEAD0 (feb 0x004AF230) [exact    ] d3dDRIVER::ToggleFSAA  -- drv:d3d_cfg.obj
//   v12 0x004BEB40 (feb 0x004AF2A0) [exact    ] d3dDRIVER::ConfigureFiltration  -- drv:d3d_cfg.obj
//   v12 0x004BED30 (feb 0x004AF490) [exact    ] d3dDRIVER::SetStartRenderingState  -- drv:d3d_cfg.obj
//   v12 0x004C98C0 (feb 0x004B9A20) [exact    ] d3dDRIVER::DrawIndexedPrimitiveVB  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9910 (feb 0x004B9A70) [exact    ] d3dDRIVER::DrawPrimitiveVB  -- drv:D3d_rnd_vbuf.obj

// ---- netDRV
//   v12 0x00522F80 (feb 0x004AC330) [vtable   ] netDRV::IsServersEqual  -- drv:D3d_drv.obj

// ---- shtQUALIFY_PLANE
//   v12 0x00522F80 (feb 0x004AC330) [vtable   ] shtQUALIFY_PLANE::ProcessNode  -- drv:D3d_drv.obj

// ---- txmD3D_MANAGER
//   v12 0x004BBE60 (feb 0x004AC5E0) [vtable   ] txmD3D_MANAGER::MakeTex  -- drv:D3d_drv.obj
//   v12 0x004BBE80 (feb 0x004AC600) [vtable   ] txmD3D_MANAGER::MakeTexLayer  -- drv:D3d_drv.obj
//   v12 0x004C1540 (feb 0x004B1CA0) [vtable   ] txmD3D_MANAGER::NewFrame  -- drv:d3d_txm.obj
//   v12 0x004C1560 (feb 0x004B1CC0) [exact    ] txmD3D_MANAGER::SetSource  -- drv:d3d_txm.obj
//   v12 0x004C1780 (feb 0x004B1EE0) [exact    ] txmD3D_MANAGER::HwInit  -- drv:d3d_txm.obj
//   v12 0x004C1800 (feb 0x004B1F60) [exact    ] txmD3D_MANAGER::HwTerm  -- drv:d3d_txm.obj
//   v12 0x004C1830 (feb 0x004B1F90) [vtable   ] txmD3D_MANAGER::HwDownload  -- drv:d3d_txm.obj
//   v12 0x004C1940 (feb 0x004B2090) [exact    ] txmD3D_MANAGER::HwDelete  -- drv:d3d_txm.obj
//   v12 0x004C19A0 (feb 0x004B20F0) [exact    ] txmD3D_MANAGER::HwFindBestUnload  -- drv:d3d_txm.obj
//   v12 0x004C1AD0 (feb 0x004B2220) [callgraph] txmD3D_MANAGER::HwDestroyLost  -- drv:d3d_txm.obj

// ---- d3dREND_DRIVER
//   v12 0x004BBF20 (feb 0x004AC6A0) [exact    ] d3dREND_DRIVER::FlushHWQueue  -- drv:D3d_rnd_misc.obj
//   v12 0x004BBF70 (feb 0x004AC6E0) [exact    ] d3dREND_DRIVER::FilterInst  -- drv:D3d_rnd_misc.obj
//   v12 0x004BBFC0 (feb 0x004AC730) [vtable   ] d3dREND_DRIVER::DrawPoly  -- drv:D3d_rnd_misc.obj
//   v12 0x004BC180 (feb 0x004AC8E0) [exact    ] d3dREND_DRIVER::DrawLine  -- drv:D3d_rnd_misc.obj
//   v12 0x004BF160 (feb 0x004AF8C0) [exact    ] d3dREND_DRIVER::ConfigureClipPlane  -- drv:d3d_cfg.obj
//   v12 0x004BF210 (feb 0x004AF970) [exact    ] d3dREND_DRIVER::EnableClipPlane  -- drv:d3d_cfg.obj
//   v12 0x004BF340 (feb 0x004AFAA0) [exact    ] d3dREND_DRIVER::SetMtlCoeff  -- drv:d3d_cfg.obj
//   v12 0x004BF410 (feb 0x004AFB70) [exact    ] d3dREND_DRIVER::SetLight  -- drv:d3d_cfg.obj
//   v12 0x004BF4B0 (feb 0x004AFC10) [exact    ] d3dREND_DRIVER::EnableLight  -- drv:d3d_cfg.obj
//   v12 0x004BF4E0 (feb 0x004AFC40) [exact    ] d3dREND_DRIVER::BeginCamera  -- drv:d3d_cfg.obj
//   v12 0x004BF810 (feb 0x004AFF70) [vtable   ] d3dREND_DRIVER::EndCamera  -- drv:d3d_cfg.obj
//   v12 0x004BF820 (feb 0x004AFF80) [exact    ] d3dREND_DRIVER::BeginRasterizeObj  -- drv:d3d_cfg.obj
//   v12 0x004BF920 (feb 0x004B0080) [exact    ] d3dREND_DRIVER::StartUseHTL  -- drv:d3d_cfg.obj
//   v12 0x004BF950 (feb 0x004B00B0) [exact    ] d3dREND_DRIVER::StopUseHTL  -- drv:d3d_cfg.obj
//   v12 0x004BF9B0 (feb 0x004B0110) [exact    ] d3dREND_DRIVER::ConvertMatr  -- drv:d3d_cfg.obj
//   v12 0x004BFA20 (feb 0x004B0180) [callgraph] d3dREND_DRIVER::MakeProjMatrix  -- drv:d3d_cfg.obj
//   v12 0x004C1EF0 (feb 0x004B25C0) [vtable   ] d3dREND_DRIVER::InitHW  -- drv:D3d_rnd_idx.obj
//   v12 0x004C1FF0 (feb 0x004B2810) [vtable   ] d3dREND_DRIVER::TermHW  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2180 (feb 0x004B2820) [exact    ] d3dREND_DRIVER::BeginFrame  -- drv:D3d_rnd_idx.obj
//   v12 0x004C21D0 (feb 0x004B2870) [vtable   ] d3dREND_DRIVER::EndFrame  -- drv:D3d_rnd_idx.obj
//   v12 0x004C21E0 (feb 0x004B2880) [callgraph] d3dREND_DRIVER::DiscardDynVBufAll  -- drv:D3d_rnd_idx.obj
//   v12 0x004C21F0 (feb 0x004B2890) [callgraph] d3dREND_DRIVER::RasterizeObj  -- drv:D3d_rnd_idx.obj
//   v12 0x004C23E0 (feb 0x004B2A70) [exact    ] d3dREND_DRIVER::DrawIndexedPrimitive  -- drv:D3d_rnd_idx.obj
//   v12 0x004C24B0 (feb 0x004B2B40) [callgraph] d3dREND_DRIVER::RenderPrimitive  -- drv:D3d_rnd_idx.obj
//   v12 0x004C24C0 (feb 0x004B2B50) [exact    ] d3dREND_DRIVER::AllocObjVBufHier  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2510 (feb 0x004B2BA0) [exact    ] d3dREND_DRIVER::FreeObjVBufHier  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2570 (feb 0x004B2C00) [callgraph] d3dREND_DRIVER::AllocObjVBuf  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2620 (feb 0x004B2CA0) [exact    ] d3dREND_DRIVER::InvalidateObjDynVBuf  -- drv:D3d_rnd_idx.obj
//   v12 0x004C26C0 (feb 0x004B2D40) [exact    ] d3dREND_DRIVER::InitLevel  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2760 (feb 0x004B2DE0) [exact    ] d3dREND_DRIVER::TermLevel  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2890 (feb 0x004B2F10) [exact    ] d3dREND_DRIVER::ObjLoadNotify  -- drv:D3d_rnd_idx.obj
//   v12 0x004C28C0 (feb 0x004B2F40) [exact    ] d3dREND_DRIVER::ObjDestroyNotify  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2900 (feb 0x004B2F80) [vtable   ] d3dREND_DRIVER::InstLoadNotify  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2910 (feb 0x004B2F90) [vtable   ] d3dREND_DRIVER::InstDestroyNotify  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2920 (feb 0x004B2FA0) [exact    ] d3dREND_DRIVER::ObjTplInitHWNotify  -- drv:D3d_rnd_idx.obj
//   v12 0x004C29B0 (feb 0x004B3030) [exact    ] d3dREND_DRIVER::ObjInitHWNotify  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2A40 (feb 0x004B30C0) [exact    ] d3dREND_DRIVER::SetVBStatFrames  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2AA0 (feb 0x004B3120) [exact    ] d3dREND_DRIVER::SetVBStatFrames_StoreData  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2C80 (feb 0x004B3320) [vtable   ] d3dREND_DRIVER::InvalidateSignCFG  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2C90 (feb 0x004B3330) [vtable   ] d3dREND_DRIVER::ValidatePrepCFG  -- drv:D3d_rnd_idx.obj
//   v12 0x004C2D50 (feb 0x004B33F0) [exact    ] d3dREND_DRIVER::ConfigureTransp  -- drv:D3d_rnd.obj
//   v12 0x004C2D80 (feb 0x004B3420) [exact    ] d3dREND_DRIVER::Configure  -- drv:D3d_rnd.obj
//   v12 0x004C2E20 (feb 0x004B34C0) [exact    ] d3dREND_DRIVER::PreDrawScene  -- drv:D3d_rnd.obj
//   v12 0x004BBF60 (feb 0x004B3550) [vtable   ] d3dREND_DRIVER::PostDrawScene  -- drv:D3d_rnd.obj
//   v12 0x004BBF60 (feb 0x004B3550) [vtable   ] d3dREND_DRIVER::PurgeSWBuffer  -- drv:D3d_rnd.obj
//   v12 0x004C31D0 (feb 0x004B3880) [exact    ] d3dREND_DRIVER::PreDrawObj  -- drv:D3d_rnd.obj
//   v12 0x004C3660 (feb 0x004B3D10) [exact    ] d3dREND_DRIVER::FillPostBuf_Shaded  -- drv:D3d_rnd.obj
//   v12 0x004C3770 (feb 0x004B3E20) [exact    ] d3dREND_DRIVER::DrawFace  -- drv:D3d_rnd.obj
//   v12 0x004C37B0 (feb 0x004B3E60) [exact    ] d3dREND_DRIVER::DrawFace_NotShaded  -- drv:D3d_rnd.obj
//   v12 0x004C3950 (feb 0x004B4000) [exact    ] d3dREND_DRIVER::DrawObj_ShadedNoRevert  -- drv:D3d_rnd.obj
//   v12 0x004C3A60 (feb 0x004B4110) [exact    ] d3dREND_DRIVER::DrawShaderFace  -- drv:D3d_rnd.obj
//   v12 0x004C3DF0 (feb 0x004B44A0) [exact    ] d3dREND_DRIVER::DrawShaderObj  -- drv:D3d_rnd.obj
//   v12 0x004C46F0 (feb 0x004B4DA0) [callgraph] d3dREND_DRIVER::DrawPostBuf  -- drv:D3d_rnd.obj
//   v12 0x004C5020 (feb 0x004B56D0) [exact    ] d3dREND_DRIVER::GetVListPtr  -- drv:D3d_rnd.obj
//   v12 0x004C5050 (feb 0x004B5700) [exact    ] d3dREND_DRIVER::CallShaders  -- drv:D3d_rnd.obj
//   v12 0x004C5370 (feb 0x004B5A20) [callgraph] d3dREND_DRIVER::IsDetailTexConst  -- drv:D3d_rnd.obj
//   v12 0x004C5660 (feb 0x004B5D10) [exact    ] d3dREND_DRIVER::CalcBumpDistCoeff  -- drv:D3d_rnd.obj
//   v12 0x004C5890 (feb 0x004B5F40) [vtable   ] d3dREND_DRIVER::InvalidatePrepCFG  -- drv:D3d_prep_cfg.obj
//   v12 0x004C58B0 (feb 0x004B5F60) [vtable   ] d3dREND_DRIVER::ObjAllocPrepCFG  -- drv:D3d_prep_cfg.obj
//   v12 0x004C5C70 (feb 0x004B62C0) [vtable   ] d3dREND_DRIVER::IsValidPrepCFG  -- drv:D3d_prep_cfg.obj
//   v12 0x004C6030 (feb 0x004B6690) [vtable   ] d3dREND_DRIVER::ValidatePrepCFG  -- drv:D3d_prep_cfg.obj
//   v12 0x004C60A0 (feb 0x004B66F0) [callgraph] d3dREND_DRIVER::PrepareCFG  -- drv:D3d_prep_cfg.obj
//   v12 0x004C6500 (feb 0x004B6AC0) [callgraph] d3dREND_DRIVER::PrepareCFG_NotHTL  -- drv:D3d_prep_cfg.obj
//   v12 0x004C68F0 (feb 0x004B6E80) [exact    ] d3dREND_DRIVER::PrepStgBlend  -- drv:D3d_prep_cfg.obj
//   v12 0x004B9CF0 (feb 0x004DAF60) [vtable   ] d3dREND_DRIVER::FilterObj  -- engine:Rend_ini.obj

// ---- txmD3D
//   v12 0x004BFB50 (feb 0x004B02B0) [callgraph] txmD3D::D3DRequestSize  -- drv:d3d_txm.obj
//   v12 0x004BFC00 (feb 0x004B0360) [exact    ] txmD3D::D3DFindBestWithAlpha  -- drv:d3d_txm.obj
//   v12 0x004BFC70 (feb 0x004B03D0) [exact    ] txmD3D::D3DCreateDrv  -- drv:d3d_txm.obj
//   v12 0x004BFFA0 (feb 0x004B0700) [callgraph] txmD3D::D3DLoadDrv  -- drv:d3d_txm.obj
//   v12 0x004C01A0 (feb 0x004B0900) [callgraph] txmD3D::D3DCompress  -- drv:d3d_txm.obj
//   v12 0x004C04B0 (feb 0x004B0C10) [callgraph] txmD3D::D3DGetColor  -- drv:d3d_txm.obj
//   v12 0x004C0860 (feb 0x004B0FC0) [callgraph] txmD3D::D3DSetColor  -- drv:d3d_txm.obj
//   v12 0x004C0BF0 (feb 0x004B1350) [exact    ] txmD3D::D3DBlendDrv_888  -- drv:d3d_txm.obj
//   v12 0x004C0E00 (feb 0x004B1560) [exact    ] txmD3D::D3DBlendDrv  -- drv:d3d_txm.obj
//   v12 0x004C0F80 (feb 0x004B16E0) [exact    ] txmD3D::D3DTryDownLoadHw  -- drv:d3d_txm.obj
//   v12 0x004C1190 (feb 0x004B18F0) [exact    ] txmD3D::D3DReplaceTex  -- drv:d3d_txm.obj
//   v12 0x004C1260 (feb 0x004B19C0) [callgraph] txmD3D::D3DFindFormat  -- drv:d3d_txm.obj
//   v12 0x004C13C0 (feb 0x004B1B20) [exact    ] txmD3D::D3DCalcTexStageNmb  -- drv:d3d_txm.obj

// ---- txmD3D_TEX_LAY
//   v12 0x004C14E0 (feb 0x004B1C40) [vtable   ] txmD3D_TEX_LAY::GetColor  -- drv:d3d_txm.obj
//   v12 0x004C1510 (feb 0x004B1C70) [vtable   ] txmD3D_TEX_LAY::SetColor  -- drv:d3d_txm.obj
//   v12 0x004C1D50 (feb 0x004B2420) [vtable   ] txmD3D_TEX_LAY::RequestSize  -- drv:d3d_txm.obj
//   v12 0x004C1D60 (feb 0x004B2430) [vtable   ] txmD3D_TEX_LAY::CreateDrv  -- drv:d3d_txm.obj
//   v12 0x004C1D70 (feb 0x004B2440) [vtable   ] txmD3D_TEX_LAY::LoadDrv  -- drv:d3d_txm.obj
//   v12 0x004C1D80 (feb 0x004B2450) [exact    ] txmD3D_TEX_LAY::UnloadDrv  -- drv:d3d_txm.obj
//   v12 0x004C1DF0 (feb 0x004B24C0) [vtable   ] txmD3D_TEX_LAY::BlendDrv  -- drv:d3d_txm.obj

// ---- txmD3D_TEX
//   v12 0x004C1BE0 (feb 0x004B22B0) [vtable   ] txmD3D_TEX::RequestSize  -- drv:d3d_txm.obj
//   v12 0x004C1BF0 (feb 0x004B22C0) [vtable   ] txmD3D_TEX::CreateDrv  -- drv:d3d_txm.obj
//   v12 0x004C1C00 (feb 0x004B22D0) [vtable   ] txmD3D_TEX::LoadDrv  -- drv:d3d_txm.obj
//   v12 0x004C1C10 (feb 0x004B22E0) [exact    ] txmD3D_TEX::UnloadDrv  -- drv:d3d_txm.obj
//   v12 0x004C1C80 (feb 0x004B2350) [vtable   ] txmD3D_TEX::GetColor  -- drv:d3d_txm.obj
//   v12 0x004C1CA0 (feb 0x004B2370) [vtable   ] txmD3D_TEX::SetColor  -- drv:d3d_txm.obj

// ---- objOBJ_ITER
//   v12 ---------- (feb 0x004B32F0) [none     ] objOBJ_ITER::IsDone  -- drv:D3d_rnd_idx.obj

// ---- rendDRIVER
//   v12 0x004C2C70 (feb 0x004B3310) [vtable   ] rendDRIVER::SetBVolUse  -- drv:D3d_rnd_idx.obj
//   v12 0x004E8650 (feb 0x004D89C0) [exact    ] rendDRIVER::RenderSphere  -- engine:rend_vis.obj
//   v12 0x004E87E0 (feb 0x004D8B50) [exact    ] rendDRIVER::RenderLine  -- engine:rend_vis.obj
//   v12 0x004E89E0 (feb 0x004D8D50) [exact    ] rendDRIVER::RenderVector  -- engine:rend_vis.obj
//   v12 0x004E8A50 (feb 0x004D8DC0) [exact    ] rendDRIVER::RenderCross  -- engine:rend_vis.obj
//   v12 ---------- (feb 0x004D8E30) [none     ] rendDRIVER::RenderCS  -- engine:rend_vis.obj
//   v12 0x004E8C10 (feb 0x004D8F50) [exact    ] rendDRIVER::_RenderPoly  -- engine:rend_vis.obj
//   v12 0x004E9600 (feb 0x004D9940) [exact    ] rendDRIVER::RenderPoly  -- engine:rend_vis.obj
//   v12 0x004E9670 (feb 0x004D99B0) [exact    ] rendDRIVER::_RenderPolyCompat  -- engine:rend_vis.obj
//   v12 0x004E9E70 (feb 0x004DA1B0) [exact    ] rendDRIVER::RenderPolyCompat  -- engine:rend_vis.obj
//   v12 0x004EA0B0 (feb 0x004DA3F0) [exact    ] rendDRIVER::RenderQuad  -- engine:rend_vis.obj
//   v12 0x004EA540 (feb 0x004DA880) [exact    ] rendDRIVER::InitHW  -- engine:Rend_ini.obj
//   v12 0x004EA5A0 (feb 0x004DA8E0) [exact    ] rendDRIVER::BeginCamera  -- engine:Rend_ini.obj
//   v12 0x004EA600 (feb 0x004DA940) [exact    ] rendDRIVER::ConfigureClipPlane  -- engine:Rend_ini.obj
//   v12 0x004EA670 (feb 0x004DA9B0) [exact    ] rendDRIVER::EnableClipPlane  -- engine:Rend_ini.obj
//   v12 0x004EA690 (feb 0x004DA9D0) [vtable   ] rendDRIVER::EndCamera  -- engine:Rend_ini.obj
//   v12 0x004EA6A0 (feb 0x004DA9E0) [exact    ] rendDRIVER::InitLevel  -- engine:Rend_ini.obj
//   v12 0x004EA7A0 (feb 0x004DAAE0) [callgraph] rendDRIVER::TermLevel  -- engine:Rend_ini.obj
//   v12 0x004EA7B0 (feb 0x004DAAF0) [exact    ] rendDRIVER::InstLoadNotify  -- engine:Rend_ini.obj
//   v12 0x004EA8E0 (feb 0x004DAC20) [exact    ] rendDRIVER::ObjLoadNotify  -- engine:Rend_ini.obj
//   v12 0x004EA980 (feb 0x004DACC0) [exact    ] rendDRIVER::ObjDestroyNotify  -- engine:Rend_ini.obj
//   v12 0x004EA9A0 (feb 0x004DACE0) [callgraph] rendDRIVER::BeginFrame  -- engine:Rend_ini.obj
//   v12 0x004EA9C0 (feb 0x004DAD00) [vtable   ] rendDRIVER::EndFrame  -- engine:Rend_ini.obj
//   v12 0x004EA9E0 (feb 0x004DAD20) [exact    ] rendDRIVER::InvalidatePrepCFG  -- engine:Rend_ini.obj
//   v12 0x004EAA30 (feb 0x004DAD70) [exact    ] rendDRIVER::InvalidatePrepCFG  -- engine:Rend_ini.obj
//   v12 0x004EAAB0 (feb 0x004DADF0) [exact    ] rendDRIVER::ValidatePrepCFG  -- engine:Rend_ini.obj
//   v12 0x004EAB20 (feb 0x004DAE60) [exact    ] rendDRIVER::ValidatePrepCFG  -- engine:Rend_ini.obj
//   v12 0x004EAB60 (feb 0x004DAEA0) [vtable   ] rendDRIVER::IsValidPrepCFG  -- engine:Rend_ini.obj
//   v12 0x004EADA0 (feb 0x004DB0C0) [vtable   ] rendDRIVER::DrawPrimitive  -- engine:Rend_ini.obj
//   v12 0x004EADB0 (feb 0x004DB0D0) [vtable   ] rendDRIVER::DrawIndexedPrimitive  -- engine:Rend_ini.obj
//   v12 0x004FBE20 (feb 0x004E9910) [exact    ] rendDRIVER::RenderObjListStatic  -- engine:Rend_obj.obj
//   v12 0x004FC020 (feb 0x004E9B10) [exact    ] rendDRIVER::ProjectPoint  -- engine:Rend_obj.obj
//   v12 0x004FC140 (feb 0x004E9C30) [exact    ] rendDRIVER::_ProjectPoint  -- engine:Rend_obj.obj
//   v12 0x004FC1E0 (feb 0x004E9CD0) [exact    ] rendDRIVER::RenderObj  -- engine:Rend_obj.obj
//   v12 0x004FC400 (feb 0x004E9EF0) [exact    ] rendDRIVER::QualObj  -- engine:Rend_obj.obj
//   v12 0x004FC540 (feb 0x004EA030) [callgraph] rendDRIVER::SetPOVClipFlags  -- engine:Rend_obj.obj
//   v12 0x004FCBF0 (feb 0x004EA6C0) [callgraph] rendDRIVER::_RenderObj  -- engine:Rend_obj.obj
//   v12 0x004FCC70 (feb 0x004EA730) [exact    ] rendDRIVER::_RenderObj  -- engine:Rend_obj.obj
//   v12 0x004FD010 (feb 0x004EAAD0) [exact    ] rendDRIVER::RenderObjFace_Isect  -- engine:Rend_obj.obj
//   v12 0x004FD220 (feb 0x004EACE0) [exact    ] rendDRIVER::RenderObjFace_Inside  -- engine:Rend_obj.obj
//   v12 0x004FD3A0 (feb 0x004EAE60) [callgraph] rendDRIVER::RasterizeObjFace_Isect  -- engine:Rend_obj.obj
//   v12 0x004FDB90 (feb 0x004EB600) [callgraph] rendDRIVER::RasterizeObjFace_Inside  -- engine:Rend_obj.obj
//   v12 0x004FDF80 (feb 0x004EB9A0) [exact    ] rendDRIVER::RasterizeObjFace_ZOnly  -- engine:Rend_obj.obj
//   v12 0x0059B900 (feb 0x0057FEE0) [callgraph] rendDRIVER::RenderInst  -- engine:Rend_anim.obj
//   v12 0x0059BA70 (feb 0x0057FFD0) [callgraph] rendDRIVER::QualInst  -- engine:Rend_anim.obj
//   v12 0x0059BCF0 (feb 0x00580250) [exact    ] rendDRIVER::RenderObjListAnim  -- engine:Rend_anim.obj

// ---- _d3drSDR_REND_VERT
//   v12 0x004C43B0 (feb 0x004B4A60) [callgraph] _d3drSDR_REND_VERT::AddFace  -- drv:D3d_rnd.obj

// ---- d3dCFG_PREP
//   v12 0x004C6AA0 (feb 0x004B7030) [exact    ] d3dCFG_PREP::CalcSignature  -- drv:D3d_prep_cfg.obj

// ---- rendOBJ_UD_PREP_CFG
//   v12 0x004C6B20 (feb 0x004B70B0) [vtable   ] rendOBJ_UD_PREP_CFG::GetID  -- drv:D3d_prep_cfg.obj
//   v12 0x004B9CF0 (feb 0x004DAF60) [vtable   ] rendOBJ_UD_PREP_CFG::Duplicate  -- engine:Rend_ini.obj
//   v12 0x004EAC20 (feb 0x004DAF70) [exact    ] rendOBJ_UD_PREP_CFG::Init  -- engine:Rend_ini.obj

// ---- d3dVBUF_MNG
//   v12 0x004C7AE0 (feb 0x004B7EB0) [exact    ] d3dVBUF_MNG::BeginFrame  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C7B20 (feb 0x004B7EF0) [exact    ] d3dVBUF_MNG::PrepStatObj  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C7B70 (feb 0x004B7F40) [callgraph] d3dVBUF_MNG::AddStatData  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C7E90 (feb 0x004B8250) [exact    ] d3dVBUF_MNG::PrepDynObj  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C8000 (feb 0x004B83C0) [exact    ] d3dVBUF_MNG::FitStaticObj  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C80C0 (feb 0x004B8480) [exact    ] d3dVBUF_MNG::StoreStaticObj  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C8190 (feb 0x004B8550) [exact    ] d3dVBUF_MNG::StoreViaObjTpl  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C8280 (feb 0x004B8640) [exact    ] d3dVBUF_MNG::UpdateObjTpl  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C8310 (feb 0x004B86D0) [callgraph] d3dVBUF_MNG::AllocVBuf  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C8410 (feb 0x004B87C0) [callgraph] d3dVBUF_MNG::RenderStatObj  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C84B0 (feb 0x004B8850) [exact    ] d3dVBUF_MNG::RenderDynObj  -- drv:D3d_rnd_vbuf.obj
//   v12 ---------- (feb 0x004B90B0) [none     ] d3dVBUF_MNG::RenderPrimitive  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C8F80 (feb 0x004B9170) [exact    ] d3dVBUF_MNG::RenderIndexedPrimitive  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C90A0 (feb 0x004B9290) [exact    ] d3dVBUF_MNG::LockNoOverwrite  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C90D0 (feb 0x004B92C0) [exact    ] d3dVBUF_MNG::LockDiscard  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9130 (feb 0x004B9320) [callgraph] d3dVBUF_MNG::Unlock  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9150 (feb 0x004B9340) [exact    ] d3dVBUF_MNG::FreeObjVBData  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9240 (feb 0x004B9430) [callgraph] d3dVBUF_MNG::CreateVBufDefault  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9260 (feb 0x004B9450) [exact    ] d3dVBUF_MNG::ReleaseVBufAll  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C92B0 (feb 0x004B94A0) [exact    ] d3dVBUF_MNG::FindVBufDynFVF  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9320 (feb 0x004B9510) [exact    ] d3dVBUF_MNG::DiscardDynVBufAll  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9350 (feb 0x004B9540) [callgraph] d3dVBUF_MNG::CreatePrepData  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9520 (feb 0x004B9700) [callgraph] d3dVBUF_MNG::ReassignFaceSlice  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9610 (feb 0x004B97D0) [exact    ] d3dVBUF_MNG::SetVBStatFrames_AddAllFrames  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9710 (feb 0x004B98D0) [exact    ] d3dVBUF_MNG::SetVBStatFrames_AddFrame  -- drv:D3d_rnd_vbuf.obj

// ---- objSPLIT_ENTRY
//   v12 0x004C9800 (feb 0x004B99C0) [callgraph] objSPLIT_ENTRY::GetNFace  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9810 (feb 0x004B99D0) [callgraph] objSPLIT_ENTRY::GetSplitNmb  -- drv:D3d_rnd_vbuf.obj
//   v12 0x005007D0 (feb 0x004EE720) [callgraph] objSPLIT_ENTRY::CalcTexDensity  -- engine:obj_split.obj

// ---- objOBJ
//   v12 0x004C9880 (feb 0x004B99E0) [callgraph] objOBJ::GetVB  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C9890 (feb 0x004B99F0) [callgraph] objOBJ::GetNSplit  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C98A0 (feb 0x004B9A00) [callgraph] objOBJ::GetSplitUnshared  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004C98B0 (feb 0x004B9A10) [callgraph] objOBJ::SetSplitUnshared  -- drv:D3d_rnd_vbuf.obj
//   v12 0x004CB4A0 (feb 0x004BB4F0) [exact    ] objOBJ::GetNObj  -- engine:Obj_misc.obj
//   v12 0x004CB510 (feb 0x004BB560) [exact    ] objOBJ::GetMaxNVert  -- engine:Obj_misc.obj
//   v12 0x004CB580 (feb 0x004BB5D0) [exact    ] objOBJ::SetIDHier  -- engine:Obj_misc.obj
//   v12 0x004CB5D0 (feb 0x004BB620) [callgraph] objOBJ::_CalcFaceSize  -- engine:Obj_misc.obj
//   v12 ---------- (feb 0x004BB630) [none     ] objOBJ::_CompressFaceList  -- engine:Obj_misc.obj
//   v12 0x004CB700 (feb 0x004BB740) [exact    ] objOBJ::_SortFaceList  -- engine:Obj_misc.obj
//   v12 ---------- (feb 0x004BB7A0) [none     ] objOBJ::_SortMtlList  -- engine:Obj_misc.obj
//   v12 0x004CB8F0 (feb 0x004BB900) [callgraph] objOBJ::MarkUsedTex  -- engine:Obj_misc.obj
//   v12 0x004CB9F0 (feb 0x004BB9E0) [callgraph] objOBJ::ReplaceTex  -- engine:Obj_misc.obj
//   v12 0x004CBC00 (feb 0x004BBB60) [callgraph] objOBJ::SetTex  -- engine:Obj_misc.obj
//   v12 0x004CBCD0 (feb 0x004BBC20) [exact    ] objOBJ::ReplaceTex  -- engine:Obj_misc.obj
//   v12 ---------- (feb 0x004BBCB0) [none     ] objOBJ::ReplaceTexList  -- engine:Obj_misc.obj
//   v12 0x004CBDC0 (feb 0x004BBD10) [exact    ] objOBJ::SetConstColor  -- engine:Obj_misc.obj
//   v12 0x004CBE40 (feb 0x004BBD90) [exact    ] objOBJ::_SetConstColor  -- engine:Obj_misc.obj
//   v12 0x004CBEC0 (feb 0x004BBE10) [exact    ] objOBJ::UpdRenderState  -- engine:Obj_misc.obj
//   v12 0x004CBF10 (feb 0x004BBE60) [exact    ] objOBJ::_UpdRenderState  -- engine:Obj_misc.obj
//   v12 0x004CC040 (feb 0x004BBF90) [callgraph] objOBJ::SetTransparency  -- engine:Obj_misc.obj
//   v12 0x004CC0B0 (feb 0x004BC030) [callgraph] objOBJ::_SetTransparency  -- engine:Obj_misc.obj
//   v12 0x004CC140 (feb 0x004BC080) [exact    ] objOBJ::SetStateYes  -- engine:Obj_misc.obj
//   v12 0x004CC190 (feb 0x004BC0D0) [exact    ] objOBJ::SetStateRendYes  -- engine:Obj_misc.obj
//   v12 0x004CC220 (feb 0x004BC160) [exact    ] objOBJ::SetStateRendNo  -- engine:Obj_misc.obj
//   v12 0x004CC290 (feb 0x004BC1D0) [exact    ] objOBJ::_SetStateRendYes  -- engine:Obj_misc.obj
//   v12 0x004CC2E0 (feb 0x004BC220) [exact    ] objOBJ::_SetStateRendNo  -- engine:Obj_misc.obj
//   v12 0x004CC310 (feb 0x004BC250) [callgraph] objOBJ::InvalidateRendCFG  -- engine:Obj_misc.obj
//   v12 0x004CC320 (feb 0x004BC260) [exact    ] objOBJ::SetStateVBNo  -- engine:Obj_misc.obj
//   v12 0x004CC380 (feb 0x004BC2C0) [exact    ] objOBJ::SetStateProcYes  -- engine:Obj_misc.obj
//   v12 0x004CC410 (feb 0x004BC350) [exact    ] objOBJ::SetStateProcNo  -- engine:Obj_misc.obj
//   v12 ---------- (feb 0x004BC3C0) [none     ] objOBJ::PropagateStateProc  -- engine:Obj_misc.obj
//   v12 0x004CC480 (feb 0x004BC420) [exact    ] objOBJ::SetMtlCoeff  -- engine:Obj_misc.obj
//   v12 0x004CC500 (feb 0x004BC4A0) [exact    ] objOBJ::_NormalizeTextCoord  -- engine:Obj_misc.obj
//   v12 0x004CC9B0 (feb 0x004BC950) [exact    ] objOBJ::FindObjMaxNVert  -- engine:Obj_misc.obj
//   v12 0x004CCA10 (feb 0x004BC9B0) [exact    ] objOBJ::FindObjMaxNVert  -- engine:Obj_misc.obj
//   v12 ---------- (feb 0x004BCA10) [none     ] objOBJ::CompMtlTbl  -- engine:Obj_misc.obj
//   v12 0x004CCF70 (feb 0x004BCF10) [exact    ] objOBJ::IsAncestor  -- engine:Obj_misc.obj
//   v12 0x004CCF90 (feb 0x004BCF30) [callgraph] objOBJ::DeclareProcAnim  -- engine:Obj_misc.obj
//   v12 0x004DC030 (feb 0x004BF2D0) [callgraph] objOBJ::Attach  -- engine:Object.obj
//   v12 0x004DC070 (feb 0x004BF300) [exact    ] objOBJ::Detach  -- engine:Object.obj
//   v12 0x004DC120 (feb 0x004BF3B0) [exact    ] objOBJ::_DestroyData  -- engine:Object.obj
//   v12 0x004DC1E0 (feb 0x004BF470) [exact    ] objOBJ::_Duplicate  -- engine:Object.obj
//   v12 0x004DC560 (feb 0x004BF7F0) [exact    ] objOBJ::Duplicate  -- engine:Object.obj
//   v12 0x004DC610 (feb 0x004BF8A0) [exact    ] objOBJ::SetName  -- engine:Object.obj
//   v12 0x004DC660 (feb 0x004BF8F0) [callgraph] objOBJ::GetTransformedVertList  -- engine:Object.obj
//   v12 ---------- (feb 0x004BF900) [none     ] objOBJ::GetTransformedVertNmb  -- engine:Object.obj
//   v12 0x004DC700 (feb 0x004BF990) [exact    ] objOBJ::ScaleVertList  -- engine:Object.obj
//   v12 0x004DC8D0 (feb 0x004BFB60) [callgraph] objOBJ::GetOrigin  -- engine:Object.obj
//   v12 ---------- (feb 0x004BFB70) [none     ] objOBJ::GetAxisY  -- engine:Object.obj
//   v12 0x004DC8F0 (feb 0x004BFB80) [callgraph] objOBJ::GetAxisZ  -- engine:Object.obj
//   v12 0x004DC900 (feb 0x004BFB90) [exact    ] objOBJ::TransformPoints  -- engine:Object.obj
//   v12 0x004DC9B0 (feb 0x004BFC40) [exact    ] objOBJ::GetTransformedVert  -- engine:Object.obj
//   v12 0x004E00B0 (feb 0x004D1E30) [exact    ] objOBJ::AssignModifiers  -- engine:obj_mod.obj
//   v12 0x004E0150 (feb 0x004D1ED0) [exact    ] objOBJ::RemoveModifiers  -- engine:obj_mod.obj
//   v12 0x004E01B0 (feb 0x004D1F30) [exact    ] objOBJ::DuplicateModifiers  -- engine:obj_mod.obj
//   v12 0x004E0230 (feb 0x004D1FB0) [exact    ] objOBJ::ValidateModifiersHier  -- engine:obj_mod.obj
//   v12 0x004E0300 (feb 0x004D2080) [exact    ] objOBJ::InvalidateModifiers  -- engine:obj_mod.obj
//   v12 0x004E0370 (feb 0x004D20F0) [exact    ] objOBJ::ValidateModifiers  -- engine:obj_mod.obj
//   v12 0x004E03E0 (feb 0x004D2160) [exact    ] objOBJ::ApplyModifiers  -- engine:obj_mod.obj
//   v12 0x004E04D0 (feb 0x004D2250) [exact    ] objOBJ::_IsApplyModifiers  -- engine:obj_mod.obj
//   v12 0x004E0530 (feb 0x004D22B0) [exact    ] objOBJ::AdvanceModifiersTime  -- engine:obj_mod.obj
//   v12 0x004E05B0 (feb 0x004D2330) [exact    ] objOBJ::_AnimateModifiers  -- engine:obj_mod.obj
//   v12 0x004E0600 (feb 0x004D2380) [exact    ] objOBJ::_FindNextModifier  -- engine:obj_mod.obj
//   v12 ---------- (feb 0x004DB3C0) [none     ] objOBJ::_CalcFacePerpVectors  -- engine:obj_normals.obj
//   v12 ---------- (feb 0x004DB470) [none     ] objOBJ::CalcFacePerpVectors  -- engine:obj_normals.obj
//   v12 ---------- (feb 0x004DB4D0) [none     ] objOBJ::NormalizeFacePerpVectors  -- engine:obj_normals.obj
//   v12 0x004EBD10 (feb 0x004DB570) [callgraph] objOBJ::CalcFaceNormals  -- engine:obj_normals.obj
//   v12 0x004EB9B0 (feb 0x004DB5D0) [callgraph] objOBJ::_CalcFaceNormals  -- engine:obj_normals.obj
//   v12 ---------- (feb 0x004DB880) [none     ] objOBJ::_CalcFaceNormalsLCS  -- engine:obj_normals.obj
//   v12 0x004EC790 (feb 0x004DB8F0) [callgraph] objOBJ::CalcVertNormals  -- engine:obj_normals.obj
//   v12 0x004EC7F0 (feb 0x004DB950) [callgraph] objOBJ::_CalcVertNormals  -- engine:obj_normals.obj
//   v12 ---------- (feb 0x004DBC60) [none     ] objOBJ::_CalcVertNormalsLCS  -- engine:obj_normals.obj
//   v12 ---------- (feb 0x004DBD30) [none     ] objOBJ::CalcFaceNormals  -- engine:obj_normals.obj
//   v12 ---------- (feb 0x004DBDA0) [none     ] objOBJ::CalcVertNormals  -- engine:obj_normals.obj
//   v12 ---------- (feb 0x004DBE10) [none     ] objOBJ::CalcVertPerpVectors  -- engine:obj_normals.obj
//   v12 ---------- (feb 0x004DBE70) [none     ] objOBJ::_CalcVertPerpVectors  -- engine:obj_normals.obj
//   v12 ---------- (feb 0x004DBF40) [none     ] objOBJ::NormalizeVertPerpVectors  -- engine:obj_normals.obj
//   v12 0x004FFD40 (feb 0x004EDE00) [callgraph] objOBJ::_CreateFaceSplitPerFace  -- engine:obj_split.obj
//   v12 0x004FFF20 (feb 0x004EDF80) [callgraph] objOBJ::_SetDefaultTCHWIndID  -- engine:obj_split.obj
//   v12 0x005001F0 (feb 0x004EE1A0) [callgraph] objOBJ::_CompressFaceSplit  -- engine:obj_split.obj
//   v12 0x00501080 (feb 0x004EEFB0) [exact    ] objOBJ::GetFaceVertPtr  -- engine:obj_split.obj

// ---- d3dFVF_DESCR
//   v12 0x004C9960 (feb 0x004B9AB0) [exact    ] d3dFVF_DESCR::IsEqualVert  -- drv:D3d_rnd_fvf.obj
//   v12 0x004C9B20 (feb 0x004B9C70) [exact    ] d3dFVF_DESCR::Init  -- drv:D3d_rnd_fvf.obj
//   v12 0x004C9C20 (feb 0x004B9D70) [exact    ] d3dFVF_DESCR::Init  -- drv:D3d_rnd_fvf.obj

// ---- _d3drPOST_ENTRY
//   v12 0x004C9D20 (feb 0x004B9E70) [exact    ] _d3drPOST_ENTRY::SetEntryData  -- drv:d3d_postbuf.obj

// ---- objFILTER_NAME
//   v12 0x004CCCE0 (feb 0x004BCC80) [exact    ] objFILTER_NAME::IsUse  -- engine:Obj_misc.obj

// ---- objFILTER_ID
//   v12 0x004CD2B0 (feb 0x004BD220) [vtable   ] objFILTER_ID::IsUse  -- engine:Obj_misc.obj

// ---- objFILTER_FUNC
//   v12 ---------- (feb 0x004BD240) [none     ] objFILTER_FUNC::IsUse  -- engine:Obj_misc.obj

// ---- objFILTER_ANIM_NMB
//   v12 0x004CD2E0 (feb 0x004BD250) [vtable   ] objFILTER_ANIM_NMB::IsUse  -- engine:Obj_misc.obj

// ---- objSET
//   v12 0x004DCE80 (feb 0x004C0110) [exact    ] objSET::Alloc  -- engine:Object.obj
//   v12 0x004DCEC0 (feb 0x004C0150) [exact    ] objSET::AddObj  -- engine:Object.obj

// ---- partEMITTER_PHYS
//   v12 0x004D1810 (feb 0x004C0260) [callgraph] partEMITTER_PHYS::ConstructorInit  -- engine:particle.obj
//   v12 0x004D1C80 (feb 0x004C06D0) [callgraph] partEMITTER_PHYS::StartEmission  -- engine:particle.obj
//   v12 0x004D1CB0 (feb 0x004C0700) [callgraph] partEMITTER_PHYS::StopEmission  -- engine:particle.obj
//   v12 0x004D1CC0 (feb 0x004C0710) [callgraph] partEMITTER_PHYS::SetAnimEmission  -- engine:particle.obj
//   v12 0x004D1D10 (feb 0x004C0760) [callgraph] partEMITTER_PHYS::SetColor  -- engine:particle.obj
//   v12 0x004D1D60 (feb 0x004C0780) [vtable   ] partEMITTER_PHYS::GetColor  -- engine:particle.obj
//   v12 0x004D1D90 (feb 0x004C07B0) [exact    ] partEMITTER_PHYS::SetIntensity  -- engine:particle.obj
//   v12 0x004D1DC0 (feb 0x004C07E0) [callgraph] partEMITTER_PHYS::SetTimeLifePart  -- engine:particle.obj
//   v12 0x004D1DE0 (feb 0x004C0800) [callgraph] partEMITTER_PHYS::SetSize  -- engine:particle.obj
//   v12 0x004D1E40 (feb 0x004C0830) [callgraph] partEMITTER_PHYS::SetTexture  -- engine:particle.obj
//   v12 0x004D1E50 (feb 0x004C0840) [callgraph] partEMITTER_PHYS::SetTextureScale  -- engine:particle.obj
//   v12 0x004D1E70 (feb 0x004C0860) [callgraph] partEMITTER_PHYS::SetTextureFrameAnim  -- engine:particle.obj
//   v12 0x004D1ED0 (feb 0x004C0890) [callgraph] partEMITTER_PHYS::SetTextureRotAnim  -- engine:particle.obj
//   v12 0x004D1EF0 (feb 0x004C08B0) [callgraph] partEMITTER_PHYS::SetDegNearParams  -- engine:particle.obj
//   v12 0x004D1F10 (feb 0x004C0900) [callgraph] partEMITTER_PHYS::UpdateUpdate  -- engine:particle.obj
//   v12 0x004D2070 (feb 0x004C0A60) [callgraph] partEMITTER_PHYS::UpdateErase  -- engine:particle.obj
//   v12 0x004D2120 (feb 0x004C0B10) [callgraph] partEMITTER_PHYS::UpdateGetNmbEmit  -- engine:particle.obj
//   v12 0x004D21E0 (feb 0x004C0BD0) [callgraph] partEMITTER_PHYS::UpdateEmit  -- engine:particle.obj
//   v12 0x004D2270 (feb 0x004C0C60) [callgraph] partEMITTER_PHYS::KillParticle  -- engine:particle.obj
//   v12 ---------- (feb 0x004C0C70) [none     ] partEMITTER_PHYS::KillAll  -- engine:particle.obj
//   v12 0x004D2290 (feb 0x004C0C80) [vtable   ] partEMITTER_PHYS::PartUpdateVisible  -- engine:particle.obj
//   v12 0x004D23D0 (feb 0x004C0DC0) [callgraph] partEMITTER_PHYS::SetPosDirSpeed  -- engine:particle.obj
//   v12 0x004D2530 (feb 0x004C0F20) [callgraph] partEMITTER_PHYS::GetPosDirSpeed  -- engine:particle.obj
//   v12 ---------- (feb 0x004C0F90) [none     ] partEMITTER_PHYS::SetCS  -- engine:particle.obj
//   v12 0x004D25C0 (feb 0x004C0FB0) [callgraph] partEMITTER_PHYS::SetAreaEmission  -- engine:particle.obj
//   v12 0x004D2750 (feb 0x004C1140) [callgraph] partEMITTER_PHYS::SetCDTPlane  -- engine:particle.obj
//   v12 0x004D2790 (feb 0x004C1180) [callgraph] partEMITTER_PHYS::SetCDTParam  -- engine:particle.obj
//   v12 ---------- (feb 0x004C11B0) [none     ] partEMITTER_PHYS::AttachEmitter  -- engine:particle.obj
//   v12 0x004D27E0 (feb 0x004C11D0) [callgraph] partEMITTER_PHYS::SetDegFarParams  -- engine:particle.obj
//   v12 0x004D2810 (feb 0x004C1200) [callgraph] partEMITTER_PHYS::SetScaleCSVal  -- engine:particle.obj
//   v12 ---------- (feb 0x004C1230) [none     ] partEMITTER_PHYS::SetScaleCS  -- engine:particle.obj
//   v12 0x004D2860 (feb 0x004C1250) [callgraph] partEMITTER_PHYS::PartCreate  -- engine:particle.obj
//   v12 0x004D35A0 (feb 0x004C1F90) [vtable   ] partEMITTER_PHYS::PartUpdate  -- engine:particle.obj
//   v12 0x004D3730 (feb 0x004C2120) [vtable   ] partEMITTER_PHYS::Update  -- engine:particle.obj
//   v12 0x004D3CA0 (feb 0x004C2690) [callgraph] partEMITTER_PHYS::PartApplyCDT  -- engine:particle.obj
//   v12 ---------- (feb 0x004C2920) [none     ] partEMITTER_PHYS::PredictCDT  -- engine:particle.obj
//   v12 0x004D5450 (feb 0x004C6470) [callgraph] partEMITTER_PHYS::RegisterPart  -- engine:Partrend.obj
//   v12 0x004D5980 (feb 0x004C6980) [vtable   ] partEMITTER_PHYS::Register  -- engine:Partrend.obj
//   v12 0x004D5C00 (feb 0x004C6BF0) [callgraph] partEMITTER_PHYS::CalcBBox  -- engine:Partrend.obj

// ---- partEMITTER_PHYS_FLY
//   v12 0x004D40E0 (feb 0x004C2AE0) [vtable   ] partEMITTER_PHYS_FLY::Update  -- engine:particle.obj

// ---- pteSPOT
//   v12 0x004D62C0 (feb 0x004C72E0) [exact    ] pteSPOT::Init  -- engine:pte_spot.obj
//   v12 0x004D6480 (feb 0x004C74A0) [callgraph] pteSPOT::SetStateRend  -- engine:pte_spot.obj
//   v12 0x004D64A0 (feb 0x004C74C0) [callgraph] pteSPOT::SetRadius  -- engine:pte_spot.obj
//   v12 0x004D64B0 (feb 0x004C74D0) [vtable   ] pteSPOT::SetOrg  -- engine:pte_spot.obj
//   v12 0x004D64D0 (feb 0x004C74F0) [exact    ] pteSPOT::SetColor  -- engine:pte_spot.obj
//   v12 0x004D6520 (feb 0x004C7540) [callgraph] pteSPOT::SetTex  -- engine:pte_spot.obj
//   v12 0x004D6540 (feb 0x004C7560) [exact    ] pteSPOT::InitLevel  -- engine:pte_spot.obj
//   v12 0x004D6570 (feb 0x004C7590) [vtable   ] pteSPOT::TermLevel  -- engine:pte_spot.obj
//   v12 0x004D6590 (feb 0x004C75B0) [exact    ] pteSPOT::DeleteObjNotify  -- engine:pte_spot.obj
//   v12 0x004D6610 (feb 0x004C7630) [exact    ] pteSPOT::AddObjExclude  -- engine:pte_spot.obj
//   v12 0x004D6640 (feb 0x004C7660) [exact    ] pteSPOT::IsShadeObj  -- engine:pte_spot.obj
//   v12 0x004D6710 (feb 0x004C7730) [vtable   ] pteSPOT::IsShadeInst  -- engine:pte_spot.obj
//   v12 0x004D6730 (feb 0x004C7750) [vtable   ] pteSPOT::IsActive  -- engine:pte_spot.obj
//   v12 0x004D68C0 (feb 0x004C78F0) [exact    ] pteSPOT::PrepareFrame  -- engine:pte_spot.obj
//   v12 0x004D69A0 (feb 0x004C79D0) [vtable   ] pteSPOT::ShadeFacePrepare  -- engine:pte_spot.obj
//   v12 0x004D69C0 (feb 0x004C79F0) [vtable   ] pteSPOT::ShadeFace  -- engine:pte_spot.obj
//   v12 0x004D7400 (feb 0x004C8430) [exact    ] pteSPOT::GetStateRend  -- engine:pte_spot.obj
//   v12 0x004D7500 (feb 0x004C8530) [vtable   ] pteSPOT::GetTex  -- engine:pte_spot.obj
//   v12 0x004D7520 (feb 0x004C8550) [exact    ] pteSPOT::GetColor  -- engine:pte_spot.obj
//   v12 0x004D7560 (feb 0x004C8590) [vtable   ] pteSPOT::GetColor  -- engine:pte_spot.obj
//   v12 0x004D7880 (feb 0x004C88C0) [exact    ] pteSPOT::MakeObjIndList  -- engine:pte_spot.obj
//   v12 0x004D78C0 (feb 0x004C8900) [exact    ] pteSPOT::IsIncludeFaceStaticData  -- engine:pte_spot.obj
//   v12 0x004D7BA0 (feb 0x004C8BE0) [vtable   ] pteSPOT::GetMaxRadius  -- engine:pte_spot.obj

// ---- pteSPOT_ANIM
//   v12 0x004D7BD0 (feb 0x004C8C10) [exact    ] pteSPOT_ANIM::Create  -- engine:pte_spot_a.obj
//   v12 0x004D7C20 (feb 0x004C8C60) [exact    ] pteSPOT_ANIM::Update  -- engine:pte_spot_a.obj
//   v12 0x004D7DC0 (feb 0x004C8E00) [exact    ] pteSPOT_ANIM::Init  -- engine:pte_spot_a.obj
//   v12 0x004D80A0 (feb 0x004C90E0) [exact    ] pteSPOT_ANIM::SetCtrlRad  -- engine:pte_spot_a.obj
//   v12 0x004D80D0 (feb 0x004C9110) [exact    ] pteSPOT_ANIM::SetCtrlPos  -- engine:pte_spot_a.obj
//   v12 0x004D8120 (feb 0x004C9160) [exact    ] pteSPOT_ANIM::SetCtrlColorLM  -- engine:pte_spot_a.obj
//   v12 0x004D8190 (feb 0x004C91D0) [exact    ] pteSPOT_ANIM::SetCtrlColorBase  -- engine:pte_spot_a.obj
//   v12 0x004D8200 (feb 0x004C9240) [exact    ] pteSPOT_ANIM::SetCurTime  -- engine:pte_spot_a.obj
//   v12 0x004D8260 (feb 0x004C92A0) [vtable   ] pteSPOT_ANIM::GetMaxRadius  -- engine:pte_spot_a.obj
//   v12 0x004D8270 (feb 0x004C92B0) [vtable   ] pteSPOT_ANIM::DeleteInstNotify  -- engine:pte_spot_a.obj
//   v12 0x004D82D0 (feb 0x004C9310) [vtable   ] pteSPOT_ANIM::DeleteObjNotify  -- engine:pte_spot_a.obj

// ---- pteMANAGER
//   v12 0x004D8480 (feb 0x004C94C0) [exact    ] pteMANAGER::RegisterClass  -- engine:pte_shader.obj
//   v12 0x004D8520 (feb 0x004C9560) [exact    ] pteMANAGER::CreateShader  -- engine:pte_shader.obj
//   v12 0x004D8650 (feb 0x004C9690) [exact    ] pteMANAGER::CreateShader  -- engine:pte_shader.obj
//   v12 0x004D86D0 (feb 0x004C9710) [exact    ] pteMANAGER::CreateShader  -- engine:pte_shader.obj
//   v12 0x004D8750 (feb 0x004C9790) [exact    ] pteMANAGER::InitLevel  -- engine:pte_shader.obj
//   v12 0x004D87B0 (feb 0x004C97F0) [exact    ] pteMANAGER::TermLevel  -- engine:pte_shader.obj
//   v12 0x004D8820 (feb 0x004C9860) [exact    ] pteMANAGER::DeleteInstNotify  -- engine:pte_shader.obj
//   v12 0x004D8870 (feb 0x004C98B0) [exact    ] pteMANAGER::DeleteObjNotify  -- engine:pte_shader.obj
//   v12 0x004D88E0 (feb 0x004C9920) [exact    ] pteMANAGER::FindNextShader  -- engine:pte_shader.obj
//   v12 0x004D8910 (feb 0x004C9950) [exact    ] pteMANAGER::RemoveShader  -- engine:pte_shader.obj
//   v12 0x004D89A0 (feb 0x004C99E0) [exact    ] pteMANAGER::UpdateAllShaders  -- engine:pte_shader.obj
//   v12 0x004D8A80 (feb 0x004C9AC0) [exact    ] pteMANAGER::ActivateAllShaders  -- engine:pte_shader.obj
//   v12 ---------- (feb 0x004C9B90) [none     ] pteMANAGER::DeactivateAllShaders  -- engine:pte_shader.obj
//   v12 0x004D8B60 (feb 0x004C9BA0) [exact    ] pteMANAGER::PrepareFrame  -- engine:pte_shader.obj
//   v12 0x004D9030 (feb 0x004CA070) [exact    ] pteMANAGER::IsShadeInst  -- engine:pte_shader.obj
//   v12 0x004D9070 (feb 0x004CA0B0) [callgraph] pteMANAGER::PrepareObj  -- engine:pte_shader.obj
//   v12 0x004D9310 (feb 0x004CA350) [callgraph] pteMANAGER::GetNShader  -- engine:pte_shader.obj
//   v12 0x004D9330 (feb 0x004CA370) [callgraph] pteMANAGER::GetShader  -- engine:pte_shader.obj
//   v12 0x004D9360 (feb 0x004CA3A0) [callgraph] pteMANAGER::FindShaderAround  -- engine:pte_shader.obj
//   v12 0x004D94E0 (feb 0x004CA520) [exact    ] pteMANAGER::ShadeFaceWCS  -- engine:pte_shader.obj
//   v12 0x004D9850 (feb 0x004CA890) [exact    ] pteMANAGER::ShadeVert  -- engine:pte_shader.obj
//   v12 0x004D98A0 (feb 0x004CA8E0) [exact    ] pteMANAGER::StartShadeObj  -- engine:pte_shader.obj
//   v12 0x004D98E0 (feb 0x004CA920) [callgraph] pteMANAGER::EndShadeObj  -- engine:pte_shader.obj

// ---- pteSHADER
//   v12 0x004D8D20 (feb 0x004C9D60) [exact    ] pteSHADER::Init  -- engine:pte_shader.obj
//   v12 0x004D8DA0 (feb 0x004C9DE0) [exact    ] pteSHADER::ReadStr  -- engine:pte_shader.obj
//   v12 0x004D8DE0 (feb 0x004C9E20) [callgraph] pteSHADER::ReadFloat  -- engine:pte_shader.obj
//   v12 0x004D8E10 (feb 0x004C9E50) [callgraph] pteSHADER::ReadBool  -- engine:pte_shader.obj
//   v12 0x004D8E40 (feb 0x004C9E80) [callgraph] pteSHADER::ReadColor  -- engine:pte_shader.obj
//   v12 0x004D8E70 (feb 0x004C9EB0) [exact    ] pteSHADER::Register  -- engine:pte_shader.obj
//   v12 0x004D8EB0 (feb 0x004C9EF0) [callgraph] pteSHADER::Enable  -- engine:pte_shader.obj
//   v12 0x004D8ED0 (feb 0x004C9F10) [exact    ] pteSHADER::SetSheet  -- engine:pte_shader.obj
//   v12 ---------- (feb 0x004C9F40) [none     ] pteSHADER::SetInstCreate  -- engine:pte_shader.obj
//   v12 ---------- (feb 0x004C9F60) [none     ] pteSHADER::SetObjCreate  -- engine:pte_shader.obj
//   v12 0x004D8F40 (feb 0x004C9F80) [exact    ] pteSHADER::DeleteInstNotify  -- engine:pte_shader.obj
//   v12 0x004D8F70 (feb 0x004C9FB0) [exact    ] pteSHADER::DeleteObjNotify  -- engine:pte_shader.obj
//   v12 0x004D8FA0 (feb 0x004C9FE0) [exact    ] pteSHADER::GetName  -- engine:pte_shader.obj
//   v12 0x004D9010 (feb 0x004CA050) [callgraph] pteSHADER::MarkFaceFirstShaded  -- engine:pte_shader.obj
//   v12 0x004E0920 (feb 0x004D2690) [exact    ] pteSHADER::MakeObjIndList  -- engine:pte_sdr_static.obj
//   v12 0x004E0990 (feb 0x004D2700) [exact    ] pteSHADER::CalcStaticData  -- engine:pte_sdr_static.obj
//   v12 0x004E0C90 (feb 0x004D2A00) [exact    ] pteSHADER::CalcStaticDataTexCoord  -- engine:pte_sdr_static.obj
//   v12 0x004E1020 (feb 0x004D2D90) [exact    ] pteSHADER::IncludeStaticDataObj  -- engine:pte_sdr_static.obj

// ---- pteSDR_INFO
//   v12 0x004D98F0 (feb 0x004CA930) [exact    ] pteSDR_INFO::ValidateNormalSmooth  -- engine:pte_shader.obj
//   v12 0x004D9A80 (feb 0x004CAAC0) [exact    ] pteSDR_INFO::ValidateReflectPoint  -- engine:pte_shader.obj

// ---- scnSCENE
//   v12 ---------- (feb 0x004CB3F0) [none     ] scnSCENE::MakeAnimList  -- engine:scn_coll.obj
//   v12 0x004DA6A0 (feb 0x004CB6D0) [callgraph] scnSCENE::MakeAnimList  -- engine:scn_coll.obj
//   v12 0x004DAA10 (feb 0x004CBA20) [callgraph] scnSCENE::_ConstrSphere_1  -- engine:scn_coll.obj
//   v12 0x004DB000 (feb 0x004CBFA0) [exact    ] scnSCENE::ConstrSphere  -- engine:scn_coll.obj
//   v12 0x004DB040 (feb 0x004CBFE0) [callgraph] scnSCENE::_ConstrSphere  -- engine:scn_coll.obj
//   v12 0x004DB2B0 (feb 0x004CC250) [exact    ] scnSCENE::GetClosestPoint  -- engine:scn_coll.obj
//   v12 0x004DB370 (feb 0x004CC310) [callgraph] scnSCENE::_GetClosestPointAnim  -- engine:scn_coll.obj
//   v12 ---------- (feb 0x004CC750) [none     ] scnSCENE::_DistPointInst  -- engine:scn_coll.obj
//   v12 ---------- (feb 0x004CC9B0) [none     ] scnSCENE::DistPointObj  -- engine:scn_coll.obj
//   v12 0x004DBAF0 (feb 0x004CCA90) [exact    ] scnSCENE::CheckCDTInfo  -- engine:scn_coll.obj
//   v12 0x004DBB70 (feb 0x004CCB10) [callgraph] scnSCENE::GetCDTFace  -- engine:scn_coll.obj
//   v12 0x004DBE70 (feb 0x004CCE30) [exact    ] scnSCENE::GetCDTObj  -- engine:scn_coll.obj
//   v12 0x004DE9F0 (feb 0x004CF890) [exact    ] scnSCENE::ShootRay  -- engine:scn_vis.obj
//   v12 0x004DEB20 (feb 0x004CF9C0) [callgraph] scnSCENE::_ShootRayAnim  -- engine:scn_vis.obj
//   v12 0x004DED20 (feb 0x004CFBD0) [callgraph] scnSCENE::IsectRayInst  -- engine:scn_vis.obj
//   v12 0x004DEDC0 (feb 0x004CFC70) [callgraph] scnSCENE::_IsectRayInst  -- engine:scn_vis.obj
//   v12 0x004DEFD0 (feb 0x004CFE80) [callgraph] scnSCENE::IsVisiblePoint  -- engine:scn_vis.obj
//   v12 ---------- (feb 0x004D00E0) [none     ] scnSCENE::IsVisiblePointCone  -- engine:scn_vis.obj
//   v12 0x004DF2B0 (feb 0x004D0370) [exact    ] scnSCENE::FloorPlace  -- engine:scn_vis.obj
//   v12 0x004E5FE0 (feb 0x004DD6F0) [callgraph] scnSCENE::IsEmptyInst  -- engine:Scene.obj
//   v12 0x004E5FF0 (feb 0x004DD700) [callgraph] scnSCENE::InstPtr  -- engine:Scene.obj
//   v12 0x004E6010 (feb 0x004DD720) [callgraph] scnSCENE::InitLevel  -- engine:Scene.obj
//   v12 0x004E61E0 (feb 0x004DD8E0) [exact    ] scnSCENE::DestroyLevel  -- engine:Scene.obj
//   v12 0x004E63A0 (feb 0x004DDAA0) [exact    ] scnSCENE::FindTplName  -- engine:Scene.obj
//   v12 0x004E6410 (feb 0x004DDB10) [exact    ] scnSCENE::AllocTpl  -- engine:Scene.obj
//   v12 0x004E6470 (feb 0x004DDB70) [callgraph] scnSCENE::DestroyTpl  -- engine:Scene.obj
//   v12 0x004E6490 (feb 0x004DDB90) [exact    ] scnSCENE::_FindInstEmpty  -- engine:Scene.obj
//   v12 0x004E64C0 (feb 0x004DDBC0) [exact    ] scnSCENE::_AttachInst  -- engine:Scene.obj
//   v12 0x004E6510 (feb 0x004DDC10) [exact    ] scnSCENE::AllocInst  -- engine:Scene.obj
//   v12 0x004E6620 (feb 0x004DDD20) [callgraph] scnSCENE::CreateInst  -- engine:Scene.obj
//   v12 0x004E6950 (feb 0x004DE050) [callgraph] scnSCENE::CreateInst  -- engine:Scene.obj
//   v12 0x004E6CD0 (feb 0x004DE390) [exact    ] scnSCENE::DestroyInst  -- engine:Scene.obj
//   v12 0x004E6D90 (feb 0x004DE450) [exact    ] scnSCENE::FindInstName  -- engine:Scene.obj
//   v12 0x004E6DE0 (feb 0x004DE4A0) [exact    ] scnSCENE::ForAllInst  -- engine:Scene.obj
//   v12 0x004E6E20 (feb 0x004DE4E0) [exact    ] scnSCENE::FindNextInst  -- engine:Scene.obj
//   v12 0x004E6E50 (feb 0x004DE510) [exact    ] scnSCENE::FindNextTpl  -- engine:Scene.obj
//   v12 0x004E6EC0 (feb 0x004DE580) [exact    ] scnSCENE::GetNInst  -- engine:Scene.obj
//   v12 0x004E6EF0 (feb 0x004DE5B0) [exact    ] scnSCENE::GetNTpl  -- engine:Scene.obj
//   v12 0x004E6F60 (feb 0x004DE620) [exact    ] scnSCENE::SetAnimLightFlags  -- engine:Scene.obj
//   v12 0x004E7030 (feb 0x004DE6F0) [exact    ] scnSCENE::GetAnimLights  -- engine:Scene.obj
//   v12 0x004E7390 (feb 0x004DEA50) [exact    ] scnSCENE::ExpandV3DTmpList  -- engine:Scene.obj
//   v12 0x004E73E0 (feb 0x004DEAA0) [exact    ] scnSCENE::SetStateYesInst  -- engine:Scene.obj

// ---- objMOD_TEX
//   v12 0x004DCF30 (feb 0x004CEAE0) [exact    ] objMOD_TEX::AddObj  -- engine:obj_mod_tex.obj

// ---- objMOD_TEX_SCROLL
//   v12 0x004DD080 (feb 0x004CEC30) [exact    ] objMOD_TEX_SCROLL::Modify  -- engine:obj_mod_tex.obj

// ---- objMOD_TEX_FRAME_ANIM
//   v12 0x004DD240 (feb 0x004CEDF0) [exact    ] objMOD_TEX_FRAME_ANIM::AddObj  -- engine:obj_mod_tex.obj
//   v12 0x004DD2C0 (feb 0x004CEE70) [exact    ] objMOD_TEX_FRAME_ANIM::SetParams  -- engine:obj_mod_tex.obj
//   v12 0x004DD320 (feb 0x004CEED0) [exact    ] objMOD_TEX_FRAME_ANIM::Modify  -- engine:obj_mod_tex.obj
//   v12 0x004DD3E0 (feb 0x004CEF90) [exact    ] objMOD_TEX_FRAME_ANIM::ApplyFrameNmb  -- engine:obj_mod_tex.obj
//   v12 0x004DDC90 (feb 0x004CF820) [vtable   ] objMOD_TEX_FRAME_ANIM::AdvanceTime  -- engine:obj_mod_tex.obj

// ---- objMOD_TEX_SEQ_ANIM
//   v12 0x004DD5A0 (feb 0x004CF140) [exact    ] objMOD_TEX_SEQ_ANIM::AddObj  -- engine:obj_mod_tex.obj
//   v12 0x004DD5D0 (feb 0x004CF170) [exact    ] objMOD_TEX_SEQ_ANIM::SetParams  -- engine:obj_mod_tex.obj
//   v12 0x004DD630 (feb 0x004CF1D0) [vtable   ] objMOD_TEX_SEQ_ANIM::Modify  -- engine:obj_mod_tex.obj
//   v12 0x004DDCA0 (feb 0x004CF830) [vtable   ] objMOD_TEX_SEQ_ANIM::AdvanceTime  -- engine:obj_mod_tex.obj

// ---- objMOD_BLINK_TRANSP
//   v12 0x004DD740 (feb 0x004CF2D0) [exact    ] objMOD_BLINK_TRANSP::AddObj  -- engine:obj_mod_tex.obj
//   v12 0x004DD780 (feb 0x004CF310) [exact    ] objMOD_BLINK_TRANSP::SetParams  -- engine:obj_mod_tex.obj
//   v12 0x004DD830 (feb 0x004CF3C0) [exact    ] objMOD_BLINK_TRANSP::Modify  -- engine:obj_mod_tex.obj
//   v12 0x004DDCB0 (feb 0x004CF840) [vtable   ] objMOD_BLINK_TRANSP::AdvanceTime  -- engine:obj_mod_tex.obj

// ---- objMOD_NOISE_DEFORM_UV
//   v12 0x004DD950 (feb 0x004CF4E0) [exact    ] objMOD_NOISE_DEFORM_UV::AddObj  -- engine:obj_mod_tex.obj
//   v12 0x004DDA00 (feb 0x004CF590) [callgraph] objMOD_NOISE_DEFORM_UV::SetParams  -- engine:obj_mod_tex.obj
//   v12 0x004DDA20 (feb 0x004CF5B0) [exact    ] objMOD_NOISE_DEFORM_UV::Modify  -- engine:obj_mod_tex.obj
//   v12 0x004F7680 (feb 0x004E5190) [vtable   ] objMOD_NOISE_DEFORM_UV::AdvanceTime  -- engine:obj_mod_geom.obj

// ---- objMODIFIER
//   v12 0x004DFE70 (feb 0x004D1BF0) [exact    ] objMODIFIER::SetViewPos  -- engine:obj_mod.obj
//   v12 0x004DFF70 (feb 0x004D1CF0) [exact    ] objMODIFIER::AddObj  -- engine:obj_mod.obj
//   v12 0x004E0000 (feb 0x004D1D80) [exact    ] objMODIFIER::DelObj  -- engine:obj_mod.obj

// ---- pteSDR_STATIC_DATA
//   v12 0x004E14D0 (feb 0x004D3240) [exact    ] pteSDR_STATIC_DATA::IsShadedFace  -- engine:pte_sdr_static.obj
//   v12 0x004E16C0 (feb 0x004D3430) [exact    ] pteSDR_STATIC_DATA::GetObjShadeCode  -- engine:pte_sdr_static.obj

// ---- pteSPOT_ELLIPSE
//   v12 0x004E1700 (feb 0x004D3470) [exact    ] pteSPOT_ELLIPSE::SetParams  -- engine:pte_spot_e.obj
//   v12 0x004E17D0 (feb 0x004D3540) [vtable   ] pteSPOT_ELLIPSE::ShadeFace  -- engine:pte_spot_e.obj

// ---- pteEMAP
//   v12 0x004E2260 (feb 0x004D3E80) [exact    ] pteEMAP::Init  -- engine:pte_envmap.obj
//   v12 0x004E23F0 (feb 0x004D4010) [exact    ] pteEMAP::IsShadeObj  -- engine:pte_envmap.obj
//   v12 0x004E2420 (feb 0x004D4040) [exact    ] pteEMAP::GetMatrL2W  -- engine:pte_envmap.obj
//   v12 0x004E24B0 (feb 0x004D40D0) [exact    ] pteEMAP::PrepareFrame  -- engine:pte_envmap.obj
//   v12 0x004E58C0 (feb 0x004D74E0) [vtable   ] pteEMAP::IsShadeInst  -- engine:pte_tex2.obj

// ---- pteEMAP_CHROME
//   v12 ---------- (feb 0x004D4150) [none     ] pteEMAP_CHROME::Create  -- engine:pte_envmap.obj
//   v12 0x004E2A50 (feb 0x004D4670) [exact    ] pteEMAP_CHROME::GetMatrL2W  -- engine:pte_envmap.obj
//   v12 0x004E3D90 (feb 0x004D59B0) [exact    ] pteEMAP_CHROME::ShadeFace  -- engine:pte_envmap.obj
//   v12 0x004E3F70 (feb 0x004D5B90) [exact    ] pteEMAP_CHROME::ShadeVert  -- engine:pte_envmap.obj
//   v12 0x004E54B0 (feb 0x004D70D0) [vtable   ] pteEMAP_CHROME::GetBlendTypeDef  -- engine:pte_envmap.obj

// ---- pteEMAP_SPECULAR
//   v12 ---------- (feb 0x004D4190) [none     ] pteEMAP_SPECULAR::Create  -- engine:pte_envmap.obj
//   v12 0x004E27D0 (feb 0x004D43F0) [exact    ] pteEMAP_SPECULAR::Init  -- engine:pte_envmap.obj
//   v12 0x004E2920 (feb 0x004D4540) [exact    ] pteEMAP_SPECULAR::PrepareFrame  -- engine:pte_envmap.obj
//   v12 0x004E2970 (feb 0x004D4590) [exact    ] pteEMAP_SPECULAR::GetMatrL2W  -- engine:pte_envmap.obj
//   v12 0x004E36B0 (feb 0x004D52D0) [vtable   ] pteEMAP_SPECULAR::ShadeFacePrepare  -- engine:pte_envmap.obj
//   v12 0x004E3A90 (feb 0x004D56B0) [vtable   ] pteEMAP_SPECULAR::ShadeFace  -- engine:pte_envmap.obj

// ---- pteEMAP_ENVIR
//   v12 ---------- (feb 0x004D41F0) [none     ] pteEMAP_ENVIR::Create  -- engine:pte_envmap.obj
//   v12 0x004E2A70 (feb 0x004D4690) [exact    ] pteEMAP_ENVIR::Init  -- engine:pte_envmap.obj
//   v12 ---------- (feb 0x004D4AD0) [none     ] pteEMAP_ENVIR::GetMatrL2W  -- engine:pte_envmap.obj
//   v12 0x004E3CE0 (feb 0x004D5900) [exact    ] pteEMAP_ENVIR::ShadeFace  -- engine:pte_envmap.obj

// ---- pteEMAP_PANEL
//   v12 ---------- (feb 0x004D4240) [none     ] pteEMAP_PANEL::Create  -- engine:pte_envmap.obj
//   v12 0x004E2B10 (feb 0x004D4730) [exact    ] pteEMAP_PANEL::Init  -- engine:pte_envmap.obj
//   v12 ---------- (feb 0x004D4AD0) [none     ] pteEMAP_PANEL::GetMatrL2W  -- engine:pte_envmap.obj
//   v12 0x004E2ED0 (feb 0x004D4AF0) [exact    ] pteEMAP_PANEL::PrepareFrame  -- engine:pte_envmap.obj
//   v12 0x004E4130 (feb 0x004D5D50) [exact    ] pteEMAP_PANEL::ShadeFace  -- engine:pte_envmap.obj
//   v12 0x004E4350 (feb 0x004D5F70) [exact    ] pteEMAP_PANEL::ShadeVert  -- engine:pte_envmap.obj

// ---- pteEMAP_REFLECT
//   v12 ---------- (feb 0x004D4290) [none     ] pteEMAP_REFLECT::Create  -- engine:pte_envmap.obj
//   v12 0x004E4550 (feb 0x004D6170) [vtable   ] pteEMAP_REFLECT::Init  -- engine:pte_envmap.obj
//   v12 0x004E4780 (feb 0x004D63A0) [exact    ] pteEMAP_REFLECT::ShadeFace  -- engine:pte_envmap.obj
//   v12 0x004E4910 (feb 0x004D6530) [callgraph] pteEMAP_REFLECT::ShadeVert  -- engine:pte_envmap.obj

// ---- pteEMAP_REFRACT
//   v12 ---------- (feb 0x004D42E0) [none     ] pteEMAP_REFRACT::Create  -- engine:pte_envmap.obj
//   v12 0x004E4C90 (feb 0x004D68B0) [exact    ] pteEMAP_REFRACT::Init  -- engine:pte_envmap.obj
//   v12 0x004E4DB0 (feb 0x004D69D0) [exact    ] pteEMAP_REFRACT::ShadeFace  -- engine:pte_envmap.obj
//   v12 0x004E4E40 (feb 0x004D6A60) [exact    ] pteEMAP_REFRACT::ShadeVert  -- engine:pte_envmap.obj
//   v12 0x004E5050 (feb 0x004D6C70) [callgraph] pteEMAP_REFRACT::ShadeVert3D  -- engine:pte_envmap.obj
//   v12 0x004E53D0 (feb 0x004D6FF0) [callgraph] pteEMAP_REFRACT::IsectRayBed  -- engine:pte_envmap.obj

// ---- wrsfxPORTAL_VIEW
//   v12 ---------- (feb 0x004D4AD0) [none     ] wrsfxPORTAL_VIEW::GetMatrL2W  -- engine:pte_envmap.obj

// ---- pteEMAP_SPHERE
//   v12 0x004E3060 (feb 0x004D4C80) [exact    ] pteEMAP_SPHERE::ShadeFacePrepare  -- engine:pte_envmap.obj
//   v12 0x004E3470 (feb 0x004D5090) [exact    ] pteEMAP_SPHERE::ShadeFace  -- engine:pte_envmap.obj

// ---- pteTEX2
//   v12 0x004E5550 (feb 0x004D7170) [exact    ] pteTEX2::Init  -- engine:pte_tex2.obj
//   v12 0x004E5590 (feb 0x004D71B0) [exact    ] pteTEX2::AddTexReplace  -- engine:pte_tex2.obj
//   v12 0x004E5640 (feb 0x004D7260) [exact    ] pteTEX2::AddTexReplace  -- engine:pte_tex2.obj
//   v12 ---------- (feb 0x004D72E0) [none     ] pteTEX2::SetupTexList  -- engine:pte_tex2.obj
//   v12 0x004E5720 (feb 0x004D7340) [exact    ] pteTEX2::SetBlendCoeff  -- engine:pte_tex2.obj
//   v12 0x004E5880 (feb 0x004D74A0) [exact    ] pteTEX2::Update  -- engine:pte_tex2.obj
//   v12 0x004E58C0 (feb 0x004D74E0) [vtable   ] pteTEX2::IsShadeInst  -- engine:pte_tex2.obj
//   v12 0x004E58E0 (feb 0x004D7500) [exact    ] pteTEX2::IsShadeObj  -- engine:pte_tex2.obj
//   v12 0x004E5910 (feb 0x004D7530) [exact    ] pteTEX2::GetTex  -- engine:pte_tex2.obj
//   v12 0x004E5980 (feb 0x004D75A0) [vtable   ] pteTEX2::ShadeFace  -- engine:pte_tex2.obj
//   v12 0x004E5CB0 (feb 0x004D78A0) [exact    ] pteTEX2::ShadeVert  -- engine:pte_tex2.obj

// ---- _rendSCN_STAT
//   v12 0x004EA210 (feb 0x004DA550) [exact    ] _rendSCN_STAT::Collect  -- engine:Rend_ini.obj

// ---- rendMTL_TBL
//   v12 0x004EAB70 (feb 0x004DAEB0) [exact    ] rendMTL_TBL::UseNMtl  -- engine:Rend_ini.obj

// ---- rendCFG
//   v12 0x004EABD0 (feb 0x004DAF10) [callgraph] rendCFG::InitMtl1  -- engine:Rend_ini.obj
//   v12 0x004EABF0 (feb 0x004DAF30) [exact    ] rendCFG::FindMtlRendMtl  -- engine:Rend_ini.obj

// ---- aiGLAD_KNIFE
//   v12 0x004B9CF0 (feb 0x004DAF60) [vtable   ] aiGLAD_KNIFE::IsArmoredObject  -- engine:Rend_ini.obj

// ---- aiSKEL_SPEAR
//   v12 0x004B9CF0 (feb 0x004DAF60) [vtable   ] aiSKEL_SPEAR::IsArmoredObject  -- engine:Rend_ini.obj

// ---- aiGLAD_MACE
//   v12 0x004B9CF0 (feb 0x004DAF60) [vtable   ] aiGLAD_MACE::IsArmoredObject  -- engine:Rend_ini.obj

// ---- cdtCOLL_OBJ
//   v12 0x004B9CF0 (feb 0x004DAF60) [vtable   ] cdtCOLL_OBJ::GetID2Ptr  -- engine:Rend_ini.obj
//   v12 0x00502560 (feb 0x004DCD70) [callgraph] cdtCOLL_OBJ::GetID2BBox  -- engine:scn_domain.obj
//   v12 0x00502560 (feb 0x004DCD70) [callgraph] cdtCOLL_OBJ::IsectID2BBox  -- engine:scn_domain.obj

// ---- aiSKEL_SWORD
//   v12 0x004B9CF0 (feb 0x004DAF60) [vtable   ] aiSKEL_SWORD::IsArmoredObject  -- engine:Rend_ini.obj

// ---- rendCFG_PREP
//   v12 0x004EAC70 (feb 0x004DAFC0) [vtable   ] rendCFG_PREP::Init  -- engine:Rend_ini.obj

// ---- cinACTION
//   v12 0x004EADA0 (feb 0x004DB0C0) [vtable   ] cinACTION::Init  -- engine:Rend_ini.obj

// ---- scnDOMAIN
//   v12 0x004EFB10 (feb 0x004DC1B0) [callgraph] scnDOMAIN::Init  -- engine:scn_domain.obj
//   v12 0x004F01D0 (feb 0x004DC870) [exact    ] scnDOMAIN::Term  -- engine:scn_domain.obj
//   v12 0x004F0200 (feb 0x004DC8A0) [exact    ] scnDOMAIN::SetHeight  -- engine:scn_domain.obj
//   v12 0x004F0430 (feb 0x004DCAD0) [exact    ] scnDOMAIN::IsBelongPoint  -- engine:scn_domain.obj
//   v12 0x004F0560 (feb 0x004DCC00) [exact    ] scnDOMAIN::IsBelongPointXZ  -- engine:scn_domain.obj
//   v12 0x004F0610 (feb 0x004DCCB0) [exact    ] scnDOMAIN::IsLSegCrossBoundaryXZ  -- engine:scn_domain.obj
//   v12 0x00502560 (feb 0x004DCD70) [callgraph] scnDOMAIN::ISectLSegBoundaryXZ  -- engine:scn_domain.obj
//   v12 0x004F06D0 (feb 0x004DCD80) [exact    ] scnDOMAIN::IsIsectBox  -- engine:scn_domain.obj
//   v12 0x004F0750 (feb 0x004DCE00) [exact    ] scnDOMAIN::_IsIsectObj  -- engine:scn_domain.obj
//   v12 0x004F0BD0 (feb 0x004DD280) [exact    ] scnDOMAIN::IsIsectObj  -- engine:scn_domain.obj
//   v12 0x004F0C60 (feb 0x004DD310) [vtable   ] scnDOMAIN::IsIsectInst  -- engine:scn_domain.obj
//   v12 0x004F0C80 (feb 0x004DD330) [exact    ] scnDOMAIN::_IsIsectInst  -- engine:scn_domain.obj
//   v12 0x004F0CC0 (feb 0x004DD370) [exact    ] scnDOMAIN::CalcBBox  -- engine:scn_domain.obj
//   v12 0x004F0DA0 (feb 0x004DD450) [exact    ] scnDOMAIN::GetRandomPoint  -- engine:scn_domain.obj
//   v12 0x004F0E20 (feb 0x004DD4D0) [exact    ] scnDOMAIN::GetPoly  -- engine:scn_domain.obj

// ---- gssWND
//   v12 0x00502560 (feb 0x004DCD70) [callgraph] gssWND::ProcessAct  -- engine:scn_domain.obj

// ---- lodLOD
//   v12 0x004F2FF0 (feb 0x004DED30) [vtable   ] lodLOD::Apply  -- engine:lod.obj
//   v12 0x004F3180 (feb 0x004DEED0) [exact    ] lodLOD::SetLodNmb  -- engine:lod.obj
//   v12 0x004F31F0 (feb 0x004DEF40) [exact    ] lodLOD::Init  -- engine:lod.obj
//   v12 0x004F3250 (feb 0x004DEFA0) [vtable   ] lodLOD::GetDataPtr  -- engine:lod.obj

// ---- objLOD
//   v12 0x004F3260 (feb 0x004DEFB0) [exact    ] objLOD::Init  -- engine:lod.obj
//   v12 0x004F33D0 (feb 0x004DF120) [exact    ] objLOD::_SetLodNmb  -- engine:lod.obj
//   v12 0x004F3440 (feb 0x004DF190) [vtable   ] objLOD::GetLodCenter  -- engine:lod.obj

// ---- animLOD_SEL
//   v12 0x004F3460 (feb 0x004DF1B0) [vtable   ] animLOD_SEL::GetLodCenter  -- engine:lod.obj
//   v12 0x004F3470 (feb 0x004DF1C0) [exact    ] animLOD_SEL::_SetLodNmb  -- engine:lod.obj
//   v12 0x004F3500 (feb 0x004DF250) [exact    ] animLOD_SEL::Init  -- engine:lod.obj
//   v12 0x004F36B0 (feb 0x004DF400) [exact    ] animLOD_SEL::Term  -- engine:lod.obj

// ---- animINST
//   v12 0x004F3B50 (feb 0x004DF910) [callgraph] animINST::LODInit  -- engine:lod.obj
//   v12 0x004F3D90 (feb 0x004DFB50) [exact    ] animINST::LODTerm  -- engine:lod.obj
//   v12 0x004F3E00 (feb 0x004DFBC0) [callgraph] animINST::OBJ_LODInit  -- engine:lod.obj
//   v12 ---------- (feb 0x004DFC60) [none     ] animINST::LODApply  -- engine:lod.obj
//   v12 0x004F4040 (feb 0x004DFE00) [callgraph] animINST::LODSetTemp  -- engine:lod.obj
//   v12 0x004F4120 (feb 0x004DFEE0) [exact    ] animINST::LODRestore  -- engine:lod.obj
//   v12 0x004F41A0 (feb 0x004DFF60) [exact    ] animINST::LODEnable  -- engine:lod.obj
//   v12 0x004F4210 (feb 0x004DFFD0) [exact    ] animINST::LODFreeze  -- engine:lod.obj

// ---- psSHEET
//   v12 0x004F4430 (feb 0x004E01F0) [exact    ] psSHEET::GetStr  -- engine:pssheet.obj
//   v12 0x004F4490 (feb 0x004E0250) [callgraph] psSHEET::GetInt  -- engine:pssheet.obj
//   v12 0x004F4650 (feb 0x004E0410) [exact    ] psSHEET::GetBool  -- engine:pssheet.obj
//   v12 0x004F4820 (feb 0x004E05E0) [callgraph] psSHEET::GetFloat  -- engine:pssheet.obj
//   v12 ---------- (feb 0x004E07A0) [none     ] psSHEET::GetHEX  -- engine:pssheet.obj
//   v12 0x004F4BA0 (feb 0x004E0960) [exact    ] psSHEET::GetColor  -- engine:pssheet.obj
//   v12 0x004F4BF0 (feb 0x004E09B0) [exact    ] psSHEET::Empty  -- engine:pssheet.obj
//   v12 0x004F4C90 (feb 0x004E0A50) [exact    ] psSHEET::SetString  -- engine:pssheet.obj
//   v12 0x004F4CC0 (feb 0x004E0A80) [exact    ] psSHEET::ParseString  -- engine:pssheet.obj
//   v12 0x004F5030 (feb 0x004E0DF0) [exact    ] psSHEET::CompileString  -- engine:pssheet.obj
//   v12 0x004F51D0 (feb 0x004E0F90) [exact    ] psSHEET::FindSect  -- engine:pssheet.obj
//   v12 0x004F5250 (feb 0x004E1010) [exact    ] psSHEET::FindVarStr  -- engine:pssheet.obj
//   v12 0x004F5340 (feb 0x004E1100) [exact    ] psSHEET::AddVar  -- engine:pssheet.obj
//   v12 0x004F55C0 (feb 0x004E1380) [exact    ] psSHEET::_FindSect  -- engine:pssheet.obj
//   v12 0x004F5610 (feb 0x004E13D0) [exact    ] psSHEET::_FindVar  -- engine:pssheet.obj
//   v12 0x004F5690 (feb 0x004E1450) [exact    ] psSHEET::_GetStr  -- engine:pssheet.obj
//   v12 0x004F5780 (feb 0x004E1540) [exact    ] psSHEET::_GetColor  -- engine:pssheet.obj
//   v12 0x004F59C0 (feb 0x004E1780) [exact    ] psSHEET::SetStr  -- engine:pssheet.obj
//   v12 0x004F5A20 (feb 0x004E17E0) [exact    ] psSHEET::Save  -- engine:pssheet.obj
//   v12 0x004F5AD0 (feb 0x004E1890) [exact    ] psSHEET::Load  -- engine:pssheet.obj

// ---- objMOD_GEOM
//   v12 0x004F6F30 (feb 0x004E2CE0) [exact    ] objMOD_GEOM::AddObj  -- engine:obj_mod_geom.obj
//   v12 0x004F6F80 (feb 0x004E2D30) [exact    ] objMOD_GEOM::StoreIniVert  -- engine:obj_mod_geom.obj

// ---- objMOD_SKIN
//   v12 0x004ECBD0 (feb 0x004E3010) [callgraph] objMOD_SKIN::Init  -- engine:obj_mod_geom.obj
//   v12 ---------- (feb 0x004E30F0) [none     ] objMOD_SKIN::Squeeze  -- engine:obj_mod_geom.obj
//   v12 0x004ECE80 (feb 0x004E3250) [vtable   ] objMOD_SKIN::Duplicate  -- engine:obj_mod_geom.obj
//   v12 ---------- (feb 0x004E34C0) [none     ] objMOD_SKIN::Rebuild  -- engine:obj_mod_geom.obj
//   v12 ---------- (feb 0x004E38C0) [none     ] objMOD_SKIN::Reduce  -- engine:obj_mod_geom.obj
//   v12 0x004ED840 (feb 0x004E3B90) [exact    ] objMOD_SKIN::Check1Bone  -- engine:obj_mod_geom.obj
//   v12 ---------- (feb 0x004E3BE0) [none     ] objMOD_SKIN::Reorder  -- engine:obj_mod_geom.obj
//   v12 0x004EDB20 (feb 0x004E3E40) [callgraph] objMOD_SKIN::StartApply  -- engine:obj_mod_geom.obj
//   v12 0x004EDB30 (feb 0x004E3E50) [vtable   ] objMOD_SKIN::Animate  -- engine:obj_mod_geom.obj
//   v12 0x004EDBD0 (feb 0x004E3EF0) [exact    ] objMOD_SKIN::Modify  -- engine:obj_mod_geom.obj
//   v12 0x004EDC20 (feb 0x004E3F40) [callgraph] objMOD_SKIN::Apply_NoReorder  -- engine:obj_mod_geom.obj
//   v12 0x004EE3A0 (feb 0x004E46C0) [callgraph] objMOD_SKIN::Apply_Reorder  -- engine:obj_mod_geom.obj

// ---- objMOD_NOISE_DEFORM
//   v12 0x004F7270 (feb 0x004E4CA0) [exact    ] objMOD_NOISE_DEFORM::AddObj  -- engine:obj_mod_geom.obj
//   v12 0x004F72F0 (feb 0x004E4D20) [callgraph] objMOD_NOISE_DEFORM::SetParams  -- engine:obj_mod_geom.obj
//   v12 0x004F7310 (feb 0x004E4D40) [exact    ] objMOD_NOISE_DEFORM::FixBoundary  -- engine:obj_mod_geom.obj
//   v12 0x004F7530 (feb 0x004E4F40) [exact    ] objMOD_NOISE_DEFORM::Modify  -- engine:obj_mod_geom.obj
//   v12 0x004F7680 (feb 0x004E5190) [vtable   ] objMOD_NOISE_DEFORM::AdvanceTime  -- engine:obj_mod_geom.obj

// ---- objSPLIT
//   v12 0x004FFD00 (feb 0x004EDDC0) [exact    ] objSPLIT::Make  -- engine:obj_split.obj
//   v12 0x005005A0 (feb 0x004EE520) [exact    ] objSPLIT::Alloc  -- engine:obj_split.obj
//   v12 0x00500640 (feb 0x004EE5C0) [exact    ] objSPLIT::Duplicate  -- engine:obj_split.obj
//   v12 ---------- (feb 0x004EE6C0) [none     ] objSPLIT::RestoreFaceSplitNmb  -- engine:obj_split.obj

// ---- objSPLIT_UNSHARED
//   v12 0x00500FF0 (feb 0x004EEF20) [exact    ] objSPLIT_UNSHARED::Alloc  -- engine:obj_split.obj

// ---- rnsBUF_TRANSP_TYPE
//   v12 0x00503A70 (feb 0x004F1600) [exact    ] rnsBUF_TRANSP_TYPE::StoreObj  -- engine:Rend_sort.obj
//   v12 ---------- (feb 0x004F1680) [none     ] rnsBUF_TRANSP_TYPE::Purge  -- engine:Rend_sort.obj

// ---- rnsBUF_OPAQUE
//   v12 0x00503DB0 (feb 0x004F1710) [callgraph] rnsBUF_OPAQUE::Purge  -- engine:Rend_sort.obj
//   v12 0x00503E30 (feb 0x004F1790) [callgraph] rnsBUF_OPAQUE::LinkObjSplit  -- engine:Rend_sort.obj

// ---- rnsMNG
//   v12 0x00503EC0 (feb 0x004F1850) [callgraph] rnsMNG::InitLevel  -- engine:Rend_sort.obj
//   v12 0x005040A0 (feb 0x004F1A30) [callgraph] rnsMNG::StoreObj  -- engine:Rend_sort.obj
//   v12 0x00504370 (feb 0x004F1D60) [callgraph] rnsMNG::Purge  -- engine:Rend_sort.obj
//   v12 0x00504590 (feb 0x004F20B0) [exact    ] rnsMNG::IsEmpty  -- engine:Rend_sort.obj
//   v12 0x005045F0 (feb 0x004F2110) [exact    ] rnsMNG::StorePoly  -- engine:Rend_sort.obj
//   v12 0x00504680 (feb 0x004F21A0) [exact    ] rnsMNG::PurgePoly  -- engine:Rend_sort.obj
//   v12 0x00504700 (feb 0x004F2220) [callgraph] rnsMNG::ObjDestroyNotify  -- engine:Rend_sort.obj
//   v12 0x00504780 (feb 0x004F22A0) [callgraph] rnsMNG::LinkObjSplit  -- engine:Rend_sort.obj
//   v12 ---------- (feb 0x004F22D0) [none     ] rnsMNG::LinkObj  -- engine:Rend_sort.obj
//   v12 0x00504950 (feb 0x004F23B0) [exact    ] rnsMNG::_UnLinkObj  -- engine:Rend_sort.obj
//   v12 0x005049E0 (feb 0x004F2460) [exact    ] rnsMNG::UnLinkObj  -- engine:Rend_sort.obj
//   v12 ---------- (feb 0x004F2550) [none     ] rnsMNG::LinkInst  -- engine:Rend_sort.obj
//   v12 0x00504C60 (feb 0x004F2650) [callgraph] rnsMNG::UnLinkInst  -- engine:Rend_sort.obj

// ---- rnsSIGNATURE
//   v12 0x00504D70 (feb 0x004F2760) [exact    ] rnsSIGNATURE::Insert  -- engine:Rend_sort.obj
//   v12 0x00504DA0 (feb 0x004F2790) [exact    ] rnsSIGNATURE::Compare  -- engine:Rend_sort.obj

// ---- pteFOG_GROUND
//   v12 0x0059A0D0 (feb 0x0057E6B0) [exact    ] pteFOG_GROUND::Create  -- engine:pte_fog.obj
//   v12 0x0059A2F0 (feb 0x0057E8D0) [exact    ] pteFOG_GROUND::InitLevel  -- engine:pte_fog.obj
//   v12 0x0059A4A0 (feb 0x0057EA80) [exact    ] pteFOG_GROUND::SetDomainObj  -- engine:pte_fog.obj
//   v12 0x0059A500 (feb 0x0057EAE0) [exact    ] pteFOG_GROUND::InitParam  -- engine:pte_fog.obj
//   v12 0x0059A660 (feb 0x0057EC40) [exact    ] pteFOG_GROUND::IsActive  -- engine:pte_fog.obj
//   v12 0x0059A6B0 (feb 0x0057EC90) [vtable   ] pteFOG_GROUND::Update  -- engine:pte_fog.obj
//   v12 0x0059A6D0 (feb 0x0057ECB0) [exact    ] pteFOG_GROUND::IsShadeInst  -- engine:pte_fog.obj
//   v12 0x0059A790 (feb 0x0057ED70) [exact    ] pteFOG_GROUND::IsShadeObj  -- engine:pte_fog.obj
//   v12 0x0059A850 (feb 0x0057EE30) [exact    ] pteFOG_GROUND::PrepareFrame  -- engine:pte_fog.obj
//   v12 0x0059A8F0 (feb 0x0057EED0) [exact    ] pteFOG_GROUND::ShadeFace  -- engine:pte_fog.obj
//   v12 0x0059ACE0 (feb 0x0057F2C0) [exact    ] pteFOG_GROUND::ShadeVert  -- engine:pte_fog.obj
//   v12 0x0059AF10 (feb 0x0057F4F0) [callgraph] pteFOG_GROUND::CalcVertFog  -- engine:pte_fog.obj

// ---- fntMANAGER
//   v12 0x0059B540 (feb 0x0057FB20) [callgraph] fntMANAGER::SetScreenSize  -- engine:font.obj

// ---- fntFONT
//   v12 0x0059B560 (feb 0x0057FB40) [exact    ] fntFONT::Print  -- engine:font.obj
//   v12 0x0059B6A0 (feb 0x0057FC80) [exact    ] fntFONT::GetStringExtents  -- engine:font.obj
//   v12 0x0059B760 (feb 0x0057FD40) [exact    ] fntFONT::PrintChar  -- engine:font.obj

// total 1013 functions, 934 with a v12 address
