// ai_area.cpp -- an AI area (Will Rock 1.2, v12.exe; feb obj gs_shared:ai_area.obj)
//
// A scnDOMAIN with a list of random points that a body fits on. A point is taken by its weight (the least), set by
// the distribution: 2 "distant" (the far half from a place), 1 "even" / 4 "formation" (away from the living NPCs),
// 0x10 (behind a border from a place).

#include "../Sources/game.h"

void __fastcall m3dPolyApplyBasisCoord(m3dPOLY *p, float u, float v, m3dV *out);  // v12 0x00572160 (src/m3d/m3d_poly.cpp)

static inline float xzDist(m3dV *a, m3dV *b)
{
    float dx = a->x - b->x, dz = a->z - b->z;
    return m3dSqrt(dx * dx + dz * dz);
}

// v12 0x00548CC0  (feb an inline)
aiAREA::aiAREA()
{
    points = 0;
    nPoint = 0;
    nBound = 0;                                         // (native: v12 leaves it to the zeroed heap)
    areaFlags = 2;
}

// v12 0x00552100  (feb 0x00537940)
BOOL32 aiAREA::Init()
{
    if (!obj->nFace) {
        apLogErr((char *)"ERROR: AI area without geometry: %s", obj->name);
        return 0;
    }
    if (!scnDOMAIN::Init())
        return 0;
    MakePointList();
    obj->SetStateProcYes(8, 0);
    return 1;
}

// v12 0x00552150  (feb 0x00537990)
void aiAREA::Term()
{
    if (points)
        apFree(points);
    points = 0;
    scnDOMAIN::Term();
}

// v12 0x00552180  (feb 0x005379C0)
void aiAREA::CalcWeights(aiAREA_GET_POINT_PARAM *q)
{
    cdtREFINE ref(10, 0);
    if (areaFlags & 2) {
        float far_ = 0;
        for (int i = 0; i < nPoint; i++) {
            float d = xzDist(&points[i].pos, &q->pos);
            if (d > far_)
                far_ = d;
            points[i].weight = 1.0f;
        }
        m3dV eye = q->pos;
        eye.y += 1.0f;
        far_ *= 0.5f;
        for (int i = 0; i < nPoint; i++) {
            m3dV from = points[i].pos;
            from.y += 1.0f;
            if (xzDist(&points[i].pos, &q->pos) > far_) {
                if (!(areaFlags & 8))
                    points[i].weight = m3dRandRange(0, 0.9f);
                else if (gsScenePtr->IsVisiblePoint(&from, &eye, 0, &ref, 0))
                    points[i].weight = m3dRandRange(0, 0.9f);
                else
                    points[i].weight = 1.0f;
            }
        }
    } else if (areaFlags & 0x10) {
        float far_ = 0;
        m3dV from = q->crossFrom;
        for (int i = 0; i < nPoint; i++) {
            m3dV to = points[i].pos;
            if (q->cross->IsLSegCrossBoundaryXZ(&from, &to)) {
                points[i].weight = 1.0f;
            } else {
                float d = xzDist(&points[i].pos, &q->pos);
                if (d > far_)
                    far_ = d;
                points[i].weight = -1.0f;
            }
        }
        if (far_ > 80.0f)
            far_ = 80.0f;
        for (int i = 0; i < nPoint; i++)
            if (points[i].weight < 0) {
                float d = xzDist(&points[i].pos, &q->pos);
                if (d < far_ * 0.5f || d > far_)
                    points[i].weight = 0.95f;
                else
                    points[i].weight = m3dRandRange(0, 0.9f);
            }
    } else if (areaFlags & 5) {
        for (int i = 0; i < nPoint; i++)
            points[i].weight = 0;
        for (aiNPC *n = aiNPC::GetNext(0, 0); n; n = aiNPC::GetNext(n, 0)) {
            m3dV p;
            n->inst->GetPos(&p);
            for (int i = 0; i < nPoint; i++) {
                float d = xzDist(&points[i].pos, &p);
                points[i].weight += 1.0f / (d * d + 0.1f);
            }
        }
    }
}

// v12 0x005528D0  (feb 0x00538120)
void aiAREA::GetRandomPos(m3dV *out, aiAREA_GET_POINT_PARAM *q)
{
    CalcWeights(q);
    int best = 0;
    float w = 2000000.0f;
    for (int i = 0; i < nPoint; i++)
        if (points[i].weight < w) {
            w = points[i].weight;
            best = i;
        }
    *out = points[best].pos;
    out->y = m3dRandRange(yMin, yMax);
}

