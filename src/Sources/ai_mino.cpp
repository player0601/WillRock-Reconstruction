// ai_mino.cpp -- the minotaur (Will Rock 1.2, v12.exe; feb objs ai_mino.obj, mino_sfx.obj)
//
// "ai_mino": runs at the player and slashes with its axe (SEKIRA); a red one (_gen2 / AI_MINO isRed) is tougher;
// a statue (_statue / AI_PROP isStatue) stands frozen until activated (shot, or a script message), then comes to
// life (STATUE2LIFE); a regenerating one (AI_MINO isRegen, the default for a placed one) bursts into pieces of which
// some regrow into new minotaurs (wrsfxStartChunkMeat 1/2 -> wrsfxCHUNK -> Regenerate). Some throw axes (axCount).
//
// minoFlags (+0x567): 0x8 slashing (in reach), 0x10 appearing (spawned), 0x20 statue, 0x40 statue coming to life,
// 0x80 a regrown one (init data 'ISWA'), 0x100 regrowing, 0x200 regenerates on death, 0x400 fell back dead,
// 0x1000 regrow shader done, 0x4000 the fallback's blood pool made, 0x8000 hit fallback, 0x10000 red,
// 0x20000 (every 2..3 s at random, Slot67).
//
// Not rebuilt (PLACEHOLDER): the axe's flash ("sfx_ax_appear"),
// the stone and regrow shaders (pteTEX2) -- a statue looks like flesh until it moves -- and "ai_mino_atrium".

#include "game.h"
#include "../../include/sfx_sob/sfx_api.h"
#include "../../include/game_shared/projectile.h"

int __fastcall minoAxInit();                                         // v12 0x004642C0 (mino_ax.cpp)
void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int skip);  // v12 0x005130D0 (gs:gs_effect.obj)

// v12 0x0064C368.. the sounds, 0x68 apart (static init v12 0x0044FFC0); the axe's own three follow (fly, hit,
// hit_body: aiMinoGetSound 11..13). [2] mino_roar is never played (nothing asks for it, in feb either).
static sncSOUND_DESCR_3D minoSnd[14];
enum { MSND_APPEAR, MSND_REGEN, MSND_ROAR, MSND_DEATH1, MSND_DEATH2, MSND_HIT, MSND_EXPLOS, MSND_AX_APPEAR,
       MSND_AX_SLASH, MSND_STATUE, MSND_FALL, MSND_AX_FLY, MSND_AX_HIT, MSND_AX_HIT_BODY };

