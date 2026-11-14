// test_ents.cpp -- (test) hosts the rebuilt game entities (gs_shared doorDOOR, bttnBUTTON) in a loaded level.
//
// PLACEHOLDER (test side only) for what is not rebuilt yet:
//   - gsLVL_SYSTEM::ObjLoadNotify: here only SetExtraObjFlags (the real one, gs_level_init.cpp) on every object of
//     the level and of its templates; "&bb" objects are skipped (objMOD_BILLBOARD is not rebuilt; the test draws
//     billboards itself);
//   - the level loader finishing the instances read from the .lg (template, own object tree), done the way
//     scnSCENE::CreateInst does it;
//   - entCreate / entRegister and msgSYSTEM: the entity is made with its class's Create and the messages go
//     straight to ProcessMsg (Pre/PostProcessMsg need the sound system, gsSND_SYSTEM, not rebuilt);
//   - gsSysServer / gsSysMP: a single-player gsMP_SERVER (MPSRV_LOCAL) with the test player in slot 0;
//   - the level messages (LEVEL_LOADED, INIT_LEVEL) and the frame pump sent from here.
// Only the classes rebuilt so far are created: "door", "button", "trampoline", "water", "lava".

#include "../src/Sources/game.h"
#include "../src/engine/engine.h"      // (test) cdtCOLL_ANIM, cdtOflPtr for the grid check
#include "../src/gs/gs_mp_server.h"
#include "../src/gs/gs_mp_misc.h"
#include "../src/gs/gs_mp_client.h"
#include "../include/game_shared/wr_item.h"
#include "../include/frontend/wr_msgs.h"
#include "test_ents.h"
#include "test_menu.h"
#include "test_assets.h"
#include "tst_pte.h"
#include "../include/engine_render/shader.h"
#include <stdio.h>

static entENTITY *s_ent[1024];                           // (test) was 256: lev_2_2_city has more
static int s_nEnt;
static char s_entInMsg[1024];                            // made by entCreate: in msgSystem (its FRAME comes from there)
static char s_lvlBuf[0x4000];               // a zeroed gsLVL_SYSTEM (SetExtraObjFlags reads nothing of it)
static char s_mpBuf[0x664];                 // a zeroed gsMP_SYSTEM, mode 1 (single player)

// (test) the start-up's shader part (gsAPP_SYSTEM::InitAll: pteInit, pteSpotInit, pteFogInit): the manager and the
// "fog" class (the spot classes need the texture manager: not here). Once.
void tstPteInit()
{
    if (pteManager && *(int *)((char *)pteManager + 0x17944))
        return;
    if (!pteInit())
        printf("pteInit failed%c", 10);
    pteFogInit();
}

static void objFlags(gsLVL_SYSTEM *l, objOBJ *o)
{
    if (!o)
        return;
    tstPteInit();
    pteManager->CreateShader(o);                        // (gsLVL_SYSTEM::ObjLoadNotify v12 0x005356B2)
    if (!o->name || !strstr(o->name, "&bb"))
        l->SetExtraObjFlags(o);
    else if (char *t = strstr(o->name, "&tra")) {       // (a billboard skips the real one, for its modifier, not rebuilt:
        int a = 0xFF;                                   //  its transparency as SetExtraObjFlags gives it)
        sscanf(t, "&tra %d", &a);
        o->SetTransparency(0x40, (u8)a);
    }
    objOBJ *first = o->child;
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        objFlags(l, c);
}

static void lvlMirrorTree(objOBJ *o);

// ObjLoadNotify's next call (v12 0x005356A6, on the scene's and the instances' trees, not the templates'): the texture
// animations by name
static void objTexAnim(gsLVL_SYSTEM *l, objOBJ *o)
{
    if (!o)
        return;
    if (o->geom && o->nFace > 0)
        l->SetTexAnimObjFlags(o);
    objOBJ *first = o->child;
    int more = 0;
    for (objOBJ *c = first; c && (!more || c != first); c = c->next, more = 1)
        objTexAnim(l, c);
}

void tstLevelObjFlags(scnSCENE *s)
{
    gsLVL_SYSTEM *l = (gsLVL_SYSTEM *)s_lvlBuf;
    objFlags(l, s->root);
    objTexAnim(l, s->root);
    for (int k = 0; k < 300; k++)
        if (s->tplPool[k].magic && s->tplPool[k].root)
            objFlags(l, s->tplPool[k].root);
}

static animTPL *findTpl(scnSCENE *s, const char *n)
{
    for (int k = 0; k < 300; k++)
        if (s->tplPool[k].magic && s->tplPool[k].root && !stricmp(s->tplPool[k].tplName, n))
            return &s->tplPool[k];
    return 0;
}

// what scnSCENE::CreateInst (v12 0x004E6620) does after placing an instance
static int completeInst(scnSCENE *s, animINST *a)
{
    animTPL *t = a->tpl ? a->tpl : findTpl(s, a->tplName);
    if (!t || !t->root)
        return 0;
    a->tpl = t;
    if (!a->root) {
        a->root = t->root->Duplicate((objOBJ *)0);
        if (!a->root)
            return 0;
    }
    a->flags |= t->flags;
    if (a->props)
        a->props->parent = t->props;
    {                                                   // [ANIM_PROP] scale, as scnSCENE::CreateInst (v12 0x004E6620)
        float sc;                                       // -- the game makes every level instance again through it
        if (a->GetFloat((char *)"ANIM_PROP", (char *)"scale", &sc))
            a->Scale(sc, sc, sc, 2);
    }
    a->SetObjInstRef();
    a->nObj = a->root->GetNObj(0);
    a->objTable = (objOBJ **)apCalloc(a->nObj, 4);
    objMakePtrListID(a->root, a->objTable, a->nObj);
    // (the pose bits 0x18 only with a sequence: posing runs animSEQ::TimeToFrame on seq[seq], and a template without
    // sequences -- the items: key, money -- has none; its "length < 0.001 -> 0.5" then wrote 0.5f into the next heap
    // block, the medkit's property sheet. The game's CreateInst sets 0x84 | 0x20004 only)
    a->Invalidate(0x20004 | 0x80 | (t->nSeq > 0 ? 0x18 : 0));
    a->AbvInit();                                       // (as CreateInst: its volumes, what a shot tests against)
    // the level loader's notify for the instance (gsLVL_SYSTEM::InstLoadNotify -> ObjLoadNotify, v12 0x00535700 /
    // 0x00535680): the "&tag" flags on its own tree too -- an instance read with the level keeps its own objects, so
    // the scene's and the templates' pass does not reach them (the lava's "&dom_&fog" outlines stayed drawn)
    if (!(a->flags & 0x20000)) {
        objFlags((gsLVL_SYSTEM *)s_lvlBuf, a->root);
        objTexAnim((gsLVL_SYSTEM *)s_lvlBuf, a->root);
        lvlMirrorTree(a->root);
        a->flags |= 0x20000;
    }
    return 1;
}

const char *tstEntsGameDir = ".";

// PLACEHOLDER (test side; DELETE when rgdRIGID / nchRIGID, the flying pieces, are rebuilt: phys_rigid.obj,
// sob_niche.obj): a broken thing's piece ("nch_rigid", dstrDESTROY::StartExplos) is hidden and goes at once instead
// of flying off -- what breaks leaves its "geom_debris_left", the vase its shards and dust (particles).
class tstRIGID : public entENTITY {
public:
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret)
    {
        if (id == 1) {
            inst->flags |= 1 | 0x10;
            msgSystem.PostMsg(this, 2, this);
        }
        return entENTITY::ProcessMsg(id, data, from, ret);
    }
    static entENTITY *__fastcall Create(animINST *)
    {
        return new tstRIGID;
    }
};

void tstBreakVases()                            // (test) --picktest: every vase broken by one hit
{
    dmgGENERAL g;
    g.amount = 100.0f;
    for (entENTITY *e = entFindNext(0); e; e = entFindNext(e))
        if (e->classId == 0x24445354 && e->inst && e->inst->className && !stricmp(e->inst->className, "vase_destroy"))
            msgSystem.SendMsg(e, 0x4C6, &g, 0);
}

// PLACEHOLDER (test renderer helper) for the shader pass of d3drSDR_REND_VERT: this entity's vertex shader, if it has one
pteSHADER *tstEntVertShader(entENTITY *e, scnSCENE *s)
{
    if (!e->inst->className || stricmp(e->inst->className, "sfx_portal"))
        return 0;
    wrsfxPORTAL_VIEW *v = ((wrsfxPORTAL *)e)->view;
    if (v && !tstDrawLayer) {
        v->Update(gsElapsedTime);
        v->PrepareFrame(s, gsCameraPtr);
    }
    return v;
}

// PLACEHOLDER (test renderer helper, asked for): the live items ('$ITM') that can be picked now -- active and shown --
// tstPickupCount lists them in one walk of the entities; tstPickupBox reads that list.
static entENTITY *s_pick[512];
static int s_nPick;
int tstPickupCount()
{
    s_nPick = 0;
    for (entENTITY *e = entFindNext(0); e && s_nPick < 512; e = entFindNext(e))
        if (e->classId == 0x2449544D && (((itemITEM *)e)->flags & ITEMF_ACTIVE) && !(e->inst->flags & 1))
            s_pick[s_nPick++] = e;
    return s_nPick;
}
int tstPickupBox(int i, m3dV *c)
{
    if (i < 0 || i >= s_nPick)
        return 0;
    s_pick[i]->inst->GetPos(c);
    return 1;
}

