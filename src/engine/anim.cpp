// anim.cpp -- engine.lib animation templates and instances: names, invalidation / validation of the derived
// data, placement, boxes, collision data, property lookups (Will Rock 1.2, v12.exe; feb obj engine:anim.obj)
//
// v12 0x004CD300..0x004CED17. 1.2 changes, from the disassembly:
//   - UpdateInvalidateState / UpdateValidateState: other bit tables (state 1 no longer drags in the vertex
//     normals; state 4 ORs instead of replacing; 0x400 / 0x1000 need less);
//   - Validate recomputes the normals through the 1.2 normal functions and starts the skin before the vertex
//     normals; GetCenter falls back to the instance origin when there is no enclosing volume;
//   - CDTInit allocates a 0x74-byte cdtCOLL_INST (cdtCOLL grew to 0x70).
// RotateX/RotateY/SetPosDir_XZ/GetDirUp/GetHEX have no listed twin but are in 1.2 at the addresses given.
//
// invalid (+0x0C) bits: 1 face normals, 2 object boxes, 4 world matrices, 0x08/0x10 animation, 0x20 hit boxes,
// 0x40 world-to-local, 0x80 matr -> root local, 0x100 enclosing volume, 0x200 skin, 0x400/0x800/0x1000 normals
// and their perpendicular vectors, 0x2000/0x4000/0x10000 modifiers, 0x8000 modifier time, 0x20000 modifier
// hierarchy, 0x40000 instance box (+ collision grid).

#include "engine.h"

#define AI_FIELD(t, off) (*(t *)((u8 *)this + (off)))

// v12 0x004CD300  (feb 0x004C2BC0)
void animTPL::DestroyData()
{
    AbvTerm();
    for (int i = 0; i < nSkin; i++) {
        wrVCall<void>(skinList[i], 8);              // Term
        if (skinList[i])
            wrVCall<void *>(skinList[i], 0, 1u);    // deleting dtor
    }
    apFree(skinList);
    skinList = 0;
    if (root) {
        root->~objOBJ();
        operator delete(root);
    }
    root = 0;
    if (seq)
        operator delete((int *)seq - 1);                // new animSEQ[] (trivial dtor, cookie)
    seq = 0;
    if (objAnim) {
        int n = ((int *)objAnim)[-1];
        for (int i = n - 1; i >= 0; i--)
            objAnim[i].~animOBJ_ANIM();
        operator delete((int *)objAnim - 1);
    }
    objAnim = 0;
    if (aniSystem)
        wrVCall<void *>(aniSystem, 0, 1u);
    aniSystem = 0;
    if (sel) {
        int n = ((int *)sel)[-1];
        for (int i = n - 1; i >= 0; i--) {
            operator delete(sel[i].obj);
            sel[i].obj = 0;
            sel[i].n = 0;
        }
        operator delete((int *)sel - 1);
    }
    sel = 0;
    nSel = 0;
    if (props) {
        props->Empty();
        operator delete(props);
    }
    props = 0;
}

// v12 0x004CD470  (feb 0x004C2D30)
int animTPL::FindSeqName(char *name)
{
    for (int i = 0; i < nSeq; i++)
        if (!strcmp((char *)&seq[i], name))             // the name is the first field of animSEQ
            return i;
    return -1;
}

// v12 0x004CD4E0  (feb 0x004C2DA0)
BOOL32 animTPL::IsDummy()
{
    return memcmp(tplName, "__DUMMY__", 10) == 0;
}

// v12 0x004CD500  (feb 0x004C2DC0)
BOOL32 animINST::SetName(char *s)
{
    apFree(name);
    name = 0;
    if (s) {
        name = apStrdup(s);
        if (!name)
            return 0;
    }
    return 1;
}

// v12 0x004CD540  (feb 0x004C2E00)
BOOL32 animINST::SetNameClass(char *s)
{
    apFree(className);
    className = 0;
    if (s) {
        className = apStrdup(s);
        if (!className)
            return 0;
    }
    return 1;
}

// v12 0x004CD580  (feb 0x004C2E40)
BOOL32 animINST::SetNameTpl(char *s)
{
    apFree(tplName);
    tplName = 0;
    if (s) {
        tplName = apStrdup(s);
        if (!tplName)
            return 0;
    }
    return 1;
}

