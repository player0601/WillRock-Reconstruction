// player_mp.cpp -- plrPLAYER_MP, "player_mp": a remote player in multiplayer, and the local player's own body
// (Will Rock 1.2, v12.exe; feb obj player_mp.obj / player_mp_skin.obj)
//
// How the body works. plrPLAYER::ProcessINIT makes one "player_mp" entity (player_mp.tpl, the cowboy, scaled
// 0.073) named "player_3d", mpType 2. Every frame plrPLAYER::Animate picks the sequence the player's movement asks
// for (idle, run, strafe, back, jump, crouch, swim, death) and writes it with the pitch and the move bits into the
// player's sync block -- the same 0x53 bytes multiplayer sends for a remote player -- then hands the block to the
// body (ReadAnimSync, slot +0x12C), puts it where the player stands facing the lagging leg direction, and gives it
// the angle between the view and the legs (bodyTwist). The body's frame plays the sequence (AnimateBody), turns its
// back by the twist and bends it by the pitch (AnimateProcedural), and hangs the held weapon's model on its
// "wpn_holder_*" (UpdateWeapon). plrPLAYER::SetupGhostRendState decides what of it is drawn: in first person all
// of it is kept out of the main pass (procState 0x200, rendDRIVER::QualObj) except the "DOWN_BODY" subtree -- the
// legs -- and only while VIDEO_PlrLegs is on and the view looks down (pitch 150 or more); the mirror pass and the
// projected shadow still take the whole body.
//
// Rebuilt so far: Create, ProcessMsg, ProcessINIT / TERM / INIT_LEVEL / INIT_MAP / INIT_GAME / TERM_GAME,
// ProcessFRAME, AnimateBody, UpdateWeapon, AnimateProcedural, ReadAnimSync, SetCurWpn, SetPosDir, SetDefaultValues,
// SetupGhostWpnState, the small slots. Not rebuilt: MP_NOTIFY, CDT_FIREABLE, the Pick* slots, StartDeath, SelectSkin
// / SetTeam (player_mp_skin), the power-up emitter (v12 0x0042C3F0) -- the header marks them.

#include "game.h"
#include "../wr_vcall.h"
#include "../m3d/m3d_spl.h"

static_assert(sizeof(plrPLAYER_MP) == 0x293 && offsetof(plrPLAYER_MP, seqTarget) == 0x263 &&
              offsetof(plrPLAYER, eyeHeight) == 0x26F && offsetof(plrPLAYER, body) == 0x2DB, "plrPLAYER_MP");

float __fastcall m3dDequantize8(int code, float mn, float mx);     // v12 0x00572540 (m3d_quant.cpp)

// v12 0x0042A950  (feb 0x00426400)
// operator new(0x293) and the ctor inline: no weapon, team -1, no power-ups, the floor record cleared; mpType 1
// (a remote player; plrPLAYER::ProcessINIT makes its own body 2).
plrPLAYER_MP *__fastcall plrPLAYER_MP::Create(animINST *)
{
    plrPLAYER_MP *p = new plrPLAYER_MP;                 // (plgsACTION ctor v12 0x00546DE0, not rebuilt)
    if (!p)
        return 0;
    p->skinNo = -1;
    p->moveFlags = 0;
    p->flags2 = 0;
    p->armor = 0;
    p->money = 0;
    p->team = -1;
    p->frags = 0;
    p->unk_217 = 0;
    p->treasures = 0;
    p->unk_21F = -1;
    for (int i = 0; i < 3; i++) {
        p->powerup[i].state = 0;
        p->powerup[i].timeLeft = 0;
    }
    p->cfg.bits = (p->cfg.bits & ~6u) | 1;              // plrPLAYER_COMMON_CFG ctor (v12 0x004265C0)
    p->flags3 = 0;
    p->floorInst = 0;
    p->unk_247 = 0;
    p->unk_24B = 0;
    p->floorPoint = m3dVZero;
    p->unk_25B = 0;
    p->pwrEmitter = 0;
    p->nSkin = 0;
    WR_FIELD(int, p, 0x98) = 1;                         // mpType
    return p;
}

// v12 0x0042AA40  (feb 0x004264B0)
BOOL32 plrPLAYER_MP::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    return plrPLAYER_COMMON::ProcessMsg(id, data, from, ret) != 0;
}

