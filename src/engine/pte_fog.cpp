// pte_fog.cpp -- engine.lib ground fog shader "fog" (Will Rock 1.2, v12.exe; feb obj engine:pte_fog.obj)
//
// pteFOG_GROUND: fog filling up to 5 volumes (scnDOMAINs from outline objects) down to depth below the top; the
// water and lava entities' volumes too (waterLIQUID::ProcessMsg), which waterWATER::IsUnderWater tests.
// All of it: the set-up (ctor, Create, SetDomainObj, InitLevel, InitParam), IsActive, Update, IsShadeObj /
// IsShadeInst, and the shading (PrepareFrame, ShadeFace, ShadeVert, CalcVertFog): per vertex the fog colour with an
// alpha from the length of the view ray inside the fog (and some noise).

#include "engine.h"
#include "../../include/engine_render/shader.h"
#include "../wr_vcall.h"

// v12 0x0059A1C0  (feb 0x0057E7A0) exact
// Empty volumes; with an outline object, the first volume from it.
pteFOG_GROUND::pteFOG_GROUND(objOBJ *o)
{
    paramFlags = 0;
    fogColor[0] = fogColor[1] = fogColor[2] = fogColor[3] = 0;
    stateRend = 0xBA;
    nObj = 0;
    nInst = 0;
    time = 0;
    nDomain = 0;
    if (o) {
        domain[nDomain].obj = o;
        if (domain[nDomain].Init())
            nDomain++;
    }
}

// v12 0x0059A280  (feb 0x0057E860)
void *pteFOG_GROUND::__vecDelDtor(unsigned f)
{
    for (int i = 0; i < nDomain; i++)
        domain[i].Term();
    for (int i = 5; i-- > 0;)                           // the volumes' vtables back to scnDOMAIN / m3dVOL
        domain[i].bbox.vtbl = (void *)m3dVOL_vftable;
    return pteSHADER::__vecDelDtor(f);
}

// v12 0x0059A4A0
int pteFOG_GROUND::SetDomainObj(objOBJ *o)
{
    domain[nDomain].obj = o;
    if (!domain[nDomain].Init())
        return 0;
    nDomain++;
    return 1;
}

// v12 0x0059A2F0  (feb 0x0057E8D0)
// The section's parameters (color, depth 5, farDist 8, nearIntensity 0, farIntensity 0.8, gamma 1, isGround,
// isApplyNoise, freqXZ 0.25, freqTime 1.5, noisePercent 0.3, windSpeed 0.5, farNoiseScale 0), then InitParam.
int pteFOG_GROUND::InitLevel(scnSCENE *s)
{
    struct { u32 flags; float color[4], depth, farDist, nearIntensity, farIntensity, gamma, freqXZ, freqTime,
             noisePercent, windSpeed, farNoiseScale; } p;
    int isGround, isNoise;
    p.flags = 0;
    p.color[0] = p.color[1] = p.color[2] = p.color[3] = 255.0f;
    p.depth = 5.0f;
    p.nearIntensity = 0.0f;
    p.farIntensity = 0.8f;
    p.farDist = 8.0f;
    p.gamma = 1.0f;
    p.freqXZ = 0.25f;
    p.freqTime = 1.5f;
    p.noisePercent = 0.3f;
    p.windSpeed = 0.5f;
    p.farNoiseScale = 0.0f;
    ReadColor((char *)"color", (m3dCOLOR *)p.color);
    ReadFloat((char *)"depth", &p.depth);
    ReadFloat((char *)"farIntensity", &p.farIntensity);
    ReadFloat((char *)"nearIntensity", &p.nearIntensity);
    ReadFloat((char *)"farDist", &p.farDist);
    ReadFloat((char *)"gamma", &p.gamma);
    if (ReadBool((char *)"isGround", &isGround) && isGround)
        p.flags |= 2;
    if (ReadBool((char *)"isApplyNoise", &isNoise) && isNoise)
        p.flags |= 1;
    ReadFloat((char *)"freqXZ", &p.freqXZ);
    ReadFloat((char *)"freqTime", &p.freqTime);
    ReadFloat((char *)"noisePercent", &p.noisePercent);
    ReadFloat((char *)"windSpeed", &p.windSpeed);
    ReadFloat((char *)"farNoiseScale", &p.farNoiseScale);
    InitParam(s, (u32 *)&p);
    return 1;
}

