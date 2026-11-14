// physics.cpp -- simple thrown things: physMATH (Will Rock 1.2, v12.exe; feb obj gs:physics.obj)
//
// So far the math object a passive NPC is carried by. Not rebuilt: physPHYS (the entity), physMATH::ProcessCollision
// (v12 0x00509740; a passive NPC does not move in the rat's case: its class has no fallback sequence).

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"

// v12 0x005094B0  (feb 0x004F6E10)
BOOL32 physMATH::ProcessINIT(physINIT *p)
{
    init = *p;
    vel[0] = p->vel[0];
    vel[1] = p->vel[1];
    vel[2] = p->vel[2];
    age = 0;
    unkB0 = 0;
    flags |= 1;
    if (owner)
        owner->inst->flags |= 0x10;
    unkA8 = 0;
    return 1;
}

// v12 0x00509520  (feb 0x004F6EA0)
void physMATH::ProcessTERM()
{
    operator delete(this);
}

// v12 0x00509540  (feb 0x004F6EC0)
void physMATH::ProcessFRAME()
{
    ProcessPhysics();
    age += gsElapsedTime;
    if (init.lifetime < age && init.lifetime > 0.0f) {
        if (owner)
            msgSystem.PostMsg(owner, 2, owner);
        else
            ProcessTERM();
    }
}

// v12 0x005095A0  (feb 0x004F6F20)
void physMATH::ProcessPhysics()
{
    if (flags & 1) {
        flags &= ~1u;
        return;
    }
    if (flags & 0x10)
        return;
    m3dV from, to;
    ((m3dMATR *)matr)->GetOrigin(&from);
    if (IsAtRest()) {
        if (++restFrames > 3) {
            flags |= 0x10;
            return;
        }
    } else {
        restFrames = 0;
    }
    CalcNewPos(&from, &to);
    ProcessCollision(&from, &to);
    ((m3dMATR *)matr)->Translate(&to, 0);
}

// v12 0x00509660  (feb 0x004F6FB0)
void physMATH::CalcNewPos(m3dV *from, m3dV *to)
{
    vel[0] += gsElapsedTime * init.accel[0];
    vel[1] += gsElapsedTime * init.accel[1];
    vel[2] += gsElapsedTime * init.accel[2];
    to->x = gsElapsedTime * vel[0] + from->x;
    to->y = gsElapsedTime * vel[1] + from->y;
    to->z = gsElapsedTime * vel[2] + from->z;
}

// v12 0x005096C0  (feb 0x004F7010)
void physDRIFT_MATH::CalcNewPos(m3dV *from, m3dV *to)
{
    physMATH::CalcNewPos(from, to);
    to->x += drift.x;
    to->y += drift.y;
    to->z += drift.z;
}

// v12 0x0050A2B0  (feb 0x004F7BC0)
void physMATH::ProcessDESTROY_ENT(entENTITY *e)
{
    entENTITY **l = (entENTITY **)unk94;
    for (int i = 0; i < unkA8; i++)
        if (l[i] == e)
            l[i] = 0;
}

// v12 0x00403200  (feb 0x004036B0) the base: a hit means nothing (the shared "return 0")
BOOL32 physMATH::ProcessCDT(cdtINFO *)
{
    return 0;
}

// v12 0x0050A2F0  (feb 0x004F7C00)
void physMATH::AddEntCDTExclude(entENTITY *e)
{
    if (unkA8 < 5 && e->inst) {
        ((entENTITY **)unk94)[unkA8++] = e;
        if (owner)
            owner->flags |= 2;
    }
}

// v12 0x0050A340  (feb 0x004F7C50)
void physMATH::GetPos(m3dV *out)
{
    ((m3dMATR *)matr)->GetOrigin(out);
}

// v12 0x0050A350  (feb 0x004F7C60)
void physMATH::SetPos(m3dV *p)
{
    ((m3dMATR *)matr)->Translate(p, 0);
}

