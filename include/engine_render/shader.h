// Will Rock 1.2 unofficial SDK - engine render: "pte" per-face/vertex shaders (CPU effects: env maps, spot lights,
// ground fog, texture swap, caustics, portal view). These are NOT GPU shaders: they recolour/retexture faces and verts
// of objects before the driver draws them.
// objs: engine:pte_shader.obj, pte_envmap.obj, pte_fog.obj, pte_spot*.obj, pte_tex2.obj, pte_sdr_static.obj
// Assumes from re/sdk/core: objOBJ, animINST, psSHEET (property sheet / script block), camCAMERA, m3dV, m3dCOLOR, scnSCENE
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include <stdint.h>

class pteMANAGER;
extern pteMANAGER *pteManager;          // v12 0x00974E10 (feb 0x00930BF8) created by pteInit, 71 xrefs
extern unsigned    pteSHADER_faceUID;   // v12 0x00974E18 (feb 0x00930C00) pteSHADER::faceUID (static)
extern int         pteClassCount;       // v12 0x00974E14 (feb 0x00930BFC) number of registered shader classes
extern uint8_t     pteClassTable[];     // v12 0x00974E20 (feb 0x00930C08) registered classes {name, priority, factory}

typedef class pteSHADER *(__fastcall *pteFACTORY)(animINST *, objOBJ *);

// Registered shader classes (name used in object/instance property sheets "type", priority, factory)
//   "emap_chrome" 0x1E, "emap_specular" 0x28, "emap_envir" 0x1E, "emap_panel" 0x1E, "emap_reflect" 0x23, "emap_refract" 0x1E,
//   "fog" 0x32 (pteFOG_GROUND), "spot" 0x14 (pteSPOT_ANIM), "_caustic" 0x2D (pteCAUSTICS), "wrsfx_portal" 0x1E (game: wrsfxPORTAL_VIEW)
// Config strings: "PTE_SHADER", "PTE_SHADER%d", "VIDEO_UseShaders" (pte_shader.obj)

int __fastcall  pteInit();   // v12 0x004D8370 (feb 0x004C93B0) exact: new pteMANAGER (0x17958 bytes) + cdtOFL(500,3000)
void __fastcall pteTerm();   // v12 0x004D8420 (feb 0x004C9460) exact

class pteMANAGER {
public:
    // vtable v12 0x00606FE4 (feb 0x005DBB4C): only the deleting dtor (v12 0x004D8460)
    int  RegisterClass(char *name, int priority, pteFACTORY); // v12 0x004D8480 (feb 0x004C94C0) exact. HOOK/MOD: add a new shader type
    void CreateShader(animINST *);                       // v12 0x004D8650 (feb 0x004C9690) exact  from the instance's sheet
    pteSHADER *CreateShader(class psSHEET *sh, int n, animINST *a, objOBJ *o); // v12 0x004D8520 (feb 0x004C9560) exact
    void CreateShader(objOBJ *);                         // v12 0x004D86D0 (feb 0x004C9710) exact
    int  InitLevel(scnSCENE *);                          // v12 0x004D8750 (feb 0x004C9790) exact
    void TermLevel();                                    // v12 0x004D87B0 (feb 0x004C97F0) exact
    void DeleteInstNotify(animINST *);                   // v12 0x004D8820 (feb 0x004C9860) exact
    void DeleteObjNotify(objOBJ *);                      // v12 0x004D8870 (feb 0x004C98B0) exact
    BOOL32 FindShaderAround(int n, int *sigs, m3dV *p, float r); // v12 0x004D9360 (feb 0x004CA3A0) one of these kinds within r
    pteSHADER *FindNextShader(pteSHADER *);              // v12 0x004D88E0 (feb 0x004C9920) exact
    void RemoveShader(pteSHADER *);                      // v12 0x004D8910 (feb 0x004C9950) exact
    void UpdateAllShaders(float dt);                     // v12 0x004D89A0 (feb 0x004C99E0) exact
    void ActivateAllShaders(camCAMERA *);                // v12 0x004D8A80 (feb 0x004C9AC0) exact  (per frame, RenderFrame)
    void PrepareFrame(scnSCENE *, camCAMERA *);          // v12 0x004D8B60 (feb 0x004C9BA0) exact  (per frame, RenderFrame)
    int  IsShadeInst(animINST *);                        // v12 0x004D9030 (feb 0x004CA070) exact
    void PrepareObj(objOBJ *);                           // v12 0x004D9070 (feb 0x004CA0B0)
    int  ShadeFaceWCS(objOBJ *, int, int, struct ptePOLY_DATA *); // v12 0x004D94E0 (feb 0x004CA520) exact
    int  ShadeVert(objOBJ *, int, int, struct pteVERT_DATA *);    // v12 0x004D9850 (feb 0x004CA890) exact

