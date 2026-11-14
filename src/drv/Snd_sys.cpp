// Snd_sys.cpp -- _sndSYSTEM, the DirectSound 8 sound driver object (Will Rock 1.2, v12.exe; feb obj drv:Snd_sys.obj)
//
// Rebuilt from the 1.2 code (v12 0x004B4F20..0x004B585F): the ctor, the deleting dtor, makeBuffer /
// makeStrBuffer, Init (device enumeration, DirectSoundCreate8, the primary buffer, the 3D listener), Term, the
// format / volume / listener slots. The music slots are in Snd_mus.cpp.

#define DIRECTSOUND_VERSION 0x0800
#include <windows.h>
#include <unknwn.h>
#include <mmsystem.h>
#include <dsound.h>
#include <math.h>
#include <string.h>
#include "../../include/wr_sdk.h"

// v12 0x004B4F60  (feb 0x004A5B00?)
_sndSYSTEM::_sndSYSTEM()
{
    flags = 0;
    state = 0;
    curMusic = 0;
    musicVolume = 0.0f;
    nBuffers = 0;
    memset(lstPos, 0, sizeof(lstPos));
    memset(lstVel, 0, sizeof(lstVel));
    memset(lstFront, 0, sizeof(lstFront));
    memset(lstTop, 0, sizeof(lstTop));
    pDS = 0;
    pPrimary = 0;
    pListener = 0;
    memset(&primaryFmt, 0, sizeof(primaryFmt));
    memset(dsCaps, 0, sizeof(dsCaps));
    nDevices = 0;
    memset(devices, 0, sizeof(devices));
    pGraph = 0;
    pMediaControl = 0;
    pMediaSeeking = 0;
    pSource = 0;
    pSourcePin = 0;
    pBasicAudio = 0;
    curMusic = 0;
    musicFadeTime = 0.0f;
    musicVolume = 0.0f;
    musicCurVolume = 0.0f;
}

// v12 0x004B50A0  (feb 0x004A5C70)
// Releases the listener, the primary buffer and the device, then the music.
void *_sndSYSTEM::__vecDelDtor(unsigned f)
{
    if (pListener)
        ((IUnknown *)pListener)->Release();
    pListener = 0;
    if (pPrimary)
        ((IUnknown *)pPrimary)->Release();
    pPrimary = 0;
    if (pDS)
        ((IUnknown *)pDS)->Release();
    pDS = 0;
    TermMusic();
    if (f & 1)
        operator delete(this);
    return this;
}

_sndSYSTEM *_sndSystem;                     // v12 0x006561F4 (feb 0x00612014)
_sndSYSTEM *sndSystem;                      // v12 0x006561F8 (feb 0x00612018) made by gsSND_SYSTEM::Init

// the GUID of IDirectSound3DListener as the exe has it                                          v12 0x0060A3A8
static const GUID sndIID_IDirectSound3DListener =
    { 0x279AFA84, 0x4981, 0x11CE, { 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60 } };
static const float sndSysEps = 1.0e-6f;     // v12 0x00606780
float sndSysEpsSq;                          // v12 0x006561F0 (static init v12 0x004B5870)
static struct sndSYS_EPS_INIT { sndSYS_EPS_INIT() { sndSysEpsSq = sndSysEps * sndSysEps; } } sndSysEpsInit;

#define DS(s)      ((IDirectSound8 *)(s)->pDS)
#define PRIMARY(s) ((IDirectSoundBuffer *)(s)->pPrimary)
#define LISTENER(s) ((IDirectSound3DListener *)(s)->pListener)

// v12 0x004B47B0 (the base class make, just before Snd_sys.obj)
_sndSYSTEM *_sndSYSTEM::make()
{
    _sndSystem = new _sndSYSTEM;            // operator new(0x16A2)
    return _sndSystem;
}

// v12 0x004B4F20
sndBUFFER *_sndSYSTEM::makeBuffer()
{
    return new _sndBUFFER;                  // operator new(0x4E), ctor v12 0x004B93A0
}

// v12 0x004B4F40
sndBUFFER *_sndSYSTEM::makeStrBuffer()
{
    return new _sndSTR_BUFFER;              // operator new(0x5E), ctor v12 0x004BA5B0
}

