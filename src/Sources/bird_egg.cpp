// bird_egg.cpp -- the Stymphalian bird's egg ("bird_egg") and its burst ("sfx_egg") (Will Rock 1.2, v12.exe; feb obj
// bird_egg.obj)
//
// The egg: a pjlNPC_PJL projectile (aiBIRD::ThrowEgg sets 10 damage over 4 m) with a glowing trail from its "egg"
// object; it hatches nothing -- where it lands it leaves a scorch mark and "sfx_egg": two bursts of sparks (with
// trails) bouncing on the floor under it, gone in 1.5 s. Its sounds are the bird's 6..8 (fly, explosion, body).
//
// PLACEHOLDER (test): the glow round the egg (a pteSPOT_ANIM following it, radius 3, colour 85/66/33, growing
// 3 -> 6 -> 6 -> 0 over its explosion) -- pteSPOT is not rebuilt; the scorch mark goes through the cyclops' stub.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../m3d/m3d_spl.h"
#include "../../include/sfx_sob/sfx_api.h"
#include "../../include/game_shared/projectile.h"

sncSOUND_DESCR_3D *__fastcall birdGetSound(int i);                  // v12 0x00438CF0 (ai_bird.cpp)
BOOL32 appFloorPlane(m3dV *c, m3dV *plane, float r);                // v12 0x00467B80 (sfx_appear.cpp)
struct wrsfxSCORCHMARK {                                // (not rebuilt: the test prints "scorchmark: not made")
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
                                                        // v12 0x00467DE0 (feb 0x0045DED0)
};

static txmTEXTURE *eggTexScorch;                        // v12 0x0064E9F8 "sfx_egg_scorch_mark"
static txmTEXTURE *eggTexFire;                          // v12 0x0064E9F0 "sfx_egg_fire"
static txmTEXTURE *eggTexGlow;                          // v12 0x0064E9EC "sfx_egg_glow"

static m3dSPL_LINEAR1D *eggSpl(int n)
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

// ---- the emitters ---------------------------------------------------------------------------------------------

class birdEGG_TRAIL : public partEMITTER_PHYS { public: birdEGG_TRAIL(); };
class birdEGG_SPARKS_1 : public partEMITTER_PHYS { public: birdEGG_SPARKS_1(); };
class birdEGG_SPARKS_1_TRAIL : public partEMITTER_PHYS { public: birdEGG_SPARKS_1_TRAIL(); };
class birdEGG_SPARKS_2 : public partEMITTER_PHYS { public: birdEGG_SPARKS_2(); };
class birdEGG_SPARKS_2_TRAIL : public partEMITTER_PHYS { public: birdEGG_SPARKS_2_TRAIL(); };

// v12 0x00463400  (feb 0x00459CD0)
// The egg's glow trail: 100 a second, 0.45 (+-0.15) s each, 0.75 -> nothing, turning, half the egg's speed, slowed.
birdEGG_TRAIL::birdEGG_TRAIL() : partEMITTER_PHYS(100.0f, 0.6f)
{
    flags |= 0x200;
    SetTimeLifePart(0.45f, 0.15f);
    m3dSPL_LINEAR1D *sz = eggSpl(2);
    sz->SetKp(0, 0.0f, 0.75f);
    sz->SetKp(1, 1.0f, 0.001f);
    SetSize(0.0f, 0.0f, sz, 1.0f);
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 0.0f);
    flags |= 4;
    speedA = 0;
    speedARand = 0;
    speedASpl = 0;
    speedK = 0.5f;
    speedKRand = 0;
    drag = 3.0f;
    flags |= 0x2000;
    SetTexture(eggTexGlow);
    flags |= 8;
}

