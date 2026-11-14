// pte_spot.cpp -- CPU shader: a round light thrown on the faces near a point (Will Rock 1.2, v12.exe;
// feb obj engine:pte_spot.obj)
//
// pteSPOT: a light of `radius` around org. Each frame (PrepareFrame) the scene objects whose boxes come within the
// radius are marked; a marked object's faces (or a moving instance's, by its bounding volume) are shaded: the face's
// own frame (x along the face, y its normal) is worked out once per face and frame (the first shader to touch it),
// the spot is a circle of the radius cut by the face's plane, texture coordinates map that circle to the texture
// (0.5 +- 0.484), and the kind-1 spots ("lspot") fade with the distance. It also lights points (GetLightAt, which
// gsEffEnvirColor sums for the characters). Its base: pteSPOT_ANIM (pte_spot_a.cpp), wrsfxSCORCHMARK, the blood.
// 1.2: the same code as the beta.

#include <math.h>
#include "engine.h"
#include "../../include/engine_render/shader.h"
#include "../../include/math_app/camera.h"

int    __fastcall _m3xzIsIsectPolyCircle(m3dPOLY *poly, m3dV *c, float r);           // m3d_misc.cpp
void   __fastcall _m3xzDist2PointPoly(m3dV *p, m3dPOLY *poly, m3dV *closest, float *d); // m3d_misc.cpp
void   __fastcall m3dReOrientPoly(m3dPOLY *p);                                          // m3d_poly.cpp
int    __fastcall pteSpotAnimInit();                                                    // pte_spot_a.cpp

txmTEXTURE *pteSpotTex;                         // v12 0x00974DE4 "lspot"
static const float spotEps = 1e-6f;             // v12 0x00606F38
static float spotEps2;                          // v12 0x00974CE8 (static init v12 0x004D68A0: spotEps^2; unused)
static struct pteSPOT_STATIC_INIT { pteSPOT_STATIC_INIT() { spotEps2 = spotEps * spotEps; } } pteSpotStaticInit;

// the face being shaded, in its own frame (kept from the first shader to touch it: pteSDR_INFO::first)
static m3dMATR spotW2F;                         // v12 0x00974CA0 (feb 0x00930A90) world -> face
static m3dPOLY spotPoly;                        // v12 0x00974CF0 (feb 0x00930AD8) the face in its frame
static m3dV    spotAxisX;                       // v12 0x00974DE8 (feb 0x00930BD0) along the face
static m3dV    spotAxisZ;                       // v12 0x00974DF4 (feb 0x00930BDC) axisX x normal

static inline void spotXform(const m3dV *p, const m3dMATR *m, m3dV *o)
{
    float x = p->x, y = p->y, z = p->z;
    o->x = x * m->m[0][0] + m->m[3][0] + y * m->m[1][0] + z * m->m[2][0];
    o->y = x * m->m[0][1] + m->m[3][1] + y * m->m[1][1] + z * m->m[2][1];
    o->z = x * m->m[0][2] + m->m[3][2] + y * m->m[1][2] + z * m->m[2][2];
}

// ARGB of a colour (0..255 floats), as __ftol packs it
static inline u32 spotPack(const m3dCOLOR *c)
{
    u32 v = (u32)(int)c->a;
    v = (v << 8) | ((u32)(int)c->r & 0xFF);
    v = (v << 8) | ((u32)(int)c->g & 0xFF);
    v = (v << 8) | ((u32)(int)c->b & 0xFF);
    return v;
}

// v12 0x004D60F0  (feb 0x004C7110) exact
int __fastcall pteSpotInit()
{
    if (!(pteSpotTex = txmManager->Add((char *)"lspot", 0x40003, 1)))
        return 0;
    return pteSpotAnimInit() != 0;
}