    // size 0x17958 (operator new in pteInit). Fields (feb == v12, pteInit exact):
    //   +0x1F44 nShaders (ctor 0, read by pteSHADER ctor), +0x1F48, +0x17938..+0x17954 misc, +0x1794C cdtOFL*, +0x17944 initialised flag
    //   +0x0004 pteSHADER *list[] (FindNextShader v12 0x004D88F7); a shader's own index is at shader +0x80, flags at +0x04 (1 = on)
};

struct pteSDR_STATIC_DATA {                                       // per-shader face / vertex data (pte_sdr_static.obj)
    ~pteSDR_STATIC_DATA();                                       // v12 0x004E08E0 (not rebuilt)
};

class pteSHADER {
public:
    // vtable v12 0x00606FE8 (feb 0x005DBB50), 25 slots (bases' bodies: shared one-liners)
    virtual void *__vecDelDtor(unsigned flags);                  // slot +0x00 v12 0x004D8C90 (feb 0x004C9CD0)
    virtual void Enable(int on);                                 // slot +0x04 v12 0x004D8EB0 (feb 0x004C9EF0) flags bit 1
    virtual class txmTEXTURE *GetTex(int face, objOBJ *, int);   // slot +0x08 v12 0x0041C180 (feb 0x00419720) its texture
    virtual int  GetLightAt(m3dV *p, int, m3dCOLOR *out);         // slot +0x0C v12 0x00403970 (feb 0x00403640) the light this
                                                                 //   shader adds at a point (gsEffEnvirColor sums them); base: none
    virtual float *GetColor(float *out, int);                    // slot +0x10 v12 0x0041C190 (feb 0x00419730) its colour
    virtual void SetOrg(m3dV *);                                 // slot +0x14 base nothing (pteSPOT override)
    virtual int  GetOrg(m3dV *);                                 // slot +0x18 base 0
    virtual void SetStateRend(int);                              // slot +0x1C v12 0x0041C1C0 (feb 0x00419760) +0x7C
    virtual int  GetStateRend(int which, int *out);              // slot +0x20 v12 0x0041C1D0 (feb 0x00419770) +0x7C, which == 1
    virtual int  _24();                                          // slot +0x24 v12 0x0048DF30 (feb 0x00403680) 0
    virtual char *GetName();                                     // slot +0x28 v12 0x004D8FA0 (feb 0x004C9FE0) "object::texture"
    virtual void Init();                                         // slot +0x2C v12 0x004D8D20 (feb 0x004C9D60) read params from the sheet
    virtual int  IsShadeObj(objOBJ *) = 0;                       // slot +0x30
    virtual int  IsShadeInst(animINST *) = 0;                    // slot +0x34
    virtual int  ShadeFacePrepare(void *);                       // slot +0x38 base 0
    virtual int  ShadeFace(objOBJ *o, int face, struct pteSDR_INFO *info, struct ptePOLY_DATA *out) = 0; // slot +0x3C
    virtual int  ShadeVert(objOBJ *, int, void *);               // slot +0x40 base 0
    virtual int  IsActive(int) = 0;                              // slot +0x44
    virtual void Update(float dt);                               // slot +0x48 base nothing
    virtual void PrepareFrame(scnSCENE *, camCAMERA *);          // slot +0x4C v12 0x00403860 base nothing
    virtual int  InitLevel(scnSCENE *);                          // slot +0x50 base 1 (v12 0x0048D590)
    virtual void TermLevel();                                    // slot +0x54 base nothing
    virtual int  DeleteInstNotify(animINST *);                   // slot +0x58 v12 0x004D8F40 (feb 0x004C9F80)
    virtual int  DeleteObjNotify(objOBJ *);                      // slot +0x5C v12 0x004D8F70 (feb 0x004C9FB0)
    virtual void MakeObjIndList(struct cdtOFL *ofl);             // slot +0x60 v12 0x004E0920 (feb 0x004D2690) (not rebuilt)

