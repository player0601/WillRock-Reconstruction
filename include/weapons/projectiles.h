// projectiles.h -- Will Rock 1.2 unofficial SDK, subsystem "weapons": player projectiles and their launch data.
//
// Assumed: physPHYS / physINIT (engine_core/physics.h), pjlPJL<SFX> template (game_shared/weapon_base.h),
//   pjlPJL_INIT (msgDATA, 0x69 bytes feb / 0x6D bytes v12: its msgDATA size word is written as 0x69 in feb and 0x6D in
//   v12 by every launcher, e.g. medusa ProcessSHOOT feb 0x0040F226 "mov word [esp+0xA],0x69" / v12 0x00410868 "0x6D"),
//   dmgDispatchExplosDamage (game_shared/damage.h), sncCHANNEL, partEMITTER_PHYS, entCreate.
// Conventions as in weapons.h. Projectile class field offsets inside physPHYS-derived classes are NOT feb+0x1C:
//   class sizes grew by 0x1C..0x24 (see sizes below), so own fields are "// ?" unless an address is given.
//
// LIFECYCLE: launcher ProcessSHOOT (msg 0x4C7 v12 / 0x4C6 feb, runs on server and clients) fills a physINIT +
//   pjlPJL_INIT on the stack and calls entCreate(gsScenePtr, "<class>", ..., init) -> Create -> ProcessINIT
//   (physPHYS::ProcessINIT, SFX::StartSFX attaches trail particles) -> ProcessFRAME (physics step, trail) ->
//   collision ProcessEV_CDT -> ShowCollisSFX (explosion/decal) + DispatchDamage (radius damage or pwpOPP_SFX effect).
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#pragma pack(push, 1)

// ---- launch parameters. Each launcher's ProcessSHOOT fills a pjlPJL_INIT (msgDATA, size word 0x69 feb / 0x6D v12)
//      on the stack: vel = aimDir * speed, accel (gravity), then a few floats/ints. Values below are in store order
//      (work/sdk_weapons/pinit.py, every v12 store compared with feb). Field NAMES for the extra floats are not
//      confirmed (the struct is not physINIT: its header starts 4 bytes later and the defaults differ) -> "// ?".
//      Speed constants are POOLED __real's: patch the fmul operand sites in re/results/weapon_npc.json (PROJ_*).
//  projectile        class name       launcher ProcessSHOOT v12 (feb)  speed  gravity       other launch values (feb == v12)
//  pwpACD_PJL        "pwp_acd_pjl"    0x004017D0 (0x00401950)          128    none          0.3, 1, 1000; model scale 0.3
//  pwpAMG_PJL        "pwp_amg_pjl"    0x00404550 (0x00404820)          28     -9.8 * up     100, 0.3, 1000; scale 0.2
//  pwpBMB_PJL        "pwp_bmb_pjl"    0x00406D20 (0x004067E0)          60     -19.6 * up    120, 0.7, 0.2, -1, 0.15; scale 0.3
//  pwpBSK_BULLET     "sfx_bsk_bullet" 0x00408950 (0x00408420)          50     none          25, 0.2, 1000; 5, 6
//  pwpCROSSBOW_PJL   "pwp_crb_pjl"    0x0040D9C0 (0x0040CF40)          297.6  none          100, 0.3, 1000; scale 0.2
//  pwpMEDUSA_PJL     "sfx_mdg_pjl"    0x004107D0 (0x0040F180)          55.8   2.5 * up(?)   1.2, 1000, and CHANGED:
//      float 0.01 -> 0.3: feb 0x0040F2B8 "mov [esp+0x28],0x3C23D70A" / v12 0x004108F2 "mov [esp+0x2C],0x3E99999A"
//      int   2    -> 6  : feb 0x0040F283 "mov [esp+0x3C],2"          / v12 0x004108C5 "mov [esp+0x40],6"
//      (same slot relative to the struct header, which moved +4 on the stack). // ? meaning: collision radius / flags
//  Note: the medusa "gravity" is the global up vector (feb 0x005F9990, v12 0x00637BDC) * 2.5, i.e. it floats upward.

