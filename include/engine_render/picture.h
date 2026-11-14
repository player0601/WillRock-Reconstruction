// Will Rock 1.2 unofficial SDK - engine: pictures (pct*), the in-memory image every texture is loaded through
// objs: engine:picture.obj, pic_decompress.obj, pic_tga.obj, pic_bmp.obj, pic_pic.obj
//
// A .pct file is a chunk file: 0xF0 { u32 'PICT' magic, sub-chunks up to the end chunk }:
//   0xF1 width, height   0xF2 format (pctFORMAT_ID)   0xF9 mip map count   0xFA / 0xFB extra data (two dwords)
//   0xF3 / 0xF5..0xF8 read and dropped (0xF3 also sets 1 mip map)   0xF4 the pixels of every mip level
// Packed pixels are stored big-endian (the first byte is the high byte). DXT pictures start with a
// table of u32 offsets (one per mip level, from the start of the data, padded to 16 bytes), then the blocks.
#pragma once
#include "../core/wr_core.h"

struct pctPIXEL {                      // 4 bytes, byte 0 = alpha
    u8 a, r, g, b;
};
struct pctHEADER {                     // 0x14
    u32 magic;                         // +0x00 'PICT' (0x50494354)
    int w, h;                          // +0x04
    int nMip;                          // +0x0C
    int fmt;                           // +0x10 pctFORMAT_ID
};
struct pctEXTRA_DATA {                 // 8 bytes, chunks 0xFA / 0xFB; txmTEXTURE::SetTexParams maps the flags
    u32 flags;                         // +0x00 1/2/4/8/0x10/0x20/0x40 -> texture flags
    u32 value;                         // +0x04 -> txmTEXTURE::extra
};
struct pctPICTURE : pctHEADER {        // 0x20, apCalloc'ed (pctCreate / pctRead)
    pctEXTRA_DATA extra;               // +0x14
    u8 *data;                          // +0x1C every mip level, apCalloc'ed
};
// One entry of the format table (v12 0x00978770, 18 entries, filled by pctInit)
struct pctFORMAT {                     // 0x70
    int  nComp;                        // +0x00
    u32  flags;                        // +0x04 PCT_FMT_*
    int  bpp;                          // +0x08 bytes per pixel (0 = block compressed)
    u32  mask[4];                      // +0x0C A, R, G, B (I = mask[1]; UV = mask[0], mask[1])
    int  shift[4];                     // +0x1C
    u8   bits[4];                      // +0x2C
    char name[0x40];                   // +0x30
};
enum pctFORMAT_FLAGS {
    PCT_FMT_ALPHA      = 0x01,
    PCT_FMT_INTENSITY  = 0x02,
    PCT_FMT_UV         = 0x04,
    PCT_FMT_DXT        = 0x08,
    PCT_FMT_PREMUL     = 0x10,         // XT2 / XT4 // ?
};
enum pctFORMAT_ID {
    PCT_ARGB8888 = 0, PCT_ARGB4444, PCT_RGB888, PCT_A8, PCT_RGB332, PCT_RGB565, PCT_ARGB8332, PCT_ARGB1555,
    PCT_I8, PCT_AI44, PCT_AI88, PCT_UV88,
    PCT_DXT1 = 12,                     // "OXT1" opaque
    PCT_DXT1A,                         // "AXT1" 1-bit alpha
    PCT_DXT2, PCT_DXT3, PCT_DXT4, PCT_DXT5,
    PCT_NFORMAT
};
#define PCT_MAGIC 0x50494354           // 'PICT'

extern pctFORMAT pctFormat[PCT_NFORMAT];   // v12 0x00978770 (feb 0x00934530)
extern int       pctInited;                // v12 0x00978F50 (feb 0x00934D10) // ? name