    pteSHADER();                                                 // v12 0x004D8BB0 (feb 0x004C9BF0) exact; into the manager's list
    int  ReadStr(char *key, char *out, int n);                   // v12 0x004D8DA0 (feb 0x004C9DE0) exact  sheet helpers for Init()
    int  ReadFloat(char *key, float *);                          // v12 0x004D8DE0 (feb 0x004C9E20)
    int  ReadBool(char *key, int *);                             // v12 0x004D8E10 (feb 0x004C9E50)
    int  ReadColor(char *key, m3dCOLOR *);                       // v12 0x004D8E40 (feb 0x004C9E80)
    int  Register();                                             // v12 0x004D8E70 (feb 0x004C9EB0) exact
    void SetSheet(psSHEET *, char *sect);                        // v12 0x004D8ED0 (feb 0x004C9F10) exact
    void SetInst(animINST *);                                    // v12 0x004D8F00 flags |= 4
    void SetObj(objOBJ *);                                       // v12 0x004D8F20 flags |= 8
    void MarkFaceFirstShaded();                                  // v12 0x004D9010 (feb 0x004CA050) faceUID = the manager's +0x17954

    // data (ctor exact in v12)
    u32      flags;        // v12 +0x04 1 on (Enable), 2 registered / not in the list, 4 own instance, 8 own object, 0x100 kept
    int      priority;     // v12 +0x08 ctor = 100
    int      frameNmb;     // v12 +0x0C the message frame it was made in
    int      _10, _14;     // v12 +0x10
    psSHEET *sheet;        // v12 +0x18 (SetSheet)
    char     sect[0x40];   // v12 +0x1C its section in the sheet ("PTE_SHADER" at first)
    float    _5c;          // v12 +0x5C ctor = -1.0
    animINST *inst;        // v12 +0x60
    objOBJ  *obj;          // v12 +0x64
    class txmTEXTURE *tex; // v12 +0x68 ("tex" key)
    float    color[4];     // v12 +0x6C ctor = 255,255,255,255 ("color")
    int      stateRend;    // v12 +0x7C
    int      id;           // v12 +0x80 its index in the manager's list, -1 none
    float    distOff;      // v12 +0x84 ctor = -1.0 ("distOff")
    pteSDR_STATIC_DATA *staticData; // v12 +0x88
};

// The face being shaded (pteMANAGER::ShadeFaceWCS fills it): its world vertices; first = the first shader to touch
// it this frame (the shader then works out the face's own frame and keeps it for the next shaders: MarkFaceFirstShaded).
struct pteSDR_SPLIT { int clipped; u8 _04[0xF4]; float a[8]; float b[8]; }; // the clip of a face: barycentric a/b
struct pteSDR_INFO {
    pteSDR_SPLIT *split;           // +0x000
    int    nVert;                  // +0x004 (with vert: an m3dPOLY)
    m3dV   vert[20];               // +0x008 world
    m3dV   normal[20];             // +0x0F8 smoothed (ValidateNormalSmooth)
    u8     _1e8[0x30];
    int    first;                  // +0x218
    int    _21c;
    u32    valid;                  // +0x220 1 = normals made
    void   ValidateNormalSmooth(objOBJ *o, int face);   // v12 0x004D98F0 (feb 0x004CA930) exact
};
// What a shader gives the face: an intensity and texture coordinates for each vertex.
struct ptePOLY_DATA {
    float  intensity;              // +0x00
    struct { float u, v; } uv[20]; // +0x04
};