// v12 0x004D6120  (feb 0x004C7140) exact
// Kind 1 takes the "lspot" texture; white (255) both ways, radius 15, rendered 0x6F, priority-ready (flags 0x10).
pteSPOT::pteSPOT(int k)
{
    kind = k;
    objMark = 0;
    colorLM.r = colorLM.g = colorLM.b = colorLM.a = 0;
    colorBase.r = colorBase.g = colorBase.b = colorBase.a = 0;
    spotFlags = 0;
    nExclude = 0;
    if (kind == 1) {
        texBase = pteSpotTex;
        texLM = pteSpotTex;
    }
    stateRend |= 0x6F;
    flags |= 0x10;
    radius = 15.0f;
    colorLM.r = colorLM.g = colorLM.b = colorLM.a = 255.0f;      // m3dCOLOR(0xFFFFFFFF)
    colorBase.r = colorBase.g = colorBase.b = colorBase.a = 255.0f;
    minLight = 0;
}

// v12 0x004D6250  (feb 0x004C7270) exact (the destructor v12 0x004D6290)
void *pteSPOT::__vecDelDtor(unsigned f)
{
    apFree(objMark);
    objMark = 0;
    return pteSHADER::__vecDelDtor(f);
}

// v12 0x004D62C0  (feb 0x004C72E0) exact
// "radius"; "color" sets both (the base at 0.7 of it, the light map's alpha 255), else "colorLM" / "colorBase".
void pteSPOT::Init()
{
    m3dCOLOR c = { 0, 0, 0, 0 }, lm, base;
    float r;
    if (ReadFloat((char *)"radius", &r))
        radius = r;
    lm = colorLM;
    base = colorBase;
    ReadColor((char *)"colorLM", &lm);
    ReadColor((char *)"colorBase", &base);
    c = colorBase;
    if (ReadColor((char *)"color", &c)) {
        colorLM.r = c.r;
        colorBase.r = c.r * 0.7f;
        colorLM.g = c.g;
        colorLM.b = c.b;
        colorLM.a = 255.0f;
        colorBase.g = c.g * 0.7f;
        colorBase.b = c.b * 0.7f;
        return;
    }
    colorLM = lm;
    colorBase = base;
}

// v12 0x004D6480  (feb 0x004C74A0)
// Additive (0x40) spots go later (priority 20), the others at 10.
void pteSPOT::SetStateRend(int s)
{
    stateRend = s;
    priority = (s & 0x40) ? 20 : 10;
}

// v12 0x004D64B0  (feb 0x004C74D0)
void pteSPOT::SetOrg(m3dV *p)
{
    org = *p;
}

// v12 0x00403980  (feb 0x00403650)
int pteSPOT::GetOrg(m3dV *p)
{
    *p = org;
    return 1;
}

// v12 0x004D6540  (feb 0x004C7560) exact
// A mark byte for each object of the level.
int pteSPOT::InitLevel(scnSCENE *s)
{
    objMark = (u8 *)apMalloc(s->nObj);
    return objMark != 0;
}

// v12 0x004D6570  (feb 0x004C7590)
void pteSPOT::TermLevel()
{
    apFree(objMark);
    objMark = 0;
}

// v12 0x004D6590  (feb 0x004C75B0) exact
// An excluded object going away leaves the list (the entries after it moved down; the count is not lowered, as in
// the game: the last entry stays twice).
int pteSPOT::DeleteObjNotify(objOBJ *o)
{
    int i;
    for (i = 0; i < nExclude && exclude[i] != o; i++)
        ;
    if (i == nExclude)
        return 0;
    for (i = 0; i < nExclude && exclude[i] != o; i++)
        ;
    if (i < nExclude && i < nExclude - 1)
        memmove(&exclude[i], &exclude[i + 1], (nExclude - i - 1) * 4);
    return 0;
}

// v12 0x004D6610  (feb 0x004C7630) exact
void pteSPOT::AddObjExclude(objOBJ *o)
{
    if (o && nExclude < 8)
        exclude[nExclude++] = o;
}

