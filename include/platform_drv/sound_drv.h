// Will Rock 1.2 unofficial SDK - platform_drv: sound driver (drv:Snd_sys.obj, Snd_buf.obj, Snd_mus.obj, Snd_mng.obj,
// Snd_chn.obj).  API: DirectSound 8 (DSOUND ord 11 DirectSoundCreate8 + ord 2 DirectSoundEnumerateA) for samples,
// DirectShow (ole32 CoCreateInstance CLSID_FilterGraph) for music.
// Assumes from re/sdk/core: m3dV, apNAME.  Detailed volume/pitch notes and evidence: re/results/sound.json.
// BIG 1.2 vs feb DIFFERENCE: retail rewrote this layer for DirectSound8. feb imported DSOUND ord 1 (DirectSoundCreate,
// IDirectSound), v12 imports ord 11 (DirectSoundCreate8). Layouts and vtables below are the v12 ones.
#pragma once
#include "../core/wr_core.h"   // shared core types (integrator)
#include <stdint.h>

struct m3dV;
#pragma pack(push, 1)
struct sndFORMAT { uint16_t channels; uint32_t rate, bytesPerSec; uint16_t align, bits; }; // 0x0E: the wav "fmt " chunk
                                                    // without the format tag (sndParseWaveResource); buffers assume PCM
struct sndWAVEFMT { uint16_t tag, channels; uint32_t rate, bytesPerSec; uint16_t align, bits, cbSize; }; // = WAVEFORMATEX
#pragma pack(pop)

struct sndMUSIC {                 // 0x50 bytes (unchanged)
    uint32_t _00;                 // +0x00 flags: 1 loop, 2 restart from the beginning, 4 ?, 0x100000 << g group g unmutable
    void SetMuteGroup(int canMute, int group);            // v12 0x004B58E0 canMute 0: unmutable by group g (0x100000 << g)
    char     name[0x40];          // +0x04 apNAME, file name looked up with fnmFindDisc in the music folder
    float    volumeDb;            // +0x44
    uint32_t pos[2];              // +0x48/+0x4C position
};

class sndBUFFER;