// v12 0x004CD5C0  (feb 0x004C2E80)  [1.2 table, from the disassembly]
int animINST::UpdateInvalidateState(int b)
{
    u32 r = 0;
    if (b & 1)
        r = 0x801;
    if (b & 0x800)
        r |= 1;
    if (b & 0x1000)
        r |= 0x1400;
    if (b & 2)
        r |= 2;
    if (b & 4) {
        r |= 0x1E07;
        if (!(flags & 0x80))
            r |= 0x20;
    }
    if (b & 0x200)
        r |= 0x1E03;
    if (b & 8) {
        r |= 0x1E0F;
        if (!(flags & 0x80))
            r |= 0x20;
        if (bvCur && *(int *)((u8 *)bvCur + 4))
            r |= 0x40100;
    }
    if (b & 0x10) {
        r |= 0x10;
        if (!(flags & 0x80))
            r |= 0x20;
    }
    if (b & 0x80)
        r |= 0x41FE7;
    if (b & 0x100)
        r |= 0x40100;
    if (b & 0x20)
        r |= 0x20;
    if (b & 0x40)
        r |= 0x40;
    if (b & 0x40000)
        r |= 0x40000;
    if (b & 0x8000)
        r |= 0x8000;
    if (b & 0x10000)
        r |= 0x10000;
    if (b & 0x2000)
        r |= 0x2000;
    if (b & 0x4000)
        r |= 0x4000;
    return r;
}

// v12 0x004CD6A0  (feb 0x004C2F60)  [1.2 table, from the disassembly]
int animINST::UpdateValidateState(int b)
{
    u32 r = 0;
    if (b & 1)
        r = 0x21D;
    if (b & 0x800)
        r |= 0xA1C;
    if (b & 0x400)
        r |= 0x161C;
    if (b & 0x1000)
        r |= 0x121C;
    if (b & 2)
        r |= 0x21E;
    if (b & 4)
        r |= 0x1C;
    if (b & 8)
        r |= 0x18;
    if (b & 0x10)
        r |= 0x10;
    if (b & 0x200)
        r |= 0x21C;
    if (b & 0x20)
        r |= 0x20;
    if (b & 0x100)
        r |= 0x100;
    if (b & 0x40000)
        r |= 0x40100;
    if (b & 0x40)
        r |= 0x40;
    if (b & 0x8000)
        r |= 0x8000;
    if (b & 0x10000)
        r |= 0x10000;
    if (b & 0x2000)
        r |= 0x2000;
    if (b & 0x4000)
        r |= 0x4000;
    return r;
}

// v12 0x004CD770  (feb 0x004C3030)
// Mark derived data stale (and everything depending on it); a changed box re-queues the instance in the
// scene's instance grid.
void animINST::Invalidate(int bits)
{
    if ((invalid & bits) == (u32)bits)
        return;
    if (bits & 0x20000) {
        invalid |= 0x20000;
        bits &= ~0x20000;
    }
    if (invalid & 0x20000) {
        root->ValidateModifiersHier(0, 0);
        invalid &= ~0x20000u;
    }
    u32 u = UpdateInvalidateState(bits);
    if (root) {
        if (flags & 0x20000000)
            u = root->InvalidateModifiers(u);
        else if (root->invalidMask & u)
            u |= root->invalidAdd;
    }
    u = UpdateInvalidateState(u);
    invalid |= u;
    if ((u & 0x40000) && !(flags & 0x10) && scene && scene->collAnim)
        scene->collAnim->Invalidate(index, 0);
}

