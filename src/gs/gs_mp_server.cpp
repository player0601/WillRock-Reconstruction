// gs_mp_server.cpp -- Will Rock 1.2 (v12.exe) gsMP_SERVER, reconstructed for modders.
// See gs_mp_server.h for conventions, assumed types and the message id table.
// Disassembly used: re/work/mpsrc_gs_mp_server/v12.asm (0x0050F140..0x005109A0), feb.asm, feb.cpp.

#include "gs_mp_server.h"
// [integrator: all MP headers, so cross-object types are complete for the syntax check]
#include "network.h"
#include "network_dpubi.h"
#include "network_dpubi_dp.h"
#include "mp_messages.h"
#include "gs_mp_conn.h"
#include "gs_mp_network.h"
#include "gs_mp_misc.h"
#include "gs_mp_client.h"
#include "gs_mp_server.h"
#include "gs_play.h"
#include "wr_go_mp.h"
#include "wr_ui_mult.h"
#include "wr_ui_server.h"
#include "wr_ui_join.h"

// ---------------------------------------------------------------------------
// v12 0x0050FA10  (feb 0x004FD220 "_$E2")
// static initializer of this .obj: a file-scope float = 1e-6f * 1e-6f
// (constant 1e-6f at v12 0x00607AB0, result stored at v12 0x0098E9E8, feb 0x0094830C)
static float s_fEps2 = 1e-6f * 1e-6f;   // ? name / users unknown (no reference inside this file)


// ===========================================================================
// construction
// ===========================================================================

// v12 0x0050F140  (feb 0x004FCA40)
// ctor: clear the client table; every slot = MPCLIENT_NONE
gsMP_SERVER::gsMP_SERVER()
    : gsMP_COMM()          // msgADDR fields +4..+0x24 = 0, +0x10 = 'RDAM' (0x4D414452), +0x28 = 0, gsMP_CONNECTION(+0x68)
{
    flags = 0;
    for (int i = 0; i < MPSRV_MAX_CLIENTS; i++) {
        gsMP_CLIENT_REC &r = clients[i];
        ((char *)&r)[8] = 0;                  // apNAME empty
        memset(&r, 0, sizeof(r));             // 0x15 dwords (feb 0x16)
        r.rate = 1.0f;                        // 1.2: 1.0f (0x3F800000); feb 1/30 s (0x3D088889)
        r.id   = MPCLIENT_NONE;
    }
    // msgADDR +0x1C (priority / order ?) = 10
    *(int *)((char *)this + 0x1C) = 10;
    nClients = 0;
    nextMPID = 0;
    // 1.2: connSpeed (+0x2CD0) is not initialised here; InitGame sets it
}

// v12 0x0050F1D0  (feb 0x004FCAD0)
// vector deleting destructor (vtable slot +0x00)
void *gsMP_SERVER::__vecDelDtor(unsigned int f)
{
    this->~gsMP_SERVER();
    if (f & 1) operator delete(this);
    return this;
}

// v12 0x0050F200  (feb 0x004FCB00)
// gsMP_COMM scalar deleting destructor (gsMP_SERVER vtable slot +0x30, compiler-generated)
//   ~gsMP_CONNECTION(this+0x68); ~msgADDR(); if (f & 1) delete this;

// v12 0x0050F230  (feb 0x004FCB30)
// destructor: only the embedded gsMP_CONNECTION and msgADDR base are torn down
gsMP_SERVER::~gsMP_SERVER()
{
    // conn.~gsMP_CONNECTION(); msgADDR::~msgADDR();
}


// ===========================================================================
// client table
// ===========================================================================

// v12 0x0050F250  (feb 0x004FCB50)   vtable slot +0x20
// a DirectPlay player connected (gsMP_NETWORK::OnNetDrvEvent ev 3): append a record
int gsMP_SERVER::AddClient(unsigned long id)
{
    if (nClients >= MPSRV_MAX_CLIENTS)      // hard cap 16 (cmp 0x10 at v12 0x0050F256)
        return 0;
    clients[nClients].id = id;
    nClients++;
    return 1;
}

