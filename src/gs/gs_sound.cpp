// gs_sound.cpp -- gs.lib game sound system (Will Rock 1.2, v12.exe; feb obj gs:gs_sound.obj)
//
// gsSND_SYSTEM (gsSysSound): makes the sound driver and the sample cache (Init on [Sound] Sound / Path), the list of
// the channels entities own (gsCHANNEL_LIST, 400), 30 cone channels for positional one-shots (Play), the listener
// from the camera every frame (ProcessCamera), the volumes and mute switches from the ini (ApplySettings), and the
// channels read from an object's property sheet (CreateChannel: [SOUND_CHN] / [SOUND] sections, gsSOUND_DESCR).
// Also the sound and music domain lists it owns (gsDOMAIN_LIST_SND "&dom_snd", gsDOMAIN_LIST_MUS "&dom_mus"):
// their constructors and the music list's own functions (tracks from the scene sheet [MUSIC], SetMusicTempo).
// Not yet: the music list's MP sync.

#include <windows.h>
#include <mmsystem.h>
#include "../Sources/game.h"
#include "gs_mp_server.h"
#include <stdio.h>
#include <string.h>

static float gsSndEpsSq = 1e-6f * 1e-6f;  // v12 0x0098EA28 (static initialiser 0x005115F0; written only) // ?
static void *gsSndSwitchChnCfg;           // v12 0x0098EA30 [Sound] SwitchChn ini cache (ProcessCamera)

// v12 0x00510E10  (feb 0x004FDFC0)
// The 3D sound's keys, then [sect] <prefix>animSeq, actionFrame, nextSoundNmb and action (an IAction state name).
int gsSOUND_DESCR::ReadParams(psSHEET *ps, char *sect, char *prefix)
{
    char key[0x40], act[0x40];
    if (!sncSOUND_DESCR_3D::ReadParams(ps, sect, prefix))
        return 0;
    sprintf(key, "%sanimSeq", prefix);
    ps->GetStr(sect, key, animSeq, 0x40);
    sprintf(key, "%sactionFrame", prefix);
    ps->GetInt(sect, key, &actionFrame);
    sprintf(key, "%snextSoundNmb", prefix);
    ps->GetInt(sect, key, &nextSoundNmb);
    sprintf(key, "%saction", prefix);
    if (ps->GetStr(sect, key, act, 0x40))
        action = entIACTIVE_OBJ::ActionName2State(act);
    return 1;
}

// v12 0x00510F10  (feb 0x004FE0C0)
int gsCHANNEL_LIST::Init(int cnt)
{
    if (!sncCHANNEL_LIST::Init(cnt))
        return 0;
    ents = (entIACTIVE_OBJ **)apCalloc(cnt, 4);
    if (!ents)
        return 0;
    objs = (objOBJ **)apCalloc(cnt, 4);
    return objs != 0;
}

// v12 0x00510F60  (feb 0x004FE110)
void gsCHANNEL_LIST::Term()
{
    apFree(ents);
    apFree(objs);
}

// v12 0x00510F80  (feb 0x004FE130)
int gsCHANNEL_LIST::Del(sncCHANNEL *c)
{
    int i = sncCHANNEL_LIST::Del(c);
    if (i == -1)
        return -1;
    ents[i] = 0;
    objs[i] = 0;
    return i;
}

// v12 0x00511420
void *gsCHANNEL_LIST::__vecDelDtor(unsigned f)
{
    Destroy();
    if (f & 1)
        operator delete(this);
    return this;
}

// v12 0x00510FB0  (feb 0x004FE160)
static_assert(offsetof(gsSND_SYSTEM, sndDomains) == 0x28 && offsetof(gsSND_SYSTEM, enabled) == 0xEC, "gsSND_SYSTEM");

gsSND_SYSTEM::gsSND_SYSTEM()
{
    node.prev = 0;
    node.next = 0;
    flags = 0;
    magic = 0x4D414452;
    classId = 0;
    addrLock = 0;
    unk18 = 0;
    sign = 0;
    *(int *)((char *)this + 0x24) = 0;  // msgADDR +0x24 (inline msgADDR ctor)
    enabled = 0;
    entChannels = 0;
    priority = 0x57;
    listener.Identity();
}

// v12 0x00511030  (deleting 0x00511000)
gsSND_SYSTEM::~gsSND_SYSTEM()
{
}