int __fastcall dntInit();                               // src/Sources/day_night.cpp
int __fastcall wrsfxWaterInit();                        // src/Sources/sfx_water.cpp
int __fastcall wrsfxWFallInit();                        // src/Sources/sfx_wfall.cpp
int __fastcall wrsfxFountainInit();                     // src/Sources/sfx_fountain.cpp
int __fastcall wrsfxBarrelInit();                       // src/Sources/sfx_barrel.cpp
static int hostedClass(const char *c)
{
    return c && (!stricmp(c, "door") || !stricmp(c, "button") || !stricmp(c, "trampoline") || !strnicmp(c, "item", 4)
                 || !stricmp(c, "vase_destroy") || !stricmp(c, "tripod") || !stricmp(c, "mirror") || !stricmp(c, "barrel") || !stricmp(c, "dyn_destroy") || !stricmp(c, "sob_katapult") || !stricmp(c, "sob_tra_altar")
                 || !stricmp(c, "sob_tra_button") || !stricmp(c, "sfx_flame") || !stricmp(c, "sob_blade")
                 || !stricmp(c, "sob_tree") || !stricmp(c, "sfx_portal") || !strnicmp(c, "ai_rat", 6) || !strnicmp(c, "ai_skel", 7) || !stricmp(c, "ai_zevs") || !strnicmp(c, "ai_mino", 7) || !stricmp(c, "ai_satyr") || !stricmp(c, "ai_bird") || !stricmp(c, "bird_egg") || !stricmp(c, "sfx_egg") || !stricmp(c, "sob_niche") || !stricmp(c, "sfx_niche_explode") || !strnicmp(c, "ai_cyclop", 9) || !stricmp(c, "pjl_stone_cyclop") || !stricmp(c, "pjl_satyr_arrow") || !stricmp(c, "sfx_satir_arrow_land") || !stricmp(c, "sfx_appear") || !stricmp(c, "sfx_treasure") || !stricmp(c, "ai_gefest") || !stricmp(c, "ai_croc") || !stricmp(c, "ai_amur") || !stricmp(c, "ai_cent") || !strnicmp(c, "ai_tiger", 8) || !strnicmp(c, "ai_lion", 7) || !strnicmp(c, "ai_cerber", 9) || !stricmp(c, "pjl_crb_frb") || !stricmp(c, "pjl_crb_acd") || !stricmp(c, "sfx_cerber_pjl_expl") || !stricmp(c, "sfx_cerber_pjl_acid_expl") || !stricmp(c, "ai_sketavr") || !stricmp(c, "pjl_trident") || !stricmp(c, "pjl_amur_arrow") || !strnicmp(c, "ai_glad", 7) || !stricmp(c, "pjl_knife_glad") || !stricmp(c, "pjl_mace_glad") || !stricmp(c, "sfx_glad_mace_expl") || !stricmp(c, "pjl_spear_skel") || !stricmp(c, "sfx_bsk_bullet") || !stricmp(c, "pwp_bmb_pjl") || !stricmp(c, "pwp_bmb_pjl_expl") || !stricmp(c, "sfx_skel_spear_land") || !stricmp(c, "pjl_gefest") || !stricmp(c, "sfx_gefest_hit") || !stricmp(c, "sfx_gefest_expl") || !stricmp(c, "sfx_water_stream") || !stricmp(c, "sfx_waterfall") || !stricmp(c, "sfx_fountain") || !stricmp(c, "sfx_barrel_expl"));
}

// the rebuilt classes the level start creates (with "dynamic")
static int hostedEnt(const char *c)
{
    return hostedClass(c) || (c && (!stricmp(c, "lava") || !stricmp(c, "water") || !stricmp(c, "cinema")));
}

// PLACEHOLDER (test side) for the level loader's external templates: an instance whose template is not in the level
// (database\<tpl>.tpl, e.g. "trampoline") gets it loaded -- but only when its class is one this host creates, so
// items and enemies stay unloaded and undrawn. The instance takes the template's class, as scnSCENE::CreateInst.
static int tplAfterLoad(scnSCENE *s, animTPL *t, const char *name);
static void loadClassTpls(scnSCENE *s)
{
    static char skip[64][0x40];
    int nSkip = 0;
    for (int k = 0; k < 1000; k++) {
        animINST *a = s->InstPtr(k);
        if (const char *sw = getenv("WR_SWAPCLASS"))    // (test) WR_SWAPCLASS=from=to: instances of class "from..."
            if (a->magic == 0x41494E53 && a->tplName) {     //  become "to" (its database template), e.g. ai_mino=ai_amur
                static char from[0x40], to[0x40];
                if (!from[0] && sscanf(sw, "%63[^=]=%63s", from, to) != 2)
                    from[0] = 0;
                animTPL *ot = a->className && a->className[0] ? 0 : findTpl(s, a->tplName);
                const char *oc = a->className && a->className[0] ? a->className : ot ? ot->className : "";
                if (from[0] && !strnicmp(oc, from, strlen(from))) {
                    printf("swapclass: %s (%s) -> %s%c", a->name ? a->name : "?", oc, to, 10);
                    a->tplName = to;
                    a->className = 0;
                }
            }
        if (a->magic != 0x41494E53 || !a->tplName || !a->tplName[0] ||
            (a->className && a->className[0] && (!hostedClass(a->className) || findTpl(s, a->tplName))))
            continue;
        animTPL *t = findTpl(s, a->tplName);
        if (!t) {
            int j;
            for (j = 0; j < nSkip && stricmp(skip[j], a->tplName); j++)
                ;
            if (j < nSkip)
                continue;
            int slot;
            for (slot = 0; slot < 300 && s->tplPool[slot].magic; slot++)
                ;
            char path[512];
            snprintf(path, sizeof(path), "%s\\database\\%s.tpl", tstEntsGameDir, a->tplName);
            animTPL *f = slot < 300 && GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES ? tstLoadTemplate(path) : 0;
            if (!f || !hostedClass(f->className)) {
                if (nSkip < 64)
                    strncpy(skip[nSkip++], a->tplName, 0x3F);
                continue;
            }
            memcpy((void *)&s->tplPool[slot], (void *)f, sizeof(animTPL));
            strncpy(s->tplPool[slot].tplName, a->tplName, 0x3F);   // found by the file's name
            t = &s->tplPool[slot];
            if (!tplAfterLoad(s, t, a->tplName))        // (without it a skinned enemy -- the minotaur, Zeus -- had
                continue;                               //  no skins: its body stayed in the reference pose)
        }
        if (hostedClass(t->className))
            a->className = t->className;
    }
}

static void send(entENTITY *e, int id, void *data)
{
    int ret = 0;
    e->ProcessMsg(id, data, 0, &ret);
}

extern fioFILE_MEM *gsMemFile;                  // v12 0x009A0054 (ent_mp.cpp)

// PLACEHOLDER: a single-player gsMP_SERVER (MPSRV_LOCAL), a local gsMP_CLIENT (MPCL_LOCAL) and a zeroed gsMP_SYSTEM in mode 1 (the game makes both
// in gsAPP_SYSTEM::InitAll, not rebuilt)
void tstServerInit()
{
    if (!gsMemFile)                         // PLACEHOLDER (test): the shared memory file (its owner, not rebuilt, makes it):
        gsMemFile = new fioFILE_MEM;        //   an entity's init data is packed into it (entPackInitDataPacked)
    if (!gsSysServer)
        gsSysServer = new gsMP_SERVER;
    gsSysServer->flags |= MPSRV_LOCAL;
    if (!gsSysMP) {
        gsSysMP = (gsMP_SYSTEM *)s_mpBuf;
        gsSysMP->mode = 1;
    }
    // PLACEHOLDER (test) for wrGOInit's statistics system (new wrSTAT_SYSTEM 0x144; its ctor and ProcessMsg, v12
    // 0x004AD160, are not rebuilt, so it is no address: nothing resets the counts between levels). The records are
    // the real ones: Notify_GameBegin here, Notify_AddPlayer in tstPlayerJoin
    if (!wrSysStat) {
        wrSysStat = (wrSTAT_SYSTEM *)apCalloc(1, 0x144);
        wrSysStat->Notify_GameBegin();
    }
    if (!gsSysClient) {                     // the local client (single player: messages delivered at once)
        gsSysClient = new gsMP_CLIENT;
        gsSysClient->flags |= MPCL_LOCAL;
    }
}

void tstLevelSystemInit(scnSCENE *s);
void tstLevelSystemSetScene(scnSCENE *s);

// PLACEHOLDER (test side) for the wr game's GS_MSG_TELEPORT_LEVEL handler (wrGAME, wr_main.cpp: not level complete ->
// GS_MSG_LOAD_LEVEL 0x3EC to gspSysPlay -> gsSysLevel LoadLevel with PrepareReloadAnchor / ApplyReloadAnchor, none
// rebuilt): an address that keeps the last request; the test's main loop then changes the level (box_level.cpp).
static gsLOAD_SCENE_PARAM s_tptReq;
static int s_tptPending;
class tstGAME_ADDR : public msgADDR {
public:
    int lock;                       // +0x24 (the lock count msgSystem reads)
    tstGAME_ADDR()
    {
        node.prev = node.next = 0;
        flags = 0;
        magic = 0x4D414452;
        classId = 0;
        unk18 = 0;
        priority = 0;
        sign = 0;
        lock = 0;
    }
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret)
    {
        if (id == GS_MSG_TELEPORT_LEVEL && data) {
            memcpy(&s_tptReq, data, sizeof(s_tptReq));
            s_tptPending = 1;
        }
        return 0;
    }
};
int tstTakeTeleport(gsLOAD_SCENE_PARAM *out)
{
    if (!s_tptPending)
        return 0;
    s_tptPending = 0;
    *out = s_tptReq;
    return 1;
}

