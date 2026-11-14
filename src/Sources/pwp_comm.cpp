// pwp_comm.cpp -- pwpWEAPON, the base of the player's weapons: clip and ammo, the weapon animation set and its
// transitions, the weapon sound channel, the class registration of every player weapon
// (Will Rock 1.2, v12.exe; feb obj pwp_comm.obj)
//
// Rebuilt from the 1.2 code (v12 0x00416EC0..0x0041810F; vtable 0x005F9580). Fields at feb + 0x1C. 1.2 changes,
// from the disassembly:
//   - ProcessFRAME now sets the sound channel's mute flag and the model's render state 0x1000000 (hidden) from
//     IsWeaponMP every frame (feb: once, in ProcessINIT_LEVEL); ProcessINIT_LEVEL just returns 1 and
//     ProcessTERM_LEVEL stops the sound; new slot +0xCC stops it too;
//   - ProcessMsg: message 0x4C8 goes to the new slot ShootBody (+0xB4);
//   - SetupAniSystem frees the template's old sequence array and passes 0 as the last argument of the last
//     transition; pwpInit registers two new MP message packers.
// Not rebuilt here yet: the two 1.2 shot-message packers (v12 0x00417000..0x0041762F; objects at v12 0x00641884 /
// 0x0064188C, vtables 0x005FAF7C / 0x005FAF74; positions through mpQuantPos, directions in 3 bytes).

#include "game.h"
#include "../wr_vcall.h"

extern wrGO_SYSTEM *wrSysGO;                // v12 0x0064FF5C
extern u32 gsAppState;
extern msgDATA_PACK pwpPackShot;            // v12 0x0064188C (1.2 only, name ours) -- not rebuilt
extern msgDATA_PACK pwpPackShot2;           // v12 0x00641884 (1.2 only, name ours) -- not rebuilt

int __fastcall pwpSVLInit();  int __fastcall pwpBSKInit();  int __fastcall pwpBulWpnInit();
int __fastcall pwpMCGInit();  int __fastcall pwpCLTInit();  int __fastcall pwpSTGInit();
int __fastcall pwpMDGInit();  int __fastcall pwpMNGInit();  int __fastcall pwpCRBInit();
int __fastcall pwpBMBInit();  int __fastcall pwpACDInit();  int __fastcall pwpAMGInit();
int __fastcall pwpOppSfxInit(); int __fastcall pwpWaterSfxInit();

// v12 0x00416F00  (feb 0x00415170)
int __fastcall pwpInit()
{
    if (!pwpSVLInit() || !pwpBSKInit() || !pwpBulWpnInit() || !pwpMCGInit() || !pwpCLTInit() || !pwpSTGInit() ||
        !pwpMDGInit() || !pwpMNGInit() || !pwpCRBInit() || !pwpBMBInit() || !pwpACDInit() || !pwpAMGInit() ||
        !pwpOppSfxInit() || !pwpWaterSfxInit())
        return 0;
    msgSystem.RegisterMsgData(&pwpPackShot);
    msgSystem.RegisterMsgData(&pwpPackShot2);
    return 1;
}

// v12 0x00416FB0  (feb 0x00415200)
// The game calls the 14 Term functions of the weapon objects here; in 1.2 every one of them is the shared empty
// function (v12 0x00401380), so nothing happens.
void __fastcall pwpTerm()
{
}

// v12 0x00417630  (feb 0x00415250)
// Refill the clip from the ammo: as many rounds as the ammo outside the clip and the room in the clip allow.
void pwpWEAPON::Recharge()
{
    int c = clip, size = clipSize;
    if (c == size)
        return;
    int n = (int)(ammo - (float)c);
    if (n <= 0)
        n = 0;
    if (size - c < n)
        n = size - c;
    clip = c + n;
}

// v12 0x00417680  (feb 0x004152A0)
// Infinite ammo (wrGO_SYSTEM +0xA4) takes nothing; else the ammo, then the clip.
int pwpWEAPON::SubtractAmmo(float n)
{
    if (!*(int *)((char *)wrSysGO + 0xA4)) {
        if (!wpnWEAPON::SubtractAmmo(n))
            return 0;
        if (clip != -1)
            clip -= (int)n;
    }
    return 1;
}

