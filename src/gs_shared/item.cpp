// item.cpp -- the pickup base class "item" (Will Rock 1.2, v12.exe; feb obj gs_shared:item.obj)
//
// v12 0x00556C90..0x00557AD9. An item stands on the floor (isPlaceGround), spins (isRotated, 70 deg/s) and bobs
// (heightDisp), has no collision (ANIM_ST_IGNORE_CDT) and is picked up by a player within 2.5 (and 2.3 up / down):
// the player's Pick slot (+0x9C) takes the bonus, then the item's Pick (the game's wrITEM, wr_item.cpp, gives the
// rest). An item with [ITEM] nmbPicks n is locked n times and hidden: each IAction PICK takes one lock, the last
// one (or UNLOCK) shows it -- moved onto the one who unlocked it (isMoveUponUnlock) -- with the unhide effect.
// 1.2 changes, from the disassembly: the item's own fields moved +0x1C (the bigger entENTITY); the IAction query is
// 0x3FC and the MP notify 0x40E (feb 0x3FB / 0x40C), its type a short at +4 and the data at +6; the server notifies
// in mode 4 (dedicated) too; the pick test allows 2.3 up / down (feb 2.0).
// Not rebuilt: the MP sync data slots (GetMPSysSyncData, ProcessMP_GET / WRITE / READ_SYNC_DATA).

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30 (drv:Snd_chn.obj)

sncSOUND_DESCR_3D itemUnlockSound;                      // v12 0x009AE648

// v12 0x00556C90  static initialiser (the sncSOUND_DESCR_3D constructor inline)
static struct itemSND_INIT {
    itemSND_INIT()
    {
        sncSOUND_DESCR_3D *d = &itemUnlockSound;
        d->name.SetName((char *)"object_portal_appear");
        d->flags = 0;
        d->volume = 0.0f;
        d->delay = 0;
        d->count = 0;
        d->time = 0;
        d->data = 0;
        d->dataFlags = 0;
        d->minDist = 100.0f;
        d->maxDist = 300.0f;
        d->vtbl = 0;                                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
    }
} itemSndInit;

#pragma pack(push, 1)
struct itemNOTIFY {                     // GS_MSG_MP_NOTIFY 0x40E to the clients
    u8    packId, tag;
    u16   size;
    short type;                         // +0x04 -100 sync block, 0 enable, 1 moved, 2 unlocked, 3 unhide effect
    union {                             // +0x06
        int   on;
        m3dV  pos;
        struct { u32 _0; u32 enabled; u32 locked; } *sync;
    };
};
#pragma pack(pop)

// v12 0x00556CF0  (feb 0x0053E140)
int __fastcall itemInit(void)
{
    if (!entRegisterClass((char *)"item", 0x2449544D, 0x28, itemITEM::Create, (char *)"item", 0))
        return 0;
    if (!sncLinkSndListData(1, &itemUnlockSound)) {
        apMsg((char *)"Cannot link sound data");
        return 0;
    }
    return 1;
}

// v12 0x00556D50  (feb 0x0053E1A0)
BOOL32 itemITEM::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case MSG_MSG_INIT:
        ProcessINIT();
        return 0;
    case MSG_MSG_TERM:
        ProcessTERM();
        return 0;
    case 3:                                             // an address goes away: the sfx entity
        if (sfx == data)
            sfx = 0;
        return 0;
    case 1000:
        ProcessFRAME();
        return 0;
    case 0x3F4:                                         // INIT_LEVEL
        ProcessINIT_LEVEL();
        return 0;
    case 0x3F8: {                                       // IActions
        apNAME *a = (apNAME *)((u8 *)data + 8);
        if (a->IsName((char *)"PICK")) {
            nLocks--;
            Unlock((entIACTIVE_OBJ *)from);
            return 0;
        }
        if (a->IsName((char *)"UNLOCK")) {
            Unlock((entIACTIVE_OBJ *)from);
            return 1;
        }
        return 0;
    }
    case 0x3FC:
        *ret = 1;
        return 1;
    case 0x40E:                                         // MP notify
        ProcessMP_NOTIFY(data);
        return 0;
    }
    return 0;
}

