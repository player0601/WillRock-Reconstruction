// picture.cpp -- engine.lib pictures: the format table, .pct read/write, pixel access, copies into
// driver surfaces
// (Will Rock 1.2, v12.exe; feb obj engine:picture.obj)
//
// v12 0x004F7FC0..0x004FBB6F (beta decompile + the 1.2 code for the float conversions). Not rebuilt yet:
// pctConvert, pctGetFromBuf, pctGenMipMaps, pctGenMipMapLevel, pctResample, pctCalcAveragePixel (the
// texture loader only calls them when the driver asks for another size, format or mip count).
// Channel conversions truncate (__ftol): to 8 bits v * 255 / (2^bits - 1); between two packed formats
// v * (2^dstBits - 1) / (2^srcBits - 1), skipped when that ratio is exactly 1. Kept as in the game:
// pctSetPixel scales by the float 1/255 (0x3B808081, a little below 1/255), so 255 is stored as 254 in
// an 8-bit channel; I8 has bits = 0 and AI44 an alpha shift of 8, so neither decodes; pctRead frees *data
// on failure even when the caller passed no data pointer (pctReadHdr).

#include "engine.h"
#include "../../include/engine_render/picture.h"

pctFORMAT pctFormat[PCT_NFORMAT];      // v12 0x00978770 (feb 0x00934530)
int       pctInited;                   // v12 0x00978F50 (feb 0x00934D10)

static void pctSetFormat(int i, int nComp, u32 flags, int bpp, u32 mA, u32 mR, u32 mG, u32 mB,
                         int sA, int sR, int sG, int sB, int bA, int bR, int bG, int bB, const char *name)
{
    pctFORMAT *f = &pctFormat[i];
    f->nComp = nComp;
    f->flags = flags;
    f->bpp = bpp;
    f->mask[0] = mA; f->mask[1] = mR; f->mask[2] = mG; f->mask[3] = mB;
    f->shift[0] = sA; f->shift[1] = sR; f->shift[2] = sG; f->shift[3] = sB;
    f->bits[0] = (u8)bA; f->bits[1] = (u8)bR; f->bits[2] = (u8)bG; f->bits[3] = (u8)bB;
    strncpy(f->name, name, 0x3F);
    f->name[0x3E] = 0;
}

