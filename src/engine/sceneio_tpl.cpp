// sceneio_tpl.cpp -- engine.lib template (.tpl) readers: animation sequences, object animation tracks,
// per-sequence bounding volumes, skin modifiers
// (Will Rock 1.2, v12.exe; feb obj engine:sceneio_tpl.obj)
//
// v12 0x005A0BD0..0x005A16DF, rebuilt from the 1.2 code. Called by sioReadTemplate in this order:
// skins, object animations, sequences, bounding volumes. Each list is optional: a missing chunk is put back
// and the reader returns 1; any read failure returns 0 and fails the template.
//   0x2E6 sequences: u32 n, n x 0x2E7 { name 0x40, start, end, length (ms), +0x54, +0x58 action frame 0,
//         optional 0x2F3 { +0x48 } then 0x2F4 { frame range }, optional 0x2F7 { the 10 action frames } }
//   0x2E8 object animations: u32 n, n x 0x2E9 { u32, sub-chunks to the end chunk: 0x2EA/0x2EB/0x2EC/0x2ED
//         position / rotation / scale / 4th spline, 0x2FA/0x2FB/0x2FC constant position / quaternion / scale }
//         -- an entry without constant values takes them from the object whose hierarchy id is its index
//   0x2F9 bounding volumes, one per sequence: 0x2F8 { volume, enclosing sphere, updater data by volume type }
//   0x2EE skins: u32 n, n x { 0x2EF counts + object ids, 0x2F0 vertex refs, 0x2F1 bones x 0x2F2 { name,
//         offsets, optional 0x2F5, optional 0x2F6 spline } }

#include "engine.h"

// v12 0x005A0BD0  (feb 0x00584710)
BOOL32 __fastcall _sioReadAnimSeq(animTPL *t)
{
    if (sioFilePtr->ReadChunk(0) != 0x2E6) {
        sioFilePtr->RevertChunk(0);
        return 1;
    }
    sioFilePtr->ReadData(&t->nSeq, 4, 4);
    int n = t->nSeq;
    int *mem = (int *)operator new(n * sizeof(animSEQ) + 4);   // new animSEQ[n] (inline ctor, count cookie)
    animSEQ *seq = 0;
    if (mem) {
        *mem = n;
        seq = (animSEQ *)(mem + 1);
        for (int i = 0; i < n; i++) {
            memset(&seq[i], 0, sizeof(animSEQ));
            seq[i].dir = 2;
        }
    }
    t->seq = seq;
    if (!seq)
        return 0;
    for (int i = 0; i < t->nSeq; i++) {
        if (sioFilePtr->ReadChunk(0) != 0x2E7)
            return 0;
        animSEQ *s = &t->seq[i];
        sioFilePtr->Read(s->name, 0x40);
        sioFilePtr->ReadData(&s->frameStart, 4, 4);
        sioFilePtr->ReadData(&s->frameEnd, 4, 4);
        sioFilePtr->ReadData(&s->lengthMs, 4, 4);
        sioFilePtr->ReadData(&s->unk54, 4, 4);
        sioFilePtr->ReadData(&s->actionFrame[0], 4, 4);
        if (sioFilePtr->ReadChunk(0) == 0x2F3) {
            sioFilePtr->ReadData(&s->unk48, 4, 4);
            if (sioFilePtr->ReadChunk(0) != 0x2F4)
                return 0;
            sioFilePtr->ReadData(&s->frameRange, 4, 4);
            if (sioFilePtr->ReadChunk(0) == 0x2F7) {
                sioFilePtr->ReadData(s->actionFrame, 0x28, 4);
                continue;
            }
        }
        sioFilePtr->RevertChunk(0);
    }
    for (int i = 0; i < t->nSeq; i++)
        t->seq[i].PrepareDir();
    return _sioReadLastChunk() != 0;
}