static float physLen(float *v)
{
    return m3dSqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

// v12 0x004547D0  (feb 0x0044C140)
BOOL32 physMATH::IsAtRest()
{
    return fabsf(physLen(vel)) < 0.05f && (init.unk24 < 0 || unkB0 > init.unk24);
}

// v12 0x00454850  (feb 0x0044C250)
BOOL32 physDRIFT_MATH::IsAtRest()
{
    return physMATH::IsAtRest() && fabsf(physLen(&drift.x)) < 1e-6f;
}

// ---------------------------------------------------------------------------------------------- collision

// The collision refine of a moving thing: not the entities it passes through. vtable v12 0x00607A24
class physCDT_REFINE : public entCDT_REFINE_EXCL {
public:
    physMATH *math;                                     // +0x14
    physCDT_REFINE(int m, animINST *excl, u32 c) : entCDT_REFINE_EXCL(m, excl, c) {}
    BOOL32 IsInclInst(animINST *i);                     // v12 0x0050A370 (feb 0x004F7C80)
};

// v12 0x0050A370  (feb 0x004F7C80)
BOOL32 physCDT_REFINE::IsInclInst(animINST *i)
{
    for (int k = 0; k < math->nExcl; k++)
        if (math->excl[k] == i->ent)
            return 0;
    return entCDT_REFINE_EXCL::IsInclInst(i);
}

static inline float physLen3(float x, float y, float z)
{
    return m3dSqrt(x * x + y * y + z * z);
}

// v12 0x00509740  (feb 0x004F7090)
// The step from -> to against the world: a ray, or (PHYSM_SPHERE) a sphere of init.radius -- a slow one is lifted
// 0.2 for the test so it does not catch on the floor it lies on. A miss of the level may still be an instance
// near (PHYSM_CDT_ANIM). On a hit the step and the speed are mirrored in the surface, the part across it scaled by
// init.bounce (under 0.6 it goes) and the part along it by init.friction (a sphere: under 0.2 it goes); the thing is
// put 0.02 off the surface, and its owner (GS_MSG_CDT) or its own ProcessCDT hears of it: a non-zero answer, or
// init.maxHits reached, ends it.
void physMATH::ProcessCollision(m3dV *from, m3dV *to)
{
    physCDT_REFINE ref(init.cdtMode, owner ? owner->inst : 0, init.cdtCls);
    ref.math = this;
    cdtINFO ci;
    memset(&ci, 0, sizeof(ci));
    ci.point[0] = m3dVZero.x;
    ci.point[1] = m3dVZero.y;
    ci.point[2] = m3dVZero.z;
    m3dV dir = { to->x - from->x, to->y - from->y, to->z - from->z };
    float len = physLen3(dir.x, dir.y, dir.z);
    if (!_m3dNormalize(&dir)) {
        ((m3dMATR *)matr)->GetAxisZ(&dir);
        len = 0.1f;
    }
    int hit;
    if (flags & PHYSM_SPHERE) {
        float lift = 0;
        if (fabsf(physLen3(vel[0], vel[1], vel[2])) < 0.05f && (init.maxHits < 0 || nHits < init.maxHits))
            lift = 0.2f;
        from->y += lift;
        to->y += lift;
        hit = gsScenePtr->ConstrSphere(from, to, init.radius, 0x20, &ref, &ci);
        from->y -= lift;
        to->y -= lift;
        ci.point[1] -= lift;
    } else {
        hit = gsScenePtr->ShootRay(from, &dir, len, 0x20, &ref, &ci);
    }
    if (!hit) {
        if (!(flags & PHYSM_CDT_ANIM))
            return;
        scnAOBJ list[10];
        if (gsScenePtr->MakeAnimList(from, to, init.radius, &ref, list, 10) <= 0)
            return;
        ref.mode |= 0xD;
        gsScenePtr->ConstrSphere(from, to, init.radius, 0, &ref, &ci);
        if (!(ci.flags & 2))
            return;
    }
    m3dV n, P = { ci.point[0], ci.point[1], ci.point[2] };
    gsScenePtr->GetCDTFace(&ci, 0, &n);
    m3dV bv, tv;                                        // the new speed: across and along the surface
    if (!(flags & PHYSM_SPHERE)) {
        m3dMirrorPointPlane(&P, &n, to, to);
        m3dV d = { to->x - P.x, to->y - P.y, to->z - P.z };
        float dn = n.y * d.y + n.z * d.z + n.x * d.x;
        m3dV across = { n.x * dn, n.y * dn, n.z * dn };
        m3dV along = { (d.x - across.x) * init.friction, (d.y - across.y) * init.friction,
                       (d.z - across.z) * init.friction };
        m3dV b = { across.x * init.bounce, across.y * init.bounce, across.z * init.bounce };
        if (fabsf(physLen3(b.x, b.y, b.z)) < 0.001f) {
            b = _m3dNormalize(&across) ? across : m3dVUnitY;
            b.x *= 0.001f;
            b.y *= 0.001f;
            b.z *= 0.001f;
        }
        to->x = b.x + along.x + P.x;
        to->y = b.y + along.y + P.y;
        to->z = b.z + along.z + P.z;
        m3dMirrorPointPlane(&m3dVZero, &n, (m3dV *)vel, (m3dV *)vel);
        float vn = n.y * vel[1] + n.z * vel[2] + n.x * vel[0];
        tv.x = (vel[0] - n.x * vn) * init.friction;
        tv.y = (vel[1] - n.y * vn) * init.friction;
        tv.z = (vel[2] - n.z * vn) * init.friction;
        bv.x = n.x * vn * init.bounce;
        bv.y = n.y * vn * init.bounce;
        bv.z = n.z * vn * init.bounce;
    } else {
        m3dMirrorPointPlane(&m3dVZero, &n, (m3dV *)vel, (m3dV *)vel);
        float vn = n.y * vel[1] + n.z * vel[2] + n.x * vel[0];
        tv.x = (vel[0] - n.x * vn) * init.friction;
        tv.y = (vel[1] - n.y * vn) * init.friction;
        tv.z = (vel[2] - n.z * vn) * init.friction;
        if (physLen3(tv.x, tv.y, tv.z) < 0.2f)
            tv = m3dVZero;
        bv.x = n.x * vn * init.bounce;
        bv.y = n.y * vn * init.bounce;
        bv.z = n.z * vn * init.bounce;
    }
    if (physLen3(bv.x, bv.y, bv.z) < 0.6f)
        bv = m3dVZero;
    vel[0] = bv.x + tv.x;
    vel[1] = bv.y + tv.y;
    vel[2] = bv.z + tv.z;
    ci.point[0] += n.x * 0.02f;
    ci.point[1] += n.y * 0.02f;
    ci.point[2] += n.z * 0.02f;
    nHits++;
    ((m3dMATR *)matr)->Translate((m3dV *)ci.point, 0);
    if (init.maxHits <= 0 || nHits < init.maxHits) {
        int r = owner ? msgSystem.SendMsg(owner, 0x3F9, &ci, owner) : ProcessCDT(&ci);
        if (!r) {
            flags |= PHYSM_HIT;
            return;
        }
    }
    if (!owner) {
        ProcessTERM();
        return;
    }
    msgSystem.PostMsg(owner, 2, owner);
    flags |= PHYSM_HIT;
}

// ---------------------------------------------------------------------------------------------- physPHYS
// The entity of a thrown thing ("phys"; gibs and projectiles derive from it): its physMATH moves it.

#pragma pack(push, 1)
struct physSYNC : msgDATA {                             // the sync block, v12 ? (feb 0x00947CB4), size 0x10
    m3dV vel;                                           // +0x04
};
struct physNOTIFY : msgDATA {                           // GS_MSG_MP_NOTIFY 0x40E
    short type;                                         // +0x04 -100: the sync block
    physSYNC *sync;                                     // +0x06
};
#pragma pack(pop)
static physSYNC physSync;
static int physSyncInited;

static void physSyncInit()
{
    if (!(physSyncInited & 1)) {
        physSyncInited |= 1;
        physSync.packId = 0;
        physSync.tag = 0x40;
        physSync.vel = m3dVZero;
        physSync.size = 0x10;
    }
}

// v12 0x005089E0  (feb 0x004F63C0)
// PLACEHOLDER: rgdInit ("rigid", "rigid_explos": phys_rigid.obj, not rebuilt) is not called.
BOOL32 __fastcall physInit()
{
    return entRegisterClass((char *)"phys", 0x50485953, 0x28, physPHYS::Create, 0, 1) != 0;
}

// (inline in physPHYS::Create v12 0x00508A10 and the derived factories; feb 0x004F64F0)
physPHYS::physPHYS()
{
    physMATH *m = new physMATH;
    if (m) {
        m->flags = 0;
        m->init.packId = 0;
        m->init.tag = 0x40;
        m->init.vel[0] = m->init.vel[1] = m->init.vel[2] = 0;
        m->init.accel[0] = m->init.accel[1] = m->init.accel[2] = 0;
        m->init.radius = 0.1f;
        m->init.lifetime = 2000000.0f;                  // v12 0x006079A0
        m->init.maxHits = 1;
        m->init.unk28[0] = m->init.unk28[1] = m->init.unk28[2] = m->init.unk28[3] = 0;
        m->init.size = 0x38;
        m->nExcl = 0;
        ((m3dMATR *)m->matr)->Identity();
        m->owner = this;
    }
    math = m;
}

// v12 ?  (feb 0x004F6620)
physPHYS::~physPHYS()
{
    if (math)
        operator delete(math);
}

// v12 0x00508A10  (feb 0x004F63F0)
entENTITY *__fastcall physPHYS::Create(animINST *)
{
    return new physPHYS;
}

// v12 0x00508C70  (feb 0x004F6650)
BOOL32 physPHYS::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 1:
        ProcessINIT((physINIT *)data);
        break;
    case 2:
        ProcessTERM();
        break;
    case 3:
        ProcessDESTROY_ENT((entENTITY *)data);
        break;
    case 1000:
        ProcessFRAME();
        break;
    case 0x3F9:
        *ret = ProcessCDT((cdtINFO *)data);
        break;
    case 0x3FA:                                         // (1.2) its dead-reckoning speed forgotten
        mpState.vel[0] = m3dVZero.x;
        mpState.vel[1] = m3dVZero.y;
        mpState.vel[2] = m3dVZero.z;
        entFlags |= 0x280;
        break;
    case 0x40E:
        ProcessMP_NOTIFY((msgDATA *)data);
        break;
    }
    return 0;
}

