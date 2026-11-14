// entity.cpp -- gs.lib entities: entENTITY, the base of every game object with an instance (player, NPCs,
// weapons, items, doors...), its message handling, light colour, following another entity, and the entity class
// registry (Will Rock 1.2, v12.exe; feb obj gs:entity.obj)
//
// Rebuilt from the 1.2 code (v12 0x00506500..0x00507E9B, vtable 0x00607938), checked against the Feb 2003 decompile.
// The slot meanings follow the vtable (entity.h); some names the address tables give in this range are wrong
// (0x00507200 is FloorPlace, 0x00507D00 GetCenter, 0x00507D30 AssignANIM_ST_ANIMATE_HIDDEN, 0x00506C00 ParsePS).
// Not rebuilt here yet: the MP sync slots (0x0050A580..0x0050AA30), entCreate / entFindClass (0x00507EA0..),
// GetDistToClosestPlayer, and entIACTIVE_OBJ itself (its ctor, Pre/PostProcessMsg, IActions).

#include "../Sources/game.h"
#include "../m3d/m3d_vol.h"
#include "../engine/engine.h"   // wrArrayNew / wrDelete (abvBVOL arrays), objInitLOD
#include "gs_mp_server.h"      // entRegister: gsMSG_MP_PTR, SendMsgClientAll

u32 __fastcall gsEffEnvirColor(cdtINFO *ci, m3dCOLOR *col, m3dCOLOR *dyn);   // v12 0x005130E0 (gs:gs_eff)

extern float gsElapsedTime;                         // v12 0x0098E9F4 (feb 0x009482DC)
int __fastcall gsLvlInstDestroyNotify(animINST *i, void *p);   // v12 0x005358E0 (gs_level): gsSysLevel slot +0x34

static const float entEps = 1e-6f;                  // v12 0x00607934
float entEpsSq;                                     // v12 0x00984E60 (static init v12 0x00506F40)
static struct entEPS_INIT { entEPS_INIT() { entEpsSq = entEps * entEps; } } entEpsInit;

entCLASS_INFO entClassTable[256];                   // v12 0x00984E70 (feb 0x00940C30) // ? capacity (none checked)
int           entNClass;                            // v12 0x00984E68 (feb 0x00940C28)

// ---------------------------------------------------------------------------------------------------- creation

// v12 0x00506500  (feb 0x004F3EF0)
entENTITY::entENTITY()
{
    color[0] = color[1] = color[2] = color[3] = 0.0f;
    mpState.flags = 0;
    *(m3dV *)mpState.pos = m3dVZero;
    *(m3dV *)mpState.right = m3dVZero;
    *(m3dV *)mpState.dir = m3dVZero;
    *(m3dV *)mpState.vel = m3dVZero;
    *(m3dV *)mpState.accel = m3dVZero;
    *(m3dV *)mpState.extra = m3dVZero;
    mpSmooth[0] = 0.1f;
    mpSmooth[1] = 2.0f;
    mpSmooth[2] = 0.2f;
    unk161 = 0;
    unk98 = -1;
    unkA0 = -1;
    sign = 0x454E5424;                              // '$TNE'
    flags2 = 0;
    inst = 0;
    followEnt = 0;
    followObj = 0;
    followOffset = 0;
    unk9C = 0;
    color[0] = color[1] = color[2] = color[3] = 255.0f;
    unkA4 = 0;
    priority = 0x46;
    unkBC = 0;
    *(m3dV *)lastPos = m3dVUndef;
    *(m3dV *)lastPos2 = m3dVUndef;
    *(m3dV *)velocity = m3dVZero;
    *(m3dV *)accel = m3dVZero;
    extraBuf = 0;
    extraBufLen = 0;
}

// v12 0x00502750 (the shared "return 0")
BOOL32 entENTITY::ProcessMsg(int, void *, msgADDR *, int *)
{
    return 0;
}

// ---------------------------------------------------------------------------------------------------- messages