// _sndSYSTEM: object 0x16A2 bytes in v12 (feb 0x1676); made by sndSYSTEM::make (new + ctor v12 0x004B4F60)
#pragma pack(push, 1)
class _sndSYSTEM {
public:
    // vtable v12 0x00606788 (feb 0x005DB248). 22 slots in v12, 24 in feb (feb slots 10/11 GetVolume/SetVolume removed).
    _sndSYSTEM();                                           // v12 0x004B4F60 (src/drv/Snd_sys.cpp)
    static _sndSYSTEM *make();                              // v12 0x004B47B0 _sndSystem = new _sndSYSTEM
    int  BaseSetUpListener(m3dV *pos, m3dV *front, m3dV *top, m3dV *vel, int now); // v12 0x004B47E0 (the base class slot)
    virtual void *__vecDelDtor(unsigned f);                 // slot +0x00 v12 0x004B50A0 (feb 0x004A5C70) deleting dtor
    virtual sndBUFFER *makeBuffer();                        // slot +0x04 v12 0x004B4F20 (feb 0x004A5B80)
    virtual sndBUFFER *makeStrBuffer();                     // slot +0x08 v12 0x004B4F40 (feb 0x004A5BA0) streamed buffer
    virtual int  Init();                                    // slot +0x0C v12 0x004B5120 (feb 0x004A5CF0) enum devices, DirectSoundCreate8,
                                                            //   SetCooperativeLevel(apHMainWindow, DSSCL_PRIORITY=2), primary buffer
                                                            //   (DSBCAPS 0x91 = PRIMARY|CTRL3D|CTRLVOLUME), SetFormat, Play(loop),
                                                            //   QI IDirectSound3DListener, SetRolloffFactor(1.0)
    virtual void Term();                                    // slot +0x10 v12 0x004B5320 (feb 0x004A5EF0)
    virtual void SetFormat(sndFORMAT *);                    // slot +0x14 v12 0x004B5380 (feb 0x004A5F50) primary buffer format
    virtual void CheckPrimaryLost();                        // slot +0x18 v12 0x004B5450 (feb 0x004A6020) DSERR_BUFFERLOST -> Restore
    virtual float GetPrimaryVolume();                       // slot +0x1C v12 0x004B54D0 (feb 0x004A60A0)
    virtual void SetPrimaryVolume(float);                   // slot +0x20 v12 0x004B5490 (feb 0x004A6060) never called in 1.2
    virtual int  GetNmbHwBuf(int *max3D, int *free3D);      // slot +0x24 v12 0x004B5510 (feb 0x004A60E0) DSCAPS hardware 3D buffers
    virtual int  SetUpListener(m3dV *pos, m3dV *front, m3dV *top, m3dV *vel, int now); // slot +0x28 v12 0x004B55F0 (feb +0x30 0x004A61C0)
    virtual void SetDopplerShift(float);                    // slot +0x2C v12 0x004B5570 (feb +0x34 0x004A6140)
    virtual void SetAttenuationPow(float);                  // slot +0x30 v12 0x004B55A0 (feb +0x38 0x004A6170) rolloff; gsSND_SYSTEM::Init passes 0.5
    virtual void CommitSettings();                          // slot +0x34 v12 0x004B55D0 (feb +0x3C 0x004A61A0) listener CommitDeferredSettings
    virtual int  InitMusic(char *folder);                   // slot +0x38 v12 0x004B5910 (feb +0x40 0x004AABA0) FilterGraph, IMediaControl, IMediaSeeking, IBasicAudio
    virtual void TermMusic();                               // slot +0x3C v12 0x004B59F0 (feb +0x44 0x004AAC80)
    virtual int  SetMusic(sndMUSIC *, int);                 // slot +0x40 v12 0x004B5AA0 (feb +0x48 0x004AAD30) fnmFindDisc + RenderFile(wide name) + Run
    virtual void StopMusic(sndMUSIC *t, int fade);          // slot +0x44 v12 0x004B5E40 (feb +0x4C 0x004AB090) tail-jumps to slot +0x40
    virtual sndMUSIC *GetMusic();                           // slot +0x48 v12 0x004AC320 (feb +0x50 0x004A6470) returns [this+0xC] (shared getter)
    virtual void MuteMusic(int, int);                       // slot +0x4C v12 0x004B5E60 (feb +0x54 0x004AB0B0)
    virtual void UpdateMusic(float dt);                     // slot +0x50 v12 0x004B5F90 (feb +0x58 0x004AB1E0) loop/position via IMediaSeeking
    virtual void SetMusicVolume(float dB);                  // slot +0x54 v12 0x004B5F20 (feb +0x5C 0x004AB170) IBasicAudio::put_Volume