// (test) --tpttest: the player into the middle of the first teleport domain
// (test) --gotodom <part of the name>: into a spawn domain's middle
void tstEntsToSpawnDomain(entENTITY *player, const char *name)
{
    if (wrSecretSystem)
        for (int i = 0; i < wrSecretSystem->nDomains; i++) {
            gsDOMAIN *d = wrSecretSystem->domain[i];
            if (!strstr(d->obj->name, name))
                continue;
            m3dV p = { 0.5f * (d->bbox.min[0] + d->bbox.max[0]), d->yMin + 1.0f, 0.5f * (d->bbox.min[2] + d->bbox.max[2]) };
            m3dV dir = { 0, 0, 1 };
            wrEVCall<void>(player, 0x70, &p, &dir, 1);
            printf("gotodom %s: at %.2f %.2f %.2f%c", d->obj->name, p.x, p.y, p.z, 10);
            return;
        }
    if (!aiSpawnDomList)
        return;
    for (int i = 0; i < aiSpawnDomList->nDomains; i++) {
        gsDOMAIN *d = aiSpawnDomList->domain[i];
        printf("spawn domain %s: %.1f %.1f %.1f .. %.1f %.1f %.1f\n", d->obj->name, d->bbox.min[0], d->bbox.min[1],
               d->bbox.min[2], d->bbox.max[0], d->bbox.max[1], d->bbox.max[2]);
    }
    for (int i = 0; i < aiSpawnDomList->nDomains; i++) {
        gsDOMAIN *d = aiSpawnDomList->domain[i];
        if (!strstr(d->obj->name, name))
            continue;
        m3dV p = { 0.5f * (d->bbox.min[0] + d->bbox.max[0]), d->yMin + 2.0f, 0.5f * (d->bbox.min[2] + d->bbox.max[2]) };
        m3dV dir = { 0, 0, 1 };
        d->nLocks = 0;                                   // (test) whatever the level would unlock it with
        wrEVCall<void>(player, 0x70, &p, &dir, 1);
        printf("gotodom %s: at %.2f %.2f %.2f\n", d->obj->name, p.x, p.y, p.z);
        return;
    }
}

void tstEntsToTpt(entENTITY *player)
{
    if (!domTptList || !domTptList->nDomains) {
        printf("tpttest: no teleport domain\n");
        return;
    }
    gsDOMAIN *d = domTptList->domain[0];
    m3dV p = { 0.5f * (d->bbox.min[0] + d->bbox.max[0]), d->yMin + 2.0f, 0.5f * (d->bbox.min[2] + d->bbox.max[2]) };
    m3dV dir;
    player->inst->GetPos(&dir);
    dir.x = p.x - dir.x;
    dir.y = 0;
    dir.z = p.z - dir.z;
    float L = sqrtf(dir.x * dir.x + dir.z * dir.z);
    if (L < 1e-3f) { dir.x = 0; dir.z = 1; } else { dir.x /= L; dir.z /= L; }
    player->inst->SetPosDir_XZ(&p, &dir);
    printf("tpttest: player to %s at %.1f %.1f %.1f\n", d->obj->name, p.x, p.y, p.z);
}

int __fastcall partInit();                              // engine particle.cpp

