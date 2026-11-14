// test_menu.cpp -- the box test's front end: the Will Rock 1.2 main menu rebuilt from the game's own data,
// plus a DEBUG menu (test only).
//
// What is the game's: the menu layout and every button (database\ui.tpl, frames "main", "single_player",
// "options", ... with their [UI_PROP] frameID / code links), the 3D backdrop (scenes\ui.lg: the "Base"
// template with the wall, the torches, the parchment and the logos, and the "Actor_cherep" skull), the
// strings (texts\strings.str through the rebuilt gsSTRINGS), the fonts (database\font_s/b/h.tpl through the
// rebuilt uiFONT), the page layout and text placement (rebuilt uiGetObjRenderRect / uiRENDER_ELEM / uiFONT),
// the wr_btn text colours (wrUI_BUTTON::GetColorText v12 0x0048B630: normal 0x9C000000, pressed / rollover
// 0xFF000000, disabled 0x3C000000) and its font (font 1, font_b, v12 0x0048D680).
// What is the test's: the element tree walk (in place of uiElementCreate and the 60 wrUI_* classes), the
// renderer calls (rendDRIVER::RenderPolyCompat / RenderObj, rendCFG::InitMtl1 below, for drv:D3d_*.obj),
// the skull's frame animation (wrUI_CHEREP is not rebuilt), no torch flames (sfx_flame particles not rebuilt),
// no sounds, and the DEBUG entry under QUIT with its tools.

#include "../src/engine/engine.h"
#include "../include/engine_render/texture.h"
#include "../include/engine_render/shader.h"
#include "../include/engine_render/rend_driver.h"
#include "../include/game_systems/strings.h"
#include "../include/frontend/ui_core.h"
#include "../include/frontend/wr_ui.h"
#include "../include/player/player.h"
#include "plat_d3d7.h"
#include "test_assets.h"
#include "test_menu.h"
#include "test_ents.h"
#include <windows.h>
#include <commdlg.h>
#include <dinput.h>
#include <math.h>
#include <wchar.h>

extern char uiFontNames[3][0x40];

static char s_game[0x104];
static float s_time;

// ---------------------------------------------------------------------------------------------
// renderer stand-ins (drv:D3d_*.obj is not rebuilt): what the rebuilt UI code calls
void rendCFG::InitMtl1(txmTEXTURE *t)
{
    *(txmTEXTURE **)_00 = t;
}

// the screen-pixel variant (the crosshair, gscCURSOR::Draw): the same quads
int rendDRIVER::_RenderPolyCompat(camCAMERA *cam, int, int, int, int, int n, rendVERTEX *v, rendCFG *c)
{
    return RenderPolyCompat(cam, n, v, c);
}

int rendDRIVER::RenderPolyCompat(camCAMERA *cam, int n, rendVERTEX *v, rendCFG *c)
{
    platVERT2D q[3 * 16];
    if (n < 3 || n > 16)
        return 0;
    txmTEXTURE *ct = *(txmTEXTURE **)c->_00;
    void *tex = ct && ct->name && !_stricmp(ct->name, "white") ? 0 : tstTexSurface(ct);   // (the engine's "white" has no picture here: plain colour)
    // (the engine's "black" too: its RGB565 pixel was filled with alpha 0, which the test's surface keeps -- plain black,
    //  the element's alpha kept: the statistics' backdrop)
    int black = ct && ct->name && !_stricmp(ct->name, "black");
    if (black)
        tex = 0;
    int k = 0;
    for (int i = 1; i + 1 < n; i++) {
        const rendVERTEX *t[3] = { &v[0], &v[i], &v[i + 1] };
        for (int j = 0; j < 3; j++, k++) {
            q[k].x = t[j]->x - 0.5f;
            q[k].y = t[j]->y - 0.5f;
            q[k].z = 0;
            q[k].rhw = 1;
            q[k].color = black ? c->color & 0xFF000000 : c->color;
            q[k].u = t[j]->u;
            q[k].v = t[j]->v;
        }
    }
    rndTris2D(tex, q, n - 2);
    return 1;
}

// UI objects drawn "3D" (obj3D=yes, e.g. the window borders): the UI camera is the page, so x, -y
extern int tstInMirror;
int rendDRIVER::RenderObj(objOBJ *o, int, int *)
{
    if (tstInMirror)                                    // (the mirror's object: the test only for it, _RenderObj draws)
        return 0;
    if (!o->nFace || !o->geom || !o->geom->split)
        return 0;
    m2dV sc = ((uiUI_SYSTEM *)uiSystem)->scale;
    unsigned col = (unsigned)o->geom->unk62;
    if (!col)
        col = 0xFFFFFFFF;
    for (int f = 0; f < o->nFace; f++) {
        objFACE *fc = o->GetFace(f);
        platVERT2D q[3];
        for (int j = 0; j < 3; j++) {
            m3dV *p = &o->geom->vert[fc->vert[j]];
            const float *m = o->ltm;
            float wx = p->x * m[0] + p->y * m[4] + p->z * m[8] + m[12];
            float wy = p->x * m[1] + p->y * m[5] + p->z * m[9] + m[13];
            m2dV uv = { 0, 0 };
            if (o->geom->texCoord[0])
                o->GetFaceTexCoord(f, j, 0, &uv);
            q[j].x = wx * sc.x;
            q[j].y = -wy * sc.y;
            q[j].z = 0;
            q[j].rhw = 1;
            q[j].color = col;
            q[j].u = uv.x;
            q[j].v = uv.y;
        }
        rndTris2D(tstTexSurface(o->GetFaceTexMtlNmb(f, 0)), q, 1);
    }
    return 1;
}

// ---------------------------------------------------------------------------------------------
// 3D object trees (ui.lg backdrop, level / template viewers)
static int s_blendPass = -1;                            // (test) -1 all, 0 opaque only, 1 blended only
int tstDrawLayer;                                       // 0 the world, 1 the first-person buffer (render state 0x1000000)
static int s_entColor;                                  // drawing an entity's model: its objects' constant colour
static float s_eye[3];                                  // the camera position (billboards)
static int   s_haveEye;
static int   s_additive;                                // draw additively (transparency type 0x40)
static platVERT3T2 *s_tv;
static int s_ntvMax;

