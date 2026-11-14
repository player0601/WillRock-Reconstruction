// obj_mod_tex.cpp -- engine.lib texture modifiers (Will Rock 1.2, v12.exe; feb obj engine:obj_mod_tex.obj)
//
// Modifiers of one texture layer of an object: its coordinates scrolled (objMOD_TEX_SCROLL) or wobbled by noise
// (objMOD_NOISE_DEFORM_UV), stepped through the cells of a texture sheet (objMOD_TEX_FRAME_ANIM: the rocket
// launcher's flame), or its texture cycled through a list (objMOD_TEX_SEQ_ANIM) -- the water and lava surfaces'
// (waterLIQUID::InitObj). Not rebuilt yet: objMOD_BLINK_TRANSP, objSET.

#include <math.h>
#include "engine.h"
#include "../../include/engine_render/texture.h"

static inline objMTL_CTRL_TBL *omtCtrlTbl(objOBJ *o)
{
    return (objMTL_CTRL_TBL *)((char *)o->geom->split + 6);
}

// v12 0x004DCF30  (feb 0x004CEAE0)
// Into the list; the layer gets coordinates of its own: a free coordinate set copied from the shared one (then
// the hardware indices again), or the set unshared from the template.
BOOL32 objMOD_TEX::AddObj(objOBJ *o)
{
    if (!objMODIFIER::AddObj(o))
        return 0;
    ctrl = omtCtrlTbl(obj)->FindMtlCtrl(layer);
    if (omtCtrlTbl(obj)->IsSharedTCCoordID(ctrl->tcCoord)) {
        int id = omtCtrlTbl(obj)->FindFreeTCCoordID();
        if (id != -1) {
            int n = obj->geom->nTexCoord[ctrl->tcCoord];
            if (!obj->_AllocTexCoord(id, n))
                return 0;
            objGEOM *g = obj->geom;
            memcpy(g->texCoord[id], g->texCoord[ctrl->tcCoord], g->nTexCoord[ctrl->tcCoord] * 8);
            ctrl->tcCoord = (u8)id;
        }
        obj->_SetDefaultTCHWIndID();
    } else if (!obj->_UnshareTexCoord(ctrl->tcCoord))
        return 0;
    *(u32 *)((char *)obj->geom + 0x66) |= 0x40;
    return 1;
}

// ---------------------------------------------------------------------------------------------- scroll

// v12 0x004DD040
objMOD_TEX_SCROLL::objMOD_TEX_SCROLL(int l) : objMOD_TEX(0x54534352, 0x1E)      // 'RCST'
{
    outBits = 0x4000;
    layer = l;
    ctrl = 0;
    _24 = 0;
    uSpeed = 0;
    vSpeed = 0;
    inBits = 0x8000;
}

// v12 0x004DCEF0
void *objMOD_TEX_SCROLL::__vecDelDtor(unsigned f)
{
    return objMODIFIER::__vecDelDtor(f);
}

// v12 0x004DD080
// The layer's coordinates moved by speed * elapsed time (each axis only when its speed is not 0), then brought
// back near 0 (_NormalizeTextCoord).
BOOL32 objMOD_TEX_SCROLL::Modify()
{
    if (fabsf(elapsedTime) < 1e-6f)
        return 1;
    objGEOM *g = obj->geom;
    int tc = ctrl->tcCoord;
    for (int i = 0; i < obj->geom->nTexCoord[ctrl->tcCoord]; i++) {
        if (!(fabsf(uSpeed) < 1e-6f))
            ((float *)g->texCoord[tc])[i * 2] += elapsedTime * uSpeed;
        if (!(fabsf(vSpeed) < 1e-6f))
            ((float *)obj->geom->texCoord[ctrl->tcCoord])[i * 2 + 1] += elapsedTime * vSpeed;
    }
    obj->_NormalizeTextCoord(ctrl->tcCoord);
    *(u32 *)((char *)obj->geom + 0x66) |= 0x4000;
    return 1;
}

// ---------------------------------------------------------------------------------------------- frame animation

// v12 0x004DD1F0  (feb 0x004CEDA0)
objMOD_TEX_FRAME_ANIM::objMOD_TEX_FRAME_ANIM(int l) : objMOD_TEX(0x5446414E, 0xA)   // 'NAFT'
{
    outBits = 0x4000;
    layer = l;
    ctrl = 0;
    uvIni = 0;
    nRows = 0;
    nCols = 0;
    nFrames = 0;
    period = 0;
    cur = 0;
    texScaleU = 0;
    texScaleV = 0;
    time = 0;
    inBits = 0x8000;
}

