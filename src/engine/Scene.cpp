// Scene.cpp -- engine.lib scnSCENE: the template / instance pools, level set-up and tear-down
// (Will Rock 1.2, v12.exe; feb obj engine:Scene.obj)
//
// v12 0x004E5E10..0x004E7690. The scene owns 300 templates (0xCC) and 1000 instances (0x14C) allocated up
// front; slot 0 of a pool entry is its magic ('LPTA' / 'SNIA'), 0 = free. Live instances are also on a ring
// (instFirst, links at animINST +0x58/+0x5C); physics/dynamic ones (flags 0x48) are put at its head.
// Also here (emitted in Scene.obj): the cdtCOLL_ANIM constructor and the instance-grid deleting destructor;
// the lstNODE_CACHE template members are in cdt_sys_anim.cpp.
// Not rebuilt yet: CreateInst(objOBJ*) (v12 0x004E6950, 1.2 changed the pool-full path) and GetAnimLights.

#include "engine.h"

#define SCN_MAX_TPL  300
#define SCN_MAX_INST 1000

static inline void scnUnlinkInst(scnSCENE *s, animINST *i)     // inline ring removal (CreateInst, SetStateYesInst)
{
    if (s->instFirst == i && s->instFirst->listNext == i) {
        i->listPrev = 0;
        i->listNext = 0;
        s->instFirst = 0;
    } else {
        i->listPrev->listNext = i->listNext;
        i->listNext->listPrev = i->listPrev;
        if (i == s->instFirst)
            s->instFirst = i->listNext;
        i->listPrev = 0;
        i->listNext = 0;
    }
}

// Clear a pool entry and make it a fresh instance (inlined in AllocInst / CreateInst).
static inline animINST *scnInitInst(scnSCENE *s, int idx)
{
    animINST *i = (animINST *)((u8 *)s->instPool + idx * SCN_INST_SIZE);
    memset(i, 0, SCN_INST_SIZE);
    i->magic = 0x41494E53;                              // 'SNIA'
    i->index = idx;
    ((m3dMATR *)i->matr)->Identity();
    i->scene = s;
    i->tplObjId = -1;
    *(u32 *)&i->frame = 0xC2EA0000;                     // -117.0
    i->Invalidate(0x84);
    return i;
}

// v12 0x004E5E10  (feb 0x004DD520)
scnSCENE::scnSCENE()
{
    flags = 0;
    memset(this, 0, 0x8C);
    tplPool = (animTPL *)apCalloc(sizeof(animTPL), SCN_MAX_TPL);
    if (!tplPool) {
        apFree(this);                                   // (as the original: frees itself)
        return;
    }
    instPool = (animINST *)apCalloc(SCN_INST_SIZE, SCN_MAX_INST);
    if (!instPool) {
        apFree(tplPool);
        apFree(this);
        return;
    }
    for (int k = 0; k < SCN_MAX_INST; k++)
        InstPtr(k)->index = k;
    _64 = 0;
    animTPL *t = 0;
    for (int k = 0; k < SCN_MAX_TPL; k++)               // inline AllocTpl
        if (!tplPool[k].magic) {
            t = &tplPool[k];
            memset(t, 0, sizeof(animTPL));
            t->magic = SCN_TPL_MAGIC;
            *(scnSCENE **)((u8 *)t + 0xC8) = this;
            break;
        }
    dummyTpl = t;
    if (t) {
        strncpy(t->tplName, "__DUMMY__", 0x3F);
        dummyTpl->tplName[0x3E] = 0;
        strncpy(dummyTpl->className, "", 0x3F);
        dummyTpl->className[0x3E] = 0;
        dummyTpl->flags |= 0x10;
        dummyTpl->flags |= 0x100;
    }
}

// v12 0x004E5F60  (feb 0x004DD670)
scnSCENE::~scnSCENE()
{
    DestroyLevel(1);
    for (int k = 0; k < SCN_MAX_INST; k++)
        if (InstPtr(k)->magic)
            DestroyInst(InstPtr(k));
    for (int k = 0; k < SCN_MAX_TPL; k++)
        if (tplPool[k].magic) {
            tplPool[k].DestroyData();
            tplPool[k].magic = 0;
        }
    apFree(tplPool);
    apFree(instPool);
    apFree(tmpV3D);
}

