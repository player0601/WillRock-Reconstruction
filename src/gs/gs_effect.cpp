// gs_effect.cpp -- the light of a point in the level for models: the lightmap texel under it, the dynamic lights
// there, and the curve that turns it into a character light (Will Rock 1.2, v12.exe; feb obj gs:gs_effect.obj)
//
// Rebuilt from the 1.2 code (v12 0x005130E0..0x00513BC9); both main functions are exact twins of the feb ones.
// This is what lights the player's weapon, NPCs and items: entENTITY::ApplyColor(cdtINFO*) and FloorPlace take
// the floor hit of a collision query and pass it here.
//

#include "../Sources/game.h"
#include "../../include/engine_render/shader.h"
#include "../../include/engine_render/texture.h"

extern vidDRIVER *vidDriver;                    // v12 0x006561C4 (feb 0x00611FE4)

static inline float gefClamp255(float v)
{
    if (v < 0.0f)
        return 0.0f;
    if (v > 255.0f)
        return 255.0f;
    return v;
}

static inline u32 gefPack(float a, float r, float g, float b)
{
    return ((((u32)(int)a << 8 | ((u32)(int)r & 0xFF)) << 8 | ((u32)(int)g & 0xFF)) << 8) | ((u32)(int)b & 0xFF);
}

static inline void gefUnpack(u32 c, m3dCOLOR *out)
{
    out->r = (float)((c >> 16) & 0xFF);
    out->g = (float)((c >> 8) & 0xFF);
    out->b = (float)(c & 0xFF);
    out->a = (float)(c >> 24);
}

// v12 0x00513690  (feb 0x00500690)
// A lightmap colour as a character light: a little of the grey level added to each channel (15%, 85% of the
// channel kept, +0.2), then a gamma of 0.6 (brightens the dark end). Alpha 255.
void __fastcall gsEffColor2CharLightColor(m3dCOLOR *c)
{
    float grey = (c->b + c->g + c->r) * 0.33333334f * 0.15f;
    float r = c->r * 0.85f + grey + 0.2f;
    float g = c->g * 0.85f + grey + 0.2f;
    float b = c->b * 0.85f + grey + 0.2f;
    r = gefClamp255(r);
    g = gefClamp255(g);
    b = gefClamp255(b);
    c->r = (float)(pow((double)(r * 0.003921569f), (double)0.6f) * 255.0);
    c->g = (float)(pow((double)(g * 0.003921569f), (double)0.6f) * 255.0);
    c->a = 255.0f;
    c->b = (float)(pow((double)(b * 0.003921569f), (double)0.6f) * 255.0);
}

// v12 0x00513810  (feb 0x00500810)
// The colour of texture layer `layer` (1 = the lightmap) at a collision hit on static geometry: the hit point's
// barycentric position in its face picks the texture coordinate, GetColorUV the colour. 0 for no static face,
// -1 when the face has no texture on that layer.
u32 __fastcall gsEffGetTexColor(cdtINFO *ci, int layer)
{
    if (!(ci->flags & 2) || ci->objId == -1 || ci->faceId == -1)
        return 0;
    objOBJ *o = gsScenePtr->GetCDTObj(ci);
    objFACE *f = o->GetFace(ci->faceId);
    txmTEXTURE *tex = o->GetFaceTexRendMtl(ci->faceId, layer);
    if (!tex)
        return 0xFFFFFFFF;
    m3dV v[3];
    if (!(o->state & 1)) {                          // the face in world space (the x87 / 3DNow! / SSE paths)
        for (int k = 0; k < 3; k++) {
            const m3dV *p = &o->geom->vert[f->vert[k]];
            const float *m = o->ltm;
            v[k].x = p->x * m[0] + p->y * m[4] + p->z * m[8] + m[12];
            v[k].y = p->x * m[1] + p->y * m[5] + p->z * m[9] + m[13];
            v[k].z = p->x * m[2] + p->y * m[6] + p->z * m[10] + m[14];
        }
    } else
        for (int k = 0; k < 3; k++)
            v[k] = *o->GetFaceVertPtr(ci->faceId, k);
    m3dV d = { ci->point[0] - v[0].x, ci->point[1] - v[0].y, ci->point[2] - v[0].z };
    m3dV e1 = { v[1].x - v[0].x, v[1].y - v[0].y, v[1].z - v[0].z };
    m3dV e2 = { v[2].x - v[0].x, v[2].y - v[0].y, v[2].z - v[0].z };
    float s, t;
    m3dBasisCoord(&e1, &e2, &d, &s, &t);
    int tcSet = o->GetTCIndIDRendMtl(layer);
    int coord = o->GetTCCoordIDRendMtl(layer);
    m2dV *uv = (m2dV *)o->geom->texCoord[coord];
    m2dV *t0 = &uv[f->tc[tcSet * 3 + 0]], *t1 = &uv[f->tc[tcSet * 3 + 1]], *t2 = &uv[f->tc[tcSet * 3 + 2]];
    float u = (t1->x - t0->x) * s + (t2->x - t0->x) * t + t0->x;
    float w = (t1->y - t0->y) * s + (t2->y - t0->y) * t + t0->y;
    return tex->GetColorUV(u, w);
}