// v12 0x0050F290  (feb 0x004FCB90)   vtable slot +0x24
// a client left (OnNetDrvEvent ev 4, TermGame, or its player entity died: msg 3):
// tell its player entity to delete itself, compact the table, clear the last slot
void gsMP_SERVER::RemoveClient(unsigned long id)
{
    int i;
    if (gsSysMP->mode == 1 /*Single*/ && id == MPCLIENT_LOCAL) {
        i = 0;
    } else {
        for (i = 0; i < nClients; i++)
            if (clients[i].id == id) break;
        if (i >= nClients) return;
        if (i < 0) return;
        if (id != MPCLIENT_LOCAL && clients[i].player != NULL)
            msgSystem.PostMsg(clients[i].player, 2 /*delete yourself*/, NULL);
    }
    if (i < nClients - 1)
        memmove(&clients[i], &clients[i + 1], (nClients - i - 1) * sizeof(gsMP_CLIENT_REC));
    nClients--;
    memset(&clients[nClients], 0, sizeof(gsMP_CLIENT_REC));   // 1.2: 0x15 dwords; rate is left 0 here (not reset to 1.0)
    clients[nClients].id = MPCLIENT_NONE;
}

// v12 0x0050F3B0  (feb 0x004FCC90)   vtable slot +0x28
// bind a player entity (and its name) to the client record
void gsMP_SERVER::AddPlayer(entENTITY *ent, unsigned long id)
{
    int i;
    if (flags & MPSRV_LOCAL) {
        i = 0;
    } else if (gsSysMP->mode == 1 && id == MPCLIENT_LOCAL) {
        i = 0;
    } else {
        for (i = 0; i < nClients; i++)
            if (clients[i].id == id) break;
        if (i >= nClients) i = -1;          // not found: writes clients[-1] (same bug as feb)
    }
    clients[i].player = ent;
    const char *n = ent->anim /*+0xC0*/->name /*+0x18*/;
    clients[i].name.SetName(n ? n : "");
}

// v12 0x0050F440  (feb 0x004FCD20)
// iterate the players: GetNextPlayer(NULL) = first, then pass the previous result
entENTITY *gsMP_SERVER::GetNextPlayer(entENTITY *prev)
{
    if (flags & MPSRV_LOCAL) {
        if (prev) return NULL;
        return clients[0].player;
    }
    if (prev == NULL) {
        if (gsAppState & 0x08000000) {      // 1.2: -dedicated: slot 0 is the host, which has no real player
            if (nClients <= 1) return NULL;
            iterPlayer = 1;
        } else {
            iterPlayer = 0;
        }
        return clients[iterPlayer].player;
    }
    // 1.2: re-find prev if the cursor was moved by a nested iteration
    if (prev != clients[iterPlayer].player) {
        int i;
        for (i = 0; i < nClients; i++)
            if (prev == clients[i].player) { iterPlayer = i; break; }
        if (i == nClients) return NULL;
    }
    if (iterPlayer >= nClients) return NULL;
    if (++iterPlayer >= nClients) return NULL;
    return clients[iterPlayer].player;
}

// v12 0x0050F530  (feb 0x004FCD90)
// number of players (1 in single player)
int gsMP_SERVER::GetNPlayer()
{
    if (gsSysMP->mode == 1) return 1;
    return nClients;            // includes the dedicated host's slot 0
}

// v12 0x0050F550  (feb 0x004FCDB0)
// player entity of record i
entENTITY *gsMP_SERVER::GetPlayer(int i)
{
    if (gsSysMP->mode == 1 && i == 0)
        return clients[0].player;
    if (i >= 0 && i < nClients)
        return clients[i].player;
    return NULL;
}


// ===========================================================================
// MPIDs (network ids of syncable entities)
// ===========================================================================

// v12 0x0050F5A0  (feb 0x004FCDF0)
// give an entity the next MPID (server) or mark it client-side
void gsMP_SERVER::DeclSyncable(entIACTIVE_OBJ *o)
{
    if (o->mpid /*+0x88*/ != MPID_UNDECLARED) return;
    if (gsSysMP->mode == 3 /*Client*/) {
        o->mpid = MPID_CLIENTSIDE;
        return;
    }
    o->mpFlags /*+0x84*/ |= 6;
    o->mpid = nextMPID++;
}

// v12 0x0050F600  (feb 0x004FCE50)
void gsMP_SERVER::UnDeclSyncable(entIACTIVE_OBJ *o)
{
    o->mpid = MPID_UNDECLARED;
}

// v12 0x0050F620  (feb 0x004FCE70)
// keep nextMPID above an id assigned elsewhere
void gsMP_SERVER::NotifyNewMPID(unsigned long mpid)
{
    if (nextMPID <= mpid)
        nextMPID = mpid + 1;
}


// ===========================================================================
// entity sync
// ===========================================================================

