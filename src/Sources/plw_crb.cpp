// plw_crb.cpp -- pwpCROSSBOW, the crossbow (weapon 4): six explosive bolts, shown on the bow while loaded, the empty
// clip thrown out on reload (Will Rock 1.2, v12.exe; feb obj plw_crb.obj)
//
// Rebuilt from the 1.2 code (v12 0x0040CFF0..0x0040FC2A, vtable 0x005FA500): the class, its sequences, INIT /
// FRAME / sound slots and the clip. Not rebuilt: the shot (Shoot +0x68 v12 0x0040D7E0, ProcessSHOOT +0x98 v12
// 0x0040D9C0), the bolt "pwp_crb_pjl" and its explosion "pwp_crb_pjl_expl" (Create v12 0x0040D280 / 0x0040D2F0) and
// the bolt's message packer (vtable v12 0x005FA2DC: Pack v12 0x0040F2D0, UnPack v12 0x0040F3E0).

#include "game.h"
#include "../wr_vcall.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30
entENTITY *__fastcall pwpCRB_PJL_Create(animINST *i);     // v12 0x0040D280 "pwp_crb_pjl" -- not rebuilt
entENTITY *__fastcall pwpCRB_PJL_EXPL_Create(animINST *i); // v12 0x0040D2F0 "pwp_crb_pjl_expl" -- not rebuilt

// The bolt's msgDATA packer (its slots not rebuilt).
class pwpCRB_PJL_PACK : public msgDATA_PACK {
public:
    BOOL32   Pack(fioFILE_MEM *f, msgDATA *d);          // v12 0x0040F2D0 -- not rebuilt
    msgDATA *UnPack(fioFILE_MEM *f);                    // v12 0x0040F3E0 -- not rebuilt
};

sncSOUND_DESCR_3D pwpCrbSnd[6];                         // v12 0x00640BC8 appear, shot, reload, recharge, water, ground
static pwpCRB_PJL_PACK pwpCrbPjlPack;                   // v12 0x00640B7C (id -1)
txmTEXTURE *pwpCROSSBOW::pTexFlame;                     // v12 0x00640B78 "pwp_crb_pjl_flame"
txmTEXTURE *pwpCROSSBOW::pTexSparks;                    // v12 0x00640B88 "pwp_crb_pjl_sparks"
txmTEXTURE *pwpCROSSBOW::pTexScorchMark;                // v12 0x00640BC0 "sfx_bsk_scorch_mark"

