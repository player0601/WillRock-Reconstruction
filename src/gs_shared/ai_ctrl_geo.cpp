// ai_ctrl_geo.cpp -- where an NPC stands (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_ctrl_geo.obj)

#include "../Sources/game.h"

void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int skip);  // v12 0x005130D0 (gs:gs_effect.obj)

// v12 0x0054EB40  (feb 0x005336A0)
void aiNPC::SettleSurface()
{
    entCDT_REFINE_EXCL ref(2, 0, 0);
    ref.cls.bits |= GetStateEntExcludeCDT();
    m3dV pos, to;
    inst->GetPos(&pos);
    if (gsScenePtr->FloorPlace(&pos, 2000000.0f, &to, 0.55f, 0, 0, &ref, &floor)) {
        inst->Translate(&to, 0);
        return;
    }
    if (gsScenePtr->FloorPlace(&pos, 2000000.0f, &to, 3.55f, 0, 0, &ref, &floor))
        inst->Translate(&to, 0);
}

// v12 0x0054EC30  (feb 0x00533780)
// The enemy's distance and direction from the actor's root.
void aiNPC::FillToPlayerCoords()
{
    m3dV pos, d, t, to;
    GetPosRooted(&pos);
    inst->GetDir(&d);
    GetPlayerTargetPos(&t);
    to.x = t.x - pos.x;
    to.y = t.y - pos.y;
    to.z = t.z - pos.z;
    enemyDist = m3dSqrt(to.x * to.x + to.y * to.y + to.z * to.z);
    enemyHeight = m3xzAngleVector(&to, &d);
    enemyAngle = fabsf(enemyHeight);
}

// v12 0x0054ED60  (feb 0x005338E0)
void aiNPC::GetWpnTip(m3dV *out)
{
    if (inst->root)
        inst->GetCenter(out);
    else
        inst->GetPos(out);
}

// v12 0x0054ED80  (feb 0x00533900)
void aiNPC::GetPlayerTargetPos(m3dV *out)
{
    enemy.GetPos(out);
}

// v12 0x0054ED90  (feb 0x00533910)
BOOL32 aiNPC::IsPivotUsable()
{
    if (!pivotObj)
        return 0;
    inst->Validate(4);
    return !(pivotObj->procState & 4);
}

// v12 0x0054EDD0  (feb 0x00533930, a slot again in 1.2)
// Turn towards a direction at turnSpeed (a client takes it as it is), about the pivot if the sequence says so.
void aiNPC::Slot23(void *dirv)
{
    m3dV to;
    if (entFlags & 4)
        m3dClampVector_A(&dir, (m3dV *)dirv, gsElapsedTime * turnSpeed, &to, 0, 0);
    else
        to = *(m3dV *)dirv;
    if (inst->tpl->seq[inst->seq].unk84 & 2) {
        if (pivotObj) {
            inst->Validate(4);
            if (!(pivotObj->procState & 4)) {
                m3dV o;
                inst->Validate(4);
                ((m3dMATR *)pivotObj->ltm)->GetOrigin(&o);
                inst->SetDir_XZ_UsePivot(&to, &o);
            }
        }
        return;
    }
    inst->SetDir_XZ(&to);
}

// v12 0x0054EED0  (feb 0x005339F0)
int aiNPC::GetStateEntExcludeCDT()
{
    return (~aiFlags >> 30) & 1;
}

// v12 0x0054EEE0  (feb 0x00533A00)
// The light on it from the floor under it, every colorizeFrames frames. A client looks for the floor itself.
void aiNPC::Colorize(int skip)
{
    if (colorizePhase > 0 && skip) {
        colorizePhase--;
        return;
    }
    colorizePhase = colorizeFrames - 1;
    if (!(entFlags & 4)) {
        cdtREFINE ref(2, 0);
        m3dV p, to;
        inst->GetPos(&p);
        gsScenePtr->FloorPlace(&p, 30.0f, &to, 1.0f, 1.0f, 0, &ref, &floor);
    }
    gsEffApplyLMChar(this, &floor, skip);
}

// v12 0x0054EFA0  (feb 0x00533AB0)
// Not the dead a second after death (the big ones stay solid: cyclop, croc, gefest); a boss's corpse not the player.
BOOL32 aiCDT_REFINE::IsInclInst(animINST *a)
{
    if (aiIsNPCDead(a->ent, 0) && ((aiNPC *)a->ent)->deadTimer > 1.0f) {
        const char *c = a->className ? a->className : "";
        if (!strstr(c, "ai_cyclop") && !strstr(c, "ai_croc") && !strstr(c, "ai_gefest"))
            return 0;
    }
    if ((((aiNPC *)exclude->ent)->aiFlags2 & 0x40000) && a->ent && a->ent->classId == 0x504C5952)
        return 0;
    return entCDT_REFINE_EXCL::IsInclInst(a);
}

// v12 0x0054F070  (feb 0x00533B20)
BOOL32 aiCDT_REFINE::IsInclObj(objOBJ *o)
{
    if (o->name && strstr(o->name, "&aic"))
        return 1;
    return (!(o->procState & 0x1F) || (o->procState & 0x10000)) && !(o->procState & 0x40000);
}

// v12 0x0054F0E0  (feb 0x00533B90)
void aiNPC::GetPrevPositions()
{
    inst->GetPos(&prevPos[0]);
    inst->GetDir(&dir);
    GetPosRooted(&prevPos[1]);
}

// v12 0x0054F120  (feb 0x00533BD0)
int aiNPC::IsStayOn(animINST *i)
{
    return floor.inst == i && floorGap < 0.05f && speed.y < 1e-6f;
}