// v12 0x00556E60  (feb 0x0053E2B0)
// [ITEM] isPlaceGround (1), isMoveUponUnlock, isRotated (1), nmbPicks (locks, hidden till then; moves on unlock),
// sfxName ("sfx_item"), height (0.95), heightDisp (0.6 -> bob amplitude * 0.4). No collision; the first sequence.
BOOL32 itemITEM::ProcessINIT()
{
    int b = 1;
    inst->GetBool((char *)"ITEM", (char *)"isPlaceGround", &b);
    if (b == 1)
        flags |= ITEMF_PLACE_GROUND;
    b = 0;
    inst->GetBool((char *)"ITEM", (char *)"isMoveUponUnlock", &b);
    if (b == 1)
        flags |= ITEMF_MOVE_ON_UNLOCK;
    b = 1;
    inst->GetBool((char *)"ITEM", (char *)"isRotated", &b);
    if (b == 1)
        flags |= ITEMF_ROTATED;
    int picks = 0;
    inst->GetInt((char *)"ITEM", (char *)"nmbPicks", &picks);
    if (picks > 0) {
        nLocks = picks;
        flags |= ITEMF_MOVE_ON_UNLOCK;
    }
    if (inst->tpl && inst->tpl->nSeq > 0)
        inst->SetAnimSeq(0, 0.0f);
    inst->flags |= ANIM_ST_IGNORE_CDT;
    char sfxName[0x40];
    strcpy(sfxName, "sfx_item");
    inst->GetStr((char *)"ITEM", (char *)"sfxName", sfxName, 0x40);
    height = 0.95f;
    inst->GetFloat((char *)"ITEM", (char *)"height", &height);
    bobAmp = 0.6f;
    inst->GetFloat((char *)"ITEM", (char *)"heightDisp", &bobAmp);
    bobTime = 0.0f;
    bobPeriod = 2.0f;
    bobAmp = bobAmp * 0.4f;
    bobOffset = 0.0f;
    CreateSfx(sfxName);
    hideTimer = 0.0f;
    flags |= ITEMF_ACTIVE;
    gsSysServer->DeclSyncable(this);
    *(objOBJ **)((u8 *)inst + 0xEC) = inst->root;       // (animINST +0xEC: the object the centre is taken from) // ?
    inst->root->SetStateYes(0x10);
    return 1;
}

// v12 0x00557060  (feb 0x0053E4B0)
void itemITEM::ProcessINIT_LEVEL()
{
    if (flags & ITEMF_PLACE_GROUND)
        FloorPlace(height, 1, &floor);
}

// v12 0x00557080  (feb 0x0053E4D0)
void itemITEM::ProcessTERM()
{
    DestroySfx();
}

