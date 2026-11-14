// ent_interact.cpp -- entIACTIVE_OBJ, the base of every interactive object: locks, the IAction events, the
// routing of the MP sync messages (Will Rock 1.2, v12.exe; feb obj gs:ent_interact.obj)
//
// Rebuilt from the 1.2 code: ctor v12 0x0050AC00, PreProcessMsg 0x0050AC80, PostProcessMsg 0x0050AEF0,
// IsMPSynchableExisting 0x0050A560, TermIActive 0x0050B100, SendMP_* 0x0050C2E0 / 0x0050C310. 1.2 changes:
//   - the ids moved (GET/WRITE/READ_SYNC_DATA 0x405 / 0x40F / 0x410, MP_NOTIFY 0x40E, MP_UPD_STATE 0x40B);
//   - a lock change is sent to the clients (notify -96, the lock count at +6) once the object has raised event
//     0x800; the clients take the count from it in PostProcessMsg;
//   - MP_UPD_STATE: the u16 sequence at +4 must not go back (wrapping after 50000 to below 2000), unless
//     entFlags 0x800 lets one through; then the subclass gets it (slot +0x20);
//   - FRAME: a remote object is updated from the network (gsMP_SYSTEM v12 0x005149A0) before, a local one in
//     MP after; message 0x3EF resets the incoming sequence.
// The IActions: ParseIActions 0x0050B680 reads the sheet's ON_xxx sections, AddIAction 0x0050BB40 /
// ParseIActionData 0x0050BC20 each one, ExecuteIAction 0x0050B150 sends one to its targets.
// Not rebuilt here yet: the CHG_MODE handler (v12 0x0050B090), GetMPSysSyncData, IdentifyMPSysSyncData.

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "../Sources/game.h"
#include "gs_mp_server.h"
#include "gs_mp_client.h"
#include "gs_mp_misc.h"

int entMPDispatchMsg[10];                   // v12 0x0098E674 the ids PostProcessMsg forwards to the clients
int entNMPDispatchMsg;                      // v12 0x0098E69C

// v12 0x0050AC00  (feb 0x004F8290)
entIACTIVE_OBJ::entIACTIVE_OBJ()
{
    *(u32 *)((char *)this + 4) = 0;         // msgADDR +0x04..+0x1C
    *(u32 *)((char *)this + 8) = 0;
    *(u32 *)((char *)this + 0xC) = 0;
    *(u32 *)((char *)this + 0x10) = 0x4D414452;
    classId = 0;
    *(u32 *)((char *)this + 0x18) = 0;
    priority = 0;
    unk24 = 0;
    nIActions = 0;
    iActions = 0;
    nLocks = 0;
    nSndChannels = 0;
    sndChannels = 0;
    name.str[0] = 0;
    pendingEvents = 0;
    sign = 0x454E5449;                      // ITNE
    mpId = -1;
    mpOwner = -3;
    entFlags = 0;
    mpSeqOut = 0;
    mpSeqIn = 0;
    mpUpdPeriod = 1.0f / 30.0f;             // 0x3D088889
    mpUpdTimer = 0.0f;
}

// v12 0x0050A560  (feb 0x004F7D10)
// A declared network id (not -1 / -3) and the address not being destroyed (msgADDR flags 4 / 8).
BOOL32 entIACTIVE_OBJ::IsMPSynchableExisting()
{
    if (mpId == -1 || mpId == -3)
        return 0;
    return !(*(u8 *)((char *)this + 0xC) & 0xC);
}

