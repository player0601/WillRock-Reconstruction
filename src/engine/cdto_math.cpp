// cdto_math.cpp -- engine.lib generic collision queries over the cdtCOLL accessor interface
// (Will Rock 1.2, v12.exe; feb obj engine:cdto_math.obj)
//
// v12 0x004F5C50..0x004F6F5F. These are cdtCOLL_OBJ methods, inherited unchanged by the level
// (cdtCOLL_SCN) and instance (cdtCOLL_INST) collision: everything they need comes through the virtual
// accessors, so one ray / closest-point algorithm serves 1-level data (an object: ids are faces) and
// 2-level data (ids are objects, id2 their faces). With a grid (cellObj != 0) only the ids found in the
// cells the query touches are tested; per-face lists (flags & 1) narrow the faces too.
//
// The beta decompile of these functions is unusable (registers lost); ShootRay and GetClosestPoint are
// rebuilt from the 1.2 disassembly. ShootCyl and the two other GetClosestPoint forms (segment / query
// object, used by AI and weapons) are not rebuilt yet.

#include "engine.h"

#define CDTO_EPS 1e-6f                 // v12 0x0060785C

cdtOFL *cdtOflPtr = 0;                 // v12 0x00978758 (feb 0x00934510)

// v12 0x004F5C50  (feb 0x004E1A10)
BOOL32 __fastcall cdtoInit()
{
    cdtOFL *o = (cdtOFL *)operator new(sizeof(cdtOFL));
    if (!o) {
        cdtOflPtr = 0;
        return 0;
    }
    cdtOflPtr = cdtOFL_Construct(o, 2000, 8000);
    return cdtOflPtr != 0;
}

// v12 0x004F5C90  (feb 0x004E1A50)
void __fastcall cdtoTerm()
{
    cdtOFL *o = cdtOflPtr;
    if (o) {
        cdtOFL_Destroy(o);
        operator delete(o);
    }
}

// Candidate ids of a query: from the grid when there is one, else all of them.
#define CDTO_ID(i)   (useGrid ? (int)ofl->obj[i] : (i))
#define CDTO_FACE(i, k, id)                                                              \
    (twoLevel ? ((useGrid && (flags & 1)) ? (int)ofl->face[ofl->objFaceStart[i] + (k)] : (k)) : (id))

// v12 0x004F5CB0  (feb 0x004E1A70)
// First face hit by the ray org + t*dir, 0 <= t <= len. qflags: 1 = also back faces, 2 = skip front
// faces, 4 = stop at the first hit (any, not the nearest). info: id (+8), face (+0xC), hit point (+0x10),
// t (+0x1C); flags bit 0 = back face hit, bit 1 = hit.
BOOL32 cdtCOLL_OBJ::ShootRay(m3dV *org, m3dV *dir, float len, int qflags, cdtINFO *info)
{
    cdtOFL *ofl = cdtOflPtr;
    BOOL32 useGrid  = cellObj != 0;
    BOOL32 twoLevel = Is2LevelData();
    if (!IsIsectRayBVolAll(data, org, dir, len))
        return 0;
    int n;
    if (useGrid) {
        m3dV end = { len * dir->x + org->x, len * dir->y + org->y, len * dir->z + org->z };
        MakeIndListRay(org, &end, ofl, qflags);
        n = ofl->nObj;
    } else {
        n = GetNID();
    }
    float best = len;
    BOOL32 found = 0;
    int hitId = 0, hitFace = 0, hitBack = 0;
    m3dV hitPt = { 0, 0, 0 };
    for (int i = 0; i < n; i++) {
        int id = CDTO_ID(i);
        void *d2;
        int nf;
        if (twoLevel) {
            d2 = GetID2Ptr(id);
            if (!IsIsectRayBVol(data, id, org, dir, len))
                continue;
            nf = (useGrid && (flags & 1)) ? ofl->nObjFace[i] : GetNID2(id);
        } else {
            d2 = data;
            nf = 1;
        }
        BOOL32 skipBack, skipFront;
        if (IsDoubleSided(d2)) {
            skipBack = 0;
            skipFront = 0;
        } else {
            skipBack  = ~qflags & 1;
            skipFront = qflags & 2;
        }
        for (int k = 0; k < nf; k++) {
            int f = CDTO_FACE(i, k, id);
            m3dV nrm;
            GetFaceNormal(d2, f, &nrm);
            float dn = nrm.x * dir->x + nrm.z * dir->z + nrm.y * dir->y;
            int back;
            if (-CDTO_EPS < dn) {                       // ray leaves through this face: its back side
                back = 1;
                if (skipBack)
                    continue;
                nrm.x *= -1.0f;  nrm.y *= -1.0f;  nrm.z *= -1.0f;
                dn *= -1.0f;
            } else {
                back = 0;
                if (skipFront)
                    continue;
            }
            if (fabsf(dn) < CDTO_EPS)
                continue;
            m3dPOLY poly;
            GetFacePoly(d2, f, &poly);
            float t = -(-(poly.vert[0].z * nrm.z + poly.vert[0].y * nrm.y + poly.vert[0].x * nrm.x)
                        + nrm.z * org->z + nrm.y * org->y + nrm.x * org->x) / dn;
            if (t < 0 || t > best)
                continue;
            m3dV p = { t * dir->x + org->x, t * dir->y + org->y, t * dir->z + org->z };
            if (!m3dIsBelongPointPoly_A(&poly, &p, &nrm, CDTO_EPS))
                continue;
            hitPt = p;
            best = t;
            hitId = id;
            hitFace = f;
            found = 1;
            hitBack = back;
            if (qflags & 4)
                goto done;
        }
    }
    if (!found)
        return 0;
done:
    if (info) {
        info->objId  = hitId;
        info->faceId = hitFace;
        info->normal[0] = hitPt.x;  info->normal[1] = hitPt.y;  info->normal[2] = hitPt.z;
        info->dist = best;
        info->flags = hitBack ? (info->flags | 1) : (info->flags & ~1u);
        info->flags |= 2;
    }
    return 1;
}

