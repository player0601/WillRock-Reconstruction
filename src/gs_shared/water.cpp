// water.cpp -- gs_shared.lib water and lava volumes (Will Rock 1.2, v12.exe; feb obj gs_shared:water.obj)
//
// A water (or lava) entity is a template of class "water"/"lava" whose objects are the surfaces (with a
// [WATER] script: waves maxAmp/freqXZ/freqTime, texture scrolling and animation, a REFLECTION section) and "&fog"
// objects that outline its volume: they become the domains of a ground-fog shader (pteFOG_GROUND, [FOG] depth =
// how far below the surface the volume reaches). In 1.2 the water entities register in a fixed table of 30 that the
// player's queries walk (waterIsUnderWater). Everything of water.obj is rebuilt (v12 0x0054B0F0..0x0054BCCC).

#include <stdio.h>
#include <string.h>
#include "../Sources/game.h"
#include "../gs/gs_mp_server.h"
#include "../wr_vcall.h"

waterLIQUID *waterTable[30];                // v12 0x009A89F8..0x009A8A70 (feb 0x0095D6E8)
static txmTEXTURE *waterTex[50];            // v12 0x009A8A70 the texture list InitObj builds (copied by SetParams)
static void *waterCfgCache;                 // v12 0x009A8B38 [Video] VIDEO_WaterReflCaust lookup
static float waterEpsSq = 1e-6f * 1e-6f;    // v12 0x009A8B3C (initialiser v12 0x0054BA10)

// v12 0x0054B0F0  (feb 0x0052FED0)
int __fastcall waterInit(void)
{
    if (!entRegisterClass((char *)"water", 0x24575452, 0x2D, waterWATER::Create, 0, 1))
        return 0;
    return entRegisterClass((char *)"lava", 0x4C415641, 0x2D, waterLAVA::Create, 0, 1) != 0;
}

// v12 0x0054B130  (1.2 only, no feb twin; name ours)
// Is p under the surface of an active water volume (entity class bit 1); *level = that surface's height.
BOOL32 __fastcall waterIsUnderWater(m3dV *p, float *level, float r)
{
    for (waterLIQUID **w = waterTable; w < waterTable + 30; w++) {
        if (*w && ((*w)->unkBC & 1) && (*w)->IsUnderWater(p, r, level))
            return 1;
    }
    return 0;
}

// v12 0x0054B180  (feb 0x0052FF60)
// INIT: declared to the MP server, into the table. TERM: out of it. INIT_LEVEL: every surface object of the
// instance set up (InitObj, its sheet falling back to the template's), the "&fog" objects (or, without any, the
// surfaces) made the volumes of a [FOG] shader, the box and the bounding volume grown by the waves, and a
// collision grid of 3 x 3 cells (30 x 30 at most) for the surface.
BOOL32 waterLIQUID::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case MSG_MSG_INIT:
        gsSysServer->DeclSyncable(this);
        for (int i = 0; i < 30; i++)
            if (!waterTable[i]) {
                waterTable[i] = this;
                break;
            }
        return 0;
    case MSG_MSG_TERM:
        for (int i = 0; i < 30; i++)
            if (waterTable[i] == this) {
                waterTable[i] = 0;
                break;
            }
        return 0;
    case GS_MSG_INIT_LEVEL: {
        objOBJ *objs[0x32], *fogs[0x32];
        psSHEET *tps = inst->tpl->props;
        int n = objMakePtrList(&inst->root, 1, objs, 0x32);
        int nFog = 0;
        for (int i = 0; i < n; i++) {
            objOBJ *o = objs[i];
            objs[i] = 0;
            if (!o->nVert)
                continue;
            if (strstr(o->name, "&fog")) {
                fogs[nFog++] = o;
                continue;
            }
            if (strstr(o->name, "&caust"))
                continue;
            psSHEET *ps = o->props;
            if (!ps)
                ps = tps;
            else
                ps->parent = tps;
            if (!InitObj(o, ps))
                return 0;
            objs[i] = o;
        }
        if (tps && tps->FindSect((char *)"FOG")) {
            fog = new pteFOG_GROUND(0);
            if (!fog)
                return 0;
            fog->priority = 0x32;
            fog->SetInst(inst);
            fog->SetSheet(tps, (char *)"FOG");
            for (int i = 0; i < nFog; i++)
                fog->SetDomainObj(fogs[i]);
            if (!fog->nDomain)
                for (int i = 0; i < n; i++)
                    if (objs[i])
                        fog->SetDomainObj(objs[i]);
            fog->Register();
        }
        inst->Validate(2);
        wrVCall<void>(&inst->root->bbox, 0x2C, maxAmp);                 // m3dBOX::Expand
        m3dV vox = { 3.0f, -1.0f, 3.0f };
        int maxVox[3] = { 30, 1, 30 };
        if (!inst->CDTInit(&vox, (m3dVINT *)maxVox, 0, 0.0f))
            return 0;
        void *upd = *(void **)((char *)inst->bvCur + 8);                // abvBVOL::upd
        wrVCall<void>(upd, 0x0C, maxAmp);                               // abvVOL_UPD::Expand
        return 0;
    }
    }
    return 0;
}

