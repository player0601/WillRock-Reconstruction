// plw_mdg.cpp -- pwpMEDUSA, the Medusa gun (weapon 7): petrifying gas balloons, 3 to a clip, the empty balloon
// thrown out on reload (Will Rock 1.2, v12.exe; feb obj plw_mdg.obj)
//
// Rebuilt from the 1.2 code (v12 0x00410380..0x004121C0, vtable 0x005FA850): the class, its INIT / FRAME / sound
// slots and the reload's balloon. Not rebuilt: the shot (Shoot +0x68 v12 0x004106F0, ProcessSHOOT +0x98 v12
// 0x004107D0) and the gas cloud "sfx_mdg_pjl" (Create v12 0x004104B0) with its emitters.

#include "game.h"

int __fastcall sncLinkSndListData(int n, sncSOUND_DESCR_3D *list); // v12 0x004B1E30
entENTITY *__fastcall pwpMDG_PJL_Create(animINST *i);  // v12 0x004104B0 "sfx_mdg_pjl" -- not rebuilt

sncSOUND_DESCR_3D pwpMdgSnd[2];                         // v12 0x00640E48 shot, recharge
txmTEXTURE *pwpMEDUSA::pCloudTex;                       // v12 0x00640F18 "sfx_mdg_cloud"

// v12 0x00410380  static initialiser (the sncSOUND_DESCR_3D constructor inline)
static struct pwpMDG_SND_INIT {
    pwpMDG_SND_INIT()
    {
        static const char *names[2] = { "wpn_medusa_gun_shoot", "wpn_medusa_gun_recharge" };
        for (int i = 0; i < 2; i++) {
            sncSOUND_DESCR_3D *d = &pwpMdgSnd[i];
            d->name.SetName((char *)names[i]);
            d->flags = 0;
            d->volume = 0.0f;
            d->delay = 0;
            d->count = 0;
            d->time = 0;
            d->data = 0;
            d->dataFlags = 0;
            d->minDist = 0.0f;
            d->maxDist = 0.0f;
            d->vtbl = 0;                    // (sncSOUND_DESCR_3D vtable v12 0x005F9008)
        }
    }
} pwpMdgSndInit;

// v12 0x00410430  (feb 0x0040EDD0)
int __fastcall pwpMDGInit()
{
    if (!entRegisterClass((char *)"pwp_mdg", 'PMDG', 0x50, pwpMEDUSA::Create, 0, 1))
        return 0;
    if (!entRegisterClass((char *)"sfx_mdg_pjl", 'PMDG', 0x2D, pwpMDG_PJL_Create, 0, 1))
        return 0;
    pwpMEDUSA::pCloudTex = txmManager->Add((char *)"sfx_mdg_cloud", 0x40003, 1);
    if (!pwpMEDUSA::pCloudTex)
        return 0;
    return sncLinkSndListData(2, pwpMdgSnd) != 0;
}

// v12 0x00410540  (feb 0x0040EEE0)
// Event 1: the shot (SHOOT's action frame 2); event 2: the recharge (RECHARGE / FORCE_RECHARGE's action frame 1).
void pwpMEDUSA::Anim2Sound()
{
    if (inst->IsActionFrame(7, 2)) {
        unk108 |= 1;
        return;
    }
    if (inst->IsActionFrame(8, 1) || inst->IsActionFrame(9, 1))
        unk108 |= 2;
}

// v12 0x004105A0  (feb 0x0040EF40)
void pwpMEDUSA::UpdateSound()
{
    if (unk108 & 1) {
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpMdgSnd[0], 0);
    } else if (unk108 & 2) {
        ((sncCHANNEL *)sndChannel)->Play((sncSOUND_DESCR *)&pwpMdgSnd[1], 0);
    }
    pwpWEAPON::UpdateSound();
}

// v12 0x004105F0  (feb 0x0040EF90)
// 3 balloons a clip (action frames: shot 0, recharge 1), 1 shot a second, damage 50, 50 at most; the model has no
// collision and starts hidden; no ammo at first; its normals for the lighting.
int pwpMEDUSA::ProcessINIT()
{
    animCREATE_DATA cd;                                 // (constructed and unused in the game)
    if (!pwpWEAPON::ProcessINIT())
        return 0;
    clipSize = 3;
    clip = 3;
    flags2 |= 3;
    shootActFrame = 0;
    reloadActFrame = 1;
    SetShootRate(1.0f);
    damage = 50.0f;
    inst->flags |= 0x10;
    inst->flags |= 1;
    SetAmmo(0.0f);
    maxAmmo = 50.0f;
    inst->root->AllocVertNormalList();
    return 1;
}

// v12 0x004106B0  (feb 0x0040F050)
// The empty balloon thrown out on RECHARGE / FORCE_RECHARGE's action frame 2.
void pwpMEDUSA::ProcessFRAME()
{
    pwpWEAPON::ProcessFRAME();
    if (inst->flags & 1)
        return;
    if (inst->IsActionFrame(8, 2) || inst->IsActionFrame(9, 2))
        ThrowBalloon();
}

// v12 0x004109D0  (feb ?)
// As the colt's clip: the "ballon" object flies out from beside the owner as a rigid "pwp_mdg_clip", tinted like
// the weapon's light, deleted 5 s later.
void pwpMEDUSA::ThrowBalloon()
{
    u8 init[0xB8];                          // the rigid body's init data (physics), as the game fills it
    memset(init, 0, sizeof(init));
    init[1] = 0x40;
    *(u16 *)(init + 2) = 0xB8;
    *(int *)(init + 4) = 8;
    *(m3dV *)(init + 8) = m3dVZero;
    *(m3dV *)(init + 0x14) = m3dVZero;
    *(float *)(init + 0x98) = 0.0f;
    *(float *)(init + 0x9C) = 0.6f;
    *(float *)(init + 0xA0) = 0.8f;
    *(float *)(init + 0xA4) = 10.0f;
    *(float *)(init + 0xA8) = 0.3f;
    *(float *)(init + 0xAC) = -1.0f;
    *(float *)(init + 0xB0) = -1.0f;
    *(float *)(init + 0xB4) = -1.0f;
    animCREATE_DATA cd;
    if (!objFindName(inst->root, (char *)"ballon"))
        return;
    owner->inst->GetMatrL2W(&cd.matr);
    cd.matr.Translate(-0.5f, 1.3f, 0.1f, 1);
    cd.scale.x = cd.scale.y = cd.scale.z = 0.035f;
    cd.matr.GetAxisZ((m3dV *)(init + 8));
    *(float *)(init + 0xC) = -0.3f;                     // (written after the four entCreate pushes)
    *(float *)(init + 0xA0) = 0.5f;
    *(float *)(init + 0x9C) = 0.4f;
    entENTITY *c = entCreate(gsScenePtr, (char *)"rigid", (char *)"pwp_mdg_clip", &cd, init, 0);
    if (!c)
        return;
    c->inst->root->SetConstColor(colDiffuse, 2, 8);
    c->inst->flags |= 0x10;
    msgSystem.PostTimeMsg(c, MSG_MSG_TERM, 0, 5.0f);
}

// v12 0x00412170  (feb 0x004107C0)
entENTITY *__fastcall pwpMEDUSA::Create(animINST *)
{
    pwpMEDUSA *w = new pwpMEDUSA;           // operator new(0x1CB), pwpWEAPON ctor
    if (!w)
        return 0;
    w->sndChannel = 0;
    w->owner = 0;
    w->unk1C3 = 1.0f;
    w->wpnIndex = PWP_MEDUSA;
    return w;
}
