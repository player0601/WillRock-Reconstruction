// abv_encl.cpp -- engine.lib enclosing volumes of instances (Will Rock 1.2, v12.exe; feb obj engine:abv_encl.obj)
//
// v12 0x004F7700..0x004F7FBF. The volume that encloses an instance over a whole sequence (one per sequence of
// the template, shared through animTPL::bvSeq), or a single volume around the "cdtEnclosRootObj" object / the
// instance when the template has no sequences. entRegister builds them with type 4 (OBB), mode 2 (reuse the
// template's per-sequence volumes when they exist).
// 1.2 change: AbvCalcBVolEnclos clears the instance's frozen bits (flags & 3) while it plays the sequences
// and restores them afterwards (the beta played nothing for a frozen instance).

#include "engine.h"

// v12 0x004F7CD0  (feb 0x004E57C0)
// Grow the collected box by 5 % on every side and store it in the volume: a box as it is (type 1; mode bit 1
// first replaces it by the sphere's cube around org), an OBB as its axis-aligned corner and edges (type 4).
void __fastcall _animIncreaseBVolEnclos(animINST *inst, abvBVOL *bv, int mode, m3dV *org, m3dBOX *box, m3dSPHERE *sph)
{
    m3dV o;
    bv->GetOrg(inst, &o);
    m3dVOL *v = bv->upd->vol;
    if (v->type == M3D_VOL_BOX) {
        if (mode & 1) {
            box->vMin.x = org->x - sph->radius;
            box->vMin.y = org->y - sph->radius;
            box->vMin.z = org->z - sph->radius;
            box->vMax.x = sph->radius + org->x;
            box->vMax.y = sph->radius + org->y;
            box->vMax.z = sph->radius + org->z;
        }
        float dx = (box->vMax.x - box->vMin.x) * 0.05f;
        float dy = (box->vMax.y - box->vMin.y) * 0.05f;
        float dz = (box->vMax.z - box->vMin.z) * 0.05f;
        box->vMax.x = dx + box->vMax.x;
        box->vMax.y = dy + box->vMax.y;
        box->vMax.z = dz + box->vMax.z;
        box->vMin.x = box->vMin.x - dx;
        box->vMin.y = box->vMin.y - dy;
        box->vMin.z = box->vMin.z - dz;
        m3dBOX *b = (m3dBOX *)bv->upd->vol;
        b->type = box->type;
        b->vMin = box->vMin;
        b->vMax = box->vMax;
    } else if (v->type == M3D_VOL_OBB) {
        float dx = (box->vMax.x - box->vMin.x) * 0.05f;
        float dy = (box->vMax.y - box->vMin.y) * 0.05f;
        float dz = (box->vMax.z - box->vMin.z) * 0.05f;
        box->vMax.x = dx + box->vMax.x;
        box->vMax.y = dy + box->vMax.y;
        box->vMax.z = dz + box->vMax.z;
        box->vMin.x = box->vMin.x - dx;
        box->vMin.y = box->vMin.y - dy;
        box->vMin.z = box->vMin.z - dz;
        m3dOBB *b = (m3dOBB *)v;
        b->org = box->vMin;
        b->ax[0] = m3dVZero;
        b->ax[1] = m3dVZero;
        b->ax[2] = m3dVZero;
        b->ax[0].x = box->vMax.x - box->vMin.x;
        b->ax[1].y = box->vMax.y - box->vMin.y;
        b->ax[2].z = box->vMax.z - box->vMin.z;
    }
}