// v12 0x004F7FC0  (feb 0x004E5AB0)
int __fastcall pctInit()
{
    //            fmt            n  flags bpp  masks A R G B                          shifts A R G B  bits A R G B
    pctSetFormat(PCT_ARGB8888, 4, 1, 4, 0xFF000000, 0xFF0000, 0xFF00, 0xFF,  0x18, 0x10, 8, 0,  8, 8, 8, 8, "ARGB8888");
    pctSetFormat(PCT_ARGB4444, 4, 1, 2, 0xF000, 0xF00, 0xF0, 0xF,            0xC, 8, 4, 0,      4, 4, 4, 4, "ARGB4444");
    pctSetFormat(PCT_RGB888,   3, 0, 3, 0, 0xFF0000, 0xFF00, 0xFF,           0, 0x10, 8, 0,     0, 8, 8, 8, "RGB888");
    pctSetFormat(PCT_A8,       1, 1, 1, 0xFF, 0, 0, 0,                        0, 0, 0, 0,        8, 0, 0, 0, "A8");
    pctSetFormat(PCT_RGB332,   3, 0, 1, 0, 0xE0, 0x1C, 3,                     0, 5, 2, 0,        0, 3, 3, 2, "RGB332");
    pctSetFormat(PCT_RGB565,   3, 0, 2, 0, 0xF800, 0x7E0, 0x1F,               0, 0xB, 5, 0,      0, 5, 6, 5, "RGB565");
    pctSetFormat(PCT_ARGB8332, 4, 1, 2, 0xFF00, 0xE0, 0x1C, 3,                8, 5, 2, 0,        8, 3, 3, 2, "ARGB8332");
    pctSetFormat(PCT_ARGB1555, 4, 1, 2, 0x8000, 0x7C00, 0x3E0, 0x1F,          0xF, 0xA, 5, 0,    1, 5, 5, 5, "ARGB1555");
    pctSetFormat(PCT_I8,       1, 2, 1, 0, 0xFF, 0, 0,                        0, 0, 0, 0,        0, 0, 0, 0, "I8");
    pctSetFormat(PCT_AI44,     2, 3, 1, 0xF0, 0xF, 0, 0,                      8, 0, 0, 0,        4, 4, 0, 0, "AI44");
    pctSetFormat(PCT_AI88,     2, 3, 2, 0xFF00, 0xFF, 0, 0,                   8, 0, 0, 0,        8, 8, 0, 0, "AI88");
    pctSetFormat(PCT_UV88,     2, 4, 2, 0xFF, 0xFF00, 0, 0,                   0, 8, 0, 0,        8, 8, 0, 0, "UV88");
    pctSetFormat(PCT_DXT1,     3, 8, 0, 0, 0, 0, 0,                           0, 0, 0, 0,        0, 0, 0, 0, "OXT1");
    pctSetFormat(PCT_DXT1A,    4, 8, 0, 0, 0, 0, 0,                           0, 0, 0, 0,        0, 0, 0, 0, "AXT1");
    pctSetFormat(PCT_DXT2,     4, 0x18, 0, 0, 0, 0, 0,                        0, 0, 0, 0,        0, 0, 0, 0, "XT2 ");
    pctSetFormat(PCT_DXT3,     4, 8, 0, 0, 0, 0, 0,                           0, 0, 0, 0,        0, 0, 0, 0, "XT3 ");
    pctSetFormat(PCT_DXT4,     4, 0x18, 0, 0, 0, 0, 0,                        0, 0, 0, 0,        0, 0, 0, 0, "XT4 ");
    pctSetFormat(PCT_DXT5,     4, 8, 0, 0, 0, 0, 0,                           0, 0, 0, 0,        0, 0, 0, 0, "XT5 ");
    pctInited = 1;
    return 1;
}

// The byte size of one mip level (inlined everywhere in the game, out of line in pctGetMipMapMemSize)
static inline int pctLevelSize(pctHEADER *h, int mip)
{
    int w, ww, hh;
    switch (h->fmt) {
    case PCT_DXT1:
    case PCT_DXT1A:
        ww = h->w >> mip; if (ww < 4) ww = 4;
        hh = h->h >> mip; if (hh < 4) hh = 4;
        return (hh * ww * 4 + 7) >> 3;
    case PCT_DXT2:
    case PCT_DXT4:
        return 4;
    case PCT_DXT3:
    case PCT_DXT5:
        ww = h->w >> mip; if (ww < 4) ww = 4;
        hh = h->h >> mip; if (hh < 4) hh = 4;
        return hh * ww;
    default:
        w = h->w >> mip;
        return pctFormat[h->fmt].bpp * (h->h >> mip) * w;
    }
}
// The DXT offset table in front of the blocks: one u32 per level, padded to 16 bytes
static inline int pctDXTTable(int fmt, int nMip)
{
    switch (fmt) {
    case PCT_DXT1:
    case PCT_DXT1A:
    case PCT_DXT3:
    case PCT_DXT5:
        return (nMip * 4 + 0xF) & ~0xF;
    }
    return 0;
}

// v12 0x004F8660  (feb 0x004E6150)
int __fastcall pctReadHdr(char *name, pctHEADER *h, pctEXTRA_DATA *e)
{
    if (h)
        memset(h, 0, sizeof(*h));
    if (e) {
        e->flags = 0;
        e->value = 0;
    }
    return pctRead(name, h, e, 0);
}

// v12 0x004F86A0  (feb 0x004E6190)
pctPICTURE *__fastcall pctRead(char *name)
{
    pctPICTURE *p = (pctPICTURE *)apCalloc(1, sizeof(pctPICTURE));
    if (!p)
        return 0;
    if (!pctRead(name, p, &p->extra, (void **)&p->data)) {
        apFree(p->data);
        apFree(p);
        return 0;
    }
    return p;
}

