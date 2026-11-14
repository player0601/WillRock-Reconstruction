// mirror.cpp -- the mirrors and portals of a level (Will Rock 1.2, v12.exe; feb gs:mirror.obj)
//
// An object whose sheet says [MIRROR] isMirror or [PORTAL] isPortal. Before the frame, what each one sees is drawn over
// the whole screen from its reflected (or moved) camera, the depth cleared; the mirror's polygon then goes in the depth
// buffer, so the frame drawn after it leaves that part of the picture alone. Render runs inside itself once more
// (the render frame calls it): a mirror seen in a mirror, its picture clipped to the plane of the first.

#include "../Sources/game.h"
#include "../wr_vcall.h"
#include <stdlib.h>

extern vidDRIVER *vidDriver;                            // v12 0x006561C4 (feb 0x00611FE4)

static void *_mrrCfgMirror;                             // v12 0x009946E4 cfg cache of [Video] VIDEO_Mirror
static camCAMERA *_mrrCmpCam;                           // (feb 0x0094DE60) the view _mrrCmpMirror sorts for

static_assert(sizeof(mrrMIRROR) == 0x38, "mrrMIRROR size");
static_assert(__builtin_offsetof(mrrMIRROR_SYSTEM, nMirrors) == 0x28, "mrrMIRROR_SYSTEM +0x28");
static_assert(__builtin_offsetof(mrrMIRROR_SYSTEM, depth) == 0x48C, "mrrMIRROR_SYSTEM +0x48C");
static_assert(__builtin_offsetof(mrrMIRROR_SYSTEM, mainCam) == 0x494, "mrrMIRROR_SYSTEM +0x494");

static inline void mrrPointL2W(objOBJ *o, m3dV *p, m3dV *out)
{
    float *m = o->ltm;
    float x = p->x, y = p->y, z = p->z;
    out->x = x * m[0] + y * m[4] + z * m[8] + m[12];
    out->y = x * m[1] + y * m[5] + z * m[9] + m[13];
    out->z = x * m[2] + y * m[6] + z * m[10] + m[14];
}

static inline void mrrVecRot(float *m, m3dV *v, m3dV *out)
{
    float x = v->x, y = v->y, z = v->z;
    out->x = x * m[0] + y * m[4] + z * m[8];
    out->y = x * m[1] + y * m[5] + z * m[9];
    out->z = x * m[2] + y * m[6] + z * m[10];
}

// The colour the driver draws the object with: its constant one; with additive transparency (0x40) scaled by its alpha.
static void mrrSetDrawColor(objOBJ *o)
{
    objGEOM *g = o->geom;
    u32 c = g->constColor;
    if ((o->rendState & 0x40) && (c >> 24) != 0xFF) {
        float k = (float)(c >> 24) * 0.003921569f;
        c = 0xFF000000u | (((int)((float)((c >> 16) & 0xFF) * k) & 0xFF) << 16) |
            (((int)((float)((c >> 8) & 0xFF) * k) & 0xFF) << 8) | ((int)((float)(c & 0xFF) * k) & 0xFF);
    }
    g->unk62 = (int)c;
}

// (feb 0x0050A750; inline in 1.2's qsort call site's table)
// By priority, then the farther from the view first.
static int __cdecl _mrrCmpMirror(const void *pa, const void *pb)
{
    const mrrMIRROR *a = (const mrrMIRROR *)pa, *b = (const mrrMIRROR *)pb;
    if (b->priority < a->priority)
        return 1;
    if (b->priority <= a->priority) {
        m3dV ca, cb, va, vb;
        if (!a->obj->inst)
            wrVCall<void>(&a->obj->bbox, 0x24, &ca);    // m3dVOL::GetCenter
        else
            a->obj->GetOrigin(&ca);
        if (!b->obj->inst)
            wrVCall<void>(&b->obj->bbox, 0x24, &cb);
        else
            b->obj->GetOrigin(&cb);
        _mrrCmpCam->TransformPoint(&ca, &va);
        _mrrCmpCam->TransformPoint(&cb, &vb);
        if (va.z < vb.z)
            return 1;
    }
    return -1;
}