// pteSPOT (pte_spot.cpp): a round light of `radius` around org thrown on the faces near it, textured ("lspot" by
// default when the kind is 1), in colorLM (light-mapped faces, n 0) / colorBase (n 1). Also lights points
// (GetLightAt: gsEffEnvirColor sums them). Scene objects within the radius are marked each frame (PrepareFrame);
// up to 8 objects can be left out (AddObjExclude).
class pteSPOT : public pteSHADER {                               // size 0xF8
public:
    int        kind;               // +0x08C ctor argument (1: the "lspot" texture, a fade-out by distance)
    m3dV       org;                // +0x090
    u8        *objMark;            // +0x09C one byte per scene object (InitLevel: scene nObj bytes)
    class txmTEXTURE *texLM;       // +0x0A0 GetTex(0)
    class txmTEXTURE *texBase;     // +0x0A4 GetTex(1)
    m3dCOLOR   colorLM;            // +0x0A8 ("colorLM"; "color")
    m3dCOLOR   colorBase;          // +0x0B8 ("colorBase"; "color" x 0.7)
    float      radius;             // +0x0C8 ("radius", 15)
    objOBJ    *exclude[8];         // +0x0CC
    int        nExclude;           // +0x0EC
    float      minLight;           // +0x0F0 kind 1: faces lit less than this are skipped (only when above 1)
    u32        spotFlags;          // +0x0F4 1 = out of sight (IsActive)

