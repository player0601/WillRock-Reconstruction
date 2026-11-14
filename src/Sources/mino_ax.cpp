// mino_ax.cpp -- the minotaur's thrown axe ("mino_ax") (Will Rock 1.2, v12.exe; feb obj mino_ax.obj)
//
// aiMINO::ThrowAx throws it from the axe it holds (SEKIRA), on an arc at the enemy: a pjlNPC_PJL projectile spinning
// round its "rot_center" (720 degrees a second), a smoke trail from "emitter_1". A player within 1.5 of it is hit;
// it explodes where it lands (8 damage over 3.5 m): "sfx_ax_explos" (a sprite sheet played once) and a scorch mark.
// Its sounds are the minotaur's 11..13 (mino_ax_fly, mino_ax_hit, mino_ax_hit_body).

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"
#include "../../include/game_shared/projectile.h"

sncSOUND_DESCR_3D *__fastcall aiMinoGetSound(int i);                 // v12 0x00450390 (ai_mino.cpp)
float __fastcall entNearestPlayerDist(entENTITY *self, float yOff, m3dV *out, entENTITY **who);  // v12 0x00547450
struct wrsfxSCORCHMARK {                                // (not rebuilt: the test prints "scorchmark: not made")
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
                                                        // v12 0x00467DE0 (feb 0x0045DED0)
};

static txmTEXTURE *axTexScorch;                         // v12 0x0064EA00 "sfx_ax_scorch_mark"
static txmTEXTURE *axTexTrail;                          // v12 0x0064E9FC "sfx_ax_trail"
static float axEpsilon2;                                // v12 0x0064EA04 1e-6 squared (static init v12 0x00464C90)
static struct minoAX_STATIC { minoAX_STATIC() { axEpsilon2 = 0.000001f * 0.000001f; } } minoAxStatic;

static m3dSPL_LINEAR1D *axSpl(int n)
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

// ---- the trail ---------------------------------------------------------------------------------------------------

class minoAX_TRAIL_EMITTER : public partEMITTER_PHYS { public: minoAX_TRAIL_EMITTER(float scale); };

// v12 0x00464AB0  (feb 0x0045B1D0)
// The axe's smoke: 50 a second, 2.25 (+-0.25) s each, growing from 0.75 to nothing (x scale), 0.55 big, fading from
// 255, turning 180..-240 degrees, thrown off at 0.2 (x scale) and 0.1 of the axe's speed, any direction, barely slowed.
minoAX_TRAIL_EMITTER::minoAX_TRAIL_EMITTER(float s) : partEMITTER_PHYS(50.0f, 2.5f)
{
    flags |= 0x200;
    SetTimeLifePart(2.25f, 0.25f);
    m3dSPL_LINEAR1D *sz = axSpl(2);
    sz->SetKp(0, 0.0f, s * 0.75f);
    sz->SetKp(1, 1.0f, s * 0.001f);
    SetSize(s * 0.55f, s * 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = axSpl(2);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 1.0f, 0.0f);
    SetColor(0xFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 180.0f, -240.0f, 0.0f);
    flags |= 4;
    speedARand = 0;
    speedA = s * 0.2f;
    speedASpl = 0;
    speedKRand = 0;
    cone = 180.0f;
    coneSpl = 0;
    drag = 0.005f;
    speedK = s * 0.1f;
    flags |= 0x10;
    SetTexture(axTexTrail);
    flags |= 8;
    SetDegNearParams(4.0f, 0.1f);
}

struct minoAX_TRAIL {                                   // 8 bytes, at pjlPJL +0x16F
    partEMITTER_PHYS *emitter;                          // minoAX_TRAIL_EMITTER
    objOBJ           *emitterObj;                       // "emitter_1"
    minoAX_TRAIL() : emitter(0), emitterObj(0) {}

