// ai_cyclop.cpp -- the cyclops (the 2_5 coliseum boss) and the mini cyclops, and the stones they spit
// (Will Rock 1.2, v12.exe; feb objs ai_cyclop.obj, ai_cyc_stone_pjl.obj)
//
// "ai_cyclop": the boss stands caged (cycFlags 1, its instance frozen) till the player sees it, then breaks out
// (APPEAR); in the coliseum it is activated then. It walks at the player and, by the distance and the angle,
// clubs (ATTACK / CLOSE_ATTACK: a blast before it, 100 hp within 15, a scorch mark and dust), spits a volley of
// 10..14 stones (SPIT), or grabs (GRAB). Hit by a big gun it flinches (SHOT) or roars. "_mini" in the class name:
// a small one (half size x AI_PROP size), quicker, 300 hp, never a boss, spits one stone every 0.05..0.3 s.
//
// cycFlags (+0x567): 1 caged (asleep), 2 appearing, 4 hit (may roar), 8 hit by a big gun (flinch), 0x10 reacting
// to a hit, 0x20 a close attack wanted, 0x100 a spit wanted, 0x200 a club wanted, 0x400 grabbing, 0x800 the grab
// caught (nothing in 1.2 sets it), 0x1000 after a grab, 0x2000 spitting, 0x4000 the next stone due, 0x8000 out of
// its nav area: walking back, 0x10000000 a mini.
// unk108 sound bits: 1 grab missed, 2 death, 4 break the cage, 8 grab, 0x10 / 0x20 the club hits the ground,
// 0x40 roar, 0x80 / 0x100 steps, 0x200 spit, 0x400 hit.
//
// PLACEHOLDER (test): the dust effects "sfx_cyclop_walk", "sfx_cyclop_hit", "sfx_cyclop_death" (sfx_cyclop.obj,
// wrsfxCyclopInit v12 0x0046E530: 12 emitters) are registered as empty effects that go at once.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"
#include "../../include/game_shared/projectile.h"

struct wrsfxSCORCHMARK {                                // (not rebuilt: the test prints "scorchmark: not made")
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
                                                        // v12 0x00467DE0 (feb 0x0045DED0)
};

txmTEXTURE *aiCYCLOP::pTexScorchMark;

// v12 0x0064A418 the boss's sounds (17, 0x68 apart), v12 0x0064AB00 a mini's (11). (static init v12 0x004416F0)
static sncSOUND_DESCR_3D cycSnd[17], miniSnd[11];
enum { CSND_EAT, CSND_DEATH, CSND_CAGE, CSND_GET_STONES, CSND_HIT_GROUND, CSND_HIT_GROUND_2, CSND_ROAR, CSND_STEP_1,
       CSND_STEP_2, CSND_SPIT, CSND_HIT, CSND_STONE_FLY, CSND_STONE_GROUND_1, CSND_STONE_BODY = 16 };

static void cycSndSet(sncSOUND_DESCR_3D *d, const char *name, int flags, float mn, float mx)
{
    d->name.SetName((char *)name);
    d->flags = flags;
    d->volume = 0.0f;
    d->delay = 0;
    d->count = 0;
    d->time = 0;
    d->data = 0;
    d->dataFlags = 0;
    d->minDist = mn;
    d->maxDist = mx;
    d->vtbl = 0;
}

static struct aiCYCLOP_SND_INIT {
    aiCYCLOP_SND_INIT()
    {
        static const struct { const char *name; int flags; float mn, mx; } b[17] = {
            { "cyclop_eat_stones", 0x40000, 50.0f, 200.0f }, { "cyclop_death", 0x40000, 90.0f, 200.0f },
            { "cyclop_break_cage", 0x40000, 90.0f, 200.0f }, { "cyclop_get_stones", 0x40000, 60.0f, 110.0f },
            { "cyclop_hit_ground", 0, 70.0f, 200.0f }, { "cyclop_hit_ground_2", 0, 70.0f, 200.0f },
            { "cyclop_roar", 0x40000, 120.0f, 220.0f }, { "cyclop_walk_step_1", 0x40000, 90.0f, 150.0f },
            { "cyclop_walk_step_2", 0x40000, 90.0f, 150.0f }, { "cyclop_spit_stones", 0x40000, 50.0f, 160.0f },
            { "cyclop_being_hit", 0x40000, 50.0f, 160.0f }, { "cyclop_stone_fly", 0x10002, 0, 0 },
            { "cyclop_stone_hit_ground_1", 0x60000, 0, 0 }, { "cyclop_stone_hit_ground_2", 0x60000, 0, 0 },
            { "cyclop_stone_hit_ground_3", 0x60000, 0, 0 }, { "cyclop_stone_hit_ground_4", 0x60000, 0, 0 },
            { "cyclop_stone_hit_body", 0x60000, 0, 0 } };
        static const struct { const char *name; int flags; float mn, mx; } m[11] = {
            { "miniclop_eat_stones", 0x40000, 15.0f, 50.0f }, { "miniclop_death", 0x40000, 30.0f, 80.0f },
            { "cyclop_break_cage", 0x40000, 15.0f, 50.0f }, { "miniclop_get_stones", 0x40000, 30.0f, 80.0f },
            { "miniclop_hit_ground", 0, 30.0f, 80.0f }, { "miniclop_hit_ground", 0, 30.0f, 80.0f },
            { "cyclop_roar", 0x40000, 25.5f, 70.0f }, { "cyclop_walk_step_1", 0x40000, 10.0f, 33.0f },
            { "cyclop_walk_step_2", 0x40000, 10.0f, 33.0f }, { "cyclop_spit_stones", 0x40000, 25.5f, 70.0f },
            { "miniclop_being_hit", 0x40000, 25.5f, 70.0f } };
        for (int i = 0; i < 17; i++)
            cycSndSet(&cycSnd[i], b[i].name, b[i].flags, b[i].mn, b[i].mx);
        for (int i = 0; i < 11; i++)
            cycSndSet(&miniSnd[i], m[i].name, m[i].flags, m[i].mn, m[i].mx);
    }
} aiCyclopSndInit;

// v12 0x00444780 (feb 0x0043D380)
static sncSOUND_DESCR_3D *__fastcall aiCyclopGetSound(int i) { return &cycSnd[i]; }

// ================================================================================================================
// the stones
// ================================================================================================================
static txmTEXTURE *pjlStoneTexTrail;                    // v12 0x0064A3E0 "sfx_skel_spear_appear_part"
static const float stoneSize[6] = { 0.8f, 0.6f, 0.3f, 0.8f, 0.6f, 0.3f };      // v12 0x0062D598 (3..5: a mini's)
static const float stoneDamage[6] = { 20.0f, 15.0f, 12.0f, 11.0f, 10.0f, 9.0f }; // v12 0x0062D5B0
static const char *stoneTpl[6] = { "pjl_stone1_cyclop", "pjl_stone2_cyclop", "pjl_stone3_cyclop",
                                   "pjl_stone1_cyclop", "pjl_stone2_cyclop", "pjl_stone3_cyclop" }; // v12 0x0062D580

#pragma pack(push, 1)
struct pjlSTONE_INIT : pjlPJL_INIT {                    // size 0x5D
    u32 f_51;                                           // +0x51 0
    int mini;                                           // +0x55 a mini's (smaller dust)
    int type;                                           // +0x59 0..5 (stoneSize, stoneDamage, stoneTpl)
};
#pragma pack(pop)