// v12 0x004E5FE0  (feb 0x004DD6F0)
int scnSCENE::IsEmptyInst(animINST *i)
{
    return i->magic == 0;
}

// v12 0x004E5FF0  (feb 0x004DD700)
animINST *scnSCENE::InstPtr(int i)
{
    return (animINST *)((u8 *)instPool + i * SCN_INST_SIZE);
}

// v12 0x004E6010  (feb 0x004DD720)  [callgraph: 1.2 CalcFaceNormals takes an argument]
// After loading: the flat object list (by idHier), the shared vertex buffer, frozen world vertices, face
// normals, boxes, and the instance grid (half the level grid's resolution, at least 20 high).
int scnSCENE::InitLevel()
{
    struct { const void *vtbl; int type; m3dV vMin, vMax; } box = { m3dBOX_vftable, 1 };
    nObj = root->GetNObj(0);
    objList = (objOBJ **)apRealloc(objList, nObj * 4);
    if (!objList)
        return 0;
    memset(objList, 0, nObj * 4);
    objMakePtrListID(root, objList, nObj);
    int nv = root->GetMaxNVert();
    if (nTmpV3D < nv) {
        tmpV3D = (m3dV *)apRealloc(tmpV3D, nv * sizeof(m3dV));
        if (!tmpV3D) {
            nTmpV3D = 0;
            return 0;
        }
        nTmpV3D = nv;
    }
    root->FreezeLTM();
    root->CalcFaceNormals(0);
    objCalcBBox(root);
    if (!coll)
        return 1;
    cdtCOLL_ANIM_SCN *a = (cdtCOLL_ANIM_SCN *)operator new(0x74);
    if (a)
        new (a) cdtCOLL_ANIM_SCN;                       // cdtCOLL_ANIM ctor, vtable v12 0x006075D0
    collAnim = a;
    if (!a)
        return 0;
    voxGRID *g = &coll->grid;
    m3dBOX_DATA *gb = (m3dBOX_DATA *)((u8 *)g + 0x0C);
    box.type = gb->type;
    memcpy(&box.vMin, gb->min, 12);
    memcpy(&box.vMax, gb->max, 12);
    if (box.vMax.y - box.vMin.y < 20.0f)
        box.vMax.y = box.vMax.y + 20.0f;
    a->scene = this;
    if (!collAnim->InitData(g->voxSize.x + g->voxSize.x, g->voxSize.y + g->voxSize.y, g->voxSize.z + g->voxSize.z,
                            g->nx / 2, g->ny / 2, g->nz / 2, (m3dBOX *)&box))
        return 0;
    return 1;
}

// v12 0x004E61E0  (feb 0x004DD8E0)
// Free the level: instances (all = 0: only those not created by the game, flags 0x100), the templates
// loaded with it (0x800), geometry, collision, scripts, selections, lights, camera.
void scnSCENE::DestroyLevel(int all)
{
    if (all)
        for (int k = 0; k < SCN_MAX_INST; k++) {
            animINST *i = InstPtr(k);
            if (i->magic && !(i->flags & 0x100))
                DestroyInst(i);
        }
    for (int k = 0; k < SCN_MAX_TPL; k++) {
        animTPL *t = &tplPool[k];
        if (t->magic && (t->flags & 0x800)) {
            t->DestroyData();
            t->magic = 0;
        }
    }
    if (root) {
        root->~objOBJ();
        operator delete(root);
    }
    root = 0;
    apFree(objList);
    objList = 0;
    if (coll)
        coll->__vecDelDtor(1);
    coll = 0;
    if (collAnim)
        collAnim->__vecDelDtor(1);
    collAnim = 0;
    if (sheet) {
        sheet->Empty();
        operator delete(sheet);
    }
    sheet = 0;
    if (sel) {
        int n = ((int *)sel)[-1];
        for (int k = n - 1; k >= 0; k--) {
            operator delete(sel[k].obj);
            sel[k].obj = 0;
            sel[k].n = 0;
        }
        operator delete((int *)sel - 1);
    }
    sel = 0;
    nSel = 0;
    for (int k = 0; k < nLights; k++) {
        scnLIGHT *l = &lights[k];
        operator delete(l->sel.obj);
        l->sel.obj = 0;
        l->sel.n = 0;
        psSHEET *ps = l->sheet;
        if (ps) {
            ps->Empty();
            operator delete(ps);
        }
    }
    apFree(lights);
    lights = 0;
    nLights = 0;
    if (camera) {
        camera->~camCAMERA();
        operator delete(camera);
    }
    camera = 0;
}

