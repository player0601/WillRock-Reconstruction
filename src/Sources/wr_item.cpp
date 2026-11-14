// wr_item.cpp -- the game's pickups (Will Rock 1.2, v12.exe; feb obj wr_item.obj)
//
// v12 0x004A88B0..0x004A9FF1. wrITEM is the "item" class the game registers (over gs_shared's itemITEM, same name):
// ProcessINIT gives each template its kind and amount (the list in wr_item.h), the "sfx_item" sparks are a
// particle emitter (wrITEM_SPARKS) instead of an entity, an item appearing throws flame sparks, and Pick removes it
// (single player), respawns it (DM / TH) or keeps it for the other players (COOP).
// 1.2 changes, from the disassembly: the fields moved +0x1C; mdg ammo gives 9 and bmb ammo 1 (feb 5 / 5); a Treasure
// Hunt treasure is drawn always (inst flag 0x2000000); a treasure (0x34..0x38) is always removed when picked; the
// money's sparks sit 0.6 higher; Pick marks the entity 0x280 (feb nothing); a dedicated server (mode 4) runs the
// Treasure Hunt physics too; message 0x3FA puts it on the floor again (new).
// Not rebuilt: the Treasure Hunt treasure's physics (physMATH, made in ProcessINIT in mode 3; run in ProcessFRAME).

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../../include/game_shared/wr_item.h"
#include "../../include/frontend/wr_game.h"
#include "../m3d/m3d_spl.h"
#include "../m3d/m3d_vol.h"     // m3dBOX_vftable
#include "../wr_vcall.h"

txmTEXTURE *wrITEM::pTexSparks;                         // v12 0x0064FF90

#pragma pack(push, 1)
struct wrITEM_NOTIFY {                  // GS_MSG_MP_NOTIFY 0x40E (itemITEM's types, 5: COOP "you took it")
    u8    packId, tag;
    u16   size;
    short type;
    int   val;
};
#pragma pack(pop)

// the m3dSPL_LINEAR1D constructor inline (v12 0x004A9BD4)
static m3dSPL_LINEAR1D *wrItemSpl(int n)
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

// v12 0x004A88B0  (feb 0x0049A7A0)
int __fastcall wrItemInit(void)
{
    if (!entRegisterClass((char *)"item", 0x2449544D, 0x28, wrITEM::Create, (char *)"item", 0))
        return 0;
    wrITEM::pTexSparks = txmManager->Add((char *)"sfx_item_spark", 0x40003, 1);
    return wrITEM::pTexSparks != 0;
}