static inline void xform(const float *m, const m3dV *p, float *o)
{
    o[0] = p->x * m[0] + p->y * m[4] + p->z * m[8] + m[12];
    o[1] = p->x * m[1] + p->y * m[5] + p->z * m[9] + m[13];
    o[2] = p->x * m[2] + p->y * m[6] + p->z * m[10] + m[14];
}
static inline void matMul(const float *a, const float *b, float *o)   // o = a * b (row vectors)
{
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            o[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
}

typedef void (*tstUVHOOK)(objOBJ *o, m2dV *uv);

// Static lighting is baked: a split entry's material whose control has rendMtl 1 is the lightmap (a layered
// texture, e.g. ui_0_0 = ui_0_0_0.pct), on its own coordinate set; it multiplies the base material (rendMtl 0).
// Objects named "&ln" have no lightmap layer and draw full bright.
static int lightmapLayer(objSPLIT *sp, objSPLIT_ENTRY *en)
{
    for (int k = 1; k < en->nMtl && k < 4; k++) {
        int c = en->mtl[k].ctrl;
        if (c >= 0 && c < sp->ctrl.n && sp->ctrl.c[c].rendMtl == 1)
            return k;
    }
    return -1;
}

// the coordinate set of material k exists (Box01 in ui.lg has only the lightmap's)
static int hasSet(objOBJ *o, objSPLIT *sp, objSPLIT_ENTRY *en, int k)
{
    if (k >= en->nMtl)
        return 0;
    int set = sp->ctrl.Get(en->mtl[k].ctrl)->tcCoord;
    return set < 3 && o->geom->texCoord[set] && o->geom->nTexCoord[set];
}

static int s_cull = 1;          // --nocull draws every face (the old test renderer)
static int s_cullFront = 2;     // D3DCULL_CW, as the game's driver
void tstSetCull(int on, int mode)
{
    s_cull = on;
    s_cullFront = mode;
}

// the render driver's tests before drawing (rendDRIVER::QualObj / QualInst, rebuilt in engine/rend_obj.cpp and
// rend_anim.cpp): WR_NOQUAL=1 draws everything (the test's old way), to compare
static int s_qual = -1;                 // the scene's static objects go through QualObj while set
static int s_nInst, s_nInstDrawn, s_nObj, s_nObjDrawn;
static char s_stats[260];
static double s_tStatic, s_tInst, s_tQual;              // (test) ms in the last frame: static draw, instances, tests
const char *tstDrawStats()
{
    _snprintf(s_stats, sizeof(s_stats) - 1, "last frame: instances %d of %d drawn, static objects %d of %d%s; "
              "static %.1f ms, instances %.1f ms (their tests %.1f)", s_nInstDrawn, s_nInst, s_nObjDrawn, s_nObj,
              getenv("WR_NOQUAL") ? " (WR_NOQUAL: no tests)" : "", s_tStatic, s_tInst, s_tQual);
    return s_stats;
}
static int qualOn()
{
    return !getenv("WR_NOQUAL") && rendDrv && rendDrv->camera;
}

// PLACEHOLDER (test renderer) for the mirror's draws (rendDRIVER::BeginCamera's camera, the driver's cull mode with a
// flipped window, _RenderObj, RenderPoly): tstSceneCam is what tstDrawScene draws from (0: the game camera); a picture
// seen in a mirror keeps the other side of the faces. No textures on the mirror itself: its flat colour.
camCAMERA *tstSceneCam;
int tstCullFlip, tstInMirror;
static platVERT s_mv[3000];

int rendDRIVER::_RenderObj(objOBJ *o, int)
{
    if (!o->nFace || !o->geom)
        return 0;
    unsigned col = (unsigned)o->geom->unk62;
    int n = 0;
    for (int f = 0; f < o->nFace && n + 3 <= 3000; f++) {
        objFACE *fc = o->GetFace(f);
        for (int j = 0; j < 3; j++) {
            m3dV *p = &o->geom->vert[fc->vert[j]];
            float *m = o->ltm;
            platVERT *v = &s_mv[n++];
            if (o->state & 1) {
                v->x = p->x;
                v->y = p->y;
                v->z = p->z;
            } else {
                v->x = p->x * m[0] + p->y * m[4] + p->z * m[8] + m[12];
                v->y = p->x * m[1] + p->y * m[5] + p->z * m[9] + m[13];
                v->z = p->x * m[2] + p->y * m[6] + p->z * m[10] + m[14];
            }
            v->color = col;
        }
    }
    rndTrisAlphaZ(s_mv, n / 3);
    return 1;
}

int rendDRIVER::RenderPoly(camCAMERA *, m3dPOLY *p, int, rendCFG *)
{
    platVERT v[3];
    for (int j = 0; j < 3; j++) {
        v[j].x = p->vert[j].x;
        v[j].y = p->vert[j].y;
        v[j].z = p->vert[j].z;
        v[j].color = 0;
    }
    rndTrisAlphaZ(v, 1);
    return 1;
}

static void drawObj(objOBJ *o, const float *inst, unsigned color, tstUVHOOK uvHook, int qualIn = 0)
{
    // rendDRIVER::RenderObj's test (v12 0x004FC1E0 -> QualObj): 3 = neither the object nor its children, 1 = not
    // the object; the children are tested from the qualification the object leaves (-1: all inside, no more tests)
    int qualOut = qualIn, skipSelf = 0;
    if (s_qual > 0) {
        int r = rendDrv->QualObj(o, qualIn, &qualOut);
        if (o->nFace > 0 && !o->inst)
            s_nObj++;
        if (r == 3)
            return;
        skipSelf = r == 1;
        if (!skipSelf && o->nFace > 0 && !o->inst)
            s_nObjDrawn++;
    }
    // hidden objects (procState 8) are not drawn: the "&tag" flags of gsLVL_SYSTEM::SetExtraObjFlags put it on
    // "&dom" domains (with their subtree, e.g. the spawn areas of &dom_ai_mino_1st_atrium), "&h", "&plrc" / "&aic"
    // (collision for the player / AI only); nor objects the animation's visibility track hides (procState 4, set by
    // animINST::_CalcObjModelMatrix) -- they are no longer posed either (the colt's left arm after a reload)
    if (o->procState & (8 | 4))
        return;
    if (o->name && !strncmp(o->name, "&dom", 4))        // (test) a domain is never seen (the white &dom_ai_mino_1st_atrium boxes)
        return;
    if (o->procState & 0x80000)                         // (test) a mirror: its own pass only (mrrMIRROR_SYSTEM)
        skipSelf = 1;
    if ((o->procState & 0x200) && o->nFace > 0 && getenv("WR_DRAWLIST")) {   // (test) a main-pass-excluded object drawn
        static int n;
        if (n++ < 5)
            printf("draw200: %s s_qual %d procState %X%c", o->name ? o->name : "?", s_qual, (unsigned)o->procState, 10);
    }
    // PLACEHOLDER (test renderer) for the render-sort buffers (rnsMNG::StoreObj / LinkInst): an object with render
    // state 0x1000000 (the held weapon, pwpWEAPON::ProcessFRAME) goes to the second buffer, which the render frame
    // (v12 0x0051BDDA) draws after clearing the depth buffer, with the near / far planes 0.05 / 100 -- so the
    // weapon never sinks into walls. Objects a LOD switch hides (procState 1 / 2, lod.cpp) are not drawn.
    int here = ((o->rendState & 0x1000000) != 0) == tstDrawLayer && !(o->procState & 3) && !skipSelf;
    if (here && o->nFace > 0 && o->geom && o->geom->vert && o->geom->split) {
        // PLACEHOLDER (test renderer) for the driver's lighting of entity models: the object's constant colour
        // (entENTITY::UpdateColorBlend -> SetConstColor: the lightmap texel under the entity, gsEffEnvirColor)
        // modulates the texture; its alpha is not used here
        if (s_entColor && o->geom->constColor)
            color = 0xFF000000 | (o->geom->constColor & 0x00FFFFFF);
        float m[16];
        if (o->state & 1) {                      // vertices already in world space (a modifier wrote
            static const float id[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };   // them: the water)
            memcpy(m, id, sizeof(m));
        } else if (inst)
            matMul(o->ltm, inst, m);
        else
            memcpy(m, o->ltm, sizeof(m));
        if (s_ntvMax < o->nFace * 3) {
            s_ntvMax = o->nFace * 3;
            s_tv = (platVERT3T2 *)realloc(s_tv, s_ntvMax * sizeof(platVERT3T2));
        }
        // "&bbv": a vertical billboard (objMOD_BILLBOARD flag 0x2000, not rebuilt): the quad turns about the
        // world Y axis through its centre to face the camera; each vertex keeps its height and its signed
        // horizontal distance from the centre along the quad's own width
        float bbC[3] = { 0, 0, 0 }, bbAxis[3] = { 1, 0, 0 }, bbRight[3] = { 1, 0, 0 };
        int bb = o->name && strstr(o->name, "&bbv") && s_haveEye;
        if (bb) {
            float p0[3], p1[3];
            for (int i = 0; i < o->nVert; i++) {
                float w[3];
                xform(m, &o->geom->vert[i], w);
                bbC[0] += w[0] / o->nVert; bbC[1] += w[1] / o->nVert; bbC[2] += w[2] / o->nVert;
            }
            xform(m, &o->geom->vert[0], p0);
            xform(m, &o->geom->vert[1], p1);
            float ax = p1[0] - p0[0], az = p1[2] - p0[2], L = sqrtf(ax * ax + az * az);
            if (L < 1e-4f && o->nVert > 2) {
                xform(m, &o->geom->vert[2], p1);
                ax = p1[0] - p0[0]; az = p1[2] - p0[2]; L = sqrtf(ax * ax + az * az);
            }
            if (L > 1e-4f) { bbAxis[0] = ax / L; bbAxis[2] = az / L; }
            float tx = s_eye[0] - bbC[0], tz = s_eye[2] - bbC[2], T = sqrtf(tx * tx + tz * tz);
            if (T > 1e-4f) { bbRight[0] = tz / T; bbRight[2] = -tx / T; }
        }
        // "&bb": a full billboard (objMOD_BILLBOARD, not rebuilt): the plane about its centre to face the camera
        int bbf = o->name && strstr(o->name, "&bb") && !bb && s_haveEye && o->nVert > 2;
        float fC[3] = { 0, 0, 0 }, fA[3], fB[3], fR[3], fU[3];
        if (bbf) {
            float p0[3], p1[3], p2[3];
            xform(m, &o->geom->vert[0], p0);
            xform(m, &o->geom->vert[1], p1);
            xform(m, &o->geom->vert[2], p2);
            for (int i = 0; i < o->nVert; i++) {
                float w[3];
                xform(m, &o->geom->vert[i], w);
                for (int k = 0; k < 3; k++)
                    fC[k] += w[k] / o->nVert;
            }
            float e1[3], e2[3], T[3], up[3] = { 0, 1, 0 };
            for (int k = 0; k < 3; k++) {
                e1[k] = p1[k] - p0[k];
                e2[k] = p2[k] - p0[k];
                T[k] = s_eye[k] - fC[k];
            }
            float L = sqrtf(e1[0] * e1[0] + e1[1] * e1[1] + e1[2] * e1[2]);
            float N[3] = { e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0] };
            float LN = sqrtf(N[0] * N[0] + N[1] * N[1] + N[2] * N[2]), LT = sqrtf(T[0] * T[0] + T[1] * T[1] + T[2] * T[2]);
            if (L < 1e-6f || LN < 1e-6f || LT < 1e-6f)
                bbf = 0;
            else {
                for (int k = 0; k < 3; k++) {
                    fA[k] = e1[k] / L;
                    N[k] /= LN;
                    T[k] /= LT;
                }
                fB[0] = N[1] * fA[2] - N[2] * fA[1];
                fB[1] = N[2] * fA[0] - N[0] * fA[2];
                fB[2] = N[0] * fA[1] - N[1] * fA[0];
                fR[0] = up[1] * T[2] - up[2] * T[1];
                fR[1] = up[2] * T[0] - up[0] * T[2];
                fR[2] = up[0] * T[1] - up[1] * T[0];
                float LR = sqrtf(fR[0] * fR[0] + fR[1] * fR[1] + fR[2] * fR[2]);
                if (LR < 1e-6f)
                    bbf = 0;
                else {
                    for (int k = 0; k < 3; k++)
                        fR[k] /= LR;
                    fU[0] = T[1] * fR[2] - T[2] * fR[1];
                    fU[1] = T[2] * fR[0] - T[0] * fR[2];
                    fU[2] = T[0] * fR[1] - T[1] * fR[0];
                }
            }
        }
        // the cull mode as d3dREND_DRIVER::BeginRasterizeObj (v12 0x004BF820, feb 0x004AFF80) sets it per object:
        // a double-sided object (state 4) none, every other one the faces of one side (the mirror pass would
        // take the other side; not drawn here). s_cullFront: the side this test's view keeps (tstCullFront)
        rndSetCull((o->state & 4) || !s_cull || bbf ? 1 : tstCullFlip ? 5 - s_cullFront : s_cullFront);
        objSPLIT *sp = o->geom->split;
        for (int e = 0; e < sp->n; e++) {               // one batch per split entry (material)
            objSPLIT_ENTRY *en = &sp->entry[e];
            int lm = lightmapLayer(sp, en);
            txmTEXTURE *tx = en->nMtl > 0 ? en->mtl[0].tex : 0;
            txmTEXTURE *tl = lm > 0 ? en->mtl[lm].tex : 0;
            {   // (test) the opaque pass first, the blended second (tstDrawScene): a blended surface writes no depth
                int bl = s_additive || (o->rendState & 0x40) ? 2 : (o->rendState & 0x80) || (tx && (tx->flags & TXM_F_ALPHA)) ? 1 : 0;
                if (s_blendPass >= 0 && (bl != 0) != s_blendPass)
                    continue;
            }
            int n = 0;
            for (int f = 0; f < o->nFace; f++) {
                objFACE *fc = o->GetFace(f);
                if (fc->split != e)
                    continue;
                for (int j = 0; j < 3; j++) {
                    platVERT3T2 *v = &s_tv[n++];
                    float w[3];
                    xform(m, &o->geom->vert[fc->vert[j]], w);
                    if (bbf) {
                        float d[3] = { w[0] - fC[0], w[1] - fC[1], w[2] - fC[2] };
                        float s = d[0] * fA[0] + d[1] * fA[1] + d[2] * fA[2], t = d[0] * fB[0] + d[1] * fB[1] + d[2] * fB[2];
                        for (int k = 0; k < 3; k++)
                            w[k] = fC[k] + fR[k] * s + fU[k] * t;
                    }
                    if (bb) {
                        float s = (w[0] - bbC[0]) * bbAxis[0] + (w[2] - bbC[2]) * bbAxis[2];
                        w[0] = bbC[0] + bbRight[0] * s;
                        w[2] = bbC[2] + bbRight[2] * s;
                    }
                    v->x = w[0];
                    v->y = w[1];
                    v->z = w[2];
                    v->color = color;
                    m2dV uv = { 0, 0 }, uv2 = { 0, 0 };
                    if (hasSet(o, sp, en, 0))
                        o->GetFaceTexCoord(f, j, 0, &uv);
                    if (lm > 0 && hasSet(o, sp, en, lm))
                        o->GetFaceTexCoord(f, j, lm, &uv2);
                    if (uvHook)
                        uvHook(o, &uv);
                    v->u = uv.x;
                    v->v = uv.y;
                    v->u2 = uv2.x;
                    v->v2 = uv2.y;
                }
            }
            if (n && tx && !tx->drvData && (tx->flags & TXM_F_UNLOADABLE))   // (the driver loads these when first drawn)
                txmManager->Load(tx, 0);
            if (n)
                rndTrisTex2(tstTexSurface(tx), tstTexSurface(tl), s_tv, n / 3,
                            // (the object's transparency, objOBJ::SetTransparency: 0x40 additive, 0x80 blended)
                            s_additive || (o->rendState & 0x40) ? 2 : (o->rendState & 0x80) || (tx && (tx->flags & TXM_F_ALPHA)) ? 1 : 0);
        }
    }
    objOBJ *c = o->child;
    if (c)
        do {
            drawObj(c, inst, color, uvHook, qualOut);
            c = c->next;
        } while (c != o->child);
}

// PLACEHOLDER (test renderer) for the driver's vertex shader pass (d3drSDR_REND_VERT): every drawn object of the tree
// again, with the shader's texture and its ShadeVert's coordinates and colours, blended
static void drawVertShader(objOBJ *o, pteSHADER *s)
{
    if (o->procState & (8 | 4))
        return;
    if (o->name && !strncmp(o->name, "&dom", 4))        // (test) a domain is never seen (the white &dom_ai_mino_1st_atrium boxes)
        return;
    if (!(o->procState & 3) && o->nFace > 0 && o->geom && o->geom->vert && s->tex) {
        if (s_ntvMax < o->nFace * 3) {
            s_ntvMax = o->nFace * 3;
            s_tv = (platVERT3T2 *)realloc(s_tv, s_ntvMax * sizeof(platVERT3T2));
        }
        if (!s->tex->drvData && (s->tex->flags & TXM_F_UNLOADABLE))
            txmManager->Load(s->tex, 0);
        int n = 0;
        for (int f = 0; f < o->nFace; f++) {
            objFACE *fc = o->GetFace(f);
            for (int j = 0; j < 3; j++) {
                platVERT3T2 *v = &s_tv[n++];
                float w[3];
                xform(o->ltm, &o->geom->vert[fc->vert[j]], w);
                v->x = w[0];
                v->y = w[1];
                v->z = w[2];
                u8 d[13];
                s->ShadeVert(o, fc->vert[j], d);
                v->u = *(float *)(d + 1);
                v->v = *(float *)(d + 5);
                v->color = *(unsigned *)(d + 9);
                v->u2 = v->v2 = 0;
            }
        }
        if (getenv("WR_PORTALDBG")) {                   // (test) the shader's alpha over this object
            static int fr;
            if (++fr % 60 == 1) {
                unsigned lo = 255, hi = 0, sum = 0;
                for (int i = 0; i < n; i++) {
                    unsigned al = s_tv[i].color >> 24;
                    lo = al < lo ? al : lo;
                    hi = al > hi ? al : hi;
                    sum += al;
                }
                txmTEXTURE *base = o->geom->split && o->geom->split->entry[0].nMtl > 0 ? o->geom->split->entry[0].mtl[0].tex : 0;
                printf("portal %s: alpha %u..%u avg %u, rendState %X state %X, base tex flags %X, view tex flags %X%c", o->name, lo, hi,
                       n ? sum / n : 0, (unsigned)o->rendState, (unsigned)o->state, base ? (unsigned)base->flags : 0, (unsigned)s->tex->flags, 10);
            }
        }
        if (n)
            rndTrisTex2(tstTexSurface(s->tex), 0, s_tv, n / 3, 1);
    }
    objOBJ *c = o->child;
    if (c)
        do {
            drawVertShader(c, s);
            c = c->next;
        } while (c != o->child);
}

void tstDrawTree(objOBJ *root, const float *inst, unsigned color)
{
    if (root)
        drawObj(root, inst, color, 0);
}

// A loaded level from an eye position (the game loop's level mode): the static geometry with its lightmaps,
// then the instances whose template the level carries (doors, altars, water...); the rest (AI, items) come
// from database\ templates in the game and are not drawn yet.
objOBJ *__fastcall dntSkyShown();                       // src/Sources/day_night.cpp
static void tstDrawScenePass(scnSCENE *s, const float *eye);
// PLACEHOLDER (test renderer) for the render-sort buffers' order: everything opaque, then everything blended (a
// fading corpse, glass) -- drawn in scene order alone, what was drawn after a blended surface showed through it
// PLACEHOLDER (test renderer) for the shaded pass of the ground fog (pteMANAGER's face / vertex shading of the
// render driver, not rebuilt): each active pteFOG_GROUND's objects drawn again, untextured, every corner the colour
// its ShadeVert gives (the fog colour, the fog as alpha), blended over what is there
static void tstDrawFog()
{
    static platVERT *v;
    static int nv;
    for (pteSHADER *sh = *(pteSHADER **)((char *)pteManager + 0x1F48); sh; sh = (pteSHADER *)sh->_10) {
        pteFOG_GROUND *f = dynamic_cast<pteFOG_GROUND *>(sh);
        if (!f)
            continue;
        for (int i = 0; i < f->nObj; i++) {
            objOBJ *o = f->objs[i];
            if (!o || !o->geom || !o->geom->vert || o->nFace <= 0 || (o->procState & (8 | 4)))
                continue;
            if (nv < o->nFace * 3) {
                nv = o->nFace * 3;
                v = (platVERT *)realloc(v, nv * sizeof(platVERT));
            }
            int n = 0;
            for (int fc = 0; fc < o->nFace; fc++)
                for (int k = 0; k < 3; k++) {
                    m3dV *s = o->GetFaceVertPtr(fc, k);
                    m3dV w;
                    if (o->state & 1)
                        w = *s;
                    else {
                        float *m = o->ltm;
                        w.x = s->x * m[0] + s->y * m[4] + s->z * m[8] + m[12];
                        w.y = s->x * m[1] + s->y * m[5] + s->z * m[9] + m[13];
                        w.z = s->x * m[2] + s->y * m[6] + s->z * m[10] + m[14];
                    }
                    float a = f->CalcVertFog(&f->camPos, &w) * 255.0f;   // (ShadeVert's colour, from the world point)
                    platVERT *q = &v[n++];
                    q->x = w.x;
                    q->y = w.y;
                    q->z = w.z;
                    q->color = ((u32)(int)a << 24) | (((u32)(int)f->color[0] & 0xFF) << 16) |
                               (((u32)(int)f->color[1] & 0xFF) << 8) | ((u32)(int)f->color[2] & 0xFF);
                }
            if (n)
                rndTrisAlpha(v, n / 3);
        }
    }
}

void tstDrawScene(scnSCENE *s, const float *eye)
{
    s_blendPass = 0;
    tstDrawScenePass(s, eye);
    // the fog shades the solid faces only: water and lava surfaces (blended) are drawn over it, as the render-sort
    // buffers do (the fogged level first, the transparent ones after)
    if (!tstDrawLayer && pteManager && !getenv("WR_NOFOG"))
        tstDrawFog();
    s_blendPass = 1;
    tstDrawScenePass(s, eye);
    s_blendPass = -1;
}

