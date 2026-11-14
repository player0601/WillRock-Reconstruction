// scn_coll.cpp -- engine.lib scene collision queries: sphere push-out (movement), closest point,
// collision info helpers (Will Rock 1.2, v12.exe; feb obj engine:scn_coll.obj)
//
// v12 0x004DA6A0..0x004DBEBF. Static geometry goes through scnSCENE::coll (cdtCOLL_SCN, cdto_math), animated
// instances through collAnim (cdtCOLL_ANIM) and then either their enclosing volume / object faces or their
// hit boxes (abvBVOL). 1.2 changes against the beta (all from the 1.2 disassembly):
//   - the caller's query flags are stored in the filter (cdtREFINE::queryFlags) and the filter's new slot
//     IsUseBVolEncl decides between the enclosing volume and the per-bone hit boxes (MakeAnimList,
//     _GetClosestPointAnim);
//   - _ConstrSphere_1 has a new query flag 0x400: a push-out that turns the movement by more than 90
//     degrees is refused (the sphere stays where it was);
//   - _DistPointInst works on a temporary low LOD (LODSetTemp(5)) and validates more state (0x201);
//   - the "lustra" (chandelier) exclusion of _GetClosestPointAnim is gone: strstr is still called, its
//     result unused.

#include "engine.h"

#define SCN_EPS 1e-6f                  // v12 0x00607050

static inline void cdtInfoClear(cdtINFO *c)
{
    c->flags = 0; c->inst = 0; c->objId = 0; c->faceId = 0;
    c->normal[0] = m3dVZero.x; c->normal[1] = m3dVZero.y; c->normal[2] = m3dVZero.z;
    c->dist = 0;
}
static inline m3dV *ciPoint(cdtINFO *c) { return (m3dV *)c->normal; }   // +0x10 is the contact point

// v12 0x004DA6A0  (feb 0x004CB6D0)
// Instances (and hit boxes) near the segment a-b, as a cylinder of radius r (r ~ 0: a ray).
int scnSCENE::MakeAnimList(m3dV *a, m3dV *b, float r, cdtREFINE *ref, scnAOBJ *out, int max)
{
    struct { const void *vtbl; int type; m3dV vMin, vMax; } box = { m3dBOX_vftable, M3D_VOL_BOX };
    BOOL32 isRay = fabsf(r) < SCN_EPS;
    if (isRay) {
        collAnim->MakeIndListRay(a, b, cdtOflPtr, 0);
    } else {
        ((m3dBOX *)&box)->MakePointPoint(a, b);
        ((m3dBOX *)&box)->Expand(r);
        collAnim->MakeIndList((m3dBOX *)&box, cdtOflPtr, 0);
    }
    m3dV dir = { b->x - a->x, b->y - a->y, b->z - a->z };
    float len = m3dSqrt(dir.z * dir.z + dir.y * dir.y + dir.x * dir.x);
    if (!_m3dNormalize(&dir))
        dir = m3dVUnitY;
    int n = 0;
    for (int i = 0; i < cdtOflPtr->nObj; i++) {
        animINST *inst = InstPtr(cdtOflPtr->obj[i]);
        if (IsEmptyInst(inst))
            continue;
        if ((ref->mode & 2) && !(inst->flags & 0x48))
            continue;
        if (inst->flags & 0x10)
            continue;
        if (!(flags & 1) && (inst->flags & 2))
            continue;
        if (!ref->IsInclInst(inst))
            continue;
        if (ref->IsUseBVolEncl(inst)) {
            inst->Validate(0x100);
            if (!inst->bvCur || !inst->bvCur->IsIsectCyl(a, &dir, len, r))
                continue;
            if (n >= max)
                return n;
            out[n].inst = inst;
            out[n].bvol = -1;
            n++;
        } else {
            if (!inst->nBVol)
                continue;
            inst->Validate(0x20);
            for (int k = 0; k < inst->nBVol; k++) {
                if (!(fabsf(r) < SCN_EPS) && !inst->bvol[k].IsIsectCyl(a, &dir, len, r))
                    continue;
                if (n >= max)
                    return n;
                out[n].inst = inst;
                out[n].bvol = k;
                n++;
            }
        }
    }
    return n;
}