// v12 0x004CD830  (feb 0x004C30F0)  [callgraph: the normal recomputation is the 1.2 one]
// Bring the derived data named by bits (and what it needs) up to date.
void animINST::Validate(int bits)
{
    if (!(bits & invalid))
        return;
    if (invalid & 0x20000) {
        root->ValidateModifiersHier(0, 0);
        invalid &= ~0x20000u;
    }
    u32 u = UpdateValidateState(bits);
    if (root) {
        if (flags & 0x20000000)
            u = root->ValidateModifiers(u);
        else if (root->invalidAdd & u)
            u |= root->invalidMask;
        u = UpdateValidateState(u);
    }
    if (invalid & 0x8000) {
        root->AdvanceModifiersTime();
        invalid &= ~0x8000u;
    }
    if (invalid & 0x80) {                               // the instance matrix is the root's local matrix
        if (root)
            memcpy(root->local, matr, sizeof(matr));
        invalid &= ~0x80u;
    }
    if ((u & 0x18) && (invalid & 0x18)) {
        ApplyAnim();
        invalid &= ~0x18u;
    }
    if ((u & 4) && (invalid & 4)) {
        objCalcLTM(root);
        root->ApplyModifiers(4);
        invalid &= ~4u;
    }
    if ((u & 0x200) && (invalid & 0x200)) {
        if (root && (root->invalidAdd & 0x200) && objTable) {
            objMOD_SKIN::StartApply(objTable);
            root->ApplyModifiers(0x200);
        }
        invalid &= ~0x200u;
    }
    if ((u & 2) && (invalid & 2)) {
        objCalcBBox(root);
        invalid &= ~2u;
    }
    u32 inv = invalid, c = inv & 0x800;
    if (c && (u & 1) && (inv & 1)) {
        root->CalcFaceNormals(0);
        invalid = inv & 0xFFFFF7FE;
    } else {
        if ((u & 0x800) && c) {
            root->CalcFacePerpVectors();
            invalid &= ~0x800u;
        }
        if ((u & 1) && (invalid & 1)) {
            root->NormalizeFacePerpVectors();
            invalid &= ~1u;
        }
    }
    inv = invalid;
    c = inv & 0x1000;
    if (c && (u & 0x400) && (inv & 0x400)) {
        objMOD_SKIN::StartApply(objTable);
        root->CalcVertNormals(0);
        root->ApplyModifiers(0x400);
        invalid = inv & 0xFFFFEBFF;
    } else {
        if ((u & 0x1000) && c) {
            objMOD_SKIN::StartApply(objTable);
            root->CalcVertPerpVectors();
            root->ApplyModifiers(0x1000);
            invalid &= ~0x1000u;
        }
        if ((u & 0x400) && (invalid & 0x400)) {
            root->NormalizeVertPerpVectors();
            root->ApplyModifiers(0x400);
            invalid &= ~0x400u;
        }
    }
    if ((u & 0x20) && (invalid & 0x20)) {
        AbvUpdateBVList();
        invalid &= ~0x20u;
    }
    if ((u & 0x40) && (invalid & 0x40)) {
        ((m3dMATR *)matr)->Invert((m3dMATR *)matrW2L);
        invalid &= ~0x40u;
    }
    if ((u & 0x100) && (invalid & 0x100)) {
        abvBVOL *bv = bvCur;
        if (bv) {                                       // update the enclosing volume, then its enclosing sphere
            abvVOL_UPD *upd = bv->upd;
            wrVCall<void>(upd, 8, this, bv);
            wrVCall<void>(*(void **)((u8 *)upd + 4), 0x1C, (void *)((u8 *)bv + 0xC));
        }
        invalid &= ~0x100u;
    }
    if ((u & 0x40000) && (invalid & 0x40000)) {
        ValidateBBox();
        invalid &= ~0x40000u;
    }
    if ((u & 0x2000) && (invalid & 0x2000)) {
        root->ApplyModifiers(0x2000);
        invalid &= ~0x2000u;
    }
    if ((u & 0x4000) && (invalid & 0x4000)) {
        root->ApplyModifiers(0x4000);
        invalid &= ~0x4000u;
    }
    if (u & 0x8000)
        invalid &= ~0x8000u;
    if (u & 0x10000) {
        root->ApplyModifiers(0x10000);
        invalid &= ~0x10000u;
    }
}

// v12 0x004CDB80  (feb 0x004C3420)
// Spawn data of this instance: sequence and frame, scale (root child scale x matrix scale), matrix without
// scale, props / seed, name.
void animINST::MakeCreateData(void *p)
{
    animCREATE_DATA *cd = (animCREATE_DATA *)p;
    memset(cd, 0, sizeof(animCREATE_DATA));
    if (!tpl || tpl->nSeq < 1) {
        cd->seqId = -1;
        cd->unk0E = 0;
    } else {
        cd->seqId = (short)seq;
        cd->f_0E = frame;
    }
    float sx, sy, sz;
    if (!root || !root->child) {
        sz = 1.0f;
        sy = 1.0f;
        sx = 1.0f;
    } else {
        ((m3dMATR *)root->child->local)->GetScale(&sx, &sy, &sz);
    }
    memcpy(&cd->matr, matr, sizeof(matr));
    float mx, my, mz;
    cd->matr.GetScale(&mx, &my, &mz);
    cd->matr.RemoveScale();
    cd->scale.x = mx * sx;
    cd->scale.y = my * sy;
    cd->scale.z = mz * sz;
    cd->animSeed = seed;
    strncpy(cd->name, name ? name : (char *)"", 0x3F);
    cd->name[0x3E] = 0;                                 // (byte +0x90)
}

