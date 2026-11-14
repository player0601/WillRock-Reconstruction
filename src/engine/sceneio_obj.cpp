// sceneio_obj.cpp -- engine.lib scene reader: one object (and its children), chunk 0x0F0
// (Will Rock 1.2, v12.exe; feb obj engine:sceneio_obj.obj)
//
// v12 0x0059FBA0..0x005A0BCF, rebuilt from the 1.2 code (the beta decompile of this function is garbled;
// 1.2 reads the material lists through the clamping accessors). Sub-chunks, up to the end chunk:
//   0x0F0 a child object                0x0F1 vertices (u32 n, n x 12 bytes)
//   0x0F4 name (max 0x80)               0x0F5 bounding sphere (centre, radius)
//   0x0F6 u16 hierarchy id              0x0F7 render state (masked)   0x0F8 constant colour (ARGB)
//   0x0F9 local matrix (0x40)           0x0FA / 0x0FB the shorts +0x20 / +0x22
//   0x0FC state (masked)                0x0FD the object's script
//   0x0FE faces, old format: u32 n, n x { 0xFF vertices, 0x100 u32 nMtl, nMtl x { u32 texture, 3 u16 coord
//         indices } , end } -- materials per face, then sorted and merged
//   0x101 { 0x102 u32 nSets, nSets x 0x103 { u32 n, n x 8 bytes } } coordinate sets by material control
//   0x104 three lists of u32 n words (read and dropped)
//   0x106 faces, new format: u32 n, n x { 0xFF vertices, 0x100 u32 nSets, nSets x 3 u16, end }
//   0x107 materials: 0x104 split entries { 0x105 first/count, 0x10B layers { 0x10A texture, scale, control,
//         0x10C } }, 0x109 material controls { 0x108 }, 0x0F3 coordinate sets { 0x103 } -- then the faces'
//         split numbers from the ranges and the face records trimmed to the sets used
// The masks applied to the state words (v12 0x009AF8B0 / 0x009AF8B4, function statics) are never written
// in the game: the loaded state and render-state bits are always cleared.

#include "engine.h"

static u32 s_sioStateMask;             // v12 0x009AF8B0 (feb 0x0096443C) never set: 0
static u32 s_sioRendMask;              // v12 0x009AF8B4 (feb 0x00964440) never set: 0

static inline objSPLIT *objSplitOf(objOBJ *o)
{
    return o->geom->split;
}

