// Animbvol.cpp -- engine.lib bounding volumes ("hit boxes") of animated instances
// (Will Rock 1.2, v12.exe; feb obj engine:Animbvol.obj)
//
// v12 0x004F0E70..0x004F2FDF. Everything is byte-for-byte the beta (apart from addresses) except:
//   - animINST::AbvInit was rewritten for 1.2: every OBB hit box is replaced by the instance's enclosing
//     OBB of its "RUN" sequence ("JUMP" for ai_rat), shrunk around its centre (see AbvInit below);
//   - animTPL::AbvInit moved the default-volume code into a new function (AbvMakeDefaultBVol) that gives
//     ai_* / player* templates an OBB on their "ROOT" object instead of an axis-aligned box;
//   - the OBB updaters gained an SSE path (m3dSimdType) next to the 3DNow!/x87 ones (inline transforms).
//
// Model: animTPL::bvol[] are the volumes of the .tpl (each names an object); every animINST gets a copy
// in animINST::bvol[], bound to its own duplicated objects, with an abvVOL_UPD that remembers where the
// volume sits relative to its object and moves the world-space volume with it (Update, every frame from
// AbvUpdateBVList). Each abvBVOL also keeps an enclosing sphere for quick rejection.

#include "engine.h"

// The beta and 1.2 matrix code branches on m3dSimdType (0 x87, 1 3DNow!, 2.. generic; 1.2 adds SSE): all
// paths compute the same row-vector transform, written once here.
static inline void abvXformPoint(const m3dV *p, const m3dMATR *m, m3dV *o)  { m3dTransformPoint(p, m, o); }
static inline void abvXformVector(const m3dV *p, const m3dMATR *m, m3dV *o) { m3dTransformVector(p, m, o); }

// Object origin of a volume, the way every updater gets it (the instance position when obj == 0).
static inline void abvGetOrigin(animINST *inst, objOBJ *obj, m3dV *o)
{
    if (!obj) {
        inst->GetPos(o);
        return;
    }
    if (inst->invalid & ANIM_INV_POS)
        objValidateLTM(obj);
    obj->GetOrigin(o);
}

// =============================================================================================
// abvVOL_UPD
// =============================================================================================

// v12 0x004F0E70  (feb 0x004CCE80)  [callgraph match, same size and code]
// Updater for an existing volume (the volume of a template, see animINST::AbvInit). The inlined
// constructors also allocate a fresh volume of the same type, which is immediately deleted and replaced
// by the one passed in.
abvVOL_UPD *__fastcall abvVOL_UPD::Make(m3dVOL *v)
{
    abvVOL_UPD *u;
    switch (v->type) {
    case M3D_VOL_BOX:    u = new abvBOX_UPD;    break;
    case M3D_VOL_PRISM:  u = new abvPRISM_UPD;  break;
    case M3D_VOL_SPHERE: u = new abvSPHERE_UPD; break;
    case M3D_VOL_OBB:
        u = new abvOBB_UPD;
        if (u) {
            u->vol = 0;
            ((abvOBB_UPD *)u)->local.type = M3D_VOL_OBB;
            ((abvOBB_UPD *)u)->local.vtbl = m3dOBB_vftable;
        }
        break;
    default:
        return 0;
    }
    if (!u)
        return 0;
    u->vol = abvNewVol(v->type);
    if (u->vol)
        wrDelete(u->vol, 1);
    u->vol = v;
    return u;
}

void abvVOL_UPD::Destroy()
{
    if (vol)
        wrDelete(vol, 1);
}

// v12 0x004F2F80  (feb 0x004CEA30)  scalar deleting destructor of abvVOL_UPD / BOX / PRISM / SPHERE
void *abvVOL_UPD::__vecDelDtor(unsigned flags)
{
    Destroy();
    if (flags & 1)
        operator delete(this);
    return this;
}

// v12 0x004F2F70  (feb 0x004CEA20)
BOOL32 abvVOL_UPD::CheckEnclSph(m3dSPHERE *s)
{
    return vol->CheckEnclSph(s);
}

