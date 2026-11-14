// dyn_destroy.cpp -- things that break (Will Rock 1.2, v12.exe; feb obj gs_shared:dyn_destroy.obj)
//
// A template with "geom_orig" (whole), "geom_debris" (the pieces) and maybe "geom_debris_left" (what stays). Damage up
// to [DESTROY] damage breaks it: the whole goes, what is left shows, and the debris either plays the template's
// animation (then waits 5 s and fades in 1 s) or, without one, every piece becomes a rigid body flying off
// ("nch_rigid"); with no debris at all the model shatters. The server tells the clients.
// Not rebuilt here: dstrSHATTER_RIGID_START (CreatePiece / GetShatter, with the rigid bodies and shatter.obj).

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"

txmTEXTURE *dstrTexOblom;                               // v12 ? "stat_oblom"

#pragma pack(push, 1)
struct dstrSYNC : msgDATA {                             // the sync block, v12 0x009AE5B8, size 8
    u32 flags;                                          // +0x04
};
struct dstrNOTIFY : msgDATA {                           // GS_MSG_MP_NOTIFY 0x40E: 0 broken, -100 the sync block
    short type;                                         // +0x04
    dstrSYNC *sync;                                     // +0x06 (type -100)
};
#pragma pack(pop)
static dstrSYNC dstrSync;
static int dstrSyncInited;                              // v12 0x009AE5DC bit 0

static void dstrSyncInit()
{
    if (!(dstrSyncInited & 1)) {
        dstrSyncInited |= 1;
        dstrSync.packId = 0;
        dstrSync.tag = 0x40;
        dstrSync.flags = 0;
        dstrSync.size = 8;
    }
}

// v12 0x00553CA0  (feb 0x0053B390)
int __fastcall dstrInit(void)
{
    if (!entRegisterClass((char *)"dyn_destroy", 0x24445354, 0x28, dstrDESTROY::Create, 0, 1))
        return 0;
    dstrTexOblom = txmManager->Add((char *)"stat_oblom", 0x40000, 1);
    return dstrTexOblom != 0;
}

// v12 0x00553FF0  (feb 0x0053B6E0)
entENTITY *__fastcall dstrDESTROY::Create(animINST *i)
{
    dstrDESTROY *d = new dstrDESTROY;
    if (!d)
        return 0;
    d->flags = 0;
    return d;
}

// v12 0x00554030  (feb 0x0053B720)
BOOL32 dstrDESTROY::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 1:
        ProcessINIT();
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
    case 0x3F4:
        ProcessINIT_LEVEL();
        break;
    case 0x40E:
        ProcessMP_NOTIFY((msgDATA *)data);
        break;
    case 0x4C6:
        ProcessCDT_FIREABLE((dmgDAMAGE *)data);
        break;
    }
    return 0;
}

// v12 0x005540B0  (feb 0x0053B7B0)
BOOL32 dstrDESTROY::ProcessINIT()
{
    if (inst->flags & 0x40)
        priority = 0x28;
    gsSysServer->DeclSyncable(this);
    damageTaken = 0;
    health = 2.0f;
    inst->GetFloat((char *)"DESTROY", (char *)"damage", &health);
    unkBC |= 4;
    if (inst->tpl && inst->tpl->nSeq > 0)
        inst->SetAnimSeq(0, 0);
    if (objOBJ *o = objFindName(inst->root, (char *)"geom_debris"))
        o->SetStateProcYes(8, 0);
    if (objOBJ *o = objFindName(inst->root, (char *)"geom_debris_left"))
        o->SetStateProcYes(8, 0);
    return 1;
}

// v12 0x00554180  (feb 0x0053B880)
// [DESTROY] isPlaceGround: on the floor, lit by it; else just lit where it stands.
void dstrDESTROY::ProcessINIT_LEVEL()
{
    int b;
    if (inst->GetBool((char *)"DESTROY", (char *)"isPlaceGround", &b) && b) {
        FloorPlace(0, 1, 0);
        return;
    }
    ApplyColor();
}

