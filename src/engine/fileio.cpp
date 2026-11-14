// fileio.cpp -- engine.lib file IO: fioFILE (position/size bookkeeping and the chunk format), fioFILE_DISK
// (C runtime FILE*) and fioFILE_MEM (whole file in memory / growable write buffer)
// (Will Rock 1.2, v12.exe; feb obj engine:fileio.obj)
//
// v12 0x004E7690..0x004E85FF, every function "exact" or the same code (callgraph/vtable: only call targets
// moved). fioIsReadOnly has no 1.2 copy (unused, not linked). The chunk format: a 6-byte header
// { u16 id, u32 end } where "end" is the absolute file position after the chunk body (SkipChunk seeks to it),
// back-patched by Term() for files opened WRITE|CHUNKS. Chunk id 1 is the end marker of a chunk list.
// Kept as in the original: Init with neither READ nor WRITE opens with an undefined mode string (here ""),
// fioFILE_MEM::Init(name) leaks the FILE* when the read fails, fioFILE_MEM::Term in write mode does not free
// the buffer.

#include "engine.h"
#include <stdio.h>
#include <sys/stat.h>
#include <io.h>

// v12 0x004E7690  (feb 0x004D7A00)
BOOL32 __fastcall fioIsExist(char *name)
{
    return access(name, 0) == 0;
}

// (feb 0x004D7A20) not in 1.2 (no caller)
BOOL32 __fastcall fioIsReadOnly(char *name)
{
    return access(name, 2) != 0;
}

// v12 0x00492840  (feb 0x00487630)
// The deleting destructor shared by fioFILE and fioFILE_DISK (fioFILE has no destructor body).
void *fioFILE::__vecDelDtor(unsigned flags)
{
    if (flags & 1)
        operator delete(this);
    return this;
}

// The common part of every Init (inlined in each): name, position, flags, empty chunk table, size by stat().
static BOOL32 fioInitBase(fioFILE *f, char *name, int flags)
{
    strcpy(f->name, name);
    f->pos = 0;
    f->flags = flags;
    f->nChunk = 0;
    f->capChunk = 0;
    f->chunkTbl = 0;
    if (flags & FIO_READ) {
        struct stat st;
        if (stat(name, &st) != 0)
            return 0;
        f->size = st.st_size;
        return 1;
    }
    f->size = 0;
    return 1;
}

// The common part of every Term: back-patch the "end" of every chunk written (WRITE|CHUNKS), free the table.
static void fioTermChunks(fioFILE *f)
{
    if ((f->flags & FIO_WRITE) && (f->flags & FIO_CHUNKS)) {
        for (int i = 0; i < f->nChunk; i++) {
            f->Seek(f->chunkTbl[i * 2] + 2, FIO_SEEK_SET);
            u32 end = f->chunkTbl[i * 2 + 1];
            f->WriteData(&end, 4, 4);
        }
    }
    apFree(f->chunkTbl);
    f->chunkTbl = 0;
}

// Byte-swap 'bytes / elemSize' elements in place (FIO_SWAP).
static void fioSwap(void *dst, int bytes, int elemSize)
{
    if (elemSize < 2 || bytes / elemSize < 1)
        return;
    unsigned char *p = (unsigned char *)dst;
    for (int n = bytes / elemSize; n; n--, p += elemSize)
        for (int i = 0, j = elemSize - 1; i < elemSize / 2; i++, j--) {
            unsigned char t = p[i];
            p[i] = p[j];
            p[j] = t;
        }
}

// v12 0x004E76D0  (feb 0x004D7A40)
BOOL32 fioFILE::Init(char *name, int flags)
{
    return fioInitBase(this, name, flags);
}

// v12 0x004E7760  (feb 0x004D7AD0)
BOOL32 fioFILE::Term()
{
    fioTermChunks(this);
    return !(flags & FIO_ERROR);
}

// v12 0x004E77F0  (feb 0x004D7B60)
// Bookkeeping only (the base class has no storage); FIO_SEEK_END is ignored.
void fioFILE::Seek(int ofs, fioSEEK how)
{
    if (how == FIO_SEEK_CUR)
        pos += ofs;
    else if (how == FIO_SEEK_SET)
        pos = ofs;
}

// v12 0x004E7820  (feb 0x004D7B90)
BOOL32 fioFILE::IsEOF()
{
    return pos >= size;
}