// v12 0x00557090  (feb 0x0053E4E0)
// Shown while active and not hidden; the server re-enables a taken one when its timer runs out (MP). A locked item
// (or one hidden by an MP sync) stays hidden. A shown one spins and bobs (the root's first child), then every
// player in range that takes it picks it (the first one in range raises event 0x20).
void itemITEM::ProcessFRAME()
{
    if (flags & ITEMF_FORCE_HIDDEN)
        inst->flags |= 1;
    if (!(inst->flags & 1) && (flags & ITEMF_ACTIVE))
        ShowSfx(1);
    else
        ShowSfx(0);
    if (gsSysMP->mode != 1 && (entFlags & 4)) {
        m3dUpdateTimeField(&hideTimer, gsElapsedTime);
        if (!(flags & ITEMF_ACTIVE) && hideTimer < 1e-6f) {
            Enable(1);
            itemNOTIFY m;
            m.packId = 0;
            m.tag = 0x40;
            m.size = 0xA;
            m.type = 0;
            m.on = 1;
            gsSysServer->SendMsgClientAll(this, 0x40E, (msgDATA *)&m, 0x20000, 0xFFFFFFFD, 0xFFFFFFFD);
        }
    }
    if (!(flags & ITEMF_ACTIVE))
        return;
    if (((entFlags & 4) && nLocks > 0) || (flags & ITEMF_HIDDEN_NOPICK)) {
        inst->flags |= 1;
        return;
    }
    if (inst->flags & 3) {
        if (gsSysMP->mode == 1)
            return;
    } else if ((int)inst->flags < 0) {                  // (drawn last frame)
        m3dMATR *local = (m3dMATR *)inst->root->child->local;
        if (flags & ITEMF_ROTATED)
            local->RotateY(gsElapsedTime * 70.0f, 2);
        bobTime = gsElapsedTime + bobTime;
        if (bobPeriod < bobTime)
            bobTime = bobTime - bobPeriod;
        float t = bobTime, w;                           // a triangle wave, 0..1..-1..0 over 2 s
        if (t < 0.5f)
            w = t + t;
        else if (t <= 1.5f)
            w = 1.0f - ((t - 0.5f) + (t - 0.5f));
        else
            w = ((t - 1.5f) + (t - 1.5f)) - 1.0f;
        float y = w * bobAmp;
        local->Translate(0.0f, y - bobOffset, 0.0f, 2);
        bobOffset = y;
        inst->Invalidate(4);
    }
    UpdateSfx();
    if (!(entFlags & 4))
        return;
    for (entENTITY *pl = gsSysServer->GetNextPlayer(0); pl; pl = gsSysServer->GetNextPlayer(pl)) {
        if (!IsCanPick((plgsACTION *)pl))
            continue;
        if (!(flags & ITEMF_PICK_SEEN)) {
            pendingEvents |= 0x20;
            flags |= ITEMF_PICK_SEEN;
        }
        if (wrEVCall<BOOL32>(pl, 0x9C, (entENTITY *)this)) // the player takes it (plrPLAYER_COMMON::Pick)
            Pick((plgsACTION *)pl);
    }
}

// v12 0x00557340  (feb 0x0053E790)
// Active, within 2.5 across and 2.3 up / down of the player.
BOOL32 itemITEM::IsCanPick(plgsACTION *by)
{
    if (!(flags & ITEMF_ACTIVE))
        return 0;
    m3dV a, b;
    inst->GetPos(&a);
    wrEVCall<void>(by, 0x64, &b);                       // the player's position (GetPos)
    float dx = a.x - b.x, dz = a.z - b.z;
    if (m3dSqrt(dx * dx + dz * dz) < 2.5f && fabsf(a.y - b.y) < 2.3f)
        return 1;
    return 0;
}