static void tstDrawScenePass(scnSCENE *s, const float *eye)
{
    // PLACEHOLDER for dntSYSTEM::RenderSky (rebuilt; the test renderer has no sky pass to call it from): of the two
    // skies InitInst took out of the drawing, the one the night weight shows is drawn as part of its instance (no
    // fade between them)
    if (objOBJ *sky = dntSkyShown()) {
        static objOBJ *shown;
        if (shown != sky) {
            if (shown)
                shown->SetStateProcYes(0x1008, 0);
            sky->SetStateProcNo(0x1008, 0);
            shown = sky;
        }
    }
    s_eye[0] = eye[0]; s_eye[1] = eye[1]; s_eye[2] = eye[2];
    s_haveEye = 1;
    // PLACEHOLDER for rendDRIVER::BeginCamera (v12 0x004EA5A0, not rebuilt): the driver draws from the game camera,
    // its view frustum worked out again for the frame (camCAMERA::CalcPOVQual) -- what QualObj / QualInst test with
    if (rendDrv && gsCameraPtr) {
        rendDrv->camera = tstSceneCam ? tstSceneCam : gsCameraPtr;
        rendDrv->camera->CalcPOVQual();
    }
    int qual = qualOn();
    if (!tstDrawLayer) {
        s_nInst = s_nInstDrawn = s_nObj = s_nObjDrawn = 0;
        s_qual = qual;
        double t0 = platTime();
        drawObj(s->root, 0, 0xFFFFFFFF, 0);
        s_tStatic = (platTime() - t0) * 1000.0;
        s_tQual = 0;
        s_qual = 0;
    }
    double tI = platTime();
    for (int k = 0; k < 1000; k++) {
        animINST *a = s->InstPtr(k);
        if (a->magic != 0x41494E53 || !a->tplName)
            continue;
        if (a->className && !strnicmp(a->className, "ai_", 3) && !a->ent)
            continue;                                   // enemies whose AI is not rebuilt (no entity): not drawn (slow)
        if (a->tplName && !strnicmp(a->tplName, "ai_mino", 7) && !a->ent)
            continue;                                   // (asked for) minotaurs without their class: oversized
        if (a->ent) {                                   // an entity (door...): its own posed tree, world matrices
            // PLACEHOLDER for the state part of rendDRIVER::RenderInst (v12 0x0059B900; the render frame v12
            // 0x0051BA50 passes 0x18000 on its first pass of a frame): the instance invalidated with the modifier
            // time (0x8000) and brought up to date for drawing (+ 0x621C: matrices, animation, the modifiers'
            // vertices 0x200 and texture coordinates 0x4000) -- the water's waves move here
            if (a->root && !tstDrawLayer) {
                a->flags &= 0x7FFFFFFF;                 // (RenderInst: "drawn" cleared, set again once drawn --
                                                        //  the items spin and bob only then)
                a->Invalidate(0x18000);
                if (rendDrv && rendDrv->camera)
                    a->LODApply(rendDrv->camera);       // (RenderInst v12 0x0059B9E6: each "&lod" switch's level from
                                                        //  the camera distance -- one level of each chain drawn)
                a->Validate(0x18000 + 0x621C);
            }
            // rendDRIVER::QualInst (rebuilt): an instance with flags & 3 (a put-away weapon: SetCurWpn) is not drawn
            // -- it is not animated either, so it would hang in its last pose; nor one outside the view or less
            // than 2 pixels across
            int iq = 0, vis = a->root && (void *)a->ent != (void *)plrPlayer && !(a->flags & 3);
            if (vis && !tstDrawLayer)
                s_nInst++;
            if (vis && qual && a->bvCur) {
                double q0 = platTime();
                vis = rendDrv->QualInst(a, &iq);
                s_tQual += (platTime() - q0) * 1000.0;
            }
            if (vis && !tstDrawLayer)
                s_nInstDrawn++;
            if (vis) {
                static int listed;
                if (getenv("WR_DRAWLIST") && listed < 400 && a->name && strstr(a->name, "player")) {   // (test) what is drawn
                    listed++;
                    printf("drawlist L%d: %s (%s) flags %X%c", tstDrawLayer, a->name ? a->name : "?", a->tplName, (unsigned)a->flags, 10);
                }
                s_entColor = 1;
                s_qual = qual;                          // (QualObj's state tests: the body's 0x200 objects)
                drawObj(a->root, 0, 0xFFFFFFFF, 0, iq);
                s_qual = 0;
                s_entColor = 0;
                if (!tstDrawLayer)                      // (the second pass has no depth left: it would show through walls)
                    if (pteSHADER *vs = tstEntVertShader(a->ent, s))
                        drawVertShader(a->root, vs);
                if (!tstDrawLayer)
                    a->flags |= 0x80000000;
            }
            continue;
        }
        if (tstDrawLayer)
            continue;
        s_nInst++;
        int iq = 0;                                     // (an instance the test draws from its template's tree:
        double q0 = platTime();                         //  its own volume decides the same way)
        int iv = !(qual && a->root && a->bvCur) || rendDrv->QualInst(a, &iq);
        s_tQual += (platTime() - q0) * 1000.0;
        if (!iv)
            continue;
        s_nInstDrawn++;
        if (getenv("WR_DRAWLIST") && strstr(a->tplName, "player"))
            printf("drawlist tpl: %s (%s) flags %X ent %p%c", a->name ? a->name : "?", a->tplName, (unsigned)a->flags,
                   (void *)a->ent, 10);
        for (int t = 0; t < 300; t++) {
            animTPL *tp = &s->tplPool[t];
            if (tp->magic && tp->root && !stricmp(tp->tplName, a->tplName)) {
                drawObj(tp->root, a->matr, 0xFFFFFFFF, 0);
                break;
            }
        }
    }
    if (!tstDrawLayer)
        s_tInst = (platTime() - tI) * 1000.0;
}

static void projMatrix(float hDeg, float vDeg, float zn, float zf, float *proj)
{
    float xs = 1.0f / tanf(hDeg * 0.5f * 3.14159265f / 180.0f);
    float ys = vDeg > 0 ? 1.0f / tanf(vDeg * 0.5f * 3.14159265f / 180.0f) : xs * rndWidth() / rndHeight();
    float p[16] = { xs, 0, 0, 0, 0, ys, 0, 0, 0, 0, zf / (zf - zn), 1, 0, 0, -zn * zf / (zf - zn), 0 };
    memcpy(proj, p, sizeof(p));
}

// A game camera looks down its -Z (right-handed): z is negated into D3D's left-handed view space
static void camViewProj(camCAMERA *c, float *view, float *proj)
{
    memcpy(view, (float *)&c->w2c, 64);
    view[2] = -view[2];
    view[6] = -view[6];
    view[10] = -view[10];
    view[14] = -view[14];
    projMatrix(c->angleHor > 1 ? c->angleHor : 60.0f, c->angleVert, 1.0f, 20000.0f, proj);
}

// ---------------------------------------------------------------------------------------------
// the UI data
struct tstELEM {
    objOBJ *o;
    uiRENDER_INFO re;                                   // (an area's render info: the appear fade's data at +0xB8)
    char type[0x40], frameID[0x40], igFrameID[0x40], code[0x10];
    int button;
};
struct tstFRAME {
    objOBJ *o;
    char name[0x40];
    int first, n;                                       // range in s_elems
};
#define MAX_ELEMS  2048
#define MAX_FRAMES 96
static tstELEM  s_elems[MAX_ELEMS];
static int      s_nElems;
static tstFRAME s_frames[MAX_FRAMES];
static int      s_nFrames;
static animTPL *s_ui;
static scnSCENE *s_uiScene;
static uiRENDER_ELEM s_cursor;
static float    s_mx = 512, s_my = 384;
static int      s_uiReady;
static int      s_debugFrame = -1;
static animTPL *s_fontTpl[3];                           // (test) the font templates (kept: tstMenuMarkTex)
static animTPL *s_flameTpl;                             // databasesfx_flame.tpl (the torches)
static float    s_flamePhase;
const char     *tstMenuShot;
const char     *tstMenuLevelPath;                       // set when DEBUG > PLAY LEVEL returns TST_MENU_PLAYLG
const char     *tstMenuShotFrame;                       // --menuframe <name>: the frame to show first (screenshots)

static void propStr(objOBJ *o, const char *key, char *out, int n)
{
    out[0] = 0;
    if (o->props)
        o->props->GetStr((char *)"UI_PROP", (char *)key, out, n);
}

static int isContainer(const char *type)
{
    int n = (int)strlen(type);
    return (n > 4 && !strcmp(type + n - 4, "_wnd")) || strstr(type, "frm") || !strcmp(type, "window");
}

static void collect(objOBJ *o)
{
    objOBJ *c = o->child;
    if (!c)
        return;
    do {
        char type[0x40];
        propStr(c, "type", type, sizeof(type));
        if (!isContainer(type) && s_nElems < MAX_ELEMS && (c->nVert || c->props)) {
            tstELEM *e = &s_elems[s_nElems++];
            e->o = c;
            new (&e->re) uiRENDER_ELEM;
            e->re.uiRENDER_ELEM::Init(c);
            strcpy(e->type, type);
            propStr(c, "frameID", e->frameID, sizeof(e->frameID));
            propStr(c, "ig_frameID", e->igFrameID, sizeof(e->igFrameID));
            propStr(c, "code", e->code, sizeof(e->code));
            e->button = strstr(type, "btn") || strstr(type, "cback") || !strcmp(type, "wr_igresume"); // wrUI_BUTTON kinds
            if (e->button && (e->re.flags40 & UIRF_TEXT)) {     // wrUI_BUTTON: font 1 and its state colours
                char f[0x40];                           // uiBUTTON::Init: the normal font is GetFont(0) (font_b),
                propStr(c, "fontID", f, sizeof(f));     // only pFontID / rFontID / dFontID override a state
                (void)f;
                e->re.font = fontManager->GetFont(uiFontNames[1]);
                char ct[0x40];
                propStr(c, "colorText", ct, sizeof(ct));
                if (!ct[0])
                    e->re.colorText = 0x9C000000;
            }
        }
        collect(c);
        c = c->next;
    } while (c != o->child);
}

static int findFrame(const char *name)
{
    for (int i = 0; i < s_nFrames; i++)
        if (!stricmp(s_frames[i].name, name))
            return i;
    return -1;
}

static wrWCHAR *ownText(const char *a)
{
    int n = (int)strlen(a) + 1;
    wrWCHAR *w = (wrWCHAR *)apMalloc(n * 2);
    for (int i = 0; i < n; i++)
        w[i] = (wrWCHAR)a[i];
    return w;
}

static tstELEM *addTextButton(const tstELEM *proto, float dy, const char *text, const char *code)
{
    if (s_nElems >= MAX_ELEMS)
        return 0;
    tstELEM *e = &s_elems[s_nElems++];
    *e = *proto;
    for (int i = 0; i < 4; i++)
        e->re.rect.v[i].y += dy;
    e->re.text = ownText(text);
    e->re.textCap = (int)strlen(text) + 1;
    e->frameID[0] = 0;
    e->igFrameID[0] = 0;
    strncpy(e->code, code, sizeof(e->code) - 1);
    e->button = 1;
    return e;
}

// The DEBUG entry under QUIT, and the DEBUG frame (test only)
static const char *s_dbgItems[][2] = {
    { "PLAY BOX TEST LEVEL", "DPLY" }, { "PLAY LEVEL (.LG)", "DPLL" }, { "VIEW LEVEL (.LG)", "DLVL" }, { "VIEW TEXTURE (.PCT)", "DTEX" },
    { "VIEW TEMPLATE (.TPL)", "DTPL" }, { "UI FRAME BROWSER", "DFRM" }, { "TEXTURE LIST", "DTXL" }, { "BACK", "BACK" } };

static void addDebug()
{
    int m = findFrame("main");
    if (m < 0)
        return;
    tstELEM *quit = 0, *credits = 0;
    for (int i = s_frames[m].first; i < s_frames[m].first + s_frames[m].n; i++) {
        if (!strcmp(s_elems[i].o->name, "quit"))
            quit = &s_elems[i];
        if (!strcmp(s_elems[i].o->name, "credits"))
            credits = &s_elems[i];
    }
    if (!quit || !credits)
        return;
    float step = quit->re.rect.v[0].y - credits->re.rect.v[0].y;
    // the main frame's range must stay contiguous: the extra button goes at the end of the list, then the
    // frame is re-pointed at a copy of its elements followed by it
    int first = s_nElems;
    for (int i = s_frames[m].first; i < s_frames[m].first + s_frames[m].n && s_nElems < MAX_ELEMS; i++)
        s_elems[s_nElems++] = s_elems[i];
    addTextButton(quit, step, "DEBUG", "DBG");
    s_frames[m].first = first;
    s_frames[m].n = s_nElems - first;
    // the DEBUG frame: the same button style, one line per tool
    if (s_nFrames >= MAX_FRAMES)
        return;
    tstFRAME *df = &s_frames[s_nFrames];
    strcpy(df->name, "debug");
    df->o = s_frames[m].o;
    df->first = s_nElems;
    tstELEM *top = 0;
    for (int i = s_frames[m].first; i < s_frames[m].first + s_frames[m].n; i++)
        if (!strcmp(s_elems[i].o->name, "splayer"))
            top = &s_elems[i];
    if (!top)
        top = credits;
    int nItems = sizeof(s_dbgItems) / sizeof(s_dbgItems[0]);
    float y0 = top->re.rect.v[0].y - step * 0.5f;
    for (int k = 0; k < nItems; k++)
        addTextButton(top, y0 - top->re.rect.v[0].y + step * 0.8f * k, s_dbgItems[k][0], s_dbgItems[k][1]);
    if (s_nElems > df->first)
        s_elems[s_nElems - 1].frameID[0] = 0;
    df->n = s_nElems - df->first;
    s_debugFrame = s_nFrames++;
}

