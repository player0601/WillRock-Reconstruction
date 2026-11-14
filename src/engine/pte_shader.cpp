// pte_shader.cpp -- CPU shaders: the manager's shader list (Will Rock 1.2, v12.exe; feb obj engine:pte_shader.obj)
//
// The manager's list walk (gsEffEnvirColor), level start / end, removal and the destroy notifies; the pteSHADER base. Not rebuilt:
// face and vertex shading (pteMANAGER::ShadeFace / ShadeVert).

#include <stdio.h>
#include "engine.h"
#include "../../include/engine_render/shader.h"

// v12 0x004D88E0  (feb 0x004C9920) exact
// The shader after prev in the manager's list (the first when prev is 0), 0 at the end.
pteSHADER *pteMANAGER::FindNextShader(pteSHADER *prev)
{
    int i = 0;
    if (prev)
        i = *(int *)((char *)prev + 0x80) + 1;          // the shader's own list index
    if (i < *(int *)((char *)this + 0x1F44))           // nShaders
        return ((pteSHADER **)((char *)this + 4))[i];
    return 0;
}

// ---------------------------------------------------------------------------------------------- the manager

// v12 0x004D9360  (feb 0x004CA3A0)
BOOL32 pteMANAGER::FindShaderAround(int n, int *sigs, m3dV *p, float r)
{
    if (!(r > 0.0f))
        return 0;
    for (pteSHADER *s = FindNextShader(0); s; s = FindNextShader(s))
        for (int i = 0; i < n; i++) {
            m3dV o;
            if (*(int *)((char *)s + 0x14) == sigs[i] && (*(int *)((char *)s + 4) & 1) && s->GetOrg(&o)) {
                float dx = p->x - o.x, dy = p->y - o.y, dz = p->z - o.z;
                if (m3dSqrt(dx * dx + dy * dy + dz * dz) < r)
                    return 1;
            }
        }
    return 0;
}

// v12 0x004D8750  (feb 0x004C9790) exact
// Every shader's level set-up (gsLVL_SYSTEM::ProcessINIT_LEVEL); 0 when the shaders are off or one fails. Shaders
// registered later start with the level at once (pteSHADER::Register).
int pteMANAGER::InitLevel(scnSCENE *s)
{
    if (!*(int *)((char *)this + 0x17944))              // initialised (pteInit)
        return 0;
    pteSHADER **list = (pteSHADER **)((char *)this + 4);
    for (int i = 0; i < *(int *)((char *)this + 0x1F44); i++)
        if (!list[i]->InitLevel(s))
            return 0;
    *(scnSCENE **)((char *)this + 0x1793C) = s;
    *(int *)((char *)this + 0x17948) = 1;
    return 1;
}

// v12 0x004D87B0  (feb 0x004C97F0) exact
// Every shader's level end; those not owned by an instance or object (flags 4 / 8) are deleted, which takes them
// out of the list while it is walked (the one after each is skipped, as in the game).
void pteMANAGER::TermLevel()
{
    if (!*(int *)((char *)this + 0x17944))
        return;
    if (*(int *)((char *)this + 0x17948)) {
        pteSHADER **list = (pteSHADER **)((char *)this + 4);
        for (int i = 0; i < *(int *)((char *)this + 0x1F44); i++) {
            list[i]->TermLevel();
            pteSHADER *s = list[i];
            if (!(s->flags & 0xC) && s)
                s->__vecDelDtor(1);
        }
    }
    *(int *)((char *)this + 0x17948) = 0;
    *(scnSCENE **)((char *)this + 0x1793C) = 0;
}

// v12 0x004D8820  (feb 0x004C9860) exact
// An instance going away: each shader told (DeleteInstNotify); those it owns deleted, which takes them out of the
// list (the walk steps back one).
void pteMANAGER::DeleteInstNotify(animINST *a)
{
    if (!a)
        return;
    pteSHADER **list = (pteSHADER **)((char *)this + 4);
    for (int i = 0; i < *(int *)((char *)this + 0x1F44); i++)
        if (list[i]->DeleteInstNotify(a)) {
            if (list[i])
                list[i]->__vecDelDtor(1);
            i--;
        }
}