// v12 0x0051DCB0  (feb 0x00508B60)
mrrMIRROR_SYSTEM::mrrMIRROR_SYSTEM()
{
    msgADDR::flags = 0;
    magic = 0x4D414452;
    classId = 0;
    unk18 = 0;
    priority = 0x59;
    addrLock = 0;
    nMirrors = 0;
    memset(mirror, 0, sizeof(mirror));
    depth = 0;
}

// v12 0x0051DD50  (feb 0x00508C00)
mrrMIRROR_SYSTEM::~mrrMIRROR_SYSTEM()
{
}

// (feb 0x00508C50)
int mrrMIRROR_SYSTEM::Init()
{
    msgSystem.InsertAddr(this, 0x24535953, 0);
    return 1;
}

// v12 0x0051DDC0  (feb 0x00508C70)
void mrrMIRROR_SYSTEM::Term()
{
    nMirrors = 0;
    memset(mirror, 0, sizeof(mirror));
    msgSystem.RemoveAddr(this);
}

// v12 0x0051DDE0  (feb 0x00508C90)
BOOL32 mrrMIRROR_SYSTEM::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    return id == 2;
}

// v12 0x0051DE00  (feb 0x00508CB0)
// From the level's object notify: an object with a [MIRROR] or a [PORTAL] joins the list (0 when it is full).
int mrrMIRROR_SYSTEM::InitObj(objOBJ *o)
{
    psSHEET *ps = o->props;
    if (!ps)
        return 1;
    int isMirror = 0, isPortal = 0;
    ps->GetInt((char *)"MIRROR", (char *)"isMirror", &isMirror);
    ps->GetInt((char *)"PORTAL", (char *)"isPortal", &isPortal);
    if (!isMirror && !isPortal)
        return 1;
    if (nMirrors == MRR_MAX_MIRRORS)
        return 0;
    o->procState |= 0x80000;
    if (o->inst) {
        o->inst->Invalidate(4);
        o->inst->Validate(1);
    }
    o->SetStateRendYes(0xA0);
    mrrMIRROR *m = &mirror[nMirrors];
    m->flags = 0;
    m->obj = o;
    if (isMirror) {
        m->color.r = m->color.g = m->color.b = m->color.a = 0;
        ps->GetColor((char *)"MIRROR", (char *)"color", &m->color);
        m->priority = 0;
        ps->GetInt((char *)"MIRROR", (char *)"priority", &m->priority);
        int b;
        if (ps->GetInt((char *)"MIRROR", (char *)"isClipping", &b) && b)
            m->flags |= 0x10;
        if (ps->GetInt((char *)"MIRROR", (char *)"isRefProjShd", &b) && b)
            m->flags |= 0x40;
    }
    if (isPortal) {
        m->color.r = m->color.g = m->color.b = m->color.a = 0;
        ps->GetColor((char *)"PORTAL", (char *)"color", &m->color);
        m->priority = 0;
        ps->GetInt((char *)"PORTAL", (char *)"priority", &m->priority);
        m->flags |= 1;
        char name[0x40];
        ps->GetStr((char *)"PORTAL", (char *)"actor_name", name, 0x40);
        m->actor = gsScenePtr->FindInstName(name);
        if (!m->actor) {
            apMsg((char *)"Instance %s for portal %s not found", name, o->name);
            return 1;
        }
    }
    nMirrors++;
    return 1;
}

// v12 0x0051E0D0  (feb 0x00508F80)
void mrrMIRROR_SYSTEM::TermObj(objOBJ *o)
{
    for (int i = 0; i < nMirrors; i++) {
        if (mirror[i].obj == o) {
            if (i < nMirrors - 1)
                memmove(&mirror[i], &mirror[i + 1], (nMirrors - i - 1) * sizeof(mrrMIRROR));
            i--;
            nMirrors--;
        }
    }
}