// ---- classes (vtables: v12 (feb); v12 vtable addresses from the Create functions' vftable stores) ----------------
// pwpACD_PJL      : pjlPJL<pwpACD_PJL_SFX>  vt v12 0x005F9010 (feb 0x005CE370) size v12 0x198 (feb 0x178)
//     Create 0x00401260 (0x00401260)  ProcessINIT 0x004024A0 (0x00402470)  ShowCollisSFX 0x004026C0 (0x00402540)
//     DispatchDamage 0x00402920 (0x004027A0): pwpOPP_SFX::Start(type 2 "acid", 5, 3.0 s ...) -- the melting effect,
//       not plain damage. StartSFX 0x00402300 (0x00402290).
// pwpAMG_PJL      : pjlPJL<pwpAMG_PJL_SFX>  vt v12 0x005F9658 (feb 0x005CE8F0) // ? vt paired by slot similarity
//     ProcessINIT 0x00405220 (0x00404E90)  ProcessFRAME 0x00405540 (0x00405090)  ShowCollisSFX 0x004056B0 (0x004051F0)
//     DispatchDamage 0x00405890 (0x004053D0): pwpOPP_SFX::Start (burn, 9.0)  StartSFX 0x00405060 (0x00404CD0)
// pwpBMB_PJL      : physPHYS                vt v12 0x005F99B8 (feb 0x005CEC90) size v12 0x1A7 (feb 0x18B)
//     Create 0x00406A30 (0x00406510) ProcessINIT 0x00406FE0 (0x00406AA0) ProcessCDT 0x004071B0 (0x00406C70)
//     ProcessFRAME 0x004073A0 (0x00406E60) Explode 0x004075F0 (0x004070B0) -> dmgDispatchExplosDamage v12 0x00546410
//     ShowCollisSFX 0x00407720 (0x004071D0)
// pwpBSK_BULLET   : pjlPJL<pwpBSK_PJL_SFX>  vt v12 0x005F9C80 (feb 0x005CEF80) size v12 0x1B4 (feb 0x194)
//     Create 0x00408470 (0x00407F20) ProcessINIT 0x00409120 (0x00408BC0) ShowCollisSFX 0x004094B0 (0x00408ED0)
//     pjlPJL<BSK>::DispatchDamage 0x00409720 (0x00409410): dmgDispatchExplosDamage + ...Inst (v12 0x00546410/0x005464C0)
// pwpCROSSBOW_PJL : pjlPJL<pwpCRB_PJL_SFX>  vt v12 0x005FA2E8 (feb 0x005CF558) // ? vt paired by slot similarity
//     ProcessINIT 0x0040E710 (0x0040D7F0) ShowCollisSFX 0x0040E9D0 (0x0040D9A0) DispatchDamage 0x0040EC10 (0x0040DBE0)
//     (pwpOPP_SFX burn 2.0). Point-blank hits skip the bolt: launcher applies the effect directly (both builds).
// pwpMEDUSA_PJL   : pjlPJL<pwpMDG_PJL_SFX>  vt v12 0x005FA710 (feb 0x005CF950) // ? size v12 0x1D0 (feb 0x1AC)
//     Create 0x004104B0 (0x0040EE50) ProcessINIT 0x00410D20 (0x0040F6E0) ProcessFRAME 0x00411020 (0x0040F8C0)
//     MakeDamage 0x004112D0 (0x0040FB60): the petrifying cloud -- collects instances (dmgCollectInst), per-target
//       box scale (see FEB_VS_12.md), 150.0 range constant. DispatchDamage 0x004118A0 (0x004100B0)
//
// Generic template pjlPJL<SFX> (one copy per SFX type, all identical):
//   ProcessINIT    e.g. <ACD> v12 0x00403210 (feb 0x00403080)  1.2 adds a per-projectile looping sound channel when
//                  vslot +0x8C returns true: vslot +0x9C(&min,&max) -> new sncCHANNEL(min,max,360,360) ->
//                  gsSND_SYSTEM::AddChannel (feb had no projectile sound channel here).
//   ProcessFRAME   <ACD> 0x00403460 (0x00403190); ProcessTERM <ACD> 0x00403400 (0x00403130)
//   ProcessEV_CDT  <BSK> 0x00409CE0 (0x004094E0), <CRB> 0x0040FA30 (0x0040E4B0): collision -> vslot ShowCollisSFX,
//                  DispatchDamage, Terminate. 1.2 calls vslot +0x80 where feb called +0x88 (vtable shift).

// Shared effects (pwp_opp_sfx.obj): pwpOPP_SFX "pwp_sfx_opp" -- acid melting / burning / stone effect applied to
//   NPCs and players; Start v12 0x0041A1C0 (feb 0x00417B90); CalcRadDamage v12 0x0041BAF0 (feb 0x00419390).
//   pwpOPP_SFX_START grew 8 bytes in 1.2 (launchers zero two extra dwords, e.g. acid DispatchDamage v12 0x00402926).
// Water splashes (pwp_water_sfx.obj): pwpWSFX_SPLASH / pwpWSFX_EXPL ("sfx_water_splash", "sfx_water_expl").

#pragma pack(pop)
