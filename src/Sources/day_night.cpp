// day_night.cpp -- day and night (Will Rock 1.2, v12.exe; feb obj day_night.obj)
//
// An instance with [DAY_NIGHT] isDayNight (the sky, as lev_1_1 / the 2002 coliseum) runs a cycle: dayTime (50 s) of
// day, then nightTime (80 s) of night, transTime (10 s) of fading at the day's two ends, from curTime (half the day).
// Its "SKY_DAY*" and "SKY_NIGHT*" objects leave the normal drawing (RenderSky draws them: the night one faded over the
// day one). The lightmaps change too: the animated lights ("_lm") whose script says [DAY_NIGHT] isDay / isNight give
// the day / night layer of their lightmap, weighted 1 - night / night. isFlare: a sun ("flare") along "Sun_path*"
// through the day. The game hears DAY (0x4D0) and NIGHT (0x4D1) when it turns (the fires light at night).
// Not rebuilt: the flare entity (flrFLARE, flare.obj; without it no sun).

#include "game.h"

void __fastcall flrFLARE_SetPos(flrFLARE *f, m3dV *p);  // v12 0x00556030 flrFLARE::SetPos -- not rebuilt

#pragma pack(push, 1)
struct dntLIGHTS {                                      // one of the two light groups (0x1DC)
    lgtLIGHT_GROUP group;                               // +0x000 a copy of the scene's group
    scnLIGHT      *first;                               // +0x1D4 its first light (+0xA8: its lightmap layer)
    txmTEXTURE_LAY *lay;                                // +0x1D8 the lightmap
};
class dntSYSTEM : public msgADDR {                      // size 0x4B0, vtable v12 0x00605C70
public:
    u32        unk24;                                   // +0x24 (the ctor clears it)
    u32        flags;                                   // +0x28 1 day told, 2 night told, 8 running
    objOBJ    *sunPath;                                 // +0x2C "Sun_path*"
    entENTITY *sun;                                     // +0x30 the "flare"
    m3dSPL    *sunColor;                                // +0x34 (never made)
    m3dSPL    *sunSpl;                                  // +0x38 Sun_path as a spline
    objOBJ    *sky[2];                                  // +0x3C "SKY_DAY*", +0x40 "SKY_NIGHT*"
    u8         _44[0xDC - 0x44];
    dntLIGHTS  day;                                     // +0x0DC
    dntLIGHTS  night;                                   // +0x2B8
    float      dayTime;                                 // +0x494
    float      nightTime;                               // +0x498
    float      transTime;                               // +0x49C
    float      curTime;                                 // +0x4A0
    float      phase;                                   // +0x4A4 0..1 through the day; 1 at night
    float      dayWeight;                               // +0x4A8
    float      nightWeight;                             // +0x4AC
    dntSYSTEM();
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);
    virtual BOOL32 InitInst(animINST *i);               // +0x10 v12 0x004A38E0
    virtual void   DestroyInst(animINST *i) {}          // +0x14 v12 0x0051AAF0 (empty)
    virtual BOOL32 ProcessINIT_LEVEL();                 // +0x18 v12 0x004A4140
    virtual void   ProcessTERM_LEVEL();                 // +0x1C v12 0x004A42F0
    void RenderSky(camCAMERA *cam);                     // v12 0x004A4380
};
#pragma pack(pop)
static_assert(sizeof(dntSYSTEM) == 0x4B0, "dntSYSTEM size");

dntSYSTEM *dntSystem;                                   // v12 0x0064FF28

static float dntLerp(float x, float x0, float x1, float y0, float y1)   // clamped, either way round
{
    if (x1 < x0) {
        float t = x0; x0 = x1; x1 = t;
        t = y0; y0 = y1; y1 = t;
    }
    if (x < x0)
        return y0;
    if (x > x1)
        return y1;
    return (x - x0) * (y1 - y0) / (x1 - x0) + y0;
}

// v12 0x004A37E0  (feb 0x004963E0)
int __fastcall dntInit()
{
    dntSystem = new dntSYSTEM;
    msgSystem.InsertAddr(dntSystem, 0, 0);
    return dntSystem != 0;
}

// v12 0x004A3820  (feb 0x00496420)
void __fastcall dntTerm()
{
    msgSystem.DestroyAddr(dntSystem);
}

// v12 0x004A3840  (feb 0x00496440)
dntSYSTEM::dntSYSTEM()
{
    unk24 = 0;
    classId = 0x24444E54;                               // '$DNT'
    priority = 0x46;
    flags = 0;
    sunPath = 0;
    sunSpl = 0;
    sun = 0;
    sunColor = 0;
    dayTime = 50.0f;
    memset(&day, 0, sizeof(day));
    memset(&night, 0, sizeof(night));
    phase = 0;
    curTime = 0;
    nightWeight = 0;
    dayWeight = 0;
    transTime = 0;
}