// v12 0x0050AC80  (feb 0x004F8300)
BOOL32 entIACTIVE_OBJ::PreProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case 0x405:                             // GET_SYNC_DATA
        ProcessMP_GET_SYNC_DATA(data);
        return 1;
    case 0x40F:                             // WRITE_SYNC_DATA
        ProcessMP_WRITE_SYNC_DATA((gsMSG_MP_SAVELOAD *)data);
        return 1;
    case 0x410:                             // READ_SYNC_DATA
        ProcessMP_READ_SYNC_DATA((gsMSG_MP_SAVELOAD *)data);
        return 1;
    case MSG_MSG_INIT:
        pendingEvents = 0;
        return 0;
    case 0x3EF:
        mpSeqIn = 0;
        return 0;
    case 1000:                              // FRAME: not while the game is paused
        if (!(gsAppState & 0x40000))
            return 1;
        if (entFlags & ENTF_REMOTE)
            gsSysMP->UpdateEntFrame(this);
        return 0;
    case 0x3F8: {                           // IACTION LOCK / UNLOCK
        apNAME *a = (apNAME *)((char *)data + 8);
        if (a->IsName((char *)"LOCK")) {
            if (!nLocks)
                pendingEvents |= 0x40;
            nLocks++;
            pendingEvents |= 0x100;
            ProcessLOCK();
            apLog((char *)"%s Locked (nmb. locks = %d)", name.str, nLocks);
        } else if (a->IsName((char *)"UNLOCK")) {
            if (!--nLocks)
                pendingEvents |= 0x80;
            pendingEvents |= 0x200;
            ProcessUNLOCK();
            apLog((char *)"%s Unlocked (nmb. locks = %d)", name.str, nLocks);
        } else
            return 0;
        if ((entFlags & 4) && (pendingEvents & 0x800)) {
#pragma pack(push, 1)
            struct { msgDATA h; short type; int locks; } m;
#pragma pack(pop)
            m.h.packId = 0;
            m.h.tag = 0x40;
            m.type = -96;
            m.h.size = 0xA;
            m.locks = nLocks;
            if (mpId != -1)
                gsSysServer->SendMsgClientAll(this, 0x40E, &m.h, 0x20000, 0xFFFFFFFF, 0xFFFFFFFD);
        }
        return 0;
    }
    case 0x40B: {                           // MP_UPD_STATE: drop old ones
        u16 seq = *(u16 *)((char *)data + 4);
        if (mpSeqIn > seq && !(mpSeqIn > 50000 && seq < 2000)) {
            if (!(entFlags & 0x800))
                return 1;
        } else
            mpSeqIn = seq;
        entFlags &= ~0x800u;
        return 0;
    }
    }
    return 0;
}

// v12 0x0050AEF0  (feb 0x004F8490)
// The events raised during the message run the IActions hung on them (on the side that runs the object) and the
// sound system's; then messages registered with entRegMsgMPDispatch go to the clients.
BOOL32 entIACTIVE_OBJ::PostProcessMsg(int id, void *data, msgADDR *from, int *ret)
{
    switch (id) {
    case MSG_MSG_TERM:
        gsSysSound->DelChannel(this);
        break;
    case 1000:
        if (gsSysMP->mode != 1 && !(entFlags & ENTF_REMOTE))
            gsSysMP->UpdateEntFrame(this);
        break;
    case 0x3F5:                             // TERM_LEVEL
        TermIActive();
        break;
    case 0x3FD:                             // CHG_MODE notify
        PostProcessCHG_MODE_NOTIFY(data);
        break;
    case 0x40B:
        ProcessMP_UPD_STATE(data);
        break;
    case 0x40E:
        if (*(short *)((char *)data + 4) == -96)
            nLocks = *(int *)((char *)data + 6);
        break;
    }
    u32 ev = pendingEvents;
    if (ev) {
        pendingEvents = ev & 0xFFFFFC00;
        if (entFlags & 4)
            for (int i = 0; i < nIActions; i++)
                if (iActions[i].eventMask & ev)
                    ExecuteIAction(&iActions[i], this);
        gsSysSound->UpdateIAction(this, ev);
    }
    if (mpId != -1 && (gsSysMP->mode == 4 || gsSysMP->mode == 2))
        for (int i = 0; i < entNMPDispatchMsg; i++)
            if (entMPDispatchMsg[i] == id)
                gsSysServer->SendMsgClientAll(this, id, (msgDATA *)data, 0xA0000, 0xFFFFFFFF, 0xFFFFFFFD);
    return 0;
}

// v12 0x00527DC0  (emitted in gs_domain.obj) the IAction table freed, then msgADDR's
entIACTIVE_OBJ::~entIACTIVE_OBJ()
{
    TermIActive();
}