// v12 0x00506740  (feb 0x004F40E0)
// After the IActive object's own handling (1 = consumed): INIT adds the sound channels the property sheet names
// (and the 1.2 dead-reckoning constants), FRAME moves a follower, INIT_LEVEL reads the instance's ANIM_PROP and
// takes the start state for MP prediction.
BOOL32 entENTITY::PreProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (entIACTIVE_OBJ::PreProcessMsg(id, data, from, ret))
        return 1;
    switch (id) {
    case MSG_MSG_INIT:
        *ret = 1;
        gsSysSound->AddChannel(inst->props ? inst->props : inst->tpl->props, this, inst->root);
        mpSmooth[0] = 0.2f;
        mpSmooth[1] = 3.0f;
        mpSmooth[2] = 0.3f;
        break;
    case 1000:                                      // GS_MSG_FRAME
        UpdateFollow(0);
        break;
    case 0x3F4:                                     // GS_MSG_INIT_LEVEL
        ParsePS();
        inst->GetPos((m3dV *)mpState.pos);
        ((m3dMATR *)inst->matr)->GetAxisZ((m3dV *)mpState.dir);
        ((m3dMATR *)inst->matr)->GetAxisX((m3dV *)mpState.right);
        *(m3dV *)mpState.vel = m3dVZero;
        *(m3dV *)mpState.accel = m3dVZero;
        GetAccel((m3dV *)mpState.extra);
        break;
    }
    return 0;
}

// v12 0x00403A30  (feb 0x004036C0)  (inline in the header, emitted early in the exe)
void entENTITY::GetAccel(m3dV *out)
{
    *out = *(m3dV *)accel;
}

// v12 0x005068A0  (feb 0x004F4250)
BOOL32 entENTITY::PostProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case MSG_MSG_DESTROY_ADDR:                      // the followed entity is gone
        if (followEnt == (entENTITY *)data) {
            followEnt = 0;
            followObj = 0;
            delete (m3dMATR *)followOffset;
            followOffset = 0;
        }
        break;
    case MSG_MSG_TERM:
        if (this) {
            apFree(extraBuf);
            extraBuf = 0;
            extraBufLen = 0;
            TermFollow();
            msgSystem.DestroyAddr(this);
            gsLvlInstDestroyNotify(inst, 0);
            if (inst)
                gsScenePtr->DestroyInst(inst);
        }
        break;
    case MSG_MSG_INIT:
        if (!*ret) {                                // nothing took the INIT: the entity removes itself
            apMsg((char *)"Automatic destroy of entity \"%s\" - initialization failed",
                  inst->name ? inst->name : (char *)"");
            msgSystem.PostMsg(this, MSG_MSG_TERM, 0);
        }
        break;
    case 1000: {                                    // GS_MSG_FRAME
        // the "ON_ACTION_FRAME" IActions of the sequence playing (instance flag 0x2000: action frames crossed)
        if ((entFlags & ENTF_IACTIONS_ON) && inst && (inst->flags & 0x2000)) {
            for (int i = 0; i < nIActions; i++) {
                entIACTION_DATA *a = &iActions[i];
                if (!(a->eventMask & 0x400))
                    continue;
                if (a->animSeq != -1 && a->animSeq != inst->seq)
                    continue;
                int hit = a->actionFrame != -1 ? (inst->actionFrames & (1u << a->actionFrame)) : inst->actionFrames != 0;
                if (hit)
                    ExecuteIAction(a, this);
            }
        }
        if (inst) {                                 // the velocity from the last frame's position
            m3dV p;
            inst->GetPos(&p);
            if (!(entFlags & ENTF_REMOTE)) {
                velocity[0] = p.x - lastPos[0];
                velocity[1] = p.y - lastPos[1];
                velocity[2] = p.z - lastPos[2];
                float k = 1.0f / gsElapsedTime;
                velocity[0] = k * velocity[0];
                velocity[1] = k * velocity[1];
                velocity[2] = k * velocity[2];
            }
            *(m3dV *)lastPos = p;
        }
        if (nSndChannels > 0)
            gsSysSound->UpdateEntPos(this);
        if (flags2 & ENTF2_COLOR_BLEND)
            UpdateColorBlend();
        break;
    }
    case 0x3F4:                                     // GS_MSG_INIT_LEVEL
        ParseIActions(inst->props ? inst->props : inst->tpl->props);
        if (entFlags & ENTF_IACTIONS_ON)
            AssignANIM_ST_ANIMATE_HIDDEN();
        if (inst) {
            inst->GetPos((m3dV *)lastPos);
            *(m3dV *)lastPos2 = *(m3dV *)lastPos;
        }
        break;
    case 0x3FB:                                     // a matrix to apply to the instance
        if (data && inst)
            inst->Transform((m3dMATR *)data, 2);
        break;
    case 0x3FC:                                     // answered "no" here, and the chain stops
        *ret = 0;
        return 1;
    }
    entIACTIVE_OBJ::PostProcessMsg(id, data, from, ret);
    return 0;
}

