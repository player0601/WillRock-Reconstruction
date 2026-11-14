// ai_ctrl_MP.cpp -- an NPC over the network and in saves (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_ctrl_MP.obj)

#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"
#include "../gs/gs_mp_misc.h"

#pragma pack(push, 1)
struct aiSYNC_DATA : msgDATA {                          // v12 0x0054D740, size 0x5C
    float health;                                       // +0x04
    int   seq;                                          // +0x08
    float seqTime;                                      // +0x0C
    int   domainId;                                     // +0x10 the spawn domain's MP id
    int   spawnParam;                                   // +0x14
    u32   flags;                                        // +0x18
    int   layer[3];                                     // +0x1C
    m3dV  physA;                                        // +0x28 the passive physics: +0xB4
    m3dV  physPos;                                      // +0x34
    m3dV  physB;                                        // +0x40 +0x48
    int   crumbID;                                      // +0x4C
    m3dV  goalPos;                                      // +0x50
};
struct aiNOTIFY : msgDATA {                             // GS_MSG_MP_NOTIFY 0x40E
    short type;                                         // +0x04 -100 the sync block, 0 flags, 1 health, 2 death
    union {
        aiSYNC_DATA *sync;                              // +0x06
        u32   flags;
        float health;
        void *killer;
    };
    float killerId;                                     // +0x0A
    int   weapon;                                       // +0x0E
};
#pragma pack(pop)

static aiSYNC_DATA aiSyncData;
static int aiSyncInited;                                // v12 0x009A8BC0

static void aiSyncInit()
{
    if (aiSyncInited & 1)
        return;
    aiSyncInited |= 1;
    memset(&aiSyncData, 0, sizeof(aiSyncData));
    aiSyncData.tag = 0x40;
    aiSyncData.size = 0x5C;
    aiSyncData.seq = -1;
    aiSyncData.domainId = -1;
}

// v12 0x0054CB30  (feb 0x00531830)
void aiNPC::NotifyAIState()
{
    aiNOTIFY m;
    m.packId = 0;
    m.tag = 0x40;
    m.size = 0xA;
    m.type = 0;
    m.flags = aiFlags;
    gsSysServer->SendMsgClientAll(this, 0x40E, &m, 0x20000, -1, -3);
}

// v12 0x0054CB90  (feb 0x00531890)
void aiNPC::ProcessMP_GET_SYNC_DATA(void *d)
{
    msgDATA **out = (msgDATA **)d;
    if (!*out) {
        aiSyncInit();
        *out = &aiSyncData;
    }
    aiSYNC_DATA *s = (aiSYNC_DATA *)*out;
    s->health = health;
    s->seq = inst->seq;
    s->seqTime = inst->seqTime;
    s->domainId = spawnDomain ? WR_FIELD(int, spawnDomain, 0x8C) : -1;
    s->spawnParam = spawnParam;
    s->flags = aiFlags;
    s->layer[0] = GetLayer1State();
    s->layer[1] = GetLayer2State();
    s->layer[2] = GetLayer3State();
    if (aiFlags & 0x100) {
        u8 *p = (u8 *)passivePhys;
        s->physA = *(m3dV *)(p + 0xB4);
        passivePhys->GetPos(&s->physPos);
        s->physB = *(m3dV *)(p + 0x48);
    }
    s->crumbID = crumbID;
    s->goalPos = goalPos;
    if (s->seq < 0 || s->seq >= (inst->tpl ? inst->tpl->nSeq : 0))
        s->seq = 0;
    entENTITY::ProcessMP_GET_SYNC_DATA(d);
}

static void aiWriteChunk(fioFILE *f, int id, void *p, int n, int el)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    f->WriteChunk(id, &c);
    f->WriteData(p, n, el);
    f->UpdateChunk(&c);
}

