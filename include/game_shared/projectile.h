// projectile.h -- pjlPJL<SFX>, the thrown things of the enemies and of some weapons (Will Rock 1.2, v12.exe)
//
// A physPHYS that flies (physMATH), hits (GS_MSG_CDT 0x3F9), tells every machine of the hit (GS_MSG_MP_NOTIFY
// 0x40E type 0, pjlEV_CDT), deals its damage (explosion round the hit, or to the instance hit) and goes (at once,
// or after termDelay). SFX is the effect riding on it (a trail...), a member right after physPHYS: its size moves
// the fields after it, so each instantiation has its own copy of the code (identical ones folded in the exe: the
// collision handler v12 0x0040F800, Terminate v12 0x0044C830...).
//
// The SFX type provides: int StartSFX(physPHYS *, pjlPJL_INIT *); void UpdateSFX(physPHYS *); void StopSFX(physPHYS *).
#pragma once
#include "../engine_core/physics.h"

#pragma pack(push, 1)
struct pjlPJL_INIT {                                    // INIT data of a projectile
    physINIT phys;                                      // +0x00 (0x38)
    float    damage;                                    // +0x38
    float    radius;                                    // +0x3C the explosion's (explosive)
    int      weaponType;                                // +0x40 (dmgDispatchExplosDamage)
    int      isGeneral;                                 // +0x44
    u32      ownerId;                                   // +0x48 the shooter's MP id (-1 none): not hit by its own
    float    termDelay;                                 // +0x4C after a hit (-1: at once)
    u8       explosive;                                 // +0x50 1: damage all round the hit
};                                                      // 0x51
struct pjlEV_CDT : msgDATA {                            // GS_MSG_MP_NOTIFY type 0: a hit (0x26 bytes)
    short type;                                         // +0x04 0
    int   kind;                                         // +0x06 1 the level / an inert thing, 0 a pass-through entity,
                                                        //       2 a player, 3 an NPC, 4 a dead NPC (unk98 != 0), 5 other
    m3dV  pos;                                          // +0x0A
    m3dV  normal;                                       // +0x16 the face's
    u32   color;                                        // +0x22 the light there (gsEffEnvirColor)
};
struct pjlEV_LAYER : msgDATA {                          // GS_MSG_MP_NOTIFY type 7: the "layer" state (0x0A bytes)
    short type;                                         // +0x04 7
    int   state;                                        // +0x06
};
struct pjlHIT_MSG : msgDATA {                           // msg 0x4D3: a hit found by the INIT (born inside a wall)
    u32  mpId;                                          // +0x04 of the entity hit (-1: the level)
    u32  ci0;                                           // +0x08 the cdtINFO, its words 0, 2, 3, the point, 7
    u32  ci8;                                           // +0x0C
    u32  ciC;                                           // +0x10
    m3dV point;                                         // +0x14
    u32  ci1C;                                          // +0x20
};
#pragma pack(pop)

enum pjlFLAGS {                                         // pjlPJL::pjlFlags
    PJLF_HIT        = 0x01,                             // it has hit (stuck, or going)
    PJLF_FLYING     = 0x02,                             // (INIT; cleared by the hit)
    PJLF_SND_STATIC = 0x04,                             // hit the level: sound 1
    PJLF_SND_ENTITY = 0x08,                             // hit something else: sound 2
    PJLF_SND_LAUNCH = 0x10,                             // just thrown: sound 3
};

u32 __fastcall gsEffEnvirColor(cdtINFO *ci, m3dCOLOR *col, m3dCOLOR *dyn);              // v12 0x005130E0 (gs:gs_eff)
float __fastcall entNearestPlayerDist(entENTITY *self, float yOff, m3dV *out, entENTITY **who);  // v12 0x00547450

