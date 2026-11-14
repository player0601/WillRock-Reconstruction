// ai_ctrl_nav_phys.cpp -- how an NPC moves (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_ctrl_nav_phys.obj)
//
// 1.2 changes: the vertical snap is skipped on the way up and for a sniper; falling in water kills with 5000, subtype 5;
// ConstrainMotion skips the low sphere while jumping and (aiFlags2 1) keeps the height; a passive fall reports its end.

#include "../Sources/game.h"

// v12 0x0054D7F0  (feb 0x00532420)
void aiNPC::MoveToGoal()
{
    m3dV pos, d;
    inst->GetPos(&pos);
    d = goalDir;
    if (!(aiFlags & 0x400))
        Slot23(&d);
    if (aiFlags & 0x200)
        return;
    m3dV to = goalPos;
    ClampGoalFrame(moveSpeed, &d, &to);
    inst->Translate(&to, 0);
    prevPos[2] = to;
}

// v12 0x0054D8B0  (feb 0x005324F0)
// Moving sideways (navFlags 1): minTurnRadius goes sideways, the rest of the speed forward (at least 1/5 of it).
void aiNPC::GetClampingSpeeds(float spd, float *forward, float *side)
{
    if (!(navFlags & 1)) {
        *forward = spd;
        *side = 0.0f;
        return;
    }
    float d = spd * spd - minTurnRadius * minTurnRadius;
    *side = minTurnRadius;
    if (d > 1e-6f) {
        *forward = m3dSqrt(d);
    } else if (d >= -1e-6f) {
        *forward = 0.0f;
    } else {
        float r = m3dSqrt(-d), limit = spd * 0.2f;
        *forward = r <= limit ? r : limit;
    }
}

// v12 0x0054DBC0  (feb 0x00532800)
// The goal this frame: towards it at the speed; if the facing is too far from the direction, a turn on the spot (or, with
// aiFlags2 2, up to 120 degrees off: on along the facing turned by at most maxTurnStep).
void aiNPC::ClampGoalFrame(float spd, m3dV *dirTo, m3dV *goal)
{
    float fwd, side;
    GetClampingSpeeds(spd, &fwd, &side);
    m3dV pos, d;
    inst->GetPos(&pos);
    inst->GetDir(&d);
    float a = m3xzAngleVector(&d, dirTo);
    if (maxTurnStep < fabsf(a)) {
        if (!(aiFlags2 & 2) || (aiFlags & 4)) {
            fwd = 0.0f;
        } else if (fabsf(a) <= 120.0f) {
            float lim = maxTurnStep;
            if (a > lim)
                a = lim;
            else if (a < -lim)
                a = -lim;
            m3dMATR r;
            r.MakeRotY(a);
            m3dV o;
            o.x = d.x * r.m[0][0] + d.y * r.m[1][0] + d.z * r.m[2][0];
            o.y = d.x * r.m[0][1] + d.y * r.m[1][1] + d.z * r.m[2][1];
            o.z = d.x * r.m[0][2] + d.y * r.m[1][2] + d.z * r.m[2][2];
            goal->x = o.x * 20.0f + pos.x;
            goal->y = o.y * 20.0f + pos.y;
            goal->z = o.z * 20.0f + pos.z;
        } else {
            goal->x = d.x * 20.0f + pos.x;
            goal->y = d.y * 20.0f + pos.y;
            goal->z = d.z * 20.0f + pos.z;
        }
    }
    m3dClampVector_L(&pos, goal, gsElapsedTime * fwd);
    if (navFlags & 1) {
        float k = gsElapsedTime * side;
        goal->x += k * strafeDir.x;
        goal->y += k * strafeDir.y;
        goal->z += k * strafeDir.z;
    }
}

// v12 0x0054E160  (feb 0x00532DA0)
cdtREFINE *aiNPC::GetFloorPlaceRefiner()
{
    static aiCDT_REFINE ref;
    ref.exclude = inst;
    ref.cls.bits = GetStateEntExcludeCDT();
    ref.mode = 2;
    return &ref;
}

// v12 0x0054E1E0  NEW in 1.2 (name ours)
int aiNPC::IsSniperOnFloor()
{
    return (aiFlags & 0x40000) && !(aiFlags & 0x200800);
}

