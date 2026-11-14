// plw_clt.cpp -- pwpCOLT, Will's colt (weapon 1): a 14-round revolver, the empty clip thrown out on reload
// (Will Rock 1.2, v12.exe; feb obj plw_clt.obj)
//
// Rebuilt from the 1.2 code (v12 0x0040C710..0x0040CFE3, vtable 0x005FA1D0). 1.2 changes, from the disassembly:
//   - the shot comes on the SHOOT sequence's action frame 1, which ProcessINIT puts at the sequence's first frame;
//   - an 11th sequence CLT_SEQ_PREPARE_TO_SHOOT (reported as SHOOT by GetCurAnimSeq) and its transitions;
//   - ProcessINIT creates the flare and the shells through the 1.2 slots (+0xE0 / +0xE4);
//   - ThrowClip: the clip is a "rigid" "pwp_clt_clip" tinted like the weapon, deleted after 5 s.

#include "game.h"

sncSOUND_DESCR_3D pwpCltSnd[6];                         // v12 0x00640908: 0..2 shots, 3 empty, 4 reload, 5 shell
// v12 0x0040C5B0  static initialiser (it was never defined here: the build made zeroed data of it, nameless sounds --
// the colt's shots and reload were silent)
static struct pwpCLT_SND_INIT {
    pwpCLT_SND_INIT()
    {
        static const struct { const char *name; u32 flags; float volume; int delay; float minDist, maxDist; } s[6] = {
            { "wpn_colt_shoot", 0, 0.0f, 0, 0.0f, 0.0f },
            { "wpn_colt_shoot_1", 0, 0.0f, 0, 0.0f, 0.0f },
            { "wpn_colt_shoot_2", 0, 0.0f, 0, 0.0f, 0.0f },
            { "wpn_colt_shoot_recharge", 0, 0.0f, 0, 0.0f, 0.0f },
            { "wpn_colt_recharge", 0, 0.0f, 0, 0.0f, 0.0f },
            { "wpn_colt_drop_shell", 0, -10.0f, 0, 0.0f, 0.0f },
        };
        for (int i = 0; i < 6; i++) {
            sncSOUND_DESCR_3D *d = &pwpCltSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = s[i].volume;
            d->delay = s[i].delay;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].minDist;
            d->maxDist = s[i].maxDist;
            d->vtbl = 0;                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} pwpCLTSndInit;
int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30
static const float pwpCltEps = 1e-6f;                   // v12 0x005FA1C8
float pwpCltEpsSq;                                      // v12 0x00640900 (static init v12 0x0040CDD0)
static struct pwpCLT_EPS_INIT { pwpCLT_EPS_INIT() { pwpCltEpsSq = pwpCltEps * pwpCltEps; } } pwpCltEpsInit;

// v12 0x0040C710  (feb 0x0040B850)
int __fastcall pwpCLTInit()
{
    return entRegisterClass((char *)"pwp_clt", 'PCLT', 0x50, pwpCOLT::Create, 0, 1) != 0;
}

// v12 0x0040C740
sncSOUND_DESCR_3D *pwpCOLT::GetShellSoundDesc()
{
    return &pwpCltSnd[5];
}

// v12 0x0040C750  (feb 0x0040B890)
// One of the three shots at random, or the click of an empty clip.
void pwpCOLT::PlayShootSound()
{
    if (!clip) {
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpCltSnd[3], 0);
        return;
    }
    int i = m3dRandIntRange(0, 2);
    ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpCltSnd[i], 0);
}

// v12 0x0040C7A0  (feb 0x0040B8E0)
void pwpCOLT::Anim2Sound()
{
    if (inst->IsActionFrame(9, 0))
        unk108 |= 1;
}

// v12 0x0040C7D0  (feb 0x0040B910)
// The reload sound on event 1 (Anim2Sound).
void pwpCOLT::UpdateSound()
{
    if (unk108 & 1)
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpCltSnd[4], 0);
    pwpWEAPON::UpdateSound();
}

// v12 0x0040C800  (feb 0x0040B940)
// 14 rounds; the shot on the SHOOT sequence's first frame; damage 10 (the animation sets the rate); the flare;
// shells thrown at 0.0096..0.03 over 1/6 s.
int pwpCOLT::ProcessINIT()
{
    if (!pwpBULLET_WPN::ProcessINIT())
        return 0;
    clipSize = 14;
    clip = 14;
    reloadActFrame = 0;
    shootActFrame = 1;
    animSEQ *sh = &inst->tpl->seq[7];
    sh->actionFrame[1] = (int)sh->frameStart;
    wpnWEAPON::flags |= 4;
    pwpWEAPON::flags2 |= 2;
    SetShootRate(0.0f);
    damage = 10.0f;
    CreateFlare(1.0f);
    m3dSPL_LINEAR1D *spl = new m3dSPL_LINEAR1D;
    if (spl) {
        spl->flags = 0;
        spl->type = 0;
        spl->nKp = 0;
        spl->kpValueSize = 8;
        spl->kpStride = 8;
        spl->dim = 1;
        spl->kpBytes = 0;
        spl->kp = 0;
        spl->pad[0] = spl->pad[1] = spl->pad[2] = 0;
        spl->AllocKpList(2);
    }
    spl->SetKp(0, 0.0f, 0.0096f);
    spl->SetKp(1, 0.16666667f, 0.03f);
    CreateShell(1.0f, 0.005f, spl);
    return sncLinkSndListData(6, pwpCltSnd) != 0;
}