// v12 0x0050B100  (feb 0x004F8640)
void entIACTIVE_OBJ::TermIActive()
{
    for (int i = 0; i < nIActions; i++)
        apFree(iActions[i].heapData);
    apFree(iActions);
    iActions = 0;
    nIActions = 0;
}

// ---------------------------------------------------------------------------------------------- IActions

// (inlined in ExecuteIAction and ParseIActionData) every active object or entity in the message system named name:
// its name starting with name (any case) when partial, else equal to it. out (0: count only) gets them.
static int entFindIActionTarget(char *name, int partial, entIACTIVE_OBJ **out)
{
    int n = 0;
    for (msgADDR *a = msgSystem.GetFirstAddr(); a; a = a->GetNext()) {
        if (a->sign != 0x454E5449 && a->sign != 0x454E5424)           // 'ITNE' entIACTIVE_OBJ, '$TNE' entENTITY
            continue;
        entIACTIVE_OBJ *e = (entIACTIVE_OBJ *)a;
        if ((partial && !_strnicmp(name, e->name.str, strlen(name))) || e->name.IsName(name)) {
            if (out)
                out[n] = e;
            n++;
        }
    }
    return n;
}

// v12 0x0050B150  (feb 0x004F8690)
// One run of a (nAction left): ENABLE_IACTION / DISABLE_IACTION switch the targets' IActions; any other command,
// unless self's IActions are off or its lock count is not condNmbLocks, goes to every target as GS_MSG_IACTION,
// at once or after delay. Each step is logged.
void entIACTIVE_OBJ::ExecuteIAction(entIACTION_DATA *a, entIACTIVE_OBJ *self)
{
    entIACTIVE_OBJ *to[46];                                            // (no bound check)
    if (a->nAction <= 0)
        return;
    a->nAction--;
    apLog((char *)"----- Frame: %d -----", msgSystem.frameNmb);
    apLog((char *)"From: %s", self ? self->name.str : "unknown");
    apLog((char *)"Cmd : %s(%s)", a->cmd.str, a->s1);
    if (!(fabsf(a->delay) < 1e-6f))
        apLog((char *)"Delay: %.3f", (double)a->delay);
    if (a->cmd.IsName((char *)"ENABLE_IACTION") || a->cmd.IsName((char *)"DISABLE_IACTION")) {
        int enable = a->cmd.IsName((char *)"ENABLE_IACTION");
        for (int i = 0; i < a->nObj; i++) {
            int n = entFindIActionTarget(a->objName[i].str, a->eventMask & 0x20000000, to);
            for (int k = 0; k < n; k++) {
                if (enable)
                    to[k]->pendingEvents &= ~IAEV_DISABLED;
                else
                    to[k]->pendingEvents |= IAEV_DISABLED;
                apLog((char *)"To  : %s", to[k]->name.str);
            }
            if (!n)
                apLogErr((char *)"NotFound: %s", a->objName[i].str);
        }
        return;
    }
    if (self && (self->pendingEvents & IAEV_DISABLED)) {
        apLog((char *)"To  : IACTION_DISABLED");
        return;
    }
    if (a->condNmbLocks != -1 && a->condNmbLocks != self->nLocks) {
        apLog((char *)"Skip IAction : nmbLocks condition %d  %d", a->condNmbLocks, self->nLocks);
        return;
    }
    for (int i = 0; i < a->nObj; i++) {
        int n = entFindIActionTarget(a->objName[i].str, a->eventMask & 0x20000000, to);
        for (int k = 0; k < n; k++) {
            apLog((char *)"To  : %s", to[k]->name.str);
            if (fabsf(a->delay) < 1e-6f)
                msgSystem.SendMsg(to[k], 0x3F8, a, self);
            else
                msgSystem.PostTimeMsgData(to[k], 0x3F8, (msgDATA *)a, self, a->delay);
        }
        if (!n)
            apLogErr((char *)"NotFound: %s", a->objName[i].str);
    }
}

