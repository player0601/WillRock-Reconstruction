// Snd_mng.cpp -- snmMANAGER, the sample cache: one snmSOUND_DATA per sound file with up to 10 buffers that the
// channels borrow; loading on demand, unloading of idle sounds, the one-shot voice limits and the sharing of loop
// buffers (Will Rock 1.2, v12.exe; feb obj drv:Snd_mng.obj)
//
// Rebuilt from the 1.2 code (v12 0x004B68F0..0x004B7F20). 1.2 REWROTE this file: 10 slots of 0x10 per sound (feb 4
// of 0xC), the play / stop / loop logic moved here from the channels (PlayBuffer, StopBuffer, PlayOneShot,
// UpdateLoops), the per-frame voice counts. The sounds use sndSystem (the game's driver pointer) to make buffers.
//   - a one-shot 3D sound does not start again within 100 ms; its gain falls from 1 at its min distance to 0 at
//     its max distance from the listener; with more than 8 voices it needs 0.3, with more than 14 0.6;
//   - every 500 ms (and on each loop start / stop) the slots are sorted by a key -- loops by distance, then the
//     others -- and at most 2 loop buffers of the sound play, the first 2 that are loud enough.

#include <windows.h>
#include <mmsystem.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/wr_sdk.h"

snmMANAGER *snmManager;                 // v12 0x00656204

static const float snmEps = 1.0e-6f;    // v12 0x006068BC
float snmEpsSq;                         // v12 0x00656208 (static init v12 0x004B6F70)
static struct snmEPS_INIT { snmEPS_INIT() { snmEpsSq = snmEps * snmEps; } } snmEpsInit;
static const float snmKeyStep = 2.0e6f; // v12 0x006068C0 the sort key unit of the slots that are not loops

#define NOW() (snmManager->time)

static snmSLOT *slotOf(snmSOUND_DATA *d, sndBUFFER *b)
{
    for (int i = 0; i < 10; i++)
        if (d->slot[i].buf == b)
            return &d->slot[i];
    return 0;
}

// a slot's buffer is given up (Invalidate / Term / Del): kept (only terminated) while lent, deleted otherwise
static void unloadSlot(snmSLOT *s)
{
    if (s->flags & SNMS_USED) {
        if (s->buf)
            s->buf->Term();
        return;
    }
    if (s->buf) {
        s->buf->Term();
        if (s->buf)
            s->buf->__vecDelDtor(1);
        s->buf = 0;
    }
    s->flags = 0;
    s->time = 0;
}

static void unloadAll(snmSOUND_DATA *d)
{
    for (int i = 0; i < 10; i++)
        unloadSlot(&d->slot[i]);
    d->flags &= ~SNMD_LOADED;
}

// gain from distance: 1 at or before x0, 0 at or past x1, linear between (as the original orders the bounds)
static float distRamp(float d, float x0, float x1)
{
    float a = x0, ga = 0.0f, b = x1, gb = 1.0f;
    if (!(b > a)) {
        float t = a;
        gb = 0.0f;
        a = b;
        b = t;
        ga = 1.0f;
    }
    if (!(d > a))
        return gb;
    if (!(d < b) && d != b)
        return ga;
    return gb + (ga - gb) * (d - a) / (b - a);
}

static float listenerDist(sndBUFFER *b)
{
    float *l = sndSystem->lstPos;
    float dx = l[0] - b->pos.x, dy = l[1] - b->pos.y, dz = l[2] - b->pos.z;
    return m3dSqrt(dx * dx + dy * dy + dz * dz);
}

// ---------------------------------------------------------------------------------------------------- the sound

// v12 0x004B68F0
// The file (name + ext) in dir or its subfolders; its length from the wav header.
int snmSOUND_DATA::Find(char *dir, char *ext)
{
    char *f = fnmFindDisc(dir, name.str, ext);
    if (!f) {
        apLogErr((char *)"Sound %s not found\n", name.str);
        return 0;
    }
    strcpy(path, f);
    void *v = sndMapFile(path);
    if (!v)
        return 0;
    sndFORMAT fmt;
    uint8_t *data;
    unsigned long size;
    int ok = sndParseWaveResource(v, &fmt, &data, &size);
    sndUnmapFile(v);
    if (!ok)
        return 0;
    lengthMs = (uint32_t)(int)((double)(long long)size / (int)fmt.bytesPerSec * 1000.0f);
    flags |= SNMD_FOUND;
    return 1;
}