// v12 0x00508D30  (feb 0x004F66D0)
// The math starts from the instance's place with the init data; lit by the floor under it; the clients' dead
// reckoning gets its speed and acceleration (1.2), sent 15 times a second.
BOOL32 physPHYS::ProcessINIT(physINIT *p)
{
    if (math) {
        memcpy(math->matr, inst->matr, sizeof(math->matr));
        if (!math->ProcessINIT(p))
            return 0;
        cdtREFINE ref(2, 0);
        cdtINFO ci;
        memset(&ci, 0, sizeof(ci));
        m3dV pos;
        inst->GetPos(&pos);
        gsScenePtr->FloorPlace(&pos, 2000000.0f, 0, 0, 0, 0, &ref, &ci);
        ApplyColor(&ci, 0);
    }
    entFlags |= 0x30;
    mpState.flags = 9;
    memcpy(mpState.vel, p->vel, 12);
    memcpy(mpState.extra, p->accel, 12);
    memcpy(accel, p->accel, 12);
    SetMPUpdPeriod(0.06666667f);
    gsSysServer->DeclSyncable(this);
    return 1;
}

// v12 0x00508E90  (feb 0x004F6960)
void physPHYS::ProcessTERM()
{
    if (math) {
        math->ProcessTERM();
        math = 0;
    }
}