    uint32_t flags;               // v12 +0x004 (feb +0x004) 1 inited, 0x10 music inited, 0x20 music fading out, 0x40 a track
                                  //   set, 0x80 SetMusic forced, 0x100 graph running, 0x1000000 << g music muted by group g (0..5)
    uint32_t state;               // v12 +0x008 (feb +0x008) 0 ok, 1 device busy (DSERR_ALLOCATED 0x8878000A: game runs silent), 2 failed
    sndMUSIC *curMusic;           // v12 +0x00C (feb +0x010)
    char     musicDir[0x104];     // v12 +0x010 (feb +0x014)
    float    musicVolume;         // v12 +0x114 (feb +0x118) dB
    int      nBuffers;            // v12 +0x118 DirectSound buffers alive (_sndBUFFER CreateObj / Duplicate / Term)
    float    lstPos[3], lstFront[3], lstTop[3], lstVel[3]; // v12 +0x11C/+0x128/+0x134/+0x140 the listener (SetUpListener)
    void    *pDS;                 // v12 +0x14C (feb +0x11C) IDirectSound8* (Init v12 0x004B515B)
    void    *pPrimary;            // v12 +0x150 (feb +0x120) primary IDirectSoundBuffer (CreateSoundBuffer v12 0x004B524D)
    void    *pListener;           // v12 +0x154 (feb +0x124) IDirectSound3DListener (QI at v12 0x004B528E)
    sndWAVEFMT primaryFmt;        // v12 +0x158 (feb +0x128) GetFormat(…,0x12) at v12 0x004B5264
    uint8_t  dsCaps[0x60];        // v12 +0x16A (feb +0x13A) DSCAPS (dwSize 0x60), IDirectSound::GetCaps at v12 0x004B5215
    int      nDevices;            // v12 +0x1CA (feb +0x19A) filled by _sndDSEnumProc (v12 0x004B5790)
    struct { int skip; uint32_t guid[4]; char desc[0xFF]; char module[0xFF]; } devices[10]; // v12 +0x1CE (feb +0x19E), stride 0x212;
                                  //   skip = 1 for the primary driver (no GUID); no bound check on the count (original)
    // music (DirectShow) - v12 offsets = feb + 0x2C (verified in v12 Snd_mus 0x004B5910..)
    void    *pGraph;              // v12 +0x1682 (feb +0x1656) IGraphBuilder (CoCreateInstance at v12 0x004B593D)
    void    *pMediaControl;       // v12 +0x1686 (feb +0x165A) IMediaControl
    void    *pMediaSeeking;       // v12 +0x168A (feb +0x165E) IMediaSeeking
    void    *pSource;             // v12 +0x168E (feb +0x1662) the file's source filter (AddSourceFilter)
    void    *pSourcePin;          // v12 +0x1692 (feb +0x1666) its "Output" pin
    void    *pBasicAudio;         // v12 +0x1696 (feb +0x166A) IBasicAudio
    float    musicCurVolume;      // v12 +0x169A (feb +0x166E) dB now (with the track's and the fade)
    float    musicFadeTime;       // v12 +0x169E (feb +0x1672) seconds into a fade-out (flags 0x20)
};
#pragma pack(pop)