// v12 0x0059A500  (feb 0x0057EAE0)
// The parameters taken; the plane through the top of the first volume; every volume's bottom = top - depth, its
// box, and the scene's static objects (up to 500) and not-animated instances (up to 20) in it.
int pteFOG_GROUND::InitParam(scnSCENE *s, u32 *params)
{
    memcpy(&paramFlags, params, 15 * 4);
    planeN = m3dVUnitY;
    planePt.y = domain[0].yMax;
    planePt.x = 0;
    planePt.z = 0;
    for (int d = 0; d < nDomain; d++) {
        scnDOMAIN *dm = &domain[d];
        dm->yMin = planePt.y - depth;
        dm->CalcBBox();
        for (int i = 0; i < s->nObj; i++) {
            objOBJ *o = s->objList[i];
            if (o->state & 0x10)
                continue;
            if (!dm->_IsIsectObj(o))
                continue;
            if (nObj >= 500)
                break;
            objs[nObj++] = s->objList[i];
        }
        for (animINST *a = s->FindNextInst(0); a; a = s->FindNextInst(a)) {
            if (!(a->flags & 0x40) || (a->tpl && a->tpl->nSeq))
                continue;
            if (!dm->IsIsectObj(a->root))
                continue;
            if (nInst >= 20)
                break;
            insts[nInst++] = a;
        }
    }
    return 1;
}

// v12 0x0059A660
// Not when its instance (or its object's) is hidden (flags 2), nor when its object is switched off (procState 0x20).
int pteFOG_GROUND::IsActive(int n)
{
    if (inst) {
        if (inst->flags & 2)
            return 0;
    } else if (obj) {
        if (obj->inst && (obj->inst->flags & 2))
            return 0;
        if (obj->procState & 0x20)
            return 0;
    }
    return 1;
}

// v12 0x0059A6B0
void pteFOG_GROUND::Update(float dt)
{
    time += dt;
}

// v12 0x0059A6D0
// Everything when the camera is in a ground fog ("isGround"); else the instances listed, or those whose box meets a volume.
int pteFOG_GROUND::IsShadeInst(animINST *a)
{
    m3dBOX b;
    b.type = 1;
    if (!camOut && (paramFlags & 2))
        return 1;
    if ((a->flags & 0x40) && (!a->tpl || !a->tpl->nSeq))
        for (int i = 0; i < nInst; i++)
            if (a == insts[i])
                return 1;
    a->GetBBox(&b);
    for (int i = 0; i < nDomain; i++)
        if (domain[i].IsIsectBox(&b))
            return 1;
    return 0;
}

// v12 0x0059A790
// Not hidden objects (state 0x10); everything when the camera is outside a ground fog; else the static objects
// listed, or the objects of the instances listed.
int pteFOG_GROUND::IsShadeObj(objOBJ *o)
{
    int r = 0;
    if (o->state & 0x10)
        return 0;
    if (!camOut && (paramFlags & 2))
        return 1;
    animINST *a = o->inst;
    if (!a) {
        for (int i = 0; i < nObj; i++)
            if (o == objs[i])
                return 1;
        return 0;
    }
    if ((a->flags & 0x40) && (!a->tpl || !a->tpl->nSeq))
        for (int i = 0; i < nInst; i++)
            if (a == insts[i])
                r = 1;
    return r;
}

// (feb 0x0057E690; in 1.2 its call is inlined into the start-up)
int __fastcall pteFogInit()
{
    return pteManager->RegisterClass((char *)"fog", 0x32, pteFOG_GROUND::Create) != 0;
}

// v12 0x0059A0D0  (feb 0x0057E6B0) exact: only on an object (its outline: the first volume)
pteSHADER *__fastcall pteFOG_GROUND::Create(animINST *, objOBJ *o)
{
    if (!o)
        return 0;
    return new pteFOG_GROUND(o);
}

// v12 0x0059A850  (feb 0x0057EE30) exact
// Where the camera is: above the top, or in none of the volumes -> "out" (only the fog seen through counts).
void pteFOG_GROUND::PrepareFrame(scnSCENE *, camCAMERA *c)
{
    c->GetOrigin(&camPos);
    if (planePt.y < camPos.y) {
        camOut = 1;
        return;
    }
    if (nDomain <= 0) {
        camOut = 0;
        return;
    }
    camOut = 1;
    for (int i = 0; i < nDomain; i++)
        if (domain[i].IsBelongPoint(&camPos)) {
            camOut = 0;
            return;
        }
}

// (the clamped linear map of CalcVertFog: x0 -> y0, x1 -> y1, either order)
static float fogLinear(float x, float x0, float y0, float x1, float y1)
{
    if (x1 < x0) {
        float t = x0;
        x0 = x1;
        x1 = t;
        t = y0;
        y0 = y1;
        y1 = t;
    }
    if (x < x0)
        return y0;
    if (x > x1)
        return y1;
    return (x - x0) * (y1 - y0) / (x1 - x0) + y0;
}