// v12 0x004F6730  (feb 0x004E24C0)
// Closest face point to p within r. qflags bit 0: one-sided faces count from both sides. info: id, face,
// inst = 0, closest point, distance; flags |= 2. A closer hit must win by more than 100 * eps.
BOOL32 cdtCOLL_OBJ::GetClosestPoint(m3dV *p, float r, int qflags, cdtINFO *info)
{
    cdtOFL *ofl = cdtOflPtr;
    BOOL32 useGrid  = cellObj != 0;
    BOOL32 twoLevel = Is2LevelData();
    if (!twoLevel && !IsNearBVolAll(data, p, r))
        return 0;
    int n;
    if (useGrid) {
        MakeIndList(p, p, r, ofl, qflags);
        n = ofl->nObj;
    } else {
        n = GetNID();
    }
    float best = r;
    BOOL32 found = 0;
    int hitId = 0, hitFace = 0;
    m3dV hitPt = { 0, 0, 0 };
    int flags_ = flags;
    for (int i = 0; i < n; i++) {
        int id = CDTO_ID(i);
        void *d2;
        int nf;
        if (twoLevel) {
            d2 = GetID2Ptr(id);
            if (!IsNearBVol(data, id, p, r))
                continue;
            nf = (useGrid && (flags_ & 1)) ? ofl->nObjFace[i] : GetNID2(id);
        } else {
            d2 = data;
            nf = 1;
        }
        BOOL32 oneSided = (!(qflags & 1) && !IsDoubleSided(d2)) ? 1 : 0;
        int bothSides = oneSided == 0;
        for (int k = 0; k < nf; k++) {
            int f = CDTO_FACE(i, k, id);
            m3dPOLY poly;
            m3dV nrm, closest;
            float d = r;
            GetFacePolyNormal(d2, f, &poly, &nrm);
            if (!m3dDistPointPoly(p, &poly, &nrm, bothSides, &closest, &d, best))
                continue;
            if (!(CDTO_EPS * 100.0f + d <= best))
                continue;
            best = d;
            hitId = id;
            hitFace = f;
            found = 1;
            hitPt = closest;
        }
    }
    if (!found)
        return 0;
    if (info) {
        info->objId  = hitId;
        info->faceId = hitFace;
        info->inst   = 0;
        info->normal[0] = hitPt.x;  info->normal[1] = hitPt.y;  info->normal[2] = hitPt.z;
        info->dist   = best;
        info->flags |= 2;
    }
    return 1;
}

// [not rebuilt yet: swept cylinder, used by projectiles / AI] original v12 0x004F6170 (feb 0x004E1F00)
BOOL32 cdtCOLL_OBJ::ShootCyl(m3dV *a, m3dV *b, float r, int qflags, cdtINFO_LSEG *info)
{
    WR_NOT_REBUILT(0x004F6170);
    return 0;
}

// [not rebuilt yet] original v12 0x004F6A10 (feb 0x004E27B0)
BOOL32 cdtCOLL_OBJ::GetClosestPointSeg(m3dV *a, m3dV *b, float r, int qflags, cdtINFO_LSEG *i)
{
    WR_NOT_REBUILT(0x004F6A10);
    return 0;
}

// [not rebuilt yet] original v12 0x004F6CD0 (feb 0x004E2A80)
BOOL32 cdtCOLL_OBJ::GetClosestPointQ(cdtQUERY *q, int qflags)
{
    WR_NOT_REBUILT(0x004F6CD0);
    return 0;
}

// feb 0x004E24A0 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