// v12 0x0050F640  (feb 0x004FCE90)
// fill a GS_MSG_MP_SYNC for one syncable entity; 0 if it does not sync
int gsMP_SERVER::CreateSyncInfo(entIACTIVE_OBJ *o, gsMSG_MP_SYNC *m)
{
    if (!o->IsMPSynchableExisting())          // vt +0x10
        return 0;
    m->mpid    = o->mpid;                     // +0x88
    m->classId = o->classId;                  // +0x14
    strcpy(m->str1, "");
    strcpy(m->str2, "");
    strcpy(m->name, o->name /*+0x40*/);
    o->GetMPSysSyncData(m);                   // vt +0x14
    m->ent30 = o->field30;                    // +0x30
    m->ent80 = o->field80;                    // 1.2: +0x80 also sent (? meaning)
    return 1;
}

// v12 0x0050F6D0  (feb 0x004FCF10)
// ask an entity for its extra sync data (GS_MSG_MP_GET_SYNC_DATA returns a msgDATA*)
msgDATA *gsMP_SERVER::CreateSyncUsrInfo(entIACTIVE_OBJ *o)
{
    if (!o->IsMPSynchableExisting())
        return NULL;
    msgDATA *p = NULL;
    msgSystem.SendMsg(o, GS_MSG_MP_GET_SYNC_DATA /*1.2: 0x405, feb 0x404*/, &p, NULL);
    return p;
}

// v12 0x0050F710  (feb 0x004FCF50)
// bring a freshly joined client up to date: rate, every syncable entity, their
// extra data, positions, the other players' ready state, then "sync complete".
// Called from the join handler and from gspPLAY_SYSTEM::ProcessMsg (0x413 after map change, v12 0x00534B4D).
void gsMP_SERVER::SyncClient(entENTITY *player)
{
    gsMSG_MP_SYNC          sync;      // ctor v12 0x0050E870 then: mpid -1, classId -3, +0x10/+0x14/+0x18 0, +0xDC -1
    gsEV_SYNC              ev;
    gsMSG_MP_CLIENT_READY  rdy;
    unsigned long          to = player->ownerClient;    // +0x8C (feb +0x90)

    sync.size = 0x176;                // feb 0x172
    sync.str1[0] = 0; sync.name[0] = 0;
    rdy.type = 0; rdy.flags = 0x40; rdy.size = 0x0C; rdy.mpid = MPID_UNDECLARED; rdy.ready = 1;

    // 1.2: first tell the client the server's connection-speed factor
    ev.type = 0; ev.flags = 0x40; ev.size = 0x0A;
    ev.code  = MPNOTIFY_RATE;         // -200 (0xFF38)
    ev.fRate = connSpeed;             // +0x2CD0
    SendMsgClient(to, NULL, GS_MSG_MP_NOTIFY /*0x40E*/, &ev, MPSEND_GUARANTEED);

    // every syncable object except the player itself
    for (entIACTIVE_OBJ *o = entFindNextIO(NULL); o; o = entFindNextIO(o)) {
        if (o == player) continue;
        if (!o->IsMPSynchableExisting()) continue;
        sync.mpid    = o->mpid;
        sync.classId = o->classId;
        strcpy(sync.str1, "");
        strcpy(sync.str2, "");
        strcpy(sync.name, o->name);
        o->GetMPSysSyncData(&sync);
        sync.ent30 = o->field30;
        sync.ent80 = o->field80;      // 1.2
        SendMsgClient(to, NULL, GS_MSG_MP_SYNC /*0x404, feb 0x403*/, &sync, MPSEND_GUARANTEED);
    }
    // end of the entity list
    sync.classId  = 0xFFFFFFFF;       // -1 in +0x0C marks "end of entity list" (mpid keeps the last value)
    sync.userData = NULL;
    SendMsgClient(to, NULL, GS_MSG_MP_SYNC, &sync, MPSEND_GUARANTEED);

    // extra per-entity sync data
    for (entIACTIVE_OBJ *o = entFindNextIO(NULL); o; o = entFindNextIO(o)) {
        if (o == player) continue;
        if (!o->IsMPSynchableExisting()) continue;
        msgDATA *p = NULL;
        msgSystem.SendMsg(o, GS_MSG_MP_GET_SYNC_DATA /*0x405*/, &p, NULL);
        if (p) {
            gsEV_SYNC e;              // code -100
            e.pData = p;
            SendMsgClient(to, o, GS_MSG_MP_NOTIFY /*0x40E, feb 0x40C*/, &e, MPSEND_GUARANTEED);
        }
    }

    // positions of all entities
    for (entENTITY *e = entFindNext(NULL); e; e = entFindNext(e)) {
        if (e == player) continue;
        // 1.2: no "(e->flags & 0x100)" test any more, and the message is built by the entity
        //      (vt +0x1C, v12 0x0050A750: static msg + sequence number e+0x3C, then GetMP_UPD_POS)
        msgDATA *p = NULL;
        e->GetMP_UPD_REGULAR(&p);     // vt +0x1C (name ?)
        SendMsgClient(to, e, GS_MSG_MP_UPD_REGULAR /*0x40B, feb 0x40A UPD_POS*/, p, MPSEND_GUARANTEED);
    }

    // which other players are already in the game
    for (int i = 0; i < nClients; i++) {
        entENTITY *pl = clients[i].player;
        if (!clients[i].ready) continue;
        if (pl == player) continue;
        if (pl == NULL) continue;     // 1.2: null check
        rdy.mpid = pl->mpid;
        SendMsgClient(to, NULL, GS_MSG_MP_CLIENT_READY /*0x406, feb 0x405*/, &rdy, MPSEND_GUARANTEED);
    }

    // sync complete
    sync.classId  = 0xFFFFFFFE;       // -2 in +0x0C: "sync complete"
    sync.userData = NULL;
    SendMsgClient(to, NULL, GS_MSG_MP_SYNC, &sync, MPSEND_GUARANTEED);   // feb 0x403 (same message; feb decompile shows a different local, ?)
}