// v12 0x0054E200  (feb 0x00532E20)
void aiNPC::ProcessVoluntaryPhysics()
{
    cdtINFO hit;
    memset(&hit, 0, sizeof(hit));
    m3dV start, pos, out;
    inst->GetPos(&start);
    m3dV step = { gsElapsedTime * speed.x, gsElapsedTime * speed.y, gsElapsedTime * speed.z };
    inst->Translate(&step, 2);
    GetPosRooted(&pos);
    if (speed.y > 0.01f) {
        out = pos;
    } else {
        float up = (aiFlags & 0x200000) ? 2.0f : heightUpTester;
        float rise = prevPos[1].y - pos.y;
        if (rise > 0.0f)
            up += rise;
        if (IsSniperOnFloor()) {
            out = pos;
        } else {
            cdtREFINE *r = GetFloorPlaceRefiner();
            if (!gsScenePtr->FloorPlace(&pos, 2000000.0f, &out, up, 0, 0x20, r, &floor))
                gsScenePtr->FloorPlace(&pos, 2000000.0f, &out, up + 3.0f, 0, 0x20, r, &floor);
        }
        int keep = 0;
        if (up + floor.point[1] < pos.y) {
            float v = speed.y > -1.0f ? -1.0f : speed.y;
            if (gsElapsedTime * v + pos.y > floor.point[1]) {
                out.y = pos.y;
                keep = 1;
            }
        }
        if (!keep && (fallDist > 0.05f || speed.y < -0.01f) && pos.y < out.y)
            out.y = pos.y;
    }
    if (floor.flags & 2)
        floorGap = out.y - floor.point[1];
    if (ConstrainMotion(&prevPos[1], &out, &hit))
        ProcessCDT(&hit);
    SetPosRooted(&out);
    m3dV now;
    inst->GetPos(&now);
    speed.y -= gsElapsedTime * gravity;
    float fell = now.y - start.y + fallDist;
    fallDist = fell;
    if (floor.flags & 2) {
        if (floorGap >= 0.05f)
            return;
        m3dV h = { speed.x, 0.0f, speed.z };
        if (_m3dNormalize(&h)) {
            float k = gsElapsedTime * 200.0f;
            h.x *= k;
            h.z *= k;
            if (h.x * h.x + h.z * h.z <= speed.x * speed.x + speed.z * speed.z) {
                speed.x -= h.x;
                speed.y -= h.y * k;
                speed.z -= h.z;
            } else {
                speed.x = 0;
                speed.z = 0;
            }
        }
        if (fallDist < -1.0f)
            aiFlags |= 0x20000000;
        fallDist = 0;
        if (speed.y >= 0.01f)
            return;
        speed.y = 0;
    } else {
        if (fell < -10.0f)
            aiFlags |= 0x20000000;
        if (fell < -5.0f && (aiFlags & 0x80000000)) {
            static dmgDAMAGE d(0);
            d.type = 5;
            d.subtype = 0;
            d.amount = 5000.0f;
            msgSystem.SendMsg(this, 0x4C6, &d, 0);
        }
        fallDist = 0;
        speed = m3dVZero;
    }
    aiFlags &= ~0x800u;
}

// v12 0x0054E6B0  (feb 0x00533290)
// The physics object carries it (a knock-back): the root's movement is taken out unless a fall has just landed.
void aiNPC::ProcessPassivePhysics()
{
    u8 *ph = (u8 *)passivePhys;
    int seqF = inst->tpl->seq[inst->seq].unk84;
    m3dV r;
    GetPosRooted(&r);
    *(m3dV *)(ph + 0xB4) = m3dV { r.x - prevPos[1].x, r.y - prevPos[1].y, r.z - prevPos[1].z };
    int bounces = *(int *)(ph + 0xB0);
    passivePhys->ProcessFRAME();                        // slot +0x08 (a physMATH: no msgADDR shift -- was one slot off)
    m3dMATR m;
    memcpy(&m, ph + 8, sizeof(m));
    inst->Transform(&m, 0);
    m3dV off = { 0, 0, 0 };
    if ((seqF & 1) && (*(int *)(ph + 0xB0) > bounces || bounces > 10)) {
        off.x = prevPos[0].x - prevPos[1].x;
        off.y = prevPos[0].y - prevPos[1].y;
        off.z = prevPos[0].z - prevPos[1].z;
    }
    off.y = -*(float *)(ph + 0x78) - fallSpeed;
    inst->Translate(&off, 2);
    if (*(int *)(ph + 0xB0) > bounces)
        aiFlags |= 0x8000;
}

// v12 0x0054E820  (feb 0x005333E0)
void aiNPC::ProcessPhysics()
{
    if (aiFlags & 0x100)
        ProcessPassivePhysics();
    else
        ProcessVoluntaryPhysics();
    UpdateUnderwater();
}

// v12 0x0054E850  (feb 0x00533410)
void aiNPC::ApplySpeed(m3dV *v)
{
    speed = *v;
}

// v12 0x0054E870  (feb 0x00533430)
void aiNPC::ApplySpeedTrampoline(m3dV *v)
{
    ApplySpeed(v);
}

// v12 0x0054E880  (feb 0x00533440)
BOOL32 aiNPC::GetFloorInfo(cdtINFO *ci)
{
    *ci = floor;
    return 1;
}

// v12 0x0054E8A0  (feb 0x00533470)
// Two collision spheres between the old and new places: the low one (not while jumping), then a body sphere at heightCDT,
// unk17B ahead. 1.2: when both were hit, the move stops at the old place. aiFlags2 1 keeps the height.
int aiNPC::ConstrainMotion(m3dV *from, m3dV *to, cdtINFO *ci)
{
    cdtINFO hit;
    memset(&hit, 0, sizeof(hit));
    aiCDT_REFINE ref;
    ref.mode = 0;
    ref.exclude = inst;
    ref.queryFlags = 0;
    m3dV d;
    inst->GetDir(&d);
    float y0 = to->y;
    m3dV a = *from;
    ref.cls.bits = GetStateEntExcludeCDT();
    int low = 0, body = 0;
    if (!(aiFlags & 0x800)) {
        to->y += unk177;
        a.y += unk177;
        ref.mode = 9;
        if (gsScenePtr->ConstrSphere(&a, to, heightLowTester, 0x4A0, &ref, &hit)) {
            low = 1;
            *ci = hit;
        }
        a.y -= unk177;
        to->y -= unk177;
    }
    if (!(aiFlags2 & 1))
        to->y = y0;
    ref.mode = 2;
    to->y += heightCDT;
    to->x += d.x * unk17B;
    to->y += d.y * unk17B;
    to->z += d.z * unk17B;
    a.x += d.x * unk17B;
    a.y += d.y * unk17B + heightCDT;
    a.z += d.z * unk17B;
    if (gsScenePtr->ConstrSphere(&a, to, radCDT, 0, &ref, &hit)) {
        body = 1;
        *ci = hit;
    }
    to->x -= d.x * unk17B;
    to->y -= d.y * unk17B + heightCDT;
    to->z -= d.z * unk17B;
    if (!low)
        return body;
    if (body)                                           // both hit: back to the old place (v12 0x0054EADD; was
        *to = *from;                                    //   inverted here: a step under the feet froze the NPC)
    return 1;
}