// v12 0x0054CDA0  (feb 0x00531AA0)
void aiNPC::ProcessMP_WRITE_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    aiSYNC_DATA *s = (aiSYNC_DATA *)d->data;
    if (s->seq < 0 || s->seq >= (inst->tpl ? inst->tpl->nSeq : 0))
        s->seq = 0;
    fioFILE *f = d->file;
    aiWriteChunk(f, 0xF0, &s->health, 4, 4);
    aiWriteChunk(f, 0xF1, &s->seq, 4, 4);
    aiWriteChunk(f, 0xF2, &s->seqTime, 4, 4);
    aiWriteChunk(f, 0xF3, &s->domainId, 4, 4);
    aiWriteChunk(f, 0xF4, &s->spawnParam, 4, 4);
    aiWriteChunk(f, 0xF5, &s->flags, 0x10, 4);
    if (aiFlags & 0x100) {
        aiWriteChunk(f, 0xF6, &s->physA, 0xC, 4);
        aiWriteChunk(f, 0xF7, &s->physPos, 0xC, 4);
        aiWriteChunk(f, 0xF8, &s->physB, 0xC, 4);
    }
    aiWriteChunk(f, 0xF9, &s->crumbID, 4, 4);
    aiWriteChunk(f, 0xFA, &goalPos, 0xC, 4);            // (the entity's, not the block's: as the game)
    f->WriteLastChunk();
    entENTITY::ProcessMP_WRITE_SYNC_DATA(d);
}

// v12 0x0054D050  (feb 0x00531D50)
void aiNPC::ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    aiSYNC_DATA *s = (aiSYNC_DATA *)d->data;
    if (!s) {
        aiSyncInit();
        s = &aiSyncData;
        memset(s, 0, sizeof(*s));
        s->tag = 0x40;
        s->size = 0x5C;
        s->seq = -1;
        s->domainId = -1;
        d->data = s;
    }
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    for (int ok = d->file->ReadChunk(&c); ok && c.id != 1; ok = d->file->ReadChunk(&c)) {
        void *dst = 0;
        int n = 4;
        switch (c.id) {
        case 0xF0: dst = &s->health; break;
        case 0xF1:
            d->file->ReadData(&s->seq, 4, 4);
            if (s->seq < 0 || s->seq >= (inst->tpl ? inst->tpl->nSeq : 0))
                s->seq = 0;
            continue;
        case 0xF2: dst = &s->seqTime; break;
        case 0xF3: dst = &s->domainId; break;
        case 0xF4: dst = &s->spawnParam; break;
        case 0xF5: dst = &s->flags; n = 0x10; break;
        case 0xF6: dst = &s->physA; n = 0xC; break;
        case 0xF7: dst = &s->physPos; n = 0xC; break;
        case 0xF8: dst = &s->physB; n = 0xC; break;
        case 0xF9: dst = &s->crumbID; break;
        case 0xFA: dst = &s->goalPos; n = 0xC; break;
        }
        if (dst)
            d->file->ReadData(dst, n, 4);
    }
    if (s->spawnParam < 0)
        s->spawnParam = 0;
    entENTITY::ProcessMP_READ_SYNC_DATA(d);
}

// v12 0x0054D270  (feb 0x00531F70)
// The entity's snapshot (0x52 bytes) with the sequence, the one wanted and the play rate.
int aiNPC::GetMP_UPD_STATE(void **msg)
{
    static u8 block[0x52];
    static int inited;
    if (!inited) {
        inited = 1;
        memset(block, 0, sizeof(block));                // (gsMSG_MP_UPD_STATE's ctor, v12 0x005151B0)
        block[1] = 0x40;
        *(u16 *)(block + 2) = 0x52;
        block[0x4F] = 0xFF;
        block[0x50] = 0xFF;
        block[0x51] = 0;
    }
    if (!*msg)
        *msg = block;
    u8 *b = (u8 *)*msg;
    int r = entENTITY::GetMP_UPD_STATE(msg);
    b[0x4F] = (u8)inst->seq;
    b[0x50] = (u8)EvalAnimGoalSeq();
    b[0x51] = m3dFloatToByte(animRate2, 0.0f, 5.0f);
    return r;
}

