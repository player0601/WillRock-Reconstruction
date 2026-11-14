// pic_decompress.cpp -- engine.lib DXT block decoding, one pixel at a time
// (Will Rock 1.2, v12.exe; feb obj engine:pic_decompress.obj)
//
// v12 0x00505DD0..0x005064AF. The last colour block and the last alpha block are cached with their
// derived colours / alphas (statics below). DXT1 (OXT1 / AXT1): 3-colour mode with transparent black when
// c0 <= c1. DXT3: 4-bit alpha shifted up (not replicated). DXT5: 8 or 6 interpolated alphas.
// XT2 / XT4 pixels are not decoded (left as they were).

#include "engine.h"
#include "../../include/engine_render/picture.h"

static u8 *s_colBlock;                 // v12 0x00984E3C (feb 0x00940C00) cached colour block
static u16 s_col[2];                   // v12 0x00984E40 (feb 0x00940C04) c0, c1
static u16 s_col2, s_col3a, s_col3b;   // v12 0x00984E44 / 46 / 48: (c0+c1)/2, (2c0+c1)/3, (c0+2c1)/3
static u8 *s_alphaBlock;               // v12 0x00984E4C (feb 0x00940C10) cached DXT5 alpha block
static u8  s_alpha[8];                 // v12 0x00984E50 (feb 0x00940C14)

static const u8  s_codeMask[4]  = { 0x03, 0x0C, 0x30, 0xC0 };              // v12 0x00634B84
static const int s_codeShift[4] = { 0, 2, 4, 6 };                          // v12 0x00634B88
static const u8  s_a4Mask[2]    = { 0x0F, 0xF0 };                          // v12 0x00634B98
static const int s_a4Shift[2]   = { 4, 0 };                                // v12 0x00634B9C
static const u8  s_a3MaskLo[8]  = { 0x07, 0x38, 0xC0, 0x0E, 0x70, 0x80, 0x1C, 0xE0 }; // v12 0x00634BA4
static const u8  s_a3MaskHi[8]  = { 0x00, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x00 }; // v12 0x00634BAC
static const int s_a3ShiftR[8]  = { 0, 3, 6, 1, 4, 7, 2, 5 };              // v12 0x00634BB4
static const int s_a3ShiftL[8]  = { 0, 0, 2, 0, 0, 1, 0, 0 };              // v12 0x00634BD4

// v12 0x00505DD0  (feb 0x004F37C0)
void __fastcall pctGetDXTPixel(pctPICTURE *p, int x, int y, int mip, pctPIXEL *c)
{
    u8 *base = p->data + ((u32 *)p->data)[mip];
    int bw = (p->w + 3) >> (mip + 2);                   // blocks per row
    if ((unsigned)(p->fmt - PCT_DXT1) >= 6)
        return;
    int bx = x >> 2, by = y >> 2;
    u16 col;
    u8 alpha, dummy;
    switch (p->fmt) {
    default: {                                          // OXT1 / AXT1
        u8 *b = base + (by * bw + bx) * 8;
        if (s_colBlock != b) {
            s_col[0] = ((u16 *)b)[0];
            s_col[1] = ((u16 *)b)[1];
            s_colBlock = b;
            s_col2  = pctLineCombColor565(((u16 *)b)[0], 1, ((u16 *)b)[1], 1, 2);
            s_col3a = pctLineCombColor565(s_col[0], 2, s_col[1], 1, 3);
            s_col3b = pctLineCombColor565(s_col[0], 1, s_col[1], 2, 3);
        }
        u8 bits = b[4 + (y & 3)];
        pctGetPairByCode((bits & s_codeMask[x & 3]) >> s_codeShift[x & 3], &col, &alpha);
        break;
    }
    case PCT_DXT2:
    case PCT_DXT4:
        return;
    case PCT_DXT3: {
        u8 *b = base + (by * bw + bx) * 16;
        alpha = (u8)((b[((x >> 1) & 1) + (y & 3) * 2] & s_a4Mask[x & 1]) << s_a4Shift[x & 1]);
        UpdateCache1(b + 8);
        u8 bits = b[0xC + (y & 3)];
        pctGetPairByCode((bits & s_codeMask[x & 3]) >> s_codeShift[x & 3], &col, &dummy);
        break;
    }
    case PCT_DXT5: {
        u8 *b = base + (by * bw + bx) * 16;
        UpdateCache5(b);
        int k = (x & 3) + (y & 3) * 4;
        int bit = k * 3;
        int j = k & 7;
        u8 *a = b + (bit >> 3) + 2;
        u8 ai = (u8)(((s_a3MaskLo[j] & a[0]) >> s_a3ShiftR[j]) | ((s_a3MaskHi[j] & a[1]) << s_a3ShiftL[j]));
        u8 a8 = s_alpha[ai];
        UpdateCache1(b + 8);
        pctGetPairByCode((b[0xC + (y & 3)] & s_codeMask[x & 3]) >> s_codeShift[x & 3], &col, &dummy);
        pctDecode(col, a8, c);
        return;
    }
    }
    c->a = alpha;
    c->r = (u8)(((col >> 11) & 0x1F) * 0xFF / 0x1F);
    c->g = (u8)(((col >> 5) & 0x3F) * 0xFF / 0x3F);
    c->b = (u8)((col & 0x1F) * 0xFF / 0x1F);
}

