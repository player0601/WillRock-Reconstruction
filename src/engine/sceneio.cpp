// sceneio.cpp -- engine.lib scene / template file readers: the top-level chunk loops
// (Will Rock 1.2, v12.exe; feb obj engine:sceneio.obj)
//
// v12 0x0059E670..0x0059EA7F. sioReadSceneTex / sioReadTemplate "exact"; sioSetSourceFile / sioReadScene
// only differ by call targets (1.2: the collision's back pointer is cdtCOLL::data at +0x6C).
// A .lg level file is a list of top-level chunks {u16 id, u32 end}; every reader is handed the file
// positioned on its chunk header. Unknown chunks are skipped by their end position. The readers' results
// are not checked here: a failed chunk leaves the position wherever the reader stopped (original).
//   0x0F0 object tree         -> scene->root           0x154/0x155 texture list -> txmManager
//   0x1B8 instance list       -> scene instances      0x1EA template list      -> scene templates
//   0x21C old collision grid  -> scene->coll          0x21F collision grid     -> scene->coll
//   0x21D named selections    -> scene->nSel/sel      0x21E global script      -> scene->sheet
//   0x280 lights              -> scene->lights        0x3AC camera             -> scene->camera

#include "engine.h"

fioFILE *sioFilePtr;           // v12 0x009AF8A0 (feb 0x0096442C) the file every sio* reader reads from
int     *sioTexListNmb;        // v12 0x009AF898 (feb 0x00964424) file texture index -> txmTEXTURE::listNmb

// v12 0x0059E670  (feb 0x00582340)
void __fastcall sioSetSourceFile(fioFILE *f)
{
    sioFilePtr = f;
    f->Seek(0, FIO_SEEK_SET);
}

// v12 0x0059E680  (feb 0x00582350)
BOOL32 __fastcall sioReadScene(scnSCENE *s)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    while (!sioFilePtr->IsEOF()) {
        if (!sioFilePtr->ReadChunk(&c) || sioFilePtr->IsEOF())
            break;
        sioFilePtr->RevertChunk(0);
        switch (c.id) {
        case 0x0F0:
            s->root = _sioReadObj();
            break;
        case 0x154:
        case 0x155:
            _sioReadTex();
            break;
        case 0x1B8:
            _sioReadInstList(s);
            break;
        case 0x1EA:
            _sioReadTplList(s);
            break;
        case 0x21C:
            s->coll = _sioReadColl_OLD();
            if (s->coll)
                s->coll->data = s;
            break;
        case 0x21D:
            _sioReadNamedSelList(&s->nSel, &s->sel);
            break;
        case 0x21E:
            sioFilePtr->ReadChunk(&c);                  // into the chunk: the script is its first sub-chunk
            if (!_sioReadPSheet(&s->sheet))
                apLogErr((char *)"Cannot parse global scene script");
            break;
        case 0x21F:
            s->coll = _sioReadColl();
            if (s->coll)
                s->coll->data = s;
            break;
        case 0x280:
            _sioReadLightList(s);
            break;
        case 0x3AC:
            s->camera = _sioReadCamera();
            break;
        default:
            sioFilePtr->SkipChunk(&c);
            break;
        }
    }
    apFree(sioTexListNmb);
    sioTexListNmb = 0;
    return 1;
}

// v12 0x0059E840  (feb 0x00582510)
// Only the texture lists of a scene file (the rest is skipped).
BOOL32 __fastcall sioReadSceneTex()
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    while (!sioFilePtr->IsEOF() && sioFilePtr->ReadChunk(&c)) {
        if (sioFilePtr->IsEOF())
            break;
        if (c.id < 0x154 || c.id > 0x155)
            sioFilePtr->SkipChunk(&c);
        else {
            sioFilePtr->RevertChunk(0);
            _sioReadTex();
        }
    }
    apFree(sioTexListNmb);
    sioTexListNmb = 0;
    return 1;
}

// v12 0x0059E900  (feb 0x005825D0)
// A template (.tpl file, or an entry of a level's template list):
// 0x2E4 { 0x2E5 header 'APTL' name class } [textures] object tree, skins, object animations, sequences,
// per-sequence bounding volumes, script, named selections, end chunk. Then the animation clean-up.
BOOL32 __fastcall sioReadTemplate(animTPL *t, int withTex)
{
    if (sioFilePtr->ReadChunk(0) != 0x2E4) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    if (sioFilePtr->ReadChunk(0) != 0x2E5)
        return 0;
    sioFilePtr->ReadData(&t->magic, 4, 4);
    sioFilePtr->Read(t->tplName, 0x40);
    sioFilePtr->Read(t->className, 0x40);
    if (t->magic != 0x4154504C)                         // 'APTL'
        return 0;
    if (withTex && !_sioReadTex())
        return 0;
    t->root = _sioReadObj();
    if (!t->root)
        return 0;
    if (!_sioReadObjSkinList(t))
        return 0;
    if (!_sioReadObjAnimList(t))
        return 0;
    if (!_sioReadAnimSeq(t))
        return 0;
    if (!_sioReadBVolList(t))
        return 0;
    if (!_sioReadPSheet(&t->props))
        apLogErr((char *)"Cannot parse script for template %s", t->tplName);
    if (!_sioReadNamedSelList(&t->nSel, &t->sel))
        return 0;
    if (!_sioReadLastChunk())
        return 0;
    if (withTex) {
        apFree(sioTexListNmb);
        sioTexListNmb = 0;
    }
    animCleanConstSpl(t);
    animCompressAnim(t);
    animCleanConstSpl(t);
    return 1;
}