// v12 0x004D6640  (feb 0x004C7660) exact
// When on: a static object marked this frame, or a visible moving instance's (flags 0x48, not hidden 0x10) whose
// bounding volume comes within the radius -- not when in sight is lost (static), not switched off (state 0x20),
// not excluded.
int pteSPOT::IsShadeObj(objOBJ *o)
{
    animINST *a = o->inst;
    if (!(flags & 1))
        return 0;
    if (!a && (spotFlags & 1))
        return 0;
    if (o->state & 0x20)
        return 0;
    if (!a) {
        if (!objMark[o->idHier])
            return 0;
    } else {
        if (a->flags & 0x10)
            return 0;
        if (!(a->flags & 0x48))
            return 0;
        if (!a->bvCur->DistPoint(&org, radius, 0))
            return 0;
    }
    for (int i = 0; i < nExclude; i++)
        if (exclude[i] == o)
            return 0;
    return 1;
}

// v12 0x004D6710  (feb 0x004C7730)
int pteSPOT::IsShadeInst(animINST *a)
{
    return (a->flags & 0x48) != 0;
}

// v12 0x004D6730  (feb 0x004C7750)
// Too far (further than near plane x radius x 4600): out (flag 1); else in sight unless the camera's view misses it.
int pteSPOT::IsActive(int camArg)
{
    camCAMERA *cam = (camCAMERA *)camArg;
    m3dSPHERE_D sph;
    sph.type = 3;
    sph.vtbl = m3dSPHERE_vftable;
    m3dV eye;
    cam->GetOrigin(&eye);
    float dx = org.x - eye.x, dy = org.y - eye.y, dz = org.z - eye.z;
    float d = m3dSqrt(dx * dx + dy * dy + dz * dz);
    if (cam->zNear * radius * 4600.0f < d) {
        spotFlags |= 1;
        return 0;
    }
    spotFlags &= ~1u;
    sph.center = org;
    sph.radius = radius;
    return cam->QualSpherePOV((m3dSPHERE *)&sph) != 1;
}

// v12 0x004D68C0  (feb 0x004C78F0) exact
// In sight (or no camera): the level's objects round the spot (the collision grid's list), each marked when its box
// comes within the radius.
void pteSPOT::PrepareFrame(scnSCENE *s, camCAMERA *c)
{
    if (!c)
        spotFlags &= ~1u;
    if (spotFlags & 1)
        return;
    cdtOFL *ofl = *(cdtOFL **)((char *)pteManager + 0x1794C);
    ((cdtCOLL *)s->coll)->MakeIndList(&org, &org, radius, ofl, 0);
    memset(objMark, 0, s->nObj);
    int n = ofl->nObj;
    u16 *ids = ofl->obj;
    for (int i = 0; i < n; i++) {
        float d;
        objOBJ *o = s->objList[ids[i]];
        if (((m3dVOL *)&o->bbox)->DistPoint(&org, radius, 0, &d, 0))
            objMark[ids[i]] = 1;
    }
}

// v12 0x004D69A0  (feb 0x004C79D0)
// A moving instance's normals brought up to date first.
int pteSPOT::ShadeFacePrepare(void *p)
{
    objOBJ *o = (objOBJ *)p;
    if (o->inst)
        o->inst->Validate(1);
    return 1;
}

