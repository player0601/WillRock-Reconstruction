// cinema.cpp -- gs_shared.lib in-engine cinematics: cinSYSTEM, cinDATA_ENTITY ("cinema"), cinACTOR, cinFRAME and the
// cinACN_* actions (Will Rock 1.2, v12.exe 0x00544100..0x005461A0; feb obj gs_shared:cinema.obj 0x005414F0..)
//
// Rebuilt from the 1.2 code, the beta's decompile as the guide. 1.2 changes against feb: the cinema entity grew
// isSuspendAction (+0x16B, [CINEMA] isSuspendAction, default 1) which decides the game's mode 4 while it plays;
// cinSYSTEM keeps the playing entity at +0x88 (every field after it moved by 4); the skip key no longer jumps the
// cinema's sequence to its end but makes the next frame that long (gsMsgSetNextElapsedTime), so everything that
// runs with it gets there too; Stop / IsSuspendAction are new; the message ids are 2 higher; the actors play with
// AdvanceAnimation's 1.2 signature. Layout in include/game_systems/cinema.h.

#include "../Sources/game.h"
#include "../m3d/m3d_spl.h"
#include "../gs/gs_mp_misc.h"

static_assert(sizeof(cinSYSTEM) == 0xCE0 && offsetof(cinSYSTEM, curEnt) == 0x88 && offsetof(cinSYSTEM, actor) == 0x9C &&
              offsetof(cinSYSTEM, frame) == 0x280 && offsetof(cinSYSTEM, lastFrame) == 0xCA8 &&
              offsetof(cinSYSTEM, curFrame) == 0xCDC, "cinSYSTEM");
template <int N> struct cinShow;
static_assert(offsetof(cinDATA_ENTITY, isSuspendAction) == 0x16B, "cinDATA_ENTITY");
static_assert(sizeof(cinACN_SOUND) == 0x138, "cinACN_SOUND");
static_assert(sizeof(cinACN_COMMAND) == 0xC0, "cinACN_COMMAND");
static_assert(sizeof(cinACN_CREATE_ENT) == 0xC4, "cinACN_CREATE_ENT");

cinSYSTEM *gsSysCinema;                                 // v12 0x0098E840 (feb 0x00948240)

// v12 0x00544100  (feb 0x005414F0)
int __fastcall cinInit()
{
    if (!entRegisterClass((char *)"cinema", 0x43494E45, 0x4F, cinDATA_ENTITY::Create, 0, 1))
        return 0;
    gsSysCinema = new cinSYSTEM;
    msgSystem.InsertAddr(gsSysCinema, 0, 0);
    return 1;
}

// v12 0x00544160  (feb 0x00541550)
void __fastcall cinTerm()
{
    msgSystem.DestroyAddr(gsSysCinema);
    gsSysCinema = 0;
}

// ---------------------------------------------------------------------------------------------- cinDATA_ENTITY

// v12 0x00544180  (feb 0x00541570)
// On INIT the sheet's lock count (the level's UNLOCK IActions bring it to 0: the cinema starts) and whether the game
// waits for it; its instance is excluded from the hierarchy (0x1000). (1.2 also looks for "lev_2_5_" in the level's
// name and does nothing with it.)
BOOL32 cinDATA_ENTITY::ProcessMsg(int id, void *, msgADDR *, int *)
{
    if (id == 1) {
        inst->GetInt((char *)"CINEMA", (char *)"nmbLocks", &nLocks);
        inst->flags |= 0x1000;
        inst->GetInt((char *)"CINEMA", (char *)"isSuspendAction", &isSuspendAction);
        strstr(gsSceneFullName, "lev_2_5_");
    }
    return 0;
}

// v12 0x00546170  (feb 0x005433C0)
entENTITY *__fastcall cinDATA_ENTITY::Create(animINST *)
{
    cinDATA_ENTITY *e = new cinDATA_ENTITY;            // operator new(0x16F), entENTITY ctor
    if (!e)
        return 0;
    e->isSuspendAction = 1;
    return e;
}