// v12 0x00506090  (feb 0x004F3A80)
u16 __fastcall pctLineCombColor565(u16 c0, u32 w0, u16 c1, u32 w1, u32 div)
{
    u32 g = (((c1 >> 5) & 0x3F) * w1 + ((c0 >> 5) & 0x3F) * w0) / div & 0x3F;
    u32 r = ((c1 >> 11) * w1 + (c0 >> 11) * w0) / div & 0x1F;
    u32 b = ((c1 & 0x1F) * w1 + (c0 & 0x1F) * w0) / div & 0x1F;
    return (u16)(((g | (r << 6)) << 5) | b);
}

// v12 0x00506120  (feb 0x004F3B10)
void __fastcall UpdateCache1(u8 *block)
{
    if (s_colBlock == block)
        return;
    u16 c1 = *(u16 *)(block + 2);
    s_col[0] = *(u16 *)block;
    s_col[1] = c1;
    u32 g1 = (c1 >> 5) & 0x3F, g0 = (s_col[0] >> 5) & 0x3F;
    u32 b0 = s_col[0] & 0x1F, r1 = c1 >> 11, r0 = s_col[0] >> 11, b1 = c1 & 0x1F;
    s_col2  = (u16)(((((r0 + r1) >> 1) << 6 | ((g0 + g1) >> 1)) << 5) | ((b0 + b1) >> 1));
    s_col3a = (u16)(((((r1 + r0 * 2) / 3) << 6 | (((g1 + g0 * 2) / 3) & 0x3F)) << 5) | (((b1 + b0 * 2) / 3) & 0x1F));
    s_col3b = (u16)(((((r0 + r1 * 2) / 3) << 6 | (((g0 + g1 * 2) / 3) & 0x3F)) << 5) | (((b0 + b1 * 2) / 3) & 0x1F));
    s_colBlock = block;
}

// v12 0x00506250  (feb 0x004F3C40)
void __fastcall pctGetPairByCode(u8 code, u16 *color, u8 *alpha)
{
    *alpha = 0xFF;
    if (s_col[1] < s_col[0]) {                          // 4 colours
        switch (code) {
        case 0: *color = s_col[0]; return;
        case 1: *color = s_col[1]; return;
        case 2: *color = s_col3a; return;
        case 3: *color = s_col3b; return;
        }
        return;
    }
    switch (code) {                                     // 3 colours + transparent
    case 0: *color = s_col[0]; return;
    case 1: *color = s_col[1]; return;
    case 2: *color = s_col2; return;
    case 3:
        *alpha = 0;
        *color = 0;
        return;
    }
}

// v12 0x00506300  (feb 0x004F3CF0)
void __fastcall UpdateCache5(u8 *block)
{
    if (s_alphaBlock == block)
        return;
    s_alphaBlock = block;
    u32 a1 = block[1], a0 = block[0];
    s_alpha[0] = block[0];
    s_alpha[1] = block[1];
    if (a1 < a0) {
        s_alpha[2] = (u8)((a1 + a0 * 6) / 7);
        s_alpha[3] = (u8)((a0 * 5 + a1 * 2) / 7);
        s_alpha[4] = (u8)((a1 * 3 + a0 * 4) / 7);
        s_alpha[5] = (u8)((a0 * 3 + a1 * 4) / 7);
        s_alpha[6] = (u8)((a1 * 5 + a0 * 2) / 7);
        s_alpha[7] = (u8)((a0 + a1 * 6) / 7);
        return;
    }
    s_alpha[2] = (u8)((a1 + a0 * 4) / 5);
    s_alpha[6] = 0;
    s_alpha[7] = 0xFF;
    s_alpha[3] = (u8)((a0 * 3 + a1 * 2) / 5);
    s_alpha[4] = (u8)((a1 * 3 + a0 * 2) / 5);
    s_alpha[5] = (u8)((a0 + a1 * 4) / 5);
}

// v12 0x00506460  (feb 0x004F3E50)
void __fastcall pctDecode(u16 color, u8 alpha, pctPIXEL *c)
{
    c->a = alpha;
    c->r = (u8)((u32)(color >> 11) * 0xFF / 0x1F);
    c->g = (u8)(((color & 0x7E0) >> 5) * 0xFF / 0x3F);
    c->b = (u8)((color & 0x1F) * 0xFF / 0x1F);
}