// v12 0x0054B460  (feb 0x00530240)
// One surface's [WATER] keys: waves (maxAmp > 0: objMOD_NOISE_DEFORM with freqXZ / freqTime / maxDistUpdate, the
// boundary held still over fixDistance unless isFixBoundary is No), texture waves (maxAmpUV), the detail layer's
// tiny coefficient and additive drawing (isAdditiveDet), base and detail scrolling, base and detail texture
// animations (nTexBase / nameTexBase_<i> over texAnimTimeBase, nTexDet / nameTexDet_<i> over texAnimTimeDet).
BOOL32 waterLIQUID::InitObj(objOBJ *o, psSHEET *ps)
{
    float maxDistUpdate = 60.0f, amp = 0.0f, freqXZ = 1.0f, freqTime = 1.0f;
    ps->GetFloat((char *)"WATER", (char *)"maxDistUpdate", &maxDistUpdate);
    ps->GetFloat((char *)"WATER", (char *)"maxAmp", &amp);
    ps->GetFloat((char *)"WATER", (char *)"freqXZ", &freqXZ);
    ps->GetFloat((char *)"WATER", (char *)"freqTime", &freqTime);
    if (!(amp <= 1e-6f)) {
        objMOD_NOISE_DEFORM *m = new objMOD_NOISE_DEFORM;
        m->SetParams(amp, freqXZ, freqTime, maxDistUpdate);
        m->AddObj(o);
        int fix = 1;
        ps->GetBool((char *)"WATER", (char *)"isFixBoundary", &fix);
        if (fix) {
            float fixDist = 1.0f;
            ps->GetFloat((char *)"WATER", (char *)"fixDistance", &fixDist);
            if (!m->FixBoundary(fixDist))
                apLogErr((char *)"Water Fix Boundary Failed!!!!!!!");
        }
    }
    if (!(maxAmp > amp))
        maxAmp = amp;
    float ampUV = 0.0f, freqXZUV = 1.0f, freqTimeUV = 1.0f;
    ps->GetFloat((char *)"WATER", (char *)"maxAmpUV", &ampUV);
    ps->GetFloat((char *)"WATER", (char *)"freqXZUV", &freqXZUV);
    ps->GetFloat((char *)"WATER", (char *)"freqTimeUV", &freqTimeUV);
    if (!(ampUV <= 1e-6f)) {
        objMOD_NOISE_DEFORM_UV *m = new objMOD_NOISE_DEFORM_UV(0);
        m->SetParams(ampUV, freqXZUV, freqTimeUV, maxDistUpdate);
        m->AddObj(o);
    }
    objMTL *det = o->GetSplitMtlRendMtl(0, 2);
    if (det) {
        det->_0c = 1e-4f;
        int additive = 1;
        ps->GetBool((char *)"WATER", (char *)"isAdditiveDet", &additive);
        if (additive)
            o->_SetStateRendYes(0x1000);
    }
    float u, v;
    if (!ps->GetFloat((char *)"WATER", (char *)"scrollUSpeedBase", &u))
        u = 0;
    if (!ps->GetFloat((char *)"WATER", (char *)"scrollVSpeedBase", &v))
        v = 0;
    objMOD_TEX_SCROLL *s = new objMOD_TEX_SCROLL(0);
    s->uSpeed = u;
    s->vSpeed = v;
    s->AddObj(o);
    if (det) {
        if (!ps->GetFloat((char *)"WATER", (char *)"scrollUSpeedDet", &u))
            u = 0;
        if (!ps->GetFloat((char *)"WATER", (char *)"scrollVSpeedDet", &v))
            v = 0;
        s = new objMOD_TEX_SCROLL(2);
        s->uSpeed = u;
        s->vSpeed = v;
        s->AddObj(o);
    }
    int nTex = 0;
    char name[0x40];
    float animTime;
    ps->GetInt((char *)"WATER", (char *)"nTexBase", &nTex);
    name[0] = 0;
    ps->GetStr((char *)"WATER", (char *)"nameTexBase", name, 0x40);
    if (nTex > 0 && strlen(name)) {
        for (int i = 0; i < nTex; i++) {
            sprintf(apBuffer, "%s_%d", name, i);
            waterTex[i] = txmManager->Add(apBuffer, 0x100, 1);
            if (!waterTex[i])
                apLogErr((char *)"Cannot find water texture %s", apBuffer);
        }
        animTime = 2.0f;
        ps->GetFloat((char *)"WATER", (char *)"texAnimTimeBase", &animTime);
        objMOD_TEX_SEQ_ANIM *a = new objMOD_TEX_SEQ_ANIM(0);
        a->SetParams(nTex, waterTex, animTime);
        a->AddObj(o);
    }
    if (det) {
        nTex = 0;
        ps->GetInt((char *)"WATER", (char *)"nTexDet", &nTex);
        name[0] = 0;
        ps->GetStr((char *)"WATER", (char *)"nameTexDet", name, 0x40);
        if (nTex > 0 && strlen(name)) {
            for (int i = 0; i < nTex; i++) {
                sprintf(apBuffer, "%s_%d", name, i);
                waterTex[i] = txmManager->Add(apBuffer, 0x110, 1);
                if (!waterTex[i])
                    apLogErr((char *)"Cannot find water texture %s", apBuffer);
            }
            animTime = 2.0f;
            ps->GetFloat((char *)"WATER", (char *)"texAnimTimeDet", &animTime);
            objMOD_TEX_SEQ_ANIM *a = new objMOD_TEX_SEQ_ANIM(2);
            a->SetParams(nTex, waterTex, animTime);
            a->AddObj(o);
        }
    }
    return 1;
}