#pragma pack(push, 1)
// sndBUFFER: one sample buffer, the interface the channels and the sample cache talk to (drv:Snd_buf.obj). Rebuilt
// from the 1.2 code in src/drv/Snd_buf.cpp. _sndBUFFER is the DirectSound 8 buffer (vtable v12 0x006068D0, 27
// slots), _sndSTR_BUFFER the streamed one (vtable v12 0x006069B0, 29 slots: a 0x14820-byte buffer refilled half by
// half from the mapped .wav by Update). The base vtable (v12 0x00606940) is mostly __purecall.
enum sndBUFFER_FLAGS {
    SNDBF_CREATED   = 0x001,      // the DirectSound buffer exists
    SNDBF_PLAYING   = 0x002,      // Play called (cleared by Stop)
    SNDBF_LOOP      = 0x004,      // played with flag 1
    SNDBF_PAUSED    = 0x008,
    SNDBF_3D        = 0x010,      // 3D processing on (ctor default; Enable3D)
    SNDBF_LOADED    = 0x020,      // created by CreateObj
    SNDBF_DUP       = 0x040,      // made by Duplicate
    SNDBF_STR_HALF1 = 0x100,      // streamed: the play cursor was in the first half
    SNDBF_STR_HALF2 = 0x200,      // streamed: ... in the second half
    SNDBF_STR_FILL  = 0x400,      // streamed: refill both halves from the data position at the next Update
};
class sndBUFFER {
public:
    virtual void *__vecDelDtor(unsigned f);                // slot +0x00 v12 0x004B9430 (sets the base vtable, frees)
    virtual int  Init(sndFORMAT *fmt, int size) = 0;       // slot +0x04 v12 0x004B9540 (streamed 0x004BA740)
    virtual int  Init(char *file) = 0;                     // slot +0x08 v12 0x004B9450 the .wav, mapped (streamed 0x004BA650)
    virtual void Term() = 0;                               // slot +0x0C v12 0x004B9820 (streamed 0x004BA7A0)
    virtual int  CheckLost() = 0;                          // slot +0x10 v12 0x004B9AC0 BUFFERLOST -> Restore, 0
    virtual int  Update() = 0;                             // slot +0x14 v12 0x004031F0 (1) / streamed refill 0x004BA820
    virtual int  SetVolume(float dB) = 0;                  // slot +0x18 v12 0x004B9C30
    virtual int  ApplyVolume() = 0;                        // slot +0x1C v12 0x004B9CA0 with the cache master volume
    virtual void SetCurPlayTime(unsigned ms, int apply) = 0; // slot +0x20 v12 0x004B9D10
    virtual unsigned GetCurPlayTime(int fromBuffer) = 0;   // slot +0x24 v12 0x004B9D80
    virtual unsigned GetTotalTime() = 0;                   // slot +0x28 v12 0x004B9E10 ms (streamed 0x004BAB00)
    virtual void Play(int flags) = 0;                      // slot +0x2C v12 0x004B9E40 1 loop, 2 from 0, 4 from playMs
    virtual int  IsPlay() = 0;                             // slot +0x30 v12 0x004B9EF0
    virtual void Stop() = 0;                               // slot +0x34 v12 0x004B9F30
    virtual void Pause() = 0;                              // slot +0x38 v12 0x004B9F90
    virtual void Continue() = 0;                           // slot +0x3C v12 0x004B9FC0
    virtual int  Duplicate(sndBUFFER *src) = 0;            // slot +0x40 v12 0x004B9890 DuplicateSoundBuffer
    virtual int  TakeOver(sndBUFFER *src) = 0;             // slot +0x44 v12 0x004B99A0 takes the source DirectSound buffer
    virtual int  Write(void *data, unsigned n, unsigned ofs) = 0; // slot +0x48 v12 0x004B9B10
    virtual int  IsSoftware() = 0;                         // slot +0x4C v12 0x004B97D0 not DSBCAPS_LOCHARDWARE
    virtual int  SetPan(float pan) = 0;                    // slot +0x50 v12 0x004B9CF0 1.2: does nothing
    virtual float GetPan() = 0;                            // slot +0x54 v12 0x004B9D00 1.2: 0.0
    virtual int  Set3DPosVel(m3dV *pos, m3dV *vel, int now) = 0;    // slot +0x58 v12 0x004BA1E0
    virtual int  Set3DMinMaxDist(float mn, float mx, int now) = 0;  // slot +0x5C v12 0x004BA470
    virtual int  Enable3D(int on, int now) = 0;            // slot +0x60 v12 0x004BA160
    virtual int  SetConeParams(float inAng, float outAng, float outVol, int now) = 0; // slot +0x64 1.2: does nothing
    virtual int  SetConeOrientation(m3dV *dir, int now) = 0;                          // slot +0x68 1.2: does nothing
    uint32_t  flags;              // v12 +0x04 sndBUFFER_FLAGS
    float     volumeDb;           // v12 +0x08
    uint32_t  playMs;             // v12 +0x0C start position (SetCurPlayTime)
    float     minDist;            // v12 +0x10 0.1
    float     maxDist;            // v12 +0x14 1e9
    m3dV      pos;                // v12 +0x18 the cache culls one-shots by it
    m3dV      vel;                // v12 +0x24
    sndWAVEFMT fmt;               // v12 +0x30
    void     *pDSB;               // v12 +0x42 IDirectSoundBuffer8
    void     *pDS3DB;             // v12 +0x46 IDirectSound3DBuffer
    uint32_t  size;               // v12 +0x4A buffer bytes
};
class _sndBUFFER : public sndBUFFER {
public:
    _sndBUFFER();                                          // inline at every new (v12 makeBuffer 0x004B4F20)
    int  Init(sndFORMAT *fmt, int size);
    int  Init(char *file);
    void Term();
    int  CheckLost();
    int  Update();
    int  SetVolume(float dB);
    int  ApplyVolume();
    void SetCurPlayTime(unsigned ms, int apply);
    unsigned GetCurPlayTime(int fromBuffer);
    unsigned GetTotalTime();
    void Play(int flags);
    int  IsPlay();
    void Stop();
    void Pause();
    void Continue();
    int  Duplicate(sndBUFFER *src);
    int  TakeOver(sndBUFFER *src);
    int  Write(void *data, unsigned n, unsigned ofs);
    int  IsSoftware();
    int  SetPan(float pan);
    float GetPan();
    int  Set3DPosVel(m3dV *pos, m3dV *vel, int now);
    int  Set3DMinMaxDist(float mn, float mx, int now);
    int  Enable3D(int on, int now);
    int  SetConeParams(float inAng, float outAng, float outVol, int now);
    int  SetConeOrientation(m3dV *dir, int now);
    int  CreateObj(sndWAVEFMT *wf, int size, int unused);  // v12 0x004B95D0
    int  ApplyParams();                                    // v12 0x004BA000 volume, position, 3D state to DirectSound
};
class _sndSTR_BUFFER : public _sndBUFFER {
public:
    _sndSTR_BUFFER();                                      // v12 0x004BA5B0
    using _sndBUFFER::SetCurPlayTime;
    using _sndBUFFER::GetCurPlayTime;
    int  Init(sndFORMAT *fmt, int size);                   // v12 0x004BA740
    int  Init(char *file);                                 // v12 0x004BA650
    void Term();                                           // v12 0x004BA7A0
    int  Update();                                         // v12 0x004BA820
    unsigned GetTotalTime();                               // v12 0x004BAB00
    virtual void SetCurPlayTime(unsigned ms);              // slot +0x6C v12 0x004BAA50 (only asks for a refill)
    virtual unsigned GetCurPlayTime();                     // slot +0x70 v12 0x004BAA70
    uint32_t  dataSize;           // v12 +0x4E the wav data
    uint32_t  written;            // v12 +0x52 bytes of it already in the buffer
    void     *view;               // v12 +0x56 the mapped file
    uint8_t  *data;               // v12 +0x5A its data chunk
};
int   __fastcall sndParseWaveResource(void *wav, sndFORMAT *fmt, uint8_t **data, unsigned long *size); // v12 0x004BAB40
void *__fastcall sndMapFile(char *path);                   // v12 0x004BA0D0 read-only view of a file (0: failed)
void  __fastcall sndUnmapFile(void *view);                 // v12 0x004BA150