// v12 0x0050B680  (feb 0x004F8BC0)
// [IACTIVE] nmbLocks and isIActionEnabled, then the IAction sections: ON_ACTIVATE, ON_DEACTIVATE, ON_START, ON_STOP,
// ON_ACTION, ON_COMPLETE, ON_LOCK, ON_UNLOCK, ON_LOCK_CMD, ON_UNLOCK_CMD, then the same with _1, _2, ... while
// any of a number is there (the original unrolls the ten).
void entIACTIVE_OBJ::ParseIActions(psSHEET *ps)
{
    static const struct { const char *name, *nameN; int ev; } kinds[10] = {
        { "ON_ACTIVATE", "ON_ACTIVATE_%d", 0x01 }, { "ON_DEACTIVATE", "ON_DEACTIVATE_%d", 0x02 },
        { "ON_START", "ON_START_%d", 0x04 },       { "ON_STOP", "ON_STOP_%d", 0x08 },
        { "ON_ACTION", "ON_ACTION_%d", 0x10 },     { "ON_COMPLETE", "ON_COMPLETE_%d", 0x20 },
        { "ON_LOCK", "ON_LOCK_%d", IAEV_LOCK },    { "ON_UNLOCK", "ON_UNLOCK_%d", IAEV_UNLOCK },
        { "ON_LOCK_CMD", "ON_LOCK_CMD_%d", IAEV_LOCK_CMD }, { "ON_UNLOCK_CMD", "ON_UNLOCK_CMD_%d", IAEV_UNLOCK_CMD },
    };
    char sect[0x40];
    if (!ps)
        return;
    if (ps->GetInt((char *)"IACTIVE", (char *)"nmbLocks", &nLocks))
        apLog((char *)"%s: Read: nmbLocks  = %d", name.str, nLocks);
    int on;
    if (ps->GetBool((char *)"IACTIVE", (char *)"isIActionEnabled", &on) && !on) {
        pendingEvents |= IAEV_DISABLED;
        apLog((char *)"%s: Read: isIActionEnabled  = %d", name.str, 0);
    }
    for (int n = 0;; n++) {
        int found = 0;
        for (int k = 0; k < 10; k++) {
            sprintf(sect, kinds[k].name);
            if (n > 0)
                sprintf(sect, kinds[k].nameN, n);
            if (ps->FindSect(sect)) {
                AddIAction(ps, sect, kinds[k].ev);
                found = 1;
            }
        }
        if (!found)
            break;
    }
}

// v12 0x0050BA30  (feb 0x004F8F70)
int entIACTIVE_OBJ::ActionName2State(char *name)
{
    if (!strcmp(name, "ACTIVATE"))
        return 0x01;
    if (!strcmp(name, "DEACTIVATE"))
        return 0x02;
    if (!strcmp(name, "START"))
        return 0x04;
    if (!strcmp(name, "STOP"))
        return 0x08;
    if (!strcmp(name, "ACTION"))
        return 0x10;
    if (!strcmp(name, "COMPLETE"))
        return 0x20;
    if (!strcmp(name, "LOCK"))
        return IAEV_LOCK;
    if (!strcmp(name, "UNLOCK"))
        return IAEV_UNLOCK;
    if (!strcmp(name, "LOCK_CMD"))
        return IAEV_LOCK_CMD;
    return strcmp(name, "UNLOCK_CMD") ? 0 : IAEV_UNLOCK_CMD;
}

// v12 0x0050BB40  (feb 0x004F9080)
// The section sect as an IAction raised by the events ev; its index, -1 if unreadable.
int entIACTIVE_OBJ::AddIAction(psSHEET *ps, char *sect, int ev)
{
    entIACTION_DATA d;
    d.hdr.type = 0;
    d.hdr.flags = 0x40;
    d.hdr.size = 0xA0;
    d.eventMask = 0;
    d.cmd.str[0] = 0;
    d.i1 = 0;
    d.f1 = 0;
    d.b1 = 0;
    d.s1[0] = 0;
    if (!ParseIActionData(ps, sect, (char *)"", &d, name.str))
        return -1;
    d.eventMask |= ev;
    entIACTION_DATA *t = (entIACTION_DATA *)apRealloc(iActions, (nIActions + 1) * sizeof(entIACTION_DATA));
    if (!t)
        return -1;
    iActions = t;
    iActions[nIActions] = d;
    nIActions++;
    return nIActions - 1;
}