// v12 0x004F15E0  (feb 0x004CD610)
// Face of the volume that was hit: tail call into the volume with the face index of the hit record.
void abvVOL_UPD::GetFace(cdtINFO *info, m3dPOLY *out, m3dV *normal)
{
    vol->GetFace(info->faceId, out, normal);
}

// =============================================================================================
// abvSPHERE_UPD
// =============================================================================================

// v12 0x004F1600  (feb 0x004CD650)
// Sphere enclosing the object's local box; remember its centre relative to the object origin.
// (inst is unused by all the MakeUpdStruct except the OBB one.)
void abvSPHERE_UPD::MakeUpdStruct(animINST *inst, objOBJ *obj)
{
    m3dSPHERE *s = (m3dSPHERE *)vol;
    ((m3dVOL *)&obj->bbox)->CalcEnclSph(s);
    m3dV o;
    obj->GetOrigin(&o);
    off.x = s->center.x - o.x;
    off.y = s->center.y - o.y;
    off.z = s->center.z - o.z;
}

// v12 0x004F1680  (feb 0x004CD6B0)
void abvSPHERE_UPD::Update(animINST *inst, abvBVOL *bv)
{
    m3dSPHERE *s = (m3dSPHERE *)vol;
    m3dV o;
    abvGetOrigin(inst, bv->obj, &o);
    s->center.x = o.x + off.x;
    s->center.y = o.y + off.y;
    s->center.z = o.z + off.z;
}

// v12 0x004F16F0  (feb 0x004CD720)
// ORIGINAL BUG (both builds): calls its own vtable slot +0x0C, i.e. itself, forever. Presumably meant
// vol->Expand(d). Never reached by the shipped game (no sphere hit boxes are expanded); a mod that
// calls Expand on a sphere volume hangs the game.
void abvSPHERE_UPD::Expand(float d)
{
    Expand(d);
}

// v12 0x004F1700  (feb 0x004CD730)
// "Face" normal of a sphere: direction from the centre to the hit point.
void abvSPHERE_UPD::GetFace(cdtINFO *info, m3dPOLY *out, m3dV *normal)
{
    if (!normal)
        return;
    m3dSPHERE *s = (m3dSPHERE *)vol;
    normal->x = info->normal[0] - s->center.x;   // cdtINFO +0x10 is the hit point here (see recon_fixes)
    normal->y = info->normal[1] - s->center.y;
    normal->z = info->normal[2] - s->center.z;
    m3dNormalize(normal);
}

// =============================================================================================
// abvBOX_UPD -- axis-aligned box, follows the object's origin only (no rotation)
// =============================================================================================

// v12 0x004F1740  (feb 0x004CD770)
// World box = centre of the object's local box +/- its half size; offsets from the object origin.
void abvBOX_UPD::MakeUpdStruct(animINST *inst, objOBJ *obj)
{
    m3dBOX *b  = (m3dBOX *)vol;
    m3dBOX *ob = (m3dBOX *)&obj->bbox;
    m3dV c;
    ob->GetCenter(&c);
    float hx = ob->vMin.x - c.x, hy = ob->vMin.y - c.y, hz = ob->vMin.z - c.z;
    b->vMin.x = c.x + hx;  b->vMin.y = c.y + hy;  b->vMin.z = c.z + hz;
    b->vMax.x = c.x - hx;  b->vMax.y = c.y - hy;  b->vMax.z = c.z - hz;
    m3dV o;
    obj->GetOrigin(&o);
    offMin.x = b->vMin.x - o.x;  offMin.y = b->vMin.y - o.y;  offMin.z = b->vMin.z - o.z;
    offMax.x = b->vMax.x - o.x;  offMax.y = b->vMax.y - o.y;  offMax.z = b->vMax.z - o.z;
}

