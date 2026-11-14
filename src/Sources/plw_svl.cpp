// plw_svl.cpp -- pwpSHOVEL, Will's shovel (weapon 0): melee, no ammo; its hit effects (sparks, smoke, scorch
// mark, parts: the "sfx_shovel" entity pwpSVL_HIT) (Will Rock 1.2, v12.exe; feb obj plw_svl.obj)
//
// Rebuilt from the 1.2 code so far: pwpSVLInit (v12 0x004152D0), Create (0x00416E60), ProcessINIT (0x004154A0),
// ProcessFRAME (0x00415560), Shoot (0x004155A0), ProcessSHOOT (0x0040AC20, the relay 1.2 shares with the bullet
// weapons), ShootBody (0x00415B00). Fields at feb + 0x1C. 1.2 changes:
//   - pwpSVLInit also registers a message packer (pwpSvlPack, v12 0x006416E8); the shoot / reload action frames
//     swapped slots (+0x1B7 shoot 0, +0x1B3 reload -1);
//   - Shoot: the shot message is packed (0x26 bytes) and carries the face / object of a wall hit; an entity
//     outside the 65 degree cone still counts when it is the one under the crosshair (v12 0x0098EAC4); a hit on
//     a non-entity instance cancels the shot; the local side sends itself 0x4C8 (ShootBody) unless it is the server;
//   - ShootBody (feb ProcessSHOOT) credits the hit in the statistics and plays the flesh / stone sound by target.
// Not rebuilt here yet: pwpSVL_HIT and the effect classes (pwpSVL_SCORCHMARK / _PARTS / _SMOKE / _SPARKS).

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../gs/gs_mp_misc.h"

// the swing message's packer (1.2 only, name ours): vtable v12 0x005FAD28, static initialiser v12 0x004152B0
class pwpSVL_PACK : public msgDATA_PACK {
public:
    BOOL32   Pack(fioFILE_MEM *f, msgDATA *d);                  // v12 0x00416B60
    msgDATA *UnPack(fioFILE_MEM *f);                            // v12 0x00416CB0
};
pwpSVL_PACK pwpSvlPack;                                         // v12 0x006416E8
sncSOUND_DESCR_3D pwpSvlSnd[3];                         // v12 0x00641728 swing, body hit, wall hit