// PLACEHOLDER (test start-up): see the comment inside. Once; returns whether the sound system runs.
int tstSndStart(const char *gameDir)
{
    // PLACEHOLDER (test start-up; the game's is gsAPP_SYSTEM::InitAll): COM, the ini (apCfgInit) and the current
    // folder at <game>\bin as for the game (the ini's [Sound] Path "..\SOUNDS" is relative to it), msgSystem.Init and
    // the rebuilt gsSND_SYSTEM::Init (driver, cache, channel list, music); the menu woofer made by its rebuilt Create
    // and its rebuilt ProcessINIT called directly (the game: entCreate("ui_snd_woofer") from the ui scene, then INIT),
    // and the UI sounds linked as the end of the UI registration does (v12 0x0048BC59, the registration not rebuilt).
    static int tried;
    if (tried)
        return gsSysSound != 0;
    tried = 1;
    char p[0x200];
    {
        CoInitialize(0);
        if (gameDir[0] && (gameDir[1] == ':' || gameDir[0] == '\\')) {
            _snprintf(p, sizeof(p) - 1, "%s\\bin", gameDir);
            SetCurrentDirectoryA(p);
            _snprintf(p, sizeof(p) - 1, "%s\\bin\\willrock.ini", gameDir);
            apCfgInit(p, 0);
        }
        msgSystem.Init();
        gsMsgInit();
        gsAppState |= 0x40000;                      // (test) as the game's start-up does (v12 0x0053426C): without it
                                                    // entIACTIVE_OBJ::PreProcessMsg drops every FRAME
        gsAppState |= 0x8000;                       // (test) as WinMain does (v12 0x004AA3CD): the debug lock -- the
                                                    // fly key (~) and Alt+4 need Alt+9 first, as in 1.2
        tstServerInit();                          // (test) the single-player server (GetNUpdatePoint)
        gsSysSound = new gsSND_SYSTEM;
        int sr = gsSysSound->Init();
        if (!sr) {                                      // (test) no ini / [Sound] section: run without sound (the
            printf("sound: gsSND_SYSTEM::Init failed (no %s\\bin\\willrock.ini [Sound]?): no sound\n", gameDir);
            gsSysSound = 0;                             // game's InitAll stops with an error here)
        } else {
            printf("sound: gsSND_SYSTEM::Init %d (enabled %d, driver %p, cache %p), %d channels\n", sr,
                   gsSysSound->enabled, (void *)sndSystem, (void *)snmManager, gsSysSound->GetNChn());
        }
    }
    return gsSysSound != 0;
}

void tstHudEnsure();

int tstMenuInit(const char *gameDir)
{
    strncpy(s_game, gameDir, sizeof(s_game) - 1);
    char p[0x200];
    tstRendDrvEnsure();                                 // (test) the driver stand-in (test_ents.cpp)
    uiSystem = new wrUI_SYSTEM;                         // (InitSubsys: new 0x16C + the rebuilt ctor)
    if (tstSndStart(gameDir) && !((wrUI_SYSTEM *)uiSystem)->woofer) {
        wrUI_WOOFER *w = (wrUI_WOOFER *)wrUI_WOOFER::Create(0);
        w->inst = (animINST *)apCalloc(1, sizeof(animINST)); // (test) its instance: zeroed (the game: the ui scene's)
        w->ProcessINIT();
        printf("sound: menu woofer, UI sounds linked %d\n", sncLinkSndListData(7, wrUISnd));
    }
    fontManager = (uiFONT_MANAGER *)apCalloc(1, sizeof(uiFONT_MANAGER));
    gsStrings = new gsSTRINGS;
    _snprintf(p, sizeof(p) - 1, "%s\\texts\\strings.str", gameDir);
    int r = gsStrings->LoadStringsTable(p);
    printf("strings %s: %d (%d strings)\n", p, r, gsStrings->strPtrs.count);
    for (int i = 0; i < 3; i++) {                       // uiUI_SYSTEM::Init: GetNumFonts() fonts by GetFontName(i)
        _snprintf(p, sizeof(p) - 1, "%s\\database\\%s.tpl", gameDir, uiFontNames[i]);
        animTPL *t = tstLoadTemplate(p);
        if (!t)
            continue;
        s_fontTpl[i] = t;
        objCalcLTM(t->root);
        objCalcBBox(t->root);
        objOBJ *fo = tstFindObj(t->root, "ui_font");
        if (!fo)
            continue;
        uiFONT *f = (uiFONT *)apCalloc(1, sizeof(uiFONT));
        strncpy(f->name, uiFontNames[i], 0x3F);
        f->Create(fo);
        f->next = fontManager->first;
        fontManager->first = f;
        printf("font %s: %d pages, ascent %.1f descent %.1f\n", f->name, f->pages.count, f->ascent, f->descent);
    }
    _snprintf(p, sizeof(p) - 1, "%s\\database\\ui.tpl", gameDir);
    s_ui = tstLoadTemplate(p);
    if (!s_ui)
        return 0;
    objCalcLTM(s_ui->root);
    objCalcBBox(s_ui->root);
    objOBJ *top = tstFindObj(s_ui->root, "ui_top");
    if (!top)
        return 0;
    uiUI_SYSTEM *u = (uiUI_SYSTEM *)uiSystem;
    u->screenW = top->bbox.max[0] - top->bbox.min[0];
    u->screenH = top->bbox.max[1] - top->bbox.min[1];
    u->scale.x = rndWidth() / u->screenW;                // uiUI_SYSTEM::Resize
    u->scale.y = rndHeight() / u->screenH;
    printf("ui page %.1f x %.1f (box x %.1f..%.1f  y %.1f..%.1f)\n", u->screenW, u->screenH, top->bbox.min[0],
           top->bbox.max[0], top->bbox.min[1], top->bbox.max[1]);
    objOBJ *c = top->child;
    if (c)
        do {
            if (s_nFrames < MAX_FRAMES) {
                tstFRAME *f = &s_frames[s_nFrames++];
                f->o = c;
                strncpy(f->name, c->name ? c->name : "?", 0x3F);
                f->first = s_nElems;
                collect(c);
                f->n = s_nElems - f->first;
                printf("ui frame %d: %s (%d elements)\n", s_nFrames - 1, f->name, f->n);
            }
            c = c->next;
        } while (c != top->child);
    objOBJ *cur = tstFindObj(s_ui->root, "ui_cursor");
    if (cur) {
        objOBJ *q = cur->nVert == 4 ? cur : cur->child;
        if (q) {
            s_cursor.Init(q);
        }
    }
    addDebug();
    _snprintf(p, sizeof(p) - 1, "%s\\database\\sfx_flame.tpl", gameDir);
    s_flameTpl = tstLoadTemplate(p);
    if (s_flameTpl) {
        objCalcLTM(s_flameTpl->root);
        objOBJ *fl = objFindNameN(s_flameTpl->root, (char *)"FLAME", 5);
        if (fl) {                                       // (debug) the flame quad
            printf("flame quad: ltm t %.2f %.2f %.2f  x %.2f %.2f %.2f  y %.2f %.2f %.2f  z %.2f %.2f %.2f\n", fl->ltm[12],
                   fl->ltm[13], fl->ltm[14], fl->ltm[0], fl->ltm[1], fl->ltm[2], fl->ltm[4], fl->ltm[5], fl->ltm[6],
                   fl->ltm[8], fl->ltm[9], fl->ltm[10]);
            for (int i = 0; i < fl->nVert; i++)
                printf("  v%d %.3f %.3f %.3f\n", i, fl->geom->vert[i].x, fl->geom->vert[i].y, fl->geom->vert[i].z);
            printf("  idHier %d, %d object anims\n", fl->idHier, s_flameTpl->nObjAnim);
            for (int i = 0; i < fl->geom->nTexCoord[0]; i++)
                printf("  uv%d %.4f %.4f\n", i, ((float *)fl->geom->texCoord[0])[i * 2], ((float *)fl->geom->texCoord[0])[i * 2 + 1]);
            for (int f = 0; f < fl->nFace; f++) {
                objFACE *fc = fl->GetFace(f);
                printf("  face %d verts %d %d %d tc %d %d %d\n", f, fc->vert[0], fc->vert[1], fc->vert[2], fc->tc[0], fc->tc[1], fc->tc[2]);
            }
            for (int i = 0; i < s_flameTpl->nObjAnim; i++) {
                animOBJ_ANIM *an = &s_flameTpl->objAnim[i];
                printf("  anim %d flags %X pos %.3f %.3f %.3f rot %.3f %.3f %.3f %.3f scale %.3f %.3f %.3f spl %p %p %p %p\n", i,
                       an->flags, an->pos[0], an->pos[1], an->pos[2], an->rot[0], an->rot[1], an->rot[2], an->rot[3],
                       an->scale[0], an->scale[1], an->scale[2], (void *)an->spl14, (void *)an->spl28, (void *)an->spl38,
                       (void *)an->spl3C);
                for (int fr = 0; fr <= 40; fr += 10) {
                    float sc[4] = { 0, 0, 0, 0 }, v4 = 0;
                    if (an->spl38)
                        an->spl38->GetValue((float)fr, sc, 0);
                    if (an->spl3C)
                        an->spl3C->GetValue((float)fr, &v4, 0);
                    printf("    frame %d: scale %.3f %.3f %.3f  4th %.3f  (nKp %d / %d)\n", fr, sc[0], sc[1], sc[2], v4,
                           an->spl38 ? an->spl38->nKp : 0, an->spl3C ? an->spl3C->nKp : 0);
                }
            }
            for (int i = 0; i < s_flameTpl->nSeq; i++)
                printf("  seq %d %s frames %.1f..%.1f len %.0f ms\n", i, s_flameTpl->seq[i].name, s_flameTpl->seq[i].frameStart,
                       s_flameTpl->seq[i].frameEnd, s_flameTpl->seq[i].lengthMs);
        }
    }
    _snprintf(p, sizeof(p) - 1, "%s\\scenes\\ui.lg", gameDir);
    s_uiScene = tstLoadScene(p);
    if (s_uiScene) {
        objCalcLTM(s_uiScene->root);
        for (int k = 0; k < 300; k++)
            if (s_uiScene->tplPool[k].magic && s_uiScene->tplPool[k].root)
                objCalcLTM(s_uiScene->tplPool[k].root);
        camCAMERA *cm = s_uiScene->camera;
        if (cm) {
            printf("ui.lg camera: hor %.1f vert %.1f aspect %.3f near %.2f\n", cm->angleHor, cm->angleVert, cm->aspectHW, cm->zNear);
            float *a = (float *)&cm->c2w, *b = (float *)&cm->w2c;
            for (int r = 0; r < 4; r++)
                printf("  c2w %8.3f %8.3f %8.3f %8.3f   w2c %8.3f %8.3f %8.3f %8.3f\n", a[r * 4], a[r * 4 + 1], a[r * 4 + 2],
                       a[r * 4 + 3], b[r * 4], b[r * 4 + 1], b[r * 4 + 2], b[r * 4 + 3]);
        }
    }
    printf("ui: %d frames, %d elements\n", s_nFrames, s_nElems);
    for (int k = 0; s_uiScene && k < 300; k++) {        // (debug) the skull's texture coordinates
        objOBJ *ch = s_uiScene->tplPool[k].magic ? tstFindObj(s_uiScene->tplPool[k].root, "cherep&ln&lt") : 0;
        if (!ch || !ch->geom)
            continue;
        txmTEXTURE *tx = ch->nFace ? ch->GetFaceTexMtlNmb(0, 0) : 0;
        printf("cherep: tex %s %dx%d fmt %d flags %08X, %d coord sets", tx ? tx->name : "-", tx ? tx->w : 0, tx ? tx->h : 0,
               tx ? tx->fmt : -1, tx ? tx->flags : 0, ch->geom->nTexCoord[0]);
        for (int i = 0; i < ch->geom->nTexCoord[0]; i++)
            printf(" (%.3f %.3f)", ((float *)ch->geom->texCoord[0])[i * 2], ((float *)ch->geom->texCoord[0])[i * 2 + 1]);
        printf("\n");
    }
    s_uiReady = 1;
    tstHudEnsure();                                     // the HUD exists before any level (messages can come first)
    return 1;
}

// ---------------------------------------------------------------------------------------------
// drawing
// objMOD_TEX_FRAME_ANIM (engine:obj_mod_tex.obj, feb 0x004CEED0 Modify / 0x004CEF90 ApplyFrameNmb, not rebuilt
// as a modifier yet), as the two menu effects set it up. Modify: t wraps at the period (loop), frame =
// floor(t / period * nFrames), at most nFrames - 1. ApplyFrameNmb adds to the node's frame-0 coordinates
// u = (frame % cols) / cols and v = (frame / cols) / rows, each scaled to the frame size in texels when one is
// given: v *= cols / texW * frameW, u *= rows / texH * frameH (the flame's 60 x 120 frames in a 512 x 512 sheet
// step by 60/512 and 120/512, not 1/8 and 1/4).
static void texFrameUV(objOBJ *o, m2dV *uv, int rows, int cols, float period, float phase, int nFrames, float fw, float fh)
{
    float t = fmodf(s_time + phase, period);
    int fr = (int)floorf(t / period * (float)nFrames);
    if (fr > nFrames - 1)
        fr = nFrames - 1;
    float v = (float)(fr / cols) / (float)rows;
    float u = (float)(fr % cols) / (float)cols;
    txmTEXTURE *tx = o->nFace > 0 ? o->GetFaceTexRendMtl(0, 0) : 0;
    if (fw > 0 && fh > 0 && tx && tx->w > 0 && tx->h > 0) {
        v = ((float)cols / (float)tx->w) * fw * v;
        u = ((float)rows / (float)tx->h) * fh * u;
    }
    uv->x += u;
    uv->y += v;
}

// wr_ui_cherep (wrUI_SKULL::ProcessMsg, message 1): the "cherep" node, SetParams(4, 4, 2.0 s, random phase,
// 16 frames, 128 x 128, loop)
static void cherepUV(objOBJ *o, m2dV *uv)
{
    if (!o->name || strncmp(o->name, "cherep", 6))
        return;
    texFrameUV(o, uv, 4, 4, 2.0f, 0.0f, 16, 128.0f, 128.0f);
}

