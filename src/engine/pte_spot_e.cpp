// pte_spot_e.cpp -- CPU shader: a stretched (elliptic) spot (Will Rock 1.2, v12.exe; feb obj engine:pte_spot_e.obj)
//
// pteSPOT_ELLIPSE: the base of the blood stains (wrsfxBLOOD_SURF) and the blood on the hands. Faces whose plane is
// within the radius and whose ground outline (x/z) meets the circle, and that reach its height range, take texture
// coordinates from the spot's own frame: floors from x (stretched) / z, walls from x / y or y / z (both stretched),
// 0.5 + coordinate x 0.5 / radius.

#include "engine.h"
#include "../../include/engine_render/shader.h"

int __fastcall m3xzIsIsectPolyCircle(m3dPOLY *poly, m3dV *c, float r);   // m3d_misc.cpp

static const float ellBig  = 2000000.0f;        // v12 0x006071A8
static const float ellSqrt2 = 1.4142135f;       // v12 0x006071AC

static inline void ellXform(const m3dV *p, const m3dMATR *m, float *o)
{
    float x = p->x, y = p->y, z = p->z;
    o[0] = x * m->m[0][0] + m->m[3][0] + y * m->m[1][0] + z * m->m[2][0];
    o[1] = x * m->m[0][1] + m->m[3][1] + y * m->m[1][1] + z * m->m[2][1];
    o[2] = x * m->m[0][2] + m->m[3][2] + y * m->m[1][2] + z * m->m[2][2];
}

// v12 0x004E1700  (feb 0x004D3470) exact
// At pos, along dir (up given or the world's), stretched; the segment it covers.
void pteSPOT_ELLIPSE::SetParams(m3dV *pos, m3dV *dir, m3dV *up, float st, float p2)
{
    m3dMATR l2w;
    param2 = p2;
    stretch = st;
    SetOrg(pos);
    if (up)
        l2w.MakeLCS2WCS(pos, dir, up, 0);
    else
        l2w.MakeLCS2WCS_VZ(pos, dir);
    l2w.Invert(&w2l);
    float f = (stretch + 1.0f) * radius;
    segA.x = -f * dir->x + pos->x;
    segA.y = -f * dir->y + pos->y;
    segA.z = -f * dir->z + pos->z;
    segB.x = f * dir->x + pos->x;
    segB.y = f * dir->y + pos->y;
    segB.z = f * dir->z + pos->z;
}

// v12 0x004E17D0  (feb 0x004D3540)
int pteSPOT_ELLIPSE::ShadeFace(objOBJ *o, int face, pteSDR_INFO *info, ptePOLY_DATA *out)
{
    m3dV p;
    GetOrg(&p);
    float r = radius;
    m3dPOLY *poly = (m3dPOLY *)&info->nVert;
    m3dV *n = &o->geom->faceNormal[face];
    float h = (p.x - info->vert[0].x) * n->x + (p.y - info->vert[0].y) * n->y + (p.z - info->vert[0].z) * n->z;
    if (fabsf(h) > r)
        return 0;
    if (!m3xzIsIsectPolyCircle(poly, &p, r))
        return 0;
    int nv = poly->nVert;
    float yMax = -ellBig, yMin = ellBig;
    for (int i = 0; i < nv; i++) {
        float y = info->vert[i].y;
        if (yMax <= y)
            yMax = y;
        if (y <= yMin)
            yMin = y;
    }
    if (!(yMin <= p.y + r && p.y - r <= yMax))
        return 0;
    float s = 0.5f / r;
    float l[3];
    if (fabsf(m3dVUnitY.x * n->x + m3dVUnitY.y * n->y + m3dVUnitY.z * n->z) > ellSqrt2 * 0.33333334f) {
        for (int i = 0; i < nv; i++) {                  // a floor: x stretched, z
            ellXform(&info->vert[i], &w2l, l);
            out->uv[i].u = s / stretch * l[0] + 0.5f;
            out->uv[i].v = l[2] * s + 0.5f;
        }
        return 1;
    }
    if (fabsf(m3dVUnitZ.x * n->x + m3dVUnitZ.y * n->y + m3dVUnitZ.z * n->z) > ellSqrt2 * 0.5f) {
        for (int i = 0; i < nv; i++) {                  // a wall across z: x, y
            ellXform(&info->vert[i], &w2l, l);
            out->uv[i].u = s / stretch * l[0] + 0.5f;
            out->uv[i].v = s / stretch * l[1] + 0.5f;
        }
        return 1;
    }
    for (int i = 0; i < nv; i++) {                      // a wall across x: y, z
        ellXform(&info->vert[i], &w2l, l);
        out->uv[i].u = s / stretch * l[1] + 0.5f;
        out->uv[i].v = s / stretch * l[2] + 0.5f;
    }
    return 1;
}
