// ai_skel_generic.cpp -- what all skeletons share (Will Rock 1.2, v12.exe; feb obj ai_skel_generic.obj)
//
// One template (ai_skel) and two classes: "ai_skel_kamik" (aiSKEL_SWORD) and "ai_skel" (aiSKEL_SPEAR: _sword, _spear,
// _flexible by the class name). skelFlags: 1 in a niche, 2 coming out of it, 4 appearing, 8 walking out, 0x10 lying,
// 0x20 getting up, 0x40 in a grave, 0x80 jumps out of it.
// On death the model breaks into its pieces (aiSKEL_RIGID_START: rgdRIGID_START, the rigid bodies, not rebuilt).

#include "game.h"
#include "../../include/sfx_sob/sfx_api.h"

// v12 0x0064CFF0 the sounds; 13.. the spear's hits (with the projectile, not rebuilt)
static sncSOUND_DESCR_3D skelSnd[13];

static struct aiSKEL_SND_INIT {
    aiSKEL_SND_INIT()
    {
        static const struct { const char *name; int flags; float mn, mx; } s[13] = {
            { "skel_run", 0x10000, 5.0f, 15.0f },
            { "skel_death", 0x60000, 30.0f, 80.0f },
            { "skel_sword_appear", 0x40000, 15.0f, 50.0f },
            { "skel_sword_jump_out", 0x40000, 15.0f, 50.0f },
            { "skel_spear_jump_out", 0x40000, 15.0f, 50.0f },
            { "skel_rising", 0x40000, 15.0f, 50.0f },
            { "skel_spear_appear", 0x40000, 25.5f, 70.0f },
            { "skel_spear_far_attack", 0x40000, 15.0f, 50.0f },
            { "skel_sword_close_attack", 0x40000, 15.0f, 50.0f },
            { "skel_sword_stab_1", 0x40000, 15.0f, 50.0f },
            { "skel_sword_stab_2", 0x40000, 15.0f, 50.0f },
            { "skel_sword_stab_3", 0x40000, 15.0f, 50.0f },
            { "skel_sword_far_attack", 0x40000, 15.0f, 50.0f },
        };
        for (int i = 0; i < 13; i++) {
            sncSOUND_DESCR_3D *d = &skelSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].mn;
            d->maxDist = s[i].mx;
            d->vtbl = 0;                                // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} aiSkelSndInit;

// v12 0x00458490  (feb 0x0044FA70)
int __fastcall aiSkelSpearPjlInit();                   // ai_skel_spear_pjl.cpp
int __fastcall aiSkeletonsAllInit()
{
    if (!entRegisterClass((char *)"ai_skel_kamik", 0x244E5043, 0x32, aiSKEL_SWORD::Create, (char *)"ai_skel", 0))
        return 0;
    if (!entRegisterClass((char *)"ai_skel", 0x244E5043, 0x32, aiSKEL_SPEAR::Create, (char *)"ai_skel", 0))
        return 0;
    if (!aiSkelSpearPjlInit())
        return 0;
    if (!wrsfxSkeletonInit())
        return 0;
    return sncLinkSndListData(13, skelSnd) != 0;
}

// v12 0x00458500  (feb 0x0044FAE0)
BOOL32 aiSKEL_GENERIC::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 0x15;
    u32 *blk = (u32 *)operator new(0xB2C);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 0x15;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 0x15; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    static const char *names[0x15] = {
        "SKEL_SPEAR_SEQ_RUN_SPEAR", "SKEL_SPEAR_SEQ_THROW_MOVE", "SKEL_SPEAR_SEQ_THROW_STAY", "SKEL_SPEAR_SEQ_IDLE_EMPTY",
        "SKEL_SPEAR_SEQ_SWING_A", "SKEL_SPEAR_SEQ_SWING_B", "SKEL_SPEAR_SEQ_SWING_SPEAR", "SKEL_SPEAR_SEQ_APPEAR_WEAPON",
        "SKEL_SPEAR_SEQ_RUN_SWORD", "SKEL_SPEAR_SEQ_IDLE_SWORD", "SKEL_SPEAR_SEQ_IDLE_SPEAR", "SKEL_SPEAR_SEQ_APPEAR_SP_SKEL",
        "SKEL_SPEAR_SEQ_APPEAR_SW_SKEL", "SKEL_SWORD_SEQ_SW_RUN", "SKEL_SWORD_SEQ_SW_ATTACK", "SKEL_SWORD_SEQ_SW_IDLE",
        "SKEL_SWORD_SEQ_SW_APPEAR", "SKEL_SPEAR_SEQ_STAND_REG_SWORD", "SKEL_SPEAR_SEQ_STAND_KAMIKAZE",
        "SKEL_SPEAR_SEQ_STAND_SPEAR", "SKEL_SPEAR_SEQ_STAB" };
    for (int i = 0; i < 0x15; i++)
        _entAnimSetSeqName(t, i, (char *)names[i]);
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(0x15, 0);
    if (!s) {
        if (t->seq)
            operator delete((u32 *)t->seq - 1);
        return 0;
    }
    t->aniSystem = s;
    static const int tr[][5] = {
        { 1, 1, 2, 1, 1 }, { 2, 2, 2, 2, 1 }, { 7, 7, 2, 7, 1 }, { 0xB, 0xB, 2, 0xB, 1 }, { 0xC, 0xC, 2, 0xC, 1 },
        { 0x11, 0x11, 2, 0x11, 1 }, { 0x13, 0x13, 2, 0x13, 1 }, { 0xE, 0xE, 2, 0xE, 1 }, { 0x10, 0x10, 2, 0x10, 1 },
        { 8, 4, 0, 4, 1 }, { 8, 5, 0, 5, 1 }, { 8, 6, 0, 6, 1 }, { 8, 0x14, 0, 0x14, 1 }, { 0, 0x14, 0, 0x14, 1 },
        { 9, 8, 0, 8, 1 }, { 10, 0, 0, 0, 1 }, { 0xF, 0xD, 0, 0xD, 1 }, { 0, 1, 1, 1, 0 }, { 1, 7, 1, 7, 0 },
        { 2, 7, 1, 7, 0 }, { 0xD, 0xE, 1, 0xE, 0 }, { 0xE, 0xC, 1, 0xC, 0 }, { 0x11, 0xC, 0, 0xC, 1 },
        { 0x11, 0x10, 0, 0x10, 1 }, { 0x12, 0xC, 0, 0xC, 1 }, { 0x12, 0x10, 0, 0x10, 1 }, { 0x13, 0xB, 0, 0xB, 1 } };
    for (unsigned i = 0; i < sizeof(tr) / sizeof(tr[0]); i++)
        wrVCall<void>(s, 4, tr[i][0], tr[i][1], tr[i][2], tr[i][3], tr[i][4]);
    if (old)
        operator delete((u32 *)old - 1);
    t->seq[1].unk84 |= 1;                               // (moved by their root: THROW_MOVE, SW_ATTACK)
    t->seq[0xE].unk84 |= 1;
    return 1;
}

// v12 0x00458930  (feb 0x0044FF10)
int aiSKEL_GENERIC::ProcessINIT(void *init)
{
    if (!aiNPC_WR::ProcessINIT(init))
        return 0;
    corpseTime = 3.0f;
    corpseFadeTime = 2.0f;
    radCDT = 0.6f;
    heightLowTester = 0.72f;
    heightCDT = 1.2f;
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    shootRange = 300.0f;
    heightUp1 = 1.9f;
    fallbackAngle = 30.0f;
    rootObj = objFindName(inst->root, (char *)"ROOT");
    bloodObj = rootObj;
    wrFlags |= 2;
    skelFlags = 0;
    unk108 = 0;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
    if (c) {
        c->minDist = 0.0f;
        c->maxDist = 1.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    if (flags2 & 1)
        skelFlags |= 4;
    return 1;
}

// v12 0x00458B00  (feb 0x00450120)
// By name or sheet: in a niche, lying down, in a grave; they wait, hidden from collision, for Activate.
void aiSKEL_GENERIC::ParseNameClass()
{
    const char *cls = inst->className ? inst->className : "";
    aiNPC_WR::ParseNameClass();
    int niche = 0;
    if ((aiFlags2 & 0x20) || (inst->GetBool((char *)"AI_PROP", (char *)"isNiche", &niche) && niche)) {
        timeDeactivate = 1000.0f;
        skelFlags |= 1;
    }
    if (strstr(cls, "_lay")) {
        skelFlags |= 0x10;
        inst->flags |= 0x10;
    }
    if (strstr(cls, "_grave")) {
        skelFlags |= 0x40;
        inst->flags |= 0x10;
        heightUpTester = 1.8f;
        unk177 = 1.25f;
        inst->flags |= 1;
    }
}

// v12 0x00458C20  (feb 0x00450220)
int aiSKEL_GENERIC::IsReadyActivate()
{
    if (skelFlags & 0x50)
        return 0;
    return aiNPC_WR::IsReadyActivate();
}

// v12 0x00458C40  (feb 0x00450240)
// The blue armour.
void aiSKEL_GENERIC::GetBlueClothes()
{
    if (inst && inst->root) {
        inst->root->ReplaceTex((char *)"armour1", (char *)"armour1a");
        inst->root->ReplaceTex((char *)"armour2", (char *)"armour2a");
    }
}

// v12 0x00458C80  (feb 0x00450280)
int aiSKEL_GENERIC::Activate(int force)
{
    if (skelFlags & 1) {
        aiFlags &= ~0x10000u;
        skelFlags = (skelFlags & ~1u) | 2;
        m3dV p, d;
        inst->GetPos(&p);
        inst->GetDir(&d);
        goalPos.x = d.x * 6.0f + p.x;
        goalPos.y = d.y * 6.0f + p.y;
        goalPos.z = d.z * 6.0f + p.z;
        wayPointPos = goalPos;
        skelFlags |= 8;
    }
    if (skelFlags & 0x10) {
        skelFlags = (skelFlags & ~0x10u) | 0x20;
        unk108 |= 0x4000;
    }
    if (skelFlags & 0x40) {
        skelFlags = (skelFlags & ~0x40u) | 0x84;
        inst->flags &= ~1u;
    }
    inst->flags &= ~0x10u;
    return aiNPC::Activate(force);
}

// v12 0x00458D90  (feb 0x004503A0)
void aiSKEL_GENERIC::StartDeath(dmgDAMAGE *d)
{
    StartDeath3(d, 0, 1);
}

// v12 0x00458DA0  (feb 0x004503C0)
// Unless the damage is an instant death (subtype 2): the death sound and, unless it goes by itself, the dust and its pieces
// thrown away from the hit, the entity gone. The harder the hit and the nearer, the faster.
void aiSKEL_GENERIC::StartDeath3(dmgDAMAGE *d, cdtINFO *ci, int shatter)
{
    if (!d || d->subtype != 2) {
        unk108 = 2;
        voice->flags |= 0x400;
        if (!IsDeadVoluntary()) {
            float size = 0.07f;
            aiSKEL_RIGID_START r;
            r.sfx = (wrsfxSKELETON_DEATH *)entCreate(gsScenePtr, (char *)"sfx_skel_death", (char *)"", 0, &size, 0);
            if (!r.sfx)
                return;
            if (shatter) {
                m3dV v;
                float k;
                if (!d) {
                    if (!ci || (ci->inst && !(ci->inst->flags & 0x48)))
                        inst->GetDir(&v);
                    else
                        gsScenePtr->GetCDTFace(ci, 0, &v);
                    k = 4.0f;
                } else {
                    m3dV p, org;
                    float dist;
                    GetPosRooted(&p);
                    dmgGetOrgDir(d, &p, &org, &v, &dist);
                    float a = d->amount, base = 3.0f;
                    if (a >= 5.0f)
                        base = a <= 20.0f ? (a - 5.0f) * 0.26666668f + 3.0f : 7.0f;
                    if (dist < 0.0f)
                        k = 2.0f * base;
                    else if (dist <= 15.0f)
                        k = (2.0f - dist * 0.06666667f) * base;
                    else
                        k = base;
                }
                v.y = 0;
                if (!_m3dNormalize(&v)) {
                    v = m3dVUnitY;
                    k = 1.0f;
                }
                r.impulse.x = v.x * k;
                r.impulse.y = v.y * k;
                r.impulse.z = v.z * k;
                r.StartExplos(this);
                bloodObj = 0;
                rootObj = 0;
                inst->root = 0;
                msgSystem.PostMsg(this, 2, 0);
            }
        }
    }
    aiNPC_WR::StartDeath(d);
}

// v12 0x00459000  (feb 0x00450620)
int aiSKEL_GENERIC::SelectGoal()
{
    if (aiFlags & 0x200000)
        aiFlags |= 0x200;
    if (skelFlags & 2) {
        if (!(aiFlags & 1))
            return 1;
        skelFlags &= ~0xAu;
        timeDeactivate = 5.0f;
        OrientTowardsPlayer();
        aiFlags &= ~1u;
        enemy.GetPos(&goalPos);
    }
    return aiNPC::SelectGoal();
}

// v12 0x00459080  (feb 0x004506A0)
void aiSKEL_GENERIC::MoveToGoal()
{
    if (skelFlags & 4)
        aiFlags |= 0x200;
    aiNPC_WR::MoveToGoal();
}

// v12 0x004590A0  (feb 0x004506C0)
int aiSKEL_GENERIC::GetStateEntExcludeCDT()
{
    return aiNPC::GetStateEntExcludeCDT() | ((skelFlags & 2) ? 8 : 0);
}

// v12 0x00459410  (feb 0x00450A10)
void aiSKEL_GENERIC::PlaySounds()
{
    static const struct { u32 bit; int snd; } t[] = {
        { 1, 0 }, { 2, 2 }, { 4, 1 }, { 8, 6 }, { 0x10, 7 }, { 0x20, 8 }, { 0x100, 9 }, { 0x200, 10 }, { 0x400, 11 },
        { 0x40, 12 }, { 0x1000, 3 }, { 0x2000, 4 }, { 0x4000, 5 } };
    for (unsigned i = 0; i < sizeof(t) / sizeof(t[0]); i++)
        if (unk108 & t[i].bit)
            voice->Play(&skelSnd[t[i].snd], 0);
    aiNPC::PlaySounds();
}

// v12 0x004595C0  (feb 0x00450BA0)
void aiSKEL_GENERIC::UpdateDead()
{
    unk108 = 2;
    aiNPC_WR::UpdateDead();
}

// v12 0x004595D0  (feb 0x00450BB0)
int aiSKEL_GENERIC::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    return aiNPC_WR::ProcessCDT_FIREABLE(d);
}

// v12 0x0045A960
// 1.2: a move cycle that went on 15 s ends.
int aiSKEL_GENERIC::Slot43()
{
    return cycleTime > 15.0f;
}

// v12 0x0045CD90 / 0x0045A950
int aiSKEL_GENERIC::GetLayer2State()
{
    return skelFlags;
}

void aiSKEL_GENERIC::SetLayer2State(int s)
{
    skelFlags = s;
}