// ---------------------------------------------------------------------------------------------- cinSYSTEM

// v12 0x005441F0  (feb 0x005415B0)
cinSYSTEM::cinSYSTEM()
{
    node.prev = 0;
    node.next = 0;
    msgADDR::flags = 0;
    magic = 0x4D414452;
    classId = 0x43494E45;
    unk18 = 0;
    priority = 0x50;
    sign = 0;
    addrLock = 0;
    state = 0;
    fade = 0;                                           // (v12 +0x94; feb zeroed its +0x90, the same field)
    for (int i = 0; i < 20; i++) {
        actor[i].inst = 0;
        actor[i].conInst = 0;
        actor[i].conObj = 0;
        actor[i].seq = -1;
        actor[i].noAnimSys = 0;
        actor[i].chn = 0;
    }
    for (int i = 0; i < 50; i++) {
        frame[i].time = -1.0f;
        frame[i].nAct = 0;
        frame[i].cameraCut = 0;
        for (int k = 0; k < 10; k++)
            frame[i].act[k] = 0;
    }
    lastFrame.time = -1.0f;
    lastFrame.nAct = 0;
    lastFrame.cameraCut = 0;
    for (int k = 0; k < 10; k++)
        lastFrame.act[k] = 0;
    for (int i = 0; i < 20; i++)
        cinema[i] = 0;
    nCinemas = 0;
    ResetData();
    msgADDR::flags |= 2;                                // MSGADDR_WANT_DESTROY_NOTIFY (+0x0C, not the state alias)
    inpCtx = 0;
}

// v12 0x00544300  (feb 0x005416C0)
// FRAME, INIT_LEVEL; TERM_LEVEL forgets the cinemas; a destroyed entity is dropped from the list and from the actors;
// DESTROY (2) is refused.
BOOL32 cinSYSTEM::ProcessMsg(int id, void *data, msgADDR *, int *)
{
    if (id > GS_MSG_FRAME) {
        if (id == GS_MSG_INIT_LEVEL)
            ProcessINIT_LEVEL();
        else if (id == GS_MSG_TERM_LEVEL) {
            for (int i = 0; i < nCinemas; i++)
                cinema[i] = 0;
            nCinemas = 0;
            ResetData();
        }
        return 0;
    }
    if (id == GS_MSG_FRAME) {
        ProcessFRAME();
        return 0;
    }
    if (id == 2)
        return 1;
    if (id == 3) {                                      // MSG_MSG_DESTROY_ADDR
        for (int i = 0; i < nCinemas; i++)
            if (cinema[i] == data)
                cinema[i] = 0;
        for (int i = 0; i < 20; i++)
            if (actor[i].inst && (void *)actor[i].inst->ent == data)
                actor[i].Term();
    }
    return 0;
}

// v12 0x005443D0  (feb 0x00541790)
// In single player, with an animated scene camera: every "cinema" entity with a sheet.
int cinSYSTEM::ProcessINIT_LEVEL()
{
    if (!gsScenePtr->camera || gsSysMP->mode != 1)
        return 1;
    for (entENTITY *e = entFindNext(0); e; e = entFindNext(e))
        if (e->classId == 0x43494E45 && e->inst && (e->inst->props || e->inst->tpl->props))
            cinema[nCinemas++] = (cinDATA_ENTITY *)e;
    return 1;
}