static m3dSPL_LINEAR1D *cycSpl(int n)
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

class pjlSTONE_TRAIL : public partEMITTER_PHYS {
public:
    pjlSTONE_TRAIL(float size);
};

// v12 0x00441260
// Grit falling off the stone in flight.
pjlSTONE_TRAIL::pjlSTONE_TRAIL(float size) : partEMITTER_PHYS(300.0f, 0.2f)
{
    SetIntensity(300.0f, 0);
    SetTimeLifePart(0.2f, 0);
    m3dSPL_LINEAR1D *sz = cycSpl(2);
    float s2 = size + size;
    sz->SetKp(0, 0.0f, s2 * 1.5f);
    sz->SetKp(1, 1.0f, s2 * 0.0f);
    SetSize(size * 1.5f, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = cycSpl(2);
    a->SetKp(0, 0.0f, 127.5f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(pjlStoneTexTrail);
    SetTextureRotAnim(0.0f, 180.0f, 0.0f, 90.0f);
    flags |= 4;
    spreadSpl = 0;
    spreadMin = 0;
    speedK = 0.1f;
    spread = size * 0.1f;
    speedKRand = 0;
    speedA = size;
    speedARand = 0;
    speedASpl = 0;
    flags |= 0x200;
    flags |= 0x409;
    flags |= 0xA000;
    flags |= 0x70;
    SetDegFarParams(5.0f, 30.0f, 3.0f, 0.3f);
    SetDegNearParams(3.0f, 0.3f);
}

struct pjlSTONE_SFX {                                   // 8 bytes at pjlPJL +0x16F
    partEMITTER_PHYS *emitter;
    objOBJ           *trailObj;                         // "trail_emitter"
    pjlSTONE_SFX() : emitter(0), trailObj(0) {}
    // v12 0x004411B0
    int StartSFX(physPHYS *p, pjlPJL_INIT *init)
    {
        pjlSTONE_INIT *in = (pjlSTONE_INIT *)init;
        int t = in->type;
        if (t < 0 || t >= 6)
            t = 0;
        float s = 0.07f / stoneSize[0] * stoneSize[t];
        s = in->mini ? s * 0.7f : s + s;
        emitter = new pjlSTONE_TRAIL(s);
        if (!emitter)
            return 0;
        trailObj = objFindName(p->inst->root, (char *)"trail_emitter");
        if (!trailObj)
            return 0;
        if (emitter)
            partSetAreaEmissionObj(emitter, trailObj, -1, 0);
        emitter->StartEmission();
        return 1;
    }
    // v12 0x0045BC60
    void UpdateSFX(physPHYS *)
    {
        if (emitter)
            partSetAreaEmissionObj(emitter, trailObj, -1, 0);
    }
    void StopSFX(physPHYS *)
    {
        if (emitter) {
            emitter->StopEmission();
            emitter = 0;
        }
    }
};

#pragma pack(push, 1)
class pjlSTONE : public pjlNPC_PJL<pjlSTONE_SFX> {
public:
    objOBJ  *rotCenter;                                 // +0x1BC "rot_center": what spins
    u32      stoneFlags;                                // +0x1C0 0x10 hit (bit 1 would stop the spin: never set)
    int      type;                                      // +0x1C4
    float    hitRadius;                                 // +0x1C8 a player this near is hit (its size + 0.5)
    m3dV     spinAxis;                                  // +0x1CC
    float    spinSpeed;                                 // +0x1D8 30..70 deg/s
    // size v12 0x1DC

    pjlSTONE() : rotCenter(0), stoneFlags(0), type(0), hitRadius(0), spinSpeed(0) { spinAxis = m3dVZero; }
    static entENTITY *__fastcall Create(animINST *i);
    BOOL32 ProcessINIT(physINIT *p);
    void   ProcessFRAME();
    void   GenerateCDTUponHittingPlayer();
    void   ProcessEV_CDT(pjlEV_CDT *ev);
    sncSOUND_DESCR_3D *GetSound(int i);
    void   GetSndDist(float *mn, float *mx);
    int    GetLayerAppState() { return (int)pjlFlags; }   // v12 0x004416D0
    void   SetLayerAppState(int s) { pjlFlags = (u32)s; } // v12 0x004416E0
};
#pragma pack(pop)

// v12 0x0043FF80
entENTITY *__fastcall pjlSTONE::Create(animINST *)
{
    return new pjlSTONE;
}

// v12 0x0043FF30
int __fastcall aiCycStonePjlInit()
{
    if (!entRegisterClass((char *)"pjl_stone_cyclop", 0x53544F4E, 0x32, pjlSTONE::Create, 0, 1))
        return 0;
    return (pjlStoneTexTrail = txmManager->Add((char *)"sfx_skel_spear_appear_part", 0x40003, 1)) != 0;
}

// v12 0x00440070 (with pjlPJL<SFX>::ProcessINIT v12 0x00441440)
// The type; the damage and the rest, the owner out of its collision, the grit, the sound channel (by the size);
// born inside a wall: hit at once; a random spin.
BOOL32 pjlSTONE::ProcessINIT(physINIT *p)
{
    pjlSTONE_INIT *in = (pjlSTONE_INIT *)p;
    type = in->type;
    if (type < 0 || type >= 6)
        type = 0;
    hitRadius = stoneSize[type] + 0.5f;
    // -- pjlPJL<SFX>::ProcessINIT (v12 0x00441440)
    if (!physPHYS::ProcessINIT(p))
        return 0;
    damage = in->damage;
    radius = in->radius;
    isGeneral = in->isGeneral;
    weaponType = in->weaponType;
    termDelay = in->termDelay;
    explosive = in->explosive;
    if (entFlags & 4) {
        if (in->ownerId != 0xFFFFFFFF) {
            entIACTIVE_OBJ *e = gsSysMP->FindMPID((int)in->ownerId, 4);
            if (e)
                AddEntCDTExclude((entENTITY *)e);
        }
        ownerId = in->ownerId;
    }
    sfx.StartSFX(this, in);
    if (HasSound()) {
        float mn, mx;
        GetSndDist(&mn, &mx);
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        if (c) {
            c->minDist = mn;
            c->maxDist = mx;
        }
        sndChannel = c;
        if (!sndChannel)
            return 0;
        gsSysSound->AddChannel(sndChannel, this, 0);
    }
    pjlFlags |= 0x12;
    // --
    if ((entFlags & 4) && in->ownerId != 0xFFFFFFFF) {
        entENTITY *owner = (entENTITY *)gsSysMP->FindMPID((int)in->ownerId, 4);
        if (owner && owner->unk98 != 0x14) {
            cdtREFINE ref(2, owner->inst);
            cdtINFO ci;
            memset(&ci, 0, sizeof(ci));
            m3dV op, sp;
            owner->inst->GetPos(&op);
            inst->GetPos(&sp);
            m3dV chest = op;
            chest.y = op.y + 0.3f;
            m3dV head = op;
            head.y = ((aiNPC *)owner)->Slot90() + op.y;     // (owner vtable +0x168: heightUp1)
            if (!gsScenePtr->IsVisiblePoint(&head, &sp, 1, &ref, &ci) ||
                !gsScenePtr->IsVisiblePoint(&chest, &sp, 1, &ref, &ci)) {
                pjlHIT_MSG m;
                m.packId = 0;
                m.tag = 0x40;
                m.size = 0x24;
                entENTITY *he = ci.inst ? ci.inst->ent : 0;
                m.mpId = he ? (u32)he->mpId : 0xFFFFFFFF;
                m.ci0 = ci.flags;
                m.ci8 = (u32)ci.objId;
                m.ciC = (u32)ci.faceId;
                m.point = *(m3dV *)ci.point;
                memcpy(&m.ci1C, &ci.dist, 4);
                msgSystem.PostMsgData(this, 0x4D3, &m, this);
            }
        }
    }
    rotCenter = objFindName(inst->root, (char *)"rot_center");
    if (!rotCenter)
        return 0;
    stoneFlags = 0;
    m3dMakeVUnitRandom(&spinAxis);
    spinSpeed = m3dRandMax(40.0f) + 30.0f;
    math->flags |= 4;                                   // moves as a sphere
    return 1;
}

// v12 0x00440300
// The nearest player within reach is hit as by a collision.
void pjlSTONE::GenerateCDTUponHittingPlayer()
{
    cdtINFO ci;
    memset(&ci, 0, sizeof(ci));
    m3dV at;
    entENTITY *who = 0;
    entNearestPlayerDist(this, 1.5f, &at, &who);
    ci.flags |= 2;
    inst->GetPos((m3dV *)ci.point);
    ci.inst = who ? who->inst : 0;
    msgSystem.SendMsg(this, 0x3F9, &ci, this);
}

// v12 0x004405A0
// Flies (unless a cut-scene holds the action), the grit follows, the sounds; the state to the clients; it spins.
// On the server: at rest, it goes; a player within reach is hit.
void pjlSTONE::ProcessFRAME()
{
    if (!((gsSysCinema->state & 1) && gsSysCinema->IsSuspendAction())) {
        physPHYS::ProcessFRAME();
        sfx.UpdateSFX(this);
        PlaySounds();
    }
    if (GetLayerAppState() != lastLayerState) {
        pjlEV_LAYER ev;
        ev.packId = 0;
        ev.tag = 0x40;
        ev.size = 0xA;
        ev.type = 7;
        ev.state = GetLayerAppState();
        gsSysServer->SendMsgClientAll(this, 0x40E, &ev, 0x10000, 0xFFFFFFFD, 0xFFFFFFFD);
        lastLayerState = ev.state;
    }
    if (!(stoneFlags & 1)) {
        float a = gsElapsedTime * spinSpeed;
        ((m3dMATR *)rotCenter->local)->RotateX(a * spinAxis.x, 1);
        ((m3dMATR *)rotCenter->local)->RotateY(a * spinAxis.y, 1);
        ((m3dMATR *)rotCenter->local)->RotateZ(a * spinAxis.z, 1);
        rotCenter->DeclareProcAnim();
        inst->Invalidate(4);
    }
    if (!(entFlags & 4))
        return;
    m3dV v = *(m3dV *)math->vel;
    if (fabsf(m3dLength(v)) < 0.2f)
        Terminate();
    if ((entFlags & 4) && !(stoneFlags & 1)) {
        m3dV at;
        if (entNearestPlayerDist(this, 1.5f, &at, 0) < hitRadius) {
            GenerateCDTUponHittingPlayer();
            if (math)
                math->vel[0] = math->vel[1] = math->vel[2] = 0;
            Terminate();
        }
    }
}

// v12 0x00440850
// A hit: it bounces on the level (till its hits run out), anything else stops it.
void pjlSTONE::ProcessEV_CDT(pjlEV_CDT *ev)
{
    pjlFlags |= PJLF_HIT;
    stoneFlags |= 0x10;
    ShowCollisSFX(ev);
    pjlFlags |= ev->kind == 1 ? PJLF_SND_STATIC : PJLF_SND_ENTITY;
    if (math->nHits < math->init.maxHits && (ev->kind == 1 || ev->kind == 0))
        return;
    if (sfx.emitter) {
        sfx.emitter->StopEmission();
        sfx.emitter = 0;
    }
    Terminate();
}

// v12 0x00440440
// 0 flying, 1 the level (one of four, louder the faster), 2 a body.
sncSOUND_DESCR_3D *pjlSTONE::GetSound(int i)
{
    if (i == 0)
        return aiCyclopGetSound(CSND_STONE_FLY);
    if (i == 2)
        return aiCyclopGetSound(CSND_STONE_BODY);
    if (i != 1)
        return 0;
    sncSOUND_DESCR_3D *s = aiCyclopGetSound(m3dRandSelect(12, 13, 14, 15));
    float sp = m3dLength(*(m3dV *)math->vel);
    if (sp > 10.0f)
        sp = 10.0f;
    WR_FIELD(float, sndChannel, 0x54) = sp * 4.0f - 40.0f;   // the channel's volume
    return s;
}

// v12 0x004403B0
// The bigger, the further heard: 5..15 near, 20..60 far.
void pjlSTONE::GetSndDist(float *mn, float *mx)
{
    *mn = 15.0f - (float)type * 2.0f;
    *mx = 60.0f - (float)type * 8.0f;
    if (*mn < 5.0f)
        *mn = 5.0f;
    else if (*mn > 15.0f)
        *mn = 15.0f;
    if (*mx < 20.0f)
        *mx = 20.0f;
    else if (*mx > 60.0f)
        *mx = 60.0f;
}

// ================================================================================================================
// PLACEHOLDER (test): the dust effects (sfx_cyclop.obj) -- empty, they go at once
// ================================================================================================================
struct wrsfxCYCLOP_STUB : entENTITY {
    BOOL32 ProcessMsg(int id, void *, msgADDR *, int *)
    {
        if (id == 1)
            msgSystem.PostMsg(this, 2, this);
        return 0;
    }
    static entENTITY *__fastcall Create(animINST *) { return new wrsfxCYCLOP_STUB; }
};

// v12 0x0046E530 (PLACEHOLDER (test): registers the three effects as wrsfxCYCLOP_STUB)
int __fastcall wrsfxCyclopInit()
{
    static const char *n[3] = { "sfx_cyclop_walk", "sfx_cyclop_hit", "sfx_cyclop_death" };
    for (int i = 0; i < 3; i++)
        if (!entRegisterClass((char *)n[i], 0x24534658, 0x50, wrsfxCYCLOP_STUB::Create, 0, 1))
            return 0;
    return 1;
}

// ================================================================================================================
// the cyclops
// ================================================================================================================

// v12 0x00441EA0  (feb 0x0043AE90)
int __fastcall aiCyclopInit()
{
    if (!entRegisterClass((char *)"ai_cyclop", 0x244E5043, 0x32, aiCYCLOP::Create, (char *)"ai_cyclop", 0))
        return 0;
    if (!aiCycStonePjlInit())
        return 0;
    if (!wrsfxCyclopInit())
        return 0;
    if (!sncLinkSndListData(17, cycSnd))
        return 0;
    if (!sncLinkSndListData(11, miniSnd))
        return 0;
    return (aiCYCLOP::pTexScorchMark = txmManager->Add((char *)"sfx_ax_scorch_mark", 0x40003, 1)) != 0;
}

// v12 0x00441F30  (feb 0x0043AF00)
entENTITY *__fastcall aiCYCLOP::Create(animINST *)
{
    aiCYCLOP *c = new aiCYCLOP;
    if (!c)
        return 0;
    c->wrFlags = 0;
    c->bloodObj = 0;
    c->voice = 0;
    c->unk98 = 0x11;
    return c;
}

// v12 0x00441F80  (feb 0x0043AF50)
BOOL32 aiCYCLOP::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 12;
    u32 *blk = (u32 *)operator new(0x664);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 12;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 12; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    static const char *names[12] = { "CYCLOP_SEQ_APPEAR", "CYCLOP_SEQ_WALK", "CYCLOP_SEQ_ATTACK", "CYCLOP_SEQ_IDLE",
                                     "CYCLOP_SEQ_ROAR", "CYCLOP_SEQ_DEATH", "CYCLOP_SEQ_GRAB", "CYCLOP_SEQ_SPIT",
                                     "CYCLOP_SEQ_MUNCH", "CYCLOP_SEQ_BLINK", "CYCLOP_SEQ_SHOT",
                                     "CYCLOP_SEQ_CLOSE_ATTACK" };
    for (int i = 0; i < 12; i++)
        _entAnimSetSeqName(t, i, (char *)names[i]);
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(12, 0);
    if (!s) {
        if (t->seq)
            operator delete((u32 *)t->seq - 1);
        return 0;
    }
    t->aniSystem = s;
    s->SetTranParamFromAny(10, 0, 1, 10);
    s->SetTranParamFromAny(5, 0, 1, 5);
    static const int tr[][5] = {
        { 3, 2, 0, 2, 1 }, { 3, 6, 0, 6, 1 }, { 0, 0, 2, 0, 1 }, { 4, 4, 2, 4, 1 }, { 5, 5, 2, 5, 1 },
        { 10, 10, 2, 10, 1 }, { 1, 2, 1, 2, 0 }, { 0, 2, 1, 2, 0 }, { 0, 1, 1, 6, 0 }, { 1, 7, 1, 7, 0 },
        { 2, 1, 1, 3, 0 }, { 10, 1, 1, 3, 0 }, { 10, 7, 1, 7, 0 }, { 3, 1, 0, 1, 1 }, { 0, 7, 1, 6, 0 } };
    for (unsigned i = 0; i < sizeof(tr) / sizeof(tr[0]); i++)
        s->SetTranParam(tr[i][0], tr[i][1], (u8)tr[i][2], (u16)tr[i][3], (u8)tr[i][4]);
    if (old)
        operator delete((u32 *)old - 1);
    t->seq[2].unk84 |= 1;                               // (moved by their root)
    t->seq[5].unk84 |= 1;
    t->seq[11].unk84 |= 1;
    t->seq[2].unk84 |= 2;
    t->seq[1].unk84 |= 2;
    return 1;
}

// v12 0x004422B0  (feb 0x0043B280)
int aiCYCLOP::ProcessINIT(void *init)
{
    aiNPC_WR::ProcessINIT(init);
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    maxHealth = 4000.0f;
    navArriveDist = 4.0f;
    new1B7 = 0.6f;
    alertAngle = 180.0f;
    moveSpeed = 5.06f;
    turnSpeed = 90.0f;
    closeDist = 5.2f;
    size = 1.0f;
    f_58B = 30.0f;
    timeDeactivate = 3600.0f;
    health = 4000.0f;
    cycFlags = 1;
    unk108 = 0;
    shotSeq = 9;
    attackSeq = 9;
    maxTurnStep = 20.0f;
    attackTimer = 0;
    spitTimer = 0;
    outsideTime = 0;
    clubBorder = 888.888f;
    spitBorder = 888.888f;
    returnCount = 0;
    aiFlags2 |= 0x38042;
    navFlags |= 4;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
    if (c) {
        c->minDist = 0.0f;
        c->maxDist = 1.0f;
    }
    voice = c;
    if (!voice)
        return 0;
    gsSysSound->AddChannel(voice, this, 0);
    if (!(attackFocus = objFindName(inst->root, (char *)"AttackFocus")))
        return 0;
    if (!(pivotObj = objFindName(inst->root, (char *)"RotationPIVOT")))
        return 0;
    if (!(rootObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    gsScenePtr->SetStateYesInst(inst, 8);
    inst->root->SetStateYes(0x20);
    lFoot = objFindName(inst->root, (char *)"LKOP");
    rFoot = objFindName(inst->root, (char *)"RKOP");
    return lFoot && rFoot;
}

// v12 0x004427C0
// (1.2: only the boss let go -- the base TERM is not called)
void aiCYCLOP::ProcessTERM()
{
    if (!(cycFlags & 0x10000000))
        aiAttackPlanner->SetBoss(0);
}

// v12 0x004425A0  (feb 0x0043B570)
// "_mini": half size (x AI_PROP size), 300 hp, quicker, its club off; else the boss, caged (its instance still).
void aiCYCLOP::ParseNameClass()
{
    const char *cls = inst->className ? inst->className : "";
    aiNPC_WR::ParseNameClass();
    cycFlags &= ~3u;
    if (strstr(cls, "_mini")) {
        cycFlags = (cycFlags & ~3u) | 0x10000000;
        closeDist = -1.0f;
        maxHealth = 300.0f;
        health = 300.0f;
        size = 0.5f;
        inst->Scale(0.5f, 0.5f, 0.5f, 2);
        float k;
        if (GetFloat((char *)"AI_PROP", (char *)"size", &k))
            size *= k;
        radCDT = heightLowTester = size * 6.0f;
        heightCDT = size * 10.0f;
        heightUpTester = size * 5.0f;
        heightUp1 = size * 11.0f;
        moveSpeed = size * moveSpeed * 2.9f;
        if (aiFlags & 0x40000) {
            objOBJ *o = objFindName(inst->root, (char *)"DUBINA");
            if (o)
                o->SetStateProcYes(8, 0);
        }
        corpseTime = 10.0f;
        corpseFadeTime = 9.0f;
        fallbackAngle = 400.0f;
    } else {
        aiFlags2 |= 0x40000;
        cycFlags |= 1;
        corpseTime = 15.0f;
        corpseFadeTime = 14.0f;
        fallbackAngle = 10000.0f;
        WR_FIELD(u32, inst, 8) |= 4;                    // still, till seen
    }
    ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, EvalAnimGoalSeq(), 0.0f, 0);
    if (!(cycFlags & 0x10000000))
        aiAttackPlanner->SetBoss(this);
}

// v12 0x004427E0  (feb 0x0043B780)
// Seen: out of the cage (APPEAR); in the coliseum it comes at once.
void aiCYCLOP::ActivatePlrVisibility()
{
    if (cycFlags & 1) {
        cycFlags = (cycFlags & ~1u) | 2;
        WR_FIELD(u32, inst, 8) &= ~4u;
        if ((aiFlags & 0x10000) && strstr(gsSceneFullName, "2_5_coliseum"))
            Activate(0);
    }
    aiNPC::ActivatePlrVisibility();
}

// v12 0x00442840  (feb 0x0043B7E0)
// The club comes down 7 (x size) ahead of AttackFocus: 100 within 15 (a mini 50 within 7.5), a scorch mark, dust.
void aiCYCLOP::SendClubDamage()
{
    animCREATE_DATA cd;
    aiDMG_DISPATCHER_TO_SIBLING disp;
    disp.amount = 0;
    m3dV p, dir;
    ((m3dMATR *)attackFocus->ltm)->GetOrigin(&p);
    inst->GetDir(&dir);
    float k = size * -7.0f;
    p.x += dir.x * k;
    p.y += dir.y * k;
    p.z += dir.z * k;
    float dmg, rad;
    if (cycFlags & 0x10000000) {
        dmg = 50.0f;
        rad = 7.5f;
    } else {
        dmg = 100.0f;
        rad = 15.0f;
    }
    dmgDispatchExplosDamage(this, &p, dmg, rad, 4, -1, 0, 0xFFFFFFFF, &disp);
    ((m3dMATR *)attackFocus->ltm)->GetOrigin(&p);
    wrsfxSCORCHMARK::Add(size * 2.5f, 10.0f, pTexScorchMark, &p, 0, 0x5343524D, 0, 0);   // 'SCRM'
    p.y += 0.5f;
    cd.matr._MakeLCS2WCS_VY(&p, &m3dVUnitY);
    struct { u32 mini; float size; } d = { (cycFlags >> 28) & 1, 0.1f };
    entCreate(gsScenePtr, (char *)"sfx_cyclop_hit", (char *)"sfx_cyclop_hit", &cd, &d, 0);
}

// a step: the sound, the dust under the foot, 10 within 5
void aiCYCLOP::Step(objOBJ *foot, u32 snd)
{
    unk108 |= snd;
    animCREATE_DATA cd;
    m3dV p;
    foot->GetOrigin(&p);
    p.y -= size * 1.2f;
    cd.matr._MakeLCS2WCS_VY(&p, &m3dVUnitY);
    struct { u32 mini; float size; } d = { (cycFlags >> 28) & 1, 0.13f };
    entCreate(gsScenePtr, (char *)"sfx_cyclop_walk", (char *)"sfx_cyclop_walk", &cd, &d, 0);
    aiDMG_DISPATCHER_TO_SIBLING disp;
    disp.amount = 0;
    foot->GetOrigin(&p);
    p.y -= size * 1.2f;
    dmgDispatchExplosDamage(this, &p, 10.0f, 5.0f, 4, -1, 0, 0xFFFFFFFF, &disp);
}

// v12 0x004429E0  (feb 0x0043B970)
void aiCYCLOP::ProcessFRAME()
{
    aiNPC_WR::ProcessFRAME();
    if ((cycFlags & 0x2000) && (cycFlags & 0x4000)) {
        if (cycFlags & 0x10000000)
            LaunchOneStone();
        else
            LaunchSeveralStones();
    }
}

// v12 0x00442A10  (feb 0x0043B9A0)
// Hit: dead -> DEATH; else (no flinch lately) a roar now and then, or by a big gun near the facing a flinch (SHOT);
// else BLINK.
int aiCYCLOP::DecideShotSequence()
{
    int s;
    if (aiFlags & 0x200000) {
        s = 5;
    } else if (shootFallbackTimer <= 1e-6f && (cycFlags & 4) && !(m3dRandom() < 0.7f)) {
        s = 4;
    } else if (shootFallbackTimer <= 1e-6f && !(cycFlags & 4) && (cycFlags & 8) && enemyAngle < 60.0f) {
        s = 10;
    } else {
        s = 9;
    }
    cycFlags &= ~0xCu;
    if (s != 9)
        cycFlags |= 0x10;
    return s;
}

// the attack, by the chances worked out: close, club (CLOSE_ATTACK near, ATTACK far) or spit
int aiCYCLOP::ChooseAttack()
{
    clubBorder = CalcBorderForClubAttack();
    spitBorder = CalcBorderForSpitAttack();
    if (enemyDist < closeDist) {
        cycFlags |= 0x20;
    } else {
        if (m3dRandom() < clubBorder) {
            if (!(cycFlags & 0x200))
                entFlags |= 0x100;
            cycFlags |= 0x200;
        }
        if (m3dRandom() < spitBorder) {
            if (!(cycFlags & 0x100))
                entFlags |= 0x100;
            cycFlags |= 0x100;
        }
    }
    float r = enemyDist / size;
    int s;
    if (cycFlags & 0x20)
        s = 0xB;
    else if (!(cycFlags & 0x200))
        s = 7;
    else if (r < 10.0f)
        s = 0xB;
    else if (r < 20.0f)
        s = m3dRandSelect(0xB, 2);
    else
        s = 2;
    cycFlags &= ~0x320u;
    aiFlags |= 0x1000;
    attackSeq = s;
    return s;
}

// v12 0x00442AA0  (feb 0x0043BA40)
int aiCYCLOP::EvalAnimGoalSeq()
{
    if (cycFlags & 1)
        return 0;
    if (cycFlags & 2)
        return 0;
    if (aiFlags & 0x200000)
        return 5;
    if (enemy.IsDead())
        return m3dRandom() < 0.8f ? 3 : 4;
    if (cycFlags & 0xC)
        return shotSeq = DecideShotSequence();
    if (cycFlags & 0x10)
        return shotSeq;
    if (aiFlags & 0x40000) {                            // standing (a sniper's place)
        if (!(enemyAngle <= 30.0f))
            return 1;
        if (cycFlags & 0x1000)
            return 7;
        if (aiFlags & 0x10000000) {
            if (m3dRandom() < 0.65f)
                return 7;
            return m3dRandom() < 0.5f ? 6 : 4;
        }
        return m3dRandSelect(4, 3, 3);
    }
    if (aiFlags & 0x1000) {
        if (attackSeq == 7 || attackSeq == 2 || attackSeq == 0xB)
            return attackSeq;
        return ChooseAttack();
    }
    if (cycFlags & 0x320)
        return ChooseAttack();
    return 1;
}

// v12 0x00442E70  (feb 0x0043BDA0)
void aiCYCLOP::ProcessActionFrame()
{
    if (inst->IsActionFrame(0, 0))
        unk108 |= 4;                                    // the cage breaks
    inst->IsActionFrame(0, 1);
    inst->IsActionFrame(0, 2);
    if (inst->IsActionFrame(0, 3)) {                    // out of the cage: the boss's real size
        u32 f = cycFlags;
        cycFlags &= ~2u;
        if ((aiFlags2 & 0x40000) && !(cycFlags & 3)) {
            heightCDT = 8.5f;
            radCDT = 5.0f;
            heightLowTester = 5.0f;
            heightUpTester = 3.0f;
            heightUp1 = 9.0f;
        }
        (void)f;
    }
    if (inst->IsActionFrame(1, 0))
        Step(lFoot, 0x80);
    if (inst->IsActionFrame(1, 1))
        Step(rFoot, 0x100);
    if (!inst->IsActionFrame(1, 2))
        inst->IsActionFrame(1, 3);
    if (inst->IsActionFrame(4, 0))
        unk108 |= 0x40;
    if (inst->IsActionFrame(4, 1))
        cycFlags &= ~0x10u;
    if (inst->IsActionFrame(10, 0))
        unk108 |= 0x400;
    if (inst->IsActionFrame(10, 1))
        cycFlags &= ~0x10u;
    if (inst->IsActionFrame(10, 2))
        Step(rFoot, 0x80);
    if (inst->IsActionFrame(2, 0))
        Step(lFoot, 0x80);
    if (inst->IsActionFrame(2, 1))
        Step(rFoot, 0x10);
    if (inst->IsActionFrame(2, 2))
        SendClubDamage();
    if (inst->IsActionFrame(2, 3)) {
        aiFlags &= ~0x1000u;
        attackTimer = 1.5f;
        m3dV p;
        GetPosRooted(&p);
        if ((aiFlags2 & 8) && !((scnDOMAIN *)navArea)->IsBelongPointXZ(&p)) {
            returnCount = 30;
            cycFlags |= 0x8000;
        }
    }
    if (inst->IsActionFrame(0xB, 2))
        unk108 |= 0x20;
    if (inst->IsActionFrame(0xB, 0))
        SendClubDamage();
    if (inst->IsActionFrame(0xB, 1))
        aiFlags &= ~0x1000u;
    if (inst->IsActionFrame(6, 0)) {
        cycFlags |= 0x400;
        unk108 |= 8;
    }
    if (inst->IsActionFrame(6, 1)) {
        u32 f = cycFlags;
        cycFlags &= ~0x400u;
        if (f & 0x800) {                                // caught: eaten
            dmgGENERAL d;
            cycFlags &= ~0x800u;
            GetPlayerTargetPos(&d.pos);
            d.amount = 400.0f;
            if (enemy.ent)
                msgSystem.SendMsg(enemy.ent, 0x4C6, &d, this);
        } else {
            unk108 |= 1;
        }
    }
    if (inst->IsActionFrame(6, 2)) {
        aiFlags &= ~0x1000u;
        attackTimer = 2.0f;
        cycFlags |= 0x1000;
    }
    if (inst->IsActionFrame(7, 0)) {                    // the spit: stones from now on; after it walk, or grab
        cycFlags |= 0x6000;
        unk108 |= 0x200;
        pjlEV_LAYER ev;                                 // (6 bytes used: the header and a short)
        ev.packId = 0;
        ev.tag = 0x40;
        ev.size = 6;
        ev.type = m3dRandom() <= 0.8f ? 0xC : 0xB;
        gsSysServer->SendMsgClientAll(this, 0x40E, &ev, 0x20000, 0xFFFFFFFD, 0xFFFFFFFD);
    }
    if (inst->IsActionFrame(7, 1)) {
        aiFlags &= ~0x1000u;
        cycFlags &= ~0x7000u;
        spitTimer = 0;
        attackTimer = 2.0f;
    }
    inst->IsActionFrame(7, 2);
    if (inst->IsActionFrame(5, 0)) {
        struct { u32 mini; float size; aiCYCLOP *who; } d = { (cycFlags >> 28) & 1, 0.1f, this };
        entCreate(gsScenePtr, (char *)"sfx_cyclop_death", (char *)"", 0, &d, 0);
    }
    inst->IsActionFrame(5, 1);
    if (inst->IsActionFrame(5, 2))
        Step(rFoot, 0x80);
    if (inst->IsActionFrame(5, 3))
        Step(lFoot, 0x100);
}

// v12 0x00443AC0  (feb 0x0043C840)
// Caged: still. A mini plays its moves quicker.
float aiCYCLOP::GetAnimPlayRate()
{
    if (cycFlags & 1)
        return 0.0f;
    if (!(cycFlags & 0x10000000))
        return 1.0f;
    switch (inst->seq) {
    case 3: case 8: case 9: case 4: return 1.0f;
    case 1: case 0xA: return 2.9f;
    case 5: case 7: return 1.4f;
    case 0xB: return 1.2f;
    case 2: return 2.4f;
    case 6: return 1.6f;
    }
    return 1.0f;
}

// v12 0x00443B50  (feb 0x0043C880)
// The chance of a club attack: none unless it sees the enemy within 110 of its facing; sure within 10 (x size),
// 0.7 to 20, less beyond; out of its nav area only 10..25 (0.8).
float aiCYCLOP::CalcBorderForClubAttack()
{
    if (!(aiFlags & 0x10000000) || enemyAngle > 110.0f)
        return -1.0f;
    float r = enemyDist / size;
    if ((aiFlags2 & 8) && !((scnDOMAIN *)navArea)->IsBelongPointXZ(&wayPointPos))
        return (10.0f < r && r < 25.0f) ? 0.8f : -1.0f;
    if (r < 10.0f)
        return 1.0f;
    if (r > 20.0f)
        return 0.7f - (r - 20.0f) * 0.175f;
    return 0.7f;
}

// v12 0x00443C40  (feb 0x0043C970)
// The chance of a spit: none dead / unseen / more than 60 off; sure for a far one dead ahead on its trail;
// growing from 8 to 20, 0.2 to 140, less beyond.
float aiCYCLOP::CalcBorderForSpitAttack()
{
    if ((int)aiFlags < 0)
        return 0.0f;
    if (!(aiFlags & 0x10000000) || enemyAngle > 60.0f)
        return -1.0f;
    float r = enemyDist / size;
    if ((aiFlags & 1) && (aiFlags & 2) && enemyAngle < 10.0f && r > 25.0f)
        return 1.0f;
    if (r < 20.0f)
        return (r - 8.0f) * 0.016666668f;
    if (r > 140.0f)
        return 0.2f - (r - 140.0f) * 0.0071428572f;
    return 0.2f;
}

// v12 0x00443D20  (feb 0x0043CA50)
// Free and facing the enemy (within 30): what next. The boss's SHOT holds its fallbacks 10 s.
void aiCYCLOP::Think()
{
    aiNPC_WR::Think();
    if (!(aiFlags & 0x1000) && attackTimer <= 0.01f) {
        cycFlags &= ~0x320u;
        if (enemyAngle <= 30.0f && !(cycFlags & 0x8000) && (!(cycFlags & 0x10000000) || !(wrFlags & 0x200))) {
            clubBorder = CalcBorderForClubAttack();
            spitBorder = CalcBorderForSpitAttack();
            if (enemyDist < closeDist) {
                cycFlags |= 0x20;
            } else {
                if (m3dRandom() < clubBorder) {
                    if (!(cycFlags & 0x200))
                        entFlags |= 0x100;
                    cycFlags |= 0x200;
                }
                if (m3dRandom() < spitBorder) {
                    if (!(cycFlags & 0x100))
                        entFlags |= 0x100;
                    cycFlags |= 0x100;
                }
            }
        }
    }
    if (inst->seq == 10 && !(cycFlags & 0x10000000))
        shootFallbackTimer = 10.0f;
    UpdateInAreaFlags();
}

// v12 0x00443E70
void aiCYCLOP::PostThink()
{
    aiNPC_WR::PostThink();
    if (WR_FIELD(u32, inst, 8) & 0x100)
        cycFlags &= ~0x10u;
}

// v12 0x00443EA0  (feb 0x0043CB70)
// Out of its nav area (2 s, not in a club): back to its centre for 30 goals.
int aiCYCLOP::SelectGoal()
{
    aiNPC::SelectGoal();
    m3dV p;
    GetPosRooted(&p);
    if (!(aiFlags2 & 8))
        return 1;
    if (cycFlags & 0x8000) {
        aiAttackPlanner->GetNavAreaCenter(this, &goalPos);
        wayPointPos = goalPos;
    }
    if (wrFlags & 0x10000) {
        if (--returnCount <= 0) {
            returnCount = 0;
            cycFlags &= ~0x8000u;
        }
        return 1;
    }
    if (outsideTime > 2.0f && !(cycFlags & 0x8000)) {
        m3dV q;
        GetPosRooted(&q);
        if ((aiFlags2 & 8) && !((scnDOMAIN *)navArea)->IsBelongPointXZ(&q)) {
            returnCount = 30;
            cycFlags |= 0x8000;
        }
    }
    return 1;
}

// v12 0x00443FC0  (feb 0x0043CC90)
// Turns only walking or clubbing, quicker when the enemy is well off to the side.
void aiCYCLOP::SetMovementSpeed()
{
    int s = inst->seq;
    if (s != 1) {
        aiFlags |= 0x200;
        if (s != 2) {
            turnSpeed = 0;
            return;
        }
    }
    if (!IsPivotUsable()) {
        turnSpeed = 0;
        return;
    }
    m3dV dir, t, p;
    inst->GetDir(&dir);
    GetPlayerTargetPos(&t);
    inst->GetPos(&p);
    dir.y = 0;
    m3dV d = { t.x - p.x, 0, t.z - p.z };
    turnSpeed = m3dAngleVector_COS(&d, &dir) > 0.2f ? 90.0f : 108.00001f;
}

// v12 0x004440B0  (feb 0x0043CD80, shared with aiGEFEST_GENERIC)
// The goal 20 ahead, turned at most maxTurnStep (beyond 120: straight ahead); a goal nearer than this frame's step:
// 20 ahead; then the step clamped.
void aiCYCLOP::ClampGoalFrame(float speed, m3dV *dir, m3dV *goal)
{
    float side = minTurnRadius;
    GetClampingSpeeds(speed, &speed, &side);
    m3dV p, f;
    inst->GetPos(&p);
    inst->GetDir(&f);
    float a = m3xzAngleVector(&f, dir);
    float aa = fabsf(a);
    if (aa > maxTurnStep) {
        if (aa <= 120.0f) {
            float lo = -maxTurnStep, hi = maxTurnStep;
            if (hi < lo) {
                float t = lo;
                lo = hi;
                hi = t;
            }
            float r = a < lo ? lo : (a > hi ? hi : a);
            m3dMATR m;
            m.MakeRotY(r);
            float *mm = (float *)&m;
            m3dV v = { f.x * mm[0] + f.y * mm[4] + f.z * mm[8], f.x * mm[1] + f.y * mm[5] + f.z * mm[9],
                       f.x * mm[2] + f.y * mm[6] + f.z * mm[10] };
            goal->x = v.x * 20.0f + p.x;
            goal->y = v.y * 20.0f + p.y;
            goal->z = v.z * 20.0f + p.z;
        } else {
            goal->x = f.x * 20.0f + p.x;
            goal->y = f.y * 20.0f + p.y;
            goal->z = f.z * 20.0f + p.z;
        }
    }
    float dx = p.x - goal->x, dz = p.z - goal->z;
    if (sqrtf(dx * dx + dz * dz) < gsElapsedTime * speed) {
        goal->x = f.x * 20.0f + p.x;
        goal->y = f.y * 20.0f + p.y;
        goal->z = f.z * 20.0f + p.z;
    }
    m3dClampVector_L(&p, goal, gsElapsedTime * speed);
}

// v12 0x00444430  (feb 0x0043D110)
void aiCYCLOP::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&attackTimer, gsElapsedTime);
    if (m3dUpdateTimeField(&spitTimer, gsElapsedTime))
        cycFlags |= 0x4000;                             // the next stone
    if (wrFlags & 0x10000)
        outsideTime = 0;
    else
        outsideTime = gsElapsedTime + outsideTime;
}

