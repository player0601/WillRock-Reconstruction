// sob_trading_altar.cpp -- the trading altar and its buttons (Will Rock 1.2, v12.exe; feb obj sob_trading_altar.obj)
//
// v12 0x00489A60..0x0048ADC0. Beside the altar ("sob_tra_altar") stand three buttons ("sob_tra_button", [TRA_BUTTON]
// nameEffect: quad_damage, unholy_armor or slow_motion). plrPLAYER::ProcessUse sells: with the gold, pushing a button
// takes the buyer into a cut-scene (ProcessPowerup: moved to the button's "player_pos", the view on a camera of the
// shared "sob_tra_camera"), the altar plays its sequence 0; then the "sfx_trading_altar" effect for 5 s; at the end the
// powerup is picked and the button pops back and rests for 3 s. The use key (command 0x11) skips it all.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../gs/gs_mp_misc.h"
#include "../m3d/m3d_spl.h"

txmTEXTURE       *wrsobTRA_ALTAR::pTexPart;             // v12 0x0064F600 "sfx_trading_altar_part"
sncSOUND_DESCR_3D wrsobTRA_ALTAR::snd[4];               // v12 0x0064F460 glow (looped), sparks, button in, button out

static struct wrsobTRA_SND_INIT {
    wrsobTRA_SND_INIT()
    {
        static const char *names[4] = { "trad_altar_glow", "trad_altar_sparks", "trad_altar_btn", "trad_altar_btn_pop" };
        for (int i = 0; i < 4; i++) {
            sncSOUND_DESCR_3D *d = &wrsobTRA_ALTAR::snd[i];
            d->name.SetName((char *)names[i]);
            d->flags = i == 0 ? 2 : 0;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = i == 0 ? 0.0f : 150.0f;
            d->maxDist = i == 0 ? 0.0f : 200.0f;
            d->vtbl = 0;                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} wrsobTraSndInit;

#pragma pack(push, 1)
struct wrsobTRA_MSG : msgDATA {                         // GS_MSG_MP_NOTIFY 0x40E to the altar (0x0E bytes; type 1: 6)
    short         type;                                 // +0x04 0 a sale begun, 1 skipped
    unsigned long buyer;                                // +0x06 MP ids
    unsigned long button;                               // +0x0A
};
struct wrsobTRA_START_MSG : msgDATA {                   // 0x40E to the buyer: type 13 begun (0x3E bytes), 14 over (0x0A)
    short type;                                         // +0x04
    int   powerup;                                      // +0x06
    m3dV  altarPos;                                     // +0x0A
    m3dV  altarDir;                                     // +0x16
    m3dV  plrPos;                                       // +0x22
    m3dV  plrDir;                                       // +0x2E
    float time;                                         // +0x3A the altar's sequence
};
#pragma pack(pop)

// the m3dSPL_LINEAR1D constructor inline (v12 0x00489B3F)
static m3dSPL_LINEAR1D *traSpl(int n)
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

// v12 0x00489A60  (feb 0x0047F270)
int __fastcall wrsobInitTradingAltar()
{
    if (!entRegisterClass((char *)"sob_tra_altar", 0x2444594E, 0x28, wrsobTRA_ALTAR::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"sob_tra_button", 0x4254544E, 0x28, wrsobTRA_BUTTON::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"sob_tra_camera", 0x2444594E, 0x28, wrsobTRA_CAMERA::Create, 0, 1))
        return 0;
    wrsobTRA_ALTAR::pTexPart = txmManager->Add((char *)"sfx_trading_altar_part", 0x40003, 1);
    if (!wrsobTRA_ALTAR::pTexPart)
        return 0;
    if (!sncLinkSndListData(4, wrsobTRA_ALTAR::snd))
        return 0;
    return wrsfxTradingAltarInit() != 0;
}

// v12 0x00489B00  (feb 0x0047F300)
// The altar's slow glow: rising sparks all the time.
wrsfxTRADING_ALTAR_EMITTER::wrsfxTRADING_ALTAR_EMITTER(float scale) : partEMITTER_PHYS(50.0f, 1.0f)
{
    SetTimeLifePart(0.65f, 0.35f);
    m3dSPL_LINEAR1D *sz = traSpl(2);
    sz->SetKp(0, 0.0f, scale * 0.0f);
    float size = scale * 5.0f;
    sz->SetKp(1, 1.0f, size);
    SetSize(size, 0, sz, 1.0f);
    m3dSPL_LINEAR1D *a = traSpl(2);
    a->SetKp(0, 0.0f, 0.0f);
    a->SetKp(1, 1.0f, 255.0f);
    SetColor(0xFFFFFFFF, 0, a);
    SetTexture(wrsobTRA_ALTAR::pTexPart);
    speedASpl = 0;
    speedA = scale * 0.2f;
    speedARand = scale * 0.1f;
    flags |= 0x409;
    flags |= 0xA000;
    flags |= 0x1000000;
    SetScaleCSVal(0.1f, 1.0f, 0.1f);
    flags |= 0x8000000;
}

// v12 0x00489CC0  (feb 0x0047F4E0)
// The sale begins: on the server the buyer is told (13: into the cut-scene, for the length of the altar's sequence 0).
void wrsobTRA_ALTAR::StartPowerup(wrsobTRA_BUTTON *b)
{
    wrsobTRA_START_MSG m;
    m.packId = 0;
    m.tag = 0x40;
    m.powerup = -1;
    m.time = 0;
    m.type = 0x0D;
    m.size = 0x3E;
    button = b;
    if (entFlags & 4) {
        m.powerup = b->powerup;
        m.time = inst->GetSeqLength(0);
        objOBJ *pp = button->playerPos;
        pp->GetOrigin(&m.plrPos);
        pp->GetAxisZ(&m.plrDir);
        inst->GetPos(&m.altarPos);
        inst->GetDir(&m.altarDir);
        msgSystem.SendMsg(buyer, 0x40E, &m, this);
    }
    state = 1;
    time = inst->GetSeqLength(0);
}

// v12 0x00489DB0  (feb 0x0047F500..)
// The sequence is over: 5 s of the effect, on the altar's "place_effect", facing away from the button.
void wrsobTRA_ALTAR::StartEffect(int pw)
{
    animCREATE_DATA cd;
    state = 2;
    time = 5.0f;
    wrsfxTRADING_ALTAR_INIT d;
    d.packId = 0;
    d.tag = 0x40;
    d.scale = 1.0f;
    d.kind = -1;
    d.size = 0x0C;
    sndOn->Play(&snd[1], 0);
    m3dV org, dir;
    placeEffect->GetOrigin(&org);
    button->faceObj->GetAxisY(&dir);
    dir.y = 0;
    m3dNormalize(&dir);
    dir.x = -dir.x;
    dir.y = -dir.y;
    dir.z = -dir.z;
    ((m3dMATR *)((char *)&cd + 0x12))->_MakeLCS2WCS_VZ(&org, &dir);
    d.scale = 1.0f;
    d.kind = pw;
    if (entFlags & 4)
        sfx = entCreate(gsScenePtr, (char *)"sfx_trading_altar", (char *)"sfx_trading_altar", &cd, &d, 0);
}

// v12 0x00489ED0  (feb 0x0047F700..)
// Over: the buyer told (14: out of the cut-scene, the powerup picked), the button pops back and rests for 3 s.
void wrsobTRA_ALTAR::EndPowerup()
{
    wrsobTRA_START_MSG m;
    m.packId = 0;
    m.tag = 0x40;
    m.powerup = -1;
    m.type = 0x0E;
    m.size = 0x0A;
    state = 0;
    if (buyer && (entFlags & 4)) {
        m.powerup = button->powerup;
        msgSystem.SendMsg(buyer, 0x40E, &m, this);
        wrEVCall<BOOL32>(button, 0x64, buyer);          // Toggle
        button->lockTime = 3.0f;
        button->locked = 1;
        button = 0;
    }
    buyer = 0;
    if (entFlags & 4)
        sfx = 0;
}

// v12 0x00489F90  (feb 0x0047F790..)
// The objects, the glow on "emitter", the parts that do not cast shadows, two voices (the glow's loop starts), the
// invisible collision object at the altar's feet.
BOOL32 wrsobTRA_ALTAR::ProcessINIT(void *)
{
    static const char *noShadow[] = { "pSphere1", "pSphere2", "revol11", "revol12", "revol13", "revol14", "revol21",
                                      "revol22", "revol23", "revol24", 0 };     // v12 0x00630538
    placeEffect = objFindNameN(inst->root, (char *)"place_effect", 12);
    if (!placeEffect)
        return 0;
    inst->Validate(4);
    emitter = new wrsfxTRADING_ALTAR_EMITTER(1.0f);
    if (!emitter)
        return 0;
    objOBJ *em = objFindNameN(inst->root, (char *)"emitter", 7);
    if (!em)
        return 0;
    em->SetStateProcYes(8, 0);
    if (!partSetAreaEmissionObj(emitter, em, -1, 0))
        return 0;
    emitter->SetScaleCS((m3dMATR *)((char *)em + 0x3C));
    partAttachInst(emitter, inst);
    emitter->StartEmission();
    for (const char **n = noShadow; *n; n++) {
        objOBJ *o = objFindNameN(inst->root, (char *)*n, (int)strlen(*n));
        if (o)
            o->state |= 0x20;
    }
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
    c->minDist = 20.0f;
    c->maxDist = 70.0f;
    sndOn = c;
    gsSysSound->AddChannel(sndOn, this, 0);
    c = new sncCHANNEL_CONE;
    c->minDist = 15.0f;
    c->maxDist = 55.0f;
    sndLoop = c;
    gsSysSound->AddChannel(sndLoop, this, 0);
    gsSysServer->DeclSyncable(this);
    sndLoop->Play(&snd[0], 0);
    msgADDR::flags |= 2;                                // (told when an address goes: the buyer)
    entENTITY *coll = entCreate(gsScenePtr, (char *)"dynamic", (char *)"sob_trading_altar_coll_obj", 0, 0, 0);
    if (!coll)
        return 0;
    m3dV pos, o, dir;
    inst->GetPos(&pos);
    em->GetOrigin(&o);
    pos.y = o.y - 0.5f;
    inst->GetDir(&dir);
    coll->inst->SetPosDir_XZ(&pos, &dir);
    coll->inst->Validate(4);
    coll->inst->flags |= 8;
    return 1;
}

// v12 0x0048A3D0  (feb 0x0047FB90..)
// While selling: the local buyer's skip key asks to end it (notify 1); the time out: the next stage.
void wrsobTRA_ALTAR::ProcessFRAME()
{
    if (!state)
        return;
    if (buyer && !wrEVCall<BOOL32>(buyer, 0x11C)) {     // the first-person buyer
        gsINP_ACTIONS *a = gsSysInput->actions;
        if (a->first <= 0x11 && 0x11 < a->last) {
            gsiCOMMAND *c = &a->states[0x11 - a->first];
            if ((c->flags & 1) && (c->flags & 2)) {
                wrsobTRA_MSG m;
                m.packId = 0;
                m.tag = 0x40;
                m.size = 6;
                m.type = 1;
                wrEVCall<int>(gsSysClient, 0x2C, (void *)this, 0x40E, (void *)&m, 0x20000);
                c->flags &= 0x60;
                *(float *)((char *)c + 0x44) = 0;
            }
        }
    }
    if (!m3dUpdateTimeField(&time, gsElapsedTime))
        return;
    if (state == 1)
        StartEffect(button->powerup);
    else if (state == 2)
        EndPowerup();
}

// v12 0x0048A530  (feb 0x0047FD40..)
// 1: skipped (the effect goes, it ends, the clients hear); 0 on a client: a sale begun.
void wrsobTRA_ALTAR::ProcessMP_NOTIFY(void *d)
{
    wrsobTRA_MSG *m = (wrsobTRA_MSG *)d;
    if (m->type == 1) {
        if ((entFlags & 4) && sfx) {
            msgSystem.PostMsg(sfx, 2, this);
            sfx = 0;
        }
        EndPowerup();
        if (entFlags & 4)
            gsSysServer->SendMsgClientAll(this, 0x40E, m, 0x20000, -1, -3);
    } else if (m->type == 0) {
        buyer = (entENTITY *)gsSysMP->FindMPID(m->buyer, 0);
        StartPowerup((wrsobTRA_BUTTON *)gsSysMP->FindMPID(m->button, 0));
    }
}

// v12 0x0048A5E0  (feb 0x0047F5F0)
BOOL32 wrsobTRA_ALTAR::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 1:
        ProcessINIT(data);
        break;
    case 2:
        if (emitter) {
            emitter->__vecDelDtor(1);
            emitter = 0;
        }
        break;
    case 3:
        if ((void *)buyer == data)
            buyer = 0;
        break;
    case 1000:
        ProcessFRAME();
        break;
    case 0x40E:
        ProcessMP_NOTIFY(data);
        break;
    }
    return 0;
}

