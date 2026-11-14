// plw_amg.cpp -- pwpATOMICGUN, the atomic gun (weapon 10) (Will Rock 1.2, v12.exe; feb obj plw_amg.obj)
//
// Rebuilt so far: ReleaseShot (v12 0x00404E70), the one the player's input calls.

#include "game.h"

// v12 0x00404E70
// The fire key went up: a charge being loaded (state 1) fires after 0.7 s, one loaded (2) at once (state 3).
void pwpATOMICGUN::ReleaseShot()
{
    if (chargeState != 2) {
        if (chargeState != 1)
            return;
        chargeA = 0.7f;
    }
    chargeState = 3;
}