// v12 0x00506DC0  (feb 0x004F4780)
// The IActive object's IActions, then the entity's own kind: "ON_ACTION_FRAME[_\"sequence\"][_n]" sections fire
// when the instance crosses action frame n (any when absent) of that sequence (any when absent).
void entENTITY::ParseIActions(psSHEET *ps)
{
    if (!ps)
        return;
    entIACTIVE_OBJ::ParseIActions(ps);
    for (int i = 0; i < ps->nSect; i++) {
        psSECT *sc = &ps->sect[i];
        char key[0x30];
        strcpy(key, "ON_ACTION_FRAME");
        if (!strstr(sc->name, key))
            continue;
        int frame = -1;
        char seq[0x40];
        seq[0] = 0;
        char *p = sc->name + strlen(key);
        if (*p == '_') {
            if (p[1] == '"') {
                char *q = strchr(p + 2, '"');
                if (q) {
                    int n = (int)(q - p);
                    strncpy(seq, p + 2, n - 2);
                    seq[n - 2] = 0;
                    p = q + 1;
                }
            }
            if (*p == '_')
                sscanf(p, "_%d", &frame);
        }
        int k = AddIAction(ps, sc->name, 0x400);
        if (k != -1) {
            iActions[k].actionFrame = frame - 1;
            iActions[k].animSeq = inst->tpl->FindSeqName(seq);
        }
    }
}

// ---------------------------------------------------------------------------------------------------- slots

// v12 0x00506C00  (feb 0x004F56C0) [24] ParsePS
// The instance's ANIM_PROP: collision and hierarchy flags, render-off distance, and a constraint (follow) actor.
void entENTITY::ParsePS()
{
    if (!inst)
        return;
    int v;
    if (inst->GetInt((char *)"ANIM_PROP", (char *)"ANIM_ST_IGNORE_CDT_ALWAYS", &v) && v == 1)
        inst->flags |= 0x10;
    if (inst->GetBool((char *)"ANIM_PROP", (char *)"ANIM_ST_EXCLUDE_HH", &v) && v == 1)
        inst->flags |= 0x1000;
    float f;
    if (inst->GetFloat((char *)"ANIM_PROP", (char *)"renderOff", &f))
        inst->renderOff = f;
    char actor[0x40], obj[0x40];
    animINST *ci = 0;
    if (inst->GetStr((char *)"ANIM_PROP", (char *)"constraintActor", actor, 0x40)) {
        ci = gsScenePtr->FindInstName(actor);
        if (!ci)
            apLogErr((char *)"Actor %s. Cannot find constraint actor %s", inst->name ? inst->name : (char *)"", actor);
    }
    if (ci) {
        objOBJ *o = 0;
        if (inst->GetStr((char *)"ANIM_PROP", (char *)"constraintObj", obj, 0x40)) {
            o = objFindName(ci->root, obj);
            if (!o)
                apLogErr((char *)"Actor %s. Cannot find constraint object %s", inst->name ? inst->name : (char *)"", obj);
        }
        SetFollow(ci->ent, o, o != 0);
    }
    if (inst->GetBool((char *)"ANIM_PROP", (char *)"isExactCDT", &v) && v)
        gsScenePtr->SetStateYesInst(inst, 8);
}

// v12 0x00507200  (feb 0x004F4B90) [18] FloorPlace
// Drop the instance onto the floor below it (within 50 units), yOfs above it, and optionally take the light there.
BOOL32 entENTITY::FloorPlace(float yOfs, int applyColor, cdtINFO *ci)
{
    cdtINFO local;
    memset(&local, 0, sizeof(local));               // (point = m3dVZero)
    cdtREFINE ref(2, inst);
    struct { const void *vtbl; int type; m3dV mn, mx; } box = { m3dBOX_vftable, 1, { 0, 0, 0 }, { 0, 0, 0 } };
    if (!ci)
        ci = &local;
    m3dV p;
    inst->GetPos(&p);
    inst->GetBBox((m3dBOX *)&box);
    if (!gsScenePtr->FloorPlace(&p, 50.0f, &p, box.mx.y - p.y, 0, 0, &ref, ci))
        return 0;
    p.y = p.y + yOfs;
    inst->Translate(&p, 0);
    if (applyColor) {
        m3dCOLOR col = { 0, 0, 0, 0 }, dyn = { 0, 0, 0, 0 };
        gsEffEnvirColor(ci, &col, &dyn);
        ApplyColor(&col, &dyn, 0, 0);
    }
    return 1;
}

// v12 0x00507370  (feb 0x004F4D00)
// Take the light of the floor under the entity without moving it.
void entENTITY::ApplyColor()
{
    m3dV p;
    inst->GetPos(&p);
    FloorPlace(0.0f, 1, 0);
    inst->Translate(&p, 0);
}

// v12 0x00507D00 (feb 0x004F5690) [21] GetCenter
void entENTITY::GetCenter(m3dV *out)
{
    if (!inst) {
        *out = m3dVZero;
        return;
    }
    inst->GetCenter(out);
}