// v12 0x0048AD20  (feb 0x0047FFD0)
entENTITY *__fastcall wrsobTRA_ALTAR::Create(animINST *)
{
    void *mem = operator new(0x18F);
    if (!mem)
        return 0;
    wrsobTRA_ALTAR *a = new (mem) wrsobTRA_ALTAR;
    a->state = 0;
    a->time = 0;
    a->emitter = 0;
    a->button = 0;
    a->placeEffect = 0;
    a->buyer = 0;
    a->sfx = 0;
    a->sndOn = 0;
    a->sndLoop = 0;
    return a;
}

// v12 0x0048A670  (feb 0x0047F9B0)
// Not while it rests; slow motion not in co-op; not while the altar sells; not locked or pushed; the buyer must face
// it, within 2 and 90 degrees.
BOOL32 wrsobTRA_BUTTON::IsCanPush(entENTITY *by)
{
    if (locked)
        return 0;
    if (powerup == 2 && wrSysGO->mode == 2)
        return 0;
    if (((wrsobTRA_ALTAR *)altarInst->ent)->buyer)
        return 0;
    if (nLocks > 0 || ((flags & 0x10) && (flags & 0x20)))
        return 0;
    m3dV pos, dir, org;
    wrEVCall<void>(by, 0x64, &pos);                     // GetPos
    wrEVCall<void>(by, 0x6C, &dir);                     // GetDir
    faceObj->GetOrigin(&org);
    return m3dIsBelongPointCone(&org, &pos, &dir, 2.0f, 0.0f, 90.0f) != 0;
}

