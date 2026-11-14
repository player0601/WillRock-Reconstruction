// lod.cpp -- engine.lib levels of detail of objects and instances (Will Rock 1.2, v12.exe; feb obj engine:lod.obj)
//
// v12 0x004F2FF0..0x004F425F. Two kinds of switch (lodLOD): an "&lod" object whose children are the levels
// (objLOD: each level shows one child, hiding the others with procState bit 1), and the template's "&lod"
// named selections (animLOD_SEL: each level shows one object set, hiding the rest with procState bit 2).
// entRegister builds them: objInitLOD on the instance's tree, then LODInit moves them into the instance's set.
// Every frame the renderer picks each switch's level from the camera distance (Apply, through LODApply).
// 1.2 changes: LODSetTemp scales the level by each switch's number of levels; LODInit has larger LOD distances
// for instances with flag 0x40. Not rebuilt here: the 1.2 skin-LOD builder v12 0x004F36E0 (an animTPL function
// called by gsLVL_SYSTEM at template load).

#include "engine.h"

static void *_lodCfgMaxLod;                             // v12 0x00978748 cfg cache of [Video] VIDEO_MaxLod
M3D_DECLARE_EPS_SQ();                                   // v12 0x004F39E0 (static init of the m3d header)

// ---------------------------------------------------------------------------------------------
// lodLOD
// ---------------------------------------------------------------------------------------------

// v12 0x004F2FF0  (feb 0x004DED30)
// The level for the camera: the first whose distance is beyond the camera's (the last if none), plus the
// [Video] VIDEO_MaxLod bias. A zoomed camera (FOV under 50 degrees) counts as distance 0.
void lodLOD::Apply(camCAMERA *cam)
{
    if (inst && (inst->flags & 1))
        return;
    float d;
    if (cam->angleHor < 50.0f) {
        d = 0;
    } else {
        m3dV eye, c;
        cam->GetOrigin(&eye);
        GetLodCenter(&c);
        float dx = c.x - eye.x, dy = c.y - eye.y, dz = c.z - eye.z;
        d = m3dSqrt(dx * dx + dy * dy + dz * dz);
    }
    int n = nLod, lv = -1, i;
    for (i = 0; i < n; i++) {
        if (d < lev[i].dist && (i == 0 || !(d < lev[i - 1].dist))) {
            lv = i;
            break;
        }
    }
    if (i == n)
        lv = n - 1;
    int bias;                                           // (read uninitialised when the key is missing)
    _apCfgReadInt(&_lodCfgMaxLod, (char *)"Video", (char *)"VIDEO_MaxLod", &bias);
    lv += bias;
    if (lv >= nLod)
        lv = nLod - 1;
    if (lv != cur)
        SetLodNmb(lv);
}

// v12 0x004F3180  (feb 0x004DEED0)
// n: -3 shows every level (slot +0x0C with -3), -2 re-applies the current one, -1 the lowest detail, else
// clamped to [0, nLod).
void lodLOD::SetLodNmb(int n)
{
    if (n == -3) {
        _SetLodNmb(n);
    } else if (n == -2) {
        _SetLodNmb(cur);
        return;
    } else {
        if (n == -1)
            n = nLod - 1;
        else if (n >= nLod)
            n = nLod - 1;
        else if (n < 0)
            n = 0;
        _SetLodNmb(n);
        cur = n;
    }
    if (inst)
        inst->Invalidate(0x200);
}

// v12 0x004F31F0  (feb 0x004DEF40)
// The distances: maxDist * ((i + 1) / n) ^ gamma.
int lodLOD::Init()
{
    for (int i = 0; i < nLod; i++)
        lev[i].dist = (float)pow((double)((float)(i + 1) / (float)nLod), (double)gamma) * maxDist;
    SetLodNmb(0);
    return 1;
}

// v12 0x004F3250  (feb 0x004DEFA0)
void *lodLOD::GetDataPtr(int n)
{
    return lev[n].data;
}

// v12 0x00401380 (the shared empty function)
void lodLOD::Term()
{
}

// ---------------------------------------------------------------------------------------------
// objLOD
// ---------------------------------------------------------------------------------------------

// v12 0x004F3AF0  (feb 0x004DF8B0) qsort: the level objects by face count, most first
static int __cdecl _lodCmpLODObj(const void *a, const void *b)
{
    int fa = ((objOBJ *)((void **)a)[1])->nFace;
    int fb = ((objOBJ *)((void **)b)[1])->nFace;
    if (fa < fb)
        return 1;
    return (fa <= fb) - 1;
}