// v12 0x004CDCA0  (feb 0x004C3540)
void animINST::ApplyCreateData(void *p)
{
    animCREATE_DATA *cd = (animCREATE_DATA *)p;
    if (!cd) {
        if (tpl && tpl->nSeq)
            return;
        seq = -1;
        return;
    }
    float sx, sy, sz;
    cd->matr.GetScale(&sx, &sy, &sz);
    cd->matr.RemoveScale();
    ((m3dMATR *)matr)->Transform(&cd->matr, 0);
    Invalidate(0xC4);
    float t = 0;
    if (cd->seqId >= 0 && cd->seqId < (tpl ? tpl->nSeq : 0))
        t = tpl->seq[cd->seqId].FrameToTime(cd->f_0E);
    SetAnimSeq(cd->seqId, t);
    seed = cd->animSeed;
    if (root && root->child)
        ((m3dMATR *)root->child->local)->Scale(cd->scale.x * sx, cd->scale.y * sy, cd->scale.z * sz, 0);
    Invalidate(0xC4);
    Invalidate(0x80);
    if (cd->name[0]) {
        apFree(name);
        name = 0;
        name = apStrdup(cd->name);
    }
}

// v12 0x004CDE10  (feb 0x004C36B0)
void animINST::DestroyData()
{
    apFree(AI_FIELD(void *, 0x2C));
    AbvTerm();
    LODTerm();
    if (root) {
        root->~objOBJ();
        operator delete(root);
    }
    if (props) {
        props->Empty();
        operator delete(props);
    }
    if (AI_FIELD(void *, 0x4C))
        apFree(AI_FIELD(void *, 0x4C));
    apFree(objTable);
    if (coll)
        coll->__vecDelDtor(1);
    coll = 0;
    apFree(name);
    apFree(className);
    apFree(tplName);
    scene = 0;
}

// v12 0x004CDED0  (feb 0x004C3770)
BOOL32 animINST::SetObjInstRef()
{
    if (root) {
        root->inst = this;
        for (objOBJ *f = root->child, *c = f; c; c = c->next == f ? 0 : c->next)
            _animSetObjInstRef(c, this);
    }
    return 1;
}

// v12 0x004CDF20  (feb 0x004C37C0)
void animINST::RotateX(float a, int space)
{
    ((m3dMATR *)matr)->RotateX(a, space);
    Invalidate(0xC4);
}

// v12 0x004CDF50  (feb 0x004C37F0)
void animINST::RotateY(float a, int space)
{
    ((m3dMATR *)matr)->RotateY(a, space);
    Invalidate(0xC4);
}

// v12 0x004CDF80  (feb 0x004C3820)
void animINST::Translate(float x, float y, float z, int space)
{
    ((m3dMATR *)matr)->Translate(x, y, z, space);
    Invalidate(0xC4);
}

// v12 0x004CDFB0  (feb 0x004C3850)
void animINST::Translate(m3dV *v, int space)
{
    ((m3dMATR *)matr)->Translate(v, space);
    Invalidate(0xC4);
}

// v12 0x004CDFE0  (feb 0x004C3880)  scale of the root's (first) child
void animINST::Scale(float x, float y, float z, int space)
{
    if (root && root->child)
        ((m3dMATR *)root->child->local)->Scale(x, y, z, space);
    Invalidate(0xC4);
}

// v12 0x004CE020  (feb 0x004C38C0)
float animINST::GetScale()
{
    if (root && root->child) {
        float x, y, z;
        ((m3dMATR *)root->child->local)->GetScale(&x, &y, &z);
        return (x + y + z) * 0.33333334f;
    }
    return (1.0f + 2.0f) * 0.33333334f;
}