// The sample cache (drv:Snd_mng.obj, rebuilt in src/drv/Snd_mng.cpp). One snmSOUND_DATA per sound file, up to 400
// of them; each keeps up to 10 buffers (1.2: feb had 4) that channels borrow with GetBuffer and give back with
// DropBuffer. Sounds load on first play, unload 15 s after their last buffer was given back; looping sounds of one
// file share at most 2 playing buffers (the nearest by the order UpdateLoops sorts them in).
enum snmDATA_FLAGS {
    SNMD_STREAMED  = 0x0001,      // buffers are _sndSTR_BUFFER (descr "isStreaming")
    SNMD_NO_LOAD   = 0x0002,      // skipped by LoadAll // ? (descr "isAutoLoad" gives 6)
    SNMD_DROP      = 0x0004,      // unloaded as soon as no buffer is in use
    SNMD_LOADED    = 0x1000,      // the first slot's buffer holds the file
    SNMD_FOUND     = 0x2000,      // the file was found and parsed (Find)
};
enum snmSLOT_FLAGS {
    SNMS_USED      = 0x01,        // lent to a channel (GetBuffer)
    SNMS_AUTODROP  = 0x02,        // give back when it stops playing (Update)
    SNMS_PLAYING   = 0x04,        // PlayBuffer called
    SNMS_LOOP      = 0x08,        // ... looping
    SNMS_LOOP_STOP = 0x10,        // a loop was stopped: UpdateLoops stops its buffer
};
struct snmSLOT {                  // 0x10
    uint32_t   flags;             // +0x00 snmSLOT_FLAGS
    sndBUFFER *buf;               // +0x04
    uint32_t   time;              // +0x08 ms of the last give-back
    float      key;               // +0x0C UpdateLoops sort key (loops: 0..2 by distance; others 2e6 / 4e6 / 6e6)
};
class snmSOUND_DATA {             // 0x1F4, made by snmMANAGER::Add
public:
    uint32_t  flags;              // +0x000 snmDATA_FLAGS
    apNAME    name;               // +0x004 the file name without extension
    char      path[0x104];        // +0x044 the file found (Find)
    snmSLOT   slot[10];           // +0x148
    uint32_t  lengthMs;           // +0x1E8
    uint32_t  lastUse;            // +0x1EC ms
    uint32_t  lastPlay;           // +0x1F0 ms of the last one-shot start / loop update
    int  Find(char *dir, char *ext);                       // v12 0x004B68F0
    int  Validate(snmSLOT *s);                             // v12 0x004B69D0 load the file into s (0: the first slot)
    void Invalidate();                                     // v12 0x004B6A60 unload when unused
    int  Update();                                         // v12 0x004B6B70
    sndBUFFER *GetBuffer();                                // v12 0x004B6C90
    void DropBuffer(sndBUFFER *b);                         // v12 0x004B6D70
    void AutoDropBuffer(sndBUFFER *b);                     // v12 0x004B6DD0
    void PlayBuffer(sndBUFFER *b, int flags);              // v12 0x004B6E30 1 loop
    int  IsBufferPlay(sndBUFFER *b);                       // v12 0x004B6E90
    void StopBuffer(sndBUFFER *b);                         // v12 0x004B6EE0
    void PlayOneShot(sndBUFFER *b);                        // v12 0x004B6F90
    void UpdateLoops();                                    // v12 0x004B7370
    void GetStats(int *nSoft, int *nHard, int *playSoft, int *playHard); // v12 0x004B78F0 (adds)
};
class snmMANAGER {
public:
    uint32_t  flags;              // +0x000 1: no idle unloading
    char      dir[0x104];         // +0x004 the sounds folder
    char      ext[0x40];          // +0x108 "wav"
    int       next;               // +0x148 where Add looks for a free entry first
    float     masterVolume;       // +0x14C dB, added by every _sndBUFFER::ApplyVolume
    snmSOUND_DATA **data;         // +0x150 400 entries
    uint32_t  time;               // +0x154 timeGetTime of this frame
    int       nSoft;              // +0x158 software buffers (BeginFrame)
    int       nHard;              // +0x15C hardware buffers
    int       nPlaying;           // +0x160 buffers playing
    int       nPlaying2;          // +0x164 0 (counted into nPlaying)
    int  Init(char *dir, char *ext);                       // v12 0x004B79A0
    void Term();                                           // v12 0x004B7A00
    snmSOUND_DATA *Add(char *name, int flags);             // v12 0x004B7B80
    int  Del(snmSOUND_DATA *d);                            // v12 0x004B7CD0
    int  LoadAll();                                        // v12 0x004B7DB0 failures
    void BeginFrame();                                     // v12 0x004B7E10
    void SetMasterVolume(float dB);                        // v12 0x004B7E90
};
extern snmMANAGER *snmManager;    // v12 0x00656204
#pragma pack(pop)


