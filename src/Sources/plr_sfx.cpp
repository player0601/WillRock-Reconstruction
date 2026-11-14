// plr_sfx.cpp -- the player's own effects: blood on the hands, the chrome weapons (Will Rock 1.2, v12.exe; feb obj plr_sfx.obj)
//
// plrWPN_CHROME: an environment map on the held weapon (not its "RUKA_" hand objects, not "nospec" ones). 1.2: the
// texture shows the powerup -- "plr_wpn_tdmg" with titan damage (powerup 0), "plr_wpn_uarm" with immortality
// (powerup 1), else "plr_wpn_spec" -- and with either one the map turns (366.7 degrees a second).
// plrBLOOD_HANDS: a blood stain (wrsfxBLOOD_SURF -> pteSPOT_ELLIPSE -> pteSPOT_ANIM) on the held weapon's objects,
// started by blood within 5 m (NotifyBloodSource), fading in and out over 4 s in the floor's light.
// (Both draw through the CPU shader pass, pteMANAGER::ShadeFaceWCS, which the test renderer does not run yet.)

#include "game.h"
#include "../m3d/m3d_ctrl.h"
#include "../m3d/m3d_spl.h"
#include "../../include/engine_render/shader.h"

static txmTEXTURE *chromeTexSpec;                       // v12 0x006432A8 "plr_wpn_spec"
static txmTEXTURE *chromeTexTDmg;                       // v12 0x006432AC "plr_wpn_tdmg" (1.2)
static txmTEXTURE *chromeTexUArm;                       // v12 0x006432B0 "plr_wpn_uarm" (1.2)
static float chromeAngle;                               // v12 0x006432A0
static void *chromeDecompose3;                          // v12 0x006432B4 [Debug] Decompose3 (read cache)
extern txmTEXTURE *wrsfxBloodTexGround;                 // sfx_blood.cpp "sfx_blood_floor_1"
u32 __fastcall gsEffEnvirColor(cdtINFO *ci, m3dCOLOR *col, m3dCOLOR *dyn);  // gs_effect.cpp

static inline animINST *plrHeldInst() { return plrPlayer->weapons[plrPlayer->curWeapon]->inst; }

// ---------------------------------------------------------------------------------------------- plrWPN_CHROME

class plrWPN_CHROME : public pteEMAP {                  // size 0x110, vtable v12 0x005FB960
public:
    char *GetBlendTypeDef();                            // v12 0x0042E110 "add"
    void  GetMatrL2W(m3dMATR *m);                       // v12 0x0042E150
    int   IsShadeObj(objOBJ *o);                        // v12 0x0042E1D0
    int   IsShadeInst(animINST *a);                     // v12 0x0042E870 (shared with the hands)
    int   IsActive(int cam);                            // v12 0x0042E250
    void  PrepareFrame(scnSCENE *s, camCAMERA *c);      // v12 0x0042E300
    int   ShadeFace(objOBJ *o, int face, pteSDR_INFO *info, ptePOLY_DATA *out); // v12 0x0042E3F0 exact
    int   ShadeVert(objOBJ *o, int v, void *out);       // v12 0x0042E5F0 exact
};

// v12 0x0042E290: the held weapon's own objects, shown, with faces, not "nospec", not the hands ("RUKA_")
static BOOL32 __fastcall plrChromeIsShadeObj(objOBJ *o, void *)
{
    animINST *a = o->inst;
    if (!a || plrHeldInst() != a)
        return 0;
    if (o->procState & 0x3F)
        return 0;
    if (!o->nFace)
        return 0;
    if (strstr(o->name, "nospec"))
        return 0;
    return !strstr(o->name, "RUKA_");
}

char *plrWPN_CHROME::GetBlendTypeDef()
{
    return (char *)"add";
}

// v12 0x0042E150
// At the player; turning while titan damage or immortality is on.
void plrWPN_CHROME::GetMatrL2W(m3dMATR *m)
{
    m3dV p;
    m->Identity();
    plrPlayer->GetPos(&p);
    m->Translate(&p, 0);
    if (plrPlayer->IsActivePowerup(0) || plrPlayer->IsActivePowerup(1)) {
        chromeAngle = gsElapsedTime * 366.66666f + chromeAngle;
        m->RotateY(chromeAngle, 1);
    }
}

int plrWPN_CHROME::IsShadeObj(objOBJ *o)
{
    return plrChromeIsShadeObj(o, 0);
}