// v12 0x004444A0  (feb 0x0043D180)
// A big gun (6, 9, 10) makes it flinch; caged / appearing it takes nothing.
int aiCYCLOP::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (entFlags & 4) {
        if (d->type == 1) {
            int w = ((dmgWEAPON *)d)->weaponType;
            if (w == 6 || (w > 8 && w <= 10))
                cycFlags |= 8;
        }
        aiNPC_WR::ProcessCDT_FIREABLE(d);
        if (cycFlags & 3)
            health = maxHealth;
    }
    return 1;
}

// v12 0x00444500
void aiCYCLOP::FillSoundFlags()
{
    if ((inst->flags & 0x4000) && inst->seq == 5)
        unk108 |= 2;
}

// v12 0x00444530  (feb 0x0043D200)
// One sound a frame (the first of: death, cage, club x2, hit, spit, roar, grab missed, grab, then the steps, played
// where it stands); a mini's own list.
void aiCYCLOP::PlaySounds()
{
    m3dV p;
    inst->GetPos(&p);
    sncSOUND_DESCR_3D *l = (cycFlags & 0x10000000) ? miniSnd : cycSnd;
    static const struct { u32 bit; int snd; } t[] = {
        { 2, CSND_DEATH }, { 4, CSND_CAGE }, { 0x10, CSND_HIT_GROUND }, { 0x20, CSND_HIT_GROUND_2 }, { 0x400, CSND_HIT },
        { 0x200, CSND_SPIT }, { 0x40, CSND_ROAR }, { 1, CSND_EAT }, { 8, CSND_GET_STONES } };
    u32 f = unk108;
    for (unsigned i = 0; i < sizeof(t) / sizeof(t[0]); i++)
        if (f & t[i].bit) {
            voice->Play(&l[t[i].snd], 0);
            aiNPC::PlaySounds();
            return;
        }
    if (f & 0x80)
        gsSysSound->Play(&l[CSND_STEP_1], &p);
    else if (f & 0x100)
        gsSysSound->Play(&l[CSND_STEP_2], &p);
    aiNPC::PlaySounds();
}