void tstEntsStart(scnSCENE *s, entENTITY *player)
{
    s_nEnt = 0;
    tstServerInit();
    gsSysServer->clients[0].player = player;
    // as the game's single player: the player is owned by the local client (-1), so server -> client messages (a
    // locked door's "locked" notify) reach gsSysClient through msgSYSTEM::SendMsg
    player->mpOwner = -1;                               // (the local client: gsSysClient, MPCL_LOCAL)
    static int reg;
    if (!reg) {
        if (!partInit())                                // (the game: with the engine, before any entity makes an emitter)
            printf("partInit failed (no particle texture)\n");
        doorInit();
        bttnInit();
        trmpInit();
        waterInit();
        dynInit();
        physInit();                                     // "phys" (the gibs' base)
        wrsobInit();                                    // the game's scene objects (not all rebuilt)
        wrsfxPortalInit();
        wrsfxInit();                                    // (its own part)
        wrsfxTreasureInit();
        wrsfxWaterInit();                               // the water stream, the waterfall's foam, the fountain,
        wrsfxWFallInit();                               //  the barrel's explosion
        wrsfxFountainInit();
        wrsfxBarrelInit();
        aiInit();                                       // the attack director, then the one enemy rebuilt so far
        aiRatInit();
        aiSkeletonsAllInit();
        aiZevsInit();                                   // Zeus (the bolts not rebuilt)
        aiMinoInit();                                   // the minotaur and its thrown axe
        aiSatyrInit();                                  // the satyr archer, its arrow and the arrow's dust
        aiCyclopInit();                                 // the cyclops (boss and _mini) and their stones (dust: placeholder)
        aiBirdInit();                                   // the Stymphalian bird (its egg and death effect not rebuilt)
        aiGladiatorsAllInit();                          // both gladiators, their knife / mace, the mace explosion
        aiCrocInit();                                   // the crocodile
        aiAmurInit();                                   // the amur and its arrow
        aiCentInit();                                   // the centaur, the skeleton centaur, the trident
        aiTigerInit();                                  // the tiger and the lion
        aiCerberInit();                                 // the cerberus, its fireball and acid, their bursts
        aiGefestInit();                                 // Hephaestus (boss and _mini), his coals and effects
        if (getenv("WR_DNT"))                           // (test, asked for) off: the level hangs with it (WR_DNT=1 on)
            dntInit();                                  // day and night (day_night.cpp)
        domMiscInit();                                  // the action / cleanup / teleport / kill domain lists
        wrMsgInit();                                    // the "msg" / "secret" domain lists and Will's voice
        cinInit();                                      // "cinema" and the cinematics system (the shared objects' init,
                                                        //  v12 0x00557C66)
        // PLACEHOLDER (test) for wrGO_SYSTEM::ProcessINIT_GAME (rebuilt, not run here): single player turns the
        // cinematics on (state 4)
        gsSysCinema->state |= 4;
        itemInit();                                     // "item": gs_shared's, then the game's (wrItemInit) over it
        wrItemInit();
        dstrInit();                                     // "dyn_destroy", then the game's breakables over it
        wrsfxDestrInit();
        entRegisterClass((char *)"nch_rigid", 0x52474442, 0x32, tstRIGID::Create, 0, 1);   // (stand-in above)
        wrsfxFlameInit();
        static tstGAME_ADDR game;
        msgSystem.InsertAddr(&game, 0, 0);
        reg = 1;
    }
    loadClassTpls(s);
    if (!gsSysLevel)                                    // the level system stand-in (entRegister tells it of each
        tstLevelSystemInit(s);                          // new instance: gsLvlInstLoadNotify)
    tstLevelSystemSetScene(s);
    // PLACEHOLDER for gsLVL_SYSTEM::LoadInstAll (v12 0x00536EF0, not rebuilt): a placed instance without a class
    // takes its template's class, else "dynamic", and gets that entity through entCreate (entRegister: INIT,
    // INIT_LEVEL, the collision data CDTInit, the instance grid). Here only for the rebuilt classes ("dynamic" and
    // the hosted door / button / trampoline / water / lava); an instance of a class not rebuilt yet stays as it is
    // (the game would destroy it). (WR_NODYNAMIC=1: the dynamic ones off)
    // (the level is loading: gsAppState 0x40000 "level running" clear, so entRegister sends no INIT_LEVEL of its own --
    // the one SendAll below gives it, else the IActions would be parsed twice)
    int appState = gsAppState;
    gsAppState &= ~0x40000;
    for (int k = 0; k < 1000 && s_nEnt < 1024; k++) {
        animINST *a = s->InstPtr(k);
        if (a->magic != 0x41494E53 || a->ent || !a->tplName || !a->tplName[0])
            continue;
        int own = a->className && a->className[0];
        if (own && !hostedEnt(a->className) && stricmp(a->className, "dynamic"))
            continue;
        if (!completeInst(s, a))
            continue;
        const char *cls = own ? a->className : a->tpl->className[0] ? a->tpl->className : "dynamic";
        if (getenv("WR_CLASSDUMP"))
            printf("resolve %s -> %s%s\n", a->name ? a->name : "", cls, own ? " (own)" : "");
        int dyn = !stricmp(cls, "dynamic");
        if (dyn ? getenv("WR_NODYNAMIC") != 0 : !hostedEnt(cls))
            continue;
        if (getenv("WR_CLASSDUMP")) {
            printf("dynamic: create %s (tpl %s, root %p, nObj %d)\n", a->name ? a->name : "", a->tplName ? a->tplName : "", (void *)a->root, a->nObj);
            fflush(stdout);
        }
        // PLACEHOLDER: on the server the game's IActive set-up (entIACTIVE_OBJ::InitIActive, not rebuilt) enables
        // the IActions; a button without it acts as a client and only asks the server to toggle
        entENTITY *e = entCreate((char *)cls, a, 0, !stricmp(cls, "button") || !strnicmp(cls, "item", 4)
                                 || !strnicmp(cls, "sob_tra", 7) || !strnicmp(cls, "ai_rat", 6) || !strnicmp(cls, "ai_skel", 7) || !strnicmp(cls, "ai_mino", 7) || !stricmp(cls, "ai_satyr") || !stricmp(cls, "ai_bird") || !stricmp(cls, "sob_niche") || !strnicmp(cls, "ai_cyclop", 9) || !stricmp(cls, "ai_zevs") || !stricmp(cls, "ai_gefest") || !stricmp(cls, "ai_croc") || !stricmp(cls, "ai_cent") || !strnicmp(cls, "ai_tiger", 8) || !strnicmp(cls, "ai_lion", 7) || !strnicmp(cls, "ai_cerber", 9) || !stricmp(cls, "ai_sketavr") || !stricmp(cls, "ai_amur") || !strnicmp(cls, "ai_glad", 7) || !stricmp(cls, "sob_blade") || !stricmp(cls, "sfx_portal") || !stricmp(cls, "sfx_flame")
                                 || !stricmp(cls, "vase_destroy") || (!stricmp(cls, "dynamic") && !getenv("WR_NODYNSCRIPT")) ? ENTF_IACTIONS_ON : 0);
        if (!e)
            continue;
        a->Validate(0x6);
        s_entInMsg[s_nEnt] = 1;
        s_ent[s_nEnt++] = e;
        if (!dyn) {
            m3dV p;
            a->GetPos(&p);
            printf("entity %s \"%s\" (tpl %s) at %.2f %.2f %.2f\n", cls, a->name ? a->name : "", a->tplName, p.x, p.y, p.z);
        }
        if (getenv("WR_CLASSDUMP")) {
            m3dV p;
            a->GetPos(&p);
            printf("dynamic %s: coll %p at %.2f %.2f %.2f nSeq %d\n", a->name ? a->name : "", (void *)a->coll, p.x, p.y, p.z, a->tpl->nSeq);
            if (s->collAnim) {
                m3dBOX bb;
                a->GetBBox(&bb);
                s->collAnim->MakeIndList(&bb, cdtOflPtr, 0);
                int in = 0;
                for (int q = 0; q < cdtOflPtr->nObj; q++)
                    if (cdtOflPtr->obj[q] == a->index)
                        in = 1;
                printf("  grid: in %d (%d ids), box %.1f %.1f %.1f .. %.1f %.1f %.1f, flags %X\n", in, cdtOflPtr->nObj,
                       bb.vMin.x, bb.vMin.y, bb.vMin.z, bb.vMax.x, bb.vMax.y, bb.vMax.z, a->flags);
            }
        }
    }
    gsAppState = appState;
    if (getenv("WR_CLASSDUMP"))
        printf("dynamic: %d entities\n", s_nEnt);
    if (getenv("WR_CLASSDUMP"))                         // (test) the classes of the level's instances
        for (int k = 0; k < 1000; k++) {
            animINST *a = s->InstPtr(k);
            if (a->magic == 0x41494E53)
                printf("inst %s class %s tpl %s\n", a->name ? a->name : "", a->className ? a->className : "-",
                       a->tplName ? a->tplName : "-");
        }
    for (int k = 0; k < 1000 && s_nEnt < 1024; k++) {
        animINST *a = s->InstPtr(k);
        if (a->magic != 0x41494E53 || !a->className || a->ent)
            continue;
        int c;
        for (c = 0; c < entNClass; c++)
            if (!stricmp(entClassTable[c].name, a->className))
                break;
        if (c == entNClass || (stricmp(a->className, "door") && stricmp(a->className, "button") && stricmp(a->className, "trampoline") &&
                                 stricmp(a->className, "water") && stricmp(a->className, "lava") &&
                                 stricmp(a->className, "cinema")))
            continue;
        if (!completeInst(s, a))
            continue;
        entENTITY *e = entClassTable[c].create(a);
        if (!e)
            continue;
        e->classId = entClassTable[c].classId;
        e->priority = entClassTable[c].priority;
        e->name.SetName(a->name ? a->name : (char *)"");
        e->inst = a;
        a->ent = e;
        // PLACEHOLDER: on the server the game's IActive set-up (entIACTIVE_OBJ::InitIActive, not rebuilt) enables
        // the IActions; a button without it acts as a client and only asks the server to toggle
        if (!stricmp(a->className, "button") || !stricmp(a->className, "cinema"))
            e->entFlags |= ENTF_IACTIONS_ON;
        send(e, MSG_MSG_INIT, 0);
        send(e, 0x3F4, 0);                  // INIT_LEVEL
        a->Validate(0x6);
        m3dV p;
        a->GetPos(&p);
        printf("entity %s \"%s\" (tpl %s) at %.2f %.2f %.2f\n", a->className, a->name ? a->name : "", a->tplName, p.x,
               p.y, p.z);
        s_entInMsg[s_nEnt] = 0;
        s_ent[s_nEnt++] = e;
    }
    // the level's messages to every address (the sound system, the domain lists: they collect and init their
    // domains), as gsLVL_SYSTEM does after loading (not rebuilt): LEVEL_LOADED, then INIT_LEVEL. Before INIT_LEVEL
    // the level runs again (__gsLoadLevel v12 0x00536B04: 0x80000 off, 0x40000 on): gsLVL_SYSTEM::DestroyLevel took
    // 0x40000 away, and without it entIACTIVE_OBJ::PreProcessMsg drops every FRAME (the player and its weapons
    // stood still after a level change)
    // the rest of the loader's sweep (gsLVL_SYSTEM::LoadInstAll, v12 0x005358C4: InstLoadNotify on every instance): those
    // the entities made while the level loads (the altar's collision object: its "&plrc" hides it)
    for (int k = 0; k < 1000; k++) {
        animINST *a = s->InstPtr(k);
        if (a->magic == 0x41494E53 && a->root && !(a->flags & 0x20000)) {
            objFlags((gsLVL_SYSTEM *)s_lvlBuf, a->root);
            objTexAnim((gsLVL_SYSTEM *)s_lvlBuf, a->root);
            a->flags |= 0x20000;
        }
    }
    gsAppState = (gsAppState & ~0x80000u) | 0x40000;
    if (gsSysSound) {
        if (getenv("WR_SENDLOG"))                       // (test) which receiver of the two
            for (msgADDR *m = msgSystem.GetFirstAddr(); m; m = m->GetNext()) {
                if (m->classId == 0x504C5952) continue;
                printf("send: %08X %p vt %p ms %p%c", m->classId, (void *)m, *(void **)m, (void *)&msgSystem, 10);
                fflush(stdout);
                int r = 0;
                m->ProcessMsg(GS_MSG_LEVEL_LOADED, 0, 0, &r);
                m->ProcessMsg(GS_MSG_INIT_LEVEL, 0, 0, &r);
            }
        msgSystem.SendAll(GS_MSG_LEVEL_LOADED, 0, 0);
        msgSystem.SendAll(GS_MSG_INIT_LEVEL, 0, 0);
        // PLACEHOLDER: entIACTIVE_OBJ::InitIActive (not rebuilt) enables the IActions on the server; the domains
        // only react with it (entFlags 4)
        gsDOMAIN_LIST *dl[7] = { domActList, domClnList, domTptList, domKillList, wrMsgSystem, wrSecretSystem, aiSpawnDomList };
        for (int j = 0; j < 7; j++)
            for (int i = 0; dl[j] && i < dl[j]->nDomains; i++) {
                dl[j]->domain[i]->entFlags |= ENTF_IACTIONS_ON;
                if (j == 6)                         // and their spawn areas ("ai_area*": the coliseum gates' ON_START)
                    for (int k = 0; k < dl[j]->domain[i]->nSub; k++)
                        if (scnDOMAIN *a = dl[j]->domain[i]->sub[k])
                            ((aiSPAWN_AREA *)((u8 *)a - 0x98))->entFlags |= ENTF_IACTIONS_ON;
                if (j == 2)
                    printf("teleport domain %s: %s%s\n", dl[j]->domain[i]->obj->name,
                           ((domTPT_DOMAIN *)dl[j]->domain[i])->sceneName[0] ? "level " : "anchor ",
                           ((domTPT_DOMAIN *)dl[j]->domain[i])->sceneName[0] ? ((domTPT_DOMAIN *)dl[j]->domain[i])->sceneName
                                                                              : ((domTPT_DOMAIN *)dl[j]->domain[i])->anchor);
            }
        // PLACEHOLDER (test): a spawner of an enemy not rebuilt yet (ai_mino, ai_glad, ai_amur..) makes a satyr
        // skeleton instead, so that the levels can be played and their kill counts finish. DELETE enemy by enemy.
        for (int i = 0; aiSpawnDomList && i < aiSpawnDomList->nDomains; i++) {
            aiDOMAIN_SPAWN *d = (aiDOMAIN_SPAWN *)aiSpawnDomList->domain[i];
            if (const char *f = getenv("WR_FORCEOPP")) {     // (test) every spawner makes this one
                strcpy(d->oppClass, f);
                strcpy(d->oppName, strnicmp(f, "ai_skel", 7) ? f : "ai_skel");   // (the class's own template)
            } else if (d->oppClass[0] && _entFindClass(d->oppClass) < 0) {
                strcpy(d->oppClass, "ai_satyr");      // (asked: satyrs instead of the sword skeleton)
                strcpy(d->oppName, "ai_satyr");
            }
        }
        for (int i = 0; getenv("WR_SPAWNDUMP") && aiSpawnDomList && i < aiSpawnDomList->nDomains; i++) {   // (test)
            aiDOMAIN_SPAWN *d = (aiDOMAIN_SPAWN *)aiSpawnDomList->domain[i];
            printf("spawn domain %s: %s box %.1f %.1f %.1f - %.1f %.1f %.1f%c", d->obj->name, d->oppClass,
                   d->bbox.min[0], d->bbox.min[1], d->bbox.min[2], d->bbox.max[0], d->bbox.max[1], d->bbox.max[2], 10);
        }
        printf("domains: %d action, %d cleanup, %d teleport, %d kill, %d msg, %d secret\n", domActList->nDomains,
               domClnList->nDomains, domTptList->nDomains, domKillList->nDomains, wrMsgSystem->nDomains,
               wrSecretSystem->nDomains);
        printf("domains: %d ambient sound, %d music\n", gsSysSound->sndDomains->nDomains,
               gsSysSound->musDomains->nDomains);
    }
}