// v12 0x00544450  (feb 0x00541810)
// Off (state 4 clear: not single player): the game's input context back. Not playing: the first cinema without locks
// starts (its data made, "playing", gsAppState 0x200, the sequence from 0, the game suspended for it, CINEMA_START).
// Playing: input on the "CIN" context; its first command (skip) more than 0.5 s in, with more than 0.2 s left, makes
// the next frame end it; the sequence runs (to its end: state 2); the scene camera follows the frame (a camera-cut
// frame holds it for one frame); the next key frame's actions as it passes; the actors; the fade. At the end: the
// last frame's actions, the actors released, the camera's angle back, the music released, CINEMA_END, the game on.
void cinSYSTEM::ProcessFRAME()
{
    if (!(state & 4)) {
        if (gsSysInput->curCtx == inpCtx)
            gsSysInput->SetDefCtx();
        return;
    }
    if (!(state & 1)) {
        for (int i = 0; i < nCinemas; i++) {
            if (!cinema[i] || cinema[i]->nLocks > 0)
                continue;
            CreateData(cinema[i]);
            cinema[i]->nLocks = 1;
            state |= 1;
            gsAppState |= 0x200;
            curFrame = 0;
            ctrlInst->SetAnimSeq(0, 0.0f);
            if (curEnt->isSuspendAction)
                gsMsgChangeMode(4);
            msgSystem.SendAll(GS_MSG_CINEMA_START, 0, 0);
        }
        if (!(state & 1)) {
            if (gsSysInput->curCtx == inpCtx)
                gsSysInput->SetDefCtx();
            return;
        }
    }
    if (!inpCtx)
        inpCtx = gsSysInput->GetCtxByName("CIN");
    if (gsSysInput->curCtx != inpCtx)
        gsSysInput->SetCurCtx(inpCtx);
    if (gsSysInput->IsOn(0) && gsSysInput->IsToggled(0) && ctrlInst->seqTime > 0.5f) {
        float left = curData->lengthMs * 0.001f - ctrlInst->seqTime;
        if (left > 0.2f)
            gsMsgSetNextElapsedTime(left - 0.1f);
    }
    if (ctrlInst->AdvanceTime(gsElapsedTime, 1, 0))
        state |= 2;
    cinFRAME *f = &frame[curFrame];
    float t = ctrlInst->frame;
    if (f->cameraCut && !(f->time > ctrlInst->frame) && f->time + 1.0f > ctrlInst->frame)
        t = f->time;
    gsScenePtr->camera->Animate(t, gsCameraPtr);
    if (curFrame < nFrames && frame[curFrame].time < ctrlInst->frame) {
        f = &frame[curFrame];
        for (int k = 0; k < f->nAct; k++)
            f->act[k]->UpdateFrame((int)f->time);
        curFrame++;
    }
    for (int i = 0; i < 20; i++)
        if (actor[i].inst)
            actor[i].UpdateFrame();
    float a;
    fade->GetValue(ctrlInst->frame, &a, 0);
    a *= 255.0f;
    gsSysRender->SetColorFullScr((unsigned)(int)a << 24);
    if (!(state & 2))
        return;
    for (int k = 0; k < lastFrame.nAct; k++)
        lastFrame.act[k]->UpdateFrame((int)lastFrame.time);
    state &= ~1u;
    gsAppState &= ~0x200;
    for (int i = 0; i < nActors; i++)
        actor[i].Term();
    gsCameraPtr->SetAngleHor(75.0f);                    // (the shipped value; this exe's widescreen patch changes it)
    gsSysSound->ReleaseMusic();
    msgSystem.SendAll(GS_MSG_CINEMA_END, 0, 0);
    if (curEnt->isSuspendAction)
        gsMsgChangeMode(5);
    ResetData();
}