// v12 0x00463540  (feb 0x00459DF0)
// 25 sparks at once (in the first 1/24 s of 2.3), 1.25 (+-0.75) s each, 0.3 big, falling, bouncing on the floor.
birdEGG_SPARKS_1::birdEGG_SPARKS_1() : partEMITTER_PHYS(25, 100.0f, 2.0f)
{
    m3dSPL_LINEAR1D *in = eggSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.041666668f, 100.0f);
    in->SetKp(2, 0.41666666f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(2.3f, 0.0f);
    flags |= 0x200;
    SetTimeLifePart(1.25f, 0.75f);
    SetSize(0.3f, 0.0f, 0, 1.0f);
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 0.0f);
    flags |= 4;
    gravity.x = 0;
    gravity.y = -9.8f;
    gravity.z = 0;
    speedA = 3.5f;
    speedARand = 2.5f;
    speedASpl = 0;
    speedB = 1.05f;
    speedBRand = 0.75f;
    speedBSpl = 0;
    flags |= 0x8000;
    SetTexture(eggTexGlow);
    flags |= 8;
    SetCDTParam(0.1f, 0.0f, 0.0f, -1);
    flags |= 0x30000;
    flags |= 0x2000;
}

// v12 0x004636C0  (feb 0x00459F70)
birdEGG_SPARKS_1_TRAIL::birdEGG_SPARKS_1_TRAIL() : partEMITTER_PHYS(500.0f, 0.4f)
{
    SetTimeLifePart(0.3f, 0.1f);
    SetIntensity(20.0f, 0);
    m3dSPL_LINEAR1D *sz = eggSpl(2);
    sz->SetKp(0, 0.0f, 0.3f);
    sz->SetKp(1, 1.0f, 0.001f);
    SetSize(0.0f, 0.0f, sz, 1.0f);
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 0.0f);
    flags |= 4;
    gravity.x = 0;
    gravity.y = -5.0f;
    gravity.z = 0;
    speedK = 0.5f;
    speedKRand = 0;
    SetTexture(eggTexGlow);
    flags |= 8;
}

// v12 0x004637E0  (feb 0x0045A090)
// 13 slower ones that last 5 (+-2) s, 0.1 big, bouncing (0.05, 0.3 friction) up to 4 times.
birdEGG_SPARKS_2::birdEGG_SPARKS_2() : partEMITTER_PHYS(13, 100.0f, 7.0f)
{
    m3dSPL_LINEAR1D *in = eggSpl(3);
    in->SetKp(0, 0.0f, 0.0f);
    in->SetKp(1, 0.041666668f, 100.0f);
    in->SetKp(2, 0.20833333f, 0.0f);
    SetIntensity(0.0f, in);
    SetAnimEmission(1.0f, 0.0f);
    flags |= 0x200;
    SetTimeLifePart(5.0f, 2.0f);
    SetSize(0.1f, 0.0f, 0, 1.0f);
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 0.0f);
    flags |= 4;
    gravity.x = 0;
    gravity.y = -9.8f;
    gravity.z = 0;
    speedA = 4.5f;
    speedARand = 1.5f;
    speedASpl = 0;
    speedB = 1.35f;
    speedBRand = 1.35f;
    speedBSpl = 0;
    flags |= 0x8000;
    SetTexture(eggTexGlow);
    flags |= 8;
    SetCDTParam(0.05f, 0.3f, 0.1f, 4);
    flags |= 0x10000;
    flags |= 0x400000;
    flags |= 0x2000;
}

