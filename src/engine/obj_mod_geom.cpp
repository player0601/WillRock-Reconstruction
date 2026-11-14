// obj_mod_geom.cpp -- engine.lib vertex modifiers (Will Rock 1.2, v12.exe; feb obj engine:obj_mod_geom.obj)
//
// objMOD_GEOM keeps copies of an object's vertices (its own, or in world space); objMOD_NOISE_DEFORM moves the
// world copy up and down with gradient noise: the water surfaces' waves (waterLIQUID::InitObj).
// Rebuilt: v12 0x004F6E90..0x004F76EE (objMOD_GEOM, objMOD_NOISE_DEFORM).

#include <math.h>
#include "engine.h"
#include "../m3d/cgm.h"

// v12 0x004F6E90  (feb 0x004E2C40)
objMOD_GEOM::objMOD_GEOM(u32 t, int p) : objMODIFIER(t, p)
{
    vertLCS = 0;
    vertWCS = 0;
    outBits = 0x200;
}

// v12 0x004F6EC0  (feb 0x004E2C70)
void *objMOD_GEOM::__vecDelDtor(unsigned f)
{
    apFree(vertLCS);
    apFree(vertWCS);
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004F6F30
// Into the list; the object gets its own vertices (the modifier writes them) and no object-space normals.
BOOL32 objMOD_GEOM::AddObj(objOBJ *o)
{
    if (!objMODIFIER::AddObj(o))
        return 0;
    if (!obj->_UnshareVertList())
        return 0;
    obj->_FreeNormalListLCS();
    obj->_FreeVertNormalListLCS();
    *(u32 *)((char *)obj->geom + 0x66) |= 0x20;
    return 1;
}

// v12 0x004F6F80
// lcs: a copy of o's vertices; wcs: them in world space (as they are when the object is already in world space).
BOOL32 objMOD_GEOM::StoreIniVert(objOBJ *o, int lcs, int wcs)
{
    if (lcs) {
        vertLCS = (m3dV *)apRealloc(vertLCS, o->nVert * sizeof(m3dV));
        if (!vertLCS)
            return 0;
        memcpy(vertLCS, o->geom->vert, o->nVert * sizeof(m3dV));
    }
    if (wcs) {
        vertWCS = (m3dV *)apRealloc(vertWCS, o->nVert * sizeof(m3dV));
        if (!vertWCS)
            return 0;
        if (o->inst)
            o->inst->Validate(0x200);
        for (int i = 0; i < o->nVert; i++) {
            if (!(o->state & 1))
                m3dTransformPoint(&o->geom->vert[i], (m3dMATR *)o->ltm, &vertWCS[i]);
            else
                vertWCS[i] = o->geom->vert[i];
        }
    }
    return 1;
}

// v12 0x004F7210  (feb 0x004E4C40)
objMOD_NOISE_DEFORM::objMOD_NOISE_DEFORM() : objMOD_GEOM(0x4E444546, 0x37)      // 'NDEF'
{
    maxAmp = 0;
    freqXZ = 0;
    freqTime = 0;
    weight = 0;
    dist = 2000000.0f;
    prism.vtbl = m3dPRISM_vftable;
    prism.type = 2;
    time = 0;
    inBits = 0x8000;
}

// v12 0x004F76A0  (feb 0x004E51B0)
void *objMOD_NOISE_DEFORM::__vecDelDtor(unsigned f)
{
    apFree(weight);
    prism.vtbl = m3dVOL_vftable;
    return objMOD_GEOM::__vecDelDtor(f);
}

// v12 0x004F7270
// The surface's world vertices kept, the object made to own its vertices, the prism fitted around the surface
// and grown by the wave height.
BOOL32 objMOD_NOISE_DEFORM::AddObj(objOBJ *o)
{
    if (!StoreIniVert(o, 0, 1))
        return 0;
    if (!objMODIFIER::AddObj(o))
        return 0;
    if (!obj->_UnshareVertList())
        return 0;
    obj->_FreeNormalListLCS();
    obj->_FreeVertNormalListLCS();
    *(u32 *)((char *)obj->geom + 0x66) |= 0x20;
    wrVCall<void>(&prism, 0x14, vertWCS, obj->nVert);              // m3dPRISM::Calc
    wrVCall<void>(&prism, 0x2C, maxAmp);                           // m3dPRISM::Expand
    return 1;
}

// v12 0x004F72F0
void objMOD_NOISE_DEFORM::SetParams(float a, float fxz, float ft, float d)
{
    maxAmp = a;
    freqXZ = fxz;
    freqTime = ft;
    dist = d;
}

// v12 0x004F7310
// The waves fade out towards the mesh's open edges: each vertex's weight goes from 0 on the boundary to 1 at
// distance d along the edges (Dijkstra from the boundary vertices). 0: no memory or no mesh graph.
BOOL32 objMOD_NOISE_DEFORM::FixBoundary(float d)
{
    cgmWED_3D w;
    if (!obj)
        return 1;
    weight = (float *)apMalloc(obj->nVert * 4);
    if (!weight)
        return 0;
    if (!w.Init(obj))
        return 0;
    w.CalcBoundary(0);
    w.vert = vertWCS;
    int *bnd = (int *)apCalloc(obj->nVert, 4);
    if (!bnd)
        return 0;
    int n = 0;
    for (int i = 0; i < obj->nVert; i++)
        if (w.node[i].flags & 0x10)
            bnd[n++] = i;
    w.Dijkstra(0, 0, n, bnd, 0, cgmWED_SYS::pQueryDefault);
    for (int i = 0; i < obj->nVert; i++) {
        float x = w.node[i].dist, x0 = 0.0f, y0 = 0.0f, x1 = d, y1 = 1.0f, v;
        if (d < 0.0f) {
            x0 = d;
            y0 = 1.0f;
            x1 = 0.0f;
            y1 = 0.0f;
        }
        if (x < x0)
            v = y0;
        else if (!(x <= x1))
            v = y1;
        else
            v = (y1 - y0) * (x - x0) / (x1 - x0) + y0;
        weight[i] = v;
    }
    apFree(bnd);
    w.Term();
    return 1;
}

// v12 0x004F7680
void objMOD_NOISE_DEFORM::AdvanceTime()
{
    time += elapsedTime;
}

// v12 0x004F7530
// Not when no time passed or the view is out of range (prism DistPoint); else every vertex at its rest place
// lifted by maxAmp * (noise(x * freqXZ, time * freqTime, z * freqXZ) + 1) / 2 * its weight. The vertices are
// world space from now on (state bit 1).
BOOL32 objMOD_NOISE_DEFORM::Modify()
{
    if (fabsf(elapsedTime) < 1e-6f)
        return 1;
    if (!wrVCall<BOOL32>(&prism, 0x10, &viewPos, 2000000.0f, (m3dV *)0, &dist, (int *)0))   // m3dPRISM::DistPoint
        return 1;
    obj->state |= 1;
    for (int i = 0; i < obj->nVert; i++) {
        m3dV *ini = &vertWCS[i];
        m3dV p = { freqXZ * ini->x, freqTime * time, freqXZ * ini->z };
        float h = (wrVCall<float>(m3dNoise, 8, &p) + 1.0f) * 0.5f;        // m3dNOISE_GRAD::GetValue
        if (weight)
            h *= weight[i];
        m3dV *v = &obj->geom->vert[i];
        v->x = ini->x;
        v->y = h * maxAmp + ini->y;
        v->z = ini->z;
    }
    *(u32 *)((char *)obj->geom + 0x66) |= 0x2000;
    return 1;
}