// v12 0x00511040  (feb 0x004FE1F0)
// The two domain lists, then [Sound] Sound and Path, the channel list (400) with the 30 one-shot channels; with
// sound on: the driver (primary format, rolloff 0.5, music on Path) and the sample cache on Path with "wav". If the
// driver or the cache fails the game runs silent (enabled 0) and the system is not put in the message list.
int gsSND_SYSTEM::Init()
{
    char path[0x104];
    gsDOMAIN_LIST_SND *s = new gsDOMAIN_LIST_SND;
    sndDomains = s;
    if (!s)
        return 0;
    msgSystem.InsertAddr(s, 0x24534E44, 0x57);           // '$SND'
    gsDOMAIN_LIST_MUS *m = new gsDOMAIN_LIST_MUS;
    if (m) {
        m->soft._00 = 0;
        m->soft.name[0] = 0;
        m->soft.volumeDb = 0.0f;
        m->soft.pos[0] = m->soft.pos[1] = 0;
        m->hard._00 = 0;
        m->hard.name[0] = 0;
        m->hard.volumeDb = 0.0f;
        m->hard.pos[0] = m->hard.pos[1] = 0;
        m->fight._00 = 0;                                // (an out-of-line sndMUSIC ctor, v12 0x00513020)
        m->fight.name[0] = 0;
        m->fight.volumeDb = 0.0f;
        m->fight.pos[0] = m->fight.pos[1] = 0;
        m->tempo[0] = 0;
        m->cur = 0;
        m->curName.str[0] = 0;
        m->name.SetName((char *)"music");
    }
    musDomains = m;
    if (!m)
        return 0;
    msgSystem.InsertAddr(m, 0x24534E44, 0x57);
    if (!_apCfgReadBool(0, (char *)"Sound", (char *)"Sound", &enabled))
        return 0;
    if (!_apCfgReadString(0, (char *)"Sound", (char *)"Path", path, 0x104))
        return 0;
    gsCHANNEL_LIST *l = new gsCHANNEL_LIST;
    entChannels = l;
    if (!l)
        return 0;
    if (!l->Init(400))
        return 0;
    for (int i = 0; i < 30; i++) {
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        if (c) {
            c->minDist = 0.0f;
            c->maxDist = 0.0f;
        }
        pool[i] = c;
        if (!c)
            return 0;
        gsCHANNEL_LIST *e = entChannels;
        int k = e->Add(c);
        if (k != -1) {
            e->objs[k] = 0;
            e->ents[k] = 0;
        }
    }
    if (enabled) {
        sndSystem = _sndSYSTEM::make();
        if (!sndSystem || !sndSystem->Init())
            goto silent;
        sndSystem->SetFormat(0);
        sndSystem->SetAttenuationPow(0.5f);
        primaryVolume = sndSystem->GetPrimaryVolume();
        sndSystem->InitMusic(path);
        snmMANAGER *g = (snmMANAGER *)operator new(sizeof(snmMANAGER));
        if (!g)
            goto silent;
        g->flags = 0;
        g->next = 0;
        g->masterVolume = 0.0f;
        g->data = 0;
        g->time = 0;
        snmManager = g;
        if (!g->Init(path, (char *)"wav"))
            goto silent;
    }
    if ((gsAppState & 0x8000000) && enabled) {           // GSAPP_DEDICATED: no sound
        entChannels->Mute(1, 5);
        sndSystem->MuteMusic(1, 5);
    }
    ApplySettings();
    msgSystem.InsertAddr(this, 0x57, 0);
    return 1;
silent:
    sndSystem = 0;
    snmManager = 0;
    enabled = 0;
    return 1;
}

// v12 0x00511440  (feb 0x004FE5D0)
// (sndSystem and snmManager keep their pointers, original.)
void gsSND_SYSTEM::Term()
{
    for (int i = 0; i < 30; i++) {
        entChannels->Del(pool[i]);
        if (pool[i])
            pool[i]->__vecDelDtor(1);
    }
    entChannels->Term();
    if (entChannels)
        entChannels->__vecDelDtor(1);
    snmMANAGER *g = snmManager;
    if (g) {
        if (g->data)
            g->Term();
        operator delete(g);
    }
    if (sndSystem)
        sndSystem->__vecDelDtor(1);
    msgSystem.RemoveAddr(this);
    msgSystem.DestroyAddr(sndDomains);
    msgSystem.DestroyAddr(musDomains);
}