// v12 0x0050BC20  (feb 0x004F9160)
// One IAction from the section sect (keys prefixed by prefix): cmd (required), isPartialName, the targets objName,
// objName_1, ..., delay, nAction, condNmbLocks and the arguments i1_ i2_ f1_ f2_ b1_ b2_ s1_ (each the first key
// starting so). Everything read is logged with the owner's name. 0: no cmd; -1: out of memory.
BOOL32 entIACTIVE_OBJ::ParseIActionData(psSHEET *ps, char *sect, char *prefix, entIACTION_DATA *d, char *owner)
{
    char key[0x40], obj[0x40], cmd[0x40];
    apLog((char *)"%s: Read: ON_XXX  = %s", owner, sect);
    sprintf(key, "%scmd", prefix);
    if (!ps->GetStr(sect, key, cmd, 0x40)) {
        apLogErr((char *)"%s Error Reading command: %s", owner, key);
        return 0;
    }
    d->animSeq = -1;
    d->actionFrame = -1;
    d->condNmbLocks = -1;
    d->objName = 0;
    d->nObj = 0;
    d->delay = 0.0f;
    d->nAction = 0x5FFFFFFF;
    d->eventMask = 0;
    d->cmd.SetName(cmd);
    apLog((char *)"%s: Read: %s  = %s", owner, key, cmd);
    sprintf(key, "%sisPartialName", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        int partial;
        ps->GetBool(sect, key, &partial);
        if (partial)
            d->eventMask |= 0x20000000;
        apLog((char *)"%s: Read: %s  = %d", owner, v->name, partial);
    }
    for (int i = 0;; i++) {
        if (!i)
            sprintf(key, "%s%s", prefix, "objName");
        else
            sprintf(key, "%s%s_%d", prefix, "objName", i);
        if (!ps->GetStr(sect, key, obj, 0x40))
            break;
        apLog((char *)"%s: Read: %s  = %s", owner, key, obj);
        if (!entFindIActionTarget(obj, d->eventMask & 0x20000000, 0))
            apLogErr((char *)"%s Error: Objects %s not found", owner, obj);
        apNAME *t = (apNAME *)apRealloc(d->objName, (d->nObj + 1) * sizeof(apNAME));
        if (!t)
            return -1;
        d->objName = t;
        d->objName[d->nObj].SetName(obj);
        d->nObj++;
    }
    if (!d->nObj)
        apLogErr((char *)"%s Error: No objects were specifyed for section: %s", owner, sect);
    sprintf(key, "%sdelay", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetFloat(sect, key, &d->delay);
        apLog((char *)"%s: Read: %s  = %f", owner, v->name, (double)d->delay);
    }
    sprintf(key, "%snAction", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetInt(sect, key, &d->nAction);
        apLog((char *)"%s: Read: %s  = %d", owner, v->name, d->nAction);
    }
    sprintf(key, "%scondNmbLocks", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetInt(sect, key, &d->condNmbLocks);
        apLog((char *)"%s: Read: %s  = %d", owner, v->name, d->condNmbLocks);
    }
    sprintf(key, "%si1_", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetInt(sect, v->name, &d->i1);
        apLog((char *)"%s: Read: %s  = %d", owner, v->name, d->i1);
    }
    sprintf(key, "%si2_", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetInt(sect, v->name, &d->i2);
        apLog((char *)"%s: Read: %s  = %d", owner, v->name, d->i2);
    }
    sprintf(key, "%sf1_", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetFloat(sect, v->name, &d->f1);
        apLog((char *)"%s: Read: %s  = %f", owner, v->name, (double)d->f1);
    }
    sprintf(key, "%sf2_", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetFloat(sect, v->name, &d->f2);
        apLog((char *)"%s: Read: %s  = %f", owner, v->name, (double)d->f2);
    }
    sprintf(key, "%sb1_", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetBool(sect, v->name, &d->b1);
        apLog((char *)"%s: Read: %s  = %d", owner, v->name, d->b1);
    }
    sprintf(key, "%sb2_", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetBool(sect, v->name, &d->b2);
        apLog((char *)"%s: Read: %s  = %d", owner, v->name, d->b2);
    }
    sprintf(key, "%ss1_", prefix);
    if (psVAR *v = ps->FindVarStr(sect, key)) {
        ps->GetStr(sect, v->name, d->s1, 0x40);
        apLog((char *)"%s: Read: %s  = %s", owner, v->name, d->s1);
    }
    return 1;
}