// v12 0x004E63A0  (feb 0x004DDAA0)
animTPL *scnSCENE::FindTplName(char *name)
{
    if (!name || !strlen(name))
        return 0;
    for (int k = 0; k < SCN_MAX_TPL; k++)
        if (tplPool[k].magic == SCN_TPL_MAGIC && !stricmp(tplPool[k].tplName, name))
            return &tplPool[k];
    return 0;
}

// v12 0x004E6410  (feb 0x004DDB10)
animTPL *scnSCENE::AllocTpl()
{
    for (int k = 0; k < SCN_MAX_TPL; k++)
        if (!tplPool[k].magic) {
            animTPL *t = &tplPool[k];
            memset(t, 0, sizeof(animTPL));
            *(scnSCENE **)((u8 *)t + 0xC8) = this;
            t->magic = SCN_TPL_MAGIC;
            return t;
        }
    return 0;
}

// v12 0x004E6470  (feb 0x004DDB70)
void scnSCENE::DestroyTpl(animTPL *t)
{
    if (t) {
        t->DestroyData();
        t->magic = 0;
    }
}

// v12 0x004E6490  (feb 0x004DDB90)
int scnSCENE::_FindInstEmpty()
{
    for (int k = 0; k < SCN_MAX_INST; k++)
        if (!InstPtr(k)->magic) {
            _64 = k + 1 <= _64 ? _64 : k + 1;          // highest used slot + 1
            return k;
        }
    return -1;
}

// v12 0x004E64C0  (feb 0x004DDBC0)
void scnSCENE::_AttachInst(animINST *i)
{
    if (!instFirst) {
        instFirst = i;
        i->listNext = i;
        i->listPrev = i;
        return;
    }
    i->listPrev = instFirst->listPrev;
    i->listNext = instFirst;
    instFirst->listPrev->listNext = i;
    instFirst->listPrev = i;
    if (i->flags & 0x48)
        instFirst = i;
}

// v12 0x004E6510  (feb 0x004DDC10)
animINST *scnSCENE::AllocInst()
{
    int k = _FindInstEmpty();
    if (k == -1) {
        _scnDumpInstList(this);
        return 0;
    }
    animINST *i = scnInitInst(this, k);
    _AttachInst(i);
    return i;
}

// v12 0x004E6620  (feb 0x004DDD20)
// Instance of template t (or of its sub-object objId, made a stand-alone instance) placed by cd.
animINST *scnSCENE::CreateInst(animTPL *t, animCREATE_DATA *cd, int objId)
{
    objOBJ *src;
    if (objId == -1) {
        src = t->root;
    } else {
        objOBJ *o = objFindID(t->root, objId);
        if (o) {
            objOBJ *d = o->Duplicate((objOBJ *)0);
            if (!d)
                return 0;
            float sx, sy, sz;
            cd->matr.GetScale(&sx, &sy, &sz);
            cd->matr.RemoveScale();
            sx = cd->scale.x * sx;
            sy = cd->scale.y * sy;
            sz = cd->scale.z * sz;
            cd->scale.x = sx;
            cd->scale.y = sy;
            cd->scale.z = sz;
            ((m3dMATR *)d->local)->Identity();
            animINST *r = CreateInst(d, cd);
            r->tplObjId = objId;
            return r;
        }
        src = t->root;
    }
    int k = 0;
    while (InstPtr(k)->magic)
        if (++k >= SCN_MAX_INST) {
            _scnDumpInstList(this);
            return 0;
        }
    _64 = k + 1 <= _64 ? _64 : k + 1;
    animINST *i = scnInitInst(this, k);
    _AttachInst(i);
    if (src) {
        i->root = src->Duplicate((objOBJ *)0);
        if (!i->root)
            goto fail;
    }
    i->flags |= t->flags;
    scnUnlinkInst(this, i);                             // re-insert: the flags decide the ring position
    _AttachInst(i);
    if (t->flags & 0x100) {
        i->flags &= ~0x100u;
        scnUnlinkInst(this, i);
        _AttachInst(i);
    }
    i->SetNameClass(t->className);
    i->SetNameTpl(t->tplName);
    i->tpl = t;
    i->ApplyCreateData(cd);
    if (i->props)
        i->props->parent = t->props;
    {
        float s;
        if (i->GetFloat((char *)"ANIM_PROP", (char *)"scale", &s))
            i->Scale(s, s, s, 2);
    }
    i->SetObjInstRef();
    i->nObj = i->root->GetNObj(0);
    i->objTable = (objOBJ **)apCalloc(i->nObj, 4);
    if (!i->objTable)
        return 0;
    objMakePtrListID(i->root, i->objTable, i->nObj);
    i->Invalidate(0x20004);
    if (i->AbvInit()) {
        i->renderOff = 2000000.0f;                      // v12 0x006075CC
        return i;
    }
fail:
    DestroyInst(i);
    return 0;
}