// v12 0x004F1810  (feb 0x004CD840)
void abvBOX_UPD::Update(animINST *inst, abvBVOL *bv)
{
    m3dBOX *b = (m3dBOX *)vol;
    m3dV o;
    abvGetOrigin(inst, bv->obj, &o);
    b->vMin.x = o.x + offMin.x;  b->vMin.y = o.y + offMin.y;  b->vMin.z = o.z + offMin.z;
    b->vMax.x = o.x + offMax.x;  b->vMax.y = o.y + offMax.y;  b->vMax.z = o.z + offMax.z;
}

// v12 0x004F18A0  (feb 0x004CD8D0)
// Grows the current box on every side, but the stored offsets only vertically: after the next Update
// the box is d taller at each end and its original width again.
void abvBOX_UPD::Expand(float d)
{
    vol->Expand(d);
    offMin.y -= d;
    offMax.y += d;
}

// =============================================================================================
// abvPRISM_UPD -- vertical prism (XZ outline + Y range), follows the object's origin
// =============================================================================================

// v12 0x004F18D0  (feb 0x004CD900)
// Only the first 4 outline vertices are tracked (objCalcBPrism builds 4-sided prisms).
void abvPRISM_UPD::MakeUpdStruct(animINST *inst, objOBJ *obj)
{
    m3dPRISM *p = (m3dPRISM *)vol;
    objCalcBPrism(obj, p);
    m3dV o;
    obj->GetOrigin(&o);
    offYMin = p->yMin - o.y;
    offYMax = p->yMax - o.y;
    m3dV v[M3D_POLY_MAX_VERT * 2];
    p->GetVertices(v);
    for (int i = 0; i < 4; i++) {
        off[i].x = v[i].x - o.x;
        off[i].y = v[i].z - o.z;
    }
}

// v12 0x004F1990  (feb 0x004CD9C0)
void abvPRISM_UPD::Update(animINST *inst, abvBVOL *bv)
{
    m3dPRISM *p = (m3dPRISM *)vol;
    m3dV o;
    abvGetOrigin(inst, bv->obj, &o);
    p->yMin = o.y + offYMin;
    p->yMax = o.y + offYMax;
    for (int i = 0; i < 4; i++) {
        p->base.vert[i].x = o.x + off[i].x;
        p->base.vert[i].y = o.z + off[i].y;
    }
}

// v12 0x0051AAF0  (feb 0x004CDA70)  identical-code-folded with 18 other empty one-argument methods (a swap
// is safe only because this body is the same empty "ret 4")
void abvPRISM_UPD::Expand(float d)
{
}

// =============================================================================================
// abvOBB_UPD -- oriented box, follows the object's full matrix
// =============================================================================================

// v12 0x004F1A40  (feb 0x004CDA80)
// Bring the world-space OBB into the object's (or, with no object, the instance's) local frame.
void abvOBB_UPD::MakeUpdStruct(animINST *inst, objOBJ *obj)
{
    m3dOBB *w = (m3dOBB *)vol;
    m3dMATR w2l;
    if (!obj) {
        inst->Validate(0x40);
        memcpy(&w2l, inst->matrW2L, sizeof(w2l));
    } else {
        objValidateLTM(obj);
        ((m3dMATR *)obj->ltm)->Invert(&w2l);
    }
    abvXformPoint (&w->org,   &w2l, &local.org);
    abvXformVector(&w->ax[0], &w2l, &local.ax[0]);
    abvXformVector(&w->ax[1], &w2l, &local.ax[1]);
    abvXformVector(&w->ax[2], &w2l, &local.ax[2]);
}

// v12 0x004F1FF0  (feb 0x004CE030)
void abvOBB_UPD::Update(animINST *inst, abvBVOL *bv)
{
    m3dOBB *w = (m3dOBB *)vol;
    m3dMATR l2w;
    if (!bv->obj) {
        inst->GetMatrL2W(&l2w);
    } else {
        objValidateLTM(bv->obj);
        memcpy(&l2w, bv->obj->ltm, sizeof(l2w));
    }
    abvXformPoint (&local.org,   &l2w, &w->org);
    abvXformVector(&local.ax[0], &l2w, &w->ax[0]);
    abvXformVector(&local.ax[1], &l2w, &w->ax[1]);
    abvXformVector(&local.ax[2], &l2w, &w->ax[2]);
}