// The local player joins, as the game's client does once the level is in and the player made: GS_MSG_MP_CREATE_PLAYER
// to the local server (gsMP_SERVER::ProcessMsg, rebuilt) -- its MP id, so the server's notifies reach it through the
// local client, and the server's player record
void tstPlayerJoin(entENTITY *player)
{
    if (gsSysMP->FindPlrLocal() != player)
        return;
    // PLACEHOLDER (test) for wrGO_SYSTEM::ProcessMsg 0x408 (the player joined): its statistics record
    ((plrPLAYER_COMMON *)player)->unk_21F = -1;
    wrSysStat->Notify_AddPlayer((plrPLAYER_COMMON *)player);
    static u8 join[0x200];
    ((msgDATA *)join)->tag = 0x40;
    msgSystem.SendMsg(gsSysServer, 0x402, join, 0);
    printf("join: the player has MP id %d%c", player->mpId, 10);
}

// (test) WR_AILOG=1: once a second, every NPC: place, distance to its enemy, sequence, health, flags
static void tstAiLog()
{
    static const char *on = getenv("WR_AILOG");
    if (!on)
        return;
    static int sparkMax, sparkOn;
    for (aiNPC *a = aiNPC::GetNext(0, 1); a; a = aiNPC::GetNext(a, 1)) {
        wrsfxRAT_SPARKS *s = a->inst->className && !strnicmp(a->inst->className, "ai_rat", 6) ? ((aiRAT *)a)->sparks : 0;
        if (s && s->started) {
            sparkOn++;
            if (s->emitter->nPart > sparkMax)
                sparkMax = s->emitter->nPart;
        }
    }
    static float t;
    t += gsElapsedTime;
    if (t < 1.0f)
        return;
    printf("sparks: on %d frames, most particles %d", sparkOn, sparkMax);
    puts("");
    t = 0;
    int n = 0;
    int shown = 0;
    const char *only = getenv("WR_AILOGCLASS");         // (test) only NPCs of this class (prefix)
    for (aiNPC *a = aiNPC::GetNext(0, 1); a; a = aiNPC::GetNext(a, 1), n++) {
        if (only && (!a->inst->className || strnicmp(a->inst->className, only, strlen(only))))
            continue;
        if (shown++ >= 6)
            continue;
        m3dV p;
        a->inst->GetPos(&p);
        printf("ai %s at %.1f %.1f %.1f dist %.1f seq %d hp %.0f flags %X col %08X\n", a->inst->className ? a->inst->className : "?",
               p.x, p.y, p.z, a->enemyDist, a->inst->seq, a->health, (unsigned)a->aiFlags, (unsigned)a->colDiffuse);
        if (only)                                       // (test) with WR_AILOGCLASS: its speeds and goal
            printf("   move %.2f turn %.1f goal %.1f %.1f %.1f goalAng %.1f wr %X flags2 %X spd %.2f %.2f %.2f%c", a->moveSpeed,
                   a->turnSpeed, a->goalPos.x, a->goalPos.y, a->goalPos.z, a->goalAngle,
                   (unsigned)((aiNPC_WR *)a)->wrFlags, (unsigned)a->aiFlags2, a->speed.x, a->speed.y, a->speed.z, 10);
    }
    extern plrPLAYER *plrPlayer;
    if (plrPlayer && wrSysStat->GetPlrRecord(plrPlayer))
        printf("stats: secrets found %d of %d, kills %d\n", *(int *)(wrSysStat->GetPlrRecord(plrPlayer) + 0x78),
               *(int *)((char *)wrSysStat + 0x9C), *(int *)(wrSysStat->GetPlrRecord(plrPlayer) + 0x5C));
    for (int i = 0; aiSpawnDomList && i < aiSpawnDomList->nDomains; i++) {
        aiDOMAIN_SPAWN *d = (aiDOMAIN_SPAWN *)aiSpawnDomList->domain[i];
        if (d->domFlags & 1)
            printf("dom %s flags %X locks %d %s/%s spawned %d of %d timer %.1f ready %d%c", d->obj->name, d->domFlags, d->nLocks, d->oppClass, d->oppName, d->nSpawned, d->nSpawn, d->timer, d->IsReadySpawn(), 10);
    }
    if (gsSysCinema)
        printf("cinema: state %X suspend %d dt %.4f scale %.2f t %.2f%c", gsSysCinema->state, gsSysCinema->IsSuspendAction(), gsElapsedTime, gsTimeScale, gsSysCinema->ctrlInst ? gsSysCinema->ctrlInst->seqTime : -1.0f, 10);
    printf("ai total %d; player hp %.0f moveFlags %X body col %08X\n", n, plrPlayer ? plrPlayer->health : 0, plrPlayer ? (unsigned)plrPlayer->moveFlags : 0,
           plrPlayer && plrPlayer->body ? (unsigned)plrPlayer->body->colDiffuse : 0);
}

void tstEntsFrame()
{
    tstAiLog();
    if (const char *pc = getenv("WR_PJLDUMP"))         // (test) WR_PJLDUMP=<class>: its instances' axes, and of one
        for (int i = 0; gsScenePtr && i < 1000; i++) {  //  object WR_PJLOBJ (default the root)
            animINST *ai = gsScenePtr->InstPtr(i);
            if (ai->magic != 0x41494E53 || !ai->ent || !ai->tplName || stricmp(ai->tplName, pc))
                continue;
            entENTITY *e = ai->ent;
            float *m = e->inst->matr;
            objOBJ *o = getenv("WR_PJLOBJ") ? objFindName(e->inst->root, (char *)getenv("WR_PJLOBJ")) : e->inst->root;
            printf("pjl %s: inst Y %.2f %.2f %.2f Z %.2f %.2f %.2f pos %.1f %.1f %.1f", pc, m[4], m[5], m[6], m[8], m[9], m[10],
                   m[12], m[13], m[14]);
            if (o)
                printf(" | %s ltm Y %.2f %.2f %.2f Z %.2f %.2f %.2f", o->name ? o->name : "?", o->ltm[4], o->ltm[5], o->ltm[6],
                       o->ltm[8], o->ltm[9], o->ltm[10]);
            printf("%c", 10);
        }
    static int tptFr;                       // (test) WR_TPTDUMP=1: the teleport domains twice a second
    if (getenv("WR_TPTDUMP") && domTptList && !(tptFr++ % 30))
        for (int i = 0; i < domTptList->nDomains; i++) {
            gsDOMAIN *d = domTptList->domain[i];
            m3dV p;
            domTptList->GetUpdatePoint(0, &p);
            printf("tpt %s: domFlags %x entFlags %x locks %d ev %x nTri %d y %.2f..%.2f eye %.2f %.2f %.2f in %d\n",
                   d->obj->name, d->domFlags, d->entFlags, d->nLocks, d->pendingEvents, d->nTri, d->yMin, d->yMax,
                   p.x, p.y, p.z, d->IsBelongPoint(&p));
        }
    static int once;
    if (!once++)                            // the call a closing door makes (doorDOOR::IsReadyOpen, NPC doors)
        printf("NPC list: first %p\n", (void *)aiNPC::GetNext(0, 0));
    for (int i = 0; i < s_nEnt; i++) {
        if (!s_entInMsg[i] || !gsSysSound)  // (the entCreate ones get it from the frame dispatch below)
            send(s_ent[i], 1000, 0);        // FRAME
        s_ent[i]->inst->Validate(0x6);
    }
    if (gsSysSound)                         // the game's frame of messages (the entities above: not in msgSystem yet)
        _gsMsgProcessFrameProfile();
    // PLACEHOLDER (test) for gsRENDER_SYSTEM::BeginFrame (feb 0x00506360, not rebuilt): the particle emitters' frame
    // after the shaders' (unless the game is paused, gsAppState 0x2004000)
    partUpdate(gsCameraPtr, gsElapsedTime);
}