// v12 0x004D8870  (feb 0x004C98B0) exact
// The same for an object going away (DeleteObjNotify).
void pteMANAGER::DeleteObjNotify(objOBJ *o)
{
    if (!o)
        return;
    pteSHADER **list = (pteSHADER **)((char *)this + 4);
    for (int i = 0; i < *(int *)((char *)this + 0x1F44); i++)
        if (list[i]->DeleteObjNotify(o)) {
            if (list[i])
                list[i]->__vecDelDtor(1);
            i--;
        }
}

// v12 0x004D8910  (feb 0x004C9950) exact
// s out of the list (not the kept ones, flags 0x100), the others renumbered.
void pteMANAGER::RemoveShader(pteSHADER *s)
{
    if (s->flags & 0x100)
        return;
    int *n = (int *)((char *)this + 0x1F44);
    pteSHADER **list = (pteSHADER **)((char *)this + 4);
    int i;
    for (i = 0; i < *n && list[i] != s; i++)
        ;
    if (i == *n)
        return;
    if (i < *n - 1)
        memmove(&list[i], &list[i + 1], (*n - i - 1) * 4);
    (*n)--;
    for (int k = 0; k < *n; k++)
        list[k]->id = k;
}

// ---------------------------------------------------------------------------------------------- pteSHADER

unsigned pteSHADER_faceUID;                 // v12 0x00974E18 pteSHADER::faceUID

// v12 0x004D8BB0  (feb 0x004C9BF0) exact
// Defaults, then into the manager's list (at most 2000, once) with its index.
pteSHADER::pteSHADER()
{
    flags = 0;
    priority = 100;
    _14 = 0;
    tex = 0;
    color[0] = color[1] = color[2] = color[3] = 255.0f;
    _10 = 0;
    frameNmb = msgSystem.frameNmb;
    id = -1;
    pteSHADER_faceUID = 0;
    distOff = -1.0f;
    inst = 0;
    obj = 0;
    sheet = 0;
    staticData = 0;
    _5c = -1.0f;
    stateRend = 0;
    int *n = (int *)((char *)pteManager + 0x1F44);
    pteSHADER **list = (pteSHADER **)((char *)pteManager + 4);
    if (*n <= 1999) {
        int i;
        for (i = 0; i < *n && list[i] != this; i++)
            ;
        if (i == *n) {
            list[*n] = this;
            id = *n;
            (*n)++;
            flags &= ~2u;
        }
    }
    strcpy(sect, "PTE_SHADER");
}