// v12 0x004E7840  (feb 0x004D7BB0)
// Start a chunk: header {id, end placeholder} at the current position, recorded in the chunk table
// (grown by 100 entries).
void fioFILE::WriteChunk(u16 id, fioCHUNK *c)
{
    c->id = id;
    c->start = pos;
    if (capChunk <= nChunk) {
        u32 *t = (u32 *)apRealloc(chunkTbl, capChunk * 8 + 800);
        chunkTbl = t;
        if (!t) {
            flags |= FIO_ERROR;
            return;
        }
        capChunk += 100;
    }
    chunkTbl[nChunk * 2] = c->start;
    c->index = nChunk;
    nChunk++;
    WriteData(c, 6, 1);
}

// v12 0x004E78E0  (feb 0x004D7C50)
// End a chunk: its end is the current position.
void fioFILE::UpdateChunk(fioCHUNK *c)
{
    chunkTbl[c->index * 2 + 1] = pos;
    c->start = 0xFFFFFFFF;
    c->index = 0xFFFFFFFF;
}

// v12 0x004E7910  (feb 0x004D7C80)
// Read a chunk header; returns its id (0xFFFF when nothing could be read). c may be 0.
int fioFILE::ReadChunk(fioCHUNK *c)
{
    fioCHUNK tmp;
    tmp.id = 0;
    tmp.size = 0xFFFFFFFF;
    tmp.start = 0xFFFFFFFF;
    tmp.index = 0xFFFFFFFF;
    if (!c)
        c = &tmp;
    c->id = 0xFFFF;
    c->start = pos;
    ReadData(c, 6, 1);
    return c->id;
}

// v12 0x004E7960  (feb 0x004D7CD0)
void fioFILE::SkipChunk(fioCHUNK *c)
{
    Seek(c->size, FIO_SEEK_SET);
}

// v12 0x004E7980  (feb 0x004D7CF0)
// Back to the chunk's header (c == 0: the header just read, 6 bytes back).
void fioFILE::RevertChunk(fioCHUNK *c)
{
    if (!c) {
        Seek(-6, FIO_SEEK_CUR);
        return;
    }
    Seek(c->start, FIO_SEEK_SET);
}

// v12 0x004E79B0  (feb 0x004D7D20)
void fioFILE::WriteLastChunk()
{
    fioCHUNK c;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    c.id = 0;
    WriteChunk(1, &c);
    chunkTbl[c.index * 2 + 1] = pos;
}

// v12 0x004E7A00  (feb 0x004D7D70)
// Expect the end marker (id 1); otherwise step back over the header read and return 0.
BOOL32 fioFILE::ReadLastChunk()
{
    fioCHUNK c;
    c.size = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    c.start = pos;
    c.id = 0xFFFF;
    ReadData(&c, 6, 1);
    if (c.id != 1) {
        Seek(-6, FIO_SEEK_CUR);
        return 0;
    }
    return 1;
}

// v12 0x004E7A60  (feb 0x004D7DD0)
// No storage: the swap runs over the (unfilled) destination, the position advances.
void fioFILE::ReadData(void *dst, int bytes, int elemSize)
{
    if (flags & FIO_SWAP)
        fioSwap(dst, bytes, elemSize);
    pos += bytes;
}

// v12 0x004E7B30  (feb 0x004D7EA0)
// A NUL-terminated string of at most max-1 characters; the rest of a longer one is read and dropped.
void fioFILE::Read(char *str, int max)
{
    char c = 0;
    int i = 0;
    for (; i < max - 1; i++) {
        ReadData(&c, 1, 1);
        str[i] = c;
        if (!c)
            return;
    }
    str[max - 1] = 0;
    while (c && !IsEOF())
        ReadData(&c, 1, 1);
}

// v12 0x004E7BB0  (feb 0x004D7F20)
void fioFILE::WriteData(void *src, int bytes, int elemSize)
{
    pos += bytes;
    if (size < pos)
        size = pos;
}

// v12 0x004E7BE0  (feb 0x004D7F50)
// The string with its NUL.
void fioFILE::Write(char *str)
{
    WriteData(str, (int)strlen(str) + 1, 1);
}

// v12 0x004E7C10  (feb 0x004D7F80)
// type, number of key points, then the key points.
m3dSPL *fioFILE::ReadSpline()
{
    int type, nKp;
    ReadData(&type, 4, 4);
    ReadData(&nKp, 4, 4);
    m3dSPL *s = m3dSPL::Make(type, nKp);
    if (!s) {
        flags |= FIO_ERROR;
        return 0;
    }
    ReadData(s->kp, s->kpValueSize * s->nKp, 1);
    return s;
}