// v12 0x004D69C0  (feb 0x004C79F0)
// The spot on one face: none when the face's plane is further than the radius; the circle where the light's sphere
// cuts the plane must touch the face (in the face's own frame, x/z); kind 1 with a threshold above 1: none when the
// face's nearest point is lit less than it; then the texture coordinates of each vertex (the circle -> 0.5 +-0.484),
// and for kind 1 the intensity by the height over the face ((1/255) ^ (h / radius)^2).
int pteSPOT::ShadeFace(objOBJ *o, int face, pteSDR_INFO *info, ptePOLY_DATA *out)
{
    float r = radius;
    m3dV p;
    GetOrg(&p);
    m3dV *n = &o->geom->faceNormal[face];
    float h = (p.x - info->vert[0].x) * n->x + (p.y - info->vert[0].y) * n->y + (p.z - info->vert[0].z) * n->z;
    if (fabsf(h) > r)
        return 0;
    if (info->first) {
        float k = m3dVUnitX.y * n->y + m3dVUnitX.z * n->z + m3dVUnitX.x * n->x;
        spotAxisX.x = m3dVUnitX.x - k * n->x;
        spotAxisX.y = m3dVUnitX.y - k * n->y;
        spotAxisX.z = m3dVUnitX.z - k * n->z;
        if (!_m3dNormalize(&spotAxisX)) {
            k = m3dVUnitY.y * n->y + m3dVUnitY.z * n->z + m3dVUnitY.x * n->x;
            spotAxisX.x = m3dVUnitY.x - k * n->x;
            spotAxisX.y = m3dVUnitY.y - k * n->y;
            spotAxisX.z = m3dVUnitY.z - k * n->z;
            if (!_m3dNormalize(&spotAxisX))
                return 0;
        }
        spotAxisZ.x = spotAxisX.y * n->z - spotAxisX.z * n->y;
        spotAxisZ.y = spotAxisX.z * n->x - spotAxisX.x * n->z;
        spotAxisZ.z = spotAxisX.x * n->y - spotAxisX.y * n->x;
        m3dV at;
        if (!(o->state & 1))                            // local vertices: the first one into the world
            spotXform(&o->geom->vert[o->GetFace(face)->vert[0]], (m3dMATR *)o->ltm, &at);
        else
            at = o->geom->vert[o->GetFace(face)->vert[0]];
        spotW2F.MakeWCS2LCS(&at, &spotAxisX, n, &spotAxisZ);
        for (int i = 0; i < info->nVert; i++)
            spotXform(&info->vert[i], &spotW2F, &spotPoly.vert[i]);
        spotPoly.nVert = info->nVert;
        MarkFaceFirstShaded();
    }
    GetOrg(&p);
    m3dV l;
    spotXform(&p, &spotW2F, &l);
    float d2 = r * r - l.y * l.y;
    if (!(d2 >= spotEps))
        return 0;
    float cr = m3dSqrt(d2);
    BOOL32 inside = m3xzIsBelongPointPoly_A(&spotPoly, &l, spotEps);
    if (!inside && !_m3xzIsIsectPolyCircle(&spotPoly, &l, cr))
        return 0;
    if (kind == 1 && minLight > 1.0f) {
        float dd;
        if (!inside) {
            _m3xzDist2PointPoly(&l, &spotPoly, 0, &dd);
            dd = l.y * l.y + dd;
        } else
            dd = l.y * l.y;
        float t = m3dSqrt(dd) / r * 64.0f;
        float k = (float)pow(1.0 / 255.0, (double)(t * t * 0.000125f));
        if (!(k >= 0.0f))
            k = 0;
        else if (k > 1.0f)
            k = 1.0f;
        if (k * colorLM.r < minLight && k * colorLM.g < minLight && k * colorLM.b < minLight &&
            k * colorBase.r < minLight && k * colorBase.g < minLight && k * colorBase.b < minLight)
            return 0;
    }
    float s = 0.484375f / cr;
    float u0 = 0.5f - l.x * s;
    float v0 = 0.5f - l.z * s;
    for (int i = 0; i < info->nVert; i++) {
        out->uv[i].u = spotPoly.vert[i].x * s + u0;
        out->uv[i].v = spotPoly.vert[i].z * s + v0;
    }
    if (kind == 1) {
        float u = l.y / r;
        out->intensity = (float)pow(1.0 / 255.0, (double)(u * u));
    }
    return 1;
}

// v12 0x004D7400  (feb 0x004C8430) exact
// Its render state for the light-mapped faces (0) / the others (1): additive spots (0x40) with a black colour do
// nothing (on the light maps only when the driver cannot add without them: caps 0x40000000).
int pteSPOT::GetStateRend(int n, int *out)
{
    u32 s = stateRend;
    if (n == 0) {
        if (!(s & 0x40))
            return 0;
        if (!spotPack(&colorLM) && !(*(u32 *)(*(u8 **)((u8 *)vidDriver + 0x210) + 0x14) & 0x40000000))
            return 0;
    } else if (s & 0x40) {
        if (!spotPack(&colorBase))
            return 0;
    }
    *out = s;
    return 1;
}