// The world box around a volume (box: copied, prism / OBB: their 8 corners; a sphere leaves it as it was).
static void scnVolBox(m3dVOL *v, m3dBOX *out)
{
    m3dV pts[8];
    switch (v->type) {
    case M3D_VOL_BOX:
        out->type = v->type;
        out->vMin = ((m3dBOX *)v)->vMin;
        out->vMax = ((m3dBOX *)v)->vMax;
        break;
    case M3D_VOL_PRISM:
        ((m3dPRISM *)v)->GetVertices(pts);
        out->CalcStart();
        out->CalcUpdate(pts, 8);
        break;
    case M3D_VOL_OBB:
        ((m3dOBB *)v)->GetVertices(pts);
        out->CalcStart();
        out->CalcUpdate(pts, 8);
        break;
    }
}

// v12 0x004DA3B0  (feb 0x004CB3F0)
// Instances (and hit boxes) whose volume's world box touches box -- the same filters as the cylinder query.
int scnSCENE::MakeAnimList(m3dBOX *b, cdtREFINE *ref, scnAOBJ *out, int max)
{
    struct { const void *vtbl; int type; m3dV vMin, vMax; } box = { m3dBOX_vftable, M3D_VOL_BOX };
    collAnim->MakeIndList(b, cdtOflPtr, 0);
    int n = 0;
    for (int i = 0; i < cdtOflPtr->nObj; i++) {
        animINST *inst = InstPtr(cdtOflPtr->obj[i]);
        if (IsEmptyInst(inst))
            continue;
        if ((ref->mode & 2) && !(inst->flags & 0x48))
            continue;
        if (inst->flags & 0x10)
            continue;
        if (!(flags & 1) && (inst->flags & 2))
            continue;
        if (!ref->IsInclInst(inst))
            continue;
        if (ref->IsUseBVolEncl(inst)) {
            inst->Validate(0x100);
            if (!inst->bvCur)
                continue;
            scnVolBox(inst->bvCur->upd->vol, (m3dBOX *)&box);
            if (((m3dBOX *)&box)->IsIsectBox(b) == 1)
                continue;
            if (n >= max)
                return n;
            out[n].inst = inst;
            out[n].bvol = -1;
            n++;
        } else {
            if (!inst->nBVol)
                continue;
            inst->Validate(0x20);
            for (int k = 0; k < inst->nBVol; k++) {
                scnVolBox(inst->bvol[k].upd->vol, (m3dBOX *)&box);
                if (((m3dBOX *)&box)->IsIsectBox(b) == 1)
                    continue;
                if (n >= max)
                    return n;
                out[n].inst = inst;
                out[n].bvol = k;
                n++;
            }
        }
    }
    return n;
}

// v12 0x004DAA10  (feb 0x004CBA20)
// One step of the sphere movement from 'from' to *pos: push *pos out of the closest geometry (up to
// three times); when it still touches, give the step up (*pos = from) unless query flag 0x100 allows
// one more push along the sum of the two pushes. Query flag 0x10: also for zero-length steps.
int scnSCENE::_ConstrSphere_1(m3dV *from, m3dV *pos, float r, int qflags, cdtREFINE *ref, cdtINFO *ci)
{
    cdtINFO c;
    memset(&c, 0, sizeof(c));
    m3dV move = { pos->x - from->x, pos->y - from->y, pos->z - from->z };
    float d = m3dSqrt(move.z * move.z + move.y * move.y + move.x * move.x);
    if (!(qflags & 0x10) && !(SCN_EPS * 10.0f < d))
        return 0;
    if (!GetClosestPoint(pos, SCN_EPS + r, qflags, ref, &c))
        return 0;
    m3dV n = { pos->x - ciPoint(&c)->x, pos->y - ciPoint(&c)->y, pos->z - ciPoint(&c)->z };
    if (!_m3dNormalize(&n))
        GetCDTFace(&c, 0, &n);
    float k = r - c.dist;
    n.x *= k;  n.y *= k;  n.z *= k;
    pos->x += n.x;  pos->y += n.y;  pos->z += n.z;
    if (GetClosestPoint(pos, r, qflags, ref, &c)) {
        m3dV v = { pos->x - ciPoint(&c)->x, pos->y - ciPoint(&c)->y, pos->z - ciPoint(&c)->z };
        if (m3dSqrt(v.z * v.z + v.y * v.y + v.x * v.x) > 0.001f) {
            m3dNormalize(&v);
            float k2 = r - c.dist;
            v.x *= k2;  v.y *= k2;  v.z *= k2;
            if (m3dSqrt(v.z * v.z + v.y * v.y + v.x * v.x) > 0.001f) {
                pos->x += v.x;  pos->y += v.y;  pos->z += v.z;
                if (GetClosestPoint(pos, r - SCN_EPS, qflags, ref, &c)) {
                    if (!(qflags & 0x100)) {
                        *pos = *from;
                    } else {
                        m3dV w = { v.x + n.x, v.y + n.y, v.z + n.z };
                        if (m3dSqrt(w.z * w.z + w.y * w.y + w.x * w.x) > 0.001f) {
                            m3dNormalize(&w);
                            pos->x += w.x;  pos->y += w.y;  pos->z += w.z;
                            if (GetClosestPoint(pos, r - SCN_EPS, qflags, ref, &c))
                                *pos = *from;
                        }
                    }
                }
            }
        }
    }
    if (qflags & 0x400) {                               // 1.2: refuse a push that reverses the movement
        m3dV now = { pos->x - from->x, pos->y - from->y, pos->z - from->z };
        if (m3dAngleVector(&now, &move) > 90.0f)
            *pos = *from;
    }
    if (ci)
        *ci = c;
    return 1;
}