static struct aiMINO_SND_INIT {
    aiMINO_SND_INIT()
    {
        static const struct { const char *name; int flags; float mn, mx; } s[14] = {
            { "mino_appear", 0x40000, 25.5f, 70.0f },
            { "mino_regeneration", 0, 15.0f, 50.0f },
            { "mino_roar", 0x10400, 15.0f, 50.0f },      // (delay 6000)
            { "mino_death_1", 0, 25.5f, 70.0f },
            { "mino_death_2", 0, 25.5f, 70.0f },
            { "mino_hit", 0x40000, 25.5f, 70.0f },
            { "char_explos", 0x20000, 25.5f, 70.0f },
            { "mino_ax_appear", 0x40000, 15.0f, 50.0f },
            { "mino_ax_slash", 0x40000, 15.0f, 50.0f },
            { "mino_statue_appear", 0x40000, 15.0f, 50.0f },
            { "mino_fall", 0x40001, 15.0f, 50.0f },
            { "mino_ax_fly", 0x10002, 3.0f, 18.0f },     // the thrown axe's (mino_ax.cpp): 11..13
            { "mino_ax_hit", 0x20000, 3.9f, 27.0f },
            { "mino_ax_hit_body", 0x20000, 3.9f, 27.0f },
        };
        for (int i = 0; i < 14; i++) {
            sncSOUND_DESCR_3D *d = &minoSnd[i];
            d->name.SetName((char *)s[i].name);
            d->flags = s[i].flags;
            d->volume = 0.0f;
            d->delay = i == MSND_ROAR ? 6000 : 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = s[i].mn;
            d->maxDist = s[i].mx;
            d->vtbl = 0;                                // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} aiMinoSndInit;

// v12 0x0062F570 the textures a red / regrowing / stone minotaur swaps ("%s_red", "%s_chunk", "%sSton")
static char *aiMinoTexBaseList[4] = { (char *)"minotavr1", (char *)"minotavr2", (char *)"minotavr3", (char *)"Sekira" };

#pragma pack(push, 1)
struct aiMINO_INIT : aiINIT_DATA {                      // a regrown one's init data: tag 'ISWA', size 0x14
    int regrown;                                        // +0x10 1
};
#pragma pack(pop)

// (feb 0x00447F40, v12 inline in the AI init)
int __fastcall aiMinoInit()
{
    // "ai_mino_atrium" first: the class table matches by prefix
    if (!entRegisterClass((char *)"ai_mino_atrium", 0x244E5043, 0x32, aiMINO_ATRIUM::Create, (char *)"ai_mino", 0))
        return 0;
    if (!entRegisterClass((char *)"ai_mino", 0x244E5043, 0x32, aiMINO::Create, (char *)"ai_mino", 0))
        return 0;
    if (!minoAxInit())
        return 0;
    if (!minoSFXInit())
        return 0;
    return sncLinkSndListData(14, minoSnd) != 0;
}

// v12 0x00450390
// The minotaur's sound i (the axe asks for 11..13).
sncSOUND_DESCR_3D *__fastcall aiMinoGetSound(int i)
{
    return &minoSnd[i];
}

// v12 0x004650D0  (feb 0x0045B6C0)
int __fastcall minoSFXInit()
{
    static const char *suf[3] = { "_red", "_chunk", "Ston" };
    char b[0x40];
    for (int k = 0; k < 3; k++)
        for (int i = 0; i < 4; i++) {
            sprintf(b, "%s%s", aiMinoTexBaseList[i], suf[k]);
            if (!txmManager->Add(b, 0x40000, 1))
                return 0;
        }
    return 1;
}

// v12 0x00452750  (feb 0x0044A1A0)
entENTITY *__fastcall aiMINO::Create(animINST *)
{
    aiMINO *m = new aiMINO;
    if (!m)
        return 0;
    m->wrFlags = 0;
    m->bloodObj = 0;
    m->minoFlags = 0;
    m->voice = 0;
    m->axAppear = 0;
    m->stoneTex = 0;
    m->regenTex = 0;
    m->f_593 = 0;
    m->f_597 = 0;
    m->axAnim[0] = m->axAnim[1] = 0;
    m->unk98 = 0;
    return m;
}

// v12 0x00450410  (feb 0x00447FB0)
BOOL32 aiMINO::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 0xF;
    u32 *blk = (u32 *)operator new(0x7FC);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 0xF;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 0xF; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    static const char *names[0xF] = {
        "MINO_SEQ_UNUSED", "MINO_SEQ_APPEAR", "MINO_SEQ_RUN", "MINO_SEQ_THROW_AX", "MINO_SEQ_SLASH", "MINO_SEQ_SLASH_2",
        "MINO_SEQ_SLASH_3", "MINO_SEQ_SLASH_4", "MINO_SEQ_DEATH", "MINO_SEQ_DEATH_FALLBACK", "MINO_SEQ_STATUE2LIFE",
        "MINO_SEQ_REGENERATION", "MINO_SEQ_HIT_FALLBACK", "MINO_SEQ_HIT_FALLBACK_2", "MINO_SEQ_IDLE" };
    for (int i = 0; i < 0xF; i++)
        _entAnimSetSeqName(t, i, (char *)names[i]);
    entAnimAssignTplSeq(t, old, nOld);
    t->seq[0].frameStart = 0;                           // (the unused one: one frame, half a second)
    t->seq[0].frameEnd = 1.0f;
    t->seq[0].lengthMs = 500.0f;
    t->seq[0].dir = 0;
    animSYSTEM *s = new animSYSTEM(0xF, 0);
    if (!s)
        return 0;
    t->aniSystem = s;
    s->SetTranTypeToAny(0, 0, 1, -1);
    s->SetTranCycleAnim(2);
    s->SetTranTypeToAny(0xE, 0, 1, -1);
    s->SetTranCycleAnim(0xE);
    s->SetTranTypeToAny(0xA, 1, 1, 2);
    s->SetTranTypeToAny(0xB, 1, 1, 2);
    s->SetTranTypeToAny(1, 1, 1, 2);
    static const int tr[][5] = {
        { 1, 0xE, 1, 0xE, 1 }, { 2, 3, 0, 3, 1 }, { 3, 2, 1, 2, 1 }, { 2, 4, 0, 4, 1 }, { 2, 5, 0, 5, 1 },
        { 2, 6, 0, 6, 1 }, { 2, 7, 0, 7, 1 }, { 4, 2, 0, 2, 1 }, { 5, 2, 0, 2, 1 }, { 6, 2, 0, 2, 1 },
        { 7, 2, 0, 2, 1 }, { 4, 3, 0, 3, 1 }, { 5, 3, 0, 3, 1 }, { 6, 3, 0, 3, 1 }, { 7, 3, 0, 3, 1 },
        { 3, 4, 1, 4, 1 }, { 3, 5, 1, 5, 1 }, { 3, 6, 1, 6, 1 }, { 3, 7, 1, 7, 1 }, { 2, 0xC, 0, 0xC, 1 },
        { 2, 0xD, 0, 0xD, 1 }, { 4, 0xC, 0, 0xC, 1 }, { 4, 0xD, 0, 0xD, 1 }, { 5, 0xC, 0, 0xC, 1 },
        { 5, 0xD, 0, 0xD, 1 }, { 6, 0xC, 0, 0xC, 1 }, { 6, 0xD, 0, 0xD, 1 }, { 7, 0xC, 0, 0xC, 1 },
        { 7, 0xD, 0, 0xD, 1 } };
    for (unsigned i = 0; i < sizeof(tr) / sizeof(tr[0]); i++)
        s->SetTranParam(tr[i][0], tr[i][1], (u8)tr[i][2], (u16)tr[i][3], (u8)tr[i][4]);
    s->SetTranTypeToAny(0xC, 0, 1, -1);
    s->SetTranTypeToAny(0xD, 0, 1, -1);
    s->SetTranParamFromAny(8, 0, 1, 8);
    s->SetTranParamFromAny(9, 0, 1, 9);
    s->SetTranParam(8, 8, 2, 8, 1);
    s->SetTranParam(9, 9, 2, 9, 1);
    s->SetTranParam(0xC, 0xD, 2, 0xC, 1);
    s->SetTranParam(0xD, 0xC, 2, 0xD, 1);
    if (old)
        operator delete((u32 *)old - 1);
    t->seq[9].unk84 |= 1;                               // (DEATH_FALLBACK is moved by its root)
    return 1;
}

// v12 0x00450900  (feb 0x004484A0)
int aiMINO::ProcessINIT(void *init)
{
    turnSpeed = 90.0f;
    fallbackAngle = 90.0f;
    radActivate = 90.0f;
    aiFlags2 |= 0x18000;
    radCDT = 0.9f;
    heightLowTester = 1.75f;
    heightCDT = 1.3f;
    moveSpeed = 11.0f;
    minTurnRadius = 7.7f;
    maxTurnStep = 45.0f;
    aimAngle = 45.0f;
    radActivateBack = 20.0f;
    navArriveDist = 2.0f;
    animRate = 1.0f;
    axCount = 0;
    meetDist = 10.0f;
    if (!(bloodObj = objFindName(inst->root, (char *)"CENTRE")))
        return 0;
    if (!(rootObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    aiMINO_INIT *mi = (aiMINO_INIT *)init;
    if ((flags2 & 1) && mi && *(u32 *)(mi->hdr + 4) == 0x41575349 && mi->regrown)   // 'ISWA'
        minoFlags |= 0x80;
    if (!aiNPC_WR::ProcessINIT(init))
        return 0;
    axTimer = 0;
    roarTimer = m3dRandRange(2.0f, 3.0f);
    maxHealth = health = (minoFlags & 0x80) ? 7.0f : 15.0f;
    if (minoFlags & 0x20)                               // a statue: its animation stands still till it is activated
        aiFlags &= ~0x1000000u;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
    if (c) {
        c->minDist = 0.0f;
        c->maxDist = 1.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    return 1;
}

// v12 0x00450B80  (feb 0x004487A0)
void aiMINO::ParseNameClass()
{
    const char *cls = inst->className ? inst->className : "";
    int v = 0;
    aiNPC_WR::ParseNameClass();
    if (strstr(cls, "_gen2") || (GetInt((char *)"AI_MINO", (char *)"isRed", &v) && v))
        minoFlags |= 0x10000;
    if (strstr(cls, "_atrium"))
        axCount = (int)m3dRandRange(10.0f, 15.0f);
    int regrown = (flags2 & 1) && (minoFlags & 0x80);
    int spawned = ((flags2 & 1) && !(minoFlags & 0x80)) || (minoFlags & 0x10000);
    int statue = (inst->GetBool((char *)"AI_PROP", (char *)"isStatue", &v) && v) || strstr(cls, "_statue");
    if (regrown) {
        minoFlags |= 0x100;
        animRate = m3dRandRange(0.9f, 1.25f);
        ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, 0xB, 0.0f, 0);
        Activate(0);
        maxHealth = health = 7.0f;
        axCount = (int)m3dRandRange(15.0f, 25.0f);
        alertAngle = 360.0f;
        shootRange = 300.0f;
    } else if (spawned) {
        minoFlags |= 0x10;
        if (!(minoFlags & 0x10000))
            minoFlags |= 0x200;
    } else if (statue) {
        radActivate = 12.0f;
        radActivateBack = 12.0f;
        minoFlags |= 0x220;
    } else {
        minoFlags |= 0x200;
    }
    if (GetBool((char *)"AI_MINO", (char *)"isRegen", &v) && !v)
        minoFlags &= ~0x200u;
    ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, EvalAnimGoalSeq(), 0.0f, 0);
    if (minoFlags & 0x10000)
        fallbackAngle = 80.0f;
}

// v12 0x00450E00  (feb 0x00448A30)
void aiMINO::ProcessFRAME()
{
    aiNPC_WR::ProcessFRAME();
    if ((aiFlags & 0x800000) && axAppear && axAppear->inst) {
        axAppear->inst->flags |= 1;
        axAppearTimer = 0;
    }
}

// v12 0x00450E40  (feb 0x00448A70)
void aiMINO::ProcessDESTROY_ENT(entENTITY *e)
{
    aiNPC::ProcessDESTROY_ENT(e);
    if (e == axAppear)
        axAppear = 0;
}

// v12 0x00450E70  (feb 0x00448AA0)
// Fell back dead: where it lands, a pool of blood (once).
void aiMINO::ProcessCDT(cdtINFO *)
{
    if (!(entFlags & 1) && (minoFlags & 0x400) && !(minoFlags & 0x4000)) {
        minoFlags |= 0x4000;
        m3dV p;
        GetPosRooted(&p);
        m3dV v = { passivePhys->vel[0], passivePhys->vel[1], passivePhys->vel[2] };
        StartBlood(4, 1, &p, 0, &v, &p);
    }
}

// v12 0x00450EF0  (feb 0x00448B20)
int aiMINO::IsAnimSeqGoodForFallback()
{
    int s = inst->seq;
    return s == 2 || s == 4 || s == 5 || s == 6 || s == 7;
}

// v12 0x00450F20  (feb 0x00448B50)
void aiMINO::AdjustSpeedsOnFallbackStart(dmgWEAPON *, float *back, float *)
{
    minoFlags |= 0x8000;
    if (inst->seq == 2)
        *back *= 0.75f;
}

// v12 0x00403200 (1.2: the slot holds the shared "return 0"; feb compared the name with "SEKIRA")
int aiMINO::IsArmoredObject(char *)
{
    return 0;
}

// v12 0x00460030  (feb 0x00448BA0; 1.2 dropped the hit sound flag)
int aiMINO::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    aiNPC_WR::ProcessCDT_FIREABLE(d);
    return 1;
}

// v12 0x00450F50  (feb 0x00448BD0)
int aiMINO::ProcessASK_KEEP_LEVEL_RELOAD()
{
    if (f_597)
        f_597->flags2 |= 8;
    if (axAppear)
        axAppear->flags2 |= 8;
    return aiNPC::ProcessASK_KEEP_LEVEL_RELOAD();
}

// v12 0x00450F80  (feb 0x00448C00)
// Blown apart: the pieces (a regenerating one's regrow: 1, or 2 by an explosion of weapon 6), blood all round.
void aiMINO::Explode3(dmgDAMAGE *d, int pieces, int kind)
{
    m3dV *from = 0;
    if (d) {
        if (d->type == 1) {
            if (((dmgWEAPON *)d)->weaponType == 7)
                return;
            from = &((dmgWEAPON *)d)->hitPos;
        } else if (d->type == 3 || d->type == 4) {
            from = &((dmgWEAPON *)d)->hitPos;
        }
    }
    m3dV c;
    bloodObj->GetOrigin(&c);
    int t;
    if (!pieces || (int)aiFlags < 0)
        t = 0;
    else if (d->type == 1 && ((dmgWEAPON *)d)->weaponType == 6)
        t = 2;
    else
        t = 1;
    StartChunkMeat(t, kind, &c, from);
    StartBlood(6, kind, &c, 0, 0, from);
    unk108 |= 0x40;
    msgSystem.PostMsg(this, 2, this);
}

// v12 0x004527E0  (feb 0x0044A230)
void aiMINO::Explode(dmgDAMAGE *d, int dead)
{
    Explode3(d, 0, dead);
}

// v12 0x00451050  (feb 0x00448CD0)
// A regenerating one bursts (unless the death was "of inactivity"); the others fall, back or forward, bleeding.
void aiMINO::StartDeath(dmgDAMAGE *d)
{
    if (axAppear && axAppear->inst)                     // (1.2 has no null check)
        axAppear->inst->flags |= 1;
    aiNPC_WR::StartDeath(d);
    if (!(entFlags & 4) || !d)
        return;
    if (d->type != 1 && d->type != 3 && d->type != 4)
        return;
    m3dV from = ((dmgWEAPON *)d)->hitPos;
    m3dV c, p;
    bloodObj->GetOrigin(&c);
    GetPosRooted(&p);
    if ((minoFlags & 0x200) && !(wrFlags & 4)) {
        if (d->subtype != 2)
            Explode3(d, 1, 0);
        return;
    }
    if (SelectDeathAnim() == 9) {
        minoFlags |= 0x400;
        return;
    }
    StartBlood(3, 1, &c, 0, 0, &from);
}

// v12 0x00451160  (feb 0x00448DE0)
int aiMINO::SelectDeathAnim()
{
    return ((aiFlags & 0x100) | 0x800) >> 8;            // 8 DEATH, 9 DEATH_FALLBACK (falling)
}

static float flatDist(m3dV *a, m3dV *b)
{
    float dx = a->x - b->x, dz = a->z - b->z;
    return sqrtf(dx * dx + dz * dz);
}

// within a slash: closer than 4 across and 3 up or down
static int inReach(aiMINO *m)
{
    m3dV p, e;
    m->inst->GetPos(&p);
    m->enemy.GetPos(&e);
    float d = flatDist(&p, &e);
    return !m->enemy.IsDead() && d < 4.0f && fabsf(p.y - e.y) < 3.0f;
}

// v12 0x00451190  (feb 0x00448E00)
void aiMINO::MoveToGoal()
{
    if (minoFlags & 0x60)
        return;
    if ((minoFlags & 0x10) || (minoFlags & 0x100)) {    // appearing / regrowing: stands
        aiFlags |= 0x200;
        aiNPC_WR::MoveToGoal();
        return;
    }
    if (enemy.IsDead())
        aiFlags |= 0x200;
    int s = inst->seq;
    if ((s >= 3 && s <= 7) || (minoFlags & 0x8000))
        aiFlags |= 0x200;
    aiNPC_WR::MoveToGoal();
    if ((aiFlags & 0x1000) && ((animSYSTEM_INST *)inst->link)->groupBase == 3 && inst->IsActionFrame(3, 0)) {
        axTimer = (aiFlags & 0x40000) ? 1.0f : 4.0f;
        ThrowAx();
        aiFlags |= 0x2000;
    }
    if (minoFlags & 8) {
        int g = ((animSYSTEM_INST *)inst->link)->groupBase;
        if (g == 0xC || g == 0xD) {
            minoFlags &= ~8u;
        } else {
            SingleProcessSlash(g, 4);
            SingleProcessSlash(g, 5);
            SingleProcessSlash(g, 6);
            SingleProcessSlash(g, 7);
        }
    }
    if (aiFlags & 0x10)
        return;
    if (inReach(this) && (aiFlags & 1))
        minoFlags |= 8;
}

// v12 0x00451420  (feb 0x00449090)
// One with axes to throw stops at range (10..90 away, or further than meetDist) and throws.
int aiMINO::SelectGoal()
{
    m3dV p, e;
    inst->GetPos(&p);
    enemy.GetPos(&e);
    if (axCount > 0 && ((aiFlags & 4) || (enemyDist > 10.0f && enemyDist < 90.0f)) && (aiFlags & 0x10000000) &&
        !(aiFlags & 0x80000008) && !(wrFlags & 0x200) && !enemy.IsDead() && !(aiFlags & 0x10) &&
        !(inReach(this) && (aiFlags & 1)) && !(minoFlags & 8) &&
        ((aiFlags & 0x40000) || enemyDist > meetDist || (aiFlags & 4))) {
        aiFlags |= 0x200;
        goalPos = p;
        wayPointPos = e;
        return 1;
    }
    return aiNPC::SelectGoal();
}

// v12 0x00451670  (feb 0x004492D0)
// A statue comes to life (at most 4 minotaurs at a time unless forced).
int aiMINO::Activate(int force)
{
    if (!force && nLocks > 0)
        return 0;
    if (minoFlags & 0x20) {
        if (!force && aiNPC::GetNOpp(0, (char *)"ai_mino") >= 4)
            return 0;
        minoFlags = (minoFlags & ~0x20u) | 0x40;
        aiFlags |= 0x1000000;
    }
    radActivate = 70.0f;
    radActivateBack = 20.0f;
    return aiNPC::Activate(0);
}

// v12 0x004516F0  (feb 0x00449350)
float aiMINO::GetAnimPlayRate()
{
    return (minoFlags & 0x100) ? animRate : 1.0f;
}

// v12 0x00451710  (feb 0x00449370)
int aiMINO::EvalAnimGoalSeq()
{
    int s = ((aiFlags & 0x10000) || enemy.IsDead()) ? 0xE : 2;
    if (aiFlags & 0x200000)
        return ((minoFlags & 0x400) | 0x2000) >> 10;    // 8 / 9
    if (minoFlags & 0x60)
        return 0xA;                                     // STATUE2LIFE (a statue: its first frame, frozen)
    if (minoFlags & 0x10)
        return 1;
    if (minoFlags & 0x100)
        return 0xB;
    if (idleTimer > 1e-6f)
        return 0xE;
    if ((aiFlags & 0x1000) && !(aiFlags & 0x2000))
        return 3;
    if (minoFlags & 0x8000)
        return m3dRandSelect(0xC, 0xD);
    if (minoFlags & 8)
        return m3dRandSelect(4, 5, 6, 7);
    if (navFlags & 1)
        return 2;
    if (aiFlags & 0x40000)
        return (aiFlags & 2) ? 0xE : 2;
    return s;
}

// v12 0x00451830  (feb 0x00449490)
void aiMINO::ProcessActionFrame()
{
    if (inst->IsActionFrame(1, 0))
        minoFlags &= ~0x10u;
    if (inst->IsActionFrame(0xB, 0))
        minoFlags &= ~0x100u;
    if (inst->IsActionFrame(0xA, 0))
        minoFlags &= ~0x40u;
    if (inst->IsActionFrame(0xC, 0) || inst->IsActionFrame(0xD, 0))
        minoFlags &= ~0x8000u;
}

// v12 0x004518D0  (feb 0x00449530)
void aiMINO::Think()
{
    aiNPC_WR::Think();
    if (floorGap > 20.0f && !(aiFlags & 0x200000))
        unk108 |= 0x200;                                // falling: the scream
    if ((aiFlags & 0x200000) && (aiFlags & 0x20000000) && fallDist < -10.0f && m3dRandom() > 0.15f)
        Explode3(0, 0, 0);                              // dead after a long fall: mostly bursts
    if (!(aiFlags & 0x2000) && (aiFlags & 0x10000000) && !(aiFlags & 0x80000008) && !(wrFlags & 0x200) &&
        !enemy.IsDead() && !(aiFlags & 0x10) && !(inReach(this) && (aiFlags & 1)) && !(minoFlags & 8) &&
        ((aiFlags & 0x40000) || enemyDist > meetDist || (aiFlags & 4)) && axTimer < 1e-6f)
        aiFlags = (aiFlags & ~0x2000u) | 0x1000;        // throw
    if ((aiFlags & 0x2000) && ((animSYSTEM_INST *)inst->link)->groupBase != 3)
        aiFlags &= ~0x3000u;
    navArriveDist = (aiFlags & 0x10) ? 3.0f : 2.0f;
}

// v12 0x00451B50 (shared)
void aiMINO::GetWpnTip(m3dV *out)
{
    inst->GetPos(out);
    out->y += 1.0f;
}

// v12 0x0043B900 (shared)
void aiMINO::GetPlayerTargetPos(m3dV *out)
{
    enemy.GetPos(out);
    out->y += 0.01f;
}

// v12 0x0045CD90 / 0x0045A950 (shared)
int aiMINO::GetLayer3State()
{
    return (int)minoFlags;
}
void aiMINO::SetLayer3State(int s)
{
    minoFlags = (u32)s;
}

// v12 0x00451B80  (feb 0x004497B0)
// The slash 'want' playing: its first action frames end the slash and set the next throw's wait; its hit
// frames hurt the enemy in reach for 6.
void aiMINO::SingleProcessSlash(int seq, int want)
{
    dmgGENERAL d;
    if (seq != want)
        return;
    if (inst->IsActionFrame(want, 0) || inst->IsActionFrame(want, 1)) {
        minoFlags &= ~8u;
        axTimer = (aiFlags & 0x40000) ? 1.0f : 3.0f;
    }
    if (!inst->IsActionFrame(want, 1) && !inst->IsActionFrame(want, 2))
        return;
    if (aiFlags & 0x10)
        return;
    if (!inReach(this))
        return;
    m3dV p;
    inst->GetPos(&p);
    d.amount = 6.0f;
    d.pos = p;
    if (enemy.ent)
        msgSystem.SendMsg(enemy.ent, 0x4C6, &d, this);
}

// v12 0x00451D90  (feb 0x004499D0)
void aiMINO::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&axTimer, gsElapsedTime);
    if (aiFlags & 4)
        axTimer = 0;
    if ((aiFlags & 0x10000000) && m3dUpdateTimeField(&roarTimer, gsElapsedTime)) {
        roarTimer = m3dRandRange(2.0f, 3.0f);
        if (m3dRandom() > 0.4f)
            minoFlags |= 0x20000;
        else
            minoFlags &= ~0x20000u;
    }
}