// v12 0x005114E0  (feb 0x004FE690)
// FRAME: the listener, the cache's frame, the music; the 3D settings are committed every other frame.
// INIT_LEVEL: the level's sounds loaded. CHG_MODE 0 / 1: everything muted / unmuted by group 1.
BOOL32 gsSND_SYSTEM::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    switch (id) {
    case GS_MSG_INIT_LEVEL:
        if (snmManager)
            snmManager->LoadAll();
        entChannels->lastTime = timeGetTime();
        return 0;
    case MSG_MSG_TERM:
        return 1;
    case GS_MSG_FRAME:
        ProcessCamera();
        if (snmManager)
            snmManager->BeginFrame();
        if (sndSystem) {
            sndSystem->UpdateMusic(gsElapsedTime);
            if (msgSystem.frameNmb % 2 == 0)
                sndSystem->CommitSettings();
        }
        return 0;
    case GS_MSG_CHG_MODE:
        if (*(int *)data == 0) {
            if (enabled) {
                entChannels->Mute(1, 1);
                sndSystem->MuteMusic(1, 1);
            }
        } else if (*(int *)data == 1) {
            if (enabled) {
                entChannels->Mute(0, 1);
                sndSystem->MuteMusic(0, 1);
            }
        }
        return 0;
    }
    return 0;
}

// v12 0x00511610  (feb 0x004FE790)
// The camera is the listener: its position, forward and (negated) up, swapped back with [Sound] SwitchChn; its
// speed from the last frame's matrix. The entity channels are updated first, at timeGetTime.
void gsSND_SYSTEM::ProcessCamera()
{
    m3dV pos, fwd, up, right, vel, old;
    int sw;
    gsCameraPtr->GetOrigin(&pos);
    gsCameraPtr->GetVForw(&fwd);
    gsCameraPtr->GetVUp(&up);
    up.x = -up.x;
    up.y = -up.y;
    up.z = -up.z;
    gsCameraPtr->GetVRight(&right);
    if (_apCfgReadBool(&gsSndSwitchChnCfg, (char *)"Sound", (char *)"SwitchChn", &sw) && sw) {
        up.x = -up.x;
        up.y = -up.y;
        up.z = -up.z;
    }
    if (gsElapsedTime > 1e-6f) {
        listener.GetOrigin(&old);
        float k = 1.0f / gsElapsedTime;
        vel.x = (pos.x - old.x) * k;
        vel.y = (pos.y - old.y) * k;
        vel.z = (pos.z - old.z) * k;
    } else
        vel = m3dVZero;
    entChannels->Update(timeGetTime(), &pos, &fwd);
    memcpy(&listener, gsCameraPtr, sizeof(m3dMATR));
    if (sndSystem)
        sndSystem->SetUpListener(&pos, &fwd, &up, &vel, 0);
}

// v12 0x005117A0  (feb 0x004FE920)
// [Sound] Music_Volume to the music, twice Volume to the cache's master volume, SFX / Music switches as mute
// group 0. (SFX_Volume and Voice_Volume are read but not used; missing keys leave the values unset, original.)
int gsSND_SYSTEM::ApplySettings()
{
    if (!enabled)
        return 1;
    float vol, music, sfx, voice;
    int on;
    _apCfgReadFloat(0, (char *)"Sound", (char *)"Volume", &vol);
    _apCfgReadFloat(0, (char *)"Sound", (char *)"Music_Volume", &music);
    _apCfgReadFloat(0, (char *)"Sound", (char *)"SFX_Volume", &sfx);
    _apCfgReadFloat(0, (char *)"Sound", (char *)"Voice_Volume", &voice);
    sndSystem->SetMusicVolume(music);
    snmManager->SetMasterVolume(vol + vol);
    on = 1;
    _apCfgReadBool(0, (char *)"Sound", (char *)"SFX", &on);
    entChannels->Mute(on == 0, 0);
    on = 1;
    _apCfgReadBool(0, (char *)"Sound", (char *)"Music", &on);
    sndSystem->MuteMusic(on == 0, 0);
    if (snmManager)
        snmManager->flags &= ~1u;
    return 1;
}

// v12 0x005118C0  (feb 0x004FEA10)
void gsSND_SYSTEM::Mute(int on, int group)
{
    if (!enabled)
        return;
    entChannels->Mute(on, group);
    sndSystem->MuteMusic(on, group);
    if (!on && (group == 3 || group == 2))
        ApplySettings();
}

// v12 0x00511910  (feb 0x004FEB30)
// c into the entity list, owned by e (positioned at object o, or at e's instance). (A full list gives index 0.)
int gsSND_SYSTEM::AddChannel(sncCHANNEL *c, entIACTIVE_OBJ *e, objOBJ *o)
{
    e->sndChannels = (int *)apRealloc(e->sndChannels, e->nSndChannels * 4 + 4);
    if (!e->sndChannels)
        return 0;
    gsCHANNEL_LIST *l = entChannels;
    int k = l->Add(c);
    if (k == -1)
        k = 0;
    else {
        l->objs[k] = o;
        l->ents[k] = e;
    }
    e->sndChannels[e->nSndChannels] = k;
    e->nSndChannels++;
    if (e->sign == 0x454E5424)                           // '$TNE' entENTITY
        UpdateEntPos((entENTITY *)e);
    return 1;
}