// v12 0x00557460  (feb 0x0053E8B0)
// The last lock gone: (moving onto an entity that unlocked it, isMoveUponUnlock) shown, the unhide effect, the
// clients told; the unlock sound unless it moved.
void itemITEM::Unlock(entIACTIVE_OBJ *by)
{
    if (nLocks > 0)
        return;
    m3dV p;
    if (by && *(u32 *)((u8 *)by + 0x20) == 0x454E5424 && (flags & ITEMF_MOVE_ON_UNLOCK)) {
        ((entENTITY *)by)->inst->GetPos(&p);
        p.y = p.y + height;
        inst->Translate(&p, 0);
        if (gsSysMP->mode == 2 || gsSysMP->mode == 4) {
            itemNOTIFY m;
            m.packId = 0;
            m.tag = 0x40;
            m.size = 0x12;
            m.type = 1;
            m.pos = p;
            gsSysServer->SendMsgClientAll(this, 0x40E, (msgDATA *)&m, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
        }
    }
    inst->flags &= ~1u;
    ShowUnhideSfx();
    itemNOTIFY m;
    m.packId = 0;
    m.tag = 0x40;
    m.size = 6;
    m.type = 2;
    gsSysServer->SendMsgClientAll(this, 0x40E, (msgDATA *)&m, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
    if (!(flags & ITEMF_MOVE_ON_UNLOCK)) {
        inst->GetPos(&p);
        gsSysSound->Play(&itemUnlockSound, &p);
    }
}

// v12 0x005575B0  (feb 0x0053EA00)
void itemITEM::Pick(plgsACTION *)
{
    pendingEvents |= 0x20;
}

// v12 0x005575D0  (feb 0x0053EA20)
void itemITEM::ProcessMP_NOTIFY(void *ev)
{
    itemNOTIFY *m = (itemNOTIFY *)ev;
    switch (m->type) {
    case 0:
        Enable(m->on);
        break;
    case 1:
        inst->Translate(&m->pos, 0);
        break;
    case 2:
        flags &= ~ITEMF_HIDDEN_NOPICK;
        inst->flags &= ~1u;
        break;
    case 3:
        ShowUnhideSfx();
        break;
    case -100:
        Enable(m->sync->enabled);
        if (m->sync->locked)
            flags |= ITEMF_HIDDEN_NOPICK;
        break;
    }
}

// v12 0x005578B0  (feb 0x0053EDC0)
void itemITEM::Enable(int on)
{
    if (!on) {
        flags &= ~ITEMF_ACTIVE;
        inst->flags |= 1;
        return;
    }
    flags |= ITEMF_ACTIVE;
    inst->flags &= ~1u;
}

// v12 0x005578F0  (feb 0x0053EE00)
// The server tells the clients to show the effect (1.2: a dedicated server too).
void itemITEM::ShowUnhideSfx()
{
    itemNOTIFY m;
    m.packId = 0;
    m.tag = 0x40;
    m.size = 6;
    m.type = -1;
    if (gsSysMP->mode == 2 || gsSysMP->mode == 4) {
        m.type = 3;
        gsSysServer->SendMsgClientAll(this, 0x40E, (msgDATA *)&m, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
    }
}

// v12 0x00557950  (feb 0x0053EE60)
// The sfx entity of that class at the item, following its root's first child.
BOOL32 itemITEM::CreateSfx(char *name)
{
    animCREATE_DATA cd;
    m3dV p;
    inst->GetPos(&p);
    cd.matr.Translate(&p, 0);
    sfx = entCreate(gsScenePtr, name, name, &cd, inst, 0);
    if (!sfx)
        return 0;
    sfx->SetFollow(this, inst->root->child, 0);
    return 1;
}

// v12 0x00557A00  (feb 0x0053EF10)
void itemITEM::DestroySfx()
{
    if (sfx)
        msgSystem.PostMsg(sfx, MSG_MSG_TERM, this);
}

// v12 0x00557A20  (feb 0x0053EF30)
void itemITEM::ShowSfx(int on)
{
    if (!sfx)
        return;
    if (on)
        sfx->inst->flags &= ~1u;
    else
        sfx->inst->flags |= 1;
}

// v12 0x00403A20 (the shared empty function)
void itemITEM::UpdateSfx()
{
}

// v12 0x004A9FB0  (feb 0x0049BD60, emitted in wr_item.obj)
BOOL32 itemITEM::GetFloorInfo(cdtINFO *ci)
{
    *ci = floor;
    return 1;
}

// v12 0x00557A50  (feb 0x0053EF60)
entENTITY *__fastcall itemITEM::Create(animINST *)
{
    itemITEM *e = new itemITEM;                         // operator new(0x1AF), entENTITY ctor
    if (!e)
        return 0;
    e->flags = 0;
    e->amount = 0;
    e->height = 0;
    e->bobAmp = 0;
    e->sfx = 0;
    e->floor.flags = 0;
    e->floor.inst = 0;
    e->floor.objId = 0;
    e->floor.faceId = 0;
    e->floor.normal[0] = m3dVZero.x;
    e->floor.normal[1] = m3dVZero.y;
    e->floor.normal[2] = m3dVZero.z;
    e->floor.dist = 0;
    return e;
}