// v12 0x00451E30  aiMINO::AimAxToPlayer (a member there; a helper here: no header change)
// The throw from `from`: at 35 m/s flat towards the point where it meets its enemy (1.3 up) when that is within
// 60 / (distance + 0.1) + aimAngle degrees of its facing, else straight ahead; up so as to come down there under
// gravity 10 (without a meeting point, or wrFlags 0x10: ahead, the enemy's own position for the height).
static void minoAimAx(aiMINO *m, m3dV *from, pjlPJL_INIT *in)
{
    m3dV dir, target, v;
    m->inst->GetDir(&dir);
    if (!(m->wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(m, 35.0f, &target)) {
        target.y += 1.3f;
        v.x = target.x - from->x;
        v.y = 0;
        v.z = target.z - from->z;
        m3dNormalize(&v);
        float a = fabsf(m3xzAngleVector(&v, &dir));
        if (!(60.0f / (m->enemyDist + 0.1f) + m->aimAngle >= a))
            v = dir;
    } else {
        m->enemy.GetPos(&target);
        v = dir;
    }
    v.x *= 35.0f;
    v.y *= 35.0f;
    v.z *= 35.0f;
    float dx = from->x - target.x, dz = from->z - target.z;
    float t = m3dSqrt(dx * dx + dz * dz) * 0.028571429f;          // the flight's time: 1/35 a metre
    in->phys.vel[0] = v.x;
    in->phys.vel[1] = v.y;
    in->phys.vel[2] = v.z;
    in->phys.vel[1] = ((target.y - from->y) - t * t * -5.0f) / t;
    in->phys.accel[0] = m3dVUnitY.x * -10.0f;
    in->phys.accel[1] = m3dVUnitY.y * -10.0f;
    in->phys.accel[2] = m3dVUnitY.z * -10.0f;
}

// v12 0x00452080  (feb 0x00449C30)
// An axe ("mino_ax") from the one it holds (SEKIRA), thrown at the enemy: 8 damage over 3.5 m (general 4), a 0.3
// sphere, 7 s; not hitting the thrower, lit as the floor under it. One fewer to throw; the next in a second.
void aiMINO::ThrowAx()
{
    pjlPJL_INIT in;
    memset(&in, 0, sizeof(in));
    in.phys.packId = 0;
    in.phys.tag = 0x40;
    in.phys.size = 0x55;
    in.phys.vel[0] = m3dVZero.x;
    in.phys.vel[1] = m3dVZero.y;
    in.phys.vel[2] = m3dVZero.z;
    in.phys.accel[0] = m3dVZero.x;
    in.phys.accel[1] = m3dVZero.y;
    in.phys.accel[2] = m3dVZero.z;
    in.phys.radius = 0.1f;
    in.phys.lifetime = 2000000.0f;
    in.phys.maxHits = 1;
    in.damage = -1.0f;
    in.radius = 0;
    in.weaponType = -1;
    in.isGeneral = -1;
    in.ownerId = 0xFFFFFFFF;
    in.termDelay = -1.0f;
    in.explosive = 1;
    animCREATE_DATA cd;
    inst->Validate(4);
    objOBJ *ax = objFindName(inst->root, (char *)"SEKIRA");
    if (!ax)
        return;
    in.phys.radius = 0.3f;
    in.phys.lifetime = 7.0f;
    in.phys.maxHits = 1;
    in.phys.friction = 0;
    in.ownerId = (u32)mpId;
    in.damage = 8.0f;
    in.radius = 3.5f;
    in.isGeneral = 4;
    m3dV org, dir;
    ax->GetOrigin(&org);
    inst->GetDir(&dir);
    minoAimAx(this, &org, &in);
    memcpy(&cd.matr, ax->ltm, sizeof(cd.matr));
    struct { pjlPJL_INIT i; u32 extra; } blk;           // (0x55 bytes: a zero dword after the init)
    blk.i = in;
    blk.extra = 0;
    entENTITY *e = entCreate(gsScenePtr, (char *)"mino_ax", (char *)"mino_ax", &cd, &blk, 0);
    if (e) {
        ((physPHYS *)e)->AddEntCDTExclude(this);
        gsEffApplyLMChar(e, &floor, 0);
    }
    if (--axCount > 0)
        axTimer = 1.0f;
}

// v12 0x00452240  (feb 0x00449DF0)
void aiMINO::SettleSurface()
{
    m3dV p;
    inst->GetPos(&p);
    aiNPC::SettleSurface();
    if (minoFlags & 0x80) {                             // a regrown one stays where its piece lay
        inst->Translate(&p, 0);
        fallDist = 1.0f;
    }
}

// v12 0x00452290  (feb 0x00449E40)
void aiMINO::FillSoundFlags()
{
    for (int s = 4; s <= 7; s++)
        if (inst->IsActionFrame(s, 1) || inst->IsActionFrame(s, 2)) {
            unk108 |= 0x80;                             // the swing
            break;
        }
    int s = inst->seq;
    if (inst->flags & 0x4000) {
        switch (s) {
        case 8: unk108 |= 8; break;
        case 9: unk108 |= 0x10; break;
        case 0xB: unk108 |= 2; break;
        case 0xC:
        case 0xD: unk108 |= 0x20; break;
        }
    } else if (s == 1) {
        unk108 |= 1;
    } else if (s == 0xA && !(minoFlags & 0x20)) {
        unk108 |= 0x100;                                // the statue comes to life
    }
}

// v12 0x004523F0  (feb 0x00449F70)
void aiMINO::PlaySounds()
{
    static const struct { u32 bit; int snd; } t[] = {
        { 0x40, MSND_EXPLOS }, { 8, MSND_DEATH1 }, { 0x10, MSND_DEATH2 }, { 2, MSND_REGEN }, { 0x20, MSND_HIT },
        { 1, MSND_APPEAR }, { 0x100, MSND_STATUE }, { 0x200, MSND_FALL }, { 0x80, MSND_AX_SLASH } };
    for (unsigned i = 0; i < sizeof(t) / sizeof(t[0]); i++)
        if (unk108 & t[i].bit) {
            voice->Play(&minoSnd[t[i].snd], 0);
            break;
        }
    aiNPC::PlaySounds();
}

// v12 0x00452510  (feb 0x0044A090)
// A regrowing piece is done: a new minotaur where it lies, ten times its size, pushed out of the walls.
aiNPC *__fastcall aiMINO::Regenerate(physPHYS *chunk)
{
    animCREATE_DATA cd;
    aiMINO_INIT in;
    in.hdr[2] = 0x14;
    in.hdr[3] = 0;
    *(u32 *)(in.hdr + 4) = 0x41575349;                  // 'ISWA'
    in.regrown = 0;
    cdtREFINE ref(2, 0);
    cdtINFO ci;
    memset(&ci, 0, sizeof(ci));
    float sx, sy, sz;
    ((m3dMATR *)chunk->inst->root->child->local)->GetScale(&sx, &sy, &sz);   // (the piece's first object)
    float k = sx * 10.0f;
    memcpy(&cd.matr, chunk->inst->matr, sizeof(cd.matr));
    cd.scale.x = cd.scale.y = cd.scale.z = k;
    m3dV p;
    cd.matr.GetOrigin(&p);
    gsScenePtr->FloorPlace(&p, 2000000.0f, &p, 1.8f, 0, 0x20, &ref, &ci);
    p.y = p.y + 1e-6f + 0.9f;
    gsScenePtr->ConstrSphere(&p, &p, 0.9f, 0x10, &ref, &ci);
    gsScenePtr->FloorPlace(&p, 2000000.0f, &p, 1.8f, 0, 0x20, &ref, &ci);
    cd.matr.Translate(&p, 0);
    in.regrown = 1;
    return (aiNPC *)entCreate(gsScenePtr, (char *)"ai_mino", (char *)"ai_mino", &cd, &in, 0);
}

// (feb 0x0045BD70, mino_sfx.obj) a regrowing piece was made: the spawn domain counted it already
void __fastcall aiMINO::ChunkCreateNotify(physPHYS *)
{
}

// v12 0x004526D0  (feb 0x0044A150)
int aiMINO::IsReadyActivate()
{
    if ((minoFlags & 0x20) && nLocks > 0)
        return 0;
    return aiNPC_WR::IsReadyActivate();
}

// v12 0x004526F0  (feb 0x0044A170)
void aiMINO::SetMovementSpeed()
{
    if (idleTimer > 1e-6f)
        aiFlags |= 0x200;
}

// v12 0x00452720 (1.2 new slot)
int aiMINO::Slot67()
{
    return (aiFlags & 0x10000000) && (minoFlags & 0x20000);
}

// v12 0x004527D0  (feb 0x0044A220)
int aiMINO::DoesNPCAllowSFXAppear()
{
    return minoFlags & 0x10;
}

// v12 0x004651B0  (feb 0x0045B7A0)
int aiMINO::InitSFX()
{
    aiNPC_WR::InitSFX();
    if ((flags2 & 1) && (minoFlags & 0x80)) {
        inst->root->ReplaceTexList(aiMinoTexBaseList, 4, (char *)"_red");
        // PLACEHOLDER (test): regenTex, a pteTEX2 blending "%s_chunk" -> "%s_red" over the regrowth
        // (REGENERATION's length / animRate, spline 0 / 0.65 / 0.85 / 1) -- pteTEX2 is not rebuilt
    }
    if (minoFlags & 0x10000)
        inst->root->ReplaceTexList(aiMinoTexBaseList, 4, (char *)"_red");
    // PLACEHOLDER (test): axAppear = entCreate("sfx_ax_appear", "dynamic") (scaled 10, hidden, its planes
    // '&ds_pPlane1_&nf' / '&ds_pPlane2_&nf' with 4 x 8 frame animations) -- "sfx_ax_appear" is not rebuilt;
    // 1.2 fails the init without it, here the minotaur goes on without the flash
    axAppear = 0;
    axAppearTimer = 0;
    msgADDR::flags |= 2;
    // PLACEHOLDER (test): a statue's stoneTex (pteTEX2 "%sSton", blend 1, faded by STATUE2LIFE) -- not rebuilt
    return 1;
}

// v12 0x00465580  (feb 0x0045BB80)
void aiMINO::TermSFX()
{
    regenTex = 0;                                       // (PLACEHOLDER: never made)
    aiNPC_WR::TermSFX();
    if (axAppear) {
        msgSystem.PostMsg(axAppear, 2, this);
        axAppear = 0;
    }
}

// v12 0x004655D0  (feb 0x0045BBD0) UpdateSFX
// The slash's flash on the axe, and the axe-appear sound with it.
void aiMINO::Slot33()
{
    if (inst->IsActionFrame(3, 1) && !(aiFlags & 0x200000)) {
        if (axAppear && axAppear->inst)
            axAppear->inst->flags &= ~1u;
        axAppearTimer = 0.8f;
        voice->Play(&minoSnd[MSND_AX_APPEAR], 0);
    }
    if (axAppear && axAppear->inst) {
        inst->Validate(4);
        objOBJ *ax = objFindName(inst->root, (char *)"SEKIRA");
        if (ax)
            axAppear->inst->Transform((m3dMATR *)((char *)ax + 0x3C), 0);
    }
    if (m3dUpdateTimeField(&axAppearTimer, gsElapsedTime) && axAppear && axAppear->inst)
        axAppear->inst->flags |= 1;
    if (aiFlags & 0x800000) {
        if (axAppear && axAppear->inst)
            axAppear->inst->flags |= 1;
        axAppearTimer = 0;
    }
}

// ================================================================================================================
// aiMINO_ATRIUM ("ai_mino_atrium"): a minotaur of a coliseum stage. While the player is not in the arena yet (his
// stage 0) and it stands on one, it throws 2..5 axes from up there (300 m range) and, at an edge in front of it,
// turns back; dying there it falls off the stage (DEATH) on its own fling. Its activation radius 150 either way.
// (The arena lookups are not rebuilt: until they are, every one behaves as a plain minotaur.)

// v12 0x00452800  (feb 0x0044A250)
entENTITY *__fastcall aiMINO_ATRIUM::Create(animINST *)
{
    aiMINO_ATRIUM *m = new aiMINO_ATRIUM;               // operator new(0x5A3), aiNPC ctor
    if (!m)
        return 0;
    m->wrFlags = 0;
    m->bloodObj = 0;
    m->minoFlags = 0;
    m->voice = 0;
    m->axAppear = 0;
    m->stoneTex = 0;
    m->regenTex = 0;
    m->f_593 = 0;
    WR_FIELD(u32, m, 0x597) = 0;
    m->axAnim[0] = 0;
    m->axAnim[1] = 0;
    m->unk98 = 0;
    return m;
}

// v12 0x00452880  (feb 0x0044A2D0)
int aiMINO_ATRIUM::ProcessINIT(void *init)
{
    if (!aiMINO::ProcessINIT(init))
        return 0;
    int plr = aiaArenaDomList->GetPlrStageNmb();
    int own = aiaArenaDomList->GetNPCStageNmb(this);
    if (own > 0 && plr == 0)
        minoFlags &= ~0x200u;
    radActivateBack = 150.0f;
    radActivate = 150.0f;
    if (own > 0 && plr != own)
        axesLeft = (int)m3dRandRange(2.0f, 5.0f);
    else
        axesLeft = 0;
    shootRange = 300.0f;
    return 1;
}

// v12 0x00452930  (feb 0x0044A380)
int aiMINO_ATRIUM::Activate(int force)
{
    if (!aiMINO::Activate(force))
        return 0;
    radActivateBack = 150.0f;
    radActivate = 150.0f;
    return 1;
}

// v12 0x00452960  (feb 0x0044A3B0)
void aiMINO_ATRIUM::ThrowAx()
{
    aiMINO::ThrowAx();
    axesLeft--;
}

// v12 0x00452980  (feb 0x0044A3D0)
// On a stage with axes left, chasing: no floor 8 m below a point 0.7 m ahead (1 m up) -> the edge: it turns (0x200).
void aiMINO_ATRIUM::MoveToGoal()
{
    cdtREFINE ref(2, 0);
    int plr = aiaArenaDomList->GetPlrStageNmb();
    int own = aiaArenaDomList->GetNPCStageNmb(this);
    if (own > 0 && plr == 0 && axesLeft > 0 && (aiFlags & 0x10000000)) {
        m3dV p, d, a, b;
        inst->GetPos(&p);
        inst->GetDir(&d);
        p.y += 1.0f;
        a.x = d.x * 0.7f + p.x;
        a.y = d.y * 0.7f + p.y;
        a.z = d.z * 0.7f + p.z;
        b.x = a.x - m3dVUnitY.x * 8.0f;
        b.y = a.y - m3dVUnitY.y * 8.0f;
        b.z = a.z - m3dVUnitY.z * 8.0f;
        if (!gsScenePtr->IsVisiblePoint(&a, &b, 0, &ref, 0) && axesLeft > 0)
            aiFlags |= 0x200;
    }
    aiMINO::MoveToGoal();
}

// v12 0x00452AE0  (feb 0x0044A520) exact: on a stage before the player comes, the plain DEATH (8)
int aiMINO_ATRIUM::SelectDeathAnim()
{
    int plr = aiaArenaDomList->GetPlrStageNmb();
    int own = aiaArenaDomList->GetNPCStageNmb(this);
    if (own > 0 && plr == 0)
        return 8;
    return aiMINO::SelectDeathAnim();
}

// v12 0x00452B20  (feb 0x0044A560)
// On a stage before the player comes: thrown off it along its facing (5 m/s, 2.25 up, falling at 10), bouncing 0,
// sliding 0.8, never ending by hits; the fall from its place, not hitting itself. Else the usual.
int aiMINO_ATRIUM::StartPassivePhysics(dmgDAMAGE *d)
{
    physINIT p;
    memset(&p, 0, sizeof(p));
    p.packId = 0;
    p.tag = 0x40;
    p.size = 0x38;
    p.radius = 0.1f;
    p.lifetime = 2000000.0f;
    p.maxHits = 1;
    int plr = aiaArenaDomList->GetPlrStageNmb();
    int own = aiaArenaDomList->GetNPCStageNmb(this);
    if (own <= 0 || plr != 0)
        return aiNPC_WR::StartPassivePhysics(d);
    p.radius = 0.2f;
    p.maxHits = -1;
    p.friction = 0.8f;
    memcpy(&p.bounce, &plr, 4);                         // (the stage number 0: bounce 0)
    p.lifetime = -1.0f;
    m3dV dir;
    inst->GetDir(&dir);
    p.vel[0] = dir.x * 5.0f;
    p.vel[1] = 2.25f;
    p.vel[2] = dir.z * 5.0f;
    p.accel[0] = m3dVUnitY.x * -10.0f;
    p.accel[1] = m3dVUnitY.y * -10.0f;
    p.accel[2] = m3dVUnitY.z * -10.0f;
    passivePhys->ProcessINIT(&p);
    memcpy((u8 *)passivePhys + 8, inst->matr, 0x40);
    WR_FIELD(u32, passivePhys, 4) |= 4;
    passivePhys->AddEntCDTExclude(this);
    return 1;
}
static_assert(sizeof(aiMINO_ATRIUM) == 0x5A3, "aiMINO_ATRIUM");