// v12 0x0048A750  (feb 0x0047FAB0)
// Pushed: on the server, with the altar free, the sale begins (the clients hear). The button's sounds on the altar's
// voice; then the button's own toggle.
BOOL32 wrsobTRA_BUTTON::Toggle(entENTITY *by)
{
    wrsobTRA_ALTAR *a = (wrsobTRA_ALTAR *)altarInst->ent;
    if (flags & 4) {
        if ((entFlags & 4) && !a->buyer) {
            a->buyer = by;
            a->StartPowerup(this);
            wrsobTRA_MSG m;
            m.packId = 0;
            m.tag = 0x40;
            m.type = 0;
            m.size = 0x0E;
            m.buyer = by->mpId;
            m.button = mpId;
            gsSysServer->SendMsgClientAll(a, 0x40E, &m, 0x20000, -1, -3);
        }
        a->sndOn->Play(&wrsobTRA_ALTAR::snd[2], 0);
        return bttnBUTTON::Toggle(by);
    }
    a->sndOn->Play(&wrsobTRA_ALTAR::snd[3], 0);
    return bttnBUTTON::Toggle(by);
}

// v12 0x0048A830  (feb 0x0047FAF0)
// The button's messages first. INIT: "player_pos", the first object with faces, the nearest altar, the powerup it
// sells ([TRA_BUTTON] nameEffect; entity type 1000 + the powerup, which plrPLAYER::ProcessUse reads). FRAME: its rest.
BOOL32 wrsobTRA_BUTTON::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    bttnBUTTON::ProcessMsg(id, data, from, ret);
    if (id == 1) {
        playerPos = objFindNameN(inst->root, (char *)"player_pos", 10);
        if (!playerPos)
            return 0;
        faceObj = objFind(inst->root, objIsWithFaces, 0);
        if (!faceObj)
            return 0;
        flags |= 0x40;
        altarInst = 0;
        m3dV org;
        faceObj->GetOrigin(&org);
        float best = 2000000.0f;
        for (animINST *a = gsScenePtr->FindNextInst(0); a; a = gsScenePtr->FindNextInst(a)) {
            if (_strcmpi(a->className ? a->className : "", "sob_tra_altar"))
                continue;
            m3dV p;
            a->GetPos(&p);
            float dx = p.x - org.x, dy = p.y - org.y, dz = p.z - org.z;
            float dist = m3dSqrt(dx * dx + dy * dy + dz * dz);
            if (!altarInst || dist < best) {
                best = dist;
                altarInst = a;
            }
        }
        if (!altarInst)
            return 0;
        char name[0x40];
        if (!inst->GetStr((char *)"TRA_BUTTON", (char *)"nameEffect", name, 0x40))
            return 0;
        if (!_strcmpi(name, "quad_damage")) {
            powerup = 0;
            unk98 = 1000;
        } else if (!_strcmpi(name, "unholy_armor")) {
            powerup = 1;
            unk98 = 1001;
        } else if (!_strcmpi(name, "slow_motion")) {
            powerup = 2;
            unk98 = 1002;
        }
    } else if (id == 1000 && locked && m3dUpdateTimeField(&lockTime, gsElapsedTime)) {
        locked = 0;
    }
    return 0;
}