// v12 0x004F7700  (feb 0x004E5210)
// type: the volume type (m3dVOL_TYPE); mode bit 1: copy the template's volume of a sequence when it has one;
// step: the time step the sequences are played with (0.1 s when negative). The volumes are collected in the
// instance's own frame (identity world matrix for OBBs, the root's local matrix cleared), unless the instance
// is a static one (flag 0x40). The current sequence and time are restored at the end.
BOOL32 animINST::AbvCalcBVolEnclos(int type, int mode, float step)
{
    struct { const void *vtbl; int type; m3dV vMin, vMax; } box = { m3dBOX_vftable, 1 };      // m3dBOX
    m3dSPHERE_D sph;
    sph.vtbl = m3dSPHERE_vftable;
    sph.type = 3;
    if (!root)
        return 1;
    AbvTermBVolEnclos();
    if (bvSeq) {
        wrDelete(bvSeq, 3);
        bvSeq = 0;
    }
    if (step < 0.0f)
        step = 0.1f;
    animSEQ *seqs = tpl->seq;
    int curSeq = seq;
    float curTime;
    if (curSeq == -1)
        curTime = 0;
    else
        curTime = seqs[curSeq].FrameToTime(frame);
    m3dV org;
    GetPos(&org);
    if (flags & 0x40) {
        Validate(2);
        ((m3dVOL *)&root->bbox)->GetCenter(&org);
    }
    float savedMatr[16];
    float savedLocal[16];
    if (!(flags & 0x40)) {
        if (type == 4) {
            memcpy(savedMatr, matr, sizeof(savedMatr));
            m3dMATR id;
            id.Identity();
            Transform(&id, 0);
        }
        if (!(flags & 0x40) && root && root->child) {
            objOBJ *c = root->child;
            memcpy(savedLocal, c->local, sizeof(savedLocal));
            ((m3dMATR *)c->local)->Identity();
            Invalidate(4);
        }
    }
    Validate(0x40);
    char name[0x40];
    objOBJ *encl = 0;
    if (GetStr((char *)"ANIM_PROP", (char *)"cdtEnclosRootObj", name, 0x40))
        encl = objFindName(root, name);
    u32 frozen = flags & 3;                             // 1.2: play the sequences even when frozen
    flags &= ~3u;
    if (!encl && tpl && tpl->nSeq) {
        int n = tpl->nSeq;
        bvSeq = wrArrayNew<abvBVOL>(n);
        if (!tpl->bvSeq)
            tpl->bvSeq = wrArrayNew<abvBVOL>(tpl ? tpl->nSeq : 0);
        for (int s = 0; s < (tpl ? tpl->nSeq : 0); s++) {
            abvBVOL *bv = &bvSeq[s];
            if ((mode & 2) && tpl->bvSeq[s].upd) {
                *bv = tpl->bvSeq[s];
                continue;
            }
            bv->Alloc(type);
            if (seqs[s].dir == 2) {                     // a single frame: the root's box
                memcpy(&box.type, &root->bbox.type, 0x1C);
                sph.center = org;
                sph.radius = 0;
                objCalcBSphereCenter(root, (m3dSPHERE *)&sph);
            } else {
                SetAnimSeq(s, 0.0f);
                ((m3dBOX *)&box)->CalcStart();
                sph.center = org;
                sph.radius = 0;
                do {
                    Validate(2);
                    ((m3dBOX *)&box)->CalcUpdate((m3dBOX *)&root->bbox);
                    objCalcBSphereCenter(root, (m3dSPHERE *)&sph);
                } while (!AdvanceTime(step, 1, 0));
            }
            _animIncreaseBVolEnclos(this, bv, type, &org, (m3dBOX *)&box, (m3dSPHERE *)&sph);
            bv->Calc(this);
            tpl->bvSeq[s] = *bv;
        }
    } else {
        abvBVOL *bv = new abvBVOL;
        bvCur = bv;
        bv->Alloc(type);
        unk08 |= 1;
        bvCur->obj = encl;
        Validate(2);
        memcpy(&box.type, &root->bbox.type, 0x1C);
        sph.center = org;
        sph.radius = 0;
        objCalcBSphereCenter(root, (m3dSPHERE *)&sph);
        _animIncreaseBVolEnclos(this, bvCur, type, &org, (m3dBOX *)&box, (m3dSPHERE *)&sph);
        bvCur->Calc(this);
    }
    flags |= frozen;
    if (tpl && tpl->nSeq)
        SetAnimSeq(curSeq, curTime);
    if (!(flags & 0x40)) {
        if (root && root->child) {
            memcpy(root->child->local, savedLocal, sizeof(savedLocal));
            Invalidate(4);
        }
        if (!(flags & 0x40) && type == 4)
            Transform((m3dMATR *)savedMatr, 0);
    }
    return 1;
}

// v12 0x004F7F10  (feb 0x004E5A00)
// The template's per-sequence volumes, from a throw-away instance in the template's own scene (made for the
// call when it has none).
BOOL32 animTPL::AbvCalcBVolEnclos()
{
    int own;
    if (!_c8) {
        scnSCENE *s = new scnSCENE;
        _c8 = (int)s;
        if (!s)
            return 0;
        if (!s->ExpandV3DTmpList(root))
            return 0;
        own = 1;
    } else {
        own = 0;
    }
    scnSCENE *s = (scnSCENE *)_c8;
    animINST *i = s->CreateInst(this, 0, -1);
    s->DestroyInst(i);
    if (own) {
        s = (scnSCENE *)_c8;
        if (s)
            delete s;
        _c8 = 0;
    }
    return 1;
}