// v12 0x004CE090  (feb 0x004C3930)
void animINST::Transform(m3dMATR *m, int mode)
{
    ((m3dMATR *)matr)->Transform(m, mode);
    Invalidate(0xC4);
}

// v12 0x004CE0C0  (feb 0x004C3960)
void animINST::SetPos(m3dV *p)
{
    ((m3dMATR *)matr)->Translate(p, 0);
    Invalidate(0xC4);
}

// v12 0x004CE0F0  (feb 0x004C3990)
void animINST::SetDir_XZ(m3dV *dir)
{
    m3dV o;
    ((m3dMATR *)matr)->GetOrigin(&o);
    ((m3dMATR *)matr)->MakeLCS2WCS_VZ(&o, dir);
    Invalidate(0xC4);
}

// v12 0x004CE130  (feb 0x004C39D0)
// Turn to dir about the pivot point (world): the pivot keeps its world position.
void animINST::SetDir_XZ_UsePivot(m3dV *dir, m3dV *pivot)
{
    m3dMATR *m = (m3dMATR *)matr;
    m3dV o, ax, ay, az;
    m->GetOrigin(&o);
    m3dV d = { pivot->x - o.x, pivot->y - o.y, pivot->z - o.z };
    m->GetAxisX(&ax);
    m->GetAxisY(&ay);
    m->GetAxisZ(&az);
    m3dV l = { d.x * ax.x + ax.y * d.y + ax.z * d.z,     // pivot in local space
               ay.x * d.x + ay.y * d.y + ay.z * d.z,
               az.x * d.x + az.y * d.y + az.z * d.z };
    m->MakeLCS2WCS_VZ(&o, dir);
    Invalidate(0xC4);
    m3dV w;
    m3dTransformPoint(&l, m, &w);                       // where the pivot went
    float dz = w.z - pivot->z;
    o.x = o.x - (w.x - pivot->x);
    o.y = o.y - (w.y - pivot->y);
    o.z = o.z - dz;
    m->Translate(&o, 0);
    Invalidate(0xC4);
}

// v12 0x004CE3C0  (feb 0x004C3C60)
void animINST::SetPosDir_XZ(m3dV *pos, m3dV *dir)
{
    ((m3dMATR *)matr)->MakeLCS2WCS_VZ(pos, dir);
    Invalidate(0xC4);
}

// v12 0x004CE3F0  (feb 0x004C3C90)
void animINST::GetPos(m3dV *out)
{
    ((m3dMATR *)matr)->GetOrigin(out);
}

// v12 0x004CE400  (feb 0x004C3CA0)
void animINST::GetDir(m3dV *out)
{
    ((m3dMATR *)matr)->GetAxisZ(out);
}

// v12 0x004CE410  (feb 0x004C3CB0)
void animINST::GetDirUp(m3dV *out)
{
    ((m3dMATR *)matr)->GetAxisY(out);
}

// v12 0x004CE420  (feb 0x004C3CC0)
void animINST::GetDirRight(m3dV *out)
{
    ((m3dMATR *)matr)->GetAxisX(out);
    out->x *= -1.0f;
    out->y *= -1.0f;
    out->z *= -1.0f;
}

// v12 0x004CE460  (feb 0x004C3D00)
void animINST::GetMatrL2W(m3dMATR *out)
{
    memcpy(out, matr, sizeof(matr));
    if (root && root->child)
        m3dMultiplyMatr((m3dMATR *)root->child->local, (m3dMATR *)matr, out);
}

// v12 0x004CE4A0  (feb 0x004C3D40)  [1.2: no enclosing volume -> the instance origin]
void animINST::GetCenter(m3dV *out)
{
    if (centerObj) {
        if (centerObj == root) {
            ((m3dMATR *)matr)->GetOrigin(out);
            return;
        }
        objValidateLTM(centerObj);
        centerObj->GetOrigin(out);
        return;
    }
    if (bvCur) {
        Validate(0x100);
        wrVCall<void>(*(void **)((u8 *)bvCur->upd + 4), 0x24, out);   // the volume's GetCenter
        return;
    }
    ((m3dMATR *)matr)->GetOrigin(out);
}

