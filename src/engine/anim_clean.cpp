// anim_clean.cpp -- engine.lib template animation clean-up after loading
// (Will Rock 1.2, v12.exe; feb obj engine:anim_clean.obj)
//
// v12 0x0059DB30..0x0059E4FF (beta decompile; 1.2 walks the skin bones at their new stride 0x54, spline +0x50).
// sioReadTemplate runs animCleanConstSpl, animCompressAnim, animCleanConstSpl:
//   - constant splines are replaced by their value (an object left with no position / rotation / scale
//     spline gets that pose baked into its local matrix); a constant 4th spline is dropped when it is 1,
//     else cut to one key;
//   - every spline not already keyed on whole frames is resampled at 1 frame steps over the sequences'
//     frame range, then frames that belong to no sequence are removed (the later keys, sequence bounds and
//     action frames shift down by one).

#include "engine.h"
#include "../m3d/m3d_spl.h"

static const float animEps = 1e-6f;         // v12 0x0060ABDC
static const float animBig = 2000000.0f;    // v12 0x0060ABE0

static inline void animDelSpl(m3dSPL **p)
{
    if (*p)
        (*p)->__vecDelDtor(1);
    *p = 0;
}
static inline BOOL32 animIsConst(m3dSPL *s) { return ((m3dSPL_EXT *)s)->IsConst(); }
static inline BOOL32 animIsInt(m3dSPL *s) { return ((m3dSPL_EXT *)s)->IsIntegerArg(); }
static inline objSKIN_BONE *animBone(objMOD_SKIN *sk, int i) { return &sk->bone[i]; }   // stride 0x54 (1.2)

// v12 0x0059DB30  (feb 0x00581800)
BOOL32 __fastcall animCleanConstSpl(animTPL *t)
{
    float tMin = 0, tMax;
    if (t->nSeq >= 1) {
        tMin = animBig;
        tMax = -animBig;
        for (int i = 0; i < t->nSeq; i++) {
            animSEQ *q = &t->seq[i];
            float lo = q->frameStart < q->frameEnd ? q->frameStart : q->frameEnd;
            float hi = q->frameEnd < q->frameStart ? q->frameStart : q->frameEnd;
            if (tMin < lo)
                lo = tMin;
            if (hi < tMax)
                hi = tMax;
            tMin = lo;
            tMax = hi;
        }
    }
    for (int i = 0; i < t->nObjAnim; i++) {
        animOBJ_ANIM *a = &t->objAnim[i];
        BOOL32 had = a->spl14 || a->spl28 || a->spl38;
        if (a->spl14 && animIsConst(a->spl14)) {
            a->spl14->GetValue(tMin, a->pos, 0);
            animDelSpl(&a->spl14);
        }
        if (a->spl28 && animIsConst(a->spl28)) {
            a->spl28->GetValue(tMin, a->rot, 0);
            animDelSpl(&a->spl28);
        }
        if (a->spl38 && animIsConst(a->spl38)) {
            a->spl38->GetValue(tMin, a->scale, 0);
            animDelSpl(&a->spl38);
        }
        if (a->spl3C && animIsConst(a->spl3C)) {
            float v;
            a->spl3C->GetValue(tMin, &v, 0);
            if (!(fabsf(1.0f - v) < animEps))
                a->spl3C->RemoveKp(1, a->spl3C->nKp - 1);
            else
                animDelSpl(&a->spl3C);
        }
        if (had && !a->spl14 && !a->spl28 && !a->spl38) {
            objOBJ *o = objFindAnimNmb(t->root, i);        // (not checked for 0 in the game)
            m3dQUAT q = *(m3dQUAT *)a->rot;
            q.Invert();
            _animCombineTRS(0xFFFFFFFF, (m3dV *)a->pos, &q, (m3dV *)a->scale, (m3dMATR *)o->local);
        }
    }
    for (int k = 0; k < t->nSkin; k++) {
        objMOD_SKIN *sk = t->skinList[k];
        for (int b = 0; b < sk->nBone; b++) {
            objSKIN_BONE *bn = animBone(sk, b);
            if (bn->spl && animIsConst(bn->spl)) {
                bn->spl->GetValue(tMin, &bn->weight, 0);
                animDelSpl(&bn->spl);
            }
        }
    }
    return 1;
}

// v12 0x0059DE60  (feb 0x00581B30)
BOOL32 __fastcall animOptimizeSpl(animTPL *t)
{
    for (int i = 0; i < t->nObjAnim; i++) {
        animOBJ_ANIM *a = &t->objAnim[i];
        if (a->spl14)
            a->spl14->Optimize(0x1C);
        if (a->spl28)
            a->spl28->Optimize(0x1C);
        if (a->spl38)
            a->spl38->Optimize(0x1C);
        if (a->spl3C)
            a->spl3C->Optimize(0x1C);
    }
    for (int k = 0; k < t->nSkin; k++) {
        objMOD_SKIN *sk = t->skinList[k];
        for (int b = 0; b < sk->nBone; b++)
            if (animBone(sk, b)->spl)
                animBone(sk, b)->spl->Optimize(0x1C);
    }
    return 1;
}