// v12 0x0059FBA0  (feb 0x00583870)
objOBJ *__fastcall _sioReadObj()
{
    fioCHUNK c;
    c.id = 0;
    c.size = 0xFFFFFFFF;
    c.start = 0xFFFFFFFF;
    c.index = 0xFFFFFFFF;
    m3dSPHERE_D sph;
    sph.vtbl = (void *)m3dSPHERE_vftable;
    sph.type = 3;
    if (sioFilePtr->ReadChunk(0) != 0xF0) {
        sioFilePtr->RevertChunk(0);
        return 0;
    }
    objOBJ *o = objOBJ::Make();
    if (!o)
        return 0;
    if (!sioFilePtr->ReadChunk(&c))
        return o;
    while (c.id != 1) {
        switch (c.id) {
        case 0xF0: {
            sioFilePtr->RevertChunk(0);
            objOBJ *ch = _sioReadObj();
            ch->Attach(o);
            break;
        }
        case 0xF1:
            sioFilePtr->ReadData(&o->nVert, 4, 4);
            if (o->nVert > 0) {
                if (!o->_AllocGeom() || !o->_AllocVertList(o->nVert))
                    goto fail;
                sioFilePtr->ReadData(o->geom->vert, o->nVert * 12, 4);
            }
            break;
        case 0xF2:
        case 0xF3:
            break;                                      // (no body; not skipped)
        case 0xF4: {
            char name[0x80];
            sioFilePtr->Read(name, 0x80);
            o->SetName(name);
            break;
        }
        case 0xF5:
            sioFilePtr->ReadData(&sph.center, 12, 4);
            sioFilePtr->ReadData(&sph.radius, 4, 4);
            if (o->geom) {
                o->geom->bsphere.type = sph.type;
                o->geom->bsphere.center[0] = sph.center.x;
                o->geom->bsphere.center[1] = sph.center.y;
                o->geom->bsphere.center[2] = sph.center.z;
                o->geom->bsphere.radius = sph.radius;
            }
            break;
        case 0xF6: {
            u16 id;
            sioFilePtr->ReadData(&id, 2, 2);
            o->idHier = (short)id;
            o->unk20 = (short)id;
            break;
        }
        case 0xF7: {
            u32 v;
            sioFilePtr->ReadData(&v, 4, 4);
            o->rendState = v & s_sioRendMask;
            break;
        }
        case 0xF8: {
            u32 v;
            sioFilePtr->ReadData(&v, 4, 4);
            if (o->geom)
                o->geom->constColor = v;
            break;
        }
        case 0xF9:
            sioFilePtr->ReadData(o->local, 0x40, 4);
            break;
        case 0xFA: {
            int v;
            sioFilePtr->ReadData(&v, 4, 4);
            o->unk20 = (short)v;
            break;
        }
        case 0xFB: {
            int v;
            sioFilePtr->ReadData(&v, 4, 4);
            o->unk22 = (short)v;
            break;
        }
        case 0xFC:
            sioFilePtr->ReadData(&o->state, 4, 4);
            o->state &= s_sioStateMask;
            break;
        case 0xFD:
            if (!_sioReadPSheet(&o->props))
                apLogErr((char *)"Cannot parse script for object %s", o->name);
            break;
        case 0xFE: {                                    // faces, old format: materials per face
            sioFilePtr->ReadData(&o->nFace, 4, 4);
            if (o->nFace <= 0)
                break;
            if (!o->_AllocGeom() || !o->_AllocFaceList(o->nFace, 3))
                goto fail;
            o->_CreateFaceSplitPerFace();
            for (int i = 0; i < o->nFace; i++) {
                objFACE *fc = (objFACE *)((u8 *)o->geom->faceList + o->geom->faceSize * i);
                if (sioFilePtr->ReadChunk(0) != 0xFF)
                    goto fail;
                sioFilePtr->ReadData(fc->vert, 6, 2);
                if (sioFilePtr->ReadChunk(0) != 0x100)
                    goto fail;
                int nm;
                sioFilePtr->ReadData(&nm, 4, 4);
                int lay1 = -1, lay2 = -1;
                for (int m = 0; m < nm; m++) {
                    int ti;
                    sioFilePtr->ReadData(&ti, 4, 4);
                    txmTEXTURE *t = ti >= 0 ? txmManager->GetListNmb(sioTexListNmb[ti]) : 0;
                    int ctl = -1;
                    objSPLIT *s = objSplitOf(o);
                    if (m == 0) {
                        ctl = s->ctrl.AddMtlCtrl(0);
                        s->GetEntry(i)->AddMtl(ctl, t);
                    } else if (m == 1 && t) {
                        ctl = s->ctrl.AddMtlCtrl(1);
                        lay1 = s->GetEntry(i)->AddMtl(ctl, t);
                        t->flags |= 8;
                    } else if (m == 2 && t) {
                        ctl = s->ctrl.AddMtlCtrl(2);
                        lay2 = s->GetEntry(i)->AddMtl(ctl, t);
                    } else {
                        u16 junk[3];
                        sioFilePtr->ReadData(junk, 6, 2);
                        continue;
                    }
                    if (ctl < 0) {
                        u16 junk[3];
                        sioFilePtr->ReadData(junk, 6, 2);
                        continue;
                    }
                    objMTL_CTRL_TBL *ct = &objSplitOf(o)->ctrl;
                    if (ctl > ct->n)
                        ctl = ct->n;
                    sioFilePtr->ReadData(&fc->tc[ct->c[ctl].tcInd * 3], 6, 2);
                }
                if (lay1 != -1 && lay2 != -1 && lay1 < lay2) {     // the pass-2 layer goes before pass 1
                    objSPLIT_ENTRY *e = objSplitOf(o)->GetEntry(i);
                    objMTL tmp = *e->Get(lay1);
                    *e->Get(lay1) = *e->Get(lay2);
                    *e->Get(lay2) = tmp;
                }
                if (!_sioReadLastChunk())
                    goto fail;
            }
            o->_SortFaceList();
            o->_CompressFaceSplit();
            break;
        }
        case 0x101: {                                   // coordinate sets, by material control
            if (sioFilePtr->ReadChunk(0) != 0x102)
                goto fail;
            int ns;
            sioFilePtr->ReadData(&ns, 4, 4);
            for (int k = 0; k < ns; k++) {
                if (sioFilePtr->ReadChunk(0) != 0x103)
                    goto fail;
                int n;
                sioFilePtr->ReadData(&n, 4, 4);
                if (!n)
                    continue;
                if (!o->_AllocGeom())
                    goto fail;
                int set = o->GetTCCoordIDRendMtl(k);
                o->geom->nTexCoord[set] = (u16)n;
                if (!o->_AllocTexCoord(set, n))
                    goto fail;
                sioFilePtr->ReadData(o->geom->texCoord[set], o->geom->nTexCoord[set] * 8, 4);
            }
            break;
        }
        case 0x104: {                                   // three per-something lists, not kept
            int n, junk;
            sioFilePtr->ReadData(&n, 4, 4);
            for (int k = 0; k < 3; k++)
                for (int i = 0; i < n; i++)
                    sioFilePtr->ReadData(&junk, 4, 4);
            break;
        }
        case 0x106: {                                   // faces, new format
            int n;
            sioFilePtr->ReadData(&n, 4, 4);
            if (n <= 0)
                break;
            if (!o->_AllocGeom())
                goto fail;
            for (int i = 0; i < n; i++) {
                u16 v[3];
                int ns;
                if (sioFilePtr->ReadChunk(0) != 0xFF)
                    goto fail;
                sioFilePtr->ReadData(v, 6, 2);
                if (sioFilePtr->ReadChunk(0) != 0x100)
                    goto fail;
                sioFilePtr->ReadData(&ns, 4, 4);
                if (!o->geom->faceList && !o->_AllocFaceList(n, ns))
                    goto fail;
                objFACE *fc = o->GetFace(i);
                fc->vert[0] = v[0];
                fc->vert[1] = v[1];
                fc->vert[2] = v[2];
                for (int k = 0; k < ns; k++)
                    sioFilePtr->ReadData(&fc->tc[k * 3], 6, 2);
                if (!_sioReadLastChunk())
                    goto fail;
            }
            break;
        }
        case 0x107: {                                   // materials
            fioCHUNK c1, c2, c3;
            c1.id = 0; c1.size = c1.start = c1.index = 0xFFFFFFFF;
            c2 = c1;
            c3 = c1;
            if (!o->_AllocGeom())
                break;
            objSPLIT *sp = objSPLIT::Make();
            if (!sp)
                break;
            o->geom->split = sp;                        // (a previous split is not freed)
            while (sioFilePtr->ReadChunk(&c1) && c1.id != 1) {
                if (c1.id == 0x104) {                   // split entries
                    int n;
                    sioFilePtr->ReadData(&n, 4, 4);
                    if (!objSplitOf(o)->Alloc(n))
                        goto next;                      // (the rest of the materials chunk is not read)
                    for (int e = 0; e < objSplitOf(o)->n; e++) {
                        while (sioFilePtr->ReadChunk(&c2) && c2.id != 1) {
                            if (c2.id == 0x105) {
                                int v;
                                sioFilePtr->ReadData(&v, 4, 4);
                                objSplitOf(o)->GetEntry(e)->firstFace = (u16)v;
                                sioFilePtr->ReadData(&v, 4, 4);
                                objSplitOf(o)->GetEntry(e)->nFace = (u16)v;
                            } else if (c2.id == 0x10B) {
                                int nm;
                                sioFilePtr->ReadData(&nm, 4, 4);
                                objSplitOf(o)->GetEntry(e)->nMtl = (short)(nm < 4 ? nm : 4);
                                for (int m = 0; m < objSplitOf(o)->GetEntry(e)->nMtl; m++) {
                                    while (sioFilePtr->ReadChunk(&c3) && c3.id != 1) {
                                        if (c3.id == 0x10A) {
                                            int ti;
                                            sioFilePtr->ReadData(&ti, 4, 4);
                                            if (ti >= 0 && ti < 1000)
                                                objSplitOf(o)->GetEntry(e)->Get(m)->tex =
                                                    txmManager->GetListNmb(sioTexListNmb[ti]);
                                            sioFilePtr->ReadData(&objSplitOf(o)->GetEntry(e)->Get(m)->uScale, 4, 4);
                                            sioFilePtr->ReadData(&objSplitOf(o)->GetEntry(e)->Get(m)->vScale, 4, 4);
                                            sioFilePtr->ReadData(&objSplitOf(o)->GetEntry(e)->Get(m)->ctrl, 1, 1);
                                        } else if (c3.id == 0x10C) {
                                            sioFilePtr->ReadData(&objSplitOf(o)->GetEntry(e)->Get(m)->_0c, 4, 4);
                                        } else {
                                            sioFilePtr->SkipChunk(&c3);
                                        }
                                    }
                                }
                            } else {
                                sioFilePtr->SkipChunk(&c2);
                            }
                        }
                    }
                } else if (c1.id == 0x109) {            // material controls
                    int n;
                    sioFilePtr->ReadData(&n, 4, 4);
                    objMTL_CTRL_TBL *ct = &objSplitOf(o)->ctrl;
                    ct->n = (short)(n < 4 ? n : 4);
                    for (int k = 0; k < ct->n; k++) {
                        while (sioFilePtr->ReadChunk(&c3) && c3.id != 1) {
                            if (c3.id == 0x108) {
                                u16 crd;
                                sioFilePtr->ReadData(&ct->Get(k)->tcInd, 1, 1);
                                sioFilePtr->ReadData(&crd, 2, 2);
                                ct->Get(k)->tcCoord = (u8)crd;
                                sioFilePtr->ReadData(&ct->Get(k)->rendMtl, 1, 1);
                            } else {
                                sioFilePtr->SkipChunk(&c3);
                            }
                        }
                        ct = &objSplitOf(o)->ctrl;
                    }
                } else if (c1.id == 0xF3) {             // coordinate sets 0..2
                    int dummy;
                    sioFilePtr->ReadData(&dummy, 4, 4);
                    for (int set = 0; set < 3; set++) {
                        while (sioFilePtr->ReadChunk(&c3) && c3.id != 1) {
                            if (c3.id == 0x103) {
                                int n;
                                sioFilePtr->ReadData(&n, 4, 4);
                                o->geom->nTexCoord[set] = (u16)n;
                                if (!o->_AllocTexCoord(set, n))
                                    goto next;
                                sioFilePtr->ReadData(o->geom->texCoord[set], o->geom->nTexCoord[set] * 8, 4);
                            } else {
                                sioFilePtr->SkipChunk(&c3);
                            }
                        }
                    }
                } else {
                    sioFilePtr->SkipChunk(&c1);
                }
            }
            objSplitOf(o)->RestoreFaceSplitNmb(o);
            o->_CompressFaceList(-1);
            break;
        }
        default:
            sioFilePtr->SkipChunk(&c);
            break;
        }
    next:
        if (!sioFilePtr->ReadChunk(&c))
            return o;
    }
    return o;
fail:
    o->~objOBJ();
    operator delete(o);
    return 0;
}