// v12 0x004151B0  static initialiser (the sncSOUND_DESCR_3D constructor inline): names, the rest 0
static struct pwpSVL_SND_INIT {
    pwpSVL_SND_INIT()
    {
        static const char *names[3] = { "wpn_shovel_swipe", "wpn_shovel_hit_body", "wpn_shovel_hit_wall" };
        for (int i = 0; i < 3; i++) {
            sncSOUND_DESCR_3D *d = &pwpSvlSnd[i];
            d->name.SetName((char *)names[i]);
            d->flags = 0;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = 0.0f;
            d->maxDist = 0.0f;
            d->vtbl = 0;                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} pwpSvlSndInit;
int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30 (drv:Snd_chn.obj)

struct pwpSVL_SMOKE { static txmTEXTURE *pTexSmoke; };
struct pwpSVL_PARTS { static txmTEXTURE *pTexPart; };
struct pwpSVL_SPARKS { static txmTEXTURE *pTexSparks; };
struct pwpSVL_SCORCHMARK_TEX { static txmTEXTURE *pTexScorchMark; };
txmTEXTURE *pwpSVL_SMOKE::pTexSmoke;                    // v12 0x00641860
txmTEXTURE *pwpSVL_PARTS::pTexPart;                     // v12 0x006416FC
txmTEXTURE *pwpSVL_SPARKS::pTexSparks;                  // v12 0x006416E0
txmTEXTURE *pwpSVL_SCORCHMARK_TEX::pTexScorchMark;      // v12 0x006416E4 (pwpSVL_SCORCHMARK::pTexScorchMark)

entENTITY *__fastcall pwpSVL_HIT_Create(animINST *i);  // v12 0x004153C0 (pwpSVL_HIT::Create) -- not rebuilt

// v12 0x004152D0  (feb 0x00413970)
int __fastcall pwpSVLInit()
{
    if (!entRegisterClass((char *)"pwp_svl", 'PSVL', 0x50, pwpSHOVEL::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"sfx_shovel", 'PSVL', 0x50, pwpSVL_HIT_Create, 0, 1))
        return 0;
    if (!(pwpSVL_SMOKE::pTexSmoke = txmManager->Add((char *)"sfx_mcg_cloud", 0x40003, 1)))
        return 0;
    if (!(pwpSVL_PARTS::pTexPart = txmManager->Add((char *)"sfx_mcg_part", 0x40003, 1)))
        return 0;
    if (!(pwpSVL_SPARKS::pTexSparks = txmManager->Add((char *)"sfx_shovel_sparks", 0x40003, 1)))
        return 0;
    if (!(pwpSVL_SCORCHMARK_TEX::pTexScorchMark = txmManager->Add((char *)"sfx_shovel_scorchmark", 0x40003, 1)))
        return 0;
    if (!sncLinkSndListData(3, pwpSvlSnd))
        return 0;
    msgSystem.RegisterMsgData(&pwpSvlPack);
    return 1;
}

// v12 0x00416E60  (feb 0x00415110)
entENTITY *__fastcall pwpSHOVEL::Create(animINST *)
{
    pwpSHOVEL *w = new pwpSHOVEL;           // operator new(0x1CB), wpnWEAPON ctor, the pwpWEAPON inline part
    if (!w)
        return 0;
    w->wpnIndex = PWP_SHOVEL;
    return w;
}

// v12 0x004154A0  (feb 0x00413B10)
// Rate 25 (the animation limits it), damage 120, the shot on the SHOOT sequence's action frame 0, no ammo use
// (wpnWEAPON flag 4), ammo 1; the model gets vertex normals.
int pwpSHOVEL::ProcessINIT()
{
    animCREATE_DATA cd;                     // (constructed and unused in the game)
    (void)cd;
    if (!pwpWEAPON::ProcessINIT())
        return 0;
    shootActFrame = 0;
    reloadActFrame = -1;
    SetShootRate(25.0f);
    damage = 120.0f;
    inst->flags |= 0x10;
    inst->flags |= 1;
    pwpWEAPON::flags2 |= 3;
    wpnWEAPON::flags |= 4;
    SetAmmo(1.0f);
    inst->root->AllocVertNormalList();
    return 1;
}

// v12 0x00415560  (feb 0x00413BD0)
// The swing sound when the SHOOT sequence (7) passes action frame 1.
void pwpSHOVEL::ProcessFRAME()
{
    pwpWEAPON::ProcessFRAME();
    animINST *a = inst;
    if (!(a->flags & 1) && a->seq == 7 && (a->actionFrames & 2))
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)pwpSvlSnd, 0);
}

// v12 0x0040AC20 (folded with pwpBULLET_WPN's)
// On the server the shot goes to every client but its owner; then to the owner (msg 0x4C8 -> ShootBody).
void pwpSHOVEL::ProcessSHOOT(void *msg)
{
    if (entFlags & 4)
        gsSysServer->SendMsgClientAll(owner, 0x4C7, (msgDATA *)msg, 0x90000, 0xFFFFFFFF, owner->mpOwner);
    msgSystem.SendMsg(owner, 0x4C8, msg, this);
}

// the entity under the crosshair: gscCursor.aimEnt (v12 0x0098EAC4, 1.2 only)

#pragma pack(push, 1)
struct pwpSVL_SHOOT : msgDATA {                         // the swing, 0x26 bytes (0x4C7 to the server, 0x4C8 locally)
    u8    _04;                                          // +0x04 0
    u8    hit;                                          // +0x05 the ray hit something (wall / object)
    int   targetId;                                     // +0x06 mpId of the entity hit, -1 none
    u16   faceId;                                       // +0x0A of a wall hit
    u16   objId;                                        // +0x0C of a wall hit (-1 none)
    m3dV  point;                                        // +0x0E the hit point
    m3dV  dir;                                          // +0x1A the blade's Z axis
};
#pragma pack(pop)