// v12 0x004E6950  (feb 0x004DE050)
// An instance of one object taken out of its tree (a piece of something broken): it keeps the object's place in the
// world, its scale goes into the vertices; under two new objects "<name>.root" and "scaleObj", class "dynamic", the
// scene's empty template. Not for an object with children (the instance taken is then left as it is).
animINST *scnSCENE::CreateInst(objOBJ *o, animCREATE_DATA *cd)
{
    if (!o)
        return 0;
    int k = _FindInstEmpty();
    if (k == -1) {                                      // (the original walks the list reading the names: no effect)
        return 0;
    }
    animINST *i = scnInitInst(this, k);
    _AttachInst(i);
    if (o->inst)
        o->inst->Validate(0x204);
    m3dMATR m = *(m3dMATR *)o->ltm;
    if (o->child)
        return 0;
    if (o->allocFlags && !o->_UnshareAll()) {
        DestroyInst(i);
        return 0;
    }
    if (o->parent)
        o->Detach();
    u8 *g = (u8 *)o->geom;                              // the driver's data for it goes
    *(u32 *)(g + 0x66) = 0;
    *(u32 *)(g + 0x6A) = 0;
    *(u16 *)(g + 0x6E) = 0;
    *(u32 *)(g + 0x70) = 0;
    *(u16 *)(g + 0x74) = 0;
    *(u16 *)(g + 0x76) = 0xFFFF;
    *(u16 *)(g + 0x78) = 0;
    *(u32 *)(g + 0x7A) = 0;
    *(u32 *)(g + 0x66) |= 0x10;
    o->state &= ~0x1000u;
    objOBJ *root = objOBJ::Make();
    if (!root) {
        DestroyInst(i);
        return 0;
    }
    objOBJ *scaleObj = objOBJ::Make();
    if (!scaleObj) {
        delete root;
        DestroyInst(i);
        return 0;
    }
    scaleObj->Attach(root);
    o->Attach(scaleObj);
    i->root = root;
    root->idHier = -1;
    scaleObj->idHier = -1;
    o->unk22 = -1;
    o->unk20 = -1;
    o->RemoveModifiers();
    char name[0x40];
    sprintf(name, "%s.root", o->name);
    root->SetName(name);
    scaleObj->SetName((char *)"scaleObj");
    i->flags = 0;
    i->tpl = dummyTpl;
    i->SetNameTpl(dummyTpl->tplName);
    i->SetNameClass((char *)"dynamic");
    i->SetAnimSeq(-1, 0);
    i->SetObjInstRef();
    i->root->SetIDHier(0);
    float sx, sy, sz;
    m.GetScale(&sx, &sy, &sz);
    m.RemoveScale();
    i->Transform(&m, 0);
    *(m3dMATR *)o->local = m3dMatrIdentity;
    if (!(o->state & 1))
        o->ScaleVertList(sx, sy, sz);
    else
        o->_UnfreezeLTM();
    i->nObj = i->root->GetNObj(0);
    i->objTable = (objOBJ **)apCalloc(i->nObj, 4);
    if (!i->objTable)
        return 0;
    objMakePtrListID(i->root, i->objTable, i->nObj);
    i->ApplyCreateData(cd);
    i->Invalidate(4);
    if (!i->AbvInit()) {
        DestroyInst(i);
        return 0;
    }
    i->renderOff = 2000000.0f;                          // v12 0x006075CC
    return i;
}