// v12 0x0050FA30  (feb 0x004FD240)
// before a map change (gspPLAY_SYSTEM::StartNewGameMap): every remote player gets CLIENT_READY(0) and is un-readied
void gsMP_SERVER::DeActivateClientAll()
{
    gsMSG_MP_CLIENT_READY m;
    m.type = 0; m.flags = 0x40; m.size = 0x0C;
    m.mpid = MPID_UNDECLARED;
    m.ready = 0;
    for (int i = 0; i < nClients; i++) {
        if (clients[i].id != MPCLIENT_LOCAL && clients[i].player != NULL) {
            msgSystem.SendMsg(clients[i].player, GS_MSG_MP_CLIENT_READY /*0x406, feb 0x405*/, &m, NULL);
            clients[i].ready = 0;
        }
    }
}


// ===========================================================================
// message handler
// ===========================================================================

// v12 0x0050FAB0  (feb 0x004FD2C0)   vtable slot +0x04
// all server-side game messages. Returns 0 except for routed entity messages (1).
// v12 dispatch: 0x401, 0x400 (unwrap), then 3, 0x3E8, 0x402, 0x403, 0x406, 0x40E.
int gsMP_SERVER::ProcessMsg(int msg, void *data, msgADDR *from, int *ret)
{
    unsigned long sender = MPCLIENT_NONE;   // -3 when the message did not come through the network

    // ---- packed buffer (loop-back / simulated network) -> unpack and re-send as 0x400
    if (msg == GS_MSG_MP_SERVERCLIENT_SIMUL) {          // 0x401 (feb 0x400)
        gsMSG_MP_WRAP w = { MPID_UNDECLARED, -1, MPCLIENT_NONE, NULL };
        conn.UnPack(&w, data, -1);
        w.client = MPCLIENT_LOCAL;
        msgSystem.SendMsg(gsSysServer, GS_MSG_MP_SERVERCLIENT, &w, this);
        return 0;
    }

    // ---- unpacked network message: route to the entity, or handle here (mpid -2)
    if (msg == GS_MSG_MP_SERVERCLIENT) {                // 0x400 (feb 0x3FF)
        gsMSG_MP_WRAP *w = (gsMSG_MP_WRAP *)data;
        sender = w->client;
        if (w->mpid != MPID_WRAP_SYSTEM) {
            entIACTIVE_OBJ *o = gsSysMP->FindMPID(w->mpid, 4);   // feb FindEnt
            if (o) {
                if (w->msg == GS_MSG_MP_UPD_CLIENT) {           // 0x40C (feb 0x40B)
                    o->mpFlags |= 0x100;                        // 1.2: 0x100 (feb 0x10 at +0x88) ?
                    w->msg = GS_MSG_MP_UPD_REGULAR;             // 0x40B (feb 0x40A)
                } else if (w->msg == GS_MSG_MP_UPD_CLIENT2) {   // 1.2: 0x40D, new
                    o->mpFlags |= 0xA00;                        // ? meaning of 0x800|0x200
                    w->msg = GS_MSG_MP_UPD_REGULAR;
                }
                // 1.2: feb's "(0x40C NOTIFY, code -98) -> flag 0x10, code -99" rewrite is gone
                msgSystem.SendMsg(o, w->msg, w->data, this);
            }
            return 1;
        }
        msg  = w->msg;
        data = w->data;
    }

    switch (msg) {

    // ---- an entity is being destroyed
    case GS_MSG_ENT_DESTROY: {                          // 3
        entIACTIVE_OBJ *o = (entIACTIVE_OBJ *)data;
        if ((long)o->ownerClient /*+0x8C*/ >= 0)        // a client's player died for good -> drop that client
            RemoveClient(o->ownerClient);               // vt +0x24
        if (o->mpid == MPID_UNDECLARED) break;
        gsMP_SERVER *srv = gsSysServer;
        gsMP_CLIENT_FILTER_CLIENT2 f(MPCLIENT_LOCAL, MPCLIENT_NONE);   // everyone but the host
        if (srv->nClients == 0) break;
        for (int i = 0; i < srv->nClients; i++) {
            if (f.IsSend(i)) {
                char log[64];
                sprintf(log, "mp_send_%s.log", o->name);        // built, never used (debug leftover)
                srv->SendMsgClient(srv->clients[i].id, o, GS_MSG_ENT_DESTROY, NULL, MPSEND_GUARANTEED);
            }
        }
        return 0;
    }

    case GS_MSG_FRAME:                                  // 0x3E8
        // 1.2: nothing. feb counted down each client's snapshot timer (+0x54) here and
        //      raised +0x4C when it expired; 1.2 schedules snapshots in
        //      gsMP_CLIENT_FILTER_SNAPSHOT::IsSend (v12 0x00516410) from readyFrame/rate/connSpeed.
        break;

    // ---- join: a client asks for its player (GS_MSG_MP_CREATE_PLAYER)
    case GS_MSG_MP_CREATE_PLAYER: {                     // 0x402 (feb 0x401)
        gsMSG_MP_CREATE_ENT *m = (gsMSG_MP_CREATE_ENT *)data;
        entENTITY *pl;

        if (flags & MPSRV_LOCAL) {                      // single player: just declare the local player
            pl = gsSysMP->FindPlrLocal();
            pl->mpid = MPID_UNDECLARED;
            pl->mpFlags = (pl->mpFlags & ~1) | 6;
            if (gsSysMP->mode == 3) {
                pl->mpid = MPID_CLIENTSIDE;
            } else {
                pl->mpFlags |= 6;
                pl->mpid = nextMPID++;
            }
            AddPlayer(pl, pl->ownerClient);             // vt +0x28
            return 0;
        }

        if ((flags & MPSRV_HOSTPLAYER) && sender == MPCLIENT_NONE)
            sender = MPCLIENT_LOCAL;                    // the listen-server host's own join

        m3dV pos, dir;
        gsSysMP->SelectStartPosDir(&pos, &dir, 0);      // 1.2: third argument 0 (? meaning)
        ((m3dMATR *)((char *)m + 0xE0))->_MakeLCS2WCS_VZ(&pos, &dir);   // spawn matrix inside m->create (feb +0xE2)

        if (sender == MPCLIENT_LOCAL && (flags & MPSRV_HOSTPLAYER)) {
            pl = gsSysMP->FindPlrLocal();               // host's player already exists
            pl->mpFlags = (pl->mpFlags & ~1) | 6;
        } else {
            pl = entCreate(gsScenePtr, "player_mp", "player_mp", &m->create, m->user, 5);
        }
        pl->anim->SetName(m->name);                     // +0xC0 animINST
        pl->mpid = MPID_UNDECLARED;
        if (gsSysMP->mode == 3) {
            pl->mpid = MPID_CLIENTSIDE;
        } else {
            pl->mpFlags |= 6;
            pl->mpid = nextMPID++;
        }
        m->mpid = pl->mpid;
        pl->ownerClient = sender;                       // +0x8C
        AddPlayer(pl, sender);                          // vt +0x28

        if (sender != MPCLIENT_LOCAL) {
            // 1) echo CREATE_PLAYER (now with the MPID) to the joining client
            SendMsgClient(pl->ownerClient, NULL, GS_MSG_MP_CREATE_PLAYER, m, MPSEND_GUARANTEED);

            // 2) tell it which level to load
            gsMSG_LOAD_LEVEL lv;
            lv.type = 0; lv.flags = 0x40; lv.size = 0x154;
            lv.d148 = lv.d14C = lv.d150 = 0;
            lv.level[0] = 0; lv.b108 = 0;
            strcpy(lv.level, gsSceneFullName);
            SendMsgClient(sender, NULL, GS_MSG_LOAD_LEVEL /*0x3EC*/, &lv, MPSEND_GUARANTEED);

            // 3) announce the new player entity to every other remote client
            gsMSG_MP_CREATE_ENT ce;                     // ctor v12 0x0050E8C0, size 0x164 (feb set 0x166)
            const char *n = pl->anim->name;
            strcpy(ce.name, n ? n : "");
            strcpy(ce.kind, "player_mp");
            strcpy(ce.className, "player_mp");
            ce.mpid = pl->mpid;
            pl->anim->MakeCreateData(&ce.create);
            if (nClients != 0) {
                gsMP_CLIENT_FILTER_CLIENT2 f(pl->ownerClient, MPCLIENT_LOCAL);  // not the new one, not the host
                for (int i = 0; i < nClients; i++)      // 1.2: inline loop (feb called SendMsgClientAll(filter))
                    if (f.IsSend(i))
                        SendMsgClient(clients[i].id, NULL, GS_MSG_MP_CREATE_ENT /*0x403*/, &ce, MPSEND_GUARANTEED);
            }
            // 4) send it the whole world
            SyncClient(pl);
        }
        const char *n = pl->anim->name;
        pl->name.SetName(n ? n : "");                   // ent +0x40 apNAME
        return 0;
    }

    // ---- create a syncable entity on request
    case GS_MSG_MP_CREATE_ENT:                          // 0x403 (feb 0x402)
        CreateEntitySyncable((gsMSG_MP_CREATE_ENT *)data);
        return 0;

    // ---- a client finished loading/spawning
    case GS_MSG_MP_CLIENT_READY: {                      // 0x406 (feb 0x405)
        int i;
        if (gsSysMP->mode == 1 && sender == MPCLIENT_LOCAL) {
            i = 0;
        } else {
            for (i = 0; i < nClients; i++)
                if (clients[i].id == sender) break;
            if (i >= nClients) return 0;
            if (i < 0) break;
        }
        clients[i].ready      = 1;
        clients[i].readyFrame = gsFrameCounter;         // 1.2: [0x008FACC8] ("?" name: frame counter)
        gsMSG_MP_CLIENT_READY m;
        m.type = 0; m.flags = 0x40; m.size = 0x0C;
        m.mpid  = MPID_UNDECLARED;
        m.ready = 1;
        if (clients[i].player)                          // 1.2: null check
            m.mpid = clients[i].player->mpid;
        SendMsgClientAll(NULL, GS_MSG_MP_CLIENT_READY, &m, MPSEND_GUARANTEED, MPCLIENT_NONE, MPCLIENT_NONE);  // to everyone
        break;
    }

    // ---- notify: only the "my rate" code is handled by the server
    case GS_MSG_MP_NOTIFY: {                            // 0x40E (feb 0x40C)
        gsEV_SYNC *ev = (gsEV_SYNC *)data;
        if (ev->code != MPNOTIFY_RATE) break;           // 1.2: s16 compare, value at +6 (feb: int at +4, value at +8)
        int i;
        if (gsSysMP->mode == 1 && sender == MPCLIENT_LOCAL) {
            i = 0;
        } else {
            for (i = 0; i < nClients; i++)
                if (clients[i].id == sender) break;
            if (i >= nClients) return 0;
            if (i < 0) break;
        }
        clients[i].rate = ev->fRate;                    // rec +0x50
        return 0;
    }
    }
    return 0;
}