// v12 0x004E7C80  (feb 0x004D7FF0)
// type (must be the volume's), then its data: box min/max, prism outline/yMin/yMax, sphere centre/radius,
// OBB corner and three edges.
BOOL32 fioFILE::ReadVol(m3dVOL *vol)
{
    int type;
    ReadData(&type, 4, 4);
    if (type != vol->type)
        return 0;
    switch (type) {
    case M3D_VOL_BOX: {
        m3dBOX *b = (m3dBOX *)vol;
        ReadData(&b->vMin, 12, 4);
        ReadData(&b->vMax, 12, 4);
        return 1;
    }
    case M3D_VOL_PRISM: {
        m3dPRISM *p = (m3dPRISM *)vol;
        ReadData(&p->base.nVert, 4, 4);
        ReadData(p->base.vert, p->base.nVert * 8, 4);
        ReadData(&p->yMin, 4, 4);
        ReadData(&p->yMax, 4, 4);
        return 1;
    }
    case M3D_VOL_SPHERE: {
        m3dSPHERE *s = (m3dSPHERE *)vol;
        ReadData(&s->center, 12, 4);
        ReadData(&s->radius, 4, 4);
        return 1;
    }
    case M3D_VOL_OBB: {
        m3dOBB *o = (m3dOBB *)vol;
        ReadData(&o->org, 12, 4);
        ReadData(&o->ax[0], 12, 4);
        ReadData(&o->ax[1], 12, 4);
        ReadData(&o->ax[2], 12, 4);
        return 1;
    }
    }
    return 1;                                           // unknown type equal to vol->type: nothing read
}

// v12 0x004E7DD0  (feb 0x004D8140)
m3dVOL *fioFILE::ReadVol()
{
    int type;
    ReadData(&type, 4, 4);
    m3dVOL *v = m3dVOL::Make(type);
    if (!v)
        return 0;
    Seek(-4, FIO_SEEK_CUR);
    ReadVol(v);
    return v;
}

// ---------------------------------------------------------------------------------------------
// fioFILE_DISK
// ---------------------------------------------------------------------------------------------

// v12 0x004E7E20  (feb 0x004D8190)
BOOL32 fioFILE_DISK::Init(char *name, int fl)
{
    if (!fioInitBase(this, name, fl))
        return 0;
    char mode[16];
    mode[0] = 0;                                        // original: uninitialised when neither READ nor WRITE
    if ((flags & FIO_READ) && (flags & FIO_WRITE))
        strcpy(mode, "rb+");
    else if (flags & FIO_READ)
        strcpy(mode, "rb");
    else if (flags & FIO_WRITE)
        strcpy(mode, "wb");
    if (!strlen(name))
        return 0;
    fp = fopen(name, mode);
    return fp != 0;
}

// v12 0x004E7F20  (feb 0x004D8290)
BOOL32 fioFILE_DISK::Term()
{
    fioTermChunks(this);
    u32 fl = flags;
    fclose((FILE *)fp);
    fp = 0;
    return !(fl & FIO_ERROR);
}

// v12 0x004E7FC0  (feb 0x004D8330)
void fioFILE_DISK::Seek(int ofs, fioSEEK how)
{
    int whence;
    if (how == FIO_SEEK_CUR)
        pos += ofs;
    else if (how == FIO_SEEK_SET)
        pos = ofs;
    if (how == FIO_SEEK_CUR)
        whence = SEEK_CUR;
    else if (how == FIO_SEEK_SET)
        whence = SEEK_SET;
    else if (how == FIO_SEEK_END)
        whence = SEEK_END;
    else
        return;
    if (fseek((FILE *)fp, ofs, whence))
        flags |= FIO_ERROR;
}

// v12 0x004E8030  (feb 0x004D83A0)
void fioFILE_DISK::ReadData(void *dst, int bytes, int elemSize)
{
    if (!bytes)
        return;
    if (fread(dst, bytes, 1, (FILE *)fp) != 1) {
        flags |= FIO_ERROR;
        return;
    }
    if (flags & FIO_SWAP)
        fioSwap(dst, bytes, elemSize);
    pos += bytes;
}

// v12 0x004E80E0  (feb 0x004D8450)
void fioFILE_DISK::WriteData(void *src, int bytes, int elemSize)
{
    if (!bytes)
        return;
    if (fwrite(src, bytes, 1, (FILE *)fp) != 1) {
        flags |= FIO_ERROR;
        return;
    }
    pos += bytes;
    if (size < pos)
        size = pos;
}