// v12 0x005541D0  (feb 0x0053B8D0)
// The debris animation once; then 5 s; then the debris fades over 1 s and goes.
void dstrDESTROY::ProcessFRAME()
{
    if (inst->flags & 1)
        return;
    if (inst->tpl && inst->tpl->nSeq > 0) {
        if ((flags & DSTRF_ANIMATING) && inst->AdvanceTime(gsElapsedTime, 1, 0) && !(flags & DSTRF_BROKEN)) {
            fadeDelay = 5.0f;
            flags = (flags & ~DSTRF_ANIMATING) | DSTRF_BROKEN;
            fadeLen = 1.0f;
            fadeTime = 1.0f;
        }
        if (flags & DSTRF_BROKEN) {
            if (m3dUpdateTimeField(&fadeDelay, gsElapsedTime))
                flags |= DSTRF_FADING;
            if (flags & DSTRF_FADING) {
                float t0 = 0, t1 = fadeLen, v0 = 0, v1 = 255.0f;
                if (fadeLen < 0) {
                    t0 = fadeLen;
                    t1 = 0;
                    v0 = 255.0f;
                    v1 = 0;
                }
                objOBJ *d = objFindName(inst->root, (char *)"geom_debris");
                d->SetTransparency(0x80, (u8)(int)((v1 - v0) * (fadeTime - t0) / (t1 - t0) + v0));
                if (m3dUpdateTimeField(&fadeTime, gsElapsedTime))
                    d->SetStateProcYes(8, 0);
            }
        }
    }
    flags &= ~DSTRF_THIS_FRAME;
}

void dstrDESTROY::ProcessTERM() {}                      // v12 0x00403A20
void dstrDESTROY::ProcessDESTROY_ENT(entENTITY *) {}    // v12 0x00403950

// v12 0x00554370  (feb 0x0053BA70)
// On the server: the damage adds up; enough breaks it (a second hit of the same frame does not).
void dstrDESTROY::ProcessCDT_FIREABLE(dmgDAMAGE *d)
{
    if (!(entFlags & 4))
        return;
    damageTaken += d->amount;
    if (damageTaken < health)
        return;
    if (flags & DSTRF_THIS_FRAME) {
        flags &= ~DSTRF_THIS_FRAME;
        return;
    }
    StartExplos(0);
}

// v12 0x005543C0  (feb 0x0053BAC0)
// A client: broken (0), or the sync block of a joining one (-100: already broken, silently).
void dstrDESTROY::ProcessMP_NOTIFY(msgDATA *ev)
{
    dstrNOTIFY *n = (dstrNOTIFY *)ev;
    if (n->type == -100) {
        if (n->sync->flags)
            StartExplos(1);
    } else if (n->type == 0) {
        StartExplos(0);
    }
}

// v12 0x00554410  (feb 0x0053BB00)
void dstrDESTROY::ProcessMP_GET_SYNC_DATA(void *d)
{
    msgDATA **out = (msgDATA **)d;
    if (!*out) {
        dstrSyncInit();
        *out = &dstrSync;
    }
    ((dstrSYNC *)*out)->flags = flags;
    entENTITY::ProcessMP_GET_SYNC_DATA(d);
}

// v12 0x00554480  (feb 0x0053BB70)
// The sync block from chunk 0xF0 (only read: ProcessMP_NOTIFY applies it).
void dstrDESTROY::ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    dstrSYNC *s = (dstrSYNC *)d->data;
    if (!s) {
        dstrSyncInit();
        s = &dstrSync;
        d->data = s;
        s->packId = 0;
        s->tag = 0x40;
        s->size = 8;
        s->flags = 0;
    }
    for (int ok = d->file->ReadChunk(&c); ok && c.id != 1; ok = d->file->ReadChunk(&c))
        if (c.id == 0xF0)
            d->file->ReadData(&s->flags, 4, 4);
    entENTITY::ProcessMP_READ_SYNC_DATA(d);
}

