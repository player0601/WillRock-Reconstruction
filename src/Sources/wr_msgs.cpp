// wr_msgs.cpp -- Will's voice lines, the level's message and secret-area domains (Will Rock 1.2, v12.exe; feb obj
// wr_msgs.obj)
//
// v12 0x004AC5D0..0x004AD050 (see include/frontend/wr_msgs.h). The line code is written out again in each place that
// speaks (ProcessMsg's SHOW, wrDOMAIN_MSG::Enter, the secret found) as the original's inline copies of ShowMsg; here
// they call it.

#include "game.h"
#include "../gs/gs_mp_server.h"
#include "../gs/gs_mp_misc.h"
#include "../../include/frontend/wr_msgs.h"

wrSECRET_SYSTEM *wrSecretSystem;                        // v12 0x0064FFBC
wrMSG_SYSTEM    *wrMsgSystem;                           // v12 0x0064FFC0
sncCHANNEL      *wrMsgChannel;                          // v12 0x0064FFC4
sncSOUND_DESCR   wrMsgDescr;                            // v12 0x0064FFC8

#pragma pack(push, 1)
struct wrSECRET_MSG : msgDATA {                         // GS_MSG_MP_NOTIFY 0x40E type 0: a secret area found (0x0A bytes)
    short         type;                                 // +0x04 0
    unsigned long id;                                   // +0x06 the finder's MP id
};
#pragma pack(pop)

// v12 0x004AC5D0  (feb 0x0049E1E0)
// The "msg" list, its voice (in the sound system's list, the list its owner) and the "secret" list.
int __fastcall wrMsgInit()
{
    wrMSG_SYSTEM *m = new wrMSG_SYSTEM;
    if (!m) {
        wrMsgSystem = 0;
        return 0;
    }
    m->name.SetName((char *)"msg");
    wrMsgSystem = m;
    msgSystem.InsertAddr(m, 0x24534E44, 0x57);
    wrMsgChannel = new sncCHANNEL;
    if (!wrMsgChannel)
        return 0;
    gsSysSound->AddChannel(wrMsgChannel, wrMsgSystem, 0);
    wrSECRET_SYSTEM *s = new wrSECRET_SYSTEM;
    if (!s) {
        wrSecretSystem = 0;
        return 0;
    }
    s->name.SetName((char *)"secret");
    wrSecretSystem = s;
    msgSystem.InsertAddr(s, 0x24534E44, 0x57);
    return 1;
}

// v12 0x004AC6D0  (feb 0x0049E2E0)
void __fastcall wrMsgTerm()
{
    gsSysSound->DelChannel(wrMsgSystem);
    wrMsgChannel = 0;
    msgSystem.DestroyAddr(wrMsgSystem);
    wrMsgSystem = 0;
    msgSystem.DestroyAddr(wrSecretSystem);
    wrSecretSystem = 0;
}

// v12 0x004AC730  (feb 0x0049E340)
// The line id said (the one before stopped) and shown for time seconds: always when it has no sound, else unless
// [GameOpt] Subtitles is off; a line with no string is not shown.
void __fastcall wrMSG_SYSTEM::ShowMsg(char *id, float time)
{
    wrMsgChannel->Stop();
    wrMsgDescr.UnlinkData();
    if (wrMsgDescr.LinkData(id, 6)) {
        wrMsgChannel->Play(&wrMsgDescr, 0);
        int on;
        if (_apCfgReadBool(0, (char *)"GameOpt", (char *)"Subtitles", &on) && !on)
            return;
    }
    if (!gsStrings->GetStringId(id))
        return;
    uiSystem->ShowMsgW(gsStrings->GetStringById(gsStrings->GetStringId(id)), time, 0);
}

// v12 0x004AC7E0  (feb 0x0049E3F0)
int wrMSG_SYSTEM::IsAddObj(objOBJ *o)
{
    return strstr(o->name, "&dom_msg") != 0;
}

// v12 0x004AC800  (feb 0x0049E410)
// 0x3F5: the line stopped; the IAction "SHOW" (not in co-op): its string s1 said for its f1 seconds (none:
// 1.5). Then the list's own messages.
BOOL32 wrMSG_SYSTEM::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x3F5) {
        wrMsgChannel->Stop();
    } else if (id == 0x3F8 && wrSysGO->mode != 2 && ((apNAME *)((u8 *)data + 8))->IsName((char *)"SHOW")) {
        float t = *(float *)((u8 *)data + 0x50);
        ShowMsg((char *)data + 0x60, t > 1e-6f ? t : 1.5f);
    }
    return gsDOMAIN_LIST::ProcessMsg(id, data, from, ret);
}

// v12 0x004ACFD0  (feb 0x0049EB80)
gsDOMAIN *wrMSG_SYSTEM::MakeDomain()
{
    return new wrDOMAIN_MSG;
}