int plrWPN_CHROME::IsShadeInst(animINST *a)
{
    return plrHeldInst() == a;
}

// v12 0x0042E250
// On while the player's flags2 bit 2 is, unless [Debug] Decompose3.
int plrWPN_CHROME::IsActive(int)
{
    int v;
    if (_apCfgReadBool(&chromeDecompose3, (char *)"Debug", (char *)"Decompose3", &v) && v)
        return 0;
    return plrPlayer->flags2 & 2;
}

// v12 0x0042E300
// Its frame (and the inverse), the weapon's skin and vertex normals brought up to date, white at 80, the texture
// for the powerup on.
void plrWPN_CHROME::PrepareFrame(scnSCENE *, camCAMERA *)
{
    GetMatrL2W(&matrL2W);
    matrL2W.Invert(&matrW2L);
    animINST *a = plrHeldInst();
    objOBJ *root = a->root;
    objMOD_SKIN::StartApply(a->objTable);
    root->CalcVertNormals(plrChromeIsShadeObj);
    color[0] = color[1] = color[2] = 255.0f;
    color[3] = 80.0f;
    if (plrPlayer->IsActivePowerup(0))
        tex = chromeTexTDmg;
    else if (plrPlayer->IsActivePowerup(1))
        tex = chromeTexUArm;
    else
        tex = chromeTexSpec;
}

// v12 0x0042E3F0  (feb 0x004292E0) exact
// The smoothed normals in the map's frame: u, v = x, z x 0.5 + 0.25.
int plrWPN_CHROME::ShadeFace(objOBJ *o, int face, pteSDR_INFO *info, ptePOLY_DATA *out)
{
    int n = info->nVert;
    info->ValidateNormalSmooth(o, face);
    for (int i = 0; i < n; i++) {
        m3dV *v = &info->normal[i];
        float x = v->x * matrW2L.m[0][0] + v->y * matrW2L.m[1][0] + v->z * matrW2L.m[2][0];
        float z = v->x * matrW2L.m[0][2] + v->y * matrW2L.m[1][2] + v->z * matrW2L.m[2][2];
        out->uv[i].u = x * 0.5f + 0.25f;
        out->uv[i].v = z * 0.5f + 0.25f;
    }
    return 1;
}

// v12 0x0042E5F0  (feb 0x004294E0) exact
// A vertex: its normal in the map's frame, x, z / 3 + 0.25 (bytes +1 / +5 of the vertex data).
int plrWPN_CHROME::ShadeVert(objOBJ *o, int vi, void *d)
{
    m3dV *vn = o->geom->vertNormal;
    if (!vn)
        return 0;
    m3dV *v = &vn[vi];
    float x = v->x * matrW2L.m[0][0] + v->y * matrW2L.m[1][0] + v->z * matrW2L.m[2][0];
    float z = v->x * matrW2L.m[0][2] + v->y * matrW2L.m[1][2] + v->z * matrW2L.m[2][2];
    *(float *)((u8 *)d + 1) = x * 0.33333334f + 0.25f;
    *(float *)((u8 *)d + 5) = z * 0.33333334f + 0.25f;
    return 1;
}

// ---------------------------------------------------------------------------------------------- the blood

// wrsfxBLOOD_SURF (sfx_blood.obj, size 0x168): a pteSPOT_ELLIPSE of kind 0, radius 5, red (255, 50, 0), the ground
// blood texture, rendered 0xAF, switched off when its controllers stop (spotFlags 4), flags 0x40.
class wrsfxBLOOD_SURF : public pteSPOT_ELLIPSE {
public:
    wrsfxBLOOD_SURF();                                  // v12 0x0046B9F0 exact
    void SetParams(m3dV *pos, m3dV *dir, float time, m3dCOLOR *c); // v12 0x0046BAC0
};

wrsfxBLOOD_SURF::wrsfxBLOOD_SURF()
{
    kind = 0;                                           // (pteSPOT(0): no "lspot")
    texBase = texLM = 0;
    stretch = 1.0f;
    param2 = 0;
    *(u32 *)((u8 *)this + 0x14) = 0x424C5346;
    SetRadius(5.0f);
    m3dCOLOR c = { 255.0f, 50.0f, 0, 255.0f };
    SetColor(c, c);
    SetTex(wrsfxBloodTexGround);
    SetStateRend(0xAF);
    spotFlags |= 4;
    flags |= 0x40;
}