// v12 0x00511A10  (feb 0x004FEC30)
// Every channel the sheet describes (SOUND_CHN, SOUND_CHN_1, ...), each at its followObj under o if it names one.
int gsSND_SYSTEM::AddChannel(psSHEET *ps, entIACTIVE_OBJ *e, objOBJ *o)
{
    objOBJ *fo = 0;
    int n = 0;
    sncCHANNEL *c = CreateChannel(ps, 0, (char *)"SOUND_CHN", (char *)"", (char *)"SOUND", (char *)"", o, (sncCHANNEL **)&fo);
    while (c) {
        e->sndChannels = (int *)apRealloc(e->sndChannels, e->nSndChannels * 4 + 4);
        if (e->sndChannels) {
            gsCHANNEL_LIST *l = entChannels;
            int k = l->Add(c);
            if (k == -1)
                k = 0;
            else {
                l->objs[k] = fo;
                l->ents[k] = e;
            }
            e->sndChannels[e->nSndChannels] = k;
            e->nSndChannels++;
            if (e->sign == 0x454E5424)
                UpdateEntPos((entENTITY *)e);
        }
        n++;
        c = CreateChannel(ps, n, (char *)"SOUND_CHN", (char *)"", (char *)"SOUND", (char *)"", o, (sncCHANNEL **)&fo);
    }
    return 1;
}

// v12 0x005119A0  (feb 0x004FEBC0)
// Every channel e owns is taken out and deleted.
void gsSND_SYSTEM::DelChannel(entIACTIVE_OBJ *e)
{
    gsCHANNEL_LIST *l = entChannels;
    if (e)
        for (int i = 0; i < l->n; i++) {
            sncCHANNEL *c = l->ch[i];
            if (c && l->ents[i] == e) {
                l->Del(c);
                c->__vecDelDtor(1);
            }
        }
    apFree(e->sndChannels);
    e->sndChannels = 0;
    e->nSndChannels = 0;
}

// v12 0x00511A00
void gsSND_SYSTEM::DelChannel(sncCHANNEL *c)
{
    entChannels->Del(c);
}

// v12 0x00511B20  (feb 0x004FED40)
// Channel n of a sheet: section chnSect (n > 0: chnSect_n) read as a cone channel (<chnPrefix> keys, isRandNext 0x100,
// isRandPos 0x200, isNoMuteHidden 0x400), its followObj found under root into *follow, then every sound section
// sndSect, sndSect_1.. (or sndSect1..) whose channelNmb is n as a gsSOUND_DESCR (isStartPlay: played at once).
// The last argument is really an objOBJ ** (the follow object).
sncCHANNEL *gsSND_SYSTEM::CreateChannel(psSHEET *ps, int n, char *chnSect, char *chnPrefix, char *sndSect,
                                        char *sndPrefix, objOBJ *root, sncCHANNEL **followOut)
{
    char sect[0x40], key[0x40], fo[0x40];
    int b;
    objOBJ **follow = (objOBJ **)followOut;
    if (!ps)
        return 0;
    if (!n)
        sprintf(sect, "%s", chnSect);
    else
        sprintf(sect, "%s_%d", chnSect, n);
    if (!ps->FindSect(sect))
        return 0;
    sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
    if (!c)
        return 0;
    c->minDist = 0.0f;
    c->maxDist = 0.0f;
    c->ReadParams(ps, sect, chnPrefix);
    sprintf(key, "%sisRandNext", chnPrefix);
    if (ps->GetBool(sect, key, &b) && b)
        c->flags |= 0x100;
    sprintf(key, "%sisRandPos", chnPrefix);
    if (ps->GetBool(sect, key, &b) && b)
        c->flags |= 0x200;
    sprintf(key, "%sisNoMuteHidden", chnPrefix);
    if (ps->GetBool(sect, key, &b) && b)
        c->flags |= 0x400;
    *follow = 0;
    if (root && ps->GetStr(sect, (char *)"followObj", fo, 0x40))
        *follow = objFindName(root, fo);
    for (int i = 0;; i++) {
        if (!i)
            strcpy(sect, sndSect);
        else
            sprintf(sect, "%s_%d", sndSect, i);
        if (!ps->FindSect(sect)) {
            if (!i)
                strcpy(sect, sndSect);
            else
                sprintf(sect, "%s%d", sndSect, i);
            if (!ps->FindSect(sect))
                return c;
        }
        int nmb = 0;
        sprintf(key, "%schannelNmb", sndPrefix);
        ps->GetInt(sect, key, &nmb);
        if (nmb != n)
            continue;
        gsSOUND_DESCR *d = (gsSOUND_DESCR *)operator new(sizeof(gsSOUND_DESCR));
        if (!d)
            return 0;
        d->vtbl = 0;                                     // (gsSOUND_DESCR vtable v12 0x00607C20; called directly here)
        d->name.str[0] = 0;
        d->flags = 0;
        d->volume = 0.0f;
        d->delay = 0;
        d->count = 0;
        d->time = 0;
        d->data = 0;
        d->dataFlags = 0;
        d->minDist = 0.0f;
        d->maxDist = 0.0f;
        d->animSeq[0] = 0;
        d->actionFrame = -1;
        d->action = 0;
        d->nextSoundNmb = -1;
        d->dataFlags |= 6;
        if (!d->ReadParams(ps, sect, sndPrefix))
            return c;
        if (c->AddDescr(d) == -1)
            return 0;
        sprintf(key, "%sisStartPlay", sndPrefix);
        if (ps->GetBool(sect, key, &b) && b)
            c->Play((sncSOUND_DESCR *)d, (sncSOUND_DESCR *)0);
    }
}

