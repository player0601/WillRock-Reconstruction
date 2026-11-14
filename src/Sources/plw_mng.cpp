// plw_mng.cpp -- pwpMINIGUN, the minigun (weapon 8) (Will Rock 1.2, v12.exe; feb obj plw_mng.obj)
//
// A clip of 100, 10 shots a second of two rays each. Pressed, it spins up (PWP_MNG_SEQ_START_SHOOT), shoots on
// SHOOT's action frame, spins down when let go (PWP_MNG_SEQ_STOP_SHOOT) -- in 1.2 pressing again while it spins
// down goes through START_SHOOT again (feb let it shoot at once) -- and on the last round of a clip it plays
// SHELLS_FINISHED before the recharge. Its own flare ("sfx_mng_flare", turned to the viewer and spun each frame),
// smoke from the barrels and shells (a 0.05 scale).

#include "game.h"
#include "../gs/gs_mp_client.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30

// v12 0x00640F20.. the sounds, 0x68 apart (static initialiser v12 0x004121D0)
static sncSOUND_DESCR_3D pwpMngSnd[7];
enum { MNG_SND_START, MNG_SND_SHOOT, MNG_SND_STOP_1, MNG_SND_STOP_2, MNG_SND_RECHARGE, MNG_SND_FORCE_RECHARGE,
       MNG_SND_SHELL };