int  __fastcall pctInit();                                                   // v12 0x004F7FC0 (feb 0x004E5AB0)
int  __fastcall pctReadHdr(char *name, pctHEADER *h, pctEXTRA_DATA *e);      // v12 0x004F8660 (feb 0x004E6150)
pctPICTURE *__fastcall pctRead(char *name);                                  // v12 0x004F86A0 (feb 0x004E6190)
pctPICTURE *__fastcall pctRead(class fioFILE *f);                            // v12 0x004F86F0 (feb 0x004E61E0)
int  __fastcall pctRead(char *name, pctHEADER *h, pctEXTRA_DATA *e, void **data);       // v12 0x004F8740 (feb 0x004E6230)
int  __fastcall pctRead(class fioFILE *f, pctHEADER *h, pctEXTRA_DATA *e, void **data); // v12 0x004F8810 (feb 0x004E6300)
int  __fastcall pctWrite(char *name, pctPICTURE *p);                         // v12 0x004F8B40 (feb 0x004E6630)
int  __fastcall pctWrite(class fioFILE *f, pctPICTURE *p);                   // v12 0x004F8BC0 (feb 0x004E66B0)
pctPICTURE *__fastcall pctCreate(pctHEADER *h);                              // v12 0x004F8E80 (feb 0x004E6970)
void __fastcall pctDestroy(pctPICTURE *p);                                   // v12 0x004F8FD0 (feb 0x004E6AC0)
int  __fastcall pctIsAlpha(int fmt);                                         // v12 0x004F8FF0 (feb 0x004E6AE0)
pctPICTURE *__fastcall pctConvert(pctPICTURE *p, int fmt);                   // v12 0x004F9030 (feb 0x004E6B20)
void __fastcall pctSetPixel(pctPICTURE *p, int x, int y, int mip, pctPIXEL *c); // v12 0x004F9620 (feb 0x004E7110)
void __fastcall pctGetPixel(pctPICTURE *p, int x, int y, int mip, pctPIXEL *c); // v12 0x004F9870 (feb 0x004E7360)
int  __fastcall pctGetBPP(pctHEADER *h);                                     // v12 0x004F9B10 (feb 0x004E7600)
pctFORMAT *__fastcall pctGetFormat(pctPICTURE *p);                           // v12 0x004F9B30 (feb 0x004E7620)
pctFORMAT *__fastcall pctGetFormat(pctHEADER *h);                            // v12 0x004F9B50 (feb 0x004E7640)
pctFORMAT *__fastcall pctGetFormat(int fmt);                                 // v12 0x004F9B70 (feb 0x004E7660)
int  __fastcall pctGetMipMapMemSize(pctHEADER *h, int mip);                  // v12 0x004F9B90 (feb 0x004E7680)
int  __fastcall pctGetMemSize(pctHEADER *h, int nMip);                       // v12 0x004F9C50 (feb 0x004E7740) -1 = all levels
void __fastcall pctCopyViaUnpackDXTBuf(pctPICTURE *p, int w, int h, int dstBpp, int mip, int rowPad,
                                       u32 rMask, u32 gMask, u32 bMask, u32 aMask, u8 *dst); // v12 0x004F9D60 (feb 0x004E7850)
void __fastcall pctCopyDXTBuf(pctPICTURE *p, int mip, u8 *dst);              // v12 0x004FA270 (feb 0x004E7D60)
void __fastcall pctCopyBuf(pctPICTURE *p, int w, int h, int dstBpp, int mip, int rowPad,
                           u32 rMask, u32 gMask, u32 bMask, u32 aMask, u8 *dst);   // v12 0x004FA350 (feb 0x004E7E40)
void __fastcall pctGetFromBuf(int w, int h, int srcBpp, int rowPad, u32 rMask, u32 gMask, u32 bMask, u32 aMask,
                              u8 *src, pctPICTURE *p);                       // v12 0x004FA980 (feb 0x004E8470)
int  __fastcall pctGenMipMaps(pctPICTURE *p, int nMip, float gamma);         // v12 0x004FAEF0 (feb 0x004E89E0)
pctPICTURE *__fastcall pctGenMipMapLevel(pctPICTURE *p, int mip, float gamma); // v12 0x004FB2B0 (feb 0x004E8DA0)
int  __fastcall pctGetMaxMipMapCount(int w, int h);                          // v12 0x004FB650 (feb 0x004E9140)
pctPICTURE *__fastcall pctResample(pctPICTURE *p, int w, int h, int mode);   // v12 0x004FB6A0 (feb 0x004E9190)
void __fastcall pctCalcAveragePixel(pctPICTURE *p, struct m2dV *uv, pctPIXEL *out); // v12 0x004FB900 (feb 0x004E93F0)

// pic_decompress.obj: DXT block decoding (one pixel at a time, with a one-block cache)
void __fastcall pctGetDXTPixel(pctPICTURE *p, int x, int y, int mip, pctPIXEL *c); // v12 0x00505DD0 (feb 0x004F37C0)
u16  __fastcall pctLineCombColor565(u16 c0, u32 w0, u16 c1, u32 w1, u32 div);   // v12 0x00506090 (feb 0x004F3A80)
void __fastcall UpdateCache1(u8 *block);                                     // v12 0x00506120 (feb 0x004F3B10)
void __fastcall pctGetPairByCode(u8 code, u16 *color, u8 *alpha);            // v12 0x00506250 (feb 0x004F3C40)
void __fastcall UpdateCache5(u8 *block);                                     // v12 0x00506300 (feb 0x004F3CF0)
void __fastcall pctDecode(u16 color, u8 alpha, pctPIXEL *c);                 // v12 0x00506460 (feb 0x004F3E50)
