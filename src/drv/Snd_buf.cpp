// Snd_buf.cpp -- the sample buffers: _sndBUFFER (one DirectSound 8 buffer, 2D or 3D) and _sndSTR_BUFFER (a
// 0x14820-byte buffer refilled half by half from a memory-mapped .wav) (Will Rock 1.2, v12.exe; feb obj
// drv:Snd_buf.obj)
//
// Rebuilt from the 1.2 code (v12 0x004B9430..0x004BAC0B, vtables 0x006068D0 / 0x006069B0). 1.2 rewrote this file for
// DirectSound 8: the buffers are IDirectSoundBuffer8 (QueryInterface after CreateSoundBuffer / DuplicateSoundBuffer),
// a 3D buffer only exists when the system has a 3D listener, and it uses DS3DALG_NO_VIRTUALIZATION. Volumes are dB
// (x100 -> DirectSound hundredths, clamped to -10000..0) plus the sample cache's master volume. Every call is a no-op
// while the sound system is not initialised (_sndSystem->flags & 1). The "now" argument of the 3D setters is
// DS3D_IMMEDIATE when set, DS3D_DEFERRED (committed with the listener) when 0.

#define DIRECTSOUND_VERSION 0x0800
#include <windows.h>
#include <unknwn.h>
#include <mmsystem.h>
#include <dsound.h>
#include <math.h>
#include <string.h>
#include "../../include/wr_sdk.h"

// the GUIDs as the exe has them in .rdata
static const GUID sndGUID_NULL = { 0, 0, 0, { 0, 0, 0, 0, 0, 0, 0, 0 } };                      // v12 0x0060A308
static const GUID sndDS3DALG_NO_VIRTUALIZATION =                                              // v12 0x0060A388
    { 0xC241333F, 0x1C1B, 0x11D2, { 0x94, 0xF5, 0x00, 0xC0, 0x4F, 0xC2, 0x8A, 0xCA } };
static const GUID sndIID_IDirectSoundBuffer8 =                                                // v12 0x0060A3B8
    { 0x6825A449, 0x7524, 0x4D82, { 0x92, 0x0F, 0x50, 0xE3, 0x6A, 0xB3, 0xAB, 0x1E } };
static const GUID sndIID_IDirectSound3DBuffer =                                               // v12 0x0060A398
    { 0x279AFA86, 0x4981, 0x11CE, { 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60 } };

static const float sndEps = 1.0e-6f;    // v12 0x006068C8
float sndEpsSq;                         // v12 0x00656248 (static init v12 0x004B9C10)
static struct sndEPS_INIT { sndEPS_INIT() { sndEpsSq = sndEps * sndEps; } } sndEpsInit;
int   sndNmbPlay;                       // v12 0x0065624C DirectSound Play calls that succeeded

#define SYS        _sndSystem
#define SYS_ON()   (SYS->flags & 1)
#define DSB(b)     ((IDirectSoundBuffer8 *)(b)->pDSB)
#define DS3DB(b)   ((IDirectSound3DBuffer *)(b)->pDS3DB)

static inline int sndFtol(double v) { return (int)v; }  // MSVC __ftol: truncation

// ---------------------------------------------------------------------------------------------------- base

// v12 0x004B9430
void *sndBUFFER::__vecDelDtor(unsigned f)
{
    if (f & 1)
        operator delete(this);
    return this;
}

// ---------------------------------------------------------------------------------------------------- _sndBUFFER

_sndBUFFER::_sndBUFFER()
{
    flags = 0;
    memset(&fmt, 0, sizeof(fmt));
    size = 0;
    pDSB = 0;
    playMs = 0;
    pDS3DB = 0;
    volumeDb = 0.0f;
    minDist = 0.1f;
    maxDist = 1.0e9f;
    flags |= SNDBF_3D;
    pos = m3dVZero;
    vel = m3dVZero;
}

