// txm.cpp -- texture manager and textures (plain and layered), the driver-independent part
// (Will Rock 1.2, v12.exe; feb obj drv:txm.obj)
//
// v12 0x004B1EC0..0x004B3AE0, all "exact" in 1.2 (beta decompile, call arguments from the 1.2 code).
// GetColorUV reads texels through the virtual GetColor (layer -1), which the render driver's texture answers.
// A texture is created at the size / format the driver accepts (RequestSize, 0 = ok) and the picture is
// read, resampled, converted and mip-mapped to match (ReadResample), then handed to the driver (LoadDrv).
// The manager keeps 1000 slots; a texture's slot number is its listNmb. Add looks a name up first:
// a plain texture asked as layered (or the opposite) fails. Kept: txmTEXTURE_LAY::Set / Load use layer
// index -1 for the whole texture (that "layer" overlaps the base fields), txmTEXTURE_LAY::Load clears
// the dirty bit it has just set, the extra data written by Save is uninitialised in the game (0 here).

#include "engine.h"
#include "../../include/engine_render/texture.h"

txmMANAGER *txmManager;                // v12 0x006561B8 (feb 0x00611FD8) set by the renderer (a txmD3D_MANAGER)

static_assert(sizeof(txmTEXTURE) == 0x78, "txmTEXTURE");
static_assert(sizeof(txmTEXTURE_LAY) == 0x228, "txmTEXTURE_LAY");
static_assert(sizeof(txmMANAGER) == 0x1100, "txmMANAGER");

static const float txmEps = 1e-6f;     // v12 0x00606650 (feb 0x005DB110)

// ------------------------------------------------------------------------------------------------
// txmTEXTURE

// v12 0x004B1EC0  (feb 0x004A3260)
txmTEXTURE::txmTEXTURE(char *n, int f)
{
    flags = f;
    ((apNAME *)name)->SetName(n);
    w = 0;
    h = 0;
    _5c = 0;
    _60 = 0;
    memSize = 0;
    extra = 0;
    drvData = 0;
    magic = TXM_MAGIC_TEX;
    nMip = -1;
    fmt = -1;
    _68 = -1;
    listNmb = -1;
}