// v12 0x00463960  (feb 0x0045A210)
// Its trail rises (5 up), fading from 255 to 139 over its 0.5 (+-0.2) s.
birdEGG_SPARKS_2_TRAIL::birdEGG_SPARKS_2_TRAIL() : partEMITTER_PHYS(200.0f, 0.7f)
{
    SetTimeLifePart(0.5f, 0.2f);
    SetIntensity(15.0f, 0);
    m3dSPL_LINEAR1D *sz = eggSpl(2);
    sz->SetKp(0, 0.0f, 1.0f);
    sz->SetKp(1, 1.0f, 0.001f);
    SetSize(0.0f, 0.0f, sz, 1.0f);
    m3dSPL_LINEAR1D *a = eggSpl(2);
    a->SetKp(0, 0.0f, 255.0f);
    a->SetKp(1, 1.0f, 138.97501f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTextureRotAnim(0.0f, 360.0f, 0.0f, 0.0f);
    flags |= 4;
    gravity.x = 0;
    gravity.y = 5.0f;
    gravity.z = 0;
    spread = 0.1f;
    spreadSpl = 0;
    spreadMin = 0;
    cone = 180.0f;
    coneSpl = 0;
    speedA = 0.5f;
    speedARand = 0;
    speedASpl = 0;
    speedK = 0.3f;
    speedKRand = 0;
    SetTexture(eggTexGlow);
    flags |= 8;
}

// ---- the egg -----------------------------------------------------------------------------------------------------

struct birdEGG_PJL_SFX {                                // 12 bytes, at pjlPJL +0x16F
    partEMITTER_PHYS *emitter;                          // birdEGG_TRAIL
    objOBJ           *eggObj;                           // "egg"
    void             *spot;                             // (pteSPOT_ANIM: PLACEHOLDER, never made)
    birdEGG_PJL_SFX() : emitter(0), eggObj(0), spot(0) {}

    // v12 0x00462B40  (feb 0x00459410)
    // Its animation from the start, the trail from the "egg" object (then the glow round it: PLACEHOLDER).
    int StartSFX(physPHYS *p, pjlPJL_INIT *)
    {
        p->inst->SetAnimSeq(0, 0);
        emitter = new birdEGG_TRAIL;
        if (!emitter)
            return 0;
        eggObj = objFindName(p->inst->root, (char *)"egg");
        if (!eggObj)
            return 0;
        partSetAreaEmissionObj(emitter, eggObj, -1, 0);
        emitter->StartEmission();
        return 1;
    }
    void UpdateSFX(physPHYS *) {}                       // (birdEGG::ProcessFRAME moves the trail itself)
    // v12 0x00447270 (shared) stopped: it goes by itself once its particles are gone
    void StopSFX(physPHYS *)
    {
        if (emitter) {
            emitter->StopEmission();
            emitter = 0;
        }
    }
};

#pragma pack(push, 1)
class birdEGG : public pjlNPC_PJL<birdEGG_PJL_SFX> {
public:
    static entENTITY *__fastcall Create(animINST *i);
    BOOL32 ProcessINIT(physINIT *p);
    void   ProcessFRAME();
    void   ShowCollisSFX(pjlEV_CDT *ev);
    sncSOUND_DESCR_3D *GetSound(int i);
};
#pragma pack(pop)

// v12 0x00463AF0  (feb 0x0045A450)
entENTITY *__fastcall birdEGG::Create(animINST *)
{
    return new birdEGG;
}

// v12 0x00462CD0  (feb 0x004595A0)
// As the satyr's arrow: the flight's sound channel, and an egg born inside a wall (the line from the bird's head or
// chest to it blocked) hits at once.
BOOL32 birdEGG::ProcessINIT(physINIT *p)
{
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
    }
    return 1;
}