// v12 0x005448B0  (feb 0x00541C20)
// The cinema's instance, its first sequence, its sheet (the instance's, else the template's): a key frame for every
// [FRAME_n] of the sequence's frames, [FRAME_LAST], and the fade spline -- one key a frame, all dark (1).
void cinSYSTEM::CreateData(cinDATA_ENTITY *e)
{
    ctrlInst = e->inst;
    if (!ctrlInst || !(curData = ctrlInst->tpl->seq))
        return;
    curEnt = e;
    sheet = ctrlInst->props ? ctrlInst->props : ctrlInst->tpl->props;
    if (!sheet)
        return;
    nFrames = 0;
    char sect[0x40];
    int n = (int)curData->frameStart;
    for (; n < (int)curData->frameEnd; n++) {
        sprintf(sect, "FRAME_%d", n);
        if (sheet->FindSect(sect)) {
            int k = nFrames++;
            frame[k].Init((float)n, sheet, sect);
        }
    }
    if (!nFrames)
        apLogErr((char *)"No script frames found in cinematic %s!", ctrlInst->name ? ctrlInst->name : (char *)"");
    sprintf(sect, "FRAME_LAST");
    if (sheet->FindSect(sect))
        lastFrame.Init((float)n, sheet, sect);
    m3dSPL_LINEAR1D *s = new m3dSPL_LINEAR1D;
    if (s)
        m3dSPL_Init(s, 0, 8, 1);
    fade = s;
    if (!s || !s->AllocKpList((int)(curData->frameEnd - curData->frameStart + 1.0f)))
        return;
    for (int i = 0; (float)i < curData->frameEnd - curData->frameStart + 1.0f; i++) {
        float *kp = (float *)((char *)fade->kp + fade->kpStride * (i < fade->nKpMax ? i : fade->nKpMax));
        kp[0] = (float)i + curData->frameStart;
        kp = (float *)((char *)fade->kp + fade->kpStride * (i < fade->nKpMax ? i : fade->nKpMax));
        kp[1] = 1.0f;
    }
}

// v12 0x00544B70  (feb 0x00541ED0)
void cinSYSTEM::ResetData()
{
    state &= ~3u;
    ctrlInst = 0;
    curEnt = 0;
    sheet = 0;
    for (int i = 0; i < 20; i++)
        actor[i].Term();
    nActors = 0;
    for (int i = 0; i < 50; i++) {
        frame[i].time = -1.0f;
        frame[i].cameraCut = 0;
        for (int k = 0; k < 10; k++) {
            operator delete(frame[i].act[k]);
            frame[i].act[k] = 0;
        }
        frame[i].nAct = 0;
    }
    nFrames = 0;
    lastFrame.time = -1.0f;
    lastFrame.cameraCut = 0;
    for (int k = 0; k < 10; k++) {
        operator delete(lastFrame.act[k]);
        lastFrame.act[k] = 0;
    }
    lastFrame.nAct = 0;
    curFrame = -1;
    if (fade)
        fade->__vecDelDtor(1);
    fade = 0;
}

// v12 0x00544C70  (1.2 only, name ours)
int cinSYSTEM::IsSuspendAction()
{
    return curEnt ? curEnt->isSuspendAction : 0;
}

// v12 0x00544C90  (1.2 only, name ours)
// The end of the playing cinema as ProcessFRAME has it, without forgetting its data.
void cinSYSTEM::Stop()
{
    for (int k = 0; k < lastFrame.nAct; k++)
        lastFrame.act[k]->UpdateFrame((int)lastFrame.time);
    state &= ~1u;
    gsAppState &= ~0x200;
    for (int i = 0; i < nActors; i++)
        actor[i].Term();
    gsCameraPtr->SetAngleHor(75.0f);                    // (the shipped value; this exe's widescreen patch changes it)
    gsSysSound->ReleaseMusic();
    msgSystem.SendAll(GS_MSG_CINEMA_END, 0, 0);
    if (curEnt->isSuspendAction)
        gsMsgChangeMode(5);
}

// v12 0x00544D80  (feb 0x00541FD0)
// The actor of that instance name, else a free one taken: the instance found (its entity stops animating itself,
// flags2 0x10) and told it acts (the player for its body "player_3d").
cinACTOR *cinSYSTEM::FindActor(char *name)
{
    for (int i = 0; i < 20; i++)
        if (actor[i].inst && !strcmp(actor[i].inst->name ? actor[i].inst->name : "", name))
            return &actor[i];
    int i = 0;
    while (actor[i].inst)
        if (++i >= 20)
            return 0;
    cinACTOR *a = &actor[i];
    a->inst = gsScenePtr->FindInstName(name);
    if (!a->inst)
        return 0;
    a->inst->ent->flags2 |= 0x10;
    if (!strcmp(a->inst->name ? a->inst->name : "", "player_3d"))
        msgSystem.SendMsg(plgsPlayer, GS_MSG_CINEMA_ACTOR_ON, 0, 0);
    else
        msgSystem.SendMsg(a->inst->ent, GS_MSG_CINEMA_ACTOR_ON, 0, 0);
    return a;
}

