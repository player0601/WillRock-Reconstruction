// particle.cpp -- engine.lib particle emitters partEMITTER_PHYS (Will Rock 1.2, v12.exe; feb obj engine:particle.obj)
//
// v12 0x004D1730..0x004D42E8. Every emitter keeps a ring of particles (maxPart slots of partSize bytes, 0x3C for the
// base class) and sits in a global list of 600 (_partEmitList) that partUpdate / partRender walk each frame.
// Rebuilt: the list (partInit / partTerm / partGetNEmit), construction and destruction, emission start / stop, every
// parameter setter, the simulation (Update, UpdateUpdate / UpdateErase / UpdateGetNmbEmit / UpdateEmit, PartCreate,
// PartUpdate, PartUpdateVisible, PartApplyCDT, PredictCDT) and Partrend.obj's partUpdate / partDestroyEmitters.
// Not yet: Register / RegisterPart / partRender (Partrend.obj, the drawing).
// 1.2 changes, from the disassembly: a new float at +0x34 (the size's maximum, kept by SetSize; every later field is
// 4 bytes up); SetColor and SetSize free the curves they replace; the size curve drives the size (PartCreate /
// PartUpdate) only with flag 0x40000000; partUpdate updates an emitter that just started (state 0x40) always.

#include "engine.h"
#include "../../include/engine_render/particle.h"
#include "../m3d/m3d_spl.h"     // m3dSPL_LINEAR1D (the area weights)
float __fastcall m3dCalcTriArea2(m3dV *a, m3dV *b, m3dV *c);   // v12 0x005703A0 (m3d_poly.cpp)

partEMITTER_PHYS *_partEmitList[PART_MAX_EMITTERS];   // v12 0x008FB1B0
partPART         *_partTmpList[PART_MAX_VISIBLE];     // v12 0x008FBB10
int               _partNTmp;                          // v12 0x008FDA54
txmTEXTURE       *_partDefaultTex;                    // v12 0x008FDA50
static const float partEps = 1e-6f;                   // v12 0x00606EC8
float partEpsSq;                                      // v12 0x008FB1A8 (static init v12 0x004D1EB0)
static struct partEPS_INIT { partEPS_INIT() { partEpsSq = partEps * partEps; } } partEpsInit;
static const float partTimeBig = 1e6f;                // v12 0x00606ECC "no end" (the default emission window)

// v12 0x004D1730  (feb 0x004C0180)
// The default particle texture, an empty list.
int __fastcall partInit()
{
    _partDefaultTex = txmManager->Add((char *)"particle", 0x40003, 1);
    if (!_partDefaultTex)
        return 0;
    for (int i = 0; i < PART_MAX_EMITTERS; i++)
        _partEmitList[i] = 0;
    return 1;
}

// v12 0x004D1770  (feb 0x004C01C0)
void __fastcall partTerm()
{
    for (int i = 0; i < PART_MAX_EMITTERS; i++)
        if (_partEmitList[i])
            _partEmitList[i]->__vecDelDtor(1);
}

// v12 0x004D42D0  (feb 0x004C52F0)
int __fastcall partGetNEmit()
{
    int n = 0;
    for (int i = 0; i < PART_MAX_EMITTERS; i++)
        if (_partEmitList[i])
            n++;
    return n;
}

// v12 0x004D1790  (feb 0x004C01E0)
// Rate particles a second living about life seconds: a ring for 1.5 lifetimes of particles (3 at least).
partEMITTER_PHYS::partEMITTER_PHYS(float rate, float life)
{
    flags = 0;
    state = 0;
    int n = (int)(rate * life * 1.5f + 3.0f);
    if (n < 3)
        n = 3;
    ConstructorInit(0x3C, n, rate, life);
}

// v12 0x004D17E0  (feb 0x004C0230)
partEMITTER_PHYS::partEMITTER_PHYS(int n, float rate, float life)
{
    flags = 0;
    state = 0;
    ConstructorInit(0x3C, n, rate, life);
}

// v12 0x004D1810  (feb 0x004C0260)
// The defaults, the ring (pool + pointers) and a free slot in the global list.
int partEMITTER_PHYS::ConstructorInit(int partSize, int n, float rate, float life)
{
    state = 0;
    state = 0xC0;
    state = 0x2C0;
    emitRate = rate;
    lifeMin = life;
    sizeStart = 5.0f;
    sizeMax = 5.0f;
    intensity = rate;
    intensitySpl = 0;
    f_18 = life;
    lifeRand = 0;
    sizeEnd = 0;
    sizeRand = 1.0f;
    sizeSpl = 0;
    color = 0x7FFFFFFF;
    colorSpl = 0;
    alphaSpl = 0;
    f_58 = 0.5f;
    texScale[0] = 1.0f;
    texScale[1] = 0;
    texScale[2] = 1.0f;
    texScale[3] = 0;
    texRot[0] = texRot[1] = texRot[2] = texRot[3] = 0;
    frameFirst = 1;
    frameCount = 1;
    frameFps = 0;
    frameRand = 0;
    degNear[0] = 1.0f;
    texture = _partDefaultTex;
    degNear[1] = 1.0f;
    degFarK[0] = 1.0f;
    degFarK[1] = 1.0f;
    animEmitTime = 1e6f;
    animEmitRate = 0;
    time = 0;
    u8 *b = (u8 *)this;
    *(u32 *)(b + 0xB4) = 0;
    *(m3dV *)(b + 0xE0) = m3dVZero;
    for (int o = 0xB8; o <= 0xDC; o += 4)
        *(u32 *)(b + o) = 0;
    pos = m3dVZero;
    dir = m3dVUnitY;
    speed = m3dVZero;
    ((m3dMATR *)&cs)->Identity();
    ((m3dMATR *)&scaleCS)->Identity();
    *(m3dV *)scaleCSVal = m3dVOneXYZ;
    for (int o = 0xEC; o <= 0x100; o += 4)
        *(u32 *)(b + o) = 0;
    pos2 = m3dVZero;
    dir2 = m3dVUnitY;
    speed2 = m3dVZero;
    degFar[0] = degFar[1] = degFar[2] = degFar[3] = 1.0f;
    nArea = 0;
    area = 0;
    areaPrev = 0;
    areaNormal = 0;
    areaSpl = 0;
    cdtPlaneCount = 0;
    cdtDist = 0;
    cdtBounce = 0;
    cdtFriction = 0;
    cdtMaxHit = -1;
    parent = 0;
    parentArg = 0;
    maxPart = n;
    first = 0;
    nPart = 0;
    attachInst = 0;
    partPool = apMalloc(partSize * n);
    if (!partPool)
        return 0;
    partPtrs = (partPART **)apMalloc(maxPart * 4);
    if (!partPtrs)
        return 0;
    for (int i = 0, o = 0; i < maxPart; i++, o += partSize)
        partPtrs[i] = (partPART *)((u8 *)partPool + o);
    for (int i = 0; i < PART_MAX_EMITTERS; i++)
        if (!_partEmitList[i]) {
            _partEmitList[i] = this;
            return 1;
        }
    partGetNEmit();
    return 0;
}