// v12 0x00464120 (pjlNPC_PJL<birdEGG_PJL_SFX>::ProcessFRAME)
// Flies (unless a cut-scene holds the action), its animation runs, the trail follows "egg", the sounds; the layer
// state to the clients.
void birdEGG::ProcessFRAME()
{
    if (!((gsSysCinema->state & 1) && gsSysCinema->IsSuspendAction())) {
        physPHYS::ProcessFRAME();
        inst->AdvanceTime(gsElapsedTime, 0, 0);
        if (sfx.emitter)
            partSetAreaEmissionObj(sfx.emitter, sfx.eggObj, -1, 0);
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
}

// v12 0x00462FE0  (feb 0x00459850)
// Its first hit: through an entity nothing (the glow goes); else a scorch mark (1.35, in the light there) and the
// burst, standing on the surface. (The glow's growth and fading: PLACEHOLDER, see the top.)
void birdEGG::ShowCollisSFX(pjlEV_CDT *ev)
{
    animCREATE_DATA cd;
    if (math->nHits > 1)
        return;
    if (ev->kind == 0)
        return;
    wrsfxSCORCHMARK::Add(1.35f, 10.0f, eggTexScorch, &ev->pos, ev->color, 0x5343524D, 0, 0);    // 'SCRM'
    cd.matr._MakeLCS2WCS_VY(&ev->pos, &ev->normal);
    entCreate(gsScenePtr, (char *)"sfx_egg", (char *)"sfx_egg", &cd, 0, 0);
}

// v12 0x004631C0
// 0 flying, 1 the explosion, 2 into a body: the bird's 6..8.
sncSOUND_DESCR_3D *birdEGG::GetSound(int i)
{
    if (i < 0 || i > 2)
        return 0;
    return birdGetSound(i + 6);
}

// ---- the burst -----------------------------------------------------------------------------------------------------

// v12 0x004631E0  (feb 0x00459A50)
// Gone in 1.5 s, played once; the two bursts from "emitter", bouncing on the floor under it (within 3 m).
BOOL32 birdEGG_SFX::ProcessINIT(void *init)
{
    if (!dynDYNAMIC::ProcessINIT(init))
        return 0;
    animMode = 1;
    msgSystem.PostTimeMsg(this, 2, this, 1.5f);
    if (!(emitterObj = objFindName(inst->root, (char *)"emitter")))
        return 0;
    emitterObj->SetStateProcYes(0x1008, 0);
    inst->flags |= 0x10;
    m3dV p, plane[2];
    inst->GetPos(&p);
    appFloorPlane(&p, plane, 3.0f);
    partEMITTER_PHYS *t;
    if (!(sparks1 = new birdEGG_SPARKS_1) || !(t = new birdEGG_SPARKS_1_TRAIL))
        return 0;
    sparks1->AttachEmitter(t, 0, 0);
    sparks1->flags |= 0x40000;
    sparks1->SetCDTPlane(1, 0, (m3dPLANE *)plane);
    if (!partSetAreaEmissionObj(sparks1, emitterObj, -1, 0))
        return 0;
    sparks1->StartEmission();
    if (!(sparks2 = new birdEGG_SPARKS_2) || !(t = new birdEGG_SPARKS_2_TRAIL))
        return 0;
    sparks2->AttachEmitter(t, 0, 0);
    sparks2->flags |= 0x40000;
    sparks2->SetCDTPlane(1, 0, (m3dPLANE *)plane);
    if (!partSetAreaEmissionObj(sparks2, emitterObj, -1, 0))
        return 0;
    sparks2->StartEmission();
    return 1;
}

// v12 0x004641F0  (feb 0x0045A850)
entENTITY *__fastcall birdEGG_SFX::Create(animINST *)
{
    birdEGG_SFX *e = new birdEGG_SFX;
    if (!e)
        return 0;
    e->emitterObj = 0;
    e->sparks1 = 0;
    e->sparks2 = 0;
    return e;
}

// v12 0x00462A90  (feb 0x00459350)
int __fastcall birdEggInit()
{
    if (!entRegisterClass((char *)"bird_egg", 0x42444547, 0x37, birdEGG::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"sfx_egg", 0x42444547, 0x37, birdEGG_SFX::Create, 0, 1))
        return 0;
    if (!(eggTexScorch = txmManager->Add((char *)"sfx_egg_scorch_mark", 0x40003, 1)))
        return 0;
    if (!(eggTexFire = txmManager->Add((char *)"sfx_egg_fire", 0x40003, 1)))
        return 0;
    return (eggTexGlow = txmManager->Add((char *)"sfx_egg_glow", 0x40003, 1)) != 0;
}