// v12 0x0046BAC0
// Along dir, stretched 0.2; a colour curve over `time`: in by 0.1 s, held to 0.9, out.
// PLACEHOLDER (quick rebuild): the curve's keys are plrBLOOD_HANDS's (same shape); check against v12.
void wrsfxBLOOD_SURF::SetParams(m3dV *pos, m3dV *dir, float time, m3dCOLOR *c)
{
    pteSPOT_ELLIPSE::SetParams(pos, dir, 0, 0.2f, 0);
    m3dSPL_COLOR *s = new m3dSPL_COLOR;                 // (fields as v12 0x0046BAE0 sets them: a 4D key {t, r, g, b, a})
    s->flags = 0;
    s->type = 8;
    s->nKp = 0;
    s->kpValueSize = 0x14;
    s->kpStride = 0x14;
    s->dim = 4;
    s->kpBytes = 0;
    s->kp = 0;
    s->pad[0] = s->pad[1] = s->pad[2] = 0;
    s->AllocKpList(4);
    s->SetKp(0, 0.0f, c->r, c->g, c->b, 0.0f);
    s->SetKp(1, 0.1f / time, c->r, c->g, c->b, 255.0f);
    s->SetKp(2, 0.9f, c->r, c->g, c->b, 255.0f);
    s->SetKp(3, 1.0f, c->r, c->g, c->b, 0.0f);
    m3dCTRL_COLOR_SPL *k = new m3dCTRL_COLOR_SPL;
    m3dCtrlTimeInit(k);
    k->length = time;
    k->spl = (m3dSPL *)s;
    k->flags |= 6;
    if (ctrlColorBase)
        ctrlColorBase->__vecDelDtor(1);
    SetCtrlColorBase(k);
}

class plrBLOOD_HANDS : public wrsfxBLOOD_SURF {         // size 0x16C, vtable v12 0x005FB8F0
public:
    float time;                                         // +0x168 4 s
    void *__vecDelDtor(unsigned f);                     // v12 0x0042E7C0 exact
    int   IsActive(int);                                // v12 0x0042E850
    int   IsShadeInst(animINST *a);                     // v12 0x0042E870
    int   IsShadeObj(objOBJ *o);                        // v12 0x0042E8A0
    void  Enable(int on);                               // v12 0x0042E910 exact
    void  Update(float dt);                             // v12 0x0042EDA0
    void  NotifyBloodSource(m3dV *src);                 // v12 0x0042E950
};

void *plrBLOOD_HANDS::__vecDelDtor(unsigned f)
{
    if (ctrlColorBase)
        ctrlColorBase->__vecDelDtor(1);
    ctrlColorBase = 0;
    return pteSPOT_ANIM::__vecDelDtor(f);
}

// not with blood off (wrAppState 2)
int plrBLOOD_HANDS::IsActive(int)
{
    return ((int)(char)~(u8)wrAppState & 2) >> 1;
}

int plrBLOOD_HANDS::IsShadeInst(animINST *a)
{
    return plrHeldInst() == a;
}

// the held weapon's objects (blood on)
int plrBLOOD_HANDS::IsShadeObj(objOBJ *o)
{
    if (!o->inst)
        return 0;
    if (wrAppState & 2)
        return 0;
    return plrHeldInst() == o->inst;
}

// off drops the colour curve
void plrBLOOD_HANDS::Enable(int on)
{
    pteSHADER::Enable(on);
    if (!on) {
        if (ctrlColorBase)
            ctrlColorBase->__vecDelDtor(1);
        ctrlColorBase = 0;
    }
}

static void plrBloodCurve(m3dSPL_COLOR *s, float time, float r, float g, float b)
{
    s->SetKp(0, 0.0f, r, g, b, 0.0f);
    s->SetKp(1, 0.1f / time, r, g, b, 255.0f);
    s->SetKp(2, 0.9f, r, g, b, 255.0f);
    s->SetKp(3, 1.0f, r, g, b, 0.0f);
}

// v12 0x0042EDA0  (feb 0x00429C60)
// The controllers; off: its face data dropped; on: the curve kept in the floor's light.
void plrBLOOD_HANDS::Update(float dt)
{
    pteSPOT_ANIM::Update(dt);
    if (!(flags & 1)) {
        if (staticData)
            delete staticData;
        staticData = 0;
        return;
    }
    u32 c = gsEffEnvirColor(&plrPlayer->floorInfo, 0, 0);
    if (ctrlColorBase)
        plrBloodCurve((m3dSPL_COLOR *)((m3dCTRL_COLOR_SPL *)ctrlColorBase)->spl, time,
                      (float)((c >> 16) & 0xFF), (float)((c >> 8) & 0xFF), (float)(c & 0xFF));
}