// v12 0x00511F50  (feb 0x004FF170)
// e's channels (not muted by group 0) to its object's position and Z axis, or its instance's, with e's speed.
void gsSND_SYSTEM::UpdateEntPos(entENTITY *e)
{
    m3dV pos, dir, vel;
    for (int i = 0; i < e->nSndChannels; i++) {
        gsCHANNEL_LIST *l = entChannels;
        int k = e->sndChannels[i];
        sncCHANNEL *c = l->ch[k];
        if (!c || (c->flags & SNCC_MUTE(0)))
            continue;
        objOBJ *o = l->objs[k];
        animINST *inst = e->inst;
        if (o) {
            inst->Validate(4);
            o->GetOrigin(&pos);
            o->GetAxisZ(&dir);
        } else {
            inst->GetPos(&pos);
            inst->GetDir(&dir);
        }
        vel.x = e->velocity[0];
        vel.y = e->velocity[1];
        vel.z = e->velocity[2];
        c->SetParameters(&pos, &vel, &dir);
    }
}

// v12 0x00512040  (feb 0x004FF250)
// Anim-event sounds: a channel is muted (group 4) while e's instance is hidden unless isNoMuteHidden; a sound with an
// animSeq plays when that sequence runs (at its start, or when its actionFrame was crossed), the first one per
// channel; a playing channel stops while the instance is not animating (flag 0x2000).
void gsSND_SYSTEM::UpdateEntAnim(entENTITY *e)
{
    for (int i = 0; i < e->nSndChannels; i++) {
        sncCHANNEL *c = entChannels->ch[e->sndChannels[i]];
        if (!c)
            continue;
        animINST *inst = e->inst;
        if ((inst->flags & 3) && !(c->flags & 0x400))
            c->Mute(1, 4);
        else
            c->Mute(0, 4);
        int played = 0;
        for (int j = 0; j < c->nDescr; j++) {
            gsSOUND_DESCR *d = (gsSOUND_DESCR *)c->descr[j];
            if (!strlen(d->animSeq))
                continue;
            int seq = inst->tpl->FindSeqName(d->animSeq);
            if (seq == -1)
                continue;
            if (d->actionFrame == -1) {
                if (seq == inst->seq && (inst->flags & 0x8000)) {
                    c->Play(j, d->nextSoundNmb);
                    played = 1;
                }
            } else if (seq == inst->seq && (inst->actionFrames & (1u << d->actionFrame))) {
                c->Play(j, d->nextSoundNmb);
                played = 1;
            }
            if (c->IsPlay(j) && !(inst->flags & 0x2000))
                c->Play(-1, -1);
            if (played)
                break;
        }
    }
}

// v12 0x00512190  (feb 0x004FF3A0)
// Every sound of e's channels whose action has a bit of state plays; 1 if one did.
int gsSND_SYSTEM::UpdateIAction(entIACTIVE_OBJ *e, int state)
{
    int r = 0;
    for (int i = 0; i < e->nSndChannels; i++) {
        sncCHANNEL *c = entChannels->ch[e->sndChannels[i]];
        if (!c)
            continue;
        for (int j = 0; j < c->nDescr; j++)
            if (((gsSOUND_DESCR *)c->descr[j])->action & state) {
                c->Play(j, -1);
                r = 1;
            }
    }
    return r;
}

