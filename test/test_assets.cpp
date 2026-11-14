// test_assets.cpp -- the box test's asset side: a texture driver for the rebuilt texture manager
// (in place of drv:d3d_txm.obj, not rebuilt), template loading through the rebuilt readers
// (sioReadTemplate -> sceneio_*.cpp, txm.cpp, picture.cpp), and a dump of a loaded object tree.

#include "../src/engine/engine.h"
#include "../include/engine_render/texture.h"
#include "plat_d3d7.h"
#include "test_assets.h"
#include "../src/m3d/m3d_spl.h"
#include "../include/engine_core/anim.h"   // _animCombineTRS (engine/animkinem.cpp v12 0x00501AF0)
#include <math.h>

int tstHaveDevice;                    // set once the D3D device exists: textures are uploaded then

// The driver side of a texture: an ARGB8888 copy of mip level 0 in a managed D3D surface, kept in drvData
// (+0x74, where the D3D driver keeps its own). For plain and layered textures (lightmaps are layered: the
// .lg texture entry 0x156 with a 0x158 layer name, e.g. ui_0_0 -> ui_0_0_0.pct).
template <class B> class tstDRV : public B {
public:
    unsigned *pix;
    tstDRV(char *n, int f) : B(n, f), pix(0) {}
    void *__vecDelDtor(unsigned f)
    {
        UnloadDrv();
        return B::__vecDelDtor(f);
    }
    int RequestSize(int *pw, int *ph) { return 0; }     // any size, as the file has it
    int CreateDrv(pctHEADER *hd)
    {
        pix = (unsigned *)apCalloc(hd->w * hd->h, 4);
        if (!pix)
            return 0;
        if (tstHaveDevice)
            this->drvData = rndTexCreate(hd->w, hd->h);
        return 1;
    }
    int LoadDrv(pctPICTURE *p)
    {
        if (!pix)
            return 0;
        if (pctFormat[p->fmt].flags & PCT_FMT_DXT)
            pctCopyViaUnpackDXTBuf(p, this->w, this->h, 4, 0, 0, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, (u8 *)pix);
        else
            pctCopyBuf(p, this->w, this->h, 4, 0, 0, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, (u8 *)pix);
        if (!pctIsAlpha(p->fmt))                        // no alpha channel: opaque
            for (int i = 0; i < this->w * this->h; i++)
                pix[i] |= 0xFF000000;
        if (this->drvData)
            rndTexUpload(this->drvData, pix, this->w, this->h);
        return 1;
    }
    // the driver's read-back (txmD3D_TEX(_LAY)::GetColor in the game reads its device copy): the test's pixel copy.
    // GetColorUV asks with layer -1, which the layered base class ignores.
    int GetColor(int x, int y, pctPIXEL *c, int layer)
    {
        if (pix && x < this->w && y < this->h) {
            unsigned v = pix[y * this->w + x];
            c->a = v >> 24; c->r = v >> 16; c->g = v >> 8; c->b = v;
        }
        return 1;
    }
    void UnloadDrv()
    {
        rndTexFree(this->drvData);
        this->drvData = 0;
        apFree(pix);
        pix = 0;
    }
};

class tstTEX : public tstDRV<txmTEXTURE> {
public:
    tstTEX(char *n, int f) : tstDRV<txmTEXTURE>(n, f) {}
    int SetColor(int x, int y, pctPIXEL *c, int layer)  // the built-in 1x1 textures (Fill)
    {
        if (pix && x < w && y < h) {
            pix[y * w + x] = ((unsigned)c->a << 24) | (c->r << 16) | (c->g << 8) | c->b;
            if (drvData)
                rndTexUpload(drvData, pix, w, h);
        }
        return 1;
    }
    int GetColor(int x, int y, pctPIXEL *c, int layer)
    {
        if (pix && x < w && y < h) {
            unsigned v = pix[y * w + x];
            c->a = v >> 24; c->r = v >> 16; c->g = v >> 8; c->b = v;
        }
        return 1;
    }
};
typedef tstDRV<txmTEXTURE_LAY> tstTEX_LAY;

class tstTXM_MANAGER : public txmMANAGER {
public:
    txmTEXTURE *MakeTex(char *n, int f)
    {
        void *mem = operator new(sizeof(tstTEX));
        return mem ? new (mem) tstTEX(n, f) : 0;
    }
    txmTEXTURE *MakeTexLayer(char *n, int f)
    {
        void *mem = operator new(sizeof(tstTEX_LAY));
        return mem ? new (mem) tstTEX_LAY(n, f) : 0;
    }
};

