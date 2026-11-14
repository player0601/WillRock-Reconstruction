// Snd_chn.cpp -- the sound channels: sncSOUND_DESCR (one sound: file, volume, delay, repeat count / time, flags),
// sncCHANNEL (plays one description at a time through a buffer borrowed from the sample cache), sncCHANNEL_3D
// (with the listener), sncCHANNEL_CONE (a positioned source), sncCHANNEL_LIST (a group sharing mute groups and a
// volume) (Will Rock 1.2, v12.exe; feb obj drv:Snd_chn.obj)
//
// Rebuilt from the 1.2 code (v12 0x004B0980..0x004B1E7F). 1.2 changes, from the disassembly:
//   - 6 mute groups (muted 0x1000 << g, unmutable 0x100000 << g; feb 4 groups at 0x100000 / 0x1000000);
//   - starting and stopping the buffer goes through the cache (snmSOUND_DATA PlayBuffer / StopBuffer /
//     IsBufferPlay), the channel no longer validates the sound itself; the length is the cache's (+0x1E8);
//   - a random delay is delay/2 + rnd * delay/2, a random count rnd * count;
//   - the channel's ReadParams reads only "volume"; the destructor auto-drops the buffer when the channel or its
//     sound asks for it, else stops;
//   - a cone channel takes the sound's min / max distance when the sound has one.

#include <math.h>
#include <stdio.h>
#include "../../include/wr_sdk.h"

int sncCHANNEL::nChannels;              // v12 0x006561A8
m3dRND_GEN sncCHANNEL::rndGen;          // v12 0x006561B0
static const float sncEps = 1.0e-6f;    // v12 0x006065F8
float sncEpsSq;                         // v12 0x006561AC (static init v12 0x004B12C0)
static struct sncEPS_INIT { sncEPS_INIT() { sncEpsSq = sncEps * sncEps; } } sncEpsInit;

// ---------------------------------------------------------------------------------------------------- descriptions

// v12 0x004B0980
int sncSOUND_DESCR::LinkData(char *nm, int df)
{
    if (nm) {
        name.SetName(nm);
        dataFlags = df;
    }
    if (!snmManager) {
        data = 0;
        return 1;
    }
    data = snmManager->Add(name.str, dataFlags);
    if (!data)
        return 0;
    return 1;
}

// v12 0x004B09D0
void sncSOUND_DESCR::UnlinkData()
{
    if (data)
        snmManager->Del(data);
    name.SetName((char *)"");
    dataFlags = 0;
    data = 0;
}

// v12 0x004B0A10
int sncSOUND_DESCR::ReadParams(psSHEET *ps, char *sect, char *prefix)
{
    char key[0x40], nm[0x40];
    int b;
    sprintf(key, "%sname", prefix);
    if (!ps->GetStr(sect, key, nm, 0x40))
        return 0;
    sprintf(key, "%svolume", prefix);
    ps->GetFloat(sect, key, &volume);
    sprintf(key, "%sdelay", prefix);
    ps->GetInt(sect, key, &delay);
    sprintf(key, "%stime", prefix);
    ps->GetInt(sect, key, &time);
    sprintf(key, "%sisInfLoop", prefix);
    if (ps->GetBool(sect, key, &b) && b)
        flags |= SNCD_INF_LOOP;
    sprintf(key, "%sisRandDelay", prefix);
    if (ps->GetBool(sect, key, &b) && b)
        flags |= SNCD_RAND_DELAY;
    sprintf(key, "%sisSkipSame", prefix);
    if (ps->GetBool(sect, key, &b) && b)
        flags |= SNCD_SKIP_SAME;
    sprintf(key, "%sisStreaming", prefix);
    if (ps->GetBool(sect, key, &b) && b)
        dataFlags |= SNMD_STREAMED;
    sprintf(key, "%sisAutoLoad", prefix);
    if (ps->GetBool(sect, key, &b) && b)
        dataFlags |= SNMD_NO_LOAD | SNMD_DROP;
    u32 df = dataFlags;
    name.SetName(nm);
    dataFlags = df;
    if (!snmManager) {
        data = 0;
        return 1;
    }
    data = snmManager->Add(name.str, df);
    return data != 0;
}