// v12 0x0042E950  (feb 0x004297F0)
// Off and the blood within 5 m (flat): on for 4 s at the player's eyes less 0.2, 0.1 forward, pushed from the
// source by 0.06 x the distance, in the floor's light; radius 0.3 for weapon kinds 1 and 3, else 0.5.
void plrBLOOD_HANDS::NotifyBloodSource(m3dV *src)
{
    if (flags & 1)
        return;
    m3dV p;
    plrPlayer->GetPos(&p);
    float dx = src->x - p.x, dz = src->z - p.z;
    float d = m3dSqrt(dx * dx + dz * dz);
    if (!(d <= 5.0f))
        return;
    Enable(1);
    time = 4.0f;
    m3dV away = { p.x - src->x, 0, p.z - src->z };
    if (!_m3dNormalize(&away)) {
        plrPlayer->GetDir(&away);
    } else {
        float k = d > 0.0f ? d * 0.06f : 0.0f;          // v12 0x005FB9D0
        p.x += away.x * -k;
        p.y += away.y * -k;
        p.z += away.z * -k;
    }
    m3dV dir;
    p.y += plrPlayer->eyeHeight - 0.2f;
    plrPlayer->GetDir(&dir);
    p.x += dir.x * 0.1f;
    p.y += dir.y * 0.1f;
    p.z += dir.z * 0.1f;
    u32 c = gsEffEnvirColor(&plrPlayer->floorInfo, 0, 0);
    m3dCOLOR col = { (float)((c >> 16) & 0xFF), (float)((c >> 8) & 0xFF), (float)(c & 0xFF), (float)(c >> 24) };
    SetParams(&p, &dir, time, &col);
    int kind = *(int *)((u8 *)plrPlayer->weapons[plrPlayer->curWeapon] + 0x94);
    SetRadius(kind == 3 || kind == 1 ? 0.3f : 0.5f);
    plrHeldInst()->Validate(1);
    // (pteSHADER::CalcStaticData(2), v12 0x004E0990: the faces it covers -- the renderer's, not rebuilt)
}

// ---------------------------------------------------------------------------------------------- the player

// v12 0x0042DF30  (feb 0x00428E50; 1.2: the two powerup textures)
int plrPLAYER::InitSFX()
{
    if (!(chromeTexSpec = txmManager->Add((char *)"plr_wpn_spec", 0x40000, 1)))
        return 0;
    if (!(chromeTexTDmg = txmManager->Add((char *)"plr_wpn_tdmg", 0x40000, 1)))
        return 0;
    if (!(chromeTexUArm = txmManager->Add((char *)"plr_wpn_uarm", 0x40000, 1)))
        return 0;
    plrWPN_CHROME *ch = new plrWPN_CHROME;
    ch->blendCoeff = 0.3f;
    bloodHands[1] = (int)ch;
    ch->tex = chromeTexSpec;
    ch->Register();
    ch->Enable(1);
    ch->SetInst(inst);
    txmTEXTURE *t = txmManager->Add((char *)"plr_blood_hands", 0x40000, 1);
    if (!t)
        return 0;
    plrBLOOD_HANDS *b = new plrBLOOD_HANDS;
    *(u32 *)((u8 *)b + 0x14) = 0x504C424C;
    bloodHands[0] = (int)b;
    if (!b)
        return 0;
    b->flags &= ~0x40u;
    b->SetTex(t);
    b->Register();
    b->Enable(0);
    b->SetInst(inst);
    b->colorLM.r = b->colorLM.g = b->colorLM.b = b->colorLM.a = 0;
    return 1;
}

// v12 0x0042E120  (feb 0x00428FF0)
// Blood from src (within 5 m) runs over the hands; no source: the effect switched off.
void plrPLAYER::NotifySFXBloodHands(m3dV *src)
{
    plrBLOOD_HANDS *b = (plrBLOOD_HANDS *)bloodHands[0];
    if (!b)
        return;                                         // (test: InitSFX is not called by the test's player set-up)
    if (!src) {
        b->Enable(0);
        return;
    }
    b->NotifyBloodSource(src);
}
