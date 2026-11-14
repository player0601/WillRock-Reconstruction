// sound.h -- Will Rock 1.2 unofficial SDK, game_systems: gsSND_SYSTEM (gs:gs_sound), music domains
// Assumes from re/sdk/core: msgADDR, sncCHANNEL / sncCHANNEL_3D (size 0xBC) / sncSOUND_DESCR_3D (engine sound lib, the
//   only part that talks to DirectSound), psSHEET, m3dV, m3dMATR, entENTITY, entIACTIVE_OBJ, objOBJ.
// gsSND_SYSTEM layout up to +0xEC is identical in both builds (ctor v12 0x00510FB0 == feb 0x004FE160).
#pragma once
#include "gs_common.h"
#include "domains.h"

// A channel's sound with the anim-event and IAction keys (CreateChannel). sncSOUND_DESCR_3D + 0x4C, size 0xB4.
class gsSOUND_DESCR : public sncSOUND_DESCR_3D { // vtbl v12 0x00607C20 (feb 0x005DC774)
public:
    int  ReadParams(psSHEET *s, char *sect, char *prefix); // slot +0 v12 0x00510E10 (feb 0x004FDFC0)
    char animSeq[0x40];            // +0x68 [sect] %sanimSeq: played when the entity's anim runs this sequence
    int  actionFrame;              // +0xA8 %sactionFrame (-1: at the sequence start)
    int  action;                   // +0xAC %saction: IAction state bits (UpdateIAction)
    int  nextSoundNmb;             // +0xB0 %snextSoundNmb (-1)
};

// The channels entities own (size 0x20, vtbl v12 0x00607B10, feb 0x005DC664): an sncCHANNEL_LIST with the owner
// entity and object of each channel.
class gsCHANNEL_LIST : public sncCHANNEL_LIST {
public:
    void *__vecDelDtor(unsigned f);            // [0] v12 0x00511420
    int   Init(int n);                         // [1] v12 0x00510F10 (feb 0x004FE0C0)
    int   Del(sncCHANNEL *c);                  // [3] v12 0x00510F80 (feb 0x004FE130)
    virtual void Term();                       // [5] v12 0x00510F60 (feb 0x004FE110)
    entIACTIVE_OBJ **ents;                     // +0x18
    objOBJ         **objs;                     // +0x1C
};

class gsSND_SYSTEM : public msgADDR {          // object is wrSND_SYSTEM (vtbl v12 0x00606144). size v12 0xF4 (feb 0xF8)  // ? tail
public:  // tag +0x10 'RDAM', +0x1C = 0x57 priority (verified v12 0x00510FE6)
    int            addrLock;               // +0x024 msgADDR's lock count (msgSYSTEM skips a locked address; the C++ msgADDR
                                           //   ends at 0x24, each class declares it; zeroed by the ctor)
    class gsDOMAIN_LIST_SND *sndDomains;      // +0x028 v12 (feb +0x028) new 0x3C8 (feb 0x3C4), msg addr '$SND' prio 0x57  verified v12 0x00511076
    class gsDOMAIN_LIST_MUS *musDomains;      // +0x02C v12 (feb +0x02C) new 0x53C (feb 0x538)                         verified v12 0x00511126
    gsCHANNEL_LIST *entChannels;              // +0x030 v12 (feb +0x030) new 0x20
    sncCHANNEL_3D  *pool[30];                 // +0x034 v12 (feb +0x034) 30 one-shot cone channels (Play; cmp 0x1E at v12 0x005112E0)
    m3dMATR         listener;                 // +0x0AC v12 (feb +0x0AC) camera matrix (ProcessCamera)
    int             enabled;                  // +0x0EC v12 (feb +0x0EC) INI [Sound] Sound (bool)
    float           primaryVolume;            // +0x0F0 v12 (feb +0x0F4) the primary buffer's volume at Init