// v12 0x004E6CD0  (feb 0x004DE390)
void scnSCENE::DestroyInst(animINST *i)
{
    if (!i)
        return;
    if (collAnim)
        collAnim->Invalidate(i->index, 1);
    i->DestroyData();
    if (instFirst == i && instFirst->listNext == i) {
        i->listPrev = 0;
        i->listNext = 0;
        instFirst = 0;
    } else {
        i->listPrev->listNext = i->listNext;
        i->listNext->listPrev = i->listPrev;
        if (i == instFirst)
            instFirst = i->listNext;
        i->listPrev = 0;
        i->listNext = 0;
    }
    animSYSTEM_INST *as = (animSYSTEM_INST *)i->link;
    if (as) {
        as->~animSYSTEM_INST();
        operator delete(as);
    }
    i->magic = 0;
    while (_64 > 0) {                                   // shrink the used range
        if (InstPtr(_64 - 1)->magic)
            return;
        _64--;
    }
}

// v12 0x004E6D90  (feb 0x004DE450)
animINST *scnSCENE::FindInstName(char *s)
{
    for (animINST *i = FindNextInst(0); i; i = FindNextInst(i))
        if (!stricmp(i->name ? i->name : (char *)"", s))
            return i;
    return 0;
}

// v12 0x004E6DE0  (feb 0x004DE4A0)
void scnSCENE::ForAllInst(int (__fastcall *f)(animINST *, void *), void *data)
{
    for (animINST *i = FindNextInst(0); i; i = FindNextInst(i))
        if (!f(i, data))
            return;
}

// v12 0x004E6E20  (feb 0x004DE4E0)
animINST *scnSCENE::FindNextInst(animINST *i)
{
    if (!i)
        return instFirst;
    return i->listNext != instFirst ? i->listNext : 0;
}

// v12 0x004E6E50  (feb 0x004DE510)
animTPL *scnSCENE::FindNextTpl(animTPL *t)
{
    int k = 0;
    if (t) {
        k = (int)(t - tplPool) + 1;
        if (k > SCN_MAX_TPL - 1)
            return 0;
    }
    for (; k < SCN_MAX_TPL; k++)
        if (tplPool[k].magic)
            return &tplPool[k];
    return 0;
}

// v12 0x004E6EC0  (feb 0x004DE580)
int scnSCENE::GetNInst()
{
    int n = 0;
    for (animINST *i = FindNextInst(0); i; i = FindNextInst(i))
        n++;
    return n;
}

// v12 0x004E6EF0  (feb 0x004DE5B0)
int scnSCENE::GetNTpl()
{
    int n = 0;
    for (animTPL *t = FindNextTpl(0); t; t = FindNextTpl(t))
        n++;
    return n;
}

// v12 0x004E6F60  (feb 0x004DE620)
// Lights named "..._lm", or listed in a selection named "..._lm", are animated lightmaps (flag 0x1000).
void scnSCENE::SetAnimLightFlags()
{
    for (int k = 0; k < nLights; k++)
        if (strstr(lights[k].name, "_lm"))
            lights[k].flags |= 0x1000;
    for (int s = 0; s < nSel; s++) {
        if (!strstr(sel[s].name, "_lm"))
            continue;
        for (int k = 0; k < nLights; k++)
            if (sel[s].Find(lights[k].name) != -1)
                lights[k].flags |= 0x1000;
    }
}