// v12 0x004F25D0  (feb 0x004CE5D0)
void abvOBB_UPD::Expand(float d)
{
    vol->Expand(d);
    ((m3dVOL *)&local)->Expand(d);
}

void abvOBB_UPD::Destroy()
{
    local.vtbl = m3dVOL_vftable;   // ~m3dOBB of the embedded member (back to the m3dVOL vtable)
    abvVOL_UPD::Destroy();
}

// v12 0x004F2FB0  (feb 0x004CEA60)
void *abvOBB_UPD::__vecDelDtor(unsigned flags)
{
    Destroy();
    if (flags & 1)
        operator delete(this);
    return this;
}

// =============================================================================================
// abvBVOL
// =============================================================================================

// v12 0x004F0FE0  (feb 0x004CCFF0)
abvBVOL::abvBVOL()
{
    obj = 0;
    upd = 0;
    sphere.type   = M3D_VOL_SPHERE;
    sphere.vtbl   = m3dSPHERE_vftable;
    sphere.radius = -1.0f;
}

// (feb 0x004CD020)  ~abvBVOL; in 1.2 only the vector deleting destructor (v12 0x004037D0) remains.
void abvBVOL::Destroy()
{
    if (upd)
        wrDelete(upd, 1);
    sphere.vtbl = m3dVOL_vftable;
}

// v12 0x004037D0  vector deleting destructor (compiler-generated, emitted in an earlier obj)
void *abvBVOL::__vecDelDtor(unsigned flags)
{
    return wrVecDelDtor(this, flags);
}

// v12 0x004F1010  (feb 0x004CD040)
abvBVOL &abvBVOL::operator=(abvBVOL &src)
{
    Alloc(src.upd->vol->type);
    obj           = src.obj;
    sphere.type   = src.sphere.type;
    sphere.center = src.sphere.center;
    sphere.radius = src.sphere.radius;
    DuplicateVol(&src);
    return *this;
}

// v12 0x004F1060  (feb 0x004CD090)
// Fresh updater + empty volume of the given type. Returns 0 only for an unknown type; an allocation
// failure leaves upd (or upd->vol) 0 and still returns 1, as in the original.
BOOL32 abvBVOL::Alloc(int volType)
{
    sphere.radius = -1.0f;
    abvVOL_UPD *u;
    switch (volType) {
    case M3D_VOL_BOX:    u = new abvBOX_UPD;    break;
    case M3D_VOL_PRISM:  u = new abvPRISM_UPD;  break;
    case M3D_VOL_SPHERE: u = new abvSPHERE_UPD; break;
    case M3D_VOL_OBB:
        u = new abvOBB_UPD;
        if (u) {
            u->vol = 0;
            ((abvOBB_UPD *)u)->local.type = M3D_VOL_OBB;
            ((abvOBB_UPD *)u)->local.vtbl = m3dOBB_vftable;
        }
        break;
    default:
        return 0;
    }
    if (u)
        u->vol = abvNewVol(volType);
    upd = u;
    return 1;
}

// v12 0x004F1200  (feb 0x004CD230)
// Copy updater data and volume data from src (same type assumed: Alloc was called with src's type).
// The compiler-generated assignments copy everything but the vtable pointers; our own vol pointer is kept.
void abvBVOL::DuplicateVol(abvBVOL *src)
{
    m3dVOL *v = upd->vol;
    m3dVOL *s = src->upd->vol;
    size_t usz, vsz;
    switch (v->type) {
    case M3D_VOL_BOX:    usz = sizeof(abvBOX_UPD);    vsz = 0x20; break;
    case M3D_VOL_PRISM:  usz = sizeof(abvPRISM_UPD);  vsz = 0xB4; break;
    case M3D_VOL_SPHERE: usz = sizeof(abvSPHERE_UPD); vsz = 0x18; break;
    case M3D_VOL_OBB:    usz = sizeof(abvOBB_UPD);    vsz = 0x38; break;
    default: return;
    }
    memcpy((char *)upd + 4, (char *)src->upd + 4, usz - 4);       // updater fields (+4 = vol, restored below)
    if (v->type == M3D_VOL_OBB)
        ((abvOBB_UPD *)upd)->local.vtbl = m3dOBB_vftable;        // the embedded OBB keeps its own vptr
    upd->vol = v;
    memcpy((char *)v + 4, (char *)s + 4, vsz - 4);                // volume fields
}