// Resample a spline at whole frames unless it already is (type: the new spline type)
static BOOL32 animResample(m3dSPL **p, float t0, float t1, int type, int *maxKp)
{
    if (*p && !animIsInt(*p)) {
        m3dSPL *n = (*p)->Sample(t0, t1, 1.0f, type);
        if (!n)
            return 0;
        animIsInt(n);
        animDelSpl(p);
        *p = n;
    }
    if (*p && *maxKp <= (*p)->nKp)
        *maxKp = (*p)->nKp;
    return 1;
}

// Remove key k (when there are at least two) and move the later keys one frame back
static void animDropFrame(m3dSPL *s, int k)
{
    if (!s || k >= s->nKp || s->nKp <= 1)
        return;
    s->RemoveKp(k, 1);
    for (int j = k; j < s->nKp; j++) {
        int i = j < s->nKpMax ? j : s->nKpMax;
        float *kp = (float *)((u8 *)s->kp + s->kpStride * i);
        *kp -= 1.0f;
    }
}

// v12 0x0059DF30  (feb 0x00581C00)
BOOL32 __fastcall animCompressAnim(animTPL *t)
{
    float tMin, tMax;
    if (t->nSeq < 1) {
        tMax = 0;
        tMin = 0;
    } else {
        tMax = -animBig;
        tMin = animBig;
        for (int i = 0; i < t->nSeq; i++) {
            animSEQ *q = &t->seq[i];
            float lo = q->frameStart < q->frameEnd ? q->frameStart : q->frameEnd;
            if (lo <= tMin)
                tMin = lo;
            float hi = q->frameEnd < q->frameStart ? q->frameStart : q->frameEnd;
            if (hi < tMax)
                hi = tMax;
            tMax = hi;
        }
        if (0.0f < tMin)
            tMin = 0;
    }
    int maxKp = 0;
    for (int i = 0; i < t->nObjAnim; i++) {
        animOBJ_ANIM *a = &t->objAnim[i];
        if (!animResample(&a->spl14, tMin, tMax, 2, &maxKp)
            || !animResample(&a->spl28, tMin, tMax, 7, &maxKp)
            || !animResample(&a->spl38, tMin, tMax, 2, &maxKp)
            || !animResample(&a->spl3C, tMin, tMax, 0, &maxKp))
            return 0;
    }
    for (int k = 0; k < t->nSkin; k++) {
        objMOD_SKIN *sk = t->skinList[k];
        for (int b = 0; b < sk->nBone; b++)
            if (!animResample(&animBone(sk, b)->spl, tMin, tMax, 0, &maxKp))
                return 0;
    }
    for (int f = 0; f < maxKp; f++) {
        float ff = (float)f;
        int s = 0;
        for (; s < t->nSeq; s++) {                      // a frame of some sequence: kept
            animSEQ *q = &t->seq[s];
            float lo = q->frameStart < q->frameEnd ? q->frameStart : q->frameEnd;
            float hi = q->frameEnd < q->frameStart ? q->frameStart : q->frameEnd;
            if (lo - animEps <= ff && ff <= hi + animEps)
                break;
        }
        if (s < t->nSeq)
            continue;
        for (int i = 0; i < t->nObjAnim; i++) {
            objFindAnimNmb(t->root, i);                 // (result unused in the game)
            animOBJ_ANIM *a = &t->objAnim[i];
            animDropFrame(a->spl14, f);
            animDropFrame(a->spl28, f);
            animDropFrame(a->spl38, f);
            animDropFrame(a->spl3C, f);
        }
        for (int k = 0; k < t->nSkin; k++) {
            objMOD_SKIN *sk = t->skinList[k];
            for (int b = 0; b < sk->nBone; b++)
                animDropFrame(animBone(sk, b)->spl, f);
        }
        for (int i = 0; i < t->nSeq; i++) {
            animSEQ *q = &t->seq[i];
            if (ff < q->frameStart)
                q->frameStart -= 1.0f;
            if (ff < q->frameEnd)
                q->frameEnd -= 1.0f;
            if (ff < q->unk48)
                q->unk48 -= 1.0f;
            for (int j = 0; j < 10; j++)
                if (q->actionFrame[j] && f < q->actionFrame[j])
                    q->actionFrame[j]--;
        }
        maxKp--;
        f--;
    }
    return 1;
}