// v12 0x005A0E20  (feb 0x00584960)
BOOL32 __fastcall _sioReadObjAnimList(animTPL *t)
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    if (sioFilePtr->ReadChunk(0) != 0x2E8) {
        sioFilePtr->RevertChunk(0);
        return 1;
    }
    sioFilePtr->ReadData(&t->nObjAnim, 4, 4);
    int n = t->nObjAnim;
    int *mem = (int *)operator new(n * sizeof(animOBJ_ANIM) + 4); // new animOBJ_ANIM[n] (inline ctor, cookie)
    animOBJ_ANIM *list = 0;
    if (mem) {
        *mem = n;
        list = (animOBJ_ANIM *)(mem + 1);
        for (int i = 0; i < n; i++) {
            list[i].flags = 0;
            list[i].unk04 = 0;
            list[i].spl14 = 0;
            list[i].spl28 = 0;
            list[i].spl38 = 0;
            list[i].spl3C = 0;
        }
    }
    t->objAnim = list;
    if (!list)
        return 0;
    for (int i = 0; i < t->nObjAnim; i++) {
        if (sioFilePtr->ReadChunk(0) != 0x2E9)
            return 0;
        animOBJ_ANIM *a = &t->objAnim[i];
        sioFilePtr->ReadData(&a->unk04, 4, 4);
        if (sioFilePtr->ReadChunk(&c)) {
            while (c.id != 1) {
                switch (c.id) {
                case 0x2EA:
                    a->spl14 = sioFilePtr->ReadSpline();
                    if (!a->spl14)
                        return 0;
                    a->flags |= 2;
                    break;
                case 0x2EB:
                    a->spl28 = sioFilePtr->ReadSpline();
                    if (!a->spl28)
                        return 0;
                    a->flags |= 4;
                    break;
                case 0x2EC:
                    a->spl38 = sioFilePtr->ReadSpline();
                    if (!a->spl38)
                        return 0;
                    a->flags |= 8;
                    break;
                case 0x2ED:
                    a->spl3C = sioFilePtr->ReadSpline();
                    if (!a->spl3C)
                        return 0;
                    break;
                case 0x2FA:
                    sioFilePtr->ReadData(a->pos, 0xC, 4);
                    a->flags |= 1;
                    a->flags |= 2;
                    break;
                case 0x2FB:
                    sioFilePtr->ReadData(a->rot, 0x10, 4);
                    a->flags |= 1;
                    a->flags |= 4;
                    break;
                case 0x2FC:
                    sioFilePtr->ReadData(a->scale, 0xC, 4);
                    a->flags |= 1;
                    a->flags |= 8;
                    break;
                default:
                    sioFilePtr->SkipChunk(&c);
                    break;
                }
                if (!sioFilePtr->ReadChunk(&c))
                    break;
            }
        }
        if (!(a->flags & 1)) {
            objOBJ *o = objFindAnimNmb(t->root, i);
            if (o) {
                m3dMATR *local = (m3dMATR *)o->local;
                local->GetTranslate(&a->pos[0], &a->pos[1], &a->pos[2]);
                ((m3dQUAT *)a->rot)->SetMatr(local);
                local->GetScale(&a->scale[0], &a->scale[1], &a->scale[2]);
                a->flags |= 1;
            } else
                apLog("Template %s: invalid Anim", t->className);
        }
    }
    return 1;
}