// v12 0x004F1470  (feb 0x004CD4A0)
// (Re)capture the volume from the current pose, then its enclosing sphere.
void abvBVOL::Calc(animINST *inst)
{
    upd->MakeUpdStruct(inst, obj);
    upd->vol->CalcEnclSph((m3dSPHERE *)&sphere);
}

// v12 0x004F14A0  (feb 0x004CD4D0)
void abvBVOL::GetOrg(animINST *inst, m3dV *out)
{
    abvGetOrigin(inst, obj, out);
}

// v12 0x004F14E0  (feb 0x004CD510)
BOOL32 abvBVOL::IsIsectCyl(m3dV *base, m3dV *top, float r, float h)
{
    return upd->vol->IsIsectCyl(base, top, r, h);
}

// v12 0x004F1500  (feb 0x004CD530)
// Ray test against the world volume; hit point -> info+0x10, distance -> +0x1C, face -> +0x0C.
BOOL32 abvBVOL::IsectRay(m3dV *org, m3dV *dir, float maxT, cdtINFO *info)
{
    cdtINFO tmp;
    tmp.flags = 0; tmp.inst = 0; tmp.objId = 0; tmp.faceId = 0;
    tmp.normal[0] = m3dVZero.x; tmp.normal[1] = m3dVZero.y; tmp.normal[2] = m3dVZero.z;
    tmp.dist = 0;
    if (!info)
        info = &tmp;
    return upd->vol->IsectRay(org, dir, maxT, (m3dV *)info->normal, &info->dist, &info->faceId);
}

// v12 0x004F1570  (feb 0x004CD5A0)
BOOL32 abvBVOL::DistPoint(m3dV *p, float maxD, cdtINFO *info)
{
    cdtINFO tmp;
    tmp.flags = 0; tmp.inst = 0; tmp.objId = 0; tmp.faceId = 0;
    tmp.normal[0] = m3dVZero.x; tmp.normal[1] = m3dVZero.y; tmp.normal[2] = m3dVZero.z;
    tmp.dist = 0;
    if (!info)
        info = &tmp;
    return upd->vol->DistPoint(p, maxD, (m3dV *)info->normal, &info->dist, &info->faceId);
}

// =============================================================================================
// animTPL
// =============================================================================================

// v12 0x004F2680  NEW in 1.2 (the beta did this inline in animTPL::AbvInit with a box on the root object)
// Default hit box of a template that has none in its file: an OBB around the "ROOT" object for enemies
// and the player, an axis-aligned box following the instance position for everything else.
void animTPL::AbvMakeDefaultBVol()
{
    nBVol = 1;
    bvol = wrArrayNew<abvBVOL>(1);
    bvol->obj = 0;
    if (strncmp(className, "ai_", 3) == 0 || strncmp(className, "player", 6) == 0) {
        bvol->Alloc(M3D_VOL_OBB);
        bvol->obj = objFindName(root, "ROOT");
    } else {
        bvol->Alloc(M3D_VOL_BOX);
    }
}

// v12 0x004F25F0  (feb 0x004CE5F0)
// Called by the level loader once per template. seqEncl: per-sequence enclosing volumes read from the
// level file, if any; otherwise they are computed.
BOOL32 animTPL::AbvInit(abvBVOL *seqEncl)
{
    if (nBVol == 0 && !(flags & 0x40))
        AbvMakeDefaultBVol();
    if (seqEncl) {
        bvSeq = seqEncl;
        return 1;
    }
    AbvCalcBVolEnclos();
    return 1;
}