// v12 0x00510160  (feb 0x004FD9B0)
// create an entity from a GS_MSG_MP_CREATE_ENT and forward it to the ready remote clients
entENTITY *gsMP_SERVER::CreateEntitySyncable(gsMSG_MP_CREATE_ENT *m)
{
    int createFlags;
    unsigned long owner;
    if ((flags & (MPSRV_LOCAL | MPSRV_HOSTPLAYER)) == 0) {
        createFlags = 4;  owner = MPCLIENT_NONE;
    } else {
        createFlags = 6;  owner = MPCLIENT_LOCAL;
    }
    entENTITY *e = entCreate(gsScenePtr, m->className, m->kind, (int)m->type /*1.2: s16*/,
                             &m->create, m->user, createFlags);
    if (e->mpid == MPID_UNDECLARED) {
        if (gsSysMP->mode == 3) {
            e->mpid = MPID_CLIENTSIDE;
        } else {
            e->mpFlags |= 6;
            e->mpid = nextMPID++;
        }
    }
    m->mpid = e->mpid;
    e->ownerClient = owner;

    gsMP_CLIENT_FILTER_CLIENT2 f(owner, MPCLIENT_NONE);
    if (nClients == 0) {
        if (owner == MPCLIENT_NONE)                     // no local player (dedicated 0 / client mode 0x20): to local client (-1)
            SendMsgClient(MPCLIENT_LOCAL, NULL, GS_MSG_MP_CREATE_ENT /*0x403*/, m, MPSEND_GUARANTEED | MPSEND_ONLYREADY);
        return e;
    }
    for (int i = 0; i < nClients; i++)
        if (f.IsSend(i) && clients[i].ready)
            SendMsgClient(clients[i].id, NULL, GS_MSG_MP_CREATE_ENT, m, 0xA0000);
    return e;
}