// v12 0x004D8C90  (feb 0x004C9CD0) exact (the destructor v12 0x004D8CE0 inlined)
void *pteSHADER::__vecDelDtor(unsigned f)
{
    pteManager->RemoveShader(this);
    if (staticData)
        delete staticData;
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004D8D20  (feb 0x004C9D60)
// "color", "distOff" and "tex" (a texture by name) of its section.
void pteSHADER::Init()
{
    char t[0x40];
    if (sheet)
        sheet->GetColor(sect, (char *)"color", (m3dCOLOR *)color);
    if (sheet)
        sheet->GetFloat(sect, (char *)"distOff", &distOff);
    if (sheet && sheet->GetStr(sect, (char *)"tex", t, 0x40))
        tex = txmManager->Add(t, 0x40000, 1);
}

// v12 0x004D8DA0 .. 0x004D8E40  (feb 0x004C9DE0 ..) exact: its section's keys
int pteSHADER::ReadStr(char *key, char *out, int n)
{
    return sheet && sheet->GetStr(sect, key, out, n) ? 1 : 0;
}

int pteSHADER::ReadFloat(char *key, float *out)
{
    return sheet && sheet->GetFloat(sect, key, out) ? 1 : 0;
}

int pteSHADER::ReadBool(char *key, int *out)
{
    return sheet && sheet->GetBool(sect, key, out) ? 1 : 0;
}

int pteSHADER::ReadColor(char *key, m3dCOLOR *out)
{
    return sheet && sheet->GetColor(sect, key, out) ? 1 : 0;
}

// v12 0x004D8E70  (feb 0x004C9EB0) exact
// Started with the level if one is on, its parameters read, switched on.
int pteSHADER::Register()
{
    if (*(int *)((char *)pteManager + 0x17948))
        InitLevel(*(scnSCENE **)((char *)pteManager + 0x1793C));
    Init();
    Enable(1);
    flags |= 2;
    return 1;
}

// v12 0x004D8EB0
void pteSHADER::Enable(int on)
{
    if (on)
        flags |= 1;
    else
        flags &= ~1u;
}

// v12 0x004D8ED0  (feb 0x004C9F10) exact
void pteSHADER::SetSheet(psSHEET *ps, char *s)
{
    sheet = ps;
    if (s)
        strcpy(sect, s);
}

// v12 0x004D8F00 / 0x004D8F20
void pteSHADER::SetInst(animINST *a)
{
    inst = a;
    flags |= 4;
}

void pteSHADER::SetObj(objOBJ *o)
{
    obj = o;
    flags |= 8;
}

// v12 0x004D8F40  (feb 0x004C9F80): 1 when the instance going away owns this shader
int pteSHADER::DeleteInstNotify(animINST *a)
{
    if (inst == a) {
        if (flags & 4)
            return 1;
        inst = 0;
    }
    return 0;
}

// v12 0x004D8F70  (feb 0x004C9FB0)
int pteSHADER::DeleteObjNotify(objOBJ *o)
{
    if (obj == o) {
        if (flags & 8)
            return 1;
        obj = 0;
    }
    return 0;
}

// v12 0x004D8FA0  (feb 0x004C9FE0): "object::texture" in apBuffer
char *pteSHADER::GetName()
{
    char *t = tex ? (char *)tex + 8 : (char *)"texture_undefined";
    if (obj)
        sprintf(apBuffer, "%s::%s", obj->name, t);
    else if (inst)
        sprintf(apBuffer, "%s::%s", inst->name ? inst->name : "", t);
    else
        sprintf(apBuffer, "%s::%s", "object_undefined", t);
    return apBuffer;
}

// the base slots (shared one-liners: v12 0x0041C180, 0x00403970, 0x0041C190, 0x00403950, 0x00403200, 0x0041C1C0,
// 0x0041C1D0, 0x0048DF30, 0x00403860, 0x0048D590, 0x00403A20)
txmTEXTURE *pteSHADER::GetTex(int face, objOBJ *o, int n)
{
    return tex;
}

int pteSHADER::GetLightAt(m3dV *p, int n, m3dCOLOR *out)
{
    return 0;
}

float *pteSHADER::GetColor(float *out, int n)
{
    out[0] = color[0];
    out[1] = color[1];
    out[2] = color[2];
    out[3] = color[3];
    return out;
}

void pteSHADER::SetOrg(m3dV *p)
{
}

int pteSHADER::GetOrg(m3dV *p)
{
    return 0;
}

void pteSHADER::SetStateRend(int s)
{
    stateRend = s;
}

int pteSHADER::GetStateRend(int which, int *out)
{
    *out = stateRend;
    return which == 1;
}

int pteSHADER::_24()
{
    return 0;
}

int pteSHADER::ShadeFacePrepare(void *d)
{
    return 0;
}

int pteSHADER::ShadeVert(objOBJ *o, int i, void *d)
{
    return 0;
}

void pteSHADER::Update(float dt)
{
}

void pteSHADER::PrepareFrame(scnSCENE *s, camCAMERA *c)
{
}

int pteSHADER::InitLevel(scnSCENE *s)
{
    return 1;
}

void pteSHADER::TermLevel()
{
}

// the shader classes (v12 0x00974E14 count, 0x00974E20 the table of 0x48-byte entries)
static struct { char name[0x40]; int priority; pteFACTORY create; } pteClass[32];
static int pteNClass;

// v12 0x004D8480  (feb 0x004C94C0) exact
// A name registered twice keeps its first.
int pteMANAGER::RegisterClass(char *name, int priority, pteFACTORY create)
{
    for (int i = 0; i < pteNClass; i++)
        if (!strcmp(pteClass[i].name, name))
            return 1;
    ((apNAME *)pteClass[pteNClass].name)->SetName(name);
    pteClass[pteNClass].priority = priority;
    pteClass[pteNClass].create = create;
    pteNClass++;
    return 1;
}

// v12 0x004D9010  (feb 0x004CA050)
// This face's frame is worked out: the later shaders of the frame reuse it (the manager's face stamp).
void pteSHADER::MarkFaceFirstShaded()
{
    pteSHADER_faceUID = *(unsigned *)((char *)pteManager + 0x17954);
}

// v12 0x004D8370  (feb 0x004C93B0) exact
// The manager (its counts and level pointers cleared) and its object/face list (500 objects, 3000 faces).
// (Its vtable v12 0x00606FE4 holds only the deleting destructor: not rebuilt, the pointer is left 0.)
int __fastcall pteInit()
{
    char *m = (char *)operator new(0x17958);
    if (m) {
        *(void **)m = 0;
        *(int *)(m + 0x17938) = 0;
        *(int *)(m + 0x17944) = 0;
        *(int *)(m + 0x17948) = 0;
        *(int *)(m + 0x1794C) = 0;
        *(int *)(m + 0x17950) = 0;
        *(int *)(m + 0x1F44) = 0;
        *(int *)(m + 0x1F48) = 0;
        *(int *)(m + 0x17954) = 0;
        *(int *)(m + 0x1793C) = 0;
        *(int *)(m + 0x17940) = 0;
    }
    pteManager = (pteMANAGER *)m;
    cdtOFL *o = (cdtOFL *)operator new(sizeof(cdtOFL));
    if (o)
        o = cdtOFL_Construct(o, 500, 3000);
    *(cdtOFL **)(m + 0x1794C) = o;
    if (!o)
        return 0;
    *(int *)(m + 0x17944) = 1;
    return 1;
}

// v12 0x004D8420  (feb 0x004C9460) exact
void __fastcall pteTerm()
{
    char *m = (char *)pteManager;
    cdtOFL *o = *(cdtOFL **)(m + 0x1794C);
    if (o) {
        cdtOFL_Destroy(o);
        operator delete(o);
    }
    *(int *)(m + 0x17944) = 0;
    if (pteManager)
        operator delete(pteManager);                    // (the deleting destructor v12 0x004D8460)
}

// v12 0x004D98F0  (feb 0x004CA930) exact
// Once per face and frame: each vertex's normal from the object's vertex normals (a clipped face's vertices
// interpolated between the triangle's three by their a/b weights).
void pteSDR_INFO::ValidateNormalSmooth(objOBJ *o, int face)
{
    if (valid & 1)
        return;
    m3dV *vn = o->geom->vertNormal;
    objFACE *f = o->GetFace(face);
    m3dV *n0 = &vn[f->vert[0]], *n1 = &vn[f->vert[1]], *n2 = &vn[f->vert[2]];
    for (int i = 0; i < nVert; i++) {
        if (!split->clipped) {
            normal[i] = vn[f->vert[i]];
        } else {
            float a = split->a[i], b = split->b[i];
            normal[i].x = a * (n1->x - n0->x) + b * (n2->x - n0->x) + n0->x;
            normal[i].y = (n2->y - n0->y) * b + (n1->y - n0->y) * a + n0->y;
            normal[i].z = (n2->z - n0->z) * b + (n1->z - n0->z) * a + n0->z;
        }
    }
    valid |= 1;
}

static void *_pteCfgUseShaders;                         // (the cfg cache of [Video] VIDEO_UseShaders)

// v12 0x004D8520  (feb 0x004C9560) exact
// The sheet's "PTE_SHADER" section (n > 0: "PTE_SHADER<n>"): its "type" a registered class -> that class's shader,
// its priority, its section name and sheet.
pteSHADER *pteMANAGER::CreateShader(psSHEET *sh, int n, animINST *a, objOBJ *o)
{
    char sect[0x40], type[0x20];
    if (!sh)
        return 0;
    if (!n)
        strcpy(sect, "PTE_SHADER");
    else
        sprintf(sect, "PTE_SHADER%d", n);
    if (!sh->GetStr(sect, (char *)"type", type, 0x20))
        return 0;
    for (int i = 0; i < pteNClass; i++) {
        if (strcmp(pteClass[i].name, type))
            continue;
        pteSHADER *s = pteClass[i].create(a, o);
        if (!s)
            return 0;
        s->priority = pteClass[i].priority;
        strcpy(s->sect, sect);
        s->sheet = sh;
        return s;
    }
    return 0;
}

// (the two public ones: up to 16 sections; each shader owned (4 instance / 8 object), started with the level when
// it is running, initialised, on, registered)
static void pteStartCreated(pteSHADER *s)
{
    if (*(int *)((char *)pteManager + 0x17948))
        s->InitLevel(*(scnSCENE **)((char *)pteManager + 0x1793C));
    s->Init();
    s->Enable(1);
    s->flags |= 2;
}

// v12 0x004D8650  (feb 0x004C9690) exact
void pteMANAGER::CreateShader(animINST *a)
{
    if (!a)
        return;
    for (int n = 0; n < 16; n++) {
        pteSHADER *s = CreateShader(*(psSHEET **)((char *)a + 0x134), n, a, 0);
        if (!s)
            continue;
        s->flags |= 4;
        s->inst = a;
        pteStartCreated(s);
    }
}

// v12 0x004D86D0  (feb 0x004C9710) exact
void pteMANAGER::CreateShader(objOBJ *o)
{
    if (!o)
        return;
    for (int n = 0; n < 16; n++) {
        pteSHADER *s = CreateShader(o->props, n, 0, o);
        if (!s)
            continue;
        s->flags |= 8;
        s->obj = o;
        pteStartCreated(s);
    }
}

// v12 0x004D89A0  (feb 0x004C99E0) exact
// Every shader's Update; a timed-out one (distOff run below 0 with flags 0x80) or one switched off for good (not on,
// 0x40) is deleted (kept: 0x100) and taken out of the list.
void pteMANAGER::UpdateAllShaders(float dt)
{
    pteSHADER **list = (pteSHADER **)((char *)this + 4);
    int *n = (int *)((char *)this + 0x1F44);
    for (int i = 0; i < *n; i++) {
        list[i]->Update(dt);
        pteSHADER *s = list[i];
        if ((s->_5c <= 0.0f && (s->flags & 0x80)) || (!(s->flags & 1) && (s->flags & 0x40))) {
            s->flags |= 0x100;
            s->__vecDelDtor(1);
            if (i < *n - 1)
                memmove(&list[i], &list[i + 1], (*n - i - 1) * sizeof(pteSHADER *));
            (*n)--;
            i--;
        }
    }
}

// v12 0x004D9B30  (feb 0x004CAB70) exact: higher priority first, then the older frame, then the list order
static int __cdecl _pteSortFunc(const void *pa, const void *pb)
{
    pteSHADER *a = *(pteSHADER **)pa, *b = *(pteSHADER **)pb;
    int d = b->priority - a->priority;
    if (!d)
        d = b->frameNmb - a->frameNmb;
    if (!d)
        d = (int)(((char *)b - (char *)a) / 0x8C);
    return d;
}

// v12 0x004D8A80  (feb 0x004C9AC0) exact
// Unless [Video] VIDEO_UseShaders is off: the list sorted (each its index), the active ones (on, IsActive) chained.
void pteMANAGER::ActivateAllShaders(camCAMERA *c)
{
    int use = 1;
    *(pteSHADER **)((char *)this + 0x1F48) = 0;
    if (_apCfgReadBool(&_pteCfgUseShaders, (char *)"Video", (char *)"VIDEO_UseShaders", &use) && !use)
        return;
    pteSHADER **list = (pteSHADER **)((char *)this + 4);
    int n = *(int *)((char *)this + 0x1F44);
    qsort(list, n, 4, _pteSortFunc);
    for (int i = 0; i < n; i++)
        list[i]->id = i;
    for (int i = 0; i < n; i++) {
        pteSHADER *s = list[i];
        if ((s->flags & 1) && s->IsActive((int)c)) {
            s->_10 = (int)*(pteSHADER **)((char *)pteManager + 0x1F48);
            *(pteSHADER **)((char *)pteManager + 0x1F48) = s;
        }
    }
}

// v12 0x004D8B60  (feb 0x004C9BA0) exact
// With the level running: the camera kept, the frame's counts cleared, each active shader prepares its frame.
void pteMANAGER::PrepareFrame(scnSCENE *s, camCAMERA *c)
{
    if (!*(int *)((char *)this + 0x17948))
        return;
    pteSHADER *sh = *(pteSHADER **)((char *)this + 0x1F48);
    if (!sh)
        return;
    *(camCAMERA **)((char *)this + 0x17940) = c;
    *(int *)((char *)this + 0x3E8C) = 0;
    *(int *)((char *)this + 0x17710) = 0;
    for (; sh; sh = (pteSHADER *)sh->_10)
        sh->PrepareFrame(s, c);
}