// v12 0x00512210  (feb 0x004FF420)
// A positional one-shot on the first free channel of the 30 (none free: dropped; endless loops never).
void gsSND_SYSTEM::Play(sncSOUND_DESCR_3D *d, m3dV *pos)
{
    if (d->flags & SNCD_INF_LOOP)
        return;
    for (int i = 0; i < 30; i++) {
        if (pool[i]->IsPlay(-1))
            continue;
        sncCHANNEL_CONE *c = (sncCHANNEL_CONE *)pool[i];
        c->minDist = d->minDist;
        c->maxDist = d->maxDist;
        pool[i]->SetParameters(pos, 0, 0);
        pool[i]->Play((sncSOUND_DESCR *)d, (sncSOUND_DESCR *)0);
        return;
    }
}

// v12 0x00512280  (feb 0x004FF490)
void gsSND_SYSTEM::ReleaseMusic()
{
    musDomains->_9c &= ~0x100;
}

// v12 0x005122A0  (feb 0x004FF4B0)
int gsSND_SYSTEM::GetNChn()
{
    return entChannels->GetNmb();
}

// ---------------------------------------------------------------------------------------------- domains

// v12 0x005122B0  (feb 0x004FF4C0)
// The domain's channels from its object's sheet (SOUND_CHN / SOUND sections, as an entity's: gsSND_SYSTEM::AddChannel
// inlined), each at its followObj; [DOM_SND] fadeTime (default 2 s).
BOOL32 gsDOMAIN_SND::Init()
{
    if (!gsDOMAIN::Init())
        return 0;
    psSHEET *ps = obj->props;
    gsSND_SYSTEM *snd = gsSysSound;
    objOBJ *fo = 0;
    int n = 0;
    sncCHANNEL *c = snd->CreateChannel(ps, 0, (char *)"SOUND_CHN", (char *)"", (char *)"SOUND", (char *)"", 0, (sncCHANNEL **)&fo);
    while (c) {
        sndChannels = (int *)apRealloc(sndChannels, nSndChannels * 4 + 4);
        if (sndChannels) {
            gsCHANNEL_LIST *l = snd->entChannels;
            int k = l->Add(c);
            if (k == -1)
                k = 0;
            else {
                l->objs[k] = fo;
                l->ents[k] = this;
            }
            sndChannels[nSndChannels] = k;
            nSndChannels++;
            if (sign == 0x454E5424)                       // '$TNE' entENTITY
                snd->UpdateEntPos((entENTITY *)this);
        }
        n++;
        c = snd->CreateChannel(ps, n, (char *)"SOUND_CHN", (char *)"", (char *)"SOUND", (char *)"", 0, (sncCHANNEL **)&fo);
    }
    fadeTime = 2.0f;
    if (obj->props)
        obj->props->GetFloat((char *)"DOM_SND", (char *)"fadeTime", &fadeTime);
    return 1;
}

// v12 0x00512400
// The domain's channels out of the list and deleted, then the base's.
void gsDOMAIN_SND::Term()
{
    gsCHANNEL_LIST *l = gsSysSound->entChannels;
    for (int i = 0; i < l->n; i++) {
        sncCHANNEL *c = l->ch[i];
        if (c && l->ents[i] == this) {
            l->Del(c);
            c->__vecDelDtor(1);
        }
    }
    apFree(sndChannels);
    sndChannels = 0;
    nSndChannels = 0;
    gsDOMAIN::Term();
}

// v12 0x00512480  (feb 0x004FF5E0)
// Faded out (-30 dB), then every channel plays its first sound (RandPos: at a random point of the volume).
void gsDOMAIN_SND::Enter()
{
    fade = 0.0f;
    UpdateFade(0.0f);
    for (int i = 0; i < nSndChannels; i++) {
        sncCHANNEL *c = gsSysSound->entChannels->ch[sndChannels[i]];
        if (!c)
            continue;
        if (c->flags & 0x200) {                          // RandPos
            m3dV p;
            GetRandomPoint(&p);
            c->SetParameters(&p, 0, 0);
        }
        c->Play(0, -1);
    }
}

// v12 0x00512500  (feb 0x004FF690)
// Inside: a RandNext channel that went quiet plays a random one of its sounds (RandPos: somewhere new). Then the fade.
void gsDOMAIN_SND::Update(m3dV *pos)
{
    for (int i = 0; i < nSndChannels; i++) {
        sncCHANNEL *c = gsSysSound->entChannels->ch[sndChannels[i]];
        if (!c || !(domFlags & DOMF_ACTIVE) || !(c->flags & 0x100) || c->IsPlay(-1))   // RandNext
            continue;
        if (c->flags & 0x200) {
            m3dV p;
            GetRandomPoint(&p);
            c->SetParameters(&p, 0, 0);
        }
        c->Play((int)m3dRandMax((float)c->nDescr - 1e-6f), -1);
    }
    UpdateFade(gsElapsedTime);
}