// v12 0x004B1F10  (feb 0x004A32B0)
void *txmTEXTURE::__vecDelDtor(unsigned f)
{
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004B1F40  (feb 0x004A32E0)
int txmTEXTURE::FindDisk(char *dir, char *ext, char *out)
{
    char *s = fnmFindDisc(dir, name, ext);
    if (!s) {
        apLog((char *)"Texture %s not found", name);
        return 0;
    }
    if (out)
        strcpy(out, s);
    return 1;
}

// v12 0x004B1F90  (feb 0x004A3330)
int txmTEXTURE::Create(int cw, int ch, int cMip, int cFmt)
{
    if (flags & TXM_F_CREATED)
        return w == cw && h == ch && nMip == cMip && fmt == cFmt;
    if (RequestSize(&cw, &ch))
        return 0;
    h = ch;
    nMip = cMip;
    w = cw;
    fmt = cFmt;
    pctHEADER hd;
    hd.magic = PCT_MAGIC;
    hd.w = cw;
    hd.h = ch;
    hd.nMip = cMip;
    hd.fmt = cFmt;
    memSize = pctGetMemSize(&hd, -1);
    if (!CreateDrv(&hd))
        return 0;
    flags |= TXM_F_CREATED;
    return 1;
}

// v12 0x004B2070  (feb 0x004A3410)
int txmTEXTURE::Set(pctPICTURE *p, int layer)
{
    if (!Create(p->w, p->h, p->nMip, p->fmt))
        return 0;
    if (!LoadDrv(p))
        return 0;
    flags |= TXM_F_LOADED;
    return 1;
}

// v12 0x004B20C0  (feb 0x004A3460)
int txmTEXTURE::Load(char *path, char *ext, int layer)
{
    if (!(flags & TXM_F_LOADED)) {
        pctPICTURE *p = ReadResample(path, name, ext, flags & TXM_F_CREATED);
        if (!p) {
            apMsg((char *)"Cannot resample picture %s", name);
            return 0;
        }
        if (!Set(p, -1)) {
            apMsg((char *)"Cannot set picture %s", name);
            return 0;
        }
        pctDestroy(p);
    }
    return 1;
}

// v12 0x004B2140  (feb 0x004A34E0)
void txmTEXTURE::Unload(int layer)
{
    if (flags & TXM_F_CREATED) {
        UnloadDrv();
        flags &= ~(TXM_F_CREATED | TXM_F_LOADED);
    }
}

// v12 0x004B2160  (feb 0x004A3500)
int txmTEXTURE::Fill(pctPIXEL *c, int layer)
{
    if (!(flags & TXM_F_CREATED))
        return 0;
    for (int x = 0; x < w; x++)
        for (int y = 0; y < h; y++)
            SetColor(x, y, c, layer);
    return 1;
}

// The shared one-line defaults of the vtable (COMDAT-folded in the game)
int  txmTEXTURE::Update(float dt) { return 1; }                          // v12 0x0048D590
int  txmTEXTURE::RequestSize(int *pw, int *ph) { return 0; }             // v12 0x004036D0
int  txmTEXTURE::CreateDrv(pctHEADER *hd) { return 1; }                  // v12 0x0048D590
int  txmTEXTURE::LoadDrv(pctPICTURE *p) { return 1; }                    // v12 0x0048D590
void txmTEXTURE::UnloadDrv() {}                                          // v12 0x00403A20
int  txmTEXTURE::GetColor(int x, int y, pctPIXEL *c, int layer) { return 1; } // v12 0x004B39C0
int  txmTEXTURE::SetColor(int x, int y, pctPIXEL *c, int layer) { return 1; } // v12 0x004B39C0

// v12 0x004B21C0  (feb 0x004A3560)
// The colour at (u, v), bilinear between the four texels around it (ARGB; alpha only for alpha textures, else
// 0xFF). u and v wrap with a period of 1.0001 unless the texture clamps them to 0..1 (TXM_F_CLAMP_U / V).
// -1 when the texture holds no picture.
// (txmEps, v12 0x00606650, is the file's 1e-6 above)

static inline float txmWrap(float t)
{
    float k = txmEps * 100.0f + 1.0f;
    float sub = t;                                      // (a period of ~0 wraps to 0)
    if (!(fabsf(k) < txmEps))
        sub = (float)floor((double)(t / k)) * k;
    return t - sub;
}

static inline float txmClamp01(float t)
{
    if (t < 0.0f)
        return 0.0f;
    if (t > 1.0f)
        return 1.0f;
    return t;
}

uint32_t txmTEXTURE::GetColorUV(float u, float v)
{
    if (!(flags & TXM_F_LOADED))
        return 0xFFFFFFFF;
    u = (flags & TXM_F_CLAMP_U) ? txmClamp01(u) : txmWrap(u);
    v = (flags & TXM_F_CLAMP_V) ? txmClamp01(v) : txmWrap(v);
    int x0 = (int)floorf((float)w * u - 0.5f);         // (m3dIntFloor)
    int y0 = (int)floorf((float)h * v - 0.5f);
    m2dV c0 = { txmClamp01(((float)x0 + 0.5f) / (float)w), txmClamp01(((float)y0 + 0.5f) / (float)h) };
    pctPIXEL px[4];                                     // (x0,y0) (x0,y0+1) (x0+1,y0) (x0+1,y0+1)
    for (int i = 0; i < 2; i++)
        for (int j = 0; j < 2; j++) {
            int x = x0 + i, y = y0 + j;
            if (x > w - 1)
                x = w - 1;
            if (x < 0)
                x = 0;
            if (y > h - 1)
                y = h - 1;
            if (y < 0)
                y = 0;
            GetColor(x, y, &px[i * 2 + j], -1);
        }
    m2dV c1 = { 1.0f / (float)w + c0.x, 1.0f / (float)h + c0.y };
    m2dV p = { u, v };
    u8 r = (u8)(int)m3dLerpRectCorners(&c0, &c1, &p, px[0].r, px[1].r, px[2].r, px[3].r);
    u8 g = (u8)(int)m3dLerpRectCorners(&c0, &c1, &p, px[0].g, px[1].g, px[2].g, px[3].g);
    u8 b = (u8)(int)m3dLerpRectCorners(&c0, &c1, &p, px[0].b, px[1].b, px[2].b, px[3].b);
    u8 a = (flags & TXM_F_ALPHA) ? (u8)(int)m3dLerpRectCorners(&c0, &c1, &p, px[0].a, px[1].a, px[2].a, px[3].a)
                                 : 0xFF;
    return (((u32)a << 8 | r) << 8 | g) << 8 | b;
}

// v12 0x004B2700  (feb 0x004A3AA0)
pctPICTURE *txmTEXTURE::ReadResample(char *path, char *n, char *ext, int keepSize)
{
    char *full = fnmMakeFullName(path, n, ext, 0);
    pctPICTURE *p = pctRead(full);
    if (!p)
        return 0;
    int rw, rh, rMip, rFmt;
    if (keepSize) {
        rw = w;
        rh = h;
        rMip = nMip;
        rFmt = fmt;
    } else {
        rw = p->w;
        rh = p->h;
        rMip = p->nMip;
        rFmt = p->fmt;
        RequestSize(&rw, &rh);
    }
    SetTexParams(p, &p->extra);
    BOOL32 changed = 0;
    pctPICTURE *q = p;
    if (rw != p->w || rh != p->h) {
        apMsg((char *)"Texture %s was resampled from %dx%d to %dx%d", full, p->w, p->h, rw, rh);
        q = pctResample(p, rw, rh, -1);
        if (!q)
            return 0;
        pctDestroy(p);
        changed = 1;
    }
    p = q;
    if (rFmt != q->fmt) {
        apMsg((char *)"Texture %s was converted from %s to %s", full, pctGetFormat(q->fmt)->name, pctGetFormat(rFmt)->name);
        p = pctConvert(q, rFmt);
        if (!p)
            return 0;
        pctDestroy(q);
        changed = 1;
    }
    if (rMip == p->nMip) {
        if (!changed)
            return p;
    } else {
        apMsg((char *)"Mip-Maps was generated for texture %s", full);
        if (!pctGenMipMaps(p, rMip, 2.2f))
            return 0;
    }
    apMsg((char *)">Save texture %s as %s ", n, full);
    return p;
}

// v12 0x004B28A0  (feb 0x004A3C40)
void txmTEXTURE::SetTexParams(pctHEADER *hd, pctEXTRA_DATA *e)
{
    if (e->flags & 1)
        flags |= TXM_F_EXTRA_01;
    if (e->flags & 2)
        flags |= TXM_F_EXTRA_02;
    if (e->flags & 0x40)
        flags |= TXM_F_EXTRA_40;
    if (e->flags & 4)
        flags |= TXM_F_EXTRA_04;
    if (e->flags & 8)
        flags |= TXM_F_CLAMP_U;
    if (e->flags & 0x10)
        flags |= TXM_F_CLAMP_V;
    extra = e->value;
    if (e->flags & 0x20)
        flags |= TXM_F_EXTRA_20;
    if (pctIsAlpha(hd->fmt))
        flags |= TXM_F_ALPHA;
    if (hd->fmt > PCT_UV88 && hd->fmt < PCT_NFORMAT)
        flags |= TXM_F_COMPRESSED;
}

// ------------------------------------------------------------------------------------------------
// txmLAYER / txmTEXTURE_LAY

// v12 0x004B2930  (feb 0x004A3CD0)
void txmLAYER::SetWeight(float wt)
{
    BOOL32 same = fabsf(weight - wt) < txmEps;
    weight = wt;
    if (!same)
        flags |= TXM_F_DIRTY;
}

static void txmLayInit(txmTEXTURE_LAY *t)
{
    for (int i = 0; i < TXM_MAX_LAYER; i++) {
        t->lay[i].flags = 0;
        t->lay[i].name[0] = 0;
        t->lay[i].data = 0;
        t->lay[i].weight = 0;
    }
    t->magic = TXM_MAGIC_LAYER;
    t->nLayer = 0;
    *(u32 *)((u8 *)t + 0x1F8) = 0;
    *(u32 *)((u8 *)t + 0x224) = 0;
}

// v12 0x004B2990  (feb 0x004A3D30)
txmTEXTURE_LAY::txmTEXTURE_LAY(char *n, int f) : txmTEXTURE(n, f)
{
    txmLayInit(this);
}

// v12 0x004B2A20  (feb 0x004A3DC0)
void *txmTEXTURE_LAY::__vecDelDtor(unsigned f)
{
    for (int i = TXM_MAX_LAYER - 1; i >= 0; i--) {     // ~txmTEXTURE_LAY v12 0x004B2A70
        apFree(lay[i].data);
        lay[i].data = 0;
    }
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004B2AB0  (feb 0x004A3E50)
int txmTEXTURE_LAY::FindDisk(char *dir, char *ext, char *out)
{
    for (int i = 0; i < nLayer; i++) {
        char *s = fnmFindDisc(dir, lay[i].name, ext);
        if (s) {
            strcpy(out, s);
            return 1;
        }
    }
    return 0;
}

// v12 0x004B2B10  (feb 0x004A3EB0)
int txmTEXTURE_LAY::Create(int cw, int ch, int cMip, int cFmt)
{
    return txmTEXTURE::Create(cw, ch, cMip, cFmt);
}

// v12 0x004B2B20  (feb 0x004A3EC0)
int txmTEXTURE_LAY::Set(pctPICTURE *p, int layer)
{
    if (!Create(p->w, p->h, p->nMip, p->fmt))
        return 0;
    txmLAYER *L = Layer(layer);
    if (!L->data) {
        L->data = p->data;                              // takes the picture's pixels
        p->data = 0;
    } else
        memcpy(L->data, p->data, pctGetMemSize(p, -1));
    L->flags |= TXM_F_LOADED;
    if (layer == -1 || layer == 0)
        flags |= TXM_F_LOADED;
    return 1;
}

// v12 0x004B2BD0  (feb 0x004A3F70)
int txmTEXTURE_LAY::Load(char *path, char *ext, int layer)
{
    int last, i;
    if (layer == -1) {
        last = nLayer - 1;
        i = 0;
    } else {
        last = layer;
        i = layer;
    }
    if (nLayer == 1 && (flags & TXM_F_LOADED))
        return 1;
    for (; i <= last; i++) {
        txmLAYER *L = &lay[i];
        if (L->flags & (TXM_F_LOADED | TXM_F_NO_FILE))
            continue;
        pctPICTURE *p = ReadResample(path, L->name, ext, flags & TXM_F_CREATED);
        if (!p)
            continue;
        if (!Set(p, i))
            return 0;
        pctDestroy(p);
    }
    u32 f = flags;
    flags = f & 0xFFFFCFCF;
    if (!(f & 0xB))
        flags |= 0xB;
    if (layer == -1 && (flags & TXM_F_CREATED) && lay[0].data) {
        pctPICTURE pic;
        pic.magic = PCT_MAGIC;
        pic.w = w;
        pic.h = h;
        pic.nMip = nMip;
        pic.fmt = fmt;
        pic.extra.flags = 0;                            // (uninitialised in the game)
        pic.extra.value = 0;
        pic.data = lay[0].data;
        if (LoadDrv(&pic)) {
            for (int k = 0; k < nLayer; k++) {
                if (k == 0) {
                    if (!(fabsf(lay[0].weight - 1.0f) < txmEps))
                        lay[0].flags |= TXM_F_DIRTY;
                    lay[0].weight = 1.0f;
                } else {
                    if (!(fabsf(lay[k].weight) < txmEps))
                        lay[k].flags |= TXM_F_DIRTY;
                    lay[k].weight = 0;
                }
                lay[k].flags &= ~TXM_F_DIRTY;
            }
        }
    }
    if (nLayer == 1) {
        if (!(txmManager->flags & 1))
            Unload(0);
        flags |= TXM_F_LOADED;
    }
    return 1;
}

// v12 0x004B2E10  (feb 0x004A41B0)
int txmTEXTURE_LAY::Save(char *dir, char *ext, int layer)
{
    u32 f = flags;
    if (!(f & TXM_F_CREATED) || !lay[layer].data)
        return 0;
    pctPICTURE pic;
    pic.magic = PCT_MAGIC;
    pic.w = w;
    pic.h = h;
    pic.nMip = nMip;
    pic.fmt = fmt;
    pic.extra.flags = 0;
    if (f & TXM_F_EXTRA_04)
        pic.extra.flags = 4;
    if (f & TXM_F_CLAMP_U)
        pic.extra.flags |= 8;
    if (f & TXM_F_CLAMP_V)
        pic.extra.flags |= 0x10;
    pic.extra.value = 0;                                // (uninitialised in the game)
    pic.data = lay[layer].data;
    return pctWrite(fnmMakeFullName(dir, lay[layer].name, ext, 0), &pic) != 0;
}

// v12 0x004B2EE0  (feb 0x004A4280)
void txmTEXTURE_LAY::Unload(int layer)
{
    txmLAYER *L = &lay[layer];
    if (L->flags & TXM_F_LOADED) {
        apFree(L->data);
        L->data = 0;
        L->flags &= ~TXM_F_LOADED;
    }
}

// v12 0x004B2F30  (feb 0x004A42D0)
int txmTEXTURE_LAY::GetColor(int x, int y, pctPIXEL *c, int layer)
{
    if (layer != -1 && (flags & TXM_F_CREATED) && lay[layer].data) {
        pctPICTURE pic;
        pic.magic = PCT_MAGIC;
        pic.w = w;
        pic.h = h;
        pic.nMip = nMip;
        pic.fmt = fmt;
        pic.data = lay[layer].data;
        pctGetPixel(&pic, x, y, 0, c);
    }
    return 1;
}

// v12 0x004B2FB0  (feb 0x004A4350)
int txmTEXTURE_LAY::SetColor(int x, int y, pctPIXEL *c, int layer)
{
    if (layer != -1 && (flags & TXM_F_CREATED) && lay[layer].data) {
        pctPICTURE pic;
        pic.magic = PCT_MAGIC;
        pic.w = w;
        pic.h = h;
        pic.nMip = nMip;
        pic.fmt = fmt;
        pic.data = lay[layer].data;
        pctSetPixel(&pic, x, y, 0, c);
    }
    return 1;
}

// v12 0x004B3030  (feb 0x004A43D0)
int txmTEXTURE_LAY::Add(char *layerName)
{
    int i = 0;
    for (; i < nLayer; i++)
        if (((apNAME *)lay[i].name)->IsName(layerName))
            break;
    if (i >= nLayer) {
        if (nLayer > TXM_MAX_LAYER - 1)
            return -1;
        ((apNAME *)lay[nLayer].name)->SetName(layerName);
        i = nLayer;
        nLayer++;
    }
    if ((flags & TXM_F_CREATED) && !lay[i].data) {
        pctHEADER hd;
        hd.magic = PCT_MAGIC;
        hd.w = w;
        hd.h = h;
        hd.nMip = nMip;
        hd.fmt = fmt;
        lay[i].data = (u8 *)apMalloc(pctGetMemSize(&hd, -1));
        if (!lay[i].data)
            return -1;
    }
    if (i == nLayer)
        nLayer++;
    return i;
}

// v12 0x004B3120  (feb 0x004A44C0)
int txmTEXTURE_LAY::Update(float dt)
{
    flags |= TXM_F_DIRTY;
    return 1;
}

int txmTEXTURE_LAY::unk3C(int, int, int, int, int) { return 1; }        // v12 0x004EADB0

// ------------------------------------------------------------------------------------------------
// txmMANAGER

// v12 0x004B3140  (feb 0x004A44E0)
txmMANAGER::txmMANAGER()
{
    flags = 0;
    searchPath[0] = 0;
    ext[0] = 0;
    nextFree = 0;
    for (int i = 0; i < TXM_MAX_TEX; i++)
        tex[i] = 0;
    white = 0;
    whiteLM = 0;
    black = 0;
    detailNeutral = 0;
}

// (feb 0x004A4580) ~txmMANAGER, inlined into the deleting destructor
void txmMANAGER::Destroy()
{
    for (int i = 0; i < TXM_MAX_TEX; i++) {
        txmTEXTURE *t = tex[i];
        if (t) {
            tex[t->listNmb] = 0;
            t->__vecDelDtor(1);
        }
    }
}

// v12 0x004B3190  (feb 0x004A4530)
void *txmMANAGER::__vecDelDtor(unsigned f)
{
    Destroy();
    if (f & 1)
        operator delete(this);
    return this;
}

int  txmMANAGER::HwInit() { return 1; }                                   // v12 0x004031F0
void txmMANAGER::HwTerm() {}                                              // v12 0x00403A20
void txmMANAGER::SetSource(txmTEXTURE *t) {}                              // v12 0x00403950
int  txmMANAGER::HwDownload(txmTEXTURE *t) { return 1; }                  // v12 0x0048D590
void txmMANAGER::HwDelete(txmTEXTURE *t) {}                               // v12 0x00403950

// v12 0x004B3220  (feb 0x004A45C0)
int txmMANAGER::Init(char *texPath, char *e)
{
    pctPIXEL c;
    strcpy(searchPath, texPath);
    strcpy(ext, e);
    white = Add((char *)"white", 0xC00, TXM_ADD_PLAIN);
    if (!white || !white->Create(1, 1, 1, PCT_RGB565))
        return 0;
    c.a = 0;
    c.b = 0xFF;
    c.g = 0xFF;
    c.r = 0xFF;
    if (!white->Fill(&c, -1))
        return 0;
    whiteLM = Add((char *)"whiteLM", 0xC00, TXM_ADD_PLAIN);
    if (!whiteLM || !whiteLM->Create(1, 1, 1, PCT_RGB565))
        return 0;
    whiteLM->flags |= TXM_F_EXTRA_04;
    if (!whiteLM->Fill(&c, -1))
        return 0;
    black = Add((char *)"black", 0xC00, TXM_ADD_PLAIN);
    if (!black || !black->Create(1, 1, 1, PCT_RGB565))
        return 0;
    c.b = 0;
    c.g = 0;
    c.r = 0;
    if (!black->Fill(&c, -1))
        return 0;
    detailNeutral = Add((char *)"detailNeutral", 0xC00, TXM_ADD_PLAIN);
    if (!detailNeutral || !detailNeutral->Create(1, 1, 1, PCT_ARGB8888))
        return 0;
    c.b = 0;
    c.g = 0;
    c.r = 0;
    c.a = 0;
    if (!detailNeutral->Fill(&c, -1))
        return 0;
    detailNeutral->flags |= TXM_F_EXTRA_02;
    return 1;
}

// v12 0x004B3410  (feb 0x004A47B0)
txmTEXTURE *txmMANAGER::Add(char *n, int f, txmADD_MODE mode)
{
    pctEXTRA_DATA ex;
    pctHEADER hd;
    char path[0x104];
    txmTEXTURE *t;
    for (int i = 0; i < TXM_MAX_TEX; i++) {
        if (tex[i] && !stricmp(tex[i]->name, n)) {
            t = tex[i];
            if (!t)
                break;
            if (mode == TXM_ADD_LAYERED) {
                if (t->magic != TXM_MAGIC_TEX)
                    goto found;
            } else if (t->magic != TXM_MAGIC_LAYER)
                goto found;
            return 0;                                   // asked layered for a plain texture, or the opposite
        }
    }
    if (mode == TXM_ADD_LAYERED)
        return Alloc(n, f, 1);
    if (mode == TXM_ADD_HEADER || mode == TXM_ADD_LOAD) {
        char *s = fnmFindDisc(searchPath, n, ext);
        if (!s) {
            apLog((char *)"Texture %s not found", n);
            return 0;
        }
        strcpy(path, s);
    }
    if (mode == TXM_ADD_HEADER && !pctReadHdr(path, &hd, &ex))
        return 0;
    t = Alloc(n, f, 0);
    if (!t)
        return 0;
    if (mode == TXM_ADD_HEADER)
        t->SetTexParams(&hd, &ex);
    else if (mode == TXM_ADD_LOAD && !Load(t, 1)) {
        tex[t->listNmb] = 0;
        t->__vecDelDtor(1);
        return 0;
    }
found:
    t->flags |= TXM_F_ADDED;
    return t;
}

// v12 0x004B35C0  (feb 0x004A4960)
void txmMANAGER::Del(txmTEXTURE *t)
{
    if (t) {
        tex[t->listNmb] = 0;
        t->__vecDelDtor(1);
    }
}

// v12 0x004B35F0  (feb 0x004A4990)
txmTEXTURE *txmMANAGER::Find(char *n)
{
    for (int i = 0; i < TXM_MAX_TEX; i++)
        if (tex[i] && !stricmp(tex[i]->name, n))
            return tex[i];
    return 0;
}

// v12 0x004B3640  (feb 0x004A49E0)
txmTEXTURE *txmMANAGER::Alloc(char *n, int f, int layered)
{
    for (;;) {
        int start = nextFree, i = start;
        for (; i < TXM_MAX_TEX; i++)
            if (!tex[i])
                break;
        if (i != TXM_MAX_TEX) {
            txmTEXTURE *t = layered ? MakeTexLayer(n, f) : MakeTex(n, f);
            tex[i] = t;
            if (!t)
                return 0;
            t->listNmb = i;
            nextFree = i + 1;
            return tex[i];
        }
        if (start < 1)
            return 0;
        nextFree = 0;                                   // wrap once
    }
}

// v12 0x004B36D0  (feb 0x004A4A70)
int txmMANAGER::Load(txmTEXTURE *t, int retry)
{
    char full[0x104], dir[0x104];
    if ((t->flags & TXM_F_LOADED) || (t->flags & TXM_F_NO_FILE))
        return 1;
    if (!retry && (t->flags & TXM_F_NOT_FOUND))
        return 0;
    if (!t->FindDisk(searchPath, ext, full)) {
        apMsg((char *)"Cannot locate texture %s", t->name);
        t->flags |= TXM_F_NOT_FOUND;
        return 0;
    }
    strcpy(dir, fnmGetPath(full));
    if (!t->Load(dir, ext, -1)) {
        apMsg((char *)"Cannot load texture %s", t->name);
        return 0;
    }
    t->flags &= ~TXM_F_NOT_FOUND;
    return 1;
}

// v12 0x004B37C0  (feb 0x004A4B60)
int txmMANAGER::LoadAll(int (__fastcall *onFail)(txmTEXTURE *))
{
    int size = 0, nFail = 0;
    for (int i = 0; i < TXM_MAX_TEX; i++) {
        txmTEXTURE *t = tex[i];
        if (t && !(t->flags & TXM_F_UNLOADABLE)) {
            if (!Load(t, 1)) {
                if (onFail)
                    onFail(t);                          // (the game passes a stale register, not t)
                nFail++;
            } else
                size += tex[i]->memSize;
        }
    }
    apLog((char *)"Cur Texure Size %d", size);
    return nFail;
}

// v12 0x004B3830  (feb 0x004A4BD0)
int txmMANAGER::PurgeAll()
{
    int n = 0;
    for (int i = 0; i < TXM_MAX_TEX; i++) {
        txmTEXTURE *t = tex[i];
        if (!t || (t->flags & 0xE00))
            continue;
        if (t->flags & TXM_F_SCENE) {
            apLog((char *)"Texture %s purged from texture list.", t->name);
            if (tex[i]) {
                txmTEXTURE *d = tex[i];
                tex[d->listNmb] = 0;
                d->__vecDelDtor(1);
            }
        } else {
            if (!(t->flags & TXM_F_UNLOADABLE) || !(t->flags & TXM_F_LOADED))
                continue;
            apLog((char *)"Texture %s unloaded from memory.", t->name);
            tex[i]->Unload(-1);
        }
        n++;
    }
    return n;
}

// v12 0x004B38D0  (feb 0x004A4C70)
void txmMANAGER::MngTexState(int bits, txmTEX_STATE_OPER op)
{
    for (int i = 0; i < TXM_MAX_TEX; i++) {
        txmTEXTURE *t = tex[i];
        if (!t)
            continue;
        if (op == TXM_STATE_SET)
            t->flags |= bits;
        else if (op == TXM_STATE_CLEAR)
            t->flags &= ~bits;
        else if (op == TXM_STATE_TOGGLE)
            t->flags ^= bits;
    }
}

// v12 0x004B3930  (feb 0x004A4CD0)
txmTEXTURE *txmMANAGER::GetNext(txmTEXTURE *t)
{
    int i = t ? t->listNmb + 1 : 0;
    for (; i < TXM_MAX_TEX; i++)
        if (tex[i])
            return tex[i];
    return 0;
}

// v12 0x004B3980  (feb 0x004A4D20)
void txmMANAGER::NewFrame(float dt)
{
    for (int i = 0; i < TXM_MAX_TEX; i++) {
        if (tex[i]) {
            tex[i]->flags &= ~TXM_F_USED_FRAME;
            tex[i]->Update(dt);
        }
    }
}

// v12 0x004B39D0  (feb 0x004A4D70)
txmTEXTURE *txmMANAGER::MakeTex(char *n, int f)
{
    void *mem = operator new(sizeof(txmTEXTURE));
    return mem ? new (mem) txmTEXTURE(n, f) : 0;
}

// v12 0x004B3A40  (feb 0x004A4DE0)
txmTEXTURE *txmMANAGER::MakeTexLayer(char *n, int f)
{
    void *mem = operator new(sizeof(txmTEXTURE_LAY));
    return mem ? new (mem) txmTEXTURE_LAY(n, f) : 0;
}