// v12 0x0042AA70  (feb 0x004264E0)
// The shared INIT (sound voices, the weapons); the holder object of each weapon slot (the shovel's, the colt's, the
// machine gun's, the bomb's, every other the bazooka's); the instance out of its own projected shadow, merged with
// its weapons'; the back bones animated by hand; the power-up emitter (0.1);
// the body's class bit (+0xBC 4); the body hidden until the player's ghost state shows it.
BOOL32 plrPLAYER_MP::ProcessINIT()
{
    if (!plrPLAYER_COMMON::ProcessINIT())
        return 0;
    holders = (objOBJ **)apCalloc(0xB, 4);
    if (!holders)
        return 0;
    static const char *h[11] = { "wpn_holder_svl", "wpn_holder_clt", "wpn_holder_bsk", "wpn_holder_mcg",
                                 "wpn_holder_bmb", "wpn_holder_bsk", "wpn_holder_bsk", "wpn_holder_bsk",
                                 "wpn_holder_bsk", "wpn_holder_bsk", "wpn_holder_bsk" };
    static const int at[11] = { 0, 1, 2, 3, 9, 5, 6, 7, 4, 8, 10 };  // (the order the 1.2 code fills them)
    for (int k = 0; k < 11; k++)
        holders[at[k]] = objFindName(inst->root, (char *)h[k]);
    shadowExcl = gsSysProjShd->AddExcludeList(inst, -1);
    for (int i = 0; i < PLR_WPN_COUNT; i++)
        if (weapons[i])
            gsSysProjShd->MergeShadowSources(inst, weapons[i]->inst);
    WR_FIELD(u32, this, 0xBC) |= 4;                     // the entity's class bits
    seqTarget = 1;
    boneCentre = objFindName(inst->root, (char *)"CENTRE");
    boneLowBack = objFindName(inst->root, (char *)"LowBACK");
    boneUpBack = objFindName(inst->root, (char *)"UpBACK");
    boneCentre->IsAncestor(boneUpBack);
    boneLowBack->IsAncestor(boneUpBack);
    boneCentre->DeclareProcAnim();
    boneLowBack->DeclareProcAnim();
    boneUpBack->DeclareProcAnim();
    pwrEmitter = new plrPOWERUP_EMITTER(0.1f);
    if (!pwrEmitter)
        return 0;
    WR_FIELD(u32, pwrEmitter, 4) |= 0x800000;
    inst->flags |= 1;
    return 1;
}