// v12 0x004F86F0  (feb 0x004E61E0)
pctPICTURE *__fastcall pctRead(fioFILE *f)
{
    pctPICTURE *p = (pctPICTURE *)apCalloc(1, sizeof(pctPICTURE));
    if (!p)
        return 0;
    if (!pctRead(f, p, &p->extra, (void **)&p->data)) {
        apFree(p->data);
        apFree(p);
        return 0;
    }
    return p;
}

// v12 0x004F8740  (feb 0x004E6230)
int __fastcall pctRead(char *name, pctHEADER *h, pctEXTRA_DATA *e, void **data)
{
    fioFILE_DISK f;
    if (!f.Init(name, FIO_READ | FIO_CHUNKS))
        return 0;
    if (pctRead(&f, h, e, data) && f.Term())
        return 1;
    if (*data)                                          // (original: dereferenced even when data is 0)
        apFree(*data);
    f.Term();
    return 0;
}

// v12 0x004F8810  (feb 0x004E6300)
int __fastcall pctRead(fioFILE *f, pctHEADER *h, pctEXTRA_DATA *e, void **data)
{
    fioCHUNK c;
    u32 skip;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    if (f->ReadChunk(0) == 0xF0) {
        f->ReadData(&h->magic, 4, 4);
        if (h->magic == PCT_MAGIC) {
            BOOL32 eof = f->IsEOF();
            for (;;) {
                if (eof || (f->flags & FIO_ERROR))
                    return 1;
                f->ReadChunk(&c);
                if (c.id == 1)
                    return 1;
                switch (c.id) {
                case 0xF1:
                    f->ReadData(&h->w, 4, 4);
                    f->ReadData(&h->h, 4, 4);
                    break;
                case 0xF2:
                    f->ReadData(&h->fmt, 4, 4);
                    break;
                case 0xF3:
                    f->ReadData(&skip, 4, 4);
                    h->nMip = 1;
                    break;
                case 0xF4: {
                    int size = 0;
                    for (int i = 0; i < h->nMip; i++)
                        size += pctLevelSize(h, i);
                    size += pctDXTTable(h->fmt, h->nMip);
                    if (!data)
                        f->Seek(size, FIO_SEEK_CUR);
                    else {
                        *data = apCalloc(1, size);
                        if (!*data)
                            goto term;
                        f->ReadData(*data, size, 1);
                    }
                    break;
                }
                case 0xF5:
                case 0xF6:
                case 0xF7:
                case 0xF8:
                    f->ReadData(&skip, 4, 4);
                    break;
                case 0xF9:
                    f->ReadData(&h->nMip, 4, 4);
                    break;
                case 0xFA:
                    if (e)
                        f->ReadData(&e->flags, 4, 4);
                    else
                        f->SkipChunk(&c);
                    break;
                case 0xFB:
                    if (e)
                        f->ReadData(&e->value, 4, 4);
                    else
                        f->SkipChunk(&c);
                    break;
                default:
                    goto fail;
                }
                eof = f->IsEOF();
            }
        }
    }
fail:
    if (*data)                                          // (original: dereferenced even when data is 0)
        apFree(*data);
term:
    f->Term();
    return 0;
}

// v12 0x004F8B40  (feb 0x004E6630)
int __fastcall pctWrite(char *name, pctPICTURE *p)
{
    fioFILE_DISK f;
    if (!f.Init(name, FIO_WRITE | FIO_CHUNKS))
        return 0;
    pctWrite(&f, p);
    return f.Term() != 0;
}