// v12 0x004F3260  (feb 0x004DEFB0)
// The levels are the objects of the "&lod" tree that have vertices, by face count; [LOD] maxDist / gamma from
// the owning instance, then from the object's own properties.
int objLOD::Init(objOBJ *o)
{
    objOBJ *list[40];
    int n = objMakePtrList(&o, 1, list, 40);
    if (n <= 1)
        return 0;
    inst = o->inst;
    nLod = 0;
    for (int i = 0; i < n; i++) {
        if (list[i]->nVert > 0) {
            lev[nLod].data = list[i];
            nLod++;
        }
    }
    qsort(lev, nLod, 8, _lodCmpLODObj);
    if (o->inst) {
        o->inst->GetFloat((char *)"LOD", (char *)"maxDist", &maxDist);
        o->inst->GetFloat((char *)"LOD", (char *)"gamma", &gamma);
    }
    if (o->props) {
        o->props->GetFloat((char *)"LOD", (char *)"maxDist", &maxDist);
        o->props->GetFloat((char *)"LOD", (char *)"gamma", &gamma);
    }
    for (int i = 0; i < nLod; i++)
        lev[i].dist = (float)pow((double)((float)(i + 1) / (float)nLod), (double)gamma) * maxDist;
    SetLodNmb(0);
    return 1;
}

// v12 0x004F33D0  (feb 0x004DF120)
// procState bit 1 hides an object: -3 shows all levels, else only level n is shown.
void objLOD::_SetLodNmb(int n)
{
    if (n == -3) {
        for (int i = 0; i < nLod; i++)
            ((objOBJ *)lev[i].data)->procState &= ~1u;
        return;
    }
    for (int i = 0; i < nLod; i++)
        ((objOBJ *)lev[i].data)->procState |= 1;
    ((objOBJ *)lev[n].data)->procState &= ~1u;
}

// v12 0x004F3440  (feb 0x004DF190)
void objLOD::GetLodCenter(m3dV *out)
{
    if (inst) {
        inst->GetCenter(out);
        return;
    }
    ((objOBJ *)lev[0].data)->GetCenter(out);
}

// ---------------------------------------------------------------------------------------------
// animLOD_SEL
// ---------------------------------------------------------------------------------------------

// v12 0x004F3460  (feb 0x004DF1B0)
void animLOD_SEL::GetLodCenter(m3dV *out)
{
    inst->GetCenter(out);
}

// v12 0x004F3470  (feb 0x004DF1C0)
// procState bit 2 hides an object: level 0 (or -3) shows the full set; another level hides level 0's set and
// shows its own.
void animLOD_SEL::_SetLodNmb(int n)
{
    if (n == 0 || n == -3) {
        objSET *s = (objSET *)lev[0].data;
        for (int i = 0; i < s->n; i++)
            s->obj[i]->procState &= ~2u;
        return;
    }
    objSET *s = (objSET *)lev[0].data;
    for (int i = 0; i < s->n; i++)
        s->obj[i]->procState |= 2;
    s = (objSET *)lev[n].data;
    for (int i = 0; i < s->n; i++)
        s->obj[i]->procState &= ~2u;
}

// v12 0x004F3B20  (feb 0x004DF8E0) qsort: the selections by object count, most first
static int __cdecl _lodCmpLODSel(const void *a, const void *b)
{
    int na = ((objSET *)((void **)a)[1])->n;
    int nb = ((objSET *)((void **)b)[1])->n;
    if (na < nb)
        return 1;
    return (na <= nb) - 1;
}

// v12 0x004F3500  (feb 0x004DF250)
// One level per "&lod" named selection of the template (the selection's objects found by name in the
// instance); a switch needs at least three levels.
int animLOD_SEL::Init(animINST *i)
{
    inst = i;
    nLod = 0;
    for (int k = 0; k < inst->tpl->nSel; k++) {
        selSEL *sel = &inst->tpl->sel[k];
        if (!strstr(sel->name, "&lod"))
            continue;
        objSET *s = new objSET;
        s->max = 0;
        s->n = 0;
        s->obj = 0;
        s->Alloc(sel->n);
        for (int j = 0; j < sel->n; j++) {
            objOBJ *o = objFindName(inst->root, sel->obj[j].name);
            if (o)
                s->AddObj(o);
        }
        lev[nLod].data = s;
        nLod++;
    }
    if (nLod <= 2)
        return 0;
    qsort(lev, nLod, 8, _lodCmpLODSel);
    inst->GetFloat((char *)"LOD", (char *)"maxDist", &maxDist);
    inst->GetFloat((char *)"LOD", (char *)"gamma", &gamma);
    for (int k = 0; k < nLod; k++)
        lev[k].dist = (float)pow((double)((float)(k + 1) / (float)nLod), (double)gamma) * maxDist;
    SetLodNmb(0);
    return 1;
}