// v12 0x0048ABB0  (feb 0x0047FE70)
entENTITY *__fastcall wrsobTRA_BUTTON::Create(animINST *)
{
    void *mem = operator new(0x193);
    if (!mem)
        return 0;
    wrsobTRA_BUTTON *b = new (mem) wrsobTRA_BUTTON;
    b->flags = 0;
    b->seqPush = -1;
    b->seqPop = -1;
    b->pushLen = 0;
    b->unk98 = 0;
    b->playerPos = 0;
    b->altarInst = 0;
    b->faceObj = 0;
    b->powerup = -1;
    b->locked = 0;
    b->lockTime = 0;
    return b;
}

// v12 0x0048AB30  (feb 0x0047FDF0)
// The dynamic's, then its three cameras (by powerup), at the start of its sequence, out of collision.
BOOL32 wrsobTRA_CAMERA::ProcessINIT(void *init)
{
    static const char *names[3] = { "camera_qd", "camera_ua", "camera_sm" };    // v12 0x0063052C
    if (!dynDYNAMIC::ProcessINIT(init))
        return 0;
    for (int i = 0; i < 3; i++) {
        cam[i] = objFindName(inst->root, (char *)names[i]);
        if (!cam[i])
            return 0;
    }
    inst->SetAnimSeq(0, 0);
    flags2 |= 4;
    inst->flags |= 0x10;
    return 0;
}

// v12 0x0048AC60  (feb 0x0047FF20)
entENTITY *__fastcall wrsobTRA_CAMERA::Create(animINST *)
{
    void *mem = operator new(0x1A7);
    if (!mem)
        return 0;
    wrsobTRA_CAMERA *c = new (mem) wrsobTRA_CAMERA;
    c->catchUpRate = 1.0f;                              // (the dynamic's Create, inline)
    c->timeDebt = 0;
    c->dynFlags = 0;
    c->unk177[0] = c->unk177[1] = c->unk177[2] = c->unk177[3] = 0;
    c->unk187 = m3dVZero;
    c->unk193 = 0;
    return c;
}