// ===========================================================================
// lifetime
// ===========================================================================

// v12 0x005102C0  (feb 0x004FDB10)   vtable slot +0x10
int gsMP_SERVER::Init()
{
    *(unsigned long *)((char *)this + 0x0C) |= 2;      // msgADDR flags: receive messages ?
    nextMPID = 0;
    return 1;
}

// v12 0x005102E0  (feb 0x004FDB40)   vtable slot +0x18
// start of a game: store mode flags, reset MPIDs, un-declare previously synced entities,
// 1.2: read [CONFIG] ConnSpeed
int gsMP_SERVER::InitGame(int initFlags)
{
    flags = initFlags;
    if (initFlags & MPSRV_LOCAL)
        flags = initFlags | MPSRV_HOSTPLAYER;
    nextMPID = 0;
    for (entIACTIVE_OBJ *o = entFindNextIO(NULL); o; o = entFindNextIO(o))
        if (gsSysMP->IsRegularMPID(o->mpid))
            o->mpid = MPID_CLIENTSIDE;

    // 1.2: connection speed. The value is read into the argument's stack slot: when the key
    //      is missing it stays = initFlags (Single 1 -> 2.0, Server 2 -> 4.0, dedicated 0 -> 1.0,
    //      Client 0x20 -> 1.0).
    int nConnSpeed /* = initFlags, see above */;
    _apCfgReadInt(NULL, "CONFIG", "ConnSpeed", &nConnSpeed);     // strings v12 0x00631900 / 0x006318F4
    connSpeed = gsSysMP->ConnSpeedScale(nConnSpeed);           // v12 0x0050E5E0: 0->1.0, 1->2.0, 2->4.0, else 1.0
    return 1;
}