// v12 0x004DB000  (feb 0x004CBFA0)
int scnSCENE::ConstrSphere(m3dV *from, m3dV *to, float r, int qflags, cdtREFINE *ref, cdtINFO *ci)
{
    if (ci)
        memset(ci, 0, sizeof(*ci));
    return _ConstrSphere(from, to, r, qflags, ref, ci);
}

// v12 0x004DB040  (feb 0x004CBFE0)
// Move a sphere of radius r from 'from' towards *to in steps of at most ~0.7 r; on the first contact
// the remaining movement (clamped to 0.9 r) is resolved from there. Returns 1 when something was touched.
int scnSCENE::_ConstrSphere(m3dV *from, m3dV *to, float r, int qflags, cdtREFINE *ref, cdtINFO *ci)
{
    cdtINFO c;
    memset(&c, 0, sizeof(c));
    float dx = to->x - from->x, dy = to->y - from->y, dz = to->z - from->z;
    float d = m3dSqrt(dz * dz + dy * dy + dx * dx);
    float x = -((r * 0.3f + d) / r);
    int nSteps = -(int)lrintf(x - 0.4999990f);         // m3dIntFloor (x87 round to nearest of x - 0.5): ceil
    if ((qflags & 0x10) && nSteps == 0)
        nSteps = 1;
    float inv = 1.0f / (float)nSteps;
    m3dV step = { dx * inv, dy * inv, dz * inv };
    m3dV cur = *from;
    for (int i = 0; i < nSteps; i++) {
        m3dV nxt = { cur.x + step.x, cur.y + step.y, cur.z + step.z };
        if (_ConstrSphere_1(&cur, &nxt, r, qflags, ref, &c)) {
            if (nSteps > 1) {
                m3dClampVector_L(&nxt, to, r * 0.9f);
                if (!_ConstrSphere_1(&nxt, to, r, qflags, ref, ci))
                    return 0;
            } else {
                *to = nxt;
            }
            if (ci)
                *ci = c;
            return 1;
        }
        cur = nxt;
    }
    return 0;
}

// v12 0x004DB2B0  (feb 0x004CC250)
// Closest geometry point within r: the level (unless filter mode 1), then the instances (unless mode 2|8).
int scnSCENE::GetClosestPoint(m3dV *p, float r, int qflags, cdtREFINE *ref, cdtINFO *ci)
{
    cdtINFO tmp;
    cdtInfoClear(&tmp);
    if (!ci)
        ci = &tmp;
    int res = 0;
    if (!(ref->mode & 1)) {
        res = coll->GetClosestPoint(p, r, qflags, ci);
        if (res)
            r = ci->dist;
    }
    if ((ref->mode & 2) && (ref->mode & 8))
        return res;
    return _GetClosestPointAnim(p, r, qflags, ref, ci) | res;
}