// v12 0x004F36B0  (feb 0x004DF400)
void animLOD_SEL::Term()
{
    for (int i = 0; i < nLod; i++)
        if (lev[i].data)
            ((objSET *)lev[i].data)->__vecDelDtor(1);
}

// ---------------------------------------------------------------------------------------------
// building the switches
// ---------------------------------------------------------------------------------------------

// v12 0x004F3A00  (feb 0x004DF7C0)
// Every "&lod" object of the tree gets an objLOD (kept at objOBJ +0xF4 until LODInit takes it).
int __fastcall objInitLOD(objOBJ *o)
{
    objOBJ *c = 0;
    if (o) {
        c = o->child;
        if (o->name && strstr(o->name, "&lod")) {
            objLOD *l = new objLOD;
            if (!l || !l->Init(o))
                return 0;
            o->lod = l;
        }
    }
    objOBJ *k = c;
    int looped = 0;
    while (c && (!looped || k != c)) {
        if (!objInitLOD(k))
            return 0;
        k = k->next;
        looped = 1;
    }
    return 1;
}

// v12 0x004F3B50  (feb 0x004DF910)
// The instance's LOD set: the objLODs of its tree (taken off the objects), then one animLOD_SEL for the
// template's "&lod" selections; then the distances for skipping animation / matrices / blending
// (1.2: 200 / 1000 / 1000 with instance flag 0x40, else 50 / 12 / 20; [LOD] overrides).
void animINST::LODInit()
{
    if (!root)
        return;
    animLODS *s = (animLODS *)operator new(0x200);
    if (s) {
        s->saved = 0xFFFFFFFF;
        s->flags = 0;
        s->n = 0;
        s->scale = 0;
        s->unk1F0 = -100.0f;
    }
    lods = s;
    objOBJ *c = 0;
    if (root) {
        objLOD *l = root->lod;
        c = root->child;
        if (l) {
            if ((u32)s->n < 0x3C) {
                s->lod[s->n] = l;
                s->n++;
                if (l->nLod > s->scale)
                    s->scale = l->nLod;
            }
            root->lod = 0;
        }
    }
    objOBJ *k = c;
    int looped = 0;
    while (c && (!looped || k != c)) {
        OBJ_LODInit(k);
        k = k->next;
        looped = 1;
    }
    for (int i = 0; i < tpl->nSel; i++) {
        if (!strstr(tpl->sel[i].name, "&lod"))
            continue;
        animLOD_SEL *l = new animLOD_SEL;
        if (!l->Init(this)) {
            operator delete(l);
            break;
        }
        animLODS *ls = lods;
        if ((u32)ls->n < 0x3C) {
            ls->lod[ls->n] = l;
            ls->n++;
            if (l->nLod > ls->scale)
                ls->scale = l->nLod;
        }
        break;
    }
    if (flags & 0x40) {
        lods->distAnim = 200.0f;
        lods->distOffMatrModel = 1000.0f;
        lods->distOffAnimBlend = 1000.0f;
    } else {
        lods->distAnim = 50.0f;
        lods->distOffMatrModel = 12.0f;
        lods->distOffAnimBlend = 20.0f;
    }
    GetFloat((char *)"LOD", (char *)"distLODAnim", &lods->distAnim);
    GetFloat((char *)"LOD", (char *)"distLODOffMatrModel", &lods->distOffMatrModel);
    GetFloat((char *)"LOD", (char *)"distLODOffAnimBlend", &lods->distOffAnimBlend);
}

// v12 0x004F3D90  (feb 0x004DFB50)
void animINST::LODTerm()
{
    if (!lods)
        return;
    for (int i = 0; i < lods->n; i++) {
        lods->lod[i]->Term();
        operator delete(lods->lod[i]);
    }
    operator delete(lods);
    lods = 0;
}