// v12 0x00510380  (feb 0x004FDBB0)   vtable slot +0x1C
// end of game: drop every client, then stop the DirectPlay host
void gsMP_SERVER::TermGame()
{
    while (nClients > 0)
        RemoveClient(clients[0].id);                    // vt +0x24
    gsSysNetwork->Server_Stop();                        // tail jump
}


// ===========================================================================
// sending
// ===========================================================================

// v12 0x005103C0  (feb 0x004FDBF0)
// send to the client that owns ent (single player: deliver directly)
void gsMP_SERVER::SendMsgClient(entENTITY *ent, int msg, msgDATA *data, int sendFlags)
{
    if (ent == NULL) return;
    if (gsSysMP->mode == 1) {
        msgSystem.SendMsg(ent, msg, data, NULL);        // tail jump
        return;
    }
    if (ent->ownerClient != MPCLIENT_NONE)
        SendMsgClient(ent->ownerClient, ent, msg, data, sendFlags);   // vt +0x2C
}

// v12 0x00510420  (feb 0x004FDC50)   vtable slot +0x2C
// the one place every server->client message goes through. ent = target entity on
// the client (its MPID goes into the wrap) or NULL for the client system.
void gsMP_SERVER::SendMsgClient(unsigned long id, entIACTIVE_OBJ *ent, int msg, msgDATA *data, int sendFlags)
{
    gsMSG_MP_WRAP w = { MPID_UNDECLARED, -1, MPCLIENT_NONE, NULL };
    if (id == MPCLIENT_NONE) return;
    PrepareMsgWrap(&w, ent, msg, data);                 // gsMP_COMM

    if (id == MPCLIENT_LOCAL) {                         // the host's own gsMP_CLIENT
        if (!(flags & MPSRV_SIMUL)) {
            msgSystem.SendMsg(gsSysClient, GS_MSG_MP_SERVERCLIENT /*0x400, feb 0x3FF*/, &w, this);
            return;
        }
        conn.Pack(&w);
        msgSystem.SendMsg(gsSysClient, GS_MSG_MP_SERVERCLIENT_SIMUL /*0x401, feb 0x400*/, conn.GetBuffer(), NULL);
        return;
    }
    if (id == MPCLIENT_DEMO) {
        msgSystem.SendMsg(gsDemoClient, GS_MSG_MP_SERVERCLIENT, &w, this);
        return;
    }
    // remote client
    PrepareConn(&w);                                    // packs into conn
    conn.GetBufferSize();
    if (ent)
        gsSysNetwork->stat.AddTraffic(ent->classId, conn.GetBufferSize());   // +0xC4: per-class traffic statistics
    else
        gsSysNetwork->stat.AddTraffic(0, conn.GetBufferSize());
    gsSysNetwork->Server_SendDataToClient(id, conn.GetBuffer(), conn.GetBufferSize(), sendFlags);
}