// v12 0x004F8BC0  (feb 0x004E66B0)
int __fastcall pctWrite(fioFILE *f, pctPICTURE *p)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    f->WriteChunk(0xF0, &c);
    f->WriteData(&p->magic, 4, 4);
    f->UpdateChunk(&c);
    f->WriteChunk(0xF1, &c);
    f->WriteData(&p->w, 4, 4);
    f->WriteData(&p->h, 4, 4);
    f->UpdateChunk(&c);
    f->WriteChunk(0xF2, &c);
    f->WriteData(&p->fmt, 4, 4);
    f->UpdateChunk(&c);
    f->WriteChunk(0xF9, &c);
    f->WriteData(&p->nMip, 4, 4);
    f->UpdateChunk(&c);
    f->WriteChunk(0xFA, &c);
    f->WriteData(&p->extra.flags, 4, 4);
    f->UpdateChunk(&c);
    f->WriteChunk(0xFB, &c);
    f->WriteData(&p->extra.value, 4, 4);
    f->UpdateChunk(&c);
    f->WriteChunk(0xF4, &c);
    int size = 0;
    for (int i = 0; i < p->nMip; i++)
        size += pctLevelSize(p, i);
    size += pctDXTTable(p->fmt, p->nMip);
    f->WriteData(p->data, size, 1);
    f->UpdateChunk(&c);
    f->WriteLastChunk();
    return 1;
}

// v12 0x004F8E80  (feb 0x004E6970)
pctPICTURE *__fastcall pctCreate(pctHEADER *h)
{
    pctPICTURE *p = (pctPICTURE *)apCalloc(1, sizeof(pctPICTURE));
    if (!p)
        return 0;
    h->magic = PCT_MAGIC;
    *(pctHEADER *)p = *h;
    if (p->fmt < PCT_DXT1 || p->fmt > PCT_DXT5)
        p->nMip = 1;
    int size = 0;
    for (int i = 0; i < p->nMip; i++)
        size += pctLevelSize(p, i);
    size += pctDXTTable(p->fmt, p->nMip);
    p->data = (u8 *)apCalloc(1, size);
    return p->data ? p : 0;                             // (the header is leaked when the data fails)
}

// v12 0x004F8FD0  (feb 0x004E6AC0)
void __fastcall pctDestroy(pctPICTURE *p)
{
    if (p) {
        apFree(p->data);
        apFree(p);
    }
}

// v12 0x004F8FF0  (feb 0x004E6AE0)
int __fastcall pctIsAlpha(int fmt)
{
    switch (fmt) {
    case PCT_ARGB8888:
    case PCT_ARGB4444:
    case PCT_A8:
    case PCT_ARGB8332:
    case PCT_ARGB1555:
    case PCT_AI44:
    case PCT_AI88:
    case PCT_DXT1A:
    case PCT_DXT3:
    case PCT_DXT5:
        return 1;
    }
    return 0;
}

// v12 0x004F9030  (feb 0x004E6B20)
pctPICTURE *__fastcall pctConvert(pctPICTURE *p, int fmt)
{
    WR_NOT_REBUILT(0x004F9030);                         // format conversion (and the UV88 bump-map build)
    return 0;
}

// v12 0x004F9620  (feb 0x004E7110)
void __fastcall pctSetPixel(pctPICTURE *p, int x, int y, int mip, pctPIXEL *c)
{
    if (p->fmt >= PCT_DXT1 && p->fmt <= PCT_DXT5)
        return;
    pctFORMAT *f = &pctFormat[p->fmt];
    u8 *dst = p->data + pctGetMemSize(p, mip) + ((p->w >> mip) * y + x) * pctFormat[p->fmt].bpp;
    const float k = 0.0039215688593685627f;             // __real@3b808081
    u32 v;
    if (f->flags & PCT_FMT_UV) {
        u32 a = (u32)(int)((double)((1 << f->bits[1]) - 1) * c->r * k) << f->shift[1];
        v = ((u32)(int)((double)((1 << f->bits[0]) - 1) * c->a * k) << f->shift[0]) + a;
    } else {
        u32 a = 0;
        if (f->flags & PCT_FMT_ALPHA)
            a = (u32)(int)((double)((1 << f->bits[0]) - 1) * c->a * k) << f->shift[0];
        u32 b = (u32)(int)((double)((1 << f->bits[3]) - 1) * c->b * k) << f->shift[3];
        u32 g = (u32)(int)((double)((1 << f->bits[2]) - 1) * c->g * k) << f->shift[2];
        u32 r = (u32)(int)((double)((1 << f->bits[1]) - 1) * c->r * k) << f->shift[1];
        v = g + b + r + a;
    }
    u8 be[4] = { (u8)(v >> 24), (u8)(v >> 16), (u8)(v >> 8), (u8)v };   // stored big-endian
    int skip = 4 - f->bpp;
    if (skip < 4)
        memcpy(dst, be + skip, 4 - skip);
}

