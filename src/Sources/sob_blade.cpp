// sob_blade.cpp -- the swinging blades (Will Rock 1.2, v12.exe; feb obj sob_blade.obj); also the scene objects' init
//
// A "dynamic" that kills: while it is deadly (the [SOB_BLADE] trackObj inside killRangeMin..killRangeMax, moving down its
// axis; without trackObj from action frame bit 1 to bit 2) everything near it takes 5000 damage: inside the killArea
// object's outline or, with none, within a metre of its plane. TOGGLE stops it at the end of the sequence (action frame
// bit 0x10, frame 1) and starts it again.

#include "game.h"
#include "../gs/gs_mp_server.h"

enum {
    BLADEF_DEADLY  = 1,
    BLADEF_STOPPING = 2,                                // to BLADEF_STOPPED at action frame bit 0x10
    BLADEF_STOPPED = 4,
};

#pragma pack(push, 1)
struct bladeSYNC : msgDATA {                            // the sync block (GET / READ_SYNC_DATA), v12 0x0064F378, size 0x10
    u32   flags;                                        // +0x04
    int   seq;                                          // +0x08
    float time;                                         // +0x0C
};
struct bladeNOTIFY : msgDATA {                          // GS_MSG_MP_NOTIFY 0x40E
    short type;                                         // +0x04 -100 sync block, 0 flags, 1 sequence's time
    union {
        bladeSYNC *sync;                                // +0x06 (type -100)
        u32 flags;                                      // +0x06 (type 0)
        float time;                                     // +0x06 (type 1)
    };
};
#pragma pack(pop)
static bladeSYNC bladeSync;
static int bladeSyncInited;                             // v12 0x0064F374 bit 0

static void bladeSyncInit()
{
    if (!(bladeSyncInited & 1)) {
        bladeSyncInited |= 1;
        bladeSync.packId = 0;
        bladeSync.tag = 0x40;
        bladeSync.size = 0x10;
        bladeSync.flags = 0;
        bladeSync.seq = -1;
    }
}

// v12 0x00486560  (feb 0x0047C090)
// Not rebuilt: wrsobInitGeyser (v12 0x00487200),
// wrsobInitSpikes (v12 0x004894C0).
int __fastcall wrsobInit()
{
    if (!entRegisterClass((char *)"sob_blade", 0x2444594E, 0x28, wrsobBLADE::Create, 0, 1))
        return 0;
    if (!wrsobInitNiche())
        return 0;
    if (!wrsobInitKatapult())
        return 0;
    if (!wrsobInitTree())
        return 0;
    return wrsobInitTradingAltar() != 0;
}