    pteSPOT(int kind);                                           // v12 0x004D6120 (feb 0x004C7140) exact
    void *__vecDelDtor(unsigned flags);                          // v12 0x004D6250 (feb 0x004C7270) exact
    txmTEXTURE *GetTex(int n, objOBJ *, int);                    // v12 0x004D7500
    int    GetLightAt(m3dV *p, int n, m3dCOLOR *out);            // v12 0x004D7560 (feb GetColor(m3dV*, int, m3dCOLOR*))
    float *GetColor(float *out, int n);                          // v12 0x004D7520 (feb 0x004C8550) exact
    void   SetOrg(m3dV *p);                                      // v12 0x004D64B0
    int    GetOrg(m3dV *p);                                      // v12 0x00403980
    void   SetStateRend(int s);                                  // v12 0x004D6480
    int    GetStateRend(int n, int *out);                        // v12 0x004D7400 (feb 0x004C8430) exact
    void   Init();                                               // v12 0x004D62C0 (feb 0x004C72E0) exact
    int    IsShadeObj(objOBJ *o);                                // v12 0x004D6640 (feb 0x004C7660) exact
    int    IsShadeInst(animINST *a);                             // v12 0x004D6710
    int    ShadeFacePrepare(void *o);                            // v12 0x004D69A0
    int    ShadeFace(objOBJ *o, int face, pteSDR_INFO *info, ptePOLY_DATA *out); // v12 0x004D69C0 (feb 0x004C79F0)
    int    IsActive(int cam);                                    // v12 0x004D6730 (camCAMERA *)
    void   PrepareFrame(scnSCENE *s, camCAMERA *c);              // v12 0x004D68C0 (feb 0x004C78F0) exact
    int    InitLevel(scnSCENE *s);                               // v12 0x004D6540 (feb 0x004C7560) exact
    void   TermLevel();                                          // v12 0x004D6570
    int    DeleteObjNotify(objOBJ *o);                           // v12 0x004D6590 (feb 0x004C75B0) exact
    void   MakeObjIndList(struct cdtOFL *ofl);                   // v12 0x004D7880 (feb 0x004C88C0) exact
    virtual float GetMaxRadius();                                // [25] +0x64 v12 0x004D7BA0
    virtual int   IsIncludeFaceStaticData(objOBJ *o, int face);  // [26] +0x68 v12 0x004D78C0 (feb 0x004C8900) exact
    void   SetRadius(float r)          { radius = r; }           // v12 0x004D64A0
    void   SetColor(m3dCOLOR lm, m3dCOLOR base) { colorLM = lm; colorBase = base; } // v12 0x004D64D0
    void   SetTex(txmTEXTURE *t)       { texBase = t; texLM = t; } // v12 0x004D6520
    void   AddObjExclude(objOBJ *o);                             // v12 0x004D6610 (feb 0x004C7630) exact
};
// pteSPOT_ANIM ("spot", pte_spot_a.cpp): a pteSPOT whose radius, position and colours follow controllers (each
// Update); a controller that stops (not running) switches the shader off when spotFlags & 4. Sheet: "ctrl_pos" =
// "follow" (its instance, or "object" of it), "ctrl_rad" = "noise" (radMin 0.5 .. radMax 2, radFreq 1).
class pteSPOT_ANIM : public pteSPOT {                            // size 0x108, vtable v12 0x005F92B0
public:
    class m3dCTRL_1D    *ctrlRad;        // +0x0F8
    class m3dCTRL_3D    *ctrlPos;        // +0x0FC
    class m3dCTRL_COLOR *ctrlColorLM;    // +0x100
    class m3dCTRL_COLOR *ctrlColorBase;  // +0x104
    pteSPOT_ANIM() : pteSPOT(1) { ctrlRad = 0; ctrlPos = 0; ctrlColorLM = 0; ctrlColorBase = 0; }
    static pteSHADER *__fastcall Create(animINST *, objOBJ *);  // v12 0x004D7BD0 (feb 0x004C8C10) exact
    void *__vecDelDtor(unsigned flags);                          // v12 0x004039B0
    void  Init();                                                // v12 0x004D7DC0 (feb 0x004C8E00) exact
    void  Update(float dt);                                      // v12 0x004D7C20 (feb 0x004C8C60) exact
    int   DeleteInstNotify(animINST *a);                         // v12 0x004D8270
    int   DeleteObjNotify(objOBJ *o);                            // v12 0x004D82D0
    float GetMaxRadius();                                        // v12 0x004D8260
    void  SetCtrlRad(m3dCTRL_1D *c);                             // v12 0x004D80A0 exact
    void  SetCtrlPos(m3dCTRL_3D *c);                             // v12 0x004D80D0 exact
    void  SetCtrlColorLM(m3dCTRL_COLOR *c);                      // v12 0x004D8120 exact
    void  SetCtrlColorBase(m3dCTRL_COLOR *c);                    // v12 0x004D8190 exact
    void  SetCurTime(float t);                                   // v12 0x004D8200 exact
};
// pteSPOT_ELLIPSE (pte_spot_e.cpp): a spot stretched along a direction (blood running down): faces in its own
// frame (SetParams: org, along dir, up), texture coordinates from the two axes the face lies in, the stretched one
// divided by `stretch`.
class pteSPOT_ELLIPSE : public pteSPOT_ANIM {                    // size 0x168
public:
    float   stretch;                     // +0x108 (wrsfxBLOOD_SURF ctor: 1)
    float   param2;                      // +0x10C
    m3dMATR w2l;                         // +0x110 world -> its frame
    m3dV    segA, segB;                  // +0x150 / +0x15C org -+ (stretch + 1) x radius along dir
    void SetParams(m3dV *pos, m3dV *dir, m3dV *up, float stretch, float p2); // v12 0x004E1700 (feb 0x004D3470) exact
    int  ShadeFace(objOBJ *o, int face, pteSDR_INFO *info, ptePOLY_DATA *out); // v12 0x004E17D0 (feb 0x004D3540)
};
int __fastcall pteSpotAnimInit();                                // v12 0x004D7BB0 (feb 0x004C8BF0): class "spot", 0x14
int __fastcall pteSpotInit();
int __fastcall pteFogInit();                                     // (feb 0x0057E690; v12 inlined) class "fog", priority 0x32                                    // v12 0x004D60F0 (feb 0x004C7110) exact: "lspot", pteSpotAnimInit
class m3dCTRL_3D *__fastcall pteSpotNewFollow(animINST *a, objOBJ *o);  // (inline animCTRL_3D_FOLLOW ctor, kind 2)
extern class txmTEXTURE *pteSpotTex;                             // v12 0x00974DE4 "lspot"

// pteSPOT (dynamic spot light projected on faces; sheet keys "lspot","color","colorBase","colorLM","radius";
// pteSPOT_ANIM adds "spot","radMax","radMin","radFreq","noise","ctrl_rad","object","follow","ctrl_pos")
// vtable v12 0x00606F40 (feb 0x005DBAA8), 27 slots, all verified (see vtables_dump.txt):
//   +0x08 GetTex 0x004D7500, +0x14 SetOrg 0x004D64B0, +0x2C Init 0x004D62C0, +0x30 IsShadeObj 0x004D6640,
//   +0x34 IsShadeInst 0x004D6710, +0x38 ShadeFacePrepare 0x004D69A0, +0x3C ShadeFace 0x004D69C0, +0x44 IsActive 0x004D6730,
//   +0x4C PrepareFrame 0x004D68C0, +0x50 InitLevel 0x004D6540, +0x60 MakeObjIndList 0x004D7880, +0x64 GetMaxRadius 0x004D7BA0,
//   +0x68 IsIncludeFaceStaticData 0x004D78C0  (all v12)
// pteSPOT_ANIM vtable feb 0x005CE5F0 (v12: map)  // ?

