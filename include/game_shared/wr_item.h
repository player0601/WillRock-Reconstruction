// wr_item.h -- Will Rock 1.2 unofficial SDK, game_shared: the game's pickups (wr_item.obj): wrITEM, wrITEM_SPARKS
#pragma once
#include "world_objects.h"
#include "../engine_render/particle.h"
#pragma pack(push, 1)

// The item kinds (wrITEM::ProcessINIT, by template name; entENTITY +0x98 itemType). The amount is itemITEM::amount.
//   weapons  pwp_svl.. 1..10 (pwp_stg_item 2 / 10, pwp_mcg_item 3 / 100, pwp_crb_item 4 / 12, pwp_acd_item 5 / 10,
//            pwp_bsk_item 6 / 8, pwp_mdg_item 7 / 10, pwp_mng_item 8 / 200, pwp_bmb_item 9 / 1, pwp_amg_item 10 / 8)
//   ammo     0x14 stg 10, 0x15 mcg 50, 0x16 crb 6, 0x17 acd 5, 0x18 bsk 4, 0x19 mdg 9 (feb 5), 0x1A mng 100,
//            0x1B bmb 1 (feb 5), 0x1C amg 4
//   health   0x28 item_medkit1 10, 0x29 item_medkit / item_medkit2 25, 0x2A item_medkit3 100, 0x2B item_medkit4 200
//   armor    0x2C item_armor1 10, 0x2D item_armor2 25 / item_armor3 100, 0x2F item_armor4 200
//   0x30 item_money 10, 0x31 item_key, 0x32 item_quad_damage, 0x33 item_unholy_armor
//   treasure 0x34 item_treasure_small / item_treasure_helmet1 10, 0x35 _helmet 20, 0x36 _sword 30, 0x37 _shield 40,
//            0x38 _armor 50; 0x3C item_mpth* (the Treasure Hunt treasure)
class wrITEM : public itemITEM {
public:
    // vtable v12 0x00605EE8 (feb 0x005DA9B8)
    virtual BOOL32 ProcessMsg(int id, void *data, msgADDR *from, int *ret);    // +0x04 v12 0x004A96A0 (feb 0x0049B4E0)
    virtual BOOL32 ProcessINIT();                  // +0x64 v12 0x004A8900 (feb 0x0049A7F0)
    virtual void   ProcessFRAME();                 // +0x68 v12 0x004A9820 (feb 0x0049B5D0)
    virtual void   ProcessINIT_LEVEL();            // +0x70 v12 0x004A9240 (feb 0x0049B120)
    virtual void   ProcessMP_NOTIFY(void *ev);     // +0x74 v12 0x004A9770 (feb 0x0049B520)
    virtual void   ShowUnhideSfx();                // +0x7C v12 0x004A9500 (feb 0x0049B370)
    virtual BOOL32 CreateSfx(char *name);          // +0x80 v12 0x004A9D40 (feb 0x0049BB10)
    virtual void   DestroySfx();                   // +0x84 v12 0x004A9EA0 (feb 0x0049BC60)
    virtual void   ShowSfx(int on);                // +0x88 v12 0x004A9ED0 (feb 0x0049BC90)
    virtual void   Pick(plgsACTION *by);           // +0x90 v12 0x004A9310 (feb 0x0049B1D0)
    virtual BOOL32 IsCanPick(plgsACTION *by);      // +0x94 v12 0x004A92A0 (feb 0x0049B160)
    virtual void   ProcessDESTROY_ENT(entENTITY *e); // [38] +0x98 v12 0x004A97A0 (feb 0x0049B550)

    int               pickedBy[16];  // v12 +0x1AF (feb +0x193) MP "each player once": the mpIds that took it
    int               nPickedBy;     // v12 +0x1EF (feb +0x1D3)
    struct physMATH  *phys;          // v12 +0x1F3 (feb +0x1D7) Treasure Hunt: the thrown treasure's physics
    float             landTimer;     // v12 +0x1F7 (feb +0x1DB) Treasure Hunt: back to a default place when it runs out
    float             pickDelay;     // v12 +0x1FB (feb +0x1DF) -1; not pickable while > 0
    partEMITTER_PHYS *sparks;        // v12 +0x1FF (feb +0x1E3) the "sfx_item" sparks (wrITEM_SPARKS)
    // size 0x203

    static txmTEXTURE *pTexSparks;                       // v12 0x0064FF90 "sfx_item_spark"
    static entENTITY *__fastcall Create(animINST *inst); // v12 0x004A9F00 (feb 0x0049BCC0)
};
static_assert(sizeof(wrITEM) == 0x203, "wrITEM size (v12 0x203)");

// The sparks round an item ("sfx_item"): a partEMITTER_PHYS (size 0x2C4) with the base's slots, vtable v12 0x00605EB4.
class wrITEM_SPARKS : public partEMITTER_PHYS {
public:
    wrITEM_SPARKS(float size);                           // v12 0x004A9B70 (feb 0x0049B940)
};
// The sparks of a flame / an item appearing (sfx_flame.obj): partEMITTER_PHYS, size 0x2C0, vtable v12 0x00601B68.
class wrsfxFLAME_SMOKE : public partEMITTER_PHYS {
public:
    wrsfxFLAME_SMOKE(float size);                        // v12 0x00472D60 (feb 0x00469090)
};

class wrsfxFLAME_SPARKS : public partEMITTER_PHYS {
public:
    wrsfxFLAME_SPARKS(float size);                       // v12 0x00472B40 (feb 0x00468E70)
};
int __fastcall wrItemInit(void);                         // v12 0x004A88B0 (feb 0x0049A7A0)

#pragma pack(pop)