// v12 0x004176D0  (feb 0x004152F0)
void pwpWEAPON::SetShootRate(float r)
{
    shootRate = r;
    baseShootRate = r;
}

// v12 0x004176F0  (feb 0x00415310)
// x4 with the owner's powerup 0 (the Titan).
float pwpWEAPON::GetDamage()
{
    float d = damage;
    if (owner->IsActivePowerup(0))
        return d * 4.0f;
    return d;
}

// v12 0x00417720  (feb 0x00415340)
// The speed of the weapon's sequences per weapon (the lengths are scaled): TAKE_OFF, TAKE_OFF_EMPTY and PUT_ON by
// `sw`, SHOOT by `sh` (and sequence 10 for the minigun / atomic gun), RECHARGE and FORCE_RECHARGE by `rc`.
void pwpWEAPON::AdjustTiming()
{
    animTPL *t = inst->tpl;
    float sh = 1.0f, rc = 0.5f, sw = 0.4f;
    switch (wpnIndex) {
    case 0:
        sh = 0.7f;
        rc = 0.7f;
        break;
    case 1:
        rc = 0.7f;
        break;
    case 2:
        sh = 0.6f;
        break;
    case 3:
        sw = 0.7f;
        break;
    case 4:
        sw = 0.6f;
        sh = 0.6f;
        rc = 0.7f;
        break;
    case 5:
        sw = 0.35f;
        rc = 0.6f;
        break;
    case 6:
        rc = 1.0f;
        break;
    case 7:
        sw = 0.5f;
        sh = 0.5f;
        rc = 0.8f;
        break;
    case 8:
        sw = 0.65f;
        rc = 0.8f;
        break;
    case 9:
        sw = 0.35f;
        sh = 0.5f;
        break;
    case 10:
        sw = 0.55f;
        sh = 0.7f;
        break;
    }
    t->seq[4].lengthMs = sw * t->seq[4].lengthMs;
    t->seq[5].lengthMs = sw * t->seq[5].lengthMs;
    t->seq[6].lengthMs = sw * t->seq[6].lengthMs;
    t->seq[7].lengthMs = sh * t->seq[7].lengthMs;
    t->seq[8].lengthMs = rc * t->seq[8].lengthMs;
    t->seq[9].lengthMs = rc * t->seq[9].lengthMs;
    if (wpnIndex == 4)
        t->seq[10].lengthMs = sh * t->seq[10].lengthMs;
    if (wpnIndex == 10)
        t->seq[10].lengthMs = sh * t->seq[10].lengthMs;
}