// v12 0x004A8900  (feb 0x0049A7F0)
// The kind and amount by the template's name, then itemITEM's INIT.
BOOL32 wrITEM::ProcessINIT()
{
    static const struct { const char *tpl; int type; float amount; } kinds[] = {
        { "pwp_bsk_item", 6, 8.0f },   { "pwp_stg_item", 2, 10.0f },  { "pwp_mcg_item", 3, 100.0f },
        { "pwp_mdg_item", 7, 10.0f },  { "pwp_mng_item", 8, 200.0f }, { "pwp_crb_item", 4, 12.0f },
        { "pwp_acd_item", 5, 10.0f },  { "pwp_bmb_item", 9, 1.0f },   { "pwp_amg_item", 10, 8.0f },
        { "pwp_stg_ammo", 0x14, 10.0f }, { "pwp_bsk_ammo", 0x18, 4.0f }, { "pwp_mdg_ammo", 0x19, 9.0f },
        { "pwp_mcg_ammo", 0x15, 50.0f }, { "pwp_mng_ammo", 0x1A, 100.0f }, { "pwp_crb_ammo", 0x16, 6.0f },
        { "pwp_acd_ammo", 0x17, 5.0f },  { "pwp_bmb_ammo", 0x1B, 1.0f },  { "pwp_amg_ammo", 0x1C, 4.0f },
        { "item_medkit1", 0x28, 10.0f }, { "item_medkit2", 0x29, 25.0f }, { "item_medkit", 0x29, 25.0f },
        { "item_medkit3", 0x2A, 100.0f }, { "item_medkit4", 0x2B, 200.0f },
        { "item_armor1", 0x2C, 10.0f },  { "item_armor2", 0x2D, 25.0f },  { "item_armor3", 0x2D, 100.0f },
        { "item_armor4", 0x2F, 200.0f },
    };
    const char *t = inst->tplName ? inst->tplName : "";
    int i, n = sizeof(kinds) / sizeof(kinds[0]);
    for (i = 0; i < n && strcmp(t, kinds[i].tpl); i++)
        ;
    if (i < n) {
        itemType = kinds[i].type;
        amount = kinds[i].amount;
    } else if (!strcmp(t, "item_key")) {
        itemType = 0x31;                                // (no amount)
    } else if (!strcmp(t, "item_money")) {
        itemType = 0x30;
        amount = 10.0f;
    } else if (!strcmp(t, "item_treasure_small") || !strcmp(t, "item_treasure_helmet1")) {
        itemType = 0x34;
        amount = 10.0f;
    } else if (!strcmp(t, "item_treasure_helmet")) {
        itemType = 0x35;
        amount = 20.0f;
    } else if (!strcmp(t, "item_treasure_sword")) {
        itemType = 0x36;
        amount = 30.0f;
    } else if (!strcmp(t, "item_treasure_shield")) {
        itemType = 0x37;
        amount = 40.0f;
    } else if (!strcmp(t, "item_treasure_armor")) {
        itemType = 0x38;
        amount = 50.0f;
    } else if (!strcmp(t, "item_quad_damage")) {
        itemType = 0x32;
    } else if (!strcmp(t, "item_unholy_armor")) {
        itemType = 0x33;
    } else if (strstr(t, "item_mpth")) {
        itemType = 0x3C;
        inst->flags |= 0x2000000;
    }
    msgADDR::flags |= 2;
    pickDelay = -1.0f;
    // (not rebuilt: in Treasure Hunt, wrSysGO->mode 3, the treasure 0x3C gets its physMATH here, entFlags 0x20)
    if (wrSysStat)
        wrSysStat->Notify_CreateItem(itemType, amount);
    return itemITEM::ProcessINIT();
}

// v12 0x004A9240  (feb 0x0049B120)
void wrITEM::ProcessINIT_LEVEL()
{
    itemITEM::ProcessINIT_LEVEL();
    if (!sparks)
        return;
    m3dV p;
    inst->GetPos(&p);
    if (itemType == 0x30)                               // 1.2: the money's sparks higher
        p.y = p.y + 0.6f;
    sparks->SetPosDirSpeed(&p, 0, 0);
}

// v12 0x004A92A0  (feb 0x0049B160)
// Not while its pick delay runs, not twice by one player (COOP), not by one flagged 0x40000000 (+0x1F3).
BOOL32 wrITEM::IsCanPick(plgsACTION *by)
{
    if (pickDelay > 0.0f)
        return 0;
    for (int i = 0; i < nPickedBy; i++)
        if (pickedBy[i] == by->mpId)
            return 0;
    if (*(u32 *)((u8 *)by + 0x1F3) & 0x40000000)
        return 0;
    return itemITEM::IsCanPick(by);
}

