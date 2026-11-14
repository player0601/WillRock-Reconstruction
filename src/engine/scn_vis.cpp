// scn_vis.cpp -- engine.lib scene ray queries: ray cast, ray vs one instance, line of sight, floor placement
// (Will Rock 1.2, v12.exe; feb obj engine:scn_vis.obj)
//
// v12 0x004DE9F0..0x004DF3BF. Like scn_coll.cpp: the level through scnSCENE::coll, the instances through
// collAnim + MakeAnimList. 1.2 changes against the beta: the query flags go into the filter
// (cdtREFINE::queryFlags) and the filter decides between enclosing volume and hit boxes; _IsectRayInst
// validates with 0x209 at LOD 5 (as the beta).

#include "engine.h"

#define SCN_EPS 1e-6f                  // v12 0x00607148

int scnVisPointActive = 0;             // v12 0x00978698 (feb 0x0093445C) 1 while IsVisiblePoint runs

static inline void cdtInfoClear(cdtINFO *c)
{
    c->flags = 0; c->inst = 0; c->objId = 0; c->faceId = 0;
    c->normal[0] = m3dVZero.x; c->normal[1] = m3dVZero.y; c->normal[2] = m3dVZero.z;
    c->dist = 0;
}

// v12 0x004DE9F0  (feb 0x004CF890)
// First hit of the ray org + t*dir, 0 <= t <= len: level (unless filter mode 1), then the instances
// (unless mode 2|8), which only count when nearer. No info: query flag 4 (any hit will do).
int scnSCENE::ShootRay(m3dV *org, m3dV *dir, float len, int qflags, cdtREFINE *ref, cdtINFO *ci)
{
    cdtINFO tmp;
    cdtInfoClear(&tmp);
    if (!ci) {
        ci = &tmp;
        qflags |= 4;
    }
    memset(ci, 0, sizeof(*ci));
    if (fabsf(len) < SCN_EPS)
        return 0;
    int res = 0;
    if (!(ref->mode & 1)) {
        res = coll->ShootRay(org, dir, len, qflags, ci);
        if (res)
            len = ci->dist;
    }
    if (!((ref->mode & 2) && (ref->mode & 8)))
        res |= _ShootRayAnim(org, dir, len, qflags, ref, ci);
    return res != 0;
}

// v12 0x004DEB20  (feb 0x004CF9C0)
int scnSCENE::_ShootRayAnim(m3dV *org, m3dV *dir, float len, int qflags, cdtREFINE *ref, cdtINFO *ci)
{
    if (!(SCN_EPS * 100.0f < len))
        return 0;
    cdtINFO tmp;
    cdtInfoClear(&tmp);
    if (!ci)
        ci = &tmp;
    ref->queryFlags = qflags;
    m3dV end = { dir->x * len + org->x, dir->y * len + org->y, dir->z * len + org->z };
    scnAOBJ list[150];
    int n = MakeAnimList(org, &end, 0.0f, ref, list, 150);
    float best = len;
    int found = 0;
    for (int i = 0; i < n; i++) {
        animINST *inst = list[i].inst;
        if (scnVisPointActive && strstr(inst->tplName ? inst->tplName : (char *)"", "lustra"))
            continue;                                   // chandeliers never block the line of sight
        if (ref->IsUseBVolEncl(inst)) {
            if (_IsectRayInst(inst, org, dir, best, ref, ci)) {
                best = ci->dist;
                found = 1;
            } else if (!found) {
                continue;
            }
        } else {
            int r = inst->bvol[list[i].bvol].IsectRay(org, dir, best, ci);
            if (!r || r == 2)
                continue;
            ci->inst  = inst;
            ci->objId = list[i].bvol;
            ci->flags &= ~0x100u;
            found = 1;
            best = ci->dist;
        }
        if (qflags & 4)
            break;
    }
    if (found)
        ci->flags |= 2;
    return found;
}

// v12 0x004DED20  (feb 0x004CFBD0)
int scnSCENE::IsectRayInst(animINST *inst, m3dV *org, m3dV *dir, float len, cdtREFINE *ref, cdtINFO *ci)
{
    cdtREFINE def(0, 0);
    cdtINFO tmp;
    cdtInfoClear(&tmp);
    if (!ci)
        ci = &tmp;
    memset(ci, 0, sizeof(*ci));
    if (!ref)
        ref = &def;
    return _IsectRayInst(inst, org, dir, len, ref, ci);
}

