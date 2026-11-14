// ai_cerber.cpp -- the cerberus ("ai_cerber") and its fireball ("pjl_crb_frb") and acid ("pjl_crb_acd") (Will Rock 1.2,
// v12.exe; feb objs ai_cerber.obj, ai_cerber_pjl.obj -- 1.2 rewrote the projectiles on the arrow's pjlNPC_PJL)
//
// It runs (9.5 m/s; half that, turning faster, when the enemy is close behind it), spitting a fireball from one head
// every 3..7 s. Close up (2.4 m, within 30 degrees, the enemy slow) it strikes with its paw (10 damage within 1.9 m, none
// at 2.3); stopped by the planner it fires both heads (one aimed 5 m short, one 2 m long), or the tail's acid. Its run
// can turn into a tail attack (ATTACK_TAIL_RUN, the acid at its second frame). Hits: a light one (most guns) and a
// heavy one (the big guns, dying) play WOUNDED. The fireball flies straight at 30 m/s (3 damage round 3 m); the acid
// falls (gravity 7; 5 damage round 1.5 m). Each blows up in its own effect (sfx_cerber.cpp).

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../../include/ai/ai_world.h"
#include "../../include/game_shared/projectile.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list);  // v12 0x004B1E30
void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int skip);  // v12 0x005130D0
int __fastcall wrsfxCerberInit();                                   // v12 0x0046D590 (sfx_cerber.cpp)

// v12 0x00649EE0.. 11 sounds, 0x68 apart (static initialiser v12 0x0043C4A0)
static sncSOUND_DESCR_3D cerbSnd[11];
enum { CBSND_BEING_HIT, CBSND_TAIL_RUN, CBSND_HEADS, CBSND_TAIL_STAY, CBSND_PAW, CBSND_DEATH, CBSND_RUN, CBSND_ONE_HEAD,
       CBSND_PJL_FLY, CBSND_PJL_HIT_GROUND, CBSND_PJL_HIT_BODY };
static float cerbEps2;                  // v12 0x0064A358 (1e-6 squared, v12 0x0043CD70; unused)
static float cerbPjlEps2;               // v12 0x0064A364 (v12 0x0043E8B0; unused)
static void *cerbCfgFrb, *cerbCfgAcd;   // v12 0x0064A360 / 0x0064A36C the "Debug / Decomp_OFF_AI" reads' caches
static txmTEXTURE *cerbTexBurn;         // v12 0x0064A368 aiCERBER_GENERIC::pTexBurn "sfx_cerber_pjl_burn"
static txmTEXTURE *cerbTexAcid;         // v12 0x0064A35C aiCERBER_GENERIC::pTexAcid "sfx_cerber_pjl_acd"

static void cerbSndSet(sncSOUND_DESCR_3D *d, const char *name, int flags, int delay, float mn, float mx)
{
    d->name.SetName((char *)name);
    d->flags = flags;
    d->volume = 0.0f;
    d->delay = delay;
    d->count = 0;
    d->time = 0;
    d->data = 0;
    d->dataFlags = 0;
    d->minDist = mn;
    d->maxDist = mx;
    d->vtbl = 0;
}

static struct aiCERBER_SND_INIT {
    aiCERBER_SND_INIT()
    {
        cerbSndSet(&cerbSnd[0], "cerber_being_hit", 0x40000, 1000, 25.5f, 70.0f);
        cerbSndSet(&cerbSnd[1], "cerber_attack_tail_run", 0, 0, 25.5f, 70.0f);
        cerbSndSet(&cerbSnd[2], "cerber_attack_heads", 0, 0, 25.5f, 70.0f);
        cerbSndSet(&cerbSnd[3], "cerber_attack_tail_stay", 0, 0, 25.5f, 70.0f);
        cerbSndSet(&cerbSnd[4], "cerber_attack_paw", 0, 0, 15.0f, 25.0f);
        cerbSndSet(&cerbSnd[5], "cerber_death", 0x40000, 20000, 30.0f, 80.0f);
        cerbSndSet(&cerbSnd[6], "gladiator_run", 0x10000, 0, 15.0f, 50.0f);
        cerbSndSet(&cerbSnd[7], "cerber_attack_single_head", 0x40000, 0, 25.5f, 70.0f);
        cerbSndSet(&cerbSnd[8], "cerber_fire_pjl_fly", 0x10002, 0, 4.0f, 20.0f);
        cerbSndSet(&cerbSnd[9], "cerber_fire_pjl_hit_ground", 0x20000, 0, 4.0f * 1.3f, 20.0f * 1.5f);
        cerbSndSet(&cerbSnd[10], "cerber_fire_pjl_hit_body", 0x20000, 0, 4.0f * 1.3f, 20.0f * 1.5f);
        cerbEps2 = 1e-6f * 1e-6f;
        cerbPjlEps2 = 1e-6f * 1e-6f;
    }
} aiCerberSndInit;