// v12 0x00416B60
// The target id and the hit flag; with a hit, the point and the direction (mpQuantPos: 3 x 16 bits each), the
// face and the object. 0: no message (size -1).
BOOL32 pwpSVL_PACK::Pack(fioFILE_MEM *f, msgDATA *d)
{
    pwpSVL_SHOOT *m = (pwpSVL_SHOOT *)d;
    if (m->size == -1)
        return 0;
    int id = m->targetId;
    f->WriteData(&id, 4, 1);
    u8 hit = m->hit;
    f->WriteData(&hit, 1, 1);
    if (m->hit) {
        unsigned short qx, qz, qy;
        int v;
        mpQuantPos(&m->point, &qx, &qz, &qy);
        v = qx;
        f->WriteData(&v, 2, 2);
        v = qz;
        f->WriteData(&v, 2, 2);
        v = qy;
        f->WriteData(&v, 2, 2);
        mpQuantPos(&m->dir, &qx, &qz, &qy);
        v = qx;
        f->WriteData(&v, 2, 2);
        v = qz;
        f->WriteData(&v, 2, 2);
        v = qy;
        f->WriteData(&v, 2, 2);
        v = m->faceId;
        f->WriteData(&v, 2, 2);
        v = m->objId;
        f->WriteData(&v, 2, 2);
    }
    return 1;
}

// v12 0x00416CB0
// Into one static message (made once: this packer's id, size 0x26); without a hit the face / object are -1 and the
// vectors zero.
msgDATA *pwpSVL_PACK::UnPack(fioFILE_MEM *f)
{
    static pwpSVL_SHOOT m;
    static int made;
    if (!(made & 1)) {
        made |= 1;
        m._04 = 0;
        m.hit = 0;
        m.targetId = -1;
        m.objId = 0xFFFF;
        m.size = 0x26;
        m.packId = (u8)pwpSvlPack.id;
        m.tag = 0x40;                       // (msgDATA ctor v12 0x00406700)
    }
    f->ReadData(&m.targetId, 4, 4);
    f->ReadData(&m.hit, 1, 1);
    if (!m.hit) {
        m.objId = 0xFFFF;
        m.faceId = 0xFFFF;
        m.point = m3dVZero;
        m.dir = m3dVZero;
        return &m;
    }
    unsigned short qx, qz, qy;
    f->ReadData(&qx, 2, 2);
    f->ReadData(&qz, 2, 2);
    f->ReadData(&qy, 2, 2);
    mpDequantPos(qx, qz, qy, &m.point);
    f->ReadData(&qx, 2, 2);
    f->ReadData(&qz, 2, 2);
    f->ReadData(&qy, 2, 2);
    mpDequantPos(qx, qz, qy, &m.dir);
    f->ReadData(&m.faceId, 2, 2);
    f->ReadData(&m.objId, 2, 2);
    return &m;
}