// v12 0x004B9450
// The .wav, mapped read-only: its format makes the buffer, its data is written in.
int _sndBUFFER::Init(char *file)
{
    HANDLE h = CreateFileA(file, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    HANDLE m = CreateFileMappingA(h, 0, PAGE_READONLY, 0, 0, 0);
    if (m == INVALID_HANDLE_VALUE) {    // (original: CreateFileMapping returns 0 on failure)
        CloseHandle(h);
        return 0;
    }
    CloseHandle(h);
    void *v = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(m);
    if (!v)
        return 0;
    sndFORMAT f;
    uint8_t *data;
    if (sndParseWaveResource(v, &f, &data, (unsigned long *)&size) && Init(&f, size) && Write(data, size, 0)) {
        UnmapViewOfFile(v);
        return 1;
    }
    UnmapViewOfFile(v);
    return 0;
}

// v12 0x004B9540
int _sndBUFFER::Init(sndFORMAT *f, int sz)
{
    sndWAVEFMT w;
    w.tag = 1;                          // WAVE_FORMAT_PCM
    w.channels = f->channels;
    w.rate = f->rate;
    w.bytesPerSec = f->bytesPerSec;
    w.align = f->align;
    w.bits = f->bits;
    w.cbSize = 0;
    if (!CreateObj(&w, sz, 1))
        return 0;
    return ApplyParams() != 0;
}

// v12 0x004B95D0
// The DirectSound buffer: 3D (CTRL3D | MUTE3DATMAXDISTANCE, no virtualisation) when the system has a listener,
// else 2D; always CTRLVOLUME | LOCDEFER. Then the device caps are read again (the free hardware buffers).
int _sndBUFFER::CreateObj(sndWAVEFMT *wf, int sz, int)
{
    if (!SYS_ON())
        return 0;
    if (flags & SNDBF_CREATED)
        return 0;
    int is3D = SYS->pListener != 0;
    fmt = *wf;
    size = sz;
    DSBUFFERDESC d;
    memset(&d, 0, sizeof(d));
    d.dwSize = sizeof(DSBUFFERDESC);    // 0x24
    d.dwBufferBytes = sz;
    d.lpwfxFormat = (WAVEFORMATEX *)&fmt;
    d.guid3DAlgorithm = sndGUID_NULL;
    DWORD f = DSBCAPS_CTRLVOLUME;
    if (is3D) {
        d.guid3DAlgorithm = sndDS3DALG_NO_VIRTUALIZATION;
        f = DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME | DSBCAPS_MUTE3DATMAXDISTANCE;
    }
    d.dwFlags = f | DSBCAPS_LOCDEFER;
    IDirectSoundBuffer *tmp;
    if (FAILED(((IDirectSound8 *)SYS->pDS)->CreateSoundBuffer(&d, &tmp, 0)))
        return 0;
    if (tmp->QueryInterface(sndIID_IDirectSoundBuffer8, &pDSB))
        return 0;
    if (tmp)
        tmp->Release();
    if (is3D && DSB(this)->QueryInterface(sndIID_IDirectSound3DBuffer, &pDS3DB))
        return 0;
    flags |= SNDBF_LOADED;
    flags |= SNDBF_CREATED;
    SYS->nBuffers++;
    DSBCAPS c;
    memset(&c, 0, sizeof(c));
    c.dwSize = sizeof(DSBCAPS);         // 0x14
    DSB(this)->GetCaps(&c);
    memset(SYS->dsCaps, 0, sizeof(SYS->dsCaps));
    *(DWORD *)SYS->dsCaps = 0x60;
    ((IDirectSound8 *)SYS->pDS)->GetCaps((DSCAPS *)SYS->dsCaps);
    return 1;
}

// v12 0x004B97D0
int _sndBUFFER::IsSoftware()
{
    DSBCAPS c;
    memset(&c, 0, sizeof(c));
    c.dwSize = sizeof(DSBCAPS);
    DSB(this)->GetCaps(&c);
    if (c.dwFlags & DSBCAPS_LOCHARDWARE)
        return 0;
    return 1;
}

// v12 0x004B9820
void _sndBUFFER::Term()
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return;
    if (pDSB)
        SYS->nBuffers--;
    if (pDS3DB)
        DS3DB(this)->Release();
    pDS3DB = 0;
    if (pDSB)
        DSB(this)->Release();
    pDSB = 0;
    flags &= ~SNDBF_CREATED;
    flags &= ~(SNDBF_PLAYING | SNDBF_LOOP | SNDBF_PAUSED);
    flags &= ~(SNDBF_LOADED | SNDBF_DUP);
}

// v12 0x004B9890
// A second buffer on the same sample memory (DuplicateSoundBuffer).
int _sndBUFFER::Duplicate(sndBUFFER *src)
{
    if (!SYS_ON() || !(src->flags & SNDBF_CREATED) || src == this)
        return 0;
    Term();
    fmt = src->fmt;
    size = src->size;
    IDirectSoundBuffer *tmp;
    if (((IDirectSound8 *)SYS->pDS)->DuplicateSoundBuffer((IDirectSoundBuffer *)src->pDSB, &tmp))
        return 0;
    if (tmp->QueryInterface(sndIID_IDirectSoundBuffer8, &pDSB))
        return 0;
    if (tmp)
        tmp->Release();
    if (src->pDS3DB && DSB(this)->QueryInterface(sndIID_IDirectSound3DBuffer, &pDS3DB))
        return 0;
    flags |= SNDBF_DUP;
    flags |= SNDBF_CREATED;
    if (!ApplyParams())
        return 0;
    SYS->nBuffers++;
    return 1;
}