// PLACEHOLDER (test renderer) for the driver's on-demand load: a texture registered plain after the level's LoadAll
// (txmManager->Add(name, 0, TXM_ADD_PLAIN): the boss bar's portrait, the comics' pictures) is read and uploaded the
// first time it is drawn -- once per texture, so a missing file is not retried every frame
void *tstTexSurface(txmTEXTURE *t)
{
    if (t && !t->drvData && txmManager) {
        static txmTEXTURE *tried[512];
        static int nTried;
        int i = 0;
        while (i < nTried && tried[i] != t)
            i++;
        if (i == nTried && nTried < 512) {
            tried[nTried++] = t;
            txmManager->LoadAll(0);
        }
    }
    return t ? t->drvData : 0;
}

int tstTexInit(const char *gameDir)
{
    static char texDir[0x104];
    _snprintf(texDir, sizeof(texDir) - 1, "%s\\textures", gameDir);
    pctInit();
    txmManager = new tstTXM_MANAGER;
    return txmManager->Init(texDir, (char *)"pct");
}

animTPL *tstLoadTemplate(const char *path)
{
    fioFILE_DISK f;
    if (!f.Init((char *)path, FIO_READ | FIO_CHUNKS)) {
        printf("cannot open %s\n", path);
        return 0;
    }
    animTPL *t = (animTPL *)apCalloc(1, sizeof(animTPL));
    sioSetSourceFile(&f);
    int ok = sioReadTemplate(t, 1);
    f.Term();
    if (!ok) {
        printf("sioReadTemplate failed: %s\n", path);
        return 0;
    }
    int nFail = txmManager->LoadAll(0);
    printf("template %s (%s): %d sequences, %d object anims, %d selections, %d textures not loaded\n",
           t->tplName, t->className, t->nSeq, t->nObjAnim, t->nSel, nFail);
    return t;
}

objOBJ *tstFindObj(objOBJ *o, const char *name)
{
    if (!o)
        return 0;
    if (o->name && !stricmp(o->name, name))
        return o;
    objOBJ *c = o->child;
    if (c)
        do {
            objOBJ *r = tstFindObj(c, name);
            if (r)
                return r;
            c = c->next;
        } while (c != o->child);
    return 0;
}

static void dumpProps(psSHEET *ps, int depth)
{
    if (!ps)
        return;
    for (int s = 0; s < ps->nSect; s++) {
        printf("%*s  [%s]", depth * 2, "", ps->sect[s].name);
        for (int v = 0; v < ps->sect[s].nVar; v++)
            printf(" %s=%s", ps->sect[s].var[v].name, ps->sect[s].var[v].value ? ps->sect[s].var[v].value : "");
        printf("\n");
    }
}

void tstDumpObj(objOBJ *o, int depth, int maxDepth)
{
    if (!o || depth > maxDepth)
        return;
    float *m = o->local;
    printf("%*s%s  id %d  v %d f %d  pos %.3f %.3f %.3f  scale %.3f %.3f", depth * 2, "", o->name ? o->name : "?",
           o->idHier, o->nVert, o->nFace, m[12], m[13], m[14],
           sqrtf(m[0] * m[0] + m[1] * m[1] + m[2] * m[2]), sqrtf(m[4] * m[4] + m[5] * m[5] + m[6] * m[6]));
    if (o->nFace > 0 && o->geom && o->geom->split) {
        txmTEXTURE *tx = o->GetFaceTexMtlNmb(0, 0);
        printf("  tex %s", tx ? tx->name : "-");
        objSPLIT *sp = o->geom->split;
        printf("  [sets %d %d %d, ctrl %d:", o->geom->nTexCoord[0], o->geom->nTexCoord[1], o->geom->nTexCoord[2], sp->ctrl.n);
        for (int c = 0; c < sp->ctrl.n && c < 4; c++)
            printf(" {ind %d coord %d rend %d hw %d}", sp->ctrl.c[c].tcInd, sp->ctrl.c[c].tcCoord, sp->ctrl.c[c].rendMtl,
                   sp->ctrl.c[c].tcHWInd);
        printf("]");
        for (int e = 0; e < sp->n && e < 4; e++) {
            objSPLIT_ENTRY *en = &sp->entry[e];
            printf("  e%d:", e);
            for (int k = 0; k < en->nMtl && k < 4; k++)
                printf(" %s/c%d", en->mtl[k].tex ? en->mtl[k].tex->name : "-", en->mtl[k].ctrl);
        }
    }
    printf("\n");
    dumpProps(o->props, depth);
    objOBJ *c = o->child;
    if (c)
        do {
            tstDumpObj(c, depth + 1, maxDepth);
            c = c->next;
        } while (c != o->child);
}

