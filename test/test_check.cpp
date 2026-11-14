// test_check.cpp -- (test) --checklevel: load a level with the rebuilt scene reader and check every mesh the way
// the renderer and collision will read it. Used by the level editor before a playtest: a level the game would
// draw wrong or crash on is reported instead of started.
//
// Checks per object (scene and templates): vertex indices < nVert; the face's split (material) < number of splits;
// per material layer, its texture coordinate indices < the coordinate set it uses; split face ranges inside the
// face list; material layers with a texture. --obj a,b,c prints those objects' materials (texture and faces).

#include "../src/engine/engine.h"
#include "../include/engine_render/texture.h"
#include "test_assets.h"
#include <stdio.h>
#include <string.h>

static int s_err, s_obj, s_faces, s_shown;
static const char *s_want;

static void report(objOBJ *o, const char *what, int a, int b)
{
    s_err++;
    if (s_shown++ < 40)
        printf("ERROR %s: %s (%d, limit %d)\n", o->name ? o->name : "?", what, a, b);
}

static int wanted(const char *name)
{
    if (!s_want || !name)
        return 0;
    const char *p = s_want;
    size_t n = strlen(name);
    while (*p) {
        const char *e = strchr(p, ',');
        size_t k = e ? (size_t)(e - p) : strlen(p);
        if (k == n && !strnicmp(p, name, n))
            return 1;
        if (!e)
            break;
        p = e + 1;
    }
    return 0;
}

static void checkObj(objOBJ *o, int depth)
{
    s_obj++;
    if (s_want && !strcmp(s_want, "*"))         // --obj "*": every object (comparing two saves)
        printf("OBJ %d %s %d %d\n", depth, o->name ? o->name : "?", o->nVert, o->nFace);
    objGEOM *g = o->geom;
    if (g && o->nFace > 0 && g->faceList) {
        int nSets = (g->faceSize - 8) / 6;
        objSPLIT *sp = g->split;
        int nSplit = sp ? sp->n : 0;
        for (int f = 0; f < o->nFace; f++) {
            objFACE *fc = (objFACE *)((char *)g->faceList + f * g->faceSize);
            s_faces++;
            for (int k = 0; k < 3; k++)
                if (fc->vert[k] >= o->nVert)
                    report(o, "face vertex index out of range", fc->vert[k], o->nVert);
            if (!sp)
                continue;
            if (fc->split >= nSplit) {
                report(o, "face material (split) out of range", fc->split, nSplit);
                continue;
            }
            objSPLIT_ENTRY *e = &sp->entry[fc->split];
            for (int m = 0; m < e->nMtl && m < 4; m++) {
                if (!e->mtl[m].tex)                     // untextured (domains, helper volumes): never drawn
                    continue;
                int c = e->mtl[m].ctrl;
                if (c < 0 || c >= sp->ctrl.n) {
                    report(o, "material control out of range", c, sp->ctrl.n);
                    continue;
                }
                objMTL_CTRL *mc = &sp->ctrl.c[c];
                if (mc->tcInd >= nSets) {
                    report(o, "material uses a texture index set the faces lack", mc->tcInd, nSets);
                    continue;
                }
                if (mc->tcCoord >= 3 || !g->texCoord[mc->tcCoord]) {
                    report(o, "material uses a missing texture coordinate set", mc->tcCoord, 3);
                    continue;
                }
                for (int k = 0; k < 3; k++) {
                    int t = fc->tc[mc->tcInd * 3 + k];
                    if (t >= g->nTexCoord[mc->tcCoord])
                        report(o, "texture coordinate index out of range", t, g->nTexCoord[mc->tcCoord]);
                }
            }
        }
        for (int i = 0; i < nSplit; i++) {
            objSPLIT_ENTRY *e = &sp->entry[i];
            if (e->firstFace + e->nFace > o->nFace)
                report(o, "material face range past the face list", e->firstFace + e->nFace, o->nFace);
        }
        if (wanted(o->name)) {
            printf("OBJECT %s: %d verts, %d faces, %d materials\n", o->name, o->nVert, o->nFace, nSplit);
            for (int i = 0; i < nSplit; i++) {
                objSPLIT_ENTRY *e = &sp->entry[i];
                printf("  material %d: faces %d..%d,", i, e->firstFace, e->firstFace + e->nFace - 1);
                for (int m = 0; m < e->nMtl && m < 4; m++)
                    printf(" [%s]", e->mtl[m].tex ? e->mtl[m].tex->name : "-");
                printf("\n");
            }
        }
    }
    objOBJ *first = o->child;
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        checkObj(c, depth + 1);
}

// The template count the file's 0x1EA list declares (u32 after the chunk header), found by walking the top-level
// chunks by their end offsets; -1 without a template list. The game's reader is sequential: an object whose
// sub-chunks it reads differently from their sizes runs on into the next top-level chunks, and the templates
// (doors, water, buttons) are the first thing lost.
static int fileTplCount(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    int n = -1;
    unsigned char h[10];
    long p = 0;
    while (fseek(f, p, SEEK_SET) == 0 && fread(h, 1, 10, f) >= 6) {
        unsigned id = h[0] | (h[1] << 8);
        long end = (long)(h[2] | (h[3] << 8) | (h[4] << 16) | ((unsigned)h[5] << 24));
        if (id == 0x1EA) {
            n = h[6] | (h[7] << 8) | (h[8] << 16) | (h[9] << 24);
            break;
        }
        if (end <= p)
            break;
        p = end;
    }
    fclose(f);
    return n;
}

int tstCheckLevel(const char *path, const char *objs)
{
    s_want = objs;
    int wantTpl = fileTplCount(path);
    scnSCENE *s = tstLoadScene(path);
    if (!s) {
        printf("CHECK FAILED: the scene reader rejected %s\n", path);
        return 2;
    }
    if (s->root)
        checkObj(s->root, 0);
    int nTpl = 0;
    for (int k = 0; k < 300; k++)
        if (s->tplPool[k].magic && s->tplPool[k].root) {
            nTpl++;
            checkObj(s->tplPool[k].root, 100);
        }
    if (wantTpl >= 0 && nTpl < wantTpl) {
        s_err++;
        printf("ERROR the game read %d of the %d templates: an object's chunks are not laid out the way the game "
               "reads them, and it read past the object tree\n", nTpl, wantTpl);
    }
    printf("%s: %d objects, %d faces checked, %d errors\n", path, s_obj, s_faces, s_err);
    printf(s_err ? "CHECK FAILED\n" : "CHECK OK\n");
    return s_err ? 3 : 0;
}