// v12 0x004B69D0
int snmSOUND_DATA::Validate(snmSLOT *s)
{
    if (!(flags & SNMD_FOUND))
        return 0;
    if (flags & SNMD_LOADED)
        return 1;
    if (!s)
        s = &slot[0];
    if (!s->buf) {
        s->buf = 0;
        s->flags = 0;
        s->time = 0;
        s->buf = (flags & SNMD_STREAMED) ? sndSystem->makeStrBuffer() : sndSystem->makeBuffer();
        if (!s->buf)
            return 0;
    }
    if (!s->buf->Init(path))
        return 0;
    lengthMs = s->buf->GetTotalTime();
    flags |= SNMD_LOADED;
    return 1;
}

// v12 0x004B6A60
// While a buffer is lent the sound counts as used; otherwise it is unloaded at once (SNMD_DROP) or after 15 s.
void snmSOUND_DATA::Invalidate()
{
    int used = 0;
    for (int i = 0; i < 10; i++)
        if (slot[i].flags & SNMS_USED)
            used = 1;
    if (used) {
        lastUse = NOW();
        return;
    }
    if (flags & SNMD_DROP) {
        if (flags & SNMD_LOADED)
            unloadAll(this);
        return;
    }
    if (!(snmManager->flags & 1) && NOW() - lastUse > 15000 && (flags & SNMD_LOADED))
        unloadAll(this);
}

// v12 0x004B6B70
// Per buffer: its Update (streaming); spare duplicates idle 3 s are deleted; an auto-drop buffer that stopped is
// given back. The loops are re-sorted every 500 ms.
int snmSOUND_DATA::Update()
{
    for (int i = 0; i < 10; i++) {
        snmSLOT *s = &slot[i];
        if (!s->buf)
            continue;
        s->buf->Update();
        sndBUFFER *b = s->buf;
        if ((b->flags & SNDBF_CREATED) && !(b->flags & SNDBF_LOADED) && !(s->flags & SNMS_USED) &&
            NOW() - s->time > 3000) {
            if (b) {
                b->Term();
                if (s->buf)
                    s->buf->__vecDelDtor(1);
                s->buf = 0;
            }
            s->flags = 0;
            s->time = 0;
        }
        if (!s->buf || !(s->flags & SNMS_USED) || !(s->flags & SNMS_AUTODROP) || s->buf->IsPlay())
            continue;
        snmSLOT *o = slotOf(this, s->buf);
        if (!o)
            continue;
        o->time = NOW();
        o->flags &= ~SNMS_USED;
        StopBuffer(s->buf);
        Invalidate();
    }
    if (NOW() - lastPlay > 500)
        UpdateLoops();
    Invalidate();
    return 1;
}

// v12 0x004B6C90
// A free loaded buffer, else any free buffer, else a new one in an empty slot (not loaded yet).
sndBUFFER *snmSOUND_DATA::GetBuffer()
{
    snmSLOT *s = 0;
    for (int i = 0; i < 10; i++)
        if (slot[i].buf && (slot[i].buf->flags & SNDBF_LOADED)) {
            s = &slot[i];
            break;
        }
    if (s && !(s->flags & SNMS_USED)) {
        s->flags |= SNMS_USED;
        return s->buf;
    }
    for (int i = 0; i < 10; i++)
        if (slot[i].buf && !(slot[i].flags & SNMS_USED)) {
            slot[i].flags |= SNMS_USED;
            return slot[i].buf;
        }
    for (int i = 0; i < 10; i++) {
        if (slot[i].buf)
            continue;
        s = &slot[i];
        s->buf = 0;
        s->flags = 0;
        s->time = 0;
        s->buf = (flags & SNMD_STREAMED) ? sndSystem->makeStrBuffer() : sndSystem->makeBuffer();
        if (!s->buf)
            return 0;
        s->flags |= SNMS_USED;
        return s->buf;
    }
    return 0;
}

// v12 0x004B6D70
void snmSOUND_DATA::DropBuffer(sndBUFFER *b)
{
    if (!b)
        return;
    snmSLOT *s = slotOf(this, b);
    if (!s)
        return;
    s->time = NOW();
    s->flags &= ~SNMS_USED;
    StopBuffer(b);
    Invalidate();
}