// v12 0x00401380 (a shared empty function)
void gsDOMAIN_SND::Leave()
{
}

// v12 0x005125C0  (name ours)
// fade grows by dt inside (up to fadeTime) and falls outside; the channels' volume goes linearly from -30 dB at 0 to
// 0 dB at fadeTime (an inlined clamped interpolation). Below 0 the sounds stop and fade restarts from 0.
void gsDOMAIN_SND::UpdateFade(float dt)
{
    if (domFlags & DOMF_ACTIVE)
        fade += dt;
    else
        fade -= dt;
    if (!(fade <= fadeTime))
        fade = fadeTime;
    float x = fade, x0 = 0.0f, y0 = -30.0f, x1 = fadeTime, y1 = 0.0f, v;
    if (x1 < 0.0f) {
        x0 = fadeTime;
        x1 = 0.0f;
        y0 = 0.0f;
        y1 = -30.0f;
    }
    if (x < x0)
        v = y0;
    else if (!(x <= x1))
        v = y1;
    else
        v = (y1 - y0) * (x - x0) / (x1 - x0) + y0;
    if (!(-1e-6f <= fade)) {
        fade = 0.0f;
        for (int i = 0; i < nSndChannels; i++)
            if (sncCHANNEL *c = gsSysSound->entChannels->ch[sndChannels[i]])
                c->Play(-1, -1);
    }
    for (int i = 0; i < nSndChannels; i++)
        if (sncCHANNEL *c = gsSysSound->entChannels->ch[sndChannels[i]])
            c->volume = v;
}

// v12 0x005127B0  (feb 0x004FF770)
// The tempo it sets: [MUSIC] curMusic / cur of its object's sheet.
BOOL32 gsDOMAIN_MUS::Init()
{
    if (!gsDOMAIN::Init())
        return 0;
    if (obj->props) {
        obj->props->GetStr((char *)"MUSIC", (char *)"curMusic", tempo, 0x40);
        obj->props->GetStr((char *)"MUSIC", (char *)"cur", tempo, 0x40);
    }
    return 1;
}

// v12 0x00512810
void gsDOMAIN_MUS::Enter()
{
    gsDOMAIN_LIST_MUS::SetMusicTempo((gsDOMAIN_LIST_MUS *)list, tempo);
}

// v12 0x00512830
// IAction "SET": the tempo in its string.
BOOL32 gsDOMAIN_MUS::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x3F8 && ((apNAME *)((char *)data + 8))->IsName((char *)"SET"))
        gsDOMAIN_LIST_MUS::SetMusicTempo((gsDOMAIN_LIST_MUS *)list, (char *)data + 0x60);
    return gsDOMAIN::ProcessMsg(id, data, from, ret);
}

// v12 0x00513040
gsDOMAIN *gsDOMAIN_LIST_SND::MakeDomain()
{
    return new gsDOMAIN_SND;
}

// v12 0x00513090
gsDOMAIN *gsDOMAIN_LIST_MUS::MakeDomain()
{
    return new gsDOMAIN_MUS;
}

// ---------------------------------------------------------------------------------------------- domain lists

// v12 0x00512790
int gsDOMAIN_LIST_SND::IsAddObj(objOBJ *o)
{
    return strstr(o->name, "&dom_snd") != 0;
}

// v12 0x00512880
int gsDOMAIN_LIST_MUS::IsAddObj(objOBJ *o)
{
    return strstr(o->name, "&dom_mus") != 0;
}

// v12 0x005128A0
void gsDOMAIN_LIST_MUS::ProcessINIT()
{
    soft._00 |= 1;
    hard._00 |= 1;
    fight._00 |= 1;
}

// v12 0x005128D0
void gsDOMAIN_LIST_MUS::ProcessTERM()
{
    if (sndSystem)
        sndSystem->StopMusic(cur, 1);
}