// v12 0x00417920  (feb 0x00415520)
// The weapon template gets the 10 PWP_SEQ_* slots (its own sequences assigned to them by name) and the transition
// table: idle <-> run, take off / put on, shoot, the two recharges. Not for a remote weapon unless gsAppState 0x800.
BOOL32 pwpWEAPON::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    if ((entFlags & ENTF_REMOTE) && !(gsAppState & 0x800))
        return 1;
    animSEQ *old = 0;
    int nOld = 0;
    if (t->seq) {
        nOld = t->nSeq;
        old = t->seq;
    }
    t->nSeq = 10;
    int *blk = (int *)operator new(0x554);  // MSVC array: count, then 10 x 0x88
    animSEQ *seq = 0;
    if (blk) {
        blk[0] = 10;
        seq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 10; i++) {
            memset(&seq[i], 0, sizeof(animSEQ));
            seq[i].dir = 2;
        }
    }
    t->seq = seq;
    _entAnimSetSeqName(t, 0, (char *)"PWP_SEQ_UNUSED");
    _entAnimSetSeqName(t, 1, (char *)"PWP_SEQ_IDLE");
    _entAnimSetSeqName(t, 2, (char *)"PWP_SEQ_IDLE_1");
    _entAnimSetSeqName(t, 3, (char *)"PWP_SEQ_RUN");
    _entAnimSetSeqName(t, 4, (char *)"PWP_SEQ_TAKE_OFF");
    _entAnimSetSeqName(t, 5, (char *)"PWP_SEQ_TAKE_OFF_EMPTY");
    _entAnimSetSeqName(t, 6, (char *)"PWP_SEQ_PUT_ON");
    _entAnimSetSeqName(t, 7, (char *)"PWP_SEQ_SHOOT");
    _entAnimSetSeqName(t, 8, (char *)"PWP_SEQ_RECHARGE");
    _entAnimSetSeqName(t, 9, (char *)"PWP_SEQ_FORCE_RECHARGE");
    entAnimAssignTplSeq(t, old, nOld);
    t->seq[0].frameStart = 0.0f;
    t->seq[0].frameEnd = 1.0f;
    t->seq[0].lengthMs = 500.0f;
    t->seq[0].dir = 0;
    AdjustTiming();
    animSYSTEM *as = new animSYSTEM(10, 0);
    if (!as)
        return 0;
    *(animSYSTEM **)((char *)t + 0xA4) = as;
    wrVCall<void>(as, 0xC, 0, 0, 1, -1);
    wrVCall<void>(as, 0xC, 1, 0, 1, -1);
    wrVCall<void>(as, 0x14, 1);
    wrVCall<void>(as, 0x14, 3);
    wrVCall<void>(as, 4, 3, 1, 0, 1, 1);
    wrVCall<void>(as, 4, 3, 4, 0, 4, 1);
    wrVCall<void>(as, 4, 3, 6, 0, 6, 1);
    wrVCall<void>(as, 4, 3, 7, 0, 7, 1);
    wrVCall<void>(as, 0xC, 4, 1, 1, -1);
    wrVCall<void>(as, 4, 4, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 6, 3, 1, 3, 1);
    wrVCall<void>(as, 4, 6, 1, 0, 1, 1);
    wrVCall<void>(as, 4, 6, 6, 2, 6, 1);
    wrVCall<void>(as, 4, 6, 7, 1, 7, 1);
    wrVCall<void>(as, 0xC, 7, 1, 1, -1);
    wrVCall<void>(as, 4, 7, 1, 1, 1, 0);
    wrVCall<void>(as, 4, 7, 3, 1, 1, 0);
    wrVCall<void>(as, 4, 7, 7, 1, 7, 0);
    wrVCall<void>(as, 4, 7, 8, 0, 8, 1);
    wrVCall<void>(as, 4, 1, 7, 0, 7, 0);
    wrVCall<void>(as, 4, 3, 7, 0, 7, 0);
    wrVCall<void>(as, 0xC, 8, 1, 1, -1);
    wrVCall<void>(as, 4, 4, 1, 0, 1, 0);
    wrVCall<void>(as, 4, 4, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 1, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 3, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 7, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 8, 6, 0, 6, 0);
    wrVCall<void>(as, 0x20, 1, 2);
    wrVCall<void>(as, 4, 2, 1, 1, 1, 1);
    if (t->seq[5].dir == 2)                 // TAKE_OFF_EMPTY missing: a copy of TAKE_OFF
        memcpy(&t->seq[5], &t->seq[4], sizeof(animSEQ));
    wrVCall<void>(as, 0x20, 4, 5);
    if (t->seq[9].dir == 2)                 // FORCE_RECHARGE missing: a copy of RECHARGE
        memcpy(&t->seq[9], &t->seq[8], sizeof(animSEQ));
    wrVCall<void>(as, 0x20, 8, 9);
    wrVCall<void>(as, 4, 3, 9, 0, 9, 0);
    if (old)
        operator delete((int *)old - 1);
    return 1;
}

// the filter of ProcessINIT: transparent parts (render state 0x20) drawn last (0x8000000). vtable v12 0x005FAF94
class objFILTER_SET_TRANSP_LAST : public objFILTER {
public:
    BOOL32 IsUse(objOBJ *o);
};

// v12 0x00417F20  (feb 0x00415B40)
BOOL32 objFILTER_SET_TRANSP_LAST::IsUse(objOBJ *o)
{
    if (o->rendState & 0x20)
        o->SetStateRendYes(0x8000000);
    return 0;
}