// v12 0x00507D30 (feb 0x004F7EF0) [22] AssignANIM_ST_ANIMATE_HIDDEN
// A networked entity whose instance animates while hidden (flag 0x40) keeps animating (flag 4).
void entENTITY::AssignANIM_ST_ANIMATE_HIDDEN()
{
    if (mpId == -1 || mpId == -3 || !inst)
        return;
    if (inst->flags & 0x40)
        inst->flags |= 4;
}

// ---------------------------------------------------------------------------------------------------- following

// v12 0x00506F60  (feb 0x004F4910)
// Follow another entity (or one of its objects); keepOffset: keep the current offset to it.
void entENTITY::SetFollow(entENTITY *target, objOBJ *bone, int keepOffset)
{
    followEnt = 0;
    followObj = 0;
    delete (m3dMATR *)followOffset;
    followOffset = 0;
    followEnt = target;
    followObj = bone;
    if (!target)
        return;
    flags |= MSGADDR_WANT_DESTROY_NOTIFY;           // hear when the target goes away
    if (!keepOffset)
        return;
    inst->Validate(4);
    target->inst->Validate(4);
    m3dMATR *m = new m3dMATR;
    followOffset = (float *)m;
    float x, y, z, tx, ty, tz;
    ((m3dMATR *)inst->matr)->GetTranslate(&x, &y, &z);
    if (followObj)
        ((m3dMATR *)followObj->ltm)->GetTranslate(&tx, &ty, &tz);
    else
        ((m3dMATR *)target->inst->matr)->GetTranslate(&tx, &ty, &tz);
    m3dV d = { x - tx, y - ty, z - tz };
    m->Identity();
    m->Translate(&d, 0);
}

// v12 0x005070A0  (feb 0x004F4A50)
void entENTITY::TermFollow()
{
    followEnt = 0;
    followObj = 0;
    delete (m3dMATR *)followOffset;
    followOffset = 0;
}

// v12 0x005070E0  (feb 0x004F4A90)
// Every frame: with an offset only the position follows; without one the whole matrix is the target's. Not while
// this instance is off (flag 1) or the target's is (flags 1 | 2), unless forced.
void entENTITY::UpdateFollow(int force)
{
    if (!followEnt)
        return;
    if (!force && ((inst->flags & 1) || (followEnt->inst->flags & 3)))
        return;
    const float *src;
    if (followObj) {
        followEnt->inst->Validate(4);
        src = followObj->ltm;
    } else
        src = followEnt->inst->matr;
    m3dMATR m;
    memcpy(&m, src, sizeof(m));
    if (followOffset) {
        float x, y, z, ox, oy, oz;
        m.GetTranslate(&x, &y, &z);
        ((m3dMATR *)followOffset)->GetTranslate(&ox, &oy, &oz);
        m3dV p = { x + ox, y + oy, z + oz };
        inst->Translate(&p, 0);
        return;
    }
    m3dMATR out;
    out = m;                                        // (v12 0x00563700 copies the matrix through its own ctor)
    inst->Transform(&out, 0);
}

// ---------------------------------------------------------------------------------------------------- light colour

static inline u32 entPackColor(const m3dCOLOR *c)
{
    return (((u32)(int)c->a << 8 | ((u32)(int)c->r & 0xFF)) << 8 | ((u32)(int)c->g & 0xFF)) << 8 | ((u32)(int)c->b & 0xFF);
}

static inline void entUnpackColor(u32 c, float *out)
{
    out[0] = (float)((c >> 16) & 0xFF);
    out[1] = (float)((c >> 8) & 0xFF);
    out[2] = (float)(c & 0xFF);
    out[3] = (float)(c >> 24);
}

static inline float entClamp255(float v)
{
    if (v < 0.0f)
        return 0.0f;
    if (v > 255.0f)
        return 255.0f;
    return v;
}

// v12 0x005073B0  (feb 0x004F4D40)
// The light of a floor point (lightmap texel and dynamic lights, gsEffEnvirColor) on the entity's model.
void entENTITY::ApplyColor(cdtINFO *floor, int blend)
{
    m3dCOLOR col = { 0, 0, 0, 0 }, dyn = { 0, 0, 0, 0 };
    gsEffEnvirColor(floor, &col, &dyn);
    ApplyColor(&col, &dyn, 0, blend);
}