// v12 0x004F9870  (feb 0x004E7360)
void __fastcall pctGetPixel(pctPICTURE *p, int x, int y, int mip, pctPIXEL *c)
{
    pctFORMAT *f = &pctFormat[p->fmt];
    if (f->flags & PCT_FMT_DXT) {
        pctGetDXTPixel(p, x, y, mip, c);
        return;
    }
    u8 *s = p->data + pctGetMemSize(p, mip) + ((p->w >> mip) * y + x) * pctFormat[p->fmt].bpp;
    u32 v;
    switch (f->bpp) {
    case 1: v = s[0]; break;
    case 2: v = (s[0] << 8) | s[1]; break;
    case 3: v = (s[0] << 16) | (s[1] << 8) | s[2]; break;
    case 4: v = ((u32)s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3]; break;
    default: v = (u32)(uintptr_t)c; break;              // (original: the pixel pointer)
    }
#define PCT_CH(i) (u8)(int)((double)((v & f->mask[i]) >> f->shift[i]) / (double)((1 << f->bits[i]) - 1) * 255.0)
    if (f->flags & PCT_FMT_UV) {
        c->a = PCT_CH(0);
        c->r = PCT_CH(1);
        return;
    }
    if (f->flags & PCT_FMT_ALPHA)
        c->a = PCT_CH(0);
    else
        c->a = 0xFF;
    if (f->flags & PCT_FMT_INTENSITY) {
        c->r = PCT_CH(1);
        return;
    }
    c->r = PCT_CH(1);
    c->g = PCT_CH(2);
    c->b = PCT_CH(3);
#undef PCT_CH
}

// v12 0x004F9B10  (feb 0x004E7600)
int __fastcall pctGetBPP(pctHEADER *h)
{
    return pctFormat[h->fmt].bpp;
}

// v12 0x004F9B30  (feb 0x004E7620)
pctFORMAT *__fastcall pctGetFormat(pctPICTURE *p)
{
    return &pctFormat[p->fmt];
}

// v12 0x004F9B50  (feb 0x004E7640)
pctFORMAT *__fastcall pctGetFormat(pctHEADER *h)
{
    return &pctFormat[h->fmt];
}

// v12 0x004F9B70  (feb 0x004E7660)
pctFORMAT *__fastcall pctGetFormat(int fmt)
{
    return &pctFormat[fmt];
}

// v12 0x004F9B90  (feb 0x004E7680)
int __fastcall pctGetMipMapMemSize(pctHEADER *h, int mip)
{
    return pctLevelSize(h, mip);
}

// v12 0x004F9C50  (feb 0x004E7740)
int __fastcall pctGetMemSize(pctHEADER *h, int nMip)
{
    if (nMip == -1)
        nMip = h->nMip;
    int size = 0;
    for (int i = 0; i < nMip; i++)
        size += pctLevelSize(h, i);
    return size + pctDXTTable(h->fmt, nMip);
}

// The low bit and the width of a contiguous channel mask (0, 0 when the mask is empty or has holes)
static void pctMaskBits(u32 mask, int *shift, int *bits)
{
    *shift = 0;
    *bits = 0;
    if (!mask)
        return;
    int lo = 0;
    while (lo < 32 && !(mask & (1u << lo)))
        lo++;
    if (lo == 32)
        return;
    int hi = lo;
    for (; hi < 32; hi++)
        if (!(mask & (1u << hi))) {
            hi--;
            break;
        }
    if (hi == 32)
        hi = 31;
    else if (hi < 31 && mask != ((1u << (hi - lo + 1)) - 1) << lo)
        return;
    *shift = lo;
    *bits = hi - lo + 1;
}

