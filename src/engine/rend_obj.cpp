// rend_obj.cpp -- the render driver's object side (Will Rock 1.2, v12.exe; feb obj engine:Rend_obj.obj)
//
// Rebuilt so far: QualObj, the test an object passes before it is drawn. Not rebuilt: RenderObj, _RenderObj,
// RenderObjListStatic (rend_driver.h lists them).

#include "engine.h"
#include "../../include/engine_render/rend_driver.h"
#include "../../include/math_app/camera.h"
#include "../wr_vcall.h"

// v12 0x004FC400  (feb 0x004E9EF0)
// Whether an object is drawn: 3 = neither it nor its children (hidden: procState 0x1C; the driver's mirror pass
// (flags 8) skips objects flagged 0x100, the main pass those flagged 0x200; with flags 0x100 only those of
// an instance; or a static object outside the view), 1 = not it but its children (procState 0x23), else what the
// driver's FilterObj says, 0 = drawn. A static object with faces is tested against the view frustum by its box --
// unless its parent was already inside it all (qualIn -1); *qualOut is the result for its children (1 outside, -1
// inside, 0 across). With the clip plane on (flags 0x10) a static object behind it is not drawn either; one across
// it is flagged 0x100 (clipped while drawn).
int rendDRIVER::QualObj(objOBJ *o, int qualIn, int *qualOut)
{
    *qualOut = qualIn;
    if (o->procState & 0x1C)
        return 3;
    if (o->procState & 0x3F)
        return 1;
    if (flags & 8) {
        if (o->procState & 0x100)
            return 3;
    } else if (o->procState & 0x200)
        return 3;
    if ((flags & 0x100) && !o->inst)
        return 3;
    if (int r = wrVCall<int>(this, 0x18, o))            // FilterObj
        return r;
    if (o->nFace > 0 && !o->inst) {
        int q = qualIn;
        if (q != -1)
            q = camera->QualBoxPOV((m3dBOX *)&o->bbox);
        *qualOut = q;
        if (q == 1)
            return 3;
        if (flags & 0x10) {
            int side = wrVCall<int>(&o->bbox, 0x30, &clipOrg, &clipNrm);   // m3dVOL::QualPlane
            if (side == 1)
                return 3;
            if (side == 0) {
                o->state |= 0x100;
                return 0;
            }
            o->state &= ~0x100u;
        }
    }
    return 0;
}