// pteFOG_GROUND ("fog"): ground fog in up to 5 scnDOMAINs; also the volume of every water/lava entity
// (waterLIQUID::ProcessMsg builds it from the template's "*_fog" objects; waterWATER::IsUnderWater tests it).
// Data layout as far as the rebuilt code uses it (ctor v12 0x0059A1C0, InitParam v12 0x0059A500); size 0xA4C.
class pteFOG_GROUND_DATA {
public:
    u8         shader[0x8C];       // +0x000 pteSHADER part
    int        nDomain;            // +0x08C
    scnDOMAIN  domain[5];          // +0x090 0x3C each; InitParam: domain.yMin = top of domain 0 - depth
    u8         _1bc[0x1D4 - 0x1BC];// +0x1BC plane (+0x1BC 0, +0x1C0 top, +0x1C4 0, +0x1C8 up = m3dVUnitY)
    u32        paramFlags;         // +0x1D4 1 = noise, 2 = ground ("isGround")
    float      color[4];           // +0x1D8 ARGB 0..255 ("color", default 255,255,255,255)
    float      depth;              // +0x1E8 ("depth", default 5)
    float      farDist;            // +0x1EC ("farDist", 8)
    float      nearIntensity;      // +0x1F0 (0)
    float      farIntensity;       // +0x1F4 (0.8)
    float      gamma;              // +0x1F8 (1)
    float      freqXZ, freqTime;   // +0x1FC (0.25, 1.5)
    float      noisePercent;       // +0x204 (0.3)
    float      windSpeed;          // +0x208 (0.5)
    float      farNoiseScale;      // +0x20C (0)
    int        nObj;               // +0x210 static objects in the fog (max 500)
};
class pteFOG_GROUND : public pteSHADER {                         // pte_fog.obj; size 0xA4C, layout as pteFOG_GROUND_DATA
public:
    int        nDomain;            // +0x08C
    scnDOMAIN  domain[5];          // +0x090
    m3dV       planePt;            // +0x1BC (0, top of domain 0, 0)
    m3dV       planeN;             // +0x1C8 up
    u32        paramFlags;         // +0x1D4 InitLevel's 15 parameters from here (InitParam copies them)
    float      fogColor[4];        // +0x1D8
    float      depth, farDist, nearIntensity, farIntensity, gamma, freqXZ, freqTime, noisePercent, windSpeed, farNoiseScale;
    int        nObj;               // +0x210 static objects in the fog (max 500)
    objOBJ    *objs[500];          // +0x214
    int        nInst;              // +0x9E4 instances (max 20)
    animINST  *insts[20];          // +0x9E8
    float      time;               // +0xA38 (Update)
    int        camOut;             // +0xA3C (PrepareFrame) 1: the camera above the fog's top or in none of its volumes
    m3dV       camPos;             // +0xA40
    pteFOG_GROUND(objOBJ *o);                                    // v12 0x0059A1C0 (feb 0x0057E7A0)
    void *__vecDelDtor(unsigned flags);                          // v12 0x0059A280
    int  IsShadeObj(objOBJ *);                                   // v12 0x0059A790
    int  IsShadeInst(animINST *);                                // v12 0x0059A6D0
    int  ShadeFace(objOBJ *, int, struct pteSDR_INFO *, struct ptePOLY_DATA *);                                     // v12 0x0059A8F0
    int  ShadeVert(objOBJ *, int, void *);                       // v12 0x0059ACE0 (writes the colour at +9 of the data)
    float CalcVertFog(m3dV *eye, m3dV *p);                       // v12 0x0059AF10 (feb 0x0057F4F0) 0..1 at world point p
    static pteSHADER *__fastcall Create(animINST *, objOBJ *o); // v12 0x0059A0D0 (feb 0x0057E6B0) needs an outline object
    int  IsActive(int);                                          // v12 0x0059A660
    void Update(float dt);                                       // v12 0x0059A6B0
    void PrepareFrame(scnSCENE *, camCAMERA *);                  // v12 0x0059A850
    int  InitLevel(scnSCENE *);                                  // v12 0x0059A2F0 the "FOG" section, then InitParam
    int  InitParam(scnSCENE *, u32 *params);                     // v12 0x0059A500
    int  SetDomainObj(objOBJ *o);                                // v12 0x0059A4A0 one more volume (outline o)
};
// pteEMAP (pte_envmap.obj): environment-map shaders on one object or instance. pteEMAP_REFLECT: a reflection
// around a sphere ("sphRadius"); pteEMAP_REFLECT_WATER (water.obj's REFLECTION section, vtable v12 0x00608B80).
class pteEMAP : public pteSHADER {
public:
    m3dMATR matrL2W;               // +0x08C (PrepareFrame, GetMatrL2W)
    m3dMATR matrW2L;               // +0x0CC its inverse
    float  blendCoeff;             // +0x10C ("blendCoeff", REFLECT ctor 0.15)
    void   Init();                                               // v12 0x004E2260 blendType, blendCoeff, normals
    int    IsShadeObj(objOBJ *);                                 // v12 0x004E23F0 its object or its instance's
    int    IsShadeInst(animINST *);                              // v12 0x004E58C0
    pteEMAP() { blendCoeff = 1.0f; }                             // v12 0x004E2240
    void  PrepareFrame(scnSCENE *, camCAMERA *);                 // v12 0x004E24B0 (feb 0x004D40D0)
    virtual void  GetMatrL2W(m3dMATR *out);                      // [25] +0x64 v12 0x004E2420
    virtual char *GetBlendTypeDef() = 0;                         // [26] +0x68 per kind (REFLECT_WATER: v12 0x0047F500)
};
class pteEMAP_REFLECT : public pteEMAP {         // size 0x128
public:
    struct { const void *vtbl; int type; m3dV center; float radius; } sphere; // +0x110 m3dSPHERE
    pteEMAP_REFLECT();                                           // v12 0x004E2FB0
    void *__vecDelDtor(unsigned flags);                          // v12 0x004E2FF0
    void  Init();                                                // v12 0x004E4550 the sphere from the object's box
    int   ShadeFace(objOBJ *, int, struct pteSDR_INFO *, struct ptePOLY_DATA *);                                    // v12 0x004E4780 (not rebuilt)
    int   IsActive(int);                                         // v12 0x0048D590 1
    // (PrepareFrame: pteEMAP's, v12 0x004E24B0)
};
class pteEMAP_REFLECT_WATER : public pteEMAP_REFLECT {
public:
    // (GetBlendTypeDef first: GCC emits the vtable with the first declared out-of-line virtual, a rebuilt one)
    char *GetBlendTypeDef();                                     // v12 0x0047F500
    int   ShadeFace(objOBJ *, int, struct pteSDR_INFO *, struct ptePOLY_DATA *);                                    // v12 0x0055AD00 (not rebuilt)
    int   ShadeVert(objOBJ *, int, void *);                      // v12 0x0055AD60 (not rebuilt)
};
// pteFOG_GROUND ("fog") vtable v12 0x0060AB60 (feb 0x005DF598), all slots verified:
//   +0x30 IsShadeObj 0x0059A790, +0x34 IsShadeInst 0x0059A6D0, +0x3C ShadeFace 0x0059A8F0, +0x40 ShadeVert 0x0059ACE0,
//   +0x44 IsActive 0x0059A660, +0x48 Update 0x0059A6B0, +0x4C PrepareFrame 0x0059A850, +0x50 InitLevel 0x0059A2F0 (v12)

// pteEMAP family ("emap_*"; keys decal, mult, blendCoeff, blendType, isSource, scaleTile, sphRadius, alphaMax, bbox,
// isectBed, objBed; "SPECULAR" source object). feb vtable 0x005DBDD0 (28 slots: +0x2C Init 0x004D3E80, +0x30 IsShadeObj
// 0x004D4010, +0x34 IsShadeInst 0x004D74E0, +0x4C PrepareFrame 0x004D40D0, +0x64 GetMatrL2W 0x004D4040).
// v12 vtable address NOT resolved (automatic data map points into the wrong table)  // ?