// v12 0x004D7500  (feb 0x004C8530)
txmTEXTURE *pteSPOT::GetTex(int n, objOBJ *, int)
{
    return n == 0 ? texLM : texBase;
}

// v12 0x004D7520  (feb 0x004C8550) exact
float *pteSPOT::GetColor(float *out, int n)
{
    m3dCOLOR *c = n == 0 ? &colorLM : &colorBase;
    out[0] = c->r;
    out[1] = c->g;
    out[2] = c->b;
    out[3] = c->a;
    return out;
}

// v12 0x004D7560  (feb 0x004C8590)
// The light it gives a point: its colour (n), with a radius of at least 0.01 fading from 1 at the centre to 0 at
// the radius (a negative radius: from 0 at it to 1 at the centre); smaller: all of it inside, none outside.
int pteSPOT::GetLightAt(m3dV *p, int n, m3dCOLOR *out)
{
    int st;
    if (!GetStateRend(n, &st))
        return 0;
    float dx = org.x - p->x, dy = org.y - p->y, dz = org.z - p->z;
    float d = m3dSqrt(dx * dx + dy * dy + dz * dz);
    m3dCOLOR c;
    if (!(radius >= 0.01f)) {
        if (!(d < radius))
            return 0;
        GetColor((float *)&c, n);
        *out = c;
        return 1;
    }
    float x0 = 0, v0 = 1.0f, x1 = radius, v1 = 0;
    if (!(x1 > 0.0f)) {
        x0 = radius;
        x1 = 0;
        v0 = 0;
        v1 = 1.0f;
    }
    float k;
    if (d < x0)
        k = v0;
    else if (d > x1)
        k = v1;
    else {
        float xa = x0, va = v0, xb = x1, vb = v1;
        if (x1 < x0) {
            xa = x1; va = v1; xb = x0; vb = v0;
        }
        k = (vb - va) * (d - xa) / (xb - xa) + va;
    }
    GetColor((float *)&c, n);
    out->r = k * c.r;
    out->g = k * c.g;
    out->b = k * c.b;
    out->a = k * c.a;
    return 1;
}

// v12 0x004D7880  (feb 0x004C88C0) exact
// The level's objects within its greatest radius.
void pteSPOT::MakeObjIndList(cdtOFL *ofl)
{
    scnSCENE *s = *(scnSCENE **)((char *)pteManager + 0x1793C);
    ((cdtCOLL *)s->coll)->MakeIndList(&org, &org, GetMaxRadius(), ofl, 0);
}

// v12 0x004D78C0  (feb 0x004C8900) exact
// A face (its triangle in the world) within the greatest radius, on either side.
int pteSPOT::IsIncludeFaceStaticData(objOBJ *o, int face)
{
    float r = GetMaxRadius();
    m3dPOLY tri;
    tri.nVert = 3;
    objFACE *f = o->GetFace(face);
    for (int i = 0; i < 3; i++) {
        if (!(o->state & 1))
            spotXform(&o->geom->vert[f->vert[i]], (m3dMATR *)o->ltm, &tri.vert[i]);
        else
            tri.vert[i] = o->geom->vert[f->vert[i]];
    }
    m3dV *n = &o->geom->faceNormal[face];
    if (!m3dDistPointPoly(&org, &tri, n, 0, 0, 0, r)) {
        m3dReOrientPoly(&tri);
        m3dV nn = { n->x * -1.0f, n->y * -1.0f, n->z * -1.0f };
        if (!m3dDistPointPoly(&org, &tri, &nn, 0, 0, 0, r))
            return 0;
    }
    return 1;
}

// v12 0x004D7BA0  (feb 0x004C8BE0)
float pteSPOT::GetMaxRadius()
{
    return radius;
}