// v12 0x004B6DD0
void snmSOUND_DATA::AutoDropBuffer(sndBUFFER *b)
{
    if (!b)
        return;
    snmSLOT *s = slotOf(this, b);
    if (!s || !(s->flags & SNMS_USED))
        return;
    if (s->buf->IsPlay() && (s->flags & SNMS_LOOP))
        s->buf->Stop();
    s->flags |= SNMS_AUTODROP;
}

// v12 0x004B6E30
void snmSOUND_DATA::PlayBuffer(sndBUFFER *b, int f)
{
    if (!b)
        return;
    snmSLOT *s = slotOf(this, b);
    if (!s)
        return;
    s->flags |= SNMS_PLAYING;
    if (f & 1) {
        s->flags |= SNMS_LOOP;
        UpdateLoops();
        return;
    }
    PlayOneShot(b);
}

// v12 0x004B6E90
int snmSOUND_DATA::IsBufferPlay(sndBUFFER *b)
{
    if (!b)
        return 0;
    snmSLOT *s = slotOf(this, b);
    if (!s)
        return 0;
    return s->flags & SNMS_PLAYING;
}

// v12 0x004B6EE0
// A loop is handed to UpdateLoops; a one-shot is cut only when more than 100 ms of it are left.
void snmSOUND_DATA::StopBuffer(sndBUFFER *b)
{
    if (!b)
        return;
    snmSLOT *s = slotOf(this, b);
    if (!s)
        return;
    int loop = s->flags & SNMS_LOOP;
    s->flags &= ~(SNMS_PLAYING | SNMS_LOOP);
    if (loop) {
        s->flags |= SNMS_LOOP_STOP;
        UpdateLoops();
        return;
    }
    if (!s->buf->IsPlay())
        return;
    sndBUFFER *p = s->buf;
    unsigned at = p->GetCurPlayTime(0);
    if (p->GetTotalTime() - at > 100)
        s->buf->Stop();
}

// v12 0x004B6F90
// Plays b once: a playing buffer just restarts; an idle created one plays; otherwise one of the sound's buffers is
// taken over (the one furthest along when 3 or more play, a stopped one else) or a loaded one is duplicated.
void snmSOUND_DATA::PlayOneShot(sndBUFFER *b)
{
    if (b->flags & SNDBF_3D) {
        if (NOW() - lastPlay < 100)
            return;
        float g = distRamp(listenerDist(b), b->minDist, b->maxDist);
        int voices = snmManager->nPlaying2 + snmManager->nPlaying;
        if (voices > 8 && g < 0.3f)
            return;
        if (voices > 14 && g < 0.6f)
            return;
    }
    snmSLOT *my = slotOf(this, b);
    if (!my)
        return;
    if ((b->flags & SNDBF_CREATED) && b->IsPlay())
        goto play;
    if (!(flags & SNMD_LOADED)) {
        Validate(my);
        lastUse = NOW();
        if (!(flags & SNMD_LOADED))
            return;
    }
    {
        int nPlay = 0;
        snmSLOT *along = 0, *idle = 0;
        for (int i = 0; i < 10; i++) {
            snmSLOT *s = &slot[i];
            if (!s->buf || !(s->buf->flags & SNDBF_CREATED))
                continue;
            if (!s->buf->IsPlay()) {
                idle = s;
                continue;
            }
            nPlay++;
            if (!along || along->buf->GetCurPlayTime(0) < s->buf->GetCurPlayTime(0))
                along = s;
        }
        if (nPlay >= 3) {
            if (!along)
                return;
            my->buf->TakeOver(along->buf);
        } else if (!(my->buf->flags & SNDBF_CREATED)) {
            if (idle) {
                my->buf->TakeOver(idle->buf);
            } else {
                snmSLOT *src = 0;
                for (int i = 0; i < 10; i++)
                    if (slot[i].buf && (slot[i].buf->flags & SNDBF_LOADED)) {
                        src = &slot[i];
                        break;
                    }
                if (!src)
                    return;
                if (!my->buf->Duplicate(src->buf))
                    return;
            }
        }
    }
play:
    my->buf->Play(4);
    lastPlay = NOW();
}

static int snmCompareSlots(const void *a, const void *b)     // v12 0x004B7EF0
{
    float ka = ((const snmSLOT *)a)->key, kb = ((const snmSLOT *)b)->key;
    if (ka < kb)
        return -1;
    if (ka > kb)
        return 1;
    return 0;
}