// v12 0x00444790  (feb 0x0043D3A0)
// -100: reset (out of the cage sizes); 0xB / 0xC: after the spit grab or walk.
void aiCYCLOP::ProcessMP_NOTIFY(gsEVENT *ev)
{
    aiNPC_WR::ProcessMP_NOTIFY(ev);
    int t = WR_FIELD(short, ev, 4);
    if (t == -100) {
        u32 f = cycFlags & ~0x320u;
        cycFlags = f;
        aiFlags &= ~0x1000u;
        if ((aiFlags2 & 0x40000) && !(f & 3)) {
            heightCDT = 8.5f;
            radCDT = 5.0f;
            heightLowTester = 5.0f;
            heightUpTester = 3.0f;
            heightUp1 = 9.0f;
        }
    } else if (t == 0xB) {
        inst->tpl->aniSystem->SetTranParam(7, 1, 1, 6, 0);
    } else if (t == 0xC) {
        inst->tpl->aniSystem->SetTranParam(7, 1, 1, 1, 1);
    }
}

// v12 0x00444870  (feb 0x0043D420) the club and the skull take no hits
int aiCYCLOP::IsArmoredObject(char *name)
{
    return !strnicmp(name, "dubina", 6) || !strnicmp(name, "cherep", 6);
}

// v12 0x004448B0  (feb 0x0043D460)
int aiCYCLOP::IsAnimSeqGoodForFallback()
{
    int s = EvalAnimGoalSeq();
    return s != 5 && s != 0;
}