// Globals
extern _sndSYSTEM *_sndSystem;      // v12 0x006561F4 (feb 0x00612014)
extern _sndSYSTEM *sndSystem;       // v12 0x006561F8 (feb 0x00612018) written by gsSND_SYSTEM::Init at v12 0x0051130C
int __stdcall _sndDSEnumProc(void *guid, const char *desc, const char *module, void *ctx); // v12 0x004B5790 (feb 0x004A6330)

// game-side channels (drv:Snd_chn.obj, portable C++ over sndBUFFER): sncCHANNEL 0xBC bytes, +0x68 buffer;
//   sncCHANNEL::Play v12 0x004B0FA0 (feb 0x004A2390), Update v12 0x004B11D0, ValidateBuffer v12 0x004B12E0,
//   sncCHANNEL_LIST::Update v12 0x004B1CA0 (feb 0x004A3040). These need no port work.

// The channel classes (drv:Snd_chn.obj, rebuilt in src/drv/Snd_chn.cpp). A channel plays one of its sound
// descriptions at a time through a buffer borrowed from the sample cache. sncCHANNEL 0x74 (vtable v12 0x00606600),
// sncCHANNEL_3D 0x84 (0x005F9354), sncCHANNEL_CONE 0xBC (0x005F931C), sncCHANNEL_LIST (0x0060663C). The _3D / _CONE
// constructors are inline (at every new). Times are integer milliseconds.
#pragma pack(push, 1)
enum sncDESCR_FLAGS {
    SNCD_NO_3D      = 0x00000001, // isDisable3D
    SNCD_INF_LOOP   = 0x00000002, // isInfLoop
    SNCD_RAND_COUNT = 0x00000200,
    SNCD_RAND_DELAY = 0x00000400, // isRandDelay: delay/2 .. delay
    SNCD_NOT_OVER   = 0x00010000, // not played over a busy channel
    SNCD_KEEP_BUF   = 0x00020000, // the buffer is auto-dropped when the channel goes
    SNCD_SKIP_SAME  = 0x00040000, // isSkipSame
    SNCD_QUEUE_NEXT = 0x00080000, // a sound played over this one waits for its end
    SNCD_NO_QUEUE   = 0x00100000,
    SNCD_ONLY_OVER  = 0x00200000, // only played over a busy channel
    SNCD_DELAY_FIRST = 0x00400000, // the delay comes before the sound
    SNCD_LOOPING    = 0x10000000, // set by Play: the buffer plays looping
};
// sncSOUND_DESCR (0x60, vtable v12 0x005F94AC) / _3D (0x68, 0x005F9008): one sound of a channel, read from a
// property sheet ("%sname volume delay time isInfLoop isRandDelay isSkipSame isStreaming isAutoLoad [minDist maxDist
// isDisable3D]") or linked from a static table (sncLinkSndListData).
struct sncSOUND_DESCR {
    void  *vtbl;
    apNAME name;                  // +0x04
    u32    flags;                 // +0x44 sncDESCR_FLAGS
    float  volume;                // +0x48 dB
    int    delay;                 // +0x4C ms
    int    count;                 // +0x50
    int    time;                  // +0x54 ms (0: count x (length + delay))
    class snmSOUND_DATA *data;    // +0x58
    u32    dataFlags;             // +0x5C snmDATA_FLAGS: 1 streamed, 6 autoload
    int    LinkData(char *name, int dataFlags);                 // v12 0x004B0980
    void   UnlinkData();                                        // v12 0x004B09D0
    int    ReadParams(psSHEET *ps, char *sect, char *prefix);   // v12 0x004B0A10
};
struct sncSOUND_DESCR_3D : sncSOUND_DESCR {
    float  minDist;               // +0x60
    float  maxDist;               // +0x64
    int    ReadParams(psSHEET *ps, char *sect, char *prefix);   // v12 0x004B0C50
};
int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR *d);    // v12 0x004B1DE0
int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *d); // v12 0x004B1E30