// sfx_flame (wrsfxFLAME::ProcessMsg, message 1): the "FLAME..." quad gets an objMOD_TEX_FRAME_ANIM with
// SetParams(4 rows, 8 columns, 1.5 s, random phase, 30 frames, 60 x 120 frame, loop), constant colour
// 0xFFAFAFAF and transparency 0x40 / 255. Without [FLAME] isDayNight (ui.lg's flames have no script) there are
// no spark / smoke emitters. The sheet stepping is texFrameUV above. The node's "&bbv" is the vertical
// billboard (objMOD_BILLBOARD flag 0x2000: about Y through the pivot, local Z towards the camera), done in drawObj.



static void flameUV(objOBJ *o, m2dV *uv)
{
    if (!o->name || strncmp(o->name, "FLAME", 5))
        return;
    texFrameUV(o, uv, 4, 8, 1.5f, s_flamePhase, 30, 60.0f, 120.0f);
}

static void drawBackdrop()
{
    if (!s_uiScene || !s_uiScene->camera)
        return;
    float view[16], proj[16];
    camViewProj(s_uiScene->camera, view, proj);
    rndSetTransforms(view, proj);
    float *c2w = (float *)&s_uiScene->camera->c2w;
    s_eye[0] = c2w[12]; s_eye[1] = c2w[13]; s_eye[2] = c2w[14];
    s_haveEye = 1;
    drawObj(s_uiScene->root, 0, 0xFFFFFFFF, 0);
    for (int pass = 0; pass < 2; pass++)                // the see-through flames after everything else
        for (int k = 0; k < 1000; k++) {
            animINST *a = s_uiScene->InstPtr(k);
            if (a->magic != 0x41494E53 || !a->tplName)
                continue;
            int flame = !stricmp(a->tplName, "sfx_flame");
            if (flame != pass)
                continue;
            if (flame) {
                if (s_flameTpl) {
                    s_flamePhase = (float)((k * 7919) % 1500) * 0.001f;   // (m3dRandMax in the game)
                    static int once[1000];
                    if (!once[k]++)
                        printf("flame inst %d matr x %.2f %.2f %.2f y %.2f %.2f %.2f z %.2f %.2f %.2f t %.2f %.2f %.2f\n", k,
                               a->matr[0], a->matr[1], a->matr[2], a->matr[4], a->matr[5], a->matr[6], a->matr[8],
                               a->matr[9], a->matr[10], a->matr[12], a->matr[13], a->matr[14]);
                    s_additive = 1;                     // SetTransparency(0x40, 0xFF): additive
                    drawObj(s_flameTpl->root, a->matr, 0xFFAFAFAF, flameUV);
                    s_additive = 0;
                }
                continue;
            }
            for (int t = 0; t < 300; t++) {
                animTPL *tp = &s_uiScene->tplPool[t];
                if (tp->magic && tp->root && !stricmp(tp->tplName, a->tplName)) {
                    drawObj(tp->root, a->matr, 0xFFFFFFFF, cherepUV);
                    break;
                }
            }
        }
    rndClearDepth();
}

static int hitElem(tstELEM *e, float px, float py)
{
    return px >= e->re.rect.v[0].x && px <= e->re.rect.v[2].x && py >= e->re.rect.v[0].y && py <= e->re.rect.v[2].y;
}

// Keyboard selection, in the game's tab order: uiELEMENT::GetTabstopId (v12 0x00518D70) puts rows of 4 page units
// top to bottom and right before left in a row, uiELEMENT::FindTabstop (v12 0x00518DA0) takes the nearest id in
// the direction asked. PLACEHOLDER (test only): the menu hover and keys are the test's own until wr_ui.obj (wrUI_BUTTON,
// uiUI_SYSTEM::OnSysInput / SetMouseFocus) is rebuilt; then the menu runs through uiELEMENT focus instead.
static int tabId(tstELEM *e)
{
    int y = (int)e->re.rect.v[0].y, x = (int)e->re.rect.v[0].x;
    return ((y + 2) >> 2) * 0x4000 - x;
}

static tstELEM *tabNext(int fi, tstELEM *from, int dir)
{
    tstFRAME *f = &s_frames[fi];
    tstELEM *best = 0;
    int cur = from ? tabId(from) : (dir > 0 ? -0x7FFFFFFF : 0x7FFFFFFF), bestD = 0x7FFFFFFF;
    for (int i = f->first; i < f->first + f->n; i++) {
        tstELEM *e = &s_elems[i];
        if (!e->button || e == from)
            continue;
        int id = tabId(e);
        if ((dir > 0 && id <= cur) || (dir < 0 && id >= cur))
            continue;
        int d = id > cur ? id - cur : cur - id;
        if (d < bestD) {
            bestD = d;
            best = e;
        }
    }
    return best ? best : from;
}

// PLACEHOLDER mouse: a button's box is its whole render box (rectangle and text, uiRENDER_ELEM::CalcBBox), so
// the text of a button is hit even where its rectangle is smaller
static const int s_menuMouse = 1;                       // PLACEHOLDER: the menu mouse (0 = keys only)

static tstELEM *pickButton(int fi, float px, float py)
{
    tstFRAME *f = &s_frames[fi];
    for (int i = f->first; i < f->first + f->n; i++) {
        tstELEM *e = &s_elems[i];
        if (!e->button || !e->re.font)
            continue;
        uiBOX_ b;
        e->re.CalcBBox(&b);
        if (px >= b.min.x && px <= b.max.x && py >= b.min.y && py <= b.max.y)
            return e;
    }
    return 0;
}

static void drawFrame(int fi, float px, float py, int down, tstELEM **hover, tstELEM *sel = 0)
{
    tstFRAME *f = &s_frames[fi];
    *hover = 0;
    for (int i = f->first; i < f->first + f->n; i++) {
        tstELEM *e = &s_elems[i];
        unsigned keep = e->re.colorText;
        if (e == sel) {                                 // the selection (mouse or keys): drawn as rollover
            *hover = e;
            if (e->re.colorText == 0x9C000000)
                e->re.colorText = 0xFF000000;
        }
        if (e->re.font || !(e->re.flags40 & UIRF_TEXT))
            e->re.uiRENDER_ELEM::Render(0);
        e->re.colorText = keep;
    }
}

static void drawText(const char *s, float x, float y, unsigned color, float scale)
{
    uiFONT *f = fontManager->GetFont(uiFontNames[0]);
    if (!f)
        return;
    wrWCHAR w[256];
    int i = 0;
    for (; s[i] && i < 255; i++)
        w[i] = (wrWCHAR)(unsigned char)s[i];
    w[i] = 0;
    m2dV sc = ((uiUI_SYSTEM *)uiSystem)->scale, end;
    m2dV fs = { sc.x * scale, sc.y * scale };
    f->PrintW(&end, 0, w, x * sc.x, y * sc.y, color, &fs, 0);
}

// The build tag, top left, in font_s (test only); after the UI is up, on every screen including the game
void tstDrawText(const char *s, float x, float y, unsigned color, float scale)
{
    if (s_uiReady)
        drawText(s, x, y, color, scale);
}

void tstDrawWatermark()
{
    if (!s_uiReady)
        return;
    drawText("WILL ROCK - Decompilation Build 2026", 12, 26, 0xD0E8DCC0, 2.6f);   // y is the baseline
}

static void drawCursor(float px, float py)
{
    if (!s_cursor.font && !s_cursor.rect.tex)
        return;
    uiRENDER_ELEM c = s_cursor;
    float dx = px - s_cursor.rect.v[0].x, dy = py - s_cursor.rect.v[0].y;
    for (int i = 0; i < 4; i++) {                       // uiCURSOR::MoveTo
        c.rect.v[i].x += dx;
        c.rect.v[i].y += dy;
    }
    c.flags40 &= ~(UIRF_HIDDEN | UIRF_TEXT);
    c.rect.Render(0);
}

// ---------------------------------------------------------------------------------------------
// input helpers
static void pumpCursor(unsigned char *keys, int *click, int *rclick)
{
    static int last;
    int mx, my;
    platInput(keys, &mx, &my);
    uiUI_SYSTEM *u = (uiUI_SYSTEM *)uiSystem;
    s_mx += mx * 1.2f / u->scale.x;
    s_my += my * 1.2f / u->scale.y;
    if (s_mx < 0) s_mx = 0;
    if (s_my < 0) s_my = 0;
    if (s_mx > u->screenW) s_mx = u->screenW;
    if (s_my > u->screenH) s_my = u->screenH;
    int b = platMouseButtons();
    *click = (b & 1) && !(last & 1);
    *rclick = (b & 2) && !(last & 2);
    last = b;
}

static unsigned char s_keyPrev[256];                    // keyEdge's last state of each key

static int keyEdge(unsigned char *keys, int dik)
{
    int r = (keys[dik] & 0x80) && !(s_keyPrev[dik] & 0x80);
    s_keyPrev[dik] = keys[dik];
    return r;
}

static int openFile(const char *filter, const char *dir, char *out, int n)
{
    OPENFILENAMEA ofn;
    memset(&ofn, 0, sizeof(ofn));
    out[0] = 0;
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = GetActiveWindow();
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = out;
    ofn.nMaxFile = n;
    ofn.lpstrInitialDir = dir;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    ShowCursor(TRUE);
    int r = GetOpenFileNameA(&ofn);
    ShowCursor(FALSE);
    return r;
}