// v12 0x004155A0  (feb 0x00413C10)
// The swing: the closest instance within 1.5 of the player and 65 degrees of its facing (or the one aimed at),
// if visible from 1.5 above the player's feet; else a 1.5 ray from the camera through the blade tip.
int pwpSHOVEL::Shoot(entENTITY *target, m3dV *aim)
{
    pwpSVL_SHOOT m;
    memset(&m, 0, sizeof(m));
    m.targetId = -1;
    m.objId = 0xFFFF;
    m.packId = *((u8 *)&pwpSvlPack + 4);
    m.tag = 0x40;
    m.size = 0x26;
    cdtINFO ci;
    memset(&ci, 0, sizeof(ci));
    *(m3dV *)ci.point = m3dVZero;
    entCDT_REFINE_EXCL r(2, plrPlayer->inst, 1);
    if (!wpnWEAPON::Shoot(owner, aim))
        return 0;
    m3dV pos, dir;
    plrPlayer->inst->GetPos(&pos);
    plrPlayer->inst->GetDir(&dir);
    dir.y = 0.0f;
    m3dNormalize(&dir);
    animINST *list[50];
    int n;
    dmgCollectInst(&pos, 1.5f, list, &n, 0x32);
    animINST *best = 0;
    float bestD = 2000000.0f;
    for (int i = 0; i < n; i++) {
        animINST *a = list[i];
        if (a == plrPlayer->inst || (a->flags & 0x40))
            continue;
        m3dV p, d;
        a->GetPos(&p);
        d.x = p.x - pos.x;
        d.y = p.y - pos.y;
        d.z = p.z - pos.z;
        if (m3dAngleVector(&d, &dir) > 65.0f && !(gscCursor.aimEnt && gscCursor.aimEnt->inst == list[i]))
            continue;
        float dx = pos.x - p.x, dy = pos.y - p.y, dz = pos.z - p.z;
        float dist = m3dSqrt(dx * dx + dy * dy + dz * dz);
        if (dist < bestD) {
            bestD = dist;
            best = list[i];
        }
    }
    if (best) {
        m3dV from = pos, c;
        best->GetCenter(&c);
        from.y += 1.5f;
        if (gsScenePtr->IsVisiblePoint(&from, &c, 0, &r, 0)) {
            m.targetId = best->ent->mpId;
            gsSysClient->SendMsgServer(plrPlayer, 0x4C7, &m, 0x20000);
            if (!(entFlags & 4))
                msgSystem.SendMsg(plrPlayer, 0x4C8, &m, this);
            return 1;
        }
    }
    m3dV cam, tipPos, d;
    gsCameraPtr->GetOrigin(&cam);
    tip->GetOrigin(&tipPos);
    d.x = tipPos.x - cam.x;
    d.y = tipPos.y - cam.y;
    d.z = tipPos.z - cam.z;
    m3dNormalize(&d);
    if (gsScenePtr->ShootRay(&cam, &d, 1.5f, 0, &r, &ci)) {
        m.point = *(m3dV *)ci.point;
        m.hit = 1;
        tip->GetAxisZ(&m.dir);
        m3dNormalize(&m.dir);
        if (ci.flags & 2) {
            if (!ci.inst)
                m.targetId = -1;
            else {
                entENTITY *e = ci.inst->ent;
                if (!e || e->sign != 0x454E5424)
                    return 0;
                m.targetId = e->mpId;
            }
            m.faceId = (u16)ci.faceId;
            m.objId = (u16)ci.objId;
        }
    }
    gsSysClient->SendMsgServer(plrPlayer, 0x4C7, &m, 0x20000);
    if (!(entFlags & 4))
        msgSystem.SendMsg(plrPlayer, 0x4C8, &m, this);
    return 1;
}

// v12 0x00415B00  (feb 0x00413C10 ProcessSHOOT 0x00414110)
// The server damages the entity hit (at its position + 1); every side plays the sound: [2] for a living NPC
// (vslot +0x1C8) or a statue, [1] for anything else hit, [2] and the hit effect ("sfx_shovel") for a wall.
void pwpSHOVEL::ShootBody(void *msg)
{
    pwpSVL_SHOOT *m = (pwpSVL_SHOOT *)msg;
    dmgWEAPON d;
    if ((entFlags & 4) && m->targetId != -1) {
        entENTITY *e = (entENTITY *)gsSysMP->FindMPID(m->targetId, 0);
        if (!e)
            return;
        wrSysStat->Notify_WpnHit(owner->mpId, wpnIndex);
        m3dV p;
        e->inst->GetPos(&p);
        p.y += 1.0f;
        MakeDamage(&d, &p);
        d.sourceId = owner->mpId;
        msgSystem.SendMsg(e, 0x4C6, &d, this);
    }
    if (m->targetId != -1) {
        entENTITY *e = (entENTITY *)gsSysMP->FindMPID(m->targetId, 0);
        if (e && ((e->classId == AI_SIG_NPC && wrEVCall<BOOL32>(e, 0x1C8)) || e->classId == AI_SIG_STATUE)) {
            ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpSvlSnd[2], 0);
            return;
        }
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpSvlSnd[1], 0);
        return;
    }
    if (m->hit) {
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpSvlSnd[2], 0);
        entCreate(gsScenePtr, (char *)"sfx_shovel", (char *)"sfx_shovel", (animCREATE_DATA *)0, m, 0);
    }
}