static struct pwpMNG_SND_INIT {
    pwpMNG_SND_INIT()
    {
        static const struct { const char *name; u32 flags; } s[7] = {
            { "wpn_minigun_start_shoot", 0 },
            { "wpn_minigun_shoot", 0x40002 },           // (looped)
            { "wpn_minigun_stop_shoot_1", 0 },
            { "wpn_minigun_stop_shoot_2", 0 },
            { "wpn_minigun_recharge", 0 },
            { "wpn_minigun_force_recharge", 0 },
            { "wpn_minigun_shell", 0 },
        };
        for (int i = 0; i < 7; i++) {
            sncSOUND_DESCR_3D *d = &pwpMngSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = 0.0f;
            d->maxDist = 0.0f;
            d->vtbl = 0;                                // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} pwpMngSndInit;

struct pwpMNG_SHELL_TEX { static txmTEXTURE *pTex; };
struct pwpMNG_SMOKE_TEX { static txmTEXTURE *pTex; };
txmTEXTURE *pwpMNG_SHELL_TEX::pTex;                     // v12 0x006411F8 "sfx_mng_shells"
txmTEXTURE *pwpMNG_SMOKE_TEX::pTex;                     // v12 0x00641200 "sfx_discus_land_cloud"

static m3dSPL_LINEAR1D *mngSpl(int n)
{
    m3dSPL_LINEAR1D *s = new m3dSPL_LINEAR1D;
    if (s) {
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
    }
    return s;
}

// pwpMNG_SHELL (0x2D0, vtable v12 0x005FAA08): the shells; they ring on the floor as the Tommy gun's
// (PartApplyCDT is pwpMCG_SHELL's, v12 0x0040C010) and keep the gun's sideways speed only while it moves.
class pwpMNG_SHELL : public partEMITTER_PHYS {
public:
    sncCHANNEL       **snd;                             // +0x2C4
    sncSOUND_DESCR_3D *sndDesc;                         // +0x2C8
    pwpMINIGUN        *gun;                             // +0x2CC
    pwpMNG_SHELL(sncCHANNEL **ch, sncSOUND_DESCR_3D *d, float k);
    void PartUpdate(int n, partPART **list, float dt);  // slot +0x20 v12 0x00412680
    int  PartApplyCDT(partPART *p, m3dV *v, float t);   // slot +0x2C v12 0x0040C010 (shared)
};

// pwpMNG_SMOKE (0x2C4, vtable v12 0x005FAA3C, no overrides): the barrels' smoke after each shot.
class pwpMNG_SMOKE : public partEMITTER_PHYS {
public:
    pwpMNG_SMOKE(float k);                              // v12 0x00412910
};

// pwpMNG_MOD_FLARE (an objMOD_BILLBOARD, vtable v12 0x005FAB74): the flare faces the viewer about its own axis
// and turns at random every frame.
class pwpMNG_MOD_FLARE : public objMOD_BILLBOARD {
public:
    BOOL32 Modify();                                    // v12 0x00412B60
};

// v12 0x00412330
int __fastcall pwpMNGInit()
{
    if (!entRegisterClass((char *)"pwp_mng", 0x504D4E47, 0x50, pwpMINIGUN::Create, 0, 1))
        return 0;
    if (!(pwpMNG_SHELL_TEX::pTex = txmManager->Add((char *)"sfx_mng_shells", 0x40003, 1)))
        return 0;
    if (!(pwpMNG_SMOKE_TEX::pTex = txmManager->Add((char *)"sfx_discus_land_cloud", 0x40003, 1)))
        return 0;
    return sncLinkSndListData(7, pwpMngSnd) != 0;
}

// v12 0x004123B0
// 25 a second (a burst of 1/30 s), 3 (+-1) s each; k: the size (0.175, the curve 0.139k at 0.2 -> 1.155k at 0.5),
// the speeds (12k out, 3k random) and gravity (-39.2k); fading at the end, 2 x 2 frames, spinning; bouncing.
pwpMNG_SHELL::pwpMNG_SHELL(sncCHANNEL **ch, sncSOUND_DESCR_3D *d, float k) : partEMITTER_PHYS(25.0f, 4.0f)
{
    gun = 0;
    snd = ch;
    sndDesc = d;
    m3dSPL_LINEAR1D *in = mngSpl(2);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 25.0f);
    SetIntensity(0.0f, in);
    m3dSPL_LINEAR1D *sz = mngSpl(2);
    sz->SetKp(0, 0.2f, k * 0.1386f);
    sz->SetKp(1, 0.5f, k * 1.155f);
    SetSize(k * 0.175f, 0.0f, sz, 1.0f);
    SetTimeLifePart(3.0f, 1.0f);
    m3dSPL_LINEAR1D *a = mngSpl(3);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 0.9f, 255.0f);
    a->SetKp(2, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureFrameAnim(2, 2, 0.0f, 4.0f);
    SetTextureRotAnim(180.0f, 180.0f, 300.0f, 0.0f);
    flags |= 4;
    flags |= 2;
    gravity.x = 0;
    gravity.y = k * -39.2f;
    gravity.z = 0;
    speedASpl = 0;
    speedK = 0.8f;
    speedKRand = 0;
    cone = 18.0f;
    coneSpl = 0;
    speedA = k * 12.0f;
    speedARand = k * 3.0f;
    SetTexture(pwpMNG_SHELL_TEX::pTex);
    flags |= 0x400;
    flags |= 8;
    flags |= 0x60;
    flags |= 0x4000000;
    SetCDTParam(0.015f, 0.4f, 0.3f, 4);
    flags |= 0x10000;
    flags |= 0x10000000;
    flags |= 0x40000000;
}

// v12 0x00412680
// While the gun (its owner's speed) is all but still, the new shells lose the part of their speed that goes
// forward along "shell_source", unless they fly out steeply.
void pwpMNG_SHELL::PartUpdate(int n, partPART **list, float dt)
{
    partEMITTER_PHYS::PartUpdate(n, list, dt);
    m3dV v = *(m3dV *)((u8 *)gun + 0xF0);               // the gun's speed (entENTITY +0xF0)
    v.y = 0;
    float sp = fabsf(sqrtf(v.z * v.z + v.x * v.x));
    if (sp >= 0.01f)
        return;
    m3dV ax;
    gun->shellSource->GetAxisZ(&ax);
    ax.y = 0;
    m3dNormalize(&ax);
    for (int i = 0; i < n; i++) {
        m3dV *pv = (m3dV *)((u8 *)list[i] + 0x1C);      // the particle's speed
        float along = pv->z * ax.z + ax.x * pv->x + ax.y * 0.0f;
        float flat = sqrtf(pv->z * pv->z + pv->x * pv->x);
        if (flat * 0.8f <= along)
            continue;
        if (flat <= 0.5f)
            continue;
        pv->x *= 0.0f;
        pv->z *= 0.0f;
    }
}

// v12 0x0040C010 (pwpMCG_SHELL::PartApplyCDT, shared)
// A shell's first bounce (byte +0x39 == 1) rings at its place, unless the channel still plays.
int pwpMNG_SHELL::PartApplyCDT(partPART *p, m3dV *v, float t)
{
    if (!partEMITTER_PHYS::PartApplyCDT(p, v, t))
        return 0;
    if (((u8 *)p)[0x39] == 1 && !(*snd)->IsPlay(-1)) {
        wrVCall<void>(*snd, 0x14, (m3dV *)((u8 *)p + 0x10), 0, 0);   // the channel's position
        wrVCall<void>(*snd, 4, (sncSOUND_DESCR *)sndDesc, 0);        // Play
    }
    return 1;
}

// v12 0x00412910
// 100 a second for 1/30 s (dying out by 1/2), 0.3 s each; k the size (0.5k -> k), the speed (5k, 1.5k tangent)
// and gravity (-9.8k); faint (48), turning.
pwpMNG_SMOKE::pwpMNG_SMOKE(float k) : partEMITTER_PHYS(30, 100.0f, 0.3f)
{
    m3dSPL_LINEAR1D *in = mngSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.033333335f, 100.0f);
    in->SetKp(2, 0.5f, 0.0f);
    SetIntensity(0.0f, in);
    m3dSPL_LINEAR1D *sz = mngSpl(2);
    sz->SetKp(0, 0.0f, k * 0.5f);
    sz->SetKp(1, 1.0f, k);
    SetSize(k, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = mngSpl(2);
    a->SetKp(0, 0.0f, 48.45f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(pwpMNG_SMOKE_TEX::pTex);
    SetTextureRotAnim(180.0f, 180.0f, 0.0f, 0.0f);
    flags |= 4;
    speedARand = 0;
    speedASpl = 0;
    speedBRand = 0;
    speedA = k * 5.0f;
    speedBSpl = 0;
    speedK = 1.0f;
    speedKRand = 0;
    gravity.x = 0;
    gravity.z = 0;
    speedB = k * 1.5f;
    gravity.y = k * -9.8f;
    flags |= 0x408;
    flags |= 0xA000;
    flags |= 0x4000000;
}

// v12 0x00412B60
BOOL32 pwpMNG_MOD_FLARE::Modify()
{
    m3dV pos, z, v;
    ((m3dMATR *)obj->ltm)->GetOrigin(&pos);
    ((m3dMATR *)obj->ltm)->GetAxisZ(&z);
    m3dNormalize(&z);
    v.x = viewPos.x - pos.x;
    v.y = viewPos.y - pos.y;
    v.z = viewPos.z - pos.z;
    m3dNormalize(&v);
    float ang = m3dAngleVector(&z, &v);
    m3dV axis = { z.y * v.z - z.z * v.y, z.z * v.x - v.z * z.x, z.x * v.y - z.y * v.x };
    if (!_m3dNormalize(&axis))
        return 1;
    obj->RotateGlbAxis(ang, &axis);                     // its Z to the viewer
    m3dV lz, lo;
    ((m3dMATR *)obj->local)->GetAxisZ(&lz);
    m3dNormalize(&lz);
    ((m3dMATR *)obj->local)->GetOrigin(&lo);
    m3dMATR r;
    r.MakeRotRay(&lo, &lz, m3dRandMax(360.0f));         // and a turn about it
    ((m3dMATR *)obj->local)->Transform(&r, 2);
    if (obj->parent)
        m3dMultiplyMatr((m3dMATR *)obj->local, (m3dMATR *)obj->parent->ltm, (m3dMATR *)obj->ltm);
    else
        memcpy(obj->ltm, obj->local, sizeof(obj->ltm));
    return 1;
}

// v12 0x004142E0  (feb 0x00412980)
entENTITY *__fastcall pwpMINIGUN::Create(animINST *)
{
    pwpMINIGUN *w = new pwpMINIGUN;                     // operator new(0x1F3), pwpBULLET_WPN ctor
    if (!w)
        return 0;
    w->mngFlare = 0;
    w->mngSmoke = 0;
    w->smokeSource = 0;
    w->remoteShooting = 0;
    w->shellSource = 0;
    w->wpnIndex = PWP_MINIGUN;
    return w;
}

// v12 0x00412CF0  (feb 0x00411350)
// The flare: a "dynamic" "sfx_mng_flare" drawn with the weapon, no collision, at the end of its sequence, 0.8 its
// size; its first object with faces turned by pwpMNG_MOD_FLARE.
int pwpMINIGUN::CreateMngFlare()
{
    mngFlare = entCreate(gsScenePtr, (char *)"dynamic", (char *)"sfx_mng_flare", (animCREATE_DATA *)0, 0, 0);
    if (!mngFlare)
        return 0;
    mngFlare->inst->root->SetStateRendYes(0x1000000);
    mngFlare->inst->root->SetStateYes(0x10);
    mngFlare->inst->flags |= 0x10;
    ((dynDYNAMIC *)mngFlare)->StartAnim(0, mngFlare->inst->GetSeqLength(0), 0);
    mngFlare->flags2 |= 4;
    mngFlare->inst->Scale(0.8f, 0.8f, 0.8f, 1);
    objOBJ *o = objFind(mngFlare->inst->root, objIsWithFaces, 0);
    pwpMNG_MOD_FLARE *m = new pwpMNG_MOD_FLARE;
    m->AddObj(o);
    o->_SetStateRendYes(0x60);
    return 1;
}

// v12 0x00412E40  (feb 0x004114B0)
// The shells' sound channel (a sphere, 1..5 m), the shells (0.05) and where they come out.
int pwpMINIGUN::CreateMngShells()
{
    sncCHANNEL **ch = &shellSnd;
    for (int i = 0; i < 1; i++, ch++) {
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        if (c) {
            c->minDist = 1.0f;
            c->maxDist = 5.0f;
        }
        *ch = c;
        if (!c)
            return 0;
        c->flags |= 0x1400;
        gsSysSound->AddChannel(*ch, this, 0);
    }
    pwpMNG_SHELL *s = new pwpMNG_SHELL(&shellSnd, &pwpMngSnd[MNG_SND_SHELL], 0.05f);
    shells = s;
    if (!s)
        return 0;
    s->gun = this;
    shellSource = objFindName(inst->root, (char *)"shell_source");
    return shellSource != 0;
}

// v12 0x00412FD0  (feb 0x004117D0)
// The PWP_SEQ_* slots plus START_SHOOT (10), STOP_SHOOT (11) and SHELLS_FINISHED (12), and the transitions: a shot
// starts with START_SHOOT, SHOOT loops, any other wish from SHOOT or START_SHOOT goes through STOP_SHOOT, the last
// round of a clip through SHELLS_FINISHED to RECHARGE. Not for a remote weapon unless gsAppState 0x800.
BOOL32 pwpMINIGUN::SetupAniSystem()
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
    t->nSeq = 13;
    int *blk = (int *)operator new(0x6EC);  // MSVC array: count, then 13 x 0x88
    animSEQ *seq = 0;
    if (blk) {
        blk[0] = 13;
        seq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 13; i++) {
            memset(&seq[i], 0, sizeof(animSEQ));
            seq[i].dir = 2;
        }
    }
    t->seq = seq;
    _entAnimSetSeqName(t, 0, (char *)"PWP_SEQ_UNUSED");
    _entAnimSetSeqName(t, 1, (char *)"PWP_SEQ_IDLE");
    _entAnimSetSeqName(t, 3, (char *)"PWP_SEQ_RUN");
    _entAnimSetSeqName(t, 7, (char *)"PWP_SEQ_SHOOT");
    _entAnimSetSeqName(t, 8, (char *)"PWP_SEQ_RECHARGE");
    _entAnimSetSeqName(t, 4, (char *)"PWP_SEQ_TAKE_OFF");
    _entAnimSetSeqName(t, 5, (char *)"PWP_SEQ_TAKE_OFF_EMPTY");
    _entAnimSetSeqName(t, 6, (char *)"PWP_SEQ_PUT_ON");
    _entAnimSetSeqName(t, 0xA, (char *)"PWP_MNG_SEQ_START_SHOOT");
    _entAnimSetSeqName(t, 0xB, (char *)"PWP_MNG_SEQ_STOP_SHOOT");
    _entAnimSetSeqName(t, 9, (char *)"PWP_SEQ_FORCE_RECHARGE");
    _entAnimSetSeqName(t, 0xC, (char *)"PWP_MNG_SEQ_SHELLS_FINISHED");
    entAnimAssignTplSeq(t, old, nOld);
    t->seq[0].frameStart = 0.0f;
    t->seq[0].frameEnd = 1.0f;
    t->seq[0].lengthMs = 500.0f;
    t->seq[0].dir = 0;
    AdjustTiming();
    animSYSTEM *as = new animSYSTEM(13, 0);
    if (!as)
        return 0;
    *(animSYSTEM **)((char *)t + 0xA4) = as;
    // SetTranTypeToAny (0xC): from, type, blend, next; SetTranParam (4): from, to, type, next, blend
    wrVCall<void>(as, 0xC, 0, 0, 1, -1);
    wrVCall<void>(as, 0xC, 1, 0, 1, -1);
    wrVCall<void>(as, 4, 1, 7, 0, 0xA, 0);
    wrVCall<void>(as, 0x14, 1);
    wrVCall<void>(as, 0x14, 3);
    static const int tr[][5] = {
        { 3, 1, 0, 1, 1 }, { 3, 4, 0, 4, 1 }, { 3, 6, 0, 6, 1 }, { 3, 7, 0, 0xA, 1 },
        { 0xA, 7, 1, 7, 0 }, { 0xA, 1, 0, 0xB, 2 }, { 0xA, 3, 1, 0xB, 0 }, { 0xA, 8, 1, 0xB, 0 },
        { 0xA, 4, 1, 0xB, 0 }, { 0xA, 0xB, 0, 0xB, 2 }, { 0xA, 0xA, 1, 0xA, 0 },
        { 7, 1, 1, 0xB, 0 }, { 7, 3, 1, 0xB, 0 }, { 7, 8, 1, 0xB, 0 }, { 7, 4, 1, 0xB, 0 }, { 7, 7, 1, 7, 0 },
        { 7, 0xC, 1, 0xC, 0 }, { 7, 9, 1, 0xB, 0 } };
    for (unsigned i = 0; i < sizeof(tr) / sizeof(tr[0]); i++)
        wrVCall<void>(as, 4, tr[i][0], tr[i][1], tr[i][2], tr[i][3], tr[i][4]);
    wrVCall<void>(as, 0xC, 0xC, 1, 0, 8);
    wrVCall<void>(as, 4, 0xC, 8, 1, 8, 0);
    wrVCall<void>(as, 0xC, 4, 1, 1, -1);
    static const int tr2[][5] = {
        { 4, 6, 0, 6, 0 }, { 6, 3, 1, 3, 0 }, { 6, 1, 0, 1, 1 }, { 6, 6, 2, 6, 0 }, { 6, 7, 1, 0xA, 0 } };
    for (unsigned i = 0; i < sizeof(tr2) / sizeof(tr2[0]); i++)
        wrVCall<void>(as, 4, tr2[i][0], tr2[i][1], tr2[i][2], tr2[i][3], tr2[i][4]);
    wrVCall<void>(as, 0xC, 0xB, 1, 0, -1);
    wrVCall<void>(as, 4, 0xB, 7, 1, 0xA, 0);            // (1.2: STOP_SHOOT -> SHOOT spins up again)
    wrVCall<void>(as, 0xC, 8, 1, 1, -1);
    static const int tr3[][5] = {
        { 8, 7, 1, 0xA, 0 }, { 4, 1, 0, 1, 0 }, { 4, 6, 0, 6, 0 }, { 1, 6, 0, 6, 0 }, { 3, 6, 0, 6, 0 },
        { 7, 6, 0, 6, 0 }, { 8, 6, 0, 6, 0 }, { 0xA, 6, 0, 6, 0 }, { 0xB, 6, 0, 6, 0 }, { 0xB, 6, 0, 6, 0 },
        { 0xC, 6, 0, 6, 0 } };
    for (unsigned i = 0; i < sizeof(tr3) / sizeof(tr3[0]); i++)
        wrVCall<void>(as, 4, tr3[i][0], tr3[i][1], tr3[i][2], tr3[i][3], tr3[i][4]);
    if (t->seq[5].dir == 2)                 // TAKE_OFF_EMPTY missing: a copy of TAKE_OFF
        memcpy(&t->seq[5], &t->seq[4], sizeof(animSEQ));
    wrVCall<void>(as, 0x20, 4, 5);
    if (t->seq[9].dir == 2)                 // FORCE_RECHARGE missing: a copy of RECHARGE
        memcpy(&t->seq[9], &t->seq[8], sizeof(animSEQ));
    wrVCall<void>(as, 0x20, 8, 9);
    static const int tr4[][5] = {
        { 3, 9, 0, 9, 0 }, { 0xA, 9, 1, 0xB, 0 }, { 1, 9, 0, 9, 0 }, { 0xB, 9, 1, 9, 0 }, { 7, 9, 1, 0xB, 0 } };
    for (unsigned i = 0; i < sizeof(tr4) / sizeof(tr4[0]); i++)
        wrVCall<void>(as, 4, tr4[i][0], tr4[i][1], tr4[i][2], tr4[i][3], tr4[i][4]);
    if (old)
        operator delete((u32 *)old - 1);
    return 1;
}

// v12 0x004135C0
// The last round of a clip plays SHELLS_FINISHED instead of SHOOT.
void pwpMINIGUN::AdvanceAnimation(int seq, float dt)
{
    if (seq == 7 && clip == 1)
        seq = 0xC;
    ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, seq, dt * unk1C3, 0);
}