enum sncCHANNEL_FLAGS {
    SNCC_NO_3D       = 0x00000001,
    SNCC_AUTODROP    = 0x00000002,
    SNCC_MUTED       = 0x0003F000, // 0x1000 << g: muted by group g (0..5)
    SNCC_LISTENER    = 0x10000000, // _3D: the listener position is known
    SNCC_SOURCE      = 0x20000000, // _CONE: the source position is set
};
#define SNCC_MUTE(g)      (0x1000u << (g))
#define SNCC_UNMUTABLE(g) (0x100000u << (g))
class sncCHANNEL {
public:
    virtual void *__vecDelDtor(unsigned f);                 // slot 0
    virtual void  Play(sncSOUND_DESCR *d, sncSOUND_DESCR *next); // slot 1 v12 0x004B0FA0
    virtual int   IsPlay(sncSOUND_DESCR *d);                // slot 2 v12 0x004B11A0
    virtual void  Stop();                                   // slot 3 v12 0x004B1140
    virtual void  Update(int dt, m3dV *listener, m3dV *vel); // slot 4 v12 0x004B11D0
    virtual void  SetParameters(m3dV *pos, m3dV *vel, m3dV *dir); // slot 5 (base: nothing)
    virtual void  ReadParams(psSHEET *ps, char *sect, char *prefix); // slot 6 v12 0x004B0F00 (1.2: "volume" only)
    virtual int   ValidateBuffer(int want);                 // slot 7 v12 0x004B12E0
    virtual void  ValidatePlay(int on);                     // slot 8 v12 0x004B1390
    virtual int   AdvanceTime(int dt, int *audible);        // slot 9 v12 0x004B1420
    virtual int   IsHear();                                 // slot 10 (base 1)
    virtual void  CalcVolumePan(float *vol, float *pan);    // slot 11 v12 0x004B1520
    virtual void  UpdatePos(int now);                       // slot 12 (base: nothing)
    virtual void  Update3D(int now);                        // slot 13 v12 0x004B1500
    sncCHANNEL();                                           // v12 0x004B0D10
    void  Destroy();                                        // v12 0x004B0D70 (the dtor body)
    int   AddDescr(sncSOUND_DESCR *d);                      // v12 0x004B0E10
    void  Mute(int on, int group);                          // v12 0x004B0E60
    void  SetMuteGroup(int canMute, int group);             // v12 0x004B0ED0 canMute 0: unmutable by group g
    void  Play(int i, int next);                            // v12 0x004B0F40 (-1: stop)
    int   IsPlay(int i);                                    // v12 0x004B1170 (-1: anything)
    u32    flags;                 // +0x04 sncCHANNEL_FLAGS
    apNAME name;                  // +0x08
    int    nDescr;                // +0x48
    sncSOUND_DESCR **descr;       // +0x4C
    float  volume;                // +0x50 dB
    float  listVolume;            // +0x54 dB (the channel list's)
    int    timeLeft;              // +0x58 ms the sound lasts (0x7FFFFFFF endless)
    int    played;                // +0x5C ms played
    int    delay;                 // +0x60 ms between repeats
    int    playPos;               // +0x64 ms into the sample
    sndBUFFER *buffer;            // +0x68
    sncSOUND_DESCR *cur;          // +0x6C
    sncSOUND_DESCR *next;         // +0x70
    static int nChannels;         // v12 0x006561A8
    static m3dRND_GEN rndGen;     // v12 0x006561B0
};
class sncCHANNEL_3D : public sncCHANNEL {
public:
    m3dV   listener;              // +0x74
    float  dist;                  // +0x80
    sncCHANNEL_3D() : sncCHANNEL() { listener = m3dVZero; dist = 0.0f; }
    void *__vecDelDtor(unsigned f);                         // v12 0x004B0D50
    void Update(int dt, m3dV *listener, m3dV *vel);         // v12 0x004B1540
    void Update3D(int now);                                 // v12 0x004B1580
};
class sncCHANNEL_CONE : public sncCHANNEL_3D {
public:
    float  minDist;               // +0x84
    float  maxDist;               // +0x88
    float  coneIn;                // +0x8C 360
    float  coneOut;               // +0x90 360
    float  coneVol;               // +0x94 0
    m3dV   pos;                   // +0x98
    m3dV   vel;                   // +0xA4
    m3dV   dir;                   // +0xB0 unit Z
    sncCHANNEL_CONE() : sncCHANNEL_3D()
    {
        minDist = 10.0f;
        maxDist = 50.0f;
        coneIn = 360.0f;
        coneOut = 360.0f;
        coneVol = 0.0f;
        pos = m3dVZero;
        vel = m3dVZero;
        dir = m3dVUnitZ;
    }
    void SetParameters(m3dV *pos, m3dV *vel, m3dV *dir);   // v12 0x004B15D0
    void ReadParams(psSHEET *ps, char *sect, char *prefix); // v12 0x004B1680
    int  ValidateBuffer(int want);                          // v12 0x004B1760
    int  IsHear();                                          // v12 0x004B1830
    void UpdatePos(int now);                                // v12 0x004B19B0
};
class sncCHANNEL_LIST {
public:
    virtual void *__vecDelDtor(unsigned f);                 // [0] vtable v12 0x0060663C
    virtual int  Init(int n);                               // [1] v12 0x004B19F0
    virtual int  Add(sncCHANNEL *c);                        // [2] v12 0x004B1A40 its mute groups and volume to c
    virtual int  Del(sncCHANNEL *c);                        // [3] v12 0x004B1C60
    virtual void Update(u32 time, m3dV *listener, m3dV *vel); // [4] v12 0x004B1CA0
    sncCHANNEL_LIST() : flags(0), n(0), ch(0), lastTime(0), volume(0.0f) {}  // inline
    void   Destroy();                                       // v12 0x004B1A20 (the dtor body)
    void   Mute(int on, int group);                         // v12 0x004B1CF0
    int    GetNmb();                                        // v12 0x004B1DC0
    u32    flags;                 // +0x04 SNCC_MUTE(g) bits
    int    n;                     // +0x08
    sncCHANNEL **ch;              // +0x0C
    u32    lastTime;              // +0x10 ms
    float  volume;                // +0x14 dB
};
#pragma pack(pop)