static inline void pctStore(u8 *d, int bpp, u32 v)
{
    switch (bpp) {
    case 1: d[0] = (u8)v; break;
    case 2: *(u16 *)d = (u16)v; break;
    case 3: d[0] = (u8)v; d[1] = (u8)(v >> 8); d[2] = (u8)(v >> 16); break;
    case 4: *(u32 *)d = v; break;
    }
}

// v12 0x004F9D60  (feb 0x004E7850)
// A DXT picture into a packed surface (no hardware DXT): every pixel decoded to ARGB8888, then scaled.
void __fastcall pctCopyViaUnpackDXTBuf(pctPICTURE *p, int w, int h, int dstBpp, int mip, int rowPad,
                                       u32 rMask, u32 gMask, u32 bMask, u32 aMask, u8 *dst)
{
    int rs, rb, gs, gb, bs, bb, as, ab;
    pctMaskBits(rMask, &rs, &rb);
    pctMaskBits(gMask, &gs, &gb);
    pctMaskBits(bMask, &bs, &bb);
    pctMaskBits(aMask, &as, &ab);
    pctFORMAT *f0 = &pctFormat[PCT_ARGB8888];
    float ka = f0->bits[0] ? (float)((double)((1 << ab) - 1) / (double)((1 << f0->bits[0]) - 1)) : 1.0f;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            pctPIXEL c;
            pctGetDXTPixel(p, x, y, mip, &c);
            u32 v = (c.g << f0->shift[2]) | (c.a << f0->shift[0]) | (c.b << f0->shift[3]) | (c.r << f0->shift[1]);
            u32 r = (f0->mask[1] & v) >> f0->shift[1];
            float kr = (float)((double)((1 << rb) - 1) / (double)((1 << f0->bits[1]) - 1));
            if (kr != 1.0f)
                r = (u32)(int)((double)r * kr);
            u32 g = (f0->mask[2] & v) >> f0->shift[2];
            float kg = (float)((double)((1 << gb) - 1) / (double)((1 << f0->bits[2]) - 1));
            if (kg != 1.0f)
                g = (u32)(int)((double)g * kg);
            u32 b = (f0->mask[3] & v) >> f0->shift[3];
            float kb = (float)((double)((1 << bb) - 1) / (double)((1 << f0->bits[3]) - 1));
            if (kb != 1.0f)
                b = (u32)(int)((double)b * kb);
            u32 a = (f0->mask[0] & v) >> f0->shift[0];
            if (ka != 1.0f)
                a = (u32)(int)((double)a * ka);
            pctStore(dst, dstBpp, (a << as) + (b << bs) + (g << gs) + (r << rs));
            dst += dstBpp;
        }
        dst += rowPad;
    }
}

// v12 0x004FA270  (feb 0x004E7D60)
// One level of a DXT picture as it is (hardware DXT surfaces)
void __fastcall pctCopyDXTBuf(pctPICTURE *p, int mip, u8 *dst)
{
    int size = pctLevelSize(p, mip);
    memcpy(dst, p->data + ((u32 *)p->data)[mip], size);
}