    // where the smoke comes from: "emitter_1", else the axe's position
    static void From(physPHYS *p, objOBJ *o, m3dV *pos)
    {
        if (o)
            o->GetOrigin(pos);
        else
            p->inst->GetPos(pos);
    }
    // v12 0x00464330  (feb 0x0045AA50)
    int StartSFX(physPHYS *p, pjlPJL_INIT *)
    {
        emitterObj = objFindName(p->inst->root, (char *)"emitter_1");
        emitter = new minoAX_TRAIL_EMITTER(1.0f);
        if (!emitter)
            return 0;
        p->inst->Validate(4);
        m3dV pos;
        From(p, emitterObj, &pos);
        emitter->SetPosDirSpeed(&pos, 0, (m3dV *)p->velocity);
        emitter->StartEmission();
        return 1;
    }
    // v12 0x004643E0  (feb 0x0045AB00)
    void UpdateSFX(physPHYS *p)
    {
        if (!emitter)
            return;
        p->inst->Validate(4);
        m3dV pos;
        From(p, emitterObj, &pos);
        emitter->SetPosDirSpeed(&pos, 0, (m3dV *)p->velocity);
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

// ---- the axe -----------------------------------------------------------------------------------------------------

#pragma pack(push, 1)
class minoAX : public pjlNPC_PJL<minoAX_TRAIL> {
public:
    static entENTITY *__fastcall Create(animINST *i);
    BOOL32 ProcessINIT(physINIT *p);
    void   ProcessFRAME();
    void   ShowCollisSFX(pjlEV_CDT *ev);
    sncSOUND_DESCR_3D *GetSound(int i);
};
#pragma pack(pop)

// v12 0x00464CB0  (feb 0x0045B3D0)
entENTITY *__fastcall minoAX::Create(animINST *)
{
    return new minoAX;
}

// v12 0x00464440  (feb 0x0045AB60)
// Lives up to 1000 s and 10 hits, no bounce; then as the egg: the flight's sound channel, and an axe born inside a
// wall (the line from the thrower's chest or head to it blocked) hits at once.
BOOL32 minoAX::ProcessINIT(physINIT *p)
{
    p->lifetime = 1000.0f;
    p->friction = 0;
    p->bounce = 0;
    p->maxHits = 10;
    if (!physPHYS::ProcessINIT(p))
        return 0;
    pjlPJL_INIT *in = (pjlPJL_INIT *)p;
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
    if ((entFlags & 4) && in->ownerId != 0xFFFFFFFF) {
        entENTITY *owner = (entENTITY *)gsSysMP->FindMPID((int)in->ownerId, 4);
        if (owner && owner->unk98 != 0x14) {
            cdtREFINE ref(2, owner->inst);
            cdtINFO ci;
            memset(&ci, 0, sizeof(ci));
            m3dV op, ap;
            owner->inst->GetPos(&op);
            inst->GetPos(&ap);
            m3dV chest = op;
            chest.y = op.y + 0.3f;
            m3dV head = op;
            head.y = ((aiNPC *)owner)->Slot90() + op.y;
            if (!gsScenePtr->IsVisiblePoint(&chest, &ap, 1, &ref, &ci) ||
                !gsScenePtr->IsVisiblePoint(&head, &ap, 1, &ref, &ci)) {
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
    return 1;
}

// v12 0x00464760  (feb 0x0045AE60)
// Spins round "rot_center"; flies (unless a cut-scene holds the action) with its smoke and sounds; the layer state
// to the clients. On the server, in flight: a player within 1.5 of it is hit and it stops there.
void minoAX::ProcessFRAME()
{
    objOBJ *rc = objFindName(inst->root, (char *)"rot_center");
    ((m3dMATR *)rc->local)->RotateZ(gsElapsedTime * -720.0f, 1);
    inst->Invalidate(4);
    cdtINFO ci;
    memset(&ci, 0, sizeof(ci));
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
    if ((entFlags & 4) && !(pjlFlags & PJLF_HIT)) {
        m3dV pos, at;
        entENTITY *who = 0;
        inst->GetPos(&pos);
        if (entNearestPlayerDist(this, 1.5f, &at, &who) < 1.5f) {
            ci.flags |= 2;
            math->nHits++;
            *(m3dV *)ci.point = pos;
            ci.inst = who ? who->inst : 0;
            msgSystem.SendMsg(this, 0x3F9, &ci, this);
            if (math)
                math->vel[0] = math->vel[1] = math->vel[2] = 0;
            Terminate();
        }
    }
}

// v12 0x00464980  (feb 0x0045B080)
// Where it hits: "sfx_ax_explos" standing on the surface, its sprite sheet ("&bb_&ds_sprite_&nf", 4 x 8 frames over
// 1 s) played once and gone after its sequence; a scorch mark (0.75, in the light there).
void minoAX::ShowCollisSFX(pjlEV_CDT *ev)
{
    animCREATE_DATA cd;
    cd.matr._MakeLCS2WCS_VY(&ev->pos, &ev->normal);
    entENTITY *e = entCreate(gsScenePtr, (char *)"dynamic", (char *)"sfx_ax_explos", &cd, 0, 0);
    if (!e)
        return;
    objOBJ *o = objFindName(e->inst->root, (char *)"&bb_&ds_sprite_&nf");
    if (!o)
        return;
    ((dynDYNAMIC *)e)->StartAnim(0, 0, 0);
    msgSystem.PostTimeMsg(e, 2, 0, e->inst->GetSeqLength(0) + 0.1f);
    e->inst->flags |= 0x10;
    wrsfxSCORCHMARK::Add(0.75f, 10.0f, axTexScorch, &ev->pos, ev->color, 0x5343524D, 0, 0);     // 'SCRM'
    objMOD_TEX_FRAME_ANIM *m = new objMOD_TEX_FRAME_ANIM(0);
    m->SetParams(4, 8, 1.0f, 0, -1, -1.0f, -1.0f, 0);
    m->AddObj(o);
}

// v12 0x00464960
// 0 flying, 1 into the level, 2 into a body: the minotaur's 11..13.
sncSOUND_DESCR_3D *minoAX::GetSound(int i)
{
    if (i < 0 || i > 2)
        return 0;
    return aiMinoGetSound(i + 11);
}

// v12 0x004642C0  (feb 0x0045A9D0)
int __fastcall minoAxInit()
{
    if (!entRegisterClass((char *)"mino_ax", 0x4D4E4158, 0x37, minoAX::Create, 0, 1))
        return 0;
    if (!(axTexScorch = txmManager->Add((char *)"sfx_ax_scorch_mark", 0x40003, 1)))
        return 0;
    return (axTexTrail = txmManager->Add((char *)"sfx_ax_trail", 0x40003, 1)) != 0;
}
