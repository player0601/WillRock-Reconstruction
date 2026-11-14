// Snd_mus.cpp -- the music player of the sound driver: one mp3 track at a time through a DirectShow filter graph,
// fade-out, looping, mute groups (Will Rock 1.2, v12.exe; feb obj drv:Snd_mus.obj)
//
// Rebuilt from the 1.2 code (v12 0x004B58E0..0x004B6165). _sndSYSTEM fields at feb + 0x2C (before +0x14 at feb
// - 4). 1.2 changes, from the disassembly:
//   - six mute groups (flags 0x1000000 << g, g 0..5; feb 4) and per-track exceptions (sndMUSIC flags
//     0x100000 << g, SetMuteGroup): a track plays when every group muting the music lets it through;
//   - the state bits were renumbered: 0x20 fading, 0x40 track set, 0x80 forced, 0x100 graph running (feb 0x40 /
//     0x80 / 0x100 / "0x20 paused" inverted);
//   - the volume (dB) is clamped to IBasicAudio's -10000..0 hundredths; SetMusicVolume no longer adds the
//     track's own volume.

#include <windows.h>
#include <dshow.h>
#include "../../include/wr_sdk.h"

// v12 0x004B58E0
void sndMUSIC::SetMuteGroup(int canMute, int group)
{
    if (canMute)
        _00 &= ~(0x100000u << group);
    else
        _00 |= 0x100000u << group;
}

static inline long smVolume(float db)       // IBasicAudio::put_Volume units, clamped (v12 0x00606838 = -10000)
{
    float v = db * 100.0f;
    if (!(v >= -10000.0f))
        v = -10000.0f;
    else if (v > 0.0f)
        v = 0.0f;
    return (long)v;
}

// Is the music muted for track t: a group mutes it unless t is excepted from that group.
static inline int smMuted(_sndSYSTEM *s, sndMUSIC *t)
{
    if (!t)
        return (s->flags & 0x3F000000) != 0;
    for (int g = 0; g <= 5; g++)
        if ((s->flags & (0x1000000u << g)) && !(t->_00 & (0x100000u << g)))
            return 1;
    return 0;
}

// v12 0x004B5910  (feb 0x004AABA0)
// The filter graph and its control, seeking (in 100 ns units) and volume interfaces. folder: where the music is.
int _sndSYSTEM::InitMusic(char *folder)
{
    strcpy(musicDir, folder);
    if (FAILED(CoCreateInstance(CLSID_FilterGraph, 0, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, &pGraph)))
        return 0;
    IGraphBuilder *g = (IGraphBuilder *)pGraph;
    if (FAILED(g->QueryInterface(IID_IMediaControl, &pMediaControl)))
        return 0;
    if (FAILED(g->QueryInterface(IID_IMediaSeeking, &pMediaSeeking)))
        return 0;
    if (FAILED(g->QueryInterface(IID_IBasicAudio, &pBasicAudio)))
        return 0;
    if (FAILED(((IMediaSeeking *)pMediaSeeking)->SetTimeFormat(&TIME_FORMAT_MEDIA_TIME)))
        return 0;
    musicVolume = 0.0f;
    musicCurVolume = 0.0f;
    flags |= 0x10;
    return 1;
}

// v12 0x004B59F0  (feb 0x004AAC80)
void _sndSYSTEM::TermMusic()
{
    flags &= ~0x10u;
    if (pSource)
        ((IBaseFilter *)pSource)->Release();
    pSource = 0;
    if (pSourcePin)
        ((IPin *)pSourcePin)->Release();
    pSourcePin = 0;
    if (pMediaControl)
        ((IMediaControl *)pMediaControl)->Stop();
    if (pBasicAudio)
        ((IBasicAudio *)pBasicAudio)->Release();
    pBasicAudio = 0;
    if (pMediaSeeking)
        ((IMediaSeeking *)pMediaSeeking)->Release();
    pMediaSeeking = 0;
    if (pMediaControl)
        ((IMediaControl *)pMediaControl)->Release();
    pMediaControl = 0;
    if (pGraph)
        ((IGraphBuilder *)pGraph)->Release();
    pGraph = 0;
}