// (feb 0x004F9800; v12 inlined into its callers)
void __fastcall entRegMsgMPDispatch(int id)
{
    entMPDispatchMsg[entNMPDispatchMsg++] = id;
}

// v12 0x0050C2E0  (feb 0x004F9840)
void entIACTIVE_OBJ::SendMP_Server_2_ClientAll(msgDATA *d, int flags)
{
    if (mpId != -1)
        gsSysServer->SendMsgClientAll(this, 0x40E, d, flags, 0xFFFFFFFF, 0xFFFFFFFD);
}

// v12 0x0050C310  (feb 0x004F9870)
void entIACTIVE_OBJ::SendMP_Client_2_Server(msgDATA *d, int flags)
{
    gsSysClient->SendMsgServer(this, 0x40E, d, flags);
}

// The empty base slots (v12: the shared empty functions 0x00403A20 / 0x0051AAF0 / 0x00403950)
void entIACTIVE_OBJ::ProcessLOCK() {}
void entIACTIVE_OBJ::ProcessUNLOCK() {}
void entIACTIVE_OBJ::ProcessMP_GET_SYNC_DATA(void *) {}
void entIACTIVE_OBJ::ProcessMP_WRITE_SYNC_DATA(gsMSG_MP_SAVELOAD *) {}
void entIACTIVE_OBJ::ProcessMP_READ_SYNC_DATA(gsMSG_MP_SAVELOAD *) {}
int  entIACTIVE_OBJ::ProcessMP_UPD_STATE(void *) { return 0; }

// v12 0x0050C340  (feb 0x004F98A0)
// The next interactive object ('ITNE' or entity '$TNE') in the global address list after prev (0: the first).
entIACTIVE_OBJ *__fastcall entFindNextIO(entIACTIVE_OBJ *prev)
{
    msgADDR *a = prev ? ((msgADDR *)prev)->GetNext() : msgSystem.GetFirstAddr();
    while (a && a->GetSign() != 0x454E5424 && a->GetSign() != 0x454E5449)
        a = a->GetNext();
    return (entIACTIVE_OBJ *)a;
}

// v12 0x0050C390  (feb 0x004F98F0)  entIACTION::ctor
// The message header (pack 0, '@', 0xA0), no events, no command, the first int / float / bool / string cleared.
void entIACTION_Init(entIACTION_DATA *d)
{
    d->hdr.type = 0;
    d->hdr.flags = 0x40;
    d->eventMask = 0;
    d->cmd.str[0] = 0;
    d->i1 = 0;
    d->f1 = 0;
    d->b1 = 0;
    d->s1[0] = 0;
    d->hdr.size = 0xA0;
}

// v12 0x0050B090  (feb 0x004F85D0)
// The game's mode (gsMsgChangeMode): suspended (0) every address is locked out of the messages if the game really
// stopped -- except an entity with flags2 0x40, which is let in -- and back (1) the other way round; 2 / 3 lock /
// unlock always; the others (a cinema's 4 / 5) change nothing.
void entIACTIVE_OBJ::PostProcessCHG_MODE_NOTIFY(void *d)
{
    int isEnt = sign == 0x454E5424;
    switch (*(int *)d) {
    case 0:
        if (isEnt && (((entENTITY *)this)->flags2 & 0x40))
            unk24--;
        else if (_eioIsUpdSuspend)
            unk24++;
        break;
    case 1:
        if (isEnt && (((entENTITY *)this)->flags2 & 0x40))
            unk24++;
        else if (_eioIsUpdSuspend)
            unk24--;
        break;
    case 2:
        unk24++;
        break;
    case 3:
        unk24--;
        break;
    }
}
