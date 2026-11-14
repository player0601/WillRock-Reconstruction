// gs_level.cpp -- the level system: loading and destroying levels (Will Rock 1.2, v12.exe; feb obj gs:gs_level.obj)
//
// Rebuilt so far: DestroyLevel, PreloadUnloadData (+ _gsLvlPreloadListFind), the destroy notifies (object,
// instance, template). Not rebuilt: the rest of gsLVL_SYSTEM (LoadLevel, LoadInstAll, the load notifies, the preload
// lists -- level.h lists them).

#include "../Sources/game.h"
#include "../wr_vcall.h"

// v12 0x005357A0  (feb 0x0051FE20)  slot +0x30
// An object going away: its shaders (those it owns deleted), its mirror, the renderer's data for it; then each child.
void gsLVL_SYSTEM::ObjDestroyNotify(objOBJ *o)
{
    if (!o)
        return;
    pteManager->DeleteObjNotify(o);
    gsSysMirror->TermObj(o);
    wrVCall<void>(rendDrv, 0xAC, o);
    objOBJ *first = o->child;
    int started = 0;
    for (objOBJ *c = first; c && !(started && c == first); c = c->next, started = 1)
        wrEVCall<void>(this, 0x30, c);
}

// v12 0x00535800  (feb 0x0051FE80)  slot +0x34
// An instance going away: its shaders (those it owns deleted), its projected shadow, the renderer's data, the
// emitters following it let go; then its object tree.
void gsLVL_SYSTEM::InstDestroyNotify(animINST *a)
{
    pteManager->DeleteInstNotify(a);
    gsSysProjShd->TermInst(a);
    wrVCall<void>(rendDrv, 0xA4, a);
    partDetachInst(a);
    if (a)
        wrEVCall<void>(this, 0x30, a->root);
}

// v12 0x00535850  (feb 0x0051FED0)  slot +0x3C
void gsLVL_SYSTEM::TplDestroyNotify(animTPL *t)
{
    wrEVCall<void>(this, 0x30, t->root);
}

// v12 0x00535870  (feb 0x0051FEF0)  slot +0x38
// The templates going away: all of them, or those read with the level (flags 0x800).
void gsLVL_SYSTEM::TplDestroyNotify(int all)
{
    for (animTPL *t = gsScenePtr->FindNextTpl(0); t; t = gsScenePtr->FindNextTpl(t))
        if (all || (t->flags & 0x800))
            wrEVCall<void>(this, 0x3C, t);
}

// v12 0x00536B90  (feb 0x005213A0)  slot +0x20
// The level ends: no longer running (gsAppState 0x40000); the preload data brought up to date; with a load
// parameter the entities to keep across a reload marked (flags2 8: those flagged 4, or -- when the parameter keeps
// entities through a filter -- those that answer 0x3FC (or are flagged 1) and whose instance the filter takes);
// TERM_LEVEL to all; every other entity (or one already destroyed) gets TERM; the objects and templates of the level
// let go (without a parameter: all of them), the particles and shaders ended, the scene's level data freed, the
// renderer's too; the destroyed addresses purged and the pending messages dropped.
void gsLVL_SYSTEM::DestroyLevel(gsLOAD_SCENE_PARAM *p)
{
    apMsg((char *)"Destroy current level");
    gsAppState &= ~0x40000;
    if (!gsScenePtr)
        return;
    UpdatePreloadData(0);
    for (entENTITY *e = entFindNext(0); e; e = entFindNext(e))
        e->flags2 &= ~8u;
    for (entENTITY *e = entFindNext(0); e; e = entFindNext(e)) {
        if (!p)
            continue;
        if (!(e->flags2 & 4)) {
            if (!p->keepEntities || !p->keepFilter || !e->inst)
                continue;
            if (!msgSystem.SendMsg(e, 0x3FC, 0, 0) && !(e->flags2 & 1))
                continue;
            if (!wrVCall<BOOL32>(p->keepFilter, 0x2C, e->inst))  // scnDOMAIN::IsIsectInst
                continue;
        }
        e->flags2 |= 8;
    }
    msgSystem.SendAll(GS_MSG_TERM_LEVEL, 0, 0);
    for (entENTITY *e = entFindNext(0), *n; e; e = n) {
        n = entFindNext(e);
        if (!(e->flags2 & 8) || (e->msgADDR::flags & 4))
            msgSystem.SendMsg(e, MSG_MSG_TERM, 0, 0);
    }
    wrEVCall<void>(gsSysLevel, 0x30, gsScenePtr->root);  // ObjDestroyNotify
    partDestroyEmitters();
    pteManager->TermLevel();
    wrEVCall<void>(this, 0x38, p ? 0 : 1);                // TplDestroyNotify(all)
    gsScenePtr->DestroyLevel(0);
    wrVCall<void>(rendDrv, 0x9C, gsScenePtr);            // rendDRIVER::TermLevel
    msgSystem.PurgeDestroyAddrAll();
    msgSystem.DeleteAllMsg();
}