// [not rebuilt yet] original v12 0x004E7030 (feb 0x004DE6F0): the animated lightmap groups
// v12 0x004E7030  (feb 0x004DE6F0)
// The animated-lightmap lights ("_lm", flags 0x1000) in groups: those of one named selection together (a light in two
// is kept in the first, with a message), one alone otherwise. lay (may be 0) gets each group's lightmap: the texture
// of the first face of the first object, among those its lights light, that has one.
int scnSCENE::GetAnimLights(int *n, lgtLIGHT_GROUP **g, txmTEXTURE_LAY ***lay)
{
    int nAnim = 0;
    for (int i = 0; i < nLights; i++)
        if (lights[i].flags & 0x1000)
            nAnim++;
    lgtLIGHT_GROUP *gr = (lgtLIGHT_GROUP *)operator new(nAnim * sizeof(lgtLIGHT_GROUP));
    if (gr)
        for (int i = 0; i < nAnim; i++) {
            gr[i].name[0] = 0;
            gr[i].n = 0;
        }
    *g = gr;
    if (!gr)
        return 0;
    int *map = (int *)operator new(nSel * 4);
    if (!map)
        return 0;
    for (int k = 0; k < nSel; k++)
        map[k] = -1;
    *n = 0;
    for (int i = 0; i < nLights; i++) {
        scnLIGHT *l = &lights[i];
        if (!(l->flags & 0x1000))
            continue;
        int found = 0;
        for (int k = 0; k < nSel; k++) {
            if (sel[k].Find(l->name) == -1)
                continue;
            if (found) {
                apMsg((char *)"Named selection %s has the second occurence of ligth %s, skipped", sel[k].name, l->name);
                continue;
            }
            if (map[k] == -1)
                map[k] = (*n)++;
            lgtLIGHT_GROUP *x = &gr[map[k]];
            x->light[x->n++] = l;
            found = 1;
        }
        if (!found) {
            lgtLIGHT_GROUP *x = &gr[(*n)++];
            x->light[x->n++] = l;
        }
    }
    operator delete(map);
    if (!lay)
        return 1;
    *lay = (txmTEXTURE_LAY **)operator new(*n * 4);
    if (!*lay)
        return 0;
    for (int i = 0; i < *n; i++) {
        (*lay)[i] = 0;
        for (int j = 0; j < gr[i].n && !(*lay)[i]; j++) {
            scnLIGHT *l = gr[i].light[j];
            for (int m = 0; m < l->sel.n; m++) {
                objOBJ *o = objFindName(root, l->sel.obj[m].name);
                if (o && o->nFace > 0 && o->GetFaceTexRendMtl(0, 1)) {
                    (*lay)[i] = (txmTEXTURE_LAY *)o->GetFaceTexRendMtl(0, 1);
                    break;
                }
            }
        }
    }
    return 1;
}

// v12 0x004E7390  (feb 0x004DEA50)
int scnSCENE::ExpandV3DTmpList(objOBJ *o)
{
    int nv = o->GetMaxNVert();
    if (nTmpV3D < nv) {
        tmpV3D = (m3dV *)apRealloc(tmpV3D, nv * sizeof(m3dV));
        if (!tmpV3D) {
            nTmpV3D = 0;
            return 0;
        }
        nTmpV3D = nv;
    }
    return 1;
}

// v12 0x004E73E0  (feb 0x004DEAA0)
// Set instance flags and re-insert it in the ring (0x48 moves it to the head).
void scnSCENE::SetStateYesInst(animINST *i, int bits)
{
    i->flags |= bits;
    scnUnlinkInst(this, i);
    _AttachInst(i);
}

// v12 0x004E7470  (feb 0x004DEB30)
void __fastcall _scnCalcRadRenderOff(animINST *i)
{
    i->renderOff = 2000000.0f;
}

// v12 0x004E7480  (feb 0x004DEB40)  (walks the ring; the dump itself is compiled out)
void __fastcall _scnDumpInstList(scnSCENE *s)
{
    for (animINST *i = s->instFirst; i; ) {
        i = i->listNext;
        if (i == s->instFirst)
            return;
    }
}

// v12 0x004E74C0  (feb 0x004DEB60)
cdtCOLL_ANIM::cdtCOLL_ANIM()
{
    flags = 0;
    grid.nx = m3dVIntZero.x;
    grid.ny = m3dVIntZero.y;
    grid.nz = m3dVIntZero.z;
    m3dBOX_DATA *b = (m3dBOX_DATA *)((u8 *)&grid + 0x0C);
    b->type = 1;
    b->vtbl = (void *)m3dBOX_vftable;
    grid.voxSize = m3dVZero;
    cells = 0;
    cache.n = 0;
    cache.nodes = 0;
    memset(_58, 0, sizeof(_58));
    nPending = 0;
    pending = 0;
    mark = 0;
    ((cdtCOLL_ANIM_SCN *)this)->scene = 0;              // +0x70 (cleared by the base)
}

// v12 0x004E7660  (feb 0x004DED00)
void *cdtCOLL_ANIM_SCN::__vecDelDtor(unsigned f)
{
    Destroy();
    if (f & 1)
        operator delete(this);
    return this;
}

// feb 0x004DDD00 _$E4 / 0x004DEA30: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