// v12 0x0040CFF0  static initialiser (the sncSOUND_DESCR_3D constructor inline, then the packer)
static struct pwpCRB_SND_INIT {
    pwpCRB_SND_INIT()
    {
        static const struct { const char *name; float minDist, maxDist; } s[6] = {
            { "wpn_crossbow_appear", 0.0f, 0.0f },       { "wpn_crossbow_shoot", 0.0f, 0.0f },
            { "wpn_crossbow_reload", 0.0f, 0.0f },       { "wpn_crossbow_recharge", 0.0f, 0.0f },
            { "wpn_basooka_hit_water", 40.0f, 140.0f },  { "wpn_crossbow_hit_ground", 40.0f, 140.0f },
        };
        for (int i = 0; i < 6; i++) {
            sncSOUND_DESCR_3D *d = &pwpCrbSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = 0;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].minDist;
            d->maxDist = s[i].maxDist;
            d->vtbl = 0;                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} pwpCrbSndInit;

// v12 0x0040D190  (feb 0x0040C2D0)
int __fastcall pwpCRBInit()
{
    if (!entRegisterClass((char *)"pwp_crb", 'PCRB', 0x50, pwpCROSSBOW::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"pwp_crb_pjl", 'PCRB', 0x2D, pwpCRB_PJL_Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"pwp_crb_pjl_expl", 'PCRB', 0x2D, pwpCRB_PJL_EXPL_Create, 0, 1))
        return 0;
    if (!(pwpCROSSBOW::pTexFlame = txmManager->Add((char *)"pwp_crb_pjl_flame", 0x40003, 1)))
        return 0;
    if (!(pwpCROSSBOW::pTexSparks = txmManager->Add((char *)"pwp_crb_pjl_sparks", 0x40000, 1)))
        return 0;
    if (!(pwpCROSSBOW::pTexScorchMark = txmManager->Add((char *)"sfx_bsk_scorch_mark", 0x40000, 1)))
        return 0;
    if (!sncLinkSndListData(6, pwpCrbSnd))
        return 0;
    msgSystem.RegisterMsgData(&pwpCrbPjlPack);
    return 1;
}

// v12 0x0040D370  (feb ?)
// As pwpWEAPON::SetupAniSystem with an 11th sequence CRB_SEQ_PREPARE_TO_SHOOT and the crossbow's transitions.
BOOL32 pwpCROSSBOW::SetupAniSystem()
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
    t->nSeq = 11;
    int *blk = (int *)operator new(0x5DC);
    animSEQ *seq = 0;
    if (blk) {
        blk[0] = 11;
        seq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 11; i++) {
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
    _entAnimSetSeqName(t, 10, (char *)"CRB_SEQ_PREPARE_TO_SHOOT");
    entAnimAssignTplSeq(t, old, nOld);
    t->seq[0].frameStart = 0.0f;
    t->seq[0].frameEnd = 1.0f;
    t->seq[0].lengthMs = 500.0f;
    t->seq[0].dir = 0;
    AdjustTiming();
    animSYSTEM *as = new animSYSTEM(11, 0);
    if (!as)
        return 0;
    *(animSYSTEM **)((char *)t + 0xA4) = as;
    wrVCall<void>(as, 0xC, 0, 0, 1, -1);
    wrVCall<void>(as, 0xC, 1, 0, 0, -1);
    wrVCall<void>(as, 0x14, 1);
    wrVCall<void>(as, 0x14, 3);
    wrVCall<void>(as, 4, 3, 1, 0, 1, 1);
    wrVCall<void>(as, 4, 3, 4, 0, 4, 1);
    wrVCall<void>(as, 4, 3, 6, 0, 6, 1);
    wrVCall<void>(as, 4, 3, 7, 0, 7, 0);
    wrVCall<void>(as, 0xC, 4, 1, 1, -1);
    wrVCall<void>(as, 4, 4, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 6, 3, 1, 3, 1);
    wrVCall<void>(as, 4, 6, 1, 0, 1, 1);
    wrVCall<void>(as, 4, 6, 6, 2, 6, 1);
    wrVCall<void>(as, 4, 6, 7, 1, 7, 1);
    wrVCall<void>(as, 0xC, 7, 1, 0, 10);
    wrVCall<void>(as, 4, 7, 8, 0, 8, 0);
    wrVCall<void>(as, 4, 7, 5, 0, 5, 0);
    wrVCall<void>(as, 0xC, 8, 1, 0, -1);
    wrVCall<void>(as, 4, 4, 1, 0, 1, 0);
    wrVCall<void>(as, 4, 4, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 1, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 3, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 7, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 8, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 10, 6, 0, 6, 0);
    wrVCall<void>(as, 0x20, 1, 2);
    wrVCall<void>(as, 4, 2, 1, 1, 1, 1);
    if (t->seq[5].dir == 2)                 // TAKE_OFF_EMPTY missing: a copy of TAKE_OFF
        memcpy(&t->seq[5], &t->seq[4], sizeof(animSEQ));
    wrVCall<void>(as, 0x20, 4, 5);
    wrVCall<void>(as, 0xC, 10, 1, 0, -1);
    wrVCall<void>(as, 4, 10, 8, 0, 8, 0);
    wrVCall<void>(as, 4, 10, 5, 0, 5, 0);
    if (t->seq[9].dir == 2)                 // FORCE_RECHARGE missing: a copy of RECHARGE
        memcpy(&t->seq[9], &t->seq[8], sizeof(animSEQ));
    wrVCall<void>(as, 0x20, 8, 9);
    wrVCall<void>(as, 4, 3, 9, 0, 9, 0);
    wrVCall<void>(as, 4, 10, 9, 1, 9, 0);
    if (old)
        operator delete((int *)old - 1);
    return 1;
}

// v12 0x0040DC90  (feb ?)
// The bolts on the bow ("arrow01".."arrow06", hidden at first), the model's normals, the sound channel flag 0x400;
// 6 a clip, none loaded (action frames: shot 0, recharge 1), 25 a second, damage 60, 50 at most, none at first.
int pwpCROSSBOW::ProcessINIT()
{
    if (!pwpWEAPON::ProcessINIT())
        return 0;
    for (int i = 0; i < 6;) {
        char name[0x40];
        sprintf(name, "arrow%02d", i + 1);
        arrow[i] = objFindName(inst->root, name);
        if (!arrow[i])
            break;
        arrow[i]->SetStateProcYes(8, 0);
        i++;
    }
    inst->root->AllocVertNormalList();
    *(u32 *)((u8 *)sndChannel + 4) |= 0x400;
    flags2 |= 2;
    shootActFrame = 0;
    reloadActFrame = 1;
    clipSize = 6;
    clip = 0;
    SetShootRate(25.0f);
    damage = 60.0f;
    maxAmmo = 50.0f;
    SetAmmo(0.0f);
    return 1;
}

// v12 0x0040DD90  (feb ?)
// Events: 2 the shot, 8 the recharge (RECHARGE / FORCE_RECHARGE action frame 2), 4 the reload (RECHARGE action
// frame 3, PREPARE action frame 1), 1 the bow appearing (PUT_ON action frame 2).
void pwpCROSSBOW::Anim2Sound()
{
    if (inst->IsActionFrame(7, shootActFrame)) {
        unk108 |= 2;
        return;
    }
    if (inst->IsActionFrame(8, 2) || inst->IsActionFrame(9, 2)) {
        unk108 |= 8;
        return;
    }
    if (inst->IsActionFrame(8, 3)) {
        unk108 |= 4;
        return;
    }
    if (inst->IsActionFrame(6, 2)) {
        unk108 |= 1;
        return;
    }
    if (inst->IsActionFrame(10, 1))
        unk108 |= 4;
}

// v12 0x0040DE60  (feb ?)
void pwpCROSSBOW::UpdateSound()
{
    u32 e = unk108;
    if (e & 2)
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpCrbSnd[1], 0);
    else if (e & 8)
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpCrbSnd[3], 0);
    else if (e & 4)
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpCrbSnd[2], 0);
    else if (e & 1)
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpCrbSnd[0], 0);
    pwpWEAPON::UpdateSound();
}