// v12 0x004DCEF0 (shared)
void *objMOD_TEX_FRAME_ANIM::__vecDelDtor(unsigned f)
{
    return objMODIFIER::__vecDelDtor(f);
}

// v12 0x004DD240  (feb 0x004CEDF0)
// As objMOD_TEX's, keeping the layer's coordinates as they were.
BOOL32 objMOD_TEX_FRAME_ANIM::AddObj(objOBJ *o)
{
    if (!objMOD_TEX::AddObj(o))
        return 0;
    uvIni = (float *)apRealloc(uvIni, obj->geom->nTexCoord[ctrl->tcCoord] * 8);
    if (!uvIni)
        return 0;
    memcpy(uvIni, obj->geom->texCoord[ctrl->tcCoord], obj->geom->nTexCoord[ctrl->tcCoord] * 8);
    return 1;
}

// v12 0x004DD2C0  (feb 0x004CEE70)
void objMOD_TEX_FRAME_ANIM::SetParams(int rows, int cols, float p, float t, int n, float su, float sv, int loop)
{
    nCols = cols;
    nRows = rows;
    nFrames = n < 0 ? rows * cols : n;
    time = t;
    period = p;
    texScaleV = sv;
    texScaleU = su;
    if (loop)
        flags |= 0x4000;
}

// v12 0x004DDC90  (feb 0x004CF820)
void objMOD_TEX_FRAME_ANIM::AdvanceTime()
{
    time = elapsedTime + time;
}

// v12 0x004DD320  (feb 0x004CEED0)
// Frame time / period of the sheet (looped: the time brought back under the period), shown.
BOOL32 objMOD_TEX_FRAME_ANIM::Modify()
{
    if (fabsf(elapsedTime) < 1e-6f)
        return 1;
    if (!(time < period) && (flags & 0x4000))
        time = time - period;
    int f = (int)floorf(time / period * (float)nFrames);
    if (f > nFrames - 1)
        f = nFrames - 1;
    cur = f;
    ApplyFrameNmb(f);
    *(u32 *)((char *)obj->geom + 0x66) |= 0x4000;
    return 1;
}

// v12 0x004DD3E0  (feb 0x004CEF90)
// Cell n: its column moves u by n % cols / cols, its row v by n / cols / rows (with the scales set, by the sheet's
// share of the texture instead).
void objMOD_TEX_FRAME_ANIM::ApplyFrameNmb(int n)
{
    int row = n / nCols, col = n % nCols;
    float dv = (float)row / (float)nRows;
    float du = (float)col / (float)nCols;
    if (texScaleU > 0.0f && texScaleV > 0.0f && obj->nFace > 0) {
        txmTEXTURE *t = obj->GetFaceTexRendMtl(0, layer);
        if (t) {
            dv = (float)nCols / (float)t->w * texScaleU * dv;
            du = (float)nRows / (float)t->h * texScaleV * du;
        }
    }
    float *uv = (float *)obj->geom->texCoord[ctrl->tcCoord];
    for (int i = 0; i < obj->geom->nTexCoord[ctrl->tcCoord]; i++) {
        uv[i * 2] = du + uvIni[i * 2];
        uv[i * 2 + 1] = dv + uvIni[i * 2 + 1];
    }
}

// ---------------------------------------------------------------------------------------------- texture sequence

// v12 0x004DD4F0
objMOD_TEX_SEQ_ANIM::objMOD_TEX_SEQ_ANIM(int l) : objMODIFIER(0x5446414E, 0xA)   // 'NAFT'
{
    cur = -1;
    layer = l;
    nTex = 0;
    tex = 0;
    period = 0;
    time = 0;
    inBits = 0x8000;
    outBits = 0x4000;
}

// v12 0x004DD540
void *objMOD_TEX_SEQ_ANIM::__vecDelDtor(unsigned f)
{
    apFree(tex);
    return objMODIFIER::__vecDelDtor(f);
}

// v12 0x004DD5A0
BOOL32 objMOD_TEX_SEQ_ANIM::AddObj(objOBJ *o)
{
    if (!objMODIFIER::AddObj(o))
        return 0;
    return obj->UnshareFaceSplit() != 0;
}

// v12 0x004DD5D0
BOOL32 objMOD_TEX_SEQ_ANIM::SetParams(int n, txmTEXTURE **t, float p)
{
    nTex = n;
    period = p;
    tex = (txmTEXTURE **)apRealloc(tex, n * 4);
    if (!tex)
        return 0;
    memcpy(tex, t, nTex * 4);
    return 1;
}

// v12 0x004DDCA0
void objMOD_TEX_SEQ_ANIM::AdvanceTime()
{
    time += elapsedTime;
}