// v12 0x004A38E0  (feb 0x004964E0)  wrLVL_SYSTEM::InstLoadNotify, every instance
BOOL32 dntSYSTEM::InitInst(animINST *i)
{
    animCREATE_DATA cd;
    int on;
    if (i && i->GetInt((char *)"DAY_NIGHT", (char *)"isDayNight", &on) && on) {
        i->Validate(0x204);
        i->flags |= 0x10;
        dayTime = 50.0f;
        i->GetFloat((char *)"DAY_NIGHT", (char *)"dayTime", &dayTime);
        nightTime = 80.0f;
        i->GetFloat((char *)"DAY_NIGHT", (char *)"nightTime", &nightTime);
        transTime = 10.0f;
        i->GetFloat((char *)"DAY_NIGHT", (char *)"transTime", &transTime);
        curTime = dayTime * 0.5f;
        i->GetFloat((char *)"DAY_NIGHT", (char *)"curTime", &curTime);
        sky[0] = objFindNameN(i->root, (char *)"SKY_DAY", 7);
        sky[1] = objFindNameN(i->root, (char *)"SKY_NIGHT", 9);
        for (int k = 0; k < 2; k++)
            if (sky[k])
                sky[k]->SetStateProcYes(0x1008, 0);
    }
    if (i->GetInt((char *)"DAY_NIGHT", (char *)"isFlare", &on) && on) {
        m3dV p;
        sunPath = objFindNameN(i->root, (char *)"Sun_path", 8);
        if (!sunPath) {
            i->GetPos(&p);
        } else if ((sunSpl = objConvertSpl(sunPath)) != 0) {
            sunSpl->GetValue(0, &p, 0);
        } else {
            sunPath->GetOrigin(&p);
        }
        ((m3dMATR *)&cd.matr)->Translate(p.x, p.y, p.z, 0);
        strcpy(cd.name, "sun");
        sun = entCreate(gsScenePtr, (char *)"flare", (char *)"", &cd, 0, 0);
        if (!sun)
            return 0;
        flrFLARE_SetPos((flrFLARE *)sun, &p);
    }
    return 1;
}

// v12 0x004A3B00  (feb 0x00496700)
BOOL32 dntSYSTEM::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x3F4) {
        ProcessINIT_LEVEL();
        return 0;
    }
    if (id == 0x3F5) {
        ProcessTERM_LEVEL();
        return 0;
    }
    if (id == 2)
        return 1;
    if (id == 3) {
        if (sun == data)
            sun = 0;
        return 0;
    }
    if (id != 1000 || !(flags & 8))
        return 0;
    curTime += gsElapsedTime;
    float period = 1e-6f * 100.0f + nightTime + dayTime, t = curTime;
    if (!(fabsf(period) < 1e-6f))
        t = curTime - (float)floor(curTime / period) * period;
    phase = dntLerp(t, 0, dayTime, 0, 1.0f);
    float tr = transTime / dayTime;
    if (phase >= 0 && phase < tr)
        nightWeight = dntLerp(phase, 0, tr, 1.0f, 0);   // dawn
    else if (phase >= tr && phase < 1.0f - tr)
        nightWeight = 0;
    else if (phase >= 1.0f - tr)
        nightWeight = dntLerp(phase, 1.0f - tr, 1.0f, 0, 1.0f);   // dusk
    dayWeight = 1.0f - nightWeight;
    if (sun) {
        m3dV p;
        if (sunSpl) {
            sunSpl->GetValue((float)(sunSpl->nKp - 1) * phase, &p, 0);
            flrFLARE_SetPos((flrFLARE *)sun, &p);
            if (sunColor) {
                float c[4] = { 0, 0, 0, 0 };
                sunColor->GetValue(phase, c, 0);
                memcpy((u8 *)sun + 0x163, c, 16);
            }
        } else if (sunPath) {
            sunPath->inst->Validate(4);
            sunPath->GetOrigin(&p);
            flrFLARE_SetPos((flrFLARE *)sun, &p);
        }
    }
    if (day.lay)
        ((txmLAYER *)((u8 *)day.lay + 0x7C + (int)WR_FIELD(int, day.first, 0xA8) * 0x4C))->SetWeight(dayWeight);
    if (night.lay)
        ((txmLAYER *)((u8 *)night.lay + 0x7C + (int)WR_FIELD(int, night.first, 0xA8) * 0x4C))->SetWeight(nightWeight);
    if (phase > 0 && phase < 1.0f) {
        if (!(flags & 1)) {
            msgSystem.SendAll(0x4D0, 0, this);          // day
            flags = (flags | 1) & ~2u;
        }
    } else if (!(flags & 2)) {
        msgSystem.SendAll(0x4D1, 0, this);              // night
        flags = (flags | 2) & ~1u;
    }
    return 0;
}