// v12 0x004B5120  (feb 0x004A5CF0)
// The first enumerated device that has a GUID, else the default one; priority level on the main window; the caps;
// a looping primary buffer (3D, volume) and its listener, rolloff 1.0. A device another program holds
// (DSERR_ALLOCATED) leaves the game silent (state 1, Init returns 1). Any other failure: state 2 and the object
// DELETES ITSELF (original), returning 0. The stored primary format is changed to mono when it is stereo.
int _sndSYSTEM::Init()
{
    if (DirectSoundEnumerateA((LPDSENUMCALLBACKA)_sndDSEnumProc, this))
        goto fail;
    for (int i = 0; i < nDevices; i++) {
        if (devices[i].skip)
            continue;
        if (DirectSoundCreate8((GUID *)devices[i].guid, (IDirectSound8 **)&pDS, 0) == DSERR_ALLOCATED) {
            state = 1;
            return 1;
        }
        break;
    }
    if (!pDS && DirectSoundCreate8(0, (IDirectSound8 **)&pDS, 0))
        goto fail;
    if (DS(this)->SetCooperativeLevel((HWND)apHMainWindow, DSSCL_PRIORITY) == DSERR_ALLOCATED) {
        if (pDS)
            DS(this)->Release();
        state = 1;
        pDS = 0;
        return 1;
    }
    {
        memset(dsCaps, 0, sizeof(dsCaps));
        *(DWORD *)dsCaps = 0x60;
        if (DS(this)->GetCaps((DSCAPS *)dsCaps))
            goto fail;
        DSBUFFERDESC d;
        memset(&d, 0, sizeof(d));
        d.dwSize = sizeof(DSBUFFERDESC);
        d.dwFlags = DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D | DSBCAPS_CTRLVOLUME;
        if (DS(this)->CreateSoundBuffer(&d, (IDirectSoundBuffer **)&pPrimary, 0))
            goto fail;
        if (PRIMARY(this)->GetFormat((WAVEFORMATEX *)&primaryFmt, 0x12, 0))
            goto fail;
        if (PRIMARY(this)->Play(0, 0, DSBPLAY_LOOPING))
            goto fail;
        if (PRIMARY(this)->QueryInterface(sndIID_IDirectSound3DListener, &pListener))
            goto fail;
        if (LISTENER(this)->SetRolloffFactor(1.0f, DS3D_IMMEDIATE))
            goto fail;
    }
    if (primaryFmt.channels == 2) {
        primaryFmt.channels = 1;
        primaryFmt.align = primaryFmt.bits >> 3;
        primaryFmt.bytesPerSec = primaryFmt.rate * primaryFmt.align;
    }
    flags |= 1;
    state = 0;
    return 1;
fail:
    state = 2;
    __vecDelDtor(1);
    return 0;
}

// v12 0x004B5320  (feb 0x004A5EF0)
void _sndSYSTEM::Term()
{
    if (pListener)
        LISTENER(this)->Release();
    pListener = 0;
    if (pPrimary)
        PRIMARY(this)->Release();
    pPrimary = 0;
    if (pDS)
        DS(this)->Release();
    pDS = 0;
    TermMusic();
}

// v12 0x004B5380  (feb 0x004A5F50)
// The primary buffer format (0: 22050 Hz 16-bit stereo); the stored copy is then made mono like Init does.
void _sndSYSTEM::SetFormat(sndFORMAT *f)
{
    primaryFmt.tag = 1;
    if (f) {
        primaryFmt.bytesPerSec = f->bytesPerSec;
        primaryFmt.align = f->align;
        primaryFmt.channels = f->channels;
        primaryFmt.rate = f->rate;
        primaryFmt.bits = f->bits;
    } else {
        primaryFmt.channels = 2;
        primaryFmt.rate = 22050;
        primaryFmt.bits = 16;
        primaryFmt.align = 4;
        primaryFmt.bytesPerSec = 88200;
    }
    primaryFmt.cbSize = 0;
    if (PRIMARY(this)->SetFormat((WAVEFORMATEX *)&primaryFmt) == 0 && primaryFmt.channels == 2) {
        primaryFmt.channels = 1;
        primaryFmt.align = primaryFmt.bits >> 3;
        primaryFmt.bytesPerSec = primaryFmt.align * primaryFmt.rate;
    }
}

// v12 0x004B5450  (feb 0x004A6020)
void _sndSYSTEM::CheckPrimaryLost()
{
    if (!(flags & 1))
        return;
    DWORD st;
    if (PRIMARY(this)->GetStatus(&st) == 0 && (st & DSBSTATUS_BUFFERLOST))
        PRIMARY(this)->Restore();
}

// v12 0x004B5490  (feb 0x004A6060)
void _sndSYSTEM::SetPrimaryVolume(float dB)
{
    if (!(flags & 1))
        return;
    float v = dB * 100.0f;
    if (v < -10000.0f)
        v = -10000.0f;
    PRIMARY(this)->SetVolume((int)v);
}

// v12 0x004B54D0  (feb 0x004A60A0)
float _sndSYSTEM::GetPrimaryVolume()
{
    if (!(flags & 1))
        return 0.0f;
    LONG v;
    if (PRIMARY(this)->GetVolume(&v))
        return 0.0f;
    return v * 0.01f;
}