// v12 0x0042AF30  (feb 0x004268F0)
// A body without a name is a desync (logged). Once: a remote player announces itself (0x408). The shared frame;
// the animation; the floor (the local body takes the player's, a remote one looks 50 down from 1.5 up) lights the
// body and, for a remote player, its weapon; the weapon hidden with the body, shown again (alive, not dead / not
// 0x400000), its "dead" bit 2 following the body's; the power-up emitter over the "Skin" objects; the
// respawn blink (moveFlags 0x80000: transparency pulsing at 3 Hz until the game's respawn time).
void plrPLAYER_MP::ProcessFRAME()
{
    const char *nm = inst->name ? inst->name : "";
    if (!*nm)
        apLog((char *)"Player desync detected!!! Please send the log file to developers");
    if (!(flags2 & 0x10)) {
        if (WR_FIELD(int, this, 0x98) == 1)
            msgSystem.SendAll(0x408, this, 0);
        flags2 |= 0x10;
    }
    if (flags2 & 0x40)
        wrEVCall<void>(this, 0x10C, flags2 & 0x80);
    flags2 &= ~0xC0u;
    plrPLAYER_COMMON::ProcessFRAME();
    m3dV pos;
    inst->GetPos(&pos);
    AnimateBody();
    cdtINFO fi;
    memset(&fi, 0, sizeof(fi));
    *(m3dV *)fi.point = m3dVZero;
    cdtREFINE ref(2, inst);
    if (WR_FIELD(int, this, 0x98) == 2)
        wrEVCall<BOOL32>(plrPlayer, 0x44, &floorInfo);   // GetFloorInfo
    else {
        m3dV p;
        inst->GetPos(&p);
        if (gsScenePtr->FloorPlace(&p, 50.0f, &p, 1.5f, 0, 0, &ref, &fi))
            floorInfo = fi;
    }
    ApplyColor(&floorInfo, 1);
    if (WR_FIELD(int, this, 0x98) == 1 && curWeapon >= 0)
        weapons[curWeapon]->ApplyColor(&floorInfo, 1);
    animINST *wi = weapons[curWeapon]->inst;
    if ((inst->flags & 1) || (inst->root->procState & 8))
        wi->flags |= 1;
    else if (!(moveFlags & 2) && !(moveFlags & 0x400000))
        wi->flags &= ~1u;
    if (inst->flags & 2)
        wi->flags |= 2;
    else
        wi->flags &= ~2u;
    if (flags2 & 0x200)
        flags2 &= ~0x300u;
    if ((WR_FIELD(u8, pwrEmitter, 8) & 1) || WR_FIELD(int, pwrEmitter, 0x4C)) {
        for (int i = 0; i < nSkin; i++)
            if (skin[i]->procState & 3) {              // a skin object went away: look them up again
                objFILTER_PART_SOURCE f(2);
                objFILTER_HIDDEN_NAME byName((char *)"Skin");
                objOBJ *src = objFind(inst->root, &byName);
                objFind(src, &f);
                nSkin = f.n;
                for (int k = 0; k < nSkin; k++)
                    skin[k] = f.found[k];
                break;
            }
        m3dV org;
        if (nSkin > 0) {
            inst->Validate(0x204);
            m3dV up;
            skin[0]->GetOrigin(&org);
            skin[0]->GetAxisY(&up);
            inst->GetPos(&org);
            m3dMATR m;
            m._MakeLCS2WCS_VY(&org, &m3dVUnitY);
            pwrEmitter->SetCS(&m);
        }
        partSetAreaEmissionObjList(pwrEmitter, nSkin, skin, 100, &org);
    }
    if (!(moveFlags & 0x80000))
        return;
    int mode = WR_FIELD(int, wrSysGO, 0x98);
    if (mode != 1 && mode != 3)
        return;
    float rt = WR_FIELD(float, this, 0x1D3);            // the time since the respawn
    if (!(rt < WR_FIELD(float, wrSysGO, 0x9C))) {
        moveFlags &= ~0x80000u;
        inst->root->SetTransparency(0, 0xFF);
        for (int i = 0; i < PLR_WPN_COUNT; i++)
            if (weapons[i] && weapons[i]->inst && weapons[i]->inst->root)
                weapons[i]->inst->root->SetTransparency(0, 0xFF);
        return;
    }
    int a = (int)fabsf(sinf(3.1415927f * rt * 3.0f) * 255.0f) & 0xFF;
    inst->root->SetTransparency(0, (u8)a);
    if (weapons[curWeapon] && weapons[curWeapon]->inst && weapons[curWeapon]->inst->root)
        weapons[curWeapon]->inst->root->SetTransparency(0, (u8)a);
}

// v12 0x0042BE10  (feb ?)  (name ours)
// The sequence the player's state asks for, played (unless frozen, 0x10 at +0xB8); the back by hand; the weapon on
// its holder. Then the body's footstep / landing sound events from the action frames of the current mount type's
// sequences (+0x108 bits, as the weapons' Anim2Sound): steps (run 8 / back 0x13: frame 0 -> 1, frame 1 -> 2, both
// 0x10 / 0x20 in water), 0x100 on the jump (0x24), 0x240 / 0x80 crouched on landing (0x2F), 0x800000 (0x47),
// 0x20000000 (0x53).
void plrPLAYER_MP::AnimateBody()
{
    if (!(WR_FIELD(u8, this, 0xB8) & 0x10))
        ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, seqTarget, gsElapsedTime, 0);
    AnimateProcedural();
    UpdateWeapon();
    int b = GetMountType(curWeapon);
    if (inst->IsActionFrame(b + 8, 0) || inst->IsActionFrame(b + 0x13, 0)) {
        if (moveFlags & 0x20000)
            unk108 |= 0x10;
        else if (!(moveFlags & 0x200))
            unk108 |= 1;
        return;
    }
    if (inst->IsActionFrame(b + 8, 1) || inst->IsActionFrame(b + 0x13, 1)) {
        if (moveFlags & 0x20000)
            unk108 |= 0x20;
        else if (!(moveFlags & 0x200))
            unk108 |= 2;
        return;
    }
    if (inst->IsActionFrame(b + 0x24, 0)) {
        unk108 |= 0x100;
        return;
    }
    if (inst->IsActionFrame(b + 0x2F, 0)) {
        unk108 |= (moveFlags & 0x400) ? 0x80 : 0x240;
        return;
    }
    if (inst->IsActionFrame(b + 0x47, 0)) {
        unk108 |= 0x800000;
        return;
    }
    if (inst->IsActionFrame(b + 0x53, 0))
        unk108 |= 0x20000000;
}

