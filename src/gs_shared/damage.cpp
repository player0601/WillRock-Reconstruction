// damage.cpp -- damage messages: dmgDAMAGE / dmgGENERAL / dmgWEAPON and the radial damage helpers
// (Will Rock 1.2, v12.exe; feb obj gs_shared:damage.obj)
//
// So far: dmgCollectInst (v12 0x00546210), the dmgGENERAL ctor (v12 0x005461F0, feb 0x0052E690) and the dmgWEAPON ctor (v12 0x00546D00, feb 0x0052F0C0; 1.2 packs the struct: 0x30 bytes, feb 0x32).

#include "../Sources/game.h"
#include "../wr_vcall.h"
#include "../gs/gs_mp_server.h"

unsigned char dmgMsgDataId;                 // v12 0x009A84CC (feb 0x0095D620) set when dmgInit registers the packer

// v12 0x005461F0  (feb 0x0052E690)
dmgGENERAL::dmgGENERAL()
{
    msgId = 0;
    msgFlags = 0x40;
    type = DMG_GENERAL;
    subtype = 0;
    sourceId = 0xFFFFFFFF;
    size = 0x1A;
}

// v12 0x00546D00  (feb 0x0052F0C0)
dmgWEAPON::dmgWEAPON()
{
    msgFlags = 0x40;
    type = 1;
    subtype = 0;
    sourceId = 0xFFFFFFFF;
    weaponType = -1;
    unk2A = 0;
    unk2E = -1;
    size = 0x30;
    msgId = dmgMsgDataId;
}

// v12 0x00546210  (feb 0x0052E6B0)
// The instances of entities (class 4) within radius of pos, max of them; the player too when within reach (its own
// instance is kept out of the scene queries: flag 0x10) and on the server.
void __fastcall dmgCollectInst(m3dV *pos, float radius, animINST **out, int *n, int max)
{
    entCDT_REFINE_INCL ref(0, 0, 4);
    scnAOBJ list[100];
    int k = gsScenePtr->MakeAnimList(pos, pos, radius, &ref, list, 100);
    if (k > max)
        k = max;
    int i;
    for (i = 0; i < k; i++)
        out[i] = list[i].inst;
    if (plgsPlayer && (plgsPlayer->inst->flags & 0x10) && (plgsPlayer->entFlags & 4)) {
        m3dV p;
        wrEVCall<void>(plgsPlayer, 0x64, &p);           // GetPos
        float dx = p.x - pos->x, dy = p.y - pos->y, dz = p.z - pos->z;
        if (m3dSqrt(dx * dx + dy * dy + dz * dz) < radius && k < max)
            out[k++] = plgsPlayer->inst;
    }
    *n = k;
}

// v12 0x005463E0  (feb 0x0052E870)
void dmgDISPATCHER::Dispatch(entENTITY *target, dmgDAMAGE *d, entENTITY *attacker)
{
    msgSystem.SendMsg(target, GS_MSG_DAMAGE, d, attacker);
}

// v12 0x00546410  (feb 0x0052E8A0)
// 1.2: not on an MP client.
void __fastcall dmgDispatchExplosDamage(entENTITY *attacker, m3dV *pos, float dmg, float radius, int isGeneral,
                                        int weaponType, cdtINFO *hit, unsigned long srcId, dmgDISPATCHER *disp)
{
    if (gsSysMP->mode == 3)
        return;
    animINST *list[50];
    int n;
    dmgCollectInst(pos, radius, list, &n, 0x32);
    for (int i = 0; i < n; i++)
        dmgDispatchExplosDamageInst(list[i], attacker, pos, dmg, radius, isGeneral, weaponType, hit, srcId, disp);
}