// v12 0x004B0C50
int sncSOUND_DESCR_3D::ReadParams(psSHEET *ps, char *sect, char *prefix)
{
    char key[0x40];
    int b;
    if (!sncSOUND_DESCR::ReadParams(ps, sect, prefix))
        return 0;
    sprintf(key, "%sminDist", prefix);
    ps->GetFloat(sect, key, &minDist);
    sprintf(key, "%smaxDist", prefix);
    ps->GetFloat(sect, key, &maxDist);
    sprintf(key, "%sisDisable3D", prefix);
    if (ps->GetBool(sect, key, &b) && b)
        flags |= SNCD_NO_3D;
    return 1;
}

// v12 0x004B1DE0
// A static table of descriptions (the weapons' sounds): each one entered in the cache.
int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR *d)
{
    for (int i = 0; i < n; i++) {
        if (!snmManager) {
            d[i].data = 0;
            continue;
        }
        d[i].data = snmManager->Add(d[i].name.str, d[i].dataFlags);
        if (!d[i].data)
            return 0;
    }
    return 1;
}

// v12 0x004B1E30 (the same over 0x68-byte descriptions)
int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *d)
{
    for (int i = 0; i < n; i++) {
        if (!snmManager) {
            d[i].data = 0;
            continue;
        }
        d[i].data = snmManager->Add(d[i].name.str, d[i].dataFlags);
        if (!d[i].data)
            return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------------------------------- the channel

// v12 0x004B0D10
sncCHANNEL::sncCHANNEL()
{
    flags = 0;
    name.str[0] = 0;
    nDescr = 0;
    descr = 0;
    volume = 0.0f;
    listVolume = 0.0f;
    timeLeft = 0;
    played = 0;
    delay = 0;
    playPos = 0;
    buffer = 0;
    cur = 0;
    next = 0;
    nChannels++;
}

// v12 0x004B0D70
// The buffer goes back: auto-dropped when the channel or its sound wants it to finish, else the channel stops.
// The descriptions it owns are freed.
void sncCHANNEL::Destroy()
{
    if (sncCHANNEL::IsPlay((sncSOUND_DESCR *)0) && buffer) {
        if ((flags & SNCC_AUTODROP) || (cur->flags & SNCD_KEEP_BUF)) {
            cur->data->AutoDropBuffer(buffer);
        } else if (IsPlay((sncSOUND_DESCR *)0)) {
            ValidateBuffer(0);
            cur = 0;
            next = 0;
        }
    }
    for (int i = 0; i < nDescr; i++)
        operator delete(descr[i]);
    apFree(descr);
    descr = 0;
    cur = 0;
    next = 0;
    nChannels--;
}

void *sncCHANNEL::__vecDelDtor(unsigned f)
{
    Destroy();
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004B0D50
void *sncCHANNEL_3D::__vecDelDtor(unsigned f)
{
    Destroy();
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004B0E10
int sncCHANNEL::AddDescr(sncSOUND_DESCR *d)
{
    descr = (sncSOUND_DESCR **)apRealloc(descr, nDescr * 4 + 4);
    if (!descr)
        return -1;
    descr[nDescr] = d;
    nDescr++;
    return nDescr - 1;
}

// v12 0x004B0E60
// Group g mutes (unless the channel is unmutable by it) or unmutes; a change of the muted state updates at once.
void sncCHANNEL::Mute(int on, int g)
{
    u32 was = flags & SNCC_MUTED;
    if (on && !(flags & SNCC_UNMUTABLE(g)))
        flags |= SNCC_MUTE(g);
    else
        flags &= ~SNCC_MUTE(g);
    u32 now = flags & SNCC_MUTED;
    if ((was != 0) != (now != 0))
        Update(0, 0, 0);
}

// v12 0x004B0ED0
void sncCHANNEL::SetMuteGroup(int canMute, int g)
{
    if (canMute)
        flags &= ~SNCC_UNMUTABLE(g);
    else
        flags |= SNCC_UNMUTABLE(g);
}

// v12 0x004B0F00
void sncCHANNEL::ReadParams(psSHEET *ps, char *sect, char *prefix)
{
    char key[0x40];
    sprintf(key, "%svolume", prefix);
    ps->GetFloat(sect, key, &volume);
}

// v12 0x004B0F40
void sncCHANNEL::Play(int i, int n)
{
    if (!this)
        return;
    if (i == -1) {
        Play((sncSOUND_DESCR *)0, (sncSOUND_DESCR *)0);
        return;
    }
    if (!descr)
        return;
    if (n != -1)
        Play(descr[i], descr[n]);
    else
        Play(descr[i], (sncSOUND_DESCR *)0);
}

// v12 0x004B0FA0
// Starts d (0: stops), n to follow it. A busy channel can refuse it (NOT_OVER / ONLY_OVER / SKIP_SAME) or queue it
// after the sound playing (QUEUE_NEXT). The length is count x (sample + delay), or the description's time, or
// endless for an infinite loop.
void sncCHANNEL::Play(sncSOUND_DESCR *d, sncSOUND_DESCR *n)
{
    if (!this)
        return;
    if (d) {
        if (cur && (d->flags & SNCD_NOT_OVER))
            return;
        if (!cur && (d->flags & SNCD_ONLY_OVER))
            return;
        if ((d->flags & SNCD_SKIP_SAME) && cur && cur == d)
            return;
    }
    if (cur) {
        if ((cur->flags & SNCD_QUEUE_NEXT) && d && !(d->flags & SNCD_NO_QUEUE)) {
            next = d;
            played = playPos;
            timeLeft = cur->data->lengthMs;
            return;
        }
        Stop();
    }
    if (!d || !d->data) {
        Stop();
        return;
    }
    next = n;
    int count = d->count;
    if (d->flags & SNCD_RAND_COUNT)
        count = (int)(rndGen.RndFloat() * count);
    delay = d->delay;
    if (d->flags & SNCD_RAND_DELAY) {
        int dl = d->delay;
        int r = (int)(rndGen.RndFloat() * dl * -0.5f);
        delay = dl / 2 - r;
    }
    cur = d;
    if (d->time == 0) {
        timeLeft = d->data->lengthMs + delay;
        if (count)
            timeLeft *= count;
        else if (d->flags & SNCD_INF_LOOP)
            timeLeft = 0x7FFFFFFF;
    } else {
        timeLeft = d->time;
    }
    playPos = 0;
    played = 0;
    if ((d->flags & SNCD_INF_LOOP) || (u32)d->time > 0 || (d->delay == 0 && d->count != 0))
        d->flags |= SNCD_LOOPING;
    else
        d->flags &= ~SNCD_LOOPING;
    if (timeLeft > 0)
        Update(0, 0, 0);
}

// v12 0x004B1140
void sncCHANNEL::Stop()
{
    if (!IsPlay((sncSOUND_DESCR *)0))
        return;
    ValidateBuffer(0);
    cur = 0;
    next = 0;
}

// v12 0x004B1170
int sncCHANNEL::IsPlay(int i)
{
    if (i == -1)
        return IsPlay((sncSOUND_DESCR *)0);
    return IsPlay(descr[i]);
}

// v12 0x004B11A0
int sncCHANNEL::IsPlay(sncSOUND_DESCR *d)
{
    if (!cur)
        return 0;
    if (d)
        return cur == d;
    return 1;
}

// v12 0x004B11D0
// One step of dt ms: a muted channel gives its buffer back; at the end the next sound (if any) starts; otherwise
// the buffer is kept while the channel is heard (position, volume) and plays while the sound is audible.
void sncCHANNEL::Update(int dt, m3dV *, m3dV *)
{
    if (!cur)
        return;
    if (flags & SNCC_MUTED) {
        ValidateBuffer(0);
        return;
    }
    int audible;
    if (!AdvanceTime(dt, &audible)) {
        sncSOUND_DESCR *n = next;
        Stop();
        if (n)
            Play(n, (sncSOUND_DESCR *)0);
        return;
    }
    int hear = IsHear();
    if (ValidateBuffer(hear) && hear) {
        UpdatePos(0);
        if (buffer) {
            float vol, pan;
            CalcVolumePan(&vol, &pan);
            buffer->SetVolume(vol);
            buffer->SetPan(pan);
        }
    }
    ValidatePlay(audible && hear ? 1 : 0);
}

// v12 0x004B12E0
// want: borrow a buffer from the sound's cache entry (3D state, volume, position set at once); else give it back.
int sncCHANNEL::ValidateBuffer(int want)
{
    if (!want) {
        if (buffer) {
            if (cur->data)
                cur->data->DropBuffer(buffer);
            buffer = 0;
        }
        return buffer != 0;
    }
    if (buffer)
        return 1;
    buffer = cur->data->GetBuffer();
    if (!buffer)
        return 0;
    Update3D(1);
    if (buffer) {
        float vol, pan;
        CalcVolumePan(&vol, &pan);
        buffer->SetVolume(vol);
        buffer->SetPan(pan);
    }
    UpdatePos(1);
    return buffer != 0;
}

// v12 0x004B1390
// The buffer plays (looping or once, from the channel's position) while on and not muted, else stops.
void sncCHANNEL::ValidatePlay(int on)
{
    if (!buffer)
        return;
    int play = on && !(flags & SNCC_MUTED);
    int playing = cur->data->IsBufferPlay(buffer);
    buffer->SetCurPlayTime(playPos, 0);
    if (play) {
        if (!playing)
            cur->data->PlayBuffer(buffer, (cur->flags & SNCD_LOOPING) ? 1 : 4);
    } else if (playing) {
        cur->data->StopBuffer(buffer);
    }
}

// v12 0x004B1420
// 0 when the sound's time is over. Else the position in the repeat period (sample + delay), audible while in
// the sample part (the delay first with DELAY_FIRST).
int sncCHANNEL::AdvanceTime(int dt, int *audible)
{
    played += dt;
    if (timeLeft - played < 0) {
        *audible = 0;
        return 0;
    }
    int len = cur->data->lengthMs;
    int period = len + delay;
    if (!period)
        return 0;
    playPos = played % period;
    if (cur->flags & SNCD_DELAY_FIRST) {
        if (playPos < delay) {
            *audible = 0;
            playPos = 0;
            return 1;
        }
        *audible = 1;
        playPos -= delay;
        return 1;
    }
    if (playPos > (int)cur->data->lengthMs) {
        *audible = 0;
        playPos = cur->data->lengthMs;
        return 1;
    }
    *audible = 1;
    return 1;
}

// (base slots: v12 shared "return" functions)
void sncCHANNEL::SetParameters(m3dV *, m3dV *, m3dV *) {}
int  sncCHANNEL::IsHear() { return 1; }
void sncCHANNEL::UpdatePos(int) {}

// v12 0x004B1500
void sncCHANNEL::Update3D(int now)
{
    if (buffer)
        buffer->Enable3D(0, now);
}

// v12 0x004B1520
void sncCHANNEL::CalcVolumePan(float *vol, float *pan)
{
    *pan = 0.0f;
    *vol = cur->volume + listVolume + volume;
}

// ---------------------------------------------------------------------------------------------------- 3D, cone

// v12 0x004B1540
void sncCHANNEL_3D::Update(int dt, m3dV *l, m3dV *)
{
    if (l) {
        listener = *l;
        flags |= SNCC_LISTENER;
    }
    sncCHANNEL::Update(dt, 0, 0);
}

// v12 0x004B1580
void sncCHANNEL_3D::Update3D(int now)
{
    if (!buffer)
        return;
    if (!(flags & SNCC_NO_3D) && !(cur && (cur->flags & SNCD_NO_3D)))
        buffer->Enable3D(1, now);
    else
        buffer->Enable3D(0, now);
}

// v12 0x004B15D0
void sncCHANNEL_CONE::SetParameters(m3dV *p, m3dV *v, m3dV *d)
{
    pos = *p;
    vel = v ? *v : m3dVZero;
    dir = d ? *d : m3dVUnitZ;
    flags |= SNCC_SOURCE;
}

// v12 0x004B1680
void sncCHANNEL_CONE::ReadParams(psSHEET *ps, char *sect, char *prefix)
{
    char key[0x40];
    int b;
    sprintf(key, "%svolume", prefix);
    ps->GetFloat(sect, key, &volume);
    sprintf(key, "%sminDist", prefix);
    ps->GetFloat(sect, key, &minDist);
    sprintf(key, "%smaxDist", prefix);
    ps->GetFloat(sect, key, &maxDist);
    sprintf(key, "%sisDisable3D", prefix);
    if (ps->GetBool(sect, key, &b) && b)
        flags |= SNCC_NO_3D;
}

// v12 0x004B1760
// A fresh buffer gets the distances (the sound's when it has a max distance, else the channel's) and the cone.
int sncCHANNEL_CONE::ValidateBuffer(int want)
{
    int fresh = want && !buffer;
    if (!sncCHANNEL::ValidateBuffer(want))
        return 0;
    if (fresh) {
        sncSOUND_DESCR_3D *d = (sncSOUND_DESCR_3D *)cur;
        if (fabsf(d->maxDist) < sncEps)
            buffer->Set3DMinMaxDist(minDist, maxDist, 1);
        else
            buffer->Set3DMinMaxDist(d->minDist, d->maxDist, 1);
        buffer->SetConeParams(coneIn, coneOut, coneVol, 1);
    }
    return 1;
}

// v12 0x004B1830
// Heard when 2D, else when the listener and source are known and the source is within the max distance (the
// sound's, else the channel's); dist keeps the distance.
int sncCHANNEL_CONE::IsHear()
{
    if (flags & SNCC_NO_3D)
        return 1;
    if (cur && (cur->flags & SNCD_NO_3D))
        return 1;
    if (!(flags & SNCC_LISTENER) || !(flags & SNCC_SOURCE))
        return 0;
    float mx = 0.0f;
    if (cur)
        mx = ((sncSOUND_DESCR_3D *)cur)->maxDist;
    if (fabsf(mx) < sncEps)
        mx = maxDist;
    float dx = pos.x - listener.x, dy = pos.y - listener.y, dz = pos.z - listener.z;
    dist = m3dSqrt(dx * dx + dy * dy + dz * dz);
    return dist <= mx;
}

// v12 0x004B19B0
void sncCHANNEL_CONE::UpdatePos(int now)
{
    buffer->Set3DPosVel(&pos, &vel, now);
    buffer->SetConeOrientation(&dir, now);
}

// ---------------------------------------------------------------------------------------------------- the list

void *sncCHANNEL_LIST::__vecDelDtor(unsigned f)
{
    Destroy();
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x004B19F0
int sncCHANNEL_LIST::Init(int cnt)
{
    ch = (sncCHANNEL **)apCalloc(cnt, 4);
    if (!ch)
        return 0;
    n = cnt;
    return 1;
}

// v12 0x004B1A20
void sncCHANNEL_LIST::Destroy()
{
    apFree(ch);
    ch = 0;
    n = 0;
}

// v12 0x004B1A40
// c into the first free place; it takes the list's mute groups (unless unmutable by them) and volume.
int sncCHANNEL_LIST::Add(sncCHANNEL *c)
{
    int i;
    for (i = 0; i < n; i++)
        if (!ch[i])
            break;
    if (i == n)
        return -1;
    ch[i] = c;
    for (int g = 0; g < 6; g++) {
        u32 was = c->flags & SNCC_MUTED;
        if ((flags & SNCC_MUTE(g)) && !(c->flags & SNCC_UNMUTABLE(g)))
            c->flags |= SNCC_MUTE(g);
        else
            c->flags &= ~SNCC_MUTE(g);
        u32 now = c->flags & SNCC_MUTED;
        if ((was != 0) != (now != 0))
            c->Update(0, 0, 0);
    }
    c->listVolume = volume;
    return i;
}

// v12 0x004B1C60
int sncCHANNEL_LIST::Del(sncCHANNEL *c)
{
    for (int i = 0; i < n; i++)
        if (ch[i] == c) {
            ch[i] = 0;
            return i;
        }
    return -1;
}

// v12 0x004B1CA0
void sncCHANNEL_LIST::Update(u32 time, m3dV *l, m3dV *v)
{
    for (int i = 0; i < n; i++)
        if (ch[i])
            ch[i]->Update((int)(time - lastTime), l, v);
    lastTime = time;
}

// v12 0x004B1CF0
void sncCHANNEL_LIST::Mute(int on, int g)
{
    for (int i = 0; i < n; i++)
        if (ch[i])
            ch[i]->Mute(on, g);
    if (on)
        flags |= SNCC_MUTE(g);
    else
        flags &= ~SNCC_MUTE(g);
}

// v12 0x004B1DC0
int sncCHANNEL_LIST::GetNmb()
{
    int k = 0;
    for (int i = 0; i < n; i++)
        if (ch[i])
            k++;
    return k;
}