// ---------------------------------------------------------------------------------------------- cinACTOR

// v12 0x00544EC0  (feb 0x00542110)
// The sequence to play (empty: the first); with a transition table the actor's own system plays it (the body's
// sequences are one lower, "player_3d"), else it is set at the cinema's start time and timed by hand.
int cinACTOR::Animate(char *name)
{
    if (!*name) {
        if (inst->tpl->nSeq > 0)
            seq = 0;
    } else {
        seq = inst->tpl->FindSeqName(name);
        if (seq < 0) {
            apMsg((char *)"Cinematic: wrong animSeqName %s", name);
            return 0;
        }
    }
    if (inst->tpl->aniSystem) {
        if (!strcmp(inst->name ? inst->name : "", "player_3d"))
            seq--;
        noAnimSys = 0;
        return 1;
    }
    noAnimSys = 1;
    animSEQ *cs = gsSysCinema->ctrlInst->tpl->seq;
    inst->SetAnimSeq(seq, cs->FrameToTime(cs->frameStart));
    return 1;
}

// v12 0x00544FB0  (feb 0x00542200)
// The actor's voice (a cone channel at the instance or at one of its objects, made the first time) plays d.
int cinACTOR::PlaySound(char *obj, gsSOUND_DESCR *d)
{
    if (!chn) {
        sncCHANNEL_CONE *c = new sncCHANNEL_CONE;
        if (c) {
            c->minDist = 0.0f;
            c->maxDist = 0.0f;
        }
        chn = c;
        if (!chn)
            return 0;
        objOBJ *o = 0;
        if (*obj && !(o = objFindName(inst->root, obj)))
            apMsg((char *)"Cinematic: cannot find sound Object %s  %s", inst->name ? inst->name : (char *)"", obj);
        gsSysSound->AddChannel(chn, inst->ent, o);
    }
    chn->Play((sncSOUND_DESCR *)d, 0);
    return 1;
}

// v12 0x00545120  (feb 0x00542370)
// The actor released: its entity animates itself again, its objects shown, told (the player for "player_3d"); its
// voice dropped.
void cinACTOR::Term()
{
    if (inst) {
        inst->ent->flags2 &= ~0x10u;
        if (inst->root)
            inst->root->SetStateProcNo(0x10, 0);
        if (!strcmp(inst->name ? inst->name : "", "player_3d"))
            msgSystem.SendMsg(plgsPlayer, GS_MSG_CINEMA_ACTOR_OFF, 0, 0);
        else
            msgSystem.SendMsg(inst->ent, GS_MSG_CINEMA_ACTOR_OFF, 0, 0);
    }
    if (chn) {
        gsSysSound->DelChannel(chn);
        if (chn)
            chn->__vecDelDtor(1);
        chn = 0;
    }
    inst = 0;
    conInst = 0;
    conObj = 0;
    noAnimSys = 0;
    seq = -1;
}

// v12 0x005451D0  (feb 0x00542420)
// Constrained: onto the instance (or its object), unscaled; hidden while that object is (procState 0x3F). Its
// sequence: while the cinema runs, by its system or its own time; once it has ended, a hand-timed one to its end.
void cinACTOR::UpdateFrame()
{
    if (conInst) {
        conInst->Validate(4);
        m3dMATR m;
        memcpy(&m, conObj ? (void *)conObj->ltm : (void *)conInst->matr, sizeof(m));
        float sx, sy, sz;
        m.GetScale(&sx, &sy, &sz);
        m.Scale(1.0f / sx, 1.0f / sy, 1.0f / sz, 1);
        m3dV o;
        m.GetOrigin(&o);
        inst->Transform(&m, 0);
        if (objOBJ *r = inst->root) {
            if (!conObj || !(conObj->procState & 0x3F))
                r->SetStateProcNo(0x10, 0);
            else
                r->SetStateProcYes(0x10, 0);
        }
    }
    if (seq == -1)
        return;
    if (!(gsSysCinema->state & 2)) {
        inst->flags &= ~2u;
        if (!noAnimSys)
            ((animSYSTEM_INST *)inst->link)->AdvanceAnimation(inst, seq, gsElapsedTime, 0);
        else
            inst->AdvanceTime(gsElapsedTime, 1, 0);
        return;
    }
    if (noAnimSys)
        inst->SetAnimSeq(seq, inst->tpl->seq[inst->seq].lengthMs * 0.001f);
}

