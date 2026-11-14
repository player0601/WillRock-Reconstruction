// m3d_box.h -- m3dBOX private members missing from the SDK class (m3d_box.obj). Will Rock 1.2 reconstruction.
#pragma once
#include "m3d_lib.h"

// SDK gap: m3dBOX (math_app/m3d.h) does not declare these two members. They are reconstructed on a
// derived view class with no extra data; the original symbols are
//   ?GetEdgePoints@m3dBOX@@QAEXHPAUm3dV@@0@Z      v12 0x0056D3F0 (feb 0x00555390)
//   ?IsBelongPointFace@m3dBOX@@AAEHHPAUm3dV@@@Z   v12 0x0056DA40 (feb 0x005559E0)  (private)
class m3dBOX_EXT : public m3dBOX {
public:
    void   GetEdgePoints(int edge, m3dV *a, m3dV *b);   // the 12 edges of the box
    BOOL32 IsBelongPointFace(int face, m3dV *p);         // p inside the face rectangle (+-eps)
};

// box face numbering = index into m3dBoxNorm
enum m3dBOX_FACE {
    M3D_BOX_FACE_ZMIN = 0, M3D_BOX_FACE_XMAX = 1, M3D_BOX_FACE_ZMAX = 2,
    M3D_BOX_FACE_XMIN = 3, M3D_BOX_FACE_YMIN = 4, M3D_BOX_FACE_YMAX = 5
};

// SDK fix: m3dBoxNorm is an ARRAY of 6 outward face normals (MSVC mangles arrays as pointers, hence the
// SDK's "extern m3dV *m3dBoxNorm"). The data at v12 0x00637C68 is
// (0,0,-1) (1,0,0) (0,0,1) (-1,0,0) (0,-1,0) (0,1,0). Access it through this macro.
#define M3D_BOX_NORM ((m3dV *)&m3dBoxNorm)

// m3dSPHERE as a stack temporary (m3dBOX::CheckEnclSph builds one); the SDK class is abstract.
struct m3dSPHERE_TMP {
    const void *vtbl;           // m3dSPHERE vtable (v12 0x005FB68C)
    int   type;                 // +0x04
    m3dV  center;               // +0x08
    float radius;               // +0x14
    BOOL32 IsBelongPoint(m3dV *p);   // = m3dSPHERE::IsBelongPoint, v12 0x0056F670 (feb 0x00557560), direct call
};

// other m3d objs
void   __fastcall m3xzPoly3DToPoly2D(m3dPOLY *in, m2dPOLY *out);                       // v12 0x00570060
BOOL32 __fastcall m3dIsIsectYLinePlane(m3dV *p, m3dV *planePt, m3dV *n, m3dV *out);    // v12 0x00575C40