// v12 0x004B99A0
// Takes src's DirectSound buffer (and its state bits); src is left empty.
int _sndBUFFER::TakeOver(sndBUFFER *src)
{
    if (!SYS_ON() || !(src->flags & SNDBF_CREATED) || src == this)
        return 0;
    Term();
    pDSB = src->pDSB;
    pDS3DB = src->pDS3DB;
    fmt = src->fmt;
    size = src->size;
    static const uint32_t keep[] = { SNDBF_LOADED, SNDBF_DUP, SNDBF_PLAYING, SNDBF_LOOP, SNDBF_PAUSED };
    for (int i = 0; i < 5; i++) {
        if (src->flags & keep[i])
            flags |= keep[i];
        else
            flags &= ~keep[i];
    }
    flags |= SNDBF_CREATED;
    src->pDSB = 0;
    src->pDS3DB = 0;
    src->Term();
    return ApplyParams() != 0;
}

// v12 0x004B9AC0
// 0 when the buffer is lost (it is restored for next time) or the system is off.
int _sndBUFFER::CheckLost()
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return 0;
    DWORD st;
    if (DSB(this)->GetStatus(&st))
        return 0;
    if (st & DSBSTATUS_BUFFERLOST) {
        DSB(this)->Restore();
        return 0;
    }
    return 1;
}

// v12 0x004031F0 (a shared "return 1")
int _sndBUFFER::Update()
{
    return 1;
}

// v12 0x004B9B10
// n bytes (at most the buffer size) at byte ofs of the buffer, wrapping (Lock gives two parts).
int _sndBUFFER::Write(void *data, unsigned n, unsigned ofs)
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return 0;
    if (n > size)
        n = size;
    void *p1, *p2;
    DWORD n1, n2;
    HRESULT hr = DSB(this)->Lock(ofs, n, &p1, &n1, &p2, &n2, 0);
    if (hr == DSERR_BUFFERLOST) {
        DSB(this)->Restore();
        hr = DSB(this)->Lock(ofs, n, &p1, &n1, &p2, &n2, 0);
    }
    if (hr)
        return 0;
    memcpy(p1, data, n1);
    if (p2)
        memcpy(p2, (char *)data + n1, n2);
    return DSB(this)->Unlock(p1, n1, p2, n2) == 0;
}

// v12 0x004B9C30
int _sndBUFFER::SetVolume(float dB)
{
    if (!SYS_ON())
        return 0;
    if (fabsf(dB - volumeDb) < sndEps)
        return 0;
    volumeDb = dB;
    return ApplyVolume();
}

// v12 0x004B9CA0
int _sndBUFFER::ApplyVolume()
{
    if (!pDSB)
        return 0;
    int v = sndFtol((snmManager->masterVolume + volumeDb) * 100.0f);
    if (v >= 0)
        v = 0;
    if (v <= DSBVOLUME_MIN)
        v = DSBVOLUME_MIN;
    return DSB(this)->SetVolume(v) == 0;
}

// v12 0x004B9CF0
int _sndBUFFER::SetPan(float)
{
    return 0;
}

// v12 0x004B9D00
float _sndBUFFER::GetPan()
{
    return 0.0f;
}

// v12 0x004B9D10
// The start position; with apply, the play cursor is moved there too (rounded down to a whole sample frame).
void _sndBUFFER::SetCurPlayTime(unsigned ms, int apply)
{
    if (!SYS_ON())
        return;
    playMs = ms;
    if (!(flags & SNDBF_CREATED) || !apply)
        return;
    int b = sndFtol((double)(long long)fmt.bytesPerSec * (int)ms * 0.001f);
    int a = fmt.align;
    DSB(this)->SetCurrentPosition(b / a * a);
}

// v12 0x004B9D80
unsigned _sndBUFFER::GetCurPlayTime(int fromBuffer)
{
    if (!SYS_ON())
        return 0;
    if (!fromBuffer || !(flags & SNDBF_CREATED))
        return playMs;
    DWORD play;
    if (DSB(this)->GetCurrentPosition(&play, 0))
        return 0;
    return sndFtol((int)play * 1000.0f / (double)(int)fmt.bytesPerSec);
}