// v12 0x004DB370  (feb 0x004CC310)
int scnSCENE::_GetClosestPointAnim(m3dV *p, float r, int qflags, cdtREFINE *ref, cdtINFO *ci)
{
    cdtINFO hit;
    cdtInfoClear(&hit);
    scnAOBJ list[150];
    ref->queryFlags = qflags;
    int n = MakeAnimList(p, p, r, ref, list, 150);
    float best = r;
    int found = 0;
    for (int i = 0; i < n; i++) {
        animINST *inst = list[i].inst;
        strstr(inst->tplName ? inst->tplName : (char *)"", "lustra");   // 1.2: result no longer used
        if (ref->IsUseBVolEncl(inst)) {
            if (_DistPointInst(inst, p, best, ref, ci)) {
                best = ci->dist;
                found = 1;
            }
            continue;
        }
        abvBVOL *bv = &inst->bvol[list[i].bvol];
        int rr = bv->DistPoint(p, best, ci);
        if (!rr)
            continue;
        if (rr == 2) {                                  // inside the hit box: distance to its surface, negative
            m3dV c = bv->sphere.center;
            m3dV dir = { p->x - c.x, p->y - c.y, p->z - c.z };
            if (!_m3dNormalize(&dir))
                dir = m3dVUnitX;
            float L = bv->sphere.radius + 1.0f;
            m3dV start = { dir.x * L + c.x, dir.y * L + c.y, dir.z * L + c.z };
            dir.x = -dir.x;  dir.y = -dir.y;  dir.z = -dir.z;
            if (bv->IsectRay(&start, &dir, L, &hit) != 1)
                continue;
            m3dV hp = *ciPoint(&hit);
            float dd = m3dSqrt(dist2(hp, *p));
            if (-dd < best)
                continue;
            ci->faceId = hit.faceId;
            *ciPoint(ci) = hp;
            ci->dist = -m3dSqrt(dist2(hp, *p));
        }
        ci->flags &= ~0x100u;
        ci->inst  = inst;
        ci->objId = list[i].bvol;
        found = 1;
        best = ci->dist;
    }
    if (found)
        ci->flags |= 2;
    return found;
}

// v12 0x004DB790  (feb 0x004CC750)
// Closest point on the faces of one instance (its own collision data when it has a current frame of it,
// else each object's faces through a temporary cdtCOLL_OBJ).
int scnSCENE::_DistPointInst(animINST *inst, m3dV *p, float r, cdtREFINE *ref, cdtINFO *ci)
{
    cdtCOLL_OBJ obj;
    inst->LODSetTemp(5, 0);
    inst->Validate(0x201);
    cdtINFO save = *ci;
    float best = r;
    int found = 0, res = 0, hitObj = 0, hitFace = 0;
    m3dV hitPt = { 0, 0, 0 };
    cdtCOLL *own = *(cdtCOLL **)((u8 *)inst + 0x144);
    BOOL32 ownFrame = own && ((cdtCOLL_INST *)own)->IsCDTFrame();
    if (ownFrame) {
        res = own->GetClosestPoint(p, r, 0, &save);
        if (res) {
            save.inst = inst;
            save.flags |= 0x100;
        }
    } else {
        for (int i = 0; i < inst->nObj; i++) {
            objOBJ *o = inst->objTable[i];
            if (!o || !o->nFace || !ref->IsInclObj(o))
                continue;
            m3dSPHERE *s = animGetObjBSphere(inst->objTable[i]);
            objGEOM *g = inst->objTable[i]->geom;
            g->bsphere.type = ((int *)s)[1];
            memcpy(g->bsphere.center, &((m3dSPHERE *)s)->center, 12);
            g->bsphere.radius = s->radius;
            obj.data = inst->objTable[i];
            if (obj.GetClosestPoint(p, best, 0, ci)) {
                hitFace = ci->objId;
                best = ci->dist;
                hitObj = i;
                found = 1;
                hitPt = *ciPoint(ci);
            }
        }
        if (found) {
            ci->objId  = hitObj;
            ci->faceId = hitFace;
            ci->dist   = best;
            ci->inst   = inst;
            *ciPoint(ci) = hitPt;
            ci->flags |= 0x102;
        }
    }
    if (ownFrame) {
        *ci = save;
        found = res;
    }
    inst->LODRestore();
    obj.Destroy();
    return found;
}