// v12 0x004D1B60  (feb 0x004C05B0)  ~partEMITTER_PHYS
// A child emitter stops and is marked for deletion; the ring, the curves and the area data freed; out of the list.
void partEMITTER_PHYS::Destroy()
{
    u8 *b = (u8 *)this;
    if (parent) {
        parent->StopEmission();
        parent->state |= 0x20;
        parent = 0;
    }
    apFree(partPtrs);
    apFree(partPool);
    if (intensitySpl)
        wrVCall<void *>(intensitySpl, 4, 1u);
    if (sizeSpl)
        wrVCall<void *>(sizeSpl, 4, 1u);
    if (colorSpl)
        wrVCall<void *>(colorSpl, 4, 1u);
    if (alphaSpl)
        wrVCall<void *>(alphaSpl, 4, 1u);
    apFree(area);
    apFree(areaPrev);
    apFree(areaNormal);
    if (areaSpl)
        wrVCall<void *>(areaSpl, 4, 1u);
    static const int owned[4] = { 0xC0, 0xCC, 0xF4, 0x100 };
    for (int k = 0; k < 4; k++)
        if (void *o = *(void **)(b + owned[k]))
            wrVCall<void *>(o, 4, 1u);
    for (int i = 0; i < PART_MAX_EMITTERS; i++)
        if (_partEmitList[i] == this) {
            _partEmitList[i] = 0;
            return;
        }
}