// v12 0x0051E160  (feb 0x00509010)
int mrrMIRROR_SYSTEM::GetNActive()
{
    int n = 0;
    for (int i = 0; i < nMirrors; i++)
        if (!(mirror[i].obj->procState & 0x3F))
            n++;
    return n;
}

// v12 0x0051E180  (feb 0x00509030)
// st: this camera's flags per mirror, 2 seen by the main camera, 4 by this one, 8 the last one seen, 0x20 from behind.
int mrrMIRROR_SYSTEM::Render(camCAMERA *cam)
{
    u32 clear[10] = { 0 };                              // vidCLEAR
    camCAMERA c;
    u32 stCam[20] = { 0 }, stMain[20] = { 0 };
    if (!nMirrors)
        return 0;
    int on = 1;
    _apCfgReadBool(&_mrrCfgMirror, (char *)"Video", (char *)"VIDEO_Mirror", &on);
    depth++;
    cam->CalcPOVQual();
    if (depth == 1) {
        memcpy((void *)&mainCam, (void *)cam, sizeof(camCAMERA));
        PrepareNormals();
        _mrrCmpCam = cam;
        qsort(mirror, nMirrors, sizeof(mrrMIRROR), _mrrCmpMirror);
    }
    SetVisFlags(cam, depth == 2, stCam);
    SetVisFlags(&mainCam, 0, stMain);
    for (int i = 0; i < nMirrors; i++) {
        mrrMIRROR *m = &mirror[i];
        if (!(stCam[i] & 4))
            continue;
        memcpy((void *)&c, (void *)cam, sizeof(camCAMERA));
        int flip = depth == 1;                          // a picture seen once in a mirror is the other way round
        m3dV org, up, right, forw;
        if (m->flags & 1) {                             // a portal: the view turned and moved to its instance
            flip = 0;
            m3dV to, from, center;
            m3dMATR rot;
            m->actor->GetDir(&to);
            m3dV pos;
            m->actor->GetPos(&pos);
            from.x = -m->normal.x;
            from.y = -m->normal.y;
            from.z = -m->normal.z;
            wrVCall<void>(&m->obj->bbox, 0x24, &center);
            to.y = 0;
            from.y = 0;
            rot.MakeRot(&from, &to);
            cam->GetParameters(&org, &up, &right, &forw);
            m3dV up2, right2, forw2;
            mrrVecRot((float *)&rot, &up, &up2);
            mrrVecRot((float *)&rot, &right, &right2);
            mrrVecRot((float *)&rot, &forw, &forw2);
            c.SetParameters(&pos, &up2, &right2, &forw2);
        } else {
            cam->GetParameters(&org, &up, &right, &forw);
            m3dV org2, up2, right2, forw2;
            m3dMirrorPointPlane(&m->point, &m->normal, &org, &org2);
            m3dMirrorPointPlane(&m3dVZero, &m->normal, &up, &up2);
            m3dMirrorPointPlane(&m3dVZero, &m->normal, &right, &right2);
            right2.x = -right2.x;
            right2.y = -right2.y;
            right2.z = -right2.z;
            m3dMirrorPointPlane(&m3dVZero, &m->normal, &forw, &forw2);
            c.SetParameters(&org2, &up2, &right2, &forw2);
        }
        c.SetViewPort(mainCam.vpX, mainCam.vpY, mainCam.vpDx, mainCam.vpDy);
        if (flip)
            c.SetViewWindow(mainCam.winX + mainCam.winDx, mainCam.winY, -mainCam.winDx, mainCam.winDy);
        else
            c.SetViewWindow(mainCam.winX, mainCam.winY, mainCam.winDx, mainCam.winDy);
        if (depth == 1)
            curMirror = i;
        else if (curMirror == i)
            continue;
        if (m->flags & 1) {
            wrEVCall<void>(gsSysVis, 0x80, &c.pov.eye); // SetViewPos
            wrEVCall<void>(gsSysVis, 0x84, gsScenePtr); // SetVisInfo
        }
        rendDrv->flags |= 8;
        int pass = (m->flags & 0x40) ? 0x30 : 0x34;
        if (depth < 2) {
            if (on)
                wrEVCall<void>(gsSysRender, 0x1C, &c, pass);    // RenderFrame
        } else {
            m3dV n = m->normal;
            if (!(stCam[i] & 0x20)) {
                n.x = -n.x;
                n.y = -n.y;
                n.z = -n.z;
            }
            wrVCall<void>(rendDrv, 0x60, &m->point, &n);        // ConfigureClipPlane
            if (m->flags & 0x10)
                wrVCall<void>(rendDrv, 0x64, 1);                // EnableClipPlane
            if (on)
                wrEVCall<void>(gsSysRender, 0x1C, &c, pass | 8);
            RenderMirrorObjects(&c, 0, -1, 0, -1, i);
            wrVCall<void>(rendDrv, 0x64, 0);
        }
        wrVCall<void>(rendDrv, 0x64, 0);
        if (depth == 1)
            rendDrv->flags &= ~8u;
        if (on) {
            clear[0] |= 2;                              // the depth
            gsSysProjShd->UpdateClear(clear);
            wrVCall<void>(vidDriver, 0x24, clear);      // ClearBuffers
        }
        if (nMirrors > 1) {
            if (depth == 2) {
                MaskMirrors(&mainCam, stMain, 2, 1, curMirror, -1);
                RenderMirrorObjects(cam, 0, -1, 1, i, -1);
                MaskMirrors(cam, stCam, 4, 0, i + 1, curMirror);
            } else {
                MaskMirrors(&mainCam, stMain, 2, (stCam[i] & 8) ? 0 : 1, curMirror + 1, -1);
            }
        }
    }
    if (depth == 1) {
        RenderMirrorObjects(&mainCam, stMain, 2, on, -1, -1);
        wrVCall<void>(rendDrv, 0x64, 0);
    } else {
        mrrMIRROR *m = &mirror[curMirror];
        m3dV n = m->normal;
        if (!(stCam[curMirror] & 0x20)) {
            n.x = -n.x;
            n.y = -n.y;
            n.z = -n.z;
        }
        wrVCall<void>(rendDrv, 0x60, &m->point, &n);
        if (m->flags & 0x10)
            wrVCall<void>(rendDrv, 0x64, 1);
    }
    depth--;
    return 1;
}