// v12 0x004F2640  (feb 0x004CE6A0)
void animTPL::AbvTerm()
{
    if (bvol)
        wrDelete(bvol, 3);
    bvol = 0;
    if (nSeq > 0 && bvSeq)
        wrDelete(bvSeq, 3);
}

// =============================================================================================
// animINST
// =============================================================================================

// v12 0x004F2740  (feb 0x004CE6E0)  [callgraph match: REWRITTEN in 1.2, 0x600 bytes vs 0x250]
// Give the instance its own copy of the template's volumes, bound to the instance's objects.
//
// Beta: each volume was captured from its object's box in the template's default pose.
// 1.2:  the per-sequence enclosing volumes are computed FIRST (AbvCalcBVolEnclos moved up), and every
//       OBB hit box is overwritten with the enclosing OBB of the instance in its run pose (first sequence
//       whose name contains "RUN", "JUMP" for ai_rat; the instance's current enclosing OBB if there is no
//       per-sequence data), then shrunk around its centre: X and Y by 30 %, Z by 15 % for ai_cerber and
//       ai_croc, 40 % for every other template except ai_sphinx (left at full size).
//       This is the 1.2 "fairer hit boxes" change for enemies; the player is an OBB too (AbvMakeDefaultBVol).
BOOL32 animINST::AbvInit()
{
    if (root) {
        if (tpl) {
            int   savedSeq  = seq;
            float savedTime = (seq == -1) ? 0.0f : tpl->seq[seq].FrameToTime(frame);
            if (tpl && tpl->nSeq > 0)
                SetAnimSeq(tpl->defSeq, 0.0f);
            AbvCalcBVolEnclos(4, 2, -1.0f);
            Validate(6);

            nBVol = tpl->nBVol;
            bvol  = nBVol ? wrArrayNew<abvBVOL>(nBVol) : 0;
            Validate(0x104);

            const char *poseSeq = strstr(className, "ai_rat") ? "JUMP" : "RUN";
            abvBVOL *bv = 0;
            for (int i = 0; i < nBVol; i++) {
                abvBVOL *src = &tpl->bvol[i];
                bv = &bvol[i];
                bv->Alloc(src->upd->vol->type);

                objOBJ *o = 0;
                if (src->obj)
                    o = objFindName(root, src->obj->name);
                char name[0x40];
                if (GetStr("ANIM_PROP", "cdtRootObj", name, sizeof(name)))
                    o = objFindName(root, name);
                bv->obj = o;
                root->unk1E = (short)i;

                m3dOBB *ob = (m3dOBB *)bv->upd->vol;
                if (ob->type == M3D_VOL_OBB) {
                    m3dOBB *from = 0;
                    if (bvSeq && bvSeq->upd->vol->type == M3D_VOL_OBB) {
                        from = (m3dOBB *)bvSeq->upd->vol;
                        for (int s = 0; s < tpl->nSeq; s++) {
                            if (strstr((char *)&tpl->seq[s], poseSeq)) {     // animSEQ +0x00: name
                                abvBVOL *sb = &bvSeq[s];
                                sb->upd->Update(this, sb);
                                sb->upd->vol->CalcEnclSph((m3dSPHERE *)&sb->sphere);
                                from = (m3dOBB *)bvSeq[s].upd->vol;
                                break;
                            }
                        }
                    } else {
                        from = (m3dOBB *)bvCur->upd->vol;
                    }
                    ob->type  = from->type;
                    ob->org   = from->org;
                    ob->ax[0] = from->ax[0];
                    ob->ax[1] = from->ax[1];
                    ob->ax[2] = from->ax[2];

                    if (!strstr(tplName, "ai_sphinx")) {
                        float kz = (strstr(tplName, "ai_cerber") || strstr(tplName, "ai_croc")) ? 0.15f : 0.4f;
                        ob->org.x += ob->ax[0].x * 0.15f;  ob->org.y += ob->ax[0].y * 0.15f;  ob->org.z += ob->ax[0].z * 0.15f;
                        ob->org.x += ob->ax[1].x * 0.15f;  ob->org.y += ob->ax[1].y * 0.15f;  ob->org.z += ob->ax[1].z * 0.15f;
                        float hz = 0.5f * kz;
                        ob->org.x += hz * ob->ax[2].x;     ob->org.y += hz * ob->ax[2].y;     ob->org.z += hz * ob->ax[2].z;
                        ob->ax[0].x -= ob->ax[0].x * 0.3f; ob->ax[0].y -= ob->ax[0].y * 0.3f; ob->ax[0].z -= ob->ax[0].z * 0.3f;
                        ob->ax[1].x -= ob->ax[1].x * 0.3f; ob->ax[1].y -= ob->ax[1].y * 0.3f; ob->ax[1].z -= ob->ax[1].z * 0.3f;
                        ob->ax[2].x += ob->ax[2].x * -kz;  ob->ax[2].y += ob->ax[2].y * -kz;  ob->ax[2].z += ob->ax[2].z * -kz;
                    }
                } else if (!bv->obj) {
                    bv->obj = root;
                }
                bv->upd->MakeUpdStruct(this, bv->obj);
                bv->upd->vol->CalcEnclSph((m3dSPHERE *)&bv->sphere);
            }
            // a single volume on the root object: track the instance itself (cheaper)
            if (nBVol == 1 && bv->obj == root) {
                bv->obj = 0;
                flags |= 0x80;
            }
            if (tpl && tpl->nSeq > 0)
                SetAnimSeq(savedSeq, savedTime);
            return 1;
        }
        flags |= ANIM_ST_IGNORE_CDT;
    }
    nBVol = 0;
    bvol  = 0;
    bvSeq = 0;
    bvCur = 0;
    return 1;
}