// v12 0x005464C0  (feb 0x0052E930)
// Not on the attacker itself. All of dmg to what the blast hit directly; else all within a third of the radius, a
// quarter at the radius, in line between. The target must be seen from the blast, from 1.5 / 1 m up or level
// (flags2 0x80: it need not be).
void __fastcall dmgDispatchExplosDamageInst(animINST *inst, entENTITY *attacker, m3dV *pos, float dmg, float radius,
                                            int type, int weaponType, cdtINFO *hit, unsigned long srcId,
                                            dmgDISPATCHER *disp)
{
    dmgWEAPON w;
    dmgGENERAL g;
    w.type = 1;
    w.subtype = 0;
    w.msgId = dmgMsgDataId;
    w.sourceId = 0xFFFFFFFF;
    w.weaponType = -1;
    w.unk2A = 0;
    w.unk2E = -1;
    w.size = 0x30;
    if (attacker == inst->ent)
        return;
    dmgDAMAGE *d;
    if (type == 1) {
        d = &w;
        w.weaponType = weaponType;
        w.origin = *pos;
    } else {
        d = &g;
    }
    if (srcId != 0xFFFFFFFF)
        d->sourceId = srcId;
    else if (attacker && gsSysMP->IsRegularMPID(attacker->mpId))
        d->sourceId = attacker->mpId;
    d->type = (unsigned char)type;
    ((dmgGENERAL *)d)->pos = *pos;
    m3dV p;
    inst->GetPos(&p);
    float dx = pos->x - p.x, dy = pos->y - p.y, dz = pos->z - p.z;
    float dist = m3dSqrt(dx * dx + dy * dy + dz * dz);
    if (hit && (hit->flags & 2) && hit->inst == inst) {
        d->amount = dmg;
    } else {
        float a = radius * (1.0f / 3.0f), b = radius, va = dmg, vb = dmg * 0.25f;
        if (b < a) {
            float t = a;
            a = b;
            b = t;
            t = va;
            va = vb;
            vb = t;
        }
        if (dist < a)
            d->amount = va;
        else if (dist > b)
            d->amount = vb;
        else
            d->amount = (dist - a) * (vb - va) / (b - a) + va;
    }
    entENTITY *e = (entENTITY *)inst->ent;
    entCDT_REFINE_EXCL ref(6, inst, 7);
    if (!(e->flags2 & 0x80)) {
        m3dV from = *pos, c;
        wrEVCall<void>(e, 0x54, &c);
        from.y += 1.5f;
        c.y += 1.0f;
        if (!gsScenePtr->IsVisiblePoint(&from, &c, 1, &ref, 0)) {
            from.y -= 1.5f;
            c.y -= 1.0f;
            if (!gsScenePtr->IsVisiblePoint(&from, &c, 1, &ref, 0))
                return;
        }
    }
    if (disp)
        disp->Dispatch(e, d, attacker);
    else
        msgSystem.SendMsg(e, GS_MSG_DAMAGE, d, attacker);
}

// v12 0x00546A20  (feb 0x0052EDE0)
// Where a hit on the place came from: the origin (of the weapon, of the blast), the way from there to here on the
// level, the distance. A damage with no place: 0, here and along X.
int __fastcall dmgGetOrgDir(dmgDAMAGE *d, m3dV *at, m3dV *org, m3dV *dir, float *dist)
{
    m3dV o;
    switch (d->type) {
    case 1:
        o = ((dmgWEAPON *)d)->origin;
        break;
    case 2:
    case 3:
    case 4:
        o = ((dmgGENERAL *)d)->pos;
        break;
    case 0:
    case 5:
        if (org)
            *org = *at;
        if (dir)
            *dir = m3dVUnitX;
        if (dist)
            *dist = 0;
        return 0;
    default:
        return 1;
    }
    if (org)
        *org = o;
    if (dir) {
        dir->x = at->x - o.x;
        dir->y = 0;
        dir->z = at->z - o.z;
        m3dNormalize(dir);
    }
    if (dist) {
        float dx = at->x - o.x, dy = at->y - o.y, dz = at->z - o.z;
        *dist = m3dSqrt(dx * dx + dy * dy + dz * dz);
    }
    return 1;
}
