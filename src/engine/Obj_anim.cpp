// Obj_anim.cpp -- engine.lib object matrices: world matrix (LTM) validation, freezing transforms into
// vertices, setting an object's world matrix (Will Rock 1.2, v12.exe; feb obj engine:Obj_anim.obj)
//
// v12 0x004DF3F0..0x004DFDEF, every function an "exact" match of the beta. The vertex loops call an
// inlined point*matrix transform that branches on m3dSimdType (x87 / 3DNow! / generic; Ghidra drops the
// x87 branch of FreezeLTM, it is there at v12 0x004DF53C): written once as m3dTransformPoint.
//
// objOBJ::ltm (+0x3C) is the object's local-to-world matrix, valid only after objValidateLTM when the
// owning instance has ANIM_INV_POS set; objOBJ::local (+0x7C) is the object's matrix in its parent's frame.
// "Freezing" bakes a matrix into the vertex positions and resets the matrix to identity; objSTATE bit 1
// marks an object whose vertices are in world space.

#include "engine.h"

#define OBJ_ST_LTM_FROZEN 0x0001          // vertices are in world space (FreezeLTM)

static inline m3dMATR *LTM(objOBJ *o)   { return (m3dMATR *)o->ltm; }
static inline m3dMATR *LOCAL(objOBJ *o) { return (m3dMATR *)o->local; }

// v12 0x004DF3F0  (feb 0x004D1170)
// The template object this instance object was duplicated from (same depth-first id).
objOBJ *__fastcall objGetTplObj(objOBJ *o)
{
    if (o->inst && o->inst->tpl)
        return objFindID(o->inst->tpl->root, o->idHier);
    return 0;
}

// v12 0x004DF420  (feb 0x004D11A0)
// Bake the current world matrix into the vertices of the whole subtree; matrices become identity.
// Vertices of an object already frozen are left as they are (but its matrices are still reset).
void objOBJ::FreezeLTM()
{
    objOBJ *first = this ? child : 0;
    for (int i = 0; i < nVert; i++) {
        m3dV p;
        if (!(state & OBJ_ST_LTM_FROZEN))
            m3dTransformPoint(&geom->vert[i], LTM(this), &p);
        else
            p = geom->vert[i];
        geom->vert[i] = p;
    }
    LTM(this)->Identity();
    LOCAL(this)->Identity();
    state |= OBJ_ST_LTM_FROZEN;
    BOOL32 started = 0;
    for (objOBJ *c = first; c && (!started || c != first); c = c->next) {
        c->FreezeLTM();
        started = 1;
    }
}

// v12 0x004DF670  (feb 0x004D13F0)
// Undo FreezeLTM for this object only: vertices back to object space with the inverse world matrix.
void objOBJ::_UnfreezeLTM()
{
    if (!inst || !(state & OBJ_ST_LTM_FROZEN))
        return;
    inst->Validate(4);
    m3dMATR w2l;
    LTM(this)->Invert(&w2l);
    for (int i = 0; i < nVert; i++) {
        m3dV p;
        m3dTransformPoint(&geom->vert[i], &w2l, &p);
        geom->vert[i] = p;
    }
    state &= ~OBJ_ST_LTM_FROZEN;
}

// v12 0x004DF850  (feb 0x004D15D0)
// Bake the local matrix into the vertices; children keep their world placement (child.local *= local).
// Only direct children are adjusted (their own vertices are not touched).
void objOBJ::_FreezeModelMatr()
{
    objOBJ *first = this ? child : 0;
    for (int i = 0; i < nVert; i++) {
        m3dV p;
        m3dTransformPoint(&geom->vert[i], LOCAL(this), &p);
        geom->vert[i] = p;
    }
    BOOL32 started = 0;
    for (objOBJ *c = first; c && (!started || c != first); c = c->next) {
        m3dMATR m;
        m3dMultiplyMatr(LOCAL(c), LOCAL(this), &m);
        *LOCAL(c) = m;
        started = 1;
    }
    LOCAL(this)->Identity();
}

// v12 0x004DFA80  (feb 0x004D1800)
// Centre: the local box centre for a free object; the object origin, or the instance centre when the
// instance has ANIM_INV_SEQ pending (bit 8 of animINST::invalid).
void objOBJ::GetCenter(m3dV *out)
{
    if (!inst) {
        ((m3dVOL *)&bbox)->GetCenter(out);
        return;
    }
    if (!(inst->invalid & ANIM_INV_SEQ)) {
        GetOrigin(out);
        return;
    }
    inst->GetCenter(out);
}