// v12 0x004AC9E0  (feb 0x0049E5B0)
// [DOM_MSG] isShowOnce: said the first time only.
BOOL32 wrDOMAIN_MSG::Init()
{
    int once;
    psSHEET *ps = obj->props;
    if (ps && ps->GetBool((char *)"DOM_MSG", (char *)"isShowOnce", &once) && once)
        domFlags |= 0x200;
    return gsDOMAIN::Init();
}

// v12 0x004ACA30  (feb 0x0049E600)
// A player came in (a show-once domain: the first time): its [DOM_MSG] msg said for 1.5 s; the IActions' event 0x20.
void wrDOMAIN_MSG::Enter()
{
    if ((domFlags & 0x200) && (domFlags & 0x100))
        return;
    domFlags |= 0x100;
    char line[0x104];
    psSHEET *ps = obj->props;
    if (ps && ps->GetStr((char *)"DOM_MSG", (char *)"msg", line, 0x104))
        wrMSG_SYSTEM::ShowMsg(line, 1.5f);
    pendingEvents |= 0x20;
}

// v12 0x004ACB50  (feb 0x0049E720)
int wrSECRET_SYSTEM::IsAddObj(objOBJ *o)
{
    return strstr(o->name, "&dom_secret") != 0;
}

// v12 0x004ACB70  (feb 0x0049E740)
// The IAction "FIND": "GAME_MSG_SECRET_FOUND" said. Then the list's own messages.
BOOL32 wrSECRET_SYSTEM::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x3F8 && ((apNAME *)((u8 *)data + 8))->IsName((char *)"FIND"))
        wrMSG_SYSTEM::ShowMsg((char *)"GAME_MSG_SECRET_FOUND", 1.5f);
    return gsDOMAIN_LIST::ProcessMsg(id, data, from, ret);
}

// v12 0x004AD010  (feb 0x0049EBC0)
gsDOMAIN *wrSECRET_SYSTEM::MakeDomain()
{
    wrDOMAIN_SECRET *d = new wrDOMAIN_SECRET;
    if (d)
        d->nFound = 0;
    return d;
}

// v12 0x004ACC70  (feb 0x0049E840)
// One more secret area in the level (the statistics' +0x9C), declared to the MP server.
BOOL32 wrDOMAIN_SECRET::Init()
{
    if (wrSysStat)
        WR_FIELD(int, wrSysStat, 0x9C)++;
    gsSysServer->DeclSyncable(this);
    return gsDOMAIN::Init();
}

// v12 0x004ACCA0  (feb 0x0049E870)
// A player's point inside: the first time for that player (16 kept, the last one replaced when full) the server
// tells every client who found it (notify type 0).
void wrDOMAIN_SECRET::Update(m3dV *pos)
{
    if (!IsBelongPoint(pos))
        return;
    entENTITY *p = gsSysServer->GetPlayer(wrSecretSystem->curDomain);
    if (!p)
        return;
    unsigned long id = p->mpId;
    for (int i = 0; i < nFound; i++)
        if (found[i] == id)
            return;
    if (nFound == 16)
        nFound = 15;
    found[nFound] = id;
    nFound++;
    if (!(entFlags & 4))
        return;
    wrSECRET_MSG m;
    m.packId = 0;
    m.type = 0;
    m.tag = 0x40;
    m.size = 0x0A;
    m.id = p->mpId;
    gsSysServer->SendMsgClientAll(this, 0x40E, &m, 0x20000, -3, -3);
}

// v12 0x004ACD90  (feb 0x0049E960)
// A player gone (0x409): forgotten. The secret found (notify type 0): for the local player "GAME_MSG_SECRET_FOUND"
// said, its sound (unk108 0x4000000) and the IActions' event 0x20; the finder's statistics count it (+0x78).
BOOL32 wrDOMAIN_SECRET::ProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    if (id == 0x409) {
        unsigned long pid = ((entENTITY *)data)->mpId;
        int i = 0;
        for (; i < nFound; i++)
            if (found[i] == pid)
                break;
        if (i != nFound) {
            for (; i < nFound - 1; i++)
                found[i] = found[i + 1];
            nFound--;
        }
    } else if (id == 0x40E && ((wrSECRET_MSG *)data)->type == 0) {
        wrSECRET_MSG *m = (wrSECRET_MSG *)data;
        if (plrPlayer && m->id == (unsigned long)plrPlayer->mpId) {
            wrMSG_SYSTEM::ShowMsg((char *)"GAME_MSG_SECRET_FOUND", 1.5f);
            plrPlayer->unk108 |= 0x4000000;
            pendingEvents |= 0x20;
        }
        if (wrSysStat)
            (*(int *)(wrSysStat->GetPlrRecord((plrPLAYER_COMMON *)gsSysMP->FindMPID(m->id, 0)) + 0x78))++;
    }
    return gsDOMAIN::ProcessMsg(id, data, from, ret);
}