// v12 0x00486600  (feb 0x0047C130)
// No base call.
BOOL32 wrsobBLADE::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    dmgGENERAL dmg;
    if (id == 1000) {
        if (bladeFlags & BLADEF_STOPPED) {
            inst->ClearActionFrame();
            return 0;
        }
        if (inst->AdvanceTime(gsElapsedTime, 0, 0) && (entFlags & 4)) {
            bladeNOTIFY m;
            m.packId = 0;
            m.tag = 0x40;
            m.size = 0xA;
            m.type = 1;
            m.time = inst->seqTime;
            SendMP_Server_2_ClientAll(&m, 0x10000);
        }
        if (trackObj) {
            inst->Validate(4);
            m3dV p;
            trackObj->GetOrigin(&p);
            float v = (&p.x)[axis];
            if (killMin - 1e-6f < v && killMax + 1e-6f > v && (&trackPrev.x)[axis] >= v) {
                if (!(bladeFlags & BLADEF_DEADLY))
                    pendingEvents |= 4;
                bladeFlags |= BLADEF_DEADLY;
            } else {
                if (bladeFlags & BLADEF_DEADLY)
                    pendingEvents |= 8;
                bladeFlags &= ~(u32)BLADEF_DEADLY;
            }
            trackPrev = p;
        } else {
            if (inst->actionFrames & 1) {
                bladeFlags |= BLADEF_DEADLY;
                pendingEvents |= 4;
            }
            if (inst->actionFrames & 2) {
                bladeFlags &= ~(u32)BLADEF_DEADLY;
                pendingEvents |= 8;
            }
        }
        if ((bladeFlags & BLADEF_STOPPING) && (inst->actionFrames & 0x10))
            bladeFlags = (bladeFlags & ~(u32)BLADEF_STOPPING) | BLADEF_STOPPED;
        if ((inst->flags & 3) && gsSysMP->mode == 1)
            return 0;
        if (!(entFlags & 4) || !(bladeFlags & BLADEF_DEADLY))
            return 0;
        m3dV pos;
        inst->GetPos(&pos);
        dmg.pos = pos;
        dmg.type = 0;
        dmg.subtype = 3;
        dmg.amount = 5000.0f;
        animINST *list[0x14];
        int n;
        if (killArea) {
            m3dV d = { killArea->bbox.max[0] - killArea->bbox.min[0], killArea->bbox.max[1] - killArea->bbox.min[1],
                       killArea->bbox.max[2] - killArea->bbox.min[2] };
            dmgCollectInst(&pos, m3dSqrt(d.x * d.x + d.y * d.y + d.z * d.z), list, &n, 0x14);
        } else {
            inst->Validate(0x40);
            dmgCollectInst(&pos, 5.0f, list, &n, 0x14);
        }
        for (int i = 0; i < n; i++) {
            m3dV q;
            list[i]->GetPos(&q);
            dmg.pos.y = q.y;
            int hit;
            if (killArea) {
                hit = killArea->IsBelongPointXZ(&q);
            } else {
                const float *m = inst->matrW2L;
                float l = q.x * m[axis] + q.y * m[axis + 4] + q.z * m[axis + 8] + m[axis + 12];
                hit = fabsf(l) < 1.0f;
            }
            if (hit)
                msgSystem.SendMsg(list[i]->ent, 0x4C6, &dmg, this);
        }
    } else if (id == 1) {
        char s[0x40];
        if (inst->GetStr((char *)"SOB_BLADE", (char *)"trackObj", s, 0x40)) {
            trackObj = objFindNameN(inst->root, s, (int)strlen(s));
            if (!trackObj)
                apLogErr((char *)"SOB_BLADE: Cannot find object %s", s);
            inst->Validate(4);
            trackObj->GetOrigin(&trackPrev);
            inst->GetFloat((char *)"SOB_BLADE", (char *)"killRangeMin", &killMin);
            inst->GetFloat((char *)"SOB_BLADE", (char *)"killRangeMax", &killMax);
        }
        if (inst->GetStr((char *)"SOB_BLADE", (char *)"killArea", s, 0x40)) {
            objOBJ *o = objFindNameN(inst->root, s, (int)strlen(s));
            if (!o)
                o = objFindNameN(gsScenePtr->root, s, (int)strlen(s));
            if (!o) {
                apLogErr((char *)"SOB_BLADE: Cannot find object %s", s);
            } else {
                killArea = new scnDOMAIN;
                if (!killArea)
                    return 0;
                killArea->obj = o;
                if (!killArea->Init())
                    return 0;
                killArea->CalcBBox();
            }
        }
        int b;
        axis = 2;
        if ((inst->GetBool((char *)"SOB_BLADE", (char *)"isForwAxisX", &b) && b)
            || (inst->GetBool((char *)"SOB_BLADE", (char *)"isUseAxisX", &b) && b)) {
            axis = 0;
        } else if ((inst->GetBool((char *)"SOB_BLADE", (char *)"isForwAxisY", &b) && b)
                   || (inst->GetBool((char *)"SOB_BLADE", (char *)"isUseAxisY", &b) && b)) {
            axis = 1;
        }
        inst->tpl->seq[0].actionFrame[4] = 1;
        StartAnim(0, 0, 1);
        gsSysServer->DeclSyncable(this);
    } else if (id == 0x3F8) {
        if (((apNAME *)((u8 *)data + 8))->IsName((char *)"TOGGLE")) {
            if (bladeFlags & BLADEF_STOPPED)
                bladeFlags &= ~(u32)BLADEF_STOPPED;
            else
                bladeFlags |= BLADEF_STOPPING;
            if ((entFlags & 4) && mpId != -1) {
                bladeNOTIFY m;
                m.packId = 0;
                m.tag = 0x40;
                m.size = 0xA;
                m.type = 0;
                m.flags = bladeFlags;
                SendMP_Server_2_ClientAll(&m, 0x20000);
            }
        }
    } else if (id == 0x40E) {
        bladeNOTIFY *m = (bladeNOTIFY *)data;
        if (m->type == -100) {
            bladeFlags = m->sync->flags;
            inst->SetAnimSeq(m->sync->seq, m->sync->time);
        } else if (m->type == 0) {
            bladeFlags = m->flags;
        } else if (m->type == 1) {
            inst->SetAnimSeq(inst->seq, m->time);
        }
    }
    return 0;
}