    virtual BOOL32 ProcessMsg(int,void*,msgADDR*,int*); // slot +0x04 v12 0x005114E0 (feb 0x004FE690) FRAME -> ProcessCamera, CHG_MODE...
    gsSND_SYSTEM();                                   // v12 0x00510FB0 (feb 0x004FE160)
    ~gsSND_SYSTEM();                                  // v12 0x00511030 (deleting 0x00511000)
    int  Init();                                      // v12 0x00511040 (feb 0x004FE1F0) INI [Sound] Sound, Path; ext "wav"
    void Term();                                      // v12 0x00511440 (feb 0x004FE5D0)
    void ProcessCamera();                             // v12 0x00511610 (feb 0x004FE790) listener = gsCameraPtr
    int  ApplySettings();                             // v12 0x005117A0 (feb 0x004FE920) volumes from options
    void Mute(int on, int fade);                      // v12 0x005118C0 (feb 0x004FEA10)
    int  AddChannel(sncCHANNEL *c, entIACTIVE_OBJ *e, objOBJ *o);  // v12 0x00511910 (feb 0x004FEB30)
    int  AddChannel(psSHEET *s, entIACTIVE_OBJ *e, objOBJ *o);     // v12 0x00511A10 (feb 0x004FEC30)
    void DelChannel(entIACTIVE_OBJ *e);               // v12 0x005119A0 (feb 0x004FEBC0)
    void DelChannel(sncCHANNEL *c);                   // v12 0x00511A00 (feb 0x004FEC20)
    sncCHANNEL *CreateChannel(psSHEET*, int, char*, char*, char*, char*, objOBJ*, sncCHANNEL**); // v12 0x00511B20 (feb 0x004FED40)
    void UpdateEntPos(entENTITY *e);                  // v12 0x00511F50 (feb 0x004FF170)
    void UpdateEntAnim(entENTITY *e);                 // v12 0x00512040 (feb 0x004FF250) anim-event sounds (called from render EndFrame)
    int  UpdateIAction(entIACTIVE_OBJ *e, int);       // v12 0x00512190 (feb 0x004FF3A0)
    void Play(sncSOUND_DESCR_3D *d, m3dV *pos);       // v12 0x00512210 (feb 0x004FF420)  HOOK: every positional one-shot
    void ReleaseMusic();                              // v12 0x00512280 (feb 0x004FF490)
    int  GetNChn();                                   // v12 0x005122A0 (feb 0x004FF4B0)
};

class gsDOMAIN_LIST_SND : public gsDOMAIN_LIST {     // size v12 0x3C8. vtbl v12 0x00607BA8 (feb 0x005DC700); the base's
public:                                              // code but for these two. '$SND' prio 0x57 (gsSND_SYSTEM::Init)
    int       IsAddObj(objOBJ *o);                   // v12 0x00512790 "&dom_snd"
    gsDOMAIN *MakeDomain();                          // v12 0x00513040 new gsDOMAIN_SND (0x138)
};
enum gsMUSIC_TEMPO { MUS_SOFT = 0, MUS_HARD = 1, MUS_FIGHT = 2 };  // strings "soft" "hard" "fight" (SetMusicTempo)  // ? numeric values
// The level's three music tracks; a "&dom_mus" domain or the IAction "SET" picks one (SetMusicTempo).
class gsDOMAIN_LIST_MUS : public gsDOMAIN_LIST {     // size v12 0x53C (feb 0x538). vtbl v12 0x00607B28 (feb 0x005DC680), 32 slots
public:  // entIACTIVE_OBJ::name (+0x40) = "music"
    sndMUSIC  soft;                // v12 +0x3C8 (feb +0x3C4) scene sheet [MUSIC] softMusic / soft   verified v12 0x0051291C
    sndMUSIC  hard;                // v12 +0x418 (feb +0x414) [MUSIC] hardMusic / hard               verified v12 0x0051293A
    sndMUSIC  fight;               // v12 +0x468 (feb +0x464) [MUSIC] fightMusic / fight             verified v12 0x00512958
    char      tempo[0x40];         // v12 +0x4B8 (feb +0x4B4) [MUSIC] curMusic / cur: the start tempo verified v12 0x00512981
    sndMUSIC *cur;                 // v12 +0x4F8 (feb +0x4F4) the track playing
    apNAME    curName;             // v12 +0x4FC (feb +0x4F8) its file name
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x00512A60 (feb 0x004FFA20)
    void   Update();                                 // [18] v12 0x00512A50 the base's
    void   Leave();                                  // [20] v12 0x00512A40 back to the start tempo
    int    ProcessINIT_LEVEL();                      // [28] v12 0x005128F0 (feb 0x004FF8B0) the tracks from the scene sheet
    void   ProcessTERM_LEVEL();                      // [29] v12 0x00512A30 the base's
    virtual void ProcessINIT();                      // [30] v12 0x005128A0 the three tracks loop
    virtual void ProcessTERM();                      // [31] v12 0x005128D0 the music stops (fading)
    int       IsAddObj(objOBJ *o);                   // v12 0x00512880 "&dom_mus"
    gsDOMAIN *MakeDomain();                          // v12 0x00513090 new gsDOMAIN_MUS (0x170)
    // MP sync (not rebuilt): GET v12 0x00512D30, WRITE v12 0x00512E00, READ v12 0x00512EC0 (music state is saved and replicated)
    static void __fastcall SetMusicTempo(gsDOMAIN_LIST_MUS *l, char *tempo); // v12 0x00512BF0 (feb 0x004FFBB0) HOOK: music changes
};
