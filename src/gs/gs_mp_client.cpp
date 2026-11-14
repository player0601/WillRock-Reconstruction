// gs_mp_client.cpp -- Will Rock 1.2 (v12.exe) multiplayer: game-level client (gsMP_CLIENT)
//
// Reconstructed source (documentation for modders). Original object: gs:gs_mp_client.obj.
// Every function: "// v12 <addr>  (feb <addr>)". 1.2 changes against the Feb 2003 build: "// 1.2:".
// Message ids are v12 values; see mp_messages.h for the feb values.
//
// Role: the client half of the game protocol. It
//   - sends entity messages to the server (SendMsgServer -> gsMSG_MP_WRAP -> gsMP_CONNECTION -> DirectPlay),
//   - receives packets from the driver (ReceiveMsg, called through gsMP_NETWORK::OnNetDrvEvent ev 8),
//   - dispatches unwrapped messages to entities by MPID (ProcessMsg / GS_MSG_MP_SERVERCLIENT),
//   - creates the local player and replicated entities (CREATE_PLAYER / CREATE_ENT / SYNC),
//   - watches the connection every frame and quits to the menu with UI_ERR_SERVER_LOST.

#include "gs_mp_client.h"
#include "mp_messages.h"
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

// ---------------------------------------------------------------------------------------------
// v12 0x00539100  (feb 0x00523730)
// Constructor.
gsMP_CLIENT::gsMP_CLIENT()
    // msgADDR part: +4,+8,+0xC = 0; +0x10 = 'RDAM'; +0x14..+0x24 = 0; byte +0x28 = 0
    // conn (+0x68) constructed: gsMP_CONNECTION::gsMP_CONNECTION (v12 0x0053FB30)
{
    flags = 0;
    memset(players, 0, sizeof(players));      // 0x20 dwords
    // vtable = gsMP_CLIENT::`vftable' (v12 0x006084EC)
    connSpeedRate = 1.0f;                     // 1.2: new (feb had no such field)
    serverRate    = 1.0f;                     // 1.2: new
    /* msgADDR +0x1C */ // = 10;              // ? priority
    localClientId = -3;
    localPlayer   = NULL;
    nPlayers      = 0;
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053F4C0  (feb 0x00529870)  -- vtable slot +0x00, scalar deleting destructor
gsMP_CLIENT::~gsMP_CLIENT()
{
    // conn.~gsMP_CONNECTION() (v12 0x0053FBE0); msgADDR::~msgADDR (v12 0x004CFCC0);
    // operator delete(this) if (flags & 1).
}

// ---------------------------------------------------------------------------------------------
// v12 0x00539190  (feb 0x005237B0)  -- slot +0x18
// Starts a session. 'initFlags' = MPCL_* from gsMP_SYSTEM::InitGame (1/2: 1|2, 2|4?, 3: 0x10 or 0x14, 4: 2).
int gsMP_CLIENT::InitGame(int initFlags)
{
    addrFlags |= 2;                                        // msgADDR +0x0C
    flags = initFlags;
    if (initFlags & MPCL_LOCAL)
        flags = initFlags | MPCL_SERVER_HERE;
    if (flags & MPCL_SERVER_HERE)
        localClientId = -1;

    // 1.2: feb set sendInterval(+0x47C) = 0.05f and sendTimer(+0x480) = 0 here.
    // 1.2: read the connection speed preset instead.
    int connSpeed = initFlags;                             // the compiler reuses the arg slot; ReadInt leaves it
                                                           // unchanged when the key is missing  // ? (then the default is initFlags)
    _apCfgReadInt(NULL, "CONFIG", "ConnSpeed", &connSpeed); // [integrator] 4-arg signature (cfg = NULL)      // v12 0x004AF7E0 (fastcall, ecx=0); strings 0x00631900 / 0x006318F4
    connSpeedRate = gsSysMP->ConnSpeedScale(connSpeed);    // v12 0x0050E5E0: 0 -> 1.0, 1 -> 2.0, 2 -> 4.0, else 1.0

    localPlayer = NULL;
    iterPlayer  = 0;
    nPlayers    = 0;
    flags |= MPCL_INIT_MAP;                                // 1.2: new flag 0x40
    return 1;
}

// ---------------------------------------------------------------------------------------------
// v12 0x00539220  (feb 0x00523820)  -- slot +0x1C
// Ends the session and drops the DirectPlay connection.
void gsMP_CLIENT::TermGame()
{
    flags &= ~MPCL_NETWORKED;
    gsSysNetwork->Client_DisconnectFromGame();             // v12 0x0052F8C0
    localPlayer = NULL;
}

// ---------------------------------------------------------------------------------------------
// v12 0x00539250  (feb 0x00523850)  -- slot +0x20
// Registers a player entity on this client (NULL = the local player). Max 16.
int gsMP_CLIENT::AddClient(entENTITY* ent)
{
    if (nPlayers >= GS_MP_MAX_LOCAL_PLAYERS)               // 0x10
        return 0;
    if (ent == NULL) {
        players[nPlayers].ent    = gsSysMP->FindPlrLocal(); // v12 0x0050E700
        players[nPlayers].active = 1;
    } else {
        players[nPlayers].ent    = ent;
        players[nPlayers].active = 0;
    }
    nPlayers++;
    return 1;
}

// ---------------------------------------------------------------------------------------------
// v12 0x005392E0  (feb 0x005238E0)  -- slot +0x24
// Removes a player entity (array compaction with memmove).
void gsMP_CLIENT::RemoveClient(entENTITY* ent)
{
    int i;
    for (i = 0; i < nPlayers; i++)
        if (players[i].ent == ent)
            break;
    if (i >= nPlayers)
        return;
    if (i < nPlayers - 1)
        memmove(&players[i], &players[i + 1], (nPlayers - i) * 8 - 8);
    nPlayers--;
}

// ---------------------------------------------------------------------------------------------
// v12 0x00539350  (feb 0x00523950)  -- slot +0x28
// Marks the player with 'mpid' active and tells its entity GS_MSG_MP_CLIENT_READY.
// If the player is unknown, looks the entity up by MPID, AddClient()s it and retries once.
void gsMP_CLIENT::ActivateClient(unsigned long mpid)
{
    bool retried = false;
    for (;;) {
        int i;
        for (i = 0; i < nPlayers; i++)
            if (players[i].ent->mpid == mpid)              // 1.2: entity +0x88 (feb +0x8C)
                break;

        if (i < nPlayers) {
            players[i].active = 1;
            gsMSG_MP_CLIENT_READY m;                       // {u8 0, u8 0x40, u16 size 0x0C, int mpid, int ready}
            m.hdr.size = 0x0C;
            m.mpid     = mpid;
            m.ready    = 1;
            msgSystem.SendMsg(players[i].ent, GS_MSG_MP_CLIENT_READY, &m, NULL);   // 1.2: 0x406 (feb 0x405)
            return;
        }
        if (retried)
            return;
        entIACTIVE_OBJ* e = gsSysMP->FindMPID(mpid, 0);    // v12 0x0050E6B0
        if (e == NULL || !AddClient((entENTITY*)e))        // virtual slot +0x20
            return;
        retried = true;
    }
}

// ---------------------------------------------------------------------------------------------
// v12 0x00539400  (feb 0x00523A00)
// Iterates this client's players: GetNextPlayer(NULL) = first.
entENTITY* gsMP_CLIENT::GetNextPlayer(entENTITY* prev)
{
    if (flags & MPCL_LOCAL)
        return prev ? NULL : players[0].ent;

    if (prev == NULL) {
        iterPlayer = 0;
        return players[iterPlayer].ent;
    }
    // 1.2: if the cursor does not point at 'prev' (list changed while iterating),
    //      re-find 'prev' by a linear search; feb trusted the cursor blindly.
    if (players[iterPlayer].ent != prev) {
        int i;
        for (i = 0; i < nPlayers; i++)
            if (players[i].ent == prev) { iterPlayer = i; break; }
        if (i == nPlayers)
            return NULL;
    }
    if (iterPlayer >= nPlayers)
        return NULL;
    if (++iterPlayer >= nPlayers)
        return NULL;
    return players[iterPlayer].ent;
}

// ---------------------------------------------------------------------------------------------
// v12 0x005394B0  (feb 0x00523A60)
// Number of players: 1 in Single mode, else nPlayers.
int gsMP_CLIENT::GetNPlayer()
{
    if (gsSysMP->mode == 1)
        return 1;
    return nPlayers;
}

// ---------------------------------------------------------------------------------------------
// v12 0x005394D0  (feb 0x00523A80)  -- slot +0x2C
// Sends 'msg' for entity 'to' (NULL = to the server's MP system object) to the server.
// netFlags: 0x20000 = guaranteed, 0x40000 = (non-guaranteed with timeout), see gsMP_NETWORK.
void gsMP_CLIENT::SendMsgServer(entIACTIVE_OBJ* to, int msg, msgDATA* data, int netFlags)
{
    gsMSG_MP_WRAP wrap = { -1, -1, MPCLIENT_UNSET, NULL };

    if (to != NULL && (flags & MPCL_LOCAL)) {
        msgSystem.SendMsg(to, msg, data, this);            // single player: direct delivery
        return;
    }

    PrepareMsgWrap(&wrap, to, msg, data);                  // v12 0x0053FD10
    wrap.client = -1;

    if (flags & MPCL_SERVER_HERE) {
        if (!(flags & MPCL_SIMUL_PACK)) {
            msgSystem.SendMsg(gsSysServer, GS_MSG_MP_SERVERCLIENT, &wrap, NULL);       // 1.2: 0x400 (feb 0x3FF)
        } else {
            conn.Pack(&wrap);                                                          // v12 0x0053FC10
            msgSystem.SendMsg(gsSysServer, GS_MSG_MP_SERVERCLIENT_SIMUL, conn.GetBuffer(), NULL); // 1.2: 0x401 (feb 0x400)
        }
    }
    else if (flags & MPCL_DEMO) {
        msgSystem.SendMsg(gsDemoServer, GS_MSG_MP_SERVERCLIENT, &wrap, NULL);          // 1.2: 0x400
    }
    else {
        PrepareConn(&wrap);                                                            // v12 0x0053FD40
        gsSysNetwork->stat.AddTraffic(to ? to->classId : 0, conn.GetBufferSize());     // gsMP_STAT at +0xC4, v12 0x0052EAD0
        gsSysNetwork->Client_SendDataToServer(conn.GetBuffer(), conn.GetBufferSize(), netFlags); // v12 0x0052F910
    }

    if (gsAppState & 0x400)                                // recording a demo
        gsDemo.Record_SaveMsg(&wrap);                      // v12 0x0053F4F0
}

// ---------------------------------------------------------------------------------------------
// v12 0x00539650  (feb 0x00523C00 "_$E2")
// Static initializer of this .obj: [0x009A5B00] = 1e-6f * 1e-6f (constant at v12 0x006084E8).
// ? user of this value not traced

// ---------------------------------------------------------------------------------------------
// v12 0x00539670  (feb 0x00523C20)  -- slot +0x04
// Message handler of the client object. Returns 0 always.
int gsMP_CLIENT::ProcessMsg(int msg, void* data, msgADDR* from, int* res)
{
    // ---- raw packet in loopback/simulated mode
    if (msg == GS_MSG_MP_SERVERCLIENT_SIMUL) {             // 1.2: 0x401 (feb 0x400)
        ReceiveMsg(data, -1);                              // virtual slot +0x30
        return 0;
    }

    // ---- an unwrapped server->client message
    if (msg == GS_MSG_MP_SERVERCLIENT) {                   // 1.2: 0x400 (feb 0x3FF)
        gsMSG_MP_WRAP* w = (gsMSG_MP_WRAP*)data;
        if (w->mpid != MPID_SYSTEM) {
            entIACTIVE_OBJ* e = gsSysMP->FindMPID(w->mpid, 2);         // v12 0x0050E6B0
            if (e == NULL)
                return 0;
            // 1.2: the two new position-update variants are folded into UPD_REGULAR
            //      and the entity is marked as having received one.
            if (w->msg == GS_MSG_MP_UPD_PACK_A || w->msg == GS_MSG_MP_UPD_PACK_B) {   // 0x40C / 0x40D, new in 1.2
                e->mpFlags |= 0x800;                                     // entity +0x84
                w->msg = GS_MSG_MP_UPD_REGULAR;                          // 0x40B
            }
            // id 2 to our own player is not echoed back to it
            if (e != localPlayer || w->msg != 2)
                msgSystem.SendMsg(e, w->msg, w->data, this);
            char logName[64];
            _snprintf(logName, sizeof(logName) - 1, "mp_recv_%s.log", e->name.str);   // dead debug code: the result is
            logName[sizeof(logName) - 1] = 0;  // never used (v12: sprintf into the same 0x40 bytes, the name at +0x40)
            return 0;
        }
        // message for the client object itself: unwrap and fall through
        msg  = w->msg;
        data = w->data;
    }

    switch (msg) {

    case 3:                                                // entity is being deleted (msgADDR "destroy")  // ? id 3 name
    {
        int i;
        for (i = 0; i < nPlayers; i++)
            if (players[i].ent == (entENTITY*)data)
                break;
        if (i < nPlayers)
            RemoveClient((entENTITY*)data);                // slot +0x24
        return 0;
    }

    case GS_MSG_FRAME:                                     // 1000, every frame
        // 1.2: feb ran the client send timer here (timer -= gsElapsedTime; sendNow = timer < 0;
        //      reload with 0.05 s). Removed in 1.2: rate control moved to the server filters.
        if ((flags & MPCL_NETWORKED) &&
            (!gsSysNetwork->Client_IsJoined() || gsSysNetwork->Client_IsError()))   // v12 0x0052F800 / 0x0052F830
        {
            // ---- connection to the server lost
            gspSysPlay->StopGame();                        // v12 0x005346A0
            TermGame();                                    // slot +0x1C
            if (!(gsAppState & 0x2000)) {                  // no UI (e.g. auto-started): just quit
                gsVideoTermHW();                           // v12 0x00533E50
                gsMsgStopLoop();                           // v12 0x00510980
                exit(0);
            }
            uiSystem->SetActiveWindow(1, "null");          // ui vtbl +0x3C (feb +0x38)  // ? method name
            uiSystem->MessageBox(gsStrings->GetStringById(gsStrings->GetStringId("UI_ERROR")),
                                 gsStrings->GetStringById(gsStrings->GetStringId("UI_ERR_SERVER_LOST")),
                                 0, 0, 0, "main");         // ui vtbl +0x18
        }
        return 0;

    case GS_MSG_LOAD_LEVEL:                                // 0x3EC from the server: map (re)load
        if (gsAppState & 0x400000) {                       // "waiting for level"
            // delay the actual load by 0.5 s so the UI can show the loading screen
            msgSystem.PostTimeMsgData(this, GS_MSG_MP_DELAYED_LOAD, (msgDATA*)data, NULL, 0.5f);  // 1.2: 0x414 (feb 0x412)
            uiSystem->ShowLoading((char*)data + 4);        // ui vtbl +0x98 (feb +0x94)  // ? method name
            gsSysNetwork->Drv_Freeze(1);                   // v12 0x0052F260
        }
        return 0;

    case GS_MSG_MP_CREATE_PLAYER:                          // 1.2: 0x402 (feb 0x401) server's reply: "this is you"
    {
        gsMSG_MP_CREATE_ENT* c = (gsMSG_MP_CREATE_ENT*)data;
        entENTITY* p = gsSysMP->FindPlrLocal();
        localPlayer = p;
        p->mpFlags  = (p->mpFlags & ~5) | 2;
        p->mpid     = c->mpid;                             // +0x08
        p->anim->ApplyCreateData(&c->create);              // 1.2: payload +0xCE (feb +0xD0)

        // tell the player entity its spawn point
        gsMSG_MP_NOTIFY_POS n;                             // {hdr size 0x1E, s16 sub = -97, m3dV origin, m3dV dir}
        n.hdr.size = 0x1E;                                 // 1.2: 0x1E (feb 0x20, sub was int -95)
        n.sub      = -97;                                  // 1.2: 0xFF9F (feb -95 = 0xFFFFFFA1)
        n.origin   = m3dVUndef;
        n.dir      = m3dVUndef;
        c->create.matr.GetOrigin(&n.origin);                      // 1.2: payload +0xE0 (feb +0xE2)
        c->create.matr.GetAxisZ(&n.dir);
        msgSystem.SendMsg(p, GS_MSG_MP_NOTIFY, &n, NULL);  // 1.2: 0x40E (feb 0x40C)
        return 0;
    }

    case GS_MSG_MP_CREATE_ENT:                             // 1.2: 0x403 (feb 0x402) another player / replicated entity
    {
        gsMSG_MP_CREATE_ENT* c = (gsMSG_MP_CREATE_ENT*)data;
        entENTITY* e = entCreate(gsScenePtr, c->className, c->templ, c->type,   // 1.2: type is s16 at +0xCC (feb int)
                                 &c->create, c->user, 3);                       // v12 0x00508450
        e->mpid        = c->mpid;
        e->ownerClient = localClientId;
        // 1.2: new -- give the instance its network name and copy it to the entity name
        e->anim->SetName(c->name);                         // payload +0x0C, v12 0x004CD500
        e->name.SetName(e->anim->name ? e->anim->name : ""); // anim +0x18; apNAME::SetName v12 0x004AF580
        c->create.animSeed = e->anim->seed;                       // payload +0x160 (feb +0x162) <- anim +0x134  // ? field meaning
        e->anim->ApplyCreateData(&c->create);
        e->anim->AbvTerm();
        e->anim->AbvInit();
        _scnCalcRadRenderOff(e->anim);
        return 0;
    }

    case GS_MSG_MP_SYNC:                                   // 1.2: 0x404 (feb 0x403)
    {
        gsMSG_MP_SYNC* s = (gsMSG_MP_SYNC*)data;
        if (s->classId != -2) {
            Synchronize(s);
            return 0;
        }
        // ---- classId -2: end of the initial sync: we are in the game
        gsMSG_MP_CLIENT_READY r;                           // {hdr size 0x0C, int mpid, int 1}
        r.hdr.size = 0x0C;
        r.mpid     = localPlayer->mpid;
        r.ready    = 1;
        SendMsgServer(NULL, GS_MSG_MP_CLIENT_READY, &r, 0x20000);       // 1.2: 0x406 (feb 0x405), guaranteed

        gsMSG_MP_NOTIFY_RATE q;                            // {hdr size 0x0A, s16 sub = -200, float rate}
        q.hdr.size = 0x0A;
        q.sub      = -200;
        q.rate     = connSpeedRate;                        // 1.2: ConnSpeed preset (feb sent fixed 0.05f)
        SendMsgServer(NULL, GS_MSG_MP_NOTIFY, &q, 0x20000);              // 1.2: 0x40E (feb 0x40C)

        uiSystem->HideLoading(0);                          // ui vtbl +0x9C (feb +0x98)  // ? method name
        if (gsAppState & 0x4000) {
            gsMsgChangeMode(3);                            // v12 0x00510D50
            gsAppState &= ~0x4000;
        }
        gsAppState &= ~0x400000;                           // no longer waiting for level
        msgSystem.SendAll(GS_MSG_MP_PLAYER_ENTER, localPlayer, NULL);    // 1.2: 0x408 (feb 0x407)
        return 0;
    }

    case GS_MSG_MP_CLIENT_READY:                           // 1.2: 0x406 (feb 0x405)
        ActivateClient(((gsMSG_MP_CLIENT_READY*)data)->mpid);            // slot +0x28
        return 0;

    // ---- v12 jump table 0x00539D08 / index table 0x00539D20 (msg - 0x40A)
    case GS_MSG_MP_SHOW_MSG:                               // 1.2: 0x40A (feb 0x409)
        ShowMPMsg((gsMSG_MP_SHOW_MSG*)data);               // slot +0x34
        break;

    case GS_MSG_MP_NOTIFY:                                 // 0x40E -- 1.2: new on the client side
        if (((gsMSG_MP_NOTIFY_RATE*)data)->sub == -200)    // 0xFF38
            serverRate = ((gsMSG_MP_NOTIFY_RATE*)data)->rate;            // +0x278C
        break;

    case GS_MSG_MP_NEW_MAP:                                // 0x411 server changes map (StartNewGameMap)
        gsAppState |= 0x400000;                            // waiting for level
        flags |= MPCL_INIT_MAP;                            // 1.2: re-send INIT_MAP after the reload
        if (gsAppState & 0x4000) {
            gsMsgChangeMode(3);
            gsAppState &= ~0x4000;
        }
        break;

    case GS_MSG_MP_LEVEL_RESTART:                          // 0x412  // ? exact meaning; feb had the same body for 0x40F/0x410
        gsAppState |= 0x400000;
        if (gsAppState & 0x4000) {
            gsMsgChangeMode(3);
            gsAppState &= ~0x4000;
        }
        break;

    case GS_MSG_MP_DELAYED_LOAD:                           // 1.2: 0x414 (feb 0x412), posted by LOAD_LEVEL above
        if (gsAppState & 0x400000) {
            gsSysNetwork->Drv_Freeze(0);
            gsMSG_MP_CLIENT_READY off;                     // {hdr size 0x0C, mpid -1, ready 0}
            off.hdr.size = 0x0C; off.mpid = -1; off.ready = 0;
            for (int i = 0; i < nPlayers; i++) {
                msgSystem.SendMsg(players[i].ent, GS_MSG_MP_CLIENT_READY, &off, NULL);  // deactivate
                players[i].active = 0;
            }
            msgSystem.SendMsg(gsSysLevel, GS_MSG_LOAD_LEVEL, data, NULL);
            if (flags & MPCL_INIT_MAP) {                   // 1.2: INIT_MAP only once per new map
                msgSystem.SendAll(GS_MSG_INIT_MAP, NULL, NULL);
                flags &= ~MPCL_INIT_MAP;
            }
            gsAppState |= 4;                               // game running
        }
        return 0;
    }
    return 0;
}

// ---------------------------------------------------------------------------------------------
// v12 0x00539D30  (feb 0x00524270)  -- slot +0x30
// A packet from the server (driver event 8): unpack and dispatch through ProcessMsg.
void gsMP_CLIENT::ReceiveMsg(void* packet, int size)
{
    gsMSG_MP_WRAP wrap = { -1, -1, MPCLIENT_UNSET, NULL };
    conn.UnPack(&wrap, packet, size);                      // v12 0x0053FC80
    msgSystem.SendMsg(gsSysClient, GS_MSG_MP_SERVERCLIENT, &wrap, this);    // 1.2: 0x400 (feb 0x3FF)
}

// ---------------------------------------------------------------------------------------------
// v12 0x00539D90  (feb 0x005242D0)
// Applies one GS_MSG_MP_SYNC record: find or create the entity it describes and bind its MPID.
entIACTIVE_OBJ* gsMP_CLIENT::Synchronize(gsMSG_MP_SYNC* s)
{
    // classId -1: start of a sync: every entity not bound to the network gets message 2 (delete)
    if (s->classId == -1) {
        for (entENTITY* e = entFindNext(NULL); e; e = entFindNext(e))       // v12 0x005086A0
            if (e->mpid == -3)
                msgSystem.PostMsg(e, 2, NULL);
        return NULL;
    }

    // already known by MPID with the same class and name? just move it
    entIACTIVE_OBJ* e = gsSysMP->FindMPID(s->mpid, 0);
    if (e && e->classId == s->classId && strcmp(s->name, e->name) == 0) {   // 1.2: s+0x9C (feb +0x98), e+0x40 (feb +0x44)
        if (e->magic == '$TNE') {
            ((entENTITY*)e)->anim->Transform(&s->createData.matr, 0);                   // 1.2: s+0xF2 (feb +0xEE)
            if (e->magic == '$TNE')
                e->OnMPSynchronized();                                       // vtbl +0x58 (feb +0x60)
        }
        return e;
    }

    entENTITY* found   = NULL;     // the entity
    entENTITY* withAnim = NULL;    // same, if it is an entENTITY with an anim
    if (s->create) {               // +0x10: create a new entity
        found = entCreate(gsScenePtr, s->className, s->templ, s->type, &s->createData, s->user, 3);
                                   // 1.2: className +0x5C, templ +0x1C, type +0xDC, createData +0xE0
                                   //      (feb +0x58, +0x18, +0xD8, +0xDC)
        if (found == NULL)
            return NULL;
        found->anim->ApplyCreateData(&s->createData);
        found->anim->AbvTerm();
        if (s->create)
            found->anim->Transform(&m3dMatrIdentity, 0);
        found->anim->AbvInit();
        if (s->create)
            found->anim->Transform(&s->createData.matr, 0);
        _scnCalcRadRenderOff(found->anim);
        found->ownerClient = localClientId;
        withAnim = found;
    } else {                       // bind an existing level entity not yet networked (mpid -3)
        for (found = (entENTITY*)entFindNextIO(NULL); found; found = (entENTITY*)entFindNextIO(found))   // v12 0x0050C340
            if (found->classId == s->classId && found->mpid == -3 &&
                found->IdentifyMPSysSyncData(s))                              // vtbl +0x18
                break;
        if (found == NULL)
            return NULL;
        if (found->magic == '$TNE' && found->anim && !(found->anim->flags & 0x40)) {  // anim +0x04
            s->createData.animSeed = found->anim->seed;               // 1.2: s+0x172 (feb +0x16E) <- anim +0x134
            found->anim->ApplyCreateData(&s->createData);
        }
        if (found->magic == '$TNE')
            withAnim = found;
    }

    found->mpid        = s->mpid;
    found->ownerClient = localClientId;
    if (withAnim && withAnim->anim) {
        animINST* a = withAnim->anim;
        if (a->link)                                       // anim +0xE8
            a->link->value = a->linkValue;                 // (+0xE8)->+8 = anim +0xE0   // ? meaning
        withAnim->OnMPSynchronized();                      // vtbl +0x58
    }
    // 1.2: state is now also copied in network games, but only for catapults
    if (gsSysMP->mode == 1 || strstr(s->name, "katapult"))     // string v12 0x0062B730
        found->state = s->state;                           // s+0x14 -> ent +0x30
    if (gsSysMP->mode == 1)
        found->field80 = s->extra;                         // 1.2: new, s+0x18 -> ent +0x80   // ? meaning
    found->mpFlags |= 3;
    return found;
}

// ---------------------------------------------------------------------------------------------
// v12 0x0053A050  (feb 0x00524560)  -- slot +0x34
// Kill message on the HUD: "GAME_PLAYER_KILLED" or "GAME_PLAYER_KILLED_<weapon+1>".
void gsMP_CLIENT::ShowMPMsg(gsMSG_MP_SHOW_MSG* m)
{
    if (m->type != 0)                                      // +0x88: only type 0 (kill) is handled
        return;
    int id;
    if (m->weapon < 0 || m->killer[0] == 0)                // +0x84, +0x04
        id = gsStrings->GetStringIdFmt("GAME_PLAYER_KILLED");                    // v12 0x00636778
    else
        id = gsStrings->GetStringIdFmt("GAME_PLAYER_KILLED_%d", m->weapon + 1);  // v12 0x0063678C
    uiSystem->ShowHudMessage(                              // ui vtbl +0x24 (feb +0x20)  // ? method name
        gsStrings->Printf(id, m->victim, m->killer), 3.0f, 1);   // [integrator] parenthesis fixed: 3.0 s / prio 1 are ShowMsgW args   // victim +0x44, killer +0x04; 3.0 s, 1
}

// =============================================================================================
// Client filters (original objects: gs_mp_server.obj / gs_mp_misc.obj)

// ---------------------------------------------------------------------------------------------
// v12 0x00510760  (feb 0x004FDF90)
// Send to the client in 'slot' unless it is one of the two excluded client ids.
int gsMP_CLIENT_FILTER_CLIENT2::IsSend(int slot)
{
    int id = gsSysServer->clients[slot].id;                // server +0x2784 + slot*0x54 (feb +0x474, stride 0x58)
    if (id != exclude1 && id != exclude2)
        return 1;
    return 0;
}

// ---------------------------------------------------------------------------------------------
// v12 0x00516410  (feb 0x004FC310)
// Snapshot rate filter.
// feb:  return CLIENT2::IsSend(slot) && clients[slot].wantSnapshot (+0x4C).
// 1.2:  rewritten: per-entity, per-client rate limiting.
int gsMP_CLIENT_FILTER_SNAPSHOT::IsSend(int slot)
{
    if (!gsMP_CLIENT_FILTER_CLIENT2::IsSend(slot))
        return 0;
    gsMP_CLIENT_REC& c = gsSysServer->clients[slot];
    if (c.readyFrame /* +0x4C */ == *(int*)0x008FACC8)      // ? msgSYSTEM field +0x368 (written by gsMP_SERVER::ProcessMsg 0x005100E6)
        return 1;
    float rate = gsSysServer->connSpeed;               // server +0x2CD0, from [CONFIG] ConnSpeed
    if (c.rate /* +0x50, client's NOTIFY -200 value */ > rate)
        rate = c.rate;                                     // max(server, client)
    return ent->IsTimeToSend(rate);                        // v12 0x0050A4B0 (on ent at +0x0C)
    // 0x0050A4B0: uses ent +0x90 (base interval, clamp <= 1.0 after * rate) and +0x94 (accumulator
    // advanced by gsElapsedTime, also by 0x0050A540); returns (accumulated ticks > previous)  // ? exact math
}