// v12 0x0042C000  (feb 0x004272B0)
// The held weapon's model onto the body: at its holder object's matrix, then scaled and moved to fit the hand
// (per weapon). Not while the body is hidden, nor -- for a weapon kept to the mirror pass with the body -- when no
// mirror is active.
void plrPLAYER_MP::UpdateWeapon()
{
    if (inst->flags & 1)
        return;
    animINST *wi = weapons[curWeapon]->inst;
    u32 wf = wi->flags;
    if ((wf & 0x100000) && (wf & 0x2000000) && (inst->flags & 0x2000000) && gsSysMirror &&
        !gsSysMirror->GetNActive())
        return;
    inst->Validate(4);
    wi->Transform((m3dMATR *)holders[curWeapon]->ltm, 0);
    switch (curWeapon) {
    case 0:  wi->Scale(5.0f, 5.0f, 5.0f, 0); break;
    case 1:  wi->Scale(1.4f, 1.4f, 1.4f, 0); wi->Translate(0.0f, 1.0f, -0.2f, 1); break;
    case 2:  wi->Scale(1.25f, 1.25f, 1.25f, 0); wi->Translate(0.0f, -1.0f, -1.0f, 1); break;
    case 3:  wi->Scale(7.5f, 7.5f, 7.5f, 0); wi->Translate(0.05f, -0.5f, -7.3f, 1); break;
    case 4:  wi->Scale(1.0f, 1.0f, 1.0f, 0); wi->Translate(-0.1f, -1.5f, -4.0f, 1); break;
    case 5:  wi->Scale(3.3000002f, 3.3000002f, 3.3000002f, 0); wi->Translate(-0.1f, -1.9f, -0.8f, 1); break;
    case 6:  wi->Scale(0.14f, 0.14f, 0.14f, 0); wi->Translate(0.05f, -2.5f, -0.5f, 1); break;
    case 7:  wi->Scale(0.33f, 0.33f, 0.33f, 0); wi->Translate(0.3f, -2.5f, -0.7f, 1); break;
    case 8:  wi->Scale(1.0f, 1.0f, 1.0f, 0); wi->Translate(-0.5f, -1.9f, 0.0f, 1); break;
    case 9:  wi->Scale(1.55f, 1.55f, 1.55f, 0); break;
    case 10: wi->Scale(1.4f, 1.4f, 1.4f, 0); wi->Translate(-0.5f, -1.0f, 0.3f, 1); break;
    }
}

// v12 0x0042D050  (feb 0x004280E0)
// The back by hand, eased at 350 degrees a second: the centre leans about Y with the strafe (moving forward
// -40 / +40 strafing, else -60; backing up +40 / -40, else +60; dead upright); the view's turn from the legs plus
// that lean twists the lower back by -0.55 and the upper by -0.3 of it (unless standing still facing the legs);
// the pitch bends both about the body's right axis (looking down -0.4, up -1.167 a degree; 0.65 of it the lower
// back, 0.2 the upper), not while backing up or dead.
void plrPLAYER_MP::AnimateProcedural()
{
    float twistLow = 0, twistUp = 0, bend = 0;
    if (inst->flags & 1)
        return;
    inst->Invalidate(8);
    inst->Validate(8);
    objValidateLTM(boneUpBack);
    u32 f = unk_1F7;
    float lean = 0;
    if (f & 2)
        lean = (f & 4) ? -40.0f : (f & 8) ? 40.0f : -60.0f;
    else if (f & 1)
        lean = (f & 4) ? 40.0f : (f & 8) ? -40.0f : 60.0f;
    if (wrEVCall<BOOL32>(this, 0x74))                   // IsDead
        lean = 0;
    m3dLimitChange_Float(unk_1BF[0], lean, gsElapsedTime * 350.0f, &lean);
    unk_1BF[0] = lean;
    boneCentre->RotateGlbAxis(lean, &m3dVUnitY);
    m3dV dir, axis;
    inst->GetDir(&dir);
    axis = m3dVUnitY;
    float twist = bodyTwist + lean;
    m3dNormalize(&axis);
    if (twistLow < 0.0f || (unk_1F7 & 2) || twist > 0.0f || (unk_1F7 & 1)) {
        twistLow = twist * -0.55f;
        twistUp = twist * -0.3f;
    }
    m3dLimitChange_Float(unk_1BF[2], twistLow, gsElapsedTime * 350.0f, &twistLow);
    m3dLimitChange_Float(unk_1BF[1], twistLow, gsElapsedTime * 350.0f, &twistUp);   // (the 1.2 code eases the
    boneLowBack->RotateGlbAxis(twistLow, &axis);                                     //  upper half to the lower's)
    boneUpBack->RotateGlbAxis(twistUp, &axis);
    unk_1BF[1] = twistUp;
    unk_1BF[2] = twistLow;
    inst->GetDirRight(&axis);
    if (unk_1F7 & 1)
        bend = 0;
    else
        bend = pitch > 90.0f ? (pitch - 90.0f) * -0.4f : (pitch - 90.0f) * -1.1666666f;
    if (wrEVCall<BOOL32>(this, 0x74))
        bend = 0;
    m3dLimitChange_Float(unk_1BF[3], bend, gsElapsedTime * 350.0f, &bend);
    unk_1BF[3] = bend;
    boneLowBack->RotateGlbAxis(bend * 0.65f, &axis);
    boneUpBack->RotateGlbAxis(bend * 0.2f, &axis);
    inst->Invalidate(4);
}