// v12 0x00404DA0 (shared with the atomic gun): the spin sequences count as SHOOT
int pwpMINIGUN::GetCurAnimSeq()
{
    int s = inst->seq;
    if (s == 0xA || s == 0xB || s == 0xC)
        return 7;
    return s;
}

// v12 0x00413600
BOOL32 pwpMINIGUN::IsShooting()
{
    if (inst->IsActionFrame(0xC, 0))
        return 1;
    return pwpWEAPON::IsShooting();
}

// v12 0x00413630
BOOL32 pwpMINIGUN::IsReadyRecharge()
{
    if (inst->IsActionFrame(0xC, 1))
        return 1;
    return pwpWEAPON::IsReadyRecharge();
}

// v12 0x00548680 (the shared "return 1"): it may always shoot (the spin-up is its delay)
int pwpMINIGUN::IsReadyShoot()
{
    return 1;
}

// v12 0x004142C0
int pwpMINIGUN::SubtractAmmo(float)
{
    pwpWEAPON::SubtractAmmo(1.0f);
    return 1;
}

// v12 0x00430B30 (shared)
float pwpMINIGUN::GetScorchmarkScale()
{
    return 1.5f;
}

// v12 0x00413660
// The loop (for a remote copy only while its owner shoots), unless a stop or recharge sound plays; at the last
// round the stop sound instead (by UpdateSound).
void pwpMINIGUN::PlayShootSound()
{
    if ((entFlags & 1) && !remoteShooting)
        return;
    sncCHANNEL *c = (sncCHANNEL *)sndChannel;
    if (c->IsPlay((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_STOP_1]) || c->IsPlay((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_STOP_2]) ||
        c->IsPlay((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_RECHARGE]) ||
        c->IsPlay((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_FORCE_RECHARGE]))
        return;
    if (inst->seq == 0xC) {
        unk108 |= 0x20;
        return;
    }
    c->Play((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_SHOOT], 0);
}