// v12 0x005130E0  (feb 0x005000E0)
// The light at a collision hit: col = the lightmap texel there as a character light (or the hit entity's own
// light, or white), dyn = what the dynamic lights (enabled CPU shaders) add there. Returns col + dyn, packed.
// -1 (white, nothing else done) when the video driver lacks capability 0x4000 or the query hit nothing.
u32 __fastcall gsEffEnvirColor(cdtINFO *ci, m3dCOLOR *col, m3dCOLOR *dyn)
{
    m3dCOLOR c0 = { 0, 0, 0, 0 };
    if (!col)
        col = &c0;
    col->r = col->b = col->g = col->a = 255.0f;
    if (dyn)
        dyn->r = dyn->b = dyn->g = dyn->a = 0.0f;
    if (vidDriver && !(*(u32 *)(*(char **)((char *)vidDriver + 0x210) + 0x14) & 0x4000))
        return 0xFFFFFFFF;
    if (!(ci->flags & 2))
        return 0xFFFFFFFF;
    if (ci->inst && !(ci->inst->flags & 0x48)) {    // a moving instance: its entity's light
        if (ci->inst->ent)
            ci->inst->ent->GetColor(col, 0, 0);
        return gefPack(col->a, col->r, col->g, col->b);
    }
    if (ci->objId == -1 || ci->faceId == -1)
        return 0xFFFFFFFF;
    objOBJ *o = gsScenePtr->GetCDTObj(ci);
    if (!o || !o->GetFaceTexRendMtl(ci->faceId, 1)) {
        col->r = col->b = col->g = col->a = 255.0f;
    } else {
        u32 t = gsEffGetTexColor(ci, 1);
        col->r = (float)((t >> 16) & 0xFF);
        col->g = (float)((t >> 8) & 0xFF);
        col->b = (float)(t & 0xFF);
        col->a = (float)(t >> 24);
        gsEffColor2CharLightColor(col);
        if (dyn) {
            m3dCOLOR sum = { 0, 0, 0, 0 }, one;
            for (pteSHADER *s = pteManager->FindNextShader(0); s; s = pteManager->FindNextShader(s)) {
                if (!(*(u32 *)((char *)s + 4) & 1))
                    continue;
                one.r = one.g = one.b = one.a = 0.0f;
                if (s->GetLightAt((m3dV *)ci->point, 0, &one)) {
                    sum.r = one.r + sum.r;
                    sum.g = sum.g + one.g;
                    sum.b = sum.b + one.b;
                    sum.a = sum.a + one.a;
                }
            }
            u32 packed = gefPack(gefClamp255(sum.a), gefClamp255(sum.r), gefClamp255(sum.g), gefClamp255(sum.b));
            gefUnpack(packed, dyn);
            gsEffColor2CharLightColor(dyn);
        }
    }
    if (!dyn)
        return gefPack(col->a, col->r, col->g, col->b);
    return gefPack(gefClamp255(dyn->a + col->a), gefClamp255(col->r + dyn->r), gefClamp255(dyn->g + col->g),
                   gefClamp255(dyn->b + col->b));
}

// v12 0x005130D0  (feb 0x005000D0)
// The floor's light on a character.
void __fastcall gsEffApplyLMChar(entENTITY *e, cdtINFO *ci, int blend)
{
    e->ApplyColor(ci, blend);
}