// v12 0x004DFAB0  (feb 0x004D1830)
// Rotate by deg about the world-space axis through the object's world origin.
void objOBJ::RotateGlbAxis(float deg, m3dV *axis)
{
    m3dV org;
    m3dMATR w2l, rot;
    LTM(this)->GetOrigin(&org);
    LTM(this)->Invert(&w2l);
    rot.MakeRotRay(&org, axis, deg);
    LOCAL(this)->Transform(&w2l, 1);
    LOCAL(this)->Transform(&rot, 1);
    LOCAL(this)->Transform(LTM(this), 1);
    DeclareProcAnim();
}

// v12 0x004DFB30  (feb 0x004D18B0)
// Place the object in the world: origin org and any subset of the axes vx / vy / vz (0 = keep the
// current orientation when all three are 0). With keepChild the FIRST child (only that one, not its
// siblings) is counter-transformed so that it stays where it was.
void objOBJ::SetMatrLT(m3dV *org, m3dV *vx, m3dV *vy, m3dV *vz, int keepChild)
{
    m3dMATR w, w2l, d, dInv;
    if (!vx && !vy && !vz) {
        w = *LTM(this);
        w.Translate(org, 0);
    } else if (!vx && !vy) {
        w.MakeLCS2WCS_VZ(org, vz);
    } else if (!vx && !vz) {
        w._MakeLCS2WCS_VY(org, vy);
    } else if (!vy && !vz) {
        w._MakeLCS2WCS_VX(org, vx);
    } else {
        w.MakeLCS2WCS(org, vx, vy, vz);
    }
    LTM(this)->Invert(&w2l);
    m3dMultiplyMatr(&w, &w2l, &d);
    LOCAL(this)->Transform(&d, 1);
    if (keepChild && child) {
        d.Invert(&dInv);
        LOCAL(child)->Transform(&dInv, 2);
    }
    inst->Invalidate(4);
    DeclareProcAnim();
}

// v12 0x004DFC70  (feb 0x004D19F0)
// Make o->ltm current. Only does work while the instance has ANIM_INV_POS pending; with a sequence or
// frame change pending (ANIM_INV_SEQ | ANIM_INV_FRAME) the animation is applied to the chain first.
void __fastcall objValidateLTM(objOBJ *o)
{
    if (!o || !o->inst || !(o->inst->invalid & ANIM_INV_POS))
        return;
    if (o->inst->invalid & (ANIM_INV_SEQ | ANIM_INV_FRAME)) {
        if (!o->parent) {
            o->inst->Validate(0);
            o->inst->StartApplyAnim();
        } else {
            _objValidateAnim(o->parent);
        }
        o->inst->_CalcObjModelMatrix(o);
    }
    if (o->parent) {
        _objValidateLTM(o->parent);
        if (o->state & OBJ_ST_NO_PARENT_XFORM)
            *LTM(o) = *LTM(o->parent);
        else
            m3dMultiplyMatr(LOCAL(o), LTM(o->parent), LTM(o));
        return;
    }
    *LTM(o) = o->inst ? *(m3dMATR *)o->inst->matr : *LOCAL(o);
}

// v12 0x004DFD30  (feb 0x004D1AB0)
// Recompute the ltm of o and all its ancestors (root first), unconditionally.
void __fastcall _objValidateLTM(objOBJ *o)
{
    if (!o->parent) {
        *LTM(o) = o->inst ? *(m3dMATR *)o->inst->matr : *LOCAL(o);
        return;
    }
    _objValidateLTM(o->parent);
    if (o->state & OBJ_ST_NO_PARENT_XFORM)
        *LTM(o) = *LTM(o->parent);
    else
        m3dMultiplyMatr(LOCAL(o), LTM(o->parent), LTM(o));
}

// v12 0x004DFDA0  (feb 0x004D1B20)
// Apply the animation to o's chain (root first).
void __fastcall _objValidateAnim(objOBJ *o)
{
    if (o->parent) {
        _objValidateAnim(o->parent);
        o->inst->_CalcObjModelMatrix(o);
        return;
    }
    o->inst->Validate(0);
    o->inst->StartApplyAnim();
    o->inst->_CalcObjModelMatrix(o);
}

// feb 0x004D1B70 _$E2: per-obj m3dEpsSq initializer of the m3d library header
M3D_DECLARE_EPS_SQ();