// v12 0x00507430  (feb 0x004F4DC0)
// New colours (any may be 0): light = the colour the model's light goes to (the lightmap texel), add = what is added
// on top (the dynamic lights), spec = specular. When one changed the model is re-coloured at once or, with blend,
// over the next frames (UpdateColorBlend).
void entENTITY::ApplyColor(m3dCOLOR *light, m3dCOLOR *add, m3dCOLOR *spec, int blend)
{
    if (!inst || !inst->root)
        return;
    int changed = 0;
    if (add) {
        u32 c = entPackColor(add);
        if (colAmbient != c) {
            colAmbient = c;
            changed = 1;
        }
    }
    if (light) {
        u32 c = entPackColor(light);
        if (colDiffuse != c) {
            colDiffuse = c;
            changed = 1;
        }
    }
    if (spec) {
        u32 c = entPackColor(spec);
        if (colSpecular != c) {
            colSpecular = c;
            changed = 1;
        }
    }
    if (!changed)
        return;
    if (blend) {
        flags2 |= ENTF2_COLOR_BLEND;
        return;
    }
    flags2 &= ~ENTF2_COLOR_BLEND;
    UpdateColorBlend();
}

// v12 0x005075A0  (feb 0x004F4F30)
// The current light (+0xA8) moves to +0xA0 at 2 x per second while blending (done when every channel is within
// 0.5), or is set to it; the model's constant colour is then current + +0x9C, clamped to 0..255.
void entENTITY::UpdateColorBlend()
{
    if (!inst || !inst->root)
        return;
    float tgt[4], add[4];
    entUnpackColor(colDiffuse, tgt);
    entUnpackColor(colAmbient, add);
    if (flags2 & ENTF2_COLOR_BLEND) {
        float k = (0.5f - gsElapsedTime) * 2.0f, s = gsElapsedTime * 2.0f;
        for (int i = 0; i < 4; i++)
            color[i] = color[i] * k + tgt[i] * s;
        int done = 1;
        for (int i = 0; i < 4 && done; i++)
            done = fabsf(tgt[i] - color[i]) < 0.5f;
        if (done)
            flags2 &= ~ENTF2_COLOR_BLEND;
    } else {
        for (int i = 0; i < 4; i++)
            color[i] = tgt[i];
    }
    m3dCOLOR out;
    out.r = entClamp255(color[0] + add[0]);
    out.g = entClamp255(color[1] + add[1]);
    out.b = entClamp255(color[2] + add[2]);
    out.a = entClamp255(color[3] + add[3]);
    inst->root->SetConstColor(entPackColor(&out), 2, 8);
}

// v12 0x00507A80  (feb 0x004F5410)
// The entity's light (diffuse, or the current blend when asked) and ambient, and their clamped sum packed.
u32 entENTITY::GetColor(m3dCOLOR *dif, m3dCOLOR *amb, int current)
{
    m3dCOLOR d0, a0;
    if (!dif)
        dif = &d0;
    if (!amb)
        amb = &a0;
    if (current) {
        dif->r = color[0];
        dif->g = color[1];
        dif->b = color[2];
        dif->a = color[3];
    } else
        entUnpackColor(colDiffuse, &dif->r);
    entUnpackColor(colAmbient, &amb->r);
    m3dCOLOR s;
    s.r = entClamp255(dif->r + amb->r);
    s.g = entClamp255(dif->g + amb->g);
    s.b = entClamp255(dif->b + amb->b);
    s.a = entClamp255(dif->a + amb->a);
    return entPackColor(&s);
}

// ---------------------------------------------------------------------------------------------------- registry

// v12 0x00507D60  (feb 0x004F7F60)
// Register (or replace) an entity class: name -> factory, class id, message priority, default template.
// exactName 0: the registered name is a prefix (a level class "ai_mino_atrium" matches "ai_mino" ...).
// HOOK: mods add their entity types here before the level loads. NB (original): no capacity check.
BOOL32 __fastcall entRegisterClass(char *name, u32 classId, int priority, entCREATE_FN create, char *tplName, int exactName)
{
    int i, found = -1;
    for (i = 0; i < entNClass; i++) {
        entCLASS_INFO *c = &entClassTable[i];
        int diff = c->exactName ? strcmp(c->name, name) : strncmp(c->name, name, strlen(c->name));
        if (!diff) {
            found = i;
            break;
        }
    }
    int add = found == -1;
    if (add)
        i = entNClass;
    entCLASS_INFO *c = &entClassTable[i];
    strncpy(c->name, name, 0x40);
    c->name[0x3F] = 0;
    strncpy(c->tplName, tplName ? tplName : (char *)"", 0x40);
    c->classId = classId;
    c->tplName[0x3F] = 0;
    c->priority = priority;
    c->create = create;
    c->exactName = exactName;
    if (add)
        entNClass++;
    return 1;
}