// v12 0x0051EE00  (feb 0x00509B20)
// Each one's plane in the world: its first vertex and the sum of its faces' normals.
void mrrMIRROR_SYSTEM::PrepareNormals()
{
    for (int i = 0; i < nMirrors; i++) {
        mrrMIRROR *m = &mirror[i];
        objOBJ *o = m->obj;
        if (o->inst) {
            o->inst->root->SetStateProcYes(0x1000, 0);
            o->inst->Validate(0x203);
        }
        if (o->state & 1)
            m->point = o->geom->vert[0];
        else
            mrrPointL2W(o, &o->geom->vert[0], &m->point);
        m->normal = m3dVZero;
        for (int f = 0; f < o->nFace; f++) {
            m3dV *n = &o->geom->faceNormal[f];
            m->normal.x += n->x;
            m->normal.y += n->y;
            m->normal.z += n->z;
        }
        m3dNormalize(&m->normal);
    }
}

// v12 0x0051F090  (feb 0x00509DB0)
// Seen by this camera: shown, in a visible place, in its view and from the front (or two-sided, 0x20 from behind).
// The second level: also with a vertex in front of the mirror that is looking at it.
void mrrMIRROR_SYSTEM::SetVisFlags(camCAMERA *cam, int second, u32 *st)
{
    m3dV e;
    cam->GetOrigin(&e);
    for (int i = 0; i < nMirrors; i++) {
        mrrMIRROR *m = &mirror[i];
        objOBJ *o = m->obj;
        st[i] &= ~0x2Eu;
        if (o->procState & 0x3F)
            continue;
        if (!wrEVCall<int>(gsSysVis, 0x98, o))          // IsVisible
            continue;
        if (cam->QualBoxPOV((m3dBOX *)&o->bbox) == 1)
            continue;
        if ((e.y - m->point.y) * m->normal.y + (e.z - m->point.z) * m->normal.z + (e.x - m->point.x) * m->normal.x <=
            1e-6f * 10.0f) {
            if (!(o->state & 4))
                continue;
            st[i] |= 0x20;
        }
        if (!second) {
            st[i] |= 6;
            continue;
        }
        mrrMIRROR *cur = &mirror[curMirror];
        for (int v = 0; v < o->nVert; v++) {
            m3dV w;
            if (o->state & 1)
                w = o->geom->vert[v];
            else
                mrrPointL2W(o, &o->geom->vert[v], &w);
            if ((w.x - cur->point.x) * cur->normal.x + (w.y - cur->point.y) * cur->normal.y +
                (w.z - cur->point.z) * cur->normal.z > 1e-6f * 10.0f) {
                st[i] |= 6;
                break;
            }
        }
    }
    for (int i = nMirrors - 1; i >= 0; i--) {
        if (st[i] & 4) {
            st[i] |= 8;
            return;
        }
    }
}