// v12 0x00444938 (aiCYCLOP_REFINE::IsInclObj): of its cage only the floor
class aiCYCLOP_REFINE : public aiCDT_REFINE {
public:
    BOOL32 IsInclObj(objOBJ *o)
    {
        animINST *i = o->inst;
        const char *on = o->name;
        if (i) {
            const char *n = i->name;
            if (!strcmp(n ? n : "", "Actor_cage_cyclop") && on && !strstr(on, "CyclopFloor"))
                return 0;
        }
        return aiCDT_REFINE::IsInclObj(o);
    }
};

// v12 0x004448D0  (feb 0x0043D480)
cdtREFINE *aiCYCLOP::GetFloorPlaceRefiner()
{
    static aiCYCLOP_REFINE ref;
    aiCDT_REFINE *b = (aiCDT_REFINE *)aiNPC::GetFloorPlaceRefiner();
    ref.mode = b->mode;                                 // (aiCDT_REFINE::Init v12 0x0054F0C0)
    ref.exclude = b->exclude;
    ref.cls = b->cls;
    return &ref;
}

// v12 0x004449A0
// The coliseum's cut-scene over: the boss put back on its feet (not below 0.6), its history there.
BOOL32 aiCYCLOP::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x4CF && (aiFlags2 & 0x40000) && !(aiFlags & 0x10000) && strstr(gsSceneFullName, "2_5_coliseum")) {
        m3dV p;
        GetPosRooted(&p);
        if (!(0.6f < p.y))
            p.y = 0.6f;
        SetPosRooted(&p);
        lastPos[0] = p.x;
        lastPos[1] = p.y;
        lastPos[2] = p.z;
        prevPos[0] = p;
        prevPos[1] = p;
    }
    return aiNPC_WR::ProcessMsg(id, data, from, ret);
}