// v12 0x005A1200  (feb 0x00584CE0)
BOOL32 __fastcall _sioReadBVolList(animTPL *t)
{
    int n;
    if (sioFilePtr->ReadChunk(0) != 0x2F9) {
        sioFilePtr->RevertChunk(0);
        return 1;
    }
    sioFilePtr->ReadData(&n, 4, 4);
    if (n != t->nSeq)
        return 0;
    int *mem = (int *)operator new(n * sizeof(abvBVOL) + 4);   // new abvBVOL[n] (count cookie)
    abvBVOL *list = 0;
    if (mem) {
        *mem = n;
        list = (abvBVOL *)(mem + 1);
        for (int i = 0; i < n; i++)
            new (&list[i]) abvBVOL;
    }
    t->bvSeq = list;
    if (!list)
        return 0;
    for (int i = 0; i < t->nSeq; i++) {
        abvBVOL *bv = &t->bvSeq[i];
        if (sioFilePtr->ReadChunk(0) != 0x2F8)
            return 0;
        m3dVOL *vol = sioFilePtr->ReadVol();
        if (!vol)
            return 0;
        if (!sioFilePtr->ReadVol((m3dVOL *)&bv->sphere))
            return 0;
        bv->upd = abvVOL_UPD::Make(vol);
        if (!bv->upd)
            return 0;
        u8 *u = (u8 *)bv->upd;
        switch (bv->upd->vol->type) {
        case M3D_VOL_BOX:                       // abvBOX_UPD: two offsets
            sioFilePtr->ReadData(u + 0x08, 0xC, 4);
            sioFilePtr->ReadData(u + 0x14, 0xC, 4);
            break;
        case M3D_VOL_PRISM:                     // abvPRISM_UPD
            sioFilePtr->ReadData(u + 0x08, 8, 4);
            sioFilePtr->ReadData(u + 0x10, 8, 4);
            sioFilePtr->ReadData(u + 0x18, 8, 4);
            sioFilePtr->ReadData(u + 0x20, 8, 4);
            sioFilePtr->ReadData(u + 0x28, 4, 4);
            sioFilePtr->ReadData(u + 0x2C, 4, 4);
            break;
        case M3D_VOL_SPHERE:                    // abvSPHERE_UPD: centre offset
            sioFilePtr->ReadData(u + 0x08, 0xC, 4);
            break;
        case M3D_VOL_OBB:                       // abvOBB_UPD: the local box
            if (!sioFilePtr->ReadVol((m3dVOL *)(u + 0x08)))
                return 0;
            break;
        }
    }
    return 1;
}

// v12 0x005A1440  (feb 0x00584F20)
BOOL32 __fastcall _sioReadObjSkinList(animTPL *t)
{
    if (sioFilePtr->ReadChunk(0) != 0x2EE) {
        sioFilePtr->RevertChunk(0);
        return 1;
    }
    sioFilePtr->ReadData(&t->nSkin, 4, 4);
    t->skinList = (objMOD_SKIN **)apCalloc(t->nSkin, 4);
    if (!t->skinList)
        return 0;
    for (int i = 0; i < t->nSkin; i++) {
        void *mem = operator new(sizeof(objMOD_SKIN));
        if (!mem)
            return 0;
        objMOD_SKIN *sk = new (mem) objMOD_SKIN;
        t->skinList[i] = sk;
        if (sioFilePtr->ReadChunk(0) != 0x2EF)
            return 0;
        sioFilePtr->ReadData(&sk->nGroup, 4, 4);
        sioFilePtr->ReadData(&sk->nRef, 4, 4);
        sioFilePtr->ReadData(&sk->nBone, 4, 4);
        if (!sk->Init(sk->nGroup, sk->nRef, sk->nBone, 0))
            return 0;
        sioFilePtr->ReadData(sk->nGroupRef, sk->nGroup * 4, 4);
        if (sioFilePtr->ReadChunk(0) != 0x2F0)
            return 0;
        for (int k = 0; k < sk->nRef; k++) {
            sioFilePtr->ReadData((u8 *)sk->ref + k * 8, 4, 4);
            sioFilePtr->ReadData((u8 *)sk->ref + k * 8 + 4, 4, 4);
        }
        if (sioFilePtr->ReadChunk(0) != 0x2F1)
            return 0;
        for (int b = 0; b < sk->nBone; b++) {
            if (sioFilePtr->ReadChunk(0) != 0x2F2)
                return 0;
            objSKIN_BONE *bone = &sk->bone[b];
            sioFilePtr->Read(bone->name, 0x40);
            sioFilePtr->ReadData(bone->offs, sk->nRef * 12, 4);
            if (sioFilePtr->ReadChunk(0) == 0x2F5)
                sioFilePtr->ReadData(&bone->weight, 4, 4);
            else
                sioFilePtr->RevertChunk(0);
            if (sioFilePtr->ReadChunk(0) == 0x2F6) {
                bone->spl = sioFilePtr->ReadSpline();
                if (!bone->spl)
                    return 0;
            } else
                sioFilePtr->RevertChunk(0);
        }
    }
    return 1;
}