// v12 0x00508600  (feb 0x004F5FD0)
// The class a name belongs to (exact, or the registered name as a prefix), -1 if none.
int __fastcall _entFindClass(char *name)
{
    for (int i = 0; i < entNClass; i++) {
        entCLASS_INFO *c = &entClassTable[i];
        int diff = c->exactName ? strcmp(c->name, name) : strncmp(c->name, name, strlen(c->name));
        if (!diff)
            return i;
    }
    return -1;
}

// v12 0x005085B0  (feb 0x004F5F80)
// The class's default template name into out (only when it has one).
BOOL32 __fastcall entGetClassTplName(char *cls, char *out)
{
    int i = _entFindClass(cls);
    if (i < 0)
        return 0;
    char *t = entClassTable[i].tplName;
    if (!strlen(t))
        return 0;
    strcpy(out, t);
    return 1;
}

// ---------------------------------------------------------------------------------------------------- creation

#pragma pack(push, 1)
// GS_MSG 0x403 (1.2; feb 0x402) the server tells the clients to create an entity (entRegister).
struct entMSG_MP_CREATE : gsMSG_MP_PTR {            // +0x04 data: the INIT data of the entity
    int             mpId;                           // +0x08
    char            name[0x40];                     // +0x0C instance name
    char            className[0x40];                // +0x4C
    char            tplName[0x40];                  // +0x8C
    short           tplObjId;                       // +0xCC animINST +0x130
    animCREATE_DATA cd;                             // +0xCE MakeCreateData, the scale moved out of the matrix
};
#pragma pack(pop)

// v12 0x00507FF0  (feb 0x004F5980)
// Puts a new entity in the message system and makes its instance ready: the template's transition table
// (slot +0x40 SetupAniSystem) when it has none, the enclosing volumes (OBB, the template's when it has them),
// the state machine, the LODs and the collision data; in a running level the level system hears of the
// instance (flags2 bit 1: created in game). Then INIT (with initData), and INIT_GAME / INIT_LEVEL / INIT_MAP as
// far as the game has got. A server tells its clients (0x403) and keeps the packed INIT data for late joiners.
void __fastcall entRegister(entENTITY *e, animINST *inst, void *initData)
{
    if (inst) {
        e->inst = inst;
        inst->ent = e;
    }
    msgSystem.InsertAddr(e, e->classId, 0);
    if (inst) {
        if (inst->tpl) {
            if (!inst->tpl->aniSystem)
                e->SetupAniSystem();
            e->inst->AbvCalcBVolEnclos(4, 2, -1.0f);
        }
        inst->CreateAnimSys();
    }
    objInitLOD(inst->root);
    inst->LODInit();
    inst->CDTInit();
    if (gsAppState & 0x40000) {
        e->flags2 |= 1;
        if (gsAppState & 0x40000)
            gsLvlInstLoadNotify(e->inst, 0);
    }
    u32 f2 = e->flags2;
    msgSystem.SendMsg(e, MSG_MSG_INIT, initData, 0);
    if (gsAppState & 4)
        msgSystem.SendMsg(e, 0x3EF, 0, 0);          // INIT_GAME
    if (gsAppState & 0x40000)
        msgSystem.SendMsg(e, 0x3F4, 0, 0);          // INIT_LEVEL
    if (gsAppState & 4)
        msgSystem.SendMsg(e, 0x3F1, 0, 0);          // INIT_MAP
    if (e->mpId == -1 || !(f2 & 1))
        return;
    if (gsSysMP->mode == 2 || gsSysMP->mode == 4) {
        entMSG_MP_CREATE m;
        m.mpId = -1;
        m.cd.f_0E = 0;
        animINST *i = e->inst;
        m.className[0] = 0;
        m.tplName[0] = 0;
        strcpy(m.name, i->name ? i->name : (char *)"");
        strcpy(m.className, i->className ? i->className : (char *)"");
        strcpy(m.tplName, i->tplName ? i->tplName : (char *)"");
        i->MakeCreateData(&m.cd);
        m.data = (msgDATA *)initData;
        m.mpId = e->mpId;
        m.tplObjId = (short)e->inst->tplObjId;
        float sx, sy, sz;
        m.cd.matr.GetScale(&sx, &sy, &sz);
        if (!(fabsf(sx - 1.0f) < entEps)) {
            m.cd.matr.RemoveScale();
            m.cd.scale.x = m.cd.scale.x * sx;
            m.cd.scale.y = m.cd.scale.y * sy;
            m.cd.scale.z = m.cd.scale.z * sz;
        }
        gsSysServer->SendMsgClientAll(0, 0x403, &m, 0x20000, -1, -3);
    }
    entPackInitDataPacked(e, (msgDATA *)initData);
}