// v12 0x004A9310  (feb 0x0049B1D0)
// By wrSysGO->itemMode: 0 hidden and removed, 1 hidden and back after 20 s (75 s for quad damage / unholy armor), 2
// (COOP) kept, the taker told and remembered. A weapon or the key is kept in COOP; a treasure is always removed. A
// client (or a game that is loading) only removes it.
void wrITEM::Pick(plgsACTION *by)
{
    wrITEM_NOTIFY hide, coop;
    hide.packId = 0;
    hide.tag = 0x40;
    hide.size = 0xA;
    hide.type = 0;
    hide.val = 0;
    coop.packId = 0;
    coop.tag = 0x40;
    coop.size = 6;
    coop.type = 5;
    itemITEM::Pick(by);
    if (gsSysMP->mode != 1 && !(gsAppState & 0x400)) {
        int mode = wrSysGO->itemMode;
        if (wrSysGO->mode == 2) {
            int t = itemType;
            if (t == 2 || t == 3 || t == 6 || t == 7 || t == 4 || t == 5 || t == 10 || t == 8)
                mode = 2;
            else if (t == 0x31)
                mode = 0;
        }
        if (wrSysGO->mode == 3 && itemType == 0x3C)
            mode = 0;
        if (itemType >= 0x34 && itemType <= 0x38)
            mode = 0;
        if (mode == 2) {
            gsSysServer->SendMsgClient(*(u32 *)((u8 *)by + 0x8C), this, 0x40E, (msgDATA *)&coop, 0x20000);
            if ((u32)nPickedBy < 0x10) {
                pickedBy[nPickedBy] = by->mpId;
                nPickedBy++;
            }
        } else if (mode == 1) {
            gsSysServer->SendMsgClientAll(this, 0x40E, (msgDATA *)&hide, 0x20000, 0xFFFFFFFD, 0xFFFFFFFD);
            hideTimer = (itemType == 0x32 || itemType == 0x33) ? 75.0f : 20.0f;
        } else {
            msgSystem.SendMsg(this, 0x40E, &hide, 0);
            msgSystem.PostMsg(this, MSG_MSG_TERM, 0);
        }
    } else {
        msgSystem.PostMsg(this, MSG_MSG_TERM, 0);
    }
    entFlags |= 0x280;
    flags &= ~0x30000u;
}

// v12 0x004A9500  (feb 0x0049B370)
// Flame sparks the size of a third of its box's diagonal, and the item sparks moved to it.
void wrITEM::ShowUnhideSfx()
{
    struct { const void *vtbl; int type; m3dV mn, mx; } box = { m3dBOX_vftable, 1 };
    itemITEM::ShowUnhideSfx();
    m3dV p;
    inst->GetPos(&p);
    inst->GetBBox((m3dBOX *)&box);
    wrsfxFLAME_SPARKS *f = 
        new wrsfxFLAME_SPARKS(m3dSqrt((box.mn.x - box.mx.x) * (box.mn.x - box.mx.x) +
                                                         (box.mn.y - box.mx.y) * (box.mn.y - box.mx.y) +
                                                         (box.mn.z - box.mx.z) * (box.mn.z - box.mx.z)) * 0.33333334f);
    if (f) {
        f->SetPosDirSpeed(&p, &m3dVUnitY, 0);
        f->StartEmission();
        f->flags |= 0x200;
    }
    if (sparks) {
        m3dV q;
        inst->GetPos(&q);
        if (itemType == 0x30)
            q.y = q.y + 0.6f;
        sparks->SetPosDirSpeed(&q, 0, 0);
    }
}

// v12 0x004A96A0  (feb 0x0049B4E0)
BOOL32 wrITEM::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 3) {
        ProcessDESTROY_ENT((entENTITY *)data);
    } else if (id == 0x3FA) {                           // 1.2: on the floor again
        cdtREFINE ref(2, inst);
        m3dV p = m3dVZero;                              // (v12 passes a local it never sets) // ?
        gsScenePtr->FloorPlace(&p, 50.0f, &p, 1.0f, 0, 0, &ref, &floor);
        entFlags |= 0x280;
        flags &= ~0x10000u;
    }
    return itemITEM::ProcessMsg(id, data, from, ret);
}

// v12 0x004A9770  (feb 0x0049B520)
void wrITEM::ProcessMP_NOTIFY(void *ev)
{
    itemITEM::ProcessMP_NOTIFY(ev);
    if (((wrITEM_NOTIFY *)ev)->type == 5)               // COOP: this player took it -- hidden for him
        flags |= ITEMF_FORCE_HIDDEN;
}

// v12 0x004A97A0  (feb 0x0049B550)
// A player leaving the game: out of the list of those that took it.
void wrITEM::ProcessDESTROY_ENT(entENTITY *e)
{
    if (e->classId != 0x504C5952 || !(entFlags & 4))
        return;
    int i;
    for (i = 0; i < nPickedBy && pickedBy[i] != e->mpId; i++)
        ;
    if (i >= nPickedBy)
        return;
    for (; i < nPickedBy - 1; i++)
        pickedBy[i] = pickedBy[i + 1];
    nPickedBy--;
}