// v12 0x004408F0
// As the satyr's arrows: at the meeting point 40 ahead (1.5 up), or at the enemy; more than aimAngle off: straight
// ahead. 40 m/s across, gravity 9.
void aiCYCLOP::GetStonePhysParam(m3dV *from, pjlPJL_INIT *init)
{
    m3dV dir, v, at;
    inst->GetDir(&dir);
    dir.y = 0;
    if (!(wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(this, 40.0f, &at)) {
        at.y += 1.5f;
        v.x = at.x - from->x;
        v.y = 0;
        v.z = at.z - from->z;
        m3dNormalize(&v);
        if (fabsf(m3xzAngleVector(&v, &dir)) > aimAngle)
            v = dir;
    } else {
        enemy.GetPos(&at);
        v = dir;
    }
    v.x *= 40.0f;
    v.y *= 40.0f;
    v.z *= 40.0f;
    float dx = from->x - at.x, dz = from->z - at.z;
    float t = sqrtf(dx * dx + dz * dz) * 0.025f;
    float dy = at.y - from->y;
    init->phys.vel[0] = v.x;
    init->phys.vel[1] = (dy - t * t * -4.5f) / t;
    init->phys.vel[2] = v.z;
    init->phys.accel[0] = m3dVUnitY.x * -9.0f;
    init->phys.accel[1] = m3dVUnitY.y * -9.0f;
    init->phys.accel[2] = m3dVUnitY.z * -9.0f;
}

static void stoneInitDefaults(pjlSTONE_INIT *in)
{
    memset(in, 0, sizeof(*in));
    in->phys.packId = 0;
    in->phys.tag = 0x40;
    in->phys.size = 0x5D;
    in->phys.radius = 0.1f;
    in->phys.lifetime = 2000000.0f;
    in->phys.maxHits = 1;
    in->damage = -1.0f;
    in->radius = 0;
    in->weaponType = -1;
    in->isGeneral = -1;
    in->ownerId = 0xFFFFFFFF;
    in->termDelay = -1.0f;
    in->explosive = 1;
    in->f_51 = 0;
    in->mini = 1;
    in->type = -1;
}

static int stoneType(aiCYCLOP *c)
{
    float r = m3dRandom();
    int t = r < 0.33f ? 0 : (r < 0.66 ? 1 : 2);
    if (c->cycFlags & 0x10000000)
        t += 3;
    return t;
}

// v12 0x00440B30
// A mini's spit: one stone from its mouth (AttackFocus), every 0.05..0.3 s while SPIT plays.
void aiCYCLOP::LaunchOneStone()
{
    pjlSTONE_INIT in;
    stoneInitDefaults(&in);
    animCREATE_DATA cd;
    inst->Validate(4);
    m3dV pos;
    attackFocus->GetOrigin(&pos);
    memcpy(&cd.matr, attackFocus->ltm, sizeof(cd.matr));
    cd.scale.x = cd.scale.y = cd.scale.z = 4.0f;
    if (inst->seq != 7) {
        cycFlags &= ~0x6000u;
        return;
    }
    int t = stoneType(this);
    in.phys.radius = stoneSize[t] * 0.8888889f;
    in.phys.lifetime = 1000.0f;
    in.phys.maxHits = 15;
    in.phys.friction = m3dRandMax(0.05f) - 0.025f + 0.5f;
    in.phys.bounce = m3dRandMax(0.05f) - 0.025f + 0.2f;
    in.ownerId = (u32)mpId;
    in.isGeneral = 4;
    in.radius = 3.0f;
    in.type = t;
    in.damage = stoneDamage[t];
    GetStonePhysParam(&pos, &in);
    entENTITY *e = entCreate(gsScenePtr, (char *)"pjl_stone_cyclop", (char *)stoneTpl[t], &cd, &in, 0);
    if (!e)
        return;
    ((physPHYS *)e)->AddEntCDTExclude(this);
    e->ApplyColor(&floor, 0);
    cycFlags &= ~0x4000u;
    spitTimer = m3dRandMax(0.25f) + 0.05f;
}

// v12 0x00440DC0
// The boss's spit: 10..14 stones at once, each its own size and a little off the aim; the next in 2 s.
void aiCYCLOP::LaunchSeveralStones()
{
    pjlSTONE_INIT base, in;
    stoneInitDefaults(&base);
    stoneInitDefaults(&in);
    animCREATE_DATA cd;
    inst->Validate(4);
    m3dV pos;
    attackFocus->GetOrigin(&pos);
    memcpy(&cd.matr, attackFocus->ltm, sizeof(cd.matr));
    cd.scale.x = cd.scale.y = cd.scale.z = 2.0f;
    if (inst->seq != 7) {
        cycFlags &= ~0x6000u;
        return;
    }
    base.mini = 0;
    base.phys.lifetime = 1000.0f;
    base.phys.maxHits = 15;
    base.phys.friction = 0.5f;
    base.phys.bounce = 0.5f;
    base.ownerId = (u32)mpId;
    base.isGeneral = 4;
    base.radius = 3.0f;
    GetStonePhysParam(&pos, &base);
    int n = m3dRandIntRange(10, 14);
    for (int i = 0; i < n; i++) {
        int t = stoneType(this);
        base.phys.radius = stoneSize[t] * 0.44444445f;
        base.damage = stoneDamage[t];
        in = base;
        in.type = t;
        in.phys.vel[0] = m3dRandRange(0.83f, 1.17f) * base.phys.vel[0];
        in.phys.vel[1] = m3dRandRange(0.95f, 1.05f) * base.phys.vel[1];
        in.phys.vel[2] = m3dRandRange(0.83f, 1.17f) * base.phys.vel[2];
        m3dNormalize((m3dV *)in.phys.vel);
        float sp = m3dLength(*(m3dV *)base.phys.vel);
        in.phys.vel[0] *= sp;
        in.phys.vel[1] *= sp;
        in.phys.vel[2] *= sp;
        entENTITY *e = entCreate(gsScenePtr, (char *)"pjl_stone_cyclop", (char *)stoneTpl[t], &cd, &in, 0);
        if (!e)
            return;
        e->ApplyColor(&floor, 0);
    }
    spitTimer = 2.0f;
    cycFlags &= ~0x4000u;
}