#pragma pack(push, 1)
struct pwpMNG_EV : msgDATA {                            // GS_MSG_MP_NOTIFY to the owner, 7 bytes
    short type;                                         // +0x04 0x11
    u8    bits;                                         // +0x06 0x10, | 1 when the spin-up starts
};
#pragma pack(pop)

// v12 0x00413C20
// The sound events of the sequences' first frames (start, stop, recharge, forced recharge); each told to the other
// machines through the owner.
void pwpMINIGUN::Anim2Sound()
{
    u8 bits;
    if (inst->IsActionFrame(0xA, 0)) {
        unk108 |= 1;
        bits = 0x11;
    } else {
        if (inst->IsActionFrame(0xB, 0))
            unk108 |= 4;
        else if (inst->IsActionFrame(8, 0))
            unk108 |= 8;
        else if (inst->IsActionFrame(9, 0))
            unk108 |= 0x10;
        else
            return;
        bits = 0x10;
    }
    pwpMNG_EV ev;
    ev.packId = 0;
    ev.tag = 0x40;
    ev.size = 7;
    ev.type = 0x11;
    ev.bits = bits;
    wrEVCall<int>(gsSysClient, 0x2C, (void *)owner, 0x40E, (void *)&ev, 0x20000);
}

// v12 0x00413D00
// The start (and the loop with it), the stop (the second one when the start still sounds), the recharges, the
// last round's stop.
void pwpMINIGUN::UpdateSound()
{
    sncCHANNEL *c = (sncCHANNEL *)sndChannel;
    u32 e = unk108;
    if (e & 1)
        c->Play((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_START], 0);
    else if (e & 4)
        c->Play((sncSOUND_DESCR *)&pwpMngSnd[c->IsPlay((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_START]) ? MNG_SND_STOP_2 : MNG_SND_STOP_1], 0);
    else if (e & 8)
        c->Play((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_RECHARGE], 0);
    else if (e & 0x10)
        c->Play((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_FORCE_RECHARGE], 0);
    else if (e & 0x20)
        c->Play((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_STOP_1], 0);
    pwpWEAPON::UpdateSound();
}