// v12 0x004DEDC0  (feb 0x004CFC70)
// Ray against the faces of one instance: its own collision data for the current frame, else each object.
int scnSCENE::_IsectRayInst(animINST *inst, m3dV *org, m3dV *dir, float len, cdtREFINE *ref, cdtINFO *ci)
{
    cdtCOLL_OBJ obj;
    inst->LODSetTemp(5, 0);
    inst->Validate(0x209);
    float best = len;
    int found = 0;
    cdtCOLL *own = *(cdtCOLL **)((u8 *)inst + 0x144);
    if (own && ((cdtCOLL_INST *)own)->IsCDTFrame()) {
        found = own->ShootRay(org, dir, len, 0, ci);
        if (found) {
            ci->inst = inst;
            ci->flags |= 0x100;
        }
    } else {
        int hitObj = 0, hitFace = 0;
        m3dV hitPt = { 0, 0, 0 };
        for (int i = 0; i < inst->nObj; i++) {
            objOBJ *o = inst->objTable[i];
            if (!o || !o->nFace || !ref->IsInclObj(o))
                continue;
            objGEOM *g = inst->objTable[i]->geom;
            m3dSPHERE *s = animGetObjBSphere(inst->objTable[i]);
            g->bsphere.type = ((int *)s)[1];
            memcpy(g->bsphere.center, (u8 *)s + 8, 12);
            g->bsphere.radius = ((float *)s)[5];
            obj.data = inst->objTable[i];
            if (obj.ShootRay(org, dir, best, 0, ci)) {
                hitFace = ci->faceId;
                best = ci->dist;
                hitPt.x = ci->normal[0];  hitPt.y = ci->normal[1];  hitPt.z = ci->normal[2];
                found = 1;
                hitObj = i;
            }
        }
        if (found) {
            ci->objId  = hitObj;
            ci->faceId = hitFace;
            ci->dist   = best;
            ci->normal[0] = hitPt.x;  ci->normal[1] = hitPt.y;  ci->normal[2] = hitPt.z;
            ci->inst   = inst;
            ci->flags |= 0x102;
        }
    }
    inst->LODRestore();
    obj.Destroy();
    return found;
}

// v12 0x004DEFD0  (feb 0x004CFE80)
// Line of sight from -> to: blocked only by a hit nearer than 'to'. Chandeliers are ignored (_ShootRayAnim).
int scnSCENE::IsVisiblePoint(m3dV *from, m3dV *to, int qflags, cdtREFINE *ref, cdtINFO *ci)
{
    cdtINFO tmp;
    cdtInfoClear(&tmp);
    cdtINFO *c = ci ? ci : &tmp;
    scnVisPointActive = 1;
    m3dV dir = { to->x - from->x, to->y - from->y, to->z - from->z };
    float d = m3dSqrt(dist2(*from, *to));
    if (0.001f <= d) {                                  // v12 0x005FB874
        m3dNormalize(&dir);
        if (ShootRay(from, &dir, d, qflags, ref, c)) {
            scnVisPointActive = 0;
            if (c->dist < m3dSqrt(dist2(*from, *to)))
                return 0;
        }
    }
    scnVisPointActive = 0;
    return 1;
}

// v12 0x004DF2B0  (feb 0x004D0370)
// Drop p onto the floor: ray down from 'up' above p (at most min(maxDown, 10000)); out.y = floor + h1.
// info->dist becomes the height of p above the floor.
int scnSCENE::FloorPlace(m3dV *p, float maxDown, m3dV *out, float up, float h1, int qflags, cdtREFINE *ref, cdtINFO *ci)
{
    m3dV tmpOut;
    cdtINFO tmp;
    cdtInfoClear(&tmp);
    if (!out)
        out = &tmpOut;
    if (!ci)
        ci = &tmp;
    *out = *p;
    if (10000.0f < maxDown)
        maxDown = 10000.0f;
    m3dV org = { p->x, up + p->y, p->z };
    m3dV down = { 0.0f, -1.0f, 0.0f };
    if (!ShootRay(&org, &down, maxDown, qflags, ref, ci))
        return 0;
    float y = (ci->dist <= up) ? out->y : p->y;
    ci->dist = y - ci->normal[1];
    out->y = h1 + ci->normal[1];
    return 1;
}

// feb 0x004D0490 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