// v12 0x004CE520  (feb 0x004C3DA0)
void animINST::GetBBox(m3dBOX *out)
{
    if (invalid & 0x40000)
        Validate(0x40000);
    ((m3dBOX_DATA *)out)->type = bbox.type;
    memcpy(&out->vMin, bbox.min, 12);
    memcpy(&out->vMax, bbox.max, 12);
}

// v12 0x004CE5D0  (feb 0x004C3E00)
// v12 0x004CE580  (1.2 only; feb inlined it) -- the length of a sequence in seconds, 0 for a bad index.
float animINST::GetSeqLength(int s)
{
    if (s < 0 || s >= (tpl ? tpl->nSeq : 0))
        return 0.0f;
    return tpl->seq[s].lengthMs * 0.001f;
}

// Box of the enclosing volume (box / prism / OBB corners); without one, the origin.
void animINST::ValidateBBox()
{
    m3dV v[8];
    m3dBOX *b = (m3dBOX *)&bbox;
    if (!bvCur) {
        ((m3dMATR *)matr)->GetOrigin(&v[0]);
        b->CalcStart();
        b->CalcUpdate(&v[0]);
        return;
    }
    m3dVOL *vol = *(m3dVOL **)((u8 *)bvCur->upd + 4);
    int type = ((int *)vol)[1];
    if (type == 1) {
        bbox.type = ((int *)vol)[1];
        memcpy(bbox.min, (u8 *)vol + 8, 12);
        memcpy(bbox.max, (u8 *)vol + 0x14, 12);
    } else if (type == 2) {
        ((m3dPRISM *)vol)->GetVertices(v);
        b->CalcStart();
        b->CalcUpdate(v, 8);
    } else if (type == 4) {
        ((m3dOBB *)vol)->GetVertices(v);
        b->CalcStart();
        b->CalcUpdate(v, 8);
    }
}

// v12 0x004CE6C0  (feb 0x004C3EF0)
// Own collision data when the instance (or its template) script has a [COLL] section:
// gridSize (1), maxNVox (10), seqName / time (the frame to build it for).
BOOL32 animINST::CDTInit()
{
    psSHEET *ps = props ? props : tpl->props;
    if (!ps || !ps->FindSect((char *)"COLL"))
        return 1;
    float g = 1.0f;
    if ((ps = props ? props : tpl->props))
        ps->GetFloat((char *)"COLL", (char *)"gridSize", &g);
    m3dV vox = { g, g, g };
    int nv = 10;
    if ((ps = props ? props : tpl->props))
        ps->GetInt((char *)"COLL", (char *)"maxNVox", &nv);
    int maxVox[3] = { nv, nv, nv };
    char sq[0x40];
    sq[0] = 0;
    if ((ps = props ? props : tpl->props))
        ps->GetStr((char *)"COLL", (char *)"seqName", sq, 0x40);
    float t = 0;
    if ((ps = props ? props : tpl->props))
        ps->GetFloat((char *)"COLL", (char *)"time", &t);
    return CDTInit(&vox, (m3dVINT *)maxVox, sq, t);
}

// v12 0x004CE810  (feb 0x004C4040)
BOOL32 animINST::CDTInit(m3dV *vox, m3dVINT *maxVox, char *sq, float t)
{
    if (sq && strlen(sq)) {
        for (int i = 0; i < tpl->nSeq; i++)
            if (!strcmp((char *)&tpl->seq[i], sq)) {
                SetAnimSeq(i, t);
                break;
            }
    }
    Validate(7);
    if (coll)
        coll->__vecDelDtor(1);
    cdtCOLL_INST *c = (cdtCOLL_INST *)operator new(0x74);
    if (c)
        new (c) cdtCOLL_INST;                           // cdtCOLL ctor, vtable v12 0x00606D70
    coll = c;
    if (!c)
        return 0;
    c->data = this;
    int *mv = (int *)maxVox;
    if (!c->CalcData2(vox->x, vox->y, vox->z, mv[0], mv[1], mv[2], 0))
        return 0;
    if (sq && strlen(sq))
        c->SetCDTFrame(frame);
    return 1;
}

// v12 0x004CECF0  (feb 0x004C4190)  deleting dtor shared (identical code) by cdtCOLL_OBJ/SCN/INST
void *cdtCOLL_OBJ::__vecDelDtor(unsigned f)
{
    Destroy();                                          // vtable cdtCOLL_OBJ v12 0x00606DE0, then cdtCOLL::~cdtCOLL
    if (f & 1)
        operator delete(this);
    return this;
}