// v12 0x00413DC0  (feb 0x00412510)
// 100 a clip, the shot on SHOOT's frame 0 and the recharge on 1, 10 a second, damage 8, 1000 at most, 500 at
// first; the smoke (1.0), the flare, the shells, the barrels' smoke (0.05) from "smoke_source".
int pwpMINIGUN::ProcessINIT()
{
    if (!pwpBULLET_WPN::ProcessINIT())
        return 0;
    pwpWEAPON::flags2 |= 2;
    clipSize = 100;
    clip = 100;
    shootActFrame = 0;
    reloadActFrame = 1;
    SetShootRate(10.0f);
    damage = 8.0f;
    maxAmmo = 1000.0f;
    SetAmmo(500.0f);
    flareScale = 0.25f;
    CreateSmoke(1.0f);
    if (!CreateMngFlare())
        return 0;
    if (!CreateMngShells())
        return 0;
    mngSmoke = new pwpMNG_SMOKE(0.05f);
    if (!mngSmoke)
        return 0;
    smokeSource = objFindName(inst->root, (char *)"smoke_source");
    if (!smokeSource)
        return 0;
    smokeSource->SetStateProcYes(0x1008, 0);
    return 1;
}

// v12 0x00413ED0  (feb 0x00412620)
void pwpMINIGUN::ProcessTERM()
{
    pwpBULLET_WPN::ProcessTERM();
    if (mngSmoke)
        wrVCall<void *>(mngSmoke, 0, 1u);
    if (mngFlare) {
        mngFlare->flags2 &= ~4u;
        msgSystem.PostMsg(mngFlare, MSG_MSG_TERM, 0);
    }
}