// v12 0x00507EA0  (feb 0x004F5830)
// The entity of class cls for an existing instance (or none): the class's factory, its class id and priority,
// the instance's class and name; then entRegister. 1.2: the names are only set when there is an instance.
entENTITY *__fastcall entCreate(char *cls, animINST *inst, void *initData, int flags)
{
    int i;
    for (i = 0; i < entNClass; i++) {
        entCLASS_INFO *c = &entClassTable[i];
        int diff = c->exactName ? strcmp(c->name, cls) : strncmp(c->name, cls, strlen(c->name));
        if (!diff)
            break;
    }
    if (i == entNClass || i == -1) {
        apLogErr((char *)"Cannot find entity class %s (instName: %s)", cls, inst->name ? inst->name : (char *)"");
        return 0;
    }
    entENTITY *e = entClassTable[i].create(inst);
    if (!e)
        return 0;
    e->classId = entClassTable[i].classId;
    e->priority = entClassTable[i].priority;
    if (inst) {
        inst->SetNameClass(cls);
        e->name.SetName(inst->name ? inst->name : (char *)"");
    }
    e->entFlags |= flags;
    entRegister(e, inst, initData);
    return e;
}

// v12 0x00508330  (feb 0x004F5D10, its body; the 1.2 entry is a jump)
// The entity of class cls with a new instance of template tpl (the class's default template when tpl is empty;
// "-" for none) placed by cd. An empty class takes the template's class, else "dynamic".
entENTITY *__fastcall _entCreateProfile(scnSCENE *scn, char *cls, char *tpl, animCREATE_DATA *cd, void *initData, int flags)
{
    char tplCls[0x40];
    if (!tpl || !strlen(tpl))
        entGetClassTplName(cls, tpl);
    animINST *inst = 0;
    if (tpl && strcmp(tpl, "-")) {
        animTPL *t = wrEVCall<animTPL *>(gsSysLevel, 0x24, tpl);   // FindTplName
        if (!t)
            return 0;
        inst = scn->CreateInst(t, cd, -1);
        if (!inst)
            return 0;
        if (!strlen(cls)) {
            strcpy(tplCls, t->className[0] ? t->className : (char *)"dynamic");
            cls = tplCls;
        }
    }
    entENTITY *e = entCreate(cls, inst, initData, flags);
    if (!e) {
        if (inst)
            scn->DestroyInst(inst);
        return 0;
    }
    if (e->inst)
        e->inst->SetNameTpl(tpl);
    return e;
}

static void *_entCfgProfCallFr;                     // v12 0x00984E64 cfg cache of [Debug] Decomp_ProfCallFr

// v12 0x005082B0  (feb 0x004F5C80)
// (The [Debug] Decomp_ProfCallFr switch chose a profiled call; both ways are the same call in the release.)
entENTITY *__fastcall entCreate(scnSCENE *scn, char *cls, char *tpl, animCREATE_DATA *cd, void *initData, int flags)
{
    int prof = (int)scn;
    if (_apCfgReadBool(&_entCfgProfCallFr, (char *)"Debug", (char *)"Decomp_ProfCallFr", &prof) && prof)
        return _entCreateProfile(scn, cls, tpl, cd, initData, flags);
    return _entCreateProfile(scn, cls, tpl, cd, initData, flags);
}

// v12 0x00508450  (feb 0x004F5E20)
// As above with the instance made of the template's sub-object tplObjId; the template name is set on the
// instance before the entity is made.
entENTITY *__fastcall entCreate(scnSCENE *scn, char *cls, char *tpl, int tplObjId, animCREATE_DATA *cd, void *initData, int flags)
{
    char tplCls[0x40];
    if (!tpl || !strlen(tpl))
        entGetClassTplName(cls, tpl);
    animINST *inst = 0;
    if (tpl && strcmp(tpl, "-")) {
        animTPL *t = wrEVCall<animTPL *>(gsSysLevel, 0x24, tpl);   // FindTplName
        if (!t)
            return 0;
        inst = scn->CreateInst(t, cd, tplObjId);
        if (!inst)
            return 0;
        if (!strlen(cls)) {
            strcpy(tplCls, t->className[0] ? t->className : (char *)"dynamic");
            cls = tplCls;
        }
        inst->SetNameTpl(tpl);
    }
    entENTITY *e = entCreate(cls, inst, initData, flags);
    if (e)
        return e;
    if (inst)
        scn->DestroyInst(inst);
    return 0;
}

// ---------------------------------------------------------------------------------------------------- collision filters

