// pjl_arrow.h -- pjlARROW_GENERIC, the arrow of the satyr and (with its own sounds) of the amur (Will Rock 1.2)
// The code: src/Sources/ai_satyr.cpp (ai_satyr_arrow_pjl.obj); pjlAMUR_ARROW: src/Sources/ai_amur.cpp.
#pragma once
#include "../game_shared/projectile.h"

partEMITTER_PHYS *__fastcall pjlArrowNewTrail(float s);  // (ai_satyr.cpp) the trail emitter, pjlARROW_TRAIL

struct pjlARROW_GENERIC_SFX {                           // 8 bytes, at pjlPJL +0x16F
    partEMITTER_PHYS *emitter;
    objOBJ           *trailObj;                         // "trail_emitter"
    pjlARROW_GENERIC_SFX() : emitter(0), trailObj(0) {}

    // v12 0x00457930
    int StartSFX(physPHYS *p, pjlPJL_INIT *)
    {
        animINST *inst = p->inst;
        emitter = pjlArrowNewTrail(0.1f);
        if (!emitter)
            return 0;
        trailObj = objFindName(inst->root, (char *)"trail_emitter");
        if (!trailObj)
            return 0;
        if (emitter) {
            p->inst->Validate(4);
            m3dV o, v;
            trailObj->GetOrigin(&o);
            v = *(m3dV *)p->math->vel;
            emitter->SetPosDirSpeed(&o, 0, &v);
            emitter->StartEmission();
        }
        return 1;
    }
    // v12 0x00457A00
    void UpdateSFX(physPHYS *p)
    {
        if (!emitter)
            return;
        p->inst->Validate(4);
        m3dV o, v;
        trailObj->GetOrigin(&o);
        v = *(m3dV *)p->math->vel;
        emitter->SetPosDirSpeed(&o, 0, &v);
    }
    // v12 0x00447270 (shared) stopped: it goes by itself once its particles are gone
    void StopSFX(physPHYS *)
    {
        if (emitter) {
            emitter->StopEmission();
            emitter = 0;
        }
    }
};

#pragma pack(push, 1)
class pjlARROW_GENERIC : public pjlNPC_PJL<pjlARROW_GENERIC_SFX> {
public:
    objOBJ  *rotCenter;                                 // +0x1BC "rot_center": what turns and quivers
    int      layerState;                                // +0x1C0 2: quivering (stuck)
    float    vibTimer;                                  // +0x1C4 0.6 s of quivering
    m3dMATR  savedLocal;                                // +0x1C8 rotCenter's local as it stuck
    // size v12 0x208

    pjlARROW_GENERIC() : rotCenter(0), layerState(0), vibTimer(0) { savedLocal.Identity(); }
    static entENTITY *__fastcall Create(animINST *i);
    BOOL32 ProcessINIT(physINIT *p);
    void   ProcessFRAME();
    void   AlignWithSpeed();
    void   Vibrate();
    void   ShowCollisSFX(pjlEV_CDT *ev);
    sncSOUND_DESCR_3D *GetSound(int i);
    int    GetLayerAppState() { return layerState; }    // v12 0x0045BC80
    void   SetLayerAppState(int s) { layerState = s; }  // v12 0x00438A00
};
#pragma pack(pop)
