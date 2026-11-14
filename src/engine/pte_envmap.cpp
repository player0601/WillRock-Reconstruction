// pte_envmap.cpp -- engine.lib environment-map shaders (Will Rock 1.2, v12.exe; feb obj engine:pte_envmap.obj)
//
// pteEMAP: an environment map on one object or instance (blendType add / blend / mult / decal, blendCoeff);
// pteEMAP_REFLECT: a reflection around a sphere fitted to the object ("sphRadius"); pteEMAP_REFLECT_WATER: the water
// surfaces' (waterWATER::InitObj, REFLECTION section). Rebuilt: the set-up and the queries. Not rebuilt yet: the
// other kinds (chrome, specular, envir, panel, refract), PrepareFrame, ShadeFace / ShadeVert (the drawing).

#include <string.h>
#include <math.h>
#include "engine.h"
#include "../../include/engine_render/shader.h"

txmTEXTURE *pteEmapReflectTex;              // v12 0x009786C8 the reflections' default texture

// v12 0x004E2260  (feb 0x004D3E80) exact
// The base's keys; vertex normals for the object (or the instance's tree); the blend from "blendType" (default
// per kind) and "blendCoeff" (the colour's alpha = 255 * it): add 0x6B, blend 0xAB, mult 0x12B, else decal (3,
// the object drawn with state 0x20000, the colour as read).
void pteEMAP::Init()
{
    pteSHADER::Init();
    if (obj && !obj->geom->vertNormal) {
        obj->_AllocVertNormalList(obj->nVert);
        if (!obj->inst)
            obj->_CalcVertNormals();
    }
    if (inst)
        inst->root->AllocVertNormalList();
    char bt[0x40];
    strcpy(bt, GetBlendTypeDef());
    float c0 = color[0], c1 = color[1], c2 = color[2];
    ReadStr((char *)"blendType", bt, 0x40);
    ReadFloat((char *)"blendCoeff", &blendCoeff);
    float c3 = blendCoeff * 255.0f;
    int st;
    if (!_stricmp(bt, "add"))
        st = 0x6B;
    else if (!_stricmp(bt, "blend"))
        st = 0xAB;
    else if (!_stricmp(bt, "mult"))
        st = 0x12B;
    else {
        if (obj)
            obj->_SetStateRendYes(0x20000);
        else
            inst->root->SetStateRendYes(0x20000);
        SetStateRend(3);
        return;
    }
    SetStateRend(st);
    color[0] = c0;
    color[1] = c1;
    color[2] = c2;
    color[3] = c3;
}

// v12 0x004E23F0  (feb 0x004D4010) exact: its object, or an object of its instance
int pteEMAP::IsShadeObj(objOBJ *o)
{
    if (o == obj)
        return 1;
    if (o->inst && o->inst == inst)
        return 1;
    return 0;
}

// v12 0x004E58C0  its instance (any when it has none)
int pteEMAP::IsShadeInst(animINST *a)
{
    if (inst && a != inst)
        return 0;
    return 1;
}

// v12 0x004E2420  (feb 0x004D4040) exact
// The object's world matrix (when it is in an instance), else the instance's, else a translation to the object's
// box centre.
void pteEMAP::GetMatrL2W(m3dMATR *m)
{
    if (inst)
        inst->Validate(4);
    if (obj && obj->inst) {
        memcpy(m, obj->ltm, sizeof(float) * 16);
        return;
    }
    if (inst) {
        memcpy(m, inst->matr, sizeof(float) * 16);
        return;
    }
    m3dV c;
    m->Identity();
    wrVCall<void>(&obj->bbox, 0x24, &c);                // m3dBOX::GetCenter
    m->Translate(&c, 0);
}

// v12 0x004E24B0  (feb 0x004D40D0)
void pteEMAP::PrepareFrame(scnSCENE *s, camCAMERA *c)
{
    if (inst && !(inst->flags & 3))
        inst->Validate(0x400);
    if (obj && obj->inst && (!(obj->procState & 0x3F) || (obj->rendState & 0x20000)) && !(obj->inst->flags & 1)
        && !(inst->flags & 2))
        obj->inst->Validate(0x400);
    GetMatrL2W(&matrL2W);
    matrL2W.Invert(&matrW2L);
}

// v12 0x004E2FB0  (feb 0x004D4BD0) exact
pteEMAP_REFLECT::pteEMAP_REFLECT()
{
    sphere.type = 3;
    sphere.vtbl = m3dSPHERE_vftable;
    blendCoeff = 0.15f;
    tex = pteEmapReflectTex;
}

// v12 0x004E2FF0  (feb 0x004D4C10) exact
void *pteEMAP_REFLECT::__vecDelDtor(unsigned f)
{
    sphere.vtbl = m3dVOL_vftable;
    return pteSHADER::__vecDelDtor(f);
}

// v12 0x004E4550  (feb 0x004D6170)
// The env-map set-up, then the sphere: centred on the object, its radius the farther box corner + 3 (or
// "sphRadius").
void pteEMAP_REFLECT::Init()
{
    pteEMAP::Init();
    m3dMATR m;
    GetMatrL2W(&m);
    m.GetOrigin(&sphere.center);
    float *mn, *mx;
    if (obj) {
        mn = obj->bbox.min;
        mx = obj->bbox.max;
    } else {
        inst->Validate(2);
        mn = inst->root->bbox.min;
        mx = inst->root->bbox.max;
    }
    float dx = sphere.center.x - mx[0], dy = sphere.center.y - mx[1], dz = sphere.center.z - mx[2];
    float dMax = sqrtf(dx * dx + dy * dy + dz * dz);
    dx = sphere.center.x - mn[0];
    dy = sphere.center.y - mn[1];
    dz = sphere.center.z - mn[2];
    float dMin = sqrtf(dx * dx + dy * dy + dz * dz);
    float r = dMin;
    if (!(dMin > dMax))
        r = dMax;
    sphere.radius = r + 3.0f;
    ReadFloat((char *)"sphRadius", &sphere.radius);
}

// v12 0x0048D590 (shared "return 1")
int pteEMAP_REFLECT::IsActive(int n)
{
    return 1;
}

// v12 0x0047F500
char *pteEMAP_REFLECT_WATER::GetBlendTypeDef()
{
    return (char *)"blend";
}
