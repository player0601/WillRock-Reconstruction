// test_sound.cpp -- (test) --sndtest: the rebuilt sound driver (drv Snd_sys.cpp / Snd_buf.cpp) and sample cache
// (Snd_mng.cpp) on their own. Makes the driver with sndSYSTEM::make and runs its Init, makes the cache on
// <game>\sounds with "wav", then through the cache: menu_accept played once (PlayOneShot) and menu_flame_loop
// (streamed) played as a loop (UpdateLoops), BeginFrame every frame, both given back (DropBuffer); prints states.
// PLACEHOLDER: the cache is made here with new + Init; the game does it in gsSND_SYSTEM::Init (not rebuilt yet).

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "../include/wr_sdk.h"
#include "test_assets.h"
#include "plat_d3d7.h"

int tstSoundTest(const char *gameDir)
{
    _sndSYSTEM *s = _sndSYSTEM::make();
    if (!s->Init()) {
        printf("sndtest: Init failed (state 2, the driver deleted itself)\n");
        return 2;
    }
    sndSystem = s;
    printf("sndtest: Init ok, state %d, flags %X, %d devices, primary %d Hz %d-bit %d ch, listener %p\n", s->state,
           s->flags, s->nDevices, s->primaryFmt.rate, s->primaryFmt.bits, s->primaryFmt.channels, s->pListener);
    for (int i = 0; i < s->nDevices; i++)
        printf("sndtest:   device %d: %s%s\n", i, s->devices[i].desc, s->devices[i].skip ? " (primary driver)" : "");

    snmManager = (snmMANAGER *)operator new(sizeof(snmMANAGER));
    memset(snmManager, 0, sizeof(snmMANAGER));
    char dir[MAX_PATH];
    _snprintf(dir, sizeof(dir) - 1, "%s\\sounds", gameDir);
    if (!snmManager->Init(dir, (char *)"wav")) {
        printf("sndtest: cache Init failed\n");
        return 3;
    }
    snmSOUND_DATA *a = snmManager->Add((char *)"menu_accept", 0);
    snmSOUND_DATA *f = snmManager->Add((char *)"menu_flame_loop", SNMD_STREAMED);
    snmSOUND_DATA *x = snmManager->Add((char *)"no_such_sound", 0);
    printf("sndtest: Add accept %p flags %X %u ms (%s), flame %p flags %X %u ms, missing %p flags %X\n", (void *)a,
           a->flags, a->lengthMs, a->path, (void *)f, f->flags, f->lengthMs, (void *)x, x ? x->flags : 0);
    sndBUFFER *ba = a->GetBuffer();
    sndBUFFER *bf = f->GetBuffer();
    a->PlayBuffer(ba, 0);
    f->PlayBuffer(bf, 1);
    printf("sndtest: after PlayBuffer: accept data %X buf %X playing %d; flame data %X buf %X playing %d, %d buffers\n",
           a->flags, ba->flags, ba->IsPlay(), f->flags, bf->flags, bf->IsPlay(), s->nBuffers);
    double t0 = platTime();
    int shown = 0;
    while (platTime() - t0 < 2.5) {
        platPump();
        snmManager->BeginFrame();
        if (platTime() - t0 > 0.5 * shown) {
            printf("sndtest: t=%.1f accept playing %d, flame playing %d (slot flags %X), voices %d soft %d hard %d\n",
                   platTime() - t0, ba->IsPlay(), bf->IsPlay(), f->slot[0].flags, snmManager->nPlaying,
                   snmManager->nSoft, snmManager->nHard);
            shown++;
        }
        Sleep(10);
    }
    // the channels (Snd_chn.cpp): a plain one and a cone one with a listener
    static sncSOUND_DESCR d;
    static sncSOUND_DESCR_3D d3;
    memset(&d, 0, sizeof(d));
    memset(&d3, 0, sizeof(d3));
    d.LinkData((char *)"menu_select", 0);
    d3.LinkData((char *)"menu_volume", 0);
    d3.minDist = 1.0f;
    d3.maxDist = 20.0f;
    sncCHANNEL *ch = new sncCHANNEL;
    sncCHANNEL_CONE *cc = new sncCHANNEL_CONE;
    m3dV lst = { 0, 0, 0 }, near5 = { 5, 0, 0 }, far100 = { 100, 0, 0 };
    cc->SetParameters(&near5, 0, 0);
    ch->Play(&d, 0);
    cc->Play((sncSOUND_DESCR *)&d3, 0);
    printf("sndtest: channels: plain cur %p buf %p, cone cur %p buf %p, %d channels\n", (void *)ch->cur,
           (void *)ch->buffer, (void *)cc->cur, (void *)cc->buffer, sncCHANNEL::nChannels);
    double c0 = platTime(), last = c0;
    int step = 0;
    while (platTime() - c0 < 1.5) {
        platPump();
        double now = platTime();
        int dt = (int)((now - last) * 1000.0);
        last = now;
        snmManager->BeginFrame();
        ch->Update(dt, 0, 0);
        cc->Update(dt, &lst, 0);
        if (step == 30)
            cc->SetParameters(&far100, 0, 0);
        if (step % 15 == 0)
            printf("sndtest: t=%.2f plain playing %d (buf %d, at %d of %d ms), cone heard %d dist %.1f buf %d playing %d\n",
                   now - c0, ch->IsPlay((sncSOUND_DESCR *)0), ch->buffer != 0, ch->playPos, ch->timeLeft,
                   cc->IsHear(), cc->dist, cc->buffer != 0, cc->buffer ? cc->buffer->IsPlay() : 0);
        step++;
        Sleep(10);
    }
    ch->__vecDelDtor(1);
    cc->__vecDelDtor(1);
    printf("sndtest: channels deleted, %d left\n", sncCHANNEL::nChannels);
    a->DropBuffer(ba);
    f->DropBuffer(bf);
    snmManager->BeginFrame();
    printf("sndtest: after DropBuffer: flame playing %d, accept slot %X, flame slot %X\n", bf->IsPlay(),
           a->slot[0].flags, f->slot[0].flags);
    snmManager->Term();
    printf("sndtest: cache Term, buffers alive %d\n", s->nBuffers);
    s->Term();
    printf("sndtest: done\n");
    return 0;
}