// ---------------------------------------------------------------------------------------------- cinFRAME

// v12 0x00545350  (feb 0x005425A0)
// The frame's actions: for each kind the key without a number, then "_1_", "_2_"... until a round adds none.
int cinFRAME::Init(float t, psSHEET *s, char *sect)
{
    time = t;
    cameraCut = 0;
    nAct = 0;
    for (int k = 0; k < 10; k++)
        act[k] = 0;
    char key[0x40];
    for (int n = 0;; n++) {
        int had = nAct;
#define CIN_KEY(base) do { sprintf(key, base "_"); if (n > 0) sprintf(key, base "_%d_", n); } while (0)
        CIN_KEY("create");
        if (s->FindVarStr(sect, key)) {
            cinACN_CREATE_ENT *a = new cinACN_CREATE_ENT;
            if (a) { a->actor[0] = 0; a->cls[0] = 0; a->tpl[0] = 0; }
            act[nAct] = a;
            if (act[nAct]->Init(s, sect, key))
                nAct++;
        }
        CIN_KEY("destroy");
        if (s->FindVarStr(sect, key)) {
            cinACN_DESTROY_ENT *a = new cinACN_DESTROY_ENT;
            if (a) a->actor[0] = 0;
            act[nAct] = a;
            if (act[nAct]->Init(s, sect, key))
                nAct++;
        }
        CIN_KEY("animate");
        if (s->FindVarStr(sect, key)) {
            cinACN_ANIMATE *a = new cinACN_ANIMATE;
            if (a) { a->actor[0] = 0; a->seq[0] = 0; }
            act[nAct] = a;
            if (act[nAct]->Init(s, sect, key))
                nAct++;
        }
        CIN_KEY("command");
        if (s->FindVarStr(sect, key)) {
            cinACN_COMMAND *a = new cinACN_COMMAND;
            if (a) entIACTION_Init(&a->data);
            act[nAct] = a;
            if (act[nAct]->Init(s, sect, key))
                nAct++;
        }
        CIN_KEY("constraint");
        if (s->FindVarStr(sect, key)) {
            cinACN_CONTRAINT *a = new cinACN_CONTRAINT;
            if (a) { a->actor[0] = 0; a->actorTo[0] = 0; a->objTo[0] = 0; }
            act[nAct] = a;
            if (act[nAct]->Init(s, sect, key))
                nAct++;
        }
        CIN_KEY("sound");
        if (s->FindVarStr(sect, key)) {
            cinACN_SOUND *a = new cinACN_SOUND;
            if (a) {
                gsSOUND_DESCR *d = &a->descr;                // (sncSOUND_DESCR ctor v12 0x0045CCA0 and the rest inline)
                d->vtbl = 0;
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
                a->actor[0] = 0;
                a->obj[0] = 0;
            }
            act[nAct] = a;
            if (act[nAct]->Init(s, sect, key))
                nAct++;
        }
        CIN_KEY("music");
        if (s->FindVarStr(sect, key)) {
            cinACN_MUSIC *a = new cinACN_MUSIC;
            if (a) a->name[0] = 0;
            act[nAct] = a;
            if (act[nAct]->Init(s, sect, key))
                nAct++;
        }
        CIN_KEY("camera_cut");
        if (s->FindVarStr(sect, key)) {
            act[nAct] = new cinACN_CAMERA_CUT;
            if (act[nAct]->Init(s, sect, key))
                nAct++;
            cameraCut = 1;
        }
        CIN_KEY("fade");
        if (s->FindVarStr(sect, key)) {
            cinACN_FADE *a = new cinACN_FADE;
            if (a) { a->frameLen = 0; a->isOut = 0; }
            act[nAct] = a;
            if (act[nAct]->Init(s, sect, key))
                nAct++;
        }
#undef CIN_KEY
        if (nAct == had)
            break;
    }
    return 1;
}