// v12 0x00413F20
void pwpMINIGUN::ProcessTERM_LEVEL()
{
    pwpWEAPON::ProcessTERM_LEVEL();
    remoteShooting = 0;
}

// v12 0x00413F40  (feb 0x00412670)
// On each shot frame 1..4 shells and a puff from the barrels (with the gun's speed); the flare at the muzzle.
void pwpMINIGUN::ProcessFRAME()
{
    pwpBULLET_WPN::ProcessFRAME();
    if (inst->flags & 1)
        return;
    if (inst->IsActionFrame(7, 0)) {
        StartShells((int)m3dRandRange(1.0f, 4.0f));
        mngSmoke->StopEmission();
        mngSmoke->StartEmission();
        mngSmoke->SetPosDirSpeed(0, 0, (m3dV *)((u8 *)this + 0xF0));
    }
    m3dV org;
    GetOrgDirTip(&org, 0);
    mngFlare->inst->SetPos(&org);
    partSetAreaEmissionObj(mngSmoke, smokeSource, -1, 0);
}

// v12 0x00414000
// A remote copy: its owner started (bit 1) or stopped shooting -- then its loop stops.
void pwpMINIGUN::ProcessMP_NOTIFY(void *data)
{
    if (((u8 *)data)[6] & 1) {
        remoteShooting = 1;
        return;
    }
    sncCHANNEL *c = (sncCHANNEL *)sndChannel;
    if (c->IsPlay((sncSOUND_DESCR *)&pwpMngSnd[MNG_SND_SHOOT]))
        c->Stop();
    remoteShooting = 0;
}