// v12 0x00554570  (feb 0x0053BC60)
// Broken: the whole hidden, what is left shown (silent: only that, a joining client's copy). Then the debris: its
// animation, or each piece flying off as a rigid body -- up and out at [DESTROY] iniSpeed (4) turning by up to
// iniMomentum (0.05), a barrel's pieces away from its middle much faster, small pieces turning less -- or, with no
// debris, the model shattered. The server tells the clients.
void dstrDESTROY::StartExplos(int silent)
{
    rgdINIT_PARAMS p;
    memset(&p, 0, sizeof(p));
    p.packId = 0;
    p.tag = 0x40;
    p.size = 0xB8;
    p.unk04 = 8;
    p.vel = m3dVZero;
    p.angVel = m3dVZero;
    p.unk98 = 0;
    p.unk9C = 0.6f;
    p.unkA0 = 0.8f;
    p.unkA4 = 10.0f;
    p.unkA8 = 0.3f;
    p.unkAC = -1.0f;
    p.unkB0 = -1.0f;
    p.unkB4 = -1.0f;
    animCREATE_DATA cd;
    if (flags & (DSTRF_ANIMATING | DSTRF_BROKEN))
        return;
    flags |= DSTRF_ANIMATING | DSTRF_THIS_FRAME;
    pendingEvents |= 0x20;                              // ON_COMPLETE
    objOBJ *o = objFindName(inst->root, (char *)"geom_orig");
    if (o)
        o->SetStateProcYes(8, 0);
    o = objFindName(inst->root, (char *)"geom_debris_left");
    if (o)
        o->SetStateProcNo(8, 0);
    if (silent) {
        flags |= DSTRF_BROKEN;
        return;
    }
    objOBJ *debris = objFindName(inst->root, (char *)"geom_debris");
    if (debris)
        debris->SetStateProcNo(8, 0);
    inst->Invalidate(6);
    if (!inst->tpl || inst->tpl->nSeq < 1) {
        if (!debris) {
            dstrShatterStart.StartExplos(this, 0x14, 8.0f, 3.0f, 1, 0);
        } else {
            objOBJ *list[100];
            int n = objMakePtrList(&debris, 1, list, 100);
            for (int i = 1; i < n; i++) {
                m3dMakeVUnitConusRandom(&m3dVUnitY, 90.0f, &p.vel);
                float speed = 4.0f;
                inst->GetFloat((char *)"DESTROY", (char *)"iniSpeed", &speed);
                p.vel.x *= speed;
                p.vel.y *= speed;
                p.vel.z *= speed;
                float mom = 0.05f;
                inst->GetFloat((char *)"DESTROY", (char *)"iniMomentum", &mom);
                float r = m3dRandRange(mom * 0.2f, mom);
                p.angVel.x = m3dVUnitY.x * r;
                p.angVel.y = m3dVUnitY.y * r;
                p.angVel.z = m3dVUnitY.z * r;
                char *cls = inst->tplName ? inst->tplName : (char *)"";
                if (!strcmp(cls, "barrel")) {
                    m3dV c0, c1;
                    wrVCall<void>(&inst->root->bbox, 0x24, &c0);       // m3dVOL::GetCenter
                    wrVCall<void>(&list[i]->bbox, 0x24, &c1);
                    p.vel.x = c1.x - c0.x;
                    p.vel.y = c1.y - c0.y;
                    p.vel.z = c1.z - c0.z;
                    m3dNormalize(&p.vel);
                    float s = m3dRandRange(4.0f, 13.0f);
                    p.vel.x *= s;
                    p.vel.y *= s;
                    p.vel.z *= s;
                    p.unkA4 = 20.0f;
                    m3dMakeVUnitConusRandom(&m3dVUnitY, 180.0f, &p.angVel);
                    float a = m3dRandRange(0.02f, 0.3f);
                    p.angVel.x *= a;
                    p.angVel.y *= a;
                    p.angVel.z *= a;
                }
                m3dBOX_DATA *b = &list[i]->bbox, *rb = &inst->root->bbox;
                float dx = b->min[0] - b->max[0], dy = b->min[1] - b->max[1], dz = b->min[2] - b->max[2];
                float size = m3dSqrt(dx * dx + dy * dy + dz * dz);
                dx = rb->min[0] - rb->max[0];
                dy = rb->min[1] - rb->max[1];
                dz = rb->min[2] - rb->max[2];
                float k = size / m3dSqrt(dx * dx + dy * dy + dz * dz);
                float turn = k < 0.25f ? 0 : k > 1.0f ? 1.0f : (k - 0.25f) * 1.3333334f;
                p.unkAC = 7.0f;
                p.unkB4 = 3.0f;
                p.angVel.x *= turn;
                p.angVel.y *= turn;
                p.angVel.z *= turn;
                animINST *pi = gsScenePtr->CreateInst(list[i], 0);
                if (pi) {
                    inst->flags |= 0x10;
                    entENTITY *e = entCreate((char *)"nch_rigid", pi, &p, 0);
                    if (e)
                        e->inst->flags |= 0x10;
                }
            }
        }
    }
    if (entFlags & 4) {
        dstrNOTIFY m;
        m.packId = 0;
        m.tag = 0x40;
        m.type = 0;
        m.size = 6;
        gsSysServer->SendMsgClientAll(this, 0x40E, &m, 0x20000, 0xFFFFFFFF, (unsigned long)-3);
    }
}