// v12 0x0042BD40  (name ours)  slot +0x12C
// The player's animation state from its sync block: the sequence (byte 0x4F), the pitch (byte 0x52 over 0..180),
// the move bits (byte 0x50: bits 2..5 -> unk_1F7 low nibble; bit 6 -> moveFlags 0x800, bit 7 -> 0x400) and the
// two overlay channels restarted (bits 0 / 1: the machine gun's and the bomb's shot).
void plrPLAYER_MP::ReadAnimSync(u8 *s)
{
    seqTarget = (signed char)s[0x4F];
    pitch = m3dDequantize8(s[0x52], 0.0f, 180.0f);
    unk_1F7 = (unk_1F7 & ~0xFu) | (u32)((signed char)(s[0x50] << 2) >> 4);
    moveFlags = (s[0x50] & 0x40) ? moveFlags | 0x800 : moveFlags & ~0x800u;
    moveFlags = (s[0x50] & 0x80) ? moveFlags | 0x400 : moveFlags & ~0x400u;
    int bits = (signed char)(s[0x50] << 6) >> 6;
    for (int i = 0; i < 2; i++)
        if (bits & (1 << i))
            ((animCHN *)((char *)((animSYSTEM_INST *)inst->link)->chn + i * 0x18))->Start();
}

// v12 0x0042AD00  (feb 0x00426790)
// A remote player leaves (0x409); the power-up emitter deleted; every weapon hidden; the holders freed; the shared TERM.
void plrPLAYER_MP::ProcessTERM()
{
    if (flags2 & 0x10)
        msgSystem.SendAll(0x409, this, 0);
    if (pwrEmitter) {
        wrVCall<void *>(pwrEmitter, 0, 1u);             // its deleting dtor
        pwrEmitter = 0;
    }
    for (int i = 0; i < 11; i++)
        weapons[i]->inst->flags |= 1;
    apFree(holders);
    holders = 0;
    plrPLAYER_COMMON::ProcessTERM();
}

// v12 0x0042AD80  (feb 0x00426810)
// The shared INIT_LEVEL; facing the legs; a remote player lit from where it stands; nothing dead (flags 2) or
// blinking any more, fully opaque; a remote player's power-up glow stopped.
BOOL32 plrPLAYER_MP::ProcessINIT_LEVEL()
{
    plrPLAYER_COMMON::ProcessINIT_LEVEL();
    bodyTwist = 0;
    if (WR_FIELD(int, this, 0x98) == 1) {
        ApplyColor(&floorInfo, 0);
        if (curWeapon >= 0)
            weapons[curWeapon]->ApplyColor(&floorInfo, 0);
    }
    inst->flags &= ~2u;
    for (int i = 0; i < 11; i++)
        weapons[i]->inst->flags &= ~2u;
    moveFlags &= ~0x80000u;
    inst->root->SetTransparency(0, 0xFF);
    for (int i = 0; i < PLR_WPN_COUNT; i++)
        if (weapons[i] && weapons[i]->inst && weapons[i]->inst->root)
            weapons[i]->inst->root->SetTransparency(0, 0xFF);
    if (WR_FIELD(int, this, 0x98) == 1)
        pwrEmitter->StopEmission();
    return 1;
}