// v12 0x004B9E10
unsigned _sndBUFFER::GetTotalTime()
{
    return sndFtol((double)(long long)size / (int)fmt.bytesPerSec * 1000.0f);
}

// v12 0x004B9E40
// flags: 1 loop, 2 from the start, 4 from playMs.
void _sndBUFFER::Play(int f)
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return;
    flags &= ~SNDBF_PAUSED;
    int loop = f & 1;
    if (loop)
        flags |= SNDBF_LOOP;
    else
        flags &= ~SNDBF_LOOP;
    flags |= SNDBF_PLAYING;
    if (f & 2)
        SetCurPlayTime(0, 1);
    if (f & 4)
        SetCurPlayTime(playMs, 1);
    if (IsPlay())
        return;
    if (DSB(this)->Play(0, 0, loop ? DSBPLAY_LOOPING : 0) == 0)
        sndNmbPlay++;
}

// v12 0x004B9EF0
int _sndBUFFER::IsPlay()
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return 0;
    DWORD st;
    if (DSB(this)->GetStatus(&st))
        return 0;
    return (st & DSBSTATUS_PLAYING) ? 1 : 0;
}

// v12 0x004B9F30
void _sndBUFFER::Stop()
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return;
    flags &= ~SNDBF_PAUSED;
    flags &= ~SNDBF_LOOP;
    flags &= ~SNDBF_PLAYING;
    DWORD st;
    if (DSB(this)->GetStatus(&st) == 0 && (st & DSBSTATUS_PLAYING))
        DSB(this)->Stop();
}

// v12 0x004B9F90
void _sndBUFFER::Pause()
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return;
    flags |= SNDBF_PAUSED;
    DSB(this)->Stop();
}

// v12 0x004B9FC0
void _sndBUFFER::Continue()
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return;
    flags &= ~SNDBF_PAUSED;
    Play((flags & SNDBF_LOOP) ? 5 : 4);
}

// v12 0x004BA000
// The buffer's state to DirectSound: volume, start position and, for a 3D buffer, mode, position, velocity and the
// distances (immediately).
int _sndBUFFER::ApplyParams()
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return 0;
    ApplyVolume();
    SetCurPlayTime(playMs, 1);
    if (!pDS3DB)
        return 1;
    if (DS3DB(this)->SetMode((flags & SNDBF_3D) ? DS3DMODE_NORMAL : DS3DMODE_DISABLE, DS3D_IMMEDIATE))
        return 0;
    if (DS3DB(this)->SetPosition(pos.x, pos.y, pos.z, DS3D_IMMEDIATE))
        return 0;
    if (DS3DB(this)->SetVelocity(vel.x, vel.y, vel.z, DS3D_IMMEDIATE))
        return 0;
    if (DS3DB(this)->SetMinDistance(minDist, DS3D_IMMEDIATE))
        return 0;
    if (DS3DB(this)->SetMaxDistance(maxDist, DS3D_IMMEDIATE))
        return 0;
    return 1;
}