// v12 0x00510580  (feb 0x004FDDB0)
// send to every client except skip1/skip2. With no client connected and no exclusions,
// the message goes to the local client (-1).
// (wr.py mislabels this as m3dMATR::RemoveScale; ~35 callers in weapons/player/AI code)
void gsMP_SERVER::SendMsgClientAll(entIACTIVE_OBJ *ent, int msg, msgDATA *data, int sendFlags,
                                   unsigned long skip1, unsigned long skip2)
{
    gsMP_CLIENT_FILTER_CLIENT2 f(skip1, skip2);
    if (nClients == 0) {
        if (skip1 == MPCLIENT_NONE && skip2 == MPCLIENT_NONE)
            SendMsgClient(MPCLIENT_LOCAL, ent, msg, data, sendFlags);
        return;
    }
    for (int i = 0; i < nClients; i++) {
        if (!f.IsSend(i)) continue;
        if (!clients[i].ready && (sendFlags & 0x90000)) continue;   // "only ready clients"
        char log[64];
        if (ent) sprintf(log, "mp_send_%s.log", ent->name);       // debug leftover, unused
        SendMsgClient(clients[i].id, ent, msg, data, sendFlags);
    }
}

// v12 0x00510660  (feb 0x004FDE90)
// same with an arbitrary filter (NULL = all). Caller in v12: 0x00514C09 (snapshot sender, gs_msg.cpp ?)
void gsMP_SERVER::SendMsgClientAll(entIACTIVE_OBJ *ent, int msg, msgDATA *data, int sendFlags,
                                   gsMP_CLIENT_FILTER *filter)
{
    for (int i = 0; i < nClients; i++) {
        if (filter && !filter->IsSend(i)) continue;
        if (!clients[i].ready && (sendFlags & 0x90000)) continue;
        char log[64];
        if (ent) sprintf(log, "mp_send_%s.log", ent->name);
        SendMsgClient(clients[i].id, ent, msg, data, sendFlags);
    }
}

// v12 0x005106F0  (feb 0x004FDF20)
// data from a remote client (gsMP_NETWORK OnNetDrvEvent ev 5, call at v12 0x0052FD57):
// unpack and post to ourselves as GS_MSG_MP_SERVERCLIENT with the sender id
void gsMP_SERVER::ReceiveMsg(unsigned long client, void *buf, int size)
{
    gsMSG_MP_WRAP w = { MPID_UNDECLARED, -1, MPCLIENT_NONE, NULL };
    conn.UnPack(&w, buf, size);
    w.client = client;
    msgSystem.SendMsg(gsSysServer, GS_MSG_MP_SERVERCLIENT /*0x400, feb 0x3FF*/, &w, this);
}

// v12 0x00510760  (feb 0x004FDF90)   gsMP_CLIENT_FILTER_CLIENT2 vtable (0x00607AF4) slot +0x00
// send to record i unless it is one of the two excluded ids
int gsMP_CLIENT_FILTER_CLIENT2::IsSend(int i)
{
    unsigned long id = gsSysServer->clients[i].id;
    if (id != skip1 && id != skip2)
        return 1;
    return 0;
}