int __fastcall gsIsInstInLevelBox(scnSCENE *s, animINST *i);   // v12 0x00508EB0 (gs/entity.cpp)

// v12 0x00508F50  (feb 0x004F6980)
// Local: the math moves it (out of the level's box it ends -- 1.2). A remote copy (entFlags 1): the math follows
// the instance. PHYSM_FLOOR_LIGHT: lit by the floor each frame.
void physPHYS::ProcessFRAME()
{
    if (!(entFlags & 1)) {
        if (math) {
            math->ProcessFRAME();
            m3dMATR m = *(m3dMATR *)math->matr;
            inst->Transform(&m, 0);
        }
        if (!gsIsInstInLevelBox(gsScenePtr, inst))
            msgSystem.PostMsg(this, 2, this);
    } else {
        memcpy(math->matr, inst->matr, sizeof(math->matr));
        m3dV o;
        ((m3dMATR *)inst->matr)->GetOrigin(&o);
        memcpy(math->vel, velocity, 12);
        ((m3dMATR *)math->matr)->Translate(&o, 0);
    }
    if (math && (math->flags & PHYSM_FLOOR_LIGHT)) {
        cdtREFINE ref(2, 0);
        cdtINFO ci;
        memset(&ci, 0, sizeof(ci));
        m3dV pos;
        inst->GetPos(&pos);
        gsScenePtr->FloorPlace(&pos, 2000000.0f, 0, 0, 0, 0, &ref, &ci);
        ApplyColor(&ci, 1);
    }
}