// v12 0x00414050
void pwpMINIGUN::StopSound()
{
    pwpWEAPON::StopSound();
    ((dynDYNAMIC *)mngFlare)->StartAnim(0, mngFlare->inst->GetSeqLength(0), 0);   // the flare out
}

// v12 0x00413570
// A ray's hit as the other guns', and the flare lit at the muzzle.
void pwpMINIGUN::ShowSFX(pwpMCG_RAY *r)
{
    pwpBULLET_WPN::ShowSFX(r);
    m3dV org, dir;
    GetOrgDirTip(&org, &dir);
    mngFlare->inst->SetPos(&org);
    ((dynDYNAMIC *)mngFlare)->StartAnim(0, 0.0f, 0);
}

// v12 0x00413700  (feb 0x00411E90)
// Two rays: the aim help's, else at random in a spread growing with the distance -- 0.45 within 5, 2.0 from 80 on,
// the up-down spread 0.7 of the sideways one; one that hits nothing goes where the crosshair points.
int pwpMINIGUN::FillCDTShootList(m3dV *org, cdtINFO *aim, cdtINFO *hits, int)
{
    entCDT_REFINE_EXCL ref(0, plrPlayer->inst, 0);
    hits[0] = *aim;
    float dx = org->x - aim->point[0], dy = org->y - aim->point[1], dz = org->z - aim->point[2];
    float d = m3dSqrt(dx * dx + dy * dy + dz * dz), s;
    if (d < 5.0f)
        s = 0.45f;
    else if (d > 80.0f)
        s = 2.0f;
    else
        s = (d - 5.0f) * 0.020666666f + 0.45f;
    m3dV dir = { aim->point[0] - org->x, aim->point[1] - org->y, aim->point[2] - org->z };
    float len = m3dSqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z);
    m3dNormalize(&dir);
    m3dMATR m;
    m._MakeLCS2WCS_VZ(org, &dir);
    int n = FillCDTShootListSpread(org, &dir, len + 10.0f, atanf(s / len) * (180.0f / 3.1415927f), hits, 2);
    if (plrPlayer->moveFlags & 0x800)
        ref.cls.bits |= 1;
    for (; n < 2; n++) {
        m3dV v = { m3dRandRange(-s, s), m3dRandRange(-s, s) * 0.7f, len }, w;
        float *mm = (float *)&m;
        w.x = v.x * mm[0] + v.y * mm[4] + v.z * mm[8];
        w.y = v.x * mm[1] + v.y * mm[5] + v.z * mm[9];
        w.z = v.x * mm[2] + v.y * mm[6] + v.z * mm[10];
        float k = 1.0f / m3dSqrt(w.x * w.x + w.y * w.y + w.z * w.z);
        w.x *= k;
        w.y *= k;
        w.z *= k;
        if (!gsScenePtr->ShootRay(org, &w, len + 10.0f, 0, &ref, &hits[n]))
            hits[n] = *aim;
    }
    return n;
}