// ---------------------------------------------------------------------------------------------- actions

int  cinACTION::Init(psSHEET *, char *, char *) { return 1; }  // v12 0x004EADA0
void cinACTION::UpdateFrame(int) {}                            // v12 0x00403950

static int cinCannotRead(char *sect, char *key)
{
    apMsg((char *)"Cinematic: section %s: cannot read action %s params", sect, key);
    return 0;
}

// v12 0x00545860  (feb 0x00542AB0)
int cinACN_CREATE_ENT::Init(psSHEET *s, char *sect, char *k)
{
    char key[0x40];
    sprintf(key, "%sActor", k);
    if (s->GetStr(sect, key, actor, 0x1000)) {
        sprintf(key, "%sClassName", k);
        if (s->GetStr(sect, key, cls, 0x1000)) {
            sprintf(key, "%sTplName", k);
            if (s->GetStr(sect, key, tpl, 0x1000))
                return 1;
        }
    }
    return cinCannotRead(sect, key);
}

// v12 0x00545940  (feb 0x00542B90)
// A new entity of the class from the template, named as the actor.
void cinACN_CREATE_ENT::UpdateFrame(int)
{
    animCREATE_DATA cd;
    strcpy(cd.name, actor);
    entCreate(gsScenePtr, cls, tpl, &cd, 0, 0);
}

// v12 0x00545990  (feb 0x00542BE0)
int cinACN_DESTROY_ENT::Init(psSHEET *s, char *sect, char *k)
{
    char key[0x40];
    sprintf(key, "%sActor", k);
    if (!s->GetStr(sect, key, actor, 0x1000))
        return cinCannotRead(sect, key);
    return 1;
}

// v12 0x00545A00  (feb 0x00542C50)
void cinACN_DESTROY_ENT::UpdateFrame(int)
{
    animINST *a = gsScenePtr->FindInstName(actor);
    if (a && a->ent)
        msgSystem.PostMsg(a->ent, 2, gsSysCinema);
}

// v12 0x00545A40  (feb 0x00542C90)
int cinACN_ANIMATE::Init(psSHEET *s, char *sect, char *k)
{
    char key[0x40];
    sprintf(key, "%sActor", k);
    if (!s->GetStr(sect, key, actor, 0x1000))
        return cinCannotRead(sect, key);
    seq[0] = 0;                                         // (apNAME::SetName "")
    sprintf(key, "%sSeqName", k);
    s->GetStr(sect, key, seq, 0x1000);
    return 1;
}

// v12 0x00545AF0  (feb 0x00542D40)
void cinACN_ANIMATE::UpdateFrame(int)
{
    if (cinACTOR *a = gsSysCinema->FindActor(actor))
        a->Animate(seq);
}

// v12 0x00545B20  (feb 0x00542D70)
int cinACN_CONTRAINT::Init(psSHEET *s, char *sect, char *k)
{
    char key[0x40];
    sprintf(key, "%sActor", k);
    if (s->GetStr(sect, key, actor, 0x1000)) {
        sprintf(key, "%sActorTo", k);
        if (s->GetStr(sect, key, actorTo, 0x1000)) {
            objTo[0] = 0;
            sprintf(key, "%sObjTo", k);
            s->GetStr(sect, key, objTo, 0x1000);
            return 1;
        }
    }
    return cinCannotRead(sect, key);
}

// v12 0x00545C00  (feb 0x00542E50)
void cinACN_CONTRAINT::UpdateFrame(int)
{
    cinACTOR *a = gsSysCinema->FindActor(actor);
    if (!a)
        return;
    a->conInst = gsScenePtr->FindInstName(actorTo);
    if (!a->conInst) {
        apMsg((char *)"Cinematic: cannot find constraint Actor %s", actorTo);
        return;
    }
    if (*objTo && !(a->conObj = objFindName(a->conInst->root, objTo)))
        apMsg((char *)"Cinematic: cannot find constraint Object %s  %s", actorTo, objTo);
}