// v12 0x004B5510  (feb 0x004A60E0)
// The device's hardware 3D buffers: the most it has and the free ones (DSCAPS, read again).
int _sndSYSTEM::GetNmbHwBuf(int *max3D, int *free3D)
{
    memset(dsCaps, 0, sizeof(dsCaps));
    *(DWORD *)dsCaps = 0x60;
    if (DS(this)->GetCaps((DSCAPS *)dsCaps))
        return 0;
    if (max3D)
        *max3D = ((DSCAPS *)dsCaps)->dwMaxHw3DAllBuffers;
    if (free3D)
        *free3D = ((DSCAPS *)dsCaps)->dwFreeHw3DAllBuffers;
    return 1;
}

// v12 0x004B47E0 (1.2: the base class slot, between vid.obj and Snd_sys.obj; which object file holds it is not known)
// Stores the listener when any of the four vectors moved by 1e-6 or more; 1 when it did.
int _sndSYSTEM::BaseSetUpListener(m3dV *pos, m3dV *front, m3dV *top, m3dV *vel, int)
{
    if (!(flags & 1))
        return 0;
    float *have[4] = { lstPos, lstFront, lstTop, lstVel };
    m3dV *want[4] = { pos, front, top, vel };
    int moved = 0;
    for (int k = 0; k < 4 && !moved; k++)
        if (!(fabsf(have[k][0] - want[k]->x) < sndSysEps) || !(fabsf(have[k][1] - want[k]->y) < sndSysEps) ||
            !(fabsf(have[k][2] - want[k]->z) < sndSysEps))
            moved = 1;
    if (!moved)
        return 0;
    for (int k = 0; k < 4; k++) {
        have[k][0] = want[k]->x;
        have[k][1] = want[k]->y;
        have[k][2] = want[k]->z;
    }
    return 1;
}

// v12 0x004B55F0  (feb +0x30 0x004A61C0)
// The stored listener to DirectSound (distance / rolloff / doppler kept from GetAllParameters). Each velocity
// component is at least 0.05 (all 0.05 without a velocity).
int _sndSYSTEM::SetUpListener(m3dV *pos, m3dV *front, m3dV *top, m3dV *vel, int now)
{
    if (!BaseSetUpListener(pos, front, top, vel, now))
        return 0;
    if (!pListener)
        return 0;
    DS3DLISTENER l;
    memset(&l, 0, sizeof(l));
    l.dwSize = sizeof(DS3DLISTENER);        // 0x40
    if (LISTENER(this)->GetAllParameters(&l))
        return 0;
    l.vPosition.x = lstPos[0];
    l.vPosition.y = lstPos[1];
    l.vPosition.z = lstPos[2];
    if (vel) {
        l.vVelocity.x = lstVel[0] < 0.05f ? 0.05f : lstVel[0];
        l.vVelocity.y = lstVel[1] < 0.05f ? 0.05f : lstVel[1];
        l.vVelocity.z = lstVel[2] < 0.05f ? 0.05f : lstVel[2];
    } else {
        l.vVelocity.x = l.vVelocity.y = l.vVelocity.z = 0.05f;
    }
    l.vOrientFront.x = lstFront[0];
    l.vOrientFront.y = lstFront[1];
    l.vOrientFront.z = lstFront[2];
    l.vOrientTop.x = lstTop[0];
    l.vOrientTop.y = lstTop[1];
    l.vOrientTop.z = lstTop[2];
    return LISTENER(this)->SetAllParameters(&l, now ? DS3D_IMMEDIATE : DS3D_DEFERRED) == 0;
}

// v12 0x004B5570  (feb +0x34 0x004A6140)
void _sndSYSTEM::SetDopplerShift(float f)
{
    if ((flags & 1) && pListener)
        LISTENER(this)->SetDopplerFactor(f * 10.0f, DS3D_IMMEDIATE);
}

// v12 0x004B55A0  (feb +0x38 0x004A6170)
void _sndSYSTEM::SetAttenuationPow(float f)
{
    if ((flags & 1) && pListener)
        LISTENER(this)->SetRolloffFactor(f * 10.0f, DS3D_IMMEDIATE);
}

// v12 0x004B55D0  (feb +0x3C 0x004A61A0)
void _sndSYSTEM::CommitSettings()
{
    if ((flags & 1) && pListener)
        LISTENER(this)->CommitDeferredSettings();
}

// v12 0x004B5790  (feb 0x004A6330)
// DirectSoundEnumerate callback: every device (the primary driver, without a GUID, is marked skip).
int __stdcall _sndDSEnumProc(void *guid, const char *desc, const char *module, void *ctx)
{
    _sndSYSTEM *s = (_sndSYSTEM *)ctx;
    if (guid) {
        memcpy(s->devices[s->nDevices].guid, guid, 16);
        s->devices[s->nDevices].skip = 0;
    } else {
        s->devices[s->nDevices].skip = 1;
    }
    strcpy(s->devices[s->nDevices].desc, desc);
    strcpy(s->devices[s->nDevices].module, module);
    s->nDevices++;
    return 1;
}