// v12 0x004A9820  (feb 0x0049B5D0)
// On a server (mode 2 / 4): the Treasure Hunt treasure's physics (not rebuilt) and its pick delay; then itemITEM's.
void wrITEM::ProcessFRAME()
{
    if (gsSysMP->mode == 2 || gsSysMP->mode == 4) {
        // (not rebuilt: flags 0x10000 -- the thrown treasure follows its physMATH; landed, flags 0x20000 and a timer
        // to wrGO_SYSTEM::SelectMP_TreasureDefaultPos)
        m3dUpdateTimeField(&pickDelay, gsElapsedTime);
    }
    itemITEM::ProcessFRAME();
}

// v12 0x004A9D40  (feb 0x0049BB10)
// "sfx_item": sparks round it (the width of its box, bigger for some kinds) attached to it; else itemITEM's entity.
BOOL32 wrITEM::CreateSfx(char *name)
{
    if (strcmp(name, "sfx_item"))
        return itemITEM::CreateSfx(name);
    struct { const void *vtbl; int type; m3dV mn, mx; } box = { m3dBOX_vftable, 1 };
    sfx = 0;
    m3dV p;
    inst->GetPos(&p);
    inst->GetBBox((m3dBOX *)&box);
    p.y = p.y + height;
    float size = (box.mx.x - box.mn.x) * 0.6666667f;
    if (itemType == 0x2F || itemType == 0x2D)
        size = size * 0.6666667f;
    else if (itemType == 0x28)
        size = size * 3.0f;
    else if (itemType == 0x2C)
        size = size * 1.5f;
    else if (itemType == 0x17)
        size = size + size;
    else if (itemType == 0x30)
        size = size * 0.7f;
    sparks = new wrITEM_SPARKS(size);
    if (!sparks)
        return 0;
    sparks->SetPosDirSpeed(&p, 0, 0);
    sparks->StartEmission();
    partAttachInst(sparks, inst);
    return 1;
}

// v12 0x004A9EA0  (feb 0x0049BC60)
void wrITEM::DestroySfx()
{
    if (sparks) {
        wrVCall<void *>(sparks, 0, 1u);                 // its deleting dtor (out of the emitter list)
        sparks = 0;
        return;
    }
    itemITEM::DestroySfx();
}

// v12 0x004A9ED0  (feb 0x0049BC90)
void wrITEM::ShowSfx(int on)
{
    if (!sparks) {
        itemITEM::ShowSfx(on);
        return;
    }
    if (on)
        sparks->StartEmission();
    else
        sparks->StopEmission();
}

// v12 0x004A9F00  (feb 0x0049BCC0)
entENTITY *__fastcall wrITEM::Create(animINST *)
{
    wrITEM *e = new wrITEM;                             // operator new(0x203), entENTITY ctor
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
    e->nPickedBy = 0;
    e->phys = 0;
    e->landTimer = 0;
    e->sparks = 0;
    return e;
}

// v12 0x004A9B70  (feb 0x0049B940)
// Slow sparks round the item: 50 a second living 0.4 s, any direction, growing to size * 0.4 (at least 0.5) and
// back, born size * 1.5 out (size in), turning.
wrITEM_SPARKS::wrITEM_SPARKS(float size) : partEMITTER_PHYS(50.0f, 0.5f)
{
    SetTimeLifePart(0.4f, 0.1f);
    float s = size < 0.5f ? 0.5f : size;
    m3dSPL_LINEAR1D *spl = wrItemSpl(3);
    float s0 = s * 0.0f;
    spl->SetKp(0, 0.0f, s0);
    s = s * 0.4f;
    spl->SetKp(1, 0.5f, s);
    spl->SetKp(2, 1.0f, s0);
    SetSize(s, 0.0f, spl, 1.0f);
    SetColor(0xFFFFFFFF, 0, 0);
    SetTexture(wrITEM::pTexSparks);
    SetTextureRotAnim(180.5f, 179.5f, 0.0f, 90.0f);
    flags |= 4;
    cone = 180.0f;
    coneSpl = 0;
    spreadSpl = 0;
    spread = size * 1.5f;
    spreadMin = size;
    speedASpl = 0;
    speedA = size * 0.2f;
    speedARand = size * 0.1f;
    flags |= 1;
    SetDegFarParams(10.0f, 25.0f, 3.5f, 0.1f);
    flags |= 0x8000000;
}