// v12 0x004FA350  (feb 0x004E7E40)
// One level of a packed picture into a surface of another packed format (UV88: copied as it is)
void __fastcall pctCopyBuf(pctPICTURE *p, int w, int h, int dstBpp, int mip, int rowPad,
                           u32 rMask, u32 gMask, u32 bMask, u32 aMask, u8 *dst)
{
    pctFORMAT *f = &pctFormat[p->fmt];
    if (f->flags & PCT_FMT_DXT)
        return;
    if (p->fmt == PCT_UV88) {
        if (mip == -1)
            mip = p->nMip;
        int ofs = 0;
        for (int i = 0; i < mip; i++)
            ofs += (p->h >> i) * (p->w >> i) * pctFormat[PCT_UV88].bpp;
        u8 *src = p->data + ofs;
        int row = w * dstBpp;
        for (int y = 0; y < h; y++) {
            memcpy(dst, src, row);
            src += row;
            dst += rowPad + row;
        }
        return;
    }
    int rs, rb, gs, gb, bs, bb, as, ab;
    pctMaskBits(rMask, &rs, &rb);
    pctMaskBits(gMask, &gs, &gb);
    pctMaskBits(bMask, &bs, &bb);
    pctMaskBits(aMask, &as, &ab);
    if (mip == -1)
        mip = p->nMip;
    u8 *src = p->data + pctGetMemSize(p, mip);
    float kr = (float)((double)((1 << rb) - 1) / (double)((1 << f->bits[1]) - 1));
    float kg = (float)((double)((1 << gb) - 1) / (double)((1 << f->bits[2]) - 1));
    float kb = (float)((double)((1 << bb) - 1) / (double)((1 << f->bits[3]) - 1));
    double ka = f->bits[0] ? (double)((1 << ab) - 1) / (double)((1 << f->bits[0]) - 1) : 1.0;   // kept on the FPU stack
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            u32 v = 0;
            switch (f->bpp) {
            case 1: v = src[0]; break;
            case 2: v = (src[0] << 8) | src[1]; break;
            case 3: v = (src[0] << 16) | (src[1] << 8) | src[2]; break;
            case 4: v = ((u32)src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3]; break;
            }
            u32 r = (f->mask[1] & v) >> f->shift[1];
            if (kr != 1.0f)
                r = (u32)(int)((double)r * kr);
            u32 g = (f->mask[2] & v) >> f->shift[2];
            if (kg != 1.0f)
                g = (u32)(int)((double)g * kg);
            u32 b = (f->mask[3] & v) >> f->shift[3];
            if (kb != 1.0f)
                b = (u32)(int)((double)b * kb);
            u32 a = (f->mask[0] & v) >> f->shift[0];
            if (ka != 1.0)
                a = (u32)(int)((double)a * ka);
            pctStore(dst, dstBpp, (a << as) + (b << bs) + (g << gs) + (r << rs));
            dst += dstBpp;
            src += f->bpp;
        }
        dst += rowPad;
    }
}

// v12 0x004FA980  (feb 0x004E8470)
void __fastcall pctGetFromBuf(int w, int h, int srcBpp, int rowPad, u32 rMask, u32 gMask, u32 bMask, u32 aMask,
                              u8 *src, pctPICTURE *p)
{
    WR_NOT_REBUILT(0x004FA980);                         // surface -> picture (screenshots, procedural textures)
}

// v12 0x004FAEF0  (feb 0x004E89E0)
int __fastcall pctGenMipMaps(pctPICTURE *p, int nMip, float gamma)
{
    WR_NOT_REBUILT(0x004FAEF0);
    return 0;
}

// v12 0x004FB2B0  (feb 0x004E8DA0)
pctPICTURE *__fastcall pctGenMipMapLevel(pctPICTURE *p, int mip, float gamma)
{
    WR_NOT_REBUILT(0x004FB2B0);
    return 0;
}

// v12 0x004FB650  (feb 0x004E9140)
int __fastcall pctGetMaxMipMapCount(int w, int h)
{
    int n = 1;
    if (w <= 0 || h <= 0)
        return 0;
    while ((w / 2) * 2 == w && (h / 2) * 2 == h) {
        n++;
        w /= 2;
        h /= 2;
    }
    return n;
}

// v12 0x004FB6A0  (feb 0x004E9190)
pctPICTURE *__fastcall pctResample(pctPICTURE *p, int w, int h, int mode)
{
    WR_NOT_REBUILT(0x004FB6A0);
    return 0;
}

// v12 0x004FB900  (feb 0x004E93F0)
void __fastcall pctCalcAveragePixel(pctPICTURE *p, m2dV *uv, pctPIXEL *out)
{
    WR_NOT_REBUILT(0x004FB900);
}