// v12 0x00417D50  (feb 0x00415970)
// No clip (-1), the sound channel (a sphere, 10..50); the local player's own weapon is drawn last, without the
// hands of a remote copy's view (render state 0x1000000) and with its transparent parts last.
int pwpWEAPON::ProcessINIT()
{
    objFILTER_SET_TRANSP_LAST f;
    if (!wpnWEAPON::ProcessINIT())
        return 0;
    unk108 = 0;
    lastAnimEvent = 0;
    inst->flags |= 0x10;
    inst->flags |= 1;
    entENTITY::flags2 |= 4;
    clipSize = -1;
    clip = -1;
    reloadActFrame = 0;
    unk1BB = 0;
    shootActFrame = -1;
    pwpWEAPON::flags2 = 0;
    sndChannel = new sncCHANNEL_CONE;
    if (!sndChannel)
        return 0;
    if (!IsWeaponMP()) {
        ((sncCHANNEL *)sndChannel)->flags |= 1;
        inst->root->SetStateRendYes(0x1000000);
        objFind(inst->root, &f);
    }
    gsSysSound->AddChannel((sncCHANNEL *)sndChannel, this, 0);
    return 1;
}

// v12 0x00548680 (the shared "return 1")
int pwpWEAPON::ProcessINIT_LEVEL()
{
    return 1;
}

// v12 0x00417F40
void pwpWEAPON::ProcessTERM_LEVEL()
{
    if (sndChannel)
        ((sncCHANNEL *)sndChannel)->Stop();
}

// v12 0x00417F50  (feb 0x00415BB0)
// A remote player's weapon: sound positioned in 3D and the model drawn; the local one: plain sound, model hidden
// from the world render (state 0x1000000, it is drawn by the player's view).
void pwpWEAPON::ProcessFRAME()
{
    wpnWEAPON::ProcessFRAME();
    if (sndChannel) {
        if (IsWeaponMP())
            ((sncCHANNEL *)sndChannel)->flags &= ~1u;
        else
            ((sncCHANNEL *)sndChannel)->flags |= 1;
    }
    if (IsWeaponMP()) {
        if (inst->root->rendState & 0x1000000)
            inst->root->SetStateRendNo(0x1000000);
    } else if (!(inst->root->rendState & 0x1000000))
        inst->root->SetStateRendYes(0x1000000);
    Anim2Sound();
    UpdateSound();
}

// v12 0x00417FF0  (feb 0x00415BD0)
BOOL32 pwpWEAPON::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x4C8) {
        ShootBody(data);
        return 0;
    }
    return wpnWEAPON::ProcessMsg(id, data, from, ret);
}

// v12 0x00418020  (feb 0x00415BE0)
// With PWPF2_SHOOT_ON_ACTFRAME the shot waits for the SHOOT sequence's action frame.
BOOL32 pwpWEAPON::IsShooting()
{
    if (!(pwpWEAPON::flags2 & PWPF2_SHOOT_ON_ACTFRAME))
        return 1;
    return inst->IsActionFrame(7, shootActFrame) != 0;
}

// v12 0x00418050  (feb 0x00415C10)
BOOL32 pwpWEAPON::IsReadyRecharge()
{
    return inst->IsActionFrame(7, reloadActFrame) != 0;
}

// v12 0x00418070  (feb 0x00415C30)
void pwpWEAPON::SetZoom(int on)
{
    if (on)
        pwpWEAPON::flags2 |= PWPF2_ZOOM;
    else
        pwpWEAPON::flags2 &= ~PWPF2_ZOOM;
}

// v12 0x004180A0  (feb 0x00415C60)
// Latches the entity event of this frame (+0x108) for the sounds.
void pwpWEAPON::UpdateSound()
{
    int e = unk108;
    unk108 = 0;
    lastAnimEvent = e;
}

// v12 0x004180C0  (feb 0x00415C80)
int pwpWEAPON::IsEmpty()
{
    return !(wpnWEAPON::flags & 4) && ammo <= 0.0f;
}

// v12 0x004180F0 (1.2 only)
void pwpWEAPON::StopSound()
{
    ((sncCHANNEL *)sndChannel)->Stop();
}

// v12 0x00403A20 / 0x00403950 / 0x00403B10 / 0x00403B40 / 0x00403B50 (header-inline in the game)
void pwpWEAPON::Anim2Sound() {}
void pwpWEAPON::ShootBody(void *) {}

void pwpWEAPON::AdvanceAnimation(int s, float dt)
{
    ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, s, dt * unk1C3, 0);
}

int pwpWEAPON::GetCurAnimSeq()
{
    return inst->seq;
}

BOOL32 pwpWEAPON::IsWeaponMP()
{
    return entENTITY::flags2 & 4;
}