// v12 0x00543B10  (feb 0x0052E060)
// Whether a preload list (.dsl) names this model, texture or sound: its [Preload_Mod] Model_0.., [Preload_Tex]
// Texture_0.., [Preload_Snd] Sound_0.. entries, each compared by its name without the path and extension.
int __fastcall _gsLvlPreloadListFind(psSHEET *s, char *name)
{
    static const char *sect[3] = { "Preload_Mod", "Preload_Tex", "Preload_Snd" };
    static const char *key[3] = { "Model_%d", "Texture_%d", "Sound_%d" };
    char buf[0x104];
    for (int k = 0; k < 3; k++)
        for (int i = 0;; i++) {
            sprintf(apBuffer, key[k], i);
            if (!s->GetStr((char *)sect[k], apBuffer, buf, sizeof(buf)))
                break;
            if (!_strcmpi(name, fnmGetName(buf)))
                return 1;
        }
    return 0;
}

// v12 0x00543420  (feb 0x0052D990)
// After a level is loaded: every template that no instance of the scene uses (by name), that is not kept (flags
// 0x100) and that no preload list in use names, is let go (TplDestroyNotify, scnSCENE::DestroyTpl). Then the
// textures still used are marked (TXM_F_ADDED; PreloadInitLevel cleared it on all before the load): the level's
// geometry, every instance, every template, and those the preload lists name; the preloaded scenes' textures read
// again; and the texture manager purges the rest (scene textures deleted, the others unloaded).
void gsLVL_SYSTEM::PreloadUnloadData()
{
    for (animTPL *t = gsScenePtr->FindNextTpl(0); t; t = gsScenePtr->FindNextTpl(t)) {
        int unused = !(t->flags & 0x100);
        for (animINST *a = gsScenePtr->FindNextInst(0); a; a = gsScenePtr->FindNextInst(a))
            if (!_strcmpi(a->tplName ? a->tplName : "", t->tplName))
                unused = 0;
        if ((preloadFlags & LVLPRE_COMM) && preloadDefault && _gsLvlPreloadListFind(preloadDefault, t->tplName))
            unused = 0;
        if ((preloadFlags & LVLPRE_MP) && preloadMP && _gsLvlPreloadListFind(preloadMP, t->tplName))
            unused = 0;
        if (preloadFlags & 0x14)
            for (int i = 0; i < nPreload; i++)
                if (preload[i].data && _gsLvlPreloadListFind((psSHEET *)preload[i].data, t->tplName))
                    unused = 0;
        if (unused) {
            apLog((char *)"Template %s unloaded from memory", t->tplName);
            wrEVCall<void>(this, 0x3C, t);                  // TplDestroyNotify(animTPL *)
            gsScenePtr->DestroyTpl(t);
        }
    }
    gsScenePtr->root->MarkUsedTex();
    for (animINST *a = gsScenePtr->FindNextInst(0); a; a = gsScenePtr->FindNextInst(a))
        if (a->root)
            a->root->MarkUsedTex();
    for (animTPL *t = gsScenePtr->FindNextTpl(0); t; t = gsScenePtr->FindNextTpl(t))
        if (t->root)
            t->root->MarkUsedTex();
    for (txmTEXTURE *x = txmManager->GetNext(0); x; x = txmManager->GetNext(x)) {
        if ((preloadFlags & LVLPRE_COMM) && preloadDefault && _gsLvlPreloadListFind(preloadDefault, x->name))
            x->flags |= TXM_F_ADDED;
        if ((preloadFlags & LVLPRE_MP) && preloadMP && _gsLvlPreloadListFind(preloadMP, x->name))
            x->flags |= TXM_F_ADDED;
        if (preloadFlags & 0x14)
            for (int i = 0; i < nPreload; i++)
                if (preload[i].data && _gsLvlPreloadListFind((psSHEET *)preload[i].data, x->name))
                    x->flags |= TXM_F_ADDED;
    }
    if (preloadFlags & 0x14)
        for (int i = 0; i < nPreload; i++)
            if (preload[i].file) {
                sioSetSourceFile((fioFILE *)preload[i].file);
                sioReadSceneTex();
            }
    txmManager->PurgeAll();
}