// v12 0x00508930  (feb 0x004F6300)
// Only entities of the given classes (entENTITY::unkBC class bits), then the base rules.
BOOL32 entCDT_REFINE_INCL::IsInclInst(animINST *i)
{
    if (!i->ent)
        return 0;
    if (!cls.IsState(i->ent->unkBC))
        return 0;
    return cdtREFINE::IsInclInst(i);
}

// v12 0x00508980  (feb 0x004F6350)
// Entities not of the given classes, then the base rules. As in the game, an instance without an entity is
// left out too.
BOOL32 entCDT_REFINE_EXCL::IsInclInst(animINST *i)
{
    if (!i->ent)
        return 0;
    if (cls.IsState(i->ent->unkBC))
        return 0;
    return cdtREFINE::IsInclInst(i);
}

// ---------------------------------------------------------------------------------------------------- iteration

// v12 0x005086A0  (feb 0x004F6070)
// The next entity ('$TNE') in the global address list after prev (0: the first).
entENTITY *__fastcall entFindNext(entENTITY *prev)
{
    msgADDR *a = prev ? ((msgADDR *)prev)->GetNext() : msgSystem.GetFirstAddr();
    while (a && a->GetSign() != 0x454E5424)
        a = a->GetNext();
    return (entENTITY *)a;
}

// v12 0x005086E0
// Sequence slot i of the template takes the part of name after "_SEQ_" ("PWP_SEQ_IDLE" -> "IDLE").
void __fastcall _entAnimSetSeqName(animTPL *t, int i, char *name)
{
    strcpy(t->seq[i].name, strstr(name, "_SEQ_") + 5);
}

// v12 0x00508720  (feb 0x004F60F0)
// A grouped sequence: slot i of each group of the template's animSYSTEM_GROUP gets the name after "_SEQ_" with the
// group's suffix ("RUN_1_SHOULDER", "RUN_1_HIP"...); without groups (or slot 0) the plain name.
void __fastcall _entAnimSetSeqGroupName(animTPL *t, int i, char *name)
{
    char *p = strstr(name, "_SEQ_");
    animSYSTEM_GROUP *s = (animSYSTEM_GROUP *)t->aniSystem;
    if (!s || i <= 0) {
        strcpy(t->seq[i].name, p + 5);
        return;
    }
    for (int g = 0; g < s->nGroup; g++) {
        strcpy(t->seq[i + g].name, p + 5);
        strcat(t->seq[i + g].name, s->GetGroupSuffix(g));
    }
}

// v12 0x00508820  (feb 0x004F61F0)
// The template's sequences as the file had them (seq, n of them) go into the named slots the entity set up, each
// with its enclosing volume; a name the entity does not know is reported.
void __fastcall entAnimAssignTplSeq(animTPL *t, void *seqv, int n)
{
    animSEQ *seq = (animSEQ *)seqv;
    abvBVOL *bv = wrArrayNew<abvBVOL>(t->nSeq);
    for (int i = 0; i < n; i++) {
        int k = t->FindSeqName(seq[i].name);
        if (k == -1) {
            apMsg((char *)"Cannot find anim sequence %s for %s", seq[i].name, t->tplName);
            continue;
        }
        memcpy(&t->seq[k], &seq[i], sizeof(animSEQ));
        bv[k] = t->bvSeq[i];
    }
    if (t->bvSeq)
        wrDelete(t->bvSeq, 3);
    t->bvSeq = bv;
}

// v12 0x004031F0 (the shared "return 1")  slot +0x40
// No transition table of its own (the classes with sequences override it).
BOOL32 entENTITY::SetupAniSystem()
{
    return 1;
}

// v12 0x00508EB0  (feb ?)
// Still in the level's box, 50 m round it and 150 m more upward.
int __fastcall gsIsInstInLevelBox(scnSCENE *s, animINST *i)
{
    objOBJ *root = gsScenePtr->root;
    struct { const void *vtbl; int type; m3dV mn, mx; } box;
    box.vtbl = m3dBOX_vftable;
    box.type = 1;
    box.mn.x = root->bbox.min[0];
    box.mn.y = root->bbox.min[1];
    box.mn.z = root->bbox.min[2];
    box.mx.x = root->bbox.max[0];
    box.mx.y = root->bbox.max[1];
    box.mx.z = root->bbox.max[2];
    ((m3dBOX *)&box)->m3dBOX::Expand(50.0f);
    box.mx.y += 150.0f;
    m3dV p;
    i->GetPos(&p);
    return ((m3dBOX *)&box)->m3dBOX::IsBelongPoint(&p);
}

// v12 0x00403200 (the shared "return 0"): an entity has no floor contact of its own
BOOL32 entENTITY::GetFloorInfo(cdtINFO *)
{
    return 0;
}
