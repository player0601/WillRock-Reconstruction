// ai_npc_wr_fly.cpp -- aiNPC_WR_FLY, the base of the flyers (bird, amur, croc, medusa) (Will Rock 1.2, v12.exe;
// feb obj ai_npc_wr_fly.obj)
//
// A flyer has no gravity of its own: its move is only held back by the walls (a sphere at its body, ahead of it) and
// it is not let climb above where it meant to be; the floor under it is looked up every half second. Shot down it
// falls as a drift (up at flyHeight first, 10 down), bleeding where it was hit.

#include "game.h"

// v12 0x00454AF0  (feb 0x0044C460)
int aiNPC_WR_FLY::ProcessINIT(void *init)
{
    if (!aiNPC_WR::ProcessINIT(init))
        return 0;
    flyHeight = -1.5f;
    floorTimer = 0;
    aiFlags2 |= 3;
    return 1;
}

// v12 0x00454B30  (feb 0x0044C4A0)
// From the old place (prevPos[1]) to the new one, both raised by heightCDT and moved unk17B along the facing: the
// sphere against the walls (mode 1, 0x4A0), not higher than the move asked (+0.001), then against the rest (mode 2).
void aiNPC_WR_FLY::ProcessVoluntaryPhysics()
{
    aiCDT_REFINE ref;
    ref.mode = 0;
    ref.exclude = inst;
    ref.queryFlags = 0;
    ref.cls.bits = GetStateEntExcludeCDT();
    ref.mode = 2;
    m3dV pos, d;
    inst->GetPos(&pos);
    inst->GetDir(&d);
    float k = unk17B;
    float up = pos.y + heightCDT;
    ref.mode = 1;
    pos.x = d.x * k + pos.x;
    pos.y = d.y * k + up;
    float asked = pos.y;
    pos.z = d.z * k + pos.z;
    m3dV *from = &prevPos[1];
    from->y += heightCDT;
    k = unk17B;
    from->x += d.x * k;
    from->y += d.y * k;
    from->z += d.z * k;
    gsScenePtr->ConstrSphere(from, &pos, radCDT, 0x4A0, &ref, 0);
    if (asked + 0.001f < pos.y)
        pos.y = asked;
    ref.mode = 2;
    gsScenePtr->ConstrSphere(from, &pos, radCDT, 0x20, &ref, 0);
    k = -unk17B;
    from->x += d.x * k;
    from->y += d.y * k;
    from->z += d.z * k;
    from->y -= heightCDT;
    k = -unk17B;
    pos.x += d.x * k;
    pos.y = pos.y + d.y * k - heightCDT;
    pos.z += d.z * k;
    inst->Translate(&pos, 0);
    floorTimer += gsElapsedTime;
    if (floorTimer > 0.5f) {
        gsScenePtr->FloorPlace(&pos, 2000000.0f, 0, 0, 0, 0, &ref, &floor);
        floorTimer = 0;
    }
}

// v12 0x00454D80  (feb 0x0044C6F0)
// The floor under it (from 0.55 up), its place left as it is.
void aiNPC_WR_FLY::SettleSurface()
{
    aiCDT_REFINE ref;
    ref.mode = 0;
    ref.queryFlags = 0;
    ref.exclude = inst;
    ref.cls.bits = GetStateEntExcludeCDT();
    ref.mode = 2;
    m3dV pos, out;
    inst->GetPos(&pos);
    gsScenePtr->FloorPlace(&pos, 2000000.0f, &out, 0.55f, 0, 0, &ref, &floor);
}

// v12 0x00454E10  (feb 0x0044C770)
void aiNPC_WR_FLY::StartDeath(dmgDAMAGE *d)
{
    aiNPC_WR::StartDeath(d);
    if (entFlags & 4) {
        m3dV org, dir;
        bloodObj->GetOrigin(&org);
        dmgGetOrgDir(d, &org, &dir, 0, 0);
        StartBlood(3, 1, &org, 0, 0, &dir);
    }
}

// v12 0x00454E70  (feb 0x0044C7D0)
// The fall: a drift of radius 1 from where it is, pushed as the hit says, up at flyHeight first and 10 down,
// endless, any number of bounces (0 back, 0.8 along), not hitting the entities (class 1).
int aiNPC_WR_FLY::StartPassivePhysics(dmgDAMAGE *d)
{
    physINIT i;
    i.vel[0] = m3dVZero.x;
    i.vel[1] = m3dVZero.y;
    i.vel[2] = m3dVZero.z;
    i.accel[0] = m3dVZero.x;
    i.accel[1] = m3dVZero.y;
    i.accel[2] = m3dVZero.z;
    i.packId = 0;
    i.tag = 0x40;
    i.size = 0x38;
    i.radius = 0.1f;
    i.lifetime = 2000000.0f;
    i.maxHits = 1;
    i.unk28[0] = i.unk28[1] = i.unk28[2] = i.unk28[3] = 0;
    m3dV org;
    bloodObj->GetOrigin(&org);
    dmgGetOrgDir(d, &org, 0, (m3dV *)i.vel, 0);
    i.accel[0] = m3dVUnitY.x * -10.0f;
    i.vel[1] = flyHeight;
    i.radius = 1.0f;
    i.maxHits = -1;
    *(float *)&i.unk28[1] = 0.8f;
    i.accel[1] = m3dVUnitY.y * -10.0f;
    i.unk28[0] = 0;
    i.lifetime = -1.0f;
    i.unk28[2] = 2;
    i.unk28[3] = 1;
    i.accel[2] = m3dVUnitY.z * -10.0f;
    passivePhys->ProcessINIT(&i);
    memcpy(passivePhys->matr, inst->matr, sizeof(m3dMATR));
    passivePhys->flags |= 4;
    passivePhys->AddEntCDTExclude(this);
    if (inst)
        inst->flags &= ~0x10u;
    return 1;
}