// v12 0x004B7370
// Keys: no buffer 6e6, not playing 2e6, a one-shot 4e6, a loop its distance ramps (0..2); the slots are sorted by
// it (the array itself is reordered). The first min(audible loops, 2) slots play (taking over / duplicating a
// buffer when theirs is empty), every later slot's buffer is stopped.
void snmSOUND_DATA::UpdateLoops()
{
    int anyLoop = 0;
    for (int i = 0; i < 10; i++)
        if ((slot[i].flags & SNMS_PLAYING) && (slot[i].flags & SNMS_LOOP))
            anyLoop = 1;
    int start = 0;
    if (anyLoop) {
        lastPlay = NOW();
        int audible = 0;
        for (int i = 0; i < 10; i++) {
            snmSLOT *s = &slot[i];
            if (!s->buf) {
                s->key = snmKeyStep * 3.0f;
                continue;
            }
            if (!(s->flags & SNMS_PLAYING)) {
                s->key = snmKeyStep;
                continue;
            }
            if (!(s->flags & SNMS_LOOP)) {
                s->key = snmKeyStep + snmKeyStep;
                continue;
            }
            float d = listenerDist(s->buf);
            float k = distRamp(d, s->buf->minDist, s->buf->maxDist) + distRamp(d, 0.0f, s->buf->minDist);
            s->key = k;
            int voices = snmManager->nPlaying2 + snmManager->nPlaying;
            if (voices > 8 && k < 0.3f)
                continue;
            if (voices > 14 && k < 0.6f)
                continue;
            audible++;
        }
        qsort(slot, 10, sizeof(snmSLOT), snmCompareSlots);
        if (audible > 0 && !(flags & SNMD_LOADED)) {
            Validate(&slot[0]);
            lastUse = NOW();
            if (!(flags & SNMD_LOADED))
                return;
        }
        int n = audible < 2 ? audible : 2;
        for (int i = 0; i < n; i++) {
            snmSLOT *s = &slot[i];
            if (s->buf->flags & SNDBF_CREATED) {
                if (!s->buf->IsPlay())
                    s->buf->Play(5);
                continue;
            }
            snmSLOT *from = 0;
            for (int j = n; j < 10; j++) {
                snmSLOT *o = &slot[j];
                if (!o->buf || !(o->buf->flags & SNDBF_CREATED))
                    continue;
                from = o;
                if (o->buf->IsPlay())
                    break;
            }
            if (from) {
                s->buf->TakeOver(from->buf);
            } else {
                snmSLOT *src = 0;
                for (int j = 0; j < 10; j++)
                    if (slot[j].buf && (slot[j].buf->flags & SNDBF_LOADED)) {
                        src = &slot[j];
                        break;
                    }
                if (!src)
                    return;
                if (!s->buf->Duplicate(src->buf))
                    return;
            }
            s->buf->Play(5);
        }
        start = n;
    }
    for (int i = start; i < 10; i++) {
        snmSLOT *s = &slot[i];
        if (!anyLoop) {
            if (s->flags & SNMS_LOOP_STOP)
                s->buf->Stop();
        } else if (s->buf && (s->buf->flags & SNDBF_CREATED) && s->buf->IsPlay()) {
            s->buf->Stop();
        }
        s->flags &= ~SNMS_LOOP_STOP;
    }
}

// v12 0x004B78F0
// Adds this sound's buffers to the counts (software / hardware, and playing ones of each).
void snmSOUND_DATA::GetStats(int *nSoft, int *nHard, int *playSoft, int *playHard)
{
    if (!(flags & SNMD_LOADED))
        return;
    int soft = 0, hard = 0, pSoft = 0, pHard = 0;
    for (int i = 0; i < 10; i++) {
        sndBUFFER *b = slot[i].buf;
        if (!b || !(b->flags & SNDBF_CREATED))
            continue;
        int sw = b->IsSoftware();
        if (sw)
            soft++;
        else
            hard++;
        if (!b->IsPlay())
            continue;
        if (sw)
            pSoft++;
        else
            pHard++;
    }
    *nSoft += soft;
    *nHard += hard;
    *playSoft += pSoft;
    *playHard += pHard;
}

// ---------------------------------------------------------------------------------------------------- the manager

// v12 0x004B79A0
int snmMANAGER::Init(char *d, char *e)
{
    strcpy(dir, d);
    strcpy(ext, e);
    data = (snmSOUND_DATA **)apCalloc(400, 4);
    if (!data)
        return 0;
    time = timeGetTime();
    return 1;
}