void tstEntsLog(entENTITY *player)
{
    m3dV pp;
    player->inst->GetPos(&pp);
    for (int i = 0; i < s_nEnt; i++) {
        entENTITY *e = s_ent[i];
        if (stricmp(e->inst->className, "door"))
            continue;
        doorDOOR *d = (doorDOOR *)e;
        m3dV p;
        e->inst->GetPos(&p);
        float dx = p.x - pp.x, dz = p.z - pp.z;
        printf("door %s flags %X seq %d time %.2f frame %.1f locks %d dist %.2f\n", e->name.str, d->flags, e->inst->seq,
               e->inst->seqTime, e->inst->frame, e->nLocks, sqrtf(dx * dx + dz * dz));
    }
    if (!gsSysSound)
        return;
    gsDOMAIN_LIST *lists[2] = { gsSysSound->sndDomains, gsSysSound->musDomains };
    for (int l = 0; l < 2; l++)                     // (test) the sound / music domains: inside, channels, fade
        for (int i = 0; i < lists[l]->nDomains; i++) {
            gsDOMAIN *d = lists[l]->domain[i];
            printf("%s %s: inside %d (flags %X) box x %.1f..%.1f z %.1f..%.1f y %.1f..%.1f", l ? "domain_mus" : "domain_snd",
                   d->obj->name, d->domFlags & DOMF_ACTIVE, d->domFlags, d->bbox.min[0], d->bbox.max[0], d->bbox.min[2],
                   d->bbox.max[2], d->yMin, d->yMax);
            m3dV up;
            int got = lists[l]->GetUpdatePoint(0, &up);
            printf(" list state %X, point %d (%.1f %.1f %.1f) belongs %d nTri %d", lists[l]->_9c, got, up.x, up.y, up.z,
                   got ? d->IsBelongPoint(&up) : -1, d->nTri);
            if (!l) {
                gsDOMAIN_SND *s = (gsDOMAIN_SND *)d;
                printf(" fade %.2f/%.2f", s->fade, s->fadeTime);
                for (int k = 0; k < d->nSndChannels; k++) {
                    sncCHANNEL *c = gsSysSound->entChannels->ch[d->sndChannels[k]];
                    if (c)
                        printf(" | chn %s: %d sounds, flags %X, playing %d, %.1f dB", c->name.str, c->nDescr, c->flags,
                               c->IsPlay(-1), c->volume);
                }
            }
            printf("\n");
        }
}

// (test) --domtest: the player into the first sound domain (the middle of its outline, at its floor)
void tstEntsToDomain(entENTITY *player)
{
    if (!gsSysSound || !gsSysSound->sndDomains->nDomains)
        return;
    gsDOMAIN *d = gsSysSound->sndDomains->domain[0];
    m3dV p = { 0.5f * (d->bbox.min[0] + d->bbox.max[0]), d->yMin + 0.2f, 0.5f * (d->bbox.min[2] + d->bbox.max[2]) };
    m3dV dir = { 0, 0, 1 };
    player->inst->SetPosDir_XZ(&p, &dir);
    printf("domtest: player to %s at %.1f %.1f %.1f\n", d->obj->name, p.x, p.y, p.z);
}

// (test) --cintest [domain]: the player into an action domain (by its object's name, else the first), as walking in
void tstEntsToActDomain(entENTITY *player, const char *name)
{
    if (!domActList)
        return;
    for (int i = 0; i < domActList->nDomains; i++) {
        gsDOMAIN *d = domActList->domain[i];
        if (name && *name && (!d->obj || !d->obj->name || stricmp(d->obj->name, name)))
            continue;
        m3dV p = { 0.5f * (d->bbox.min[0] + d->bbox.max[0]), d->yMin + 0.2f, 0.5f * (d->bbox.min[2] + d->bbox.max[2]) };
        m3dV dir = { 0, 0, 1 };
        player->inst->SetPosDir_XZ(&p, &dir);
        printf("cintest: player into %s at %.1f %.1f %.1f (%d action domains)%c", d->obj ? d->obj->name : "?", p.x,
               p.y, p.z, domActList->nDomains, 10);
        return;
    }
    printf("cintest: no action domain %s (%d)%c", name ? name : "", domActList->nDomains, 10);
}

// (test) the cinematics: the system's state, its cinemas' locks, the playing one's frame, the camera
void tstCinemaLog()
{
    cinSYSTEM *c = gsSysCinema;
    if (!c)
        return;
    printf("cinema: state %X cinemas %d", c->state, c->nCinemas);
    for (int i = 0; i < c->nCinemas; i++)
        if (c->cinema[i])
            printf(" [%s locks %d]", c->cinema[i]->inst && c->cinema[i]->inst->name ? c->cinema[i]->inst->name : "?",
                   c->cinema[i]->nLocks);
    if (c->state & 1) {
        m3dV e, d;
        gsCameraPtr->GetOrigin(&e);
        gsCameraPtr->GetVForw(&d);
        printf(" playing frame %.1f (%.0f..%.0f) key %d/%d alpha %.0f cam %.1f %.1f %.1f dir %.2f %.2f %.2f", c->ctrlInst->frame,
               c->curData->frameStart, c->curData->frameEnd, c->curFrame, c->nFrames,
               gsSysRender ? gsSysRender->fullScrColor[3] : -1.0f, e.x, e.y, e.z, d.x, d.y, d.z);
    }
    printf("%c", 10);
}

// (test) --watertest: the player into the first water volume (the middle of its outline, 1 below the surface)
void tstEntsToWater(entENTITY *player)
{
    for (int i = 0; i < 30; i++) {
        waterLIQUID *w = waterTable[i];
        if (!w || !w->fog || !w->fog->nDomain)
            continue;
        scnDOMAIN *d = &w->fog->domain[0];
        m3dV p = { 0.5f * (d->bbox.min[0] + d->bbox.max[0]), d->yMax - 1.0f, 0.5f * (d->bbox.min[2] + d->bbox.max[2]) };
        m3dV dir = { 0, 0, 1 };
        player->inst->SetPosDir_XZ(&p, &dir);
        printf("watertest: player into %s at %.1f %.1f %.1f\n", w->name.str, p.x, p.y, p.z);
        return;
    }
    printf("watertest: no water volume\n");
}

// (test) the water volumes: table, fog volume, modifiers, how far the surface vertices are from their rest place,
// and whether a point (the player's eye) is under water
void tstWaterLog(entENTITY *player)
{
    m3dV pp;
    player->inst->GetPos(&pp);
    pp.y += 1.6f;
    float level = 0;
    int under = waterIsUnderWater(&pp, &level, 0.3f);
    for (int i = 0; i < 30; i++) {
        waterLIQUID *w = waterTable[i];
        if (!w)
            continue;
        printf("water[%d] %s class %X maxAmp %.2f", i, w->name.str, w->unkBC, w->maxAmp);
        if (w->fog)
            for (int k = 0; k < w->fog->nDomain; k++)
                printf(" | vol %d y %.2f..%.2f x %.1f..%.1f z %.1f..%.1f", k, w->fog->domain[k].yMin, w->fog->domain[k].yMax,
                       w->fog->domain[k].bbox.min[0], w->fog->domain[k].bbox.max[0], w->fog->domain[k].bbox.min[2],
                       w->fog->domain[k].bbox.max[2]);
        objOBJ *objs[0x32];
        int n = objMakePtrList(&w->inst->root, 1, objs, 0x32);
        for (int k = 0; k < n; k++) {
            objOBJ *o = objs[k];
            if (!o->geom || !o->geom->nModifier)
                continue;
            float dy = 0;
            for (int m = 0; m < o->geom->nModifier; m++) {
                objMODIFIER *md = o->geom->modifier[m];
                if (md->type == 0x4E444546)             // NDEF: the waves
                    for (int v = 0; v < o->nVert; v++) {
                        float d = o->geom->vert[v].y - ((objMOD_NOISE_DEFORM *)md)->vertWCS[v].y;
                        if (d > dy)
                            dy = d;
                    }
            }
            printf(" | %s: %d modifiers, state %X, waves up to %.3f, vert[0].y %.4f", o->name, o->geom->nModifier, o->state, dy,
                   o->nVert ? o->geom->vert[0].y : 0.0f);
        }
        printf("\n");
    }
    printf("water: eye %.2f %.2f %.2f under %d (surface %.2f)\n", pp.x, pp.y, pp.z, under, level);
}

// PLACEHOLDER (test side) for gsSysLevel, the level system (wrLVL_SYSTEM; its ctor v12 0x00536700 and most slots
// are not rebuilt): the zeroed block s_lvlBuf with a vtable laid out as this build's msgADDR classes are
// (WR_MSGADDR_VTBL_SHIFT). Set up slots:
//   +0x04 ProcessMsg          -> nothing (0)
//   +0x24 FindTplName         -> the scene's template of that name, else database\<name>.tpl loaded into a free slot
//                                of the scene's pool (the real one, v12 0x00537230, loads from the PathDB and then
//                                assigns the modifiers, builds the LODs, normals and shadow data: not done here)
//   +0x28 ObjLoadNotify       -> the real SetExtraObjFlags on the object and its subtree ("&bb" skipped)
//   +0x2C InstLoadNotify      -> ObjLoadNotify on the instance's tree, the "loaded" flag 0x20000 (the real one,
//                                v12 0x00535700, also starts the instance's shaders, shadows and render data)
//   +0x34 InstDestroyNotify   -> nothing
// Any other slot stops the test with its offset.
static void *s_lvlVtbl[32];
#define LVL_VT(off) s_lvlVtbl[((off) + WR_MSGADDR_VTBL_SHIFT) / 4]
static scnSCENE *s_lvlScene;
static void __thiscall lvlTrap(gsLVL_SYSTEM *)
{
    fprintf(stderr, "gsSysLevel slot not set up in the test\n");
    ExitProcess(3);
}
static BOOL32 __thiscall lvlProcessMsg(gsLVL_SYSTEM *, int, void *, msgADDR *, int *) { return 0; }
static animTPL *__thiscall lvlFindTplName(gsLVL_SYSTEM *, char *name)
{
    scnSCENE *s = s_lvlScene;
    if (!s || !name)
        return 0;
    if (!name[0]) {                                     // (the real one too: no name, the scene's "__DUMMY__")
        if (!s->dummyTpl->root)                         // PLACEHOLDER: an empty object for the instance to hold
            s->dummyTpl->root = new objOBJ;
        return s->dummyTpl;
    }
    animTPL *t = findTpl(s, name);
    if (t)
        return t;
    int slot;
    for (slot = 0; slot < 300 && s->tplPool[slot].magic; slot++)
        ;
    char path[512];
    snprintf(path, sizeof(path), "%s\\database\\%s.tpl", tstEntsGameDir, name);
    animTPL *f = slot < 300 && GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES ? tstLoadTemplate(path) : 0;
    if (!f) {
        printf("level: cannot load template %s\n", path);
        return 0;
    }
    memcpy((void *)&s->tplPool[slot], (void *)f, sizeof(animTPL));
    t = &s->tplPool[slot];
    strncpy(t->tplName, name, 0x3F);
    if (!tplAfterLoad(s, t, name))
        return 0;
    return t;
}