// v12 0x00402FF0 (the vector deleting destructor, emitted early in the exe)
void *partEMITTER_PHYS::__vecDelDtor(unsigned f)
{
    Destroy();
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004D1C80  (feb 0x004C06D0)
void partEMITTER_PHYS::StartEmission()
{
    u32 s = state & ~4u;
    state = s;
    if (!(s & 8))
        state = s | 4;
    state |= 1;
    state |= 0x40;
    time = 0;
}

// v12 0x004D1CB0  (feb 0x004C0700)
void partEMITTER_PHYS::StopEmission()
{
    state &= ~1u;
}

// v12 0x004D1CC0  (feb 0x004C0710)
// Emission only in a window of len seconds out of every len + pause (flag 0x80); a window of the "no end" length
// or more switches it off.
void partEMITTER_PHYS::SetAnimEmission(float len, float pause)
{
    if (!(partTimeBig * 0.5f < animEmitTime)) {
        animEmitTime = len;
        animEmitRate = pause;
        flags |= 0x80;
        return;
    }
    flags &= ~0x80u;
}

// v12 0x004D1D10  (feb 0x004C0760)  slot +0x04
// 1.2: a new colour / alpha curve replaces (deletes) the old one.
void partEMITTER_PHYS::SetColor(uint32_t c, m3dSPL_COLOR *cs, m3dSPL_LINEAR1D *as)
{
    color = c;
    if (cs) {
        if (colorSpl)
            wrVCall<void *>(colorSpl, 4, 1u);
        colorSpl = cs;
    }
    if (as) {
        if (alphaSpl)
            wrVCall<void *>(alphaSpl, 4, 1u);
        alphaSpl = as;
    }
}

// v12 0x004D1D60  (feb 0x004C0780)  slot +0x08
void partEMITTER_PHYS::GetColor(uint32_t *c, m3dSPL_COLOR **cs, m3dSPL_LINEAR1D **as)
{
    if (c)
        *c = color;
    if (cs)
        *cs = colorSpl;
    if (as)
        *as = alphaSpl;
}

// v12 0x004D1D90  (feb 0x004C07B0)
void partEMITTER_PHYS::SetIntensity(float v, m3dSPL_LINEAR1D *s)
{
    intensity = v;
    if (s) {
        if (intensitySpl)
            wrVCall<void *>(intensitySpl, 4, 1u);
        intensitySpl = s;
    }
}

// v12 0x004D1DC0  (feb 0x004C07E0)
void partEMITTER_PHYS::SetTimeLifePart(float life, float rnd)
{
    lifeMin = life;
    lifeRand = rnd;
}

// v12 0x004D1DE0  (feb 0x004C0800)
// 1.2: the size's maximum is kept (start + end; with a curve, the curve's largest value over 0..1 in 20 steps).
void partEMITTER_PHYS::SetSize(float a, float b, m3dSPL_LINEAR1D *s, float rnd)
{
    sizeMax = a + b;
    sizeStart = a;
    sizeEnd = b;
    sizeRand = rnd;
    if (s) {
        if (sizeSpl)
            wrVCall<void *>(sizeSpl, 4, 1u);
        sizeSpl = s;
        float mn;
        wrVCall<void>(s, 0x24, 0, 1.0f, 20.0f, &mn, &sizeMax);
    }
}

// v12 0x004D1E40  (feb 0x004C0830)
void partEMITTER_PHYS::SetTexture(txmTEXTURE *t)
{
    texture = t;
}

// v12 0x004D1E50  (feb 0x004C0840)
void partEMITTER_PHYS::SetTextureScale(float u, float ur, float v, float vr)
{
    texScale[0] = u;
    texScale[1] = ur;
    texScale[2] = v;
    texScale[3] = vr;
}

// v12 0x004D1E70  (feb 0x004C0860)
void partEMITTER_PHYS::SetTextureFrameAnim(int firstFrame, int n, float fps, float rnd)
{
    frameFirst = firstFrame;
    frameCount = n;
    frameFps = fps;
    frameRand = rnd;
}

// v12 0x004D1ED0  (feb 0x004C0890)
void partEMITTER_PHYS::SetTextureRotAnim(float a, float ar, float s, float sr)
{
    texRot[0] = a;
    texRot[1] = ar;
    texRot[2] = s;
    texRot[3] = sr;
}

// v12 0x004D1EF0  (feb 0x004C08B0)
void partEMITTER_PHYS::SetDegNearParams(float a, float b)
{
    degNear[0] = a;
    degNear[1] = b;
}

// v12 0x004D2270  (feb 0x004C0C60)
// A particle past its life (time 10) goes at the next UpdateErase.
void partEMITTER_PHYS::KillParticle(partPART *p)
{
    *(float *)p = 10.0f;
}

// v12 0x004D2280 (name ours)
void partEMITTER_PHYS::ClearParticles()
{
    first = 0;
    nPart = 0;
}

// v12 0x004D23D0  (feb 0x004C0DC0)
// A position that moved by more than eps * 1000 on any axis marks the emitter moved (0x200); the direction is
// normalised.
void partEMITTER_PHYS::SetPosDirSpeed(m3dV *p, m3dV *d, m3dV *v)
{
    if (p) {
        float e = partEps * 1000.0f;
        if (!(fabsf(pos.x - p->x) < e) || !(fabsf(pos.y - p->y) < e) || !(fabsf(pos.z - p->z) < e))
            state |= 0x200;
        pos = *p;
    }
    if (d) {
        dir = *d;
        m3dNormalize(&dir);
    }
    if (v)
        speed = *v;
}

// v12 0x004D2530  (feb 0x004C0F20)
void partEMITTER_PHYS::GetPosDirSpeed(m3dV *p, m3dV *d, m3dV *v)
{
    if (p)
        *p = pos;
    if (d)
        *d = dir;
    if (v)
        *v = speed;
}

// v12 0x004D25A0  (feb 0x004C0F90)
void partEMITTER_PHYS::SetCS(m3dMATR *m)
{
    memcpy(&cs, m, 0x40);
}

// v12 0x004D25C0  (feb 0x004C0FB0)
// Triangle i of n of the emission area (keeps the previous one too, prev for the first time), its normal; the
// arrays and the weight curve grow to n.
int partEMITTER_PHYS::SetAreaEmission(int n, int i, m3dTRI *t, int keepPrev)
{
    if (nArea != n) {
        if (!(area = (m3dTRI *)apRealloc(area, n * 0x24)))
            return 0;
        if (!(areaPrev = (m3dTRI *)apRealloc(areaPrev, n * 0x24)))
            return 0;
        if (!(areaNormal = (m3dV *)apRealloc(areaNormal, n * 0xC)))
            return 0;
        if (!areaSpl) {
            m3dSPL_LINEAR1D *s = new m3dSPL_LINEAR1D;
            if (s) {
                s->flags = 0;
                s->type = 0;
                s->nKp = 0;
                s->kpValueSize = 8;
                s->kpStride = 8;
                s->dim = 1;
                s->kpBytes = 0;
                s->kp = 0;
                s->pad[0] = s->pad[1] = s->pad[2] = 0;
            }
            areaSpl = s;
            if (!s)
                return 0;
        }
        if (!areaSpl->AllocKpList(n + 1))
            return 0;
        nArea = n;
    }
    memcpy((u8 *)areaPrev + i * 0x24, keepPrev ? (void *)t : (void *)((u8 *)area + i * 0x24), 0x24);
    memcpy((u8 *)area + i * 0x24, t, 0x24);
    m3dV *v = (m3dV *)t;
    m3dGetPlaneNormal(&v[0], &v[1], &v[2], &areaNormal[i]);
    state |= 0x10;
    return 1;
}

// v12 0x004D2750  (feb 0x004C1140)
int partEMITTER_PHYS::SetCDTPlane(int n, int i, m3dPLANE *p)
{
    if (cdtPlaneCount != n)
        cdtPlaneCount = n;
    memcpy(cdtPlanes[i], p, 0x18);
    return 1;
}

// v12 0x004D2790  (feb 0x004C1180)
void partEMITTER_PHYS::SetCDTParam(float dist, float bounce, float friction, int maxHit)
{
    cdtDist = dist;
    cdtBounce = bounce;
    cdtFriction = friction;
    cdtMaxHit = maxHit;
}

// v12 0x004D27C0  (feb 0x004C11B0)
// e follows this emitter (flag 0x800 on e); this one stops e and marks it for deletion when it goes.
void partEMITTER_PHYS::AttachEmitter(partEMITTER_PHYS *e, int a, int b)
{
    parent = e;
    parentArg = b;
    e->flags |= 0x800;
}

// v12 0x004D27E0  (feb 0x004C11D0)
void partEMITTER_PHYS::SetDegFarParams(float a, float b, float c, float d)
{
    degFar[0] = a;
    degFar[1] = b;
    degFar[2] = c;
    degFar[3] = d;
}

// v12 0x004D2810  (feb 0x004C1200)
void partEMITTER_PHYS::SetScaleCSVal(float x, float y, float z)
{
    scaleCSVal[0] = x;
    scaleCSVal[1] = y;
    scaleCSVal[2] = z;
}

// v12 0x004D2840  (feb 0x004C1230)
void partEMITTER_PHYS::SetScaleCS(m3dMATR *m)
{
    memcpy(&scaleCS, m, 0x40);
}

// v12 0x00403860 (the shared empty function)  slot +0x28
void partEMITTER_PHYS::PartKill(int, partPART **)
{
}

// v12 0x004038E0  (feb 0x004035E0)  slot +0x10 (inline in the header, emitted in plw_acd.obj)
// n particles now (StartShells: the shells of a shot).
void partEMITTER_PHYS::Emit(int n, float dt)
{
    UpdateEmit(n, dt);
}

// v12 0x00403860 (the shared empty function)  slot +0x14
void partEMITTER_PHYS::PartHit(partPART *, float)
{
}

// ---- the simulation ---------------------------------------------------------------------------------------------

static inline float *partKpT(m3dSPL *s, int i)          // the key's t (GetKpPtr)
{
    return (float *)((u8 *)s->kp + s->kpStride * (i < s->nKpMax ? i : s->nKpMax));
}

// v12 0x004D1F10  (feb 0x004C0900)
// The emitter's clock; a timed emission (0x80) stops after its window, or (looped, 0x100) repeats with a pause
// (state 2 while in the pause). Then every live particle ages (slot 0x20 PartUpdate, the ring in one or two runs).
void partEMITTER_PHYS::UpdateUpdate(float dt)
{
    float t = time += dt;
    if (flags & 0x80) {
        if (!(flags & 0x100) && t > animEmitRate + animEmitTime) {
            StopEmission();
        } else {
            float period = animEmitRate + animEmitTime + partEps * 100.0f;
            float w = t;
            if (!(fabsf(period) < partEps))
                w = (float)floor(t / period) * period;
            time = t - w;
            if (time < animEmitTime)
                state &= ~2u;
            else
                state |= 2;
        }
    }
    if (nPart > 0) {
        if (first + nPart <= maxPart) {
            PartUpdate(nPart, &partPtrs[first], dt);
            return;
        }
        int n = maxPart - first;
        PartUpdate(n, &partPtrs[first], dt);
        PartUpdate(nPart - n, partPtrs, dt);
    }
}

// v12 0x004D2070  (feb 0x004C0A60)
// Particles past their life: each once to slot 0x28 PartKill (flag 4); the ring's oldest ones leave it.
void partEMITTER_PHYS::UpdateErase()
{
    int n = nPart;
    if (n <= 0)
        return;
    int f = first;
    int k = 0;
    _partNTmp = 0;
    for (int i = 0; i < n; i++) {
        partPART *p = partPtrs[(i + f) % maxPart];
        if (p->age > 1.0f) {
            if (!(p->flags & 4)) {
                p->flags |= 4;
                k = _partNTmp;
                if (_partNTmp < PART_MAX_VISIBLE) {
                    _partTmpList[_partNTmp] = p;
                    k = ++_partNTmp;
                }
            }
            if (p == partPtrs[first]) {
                nPart--;
                first = (first + 1) % maxPart;
                k = _partNTmp;
            }
        }
    }
    if (k > 0)
        PartKill(k, _partTmpList);
}

// v12 0x004D2120  (feb 0x004C0B10)
// How many to emit in dt: the intensity (or the mean of its curve over the last frame, 10 samples) times the
// distance factor; the fraction by chance.
int partEMITTER_PHYS::UpdateGetNmbEmit(float dt)
{
    float in;
    if (intensitySpl) {
        float sum = 0;
        for (int i = 0; i < 10; i++) {
            float v;
            intensitySpl->GetValue(time - (float)i * dt * 0.1f, &v, 0);
            sum += v;
        }
        in = sum * 0.1f;
    } else {
        in = intensity;
    }
    float x = in * degFarK[1] * dt;
    int n = (int)x;
    float fr = x - (float)n;
    if (fr > partEps && m3dRandom() < fr)
        n++;
    return n;
}

// v12 0x004D21E0  (feb 0x004C0BD0)
// n new particles at the ring's end (as many as fit), slot 0x1C PartCreate in one or two runs.
void partEMITTER_PHYS::UpdateEmit(int n, float dt)
{
    if (nPart + n > maxPart)
        n = maxPart - nPart;
    if (n <= 0)
        return;
    int at = (first + nPart) % maxPart;
    if (at + n <= maxPart) {
        PartCreate(n, &partPtrs[at], dt, n, 0);
    } else {
        int k = maxPart - at;
        PartCreate(k, &partPtrs[at], dt, n, 0);
        PartCreate(n - k, partPtrs, dt, n, k);
    }
    nPart += n;
}

// v12 0x004D2290  (feb 0x004C0C80)
// The visible particles' colour from the colour curve (over the particle's age, or the emitter's time with flag
// 0x1000) and the alpha from its curve (0..255).
void partEMITTER_PHYS::PartUpdateVisible(int n, partPART **list)
{
    float c[4] = { 0, 0, 0, 0 };
    if (!colorSpl && !alphaSpl)
        return;
    for (int i = 0; i < n; i++) {
        partPART *p = list[i];
        if (colorSpl) {
            colorSpl->GetValue((flags & 0x1000) ? time : p->age, c, 0);
            u32 a = (u32)(int)c[0] & 0xFF;
            a = (a | (u32)(int)c[1] << 8) << 8;
            a = (a | ((u32)(int)c[2] & 0xFF)) << 8;
            p->color = a | ((u32)(int)c[3] & 0xFF);
        }
        if (alphaSpl) {
            float a;
            alphaSpl->GetValue(p->age, &a, 0);
            if (!(a >= 0.0f))
                a = 0.0f;
            else if (a > 255.0f)
                a = 255.0f;
            p->color = (p->color & 0xFFFFFF) | (u32)(int)a << 24;
        }
    }
}

// v12 0x004D2860  (feb 0x004C1250)
// n particles of a burst of total (these from index start): life, colour, texture scales and spin, a sub-frame
// age (spread over the frame: 1 - (start + i) / total of dt), size (the size curve only with flag 0x40000000 --
// 1.2), frame. The area weights when the area changed. Then each one's place and velocity:
//   - the emitter's pos / dir / speed, interpolated from the last frame's (flags 0x10 / 0x20 / 0x40) unless the
//     emission just started (state 4);
//   - from an area (0x2000): a random vertex (0x4000) or a point of a triangle picked by area; its speed from the
//     area's motion (0x200000) or unit Y;
//   - plus an offset of radius spreadMin..spread (or in a cylinder round dir of height spreadH);
//   - velocity: speedA along a random direction in the cone round dir (the area normal with 0x8000), speedB along a
//     random tangent of dir, speedK times the speed, speedT along a tangent of the speed;
//   - unless 0x4000000, moved over the sub-frame with gravity and drag, and bounced on the CDT planes (0x10000).
void partEMITTER_PHYS::PartCreate(int n, partPART **list, float dt, int total, int start)
{
    float tot = (float)total;
    for (int i = 0; i < n; i++) {
        partPART *p = list[i];
        p->age = 0;
        p->invLife = 1.0f / (m3dRandMax(lifeRand + lifeRand) + lifeMin - lifeRand);
        p->flags = 0;
        p->color = color;
        p->nHit = 0;
        if (flags & 1)
            p->flags = 1;
        p->texU = m3dRandMax(texScale[1] + texScale[1]) + texScale[0] - texScale[1];
        p->texV = m3dRandMax(texScale[3] + texScale[3]) + texScale[2] - texScale[3];
        p->rot = m3dRandMax(texRot[1] + texRot[1]) + texRot[0] - texRot[1];
        p->rotSpeed = m3dRandMax(texRot[3] + texRot[3]) + texRot[2] - texRot[3];
        partPART *q = list[i];
        q->age += (1.0f - (float)(start + i) / tot) * q->invLife * dt;
        if (!sizeSpl)
            p->size = m3dRandMax(sizeEnd + sizeEnd) - sizeEnd + sizeStart;
        else if (flags & 0x40000000)
            sizeSpl->GetValue(list[i]->age, &list[i]->size, 0);
        p->frame = (uint16_t)(int)m3dRandMax(frameRand - partEps);
    }

    if (nArea > 0 && (state & 0x10)) {
        *partKpT(areaSpl, 0) = 0;
        for (int i = 0; i < nArea; i++) {
            m3dV *v = (m3dV *)((u8 *)area + i * 0x24);
            float a = sqrtf(m3dCalcTriArea2(&v[0], &v[1], &v[2]));
            *partKpT(areaSpl, i + 1) = a + *partKpT(areaSpl, i);
        }
        for (int i = 0; i < nArea; i++)
            *partKpT(areaSpl, i) = *partKpT(areaSpl, i) / *partKpT(areaSpl, nArea);
        *partKpT(areaSpl, nArea) = 1.0f;
        state &= ~0x10u;
    }

    for (int i = 0; i < n; i++) {
        m3dV P = pos, D = dir, V = speed;
        partPART *p = list[i];
        float t = 1.0f - (float)(i + start) / tot;
        if (!(state & 4)) {
            float u = 1.0f - t;
            if (flags & 0x10) {
                P.x = t * pos2.x + u * pos.x;
                P.y = u * pos.y + t * pos2.y;
                P.z = u * pos.z + t * pos2.z;
            }
            if (flags & 0x20) {
                D.x = u * dir.x + t * dir2.x;
                D.y = u * dir.y + t * dir2.y;
                D.z = u * dir.z + t * dir2.z;
                m3dNormalize(&D);
            }
            if (flags & 0x40) {
                V.x = t * speed2.x + u * speed.x;
                V.y = u * speed.y + t * speed2.y;
                V.z = u * speed.z + t * speed2.z;
            }
        }
        float rad;
        if (spreadSpl)
            spreadSpl->GetValue(time - t * dt, &rad, 0);
        else
            rad = spread;
        int tri = 0;
        if (nArea > 0 && (flags & 0x2000)) {
            m3dV Q;                                     // the same point on the last frame's area
            int fromArea = 0;
            if (!(flags & 0x4000)) {
                tri = areaSpl->FindSeg(m3dRandom());
                if (tri == -2)
                    tri = 0;
                else if (tri == -1)
                    tri = nArea - 1;
                float a, b;
                m3dRandBasicCoord(&a, &b);
                float *w = (float *)((u8 *)area + tri * 0x24);
                P.x = (w[3] - w[0]) * a + (w[6] - w[0]) * b + w[0];
                P.y = (w[4] - w[1]) * a + (w[7] - w[1]) * b + w[1];
                P.z = (w[5] - w[2]) * a + (w[8] - w[2]) * b + w[2];
                if (flags & 0x200000) {
                    float *o = (float *)((u8 *)areaPrev + tri * 0x24);
                    Q.x = (o[3] - o[0]) * a + (o[6] - o[0]) * b + o[0];
                    Q.y = (o[4] - o[1]) * a + (o[7] - o[1]) * b + o[1];
                    Q.z = (o[5] - o[2]) * a + (o[8] - o[2]) * b + o[2];
                    fromArea = 1;
                }
            } else {
                tri = m3dRandIntMax(nArea - 1);
                int k = tri * 3 + m3dRandIntMax(2);
                P = ((m3dV *)area)[k];
                Q = ((m3dV *)areaPrev)[k];
                fromArea = (flags & 0x200000) != 0;
            }
            if (fromArea) {
                float r = 1.0f / dt;
                V.x = r * (P.x - Q.x);
                V.y = (P.y - Q.y) * r;
                V.z = (P.z - Q.z) * r;
            } else {
                V = m3dVUnitY;
            }
        }
        m3dV off;
        if (!(fabsf(spreadH) < partEps)) {
            m3dMakeVCylinderRandom(&dir, rad, spreadH, &off);
            p->pos.x = off.x + P.x;
            p->pos.y = off.y + P.y;
            p->pos.z = off.z + P.z;
        } else {
            m3dMakeVUnitRandom(&off);
            float r = m3dRandRange(spreadMin, rad);
            p->pos.x = off.x * r + P.x;
            p->pos.y = off.y * r + P.y;
            p->pos.z = off.z * r + P.z;
        }
        float sa, sb, ang;
        if (speedASpl)
            speedASpl->GetValue(time - t * dt, &sa, 0);
        else
            sa = speedA;
        if (speedBSpl)
            speedBSpl->GetValue(time - t * dt, &sb, 0);
        else
            sb = speedB;
        sa = m3dRandMax(speedARand + speedARand) - speedARand + sa;
        sb = m3dRandMax(speedBRand + speedBRand) - speedBRand + sb;
        float sk = m3dRandMax(speedKRand + speedKRand) - speedKRand + speedK;
        float st = m3dRandMax(speedTRand + speedTRand) - speedTRand + speedT;
        if (coneSpl)
            coneSpl->GetValue(time - t, &ang, 0);
        else
            ang = cone;
        int onNormal = nArea > 0 && (flags & 0x2000) && (flags & 0x8000);
        m3dV dA, dB, dT;
        if (onNormal)
            m3dMakeVUnitConusRandom(&areaNormal[tri], ang, &dA);
        else if (cone < 180.0f - partEps)
            m3dMakeVUnitConusRandom(&D, ang, &dA);
        else
            m3dMakeVUnitRandom(&dA);
        m3dMakeVUnitRandom(&dB);
        m3dMakeVTangRandom(onNormal ? &areaNormal[tri] : &dir, &dB);
        if (!(fabsf(sqrtf(V.x * V.x + V.y * V.y + V.z * V.z)) < partEps))
            m3dMakeVTangRandom(&V, &dT);
        else
            dT = m3dVZero;
        p->vel = m3dVZero;
        p->vel.x += dA.x * sa; p->vel.y += dA.y * sa; p->vel.z += dA.z * sa;
        p->vel.x += dB.x * sb; p->vel.y += dB.y * sb; p->vel.z += dB.z * sb;
        p->vel.x += V.x * sk;  p->vel.y += V.y * sk;  p->vel.z += V.z * sk;
        p->vel.x += dT.x * st; p->vel.y += dT.y * st; p->vel.z += dT.z * st;
        if (!(flags & 0x4000000)) {
            float h = t * dt;
            m3dV old = p->pos;
            float k = -drag;
            float ay = k * p->vel.y + gravity.y, az = k * p->vel.z + gravity.z;
            p->vel.x += (k * p->vel.x + gravity.x) * h;
            p->vel.y += ay * h;
            p->vel.z += az * h;
            p->pos.x += h * p->vel.x;
            p->pos.y += h * p->vel.y;
            p->pos.z += h * p->vel.z;
            if (cdtPlaneCount > 0 && (flags & 0x10000))
                PartApplyCDT(p, &old, h);
        }
    }
}

// v12 0x004D35A0  (feb 0x004C1F90)
// Age, size (the curve with flag 0x40000000 -- 1.2) and spin; then, unless at rest, gravity, drag, motion and the
// CDT planes; a child emitter (0x40000) emits along each particle.
void partEMITTER_PHYS::PartUpdate(int n, partPART **list, float dt)
{
    for (int i = 0; i < n; i++) {
        partPART *p = list[i];
        p->age += dt * p->invLife;
        if (sizeSpl && (flags & 0x40000000))
            sizeSpl->GetValue(list[i]->age, &list[i]->size, 0);
        p = list[i];
        p->rot += dt * p->rotSpeed;
    }
    for (int i = 0; i < n; i++) {
        partPART *p = list[i];
        if (!(p->flags & 8)) {
            float k = -drag;
            m3dV old = p->pos;
            float gy = gravity.y, gz = gravity.z;
            p->vel.x += (k * p->vel.x + gravity.x) * dt;
            p->vel.y += (k * p->vel.y + gy) * dt;
            p->vel.z += (k * p->vel.z + gz) * dt;
            p->pos.x += dt * p->vel.x;
            p->pos.y += dt * p->vel.y;
            p->pos.z += dt * p->vel.z;
            if (cdtPlaneCount > 0 && (flags & 0x10000))
                PartApplyCDT(list[i], &old, dt);
        }
        if (parent && (flags & 0x40000)) {
            m3dV *v = &list[i]->vel;
            parent->SetPosDirSpeed(&list[i]->pos, v, v);
            parent->UpdateEmit(parent->UpdateGetNmbEmit(dt), dt);
        }
    }
}

// (Update, inline) a distance factor: 1 at d0, k at d1, linear between, held outside
static float partDegFar(float d, float d0, float d1, float k)
{
    float lo = d0, hi = d1, vlo = 1.0f, vhi = k;
    if (hi < lo) {
        hi = d0;
        lo = d1;
        vlo = k;
        vhi = 1.0f;
    }
    if (d < lo)
        return vlo;
    if (d > hi)
        return vhi;
    if (fabsf(hi - lo) < partEps)
        return (vhi + vlo) * 0.5f;
    return ((d - lo) * (vhi - vlo)) / (hi - lo) + vlo;
}

// v12 0x004D3730  (feb 0x004C2120)  slot +0x0C
// The distance factors (from the emitter, or its area's first vertex, to the camera), the child emitter's and this
// one's particles, the new ones while emitting outside the pause (state 8 emitted this frame); the area's speed
// (0x200000); this frame's pos / dir / speed kept for the next. 1 while particles live.
int partEMITTER_PHYS::Update(camCAMERA *cam, float dt)
{
    m3dV e;
    cam->GetOrigin(&e);
    m3dV *at = (nArea > 0 && (flags & 0x2000)) ? (m3dV *)area : &pos;
    float dx = e.x - at->x, dy = e.y - at->y, dz = e.z - at->z;
    float d = sqrtf(dx * dx + dy * dy + dz * dz);
    degFarK[0] = partDegFar(d, degFar[0], degFar[1], degFar[2]);
    degFarK[1] = partDegFar(d, degFar[0], degFar[1], degFar[3]);
    if (parent) {
        parent->UpdateUpdate(dt);
        parent->UpdateErase();
    }
    UpdateUpdate(dt);
    UpdateErase();
    u32 s;
    if ((state & 1) && !(state & 2)) {
        UpdateEmit(UpdateGetNmbEmit(dt), dt);
        s = state | 8;
    } else {
        s = state & ~8u;
    }
    state = s;
    state = s & ~4u;
    if (flags & 0x200000) {
        speed.x = pos.x - pos2.x;
        speed.y = pos.y - pos2.y;
        speed.z = pos.z - pos2.z;
        float r = 1.0f / dt;
        speed.x *= r;
        speed.y *= r;
        speed.z *= r;
    }
    pos2 = pos;
    dir2 = dir;
    speed2 = speed;
    return nPart > 0;
}

// v12 0x004D3CA0  (feb 0x004C2690)  slot +0x2C
// A particle that crossed a plane since old: one more bounce; mirrored back with the normal speed times cdtBounce
// and the tangential times cdtFriction (or at rest after cdtMaxHit bounces), kept cdtDist over the plane -- unless
// 0x2000000 (passes through). Killed with 0x20000; the child emitter bursts there with 0x80000; slot 0x14 PartHit.
int partEMITTER_PHYS::PartApplyCDT(partPART *p, m3dV *old, float t)
{
    int hit = 0;
    for (int i = 0; i < cdtPlaneCount; i++) {
        m3dV *P = (m3dV *)cdtPlanes[i];
        m3dV *N = (m3dV *)(cdtPlanes[i] + 0xC);
        float a = (old->z - P->z) * N->z + (old->y - P->y) * N->y + (old->x - P->x) * N->x;
        float b = (p->pos.z - P->z) * N->z + (p->pos.y - P->y) * N->y + (p->pos.x - P->x) * N->x;
        if (a * b > 0.0f)
            continue;
        uint8_t h = ++p->nHit;
        if (!(flags & 0x2000000)) {
            if (cdtMaxHit == -1 || (int)h < cdtMaxHit) {
                m3dMirrorPointPlane(P, N, &p->pos, &p->pos);
                m3dMirrorPointPlane(&m3dVZero, N, &p->vel, &p->vel);
                float vn = N->x * p->vel.x + p->vel.y * N->y + p->vel.z * N->z;
                p->vel.x = cdtBounce * vn * N->x + (p->vel.x - vn * N->x) * cdtFriction;
                p->vel.y = vn * N->y * cdtBounce + (p->vel.y - vn * N->y) * cdtFriction;
                p->vel.z = vn * N->z * cdtBounce + (p->vel.z - vn * N->z) * cdtFriction;
            } else {
                p->flags |= 8;
            }
            float dd = (p->pos.z - P->z) * N->z + (p->pos.y - P->y) * N->y + (p->pos.x - P->x) * N->x;
            if (dd < cdtDist) {
                dd = cdtDist - dd;
                p->pos.x += dd * N->x;
                p->pos.y += dd * N->y;
                p->pos.z += dd * N->z;
            }
        }
        if (flags & 0x20000)
            KillParticle(p);
        if (parent && (flags & 0x80000)) {
            parent->SetPosDirSpeed(&p->pos, &p->vel, &p->vel);
            parent->UpdateEmit(parentArg, t);
        }
        PartHit(p, t);
        hit = 1;
    }
    return hit;
}

// v12 0x004D3F20  (feb 0x004C2920)
// When (the later root) and where a particle shot straight out (speedA along dir plus the speed) falls through the
// first CDT plane's height under gravity.
int partEMITTER_PHYS::PredictCDT(m3dV *hit, float *t)
{
    m3dV v;
    v.x = speedA * dir.x + speed.x;
    v.y = speedA * dir.y + speed.y;
    v.z = speedA * dir.z + speed.z;
    float t0, t1;
    if (!m3dSolveQuadraticEq(gravity.y * 0.5f, m3dVUnitY.x * v.x + m3dVUnitY.y * v.y + m3dVUnitY.z * v.z,
                             pos.y - ((m3dV *)cdtPlanes[0])->y, &t0, &t1))
        return 0;
    if (t1 < t0)
        t1 = t0;
    if (t)
        *t = t1;
    if (!hit)
        return 1;
    *hit = pos;
    hit->x += v.x * t1;
    hit->y += v.y * t1;
    hit->z += v.z * t1;
    float g = t1 * t1 * 0.5f;
    hit->x += g * gravity.x;
    hit->y += g * gravity.y;
    hit->z += g * gravity.z;
    return 1;
}

// v12 0x004D41C0  (feb 0x004C51E0)  (Partrend.obj)
// Every emitter's Update (one that just started always; else not the paused (0x8000000) or attached (0x800) ones);
// an empty, stopped self-deleting (0x200) one is marked (state 0x20) and deleted, its child freed and every link
// to it cleared.
void __fastcall partUpdate(camCAMERA *cam, float dt)
{
    for (int i = 0; i < PART_MAX_EMITTERS; i++) {
        partEMITTER_PHYS *e = _partEmitList[i];
        if (!e)
            continue;
        if ((e->state & 0x40) || (!(e->flags & 0x8000000) && !(e->flags & 0x800)))
            e->Update(cam, dt);
        _partEmitList[i]->state &= ~0x40u;
        e = _partEmitList[i];
        if (!e->nPart && !(e->state & 1) && (e->flags & 0x200))
            e->state |= 0x20;
        if (!(_partEmitList[i]->state & 0x20))
            continue;
        if (partEMITTER_PHYS *c = _partEmitList[i]->parent)
            c->flags &= ~0x800u;
        for (int j = 0; j < PART_MAX_EMITTERS; j++)
            if (_partEmitList[j] && _partEmitList[j]->parent == _partEmitList[i])
                _partEmitList[j]->parent = 0;
        if (_partEmitList[i])
            _partEmitList[i]->__vecDelDtor(1);
    }
}

// v12 0x004D42A0  (feb 0x004C52C0)  (Partrend.obj)
// The self-deleting emitters (0x200) go.
void __fastcall partDestroyEmitters()
{
    for (int i = 0; i < PART_MAX_EMITTERS; i++) {
        partEMITTER_PHYS *e = _partEmitList[i];
        if (e && (e->flags & 0x200))
            e->__vecDelDtor(1);
    }
}

// v12 0x004D9B70  (feb 0x004CABB0, Partrend.obj)
void __fastcall partAttachInst(partEMITTER_PHYS *e, animINST *inst)
{
    e->attachInst = inst;
}

// v12 0x004D9B80  (feb 0x004CABC0)
// An instance going away: the emitters following it no longer do.
void __fastcall partDetachInst(animINST *inst)
{
    for (int i = 0; i < PART_MAX_EMITTERS; i++)
        if (_partEmitList[i] && _partEmitList[i]->attachInst == inst)
            _partEmitList[i]->attachInst = 0;
}

// One corner of a face of o, in the world (an object with the state bit 1: as it is)
static void partFaceVert(objOBJ *o, int face, int corner, m3dV *out)
{
    objGEOM *g = o->geom;
    u16 idx = *(u16 *)((u8 *)g->faceList + g->faceSize * face + 2 + corner * 2);
    float *v = (float *)((u8 *)g->vert + idx * 12);
    if (o->state & 1) {
        out->x = v[0];
        out->y = v[1];
        out->z = v[2];
        return;
    }
    float *m = o->ltm;
    out->x = v[0] * m[0] + v[1] * m[4] + v[2] * m[8] + m[12];
    out->y = v[0] * m[1] + v[1] * m[5] + v[2] * m[9] + m[13];
    out->z = v[0] * m[2] + v[1] * m[6] + v[2] * m[10] + m[14];
}

// v12 0x004D9C20  (feb 0x004CAC60)
// The emitter sends from the faces of n objects: at the position of the last (its instance's, else its centre), from
// count triangles (0 or too many: all, in order; fewer: picked at random, seeded by count), less sub if given. Not for
// a hidden owner, or an emitter with flag 0x8000000 before it has state 0x40. The first time their last position is
// themselves.
int __fastcall partSetAreaEmissionObjList(partEMITTER_PHYS *e, int n, objOBJ **objs, int count, m3dV *sub)
{
    if (!n)
        return 1;
    int had = e->state & 0x100;
    e->state &= ~0x100u;
    if (e->attachInst && (e->attachInst->flags & 3))
        return 1;
    if ((e->flags & 0x8000000) && !(e->state & 0x40))
        return 1;
    int total = 0;
    m3dV pos;
    for (int i = 0; i < n; i++) {
        objOBJ *o = objs[i];
        if (o->inst) {
            o->inst->Validate(0x204);
            o->inst->GetPos(&pos);
        } else {
            o->GetCenter(&pos);
        }
        total += o->nFace;
    }
    e->SetPosDirSpeed(&pos, 0, 0);
    if (count <= 0 || count >= total)
        count = total;
    m3dRND_GEN rnd;
    rnd.seed = count;
    for (int i = 0; i < count; i++) {
        int f = count < total ? rnd.RndIntMax(total - 1) : i;
        int k = 0, base = 0;
        for (; k < n; k++) {
            if (f >= base && f < base + objs[k]->nFace)
                break;
            base += objs[k]->nFace;
        }
        m3dV t[3];
        for (int c = 0; c < 3; c++) {
            partFaceVert(objs[k], f - base, c, &t[c]);
            if (sub) {
                t[c].x -= sub->x;
                t[c].y -= sub->y;
                t[c].z -= sub->z;
            }
        }
        if (!e->SetAreaEmission(count, i, (m3dTRI *)t, !had))
            return 0;
    }
    e->state |= 0x100;
    return 1;
}

// v12 0x004D9BF0  (feb 0x004CAC30)
int __fastcall partSetAreaEmissionObj(partEMITTER_PHYS *e, objOBJ *o, int count, m3dV *sub)
{
    if (!o)
        return 1;
    return partSetAreaEmissionObjList(e, 1, &o, count, sub);
}