// v12 0x0051F410  (feb 0x0050A130)
// The first 'count' mirrors of 'mask' into the depth buffer, in black, so that the next one does not draw over them.
void mrrMIRROR_SYSTEM::MaskMirrors(camCAMERA *cam, u32 *st, int mask, int polys, int count, int skip)
{
    rendCFG cfg;
    wrVCall<void>(rendDrv, 0x30, cam, 0.05f, 1300.0f);  // BeginCamera
    cfg.InitMtl1(0);
    for (int i = 0; i < count; i++) {
        if (!(st[i] & mask) || i == skip)
            continue;
        objOBJ *o = mirror[i].obj;
        if (!polys || i == 0) {
            o->_SetTransparency(0x80, 0);
            o->_SetConstColor(0, 0);
            mrrSetDrawColor(o);
            o->GetFaceTexRendMtl(0, 1);
            rendDrv->_RenderObj(o, 0);
        } else {
            cfg.flags = 0x80AA;
            for (int f = 0; f < o->nFace; f++) {
                m3dPOLY p;
                p.nVert = 3;
                objFACE *fc = o->GetFace(f);
                for (int j = 0; j < 3; j++) {
                    if (o->state & 1)
                        p.vert[j] = o->geom->vert[fc->vert[j]];
                    else
                        mrrPointL2W(o, &o->geom->vert[fc->vert[j]], &p.vert[j]);
                }
                rendDrv->RenderPoly(cam, &p, 0, &cfg);
            }
        }
    }
    wrVCall<void>(rendDrv, 0x34);                       // EndCamera
}

// v12 0x0051F810  (feb 0x0050A530)
// The mirrors themselves: their colour over what they show (white when mirrors are off), in the depth buffer.
void mrrMIRROR_SYSTEM::RenderMirrorObjects(camCAMERA *cam, u32 *st, int mask, int useColor, int only, int skip)
{
    int oldMode = *(u32 *)(*(char **)((char *)vidDriver + 0x210) + 0x14) & 0x800;
    wrVCall<void>(rendDrv, 0x30, cam, (rendDrv->flags & 0x400) ? 0.4f : 0.05f, 1300.0f);
    for (int i = 0; i < nMirrors; i++) {
        if (st && !(st[i] & mask))
            continue;
        if (only != -1 && i != only)
            continue;
        if (skip != -1 && i == skip)
            continue;
        mrrMIRROR *m = &mirror[i];
        objOBJ *o = m->obj;
        u32 c = 0xFFFFFFFF;
        if (useColor)
            c = (((int)m->color.a & 0xFF) << 24) | (((int)m->color.r & 0xFF) << 16) | (((int)m->color.g & 0xFF) << 8) |
                ((int)m->color.b & 0xFF);
        o->_SetConstColor(c, 0);
        mrrSetDrawColor(o);
        int q;
        rendDrv->RenderObj(o, 0, &q);
        rendDrv->_RenderObj(o, 0);
    }
    wrVCall<void>(rendDrv, 0x34);
    vidDriver->ToggleModeParam(0x800, oldMode);
}
