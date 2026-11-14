// rend_anim.cpp -- the render driver's instance side (Will Rock 1.2, v12.exe; feb obj engine:Rend_anim.obj)
//
// Rebuilt so far: QualInst, the test an instance passes before it is drawn. Not rebuilt: RenderInst (rend_driver.h).

#include "engine.h"
#include "../../include/engine_render/rend_driver.h"
#include "../../include/math_app/camera.h"
#include "../wr_vcall.h"

// v12 0x0059BA70  (feb 0x0057FFD0)
// Whether an instance is drawn (1) or not (0); *qual is how its volume sits in the view (1 outside, -1 inside,
// 0 across), for its objects. Not drawn: one the driver's FilterInst refuses, one without objects, hidden (flags
// 1 / 2), not of this pass (the mirror pass (driver flags 8) leaves out those flagged 0x80000, the main one none
// flagged 0x100000; driver flags 0x100: only those flagged 0x40000000, else none of them), one whose current
// enclosing volume is outside the view frustum or behind the clip plane (flags 0x10) -- not tested when flagged
// 0x40000 (always in view) -- and one whose enclosing sphere, seen from the camera, is less than 2 pixels across
// (far or tiny instances are not drawn).
int rendDRIVER::QualInst(animINST *a, int *qual)
{
    m3dSPHERE_D sph;
    sph.radius = 0;
    sph.type = M3D_VOL_SPHERE;
    sph.vtbl = 0;                                       // (an m3dSPHERE on the stack: vtable v12 0x005FB68C)
    if (wrVCall<int>(this, 0x1C, a) || !a->root || (a->flags & 3))   // FilterInst
        return 0;
    if (flags & 8) {
        if (a->flags & 0x80000)
            return 0;
    } else if (a->flags & 0x100000)
        return 0;
    if (flags & 0x100) {
        if (!(a->flags & 0x40000000))
            return 0;
    } else if (a->flags & 0x40000000)
        return 0;
    a->Validate(0x8100);
    abvBVOL *bv = a->bvCur;
    int q;
    if (a->flags & 0x40000)
        q = 0;
    else {
        m3dVOL *v = bv->upd->vol;
        switch (v->type) {
        case M3D_VOL_BOX:
            q = camera->QualBoxPOV((m3dBOX *)v);
            v->CalcEnclSph((m3dSPHERE *)&sph);
            break;
        case M3D_VOL_PRISM:
            q = camera->QualPrismPOV((m3dPRISM *)v);
            v->CalcEnclSph((m3dSPHERE *)&sph);
            break;
        case M3D_VOL_SPHERE:
            sph.type = v->type;
            sph.center = ((m3dSPHERE_D *)v)->center;
            sph.radius = ((m3dSPHERE_D *)v)->radius;
            q = camera->QualSpherePOV((m3dSPHERE *)&sph);
            break;
        case M3D_VOL_OBB:
            q = camera->QualOBBPOV((m3dOBB *)v);
            v->CalcEnclSph((m3dSPHERE *)&sph);
            break;
        default:
            q = 0;                                      // (the game leaves a stale register here: not 1)
            break;
        }
        if (q == 1)
            return 0;
    }
    if ((flags & 0x10) && !(a->flags & 0x40000)) {
        int side = bv->upd->vol->QualPlane(&clipOrg, &clipNrm);
        if (side == 1 || side == 0)
            return 0;
    }
    if (!(a->flags & 0x40000)) {
        m3dV c;
        camera->TransformPoint(&sph.center, &c);
        if (c.z > 1.0f) {
            float r = fabsf(camera->zNear / c.z * sph.radius);  // the sphere's radius on the near plane
            if (!(r >= 200.0f)) {
                float px = fabsf(((r - camera->vpX) * camera->scaleX + camera->winX) -
                                 (-camera->vpX * camera->scaleX + camera->winX));
                if (!(px >= 2.0f))
                    return 0;
            }
        }
    }
    *qual = q;
    return 1;
}