// v12 0x0040DEF0  (feb ?)
// While out (or flags2 8): a bolt leaves the bow on PREPARE's action frame 0 (unless the ammo is unlimited); the
// loaded ones (6 - clip, from the bow's end) shown on RECHARGE / PUT_ON / FORCE_RECHARGE action frame 1 (the last one
// hidden on FORCE_RECHARGE); the empty clip thrown on RECHARGE action frame 4 / FORCE_RECHARGE action frame 3.
void pwpCROSSBOW::ProcessFRAME()
{
    int *unlimited = &wrSysGO->unlimitedAmmo;
    pwpWEAPON::ProcessFRAME();
    if ((inst->flags & 1) && !(flags2 & 8))
        return;
    if (inst->IsActionFrame(10, 0) && loaded < 6 && !*unlimited) {
        arrow[loaded]->SetStateProcYes(8, 0);
        loaded++;
    } else if (inst->IsActionFrame(8, 1) || inst->IsActionFrame(6, 1)) {
        int n = 6 - clip;
        loaded = n > 0 ? n : 0;
        for (int i = loaded; i < 6; i++)
            arrow[i]->SetStateProcNo(8, 0);
    } else if (inst->IsActionFrame(9, 1)) {
        int n = 6 - clip;
        loaded = n > 0 ? n : 0;
        for (int i = loaded; i < 6; i++)
            arrow[i]->SetStateProcNo(8, 0);
        arrow[5]->SetStateProcYes(8, 0);
    }
    if (inst->IsActionFrame(8, 4))
        ThrowClip(1);
    else if (inst->IsActionFrame(9, 3))
        ThrowClip(0);
}

// v12 0x0040E070 (folded with the colt's)
// The prepare-to-shoot sequence counts as SHOOT.
int pwpCROSSBOW::GetCurAnimSeq()
{
    int s = inst->seq;
    if (s == 10)
        s = 7;
    return s;
}

// v12 0x0040E3B0  (feb ?)
// The empty clip ("clip" object of the model) flies out from beside the owner (on the right) as a rigid
// "pwp_crb_clip", tinted like the weapon's light, deleted 5 s later. (The argument picks nothing: both branches pass
// the same.)
void pwpCROSSBOW::ThrowClip(int)
{
    u8 init[0xB8];                          // the rigid body's init data (physics), as the game fills it
    memset(init, 0, sizeof(init));
    init[1] = 0x40;
    *(u16 *)(init + 2) = 0xB8;
    *(int *)(init + 4) = 8;
    *(m3dV *)(init + 8) = m3dVZero;
    *(m3dV *)(init + 0x14) = m3dVZero;
    *(float *)(init + 0x98) = 0.0f;
    *(float *)(init + 0x9C) = 0.6f;
    *(float *)(init + 0xA0) = 0.8f;
    *(float *)(init + 0xA4) = 10.0f;
    *(float *)(init + 0xA8) = 0.3f;
    *(float *)(init + 0xAC) = -1.0f;
    *(float *)(init + 0xB0) = -1.0f;
    *(float *)(init + 0xB4) = -1.0f;
    animCREATE_DATA cd;
    if (!objFindName(inst->root, (char *)"clip"))
        return;
    owner->inst->GetMatrL2W(&cd.matr);
    cd.matr.Translate(1.0f, 1.3f, 0.1f, 1);
    cd.scale.x = cd.scale.y = cd.scale.z = 0.2f;
    cd.matr.GetAxisZ((m3dV *)(init + 8));
    *(float *)(init + 0xC) = -0.3f;
    *(float *)(init + 0xA0) = 0.5f;
    *(float *)(init + 0x9C) = 0.4f;
    entENTITY *c = entCreate(gsScenePtr, (char *)"rigid", (char *)"pwp_crb_clip", &cd, init, 0);
    if (!c)
        return;
    c->inst->root->SetConstColor(colDiffuse, 2, 8);
    c->inst->flags |= 0x10;
    msgSystem.PostTimeMsg(c, MSG_MSG_TERM, 0, 5.0f);
}

// v12 0x0040FBD0  (feb 0x0040E570)
entENTITY *__fastcall pwpCROSSBOW::Create(animINST *)
{
    pwpCROSSBOW *w = new pwpCROSSBOW;       // operator new(0x1E7), pwpWEAPON ctor
    if (!w)
        return 0;
    w->sndChannel = 0;
    w->owner = 0;
    w->unk1C3 = 1.0f;
    w->loaded = 0;
    w->wpnIndex = PWP_CROSSBOW;
    return w;
}
