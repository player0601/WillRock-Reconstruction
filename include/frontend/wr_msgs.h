// wr_msgs.h -- Will's voice lines, the level's message and secret-area domains (Will Rock 1.2; feb obj wr_msgs.obj)
//
// v12 0x004AC5D0..0x004AD050. wrMsgInit makes two domain lists: "msg" (wrMSG_SYSTEM, the "&dom_msg" objects:
// entering one says its line) and "secret" (wrSECRET_SYSTEM, "&dom_secret": a secret area found), and the voice
// channel both speak on. A line is a sound of the same name (sounds\<language>\prom_liners: "LIN_P_*", "LIN_W_*")
// and, with subtitles on or no sound for it, the string of the same name on the HUD.
#pragma once
#include "../game_systems/domains.h"

#pragma pack(push, 1)

class sncCHANNEL; struct sncSOUND_DESCR;

class wrMSG_SYSTEM : public gsDOMAIN_LIST {     // "msg", size 0x3C8, vtbl v12 0x006063F8 (feb 0x005DAED0)
public:
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x004AC800 (feb 0x0049E410)
    int  IsAddObj(objOBJ *o);                     // [23] v12 0x004AC7E0 (feb 0x0049E3F0) "&dom_msg"
    gsDOMAIN *MakeDomain();                       // [24] v12 0x004ACFD0 (feb 0x0049EB80) new wrDOMAIN_MSG
    static void __fastcall ShowMsg(char *id, float time);   // v12 0x004AC730 (feb 0x0049E340) the line said and shown
};

class wrSECRET_SYSTEM : public gsDOMAIN_LIST {  // "secret", size 0x3C8, vtbl v12 0x00606380 (feb 0x005DAE58)
public:
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x004ACB70 (feb 0x0049E740)
    int  IsAddObj(objOBJ *o);                     // [23] v12 0x004ACB50 (feb 0x0049E720) "&dom_secret"
    gsDOMAIN *MakeDomain();                       // [24] v12 0x004AD010 (feb 0x0049EBC0) new wrDOMAIN_SECRET
};

class wrDOMAIN_MSG : public gsDOMAIN {          // "&dom_msg": [DOM_MSG] msg (the line), isShowOnce. size 0x130
public:                                         // vtbls v12 0x006064A0 / 0x00606470 (feb 0x005DAF78 / 0x005DAF44)
    BOOL32 Init();                                // scnDOMAIN slot 1 v12 0x004AC9E0 (feb 0x0049E5B0)
    void   Enter();                               // [17] v12 0x004ACA30 (feb 0x0049E600)
};

class wrDOMAIN_SECRET : public gsDOMAIN {       // "&dom_secret": found once by each player. size 0x174
public:                                         // vtbls v12 0x00606520 / 0x006064EC (feb 0x005DAFF0 / 0x005DAFC0)
    int           nFound;                         // v12 +0x130 the players that found it (at most 16, the last replaced)
    unsigned long found[16];                      // v12 +0x134 their MP ids
    BOOL32 Init();                                // scnDOMAIN slot 1 v12 0x004ACC70 (feb 0x0049E840)
    void   Update(m3dV *pos);                     // [16] v12 0x004ACCA0 (feb 0x0049E870)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret); // [1] v12 0x004ACD90 (feb 0x0049E960)
};

#pragma pack(pop)

extern wrSECRET_SYSTEM *wrSecretSystem;   // v12 0x0064FFBC "secret"
extern wrMSG_SYSTEM    *wrMsgSystem;      // v12 0x0064FFC0 "msg"
extern sncCHANNEL      *wrMsgChannel;     // v12 0x0064FFC4 the voice
extern sncSOUND_DESCR   wrMsgDescr;       // v12 0x0064FFC8 the line said
int  __fastcall wrMsgInit();              // v12 0x004AC5D0 (feb 0x0049E1E0)
void __fastcall wrMsgTerm();              // v12 0x004AC6D0 (feb 0x0049E2E0)