// v12 0x004BA0D0
void *__fastcall sndMapFile(char *path)
{
    HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (h == INVALID_HANDLE_VALUE)
        return 0;
    HANDLE m = CreateFileMappingA(h, 0, PAGE_READONLY, 0, 0, 0);
    if (m == INVALID_HANDLE_VALUE) {
        CloseHandle(h);
        return 0;
    }
    CloseHandle(h);
    void *v = MapViewOfFile(m, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(m);
    if (!v)
        return 0;
    return v;
}

// v12 0x004BA150
void __fastcall sndUnmapFile(void *view)
{
    if (view)
        UnmapViewOfFile(view);
}

// v12 0x004BA160
int _sndBUFFER::Enable3D(int on, int now)
{
    if (!SYS_ON())
        return 0;
    int is = flags & SNDBF_3D;
    if ((is && on) || (!is && !on))
        return 0;
    if (on)
        flags |= SNDBF_3D;
    else
        flags &= ~SNDBF_3D;
    if (!pDS3DB)
        return 0;
    return DS3DB(this)->SetMode(on ? DS3DMODE_NORMAL : DS3DMODE_DISABLE, now ? DS3D_IMMEDIATE : DS3D_DEFERRED) == 0;
}

// v12 0x004BA1E0
// Position and velocity (0: still); nothing happens when both are the same to 1e-6.
int _sndBUFFER::Set3DPosVel(m3dV *p, m3dV *v, int now)
{
    m3dV zero = m3dVZero;
    if (!SYS_ON())
        return 0;
    if (!p)
        return 0;
    if (!v)
        v = &zero;
    if (fabsf(pos.x - p->x) < sndEps && fabsf(pos.y - p->y) < sndEps && fabsf(pos.z - p->z) < sndEps &&
        fabsf(vel.x - v->x) < sndEps && fabsf(vel.y - v->y) < sndEps && fabsf(vel.z - v->z) < sndEps)
        return 0;
    pos = *p;
    vel = *v;
    if (!pDS3DB)
        return 0;
    DWORD apply = now ? DS3D_IMMEDIATE : DS3D_DEFERRED;
    if (DS3DB(this)->SetPosition(pos.x, pos.y, pos.z, apply))
        return 0;
    if (DS3DB(this)->SetVelocity(vel.x, vel.y, vel.z, apply))
        return 0;
    return 1;
}

// v12 0x004BA470
// (The validity tests are on the distances already set, as in the original.)
int _sndBUFFER::Set3DMinMaxDist(float mn, float mx, int now)
{
    if (!SYS_ON())
        return 0;
    if (minDist > maxDist)
        return 0;
    if (minDist <= sndEps)
        return 0;
    if (fabsf(mn - minDist) < sndEps && fabsf(mx - maxDist) < sndEps)
        return 0;
    minDist = mn;
    maxDist = mx;
    if (!pDS3DB)
        return 0;
    DWORD apply = now ? DS3D_IMMEDIATE : DS3D_DEFERRED;
    if (DS3DB(this)->SetMinDistance(mn, apply))
        return 0;
    if (DS3DB(this)->SetMaxDistance(maxDist, apply))
        return 0;
    return 1;
}

// v12 0x00502750 (a shared "return 0")
int _sndBUFFER::SetConeParams(float, float, float, int)
{
    return 0;
}

// v12 0x005EB030 (a shared "return 0")
int _sndBUFFER::SetConeOrientation(m3dV *, int)
{
    return 0;
}

// ---------------------------------------------------------------------------------------------------- _sndSTR_BUFFER

// v12 0x004BA5B0
_sndSTR_BUFFER::_sndSTR_BUFFER() : _sndBUFFER()
{
    dataSize = 0;
    written = 0;
    view = 0;
    data = 0;
}

// v12 0x004BA650
int _sndSTR_BUFFER::Init(char *file)
{
    view = sndMapFile(file);            // (inline in the original)
    if (!view)
        return 0;
    sndFORMAT f;
    if (sndParseWaveResource(view, &f, &data, (unsigned long *)&dataSize) && Init(&f, 0) && Write(data, size, 0))
        return 1;
    if (view)
        UnmapViewOfFile(view);
    return 0;
}

// v12 0x004BA740
int _sndSTR_BUFFER::Init(sndFORMAT *f, int)
{
    flags |= SNDBF_STR_FILL;
    sndWAVEFMT w;
    w.tag = 1;
    w.channels = f->channels;
    w.rate = f->rate;
    w.bytesPerSec = f->bytesPerSec;
    w.align = f->align;
    w.bits = f->bits;
    w.cbSize = 0;
    return CreateObj(&w, 0x14820, 0);
}

// v12 0x004BA7A0
void _sndSTR_BUFFER::Term()
{
    if (SYS_ON() && (flags & SNDBF_CREATED)) {
        if (pDSB)
            SYS->nBuffers--;
        if (pDS3DB)
            DS3DB(this)->Release();
        pDS3DB = 0;
        if (pDSB)
            DSB(this)->Release();
        pDSB = 0;
        flags &= ~SNDBF_CREATED;
        flags &= ~(SNDBF_PLAYING | SNDBF_LOOP | SNDBF_PAUSED);
        flags &= ~(SNDBF_LOADED | SNDBF_DUP);
    }
    if (view)
        UnmapViewOfFile(view);
    view = 0;
    data = 0;
}

// the end of the data in a half that is not looping: Stop (inline in the original)
static void strStop(_sndSTR_BUFFER *b)
{
    if (!SYS_ON() || !(b->flags & SNDBF_CREATED))
        return;
    b->flags &= ~SNDBF_PAUSED;
    b->flags &= ~SNDBF_LOOP;
    b->flags &= ~SNDBF_PLAYING;
    DWORD st;
    if (DSB(b)->GetStatus(&st) == 0 && (st & DSBSTATUS_PLAYING))
        DSB(b)->Stop();
}

// v12 0x004BA820
// Double buffering: when the play cursor has moved into the other half, the half it left is refilled with the next
// data (wrapping to the start of the data when looping, stopping at its end otherwise). STR_FILL refills both.
int _sndSTR_BUFFER::Update()
{
    if (!SYS_ON() || !(flags & SNDBF_CREATED))
        return 0;
    DWORD play = 0;
    if (!(flags & SNDBF_STR_FILL) && DSB(this)->GetCurrentPosition(&play, 0))
        return 0;
    unsigned half = size >> 1;
    unsigned left = dataSize - written;
    if (play < half)
        flags |= SNDBF_STR_HALF1;
    if ((flags & SNDBF_STR_FILL) || ((flags & SNDBF_STR_HALF1) && play > half)) {
        if (half < left) {
            Write(data + written, half, 0);
            written += half;
        } else if (!(flags & SNDBF_LOOP)) {
            strStop(this);
            return 1;
        } else {
            Write(data + written, left, 0);
            Write(data, half - left, left);
            written = half - left;
        }
        flags &= ~SNDBF_STR_HALF1;
    }
    left = dataSize - written;
    if (play > half)
        flags |= SNDBF_STR_HALF2;
    if ((flags & SNDBF_STR_FILL) || ((flags & SNDBF_STR_HALF2) && play < half)) {
        if (half < left) {
            Write(data + written, half, half);
            written += half;
        } else if (!(flags & SNDBF_LOOP)) {
            strStop(this);
            return 1;
        } else {
            Write(data + written, left, half);
            Write(data, half - left, left + half);
            written = half - left;
        }
        flags &= ~SNDBF_STR_HALF2;
    }
    flags &= ~SNDBF_STR_FILL;
    return 1;
}

// v12 0x004BAA50
// The position is not used: the next Update refills from where the data is.
void _sndSTR_BUFFER::SetCurPlayTime(unsigned)
{
    if (flags & SNDBF_CREATED)
        flags |= SNDBF_STR_FILL;
}

// v12 0x004BAA70
// (The written part is added with the sign the original gives it.)
unsigned _sndSTR_BUFFER::GetCurPlayTime()
{
    if (!(flags & SNDBF_CREATED))
        return 0;
    int t = 0;
    if (SYS_ON()) {
        DWORD play;
        if (DSB(this)->GetCurrentPosition(&play, 0) == 0)
            t = sndFtol((int)play * 1000.0f / (double)(int)fmt.bytesPerSec);
    }
    return t - sndFtol((int)written * -1000.0f / (double)(int)fmt.bytesPerSec);
}

// v12 0x004BAB00
unsigned _sndSTR_BUFFER::GetTotalTime()
{
    if (!(flags & SNDBF_CREATED))
        return 0;
    return sndFtol((int)dataSize * 1000.0f / (double)(int)fmt.bytesPerSec);
}

// ---------------------------------------------------------------------------------------------------- wav

// v12 0x004BAB40
// A RIFF WAVE image: the "fmt " chunk (at least 14 bytes; the format tag is not checked) and the "data" chunk.
int __fastcall sndParseWaveResource(void *wav, sndFORMAT *f, uint8_t **data, unsigned long *sz)
{
    uint32_t *h = (uint32_t *)wav;
    if (h[0] != 0x46464952 || h[2] != 0x45564157)       // "RIFF" ... "WAVE"
        return 0;
    uint8_t *c = (uint8_t *)wav + 12;
    uint8_t *end = c + h[1] - 4;
    uint32_t tagCh = 0, rate = 0, bps = 0, alignBits = 0;
    while (c < end) {
        uint32_t id = ((uint32_t *)c)[0], len = ((uint32_t *)c)[1];
        c += 8;
        if (id == 0x20746D66) {                          // "fmt "
            if (len < 0xE)
                return 0;
            tagCh = ((uint32_t *)c)[0];
            rate = ((uint32_t *)c)[1];
            bps = ((uint32_t *)c)[2];
            alignBits = ((uint32_t *)c)[3];
        } else if (id == 0x61746164) {                   // "data"
            *data = c;
            *sz = len;
        }
        c += (len + 1) & ~1u;
    }
    f->align = (uint16_t)alignBits;
    f->rate = rate;
    f->bytesPerSec = bps;
    f->bits = (uint16_t)(alignBits >> 16);
    f->channels = (uint16_t)(tagCh >> 16);
    return 1;
}