#pragma pack(push, 1)
template <class SFX> class pjlPJL : public physPHYS {
public:
    SFX        sfx;                                     // +0x16F
    float      damage;
    float      radius;
    int        weaponType;
    int        isGeneral;
    u32        ownerId;
    float      termDelay;
    u8         explosive;
    u32        pjlFlags;                                // pjlFLAGS
    sncCHANNEL *sndChannel;

    pjlPJL()
    {
        damage = -1.0f;
        radius = 0;
        weaponType = -1;
        isGeneral = -1;
        ownerId = 0xFFFFFFFF;
        termDelay = -1.0f;
        pjlFlags = 0;
        sndChannel = 0;
    }

    // (v12 0x0044A8C0 for the arrow) msg 0x4D3: the hit the INIT found, as a collision
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret)
    {
        physPHYS::ProcessMsg(id, data, from, ret);
        if (id != 0x4D3)
            return 0;
        pjlHIT_MSG *m = (pjlHIT_MSG *)data;
        cdtINFO ci;
        memset(&ci, 0, sizeof(ci));
        u32 *w = (u32 *)&ci;
        w[0] = 2;
        if (m->mpId != 0xFFFFFFFF) {
            entIACTIVE_OBJ *e = gsSysMP->FindMPID((int)m->mpId, 0);
            if (!e || !((entENTITY *)e)->inst)
                return 0;
            w[1] = (u32)(uintptr_t)((entENTITY *)e)->inst;
        }
        w[0] = m->ci0;
        w[2] = m->ci8;
        w[3] = m->ciC;
        *(m3dV *)&w[4] = m->point;
        w[7] = m->ci1C;
        math->init.maxHits = 1;
        math->nHits++;
        msgSystem.SendMsg(this, 0x3F9, &ci, this);
        return 0;
    }

    // (v12 0x0043F1A0 feb, gefest's) the damage, the explosion, the owner kept out of the collision, the effect
    BOOL32 ProcessINIT(physINIT *p)
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
        if (WR_FIELD(int, gsSysMP, 0x28) != 4)
            sfx.StartSFX(this, in);
        return 1;
    }

    // (v12 0x0044A860) the effect off
    void ProcessTERM()
    {
        sfx.StopSFX(this);
        physPHYS::ProcessTERM();
    }

    // (v12 0x0044A880) flies (unless a cut-scene stops the action), the effect follows, the sounds
    void ProcessFRAME()
    {
        if ((gsSysCinema->state & 1) && gsSysCinema->IsSuspendAction())
            return;
        physPHYS::ProcessFRAME();
        sfx.UpdateSFX(this);
        PlaySounds();
    }

    // (v12 0x0040F800, shared) on the server (or a local-only one): what was hit, told to all; the hit; the damage
    BOOL32 ProcessCDT(cdtINFO *ci)
    {
        pjlEV_CDT ev;
        ev.packId = 0;
        ev.tag = 0x40;
        ev.size = 0x26;
        ev.type = 0;
        ev.kind = 1;
        ev.normal = m3dVUnitY;
        ev.color = 0;
        if (!(entFlags & 4) && mpId != -1)
            return 0;
        animINST *hit = *(animINST **)((u8 *)ci + 4);
        entENTITY *he = hit ? hit->ent : 0;
        if (hit && he) {
            if (he->unkBC & 1)
                ev.kind = 0;
            else if (he->classId == 0x504C5952)         // 'PLYR'
                ev.kind = 2;
            else if (he->classId == 0x244E5043)         // '$NPC'
                ev.kind = he->unk98 ? 4 : 3;
            else
                ev.kind = (hit->flags & 0x48) ? 1 : 5;
        }
        ev.color = gsEffEnvirColor(ci, 0, 0);
        ev.pos = *(m3dV *)((u8 *)ci + 0x10);
        gsScenePtr->GetCDTFace(ci, 0, &ev.normal);
        gsSysServer->SendMsgClientAll(this, 0x40E, &ev, 0xA0000, 0xFFFFFFFF, 0xFFFFFFFD);
        ProcessEV_CDT(&ev);
        DispatchDamage(ci);
        return 0;
    }

    // [+0x7C] (v12 0x0045BB30 for the arrow) the damage: round the hit if explosive, else to the instance hit
    virtual BOOL32 DispatchDamage(cdtINFO *ci)
    {
        m3dV *at = (m3dV *)((u8 *)ci + 0x10);
        if (explosive) {
            dmgDispatchExplosDamage(this, at, damage, radius, isGeneral, weaponType, ci, ownerId, 0);
            return 1;
        }
        animINST *hit = *(animINST **)((u8 *)ci + 4);
        if (!hit)
            return 0;
        dmgDispatchExplosDamageInst(hit, this, at, damage, radius, isGeneral, weaponType, ci, ownerId, 0);
        return 1;
    }

    // [+0x80] (v12 0x00403950: the shared empty one)
    virtual void ShowCollisSFX(pjlEV_CDT *) {}

    // [+0x84] (v12 0x0044C830) the sounds, then away -- now, or after termDelay
    virtual void Terminate()
    {
        PlaySounds();
        if (termDelay == -1.0f)
            msgSystem.PostMsg(this, 2, 0);
        else
            msgSystem.PostTimeMsg(this, 2, this, termDelay);
    }

    // [+0x88] (v12 0x0043D9B0) a hit (here or told): the effect off, the hit's look, its sound, stopped, going
    virtual void ProcessEV_CDT(pjlEV_CDT *ev)
    {
        pjlFlags = (pjlFlags & ~PJLF_FLYING) | PJLF_HIT;
        sfx.StopSFX(this);
        ShowCollisSFX(ev);
        pjlFlags |= ev->kind == 1 ? PJLF_SND_STATIC : PJLF_SND_ENTITY;
        math->vel[0] = math->vel[1] = math->vel[2] = 0;
        math->flags |= 0x10;
        Terminate();
    }

    // [+0x8C] (v12 0x004031F0: the shared "return 1") it has a sound channel
    virtual BOOL32 HasSound() { return 1; }
    // [+0x90] its sounds: 0 flying (looped), 1 the level hit, 2 another hit, 3 the throw
    virtual sncSOUND_DESCR_3D *GetSound(int) { return 0; }

    // [+0x94] (v12 0x00446E80) the sound of the moment: the throw, a hit, else the flight (stopped once it hit)
    virtual void PlaySounds()
    {
        if (!sndChannel) {
            ResetSoundFlags();
            return;
        }
        sncSOUND_DESCR_3D *s;
        if ((pjlFlags & PJLF_SND_LAUNCH) && (s = GetSound(3)) != 0)
            sndChannel->Play((sncSOUND_DESCR *)s, 0);
        else if ((pjlFlags & PJLF_SND_ENTITY) && (s = GetSound(2)) != 0)
            sndChannel->Play((sncSOUND_DESCR *)s, 0);
        else if ((pjlFlags & PJLF_SND_STATIC) && (s = GetSound(1)) != 0)
            sndChannel->Play((sncSOUND_DESCR *)s, 0);
        else if ((s = GetSound(0)) != 0) {
            if (!(pjlFlags & PJLF_HIT))
                sndChannel->Play((sncSOUND_DESCR *)s, 0);
            else if (sndChannel->IsPlay((sncSOUND_DESCR *)s))
                sndChannel->Stop();
        }
        ResetSoundFlags();
    }
    // [+0x98] (v12 0x004035F0)
    virtual void ResetSoundFlags() { pjlFlags &= ~(PJLF_SND_STATIC | PJLF_SND_ENTITY | PJLF_SND_LAUNCH); }
    // [+0x9C] (v12 0x004650B0) the sound channel's distances
    virtual void GetSndDist(float *mn, float *mx) { *mn = 0; *mx = 1.0f; }
    // [+0xA0] / [+0xA4] a state kept the same on every machine (type 7) -- the arrow's wobble
    virtual int  GetLayerAppState() { return 0; }
    virtual void SetLayerAppState(int) {}
};

// pjlNPC_PJL<SFX>: an enemy's: told hits are replayed (type 0), the layer state follows (type 7)
template <class SFX> class pjlNPC_PJL : public pjlPJL<SFX> {
public:
    int   lastLayerState;                               // (arrow +0x198) the layer state last told to the clients
    u32   npcF[4];                                      // (+0x19C) zeroed by Create // ?
    m3dV  npcV;                                         // (+0x1AC) zero // ?
    u32   npcF2;                                        // (+0x1B8) // ?
    pjlNPC_PJL() : lastLayerState(0), npcF2(0) { npcF[0] = npcF[1] = npcF[2] = npcF[3] = 0; npcV = m3dVZero; }

    // (v12 0x0044FF70)
    void ProcessMP_NOTIFY(msgDATA *m)
    {
        physPHYS::ProcessMP_NOTIFY(m);
        if (((pjlEV_CDT *)m)->type == 0) {
            this->math->nHits++;
            this->ProcessEV_CDT((pjlEV_CDT *)m);
        }
        if (((pjlEV_CDT *)m)->type == 7)
            this->SetLayerAppState(((pjlEV_LAYER *)m)->state);
    }
};
#pragma pack(pop)