static void beginUI(unsigned clear)
{
    float I[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    float proj[16];
    projMatrix(60, 0, 1, 1000, proj);
    rndBegin(I, proj, clear);
}

// ---------------------------------------------------------------------------------------------
// DEBUG tools
static void viewTexture()
{
    char path[0x200], dir[0x200];
    _snprintf(dir, sizeof(dir) - 1, "%s\\textures", s_game);
    if (!openFile("Will Rock pictures (*.pct)\0*.pct\0All files\0*.*\0", dir, path, sizeof(path)))
        return;
    pctPICTURE *pic = pctRead(path);
    if (!pic) {
        printf("pctRead failed: %s\n", path);
        return;
    }
    unsigned *px = (unsigned *)calloc(pic->w * pic->h, 4);
    if (pctFormat[pic->fmt].flags & PCT_FMT_DXT)
        pctCopyViaUnpackDXTBuf(pic, pic->w, pic->h, 4, 0, 0, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, (u8 *)px);
    else
        pctCopyBuf(pic, pic->w, pic->h, 4, 0, 0, 0xFF0000, 0xFF00, 0xFF, 0xFF000000, (u8 *)px);
    if (!pctIsAlpha(pic->fmt))
        for (int i = 0; i < pic->w * pic->h; i++)
            px[i] |= 0xFF000000;
    void *tex = rndTexCreate(pic->w, pic->h);
    if (tex)
        rndTexUpload(tex, px, pic->w, pic->h);
    char info[0x200];
    const char *nm = strrchr(path, '\\');
    _snprintf(info, sizeof(info) - 1, "%s  %dX%d  %s  %d MIP  EXTRA %X/%X", nm ? nm + 1 : path, pic->w, pic->h,
              pctFormat[pic->fmt].name, pic->nMip, pic->extra.flags, pic->extra.value);
    for (char *q = info; *q; q++)
        *q = (char)toupper(*q);
    printf("texture %s\n", info);
    unsigned char keys[256];
    float zoom = 1;
    while (platPump()) {
        int click, rclick;
        pumpCursor(keys, &click, &rclick);
        if ((keys[DIK_ESCAPE] & 0x80) || rclick)
            break;
        if (keys[DIK_ADD] & 0x80 || keys[DIK_EQUALS] & 0x80) zoom *= 1.02f;
        if (keys[DIK_SUBTRACT] & 0x80 || keys[DIK_MINUS] & 0x80) zoom /= 1.02f;
        beginUI(0xFF202020);
        float W = (float)rndWidth(), H = (float)rndHeight();
        float s = zoom * (pic->w > pic->h ? (W * 0.8f) / pic->w : (H * 0.8f) / pic->h);
        float w = pic->w * s, h = pic->h * s, x0 = (W - w) * 0.5f, y0 = (H - h) * 0.5f;
        for (int cy = 0; cy < 16; cy++)                 // checker behind (alpha)
            for (int cx = 0; cx < 16; cx++) {
                float ax = x0 + w * cx / 16, ay = y0 + h * cy / 16, bx = ax + w / 16, by = ay + h / 16;
                unsigned col = ((cx + cy) & 1) ? 0xFF606060 : 0xFF909090;
                platVERT2D q[6] = { { ax, ay, 0, 1, col, 0, 0 }, { bx, ay, 0, 1, col, 0, 0 }, { bx, by, 0, 1, col, 0, 0 },
                                    { ax, ay, 0, 1, col, 0, 0 }, { bx, by, 0, 1, col, 0, 0 }, { ax, by, 0, 1, col, 0, 0 } };
                rndTris2D(0, q, 2);
            }
        platVERT2D q[6] = { { x0, y0, 0, 1, 0xFFFFFFFF, 0, 0 }, { x0 + w, y0, 0, 1, 0xFFFFFFFF, 1, 0 },
                            { x0 + w, y0 + h, 0, 1, 0xFFFFFFFF, 1, 1 }, { x0, y0, 0, 1, 0xFFFFFFFF, 0, 0 },
                            { x0 + w, y0 + h, 0, 1, 0xFFFFFFFF, 1, 1 }, { x0, y0 + h, 0, 1, 0xFFFFFFFF, 0, 1 } };
        rndTris2D(tex, q, 2);
        drawText(info, 20, 20, 0xFFFFFFFF, 1.0f);
        drawText("ESC / RIGHT CLICK: BACK   + -: ZOOM", 20, 735, 0xFFC0C0C0, 0.8f);
        tstDrawWatermark();
        rndEnd();
    }
    rndTexFree(tex);
    free(px);
    pctDestroy(pic);
}

// free-fly viewer over an object tree (a level's static geometry + its instances, or a template)
static void flyView(scnSCENE *scn, animTPL *tpl, const char *title)
{
    m3dV c = { 0, 0, 0 };
    float r = 50;
    objOBJ *root = scn ? scn->root : tpl->root;
    objCalcLTM(root);
    if (scn)
        for (int k = 0; k < 300; k++)
            if (scn->tplPool[k].magic && scn->tplPool[k].root)
                objCalcLTM(scn->tplPool[k].root);
    if (objCalcBBox(root)) {
        c.x = (root->bbox.min[0] + root->bbox.max[0]) * 0.5f;
        c.y = (root->bbox.min[1] + root->bbox.max[1]) * 0.5f;
        c.z = (root->bbox.min[2] + root->bbox.max[2]) * 0.5f;
        float dx = root->bbox.max[0] - root->bbox.min[0], dz = root->bbox.max[2] - root->bbox.min[2];
        r = sqrtf(dx * dx + dz * dz) * 0.5f + 1;
    }
    m3dV eye = { c.x, c.y + r * 0.3f, c.z - r * 1.2f };
    float yaw = 0, pitch = -10;
    if (scn && scn->camera) {
        scn->camera->GetOrigin(&eye);
        m3dV f;
        scn->camera->GetVForw(&f);
        yaw = atan2f(f.x, f.z) * 180 / 3.14159265f;
        pitch = asinf(f.y) * 180 / 3.14159265f;
    }
    float speed = r * 0.4f;
    unsigned char keys[256];
    double last = platTime();
    while (platPump()) {
        int mx, my;
        platInput(keys, &mx, &my);
        if (keys[DIK_ESCAPE] & 0x80)
            break;
        double now = platTime();
        float dt = (float)(now - last);
        last = now;
        if (dt > 0.1f)
            dt = 0.1f;
        yaw += mx * 0.15f;
        pitch -= my * 0.15f;
        if (pitch > 89) pitch = 89;
        if (pitch < -89) pitch = -89;
        float cy = cosf(yaw * 3.14159265f / 180), sy = sinf(yaw * 3.14159265f / 180);
        float cp = cosf(pitch * 3.14159265f / 180), sp = sinf(pitch * 3.14159265f / 180);
        m3dV fw = { sy * cp, sp, cy * cp }, rt = { cy, 0, -sy };   // right-handed world: +x is on the left
        float v = speed * dt * ((keys[DIK_LSHIFT] & 0x80) ? 4.0f : 1.0f);
        if (keys[DIK_W] & 0x80) { eye.x += fw.x * v; eye.y += fw.y * v; eye.z += fw.z * v; }
        if (keys[DIK_S] & 0x80) { eye.x -= fw.x * v; eye.y -= fw.y * v; eye.z -= fw.z * v; }
        if (keys[DIK_A] & 0x80) { eye.x += rt.x * v; eye.z += rt.z * v; }
        if (keys[DIK_D] & 0x80) { eye.x -= rt.x * v; eye.z -= rt.z * v; }
        if (keys[DIK_SPACE] & 0x80) eye.y += v;
        if (keys[DIK_C] & 0x80) eye.y -= v;
        // view: rows of the camera basis (x mirrored for D3D)
        m3dV up = { -sp * sy, cp, -sp * cy };
        m3dV lf = { -rt.x, 0, -rt.z };                   // D3D right = the game's left (-right)
        float view[16] = { lf.x, up.x, fw.x, 0, lf.y, up.y, fw.y, 0, lf.z, up.z, fw.z, 0,
                           -(eye.x * lf.x + eye.y * lf.y + eye.z * lf.z), -(eye.x * up.x + eye.y * up.y + eye.z * up.z),
                           -(eye.x * fw.x + eye.y * fw.y + eye.z * fw.z), 1 };
        float proj[16];
        projMatrix(80, 0, r * 0.002f + 0.05f, r * 20 + 100, proj);
        rndBegin(view, proj, 0xFF404858);
        drawObj(root, 0, 0xFFFFFFFF, 0);
        if (scn)
            for (int k = 0; k < 1000; k++) {
                animINST *a = scn->InstPtr(k);
                if (a->magic != 0x41494E53 || !a->tplName)
                    continue;
                for (int t = 0; t < 300; t++) {
                    animTPL *tp = &scn->tplPool[t];
                    if (tp->magic && tp->root && !stricmp(tp->tplName, a->tplName)) {
                        drawObj(tp->root, a->matr, 0xFFFFFFFF, 0);
                        break;
                    }
                }
            }
        char info[0x200];
        _snprintf(info, sizeof(info) - 1, "%s   POS %.0f %.0f %.0f", title, eye.x, eye.y, eye.z);
        for (char *q = info; *q; q++)
            *q = (char)toupper(*q);
        drawText(info, 20, 20, 0xFFFFFFFF, 0.9f);
        drawText("MOUSE LOOK  WASD MOVE  SPACE/C UP/DOWN  SHIFT FAST  ESC BACK", 20, 735, 0xFFC0C0C0, 0.7f);
        tstDrawWatermark();
        rndEnd();
    }
}

static void viewLevel()
{
    char path[0x200], dir[0x200];
    _snprintf(dir, sizeof(dir) - 1, "%s\\scenes", s_game);
    if (!openFile("Will Rock levels (*.lg)\0*.lg\0All files\0*.*\0", dir, path, sizeof(path)))
        return;
    scnSCENE *s = tstLoadScene(path);
    if (!s)
        return;
    const char *nm = strrchr(path, '\\');
    flyView(s, 0, nm ? nm + 1 : path);
}

static void viewTemplate()
{
    char path[0x200], dir[0x200];
    _snprintf(dir, sizeof(dir) - 1, "%s\\database", s_game);
    if (!openFile("Will Rock templates (*.tpl)\0*.tpl\0All files\0*.*\0", dir, path, sizeof(path)))
        return;
    animTPL *t = tstLoadTemplate(path);
    if (!t)
        return;
    tstDumpObj(t->root, 0, 4);
    const char *nm = strrchr(path, '\\');
    flyView(0, t, nm ? nm + 1 : path);
}

static void frameBrowser()
{
    int fi = 0;
    unsigned char keys[256];
    while (platPump()) {
        int click, rclick;
        pumpCursor(keys, &click, &rclick);
        if ((keys[DIK_ESCAPE] & 0x80) || rclick)
            break;
        if (keyEdge(keys, DIK_RIGHT) || click)
            fi = (fi + 1) % s_nFrames;
        if (keyEdge(keys, DIK_LEFT))
            fi = (fi + s_nFrames - 1) % s_nFrames;
        beginUI(0xFF000000);
        drawBackdrop();
        tstELEM *h;
        drawFrame(fi, -1, -1, 0, &h);
        char info[0x100];
        _snprintf(info, sizeof(info) - 1, "FRAME %d/%d: %s (%d ELEMENTS)   LEFT/RIGHT/CLICK: NEXT  ESC: BACK", fi + 1,
                  s_nFrames, s_frames[fi].name, s_frames[fi].n);
        for (char *q = info; *q; q++)
            *q = (char)toupper(*q);
        drawText(info, 10, 745, 0xFFFFFF80, 0.6f);
        tstDrawWatermark();
        rndEnd();
    }
}

static void textureList()
{
    int n = 0;
    for (txmTEXTURE *t = txmManager->GetNext(0); t; t = txmManager->GetNext(t), n++)
        printf("tex %3d %-32s %4dx%-4d fmt %2d mip %d flags %08X\n", t->listNmb, t->name, t->w, t->h, t->fmt, t->nMip, t->flags);
    printf("%d textures in the manager\n", n);
}

// ---------------------------------------------------------------------------------------------
// The in-game HUD. All of it is the game's (src/Sources/wr_ui_hud.cpp: wrUIHUD_WINDOW / wrUIHUD_ICONBAR, on the
// rebuilt gs:ui_main / ui_area / ui_rnd element code). The test only does what uiUI_SYSTEM would: registers the
// two element types, builds the "hud_wnd" subtree of ui.tpl with uiElementCreate, shows it as the frame group
// shows a frame (uiFRAME_GROUP::ProcessUpdate feb 0x00504230, state 1: StartAppearing(1), then ProcessAppearing
// every update while it fades in; the fade is the rebuilt wrUI_SYSTEM::StartAppear / ProcessAppear), then each frame
// ProcessAction(UIA_UPDATE) and ProcessAction(UIA_RENDER) on it (uiUI_SYSTEM::Update / Render feb 0x005052A0 /
// 0x00505260, not rebuilt yet).
static uiELEMENT *s_hudWnd;
static uiELEMENT *s_hudUhWnd;                           // "hud_uh_wnd" (wrUIHUD_UH_WINDOW)
static uiELEMENT *s_hudSpWnd;                           // "hud_sp_wnd" (wrUIHUD_SP_WINDOW): the TAB statistics
static uiELEMENT *s_hudMsg;                             // "hud_messages" (wrUIHUD_MSG) when made on its own

static uiELEMENT *findElem(uiELEMENT *e, const char *name)
{
    for (; e; e = e->next) {
        if (!strcmp(e->name, name))
            return e;
        if (uiELEMENT *c = findElem(e->firstChild, name))
            return c;
    }
    return 0;
}
static int        s_hudTried;

static int hudInit()
{
    // PLACEHOLDER for the wr start-up's call (v12 0x004AA6CE): the crosshairs' textures and [GameOpt] Crosshair, once
    // the UI's textures can load
    if (!wrCURSORS::Init())
        printf("hud: the crosshair textures did not load\n");
    objOBJ *o = tstFindObj(s_ui->root, "hud_wnd");
    if (!o)
        return 0;
    uiElementRegister("wrh_iconbar", wrUIHUD_ICONBAR::Create);
    uiElementRegister("wr_hud_wnd", wrUIHUD_WINDOW::Create);
    uiElementRegister("wr_slider", wrUIHUD_SLIDER::Create);
    uiElementRegister("wrh_msg", wrUIHUD_MSG::Create);
    s_hudWnd = uiElementCreate("wr_hud_wnd", o, 0);
    if (!s_hudWnd)
        return 0;
    s_hudWnd->ProcessAction(UIA_INIT, 0);
    int nFail = txmManager->LoadAll(0);                 // the icon sheets ProcessInit added
    int n = 0;
    for (uiELEMENT *c = s_hudWnd->firstChild; c; c = c->next)
        n++;
    printf("hud: hud_wnd with %d elements (%d textures failed)\n", n, nFail);
    s_hudWnd->StartAppearing(1);                        // the HUD frame is shown (wrUI_SYSTEM::ShowHudFrame)
    // the pain flash / chat / ping window, made like hud_wnd (the game: wrUI_SYSTEM's start-up v12 0x0048BD1E keeps
    // it at +0x144 for ShowPlayerPain)
    if (objOBJ *u = tstFindObj(s_ui->root, "hud_uh_wnd")) {
        // PLACEHOLDER for uiUI_SYSTEM's RegisterDefaultElements (inlined in its start-up in 1.2, not rebuilt): the base
        // types the untyped children become ("area" for blood_pic / ping)
        uiElementRegister("area", uiAREA::Create);
        uiElementRegister("window", uiWINDOW::Create);
        uiElementRegister("frame", uiFRAME::Create);
        uiElementRegister("wr_hud_uh_wnd", wrUIHUD_UH_WINDOW::Create);
        if ((s_hudUhWnd = uiElementCreate("wr_hud_uh_wnd", u, 0)) != 0) {

            s_hudUhWnd->ProcessAction(UIA_INIT, 0);
            s_hudUhWnd->StartAppearing(1);
            ((wrUI_SYSTEM *)uiSystem)->hudUhWnd = (wrUIHUD_UH_WINDOW *)s_hudUhWnd;
            printf("hud: hud_uh_wnd (pain flash) up\n");
        }
    }
    // the SP statistics (the game: wrUI_SYSTEM's start-up v12 0x0048BD0C keeps "hud_sp_wnd" at +0x13C)
    if (objOBJ *u = tstFindObj(s_ui->root, "hud_sp_wnd")) {
        uiElementRegister("wr_hud_sp_wnd", wrUIHUD_SP_WINDOW::Create);
        if ((s_hudSpWnd = uiElementCreate("wr_hud_sp_wnd", u, 0)) != 0) {
            s_hudSpWnd->ProcessAction(UIA_INIT, 0);
            ((wrUI_SYSTEM *)uiSystem)->hudSpWnd = s_hudSpWnd;
            printf("hud: hud_sp_wnd (statistics) up%c", 10);
        }
    }
    // the messages (the game: wrUI_SYSTEM's start-up v12 0x0048BD34 keeps "hud_messages" at +0x14C)
    uiELEMENT *m = findElem(s_hudWnd->firstChild, "hud_messages");
    if (!m)
        if (objOBJ *mo = tstFindObj(s_ui->root, "hud_messages"))
            if ((s_hudMsg = m = uiElementCreate("wrh_msg", mo, 0)) != 0) {
                m->ProcessAction(UIA_INIT, 0);
                m->StartAppearing(1);
            }
    ((wrUI_SYSTEM *)uiSystem)->hudMessages = (wrUIHUD_MSG *)m;
    printf("hud: hud_messages %s (%d console lines)\n", m ? (s_hudMsg ? "made" : "in hud_wnd") : "MISSING",
           m ? ((wrUIHUD_MSG *)m)->nCon : 0);
    return 1;
}

// PLACEHOLDER (test menu): the game reads the UI's templates (ui.tpl, the fonts) into its one scene, where
// gsLVL_SYSTEM::PreloadUnloadData marks their textures with every template's; the test keeps them apart (and ui.lg in
// a scene of its own), so before a level's purge it marks theirs the same way (objOBJ::MarkUsedTex).
void tstMenuMarkTex()
{
    animTPL *t[5] = { s_fontTpl[0], s_fontTpl[1], s_fontTpl[2], s_ui, s_flameTpl };
    for (int i = 0; i < 5; i++)
        if (t[i] && t[i]->root)
            t[i]->root->MarkUsedTex();
    if (!s_uiScene)
        return;
    if (s_uiScene->root)
        s_uiScene->root->MarkUsedTex();
    for (int k = 0; k < 300; k++)
        if (s_uiScene->tplPool[k].magic && s_uiScene->tplPool[k].root)
            s_uiScene->tplPool[k].root->MarkUsedTex();
    for (animINST *a = s_uiScene->FindNextInst(0); a; a = s_uiScene->FindNextInst(a))
        if (a->root)
            a->root->MarkUsedTex();
}

// PLACEHOLDER (test menu) for wrUI_SYSTEM::ShowLoadingScreen (v12 0x0048CE40) -> wrUI_LOADING_WND (SetParams,
// UpdateLevelName, ShowFrame: the frame "loading_frame", its "loading_mission" / "loading_level" texts set from
// _uiGetLevelNameStrId): the test draws that frame once, so it is on screen while the level loads.
void tstShowLoading(const char *level)
{
    if (!s_uiReady)
        return;
    int fi = findFrame("loading_frame");
    if (fi < 0)
        return;
    int mission = 1, lev = 1;
    _uiGetLevelNameStrId(level, &mission, &lev, 1);
    tstFRAME *f = &s_frames[fi];
    for (int i = f->first; i < f->first + f->n; i++) {
        tstELEM *e = &s_elems[i];
        const char *n = e->o && e->o->name ? e->o->name : "";
        if (!strcmp(n, "loading_mission"))
            e->re.SetTextId(mission);
        else if (!strcmp(n, "loading_level"))
            e->re.SetTextId(lev);
        if (getenv("WR_LOADDUMP"))
            printf("loading elem %s: flags %X font %p textId %d cap %d color %08X mission %d level %d\n", n, e->re.flags40, (void *)e->re.font, e->re.textId, e->re.textCap, e->re.colorText, mission, lev);
    }
    beginUI(0xFF000000);
    for (int pass = 0; pass < 2; pass++)                // the picture under the texts (it is the frame's last element)
        for (int i = f->first; i < f->first + f->n; i++) {
            tstELEM *e = &s_elems[i];
            int bg = e->o && e->o->name && !strcmp(e->o->name, "background");
            if (bg == (pass == 0))
                e->re.uiRENDER_ELEM::Render(0);
        }
    tstDrawWatermark();
    rndEnd();
}

// PLACEHOLDER (test menu) for the quit screen: wrUI_MAIN_WINDOW::OnChildNotify (feb 0x00495D20) shows the frame
// "splash_frame" (cursor hidden) once the quit is confirmed (UI_MSGCAP_QUIT / UI_MSGTEXT_QUIT_ACK message box, not
// in the test menu); wrUI_SPLASH_FRM then quits on a key or mouse press (OnKeyboard v12 0x0048F7A0 -> 0x004944D0:
// Deactivate, quit) or when its 7 s run out (ProcessUpdate v12 0x00494520). Drawn here like the loading frame.
static void showSplash()
{
    int fi = findFrame("splash_frame");
    if (fi < 0)
        return;
    tstFRAME *f = &s_frames[fi];
    unsigned char keys[256];
    double start = platTime(), last = start;
    int armed = 0;                                      // the press that chose QUIT is let go first
    for (int i = f->first; i < f->first + f->n; i++)    // it appears (wrUI_SYSTEM::StartAppear, 0.2 s)
        ((wrUI_SYSTEM *)uiSystem)->StartAppear(&s_elems[i].re, 0x40);
    while (platPump() && platTime() - start < 7.0) {
        double now = platTime();
        gsElapsedTime = (float)(now - last);
        last = now;
        for (int i = f->first; i < f->first + f->n; i++)
            if (s_elems[i].re.appear)
                ((wrUI_SYSTEM *)uiSystem)->ProcessAppear(&s_elems[i].re, 0x40);
        int click, rclick;
        pumpCursor(keys, &click, &rclick);
        int any = click || rclick;
        for (int k = 0; k < 256; k++)
            if (keys[k] & 0x80)
                any = 1;
        if (!any)
            armed = 1;
        else if (armed)
            break;
        beginUI(0xFF000000);
        for (int pass = 0; pass < 2; pass++)
            for (int i = f->first; i < f->first + f->n; i++) {
                tstELEM *e = &s_elems[i];
                int bg = e->o && e->o->name && !strcmp(e->o->name, "background");
                if (bg == (pass == 0))
                    e->re.uiRENDER_ELEM::Render(0);
            }
        tstDrawWatermark();
        rndEnd();
        if (tstMenuShot) {                              // (test) --menushot with QUIT: one picture of it
            rndScreenshot(tstMenuShot);
            break;
        }
    }
}

void tstHudEnsure()
{
    if (s_uiReady && !s_hudTried++ && !hudInit())
        printf("hud: no hud_wnd in ui.tpl\n");
}

// TWEAK (test, asked for): --alphahud or WR_ALPHAHUD=1, the early build's HUD from the screenshots: the game's unused
// database\\wr_iface.tpl laid out on its own 1024 x 768 page, the panels at their places, the ammo and health in green
// ("GOD" in god mode), the trace scrolling. Not the alpha's code, which does not exist any more. Not shown: the key
// panel (the test has no key count to show it by). The messages stay in font_h: font_alpha is not one of the game's.
extern const char *tstEntsGameDir;
static animTPL *s_aHud;
static void aHudQuad(objOBJ *src, objOBJ *rect, float du, unsigned col = 0xFFFFFFFF)
{
    uiVERTEX s[4], r[4];
    uiGetObjRenderRect(src, s);
    uiGetObjRenderRect(rect ? rect : src, r);
    float kx = rndWidth() / 1024.0f, ky = rndHeight() / 768.0f;
    static const int order[6] = { 0, 1, 2, 0, 2, 3 };
    platVERT2D q[6];
    for (int i = 0; i < 6; i++) {
        int k = order[i];
        q[i].x = r[k].x * kx;
        q[i].y = r[k].y * ky;
        q[i].z = 0.1f;
        q[i].rhw = 1.0f;
        q[i].color = col;
        q[i].u = s[k].u + du;
        q[i].v = s[k].v;
    }
    rndTris2D(tstTexSurface(src->GetFaceTexMtlNmb(0, 0)), q, 2);
}
static void aHudNumber(objOBJ *rect, const char *txt)
{
    uiFONT *f = fontManager->GetFont("font_h");
    if (!f)
        return;
    uiVERTEX r[4];
    uiGetObjRenderRect(rect, r);
    wrWCHAR w[16];
    int i = 0;
    for (; txt[i] && i < 15; i++)
        w[i] = (wrWCHAR)(unsigned char)txt[i];
    w[i] = 0;
    float kx = rndWidth() / 1024.0f, ky = rndHeight() / 768.0f;
    m2dV sc = { 3.5f * kx, 3.5f * ky }, end;
    f->PrintW(&end, 0, w, (r[0].x + r[2].x) * 0.5f * kx, (r[0].y + r[2].y) * 0.5f * ky, 0xFF2BC227, &sc, 1 | 0xC);
}
static int aHudDraw()
{
    if (!s_aHud) {
        char p[512];
        _snprintf(p, sizeof(p) - 1, "%s\\database\\wr_iface.tpl", tstEntsGameDir);
        p[sizeof(p) - 1] = 0;
        s_aHud = tstLoadTemplate(p);
        if (!s_aHud)
            return 0;
        objCalcLTM(s_aHud->root);
        objCalcBBox(s_aHud->root);
    }
    objOBJ *root = s_aHud->root;
    plrPLAYER *pl = (plrPLAYER *)plrPlayer;
    pwpWEAPON *wp = pl->curWeapon >= 0 && pl->curWeapon < PLR_WPN_COUNT ? pl->weapons[pl->curWeapon] : 0;
    const char *tn = wp && wp->inst ? wp->inst->tplName : 0;
    objOBJ *frame = tstFindObj(root, "hud_wpn_frame"), *hold = tstFindObj(root, "hud_wpn_holder");
    objOBJ *icon = tstFindObj(root, tn && strstr(tn, "bsk") ? "hud_wpn_bsk" : "hud_wpn_mcg");
    objOBJ *ecg = tstFindObj(root, "hud_ECG");
    if (!frame || !hold || !icon || !ecg)
        return 0;
    aHudQuad(frame, 0, 0);
    aHudQuad(icon, hold, 0);
    aHudQuad(ecg, 0, 0);
    float scroll = (float)fmod(platTime() * 0.5, 1.0);
    for (int i = 1; i <= 7; i++) {
        char n[16];
        sprintf(n, "heart_ECG_%02d", i);
        if (objOBJ *h = tstFindObj(root, n))
            aHudQuad(h, 0, scroll, 0xFF70E080);
    }
    char t[16];
    if (objOBJ *o = tstFindObj(root, "hud_wpn_num")) {
        sprintf(t, "%d", !wp ? 0 : wp->clip > 0 ? wp->clip : (int)wp->ammo);
        aHudNumber(o, t);
    }
    if (objOBJ *o = tstFindObj(root, "health_num")) {
        if (pl->moveFlags & PLR_MF_GOD)
            strcpy(t, "GOD");
        else
            sprintf(t, "%d", (int)pl->health);
        aHudNumber(o, t);
    }
    return 1;
}

void tstHudDraw()
{
    if (!s_uiReady || !plrPlayer)
        return;
    tstHudEnsure();
    if (!s_hudWnd)
        return;
    if (s_hudWnd->flags & UIEF_APPEARING)
        s_hudWnd->ProcessAppearing(0);
    static int alphaHud = getenv("WR_ALPHAHUD") != 0;
    s_hudWnd->ProcessAction(UIA_UPDATE, 0);
    if (!(alphaHud && aHudDraw()))
        s_hudWnd->ProcessAction(UIA_RENDER, 0);
    if (s_hudUhWnd && !alphaHud) {
        if (s_hudUhWnd->flags & UIEF_APPEARING)
            s_hudUhWnd->ProcessAppearing(0);
        s_hudUhWnd->ProcessAction(UIA_UPDATE, 0);
        s_hudUhWnd->ProcessAction(UIA_RENDER, 0);
    }
    if (s_hudSpWnd && !alphaHud) {
        // wrUI_SYSTEM::Update (v12 0x0048C74B), single player: shown while SHOWFRAGS (TAB) is held or the level is over
        int show = (gsAppState & 0x80000) || (gsSysInput && gsSysInput->IsOn(CMD_SHOWFRAGS));
        if (show)
            s_hudSpWnd->flags &= ~UIEF_HIDDEN;
        else
            s_hudSpWnd->flags |= UIEF_HIDDEN;
        s_hudSpWnd->ProcessAction(UIA_UPDATE, 0);
        if (show)
            s_hudSpWnd->ProcessAction(UIA_RENDER, 0);
    }
    if (s_hudMsg) {
        if (s_hudMsg->flags & UIEF_APPEARING)
            s_hudMsg->ProcessAppearing(0);
        s_hudMsg->ProcessAction(UIA_UPDATE, 0);
        s_hudMsg->ProcessAction(UIA_RENDER, 0);
    }
    // PLACEHOLDER (test, asked for; what hides it in 1.2 not found yet): no crosshair while a cinema plays
    // (gsAppState 0x200, cinSYSTEM::ProcessFRAME), as its own reason to hide (bit 5)
    gscCursor.SetHide(!(gsAppState & 0x200), 5);
    if (!(gsAppState & 0x1000))                         // the 2D pass draws the cursor last (v12 0x00534DD7)
        gscCursor.Draw(gsCameraPtr);
}

// ---------------------------------------------------------------------------------------------
// PLACEHOLDER (test menu): a button's press sound as wrUI_BUTTON::Init (v12 0x0048B500) picks it (code BACK 3, ACC 4,
// UI_PROP sndId "back" 3 / "accept" 4 / other 2) and GetActionSoundId (v12 0x0048B620) returns it (0: 2); the game's
// uiBUTTON::OnMouseButton / OnKeyboard (v12 0x0052B050 / 0x0052B130) play it with wrUI_SYSTEM::PlaySnd on the press.
// Replace with the rebuilt uiBUTTON / wrUI_BUTTON.
// PLACEHOLDER (test menu) for uiFRAME_GROUP::ShowFrame / ProcessUpdate (feb 0x00504170 / 0x00504230): a frame change
// lets the old frame disappear, then the new one appear -- each area's uiAREA::StartAppearing / ProcessAppearing (feb
// 0x005136D0 / 0x00513720) runs the rebuilt wrUI_SYSTEM::StartAppear / ProcessAppear (v12 0x0048C210 / 0x0048C2A0):
// a 0.2 s alpha ramp, how 0x100 out, else in. Here the whole frame at once, input waits meanwhile.
static void drawBackdrop();
static void fadeFrame(int fi, int how)
{
    if (fi < 0 || (tstMenuShot && !getenv("WR_FADELOG")))
        return;
    tstFRAME *f = &s_frames[fi];
    for (int i = f->first; i < f->first + f->n; i++)
        ((wrUI_SYSTEM *)uiSystem)->StartAppear(&s_elems[i].re, how);
    double last = platTime(), t0 = last;
    for (int busy = 1; busy && platPump();) {
        double now = platTime();
        gsElapsedTime = (float)(now - last);
        last = now;
        busy = 0;
        for (int i = f->first; i < f->first + f->n; i++)
            if (s_elems[i].re.appear && ((wrUI_SYSTEM *)uiSystem)->ProcessAppear(&s_elems[i].re, how))
                busy = 1;
        beginUI(0xFF000000);
        drawBackdrop();
        tstELEM *h;
        drawFrame(fi, -1, -1, 0, &h);
        tstDrawWatermark();
        rndEnd();
    }
    for (int i = f->first; i < f->first + f->n; i++)
        if (s_elems[i].re.appear)
            ((wrUI_SYSTEM *)uiSystem)->FinishAppear(&s_elems[i].re, how);
    if (getenv("WR_FADELOG"))                           // (test) each fade: the frame, the way, how long it ran
        printf("fade %s how %X: %.3f s%c", f->name, how, (float)(platTime() - t0), 10);
}
static void switchFrame(int *cur, int next)
{
    fadeFrame(*cur, 0x100);
    *cur = next;
    fadeFrame(next, 0x40);
}

static int tstButtonSnd(tstELEM *e)
{
    int id = 0;
    char v[0x40];
    if (!strcmp(e->code, "BACK"))
        id = 3;
    else if (!strcmp(e->code, "ACC"))
        id = 4;
    if (e->o && e->o->props && e->o->props->GetStr((char *)"UI_PROP", (char *)"sndId", v, 0x40))
        id = !strcmp(v, "back") ? 3 : !strcmp(v, "accept") ? 4 : 2;
    return id ? id : 2;
}

// ---------------------------------------------------------------------------------------------
// the comics (src/Sources/wr_ui_comics.cpp, rebuilt) in the test menu
static uiELEMENT *s_cmsPage;                            // ui.tpl's "comics_frame" made a wr_frm_comics
static int s_cmsOn;

// PLACEHOLDER (test menu) for wrUI_SYSTEM::ShowFrame (v12 0x0048D5B0; the menu's frame group is the test's own): the
// comics page is the one frame shown through it -- "comics_frame" shows it, any other ("main", the comic's end) ends it
int wrUI_SYSTEM::ShowFrame(const char *f, int how)
{
    if (s_cmsPage && f && !strcmp(f, "comics_frame")) {
        s_cmsPage->StartAppearing(1);
        s_cmsOn = 1;
    } else if (s_cmsOn) {
        s_cmsPage->StartAppearing(0);
        s_cmsOn = 0;
    }
    return 1;
}

// The page made from the template (as wrUI_SYSTEM's start-up makes every frame): its types registered, the elements
// built, ProcessInit; under a holder as the UI root, where wrUI_COMICS_FRAME::Start looks for it.
static int tstComicsEnsure()
{
    if (s_cmsPage)
        return 1;
    objOBJ *o = tstFindObj(s_ui->root, "comics_frame");
    if (!o)
        return 0;
    uiElementRegister("area", uiAREA::Create);
    uiElementRegister("window", uiWINDOW::Create);
    uiElementRegister("frame", uiFRAME::Create);
    uiElementRegister("wr_frm_comics", wrUI_COMICS_FRAME::Create);
    uiElementRegister("wr_comics", wrUI_COMICS::Create);
    s_cmsPage = uiElementCreate("wr_frm_comics", o, 0);
    if (!s_cmsPage)
        return 0;
    s_cmsPage->ProcessAction(UIA_INIT, 0);
    uiUI_SYSTEM *ui = (uiUI_SYSTEM *)uiSystem;
    if (!ui->root) {                                    // PLACEHOLDER (test): the root frame group ("ui_top") holds it
        uiELEMENT *top = uiFRAME::Create();
        top->firstChild = s_cmsPage;
        ui->root = top;
    }
    printf("comics: page up (player %p, %d elements)%c", (void *)((wrUI_COMICS_FRAME *)s_cmsPage)->comics,
           ((wrUI_COMICS_FRAME *)s_cmsPage)->comics ? ((wrUI_COMICS_FRAME *)s_cmsPage)->comics->nElem : -1, 10);
    return 1;
}

// A comic of willrock.ini [Language] (cmsLogo, cmsIntro, cmsOutro) played to its end before the menu (Esc ends it),
// as wrUI_SYSTEM::Activate plays cmsLogo the first time. WR_COMICSSHOT=prefix: a picture every 2 s (prefix_N.ppm).
static void tstComicsPlay(const char *key)
{
    char path[0x104];
    if (!_apCfgReadString(0, (char *)"Language", (char *)key, path, sizeof(path)) || !tstComicsEnsure())
        return;
    if (!wrUI_COMICS_FRAME::Start(0, 0, path, 1)) {
        printf("comics: %s (%s) not started%c", key, path, 10);
        return;
    }
    printf("comics: %s %s%c", key, path, 10);
    const char *shot = getenv("WR_COMICSSHOT");
    double last = platTime(), t0 = last, nextShot = 1.0;
    int nShot = 0;
    unsigned char keys[256];
    while (s_cmsOn && platPump()) {
        int click, rclick;
        pumpCursor(keys, &click, &rclick);
        double now = platTime();
        gsElapsedTime = (float)(now - last);
        last = now;
        if (gsSysSound)
            _gsMsgProcessFrameProfile();
        if (keyEdge(keys, DIK_ESCAPE)) {
            uiINPUT_INFO in = { 6, { 0, 0 }, 7 };       // (the UI's key event: Esc)
            s_cmsPage->OnKeyboard(&in);
        }
        if (s_cmsPage->flags & UIEF_APPEARING)
            s_cmsPage->ProcessAppearing(0);
        s_cmsPage->ProcessAction(UIA_UPDATE, 0);
        beginUI(0xFF000000);
        s_cmsPage->ProcessAction(UIA_RENDER, 0);
        tstDrawWatermark();
        rndEnd();
        if (shot && now - t0 >= nextShot) {
            char f[0x104];
            sprintf(f, "%s_%d.ppm", shot, nShot++);
            rndScreenshot(f);
            nextShot += 2.0;
        }
        if (getenv("WR_COMICSTIME") && now - t0 > atof(getenv("WR_COMICSTIME")))
            break;                                      // (test) a time limit
    }
    if (s_cmsOn)
        ((wrUI_SYSTEM *)uiSystem)->ShowFrame("main", 8);
}

int tstMenuRun(int inGame)
{
    if (!s_uiReady)
        return TST_MENU_PLAY;
    static int logoPlayed;                              // the first menu: the logo comic (Activate state 1)
    if (!inGame && !logoPlayed && !tstMenuShot && getenv("WR_COMICS")) {   // (test) opt-in: the pictures draw white yet
        logoPlayed = 1;
        tstComicsPlay(*getenv("WR_COMICS") ? getenv("WR_COMICS") : "cmsLogo");
    }
    int stack[32], sp = 0;
    int cur = findFrame(tstMenuShotFrame ? tstMenuShotFrame : inGame ? "ig_main" : "main");
    if (cur < 0)
        cur = 0;
    unsigned char keys[256];
    double last = platTime();
    char msg[0x100] = "";
    int shotFrames = 0;
    // (test) the Esc that opened the in-game menu is still down: not a press of the menu's own (it closed at once)
    s_keyPrev[DIK_ESCAPE] = 0x80;
    float msgT = 0;
    // the main menu's music: PlayAmbientMusic(0) each time the main window appears, in game too, as
    // wrUI_MAIN_WINDOW::StartAppearing (the rebuilt wrUI_SYSTEM slot +0x74)
    {
        ((wrUI_SYSTEM *)uiSystem)->PlayAmbientMusic(0);
        fadeFrame(cur, 0x40);                           // (the main window appearing)
        if (sndSystem)
            printf("music: flags %X (0x40 track set, 0x100 playing)\n", sndSystem->flags);
    }
    while (platPump()) {
        int click, rclick;
        pumpCursor(keys, &click, &rclick);
        // the game's frame of messages (gsMsgCallFrame -> _gsMsgProcessFrameProfile): due messages, FRAME to every
        // address (the sound system, the domain lists), next frame
        gsElapsedTime = (float)(platTime() - last);
        if (gsSysSound)
            _gsMsgProcessFrameProfile();
        if (!s_menuMouse)                               // PLACEHOLDER: the mouse is off (keys only) until fixed
            click = rclick = 0;
        double now = platTime();
        s_time += (float)(now - last);
        msgT -= (float)(now - last);
        last = now;
        int esc = keyEdge(keys, DIK_ESCAPE);
        if (tstMenuShot && getenv("WR_FADELOG")) {      // (test) a frame opened (frame 3) and left with Esc (frame 8)
            if (shotFrames == 3) {
                int t = findFrame("options");
                if (t >= 0) {
                    stack[sp++] = cur;
                    switchFrame(&cur, t);
                }
            }
            if (shotFrames == 8)
                esc = 1;
        }
        if (esc || rclick) {
            if (sp > 0) {
                uiSystem->PlaySnd(3);                   // wrUI_FRAME::OnKeyboard (v12 0x004942E0): Esc -> menu_back
                int to = stack[--sp];
                switchFrame(&cur, to);
            }
            else if (inGame)
                return TST_MENU_PLAY;
        }
        static tstELEM *sel;                            // keyboard selection (Up / Down / Enter)
        static int selFrame = -1;
        static float selMx, selMy;
        if (selFrame != cur) {
            sel = 0;
            selFrame = cur;
        }
        if (s_menuMouse && (s_mx != selMx || s_my != selMy)) { // PLACEHOLDER mouse: the button under the cursor
            sel = pickButton(cur, s_mx, s_my);
            selMx = s_mx;
            selMy = s_my;
        }
        int up = keyEdge(keys, DIK_UP), dn = keyEdge(keys, DIK_DOWN);
        int enterA = keyEdge(keys, DIK_RETURN), enterB = keyEdge(keys, DIK_NUMPADENTER), enter = enterA || enterB;
        if (up || dn) {
            tstELEM *was = sel;
            sel = tabNext(cur, sel, dn ? 1 : -1);
            if (sel && sel != was)                      // uiUI_SYSTEM::SetKbdFocus (v12 0x0051A570): PlaySnd(1)
                uiSystem->PlaySnd(1);                   //   menu_scroll, a focus moved to an element
            selMx = s_mx;
            selMy = s_my;
        }
        beginUI(0xFF000000);
        drawBackdrop();
        tstELEM *hover;
        drawFrame(cur, s_mx, s_my, 0, &hover, sel);
        hover = sel;                                    // a click and Enter both act on the selection
        if (sel && enter)
            click = 1;
        if (msgT > 0)
            drawText(msg, 20, 740, 0xFFFFE080, 0.7f);
        tstDrawWatermark();
        if (s_menuMouse)
            drawCursor(s_mx, s_my);
        if (tstMenuShot && getenv("WR_SNDTEST") && gsSysSound && shotFrames < 19) {   // (test) scroll sound x many
            wrUI_WOOFER *w = ((wrUI_SYSTEM *)uiSystem)->woofer;
            int h = uiSystem->PlaySnd(1);
            printf("sndtest frame %d: handle %d, busy %d %d %d %d%c", shotFrames, h,
                   w->chn[0]->IsPlay(-1), w->chn[1]->IsPlay(-1), w->chn[2]->IsPlay(-1), w->chn[3]->IsPlay(-1), ' ');
            printf("  chn0 played %d timeLeft %d len %d; list n %d%c", w->chn[0]->played, w->chn[0]->timeLeft,
                   w->chn[0]->cur ? (int)w->chn[0]->cur->data->lengthMs : -1, gsSysSound->entChannels->n, ' ');
            int inList = 0, nAddr = 0;
            for (msgADDR *a = msgSystem.GetFirstAddr(); a; a = a->GetNext(), nAddr++)
                if (a == (msgADDR *)gsSysSound)
                    inList = 1;
            printf("sound lock %d in list %d of %d, appState %X%c", *(int *)((char *)gsSysSound + 0x24), inList, nAddr,
                   (unsigned)gsAppState, 10);
            Sleep(150);
        }
        if (tstMenuShot && shotFrames == 5 && gsSysSound) { // (test) --menushot also plays menu_accept and logs it
            int h = uiSystem->PlaySnd(4);
            printf("uisnd: PlaySnd(4 menu_accept) handle %d\n", h);
        }
        if (tstMenuShot && (shotFrames == 6 || shotFrames == 19) && gsSysSound) {
            wrUI_WOOFER *w = ((wrUI_SYSTEM *)uiSystem)->woofer;
            printf("uisnd: frame %d: playing %d, channel 0 buf %p, cache voices %d\n", shotFrames,
                   uiSystem->IsSndPlaying(0), (void *)w->chn[0]->buffer, snmManager ? snmManager->nPlaying : -1);
        }
        if (tstMenuShot && ++shotFrames == 20) {        // --menushot <file.ppm>: one picture of the menu, then quit
            rndScreenshot(tstMenuShot);
            rndEnd();
            return TST_MENU_QUIT;
        }
        rndEnd();
        if (!click || !hover)
            continue;
        uiSystem->PlaySnd(tstButtonSnd(hover));         // (test) the press sound, see tstButtonSnd
        const char *code = hover->code;
        const char *target = inGame && hover->igFrameID[0] ? hover->igFrameID : hover->frameID;
        if (!strcmp(hover->type, "wr_igresume"))        // wrUI_IG_RESUME::ProcessCommand v12 0x004985E0: Deactivate
            return TST_MENU_PLAY;
        if (!strcmp(code, "QUIT")) {
            if (inGame)
                return TST_MENU_STOP;
            fadeFrame(cur, 0x100);
            showSplash();                               // the quit screen
            return TST_MENU_QUIT;
        }
        if (!strcmp(code, "EASY") || !strcmp(code, "NORM") || !strcmp(code, "HARD") || !strcmp(code, "DPLY"))
            return TST_MENU_PLAY;
        if (!strcmp(code, "DBG") && s_debugFrame >= 0) {
            stack[sp++] = cur;
            switchFrame(&cur, s_debugFrame);
            continue;
        }
        if (!strcmp(code, "DLVL")) { viewLevel(); continue; }
        if (!strcmp(code, "DPLL")) {                   // play a level: the game loop takes over with that scene
            static char path[0x200];
            char dir[0x200];
            _snprintf(dir, sizeof(dir) - 1, "%s\\scenes", s_game);
            if (openFile("Will Rock levels (*.lg)\0*.lg\0All files\0*.*\0", dir, path, sizeof(path))) {
                tstMenuLevelPath = path;
                return TST_MENU_PLAYLG;
            }
            continue;
        }
        if (!strcmp(code, "DTEX")) { viewTexture(); continue; }
        if (!strcmp(code, "DTPL")) { viewTemplate(); continue; }
        if (!strcmp(code, "DFRM")) { frameBrowser(); continue; }
        if (!strcmp(code, "DTXL")) { textureList(); strcpy(msg, "TEXTURE LIST PRINTED TO THE CONSOLE"); msgT = 3; continue; }
        if (target[0] && (!strcmp(code, "BACK") ? 1 : 1)) {
            int t = findFrame(target);
            if (t >= 0) {
                if (!strcmp(code, "BACK") && sp > 0)
                    sp--;
                else if (sp < 32)
                    stack[sp++] = cur;
                switchFrame(&cur, t);
                continue;
            }
        }
        if (!strcmp(code, "BACK") || !strcmp(code, "ACC")) {
            if (sp > 0) {
                int to = stack[--sp];
                switchFrame(&cur, to);
            }
            else if (inGame)
                return TST_MENU_PLAY;
            continue;
        }
        _snprintf(msg, sizeof(msg) - 1, "NOT IN THE REBUILD YET: %s", code[0] ? code : hover->o->name);
        for (char *q = msg; *q; q++)
            *q = (char)toupper(*q);
        msgT = 3;
    }
    return TST_MENU_QUIT;
}