// v12 0x0054D340  (feb 0x00532010)
void aiNPC::UpdateOnClientOnly()
{
    if (aiFlags & 0x10000)
        return;
    Animate();
    if (aiFlags & 0x200000)
        UpdateDead();
}

// v12 0x0054D370  (feb 0x00532040)
void aiNPC::NotifyHealthChange(dmgDAMAGE *d)
{
    if (!(entFlags & 4))
        return;
    struct { msgDATA h; short type; float hp; u32 src; int weapon; } __attribute__((packed)) m;
    m.h.packId = 0;
    m.h.tag = 0x40;
    m.h.size = 0x12;
    m.type = 1;
    m.hp = health;
    m.src = d->sourceId;
    m.weapon = d->type == 1 ? ((dmgWEAPON *)d)->weaponType : -1;
    gsSysServer->SendMsgClientAll(this, 0x40E, &m.h, 0x10000, -1, -3);
}

// v12 0x0054D3F0  (feb 0x005320C0)
void aiNPC::NotifyDeath(dmgDAMAGE *d)
{
    if (!(entFlags & 4))
        return;
    struct { msgDATA h; short type; dmgDAMAGE *dmg; } __attribute__((packed)) m;
    m.h.packId = 0;
    m.h.tag = 0x40;
    m.h.size = 0xA;
    m.type = 2;
    m.dmg = d;
    gsSysServer->SendMsgClientAll(this, 0x40E, &m.h, 0x20000, -1, -3);
}

// v12 0x0054D450  (feb 0x00532130)
void aiNPC::ProcessMP_NOTIFY(gsEVENT *ev)
{
    u8 *b = (u8 *)ev;
    short type = *(short *)(b + 4);
    if (type == -100) {
        aiSYNC_DATA *s = *(aiSYNC_DATA **)(b + 6);
        goalSeq = s->seq;
        goalSeq2 = s->seq;
        inst->SetAnimSeq(s->seq, s->seqTime);
        if (s->domainId != -1) {
            spawnDomain = (aiDOMAIN_SPAWN *)gsSysMP->FindMPID(s->domainId, 0);
            spawnParam = s->spawnParam;
        }
        aiFlags = s->flags;
        SetLayer1State(s->layer[0]);
        SetLayer2State(s->layer[1]);
        SetLayer3State(s->layer[2]);
        if (aiFlags & 0x10000)
            Deactivate();
        else
            Activate(1);
        health = s->health;
        if ((aiFlags & 0x100) && passivePhys) {
            u8 *p = (u8 *)passivePhys;
            *(m3dV *)(p + 0xB4) = s->physA;
            passivePhys->SetPos(&s->physPos);
            *(m3dV *)(p + 0x48) = s->physB;
        }
        crumbID = s->crumbID;
        goalPos = s->goalPos;
    } else if (type == 0) {
        u32 f = *(u32 *)(b + 6);
        if (!(aiFlags & 0x10000) && (f & 0x10000)) {
            Deactivate();
        } else if ((aiFlags & 0x10000) && !(f & 0x10000)) {
            Activate(1);
        }
        aiFlags = f;
    } else if (type == 1) {
        health = *(float *)(b + 6);
    } else if (type == 2) {
        StartDeath(*(dmgDAMAGE **)(b + 6));
    } else if (type == -99) {
        health = *(float *)(b + 0xA);
    }
}

// v12 0x0054D6A0  NEW in 1.2
// The entity's snapshot; then the sequences and the rate. A new sequence starts over (aiFlags 0x4000000).
int aiNPC::ProcessMP_UPD_STATE(void *msg)
{
    u8 *b = (u8 *)msg;
    int r = entENTITY::ProcessMP_UPD_STATE(msg);
    goalSeq = (signed char)b[0x4F];
    goalSeq2 = (signed char)b[0x50];
    animRate2 = m3dByteToFloat(b[0x51], 0.0f, 5.0f);
    if ((aiFlags & 0x2000000) || goalSeq != inst->seq)
        aiFlags |= 0x4000000;
    aiFlags &= ~0x2000000u;
    return r;
}