// v12 0x0042AE80  (feb 0x00426840)
BOOL32 plrPLAYER_MP::ProcessINIT_MAP()
{
    plrPLAYER_COMMON::ProcessINIT_MAP();
    return 1;
}

// v12 0x0042AE90  (feb 0x00426850)
// The shared INIT_GAME; the local body's weapons switched on; announced again on the next frame.
BOOL32 plrPLAYER_MP::ProcessINIT_GAME()
{
    plrPLAYER_COMMON::ProcessINIT_GAME();
    if (WR_FIELD(int, this, 0x98) == 2)
        for (int i = 0; i < 11; i++)
            weapons[i]->Enable(1);
    flags2 &= ~0x10u;
    return 1;
}

// v12 0x0042AEE0  (feb 0x004268A0)
// A remote player's copy and its weapons are no longer kept (flags2 4) once the game ends.
void plrPLAYER_MP::ProcessTERM_GAME()
{
    if (WR_FIELD(int, this, 0x98) != 1)
        return;
    entENTITY::flags2 &= ~4u;
    for (int i = 0; i < 11; i++)
        weapons[i]->entENTITY::flags2 &= ~4u;
}

// v12 0x0042BB30
// On the server, a remote player remembers where it was put (answering 0x415 with it).
void plrPLAYER_MP::SetPosDir(m3dV *pos, m3dV *, int)
{
    if (!(entFlags & 4) || WR_FIELD(int, this, 0x98) != 1)
        return;
    flags2 |= 0x100;
    *(m3dV *)((char *)this + 0x1E7) = *pos;
}

// v12 0x0042BB70  (feb 0x00427010)
void plrPLAYER_MP::SetDefaultValues()
{
    plrPLAYER_COMMON::SetDefaultValues();
    pwrEmitter->StopEmission();
}

// v12 0x0042BB90  (feb 0x00427030)
// w -1: the weapon hidden. Another enabled weapon, alive: the shared switch (the animation group), the old one hidden,
// the new one shown and dead with the body (flags 2).
void plrPLAYER_MP::SetCurWpn(int w, int animate)
{
    if (w == -1) {
        weapons[curWeapon]->inst->flags |= 1;
        return;
    }
    if (w == curWeapon)
        return;
    pwpWEAPON *nw = weapons[w];
    if (!nw || !(WR_FIELD(u8, nw, 0x16B) & 1) || IsDead())
        return;
    plrPLAYER_COMMON::SetCurWpn(w, animate);
    if (weapons[curWeapon])
        weapons[curWeapon]->inst->flags |= 1;
    curWeapon = w;
    weapons[w]->inst->flags &= ~1u;
    if (inst->flags & 2)
        weapons[curWeapon]->inst->flags |= 2;
    else
        weapons[curWeapon]->inst->flags &= ~2u;
}

// v12 0x0042C2D0  (feb 0x00427580)
void plrPLAYER_MP::SetupGhostWpnState()
{
    for (int i = 0; i < 11; i++)
        weapons[i]->inst->flags |= 0x100000;
}

// v12 0x0062C000  (feb 0x005EEBE0)
// Skin_03 twice: the 5th outfit is the 3rd model with Emma's second texture set, and without obj43.
plrPLAYER_MP::SKIN plrPLAYER_MP::arraySkin[6] = {
    { "Skin_01" },
    { "Skin_02" },
    { "Skin_03" },
    { "Skin_04" },
    { "Skin_03", 2, { (char *)"Emma_skin1_01", (char *)"Emma_skin1_02" }, { (char *)"Emma_skin2_01", (char *)"Emma_skin2_02" },
      1, { (char *)"obj43" } },
    { "Skin_05" },
};