// v12 0x00509170  (feb 0x004F6AF0)
void physPHYS::ProcessDESTROY_ENT(entENTITY *e)
{
    if (math)
        math->ProcessDESTROY_ENT(e);
}

// v12 0x00509190  (feb 0x004F6B10)
BOOL32 physPHYS::ProcessCDT(cdtINFO *ci)
{
    if (math)
        ApplyColor(ci, 0);
    return math ? math->ProcessCDT(ci) : 0;
}

// v12 0x005091D0  (feb 0x004F6B50)
void physPHYS::AddEntCDTExclude(entENTITY *e)
{
    if (math && e)
        math->AddEntCDTExclude(e);
}

// v12 0x00509230  (feb 0x004F6BB0)
void physPHYS::ProcessMP_GET_SYNC_DATA(void *d)
{
    msgDATA **out = (msgDATA **)d;
    if (!*out) {
        physSyncInit();
        *out = &physSync;
    }
    memcpy(&((physSYNC *)*out)->vel, math->vel, 12);
    entENTITY::ProcessMP_GET_SYNC_DATA(d);
}

// v12 0x005092D0  (feb 0x004F6C50)
void physPHYS::ProcessMP_WRITE_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    physSYNC *s = (physSYNC *)d->data;
    d->file->WriteChunk(0xF0, &c);
    d->file->WriteData(&s->vel, 0xC, 4);
    d->file->UpdateChunk(&c);
    d->file->WriteLastChunk();
    entENTITY::ProcessMP_WRITE_SYNC_DATA(d);
}

// v12 0x00509340  (feb 0x004F6CC0)
void physPHYS::ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    physSYNC *s = (physSYNC *)d->data;
    if (!s) {
        physSyncInit();
        s = &physSync;
        d->data = s;
        s->packId = 0;
        s->tag = 0x40;
        s->size = 0x10;
        s->vel = m3dVZero;
    }
    for (int ok = d->file->ReadChunk(&c); ok && c.id != 1; ok = d->file->ReadChunk(&c))
        if (c.id == 0xF0)
            d->file->ReadData(&s->vel, 0xC, 4);
    entENTITY::ProcessMP_READ_SYNC_DATA(d);
}

// v12 0x00509480  (feb 0x004F6DE0)
void physPHYS::ProcessMP_NOTIFY(msgDATA *ev)
{
    physNOTIFY *n = (physNOTIFY *)ev;
    if (n->type == -100)
        memcpy(math->vel, &n->sync->vel, 12);
}