// v12 0x004DD630
// The texture for the time within the period (wrapping once per call); a new one is sent to the hardware and set
// on the layer.
BOOL32 objMOD_TEX_SEQ_ANIM::Modify()
{
    if (fabsf(elapsedTime) < 1e-6f)
        return 1;
    if (!(time < period))
        time -= period;
    float f = time / period * nTex;
    int i = (int)floorf(f);                                 // m3dIntFloor
    int last = nTex - 1;
    if (i < last)
        last = i;
    if (cur != last) {
        cur = last;
        txmManager->HwDownload(tex[last]);
        obj->SetTex(layer, tex[cur]);
    }
    return 1;
}

// ---------------------------------------------------------------------------------------------- noise UV

// v12 0x004DD8E0  (feb 0x004CF470)
objMOD_NOISE_DEFORM_UV::objMOD_NOISE_DEFORM_UV(int l) : objMOD_TEX(0x4E445556, 0x3C)   // 'VUDN'
{
    layer = l;
    ctrl = 0;
    uvIni = 0;
    outBits = 0x4000;
    maxAmp = 0;
    freqXZ = 0;
    freqTime = 0;
    dist = 2000000.0f;
    prism.vtbl = m3dPRISM_vftable;
    prism.type = 2;
    time = 0;
    inBits = 0x8000;
}

// v12 0x004DDC50  (feb 0x004CF7E0)
void *objMOD_NOISE_DEFORM_UV::__vecDelDtor(unsigned f)
{
    prism.vtbl = m3dVOL_vftable;
    apFree(uvIni);
    return objMODIFIER::__vecDelDtor(f);
}

// v12 0x004DD950  (feb 0x004CF4E0)
// The layer's coordinates kept; the prism fitted around the object's world vertices and grown by maxAmp.
BOOL32 objMOD_NOISE_DEFORM_UV::AddObj(objOBJ *o)
{
    if (!objMOD_TEX::AddObj(o))
        return 0;
    int n = obj->geom->nTexCoord[ctrl->tcCoord];
    uvIni = (float *)apRealloc(uvIni, n * 8);
    if (!uvIni)
        return 0;
    memcpy(uvIni, obj->geom->texCoord[ctrl->tcCoord], obj->geom->nTexCoord[ctrl->tcCoord] * 8);
    obj->TransformPoints(0, 0);
    int nv = obj->nVert;
    wrVCall<void>(&prism, 0x14, objOBJ::GetTransformedVertList(), nv);   // m3dPRISM::Calc
    wrVCall<void>(&prism, 0x2C, maxAmp);                                  // m3dPRISM::Expand
    return 1;
}

// v12 0x004DDA00
void objMOD_NOISE_DEFORM_UV::SetParams(float a, float fxz, float ft, float d)
{
    maxAmp = a;
    freqXZ = fxz;
    freqTime = ft;
    dist = d;
}

// v12 0x004F7680 (objMOD_NOISE_DEFORM's, shared)
void objMOD_NOISE_DEFORM_UV::AdvanceTime()
{
    time += elapsedTime;
}

// v12 0x004DDA20  (feb 0x004CF590)
// Not when no time passed or the view is out of range; else each coordinate = its kept value + maxAmp * (n + 1) / 2,
// n = noise of (fraction of u * freqXZ, time * freqTime, fraction of v * freqXZ) for u, and with pi - time *
// freqTime for v.
BOOL32 objMOD_NOISE_DEFORM_UV::Modify()
{
    if (fabsf(elapsedTime) < 1e-6f)
        return 1;
    if (!wrVCall<BOOL32>(&prism, 0x10, &viewPos, 2000000.0f, (m3dV *)0, &dist, (int *)0))   // m3dPRISM::DistPoint
        return 1;
    for (int i = 0; i < obj->geom->nTexCoord[ctrl->tcCoord]; i++) {
        float u = uvIni[i * 2], v = uvIni[i * 2 + 1];
        m3dV p;
        p.x = (u - (float)floor(u)) * freqXZ;
        p.z = (v - (float)floor(v)) * freqXZ;
        p.y = time * freqTime;
        float du = (wrVCall<float>(m3dNoise, 8, &p) + 1.0f) * 0.5f;     // m3dNOISE_GRAD::GetValue
        p.y = 3.14159265f - time * freqTime;
        float dv = (wrVCall<float>(m3dNoise, 8, &p) + 1.0f) * 0.5f;
        float *tc = (float *)obj->geom->texCoord[ctrl->tcCoord];
        tc[i * 2] = du * maxAmp + uvIni[i * 2];
        tc[i * 2 + 1] = dv * maxAmp + uvIni[i * 2 + 1];
    }
    *(u32 *)((char *)obj->geom + 0x66) |= 0x4000;
    return 1;
}