// v12 0x004B7A00
void snmMANAGER::Term()
{
    if (!data)
        return;
    for (int i = 0; i < 400; i++)
        if (data[i] && (data[i]->flags & SNMD_LOADED))
            unloadAll(data[i]);
    for (int i = 0; i < 400; i++) {
        snmSOUND_DATA *d = data[i];
        if (!d)
            continue;
        if (d->flags & SNMD_LOADED)
            unloadAll(d);
        for (int k = 9; k >= 0; k--) {
            snmSLOT *s = &d->slot[k];
            if (s->buf) {
                s->buf->Term();
                if (s->buf)
                    s->buf->__vecDelDtor(1);
                s->buf = 0;
            }
            s->flags = 0;
            s->time = 0;
        }
        operator delete(d);
    }
    apFree(data);
    data = 0;
}

// v12 0x004B7B80
// The entry of that name, else a new one (looked up on disk now; one that is not found is still entered).
snmSOUND_DATA *snmMANAGER::Add(char *nm, int f)
{
    if (!data)
        return 0;
    for (int i = 0; i < 400; i++)
        if (data[i] && data[i]->name.IsName(nm))
            return data[i];
    int i;
    for (;;) {
        for (i = next; i < 400 && data[i]; i++)
            ;
        if (i != 400)
            break;
        if (next <= 0)
            return 0;
        next = 0;
    }
    snmSOUND_DATA *d = (snmSOUND_DATA *)operator new(sizeof(snmSOUND_DATA));
    if (d) {
        d->flags = f;
        d->name.SetName(nm);
        for (int k = 0; k < 10; k++)
            d->slot[k].flags = 0;
        d->path[0] = 0;
        d->lengthMs = 0;
        d->lastUse = 0;
        d->lastPlay = 0;
        for (int k = 0; k < 10; k++) {
            d->slot[k].buf = 0;
            d->slot[k].flags = 0;
            d->slot[k].time = 0;
        }
    }
    data[i] = d;
    if (!d)
        return 0;
    d->Find(dir, ext);
    next = i + 1;
    return d;
}

// v12 0x004B7CD0
int snmMANAGER::Del(snmSOUND_DATA *d)
{
    if (!data)
        return 0;
    int i;
    for (i = 0; i < 400; i++)
        if (data[i] && data[i] == d)
            break;
    if (i == 400)
        return 0;
    data[i] = 0;
    if (d->flags & SNMD_LOADED)
        unloadAll(d);
    for (int k = 9; k >= 0; k--) {
        snmSLOT *s = &d->slot[k];
        if (s->buf) {
            s->buf->Term();
            if (s->buf)
                s->buf->__vecDelDtor(1);
            s->buf = 0;
        }
        s->flags = 0;
        s->time = 0;
    }
    operator delete(d);
    return 1;
}

// v12 0x004B7DB0
// Loads every sound not loaded yet (except SNMD_NO_LOAD ones) when flag 1 is set; the number that failed.
int snmMANAGER::LoadAll()
{
    if (!data || !(flags & 1))
        return 0;
    int failed = 0;
    for (int i = 0; i < 400; i++) {
        snmSOUND_DATA *d = data[i];
        if (!d || (d->flags & SNMD_LOADED) || (d->flags & SNMD_NO_LOAD))
            continue;
        if (!d->Validate(0))
            failed++;
    }
    return failed;
}

// v12 0x004B7E10
void snmMANAGER::BeginFrame()
{
    if (!data)
        return;
    time = timeGetTime();
    nPlaying2 = 0;
    nPlaying = 0;
    nHard = 0;
    nSoft = 0;
    for (int i = 0; i < 400; i++) {
        snmSOUND_DATA *d = data[i];
        if (!d || !(d->flags & SNMD_LOADED))
            continue;
        d->Update();
        data[i]->GetStats(&nSoft, &nHard, &nPlaying, &nPlaying);
    }
}

// v12 0x004B7E90
void snmMANAGER::SetMasterVolume(float dB)
{
    masterVolume = dB;
    if (!data)
        return;
    for (int i = 0; i < 400; i++) {
        snmSOUND_DATA *d = data[i];
        if (!d)
            continue;
        for (int k = 0; k < 10; k++)
            if (d->slot[k].buf && (d->slot[k].buf->flags & SNDBF_CREATED))
                d->slot[k].buf->ApplyVolume();
    }
}