// v12 0x0059AF10  (feb 0x0057F4F0)
// The fog at world point p seen from eye: the length of the ray inside it (the camera in the fog and p in it: all of
// it; else up to / from the top plane), mapped 0 -> nearIntensity, farDist -> farIntensity, to the power gamma;
// with noise ("isApplyNoise"): blended with drifting gradient noise by noisePercent (towards farNoiseScale far off).
float pteFOG_GROUND::CalcVertFog(m3dV *eye, m3dV *p)
{
    float d;
    if (camOut && !(planePt.y - 1e-6f > p->y))
        return 0.0f;
    if (!camOut && p->y < planePt.y) {
        float dx = camPos.x - p->x, dy = camPos.y - p->y, dz = camPos.z - p->z;
        d = m3dSqrt(dx * dx + dy * dy + dz * dz);
    } else {
        m3dV dir;
        dir.x = p->x - eye->x;
        dir.y = p->y - eye->y;
        dir.z = p->z - eye->z;
        m3dNormalize(&dir);
        if (!m3dIsectRayVNPlane(&camPos, &dir, &planePt, &planeN, 0, &d))
            d = fabsf((camPos.x - planePt.x) * planeN.x + (camPos.y - planePt.y) * planeN.y +
                      (camPos.z - planePt.z) * planeN.z);
        if (camOut) {
            float dx = camPos.x - p->x, dy = camPos.y - p->y, dz = camPos.z - p->z;
            d = m3dSqrt(dx * dx + dy * dy + dz * dz) - d;
        }
    }
    float f = powf(fogLinear(d, 0.0f, nearIntensity, farDist, farIntensity), gamma);
    if (!(paramFlags & 1))
        return f;
    m3dV q;
    q.x = freqXZ * p->x + windSpeed * time;
    q.y = freqTime * time;
    q.z = freqXZ * p->z;
    float n = m3dNoise ? wrVCall<float>(m3dNoise, 8, &q) : 1.0f;    // m3dNOISE_GRAD::GetValue
    float w = fogLinear(d, 0.0f, 1.0f, farDist, farNoiseScale) * noisePercent;
    f = ((1.0f - w) + w * n) * f;
    if (f < 0.0f)
        return 0.0f;
    if (f > 1.0f)
        return 1.0f;
    return f;
}

static inline void fogToWorld(objOBJ *o, m3dV *s, m3dV *w)
{
    if (o->state & 1) {
        *w = *s;
        return;
    }
    float *m = o->ltm;
    w->x = s->x * m[0] + s->y * m[4] + s->z * m[8] + m[12];
    w->y = s->x * m[1] + s->y * m[5] + s->z * m[9] + m[13];
    w->z = s->x * m[2] + s->y * m[6] + s->z * m[10] + m[14];
}

static inline u32 fogARGB(float a, float r, float g, float b)
{
    return ((((u32)(int)a << 8 | ((u32)(int)r & 0xFF)) << 8 | ((u32)(int)g & 0xFF)) << 8) | ((u32)(int)b & 0xFF);
}

// v12 0x0059ACE0  (feb 0x0057F2C0) exact
// One vertex (o's vertex v, in world space): the shader's colour, the fog as its alpha (data +9).
int pteFOG_GROUND::ShadeVert(objOBJ *o, int v, void *data)
{
    m3dV p;
    fogToWorld(o, &o->geom->vert[v], &p);
    color[3] = CalcVertFog(&camPos, &p) * 255.0f;
    u32 c = fogARGB(color[3], color[0], color[1], color[2]);
    memcpy((u8 *)data + 9, &c, 4);
    return 1;
}

// v12 0x0059A8F0  (feb 0x0057EED0) exact
// One face: its three corners in world space (none shaded when the camera is out and all three are above the top);
// the fog at each, coloured with the section's colour; a clipped face's vertices take it by their barycentric weights.
int pteFOG_GROUND::ShadeFace(objOBJ *o, int face, pteSDR_INFO *info, ptePOLY_DATA *out)
{
    m3dV w[3];
    for (int k = 0; k < 3; k++)
        fogToWorld(o, o->GetFaceVertPtr(face, k), &w[k]);
    if (camOut) {
        int k;
        for (k = 0; k < 3; k++)
            if (w[k].y < planePt.y - 1e-6f)
                break;
        if (k == 3)
            return 0;
    }
    float f[3];
    for (int k = 0; k < 3; k++)
        f[k] = CalcVertFog(&camPos, &w[k]);
    u32 *col = (u32 *)((u8 *)out + 0x44);
    if (!info->split->clipped) {
        for (int k = 0; k < 3; k++)
            col[k] = fogARGB(f[k] * 255.0f, fogColor[0], fogColor[1], fogColor[2]);
        return 1;
    }
    float d1 = f[1] - f[0], d2 = f[2] - f[0];
    for (int k = 0; k < info->nVert; k++)
        col[k] = fogARGB((d1 * info->split->a[k] + d2 * info->split->b[k] + f[0]) * 255.0f, fogColor[0], fogColor[1],
                         fogColor[2]);
    return 1;
}