// FindTplName's steps after a template is read, those rebuilt (in its order): the splines, the skins onto their
// objects, [ANIM_PROP] nameClass, the scene's vertex buffer, the hit boxes (not: objBuildLOD, AddExtraObjMtls,
// SetTplObjFlags, animAllocNormalsLCS, the shadow template). Also for loadClassTpls' templates.
static int tplAfterLoad(scnSCENE *s, animTPL *t, const char *name)
{
    animOptimizeSpl(t);
    if (!t->root->AssignModifiers(t->nSkin, (objMODIFIER **)t->skinList))
        printf("level: error assigning object modifiers (%s)\n", name);
    if (t->props)
        t->props->GetStr((char *)"ANIM_PROP", (char *)"nameClass", t->className, 0x40);
    if (!s->ExpandV3DTmpList(t->root))
        return 0;
    if (!t->AbvInit(0))
        return 0;
    objFlags((gsLVL_SYSTEM *)s_lvlBuf, t->root);
    return 1;
}
static void lvlMirrorTree(objOBJ *o)                    // (the real notify calls InitObj for each object)
{
    if (gsSysMirror)
        gsSysMirror->InitObj(o);
    objOBJ *c = o->child;
    if (c)
        do {
            lvlMirrorTree(c);
            c = c->next;
        } while (c != o->child);
}
static void __thiscall lvlObjLoadNotify(gsLVL_SYSTEM *l, objOBJ *o) { objFlags(l, o); lvlMirrorTree(o); }
int __fastcall dntInit();                               // src/Sources/day_night.cpp
BOOL32 __fastcall dntInitInst(animINST *i);
static void __thiscall lvlInstLoadNotify(gsLVL_SYSTEM *l, animINST *i)
{
    if (i->flags & 0x20000)
        return;
    if (i->root) {
        objFlags(l, i->root);
        objTexAnim(l, i->root);
        lvlMirrorTree(i->root);
    }
    i->flags |= 0x20000;
    if (getenv("WR_DNT"))
        dntInitInst(i);                                 // wrLVL_SYSTEM::InstLoadNotify's (v12 0x004ABA5B)
}
// the destroy notifies: the rebuilt ones (gs_level.cpp) in the test level system's vtable
static void tstEntsForget(animINST *a);
static void __thiscall lvlInstDestroyNotify(gsLVL_SYSTEM *l, animINST *a)
{
    l->gsLVL_SYSTEM::InstDestroyNotify(a);
    tstEntsForget(a);                                   // (test) its entity off the test's own list
}
static void __thiscall lvlObjDestroyNotify(gsLVL_SYSTEM *l, objOBJ *o) { l->gsLVL_SYSTEM::ObjDestroyNotify(o); }
static void __thiscall lvlTplDestroyNotify(gsLVL_SYSTEM *l, int all) { l->gsLVL_SYSTEM::TplDestroyNotify(all); }
static void __thiscall lvlTplDestroyNotifyTpl(gsLVL_SYSTEM *l, animTPL *t) { l->gsLVL_SYSTEM::TplDestroyNotify(t); }
// PLACEHOLDER (test) for mrrMIRROR_SYSTEM::TermObj (v12 0x0051E0D0) and pshSHADOW_SYSTEM::TermInst (v12 0x00517580),
// not rebuilt: the test has no mirrors and no projected shadows, so there is nothing in their lists to take out
void pshSHADOW_SYSTEM::TermInst(animINST *) {}
// (the same: the body's exclusion lists and merged shadow sources (v12 0x00517630 / 0x00517780), no active mirror)
int  pshSHADOW_SYSTEM::AddExcludeList(animINST *, int id) { return id; }
void pshSHADOW_SYSTEM::MergeShadowSources(animINST *, animINST *) {}
void pshSHADOW_SYSTEM::UpdateClear(void *) {}           // (no projected shadows in the test)

// PLACEHOLDER (test) for gsLVL_SYSTEM::UpdatePreloadData (v12 0x005436E0, the preload lists, not rebuilt): with the
// preload info switched off (the test's level system has no lists, +0x440 0) the game's returns at once too.
void gsLVL_SYSTEM::UpdatePreloadData(int) {}

// PLACEHOLDER (test renderer) for the render driver's vtable: the test draws with its own renderer (plat_d3d7) and
// keeps no buffers per scene, so rendDRIVER::TermLevel (slot +0x9C, v12 0x004EA7A0) has nothing to free. Any other
// slot stops the test with its offset.
static void *s_rendVtbl[64];
static void __thiscall rendTrap(void *)
{
    fprintf(stderr, "rendDrv slot not set up in the test%c", 10);
    ExitProcess(3);
}
static void __thiscall rendTermLevel(void *, scnSCENE *) {}
static int __thiscall rendFilter(void *, void *) { return 0; }   // slots +0x18 FilterObj / +0x1C FilterInst: the
                                                                 //   base driver's "return 0" (v12 0x00403200)
static void __thiscall rendTermInst(void *, animINST *) {}    // slot +0xA4 (the driver's per-instance data)
static void __thiscall rendTermObj(void *, objOBJ *) {}       // slot +0xAC (per-object data)
// PLACEHOLDER (test renderer) for the slots the mirrors use: BeginCamera (+0x30), EndCamera (+0x34), ConfigureClipPlane
// (+0x60), EnableClipPlane (+0x64), on the test renderer's transforms and D3D clip plane
void tstSetCameraView(camCAMERA *c);
void rndClipPlane(const float *plane);
static void __thiscall rendBeginCamera(rendDRIVER *r, camCAMERA *c, float, float)
{
    r->camera = c;
    c->CalcPOVQual();
    tstSetCameraView(c);
}
static void __thiscall rendEndCamera(rendDRIVER *) {}
static void __thiscall rendClipPlane(rendDRIVER *r, m3dV *org, m3dV *nrm)
{
    r->clipOrg = *org;
    r->clipNrm = *nrm;
}
static void __thiscall rendEnableClip(rendDRIVER *r, int on)
{
    if (on) {                                           // (the normal points to the side that stays)
        m3dV &n = r->clipNrm, &o = r->clipOrg;
        float p[4] = { n.x, n.y, n.z, -(n.x * o.x + n.y * o.y + n.z * o.z) };
        r->flags |= 0x10;
        rndClipPlane(p);
    } else {
        r->flags &= ~0x10u;
        rndClipPlane(0);
    }
}
void tstRendDrvEnsure()
{
    if (!rendDrv)
        rendDrv = (rendDRIVER *)apCalloc(1, 0x2000);
    if (*(void ***)rendDrv)
        return;
    for (int i = 0; i < 64; i++)
        s_rendVtbl[i] = (void *)rendTrap;
    s_rendVtbl[0x9C / 4] = (void *)rendTermLevel;
    s_rendVtbl[0x18 / 4] = (void *)rendFilter;
    s_rendVtbl[0x1C / 4] = (void *)rendFilter;
    s_rendVtbl[0xA4 / 4] = (void *)rendTermInst;
    s_rendVtbl[0xAC / 4] = (void *)rendTermObj;
    s_rendVtbl[0x30 / 4] = (void *)rendBeginCamera;
    s_rendVtbl[0x34 / 4] = (void *)rendEndCamera;
    s_rendVtbl[0x60 / 4] = (void *)rendClipPlane;
    s_rendVtbl[0x64 / 4] = (void *)rendEnableClip;
    *(void ***)rendDrv = s_rendVtbl;
}

void tstLevelSystemInit(scnSCENE *s)
{
    tstRendDrvEnsure();                                 // (the destroy notifies reach the driver mid-level too)
    for (int i = 0; i < 32; i++)
        s_lvlVtbl[i] = (void *)lvlTrap;
    LVL_VT(0x04) = (void *)lvlProcessMsg;
    LVL_VT(0x24) = (void *)lvlFindTplName;
    LVL_VT(0x28) = (void *)lvlObjLoadNotify;
    LVL_VT(0x2C) = (void *)lvlInstLoadNotify;
    LVL_VT(0x34) = (void *)lvlInstDestroyNotify;
    LVL_VT(0x30) = (void *)lvlObjDestroyNotify;
    LVL_VT(0x38) = (void *)lvlTplDestroyNotify;
    LVL_VT(0x3C) = (void *)lvlTplDestroyNotifyTpl;
    *(void ***)s_lvlBuf = s_lvlVtbl;
    gsSysLevel = (gsLVL_SYSTEM *)s_lvlBuf;
    s_lvlScene = s;
}