// v12 0x004B5AA0  (feb 0x004AAD30)
// Play track t (0: none). With `fade` and a track playing, the current one fades out first (UpdateMusic calls
// back after 2 s). The graph is emptied of the last file's filters, then built for "<musicDir>\<name>" and run
// unless the music is muted for t; t's position (+0x48) is where it starts (0 with flag 2).
int _sndSYSTEM::SetMusic(sndMUSIC *t, int fade)
{
    if (!(flags & 0x10))
        return 1;
    if (!(flags & 0x80) && curMusic == t)
        return 1;
    curMusic = t;
    if (fade && (flags & 0x40)) {
        musicFadeTime = 0.0f;
        flags |= 0x20;
        return 1;
    }
    flags &= ~0x20u;
    flags &= ~0x40u;
    IGraphBuilder *g = (IGraphBuilder *)pGraph;
    if (pSource) {
        if (flags & 0x100) {
            if (FAILED(((IMediaControl *)pMediaControl)->Stop()))
                return 0;
            flags &= ~0x100u;
        }
        if (pSource) {
            IEnumFilters *en = 0;
            int n = 0;
            if (SUCCEEDED(g->EnumFilters(&en))) {
                while (en->Skip(1) == S_OK)
                    n++;
                IBaseFilter **all = (IBaseFilter **)apMalloc(n * 4);
                en->Reset();
                for (IBaseFilter **p = all; en->Next(1, p, 0) == S_OK; p++)
                    ;
                if (en)
                    en->Release();
                en = 0;
                for (int i = 0; i < n; i++) {   // every filter out, all but the old source back in
                    g->RemoveFilter(all[i]);
                    if (all[i] != (IBaseFilter *)pSource)
                        g->AddFilter(all[i], 0);
                    if (all[i])
                        all[i]->Release();
                    all[i] = 0;
                }
                apFree(all);
            }
            if (pSource)
                ((IBaseFilter *)pSource)->Release();
            pSource = 0;
            if (pSourcePin)
                ((IPin *)pSourcePin)->Release();
            pSourcePin = 0;
        }
    }
    if (!curMusic)
        return 1;
    char *path = fnmFindDisc(musicDir, curMusic->name, 0);
    if (!path) {
        apLog((char *)"music %s not found\n", curMusic->name);
        return 0;
    }
    WCHAR wpath[MAX_PATH];
    MultiByteToWideChar(0, 0, path, -1, wpath, MAX_PATH);
    if (FAILED(g->AddSourceFilter(wpath, wpath, (IBaseFilter **)&pSource)))
        return 0;
    if (FAILED(((IBaseFilter *)pSource)->FindPin(L"Output", (IPin **)&pSourcePin)))
        return 0;
    if (FAILED(g->Render((IPin *)pSourcePin)))
        return 0;
    if (curMusic->_00 & 2) {
        curMusic->pos[0] = 0;
        curMusic->pos[1] = 0;
    }
    LONGLONG *pos = (LONGLONG *)curMusic->pos;
    if (FAILED(((IMediaSeeking *)pMediaSeeking)->SetPositions(pos, AM_SEEKING_AbsolutePositioning, pos,
                                                               AM_SEEKING_NoPositioning)))
        return 0;
    flags |= 0x40;
    if (!smMuted(this, curMusic)) {
        if (FAILED(((IMediaControl *)pMediaControl)->Run()))
            return 0;
        flags |= 0x100;
    }
    musicCurVolume = curMusic->volumeDb + musicVolume;
    ((IBasicAudio *)pBasicAudio)->put_Volume(smVolume(musicCurVolume));
    return 1;
}

// v12 0x004B5E40  (feb 0x004AB090)
// Stop t (or whatever plays, t 0), fading out with `fade`.
void _sndSYSTEM::StopMusic(sndMUSIC *t, int fade)
{
    if (curMusic == t || !t)
        SetMusic(0, fade);
}

// v12 0x004B5E60  (feb 0x004AB0B0)
// Mute (on) or unmute the music for group g; the graph is stopped / run as the result needs.
void _sndSYSTEM::MuteMusic(int on, int g)
{
    if (!(flags & 0x10))
        return;
    if (on)
        flags |= 0x1000000u << g;
    else
        flags &= ~(0x1000000u << g);
    if (smMuted(this, curMusic)) {
        if (pSource && (flags & 0x100) && SUCCEEDED(((IMediaControl *)pMediaControl)->Stop()))
            flags &= ~0x100u;
    } else if (pSource && SUCCEEDED(((IMediaControl *)pMediaControl)->Run()))
        flags |= 0x100;
}

// v12 0x004B5F20  (feb 0x004AB170)
void _sndSYSTEM::SetMusicVolume(float db)
{
    musicVolume = db;
    if (!(flags & 0x10))
        return;
    musicCurVolume = db;
    ((IBasicAudio *)pBasicAudio)->put_Volume(smVolume(db));
}

// v12 0x004B5F90  (feb 0x004AB1E0)
// The fade-out (2 s down to -100 dB, then the next track), the loop at the end of a looping track, the position
// kept in the track.
void _sndSYSTEM::UpdateMusic(float dt)
{
    if (!(flags & 0x10))
        return;
    if (flags & 0x20) {
        float t = dt + musicFadeTime;
        musicFadeTime = t;
        if (t > 2.0f) {
            flags |= 0x80;
            SetMusic(curMusic, 0);
            flags &= ~0x80u;
            return;
        }
        float v = musicVolume;
        musicCurVolume = v;
        if (curMusic)
            musicCurVolume = v = v + curMusic->volumeDb;
        if (!(t < 0.0f)) {
            if (t <= 2.0f)
                v = (-100.0f - v) * t * 0.5f + v;
            else
                v = -100.0f;
        }
        musicCurVolume = v;
        ((IBasicAudio *)pBasicAudio)->put_Volume(smVolume(v));
    }
    if (curMusic)
        curMusic->_00 &= ~4u;
    if (!pSource || !curMusic || (flags & 0x3F000000))
        return;
    LONGLONG cur, stop;
    if (FAILED(((IMediaSeeking *)pMediaSeeking)->GetPositions(&cur, &stop)))
        return;
    // COMPAT (not in 1.2): current Windows MP3 decoders end the stream a little before the reported stop time, so
    // the position test below never fires and looping tracks fell silent; the graph's EC_COMPLETE counts as the end
    int ended = 0;
    if (curMusic->_00 & 1) {
        IMediaEvent *ev = 0;
        if (SUCCEEDED(((IUnknown *)pMediaSeeking)->QueryInterface(IID_IMediaEvent, (void **)&ev)) && ev) {
            long code;
            LONG_PTR p1, p2;
            while (ev->GetEvent(&code, &p1, &p2, 0) == S_OK) {
                if (code == EC_COMPLETE)
                    ended = 1;
                ev->FreeEventParams(code, p1, p2);
            }
            ev->Release();
        }
    }
    if ((curMusic->_00 & 1) && (cur >= stop || ended)) {
        cur = 0;
        if (FAILED(((IMediaSeeking *)pMediaSeeking)->SetPositions(&cur, AM_SEEKING_AbsolutePositioning, &cur,
                                                                   AM_SEEKING_NoPositioning)))
            return;
    }
    if (!(flags & 0x20))
        *(LONGLONG *)curMusic->pos = cur;
}