// v12 0x005128F0  (feb 0x004FF8B0)
// The three tracks and the start tempo from the scene sheet [MUSIC] (softMusic / soft, hardMusic / hard,
// fightMusic / fight, curMusic / cur), then that tempo.
int gsDOMAIN_LIST_MUS::ProcessINIT_LEVEL()
{
    ProcessINIT();
    if (!gsDOMAIN_LIST::ProcessINIT_LEVEL())
        return 0;
    if (psSHEET *ps = gsScenePtr->sheet) {
        ps->GetStr((char *)"MUSIC", (char *)"softMusic", soft.name, 0x40);
        gsScenePtr->sheet->GetStr((char *)"MUSIC", (char *)"hardMusic", hard.name, 0x40);
        gsScenePtr->sheet->GetStr((char *)"MUSIC", (char *)"fightMusic", fight.name, 0x40);
        gsScenePtr->sheet->GetStr((char *)"MUSIC", (char *)"curMusic", tempo, 0x40);
        gsScenePtr->sheet->GetStr((char *)"MUSIC", (char *)"soft", soft.name, 0x40);
        gsScenePtr->sheet->GetStr((char *)"MUSIC", (char *)"hard", hard.name, 0x40);
        gsScenePtr->sheet->GetStr((char *)"MUSIC", (char *)"fight", fight.name, 0x40);
        gsScenePtr->sheet->GetStr((char *)"MUSIC", (char *)"cur", tempo, 0x40);
        SetMusicTempo(this, tempo);
    }
    return 1;
}

// v12 0x00512A30
void gsDOMAIN_LIST_MUS::ProcessTERM_LEVEL()
{
    gsDOMAIN_LIST::ProcessTERM_LEVEL();
}

// v12 0x00512A40
void gsDOMAIN_LIST_MUS::Leave()
{
    SetMusicTempo(this, tempo);
}

// v12 0x00512A50
void gsDOMAIN_LIST_MUS::Update()
{
    gsDOMAIN_LIST::Update();
}

// v12 0x00512A60  (feb 0x004FFA20)
// INIT / TERM; 0x3EF: declared to the MP server; IAction (0x3F8) "SET": the tempo in its string; CHG_MODE to 1 or 5
// (and an IAction whose first word is 1 or 5) restart the track; MP notify 0x40E with sub-id -100: the track and its
// position from the server. Then the base list's handling.
BOOL32 gsDOMAIN_LIST_MUS::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id <= 0x3F8) {
        if (id == 0x3F8) {
            if (((apNAME *)((char *)data + 8))->IsName((char *)"SET"))
                SetMusicTempo(this, (char *)data + 0x60);
            goto restart;
        }
        if (id == MSG_MSG_INIT)
            ProcessINIT();
        else if (id == MSG_MSG_TERM)
            ProcessTERM();
        else if (id == 0x3EF)
            gsSysServer->DeclSyncable(this);
    } else if (id == GS_MSG_CHG_MODE) {
    restart:
        if ((*(int *)data == 1 || *(int *)data == 5) && sndSystem)
            sndSystem->SetMusic(cur, 0);
    } else if (id == 0x40E && *(short *)((char *)data + 4) == -100) {
        char *st = *(char **)((char *)data + 6);
        curName.SetName(st + 6);
        if (((apNAME *)soft.name)->IsName(curName.str))
            cur = &soft;
        else if (((apNAME *)hard.name)->IsName(curName.str))
            cur = &hard;
        else if (((apNAME *)fight.name)->IsName(curName.str))
            cur = &fight;
        if (cur) {
            cur->pos[0] = *(u32 *)(st + 0x46);
            cur->pos[1] = *(u32 *)(st + 0x4A);
            if (sndSystem) {
                sndSystem->SetMusic(0, 0);
                sndSystem->SetMusic(cur, 0);
            }
        }
    }
    return gsDOMAIN_LIST::ProcessMsg(id, data, from, ret);
}

// v12 0x00512BF0  (feb 0x004FFBB0)
// "soft" / "hard" / "fight" (any case) picks the track; a new one starts from its beginning after the old one fades
// (StopMusic), then plays. An unknown tempo with no track stops all three.
void __fastcall gsDOMAIN_LIST_MUS::SetMusicTempo(gsDOMAIN_LIST_MUS *l, char *t)
{
    if (!_strcmpi(t, "soft"))
        l->cur = &l->soft;
    else if (!_strcmpi(t, "hard"))
        l->cur = &l->hard;
    else if (!_strcmpi(t, "fight"))
        l->cur = &l->fight;
    int changed = !(l->cur && ((apNAME *)l->cur->name)->IsName(l->curName.str));
    if (!sndSystem)
        return;
    if (l->cur) {
        if (changed) {
            l->cur->pos[0] = 0;
            l->cur->pos[1] = 0;
            sndSystem->StopMusic(l->cur, 1);
        }
        l->curName.SetName(l->cur->name);
        sndSystem->SetMusic(l->cur, 1);
        return;
    }
    sndSystem->StopMusic(&l->soft, 1);
    sndSystem->StopMusic(&l->hard, 1);
    sndSystem->StopMusic(&l->fight, 1);
}