// The property lookups: the instance sheet, else the template's (the sheets themselves fall back further).
#define AI_SHEET() (props ? props : tpl->props)

// v12 0x004CE960  (feb 0x004C41C0)
BOOL32 animINST::GetStr(char *s, char *key, char *out, int n)
{
    psSHEET *ps = AI_SHEET();
    return ps && ps->GetStr(s, key, out, n);
}

// v12 0x004CE9B0  (feb 0x004C4210)
BOOL32 animINST::GetInt(char *s, char *key, int *out)
{
    psSHEET *ps = AI_SHEET();
    return ps && ps->GetInt(s, key, out);
}

// v12 0x004CEA00  (feb 0x004C4260)
BOOL32 animINST::GetBool(char *s, char *key, int *out)
{
    psSHEET *ps = AI_SHEET();
    return ps && ps->GetBool(s, key, out);
}

// v12 0x004CEA50  (feb 0x004C42B0)
BOOL32 animINST::GetFloat(char *s, char *key, float *out)
{
    psSHEET *ps = AI_SHEET();
    return ps && ps->GetFloat(s, key, out);
}

// v12 0x004CEAA0  (feb 0x004C4300)
BOOL32 animINST::GetColor(char *s, char *key, void *out)
{
    psSHEET *ps = AI_SHEET();
    return ps && ps->GetColor(s, key, (m3dCOLOR *)out);
}

// v12 0x004CEAF0  (feb 0x004C4350)
BOOL32 animINST::GetHEX(char *s, char *key, u32 *out)
{
    psSHEET *ps = AI_SHEET();
    return ps && ps->GetHEX(s, key, out);
}

// v12 0x004CEB40  (feb 0x004C43A0)
// Bounding sphere of an object: its own (free objects), its bone's hit box, or the instance's enclosing volume.
m3dSPHERE *__fastcall animGetObjBSphere(objOBJ *o)
{
    animINST *i = o->inst;
    if (!i)
        return (m3dSPHERE *)&o->geom->bsphere;
    if (o->unk1E >= 0 && o->unk1E < i->nBVol) {
        i->Validate(0x20);
        return (m3dSPHERE *)((u8 *)&i->bvol[o->unk1E] + 0xC);
    }
    i->Validate(0x100);
    return (m3dSPHERE *)((u8 *)i->bvCur + 0xC);
}

// v12 0x004CEBB0  (feb 0x004C4410)
void animCTRL_3D_FOLLOW::SetFollow(animINST *i, objOBJ *o)
{
    if (o && !i)
        i = o->inst;
    inst = i;
    obj = o;
}

// v12 0x004CEBD0  (feb 0x004C4430)
BOOL32 animCTRL_3D_FOLLOW::IsFollow(animINST *i)
{
    return i && i == inst;
}

// v12 0x004CEBF0  (feb 0x004C4450)
BOOL32 animCTRL_3D_FOLLOW::IsFollow(objOBJ *o)
{
    return o && o == obj;
}

// v12 0x004CEC10  (feb 0x004C4470)
void animCTRL_3D_FOLLOW::GetValue(m3dV *out)
{
    if (!inst) {
        if (!obj) {
            *out = m3dVZero;
            return;
        }
    } else if (!obj) {
        ((m3dMATR *)inst->matr)->GetOrigin(out);
        return;
    }
    if (inst)
        inst->Validate(4);
    obj->GetOrigin(out);
}

// v12 0x004CEC70  (feb 0x004C4630)
int __fastcall _animSetObjInstRef(objOBJ *o, animINST *i)
{
    if (o) {
        o->inst = i;
        for (objOBJ *f = o->child, *c = f; c; c = c->next == f ? 0 : c->next)
            _animSetObjInstRef(c, i);
    }
    return 1;
}

// v12 0x004CECB0  (feb 0x004C4670)
animOBJ_ANIM::~animOBJ_ANIM()
{
    if (spl14) spl14->__vecDelDtor(1);
    if (spl28) spl28->__vecDelDtor(1);
    if (spl38) spl38->__vecDelDtor(1);
    if (spl3C) spl3C->__vecDelDtor(1);
}

// v12 0x004CD750 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