// v12 0x0043D300  (feb 0x00436D60)
static sncSOUND_DESCR_3D *__fastcall aiCerberGetSound(int i)
{
    return &cerbSnd[i];
}

static m3dSPL_LINEAR1D *cerbSpl(int n)
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

// ---------------------------------------------------------------------------------------------- the projectiles

// v12 0x0043DE90: the fireball's flame, alpha 229 fading out
class pjlCERBER_BURN : public partEMITTER_PHYS {
public:
    pjlCERBER_BURN(float s);
};
pjlCERBER_BURN::pjlCERBER_BURN(float s) : partEMITTER_PHYS(66.666664f, 0.6f)
{
    SetIntensity(66.666664f, 0);
    SetTimeLifePart(0.55f, 0.05f);
    m3dSPL_LINEAR1D *sz = cerbSpl(2);
    sz->SetKp(0, 0.0f, s * 3.5f);
    sz->SetKp(1, 1.0f, s * 0.0f);
    SetSize(s * 4.025f, s * 1.575f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = cerbSpl(2);
    a->SetKp(0, 0.0f, 229.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(cerbTexBurn);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 120.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spread = s;
    spreadSpl = 0;
    spreadMin = 0;
    speedA = 0;
    speedARand = 0;
    speedASpl = 0;
    speedK = 0.6f;
    speedKRand = 0;
    flags |= 0x200;
    flags |= 0x409;
    flags |= 0x10;
    SetDegNearParams(4.0f, 0.1f);
}

// v12 0x0043E060: the acid's drops
class pjlCERBER_ACID_EMITTER : public partEMITTER_PHYS {
public:
    pjlCERBER_ACID_EMITTER(float s);
};
pjlCERBER_ACID_EMITTER::pjlCERBER_ACID_EMITTER(float s) : partEMITTER_PHYS(25.0f, 0.6f)
{
    SetTimeLifePart(0.45f, 0.15f);
    m3dSPL_LINEAR1D *sz = cerbSpl(2);
    sz->SetKp(0, 0.0f, s * 4.0f);
    sz->SetKp(1, 1.0f, s * 8.0f);
    SetSize(s * 8.0f, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = cerbSpl(2);
    a->SetKp(0, 0.0f, 200.175f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(cerbTexAcid);
    flags |= 0x100000;
    cone = 180.0f;
    coneSpl = 0;
    spreadSpl = 0;
    spread = s * 0.12f;
    spreadMin = 0;
    speedASpl = 0;
    speedK = 0.4f;
    speedKRand = 0;
    speedA = s * 3.3333333f;
    speedARand = s * 0.5f;
    flags |= 0x409;
    flags |= 0x10;
    flags |= 0x200;
}

// v12 0x0043DE40  (feb 0x00437560)
static int __fastcall aiCerberPjlTexturesInit()
{
    if (!(cerbTexBurn = txmManager->Add((char *)"sfx_cerber_pjl_burn", 0x40003, 1)))
        return 0;
    return (cerbTexAcid = txmManager->Add((char *)"sfx_cerber_pjl_acd", 0x40003, 1)) != 0;
}

struct pjlCERBER_FIREBALL_SFX {                         // 8 bytes, at pjlPJL +0x16F
    partEMITTER_PHYS *emitter;
    u32               unused;
    pjlCERBER_FIREBALL_SFX() : emitter(0), unused(0) {}

    // v12 0x0043E200  (feb 0x00437920): its animation from the start, the flame at its place along its speed
    int StartSFX(physPHYS *p, pjlPJL_INIT *)
    {
        p->inst->SetAnimSeq(0, 0);
        emitter = new pjlCERBER_BURN(0.1f);
        if (!emitter)
            return 0;
        animINST *a = p->inst;
        m3dV v = *(m3dV *)p->math->vel, pos;
        p->math->GetPos(&pos);
        emitter->SetPosDirSpeed(&pos, 0, &v);
        a->AdvanceTime(gsElapsedTime, 0, 0);
        emitter->StartEmission();
        return 1;
    }
    // v12 0x0043E2B0  (feb 0x004379D0)
    void UpdateSFX(physPHYS *p)
    {
        animINST *a = p->inst;
        if (emitter) {
            m3dV v = *(m3dV *)p->math->vel, pos;
            p->math->GetPos(&pos);
            emitter->SetPosDirSpeed(&pos, 0, &v);
        }
        a->AdvanceTime(gsElapsedTime, 0, 0);
    }
    // v12 0x00447270 (shared) stopped: it goes by itself once its particles are gone
    void StopSFX(physPHYS *)
    {
        if (emitter) {
            emitter->StopEmission();
            emitter = 0;
        }
    }
};

struct pjlCERBER_ACID_SFX {                             // 8 bytes, at pjlPJL +0x16F
    partEMITTER_PHYS *emitter;
    u32               unused;
    pjlCERBER_ACID_SFX() : emitter(0), unused(0) {}

    // v12 0x0043E820  (feb 0x00438070)
    int StartSFX(physPHYS *p, pjlPJL_INIT *)
    {
        emitter = new pjlCERBER_ACID_EMITTER(0.1f);
        if (!emitter)
            return 0;
        m3dV v = *(m3dV *)p->math->vel, pos;
        p->math->GetPos(&pos);
        emitter->SetPosDirSpeed(&pos, 0, &v);
        emitter->StartEmission();
        return 1;
    }
    // v12 0x0043E8F0  (feb 0x00438120)
    void UpdateSFX(physPHYS *p)
    {
        if (!emitter)
            return;
        m3dV v = *(m3dV *)p->math->vel, pos;
        p->math->GetPos(&pos);
        emitter->SetPosDirSpeed(&pos, 0, &v);
    }
    // v12 0x0043E8D0  (feb 0x00438100) exact: the drops deleted at once
    void StopSFX(physPHYS *)
    {
        if (emitter) {
            emitter->__vecDelDtor(1);
            emitter = 0;
        }
    }
};

#pragma pack(push, 1)
// pjlPJL<SFX>::ProcessINIT with the sound channel (v12 0x0043D780 / 0x0043DB60), shared by the two
template <class SFX> class pjlCERBER_PJL : public pjlNPC_PJL<SFX> {
public:
    u32 layerState;                                     // +0x1BC 1 it has burst, 0x10 (cleared unless the init asks)
    BOOL32 InitBase(physINIT *p)
    {
        if (!physPHYS::ProcessINIT(p))
            return 0;
        pjlPJL_INIT *in = (pjlPJL_INIT *)p;
        this->damage = in->damage;
        this->radius = in->radius;
        this->isGeneral = in->isGeneral;
        this->weaponType = in->weaponType;
        this->termDelay = in->termDelay;
        this->explosive = in->explosive;
        if (this->entFlags & 4) {
            if (in->ownerId != 0xFFFFFFFF) {
                entIACTIVE_OBJ *e = gsSysMP->FindMPID((int)in->ownerId, 4);
                if (e)
                    this->AddEntCDTExclude((entENTITY *)e);
            }
            this->ownerId = in->ownerId;
        }
        this->sfx.StartSFX(this, in);
        if (this->HasSound()) {
            float mn, mx;
            this->GetSndDist(&mn, &mx);
            sncCHANNEL_CONE *c = new sncCHANNEL_CONE;  // (a sncCHANNEL_SPH there)
            if (c) {
                c->minDist = mn;
                c->maxDist = mx;
            }
            this->sndChannel = c;
            if (!this->sndChannel)
                return 0;
            gsSysSound->AddChannel(this->sndChannel, this, 0);
        }
        this->pjlFlags |= 0x12;
        return 1;
    }
    // born inside a wall (the line from the shooter's head or chest to it blocked): hit at once
    void CheckBornInWall(pjlPJL_INIT *in)
    {
        if (!(this->entFlags & 4) || in->ownerId == 0xFFFFFFFF)
            return;
        entENTITY *owner = (entENTITY *)gsSysMP->FindMPID((int)in->ownerId, 4);
        if (!owner || owner->unk98 == 0x14)
            return;
        cdtREFINE ref(2, owner->inst);
        cdtINFO ci;
        memset(&ci, 0, sizeof(ci));
        m3dV op, ap;
        owner->inst->GetPos(&op);
        this->inst->GetPos(&ap);
        m3dV chest = op;
        chest.y = op.y + 0.3f;
        m3dV head = op;
        head.y = ((aiNPC *)owner)->Slot90() + op.y;
        if (!gsScenePtr->IsVisiblePoint(&head, &ap, 1, &ref, &ci) ||
            !gsScenePtr->IsVisiblePoint(&chest, &ap, 1, &ref, &ci)) {
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
    // the layer state to the clients when it changed
    void SendLayer()
    {
        if (this->GetLayerAppState() == this->lastLayerState)
            return;
        pjlEV_LAYER ev;
        ev.packId = 0;
        ev.tag = 0x40;
        ev.state = 0;
        ev.type = 7;
        ev.size = 0xA;
        ev.state = this->GetLayerAppState();
        gsSysServer->SendMsgClientAll(this, 0x40E, &ev, 0x10000, 0xFFFFFFFD, 0xFFFFFFFD);
        this->lastLayerState = ev.state;
    }
    int  GetLayerAppState() { return (int)layerState; }        // v12 0x0043DA40
    void SetLayerAppState(int s) { layerState = (u32)s; }       // v12 0x0043DA50
};

class pjlCERBER_FIREBALL : public pjlCERBER_PJL<pjlCERBER_FIREBALL_SFX> {
public:
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x0043D680
    BOOL32 ProcessINIT(physINIT *p);                    // v12 0x0043E320
    void   AlignWithSpeed();                            // v12 0x0043E560
    void   ProcessFRAME();                              // v12 0x0043E650
    void   ShowCollisSFX(pjlEV_CDT *ev);                // v12 0x0043E780
    sncSOUND_DESCR_3D *GetSound(int i);                 // v12 0x0043E7F0
};
class pjlCERBER_ACID : public pjlCERBER_PJL<pjlCERBER_ACID_SFX> {
public:
    static entENTITY *__fastcall Create(animINST *i);  // v12 0x0043DA60
    BOOL32 ProcessINIT(physINIT *p);                    // v12 0x0043E950
    void   ProcessFRAME();                              // v12 0x0043EB60
    void   ShowCollisSFX(pjlEV_CDT *ev);                // v12 0x0043EC70
    sncSOUND_DESCR_3D *GetSound(int i);                 // v12 0x0043ECE0
};
#pragma pack(pop)

// v12 0x0043D680
entENTITY *__fastcall pjlCERBER_FIREBALL::Create(animINST *)
{
    pjlCERBER_FIREBALL *f = new pjlCERBER_FIREBALL;
    if (!f)
        return 0;
    f->layerState = 0;
    return f;
}

// v12 0x0043E320
// The flame, its sound channel; born inside a wall it bursts at once; pointing along its speed, kept so (physMATH flag
// 2); told to the clients 8 times a second.
BOOL32 pjlCERBER_FIREBALL::ProcessINIT(physINIT *p)
{
    if (!InitBase(p))
        return 0;
    pjlPJL_INIT *in = (pjlPJL_INIT *)p;
    CheckBornInWall(in);
    if (!*((u8 *)in + 0x51))                            // (the launch's flag dword after the init)
        layerState &= ~0x10u;
    math->flags |= 2;
    AlignWithSpeed();
    SetMPUpdPeriod(0.125f);
    return 1;
}

// v12 0x0043E560: "rot_center" turned along the speed (its Y), keeping its scale
void pjlCERBER_FIREBALL::AlignWithSpeed()
{
    objOBJ *rc = objFindName(inst->root, (char *)"rot_center");
    if (!rc)
        return;
    m3dV v = *(m3dV *)math->vel, p;
    math->GetPos(&p);
    inst->Validate(4);
    float sx, sy, sz;
    ((m3dMATR *)rc->ltm)->GetScale(&sx, &sy, &sz);
    m3dMATR m, inv;
    m.Identity();
    m._MakeLCS2WCS_VY(&p, &v);
    m.Scale(sx, sy, sz, 1);
    ((m3dMATR *)rc->parent->ltm)->Invert(&inv);
    m3dMultiplyMatr(&m, &inv, (m3dMATR *)rc->local);
    rc->DeclareProcAnim();
    inst->Invalidate(4);
}

// v12 0x0043E650
// ([Debug] Decomp_OFF_AI stops it.) Flies with its flame and animation, its sounds; the layer state to the clients.
void pjlCERBER_FIREBALL::ProcessFRAME()
{
    int off;
    if (_apCfgReadBool(&cerbCfgFrb, (char *)"Debug", (char *)"Decomp_OFF_AI", &off) && off)
        return;
    if (!((gsSysCinema->state & 1) && gsSysCinema->IsSuspendAction())) {
        physPHYS::ProcessFRAME();
        sfx.UpdateSFX(this);
        PlaySounds();
    }
    SendLayer();
}

// v12 0x0043E780: the burst ("sfx_cerber_pjl_expl", 0.2) standing on the surface hit
void pjlCERBER_FIREBALL::ShowCollisSFX(pjlEV_CDT *ev)
{
    animCREATE_DATA cd;
    layerState |= 1;
    cd.matr._MakeLCS2WCS_VY(&ev->pos, &ev->normal);
    float sc = 0.2f;
    entCreate(gsScenePtr, (char *)"sfx_cerber_pjl_expl", (char *)"sfx_cerber_pjl_expl", &cd, &sc, 0);
}

// v12 0x0043E7F0: 0 flying, 1 the level, 2 a body: the cerberus' 8..10; 3 the launch: its single head
sncSOUND_DESCR_3D *pjlCERBER_FIREBALL::GetSound(int i)
{
    if (i < 0)
        return 0;
    if (i <= 2)
        return aiCerberGetSound(i + 8);
    if (i == 3)
        return aiCerberGetSound(7);
    return 0;
}

// v12 0x0043DA60
entENTITY *__fastcall pjlCERBER_ACID::Create(animINST *)
{
    pjlCERBER_ACID *a = new pjlCERBER_ACID;
    if (!a)
        return 0;
    a->layerState = 0;
    return a;
}

// v12 0x0043E950: the drops, the sound channel, born inside a wall; its direction kept (physMATH flag 2)
BOOL32 pjlCERBER_ACID::ProcessINIT(physINIT *p)
{
    if (!InitBase(p))
        return 0;
    CheckBornInWall((pjlPJL_INIT *)p);
    math->flags |= 2;
    return 1;
}

// v12 0x0043EB60: as the fireball's, without an animation
void pjlCERBER_ACID::ProcessFRAME()
{
    int off;
    if (_apCfgReadBool(&cerbCfgAcd, (char *)"Debug", (char *)"Decomp_OFF_AI", &off) && off)
        return;
    if (!((gsSysCinema->state & 1) && gsSysCinema->IsSuspendAction())) {
        physPHYS::ProcessFRAME();
        sfx.UpdateSFX(this);
        PlaySounds();
    }
    SendLayer();
}

// v12 0x0043EC70: the splash ("sfx_cerber_pjl_acid_expl", 0.4, no model) on the surface hit
void pjlCERBER_ACID::ShowCollisSFX(pjlEV_CDT *ev)
{
    animCREATE_DATA cd;
    layerState |= 1;
    cd.matr._MakeLCS2WCS_VY(&ev->pos, &ev->normal);
    float sc = 0.4f;
    entCreate(gsScenePtr, (char *)"sfx_cerber_pjl_acid_expl", (char *)"", &cd, &sc, 0);
}

// v12 0x0043ECE0: 0 flying, 1 the level, 2 a body
sncSOUND_DESCR_3D *pjlCERBER_ACID::GetSound(int i)
{
    if (i < 0 || i > 2)
        return 0;
    return aiCerberGetSound(i + 8);
}

// ---------------------------------------------------------------------------------------------- the cerberus

// v12 0x0043C7E0  (feb 0x004362A0)
int __fastcall aiCerberInit()
{
    if (!entRegisterClass((char *)"ai_cerber", 0x244E5043, 0x32, aiCERBER_GENERIC::Create, (char *)"ai_cerber", 0))
        return 0;
    if (!entRegisterClass((char *)"pjl_crb_frb", 0x46494243, 0x32, pjlCERBER_FIREBALL::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"pjl_crb_acd", 0x46494243, 0x32, pjlCERBER_ACID::Create, 0, 1))
        return 0;
    if (!wrsfxCerberInit())
        return 0;
    if (!sncLinkSndListData(11, cerbSnd))
        return 0;
    return aiCerberPjlTexturesInit() != 0;
}

// v12 0x0043C870  (feb 0x00436340)
entENTITY *__fastcall aiCERBER_GENERIC::Create(animINST *)
{
    aiCERBER_GENERIC *c = new aiCERBER_GENERIC;         // operator new(0x59B), aiNPC ctor
    if (!c)
        return 0;
    c->wrFlags = 0;
    c->bloodObj = 0;
    c->unk98 = 2;
    return c;
}

// v12 0x0043C8C0  (feb 0x00436390): WOUNDED and DEATH from anything, the run and its tail attack change
BOOL32 aiCERBER_GENERIC::SetupAniSystem()
{
    animTPL *t = inst->tpl;
    animSEQ *old = t->seq;
    int nOld = old ? t->nSeq : 0;
    t->nSeq = 8;
    u32 *blk = (u32 *)operator new(0x444);
    animSEQ *sq = 0;
    if (blk) {
        blk[0] = 8;
        sq = (animSEQ *)(blk + 1);
        for (int i = 0; i < 8; i++) {
            memset(&sq[i], 0, sizeof(animSEQ));
            sq[i].dir = 2;
        }
    }
    t->seq = sq;
    static const char *names[8] = { "CERBER_SEQ_IDLE", "CERBER_SEQ_RUN", "CERBER_SEQ_ATTACK_TAIL_RUN",
                                    "CERBER_SEQ_ATTACK_HEADS", "CERBER_SEQ_ATTACK_TAIL_STAY", "CERBER_SEQ_ATTACK_PAW",
                                    "CERBER_SEQ_WOUNDED", "CERBER_SEQ_DEATH" };
    for (int i = 0; i < 8; i++)
        _entAnimSetSeqName(t, i, (char *)names[i]);
    entAnimAssignTplSeq(t, old, nOld);
    animSYSTEM *s = new animSYSTEM(8, 0);
    if (!s) {
        if (t->seq)
            operator delete((u32 *)t->seq - 1);
        return 0;
    }
    t->aniSystem = s;
    s->SetTranParamFromAny(6, 0, 1, 6);
    s->SetTranCycleAnim(6);
    s->SetTranParamFromAny(7, 0, 1, 7);
    s->SetTranParam(1, 2, 1, 2, 0);
    s->SetTranParam(2, 1, 1, 1, 0);
    if (old)
        operator delete((u32 *)old - 1);
    t->seq[7].unk84 |= 1;
    return 1;
}

// v12 0x0043CAA0  (feb 0x00436570)
// 50 hp, 300 m activation, 9.5 m/s, turning 70; spitting within 20 degrees and 50 m; the paw within 30; corpse 8 s
// (fading 6); a sphere voice (30..150); its four attack points; bleeding from "ROOT".
int aiCERBER_GENERIC::ProcessINIT(void *init)
{
    aiNPC_WR::ProcessINIT(init);                        // (its result not looked at)
    radActivate = 300.0f;
    radActivateBack = 300.0f;
    maxHealth = 50.0f;
    navArriveDist = 1.8f;
    alertAngle = 180.0f;
    moveSpeed = 9.51815f;
    turnSpeed = 70.0f;
    pawAngle = 30.0f;
    radCDT = 0.6f;
    unk17B = 0.15f;
    heightLowTester = 0.72f;
    heightCDT = 1.2f;
    f583 = 5.0f;
    shootRange = 50.0f;
    aimAngle = 20.0f;
    fallbackAngle = 100.0f;
    maxTurnStep = 20.0f;
    corpseTime = 8.0f;
    corpseFadeTime = 6.0f;
    health = 50.0f;
    cerbFlags = 0;
    headsPause = 0;
    timer56F = 0;
    shootTimer = 3.0f;
    headShots = 0;
    woundSeq = 6;
    aiFlags2 |= 0x18002;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;          // (a sncCHANNEL_SPH there)
    if (c) {
        c->minDist = 30.0f;
        c->maxDist = 150.0f;
    }
    sndChannel = c;
    if (!sndChannel)
        return 0;
    gsSysSound->AddChannel(sndChannel, this, 0);
    lHead = objFindName(inst->root, (char *)"AttackFocus_LHEAD");
    rHead = objFindName(inst->root, (char *)"AttackFocus_RHEAD");
    tail = objFindName(inst->root, (char *)"AttackFocus_TAIL");
    arm = objFindName(inst->root, (char *)"AttackFocus_ARM");
    if (!lHead || !rHead || !tail || !arm)
        return 0;
    if (!(rootObj = objFindName(inst->root, (char *)"ROOT")))
        return 0;
    bloodObj = rootObj;
    rootObj->state |= 0x200;
    return 1;
}

// v12 0x0043CD90  (feb 0x00436850)
int aiCERBER_GENERIC::EvalAnimGoalSeq()
{
    if (aiFlags & 0x200000)
        return 7;
    if (enemy.IsDead())
        return 0;
    u32 f = aiFlags;
    if (f & 0x10000)
        return 0;
    u32 cf = cerbFlags;
    if (cf & 0x30) {
        cerbFlags = (cf & ~0x30u) | 8;
        woundSeq = (int)(((f & 0x200000) | 0xC00000) >> 21);
        woundCanEnd = (int)((~f >> 21) & 1);
        return woundSeq;
    }
    if (cf & 8)
        return woundSeq;
    if (cf & 0x100)
        return 5;
    if (f & 0x1000) {
        if (f & 0x40000)
            return m3dRandSelect(3, 4);
        return 0;
    }
    if (f & 4)
        return 0;
    if (enemyAngle < aimAngle && !(enemyDist <= 3.4f))
        return m3dRandSelect(2, 1);
    return 1;
}

// v12 0x0043CE90  (feb 0x00436950)
// Running, a head (the right 60%) spits when the timer is out (again in 3..7 s); stopped, the timer is kept under 1 s.
void aiCERBER_GENERIC::Think()
{
    aiNPC_WR::Think();
    if (inst->seq == 1) {
        if (shootTimer <= 0.0f) {
            LaunchOneProjectile(m3dRandom() <= 0.4f ? rHead : lHead, 0.0f, 1);
            shootTimer = m3dRandRange(3.0f, 7.0f);
        }
    } else if (shootTimer < 1.0f)
        shootTimer = m3dRandRange(0.1f, 0.9f);
}

// v12 0x0043CF30 (name ours)
// Not in an attack: told to stand (by the planner), within the aim angle: a heads / tail attack (one shot); else a paw
// strike when the enemy is within 2.4 m and 30 degrees and slow.
void aiCERBER_GENERIC::FillAttackFlags()
{
    int s = inst->seq;
    if (s == 2 || s == 3 || s == 4)
        return;
    u32 f = aiFlags & ~0x1000u;
    aiFlags = f;
    if (f & 0x40000) {
        if ((f & 0x10000000) && !(f & 0x80000008) && enemyAngle <= aimAngle) {
            aiFlags = f | 0x1000;
            headShots = 1;
        }
        return;
    }
    if ((f & 0x10000000) && enemyDist <= 2.4f && enemyAngle <= pawAngle && enemy.GetSpeedScalar() < 1.0f)
        cerbFlags |= 0x100;
    else
        cerbFlags &= ~0x100u;
}

// v12 0x0043D020  (feb 0x00436A00): it stands but in the run
int aiCERBER_GENERIC::SelectGoal()
{
    FillAttackFlags();
    int s = inst->seq;
    if (s != 1 && s != 2)
        aiFlags |= 0x200;
    return aiNPC::SelectGoal();
}

// v12 0x0043D060  (feb 0x00436B10)
void aiCERBER_GENERIC::UpdateTimers()
{
    aiNPC_WR::UpdateTimers();
    m3dUpdateTimeField(&headsPause, gsElapsedTime);
    m3dUpdateTimeField(&timer56F, gsElapsedTime);
    m3dUpdateTimeField(&shootTimer, gsElapsedTime);
}

// v12 0x0043D0A0  (feb 0x00436B50)
// Weapons 0, 1, 3..5, 7, 8: a light hit (not in WOUNDED already); 2, 6, 9, 10: heavy; dying: heavy.
int aiCERBER_GENERIC::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (!(entFlags & 4))
        return 1;
    if (d->type == 1) {
        u32 w = (u32)((dmgWEAPON *)d)->weaponType;
        static const u8 kind[11] = { 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 1 };
        if (w <= 10) {
            if (kind[w] == 0) {
                if (!(cerbFlags & 8))
                    cerbFlags |= 0x10;
            } else
                cerbFlags |= 0x20;
        }
    }
    aiNPC_WR::ProcessCDT_FIREABLE(d);
    if (aiFlags & 0x200000)
        cerbFlags |= 0x20;
    return 1;
}

// v12 0x0043D130  (feb 0x00436BE0): one sound, the first of: death, hit, heads, tail, one head, tail run, paw, run
void aiCERBER_GENERIC::PlaySounds()
{
    u32 f = unk108;
    sncSOUND_DESCR_3D *d = 0;
    if (f & 0x20)
        d = &cerbSnd[CBSND_DEATH];
    else if (f & 1)
        d = &cerbSnd[CBSND_BEING_HIT];
    else if (f & 4)
        d = &cerbSnd[CBSND_HEADS];
    else if (f & 8)
        d = &cerbSnd[CBSND_TAIL_STAY];
    else if (f & 0x80)
        d = &cerbSnd[CBSND_ONE_HEAD];
    else if (f & 2)
        d = &cerbSnd[CBSND_TAIL_RUN];
    else if (f & 0x10)
        d = &cerbSnd[CBSND_PAW];
    else if (f & 0x40)
        d = &cerbSnd[CBSND_RUN];
    if (d)
        sndChannel->Play((sncSOUND_DESCR *)d, 0);
    aiNPC::PlaySounds();
}

// v12 0x0043D230  (feb 0x00436CB0): a sequence's start (the run's step test asks for sequence 0x40: never)
void aiCERBER_GENERIC::FillSoundFlags()
{
    if (inst->IsActionFrame(0x40, 0))
        unk108 |= 0x40;
    if (!(inst->flags & 0x4000))
        return;
    switch (inst->seq) {
    case 2:
        unk108 |= 2;
        break;
    case 3:
        unk108 |= 4;
        break;
    case 4:
        unk108 |= 8;
        break;
    case 5:
        unk108 |= 0x10;
        break;
    case 6:
        unk108 |= 1;
        break;
    case 7:
        unk108 |= 0x20;
        break;
    }
}

// v12 0x0043D310  (feb 0x00436D70)
// WOUNDED's second frame ends it (not dying); ATTACK_HEADS: the left head (5 m short) then the right (2 m long);
// ATTACK_TAIL_STAY and ATTACK_TAIL_RUN's second frame: the acid; ATTACK_PAW: the paw's blow.
void aiCERBER_GENERIC::ProcessActionFrame()
{
    inst->IsActionFrame(6, 0);
    if (inst->IsActionFrame(6, 1) && woundCanEnd)
        cerbFlags &= ~8u;
    if (inst->IsActionFrame(3, 0))
        LaunchOneProjectile(lHead, 5.0f, 0);
    if (inst->IsActionFrame(3, 1)) {
        LaunchOneProjectile(rHead, -2.0f, 0);
        if (--headShots == 0)
            headsPause = 0.1f;
    }
    if (inst->IsActionFrame(4, 0)) {
        LaunchOneProjectile(tail, 0.0f, 0);
        if (--headShots == 0)
            headsPause = 0.1f;
    }
    if (inst->IsActionFrame(2, 1))
        LaunchOneProjectile(tail, 0.0f, 0);
    if (inst->IsActionFrame(5, 0)) {
        dmgGENERAL g;
        m3dV p, t;
        ((m3dMATR *)arm->ltm)->GetOrigin(&p);
        GetPlayerTargetPos(&t);
        float dx = t.x - p.x, dy = t.y - p.y, dz = t.z - p.z;
        float d = m3dSqrt(dz * dz + dy * dy + dx * dx);
        g.pos = p;
        float a;
        if (d < 1.9f)
            a = 10.0f;
        else if (d > 2.3f)
            a = 0.0f;
        else {
            float range = 0.4f;
            a = fabsf(range) < 1e-6f ? 5.0f : 10.0f - (d - 1.9f) * 25.0f;
        }
        g.amount = a;
        if (a > 0.1f && enemy.ent)
            msgSystem.SendMsg(enemy.ent, 0x4C6, &g, this);
    }
}

// v12 0x0043D620  (feb 0x00437080): the enemy within 20 m and behind it (80 degrees): half speed, turning 84
void aiCERBER_GENERIC::SetMovementSpeed()
{
    if (enemyDist < 20.0f && enemyAngle > 80.0f) {
        moveSpeed = 4.759073f;
        turnSpeed = 84.0f;
    } else {
        moveSpeed = 9.51815f;
        turnSpeed = 70.0f;
    }
}

// v12 0x0043ED00  (feb 0x00438680)
// At 30 m/s flat towards where it meets its enemy (within the aim angle of its facing, else ahead; the enemy's own
// place for the height), the target moved `side` m along the line towards it; ahead beyond the shooting range. The
// acid falls (gravity 7) to come down there; the fireball flies straight.
void aiCERBER_GENERIC::AimPjlToPlayer(m3dV *org, physINIT *p, float side, int isTail)
{
    m3dV fw, d, m, s;
    inst->GetDir(&fw);
    fw.y = 0;
    if (!(wrFlags & 0x10) && aiAttackPlanner->GetEnemyMeetPoint(this, 30.0f, &m)) {
        d.x = m.x - org->x;
        d.y = 0;
        d.z = m.z - org->z;
        m3dNormalize(&d);
        if (!(fabsf(m3xzAngleVector(&d, &fw)) <= aimAngle))
            d = fw;
    } else {
        enemy.GetPos(&m);
        d = fw;
    }
    s.x = org->x - m.x;
    s.y = 0;
    s.z = org->z - m.z;
    m3dNormalize(&s);
    m.x += s.x * side;
    m.y += s.y * side;
    m.z += s.z * side;
    float dx = org->x - m.x, dz = org->z - m.z;
    float dist = m3dSqrt(dx * dx + dz * dz);
    if (!(dist <= shootRange))
        d = fw;
    float t = dist * (1.0f / 30.0f);
    d.x *= 30.0f;
    d.y *= 30.0f;
    d.z *= 30.0f;
    float g = isTail ? -7.0f : 0.0f;
    p->vel[0] = d.x;
    p->vel[1] = ((m.y - org->y) - g * t * t * 0.5f) / t;
    p->vel[2] = d.z;
    p->accel[0] = m3dVUnitY.x * g;
    p->accel[1] = m3dVUnitY.y * g;
    p->accel[2] = m3dVUnitY.z * g;
}

// v12 0x0043EFC0  (feb 0x00438940)
// From the attack point's matrix: a 0.6 sphere, 1000 s at most, five hits, bouncing / sliding 0.5; it bursts round
// what it hits: the tail's acid (no model) 5 damage round 1.5 m, a head's fireball ("pjl_cerber") 3 round 3 m. `flag`
// rides in the dword after the init. Lit like its floor.
void aiCERBER_GENERIC::LaunchOneProjectile(objOBJ *o, float side, int flag)
{
    struct { pjlPJL_INIT i; u32 extra; } blk;           // (1.2 sends 0x55 bytes: a dword after the init)
    pjlPJL_INIT &in = blk.i;
    memset(&blk, 0, sizeof(blk));
    in.phys.packId = 0;
    in.phys.tag = 0x40;
    in.phys.size = 0x55;
    in.phys.radius = 0.1f;
    in.phys.lifetime = 2000000.0f;
    in.phys.maxHits = 1;
    in.damage = -1.0f;
    in.weaponType = -1;
    in.isGeneral = -1;
    in.ownerId = 0xFFFFFFFF;
    in.termDelay = -1.0f;
    in.explosive = 1;
    animCREATE_DATA cd;
    inst->Validate(4);
    m3dV org;
    o->GetOrigin(&org);
    memcpy(&cd.matr, (u8 *)o + 0x3C, sizeof(cd.matr));
    in.phys.radius = 0.6f;
    in.phys.lifetime = 1000.0f;
    in.phys.maxHits = 5;
    in.phys.friction = 0.5f;
    in.phys.bounce = 0.5f;
    in.isGeneral = 4;
    in.ownerId = (u32)mpId;
    if (flag)
        blk.extra |= 1;
    int isTail = o == tail;
    AimPjlToPlayer(&org, &in.phys, side, isTail);
    entENTITY *e;
    if (isTail) {
        in.damage = 5.0f;
        in.radius = 1.5f;
        e = entCreate(gsScenePtr, (char *)"pjl_crb_acd", (char *)"", &cd, &blk, 0);
    } else {
        in.damage = 3.0f;
        in.radius = 3.0f;
        e = entCreate(gsScenePtr, (char *)"pjl_crb_frb", (char *)"pjl_cerber", &cd, &blk, 0);
    }
    if (e)
        gsEffApplyLMChar(e, &floor, 0);
}

static_assert(sizeof(aiCERBER_GENERIC) == 0x59B, "aiCERBER_GENERIC");
static_assert(sizeof(pjlCERBER_FIREBALL) == 0x1C0, "pjlCERBER_FIREBALL");
static_assert(sizeof(pjlCERBER_ACID) == 0x1C0, "pjlCERBER_ACID");