// v12 0x0040C940  (feb 0x0040BA80)
// A shell on the SHOOT sequence's action frame 1; the clip thrown on the reload's action frame 1.
void pwpCOLT::ProcessFRAME()
{
    pwpBULLET_WPN::ProcessFRAME();
    animINST *a = inst;
    if (a->flags & 1)
        return;
    if (a->seq == 7 && (a->actionFrames & 2))
        StartShells(1);
    if (inst->IsActionFrame(8, 1) || inst->IsActionFrame(9, 1))
        ThrowClip();
}

// v12 0x0040C9A0  (feb 0x0040BAE0)
// As pwpWEAPON::SetupAniSystem with an 11th sequence CLT_SEQ_PREPARE_TO_SHOOT and the colt's transitions.
BOOL32 pwpCOLT::SetupAniSystem()
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
    _entAnimSetSeqName(t, 10, (char *)"CLT_SEQ_PREPARE_TO_SHOOT");
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
    wrVCall<void>(as, 0x20, 1, 2);
    wrVCall<void>(as, 4, 2, 1, 1, 1, 1);
    wrVCall<void>(as, 4, 4, 1, 0, 1, 0);
    wrVCall<void>(as, 4, 4, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 1, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 3, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 7, 6, 0, 6, 0);
    wrVCall<void>(as, 4, 8, 6, 0, 6, 0);
    if (t->seq[5].dir == 2)                 // TAKE_OFF_EMPTY missing: a copy of TAKE_OFF
        memcpy(&t->seq[5], &t->seq[4], sizeof(animSEQ));
    wrVCall<void>(as, 0x20, 4, 5);
    wrVCall<void>(as, 0xC, 10, 1, 0, -1);
    wrVCall<void>(as, 4, 10, 8, 0, 8, 0);
    wrVCall<void>(as, 4, 10, 7, 0, 7, 0);
    if (t->seq[9].dir == 2)                 // FORCE_RECHARGE missing: a copy of RECHARGE
        memcpy(&t->seq[9], &t->seq[8], sizeof(animSEQ));
    wrVCall<void>(as, 0x20, 8, 9);
    wrVCall<void>(as, 4, 3, 9, 0, 9, 0);
    if (old)
        operator delete((int *)old - 1);
    return 1;
}

// v12 0x0040CDF0
// The empty clip ("clip" object of the model) flies out from beside the owner as a rigid body, tinted like the
// weapon's light, and is deleted 5 s later.
void pwpCOLT::ThrowClip()
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
    cd.matr.Translate(-0.5f, 1.3f, 0.1f, 1);
    cd.scale.x = cd.scale.y = cd.scale.z = 0.2f;
    cd.matr.GetAxisZ((m3dV *)(init + 8));
    *(float *)(init + 0xC) = -0.3f;                     // (written after the four entCreate pushes:
    *(float *)(init + 0xA0) = 0.5f;                     //  v12 [esp+0x24] / +0xB8 / +0xB4 = init +0xC / +0xA0 / +0x9C)
    *(float *)(init + 0x9C) = 0.4f;
    entENTITY *c = entCreate(gsScenePtr, (char *)"rigid", (char *)"pwp_clt_clip", &cd, init, 0);
    if (!c)
        return;
    c->inst->root->SetConstColor(colDiffuse, 2, 8);
    c->inst->flags |= 0x10;
    msgSystem.PostTimeMsg(c, MSG_MSG_TERM, 0, 5.0f);
}

// v12 0x0040CFB0
entENTITY *__fastcall pwpCOLT::Create(animINST *)
{
    pwpCOLT *w = new pwpCOLT;               // operator new(0x1DF), pwpBULLET_WPN ctor
    if (!w)
        return 0;
    w->wpnIndex = PWP_COLT;
    return w;
}

// v12 0x0040E070 (the crossbow's, folded)
// The prepare-to-shoot sequence counts as SHOOT.
int pwpCOLT::GetCurAnimSeq()
{
    int s = inst->seq;
    if (s == 10)
        s = 7;
    return s;
}
