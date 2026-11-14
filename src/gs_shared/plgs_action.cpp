// plgs_action.cpp -- plgsACTION, the base of the players (local and network): breadcrumbs, sync data, the local
// player pointer (Will Rock 1.2, v12.exe; feb obj gs_shared:plgs_action.obj)
//
// So far only the global. (plgsACTION::GetGravity, a header inline, is emitted in player.cpp.)

#include "../Sources/game.h"
#include "../gs/gs_mp_misc.h"
#include "../wr_vcall.h"

plgsACTION *plgsPlayer;                 // v12 0x009A84E0 (feb 0x0095D630) the local player (plrPLAYER::Create)

// v12 0x00546E40
// Playing: no cut-scene runs (gsSysCinema +0x80 bit 1) and none holds the player (the cinema's +0x88 cut-scene,
// its +0x16B, v12 0x00544C70 inline here).
BOOL32 plgsACTION::IsActive()
{
    if (WR_FIELD(u8, gsSysCinema, 0x80) & 1)
        return 0;
    int c = WR_FIELD(int, gsSysCinema, 0x88);
    return !(c ? WR_FIELD(int, c, 0x16B) : 0);
}

// v12 0x00546E70  (the twin table names the feb twin ProcessINIT_LEVEL; it runs every frame from the player's)
// Not a client: an actor that is a target (slot +0x88) is followed by the attack planner.
void plgsACTION::ProcessFRAME()
{
    if (gsSysMP->mode != 3 && wrEVCall<BOOL32>(this, 0x88))
        aiAttackPlanner->NotifyTarget(this);
}

// v12 0x00547430
// A player restored from the kept state (notify -100): the attack planner told.
void plgsACTION::ProcessMP_NOTIFY(void *d)
{
    u8 *m = (u8 *)d;
    if (*(short *)(m + 4) == -100)
        aiAttackPlanner->RestoreTarget(*(void **)(m + 6), this);
}
