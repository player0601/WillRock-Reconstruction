// trampoline.cpp -- trmpTRAMPOLINE, a jump pad: whoever stands on it (the local player, and on the server NPCs) is
// thrown toward a target instance or along the pad's direction (Will Rock 1.2, v12.exe; feb obj
// gs_shared:trampoline.obj)
//
// Rebuilt from the 1.2 code (v12 0x00558480..0x00558B69, vtable 0x00609030). Fields at feb + 0x1C. 1.2 changes,
// from the disassembly:
//   - ProcessINIT raises event 0x800 (initialised) at its end;
//   - FRAME checks that there is a local player (plgsPlayer) before asking it; the player's slots moved down by 4
//     (IsDead +0x74, IsStayOn +0xA0, ApplySpeed +0xA4, GetGravity +0xA8);
//   - the NPC slots are +0x180 (stands on), +0x170 (gravity), +0x174 (apply speed).
// TRAMPOLINE script: height (20; with a target and no height: 15..30 from the distance), randShift, target,
// speedXZ (no target).

#include "../Sources/game.h"

static int trmpInvalidate = 1;          // v12 0x006378A4 (feb 0x005F96E4): FRAME invalidates the instance every 5 s

// v12 0x00558480  (feb 0x005433F0)
int __fastcall trmpInit(void)
{
    return entRegisterClass((char *)"trampoline", 'TRMP', 0x28, trmpTRAMPOLINE::Create, 0, 1) != 0;
}

// v12 0x005584B0  (feb 0x00543420)
void trmpTRAMPOLINE::ProcessINIT()
{
    height = 20.0f;
    BOOL32 hasHeight = inst->GetFloat((char *)"TRAMPOLINE", (char *)"height", &height);
    randShift = 0.0f;
    inst->GetFloat((char *)"TRAMPOLINE", (char *)"randShift", &randShift);
    char name[0x40];
    speedXZ = 0.0f;
    hasTarget = 0;
    if (!inst->GetStr((char *)"TRAMPOLINE", (char *)"target", name, 0x40)) {
        inst->GetFloat((char *)"TRAMPOLINE", (char *)"speedXZ", &speedXZ);
    } else if (animINST *t = gsScenePtr->FindInstName(name)) {
        m3dV p;
        inst->GetPos(&p);
        t->GetPos(&targetPos);
        if (!hasHeight) {
            float dx = p.x - targetPos.x, dz = p.z - targetPos.z;
            float d = m3dSqrt(dx * dx + dz * dz);
            if (d < 20.0f)
                height = 15.0f;
            else if (d > 60.0f)
                height = 30.0f;
            else
                height = (d - 20.0f) * 0.375f + 15.0f;
        }
        hasTarget = 1;
    }
    gsScenePtr->SetStateYesInst(inst, 8);
    pendingEvents |= 0x800;
}

// v12 0x005586E0  (feb 0x00543640)
BOOL32 trmpTRAMPOLINE::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    cdtREFINE ref(0, 0);
    m3dBOX box;
    scnAOBJ list[100];
    m3dV v;
    if (id == MSG_MSG_INIT) {
        ProcessINIT();
        return 0;
    }
    if (id == 0x3F4) {                      // INIT_LEVEL: onto the ground
        FloorPlace(0.0f, 1, 0);
        timer = 0.0f;
        return 0;
    }
    if (id != 1000)                         // FRAME
        return 0;
    if (nLocks > 0 || (inst->flags & 3))
        return 0;
    if (timer > 5.0f) {
        if (trmpInvalidate)
            inst->Invalidate(1);
        timer = 0.0f;
    } else {
        timer += gsElapsedTime;
    }
    if (plgsPlayer && wrEVCall<BOOL32>(plgsPlayer, 0xA0, inst, (objOBJ *)0, 1)  // IsStayOn
        && !wrEVCall<BOOL32>(plgsPlayer, 0x74)) {                              // IsDead
        GetSpeedThrow(wrEVCall<float>(plgsPlayer, 0xA8), &v);                  // GetGravity
        wrEVCall<void>(plgsPlayer, 0xA4, &v, 1);                               // ApplySpeed (set)
        m3dV p;
        inst->GetPos(&p);
        pendingEvents |= 0x20;
    }
    if (!(entFlags & ENTF_IACTIONS_ON))
        return 0;
    inst->GetBBox(&box);
    box.Expand(2.0f);
    int n = gsScenePtr->MakeAnimList(&box, &ref, list, 100);
    for (int i = 0; i < n; i++) {
        entENTITY *e = (entENTITY *)list[i].inst->ent;
        if (e->classId != '$NPC')
            continue;
        if (!wrEVCall<BOOL32>(e, 0x180, inst))                                 // stands on the pad
            continue;
        if (((aiNPC *)e)->aiFlags & 0x200000)                                 // dead
            continue;
        GetSpeedThrow(wrEVCall<float>(e, 0x170), &v);
        wrEVCall<void>(e, 0x174, &v);
        pendingEvents |= 0x20;
    }
    return 0;
}

// v12 0x00558920  (feb 0x00543870)
// The launch velocity: with a target, the throw that reaches the target (shifted by up to randShift) at the
// pad's height; else speedXZ along the pad's direction and the vertical speed of a jump to that height,
// randShift spread over the flight time.
void trmpTRAMPOLINE::GetSpeedThrow(float gravity, m3dV *out)
{
    m3dV r;
    m3dMakeVTangRandom(&m3dVUnitY, &r);
    if (hasTarget) {
        m3dV p, t;
        inst->GetPos(&p);
        float s = m3dRandMax(randShift);
        t.x = r.x * s + targetPos.x;
        t.y = r.y * s + targetPos.y;
        t.z = r.z * s + targetPos.z;
        cgmGetThrowSpeed_H(&p, &t, height, -gravity, out);
        return;
    }
    m3dV d;
    inst->GetDir(&d);
    out->x = d.x * speedXZ;
    out->y = d.y * speedXZ;
    out->z = d.z * speedXZ;
    float vy = m3dSqrt(gravity * height + gravity * height);
    out->y = vy;
    if (fabsf(vy) > 0.1f) {
        float s = randShift / (vy / gravity + vy / gravity);
        out->x = r.x * s + out->x;
        out->y = vy + r.y * s;
        out->z = r.z * s + out->z;
    }
}

// v12 0x00558B20  (feb 0x00543A80)
entENTITY *__fastcall trmpTRAMPOLINE::Create(animINST *)
{
    trmpTRAMPOLINE *t = new trmpTRAMPOLINE;    // operator new(0x197), entENTITY ctor
    if (!t)
        return 0;
    t->unk16B = m3dVUndef;
    return t;
}