// v12 0x0062C450  (feb 0x005EF030)
// Team 0 red (_r), team 1 blue (_b); the matador is red already, Emma's second blue texture is her own.
plrPLAYER_MP::TEAM plrPLAYER_MP::arrayTeam[6] = {
    { 2, { (char *)"WillRock01", (char *)"WillRock02" },
      { { (char *)"WillRock01_r", (char *)"WillRock02_r" }, { (char *)"WillRock01_b", (char *)"WillRock02_b" } } },
    { 2, { (char *)"Cowboy_01", (char *)"Cowboy_02" },
      { { (char *)"Cowboy_01_r", (char *)"Cowboy_02_r" }, { (char *)"Cowboy_01_b", (char *)"Cowboy_02_b" } } },
    { 2, { (char *)"Emma_skin1_01", (char *)"Emma_skin1_02" },
      { { (char *)"Emma_skin1_01_r", (char *)"Emma_skin1_02_r" }, { (char *)"Emma_skin1_01_b", (char *)"Emma_skin1_02" } } },
    { 3, { (char *)"WillRockSkin1_01", (char *)"WillRockSkin1_02", (char *)"WillRockSkin1_03" },
      { { (char *)"WillRockSkin1_01_r", (char *)"WillRockSkin1_02_r", (char *)"WillRockSkin1_03_r" },
        { (char *)"WillRockSkin1_01_b", (char *)"WillRockSkin1_02_b", (char *)"WillRockSkin1_03_b" } } },
    { 2, { (char *)"Emma_skin2_01", (char *)"Emma_skin2_02" },
      { { (char *)"Emma_skin2_01_r", (char *)"Emma_skin2_02_r" }, { (char *)"Emma_skin2_01_b", (char *)"Emma_skin2_02_b" } } },
    { 2, { (char *)"matador1", (char *)"matador2" },
      { { (char *)"matador1", (char *)"matador2" }, { (char *)"matador1_b", (char *)"matador2_b" } } },
};

// v12 0x0042C6D0  (feb 0x00427820)
// Out of range: the first. Everything shown, then the team's textures and the old outfit's replacements put back;
// every Skin_0N but the new one's model hidden (procState 8), its parts hidden and its textures replaced.
// (The team textures are put back by the outfit worn so far -- with no outfit yet, -1, the game reads the entry before
// the table: in TH the team is only set with an outfit worn.)
void plrPLAYER_MP::SelectSkin(int s)
{
    if (s <= -1 || s >= 6)
        s = 0;
    objOBJ *root = inst->root;
    root->SetStateProcNo(8, 0);
    if (team != -1)
        for (int i = 0; i < arrayTeam[skinNo].n; i++)
            inst->root->ReplaceTex(arrayTeam[skinNo].teamTex[team][i], arrayTeam[skinNo].tex[i]);
    if (skinNo != -1 && skinNo != s)
        for (int i = 0; i < arraySkin[skinNo].nRepl; i++)
            inst->root->ReplaceTex(arraySkin[skinNo].to[i], arraySkin[skinNo].from[i]);
    skinNo = s;
    SKIN *cur = &arraySkin[s];
    for (int k = 0; k < 6; k++) {
        objOBJ *o = objFindName(inst->root, arraySkin[k].name);
        if (k != skinNo && strcmp(arraySkin[k].name, cur->name))
            o->SetStateProcYes(8, 0);
        else
            o->SetStateProcNo(8, 0);
    }
    for (int i = 0; i < cur->nHide; i++) {
        objOBJ *o = objFindNameN(inst->root, cur->hide[i], strlen(cur->hide[i]));
        if (o)
            o->SetStateProcYes(8, 0);
    }
    for (int i = 0; i < cur->nRepl; i++)
        inst->root->ReplaceTex(cur->from[i], cur->to[i]);
}

// v12 0x0042C900  (feb 0x00427A40)
// The TH team (out of range: 0): the outfit's textures to the team's colour, the old team's put back first.
void plrPLAYER_MP::SetTeam(int t)
{
    if (t <= -1 || t >= 2)
        t = 0;
    if (skinNo <= -1 || skinNo >= 6)
        return;
    TEAM *e = &arrayTeam[skinNo];
    if (team != -1)
        for (int i = 0; i < e->n; i++)
            inst->root->ReplaceTex(e->teamTex[team][i], e->tex[i]);
    for (int i = 0; i < e->n; i++)
        inst->root->ReplaceTex(e->tex[i], e->teamTex[t][i]);
    plrPLAYER_COMMON::SetTeam(t);
}