// v12 0x00545C90  (feb 0x00542EE0)
int cinACN_COMMAND::Init(psSHEET *s, char *sect, char *k)
{
    return entIACTIVE_OBJ::ParseIActionData(s, sect, k, &data, (char *)"cinematic_command");
}

// v12 0x00545CB0  (feb 0x00542F00)
void cinACN_COMMAND::UpdateFrame(int)
{
    entIACTIVE_OBJ::ExecuteIAction(&data, 0);
}

// v12 0x00545CC0  (feb 0x00542F10)
int cinACN_FADE::Init(psSHEET *s, char *sect, char *k)
{
    char key[0x40];
    sprintf(key, "%sFrameLen", k);
    if (s->GetInt(sect, key, &frameLen)) {
        sprintf(key, "%sIsOut", k);
        if (s->GetBool(sect, key, &isOut))
            return 1;
    }
    return cinCannotRead(sect, key);
}

// v12 0x00545D60  (feb 0x00542FB0)
// The fade spline (the overlay's darkness) from this frame to the end: at once (no length) dark or clear, else a
// ramp over frameLen frames -- out from clear to dark, in from dark to clear -- held after it.
void cinACN_FADE::UpdateFrame(int fr)
{
    cinSYSTEM *c = gsSysCinema;
    float rel = (float)fr - c->curData->frameStart;
    int i = (int)rel;
    while ((float)i < c->curData->frameEnd - c->curData->frameStart + 1.0f) {
        float v;
        if (!frameLen)
            v = isOut ? 1.0f : 0.0f;
        else {
            float x = (float)i - rel;
            float a = 0.0f, b = (float)frameLen, va, vb;        // the ramp: (a, va) to (b, vb)
            if (isOut) {
                va = 0.0f; vb = 1.0f;
                if (b < 0.0f) { a = b; b = 0.0f; va = 1.0f; vb = 0.0f; }
            } else {
                va = 1.0f; vb = 0.0f;
                if (b < 0.0f) { a = b; b = 0.0f; va = 0.0f; vb = 1.0f; }
            }
            if (x < a)
                v = va;
            else if (!(x <= b))
                v = vb;
            else {
                float lo = a, hi = b, vlo = va, vhi = vb;
                if (b < a) { lo = b; hi = a; vlo = vb; vhi = va; }
                v = (x - lo) * (vhi - vlo) / (hi - lo) + vlo;
            }
        }
        m3dSPL_LINEAR1D *s = c->fade;
        float *kp = (float *)((char *)s->kp + s->kpStride * (i < s->nKpMax ? i : s->nKpMax));
        kp[1] = v;
        i++;
        c = gsSysCinema;
    }
}

// v12 0x00546010  (feb 0x00543260)
int cinACN_SOUND::Init(psSHEET *s, char *sect, char *k)
{
    char key[0x40];
    sprintf(key, "%sActor", k);
    s->GetStr(sect, key, actor, 0x40);
    sprintf(key, "%sObj", k);
    s->GetStr(sect, key, obj, 0x40);
    descr.dataFlags |= 6;
    descr.ReadParams(s, sect, k);
    return 1;
}

// v12 0x005460A0  (feb 0x005432F0)
// By an actor's voice, else at the camera.
void cinACN_SOUND::UpdateFrame(int)
{
    cinACTOR *a = gsSysCinema->FindActor(actor);
    if (!a) {
        m3dV p;
        gsCameraPtr->GetOrigin(&p);
        gsSysSound->Play(&descr, &p);
        return;
    }
    a->PlaySound(obj, &descr);
}

// v12 0x00546100  (feb 0x00543350)
int cinACN_MUSIC::Init(psSHEET *s, char *sect, char *k)
{
    char key[0x40];
    sprintf(key, "%sName", k);
    if (!s->GetStr(sect, key, name, 0x40))
        return cinCannotRead(sect, key);
    return 1;
}