// ---------------------------------------------------------------------------------------------
// scenes (.lg) through the rebuilt sioReadScene
scnSCENE *tstLoadScene(const char *path)
{
    fioFILE_DISK f;
    if (!f.Init((char *)path, FIO_READ | FIO_CHUNKS)) {
        printf("cannot open %s\n", path);
        return 0;
    }
    scnSCENE *s = new scnSCENE;
    sioSetSourceFile(&f);
    int ok = sioReadScene(s);
    f.Term();
    int nFail = txmManager->LoadAll(0);
    printf("scene %s: read %d, %d textures not loaded\n", path, ok, nFail);
    return ok ? s : 0;
}

// PLACEHOLDER (animINST playback is not rebuilt): pose an animated template at the start of its time range, the
// way its instance shows before any sequence runs. Same rule as animCleanConstSpl (v12 0x0059DB30) applies to the
// constant ones: the value is the constant, or the spline at tMin; local = _animCombineTRS(pos, inverse rot, scale).
// Without it a door node keeps its rest matrix (the editor's pose_nodes does the same at frame 0).
void tstPoseTemplate(animTPL *t)
{
    if (!t || !t->root || !t->objAnim)
        return;
    float tMin = 0;
    if (t->nSeq >= 1) {
        tMin = 2000000.0f;
        for (int i = 0; i < t->nSeq; i++) {
            float lo = t->seq[i].frameStart < t->seq[i].frameEnd ? t->seq[i].frameStart : t->seq[i].frameEnd;
            if (lo < tMin)
                tMin = lo;
        }
    }
    for (int i = 0; i < t->nObjAnim; i++) {
        animOBJ_ANIM *a = &t->objAnim[i];
        if (!a->spl14 && !a->spl28 && !a->spl38)
            continue;                                   // constant: animCleanConstSpl already set the local matrix
        float pos[3] = { a->pos[0], a->pos[1], a->pos[2] };
        float rot[4] = { a->rot[0], a->rot[1], a->rot[2], a->rot[3] };
        float scl[3] = { a->scale[0], a->scale[1], a->scale[2] };
        if (a->spl14)
            a->spl14->GetValue(tMin, pos, 0);
        if (a->spl28)
            a->spl28->GetValue(tMin, rot, 0);
        if (a->spl38)
            a->spl38->GetValue(tMin, scl, 0);
        objOBJ *o = objFindAnimNmb(t->root, i);
        if (!o)
            continue;
        m3dQUAT q = *(m3dQUAT *)rot;
        q.Invert();
        _animCombineTRS(0xFFFFFFFF, (m3dV *)pos, &q, (m3dV *)scl, (m3dMATR *)o->local);
    }
    objCalcLTM(t->root);
}

void tstDumpScene(scnSCENE *s, int depth)
{
    int nt = 0, ni = 0;
    for (int k = 0; k < 300; k++) {
        animTPL *t = &s->tplPool[k];
        if (t->magic && t->root) {
            printf("tpl %d: %s (%s) seq %d\n", k, t->tplName, t->className, t->nSeq);
            nt++;
            tstDumpObj(t->root, 1, depth + 2);
        }
    }
    for (int k = 0; k < 1000; k++) {
        animINST *a = s->InstPtr(k);
        if (a->magic == 0x41494E53 && a->tplName) {
            printf("inst %d: %s tpl %s class %s pos %.2f %.2f %.2f\n", k, a->name ? a->name : "?", a->tplName,
                   a->className ? a->className : "-", a->matr[12], a->matr[13], a->matr[14]);
            ni++;
        }
    }
    printf("%d templates, %d instances, %d lights, camera %p\n", nt, ni, s->nLights, (void *)s->camera);
    if (s->camera) {
        float *m = (float *)s->camera;
        (void)m;
    }
    for (int i = 0; i < s->nLights; i++) {
        scnLIGHT *l = &s->lights[i];
        printf("light %d %s type %d flags %X pos %.1f %.1f %.1f dir %.2f %.2f %.2f col %.2f %.2f %.2f  98 %.2f 9c %.2f a0 %.2f a4 %.2f a8 %.2f\n",
               i, l->name, l->type, l->flags, l->matr.m[3][0], l->matr.m[3][1], l->matr.m[3][2], l->matr.m[2][0], l->matr.m[2][1],
               l->matr.m[2][2], l->color[0], l->color[1], l->color[2], l->_98, l->_9c, l->_a0, l->_a4, l->_a8);
    }
    tstDumpObj(s->root, 0, depth);
}