// v12 0x004F3E00  (feb 0x004DFBC0)
void animINST::OBJ_LODInit(objOBJ *o)
{
    objOBJ *c = 0;
    if (o) {
        objLOD *l = o->lod;
        c = o->child;
        if (l) {
            animLODS *s = lods;
            if ((u32)s->n < 0x3C) {
                s->lod[s->n] = l;
                s->n++;
                if (l->nLod > s->scale)
                    s->scale = l->nLod;
            }
            o->lod = 0;
        }
    }
    objOBJ *k = c;
    int looped = 0;
    while (c && (!looped || k != c)) {
        OBJ_LODInit(k);
        k = k->next;
        looped = 1;
    }
}

// v12 0x004F3EA0  (feb 0x004DFC60)
// Per frame (unless the set is temporary or frozen): every switch picks its level, then the distance flags of
// the instance: 0x200000 beyond distLODAnim, 0x400000 beyond distLODOffMatrModel, 0x800000 beyond
// distLODOffAnimBlend.
void animINST::LODApply(camCAMERA *cam)
{
    if (!lods || (lods->flags & 5))
        return;
    for (int i = 0; i < lods->n; i++)
        lods->lod[i]->Apply(cam);
    m3dV c, eye;
    GetCenter(&c);
    cam->GetOrigin(&eye);
    float dx = c.x - eye.x, dy = c.y - eye.y, dz = c.z - eye.z;
    float d = m3dSqrt(dx * dx + dy * dy + dz * dz);
    animLODS *s = lods;
    if (d <= s->distAnim)
        flags &= ~0x200000u;
    else
        flags |= 0x200000;
    if (d <= s->distOffMatrModel)
        flags &= ~0x400000u;
    else
        flags |= 0x400000;
    if (d <= s->distOffAnimBlend)
        flags &= ~0x800000u;
    else
        flags |= 0x800000;
}

// v12 0x004F4040  (feb 0x004DFE00)
// Switch every LOD of the instance to a temporary level (absolute, or relative to the current one),
// remembering the current levels and the instance state for LODRestore. 1.2 scales the level by the
// number of levels of each LOD over the set's scale.
void animINST::LODSetTemp(int lod, int absolute)
{
    animLODS *s = lods;
    if (!s || !s->n || (s->flags & 1))
        return;
    s->flags |= 1;
    s->saved = invalid;
    for (int i = 0; i < s->n; i++) {
        int lv;
        if (!absolute) {
            lv = s->lod[i]->cur + lod;
            if (lv < 0)
                lv = 0;
            else if (lv >= s->lod[i]->nLod)
                lv = s->lod[i]->nLod - 1;
        } else {
            lv = lod;
        }
        s->savedLod[i] = s->lod[i]->cur;
        int n = (lv == -1) ? -1 : (int)((float)s->lod[i]->nLod * (float)lv / (float)s->scale);
        s->lod[i]->SetLodNmb(n);
        s = lods;
    }
}

// v12 0x004F4120  (feb 0x004DFEE0)
void animINST::LODRestore()
{
    animLODS *s = lods;
    if (!s || !s->n || !(s->flags & 1))
        return;
    s->flags &= ~1u;
    if (!s->n)
        return;
    for (int i = 0; i < s->n; i++)
        s->lod[i]->SetLodNmb(s->savedLod[i]);
    u32 st = invalid;
    invalid = st ^ ((s->saved ^ st) & 0x1E03);           // restore the LOD-related state bits
}

// v12 0x004F41A0  (feb 0x004DFF60)
// Off: every level shown (-3), flag 2; on again: the current levels re-applied (-2).
void animINST::LODEnable(int on)
{
    animLODS *s = lods;
    if (!s || !s->n)
        return;
    if (!on) {
        if (s->flags & 2)
            return;
        s->flags |= 2;
    } else {
        s->flags &= ~2u;
    }
    for (int i = 0; i < lods->n; i++)
        lods->lod[i]->SetLodNmb(on ? -2 : -3);
}

// v12 0x004F4210  (feb 0x004DFFD0)
void animINST::LODFreeze(int on)
{
    animLODS *s = lods;
    if (!s || !s->n)
        return;
    if (on)
        s->flags |= 4;
    else
        s->flags &= ~4u;
}

// v12 0x004F4240  (feb 0x004E0000)
void *objSET::__vecDelDtor(unsigned f)
{
    apFree(obj);
    if (f & 1)
        operator delete(this);
    return this;
}