// v12 0x004DBA00  (feb 0x004CC9B0)
int scnSCENE::DistPointObj(objOBJ *o, m3dV *p, float r, int flags, cdtINFO *ci)
{
    cdtCOLL_OBJ c;
    m3dSPHERE *s = animGetObjBSphere(o);
    objGEOM *g = o->geom;
    g->bsphere.type = ((int *)s)[1];
    memcpy(g->bsphere.center, &s->center, 12);
    g->bsphere.radius = s->radius;
    c.data = o;
    if (!c.GetClosestPoint(p, r, flags, ci))
        return 0;
    ci->inst = o->inst;
    ci->objId = *(short *)((u8 *)o + 0x1C);
    ci->flags |= 2;
    if (o->inst)
        ci->flags |= 0x100;
    return 1;
}

// v12 0x004DBAF0  (feb 0x004CCA90)
// Does the collision info still point at an existing face / hit box.
int scnSCENE::CheckCDTInfo(cdtINFO *ci)
{
    if (!(ci->flags & 2))
        return 0;
    animINST *inst = ci->inst;
    if (inst && !(ci->flags & 0x100))
        return ci->objId >= 0 && ci->objId < inst->nBVol;
    if (ci->objId < 0 || ci->faceId < 0)
        return 0;
    objOBJ *o;
    if (!inst || !(ci->flags & 0x100)) {
        if (ci->objId >= nObj)
            return 0;
        o = objList[ci->objId];
    } else {
        o = objFindID(inst->root, ci->objId);
        if (!o)
            return 0;
    }
    return ci->faceId < o->nFace;
}

// v12 0x004DBB70  (feb 0x004CCB10)
// The face (triangle, world space) and its normal from a collision info; for a hit box its updater's
// face. Unknown or stale info: normal = +Y. 1.2: also +Y when the object has no normal list.
void scnSCENE::GetCDTFace(cdtINFO *ci, m3dPOLY *poly, m3dV *normal)
{
    if (!CheckCDTInfo(ci)) {
        if (normal)
            *normal = m3dVUnitY;
        return;
    }
    objOBJ *o;
    if (ci->inst) {
        if (!(ci->flags & 0x100)) {
            ci->inst->bvol[ci->objId].upd->GetFace(ci, poly, normal);
            return;
        }
        o = objFindID(ci->inst->root, ci->objId);
    } else {
        o = objList[ci->objId];
    }
    if (poly) {
        poly->nVert = 3;
        for (int k = 0; k < poly->nVert; k++)
            objFaceVertW(o, ci->faceId, k, &poly->vert[k]);
    }
    if (!normal)
        return;
    if (o && o->geom->faceNormal)
        *normal = o->geom->faceNormal[ci->faceId];
    else
        *normal = m3dVUnitY;
}

// v12 0x004DBE70  (feb 0x004CCE30)
objOBJ *scnSCENE::GetCDTObj(cdtINFO *ci)
{
    if (!CheckCDTInfo(ci))
        return 0;
    if (ci->inst) {
        if (!(ci->flags & 0x100))
            return 0;
        return objFindID(ci->inst->root, ci->objId);
    }
    return objList[ci->objId];
}

// feb 0x004CBF80 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();

// v12 0x004DF130  (feb 0x004D00E0)
// Near enough in a cone round dir (or within back in any direction), and nothing in the way.
int scnSCENE::IsVisiblePointCone(m3dV *from, m3dV *dir, float front, float back, float angle, m3dV *to, int flags, cdtREFINE *r, cdtINFO *ci)
{
    m3dV d = { to->x - from->x, to->y - from->y, to->z - from->z };
    float len = m3dSqrt(d.x * d.x + d.y * d.y + d.z * d.z);
    if (len >= back) {                                  // (v12 0x005699A0)
        if (len > front)
            return 0;
        m3dV t = d;
        if (fabsf(m3dAngleVector(&t, dir)) > angle)
            return 0;
    }
    scnVisPointActive = 1;
    int vis = 1;
    if (len >= 0.001f) {
        m3dV u = d;
        m3dNormalize(&u);
        vis = !ShootRay(from, &u, len, flags, r, ci);
    }
    scnVisPointActive = 0;
    return vis;
}
