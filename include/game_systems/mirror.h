// mirror.h -- the mirrors and portals of a level (Will Rock 1.2, v12.exe; feb gs:mirror.obj)
#ifndef WR_MIRROR_H
#define WR_MIRROR_H

#pragma pack(push, 1)
struct mrrMIRROR {                                      // size 0x38
    u32       flags;        // +0x00 1 a portal, 0x10 isClipping, 0x40 isRefProjShd
    objOBJ   *obj;          // +0x04
    m3dCOLOR  color;        // +0x08 [MIRROR] / [PORTAL] color
    int       priority;     // +0x18 drawn in its order (_mrrCmpMirror)
    animINST *actor;        // +0x1C a portal: the instance it looks out of ([PORTAL] actor_name)
    m3dV      normal;       // +0x20 world, PrepareNormals
    m3dV      point;        // +0x2C world, its first vertex
};
#pragma pack(pop)

#define MRR_MAX_MIRRORS 20

class mrrMIRROR_SYSTEM : public msgADDR {               // size 0x4410, one more dword in 1.2's allocation
public:
    int       addrLock;                 // +0x24 msgADDR's lock count
    int       nMirrors;                 // +0x28
    mrrMIRROR mirror[MRR_MAX_MIRRORS];  // +0x2C
    int       depth;                    // +0x48C Render's recursion: 1 the view, 2 what a mirror sees
    int       curMirror;                // +0x490 the mirror the level 1 is drawing
    camCAMERA mainCam;                  // +0x494 the view's camera
    camCAMERA cam[30];                  // +0x6A8 (not used)

    mrrMIRROR_SYSTEM();                                                 // v12 0x0051DCB0 (feb 0x00508B60)
    virtual ~mrrMIRROR_SYSTEM();                                        // v12 0x0051DD50 (feb 0x00508C00)
    BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);     // v12 0x0051DDE0 (feb 0x00508C90)

    int  Init();                                                        // (inlined in 1.2) feb 0x00508C50
    void Term();                                                        // v12 0x0051DDC0 (feb 0x00508C70)
    int  InitObj(objOBJ *o);                                            // v12 0x0051DE00 (feb 0x00508CB0)
    void TermObj(objOBJ *o);                                            // v12 0x0051E0D0 (feb 0x00508F80)
    int  GetNActive();                                                  // v12 0x0051E160 (feb 0x00509010)
    int  Render(camCAMERA *cam);                                        // v12 0x0051E180 (feb 0x00509030)

private:
    void PrepareNormals();                                              // v12 0x0051EE00 (feb 0x00509B20)
    void SetVisFlags(camCAMERA *cam, int second, u32 *st);              // v12 0x0051F090 (feb 0x00509DB0)
    void MaskMirrors(camCAMERA *cam, u32 *st, int mask, int polys, int count, int skip);    // v12 0x0051F410
    void RenderMirrorObjects(camCAMERA *cam, u32 *st, int mask, int useColor, int only, int skip);  // v12 0x0051F810
};

#endif