// v12 0x00486F30  (feb 0x0047C9C0)
void wrsobBLADE::ProcessMP_GET_SYNC_DATA(void *d)
{
    msgDATA **out = (msgDATA **)d;
    if (!*out) {
        bladeSyncInit();
        *out = &bladeSync;
    }
    bladeSYNC *s = (bladeSYNC *)*out;
    s->flags = bladeFlags;
    s->seq = inst->seq;
    s->time = inst->seqTime;
    entENTITY::ProcessMP_GET_SYNC_DATA(d);
}

// v12 0x005567C0: the button's (the same blocks)
void wrsobBLADE::ProcessMP_WRITE_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    ((bttnBUTTON *)this)->bttnBUTTON::ProcessMP_WRITE_SYNC_DATA(d);
}

// v12 0x00486FC0  (feb 0x0047CA50)
void wrsobBLADE::ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *d)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    bladeSYNC *s = (bladeSYNC *)d->data;
    if (!s) {
        bladeSyncInit();
        s = &bladeSync;
        d->data = s;
        s->packId = 0;
        s->tag = 0x40;
        s->size = 0x10;
        s->flags = 0;
        s->seq = -1;
    }
    for (int ok = d->file->ReadChunk(&c); ok && c.id != 1; ok = d->file->ReadChunk(&c)) {
        void *dst = 0;
        if (c.id == 0xF0)
            dst = &s->flags;
        else if (c.id == 0xF1)
            dst = &s->seq;
        else if (c.id == 0xF2)
            dst = &s->time;
        if (dst)
            d->file->ReadData(dst, 4, 4);
    }
    entENTITY::ProcessMP_READ_SYNC_DATA(d);
}

// v12 0x00487100  (feb 0x0047CB90)
entENTITY *__fastcall wrsobBLADE::Create(animINST *i)
{
    void *mem = operator new(0x1BF);
    if (!mem)
        return 0;
    wrsobBLADE *b = new (mem) wrsobBLADE;
    b->catchUpRate = 1.0f;                              // (the dynamic's Create, inline)
    b->timeDebt = 0;
    b->dynFlags = 0;
    b->unk177[0] = b->unk177[1] = b->unk177[2] = b->unk177[3] = 0;
    b->unk187 = m3dVZero;
    b->unk193 = 0;
    b->bladeFlags = 0;
    b->killMin = 0;
    b->killMax = 0;
    b->killArea = 0;
    b->trackObj = 0;
    b->axis = 2;
    return b;
}

// v12 0x004871C0  (feb 0x0047CC40)
wrsobBLADE::~wrsobBLADE()
{
    delete killArea;
}