// v12 0x004E8140  (feb 0x004D84B0)
void fioFILE_DISK::DeleteFile(const char *name)
{
    remove(name);
}

// ---------------------------------------------------------------------------------------------
// fioFILE_MEM
// ---------------------------------------------------------------------------------------------

// v12 0x004E8150  (feb 0x004D84C0)
// READ: the whole file into an apMalloc buffer. WRITE: a 100001-byte buffer, written out by Term.
BOOL32 fioFILE_MEM::Init(char *name, int fl)
{
    if ((flags & FIO_READ) && (flags & FIO_WRITE))     // (the flags of the previous use)
        return 0;
    if (!strlen(name))
        return 0;
    buf = 0;
    cur = 0;
    capacity = 0;
    if (!fioInitBase(this, name, fl))
        return 0;
    if (flags & FIO_READ) {
        FILE *f = fopen(name, "rb");
        if (!f)
            return 0;
        buf = (u8 *)apMalloc(size);
        if (!buf)
            return 0;
        if (fread(buf, size, 1, f) != 1)
            return 0;
        fclose(f);
    } else if ((flags & FIO_WRITE) && !capacity) {
        if (flags & FIO_EXT_BUF)
            return 0;
        u8 *b = (u8 *)apRealloc(buf, 0x186A1);
        if (!b) {
            flags |= FIO_ERROR;
            return 0;
        }
        buf = b;
        capacity = 0x186A1;
        cur = b + pos;
    }
    cur = buf;
    return 1;
}

// v12 0x004E82F0  (feb 0x004D8660)
// Over a caller's buffer (never freed or grown): READ = its 'size' bytes, WRITE = up to 'size' bytes.
BOOL32 fioFILE_MEM::Init(void *b, int sz, int fl)
{
    flags = fl | FIO_EXT_BUF;
    pos = 0;
    nChunk = 0;
    capChunk = 0;
    chunkTbl = 0;
    buf = (u8 *)b;
    capacity = sz;
    if (flags & FIO_WRITE)
        size = 0;
    else if (flags & FIO_READ)
        size = sz;
    else
        return 0;
    cur = (u8 *)b;
    pos = 0;
    return 1;
}

// v12 0x004E8370  (feb 0x004D86E0)
BOOL32 fioFILE_MEM::Term()
{
    fioTermChunks(this);
    u32 fl = flags;
    if (!(fl & FIO_EXT_BUF)) {
        if (fl & FIO_READ)
            apFree(buf);
        else if (fl & FIO_WRITE) {
            FILE *f = fopen(name, "wb");
            if (!f)
                return 0;
            if (fwrite(buf, size, 1, f) != 1)
                return 0;
            fclose(f);
        }
    }
    buf = 0;
    cur = 0;
    capacity = 0;
    return !(fl & FIO_ERROR);
}

// v12 0x004E8480  (feb 0x004D87F0)
void fioFILE_MEM::Seek(int ofs, fioSEEK how)
{
    if (how == FIO_SEEK_CUR)
        pos += ofs;
    else if (how == FIO_SEEK_SET)
        pos = ofs;
    cur = buf + pos;
}

// v12 0x004E84C0  (feb 0x004D8830)
// No bounds check.
void fioFILE_MEM::ReadData(void *dst, int bytes, int elemSize)
{
    if (!bytes)
        return;
    memcpy(dst, cur, bytes);
    cur += bytes;
    if (flags & FIO_SWAP)
        fioSwap(dst, bytes, elemSize);
    pos += bytes;
}

// v12 0x004E8570  (feb 0x004D88E0)
// Grows the buffer to pos + bytes + 100000 when needed (not a caller's buffer: then the write is dropped).
void fioFILE_MEM::WriteData(void *src, int bytes, int elemSize)
{
    if (!bytes)
        return;
    if ((flags & FIO_WRITE) && (u32)capacity < pos + bytes) {
        if (flags & FIO_EXT_BUF)
            return;
        u32 n = pos + bytes + 100000;
        u8 *b = (u8 *)apRealloc(buf, n);
        if (!b) {
            flags |= FIO_ERROR;
            return;
        }
        buf = b;
        capacity = n;
        cur = b + pos;
    }
    memcpy(cur, src, bytes);
    cur += bytes;
    pos += bytes;
    if (size < pos)
        size = pos;
}

// v12 0x00531B00  (feb 0x0051C1E0)
u8 *fioFILE_MEM::GetBuf()
{
    return buf;
}

// v12 0x00531B10  (feb 0x0051C1F0)
u8 *fioFILE_MEM::GetCurPtr()
{
    return cur;
}