// v12 0x0054BA30  (feb 0x00530810)
// Is p inside one of the volume's domains: above its bottom, below the surface + wave amplitude + r, and
// within its outline. *level (may be 0) = the surface height + amplitude.
BOOL32 waterLIQUID::IsUnderWater(m3dV *p, float r, float *level)
{
    if (!fog)
        return 0;
    for (int i = 0; i < fog->nDomain; i++) {
        scnDOMAIN *d = &fog->domain[i];
        if (d->yMin < p->y && p->y < d->yMax + maxAmp + r && d->IsBelongPointXZ(p)) {
            if (level)
                *level = fog->domain[i].yMax + maxAmp;
            return 1;
        }
    }
    return 0;
}

// v12 0x0054BAE0  the liquid's, then the class bit 1 ("water") on INIT
BOOL32 waterWATER::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    waterLIQUID::ProcessMsg(id, data, from, ret);
    if (id == MSG_MSG_INIT)
        unkBC |= 1;
    return 0;
}

// v12 0x0054BB20  (feb 0x00530900)
// The liquid's set-up; the object drawn as water (unk0C 2, state 4); with [Video] VIDEO_WaterReflCaust and a
// REFLECTION section, a reflection shader on it.
BOOL32 waterWATER::InitObj(objOBJ *o, psSHEET *ps)
{
    if (!waterLIQUID::InitObj(o, ps))
        return 0;
    o->unk0C |= 2;
    o->state |= 4;
    int on = (int)(intptr_t)ps;                     // (the original reads into its argument slot: unread -> "on")
    _apCfgReadBool(&waterCfgCache, (char *)"Video", (char *)"VIDEO_WaterReflCaust", &on);
    if (on && ps->FindSect((char *)"REFLECTION")) {
        pteEMAP_REFLECT_WATER *r = new pteEMAP_REFLECT_WATER;
        if (!r)
            return 0;
        r->SetInst(inst);
        r->SetObj(o);
        r->SetSheet(ps, (char *)"REFLECTION");
        r->Register();
    }
    return 1;
}

// v12 0x0054BBE0  the liquid's, then the class bit 2 ("lava") on INIT
BOOL32 waterLAVA::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    waterLIQUID::ProcessMsg(id, data, from, ret);
    if (id == MSG_MSG_INIT)
        unkBC |= 2;
    return 0;
}

// v12 0x0054BC20  the liquid's, then unk0C 4 (drawn as lava)
BOOL32 waterLAVA::InitObj(objOBJ *o, psSHEET *ps)
{
    if (!waterLIQUID::InitObj(o, ps))
        return 0;
    o->unk0C |= 4;
    return 1;
}

// v12 0x0054BC50 / 0x0054BC90
entENTITY *__fastcall waterWATER::Create(animINST *a)
{
    waterWATER *w = new waterWATER;
    if (!w)
        return 0;
    w->maxAmp = 0;
    w->fog = 0;
    return w;
}

entENTITY *__fastcall waterLAVA::Create(animINST *a)
{
    waterLAVA *w = new waterLAVA;
    if (!w)
        return 0;
    w->maxAmp = 0;
    w->fog = 0;
    return w;
}