// PLACEHOLDER (test side) for the weapon loop of plrPLAYER_COMMON::ProcessINIT (v12 0x0042738C; the test player
// does not run its INIT): the classes registered by pwpInit's per-weapon Init (only the rebuilt ones), then, as the
// loop does, entCreate(scene, "pwp_*", "pwp_*", 0, 0, 0), the owner (+0x1A3) and Enable(0) (slot +0xA8); then
// SetDefaultValues' part for a starting weapon (Enable(1)) and SetCurWpn "at once" (the model shown).
int __fastcall pwpSVLInit();                            // plw_svl.cpp
int __fastcall pwpBulWpnInit();                         // plw_bul_wpn.cpp
int __fastcall pwpCLTInit();                            // plw_clt.cpp
int __fastcall pwpMDGInit();                            // plw_mdg.cpp
int __fastcall pwpBMBInit();                            // plw_bmb.cpp
int __fastcall pwpMCGInit();                            // plw_mcg.cpp
int __fastcall pwpSTGInit();                            // plw_stg.cpp
int __fastcall pwpCRBInit();                            // plw_crb.cpp
int __fastcall pwpBSKInit();                            // plw_bsk.cpp
int __fastcall pwpMNGInit();                            // plw_mng.cpp

// PLACEHOLDER (test) for the acid gun, the minigun and the atomic gun (pwpACIDGUN / pwpMINIGUN / pwpATOMICGUN, not
// rebuilt): a plain pwpWEAPON (idle, run, put on, take off -- no shot) with the 1.2 INIT's clip and ammo (weapons.h
// table), so the player can hold them and the body can carry their "_item" models.
class tstWPN_STANDIN : public pwpWEAPON {
public:
    int ProcessINIT()
    {
        if (!pwpWEAPON::ProcessINIT())
            return 0;
        static const struct { int idx, clip; float maxAmmo, damage; } v[] = {
            { PWP_ACIDGUN, 5, 100.0f, 60.0f }, { PWP_MINIGUN, 100, 1000.0f, 8.0f }, { PWP_ATOMICGUN, 4, 20.0f, 200.0f } };
        for (int i = 0; i < 3; i++)
            if (v[i].idx == wpnIndex) {
                clipSize = clip = v[i].clip;
                maxAmmo = v[i].maxAmmo;
                damage = v[i].damage;
            }
        if (wpnIndex == PWP_ACIDGUN) {                  // as pwpACIDGUN::ProcessINIT (feb 0x00401410): the bubbles
            char n[32];                                 //   in the canister drawn additive (render state 0x68)
            for (int k = 1; k <= 2; k++) {
                sprintf(n, "bubbles_plane%d", k);
                objOBJ *o = objFindNameN(inst->root, n, (int)strlen(n));
                if (!o)
                    break;
                o->SetStateRendYes(0x68);
            }
        }
        return 1;
    }
    static entENTITY *__fastcall CreateAcd(animINST *) { tstWPN_STANDIN *w = new tstWPN_STANDIN; w->wpnIndex = PWP_ACIDGUN; return w; }
    static entENTITY *__fastcall CreateMng(animINST *) { tstWPN_STANDIN *w = new tstWPN_STANDIN; w->wpnIndex = PWP_MINIGUN; return w; }
    static entENTITY *__fastcall CreateAmg(animINST *) { tstWPN_STANDIN *w = new tstWPN_STANDIN; w->wpnIndex = PWP_ATOMICGUN; return w; }
};

// PLACEHOLDER (test) for plrInit's body class (v12 0x0041D453: entRegisterClass "player_mp", 'PLYR', 0x32,
// plrPLAYER_MP::Create); plrCommInit / pwpInit are the weapons' own Inits here
int tstPlayerMpInit()
{
    static int done;
    if (done)
        return 1;
    done = 1;
    return entRegisterClass((char *)"player_mp", 0x504C5952, 0x32, (entENTITY *(__fastcall *)(animINST *))plrPLAYER_MP::Create,
                            0, 1);
}

pwpWEAPON *tstEntsWeapon(scnSCENE *s, plrPLAYER *p, const char *cls)
{
    static int reg;
    if (!txmManager || !gsSysSound)
        return 0;
    if (!reg) {
        // (the game: partInit with the engine, pwpInit's per-weapon Init at start-up -- the rebuilt ones only)
        if (!plrCommInit())                             // the player's sounds (plrInit v12 0x0041D420 runs it first)
            printf("player: plrCommInit failed%c", 10);
        if (!pwpSVLInit() || !pwpBulWpnInit() || !pwpCLTInit() || !pwpMDGInit() || !pwpBMBInit() || !pwpMCGInit()
            || !pwpSTGInit() || !pwpCRBInit() || !pwpBSKInit()) {
            printf("weapon: a pwp*Init failed\n");
            return 0;
        }
        entRegisterClass((char *)"pwp_acd", 0x50414344, 0x50, tstWPN_STANDIN::CreateAcd, 0, 1);   // (test) stand-ins
        pwpMNGInit();                                   // the minigun (plw_mng.cpp)
        entRegisterClass((char *)"pwp_amg", 0x50414D47, 0x50, tstWPN_STANDIN::CreateAmg, 0, 1);
        reg = 1;
    }
    if (!gsSysLevel)
        tstLevelSystemInit(s);
    s_lvlScene = s;
    char tpl[0x40];
    strncpy(tpl, cls, 0x3F);
    tpl[0x3F] = 0;
    pwpWEAPON *w = (pwpWEAPON *)entCreate(s, (char *)cls, tpl, 0, 0, 0);
    if (!w) {
        printf("weapon %s: not created\n", cls);
        return 0;
    }
    w->owner = p;
    w->entFlags |= 4;                       // (test) on the server, as the player (single player: gsMP_SYSTEM::
                                            //  SetupMPStates v12 0x0050DCC0 gives every entity 6; not run here)
    w->Enable(0);
    w->Enable(1);
    w->inst->flags &= ~1u;                  // shown (SetCurWpn)
    float sc = w->inst->GetScale();
    if (getenv("WR_SEQDUMP"))                           // (test) the weapon's sequences after SetupAniSystem
        for (int k = 0; k < w->inst->tpl->nSeq; k++) {
            animSEQ *q = &w->inst->tpl->seq[k];
            printf("  seq %2d %-26s frames %6.1f..%6.1f  %6.0f ms dir %d act %d %d\n", k, q->name, q->frameStart,
                   q->frameEnd, q->lengthMs, q->dir, q->actionFrame[0], q->actionFrame[1]);
        }
    char am[16];
    if (w->wpnWEAPON::flags & WPNF_INFINITE_AMMO)       // the colt (its ProcessINIT); the HUD shows "- / 14"
        snprintf(am, sizeof am, "infinite");
    else
        snprintf(am, sizeof am, "%.0f", w->ammo);
    printf("weapon %s: seq %d, clip %d, ammo %s, scale %.3f, lods %d\n", cls, w->GetCurAnimSeq(), w->clip, am, sc,
           w->inst->lods != 0);
    return w;
}

// PLACEHOLDER (test side) for the hit effects' Create functions (pwpSVL_HIT::Create v12 0x004153C0 "sfx_shovel",
// pwpMCG_HIT::Create "sfx_mcg_hit"): their sparks / parts / smoke are particle emitters (engine part.obj, not
// rebuilt). They make nothing, so the real entCreate drops the effect's instance again; the shot itself is not
// affected (Shoot does not use the result). DELETE when plw_svl.cpp / plw_bul_wpn.cpp have them.
entENTITY *__fastcall pwpSVL_HIT_Create(animINST *i)
{
    printf("sfx_shovel: hit effect not created (particles not rebuilt)\n");
    return 0;
}
entENTITY *__fastcall pwpMCG_HIT_Create(animINST *i)
{
    printf("sfx_mcg_hit: hit effect not created (particles not rebuilt)\n");
    return 0;
}
// PLACEHOLDER (test side) for aiSKEL_RIGID_START::StartExplos (feb 0x004507D0; rgdRIGID_START, shtSHATTER and the rigid
// bodies are not rebuilt): no pieces fly, the skeleton is simply gone when its entity ends. DELETE with phys_rigid.
void aiSKEL_RIGID_START::StartExplos(entENTITY *)
{
}

// PLACEHOLDER (test side) for wrsfxSCORCHMARK::Add (v12 0x00467DE0, sfx_appear.cpp): the bullet hole / burn decal, a
// pteSPOT shader fading in and out -- the projected spot shaders (pte_spot*.obj) are not rebuilt. No mark is made
// (0, as Add returns when a mark is already near). DELETE when sfx_appear.cpp has it.
struct wrsfxSCORCHMARK {
    static int __fastcall Add(float size, float b, txmTEXTURE *tex, m3dV *pos, u32 color, int sig, float rad, float d);
};
int __fastcall wrsfxSCORCHMARK::Add(float, float, txmTEXTURE *, m3dV *, u32, int, float, float)
{
    static int once;
    if (!once++)
        printf("scorchmark: not made (the pteSPOT shaders are not rebuilt)\n");
    return 0;
}


int tstEntsCount() { return s_nEnt; }
// (test) an instance going away (a vase broken, a hit effect dropped): its entity leaves the list tstEntsFrame walks
static void tstEntsForget(animINST *a)
{
    for (int i = 0; i < s_nEnt; i++)
        if (s_ent[i] && s_ent[i]->inst == a) {
            s_ent[i] = s_ent[s_nEnt - 1];
            s_entInMsg[i] = s_entInMsg[s_nEnt - 1];
            s_nEnt--;
            i--;
        }
}
entENTITY *tstEnt(int i) { return s_ent[i]; }

void tstLevelSystemSetScene(scnSCENE *s)
{
    s_lvlScene = s;
}
