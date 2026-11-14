// cam.h -- camCAMERA (m3d: cam.obj, cam_trans.obj, cam_qual.obj). Will Rock 1.2 reconstruction.
//
// The class itself is in the SDK (math_app/camera.h); this header adds the m3d-private declarations.
#pragma once
#include "m3d_lib.h"
#include "math_app/camera.h"


// m3d functions used by cam_*.obj with the argument order they really have
// (SDK m3d.h names the 4th/5th parameters of m3dIsectRayPlane the other way round: 4th = hit OUT, 5th = plane normal or 0).