// the small slots
void   plrPLAYER_MP::GetPos(m3dV *out) { inst->GetPos(out); }                 // v12 0x0041DE70
void   plrPLAYER_MP::SetPos(m3dV *) {}                                        // v12 0x00403950
void   plrPLAYER_MP::GetDir(m3dV *out) { inst->GetDir(out); }                 // v12 0x0041DE80
BOOL32 plrPLAYER_MP::IsDead() { return moveFlags & 2; }                       // v12 0x00425E50
BOOL32 plrPLAYER_MP::IsActive() { return plgsACTION::IsActive(); }           // v12 0x00546E40
BOOL32 plrPLAYER_MP::IsRemote() { return WR_FIELD(int, this, 0x98) != 2; }    // v12 0x0042C670
BOOL32 plrPLAYER_MP::Slot90() { return flags2 & 8; }                          // v12 0x00425E60
BOOL32 plrPLAYER_MP::HasKey() { return 0; }                                   // v12 0x0048DF30
void   plrPLAYER_MP::UseKey() {}                                              // v12 0x00403A20
BOOL32 plrPLAYER_MP::Pick(entENTITY *e) { return plrPLAYER_COMMON::Pick(e); }  // v12 0x00428100
BOOL32 plrPLAYER_MP::IsStayOn(animINST *, objOBJ *, int) { return 0; }        // v12 0x00403970
void   plrPLAYER_MP::ApplySpeed(m3dV *, int) {}                               // v12 0x00403860
float  plrPLAYER_MP::GetGravity() { return plgsACTION::GetGravity(); }        // v12 0x00425E20
void   plrPLAYER_MP::ProcessTERM_LEVEL() { plrPLAYER_COMMON::ProcessTERM_LEVEL(); }  // v12 0x00427590
BOOL32 plrPLAYER_MP::Slot0B8() { return 1; }                                  // v12 0x00434800
void   plrPLAYER_MP::ProcessSHOOT(void *m) { plrPLAYER_COMMON::ProcessSHOOT(m); }            // v12 0x00428150
void   plrPLAYER_MP::ProcessSHOOT_BODY(void *m) { plrPLAYER_COMMON::ProcessSHOOT_BODY(m); }  // v12 0x004281C0
void   plrPLAYER_MP::Slot0D8(void *) {}                                       // v12 0x00403950
void   plrPLAYER_MP::Slot100(m3dV *out) { *out = m3dVZero; }                  // v12 0x0042C640
void   plrPLAYER_MP::Slot120() { if (WR_FIELD(int, this, 0x98) != 2) UpdateSound(); } // v12 0x0042C5B0 (not the local body)
void   plrPLAYER_MP::NotifyHealthChange(dmgDAMAGE *d) { plrPLAYER_COMMON::NotifyHealthChange(d); } // v12 0x00429040
BOOL32 plrPLAYER_MP::IsThirdPerson() { return 1; }                            // v12 0x004031F0

// the m3dSPL_LINEAR1D constructor (inline: m3dSPL::ctor(0, 8, 1) v12 0x00403670) with n key points
static m3dSPL_LINEAR1D *pwrSpl(int n)
{
    m3dSPL_LINEAR1D *s = new m3dSPL_LINEAR1D;
    if (!s)
        return 0;
    s->flags = 0;
    s->type = 0;
    s->nKp = 0;
    s->kpValueSize = 8;
    s->kpStride = 8;
    s->dim = 1;
    s->kpBytes = 0;
    s->kp = 0;
    s->pad[0] = s->pad[1] = s->pad[2] = 0;
    s->AllocKpList(n);
    return s;
}

// v12 0x0042C3F0  (feb 0x00427630)
// The power-up glow: up to 300 a second (1.2 s), 0.9 s (+0.3) each, rate * 3 big shrinking to nothing, fading in to
// 178 at 0.3 of their life and out, turning 180 degrees a second (600 random), rising (rate * 20), from the skin's
// area, fading with the distance (near 3 / 0.2, far 7 - 30, 4, 0.6).
plrPOWERUP_EMITTER::plrPOWERUP_EMITTER(float rate) : partEMITTER_PHYS(300.0f, 1.2f)
{
    SetTimeLifePart(0.9f, 0.3f);
    m3dSPL_LINEAR1D *sz = pwrSpl(2);
    float s0 = rate * 3.0f;
    sz->SetKp(0, 0.0f, s0);
    sz->SetKp(1, 1.0f, rate * 0.0f);
    SetSize(s0, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = pwrSpl(3);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 0.295f, 178.5f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 600.0f);
    flags |= 4;
    gravity.x = 0.0f;
    gravity.y = rate * 20.0f;
    gravity.z = 0.0f;
    flags |= 8;
    flags |= 1;
    flags |= 0x2000;
    flags |= 0x8000;
    flags |= 0x4000000;
    SetDegFarParams(7.0f, 30.0f, 4.0f, 0.6f);
    SetDegNearParams(3.0f, 0.2f);
}