// v12 0x00552960  (feb 0x005381B0)
void aiAREA::GetRandomPos(m3dV *out, m3dV *from)
{
    aiAREA_GET_POINT_PARAM q;
    q.pos = *from;
    q.domain = 0;
    GetRandomPos(out, &q);
}

// v12 0x00552A10  (a slot in 1.2; inline in feb's MakePointList)
BOOL32 aiAREA::IsValidPoint(m3dV *p, float r)
{
    cdtREFINE ref(2, 0);
    return !gsScenePtr->GetClosestPoint(p, r, 0, &ref, 0);
}

// v12 0x00552A60  (the same)
void aiAREA::ConstrainPoint(m3dV *p, float r)
{
    cdtREFINE ref(2, 0);
    p->y += 0.5f;
    gsScenePtr->ConstrSphere(p, p, r, 0x10, &ref, 0);
}

// v12 0x00552AB0  (feb 0x00538260)
// [AI_SPAWN] nPoint (50) points on random faces of the outline, with up to 10 tries a point.
void aiAREA::MakePointList()
{
    objOBJ *o = obj;
    o->SetStateProcYes(8, 0);
    int nFace = o->nFace;
    nPoint = 50;
    if (o->props) {
        o->props->GetInt((char *)"AI_SPAWN", (char *)"nPoint", &nPoint);
        int show;
        if (o->props->GetInt((char *)"AI_SPAWN", (char *)"isShowPoints", &show) && show)
            areaFlags |= 0x20;
    }
    points = (aiAREA_POINT *)apCalloc(nPoint, sizeof(aiAREA_POINT));
    int n = 0;
    for (int t = 0; t < nPoint * 10; t++) {
        int f = m3dRandIntMax(nFace - 1);
        float u, v;
        m3dRandBasicCoord(&u, &v);
        m3dPOLY poly;
        poly.nVert = 3;
        objFACE *fc = o->GetFace(f);
        for (int k = 0; k < 3; k++) {
            m3dV *src = &o->geom->vert[fc->vert[k]];
            if (!(o->state & 1))
                m3dTransformPoint(src, (m3dMATR *)o->ltm, &poly.vert[k]);
            else
                poly.vert[k] = *src;
        }
        m3dV p;
        m3dPolyApplyBasisCoord(&poly, u, v, &p);
        ConstrainPoint(&p, 1.5f);
        if (IsValidPoint(&p, 0.7f)) {
            points[n].pos = p;
            if (++n >= nPoint)
                break;
        }
    }
    nPoint = n;
}

// v12 0x00552E40  (feb 0x00538640)
// The outline's edges: an edge two faces share goes.
void aiAREA::FillBoundaryPoly()
{
    nBound = 0;
    for (int f = 0; f < obj->nFace; f++) {
        m2dPOLY poly;
        GetPoly(f, &poly);
        for (int k = 0; k < poly.nVert; k++) {
            int i = nBound++;
            if (nBound > 0x13)
                return;
            bound0[i] = poly.vert[k];
            bound1[i] = poly.vert[(k + 1) % poly.nVert];
            for (int j = i - 1; j >= 0; j--) {
                if ((fabsf(bound0[i].y - bound0[j].y) + fabsf(bound0[i].x - bound0[j].x) < 1e-6f &&
                     fabsf(bound1[i].y - bound1[j].y) + fabsf(bound1[i].x - bound1[j].x) < 1e-6f) ||
                    (fabsf(bound0[i].y - bound1[j].y) + fabsf(bound0[i].x - bound1[j].x) < 1e-6f &&
                     fabsf(bound1[i].y - bound0[j].y) + fabsf(bound1[i].x - bound0[j].x) < 1e-6f)) {
                    for (int m = j; m < i - 1; m++) {
                        bound0[m] = bound0[m + 1];
                        bound1[m] = bound1[m + 1];
                    }
                    nBound -= 2;
                    break;
                }
            }
        }
    }
    areaFlags |= 0x20000;
}

// v12 0x00553090  (feb 0x00538890)
BOOL32 aiAREA::ISectLSegBoundaryXZ(m3dV *a, m3dV *b, m3dV *out)
{
    if (!obj)
        return 0;
    if (!(areaFlags & 0x10000))
        return scnDOMAIN::ISectLSegBoundaryXZ(a, b, out);
    if (!(areaFlags & 0x20000))
        FillBoundaryPoly();
    m2dV p, q, hit;
    p.x = a->x;
    p.y = a->z;
    q.x = b->x;
    q.y = b->z;
    for (int i = 0; i < nBound; i++)
        if (m2dIsectLSegLSeg(&bound0[i], &bound1[i], &p, &q, &hit)) {
            out->x = hit.x;
            out->z = hit.y;
            out->y = 0;
            return 1;
        }
    return 0;
}