// v12 0x004A4140  (feb 0x00496D40)
// The lights: the first group with a light whose script says isDay is the day's, isNight the night's. The day's
// layers start at weight 0.
BOOL32 dntSYSTEM::ProcessINIT_LEVEL()
{
    int n;
    lgtLIGHT_GROUP *g;
    txmTEXTURE_LAY **lay;
    if (getenv("WR_DNTLOG")) printf("dnt: INIT_LEVEL lights %d sel %d%c", gsScenePtr->nLights, gsScenePtr->nSel, 10), fflush(stdout);
    if (!gsScenePtr->GetAnimLights(&n, &g, &lay))
        return 0;
    for (int i = 0; i < n; i++) {
        if (!lay[i])
            continue;
        for (int j = 0; j < g[i].n; j++) {
            psSHEET *s = g[i].light[j]->sheet;
            if (!s)
                continue;
            int on;
            dntLIGHTS *to;
            if (s->GetInt((char *)"DAY_NIGHT", (char *)"isDay", &on) && on)
                to = &day;
            else if (s->GetInt((char *)"DAY_NIGHT", (char *)"isNight", &on) && on)
                to = &night;
            else
                continue;
            to->group = g[i];
            to->first = g[i].light[0];
            to->lay = lay[i];
            break;
        }
    }
    if (getenv("WR_DNTLOG")) printf("dnt: groups %d day %p night %p%c", n, (void *)day.lay, (void *)night.lay, 10), fflush(stdout);
    operator delete(lay);
    operator delete(g);
    flags |= 8;
    if (day.lay)
        for (int k = 0; k < WR_FIELD(int, day.lay, 0x78); k++)
            ((txmLAYER *)((u8 *)day.lay + 0x7C + k * 0x4C))->SetWeight(0);
    return 1;
}

// v12 0x004A42F0  (feb 0x00496EF0)
void dntSYSTEM::ProcessTERM_LEVEL()
{
    sunPath = 0;
    if (sunSpl)
        delete sunSpl;
    sunSpl = 0;
    if (sunColor)
        delete sunColor;
    sunColor = 0;
    sun = 0;
    sky[0] = sky[1] = 0;
    memset(&day, 0, sizeof(day));
    memset(&night, 0, sizeof(night));
    nightTime = 0;
    dayTime = 50.0f;
    phase = 0;
    curTime = 0;
    nightWeight = 0;
    dayWeight = 0;
    flags = 0;
}

// v12 0x004A4380  (feb 0x00496F80)  wrRENDER_SYSTEM::RenderSky
// The day sky unless the night covers it; over it the night one, faded in by nightWeight (and tinted to it when the
// object is drawn with its constant colour, 0x40).
void dntSYSTEM::RenderSky(camCAMERA *cam)
{
    u32 mode = WR_FIELD(u32, WR_FIELD(void *, vidDriver, 0x210), 0x14);
    vidDriver->ToggleModeParam(0x40800, 0);
    if ((!sky[1] || (sky[0] && nightWeight < 0.996f)) && sky[0])
        rendDrv->_RenderObj(sky[0], 0);
    if (sky[1] && nightWeight >= 1.0f / 255.0f) {
        sky[1]->SetTransparency(0x80, (u8)(int)(nightWeight * 255.0f));
        objGEOM *g = sky[1]->geom;
        u32 c = g->constColor;
        if ((sky[1]->rendState & 0x40) && (c >> 24) != 0xFF) {
            u32 a = (u32)(int)(nightWeight * 255.0f);
            c = (((a | 0xFFFFFF00) << 8 | (a & 0xFF)) << 8) | (a & 0xFF);
        }
        g->unk62 = c;
        rendDrv->_RenderObj(sky[1], 0);
    }
    vidDriver->ToggleModeParam(0x40000, mode & 0x40000);
    vidDriver->ToggleModeParam(0x800, mode & 0x800);
}

// the test's sky (no RenderSky there): which of the two the night weight shows (name ours)
objOBJ *__fastcall dntSkyShown()
{
    if (!dntSystem || !dntSystem->sky[0])
        return 0;
    return dntSystem->sky[1] && dntSystem->nightWeight >= 0.5f ? dntSystem->sky[1] : dntSystem->sky[0];
}
BOOL32 __fastcall dntInitInst(animINST *i)
{
    return dntSystem ? dntSystem->InitInst(i) : 0;
}