// v12 0x004F2D40  (feb 0x004CE930)
void animINST::AbvTermBVolEnclos()
{
    if (unk08 & 1) {
        if (bvCur)
            wrDelete(bvCur, 1);
        unk08 &= ~1u;
    }
    bvCur = 0;
}

// v12 0x004F2D70  (feb 0x004CE960)
void animINST::AbvTerm()
{
    if (bvSeq)
        wrDelete(bvSeq, 3);
    bvSeq = 0;
    if (bvol)
        wrDelete(bvol, 3);
    bvol = 0;
    AbvTermBVolEnclos();
}

// v12 0x004F2DD0  (feb 0x004CE9C0)
// Move every hit box with its object (once per frame for active instances).
void animINST::AbvUpdateBVList()
{
    for (int i = 0; i < nBVol; i++) {
        abvBVOL *bv = &bvol[i];
        bv->upd->Update(this, bv);
        bv->upd->vol->CalcEnclSph((m3dSPHERE *)&bv->sphere);
    }
}

// v12 0x004F2E20  (no feb twin: new in 1.2)
// A corpse keeps its pose: the first hit box, an oriented box, is fitted again to the posed tree -- the root's box
// worked out with the instance at the origin, its offsets taken from there -- then the instance goes back.
void animINST::SnapshotPose()
{
    if (!root)
        return;
    abvVOL_UPD *upd = bvol->upd;
    if (upd->vol->type != M3D_VOL_OBB)
        return;
    m3dMATR saved = *(m3dMATR *)matr;
    Transform(&m3dMatrIdentity, 0);
    Validate(2);
    m3dOBB_D *b = (m3dOBB_D *)upd->vol;
    m3dBOX_DATA *rb = &root->bbox;
    b->org.x = rb->min[0];
    b->org.y = rb->min[1];
    b->org.z = rb->min[2];
    b->ax[0] = m3dVZero;
    b->ax[1] = m3dVZero;
    b->ax[2] = m3dVZero;
    b->ax[0].x = rb->max[0] - rb->min[0];
    b->ax[1].y = rb->max[1] - rb->min[1];
    b->ax[2].z = rb->max[2] - rb->min[2];
    upd->MakeUpdStruct(this, bvol->obj);
    upd->vol->CalcEnclSph((m3dSPHERE *)&bvol->sphere);
    Transform(&saved, 0);
}
